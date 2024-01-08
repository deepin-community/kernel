/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_KVM_MMIO_H
#define _ASM_SW64_KVM_MMIO_H

#include <linux/kvm_host.h>
#include <asm/kvm_asm.h>

struct kvm_decode {
	unsigned long rt;
	bool sign_extend;
};

int kvm_handle_mmio_return(struct kvm_vcpu *vcpu, struct kvm_run *run);
int io_mem_abort(struct kvm_vcpu *vcpu, struct kvm_run *run,
		 struct hcall_args *hargs);

#endif /* _ASM_SW64_KVM_MMIO_H */
