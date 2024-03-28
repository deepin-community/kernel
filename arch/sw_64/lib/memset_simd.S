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

#include <asm/csr.h>

#define FIXUP_LDST(x, y)	\
	x, y

	.set noat
	.set noreorder
	.text
	.align 4

	.globl ____constant_c_memset_simd
	.ent ____constant_c_memset_simd
	.frame $30, 0, $26, 0
	.prologue 0
____constant_c_memset_simd:
	bis	$31, $31, $7
	bis	$31, $16, $0
#ifdef CONFIG_SUBARCH_C4
	csrr	$6, CSR_WR_FREGS
#endif
#include "set_simd_template.S"
255:
#ifdef CONFIG_SUBARCH_C4
	csrw	$6, CSR_WR_FREGS
#endif
	ret
	.end ____constant_c_memset_simd
