// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/errno.h>
#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/module.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_hwdev.h"
#include "sss_hwif_eq.h"
#include "sss_hwif_api.h"
#include "sss_hwif_ctrlq.h"
#include "sss_hwif_aeq.h"
#include "sss_hwif_ceq.h"
#include "sss_common.h"

#define SSS_CTRLQ_CMD_TIMEOUT			5000 /* millisecond */

#define SSS_CTRLQ_WQE_HEAD_LEN			32

#define SSS_HI_8_BITS(data)				(((data) >> 8) & 0xFF)
#define SSS_LO_8_BITS(data)				((data) & 0xFF)

#define SSS_CTRLQ_DB_INFO_HI_PI_SHIFT				0
#define SSS_CTRLQ_DB_INFO_HI_PI_MASK				0xFFU
#define SSS_CTRLQ_DB_INFO_SET(val, member)			\
			((((u32)(val)) & SSS_CTRLQ_DB_INFO_##member##_MASK) << \
			SSS_CTRLQ_DB_INFO_##member##_SHIFT)

#define SSS_CTRLQ_DB_HEAD_QUEUE_TYPE_SHIFT			23
#define SSS_CTRLQ_DB_HEAD_CTRLQ_TYPE_SHIFT			24
#define SSS_CTRLQ_DB_HEAD_SRC_TYPE_SHIFT			27
#define SSS_CTRLQ_DB_HEAD_QUEUE_TYPE_MASK			0x1U
#define SSS_CTRLQ_DB_HEAD_CTRLQ_TYPE_MASK			0x7U
#define SSS_CTRLQ_DB_HEAD_SRC_TYPE_MASK				0x1FU
#define SSS_CTRLQ_DB_HEAD_SET(val, member)			\
			((((u32)(val)) & SSS_CTRLQ_DB_HEAD_##member##_MASK) << \
			SSS_CTRLQ_DB_HEAD_##member##_SHIFT)

#define SSS_CTRLQ_CTRL_PI_SHIFT					0
#define SSS_CTRLQ_CTRL_CMD_SHIFT				16
#define SSS_CTRLQ_CTRL_MOD_SHIFT				24
#define SSS_CTRLQ_CTRL_ACK_TYPE_SHIFT			29
#define SSS_CTRLQ_CTRL_HW_BUSY_BIT_SHIFT		31

#define SSS_CTRLQ_CTRL_PI_MASK					0xFFFFU
#define SSS_CTRLQ_CTRL_CMD_MASK					0xFFU
#define SSS_CTRLQ_CTRL_MOD_MASK					0x1FU
#define SSS_CTRLQ_CTRL_ACK_TYPE_MASK			0x3U
#define SSS_CTRLQ_CTRL_HW_BUSY_BIT_MASK			0x1U

#define SSS_CTRLQ_CTRL_SET(val, member)			\
			((((u32)(val)) & SSS_CTRLQ_CTRL_##member##_MASK) << \
			 SSS_CTRLQ_CTRL_##member##_SHIFT)

#define SSS_CTRLQ_CTRL_GET(val, member)			\
			(((val) >> SSS_CTRLQ_CTRL_##member##_SHIFT) & \
			SSS_CTRLQ_CTRL_##member##_MASK)

#define SSS_CTRLQ_WQE_HEAD_BD_LEN_SHIFT			0
#define SSS_CTRLQ_WQE_HEAD_COMPLETE_FMT_SHIFT			15
#define SSS_CTRLQ_WQE_HEAD_DATA_FMT_SHIFT				22
#define SSS_CTRLQ_WQE_HEAD_COMPLETE_REQ_SHIFT			23
#define SSS_CTRLQ_WQE_HEAD_COMPLETE_SECT_LEN_SHIFT		27
#define SSS_CTRLQ_WQE_HEAD_CTRL_LEN_SHIFT				29
#define SSS_CTRLQ_WQE_HEAD_HW_BUSY_BIT_SHIFT			31

#define SSS_CTRLQ_WQE_HEAD_BD_LEN_MASK			0xFFU
#define SSS_CTRLQ_WQE_HEAD_COMPLETE_FMT_MASK			0x1U
#define SSS_CTRLQ_WQE_HEAD_DATA_FMT_MASK				0x1U
#define SSS_CTRLQ_WQE_HEAD_COMPLETE_REQ_MASK			0x1U
#define SSS_CTRLQ_WQE_HEAD_COMPLETE_SECT_LEN_MASK		0x3U
#define SSS_CTRLQ_WQE_HEAD_CTRL_LEN_MASK				0x3U
#define SSS_CTRLQ_WQE_HEAD_HW_BUSY_BIT_MASK				0x1U

#define SSS_CTRLQ_WQE_HEAD_SET(val, member)		\
			((((u32)(val)) & SSS_CTRLQ_WQE_HEAD_##member##_MASK) << \
			SSS_CTRLQ_WQE_HEAD_##member##_SHIFT)

#define SSS_GET_CTRLQ_WQE_HEAD(val, member)		\
			(((val) >> SSS_CTRLQ_WQE_HEAD_##member##_SHIFT) & \
			SSS_CTRLQ_WQE_HEAD_##member##_MASK)

#define SSS_STORE_DATA_ARM_SHIFT				31

#define SSS_STORE_DATA_ARM_MASK					0x1U

#define SSS_STORE_DATA_SET(val, member)			\
			(((val) & SSS_STORE_DATA_##member##_MASK) << \
			SSS_STORE_DATA_##member##_SHIFT)

#define SSS_STORE_DATA_CLEAR(val, member)			\
			((val) & (~(SSS_STORE_DATA_##member##_MASK << \
			SSS_STORE_DATA_##member##_SHIFT)))

#define SSS_WQE_ERRCODE_VAL_SHIFT				0

#define SSS_WQE_ERRCODE_VAL_MASK				0x7FFFFFFF

#define SSS_GET_WQE_ERRCODE(val, member)	\
			(((val) >> SSS_WQE_ERRCODE_##member##_SHIFT) & \
			SSS_WQE_ERRCODE_##member##_MASK)

#define SSS_CEQE_CTRLQ_TYPE_SHIFT				0

#define SSS_CEQE_CTRLQ_TYPE_MASK				0x7

#define SSS_GET_CEQE_CTRLQ(val, member)	\
			(((val) >> SSS_CEQE_CTRLQ_##member##_SHIFT) & \
			SSS_CEQE_CTRLQ_##member##_MASK)

#define SSS_WQE_COMPLETE(ctrl_info)	SSS_CTRLQ_CTRL_GET(ctrl_info, HW_BUSY_BIT)

#define SSS_WQE_HEAD(wqe)			((struct sss_ctrlq_head *)(wqe))

#define SSS_CTRLQ_DB_PI_OFF(pi)		(((u16)SSS_LO_8_BITS(pi)) << 3)

#define SSS_CTRLQ_DB_ADDR(db_base, pi)	\
			(((u8 *)(db_base)) + SSS_CTRLQ_DB_PI_OFF(pi))

#define SSS_FIRST_DATA_TO_WRITE_LAST	sizeof(u64)

#define SSS_WQE_LCMD_SIZE			64
#define SSS_WQE_SCMD_SIZE			64

#define SSS_COMPLETE_LEN			3

#define SSS_CTRLQ_WQE_SIZE			64

#define SSS_CTRLQ_TO_INFO(ctrlq)		\
			container_of((ctrlq) - (ctrlq)->ctrlq_type, struct sss_ctrlq_info, ctrlq[0])

#define SSS_CTRLQ_COMPLETE_CODE		11

enum SSS_ctrlq_scmd_type {
	SSS_CTRLQ_SET_ARM_CMD = 2,
};

enum sss_ctrl_sect_len {
	SSS_CTRL_SECT_LEN = 1,
	SSS_CTRL_DIRECT_SECT_LEN = 2,
};

enum sss_bd_len {
	SSS_BD_LCMD_LEN = 2,
	SSS_BD_SCMD_LEN = 3,
};

enum sss_data_fmt {
	SSS_DATA_SGE,
	SSS_DATA_DIRECT,
};

enum sss_completion_fmt {
	SSS_COMPLETE_DIRECT,
	SSS_COMPLETE_SGE,
};

enum sss_completion_request {
	SSS_CEQ_SET = 1,
};

enum sss_ctrlq_comm_msg_type {
	SSS_SYNC_MSG_DIRECT_REPLY,
	SSS_SYNC_MSG_SGE_REPLY,
	SSS_ASYNC_MSG,
};

#define SSS_SCMD_DATA_LEN		16

enum sss_db_src_type {
	SSS_DB_SRC_CTRLQ_TYPE,
	SSS_DB_SRC_L2NIC_SQ_TYPE,
};

enum sss_ctrlq_db_type {
	SSS_DB_SQ_RQ_TYPE,
	SSS_DB_CTRLQ_TYPE,
};

struct sss_ctrlq_db {
	u32	head;
	u32	info;
};

/* hardware define: ctrlq wqe */
struct sss_ctrlq_head {
	u32 info;
	u32 store_data;
};

struct sss_scmd_bd {
	u32 data_len;
	u32 rsvd;
	u8 data[SSS_SCMD_DATA_LEN];
};

struct sss_lcmd_bd {
	struct sss_sge sge;
	u32 rsvd1;
	u64 store_async_buf;
	u64 rsvd3;
};

struct sss_wqe_state {
	u32 info;
};

struct sss_wqe_ctrl {
	u32 info;
};

struct sss_sge_reply {
	struct sss_sge sge;
	u32 rsvd;
};

struct sss_ctrlq_completion {
	union {
		struct sss_sge_reply sge_reply;
		u64	 direct_reply;
	};
};

struct sss_ctrlq_wqe_scmd {
	struct sss_ctrlq_head head;
	u64 rsvd;
	struct sss_wqe_state state;
	struct sss_wqe_ctrl ctrl;
	struct sss_ctrlq_completion completion;
	struct sss_scmd_bd bd;
};

struct sss_ctrlq_wqe_lcmd {
	struct sss_ctrlq_head head;
	struct sss_wqe_state state;
	struct sss_wqe_ctrl ctrl;
	struct sss_ctrlq_completion completion;
	struct sss_lcmd_bd bd;
};

struct sss_ctrlq_inline_wqe {
	struct sss_ctrlq_wqe_scmd wqe_scmd;
};

struct sss_ctrlq_wqe {
	union {
		struct sss_ctrlq_inline_wqe inline_wqe;
		struct sss_ctrlq_wqe_lcmd wqe_lcmd;
	};
};

typedef int (*sss_ctrlq_type_handler_t)(struct sss_ctrlq *ctrlq,
				struct sss_ctrlq_wqe *wqe, u16 ci);

void *sss_ctrlq_read_wqe(struct sss_wq *wq, u16 *ci)
{
	if (sss_wq_is_empty(wq))
		return NULL;

	return sss_wq_read_one_wqebb(wq, ci);
}

static void *sss_ctrlq_get_wqe(struct sss_wq *wq, u16 *pi)
{
	if (!sss_wq_free_wqebb(wq))
		return NULL;

	return sss_wq_get_one_wqebb(wq, pi);
}

static void sss_ctrlq_set_completion(struct sss_ctrlq_completion *complete,
				     struct sss_ctrl_msg_buf *out_buf)
{
	struct sss_sge_reply *sge_reply = &complete->sge_reply;

	sss_set_sge(&sge_reply->sge, out_buf->dma_addr, SSS_CTRLQ_BUF_LEN);
}

static void sss_ctrlq_set_lcmd_bufdesc(struct sss_ctrlq_wqe_lcmd *wqe,
				       struct sss_ctrl_msg_buf *in_buf)
{
	sss_set_sge(&wqe->bd.sge, in_buf->dma_addr, in_buf->size);
}

static void sss_ctrlq_fill_db(struct sss_ctrlq_db *db,
			      enum sss_ctrlq_type ctrlq_type, u16 pi)
{
	db->info = SSS_CTRLQ_DB_INFO_SET(SSS_HI_8_BITS(pi), HI_PI);

	db->head = SSS_CTRLQ_DB_HEAD_SET(SSS_DB_CTRLQ_TYPE, QUEUE_TYPE) |
		   SSS_CTRLQ_DB_HEAD_SET(ctrlq_type, CTRLQ_TYPE)		|
		   SSS_CTRLQ_DB_HEAD_SET(SSS_DB_SRC_CTRLQ_TYPE, SRC_TYPE);
}

static void sss_ctrlq_set_db(struct sss_ctrlq *ctrlq,
			     enum sss_ctrlq_type ctrlq_type, u16 pi)
{
	struct sss_ctrlq_db db = {0};
	u8 *db_base = SSS_TO_HWDEV(ctrlq)->ctrlq_info->db_base;

	sss_ctrlq_fill_db(&db, ctrlq_type, pi);

	/* The data that is written to HW should be in Big Endian Format */
	db.info = sss_hw_be32(db.info);
	db.head = sss_hw_be32(db.head);

	wmb(); /* make sure write db info to reg */
	writeq(*((u64 *)&db), SSS_CTRLQ_DB_ADDR(db_base, pi));
}

static void sss_ctrlq_fill_wqe(void *dst, const void *src)
{
	memcpy((u8 *)dst + SSS_FIRST_DATA_TO_WRITE_LAST,
	       (u8 *)src + SSS_FIRST_DATA_TO_WRITE_LAST,
	       SSS_CTRLQ_WQE_SIZE - SSS_FIRST_DATA_TO_WRITE_LAST);

	wmb(); /* The first 8 bytes should be written last */

	*(u64 *)dst = *(u64 *)src;
}

static void sss_ctrlq_prepare_wqe_ctrl(struct sss_ctrlq_wqe *wqe,
				       int wrapped, u8 mod, u8 cmd, u16 pi,
				       enum sss_completion_fmt complete_fmt,
				       enum sss_data_fmt data_fmt,
				       enum sss_bd_len buf_len)
{
	struct sss_wqe_ctrl *ctrl = NULL;
	enum sss_ctrl_sect_len ctrl_len;
	struct sss_ctrlq_wqe_lcmd *wqe_lcmd = NULL;
	struct sss_ctrlq_wqe_scmd *wqe_scmd = NULL;
	u32 saved_data = SSS_WQE_HEAD(wqe)->store_data;

	if (data_fmt == SSS_DATA_SGE) {
		wqe_lcmd = &wqe->wqe_lcmd;

		wqe_lcmd->state.info = 0;
		ctrl = &wqe_lcmd->ctrl;
		ctrl_len = SSS_CTRL_SECT_LEN;
	} else {
		wqe_scmd = &wqe->inline_wqe.wqe_scmd;

		wqe_scmd->state.info = 0;
		ctrl = &wqe_scmd->ctrl;
		ctrl_len = SSS_CTRL_DIRECT_SECT_LEN;
	}

	ctrl->info = SSS_CTRLQ_CTRL_SET(pi, PI) |
		     SSS_CTRLQ_CTRL_SET(cmd, CMD) |
		     SSS_CTRLQ_CTRL_SET(mod, MOD) |
		     SSS_CTRLQ_CTRL_SET(SSS_ACK_TYPE_CTRLQ, ACK_TYPE);

	SSS_WQE_HEAD(wqe)->info =
		SSS_CTRLQ_WQE_HEAD_SET(buf_len, BD_LEN) |
		SSS_CTRLQ_WQE_HEAD_SET(complete_fmt, COMPLETE_FMT) |
		SSS_CTRLQ_WQE_HEAD_SET(data_fmt, DATA_FMT) |
		SSS_CTRLQ_WQE_HEAD_SET(SSS_CEQ_SET, COMPLETE_REQ) |
		SSS_CTRLQ_WQE_HEAD_SET(SSS_COMPLETE_LEN, COMPLETE_SECT_LEN) |
		SSS_CTRLQ_WQE_HEAD_SET(ctrl_len, CTRL_LEN) |
		SSS_CTRLQ_WQE_HEAD_SET((u32)wrapped, HW_BUSY_BIT);

	if (cmd == SSS_CTRLQ_SET_ARM_CMD && mod == SSS_MOD_TYPE_COMM) {
		saved_data &= SSS_STORE_DATA_CLEAR(saved_data, ARM);
		SSS_WQE_HEAD(wqe)->store_data = saved_data |
						SSS_STORE_DATA_SET(1, ARM);
	} else {
		saved_data &= SSS_STORE_DATA_CLEAR(saved_data, ARM);
		SSS_WQE_HEAD(wqe)->store_data = saved_data;
	}
}

static void sss_ctrlq_set_lcmd_wqe(struct sss_ctrlq_wqe *wqe,
				   enum sss_ctrlq_comm_msg_type cmd_type,
				   struct sss_ctrl_msg_buf *in_buf,
				   struct sss_ctrl_msg_buf *out_buf, int wrapped,
				   u8 mod, u8 cmd, u16 pi)
{
	struct sss_ctrlq_wqe_lcmd *wqe_lcmd = &wqe->wqe_lcmd;
	enum sss_completion_fmt complete_fmt = SSS_COMPLETE_DIRECT;

	switch (cmd_type) {
	case SSS_SYNC_MSG_DIRECT_REPLY:
		wqe_lcmd->completion.direct_reply = 0;
		break;
	case SSS_SYNC_MSG_SGE_REPLY:
		if (out_buf) {
			complete_fmt = SSS_COMPLETE_SGE;
			sss_ctrlq_set_completion(&wqe_lcmd->completion, out_buf);
		}
		break;
	case SSS_ASYNC_MSG:
		wqe_lcmd->completion.direct_reply = 0;
		wqe_lcmd->bd.store_async_buf = (u64)(in_buf);
		break;
	}

	sss_ctrlq_prepare_wqe_ctrl(wqe, wrapped, mod, cmd, pi, complete_fmt,
				   SSS_DATA_SGE, SSS_BD_LCMD_LEN);

	sss_ctrlq_set_lcmd_bufdesc(wqe_lcmd, in_buf);
}

static void sss_ctrlq_update_cmd_state(struct sss_ctrlq *ctrlq, u16 pi,
				       struct sss_ctrlq_wqe *wqe)
{
	struct sss_ctrlq_cmd_info *info = &ctrlq->cmd_info[pi];
	struct sss_ctrlq_wqe_lcmd *lcmd = &wqe->wqe_lcmd;
	u32 state = sss_hw_cpu32(lcmd->state.info);

	if (info->direct_resp)
		*info->direct_resp =
			sss_hw_cpu32(lcmd->completion.direct_reply);

	if (info->err_code)
		*info->err_code = SSS_GET_WQE_ERRCODE(state, VAL);
}

static int sss_ctrlq_check_sync_timeout(struct sss_ctrlq *ctrlq,
					struct sss_ctrlq_wqe *wqe, u16 pi)
{
	struct sss_ctrlq_wqe_lcmd *wqe_lcmd;
	struct sss_wqe_ctrl *ctrl;
	u32 ctrl_info;

	wqe_lcmd = &wqe->wqe_lcmd;
	ctrl = &wqe_lcmd->ctrl;
	ctrl_info = sss_hw_cpu32((ctrl)->info);
	if (!SSS_WQE_COMPLETE(ctrl_info)) {
		sdk_info(SSS_TO_HWDEV(ctrlq)->dev_hdl, "Ctrlq wqe do not complete\n");
		return -EFAULT;
	}

	sss_ctrlq_update_cmd_state(ctrlq, pi, wqe);

	sdk_info(SSS_TO_HWDEV(ctrlq)->dev_hdl, "Success to check ctrlq sync cmd\n");
	return 0;
}

static void sss_reset_cmd_info(struct sss_ctrlq_cmd_info *cmd_info,
			       const struct sss_ctrlq_cmd_info *store_cmd_info)
{
	if (cmd_info->err_code == store_cmd_info->err_code)
		cmd_info->err_code = NULL;

	if (cmd_info->done == store_cmd_info->done)
		cmd_info->done = NULL;

	if (cmd_info->direct_resp == store_cmd_info->direct_resp)
		cmd_info->direct_resp = NULL;
}

static int sss_ctrlq_ceq_handler_state(struct sss_ctrlq *ctrlq,
				       struct sss_ctrlq_cmd_info *cmd_info,
				       struct sss_ctrlq_cmd_info *store_cmd_info,
				       u64 curr_msg_id, u16 curr_pi,
				       struct sss_ctrlq_wqe *curr_wqe,
				       u32 timeout)
{
	ulong timeo;
	int ret;
	ulong end = jiffies + msecs_to_jiffies(timeout);

	if (SSS_TO_HWDEV(ctrlq)->poll) {
		while (time_before(jiffies, end)) {
			sss_ctrlq_ceq_handler(SSS_TO_HWDEV(ctrlq), 0);
			if (store_cmd_info->done->done != 0)
				return 0;
			usleep_range(9, 10); /* sleep 9 us ~ 10 us */
		}
	} else {
		timeo = msecs_to_jiffies(timeout);
		if (wait_for_completion_timeout(store_cmd_info->done, timeo))
			return 0;
	}

	spin_lock_bh(&ctrlq->ctrlq_lock);

	if (cmd_info->cmpt_code == store_cmd_info->cmpt_code)
		cmd_info->cmpt_code = NULL;

	if (*store_cmd_info->cmpt_code == SSS_CTRLQ_COMPLETE_CODE) {
		sdk_info(SSS_TO_HWDEV(ctrlq)->dev_hdl, "Ctrlq direct sync command complete\n");
		spin_unlock_bh(&ctrlq->ctrlq_lock);
		return 0;
	}

	if (curr_msg_id == cmd_info->msg_id) {
		ret = sss_ctrlq_check_sync_timeout(ctrlq, curr_wqe, curr_pi);
		if (ret != 0)
			cmd_info->msg_type = SSS_MSG_TYPE_TIMEOUT;
		else
			cmd_info->msg_type = SSS_MSG_TYPE_PSEUDO_TIMEOUT;
	} else {
		ret = -ETIMEDOUT;
		sdk_err(SSS_TO_HWDEV(ctrlq)->dev_hdl,
			"Ctrlq sync command curr_msg_id dismatch with cmd_info msg_id\n");
	}

	sss_reset_cmd_info(cmd_info, store_cmd_info);

	spin_unlock_bh(&ctrlq->ctrlq_lock);

	if (ret == 0)
		return 0;

	sss_dump_ceq_info(SSS_TO_HWDEV(ctrlq));

	return -ETIMEDOUT;
}

static int sss_wait_ctrlq_sync_cmd_completion(struct sss_ctrlq *ctrlq,
					      struct sss_ctrlq_cmd_info *cmd_info,
					      struct sss_ctrlq_cmd_info *store_cmd_info,
					      u64 curr_msg_id, u16 curr_pi,
					      struct sss_ctrlq_wqe *curr_wqe, u32 timeout)
{
	return sss_ctrlq_ceq_handler_state(ctrlq, cmd_info, store_cmd_info,
					   curr_msg_id, curr_pi, curr_wqe, timeout);
}

static int sss_ctrlq_msg_lock(struct sss_ctrlq *ctrlq, u16 channel)
{
	struct sss_ctrlq_info *ctrlq_info = SSS_CTRLQ_TO_INFO(ctrlq);

	spin_lock_bh(&ctrlq->ctrlq_lock);

	if (ctrlq_info->lock_channel_en && test_bit(channel, &ctrlq_info->channel_stop)) {
		spin_unlock_bh(&ctrlq->ctrlq_lock);
		return -EAGAIN;
	}

	return 0;
}

static void sss_ctrlq_msg_unlock(struct sss_ctrlq *ctrlq)
{
	spin_unlock_bh(&ctrlq->ctrlq_lock);
}

static void sss_ctrlq_set_cmd_buf(struct sss_ctrlq_cmd_info *cmd_info,
				  struct sss_hwdev *hwdev,
				  struct sss_ctrl_msg_buf *in_buf,
				  struct sss_ctrl_msg_buf *out_buf)
{
	cmd_info->in_buf = in_buf;
	cmd_info->out_buf = out_buf;

	if (in_buf)
		atomic_inc(&in_buf->ref_cnt);

	if (out_buf)
		atomic_inc(&out_buf->ref_cnt);
}

int sss_ctrlq_sync_cmd_direct_reply(struct sss_ctrlq *ctrlq, u8 mod,
				    u8 cmd, struct sss_ctrl_msg_buf *in_buf,
				    u64 *out_param, u32 timeout, u16 channel)
{
	struct sss_wq *wq = &ctrlq->wq;
	struct sss_ctrlq_wqe *curr_wqe = NULL;
	struct sss_ctrlq_wqe wqe;
	struct sss_ctrlq_cmd_info *cmd_info = NULL;
	struct sss_ctrlq_cmd_info store_cmd_info;
	struct completion done;
	u16 curr_pi, next_pi;
	int wrapped;
	int errcode = 0;
	int cmpt_code = SSS_CTRLQ_SEND_CMPT_CODE;
	u64 curr_msg_id;
	int ret;
	u32 real_timeout;

	ret = sss_ctrlq_msg_lock(ctrlq, channel);
	if (ret != 0)
		return ret;

	curr_wqe = sss_ctrlq_get_wqe(wq, &curr_pi);
	if (!curr_wqe) {
		sss_ctrlq_msg_unlock(ctrlq);
		return -EBUSY;
	}

	memset(&wqe, 0, sizeof(wqe));

	wrapped = ctrlq->wrapped;

	next_pi = curr_pi + SSS_WQEBB_NUM_FOR_CTRLQ;
	if (next_pi >= wq->q_depth) {
		ctrlq->wrapped = (ctrlq->wrapped == 0) ? 1 : 0;
		next_pi -= (u16)wq->q_depth;
	}

	cmd_info = &ctrlq->cmd_info[curr_pi];

	init_completion(&done);

	cmd_info->msg_type = SSS_MSG_TYPE_DIRECT_RESP;
	cmd_info->done = &done;
	cmd_info->err_code = &errcode;
	cmd_info->direct_resp = out_param;
	cmd_info->cmpt_code = &cmpt_code;
	cmd_info->channel = channel;
	sss_ctrlq_set_cmd_buf(cmd_info, SSS_TO_HWDEV(ctrlq), in_buf, NULL);

	memcpy(&store_cmd_info, cmd_info, sizeof(*cmd_info));

	sss_ctrlq_set_lcmd_wqe(&wqe, SSS_SYNC_MSG_DIRECT_REPLY, in_buf, NULL,
			       wrapped, mod, cmd, curr_pi);

	/* CTRLQ WQE is not shadow, therefore wqe will be written to wq */
	sss_ctrlq_fill_wqe(curr_wqe, &wqe);

	(cmd_info->msg_id)++;
	curr_msg_id = cmd_info->msg_id;

	sss_ctrlq_set_db(ctrlq, SSS_CTRLQ_SYNC, next_pi);

	sss_ctrlq_msg_unlock(ctrlq);

	real_timeout = timeout ? timeout : SSS_CTRLQ_CMD_TIMEOUT;
	ret = sss_wait_ctrlq_sync_cmd_completion(ctrlq, cmd_info, &store_cmd_info,
						 curr_msg_id, curr_pi, curr_wqe, real_timeout);
	if (ret != 0) {
		sdk_err(SSS_TO_HWDEV(ctrlq)->dev_hdl,
			"Ctrlq sync cmd direct resp timeout, mod: %u, cmd: %u, pi: 0x%x\n",
			mod, cmd, curr_pi);
		ret = -ETIMEDOUT;
	}

	if (cmpt_code == SSS_CTRLQ_FORCE_STOP_CMPT_CODE) {
		sdk_info(SSS_TO_HWDEV(ctrlq)->dev_hdl, "Force stop ctrlq cmd, mod: %u, cmd: %u\n",
			 mod, cmd);
		ret = -EAGAIN;
	}

	destroy_completion(&done);
	smp_rmb(); /* read error code after completion */

	return (ret != 0) ? ret : errcode;
}

int sss_ctrlq_sync_cmd_detail_reply(struct sss_ctrlq *ctrlq, u8 mod, u8 cmd,
				    struct sss_ctrl_msg_buf *in_buf,
				    struct sss_ctrl_msg_buf *out_buf,
				    u64 *out_param, u32 timeout, u16 channel)
{
	struct sss_wq *wq = &ctrlq->wq;
	struct sss_ctrlq_wqe *curr_wqe = NULL, wqe;
	struct sss_ctrlq_cmd_info *cmd_info = NULL, store_cmd_info;
	struct completion done;
	u16 curr_pi, next_pi;
	int wrapped, errcode = 0;
	int cmpt_code = SSS_CTRLQ_SEND_CMPT_CODE;
	u64 curr_msg_id;
	int ret;
	u32 real_timeout;

	ret = sss_ctrlq_msg_lock(ctrlq, channel);
	if (ret != 0)
		return ret;

	curr_wqe = sss_ctrlq_get_wqe(wq, &curr_pi);
	if (!curr_wqe) {
		sss_ctrlq_msg_unlock(ctrlq);
		return -EBUSY;
	}

	memset(&wqe, 0, sizeof(wqe));

	wrapped = ctrlq->wrapped;

	next_pi = curr_pi + SSS_WQEBB_NUM_FOR_CTRLQ;
	if (next_pi >= wq->q_depth) {
		ctrlq->wrapped = (ctrlq->wrapped == 0) ? 1 : 0;
		next_pi -= (u16)wq->q_depth;
	}

	cmd_info = &ctrlq->cmd_info[curr_pi];

	init_completion(&done);

	cmd_info->msg_type = SSS_MSG_TYPE_SGE_RESP;
	cmd_info->done = &done;
	cmd_info->err_code = &errcode;
	cmd_info->direct_resp = out_param;
	cmd_info->cmpt_code = &cmpt_code;
	cmd_info->channel = channel;
	sss_ctrlq_set_cmd_buf(cmd_info, SSS_TO_HWDEV(ctrlq), in_buf, out_buf);

	memcpy(&store_cmd_info, cmd_info, sizeof(*cmd_info));

	sss_ctrlq_set_lcmd_wqe(&wqe, SSS_SYNC_MSG_SGE_REPLY, in_buf, out_buf,
			       wrapped, mod, cmd, curr_pi);

	sss_ctrlq_fill_wqe(curr_wqe, &wqe);

	(cmd_info->msg_id)++;
	curr_msg_id = cmd_info->msg_id;

	sss_ctrlq_set_db(ctrlq, ctrlq->ctrlq_type, next_pi);

	sss_ctrlq_msg_unlock(ctrlq);

	real_timeout = timeout ? timeout : SSS_CTRLQ_CMD_TIMEOUT;
	ret = sss_wait_ctrlq_sync_cmd_completion(ctrlq, cmd_info, &store_cmd_info,
						 curr_msg_id, curr_pi, curr_wqe, real_timeout);
	if (ret != 0) {
		sdk_err(SSS_TO_HWDEV(ctrlq)->dev_hdl,
			"Ctrlq sync cmd detail resp timeout, mod: %u, cmd: %u, pi: 0x%x\n",
			mod, cmd, curr_pi);
		ret = -ETIMEDOUT;
	}

	if (cmpt_code == SSS_CTRLQ_FORCE_STOP_CMPT_CODE) {
		sdk_info(SSS_TO_HWDEV(ctrlq)->dev_hdl, "Force stop ctrlq cmd, mod: %u, cmd: %u\n",
			 mod, cmd);
		ret = -EAGAIN;
	}

	destroy_completion(&done);
	smp_rmb(); /* read error code after completion */

	return (ret != 0) ? ret : errcode;
}

void sss_free_ctrlq_cmd_buf(struct sss_hwdev *hwdev,
			    struct sss_ctrlq_cmd_info *info)
{
	if (info->in_buf)
		sss_free_ctrlq_msg_buf(hwdev, info->in_buf);

	if (info->out_buf)
		sss_free_ctrlq_msg_buf(hwdev, info->out_buf);

	info->out_buf = NULL;
	info->in_buf = NULL;
}

static void sss_erase_wqe_complete_bit(struct sss_ctrlq *ctrlq,
				       struct sss_ctrlq_wqe *wqe, u16 ci)
{
	struct sss_wqe_ctrl *wqe_ctrl = NULL;
	u32 head = sss_hw_cpu32(SSS_WQE_HEAD(wqe)->info);
	enum sss_data_fmt format = SSS_GET_CTRLQ_WQE_HEAD(head, DATA_FMT);

	wqe_ctrl = (format == SSS_DATA_SGE) ? &wqe->wqe_lcmd.ctrl :
		   &wqe->inline_wqe.wqe_scmd.ctrl;

	wqe_ctrl->info = 0;
	ctrlq->cmd_info[ci].msg_type = SSS_MSG_TYPE_NONE;

	/* write ctrlq wqe msg type */
	wmb();

	sss_update_wq_ci(&ctrlq->wq, SSS_WQEBB_NUM_FOR_CTRLQ);
}

static void sss_ctrlq_update_cmd_info(struct sss_ctrlq *ctrlq,
				      struct sss_ctrlq_wqe *wqe, u16 ci)
{
	struct sss_ctrlq_cmd_info *info = &ctrlq->cmd_info[ci];
	struct sss_ctrlq_wqe_lcmd *lcmd = &wqe->wqe_lcmd;
	u32 status;

	spin_lock(&ctrlq->ctrlq_lock);

	if (info->direct_resp)
		*info->direct_resp =
			sss_hw_cpu32(lcmd->completion.direct_reply);

	if (info->err_code) {
		status = sss_hw_cpu32(lcmd->state.info);
		*info->err_code = SSS_GET_WQE_ERRCODE(status, VAL);
	}

	if (info->cmpt_code) {
		*info->cmpt_code = SSS_CTRLQ_COMPLETE_CODE;
		info->cmpt_code = NULL;
	}

	/* read all before set info done */
	smp_rmb();

	if (info->done) {
		complete(info->done);
		info->done = NULL;
	}

	spin_unlock(&ctrlq->ctrlq_lock);
}

static int sss_ctrlq_arm_ceq_handler(struct sss_ctrlq *ctrlq,
				     struct sss_ctrlq_wqe *wqe, u16 ci)
{
	struct sss_wqe_ctrl *ctrl = &wqe->inline_wqe.wqe_scmd.ctrl;
	u32 info = sss_hw_cpu32((ctrl)->info);

	if (!SSS_WQE_COMPLETE(info))
		return -EBUSY;

	sss_erase_wqe_complete_bit(ctrlq, wqe, ci);

	return 0;
}

static int sss_ctrlq_default_handler(struct sss_ctrlq *ctrlq,
				     struct sss_ctrlq_wqe *wqe, u16 ci)
{
	struct sss_wqe_ctrl *ctrl = &wqe->wqe_lcmd.ctrl;
	u32 info = sss_hw_cpu32((ctrl)->info);

	if (!SSS_WQE_COMPLETE(info))
		return -EBUSY;

	dma_rmb();

	sss_ctrlq_update_cmd_info(ctrlq, wqe, ci);
	sss_free_ctrlq_cmd_buf(SSS_TO_HWDEV(ctrlq), &ctrlq->cmd_info[ci]);
	sss_erase_wqe_complete_bit(ctrlq, wqe, ci);

	return 0;
}

static int sss_ctrlq_async_cmd_handler(struct sss_ctrlq *ctrlq,
				       struct sss_ctrlq_wqe *wqe, u16 ci)
{
	struct sss_wqe_ctrl *ctrl = &wqe->wqe_lcmd.ctrl;
	u32 info = sss_hw_cpu32((ctrl)->info);

	if (!SSS_WQE_COMPLETE(info))
		return -EBUSY;

	dma_rmb();

	sss_free_ctrlq_cmd_buf(SSS_TO_HWDEV(ctrlq), &ctrlq->cmd_info[ci]);
	sss_erase_wqe_complete_bit(ctrlq, wqe, ci);

	return 0;
}

static int sss_ctrlq_pseudo_timeout_handler(struct sss_ctrlq *ctrlq,
					    struct sss_ctrlq_wqe *wqe, u16 ci)
{
	sss_free_ctrlq_cmd_buf(SSS_TO_HWDEV(ctrlq), &ctrlq->cmd_info[ci]);
	sss_erase_wqe_complete_bit(ctrlq, wqe, ci);

	return 0;
}

static int sss_ctrlq_timeout_handler(struct sss_ctrlq *ctrlq,
				     struct sss_ctrlq_wqe *wqe, u16 ci)
{
	u32 i;
	u32 *data = (u32 *)wqe;
	u32 num = SSS_CTRLQ_WQE_HEAD_LEN / sizeof(u32);

	sdk_warn(SSS_TO_HWDEV(ctrlq)->dev_hdl, "Ctrlq timeout, ci: %u\n", ci);

	for (i = 0; i < num; i += 0x4) {
		sdk_info(SSS_TO_HWDEV(ctrlq)->dev_hdl, "Ctrlq wqe data: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			 *(data + i), *(data + i + 0x1), *(data + i + 0x2),
			 *(data + i + 0x3));
	}

	sss_free_ctrlq_cmd_buf(SSS_TO_HWDEV(ctrlq), &ctrlq->cmd_info[ci]);
	sss_erase_wqe_complete_bit(ctrlq, wqe, ci);

	return 0;
}

static int sss_ctrlq_force_stop_handler(struct sss_ctrlq *ctrlq,
					struct sss_ctrlq_wqe *wqe, u16 ci)
{
	return sss_ctrlq_async_cmd_handler(ctrlq, wqe, ci);
}

void sss_ctrlq_ceq_handler(void *dev, u32 data)
{
	u16 ci;
	int ret;
	enum sss_ctrlq_type type = SSS_GET_CEQE_CTRLQ(data, TYPE);
	struct sss_ctrlq *ctrlq = &SSS_TO_CTRLQ_INFO(dev)->ctrlq[type];
	struct sss_ctrlq_wqe *ctrlq_wqe = NULL;
	struct sss_ctrlq_cmd_info *info = NULL;

	sss_ctrlq_type_handler_t handler[] = {
		NULL,
		sss_ctrlq_arm_ceq_handler,
		sss_ctrlq_default_handler,
		sss_ctrlq_default_handler,
		sss_ctrlq_async_cmd_handler,
		sss_ctrlq_pseudo_timeout_handler,
		sss_ctrlq_timeout_handler,
		sss_ctrlq_force_stop_handler,
	};

	while ((ctrlq_wqe = sss_ctrlq_read_wqe(&ctrlq->wq, &ci)) != NULL) {
		info = &ctrlq->cmd_info[ci];

		if (info->msg_type < SSS_MSG_TYPE_NONE ||
		    info->msg_type >= SSS_MSG_TYPE_MAX) {
			ret = sss_ctrlq_default_handler(ctrlq, ctrlq_wqe, ci);
			if (ret)
				break;

			continue;
		}

		if (!handler[info->msg_type])
			break;

		ret = handler[info->msg_type](ctrlq, ctrlq_wqe, ci);
		if (ret)
			break;
	}
}
