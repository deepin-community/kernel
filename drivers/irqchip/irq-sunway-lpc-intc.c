// SPDX-License-Identifier: GPL-2.0
#include <linux/bitops.h>
#include <linux/irq.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/irqchip.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/interrupt.h>

#define LPC_NR_IRQS 16
#define	LPC_IRQ  0x4
#define	LPC_IRQ_MASK  0x8

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

static const struct irq_domain_ops sw64_lpc_domain_ops = {
	.map = sw64_lpc_domain_map,
	.xlate = irq_domain_xlate_onecell,
};

struct device_node *sw_lpc_intc_node;
EXPORT_SYMBOL(sw_lpc_intc_node);

static int __init lpc_intc_of_init(struct device_node *np,
				  struct device_node *parent)
{
	struct irq_domain *lpc_domain;
	int ret;
	void __iomem *base;

	sw_lpc_intc_node = np;

	if (!parent) {
		pr_err("no parent intc found\n");
		return -ENXIO;
	}

	base = of_iomap(np, 0);
	if (!base) {
		pr_err("failed to remap lpc intc registers\n");
		ret = -ENOMEM;
		goto out_free;
	}

	parent_irq = irq_of_parse_and_map(np, 0);
	if (!parent_irq) {
		pr_err("failed to find parent interrupt\n");
		ret = -EINVAL;
		goto out_unmap;
	}

	lpc_domain = irq_domain_add_legacy(np, LPC_NR_IRQS,
					    0, 0, &sw64_lpc_domain_ops, base);
	if (!lpc_domain) {
		ret = -ENOMEM;
		goto out_unmap;
	}

	/* Set the IRQ chaining logic */
	irq_set_chained_handler_and_data(parent_irq,
					 lpc_irq_handler, lpc_domain);

	return 0;

out_unmap:
	iounmap(base);
out_free:
	return ret;
}
IRQCHIP_DECLARE(sw_lpc_intc, "sw64,lpc_intc", lpc_intc_of_init);
