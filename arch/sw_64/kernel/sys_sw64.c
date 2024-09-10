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

static void local_set_pfh_ctl(void *info)
{
	unsigned long *kcsr = info;

	sw64_write_csr(*kcsr, CSR_PFH_CTL);
}

static void local_set_pfh_cnt(void *info)
{
	unsigned long *kcsr = info;

	sw64_write_csr(*kcsr, CSR_PFH_CNT);
}

enum pfh_field_id {
	L1_CCNT,
	L1_RCNT,
	L1_MCNT,
	L2_CCNT,
	L2_RCNT,
	L2_MCNT,
	L2_RAMP,
	L3_CCNT,
	L3_RCNT,
	L3_MCNT,
	L3_RAMP,
	L1PFH_EN = 0x10,
	L2PFH_EN,
	L3PFH_EN,
	PFH_FIELD_MAX
};

struct pfh_field {
	unsigned long shift;
	unsigned long mask;
};

struct pfh_field pfh_fields_c4[PFH_FIELD_MAX] = {
	[L1_CCNT] = {0, 0x0f},
	[L1_RCNT] = {4, 0x0f},
	[L1_MCNT] = {8, 0x0f},
	[L2_CCNT] = {12, 0x0f},
	[L2_RCNT] = {16, 0x0f},
	[L2_MCNT] = {20, 0x0f},
	[L2_RAMP] = {24, 0x03},
	[L3_CCNT] = {26, 0x1f},
	[L3_RCNT] = {31, 0x1f},
	[L3_MCNT] = {36, 0x1f},
	[L3_RAMP] = {41, 0x03},
	[L1PFH_EN] = {0, 0x01},
	[L2PFH_EN] = {1, 0x01},
	[L3PFH_EN] = {2, 0x01}
};

struct pfh_field pfh_fields_c4b[PFH_FIELD_MAX] = {
	[L1_CCNT] = {0, 0x3f},
	[L1_RCNT] = {6, 0x3f},
	[L1_MCNT] = {12, 0x3f},
	[L2_CCNT] = {18, 0x3f},
	[L2_RCNT] = {24, 0x3f},
	[L2_MCNT] = {30, 0x3f},
	[L2_RAMP] = {36, 0x03},
	[L3_CCNT] = {38, 0x7f},
	[L3_RCNT] = {45, 0x7f},
	[L3_MCNT] = {52, 0x7f},
	[L3_RAMP] = {59, 0x03},
	[L1PFH_EN] = {0, 0x01},
	[L2PFH_EN] = {1, 0x01},
	[L3PFH_EN] = {2, 0x01}
};

/*
 * id:
 *	0x00: PFH_CNT: L1_CCNT
 *	0x01: PFH_CNT: L1_RCNT
 *	0x02: PFH_CNT: L1_MCNT
 *	0x03: PFH_CNT: L2_CCNT
 *	0x04: PFH_CNT: L2_RCNT
 *	0x05: PFH_CNT: L2_MCNT
 *	0x06: PFH_CNT: L2_RAMP
 *	0x07: PFH_CNT: L3_CCNT
 *	0x08: PFH_CNT: L3_RCNT
 *	0x09: PFH_CNT: L3_MCNT
 *	0x0a: PFH_CNT: L3_RAMP
 *	0x10: PFH_CTL: L1PFH_EN
 *	0x11: PFH_CTL: L2PFH_EN
 *	0x12: PFH_CTL: L3PFH_EN
 * op:	0 for get, 1 for set
 */
SYSCALL_DEFINE3(pfh_ops, unsigned long, id, unsigned long, op,
		unsigned long __user *, buf)
{
	unsigned long kcsr = 0;
	unsigned long kbuf = 0;
	unsigned long field_shift;
	unsigned long field_mask;
	unsigned long csr_idx;
	long error = 0;
	struct pfh_field *pfh_fields_arr;

	if (!is_in_host())
		return -EPERM;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	switch (id & 0xf0) {
	case 0x00:
		csr_idx = CSR_PFH_CNT;
		break;
	case 0x10:
		csr_idx = CSR_PFH_CTL;
		break;
	default:
		error = -EINVAL;
		goto out;
	}

	if (!is_junzhang_v3())
		pfh_fields_arr = pfh_fields_c4;
	else
		pfh_fields_arr = pfh_fields_c4b;

	field_shift = pfh_fields_arr[id].shift;
	field_mask = pfh_fields_arr[id].mask << field_shift;

	switch (csr_idx) {
	case CSR_PFH_CTL:
		kcsr = sw64_read_csr(CSR_PFH_CTL);
		break;
	case CSR_PFH_CNT:
		kcsr = sw64_read_csr(CSR_PFH_CNT);
		break;
	default:
		/* should never reach here */
		BUG();
	}

	switch (op) {
	case 0:		// get
		kbuf = (kcsr & field_mask) >> field_shift;
		error = put_user(kbuf, buf);
		goto out;
	case 1:		// set
		error = get_user(kbuf, buf);
		if (error)
			goto out;
		kcsr = (kcsr & (~field_mask)) |
			((kbuf << field_shift) & field_mask);
		break;
	default:
		error = -EINVAL;
		goto out;
	}

	switch (csr_idx) {
	case CSR_PFH_CTL:
		smp_call_function(local_set_pfh_ctl, &kcsr, 1);
		local_set_pfh_ctl(&kcsr);
		break;
	case CSR_PFH_CNT:
		smp_call_function(local_set_pfh_cnt, &kcsr, 1);
		local_set_pfh_cnt(&kcsr);
		break;
	default:
		/* should never reach here */
		BUG();
	}

out:
	return error;
}

#else

SYSCALL_DEFINE0(pfh_ops)
{
	return -ENOSYS;
}

#endif /* CONFIG_SUBARCH_C4 */
