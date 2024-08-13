/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HW_CTRLQ_H
#define SSS_HW_CTRLQ_H

#include <linux/types.h>
#include <linux/atomic.h>

struct sss_ctrl_msg_buf {
	void		*buf;
	dma_addr_t	dma_addr;
	u16			size;

	/* Usage count, USERS DO NOT USE */
	atomic_t	ref_cnt;
};

/**
 * @brief sss_alloc_ctrlq_msg_buf - alloc ctrlq msg buffer
 * @param hwdev: device pointer to hwdev
 * @retval non-zero: success
 * @retval null: failure
 **/
struct sss_ctrl_msg_buf *sss_alloc_ctrlq_msg_buf(void *hwdev);

/**
 * @brief sss_free_ctrlq_msg_buf - free ctrlq msg buffer
 * @param hwdev: device pointer to hwdev
 * @param msg_buf: buffer to free
 **/
void sss_free_ctrlq_msg_buf(void *hwdev, struct sss_ctrl_msg_buf *msg_buf);

/**
 * @brief sss_ctrlq_direct_reply - ctrlq direct message response
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param in_buf: message buffer in
 * @param out_param: message out
 * @param timeout: timeout
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int sss_ctrlq_direct_reply(void *hwdev, u8 mod, u8 cmd,
			   struct sss_ctrl_msg_buf *in_buf,
			   u64 *out_param, u32 timeout, u16 channel);

/**
 * @brief sss_ctrlq_detail_reply - ctrlq detail message response
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param in_buf: message buffer in
 * @param out_buf: message buffer out
 * @param out_param: inline output data
 * @param timeout: timeout
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int sss_ctrlq_detail_reply(void *hwdev, u8 mod, u8 cmd,
			   struct sss_ctrl_msg_buf *in_buf, struct sss_ctrl_msg_buf *out_buf,
			   u64 *out_param, u32 timeout, u16 channel);

#endif
