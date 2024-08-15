/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_PCI_ERROR_H
#define SSS_PCI_ERROR_H

#include <linux/pci.h>

pci_ers_result_t sss_detect_pci_error(struct pci_dev *pdev,
				      pci_channel_state_t state);
#endif
