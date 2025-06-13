/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/linkage.h>
#include <asm/bug.h>
#include <asm/assembler.h>
#include <asm/haoc/iee-asm.h>

SYM_FUNC_START(iee_rwx_gate)
	/* save daif, close irq  */
	mrs x13, daif
	msr daifset, #0xf
	isb

	/* Call sip handler functions directly before iee init done. */
	adrp x9, iee_init_done
	ldr w9, [x9, #:lo12:iee_init_done]
	cbnz w9, 1f
	str x13, [sp, #-32]!
	stp x29, x30, [sp, #16]
	/* call iee func */
	bl iee_si_handler
	ldp x29, x30, [sp, #16]
	ldr x13, [sp], #32
	isb
	b 2f
1:
	stp x29, x30, [sp, #-16]!
	bl iee_rwx_gate_tramp
	ldp x29, x30, [sp], #16
2:
	/* restore irq */
	msr daif, x13
	ret
SYM_FUNC_END(iee_rwx_gate)

/* We can confim the following code are uninterruptible by ELR_EL1 checking
 * on kernel entry.
 */
	.pushsection ".iee.text", "ax"

SYM_FUNC_START(iee_rwx_gate_tramp)
	/* entry gate */
	mrs x12, tcr_el1
	orr x12, x12, #TCR_HPD1
	bic x12, x12, #TCR_A1
	msr tcr_el1, x12
	isb
	/* Check TCR */
	ldr x9, iee_tcr
	cbz x9, 1f			/* Skip tcr check and stack switch on early init */
	ldr x10, iee_tcr_mask
	and x12, x12, x10
	cmp x9, x12
	b.ne 3f

	/* Switch to iee stack by per cpu ptr. */
	ldr_this_cpu x9, iee_cpu_stack_ptr, x10
	mov x14, sp
	mov sp, x9
1:
	stp x13, x14, [sp, #-32]!
	stp x29, x30, [sp, #16]
	/* call iee func */
	bl iee_si_handler
	ldp x29, x30, [sp, #16]
	ldp x13, x14, [sp], #32

	/* exit gate */
	mrs x12, tcr_el1
	bic x12, x12, #TCR_HPD1
	orr x12, x12, #TCR_A1
	msr tcr_el1, x12
	isb
	/* Check TCR */
	ldr x9, kernel_tcr
	cbz x9, 2f
	ldr x10, iee_tcr_mask
	and x12, x12, x10
	cmp x9, x12
	b.ne 3f

	/* Switch to kernel stack */
	mov sp, x14
2:
	ret
3:
	mov	x0, sp
	mov	x1, #BAD_TCR_EL1
	mrs	x2, esr_el1
	mrs x3, tcr_el1
	bl	iee_bad_mode
	ASM_BUG()
SYM_FUNC_END(iee_rwx_gate_tramp)

	.popsection
