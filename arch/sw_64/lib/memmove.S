/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Barely optimized memmove routine for sw64.
 * This is hand-massaged output from the original memcpy.c.  We defer to
 * memcpy whenever possible; the backwards copy loops are not unrolled.
 */
#include <asm/export.h>
	.set noat
	.set noreorder
	.text

	.align 4
	.globl memmove
	.ent memmove
memmove:
	ldgp	$29, 0($27)
	unop
	.prologue 1

	addl	$16, $18, $4
	addl	$17, $18, $5
	cmpule	$4, $17, $1		# dest + n <= src
	cmpule	$5, $16, $2		# dest >= src + n

	bis	$1, $2, $1
	mov	$16, $0
	xor	$16, $17, $2
	beq	$1, $memmove_cont
	ldi	$27, memcpy
	jmp	$31, ($27)
	/* should never reach here
	 * memcpy should return to caller directly */

$memmove_cont:
	and	$2, 7, $2		# Test for src/dest co-alignment.
	and	$16, 7, $1
	cmpule	$16, $17, $3
	bne	$3, $memmove_up		# dest < src

	and	$4, 7, $1
	bne	$2, $misaligned_dn
	unop
	beq	$1, $skip_aligned_byte_loop_head_dn

$aligned_byte_loop_head_dn:
	ldi	$4, -1($4)
	ldi	$5, -1($5)
	unop
	ble	$18, $egress

	ldbu	$1, 0($5)
	ldi	$18, -1($18)
	stb	$1, 0($4)

	and	$4, 7, $6
	bne	$6, $aligned_byte_loop_head_dn

$skip_aligned_byte_loop_head_dn:
	ldi	$18, -8($18)
	blt	$18, $skip_aligned_word_loop_dn

$aligned_word_loop_dn:
	ldl	$1, -8($5)
	ldi	$5, -8($5)
	ldi	$18, -8($18)

	stl	$1, -8($4)
	ldi	$4, -8($4)
	bge	$18, $aligned_word_loop_dn

$skip_aligned_word_loop_dn:
	ldi	$18, 8($18)
	bgt	$18, $byte_loop_tail_dn
	unop
	ret	$31, ($26), 1

	.align 4
$misaligned_dn:
	fnop
	unop
	beq	$18, $egress

$byte_loop_tail_dn:
	ldbu	$1, -1($5)
	ldi	$5, -1($5)
	ldi	$4, -1($4)

	ldi	$18, -1($18)
	stb	$1, 0($4)

	bgt	$18, $byte_loop_tail_dn
	br	$egress

$memmove_up:
	mov	$16, $4
	mov	$17, $5
	bne	$2, $misaligned_up
	beq	$1, $skip_aligned_byte_loop_head_up

$aligned_byte_loop_head_up:
	unop
	ble	$18, $egress
	ldbu	$1, 0($5)

	ldi	$18, -1($18)

	ldi	$5, 1($5)
	stb	$1, 0($4)
	ldi	$4, 1($4)

	and	$4, 7, $6
	bne	$6, $aligned_byte_loop_head_up

$skip_aligned_byte_loop_head_up:
	ldi	$18, -8($18)
	blt	$18, $skip_aligned_word_loop_up

$aligned_word_loop_up:
	ldl	$1, 0($5)
	ldi	$5, 8($5)
	ldi	$18, -8($18)

	stl	$1, 0($4)
	ldi	$4, 8($4)
	bge	$18, $aligned_word_loop_up

$skip_aligned_word_loop_up:
	ldi	$18, 8($18)
	bgt	$18, $byte_loop_tail_up
	unop
	ret	$31, ($26), 1

	.align 4
$misaligned_up:
	fnop
	unop
	beq	$18, $egress

$byte_loop_tail_up:
	ldbu	$1, 0($5)
	ldi	$18, -1($18)

	stb	$1, 0($4)

	ldi	$5, 1($5)
	ldi	$4, 1($4)
	bgt	$18, $byte_loop_tail_up

$egress:
	ret	$31, ($26), 1

	.end memmove
	EXPORT_SYMBOL(memmove)
