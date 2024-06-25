// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * loongson3-acpi-cpufreq.c - Loongson ACPI Processor P-States Driver
 *
 *  Copyright (C) 2020  lvjianmin <lvjianmin@loongson.cn>
 *			Yijun <yijun@loongson.cn>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/sched.h>
#include <linux/cpufreq.h>
#include <linux/compiler.h>
#include <linux/sched/cpufreq.h>
#include <linux/dmi.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/acpi.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/processor.h>
#include <linux/cpufeature.h>
#include <uapi/linux/sched/types.h>
#include <acpi/processor.h>

#include <asm/loongson.h>
#include "cpufreq_governor.h"

#include <asm/time.h>
#define CPU_ID_FIELD    0xf

#define COMPLETE_STATUS 0x80000000
#define VOLTAGE_COMMAND 0x21

#define DVFS_INFO	0x22
#define DVFS_INFO_BOOST_LEVEL	0x23
#define DVFS_INFO_MIN_FREQ	0xf
#define DVFS_INFO_MAX_FREQ	0xf0
#define DVFS_INFO_BOOST_CORE_FREQ	0xff00
#define DVFS_INFO_NORMAL_CORE_UPPER_LIMIT	0xf0000
#define DVFS_INFO_BOOST_CORES	0xf00000

#define BOOST_MODE	0x80000
#define NORMAL_MODE	0x40000

MODULE_DESCRIPTION("Loongson 3A ACPI Processor P-States Driver");

MODULE_LICENSE("GPL");

#define CPUFREQ_SAMPLING_INTERVAL	(2 * TICK_NSEC / NSEC_PER_USEC)
#define LOONGSON_CONTROL_MASK		(0xFF)
#define FACTOR				(0xeac0c6e8)
#define BOOST_THRESHOLD			(900)
#define MAX_CORES_PER_PACKAGE		64
#define CPU_ID_FIELD			0xf
#define VOLTAGE_COMMAND			0x21
#define MAX_READY_TIMEOUT		200000000
#define RESERVED_FREQ			3

#define LOONGSON_BOOST_FREQ_MASK		(0x7 << 8)
#define FREQ_STEP		(25)

static struct mutex boost_mutex[MAX_PACKAGES];
static bool cpufreq_has_boost_freq;
static int max_boost_cores;
static int boost_gears;
static int boost_freqs[NR_CPUS + 1];
struct package_data;
struct core_data;
static struct acpi_processor_performance __percpu *acpi_perf_data;
static struct cpufreq_driver loongson3_cpufreq_driver;
static struct freq_attr *loongson3_cpufreq_attr[];
DECLARE_PER_CPU(struct clock_event_device, stable_clockevent_device);
static inline struct core_data *get_core_data(int cpu);

static int min_freq_level;
static int max_freq_level;
static int max_upper_index;
static int max_boost_freq;

/* threshold of core's get into msa */
static int lsx_count_threshold = 200;
/* threshold of core's get into lasx */
static int lasx_count_threshold = 200;
/* other cores' upper load threshold when 1 core get into boost mode and enable msa/lasx */
static int load_threshold = 60;

DEFINE_PER_CPU(unsigned long, lsx_count);
EXPORT_PER_CPU_SYMBOL(lsx_count);

#if defined(CONFIG_CPU_HAS_LASX)
DEFINE_PER_CPU(unsigned long, lasx_count);
EXPORT_PER_CPU_SYMBOL(lasx_count);
#endif

struct ce_update_data {
	struct clock_event_device *cd;
	unsigned int new_freq;
};

static struct kthread_worker	cpufreq_worker;
static struct task_struct	*cpufreq_thread;
/**
 * struct core_data -	Store core related information
 * @in_boost:		the core is boosting to boost_freq
 * @cpu:		logical cpu of the core
 * @update_util		The update_util_data pointer of @cpu, is passed to the callback
 *			function, which will be called by cpufreq_update_util()
 * @package		The package_data structure the core belonged to
 * @work_in_progress	@work is busy
 * @irq_work		to enqueue callback handling on irq workqueue
 * @work		to enqueue work from irq workqueue on system workqueue
 * @perf		store frequency table related information from ACPI table
 * @max_freq		max normal freq of cpu
 * @boost_freq		max boost freq of cpu
 * @clock_scale		clock scale to calculate cpu_data[cpu].udelay_val in boost mode
 * @package_id		package id of core
 * @shift		clock shift to calculate cpu_data[cpu].udelay_val in boost mode
 * @update_util_set	if callback has been set for cpufreq_update_util()
 * @load		current load of the core
 * @last_freq_update_time	last freq update time
 * @freq_update_delay_ns	min interval of freq update, which is
 *			transition_latency configured in ACPI table
 *
 * following elements are used to calculate load of the core
 * @prev_update_time
 * @prev_cpu_idle
 * @prev_load
 * @sampling_rate
 *
 */
struct core_data {
	bool in_boost;
	int cpu;
	struct update_util_data update_util;
	struct package_data *package;
	bool work_in_progress;
	struct	irq_work irq_work;
	struct kthread_work work;
	struct acpi_processor_performance *perf;
	unsigned int normal_max_freq;
	unsigned int *boost_freq;
	unsigned int *clock_scale;
	unsigned int package_id;
	unsigned int *shift;
	bool update_util_set;
	unsigned long long load;

	u64 last_freq_update_time;
	s64 freq_update_delay_ns;
	u64 prev_update_time;
	u64 prev_cpu_idle;
	u32 prev_load;
	u32 sampling_rate;
};

struct package_data {
	int boost_cores;
	int max_boost_cores;
	int nr_cores;
	char in_boost;
	int nr_full_load_cores;
	struct core_data core[MAX_CORES_PER_PACKAGE];
} all_package_data[MAX_PACKAGES];

static bool boost_supported(void)
{
	return loongson3_cpufreq_driver.set_boost;
}

/*
 * Check if target_freq is a boost freq
 *
 * target_freq must be a freq in freq table when
 * calling the function.
 */
