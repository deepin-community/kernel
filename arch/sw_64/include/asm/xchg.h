/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_XCHG_H
#define _ASM_SW64_XCHG_H

#ifndef _ASM_SW64_CMPXCHG_H
#error Do not include xchg.h directly. Use cmpxchg.h
#endif
/*
 * xchg/xchg_local and cmpxchg/cmpxchg_local share the same code
 * except that local version do not have the expensive memory barrier.
 * So this file is included twice from asm/cmpxchg.h.
 */

#if defined(CONFIG_SUBARCH_C3B)
/*
 * Atomic exchange.
 * Since it can be used to implement critical sections
 * it must clobber "memory" (also for interrupts in UP).
 */

static inline unsigned long
____xchg(_u8, volatile char *m, unsigned long val)
{
	unsigned long ret, tmp, addr64;

	__asm__ __volatile__(

	"	andnot	%4, 7, %3\n"
	"	inslb	%1, %4, %1\n"
	"1:	lldl	%2, 0(%3)\n"
	"	ldi	%0, 1\n"
	"	wr_f	%0\n"
	"	extlb	%2, %4, %0\n"
	"	masklb	%2, %4, %2\n"
	"	or	%1, %2, %2\n"
	"	lstl	%2, 0(%3)\n"
	"	rd_f	%2\n"
	"	beq	%2, 2f\n"
	".subsection 2\n"
	"2:	br	1b\n"
	".previous"
	: "=&r" (ret), "=&r" (val), "=&r" (tmp), "=&r" (addr64)
	: "r" ((long)m), "1" (val) : "memory");

	return ret;
}

static inline unsigned long
____xchg(_u16, volatile short *m, unsigned long val)
{
	unsigned long ret, tmp, addr64;

	__asm__ __volatile__(
	"	andnot	%4, 7, %3\n"
	"	inslh	%1, %4, %1\n"
	"1:	lldl	%2, 0(%3)\n"
	"	ldi	%0, 1\n"
	"	wr_f	%0\n"
	"	extlh	%2, %4, %0\n"
	"	masklh	%2, %4, %2\n"
	"	or	%1, %2, %2\n"
	"	lstl	%2, 0(%3)\n"
	"	rd_f	%2\n"
	"	beq	%2, 2f\n"
	".subsection 2\n"
	"2:	br	1b\n"
	".previous"
	: "=&r" (ret), "=&r" (val), "=&r" (tmp), "=&r" (addr64)
	: "r" ((long)m), "1" (val) : "memory");

	return ret;
}

static inline unsigned long
____xchg(_u32, volatile int *m, unsigned long val)
{
	unsigned long dummy, addr;

	__asm__ __volatile__(
	"	ldi	%3, %5\n"
	"1:	lldw	%0, 0(%3)\n"
	"	ldi	%1, 1\n"
	"	wr_f	%1\n"
	"	bis	$31, %4, %1\n"
	"	lstw	%1, 0(%3)\n"
	"	rd_f	%1\n"
	"	beq	%1, 2f\n"
	".subsection 2\n"
	"2:	br	1b\n"
	".previous"
	: "=&r" (val), "=&r" (dummy), "=m" (*m), "=&r"(addr)
	: "rI" (val), "m" (*m) : "memory");

	return val;
}

static inline unsigned long
____xchg(_u64, volatile long *m, unsigned long val)
{
	unsigned long dummy, addr;

	__asm__ __volatile__(
	"	ldi	%3, %5\n"
	"1:	lldl	%0, 0(%3)\n"
	"	ldi	%1, 1\n"
	"	wr_f	%1\n"
	"	bis	$31, %4, %1\n"
	"	lstl	%1, 0(%3)\n"
	"	rd_f	%1\n"
	"	beq	%1, 2f\n"
	".subsection 2\n"
	"2:	br	1b\n"
	".previous"
	: "=&r" (val), "=&r" (dummy), "=m" (*m), "=&r"(addr)
	: "rI" (val), "m" (*m) : "memory");

	return val;
}

/*
 * Atomic compare and exchange.  Compare OLD with MEM, if identical,
 * store NEW in MEM.  Return the initial value in MEM.  Success is
 * indicated by comparing RETURN with OLD.
 *
 * The memory barrier should be placed in SMP only when we actually
 * make the change. If we don't change anything (so if the returned
 * prev is equal to old) then we aren't acquiring anything new and
 * we don't need any memory barrier as far I can tell.
 */

