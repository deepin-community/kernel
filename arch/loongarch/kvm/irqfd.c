// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 Loongson Technology Corporation Limited
 */

#include <linux/kvm_host.h>
#include <trace/events/kvm.h>
#include <asm/kvm_pch_pic.h>

static int kvm_set_ioapic_irq(struct kvm_kernel_irq_routing_entry *e,
					struct kvm *kvm, int irq_source_id,
					int level, bool line_status)
{
	/* ioapic pin (0 ~ 64) <---> gsi(0 ~ 64) */
	pch_pic_set_irq(kvm->arch.pch_pic, e->irqchip.pin, level);

	return 0;
}

/*
 * kvm_set_routing_entry: populate a kvm routing entry
 * from a user routing entry
 *
 * @kvm: the VM this entry is applied to
 * @e: kvm kernel routing entry handle
 * @ue: user api routing entry handle
 * return 0 on success, -EINVAL on errors.
 */
int kvm_set_routing_entry(struct kvm *kvm,
			struct kvm_kernel_irq_routing_entry *e,
			const struct kvm_irq_routing_entry *ue)
{
	int r = -EINVAL;

	switch (ue->type) {
	case KVM_IRQ_ROUTING_IRQCHIP:
		e->set = kvm_set_ioapic_irq;

		e->irqchip.irqchip = ue->u.irqchip.irqchip;
		e->irqchip.pin = ue->u.irqchip.pin;

		if (e->irqchip.pin >= KVM_IRQCHIP_NUM_PINS)
			goto out;
		break;
	case KVM_IRQ_ROUTING_MSI:
		e->set = kvm_set_msi;
		e->msi.address_lo = ue->u.msi.address_lo;
		e->msi.address_hi = ue->u.msi.address_hi;
		e->msi.data = ue->u.msi.data;
		break;
	default:
		goto out;
	}
	r = 0;
out:
	return r;
}

int kvm_arch_set_irq_inatomic(struct kvm_kernel_irq_routing_entry *e,
		struct kvm *kvm, int irq_source_id,
		int level, bool line_status)
{
	if (e->type == KVM_IRQ_ROUTING_MSI) {
		pch_msi_set_irq(kvm, e->msi.data, 1);
		return 0;
	}

	return -EWOULDBLOCK;
}

/**
 * kvm_set_msi: inject the MSI corresponding to the
 * MSI routing entry
 *
 * This is the entry point for irqfd MSI injection
 * and userspace MSI injection.
 */
int kvm_set_msi(struct kvm_kernel_irq_routing_entry *e,
		struct kvm *kvm, int irq_source_id,
		int level, bool line_status)
{
	if (!level)
		return -1;

	pch_msi_set_irq(kvm, e->msi.data, level);
	return 0;
}
