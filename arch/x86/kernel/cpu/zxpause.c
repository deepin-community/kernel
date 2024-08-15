// SPDX-License-Identifier: GPL-2.0
#include <linux/syscore_ops.h>
#include <linux/suspend.h>
#include <linux/cpu.h>

#include <asm/msr.h>
#include <asm/mwait.h>

#define ZXPAUSE_C02_ENABLE	0

#define ZXPAUSE_CTRL_VAL(max_time, c02_disable)				\
	(((max_time) & MSR_ZX_PAUSE_CONTROL_TIME_MASK) |		\
	((c02_disable) & MSR_ZX_PAUSE_CONTROL_C02_DISABLE))

/*
 * Cache ZX_PAUSE_CONTROL MSR. This is a systemwide control. By default,
 * zxpause max time is 100000 in TSC-quanta and C0.2 is enabled
 */
static u32 zxpause_control_cached = ZXPAUSE_CTRL_VAL(100000, ZXPAUSE_C02_ENABLE);

/*
 * Cache the original ZX_PAUSE_CONTROL MSR value which is configured by
 * hardware or BIOS before kernel boot.
 */
static u32 orig_zxpause_control_cached __ro_after_init;

/*
 * Serialize access to zxpause_control_cached and ZX_PAUSE_CONTROL MSR in
 * the sysfs write functions.
 */
static DEFINE_MUTEX(zxpause_lock);

static void zxpause_update_control_msr(void * unused)
{
	lockdep_assert_irqs_disabled();
	wrmsr(MSR_ZX_PAUSE_CONTROL, READ_ONCE(zxpause_control_cached), 0);
}

/*
 * The CPU hotplug callback sets the control MSR to the global control
 * value.
 *
 * Disable interrupts so the read of zxpause_control_cached and the WRMSR
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
static int zxpause_cpu_online(unsigned int cpu)
{
	local_irq_disable();
	zxpause_update_control_msr(NULL);
	local_irq_enable();
	return 0;
}

/*
 * The CPU hotplug callback sets the control MSR to the original control
 * value.
 */
static int zxpause_cpu_offline(unsigned int cpu)
{
	/*
	 * This code is protected by the CPU hotplug already and
	 * orig_zxpause_control_cached is never changed after it caches
	 * the original control MSR value in zxpause_init(). So there
	 * is no race condition here.
	 */
	wrmsr(MSR_ZX_PAUSE_CONTROL, orig_zxpause_control_cached, 0);

	return 0;
}

/*
 * On resume, restore ZX_PAUSE_CONTROL MSR on the boot processor which
 * is the only active CPU at this time. The MSR is set up on the APs via the
 * CPU hotplug callback.
 *
 * This function is invoked on resume from suspend and hibernation. On
 * resume from suspend the restore should be not required, but we neither
 * trust the firmware nor does it matter if the same value is written
 * again.
 */
static void zxpause_syscore_resume(void)
{
	zxpause_update_control_msr(NULL);
}

static struct syscore_ops zxpause_syscore_ops = {
	.resume	= zxpause_syscore_resume,
};

/* sysfs interface */

/*
 * When bit 0 in ZX_PAUSE_CONTROL MSR is 1, C0.2 is disabled.
 * Otherwise, C0.2 is enabled.
 */
static inline bool zxpause_ctrl_c02_enabled(u32 ctrl)
{
	return !(ctrl & MSR_ZX_PAUSE_CONTROL_C02_DISABLE);
}

static inline u32 zxpause_ctrl_max_time(u32 ctrl)
{
	return ctrl & MSR_ZX_PAUSE_CONTROL_TIME_MASK;
}

static inline void zxpause_update_control(u32 maxtime, bool c02_enable)
{
	u32 ctrl = maxtime & MSR_ZX_PAUSE_CONTROL_TIME_MASK;

	if (!c02_enable)
		ctrl |= MSR_ZX_PAUSE_CONTROL_C02_DISABLE;

	WRITE_ONCE(zxpause_control_cached, ctrl);
	/* Propagate to all CPUs */
	on_each_cpu(zxpause_update_control_msr, NULL, 1);
}

static ssize_t
enable_c02_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u32 ctrl = READ_ONCE(zxpause_control_cached);

	return sprintf(buf, "%d\n", zxpause_ctrl_c02_enabled(ctrl));
}

static ssize_t enable_c02_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	bool c02_enable;
	u32 ctrl;
	int ret;

	ret = kstrtobool(buf, &c02_enable);
	if (ret)
		return ret;

	mutex_lock(&zxpause_lock);

	ctrl = READ_ONCE(zxpause_control_cached);
	if (c02_enable != zxpause_ctrl_c02_enabled(ctrl))
		zxpause_update_control(ctrl, c02_enable);

	mutex_unlock(&zxpause_lock);

	return count;
}
static DEVICE_ATTR_RW(enable_c02);

static ssize_t
max_time_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	u32 ctrl = READ_ONCE(zxpause_control_cached);

	return sprintf(buf, "%u\n", zxpause_ctrl_max_time(ctrl));
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
	if (max_time & ~MSR_ZX_PAUSE_CONTROL_TIME_MASK)
		return -EINVAL;

	mutex_lock(&zxpause_lock);

	ctrl = READ_ONCE(zxpause_control_cached);
	if (max_time != zxpause_ctrl_max_time(ctrl))
		zxpause_update_control(max_time, zxpause_ctrl_c02_enabled(ctrl));

	mutex_unlock(&zxpause_lock);

	return count;
}
static DEVICE_ATTR_RW(max_time);

static struct attribute *zxpause_attrs[] = {
	&dev_attr_enable_c02.attr,
	&dev_attr_max_time.attr,
	NULL
};

static struct attribute_group zxpause_attr_group = {
	.attrs = zxpause_attrs,
	.name = "zxpause_control",
};

static int __init zxpause_init(void)
{
	struct device *dev;
	int ret;

	if (!boot_cpu_has(X86_FEATURE_ZXPAUSE))
		return -ENODEV;

	/*
	 * Cache the original control MSR value before the control MSR is
	 * changed. This is the only place where orig_zxpause_control_cached
	 * is modified.
	 */
	rdmsrl(MSR_ZX_PAUSE_CONTROL, orig_zxpause_control_cached);

	ret = cpuhp_setup_state(CPUHP_AP_ONLINE_DYN, "zxpause:online",
				zxpause_cpu_online, zxpause_cpu_offline);
	if (ret < 0) {
		/*
		 * On failure, the control MSR on all CPUs has the
		 * original control value.
		 */
		return ret;
	}

	register_syscore_ops(&zxpause_syscore_ops);

	/*
	 * Add zxpause control interface. Ignore failure, so at least the
	 * default values are set up in case the machine manages to boot.
	 */
	dev = bus_get_dev_root(&cpu_subsys);
	return sysfs_create_group(&dev->kobj, &zxpause_attr_group);
}
device_initcall(zxpause_init);
