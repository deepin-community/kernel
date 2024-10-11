#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/pci.h>

#include <asm/sw64io.h>
#include <asm/msi.h>
#include <asm/smp.h>
#include <asm/pci.h>
#include <asm/hw_irq.h>
#include <asm/irq_impl.h>

#define PCI_INTXCONFIG_OFFSET	7
#define PCI_INTTYPE_OFFSET	10

#if defined(CONFIG_SUBARCH_C3B)
	#define PCI_INTDST_DOMAIN_ID_SHIFT 6
#elif defined(CONFIG_SUBARCH_C4)
	#define PCI_INTDST_DOMAIN_ID_SHIFT 7
#endif

#define PCI_INTDST_THREAD_ID_SHIFT 6

static DEFINE_RAW_SPINLOCK(legacy_lock);

struct intx_chip_data {
	struct pci_controller *hose;
	unsigned long intxconfig[PCI_NUM_INTX];
	unsigned int offset;
};

static void lock_legacy_lock(void)
{
	raw_spin_lock(&legacy_lock);
}

static void unlock_legacy_lock(void)
{
	raw_spin_unlock(&legacy_lock);
}

struct intx_chip_data *alloc_intx_chip_data(u32 node)
{
	struct intx_chip_data *chip_data;

	if (WARN_ON(node >= MAX_NUMNODES))
		return NULL;

	chip_data = kzalloc_node(sizeof(struct intx_chip_data),
			GFP_KERNEL, node);

	return chip_data;
}

static int __assign_piu_intx_config(struct intx_chip_data *chip_data,
				    cpumask_t *targets)
{
	struct pci_controller *hose;
	unsigned long intxconfig;
	void __iomem *piu_ior0_base;
	unsigned int cpu;
	int thread, node, core, rcid;
	unsigned int i;

	if (is_guest_or_emul())
		return 0;

	/* Use the last cpu in valid cpus to avoid core 0. */
	cpu = cpumask_last(targets);
	rcid = cpu_to_rcid(cpu);

	thread = rcid_to_thread_id(rcid);
	node = rcid_to_domain_id(rcid);
	core = rcid_to_core_id(rcid);

	hose = chip_data->hose;
	piu_ior0_base = hose->piu_ior0_base;

	for (i = 0; i < PCI_NUM_INTX; i++) {
		intxconfig = chip_data->intxconfig[i];
		intxconfig &= ~PCI_INTX_INTDST_MASK;

		if (IS_ENABLED(CONFIG_SUBARCH_C3B))
			intxconfig |= core | (node << PCI_INTDST_DOMAIN_ID_SHIFT);
		else
			intxconfig |= core | (thread << PCI_INTDST_THREAD_ID_SHIFT)
						| (node << PCI_INTDST_DOMAIN_ID_SHIFT);

		writeq(intxconfig, piu_ior0_base + INTACONFIG +
					(i << PCI_INTXCONFIG_OFFSET));
		chip_data->intxconfig[i] = intxconfig;
	}
	return 0;
}

static int assign_piu_intx_config(struct intx_chip_data *chip_data,
				  cpumask_t *targets)
{
	int ret;

	lock_legacy_lock();
	ret = __assign_piu_intx_config(chip_data, targets);
	unlock_legacy_lock();

	return ret;
}

static void set_intx_enable(struct irq_data *irq_data, u32 flag)
{
	struct intx_chip_data *chip_data = irq_data->chip_data;
	struct pci_controller *hose;
	void __iomem *piu_ior0_base;
	unsigned long intxconfig;
	unsigned int i;

	if (!chip_data)
		return;

	hose = chip_data->hose;
	piu_ior0_base = hose->piu_ior0_base;

	for (i = 0; i < PCI_NUM_INTX; i++) {
		intxconfig = chip_data->intxconfig[i];
		if (flag)
			intxconfig |= PCI_INTX_ENABLE;
		else
			intxconfig &= PCI_INTX_DISABLE;
		writeq(intxconfig, piu_ior0_base + INTACONFIG +
					(i << PCI_INTXCONFIG_OFFSET));
	}
}

static void intx_irq_enable(struct irq_data *irq_data)
{
	if (is_guest_or_emul())
		return;

	set_intx_enable(irq_data, 1);
}

static void intx_irq_disable(struct irq_data *irq_data)
{
	if (is_guest_or_emul())
		return;

	set_intx_enable(irq_data, 0);
}

