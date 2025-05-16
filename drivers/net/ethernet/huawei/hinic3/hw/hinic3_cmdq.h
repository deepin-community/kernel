/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_CMDQ_H
#define HINIC3_CMDQ_H

#include <linux/types.h>
#include <linux/completion.h>
#include <linux/spinlock.h>

#include "mpu_inband_cmd_defs.h"
#include "hinic3_cmdq_enhance.h"
#include "hinic3_hw.h"
#include "hinic3_wq.h"
#include "hinic3_common.h"
#include "hinic3_hwdev.h"

#define HINIC3_SCMD_DATA_LEN		16

#define	HINIC3_CMDQ_DEPTH		4096
#define	HINIC3_CMDQ_MAX_BUF_SIZE	2048U
#define	HINIC3_CMDQ_MIN_BUF_SIZE	4U
#define	HINIC3_CMDQ_BUF_ALIGN		2048U

enum hinic3_cmdq_mode {
	HINIC3_NORMAL_CMDQ,
	HINIC3_ENHANCE_CMDQ,
};

enum hinic3_cmdq_type {
	HINIC3_CMDQ_SYNC,
	HINIC3_CMDQ_ASYNC,
	HINIC3_MAX_CMDQ_TYPES = 4
};

enum hinic3_db_src_type {
	HINIC3_DB_SRC_CMDQ_TYPE,
	HINIC3_DB_SRC_L2NIC_SQ_TYPE,
};

enum hinic3_cmdq_db_type {
	HINIC3_DB_SQ_RQ_TYPE,
	HINIC3_DB_CMDQ_TYPE,
};

enum bufdesc_len {
	BUFDESC_LCMD_LEN = 2,
	BUFDESC_SCMD_LEN = 3,
	BUFDESC_ENHANCE_CMD_LEN = 3,  /* 64B aligned */
};

/* hardware define: cmdq wqe */
struct hinic3_cmdq_header {
	u32	header_info;
	u32	saved_data;
};

struct hinic3_scmd_bufdesc {
	u32	buf_len;
	u32	rsvd;
	u8	data[HINIC3_SCMD_DATA_LEN];
};

struct hinic3_lcmd_bufdesc {
	struct hinic3_sge	sge;
	u32			rsvd1;
	u64			saved_async_buf;
	u64			rsvd3;
};

struct hinic3_cmdq_db {
	u32	db_head;
	u32	db_info;
};

struct hinic3_status {
	u32	status_info;
};

struct hinic3_ctrl {
	u32	ctrl_info;
};

struct hinic3_sge_resp {
	struct hinic3_sge sge;
	u32		rsvd;
};

struct hinic3_cmdq_completion {
	union {
		struct hinic3_sge_resp	sge_resp;
		u64			direct_resp;
	};
};

struct hinic3_cmdq_wqe_scmd {
	struct hinic3_cmdq_header	header;
	u64				rsvd;
	struct hinic3_status		status;
	struct hinic3_ctrl		ctrl;
	struct hinic3_cmdq_completion	completion;
	struct hinic3_scmd_bufdesc	buf_desc;
};

struct hinic3_cmdq_wqe_lcmd {
	struct hinic3_cmdq_header	header;
	struct hinic3_status		status;
	struct hinic3_ctrl		ctrl;
	struct hinic3_cmdq_completion	completion;
	struct hinic3_lcmd_bufdesc	buf_desc;
};

struct hinic3_cmdq_inline_wqe {
	struct hinic3_cmdq_wqe_scmd	wqe_scmd;
};

struct hinic3_cmdq_wqe {
	union {
		struct hinic3_cmdq_inline_wqe	inline_wqe;
		struct hinic3_cmdq_wqe_lcmd	wqe_lcmd;
		struct hinic3_enhanced_cmdq_wqe	enhanced_cmdq_wqe;
	};
};

struct hinic3_cmdq_cmd_param {
	u8 mod;
	u8 cmd;
	struct hinic3_cmd_buf *buf_in;
	struct hinic3_cmd_buf *buf_out;
	u64 *out_param;
};

struct hinic3_cmdq_arm_bit {
	u32	q_type;
	u32	q_id;
};

enum hinic3_cmdq_status {
	HINIC3_CMDQ_ENABLE = BIT(0),
};

