// SPDX-License-Identifier: GPL-2.0
#include <linux/pci.h>
#include <linux/msi.h>
#include <linux/irqdomain.h>
#include <linux/smp.h>

#include <asm/irq_impl.h>
#include <asm/kvm_emulate.h>

static struct irq_domain *msi_default_domain;
static DEFINE_RAW_SPINLOCK(vector_lock);
DEFINE_PER_CPU(vector_irq_t, vector_irq) = {
	[0 ... PERCPU_MSI_IRQS - 1] = 0,
};

static struct sw64_msi_chip_data *alloc_sw_msi_chip_data(struct irq_data *irq_data)
{
	struct sw64_msi_chip_data *data;
	int node;

	node = irq_data_get_node(irq_data);
	data = kzalloc_node(sizeof(*data), GFP_KERNEL, node);
	if (!data)
		return NULL;
	spin_lock_init(&data->cdata_lock);
	return data;
}

static void irq_msi_compose_msg(struct irq_data *data, struct msi_msg *msg)
{
	struct sw64_msi_chip_data *chip_data;
	int rcid;

	chip_data = irq_data_get_irq_chip_data(data->parent_data);
	rcid = cpu_to_rcid(chip_data->dst_cpu);
	msg->address_hi = MSI_ADDR_BASE_HI;
	msg->address_lo =
		(unsigned int)chip_data->msiaddr |
		(rcid_to_msicid(rcid) << MSI_ADDR_DEST_ID_SHIFT);
	msg->data = chip_data->vector;
}

bool find_free_cpu_vector(const struct cpumask *search_mask,
			  int *found_cpu, int *found_vector)
{
	int vector, max_vector, cpu;
	bool find_once_global = false;

	cpu = cpumask_first(search_mask);
try_again:
	if (is_guest_or_emul()) {
		vector = IRQ_PENDING_MSI_VECTORS_SHIFT;
		max_vector = SWVM_IRQS;
	} else {
		vector = 0;
		max_vector = 256;
	}
	for (; vector < max_vector; vector++) {
		while (per_cpu(vector_irq, cpu)[vector]) {
			cpu = cpumask_next(cpu, search_mask);
			if (cpu >= nr_cpu_ids) {
				if (vector == 255) {
					if (find_once_global) {
						pr_warn("No global free vector\n");
						return false;
					}
					pr_warn("No local free vector\n");
					search_mask = cpu_online_mask;
					cpu = cpumask_first(search_mask);
					find_once_global = true;
					goto try_again;
				}
				cpu = cpumask_first(search_mask);
				break;
			}
		}
		if (!per_cpu(vector_irq, cpu)[vector])
			break;
	}

	*found_cpu = cpu;
	*found_vector = vector;
	return true;
}

static bool find_free_cpu_vectors(const struct cpumask *search_mask, int *found_cpu, int *found_vector, unsigned int nr_irqs)
{
	int i, vector, cpu;
	bool found = false, find_once_global = false;

	cpu = cpumask_first(search_mask);
try_again:
	for (vector = 0; vector < 256; vector++) {
		for (i = 0; i < nr_irqs; i++)
			if (per_cpu(vector_irq, cpu)[vector + i])
				break;

		if (i == nr_irqs) {
			found = true;
			*found_cpu = cpu;
			*found_vector = vector;
			return found;
		}

		vector += i;
	}

	cpu = cpumask_next(cpu, search_mask);
	if (cpu < nr_cpu_ids)
		goto try_again;
	else {
		if (find_once_global) {
			pr_warn("No global free vectors\n");
			return found;
		}
		pr_warn("No local free vectors\n");
		search_mask = cpu_online_mask;
		cpu = cpumask_first(search_mask);
		find_once_global = true;
		goto try_again;
	}
}

