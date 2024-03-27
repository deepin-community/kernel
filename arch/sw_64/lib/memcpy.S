/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Reasonably optimized ____memcpy_sisd() routine for the sw64
 *
 *	- memory accessed as aligned quadwords only
 *	- uses bcmpge to compare 8 bytes in parallel
 *
 * Temp usage notes:
 *	$1, $2, - scratch
 */
	.set noreorder
	.set noat

	.align 4
	.globl ____memcpy_sisd
	.ent ____memcpy_sisd
____memcpy_sisd:
	.frame $30, 0, $26, 0
	.prologue 0

	mov	$16, $0
	ble	$18, $nomoredata
	xor	$16, $17, $1
	and	$1, 7, $1

	bne	$1, $misaligned
	/* source and dest are same mod 8 address */
	and	$16, 7, $1
	beq	$1, $both_0mod8

	/*
	 * source and dest are same misalignment.  move a byte at a time
	 * until a 0mod8 alignment for both is reached.
	 * At least one byte more to move
	 */

$head_align:
	ldbu	$1, 0($17)
	subl	$18, 1, $18
	addl	$17, 1, $17
	stb	$1, 0($16)
	addl	$16, 1, $16
	and	$16, 7, $1
	ble	$18, $nomoredata
	bne	$1, $head_align

$both_0mod8:
	cmple	$18, 127, $1
	bne	$1, $no_unroll
	and	$16, 63, $1
	beq	$1, $do_unroll

$single_head_quad:
	ldl	$1, 0($17)
	subl	$18, 8, $18
	addl	$17, 8, $17

	stl	$1, 0($16)
	addl	$16, 8, $16
	and	$16, 63, $1
	bne	$1, $single_head_quad

$do_unroll:
	addl	$16, 64, $7
	cmple	$18, 127, $1
	bne	$1, $tail_quads

$unroll_body:
	#wh64	($7)
	fillde	0($7)

	ldl	$6, 0($17)

	ldl	$4, 8($17)
	ldl	$5, 16($17)
	addl	$7, 64, $7

	ldl	$3, 24($17)
	addl	$16, 64, $1

	addl	$17, 32, $17
	stl	$6, 0($16)

	stl	$4, 8($16)
	stl	$5, 16($16)
	subl	$18, 192, $2

	stl	$3, 24($16)
	addl	$16, 32, $16

	ldl	$6, 0($17)
	ldl	$4, 8($17)
	#cmovlt	$2, $1, $7
	sellt	$2, $1, $7, $7

	ldl	$5, 16($17)
	ldl	$3, 24($17)
	addl	$16, 32, $16
	subl	$18, 64, $18

	addl	$17, 32, $17
	stl	$6, -32($16)
	stl	$4, -24($16)
	cmple	$18, 63, $1

	stl	$5, -16($16)
	stl	$3, -8($16)
	beq	$1, $unroll_body

$tail_quads:
$no_unroll:
	.align 4
	subl	$18, 8, $18
	blt	$18, $less_than_8

$move_a_quad:
	ldl	$1, 0($17)
	subl	$18, 8, $18
	addl	$17, 8, $17

	stl	$1, 0($16)
	addl	$16, 8, $16
	bge	$18, $move_a_quad

$less_than_8:
	.align 4
	addl	$18, 8, $18
	ble	$18, $nomoredata

	/* Trailing bytes */
$tail_bytes:
	subl	$18, 1, $18
	ldbu	$1, 0($17)
	addl	$17, 1, $17

	stb	$1, 0($16)
	addl	$16, 1, $16
	bgt	$18, $tail_bytes

	/* branching to exit takes 3 extra cycles, so replicate exit here */
	ret	$31, ($26), 1

$misaligned:
	mov	$0, $4
	and	$0, 7, $1
	beq	$1, $dest_0mod8

$aligndest:
	ble	$18, $nomoredata
	ldbu	$1, 0($17)
	subl	$18, 1, $18
	addl	$17, 1, $17

	stb	$1, 0($4)
	addl	$4, 1, $4
	and	$4, 7, $1
	bne	$1, $aligndest

	/* Source has unknown alignment, but dest is known to be 0mod8 */
$dest_0mod8:
	subl	$18, 8, $18
	blt	$18, $misalign_tail
	ldl_u	$3, 0($17)

$mis_quad:
	ldl_u	$16, 8($17)
	extll	$3, $17, $3
	exthl	$16, $17, $1
	bis	$3, $1, $1

	subl	$18, 8, $18
	addl	$17, 8, $17
	stl	$1, 0($4)
	mov	$16, $3

	addl	$4, 8, $4
	bge	$18, $mis_quad

$misalign_tail:
	addl	$18, 8, $18
	ble	$18, $nomoredata

$misalign_byte:
	ldbu	$1, 0($17)
	subl	$18, 1, $18
	addl	$17, 1, $17

	stb	$1, 0($4)
	addl	$4, 1, $4
	bgt	$18, $misalign_byte


$nomoredata:
	ret	$31, ($26), 1

	.end ____memcpy_sisd
