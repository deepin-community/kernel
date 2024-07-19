// SPDX-License-Identifier: GPL-2.0-only
/*
 * zhaoxin-cputemp.c - Driver for Zhaoxin CPU core temperature monitoring
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/hwmon.h>
#include <linux/sysfs.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <linux/cpu.h>
#include <asm/msr.h>
#include <asm/processor.h>
#include <asm/cpu_device_id.h>

#define DRVNAME "zhaoxin_cputemp"

enum { SHOW_TEMP, SHOW_LABEL, SHOW_NAME, SHOW_CRIT, SHOW_MAX };

/* Functions declaration */

struct zhaoxin_cputemp_data {
	struct device *hwmon_dev;
	const char *name;
	u32 id;
	u32 msr_temp;
	u32 msr_crit;
	u32 msr_max;
	u32 msr_index;
};

/* Sysfs stuff */

static ssize_t name_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int ret;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct zhaoxin_cputemp_data *data = dev_get_drvdata(dev);

	if (attr->index == SHOW_NAME)
		ret = sprintf(buf, "%s\n", data->name);
	else    /* show label */
		ret = sprintf(buf, "Core %d\n", data->id);
	return ret;
}

static int temperature_access(struct zhaoxin_cputemp_data *data, u32 *l, u32 *h);
static ssize_t temp_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct zhaoxin_cputemp_data *data = dev_get_drvdata(dev);
	u32 eax, edx;
	int err;

	err = rdmsr_safe_on_cpu(data->id, data->msr_temp, &eax, &edx);
	if (err)
		return -EAGAIN;

	return sprintf(buf, "%lu\n", ((unsigned long)eax & 0xffffff) * 1000);
}

static ssize_t crit_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct zhaoxin_cputemp_data *data = dev_get_drvdata(dev);
	u32 eax, edx;
	int err;

	err = rdmsr_safe_on_cpu(data->id, data->msr_crit, &eax, &edx);
	if (err)
		return -EAGAIN;

	return sprintf(buf, "%lu\n", ((unsigned long)eax & 0xff) * 1000);
}

static ssize_t max_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct zhaoxin_cputemp_data *data = dev_get_drvdata(dev);
	u32 eax, edx;
	int err;

	err = rdmsr_safe_on_cpu(data->id, data->msr_max, &eax, &edx);
	if (err)
		return -EAGAIN;

	return sprintf(buf, "%lu\n", ((unsigned long)eax & 0xff) * 1000);
}

static SENSOR_DEVICE_ATTR_RO(temp1_input, temp, SHOW_TEMP);
static SENSOR_DEVICE_ATTR_RO(temp1_label, name, SHOW_LABEL);
static SENSOR_DEVICE_ATTR_RO(name, name, SHOW_NAME);
static SENSOR_DEVICE_ATTR_RO(temp1_crit, crit, SHOW_CRIT);
static SENSOR_DEVICE_ATTR_RO(temp1_max, max, SHOW_MAX);

static struct attribute *zhaoxin_cputemp_attributes[] = {
	&sensor_dev_attr_name.dev_attr.attr,
	&sensor_dev_attr_temp1_label.dev_attr.attr,
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_temp1_crit.dev_attr.attr,
	&sensor_dev_attr_temp1_max.dev_attr.attr,
	NULL
};

static const struct attribute_group zhaoxin_cputemp_group = {
	.attrs = zhaoxin_cputemp_attributes,
};

static int temperature_access(struct zhaoxin_cputemp_data *data, u32 *l, u32 *h)
{
	int err;
	*l = 0x19;

	if (data->msr_index) {
		err = wrmsr_safe_on_cpu(data->id, data->msr_index, *l, *h);
		if (err) {
			pr_info("Unable to write TEMPERATURE INDEX MSR\n");
			return err;
		}
	}

	*l = 0;
	err = rdmsr_safe_on_cpu(data->id, data->msr_temp, l, h);
	if (err) {
		pr_info("Unable to read TEMPERATURE DATA MSR\n");
		return err;
	}
	return 0;
}

