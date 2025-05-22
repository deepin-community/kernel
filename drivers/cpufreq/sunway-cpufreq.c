// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2025 WXIAT
 */

#define pr_fmt(fmt) "cpufreq: " fmt

#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/delay.h>

#include <asm/sw64_init.h>
#include <asm/cpu.h>

#define MAX_RETRY	10

#define CLK_LV1_SEL_PROTECT	(0x1UL << 0)
#define CLK_LV1_SEL_MUXA	(0x1UL << 2)
#define CLK_LV1_SEL_MUXB	(0x1UL << 3)

#define OFFSET_CLU_LV1_SEL	0x3a80UL
#define OFFSET_CLK_CTL		0x3b80UL

/*
 * frequency in MHz, volts in mV and stored as "driver_data" in the structure.
 * volts 0 means to be determined
 */
#define FV(mhz, mv)							\
	{								\
		.frequency = (mhz) * 1000,				\
		.driver_data = (mv)					\
	}

#ifdef CONFIG_PLATFORM_JUNZHANG
#define CLK0_PROTECT		(0x1UL << 0)
#define CLK2_PROTECT		(0x1UL << 32)
#define CORE_CLK2_VALID		(0x1UL << 33)
#define CORE_CLK2_RESET		(0x1UL << 34)
#define CORE_CLK2_LOCK		(0x1UL << 35)
#define CORE_PLL0_CFG_SHIFT     4
#define CORE_PLL1_CFG_SHIFT     20
#define CORE_PLL2_CFG_SHIFT     36
#define CORE_PLL2_CFG_MASK	0x1f

struct cpufreq_frequency_table freq_table[] = {
	{0, 0, CPUFREQ_ENTRY_INVALID}, /* 200Mhz is ignored */
	FV(1200, 850),
	FV(1300, 850),
	FV(1400, 850),
	FV(1450, 850),
	FV(1500, 850),
	FV(1550, 850),
	FV(1600, 850),
	FV(1650, 900),
	FV(1700, 900),
	FV(1750, 900),
	FV(1800, 900),
	FV(1850, 900),
	FV(1900, 900),
	FV(1950, 900),
	FV(2000, 900),
	FV(2050, 950),
	FV(2100, 950),
	FV(2150, 950),
	FV(2200, 950),
	FV(2250, 0),
	FV(2300, 0),
	FV(2350, 0),
	FV(2400, 0),
	FV(2450, 0),
	FV(2500, 0),
	FV(2550, 0),
	FV(2600, 0),
	FV(2650, 0),
	FV(2700, 0),
	FV(2800, 0),
	FV(2850, 0),
	{0, 0, CPUFREQ_TABLE_END},
};
static void __init fill_freq_table(struct cpufreq_frequency_table *ft)
{
}
#endif

#ifdef CONFIG_PLATFORM_XUELANG
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

struct cpufreq_frequency_table freq_table[] = {
	{0, 0, CPUFREQ_ENTRY_INVALID}, /* 200Mhz is ignored */
	{0, 0, CPUFREQ_ENTRY_INVALID}, /* 1200Mhz is ignored */
	{0, 0, CPUFREQ_ENTRY_INVALID},
	{0, 0, CPUFREQ_ENTRY_INVALID},
	{0, 0, CPUFREQ_ENTRY_INVALID},
	{0, 0, CPUFREQ_ENTRY_INVALID},
	{0, 0, CPUFREQ_ENTRY_INVALID},
	{0, 0, CPUFREQ_ENTRY_INVALID},
	{0, 0, CPUFREQ_ENTRY_INVALID},
	{0, 0, CPUFREQ_ENTRY_INVALID},
	{0, 0, CPUFREQ_ENTRY_INVALID},
	{0, 0, CPUFREQ_ENTRY_INVALID},
	{0, 0, CPUFREQ_ENTRY_INVALID},
	{0, 0, CPUFREQ_ENTRY_INVALID},
	{0, 0, CPUFREQ_ENTRY_INVALID},
	{0, 0, CPUFREQ_ENTRY_INVALID},
	{0, 0, CPUFREQ_TABLE_END},
};

static void __init fill_freq_table(struct cpufreq_frequency_table *ft)
{
	int i;
	unsigned long freq_off;
	unsigned char external_clk;

	external_clk = *((unsigned char *)__va(MB_EXTCLK));

	if (external_clk == 240)
		freq_off = 60000;
	else
		freq_off = 50000;

	freq_table[2].frequency = freq_off * 36;
	for (i = 3; freq_table[i].frequency != CPUFREQ_TABLE_END; i++)
		freq_table[i].frequency = freq_off * 38 + ((i - 3) * freq_off);
}
#endif

