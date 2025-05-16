/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_SRIOV_H
#define HINIC3_SRIOV_H
#include <linux/types.h>
#include <linux/pci.h>

#if !(defined(HAVE_SRIOV_CONFIGURE) || defined(HAVE_RHEL6_SRIOV_CONFIGURE))
ssize_t sriov_totalvfs_show(struct device *dev,
			    struct device_attribute *attr, char *buf);
ssize_t sriov_numvfs_show(struct device *dev,
			  struct device_attribute *attr, char *buf);
ssize_t sriov_numvfs_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count);
#endif /* !(HAVE_SRIOV_CONFIGURE || HAVE_RHEL6_SRIOV_CONFIGURE) */

enum hinic3_sriov_state {
	HINIC3_SRIOV_DISABLE,
	HINIC3_SRIOV_ENABLE,
	HINIC3_FUNC_PERSENT,
};

struct hinic3_sriov_info {
	bool sriov_enabled;
	unsigned int num_vfs;
	unsigned long state;
};

struct hinic3_sriov_info *hinic3_get_sriov_info_by_pcidev(struct pci_dev *pdev);
int hinic3_pci_sriov_disable(struct pci_dev *dev);
int hinic3_pci_sriov_enable(struct pci_dev *dev, int num_vfs);
int hinic3_pci_sriov_configure(struct pci_dev *dev, int num_vfs);
#endif
