// SPDX-License-Identifier: GPL-2.0
#include <linux/pci.h>
#include <linux/irq.h>
#include <linux/msi.h>

static DEFINE_RAW_SPINLOCK(vector_lock);

static void __vt_irq_msi_compose_msg(struct sw64_msi_chip_data *cdata,
				     struct msi_msg *msg)
{
	msg->address_hi = (u32)(VT_MSIX_MSG_ADDR >> 32);
	msg->address_lo = (u32)(VT_MSIX_MSG_ADDR & 0xffffffff)
				| VT_MSIX_ADDR_DEST_ID(cdata->dst_cpu);
	msg->data = cdata->vector;
}

static void vt_irq_msi_compose_msg(struct irq_data *irqd, struct msi_msg *msg)
{
	struct sw64_msi_chip_data *cdata;

	cdata = irqd->chip_data;
	__vt_irq_msi_compose_msg(cdata, msg);
}

static void vt_irq_msi_update_msg(struct irq_data *irqd,
				  struct sw64_msi_chip_data *cdata)
{
	struct msi_msg msg[2] = { [1] = { }, };

	__vt_irq_msi_compose_msg(cdata, msg);
	pci_write_msi_msg(irqd->irq, msg);
}

static int
vt_set_affinity(struct irq_data *irqd, const struct cpumask *cpumask,
		bool force)
{
	struct sw64_msi_chip_data *cdata;
	struct cpumask searchmask;
	int cpu, vector;

	/* Is this valid ? */
	if (cpumask_any_and(cpumask, cpu_online_mask) >= nr_cpu_ids)
		return -EINVAL;

	if (!irqd_is_started(irqd))
		return IRQ_SET_MASK_OK;

	cdata = irqd->chip_data;
	if (!cdata)
		return -ENOMEM;

	/*
	 * If existing target coreid is already in the new mask,
	 * and is online then do nothing.
	 */
	if (cpu_online(cdata->dst_cpu) && cpumask_test_cpu(cdata->dst_cpu, cpumask))
		return IRQ_SET_MASK_OK;

	cpumask_and(&searchmask, cpumask, cpu_online_mask);
	if (!find_free_cpu_vector(&searchmask, &cpu, &vector))
		return -ENOSPC;

	per_cpu(vector_irq, cpu)[vector] = irqd->irq;
	spin_lock(&cdata->cdata_lock);
	cdata->dst_cpu = cpu;
	cdata->vector = vector;
	cdata->prev_cpu = cdata->dst_cpu;
	cdata->prev_vector = cdata->vector;
	cdata->move_in_progress = true;
	spin_unlock(&cdata->cdata_lock);
	cpumask_copy((struct cpumask *)irq_data_get_affinity_mask(irqd), &searchmask);
	vt_irq_msi_update_msg(irqd, irqd->chip_data);

	return 0;
}

static struct irq_chip vt_pci_msi_controller = {
	.name = "PCI-MSI",
	.irq_unmask = pci_msi_unmask_irq,
	.irq_mask = pci_msi_mask_irq,
	.irq_ack = sw64_irq_noop,
	.irq_compose_msi_msg = vt_irq_msi_compose_msg,
	.irq_set_affinity = vt_set_affinity,
};

int chip_setup_vt_msix_irq(struct pci_dev *dev, struct msi_desc *desc)
{
	int virq, val_node = 0;
	struct irq_data *irq_data;
	struct sw64_msi_chip_data *cdata;
	struct pci_controller *hose = pci_bus_to_pci_controller(dev->bus);
	unsigned long flags, node, rc_index;
	const struct cpumask *mask;

	struct cpumask searchmask;
	int cpu, vector;

	node = hose->node;
	rc_index = hose->index;
	mask = cpumask_of_node(node);

	raw_spin_lock_irqsave(&vector_lock, flags);
	/* Find unused msi config reg in PIU-IOR0 */
	if (!node_online(node))
		val_node = next_node_in(node, node_online_map);
	else
		val_node = node;

	virq = irq_alloc_descs_from(NR_IRQS_LEGACY, desc->nvec_used, val_node);
	if (virq < 0) {
		pr_err("Failed to allocate IRQ(base 16, count %d)\n", desc->nvec_used);
		raw_spin_unlock_irqrestore(&vector_lock, flags);
		return virq;
	}

	irq_data = irq_get_irq_data(virq);

	if (irqd_affinity_is_managed(irq_data)) {
		mask = irq_data_get_affinity_mask(irq_data);
		cpumask_and(&searchmask, mask, cpu_online_mask);
	} else {
		node = irq_data_get_node(irq_data);
		cpumask_copy(&searchmask, cpumask_of_node(node));
	}
	if (cpumask_first(&searchmask) >= nr_cpu_ids)
		cpumask_copy(&searchmask, cpu_online_mask);

	if (!find_free_cpu_vector(&searchmask, &cpu, &vector))
		return -ENOSPC;

	cdata = kzalloc(sizeof(*cdata), GFP_KERNEL);
	if (!cdata)
		return -ENOMEM;

	per_cpu(vector_irq, cpu)[vector] = virq;

	irq_set_msi_desc(virq, desc);
	irq_set_chip_and_handler_name(virq, &vt_pci_msi_controller,
				      handle_edge_irq, "edge");

	cdata->dst_cpu = cpu;
	cdata->vector = vector;
	cdata->rc_index = hose->index;
	cdata->rc_node = hose->node;
	cdata->prev_cpu = cpu;
	cdata->prev_vector = vector;

	irq_data->chip_data = cdata;

	vt_irq_msi_update_msg(irq_data, irq_data->chip_data);
	raw_spin_unlock_irqrestore(&vector_lock, flags);
	return 0;
}
EXPORT_SYMBOL(chip_setup_vt_msix_irq);

