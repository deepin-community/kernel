/* SPDX-License-Identifier: GPL-2.0 */
/*
 * arch/sw_64/include/asm/ftrace.h
 *
 * Copyright (C) 2019, serveros, linyue
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ASM_SW64_FTRACE_H
#define _ASM_SW64_FTRACE_H

#define MCOUNT_ADDR		((unsigned long)_mcount)
#ifdef CONFIG_DYNAMIC_FTRACE
#define MCOUNT_INSN_SIZE	16	/* 4 * SW64_INSN_SIZE */
#else
#define MCOUNT_INSN_SIZE        20      /* 5 * SW64_INSN_SIZE */
#endif
#define MCOUNT_LDGP_SIZE	8	/* 2 * SW64_INSN_SIZE */

#ifdef CONFIG_DYNAMIC_FTRACE_WITH_REGS
#define ARCH_SUPPORTS_FTRACE_OPS 1
#endif

#ifndef __ASSEMBLY__
#include <linux/compat.h>
#include <asm/insn.h>


extern void _mcount(unsigned long);

struct dyn_arch_ftrace {
	/* No extra data needed for sw64 */
};

extern unsigned long ftrace_graph_call;


static inline unsigned long ftrace_call_adjust(unsigned long addr)
{
	/*
	 * addr is the address of the mcount call instruction.
	 * recordmcount does the necessary offset calculation.
	 */
	return addr;
}

#ifdef CONFIG_HAVE_DYNAMIC_FTRACE_WITH_ARGS
struct ftrace_ops;

struct ftrace_regs {
	struct pt_regs regs;
};

static __always_inline struct pt_regs *arch_ftrace_get_regs(struct ftrace_regs *fregs)
{
	return &fregs->regs;
}

static __always_inline unsigned long
ftrace_regs_get_instruction_pointer(const struct ftrace_regs *fregs)
{
	return fregs->regs.regs[28];
}

	static __always_inline void
ftrace_regs_set_instruction_pointer(struct ftrace_regs *fregs, unsigned long ip)
{
	fregs->regs.regs[27] = ip;
	fregs->regs.regs[28] = ip;
}

static __always_inline unsigned long
ftrace_regs_get_stack_pointer(const struct ftrace_regs *fregs)
{
	return fregs->regs.regs[30];
}

static __always_inline unsigned long
ftrace_regs_get_argument(struct ftrace_regs *fregs, unsigned int n)
{
	if (n < 6)
		return fregs->regs.regs[16+n];
	return 0;
}

static __always_inline unsigned long
ftrace_regs_get_return_value(const struct ftrace_regs *fregs)
{
	return fregs->regs.regs[0];
}

static __always_inline void
ftrace_regs_set_return_value(struct ftrace_regs *fregs, unsigned long ret)
{
	fregs->regs.regs[0] = ret;
}

static __always_inline void
ftrace_override_function_with_return(struct ftrace_regs *fregs)
{
	fregs->regs.regs[27] = fregs->regs.regs[26];
	fregs->regs.regs[28] = fregs->regs.regs[26];
}

int ftrace_regs_query_register_offset(const char *name);

#define ftrace_graph_func ftrace_graph_func
void ftrace_graph_func(unsigned long ip, unsigned long parent_ip,
		       struct ftrace_ops *op, struct ftrace_regs *fregs);

#ifdef CONFIG_DYNAMIC_FTRACE_WITH_DIRECT_CALLS
static inline void
__arch_ftrace_set_direct_caller(struct pt_regs *regs, unsigned long addr)
{
	regs->regs[2] = addr;
}

#define arch_ftrace_set_direct_caller(fregs, addr) \
	__arch_ftrace_set_direct_caller(&(fregs)->regs, addr)
#endif /* CONFIG_DYNAMIC_FTRACE_WITH_DIRECT_CALLS */

#endif

#endif /* ifndef __ASSEMBLY__ */

#ifndef __ASSEMBLY__
#ifdef CONFIG_FUNCTION_GRAPH_TRACER
struct fgraph_ret_regs {
	unsigned long ax;
	unsigned long bp;
};

static inline unsigned long fgraph_ret_regs_return_value(struct fgraph_ret_regs *ret_regs)
{
	return ret_regs->ax;
}

static inline unsigned long fgraph_ret_regs_frame_pointer(struct fgraph_ret_regs *ret_regs)
{
	return ret_regs->bp;
}
#endif /* ifdef CONFIG_FUNCTION_GRAPH_TRACER */
#endif

#endif /* _ASM_SW64_FTRACE_H */
