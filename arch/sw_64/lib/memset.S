/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This is an efficient (and small) implementation of the C library "memset()"
 * function for the sw.
 *
 *	(C) Copyright 1996 Linus Torvalds
 *
 * This routine is "moral-ware": you are free to use it any way you wish, and
 * the only obligation I put on you is a moral one: if you make any improvements
 * to the routine, please send me your improvements for me to use similarly.
 *
 * The scheduling comments are according to the documentation (and done by
 * hand, so they might well be incorrect, please do tell me about it..)
 */

	.set noat
	.set noreorder
.text
	.globl ____constant_c_memset_sisd

	.ent ____constant_c_memset_sisd
	.frame $30, 0, $26, 0
	.prologue 0
.align 5
____constant_c_memset_sisd:
	addl	$18, $16, $6
	bis	$16, $16, $0
	xor	$16, $6, $1
	ble	$18, end

	bic	$1, 7, $1
	beq	$1, within_one_quad
	and	$16, 7, $3
	beq	$3, aligned

	bis	$16, $16, $5
	subl	$3, 8, $3
	addl	$18, $3, $18
	subl	$16, $3, $16

	eqv	$3, $31, $3
	addl	$3, 1, $3
unaligned_start_loop:
	stb	$17, 0($5)
	subl	$3, 1, $3
	addl	$5, 1, $5
	bgt	$3, unaligned_start_loop


.align 4
aligned:
	sra	$18, 3, $3
	and	$18, 7, $18
	bis	$16, $16, $5
	beq	$3, no_quad

/*added by JJ*/
	ldi	$3, -8($3)
	blt	$3, nounrol

.align 3
wloop:
	fillde	256($5)
	stl	$17, 0($5)
	stl	$17, 8($5)
	stl	$17, 16($5)
	stl	$17, 24($5)
	subl	$3, 8, $3
	stl	$17, 32($5)
	stl	$17, 40($5)
	stl	$17, 48($5)
	stl	$17, 56($5)
	addl	$5, 0x40, $5
	bge	$3, wloop

nounrol:
	addl	$3, 8, $3
	beq	$3, no_quad
/*end JJ*/

.align 3
loop:
	stl	$17, 0($5)
	subl	$3, 1, $3
	addl	$5, 8, $5
	bne	$3, loop

no_quad:
	bis	$31, $31, $31
	beq	$18, end
	and	$6, 7, $6
no_quad_loop:
	stb	$17, 0($5)
	subl	$6, 1, $6
	addl	$5, 1, $5
	bgt	$6, no_quad_loop
	ret	$31, ($26), 1

.align 3
within_one_quad:
	bis	$18, $18, $1
	bis	$16, $16, $5
within_one_quad_loop:
	stb	$17, 0($5)
	subl	$1, 1, $1
	addl	$5, 1, $5
	bgt	$1, within_one_quad_loop

end:
	ret	$31, ($26), 1
	.end ____constant_c_memset_sisd
