/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_KVM_EMULATE_H
#define _ASM_SW64_KVM_EMULATE_H

#include <linux/kvm_host.h>
#include <asm/kvm_asm.h>

#define R(x)	((size_t) &((struct kvm_regs *)0)->x)

#ifdef CONFIG_SUBARCH_C3B
static int reg_offsets[32] = {
	R(r0), R(r1), R(r2), R(r3), R(r4), R(r5), R(r6), R(r7), R(r8),
	R(r9), R(r10), R(r11), R(r12), R(r13), R(r14), R(r15),
	R(r16), R(r17), R(r18),
	R(r19), R(r20), R(r21), R(r22), R(r23), R(r24), R(r25), R(r26),
	R(r27), R(r28), R(gp),
	0, 0,
};


static inline void vcpu_set_reg(struct kvm_vcpu *vcpu, u8 reg_num,
				unsigned long val)
{
	void *regs_ptr = (void *)&vcpu->arch.regs;

	regs_ptr += reg_offsets[reg_num];
	*(unsigned long *)regs_ptr = val;
}

static inline unsigned long vcpu_get_reg(struct kvm_vcpu *vcpu, u8 reg_num)
{
	void *regs_ptr = (void *)&vcpu->arch.regs;

	if (reg_num == 31)
		return 0;
	regs_ptr += reg_offsets[reg_num];
	return *(unsigned long *)regs_ptr;
}

#elif defined(CONFIG_SUBARCH_C4)
static inline void vcpu_set_reg(struct kvm_vcpu *vcpu, u8 reg_num,
				unsigned long val)
{
	vcpu->arch.regs.r[reg_num] = val;
}

static inline unsigned long vcpu_get_reg(struct kvm_vcpu *vcpu, u8 reg_num)
{
	if (reg_num == 31)
		return 0;
	return vcpu->arch.regs.r[reg_num];
}
#endif

void sw64_decode(struct kvm_vcpu *vcpu, unsigned int insn,
		 struct kvm_run *run);

unsigned int interrupt_pending(struct kvm_vcpu *vcpu, bool *more);
void clear_vcpu_irq(struct kvm_vcpu *vcpu);
void inject_vcpu_irq(struct kvm_vcpu *vcpu, unsigned int irq);
void try_deliver_interrupt(struct kvm_vcpu *vcpu, unsigned int irq, bool more);
#endif /* _ASM_SW64_KVM_EMULATE_H */
