/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#ifndef _ASM_SW64_UPROBES_H
#define _ASM_SW64_UPROBES_H

#include <linux/notifier.h>
#include <linux/types.h>
#include <asm/insn.h>

/*
 * We want this to be defined as union sw64_instruction but that makes the
 * generic code blow up.
 */
typedef u32 uprobe_opcode_t;

#define MAX_UINSN_BYTES			SW64_INSN_SIZE
#define UPROBE_XOL_SLOT_BYTES		SW64_INSN_SIZE

#define UPROBE_BRK_UPROBE		0x000d000d	/* break 13 */
#define UPROBE_BRK_UPROBE_XOL		0x000e000d	/* break 14 */

#define UPROBE_SWBP_INSN		UPROBE_BRK_UPROBE
#define UPROBE_SWBP_INSN_SIZE		MAX_UINSN_BYTES

struct arch_uprobe {
	u32	insn;
	u32	ixol[2];
};

struct arch_uprobe_task {
	unsigned long saved_trap_nr;
};

#ifdef CONFIG_UPROBES
void sw64_fix_uretprobe(struct pt_regs *regs, unsigned long exc_pc);
#else
static inline void
sw64_fix_uretprobe(struct pt_regs *regs, unsigned long exc_pc) {}
#endif

#endif /* _ASM_SW64_UPROBES_H */
