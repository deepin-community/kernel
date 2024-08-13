/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_MGMT_CHANNEL_H
#define SSS_MGMT_CHANNEL_H

#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/completion.h>

#include "sss_hw_mbx.h"
#include "sss_hw_mgmt.h"
#include "sss_adm_info.h"

/* message header define */
#define SSS_MSG_HEADER_SRC_GLB_FUNC_ID_SHIFT	0
#define SSS_MSG_HEADER_STATUS_SHIFT				13
#define SSS_MSG_HEADER_SOURCE_SHIFT				15
#define SSS_MSG_HEADER_AEQ_ID_SHIFT				16
#define SSS_MSG_HEADER_MSG_ID_SHIFT				18
#define SSS_MSG_HEADER_CMD_SHIFT				22

#define SSS_MSG_HEADER_MSG_LEN_SHIFT			32
#define SSS_MSG_HEADER_MODULE_SHIFT				43
#define SSS_MSG_HEADER_SEG_LEN_SHIFT			48
#define SSS_MSG_HEADER_NO_ACK_SHIFT				54
#define SSS_MSG_HEADER_DATA_TYPE_SHIFT			55
#define SSS_MSG_HEADER_SEQID_SHIFT				56
#define SSS_MSG_HEADER_LAST_SHIFT				62
#define SSS_MSG_HEADER_DIRECTION_SHIFT			63

#define SSS_MSG_HEADER_SRC_GLB_FUNC_ID_MASK		0x1FFF
#define SSS_MSG_HEADER_STATUS_MASK				0x1
#define SSS_MSG_HEADER_SOURCE_MASK				0x1
#define SSS_MSG_HEADER_AEQ_ID_MASK				0x3
#define SSS_MSG_HEADER_MSG_ID_MASK				0xF
#define SSS_MSG_HEADER_CMD_MASK					0x3FF

#define SSS_MSG_HEADER_MSG_LEN_MASK				0x7FF
#define SSS_MSG_HEADER_MODULE_MASK				0x1F
#define SSS_MSG_HEADER_SEG_LEN_MASK				0x3F
#define SSS_MSG_HEADER_NO_ACK_MASK				0x1
#define SSS_MSG_HEADER_DATA_TYPE_MASK			0x1
#define SSS_MSG_HEADER_SEQID_MASK				0x3F
#define SSS_MSG_HEADER_LAST_MASK				0x1
#define SSS_MSG_HEADER_DIRECTION_MASK			0x1

#define SSS_GET_MSG_HEADER(val, field)	\
		(((val) >> SSS_MSG_HEADER_##field##_SHIFT) & \
		SSS_MSG_HEADER_##field##_MASK)
#define SSS_SET_MSG_HEADER(val, field)	\
		((u64)(((u64)(val)) & SSS_MSG_HEADER_##field##_MASK) << \
		SSS_MSG_HEADER_##field##_SHIFT)

enum sss_msg_ack_type {
	SSS_MSG_ACK,
	SSS_MSG_NO_ACK,
};

enum sss_data_type {
	SSS_INLINE_DATA	= 0,
	SSS_DMA_DATA	= 1,
};

enum sss_msg_seg_type {
	SSS_NOT_LAST_SEG	= 0,
	SSS_LAST_SEG		= 1,
};

enum sss_msg_direction_type {
	SSS_DIRECT_SEND_MSG	= 0,
	SSS_RESP_MSG	= 1,
};

enum sss_msg_src_type {
	SSS_MSG_SRC_MGMT	= 0,
	SSS_MSG_SRC_MBX	= 1,
};

enum sss_mgmt_msg_cb_t_state {
	SSS_CALLBACK_REG = 0,
	SSS_CALLBACK_RUNNING,
};

enum sss_pf_to_mgmt_event_state {
	SSS_ADM_EVENT_UNINIT = 0,
	SSS_ADM_EVENT_START,
	SSS_ADM_EVENT_SUCCESS,
	SSS_ADM_EVENT_FAIL,
	SSS_ADM_EVENT_TIMEOUT,
	SSS_ADM_EVENT_END,
};

struct sss_recv_msg {
	void		*buf;

	u16			buf_len;
	u16			cmd;

	u16			msg_id;
	u8			seq_id;
	u8			no_ack;

	enum sss_mod_type	mod;

	struct completion	done;
};

struct sss_msg_pf_to_mgmt {
	void			*hwdev;
	spinlock_t		async_msg_lock; /* protect msg async and sync */

	struct semaphore	sync_lock;

	struct workqueue_struct	*workq;

	void	*async_msg_buf;
	void	*sync_buf;
	void	*ack_buf;

	struct sss_recv_msg		recv_msg;
	struct sss_recv_msg		recv_resp_msg;

	u16				rsvd;
	u16				async_msg_id;
	u16				sync_msg_id;
	struct sss_adm_msg		*adm_msg[SSS_ADM_MSG_MAX];

	sss_mgmt_msg_handler_t		recv_handler[SSS_MOD_TYPE_HW_MAX];
	void				*recv_data[SSS_MOD_TYPE_HW_MAX];
	unsigned long			recv_handler_state[SSS_MOD_TYPE_HW_MAX];
	void				*async_msg_cb_data[SSS_MOD_TYPE_HW_MAX];

	/* lock when sending msg */
	spinlock_t			sync_event_lock; /* protect event async and sync */
	enum sss_pf_to_mgmt_event_state	event_state;
};

#endif
