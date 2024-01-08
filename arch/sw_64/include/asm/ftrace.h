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
#define MCOUNT_INSN_SIZE	20	/* 5 * SW64_INSN_SIZE */
#define MCOUNT_LDGP_SIZE	8	/* 2 * SW64_INSN_SIZE */

#define ARCH_SUPPORTS_FTRACE_OPS 1

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
#endif /* _ASM_SW64_FTRACE_H */
