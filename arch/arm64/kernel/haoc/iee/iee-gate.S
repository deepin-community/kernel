/* SPDX-License-Identifier: GPL-2.0 */
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Lyu Jinglin <lvjl2022@zgclab.edu.cn>
 *          Zhang Shiyang <zhangsy2023@zgclab.edu.cn>
 */

#include <linux/linkage.h>
#include <asm/bug.h>
#include <asm/assembler.h>
#include <asm/haoc/iee-asm.h>
SYM_FUNC_START(iee_rw_gate)
    /* save daif, close irq  */
    mrs x13, daif
	msr daifset, #0x2
    isb
    /* save lr */
	stp	x29, x30, [sp, #-16]!
	bl iee_protected_rw_gate
    /* restore lr */
    ldp	x29, x30, [sp], #16
    /* restore daif */
	msr daif, x13
    ret
SYM_FUNC_END(iee_rw_gate)

	.pushsection ".iee.text", "ax"

.align 4

.global iee_tcr
iee_tcr:
	.quad 0

.global kernel_tcr
kernel_tcr:
	.quad 0

.global iee_tcr_mask
iee_tcr_mask:
	.quad IEE_TCR_MASK

.global __iee_code_start
__iee_code_start:

SYM_FUNC_START(iee_protected_rw_gate)
	/* entry gate */
	mrs x12, tcr_el1
	orr x12, x12, #TCR_HPD1
	orr x12, x12, #TCR_A1
	msr tcr_el1, x12
	isb
	/* Check TCR */
	ldr x9, iee_tcr
	ldr x10, iee_tcr_mask
	and x12, x12, x10
	cmp x9, x12
	b.ne 1f

	/* Switch to iee stack by per cpu ptr. */
	ldr_this_cpu x9, iee_cpu_stack_ptr, x10
	mov x14, sp
	mov sp, x9

	stp x13, x14, [sp, #-16]!
	/* call iee func */
	adrp x12, iee_funcs
    add x12, x12, x0, lsl #3
    ldr x12, [x12, #:lo12:iee_funcs]
	stp x29, x30, [sp, #-16]!
    blr x12
	ldp x29, x30, [sp], #16
	/* Switch to kernel stack */
	ldp x13, x14, [sp], #16
	mov sp, x14
	/* exit gate */
	mrs x12, tcr_el1
	bic x12, x12, #TCR_HPD1
	bic x12, x12, #TCR_A1
	msr tcr_el1, x12
	isb
	/* Check TCR */
	ldr x9, kernel_tcr
	ldr x10, iee_tcr_mask
	and x12, x12, x10
	cmp x9, x12
	b.ne 1f
	ret
1:
	mov	x0, sp
	mov	x1, #BAD_TCR_EL1
	mrs	x2, esr_el1
	mrs x3, tcr_el1
	bl	iee_bad_mode
	ASM_BUG()
SYM_FUNC_END(iee_protected_rw_gate)

	.popsection
