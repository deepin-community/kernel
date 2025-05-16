// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt

#include <linux/pci.h>
#include <linux/interrupt.h>

#include "ossl_knl.h"
#include "hinic3_crm.h"
#include "hinic3_hw.h"
#include "hinic3_lld.h"
#include "hinic3_dev_mgmt.h"
#include "hinic3_sriov.h"

static int hinic3_init_vf_hw(void *hwdev, u16 start_vf_id, u16 end_vf_id)
{
	u16 i, func_idx;
	int err;

	/* mbox msg channel resources will be freed during remove process */
	err = hinic3_init_func_mbox_msg_channel(hwdev,
						hinic3_func_max_vf(hwdev));
	if (err != 0)
		return err;

	/* vf use 256K as default wq page size, and can't change it */
	for (i = start_vf_id; i <= end_vf_id; i++) {
		func_idx = hinic3_glb_pf_vf_offset(hwdev) + i;
		err = hinic3_set_wq_page_size(hwdev, func_idx,
					      HINIC3_DEFAULT_WQ_PAGE_SIZE,
					      HINIC3_CHANNEL_COMM);
		if (err)
			return err;
	}

	return 0;
}

static int hinic3_deinit_vf_hw(void *hwdev, u16 start_vf_id, u16 end_vf_id)
{
	u16 func_idx, idx;

	for (idx = start_vf_id; idx <= end_vf_id; idx++) {
		func_idx = hinic3_glb_pf_vf_offset(hwdev) + idx;
		hinic3_set_wq_page_size(hwdev, func_idx,
					HINIC3_HW_WQ_PAGE_SIZE,
					HINIC3_CHANNEL_COMM);
	}

	return 0;
}

#if !(defined(HAVE_SRIOV_CONFIGURE) || defined(HAVE_RHEL6_SRIOV_CONFIGURE))
ssize_t sriov_totalvfs_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	struct pci_dev *pdev = to_pci_dev(dev);

	return sprintf(buf, "%d\n", pci_sriov_get_totalvfs(pdev));
}

ssize_t sriov_numvfs_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	struct pci_dev *pdev = to_pci_dev(dev);

	return sprintf(buf, "%d\n", pci_num_vf(pdev));
}

ssize_t sriov_numvfs_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	int ret;
	u16 num_vfs;
	int cur_vfs, total_vfs;

	ret = kstrtou16(buf, 0, &num_vfs);
	if (ret < 0)
		return ret;

	cur_vfs = pci_num_vf(pdev);
	total_vfs = pci_sriov_get_totalvfs(pdev);
	if (num_vfs > total_vfs)
		return -ERANGE;

	if (num_vfs == cur_vfs)
		return count;    /* no change */

	if (num_vfs == 0) {
		/* disable VFs */
		ret = hinic3_pci_sriov_configure(pdev, 0);
		if (ret < 0)
			return ret;
		return count;
	}

	/* enable VFs */
	if (cur_vfs) {
		nic_warn(&pdev->dev, "%d VFs already enabled. Disable before enabling %d VFs\n",
			 cur_vfs, num_vfs);
		return -EBUSY;
	}

	ret = hinic3_pci_sriov_configure(pdev, num_vfs);
	if (ret < 0)
		return ret;

	if (ret != num_vfs)
		nic_warn(&pdev->dev, "%d VFs requested; only %d enabled\n",
			 num_vfs, ret);

	return count;
}

#endif /* !(HAVE_SRIOV_CONFIGURE || HAVE_RHEL6_SRIOV_CONFIGURE) */

int hinic3_pci_sriov_disable(struct pci_dev *dev)
{
#ifdef CONFIG_PCI_IOV
	struct hinic3_sriov_info *sriov_info = NULL;
	struct hinic3_event_info event = {0};
	void *hwdev = NULL;
	u16 tmp_vfs;

	sriov_info = hinic3_get_sriov_info_by_pcidev(dev);
	hwdev = hinic3_get_hwdev_by_pcidev(dev);
	if (!hwdev) {
		sdk_err(&dev->dev, "SR-IOV disable is not permitted, please wait...\n");
		return -EPERM;
	}

	/* if SR-IOV is already disabled then there is nothing to do */
	if (!sriov_info->sriov_enabled)
		return 0;

	if (test_and_set_bit(HINIC3_SRIOV_DISABLE, &sriov_info->state)) {
		sdk_err(&dev->dev, "SR-IOV disable in process, please wait");
		return -EPERM;
	}

	/* If our VFs are assigned we cannot shut down SR-IOV
	 * without causing issues, so just leave the hardware
	 * available but disabled
	 */
	if (pci_vfs_assigned(dev)) {
		clear_bit(HINIC3_SRIOV_DISABLE, &sriov_info->state);
		sdk_warn(&dev->dev, "Unloading driver while VFs are assigned - VFs will not be deallocated\n");
		return -EPERM;
	}

	event.service = EVENT_SRV_COMM;
	event.type = EVENT_COMM_SRIOV_STATE_CHANGE;
	((struct hinic3_sriov_state_info *)(void *)event.event_data)->enable = 0;
	hinic3_event_callback(hwdev, &event);

	sriov_info->sriov_enabled = false;

	/* disable iov and allow time for transactions to clear */
	pci_disable_sriov(dev);

	tmp_vfs = (u16)sriov_info->num_vfs;
	sriov_info->num_vfs = 0;
	hinic3_deinit_vf_hw(hwdev, 1, tmp_vfs);

	clear_bit(HINIC3_SRIOV_DISABLE, &sriov_info->state);

#endif

	return 0;
}

