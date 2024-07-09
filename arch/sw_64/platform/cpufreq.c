// SPDX-License-Identifier: GPL-2.0

#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <asm/sw64_init.h>
#include <asm/hw_init.h>
#include <asm/debug.h>
#include <asm/cpufreq.h>
#include <asm/platform.h>

#define MAX_RETRY	10

static struct platform_device sw64_cpufreq_device = {
	.name = "sw64_cpufreq",
	.id = -1,
};

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
#elif CONFIG_PLATFORM_XUELANG
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

static int __init sw64_cpufreq_init(void)
{
	int i;
	unsigned long max_rate = get_cpu_freq() / 1000;

	fill_freq_table(freq_table);
	for (i = 0; freq_table[i].frequency != CPUFREQ_TABLE_END; i++) {
		if (max_rate == freq_table[i].frequency)
			freq_table[i+1].frequency = CPUFREQ_TABLE_END;
	}
	return platform_device_register(&sw64_cpufreq_device);
}
arch_initcall(sw64_cpufreq_init);

static struct clk cpu_clk = {
	.name = "cpu_clk",
	.flags = CLK_ALWAYS_ENABLED | CLK_RATE_PROPAGATES,
	.rate = STARTUP_RATE,
};

struct clk *sw64_clk_get(struct device *dev, const char *id)
{
	return &cpu_clk;
}
EXPORT_SYMBOL(sw64_clk_get);

unsigned int __sw64_cpufreq_get(struct cpufreq_policy *policy)
{
	int i;
	u64 val;
	void __iomem *spbu_base = misc_platform_get_spbu_base(0);
	struct cpufreq_frequency_table *ft = policy->freq_table;

	val = readq(spbu_base + OFFSET_CLK_CTL) >> CORE_PLL2_CFG_SHIFT;
	val &= CORE_PLL2_CFG_MASK;

	for (i = 0; ft[i].frequency != CPUFREQ_TABLE_END; i++) {
		if (val == i)
			return ft[i].frequency;
	}
	return 0;
}
EXPORT_SYMBOL(__sw64_cpufreq_get);

int sw64_set_rate(unsigned int index)
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
EXPORT_SYMBOL_GPL(sw64_set_rate);
