/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_PCI_PROBE_H
#define SSS_PCI_PROBE_H

#include <linux/pci.h>

#include "sss_adapter.h"

int sss_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id);
#endif