#ifdef CONFIG_PCI_IOV
int hinic3_pci_sriov_check(struct hinic3_sriov_info *sriov_info, struct pci_dev *dev, int num_vfs)
{
	int pre_existing_vfs;
	int err;

	if (test_and_set_bit(HINIC3_SRIOV_ENABLE, &sriov_info->state)) {
		sdk_err(&dev->dev,
			"SR-IOV enable in process, please wait, num_vfs %d\n",
			num_vfs);
		return -EPERM;
	}

	pre_existing_vfs = pci_num_vf(dev);

	if (num_vfs > pci_sriov_get_totalvfs(dev)) {
		clear_bit(HINIC3_SRIOV_ENABLE, &sriov_info->state);
		return -ERANGE;
	}

	if (pre_existing_vfs && pre_existing_vfs != num_vfs) {
		err = hinic3_pci_sriov_disable(dev);
		if (err) {
			clear_bit(HINIC3_SRIOV_ENABLE, &sriov_info->state);
			return err;
		}
	} else if (pre_existing_vfs == num_vfs) {
		clear_bit(HINIC3_SRIOV_ENABLE, &sriov_info->state);
		return num_vfs;
	}

	return 0;
}
#endif

int hinic3_pci_sriov_enable(struct pci_dev *dev, int num_vfs)
{
#ifdef CONFIG_PCI_IOV
	struct hinic3_sriov_info *sriov_info = NULL;
	struct hinic3_event_info event = {0};
	void *hwdev = NULL;
	int err = 0;

	sriov_info = hinic3_get_sriov_info_by_pcidev(dev);
	hwdev = hinic3_get_hwdev_by_pcidev(dev);
	if (!hwdev) {
		sdk_err(&dev->dev, "SR-IOV enable is not permitted, please wait...\n");
		return -EPERM;
	}

	err = hinic3_pci_sriov_check(sriov_info, dev, num_vfs);
	if (err != 0)
		return err;

	err = hinic3_init_vf_hw(hwdev, 1, (u16)num_vfs);
	if (err) {
		sdk_err(&dev->dev, "Failed to init vf in hardware before enable sriov, error %d\n",
			err);
		clear_bit(HINIC3_SRIOV_ENABLE, &sriov_info->state);
		return err;
	}

	err = pci_enable_sriov(dev, num_vfs);
	if (err) {
		sdk_err(&dev->dev, "Failed to enable SR-IOV, error %d\n", err);
		clear_bit(HINIC3_SRIOV_ENABLE, &sriov_info->state);
		return err;
	}

	sriov_info->sriov_enabled = true;
	sriov_info->num_vfs = num_vfs;

	event.service = EVENT_SRV_COMM;
	event.type = EVENT_COMM_SRIOV_STATE_CHANGE;
	((struct hinic3_sriov_state_info *)(void *)event.event_data)->enable = 1;
	((struct hinic3_sriov_state_info *)(void *)event.event_data)->num_vfs = (u16)num_vfs;
	hinic3_event_callback(hwdev, &event);

	clear_bit(HINIC3_SRIOV_ENABLE, &sriov_info->state);

	return num_vfs;
#else

	return 0;
#endif
}

int hinic3_pci_sriov_configure(struct pci_dev *dev, int num_vfs)
{
	struct hinic3_sriov_info *sriov_info = NULL;

	sriov_info = hinic3_get_sriov_info_by_pcidev(dev);
	if (!sriov_info)
		return -EFAULT;

	if (!test_bit(HINIC3_FUNC_PERSENT, &sriov_info->state))
		return -EFAULT;

	if (num_vfs == 0)
		return hinic3_pci_sriov_disable(dev);
	else
		return hinic3_pci_sriov_enable(dev, num_vfs);
}
