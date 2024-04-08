// SPDX-License-Identifier: GPL-2.0
/*
 *  linux/arch/sw_64/kernel/signal.c
 *
 *  Copyright (C) 1995  Linus Torvalds
 *
 *  1997-11-02  Modified for POSIX.1b signals by Richard Henderson
 */

#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/syscalls.h>
#include <linux/livepatch.h>
#include <linux/resume_user_mode.h>

#include <asm/ucontext.h>
#include <asm/uprobes.h>
#include <asm/vdso.h>
#include <asm/switch_to.h>
#include <asm/syscall.h>

#include "proto.h"


#define DEBUG_SIG 0

#define _BLOCKABLE (~(sigmask(SIGKILL) | sigmask(SIGSTOP)))

SYSCALL_DEFINE2(odd_sigprocmask, int, how, unsigned long, newmask)
{
	sigset_t oldmask;
	sigset_t mask;
	unsigned long res;

	siginitset(&mask, newmask & _BLOCKABLE);
	res = sigprocmask(how, &mask, &oldmask);
	if (!res) {
		force_successful_syscall_return();
		res = oldmask.sig[0];
	}
	return res;
}

SYSCALL_DEFINE3(odd_sigaction, int, sig,
		const struct odd_sigaction __user *, act,
		struct odd_sigaction __user *, oact)
{
	struct k_sigaction new_ka, old_ka;
	old_sigset_t mask;
	int ret;

	if (act) {
		if (!access_ok(act, sizeof(*act)) ||
		    __get_user(new_ka.sa.sa_handler, &act->sa_handler) ||
		    __get_user(new_ka.sa.sa_flags, &act->sa_flags) ||
		    __get_user(mask, &act->sa_mask))
			return -EFAULT;
		siginitset(&new_ka.sa.sa_mask, mask);
	}

	ret = do_sigaction(sig, act ? &new_ka : NULL, oact ? &old_ka : NULL);

	if (!ret && oact) {
		if (!access_ok(oact, sizeof(*oact)) ||
		    __put_user(old_ka.sa.sa_handler, &oact->sa_handler) ||
		    __put_user(old_ka.sa.sa_flags, &oact->sa_flags) ||
		    __put_user(old_ka.sa.sa_mask.sig[0], &oact->sa_mask))
			return -EFAULT;
	}

	return ret;
}

/*
 * Do a signal return; undo the signal stack.
 */

#if _NSIG_WORDS > 1
# error "Non SA_SIGINFO frame needs rearranging"
#endif

struct rt_sigframe {
	struct siginfo info;
	struct ucontext uc;
} __aligned(16);

/*
 * If this changes, userland unwinders that Know Things about our signal
 * frame will break.  Do not undertake lightly.  It also implies an ABI
 * change wrt the size of siginfo_t, which may cause some pain.
 */
extern char compile_time_assert
	[offsetof(struct rt_sigframe, uc.uc_mcontext) == 176 ? 1 : -1];

static long
restore_sigcontext(struct sigcontext __user *sc, struct pt_regs *regs)
{
	long err = __get_user(regs->pc, &sc->sc_pc);

	err |= __copy_from_user(regs, sc->sc_regs, sizeof_field(struct pt_regs, regs));
	/* simd-fp */
	err |= __copy_from_user(&current->thread.fpstate, &sc->sc_fpregs,
				offsetof(struct user_fpsimd_state, fpcr));
	err |= __get_user(current->thread.fpstate.fpcr, &sc->sc_fpcr);

	if (likely(!err))
		__fpstate_restore(current);

	return err;
}

/*
 * Note that this syscall is also used by setcontext(3) to install
 * a given sigcontext.  This because it's impossible to set *all*
 * registers and transfer control from userland.
 */

SYSCALL_DEFINE1(sigreturn, struct sigcontext __user *, sc)
{
	struct pt_regs *regs = current_pt_regs();
	sigset_t set;

	force_successful_syscall_return();

	/* Always make any pending restarted system calls return -EINTR */
	current->restart_block.fn = do_no_restart_syscall;

	/* Verify that it's a good sigcontext before using it */
	if (!access_ok(sc, sizeof(*sc)))
		goto give_sigsegv;
	if (__get_user(set.sig[0], &sc->sc_mask))
		goto give_sigsegv;

	set_current_blocked(&set);

	if (restore_sigcontext(sc, regs))
		goto give_sigsegv;

	/* Send SIGTRAP if we're single-stepping: */
	if (ptrace_cancel_bpt(current)) {
		force_sig_fault(SIGTRAP, TRAP_BRKPT,
				(void __user *)regs->pc);
	}
	return regs->regs[0];

give_sigsegv:
	force_sig(SIGSEGV);
	return 0;
}

