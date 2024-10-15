/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Definitions for use with the sw64 wrperfmon HMCODE call.
 */

#ifndef _ASM_SW64_WRPMC_H
#define _ASM_SW64_WRPMC_H

/* Following commands are implemented on all CPUs */
/* core4 */
#define PMC_CMD_READ_PC0		5
#define PMC_CMD_READ_PC1		6
#define PMC_CMD_READ_PC2		7
#define PMC_CMD_READ_PC3		8
#define PMC_CMD_READ_PC4		9
#define PMC_CMD_ENABLE			11
#define PMC_CMD_DISABLE			12
#define PMC_CMD_WRITE_BASE		16

#define PC_RAW_BASE			0x0
#define PC_MAX				0x8D

#define SW64_PERFCTRL_AM		0x0
#define SW64_PERFCTRL_VM		0x3
#define SW64_PERFCTRL_KM		0x5
#define SW64_PERFCTRL_UM		0x7

/* pc0-4 events */
#define SW64_PMU_INSTRUCTIONS		0x3
#define SW64_PMU_BRANCH			0x4
#define SW64_PMU_BRANCH_MISSES		0x5
#define SW64_L1I_CACHE			0x6
#define SW64_L1I_CACHE_MISSES		0x7
#define SW64_PMU_CYCLE			0x30
#define SW64_DTB			0x31
#define SW64_DTB_MISSES			0x32
#define SW64_L1D_CACHE			0x3D
#define SW64_L1D_CACHE_MISSES		0x3E
#define SW64_PMU_L2_REFERENCES		0x50
#define SW64_PMU_L2_MISSES		0x53

#define PC_ALL_PM_SET			3
#define MAX_HWEVENTS			5
#define PMC_COUNT_MASK			(-1UL)

#define IACC_EN				0x4
#define IMISC_EN			0x8
#define RETIC_EN			0x10
#define BRRETC_EN			0x20
#define BRFAILC_EN			0x40

#endif /* _ASM_SW64_WRPMC_H */
