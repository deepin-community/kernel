/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_ASM_SW64_KVM_H
#define _UAPI_ASM_SW64_KVM_H

/*
 * KVM SW specific structures and definitions.
 */
#define SWVM_IRQS 256
#define IRQ_PENDING_INTX_SHIFT		16
#define IRQ_PENDING_MSI_VECTORS_SHIFT	17

enum SW64_KVM_IRQ {
	SW64_KVM_IRQ_IPI = 27,
	SW64_KVM_IRQ_TIMER = 9,
	SW64_KVM_IRQ_KBD = 29,
	SW64_KVM_IRQ_MOUSE = 30,
};

#define SWVM_VM_TYPE_DEFAULT	0
#define SWVM_VM_TYPE_PHYVCPU	1
#define __KVM_HAVE_IRQ_LINE

#define SWVM_NUM_NUMA_MEMBANKS	1
#define KVM_NR_IRQCHIPS		1
/*
 * for KVM_GET_REGS and KVM_SET_REGS
 */
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
	unsigned long __padding0;
	unsigned long fpcr;

	unsigned long fp[124];
	/* These are saved by HMcode: */
	unsigned long ps;
	unsigned long pc;
	unsigned long gp;
	unsigned long r16;
	unsigned long r17;
	unsigned long r18;
};


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

/*
 * for KVM_GET_FPU and KVM_SET_FPU
 */
struct kvm_fpu {
};

struct hcall_args {
	unsigned long arg0, arg1, arg2;
};

struct phyvcpu_hcall_args {
	unsigned long call;
	struct hcall_args args;
};

struct kvm_debug_exit_arch {
	unsigned long epc;
};

/* for KVM_SET_GUEST_DEBUG */
struct kvm_guest_debug_arch {
};

/* definition of registers in kvm_run */
struct kvm_sync_regs {
};

/* dummy definition */
struct kvm_sregs {
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

#endif /* _UAPI_ASM_SW64_KVM_H */
