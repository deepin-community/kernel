// SPDX-License-Identifier: GPL-2.0
/*
 * arch/sw_64/kernel/traps.c
 *
 * (C) Copyright 1994 Linus Torvalds
 */

/*
 * This file initializes the trap entry points
 */

#include <linux/extable.h>
#include <linux/perf_event.h>
#include <linux/kdebug.h>
#include <linux/sched.h>
#include <linux/kexec.h>
#include <linux/kallsyms.h>
#include <linux/sched/task_stack.h>
#include <linux/sched/debug.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/syscalls.h>

#include <asm/gentrap.h>
#include <asm/mmu_context.h>
#include <asm/fpu.h>
#include <asm/kprobes.h>
#include <asm/uprobes.h>
#include <asm/stacktrace.h>
#include <asm/processor.h>
#include <asm/ptrace.h>
#include <asm/debug.h>
#include <asm/efi.h>
#include <asm/syscall.h>
#include <asm/unistd.h>

#include "proto.h"

enum SW64_IF_TYPES {
	IF_BREAKPOINT = 0,
	IF_RESERVED,
	IF_GENTRAP,
	IF_FEN,
	IF_OPDEC,
	IF_SIMDEMU,
};

void show_regs(struct pt_regs *regs)
{
	show_regs_print_info(KERN_DEFAULT);

	printk(KERN_DEFAULT "pc = [<%016lx>]  ra = [<%016lx>]  ps = %04lx    %s\n",
	       regs->pc, regs->regs[26], regs->ps, print_tainted());
	printk(KERN_DEFAULT "pc is at %pSR\n", (void *)regs->pc);
	printk(KERN_DEFAULT "ra is at %pSR\n", (void *)regs->regs[26]);
	printk(KERN_DEFAULT "v0 = %016lx  t0 = %016lx  t1 = %016lx\n",
	       regs->regs[0], regs->regs[1], regs->regs[2]);
	printk(KERN_DEFAULT "t2 = %016lx  t3 = %016lx  t4 = %016lx\n",
	       regs->regs[3], regs->regs[4], regs->regs[5]);
	printk(KERN_DEFAULT "t5 = %016lx  t6 = %016lx  t7 = %016lx\n",
	       regs->regs[6], regs->regs[7], regs->regs[8]);

	printk(KERN_DEFAULT "s0 = %016lx  s1 = %016lx  s2 = %016lx\n",
	       regs->regs[9], regs->regs[10], regs->regs[11]);
	printk(KERN_DEFAULT "s3 = %016lx  s4 = %016lx  s5 = %016lx\n",
	       regs->regs[12], regs->regs[13], regs->regs[14]);
	printk(KERN_DEFAULT "s6 = %016lx\n",
	       regs->regs[15]);

	printk(KERN_DEFAULT "a0 = %016lx  a1 = %016lx  a2 = %016lx\n",
	       regs->regs[16], regs->regs[17], regs->regs[18]);
	printk(KERN_DEFAULT "a3 = %016lx  a4 = %016lx  a5 = %016lx\n",
	       regs->regs[19], regs->regs[20], regs->regs[21]);
	printk(KERN_DEFAULT "t8 = %016lx  t9 = %016lx  t10 = %016lx\n",
	       regs->regs[22], regs->regs[23], regs->regs[24]);
	printk(KERN_DEFAULT "t11= %016lx  pv = %016lx  at = %016lx\n",
	       regs->regs[25], regs->regs[27], regs->regs[28]);
	printk(KERN_DEFAULT "gp = %016lx  sp = %016lx\n", regs->regs[29], regs->regs[30]);
}

static void show_code(unsigned int *pc)
{
	long i;
	unsigned int insn;

	printk(KERN_DEFAULT "Code:");
	for (i = -6; i < 2; i++) {
		if (__get_user(insn, (unsigned int __user *)pc + i))
			break;
		printk(KERN_DEFAULT "%c%08x%c", i ? ' ' : '<', insn, i ? ' ' : '>');
	}
	printk(KERN_DEFAULT "\n");
}

static DEFINE_SPINLOCK(die_lock);

