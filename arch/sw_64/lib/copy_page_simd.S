/* SPDX-License-Identifier: GPL-2.0 */
/*
 * arch/sw/lib/copy_page.S
 *
 * Copy an entire page.
 */
#include <asm/export.h>
#include <asm/csr.h>
#include <asm/page.h>

#ifndef CONFIG_SUBARCH_C3B
# define VSTD_CPU	vstd
#else
# define VSTD_CPU	vstd_nc
#endif

	.text
	.align 5
	.global copy_page
	.ent copy_page
copy_page:
	.prologue 0

	ldi		$1, PAGE_SIZE($16)

	subl		$sp, 64, $sp
	bic		$sp, 0x1f, $3
	vstd		$f10, 0($3)

#ifdef CONFIG_SUBARCH_C4
	csrr		$4, CSR_WR_FREGS
#endif

1:
	vldd		$f10, 0($17)
	VSTD_CPU	$f10, 0($16)

	vldd		$f10, 32($17)
	VSTD_CPU	$f10, 32($16)

	vldd		$f10, 64($17)
	VSTD_CPU	$f10, 64($16)

	vldd		$f10, 96($17)
	VSTD_CPU	$f10, 96($16)

	addl		$16, 128, $16
	addl		$17, 128, $17

	.align 3
	cmpeq		$16, $1, $2
	beq		$2, 1b

#ifdef CONFIG_SUBARCH_C4
	csrw		$4, CSR_WR_FREGS
#endif
#ifdef CONFIG_SUBARCH_C3B
	memb
#endif

	vldd		$f10, 0($3)
	addl		$sp, 64, $sp

	.align 4
	nop
	ret

	.end copy_page
	EXPORT_SYMBOL(copy_page)
