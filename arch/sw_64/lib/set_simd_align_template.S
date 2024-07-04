/* SPDX-License-Identifier: GPL-2.0 */

/*
 * template for memset and clear_user with SIMD
 *
 * $7:	SIMD status
 *	0: not in simd loop
 *	1: in simd loop
 *	2: in simd_u loop
 * $16:	latest dest, clobbered
 * $17:	8-byte data to set
 * $18:	bytes left to copy
 *
 */

#ifdef CONFIG_SUBARCH_C3B
#define NC_STORE_THRESHOLD	2048
#endif

#define SAVE_SIMD_REGS \
	ldi	$sp, -0x40($sp); \
	addl	$sp, 0x1f, $23; \
	bic	$23, 0x1f, $23; \
	vstd	$f1, 0($23); \
	ldi	$7, 1

#define RESTORE_SIMD_REGS \
	vldd	$f1, 0($23); \
	ldi	$sp, 0x40($sp); \
	bis	$31, $31, $7

	ble	$18, 255f
	and	$16, 7, $1
	beq	$1, $dest_aligned_8

	.align 3
$byte_loop_head:
	FIXUP_LDST( stb $17, 0($16) )
	subl	$18, 1, $18
	addl	$16, 1, $16
	ble	$18, 255f
	and	$16, 7, $1
	bne	$1, $byte_loop_head

$dest_aligned_8:
	cmplt	$18, 16, $1
	bne	$1, $quad_loop_end
	and	$16, 31, $1
	beq	$1, $dest_aligned_32
	cmplt	$18, 64, $1
	bne	$1, $simd_end

	.align 3
$quad_loop_head:
	FIXUP_LDST( stl $17, 0($16) )
	addl	$16, 8, $16
	subl	$18, 8, $18
	and	$16, 31, $1
	beq	$1, $dest_aligned_32
	br	$31, $quad_loop_head

$dest_aligned_32:
	cmplt	$18, 64, $1
	bne	$1, $simd_end

$prep_simd_loop:
	SAVE_SIMD_REGS
	ifmovd	$17, $f1
	vcpyf	$f1, $f1
#ifdef CONFIG_SUBARCH_C3B
	ldi	$1, NC_STORE_THRESHOLD($31)
	cmple	$18, $1, $1
	bne	$1, $simd_loop

	.align 3
$simd_loop_nc:
	FIXUP_LDST( vstd_nc $f1, 0($16) )
	FIXUP_LDST( vstd_nc $f1, 32($16) )
	subl	$18, 64, $18
	addl	$16, 64, $16
	cmplt	$18, 64, $1
	beq	$1, $simd_loop_nc
	memb			# required for _nc store instructions
	br	$31, $simd_loop_end
#endif

	.align 3
$simd_loop:
	FIXUP_LDST( vstd $f1, 0($16) )
	FIXUP_LDST( vstd $f1, 32($16) )
	subl	$18, 64, $18
	addl	$16, 64, $16
	cmplt	$18, 64, $1
	beq	$1, $simd_loop

$simd_loop_end:
	cmplt	$18, 32, $1
	bne	$1, $no_more_simd
	FIXUP_LDST( vstd $f1, 0($16) )
	subl	$18, 32, $18
	addl	$16, 32, $16

$no_more_simd:
	RESTORE_SIMD_REGS

$simd_end:
	ble	$18, 255f
	cmplt	$18, 16, $1
	bne	$1, $quad_loop_end

	.align 3
$quad_loop_tail:
	FIXUP_LDST( stl $17, 0($16) )
	FIXUP_LDST( stl $17, 8($16) )
	subl	$18, 16, $18
	addl	$16, 16, $16
	cmplt	$18, 16, $1
	beq	$1, $quad_loop_tail

$quad_loop_end:
	ble	$18, 255f
	cmplt	$18, 8, $1
	bne	$1, $byte_loop_tail

$move_one_quad:
	FIXUP_LDST( stl $17, 0($16) )
	subl	$18, 8, $18
	addl	$16, 8, $16
	ble	$18, 255f

	.align 3
$byte_loop_tail:
	FIXUP_LDST( stb $17, 0($16) )
	subl	$18, 1, $18
	addl	$16, 1, $16
	bgt	$18, $byte_loop_tail
	br	$31, 255f
