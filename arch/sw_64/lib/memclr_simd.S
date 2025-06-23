/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Contributed by Mao Minkai <maominkai@wxiat.com>
 *
 * SIMD version of memset(dst, 0, size)
 */
#include <asm/csr.h>
#include <asm/export.h>

#define FIXUP_LDST(x, y)	\
	x, y

#define NBYTES	$18
#define DST	$16
#define T0	$1
#define L_OUT	$return

	.globl ____memclr_simd
	.ent ____memclr_simd
____memclr_simd:
	.prologue 0
	mov	$16, $0

#include "clear_simd_template.S"

	.align 4
$return:
	ret
	.end ____memclr_simd
