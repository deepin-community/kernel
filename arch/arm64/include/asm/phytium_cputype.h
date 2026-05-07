/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2025 ARM Ltd.
 */
#ifndef __ASM_PHYTIUM_CPUTYPE_H
#define __ASM_PHYTIUM_CPUTYPE_H

#include <linux/arm-smccc.h>
#include <asm/cputype.h>

#define SOC_ID_PE1702	0x1
#define SOC_ID_PS17064	0x2
#define SOC_ID_PD1904	0x3
#define SOC_ID_PS20064	0x4

#define SOC_ID_PD2008	0x5
#define SOC_ID_PS21064	0x6
#define SOC_ID_PE220X	0x7
#define SOC_ID_PS23064	0x8
#define SOC_ID_PD2308	0x9
#define SOC_ID_PS2480	0xa
#define SOC_ID_PD2408	0xb

#define SYS_REG_VAL_PS24080	0x6
#define SYS_REG_VAL_PS23064	0x8

#define SMCCC_SUCCESS		0
#define SMCCC_FAILURE		(-1)
#define FUNC_ID_GET_CPU_VERSION 0xc2000002

enum phyt_cpu_type {
	PE1702 = 1,
	PS17064,
	PD1904,
	PS20064,
	PD2008,
	PS21064,
	PE220X,
	PS23064,
	PD2308,
	PS24080,
	PD2408,
	PS15016,
	UNKNOWN_SOC
};

static enum phyt_cpu_type do_smccc_res(struct arm_smccc_res res)
{
	unsigned long smc_cpu_ver = res.a1;

	smc_cpu_ver = (smc_cpu_ver >> 8);
	switch (smc_cpu_ver) {
	case SOC_ID_PE1702:
		return PE1702;
	case SOC_ID_PS17064:
		return PS17064;
	case SOC_ID_PD1904:
		return PD1904;
	case SOC_ID_PS20064:
		return PS20064;
	case SOC_ID_PD2008:
		return PD2008;
	case SOC_ID_PS21064:
		return PS21064;
	case SOC_ID_PE220X:
		return PE220X;
	case SOC_ID_PS23064:
		return PS23064;
	case SOC_ID_PD2308:
		return PD2308;
	case SOC_ID_PS2480:
		return PS24080;
	case SOC_ID_PD2408:
		return PD2408;
	default:
		return UNKNOWN_SOC;
	}
}

static enum phyt_cpu_type do_read_sysreg(void)
{
	u32 aidr_reg_val = read_sysreg(aidr_el1);

	switch (aidr_reg_val) {
	case SYS_REG_VAL_PS24080:
		return PS24080;
	case SYS_REG_VAL_PS23064:
		return PS23064;
	default:
		return UNKNOWN_SOC;
	}
}

static enum phyt_cpu_type do_read_mpidr(void)
{
	u32 part_id = read_cpuid_part_number();

	switch (part_id) {
	case PHYTIUM_CPU_PART_FTC303:
		return PE220X;
	case PHYTIUM_CPU_PART_FTC660:
		return PS15016;
	case PHYTIUM_CPU_PART_FTC661:
		return PE1702;
	case PHYTIUM_CPU_PART_FTC662:
		return PS17064;
	case PHYTIUM_CPU_PART_FTC663:
		return PD1904;
	case PHYTIUM_CPU_PART_FTC862:
		return PD2408;
	default:
		return UNKNOWN_SOC;
	}
}

static inline bool is_phytium_soc(void)
{
	if (read_cpuid_implementor() == ARM_CPU_IMP_PHYTIUM)
		return true;
	return false;
}

static inline enum phyt_cpu_type phyt_read_cpu_type(void)
{
	enum phyt_cpu_type ctype;
	struct arm_smccc_res res;

	if (!is_phytium_soc())
		return UNKNOWN_SOC;

	arm_smccc_smc(FUNC_ID_GET_CPU_VERSION, 0, 0, 0, 0, 0, 0, 0, &res);
	switch (res.a0) {
	case SMCCC_SUCCESS:
		ctype = do_smccc_res(res);
		if (ctype != UNKNOWN_SOC)
			break;
		fallthrough;
	case SMCCC_FAILURE:
		ctype = do_read_sysreg();
		if (ctype != UNKNOWN_SOC)
			break;
		fallthrough;
	default:
		ctype = do_read_mpidr();
		break;
	}
	return ctype;
}

static inline bool is_pd2408(void)
{
	enum phyt_cpu_type ctype = phyt_read_cpu_type();

	if (ctype == PD2408)
		return true;
	return false;
}

static inline bool is_ps23064(void)
{
	enum phyt_cpu_type ctype = phyt_read_cpu_type();

	if (ctype == PS23064)
		return true;
	return false;
}

static inline bool is_ps24080(void)
{
	enum phyt_cpu_type ctype = phyt_read_cpu_type();

	if (ctype == PS24080)
		return true;
	return false;
}

static inline bool is_pd2308(void)
{
	enum phyt_cpu_type ctype = phyt_read_cpu_type();

	if (ctype == PD2308)
		return true;
	return false;
}

static inline bool is_pe220x(void)
{
	enum phyt_cpu_type ctype = phyt_read_cpu_type();

	if (ctype == PE220X)
		return true;

	return false;
}

#endif


