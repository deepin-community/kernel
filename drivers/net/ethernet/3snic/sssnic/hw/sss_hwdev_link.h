/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWDEV_LINK_H
#define SSS_HWDEV_LINK_H

#include "sss_kernel.h"
#include "sss_hwdev.h"
#include "sss_hw_mbx_msg.h"

int sss_init_devlink(struct sss_hwdev *hwdev);
void sss_deinit_devlink(struct sss_hwdev *hwdev);

#endif
