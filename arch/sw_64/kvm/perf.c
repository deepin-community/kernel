// SPDX-License-Identifier: GPL-2.0
/*
 * Performance events support for KVM.
 */

#include <linux/perf_event.h>
#include <linux/kvm_host.h>

#include <asm/kvm_emulate.h>


bool kvm_arch_vcpu_in_kernel(struct kvm_vcpu *vcpu)
{
	if (IS_ENABLED(CONFIG_SUBARCH_C3B))
		return (vcpu->arch.regs.ps & 0x8) == 0;
	else
		return (vcpu->arch.regs.pc & 0x3) == 0x2;

}

unsigned long kvm_arch_vcpu_get_ip(struct kvm_vcpu *vcpu)
{
	return vcpu->arch.regs.pc;
}
