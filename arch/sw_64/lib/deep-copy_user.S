/* SPDX-License-Identifier: GPL-2.0 */

#include <asm/export.h>

/* Allow an exception for an insn; exit if we get one.  */
#define FIXUP_LDST(x, y)			\
	99: x, y;			\
	.section __ex_table, "a";	\
	.long 99b - .;			\
	ldi $31, $out-99b($31);		\
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
	.globl __copy_user
	.ent __copy_user
__copy_user:
	.prologue 0
	.set noreorder
	bis	$31, $31, $7
#if defined(CONFIG_SUBARCH_C3B)
#include "deep-copy_template.S"
#elif defined(CONFIG_SUBARCH_C4)
#include "deep-copy_template_c4.S"
#endif
$out:
	bis	$31, $18, $0
	beq	$7, $return
	subl	$7, 1, $7
	beq	$7, $restore_simd

#if defined(CONFIG_SUBARCH_C3B)
$restore_simd_u:
	RESTORE_SIMD_U_REGS
	br	$31, $return
#endif

$restore_simd:
	RESTORE_SIMD_REGS

$return:
	ret
	.end __copy_user
	EXPORT_SYMBOL(__copy_user)
