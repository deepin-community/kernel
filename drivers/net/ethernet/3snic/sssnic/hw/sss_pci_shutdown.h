/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_PCI_SHUTDOWN_H
#define SSS_PCI_SHUTDOWN_H

#include <linux/pci.h>

void sss_pci_shutdown(struct pci_dev *pdev);
#endif
