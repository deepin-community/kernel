// SPDX-License-Identifier: GPL-2.0-only
/*
 *  drivers/cpufreq/cpufreq_seep.c
 *
 *  Copyright (C) 2024 heppen <hepeng68@huawei.com>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/cpufreq.h>
#include <linux/init.h>
#include <linux/module.h>
#include <acpi/cppc_acpi.h>

/*
 * Check if the platform supports required CPPC features for SEEP governor:
 * - auto_sel: Hardware-managed P-state selection
 * - auto_act_window: Autonomous activity window
 * - epp: Energy Performance Preference
 */
static bool seep_supported(void)
{
	const char *driver;
	u64 val;
	int ret;

	driver = cpufreq_get_current_driver();
	if (!driver || strcmp(driver, "cppc_cpufreq"))
		return false;

	ret = cppc_get_auto_sel(0, &val);
	if (ret)
		return false;

	ret = cppc_get_auto_act_window(0, &val);
	if (ret)
		return false;

	ret = cppc_get_epp_perf(0, &val);
	if (ret)
		return false;

	return true;
}

/*
 * Start the SEEP governor for the given policy.
 * Enable hardware-managed P-state selection.
 */
static int cpufreq_gov_seep_start(struct cpufreq_policy *policy)
{
	int ret;

	/* Enable BIOS frequency Scaling */
	ret = cppc_set_auto_sel(policy->cpu, 1);
	if (ret)
		pr_err("Failed to enable auto_sel: %d\n", ret);
	return ret;
}

/*
 * Stop the SEEP governor for the given policy.
 * Disable hardware-managed P-state selection.
 */
static void cpufreq_gov_seep_stop(struct cpufreq_policy *policy)
{
	int ret;

	/* Disable BIOS frequency Scaling */
	ret = cppc_set_auto_sel(policy->cpu, 0);
	if (ret)
		pr_err("Failed to disable auto_sel: %d\n", ret);
}

static struct cpufreq_governor cpufreq_gov_seep = {
	.name = "seep",
	.start = cpufreq_gov_seep_start,
	.stop = cpufreq_gov_seep_stop,
	.owner = THIS_MODULE,
};

static int __init cpufreq_gov_seep_init(void)
{
	if (!seep_supported()) {
		pr_err("SEEP governor requires cppc_cpufreq driver and CPPC feature support\n");
		return -ENODEV;
	}

	return cpufreq_register_governor(&cpufreq_gov_seep);
}

static void __exit cpufreq_gov_seep_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_seep);
}

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_SEEP
struct cpufreq_governor *cpufreq_default_governor(void)
{
	return &cpufreq_gov_seep;
}
#endif

module_init(cpufreq_gov_seep_init);
module_exit(cpufreq_gov_seep_exit);

MODULE_AUTHOR("heppen <hepeng68@huawei.com>");
MODULE_DESCRIPTION("CPUfreq policy governor 'seep'");
MODULE_LICENSE("GPL");
