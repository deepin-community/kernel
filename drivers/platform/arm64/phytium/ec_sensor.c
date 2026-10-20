// SPDX-License-Identifier: GPL-2.0
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/platform_device.h>
#include "ec_it8528.h"

/*----------------------------------------------------------------------*/

#if IS_REACHABLE(CONFIG_HWMON)

/*
 * Temperature and Fan support form ec devices.
 *
 * The attributes below read the EC directly through it8528_read(), so no
 * per-device driver data is needed and hwmon is registered without drvdata.
 */
static ssize_t ec_bat_temp_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int temp;

	temp = (it8528_read(INDEX_BATTERY_TEMP_HIGH) << 8) |
	       it8528_read(INDEX_BATTERY_TEMP_LOW);

	temp -= 2730;

	return sysfs_emit(buf, "%d.%d\n", temp / 10, temp % 10);
}

static ssize_t ec_gpu_temp_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	char temp;

	temp = it8528_read(INDEX_GPU_TEMP_VALUE);

	return sysfs_emit(buf, "%d\n", temp);
}

static ssize_t ec_cpu_fan_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	int speed;

	speed = (it8528_read(INDEX_CPU_FAN_SPEED_HIGH) << 8) |
		it8528_read(INDEX_CPU_FAN_SPEED_LOW);

	return sysfs_emit(buf, "%d\n", speed);
}

static ssize_t ec_gpu_fan_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	int speed;

	speed = (it8528_read(INDEX_GPU_FAN_SPEED_HIGH) << 8) |
		it8528_read(INDEX_GPU_FAN_SPEED_LOW);

	return sysfs_emit(buf, "%d\n", speed);
}

static SENSOR_DEVICE_ATTR(ec_bat_temp, 0444, ec_bat_temp_show,
			  NULL, 0);
static SENSOR_DEVICE_ATTR(ec_gpu_temp, 0444, ec_gpu_temp_show,
			  NULL, 0);
static SENSOR_DEVICE_ATTR(ec_cpu_fan, 0444, ec_cpu_fan_show,
			  NULL, 0);
static SENSOR_DEVICE_ATTR(ec_gpu_fan, 0444, ec_gpu_fan_show,
			  NULL, 0);

static struct attribute *ec_bat_temp_hwmon_attrs[] = {
	&sensor_dev_attr_ec_bat_temp.dev_attr.attr,
	NULL,
};
static struct attribute *ec_gpu_temp_hwmon_attrs[] = {
	&sensor_dev_attr_ec_gpu_temp.dev_attr.attr,
	NULL,
};
static struct attribute *ec_cpu_fan_hwmon_attrs[] = {
	&sensor_dev_attr_ec_cpu_fan.dev_attr.attr,
	NULL,
};
static struct attribute *ec_gpu_fan_hwmon_attrs[] = {
	&sensor_dev_attr_ec_gpu_fan.dev_attr.attr,
	NULL,
};

ATTRIBUTE_GROUPS(ec_bat_temp_hwmon);
ATTRIBUTE_GROUPS(ec_gpu_temp_hwmon);
ATTRIBUTE_GROUPS(ec_cpu_fan_hwmon);
ATTRIBUTE_GROUPS(ec_gpu_fan_hwmon);

static void ec_hwmon_register(struct platform_device *pdev, const char *name,
			      const struct attribute_group **groups)
{
	struct device *hwmon_dev;

	hwmon_dev = devm_hwmon_device_register_with_groups(&pdev->dev, name,
							   NULL, groups);
	if (IS_ERR(hwmon_dev))
		dev_err(&pdev->dev, "unable to register hwmon device %s: %ld\n",
			name, PTR_ERR(hwmon_dev));
}

void ec_sensors_register(struct platform_device *pdev)
{
	ec_hwmon_register(pdev, "ec_bat_temp", ec_bat_temp_hwmon_groups);
	ec_hwmon_register(pdev, "ec_gpu_temp", ec_gpu_temp_hwmon_groups);
	ec_hwmon_register(pdev, "ec_cpu_fan", ec_cpu_fan_hwmon_groups);
	ec_hwmon_register(pdev, "ec_gpu_fan", ec_gpu_fan_hwmon_groups);
}

#else

void ec_sensors_register(struct platform_device *pdev)
{
}

#endif /* IS_REACHABLE(CONFIG_HWMON) */
