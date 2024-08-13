/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWIF_MBX_H
#define SSS_HWIF_MBX_H

#include "sss_hw.h"
#include "sss_hwdev.h"

#define SSS_MGMT_SRC_ID					0x1FFF

#define SSS_IS_DMA_MBX_MSG(dest_func_id)	((dest_func_id) == SSS_MGMT_SRC_ID)

#define SSS_MBX_BUF_SIZE_MAX	2048U

#define SSS_MBX_HEADER_SIZE		8

/* MBX size is 64B, 8B for mbx_header, 8B reserved */
#define SSS_MBX_SEG_SIZE		48
#define SSS_MBX_DATA_SIZE		(SSS_MBX_BUF_SIZE_MAX - SSS_MBX_HEADER_SIZE)

#define SSS_MBX_MQ_CI_OFF	(SSS_CSR_CFG_FLAG + \
			SSS_HW_CSR_MBX_DATA_OFF + SSS_MBX_HEADER_SIZE + SSS_MBX_SEG_SIZE)

#define SSS_MBX_MQ_SYNC_CI_SHIFT		0
#define SSS_MBX_MQ_ASYNC_CI_SHIFT		8

#define SSS_MBX_MQ_SYNC_CI_MASK		0xFF
#define SSS_MBX_MQ_ASYNC_CI_MASK		0xFF

#define SSS_GET_MBX_MQ_CI(val, field)	\
		(((val) >> SSS_MBX_MQ_##field##_CI_SHIFT) & SSS_MBX_MQ_##field##_CI_MASK)
#define SSS_CLEAR_MBX_MQ_CI(val, field)		\
		((val) & (~(SSS_MBX_MQ_##field##_CI_MASK << SSS_MBX_MQ_##field##_CI_SHIFT)))

/* Recv func mbx msg */
struct sss_recv_mbx {
	void		*buf;
	u16			buf_len;
	u8			msg_id;
	u8			mod;
	u16			cmd;
	u16			src_func_id;
	enum sss_msg_ack_type ack_type;
	void		*resp_buf;
};

enum sss_mbx_cb_state {
	SSS_VF_RECV_HANDLER_REG = 0,
	SSS_VF_RECV_HANDLER_RUN,
	SSS_PF_RECV_HANDLER_REG,
	SSS_PF_RECV_HANDLER_RUN,
	SSS_PPF_RECV_HANDLER_REG,
	SSS_PPF_RECV_HANDLER_RUN,
	SSS_PPF_TO_PF_RECV_HANDLER_REG,
	SSS_PPF_TO_PF_RECV_HANDLER_RUN,
};

static inline int sss_check_mbx_param(struct sss_mbx *mbx,
				      void *buf_in, u16 in_size, u16 channel)
{
	if (!buf_in || in_size == 0)
		return -EINVAL;

	if (in_size > SSS_MBX_DATA_SIZE) {
		sdk_err(SSS_TO_HWDEV(mbx)->dev_hdl,
			"Mbx msg len %u exceed limit: [1, %u]\n",
			in_size, SSS_MBX_DATA_SIZE);
		return -EINVAL;
	}

	if (channel >= SSS_CHANNEL_MAX) {
		sdk_err(SSS_TO_HWDEV(mbx)->dev_hdl,
			"Invalid channel id: 0x%x\n", channel);
		return -EINVAL;
	}

	return 0;
}

struct sss_msg_desc *sss_get_mbx_msg_desc(struct sss_mbx *mbx, u64 src_func_id, u64 direction);
int sss_send_mbx_msg(struct sss_mbx *mbx, u8 mod, u16 cmd,
		     void *msg, u16 msg_len, u16 dest, enum sss_msg_direction_type direction_type,
		     enum sss_msg_ack_type type, struct sss_mbx_msg_info *msg_info);
int sss_send_mbx_to_func(struct sss_mbx *mbx, u8 mod, u16 cmd,
			 u16 dest_func_id, void *buf_in, u16 in_size,
			 void *buf_out, u16 *out_size, u32 timeout, u16 channel);
int sss_send_mbx_to_func_no_ack(struct sss_hwdev *hwdev, u16 func_id,
				u8 mod, u16 cmd, void *buf_in, u16 in_size, u16 channel);
#define sss_send_mbx_to_mgmt_no_ack(hwdev, mod, cmd, buf_in, in_size, channel) \
			sss_send_mbx_to_func_no_ack(hwdev, SSS_MGMT_SRC_ID, mod, cmd, \
				buf_in, in_size, channel)

#endif