static int boost_level(struct acpi_processor_performance *perf, unsigned int target_freq)
{
	int i;

	for (i = 0; i < perf->state_count; i++) {
		if (target_freq == (perf->states[i].core_frequency * 1000))
			return (perf->states[i].control & LOONGSON_BOOST_FREQ_MASK) >> 8;
	}
	return 0;
}

#ifdef CONFIG_SMP
static int loongson3_cpu_freq_notifier(struct notifier_block *nb,
		unsigned long val, void *data)
{
	struct cpufreq_freqs *freqs;
	struct clock_event_device __maybe_unused *cd;
	struct core_data *core;
	unsigned int __maybe_unused new_freq;
	unsigned long cpu;
	struct ce_update_data __maybe_unused ce_data;
	int cur_boost_level;

	if (val == CPUFREQ_POSTCHANGE) {
		freqs = (struct cpufreq_freqs *)data;
		cpu = freqs->policy->cpu;
		core = get_core_data(cpu);
		cur_boost_level = boost_level(core->perf, freqs->new);
		if (cur_boost_level != 0) {
			lpj_fine = (unsigned int) (((int64_t)core->clock_scale[cur_boost_level] *
						cpufreq_scale(loops_per_jiffy, boost_freqs[cur_boost_level] * 1000,
							freqs->new)) / core->shift[cur_boost_level]);
		} else {
			lpj_fine =
				cpufreq_scale(loops_per_jiffy, core->normal_max_freq * 1000, freqs->new);
		}
	}

	return 0;
}
#else
static int loongson3_cpu_freq_notifier(struct notifier_block *nb,
		unsigned long val, void *data)
{
	struct cpufreq_freqs *freqs;
	struct clock_event_device __maybe_unused *cd;
	struct core_data *core;
	unsigned int __maybe_unused new_freq;
	unsigned long cpu;
	int cur_boost_level;

	if (val == CPUFREQ_POSTCHANGE) {

		freqs = (struct cpufreq_freqs *)data;
		cpu = freqs->cpu;
		core = get_core_data(cpu);
		cur_boost_level = boost_level(core->perf, target_freq);

		if (cur_boost_level != 0) {
			lpj_fine = (unsigned int) (((int64_t)core->clock_scale[cur_boost_level] *
						loops_per_jiffy) / core->shift[cur_boost_level]);
		} else {
			lpj_fine = loops_per_jiffy;
		}
	}

	return 0;
}
#endif
static struct notifier_block loongson3_cpufreq_notifier_block = {
	.notifier_call = loongson3_cpu_freq_notifier
};

static int cpufreq_perf_find_level(struct acpi_processor_performance *perf,
		unsigned int target_freq,
		unsigned int boost_level)
{
	int i;

	for (i = 0; i < perf->state_count; i++) {
		if (boost_level) {
			if (perf->states[i].control & LOONGSON_BOOST_FREQ_MASK) {
				if (target_freq == (perf->states[i].core_frequency * 1000))
					return perf->states[i].control & LOONGSON_CONTROL_MASK;
			}
		} else {
			if (!(perf->states[i].control & LOONGSON_BOOST_FREQ_MASK))
				if (target_freq == (perf->states[i].core_frequency * 1000))
					return perf->states[i].control;
		}
	}
	return 0;
}

static int cpufreq_perf_find_freq(struct acpi_processor_performance *perf,
		unsigned int target_index,
		unsigned int boost_level)
{
	int i;

	for (i = 0; i < perf->state_count; i++) {
		if (boost_level) {
			if (perf->states[i].control & LOONGSON_BOOST_FREQ_MASK)
				if (target_index == (perf->states[i].control & LOONGSON_CONTROL_MASK))
					return perf->states[i].core_frequency;
		} else {
			if (!(perf->states[i].control & LOONGSON_BOOST_FREQ_MASK))
				if (target_index == perf->states[i].control)
					return perf->states[i].core_frequency;
		}
	}

	return 0;
}


static inline struct core_data *get_core_data(int cpu)
{
	int package_id = cpu_data[cpu].package;
	struct package_data *package = &all_package_data[package_id];
	int core_id = cpu_logical_map(cpu) % package->nr_cores;

	return &package->core[core_id];
}

static bool package_boost(struct package_data *package)
{
	int i;
	int cur_full_load = 0;

#if defined(CONFIG_CPU_HAS_LASX)
	int lasx_enable_count = 0;
	unsigned long lasx_num;
	bool clear_lasx = false;
#endif

	int msa_enable_count = 0;
	unsigned long msa_num;
	bool clear_msa = false;

	for (i = 0; i < package->nr_cores; i++) {

#if defined(CONFIG_CPU_HAS_LASX)
		lasx_num = per_cpu(lasx_count, package->core[i].cpu);

		if (lasx_num)
			lasx_enable_count++;

		if (lasx_num >= lasx_count_threshold)
			clear_lasx = true;

		pr_debug("%s: lasx enabled, i %d, cpu %d, lasx_num %lu\n",
				__func__, i, package->core[i].cpu, lasx_num);
#endif
		msa_num = per_cpu(lsx_count, package->core[i].cpu);

		if (msa_num)
			msa_enable_count++;

		if (msa_num >= lsx_count_threshold)
			clear_msa = true;

		pr_debug("%s: msa enabled, i %d, cpu %d, msa_num %lu\n",
				__func__, i, package->core[i].cpu, msa_num);

		if (package->core[i].prev_load >= load_threshold)
			cur_full_load++;
	}

#if defined(CONFIG_CPU_HAS_LASX)
	if (clear_lasx) {
		for (i = 0; i < package->nr_cores; i++)
			per_cpu(lasx_count, package->core[i].cpu) = 0;
	}
#endif

	if (clear_msa) {
		for (i = 0; i < package->nr_cores; i++)
			per_cpu(lsx_count, package->core[i].cpu) = 0;
	}

#if defined(CONFIG_CPU_HAS_LASX)
	if (lasx_enable_count > 1
		|| (lasx_enable_count && package->nr_full_load_cores > 1)
		|| (lasx_enable_count && cur_full_load > 1)) {
		return false;
	}
#endif

	if (msa_enable_count > 1
	|| (msa_enable_count && package->nr_full_load_cores > 1)
	|| (msa_enable_count && cur_full_load > 1)) {
		return false;
	}

	if (package->nr_full_load_cores &&
			package->nr_full_load_cores <= package->max_boost_cores)
		return true;

	return false;
}

