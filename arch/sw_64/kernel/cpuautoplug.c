// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/sched.h>
#include <linux/tick.h>
#include <linux/kernel_stat.h>
#include <linux/platform_device.h>

#include <asm/cpufreq.h>
#include <asm/cputime.h>
#include <asm/smp.h>

int autoplug_enabled;
int autoplug_verbose;
int autoplug_adjusting;

DEFINE_PER_CPU(int, cpu_adjusting);

struct cpu_autoplug_info {
	cputime64_t prev_idle;
	cputime64_t prev_wall;
	struct delayed_work work;
	unsigned int sampling_rate;
	int maxcpus;   /* max cpus for autoplug */
	int mincpus;   /* min cpus for autoplug */
	int dec_reqs;  /* continuous core-decreasing requests */
	int inc_reqs;  /* continuous core-increasing requests */
};

struct cpu_autoplug_info ap_info;

static ssize_t enabled_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", autoplug_enabled);
}


static ssize_t enabled_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	char val[5];
	int n;

	memcpy(val, buf, count);
	n = kstrtol(val, 0, 0);

	if (n > 1 || n < 0)
		return -EINVAL;

	autoplug_enabled = n;

	return count;
}

static ssize_t verbose_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", autoplug_verbose);
}

static ssize_t verbose_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	char val[5];
	int n;

	memcpy(val, buf, count);
	n = kstrtol(val, 0, 0);

	if (n > 1 || n < 0)
		return -EINVAL;

	autoplug_verbose = n;

	return count;
}

static ssize_t maxcpus_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", ap_info.maxcpus);
}

static ssize_t maxcpus_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	char val[5];
	int n;

	memcpy(val, buf, count);
	n = kstrtol(val, 0, 0);

	if (n > num_possible_cpus() || n < ap_info.mincpus)
		return -EINVAL;

	ap_info.maxcpus = n;

	return count;
}

static ssize_t mincpus_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", ap_info.mincpus);
}

static ssize_t mincpus_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	char val[5];
	int n;

	memcpy(val, buf, count);
	n = kstrtol(val, 0, 0);

	if (n > ap_info.maxcpus || n < 1)
		return -EINVAL;

	ap_info.mincpus = n;

	return count;
}

static ssize_t sampling_rate_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", ap_info.sampling_rate);
}

#define SAMPLING_RATE_MAX 1000
#define SAMPLING_RATE_MIN 600

static ssize_t sampling_rate_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	char val[6];
	int n;

	memcpy(val, buf, count);
	n = kstrtol(val, 0, 0);

	if (n > SAMPLING_RATE_MAX || n < SAMPLING_RATE_MIN)
		return -EINVAL;

	ap_info.sampling_rate = n;

	return count;
}

static ssize_t available_value_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "enabled: 0-1\nverbose: 0-1\nmaxcpus:"
			"1-%d\nmincpus: 1-%d\nsampling_rate: %d-%d\n",
			num_possible_cpus(), num_possible_cpus(),
			SAMPLING_RATE_MIN, SAMPLING_RATE_MAX);
}

static DEVICE_ATTR_RW(enabled);
static DEVICE_ATTR_RW(verbose);
static DEVICE_ATTR_RW(maxcpus);
static DEVICE_ATTR_RW(mincpus);
static DEVICE_ATTR_RW(sampling_rate);
static DEVICE_ATTR_RO(available_value);

static struct attribute *cpuclass_default_attrs[] = {
	&dev_attr_enabled.attr,
	&dev_attr_verbose.attr,
	&dev_attr_maxcpus.attr,
	&dev_attr_mincpus.attr,
	&dev_attr_sampling_rate.attr,
	&dev_attr_available_value.attr,
	NULL
};

static struct attribute_group cpuclass_attr_group = {
	.attrs = cpuclass_default_attrs,
	.name = "cpuautoplug",
};

static int __init setup_autoplug(char *str)
{
	if (!strcmp(str, "off"))
		autoplug_enabled = 0;
	else if (!strcmp(str, "on"))
		autoplug_enabled = 1;
	else
		return 0;
	return 1;
}

__setup("autoplug=", setup_autoplug);

static cputime64_t calc_busy_time(unsigned int cpu)
{
	cputime64_t busy_time;

	busy_time = kcpustat_cpu(cpu).cpustat[CPUTIME_USER];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SYSTEM];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_IRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SOFTIRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_STEAL];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_NICE];
	busy_time += 1;

	return busy_time;
}

