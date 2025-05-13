/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ASM_SW64_JUMP_LABEL_H
#define _ASM_SW64_JUMP_LABEL_H

#ifndef __ASSEMBLY__

#include <linux/types.h>
#include <asm/insn.h>

#define JUMP_LABEL_NOP_SIZE		SW64_INSN_SIZE

static __always_inline bool arch_static_branch(struct static_key *key, bool branch)
{
	asm goto("1: nop\n\t"
		".pushsection __jump_table,  \"aw\"\n\t"
		".align 3\n\t"
		".quad 1b, %l[l_yes], %0\n\t"
		".popsection\n\t"
		:  :  "i"(&((char *)key)[branch]) :  : l_yes);

	return false;
l_yes:
	return true;
}

static __always_inline bool arch_static_branch_jump(struct static_key *key, bool branch)
{
	asm goto("1: br %l[l_yes]\n\t"
		".pushsection __jump_table,  \"aw\"\n\t"
		".align 3\n\t"
		".quad 1b, %l[l_yes], %0\n\t"
		".popsection\n\t"
		:  :  "i"(&((char *)key)[branch]) :  : l_yes);

	return false;
l_yes:
	return true;
}

typedef u64 jump_label_t;

struct jump_entry {
	jump_label_t code;
	jump_label_t target;
	jump_label_t key;
};

#endif /* __ASSEMBLY__ */
#endif /* _ASM_SW64_JUMP_LABEL_H */
