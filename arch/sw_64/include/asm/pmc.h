/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Definitions for use with the sw64 PMC interface.
 */

#ifndef _ASM_SW64_PMC_H
#define _ASM_SW64_PMC_H

#define PMC_PC0			0
#define PMC_PC1			1

/* Following commands are implemented on all CPUs */
#define PMC_CMD_DISABLE		0
#define PMC_CMD_ENABLE		1
#define PMC_CMD_EVENT_BASE	2
#define PMC_CMD_PM		4
#define PMC_CMD_READ		5
#define PMC_CMD_READ_CLEAR	6
#define PMC_CMD_WRITE_BASE	7

#define PMC_DISABLE_BASE	1

#define PMC_ENABLE_BASE		1

#define PC0_RAW_BASE		0x0
#define PC1_RAW_BASE		0x100
#define PC0_MAX			0xF
#define PC1_MAX			0x3D

#define SW64_PERFCTRL_KM	2
#define SW64_PERFCTRL_UM	3
#define SW64_PERFCTRL_AM	4

/* pc0 events */
#define PC0_INSTRUCTIONS		0x0
#define PC0_BRANCH_INSTRUCTIONS		0x3
#define PC0_CPU_CYCLES			0x8
#define PC0_ITB_READ			0x9
#define PC0_DTB_READ			0xA
#define PC0_ICACHE_READ			0xB
#define PC0_DCACHE_READ			0xC
#define PC0_SCACHE_REFERENCES		0xD

/* pc1 events */
#define PC1_BRANCH_MISSES		0xB
#define PC1_SCACHE_MISSES		0x10
#define PC1_ICACHE_READ_MISSES		0x16
#define PC1_ITB_MISSES			0x17
#define PC1_DTB_SINGLE_MISSES		0x30
#define PC1_DCACHE_MISSES		0x32

#define MAX_HWEVENTS			2
#define PMC_COUNT_MASK			((1UL << 58) - 1)

#endif /* _ASM_SW64_PMC_H */
