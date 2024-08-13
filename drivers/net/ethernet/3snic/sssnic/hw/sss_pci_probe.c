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
#include "sss_pci_remove.h"
#include "sss_pci_global.h"
#include "sss_tool.h"

#define SSS_SYNC_YEAR_OFFSET		1900
#define SSS_SYNC_MONTH_OFFSET		1

#define SSS_CHECK_EVENT_INFO(event) \
			((event)->service == SSS_EVENT_SRV_COMM && \
			(event)->type == SSS_EVENT_FAULT)

#define SSS_CHECK_FAULT_EVENT_INFO(hwdev, fault_event) \
			((fault_event)->fault_level == SSS_FAULT_LEVEL_SERIOUS_FLR && \
			(fault_event)->info.chip.func_id < sss_get_max_pf_num(hwdev))

#define SSS_GET_CFG_REG_BAR(pdev) (SSS_IS_VF_DEV(pdev) ? \
			SSS_VF_PCI_CFG_REG_BAR : SSS_PF_PCI_CFG_REG_BAR)

static bool sss_get_vf_load_state(struct pci_dev *pdev)
{
	struct sss_pci_adapter *adapter = NULL;
	struct pci_dev *dev = NULL;

	if (pci_is_root_bus(pdev->bus))
		return false;

	dev = pdev->is_virtfn ? pdev->physfn : pdev;
	adapter = pci_get_drvdata(dev);

	if (!adapter) {
		sdk_err(&pdev->dev, "Invalid adapter, is null.\n");
		return false;
	}

	return true;
}

static int sss_init_pci_dev(struct pci_dev *pdev)
{
	int ret;

	ret = pci_enable_device(pdev);
	if (ret != 0) {
		sdk_err(&pdev->dev, "Fail to enable pci device\n");
		goto enable_err;
	}

	ret = pci_request_regions(pdev, SSS_DRV_NAME);
	if (ret != 0) {
		sdk_err(&pdev->dev, "Fail to request regions\n");
		goto regions_err;
	}

	pci_enable_pcie_error_reporting(pdev);

	pci_set_master(pdev);

	ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(64));
	if (ret != 0) {
		sdk_warn(&pdev->dev, "Fail to set 64-bit DMA mask\n");

		ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
		if (ret != 0) {
			sdk_err(&pdev->dev, "Fail to set DMA mask\n");
			goto dma_err;
		}
	}

	ret = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(64));
	if (ret != 0) {
		sdk_warn(&pdev->dev, "Fail to set 64-bit coherent DMA mask\n");

		ret = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
		if (ret != 0) {
			sdk_err(&pdev->dev, "Fail to set coherent DMA mask\n");
			goto dma_err;
		}
	}

	return 0;

dma_err:
	pci_clear_master(pdev);
	pci_disable_pcie_error_reporting(pdev);
	pci_release_regions(pdev);

regions_err:
	pci_disable_device(pdev);

enable_err:
	pci_set_drvdata(pdev, NULL);

	return ret;
}

void sss_set_adapter_probe_state(struct sss_pci_adapter *adapter, int state)
{
	mutex_lock(&adapter->uld_attach_mutex);
	adapter->init_state = state;
	mutex_unlock(&adapter->uld_attach_mutex);
}

static int sss_map_pci_bar(struct pci_dev *pdev,
			   struct sss_pci_adapter *adapter)
{
	adapter->db_base_paddr = pci_resource_start(pdev, SSS_PCI_DB_BAR);
	adapter->db_dwqe_len = pci_resource_len(pdev, SSS_PCI_DB_BAR);
	adapter->db_reg_bar = pci_ioremap_bar(pdev, SSS_PCI_DB_BAR);
	if (!adapter->db_reg_bar) {
		sdk_err(&pdev->dev, "Fail to map db reg bar\n");
		return -ENOMEM;
	}

	if (!SSS_IS_VF_DEV(pdev)) {
		adapter->mgmt_reg_bar = pci_ioremap_bar(pdev, SSS_PCI_MGMT_REG_BAR);
		if (!adapter->mgmt_reg_bar) {
			sdk_err(&pdev->dev, "Fail to map mgmt reg bar\n");
			goto mgmt_bar_err;
		}
	}

	adapter->intr_reg_bar = pci_ioremap_bar(pdev, SSS_PCI_INTR_REG_BAR);
	if (!adapter->intr_reg_bar) {
		sdk_err(&pdev->dev, "Fail to map intr reg bar\n");
		goto intr_bar_err;
	}

	adapter->cfg_reg_bar = pci_ioremap_bar(pdev, SSS_GET_CFG_REG_BAR(pdev));
	if (!adapter->cfg_reg_bar) {
		sdk_err(&pdev->dev, "Fail to map config reg bar\n");
		goto cfg_bar_err;
	}

