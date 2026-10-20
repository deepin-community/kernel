// SPDX-License-Identifier: GPL-2.0
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/platform_device.h>
#include "ec_it8528.h"

/*----------------------------------------------------------------------*/

#if IS_REACHABLE(CONFIG_HWMON)

/*
 * Temperature and fan support for EC devices.
 *
 * All sensors live on a single hwmon device so the standard tempN_input and
 * fanN_input numbering stays contiguous and libsensors can discover them.
 * Values follow Documentation/hwmon/sysfs-interface.rst: temperatures are
 * reported in millidegrees Celsius and fans in RPM.
 */
static ssize_t ec_bat_temp_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	u16 raw;
	int ret;

	/* The battery temperature is reported in tenths of a kelvin. */
	ret = it8528_read_word(INDEX_BATTERY_TEMP_HIGH, INDEX_BATTERY_TEMP_LOW,
			       &raw);
	if (ret)
		return ret;

	return sysfs_emit(buf, "%d\n", ((int)raw - 2730) * 100);
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
	u16 raw;
	int ret;

	ret = it8528_read_word(INDEX_CPU_FAN_SPEED_HIGH, INDEX_CPU_FAN_SPEED_LOW,
			       &raw);
	if (ret)
		return ret;

	return sysfs_emit(buf, "%d\n", raw);
}

static ssize_t ec_gpu_fan_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	u16 raw;
	int ret;

	ret = it8528_read_word(INDEX_GPU_FAN_SPEED_HIGH, INDEX_GPU_FAN_SPEED_LOW,
			       &raw);
	if (ret)
		return ret;

	return sysfs_emit(buf, "%d\n", raw);
}

static SENSOR_DEVICE_ATTR(temp1_input, 0444, ec_bat_temp_show, NULL, 0);
static SENSOR_DEVICE_ATTR(temp2_input, 0444, ec_gpu_temp_show, NULL, 0);
static SENSOR_DEVICE_ATTR(fan1_input, 0444, ec_cpu_fan_show, NULL, 0);
static SENSOR_DEVICE_ATTR(fan2_input, 0444, ec_gpu_fan_show, NULL, 0);

static struct attribute *ec_hwmon_attrs[] = {
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_temp2_input.dev_attr.attr,
	&sensor_dev_attr_fan1_input.dev_attr.attr,
	&sensor_dev_attr_fan2_input.dev_attr.attr,
	NULL,
};
ATTRIBUTE_GROUPS(ec_hwmon);

void ec_sensors_register(struct platform_device *pdev)
{
	struct device *hwmon_dev;

	hwmon_dev = devm_hwmon_device_register_with_groups(&pdev->dev,
							   "phytium_ec", NULL,
							   ec_hwmon_groups);
	if (IS_ERR(hwmon_dev))
		dev_err(&pdev->dev, "unable to register hwmon device: %ld\n",
			PTR_ERR(hwmon_dev));
}

#else

void ec_sensors_register(struct platform_device *pdev)
{
}

#endif /* IS_REACHABLE(CONFIG_HWMON) */
