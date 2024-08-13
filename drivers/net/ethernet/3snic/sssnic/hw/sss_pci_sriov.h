/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_PCI_SRIOV_H
#define SSS_PCI_SRIOV_H

#include <linux/types.h>
#include <linux/pci.h>

#include "sss_sriov_info.h"
#include "sss_adapter.h"

int sss_pci_disable_sriov(struct sss_pci_adapter *adapter);

int sss_pci_configure_sriov(struct pci_dev *pdev, int num_vfs);
#endif
