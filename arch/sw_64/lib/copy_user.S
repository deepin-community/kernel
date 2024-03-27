/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copy to/from user space, handling exceptions as we go..  This
 * isn't exactly pretty.
 *
 * This is essentially the same as "memcpy()", but with a few twists.
 * Notably, we have to make sure that $0 is always up-to-date and
 * contains the right "bytes left to copy" value (and that it is updated
 * only _after_ a successful copy). There is also some rather minor
 * exception setup stuff..
 */
/* Allow an exception for an insn; exit if we get one.  */
#define EXI(x,y...)			\
	99: x,##y;			\
	.section __ex_table, "a";	\
	.long 99b - .;			\
	ldi	$31, $exitin-99b($31);	\
	.previous

#define EXO(x,y...)			\
	99:	x, ##y;			\
	.section __ex_table, "a";	\
	.long 99b - .;			\
	ldi	$31, $exitout-99b($31);	\
	.previous

	.set noat
	.align 4
	.globl ____copy_user_sisd
	.ent ____copy_user_sisd
____copy_user_sisd:
	.prologue 0
	and	$18, $18, $0
	and	$16, 7, $3
	beq	$0, $35
	beq	$3, $36
	subl	$3, 8, $3
	.align 4
$37:
	EXI(ldbu $1, 0($17))
	EXO(stb $1, 0($16))
	addl	$3, 1, $3
	subl	$0, 1, $0
	addl	$16, 1, $16
	addl	$17, 1, $17
	beq	$0, $41
	bne	$3, $37
$36:
	and	$17, 7, $1
	bic	$0, 7, $4
	beq	$1, $43
	beq	$4, $48
	EXI(ldl_u $3, 0($17))
	.align 4
$50:
	EXI(ldl_u $2, 8($17))
	subl	$4, 8, $4
	extll	$3, $17, $3
	exthl	$2, $17, $1
	bis	$3, $1, $1
	EXO(stl $1,0($16))
	addl	$17, 8, $17
	subl	$0, 8, $0
	addl	$16, 8, $16
	bis	$2, $2, $3
	bne	$4, $50
$48:
	beq	$0, $41
	.align 4
$57:
	EXI(ldbu $1, 0($17))
	EXO(stb $1, 0($16))
	subl	$0, 1, $0
	addl	$16, 1, $16
	addl	$17, 1, $17
	bne	$0, $57
	br	$31, $41
	.align 4
$43:
	beq	$4, $65
	.align 4
$66:
	EXI(ldl $1, 0($17))
	subl	$4, 8, $4
	EXO(stl $1,0($16))
	addl	$17, 8, $17
	subl	$0, 8, $0
	addl	$16, 8, $16
	bne	$4, $66
$65:
	beq	$0, $41
	EXI(ldbu $1, 0($17))
	EXO(stb	$1, 0($16))
	addl	$17, 1, $17
	addl	$16, 1, $16
	subl	$0, 1, $0
	br	$31, $65
$41:
$35:
$exitin:
$exitout:
	ret	$31, ($26), 1

	.end ____copy_user_sisd
