/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_PERF_EVENT_H
#define _ASM_SW64_PERF_EVENT_H

#if defined(CONFIG_SUBARCH_C3B)
#include <asm/pmc.h>
#elif defined(CONFIG_SUBARCH_C4)
#include <asm/pmc_c4.h>
#endif
#include <asm/ptrace.h>
#include <linux/perf_event.h>
#include <linux/interrupt.h>

#ifdef CONFIG_PERF_EVENTS
struct pt_regs;
extern unsigned long perf_instruction_pointer(struct pt_regs *regs);
extern unsigned long perf_misc_flags(struct pt_regs *regs);
#define perf_misc_flags(regs)  perf_misc_flags(regs)
#define perf_arch_bpf_user_pt_regs(regs) &regs->user_regs
#endif

/* For tracking PMCs and the hw events they monitor on each CPU. */
struct cpu_hw_events {
	/*
	 * Set the bit (indexed by the counter number) when the counter
	 * is used for an event.
	 */
	unsigned long		used_mask[BITS_TO_LONGS(MAX_HWEVENTS)];
	/* Array of events current scheduled on this cpu. */
	struct perf_event	*event[MAX_HWEVENTS];
};

#endif /* _ASM_SW64_PERF_EVENT_H */
