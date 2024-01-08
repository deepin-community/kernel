/* SPDX-License-Identifier: GPL-2.0 */
#ifndef ARCH_PERF_REGS_H
#define ARCH_PERF_REGS_H

#include <stdlib.h>
#include <linux/types.h>
#include <asm/perf_regs.h>

void perf_regs_load(u64 *regs);

#define PERF_REGS_MASK	((1ULL << PERF_REG_SW64_MAX) - 1)
#define PERF_REGS_MAX	PERF_REG_SW64_MAX
#define PERF_SAMPLE_REGS_ABI	PERF_SAMPLE_REGS_ABI_64

#define PERF_REG_IP	PERF_REG_SW64_PC
#define PERF_REG_SP	PERF_REG_SW64_SP

static inline const char *perf_reg_name(int id)
{
	switch (id) {
	case PERF_REG_SW64_R0:
		return "r0";
	case PERF_REG_SW64_R1:
		return "r1";
	case PERF_REG_SW64_R2:
		return "r2";
	case PERF_REG_SW64_R3:
		return "r3";
	case PERF_REG_SW64_R4:
		return "r4";
	case PERF_REG_SW64_R5:
		return "r5";
	case PERF_REG_SW64_R6:
		return "r6";
	case PERF_REG_SW64_R7:
		return "r7";
	case PERF_REG_SW64_R8:
		return "r8";
	case PERF_REG_SW64_R9:
		return "r9";
	case PERF_REG_SW64_R10:
		return "r10";
	case PERF_REG_SW64_R11:
		return "r11";
	case PERF_REG_SW64_R12:
		return "r12";
	case PERF_REG_SW64_R13:
		return "r13";
	case PERF_REG_SW64_R14:
		return "r14";
	case PERF_REG_SW64_R15:
		return "r15";
	case PERF_REG_SW64_R16:
		return "r16";
	case PERF_REG_SW64_R17:
		return "r17";
	case PERF_REG_SW64_R18:
		return "r18";
	case PERF_REG_SW64_R19:
		return "r19";
	case PERF_REG_SW64_R20:
		return "r20";
	case PERF_REG_SW64_R21:
		return "r21";
	case PERF_REG_SW64_R22:
		return "r22";
	case PERF_REG_SW64_R23:
		return "r23";
	case PERF_REG_SW64_R24:
		return "r24";
	case PERF_REG_SW64_R25:
		return "r25";
	case PERF_REG_SW64_R26:
		return "r26";
	case PERF_REG_SW64_R27:
		return "r27";
	case PERF_REG_SW64_R28:
		return "r28";
	case PERF_REG_SW64_GP:
		return "gp";
	case PERF_REG_SW64_SP:
		return "sp";
	case PERF_REG_SW64_PC:
		return "pc";
	default:
		return NULL;
	}

	return NULL;
}

#endif /* ARCH_PERF_REGS_H */
