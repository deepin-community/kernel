/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_COMM_H
#define HINIC3_COMM_H

#include <linux/types.h>

#include "mpu_inband_cmd_defs.h"
#include "hinic3_hwdev.h"

#define MSG_TO_MGMT_SYNC_RETURN_ERR(err, out_size, status)	\
		((err) || (status) || !(out_size))

#define HINIC3_PAGE_SIZE_HW(pg_size)	((u8)ilog2((u32)((pg_size) >> 12)))

enum func_tmr_bitmap_status {
	FUNC_TMR_BITMAP_DISABLE,
	FUNC_TMR_BITMAP_ENABLE,
};

enum ppf_tmr_status {
	HINIC_PPF_TMR_FLAG_STOP,
	HINIC_PPF_TMR_FLAG_START,
};

#define HINIC3_HT_GPA_PAGE_SIZE 4096UL
#define HINIC3_PPF_HT_GPA_SET_RETRY_TIMES 10

int hinic3_set_cmdq_depth(void *hwdev, u16 cmdq_depth);

int hinic3_set_enhance_cmdq_ctxt(struct hinic3_hwdev *hwdev, u8 cmdq_id,
				 struct enhance_cmdq_ctxt_info *ctxt);

int hinic3_set_cmdq_ctxt(struct hinic3_hwdev *hwdev, u8 cmdq_id,
			 struct cmdq_ctxt_info *ctxt);

int hinic3_ppf_ext_db_init(struct hinic3_hwdev *hwdev);

int hinic3_ppf_ext_db_deinit(struct hinic3_hwdev *hwdev);

int hinic3_set_ceq_ctrl_reg(struct hinic3_hwdev *hwdev, u16 q_id,
			    u32 ctrl0, u32 ctrl1);

int hinic3_set_dma_attr_tbl(struct hinic3_hwdev *hwdev, u8 entry_idx, u8 st, u8 at, u8 ph,
			    u8 no_snooping, u8 tph_en);

int hinic3_get_comm_features(void *hwdev, u64 *s_feature, u16 size);
int hinic3_set_comm_features(void *hwdev, u64 *s_feature, u16 size);

int hinic3_comm_channel_detect(struct hinic3_hwdev *hwdev);

int hinic3_get_global_attr(void *hwdev, struct comm_global_attr *attr);
#endif
