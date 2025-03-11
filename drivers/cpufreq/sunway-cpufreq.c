// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2025 WXIAT
 */

#define pr_fmt(fmt) "cpufreq: " fmt

#include <linux/cpufreq.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>

#include <asm/hw_init.h>
#include <asm/cpufreq.h>
#include <asm/sw64io.h>

static unsigned int sw64_cpufreq_get(unsigned int cpu)
{
	struct cpufreq_policy *policy = cpufreq_cpu_get_raw(cpu);

	if (!policy) {
		pr_err("%s: no policy associated to cpu: %d\n",
				__func__, cpu);
		return 0;
	}

	return __sw64_cpufreq_get(policy);
}

/*
 * Here we notify other drivers of the proposed change and the final change.
 */
static int sw64_cpufreq_target(struct cpufreq_policy *policy,
				     unsigned int index)
{
	int ret;
	unsigned int cpu = policy->cpu;

	if (!cpu_online(cpu))
		return -ENODEV;

	/* setting the cpu frequency */
	ret = sw64_set_rate(index);
	if (ret)
		return ret;
	update_cpu_freq(freq_table[index].frequency);

	return 0;
}

static int sw64_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
	cpufreq_generic_init(policy, freq_table, 0);

	return 0;
}

static int sw64_cpufreq_verify(struct cpufreq_policy_data *policy)
{
	return cpufreq_frequency_table_verify(policy, freq_table);
}

static int sw64_cpufreq_exit(struct cpufreq_policy *policy)
{
	return 0;
}

static struct freq_attr *sw64_table_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs, NULL,
};

static struct cpufreq_driver sw64_cpufreq_driver = {
	.name = "sw64",
	.init = sw64_cpufreq_cpu_init,
	.verify = sw64_cpufreq_verify,
	.target_index = sw64_cpufreq_target,
	.get = sw64_cpufreq_get,
	.exit = sw64_cpufreq_exit,
	.attr = sw64_table_attr,
};

static int __init cpufreq_init(void)
{
	if (is_in_guest()) {
		pr_warn("Now sw_64 CPUFreq does not support virtual machines\n");
		return -ENODEV;
	}

	pr_info("SW-64 CPU frequency driver\n");

	return cpufreq_register_driver(&sw64_cpufreq_driver);
}
device_initcall(cpufreq_init);

