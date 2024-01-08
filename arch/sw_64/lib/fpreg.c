// SPDX-License-Identifier: GPL-2.0
/*
 * (C) Copyright 1998 Linus Torvalds
 */

#include <linux/gfp.h>
#include <linux/export.h>

#define STT(reg, val)		\
	asm volatile("fimovd $f"#reg", %0" : "=r"(val))
#define STS(reg, val)		\
	asm volatile("fimovs $f"#reg", %0" : "=r"(val))
#define LDT(reg, val)		\
	asm volatile("ifmovd %0, $f"#reg : : "r"(val))
#define LDS(reg, val)		\
	asm volatile("ifmovs %0, $f"#reg : : "r"(val))
#define VLDD(reg, val)		\
	asm volatile("vldd $f"#reg", %0" : : "m"(val) : "memory")
#define VSTD(reg, val)		\
	asm volatile("vstd $f"#reg", %0" : "=m"(val) : : "memory")
#define VLDS(reg, val)		\
	asm volatile("vlds $f"#reg", %0" : : "m"(val) : "memory")
#define LDWE(reg, val)		\
	asm volatile("ldwe $f"#reg", %0" : : "m"(val) : "memory")
#define VSTS(reg, val)		\
	asm volatile("vsts $f"#reg", %0" : "=m"(val) : : "memory")
#define STDH(reg, val)		\
	asm volatile("vstd $f"#reg", %0" : "=m"(val) : : "memory")

void
sw64_write_simd_fp_reg_s(unsigned long reg, unsigned long f0, unsigned long f1)
{

	unsigned long tmpa[4] __aligned(16);

	tmpa[0] = f0;
	tmpa[1] = f1;

	switch (reg) {
	case  0:
		VLDS(0, *tmpa);
		break;
	case  1:
		VLDS(1, *tmpa);
		break;
	case  2:
		VLDS(2, *tmpa);
		break;
	case  3:
		VLDS(3, *tmpa);
		break;
	case  4:
		VLDS(4, *tmpa);
		break;
	case  5:
		VLDS(5, *tmpa);
		break;
	case  6:
		VLDS(6, *tmpa);
		break;
	case  7:
		VLDS(7, *tmpa);
		break;
	case  8:
		VLDS(8, *tmpa);
		break;
	case  9:
		VLDS(9, *tmpa);
		break;
	case 10:
		VLDS(10, *tmpa);
		break;
	case 11:
		VLDS(11, *tmpa);
		break;
	case 12:
		VLDS(12, *tmpa);
		break;
	case 13:
		VLDS(13, *tmpa);
		break;
	case 14:
		VLDS(14, *tmpa);
		break;
	case 15:
		VLDS(15, *tmpa);
		break;
	case 16:
		VLDS(16, *tmpa);
		break;
	case 17:
		VLDS(17, *tmpa);
		break;
	case 18:
		VLDS(18, *tmpa);
		break;
	case 19:
		VLDS(19, *tmpa);
		break;
	case 20:
		VLDS(20, *tmpa);
		break;
	case 21:
		VLDS(21, *tmpa);
		break;
	case 22:
		VLDS(22, *tmpa);
		break;
	case 23:
		VLDS(23, *tmpa);
		break;
	case 24:
		VLDS(24, *tmpa);
		break;
	case 25:
		VLDS(25, *tmpa);
		break;
	case 26:
		VLDS(26, *tmpa);
		break;
	case 27:
		VLDS(27, *tmpa);
		break;
	case 28:
		VLDS(28, *tmpa);
		break;
	case 29:
		VLDS(29, *tmpa);
		break;
	case 30:
		VLDS(30, *tmpa);
		break;
	case 31:
		break;
	}

}


void sw64_write_simd_fp_reg_d(unsigned long reg, unsigned long f0,
		unsigned long f1, unsigned long f2, unsigned long f3)
{
	unsigned long tmpa[4] __aligned(32);

	tmpa[0] = f0;
	tmpa[1] = f1;
	tmpa[2] = f2;
	tmpa[3] = f3;

	switch (reg) {
	case  0:
		VLDD(0, *tmpa);
		break;
	case  1:
		VLDD(1, *tmpa);
		break;
	case  2:
		VLDD(2, *tmpa);
		break;
	case  3:
		VLDD(3, *tmpa);
		break;
	case  4:
		VLDD(4, *tmpa);
		break;
	case  5:
		VLDD(5, *tmpa);
		break;
	case  6:
		VLDD(6, *tmpa);
		break;
	case  7:
		VLDD(7, *tmpa);
		break;
	case  8:
		VLDD(8, *tmpa);
		break;
	case  9:
		VLDD(9, *tmpa);
		break;
	case 10:
		VLDD(10, *tmpa);
		break;
	case 11:
		VLDD(11, *tmpa);
		break;
	case 12:
		VLDD(12, *tmpa);
		break;
	case 13:
		VLDD(13, *tmpa);
		break;
	case 14:
		VLDD(14, *tmpa);
		break;
	case 15:
		VLDD(15, *tmpa);
		break;
	case 16:
		VLDD(16, *tmpa);
		break;
	case 17:
		VLDD(17, *tmpa);
		break;
	case 18:
		VLDD(18, *tmpa);
		break;
	case 19:
		VLDD(19, *tmpa);
		break;
	case 20:
		VLDD(20, *tmpa);
		break;
	case 21:
		VLDD(21, *tmpa);
		break;
	case 22:
		VLDD(22, *tmpa);
		break;
	case 23:
		VLDD(23, *tmpa);
		break;
	case 24:
		VLDD(24, *tmpa);
		break;
	case 25:
		VLDD(25, *tmpa);
		break;
	case 26:
		VLDD(26, *tmpa);
		break;
	case 27:
		VLDD(27, *tmpa);
		break;
	case 28:
		VLDD(28, *tmpa);
		break;
	case 29:
		VLDD(29, *tmpa);
		break;
	case 30:
		VLDD(30, *tmpa);
		break;
	case 31:
		break;
	}


}


void sw64_write_simd_fp_reg_ldwe(unsigned long reg, int a)
{
	switch (reg) {
	case  0:
		LDWE(0, a);
		break;
	case  1:
		LDWE(1, a);
		break;
	case  2:
		LDWE(2, a);
		break;
	case  3:
		LDWE(3, a);
		break;
	case  4:
		LDWE(4, a);
		break;
	case  5:
		LDWE(5, a);
		break;
	case  6:
		LDWE(6, a);
		break;
	case  7:
		LDWE(7, a);
		break;
	case  8:
		LDWE(8, a);
		break;
	case  9:
		LDWE(9, a);
		break;
	case 10:
		LDWE(10, a);
		break;
	case 11:
		LDWE(11, a);
		break;
	case 12:
		LDWE(12, a);
		break;
	case 13:
		LDWE(13, a);
		break;
	case 14:
		LDWE(14, a);
		break;
	case 15:
		LDWE(15, a);
		break;
	case 16:
		LDWE(16, a);
		break;
	case 17:
		LDWE(17, a);
		break;
	case 18:
		LDWE(18, a);
		break;
	case 19:
		LDWE(19, a);
		break;
	case 20:
		LDWE(20, a);
		break;
	case 21:
		LDWE(21, a);
		break;
	case 22:
		LDWE(22, a);
		break;
	case 23:
		LDWE(23, a);
		break;
	case 24:
		LDWE(24, a);
		break;
	case 25:
		LDWE(25, a);
		break;
	case 26:
		LDWE(26, a);
		break;
	case 27:
		LDWE(27, a);
		break;
	case 28:
		LDWE(28, a);
		break;
	case 29:
		LDWE(29, a);
		break;
	case 30:
		LDWE(30, a);
		break;
	case 31:
		break;
	}
}


void sw64_read_simd_fp_m_s(unsigned long reg, unsigned long *fp_value)
{
	volatile unsigned long tmpa[2] __aligned(16);

	switch (reg) {
	case  0:
		VSTS(0, *tmpa);
		break;
	case  1:
		VSTS(1, *tmpa);
		break;
	case  2:
		VSTS(2, *tmpa);
		break;
	case  3:
		VSTS(3, *tmpa);
		break;
	case  4:
		VSTS(4, *tmpa);
		break;
	case  5:
		VSTS(5, *tmpa);
		break;
	case  6:
		VSTS(6, *tmpa);
		break;
	case  7:
		VSTS(7, *tmpa);
		break;
	case  8:
		VSTS(8, *tmpa);
		break;
	case  9:
		VSTS(9, *tmpa);
		break;
	case 10:
		VSTS(10, *tmpa);
		break;
	case 11:
		VSTS(11, *tmpa);
		break;
	case 12:
		VSTS(12, *tmpa);
		break;
	case 13:
		VSTS(13, *tmpa);
		break;
	case 14:
		VSTS(14, *tmpa);
		break;
	case 15:
		VSTS(15, *tmpa);
		break;
	case 16:
		VSTS(16, *tmpa);
		break;
	case 17:
		VSTS(17, *tmpa);
		break;
	case 18:
		VSTS(18, *tmpa);
		break;
	case 19:
		VSTS(19, *tmpa);
		break;
	case 20:
		VSTS(20, *tmpa);
		break;
	case 21:
		VSTS(21, *tmpa);
		break;
	case 22:
		VSTS(22, *tmpa);
		break;
	case 23:
		VSTS(23, *tmpa);
		break;
	case 24:
		VSTS(24, *tmpa);
		break;
	case 25:
		VSTS(25, *tmpa);
		break;
	case 26:
		VSTS(26, *tmpa);
		break;
	case 27:
		VSTS(27, *tmpa);
		break;
	case 28:
		VSTS(28, *tmpa);
		break;
	case 29:
		VSTS(29, *tmpa);
		break;
	case 30:
		VSTS(30, *tmpa);
		break;
	case 31:
		VSTS(31, *tmpa);
		break;
	}

	*fp_value = tmpa[0];
	*(fp_value+1) = tmpa[1];
}

void sw64_read_simd_fp_m_d(unsigned long reg, unsigned long *fp_value)
{
	volatile unsigned long tmpa[4] __aligned(32);

	switch (reg) {
	case  0:
		VSTD(0, *tmpa);
		break;
	case  1:
		VSTD(1, *tmpa);
		break;
	case  2:
		VSTD(2, *tmpa);
		break;
	case  3:
		VSTD(3, *tmpa);
		break;
	case  4:
		VSTD(4, *tmpa);
		break;
	case  5:
		VSTD(5, *tmpa);
		break;
	case  6:
		VSTD(6, *tmpa);
		break;
	case  7:
		VSTD(7, *tmpa);
		break;
	case  8:
		VSTD(8, *tmpa);
		break;
	case  9:
		VSTD(9, *tmpa);
		break;
	case 10:
		VSTD(10, *tmpa);
		break;
	case 11:
		VSTD(11, *tmpa);
		break;
	case 12:
		VSTD(12, *tmpa);
		break;
	case 13:
		VSTD(13, *tmpa);
		break;
	case 14:
		VSTD(14, *tmpa);
		break;
	case 15:
		VSTD(15, *tmpa);
		break;
	case 16:
		VSTD(16, *tmpa);
		break;
	case 17:
		VSTD(17, *tmpa);
		break;
	case 18:
		VSTD(18, *tmpa);
		break;
	case 19:
		VSTD(19, *tmpa);
		break;
	case 20:
		VSTD(20, *tmpa);
		break;
	case 21:
		VSTD(21, *tmpa);
		break;
	case 22:
		VSTD(22, *tmpa);
		break;
	case 23:
		VSTD(23, *tmpa);
		break;
	case 24:
		VSTD(24, *tmpa);
		break;
	case 25:
		VSTD(25, *tmpa);
		break;
	case 26:
		VSTD(26, *tmpa);
		break;
	case 27:
		VSTD(27, *tmpa);
		break;
	case 28:
		VSTD(28, *tmpa);
		break;
	case 29:
		VSTD(29, *tmpa);
		break;
	case 30:
		VSTD(30, *tmpa);
		break;
	case 31:
		VSTD(31, *tmpa);
		break;
	}

	*fp_value = tmpa[0];
	*(fp_value+1) = tmpa[1];
	*(fp_value+2) = tmpa[2];
	*(fp_value+3) = tmpa[3];
}

unsigned long sw64_read_fp_reg(unsigned long reg)
{
	unsigned long val;

	switch (reg) {
	case  0:
		STT(0, val);
		break;
	case  1:
		STT(1, val);
		break;
	case  2:
		STT(2, val);
		break;
	case  3:
		STT(3, val);
		break;
	case  4:
		STT(4, val);
		break;
	case  5:
		STT(5, val);
		break;
	case  6:
		STT(6, val);
		break;
	case  7:
		STT(7, val);
		break;
	case  8:
		STT(8, val);
		break;
	case  9:
		STT(9, val);
		break;
	case 10:
		STT(10, val);
		break;
	case 11:
		STT(11, val);
		break;
	case 12:
		STT(12, val);
		break;
	case 13:
		STT(13, val);
		break;
	case 14:
		STT(14, val);
		break;
	case 15:
		STT(15, val);
		break;
	case 16:
		STT(16, val);
		break;
	case 17:
		STT(17, val);
		break;
	case 18:
		STT(18, val);
		break;
	case 19:
		STT(19, val);
		break;
	case 20:
		STT(20, val);
		break;
	case 21:
		STT(21, val);
		break;
	case 22:
		STT(22, val);
		break;
	case 23:
		STT(23, val);
		break;
	case 24:
		STT(24, val);
		break;
	case 25:
		STT(25, val);
		break;
	case 26:
		STT(26, val);
		break;
	case 27:
		STT(27, val);
		break;
	case 28:
		STT(28, val);
		break;
	case 29:
		STT(29, val);
		break;
	case 30:
		STT(30, val);
		break;
	case 31:
		STT(31, val);
		break;
	default:
		return 0;
	}

	return val;
}
EXPORT_SYMBOL(sw64_read_fp_reg);

void sw64_write_fp_reg(unsigned long reg, unsigned long val)
{
	switch (reg) {
	case  0:
		LDT(0, val);
		break;
	case  1:
		LDT(1, val);
		break;
	case  2:
		LDT(2, val);
		break;
	case  3:
		LDT(3, val);
		break;
	case  4:
		LDT(4, val);
		break;
	case  5:
		LDT(5, val);
		break;
	case  6:
		LDT(6, val);
		break;
	case  7:
		LDT(7, val);
		break;
	case  8:
		LDT(8, val);
		break;
	case  9:
		LDT(9, val);
		break;
	case 10:
		LDT(10, val);
		break;
	case 11:
		LDT(11, val);
		break;
	case 12:
		LDT(12, val);
		break;
	case 13:
		LDT(13, val);
		break;
	case 14:
		LDT(14, val);
		break;
	case 15:
		LDT(15, val);
		break;
	case 16:
		LDT(16, val);
		break;
	case 17:
		LDT(17, val);
		break;
	case 18:
		LDT(18, val);
		break;
	case 19:
		LDT(19, val);
		break;
	case 20:
		LDT(20, val);
		break;
	case 21:
		LDT(21, val);
		break;
	case 22:
		LDT(22, val);
		break;
	case 23:
		LDT(23, val);
		break;
	case 24:
		LDT(24, val);
		break;
	case 25:
		LDT(25, val);
		break;
	case 26:
		LDT(26, val);
		break;
	case 27:
		LDT(27, val);
		break;
	case 28:
		LDT(28, val);
		break;
	case 29:
		LDT(29, val);
		break;
	case 30:
		LDT(30, val);
		break;
	case 31:
		LDT(31, val);
		break;
	}
}
EXPORT_SYMBOL(sw64_write_fp_reg);

unsigned long sw64_read_fp_reg_s(unsigned long reg)
{
	unsigned long val;

	switch (reg) {
	case  0:
		STS(0, val);
		break;
	case  1:
		STS(1, val);
		break;
	case  2:
		STS(2, val);
		break;
	case  3:
		STS(3, val);
		break;
	case  4:
		STS(4, val);
		break;
	case  5:
		STS(5, val);
		break;
	case  6:
		STS(6, val);
		break;
	case  7:
		STS(7, val);
		break;
	case  8:
		STS(8, val);
		break;
	case  9:
		STS(9, val);
		break;
	case 10:
		STS(10, val);
		break;
	case 11:
		STS(11, val);
		break;
	case 12:
		STS(12, val);
		break;
	case 13:
		STS(13, val);
		break;
	case 14:
		STS(14, val);
		break;
	case 15:
		STS(15, val);
		break;
	case 16:
		STS(16, val);
		break;
	case 17:
		STS(17, val);
		break;
	case 18:
		STS(18, val);
		break;
	case 19:
		STS(19, val);
		break;
	case 20:
		STS(20, val);
		break;
	case 21:
		STS(21, val);
		break;
	case 22:
		STS(22, val);
		break;
	case 23:
		STS(23, val);
		break;
	case 24:
		STS(24, val);
		break;
	case 25:
		STS(25, val);
		break;
	case 26:
		STS(26, val);
		break;
	case 27:
		STS(27, val);
		break;
	case 28:
		STS(28, val);
		break;
	case 29:
		STS(29, val);
		break;
	case 30:
		STS(30, val);
		break;
	case 31:
		STS(31, val);
		break;
	default:
		return 0;
	}

	return val;
}
EXPORT_SYMBOL(sw64_read_fp_reg_s);

void sw64_write_fp_reg_s(unsigned long reg, unsigned long val)
{
	switch (reg) {
	case  0:
		LDS(0, val);
		break;
	case  1:
		LDS(1, val);
		break;
	case  2:
		LDS(2, val);
		break;
	case  3:
		LDS(3, val);
		break;
	case  4:
		LDS(4, val);
		break;
	case  5:
		LDS(5, val);
		break;
	case  6:
		LDS(6, val);
		break;
	case  7:
		LDS(7, val);
		break;
	case  8:
		LDS(8, val);
		break;
	case  9:
		LDS(9, val);
		break;
	case 10:
		LDS(10, val);
		break;
	case 11:
		LDS(11, val);
		break;
	case 12:
		LDS(12, val);
		break;
	case 13:
		LDS(13, val);
		break;
	case 14:
		LDS(14, val);
		break;
	case 15:
		LDS(15, val);
		break;
	case 16:
		LDS(16, val);
		break;
	case 17:
		LDS(17, val);
		break;
	case 18:
		LDS(18, val);
		break;
	case 19:
		LDS(19, val);
		break;
	case 20:
		LDS(20, val);
		break;
	case 21:
		LDS(21, val);
		break;
	case 22:
		LDS(22, val);
		break;
	case 23:
		LDS(23, val);
		break;
	case 24:
		LDS(24, val);
		break;
	case 25:
		LDS(25, val);
		break;
	case 26:
		LDS(26, val);
		break;
	case 27:
		LDS(27, val);
		break;
	case 28:
		LDS(28, val);
		break;
	case 29:
		LDS(29, val);
		break;
	case 30:
		LDS(30, val);
		break;
	case 31:
		LDS(31, val);
		break;
	}
}
EXPORT_SYMBOL(sw64_write_fp_reg_s);
