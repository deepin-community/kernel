// SPDX-License-Identifier: GPL-2.0

#include <linux/syscalls.h>
#include <asm/fpu.h>

SYSCALL_DEFINE5(getsysinfo, unsigned long, op, void __user *, buffer,
		unsigned long, nbytes, int __user *, start, void __user *, arg)
{
	unsigned long w;

	switch (op) {
	case GSI_IEEE_FP_CONTROL:
		/* Return current software fp control & status bits.  */
		/* Note that DU doesn't verify available space here.  */

		w = current_thread_info()->ieee_state & IEEE_SW_MASK;
		w = swcr_update_status(w, rdfpcr());
		if (put_user(w, (unsigned long __user *) buffer))
			return -EFAULT;
		return 0;
	default:
		break;
	}

	return -EOPNOTSUPP;
}

SYSCALL_DEFINE5(setsysinfo, unsigned long, op, void __user *, buffer,
		unsigned long, nbytes, int __user *, start, void __user *, arg)
{
	switch (op) {
	case SSI_IEEE_FP_CONTROL: {
		unsigned long swcr, fpcr;
		unsigned int *state;

		/*
		 * Sw_64 Architecture Handbook 4.7.7.3:
		 * To be fully IEEE compiant, we must track the current IEEE
		 * exception state in software, because spurious bits can be
		 * set in the trap shadow of a software-complete insn.
		 */

		if (get_user(swcr, (unsigned long __user *)buffer))
			return -EFAULT;
		state = &current_thread_info()->ieee_state;

		/* Update softare trap enable bits.  */
		*state = (*state & ~IEEE_SW_MASK) | (swcr & IEEE_SW_MASK);

		/* Update the real fpcr.  */
		fpcr = rdfpcr() & FPCR_DYN_MASK;
		fpcr |= ieee_swcr_to_fpcr(swcr);
		wrfpcr(fpcr);

		return 0;
	}

	case SSI_IEEE_RAISE_EXCEPTION: {
		unsigned long exc, swcr, fpcr, fex;
		unsigned int *state;

		if (get_user(exc, (unsigned long __user *)buffer))
			return -EFAULT;
		state = &current_thread_info()->ieee_state;
		exc &= IEEE_STATUS_MASK_ALL;

		/* Update softare trap enable bits.  */
		swcr = (*state & IEEE_SW_MASK) | exc;
		*state |= exc;

		/* Update the real fpcr.  */
		fpcr = rdfpcr();
		fpcr |= ieee_swcr_to_fpcr(swcr);
		wrfpcr(fpcr);

		/* If any exceptions set by this call, and are unmasked,
		 * send a signal.  Old exceptions are not signaled.
		 */
		fex = swcr_status_to_fex(exc, -1) & swcr;
		if (fex) {
			int si_code = FPE_FLTUNK;

			if (fex & IEEE_TRAP_ENABLE_DNO)
				si_code = FPE_FLTUND;
			if (fex & IEEE_TRAP_ENABLE_INE)
				si_code = FPE_FLTRES;
			if (fex & IEEE_TRAP_ENABLE_UNF)
				si_code = FPE_FLTUND;
			if (fex & IEEE_TRAP_ENABLE_OVF)
				si_code = FPE_FLTOVF;
			if (fex & IEEE_TRAP_ENABLE_DZE)
				si_code = FPE_FLTDIV;
			if (fex & IEEE_TRAP_ENABLE_INV)
				si_code = FPE_FLTINV;
			if (fex & IEEE_TRAP_ENABLE_OVI)
				si_code = FPE_INTOVF;

			send_sig_fault(SIGFPE, si_code, (void __user *)NULL, current);
		}
		return 0;
	}
	default:
		break;
	}

	return -EOPNOTSUPP;
}

SYSCALL_DEFINE2(odd_getpriority, int, which, int, who)
{
	int prio = sys_getpriority(which, who);

	if (prio >= 0) {
		/* Return value is the unbiased priority, i.e. 20 - prio.
		 * This does result in negative return values, so signal
		 * no error.
		 */
		force_successful_syscall_return();
		prio = 20 - prio;
	}
	return prio;
}

SYSCALL_DEFINE0(getxuid)
{
	current_pt_regs()->regs[20] = sys_geteuid();
	return sys_getuid();
}

SYSCALL_DEFINE0(getxgid)
{
	current_pt_regs()->regs[20] = sys_getegid();
	return sys_getgid();
}

SYSCALL_DEFINE0(getxpid)
{
	current_pt_regs()->regs[20] = sys_getppid();
	return sys_getpid();
}

SYSCALL_DEFINE0(sw64_pipe)
{
	int fd[2];
	int res = do_pipe_flags(fd, 0);

	if (!res) {
		/* The return values are in $0 and $20.  */
		current_pt_regs()->regs[20] = fd[1];
		res = fd[0];
	}
	return res;
}

#ifdef CONFIG_SUBARCH_C4

struct pfh_val {
	unsigned long pfh_ctl;
	unsigned long pfh_cnt;
};

static void local_set_pfh(void *info)
{
	struct pfh_val *kbuf = info;

	if (kbuf->pfh_ctl)
		sw64_write_csr(kbuf->pfh_ctl, CSR_PFH_CTL);
	if (kbuf->pfh_cnt)
		sw64_write_csr(kbuf->pfh_cnt, CSR_PFH_CNT);
}

SYSCALL_DEFINE3(pfh_ops, unsigned long, op,
		unsigned long __user *, pfh_ctl_p,
		unsigned long __user *, pfh_cnt_p)
{
	struct pfh_val kbuf = {0, 0};
	long error = 0;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (op) {	// op != 0, set
		if (pfh_ctl_p)
			error |= get_user(kbuf.pfh_ctl, pfh_ctl_p);
		if (pfh_cnt_p)
			error |= get_user(kbuf.pfh_cnt, pfh_cnt_p);

		if (!error && (kbuf.pfh_ctl || kbuf.pfh_cnt)) {
			smp_call_function(local_set_pfh, &kbuf, 1);
			local_set_pfh(&kbuf);
		}
	} else {	// op == 0, get
		if (pfh_ctl_p) {
			kbuf.pfh_ctl = sw64_read_csr(CSR_PFH_CTL);
			error |= put_user(kbuf.pfh_ctl, pfh_ctl_p);
		}

		if (pfh_cnt_p) {
			kbuf.pfh_cnt = sw64_read_csr(CSR_PFH_CNT);
			error |= put_user(kbuf.pfh_cnt, pfh_cnt_p);
		}
	}

	return error;
}

#else

SYSCALL_DEFINE0(pfh_ops)
{
	return -ENOSYS;
}

#endif /* CONFIG_SUBARCH_C4 */
