// SPDX-License-Identifier: GPL-2.0
#include <linux/bitops.h>
#include <linux/irq.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/irqchip.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/interrupt.h>

#define PREFIX  "LPC-INTC: "

#define	LPC_IRQ  0x4
#define	LPC_IRQ_MASK  0x8

#define SW_LPC_INTC_GSI_BASE 256

static DEFINE_RAW_SPINLOCK(lpc_lock);

static int parent_irq;

static unsigned int cached_irq_mask = 0xffffffff;
static void lpc_irq_mask(struct irq_data *irq_data)
{
	void __iomem *base = irq_data->domain->host_data;
	unsigned long flags;
	u32 mask = 1 << (irq_data->irq);

	raw_spin_lock_irqsave(&lpc_lock, flags);
	cached_irq_mask |= mask;
	writel(cached_irq_mask, base + LPC_IRQ_MASK);
	raw_spin_unlock_irqrestore(&lpc_lock, flags);
}

static void lpc_irq_unmask(struct irq_data *irq_data)
{
	void __iomem *base = irq_data->domain->host_data;
	unsigned long flags;
	u32 mask = 1 << (irq_data->irq);

	raw_spin_lock_irqsave(&lpc_lock, flags);
	cached_irq_mask &= ~mask;
	writel(cached_irq_mask, base + LPC_IRQ_MASK);
	raw_spin_unlock_irqrestore(&lpc_lock, flags);
}

static void lpc_irq_mask_ack(struct irq_data *irq_data)
{
	void __iomem *base = irq_data->domain->host_data;
	unsigned long flags;
	u32 mask = 1 << (irq_data->irq);

	raw_spin_lock_irqsave(&lpc_lock, flags);
	cached_irq_mask |= mask;
	writel(cached_irq_mask, base + LPC_IRQ_MASK);
	writel(mask, base + LPC_IRQ);
	raw_spin_unlock_irqrestore(&lpc_lock, flags);
}

static struct irq_chip sw64_lpc_chip = {
	.name			= "LPC-INT",
	.irq_mask		= lpc_irq_mask,
	.irq_unmask		= lpc_irq_unmask,
	.irq_mask_ack		= lpc_irq_mask_ack,
	.irq_set_affinity	= irq_chip_set_affinity_parent,
};