/*
 * check if the cpu can be boosted.
 *
 * call the function after load of cpu updated.
 */
static bool cpu_can_boost(int cpu)
{
	struct core_data *core = get_core_data(cpu);
	struct package_data *package = core->package;

	if (package->boost_cores >= package->max_boost_cores)
		return false;
	if (core->load > BOOST_THRESHOLD)
		return true;

	return false;
}

static void do_set_freq_level(int cpu, int freq_level)
{
	uint32_t message;
	uint32_t val;

	message = (0 << 31) | (VOLTAGE_COMMAND << 24)
		| ((uint32_t)freq_level << 4)
		| (cpu & CPU_ID_FIELD);
	iocsr_write32(message, 0x51c);
	val = iocsr_read32(0x420);

	val |= 1 << 10;
	iocsr_write32(val, 0x420);
}

static int wait_for_ready_timeout(int64_t timeout)
{
	int ret;
	struct timespec64 prev_ts;
	struct timespec64 curr_ts;
	ktime_t delay = ktime_set(0, 100);

	ktime_get_ts64(&prev_ts);
	ktime_get_ts64(&curr_ts);

	ret = -EPERM;
	while (((curr_ts.tv_sec - prev_ts.tv_sec) * 1000000000 + (curr_ts.tv_nsec - prev_ts.tv_nsec)) < timeout) {
		ktime_get_ts64(&curr_ts);

		if (iocsr_read32(0x51c) & COMPLETE_STATUS) {
			ret = 0;
			break;
		}

		__set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_hrtimeout(&delay, HRTIMER_MODE_REL);
	}
	return ret;
}

/* Find closest freq to target in a table in ascending order */
static int cpufreq_table_find_freq_ac(struct cpufreq_policy *policy,
		unsigned int target_freq,
		int boost_level)
{
	struct cpufreq_frequency_table *table = policy->freq_table;
	struct cpufreq_frequency_table *pos;
	unsigned int freq;
	unsigned int best_freq = 0;
	int idx, best = -1;

	cpufreq_for_each_valid_entry_idx(pos, table, idx) {
		freq = pos->frequency;

		if (pos->driver_data != boost_level)
			continue;
		if (freq > policy->max || freq < policy->min)
			continue;
		if (freq == target_freq)
			return freq;

		if (freq < target_freq) {
			best = idx;
			best_freq = freq;
			continue;
		}

		/* No freq found below target_freq, return freq above target_freq */
		if (best == -1)
			return freq;

		/* Choose the closest freq */
		if (target_freq - table[best].frequency > freq - target_freq)
			return freq;

		return best_freq;
	}

	return best_freq;
}

/* Find closest freq to target in a table in descending order */
static int cpufreq_table_find_freq_dc(struct cpufreq_policy *policy,
					unsigned int target_freq,
					int boost_level)
{
	struct cpufreq_frequency_table *table = policy->freq_table;
	struct cpufreq_frequency_table *pos;
	unsigned int freq;
	unsigned int best_freq = 0;
	int idx, best = -1;

	cpufreq_for_each_valid_entry_idx(pos, table, idx) {
		freq = pos->frequency;

		if (pos->driver_data != boost_level)
			continue;
		if (freq > policy->max || freq < policy->min)
			continue;

		if (freq == target_freq)
			return freq;

		if (freq > target_freq) {
			best = idx;
			best_freq = freq;
			continue;
		}

		/* No freq found above target_freq, return freq below target_freq */
		if (best == -1)
			return freq;

		/* Choose the closest freq */
		if (table[best].frequency - target_freq > target_freq - freq)
			return freq;
		return best_freq;
	}

	return best_freq;
}

/* Works only on sorted freq-tables */
static int cpufreq_table_find_freq(struct cpufreq_policy *policy,
					unsigned int target_freq,
					int boost_level)
{
	target_freq = clamp_val(target_freq, policy->min, policy->max);
	if (policy->freq_table_sorted == CPUFREQ_TABLE_SORTED_ASCENDING)
		return cpufreq_table_find_freq_ac(policy, target_freq, boost_level);
	else
		return cpufreq_table_find_freq_dc(policy, target_freq, boost_level);
}

static void transition_end(struct cpufreq_policy *policy,
		struct cpufreq_freqs *freqs, bool failed)
{
	if (unlikely(!policy->transition_ongoing))
		return;
	cpufreq_freq_transition_end(policy, freqs, failed);
}
static void transition_begin(struct cpufreq_policy *policy,
		struct cpufreq_freqs *freqs)
{
	if (unlikely(policy->transition_ongoing))
		cpufreq_freq_transition_end(policy, freqs, true);

	cpufreq_freq_transition_begin(policy, freqs);
}

static void update_core_boost_info(struct core_data *core, bool boost_set)
{
	core->in_boost = boost_set;
	if (boost_set)
		core->package->boost_cores++;
	else
		core->package->boost_cores--;
}

