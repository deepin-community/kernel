/* SPDX-License-Identifier: GPL-2.0 */

#define FIXUP_LDST(x, y)	\
	x, y

	.globl ____memcpy_hw_una
	.ent ____memcpy_hw_una
____memcpy_hw_una:
	.frame $30, 0, $26, 0
	.prologue 0
	mov	$16, $0
#include "deep-copy_template-no_una_check.S"
255:
	ret
	.end ____memcpy_hw_una
