/* SPDX-License-Identifier: GPL-2.0 */

/*
 * template for memcpy and copy_user with SIMD
 *
 * $7:	SIMD status
 *	0: not in simd loop
 *	1: in simd and simd_u loop
 * $16:	latest dest, clobbered
 * $17:	latest src, clobbered
 * $18:	bytes left to copy
 *
 */

#define SAVE_SIMD_REGS \
	ldi	$sp, -0x60($sp); \
	addl	$sp, 0x1f, $23; \
	bic	$23, 0x1f, $23; \
	vstd	$f1, 0($23); \
	vstd	$f2, 0x20($23); \
	ldi	$7, 1

#define RESTORE_SIMD_REGS \
	addl	$sp, 0x1f, $23; \
	bic	$23, 0x1f, $23; \
	vldd	$f1, 0($23); \
	vldd	$f2, 0x20($23); \
	ldi	$sp, 0x60($sp); \
	bis	$31, $31, $7


	ble	$18, 255f

	cmplt	$18, 8, $1
	bne	$1, $byte_loop_tail
	cmplt	$18, 16, $1
	bne	$1, $quad_loop_end
	cmplt	$18, 32, $1
	bne	$1, $simd_end

$prep_simd_loop:
	SAVE_SIMD_REGS
	cmplt	$18, 64, $1
	bne	$1, $simd_loop_end

	.align 4
$simd_loop:
	FIXUP_LDST( vldd $f1, 0($17) )
	FIXUP_LDST( vldd $f2, 32($17) )
	FIXUP_LDST( vstd $f1, 0($16) )
	FIXUP_LDST( vstd $f2, 32($16) )
	subl	$18, 64, $18
	addl	$17, 64, $17
	addl	$16, 64, $16
	cmplt	$18, 64, $1
	beq	$1, $simd_loop

$simd_loop_end:
	cmplt	$18, 32, $1
	bne	$1, $no_more_simd
	FIXUP_LDST( vldd $f1, 0($17) )
	FIXUP_LDST( vstd $f1, 0($16) )
	subl	$18, 32, $18
	addl	$17, 32, $17
	addl	$16, 32, $16

$no_more_simd:
	RESTORE_SIMD_REGS

$simd_end:
	ble	$18, 255f
	cmplt	$18, 16, $1
	bne	$1, $quad_loop_end

	.align 4
$quad_loop_tail:
	FIXUP_LDST( ldl $2, 0($17) )
	FIXUP_LDST( ldl $3, 8($17) )
	FIXUP_LDST( stl $2, 0($16) )
	FIXUP_LDST( stl $3, 8($16) )
	subl	$18, 16, $18
	addl	$17, 16, $17
	addl	$16, 16, $16
	cmplt	$18, 16, $1
	beq	$1, $quad_loop_tail

$quad_loop_end:
	ble	$18, 255f
	cmplt	$18, 8, $1
	bne	$1, $byte_loop_tail

$move_one_quad:
	FIXUP_LDST( ldl $2, 0($17) )
	FIXUP_LDST( stl $2, 0($16) )
	subl	$18, 8, $18
	addl	$17, 8, $17
	addl	$16, 8, $16
	ble	$18, 255f

	.align 3
$byte_loop_tail:
	FIXUP_LDST( ldbu $2, 0($17) )
	FIXUP_LDST( stb $2, 0($16) )
	subl	$18, 1, $18
	addl	$17, 1, $17
	addl	$16, 1, $16
	bgt	$18, $byte_loop_tail
	br	$31, 255f
