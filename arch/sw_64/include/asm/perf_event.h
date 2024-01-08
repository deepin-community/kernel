/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_PERF_EVENT_H
#define _ASM_SW64_PERF_EVENT_H

#include <asm/pmc.h>
#include <asm/ptrace.h>

#ifdef CONFIG_PERF_EVENTS
struct pt_regs;
extern unsigned long perf_instruction_pointer(struct pt_regs *regs);
extern unsigned long perf_misc_flags(struct pt_regs *regs);
#define perf_misc_flags(regs)  perf_misc_flags(regs)
#define perf_arch_bpf_user_pt_regs(regs) &regs->user_regs
#endif

#endif /* _ASM_SW64_PERF_EVENT_H */
