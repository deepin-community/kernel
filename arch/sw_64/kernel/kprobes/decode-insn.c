// SPDX-License-Identifier: GPL-2.0
/*
 * Based on arch/arm64/kernel/probes/decode-insn.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/kprobes.h>

#include "common.h"

static bool __kprobes sw64_insn_is_steppable(u32 insn)
{
	/*
	 * Branch instructions will write a new value into the PC which is
	 * likely to be relative to the XOL address and therefore invalid.
	 * Deliberate generation of an exception during stepping is also not
	 * currently safe. Lastly, MSR instructions can do any number of nasty
	 * things we can't handle during single-stepping.
	 */
	if (sw64_insn_is_sys_call_b(insn) ||
		sw64_insn_is_sys_call(insn) ||
		sw64_insn_is_call(insn) ||
		sw64_insn_is_ret(insn) ||
		sw64_insn_is_jmp(insn) ||
		sw64_insn_is_br(insn) ||
		sw64_insn_is_bsr(insn) ||
		sw64_insn_is_memb(insn) ||
		sw64_insn_is_imemb(insn) ||
		sw64_insn_is_rtc(insn) ||
		sw64_insn_is_lldl(insn) ||
		sw64_insn_is_lldw(insn) ||
		sw64_insn_is_beq(insn) ||
		sw64_insn_is_bne(insn) ||
		sw64_insn_is_blt(insn) ||
		sw64_insn_is_ble(insn) ||
		sw64_insn_is_bgt(insn) ||
		sw64_insn_is_bge(insn) ||
		sw64_insn_is_blbc(insn) ||
		sw64_insn_is_blbs(insn) ||
		sw64_insn_is_fbeq(insn) ||
		sw64_insn_is_fbne(insn) ||
		sw64_insn_is_fblt(insn) ||
		sw64_insn_is_fble(insn) ||
		sw64_insn_is_fbgt(insn) ||
		sw64_insn_is_fbge(insn))
		return false;

	return true;
}


#ifdef CONFIG_KPROBES
#ifdef CONFIG_SUBARCH_C3B
//  lldl  rd_f
static bool __kprobes is_probed_between_atomic(kprobe_opcode_t *addr)
{
	int count = 0;
	unsigned long size = 0, offset = 0;
	kprobe_opcode_t *scan_start = NULL;

	if (kallsyms_lookup_size_offset((unsigned long)addr, &size, &offset))
		scan_start = addr - (offset / sizeof(kprobe_opcode_t));

	while (scan_start < addr) {
		if (sw64_insn_is_lldl(le32_to_cpu(*scan_start)) ||
				sw64_insn_is_lldw(le32_to_cpu(*scan_start)))
			count++;
		if (sw64_insn_is_rd_f(le32_to_cpu(*scan_start)))
			count--;
		scan_start++;
	}
	if (count)
		return false;

	return true;
}
#endif

bool __kprobes sw64_insn_can_kprobe(kprobe_opcode_t *addr)
{
	u32 insn = le32_to_cpu(*addr);

	if (!sw64_insn_is_steppable(insn)) {
		pr_warn("addr is not steppable\n");
		return false;
	}
#ifdef CONFIG_SUBARCH_C3B
	if (!is_probed_between_atomic(addr)) {
		pr_warn("addr between atomic can't probe\n");
		return false;
	}
#endif
	return true;
}
#endif
