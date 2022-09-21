#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

/*----------------------------------------------------------------------*/

#if IS_REACHABLE(CONFIG_HWMON)

/*
 * Temperature and Fan support form ec devices.
 */
static ssize_t ec_bat_temp_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	int temp = 0;

	temp = (it8528_read(INDEX_BATTERY_TEMP_HIGH) << 8) | it8528_read(INDEX_BATTERY_TEMP_LOW);

	temp = temp - 2730;

	return sprintf(buf, "%d.%d\n", temp/10, temp%10);
}

static ssize_t ec_gpu_temp_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	char temp = 0;

	temp = it8528_read(INDEX_GPU_TEMP_VALUE);

	return sprintf(buf, "%d\n", temp);
}

static ssize_t ec_cpu_fan_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	int speed = 0;

	speed = (it8528_read(INDEX_CPU_FAN_SPEED_HIGH) << 8) | it8528_read(INDEX_CPU_FAN_SPEED_LOW);

	return sprintf(buf, "%d\n", speed);
}

static ssize_t ec_gpu_fan_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	int speed = 0;

	speed = (it8528_read(INDEX_GPU_FAN_SPEED_HIGH) << 8) | it8528_read(INDEX_GPU_FAN_SPEED_LOW);

	return sprintf(buf, "%d\n", speed);
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

static void ec_sensors_register(struct platform_device * pdev)
{
	struct device *hwmon_dev;

    char * bat_temp_data, gpu_temp_data,cpu_fan_data,gpu_fan_data;

    /* register bat temp hwmon device */
	bat_temp_data = kzalloc(0x10, GFP_KERNEL);

	hwmon_dev = devm_hwmon_device_register_with_groups(&pdev->dev, "EC_BAT_TEMP",
						     bat_temp_data,
						     ec_bat_temp_hwmon_groups);
	if (IS_ERR(hwmon_dev)) {
		dev_err(&pdev->dev, "unable to register hwmon device %ld\n",
			 PTR_ERR(hwmon_dev));
	}

    /* register gpu temp hwmon device */
	gpu_temp_data = kzalloc(0x10, GFP_KERNEL);

	hwmon_dev = devm_hwmon_device_register_with_groups(&pdev->dev, "EC_GPU_TEMP",
						     gpu_temp_data,
						     ec_gpu_temp_hwmon_groups);
	if (IS_ERR(hwmon_dev)) {
		dev_err(&pdev->dev, "unable to register hwmon device %ld\n",
			 PTR_ERR(hwmon_dev));
	}

    /* register cpu fan hwmon device */
	cpu_fan_data = kzalloc(0x10, GFP_KERNEL);

	hwmon_dev = devm_hwmon_device_register_with_groups(&pdev->dev, "EC_CPU_FAN",
						     cpu_fan_data,
						     ec_cpu_fan_hwmon_groups);
	if (IS_ERR(hwmon_dev)) {
		dev_err(&pdev->dev, "unable to register hwmon device %ld\n",
			 PTR_ERR(hwmon_dev));
	}

    /* register cpu fan hwmon device */
	gpu_fan_data = kzalloc(0x10, GFP_KERNEL);

	hwmon_dev = devm_hwmon_device_register_with_groups(&pdev->dev, "EC_GPU_FAN",
						     gpu_fan_data,
						     ec_gpu_fan_hwmon_groups);
	if (IS_ERR(hwmon_dev)) {
		dev_err(&pdev->dev, "unable to register hwmon device %ld\n",
			 PTR_ERR(hwmon_dev));
	}

}

#else

static void ec_sensors_register(struct platform_device * pdev)
{
	return;
}

#endif /* IS_REACHABLE(CONFIG_HWMON) */
