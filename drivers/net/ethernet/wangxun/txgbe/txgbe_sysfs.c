// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2019 - 2022 Beijing WangXun Technology Co., Ltd. */

#include "txgbe.h"
#include "txgbe_hw.h"
#include "txgbe_type.h"

#include <linux/module.h>
#include <linux/types.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/device.h>
#include <linux/netdevice.h>
#include <linux/time.h>
#include <linux/hwmon.h>

/* hwmon callback functions */
static ssize_t txgbe_hwmon_show_temp(struct device __always_unused *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct hwmon_attr *txgbe_attr = container_of(attr, struct hwmon_attr,
						     dev_attr);
	unsigned int value;
	struct txgbe_hw *hw = txgbe_attr->hw;

	/* reset the temp field */
	hw->mac.ops.get_thermal_sensor_data(hw);

	value = txgbe_attr->sensor->temp;

	/* display millidegree */
	value *= 1000;

	return sprintf(buf, "%u\n", value);
}

static ssize_t txgbe_hwmon_show_alarmthresh(struct device __always_unused *dev,
					    struct device_attribute *attr,
				     char *buf)
{
	struct hwmon_attr *txgbe_attr = container_of(attr, struct hwmon_attr,
						     dev_attr);
	unsigned int value = txgbe_attr->sensor->alarm_thresh;

	/* display millidegree */
	value *= 1000;

	return sprintf(buf, "%u\n", value);
}

static ssize_t txgbe_hwmon_show_dalarmthresh(struct device __always_unused *dev,
					     struct device_attribute *attr,
				     char *buf)
{
	struct hwmon_attr *txgbe_attr = container_of(attr, struct hwmon_attr,
						     dev_attr);
	unsigned int value = txgbe_attr->sensor->dalarm_thresh;

	/* display millidegree */
	value *= 1000;

	return sprintf(buf, "%u\n", value);
}

/**
 * txgbe_add_hwmon_attr - Create hwmon attr table for a hwmon sysfs file.
 * @adapter: pointer to the adapter structure
 * @type: type of sensor data to display
 *
 * For each file we want in hwmon's sysfs interface we need a device_attribute
 * This is included in our hwmon_attr struct that contains the references to
 * the data structures we need to get the data to display.
 */
static int txgbe_add_hwmon_attr(struct txgbe_adapter *adapter, int type)
{
	int rc;
	unsigned int n_attr;
	struct hwmon_attr *txgbe_attr;

	n_attr = adapter->txgbe_hwmon_buff.n_hwmon;
	txgbe_attr = &adapter->txgbe_hwmon_buff.hwmon_list[n_attr];

	switch (type) {
	case TXGBE_HWMON_TYPE_TEMP:
		txgbe_attr->dev_attr.show = txgbe_hwmon_show_temp;
		snprintf(txgbe_attr->name, sizeof(txgbe_attr->name),
			 "temp%u_input", 0);
		break;
	case TXGBE_HWMON_TYPE_ALARMTHRESH:
		txgbe_attr->dev_attr.show = txgbe_hwmon_show_alarmthresh;
		snprintf(txgbe_attr->name, sizeof(txgbe_attr->name),
			 "temp%u_alarmthresh", 0);
		break;
	case TXGBE_HWMON_TYPE_DALARMTHRESH:
		txgbe_attr->dev_attr.show = txgbe_hwmon_show_dalarmthresh;
		snprintf(txgbe_attr->name, sizeof(txgbe_attr->name),
			 "temp%u_dalarmthresh", 0);
		break;
	default:
		rc = -EPERM;
		return rc;
	}

	/* These always the same regardless of type */
	txgbe_attr->sensor =
		&adapter->hw.mac.thermal_sensor_data.sensor;
	txgbe_attr->hw = &adapter->hw;
	txgbe_attr->dev_attr.store = NULL;
	txgbe_attr->dev_attr.attr.mode = 0444;
	txgbe_attr->dev_attr.attr.name = txgbe_attr->name;

	rc = device_create_file(pci_dev_to_dev(adapter->pdev),
				&txgbe_attr->dev_attr);

	if (rc == 0)
		++adapter->txgbe_hwmon_buff.n_hwmon;

	return rc;
}

static void txgbe_sysfs_del_adapter(struct txgbe_adapter __maybe_unused *adapter)
{
	int i;

	if (!adapter)
		return;

	for (i = 0; i < adapter->txgbe_hwmon_buff.n_hwmon; i++) {
		device_remove_file(pci_dev_to_dev(adapter->pdev),
				   &adapter->txgbe_hwmon_buff.hwmon_list[i].dev_attr);
	}

	kfree(adapter->txgbe_hwmon_buff.hwmon_list);

	if (adapter->txgbe_hwmon_buff.device)
		hwmon_device_unregister(adapter->txgbe_hwmon_buff.device);
}

/* called from txgbe_main.c */
void txgbe_sysfs_exit(struct txgbe_adapter *adapter)
{
	txgbe_sysfs_del_adapter(adapter);
}

/* called from txgbe_main.c */
int txgbe_sysfs_init(struct txgbe_adapter *adapter)
{
	int rc = 0;
	struct hwmon_buff *txgbe_hwmon = &adapter->txgbe_hwmon_buff;
	int n_attrs;
	struct txgbe_hw *hw = &adapter->hw;

	if (!adapter)
		goto err;

	/* Don't create thermal hwmon interface if no sensors present */
	if (hw->mac.ops.init_thermal_sensor_thresh(hw))
		goto no_thermal;

	/* Allocation space for max attributs
	 * max num sensors * values (temp, alamthresh, dalarmthresh)
	 */
	n_attrs = 3;
	txgbe_hwmon->hwmon_list = kcalloc(n_attrs, sizeof(struct hwmon_attr),
					  GFP_KERNEL);
	if (!txgbe_hwmon->hwmon_list) {
		rc = -ENOMEM;
		goto err;
	}

	txgbe_hwmon->device =
			hwmon_device_register(pci_dev_to_dev(adapter->pdev));
	if (IS_ERR(txgbe_hwmon->device)) {
		rc = PTR_ERR(txgbe_hwmon->device);
		goto err;
	}

	/* Bail if any hwmon attr struct fails to initialize */
	rc = txgbe_add_hwmon_attr(adapter, TXGBE_HWMON_TYPE_TEMP);
	rc |= txgbe_add_hwmon_attr(adapter, TXGBE_HWMON_TYPE_ALARMTHRESH);
	rc |= txgbe_add_hwmon_attr(adapter, TXGBE_HWMON_TYPE_DALARMTHRESH);
	if (rc)
		goto err;

no_thermal:
	goto exit;

err:
	txgbe_sysfs_del_adapter(adapter);
exit:
	return rc;
}
