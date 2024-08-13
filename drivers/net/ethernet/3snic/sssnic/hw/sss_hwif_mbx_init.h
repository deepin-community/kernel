/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWIF_MBX_INIT_H
#define SSS_HWIF_MBX_INIT_H

#include "sss_hwdev.h"

int sss_init_func_mbx_msg(void *hwdev, u16 func_num);
int sss_hwif_init_mbx(struct sss_hwdev *hwdev);
void sss_hwif_deinit_mbx(struct sss_hwdev *hwdev);
void sss_recv_mbx_aeq_handler(void *handle, u8 *header, u8 size);

#endif