static unsigned int cores_freq_trans_notify(struct package_data *package,
						bool before_trans,
						bool trans_failed,
						int find_level,
						int find_freq,
						unsigned int skip_cpumask)
{
	int i;
	struct cpufreq_policy *policy;
	struct cpufreq_freqs freqs;
	unsigned int cores_level = 0;
	unsigned int core_level;

	for (i = 0; i < package->nr_cores; i++) {
		struct core_data *core = &package->core[i];

		policy = cpufreq_cpu_get_raw(core->cpu);
		if (((1 << i) & skip_cpumask) || !policy)
			continue;
		freqs.old = policy->cur;
		freqs.flags = 0;

		/* find level from normal levels */
		core_level = cpufreq_perf_find_level(core->perf, policy->cur, find_level);
		if (!core_level) {
			pr_debug("cpu%d policy->cur=%d find_level=%d freq=%d skip_cpumask=%x \n",
					policy->cpu, policy->cur,
					find_level, find_freq, skip_cpumask);
		}
		freqs.new = cpufreq_perf_find_freq(core->perf, core_level, find_freq) * 1000;
		if (!freqs.new)
			pr_debug("%s: find freq error\n", __func__);

		pr_debug("%s: cpu %d, old freq %d, new freq %d, find_level %d, find_freq %d\n",
				__func__, policy->cpu, freqs.old, freqs.new, find_level, find_freq);
		cores_level |= (core_level << (i << 2));

		if (before_trans)
			transition_begin(policy, &freqs);
		else
			transition_end(policy, &freqs, trans_failed);
	}
	return cores_level;
}
static int loongson3_set_freq(struct core_data *core, unsigned long freq, int boost_level)
{
	int ret = 0;
	int freq_level;
	int phy_cpu;
	int target_freq;
	struct cpufreq_freqs freqs;
	struct cpufreq_policy *policy = cpufreq_cpu_get_raw(core->cpu);

	if (!policy)
		return -EINVAL;

	ret = wait_for_ready_timeout(MAX_READY_TIMEOUT);
	if (ret)
		return ret;

	phy_cpu = cpu_logical_map(core->cpu);
	target_freq = cpufreq_table_find_freq(policy, freq, boost_level);
	if (!target_freq)
		return -1;
	if (target_freq == policy->cur)
		return -1;

	freqs.flags = 0;
	freqs.old = policy->cur;
	freqs.new = target_freq;
	freq_level = cpufreq_perf_find_level(core->perf, target_freq, boost_level);
	if (!freq_level) {
		pr_debug("%s: cpu%d freq=%lu targetfreq=%d boost_level=%d find level error\n",
				__func__, core->cpu, freq, target_freq, boost_level);
	}

	transition_begin(policy, &freqs);
	do_set_freq_level(phy_cpu, freq_level);
	ret = wait_for_ready_timeout(MAX_READY_TIMEOUT);
	transition_end(policy, &freqs, !!ret);

	return ret;
}

int loongson3_set_mode(int mode, int freq_level)
{
	uint32_t val;
	int ret = 0;
	uint32_t message;

	ret = wait_for_ready_timeout(MAX_READY_TIMEOUT);
	if (ret)
		return ret;

	message = mode | (VOLTAGE_COMMAND << 24) | freq_level;
	iocsr_write32(message, 0x51c);
	val = iocsr_read32(0x420);
	val |= 1 << 10;
	iocsr_write32(val, 0x420);
	return wait_for_ready_timeout(MAX_READY_TIMEOUT);
}

enum freq_adjust_action {
	FAA_NORMAL,
	FAA_N2B,
	FAA_B2N,
	FAA_BOOST,
};

static int faa_normal(struct cpufreq_policy *policy, int load)
{
	int ret;
	unsigned int freq_next, min_f, max_f;
	struct core_data *core = get_core_data(policy->cpu);

	if (!core)
		return -1;

	min_f = policy->min;
	max_f = policy->max;
	freq_next = min_f + load * (max_f - min_f) / 100;
	ret = loongson3_set_freq(core, freq_next, 0);
	return ret;
}

static void handle_boost_cores(struct core_data *core, struct package_data *package,
		unsigned long target_freq, bool skip_update_and_notify, bool update_core, bool inc_boost)
{
	int boost_level;
	int find_level;
	int find_freq;
	int ret;
	int inc_core = inc_boost ? 1 : -1;

	if (boost_gears == 1) {
		find_level = 0;
		boost_level = boost_gears;
	} else {
		find_level = package->boost_cores;
		if (update_core)
			boost_level = package->boost_cores + inc_core;
		else
			boost_level = package->boost_cores;
	}
	find_freq = boost_level;
	ret = loongson3_set_freq(core, target_freq, boost_level);
	if (ret)
		return;

	if (skip_update_and_notify) {
		if (update_core)
			update_core_boost_info(core, inc_boost);
		return;
	}

	if (boost_gears != 1) {
		cores_freq_trans_notify(package, true, false,
				find_level, find_freq, 1 << core->cpu);
		cores_freq_trans_notify(package, false, false,
				find_level, find_freq, 1 << core->cpu);
	}
	if (update_core)
		update_core_boost_info(core, inc_boost);
}

static void faa_boost(struct cpufreq_policy *policy, int load)
{
	unsigned int min_f, max_f;
	struct core_data *core = get_core_data(policy->cpu);
	struct package_data *package = core->package;
	unsigned long target_freq;

	/* boost cores form n to n + 1 */
	if (core->load > BOOST_THRESHOLD) {
		if (package->boost_cores < package->max_boost_cores
				&& !core->in_boost) {
			if (boost_gears == 1) {
				target_freq = policy->max;
			} else {
				target_freq = cpufreq_table_find_freq(policy, policy->max,
								package->boost_cores + 1);
				if (!target_freq) {
					pr_debug("%s: find freq error ,boost_level %d, cur freq %d\n",
							__func__, package->boost_cores, policy->max);
				}
			}
			handle_boost_cores(core, package, target_freq, false, true, true);
		}
	} else {
		/* 1. core not in boost, level up but not change  pll
		 * 2. core in boost, boost cores from n to n - 1
		 */
		min_f = policy->min;
		max_f = policy->max;
		target_freq = min_f + load * (max_f - min_f) / 100;
		handle_boost_cores(core, package, target_freq, !core->in_boost, core->in_boost, false);
	}


}