static int sw64_set_affinity(struct irq_data *d, const struct cpumask *cpumask, bool force)
{
	struct sw64_msi_chip_data *cdata;
	struct irq_data *irqd;
	struct msi_desc *entry;
	struct cpumask searchmask;
	unsigned long flags;
	int vector, cpu;
	int i;
	struct msi_msg msg;

	/* Is this valid ? */
	if (cpumask_any_and(cpumask, cpu_online_mask) >= nr_cpu_ids)
		return -EINVAL;

	irqd = irq_domain_get_irq_data(msi_default_domain->parent, d->irq);
	/* Don't do anything if the interrupt isn't started */
	if (!irqd_is_started(irqd))
		return IRQ_SET_MASK_OK;

	cdata = irqd->chip_data;
	if (!cdata)
		return -ENOMEM;

	/*
	 * If existing target cpu is already in the new mask and is online
	 * then do nothing.
	 */
	if (cpu_online(cdata->dst_cpu) && cpumask_test_cpu(cdata->dst_cpu, cpumask))
		return IRQ_SET_MASK_OK;

	raw_spin_lock_irqsave(&vector_lock, flags);

	cpumask_and(&searchmask, cpumask, cpu_online_mask);
	if (cdata->multi_msi > 1) {
		if (!find_free_cpu_vectors(&searchmask, &cpu,
					&vector, cdata->multi_msi)) {
			raw_spin_unlock_irqrestore(&vector_lock, flags);
			return -ENOSPC;
		}
	} else {
		if (!find_free_cpu_vector(&searchmask, &cpu, &vector)) {
			raw_spin_unlock_irqrestore(&vector_lock, flags);
			return -ENOSPC;
		}
	}

	/* update new setting */
	entry = irq_get_msi_desc(irqd->irq);
	spin_lock(&cdata->cdata_lock);
	for (i = 0; i < cdata->multi_msi; i++)
		per_cpu(vector_irq, cpu)[vector + i] = entry->irq + i;
	BUG_ON(irq_chip_compose_msi_msg(irqd, &msg));
	__pci_write_msi_msg(entry, &msg);
	cdata->prev_vector = cdata->vector;
	cdata->prev_cpu = cdata->dst_cpu;
	cdata->dst_cpu = cpu;
	cdata->vector = vector;
	cdata->move_in_progress = true;
	spin_unlock(&cdata->cdata_lock);
	cpumask_copy(irq_data_get_affinity_mask(irqd), &searchmask);

	raw_spin_unlock_irqrestore(&vector_lock, flags);

	return 0;
}

static void chip_irq_ack(struct irq_data *data)
{
}

static struct irq_chip pci_msi_controller = {
	.name                   = "PCI-MSI",
	.irq_unmask             = pci_msi_unmask_irq,
	.irq_mask               = pci_msi_mask_irq,
	.irq_ack                = chip_irq_ack,
	.irq_compose_msi_msg    = irq_msi_compose_msg,
	.flags                  = IRQCHIP_SKIP_SET_WAKE,
	.irq_set_affinity	= sw64_set_affinity,
};

static int __assign_irq_vector(int virq, unsigned int nr_irqs,
		struct irq_domain *domain, enum irq_alloc_type type)
{
	struct irq_data *irq_data;
	const struct cpumask *mask;
	struct cpumask searchmask;
	struct sw64_msi_chip_data *cdata;
	int node;
	int i, vector, cpu;
	unsigned long msiaddr;

	if (unlikely((nr_irqs > 1) && (!is_power_of_2(nr_irqs))))
		nr_irqs = __roundup_pow_of_two(nr_irqs);

	irq_data = irq_domain_get_irq_data(domain, virq);
	BUG_ON(!irq_data);
	irq_data->chip = &pci_msi_controller;

	if (irqd_affinity_is_managed(irq_data)) {
		mask = irq_data_get_affinity_mask(irq_data);
		cpumask_and(&searchmask, mask, cpu_online_mask);
	} else {
		node = irq_data_get_node(irq_data);
		cpumask_copy(&searchmask, cpumask_of_node(node));
	}

	if (cpumask_first(&searchmask) >= nr_cpu_ids)
		cpumask_copy(&searchmask, cpu_online_mask);

