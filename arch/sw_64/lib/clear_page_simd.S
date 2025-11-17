/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Zero an entire page.
 */
#include <asm/csr.h>
#include <asm/export.h>
#include <asm/page.h>

#ifndef CONFIG_SUBARCH_C3B
# define VSTD_CPU	vstd
#else
# define VSTD_CPU	vstd_nc
#endif

	.text
	.align 5
	.global clear_page
	.ent clear_page
clear_page:
	.prologue 0

	ldi		$1, PAGE_SIZE($16)

1:
	VSTD_CPU	$f31, 0($16)
	VSTD_CPU	$f31, 32($16)
	VSTD_CPU	$f31, 64($16)
	VSTD_CPU	$f31, 96($16)
	addl		$16, 128, $16
	.align 3
	cmpeq		$16, $1, $2
	beq		$2, 1b

#ifdef CONFIG_SUBARCH_C3B
	memb
#endif

	.align 4
	nop
	ret

	.end clear_page
	EXPORT_SYMBOL(clear_page)
