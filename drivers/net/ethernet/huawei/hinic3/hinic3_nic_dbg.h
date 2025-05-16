/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_NIC_DBG_H
#define HINIC3_NIC_DBG_H

#include "hinic3_mt.h"
#include "hinic3_nic_io.h"
#include "hinic3_srv_nic.h"

int hinic3_dbg_get_sq_info(void *hwdev, u16 q_id, struct nic_sq_info *sq_info,
			   u32 msg_size);

int hinic3_dbg_get_rq_info(void *hwdev, u16 q_id, struct nic_rq_info *rq_info,
			   u32 msg_size);

int hinic3_dbg_get_wqe_info(void *hwdev, u16 q_id, u16 idx, u16 wqebb_cnt,
			    u8 *wqe, const u16 *wqe_size,
			    enum hinic3_queue_type q_type);

#endif
