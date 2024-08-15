// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_hwdev.h"
#include "sss_adapter.h"
#include "sss_hwdev_api.h"
#include "sss_hwdev_mgmt_info.h"
#include "sss_hwdev_mgmt_channel.h"
#include "sss_hwdev_cap.h"
#include "sss_hwdev_link.h"
#include "sss_hwdev_io_flush.h"
#include "sss_hwif_init.h"
#include "sss_hwif_api.h"
#include "sss_hwif_export.h"
#include "sss_hwif_mgmt_init.h"

enum sss_host_mode {
	SSS_HOST_MODE_NORMAL = 0,
	SSS_HOST_MODE_VM,
	SSS_HOST_MODE_BM,
	SSS_HOST_MODE_MAX,
};

#define SSS_HWDEV_WQ_NAME				"sssnic_hardware"
#define SSS_WQ_MAX_REQ					10

#define SSS_DETECT_PCIE_LINK_DOWN_RETRY	2

#define SSS_CHN_BUSY_TIMEOUT		25

#define SSS_HEARTBEAT_TIMER_EXPIRES		5000
#define SSS_HEARTBEAT_PERIOD			1000

#define SSS_GET_PCIE_LINK_STATUS(hwdev) \
		((hwdev)->heartbeat.pcie_link_down ? \
			SSS_EVENT_PCIE_LINK_DOWN : SSS_EVENT_HEART_LOST)

#define SSS_SET_FUNC_HOST_MODE(hwdev, mode) \
do { \
	if ((mode) >= SSS_FUNC_MOD_MIN && (mode) <= SSS_FUNC_MOD_MAX) { \
		(hwdev)->func_mode = (mode); \
	} else \
		(hwdev)->func_mode = SSS_FUNC_MOD_NORMAL_HOST; \
} while (0)

#define SSS_SYNFW_TIME_PERIOD		(60 * 60 * 1000)
#define SSS_CHANNEL_DETECT_PERIOD	(5 * 1000)

#define SSS_COMM_SUPPORT_CHANNEL_DETECT(hwdev) \
			((hwdev)->features[0] & SSS_COMM_F_CHANNEL_DETECT)

typedef void (*sss_set_mode_handler_t)(struct sss_hwdev *hwdev);

static struct sss_hwdev *sss_alloc_hwdev(void)
{
	struct sss_hwdev *hwdev;

	hwdev = kzalloc(sizeof(*hwdev), GFP_KERNEL);
	if (!hwdev)
		return NULL;

	hwdev->chip_fault_stats = vzalloc(SSS_CHIP_FAULT_SIZE);
	if (!hwdev->chip_fault_stats) {
		kfree(hwdev);
		return NULL;
	}

	return hwdev;
}

static void sss_free_hwdev(struct sss_hwdev *hwdev)
{
	vfree(hwdev->chip_fault_stats);
	kfree(hwdev);
}

static void sss_init_hwdev_param(struct sss_hwdev *hwdev,
				 struct sss_pci_adapter *adapter)
{
	struct pci_dev *pdev = adapter->pcidev;

	hwdev->adapter_hdl = adapter;
	hwdev->pcidev_hdl = pdev;
	hwdev->dev_hdl = &pdev->dev;
	hwdev->chip_node = adapter->chip_node;
	spin_lock_init(&hwdev->channel_lock);
}

static void sss_set_chip_present_flag(struct sss_hwdev *hwdev, bool present)
{
	hwdev->chip_present_flag = !!present;
}

