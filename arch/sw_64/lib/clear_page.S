/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Zero an entire page.
 */
#include <asm/export.h>
	.text
	.align 4
	.global clear_page
	.ent clear_page
clear_page:
	.prologue 0

	ldi	$0, 64

/* Optimize by GUOY from SOC 2013-06-04 */
1:

	stl_nc	$31, 0x0($16)
	stl_nc	$31, 0x8($16)
	stl_nc	$31, 0x10($16)
	stl_nc	$31, 0x18($16)

	stl_nc	$31, 0x20($16)
	stl_nc	$31, 0x28($16)
	stl_nc	$31, 0x30($16)
	stl_nc	$31, 0x38($16)

	stl_nc	$31, 0x40($16)
	stl_nc	$31, 0x48($16)
	stl_nc	$31, 0x50($16)
	stl_nc	$31, 0x58($16)

	stl_nc	$31, 0x60($16)
	stl_nc	$31, 0x68($16)
	subl	$0, 1, $0

	stl_nc	$31, 0x70($16)
	stl_nc	$31, 0x78($16)
	addl	$16, 128, $16
	bne	$0, 1b

	memb
	ret

	.end clear_page
	EXPORT_SYMBOL(clear_page)
