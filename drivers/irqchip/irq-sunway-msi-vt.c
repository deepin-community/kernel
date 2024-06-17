// SPDX-License-Identifier: GPL-2.0
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/msi.h>
#include <linux/pci.h>
#include <linux/smp.h>

#include <asm/irq_impl.h>
#include <asm/kvm_emulate.h>

static struct irq_domain *vt_msi_default_domain;
static DEFINE_RAW_SPINLOCK(vector_lock);

static void vt_irq_move_complete(struct sw64_msi_chip_data *cdata, int cpu)
{
	if (likely(!cdata->move_in_progress))
		return;
	if (cdata->dst_cpu == cpu) {
		raw_spin_lock(&vector_lock);
		cdata->move_in_progress = false;
		per_cpu(vector_irq, cdata->prev_cpu)[cdata->prev_vector] = 0;
		raw_spin_unlock(&vector_lock);
	}
}

void vt_handle_pci_msi_interrupt(unsigned long type, unsigned long vector,
				 unsigned long pci_msi1_addr)
{
	int irq, cpu;
	struct irq_data *irq_data;
	struct sw64_msi_chip_data *cdata;

	cpu = smp_processor_id();
	irq = per_cpu(vector_irq, cpu)[vector];
	if (unlikely(!irq))
		return;

	irq_data = irq_domain_get_irq_data(vt_msi_default_domain->parent, irq);
	cdata = irq_data_get_irq_chip_data(irq_data);

	spin_lock(&cdata->cdata_lock);
	vt_irq_move_complete(cdata, cpu);
	spin_unlock(&cdata->cdata_lock);

	handle_irq(irq);
}

static bool vt_find_free_cpu_vector(const struct cpumask *search_mask,
			  int *found_cpu, int *found_vector, struct irq_data *d)
{
	int vector, max_vector, cpu;
	bool find_once_global = false;

	cpu = cpumask_first(search_mask);
try_again:
	vector = IRQ_PENDING_MSI_VECTORS_SHIFT;
	max_vector = SWVM_IRQS;

	for (; vector < max_vector; vector++) {
		while (per_cpu(vector_irq, cpu)[vector]) {
			if (per_cpu(vector_irq, cpu)[vector] == d->irq)
				break;

			if (!irqd_affinity_is_managed(d))
				cpu = cpumask_next(cpu, search_mask);
			else
				vector++;

			if (vector >= max_vector) {
				cpu = cpumask_next(cpu, search_mask);
				vector = IRQ_PENDING_MSI_VECTORS_SHIFT;
			}

			if (cpu >= nr_cpu_ids) {
				if (vector == max_vector-1) {
					if (find_once_global) {
						pr_err("No global free vector\n");
						return false;
					}
					pr_err("No local free vector\n");
					search_mask = cpu_online_mask;
					cpu = cpumask_first(search_mask);
					find_once_global = true;
					goto try_again;
				}
				cpu = cpumask_first(search_mask);
				break;
			}
		}
		if (per_cpu(vector_irq, cpu)[vector] == d->irq)
			break;
		if (!per_cpu(vector_irq, cpu)[vector])
			break;
	}

	*found_cpu = cpu;
	*found_vector = vector;
	return true;
}

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
	struct irq_data *d;

	d = irq_domain_get_irq_data(vt_msi_default_domain->parent, irqd->irq);
	cdata = d->chip_data;

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
vt_set_affinity(struct irq_data *d, const struct cpumask *cpumask,
		bool force)
{
	struct sw64_msi_chip_data *cdata;
	struct irq_data *irqd;
	struct cpumask searchmask;
	int cpu, vector;

	/* Is this valid ? */
	if (cpumask_any_and(cpumask, cpu_online_mask) >= nr_cpu_ids)
		return -EINVAL;

	irqd = irq_domain_get_irq_data(vt_msi_default_domain->parent, d->irq);
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
	if (!vt_find_free_cpu_vector(&searchmask, &cpu, &vector, irqd))
		return -ENOSPC;

	per_cpu(vector_irq, cpu)[vector] = irqd->irq;
	spin_lock(&cdata->cdata_lock);
	cdata->prev_cpu = cdata->dst_cpu;
	cdata->prev_vector = cdata->vector;
	cdata->dst_cpu = cpu;
	cdata->vector = vector;
	cdata->move_in_progress = true;
	spin_unlock(&cdata->cdata_lock);
	irq_data_update_effective_affinity(irqd, &searchmask);
	vt_irq_msi_update_msg(irqd, irqd->chip_data);

	return 0;
}

