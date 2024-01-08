/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_SWITCH_TO_H
#define _ASM_SW64_SWITCH_TO_H

#include <linux/sched.h>

extern void __fpstate_save(struct task_struct *save_to);
extern void __fpstate_restore(struct task_struct *restore_from);
extern struct task_struct *__switch_to(struct task_struct *prev,
				       struct task_struct *next);
extern void restore_da_match_after_sched(void);

static inline void aux_save(struct task_struct *task)
{
	struct pcb_struct *pcb;

	if (likely(!(task->flags & PF_KTHREAD))) {
		pcb = &task_thread_info(task)->pcb;
		pcb->tp = rtid();
		__fpstate_save(task);
	}
}

static inline void aux_restore(struct task_struct *task)
{
	struct pcb_struct *pcb;

	if (likely(!(task->flags & PF_KTHREAD))) {
		pcb = &task_thread_info(task)->pcb;
		wrtp(pcb->tp);
		__fpstate_restore(task);
	}
}

static inline void __switch_to_aux(struct task_struct *prev,
				   struct task_struct *next)
{
	aux_save(prev);
	aux_restore(next);
}


#define switch_to(prev, next, last)					\
do {									\
	struct task_struct *__prev = (prev);				\
	struct task_struct *__next = (next);				\
	__switch_to_aux(__prev, __next);				\
	(last) = __switch_to(__prev, __next);				\
} while (0)


/* TODO: finish_arch_switch has been removed from arch-independent code. */

/*
 * finish_arch_switch will be called after switch_to
 */
#define finish_arch_post_lock_switch		restore_da_match_after_sched


#endif /* _ASM_SW64_SWITCH_TO_H */
