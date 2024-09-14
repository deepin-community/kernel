#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/pci.h>

#include <asm/sw64io.h>
#include <asm/msi.h>
#include <asm/smp.h>
#include <asm/pci.h>
#include <asm/hw_irq.h>
#include <asm/irq_impl.h>

static DEFINE_RAW_SPINLOCK(legacy_lock);
static void lock_legacy_lock(void)
{
	raw_spin_lock(&legacy_lock);
}

static void unlock_legacy_lock(void)
{
	raw_spin_unlock(&legacy_lock);
}

static void set_intx(struct pci_controller *hose, unsigned long intx_conf)
{
	void __iomem *piu_ior0_base;

	if (is_guest_or_emul())
		return;

	piu_ior0_base = hose->piu_ior0_base;

	if (IS_ENABLED(CONFIG_SUBARCH_C3B)) {
		writeq(intx_conf | (0x8UL << 10), (piu_ior0_base + INTACONFIG));
		writeq(intx_conf | (0x4UL << 10), (piu_ior0_base + INTBCONFIG));
		writeq(intx_conf | (0x2UL << 10), (piu_ior0_base + INTCCONFIG));
		writeq(intx_conf | (0x1UL << 10), (piu_ior0_base + INTDCONFIG));
	} else {
		writeq(intx_conf | (0x8UL << 10), (piu_ior0_base + INTDCONFIG));
		writeq(intx_conf | (0x4UL << 10), (piu_ior0_base + INTCCONFIG));
		writeq(intx_conf | (0x2UL << 10), (piu_ior0_base + INTBCONFIG));
		writeq(intx_conf | (0x1UL << 10), (piu_ior0_base + INTACONFIG));
	}
}

static int __assign_piu_intx_config(struct pci_controller *hose, cpumask_t *targets)
{
	unsigned long intx_conf;
	unsigned int cpu;
	int thread, node, core, rcid;

	/* Use the last cpu in valid cpus to avoid core 0. */
	cpu = cpumask_last(targets);
	rcid = cpu_to_rcid(cpu);

	thread = rcid_to_thread_id(rcid);
	node = rcid_to_domain_id(rcid);
	core = rcid_to_core_id(rcid);

	if (IS_ENABLED(CONFIG_SUBARCH_C3B))
		intx_conf = core | (node << 6);
	else
		intx_conf = core | (thread << 6) | (node << 7);

	set_intx(hose, intx_conf);

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
	unsigned long intx_conf;
	void __iomem *piu_ior0_base;

	if (is_guest_or_emul())
		return;
	BUG_ON(!hose);

	piu_ior0_base = hose->piu_ior0_base;

	intx_conf = readq(piu_ior0_base + INTACONFIG);
	intx_conf |= PCI_INTX_ENABLE;
	writeq(intx_conf, (piu_ior0_base + INTACONFIG));

	intx_conf = readq(piu_ior0_base + INTBCONFIG);
	intx_conf |= PCI_INTX_ENABLE;
	writeq(intx_conf, (piu_ior0_base + INTBCONFIG));

	intx_conf = readq(piu_ior0_base + INTCCONFIG);
	intx_conf |= PCI_INTX_ENABLE;
	writeq(intx_conf, (piu_ior0_base + INTCCONFIG));

	intx_conf = readq(piu_ior0_base + INTDCONFIG);
	intx_conf |= PCI_INTX_ENABLE;
	writeq(intx_conf, (piu_ior0_base + INTDCONFIG));
}

static void intx_irq_disable(struct irq_data *irq_data)
{
	struct pci_controller *hose = irq_data->chip_data;
	unsigned long intx_conf;
	void __iomem *piu_ior0_base;

	if (is_guest_or_emul())
		return;

	BUG_ON(!hose);
	piu_ior0_base = hose->piu_ior0_base;

	intx_conf = readq(piu_ior0_base + INTACONFIG);
	intx_conf &= PCI_INTX_DISABLE;
	writeq(intx_conf, (piu_ior0_base + INTACONFIG));

	intx_conf = readq(piu_ior0_base + INTBCONFIG);
	intx_conf &= PCI_INTX_DISABLE;
	writeq(intx_conf, (piu_ior0_base + INTBCONFIG));

	intx_conf = readq(piu_ior0_base + INTCCONFIG);
	intx_conf &= PCI_INTX_DISABLE;
	writeq(intx_conf, (piu_ior0_base + INTCCONFIG));

	intx_conf = readq(piu_ior0_base + INTDCONFIG);
	intx_conf &= PCI_INTX_DISABLE;
	writeq(intx_conf, (piu_ior0_base + INTDCONFIG));
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

void __weak set_pcieport_service_irq(struct pci_controller *hose) {}

void setup_intx_irqs(struct pci_controller *hose)
{
	unsigned long irq, node, val_node;

	node = hose->node;

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

	set_pcieport_service_irq(hose);
}

void __init sw64_init_irq(void)
{
	struct pci_controller *hose = hose_head;

	for (hose = hose_head; hose; hose = hose->next)
		setup_intx_irqs(hose);
}

void handle_intx(unsigned int offset)
{
	struct pci_controller *hose;
	unsigned long value;
	void __iomem *piu_ior0_base;

	hose = hose_head;
	offset <<= 7;
	for (hose = hose_head; hose; hose = hose->next) {
		piu_ior0_base = hose->piu_ior0_base;

		value = readq(piu_ior0_base + INTACONFIG + offset);
		if (value & PCI_INTX_VALID) {
			value &= PCI_INTX_DISABLE;
			writeq(value, (piu_ior0_base + INTACONFIG + offset));
			handle_irq(hose->int_irq);
			value |= PCI_INTX_ENABLE;
			writeq(value, (piu_ior0_base + INTACONFIG + offset));
		}

		if (IS_ENABLED(CONFIG_PCIE_PME)) {
			value = readq(piu_ior0_base + PMEINTCONFIG);
			if (value & PCI_INTX_VALID) {
				handle_irq(hose->service_irq);
				writeq(value, (piu_ior0_base + PMEINTCONFIG));
			}
		}

		if (IS_ENABLED(CONFIG_PCIEAER)) {
			value = readq(piu_ior0_base + AERERRINTCONFIG);
			if (value & PCI_INTX_VALID) {
				handle_irq(hose->service_irq);
				writeq(value, (piu_ior0_base + AERERRINTCONFIG));
			}
		}

		if (IS_ENABLED(CONFIG_HOTPLUG_PCI_PCIE_SUNWAY)) {
			value = readq(piu_ior0_base + HPINTCONFIG);
			if (value & PCI_INTX_VALID) {
				handle_irq(hose->service_irq);
				writeq(value, (piu_ior0_base + HPINTCONFIG));
			}
		}

		if (hose->iommu_enable) {
			value = readq(piu_ior0_base + IOMMUEXCPT_STATUS);
			if (value & PCI_INTX_VALID)
				handle_irq(hose->int_irq);
		}
	}
}
