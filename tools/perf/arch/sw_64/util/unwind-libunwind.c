// SPDX-License-Identifier: GPL-2.0
#include <errno.h>

#ifndef REMOTE_UNWIND_LIBUNWIND
#include <libunwind.h>
#include "perf_regs.h"
#include "../../util/unwind.h"
#include "../../util/debug.h"
#endif

int LIBUNWIND__ARCH_REG_ID(int regnum)
{
	switch (regnum) {
	case UNW_SW_64_R0:
		return PERF_REG_SW64_R0;
	case UNW_SW_64_R1:
		return PERF_REG_SW64_R1;
	case UNW_SW_64_R2:
		return PERF_REG_SW64_R2;
	case UNW_SW_64_R3:
		return PERF_REG_SW64_R3;
	case UNW_SW_64_R4:
		return PERF_REG_SW64_R4;
	case UNW_SW_64_R5:
		return PERF_REG_SW64_R5;
	case UNW_SW_64_R6:
		return PERF_REG_SW64_R6;
	case UNW_SW_64_R7:
		return PERF_REG_SW64_R7;
	case UNW_SW_64_R8:
		return PERF_REG_SW64_R8;
	case UNW_SW_64_R9:
		return PERF_REG_SW64_R9;
	case UNW_SW_64_R10:
		return PERF_REG_SW64_R10;
	case UNW_SW_64_R11:
		return PERF_REG_SW64_R11;
	case UNW_SW_64_R12:
		return PERF_REG_SW64_R12;
	case UNW_SW_64_R13:
		return PERF_REG_SW64_R13;
	case UNW_SW_64_R14:
		return PERF_REG_SW64_R14;
	case UNW_SW_64_R15:
		return PERF_REG_SW64_R15;
	case UNW_SW_64_R16:
		return PERF_REG_SW64_R16;
	case UNW_SW_64_R17:
		return PERF_REG_SW64_R17;
	case UNW_SW_64_R18:
		return PERF_REG_SW64_R18;
	case UNW_SW_64_R19:
		return PERF_REG_SW64_R19;
	case UNW_SW_64_R20:
		return PERF_REG_SW64_R20;
	case UNW_SW_64_R21:
		return PERF_REG_SW64_R21;
	case UNW_SW_64_R22:
		return PERF_REG_SW64_R22;
	case UNW_SW_64_R23:
		return PERF_REG_SW64_R23;
	case UNW_SW_64_R24:
		return PERF_REG_SW64_R24;
	case UNW_SW_64_R25:
		return PERF_REG_SW64_R25;
	case UNW_SW_64_R26:
		return PERF_REG_SW64_R26;
	case UNW_SW_64_R27:
		return PERF_REG_SW64_R27;
	case UNW_SW_64_R28:
		return PERF_REG_SW64_R28;
	case UNW_SW_64_R29:
		return PERF_REG_SW64_GP;
	case UNW_SW_64_R30:
		return PERF_REG_SW64_SP;
	case UNW_SW_64_PC:
		return PERF_REG_SW64_PC;
	default:
		pr_err("unwind: invalid reg id %d\n", regnum);
		return -EINVAL;
	}

	return -EINVAL;
}
