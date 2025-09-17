// SPDX-License-Identifier: GPL-2.0

#include <linux/extable.h>
#include <linux/mm.h>
#include <linux/perf_event.h>
#include <linux/ptrace.h>
#include <linux/rwsem.h>
#include <linux/signal.h>
#include <linux/uaccess.h>

#include <asm/debug.h>
#include <asm/fpu.h>

#include "proto.h"

#define OP_INT_MASK	(1L << 0x22 | 1L << 0x2a | /* ldw stw */	\
			 1L << 0x23 | 1L << 0x2b | /* ldl stl */	\
			 1L << 0x21 | 1L << 0x29 | /* ldhu sth */	\
			 1L << 0x20 | 1L << 0x28)  /* ldbu stb */

#define FN_INT_MASK	(1L << 0x0 | 1L << 0x6 |   /* ldbu_a stb_a */	\
			 1L << 0x1 | 1L << 0x7 |   /* ldhu_a sth_a */	\
			 1L << 0x2 | 1L << 0x8 |   /* ldw_a stw_a */	\
			 1L << 0x3 | 1L << 0x9)    /* ldl_a stl_a */

static bool access_is_valid(void *va, unsigned int len)
{
	long error;
	unsigned long tmp1, tmp2, vb;


	vb = (unsigned long) va + len - 1;
	/* It is ok in a page */
	if (((unsigned long)va & ~0x1FFFUL) == (vb & ~0x1FFFUL))
		return true;

	__asm__ __volatile__(
		"1:	ldbu	%1, 0(%3)\n"
		"2:	stb	%1, 0(%3)\n"
		"3:	ldbu	%2, 0(%4)\n"
		"4:	stb	%2, 0(%4)\n"
		"5:\n"
		".section __ex_table, \"a\"\n\t"
		"	.long	1b - .\n"
		"	ldi	$31, 5b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	$31, 5b-2b(%0)\n"
		"	.long	3b - .\n"
		"	ldi	$31, 5b-3b(%0)\n"
		"	.long	4b - .\n"
		"	ldi	$31, 5b-4b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "r"(vb), "0"(0));

	return !error;
}

asmlinkage void noinstr do_entUna(struct pt_regs *regs)
{
	long error, disp;
	unsigned int insn, fncode, rb;
	unsigned long tmp, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, vb;
	unsigned long fp[4];
	unsigned long fake_reg, *reg_addr = &fake_reg;
	unsigned long pc = regs->pc - 4;
	void *va = (void *)regs->earg0;
	unsigned long opcode = regs->earg1;
	unsigned long reg = regs->earg2;

	if (reg == 29)
		return;

	insn = *(unsigned int *)pc;
	fncode = (insn >> 12) & 0xf;

	if (((1L << opcode) & OP_INT_MASK) ||
			((opcode == 0x1e) && ((1L << fncode) & FN_INT_MASK))) {
		/* it's an integer load/store */
		if (reg < 31) {
			reg_addr = &regs->regs[reg];
		} else {
			/* zero "register" */
			fake_reg = 0;
		}
	}

	/*
	 * We don't want to use the generic get/put unaligned macros as
	 * we want to trap exceptions. Only if we actually get an
	 * exception will we decide whether we should have caught it.
	 */

	switch (opcode) {

	case 0x0c:  /* vlds */
		if ((unsigned long)va<<61 == 0) {
			__asm__ __volatile__(
			"1:	ldl	%1, 0(%5)\n"
			"2:	ldl	%2, 8(%5)\n"
			"3:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3), "=&r"(tmp4)
			: "r"(va), "0"(0));

			if (error)
				goto got_exception;

			sw64_write_simd_fp_reg_s(reg, tmp1, tmp2);

			return;
		} else {
			__asm__ __volatile__(
			"1:	ldl_u	%1, 0(%6)\n"
			"2:	ldl_u	%2, 7(%6)\n"
			"3:	ldl_u	%3, 15(%6)\n"
			"	extll	%1, %6, %1\n"
			"	extll	%2, %6, %5\n"
			"	exthl	%2, %6, %4\n"
			"	exthl	%3, %6, %3\n"
			"4:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 4b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 4b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	%3, 4b-3b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5)
			: "r"(va), "0"(0));

			if (error)
				goto got_exception;

			tmp1 = tmp1 | tmp4;
			tmp2 = tmp5 | tmp3;

			sw64_write_simd_fp_reg_s(reg, tmp1, tmp2);

			return;
		}

	case 0x0d: /* vldd */
		if ((unsigned long)va<<61 == 0) {
			__asm__ __volatile__(
			"1:	ldl	%1, 0(%5)\n"
			"2:	ldl	%2, 8(%5)\n"
			"3:	ldl	%3, 16(%5)\n"
			"4:	ldl	%4, 24(%5)\n"
			"5:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 5b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 5b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	%3, 5b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	%4, 5b-4b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3), "=&r"(tmp4)
			: "r"(va), "0"(0));

			if (error)
				goto got_exception;

			sw64_write_simd_fp_reg_d(reg, tmp1, tmp2, tmp3, tmp4);

			return;
		} else {
			__asm__ __volatile__(
			"1:	ldl_u	%1, 0(%6)\n"
			"2:	ldl_u	%2, 7(%6)\n"
			"3:	ldl_u	%3, 15(%6)\n"
			"	extll	%1, %6, %1\n"
			"	extll	%2, %6, %5\n"
			"	exthl	%2, %6, %4\n"
			"	exthl	%3, %6, %3\n"
			"4:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 4b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 4b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	%3, 4b-3b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5)
			: "r"(va), "0"(0));

			if (error)
				goto got_exception;

			tmp7 = tmp1 | tmp4;		//f0
			tmp8 = tmp5 | tmp3;		//f1

			vb = ((unsigned long)(va))+16;

			__asm__ __volatile__(
			"1:	ldl_u	%1, 0(%6)\n"
			"2:	ldl_u	%2, 7(%6)\n"
			"3:	ldl_u	%3, 15(%6)\n"
			"	extll	%1, %6, %1\n"
			"	extll	%2, %6, %5\n"
			"	exthl	%2, %6, %4\n"
			"	exthl	%3, %6, %3\n"
			"4:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 4b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 4b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	%3, 4b-3b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5)
			: "r"(vb), "0"(0));

			if (error)
				goto got_exception;

			tmp = tmp1 | tmp4;			// f2
			tmp2 = tmp5 | tmp3;			// f3

			sw64_write_simd_fp_reg_d(reg, tmp7, tmp8, tmp, tmp2);
			return;
		}

	case 0x0e: /* vsts */
		if (!access_is_valid(va, 16))
			goto got_exception;

		sw64_read_simd_fp_m_s(reg, fp);
		if ((unsigned long)va<<61 == 0) {
			__asm__ __volatile__(
			"	bis	%4, %4, %1\n"
			"	bis	%5, %5, %2\n"
			"1:	stl	%1, 0(%3)\n"
			"2:	stl	%2, 8(%3)\n"
			"3:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
			: "r"(va), "r"(fp[0]), "r"(fp[1]), "0"(0));

			if (error)
				goto got_exception;

			return;
		} else {
			__asm__ __volatile__(
			"	zapnot	%10, 0x1, %1\n"
			"	srl	%10, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%10, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%10, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"	srl	%10, 32, %5\n"
			"	zapnot	%5, 0x1, %5\n"
			"	srl	%10, 40, %6\n"
			"	zapnot	%6, 0x1, %6\n"
			"	srl	%10, 48, %7\n"
			"	zapnot	%7, 0x1, %7\n"
			"	srl	%10, 56, %8\n"
			"	zapnot	%8, 0x1, %8\n"
			"1:	stb	%1, 0(%9)\n"
			"2:	stb	%2, 1(%9)\n"
			"3:	stb	%3, 2(%9)\n"
			"4:	stb	%4, 3(%9)\n"
			"5:	stb	%5, 4(%9)\n"
			"6:	stb	%6, 5(%9)\n"
			"7:	stb	%7, 6(%9)\n"
			"8:	stb	%8, 7(%9)\n"
			"9:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 9b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 9b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 9b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 9b-4b(%0)\n"
			"	.long	5b - .\n"
			"	ldi	$31, 9b-5b(%0)\n"
			"	.long	6b - .\n"
			"	ldi	$31, 9b-6b(%0)\n"
			"	.long	7b - .\n"
			"	ldi	$31, 9b-7b(%0)\n"
			"	.long	8b - .\n"
			"	ldi	$31, 9b-8b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
			: "r"(va), "r"(fp[0]), "0"(0));

			if (error)
				goto got_exception;


			vb = ((unsigned long)va) + 8;

			__asm__ __volatile__(
			"	zapnot	%10, 0x1, %1\n"
			"	srl	%10, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%10, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%10, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"	srl	%10, 32, %5\n"
			"	zapnot	%5, 0x1, %5\n"
			"	srl	%10, 40, %6\n"
			"	zapnot	%6, 0x1, %6\n"
			"	srl	%10, 48, %7\n"
			"	zapnot	%7, 0x1, %7\n"
			"	srl	%10, 56, %8\n"
			"	zapnot	%8, 0x1, %8\n"
			"1:	stb	%1, 0(%9)\n"
			"2:	stb	%2, 1(%9)\n"
			"3:	stb	%3, 2(%9)\n"
			"4:	stb	%4, 3(%9)\n"
			"5:	stb	%5, 4(%9)\n"
			"6:	stb	%6, 5(%9)\n"
			"7:	stb	%7, 6(%9)\n"
			"8:	stb	%8, 7(%9)\n"
			"9:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 9b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 9b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 9b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 9b-4b(%0)\n"
			"	.long	5b - .\n"
			"	ldi	$31, 9b-5b(%0)\n"
			"	.long	6b - .\n"
			"	ldi	$31, 9b-6b(%0)\n"
			"	.long	7b - .\n"
			"	ldi	$31, 9b-7b(%0)\n"
			"	.long	8b - .\n"
			"	ldi	$31, 9b-8b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
			: "r"(vb), "r"(fp[1]), "0"(0));

			if (error)
				goto got_exception;

			return;
		}

	case 0x0f: /* vstd */
		if (!access_is_valid(va, 32))
			goto got_exception;

		sw64_read_simd_fp_m_d(reg, fp);
		if ((unsigned long)va<<61 == 0) {
			__asm__ __volatile__(
			"	bis	%4, %4, %1\n"
			"	bis	%5, %5, %2\n"
			"1:	stl	%1, 0(%3)\n"
			"2:	stl	%2, 8(%3)\n"
			"3:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
			: "r"(va), "r"(fp[0]), "r"(fp[1]), "0"(0));

			if (error)
				goto got_exception;

			vb = ((unsigned long)va)+16;


			__asm__ __volatile__(
			"	bis	%4, %4, %1\n"
			"	bis	%5, %5, %2\n"
			"1:	stl	%1, 0(%3)\n"
			"2:	stl	%2, 8(%3)\n"
			"3:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
			: "r"(vb), "r"(fp[2]), "r"(fp[3]), "0"(0));

			if (error)
				goto got_exception;

			return;
		} else {
			__asm__ __volatile__(
			"	zapnot	%10, 0x1, %1\n"
			"	srl	%10, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%10, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%10, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"	srl	%10, 32, %5\n"
			"	zapnot	%5, 0x1, %5\n"
			"	srl	%10, 40, %6\n"
			"	zapnot	%6, 0x1, %6\n"
			"	srl	%10, 48, %7\n"
			"	zapnot	%7, 0x1, %7\n"
			"	srl	%10, 56, %8\n"
			"	zapnot	%8, 0x1, %8\n"
			"1:	stb	%1, 0(%9)\n"
			"2:	stb	%2, 1(%9)\n"
			"3:	stb	%3, 2(%9)\n"
			"4:	stb	%4, 3(%9)\n"
			"5:	stb	%5, 4(%9)\n"
			"6:	stb	%6, 5(%9)\n"
			"7:	stb	%7, 6(%9)\n"
			"8:	stb	%8, 7(%9)\n"
			"9:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 9b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 9b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 9b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 9b-4b(%0)\n"
			"	.long	5b - .\n"
			"	ldi	$31, 9b-5b(%0)\n"
			"	.long	6b - .\n"
			"	ldi	$31, 9b-6b(%0)\n"
			"	.long	7b - .\n"
			"	ldi	$31, 9b-7b(%0)\n"
			"	.long	8b - .\n"
			"	ldi	$31, 9b-8b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
			: "r"(va), "r"(fp[0]), "0"(0));

			if (error)
				goto got_exception;

			vb = ((unsigned long)va) + 8;

			__asm__ __volatile__(
			"	zapnot	%10, 0x1, %1\n"
			"	srl	%10, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%10, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%10, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"	srl	%10, 32, %5\n"
			"	zapnot	%5, 0x1, %5\n"
			"	srl	%10, 40, %6\n"
			"	zapnot	%6, 0x1, %6\n"
			"	srl	%10, 48, %7\n"
			"	zapnot	%7, 0x1, %7\n"
			"	srl	%10, 56, %8\n"
			"	zapnot	%8, 0x1, %8\n"
			"1:	stb	%1, 0(%9)\n"
			"2:	stb	%2, 1(%9)\n"
			"3:	stb	%3, 2(%9)\n"
			"4:	stb	%4, 3(%9)\n"
			"5:	stb	%5, 4(%9)\n"
			"6:	stb	%6, 5(%9)\n"
			"7:	stb	%7, 6(%9)\n"
			"8:	stb	%8, 7(%9)\n"
			"9:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 9b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 9b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 9b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 9b-4b(%0)\n"
			"	.long	5b - .\n"
			"	ldi	$31, 9b-5b(%0)\n"
			"	.long	6b - .\n"
			"	ldi	$31, 9b-6b(%0)\n"
			"	.long	7b - .\n"
			"	ldi	$31, 9b-7b(%0)\n"
			"	.long	8b - .\n"
			"	ldi	$31, 9b-8b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
			: "r"(vb), "r"(fp[1]), "0"(0));

			if (error)
				goto got_exception;

			vb = vb + 8;

			__asm__ __volatile__(
			"	zapnot	%10, 0x1, %1\n"
			"	srl	%10, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%10, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%10, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"	srl	%10, 32, %5\n"
			"	zapnot	%5, 0x1, %5\n"
			"	srl	%10, 40, %6\n"
			"	zapnot	%6, 0x1, %6\n"
			"	srl	%10, 48, %7\n"
			"	zapnot	%7, 0x1, %7\n"
			"	srl	%10, 56, %8\n"
			"	zapnot	%8, 0x1, %8\n"
			"1:	stb	%1, 0(%9)\n"
			"2:	stb	%2, 1(%9)\n"
			"3:	stb	%3, 2(%9)\n"
			"4:	stb	%4, 3(%9)\n"
			"5:	stb	%5, 4(%9)\n"
			"6:	stb	%6, 5(%9)\n"
			"7:	stb	%7, 6(%9)\n"
			"8:	stb	%8, 7(%9)\n"
			"9:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 9b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 9b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 9b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 9b-4b(%0)\n"
			"	.long	5b - .\n"
			"	ldi	$31, 9b-5b(%0)\n"
			"	.long	6b - .\n"
			"	ldi	$31, 9b-6b(%0)\n"
			"	.long	7b - .\n"
			"	ldi	$31, 9b-7b(%0)\n"
			"	.long	8b - .\n"
			"	ldi	$31, 9b-8b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
			: "r"(vb), "r"(fp[2]), "0"(0));

			if (error)
				goto got_exception;

			vb = vb + 8;

			__asm__ __volatile__(
			"	zapnot	%10, 0x1, %1\n"
			"	srl	%10, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%10, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%10, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"	srl	%10, 32, %5\n"
			"	zapnot	%5, 0x1, %5\n"
			"	srl	%10, 40, %6\n"
			"	zapnot	%6, 0x1, %6\n"
			"	srl	%10, 48, %7\n"
			"	zapnot	%7, 0x1, %7\n"
			"	srl	%10, 56, %8\n"
			"	zapnot	%8, 0x1, %8\n"
			"1:	stb	%1, 0(%9)\n"
			"2:	stb	%2, 1(%9)\n"
			"3:	stb	%3, 2(%9)\n"
			"4:	stb	%4, 3(%9)\n"
			"5:	stb	%5, 4(%9)\n"
			"6:	stb	%6, 5(%9)\n"
			"7:	stb	%7, 6(%9)\n"
			"8:	stb	%8, 7(%9)\n"
			"9:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 9b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 9b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 9b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 9b-4b(%0)\n"
			"	.long	5b - .\n"
			"	ldi	$31, 9b-5b(%0)\n"
			"	.long	6b - .\n"
			"	ldi	$31, 9b-6b(%0)\n"
			"	.long	7b - .\n"
			"	ldi	$31, 9b-7b(%0)\n"
			"	.long	8b - .\n"
			"	ldi	$31, 9b-8b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
			: "r"(vb), "r"(fp[3]), "0"(0));

			if (error)
				goto got_exception;

			return;
		}

	case 0x1e:
		insn = *(unsigned int *)pc;
		fncode = (insn >> 12) & 0xf;
		rb = (insn >> 16) & 0x1f;
		disp = insn & 0xfff;

		disp = (disp << 52) >> 52;	/* sext */

		switch (fncode) {
		case 0x1: /* ldhu_a */
			__asm__ __volatile__(
			"1:	ldl_u	%1, 0(%3)\n"
			"2:	ldl_u	%2, 1(%3)\n"
			"	extlh	%1, %3, %1\n"
			"	exthh	%2, %3, %2\n"
			"3:\n"
			".section __ex_table,\"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
			: "r"(va), "0"(0));
			if (error)
				goto got_exception;
			regs->regs[reg] = tmp1 | tmp2;
			regs->regs[rb] = regs->regs[rb] + disp;
			return;

		case 0x2: /* ldw_a */
			__asm__ __volatile__(
			"1:	ldl_u	%1,0(%3)\n"
			"2:	ldl_u	%2,3(%3)\n"
			"	extlw	%1,%3,%1\n"
			"	exthw	%2,%3,%2\n"
			"3:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
			: "r"(va), "0"(0));

			if (error)
				goto got_exception;
			regs->regs[reg] = (int)(tmp1 | tmp2);
			regs->regs[rb] = regs->regs[rb] + disp;
			return;

		case 0x3: /* ldl_a */
			__asm__ __volatile__(
			"1:	ldl_u	%1, 0(%3)\n"
			"2:	ldl_u	%2, 7(%3)\n"
			"	extll	%1, %3, %1\n"
			"	exthl	%2, %3, %2\n"
			"3:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
			: "r"(va), "0"(0));

			if (error)
				goto got_exception;
			regs->regs[reg] = tmp1 | tmp2;
			regs->regs[rb] = regs->regs[rb] + disp;
			return;

		case 0x7: /* sth_a */
			if (!access_is_valid(va, 2))
				goto got_exception;

			__asm__ __volatile__(
			"	zap	%6, 2, %1\n"
			"	srl	%6, 8, %2\n"
			"1:	stb	%1, 0x0(%5)\n"
			"2:	stb	%2, 0x1(%5)\n"
			"3:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%2, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%1, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2),
			"=&r"(tmp3), "=&r"(tmp4)
			: "r"(va), "r"(*reg_addr), "0"(0));

			if (error)
				goto got_exception;
			regs->regs[rb] = regs->regs[rb] + disp;
			return;

		case 0x8: /* stw_a */
			if (!access_is_valid(va, 4))
				goto got_exception;

			__asm__ __volatile__(
			"	zapnot	%6, 0x1, %1\n"
			"	srl	%6, 8, %2\n"
			"	zapnot	%2, 0x1,%2\n"
			"	srl	%6, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%6, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"1:	stb	%1, 0x0(%5)\n"
			"2:	stb	%2, 0x1(%5)\n"
			"3:	stb	%3, 0x2(%5)\n"
			"4:	stb	%4, 0x3(%5)\n"
			"5:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	$31, 5b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 5b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 5b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 5b-4b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2),
			  "=&r"(tmp3), "=&r"(tmp4)
			: "r"(va), "r"(*reg_addr), "0"(0));

			if (error)
				goto got_exception;
			regs->regs[rb] = regs->regs[rb] + disp;
			return;

		case 0x9: /* stl_a */
			if (!access_is_valid(va, 8))
				goto got_exception;

			__asm__ __volatile__(
			"	zapnot	%10, 0x1, %1\n"
			"	srl	%10, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%10, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%10, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"	srl	%10, 32, %5\n"
			"	zapnot	%5, 0x1, %5\n"
			"	srl	%10, 40, %6\n"
			"	zapnot	%6, 0x1, %6\n"
			"	srl	%10, 48, %7\n"
			"	zapnot	%7, 0x1, %7\n"
			"	srl	%10, 56, %8\n"
			"	zapnot	%8, 0x1, %8\n"
			"1:	stb	%1, 0(%9)\n"
			"2:	stb	%2, 1(%9)\n"
			"3:	stb	%3, 2(%9)\n"
			"4:	stb	%4, 3(%9)\n"
			"5:	stb	%5, 4(%9)\n"
			"6:	stb	%6, 5(%9)\n"
			"7:	stb	%7, 6(%9)\n"
			"8:	stb	%8, 7(%9)\n"
			"9:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 9b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 9b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 9b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 9b-4b(%0)\n"
			"	.long	5b - .\n"
			"	ldi	$31, 9b-5b(%0)\n"
			"	.long	6b - .\n"
			"	ldi	$31, 9b-6b(%0)\n"
			"	.long	7b - .\n"
			"	ldi	$31, 9b-7b(%0)\n"
			"	.long	8b - .\n"
			"	ldi	$31, 9b-8b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			"=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
			: "r"(va), "r"(*reg_addr), "0"(0));

			if (error)
				goto got_exception;
			regs->regs[rb] = regs->regs[rb] + disp;
			return;
		}
		return;

	case 0x21:
		__asm__ __volatile__(
		"1:	ldl_u	%1, 0(%3)\n"
		"2:	ldl_u	%2, 1(%3)\n"
		"	extlh	%1, %3, %1\n"
		"	exthh	%2, %3, %2\n"
		"3:\n"
		".section __ex_table,\"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "0"(0));

		if (error)
			goto got_exception;
		regs->regs[reg] = tmp1 | tmp2;
		return;

	case 0x22:
		__asm__ __volatile__(
		"1:	ldl_u	%1,0(%3)\n"
		"2:	ldl_u	%2,3(%3)\n"
		"	extlw	%1,%3,%1\n"
		"	exthw	%2,%3,%2\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "0"(0));

		if (error)
			goto got_exception;
		regs->regs[reg] = (int)(tmp1 | tmp2);
		return;

	case 0x23: /* ldl */
		__asm__ __volatile__(
		"1:	ldl_u	%1, 0(%3)\n"
		"2:	ldl_u	%2, 7(%3)\n"
		"	extll	%1, %3, %1\n"
		"	exthl	%2, %3, %2\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "0"(0));

		if (error)
			goto got_exception;
		regs->regs[reg] = tmp1 | tmp2;
		return;

	case 0x29: /* sth */
		if (!access_is_valid(va, 2))
			goto got_exception;

		__asm__ __volatile__(
		"	zap	%6, 2, %1\n"
		"	srl	%6, 8, %2\n"
		"1:	stb	%1, 0x0(%5)\n"
		"2:	stb	%2, 0x1(%5)\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%2, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%1, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2),
		"=&r"(tmp3), "=&r"(tmp4)
		: "r"(va), "r"(*reg_addr), "0"(0));

		if (error)
			goto got_exception;
		return;

	case 0x2a: /* stw */
		if (!access_is_valid(va, 4))
			goto got_exception;

		__asm__ __volatile__(
		"	zapnot	%6, 0x1, %1\n"
		"	srl	%6, 8, %2\n"
		"	zapnot	%2, 0x1,%2\n"
		"	srl	%6, 16, %3\n"
		"	zapnot	%3, 0x1, %3\n"
		"	srl	%6, 24, %4\n"
		"	zapnot	%4, 0x1, %4\n"
		"1:	stb	%1, 0x0(%5)\n"
		"2:	stb	%2, 0x1(%5)\n"
		"3:	stb	%3, 0x2(%5)\n"
		"4:	stb	%4, 0x3(%5)\n"
		"5:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	$31, 5b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	$31, 5b-2b(%0)\n"
		"	.long	3b - .\n"
		"	ldi	$31, 5b-3b(%0)\n"
		"	.long	4b - .\n"
		"	ldi	$31, 5b-4b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2),
		  "=&r"(tmp3), "=&r"(tmp4)
		: "r"(va), "r"(*reg_addr), "0"(0));

		if (error)
			goto got_exception;
		return;

	case 0x2b: /* stl */
		/* We worry about the cross-page unaligned write,
		 * so do the page check, if either of the pages is read-only,
		 * reject the unaligned write.
		 */
		if (!access_is_valid(va, 8))
			goto got_exception;

		__asm__ __volatile__(
		"	zapnot	%10, 0x1, %1\n"
		"	srl	%10, 8, %2\n"
		"	zapnot	%2, 0x1, %2\n"
		"	srl	%10, 16, %3\n"
		"	zapnot	%3, 0x1, %3\n"
		"	srl	%10, 24, %4\n"
		"	zapnot	%4, 0x1, %4\n"
		"	srl	%10, 32, %5\n"
		"	zapnot	%5, 0x1, %5\n"
		"	srl	%10, 40, %6\n"
		"	zapnot	%6, 0x1, %6\n"
		"	srl	%10, 48, %7\n"
		"	zapnot	%7, 0x1, %7\n"
		"	srl	%10, 56, %8\n"
		"	zapnot	%8, 0x1, %8\n"
		"1:	stb	%1, 0(%9)\n"
		"2:	stb	%2, 1(%9)\n"
		"3:	stb	%3, 2(%9)\n"
		"4:	stb	%4, 3(%9)\n"
		"5:	stb	%5, 4(%9)\n"
		"6:	stb	%6, 5(%9)\n"
		"7:	stb	%7, 6(%9)\n"
		"8:	stb	%8, 7(%9)\n"
		"9:\n"
		".section __ex_table, \"a\"\n\t"
		"	.long	1b - .\n"
		"	ldi	$31, 9b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	$31, 9b-2b(%0)\n"
		"	.long	3b - .\n"
		"	ldi	$31, 9b-3b(%0)\n"
		"	.long	4b - .\n"
		"	ldi	$31, 9b-4b(%0)\n"
		"	.long	5b - .\n"
		"	ldi	$31, 9b-5b(%0)\n"
		"	.long	6b - .\n"
		"	ldi	$31, 9b-6b(%0)\n"
		"	.long	7b - .\n"
		"	ldi	$31, 9b-7b(%0)\n"
		"	.long	8b - .\n"
		"	ldi	$31, 9b-8b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
		"=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
		: "r"(va), "r"(*reg_addr), "0"(0));

		if (error)
			goto got_exception;
		return;
	}

	pr_warn("Bad unaligned kernel access at %016lx: %p %lx %lu\n",
		pc, va, opcode, reg);
	make_task_dead(SIGSEGV);

got_exception:
	/* Ok, we caught the exception, but we don't want it. Is there
	 * someone to pass it along to?
	 */
	if (fixup_exception(regs, pc)) {
		pr_info("Forwarding unaligned exception at %lx (%lx)\n",
		       pc, regs->pc);
		return;
	}

	/*
	 * Yikes!  No one to forward the exception to.
	 * Since the registers are in a weird format, dump them ourselves.
	 */

	die("Unhandled unaligned exception", regs, error);
}

/*
 * Handle user-level unaligned fault. Handling user-level unaligned
 * faults is *extremely* slow and produces nasty messages. A user
 * program *should* fix unaligned faults ASAP.
 *
 * Notice that we have (almost) the regular kernel stack layout here,
 * so finding the appropriate registers is a little more difficult
 * than in the kernel case.
 *
 * Finally, we handle regular integer load/stores only. In
 * particular, load-linked/store-conditionally and floating point
 * load/stores are not supported. The former make no sense with
 * unaligned faults (they are guaranteed to fail) and I don't think
 * the latter will occur in any decent program.
 *
 * Sigh. We *do* have to handle some FP operations, because GCC will
 * uses them as temporary storage for integer memory to memory copies.
 * However, we need to deal with stt/ldt and sts/lds only.
 */
asmlinkage void noinstr do_entUnaUser(struct pt_regs *regs)
{
#ifdef CONFIG_UNA_PRINT
	static DEFINE_RATELIMIT_STATE(ratelimit, 5 * HZ, 5);
#endif

	unsigned long tmp1, tmp2, tmp3, tmp4;
	unsigned long fake_reg, *reg_addr = &fake_reg;
	int si_code;
	long error;
	unsigned long tmp, tmp5, tmp6, tmp7, tmp8, vb;
	unsigned long fp[4];
	unsigned long instr, instr_op, value, fncode;
	unsigned int rb = -1U;
	long disp;
	void __user *va = (void *)regs->earg0;
	unsigned long opcode = regs->earg1;
	unsigned long reg = regs->earg2;

#ifdef CONFIG_DEBUG_FS
	/*
	 * If command name is specified, record some information
	 * to debugfs.
	 */
	if (unaligned_task[0] && !strcmp(unaligned_task, current->comm)) {
		int idx;

		idx = unaligned_count % UNA_MAX_ENTRIES;
		unaligned[idx].va = (unsigned long)va;
		unaligned[idx].pc = regs->pc;
		unaligned_count++;
	}
#endif

	/* Check the UAC bits to decide what the user wants us to do
	 * with the unaliged access.
	 */
	perf_sw_event(PERF_COUNT_SW_ALIGNMENT_FAULTS,
			1, regs, regs->pc - 4);

#ifdef CONFIG_UNA_PRINT
	if (!(current_thread_info()->status & TS_UAC_NOPRINT)) {
		if (__ratelimit(&ratelimit)) {
			pr_info("%s(%d): unaligned trap at %016lx: %p %lx %ld\n",
			       current->comm, task_pid_nr(current),
			       regs->pc - 4, va, opcode, reg);
		}
	}
#endif
	if ((current_thread_info()->status & TS_UAC_SIGBUS))
		goto give_sigbus;
	/* Not sure why you'd want to use this, but... */
	if ((current_thread_info()->status & TS_UAC_NOFIX))
		return;

	/* Don't bother reading ds in the access check since we already
	 * know that this came from the user. Also rely on the fact that
	 * the page at TASK_SIZE is unmapped and so can't be touched anyway.
	 */
	if ((unsigned long)va >= TASK_SIZE)
		goto give_sigsegv;

	get_user(instr, (__u32 *)(regs->pc - 4));
	fncode = (instr >> 12) & 0xf;

	if (((1L << opcode) & OP_INT_MASK) ||
			((opcode == 0x1e) && ((1L << fncode) & FN_INT_MASK))) {
		/* it's an integer load/store */
		if (reg < 31) {
			reg_addr = &regs->regs[reg];
		} else {
			/* zero "register" */
			fake_reg = 0;
		}
	}

	get_user(instr, (__u32 *)(regs->pc - 4));
	instr_op = (instr >> 26) & 0x3f;

	get_user(value, (__u64 *)va);

	switch (instr_op) {

	case 0x0c:  /* vlds */
		if ((unsigned long)va << 61 == 0) {
			__asm__ __volatile__(
			"1:	ldl	%1, 0(%5)\n"
			"2:	ldl	%2, 8(%5)\n"
			"3:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3), "=&r"(tmp4)
			: "r"(va), "0"(0));

			if (error)
				goto give_sigsegv;

			sw64_write_simd_fp_reg_s(reg, tmp1, tmp2);

			return;
		} else {
			__asm__ __volatile__(
			"1:	ldl_u	%1, 0(%6)\n"
			"2:	ldl_u	%2, 7(%6)\n"
			"3:	ldl_u	%3, 15(%6)\n"
			"	extll	%1, %6, %1\n"
			"	extll	%2, %6, %5\n"
			"	exthl	%2, %6, %4\n"
			"	exthl	%3, %6, %3\n"
			"4:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 4b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 4b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	%3, 4b-3b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5)
			: "r"(va), "0"(0));

			if (error)
				goto give_sigsegv;

			tmp1 = tmp1 | tmp4;
			tmp2 = tmp5 | tmp3;

			sw64_write_simd_fp_reg_s(reg, tmp1, tmp2);

			return;
		}
	case 0x0a: /* ldse */
		__asm__ __volatile__(
		"1:	ldl_u	%1, 0(%3)\n"
		"2:	ldl_u	%2, 3(%3)\n"
		"	extlw	%1, %3, %1\n"
		"	exthw	%2, %3, %2\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "0"(0));

		if (error)
			goto give_sigsegv;

		tmp = tmp1 | tmp2;
		tmp = tmp | (tmp << 32);

		sw64_write_simd_fp_reg_s(reg, tmp, tmp);

		return;

	case 0x0d: /* vldd */
		if ((unsigned long)va << 61 == 0) {
			__asm__ __volatile__(
			"1:	ldl	%1, 0(%5)\n"
			"2:	ldl	%2, 8(%5)\n"
			"3:	ldl	%3, 16(%5)\n"
			"4:	ldl	%4, 24(%5)\n"
			"5:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 5b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 5b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	%3, 5b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	%4, 5b-4b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3), "=&r"(tmp4)
			: "r"(va), "0"(0));

			if (error)
				goto give_sigsegv;

			sw64_write_simd_fp_reg_d(reg, tmp1, tmp2, tmp3, tmp4);

			return;
		} else {
			__asm__ __volatile__(
			"1:	ldl_u	%1, 0(%6)\n"
			"2:	ldl_u	%2, 7(%6)\n"
			"3:	ldl_u	%3, 15(%6)\n"
			"	extll	%1, %6, %1\n"
			"	extll	%2, %6, %5\n"
			"	exthl	%2, %6, %4\n"
			"	exthl	%3, %6, %3\n"
			"4:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 4b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 4b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	%3, 4b-3b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5)
			: "r"(va), "0"(0));

			if (error)
				goto give_sigsegv;

			tmp7 = tmp1 | tmp4;		//f0
			tmp8 = tmp5 | tmp3;		//f1

			vb = ((unsigned long)(va))+16;

			__asm__ __volatile__(
			"1:	ldl_u	%1, 0(%6)\n"
			"2:	ldl_u	%2, 7(%6)\n"
			"3:	ldl_u	%3, 15(%6)\n"
			"	extll	%1, %6, %1\n"
			"	extll	%2, %6, %5\n"
			"	exthl	%2, %6, %4\n"
			"	exthl	%3, %6, %3\n"
			"4:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 4b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 4b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	%3, 4b-3b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5)
			: "r"(vb), "0"(0));

			if (error)
				goto give_sigsegv;

			tmp = tmp1 | tmp4;			// f2
			tmp2 = tmp5 | tmp3;			// f3

			sw64_write_simd_fp_reg_d(reg, tmp7, tmp8, tmp, tmp2);
			return;
		}

	case 0x0b: /* ldde */
		__asm__ __volatile__(
		"1:	ldl_u	%1, 0(%3)\n"
		"2:	ldl_u	%2, 7(%3)\n"
		"	extll	%1, %3, %1\n"
		"	exthl	%2, %3, %2\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
			: "r"(va), "0"(0));

		if (error)
			goto give_sigsegv;

		tmp = tmp1 | tmp2;

		sw64_write_simd_fp_reg_d(reg, tmp, tmp, tmp, tmp);
		return;

	case 0x09: /* ldwe */
		__asm__ __volatile__(
		"1:	ldl_u	%1, 0(%3)\n"
		"2:	ldl_u	%2, 3(%3)\n"
		"	extlw	%1, %3, %1\n"
		"	exthw	%2, %3, %2\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "0"(0));

		if (error)
			goto give_sigsegv;

		sw64_write_simd_fp_reg_ldwe(reg, (int)(tmp1 | tmp2));

		return;

	case 0x0e: /* vsts */
		if (!access_is_valid(va, 16))
			goto give_sigsegv;

		sw64_read_simd_fp_m_s(reg, fp);
		if ((unsigned long)va << 61 == 0) {
			__asm__ __volatile__(
			"	bis	%4, %4, %1\n"
			"	bis	%5, %5, %2\n"
			"1:	stl	%1, 0(%3)\n"
			"2:	stl	%2, 8(%3)\n"
			"3:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
			: "r"(va), "r"(fp[0]), "r"(fp[1]), "0"(0));

			if (error)
				goto give_sigsegv;

			return;
		} else {
			__asm__ __volatile__(
			"	zapnot	%10, 0x1, %1\n"
			"	srl	%10, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%10, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%10, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"	srl	%10, 32, %5\n"
			"	zapnot	%5, 0x1, %5\n"
			"	srl	%10, 40, %6\n"
			"	zapnot	%6, 0x1, %6\n"
			"	srl	%10, 48, %7\n"
			"	zapnot	%7, 0x1, %7\n"
			"	srl	%10, 56, %8\n"
			"	zapnot	%8, 0x1, %8\n"
			"1:	stb	%1, 0(%9)\n"
			"2:	stb	%2, 1(%9)\n"
			"3:	stb	%3, 2(%9)\n"
			"4:	stb	%4, 3(%9)\n"
			"5:	stb	%5, 4(%9)\n"
			"6:	stb	%6, 5(%9)\n"
			"7:	stb	%7, 6(%9)\n"
			"8:	stb	%8, 7(%9)\n"
			"9:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 9b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 9b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 9b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 9b-4b(%0)\n"
			"	.long	5b - .\n"
			"	ldi	$31, 9b-5b(%0)\n"
			"	.long	6b - .\n"
			"	ldi	$31, 9b-6b(%0)\n"
			"	.long	7b - .\n"
			"	ldi	$31, 9b-7b(%0)\n"
			"	.long	8b - .\n"
			"	ldi	$31, 9b-8b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
			: "r"(va), "r"(fp[0]), "0"(0));

			if (error)
				goto give_sigsegv;


			vb = ((unsigned long)va) + 8;

			__asm__ __volatile__(
			"	zapnot	%10, 0x1, %1\n"
			"	srl	%10, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%10, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%10, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"	srl	%10, 32, %5\n"
			"	zapnot	%5, 0x1, %5\n"
			"	srl	%10, 40, %6\n"
			"	zapnot	%6, 0x1, %6\n"
			"	srl	%10, 48, %7\n"
			"	zapnot	%7, 0x1, %7\n"
			"	srl	%10, 56, %8\n"
			"	zapnot	%8, 0x1, %8\n"
			"1:	stb	%1, 0(%9)\n"
			"2:	stb	%2, 1(%9)\n"
			"3:	stb	%3, 2(%9)\n"
			"4:	stb	%4, 3(%9)\n"
			"5:	stb	%5, 4(%9)\n"
			"6:	stb	%6, 5(%9)\n"
			"7:	stb	%7, 6(%9)\n"
			"8:	stb	%8, 7(%9)\n"
			"9:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 9b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 9b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 9b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 9b-4b(%0)\n"
			"	.long	5b - .\n"
			"	ldi	$31, 9b-5b(%0)\n"
			"	.long	6b - .\n"
			"	ldi	$31, 9b-6b(%0)\n"
			"	.long	7b - .\n"
			"	ldi	$31, 9b-7b(%0)\n"
			"	.long	8b - .\n"
			"	ldi	$31, 9b-8b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
			: "r"(vb), "r"(fp[1]), "0"(0));

			if (error)
				goto give_sigsegv;

			return;
		}

	case 0x0f: /* vstd */
		if (!access_is_valid(va, 32))
			goto give_sigsegv;

		sw64_read_simd_fp_m_d(reg, fp);
		if ((unsigned long)va << 61 == 0) {
			__asm__ __volatile__(
			"	bis	%4, %4, %1\n"
			"	bis	%5, %5, %2\n"
			"1:	stl	%1, 0(%3)\n"
			"2:	stl	%2, 8(%3)\n"
			"3:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
			: "r"(va), "r"(fp[0]), "r"(fp[1]), "0"(0));

			if (error)
				goto give_sigsegv;

			vb = ((unsigned long)va)+16;


			__asm__ __volatile__(
			"	bis	%4, %4, %1\n"
			"	bis	%5, %5, %2\n"
			"1:	stl	%1, 0(%3)\n"
			"2:	stl	%2, 8(%3)\n"
			"3:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
			: "r"(vb), "r"(fp[2]), "r"(fp[3]), "0"(0));

			if (error)
				goto give_sigsegv;

			return;
		} else {
			__asm__ __volatile__(
			"	zapnot	%10, 0x1, %1\n"
			"	srl	%10, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%10, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%10, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"	srl	%10, 32, %5\n"
			"	zapnot	%5, 0x1, %5\n"
			"	srl	%10, 40, %6\n"
			"	zapnot	%6, 0x1, %6\n"
			"	srl	%10, 48, %7\n"
			"	zapnot	%7, 0x1, %7\n"
			"	srl	%10, 56, %8\n"
			"	zapnot	%8, 0x1, %8\n"
			"1:	stb	%1, 0(%9)\n"
			"2:	stb	%2, 1(%9)\n"
			"3:	stb	%3, 2(%9)\n"
			"4:	stb	%4, 3(%9)\n"
			"5:	stb	%5, 4(%9)\n"
			"6:	stb	%6, 5(%9)\n"
			"7:	stb	%7, 6(%9)\n"
			"8:	stb	%8, 7(%9)\n"
			"9:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 9b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 9b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 9b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 9b-4b(%0)\n"
			"	.long	5b - .\n"
			"	ldi	$31, 9b-5b(%0)\n"
			"	.long	6b - .\n"
			"	ldi	$31, 9b-6b(%0)\n"
			"	.long	7b - .\n"
			"	ldi	$31, 9b-7b(%0)\n"
			"	.long	8b - .\n"
			"	ldi	$31, 9b-8b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
			: "r"(va), "r"(fp[0]), "0"(0));

			if (error)
				goto give_sigsegv;

			vb = ((unsigned long)va) + 8;

			__asm__ __volatile__(
			"	zapnot	%10, 0x1, %1\n"
			"	srl	%10, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%10, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%10, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"	srl	%10, 32, %5\n"
			"	zapnot	%5, 0x1, %5\n"
			"	srl	%10, 40, %6\n"
			"	zapnot	%6, 0x1, %6\n"
			"	srl	%10, 48, %7\n"
			"	zapnot	%7, 0x1, %7\n"
			"	srl	%10, 56, %8\n"
			"	zapnot	%8, 0x1, %8\n"
			"1:	stb	%1, 0(%9)\n"
			"2:	stb	%2, 1(%9)\n"
			"3:	stb	%3, 2(%9)\n"
			"4:	stb	%4, 3(%9)\n"
			"5:	stb	%5, 4(%9)\n"
			"6:	stb	%6, 5(%9)\n"
			"7:	stb	%7, 6(%9)\n"
			"8:	stb	%8, 7(%9)\n"
			"9:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 9b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 9b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 9b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 9b-4b(%0)\n"
			"	.long	5b - .\n"
			"	ldi	$31, 9b-5b(%0)\n"
			"	.long	6b - .\n"
			"	ldi	$31, 9b-6b(%0)\n"
			"	.long	7b - .\n"
			"	ldi	$31, 9b-7b(%0)\n"
			"	.long	8b - .\n"
			"	ldi	$31, 9b-8b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
			: "r"(vb), "r"(fp[1]), "0"(0));

			if (error)
				goto give_sigsegv;

			vb = vb + 8;

			__asm__ __volatile__(
			"	zapnot	%10, 0x1, %1\n"
			"	srl	%10, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%10, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%10, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"	srl	%10, 32, %5\n"
			"	zapnot	%5, 0x1, %5\n"
			"	srl	%10, 40, %6\n"
			"	zapnot	%6, 0x1, %6\n"
			"	srl	%10, 48, %7\n"
			"	zapnot	%7, 0x1, %7\n"
			"	srl	%10, 56, %8\n"
			"	zapnot	%8, 0x1, %8\n"
			"1:	stb	%1, 0(%9)\n"
			"2:	stb	%2, 1(%9)\n"
			"3:	stb	%3, 2(%9)\n"
			"4:	stb	%4, 3(%9)\n"
			"5:	stb	%5, 4(%9)\n"
			"6:	stb	%6, 5(%9)\n"
			"7:	stb	%7, 6(%9)\n"
			"8:	stb	%8, 7(%9)\n"
			"9:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 9b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 9b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 9b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 9b-4b(%0)\n"
			"	.long	5b - .\n"
			"	ldi	$31, 9b-5b(%0)\n"
			"	.long	6b - .\n"
			"	ldi	$31, 9b-6b(%0)\n"
			"	.long	7b - .\n"
			"	ldi	$31, 9b-7b(%0)\n"
			"	.long	8b - .\n"
			"	ldi	$31, 9b-8b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
			: "r"(vb), "r"(fp[2]), "0"(0));

			if (error)
				goto give_sigsegv;

			vb = vb + 8;

			__asm__ __volatile__(
			"	zapnot	%10, 0x1, %1\n"
			"	srl	%10, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%10, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%10, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"	srl	%10, 32, %5\n"
			"	zapnot	%5, 0x1, %5\n"
			"	srl	%10, 40, %6\n"
			"	zapnot	%6, 0x1, %6\n"
			"	srl	%10, 48, %7\n"
			"	zapnot	%7, 0x1, %7\n"
			"	srl	%10, 56, %8\n"
			"	zapnot	%8, 0x1, %8\n"
			"1:	stb	%1, 0(%9)\n"
			"2:	stb	%2, 1(%9)\n"
			"3:	stb	%3, 2(%9)\n"
			"4:	stb	%4, 3(%9)\n"
			"5:	stb	%5, 4(%9)\n"
			"6:	stb	%6, 5(%9)\n"
			"7:	stb	%7, 6(%9)\n"
			"8:	stb	%8, 7(%9)\n"
			"9:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 9b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 9b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 9b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 9b-4b(%0)\n"
			"	.long	5b - .\n"
			"	ldi	$31, 9b-5b(%0)\n"
			"	.long	6b - .\n"
			"	ldi	$31, 9b-6b(%0)\n"
			"	.long	7b - .\n"
			"	ldi	$31, 9b-7b(%0)\n"
			"	.long	8b - .\n"
			"	ldi	$31, 9b-8b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
			: "r"(vb), "r"(fp[3]), "0"(0));

			if (error)
				goto give_sigsegv;

			return;
		}
	}
	switch (opcode) {
	case 0x1e:
		rb = (instr >> 16) & 0x1f;
		disp = instr & 0xfff;
		disp = (disp << 52) >> 52;

		switch (fncode) {
		case 0x1: /* ldhu_a */
			__asm__ __volatile__(
			"1:	ldl_u	%1, 0(%3)\n"
			"2:	ldl_u	%2, 1(%3)\n"
			"	extlh	%1, %3, %1\n"
			"	exthh	%2, %3, %2\n"
			"3:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
			: "r"(va), "0"(0));
			if (error)
				goto give_sigsegv;
			*reg_addr = tmp1 | tmp2;
			regs->regs[rb] = regs->regs[rb] + disp;
			break;

		case 0x2: /* ldw_a */
			__asm__ __volatile__(
			"1:	ldl_u	%1, 0(%3)\n"
			"2:	ldl_u	%2, 3(%3)\n"
			"	extlw	%1, %3, %1\n"
			"	exthw	%2, %3, %2\n"
			"3:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
			: "r"(va), "0"(0));
			if (error)
				goto give_sigsegv;
			*reg_addr = (int)(tmp1 | tmp2);
			regs->regs[rb] = regs->regs[rb] + disp;
			break;

		case 0x3: /* ldl_a */
			__asm__ __volatile__(
			"1:	ldl_u	%1, 0(%3)\n"
			"2:	ldl_u	%2, 7(%3)\n"
			"	extll	%1, %3, %1\n"
			"	exthl	%2, %3, %2\n"
			"3:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
			: "r"(va), "0"(0));
			if (error)
				goto give_sigsegv;
			*reg_addr = tmp1 | tmp2;
			regs->regs[rb] = regs->regs[rb] + disp;
			break;

		case 0x4: /* flds_a */
			__asm__ __volatile__(
			"1:	ldl_u	%1, 0(%3)\n"
			"2:	ldl_u	%2, 3(%3)\n"
			"	extlw	%1, %3, %1\n"
			"	exthw	%2, %3, %2\n"
			"3:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
			: "r"(va), "0"(0));
			if (error)
				goto give_sigsegv;
			sw64_write_fp_reg_s(reg, tmp1 | tmp2);
			regs->regs[rb] = regs->regs[rb] + disp;
			break;

		case 0x5: /* fldd_a */
			__asm__ __volatile__(
			"1:	ldl_u	%1, 0(%3)\n"
			"2:	ldl_u	%2, 7(%3)\n"
			"	extll	%1, %3, %1\n"
			"	exthl	%2, %3, %2\n"
			"3:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
			: "r"(va), "0"(0));
			if (error)
				goto give_sigsegv;
			sw64_write_fp_reg(reg, tmp1 | tmp2);
			regs->regs[rb] = regs->regs[rb] + disp;
			break;

		case 0x7: /* sth_a */
			if (!access_is_valid(va, 2))
				goto give_sigsegv;

			__asm__ __volatile__(
			"	zap	%6, 2, %1\n"
			"	srl	%6, 8, %2\n"
			"1:	stb	%1, 0x0(%5)\n"
			"2:	stb	%2, 0x1(%5)\n"
			"3:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%2, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%1, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2),
			  "=&r"(tmp3), "=&r"(tmp4)
			: "r"(va), "r"(*reg_addr), "0"(0));

			if (error)
				goto give_sigsegv;
			regs->regs[rb] = regs->regs[rb] + disp;
			break;

		case 0xa: /* fsts_a */
			fake_reg = sw64_read_fp_reg_s(reg);
			regs->regs[rb] = regs->regs[rb] + disp;
			break;
			/* fallthrough; */
		case 0x8: /* stw_a */
			if (!access_is_valid(va, 4))
				goto give_sigsegv;

			__asm__ __volatile__(
			"	zapnot	%6, 0x1, %1\n"
			"	srl	%6, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%6, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%6, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"1:	stb  %1, 0x0(%5)\n"
			"2:	stb  %2, 0x1(%5)\n"
			"3:	stb  %3, 0x2(%5)\n"
			"4:	stb  %4, 0x3(%5)\n"
			"5:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	$31, 5b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 5b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 5b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 5b-4b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2),
			  "=&r"(tmp3), "=&r"(tmp4)
			: "r"(va), "r"(*reg_addr), "0"(0));

			if (error)
				goto give_sigsegv;
			regs->regs[rb] = regs->regs[rb] + disp;
			break;

		case 0xb: /* fstd_a */
			fake_reg = sw64_read_fp_reg(reg);
			regs->regs[rb] = regs->regs[rb] + disp;
			break;
			/* fallthrough; */
		case 0x9: /* stl_a */
			if (!access_is_valid(va, 8))
				goto give_sigsegv;

			__asm__ __volatile__(
			"	zapnot	%10, 0x1, %1\n"
			"	srl	%10, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%10, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%10, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"	srl	%10, 32, %5\n"
			"	zapnot	%5, 0x1, %5\n"
			"	srl	%10, 40, %6\n"
			"	zapnot	%6, 0x1, %6\n"
			"	srl	%10, 48, %7\n"
			"	zapnot	%7, 0x1, %7\n"
			"	srl	%10, 56, %8\n"
			"	zapnot	%8, 0x1, %8\n"
			"1:	stb	%1, 0(%9)\n"
			"2:	stb	%2, 1(%9)\n"
			"3:	stb	%3, 2(%9)\n"
			"4:	stb	%4, 3(%9)\n"
			"5:	stb	%5, 4(%9)\n"
			"6:	stb	%6, 5(%9)\n"
			"7:	stb	%7, 6(%9)\n"
			"8:	stb	%8, 7(%9)\n"
			"9:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 9b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 9b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 9b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 9b-4b(%0)\n"
			"	.long	5b - .\n"
			"	ldi	$31, 9b-5b(%0)\n"
			"	.long	6b - .\n"
			"	ldi	$31, 9b-6b(%0)\n"
			"	.long	7b - .\n"
			"	ldi	$31, 9b-7b(%0)\n"
			"	.long	8b - .\n"
			"	ldi	$31, 9b-8b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
			: "r"(va), "r"(*reg_addr), "0"(0));

			if (error)
				goto give_sigsegv;
			regs->regs[rb] = regs->regs[rb] + disp;
			break;
		}
		break;

	case 0x21: /* ldhu */
		__asm__ __volatile__(
		"1:	ldl_u	%1, 0(%3)\n"
		"2:	ldl_u	%2, 1(%3)\n"
		"	extlh	%1, %3, %1\n"
		"	exthh	%2, %3, %2\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "0"(0));
		if (error)
			goto give_sigsegv;
		*reg_addr = tmp1 | tmp2;
		break;

	case 0x26: /* flds */
		__asm__ __volatile__(
		"1:	ldl_u	%1, 0(%3)\n"
		"2:	ldl_u	%2, 3(%3)\n"
		"	extlw	%1, %3, %1\n"
		"	exthw	%2, %3, %2\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "0"(0));
		if (error)
			goto give_sigsegv;
		sw64_write_fp_reg_s(reg, tmp1 | tmp2);
		return;

	case 0x27: /* fldd */
		__asm__ __volatile__(
		"1:	ldl_u	%1, 0(%3)\n"
		"2:	ldl_u	%2, 7(%3)\n"
		"	extll	%1, %3, %1\n"
		"	exthl	%2, %3, %2\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "0"(0));
		if (error)
			goto give_sigsegv;
		sw64_write_fp_reg(reg, tmp1 | tmp2);
		return;

	case 0x22: /* ldw */
		__asm__ __volatile__(
		"1:	ldl_u	%1, 0(%3)\n"
		"2:	ldl_u	%2, 3(%3)\n"
		"	extlw	%1, %3, %1\n"
		"	exthw	%2, %3, %2\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "0"(0));
		if (error)
			goto give_sigsegv;
		*reg_addr = (int)(tmp1 | tmp2);
		break;

	case 0x23: /* ldl */
		__asm__ __volatile__(
		"1:	ldl_u	%1, 0(%3)\n"
		"2:	ldl_u	%2, 7(%3)\n"
		"	extll	%1, %3, %1\n"
		"	exthl	%2, %3, %2\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "0"(0));
		if (error)
			goto give_sigsegv;
		*reg_addr = tmp1 | tmp2;
		break;

	/* Note that the store sequences do not indicate that they change
	 * memory because it _should_ be affecting nothing in this context.
	 * (Otherwise we have other, much larger, problems.)
	 */
	case 0x29: /* sth with stb */
		if (!access_is_valid(va, 2))
			goto give_sigsegv;

		__asm__ __volatile__(
		"	zap	%6, 2, %1\n"
		"	srl	%6, 8, %2\n"
		"1:	stb	%1, 0x0(%5)\n"
		"2:	stb	%2, 0x1(%5)\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%2, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%1, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2),
		  "=&r"(tmp3), "=&r"(tmp4)
		: "r"(va), "r"(*reg_addr), "0"(0));

		if (error)
			goto give_sigsegv;
		return;

	case 0x2e: /* fsts*/
		fake_reg = sw64_read_fp_reg_s(reg);
		fallthrough;

	case 0x2a: /* stw with stb*/
		if (!access_is_valid(va, 4))
			goto give_sigsegv;

		__asm__ __volatile__(
		"	zapnot	%6, 0x1, %1\n"
		"	srl	%6, 8, %2\n"
		"	zapnot	%2, 0x1, %2\n"
		"	srl	%6, 16, %3\n"
		"	zapnot	%3, 0x1, %3\n"
		"	srl	%6, 24, %4\n"
		"	zapnot	%4, 0x1, %4\n"
		"1:	stb  %1, 0x0(%5)\n"
		"2:	stb  %2, 0x1(%5)\n"
		"3:	stb  %3, 0x2(%5)\n"
		"4:	stb  %4, 0x3(%5)\n"
		"5:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	$31, 5b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	$31, 5b-2b(%0)\n"
		"	.long	3b - .\n"
		"	ldi	$31, 5b-3b(%0)\n"
		"	.long	4b - .\n"
		"	ldi	$31, 5b-4b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2),
		  "=&r"(tmp3), "=&r"(tmp4)
		: "r"(va), "r"(*reg_addr), "0"(0));

		if (error)
			goto give_sigsegv;
		return;

	case 0x2f: /* fstd */
		fake_reg = sw64_read_fp_reg(reg);
		fallthrough;

	case 0x2b: /* stl */
		if (!access_is_valid(va, 8))
			goto give_sigsegv;

		__asm__ __volatile__(
		"	zapnot	%10, 0x1, %1\n"
		"	srl	%10, 8, %2\n"
		"	zapnot	%2, 0x1, %2\n"
		"	srl	%10, 16, %3\n"
		"	zapnot	%3, 0x1, %3\n"
		"	srl	%10, 24, %4\n"
		"	zapnot	%4, 0x1, %4\n"
		"	srl	%10, 32, %5\n"
		"	zapnot	%5, 0x1, %5\n"
		"	srl	%10, 40, %6\n"
		"	zapnot	%6, 0x1, %6\n"
		"	srl	%10, 48, %7\n"
		"	zapnot	%7, 0x1, %7\n"
		"	srl	%10, 56, %8\n"
		"	zapnot	%8, 0x1, %8\n"
		"1:	stb	%1, 0(%9)\n"
		"2:	stb	%2, 1(%9)\n"
		"3:	stb	%3, 2(%9)\n"
		"4:	stb	%4, 3(%9)\n"
		"5:	stb	%5, 4(%9)\n"
		"6:	stb	%6, 5(%9)\n"
		"7:	stb	%7, 6(%9)\n"
		"8:	stb	%8, 7(%9)\n"
		"9:\n"
		".section __ex_table, \"a\"\n\t"
		"	.long	1b - .\n"
		"	ldi	$31, 9b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	$31, 9b-2b(%0)\n"
		"	.long	3b - .\n"
		"	ldi	$31, 9b-3b(%0)\n"
		"	.long	4b - .\n"
		"	ldi	$31, 9b-4b(%0)\n"
		"	.long	5b - .\n"
		"	ldi	$31, 9b-5b(%0)\n"
		"	.long	6b - .\n"
		"	ldi	$31, 9b-6b(%0)\n"
		"	.long	7b - .\n"
		"	ldi	$31, 9b-7b(%0)\n"
		"	.long	8b - .\n"
		"	ldi	$31, 9b-8b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
		  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
		: "r"(va), "r"(*reg_addr), "0"(0));

		if (error)
			goto give_sigsegv;
		return;

	default:
		/* What instruction were you trying to use, exactly? */
		goto give_sigbus;
	}

	return;

give_sigsegv:
	regs->pc -= 4;  /* make pc point to faulting insn */

	/* We need to replicate some of the logic in mm/fault.c,
	 * since we don't have access to the fault code in the
	 * exception handling return path.
	 */
	if ((unsigned long)va >= TASK_SIZE)
		si_code = SEGV_ACCERR;
	else {
		struct mm_struct *mm = current->mm;

		down_read(&mm->mmap_lock);
		if (find_vma(mm, (unsigned long)va))
			si_code = SEGV_ACCERR;
		else
			si_code = SEGV_MAPERR;
		up_read(&mm->mmap_lock);
	}
	force_sig_fault(SIGSEGV, si_code, va);
	return;

give_sigbus:
	regs->pc -= 4;
	force_sig_fault(SIGBUS, BUS_ADRALN, va);
}
