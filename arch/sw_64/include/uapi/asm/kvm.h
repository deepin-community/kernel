/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_ASM_SW64_KVM_H
#define _UAPI_ASM_SW64_KVM_H

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
 * for KVM_GET_FPU and KVM_SET_FPU
 */
struct kvm_fpu {
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
#endif /* _UAPI_ASM_SW64_KVM_H */
