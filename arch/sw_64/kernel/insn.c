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

//static DEFINE_RAW_SPINLOCK(patch_lock);

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
	void *waddr = addr;
	int ret;

	//raw_spin_lock_irqsave(&patch_lock, flags);

	ret = copy_to_kernel_nofault(waddr, &insn, SW64_INSN_SIZE);

	//raw_spin_unlock_irqrestore(&patch_lock, flags);

	return ret;
}

static int __kprobes __sw64_insn_double_write(void *addr, __le64 insn)
{
	void *waddr = addr;
	//unsigned long flags = 0;
	int ret;

	//raw_spin_lock_irqsave(&patch_lock, flags);

	ret = copy_to_kernel_nofault(waddr, &insn, 2 * SW64_INSN_SIZE);

	//raw_spin_unlock_irqrestore(&patch_lock, flags);

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
