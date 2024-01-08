/* SPDX-License-Identifier: GPL-2.0 */
/*
 * (C) 1995 Linus Torvalds
 *
 * The sw64 chip doesn't provide hardware division, so we have to do it
 * by hand.  The compiler expects the functions
 *
 *	__divlu: 64-bit unsigned long divide
 *	__remlu: 64-bit unsigned long remainder
 *	__divls/__remqs: signed 64-bit
 *	__divwu/__remlu: unsigned 32-bit
 *	__divws/__remls: signed 32-bit
 *
 * These are not normal C functions: instead of the normal
 * calling sequence, these expect their arguments in registers
 * $24 and $25, and return the result in $27. Register $28 may
 * be clobbered (assembly temporary), anything else must be saved.
 *
 * In short: painful.
 *
 * This is a rather simple bit-at-a-time algorithm: it's very good
 * at dividing random 64-bit numbers, but the more usual case where
 * the divisor is small is handled better by the DEC algorithm
 * using lookup tables. This uses much less memory, though, and is
 * nicer on the cache.. Besides, I don't know the copyright status
 * of the DEC code.
 */

/*
 * My temporaries:
 *	$0 - current bit
 *	$1 - shifted divisor
 *	$2 - modulus/quotient
 *
 *	$23 - return address
 *	$24 - dividend
 *	$25 - divisor
 *
 *	$27 - quotient/modulus
 *	$28 - compare status
 */
#include <asm/export.h>

#define halt .long 0

/*
 * Select function type and registers
 */
#define mask	$0
#define divisor	$1
#define compare	$28
#define tmp1	$3
#define tmp2	$4

#ifdef DIV
#define DIV_ONLY(x,y...)	x, ##y
#define MOD_ONLY(x,y...)
#define func(x)			__div##x
#define modulus			$2
#define quotient		$27
#define GETSIGN(x)		xor $24, $25, x
#define STACK			48
#else
#define DIV_ONLY(x,y...)
#define MOD_ONLY(x,y...)	x, ##y
#define func(x)			__rem##x
#define modulus			$27
#define quotient		$2
#define GETSIGN(x)		bis $24, $24, x
#define STACK			32
#endif

/*
 * For 32-bit operations, we need to extend to 64-bit
 */
#ifdef INTSIZE
#define ufunction		func(wu)
#define sfunction		func(w)
#define LONGIFY(x)		zapnot x, 15, x
#define SLONGIFY(x)		addw x, 0, x
#else
#define ufunction		func(lu)
#define sfunction		func(l)
#define LONGIFY(x)
#define SLONGIFY(x)
#endif

.set noat
.align 3
.globl ufunction
.ent ufunction
ufunction:
	subl	$30, STACK, $30
	.frame $30, STACK, $23
	.prologue 0

7:	stl	$1, 0($30)
	bis	$25, $25, divisor
	stl	$2, 8($30)
	bis	$24, $24, modulus
	stl	$0, 16($30)
	bis	$31, $31, quotient
	LONGIFY(divisor)
	stl	tmp1, 24($30)
	LONGIFY(modulus)
	bis	$31, 1, mask
	DIV_ONLY(stl tmp2, 32($30))
	beq	divisor, 9f		# div by zero

#ifdef INTSIZE
	/*
	 * shift divisor left, using 3-bit shifts for
	 * 32-bit divides as we can't overflow. Three-bit
	 * shifts will result in looping three times less
	 * here, but can result in two loops more later.
	 * Thus using a large shift isn't worth it (and
	 * s8add pairs better than a sll..)
	 */
1:	cmpult	divisor, modulus, compare
	s8addl	divisor, $31, divisor
	s8addl	mask, $31, mask
	bne	compare, 1b
#else
1:	cmpult	divisor, modulus, compare
	blt	divisor, 2f
	addl	divisor, divisor, divisor
	addl	mask, mask, mask
	bne	compare, 1b
#endif

	/* ok, start to go right again.. */
2:	DIV_ONLY(addl quotient, mask, tmp2)
	srl	mask, 1, mask
	cmpule	divisor, modulus, compare
	subl	modulus, divisor, tmp1
	DIV_ONLY(selne compare, tmp2, quotient, quotient)
	srl	divisor, 1, divisor
	selne	compare, tmp1, modulus, modulus
	bne	mask, 2b

9:	ldl	$1, 0($30)
	ldl	$2, 8($30)
	ldl	$0, 16($30)
	ldl	tmp1, 24($30)
	DIV_ONLY(ldl tmp2, 32($30))
	addl	$30, STACK, $30
	ret	$31, ($23), 1
	.end	ufunction
	EXPORT_SYMBOL(ufunction)
/*
 * Uhh.. Ugly signed division. I'd rather not have it at all, but
 * it's needed in some circumstances. There are different ways to
 * handle this, really. This does:
 *	-a / b = a / -b = -(a / b)
 *	-a % b = -(a % b)
 *	a % -b = a % b
 * which is probably not the best solution, but at least should
 * have the property that (x/y)*y + (x%y) = x.
 */
.align 3
.globl sfunction
.ent sfunction
sfunction:
	subl	$30, STACK, $30
	.frame $30, STACK, $23
	.prologue 0
	bis	$24, $25, $28
	SLONGIFY($28)
	bge	$28, 7b
	stl	$24, 0($30)
	subl	$31, $24, $28
	stl	$25, 8($30)
	sellt	$24, $28, $24, $24	# abs($24)
	stl	$23, 16($30)
	subl	$31, $25, $28
	stl	tmp1, 24($30)
	sellt	$25, $28, $25, $25	# abs($25)
	bsr	$23, ufunction
	ldl	$24, 0($30)
	ldl	$25, 8($30)
	GETSIGN($28)
	subl	$31, $27, tmp1
	SLONGIFY($28)
	ldl	$23, 16($30)
	sellt	$28, tmp1, $27, $27
	ldl	tmp1, 24($30)
	addl	$30, STACK, $30
	ret	$31, ($23), 1
	.end	sfunction
	EXPORT_SYMBOL(sfunction)