SYSCALL_DEFINE1(rt_sigreturn, struct rt_sigframe __user *, frame)
{
	struct pt_regs *regs = current_pt_regs();
	sigset_t set;

	force_successful_syscall_return();

	/* Always make any pending restarted system calls return -EINTR */
	current->restart_block.fn = do_no_restart_syscall;

	/* Verify that it's a good ucontext_t before using it */
	if (!access_ok(&frame->uc, sizeof(frame->uc)))
		goto give_sigsegv;
	if (__copy_from_user(&set, &frame->uc.uc_sigmask, sizeof(set)))
		goto give_sigsegv;

	set_current_blocked(&set);

	if (restore_sigcontext(&frame->uc.uc_mcontext, regs))
		goto give_sigsegv;

	if (restore_altstack(&frame->uc.uc_stack))
		goto give_sigsegv;

	/* Send SIGTRAP if we're single-stepping: */
	if (ptrace_cancel_bpt(current)) {
		force_sig_fault(SIGTRAP, TRAP_BRKPT,
				(void __user *)regs->pc);
	}
	return regs->regs[0];

give_sigsegv:
	force_sig(SIGSEGV);
	return 0;
}


/*
 * Set up a signal frame.
 */

static inline void __user *
get_sigframe(struct ksignal *ksig, unsigned long sp, size_t frame_size)
{
	return (void __user *)((sigsp(sp, ksig) - frame_size) & -32ul);
}

static long
setup_sigcontext(struct sigcontext __user *sc, struct pt_regs *regs,
		 unsigned long mask)
{
	long err = 0;

	err |= __put_user(on_sig_stack((unsigned long)sc), &sc->sc_onstack);
	err |= __put_user(mask, &sc->sc_mask);
	err |= __put_user(regs->pc, &sc->sc_pc);
	err |= __put_user(8, &sc->sc_ps);

	err |= __copy_to_user(sc->sc_regs, regs, sizeof_field(struct pt_regs, regs));
	err |= __put_user(0, sc->sc_regs+31);
	/* simd-fp */
	__fpstate_save(current);
	err |= __copy_to_user(&sc->sc_fpregs, &current->thread.fpstate,
				offsetof(struct user_fpsimd_state, fpcr));
	err |= __put_user(current->thread.fpstate.fpcr, &sc->sc_fpcr);

	return err;
}

static int
setup_rt_frame(struct ksignal *ksig, sigset_t *set, struct pt_regs *regs)
{
	unsigned long err = 0;
	struct rt_sigframe __user *frame;

	frame = get_sigframe(ksig, regs->regs[30], sizeof(*frame));
	if (!access_ok(frame, sizeof(*frame)))
		return -EFAULT;

	if (ksig->ka.sa.sa_flags & SA_SIGINFO)
		err |= copy_siginfo_to_user(&frame->info, &ksig->info);

	/* Create the ucontext.  */
	err |= __put_user(0, &frame->uc.uc_flags);
	err |= __put_user(0, &frame->uc.uc_link);
	err |= __put_user(set->sig[0], &frame->uc.uc_old_sigmask);
	err |= __save_altstack(&frame->uc.uc_stack, regs->regs[30]);
	err |= setup_sigcontext(&frame->uc.uc_mcontext, regs, set->sig[0]);
	err |= __copy_to_user(&frame->uc.uc_sigmask, set, sizeof(*set));
	if (err)
		return -EFAULT;

	/* "Return" to the handler */
	regs->regs[26] = VDSO_SYMBOL(current->mm->context.vdso, rt_sigreturn);
	regs->regs[27] = regs->pc = (unsigned long) ksig->ka.sa.sa_handler;
	regs->regs[16] = ksig->sig;                    /* a0: signal number */
	if (ksig->ka.sa.sa_flags & SA_SIGINFO) {
		/* a1: siginfo pointer, a2: ucontext pointer */
		regs->regs[17] = (unsigned long) &frame->info;
		regs->regs[18] = (unsigned long) &frame->uc;
	} else {
		/* a1: exception code, a2: sigcontext pointer */
		regs->regs[17] = 0;
		regs->regs[18] = (unsigned long) &frame->uc.uc_mcontext;
	}
	regs->regs[30] = (unsigned long) frame;

#if DEBUG_SIG
	pr_info("SIG deliver (%s:%d): sp=%p pc=%p ra=%p\n",
			current->comm, current->pid, frame, regs->pc, regs->regs[26]);
#endif

	return 0;
}

