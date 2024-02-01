// SPDX-License-Identifier: GPL-2.0
#include <linux/irqdomain.h>
#include <linux/irqchip.h>
#include <linux/acpi.h>
#include <linux/acpi_iort.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/topology.h>

#define PREFIX  "PINTC: "

#define OFFSET_MCU_DVC_INT_EN  0x3080UL

#define OFFSET_DEV_INT_CONFIG  0x480UL

struct pintc_chip_data {
	bool vt;                  /* virtual pintc */
	u32 node;                 /* node ID */
	u32 version;              /* PINTC version */
	void __iomem *pintc_base; /* INTPU base address */
	void __iomem *mcu_base;   /* MCU/SPBU base address */
	struct irq_chip *mcu_chip;
};

static DEFINE_RAW_SPINLOCK(pintc_lock);
static void lock_dev_lock(void)
{
	raw_spin_lock(&pintc_lock);
}

static void unlock_dev_lock(void)
{
	raw_spin_unlock(&pintc_lock);
}

static void mcu_irq_mask(struct irq_data *data)
{
	struct pintc_chip_data *chip_data = data->chip_data;
	unsigned long mask;
	int hwirq = data->hwirq;

	mask = readq(chip_data->mcu_base + OFFSET_MCU_DVC_INT_EN);
	mask &= ~(0x1UL << hwirq);
	writeq(mask, chip_data->mcu_base + OFFSET_MCU_DVC_INT_EN);
}

static void mcu_irq_unmask(struct irq_data *data)
{
	struct pintc_chip_data *chip_data = data->chip_data;
	unsigned long mask;
	int hwirq = data->hwirq;

	mask = readq(chip_data->mcu_base + OFFSET_MCU_DVC_INT_EN);
	mask |= (0x1UL << hwirq);
	writeq(mask, chip_data->mcu_base + OFFSET_MCU_DVC_INT_EN);
}

static void mcu_irq_enable(struct irq_data *irq_data)
{
	struct pintc_chip_data *chip_data = irq_data->chip_data;
	unsigned long devint_conf;

	devint_conf = readq(chip_data->pintc_base + OFFSET_DEV_INT_CONFIG);
	devint_conf |= (1UL << 8);
	writeq(devint_conf, chip_data->pintc_base + OFFSET_DEV_INT_CONFIG);
	mcu_irq_unmask(irq_data);
}

static void mcu_irq_disable(struct irq_data *irq_data)
{
	struct pintc_chip_data *chip_data = irq_data->chip_data;
	unsigned long devint_conf;

	devint_conf = readq(chip_data->pintc_base + OFFSET_DEV_INT_CONFIG);
	devint_conf &= ~(1UL << 8);
	writeq(devint_conf, chip_data->pintc_base + OFFSET_DEV_INT_CONFIG);
	mcu_irq_mask(irq_data);
}

static unsigned long make_mcu_int_target(u32 version, int rcid)
{
	int node, core, thread;
	unsigned long target = 0;

	thread = rcid_to_thread_id(rcid);
	core = rcid_to_core_id(rcid);
	node = rcid_to_domain_id(rcid);

	switch (version) {
	case 0x1: /* PINTC v1 */
		target = core | (thread << 5) | (node << 6);
		break;
	case 0x2: /* PINTC v2 */
		target = core | (thread << 6) | (node << 7);
		break;
	default:
		break;
	}

	return target;
}

static int __assign_mcu_irq_config(const struct pintc_chip_data *chip_data,
		cpumask_t *targets)
{
	unsigned long dev_int_tar, val;
	unsigned int cpu;
	int rcid;

	for_each_cpu(cpu, targets) {
		/*
		 * Hardware requires dev ints be redirected to on-node
		 * cores only. Thus, we remove all off-node cpu in the
		 * target mask.
		 */
		if (cpu_to_node(cpu) != chip_data->node)
			cpumask_clear_cpu(cpu, targets);
	}

	/* Use the last one in valid cpus to avoid core 0. */
	cpu = cpumask_last(targets);
	if (cpu >= nr_cpu_ids)
		return -EPERM;

	rcid = cpu_to_rcid(cpu);

	val = readq(chip_data->pintc_base + OFFSET_DEV_INT_CONFIG);
	dev_int_tar = make_mcu_int_target(chip_data->version, rcid);
	val &= 0xffff;
	val |= dev_int_tar << 16;
	writeq(val, chip_data->pintc_base + OFFSET_DEV_INT_CONFIG);

	return 0;
}

static int assign_mcu_irq_config(const struct pintc_chip_data *chip_data,
		cpumask_t *targets)
{
	int ret;

	lock_dev_lock();
	ret = __assign_mcu_irq_config(chip_data, targets);
	unlock_dev_lock();

	return ret;
}

static int mcu_irq_set_affinity(struct irq_data *irq_data,
				 const struct cpumask *dest, bool force)
{
	struct pintc_chip_data *chip_data = irq_data->chip_data;
	cpumask_t targets;
	int ret = 0;

	if (cpumask_any_and(dest, cpu_online_mask) >= nr_cpu_ids)
		return -EINVAL;

	cpumask_and(&targets, dest, cpu_online_mask);

	mcu_irq_disable(irq_data);
	ret = assign_mcu_irq_config(chip_data, &targets);
	mcu_irq_enable(irq_data);

	return ret;
}

