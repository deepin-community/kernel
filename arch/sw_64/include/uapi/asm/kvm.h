/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_ASM_SW64_KVM_H
#define _UAPI_ASM_SW64_KVM_H

#include <asm/ptrace.h>

/*
 * KVM SW specific structures and definitions.
 */
#define SWVM_IRQS 256
#define IRQ_PENDING_INTX_SHIFT		16
#define IRQ_PENDING_MSI_VECTORS_SHIFT	18

#define KVM_DIRTY_LOG_PAGE_OFFSET	64

enum SW64_KVM_IRQ {
	SW64_KVM_IRQ_IPI = 27,
	SW64_KVM_IRQ_TIMER = 9,
	SW64_KVM_IRQ_KBD = 29,
	SW64_KVM_IRQ_MOUSE = 30,
};

#define __KVM_HAVE_IRQ_LINE
#define __KVM_HAVE_READONLY_MEM
#define __KVM_HAVE_GUEST_DEBUG

#define KVM_NR_IRQCHIPS		1

/*
 * for KVM_GET_REGS and KVM_SET_REGS
 */
#if defined(__sw_64_sw6b__)
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
#elif defined(__sw_64_sw8a__)
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
 * for KVM_GET_FPU and KVM_SET_FPU
 */
struct kvm_fpu {
};

struct kvm_debug_exit_arch {
	unsigned long epc;
	unsigned long reason;//indicate breakpoint or watchpoint
};

/* for KVM_SET_GUEST_DEBUG */
struct kvm_guest_debug_arch {
	uint64_t addr;
	uint64_t mask;
	uint64_t ctl;
};

/* definition of registers in kvm_run */
struct kvm_sync_regs {
};

/* dummy definition */
struct kvm_sregs {
};
#endif /* _UAPI_ASM_SW64_KVM_H */
