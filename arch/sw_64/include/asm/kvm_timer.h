/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_KVM_TIMER_H
#define _ASM_SW64_KVM_TIMER_H

void set_timer(struct kvm_vcpu *vcpu, unsigned long delta);
void set_interrupt(struct kvm_vcpu *vcpu, unsigned int irq);
enum hrtimer_restart clockdev_fn(struct hrtimer *timer);

#endif /* _ASM_SW64_KVM_TIMER_H */