static void get_boost_cores(struct package_data *package, int *boost_cores, int *boost_count)
{
	struct core_data *core;
	struct cpufreq_policy *policy;
	int i;

	/* count boost cores */
	for (i = 0; i < package->nr_cores; i++) {
		core = &package->core[i];
		policy = cpufreq_cpu_get_raw(core->cpu);
		if (!policy)
			continue;

		if (cpu_can_boost(core->cpu)) {
			if (boost_cores)
				*boost_cores |= (1 << i);

			(*boost_count)++;
		}
	}
}

static void faa_n2b(struct package_data *package, struct core_data *core)
{
	int boost_cores = 0;
	int boost_count = 0;
	int freq_level;
	int i;

	get_boost_cores(package, &boost_cores, &boost_count);

	if (boost_gears == 1)
		boost_count = 1;

	freq_level = cores_freq_trans_notify(package, true, false,
			0, boost_count, 0);
	if (!loongson3_set_mode(BOOST_MODE, freq_level)) {
		cores_freq_trans_notify(package, false, false,
				0, boost_count, 0);
		package->in_boost = true;
		for (i = 0; i < package->nr_cores; i++) {
			if (boost_cores & (1 << i))
				update_core_boost_info(&package->core[i], true);
		}
	} else
		cores_freq_trans_notify(package, false, true,
				0, boost_count, 0);
}

static void faa_b2n(struct package_data *package)
{
	int i;
	int boost_count = package->boost_cores;

	if (boost_gears == 1)
		boost_count = 1;

	cores_freq_trans_notify(package, true, false,
			boost_count, 0, 0);
	if (!loongson3_set_mode(NORMAL_MODE, 0)) {
		cores_freq_trans_notify(package, false, false,
				boost_count, 0, 0);
		for (i = 0; i < package->nr_cores; i++) {
			if (package->core[i].in_boost)
				update_core_boost_info(&package->core[i], false);
		}
		package->in_boost = false;
	} else
		cores_freq_trans_notify(package, false, true,
				boost_count, 0, 0);
}


unsigned int load_update(struct core_data *core)
{
	int i;
	u64 update_time, cur_idle_time;
	unsigned int idle_time, time_elapsed;
	unsigned int load = 0;
	struct package_data *package = core->package;

	cur_idle_time = get_cpu_idle_time(core->cpu, &update_time, true);

	time_elapsed = update_time - core->prev_update_time;
	core->prev_update_time = update_time;

	idle_time = cur_idle_time - core->prev_cpu_idle;
	core->prev_cpu_idle = cur_idle_time;

	if (unlikely(!time_elapsed)) {
		/*
		 * That can only happen when this function is called
		 * twice in a row with a very short interval between the
		 * calls, so the previous load value can be used then.
		 */
		load = core->prev_load;
	} else if (unlikely((int)idle_time > 2 * core->sampling_rate &&
				core->prev_load)) {

		load = core->prev_load;
		core->prev_load = 0;
	} else {
		if (time_elapsed >= idle_time)
			load = 100 * (time_elapsed - idle_time) / time_elapsed;
		else
			load = (int)idle_time < 0 ? 100 : 0;
		core->prev_load = load;
	}

	package->nr_full_load_cores = 0;
	for (i = 0; i < package->nr_cores; i++) {
		if (package->core[i].load > BOOST_THRESHOLD)
			package->nr_full_load_cores++;
	}

	return load;
}

static bool cpufreq_should_update_freq(struct core_data *core, u64 time)
{
	s64 delta_ns;

	delta_ns = time - core->last_freq_update_time;
	return delta_ns >= core->freq_update_delay_ns;
}

static void cpufreq_update(struct cpufreq_policy *policy)
{
	int action;
	struct core_data *core;
	struct package_data *package;
	unsigned long int load;
	bool should_be_boost = 0;

	core = get_core_data(policy->cpu);
	package = core->package;

	mutex_lock(&boost_mutex[core->package_id]);

	if (!core->update_util_set) {
		mutex_unlock(&boost_mutex[core->package_id]);
		return;
	}

	load = load_update(core);
	core->load = (u64)load + ((core->load * FACTOR) >> 32);

	if (cpufreq_boost_enabled()) {
		should_be_boost = package_boost(package);
	} else {
		if (package->in_boost)
			should_be_boost = false;
	}

	action = (package->in_boost << 1) | should_be_boost;
	switch (action) {
	case FAA_NORMAL:
		faa_normal(policy, load);
		break;
	case FAA_B2N:
		faa_b2n(package);
		break;
	case FAA_N2B:
		faa_n2b(package, core);
		break;
	case FAA_BOOST:
		faa_boost(policy, load);
		break;
	}
	mutex_unlock(&boost_mutex[core->package_id]);
}

static void set_max_within_limits(struct cpufreq_policy *policy)
{
	struct core_data *core = get_core_data(policy->cpu);
	/*
	 * policy->max <= cpu->pstate.max_freq indecates that
	 * the boost is disabled, so max freq is in normal range
	 *
	 * Skip performance policy with boost enabled!!!
	 *
	 */
	if (policy->max <= (core->normal_max_freq * 1000)) {
		mutex_lock(&boost_mutex[core->package_id]);
		if (!loongson3_set_freq(core, policy->max, 0))
			pr_debug("Set cpu %d to performance mode under normal range.\n",
					policy->cpu);
		mutex_unlock(&boost_mutex[core->package_id]);
	}
}

static void clear_update_util_hook(unsigned int cpu)
{
	struct core_data *core = get_core_data(cpu);

	if (!core->update_util_set)
		return;

	cpufreq_remove_update_util_hook(cpu);
	core->update_util_set = false;
	synchronize_rcu();
}

