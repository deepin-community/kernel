/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ASM_SW64_CPUFREQ_H
#define _ASM_SW64_CPUFREQ_H

#include <linux/kref.h>
#include <linux/list.h>
#include <linux/seq_file.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <asm/cpu.h>

#define CLK_LV1_SEL_PROTECT	(0x1UL << 0)
#define CLK_LV1_SEL_MUXA	(0x1UL << 2)
#define CLK_LV1_SEL_MUXB	(0x1UL << 3)

#ifdef CONFIG_UNCORE_JUNZHANG
#define CLK0_PROTECT		(0x1UL << 0)
#define CLK2_PROTECT		(0x1UL << 32)
#define CORE_CLK2_VALID		(0x1UL << 33)
#define CORE_CLK2_RESET		(0x1UL << 34)
#define CORE_CLK2_LOCK		(0x1UL << 35)
#define CORE_PLL0_CFG_SHIFT     4
#define CORE_PLL1_CFG_SHIFT     20
#define CORE_PLL2_CFG_SHIFT     36
#define CORE_PLL2_CFG_MASK	0x1f
#endif

#ifdef CONFIG_UNCORE_XUELANG
#define CLK_PROTECT		(0x1UL << 0)
#define CLK0_PROTECT		CLK_PROTECT
#define CLK2_PROTECT		CLK_PROTECT
#define CORE_CLK2_VALID         (0x1UL << 15)
#define CORE_CLK2_RESET         (0x1UL << 16)
#define CORE_CLK2_LOCK		(0x1UL << 17)
#define CORE_PLL0_CFG_SHIFT     4
#define CORE_PLL1_CFG_SHIFT     11
#define CORE_PLL2_CFG_SHIFT     18
#define CORE_PLL2_CFG_MASK	0xf
#endif

#define OFFSET_CLU_LV1_SEL	0x3a80UL
#define OFFSET_CLK_CTL		0x3b80UL

extern struct cpufreq_frequency_table freq_table[];

int sw64_set_rate(unsigned int index);

unsigned int __sw64_cpufreq_get(struct cpufreq_policy *policy);
#endif /* _ASM_SW64_CPUFREQ_H */
