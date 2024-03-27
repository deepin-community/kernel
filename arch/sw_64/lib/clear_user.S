/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Contributed by Richard Henderson <rth@tamu.edu>
 *
 * Zero user space, handling exceptions as we go.
 *
 * We have to make sure that $0 is always up-to-date and contains the
 * right "bytes left to zero" value (and that it is updated only _after_
 * a successful copy).  There is also some rather minor exception setup
 * stuff.
 *
 */
/* Allow an exception for an insn; exit if we get one.  */
#define EX(x,y...)			\
	99: x,##y;			\
	.section __ex_table,"a";	\
	.long 99b - .;			\
	ldi $31, $exception-99b($31);	\
	.previous

	.set noat
	.set noreorder
	.align 4

	.globl ____clear_user_sisd
	.ent ____clear_user_sisd
	.frame $30, 0, $26
	.prologue 0
____clear_user_sisd:
	and	$17, $17, $0
	and	$16, 7, $4
	beq	$0, $zerolength
	addl	$0, $4, $1
	and	$1, 7, $2
	srl	$1, 3, $1
	beq	$4, $loop

	subl	$4, 8, $4
	addl	$0, $4, $0
	beq	$1, $oneword

$head:
	EX(stb $31, 0($16))
	addl	$16, 1, $16
	addl	$4, 1, $4
	bne	$4, $head
	subl	$1, 1, $1
	br	$loop
	unop

$oneword:
	EX(stb $31, 0($16))
	addl	$16, 1, $16
	addl	$4, 1, $4
	bne	$4, $oneword
	clr	$0

$zerolength:
$exception:
	ret	$31, ($26), 1

$loop:
	and	$1, 3, $4
	beq	$4, 1f

0:	EX(stl $31, 0($16))
	subl	$0, 8, $0
	subl	$4, 1, $4
	addl	$16, 8, $16
	bne	$4, 0b
	unop

1:	bic	$1, 3, $1
	beq	$1, $tail

2:	EX(stl $31, 0($16))
	subl	$0, 8, $0
	EX(stl $31, 8($16))
	subl	$0, 8, $0
	EX(stl $31, 16($16))
	subl	$0, 8, $0
	EX(stl $31, 24($16))
	subl	$0, 8, $0
	subl	$1, 4, $1
	addl	$16, 32, $16
	bne	$1, 2b

$tail:
	bne	$2, 1f
	ret	$31, ($26), 1

1:
	EX(stb $31, 0($16))
	addl	$16, 1, $16
	subl	$2, 1, $2
	bne	$2, 1b
	clr	$0
	ret	$31, ($26), 1

	.end ____clear_user_sisd
