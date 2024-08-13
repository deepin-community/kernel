/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWIF_MGMT_INIT_H
#define SSS_HWIF_MGMT_INIT_H

#include "sss_hwdev.h"

void sss_mgmt_msg_aeqe_handler(void *hwdev, u8 *header, u8 size);
void sss_force_complete_all(void *dev);
void sss_flush_mgmt_workq(void *hwdev);

#endif