void die(char *str, struct pt_regs *regs, long err)
{
	static int die_counter;
	unsigned long flags;
	int ret;

	oops_enter();

	spin_lock_irqsave(&die_lock, flags);
	console_verbose();
	bust_spinlocks(1);

	pr_emerg("%s [#%d]\n", str, ++die_counter);

	ret = notify_die(DIE_OOPS, str, regs, err, 0, SIGSEGV);

	print_modules();
	show_regs(regs);
	show_code((unsigned int *)regs->pc);
	show_stack(current, NULL, KERN_EMERG);

	bust_spinlocks(0);
	add_taint(TAINT_DIE, LOCKDEP_NOW_UNRELIABLE);
	spin_unlock_irqrestore(&die_lock, flags);
	oops_exit();

	if (kexec_should_crash(current))
		crash_kexec(regs);
	if (in_interrupt())
		panic("Fatal exception in interrupt");
	if (panic_on_oops)
		panic("Fatal exception");

	if (ret != NOTIFY_STOP)
		make_task_dead(SIGSEGV);
}

#ifndef CONFIG_MATHEMU
static long dummy_emul(void)
{
	return 0;
}

long (*sw64_fp_emul_imprecise)(struct pt_regs *regs, unsigned long writemask) = (void *)dummy_emul;
EXPORT_SYMBOL_GPL(sw64_fp_emul_imprecise);

long (*sw64_fp_emul)(unsigned long pc) = (void *)dummy_emul;
EXPORT_SYMBOL_GPL(sw64_fp_emul);
#else
extern long sw64_fp_emul_imprecise(struct pt_regs *regs, unsigned long writemask);
extern long sw64_fp_emul(unsigned long pc);
#endif

asmlinkage void
do_entArith(unsigned long exc_sum, unsigned long write_mask,
		struct pt_regs *regs)
{
	long si_code = FPE_FLTINV;

#ifndef CONFIG_SUBARCH_C3B
	/* integer divide by zero */
	if (exc_sum & EXC_SUM_DZE_INT)
		si_code = FPE_INTDIV;
	/* integer overflow */
	else if (exc_sum & EXC_SUM_OVI)
		si_code = FPE_INTOVF;
	/* floating point invalid operation */
	else if (exc_sum & EXC_SUM_INV)
		si_code = FPE_FLTINV;
	/* floating point divide by zero */
	else if (exc_sum & EXC_SUM_DZE)
		si_code = FPE_FLTDIV;
	/* floating point overflow */
	else if (exc_sum & EXC_SUM_OVF)
		si_code = FPE_FLTOVF;
	/* floating point underflow */
	else if (exc_sum & EXC_SUM_UNF)
		si_code = FPE_FLTUND;
	/* floating point inexact result */
	else if (exc_sum & EXC_SUM_INE)
		si_code = FPE_FLTRES;
	/* denormalized operand */
	else if (exc_sum & EXC_SUM_DNO)
		si_code = FPE_FLTUND;
	/* undiagnosed floating-point exception */
	else
		si_code = FPE_FLTUNK;
#endif

	if ((exc_sum & EXC_SUM_FP_STATUS_ALL) && (exc_sum & EXC_SUM_SWC)) {
		/* Software-completion summary bit is set, so try to
		 * emulate the instruction.  If the processor supports
		 * precise exceptions, we don't have to search.
		 */
		si_code = sw64_fp_emul(regs->pc - 4);
		if (si_code == 0)
			return;
	}

	if (!user_mode(regs))
		die("Arithmetic fault", regs, 0);

	force_sig_fault(SIGFPE, si_code, (void __user *)regs->pc);
}

void simd_emulate(unsigned int inst, unsigned long va)
{
	unsigned long *fp;
	int instr_opc, reg;

	instr_opc = (inst >> 26) & 0x3f;
	reg = (inst >> 21) & 0x1f;
	fp = (unsigned long *) va;

	switch (instr_opc) {
	case 0x0d: /* vldd */
		sw64_write_simd_fp_reg_d(reg, fp[0], fp[1], fp[2], fp[3]);
		return;

	case 0x0f: /* vstd */
		sw64_read_simd_fp_m_d(reg, fp);
		return;
	}
}

