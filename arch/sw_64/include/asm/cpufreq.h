/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ASM_SW64_CPUFREQ_H
#define _ASM_SW64_CPUFREQ_H

#include <linux/kref.h>
#include <linux/list.h>
#include <linux/seq_file.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <asm/cpu.h>

struct clk;

struct clk_ops {
	void (*init)(struct clk *clk);
	void (*enable)(struct clk *clk);
	void (*disable)(struct clk *clk);
	void (*recalc)(struct clk *clk);
	int (*set_rate)(struct clk *clk, unsigned long rate, int algo_id);
	long (*round_rate)(struct clk *clk, unsigned long rate);
};

struct clk {
	struct list_head node;
	const char *name;
	int id;
	struct module *owner;

	struct clk *parent;
	const struct clk_ops *ops;

	struct kref kref;

	unsigned long rate;
	unsigned long flags;
};

#define CLK_ALWAYS_ENABLED	(1 << 0)
#define CLK_RATE_PROPAGATES	(1 << 1)

#define CORE_CLK0_VALID		(0x1UL << 1)
#define CORE_CLK0_RESET		(0x1UL << 2)

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
#define STARTUP_RATE		(2000UL * 1000 * 1000)
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
#define STARTUP_RATE		(2400UL * 1000 * 1000)
#endif

#define OFFSET_CLU_LV1_SEL	0x3a80UL
#define OFFSET_CLK_CTL		0x3b80UL

extern struct cpufreq_frequency_table freq_table[];

int clk_init(void);
int sw64_set_rate(unsigned int index);

struct clk *sw64_clk_get(struct device *dev, const char *id);

void sw64_update_clockevents(unsigned long cpu, u32 freq);

unsigned int __sw64_cpufreq_get(struct cpufreq_policy *policy);
#endif /* _ASM_SW64_CPUFREQ_H */
