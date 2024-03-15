/* SPDX-License-Identifier: GPL-2.0 */

#define FIXUP_LDST(x, y)	\
	x, y

	.globl ____memcpy_sw_una
	.ent ____memcpy_sw_una
____memcpy_sw_una:
	.frame $30, 0, $26, 0
	.prologue 0
	mov	$16, $0
#include "deep-copy_template.S"
255:
	ret
	.end ____memcpy_sw_una
