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
#include <asm/csr.h>
#include <asm/export.h>
/* Allow an exception for an insn; exit if we get one.  */
#define FIXUP_LDST(x,y...)			\
	99: x,##y;			\
	.section __ex_table,"a";	\
	.long 99b - .;			\
	ldi $31, $return-99b($31);	\
	.previous

#define NBYTES	$0
#define DST	$16
#define T0	$1
#define L_OUT	$return

	.globl ____clear_user_simd
	.ent ____clear_user_simd
____clear_user_simd:
	.prologue 0
	mov	$17, NBYTES

#include "clear_simd_template.S"

	.align 4
$return:
	ret
	.end ____clear_user_simd
