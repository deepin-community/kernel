/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_FUTEX_H
#define _ASM_SW64_FUTEX_H

#ifdef __KERNEL__

#include <linux/futex.h>
#include <linux/uaccess.h>
#include <asm/errno.h>
#include <asm/barrier.h>

#ifdef CONFIG_SUBARCH_C3B

#define __futex_atomic_op(insn, ret, oldval, uaddr, oparg, tmp)	\
	__asm__ __volatile__(					\
	"1:	lldw	%0, 0(%3)\n"				\
	"	ldi	%2, 1\n"				\
	"	wr_f	%2\n"					\
		insn						\
	"2:	lstw	%1, 0(%3)\n"				\
	"	rd_f	%1\n"					\
	"	beq	%1, 4f\n"				\
	"	bis	$31, $31, %1\n"				\
	"3:	.subsection 2\n"				\
	"4:	br	1b\n"					\
	"	.previous\n"					\
	"	.section __ex_table, \"a\"\n"			\
	"	.long	1b-.\n"					\
	"	ldi	$31, 3b-1b(%1)\n"			\
	"	.long	2b-.\n"					\
	"	ldi	$31, 3b-2b(%1)\n"			\
	"	.previous\n"					\
	:	"=&r" (oldval), "=&r"(ret), "=&r"(tmp)		\
	:	"r" (uaddr), "r"(oparg)				\
	:	"memory")

static inline int
futex_atomic_cmpxchg_inatomic(u32 *uval, u32 __user *uaddr,
			      u32 oldval, u32 newval)
{
	int ret = 0, cmp;
	u32 prev, tmp;

	if (!access_ok(uaddr, sizeof(u32)))
		return -EFAULT;

	__asm__ __volatile__ (
	"1:	lldw	%1, 0(%4)\n"
	"	cmpeq	%1, %5, %2\n"
	"	wr_f	%2\n"
	"	bis	$31, %6, %3\n"
	"2:	lstw	%3, 0(%4)\n"
	"	rd_f	%3\n"
	"	beq	%2, 3f\n"
	"	beq	%3, 4f\n"
	"3:	.subsection 2\n"
	"4:	br	1b\n"
	"	.previous\n"
	"	.section __ex_table, \"a\"\n"
	"	.long	1b-.\n"
	"	ldi	$31, 3b-1b(%0)\n"
	"	.long	2b-.\n"
	"	ldi	$31, 3b-2b(%0)\n"
	"	.previous\n"
	:	"+r"(ret), "=&r"(prev), "=&r"(cmp), "=&r"(tmp)
	:	"r"(uaddr), "r"((long)(int)oldval), "r"(newval)
	:	"memory");

	*uval = prev;
	return ret;
}
#else /* !CONFIG_SUBARCH_C3B */

#define __futex_atomic_op(insn, ret, oldval, uaddr, oparg, tmp)	\
	__asm__ __volatile__(					\
	"1:	lldw	%0, 0(%3)\n"				\
		insn						\
	"2:	lstw	%1, 0(%3)\n"				\
	"	beq	%1, 4f\n"				\
	"	bis	$31, $31, %1\n"				\
	"3:	.subsection 2\n"				\
	"4:	lbr	1b\n"					\
	"	.previous\n"					\
	"	.section __ex_table, \"a\"\n"			\
	"	.long	1b-.\n"					\
	"	ldi	$31, 3b-1b(%1)\n"			\
	"	.long	2b-.\n"					\
	"	ldi	$31, 3b-2b(%1)\n"			\
	"	.previous\n"					\
	:	"=&r" (oldval), "=&r"(ret), "=&r"(tmp)		\
	:	"r" (uaddr), "r"(oparg)				\
	:	"memory")


static inline int
futex_atomic_cmpxchg_inatomic(u32 *uval, u32 __user *uaddr,
			      u32 oldval, u32 newval)
{
	int ret = 0, cmp;
	u32 prev, tmp;

	if (!access_ok(uaddr, sizeof(u32)))
		return -EFAULT;

	__asm__ __volatile__ (
	"1:	lldw	%1, 0(%4)\n"
	"	cmpeq	%1, %5, %2\n"
	"	beq	%2, 3f\n"
	"	bis	$31, %6, %3\n"
	"2:	lstw	%3, 0(%4)\n"
	"	beq	%3, 4f\n"
	"3:	.subsection 2\n"
	"4:	lbr	1b\n"
	"	.previous\n"
	"	.section __ex_table, \"a\"\n"
	"	.long	1b-.\n"
	"	ldi	$31, 3b-1b(%0)\n"
	"	.long	2b-.\n"
	"	ldi	$31, 3b-2b(%0)\n"
	"	.previous\n"
	:	"+r"(ret), "=&r"(prev), "=&r"(cmp), "=&r"(tmp)
	:	"r"(uaddr), "r"((long)(int)oldval), "r"(newval)
	:	"memory");

	*uval = prev;
	return ret;
}
#endif /* CONFIG_SUBARCH_C3B */

static inline int arch_futex_atomic_op_inuser(int op, int oparg, int *oval,
					      u32 __user *uaddr)
{
	int oldval = 0, ret;
	unsigned long tmp;

	pagefault_disable();

	switch (op) {
	case FUTEX_OP_SET:
		__futex_atomic_op("mov %4, %1\n", ret, oldval, uaddr, oparg, tmp);
		break;
	case FUTEX_OP_ADD:
		__futex_atomic_op("addw %0, %4, %1\n", ret, oldval, uaddr, oparg, tmp);
		break;
	case FUTEX_OP_OR:
		__futex_atomic_op("or %0, %4, %1\n", ret, oldval, uaddr, oparg, tmp);
		break;
	case FUTEX_OP_ANDN:
		__futex_atomic_op("andnot %0, %4, %1\n", ret, oldval, uaddr, oparg, tmp);
		break;
	case FUTEX_OP_XOR:
		__futex_atomic_op("xor %0, %4, %1\n", ret, oldval, uaddr, oparg, tmp);
		break;
	default:
		ret = -ENOSYS;
	}

	pagefault_enable();

	if (!ret)
		*oval = oldval;

	return ret;
}

#endif /* __KERNEL__ */

#endif /* _ASM_SW64_FUTEX_H */