static inline unsigned long
____cmpxchg(_u8, volatile char *m, unsigned char old, unsigned char new)
{
	unsigned long prev, tmp, cmp, addr64;

	__asm__ __volatile__(
	"	andnot	%5, 7, %4\n"
	"	inslb	%1, %5, %1\n"
	"1:	lldl	%2, 0(%4)\n"
	"	extlb	%2, %5, %0\n"
	"	cmpeq	%0, %6, %3\n"
	"	wr_f	%3\n"
	"	masklb	%2, %5, %2\n"
	"	or	%1, %2, %2\n"
	"	lstl	%2, 0(%4)\n"
	"	rd_f	%2\n"
	"	beq	%3, 2f\n"
	"	beq	%2, 3f\n"
	"2:\n"
	".subsection 2\n"
	"3:	br	1b\n"
	".previous"
	: "=&r" (prev), "=&r" (new), "=&r" (tmp), "=&r" (cmp), "=&r" (addr64)
	: "r" ((long)m), "Ir" (old), "1" (new) : "memory");

	return prev;
}

static inline unsigned long
____cmpxchg(_u16, volatile short *m, unsigned short old, unsigned short new)
{
	unsigned long prev, tmp, cmp, addr64;

	__asm__ __volatile__(
	"	andnot	%5, 7, %4\n"
	"	inslh	%1, %5, %1\n"
	"1:	lldl	%2, 0(%4)\n"
	"	extlh	%2, %5, %0\n"
	"	cmpeq	%0, %6, %3\n"
	"	wr_f	%3\n"
	"	masklh	%2, %5, %2\n"
	"	or	%1, %2, %2\n"
	"	lstl	%2, 0(%4)\n"
	"	rd_f	%2\n"
	"	beq	%3, 2f\n"
	"	beq	%2, 3f\n"
	"2:\n"
	".subsection 2\n"
	"3:	br	1b\n"
	".previous"
	: "=&r" (prev), "=&r" (new), "=&r" (tmp), "=&r" (cmp), "=&r" (addr64)
	: "r" ((long)m), "Ir" (old), "1" (new) : "memory");

	return prev;
}

static inline unsigned long
____cmpxchg(_u32, volatile int *m, int old, int new)
{
	unsigned long prev, cmp, addr, tmp;

	__asm__ __volatile__(
	"	ldi	%3, %7\n"
	"1:	lldw	%0, 0(%3)\n"
	"	cmpeq	%0, %5, %1\n"
	"	wr_f	%1\n"
	"	bis	$31, %6, %4\n"
	"	lstw	%4, 0(%3)\n"
	"	rd_f	%4\n"
	"	beq	%1, 2f\n"
	"	beq	%4, 3f\n"
	"2:\n"
	".subsection 2\n"
	"3:	br	1b\n"
	".previous"
	: "=&r"(prev), "=&r"(cmp), "=m"(*m), "=&r"(addr), "=&r"(tmp)
	: "r"((long) old), "r"(new), "m"(*m) : "memory");

	return prev;
}

static inline unsigned long
____cmpxchg(_u64, volatile long *m, unsigned long old, unsigned long new)
{
	unsigned long prev, cmp, addr, tmp;

	__asm__ __volatile__(
	"	ldi	%3, %7\n"
	"1:	lldl	%0, 0(%3)\n"
	"	cmpeq	%0, %5, %1\n"
	"	wr_f	%1\n"
	"	bis	$31, %6, %4\n"
	"	lstl	%4, 0(%3)\n"
	"	rd_f	%4\n"
	"	beq	%1, 2f\n"
	"	beq	%4, 3f\n"
	"2:\n"
	".subsection 2\n"
	"3:	br	1b\n"
	".previous"
	: "=&r"(prev), "=&r"(cmp), "=m"(*m), "=&r"(addr), "=&r"(tmp)
	: "r"((long) old), "r"(new), "m"(*m) : "memory");

	return prev;
}

