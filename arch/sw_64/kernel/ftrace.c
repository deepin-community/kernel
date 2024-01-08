// SPDX-License-Identifier: GPL-2.0
/*
 * Based on arch/arm64/kernel/ftrace.c
 *
 * Copyright (C) 2019 os kernel team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/ftrace.h>

#include <asm/ftrace.h>

#ifdef CONFIG_FUNCTION_TRACER
EXPORT_SYMBOL(_mcount);
#endif

#ifdef CONFIG_DYNAMIC_FTRACE

#define TI_FTRACE_ADDR	(offsetof(struct thread_info, dyn_ftrace_addr))
#define TI_FTRACE_REGS_ADDR \
			(offsetof(struct thread_info, dyn_ftrace_regs_addr))

unsigned long current_tracer = (unsigned long)ftrace_stub;

/*
 * Replace a single instruction, which may be a branch or NOP.
 */
static int ftrace_modify_code(unsigned long pc, u32 new)
{
	if (sw64_insn_write((void *)pc, new))
		return -EPERM;
	return 0;
}

/*
 * Replace tracer function in ftrace_caller()
 */
int ftrace_update_ftrace_func(ftrace_func_t func)
{
	unsigned long pc;
	u32 new;
	int ret;

	current_tracer = (unsigned long)func;
	pc = (unsigned long)&ftrace_call;
	new = SW64_CALL(R26, R27, 0);
	ret = ftrace_modify_code(pc, new);

	if (!ret) {
		pc = (unsigned long)&ftrace_regs_call;
		new = SW64_CALL(R26, R27, 0);
		ret = ftrace_modify_code(pc, new);
	}

	return ret;
}

/*
 * Turn on the call to ftrace_caller() in instrumented function
 */
int ftrace_make_call(struct dyn_ftrace *rec, unsigned long addr)
{
	unsigned int insn[3];
	unsigned long pc = rec->ip + MCOUNT_LDGP_SIZE;
	unsigned long offset;

	if (addr == FTRACE_ADDR)
		offset = TI_FTRACE_ADDR;
	else
		offset = TI_FTRACE_REGS_ADDR;

	insn[0] = SW64_NOP;
	/* ldl r28,(ftrace_addr_offset)(r8) */
	insn[1] = (0x23U << 26) | (28U << 21) | (8U << 16) | offset;
	insn[2] = SW64_CALL(R28, R28, 0);

	/* replace the 3 mcount instructions at once */
	return copy_to_kernel_nofault((void *)pc, insn, 3 * SW64_INSN_SIZE);
}

/*
 * Turn off the call to ftrace_caller() in instrumented function
 */
int ftrace_make_nop(struct module *mod, struct dyn_ftrace *rec,
		    unsigned long addr)
{
	unsigned long pc = rec->ip + MCOUNT_LDGP_SIZE;
	unsigned int insn[3] = {SW64_NOP, SW64_NOP, SW64_NOP};

	return copy_to_kernel_nofault((void *)pc, insn, 3 * SW64_INSN_SIZE);
}

void arch_ftrace_update_code(int command)
{
	ftrace_modify_all_code(command);
}

int __init ftrace_dyn_arch_init(void)
{
	struct thread_info *ti = task_thread_info(&init_task);

	ti->dyn_ftrace_addr = FTRACE_ADDR;

#ifdef CONFIG_DYNAMIC_FTRACE_WITH_REGS
	ti->dyn_ftrace_regs_addr = FTRACE_REGS_ADDR;
#endif
	return 0;
}
#endif /* CONFIG_DYNAMIC_FTRACE */

#ifdef CONFIG_DYNAMIC_FTRACE_WITH_REGS
int ftrace_modify_call(struct dyn_ftrace *rec, unsigned long old_addr,
		       unsigned long addr)
{
	return 0;
}
#endif

#ifdef CONFIG_FUNCTION_GRAPH_TRACER
/*
 * function_graph tracer expects ftrace_return_to_handler() to be called
 * on the way back to parent. For this purpose, this function is called
 * in _mcount() or ftrace_caller() to replace return address (*parent) on
 * the call stack to return_to_handler.
 *
 * Note that @frame_pointer is used only for sanity check later.
 */
void prepare_ftrace_return(unsigned long *parent, unsigned long self_addr,
			   unsigned long frame_pointer)
{
	unsigned long return_hooker = (unsigned long)&return_to_handler;
	unsigned long old;

	if (unlikely(atomic_read(&current->tracing_graph_pause)))
		return;

	/*
	 * Note:
	 * No protection against faulting at *parent, which may be seen
	 * on other archs. It's unlikely on AArch64.
	 */
	old = *parent;

	if (!function_graph_enter(old, self_addr, frame_pointer, NULL))
		*parent = return_hooker;
}

#ifdef CONFIG_DYNAMIC_FTRACE
/*
 * Turn on/off the call to ftrace_graph_caller() in ftrace_caller()
 * depending on @enable.
 */
static int ftrace_modify_graph_caller(bool enable)
{
	unsigned long pc = (unsigned long)&ftrace_graph_call;
	u32 new = SW64_NOP;

	if (enable)
		new = SW64_CALL(R26, R27, 0);
	return ftrace_modify_code(pc, new);
}

int ftrace_enable_ftrace_graph_caller(void)
{
	return ftrace_modify_graph_caller(true);
}

int ftrace_disable_ftrace_graph_caller(void)
{
	return ftrace_modify_graph_caller(false);
}
#endif /* CONFIG_DYNAMIC_FTRACE */
#endif /* CONFIG_FUNCTION_GRAPH_TRACER */