	return 0;

cfg_bar_err:
	iounmap(adapter->intr_reg_bar);

intr_bar_err:
	if (!SSS_IS_VF_DEV(pdev))
		iounmap(adapter->mgmt_reg_bar);

mgmt_bar_err:
	iounmap(adapter->db_reg_bar);

	return -ENOMEM;
}

static void sss_send_event_to_uld(struct sss_pci_adapter *adapter,
				  struct sss_event_info *event_info)
{
	enum sss_service_type type;
	const char **uld_name = sss_get_uld_names();
	struct sss_uld_info *uld_info = sss_get_uld_info();

	for (type = SSS_SERVICE_TYPE_NIC; type < SSS_SERVICE_TYPE_MAX; type++) {
		if (test_and_set_bit(type, &adapter->uld_run_state)) {
			sdk_warn(&adapter->pcidev->dev,
				 "Fail to send event, svc: 0x%x, event type: 0x%x, uld_name: %s\n",
				 event_info->service, event_info->type, uld_name[type]);
			continue;
		}

		if (uld_info[type].event)
			uld_info[type].event(&adapter->hal_dev,
					     adapter->uld_dev[type], event_info);
		clear_bit(type, &adapter->uld_run_state);
	}
}

static void sss_send_event_to_dst(struct sss_pci_adapter *adapter, u16 func_id,
				  struct sss_event_info *event_info)
{
	struct sss_pci_adapter *dest_adapter = NULL;

	sss_hold_chip_node();
	list_for_each_entry(dest_adapter, &adapter->chip_node->func_list, node) {
		if (adapter->init_state == SSS_IN_REMOVE)
			continue;
		if (sss_get_func_type(dest_adapter->hwdev) == SSS_FUNC_TYPE_VF)
			continue;

		if (sss_get_global_func_id(dest_adapter->hwdev) == func_id) {
			sss_send_event_to_uld(dest_adapter, event_info);
			break;
		}
	}
	sss_put_chip_node();
}

static void sss_send_event_to_all_pf(struct sss_pci_adapter *adapter,
				     struct sss_event_info *event_info)
{
	struct sss_pci_adapter *dest_adapter = NULL;

	sss_hold_chip_node();
	list_for_each_entry(dest_adapter, &adapter->chip_node->func_list, node) {
		if (adapter->init_state == SSS_IN_REMOVE)
			continue;

		if (sss_get_func_type(dest_adapter->hwdev) == SSS_FUNC_TYPE_VF)
			continue;

		sss_send_event_to_uld(dest_adapter, event_info);
	}
	sss_put_chip_node();
}

static void sss_process_event(void *data, struct sss_event_info *event_info)
{
	u16 id;
	struct sss_pci_adapter *pci_adapter = data;
	struct sss_fault_event *fault_event = (void *)event_info->event_data;

	if (SSS_CHECK_EVENT_INFO(event_info) &&
	    SSS_CHECK_FAULT_EVENT_INFO(pci_adapter->hwdev, fault_event)) {
		id = fault_event->info.chip.func_id;
		return sss_send_event_to_dst(pci_adapter, id, event_info);
	}

	if (event_info->type == SSS_EVENT_MGMT_WATCHDOG)
		sss_send_event_to_all_pf(pci_adapter, event_info);
	else
		sss_send_event_to_uld(pci_adapter, event_info);
}

static void sss_sync_time_to_chip(struct sss_pci_adapter *adapter)
{
	int ret;
	u64 mstime;
	struct timeval val = {0};
	struct rtc_time r_time = {0};

	do_gettimeofday(&val);

	mstime = (u64)(val.tv_sec * MSEC_PER_SEC + val.tv_usec / USEC_PER_MSEC);
	ret = sss_chip_sync_time(adapter->hwdev, mstime);
	if (ret != 0) {
		sdk_err(&adapter->pcidev->dev, "Fail to sync UTC time to fw, ret:%d.\n", ret);
	} else {
		rtc_time_to_tm((unsigned long)(val.tv_sec), &r_time);
		sdk_info(&adapter->pcidev->dev,
			 "Success to sync UTC time to fw. UTC time %d-%02d-%02d %02d:%02d:%02d.\n",
			 r_time.tm_year + SSS_SYNC_YEAR_OFFSET,
			 r_time.tm_mon + SSS_SYNC_MONTH_OFFSET,
			 r_time.tm_mday, r_time.tm_hour, r_time.tm_min, r_time.tm_sec);
	}
}

int sss_attach_uld_driver(struct sss_pci_adapter *adapter,
			  enum sss_service_type type, const struct sss_uld_info *uld_info)
{
	int ret = 0;
	void *uld = NULL;
	const char **name = sss_get_uld_names();
	struct pci_dev *pdev = adapter->pcidev;