static void update_util_handler(struct update_util_data *data, u64 time,
		unsigned int flags)
{
	struct core_data *core = container_of(data, struct core_data, update_util);

	if (!cpufreq_should_update_freq(core, time))
		return;
	if (!core->work_in_progress) {
		core->last_freq_update_time = time;
		core->work_in_progress = true;
		irq_work_queue(&core->irq_work);
	}
}
static void set_update_util_hook(unsigned int cpu)
{
	struct core_data *core = get_core_data(cpu);

	if (core->update_util_set)
		return;

	cpufreq_add_update_util_hook(cpu, &core->update_util,
			update_util_handler);
	core->update_util_set = true;
}
static int loongson3_cpufreq_set_policy(struct cpufreq_policy *policy)
{
	if (!policy->cpuinfo.max_freq)
		return -ENODEV;

	if (policy->policy == CPUFREQ_POLICY_PERFORMANCE) {
		clear_update_util_hook(policy->cpu);
		set_max_within_limits(policy);
	} else {
		set_update_util_hook(policy->cpu);
	}

	return 0;
}

static int loongson3_cpufreq_verify_policy(struct cpufreq_policy_data *policy)
{
	cpufreq_verify_within_limits(policy, policy->cpuinfo.min_freq, policy->cpuinfo.max_freq);

	return 0;
}

static void set_boost_freq(bool has)
{
	cpufreq_has_boost_freq = has;
}

static bool has_boost_freq(void)
{
	return cpufreq_has_boost_freq;
}

static int compute_scale(int *shift, int dividor, int dividee)
{
	int i;
	int result = 0;
	int remainder = 0;
	int scale_resolution = 8;

	result = dividor / dividee;
	remainder = (dividor % dividee) * 10;

	for (i = 0; i < scale_resolution; i++) {
		result =  result * 10 +  remainder  / dividee;
		remainder = (remainder % dividee) * 10;
		*shift *= 10;
	}

	return result;
}

static void cpufreq_work_handler(struct kthread_work *work)
{
	struct core_data *core;
	struct cpufreq_policy *policy;

	core = container_of(work, struct core_data, work);
	policy = cpufreq_cpu_get_raw(core->cpu);

	if (policy) {
		cpufreq_update(policy);
		core->work_in_progress = false;
	}
}

static void cpufreq_irq_work(struct irq_work *irq_work)
{
	struct core_data *core = container_of(irq_work, struct core_data, irq_work);

	kthread_queue_work(&cpufreq_worker, &core->work);
}

static void cpufreq_kthread_stop(void)
{
	kthread_flush_worker(&cpufreq_worker);
	kthread_stop(cpufreq_thread);
}
static int cpufreq_kthread_create(void)
{
	struct sched_attr attr = {
		.size		= sizeof(struct sched_attr),
		.sched_policy	= SCHED_DEADLINE,
		.sched_flags	= 0x10000000,
		.sched_nice	= 0,
		.sched_priority	= 0,
		.sched_runtime	=  1000000,
		.sched_deadline = 10000000,
		.sched_period	= 10000000,
	};
	int ret;

	kthread_init_worker(&cpufreq_worker);
	cpufreq_thread = kthread_create(kthread_worker_fn, &cpufreq_worker, "lsfrq:%d", 0);
	if (IS_ERR(cpufreq_thread))
		return PTR_ERR(cpufreq_thread);

	ret = sched_setattr_nocheck(cpufreq_thread, &attr);
	if (ret) {
		kthread_stop(cpufreq_thread);
		pr_warn("%s: failed to set SCHED_DEADLINE\n", __func__);
		return ret;
	}

	wake_up_process(cpufreq_thread);

	return 0;
}

static int init_acpi(struct acpi_processor_performance *perf)
{
	int result = 0;
	int i;

	perf->shared_type = 0;
	perf->state_count = (max_freq_level - min_freq_level + 1) * (boost_gears + 1);

	perf->states =
		kmalloc_array(perf->state_count,
				sizeof(struct acpi_processor_px),
				GFP_KERNEL);

	if (!perf->states) {
		result = -ENOMEM;
		return result;
	}

	for (i = 0; i < perf->state_count; i++) {
		perf->states[i].power = 0x3A98;
		perf->states[i].transition_latency = 10000;
		perf->states[i].bus_master_latency = 10000;
		perf->states[i].status = (RESERVED_FREQ + i / (boost_gears + 1));
		perf->states[i].control = (RESERVED_FREQ + i / (boost_gears + 1));

		switch (i % (boost_gears + 1)) {
		case 0:
			perf->states[i].core_frequency = (cpu_clock_freq / 1000000) * (8 - i / (boost_gears + 1)) / 8;
			break;
		case 1:
		case 2:
		case 3:
		case 4:
			perf->states[i].core_frequency =
				boost_freqs[i % (boost_gears + 1)] * (8 - i / (boost_gears + 1)) / 8;
			perf->states[i].control |= ((i % (boost_gears + 1)) << 8);
			break;
		default:
			pr_info("%s: i %d freq table error\n", __func__, i);
		}
	}

	return result;
}