static unsigned int sunway_get_rate(struct cpufreq_policy *policy)
{
	int i;
	u64 val;
	void __iomem *spbu_base = misc_platform_get_spbu_base(0);
	struct cpufreq_frequency_table *ft = policy->freq_table;

	/* PLL2 provides working frequency for core */
	val = readq(spbu_base + OFFSET_CLK_CTL) >> CORE_PLL2_CFG_SHIFT;
	val &= CORE_PLL2_CFG_MASK;

	for (i = 0; ft[i].frequency != CPUFREQ_TABLE_END; i++) {
		if (val == i) {
			if (ft[i].frequency == CPUFREQ_ENTRY_INVALID)
				return cpuid(GET_CPU_FREQ, 0) * 1000UL;
			return ft[i].frequency;
		}
	}

	return 0;
}

static int sunway_set_rate(unsigned int index)
{
	int i, retry, cpu_num;
	void __iomem *spbu_base;

	cpu_num = sw64_chip->get_cpu_num();
	for (i = 0; i < cpu_num; i++) {
		spbu_base = misc_platform_get_spbu_base(i);

		/* select PLL0/PLL1 */
		writeq(CLK_LV1_SEL_PROTECT, spbu_base + OFFSET_CLU_LV1_SEL);
		/* reset PLL2 */
		writeq(CLK2_PROTECT | CORE_CLK2_RESET | CORE_CLK2_VALID, spbu_base + OFFSET_CLK_CTL);
		/* configure PLL2_CFG */
		writeq(CLK2_PROTECT | CORE_CLK2_VALID | (unsigned long)index << CORE_PLL2_CFG_SHIFT,
				spbu_base + OFFSET_CLK_CTL);
		udelay(1);
		/* reset over */
		writeq(CORE_CLK2_VALID, spbu_base + OFFSET_CLK_CTL);
		retry = 0;
		while (retry < MAX_RETRY) {
			if (readq(spbu_base + OFFSET_CLK_CTL) & CORE_CLK2_LOCK)
				break;
			retry++;
			udelay(100);
		}
		if (retry == MAX_RETRY)
			return -ETIME;
		/* configure over */
		writeq(0, spbu_base + OFFSET_CLK_CTL);
		/* select PLL2/PLL2 */
		writeq(CLK_LV1_SEL_MUXA | CLK_LV1_SEL_MUXB | CLK_LV1_SEL_PROTECT,
				spbu_base + OFFSET_CLU_LV1_SEL);
	}
	return 0;
}

static unsigned int sunway_cpufreq_get(unsigned int cpu)
{
	struct cpufreq_policy *policy = cpufreq_cpu_get_raw(cpu);

	if (!policy) {
		pr_err("%s: no policy associated to cpu: %d\n",
				__func__, cpu);
		return 0;
	}

	return sunway_get_rate(policy);
}

/*
 * Here we notify other drivers of the proposed change and the final change.
 */
static int sunway_cpufreq_target(struct cpufreq_policy *policy,
				     unsigned int index)
{
	int ret;
	unsigned int cpu = policy->cpu;

	if (!cpu_online(cpu))
		return -ENODEV;

	/* setting the cpu frequency */
	ret = sunway_set_rate(index);
	if (ret)
		return ret;
	update_cpu_freq(freq_table[index].frequency);

	return 0;
}

static int sunway_cpufreq_init(struct cpufreq_policy *policy)
{
	cpufreq_generic_init(policy, freq_table, 0);

	return 0;
}

static int sunway_cpufreq_verify(struct cpufreq_policy_data *policy)
{
	return cpufreq_frequency_table_verify(policy, freq_table);
}

static int sunway_cpufreq_exit(struct cpufreq_policy *policy)
{
	return 0;
}

static struct freq_attr *sunway_table_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs, NULL,
};

static struct cpufreq_driver sunway_cpufreq_driver = {
	.name = "sunway-cpufreq",
	.flags = CPUFREQ_NEED_INITIAL_FREQ_CHECK,
	.init = sunway_cpufreq_init,
	.verify = sunway_cpufreq_verify,
	.target_index = sunway_cpufreq_target,
	.get = sunway_cpufreq_get,
	.exit = sunway_cpufreq_exit,
	.attr = sunway_table_attr,
};

static int __init cpufreq_init(void)
{
	int i, ret;
	unsigned long max_rate = get_cpu_freq() / 1000;

	if (!is_in_host()) {
		pr_warn("cpufreq driver of Sunway platforms is only supported in host mode\n");
		return -ENODEV;
	}

	fill_freq_table(freq_table);
	for (i = 0; freq_table[i].frequency != CPUFREQ_TABLE_END; i++) {
		if (max_rate == freq_table[i].frequency)
			freq_table[i+1].frequency = CPUFREQ_TABLE_END;
	}

	ret = cpufreq_register_driver(&sunway_cpufreq_driver);
	if (ret)
		return ret;

	return 0;
}
device_initcall(cpufreq_init);