	mutex_lock(&adapter->uld_attach_mutex);

	if (adapter->uld_dev[type]) {
		sdk_err(&pdev->dev, "Fail to attach pci dev, driver %s\n", name[type]);
		mutex_unlock(&adapter->uld_attach_mutex);
		return 0;
	}

	ret = uld_info->probe(&adapter->hal_dev, &uld, adapter->uld_dev_name[type]);
	if (ret != 0) {
		sdk_err(&pdev->dev, "Fail to probe for driver %s\n", name[type]);
		mutex_unlock(&adapter->uld_attach_mutex);
		return ret;
	}

	adapter->uld_dev[type] = uld;
	set_bit(type, &adapter->uld_attach_state);
	mutex_unlock(&adapter->uld_attach_mutex);

	sdk_info(&pdev->dev, "Success to attach %s driver\n", name[type]);

	return 0;
}

static bool sss_get_vf_service_load(struct pci_dev *pdev,
				    enum sss_service_type service_type)
{
	struct sss_pci_adapter *adapter = NULL;
	struct pci_dev *dev = NULL;

	if (!pdev) {
		pr_err("Invalid pdev, is null.\n");
		return false;
	}

	dev = (pdev->is_virtfn != 0) ? pdev->physfn : pdev;

	adapter = pci_get_drvdata(dev);
	if (!adapter) {
		sdk_err(&pdev->dev, "Invalid pci adapter, is null.\n");
		return false;
	}

	return true;
}

static void sss_attach_all_uld_driver(struct sss_pci_adapter *adapter)
{
	enum sss_service_type type;
	struct pci_dev *pdev = adapter->pcidev;
	struct sss_uld_info *info = sss_get_uld_info();

	sss_hold_chip_node();
	sss_lock_uld();
	for (type = SSS_SERVICE_TYPE_NIC; type < SSS_SERVICE_TYPE_MAX; type++) {
		if (!info[type].probe)
			continue;
		if (pdev->is_virtfn &&
		    !sss_get_vf_service_load(pdev, type)) {
			sdk_info(&pdev->dev,
				 "VF dev disable service_type = %d load in host\n", type);
			continue;
		}
		sss_attach_uld_driver(adapter, type, &info[type]);
	}
	sss_unlock_uld();
	sss_put_chip_node();
}

static int sss_attach_uld_dev(struct sss_pci_adapter *adapter)
{
	struct pci_dev *pdev = adapter->pcidev;

	adapter->hal_dev.pdev = pdev;
	adapter->hal_dev.hwdev = adapter->hwdev;

	if (!sss_attach_is_enable())
		return 0;

	sss_attach_all_uld_driver(adapter);

	return 0;
}

int sss_register_uld(enum sss_service_type type, struct sss_uld_info *uld_info)
{
	struct sss_pci_adapter *adapter = NULL;
	struct sss_card_node *card_node = NULL;
	struct list_head *list = NULL;
	struct sss_uld_info *info = sss_get_uld_info();
	const char **uld_name = sss_get_uld_names();

	if (type >= SSS_SERVICE_TYPE_MAX) {
		pr_err("Unknown type %d of uld to register\n", type);
		return -EINVAL;
	}

	if (!uld_info || !uld_info->probe || !uld_info->remove) {
		pr_err("Invalid info of %s driver to register\n", uld_name[type]);
		return -EINVAL;
	}

	sss_hold_chip_node();
	sss_lock_uld();

	if (info[type].probe) {
		sss_unlock_uld();
		sss_put_chip_node();
		pr_err("Driver %s already register\n", uld_name[type]);
		return -EINVAL;
	}

	list = sss_get_chip_list();
	memcpy(&info[type], uld_info, sizeof(*uld_info));
	list_for_each_entry(card_node, list, node) {
		list_for_each_entry(adapter, &card_node->func_list, node) {
			if (sss_attach_uld_driver(adapter, type, uld_info) != 0) {
				sdk_err(&adapter->pcidev->dev,
					"Fail to attach %s driver to pci dev\n", uld_name[type]);
				continue;
			}
		}
	}

	sss_unlock_uld();
	sss_put_chip_node();

	pr_info("Success to register %s driver\n", uld_name[type]);
	return 0;
}
EXPORT_SYMBOL(sss_register_uld);

static int sss_notify_ok_to_chip(struct sss_pci_adapter *adapter)
{
	int ret;
	struct pci_dev *pdev = adapter->pcidev;

	if (sss_get_func_type(adapter->hwdev) == SSS_FUNC_TYPE_VF)
		return 0;

	ret = sss_chip_set_pci_bdf_num(adapter->hwdev, pdev->bus->number,
				       PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn));
	if (ret != 0) {
		sdk_err(&pdev->dev, "Fail to set BDF info to chip\n");
		return ret;
	}

	return 0;
}

