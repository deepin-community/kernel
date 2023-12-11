// SPDX-License-Identifier: GPL-2.0
#include <linux/irqdomain.h>
#include <linux/irqchip.h>
#include <linux/acpi.h>
#include <linux/acpi_iort.h>
#include <linux/of_irq.h>
#include <asm/sw64io.h>

/*
 * Multi-node platform device implementation hasn't been thought through yet,
 * which means how to obtain CPU node is ambiguous here. It is highly likely
 * that this will be passed through ACPI or DTS. Leave node with 0 as default
 * for now and wait for platform guys to check this later.
 */
#define DEFAULT_CPU_NODE 0
static int cpu_node = DEFAULT_CPU_NODE;

struct devint_chipdata {
	int node;
};

static DEFINE_RAW_SPINLOCK(devint_lock);
static void lock_dev_lock(void)
{
	raw_spin_lock(&devint_lock);
}

static void unlock_dev_lock(void)
{
	raw_spin_unlock(&devint_lock);
}

static void mcu_irq_mask(struct irq_data *data)
{
	struct devint_chipdata *chip_data = data->chip_data;
	unsigned int mask;
	int hwirq = data->hwirq;
	int node;

	node = chip_data->node;

	mask = sw64_io_read(node, MCU_DVC_INT_EN);
	mask &= ~(0x1UL << hwirq);
	sw64_io_write(node, MCU_DVC_INT_EN, mask);
}

static void mcu_irq_unmask(struct irq_data *data)
{
	struct devint_chipdata *chip_data = data->chip_data;
	unsigned int mask;
	int hwirq = data->hwirq;
	int node;

	node = chip_data->node;

	mask = sw64_io_read(node, MCU_DVC_INT_EN);
	mask |= (0x1UL << hwirq);
	sw64_io_write(node, MCU_DVC_INT_EN, mask);
}

static void mcu_irq_enable(struct irq_data *irq_data)
{
	struct devint_chipdata *data = irq_data->chip_data;
	unsigned long devint_conf;
	int node;

	node = data->node;

	devint_conf = sw64_io_read(node, DEV_INT_CONFIG);
	devint_conf |= (1UL << 8);
	sw64_io_write(node, DEV_INT_CONFIG, devint_conf);
	mcu_irq_unmask(irq_data);
}

static void mcu_irq_disable(struct irq_data *irq_data)
{
	struct devint_chipdata *data = irq_data->chip_data;
	unsigned long devint_conf;
	int node;

	node = data->node;

	devint_conf = sw64_io_read(node, DEV_INT_CONFIG);
	devint_conf &= ~(1UL << 8);
	sw64_io_write(node, DEV_INT_CONFIG, devint_conf);
	mcu_irq_mask(irq_data);
}

static int __assign_mcu_irq_config(int node, cpumask_t *targets)
{
	unsigned long dev_int_tar, val;
	unsigned int cpu;
	int phy_cpu;

	for_each_cpu(cpu, targets) {
		/*
		 * Hardware requires dev ints be redirected to on-node
		 * cores only. Thus, we remove all off-node cpu in the
		 * target mask.
		 */
		if (cpu_to_node(cpu) != node)
			cpumask_clear_cpu(cpu, targets);
	}

	/* Use the last one in valid cpus to avoid core 0. */
	cpu = cpumask_last(targets);
	if (cpu >= nr_cpu_ids)
		return -EPERM;

	phy_cpu = cpu_to_rcid(cpu);

	val = sw64_io_read(node, DEV_INT_CONFIG);
	dev_int_tar = DEV_INT_TARGET(phy_cpu);
	val &= 0xffff;
	val |= dev_int_tar << 16;
	sw64_io_write(node, DEV_INT_CONFIG, val);

	return 0;
}

static int assign_mcu_irq_config(int node, cpumask_t *targets)
{
	int ret;

	lock_dev_lock();
	ret = __assign_mcu_irq_config(node, targets);
	unlock_dev_lock();

	return ret;
}

static int mcu_irq_set_affinity(struct irq_data *irq_data,
				 const struct cpumask *dest, bool force)
{
	struct devint_chipdata *chip_data = irq_data->chip_data;
	cpumask_t targets;
	int node, ret = 0;

	if (cpumask_any_and(dest, cpu_online_mask) >= nr_cpu_ids)
		return -EINVAL;

	cpumask_and(&targets, dest, cpu_online_mask);

	node = chip_data->node;

	mcu_irq_disable(irq_data);
	ret = assign_mcu_irq_config(node, &targets);
	mcu_irq_enable(irq_data);

	return ret;
}

