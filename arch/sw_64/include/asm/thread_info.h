/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_THREAD_INFO_H
#define _ASM_SW64_THREAD_INFO_H

#ifdef __KERNEL__

#ifndef __ASSEMBLY__
#include <asm/types.h>
#include <asm/sysinfo.h>

typedef struct {
	unsigned long seg;
} mm_segment_t;


struct pcb_struct {
	unsigned long ksp;
	unsigned long tp;
	unsigned long da_match, da_mask;
	unsigned long dv_match, dv_mask;
	union {
		unsigned long dc_ctl;
		unsigned long match_ctl;
	};
	unsigned long ia_match, ia_mask;
	unsigned long iv_match;
	unsigned long ida_match, ida_mask;
};

struct thread_info {
	struct pcb_struct	pcb;		/* hmcode state */

	unsigned int		flags;		/* low level flags */
	unsigned int		ieee_state;	/* see fpu.h */

	mm_segment_t		addr_limit;	/* thread address space */
	unsigned int		cpu;		/* current CPU */
	int			preempt_count;	/* 0 => preemptible, <0 => BUG */
	unsigned int		status;		/* thread-synchronous flags */

	int bpt_nsaved;
	unsigned long bpt_addr[2];		/* breakpoint handling  */
	unsigned int bpt_insn[2];
#ifdef CONFIG_DYNAMIC_FTRACE
	unsigned long		dyn_ftrace_addr;
#ifdef CONFIG_DYNAMIC_FTRACE_WITH_REGS
	unsigned long		dyn_ftrace_regs_addr;
#endif
#endif
};

static __always_inline u64 rtid(void)
{
	u64 val;

	asm volatile("rtid %0" : "=r" (val) : :);
	return val;
}

/*
 * Macros/functions for gaining access to the thread information structure.
 */
#define INIT_THREAD_INFO(tsk)				\
{							\
	.addr_limit	= KERNEL_DS,			\
	.preempt_count	= INIT_PREEMPT_COUNT,		\
}


#endif /* __ASSEMBLY__ */

/* Thread information allocation.  */
#define THREAD_SIZE_ORDER	1
#define THREAD_SIZE		(2 * PAGE_SIZE)

/*
 * Thread information flags:
 * - these are process state flags and used from assembly
 * - pending work-to-be-done flags come first and must be assigned to be
 *   within bits 0 to 7 to fit in and immediate operand.
 *
 * TIF_SYSCALL_TRACE is known to be 0 via blbs.
 */
#define TIF_SYSCALL_TRACE	0	/* syscall trace active */
#define TIF_NOTIFY_RESUME	1	/* callback before returning to user */
#define TIF_SIGPENDING		2	/* signal pending */
#define TIF_NEED_RESCHED	3	/* rescheduling necessary */
#define TIF_SYSCALL_AUDIT	4       /* syscall audit active */
#define TIF_UPROBE		5       /* uprobe breakpoint or singlestep */
#define TIF_PATCH_PENDING       6       /* pending live patching update */
#define TIF_NOTIFY_SIGNAL	7	/* signal notifications exist */
#define TIF_DIE_IF_KERNEL	9	/* dik recursion lock */
#define TIF_SYSCALL_TRACEPOINT	10
#define TIF_SECCOMP		11	/* secure computing */
#define TIF_MEMDIE		13	/* is terminating due to OOM killer */
#define TIF_POLLING_NRFLAG	14      /* idle is polling for TIF_NEED_RESCHED */

#define _TIF_SYSCALL_TRACE	(1 << TIF_SYSCALL_TRACE)
#define _TIF_SIGPENDING		(1 << TIF_SIGPENDING)
#define _TIF_NEED_RESCHED	(1 << TIF_NEED_RESCHED)
#define _TIF_NOTIFY_RESUME	(1 << TIF_NOTIFY_RESUME)
#define _TIF_PATCH_PENDING      (1 << TIF_PATCH_PENDING)
#define _TIF_SYSCALL_AUDIT	(1 << TIF_SYSCALL_AUDIT)
#define _TIF_POLLING_NRFLAG	(1 << TIF_POLLING_NRFLAG)
#define _TIF_SECCOMP		(1 << TIF_SECCOMP)
#define _TIF_SYSCALL_TRACEPOINT	(1 << TIF_SYSCALL_TRACEPOINT)
#define _TIF_UPROBE		(1 << TIF_UPROBE)
#define _TIF_NOTIFY_SIGNAL	(1 << TIF_NOTIFY_SIGNAL)

/* Work to do on interrupt/exception return.  */
#define _TIF_WORK_MASK		(_TIF_SIGPENDING | _TIF_NEED_RESCHED | \
				 _TIF_NOTIFY_RESUME | _TIF_UPROBE | \
				 _TIF_PATCH_PENDING | _TIF_NOTIFY_SIGNAL)

#define _TIF_SYSCALL_WORK	(_TIF_SYSCALL_TRACE | _TIF_SYSCALL_AUDIT | \
				 _TIF_SYSCALL_TRACEPOINT | _TIF_SECCOMP)

/* Work to do on any return to userspace.  */
#define _TIF_ALLWORK_MASK	(_TIF_WORK_MASK	| _TIF_SYSCALL_TRACE)

#define TS_UAC_NOPRINT		0x0001	/* ! Preserve the following three */
#define TS_UAC_NOFIX		0x0002	/* ! flags as they match          */
#define TS_UAC_SIGBUS		0x0004	/* ! userspace part of 'prctl' */

#define SET_UNALIGN_CTL(task, value)	({				\
	__u32 status = task_thread_info(task)->status & ~UAC_BITMASK;	\
	if (value & PR_UNALIGN_NOPRINT)					\
		status |= TS_UAC_NOPRINT;				\
	if (value & PR_UNALIGN_SIGBUS)					\
		status |= TS_UAC_SIGBUS;				\
	if (value & PR_NOFIX)	/* sw-specific */			\
		status |= TS_UAC_NOFIX;					\
	task_thread_info(task)->status = status;			\
	0; })

#define GET_UNALIGN_CTL(task, value)	({				\
	__u32 status = task_thread_info(task)->status & ~UAC_BITMASK;	\
	__u32 res = 0;							\
	if (status & TS_UAC_NOPRINT)					\
		res |= PR_UNALIGN_NOPRINT;				\
	if (status & TS_UAC_SIGBUS)					\
		res |= PR_UNALIGN_SIGBUS;				\
	if (status & TS_UAC_NOFIX)					\
		res |= PR_NOFIX;					\
	put_user(res, (int __user *)(value));				\
	})

#endif /* __KERNEL__ */
#endif /* _ASM_SW64_THREAD_INFO_H */
