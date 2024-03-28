/* SPDX-License-Identifier: GPL-2.0 */

#include <asm/csr.h>

#define FIXUP_LDST(x, y)	\
	x, y

	.globl ____memcpy_simd_align
	.ent ____memcpy_simd_align
____memcpy_simd_align:
	.frame $30, 0, $26, 0
	.prologue 0
	mov	$16, $0
#ifdef CONFIG_SUBARCH_C4
	csrr	$6, CSR_WR_FREGS
#endif
#include "copy_simd_align_template.S"
255:
#ifdef CONFIG_SUBARCH_C4
	csrw	$6, CSR_WR_FREGS
#endif
	ret
	.end ____memcpy_simd_align
