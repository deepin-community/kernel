/* SPDX-License-Identifier: GPL-2.0 */

/*
 * template for memcpy and copy_user with SIMD
 *
 * $4:	8-byte misalignment of src when dest is 8-byte aligned
 * $5:	32-byte misalignment of src when dest is 32-byte aligned
 * $7:	SIMD status
 *	0: not in simd loop
 *	1: in simd loop
 *	2: in simd_u loop
 * $16:	latest dest, clobbered
 * $17:	latest src, clobbered
 * $18:	bytes left to copy
 *
 */

#define NC_STORE_THRESHOLD	2048

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

#define SAVE_SIMD_U_REGS \
	ldi	$sp, -0xc0($sp); \
	addl	$sp, 0x1f, $23; \
	bic	$23, 0x1f, $23; \
	vstd	$f1, 0($23); \
	vstd	$f2, 0x20($23); \
	vstd	$f4, 0x40($23); \
	vstd	$f5, 0x60($23); \
	vstd	$f3, 0x80($23); \
	ldi	$7, 2

#define RESTORE_SIMD_U_REGS \
	addl	$sp, 0x1f, $23; \
	bic	$23, 0x1f, $23; \
	vldd	$f1, 0($23); \
	vldd	$f2, 0x20($23); \
	vldd	$f4, 0x40($23); \
	vldd	$f5, 0x60($23); \
	vldd	$f3, 0x80($23); \
	ldi	$sp, 0xc0($sp); \
	bis	$31, $31, $7

	ble	$18, $out
	and	$16, 7, $1
	beq	$1, $dest_aligned_8

$byte_loop_head:
	FIXUP_LDST( ldbu $2, 0($17) )
	FIXUP_LDST( stb $2, 0($16) )
	subl	$18, 1, $18
	addl	$17, 1, $17
	addl	$16, 1, $16
	ble	$18, $out
	and	$16, 7, $1
	bne	$1, $byte_loop_head

$dest_aligned_8:
	and	$17, 7, $4
	cmplt	$18, 16, $1
	bne	$1, $quad_loop_end
	and	$16, 31, $1
	beq	$1, $dest_aligned_32
	cmplt	$18, 64, $1
	bne	$1, $simd_end
	bne	$4, $quad_u_loop_head

$quad_loop_head:
	FIXUP_LDST( ldl $2, 0($17) )
	FIXUP_LDST( stl $2, 0($16) )
	addl	$16, 8, $16
	addl	$17, 8, $17
	subl	$18, 8, $18
	and	$16, 31, $1
	beq	$1, $dest_aligned_32
	br	$31, $quad_loop_head

$dest_aligned_32:
	cmplt	$18, 64, $1
	bne	$1, $simd_end
	and	$17, 31, $5
	bne	$5, $prep_simd_u_loop

$prep_simd_loop:
	SAVE_SIMD_REGS
	ldi	$1, NC_STORE_THRESHOLD($31)
	cmple	$18, $1, $1
	bne	$1, $simd_loop

	.align 4
$simd_loop_nc:
	FIXUP_LDST( vldd $f1, 0($17) )
	FIXUP_LDST( vldd $f2, 32($17) )
	FIXUP_LDST( vstd_nc $f1, 0($16) )
	FIXUP_LDST( vstd_nc $f2, 32($16) )
	subl	$18, 64, $18
	addl	$17, 64, $17
	addl	$16, 64, $16
	cmplt	$18, 64, $1
	beq	$1, $simd_loop_nc
	memb			# required for _nc store instructions
	br	$31, $simd_loop_end

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
	ble	$18, $out
	cmplt	$18, 16, $1
	bne	$1, $quad_loop_end
	bne	$4, $prep_quad_u_loop_tail

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
	ble	$18, $out
	cmplt	$18, 8, $1
	bne	$1, $byte_loop_tail
	bne	$4, $move_one_quad_u

$move_one_quad:
	FIXUP_LDST( ldl $2, 0($17) )
	FIXUP_LDST( stl $2, 0($16) )
	subl	$18, 8, $18
	addl	$17, 8, $17
	addl	$16, 8, $16
	ble	$18, $out

	.align 3
$byte_loop_tail:
	FIXUP_LDST( ldbu $2, 0($17) )
	FIXUP_LDST( stb $2, 0($16) )
	subl	$18, 1, $18
	addl	$17, 1, $17
	addl	$16, 1, $16
	bgt	$18, $byte_loop_tail
	br	$31, $out

