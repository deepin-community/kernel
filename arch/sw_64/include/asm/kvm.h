/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_KVM_H
#define _ASM_SW64_KVM_H
#include <uapi/asm/kvm.h>

/*
 * KVM SW specific structures and definitions.
 */
#define SWVM_IRQS 256
#define IRQ_PENDING_INTX_SHIFT		16
#define IRQ_PENDING_MSI_VECTORS_SHIFT	18

#define SWVM_NUM_NUMA_MEMBANKS	1

/*
 * return stack for __sw64_vcpu_run
 */
struct vcpu_run_ret_stack {
	unsigned long ra;
	unsigned long r0;
};

struct host_int_args {
	unsigned long r18;
	unsigned long r17;
	unsigned long r16;
};

struct hcall_args {
	unsigned long arg0, arg1, arg2;
};

struct swvm_mem_bank {
	unsigned long guest_phys_addr;
	unsigned long host_phys_addr;
	unsigned long host_addr;
	unsigned long size;
};

struct swvm_mem {
	struct swvm_mem_bank membank[SWVM_NUM_NUMA_MEMBANKS];
};

#endif /* _ASM_SW64_KVM_H */