	if (type == IRQ_ALLOC_TYPE_MSI && nr_irqs > 1) {
		if (!find_free_cpu_vectors(&searchmask, &cpu,
					&vector, nr_irqs))
			return -ENOSPC;

		cdata = alloc_sw_msi_chip_data(irq_data);
		if (!cdata) {
			pr_warn("error alloc irq chip data\n");
			return -ENOMEM;
		}

		for (i = 0; i < nr_irqs; i++) {
			per_cpu(vector_irq, cpu)[vector + i] = virq + i;

			if (i) {
				irq_data = irq_domain_get_irq_data(domain, virq + i);
				irq_data->chip = &pci_msi_controller;
			}

			irq_data->chip_data = cdata;
		}

		cdata->dst_cpu = cpu;
		cdata->vector = vector;
		cdata->msiaddr = MSIX_MSG_ADDR;
		cdata->prev_cpu = cpu;
		cdata->prev_vector = vector;
		cdata->multi_msi = nr_irqs;
		cdata->move_in_progress = false;
	} else {
		for (i = 0; i < nr_irqs; i++) {
			if (!find_free_cpu_vector(&searchmask, &cpu, &vector))
				return -ENOSPC;

			per_cpu(vector_irq, cpu)[vector] = virq + i;

			if (i) {
				irq_data = irq_domain_get_irq_data(domain, virq + i);
				irq_data->chip = &pci_msi_controller;
			}

			cdata = alloc_sw_msi_chip_data(irq_data);
			if (!cdata) {
				pr_warn("error alloc irq chip data\n");
				return -ENOMEM;
			}

			irq_data->chip_data = cdata;

			cdata->dst_cpu = cpu;
			cdata->vector = vector;
			cdata->msiaddr = MSIX_MSG_ADDR;
			cdata->prev_cpu = cpu;
			cdata->prev_vector = vector;
			cdata->multi_msi = 1;
			cdata->move_in_progress = false;
		}
	}
	return 0;
}

static int assign_irq_vector(int irq, unsigned int nr_irqs,
		struct irq_domain *domain, enum irq_alloc_type type)
{
	int err;
	unsigned long flags;

	raw_spin_lock_irqsave(&vector_lock, flags);
	err = __assign_irq_vector(irq, nr_irqs, domain, type);
	raw_spin_unlock_irqrestore(&vector_lock, flags);
	return err;
}

static void sw64_vector_free_irqs(struct irq_domain *domain,
		unsigned int virq, unsigned int nr_irqs)
{
	int i, j;
	struct irq_data *irq_data;
	unsigned long flags;
	unsigned int multi_msi;

	for (i = 0; i < nr_irqs; i++) {
		irq_data = irq_domain_get_irq_data(domain, virq + i);
		if (irq_data && irq_data->chip_data) {
			struct sw64_msi_chip_data *cdata;

			raw_spin_lock_irqsave(&vector_lock, flags);
			cdata = irq_data->chip_data;
			irq_domain_reset_irq_data(irq_data);
			multi_msi = cdata->multi_msi;
			for (j = 0; j < multi_msi; j++)
				per_cpu(vector_irq, cdata->dst_cpu)[cdata->vector + j] = 0;
			kfree(cdata);
			raw_spin_unlock_irqrestore(&vector_lock, flags);
			if (multi_msi > 1)
				break;
		}
	}
}

static void sw64_irq_free_descs(unsigned int virq, unsigned int nr_irqs)
{
	if (is_guest_or_emul()) {
		vt_sw64_vector_free_irqs(virq, nr_irqs);
		return irq_free_descs(virq, nr_irqs);
	}

	return irq_domain_free_irqs(virq, nr_irqs);
}

void arch_teardown_msi_irqs(struct pci_dev *dev)
{
	struct msi_desc *desc;
	int i;

	for_each_pci_msi_entry(desc, dev) {
		if (desc->irq) {
			for (i = 0; i < desc->nvec_used; i++)
				sw64_irq_free_descs(desc->irq + i, 1);
			desc->irq = 0;
		}
	}
}

static int sw64_vector_alloc_irqs(struct irq_domain *domain, unsigned int virq,
		unsigned int nr_irqs, void *arg)
{
	int err;
	struct irq_alloc_info *info = arg;
	enum irq_alloc_type msi_type;

	if (arg == NULL)
		return -ENODEV;
	msi_type = info->type;
	err = assign_irq_vector(virq, nr_irqs, domain, msi_type);
	if (err)
		goto error;
	return 0;
error:
	sw64_vector_free_irqs(domain, virq, nr_irqs);
	return err;
}

static int pci_msi_prepare(struct irq_domain *domain, struct device *dev,
		int nvec, msi_alloc_info_t *arg)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct msi_desc *desc = first_pci_msi_entry(pdev);

	memset(arg, 0, sizeof(*arg));
	arg->msi_dev = pdev;
	if (desc->msi_attrib.is_msix)
		arg->type = IRQ_ALLOC_TYPE_MSIX;
	else
		arg->type = IRQ_ALLOC_TYPE_MSI;
	return 0;
}