static bool sss_is_chip_abnormal(struct sss_hwdev *hwdev)
{
	u32 pcie_status;

	if (!sss_get_dev_present_flag(hwdev))
		return false;

	pcie_status = sss_chip_get_pcie_link_status(hwdev);
	if (pcie_status == SSS_PCIE_LINK_DOWN) {
		hwdev->heartbeat.pcie_link_down_cnt++;
		sdk_warn(hwdev->dev_hdl, "Pcie link down\n");
		if (hwdev->heartbeat.pcie_link_down_cnt >= SSS_DETECT_PCIE_LINK_DOWN_RETRY) {
			sss_set_chip_present_flag(hwdev, false);
			sss_force_complete_all(hwdev);
			hwdev->heartbeat.pcie_link_down = true;
			return true;
		}

		return false;
	}

	if (pcie_status != SSS_PCIE_LINK_UP) {
		hwdev->heartbeat.heartbeat_lost = true;
		return true;
	}

	hwdev->heartbeat.pcie_link_down_cnt = 0;

	return false;
}

static void sss_update_aeq_stat(struct sss_hwdev *hwdev)
{
	if (hwdev->aeq_stat.last_recv_cnt != hwdev->aeq_stat.cur_recv_cnt) {
		hwdev->aeq_stat.last_recv_cnt = hwdev->aeq_stat.cur_recv_cnt;
		hwdev->aeq_stat.busy_cnt = 0;
	} else {
		hwdev->aeq_stat.busy_cnt++;
	}
}

static void sss_update_channel_status(struct sss_hwdev *hwdev)
{
	struct sss_card_node *node = hwdev->chip_node;

	if (!node)
		return;

	if (sss_get_func_type(hwdev) != SSS_FUNC_TYPE_PPF ||
	    !SSS_COMM_SUPPORT_CHANNEL_DETECT(hwdev) ||
	    atomic_read(&node->channel_timeout_cnt))
		return;

	if (test_bit(SSS_HW_MBX_INIT_OK, &hwdev->func_state)) {
		sss_update_aeq_stat(hwdev);

		if (hwdev->aeq_stat.busy_cnt > SSS_CHN_BUSY_TIMEOUT) {
			sdk_err(hwdev->dev_hdl, "Detect channel busy\n");
			atomic_inc(&node->channel_timeout_cnt);
		}
	}
}

static void sss_heartbeat_timer_handler(struct timer_list *t)
{
	struct sss_hwdev *hwdev = from_timer(hwdev, t, heartbeat.heartbeat_timer);

	if (sss_is_chip_abnormal(hwdev)) {
		queue_work(hwdev->workq, &hwdev->heartbeat.lost_work);
	} else {
		mod_timer(&hwdev->heartbeat.heartbeat_timer,
			  jiffies + msecs_to_jiffies(SSS_HEARTBEAT_PERIOD));
	}

	sss_update_channel_status(hwdev);
}

static void sss_heartbeat_lost_handler(struct work_struct *work)
{
	u16 fault_level;
	u16 pcie_src;
	struct sss_event_info event_info = {0};
	struct sss_hwdev *hwdev = container_of(work, struct sss_hwdev,
					       heartbeat.lost_work);

	atomic_inc(&hwdev->hw_stats.heart_lost_stats);

	if (hwdev->event_handler) {
		event_info.type = SSS_GET_PCIE_LINK_STATUS(hwdev);
		event_info.service = SSS_EVENT_SRV_COMM;
		hwdev->event_handler(hwdev->event_handler_data, &event_info);
	}

	if (hwdev->heartbeat.pcie_link_down) {
		sdk_err(hwdev->dev_hdl, "Detect pcie is link down\n");
		fault_level = SSS_FAULT_LEVEL_HOST;
		pcie_src = SSS_FAULT_SRC_PCIE_LINK_DOWN;
	} else {
		sdk_err(hwdev->dev_hdl, "Heart lost report received, func_id: %d\n",
			sss_get_global_func_id(hwdev));
		fault_level = SSS_FAULT_LEVEL_FATAL;
		pcie_src = SSS_FAULT_SRC_HOST_HEARTBEAT_LOST;
	}

	sss_dump_chip_err_info(hwdev);
}

