/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Optimized strncpy() for SW64

 * Copyright (C) Mao Minkai
 * Author: Mao Minkai
 *
 * Copy a string from SRC to DST. At most SIZE bytes are coppied.
 *
 * Input:
 *	$16:	DST, clobbered
 *	$17:	SRC, clobbered
 *	$18:	SIZE, clobbered
 *
 * Output:
 *	$0:	DST
 *
 * Temporaries:
 *	$1:	unaligned parts of addr (0 means aligned addr)
 *	$4:	current data to copy (could have 1 byte or 8 bytes)
 *	$5:	parts of current data, compare result
 *	$6:	number of bytes left to copy in head
 *
 * Tag naming:
 *	co:	SRC and DST are co-aligned
 *	mis:	SRC and DST are not co-aligned
 *	a:	SRC or DST has aligned address
 *	una:	SRC or DST has unaligned address
 *
 */

#include <asm/export.h>

	.text
	.align 4
	.globl strncpy
	.ent strncpy
strncpy:
	.frame $30, 0, $26
	.prologue 0

	bis	$31, $16, $0		# set return value
	beq	$18, $out		# return if size is 0
	cmplt	$18, 8, $5		# size less than 8, do 1-byte copy
	bne	$5, $tail_loop

	xor	$16, $17, $1
	and	$1, 7, $1
	bne	$1, $mis_aligned

/* src and dst are co-aligned */
	and	$16, 7, $1
	bne	$1, $co_una_head

/* do the copy in loop, for (co)-aligned src and dst with (a)ligned addr */
$co_a_loop:
	ldl	$4, 0($17)
	cmpgeb	$31, $4, $5
	bne	$5, $tail_loop		# we find null
	subl	$18, 8, $5
	blt	$5, $tail_loop		# we have fewer than 8 bytes to copy
	stl	$4, 0($16)
	subl	$18, 8, $18
	beq	$18, $out
	addl	$17, 8, $17
	addl	$16, 8, $16
	br	$31, $co_a_loop

/* src and dst are co-aligned but have unaligned address */
$co_una_head:
	ldl_u	$4, 0($17)
	extll	$4, $16, $4
	cmpgeb	$31, $4, $5
	bne	$5, $tail_loop		# we find null
	ldi	$6, 8($31)
	subl	$6, $1, $6
	addl	$17, $6, $17		# prepare addr of middle part
	subl	$18, $6, $18		# sub bytes going to be copy

/* copy the unaligned part in loop */
$co_una_head_loop:
	stb	$4, 0($16)
	addl	$16, 1, $16
	subl	$6, 1, $6
	beq	$6, $co_a_loop
	addl	$4, 1, $4
	br	$31, $co_una_head_loop

/* src and dst are not co-aligned */
$mis_aligned:
	and	$16, 7, $1
	beq	$1, $mis_a_dst

$mis_una_head:
	ldi	$6, 8($31)
	subl	$6, $1, $6

/* copy the first few bytes to make dst aligned */
$mis_una_head_loop:
	ldbu	$4, 0($17)
	stb	$4, 0($16)
	subl	$18, 1, $18
	beq	$18, $out
	beq	$4, $null_padding	# we have reached null
	addl	$17, 1, $17
	addl	$16, 1, $16
	subl	$6, 1, $6
	beq	$6, $mis_a_dst
	br	$31, $mis_una_head_loop

/* dst has aligned addr */
$mis_a_dst:
	and	$17, 7, $1

$mis_a_dst_loop:
	ldl_u	$4, 0($17)
	ldl_u	$5, 7($17)
	extll	$4, $1, $4
	exthl	$5, $1, $5
	bis	$4, $5, $4
	cmpgeb	$31, $4, $5
	bne	$5, $tail_loop		# we find null
	subl	$18, 8, $5
	blt	$5, $tail_loop		# we have fewer than 8 bytes to copy
	stl	$4, 0($16)
	subl	$18, 8, $18
	beq	$5, $out
	addl	$17, 8, $17
	addl	$16, 8, $16
	br	$31, $mis_a_dst_loop

/* we have find null in the last few bytes, copy one byte each time */
$tail_loop:
	ldbu	$4, 0($17)
	stb	$4, 0($16)
	subl	$18, 1, $18
	beq	$18, $out
	beq	$4, $null_padding	# we have reached null
	addl	$17, 1, $17
	addl	$16, 1, $16
	br	$31, $tail_loop

$null_padding:
	addl	$16, 1, $16
	subl	$18, 1, $18
	stb	$31, 0($16)
	beq	$18, $out
	br	$31, $null_padding

/* copy is done, return */
$out:
	ret

	.end strncpy
	EXPORT_SYMBOL(strncpy)
