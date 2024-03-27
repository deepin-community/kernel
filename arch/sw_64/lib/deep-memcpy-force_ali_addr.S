/* SPDX-License-Identifier: GPL-2.0 */

#define FIXUP_LDST(x, y)	\
	x, y

	.globl ____memcpy_simd_force_ali_addr
	.ent ____memcpy_simd_force_ali_addr
____memcpy_simd_force_ali_addr:
	.frame $30, 0, $26, 0
	.prologue 0
	mov	$16, $0
#include "deep-copy_template-force_ali_addr.S"
255:
	ret
	.end ____memcpy_simd_force_ali_addr
