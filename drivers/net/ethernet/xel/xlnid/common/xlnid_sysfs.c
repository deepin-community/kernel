// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2008 - 2022 Xel Technology. */

#include "xlnid.h"
#include "xlnid_common.h"
#include "xlnid_type.h"

#ifdef XLNID_SYSFS

#include <linux/module.h>
#include <linux/types.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/device.h>
#include <linux/netdevice.h>
#include <linux/time.h>
#ifdef XLNID_HWMON
#include <linux/hwmon.h>
#endif

#ifdef XLNID_HWMON
/* hwmon callback functions */
static ssize_t xlnid_hwmon_show_location(struct device __always_unused *dev,
		struct device_attribute *attr, char *buf)
{
	struct hwmon_attr *xlnid_attr = container_of(attr, struct hwmon_attr,
									dev_attr);

	return sprintf(buf, "loc%u\n", xlnid_attr->sensor->location);
}

static ssize_t xlnid_hwmon_show_temp(struct device __always_unused *dev,
										struct device_attribute *attr, char *buf)
{
	struct hwmon_attr *xlnid_attr = container_of(attr, struct hwmon_attr,
									dev_attr);
	unsigned int value;

	/* reset the temp field */
	xlnid_attr->hw->mac.ops.get_thermal_sensor_data(xlnid_attr->hw);

	value = xlnid_attr->sensor->temp;

	/* display millidegree */
	value *= 1000;

	return sprintf(buf, "%u\n", value);
}

static ssize_t xlnid_hwmon_show_cautionthresh(struct device __always_unused
		*dev, struct device_attribute *attr, char *buf)
{
	struct hwmon_attr *xlnid_attr = container_of(attr, struct hwmon_attr,
									dev_attr);
	unsigned int value = xlnid_attr->sensor->caution_thresh;

	/* display millidegree */
	value *= 1000;

	return sprintf(buf, "%u\n", value);
}

static ssize_t xlnid_hwmon_show_maxopthresh(struct device __always_unused *dev,
		struct device_attribute *attr, char *buf)
{
	struct hwmon_attr *xlnid_attr = container_of(attr, struct hwmon_attr,
									dev_attr);
	unsigned int value = xlnid_attr->sensor->max_op_thresh;

	/* display millidegree */
	value *= 1000;

	return sprintf(buf, "%u\n", value);
}

/**
		xlnid_add_hwmon_attr - Create hwmon attr table for a hwmon sysfs file.
		@adapter: pointer to the adapter structure
		@offset: offset in the eeprom sensor data table
		@type: type of sensor data to display

		For each file we want in hwmon's sysfs interface we need a device_attribute
		This is included in our hwmon_attr struct that contains the references to
		the data structures we need to get the data to display.
*/
static int xlnid_add_hwmon_attr(struct xlnid_adapter *adapter,
								unsigned int offset, int type)
{
	unsigned int n_attr;
	struct hwmon_attr *xlnid_attr;

#ifdef HAVE_HWMON_DEVICE_REGISTER_WITH_GROUPS

	n_attr = adapter->xlnid_hwmon_buff->n_hwmon;
	xlnid_attr = &adapter->xlnid_hwmon_buff->hwmon_list[n_attr];
#else
	int rc;

	n_attr = adapter->xlnid_hwmon_buff.n_hwmon;
	xlnid_attr = &adapter->xlnid_hwmon_buff.hwmon_list[n_attr];
#endif /* HAVE_HWMON_DEVICE_REGISTER_WITH_GROUPS */

	switch (type) {
	case XLNID_HWMON_TYPE_LOC:
		xlnid_attr->dev_attr.show = xlnid_hwmon_show_location;
		snprintf(xlnid_attr->name, sizeof(xlnid_attr->name), "temp%u_label",
		offset + 1);
		break;

	case XLNID_HWMON_TYPE_TEMP:
		xlnid_attr->dev_attr.show = xlnid_hwmon_show_temp;
		snprintf(xlnid_attr->name, sizeof(xlnid_attr->name), "temp%u_input",
		offset + 1);
		break;

	case XLNID_HWMON_TYPE_CAUTION:
		xlnid_attr->dev_attr.show = xlnid_hwmon_show_cautionthresh;
		snprintf(xlnid_attr->name, sizeof(xlnid_attr->name), "temp%u_max", offset + 1);
		break;

	case XLNID_HWMON_TYPE_MAX:
		xlnid_attr->dev_attr.show = xlnid_hwmon_show_maxopthresh;
		snprintf(xlnid_attr->name, sizeof(xlnid_attr->name), "temp%u_crit", offset + 1);
		break;

	default:
		return -EPERM;
	}

	/* These always the same regardless of type */
	xlnid_attr->sensor = &adapter->hw.mac.thermal_sensor_data.sensor[offset];
	xlnid_attr->hw = &adapter->hw;
	xlnid_attr->dev_attr.store = NULL;
	xlnid_attr->dev_attr.attr.mode = 0444;
	xlnid_attr->dev_attr.attr.name = xlnid_attr->name;

#ifdef HAVE_HWMON_DEVICE_REGISTER_WITH_GROUPS
	sysfs_attr_init(&xlnid_attr->dev_attr.attr);

	adapter->xlnid_hwmon_buff->attrs[n_attr] = &xlnid_attr->dev_attr.attr;

	++adapter->xlnid_hwmon_buff->n_hwmon;

	return 0;
#else
	rc = device_create_file(pci_dev_to_dev(adapter->pdev), &xlnid_attr->dev_attr);

	if (rc == 0) {
		++adapter->xlnid_hwmon_buff.n_hwmon;
	}

	return rc;
#endif /* HAVE_HWMON_DEVICE_REGISTER_WITH_GROUPS */
}
#endif /* XLNID_HWMON */

