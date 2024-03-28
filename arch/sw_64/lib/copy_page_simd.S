/* SPDX-License-Identifier: GPL-2.0 */
/*
 * arch/sw/lib/copy_page.S
 *
 * Copy an entire page.
 */
#include <asm/export.h>
#include <asm/csr.h>

	.text
	.align 4
	.global copy_page
	.ent copy_page
copy_page:
	.prologue 0

	ldi	$18, 64
	subl	$sp, 0x60, $sp
	ldi	$4, 0x40($sp)
	stl	$4, 0($sp)
	bic	$4, 0x1f, $4
	vstd	$f16, 0($4)
#ifdef CONFIG_SUBARCH_C4
	csrr	$5, CSR_WR_FREGS
#endif

/* Optimize by GUOY from SOC 2013-06-04 */
1:
	vldd	$f16, 0($17)
	vstd_nc	$f16, 0($16)

	vldd	$f16, 32($17)
	vstd_nc	$f16, 32($16)

	vldd	$f16, 64($17)
	vstd_nc	$f16, 64($16)

	vldd	$f16, 96($17)
	vstd_nc	$f16, 96($16)

	ldwe	$f31, 5*0x80($17)
	subl	$18, 1, $18
	addl	$17, 128, $17

	addl	$16, 128, $16
	bne	$18, 1b

	memb
	ldl	$4, 0($sp)
	ldi	$4, 0x40($sp)
	bic	$4, 0x1f, $4
	vldd	$f16, 0($4)
#ifdef CONFIG_SUBARCH_C4
	csrw	$5, CSR_WR_FREGS
#endif
	addl	$sp, 0x60, $sp
	ret

	.end copy_page
	EXPORT_SYMBOL(copy_page)