static int loongson3_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
	unsigned int i;
	struct acpi_processor_performance *perf;
	struct cpufreq_frequency_table *freq_table;
	struct core_data *core;
	int package_id;
	unsigned int cpu = policy->cpu;
	unsigned int result = 0;

	perf = per_cpu_ptr(acpi_perf_data, cpu);
	package_id = cpu_data[cpu].package;
	core = get_core_data(cpu);
	all_package_data[package_id].nr_cores = loongson_sysconf.cores_per_package;
	all_package_data[package_id].max_boost_cores = max_boost_cores;
	core->normal_max_freq = 0;
	all_package_data[package_id].nr_full_load_cores = 0;
	core->cpu = cpu;
	core->work_in_progress = false;
	core->last_freq_update_time = 0;
	core->perf = perf;
	core->package_id = package_id;
	core->package = &all_package_data[package_id];

	core->boost_freq = kmalloc_array(boost_gears + 1, sizeof(typeof(core->boost_freq)), GFP_KERNEL);
	core->clock_scale = kmalloc_array(boost_gears + 1, sizeof(typeof(core->clock_scale)), GFP_KERNEL);
	core->shift = kmalloc_array(boost_gears + 1, sizeof(typeof(core->shift)), GFP_KERNEL);

	for (i = 0; i < boost_gears + 1; i++) {
		core->boost_freq[i] = boost_freqs[i];
		core->shift[i] = 1;
	}

	if (!acpi_disabled)
		result = acpi_processor_register_performance(perf, cpu);
	else {
		result = init_acpi(perf);
		policy->shared_type = perf->shared_type;
	}

	if (result) {
		pr_info("CPU%d acpi_processor_register_performance failed.\n", cpu);
		return result;
	}

	for (i = 0; i < MAX_PACKAGES; i++)
		mutex_init(&boost_mutex[i]);

	/* capability check */
	if (perf->state_count <= 1) {
		pr_debug("No P-States\n");
		result = -ENODEV;
		goto err_unreg;
	}

	freq_table = kcalloc(perf->state_count + 1, sizeof(*freq_table),
			GFP_KERNEL);
	if (!freq_table) {
		result = -ENOMEM;
		goto err_unreg;
	}

	/* detect transition latency */
	policy->cpuinfo.transition_latency = 0;
	for (i = 0; i < perf->state_count; i++) {
		if ((perf->states[i].transition_latency * 1000) >
				policy->cpuinfo.transition_latency)
			policy->cpuinfo.transition_latency =
				perf->states[i].transition_latency * 1000;
		if (perf->states[i].control & LOONGSON_BOOST_FREQ_MASK) {
			set_boost_freq(true);
		} else {
			if (perf->states[i].core_frequency > core->normal_max_freq)
				core->normal_max_freq = perf->states[i].core_frequency;
		}
	}

	core->freq_update_delay_ns = policy->cpuinfo.transition_latency;

	for (i = 0; i < boost_gears + 1; i++) {
		core->clock_scale[i] = compute_scale(&core->shift[i], boost_freqs[i], core->normal_max_freq);
		pr_debug("%s: boost_freqs[%d] %d, normal_max_freq %d, scale %d, shift %d\n",
				__func__, i, boost_freqs[i], core->normal_max_freq,
				core->clock_scale[i], core->shift[i]);
	}

	/* table init */
	for (i = 0; i < perf->state_count; i++) {
		freq_table[i].driver_data = (perf->states[i].control & LOONGSON_BOOST_FREQ_MASK) >> 8;
		if (freq_table[i].driver_data)
			freq_table[i].flags |= CPUFREQ_BOOST_FREQ;
		freq_table[i].frequency =
			perf->states[i].core_frequency * 1000;
	}
	freq_table[i].frequency = CPUFREQ_TABLE_END;
	policy->freq_table = freq_table;
	perf->state = 0;

	/* add boost-attr if supported. */
	if (has_boost_freq() && boost_supported())
		loongson3_cpufreq_attr[1] = &cpufreq_freq_attr_scaling_boost_freqs;

	pr_info("CPU%u - ACPI performance management activated.\n", cpu);
	for (i = 0; i < perf->state_count; i++)
		pr_debug("     %cP%d: %d MHz, %d mW, %d uS %d level\n",
				(i == perf->state ? '*' : ' '), i,
				(u32) perf->states[i].core_frequency,
				(u32) perf->states[i].power,
				(u32) perf->states[i].transition_latency,
				(u32) perf->states[i].control);

	/*
	 * the first call to ->target() should result in us actually
	 * writing something to the appropriate registers.
	 */
	policy->fast_switch_possible = false;

	init_irq_work(&core->irq_work, cpufreq_irq_work);
	kthread_init_work(&core->work, cpufreq_work_handler);
	core->sampling_rate = max_t(unsigned int,
			CPUFREQ_SAMPLING_INTERVAL,
			cpufreq_policy_transition_delay_us(policy));
	return result;

err_unreg:
	if (!acpi_disabled)
		acpi_processor_unregister_performance(cpu);

	return result;
}

static int loongson3_cpufreq_cpu_exit(struct cpufreq_policy *policy)
{
	struct core_data *core = get_core_data(policy->cpu);

	clear_update_util_hook(policy->cpu);
	irq_work_sync(&core->irq_work);
	kthread_cancel_work_sync(&core->work);
	core->work_in_progress = false;
	policy->fast_switch_possible = false;
	if (!acpi_disabled)
		acpi_processor_unregister_performance(policy->cpu);
	kfree(policy->freq_table);
	kfree(core->boost_freq);
	kfree(core->clock_scale);
	kfree(core->shift);
	return 0;
}

static struct freq_attr *loongson3_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,   /* Extra space for boost-attr if supported */
	NULL,
};

static struct cpufreq_driver loongson3_cpufreq_driver = {
	.verify		= loongson3_cpufreq_verify_policy,
	.setpolicy	= loongson3_cpufreq_set_policy,
	.init		= loongson3_cpufreq_cpu_init,
	.exit		= loongson3_cpufreq_cpu_exit,
	.name		= "acpi-cpufreq",
	.attr		= loongson3_cpufreq_attr,
};

static void free_acpi_perf_data(void)
{
	unsigned int i;

	/* Freeing a NULL pointer is OK, and alloc_percpu zeroes. */
	for_each_possible_cpu(i)
		free_cpumask_var(per_cpu_ptr(acpi_perf_data, i)
				->shared_cpu_map);
	free_percpu(acpi_perf_data);
}

static int __init loongson3_cpufreq_early_init(void)
{
	unsigned int i;

	acpi_perf_data = alloc_percpu(struct acpi_processor_performance);
	if (!acpi_perf_data)
		return -ENOMEM;
	for_each_possible_cpu(i) {
		if (!zalloc_cpumask_var_node(
					&per_cpu_ptr(acpi_perf_data, i)->shared_cpu_map,
					GFP_KERNEL, cpu_to_node(i))) {
			free_acpi_perf_data();
			return -ENOMEM;
		}
	}
	return 0;
}

