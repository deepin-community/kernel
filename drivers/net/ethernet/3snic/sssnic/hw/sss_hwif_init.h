/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWIF_INIT_H
#define SSS_HWIF_INIT_H

#include "sss_hwdev.h"
#include "sss_adapter.h"

int sss_hwif_init(struct sss_pci_adapter *adapter);
void sss_hwif_deinit(struct sss_hwdev *hwdev);

#endif