static inline cputime64_t sw64_get_idle_time_jiffy(cputime64_t *wall)
{
	unsigned int cpu;
	cputime64_t idle_time = 0;
	cputime64_t cur_wall_time;
	cputime64_t busy_time;

	cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());

	for_each_online_cpu(cpu) {
		busy_time = calc_busy_time(cpu);

		idle_time += cur_wall_time - busy_time;
	}

	if (wall)
		*wall = (cputime64_t)jiffies_to_usecs(cur_wall_time);

	return (cputime64_t)jiffies_to_usecs(idle_time);
}

static inline cputime64_t sw64_get_idle_time(cputime64_t *wall)
{
	unsigned int cpu;
	u64 idle_time = 0;

	for_each_online_cpu(cpu) {
		idle_time += get_cpu_idle_time_us(cpu, wall);
		if (idle_time == -1ULL)
			return sw64_get_idle_time_jiffy(wall);
	}

	return idle_time;
}

static cputime64_t get_min_busy_time(cputime64_t arr[], int size)
{
	int i, min_cpu_idx;
	cputime64_t min_time = arr[0];

	for (i = 0; i < size; i++) {
		if (arr[i] > 0 && arr[i] < min_time) {
			min_time = arr[i];
			min_cpu_idx = i;
		}
	}

	return min_cpu_idx;
}

static int find_min_busy_cpu(void)
{
	int nr_all_cpus = num_possible_cpus();
	unsigned int cpus, target_cpu;
	cputime64_t busy_time;
	cputime64_t b_time[NR_CPUS];

	memset(b_time, 0, sizeof(b_time));
	for_each_online_cpu(cpus) {
		busy_time = calc_busy_time(cpus);
		b_time[cpus] = busy_time;
	}
	target_cpu = get_min_busy_time(b_time, nr_all_cpus);
	return target_cpu;
}

static void increase_cores(int cur_cpus)
{
	struct device *dev;

	if (cur_cpus == ap_info.maxcpus)
		return;

	cur_cpus = cpumask_next_zero(0, cpu_online_mask);

	dev = get_cpu_device(cur_cpus);

	per_cpu(cpu_adjusting, dev->id) = 1;
	lock_device_hotplug();
	cpu_device_up(dev);
	pr_info("The target_cpu is %d, After cpu_up, the cpu_num is %d\n",
			dev->id, num_online_cpus());
	get_cpu_device(dev->id)->offline = false;
	unlock_device_hotplug();
	per_cpu(cpu_adjusting, dev->id) = 0;
}

static void decrease_cores(int cur_cpus)
{
	struct device *dev;

	if (cur_cpus == ap_info.mincpus)
		return;

	cur_cpus = find_min_busy_cpu();

	dev = get_cpu_device(cur_cpus);

	if (dev->id > 0) {
		per_cpu(cpu_adjusting, dev->id) = -1;
		lock_device_hotplug();
		cpu_device_down(dev);
		pr_info("The target_cpu is %d. After cpu_down, the cpu_num is %d\n",
				cur_cpus, num_online_cpus());
		get_cpu_device(dev->id)->offline = true;
		unlock_device_hotplug();
		per_cpu(cpu_adjusting, dev->id) = 0;
	}
}

#define INC_THRESHOLD 80
#define DEC_THRESHOLD 40

