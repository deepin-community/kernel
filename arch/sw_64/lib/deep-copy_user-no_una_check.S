/* SPDX-License-Identifier: GPL-2.0 */

/* Allow an exception for an insn; exit if we get one.  */
#define FIXUP_LDST(x, y)			\
	99: x, y;			\
	.section __ex_table, "a";	\
	.long 99b - .;			\
	ldi $31, 255f-99b($31);		\
	.previous

/*
 * $7:	SIMD status for C3B
 *	0: not in simd loop
 *	1: in simd loop
 *	2: in simd_u loop
 * $7:	SIMD status for C4
 *	0: not in simd loop
 *	1: in simd and simd_u loop
 * $18:	bytes left to copy
 *
 */
	.globl ____copy_user_hw_una
	.ent ____copy_user_hw_una
____copy_user_hw_una:
	.prologue 0
	.set noreorder
	bis	$31, $31, $7
#include "deep-copy_template-no_una_check.S"
255:
	bis	$31, $18, $0
	beq	$7, $return
	subl	$7, 1, $7
	beq	$7, $restore_simd

$restore_simd:
	RESTORE_SIMD_REGS

$return:
	ret
	.end ____copy_user_hw_una
