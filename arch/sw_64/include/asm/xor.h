/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Optimized RAID-5 checksumming functions.
 */

#ifndef _ASM_SW64_XOR_H
#define _ASM_SW64_XOR_H

extern void xor_sw64_2(unsigned long bytes, unsigned long *__restrict p1,
		const unsigned long *__restrict p2);
extern void xor_sw64_3(unsigned long bytes, unsigned long *__restrict p1,
		const unsigned long *__restrict p2,
		const unsigned long *__restrict p3);
extern void xor_sw64_4(unsigned long bytes, unsigned long *__restrict p1,
		const unsigned long *__restrict p2,
		const unsigned long *__restrict p3,
		const unsigned long *__restrict p4);
extern void xor_sw64_5(unsigned long bytes, unsigned long *__restrict p1,
		const unsigned long *__restrict p2,
		const unsigned long *__restrict p3,
		const unsigned long *__restrict p4,
		const unsigned long *__restrict p5);

extern void xor_sw64_prefetch_2(unsigned long bytes, unsigned long *__restrict p1,
		const unsigned long *__restrict p2);
extern void xor_sw64_prefetch_3(unsigned long bytes, unsigned long *__restrict p1,
		const unsigned long *__restrict p2,
		const unsigned long *__restrict p3);
extern void xor_sw64_prefetch_4(unsigned long bytes, unsigned long *__restrict p1,
		const unsigned long *__restrict p2,
		const unsigned long *__restrict p3,
		const unsigned long *__restrict p4);
extern void xor_sw64_prefetch_5(unsigned long bytes, unsigned long *__restrict p1,
		const unsigned long *__restrict p2,
		const unsigned long *__restrict p3,
		const unsigned long *__restrict p4,
		const unsigned long *__restrict p5);

