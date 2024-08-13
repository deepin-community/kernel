/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWDEV_INIT_H
#define SSS_HWDEV_INIT_H

#include "sss_adapter.h"

int sss_init_hwdev(struct sss_pci_adapter *adapter);
void sss_deinit_hwdev(void *hwdev);
void sss_hwdev_detach(void *hwdev);
void sss_hwdev_stop(void *hwdev);
void sss_hwdev_shutdown(void *hwdev);

#endif
