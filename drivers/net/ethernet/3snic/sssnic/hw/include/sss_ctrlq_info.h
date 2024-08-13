/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_CTRLQ_INFO_H
#define SSS_CTRLQ_INFO_H

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <linux/pci.h>

#include "sss_hw_mbx_msg.h"
#include "sss_hw_wq.h"
#include "sss_hw_ctrlq.h"

#define SSS_DEFAULT_WQ_PAGE_SIZE		0x100000
#define SSS_HW_WQ_PAGE_SIZE				0x1000
#define SSS_MAX_WQ_PAGE_NUM		8

/* ctrlq ack type */
enum sss_ack_type {
	SSS_ACK_TYPE_CTRLQ,
	SSS_ACK_TYPE_SHARE_CQN,
	SSS_ACK_TYPE_APP_CQN,

	SSS_MOD_ACK_MAX = 15,
};

enum sss_ctrlq_type {
	SSS_CTRLQ_SYNC,
	SSS_CTRLQ_ASYNC,
	SSS_MAX_CTRLQ_TYPE = 4
};

enum sss_ctrlq_msg_type {
	SSS_MSG_TYPE_NONE,
	SSS_MSG_TYPE_SET_ARM,
	SSS_MSG_TYPE_DIRECT_RESP,
	SSS_MSG_TYPE_SGE_RESP,
	SSS_MSG_TYPE_ASYNC,
	SSS_MSG_TYPE_PSEUDO_TIMEOUT,
	SSS_MSG_TYPE_TIMEOUT,
	SSS_MSG_TYPE_FORCE_STOP,
	SSS_MSG_TYPE_MAX
};

struct sss_ctrlq_cmd_info {
	enum sss_ctrlq_msg_type		msg_type;
	u16							channel;

	struct completion			*done;
	int							*err_code;
	int							*cmpt_code;
	u64							*direct_resp;
	u64							msg_id;

	struct sss_ctrl_msg_buf		*in_buf;
	struct sss_ctrl_msg_buf		*out_buf;
};

struct sss_ctrlq {
	struct sss_wq					wq;

	enum sss_ctrlq_type				ctrlq_type;
	int								wrapped;

	/* spinlock for send ctrlq commands */
	spinlock_t						ctrlq_lock;

	struct sss_ctrlq_ctxt_info		ctrlq_ctxt;

	struct sss_ctrlq_cmd_info		*cmd_info;

	void							*hwdev;
};

struct sss_ctrlq_info {
	void				*hwdev;

	struct pci_pool		*msg_buf_pool;

	/* doorbell area */
	u8 __iomem			*db_base;

	/* All ctrlq's CLA of a VF occupy a PAGE when ctrlq wq is 1-level CLA */
	void				*wq_block_vaddr;
	dma_addr_t			wq_block_paddr;
	struct sss_ctrlq	ctrlq[SSS_MAX_CTRLQ_TYPE];

	u32					state;
	u32					disable_flag;

	u8					lock_channel_en;
	u8					num;
	u8					rsvd[6];
	unsigned long		channel_stop;
};
#endif
