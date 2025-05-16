/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_MBOX_H
#define HINIC3_MBOX_H

#include "hinic3_crm.h"
#include "hinic3_hwdev.h"

#define HINIC3_MBOX_PF_SEND_ERR				0x1

#define HINIC3_MGMT_SRC_ID				0x1FFF
#define HINIC3_MAX_FUNCTIONS				4096

/* message header define */
#define HINIC3_MSG_HEADER_SRC_GLB_FUNC_IDX_SHIFT	0
#define HINIC3_MSG_HEADER_STATUS_SHIFT			13
#define HINIC3_MSG_HEADER_SOURCE_SHIFT			15
#define HINIC3_MSG_HEADER_AEQ_ID_SHIFT			16
#define HINIC3_MSG_HEADER_MSG_ID_SHIFT			18
#define HINIC3_MSG_HEADER_CMD_SHIFT			22

#define HINIC3_MSG_HEADER_MSG_LEN_SHIFT			32
#define HINIC3_MSG_HEADER_MODULE_SHIFT			43
#define HINIC3_MSG_HEADER_SEG_LEN_SHIFT			48
#define HINIC3_MSG_HEADER_NO_ACK_SHIFT			54
#define HINIC3_MSG_HEADER_DATA_TYPE_SHIFT		55
#define HINIC3_MSG_HEADER_SEQID_SHIFT			56
#define HINIC3_MSG_HEADER_LAST_SHIFT			62
#define HINIC3_MSG_HEADER_DIRECTION_SHIFT		63

#define HINIC3_MSG_HEADER_SRC_GLB_FUNC_IDX_MASK		0x1FFF
#define HINIC3_MSG_HEADER_STATUS_MASK			0x1
#define HINIC3_MSG_HEADER_SOURCE_MASK			0x1
#define HINIC3_MSG_HEADER_AEQ_ID_MASK			0x3
#define HINIC3_MSG_HEADER_MSG_ID_MASK			0xF
#define HINIC3_MSG_HEADER_CMD_MASK			0x3FF

#define HINIC3_MSG_HEADER_MSG_LEN_MASK			0x7FF
#define HINIC3_MSG_HEADER_MODULE_MASK			0x1F
#define HINIC3_MSG_HEADER_SEG_LEN_MASK			0x3F
#define HINIC3_MSG_HEADER_NO_ACK_MASK			0x1
#define HINIC3_MSG_HEADER_DATA_TYPE_MASK		0x1
#define HINIC3_MSG_HEADER_SEQID_MASK			0x3F
#define HINIC3_MSG_HEADER_LAST_MASK			0x1
#define HINIC3_MSG_HEADER_DIRECTION_MASK		0x1

#define MBOX_MAX_BUF_SZ				2048U
#define MBOX_HEADER_SZ				8
#define HINIC3_MBOX_DATA_SIZE		(MBOX_MAX_BUF_SZ - MBOX_HEADER_SZ)

