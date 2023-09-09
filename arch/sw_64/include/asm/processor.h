/* SPDX-License-Identifier: GPL-2.0 */
/*
 * include/asm-sw64/processor.h
 *
 * Copyright (C) 1994 Linus Torvalds
 */

#ifndef _ASM_SW64_PROCESSOR_H
#define _ASM_SW64_PROCESSOR_H

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
 * 20230728(mcw):
 * To make sure that arch_get_unmapped_area_topdown and old
 * software, e.g. golang runtime and v8 jit, works well at
 * the same time, just providing 47-bit VAs unless a hint is
 * supplied to mmap.
 */

#define VA_BITS		(CONFIG_SW64_VA_BITS)
#if VA_BITS > 47
#define VA_BITS_MIN	(47)
#else
#define VA_BITS_MIN	(VA_BITS)
#endif

#define	DEFAULT_MAP_WINDOW_64	(1UL << VA_BITS_MIN)
#define	TASK_SIZE_64		(1UL << VA_BITS)

#define TASK_SIZE_MAX		TASK_SIZE_64
#define TASK_SIZE		TASK_SIZE_64
#define DEFAULT_MAP_WINDOW	DEFAULT_MAP_WINDOW_64

#ifdef CONFIG_SW64_FORCE_52BIT
#define STACK_TOP_MAX		TASK_SIZE
#define TASK_UNMAPPED_BASE	(PAGE_ALIGN(TASK_SIZE / 4))
#else
#define STACK_TOP_MAX		DEFAULT_MAP_WINDOW
#define TASK_UNMAPPED_BASE	(PAGE_ALIGN(DEFAULT_MAP_WINDOW / 4))
#endif

#define STACK_TOP	STACK_TOP_MAX

#ifndef CONFIG_SW64_FORCE_52BIT
#define arch_get_mmap_end(addr, len, flags) \
	(((addr) > DEFAULT_MAP_WINDOW) ? TASK_SIZE : DEFAULT_MAP_WINDOW)
#define arch_get_mmap_base(addr, base)	((addr > DEFAULT_MAP_WINDOW) ? \
		base + TASK_SIZE - DEFAULT_MAP_WINDOW : \
		base)
#else
#define arch_get_mmap_end(addr, len, flags)	(TASK_SIZE)
#define arch_get_mmap_base(addr, base)	(base)
#endif

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
