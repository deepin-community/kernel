/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ASM_SW64_STACKTRACE_H
#define _ASM_SW64_STACKTRACE_H

#include <linux/percpu.h>
#include <linux/sched.h>
#include <linux/sched/task_stack.h>
#include <asm/memory.h>
#include <asm/ptrace.h>

struct stackframe {
	unsigned long pc;
	unsigned long fp;
};

enum stack_type {
	STACK_TYPE_UNKNOWN,
	STACK_TYPE_TASK,
};

struct stack_info {
	unsigned long low;
	unsigned long high;
	enum stack_type type;
};

/* The form of the top of the frame on the stack */
struct stack_frame {
	unsigned long return_address;
	struct stack_frame *next_frame;
};

extern int unwind_frame(struct task_struct *tsk, struct stackframe *frame);
extern void walk_stackframe(struct task_struct *tsk, struct pt_regs *regs,
			    int (*fn)(unsigned long, void *), void *data);

static inline bool on_task_stack(struct task_struct *tsk, unsigned long sp,
				struct stack_info *info)
{
	unsigned long low = (unsigned long)task_stack_page(tsk);
	unsigned long high = low + THREAD_SIZE;

	if (sp < low || sp >= high)
		return false;

	if (info) {
		info->low = low;
		info->high = high;
		info->type = STACK_TYPE_TASK;
	}

	return true;
}

/*
 * We can only safely access per-cpu stacks from current in a non-preemptible
 * context.
 */
static inline bool on_accessible_stack(struct task_struct *tsk,
					unsigned long sp,
					struct stack_info *info)
{
	if (on_task_stack(tsk, sp, info))
		return true;
	if (tsk != current || preemptible())
		return false;

	return false;
}

#endif /* _ASM_SW64_STACKTRACE_H */