/* misaligned src and dst */
$quad_u_loop_head:
	FIXUP_LDST( ldl_u $2, 0($17) )
	FIXUP_LDST( ldl_u $3, 7($17) )
	extll	$2, $4, $2
	exthl	$3, $4, $3
	bis	$2, $3, $2
	FIXUP_LDST( stl $2, 0($16) )
	addl	$16, 8, $16
	addl	$17, 8, $17
	subl	$18, 8, $18
	and	$16, 31, $1
	beq	$1, $dest_aligned_32
	br	$31, $quad_u_loop_head

$prep_simd_u_loop:
	SAVE_SIMD_U_REGS
	andnot	$17, 31, $3
	ldi	$2, 256($31)
	sll	$5, 3, $1
	subl	$2, $1, $2
	sll	$1, 29, $1
	sll	$2, 29, $2
	ifmovd	$1, $f1
	ifmovd	$2, $f2
	FIXUP_LDST( vldd $f4, 0($3) )
	ldi	$1, NC_STORE_THRESHOLD($31)
	cmple	$18, $1, $1
	bne	$1, $simd_u_loop

	.align 4
$simd_u_loop_nc:
	FIXUP_LDST( vldd $f5, 32($3) )
	srlow	$f4, $f1, $f4
	sllow	$f5, $f2, $f3
	vlogfc	$f3, $f4, $f31, $f3
	FIXUP_LDST( vstd_nc $f3, 0($16) )
	FIXUP_LDST( vldd $f4, 64($3) )
	srlow	$f5, $f1, $f5
	sllow	$f4, $f2, $f3
	vlogfc	$f5, $f3, $f31, $f5
	FIXUP_LDST( vstd_nc $f5, 32($16) )
	subl	$18, 64, $18
	addl	$3, 64, $3
	addl	$16, 64, $16
	cmplt	$18, 64, $1
	beq	$1, $simd_u_loop_nc
	memb			# required for _nc store instructions
	br	$31, $simd_u_loop_end

	.align 4
$simd_u_loop:
	FIXUP_LDST( vldd $f5, 32($3) )
	srlow	$f4, $f1, $f4
	sllow	$f5, $f2, $f3
	vlogfc	$f4, $f3, $f31, $f3
	FIXUP_LDST( vstd $f3, 0($16) )
	FIXUP_LDST( vldd $f4, 64($3) )
	srlow	$f5, $f1, $f5
	sllow	$f4, $f2, $f3
	vlogfc	$f5, $f3, $f31, $f3
	FIXUP_LDST( vstd $f3, 32($16) )
	subl	$18, 64, $18
	addl	$3, 64, $3
	addl	$16, 64, $16
	cmplt	$18, 64, $1
	beq	$1, $simd_u_loop

$simd_u_loop_end:
	cmplt	$18, 32, $1
	bne	$1, $no_more_simd_u
	FIXUP_LDST( vldd $f5, 32($3) )
	srlow	$f4, $f1, $f4
	sllow	$f5, $f2, $f3
	vlogfc	$f4, $f3, $f31, $f3
	FIXUP_LDST( vstd $f3, 0($16) )
	subl	$18, 32, $18
	addl	$3, 32, $3
	addl	$16, 32, $16

$no_more_simd_u:
	RESTORE_SIMD_U_REGS
	bis	$3, $5, $17
	br	$31, $simd_end

$prep_quad_u_loop_tail:
	FIXUP_LDST( ldl_u $2, 0($17) )
	.align 4
$quad_u_loop_tail:
	FIXUP_LDST( ldl_u $3, 8($17) )
	extll	$2, $4, $22
	exthl	$3, $4, $23
	bis	$22, $23, $22
	FIXUP_LDST( stl $22, 0($16) )
	FIXUP_LDST( ldl_u $2, 16($17) )
	extll	$3, $4, $24
	exthl	$2, $4, $25
	bis	$24, $25, $24
	FIXUP_LDST( stl $24, 8($16) )
	subl	$18, 16, $18
	addl	$17, 16, $17
	addl	$16, 16, $16
	cmplt	$18, 16, $1
	beq	$1, $quad_u_loop_tail
	br	$31, $quad_loop_end

$move_one_quad_u:
	FIXUP_LDST( ldl_u $2, 0($17) )
	FIXUP_LDST( ldl_u $3, 8($17) )
	extll	$2, $4, $22
	exthl	$3, $4, $23
	bis	$22, $23, $22
	FIXUP_LDST( stl $22, 0($16) )
	subl	$18, 8, $18
	addl	$17, 8, $17
	addl	$16, 8, $16
	ble	$18, $out
	br	$31, $byte_loop_tail