static void lpc_irq_handler(struct irq_desc *desc)
{
	struct irq_domain *domain = irq_desc_get_handler_data(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	void __iomem *base = domain->host_data;
	unsigned int irq;
	u32 status;

	chained_irq_enter(chip, desc);

	status = readl(base + LPC_IRQ);

	if (status == 0) {
		handle_bad_irq(desc);
		goto out;
	}

	while (status) {
		irq = __ffs(status);
		status &= ~BIT(irq);
		generic_handle_irq(irq_find_mapping(domain, irq));
	}

out:
	chained_irq_exit(chip, desc);
}

static int sw64_lpc_domain_map(struct irq_domain *d, unsigned int virq,
				irq_hw_number_t hw)
{
	struct irq_data *irq_data, *parent_data;

	irq_data = irq_domain_get_irq_data(d, virq);
	parent_data = irq_get_irq_data(parent_irq);
	if (!parent_data) {
		pr_warn("Failed to get lpc parent irq data!\n");
		return -EFAULT;
	}

	irq_data->parent_data = parent_data;

	irq_set_chip_and_handler(virq, &sw64_lpc_chip, handle_level_irq);
	irq_set_probe(virq);
	irq_set_status_flags(virq, IRQ_LEVEL);

	return 0;
}

static int lpc_intc_translate(struct irq_domain *domain,
		struct irq_fwspec *fwspec,
		unsigned long *hwirq,
		unsigned int *type)
{
	if (WARN_ON(fwspec->param_count < 1))
		return -EINVAL;

	/* Device tree */
	if (is_of_node(fwspec->fwnode)) {
		*hwirq = fwspec->param[0];
		*type = IRQ_TYPE_NONE;
		return 0;
	}

	/* ACPI */
	if (is_fwnode_irqchip(fwspec->fwnode)) {
		if (WARN_ON(fwspec->param[0] < SW_LPC_INTC_GSI_BASE))
			return -EINVAL;
		*hwirq = fwspec->param[0] - SW_LPC_INTC_GSI_BASE;
		*type = IRQ_TYPE_NONE;
		return 0;
	}

	return -EINVAL;
}

static const struct irq_domain_ops sw64_lpc_domain_ops = {
	.map = sw64_lpc_domain_map,
	.translate = lpc_intc_translate,
};

static struct irq_domain *lpc_irq_domain;

static int __init lpc_intc_init(struct fwnode_handle *handle,
		unsigned int irqnr, int parent_irq, void __iomem *base_addr)
{
	/**
	 * The current kernel does not support "irq_domain_create_legacy",
	 * we have to call "__irq_domain_add" directly.
	 */
	lpc_irq_domain = __irq_domain_add(handle, irqnr, irqnr,
			0, &sw64_lpc_domain_ops, base_addr);
	if (!lpc_irq_domain) {
		pr_info(PREFIX "failed to create irq domain\n");
		return -ENOMEM;
	}

	irq_domain_associate_many(lpc_irq_domain, 0, 0, irqnr);

	/* Set the IRQ chaining logic */
	irq_set_chained_handler_and_data(parent_irq,
			lpc_irq_handler, lpc_irq_domain);

	return 0;
}

#ifdef CONFIG_OF
struct device_node *sw_lpc_intc_node;
EXPORT_SYMBOL(sw_lpc_intc_node);

static int __init lpc_intc_of_init(struct device_node *np,
				  struct device_node *parent)
{
	int ret;
	u32 nr_irqs, node, version;
	void __iomem *base;

	if (WARN_ON(!np || !parent))
		return -ENODEV;

	sw_lpc_intc_node = np;

	ret = of_property_read_u32(np, "sw64,node", &node);
	if (ret) {
		node = 0;
		pr_warn(PREFIX "\"sw64,node\" fallback to %u\n",
				node);
	}

	ret = of_property_read_u32(np, "sw64,irq-num", &nr_irqs);
	if (ret) {
		nr_irqs = 16;
		pr_warn(PREFIX "\"sw64,irq-num\" fallback to %u\n",
				nr_irqs);
	}

	ret = of_property_read_u32(np, "sw64,ver", &version);
	if (ret) {
		version = 1;
		pr_warn(PREFIX "\"sw64,ver\" fallback to %u\n",
				version);
	}

	base = of_iomap(np, 0);
	if (!base) {
		pr_err(PREFIX "failed to remap lpc intc registers\n");
		return -ENXIO;
	}

	parent_irq = irq_of_parse_and_map(np, 0);
	if (!parent_irq) {
		pr_err(PREFIX "failed to find parent interrupt\n");
		ret = -EINVAL;
		goto out_unmap;
	}

	ret = lpc_intc_init(of_node_to_fwnode(np), nr_irqs, parent_irq, base);
	if (ret)
		goto out_unmap;

	pr_info(PREFIX "version [%u] on node [%u] initialized\n",
			version, node);

	return 0;

out_unmap:
	iounmap(base);
	return ret;
}
IRQCHIP_DECLARE(sw_lpc_intc, "sw64,lpc_intc", lpc_intc_of_init);
#endif

#ifdef CONFIG_ACPI
#define SW_LPC_INTC_FLAG_ENABLED    ACPI_MADT_ENABLED /* 0x1 */

#define is_lpc_intc_enabled(flags)  ((flags) & SW_LPC_INTC_FLAG_ENABLED)

int __init lpc_intc_acpi_init(struct irq_domain *parent,
		struct acpi_madt_sw_lpc_intc *lpc_intc)
{
	struct fwnode_handle *handle;
	struct irq_fwspec fwspec;
	void __iomem *base_addr;
	int ret;
	bool enabled;

	enabled = is_lpc_intc_enabled(lpc_intc->flags);
	pr_info(PREFIX "version [%u] on node [%u] %s\n",
			lpc_intc->version, lpc_intc->node,
			enabled ? "found" : "disabled");
	if (!enabled)
		return 0;

	if (lpc_intc->gsi_base != SW_LPC_INTC_GSI_BASE) {
		pr_err(PREFIX "invalid GSI\n");
		return -EINVAL;
	}

	handle = irq_domain_alloc_named_id_fwnode("LPC-INTC", lpc_intc->node);
	if (!handle) {
		pr_err(PREFIX "failed to alloc fwnode\n");
		return -ENOMEM;
	}

	fwspec.fwnode = parent->fwnode;
	fwspec.param[0] = lpc_intc->cascade_vector;
	fwspec.param_count = 1;

	parent_irq = irq_create_fwspec_mapping(&fwspec);
	if (parent_irq <= 0) {
		pr_err(PREFIX "failed to map parent irq\n");
		ret = -EINVAL;
		goto out_acpi_free_fwnode;
	}

	base_addr = ioremap(lpc_intc->address, lpc_intc->size);
	if (!base_addr) {
		pr_err(PREFIX "failed to map base address\n");
		ret = -ENXIO;
		goto out_acpi_free_fwnode;
	}

	ret = lpc_intc_init(handle, lpc_intc->gsi_count,
			parent_irq, base_addr);
	if (ret)
		goto out_acpi_unmap;

	ret = sw64_add_gsi_domain_map(lpc_intc->gsi_base,
			lpc_intc->gsi_count, handle);
	if (ret) {
		pr_info(PREFIX "failed to add GSI map\n");
		goto out_acpi_free_lpc_domain;
	}

	pr_info(PREFIX "version [%u] on node [%u] initialized\n",
			lpc_intc->version, lpc_intc->node);

	return 0;

out_acpi_free_lpc_domain:
	irq_domain_remove(lpc_irq_domain);
out_acpi_unmap:
	iounmap(base_addr);
out_acpi_free_fwnode:
	irq_domain_free_fwnode(handle);
	return ret;
}
#endif