static void do_autoplug_timer(struct work_struct *work)
{
	cputime64_t cur_wall_time = 0, cur_idle_time;
	unsigned long idle_time, wall_time;
	int delay, load;
	int nr_cur_cpus = num_online_cpus();
	int nr_all_cpus = num_possible_cpus();
	int inc_req = 1, dec_req = 2;
	struct cpufreq_policy *policy = cpufreq_cpu_get_raw(smp_processor_id());

	if (!policy || IS_ERR(policy->clk)) {
		pr_err("%s: No %s associated to cpu: %d\n",
			__func__, policy ? "clk" : "policy", 0);
		return;
	}

	ap_info.maxcpus =
		setup_max_cpus > nr_cpu_ids ? nr_cpu_ids : setup_max_cpus;
	ap_info.mincpus = ap_info.maxcpus / 4;

	if (strcmp(policy->governor->name, "performance") == 0) {
		ap_info.mincpus = ap_info.maxcpus;
	} else if (strcmp(policy->governor->name, "powersave") == 0) {
		ap_info.maxcpus = ap_info.mincpus;
	} else if (strcmp(policy->governor->name, "ondemand") == 0) {
		ap_info.sampling_rate = 500;
		inc_req = 0;
		dec_req = 2;
	} else if (strcmp(policy->governor->name, "conservative") == 0) {
		inc_req = 1;
		dec_req = 3;
		ap_info.sampling_rate = 1000;  /* 1s */
	}

	BUG_ON(smp_processor_id() != 0);
	delay = msecs_to_jiffies(ap_info.sampling_rate);
	if (!autoplug_enabled || system_state != SYSTEM_RUNNING)
		goto out;

	autoplug_adjusting = 1;

	if (nr_cur_cpus > ap_info.maxcpus) {
		decrease_cores(nr_cur_cpus);
		autoplug_adjusting = 0;
		goto out;
	}
	if (nr_cur_cpus < ap_info.mincpus) {
		increase_cores(nr_cur_cpus);
		autoplug_adjusting = 0;
		goto out;
	}

	cur_idle_time = sw64_get_idle_time(&cur_wall_time);
	if (cur_wall_time == 0)
		cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());

	wall_time = (unsigned int)(cur_wall_time - ap_info.prev_wall);
	ap_info.prev_wall = cur_wall_time;

	idle_time = (unsigned int)(cur_idle_time - ap_info.prev_idle);
	idle_time += wall_time * (nr_all_cpus - nr_cur_cpus);
	ap_info.prev_wall = cur_idle_time;

	if (unlikely(!wall_time || wall_time * nr_all_cpus < idle_time)) {
		autoplug_adjusting = 0;
		goto out;
	}

	load = 100 * (wall_time * nr_all_cpus - idle_time) / wall_time;

	if (load < (nr_cur_cpus - 1) * 100 - DEC_THRESHOLD) {
		ap_info.inc_reqs = 0;
		if (ap_info.dec_reqs < dec_req)
			ap_info.dec_reqs++;
		else {
			ap_info.dec_reqs = 0;
			decrease_cores(nr_cur_cpus);
		}
	} else {
		ap_info.dec_reqs = 0;
		if (load > (nr_cur_cpus - 1) * 100 + INC_THRESHOLD) {
			if (ap_info.inc_reqs < inc_req)
				ap_info.inc_reqs++;
			else {
				ap_info.inc_reqs = 0;
				increase_cores(nr_cur_cpus);
			}
		}
	}

	autoplug_adjusting = 0;
out:
	schedule_delayed_work_on(0, &ap_info.work, delay);
}

static struct platform_device_id platform_device_ids[] = {
	{
		.name = "sw64_cpuautoplug",
	},
	{}
};

MODULE_DEVICE_TABLE(platform, platform_device_ids);

static struct platform_driver platform_driver = {
	.driver = {
		.name = "sw64_cpuautoplug",
		.owner = THIS_MODULE,
	},
	.id_table = platform_device_ids,
};

static int __init cpuautoplug_init(void)
{
	int i, ret, delay;
	struct device *dev_root;

	dev_root = bus_get_dev_root(&cpu_subsys);
	if (dev_root) {
		ret = sysfs_create_group(&dev_root->kobj,
					&cpuclass_attr_group);
		put_device(dev_root);
		if (ret)
			return ret;
	}

	ret = platform_driver_register(&platform_driver);
	if (ret)
		return ret;

	pr_info("cpuautoplug: SW64 CPU autoplug driver.\n");

	ap_info.maxcpus =
		setup_max_cpus > nr_cpu_ids ? nr_cpu_ids : setup_max_cpus;
	ap_info.mincpus = ap_info.maxcpus / 4;
	ap_info.dec_reqs = 0;
	ap_info.inc_reqs = 0;
	ap_info.sampling_rate = 720;  /* 720ms */
	if (setup_max_cpus == 0) {    /* boot with npsmp */
		ap_info.maxcpus = 1;
		autoplug_enabled = 0;
	}
	if (setup_max_cpus > num_possible_cpus())
		ap_info.maxcpus = num_possible_cpus();

	pr_info("mincpu = %d, maxcpu = %d, autoplug_enabled = %d, rate = %d\n",
			ap_info.mincpus, ap_info.maxcpus, autoplug_enabled,
			ap_info.sampling_rate);

	for_each_possible_cpu(i)
		per_cpu(cpu_adjusting, i) = 0;
	delay = msecs_to_jiffies(ap_info.sampling_rate * 24);
	INIT_DEFERRABLE_WORK(&ap_info.work, do_autoplug_timer);
	schedule_delayed_work_on(0, &ap_info.work, delay);

	if (!autoplug_enabled)
		cancel_delayed_work_sync(&ap_info.work);

	return ret;
}

late_initcall(cpuautoplug_init);