asm("								\n\
	.text							\n\
	.align 3						\n\
	.ent xor_sw64_2						\n\
xor_sw64_2:							\n\
	.prologue 0						\n\
	srl $16, 6, $16						\n\
	.align 4						\n\
2:								\n\
	ldl $0, 0($17)						\n\
	ldl $1, 0($18)						\n\
	ldl $2, 8($17)						\n\
	ldl $3, 8($18)						\n\
								\n\
	ldl $4, 16($17)						\n\
	ldl $5, 16($18)						\n\
	ldl $6, 24($17)						\n\
	ldl $7, 24($18)						\n\
								\n\
	ldl $19, 32($17)					\n\
	ldl $20, 32($18)					\n\
	ldl $21, 40($17)					\n\
	ldl $22, 40($18)					\n\
								\n\
	ldl $23, 48($17)					\n\
	ldl $24, 48($18)					\n\
	ldl $25, 56($17)					\n\
	xor $0, $1, $0		# 7 cycles from $1 load		\n\
								\n\
	ldl $27, 56($18)					\n\
	xor $2, $3, $2						\n\
	stl $0, 0($17)						\n\
	xor $4, $5, $4						\n\
								\n\
	stl $2, 8($17)						\n\
	xor $6, $7, $6						\n\
	stl $4, 16($17)						\n\
	xor $19, $20, $19					\n\
								\n\
	stl $6, 24($17)						\n\
	xor $21, $22, $21					\n\
	stl $19, 32($17)					\n\
	xor $23, $24, $23					\n\
								\n\
	stl $21, 40($17)					\n\
	xor $25, $27, $25					\n\
	stl $23, 48($17)					\n\
	subl $16, 1, $16					\n\
								\n\
	stl $25, 56($17)					\n\
	addl $17, 64, $17					\n\
	addl $18, 64, $18					\n\
	bgt $16, 2b						\n\
								\n\
	ret							\n\
	.end xor_sw64_2						\n\
								\n\
	.align 3						\n\
	.ent xor_sw64_3						\n\
xor_sw64_3:							\n\
	.prologue 0						\n\
	srl $16, 6, $16						\n\
	.align 4						\n\
3:								\n\
	ldl $0, 0($17)						\n\
	ldl $1, 0($18)						\n\
	ldl $2, 0($19)						\n\
	ldl $3, 8($17)						\n\
								\n\
	ldl $4, 8($18)						\n\
	ldl $6, 16($17)						\n\
	ldl $7, 16($18)						\n\
	ldl $21, 24($17)					\n\
								\n\
	ldl $22, 24($18)					\n\
	ldl $24, 32($17)					\n\
	ldl $25, 32($18)					\n\
	ldl $5, 8($19)						\n\
								\n\
	ldl $20, 16($19)					\n\
	ldl $23, 24($19)					\n\
	ldl $27, 32($19)					\n\
								\n\
	xor $0, $1, $1		# 8 cycles from $0 load		\n\
	xor $3, $4, $4		# 6 cycles from $4 load		\n\
	xor $6, $7, $7		# 6 cycles from $7 load		\n\
	xor $21, $22, $22	# 5 cycles from $22 load	\n\
								\n\
	xor $1, $2, $2		# 9 cycles from $2 load		\n\
	xor $24, $25, $25	# 5 cycles from $25 load	\n\
	stl $2, 0($17)						\n\
	xor $4, $5, $5		# 6 cycles from $5 load		\n\
								\n\
	stl $5, 8($17)						\n\
	xor $7, $20, $20	# 7 cycles from $20 load	\n\
	stl $20, 16($17)					\n\
	xor $22, $23, $23	# 7 cycles from $23 load	\n\
								\n\
	stl $23, 24($17)					\n\
	xor $25, $27, $27	# 7 cycles from $27 load	\n\
	stl $27, 32($17)					\n\
								\n\
	ldl $0, 40($17)						\n\
	ldl $1, 40($18)						\n\
	ldl $3, 48($17)						\n\
	ldl $4, 48($18)						\n\
								\n\
	ldl $6, 56($17)						\n\
	ldl $7, 56($18)						\n\
	ldl $2, 40($19)						\n\
	ldl $5, 48($19)						\n\
								\n\
	ldl $20, 56($19)					\n\
	xor $0, $1, $1		# 4 cycles from $1 load		\n\
	xor $3, $4, $4		# 5 cycles from $4 load		\n\
	xor $6, $7, $7		# 5 cycles from $7 load		\n\
								\n\
	xor $1, $2, $2		# 4 cycles from $2 load		\n\
	xor $4, $5, $5		# 5 cycles from $5 load		\n\
	stl $2, 40($17)						\n\
	xor $7, $20, $20	# 4 cycles from $20 load	\n\
								\n\
	stl $5, 48($17)						\n\
	subl $16, 1, $16					\n\
	stl $20, 56($17)					\n\
	addl $19, 64, $19					\n\
								\n\
	addl $18, 64, $18					\n\
	addl $17, 64, $17					\n\
	bgt $16, 3b						\n\
	ret							\n\
	.end xor_sw64_3						\n\
								\n\
	.align 3						\n\
	.ent xor_sw64_4						\n\
xor_sw64_4:							\n\
	.prologue 0						\n\
	srl $16, 6, $16						\n\
	.align 4						\n\
4:								\n\
	ldl $0, 0($17)						\n\
	ldl $1, 0($18)						\n\
	ldl $2, 0($19)						\n\
	ldl $3, 0($20)						\n\
								\n\
	ldl $4, 8($17)						\n\
	ldl $5, 8($18)						\n\
	ldl $6, 8($19)						\n\
	ldl $7, 8($20)						\n\
								\n\
	ldl $21, 16($17)					\n\
	ldl $22, 16($18)					\n\
	ldl $23, 16($19)					\n\
	ldl $24, 16($20)					\n\
								\n\
	ldl $25, 24($17)					\n\
	xor $0, $1, $1		# 6 cycles from $1 load		\n\
	ldl $27, 24($18)					\n\
	xor $2, $3, $3		# 6 cycles from $3 load		\n\
								\n\
	ldl $0, 24($19)						\n\
	xor $1, $3, $3						\n\
	ldl $1, 24($20)						\n\
	xor $4, $5, $5		# 7 cycles from $5 load		\n\
								\n\
	stl $3, 0($17)						\n\
	xor $6, $7, $7						\n\
	xor $21, $22, $22	# 7 cycles from $22 load	\n\
	xor $5, $7, $7						\n\
								\n\
	stl $7, 8($17)						\n\
	xor $23, $24, $24	# 7 cycles from $24 load	\n\
	ldl $2, 32($17)						\n\
	xor $22, $24, $24					\n\
								\n\
	ldl $3, 32($18)						\n\
	ldl $4, 32($19)						\n\
	ldl $5, 32($20)						\n\
	xor $25, $27, $27	# 8 cycles from $27 load	\n\
								\n\
	ldl $6, 40($17)						\n\
	ldl $7, 40($18)						\n\
	ldl $21, 40($19)					\n\
	ldl $22, 40($20)					\n\
								\n\
	stl $24, 16($17)					\n\
	xor $0, $1, $1		# 9 cycles from $1 load		\n\
	xor $2, $3, $3		# 5 cycles from $3 load		\n\
	xor $27, $1, $1						\n\
								\n\
	stl $1, 24($17)						\n\
	xor $4, $5, $5		# 5 cycles from $5 load		\n\
	ldl $23, 48($17)					\n\
	ldl $24, 48($18)					\n\
								\n\
	ldl $25, 48($19)					\n\
	xor $3, $5, $5						\n\
	ldl $27, 48($20)					\n\
	ldl $0, 56($17)						\n\
								\n\
	ldl $1, 56($18)						\n\
	ldl $2, 56($19)						\n\
	xor $6, $7, $7		# 8 cycles from $6 load		\n\
	ldl $3, 56($20)						\n\
								\n\
	stl $5, 32($17)						\n\
	xor $21, $22, $22	# 8 cycles from $22 load	\n\
	xor $7, $22, $22					\n\
	xor $23, $24, $24	# 5 cycles from $24 load	\n\
								\n\
	stl $22, 40($17)					\n\
	xor $25, $27, $27	# 5 cycles from $27 load	\n\
	xor $24, $27, $27					\n\
	xor $0, $1, $1		# 5 cycles from $1 load		\n\
								\n\
	stl $27, 48($17)					\n\
	xor $2, $3, $3		# 4 cycles from $3 load		\n\
	xor $1, $3, $3						\n\
	subl $16, 1, $16					\n\
								\n\
	stl $3, 56($17)						\n\
	addl $20, 64, $20					\n\
	addl $19, 64, $19					\n\
	addl $18, 64, $18					\n\
								\n\
	addl $17, 64, $17					\n\
	bgt $16, 4b						\n\
	ret							\n\
	.end xor_sw64_4						\n\
								\n\
	.align 3						\n\
	.ent xor_sw64_5						\n\
xor_sw64_5:							\n\
	.prologue 0						\n\
	srl $16, 6, $16						\n\
	.align 4						\n\
5:								\n\
	ldl $0, 0($17)						\n\
	ldl $1, 0($18)						\n\
	ldl $2, 0($19)						\n\
	ldl $3, 0($20)						\n\
								\n\
	ldl $4, 0($21)						\n\
	ldl $5, 8($17)						\n\
	ldl $6, 8($18)						\n\
	ldl $7, 8($19)						\n\
								\n\
	ldl $22, 8($20)						\n\
	ldl $23, 8($21)						\n\
	ldl $24, 16($17)					\n\
	ldl $25, 16($18)					\n\
								\n\
	ldl $27, 16($19)					\n\
	xor $0, $1, $1		# 6 cycles from $1 load		\n\
	ldl $28, 16($20)					\n\
	xor $2, $3, $3		# 6 cycles from $3 load		\n\
								\n\
	ldl $0, 16($21)						\n\
	xor $1, $3, $3						\n\
	ldl $1, 24($17)						\n\
	xor $3, $4, $4		# 7 cycles from $4 load		\n\
								\n\
	stl $4, 0($17)						\n\
	xor $5, $6, $6		# 7 cycles from $6 load		\n\
	xor $7, $22, $22	# 7 cycles from $22 load	\n\
	xor $6, $23, $23	# 7 cycles from $23 load	\n\
								\n\
	ldl $2, 24($18)						\n\
	xor $22, $23, $23					\n\
	ldl $3, 24($19)						\n\
	xor $24, $25, $25	# 8 cycles from $25 load	\n\
								\n\
	stl $23, 8($17)						\n\
	xor $25, $27, $27	# 8 cycles from $27 load	\n\
	ldl $4, 24($20)						\n\
	xor $28, $0, $0		# 7 cycles from $0 load		\n\
								\n\
	ldl $5, 24($21)						\n\
	xor $27, $0, $0						\n\
	ldl $6, 32($17)						\n\
	ldl $7, 32($18)						\n\
								\n\
	stl $0, 16($17)						\n\
	xor $1, $2, $2		# 6 cycles from $2 load		\n\
	ldl $22, 32($19)					\n\
	xor $3, $4, $4		# 4 cycles from $4 load		\n\
								\n\
	ldl $23, 32($20)					\n\
	xor $2, $4, $4						\n\
	ldl $24, 32($21)					\n\
	ldl $25, 40($17)					\n\
								\n\
	ldl $27, 40($18)					\n\
	ldl $28, 40($19)					\n\
	ldl $0, 40($20)						\n\
	xor $4, $5, $5		# 7 cycles from $5 load		\n\
								\n\
	stl $5, 24($17)						\n\
	xor $6, $7, $7		# 7 cycles from $7 load		\n\
	ldl $1, 40($21)						\n\
	ldl $2, 48($17)						\n\
								\n\
	ldl $3, 48($18)						\n\
	xor $7, $22, $22	# 7 cycles from $22 load	\n\
	ldl $4, 48($19)						\n\
	xor $23, $24, $24	# 6 cycles from $24 load	\n\
								\n\
	ldl $5, 48($20)						\n\
	xor $22, $24, $24					\n\
	ldl $6, 48($21)						\n\
	xor $25, $27, $27	# 7 cycles from $27 load	\n\
								\n\
	stl $24, 32($17)					\n\
	xor $27, $28, $28	# 8 cycles from $28 load	\n\
	ldl $7, 56($17)						\n\
	xor $0, $1, $1		# 6 cycles from $1 load		\n\
								\n\
	ldl $22, 56($18)					\n\
	ldl $23, 56($19)					\n\
	ldl $24, 56($20)					\n\
	ldl $25, 56($21)					\n\
								\n\
	xor $28, $1, $1						\n\
	xor $2, $3, $3		# 9 cycles from $3 load		\n\
	xor $3, $4, $4		# 9 cycles from $4 load		\n\
	xor $5, $6, $6		# 8 cycles from $6 load		\n\
								\n\
	stl $1, 40($17)						\n\
	xor $4, $6, $6						\n\
	xor $7, $22, $22	# 7 cycles from $22 load	\n\
	xor $23, $24, $24	# 6 cycles from $24 load	\n\
								\n\
	stl $6, 48($17)						\n\
	xor $22, $24, $24					\n\
	subl $16, 1, $16					\n\
	xor $24, $25, $25	# 8 cycles from $25 load	\n\
								\n\
	stl $25, 56($17)					\n\
	addl $21, 64, $21					\n\
	addl $20, 64, $20					\n\
	addl $19, 64, $19					\n\
								\n\
	addl $18, 64, $18					\n\
	addl $17, 64, $17					\n\
	bgt $16, 5b						\n\
	ret							\n\
	.end xor_sw64_5						\n\
								\n\
	.align 3						\n\
	.ent xor_sw64_prefetch_2					\n\
xor_sw64_prefetch_2:						\n\
	.prologue 0						\n\
	srl $16, 6, $16						\n\
								\n\
	fillde 0($17)						\n\
	fillde 0($18)						\n\
								\n\
	fillde 64($17)						\n\
	fillde 64($18)						\n\
								\n\
	fillde 128($17)						\n\
	fillde 128($18)						\n\
								\n\
	fillde 192($17)						\n\
	fillde 192($18)						\n\
	.align 4						\n\
2:								\n\
	ldl $0, 0($17)						\n\
	ldl $1, 0($18)						\n\
	ldl $2, 8($17)						\n\
	ldl $3, 8($18)						\n\
								\n\
	ldl $4, 16($17)						\n\
	ldl $5, 16($18)						\n\
	ldl $6, 24($17)						\n\
	ldl $7, 24($18)						\n\
								\n\
	ldl $19, 32($17)					\n\
	ldl $20, 32($18)					\n\
	ldl $21, 40($17)					\n\
	ldl $22, 40($18)					\n\
								\n\
	ldl $23, 48($17)					\n\
	ldl $24, 48($18)					\n\
	ldl $25, 56($17)					\n\
	ldl $27, 56($18)					\n\
								\n\
	fillde 256($17)						\n\
	xor $0, $1, $0		# 8 cycles from $1 load		\n\
	fillde 256($18)						\n\
	xor $2, $3, $2						\n\
								\n\
	stl $0, 0($17)						\n\
	xor $4, $5, $4						\n\
	stl $2, 8($17)						\n\
	xor $6, $7, $6						\n\
								\n\
	stl $4, 16($17)						\n\
	xor $19, $20, $19					\n\
	stl $6, 24($17)						\n\
	xor $21, $22, $21					\n\
								\n\
	stl $19, 32($17)					\n\
	xor $23, $24, $23					\n\
	stl $21, 40($17)					\n\
	xor $25, $27, $25					\n\
								\n\
	stl $23, 48($17)					\n\
	subl $16, 1, $16					\n\
	stl $25, 56($17)					\n\
	addl $17, 64, $17					\n\
								\n\
	addl $18, 64, $18					\n\
	bgt $16, 2b						\n\
	ret							\n\
	.end xor_sw64_prefetch_2					\n\
								\n\
	.align 3						\n\
	.ent xor_sw64_prefetch_3					\n\
xor_sw64_prefetch_3:						\n\
	.prologue 0						\n\
	srl $16, 6, $16						\n\
								\n\
	fillde 0($17)						\n\
	fillde 0($18)						\n\
	fillde 0($19)						\n\
								\n\
	fillde 64($17)						\n\
	fillde 64($18)						\n\
	fillde 64($19)						\n\
								\n\
	fillde 128($17)						\n\
	fillde 128($18)						\n\
	fillde 128($19)						\n\
								\n\
	fillde 192($17)						\n\
	fillde 192($18)						\n\
	fillde 192($19)						\n\
	.align 4						\n\
3:								\n\
	ldl $0, 0($17)						\n\
	ldl $1, 0($18)						\n\
	ldl $2, 0($19)						\n\
	ldl $3, 8($17)						\n\
								\n\
	ldl $4, 8($18)						\n\
	ldl $6, 16($17)						\n\
	ldl $7, 16($18)						\n\
	ldl $21, 24($17)					\n\
								\n\
	ldl $22, 24($18)					\n\
	ldl $24, 32($17)					\n\
	ldl $25, 32($18)					\n\
	ldl $5, 8($19)						\n\
								\n\
	ldl $20, 16($19)					\n\
	ldl $23, 24($19)					\n\
	ldl $27, 32($19)					\n\
								\n\
	xor $0, $1, $1		# 8 cycles from $0 load		\n\
	xor $3, $4, $4		# 7 cycles from $4 load		\n\
	xor $6, $7, $7		# 6 cycles from $7 load		\n\
	xor $21, $22, $22	# 5 cycles from $22 load	\n\
								\n\
	xor $1, $2, $2		# 9 cycles from $2 load		\n\
	xor $24, $25, $25	# 5 cycles from $25 load	\n\
	stl $2, 0($17)						\n\
	xor $4, $5, $5		# 6 cycles from $5 load		\n\
								\n\
	stl $5, 8($17)						\n\
	xor $7, $20, $20	# 7 cycles from $20 load	\n\
	stl $20, 16($17)					\n\
	xor $22, $23, $23	# 7 cycles from $23 load	\n\
								\n\
	stl $23, 24($17)					\n\
	xor $25, $27, $27	# 7 cycles from $27 load	\n\
	stl $27, 32($17)					\n\
								\n\
	ldl $0, 40($17)						\n\
	ldl $1, 40($18)						\n\
	ldl $3, 48($17)						\n\
	ldl $4, 48($18)						\n\
								\n\
	ldl $6, 56($17)						\n\
	ldl $7, 56($18)						\n\
	ldl $2, 40($19)						\n\
	ldl $5, 48($19)						\n\
								\n\
	ldl $20, 56($19)					\n\
	fillde 256($17)						\n\
	fillde 256($18)						\n\
	fillde 256($19)						\n\
								\n\
	xor $0, $1, $1		# 6 cycles from $1 load		\n\
	xor $3, $4, $4		# 5 cycles from $4 load		\n\
	xor $6, $7, $7		# 5 cycles from $7 load		\n\
	xor $1, $2, $2		# 4 cycles from $2 load		\n\
								\n\
	xor $4, $5, $5		# 5 cycles from $5 load		\n\
	xor $7, $20, $20	# 4 cycles from $20 load	\n\
	stl $2, 40($17)						\n\
	subl $16, 1, $16					\n\
								\n\
	stl $5, 48($17)						\n\
	addl $19, 64, $19					\n\
	stl $20, 56($17)					\n\
	addl $18, 64, $18					\n\
								\n\
	addl $17, 64, $17					\n\
	bgt $16, 3b						\n\
	ret							\n\
	.end xor_sw64_prefetch_3					\n\
								\n\
	.align 3						\n\
	.ent xor_sw64_prefetch_4					\n\
xor_sw64_prefetch_4:						\n\
	.prologue 0						\n\
	srl $16, 6, $16						\n\
								\n\
	fillde 0($17)						\n\
	fillde 0($18)						\n\
	fillde 0($19)						\n\
	fillde 0($20)						\n\
								\n\
	fillde 64($17)						\n\
	fillde 64($18)						\n\
	fillde 64($19)						\n\
	fillde 64($20)						\n\
								\n\
	fillde 128($17)						\n\
	fillde 128($18)						\n\
	fillde 128($19)						\n\
	fillde 128($20)						\n\
								\n\
	fillde 192($17)						\n\
	fillde 192($18)						\n\
	fillde 192($19)						\n\
	fillde 192($20)						\n\
	.align 4						\n\
4:								\n\
	ldl $0, 0($17)						\n\
	ldl $1, 0($18)						\n\
	ldl $2, 0($19)						\n\
	ldl $3, 0($20)						\n\
								\n\
	ldl $4, 8($17)						\n\
	ldl $5, 8($18)						\n\
	ldl $6, 8($19)						\n\
	ldl $7, 8($20)						\n\
								\n\
	ldl $21, 16($17)					\n\
	ldl $22, 16($18)					\n\
	ldl $23, 16($19)					\n\
	ldl $24, 16($20)					\n\
								\n\
	ldl $25, 24($17)					\n\
	xor $0, $1, $1		# 6 cycles from $1 load		\n\
	ldl $27, 24($18)					\n\
	xor $2, $3, $3		# 6 cycles from $3 load		\n\
								\n\
	ldl $0, 24($19)						\n\
	xor $1, $3, $3						\n\
	ldl $1, 24($20)						\n\
	xor $4, $5, $5		# 7 cycles from $5 load		\n\
								\n\
	stl $3, 0($17)						\n\
	xor $6, $7, $7						\n\
	xor $21, $22, $22	# 7 cycles from $22 load	\n\
	xor $5, $7, $7						\n\
								\n\
	stl $7, 8($17)						\n\
	xor $23, $24, $24	# 7 cycles from $24 load	\n\
	ldl $2, 32($17)						\n\
	xor $22, $24, $24					\n\
								\n\
	ldl $3, 32($18)						\n\
	ldl $4, 32($19)						\n\
	ldl $5, 32($20)						\n\
	xor $25, $27, $27	# 8 cycles from $27 load	\n\
								\n\
	ldl $6, 40($17)						\n\
	ldl $7, 40($18)						\n\
	ldl $21, 40($19)					\n\
	ldl $22, 40($20)					\n\
								\n\
	stl $24, 16($17)					\n\
	xor $0, $1, $1		# 9 cycles from $1 load		\n\
	xor $2, $3, $3		# 5 cycles from $3 load		\n\
	xor $27, $1, $1						\n\
								\n\
	stl $1, 24($17)						\n\
	xor $4, $5, $5		# 5 cycles from $5 load		\n\
	ldl $23, 48($17)					\n\
	xor $3, $5, $5						\n\
								\n\
	ldl $24, 48($18)					\n\
	ldl $25, 48($19)					\n\
	ldl $27, 48($20)					\n\
	ldl $0, 56($17)						\n\
								\n\
	ldl $1, 56($18)						\n\
	ldl $2, 56($19)						\n\
	ldl $3, 56($20)						\n\
	xor $6, $7, $7		# 8 cycles from $6 load		\n\
								\n\
	fillde 256($17)						\n\
	xor $21, $22, $22	# 8 cycles from $22 load	\n\
	fillde 256($18)						\n\
	xor $7, $22, $22					\n\
								\n\
	fillde 256($19)						\n\
	xor $23, $24, $24	# 6 cycles from $24 load	\n\
	fillde 256($20)						\n\
	xor $25, $27, $27	# 6 cycles from $27 load	\n\
								\n\
	stl $5, 32($17)						\n\
	xor $24, $27, $27					\n\
	xor $0, $1, $1		# 7 cycles from $1 load		\n\
	xor $2, $3, $3		# 6 cycles from $3 load		\n\
								\n\
	stl $22, 40($17)					\n\
	xor $1, $3, $3						\n\
	stl $27, 48($17)					\n\
	subl $16, 1, $16					\n\
								\n\
	stl $3, 56($17)						\n\
	addl $20, 64, $20					\n\
	addl $19, 64, $19					\n\
	addl $18, 64, $18					\n\
								\n\
	addl $17, 64, $17					\n\
	bgt $16, 4b						\n\
	ret							\n\
	.end xor_sw64_prefetch_4					\n\
								\n\
	.align 3						\n\
	.ent xor_sw64_prefetch_5					\n\
xor_sw64_prefetch_5:						\n\
	.prologue 0						\n\
	srl $16, 6, $16						\n\
								\n\
	fillde 0($17)						\n\
	fillde 0($18)						\n\
	fillde 0($19)						\n\
	fillde 0($20)						\n\
	fillde 0($21)						\n\
								\n\
	fillde 64($17)						\n\
	fillde 64($18)						\n\
	fillde 64($19)						\n\
	fillde 64($20)						\n\
	fillde 64($21)						\n\
								\n\
	fillde 128($17)						\n\
	fillde 128($18)						\n\
	fillde 128($19)						\n\
	fillde 128($20)						\n\
	fillde 128($21)						\n\
								\n\
	fillde 192($17)						\n\
	fillde 192($18)						\n\
	fillde 192($19)						\n\
	fillde 192($20)						\n\
	fillde 192($21)						\n\
	.align 4						\n\
5:								\n\
	ldl $0, 0($17)						\n\
	ldl $1, 0($18)						\n\
	ldl $2, 0($19)						\n\
	ldl $3, 0($20)						\n\
								\n\
	ldl $4, 0($21)						\n\
	ldl $5, 8($17)						\n\
	ldl $6, 8($18)						\n\
	ldl $7, 8($19)						\n\
								\n\
	ldl $22, 8($20)						\n\
	ldl $23, 8($21)						\n\
	ldl $24, 16($17)					\n\
	ldl $25, 16($18)					\n\
								\n\
	ldl $27, 16($19)					\n\
	xor $0, $1, $1		# 6 cycles from $1 load		\n\
	ldl $28, 16($20)					\n\
	xor $2, $3, $3		# 6 cycles from $3 load		\n\
								\n\
	ldl $0, 16($21)						\n\
	xor $1, $3, $3						\n\
	ldl $1, 24($17)						\n\
	xor $3, $4, $4		# 7 cycles from $4 load		\n\
								\n\
	stl $4, 0($17)						\n\
	xor $5, $6, $6		# 7 cycles from $6 load		\n\
	xor $7, $22, $22	# 7 cycles from $22 load	\n\
	xor $6, $23, $23	# 7 cycles from $23 load	\n\
								\n\
	ldl $2, 24($18)						\n\
	xor $22, $23, $23					\n\
	ldl $3, 24($19)						\n\
	xor $24, $25, $25	# 8 cycles from $25 load	\n\
								\n\
	stl $23, 8($17)						\n\
	xor $25, $27, $27	# 8 cycles from $27 load	\n\
	ldl $4, 24($20)						\n\
	xor $28, $0, $0		# 7 cycles from $0 load		\n\
								\n\
	ldl $5, 24($21)						\n\
	xor $27, $0, $0						\n\
	ldl $6, 32($17)						\n\
	ldl $7, 32($18)						\n\
								\n\
	stl $0, 16($17)						\n\
	xor $1, $2, $2		# 6 cycles from $2 load		\n\
	ldl $22, 32($19)					\n\
	xor $3, $4, $4		# 4 cycles from $4 load		\n\
								\n\
	ldl $23, 32($20)					\n\
	xor $2, $4, $4						\n\
	ldl $24, 32($21)					\n\
	ldl $25, 40($17)					\n\
								\n\
	ldl $27, 40($18)					\n\
	ldl $28, 40($19)					\n\
	ldl $0, 40($20)						\n\
	xor $4, $5, $5		# 7 cycles from $5 load		\n\
								\n\
	stl $5, 24($17)						\n\
	xor $6, $7, $7		# 7 cycles from $7 load		\n\
	ldl $1, 40($21)						\n\
	ldl $2, 48($17)						\n\
								\n\
	ldl $3, 48($18)						\n\
	xor $7, $22, $22	# 7 cycles from $22 load	\n\
	ldl $4, 48($19)						\n\
	xor $23, $24, $24	# 6 cycles from $24 load	\n\
								\n\
	ldl $5, 48($20)						\n\
	xor $22, $24, $24					\n\
	ldl $6, 48($21)						\n\
	xor $25, $27, $27	# 7 cycles from $27 load	\n\
								\n\
	stl $24, 32($17)					\n\
	xor $27, $28, $28	# 8 cycles from $28 load	\n\
	ldl $7, 56($17)						\n\
	xor $0, $1, $1		# 6 cycles from $1 load		\n\
								\n\
	ldl $22, 56($18)					\n\
	ldl $23, 56($19)					\n\
	ldl $24, 56($20)					\n\
	ldl $25, 56($21)					\n\
								\n\
	fillde 256($17)						\n\
	xor $28, $1, $1						\n\
	fillde 256($18)						\n\
	xor $2, $3, $3		# 9 cycles from $3 load		\n\
								\n\
	fillde 256($19)						\n\
	xor $3, $4, $4		# 9 cycles from $4 load		\n\
	fillde 256($20)						\n\
	xor $5, $6, $6		# 8 cycles from $6 load		\n\
								\n\
	stl $1, 40($17)						\n\
	xor $4, $6, $6						\n\
	xor $7, $22, $22	# 7 cycles from $22 load	\n\
	xor $23, $24, $24	# 6 cycles from $24 load	\n\
								\n\
	stl $6, 48($17)						\n\
	xor $22, $24, $24					\n\
	fillde 256($21)						\n\
	xor $24, $25, $25	# 8 cycles from $25 load	\n\
								\n\
	stl $25, 56($17)					\n\
	subl $16, 1, $16					\n\
	addl $21, 64, $21					\n\
	addl $20, 64, $20					\n\
								\n\
	addl $19, 64, $19					\n\
	addl $18, 64, $18					\n\
	addl $17, 64, $17					\n\
	bgt $16, 5b						\n\
								\n\
	ret							\n\
	.end xor_sw64_prefetch_5				\n\
");

static struct xor_block_template xor_block_sw64 = {
	.name	= "sw64",
	.do_2	= xor_sw64_2,
	.do_3	= xor_sw64_3,
	.do_4	= xor_sw64_4,
	.do_5	= xor_sw64_5,
};

static struct xor_block_template xor_block_sw64_prefetch = {
	.name	= "sw64 prefetch",
	.do_2	= xor_sw64_prefetch_2,
	.do_3	= xor_sw64_prefetch_3,
	.do_4	= xor_sw64_prefetch_4,
	.do_5	= xor_sw64_prefetch_5,
};

/* For grins, also test the generic routines.  */
#include <asm-generic/xor.h>

#undef XOR_TRY_TEMPLATES
#define XOR_TRY_TEMPLATES				\
	do {						\
		xor_speed(&xor_block_8regs);		\
		xor_speed(&xor_block_32regs);		\
		xor_speed(&xor_block_sw64);		\
		xor_speed(&xor_block_sw64_prefetch);	\
	} while (0)

/* Force the use of sw64_prefetch as it is significantly
 * faster in the cold cache case.
 */
#define XOR_SELECT_TEMPLATE(FASTEST)   (&xor_block_sw64_prefetch)

#endif /* _ASM_SW64_XOR_H */
