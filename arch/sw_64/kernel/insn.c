// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019, serveros, linyue
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/spinlock.h>
#include <linux/kprobes.h>

#include <asm/fixmap.h>

#ifdef CONFIG_SW64_KERNEL_PAGE_TABLE
static DEFINE_RAW_SPINLOCK(patch_lock);

static void __kprobes *patch_map(void *addr, int fixmap)
{
	unsigned long uintaddr = (uintptr_t)addr;
	struct page *page;

	if (core_kernel_text((unsigned long)addr))
		page = pfn_to_page(PHYS_PFN(__pa(addr)));
	else if (IS_ENABLED(CONFIG_STRICT_MODULE_RWX))
		page = vmalloc_to_page(addr);
	else
		return addr;

	BUG_ON(!page);
	return (void *)set_fixmap_offset(fixmap, page_to_pa(page) +
					 (uintaddr & ~PAGE_MASK));
}

static void __kprobes patch_unmap(int fixmap)
{
	clear_fixmap(fixmap);
}

int __kprobes sw64_patch_text_nosync(void *addr, u32 insn)
{
	return sw64_insn_write(addr, insn);
}
#endif /* CONFIG_SW64_KERNEL_PAGE_TABLE */

int __kprobes sw64_insn_read(void *addr, u32 *insnp)
{
	int ret;
	__le32 val;

	ret = copy_from_kernel_nofault(&val, addr, SW64_INSN_SIZE);
	if (!ret)
		*insnp = le32_to_cpu(val);

	return ret;
}

static int __kprobes __sw64_insn_write(void *addr, __le32 insn)
{
	int ret;
#ifdef CONFIG_SW64_KERNEL_PAGE_TABLE
	void *waddr;
	unsigned long flags = 0;

	raw_spin_lock_irqsave(&patch_lock, flags);
	waddr = patch_map(addr, FIX_TEXT_POKE0);
	ret = copy_to_kernel_nofault(waddr, &insn, SW64_INSN_SIZE);
	patch_unmap(FIX_TEXT_POKE0);
	raw_spin_unlock_irqrestore(&patch_lock, flags);
#else
	ret = copy_to_kernel_nofault(addr, &insn, SW64_INSN_SIZE);
#endif
	return ret;
}

static int __kprobes __sw64_insn_double_write(void *addr, __le64 insn)
{
	int ret;
#ifdef CONFIG_SW64_KERNEL_PAGE_TABLE
	void *waddr;
	unsigned long flags = 0;

	raw_spin_lock_irqsave(&patch_lock, flags);
	waddr = patch_map(addr, FIX_TEXT_POKE0);
	ret = copy_to_kernel_nofault(waddr, &insn, 2 * SW64_INSN_SIZE);
	patch_unmap(FIX_TEXT_POKE0);
	raw_spin_unlock_irqrestore(&patch_lock, flags);
#else
	ret = copy_to_kernel_nofault(addr, &insn, 2 * SW64_INSN_SIZE);
#endif
	return ret;
}

int __kprobes sw64_insn_write(void *addr, u32 insn)
{
	u32 *tp = addr;
	/* SW64 instructions must be word aligned */
	if ((uintptr_t)tp & 0x3)
		return -EINVAL;
	return __sw64_insn_write(addr, cpu_to_le32(insn));
}

int __kprobes sw64_insn_double_write(void *addr, u64 insn)
{
	u32 *tp = addr;
	/* SW64 instructions must be word aligned */
	if ((uintptr_t)tp & 0x3)
		return -EINVAL;
	return __sw64_insn_double_write(addr, cpu_to_le64(insn));
}

unsigned int __kprobes sw64_insn_nop(void)
{
	return SW64_BIS(R31, R31, R31);
}

unsigned int __kprobes sw64_insn_call(unsigned int ra, unsigned int rb)
{
	return SW64_CALL(ra, rb, 0);
}

unsigned int __kprobes sw64_insn_sys_call(unsigned int num)
{
	return  SW64_SYS_CALL(num);
}

/* 'pc' is the address of br instruction, not the +4 PC. 'new_pc' is the target address. */
unsigned int __kprobes sw64_insn_br(unsigned int ra, unsigned long pc, unsigned long new_pc)
{
	int offset = new_pc - pc;
	unsigned int disp, minus = 0x1fffff;

	if (!(offset <= BR_MAX_DISP && offset >= -BR_MAX_DISP))
		return -1;
	if (offset > 0)
		disp = (offset - 4) / 4;
	else
		disp = ~(-offset / 4) & minus;

	return SW64_BR(ra, disp);

}
