// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 - os kernal
 * Author: fire3 <fire3@example.com> yangzh <yangzh@gmail.com>
 * linhn <linhn@example.com>
 */
#include <asm/irq_impl.h>
#include <asm/kvm_emulate.h>
#include <asm/kvm_asm.h>
#include <linux/errno.h>
#include <linux/err.h>

void sw64_decode(struct kvm_vcpu *vcpu, unsigned int insn, struct kvm_run *run)
{
	int opc, ra;

	opc = (insn >> 26) & 0x3f;
	ra = (insn >> 21) & 0x1f;

	switch (opc) {
	case 0x20: /* LDBU */
		run->mmio.is_write = 0;
		run->mmio.len = 1;
		vcpu->arch.mmio_decode.rt = ra;
		break;
	case 0x21: /* LDHU */
		run->mmio.is_write = 0;
		run->mmio.len = 2;
		vcpu->arch.mmio_decode.rt = ra;
		break;
	case 0x22: /* LDW */
		run->mmio.is_write = 0;
		run->mmio.len = 4;
		vcpu->arch.mmio_decode.rt = ra;
		break;
	case 0x23: /* LDL */
	case 0x24: /* LDL_U */
		run->mmio.is_write = 0;
		run->mmio.len = 8;
		vcpu->arch.mmio_decode.rt = ra;
		break;
	case 0x28: /* STB */
		run->mmio.is_write = 1;
		*(unsigned long *)run->mmio.data = vcpu_get_reg(vcpu, ra) & 0xffUL;
		run->mmio.len = 1;
		break;
	case 0x29: /* STH */
		run->mmio.is_write = 1;
		*(unsigned long *)run->mmio.data = vcpu_get_reg(vcpu, ra) & 0xffffUL;
		run->mmio.len = 2;
		break;
	case 0x2a: /* STW */
		run->mmio.is_write = 1;
		*(unsigned long *)run->mmio.data = vcpu_get_reg(vcpu, ra) & 0xffffffffUL;
		run->mmio.len = 4;
		break;
	case 0x2b: /* STL */
	case 0x2c: /* STL_U */
		run->mmio.is_write = 1;
		*(unsigned long *)run->mmio.data = vcpu_get_reg(vcpu, ra);
		run->mmio.len = 8;
		break;
	default:
		pr_info("Miss done opc %d\n", opc);
		break;
	}
}

/*
 * Virtual Interrupts.
 */
unsigned int interrupt_pending(struct kvm_vcpu *vcpu, bool *more)
{
	unsigned int irq;
	DECLARE_BITMAP(blk, SWVM_IRQS);

	bitmap_copy(blk, vcpu->arch.irqs_pending, SWVM_IRQS);

	if (test_bit(INT_RTC, blk))
		return INT_RTC;

	if (test_bit(INT_IPI, blk))
		return INT_IPI;

	irq = find_last_bit(blk, SWVM_IRQS);

	return irq;
}

void clear_vcpu_irq(struct kvm_vcpu *vcpu)
{
	vcpu->arch.vcb.vcpu_irq = 0xffffffffffffffffUL;
}

void inject_vcpu_irq(struct kvm_vcpu *vcpu, unsigned int irq)
{
	vcpu->arch.vcb.vcpu_irq = irq;
}

/*
 * This actually diverts the Guest to running an interrupt handler, once an
 * interrupt has been identified by interrupt_pending().
 */
void try_deliver_interrupt(struct kvm_vcpu *vcpu, unsigned int irq, bool more)
{
	BUG_ON(irq >= SWVM_IRQS);

	/* Otherwise we check if they have interrupts disabled. */
	if (vcpu->arch.vcb.vcpu_irq_disabled) {
		clear_vcpu_irq(vcpu);
		return;
	}

	/* If they don't have a handler (yet?), we just ignore it */
	if (vcpu->arch.vcb.ent_int != 0) {
		/* OK, mark it no longer pending and deliver it. */
		clear_bit(irq, (vcpu->arch.irqs_pending));
		/*
		 * set_guest_interrupt() takes the interrupt descriptor and a
		 * flag to say whether this interrupt pushes an error code onto
		 * the stack as well: virtual interrupts never do.
		 */
		inject_vcpu_irq(vcpu, irq);
	}
}