enum hinic3_cmdq_cmd_type {
	HINIC3_CMD_TYPE_NONE,
	HINIC3_CMD_TYPE_SET_ARM,
	HINIC3_CMD_TYPE_DIRECT_RESP,
	HINIC3_CMD_TYPE_SGE_RESP,
	HINIC3_CMD_TYPE_ASYNC,
	HINIC3_CMD_TYPE_FAKE_TIMEOUT,
	HINIC3_CMD_TYPE_TIMEOUT,
	HINIC3_CMD_TYPE_FORCE_STOP,
};

enum data_format {
	DATA_SGE,
	DATA_DIRECT,
};

#define	WQ_BLOCK_PFN_SHIFT		9
#define CMDQ_PFN_SHIFT			12

#define CMDQ_PFN(addr)			((addr) >> CMDQ_PFN_SHIFT)
#define WQ_BLOCK_PFN(page_addr)		((page_addr) >> WQ_BLOCK_PFN_SHIFT)

struct hinic3_cmdq_cmd_info {
	enum hinic3_cmdq_cmd_type	cmd_type;
	u16				channel;
	u16				rsvd1;
	u16				wqebb_use_num;

	struct completion		*done;
	int				*errcode;
	int				*cmpt_code;
	u64				*direct_resp;
	u64				cmdq_msg_id;

	struct hinic3_cmd_buf		*buf_in;
	struct hinic3_cmd_buf		*buf_out;
};

struct hinic3_cmdq {
	struct hinic3_wq		wq;

	enum hinic3_cmdq_type		cmdq_type;
	int				wrapped;

	/* spinlock for send cmdq commands */
	spinlock_t			cmdq_lock;

	struct cmdq_ctxt_info		cmdq_ctxt;
	struct enhance_cmdq_ctxt_info	cmdq_enhance_ctxt;

	struct hinic3_cmdq_cmd_info	*cmd_infos;

	struct hinic3_hwdev		*hwdev;
	struct hinic3_cmdqs		*cmdqs;
	u64				rsvd1[2];
};

struct hinic3_cmdqs {
	struct hinic3_hwdev		*hwdev;

	struct dma_pool			*cmd_buf_pool;
	/* doorbell area */
	u8 __iomem			*cmdqs_db_base;

	/* All cmdq's CLA of a VF occupy a PAGE when cmdq wq is 1-level CLA */
	dma_addr_t			wq_block_paddr;
	void				*wq_block_vaddr;
	struct hinic3_cmdq		cmdq[HINIC3_MAX_CMDQ_TYPES];

	u32				status;
	u32				disable_flag;

	bool				lock_channel_en;
	unsigned long			channel_stop;
	u8				cmdq_num;
	u8				cmdq_mode;
	u8				wqebb_size;
	u8				wqebb_use_num;
	u32				rsvd1;
	u64				rsvd2;
	u32				cmd_buf_size;
	bool				poll;   /* use polling mode or int mode */
};

void hinic3_cmdq_ceq_handler(void *handle, u32 ceqe_data);

int hinic3_reinit_cmdq_ctxts(struct hinic3_hwdev *hwdev);

bool hinic3_cmdq_idle(struct hinic3_cmdq *cmdq);

int hinic3_cmdqs_init(struct hinic3_hwdev *hwdev);

void hinic3_cmdqs_free(struct hinic3_hwdev *hwdev);

void hinic3_cmdq_flush_cmd(struct hinic3_hwdev *hwdev,
			   struct hinic3_cmdq *cmdq);

int hinic3_cmdq_set_channel_status(struct hinic3_hwdev *hwdev, u16 channel,
				   bool enable);

void hinic3_cmdq_enable_channel_lock(struct hinic3_hwdev *hwdev, bool enable);

void hinic3_cmdq_flush_sync_cmd(struct hinic3_hwdev *hwdev);

void enhanced_cmdq_set_wqe(struct hinic3_cmdq_wqe *wqe, enum hinic3_cmdq_cmd_type cmd_type,
			const struct hinic3_cmdq_cmd_param *cmd_buf, int wrapped);
void enhanced_cmdq_init_queue_ctxt(struct hinic3_cmdqs *cmdqs, struct hinic3_cmdq *cmdq);
#endif

