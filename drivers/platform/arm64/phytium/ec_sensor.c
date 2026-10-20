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
 * The attributes read the EC directly through it8528_read_byte(), so no
 * per-device driver data is needed and hwmon is registered without drvdata.
 * Values follow Documentation/hwmon/sysfs-interface.rst: temperatures are
 * reported in millidegrees Celsius and fans in RPM.
 */
static int ec_read_temp_millideg(u8 high_index, u8 low_index, int offset,
				 int *temp)
{
	u8 high, low;
	int ret;

	ret = it8528_read_byte(high_index, &high);
	if (ret)
		return ret;

	ret = it8528_read_byte(low_index, &low);
	if (ret)
		return ret;

	/* The EC reports tenths of a degree; offset is subtracted first. */
	*temp = (((high << 8) | low) - offset) * 100;
	return 0;
}

static int ec_read_fan_rpm(u8 high_index, u8 low_index, int *rpm)
{
	u8 high, low;
	int ret;

	ret = it8528_read_byte(high_index, &high);
	if (ret)
		return ret;

	ret = it8528_read_byte(low_index, &low);
	if (ret)
		return ret;

	*rpm = (high << 8) | low;
	return 0;
}

static ssize_t ec_bat_temp_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int temp, ret;

	/* The battery temperature is reported in tenths of a kelvin. */
	ret = ec_read_temp_millideg(INDEX_BATTERY_TEMP_HIGH,
				    INDEX_BATTERY_TEMP_LOW, 2730, &temp);
	if (ret)
		return ret;

	return sysfs_emit(buf, "%d\n", temp);
}

static ssize_t ec_gpu_temp_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	u8 value;
	int ret;

	ret = it8528_read_byte(INDEX_GPU_TEMP_VALUE, &value);
	if (ret)
		return ret;

	return sysfs_emit(buf, "%d\n", value * 1000);
}

static ssize_t ec_cpu_fan_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	int rpm, ret;

	ret = ec_read_fan_rpm(INDEX_CPU_FAN_SPEED_HIGH, INDEX_CPU_FAN_SPEED_LOW,
			      &rpm);
	if (ret)
		return ret;

	return sysfs_emit(buf, "%d\n", rpm);
}

static ssize_t ec_gpu_fan_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	int rpm, ret;

	ret = ec_read_fan_rpm(INDEX_GPU_FAN_SPEED_HIGH, INDEX_GPU_FAN_SPEED_LOW,
			      &rpm);
	if (ret)
		return ret;

	return sysfs_emit(buf, "%d\n", rpm);
}

static SENSOR_DEVICE_ATTR(temp1_input, 0444, ec_bat_temp_show, NULL, 0);
static SENSOR_DEVICE_ATTR(temp2_input, 0444, ec_gpu_temp_show, NULL, 0);
static SENSOR_DEVICE_ATTR(fan1_input, 0444, ec_cpu_fan_show, NULL, 0);
static SENSOR_DEVICE_ATTR(fan2_input, 0444, ec_gpu_fan_show, NULL, 0);

static struct attribute *ec_bat_temp_hwmon_attrs[] = {
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	NULL,
};
static struct attribute *ec_gpu_temp_hwmon_attrs[] = {
	&sensor_dev_attr_temp2_input.dev_attr.attr,
	NULL,
};
static struct attribute *ec_cpu_fan_hwmon_attrs[] = {
	&sensor_dev_attr_fan1_input.dev_attr.attr,
	NULL,
};
static struct attribute *ec_gpu_fan_hwmon_attrs[] = {
	&sensor_dev_attr_fan2_input.dev_attr.attr,
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
