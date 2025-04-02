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

asmlinkage void noinstr do_entArith(struct pt_regs *regs)
{
	long si_code = FPE_FLTINV;
	unsigned long exc_sum = regs->earg0;

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
asmlinkage void noinstr do_entIF(struct pt_regs *regs)
{
	int signo, code;
	unsigned int inst, type;
	unsigned long inst_type = regs->earg0;
	unsigned long va = regs->earg1;

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