static int try_fix_rd_f(unsigned int inst, struct pt_regs *regs)
{
	int copied;
	unsigned int prev_inst, new_inst;
	unsigned int ra, prev_ra;

	/* not rd_f */
	if ((inst & 0xfc00ffffU) != 0x18001000)
		return -1;

	get_user(prev_inst, (__u32 *)(regs->pc - 8));
	if ((prev_inst & 0xfc00e000U) == 0x20008000) { /* lstw/lstl */
		ra = (inst >> 21) & 0x1f;
		prev_ra = (prev_inst >> 21) & 0x1f;
		/* ldi ra, 0(prev_ra) */
		new_inst = (0x3e << 26) | (ra << 21) | (prev_ra << 16);
		copied = access_process_vm(current, regs->pc - 4, &new_inst,
				sizeof(unsigned int), FOLL_FORCE | FOLL_WRITE);
		if (copied != sizeof(unsigned int))
			return -1;
		regs->pc -= 4;
		return 0;
	}
	return -1;
}

/*
 * BPT/GENTRAP/OPDEC make regs->pc = exc_pc + 4. debugger should
 * do something necessary to handle it correctly.
 */
asmlinkage void
do_entIF(unsigned long inst_type, unsigned long va, struct pt_regs *regs)
{
	int signo, code;
	unsigned int inst, type;

	type = inst_type & 0xffffffff;
	inst = inst_type >> 32;

	if (type == IF_SIMDEMU) {
		simd_emulate(inst, va);
		return;
	}

	if (!user_mode(regs) && type != IF_OPDEC) {
		if (type == IF_BREAKPOINT) {
			/* support kgdb */
			notify_die(0, "kgdb trap", regs, 0, 0, SIGTRAP);
			return;
		}
		die((type == IF_RESERVED ? "Kernel Bug" : "Instruction fault"),
				regs, type);
	}

	switch (type) {
	case IF_BREAKPOINT: /* gdb do pc-4 for sigtrap */
		if (ptrace_cancel_bpt(current))
			regs->pc -= 4;
		force_sig_fault(SIGTRAP, TRAP_BRKPT, (void __user *)regs->pc);
		return;

	case IF_GENTRAP:
		regs->pc -= 4;
		switch ((long)regs->regs[16]) {
		case GEN_INTOVF:
			signo = SIGFPE;
			code = FPE_INTOVF;
			break;
		case GEN_INTDIV:
			signo = SIGFPE;
			code = FPE_INTDIV;
			break;
		case GEN_FLTOVF:
			signo = SIGFPE;
			code = FPE_FLTOVF;
			break;
		case GEN_FLTDIV:
			signo = SIGFPE;
			code = FPE_FLTDIV;
			break;
		case GEN_FLTUND:
			signo = SIGFPE;
			code = FPE_FLTUND;
			break;
		case GEN_FLTINV:
			signo = SIGFPE;
			code = FPE_FLTINV;
			break;
		case GEN_FLTINE:
			signo = SIGFPE;
			code = FPE_FLTRES;
			break;
		case GEN_ROPRAND:
			signo = SIGFPE;
			code = FPE_FLTUNK;
			break;

		case GEN_DECOVF:
		case GEN_DECDIV:
		case GEN_DECINV:
		case GEN_ASSERTERR:
		case GEN_NULPTRERR:
		case GEN_STKOVF:
		case GEN_STRLENERR:
		case GEN_SUBSTRERR:
		case GEN_RANGERR:
		case GEN_SUBRNG:
		case GEN_SUBRNG1:
		case GEN_SUBRNG2:
		case GEN_SUBRNG3:
		case GEN_SUBRNG4:
		case GEN_SUBRNG5:
		case GEN_SUBRNG6:
		case GEN_SUBRNG7:
		default:
			regs->pc += 4;
			signo = SIGTRAP;
			code = TRAP_UNK;
			break;
		}

		force_sig_fault(signo, code, (void __user *)regs->pc);
		return;

	case IF_FEN:
		fpu_enable();
		return;

	case IF_OPDEC:
		if (try_fix_rd_f(inst, regs) == 0)
			return;
		switch (inst) {
#ifdef CONFIG_KPROBES
		case BREAK_KPROBE:
			if (notify_die(DIE_BREAK, "kprobe", regs, 0, 0, SIGTRAP) == NOTIFY_STOP)
				return;
			break;
		case BREAK_KPROBE_SS:
			if (notify_die(DIE_SSTEPBP, "single_step", regs, 0, 0, SIGTRAP) == NOTIFY_STOP)
				return;
			break;
#endif
#ifdef CONFIG_UPROBES
		case UPROBE_BRK_UPROBE:
			if (notify_die(DIE_UPROBE, "uprobe", regs, 0, 0, SIGTRAP) == NOTIFY_STOP)
				return;
			break;
		case UPROBE_BRK_UPROBE_XOL:
			if (notify_die(DIE_UPROBE_XOL, "uprobe_xol", regs, 0, 0, SIGTRAP) == NOTIFY_STOP)
				return;
#endif
		}

		if (user_mode(regs))
			regs->pc -= 4;
		else
			die("Instruction fault", regs, type);
		break;

	default: /* unexpected instruction-fault type */
		regs->pc -= 4;
		break;
	}

	force_sig_fault(SIGILL, ILL_ILLOPC, (void __user *)regs->pc);
}

