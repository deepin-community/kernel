/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sw64 KGDB support
 *
 * Based on arch/arm64/include/kgdb.h
 *
 * Copyright (C) Xia Bin
 * Author: Xia Bin
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
#ifndef _ASM_SW64_KGDB_H
#define _ASM_SW64_KGDB_H

#include <asm/ptrace.h>
#include <linux/sched.h>

#ifndef __ASSEMBLY__


#define GDB_ADJUSTS_BREAK_OFFSET
#define BREAK_INSTR_SIZE	4
#define CACHE_FLUSH_IS_SAFE	0

static inline void arch_kgdb_breakpoint(void)
{
	__asm__ __volatile__("sys_call %0" : : "i"(HMC_bpt));
}

void sw64_task_to_gdb_regs(struct task_struct *task, unsigned long *regs);

extern void kgdb_handle_bus_error(void);
extern int kgdb_fault_expected;
extern unsigned long get_reg(struct task_struct *task, unsigned long regno);

#endif /* !__ASSEMBLY__ */

/*
 * general purpose registers size in bytes.
 */
#define DBG_MAX_REG_NUM		(67)

/*
 * Size of I/O buffer for gdb packet.
 * considering to hold all register contents, size is set
 */

#define BUFMAX			4096

/*
 * Number of bytes required for gdb_regs buffer.
 * _GP_REGS: 8 bytes, _FP_REGS: 16 bytes and _EXTRA_REGS: 4 bytes each
 * GDB fails to connect for size beyond this with error
 * "'g' packet reply is too long"
 */
#define NUMREGBYTES		(DBG_MAX_REG_NUM * 8)

#endif /* _ASM_SW64_KGDB_H */
