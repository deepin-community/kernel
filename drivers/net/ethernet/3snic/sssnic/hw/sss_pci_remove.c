// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <net/addrconf.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/io-mapping.h>
#include <linux/interrupt.h>
#include <linux/inetdevice.h>
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/rtc.h>
#include <linux/aer.h>
#include <linux/debugfs.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_pci.h"
#include "sss_pci_id_tbl.h"
#include "sss_pci_sriov.h"
#include "sss_adapter_mgmt.h"
#include "sss_hwdev_init.h"
#include "sss_hwdev_api.h"
#include "sss_hwif_mgmt_init.h"
#include "sss_pci_global.h"
#include "sss_tool.h"

#define SSS_WAIT_SRIOV_CFG_TIMEOUT	15000
#define SSS_EVENT_PROCESS_TIMEOUT	10000

#define SSS_SRIOV_MIN_USLEEP 9900
#define SSS_SRIOV_MAX_USLEEP 10000

#define SSS_EVENT_MIN_USLEEP 900
#define SSS_EVENT_MAX_USLEEP 1000

static void sss_set_adapter_remove_state(struct sss_pci_adapter *adapter)
{
	struct pci_dev *pdev = adapter->pcidev;

	mutex_lock(&adapter->uld_attach_mutex);
	if (adapter->init_state != SSS_PROBE_OK) {
		sdk_warn(&pdev->dev, "Current function don not need remove\n");
		mutex_unlock(&adapter->uld_attach_mutex);
	}
	adapter->init_state = SSS_IN_REMOVE;
	mutex_unlock(&adapter->uld_attach_mutex);
}

static void sss_wait_sriov_cfg_complete(struct sss_pci_adapter *adapter)
{
	unsigned long end_time;
	struct sss_sriov_info *info = &adapter->sriov_info;

	clear_bit(SSS_SRIOV_PRESENT, &info->state);
	usleep_range(SSS_SRIOV_MIN_USLEEP, SSS_SRIOV_MAX_USLEEP);

	end_time = jiffies + msecs_to_jiffies(SSS_WAIT_SRIOV_CFG_TIMEOUT);
	do {
		if (!test_bit(SSS_SRIOV_ENABLE, &info->state) &&
		    !test_bit(SSS_SRIOV_DISABLE, &info->state))
			return;

		usleep_range(SSS_SRIOV_MIN_USLEEP, SSS_SRIOV_MAX_USLEEP);
	} while (time_before(jiffies, end_time));
}

static bool sss_wait_uld_dev_timeout(struct sss_pci_adapter *adapter,
				     enum sss_service_type type)
{
	unsigned long end_time;

	end_time = jiffies + msecs_to_jiffies(SSS_EVENT_PROCESS_TIMEOUT);
	do {
		if (!test_and_set_bit(type, &adapter->uld_run_state))
			return false;

		usleep_range(SSS_EVENT_MIN_USLEEP, SSS_EVENT_MAX_USLEEP);
	} while (time_before(jiffies, end_time));

	if (!test_and_set_bit(type, &adapter->uld_run_state))
		return false;

	return true;
}

void sss_detach_uld_driver(struct sss_pci_adapter *adapter,
			   enum sss_service_type type)
{
	bool timeout;
	struct sss_uld_info *info = sss_get_uld_info();
	const char **name = sss_get_uld_names();

	mutex_lock(&adapter->uld_attach_mutex);
	if (!adapter->uld_dev[type]) {
		mutex_unlock(&adapter->uld_attach_mutex);
		return;
	}

	timeout = sss_wait_uld_dev_timeout(adapter, type);

	spin_lock_bh(&adapter->dettach_uld_lock);
	clear_bit(type, &adapter->uld_attach_state);
	spin_unlock_bh(&adapter->dettach_uld_lock);

	info[type].remove(&adapter->hal_dev, adapter->uld_dev[type]);
	adapter->uld_dev[type] = NULL;

	if (!timeout)
		clear_bit(type, &adapter->uld_run_state);

	sdk_info(&adapter->pcidev->dev,
		 "Success to detach %s driver from pci device\n", name[type]);
	mutex_unlock(&adapter->uld_attach_mutex);
}

void sss_detach_all_uld_driver(struct sss_pci_adapter *adapter)
{
	struct sss_uld_info *info = sss_get_uld_info();
	enum sss_service_type type;

	sss_hold_chip_node();
	sss_lock_uld();
	for (type = SSS_SERVICE_TYPE_MAX - 1; type > SSS_SERVICE_TYPE_NIC; type--) {
		if (info[type].probe)
			sss_detach_uld_driver(adapter, type);
	}

	if (info[SSS_SERVICE_TYPE_NIC].probe)
		sss_detach_uld_driver(adapter, SSS_SERVICE_TYPE_NIC);
	sss_unlock_uld();
	sss_put_chip_node();
}

