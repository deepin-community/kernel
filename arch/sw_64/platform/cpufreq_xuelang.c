// SPDX-License-Identifier: GPL-2.0

#include <linux/platform_device.h>

#include <asm/cpufreq.h>
#include <asm/delay.h>
#include <asm/sw64_init.h>

/* Minimum CLK support */
enum {
	DC_0, DC_1, DC_2, DC_3, DC_4, DC_5, DC_6, DC_7, DC_8,
	DC_9, DC_10, DC_11, DC_12, DC_13, DC_14, DC_15, DC_RESV
};

struct cpufreq_frequency_table freq_table[] = {
	{0, 200, CPUFREQ_ENTRY_INVALID},
	{0, DC_1, CPUFREQ_ENTRY_INVALID},
	{0, DC_2, 0},
	{0, DC_3, 0},
	{0, DC_4, 0},
	{0, DC_5, 0},
	{0, DC_6, 0},
	{0, DC_7, 0},
	{0, DC_8, 0},
	{0, DC_9, 0},
	{0, DC_10, 0},
	{0, DC_11, 0},
	{0, DC_12, 0},
	{0, DC_13, 0},
	{0, DC_14, 0},
	{0, DC_15, 0},
	{-1, DC_RESV, CPUFREQ_TABLE_END},
};


static struct platform_device sw64_cpufreq_device = {
	.name = "sw64_cpufreq",
	.id = -1,
};

static int __init sw64_cpufreq_init(void)
{
	int i;
	unsigned char external_clk;
	unsigned long max_rate, freq_off;

	max_rate = get_cpu_freq() / 1000;

	external_clk = *((unsigned char *)__va(MB_EXTCLK));

	if (external_clk == 240)
		freq_off = 60000;
	else
		freq_off = 50000;

	/* clock table init */
	for (i = 0; freq_table[i].frequency != CPUFREQ_TABLE_END; i++) {
		if (i == 1)
			freq_table[i].driver_data = freq_off * 24;
		if (i == 2)
			freq_table[i].frequency = freq_off * 36;
		if (i > 2)
			freq_table[i].frequency = freq_off * 38 + ((i - 3) * freq_off);

		if (freq_table[i].frequency == max_rate)
			freq_table[i + 1].frequency = CPUFREQ_TABLE_END;
	}

	return platform_device_register(&sw64_cpufreq_device);
}
arch_initcall(sw64_cpufreq_init);

char curruent_policy[CPUFREQ_NAME_LEN];

static struct clk cpu_clk = {
	.name = "cpu_clk",
	.flags = CLK_ALWAYS_ENABLED | CLK_RATE_PROPAGATES,
	.rate = 2400000000,
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
	struct cpufreq_frequency_table *ft = policy->freq_table;

	val = sw64_io_read(0, CLK_CTL) >> CORE_PLL2_CFG_SHIFT;

	for (i = 0; ft[i].frequency != CPUFREQ_TABLE_END; i++) {
		if (val == i)
			return ft[i].frequency;
	}
	return 0;
}
EXPORT_SYMBOL(__sw64_cpufreq_get);

void sw64_set_rate(unsigned int index)
{
	unsigned int i, val;
	int cpu_num;

	cpu_num = sw64_chip->get_cpu_num();

	for (i = 0; i < cpu_num; i++) {
		sw64_io_write(i, CLK_CTL, CORE_CLK2_R | CORE_CLK2_V | CLK_PRT);
		val = sw64_io_read(i, CLK_CTL);

		sw64_io_write(i, CLK_CTL, val | index << CORE_PLL2_CFG_SHIFT);

		udelay(1);

		sw64_io_write(i, CLK_CTL, CORE_CLK2_V | CLK_PRT
				| index << CORE_PLL2_CFG_SHIFT);
		val = sw64_io_read(i, CLK_CTL);

		/* LV1 select PLL1/PLL2 */
		sw64_io_write(i, CLU_LV1_SEL, CLK_LV1_SEL_MUXA | CLK_LV1_SEL_PRT);

		/* Set CLK_CTL PLL0 */
		sw64_io_write(i, CLK_CTL, val | CORE_CLK0_R | CORE_CLK0_V);

		sw64_io_write(i, CLK_CTL, val | CORE_CLK0_R | CORE_CLK0_V
				| index << CORE_PLL0_CFG_SHIFT);

		udelay(1);

		sw64_io_write(i, CLK_CTL, val | CORE_CLK0_V
				| index << CORE_PLL0_CFG_SHIFT);

		/* LV1 select PLL0/PLL1 */
		sw64_io_write(i, CLU_LV1_SEL, CLK_LV1_SEL_MUXB | CLK_LV1_SEL_PRT);
	}
}
EXPORT_SYMBOL_GPL(sw64_set_rate);
