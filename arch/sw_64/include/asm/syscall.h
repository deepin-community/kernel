/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_SYSCALL_H
#define _ASM_SW64_SYSCALL_H

#include <uapi/linux/audit.h>

#ifndef __ASSEMBLY__

typedef long (*syscall_fn_t)(ulong, ulong, ulong, ulong, ulong, ulong);

extern syscall_fn_t sys_call_table[];

static inline int syscall_get_nr(struct task_struct *task,
				 struct pt_regs *regs)
{
	return regs->regs[0];
}

static inline long
syscall_get_error(struct task_struct *task, struct pt_regs *regs)
{
	return regs->regs[19] ? -regs->regs[0] : 0;
}

static inline long syscall_get_return_value(struct task_struct *task,
					    struct pt_regs *regs)
{
	return regs->regs[0];
}

static inline void syscall_set_return_value(struct task_struct *task,
					    struct pt_regs *regs,
					    int error, long val)
{
	if (error) {
		regs->regs[0]  = -error;
		regs->regs[19] = 1;
	} else {
		regs->regs[0] = val;
		regs->regs[19] = 0;
	}
}

static inline void syscall_rollback(struct task_struct *task,
				    struct pt_regs *regs)
{
	regs->regs[0] = regs->orig_r0;
	regs->regs[19] = regs->orig_r19;
}

static inline void syscall_get_arguments(struct task_struct *task,
					 struct pt_regs *regs,
					 unsigned long *args)
{
	*args++ = regs->regs[16];
	*args++ = regs->regs[17];
	*args++ = regs->regs[18];
	*args++ = regs->regs[19];
	*args++ = regs->regs[20];
	*args	= regs->regs[21];
}

static inline void syscall_set_arguments(struct task_struct *task,
					 struct pt_regs *regs,
					 const unsigned long *args)
{
	regs->regs[16] = *args++;
	regs->regs[17] = *args++;
	regs->regs[18] = *args++;
	regs->regs[19] = *args++;
	regs->regs[20] = *args++;
	regs->regs[21] = *args;
}

static inline int syscall_get_arch(struct task_struct *task)
{
	return AUDIT_ARCH_SW64;
}

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_SW64_SYSCALL_H */
