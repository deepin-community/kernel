/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_MBX_INFO_H
#define SSS_MBX_INFO_H
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>

#include "sss_hw_mbx.h"

enum sss_mbx_event_state {
	SSS_EVENT_START = 0,
	SSS_EVENT_FAIL,
	SSS_EVENT_SUCCESS,
	SSS_EVENT_TIMEOUT,
	SSS_EVENT_END,
};

struct sss_mbx_send {
	u8			*data;

	u64			*wb_state; /* write back status */
	void		*wb_vaddr;
	dma_addr_t	wb_paddr;
};

struct sss_mbx_dma_queue {
	void		*dma_buff_vaddr;
	dma_addr_t	dma_buff_paddr;

	u16			depth;
	u16			pi;
	u16			ci;
};

struct sss_mbx_msg_info {
	u8 msg_id;
	u8 state; /* can only use 1 bit */
};

struct sss_msg_desc {
	void		*msg;
	u16			msg_len;
	u8			seq_id;
	u8			mod;
	u16			cmd;
	struct sss_mbx_msg_info	msg_info;
};

struct sss_msg_buffer {
	struct sss_msg_desc		resp_msg;
	struct sss_msg_desc		recv_msg;

	atomic_t				recv_msg_cnt;
};

struct sss_mbx {
	void						*hwdev;

	u8							lock_channel_en;
	u8							rsvd0[3];
	unsigned long				channel_stop;

	/* lock for send mbx message and ack message */
	struct mutex				mbx_send_lock;
	/* lock for send mbx message */
	struct mutex				msg_send_lock;
	struct sss_mbx_send		mbx_send;

	struct sss_mbx_dma_queue	sync_msg_queue;
	struct sss_mbx_dma_queue	async_msg_queue;

	struct workqueue_struct		*workq;

	struct sss_msg_buffer		mgmt_msg; /* driver and MGMT CPU */
	struct sss_msg_buffer		*host_msg; /* PPF message between hosts */
	struct sss_msg_buffer		*func_msg; /* PF to VF or VF to PF */
	u16							num_func_msg;
	u16							cur_msg_channel;
	u8							support_h2h_msg; /* host to host */
	u8							rsvd1[3];
	/* vf receive pf/ppf callback */
	sss_vf_mbx_handler_t			vf_mbx_cb[SSS_MOD_TYPE_MAX];
	void						*vf_mbx_data[SSS_MOD_TYPE_MAX];
	/* pf/ppf receive vf callback */
	sss_pf_mbx_handler_t			pf_mbx_cb[SSS_MOD_TYPE_MAX];
	void						*pf_mbx_data[SSS_MOD_TYPE_MAX];
	/* ppf receive pf/ppf callback */
	sss_ppf_mbx_handler_t			ppf_mbx_cb[SSS_MOD_TYPE_MAX];
	void						*ppf_mbx_data[SSS_MOD_TYPE_MAX];
	/* pf receive ppf callback */
	sss_pf_from_ppf_mbx_handler_t	pf_recv_ppf_mbx_cb[SSS_MOD_TYPE_MAX];
	void						*pf_recv_ppf_mbx_data[SSS_MOD_TYPE_MAX];
	unsigned long				ppf_to_pf_mbx_cb_state[SSS_MOD_TYPE_MAX];
	unsigned long				ppf_mbx_cb_state[SSS_MOD_TYPE_MAX];
	unsigned long				pf_mbx_cb_state[SSS_MOD_TYPE_MAX];
	unsigned long				vf_mbx_cb_state[SSS_MOD_TYPE_MAX];

	enum sss_mbx_event_state	event_flag;
	/* lock for mbx event flag */
	spinlock_t					mbx_lock;

	u8							send_msg_id;
	u8							rsvd2[3];
};

#endif
