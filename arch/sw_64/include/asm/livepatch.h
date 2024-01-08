/* SPDX-License-Identifier: GPL-2.0 */
/*
 * livepatch.h - sw64-specific Kernel Live Patching Core
 */

#ifndef _ASM_SW64_LIVEPATCH_H
#define _ASM_SW64_LIVEPATCH_H

#include <asm/ptrace.h>

static inline int klp_check_compiler_support(void)
{
	return 0;
}

static inline void klp_arch_set_pc(struct pt_regs *regs, unsigned long ip)
{
	regs->regs[27] = ip;
	regs->regs[28] = ip;
}

#endif /* _ASM_SW64_LIVEPATCH_H */
