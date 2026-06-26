// SPDX-License-Identifier: GPL-2.0
/*
 * (C) Copyright 1998 Linus Torvalds
 */

#include <linux/gfp.h>
#include <linux/export.h>
#include <linux/errno.h>

#define __ASM_EX_TABLE(insn, fixup) \
	"1: " insn "\n"             \
	"2:\n" \
	".section __ex_table, \"a\"\n\t" \
	"	.long 1b - .\n"      \
	"       " fixup "\n"         \
	".previous\n"

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
	asm volatile ("vsts $f"#reg", %0" : "=m"(val) : : "memory")
#define VLDWU(reg, val)		\
	asm volatile ( \
		__ASM_EX_TABLE("vldw_u $f"#reg", %1", "ldi $31, 2b-1b(%0)") \
		: "+r"(err) : "m"(val) : "memory")
#define VSTWU(reg, val)		\
	asm volatile ( \
		__ASM_EX_TABLE("vstw_u $f"#reg", %1", "ldi $31, 2b-1b(%0)") \
		: "+r"(err), "=m"(val) : : "memory")
#define VLDSU(reg, val)		\
	asm volatile ( \
		__ASM_EX_TABLE("vlds_u $f"#reg", %1", "ldi $31, 2b-1b(%0)") \
		: "+r"(err) : "m"(val) : "memory")
#define VSTSU(reg, val)		\
	asm volatile ( \
		__ASM_EX_TABLE("vsts_u $f"#reg", %1", "ldi $31, 2b-1b(%0)") \
		: "+r"(err), "=m"(val) : : "memory")
#define VLDDU(reg, val)		\
	asm volatile ( \
		__ASM_EX_TABLE("vldd_u $f"#reg", %1", "ldi $31, 2b-1b(%0)") \
		: "+r"(err) : "m"(val) : "memory")
#define VSTDU(reg, val)		\
	asm volatile ( \
		__ASM_EX_TABLE("vstd_u $f"#reg", %1", "ldi $31, 2b-1b(%0)") \
		: "+r"(err), "=m"(val) : : "memory")
#define VSTWUL(reg, val)		\
	asm volatile ( \
		__ASM_EX_TABLE("vstw_ul $f"#reg", %1", "ldi $31, 2b-1b(%0)") \
		: "+r"(err), "=m"(val) : : "memory")
#define VSTWUH(reg, val)		\
	asm volatile ( \
		__ASM_EX_TABLE("vstw_uh $f"#reg", %1", "ldi $31, 2b-1b(%0)") \
		: "+r"(err), "=m"(val) : : "memory")
#define VSTSUL(reg, val)		\
	asm volatile ( \
		__ASM_EX_TABLE("vsts_ul $f"#reg", %1", "ldi $31, 2b-1b(%0)") \
		: "+r"(err), "=m"(val) : : "memory")
#define VSTSUH(reg, val)		\
	asm volatile ( \
		__ASM_EX_TABLE("vsts_uh $f"#reg", %1", "ldi $31, 2b-1b(%0)") \
		: "+r"(err), "=m"(val) : : "memory")
#define VSTDUL(reg, val)		\
	asm volatile ( \
		__ASM_EX_TABLE("vstd_ul $f"#reg", %1", "ldi $31, 2b-1b(%0)") \
		: "+r"(err), "=m"(val) : : "memory")
#define VSTDUH(reg, val)		\
	asm volatile ( \
		__ASM_EX_TABLE("vstd_uh $f"#reg", %1", "ldi $31, 2b-1b(%0)") \
		: "+r"(err), "=m"(val) : : "memory")

#define _CASE_REG(n, op, arg) \
	case n:               \
		op(n, arg);   \
		break;

#define _CASE_0_TO_30(op, arg) \
	_CASE_REG(0, op, arg) _CASE_REG(1, op, arg) \
	_CASE_REG(2, op, arg) _CASE_REG(3, op, arg) \
	_CASE_REG(4, op, arg) _CASE_REG(5, op, arg) \
	_CASE_REG(6, op, arg) _CASE_REG(7, op, arg) \
	_CASE_REG(8, op, arg) _CASE_REG(9, op, arg) \
	_CASE_REG(10, op, arg) _CASE_REG(11, op, arg) \
	_CASE_REG(12, op, arg) _CASE_REG(13, op, arg) \
	_CASE_REG(14, op, arg) _CASE_REG(15, op, arg) \
	_CASE_REG(16, op, arg) _CASE_REG(17, op, arg) \
	_CASE_REG(18, op, arg) _CASE_REG(19, op, arg) \
	_CASE_REG(20, op, arg) _CASE_REG(21, op, arg) \
	_CASE_REG(22, op, arg) _CASE_REG(23, op, arg) \
	_CASE_REG(24, op, arg) _CASE_REG(25, op, arg) \
	_CASE_REG(26, op, arg) _CASE_REG(27, op, arg) \
	_CASE_REG(28, op, arg) _CASE_REG(29, op, arg) \
	_CASE_REG(30, op, arg)

#define SWITCH_REG_0_30(reg, op, arg) \
	switch (reg) {         \
	_CASE_0_TO_30(op, arg) \
	case 31:              \
		break;        \
	}

#define SWITCH_REG_0_31(reg, op, arg) \
	switch (reg) {                 \
	_CASE_0_TO_30(op, arg) \
	_CASE_REG(31, op, arg) \
	}

void
sw64_write_simd_fp_reg_s(unsigned long reg, unsigned long f0, unsigned long f1)
{

	unsigned long tmpa[4] __aligned(16);

	tmpa[0] = f0;
	tmpa[1] = f1;

	SWITCH_REG_0_30(reg, VLDS, *tmpa);
}

void sw64_write_simd_fp_reg_d(unsigned long reg, unsigned long f0,
		unsigned long f1, unsigned long f2, unsigned long f3)
{
	unsigned long tmpa[4] __aligned(32);

	tmpa[0] = f0;
	tmpa[1] = f1;
	tmpa[2] = f2;
	tmpa[3] = f3;

	SWITCH_REG_0_30(reg, VLDD, *tmpa);
}

void sw64_write_simd_fp_reg_ldwe(unsigned long reg, int a)
{
	SWITCH_REG_0_30(reg, LDWE, a);
}

void sw64_read_simd_fp_m_s(unsigned long reg, unsigned long *fp_value)
{
	volatile unsigned long tmpa[2] __attribute__((aligned(16)));

	SWITCH_REG_0_31(reg, VSTS, *tmpa);

	*fp_value = tmpa[0];
	*(fp_value+1) = tmpa[1];
}

void sw64_read_simd_fp_m_d(unsigned long reg, unsigned long *fp_value)
{
	volatile unsigned long tmpa[4] __attribute__((aligned(32)));

	SWITCH_REG_0_31(reg, VSTD, *tmpa);

	*fp_value = tmpa[0];
	*(fp_value+1) = tmpa[1];
	*(fp_value+2) = tmpa[2];
	*(fp_value+3) = tmpa[3];
}

unsigned long sw64_read_fp_reg(unsigned long reg)
{
	unsigned long val;

	SWITCH_REG_0_31(reg, STT, val);

	return val;
}
EXPORT_SYMBOL(sw64_read_fp_reg);

void sw64_write_fp_reg(unsigned long reg, unsigned long val)
{
	SWITCH_REG_0_31(reg, LDT, val);
}
EXPORT_SYMBOL(sw64_write_fp_reg);

unsigned long sw64_read_fp_reg_s(unsigned long reg)
{
	unsigned long val;

	SWITCH_REG_0_31(reg, STS, val);

	return val;
}
EXPORT_SYMBOL(sw64_read_fp_reg_s);

void sw64_write_fp_reg_s(unsigned long reg, unsigned long val)
{
	SWITCH_REG_0_31(reg, LDS, val);
}
EXPORT_SYMBOL(sw64_write_fp_reg_s);

int sw64_write_simd_fp_reg_vldwu(unsigned long reg, unsigned long aligned_va)
{
	volatile unsigned long *tmpa = (unsigned long *)aligned_va;
	int err = 0;

	SWITCH_REG_0_31(reg, VLDWU, *tmpa);

	return err;
}
EXPORT_SYMBOL(sw64_write_simd_fp_reg_vldwu);

int sw64_store_simd_fp_reg_vstwu(unsigned long reg, unsigned long aligned_va)
{
	volatile unsigned long *tmpa = (unsigned long *)aligned_va;
	int err = 0;

	SWITCH_REG_0_31(reg, VSTWU, *tmpa);

	return err;
}
EXPORT_SYMBOL(sw64_store_simd_fp_reg_vstwu);

int sw64_write_simd_fp_reg_vldsu(unsigned long reg, unsigned long aligned_va)
{
	volatile unsigned long *tmpa = (unsigned long *)aligned_va;
	int err = 0;

	SWITCH_REG_0_31(reg, VLDSU, *tmpa);

	return err;
}
EXPORT_SYMBOL(sw64_write_simd_fp_reg_vldsu);

int sw64_store_simd_fp_reg_vstsu(unsigned long reg, unsigned long aligned_va)
{
	volatile unsigned long *tmpa = (unsigned long *)aligned_va;
	int err = 0;

	SWITCH_REG_0_31(reg, VSTSU, *tmpa);

	return err;
}
EXPORT_SYMBOL(sw64_store_simd_fp_reg_vstsu);

int sw64_write_simd_fp_reg_vlddu(unsigned long reg, unsigned long aligned_va)
{
	volatile unsigned long *tmpa = (unsigned long *)aligned_va;
	int err = 0;

	SWITCH_REG_0_31(reg, VLDDU, *tmpa);

	return err;
}
EXPORT_SYMBOL(sw64_write_simd_fp_reg_vlddu);

int sw64_store_simd_fp_reg_vstdu(unsigned long reg, unsigned long aligned_va)
{
	volatile unsigned long *tmpa = (unsigned long *)aligned_va;
	int err = 0;

	SWITCH_REG_0_31(reg, VSTDU, *tmpa);

	return err;
}
EXPORT_SYMBOL(sw64_store_simd_fp_reg_vstdu);

int sw64_store_simd_fp_reg_vstwul(unsigned long reg, unsigned long aligned_va)
{
	volatile unsigned long *tmpa = (unsigned long *)aligned_va;
	int err = 0;

	SWITCH_REG_0_31(reg, VSTWUL, *tmpa);

	return err;
}
EXPORT_SYMBOL(sw64_store_simd_fp_reg_vstwul);

int sw64_store_simd_fp_reg_vstwuh(unsigned long reg, unsigned long aligned_va)
{
	volatile unsigned long *tmpa = (unsigned long *)aligned_va;
	int err = 0;

	SWITCH_REG_0_31(reg, VSTWUH, *tmpa);

	return err;
}
EXPORT_SYMBOL(sw64_store_simd_fp_reg_vstwuh);

int sw64_store_simd_fp_reg_vstsul(unsigned long reg, unsigned long aligned_va)
{
	volatile unsigned long *tmpa = (unsigned long *)aligned_va;
	int err = 0;

	SWITCH_REG_0_31(reg, VSTSUL, *tmpa);

	return err;
}
EXPORT_SYMBOL(sw64_store_simd_fp_reg_vstsul);

int sw64_store_simd_fp_reg_vstsuh(unsigned long reg, unsigned long aligned_va)
{
	volatile unsigned long *tmpa = (unsigned long *)aligned_va;
	int err = 0;

	SWITCH_REG_0_31(reg, VSTSUH, *tmpa);

	return err;
}
EXPORT_SYMBOL(sw64_store_simd_fp_reg_vstsuh);

int sw64_store_simd_fp_reg_vstdul(unsigned long reg, unsigned long aligned_va)
{
	volatile unsigned long *tmpa = (unsigned long *)aligned_va;
	int err = 0;

	SWITCH_REG_0_31(reg, VSTDUL, *tmpa);

	return err;
}
EXPORT_SYMBOL(sw64_store_simd_fp_reg_vstdul);

int sw64_store_simd_fp_reg_vstduh(unsigned long reg, unsigned long aligned_va)
{
	volatile unsigned long *tmpa = (unsigned long *)aligned_va;
	int err = 0;

	SWITCH_REG_0_31(reg, VSTDUH, *tmpa);

	return err;
}
EXPORT_SYMBOL(sw64_store_simd_fp_reg_vstduh);