static int zhaoxin_cputemp_probe(struct platform_device *pdev)
{
	struct zhaoxin_cputemp_data *data;
	int err;
	u32 eax, edx;
	struct cpuinfo_x86 *c = &cpu_data(pdev->id);

	data = devm_kzalloc(&pdev->dev, sizeof(struct zhaoxin_cputemp_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->id = pdev->id;
	data->name = "zhaoxin_cputemp";
	if (c->x86_model == 0x6b) {
		data->msr_index = 0x174c;
		data->msr_temp  = 0x174d;
		data->msr_crit  = 0x175b;
		data->msr_max   = 0x175a;
	} else {
		data->msr_index = 0;
		data->msr_temp = 0x1423;
		data->msr_crit = 0x1416;
		data->msr_max = 0x1415;
	}

	/* test if we can access the TEMPERATURE MSR */
	err = temperature_access(data, &eax, &edx);
	if (err) {
		dev_err(&pdev->dev, "Unable to access TEMPERATURE MSR, giving up\n");
		return err;
	}

	platform_set_drvdata(pdev, data);

	err = sysfs_create_group(&pdev->dev.kobj, &zhaoxin_cputemp_group);
	if (err)
		return err;

	data->hwmon_dev = hwmon_device_register_for_thermal(&pdev->dev, data->name, data);
	if (IS_ERR(data->hwmon_dev)) {
		err = PTR_ERR(data->hwmon_dev);
		dev_err(&pdev->dev, "Class registration failed (%d)\n", err);
		goto exit_remove;
	}

	return 0;

exit_remove:
	sysfs_remove_group(&pdev->dev.kobj, &zhaoxin_cputemp_group);
	return err;
}

static int zhaoxin_cputemp_remove(struct platform_device *pdev)
{
	struct zhaoxin_cputemp_data *data = platform_get_drvdata(pdev);

	hwmon_device_unregister(data->hwmon_dev);
	sysfs_remove_group(&pdev->dev.kobj, &zhaoxin_cputemp_group);
	return 0;
}

static struct platform_driver zhaoxin_cputemp_driver = {
	.driver = {
		.name = DRVNAME,
	},
	.probe = zhaoxin_cputemp_probe,
	.remove = zhaoxin_cputemp_remove,
};

struct pdev_entry {
	struct list_head list;
	struct platform_device *pdev;
	unsigned int cpu;
};

static LIST_HEAD(pdev_list);
static DEFINE_MUTEX(pdev_list_mutex);

static int zhaoxin_cputemp_online(unsigned int cpu)
{
	int err;
	struct platform_device *pdev;
	struct pdev_entry *pdev_entry;

	pdev = platform_device_alloc(DRVNAME, cpu);
	if (!pdev) {
		err = -ENOMEM;
		pr_err("Device allocation failed\n");
		goto exit;
	}

	pdev_entry = kzalloc(sizeof(struct pdev_entry), GFP_KERNEL);
	if (!pdev_entry) {
		err = -ENOMEM;
		goto exit_device_put;
	}

	err = platform_device_add(pdev);
	if (err) {
		pr_err("Device addition failed (%d)\n", err);
		goto exit_device_free;
	}

	pdev_entry->pdev = pdev;
	pdev_entry->cpu = cpu;
	mutex_lock(&pdev_list_mutex);
	list_add_tail(&pdev_entry->list, &pdev_list);
	mutex_unlock(&pdev_list_mutex);

	return 0;

exit_device_free:
	kfree(pdev_entry);
exit_device_put:
	platform_device_put(pdev);
exit:
	return err;
}

static int zhaoxin_cputemp_down_prep(unsigned int cpu)
{
	struct pdev_entry *p;

	mutex_lock(&pdev_list_mutex);
	list_for_each_entry(p, &pdev_list, list) {
		if (p->cpu == cpu) {
			platform_device_unregister(p->pdev);
			list_del(&p->list);
			mutex_unlock(&pdev_list_mutex);
			kfree(p);
			return 0;
		}
	}
	mutex_unlock(&pdev_list_mutex);
	return 0;
}

static const struct x86_cpu_id __initconst cputemp_ids[] = {
	X86_MATCH_VENDOR_FAM_MODEL(CENTAUR, 7, 0x3b, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(ZHAOXIN, 7, 0x3b, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(CENTAUR, 7, 0x5b, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(ZHAOXIN, 7, 0x5b, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(CENTAUR, 7, 0x6b, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(ZHAOXIN, 7, 0x6b, NULL),
	{}
};
MODULE_DEVICE_TABLE(x86cpu, cputemp_ids);

static enum cpuhp_state zhaoxin_temp_online;

static int __init zhaoxin_cputemp_init(void)
{
	int err;

	if (!x86_match_cpu(cputemp_ids))
		return -ENODEV;

	err = platform_driver_register(&zhaoxin_cputemp_driver);
	if (err)
		goto exit;

	err = cpuhp_setup_state(CPUHP_AP_ONLINE_DYN, "hwmon/zhaoxin:online",
			zhaoxin_cputemp_online, zhaoxin_cputemp_down_prep);
	if (err < 0)
		goto exit_driver_unreg;

	zhaoxin_temp_online = err;

#ifndef CONFIG_HOTPLUG_CPU
	if (list_empty(&pdev_list)) {
		err = -ENODEV;
		goto exit_hp_unreg;
	}
#endif
	return 0;

#ifndef CONFIG_HOTPLUG_CPU
exit_hp_unreg:
	cpuhp_remove_state_nocalls(zhaoxin_temp_online);
#endif
exit_driver_unreg:
	platform_driver_unregister(&zhaoxin_cputemp_driver);
exit:
	return err;
}

static void __exit zhaoxin_cputemp_exit(void)
{
	cpuhp_remove_state(zhaoxin_temp_online);
	platform_driver_unregister(&zhaoxin_cputemp_driver);
}

MODULE_DESCRIPTION("Zhaoxin CPU temperature monitor");
MODULE_LICENSE("GPL");

module_init(zhaoxin_cputemp_init)
module_exit(zhaoxin_cputemp_exit)

MODULE_IMPORT_NS(HWMON_THERMAL);