static struct irq_chip onchip_intc = {
	.name			= "MCU-INT",
	.irq_enable		= mcu_irq_enable,
	.irq_disable		= mcu_irq_disable,
	.irq_mask		= mcu_irq_mask,
	.irq_unmask		= mcu_irq_unmask,
	.irq_set_affinity	= mcu_irq_set_affinity,
};

static struct devint_chipdata *
alloc_sw64_devint_chip_data(struct irq_data *irq_data)
{
	struct devint_chipdata *chip_data;
	int node;

	node = irq_data_get_node(irq_data);
	chip_data = kzalloc_node(sizeof(*chip_data), GFP_KERNEL, node);
	if (!chip_data)
		return NULL;

	return chip_data;
}

static void sw64_intc_free_irqs(struct irq_domain *irq_domain,
				unsigned int virq, unsigned int nr_irqs)
{
	struct irq_data *irq_data;
	struct devint_chipdata *chip_data;
	int i = 0;

	for (i = 0; i < nr_irqs; i++) {
		irq_data = irq_domain_get_irq_data(irq_domain, virq + i);
		if (irq_data && irq_data->chip_data) {
			lock_dev_lock();
			chip_data = irq_data->chip_data;
			irq_domain_reset_irq_data(irq_data);
			kfree(chip_data);
			unlock_dev_lock();
		}
	}
}

static int sw64_intc_alloc_irqs(struct irq_domain *irq_domain,
				unsigned int virq,
				unsigned int nr_irqs,
				void *arg)
{
	struct irq_data *irq_data;
	struct devint_chipdata *chip_data;
	struct irq_fwspec *fwspec = arg;
	int default_node = cpu_node, i = 0, hwirq;

	for (i = 0; i < nr_irqs; i++) {
		irq_data = irq_domain_get_irq_data(irq_domain, virq + i);
		hwirq = fwspec->param[0];
		irq_data->hwirq = hwirq;

		chip_data = alloc_sw64_devint_chip_data(irq_data);
		if (!chip_data)
			goto out_free;

		chip_data->node = default_node;
		irq_data->chip_data = chip_data;
		irq_set_chip_and_handler(virq, &onchip_intc, handle_level_irq);
		irq_set_status_flags(virq, IRQ_LEVEL);
	}

	return 0;

out_free:
	sw64_intc_free_irqs(irq_domain, virq, nr_irqs);
	return -ENOMEM;
}

static const struct irq_domain_ops sw64_intc_domain_ops = {
	.xlate = irq_domain_xlate_onecell,
	.alloc = sw64_intc_alloc_irqs,
	.free = sw64_intc_free_irqs,
};

struct irq_domain *mcu_irq_domain;
EXPORT_SYMBOL(mcu_irq_domain);

#ifdef CONFIG_OF
static int __init
init_mcu_IRQ(struct device_node *intc, struct device_node *parent)
{
	if (parent) {
		pr_warn("DeviceTree incore intc not a root irq controller\n");
		return -ENODEV;
	}

	mcu_irq_domain = irq_domain_add_linear(intc, 8,
						&sw64_intc_domain_ops, NULL);

	if (!mcu_irq_domain) {
		pr_warn("root irq domain not avail\n");
		return -ENODEV;
	}

	/* with this we don't need to export root_domain */
	irq_set_default_host(mcu_irq_domain);

	/* mask all interrupts for now */
	sw64_io_write(cpu_node, MCU_DVC_INT_EN, 0x0);

	return 0;
}

IRQCHIP_DECLARE(sw64_intc, "sw64,sw6_irq_controller", init_mcu_IRQ);

static int __init
init_mcu_vt_IRQ(struct device_node *intc, struct device_node *parent)
{
	if (parent) {
		pr_warn("DeviceTree incore intc not a root irq controller\n");
		return -ENODEV;
	}

	mcu_irq_domain = irq_domain_add_legacy(intc, 16, 0, 0,
						&sw64_intc_domain_ops, NULL);

	if (!mcu_irq_domain) {
		pr_warn("root irq domain not avail\n");
		return -ENODEV;
	}

	/* with this we don't need to export root_domain */
	irq_set_default_host(mcu_irq_domain);

	return 0;
}

IRQCHIP_DECLARE(sw64_vt_intc, "sw64,sw6_irq_vt_controller", init_mcu_vt_IRQ);
#endif
