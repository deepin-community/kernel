/* SPDX-License-Identifier: GPL-2.0 */
/*
 * include/asm-sw64/processor.h
 *
 * Copyright (C) 1994 Linus Torvalds
 */

#ifndef _ASM_SW64_PROCESSOR_H
#define _ASM_SW64_PROCESSOR_H

#include <linux/personality.h>	/* for ADDR_LIMIT_32BIT */
#include <asm/ptrace.h>

#define task_pt_regs(task) \
	((struct pt_regs *) (task->stack + THREAD_SIZE) - 1)

/*
 * Returns current instruction pointer ("program counter").
 */
#define current_text_addr() \
	({ void *__pc; __asm__ ("br %0, .+4" : "=r"(__pc)); __pc; })

/*
 * SW64 does have an arch_pick_mmap_layout()
 */
#define HAVE_ARCH_PICK_MMAP_LAYOUT 1

/*
 * We have a 52-bit user address space: 4PB user VM...
 */
#define TASK_SIZE (0x10000000000000UL)
#define UNMAPPED_BASE (TASK_SIZE >> 6)
#define STACK_TOP \
	(current->personality & ADDR_LIMIT_32BIT ? 0x80000000 : 0x00120000000UL)

#define STACK_TOP_MAX	0x00120000000UL

/* This decides where the kernel will search for a free chunk of vm
 * space during mmap's.
 */
#define TASK_UNMAPPED_BASE \
	((current->personality & ADDR_LIMIT_32BIT) ? 0x40000000 : UNMAPPED_BASE)

struct thread_struct {
	struct user_fpsimd_state fpstate;
	/* Callee-saved registers */
	unsigned long ra;
	unsigned long sp;
	unsigned long s[7];	/* s0 ~ s6 */
};
#define INIT_THREAD  { }

struct task_struct;
struct pt_regs;

/* Do necessary setup to start up a newly executed thread.  */
extern void start_thread(struct pt_regs *regs, unsigned long pc, unsigned long sp);

/* Free all resources held by a thread. */
extern void release_thread(struct task_struct *dead_task);

unsigned long __get_wchan(struct task_struct *p);

#define KSTK_EIP(tsk) (task_pt_regs(tsk)->pc)

#define KSTK_ESP(tsk) (task_pt_regs(tsk)->regs[30])

#define cpu_relax()	imemb()

#define ARCH_HAS_PREFETCH
#define ARCH_HAS_PREFETCHW
#define ARCH_HAS_SPINLOCK_PREFETCH

#ifndef CONFIG_SMP
/* Nothing to prefetch. */
#define spin_lock_prefetch(lock)	do { } while (0)
#endif

static inline void prefetch(const void *ptr)
{
	__builtin_prefetch(ptr, 0, 3);
}

static inline void prefetchw(const void *ptr)
{
	__builtin_prefetch(ptr, 1, 3);
}

#ifdef CONFIG_SMP
static inline void spin_lock_prefetch(const void *ptr)
{
	__builtin_prefetch(ptr, 1, 3);
}
#endif

static inline void wait_for_interrupt(void)
{
	__asm__ __volatile__ ("halt");
}
#endif /* _ASM_SW64_PROCESSOR_H */