static int intx_set_affinity(struct irq_data *irq_data,
			     const struct cpumask *dest, bool force)
{
	struct intx_chip_data *chip_data = irq_data->chip_data;
	cpumask_t targets;
	int ret = 0;

	if (cpumask_any_and(dest, cpu_online_mask) >= nr_cpu_ids)
		return -EINVAL;

	cpumask_copy(&targets, dest);

	intx_irq_disable(irq_data);
	ret = assign_piu_intx_config(chip_data, &targets);
	intx_irq_enable(irq_data);

	return ret;
}

static void intx_mask_irq(struct irq_data *irq_data, u32 flag)
{
	struct intx_chip_data *chip_data = irq_data->chip_data;
	struct pci_controller *hose;
	void __iomem *piu_ior0_base;
	unsigned long intxconfig;
	unsigned int offset;

	if (!chip_data)
		return;

	hose = chip_data->hose;
	piu_ior0_base = hose->piu_ior0_base;
	offset = chip_data->offset;
	intxconfig = chip_data->intxconfig[offset];

	if (flag)
		intxconfig &= PCI_INTX_DISABLE;
	else
		intxconfig |= PCI_INTX_ENABLE;

	writeq(intxconfig, piu_ior0_base + INTACONFIG +
				(offset << PCI_INTXCONFIG_OFFSET));
}

static void intx_irq_mask(struct irq_data *irq_data)
{
	if (is_guest_or_emul())
		return;

	intx_mask_irq(irq_data, 1);
}

static void intx_irq_unmask(struct irq_data *irq_data)
{
	if (is_guest_or_emul())
		return;

	intx_mask_irq(irq_data, 0);
}

static void noop(struct irq_data *d) {}

static struct irq_chip sw64_intx_chip = {
	.name			= "PCI-INTX",
	.irq_enable		= intx_irq_enable,
	.irq_disable		= intx_irq_disable,
	.irq_mask		= intx_irq_mask,
	.irq_unmask		= intx_irq_unmask,
	.irq_set_affinity	= intx_set_affinity,
	.irq_ack		= noop,
	.flags			= IRQCHIP_SKIP_SET_WAKE,
};

void __weak set_pcieport_service_irq(struct pci_controller *hose) {}

void setup_intx_irqs(struct pci_controller *hose)
{
	unsigned long irq, node, val_node;
	struct intx_chip_data *chip_data;
	void __iomem *piu_ior0_base;
	int i = 0;

	node = hose->node;
	piu_ior0_base = hose->piu_ior0_base;

	if (!node_online(node))
		val_node = next_node_in(node, node_online_map);
	else
		val_node = node;

	irq = irq_alloc_descs_from(NR_IRQS_LEGACY, 2, val_node);
	WARN_ON(irq < 0);
	irq_set_chip_and_handler(irq, &sw64_intx_chip, handle_level_irq);
	irq_set_status_flags(irq, IRQ_LEVEL);

	chip_data = alloc_intx_chip_data(val_node);
	if (!chip_data)
		return;

	chip_data->hose = hose;

	for (i = 0; i < PCI_NUM_INTX; i++) {
		if (IS_ENABLED(CONFIG_SUBARCH_C3B))
			chip_data->intxconfig[i] = (0x1UL << (3 - i)) << PCI_INTTYPE_OFFSET;
		else
			chip_data->intxconfig[i] = (0x1UL << i) << PCI_INTTYPE_OFFSET;
	}

	irq_set_chip_data(irq, chip_data);
	hose->int_irq = irq;

	irq_set_chip_and_handler(irq + 1, &dummy_irq_chip, handle_level_irq);
	hose->service_irq = irq + 1;

	set_pcieport_service_irq(hose);
}

void __init sunway_init_pci_intx(void)
{
	struct pci_controller *hose = hose_head;

	for (hose = hose_head; hose; hose = hose->next)
		setup_intx_irqs(hose);
}

void handle_intx(unsigned int offset)
{
	struct irq_data *irq_data;
	struct intx_chip_data *chip_data;
	struct pci_controller *hose;
	unsigned long value;
	void __iomem *piu_ior0_base;

	hose = hose_head;
	for (hose = hose_head; hose; hose = hose->next) {
		piu_ior0_base = hose->piu_ior0_base;

		value = readq(piu_ior0_base + INTACONFIG +
				(offset << PCI_INTXCONFIG_OFFSET));

		if ((value & (PCI_INTX_VALID)) && (value & PCI_INTX_ENABLE)) {
			irq_data = irq_get_irq_data(hose->int_irq);
			if (irq_data) {
				chip_data = irq_data->chip_data;
				if (chip_data)
					chip_data->offset = offset;
			}
			handle_irq(hose->int_irq);
		}

		if (hose->iommu_enable) {
			value = readq(piu_ior0_base + IOMMUEXCPT_STATUS);
			if (value & PCI_INTX_VALID)
				handle_irq(hose->int_irq);
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
	}
}
