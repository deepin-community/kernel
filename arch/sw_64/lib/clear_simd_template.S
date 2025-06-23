/* SPDX-License-Identifier: GPL-2.0 */

/*
 * template for memclear and clear_user with SIMD
 * no need to worry about SIMD save/restore
 */

	beq	NBYTES, L_OUT
	cmplt	NBYTES, 64, T0
	bne	T0, $move_32
	.align 4
$loop_64:
	FIXUP_LDST( vstd $f31, 0(DST) )
	FIXUP_LDST( vstd $f31, 32(DST) )
	subl	NBYTES, 64, NBYTES
	beq	NBYTES, L_OUT
	addl	DST, 64, DST
	cmplt	NBYTES, 64, T0
	beq	T0, $loop_64

$move_32:
	cmplt	NBYTES, 32, T0
	bne	T0, $move_16
	FIXUP_LDST( vstd $f31, 0(DST) )
	subl	NBYTES, 32, NBYTES
	beq	NBYTES, L_OUT
	addl	DST, 32, DST

$move_16:
	cmplt	NBYTES, 16, T0
	bne	T0, $move_8
	FIXUP_LDST( stl $31, 0(DST) )
	FIXUP_LDST( stl $31, 8(DST) )
	subl	NBYTES, 16, NBYTES
	beq	NBYTES, L_OUT
	addl	DST, 16, DST

$move_8:
	cmplt	NBYTES, 8, T0
	bne	T0, $loop_1
	FIXUP_LDST( stl $31, 0(DST) )
	subl	NBYTES, 8, NBYTES
	beq	NBYTES, L_OUT
	addl	DST, 8, DST

	.align 4
$loop_1:
	FIXUP_LDST( stb $31, 0(DST) )
	subl	NBYTES, 1, NBYTES
	addl	DST, 1, DST
	bne	NBYTES, $loop_1
