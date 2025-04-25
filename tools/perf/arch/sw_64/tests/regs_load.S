/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/linkage.h>

.text
.set noat
.globl perf_regs_load
#define STL_REG(r)	stl $r, (8 * r)($16)
#define LDL_REG(r)	ldl $r, (8 * r)($16)
#define SP	(8 * 30)
#define PC	(8 * 31)
perf_regs_load:
	STL_REG(0)
	STL_REG(1)
	STL_REG(2)
	STL_REG(3)
	STL_REG(4)
	STL_REG(5)
	STL_REG(6)
	STL_REG(7)
	STL_REG(8)
	STL_REG(9)
	STL_REG(10)
	STL_REG(11)
	STL_REG(12)
	STL_REG(13)
	STL_REG(14)
	STL_REG(15)
	STL_REG(16)
	STL_REG(17)
	STL_REG(18)
	STL_REG(19)
	STL_REG(20)
	STL_REG(21)
	STL_REG(22)
	STL_REG(23)
	STL_REG(24)
	STL_REG(25)
	STL_REG(26)
	STL_REG(27)
	STL_REG(28)
	STL_REG(29)
	mov $30, $17
	stl $17, (SP)($16)
	stl $26, (PC)($16)
	LDL_REG(17)
	ret
.end perf_regs_load
