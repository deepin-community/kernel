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
