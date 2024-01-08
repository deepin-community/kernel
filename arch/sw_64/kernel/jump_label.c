// SPDX-License-Identifier: GPL-2.0

#include <linux/jump_label.h>

#include <asm/bug.h>
#include <asm/cacheflush.h>

void arch_jump_label_transform(struct jump_entry *entry,
			       enum jump_label_type type)
{
	u32 *insnp = (u32 *)entry->code;
	u32 insn;

	if (type == JUMP_LABEL_JMP) {
		insn = sw64_insn_br(R31, (entry->code), entry->target);
		BUG_ON(insn == -1);
	} else {
		insn = sw64_insn_nop();
	}

	*insnp = insn;

	flush_icache_range(entry->code, entry->code + SW64_INSN_SIZE);
}

void arch_jump_label_transform_static(struct jump_entry *entry,
				      enum jump_label_type type)
{
	/*
	 * no need to rewrite NOP
	 */
}
