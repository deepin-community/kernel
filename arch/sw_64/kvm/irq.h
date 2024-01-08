/* SPDX-License-Identifier: GPL-2.0 */
/*
 * irq.h: in kernel interrupt controller related definitions
 */

#ifndef _SW64_KVM_IRQ_H
#define _SW64_KVM_IRQ_H
static inline int irqchip_in_kernel(struct kvm *kvm)
{
	return 1;
}
#endif /* _SW64_KVM_IRQ_H */
