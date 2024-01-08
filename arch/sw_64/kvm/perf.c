// SPDX-License-Identifier: GPL-2.0
/*
 * Performance events support for KVM.
 */

#include <linux/perf_event.h>
#include <linux/kvm_host.h>

#include <asm/kvm_emulate.h>


bool kvm_arch_vcpu_in_kernel(struct kvm_vcpu *vcpu)
{
	return (vcpu->arch.regs.ps & 8) != 0;
}

unsigned long kvm_arch_vcpu_get_ip(struct kvm_vcpu *vcpu)
{
	return vcpu->arch.regs.pc;
}



static inline bool kvm_arch_pmi_in_guest(struct kvm_vcpu *vcpu)
{
	return IS_ENABLED(CONFIG_GUEST_PERF_EVENTS) && !!vcpu;
}