static struct irq_chip vt_pci_msi_controller = {
	.name			= "PCI-MSI",
	.irq_unmask		= pci_msi_unmask_irq,
	.irq_mask		= pci_msi_mask_irq,
	.irq_ack		= sw64_irq_noop,
	.irq_compose_msi_msg    = vt_irq_msi_compose_msg,
	.flags                  = IRQCHIP_SKIP_SET_WAKE,
	.irq_set_affinity	= vt_set_affinity,
};

int chip_setup_vt_msi_irqs(int virq, unsigned int nr_irqs,
		struct irq_domain *domain, enum irq_alloc_type type)
{
	struct irq_data *irq_data;
	struct sw64_msi_chip_data *cdata;
	unsigned long node;

	const struct cpumask *mask;
	struct cpumask searchmask;
	int i, vector, cpu;

	if (type != IRQ_ALLOC_TYPE_MSI && type != IRQ_ALLOC_TYPE_MSIX) {
		pr_info("SW arch do not identify ID:%d\n", type);
		return -ENOMEM;
	}

	irq_data = irq_domain_get_irq_data(domain, virq);
	if (!irq_data)
		return -EINVAL;
	irq_data->chip = &vt_pci_msi_controller;

	if (irqd_affinity_is_managed(irq_data)) {
		mask = irq_data_get_affinity_mask(irq_data);
		cpumask_and(&searchmask, mask, cpu_online_mask);
	} else {
		node = irq_data_get_node(irq_data);
		cpumask_copy(&searchmask, cpumask_of_node(node));
	}
	if (cpumask_first(&searchmask) >= nr_cpu_ids)
		cpumask_copy(&searchmask, cpu_online_mask);

	for (i = 0; i < nr_irqs; i++) {
		if (i) {
			irq_data = irq_domain_get_irq_data(domain, virq + i);
			irq_data->chip = &vt_pci_msi_controller;
		}

		if (!vt_find_free_cpu_vector(&searchmask, &cpu, &vector, irq_data))
			return -ENOSPC;

		cdata = kzalloc(sizeof(*cdata), GFP_KERNEL);
		if (!cdata)
			return -ENOMEM;

		per_cpu(vector_irq, cpu)[vector] = virq + i;

		cdata->dst_cpu = cpu;
		cdata->vector = vector;
		cdata->prev_cpu = cpu;
		cdata->prev_vector = vector;
		cdata->move_in_progress = false;

		irq_data->chip_data = cdata;
	}

	return 0;
}
EXPORT_SYMBOL(chip_setup_vt_msi_irqs);

static void sw64_vt_vector_free_irqs(struct irq_domain *domain,
		unsigned int virq, unsigned int nr_irqs)
{
	int i;
	struct irq_data *irq_data;
	unsigned long flags;

	for (i = 0; i < nr_irqs; i++) {
		irq_data = irq_domain_get_irq_data(domain, virq + i);
		if (irq_data && irq_data->chip_data) {
			struct sw64_msi_chip_data *cdata;

			raw_spin_lock_irqsave(&vector_lock, flags);
			cdata = irq_data->chip_data;
			irq_domain_reset_irq_data(irq_data);
			per_cpu(vector_irq, cdata->dst_cpu)[cdata->vector] = 0;
			kfree(cdata);

			raw_spin_unlock_irqrestore(&vector_lock, flags);
		}
	}
}

