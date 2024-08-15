// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt

#include <linux/pci.h>
#include <linux/interrupt.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_adapter_mgmt.h"
#include "sss_hwif_mbx.h"
#include "sss_hwif_mbx_init.h"
#include "sss_pci_sriov.h"
#include "sss_hwdev_api.h"
#include "sss_hwif_api.h"

#ifdef CONFIG_PCI_IOV
static int sss_init_vf_hw(void *hwdev, u16 vf_num)
{
	int ret;
	u16 i;
	u16 id;

	/* mbx msg channel resources will be freed during remove process */
	ret = sss_init_func_mbx_msg(hwdev, sss_get_max_vf_num(hwdev));
	if (ret != 0)
		return ret;

	/* vf use 256K as default wq page size, and can't change it */
	for (i = 1; i <= vf_num; i++) {
		id = sss_get_glb_pf_vf_offset(hwdev) + i;
		ret = sss_chip_set_wq_page_size(hwdev, id, SSS_DEFAULT_WQ_PAGE_SIZE);
		if (ret != 0)
			return ret;
	}

	return 0;
}

static void sss_deinit_vf_hw(void *hwdev, u16 vf_num)
{
	u16 i;
	u16 id;

	for (i = 1; i <= vf_num; i++) {
		id = sss_get_glb_pf_vf_offset(hwdev) + i;
		sss_chip_set_wq_page_size(hwdev, id, SSS_HW_WQ_PAGE_SIZE);
	}
}

static void sss_notify_sriov_state_change(void *hwdev, u16 vf_num)
{
	struct sss_event_info event = {0};

	event.service = SSS_EVENT_SRV_COMM;
	event.type = SSS_EVENT_SRIOV_STATE_CHANGE;

	if (vf_num > 0) {
		((struct sss_sriov_state_info *)(void *)event.event_data)->enable = 1;
		((struct sss_sriov_state_info *)(void *)event.event_data)->vf_num = vf_num;
	}

	sss_do_event_callback(hwdev, &event);
}
#endif

int sss_pci_disable_sriov(struct sss_pci_adapter *adapter)
{
#ifdef CONFIG_PCI_IOV
	void *hwdev = adapter->hwdev;
	struct pci_dev *pdev = adapter->pcidev;
	struct sss_sriov_info *info = &adapter->sriov_info;

	if (!info->enabled)
		return 0;

	if (test_and_set_bit(SSS_SRIOV_DISABLE, &info->state)) {
		sdk_err(&pdev->dev, "SR-IOV disable in process.");
		return -EPERM;
	}

	if (pci_vfs_assigned(pdev) != 0) {
		clear_bit(SSS_SRIOV_DISABLE, &info->state);
		sdk_warn(&pdev->dev, "VFs are assigned - VFs will not be deallocated\n");
		return -EPERM;
	}

	sss_notify_sriov_state_change(hwdev, 0);

	info->enabled = false;

	pci_disable_sriov(pdev);

	sss_deinit_vf_hw(hwdev, (u16)info->vf_num);
	info->vf_num = 0;

	clear_bit(SSS_SRIOV_DISABLE, &info->state);

#endif

	return 0;
}

#ifdef CONFIG_PCI_IOV
static int sss_check_existing_vf(struct sss_pci_adapter *adapter, u16 vf_num)
{
	int ret;
	struct pci_dev *pdev = adapter->pcidev;
	int existing_vf = pci_num_vf(pdev);
	struct sss_sriov_info *info = &adapter->sriov_info;

	if (existing_vf != 0 && existing_vf != vf_num) {
		ret = sss_pci_disable_sriov(adapter);
		if (ret != 0) {
			clear_bit(SSS_SRIOV_ENABLE, &info->state);
			return ret;
		}
	} else if (existing_vf == vf_num) {
		clear_bit(SSS_SRIOV_ENABLE, &info->state);
		return vf_num;
	}

	return 0;
}
#endif

static int sss_pci_enable_sriov(struct sss_pci_adapter *adapter, u16 vf_num)
{
#ifdef CONFIG_PCI_IOV
	int ret = 0;
	void *hwdev = adapter->hwdev;
	struct pci_dev *pdev = adapter->pcidev;
	struct sss_sriov_info *info = &adapter->sriov_info;

	if (test_and_set_bit(SSS_SRIOV_ENABLE, &info->state)) {
		sdk_err(&pdev->dev, "SR-IOV disable, vf_num %d\n", vf_num);
		return -EPERM;
	}

	if (vf_num > pci_sriov_get_totalvfs(pdev)) {
		clear_bit(SSS_SRIOV_ENABLE, &info->state);
		return -ERANGE;
	}

	ret = sss_check_existing_vf(adapter, vf_num);
	if (ret != 0)
		return ret;

	ret = sss_init_vf_hw(hwdev, vf_num);
	if (ret != 0) {
		sdk_err(&pdev->dev, "Fail to init vf in hw, ret: %d\n", ret);
		clear_bit(SSS_SRIOV_ENABLE, &info->state);
		return ret;
	}

	ret = pci_enable_sriov(pdev, vf_num);
	if (ret != 0) {
		sdk_err(&pdev->dev, "Fail to enable SR-IOV, ret: %d\n", ret);
		clear_bit(SSS_SRIOV_ENABLE, &info->state);
		return ret;
	}

	info->enabled = true;
	info->vf_num = vf_num;

	sss_notify_sriov_state_change(hwdev, vf_num);

	clear_bit(SSS_SRIOV_ENABLE, &info->state);

	return vf_num;
#else

	return 0;
#endif
}

int sss_pci_configure_sriov(struct pci_dev *pdev, int vf_num)
{
	struct sss_pci_adapter *adapter = sss_get_adapter_by_pcidev(pdev);

	if (!adapter)
		return -EFAULT;

	if (!test_bit(SSS_SRIOV_PRESENT, &adapter->sriov_info.state))
		return -EFAULT;

	return (vf_num == 0) ? sss_pci_disable_sriov(adapter) :
	       sss_pci_enable_sriov(adapter, (u16)vf_num);
}