void sss_dettach_uld_dev(struct sss_pci_adapter *adapter)
{
	sss_detach_all_uld_driver(adapter);
}

void sss_unregister_uld(enum sss_service_type type)
{
	struct sss_pci_adapter *adapter = NULL;
	struct sss_card_node *card_node = NULL;
	struct list_head *card_list = NULL;
	struct sss_uld_info *info = sss_get_uld_info();

	if (type >= SSS_SERVICE_TYPE_MAX) {
		pr_err("Unknown type %d of uld to unregister\n", type);
		return;
	}

	sss_hold_chip_node();
	sss_lock_uld();
	card_list = sss_get_chip_list();
	list_for_each_entry(card_node, card_list, node) {
		/* detach vf first */
		list_for_each_entry(adapter, &card_node->func_list, node)
			if (sss_get_func_type(adapter->hwdev) == SSS_FUNC_TYPE_VF)
				sss_detach_uld_driver(adapter, type);

		list_for_each_entry(adapter, &card_node->func_list, node)
			if (sss_get_func_type(adapter->hwdev) == SSS_FUNC_TYPE_PF)
				sss_detach_uld_driver(adapter, type);

		list_for_each_entry(adapter, &card_node->func_list, node)
			if (sss_get_func_type(adapter->hwdev) == SSS_FUNC_TYPE_PPF)
				sss_detach_uld_driver(adapter, type);
	}

	memset(&info[type], 0, sizeof(*info));
	sss_unlock_uld();
	sss_put_chip_node();
}
EXPORT_SYMBOL(sss_unregister_uld);

void sss_deinit_function(struct pci_dev *pdev)
{
	struct sss_pci_adapter *adapter = sss_get_adapter_by_pcidev(pdev);

	sss_chip_disable_mgmt_channel(adapter->hwdev);

	sss_flush_mgmt_workq(adapter->hwdev);

	sss_del_func_list(adapter);

	sss_chip_node_lock();
	sss_tool_uninit(adapter->hwdev, adapter->chip_node);
	sss_chip_node_unlock();

	sss_dettach_uld_dev(adapter);

	sss_unregister_dev_event(adapter->hwdev);

	sss_deinit_hwdev(adapter->hwdev);
}

void sss_unmap_pci_bar(struct sss_pci_adapter *adapter)
{
	iounmap(adapter->cfg_reg_bar);
	iounmap(adapter->intr_reg_bar);

	if (!SSS_IS_VF_DEV(adapter->pcidev))
		iounmap(adapter->mgmt_reg_bar);

	iounmap(adapter->db_reg_bar);
}

int sss_deinit_adapter(struct sss_pci_adapter *adapter)
{
	struct pci_dev *pdev = adapter->pcidev;

	sss_set_adapter_remove_state(adapter);

	sss_hwdev_detach(adapter->hwdev);

	if (sss_get_func_type(adapter->hwdev) != SSS_FUNC_TYPE_VF) {
		sss_wait_sriov_cfg_complete(adapter);
		sss_pci_disable_sriov(adapter);
	}

	sss_deinit_function(pdev);

	sss_free_chip_node(adapter);

	sss_unmap_pci_bar(adapter);

	sss_set_adapter_probe_state(adapter, SSS_NO_PROBE);

	sdk_info(&pdev->dev, "Pcie device removed function\n");

	return 0;
}

void sss_deinit_pci_dev(struct pci_dev *pdev)
{
	struct sss_pci_adapter *adapter = sss_get_adapter_by_pcidev(pdev);

	pci_clear_master(pdev);
	pci_release_regions(pdev);
	pci_disable_pcie_error_reporting(pdev);
	pci_disable_device(pdev);
	pci_set_drvdata(pdev, NULL);
	kfree(adapter);
}

void sss_pci_remove(struct pci_dev *pdev)
{
	struct sss_pci_adapter *adapter = sss_get_adapter_by_pcidev(pdev);

	if (!adapter)
		return;

	sdk_info(&pdev->dev, "Begin pcie device remove\n");

	sss_deinit_adapter(adapter);

	sss_deinit_pci_dev(pdev);

	sdk_info(&pdev->dev, "Success to remove pcie device\n");
}
