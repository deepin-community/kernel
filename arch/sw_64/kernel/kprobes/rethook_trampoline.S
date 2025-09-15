/* SPDX-License-Identifier: GPL-2.0+ */
#include <linux/linkage.h>
#include <asm/asm-offsets.h>
#include <asm/csr.h>

	.text
	.set noat
	.align 4

	.macro save_all_base_regs
	/* save $26 & fp of the function before caller */
	subl	$sp, 0x10, $sp
	stl	$26, 0($sp)
	stl	$15, 0x8($sp)
	ldi	$15, 0($sp)

	/* save $28 & fp of caller */
	subl	$sp, 0x10, $sp
	stl	$28, 0($sp)
	stl	$15, 0x8($sp)
	ldi	$15, 0($sp)

	/* save pt_regs */
	ldi	$sp, -PT_REGS_SIZE($sp)
	stl	$0, PT_REGS_R0($sp)
	stl	$1, PT_REGS_R1($sp)
	stl	$2, PT_REGS_R2($sp)
	stl	$3, PT_REGS_R3($sp)
	stl	$4, PT_REGS_R4($sp)
	stl	$5, PT_REGS_R5($sp)
	stl	$6, PT_REGS_R6($sp)
	stl	$7, PT_REGS_R7($sp)
	stl	$8, PT_REGS_R8($sp)
	stl	$9, PT_REGS_R9($sp)
	stl	$10, PT_REGS_R10($sp)
	stl	$11, PT_REGS_R11($sp)
	stl	$12, PT_REGS_R12($sp)
	stl	$13, PT_REGS_R13($sp)
	stl	$14, PT_REGS_R14($sp)
	stl	$15, PT_REGS_R15($sp)
	stl	$16, PT_REGS_R16($sp)
	stl	$17, PT_REGS_R17($sp)
	stl	$18, PT_REGS_R18($sp)
	stl	$19, PT_REGS_R19($sp)
	stl	$20, PT_REGS_R20($sp)
	stl	$21, PT_REGS_R21($sp)
	stl	$22, PT_REGS_R22($sp)
	stl	$23, PT_REGS_R23($sp)
	stl	$24, PT_REGS_R24($sp)
	stl	$25, PT_REGS_R25($sp)
	stl	$26, PT_REGS_R26($sp)
	stl	$27, PT_REGS_R27($sp)
	stl	$28, PT_REGS_R28($sp)
	stl	$29, PT_REGS_GP($sp)
	ldi	$0, PT_REGS_SIZE($sp)
	stl	$0, PT_REGS_R30($sp)
	.endm

	.macro restore_all_base_regs
	/* restore pt_regs */
	ldl	$0, PT_REGS_R0($sp)
	ldl	$1, PT_REGS_R1($sp)
	ldl	$2, PT_REGS_R2($sp)
	ldl	$3, PT_REGS_R3($sp)
	ldl	$4, PT_REGS_R4($sp)
	ldl	$5, PT_REGS_R5($sp)
	ldl	$6, PT_REGS_R6($sp)
	ldl	$7, PT_REGS_R7($sp)
	ldl	$8, PT_REGS_R8($sp)
	ldl	$9, PT_REGS_R9($sp)
	ldl	$10, PT_REGS_R10($sp)
	ldl	$11, PT_REGS_R11($sp)
	ldl	$12, PT_REGS_R12($sp)
	ldl	$13, PT_REGS_R13($sp)
	ldl	$14, PT_REGS_R14($sp)
	ldl	$15, PT_REGS_R15($sp)
	ldl	$16, PT_REGS_R16($sp)
	ldl	$17, PT_REGS_R17($sp)
	ldl	$18, PT_REGS_R18($sp)
	ldl	$19, PT_REGS_R19($sp)
	ldl	$20, PT_REGS_R20($sp)
	ldl	$21, PT_REGS_R21($sp)
	ldl	$22, PT_REGS_R22($sp)
	ldl	$23, PT_REGS_R23($sp)
	ldl	$24, PT_REGS_R24($sp)
	ldl	$25, PT_REGS_R25($sp)
	ldl	$27, PT_REGS_R27($sp)
	ldl	$28, PT_REGS_R28($sp)
	ldl	$29, PT_REGS_GP($sp)
	ldi	$sp, PT_REGS_SIZE($sp)

	/* only restore $fp */
	ldl	$15, 0x18($sp)
	addl	$sp, 0x20, $sp
	.endm

ENTRY(arch_rethook_trampoline)
	save_all_base_regs

	mov	$sp, $16
	br	$27, 1f
1:	ldgp	$29, 0($27)
	call	arch_rethook_trampoline_callback
	mov	$0, $26

	restore_all_base_regs

	ret	$31, ($26), 1
ENDPROC(arch_rethook_trampoline)
