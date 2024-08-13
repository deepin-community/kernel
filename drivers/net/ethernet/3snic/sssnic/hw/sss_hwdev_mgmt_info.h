/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWDEV_MGMT_INFO_H
#define SSS_HWDEV_MGMT_INFO_H

#include "sss_hwdev.h"

int sss_init_mgmt_info(struct sss_hwdev *dev);
void sss_deinit_mgmt_info(struct sss_hwdev *dev);

#endif
