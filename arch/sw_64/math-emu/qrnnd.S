/* SPDX-License-Identifier: GPL-2.0 */
 #  __udiv_qrnnd
 # Copyright (C) 1992, 1994, 1995, 2000 Free Software Foundation, Inc.

 # This file is part of GCC.

	.set noreorder
	.set noat

	.text

	.globl __udiv_qrnnd
	.ent __udiv_qrnnd
__udiv_qrnnd:
	.frame		$30, 0, $26, 0
	.prologue	0

	# ldiq	$2,16
	ldi		$2, 16($31)
	blt		$19, $largedivisor

$loop1:	cmplt		$18, 0, $3
	addl		$17, $17, $17
	bis		$17, $3, $17
	addl		$18, $18, $18
	cmpule		$19, $17, $20
	subl		$17, $19, $3
	selne		$20, $3, $17, $17
	bis		$18, $20, $18
	cmplt		$18, 0, $3
	addl		$17, $17, $17
	bis		$17, $3, $17
	addl		$18, $18, $18
	cmpule		$19, $17, $20
	subl		$17, $19, $3
	selne		$20, $3, $17, $17
	bis		$18, $20, $18
	cmplt		$18, 0, $3
	addl		$17, $17, $17
	bis		$17, $3, $17
	addl		$18, $18, $18
	cmpule		$19, $17, $20
	subl		$17, $19, $3
	selne		$20, $3, $17, $17
	bis		$18, $20, $18
	cmplt		$18, 0, $3
	addl		$17, $17, $17
	bis		$17, $3, $17
	addl		$18, $18, $18
	cmpule		$19, $17, $20
	subl		$17, $19, $3
	selne		$20, $3, $17, $17
	bis		$18, $20, $18
	subl		$2, 1, $2
	bgt		$2, $loop1
	stl		$17, 0($16)
	bis		$31, $18, $0
	ret		$31, ($26), 1

$largedivisor:
	and		$18, 1, $4

	srl		$18, 1, $18
	sll		$17, 63, $3
	or		$3, $18, $18
	srl		$17, 1, $17

	and		$19, 1, $6
	srl		$19, 1, $5
	addl		$5, $6, $5

$loop2:	cmplt		$18, 0, $3
	addl		$17, $17, $17
	bis		$17, $3, $17
	addl		$18, $18, $18
	cmpule		$5, $17, $20
	subl		$17, $5, $3
	selne		$20, $3, $17, $17
	bis		$18, $20, $18
	cmplt		$18, 0, $3
	addl		$17, $17, $17
	bis		$17, $3, $17
	addl		$18, $18, $18
	cmpule		$5, $17, $20
	subl		$17, $5, $3
	selne		$20, $3, $17, $17
	bis		$18, $20, $18
	cmplt		$18, 0, $3
	addl		$17, $17, $17
	bis		$17, $3, $17
	addl		$18, $18, $18
	cmpule		$5, $17, $20
	subl		$17, $5, $3
	selne		$20, $3, $17, $17
	bis		$18, $20, $18
	cmplt		$18, 0, $3
	addl		$17, $17, $17
	bis		$17, $3, $17
	addl		$18, $18, $18
	cmpule		$5, $17, $20
	subl		$17, $5, $3
	selne		$20, $3, $17, $17
	bis		$18, $20, $18
	subl		$2, 1, $2
	bgt		$2, $loop2

	addl		$17, $17, $17
	addl		$4, $17, $17
	bne		$6, $Odd
	stl		$17, 0($16)
	bis		$31, $18, $0
	ret		$31, ($26), 1

$Odd:
	# q' in $18. r' in $17
	addl		$17, $18, $17

	cmpult		$17, $18, $3	# $3 := carry from addl
	subl		$17, $19, $at
	addl		$18, $3, $18
	selne		$3, $at, $17, $17

	cmpult		$17, $19, $3
	addl		$18, 1, $at
	seleq		$3, $at, $18, $18
	subl		$17, $19, $at
	seleq		$3, $at, $17, $17

	stl		$17, 0($16)
	bis		$31, $18, $0
	ret		$31, ($26), 1

	.end	__udiv_qrnnd
