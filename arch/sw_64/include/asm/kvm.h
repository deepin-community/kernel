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
 * for KVM_GET_REGS and KVM_SET_REGS
 */
#ifdef CONFIG_SUBARCH_C3B
struct kvm_regs {
	unsigned long r0;
	unsigned long r1;
	unsigned long r2;
	unsigned long r3;

	unsigned long r4;
	unsigned long r5;
	unsigned long r6;
	unsigned long r7;

	unsigned long r8;
	unsigned long r9;
	unsigned long r10;
	unsigned long r11;

	unsigned long r12;
	unsigned long r13;
	unsigned long r14;
	unsigned long r15;

	unsigned long r19;
	unsigned long r20;
	unsigned long r21;
	unsigned long r22;

	unsigned long r23;
	unsigned long r24;
	unsigned long r25;
	unsigned long r26;

	unsigned long r27;
	unsigned long r28;
	unsigned long reserved;
	unsigned long fpcr;

	unsigned long fp[124];
	/* These are saved by HMcode: */
	unsigned long ps;
	unsigned long pc;
	unsigned long gp;
	unsigned long r16;
	unsigned long r17;
	unsigned long r18;
	unsigned long __padding[6];
};

#elif CONFIG_SUBARCH_C4
struct kvm_regs {
	union {
		struct user_pt_regs regs;
		struct {
			unsigned long r[31];
			unsigned long pc;
			unsigned long ps;
		};
	};
	struct user_fpsimd_state fpstate;
};
#endif

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