static void xlnid_sysfs_del_adapter(struct xlnid_adapter __maybe_unused
									*adapter)
{
#ifdef XLNID_HWMON
#ifndef HAVE_HWMON_DEVICE_REGISTER_WITH_GROUPS
	int i;

	if (adapter == NULL) {
		return;
	}

	for (i = 0; i < adapter->xlnid_hwmon_buff.n_hwmon; i++) {
		device_remove_file(pci_dev_to_dev(adapter->pdev),
							&adapter->xlnid_hwmon_buff.hwmon_list[i].dev_attr);
	}

	kfree(adapter->xlnid_hwmon_buff.hwmon_list);

	if (adapter->xlnid_hwmon_buff.device) {
		hwmon_device_unregister(adapter->xlnid_hwmon_buff.device);
	}

#endif /* HAVE_HWMON_DEVICE_REGISTER_WITH_GROUPS */
#endif /* XLNID_HWMON */
}

/* called from xlnid_main.c */
void xlnid_sysfs_exit(struct xlnid_adapter *adapter)
{
	xlnid_sysfs_del_adapter(adapter);
}

/* called from xlnid_main.c */
int xlnid_sysfs_init(struct xlnid_adapter *adapter)
{
	int rc = 0;

#ifdef XLNID_HWMON
#ifdef HAVE_HWMON_DEVICE_REGISTER_WITH_GROUPS
	struct hwmon_buff *xlnid_hwmon;
	struct device *hwmon_dev;
#else
	struct hwmon_buff *xlnid_hwmon = &adapter->xlnid_hwmon_buff;
	int n_attrs;
#endif /* HAVE_HWMON_DEVICE_REGISTER_WITH_GROUPS */
	unsigned int i;
#endif /* XLNID_HWMON */

#ifdef XLNID_HWMON

	/* If this method isn't defined we don't support thermals */
	if (adapter->hw.mac.ops.init_thermal_sensor_thresh == NULL) {
		goto no_thermal;
	}

	/* Don't create thermal hwmon interface if no sensors present */
	if (adapter->hw.mac.ops.init_thermal_sensor_thresh(&adapter->hw)) {
		goto no_thermal;
	}

#ifdef HAVE_HWMON_DEVICE_REGISTER_WITH_GROUPS
	xlnid_hwmon = devm_kzalloc(&adapter->pdev->dev, sizeof(*xlnid_hwmon),
								GFP_KERNEL);

	if (!xlnid_hwmon) {
		rc = -ENOMEM;
		goto exit;
	}

	adapter->xlnid_hwmon_buff = xlnid_hwmon;
#else
	/*
		Allocation space for max attributs
		max num sensors * values (loc, temp, max, caution)
	*/
	n_attrs = XLNID_MAX_SENSORS * 4;
	xlnid_hwmon->hwmon_list = kcalloc(n_attrs, sizeof(struct hwmon_attr),
										GFP_KERNEL);

	if (!xlnid_hwmon->hwmon_list) {
		rc = -ENOMEM;
		goto err;
	}

#endif /* HAVE_HWMON_DEVICE_REGISTER_WITH_GROUPS */

	for (i = 0; i < XLNID_MAX_SENSORS; i++) {
		/*
			Only create hwmon sysfs entries for sensors that have
			meaningful data for.
		*/
		if (adapter->hw.mac.thermal_sensor_data.sensor[i].location == 0) {
			continue;
		}

		/* Bail if any hwmon attr struct fails to initialize */
		rc = xlnid_add_hwmon_attr(adapter, i, XLNID_HWMON_TYPE_CAUTION);

		if (rc) {
			goto err;
		}

		rc = xlnid_add_hwmon_attr(adapter, i, XLNID_HWMON_TYPE_LOC);

		if (rc) {
			goto err;
		}

		rc = xlnid_add_hwmon_attr(adapter, i, XLNID_HWMON_TYPE_TEMP);

		if (rc) {
			goto err;
		}

		rc = xlnid_add_hwmon_attr(adapter, i, XLNID_HWMON_TYPE_MAX);

		if (rc) {
			goto err;
		}
	}

#ifdef HAVE_HWMON_DEVICE_REGISTER_WITH_GROUPS
	xlnid_hwmon->groups[0] = &xlnid_hwmon->group;
	xlnid_hwmon->group.attrs = xlnid_hwmon->attrs;

	hwmon_dev = devm_hwmon_device_register_with_groups(&adapter->pdev->dev, "xlnid",
				xlnid_hwmon, xlnid_hwmon->groups);

	if (IS_ERR(hwmon_dev)) {
		rc = PTR_ERR(hwmon_dev);
		goto exit;
	}

#else
	xlnid_hwmon->device = hwmon_device_register(pci_dev_to_dev(adapter->pdev));

	if (IS_ERR(xlnid_hwmon->device)) {
		rc = PTR_ERR(xlnid_hwmon->device);
		goto err;
	}

#endif /* HAVE_HWMON_DEVICE_REGISTER_WITH_GROUPS */
no_thermal:
#endif /* XLNID_HWMON */
	goto exit;

err:
	xlnid_sysfs_del_adapter(adapter);
exit:
	return rc;
}
#endif /* XLNID_SYSFS */