#define HINIC3_MSG_HEADER_GET(val, field)	\
		(((val) >> HINIC3_MSG_HEADER_##field##_SHIFT) & \
		 HINIC3_MSG_HEADER_##field##_MASK)
#define HINIC3_MSG_HEADER_SET(val, field)	\
		((u64)(((u64)(val)) & HINIC3_MSG_HEADER_##field##_MASK) << \
		 HINIC3_MSG_HEADER_##field##_SHIFT)

#define IS_DMA_MBX_MSG(dst_func)	((dst_func) == HINIC3_MGMT_SRC_ID)

enum hinic3_msg_direction_type {
	HINIC3_MSG_DIRECT_SEND	= 0,
	HINIC3_MSG_RESPONSE	= 1,
};

enum hinic3_msg_segment_type {
	NOT_LAST_SEGMENT	= 0,
	LAST_SEGMENT		= 1,
};

enum hinic3_msg_ack_type {
	HINIC3_MSG_ACK,
	HINIC3_MSG_NO_ACK,
};

enum hinic3_data_type {
	HINIC3_DATA_INLINE	= 0,
	HINIC3_DATA_DMA		= 1,
};

enum hinic3_msg_src_type {
	HINIC3_MSG_FROM_MGMT	= 0,
	HINIC3_MSG_FROM_MBOX	= 1,
};

enum hinic3_msg_aeq_type {
	HINIC3_ASYNC_MSG_AEQ	= 0,
	/* indicate dest func or mgmt cpu which aeq to response mbox message */
	HINIC3_MBOX_RSP_MSG_AEQ	= 1,
	/* indicate mgmt cpu which aeq to response api cmd message */
	HINIC3_MGMT_RSP_MSG_AEQ	= 2,
};

#define HINIC3_MBOX_WQ_NAME	"hinic3_mbox"

struct mbox_msg_info {
	u8 msg_id;
	u8 status; /* can only use 1 bit */
};

struct hinic3_msg_desc {
	void *msg;
	u16 msg_len;
	u8 seq_id;
	u8 mod;
	u16 cmd;
	struct mbox_msg_info msg_info;
};

struct hinic3_msg_channel {
	struct hinic3_msg_desc	resp_msg;
	struct hinic3_msg_desc	recv_msg;

	atomic_t		recv_msg_cnt;
};

/* Receive other functions mbox message */
struct hinic3_recv_mbox {
	void *msg;
	u16 msg_len;
	u8 msg_id;
	u8 mod;
	u16 cmd;
	u16 src_func_idx;

	enum hinic3_msg_ack_type ack_type;
	u32 rsvd1;

	void *resp_buff;
};

struct hinic3_send_mbox {
	u8 *data;

	u64 *wb_status; /* write back status */
	void *wb_vaddr;
	dma_addr_t wb_paddr;
};

enum mbox_event_state {
	EVENT_START = 0,
	EVENT_FAIL,
	EVENT_SUCCESS,
	EVENT_TIMEOUT,
	EVENT_END,
};

enum hinic3_mbox_cb_state {
	HINIC3_VF_MBOX_CB_REG = 0,
	HINIC3_VF_MBOX_CB_RUNNING,
	HINIC3_PF_MBOX_CB_REG,
	HINIC3_PF_MBOX_CB_RUNNING,
	HINIC3_PPF_MBOX_CB_REG,
	HINIC3_PPF_MBOX_CB_RUNNING,
	HINIC3_PPF_TO_PF_MBOX_CB_REG,
	HINIC3_PPF_TO_PF_MBOX_CB_RUNNIG,
};

enum hinic3_mbox_ack_type {
	MBOX_ACK,
	MBOX_NO_ACK,
};

struct mbox_dma_msg {
	u32		xor;
	u32		dma_addr_high;
	u32		dma_addr_low;
	u32		msg_len;
	u64		rsvd;
};

struct mbox_dma_queue {
	void *dma_buff_vaddr;
	dma_addr_t dma_buff_paddr;

	u16 depth;
	u16 prod_idx;
	u16 cons_idx;
};

struct hinic3_mbox {
	struct hinic3_hwdev *hwdev;

	bool			lock_channel_en;
	unsigned long		channel_stop;
	u16			cur_msg_channel;
	u32			rsvd1;

	/* lock for send mbox message and ack message */
	struct mutex		mbox_send_lock;
	/* lock for send mbox message */
	struct mutex		msg_send_lock;
	struct hinic3_send_mbox	send_mbox;

	struct mbox_dma_queue	sync_msg_queue;
	struct mbox_dma_queue	async_msg_queue;

	struct workqueue_struct *workq;

	struct hinic3_msg_channel mgmt_msg; /* driver and MGMT CPU */
	struct hinic3_msg_channel *host_msg; /* PPF message between hosts */
	struct hinic3_msg_channel *func_msg; /* PF to VF or VF to PF */
	u16			num_func_msg;
	bool			support_h2h_msg; /* host to host */

	/* vf receive pf/ppf callback */
	hinic3_vf_mbox_cb	vf_mbox_cb[HINIC3_MOD_MAX];
	void *vf_mbox_data[HINIC3_MOD_MAX];
	/* pf/ppf receive vf callback */
	hinic3_pf_mbox_cb	pf_mbox_cb[HINIC3_MOD_MAX];
	void *pf_mbox_data[HINIC3_MOD_MAX];
	/* ppf receive pf/ppf callback */
	hinic3_ppf_mbox_cb	ppf_mbox_cb[HINIC3_MOD_MAX];
	void *ppf_mbox_data[HINIC3_MOD_MAX];
	/* pf receive ppf callback */
	hinic3_pf_recv_from_ppf_mbox_cb	pf_recv_ppf_mbox_cb[HINIC3_MOD_MAX];
	void *pf_recv_ppf_mbox_data[HINIC3_MOD_MAX];
	unsigned long		ppf_to_pf_mbox_cb_state[HINIC3_MOD_MAX];
	unsigned long		ppf_mbox_cb_state[HINIC3_MOD_MAX];
	unsigned long		pf_mbox_cb_state[HINIC3_MOD_MAX];
	unsigned long		vf_mbox_cb_state[HINIC3_MOD_MAX];

	u8			send_msg_id;
	u16			rsvd2;
	enum mbox_event_state	event_flag;
	/* lock for mbox event flag */
	spinlock_t		mbox_lock;
	u64			rsvd3;
};

struct hinic3_mbox_work {
	struct work_struct	work;
	struct hinic3_mbox *func_to_func;
	struct hinic3_recv_mbox *recv_mbox;
	struct hinic3_msg_channel *msg_ch;
};

struct vf_cmd_check_handle {
	u16 cmd;
	bool (*check_cmd)(struct hinic3_hwdev *hwdev, u16 src_func_idx,
			  void *buf_in, u16 in_size);
};

void hinic3_mbox_func_aeqe_handler(void *handle, u8 *header, u8 size);

bool hinic3_mbox_check_cmd_valid(struct hinic3_hwdev *hwdev,
				 struct vf_cmd_check_handle *cmd_handle,
				 u16 vf_id, u16 cmd, void *buf_in, u16 in_size,
				 u8 size);

int hinic3_func_to_func_init(struct hinic3_hwdev *hwdev);

void hinic3_func_to_func_free(struct hinic3_hwdev *hwdev);

int hinic3_mbox_to_host(struct hinic3_hwdev *hwdev, u16 dest_host_ppf_id,
			enum hinic3_mod_type mod, u8 cmd, void *buf_in,
			u16 in_size, void *buf_out, u16 *out_size, u32 timeout, u16 channel);

int hinic3_mbox_to_func_no_ack(struct hinic3_hwdev *hwdev, u16 func_idx,
			       u8 mod, u16 cmd, void *buf_in, u16 in_size,
			       u16 channel);

int hinic3_send_mbox_to_mgmt(struct hinic3_hwdev *hwdev, u8 mod, u16 cmd,
			     void *buf_in, u16 in_size, void *buf_out,
			     u16 *out_size, u32 timeout, u16 channel);

void hinic3_response_mbox_to_mgmt(struct hinic3_hwdev *hwdev, u8 mod, u16 cmd,
				  void *buf_in, u16 in_size, u16 msg_id);

int hinic3_send_mbox_to_mgmt_no_ack(struct hinic3_hwdev *hwdev, u8 mod, u16 cmd,
				    void *buf_in, u16 in_size, u16 channel);
int hinic3_mbox_to_func(struct hinic3_mbox *func_to_func, u8 mod, u16 cmd,
			u16 dst_func, void *buf_in, u16 in_size,
			void *buf_out, u16 *out_size, u32 timeout, u16 channel);

int hinic3_mbox_init_host_msg_channel(struct hinic3_hwdev *hwdev);

void hinic3_mbox_enable_channel_lock(struct hinic3_hwdev *hwdev, bool enable);

#endif

