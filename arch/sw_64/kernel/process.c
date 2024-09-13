// SPDX-License-Identifier: GPL-2.0
/*
 * This file handles the architecture-dependent parts of process handling.
 */

#include <linux/sched/debug.h>
#include <linux/ptrace.h>
#include <linux/elfcore.h>
#include <linux/slab.h>
#include <linux/random.h>

#include <asm/asm-offsets.h>
#include <asm/fpu.h>
#include <asm/switch_to.h>
#include <asm/syscall.h>

#include "proto.h"

/*
 * Re-start a thread when doing execve()
 */
void
start_thread(struct pt_regs *regs, unsigned long pc, unsigned long sp)
{
	regs->pc = pc;
	regs->ps = 8;
	regs->regs[30] = sp;
}
EXPORT_SYMBOL(start_thread);


void
flush_thread(void)
{
	/* Arrange for each exec'ed process to start off with a clean slate
	 * with respect to the FPU.  This is all exceptions disabled.
	 */
	unsigned int *ieee_state = &current_thread_info()->ieee_state;

	*ieee_state = 0;
#ifndef CONFIG_SUBARCH_C3B
	*ieee_state |= IEEE_HARD_DM;
#endif
	wrfpcr(FPCR_INIT | ieee_swcr_to_fpcr(*ieee_state));

	/* Clean slate for TLS.  */
	current_thread_info()->pcb.tp = 0;
}

void
release_thread(struct task_struct *dead_task)
{
}

int arch_dup_task_struct(struct task_struct *dst, struct task_struct *src)
{
	/*
	 * aux_save() has to read the current TLS pointer from CSR:TID as it
	 * may be out-of-sync with the saved value.
	 */
	aux_save(src);
	*dst = *src;
	return 0;
}

/*
 * Copy architecture-specific thread state
 */

int copy_thread(struct task_struct *p, const struct kernel_clone_args *args)
{
	unsigned long clone_flags = args->flags;
	unsigned long usp = args->stack;
	unsigned long tls = args->tls;
	struct thread_info *childti = task_thread_info(p);
	struct pt_regs *childregs = task_pt_regs(p);
	struct pt_regs *regs = current_pt_regs();

	extern void ret_from_fork(void);
	extern void ret_from_kernel_thread(void);

	p->thread.sp = (unsigned long) childregs - STACKFRAME_SIZE;

	if (unlikely(args->fn)) {
		/* kernel thread */
		memset(childregs, 0, sizeof(struct pt_regs));
		p->thread.ra = (unsigned long) ret_from_kernel_thread;
		p->thread.s[0] = (unsigned long) args->fn;	/* function */
		p->thread.s[1] = (unsigned long) args->fn_arg;
		return 0;
	}

	/*
	 * Note: if CLONE_SETTLS is not set, then we must inherit the
	 * value from the parent, which will have been set by the block
	 * copy in dup_task_struct.  This is non-intuitive, but is
	 * required for proper operation in the case of a threaded
	 * application calling fork.
	 */
	if (clone_flags & CLONE_SETTLS)
		childti->pcb.tp = tls;
	else
		regs->regs[20] = 0;
	*childregs = *regs;
	if (usp)
		childregs->regs[30] = usp;
	syscall_set_return_value(NULL, childregs, 0, 0);
	p->thread.ra = (unsigned long) ret_from_fork;
	return 0;
}

unsigned long arch_randomize_brk(struct mm_struct *mm)
{
	return randomize_page(mm->brk, 0x02000000);
}