static void sss_create_heartbeat_timer(struct sss_hwdev *hwdev)
{
	timer_setup(&hwdev->heartbeat.heartbeat_timer, sss_heartbeat_timer_handler, 0);
	hwdev->heartbeat.heartbeat_timer.expires =
		jiffies + msecs_to_jiffies(SSS_HEARTBEAT_TIMER_EXPIRES);
	add_timer(&hwdev->heartbeat.heartbeat_timer);

	INIT_WORK(&hwdev->heartbeat.lost_work, sss_heartbeat_lost_handler);
}

static void sss_destroy_heartbeat_timer(struct sss_hwdev *hwdev)
{
	destroy_work(&hwdev->heartbeat.lost_work);
	del_timer_sync(&hwdev->heartbeat.heartbeat_timer);
}

static void sss_set_bm_host_mode(struct sss_hwdev *hwdev)
{
	struct sss_service_cap *svc_cap = &hwdev->mgmt_info->svc_cap;
	u8 host_id = SSS_GET_HWIF_PCI_INTF_ID(hwdev->hwif);

	if (host_id == svc_cap->master_host_id)
		SSS_SET_FUNC_HOST_MODE(hwdev, SSS_FUNC_MOD_MULTI_BM_MASTER);
	else
		SSS_SET_FUNC_HOST_MODE(hwdev, SSS_FUNC_MOD_MULTI_BM_SLAVE);
}

static void sss_set_vm_host_mode(struct sss_hwdev *hwdev)
{
	struct sss_service_cap *svc_cap = &hwdev->mgmt_info->svc_cap;
	u8 host_id = SSS_GET_HWIF_PCI_INTF_ID(hwdev->hwif);

	if (host_id == svc_cap->master_host_id)
		SSS_SET_FUNC_HOST_MODE(hwdev, SSS_FUNC_MOD_MULTI_VM_MASTER);
	else
		SSS_SET_FUNC_HOST_MODE(hwdev, SSS_FUNC_MOD_MULTI_VM_SLAVE);
}

static void sss_set_normal_host_mode(struct sss_hwdev *hwdev)
{
	SSS_SET_FUNC_HOST_MODE(hwdev, SSS_FUNC_MOD_NORMAL_HOST);
}

static int sss_enable_multi_host(struct sss_hwdev *hwdev)
{
	if (!SSS_IS_PPF(hwdev) || !SSS_IS_MULTI_HOST(hwdev))
		return 0;

	if (SSS_IS_SLAVE_HOST(hwdev))
		sss_chip_set_slave_host_status(hwdev, sss_get_pcie_itf_id(hwdev), true);

	return 0;
}

static int sss_disable_multi_host(struct sss_hwdev *hwdev)
{
	if (!SSS_IS_PPF(hwdev) || !SSS_IS_MULTI_HOST(hwdev))
		return 0;

	if (SSS_IS_SLAVE_HOST(hwdev))
		sss_chip_set_slave_host_status(hwdev, sss_get_pcie_itf_id(hwdev), false);

	return 0;
}

static int sss_init_host_mode(struct sss_hwdev *hwdev)
{
	int ret;
	struct sss_service_cap *svc_cap = &hwdev->mgmt_info->svc_cap;
	sss_set_mode_handler_t handler[SSS_HOST_MODE_MAX] = {
		sss_set_normal_host_mode,
		sss_set_vm_host_mode,
		sss_set_bm_host_mode
	};

	if (SSS_GET_FUNC_TYPE(hwdev) == SSS_FUNC_TYPE_VF) {
		SSS_SET_FUNC_HOST_MODE(hwdev, SSS_FUNC_MOD_NORMAL_HOST);
		return 0;
	}

	if (svc_cap->srv_multi_host_mode >= SSS_HOST_MODE_MAX) {
		SSS_SET_FUNC_HOST_MODE(hwdev, SSS_FUNC_MOD_NORMAL_HOST);
		return 0;
	}

	handler[svc_cap->srv_multi_host_mode](hwdev);

	ret = sss_enable_multi_host(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init function mode\n");
		return ret;
	}

	return 0;
}

