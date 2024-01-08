/* SPDX-License-Identifier: GPL-2.0 */
/*
 * arch/sw/lib/copy_page.S
 *
 * Copy an entire page.
 */
#include <asm/export.h>

	.text
	.align 4
	.global copy_page
	.ent copy_page
copy_page:
	.prologue 0

	ldi	$18, 64

/* Optimize by GUOY from SOC 2013-06-04 */
1:
	ldl	$0, 0($17)
	ldl	$1, 8($17)
	ldl	$2, 16($17)
	ldl	$3, 24($17)

	stl_nc	$0, 0($16)
	stl_nc	$1, 8($16)
	stl_nc	$2, 16($16)
	stl_nc	$3, 24($16)

	ldl	$4, 32($17)
	ldl	$5, 40($17)
	ldl	$6, 48($17)
	ldl	$7, 56($17)

	stl_nc	$4, 32($16)
	stl_nc	$5, 40($16)
	stl_nc	$6, 48($16)
	stl_nc	$7, 56($16)

	ldl	$0, 64($17)
	ldl	$1, 72($17)
	ldl	$2, 80($17)
	ldl	$3, 88($17)

	stl_nc	$0, 64($16)
	stl_nc	$1, 72($16)
	stl_nc	$2, 80($16)
	stl_nc	$3, 88($16)

	ldl	$4, 96($17)
	ldl	$5, 104($17)
	ldl	$6, 112($17)
	ldl	$7, 120($17)

	stl_nc	$4, 96($16)
	stl_nc	$5, 104($16)
	stl_nc	$6, 112($16)
	stl_nc	$7, 120($16)

	ldwe	$f31, 3 * 0x80($17)
	subl	$18, 1, $18
	addl	$17, 128, $17

	addl	$16, 128, $16
	bne	$18, 1b

	memb
	ret

	.end copy_page
	EXPORT_SYMBOL(copy_page)
