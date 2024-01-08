/* SPDX-License-Identifier: GPL-2.0 */
/*
 *  Kernel Probes (KProbes)
 *  Based on arch/mips/include/asm/kprobes.h
 */

#ifndef _ASM_SW64_KPROBES_H
#define _ASM_SW64_KPROBES_H

#include <asm-generic/kprobes.h>

#define BREAK_KPROBE	0x40ffffff
#define BREAK_KPROBE_SS	0x40fffeff

#ifdef CONFIG_KPROBES
#include <linux/ptrace.h>
#include <linux/types.h>

#include <asm/cacheflush.h>
#include <asm/kdebug.h>

#define __ARCH_WANT_KPROBES_INSN_SLOT

struct kprobe;
struct pt_regs;

typedef u32 kprobe_opcode_t;

#define MAX_INSN_SIZE 2

#define flush_insn_slot(p)						\
do {									\
	if (p->addr)							\
		flush_icache_range((unsigned long)p->addr,		\
			(unsigned long)p->addr +			\
			(MAX_INSN_SIZE * sizeof(kprobe_opcode_t)));	\
} while (0)


#define kretprobe_blacklist_size 0

void arch_remove_kprobe(struct kprobe *p);

/* Architecture specific copy of original instruction*/
struct arch_specific_insn {
	/* copy of the original instruction */
	kprobe_opcode_t *insn;
	/*
	 * Set in kprobes code, initially to 0. If the instruction can be
	 * eumulated, this is set to 1, if not, to -1.
	 */
	int boostable;
};

struct prev_kprobe {
	struct kprobe *kp;
	unsigned long status;
};

#define SKIP_DELAYSLOT 0x0001

/* per-cpu kprobe control block */
struct kprobe_ctlblk {
	unsigned long kprobe_status;
	/* Per-thread fields, used while emulating branches */
	unsigned long flags;
	unsigned long target_pc;
	struct prev_kprobe prev_kprobe;
};
extern int kprobe_handler(struct pt_regs *regs);
extern int post_kprobe_handler(struct pt_regs *regs);
extern int kprobe_fault_handler(struct pt_regs *regs, unsigned long mmcsr);


#endif /* CONFIG_KPROBES */
#endif /* _ASM_SW64_KPROBES_H */