/*
 * OK, we're invoking a handler.
 */
static inline void
handle_signal(struct ksignal *ksig, struct pt_regs *regs)
{
	sigset_t *oldset = sigmask_to_save();
	int ret;

	rseq_signal_deliver(ksig, regs);

	ret = setup_rt_frame(ksig, oldset, regs);

	signal_setup_done(ret, ksig, 0);
}

/*
 * Note that 'init' is a special process: it doesn't get signals it doesn't
 * want to handle. Thus you cannot kill init even with a SIGKILL even by
 * mistake.
 *
 * Note that we go through the signals twice: once to check the signals that
 * the kernel can handle, and then we build all the user-level signal handling
 * stack-frames in one go after that.
 */
static void
do_signal(struct pt_regs *regs)
{
	unsigned long single_stepping = ptrace_cancel_bpt(current);
	struct ksignal ksig;

	/* This lets the debugger run, ... */
	if (get_signal(&ksig)) {
		/* ... so re-check the single stepping. */
		single_stepping |= ptrace_cancel_bpt(current);
		/* Whee!  Actually deliver the signal.  */
		if (regs->orig_r0 != NO_SYSCALL) {
			switch (syscall_get_error(current, regs)) {
			case -ERESTARTSYS:
				if (!(ksig.ka.sa.sa_flags & SA_RESTART)) {
					regs->regs[0] = EINTR;
					break;
				}
				fallthrough;
			case -ERESTARTNOINTR:
				/* reset v0 and a3 and replay syscall */
				regs->regs[0] = regs->orig_r0;
				regs->regs[19] = regs->orig_r19;
				regs->pc -= 4;
				break;
			case -ERESTARTNOHAND:
			case -ERESTART_RESTARTBLOCK:
				regs->regs[0] = EINTR;
				break;
			}
			regs->orig_r0 = NO_SYSCALL;
		}
		handle_signal(&ksig, regs);
	} else {
		single_stepping |= ptrace_cancel_bpt(current);
		if (regs->orig_r0 != NO_SYSCALL) {
			switch (syscall_get_error(current, regs)) {
			case -ERESTARTSYS:
			case -ERESTARTNOINTR:
			case -ERESTARTNOHAND:
				/* Reset v0 and a3 and replay syscall.  */
				regs->regs[0] = regs->orig_r0;
				regs->regs[19] = regs->orig_r19;
				regs->pc -= 4;
				break;
			case -ERESTART_RESTARTBLOCK:
				/* Set v0 to the restart_syscall and replay */
				regs->regs[0] = __NR_restart_syscall;
				regs->pc -= 4;
				break;
			}
			regs->orig_r0 = NO_SYSCALL;
		}
		restore_saved_sigmask();
	}
	if (single_stepping)
		ptrace_set_bpt(current);        /* re-set breakpoint */
}

asmlinkage void
do_notify_resume(struct pt_regs *regs, unsigned long thread_flags)
{
	do {
		local_irq_enable();

		if (thread_flags & _TIF_NEED_RESCHED)
			schedule();

		if (thread_flags & _TIF_UPROBE) {
			unsigned long pc = regs->pc;

			uprobe_notify_resume(regs);
			sw64_fix_uretprobe(regs, pc - 4);
		}

		if (thread_flags & _TIF_PATCH_PENDING)
			klp_update_patch_state(current);

		if (thread_flags & (_TIF_SIGPENDING | _TIF_NOTIFY_SIGNAL))
			do_signal(regs);

		if (thread_flags & _TIF_NOTIFY_RESUME)
			resume_user_mode_work(regs);

		local_irq_disable();
		thread_flags = READ_ONCE(current_thread_info()->flags);
	} while (thread_flags & _TIF_WORK_MASK);
}
