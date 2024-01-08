/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_ASM_SW64_SIGCONTEXT_H
#define _UAPI_ASM_SW64_SIGCONTEXT_H

/*
 * Signal context structure
 *
 * The context is saved before a signal handler is invoked, and it is
 * restored by sys_sigreturn / sys_rt_sigreturn.
 */
struct sigcontext {
	long		sc_onstack;
	long		sc_mask;
	long		sc_pc;
	long		sc_ps;
	long		sc_regs[32];
	long		sc_ownedfp;
	long		sc_fpregs[128];	/* SIMD-FP */
	unsigned long	sc_fpcr;
	/* TODO: Following are unused, to be removed and synced with libc */
	unsigned long	sc_fp_control;
	unsigned long	sc_reserved1, sc_reserved2;
	unsigned long	sc_ssize;
	char		*sc_sbase;
	unsigned long	sc_traparg_a0;
	unsigned long	sc_traparg_a1;
	unsigned long	sc_traparg_a2;
	unsigned long	sc_fp_trap_pc;
	unsigned long	sc_fp_trigger_sum;
	unsigned long	sc_fp_trigger_inst;
};


#endif /* _UAPI_ASM_SW64_SIGCONTEXT_H */