#define OP_INT_MASK	(1L << 0x22 | 1L << 0x2a | /* ldw stw */	\
			 1L << 0x23 | 1L << 0x2b | /* ldl stl */	\
			 1L << 0x21 | 1L << 0x29 | /* ldhu sth */	\
			 1L << 0x20 | 1L << 0x28)  /* ldbu stb */

#define FN_INT_MASK	(1L << 0x0 | 1L << 0x6 |   /* ldbu_a stb_a */	\
			 1L << 0x1 | 1L << 0x7 |   /* ldhu_a sth_a */	\
			 1L << 0x2 | 1L << 0x8 |   /* ldw_a stw_a */	\
			 1L << 0x3 | 1L << 0x9)    /* ldl_a stl_a */

asmlinkage void
do_entUna(void *va, unsigned long opcode, unsigned long reg,
	  struct pt_regs *regs)
{
	long error, disp;
	unsigned int insn, fncode, rb;
	unsigned long tmp, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, vb;
	unsigned long fp[4];
	unsigned long fake_reg, *reg_addr = &fake_reg;
	unsigned long pc = regs->pc - 4;

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
asmlinkage void
do_entUnaUser(void __user *va, unsigned long opcode,
	      unsigned long reg, struct pt_regs *regs)
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

asmlinkage void do_entSys(struct pt_regs *regs)
{
	long ret = -ENOSYS;
	unsigned long nr;
	unsigned long ti_flags = current_thread_info()->flags;

	regs->orig_r0 = regs->regs[0];
	regs->orig_r19 = regs->regs[19];
	nr = regs->regs[0];

	if (ti_flags & _TIF_SYSCALL_WORK) {
		nr = syscall_trace_enter();
		if (nr == NO_SYSCALL)
			goto syscall_out;
		regs->orig_r0 = regs->regs[0];
		regs->orig_r19 = regs->regs[19];
	}

	if (nr < __NR_syscalls) {
		syscall_fn_t syscall_fn = sys_call_table[nr];

		ret = syscall_fn(regs->regs[16], regs->regs[17], regs->regs[18],
				regs->regs[19], regs->regs[20], regs->regs[21]);
	}

	if ((nr != __NR_sigreturn) && (nr != __NR_rt_sigreturn)) {
		if (likely((ret >= 0) || regs->orig_r0 == NO_SYSCALL))
			syscall_set_return_value(current, regs, 0, ret);
		else
			syscall_set_return_value(current, regs, ret, 0);
	}

syscall_out:
	rseq_syscall(regs);

	if (ti_flags & _TIF_SYSCALL_WORK)
		syscall_trace_leave();
}

struct nmi_ctx {
	unsigned long csr_sp;
	unsigned long csr_scratch;
};

DEFINE_PER_CPU(struct nmi_ctx, nmi_context);

void save_nmi_ctx(void)
{
	this_cpu_write(nmi_context.csr_sp, sw64_read_csr(CSR_SP));
	this_cpu_write(nmi_context.csr_scratch, sw64_read_csr(CSR_SCRATCH));
}

void restore_nmi_ctx(void)
{
	sw64_write_csr_imb(this_cpu_read(nmi_context.csr_sp), CSR_SP);
	sw64_write_csr_imb(this_cpu_read(nmi_context.csr_scratch), CSR_SCRATCH);
}

void
trap_init(void)
{
	/* Tell HMcode what global pointer we want in the kernel. */
	register unsigned long gptr __asm__("$29");
	wrkgp(gptr);

	wrent(entArith, 1);
	wrent(entMM, 2);
	wrent(entIF, 3);
	wrent(entUna, 4);
	wrent(entSys, 5);
#ifdef CONFIG_EFI
	if (smp_processor_id() == 0)
		wrent((void *)entSuspend, 6);
#endif
}