static struct msi_domain_ops pci_msi_domain_ops = {
	.msi_prepare    = pci_msi_prepare,
};

static struct msi_domain_info pci_msi_domain_info = {
	.flags          = MSI_FLAG_USE_DEF_DOM_OPS | MSI_FLAG_USE_DEF_CHIP_OPS |
			  MSI_FLAG_MULTI_PCI_MSI | MSI_FLAG_PCI_MSIX,
	.ops            = &pci_msi_domain_ops,
	.chip           = &pci_msi_controller,
	.handler        = handle_edge_irq,
	.handler_name   = "edge",
};

static int sw64_irq_map(struct irq_domain *d, unsigned int virq, irq_hw_number_t hw)
{
	irq_set_chip_and_handler(virq, &sw64_irq_chip, handle_level_irq);
	irq_set_status_flags(virq, IRQ_LEVEL);
	return 0;
}

const struct irq_domain_ops sw64_msi_domain_ops = {
	.map    = sw64_irq_map,
	.alloc  = sw64_vector_alloc_irqs,
	.free   = sw64_vector_free_irqs,
};

int arch_setup_msi_irqs(struct pci_dev *pdev, int nvec, int type)
{
	int err;

	if (is_guest_or_emul())
		return sw64_setup_vt_msi_irqs(pdev, nvec, type);

	if (!msi_default_domain)
		return -EIO;
	err = msi_domain_alloc_irqs(msi_default_domain, &pdev->dev, nvec);
	return err;
}

void arch_init_msi_domain(struct irq_domain *parent)
{
	struct irq_domain *sw64_irq_domain;

	if (is_guest_or_emul())
		return;

	sw64_irq_domain = irq_domain_add_tree(NULL, &sw64_msi_domain_ops, NULL);
	BUG_ON(sw64_irq_domain == NULL);
	irq_set_default_host(sw64_irq_domain);
	msi_default_domain = pci_msi_create_irq_domain(NULL,
			&pci_msi_domain_info, sw64_irq_domain);
	if (!msi_default_domain)
		pr_warn("failed to initialize irqdomain for MSI/MSI-x.\n");
}

static void irq_move_complete(struct sw64_msi_chip_data *cdata, int cpu, int vector)
{
	if (likely(!cdata->move_in_progress))
		return;
	if (cdata->dst_cpu == cpu) {
		if (vector >= cdata->vector &&
				vector < cdata->vector + cdata->multi_msi) {
			int i;

			raw_spin_lock(&vector_lock);
			cdata->move_in_progress = false;
			for (i = 0; i < cdata->multi_msi; i++)
				per_cpu(vector_irq, cdata->prev_cpu)[cdata->prev_vector + i] = 0;
			raw_spin_unlock(&vector_lock);
		}
	}
}

void handle_pci_msi_interrupt(unsigned long type, unsigned long vector, unsigned long pci_msi1_addr)
{
	int i, irq, msi_index = 0;
	int cpu, vector_index = 0;
	unsigned long int_pci_msi[3];
	unsigned long *ptr;
	struct irq_data *irq_data;
	struct sw64_msi_chip_data *cdata;

	if (is_guest_or_emul()) {
		cpu = smp_processor_id();
		irq = per_cpu(vector_irq, cpu)[vector];
		handle_irq(irq);
		return;
	}

	ptr = (unsigned long *)pci_msi1_addr;
	int_pci_msi[0] = *ptr;
	int_pci_msi[1] = *(ptr + 1);
	int_pci_msi[2] = *(ptr + 2);

	cpu = smp_processor_id();

	for (i = 0; i < 4; i++) {
		vector_index = i * 64;
		while (vector != 0) {
			int irq = 0;

			msi_index = find_next_bit(&vector, 64, msi_index);
			if (msi_index == 64) {
				msi_index = 0;
				continue;
			}

			irq = per_cpu(vector_irq, cpu)[vector_index + msi_index];
			irq_data = irq_domain_get_irq_data(msi_default_domain->parent, irq);
			cdata = irq_data_get_irq_chip_data(irq_data);
			spin_lock(&cdata->cdata_lock);
			irq_move_complete(cdata, cpu, vector_index + msi_index);
			spin_unlock(&cdata->cdata_lock);
			handle_irq(irq);

			vector = vector & (~(1UL << msi_index));
		}

		vector = int_pci_msi[i % 3];
	}
}
