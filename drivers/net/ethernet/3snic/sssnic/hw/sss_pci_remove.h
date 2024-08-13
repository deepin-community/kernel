/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_PCI_REMOVE_H
#define SSS_PCI_REMOVE_H

#include <linux/pci.h>

#include "sss_hw_svc_cap.h"
#include "sss_adapter.h"

void sss_detach_uld_driver(struct sss_pci_adapter *adapter, enum sss_service_type type);
void sss_detach_all_uld_driver(struct sss_pci_adapter *adapter);
void sss_dettach_uld_dev(struct sss_pci_adapter *adapter);
void sss_deinit_function(struct pci_dev *pdev);
void sss_unmap_pci_bar(struct sss_pci_adapter *adapter);
int sss_deinit_adapter(struct sss_pci_adapter *adapter);
void sss_deinit_pci_dev(struct pci_dev *pdev);

void sss_pci_remove(struct pci_dev *pdev);
#endif