static int assign_vt_irq_vector(int irq, unsigned int nr_irqs,
		struct irq_domain *domain, enum irq_alloc_type type)
{
	int err;
	unsigned long flags;

	raw_spin_lock_irqsave(&vector_lock, flags);
	err = chip_setup_vt_msi_irqs(irq, nr_irqs, domain, type);
	raw_spin_unlock_irqrestore(&vector_lock, flags);
	return err;
}

static int sw64_vt_vector_alloc_irqs(struct irq_domain *domain,
		unsigned int virq, unsigned int nr_irqs, void *arg)
{
	int err;
	struct irq_alloc_info *info = arg;
	enum irq_alloc_type msi_type;

	if (arg == NULL)
		return -ENODEV;
	msi_type = info->type;
	err = assign_vt_irq_vector(virq, nr_irqs, domain, msi_type);
	if (err)
		goto error;
	return 0;
error:
	sw64_vt_vector_free_irqs(domain, virq, nr_irqs);
	return err;
}


static int vt_pci_msi_prepare(struct irq_domain *domain, struct device *dev,
		int nvec, msi_alloc_info_t *arg)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct msi_desc *desc = msi_first_desc(dev, MSI_DESC_ALL);

	memset(arg, 0, sizeof(*arg));
	arg->msi_dev = pdev;
	if (desc->pci.msi_attrib.is_msix)
		arg->type = IRQ_ALLOC_TYPE_MSIX;
	else
		arg->type = IRQ_ALLOC_TYPE_MSI;
	return 0;
}

static struct msi_domain_ops vt_pci_msi_domain_ops = {
	.msi_prepare    = vt_pci_msi_prepare,
};

static struct msi_domain_info pci_vt_msi_domain_info = {
	.flags          = MSI_FLAG_USE_DEF_DOM_OPS | MSI_FLAG_USE_DEF_CHIP_OPS |
			  MSI_FLAG_MULTI_PCI_MSI | MSI_FLAG_PCI_MSIX,
	.ops            = &vt_pci_msi_domain_ops,
	.chip           = &vt_pci_msi_controller,
	.handler        = handle_edge_irq,
	.handler_name   = "edge",
};

static int sw64_vt_irq_map(struct irq_domain *d, unsigned int virq, irq_hw_number_t hw)
{
	irq_set_chip_and_handler(virq, &sw64_irq_chip, handle_level_irq);
	irq_set_status_flags(virq, IRQ_LEVEL);
	return 0;
}

const struct irq_domain_ops sw64_vt_msi_domain_ops = {
	.map    = sw64_vt_irq_map,
	.alloc  = sw64_vt_vector_alloc_irqs,
	.free   = sw64_vt_vector_free_irqs,
};

int sw64_setup_vt_msi_irqs(struct pci_dev *pdev, int nvec, int type)
{
	struct irq_domain *domain;
	int err;

	domain = vt_msi_default_domain;
	if (domain == NULL)
		return -EIO;
	err = msi_domain_alloc_irqs_all_locked(&pdev->dev, MSI_DEFAULT_DOMAIN, nvec);
	return err;
}

void sw64_init_vt_msi_domain(struct irq_domain *parent)
{
	struct irq_domain *sw64_irq_domain;

	sw64_irq_domain = irq_domain_add_tree(NULL, &sw64_vt_msi_domain_ops, NULL);
	BUG_ON(sw64_irq_domain == NULL);
	irq_set_default_host(sw64_irq_domain);
	vt_msi_default_domain = pci_msi_create_irq_domain(NULL,
			&pci_vt_msi_domain_info, sw64_irq_domain);
	if (!vt_msi_default_domain)
		pr_warn("failed to initialize irqdomain for MSI/MSI-x.\n");
}

int vt_pcibios_device_add(struct pci_dev *dev)
{
	if (vt_msi_default_domain)
		dev_set_msi_domain(&dev->dev, vt_msi_default_domain);
	return 0;
}
