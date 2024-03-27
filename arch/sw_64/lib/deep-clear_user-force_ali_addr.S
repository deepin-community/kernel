/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Contributed by Mao Minkai <maominkai@wxiat.com>
 *
 * Zero user space, handling exceptions as we go.
 *
 * We have to make sure that $0 is always up-to-date and contains the
 * right "bytes left to zero" value (and that it is updated only _after_
 * a successful copy).  There is also some rather minor exception setup
 * stuff.
 *
 */
#include <asm/export.h>
/* Allow an exception for an insn; exit if we get one.  */
#define FIXUP_LDST(x,y...)			\
	99: x,##y;			\
	.section __ex_table,"a";	\
	.long 99b - .;			\
	ldi $31, 255f-99b($31);	\
	.previous

/*
 * $7:	SIMD status
 *	0: not in simd loop
 *	1: in simd loop
 *	2: in simd_u loop
 * $18:	bytes left to copy
 *
 */
	.globl ____clear_user_sw_una
	.ent ____clear_user_sw_una
____clear_user_sw_una:
	.prologue 0
	bis	$31, $31, $7
	mov	$17, $18
	bis	$31, $31, $17
#include "deep-set_template-force_ali_addr.S"
255:
	bis	$31, $18, $0
	beq	$7, $return

$restore_simd:
	RESTORE_SIMD_REGS

$return:
	ret
	.end ____clear_user_sw_una
