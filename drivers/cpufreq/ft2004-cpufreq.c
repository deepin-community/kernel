/*
 *	ft2000plus_freq.c: cpufreq driver for the phytium ft2000plus CPU
 *
 *	Copyright (C) 2019 Wang Yinfeng <wangyinfeng@phytium.com>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *
 *	2019-01-09: - initial revision
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/delay.h>
#include <linux/cpufreq.h>
#include <linux/timex.h>
#include <linux/io.h>
#include <asm/smp_plat.h>

#include <asm/dmi.h>

#define REG_PCRU_BASE   0x28100000
#define REG_PCRU_SIZE   0x1000
#define REG_PLL_Cx_CONF(cluster_id)  (0x600+(cluster_id*4))
#define REG_PLL_Cx_REAL(cluster_id, pll_channel)  (0x700+(cluster_id*8)+pll_channel*4)
#define REG_DFS_TRIG    0x80c
#define CLUSTER0_TRIG   0x1
#define CLUSTER1_TRIG   0x2
#define REG_DFS_STAT    0x810
#define CLUSTER0_STAT   0x1
#define CLUSTER1_STAT   0x2

static void __iomem * reg_pcru_base = NULL;
static DEFINE_SPINLOCK(cpufreq_lock);

static struct cpufreq_frequency_table ft2004_freq_table[] = {
	{.frequency = 800000},
	{.frequency = 1000000},
	{.frequency = 1200000},
	{.frequency = 1400000},
	{.frequency = 1600000},
	{.frequency = 1800000},
	{.frequency = 2000000},
	{.frequency = 2200000},
	{.frequency = 2500000},
	{.frequency = CPUFREQ_TABLE_END},
};

struct reg_2_pll_map_t
{
    unsigned int reg_value;
    unsigned int pll_value;
};

static struct reg_2_pll_map_t reg_2_pll_map[] =
{
    {0x22020085, 800000},
    {0x12020053, 1000000},
    {0x12020064, 1200000},
    {0x12020075, 1400000},
    {0x12020085, 1600000},
    {0x1102004b, 1800000},
    {0x11020053, 2000000},
    {0x1102005C, 2200000},
    {0x11020068, 2500000},
};

static unsigned int ft2004_freq_get_cpu_frequency(unsigned int cpu)
{
    unsigned int reg_value, pll_value;
    unsigned long cluster_id = cpu_logical_map(cpu) & ~0xffUL;
    int i, num = 0;

    pr_info("%s_%d: get cpu(%d) cpufreq\n", __func__, __LINE__, cpu);

    if(0 == (readl(reg_pcru_base + REG_DFS_STAT) & (1<<cluster_id)))
        reg_value = readl(reg_pcru_base + REG_PLL_Cx_REAL(cluster_id, 0));
    else
        reg_value = readl(reg_pcru_base + REG_PLL_Cx_REAL(cluster_id, 1));

    num = sizeof(reg_2_pll_map)/sizeof(struct reg_2_pll_map_t);

    for(i=0; i<num; i++)
    {
        if(reg_value == reg_2_pll_map[i].reg_value)
        {
            pll_value = reg_2_pll_map[i].pll_value;
            break;
        }
    }
    if(i == num)
    {
        pr_err("cpu%d reg_value(0x%x) is not in reg_2_pll_map table\n", cpu, reg_value);
        pll_value = 0;
    }

    return pll_value;
}

static int ft2004_freq_target(struct cpufreq_policy *policy, unsigned int index)
{
    unsigned long cluster_id = cpu_logical_map(policy->cpu) & ~0xffUL;
    pr_info("%s_%d: set cpu(%d) cpufreq index:%d\n", __func__, __LINE__, policy->cpu, index);

    if(index >= sizeof(ft2004_freq_table)/sizeof(struct cpufreq_frequency_table))
    {
        pr_err("set index(%d) is out range of initial table\n", index);
        return -EINVAL;
    }

    spin_lock(&cpufreq_lock);


    pr_info("%s_%d: set cpu(%d) cpufreq reg_value:0x%x\n", __func__, __LINE__, policy->cpu, reg_2_pll_map[index].reg_value);
    writel(reg_2_pll_map[index].reg_value, reg_pcru_base + REG_PLL_Cx_CONF(cluster_id));

    mdelay(1);
    writel(1<<cluster_id, reg_pcru_base + REG_DFS_TRIG);

    spin_unlock(&cpufreq_lock);
    pr_info("set cpu(%d) cpufreq succussful\n", policy->cpu);
    return 0;
}

/*
 *	Module init and exit code
 */

static int ft2004_freq_cpu_init(struct cpufreq_policy *policy)
{
        pr_info("%s\n", __func__);
	/* cpuinfo and default policy values */
	policy->cpuinfo.transition_latency = 1000000; /* 1ms */
        policy->freq_table = ft2004_freq_table;
        return cpufreq_table_validate_and_sort(policy);
     //   return cpufreq_table_validate_and_show(policy, ft2004_freq_table);
}


static struct cpufreq_driver ft2004_freq_driver = {
	.get	= ft2004_freq_get_cpu_frequency,
	.verify	= cpufreq_generic_frequency_table_verify,
	.target_index = ft2004_freq_target,
	.init	= ft2004_freq_cpu_init,
	.name	= "ft2004_freq",
	.attr	= cpufreq_generic_attr,
};

static int __init ft2004_freq_init(void)
{
    int ret;

    if (!cpu_is_phytium()) {
        return -EPERM;
    }

    pr_info("%s\n", __func__);
    reg_pcru_base = ioremap(REG_PCRU_BASE, REG_PCRU_SIZE);
    if(IS_ERR(reg_pcru_base))
    {
        pr_err("mapping pcru register fail\n");
        return -ENOMEM;
    }


    ret = cpufreq_register_driver(&ft2004_freq_driver);
    if (ret) {
        iounmap(reg_pcru_base);
    }

    return ret;
}

static void __exit ft2004_freq_exit(void)
{
    int i;

    if (!cpu_is_phytium()) {
        return -EPERM;
    }

    cpufreq_unregister_driver(&ft2004_freq_driver);
    iounmap(reg_pcru_base);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yangxun <yangxun@phytium.com>");
MODULE_DESCRIPTION("cpufreq driver for phytium ft2004 CPU");

module_init(ft2004_freq_init);
module_exit(ft2004_freq_exit);

