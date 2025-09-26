// SPDX-License-Identifier: GPL-2.0
#include <linux/syscore_ops.h>
#include <linux/suspend.h>
#include <linux/cpu.h>

#include <asm/msr.h>
#include <asm/mwait.h>

#define PAUSEOPT_CTRL_VAL(max_time) (((max_time) & MSR_PAUSEOPT_CONTROL_TIME_MASK))

/*
 * Cache PAUSEOPT_CONTROL MSR. This is a systemwide control. By default,
 * pauseopt max time is 100000 in TSC-quanta and P0.1 is enabled.
 */
static u32 pauseopt_control_cached = PAUSEOPT_CTRL_VAL(100000);

/*
 * Cache the original PAUSEOPT_CONTROL MSR value which is configured by
 * hardware or BIOS before kernel boot.
 */
static u32 orig_pauseopt_control_cached __ro_after_init;

/*
 * Serialize access to pauseopt_control_cached and PAUSEOPT_CONTROL MSR in
 * the sysfs write functions.
 */
static DEFINE_MUTEX(pauseopt_lock);

static void pauseopt_update_control_msr(void *unused)
{
	lockdep_assert_irqs_disabled();
	wrmsr(MSR_PAUSEOPT_CONTROL, READ_ONCE(pauseopt_control_cached), 0);
}

/*
 * The CPU hotplug callback sets the control MSR to the global control
 * value.
 *
 * Disable interrupts so the read of pauseopt_control_cached and the WRMSR
 * are protected against a concurrent sysfs write. Otherwise the sysfs
 * write could update the cached value after it had been read on this CPU
 * and issue the IPI before the old value had been written. The IPI would
 * interrupt, write the new value and after return from IPI the previous
 * value would be written by this CPU.
 *
 * With interrupts disabled the upcoming CPU either sees the new control
 * value or the IPI is updating this CPU to the new control value after
 * interrupts have been reenabled.
 */
static int pauseopt_cpu_online(unsigned int cpu)
{
	local_irq_disable();
	pauseopt_update_control_msr(NULL);
	local_irq_enable();
	return 0;
}

/*
 * The CPU hotplug callback sets the control MSR to the original control
 * value.
 */
static int pauseopt_cpu_offline(unsigned int cpu)
{
	/*
	 * This code is protected by the CPU hotplug already and
	 * orig_pauseopt_control_cached is never changed after it caches
	 * the original control MSR value in pauseopt_init(). So there
	 * is no race condition here.
	 */
	wrmsr(MSR_PAUSEOPT_CONTROL, orig_pauseopt_control_cached, 0);

	return 0;
}

/*
 * On resume, restore PAUSEOPT_CONTROL MSR on the boot processor which
 * is the only active CPU at this time. The MSR is set up on the APs via the
 * CPU hotplug callback.
 *
 * This function is invoked on resume from suspend and hibernation. On
 * resume from suspend the restore should be not required, but we neither
 * trust the firmware nor does it matter if the same value is written
 * again.
 */
static void pauseopt_syscore_resume(void)
{
	pauseopt_update_control_msr(NULL);
}

static struct syscore_ops pauseopt_syscore_ops = {
	.resume	= pauseopt_syscore_resume,
};

/* sysfs interface */

static inline u32 pauseopt_ctrl_max_time(u32 ctrl)
{
	return ctrl & MSR_PAUSEOPT_CONTROL_TIME_MASK;
}

static inline void pauseopt_update_control(u32 maxtime)
{
	u32 ctrl = maxtime & MSR_PAUSEOPT_CONTROL_TIME_MASK;

	WRITE_ONCE(pauseopt_control_cached, ctrl);
	/* Propagate to all CPUs */
	on_each_cpu(pauseopt_update_control_msr, NULL, 1);
}

static ssize_t
enable_p01_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u32 ret;

	if (boot_cpu_has(X86_FEATURE_PAUSEOPT))
		ret = 1;
	else
		ret = 0;

	return sprintf(buf, "%d\n", ret);
}
static DEVICE_ATTR_RO(enable_p01);

static ssize_t
max_time_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	u32 ctrl = READ_ONCE(pauseopt_control_cached);

	return sprintf(buf, "%u\n", pauseopt_ctrl_max_time(ctrl));
}

static ssize_t max_time_store(struct device *kobj,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	u32 max_time, ctrl;
	int ret;

	ret = kstrtou32(buf, 0, &max_time);
	if (ret)
		return ret;

	/* bits[1:0] must be zero */
	if (max_time & ~MSR_PAUSEOPT_CONTROL_TIME_MASK)
		return -EINVAL;

	mutex_lock(&pauseopt_lock);

	ctrl = READ_ONCE(pauseopt_control_cached);
	if (max_time != pauseopt_ctrl_max_time(ctrl))
		pauseopt_update_control(max_time);

	mutex_unlock(&pauseopt_lock);

	return count;
}
static DEVICE_ATTR_RW(max_time);

static struct attribute *pauseopt_attrs[] = {
	&dev_attr_enable_p01.attr,
	&dev_attr_max_time.attr,
	NULL
};

static struct attribute_group pauseopt_attr_group = {
	.attrs = pauseopt_attrs,
	.name = "pauseopt_control",
};

static int __init pauseopt_init(void)
{
	struct device *dev;
	int ret;

	if (!boot_cpu_has(X86_FEATURE_PAUSEOPT))
		return -ENODEV;

	/*
	 * Cache the original control MSR value before the control MSR is
	 * changed. This is the only place where orig_pauseopt_control_cached
	 * is modified.
	 */
	rdmsrl(MSR_PAUSEOPT_CONTROL, orig_pauseopt_control_cached);

	ret = cpuhp_setup_state(CPUHP_AP_ONLINE_DYN, "pauseopt:online",
				pauseopt_cpu_online, pauseopt_cpu_offline);
	if (ret < 0) {
		/*
		 * On failure, the control MSR on all CPUs has the
		 * original control value.
		 */
		return ret;
	}

	register_syscore_ops(&pauseopt_syscore_ops);

	/*
	 * Add pauseopt control interface. Ignore failure, so at least the
	 * default values are set up in case the machine manages to boot.
	 */
	dev = bus_get_dev_root(&cpu_subsys);
	if (dev) {
		ret = sysfs_create_group(&dev->kobj, &pauseopt_attr_group);
		put_device(dev);
	}
	return ret;
}
device_initcall(pauseopt_init);
