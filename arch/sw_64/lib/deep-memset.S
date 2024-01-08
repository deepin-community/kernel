/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Optimized memset() for SW64 with SIMD instructions
 *
 * Copyright (C) Mao Minkai
 * Author: Mao Minkai
 *
 * Fill SIZE bytes pointed to by SRC with CHAR.
 *
 * Input:
 *	$16:	SRC, clobbered
 *	$17:	CHAR, clobbered
 *	$18:	SIZE, clobbered
 *
 * Output:
 *	$0:	SRC
 *
 * Temporaries:
 *	$1:	unaligned parts of addr (0 means aligned addr), tmp data
 *	$2:	tmp data
 *	$3:	tmp data
 *	$4:	tmp data
 *	$5:	compare result
 *	$f10:	32 bytes data (manually saved)
 *
 */

#include <asm/export.h>
#include <asm/csr.h>

#define FIXUP_LDST(x, y)	\
	x, y

	.set noat
	.set noreorder
	.text
	.align 4
	.globl memset
	.globl __memset
	.globl ___memset
	.globl __memsetw
	.globl __constant_c_memset
	.ent ___memset
___memset:
	.frame $30, 0, $26, 0
	.prologue 0

#ifdef CONFIG_SUBARCH_C4
	csrr	$6, CSR_WR_FREGS
#endif
/* expand 1 byte data to 8 bytes */
	and	$17, 0xff, $17
	sll	$17, 8, $4
	bis	$17, $4, $17
	sll	$17, 16, $4
	bis	$17, $4, $17
	sll	$17, 32, $4
	bis	$17, $4, $17

__constant_c_memset:
	bis	$31, $31, $7
	bis	$31, $16, $0
#if defined(CONFIG_SUBARCH_C3B)
#include "deep-set_template.S"
#elif defined(CONFIG_SUBARCH_C4)
#include "deep-set_template_c4.S"
#endif
$out:
#ifdef CONFIG_SUBARCH_C4
	csrw	$6, CSR_WR_FREGS
#endif
	ret

	.end ___memset
	EXPORT_SYMBOL(___memset)

	.align 5
	.ent __memsetw
__memsetw:
	.prologue 0

	inslh	$17, 0, $1
	inslh	$17, 2, $2
	inslh	$17, 4, $3
	bis	$1, $2, $1
	inslh	$17, 6, $4
	bis	$1, $3, $1
	bis	$1, $4, $17
	br	$31, __constant_c_memset

	.end __memsetw
	EXPORT_SYMBOL(__memsetw)

memset = ___memset
EXPORT_SYMBOL(memset)
__memset = ___memset
EXPORT_SYMBOL(__memset)
