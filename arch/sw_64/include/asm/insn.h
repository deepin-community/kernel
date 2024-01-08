/* SPDX-License-Identifier: GPL-2.0 */
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
#ifndef _ASM_SW64_INSN_H
#define _ASM_SW64_INSN_H
#include <linux/types.h>

/* Register numbers */
enum {
	R26 = 26,
	R27,
	R28,
	R31 = 31,
};

#define BR_MAX_DISP		0xfffff
/* SW64 instructions are always 32 bits. */
#define SW64_INSN_SIZE		4

#define ___SW64_RA(a)		(((a) & 0x1f) << 21)
#define ___SW64_RB(b)		(((b) & 0x1f) << 16)
#define ___SW64_SIMP_RC(c)	(((c) & 0x1f))
#define ___SW64_ST_DISP(disp)	(((disp) & 0xffff))
#define ___SW64_SYSCALL_FUNC(func)	((func) & 0xff)
#define ___SW64_BR_DISP(disp)	(((disp) & 0x1fffff))


#define SW64_INSN_BIS		0x40000740
#define SW64_INSN_CALL		0x04000000
#define SW64_INSN_SYS_CALL	0x02000000
#define SW64_INSN_BR		0x10000000

#define SW64_NOP		(0x43ff075f)
#define SW64_BIS(a, b, c)	(SW64_INSN_BIS | ___SW64_RA(a)	| ___SW64_RB(b) | ___SW64_SIMP_RC(c))
#define SW64_CALL(a, b, disp)	(SW64_INSN_CALL | ___SW64_RA(a)	| ___SW64_RB(b) | ___SW64_ST_DISP(disp))
#define SW64_SYS_CALL(func)	(SW64_INSN_SYS_CALL | ___SW64_SYSCALL_FUNC(func))
#define SW64_BR(a, disp)	(SW64_INSN_BR | ___SW64_RA(a) | ___SW64_BR_DISP(disp))

extern int sw64_insn_read(void *addr, u32 *insnp);
extern int sw64_insn_write(void *addr, u32 insn);
extern int sw64_insn_double_write(void *addr, u64 insn);
extern unsigned int sw64_insn_nop(void);
extern unsigned int sw64_insn_call(unsigned int ra, unsigned int rb);
extern unsigned int sw64_insn_sys_call(unsigned int num);
extern unsigned int sw64_insn_br(unsigned int ra, unsigned long pc, unsigned long new_pc);

#define SW64_OPCODE_RA(opcode)	((opcode >> 21) & 0x1f)

#define SW64_INSN(name, opcode, mask)			\
static  inline  bool sw64_insn_is_##name(u32 insn)	\
{							\
	return (insn & mask) == opcode;			\
}

SW64_INSN(sys_call_b,	0x00000000, 0xfc000000);
SW64_INSN(sys_call,	0x00000001, 0xfc000000);
SW64_INSN(call,		0x04000000, 0xfc000000);
SW64_INSN(ret,		0x08000000, 0xfc000000);
SW64_INSN(jmp,		0x0c000000, 0xfc000000);
SW64_INSN(br,		0x10000000, 0xfc000000);
SW64_INSN(bsr,		0x14000000, 0xfc000000);
SW64_INSN(memb,		0x18000000, 0xfc00ffff);
SW64_INSN(imemb,	0x18000001, 0xfc00ffff);
SW64_INSN(rtc,		0x18000020, 0xfc00ffff);
SW64_INSN(halt,		0x18000080, 0xfc00ffff);
SW64_INSN(rd_f,		0x18001000, 0xfc00ffff);
SW64_INSN(beq,		0xc0000000, 0xfc000000);
SW64_INSN(bne,		0xc4000000, 0xfc000000);
SW64_INSN(blt,		0xc8000000, 0xfc000000);
SW64_INSN(ble,		0xcc000000, 0xfc000000);
SW64_INSN(bgt,		0xd0000000, 0xfc000000);
SW64_INSN(bge,		0xd4000000, 0xfc000000);
SW64_INSN(blbc,		0xd8000000, 0xfc000000);
SW64_INSN(blbs,		0xdc000000, 0xfc000000);
SW64_INSN(fbeq,		0xe0000000, 0xfc000000);
SW64_INSN(fbne,		0xe4000000, 0xfc000000);
SW64_INSN(fblt,		0xe8000000, 0xfc000000);
SW64_INSN(fble,		0xec000000, 0xfc000000);
SW64_INSN(fbgt,		0xf0000000, 0xfc000000);
SW64_INSN(fbge,		0xf4000000, 0xfc000000);
SW64_INSN(lldw,		0x20000000, 0xfc00f000);
SW64_INSN(lldl,		0x20001000, 0xfc00f000);

#endif /* _ASM_SW64_INSN_H */
