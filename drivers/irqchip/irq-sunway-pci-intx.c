#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/pci.h>

#include <asm/sw64io.h>
#include <asm/msi.h>
#include <asm/smp.h>
#include <asm/pci.h>
#include <asm/hw_irq.h>

static DEFINE_RAW_SPINLOCK(legacy_lock);
static void lock_legacy_lock(void)
{
	raw_spin_lock(&legacy_lock);
}

static void unlock_legacy_lock(void)
{
	raw_spin_unlock(&legacy_lock);
}

static void set_intx(unsigned long node, unsigned long index, unsigned long intx_conf)
{
	if (is_guest_or_emul())
		return;

	write_piu_ior0(node, index, INTACONFIG, intx_conf | (0x8UL << 10));
	write_piu_ior0(node, index, INTBCONFIG, intx_conf | (0x4UL << 10));
	write_piu_ior0(node, index, INTCCONFIG, intx_conf | (0x2UL << 10));
	write_piu_ior0(node, index, INTDCONFIG, intx_conf | (0x1UL << 10));
}

static int __assign_piu_intx_config(struct pci_controller *hose, cpumask_t *targets)
{
	unsigned long intx_conf;
	unsigned int cpu;
	int node, index;
	int phy_cpu;

	node = hose->node;
	index = hose->index;

	/* Use the last cpu in valid cpus to avoid core 0. */
	cpu = cpumask_last(targets);
	phy_cpu = cpu_to_rcid(cpu);

	intx_conf = ((phy_cpu >> 5) << 6) | (phy_cpu & 0x1f);
	set_intx(node, index, intx_conf);

	return 0;
}

static int assign_piu_intx_config(struct pci_controller *hose, cpumask_t *targets)
{
	int ret;

	lock_legacy_lock();
	ret = __assign_piu_intx_config(hose, targets);
	unlock_legacy_lock();

	return ret;
}

static void intx_irq_enable(struct irq_data *irq_data)
{
	struct pci_controller *hose = irq_data->chip_data;
	unsigned long intx_conf, node, index;

	if (is_guest_or_emul())
		return;
	BUG_ON(!hose);

	node = hose->node;
	index = hose->index;

	intx_conf = read_piu_ior0(node, index, INTACONFIG);
	intx_conf |= PCI_INTX_ENABLE;
	write_piu_ior0(node, index, INTACONFIG, intx_conf);

	intx_conf = read_piu_ior0(node, index, INTBCONFIG);
	intx_conf |= PCI_INTX_ENABLE;
	write_piu_ior0(node, index, INTBCONFIG, intx_conf);

	intx_conf = read_piu_ior0(node, index, INTCCONFIG);
	intx_conf |= PCI_INTX_ENABLE;
	write_piu_ior0(node, index, INTCCONFIG, intx_conf);

	intx_conf = read_piu_ior0(node, index, INTDCONFIG);
	intx_conf |= PCI_INTX_ENABLE;
	write_piu_ior0(node, index, INTDCONFIG, intx_conf);
}

static void intx_irq_disable(struct irq_data *irq_data)
{
	struct pci_controller *hose = irq_data->chip_data;
	unsigned long intx_conf, node, index;

	if (is_guest_or_emul())
		return;

	BUG_ON(!hose);
	node = hose->node;
	index = hose->index;

	intx_conf = read_piu_ior0(node, index, INTACONFIG);
	intx_conf &= PCI_INTX_DISABLE;
	write_piu_ior0(node, index, INTACONFIG, intx_conf);

	intx_conf = read_piu_ior0(node, index, INTBCONFIG);
	intx_conf &= PCI_INTX_DISABLE;
	write_piu_ior0(node, index, INTBCONFIG, intx_conf);

	intx_conf = read_piu_ior0(node, index, INTCCONFIG);
	intx_conf &= PCI_INTX_DISABLE;
	write_piu_ior0(node, index, INTCCONFIG, intx_conf);

	intx_conf = read_piu_ior0(node, index, INTDCONFIG);
	intx_conf &= PCI_INTX_DISABLE;
	write_piu_ior0(node, index, INTDCONFIG, intx_conf);
}

static int intx_set_affinity(struct irq_data *irq_data,
			     const struct cpumask *dest, bool force)
{
	struct pci_controller *hose = irq_data->chip_data;
	cpumask_t targets;
	int ret = 0;

	if (cpumask_any_and(dest, cpu_online_mask) >= nr_cpu_ids)
		return -EINVAL;

	cpumask_copy(&targets, dest);

	intx_irq_disable(irq_data);
	ret = assign_piu_intx_config(hose, &targets);
	intx_irq_enable(irq_data);

	return ret;
}

static void noop(struct irq_data *d) {}

static struct irq_chip sw64_intx_chip = {
	.name			= "PCI_INTX",
	.irq_enable		= intx_irq_enable,
	.irq_disable		= intx_irq_disable,
	.irq_set_affinity       = intx_set_affinity,
	.irq_ack                = noop,
	.flags			= IRQCHIP_SKIP_SET_WAKE,
};

void __weak set_pcieport_service_irq(int node, int index) {}

void setup_intx_irqs(struct pci_controller *hose)
{
	unsigned long irq, node, index, val_node;

	node = hose->node;
	index = hose->index;

	if (!node_online(node))
		val_node = next_node_in(node, node_online_map);
	else
		val_node = node;

	irq = irq_alloc_descs_from(NR_IRQS_LEGACY, 2, val_node);
	WARN_ON(irq < 0);
	irq_set_chip_and_handler(irq, &sw64_intx_chip, handle_level_irq);
	irq_set_status_flags(irq, IRQ_LEVEL);
	irq_set_chip_data(irq, hose);
	hose->int_irq = irq;
	irq_set_chip_and_handler(irq + 1, &dummy_irq_chip, handle_level_irq);
	hose->service_irq = irq + 1;

	set_pcieport_service_irq(node, index);
}

void __init sw64_init_irq(void)
{
	struct pci_controller *hose = hose_head;

	for (hose = hose_head; hose; hose = hose->next)
		setup_intx_irqs(hose);
}