static struct irq_chip pintc_mcu_chip = {
	.name			= "MCU-INT",
	.irq_enable		= mcu_irq_enable,
	.irq_disable		= mcu_irq_disable,
	.irq_mask		= mcu_irq_mask,
	.irq_unmask		= mcu_irq_unmask,
	.irq_set_affinity	= mcu_irq_set_affinity,
};

static struct irq_chip pintc_mcu_vt_chip = {
	.name                   = "VMCU-INT",
};

static void pintc_mcu_free_irqs(struct irq_domain *irq_domain,
				unsigned int virq, unsigned int nr_irqs)
{
	int i = 0;

	irq_domain_free_irqs_top(irq_domain, virq, nr_irqs);

	for (i = 0; i < nr_irqs; i++)
		irq_clear_status_flags(virq + i, IRQ_LEVEL);
}

static int pintc_mcu_map_irq(struct irq_domain *domain,
		unsigned int virq, irq_hw_number_t hwirq)
{
	struct pintc_chip_data *chip_data = domain->host_data;

	irq_domain_set_info(domain, virq, hwirq,
			chip_data->mcu_chip, chip_data,
			handle_level_irq, NULL, NULL);
	irq_set_status_flags(virq, IRQ_LEVEL);

	return 0;
}

static int pintc_mcu_alloc_irqs(struct irq_domain *domain,
				unsigned int virq,
				unsigned int nr_irqs,
				void *arg)
{
	struct irq_fwspec *fwspec = arg;
	irq_hw_number_t hwirq = fwspec->param[0];
	int i;

	for (i = 0; i < nr_irqs; i++)
		pintc_mcu_map_irq(domain, virq + i, hwirq + i);

	return 0;
}

static const struct irq_domain_ops pintc_mcu_domain_ops = {
	.map = pintc_mcu_map_irq,
	.xlate = irq_domain_xlate_onecell,
	.alloc = pintc_mcu_alloc_irqs,
	.free = pintc_mcu_free_irqs,
};

struct irq_domain *mcu_irq_domain;
EXPORT_SYMBOL(mcu_irq_domain);

#ifdef CONFIG_OF
static int __init
pintc_of_init_common(struct device_node *pintc,
		struct device_node *parent, bool vt)
{
	int ret;
	u32 nr_irqs, node, version;
	void __iomem *pintc_base;
	void __iomem *mcu_base;
	struct pintc_chip_data *chip_data;

	if (WARN_ON(!pintc))
		return -ENODEV;

	if (vt && parent) {
		pr_err(PREFIX "virtual pintc has no parent controller\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(pintc, "sw64,node", &node);
	if (ret) {
		pr_err(PREFIX "\"sw64,node\" not found\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(pintc, "sw64,irq-num", &nr_irqs);
	if (ret) {
		pr_err(PREFIX "\"sw64,irq-num\" not found\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(pintc, "sw64,ver", &version);
	if (ret) {
		pr_err(PREFIX "\"sw64,ver\" not found\n");
		return -EINVAL;
	}

	pintc_base = of_iomap(pintc, 0);
	if (!vt && !pintc_base) {
		pr_err(PREFIX "failed to map pintc base address\n");
		return -ENXIO;
	}

	mcu_base = of_iomap(pintc, 1);
	if (!vt && !mcu_base) {
		pr_err(PREFIX "failed to map mcu base address\n");
		ret = -ENXIO;
		goto out_unmap0;
	}

	chip_data = kzalloc_node(sizeof(*chip_data), GFP_KERNEL, node);
	if (!chip_data) {
		ret = -ENOMEM;
		goto out_unmap1;
	}

	chip_data->vt = vt;
	chip_data->node = node;
	chip_data->version = version;
	chip_data->pintc_base = pintc_base;
	chip_data->mcu_base = mcu_base;

	if (vt) {
		chip_data->mcu_chip = &pintc_mcu_vt_chip;
		mcu_irq_domain = irq_domain_add_legacy(pintc, nr_irqs, 0, 0,
					&pintc_mcu_domain_ops, chip_data);
	} else {
		chip_data->mcu_chip = &pintc_mcu_chip;
		mcu_irq_domain = irq_domain_add_linear(pintc, nr_irqs,
					&pintc_mcu_domain_ops, chip_data);
		/* mask all interrupts for now */
		writeq(0x0, mcu_base + OFFSET_MCU_DVC_INT_EN);
	}

	if (!mcu_irq_domain) {
		pr_err(PREFIX "failed to create irq domain\n");
		ret = -ENOMEM;
		goto out_free_mem;
	}

	pr_info(PREFIX "version [%u] on node [%u] initialized\n",
			version, node);

	irq_set_default_host(mcu_irq_domain);

	return 0;

out_free_mem:
	kfree(chip_data);
out_unmap1:
	iounmap(mcu_base);
out_unmap0:
	iounmap(pintc_base);
	return ret;
}

static int __init
pintc_of_init(struct device_node *pintc, struct device_node *parent)
{
	return pintc_of_init_common(pintc, parent, false);
}

IRQCHIP_DECLARE(sw64_pintc, "sw64,pintc", pintc_of_init);

static int __init
pintc_vt_of_init(struct device_node *pintc, struct device_node *parent)
{
	return pintc_of_init_common(pintc, parent, true);
}

IRQCHIP_DECLARE(sw64_pintc_vt, "sw64,pintc_vt", pintc_vt_of_init);
#endif