static void sss_deinit_host_mode(struct sss_hwdev *hwdev)
{
	sss_disable_multi_host(hwdev);
}

static u64 sss_get_real_time(void)
{
	struct timeval val = {0};

	do_gettimeofday(&val);

	return (u64)val.tv_sec * MSEC_PER_SEC +
	       (u64)val.tv_usec / USEC_PER_MSEC;
}

static void sss_auto_sync_time_work(struct work_struct *work)
{
	struct delayed_work *delay = to_delayed_work(work);
	struct sss_hwdev *hwdev = container_of(delay,
					       struct sss_hwdev, sync_time_task);
	int ret;

	ret = sss_chip_sync_time(hwdev, sss_get_real_time());
	if (ret != 0)
		sdk_err(hwdev->dev_hdl,
			"Fail to sync UTC time to firmware, errno:%d.\n", ret);

	queue_delayed_work(hwdev->workq, &hwdev->sync_time_task,
			   msecs_to_jiffies(SSS_SYNFW_TIME_PERIOD));
}

static void sss_auto_channel_detect_work(struct work_struct *work)
{
	struct delayed_work *delay = to_delayed_work(work);
	struct sss_hwdev *hwdev = container_of(delay,
					       struct sss_hwdev, channel_detect_task);
	struct sss_card_node *chip_node = NULL;

	sss_chip_comm_channel_detect(hwdev);

	chip_node = hwdev->chip_node;
	if (!atomic_read(&chip_node->channel_timeout_cnt))
		queue_delayed_work(hwdev->workq, &hwdev->channel_detect_task,
				   msecs_to_jiffies(SSS_CHANNEL_DETECT_PERIOD));
}

static void sss_hwdev_init_work(struct sss_hwdev *hwdev)
{
	if (SSS_GET_FUNC_TYPE(hwdev) != SSS_FUNC_TYPE_PPF)
		return;

	INIT_DELAYED_WORK(&hwdev->sync_time_task, sss_auto_sync_time_work);
	queue_delayed_work(hwdev->workq, &hwdev->sync_time_task,
			   msecs_to_jiffies(SSS_SYNFW_TIME_PERIOD));

	if (SSS_COMM_SUPPORT_CHANNEL_DETECT(hwdev)) {
		INIT_DELAYED_WORK(&hwdev->channel_detect_task,
				  sss_auto_channel_detect_work);
		queue_delayed_work(hwdev->workq, &hwdev->channel_detect_task,
				   msecs_to_jiffies(SSS_CHANNEL_DETECT_PERIOD));
	}
}

static void sss_hwdev_deinit_work(struct sss_hwdev *hwdev)
{
	if (SSS_GET_FUNC_TYPE(hwdev) != SSS_FUNC_TYPE_PPF)
		return;

	if (SSS_COMM_SUPPORT_CHANNEL_DETECT(hwdev)) {
		hwdev->features[0] &= ~(SSS_COMM_F_CHANNEL_DETECT);
		cancel_delayed_work_sync(&hwdev->channel_detect_task);
	}

	cancel_delayed_work_sync(&hwdev->sync_time_task);
}

int sss_init_hwdev(struct sss_pci_adapter *adapter)
{
	struct sss_hwdev *hwdev;
	int ret;

	hwdev = sss_alloc_hwdev();
	if (!hwdev)
		return -ENOMEM;

	sss_init_hwdev_param(hwdev, adapter);
	adapter->hwdev = hwdev;

	ret = sss_hwif_init(adapter);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init hwif\n");
		goto init_hwif_err;
	}

	sss_set_chip_present_flag(hwdev, true);

	hwdev->workq = alloc_workqueue(SSS_HWDEV_WQ_NAME, WQ_MEM_RECLAIM, SSS_WQ_MAX_REQ);
	if (!hwdev->workq) {
		sdk_err(hwdev->dev_hdl, "Fail to alloc hardware workq\n");
		goto alloc_workq_err;
	}

	sss_create_heartbeat_timer(hwdev);

	ret = sss_init_mgmt_info(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init mgmt info\n");
		goto init_mgmt_info_err;
	}

	ret = sss_init_mgmt_channel(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init mgmt channel\n");
		goto init_mgmt_channel_err;
	}