#elif defined(CONFIG_SUBARCH_C4)
/*
 * Atomic exchange.
 * Since it can be used to implement critical sections
 * it must clobber "memory" (also for interrupts in UP).
 */

static inline unsigned long
____xchg(_u8, volatile char *m, unsigned long val)
{
	unsigned long ret, tmp, addr64;

	__asm__ __volatile__(
	"	andnot	%4, 7, %3\n"
	"	inslb	%1, %4, %1\n"
	"1:	lldl	%2, 0(%3)\n"
	"	extlb	%2, %4, %0\n"
	"	masklb	%2, %4, %2\n"
	"	or	%1, %2, %2\n"
	"	lstl	%2, 0(%3)\n"
	"	beq	%2, 2f\n"
	".subsection 2\n"
	"2:	lbr	1b\n"
	".previous"
	: "=&r" (ret), "=&r" (val), "=&r" (tmp), "=&r" (addr64)
	: "r" ((long)m), "1" (val) : "memory");

	return ret;
}

static inline unsigned long
____xchg(_u16, volatile short *m, unsigned long val)
{
	unsigned long ret, tmp, addr64;

	__asm__ __volatile__(
	"	andnot	%4, 7, %3\n"
	"	inslh	%1, %4, %1\n"
	"1:	lldl	%2, 0(%3)\n"
	"	extlh	%2, %4, %0\n"
	"	masklh	%2, %4, %2\n"
	"	or	%1, %2, %2\n"
	"	lstl	%2, 0(%3)\n"
	"	beq	%2, 2f\n"
	".subsection 2\n"
	"2:	lbr	1b\n"
	".previous"
	: "=&r" (ret), "=&r" (val), "=&r" (tmp), "=&r" (addr64)
	: "r" ((long)m), "1" (val) : "memory");

	return ret;
}

static inline unsigned long
____xchg(_u32, volatile int *m, unsigned long val)
{
	unsigned long dummy, addr;

	__asm__ __volatile__(
	"	ldi	%3, %5\n"
	"1:	lldw	%0, 0(%3)\n"
	"	bis	$31, %4, %1\n"
	"	lstw	%1, 0(%3)\n"
	"	beq	%1, 2f\n"
	".subsection 2\n"
	"2:	lbr	1b\n"
	".previous"
	: "=&r" (val), "=&r" (dummy), "=m" (*m), "=&r"(addr)
	: "rI" (val), "m" (*m) : "memory");

	return val;
}

static inline unsigned long
____xchg(_u64, volatile long *m, unsigned long val)
{
	unsigned long dummy, addr;

	__asm__ __volatile__(
	"	ldi	%3, %5\n"
	"1:	lldl	%0, 0(%3)\n"
	"	bis	$31, %4, %1\n"
	"	lstl	%1, 0(%3)\n"
	"	beq	%1, 2f\n"
	".subsection 2\n"
	"2:	lbr	1b\n"
	".previous"
	: "=&r" (val), "=&r" (dummy), "=m" (*m), "=&r"(addr)
	: "rI" (val), "m" (*m) : "memory");

	return val;
}

/*
 * Atomic compare and exchange. Compare OLD with MEM, if identical,
 * store NEW in MEM. Return the initial value in MEM. Success is
 * indicated by comparing RETURN with OLD.
 *
 * The memory barrier should be placed in SMP only when we actually
 * make the change. If we don't change anything (so if the returned
 * prev is equal to old) then we aren't acquiring anything new and
 * we don't need any memory barrier as far I can tell.
 */
static inline unsigned long
____cmpxchg(_u8, volatile char *m, unsigned char old, unsigned char new)
{
	unsigned long prev, tmp, cmp, addr64;

	__asm__ __volatile__(
	"	andnot	%5, 7, %4\n"
	"	inslb	%1, %5, %1\n"
	"1:	lldl	%2, 0(%4)\n"
	"	extlb	%2, %5, %0\n"
	"	cmpeq	%0, %6, %3\n"
	"	beq	%3, 2f\n"
	"	masklb	%2, %5, %2\n"
	"	or	%1, %2, %2\n"
	"	lstl	%2, 0(%4)\n"
	"	beq	%2, 3f\n"
	"2:\n"
	".subsection 2\n"
	"3:	lbr	1b\n"
	".previous"
	: "=&r" (prev), "=&r" (new), "=&r" (tmp), "=&r" (cmp), "=&r" (addr64)
	: "r" ((long)m), "Ir" (old), "1" (new) : "memory");

	return prev;
}