static bool support_boost(void)
{
	int message;
	int val;
	int i;

	if (wait_for_ready_timeout(MAX_READY_TIMEOUT))
		return false;
	message = DVFS_INFO << 24;
	iocsr_write32(message, 0x51c);
	val = iocsr_read32(0x420);

	val |= 1 << 10;
	iocsr_write32(val, 0x420);
	if (wait_for_ready_timeout(MAX_READY_TIMEOUT)) {
		pr_info("%s: not support boost\n", __func__);
		return false;
	}

	val = iocsr_read32(0x51c);

	min_freq_level = val & DVFS_INFO_MIN_FREQ;
	max_freq_level = (val & DVFS_INFO_MAX_FREQ) >> 4;

	if ((val & DVFS_INFO_BOOST_CORE_FREQ) && ((val & DVFS_INFO_BOOST_CORES) >> 20)) {
		max_boost_cores = (val & DVFS_INFO_BOOST_CORES) >> 20;
		max_boost_freq = ((val & DVFS_INFO_BOOST_CORE_FREQ) >> 8) * 25;
		max_upper_index = (val & DVFS_INFO_NORMAL_CORE_UPPER_LIMIT) >> 16;
	} else {
		boost_gears = 0;
		return false;
	}

	/* Read boost levels */
	if (wait_for_ready_timeout(MAX_READY_TIMEOUT))
		return false;

	/* for version 1, single boost freq boost */
	message = DVFS_INFO_BOOST_LEVEL << 24;
	iocsr_write32(message, 0x51c);
	val = iocsr_read32(0x420);

	val |= 1 << 10;
	iocsr_write32(val, 0x420);

	if (wait_for_ready_timeout(MAX_READY_TIMEOUT)) {
		pr_info("%s: single boost mode\n", __func__);
		boost_gears = 1;
		boost_freqs[0] = calc_const_freq() / 1000000;
		for (i = 1; i < boost_gears + 1; i++)
			boost_freqs[i] = max_boost_freq;

		/* set 0x51c complete */
		iocsr_write32(COMPLETE_STATUS, 0x51c);
	} else {
		pr_info("%s: multi boost mode\n", __func__);
		boost_gears = max_boost_cores;
		val = iocsr_read32(0x51c);

		boost_freqs[0] = calc_const_freq() / 1000000;
		boost_freqs[1] = max_boost_freq;

		if (boost_gears > 1) {
			for (i = 2; i < boost_gears + 1; i++)
				boost_freqs[i] = max_boost_freq - (((val >> ((i-2) * 4)) & 0xf) * FREQ_STEP);
		}
	}

	pr_info("%s: min_freq_level %d, max_freq_level %d, max_boost_cores %d, boost_gears %d\n",
			__func__, min_freq_level, max_freq_level, max_boost_cores, boost_gears);

	return true;
}

static int cpufreq_table_cpuinfo(struct cpufreq_policy *policy,
				struct cpufreq_frequency_table *table,
				bool boost)
{
	struct cpufreq_frequency_table *pos;
	unsigned int min_freq = ~0;
	unsigned int max_freq = 0;
	unsigned int freq;

	cpufreq_for_each_valid_entry(pos, table) {
		freq = pos->frequency;

		if (!boost) {
			if (pos->driver_data)
				continue;
		}
		if (freq < min_freq)
			min_freq = freq;
		if (freq > max_freq)
			max_freq = freq;
	}

	policy->min = policy->cpuinfo.min_freq = min_freq;
	policy->max = policy->cpuinfo.max_freq = max_freq;
	if (policy->min == ~0)
		return -EINVAL;
	else
		return 0;
}

static int set_boost(struct cpufreq_policy *policy, int state)
{
	if (!has_boost_freq())
		return -EINVAL;

	if (!policy)
		return -EINVAL;

	if (!state) {
		if (policy->policy == CPUFREQ_POLICY_POWERSAVE)
			cpufreq_update(policy);
	}
	if (!policy->freq_table)
		return -EINVAL;

	cpufreq_table_cpuinfo(policy, policy->freq_table, state);
	down_write(&policy->rwsem);
	up_write(&policy->rwsem);

	if (!state)
		set_max_within_limits(policy);

	return 0;
}

static void __init loongson3_cpufreq_boost_init(void)
{
	if (!support_boost()) {
		pr_info("Boost capabilities not present in the processor\n");
		return;
	}

	loongson3_cpufreq_driver.set_boost = set_boost;
}

static int cpufreq_supported_detect(void)
{
	return wait_for_ready_timeout(MAX_READY_TIMEOUT);
}

static int __init loongson3_cpufreq_init(void)
{
	int ret;

	if (!cpu_has_csr || !cpu_has_scalefreq)
		return -ENODEV;

	/* don't keep reloading if cpufreq_driver exists */
	if (cpufreq_get_current_driver())
		return -EEXIST;

	if (cpufreq_supported_detect()) {
		pr_info("%s failed!\n", __func__);
		return -ENODEV;
	}

	ret = loongson3_cpufreq_early_init();
	if (ret)
		return ret;
	loongson3_cpufreq_boost_init();

	cpufreq_register_notifier(&loongson3_cpufreq_notifier_block,
			CPUFREQ_TRANSITION_NOTIFIER);
	ret = cpufreq_register_driver(&loongson3_cpufreq_driver);
	cpufreq_kthread_create();
	if (ret)
		free_acpi_perf_data();

	return ret;
}

static void __exit loongson3_cpufreq_exit(void)
{
	cpufreq_unregister_driver(&loongson3_cpufreq_driver);
	free_acpi_perf_data();
	cpufreq_kthread_stop();
}

late_initcall(loongson3_cpufreq_init);
module_exit(loongson3_cpufreq_exit);

static const struct acpi_device_id processor_device_ids[] = {
	{ACPI_PROCESSOR_OBJECT_HID, },
	{ACPI_PROCESSOR_DEVICE_HID, },
	{},
};
MODULE_DEVICE_TABLE(acpi, processor_device_ids);

MODULE_ALIAS("acpi");