#ifdef HAVE_DEVLINK_FLASH_UPDATE_PARAMS
	ret = sss_init_devlink(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init devlink\n");
		goto init_devlink_err;
	}
#endif

	ret = sss_init_capability(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init capability\n");
		goto init_cap_err;
	}

	ret = sss_init_host_mode(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init capability\n");
		goto init_multi_host_fail;
	}

	sss_hwdev_init_work(hwdev);

	ret = sss_chip_do_nego_feature(hwdev, SSS_MGMT_MSG_SET_CMD,
				       hwdev->features, SSS_MAX_FEATURE_QWORD);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to set comm features\n");
		goto set_feature_err;
	}

	return 0;

set_feature_err:
	sss_hwdev_deinit_work(hwdev);

	sss_deinit_host_mode(hwdev);
init_multi_host_fail:
	sss_deinit_capability(hwdev);

init_cap_err:
#ifdef HAVE_DEVLINK_FLASH_UPDATE_PARAMS
	sss_deinit_devlink(hwdev);

init_devlink_err:
#endif
	sss_deinit_mgmt_channel(hwdev);

init_mgmt_channel_err:
	sss_deinit_mgmt_info(hwdev);

init_mgmt_info_err:
	sss_destroy_heartbeat_timer(hwdev);
	destroy_workqueue(hwdev->workq);

alloc_workq_err:
	sss_hwif_deinit(hwdev);

init_hwif_err:
	sss_free_hwdev(hwdev);
	adapter->hwdev = NULL;

	return -EFAULT;
}

void sss_deinit_hwdev(void *hwdev)
{
	struct sss_hwdev *dev = hwdev;
	u64 drv_features[SSS_MAX_FEATURE_QWORD] = {0};

	sss_chip_do_nego_feature(hwdev, SSS_MGMT_MSG_SET_CMD,
				 drv_features, SSS_MAX_FEATURE_QWORD);

	sss_hwdev_deinit_work(dev);

	if (SSS_IS_MULTI_HOST(dev))
		sss_disable_multi_host(dev);

	sss_hwdev_flush_io(dev, SSS_CHANNEL_COMM);

	sss_deinit_capability(dev);

#ifdef HAVE_DEVLINK_FLASH_UPDATE_PARAMS
	sss_deinit_devlink(dev);
#endif

	sss_deinit_mgmt_channel(dev);

	sss_deinit_mgmt_info(dev);
	sss_destroy_heartbeat_timer(hwdev);
	destroy_workqueue(dev->workq);

	sss_hwif_deinit(dev);
	sss_free_hwdev(dev);
}

void sss_hwdev_stop(void *hwdev)
{
	struct sss_hwdev *dev = hwdev;

	if (!hwdev)
		return;

	sss_set_chip_present_flag(hwdev, false);
	sdk_info(dev->dev_hdl, "Set card absent\n");
	sss_force_complete_all(dev);
	sdk_info(dev->dev_hdl, "All messages interacting with the chip will stop\n");
}

void sss_hwdev_detach(void *hwdev)
{
	if (!sss_chip_get_present_state((struct sss_hwdev *)hwdev)) {
		sss_set_chip_present_flag(hwdev, false);
		sss_force_complete_all(hwdev);
	}
}

void sss_hwdev_shutdown(void *hwdev)
{
	struct sss_hwdev *dev = hwdev;

	if (!hwdev)
		return;

	if (SSS_IS_SLAVE_HOST(dev))
		sss_chip_set_slave_host_status(hwdev, sss_get_pcie_itf_id(hwdev), false);
}