static inline unsigned long
____cmpxchg(_u16, volatile short *m, unsigned short old, unsigned short new)
{
	unsigned long prev, tmp, cmp, addr64;

	__asm__ __volatile__(
	"	andnot	%5, 7, %4\n"
	"	inslh	%1, %5, %1\n"
	"1:	lldl	%2, 0(%4)\n"
	"	extlh	%2, %5, %0\n"
	"	cmpeq	%0, %6, %3\n"
	"	beq	%3, 2f\n"
	"	masklh	%2, %5, %2\n"
	"	or	%1, %2, %2\n"
	"	lstl	%2, 0(%4)\n"
	"	beq	%2, 3f\n"
	"2:\n"
	".subsection 2\n"
	"3:	lbr	1b\n"
	".previous"
	: "=&r" (prev), "=&r" (new), "=&r" (tmp), "=&r" (cmp), "=&r" (addr64)
	: "r" ((long)m), "Ir" (old), "1" (new) : "memory");

	return prev;
}

static inline unsigned long
____cmpxchg(_u32, volatile int *m, int old, int new)
{
	unsigned long prev, cmp, addr, tmp;

	__asm__ __volatile__(
	"	ldi	%3, %7\n"
	"1:	lldw	%0, 0(%3)\n"
	"	cmpeq	%0, %5, %1\n"
	"	beq	%1, 2f\n"
	"	bis	$31, %6, %4\n"
	"	lstw	%4, 0(%3)\n"
	"	beq	%4, 3f\n"
	"2:\n"
	".subsection 2\n"
	"3:	lbr	1b\n"
	".previous"
	: "=&r"(prev), "=&r"(cmp), "=m"(*m), "=&r"(addr), "=&r"(tmp)
	: "r"((long) old), "r"(new), "m"(*m) : "memory");

	return prev;
}

static inline unsigned long
____cmpxchg(_u64, volatile long *m, unsigned long old, unsigned long new)
{
	unsigned long prev, cmp, addr, tmp;

	__asm__ __volatile__(
	"	ldi	%3, %7\n"
	"1:	lldl	%0, 0(%3)\n"
	"	cmpeq	%0, %5, %1\n"
	"	beq	%1, 2f\n"
	"	bis	$31, %6, %4\n"
	"	lstl	%4, 0(%3)\n"
	"	beq	%4, 3f\n"
	"2:\n"
	".subsection 2\n"
	"3:	lbr	1b\n"
	".previous"
	: "=&r"(prev), "=&r"(cmp), "=m"(*m), "=&r"(addr), "=&r"(tmp)
	: "r"((long) old), "r"(new), "m"(*m) : "memory");

	return prev;
}

#endif

/* This function doesn't exist, so you'll get a linker error
 * if something tries to do an invalid xchg().
 */
extern void __xchg_called_with_bad_pointer(void);

static __always_inline unsigned long
____xchg(, volatile void *ptr, unsigned long x, int size)
{
	switch (size) {
	case 1:
		return ____xchg(_u8, ptr, x);
	case 2:
		return ____xchg(_u16, ptr, x);
	case 4:
		return ____xchg(_u32, ptr, x);
	case 8:
		return ____xchg(_u64, ptr, x);
	}
	__xchg_called_with_bad_pointer();
	return x;
}

/* This function doesn't exist, so you'll get a linker error
 * if something tries to do an invalid cmpxchg().
 */
extern void __cmpxchg_called_with_bad_pointer(void);

static __always_inline unsigned long ____cmpxchg(, volatile void *ptr,
						 unsigned long old,
						 unsigned long new, int size)
{
	switch (size) {
	case 1:
		return ____cmpxchg(_u8, ptr, old, new);
	case 2:
		return ____cmpxchg(_u16, ptr, old, new);
	case 4:
		return ____cmpxchg(_u32, ptr, old, new);
	case 8:
		return ____cmpxchg(_u64, ptr, old, new);
	}
	__cmpxchg_called_with_bad_pointer();
	return old;
}

#endif /* _ASM_SW64_XCHG_H */
