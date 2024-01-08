/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _SW64_KERNEL_PROTO_H
#define _SW64_KERNEL_PROTO_H

#include <linux/interrupt.h>
#include <linux/io.h>
#include <asm/pgtable.h>
#include <asm/sw64io.h>

/* ptrace.c */
extern int ptrace_set_bpt(struct task_struct *child);
extern int ptrace_cancel_bpt(struct task_struct *child);

/* traps.c */
extern void show_regs(struct pt_regs *regs);
extern void die(char *str, struct pt_regs *regs, long err);

#endif /* _SW64_KERNEL_PROTO_H */