static int sss_init_function(struct pci_dev *pdev, struct sss_pci_adapter *adapter)
{
	int ret;

	ret = sss_init_hwdev(adapter);
	if (ret != 0) {
		adapter->hwdev = NULL;
		sdk_err(&pdev->dev, "Fail to init hardware device\n");
		return -EFAULT;
	}

	sss_register_dev_event(adapter->hwdev, adapter, sss_process_event);

	if (sss_get_func_type(adapter->hwdev) != SSS_FUNC_TYPE_VF) {
		set_bit(SSS_SRIOV_PRESENT, &adapter->sriov_info.state);
		sss_sync_time_to_chip(adapter);
	}

	sss_chip_node_lock();
	ret = sss_tool_init(adapter->hwdev, adapter->chip_node);
	if (ret) {
		sss_chip_node_unlock();
		sdk_err(&pdev->dev, "Failed to initialize dbgtool\n");
		goto nictool_init_err;
	}
	sss_chip_node_unlock();

	sss_add_func_list(adapter);

	ret = sss_attach_uld_dev(adapter);
	if (ret != 0) {
		sdk_err(&pdev->dev, "Fail to attach uld dev\n");
		goto attach_uld_err;
	}

	return 0;

attach_uld_err:
	sss_del_func_list(adapter);

	sss_chip_node_lock();
	sss_tool_uninit(adapter->hwdev, adapter->chip_node);
	sss_chip_node_unlock();
nictool_init_err:
	sss_unregister_dev_event(adapter->hwdev);

	sss_deinit_hwdev(adapter->hwdev);

	return ret;
}

static int sss_init_adapter(struct sss_pci_adapter *adapter)
{
	int ret;
	struct pci_dev *pdev = adapter->pcidev;

	if (pdev->is_virtfn != 0 && (!sss_get_vf_load_state(pdev))) {
		sdk_info(&pdev->dev, "Vf dev disable load in host\n");
		return 0;
	}

	sss_set_adapter_probe_state(adapter, SSS_PROBE_START);

	ret = sss_map_pci_bar(pdev, adapter);
	if (ret != 0) {
		sdk_err(&pdev->dev, "Fail to map bar\n");
		goto map_bar_fail;
	}

	/* if chip information of pcie function exist, add the function into chip */
	ret = sss_alloc_chip_node(adapter);
	if (ret != 0) {
		sdk_err(&pdev->dev, "Fail to add new chip node to global list\n");
		goto alloc_chip_node_fail;
	}

	ret = sss_init_function(pdev, adapter);
	if (ret != 0)
		goto func_init_err;

	ret = sss_notify_ok_to_chip(adapter);
	if (ret != 0) {
		sdk_err(&pdev->dev, "Fail to notify ok\n");
		goto notify_err;
	}

	sss_set_adapter_probe_state(adapter, SSS_PROBE_OK);

	return 0;

notify_err:
	sss_deinit_function(pdev);

func_init_err:
	sss_free_chip_node(adapter);

alloc_chip_node_fail:
	sss_unmap_pci_bar(adapter);

map_bar_fail:
	sdk_err(&pdev->dev, "Fail to init adapter\n");
	return ret;
}

static void sss_init_adapter_param(struct sss_pci_adapter *adapter,
				   struct pci_dev *pdev)
{
	adapter->pcidev = pdev;
	adapter->init_state = SSS_NO_PROBE;
	spin_lock_init(&adapter->dettach_uld_lock);
	mutex_init(&adapter->uld_attach_mutex);
	pci_set_drvdata(pdev, adapter);
}

int sss_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int ret;
	struct sss_pci_adapter *adapter = NULL;

	sdk_info(&pdev->dev, "Pci probe begin\n");

	if (!pdev)
		return -EINVAL;

	adapter = kzalloc(sizeof(*adapter), GFP_KERNEL);
	if (!adapter) {
		ret = -ENOMEM;
		goto init_pci_err;
	}

	sss_init_adapter_param(adapter, pdev);

	ret = sss_init_pci_dev(pdev);
	if (ret != 0) {
		kfree(adapter);
		sdk_err(&pdev->dev, "Fail to init pci device\n");
		goto init_pci_err;
	}

	ret = sss_init_adapter(adapter);
	if (ret != 0)
		goto init_adapter_err;

	sdk_info(&pdev->dev, "Success to probe pci\n");
	return 0;

init_adapter_err:
	sss_deinit_pci_dev(pdev);

init_pci_err:
	sdk_err(&pdev->dev, "Fail to pci probe\n");

	return ret;
}
