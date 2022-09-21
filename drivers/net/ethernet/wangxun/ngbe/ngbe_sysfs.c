/*
 * WangXun Gigabit PCI Express Linux driver
 * Copyright (c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 */


#include "ngbe.h"
#include "ngbe_hw.h"
#include "ngbe_type.h"

#ifdef NGBE_SYSFS

#include <linux/module.h>
#include <linux/types.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/device.h>
#include <linux/netdevice.h>
#include <linux/time.h>
#ifdef NGBE_HWMON
#include <linux/hwmon.h>
#endif

#ifdef NGBE_HWMON
/* hwmon callback functions */
static ssize_t ngbe_hwmon_show_temp(struct device __always_unused *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct hwmon_attr *ngbe_attr = container_of(attr, struct hwmon_attr,
						     dev_attr);
	unsigned int value;

	/* reset the temp field */
	TCALL(ngbe_attr->hw, mac.ops.get_thermal_sensor_data);

	value = ngbe_attr->sensor->temp;

	/* display millidegree */
	value *= 1000;

	return sprintf(buf, "%u\n", value);
}

static ssize_t ngbe_hwmon_show_alarmthresh(struct device __always_unused *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct hwmon_attr *ngbe_attr = container_of(attr, struct hwmon_attr,
						     dev_attr);
	unsigned int value = ngbe_attr->sensor->alarm_thresh;

	/* display millidegree */
	value *= 1000;

	return sprintf(buf, "%u\n", value);
}

static ssize_t ngbe_hwmon_show_dalarmthresh(struct device __always_unused *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct hwmon_attr *ngbe_attr = container_of(attr, struct hwmon_attr,
						     dev_attr);
	unsigned int value = ngbe_attr->sensor->dalarm_thresh;

	/* display millidegree */
	value *= 1000;

	return sprintf(buf, "%u\n", value);
}

/**
 * ngbe_add_hwmon_attr - Create hwmon attr table for a hwmon sysfs file.
 * @adapter: pointer to the adapter structure
 * @type: type of sensor data to display
 *
 * For each file we want in hwmon's sysfs interface we need a device_attribute
 * This is included in our hwmon_attr struct that contains the references to
 * the data structures we need to get the data to display.
 */
static int ngbe_add_hwmon_attr(struct ngbe_adapter *adapter, int type)
{
	int rc;
	unsigned int n_attr;
	struct hwmon_attr *ngbe_attr;

	n_attr = adapter->ngbe_hwmon_buff.n_hwmon;
	ngbe_attr = &adapter->ngbe_hwmon_buff.hwmon_list[n_attr];

	switch (type) {
	case NGBE_HWMON_TYPE_TEMP:
		ngbe_attr->dev_attr.show = ngbe_hwmon_show_temp;
		snprintf(ngbe_attr->name, sizeof(ngbe_attr->name),
			 "temp%u_input", 0);
		break;
	case NGBE_HWMON_TYPE_ALARMTHRESH:
		ngbe_attr->dev_attr.show = ngbe_hwmon_show_alarmthresh;
		snprintf(ngbe_attr->name, sizeof(ngbe_attr->name),
			 "temp%u_alarmthresh", 0);
		break;
	case NGBE_HWMON_TYPE_DALARMTHRESH:
		ngbe_attr->dev_attr.show = ngbe_hwmon_show_dalarmthresh;
		snprintf(ngbe_attr->name, sizeof(ngbe_attr->name),
			 "temp%u_dalarmthresh", 0);
		break;
	default:
		rc = -EPERM;
		return rc;
	}

	/* These always the same regardless of type */
	ngbe_attr->sensor =
		&adapter->hw.mac.thermal_sensor_data.sensor;
	ngbe_attr->hw = &adapter->hw;
	ngbe_attr->dev_attr.store = NULL;
	ngbe_attr->dev_attr.attr.mode = S_IRUGO;
	ngbe_attr->dev_attr.attr.name = ngbe_attr->name;

	rc = device_create_file(pci_dev_to_dev(adapter->pdev),
				&ngbe_attr->dev_attr);

	if (rc == 0)
		++adapter->ngbe_hwmon_buff.n_hwmon;

	return rc;
}
#endif /* NGBE_HWMON */

static void ngbe_sysfs_del_adapter(
				struct ngbe_adapter __maybe_unused *adapter)
{
#ifdef NGBE_HWMON
	int i;

	if (adapter == NULL)
		return;

	for (i = 0; i < adapter->ngbe_hwmon_buff.n_hwmon; i++) {
		device_remove_file(pci_dev_to_dev(adapter->pdev),
			   &adapter->ngbe_hwmon_buff.hwmon_list[i].dev_attr);
	}

	kfree(adapter->ngbe_hwmon_buff.hwmon_list);

	if (adapter->ngbe_hwmon_buff.device)
		hwmon_device_unregister(adapter->ngbe_hwmon_buff.device);
#endif /* NGBE_HWMON */
}

/* called from ngbe_main.c */
void ngbe_sysfs_exit(struct ngbe_adapter *adapter)
{
	ngbe_sysfs_del_adapter(adapter);
}

/* called from ngbe_main.c */
int ngbe_sysfs_init(struct ngbe_adapter *adapter)
{
	int rc = 0;
#ifdef NGBE_HWMON
	struct hwmon_buff *ngbe_hwmon = &adapter->ngbe_hwmon_buff;
	int n_attrs;

#endif /* NGBE_HWMON */
	if (adapter == NULL)
		goto err;

#ifdef NGBE_HWMON

	/* Don't create thermal hwmon interface if no sensors present */
	if (TCALL(&adapter->hw, mac.ops.init_thermal_sensor_thresh))
		goto no_thermal;

	/*
	 * Allocation space for max attributs
	 * max num sensors * values (temp, alamthresh, dalarmthresh)
	 */
	n_attrs = 3;
	ngbe_hwmon->hwmon_list = kcalloc(n_attrs, sizeof(struct hwmon_attr),
					  GFP_KERNEL);
	if (!ngbe_hwmon->hwmon_list) {
		rc = -ENOMEM;
		goto err;
	}

	ngbe_hwmon->device =
			hwmon_device_register(pci_dev_to_dev(adapter->pdev));
	if (IS_ERR(ngbe_hwmon->device)) {
		rc = PTR_ERR(ngbe_hwmon->device);
		goto err;
	}


	/* Bail if any hwmon attr struct fails to initialize */
	rc = ngbe_add_hwmon_attr(adapter, NGBE_HWMON_TYPE_TEMP);
	rc |= ngbe_add_hwmon_attr(adapter, NGBE_HWMON_TYPE_ALARMTHRESH);
	rc |= ngbe_add_hwmon_attr(adapter, NGBE_HWMON_TYPE_DALARMTHRESH);
	if (rc)
		goto err;

no_thermal:
#endif /* NGBE_HWMON */
	goto exit;

err:
	ngbe_sysfs_del_adapter(adapter);
exit:
	return rc;
}
#endif /* NGBE_SYSFS */
