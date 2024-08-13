/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWIF_CTRLQ_H
#define SSS_HWIF_CTRLQ_H

#include "sss_hw_wq.h"

#define	SSS_CTRLQ_BUF_LEN					2048U

#define SSS_CTRLQ_SEND_CMPT_CODE			10

#define SSS_CTRLQ_FORCE_STOP_CMPT_CODE		12

#define SSS_WQEBB_NUM_FOR_CTRLQ			1

enum sss_ctrlq_state {
	SSS_CTRLQ_ENABLE = BIT(0),
};

void *sss_ctrlq_read_wqe(struct sss_wq *wq, u16 *ci);
void sss_ctrlq_ceq_handler(void *handle, u32 ceqe_data);
void sss_free_ctrlq_cmd_buf(struct sss_hwdev *hwdev,
			    struct sss_ctrlq_cmd_info *cmd_info);
int sss_ctrlq_sync_cmd_direct_reply(struct sss_ctrlq *ctrlq, u8 mod,
				    u8 cmd, struct sss_ctrl_msg_buf *in_buf,
				    u64 *out_param, u32 timeout, u16 channel);
int sss_ctrlq_sync_cmd_detail_reply(struct sss_ctrlq *ctrlq, u8 mod, u8 cmd,
				    struct sss_ctrl_msg_buf *in_buf,
				    struct sss_ctrl_msg_buf *out_buf,
				    u64 *out_param, u32 timeout, u16 channel);

#endif
