/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_FPU_SCHED_H
#define _ASM_X86_FPU_SCHED_H

#include <linux/sched.h>

#include <asm/cpufeature.h>
#include <asm/fpu/types.h>

#include <asm/trace/fpu.h>

extern void save_fpregs_to_fpstate(struct fpu *fpu);
extern void fpu__drop(struct fpu *fpu);
extern int  fpu_clone(struct task_struct *dst, unsigned long clone_flags, bool minimal,
		      unsigned long shstk_addr);
extern void fpu_flush_thread(void);

/*
 * FPU state switching for scheduling.
 *
 * This is a two-stage process:
 *
 *  - switch_fpu_prepare() saves the old state.
 *    This is done within the context of the old process.
 *
 *  - switch_fpu_finish() sets TIF_NEED_FPU_LOAD; the floating point state
 *    will get loaded on return to userspace, or when the kernel needs it.
 *
 * If TIF_NEED_FPU_LOAD is cleared then the CPU's FPU registers
 * are saved in the current thread's FPU register state.
 *
 * If TIF_NEED_FPU_LOAD is set then CPU's FPU registers may not
 * hold current()'s FPU registers. It is required to load the
 * registers before returning to userland or using the content
 * otherwise.
 *
 * The FPU context is only stored/restored for a user task and
 * PF_KTHREAD is used to distinguish between kernel and user threads.
 */
static inline void switch_fpu_prepare(struct fpu *old_fpu, int cpu)
{
	if (cpu_feature_enabled(X86_FEATURE_FPU) &&
	    !(current->flags & (PF_KTHREAD | PF_USER_WORKER))) {
		save_fpregs_to_fpstate(old_fpu);
		/*
		 * The save operation preserved register state, so the
		 * fpu_fpregs_owner_ctx is still @old_fpu. Store the
		 * current CPU number in @old_fpu, so the next return
		 * to user space can avoid the FPU register restore
		 * when is returns on the same CPU and still owns the
		 * context.
		 */
		old_fpu->last_cpu = cpu;

		trace_x86_fpu_regs_deactivated(old_fpu);
	}
}

/*
 * Delay loading of the complete FPU state until the return to userland.
 * PKRU is handled separately.
 */
static inline void switch_fpu_finish(void)
{
	if (cpu_feature_enabled(X86_FEATURE_FPU))
		set_thread_flag(TIF_NEED_FPU_LOAD);
}

/*
 * Kernel FPU state switching for scheduling.
 *
 * This is a two-stage process:
 *
 *  - switch_kernel_fpu_prepare() saves the old kernel fpu state.
 *    This is done within the context of the old process.
 *
 *  - switch_kernel_fpu_finish() restore new kernel fpu state.
 *
 * The kernel FPU context is only stored/restored for a user task in kernel
 * mode and PF_KTHREAD is used to distinguish between kernel and user threads.
 */
#if defined(CONFIG_X86_HYGON_LMC_SSE2_ON) || \
	defined(CONFIG_X86_HYGON_LMC_AVX2_ON)
extern void save_fpregs_to_fpkernelstate(struct fpu *kfpu);
extern unsigned long get_fpu_registers_pos(struct fpu *fpu, unsigned int off);
static inline void switch_kernel_fpu_prepare(struct task_struct *prev, int cpu)
{
	struct fpu *old_fpu = &prev->thread.fpu;

	if (!test_thread_flag(TIF_USING_FPU_NONATOMIC))
		return;

	if (static_cpu_has(X86_FEATURE_FPU) && !(prev->flags & PF_KTHREAD))
		save_fpregs_to_fpkernelstate(old_fpu);
}

/* Internal helper for switch_kernel_fpu_finish() and signal frame setup */
static inline void fpregs_restore_kernelregs(struct fpu *kfpu)
{
	kernel_fpu_states_restore(NULL, (void *)get_fpu_registers_pos(kfpu, MAX_FPU_CTX_SIZE),
						MAX_FPU_CTX_SIZE);
}

/* Loading of the complete FPU state immediately. */
static inline void switch_kernel_fpu_finish(struct task_struct *next)
{
	struct fpu *new_fpu = &next->thread.fpu;

	if (next->flags & PF_KTHREAD)
		return;

	if (cpu_feature_enabled(X86_FEATURE_FPU) &&
	    test_ti_thread_flag((struct thread_info *)next,
				TIF_USING_FPU_NONATOMIC))
		fpregs_restore_kernelregs(new_fpu);
}
#else
static inline void switch_kernel_fpu_prepare(struct task_struct *prev, int cpu)
{
}
static inline void switch_kernel_fpu_finish(struct task_struct *next)
{
}

#endif

#endif /* _ASM_X86_FPU_SCHED_H */
