/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWDEV_API_H
#define SSS_HWDEV_API_H

#include <linux/types.h>

#include "sss_hw_mbx_msg.h"
#include "sss_hwdev.h"

int sss_chip_sync_time(void *hwdev, u64 mstime);
int sss_chip_get_board_info(void *hwdev, struct sss_board_info *board_info);
void sss_chip_disable_mgmt_channel(void *hwdev);
int sss_chip_do_nego_feature(void *hwdev, u8 opcode, u64 *feature, u16 feature_num);
int sss_chip_set_pci_bdf_num(void *hwdev, u8 bus_id, u8 device_id, u8 func_id);
int sss_chip_comm_channel_detect(struct sss_hwdev *hwdev);

#endif