int chip_setup_vt_msi_irqs(struct pci_dev *dev, int nvec, int type)
{
	struct msi_desc *desc;
	struct pci_controller *hose = pci_bus_to_pci_controller(dev->bus);
	struct irq_data *irq_data;
	struct sw64_msi_chip_data *cdata;
	unsigned long node, rc_index;
	int virq = -1, val_node = 0;
	unsigned long flags;

	const struct cpumask *mask;
	struct cpumask searchmask;
	int i, vector, cpu;

	if (type == PCI_CAP_ID_MSI && nvec > 32)
		return 1;

	node = hose->node;
	rc_index = hose->index;
	raw_spin_lock_irqsave(&vector_lock, flags);
	msi_for_each_desc(desc, &(dev->dev), MSI_DESC_ALL) {
		/* Find unused msi config reg in PIU-IOR0 */
		if (!node_online(node))
			val_node = next_node_in(node, node_online_map);
		else
			val_node = node;
		virq = irq_alloc_descs_from(NR_IRQS_LEGACY, desc->nvec_used, val_node);
		if (virq < 0) {
			pr_err("Failed to allocate IRQ(base 16, count %d)\n", desc->nvec_used);
			raw_spin_unlock_irqrestore(&vector_lock, flags);
			return virq;
		}

		irq_data = irq_get_irq_data(virq);
		if (irqd_affinity_is_managed(irq_data)) {
			mask = irq_data_get_affinity_mask(irq_data);
			cpumask_and(&searchmask, mask, cpu_online_mask);
		} else {
			node = irq_data_get_node(irq_data);
			cpumask_copy(&searchmask, cpumask_of_node(node));
		}
		if (cpumask_first(&searchmask) >= nr_cpu_ids)
			cpumask_copy(&searchmask, cpu_online_mask);

		for (i = 0; i < desc->nvec_used; i++) {
			if (!find_free_cpu_vector(&searchmask, &cpu, &vector))
				return -ENOSPC;

			cdata = kzalloc(sizeof(*cdata), GFP_KERNEL);
			if (!cdata)
				return -ENOMEM;

			per_cpu(vector_irq, cpu)[vector] =  virq + i;
			irq_set_msi_desc_off(virq, i, desc);
			irq_set_chip_and_handler_name(virq + i, &vt_pci_msi_controller, handle_edge_irq, "edge");
			irq_data = irq_get_irq_data(virq + i);

			cdata->dst_cpu = cpu;
			cdata->vector = vector;
			cdata->rc_index = hose->index;
			cdata->rc_node = hose->node;
			cdata->prev_cpu = cpu;
			cdata->prev_vector = vector;

			irq_data->chip_data = cdata;

			vt_irq_msi_update_msg(irq_data, irq_data->chip_data);
		}
	}

	raw_spin_unlock_irqrestore(&vector_lock, flags);
	return 0;
}
EXPORT_SYMBOL(chip_setup_vt_msi_irqs);

void vt_sw64_vector_free_irqs(unsigned int virq, unsigned int nr_irqs)
{
	int i;
	unsigned long flags;
	struct irq_data *irq_data;
	struct sw64_msi_chip_data *cdata;

	for (i = 0; i < nr_irqs; i++) {
		irq_data = irq_get_irq_data(virq + i);
		if (irq_data && irq_data->chip_data) {
			raw_spin_lock_irqsave(&vector_lock, flags);
			cdata = irq_data->chip_data;
			irq_data->hwirq = 0;
			irq_data->chip = &no_irq_chip;
			irq_data->chip_data = NULL;
			per_cpu(vector_irq, cdata->dst_cpu)[cdata->vector] = 0;
			kfree(cdata);
			raw_spin_unlock_irqrestore(&vector_lock, flags);
		}
	}
}

int __arch_setup_vt_msix_irqs(struct pci_dev *dev, int nvec, int type)
{
	struct msi_desc *entry;
	int ret;

	msi_for_each_desc(entry, &dev->dev, MSI_DESC_ALL) {
		ret = chip_setup_vt_msix_irq(dev, entry);
		if (ret)
			return ret;
	}

	return 0;
}

int sw64_setup_vt_msi_irqs(struct pci_dev *dev, int nvec, int type)
{
	int ret = 0;

	if (type == PCI_CAP_ID_MSI)
		ret = chip_setup_vt_msi_irqs(dev, nvec, type);
	else if (type == PCI_CAP_ID_MSIX)
		ret = __arch_setup_vt_msix_irqs(dev, nvec, type);
	else
		pr_info("SW arch do not identify ID:%d\n", type);

	return ret;
}
