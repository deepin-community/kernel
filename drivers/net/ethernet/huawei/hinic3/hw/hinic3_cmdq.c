// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [COMM]" fmt

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

#include "ossl_knl.h"

#include "hinic3_crm.h"
#include "hinic3_hw.h"
#include "hinic3_hwdev.h"
#include "hinic3_eqs.h"
#include "hinic3_common.h"
#include "hinic3_wq.h"
#include "hinic3_hw_comm.h"
#include "hinic3_hwif.h"
#include "npu_cmdq_base_defs.h"
#include "hinic3_cmdq.h"

#define CMDQ_CMD_TIMEOUT				5000 /* millisecond */
#define CMDQ_CMD_RETRY_TIMEOUT				1000U

#define UPPER_8_BITS(data)				(((data) >> 8) & 0xFF)
#define LOWER_8_BITS(data)				((data) & 0xFF)

#define CMDQ_DB_INFO_HI_PROD_IDX_SHIFT			0
#define CMDQ_DB_INFO_HI_PROD_IDX_MASK			0xFFU
#define CMDQ_DB_INFO_SET(val, member)			\
			((((u32)(val)) & CMDQ_DB_INFO_##member##_MASK) << \
			 CMDQ_DB_INFO_##member##_SHIFT)

#define CMDQ_DB_HEAD_QUEUE_TYPE_SHIFT			23
#define CMDQ_DB_HEAD_CMDQ_TYPE_SHIFT			24
#define CMDQ_DB_HEAD_SRC_TYPE_SHIFT			27
#define CMDQ_DB_HEAD_QUEUE_TYPE_MASK			0x1U
#define CMDQ_DB_HEAD_CMDQ_TYPE_MASK			0x7U
#define CMDQ_DB_HEAD_SRC_TYPE_MASK			0x1FU
#define CMDQ_DB_HEAD_SET(val, member)			\
			((((u32)(val)) & CMDQ_DB_HEAD_##member##_MASK) << \
			 CMDQ_DB_HEAD_##member##_SHIFT)

#define CMDQ_CTRL_PI_SHIFT				0
#define CMDQ_CTRL_CMD_SHIFT				16
#define CMDQ_CTRL_MOD_SHIFT				24
#define CMDQ_CTRL_ACK_TYPE_SHIFT			29
#define CMDQ_CTRL_HW_BUSY_BIT_SHIFT			31

#define CMDQ_CTRL_PI_MASK				0xFFFFU
#define CMDQ_CTRL_CMD_MASK				0xFFU
#define CMDQ_CTRL_MOD_MASK				0x1FU
#define CMDQ_CTRL_ACK_TYPE_MASK				0x3U
#define CMDQ_CTRL_HW_BUSY_BIT_MASK			0x1U

#define CMDQ_CTRL_SET(val, member)			\
			((((u32)(val)) & CMDQ_CTRL_##member##_MASK) << \
			 CMDQ_CTRL_##member##_SHIFT)

#define CMDQ_CTRL_GET(val, member)			\
			(((val) >> CMDQ_CTRL_##member##_SHIFT) & \
			 CMDQ_CTRL_##member##_MASK)

#define CMDQ_WQE_HEADER_BUFDESC_LEN_SHIFT		0
#define CMDQ_WQE_HEADER_COMPLETE_FMT_SHIFT		15
#define CMDQ_WQE_HEADER_DATA_FMT_SHIFT			22
#define CMDQ_WQE_HEADER_COMPLETE_REQ_SHIFT		23
#define CMDQ_WQE_HEADER_COMPLETE_SECT_LEN_SHIFT		27
#define CMDQ_WQE_HEADER_CTRL_LEN_SHIFT			29
#define CMDQ_WQE_HEADER_HW_BUSY_BIT_SHIFT		31

#define CMDQ_WQE_HEADER_BUFDESC_LEN_MASK		0xFFU
#define CMDQ_WQE_HEADER_COMPLETE_FMT_MASK		0x1U
#define CMDQ_WQE_HEADER_DATA_FMT_MASK			0x1U
#define CMDQ_WQE_HEADER_COMPLETE_REQ_MASK		0x1U
#define CMDQ_WQE_HEADER_COMPLETE_SECT_LEN_MASK		0x3U
#define CMDQ_WQE_HEADER_CTRL_LEN_MASK			0x3U
#define CMDQ_WQE_HEADER_HW_BUSY_BIT_MASK		0x1U

#define CMDQ_WQE_HEADER_SET(val, member)		\
			((((u32)(val)) & CMDQ_WQE_HEADER_##member##_MASK) << \
			 CMDQ_WQE_HEADER_##member##_SHIFT)

#define CMDQ_WQE_HEADER_GET(val, member)		\
			(((val) >> CMDQ_WQE_HEADER_##member##_SHIFT) & \
			 CMDQ_WQE_HEADER_##member##_MASK)

#define CMDQ_CTXT_CURR_WQE_PAGE_PFN_SHIFT		0
#define CMDQ_CTXT_EQ_ID_SHIFT				53
#define CMDQ_CTXT_CEQ_ARM_SHIFT				61
#define CMDQ_CTXT_CEQ_EN_SHIFT				62
#define CMDQ_CTXT_HW_BUSY_BIT_SHIFT			63

#define CMDQ_CTXT_CURR_WQE_PAGE_PFN_MASK		0xFFFFFFFFFFFFF
#define CMDQ_CTXT_EQ_ID_MASK				0xFF
#define CMDQ_CTXT_CEQ_ARM_MASK				0x1
#define CMDQ_CTXT_CEQ_EN_MASK				0x1
#define CMDQ_CTXT_HW_BUSY_BIT_MASK			0x1

#define CMDQ_CTXT_PAGE_INFO_SET(val, member)		\
			(((u64)(val) & CMDQ_CTXT_##member##_MASK) << \
			 CMDQ_CTXT_##member##_SHIFT)

#define CMDQ_CTXT_PAGE_INFO_GET(val, member)		\
			(((u64)(val) >> CMDQ_CTXT_##member##_SHIFT) & \
			 CMDQ_CTXT_##member##_MASK)

#define CMDQ_CTXT_WQ_BLOCK_PFN_SHIFT			0
#define CMDQ_CTXT_CI_SHIFT				52

#define CMDQ_CTXT_WQ_BLOCK_PFN_MASK			0xFFFFFFFFFFFFF
#define CMDQ_CTXT_CI_MASK				0xFFF

#define CMDQ_CTXT_BLOCK_INFO_SET(val, member)		\
			(((u64)(val) & CMDQ_CTXT_##member##_MASK) << \
			 CMDQ_CTXT_##member##_SHIFT)

#define CMDQ_CTXT_BLOCK_INFO_GET(val, member)		\
			(((u64)(val) >> CMDQ_CTXT_##member##_SHIFT) & \
			 CMDQ_CTXT_##member##_MASK)

#define SAVED_DATA_ARM_SHIFT				31

#define SAVED_DATA_ARM_MASK				0x1U

#define SAVED_DATA_SET(val, member)			\
			(((val) & SAVED_DATA_##member##_MASK) << \
			 SAVED_DATA_##member##_SHIFT)

#define SAVED_DATA_CLEAR(val, member)			\
			((val) & (~(SAVED_DATA_##member##_MASK << \
				    SAVED_DATA_##member##_SHIFT)))

#define WQE_ERRCODE_VAL_SHIFT				0

#define WQE_ERRCODE_VAL_MASK				0x7FFFFFFF

#define WQE_ERRCODE_GET(val, member)	\
			(((val) >> WQE_ERRCODE_##member##_SHIFT) & \
			 WQE_ERRCODE_##member##_MASK)

#define CEQE_CMDQ_TYPE_SHIFT				0

#define CEQE_CMDQ_TYPE_MASK				0x7

#define CEQE_CMDQ_GET(val, member)	\
			(((val) >> CEQE_CMDQ_##member##_SHIFT) & \
			 CEQE_CMDQ_##member##_MASK)

#define WQE_COMPLETED(ctrl_info)	CMDQ_CTRL_GET(ctrl_info, HW_BUSY_BIT)

#define WQE_HEADER(wqe)			((struct hinic3_cmdq_header *)(wqe))

#define CMDQ_DB_PI_OFF(pi)		(((u16)LOWER_8_BITS(pi)) << 3)

#define CMDQ_DB_ADDR(db_base, pi)	\
			(((u8 *)(db_base)) + CMDQ_DB_PI_OFF(pi))

#define FIRST_DATA_TO_WRITE_LAST	sizeof(u64)

#define WQE_LCMD_SIZE			64
#define WQE_SCMD_SIZE			64
#define WQE_ENHANCED_CMDQ_SIZE		32

#define COMPLETE_LEN			3

#define CMDQ_WQEBB_SIZE			64
#define CMDQ_WQE_SIZE			64
#define ENHANCE_CMDQ_WQEBB_SIZE		16

#define cmdq_to_cmdqs(cmdq)	container_of((cmdq) - (cmdq)->cmdq_type, \
					     struct hinic3_cmdqs, cmdq[0])

#define CMDQ_SEND_CMPT_CODE		10
#define CMDQ_COMPLETE_CMPT_CODE		11
#define CMDQ_FORCE_STOP_CMPT_CODE	12

enum cmdq_scmd_type {
	CMDQ_SET_ARM_CMD = 2,
};

enum cmdq_wqe_type {
	WQE_LCMD_TYPE,
	WQE_SCMD_TYPE,
};

enum ctrl_sect_len {
	CTRL_SECT_LEN = 1,
	CTRL_DIRECT_SECT_LEN = 2,
};

enum completion_format {
	COMPLETE_DIRECT,
	COMPLETE_SGE,
};

enum completion_request {
	CEQ_SET = 1,
};

#define NUM_WQEBBS_FOR_CMDQ_WQE		1
#define NUM_WQEBBS_FOR_ENHANCE_CMDQ_WQE	4

bool hinic3_cmdq_idle(struct hinic3_cmdq *cmdq)
{
	return hinic3_wq_is_empty(&cmdq->wq);
}

static void *cmdq_read_wqe(struct hinic3_wq *wq, u16 *ci)
{
	if (hinic3_wq_is_empty(wq))
		return NULL;

	return hinic3_wq_read_one_wqebb(wq, ci);
}

static void *hinic3_wq_get_align_wqebbs(struct hinic3_wq *wq, u16 *pi, u16 wqebb_num)
{
	*pi = WQ_MASK_IDX(wq, wq->prod_idx);
	wq->prod_idx += wqebb_num;

	return WQ_GET_WQEBB_ADDR(wq, WQ_PAGE_IDX(wq, *pi),
				 WQ_OFFSET_IN_PAGE(wq, *pi));
}

static void *cmdq_get_wqe(struct hinic3_wq *wq, u16 *pi, u16 wqebb_use_num)
{
	if (hinic3_wq_free_wqebbs(wq) < wqebb_use_num)
		return NULL;

	return hinic3_wq_get_align_wqebbs(wq, pi, wqebb_use_num);
}

struct hinic3_cmd_buf *hinic3_alloc_cmd_buf(void *hwdev)
{
	struct hinic3_cmdqs *cmdqs = NULL;
	struct hinic3_cmd_buf *cmd_buf = NULL;
	void *dev = NULL;

	if (!hwdev) {
		pr_err("Failed to alloc cmd buf, Invalid hwdev\n");
		return NULL;
	}

	cmdqs = ((struct hinic3_hwdev *)hwdev)->cmdqs;
	dev = ((struct hinic3_hwdev *)hwdev)->dev_hdl;
	if (cmdqs == NULL || dev == NULL) {
		pr_err("Failed to alloc cmd buf, Invalid hwdev cmdqs or dev\n");
		return NULL;
	}

	cmd_buf = kzalloc(sizeof(*cmd_buf), GFP_ATOMIC);
	if (!cmd_buf)
		return NULL;

	cmd_buf->buf = dma_pool_alloc(cmdqs->cmd_buf_pool, GFP_ATOMIC,
				      &cmd_buf->dma_addr);
	if (!cmd_buf->buf) {
		sdk_err(dev, "Failed to allocate cmdq cmd buf from the pool\n");
		goto alloc_pci_buf_err;
	}

	cmd_buf->size = (u16)cmdqs->cmd_buf_size;
	atomic_set(&cmd_buf->ref_cnt, 1);

	return cmd_buf;

alloc_pci_buf_err:
	kfree(cmd_buf);
	return NULL;
}
EXPORT_SYMBOL(hinic3_alloc_cmd_buf);

void hinic3_free_cmd_buf(void *hwdev, struct hinic3_cmd_buf *cmd_buf)
{
	struct hinic3_cmdqs *cmdqs = NULL;

	if (!hwdev || !cmd_buf) {
		pr_err("Failed to free cmd buf, hwdev or cmd_buf is NULL\n");
		return;
	}

	if (!atomic_dec_and_test(&cmd_buf->ref_cnt))
		return;

	cmdqs = ((struct hinic3_hwdev *)hwdev)->cmdqs;

	dma_pool_free(cmdqs->cmd_buf_pool, cmd_buf->buf, cmd_buf->dma_addr);
	kfree(cmd_buf);
}
EXPORT_SYMBOL(hinic3_free_cmd_buf);

static void cmdq_set_completion(struct hinic3_cmdq_completion *complete,
				struct hinic3_cmd_buf *buf_out)
{
	struct hinic3_sge_resp *sge_resp = &complete->sge_resp;

	hinic3_set_sge(&sge_resp->sge, buf_out->dma_addr, buf_out->size);
}

static void cmdq_set_lcmd_bufdesc(struct hinic3_cmdq_wqe_lcmd *wqe,
				  struct hinic3_cmd_buf *buf_in)
{
	hinic3_set_sge(&wqe->buf_desc.sge, buf_in->dma_addr, buf_in->size);
}

static void cmdq_fill_db(struct hinic3_cmdq_db *db,
			 enum hinic3_cmdq_type cmdq_type, u16 prod_idx)
{
	db->db_info = CMDQ_DB_INFO_SET(UPPER_8_BITS(prod_idx), HI_PROD_IDX);

	db->db_head = CMDQ_DB_HEAD_SET(HINIC3_DB_CMDQ_TYPE, QUEUE_TYPE) |
			CMDQ_DB_HEAD_SET(cmdq_type, CMDQ_TYPE)		|
			CMDQ_DB_HEAD_SET(HINIC3_DB_SRC_CMDQ_TYPE, SRC_TYPE);
}

static void cmdq_set_db(struct hinic3_cmdq *cmdq,
			enum hinic3_cmdq_type cmdq_type, u16 prod_idx)
{
	struct hinic3_cmdq_db db = {0};
	u8 *db_base = cmdq->hwdev->cmdqs->cmdqs_db_base;

	cmdq_fill_db(&db, cmdq_type, prod_idx);

	/* The data that is written to HW should be in Big Endian Format */
	db.db_info = hinic3_hw_be32(db.db_info);
	db.db_head = hinic3_hw_be32(db.db_head);

	wmb();    /* write all before the doorbell */
	writeq(*((u64 *)&db), CMDQ_DB_ADDR(db_base, prod_idx));
}

static void cmdq_wqe_fill(void *dst, const void *src, int wqe_size)
{
	memcpy((u8 *)dst + FIRST_DATA_TO_WRITE_LAST,
	       (u8 *)src + FIRST_DATA_TO_WRITE_LAST,
	       wqe_size - FIRST_DATA_TO_WRITE_LAST);

	wmb(); /* The first 8 bytes should be written last */

	*(u64 *)dst = *(u64 *)src;
}

static void cmdq_prepare_wqe_ctrl(struct hinic3_cmdq_wqe *wqe, int wrapped,
				  u8 mod, u8 cmd, u16 prod_idx,
				  enum completion_format complete_format,
				  enum data_format data_format,
				  enum bufdesc_len buf_len)
{
	struct hinic3_ctrl *ctrl = NULL;
	enum ctrl_sect_len ctrl_len;
	struct hinic3_cmdq_wqe_lcmd *wqe_lcmd = NULL;
	struct hinic3_cmdq_wqe_scmd *wqe_scmd = NULL;
	u32 saved_data = WQE_HEADER(wqe)->saved_data;

	if (data_format == DATA_SGE) {
		wqe_lcmd = &wqe->wqe_lcmd;

		wqe_lcmd->status.status_info = 0;
		ctrl = &wqe_lcmd->ctrl;
		ctrl_len = CTRL_SECT_LEN;
	} else {
		wqe_scmd = &wqe->inline_wqe.wqe_scmd;

		wqe_scmd->status.status_info = 0;
		ctrl = &wqe_scmd->ctrl;
		ctrl_len = CTRL_DIRECT_SECT_LEN;
	}

	ctrl->ctrl_info = CMDQ_CTRL_SET(prod_idx, PI)		|
			CMDQ_CTRL_SET(cmd, CMD)			|
			CMDQ_CTRL_SET(mod, MOD)			|
			CMDQ_CTRL_SET(HINIC3_ACK_TYPE_CMDQ, ACK_TYPE);

	WQE_HEADER(wqe)->header_info =
		CMDQ_WQE_HEADER_SET(buf_len, BUFDESC_LEN)	|
		CMDQ_WQE_HEADER_SET(complete_format, COMPLETE_FMT) |
		CMDQ_WQE_HEADER_SET(data_format, DATA_FMT)	|
		CMDQ_WQE_HEADER_SET(CEQ_SET, COMPLETE_REQ)	|
		CMDQ_WQE_HEADER_SET(COMPLETE_LEN, COMPLETE_SECT_LEN) |
		CMDQ_WQE_HEADER_SET(ctrl_len, CTRL_LEN)		|
		CMDQ_WQE_HEADER_SET((u32)wrapped, HW_BUSY_BIT);

	if (cmd == CMDQ_SET_ARM_CMD && mod == HINIC3_MOD_COMM) {
		saved_data &= SAVED_DATA_CLEAR(saved_data, ARM);
		WQE_HEADER(wqe)->saved_data = saved_data	|
						SAVED_DATA_SET(1, ARM);
	} else {
		saved_data &= SAVED_DATA_CLEAR(saved_data, ARM);
		WQE_HEADER(wqe)->saved_data = saved_data;
	}
}

static void cmdq_set_lcmd_wqe(struct hinic3_cmdq_wqe *wqe,
			      enum hinic3_cmdq_cmd_type cmd_type,
			      struct hinic3_cmd_buf *buf_in,
			      struct hinic3_cmd_buf *buf_out, int wrapped,
			      u8 mod, u8 cmd, u16 prod_idx)
{
	struct hinic3_cmdq_wqe_lcmd *wqe_lcmd = &wqe->wqe_lcmd;
	enum completion_format complete_format = COMPLETE_DIRECT;

	switch (cmd_type) {
	case HINIC3_CMD_TYPE_DIRECT_RESP:
		wqe_lcmd->completion.direct_resp = 0;
		break;
	case HINIC3_CMD_TYPE_SGE_RESP:
		if (buf_out) {
			complete_format = COMPLETE_SGE;
			cmdq_set_completion(&wqe_lcmd->completion,
					    buf_out);
		}
		break;
	case HINIC3_CMD_TYPE_ASYNC:
		wqe_lcmd->completion.direct_resp = 0;
		wqe_lcmd->buf_desc.saved_async_buf = (u64)(buf_in);
		break;
	default:
		break;
	}

	cmdq_prepare_wqe_ctrl(wqe, wrapped, mod, cmd, prod_idx, complete_format,
			      DATA_SGE, BUFDESC_LCMD_LEN);

	cmdq_set_lcmd_bufdesc(wqe_lcmd, buf_in);
}

static void cmdq_update_cmd_status(struct hinic3_cmdq *cmdq, u16 prod_idx,
				   struct hinic3_cmdq_wqe *wqe)
{
	struct hinic3_cmdq_cmd_info *cmd_info = NULL;
	struct hinic3_cmdq_wqe_lcmd *wqe_lcmd = NULL;
	u32 status_info;
	u64 *direct_resp = NULL;
	u32 error_status;

	cmd_info = &cmdq->cmd_infos[prod_idx];

	if (!cmd_info->errcode) {
		sdk_err(cmdq->hwdev->dev_hdl, "cmd_info->errcode = NULL\n");
		return;
	}

	if (cmdq->hwdev->cmdq_mode == HINIC3_NORMAL_CMDQ) {
		wqe_lcmd = &wqe->wqe_lcmd;
		status_info = hinic3_hw_cpu32(wqe_lcmd->status.status_info);
		*cmd_info->errcode = WQE_ERRCODE_GET(status_info, VAL);

		if (cmd_info->direct_resp) {
			*cmd_info->direct_resp = hinic3_hw_cpu32(wqe_lcmd->completion.direct_resp);
			if ((*cmd_info->errcode != 0) && (*cmd_info->direct_resp != 0)) {
				sdk_err(cmdq->hwdev->dev_hdl, "Cmdq resp err=0x%llx\n",
					*cmd_info->direct_resp);
			}
		}
	} else {
		status_info = hinic3_hw_cpu32(wqe->enhanced_cmdq_wqe.completion.cs_format);
		*cmd_info->errcode = ENHANCE_CMDQ_WQE_CS_GET(status_info, ERR_CODE);
		if (*cmd_info->errcode != 0) {
			error_status =
				hinic3_hw_cpu32(wqe->enhanced_cmdq_wqe.completion.sge_resp_hi_addr);
			sdk_err(cmdq->hwdev->dev_hdl, "Cmdq error code 0x%x, error status 0x%x\n",
				*cmd_info->errcode, error_status);
		}

		if (cmd_info->direct_resp) {
			direct_resp = (u64 *)(&wqe->enhanced_cmdq_wqe.completion.sge_resp_lo_addr);
			*cmd_info->direct_resp = hinic3_hw_cpu32(*direct_resp);
			if ((*cmd_info->errcode != 0) && (*cmd_info->direct_resp != 0)) {
				sdk_err(cmdq->hwdev->dev_hdl, "Cmdq resp err=0x%llx\n",
					*cmd_info->direct_resp);
			}
		}
	}
}

static int hinic3_cmdq_sync_timeout_check(struct hinic3_cmdq *cmdq,
					  struct hinic3_cmdq_wqe *wqe, u16 pi)
{
	struct hinic3_cmdq_wqe_lcmd *wqe_lcmd = NULL;
	struct hinic3_ctrl *ctrl = NULL;
	u32 ctrl_info;

	if (cmdq->hwdev->cmdq_mode == HINIC3_NORMAL_CMDQ) {
		/* only arm bit is using scmd wqe, the wqe is lcmd */
		wqe_lcmd = &wqe->wqe_lcmd;
		ctrl = &wqe_lcmd->ctrl;
		ctrl_info = hinic3_hw_cpu32((ctrl)->ctrl_info);

		if (WQE_COMPLETED(ctrl_info) == 0) {
			sdk_info(cmdq->hwdev->dev_hdl, "Cmdq sync command check busy bit not set\n");
			return -EFAULT;
		}
	} else {
		ctrl_info = hinic3_hw_cpu32(wqe->enhanced_cmdq_wqe.completion.cs_format);
		if (ENHANCE_CMDQ_WQE_CS_GET(ctrl_info, HW_BUSY) == 0) {
			sdk_info(cmdq->hwdev->dev_hdl, "enhance Cmdq sync command check busy bit not set\n");
			return -EFAULT;
		}
	}

	cmdq_update_cmd_status(cmdq, pi, wqe);

	sdk_info(cmdq->hwdev->dev_hdl, "Cmdq sync command check succeed\n");
	return 0;
}

static void clear_cmd_info(struct hinic3_cmdq_cmd_info *cmd_info,
			   const struct hinic3_cmdq_cmd_info *saved_cmd_info)
{
	if (cmd_info->errcode == saved_cmd_info->errcode)
		cmd_info->errcode = NULL;

	if (cmd_info->done == saved_cmd_info->done)
		cmd_info->done = NULL;

	if (cmd_info->direct_resp == saved_cmd_info->direct_resp)
		cmd_info->direct_resp = NULL;
}

static int wait_for_cmdq_timeout(struct hinic3_cmdq *cmdq,
				struct hinic3_cmdq_cmd_info *cmd_info,
				ulong timeout)
{
	ulong timeo, end;

	if (cmdq->cmdqs->poll) {
		end = jiffies + msecs_to_jiffies((unsigned int)timeout);
		while (time_before(jiffies, end)) {
			/* must lock cmdq when poll cqe handle */
			spin_lock_bh(&cmdq->cmdq_lock);
			hinic3_cmdq_ceq_handler(cmdq->hwdev, 0);
			spin_unlock_bh(&cmdq->cmdq_lock);

			if (try_wait_for_completion(cmd_info->done) != 0)
				return 0;

			usleep_range(9, 10); /* sleep 9 us ~ 10 us */
		}
	} else {
		timeo = msecs_to_jiffies((unsigned int)timeout);
		if (wait_for_completion_timeout(cmd_info->done, timeo) != 0)
			return 0;
	}

	return -ETIMEDOUT;
}

static int cmdq_retry_get_ack(struct hinic3_cmdq *cmdq,
			      struct hinic3_cmdq_cmd_info *cmd_info, u8 ceq_id)
{
	ulong retry_timeout = msecs_to_jiffies(CMDQ_CMD_RETRY_TIMEOUT);
	int err;

	spin_lock_bh(&cmdq->cmdq_lock);
	if (try_wait_for_completion(cmd_info->done)) {
		spin_unlock_bh(&cmdq->cmdq_lock);
		return 0;
	}
	reinit_completion(cmd_info->done);
	spin_unlock_bh(&cmdq->cmdq_lock);

	err = hinic3_reschedule_eq(cmdq->hwdev, HINIC3_CEQ, ceq_id);
	if (err != 0)
		return err;

	if (wait_for_cmdq_timeout(cmdq, cmd_info, retry_timeout) == 0)
		return 0;

	return -ETIMEDOUT;
}

static int cmdq_ceq_handler_status(struct hinic3_cmdq *cmdq,
				   struct hinic3_cmdq_cmd_info *cmd_info,
				   struct hinic3_cmdq_cmd_info *saved_cmd_info,
				   u64 curr_msg_id, u16 curr_prod_idx,
				   struct hinic3_cmdq_wqe *curr_wqe,
				   u32 timeout)
{
	int err;

	err = wait_for_cmdq_timeout(cmdq, saved_cmd_info, timeout);
	if (err == 0)
		return 0;

	if (!cmdq->cmdqs->poll) {
		sdk_warn(cmdq->hwdev->dev_hdl,
			 "Cmdq retry cmd(type %u, channel %u), msg_id %llu, pi %u\n",
			 saved_cmd_info->cmd_type, saved_cmd_info->channel, curr_msg_id,
			 curr_prod_idx);

		err = cmdq_retry_get_ack(cmdq, saved_cmd_info, HINIC3_CEQ_ID_CMDQ);
		if (err == 0)
			return 0;
	}

	spin_lock_bh(&cmdq->cmdq_lock);

	if (cmd_info->cmpt_code == saved_cmd_info->cmpt_code)
		cmd_info->cmpt_code = NULL;

	if (*saved_cmd_info->cmpt_code == CMDQ_COMPLETE_CMPT_CODE) {
		sdk_info(cmdq->hwdev->dev_hdl, "Cmdq direct sync command has been completed\n");
		spin_unlock_bh(&cmdq->cmdq_lock);
		return 0;
	}

	if (curr_msg_id == cmd_info->cmdq_msg_id) {
		err = hinic3_cmdq_sync_timeout_check(cmdq, curr_wqe,
						     curr_prod_idx);
		if (err != 0)
			cmd_info->cmd_type = HINIC3_CMD_TYPE_TIMEOUT;
		else
			cmd_info->cmd_type = HINIC3_CMD_TYPE_FAKE_TIMEOUT;
	} else {
		err = -ETIMEDOUT;
		sdk_err(cmdq->hwdev->dev_hdl, "Cmdq sync command current msg id dismatch with cmd_info msg id\n");
	}

	clear_cmd_info(cmd_info, saved_cmd_info);

	spin_unlock_bh(&cmdq->cmdq_lock);

	if (err == 0)
		return 0;

	hinic3_dump_ceq_info(cmdq->hwdev);

	return -ETIMEDOUT;
}

static int wait_cmdq_sync_cmd_completion(struct hinic3_cmdq *cmdq,
					 struct hinic3_cmdq_cmd_info *cmd_info,
					 struct hinic3_cmdq_cmd_info *saved_cmd_info,
					 u64 curr_msg_id, u16 curr_prod_idx,
					 struct hinic3_cmdq_wqe *curr_wqe, u32 timeout)
{
	return cmdq_ceq_handler_status(cmdq, cmd_info, saved_cmd_info,
				       curr_msg_id, curr_prod_idx,
				       curr_wqe, timeout);
}

static int cmdq_msg_lock(struct hinic3_cmdq *cmdq, u16 channel)
{
	struct hinic3_cmdqs *cmdqs = cmdq_to_cmdqs(cmdq);
	if (cmdqs == NULL)
		return -EINVAL;

	/* Keep wrapped and doorbell index correct. bh - for tasklet(ceq) */
	spin_lock_bh(&cmdq->cmdq_lock);

	if (cmdqs->lock_channel_en && test_bit(channel, &cmdqs->channel_stop)) {
		spin_unlock_bh(&cmdq->cmdq_lock);
		return -EAGAIN;
	}

	return 0;
}

static void cmdq_msg_unlock(struct hinic3_cmdq *cmdq)
{
	spin_unlock_bh(&cmdq->cmdq_lock);
}

static void cmdq_clear_cmd_buf(struct hinic3_cmdq_cmd_info *cmd_info,
			       struct hinic3_hwdev *hwdev)
{
	if (cmd_info->buf_in)
		hinic3_free_cmd_buf(hwdev, cmd_info->buf_in);

	if (cmd_info->buf_out)
		hinic3_free_cmd_buf(hwdev, cmd_info->buf_out);

	cmd_info->buf_in = NULL;
	cmd_info->buf_out = NULL;
}

static void cmdq_update_next_prod_idx(struct hinic3_cmdq *cmdq, u16 curr_pi, u16 *next_pi,
				      u16 wqebb_use_num)
{
	u16 q_depth = (u16)cmdq->wq.q_depth;

	*next_pi = curr_pi + wqebb_use_num;
	if (*next_pi >= q_depth) {
		cmdq->wrapped = (cmdq->wrapped == 0) ? 1 : 0;
		*next_pi -= (u16)q_depth;
	}
}

static void cmdq_set_cmd_buf(struct hinic3_cmdq_cmd_info *cmd_info,
			     struct hinic3_hwdev *hwdev,
			     struct hinic3_cmd_buf *buf_in,
			     struct hinic3_cmd_buf *buf_out)
{
	cmd_info->buf_in = buf_in;
	cmd_info->buf_out = buf_out;

	if (buf_in)
		atomic_inc(&buf_in->ref_cnt);

	if (buf_out)
		atomic_inc(&buf_out->ref_cnt);
}

static void cmdq_sync_wqe_prepare(struct hinic3_cmdq *cmdq, u8 mod, u8 cmd,
				  struct hinic3_cmd_buf *buf_in, struct hinic3_cmd_buf *buf_out,
				  struct hinic3_cmdq_wqe *curr_wqe, u16 curr_pi,
				  enum hinic3_cmdq_cmd_type nic_cmd_type)
{
	struct hinic3_cmdq_wqe wqe;
	struct hinic3_cmdq_cmd_param cmd_buf;
	int wrapped, wqe_size;

	if (cmdq->cmdqs->cmdq_mode == HINIC3_ENHANCE_CMDQ) {
		wqe_size = WQE_ENHANCED_CMDQ_SIZE;

		/* enhance cmdq wqe_size aligned with 64 */
		wqe_size = ALIGN(wqe_size, 64);
	} else {
		wqe_size = WQE_LCMD_SIZE;
	}

	memset(&wqe, 0, (u32)wqe_size);

	wrapped = cmdq->wrapped;

	if (cmdq->cmdqs->cmdq_mode == HINIC3_NORMAL_CMDQ) {
		cmdq_set_lcmd_wqe(&wqe, nic_cmd_type, buf_in, buf_out, wrapped, mod, cmd, curr_pi);
	} else {
		cmd_buf.buf_in = buf_in;
		cmd_buf.buf_out = buf_out;
		cmd_buf.cmd = cmd;
		cmd_buf.mod = mod;
		enhanced_cmdq_set_wqe(&wqe, nic_cmd_type, &cmd_buf, wrapped);
	}

	/* The data that is written to HW should be in Big Endian Format */
	hinic3_hw_be32_len(&wqe, wqe_size);

	cmdq_wqe_fill(curr_wqe, &wqe, wqe_size);
}

static inline void hinic3_cmdq_fill_cmd_info(struct hinic3_cmdq_cmd_info *cmd_info,
					     enum hinic3_cmdq_cmd_type nic_cmd_type, u16 channel,
					     u16 wqebb_use_num)
{
	cmd_info->cmd_type = nic_cmd_type;
	cmd_info->channel = channel;
	cmd_info->wqebb_use_num = wqebb_use_num;
}

static inline void hinic3_cmdq_fill_completion_info(struct hinic3_cmdq_cmd_info *cmd_info,
						    int *cmpt_code, struct completion *done,
						    int *errcode, u64 *out_param)
{
	cmd_info->done = done;
	cmd_info->errcode = errcode;
	cmd_info->direct_resp = out_param;
	cmd_info->cmpt_code = cmpt_code;
}

static int cmdq_sync_cmd(struct hinic3_cmdq *cmdq, u8 mod, u8 cmd,
				     struct hinic3_cmd_buf *buf_in, struct hinic3_cmd_buf *buf_out,
				     u64 *out_param, u32 timeout, u16 channel,
					 enum hinic3_cmdq_cmd_type nic_cmd_type)
{
	struct hinic3_wq *wq = &cmdq->wq;
	struct hinic3_cmdq_wqe *curr_wqe = NULL;
	struct hinic3_cmdq_cmd_info *cmd_info = NULL, saved_cmd_info;
	struct completion done;
	u16 curr_pi, next_pi, wqebb_use_num;
	int errcode = 0;
	int cmpt_code = CMDQ_SEND_CMPT_CODE;
	u64 curr_msg_id;
	int err;
	u32 real_timeout;

	err = cmdq_msg_lock(cmdq, channel);
	if (err != 0)
		return err;

	wqebb_use_num = cmdq->cmdqs->wqebb_use_num;
	curr_wqe = cmdq_get_wqe(wq, &curr_pi, wqebb_use_num);
	if (!curr_wqe) {
		cmdq_msg_unlock(cmdq);
		return -EBUSY;
	}

	init_completion(&done);
	cmd_info = &cmdq->cmd_infos[curr_pi];
	hinic3_cmdq_fill_cmd_info(cmd_info, nic_cmd_type, channel, wqebb_use_num);
	hinic3_cmdq_fill_completion_info(cmd_info, &cmpt_code, &done, &errcode, out_param);

	cmdq_set_cmd_buf(cmd_info, cmdq->hwdev, buf_in, buf_out);
	memcpy(&saved_cmd_info, cmd_info, sizeof(struct hinic3_cmdq_cmd_info));

	cmdq_sync_wqe_prepare(cmdq, mod, cmd, buf_in, buf_out, curr_wqe, curr_pi, nic_cmd_type);

	(cmd_info->cmdq_msg_id)++;
	curr_msg_id = cmd_info->cmdq_msg_id;

	cmdq_update_next_prod_idx(cmdq, curr_pi, &next_pi, wqebb_use_num);
	cmdq_set_db(cmdq, cmdq->cmdq_type, next_pi);

	cmdq_msg_unlock(cmdq);

	real_timeout = (timeout != 0) ? timeout : CMDQ_CMD_TIMEOUT;
	err = wait_cmdq_sync_cmd_completion(cmdq, cmd_info, &saved_cmd_info,
					curr_msg_id, curr_pi, curr_wqe, real_timeout);
	if (err != 0) {
		sdk_err(cmdq->hwdev->dev_hdl, "Cmdq sync cmd(mod: %u, cmd: %u) timeout, pi: 0x%x\n",
				mod, cmd, curr_pi);
		err = -ETIMEDOUT;
	}

	if (cmpt_code == CMDQ_FORCE_STOP_CMPT_CODE) {
		sdk_info(cmdq->hwdev->dev_hdl, "Force stop cmdq cmd, mod: %u, cmd: %u\n", mod, cmd);
		err = -EAGAIN;
	}

	destroy_completion(&done);
	smp_rmb(); /* read error code after completion */

	return (err != 0) ? err : errcode;
}

static int cmdq_sync_cmd_direct_resp(struct hinic3_cmdq *cmdq, u8 mod, u8 cmd,
				     struct hinic3_cmd_buf *buf_in, u64 *out_param,
					 u32 timeout, u16 channel)
{
	return cmdq_sync_cmd(cmdq, mod, cmd, buf_in, NULL,
					out_param, timeout, channel,
					HINIC3_CMD_TYPE_DIRECT_RESP);
}

static int cmdq_sync_cmd_detail_resp(struct hinic3_cmdq *cmdq, u8 mod, u8 cmd,
				     struct hinic3_cmd_buf *buf_in,
				     struct hinic3_cmd_buf *buf_out,
				     u64 *out_param, u32 timeout, u16 channel)
{
	return cmdq_sync_cmd(cmdq, mod, cmd, buf_in, buf_out,
			     out_param, timeout, channel,
			     HINIC3_CMD_TYPE_SGE_RESP);
}

static int cmdq_async_cmd(struct hinic3_cmdq *cmdq, u8 mod, u8 cmd,
			  struct hinic3_cmd_buf *buf_in, u16 channel)
{
	struct hinic3_cmdq_cmd_info *cmd_info = NULL;
	struct hinic3_wq *wq = &cmdq->wq;
	int wqe_size;
	u16 curr_prod_idx, next_prod_idx, wqebb_use_num;
	struct hinic3_cmdq_wqe *curr_wqe = NULL, wqe;
	int wrapped, err;

	wqe_size = cmdq->cmdqs->cmdq_mode == HINIC3_NORMAL_CMDQ ?
		WQE_LCMD_SIZE : WQE_ENHANCED_CMDQ_SIZE;

	err = cmdq_msg_lock(cmdq, channel);
	if (err != 0)
		return err;

	wqebb_use_num = cmdq->cmdqs->wqebb_use_num;
	curr_wqe = cmdq_get_wqe(wq, &curr_prod_idx, wqebb_use_num);
	if (!curr_wqe) {
		cmdq_msg_unlock(cmdq);
		return -EBUSY;
	}

	memset(&wqe, 0, sizeof(wqe));

	wrapped = cmdq->wrapped;

	cmdq_update_next_prod_idx(cmdq, curr_prod_idx, &next_prod_idx, wqebb_use_num);

	cmdq_set_lcmd_wqe(&wqe, HINIC3_CMD_TYPE_ASYNC, buf_in, NULL, wrapped,
			  mod, cmd, curr_prod_idx);

	/* The data that is written to HW should be in Big Endian Format */
	hinic3_hw_be32_len(&wqe, wqe_size);
	cmdq_wqe_fill(curr_wqe, &wqe, wqe_size);

	cmd_info = &cmdq->cmd_infos[curr_prod_idx];
	cmd_info->cmd_type = HINIC3_CMD_TYPE_ASYNC;
	cmd_info->channel = channel;
	cmd_info->wqebb_use_num = wqebb_use_num;
	/* The caller will not free the cmd_buf of the asynchronous command,
	 * so there is no need to increase the reference count here
	 */
	cmd_info->buf_in = buf_in;

	cmdq_set_db(cmdq, HINIC3_CMDQ_SYNC, next_prod_idx);

	cmdq_msg_unlock(cmdq);

	return 0;
}

static int cmdq_params_valid(const void *hwdev, const struct hinic3_cmd_buf *buf_in)
{
	struct hinic3_cmdqs *cmdqs = NULL;

	if (!buf_in || !hwdev) {
		pr_err("Invalid CMDQ buffer addr or hwdev\n");
		return -EINVAL;
	}

	cmdqs = ((struct hinic3_hwdev *)hwdev)->cmdqs;
	if (!cmdqs || (buf_in->size < HINIC3_CMDQ_MIN_BUF_SIZE) ||
		(buf_in->size > cmdqs->cmd_buf_size)) {
		pr_err("Invalid cmdqs addr or CMDQ buffer size: 0x%x\n", buf_in->size);
		return -EINVAL;
	}

	return 0;
}

#define WAIT_CMDQ_ENABLE_TIMEOUT	300
static int wait_cmdqs_enable(struct hinic3_cmdqs *cmdqs)
{
	unsigned long end;
	if (cmdqs == NULL)
		return -EINVAL;

	end = jiffies + msecs_to_jiffies(WAIT_CMDQ_ENABLE_TIMEOUT);
	do {
		if (cmdqs->status & HINIC3_CMDQ_ENABLE)
			return 0;
	} while (time_before(jiffies, end) && cmdqs->hwdev->chip_present_flag &&
		 (cmdqs->disable_flag == 0));

	cmdqs->disable_flag = 1;

	return -EBUSY;
}

int hinic3_cmdq_direct_resp(void *hwdev, u8 mod, u8 cmd,
			    struct hinic3_cmd_buf *buf_in,
			    u64 *out_param, u32 timeout, u16 channel)
{
	struct hinic3_cmdqs *cmdqs = NULL;
	int err;

	err = cmdq_params_valid(hwdev, buf_in);
	if (err != 0) {
		pr_err("Invalid CMDQ parameters\n");
		return err;
	}

	if (!get_card_present_state((struct hinic3_hwdev *)hwdev))
		return -EPERM;

	cmdqs = ((struct hinic3_hwdev *)hwdev)->cmdqs;
	err = wait_cmdqs_enable(cmdqs);
	if (err != 0) {
		sdk_err(cmdqs->hwdev->dev_hdl, "Cmdq is disable\n");
		return err;
	}

	err = cmdq_sync_cmd_direct_resp(&cmdqs->cmdq[HINIC3_CMDQ_SYNC],
					mod, cmd, buf_in, out_param,
					timeout, channel);
	if (err != 0) {
		sdk_err(cmdqs->hwdev->dev_hdl, "Cmdq direct_resp fail\n");
		return err;
	}

	if ((((struct hinic3_hwdev *)hwdev)->chip_present_flag) == 0)
		return -ETIMEDOUT;
	else
		return err;
}
EXPORT_SYMBOL(hinic3_cmdq_direct_resp);

int hinic3_cmdq_detail_resp(void *hwdev, u8 mod, u8 cmd,
			    struct hinic3_cmd_buf *buf_in,
			    struct hinic3_cmd_buf *buf_out,
			    u64 *out_param, u32 timeout, u16 channel)
{
	struct hinic3_cmdqs *cmdqs = NULL;
	int err;

	err = cmdq_params_valid(hwdev, buf_in);
	if (err)
		return err;

	cmdqs = ((struct hinic3_hwdev *)hwdev)->cmdqs;

	if (!get_card_present_state((struct hinic3_hwdev *)hwdev))
		return -EPERM;

	err = wait_cmdqs_enable(cmdqs);
	if (err) {
		sdk_err(cmdqs->hwdev->dev_hdl, "Cmdq is disable\n");
		return err;
	}

	err = cmdq_sync_cmd_detail_resp(&cmdqs->cmdq[HINIC3_CMDQ_SYNC],
					mod, cmd, buf_in, buf_out, out_param,
					timeout, channel);
	if (!(((struct hinic3_hwdev *)hwdev)->chip_present_flag))
		return -ETIMEDOUT;
	else
		return err;
}
EXPORT_SYMBOL(hinic3_cmdq_detail_resp);

int hinic3_cos_id_detail_resp(void *hwdev, u8 mod, u8 cmd, u8 cos_id,
			      struct hinic3_cmd_buf *buf_in,
			      struct hinic3_cmd_buf *buf_out, u64 *out_param,
			      u32 timeout, u16 channel)
{
	struct hinic3_cmdqs *cmdqs = NULL;
	int err;

	err = cmdq_params_valid(hwdev, buf_in);
	if (err)
		return err;

	cmdqs = ((struct hinic3_hwdev *)hwdev)->cmdqs;

	if (!get_card_present_state((struct hinic3_hwdev *)hwdev))
		return -EPERM;

	err = wait_cmdqs_enable(cmdqs);
	if (err) {
		sdk_err(cmdqs->hwdev->dev_hdl, "Cmdq is disable\n");
		return err;
	}

	if (cos_id >= cmdqs->cmdq_num) {
		sdk_err(cmdqs->hwdev->dev_hdl, "Cmdq id is invalid\n");
		return -EINVAL;
	}

	err = cmdq_sync_cmd_detail_resp(&cmdqs->cmdq[cos_id], mod, cmd,
					buf_in, buf_out, out_param,
					timeout, channel);
	if (!(((struct hinic3_hwdev *)hwdev)->chip_present_flag))
		return -ETIMEDOUT;
	else
		return err;
}
EXPORT_SYMBOL(hinic3_cos_id_detail_resp);

int hinic3_cmdq_async(void *hwdev, u8 mod, u8 cmd, struct hinic3_cmd_buf *buf_in, u16 channel)
{
	struct hinic3_cmdqs *cmdqs = NULL;
	int err;

	err = cmdq_params_valid(hwdev, buf_in);
	if (err)
		return err;

	cmdqs = ((struct hinic3_hwdev *)hwdev)->cmdqs;

	if (!get_card_present_state((struct hinic3_hwdev *)hwdev))
		return -EPERM;

	err = wait_cmdqs_enable(cmdqs);
	if (err) {
		sdk_err(cmdqs->hwdev->dev_hdl, "Cmdq is disable\n");
		return err;
	}
	/* LB mode 1 compatible, cmdq 0 also for async, which is sync_no_wait */
	return cmdq_async_cmd(&cmdqs->cmdq[HINIC3_CMDQ_SYNC], mod,
			      cmd, buf_in, channel);
}
EXPORT_SYMBOL(hinic3_cmdq_async);

int hinic3_cmdq_async_cos(void *hwdev, u8 mod, u8 cmd,
			  u8 cos_id, struct hinic3_cmd_buf *buf_in, u16 channel)
{
	struct hinic3_cmdqs *cmdqs = NULL;
	int err;

	err = cmdq_params_valid(hwdev, buf_in);
	if (err)
		return err;

	cmdqs = ((struct hinic3_hwdev *)hwdev)->cmdqs;

	if (!get_card_present_state((struct hinic3_hwdev *)hwdev))
		return -EPERM;

	err = wait_cmdqs_enable(cmdqs);
	if (err) {
		sdk_err(cmdqs->hwdev->dev_hdl, "Cmdq is disable\n");
		return err;
	}

	if (cos_id >= cmdqs->cmdq_num) {
		sdk_err(cmdqs->hwdev->dev_hdl, "Cmdq id is invalid\n");
		return -EINVAL;
	}

	return cmdq_async_cmd(&cmdqs->cmdq[cos_id], mod, cmd, buf_in, channel);
}

static void clear_wqe_complete_bit(struct hinic3_cmdq *cmdq,
				   struct hinic3_cmdq_wqe *wqe, u16 ci)
{
	struct hinic3_ctrl *ctrl = NULL;
	u32 header_info;
	enum data_format df;

	if (cmdq->hwdev->cmdq_mode == HINIC3_NORMAL_CMDQ) {
		header_info = hinic3_hw_cpu32(WQE_HEADER(wqe)->header_info);
		df = CMDQ_WQE_HEADER_GET(header_info, DATA_FMT);
		if (df == DATA_SGE)
			ctrl = &wqe->wqe_lcmd.ctrl;
		else
			ctrl = &wqe->inline_wqe.wqe_scmd.ctrl;

		ctrl->ctrl_info = 0; /* clear HW busy bit */
	} else {
		wqe->enhanced_cmdq_wqe.completion.cs_format = 0; /* clear HW busy bit */
	}

	cmdq->cmd_infos[ci].cmd_type = HINIC3_CMD_TYPE_NONE;

	wmb(); /* verify wqe is clear */

	hinic3_wq_put_wqebbs(&cmdq->wq, cmdq->cmd_infos[ci].wqebb_use_num);
}

static void cmdq_sync_cmd_handler(struct hinic3_cmdq *cmdq,
				  struct hinic3_cmdq_wqe *wqe, u16 ci)
{
	/* cmdq already locked in poll mode */
	if (!cmdq->cmdqs->poll)
		spin_lock(&cmdq->cmdq_lock);

	cmdq_update_cmd_status(cmdq, ci, wqe);

	if (cmdq->cmd_infos[ci].cmpt_code) {
		*cmdq->cmd_infos[ci].cmpt_code = CMDQ_COMPLETE_CMPT_CODE;
		cmdq->cmd_infos[ci].cmpt_code = NULL;
	}

	/* make sure cmpt_code operation before done operation */
	smp_rmb();

	if (cmdq->cmd_infos[ci].done) {
		complete(cmdq->cmd_infos[ci].done);
		cmdq->cmd_infos[ci].done = NULL;
	}

	if (!cmdq->cmdqs->poll)
		spin_unlock(&cmdq->cmdq_lock);

	cmdq_clear_cmd_buf(&cmdq->cmd_infos[ci], cmdq->hwdev);
	clear_wqe_complete_bit(cmdq, wqe, ci);
}

static void cmdq_async_cmd_handler(struct hinic3_hwdev *hwdev,
				   struct hinic3_cmdq *cmdq,
				   struct hinic3_cmdq_wqe *wqe, u16 ci)
{
	cmdq_clear_cmd_buf(&cmdq->cmd_infos[ci], hwdev);
	clear_wqe_complete_bit(cmdq, wqe, ci);
}

#define HINIC3_CMDQ_WQE_HEAD_LEN		32
static void hinic3_dump_cmdq_wqe_head(struct hinic3_hwdev *hwdev,
				      struct hinic3_cmdq_wqe *wqe)
{
	u32 i;
	u32 *data = (u32 *)wqe;

	for (i = 0; i < (HINIC3_CMDQ_WQE_HEAD_LEN / sizeof(u32)); i += 0x4) {
		sdk_info(hwdev->dev_hdl, "wqe data: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			 *(data + i), *(data + i + 0x1), *(data + i + 0x2),
			 *(data + i + 0x3));
	}
}

static int cmdq_type_default_ceq_handler(struct hinic3_hwdev *hwdev,
							struct hinic3_cmdq_cmd_info *cmd_info,
							struct hinic3_cmdq *cmdq,
							struct hinic3_cmdq_wqe *wqe, u16 ci)
{
	struct hinic3_cmdq_wqe_lcmd *wqe_lcmd = NULL;
	struct hinic3_ctrl *ctrl = NULL;
	u32 ctrl_info;

	if (hwdev->cmdq_mode == HINIC3_NORMAL_CMDQ) {
		/* only arm bit is using scmd wqe, the wqe is lcmd */
		wqe_lcmd = &wqe->wqe_lcmd;
		ctrl = &wqe_lcmd->ctrl;
		ctrl_info = hinic3_hw_cpu32((ctrl)->ctrl_info);

		if (WQE_COMPLETED(ctrl_info) == 0)
			return -EBUSY;
	} else {
		ctrl_info = wqe->enhanced_cmdq_wqe.completion.cs_format;
		ctrl_info = hinic3_hw_cpu32(ctrl_info);
		if (ENHANCE_CMDQ_WQE_CS_GET(ctrl_info, HW_BUSY) == 0)
			return -EBUSY;
	}
	dma_rmb();
	/* For FORCE_STOP cmd_type, we also need to wait for
	 * the firmware processing to complete to prevent the
	 * firmware from accessing the released cmd_buf
	 */
	if (cmd_info->cmd_type == HINIC3_CMD_TYPE_FORCE_STOP) {
		cmdq_clear_cmd_buf(cmd_info, hwdev);
		clear_wqe_complete_bit(cmdq, wqe, ci);
	} else if (cmd_info->cmd_type == HINIC3_CMD_TYPE_ASYNC) {
		cmdq_async_cmd_handler(hwdev, cmdq, wqe, ci);
	} else {
		cmdq_sync_cmd_handler(cmdq, wqe, ci);
	}

	return 0;
}

void hinic3_cmdq_ceq_handler(void *handle, u32 ceqe_data)
{
	struct hinic3_cmdqs *cmdqs = ((struct hinic3_hwdev *)handle)->cmdqs;
	enum hinic3_cmdq_type cmdq_type = CEQE_CMDQ_GET(ceqe_data, TYPE);
	struct hinic3_cmdq *cmdq = NULL;
	struct hinic3_hwdev *hwdev = cmdqs->hwdev;
	struct hinic3_cmdq_wqe *wqe = NULL;
	struct hinic3_cmdq_cmd_info *cmd_info = NULL;
	u16 ci;
	int err;

	if (cmdq_type >= HINIC3_MAX_CMDQ_TYPES) {
		sdk_err(hwdev->dev_hdl, "Cmdq type invalid, type: %u\n", cmdq_type);
		return;
	}
	cmdq = &cmdqs->cmdq[cmdq_type];

	while ((wqe = cmdq_read_wqe(&cmdq->wq, &ci)) != NULL) {
		cmd_info = &cmdq->cmd_infos[ci];
		switch (cmd_info->cmd_type) {
		case HINIC3_CMD_TYPE_NONE:
			return;
		case HINIC3_CMD_TYPE_TIMEOUT:
			sdk_warn(hwdev->dev_hdl, "Cmdq timeout, q_id: %u, ci: %u\n", cmdq_type, ci);
			hinic3_dump_cmdq_wqe_head(hwdev, wqe);
			cmdq_clear_cmd_buf(cmd_info, hwdev);
			clear_wqe_complete_bit(cmdq, wqe, ci);
			break;
		case HINIC3_CMD_TYPE_FAKE_TIMEOUT:
			cmdq_clear_cmd_buf(cmd_info, hwdev);
			clear_wqe_complete_bit(cmdq, wqe, ci);
			break;
		default:
			err = cmdq_type_default_ceq_handler(hwdev, cmd_info, cmdq, wqe, ci);
			if (err != 0)
				return;
			break;
		}
	}
}

static void cmdq_init_queue_ctxt(struct hinic3_cmdqs *cmdqs,
				 struct hinic3_cmdq *cmdq,
				 struct cmdq_ctxt_info *ctxt_info)
{
	struct hinic3_wq *wq = &cmdq->wq;
	u64 cmdq_first_block_paddr, pfn;
	u16 start_ci = (u16)wq->cons_idx;

	pfn = CMDQ_PFN(hinic3_wq_get_first_wqe_page_addr(wq));

	ctxt_info->curr_wqe_page_pfn =
		CMDQ_CTXT_PAGE_INFO_SET(1, HW_BUSY_BIT) |
		CMDQ_CTXT_PAGE_INFO_SET(1, CEQ_EN)	|
		CMDQ_CTXT_PAGE_INFO_SET(1, CEQ_ARM)	|
		CMDQ_CTXT_PAGE_INFO_SET(HINIC3_CEQ_ID_CMDQ, EQ_ID) |
		CMDQ_CTXT_PAGE_INFO_SET(pfn, CURR_WQE_PAGE_PFN);

	if (!WQ_IS_0_LEVEL_CLA(wq)) {
		cmdq_first_block_paddr = cmdqs->wq_block_paddr;
		pfn = CMDQ_PFN(cmdq_first_block_paddr);
	}

	ctxt_info->wq_block_pfn = CMDQ_CTXT_BLOCK_INFO_SET(start_ci, CI) |
				CMDQ_CTXT_BLOCK_INFO_SET(pfn, WQ_BLOCK_PFN);
}

static int init_cmdq(struct hinic3_cmdq *cmdq, struct hinic3_hwdev *hwdev,
		     enum hinic3_cmdq_type q_type)
{
	int err;

	cmdq->cmdq_type = q_type;
	cmdq->wrapped = 1;
	cmdq->hwdev = hwdev;
	cmdq->cmdqs = hwdev->cmdqs;

	spin_lock_init(&cmdq->cmdq_lock);

	cmdq->cmd_infos = kcalloc(cmdq->wq.q_depth, sizeof(*cmdq->cmd_infos),
				  GFP_KERNEL);
	if (!cmdq->cmd_infos) {
		err = -ENOMEM;
		goto cmd_infos_err;
	}

	return 0;

cmd_infos_err:
	spin_lock_deinit(&cmdq->cmdq_lock);

	return err;
}

static void free_cmdq(struct hinic3_cmdq *cmdq)
{
	kfree(cmdq->cmd_infos);
	cmdq->cmd_infos = NULL;
	spin_lock_deinit(&cmdq->cmdq_lock);
}

static int hinic3_set_cmdq_ctxts(struct hinic3_hwdev *hwdev)
{
	struct hinic3_cmdqs *cmdqs = hwdev->cmdqs;
	struct enhance_cmdq_ctxt_info *ctxt = NULL;
	u8 cmdq_type;
	int err;

	cmdq_type = HINIC3_CMDQ_SYNC;
	for (; cmdq_type < cmdqs->cmdq_num; cmdq_type++) {
		if (cmdqs->cmdq_mode == HINIC3_NORMAL_CMDQ) {
			err = hinic3_set_cmdq_ctxt(hwdev, (u8)cmdq_type,
						   &cmdqs->cmdq[cmdq_type].cmdq_ctxt);
		} else {
			ctxt = &cmdqs->cmdq[cmdq_type].cmdq_enhance_ctxt;
			err = hinic3_set_enhance_cmdq_ctxt(hwdev, (u8)cmdq_type, ctxt);
		}
		if (err != 0)
			return err;
	}

	cmdqs->status |= HINIC3_CMDQ_ENABLE;
	cmdqs->disable_flag = 0;

	return 0;
}

static void cmdq_flush_sync_cmd(struct hinic3_cmdq_cmd_info *cmd_info)
{
	if (cmd_info->cmd_type != HINIC3_CMD_TYPE_DIRECT_RESP &&
	    cmd_info->cmd_type != HINIC3_CMD_TYPE_SGE_RESP)
		return;

	cmd_info->cmd_type = HINIC3_CMD_TYPE_FORCE_STOP;

	if (cmd_info->cmpt_code &&
	    *cmd_info->cmpt_code == CMDQ_SEND_CMPT_CODE)
		*cmd_info->cmpt_code = CMDQ_FORCE_STOP_CMPT_CODE;

	if (cmd_info->done) {
		complete(cmd_info->done);
		cmd_info->done = NULL;
		cmd_info->cmpt_code = NULL;
		cmd_info->direct_resp = NULL;
		cmd_info->errcode = NULL;
	}
}

void hinic3_cmdq_flush_cmd(struct hinic3_hwdev *hwdev,
			   struct hinic3_cmdq *cmdq)
{
	struct hinic3_cmdq_cmd_info *cmd_info = NULL;
	u16 ci = 0;

	spin_lock_bh(&cmdq->cmdq_lock);

	while (cmdq_read_wqe(&cmdq->wq, &ci)) {
		cmd_info = &cmdq->cmd_infos[ci];
		hinic3_wq_put_wqebbs(&cmdq->wq, cmd_info->wqebb_use_num);

		if (cmd_info->cmd_type == HINIC3_CMD_TYPE_DIRECT_RESP ||
		    cmd_info->cmd_type == HINIC3_CMD_TYPE_SGE_RESP)
			cmdq_flush_sync_cmd(cmd_info);
	}

	spin_unlock_bh(&cmdq->cmdq_lock);
}

static void hinic3_cmdq_flush_channel_sync_cmd(struct hinic3_hwdev *hwdev, u16 channel)
{
	struct hinic3_cmdq_cmd_info *cmd_info = NULL;
	struct hinic3_cmdq *cmdq = NULL;
	struct hinic3_wq *wq = NULL;
	u16 wqe_cnt, ci, i;

	if (channel >= HINIC3_CHANNEL_MAX)
		return;

	cmdq = &hwdev->cmdqs->cmdq[HINIC3_CMDQ_SYNC];

	spin_lock_bh(&cmdq->cmdq_lock);

	wq = &cmdq->wq;
	ci = wq->cons_idx;
	wqe_cnt = (u16)WQ_MASK_IDX(wq, wq->prod_idx +
				   wq->q_depth - wq->cons_idx);
	for (i = 0; i < wqe_cnt; i++) {
		cmd_info = &cmdq->cmd_infos[WQ_MASK_IDX(wq, ci + i)];
		if (cmd_info->channel == channel)
			cmdq_flush_sync_cmd(cmd_info);
	}

	spin_unlock_bh(&cmdq->cmdq_lock);
}

void hinic3_cmdq_flush_sync_cmd(struct hinic3_hwdev *hwdev)
{
	struct hinic3_cmdq_cmd_info *cmd_info = NULL;
	struct hinic3_cmdq *cmdq = NULL;
	struct hinic3_wq *wq = NULL;
	u16 wqe_cnt, ci, i;

	cmdq = &hwdev->cmdqs->cmdq[HINIC3_CMDQ_SYNC];

	spin_lock_bh(&cmdq->cmdq_lock);

	wq = &cmdq->wq;
	ci = wq->cons_idx;
	wqe_cnt = (u16)WQ_MASK_IDX(wq, wq->prod_idx +
				   wq->q_depth - wq->cons_idx);
	for (i = 0; i < wqe_cnt; i++) {
		cmd_info = &cmdq->cmd_infos[WQ_MASK_IDX(wq, ci + i)];
		cmdq_flush_sync_cmd(cmd_info);
	}

	spin_unlock_bh(&cmdq->cmdq_lock);
}

static void cmdq_reset_all_cmd_buff(struct hinic3_cmdq *cmdq)
{
	u16 i;

	for (i = 0; i < cmdq->wq.q_depth; i++)
		cmdq_clear_cmd_buf(&cmdq->cmd_infos[i], cmdq->hwdev);
}

int hinic3_cmdq_set_channel_status(struct hinic3_hwdev *hwdev, u16 channel,
				   bool enable)
{
	if (channel >= HINIC3_CHANNEL_MAX)
		return -EINVAL;

	if (enable) {
		clear_bit(channel, &hwdev->cmdqs->channel_stop);
	} else {
		set_bit(channel, &hwdev->cmdqs->channel_stop);
		hinic3_cmdq_flush_channel_sync_cmd(hwdev, channel);
	}

	sdk_info(hwdev->dev_hdl, "%s cmdq channel 0x%x\n",
		 enable ? "Enable" : "Disable", channel);

	return 0;
}

void hinic3_cmdq_enable_channel_lock(struct hinic3_hwdev *hwdev, bool enable)
{
	hwdev->cmdqs->lock_channel_en = enable;

	sdk_info(hwdev->dev_hdl, "%s cmdq channel lock\n",
		 enable ? "Enable" : "Disable");
}

int hinic3_reinit_cmdq_ctxts(struct hinic3_hwdev *hwdev)
{
	struct hinic3_cmdqs *cmdqs = hwdev->cmdqs;
	u8 cmdq_type;

	cmdq_type = HINIC3_CMDQ_SYNC;
	for (; cmdq_type < cmdqs->cmdq_num; cmdq_type++) {
		hinic3_cmdq_flush_cmd(hwdev, &cmdqs->cmdq[cmdq_type]);
		cmdq_reset_all_cmd_buff(&cmdqs->cmdq[cmdq_type]);
		cmdqs->cmdq[cmdq_type].wrapped = 1;
		hinic3_wq_reset(&cmdqs->cmdq[cmdq_type].wq);
	}

	return hinic3_set_cmdq_ctxts(hwdev);
}

static int create_cmdq_wq(struct hinic3_cmdqs *cmdqs)
{
	u8 type, cmdq_type;
	int err = 0;

	cmdq_type = HINIC3_CMDQ_SYNC;
	for (; cmdq_type < cmdqs->cmdq_num; cmdq_type++) {
		err = hinic3_wq_create(cmdqs->hwdev, &cmdqs->cmdq[cmdq_type].wq,
				       HINIC3_CMDQ_DEPTH, cmdqs->wqebb_size);
		if (err != 0) {
			sdk_err(cmdqs->hwdev->dev_hdl, "Failed to create cmdq wq\n");
			goto destroy_wq;
		}
	}

	/* 1-level CLA must put all cmdq's wq page addr in one wq block */
	if (!WQ_IS_0_LEVEL_CLA(&cmdqs->cmdq[HINIC3_CMDQ_SYNC].wq)) {
		/* cmdq wq's CLA table is up to 512B */
#define CMDQ_WQ_CLA_SIZE	512
		if (cmdqs->cmdq[HINIC3_CMDQ_SYNC].wq.num_wq_pages >
		    CMDQ_WQ_CLA_SIZE / sizeof(u64)) {
			err = -EINVAL;
			sdk_err(cmdqs->hwdev->dev_hdl, "Cmdq wq page exceed limit: %lu\n",
				CMDQ_WQ_CLA_SIZE / sizeof(u64));
			goto destroy_wq;
		}

		cmdqs->wq_block_vaddr =
			dma_zalloc_coherent(cmdqs->hwdev->dev_hdl, PAGE_SIZE,
					    &cmdqs->wq_block_paddr, GFP_KERNEL);
		if (!cmdqs->wq_block_vaddr) {
			err = -ENOMEM;
			sdk_err(cmdqs->hwdev->dev_hdl, "Failed to alloc cmdq wq block\n");
			goto destroy_wq;
		}

		type = HINIC3_CMDQ_SYNC;
		for (; type < cmdqs->cmdq_num; type++)
			memcpy((u8 *)cmdqs->wq_block_vaddr +
			       ((u64)type * CMDQ_WQ_CLA_SIZE),
			       cmdqs->cmdq[type].wq.wq_block_vaddr,
			       cmdqs->cmdq[type].wq.num_wq_pages * sizeof(u64));
	}

	return 0;

destroy_wq:
	type = HINIC3_CMDQ_SYNC;
	for (; type < cmdq_type; type++)
		hinic3_wq_destroy(&cmdqs->cmdq[type].wq);

	return err;
}

static void destroy_cmdq_wq(struct hinic3_cmdqs *cmdqs)
{
	u8 cmdq_type;

	if (cmdqs->wq_block_vaddr)
		dma_free_coherent(cmdqs->hwdev->dev_hdl, PAGE_SIZE,
				  cmdqs->wq_block_vaddr, cmdqs->wq_block_paddr);

	cmdq_type = HINIC3_CMDQ_SYNC;
	for (; cmdq_type < cmdqs->cmdq_num; cmdq_type++)
		hinic3_wq_destroy(&cmdqs->cmdq[cmdq_type].wq);
}

static int init_cmdqs(struct hinic3_hwdev *hwdev)
{
	struct hinic3_cmdqs *cmdqs = NULL;

	cmdqs = kzalloc(sizeof(*cmdqs), GFP_KERNEL);
	if (!cmdqs)
		return -ENOMEM;

	hwdev->cmdqs = cmdqs;
	cmdqs->hwdev = hwdev;
	if (HINIC3_HWIF_NUM_CEQS(hwdev->hwif) == 0 || hwdev->poll != 0)
		cmdqs->poll = true;

	if (COMM_SUPPORT_ONLY_ENHANCE_CMDQ(hwdev) != 0)
		cmdqs->cmdq_mode = HINIC3_ENHANCE_CMDQ;
	else
		cmdqs->cmdq_mode = HINIC3_NORMAL_CMDQ;

	hwdev->cmdq_mode = cmdqs->cmdq_mode;

	if (cmdqs->cmdq_mode == HINIC3_NORMAL_CMDQ) {
		cmdqs->wqebb_size = CMDQ_WQEBB_SIZE;
		cmdqs->wqebb_use_num = NUM_WQEBBS_FOR_CMDQ_WQE;
	} else {
		cmdqs->wqebb_size = ENHANCE_CMDQ_WQEBB_SIZE;
		cmdqs->wqebb_use_num = NUM_WQEBBS_FOR_ENHANCE_CMDQ_WQE;
	}

	cmdqs->cmdq_num = HINIC3_MAX_CMDQ_TYPES;
	if (COMM_SUPPORT_CMDQ_NUM(hwdev) != 0) {
		if (hwdev->glb_attr.cmdq_num <= HINIC3_MAX_CMDQ_TYPES)
			cmdqs->cmdq_num = hwdev->glb_attr.cmdq_num;
		else
			sdk_warn(hwdev->dev_hdl, "Adjust cmdq num to %d\n", HINIC3_MAX_CMDQ_TYPES);
	}

	cmdqs->cmd_buf_size = HINIC3_CMDQ_MAX_BUF_SIZE;
	if (COMM_SUPPORT_CMD_BUF_SIZE(hwdev) != 0) {
		if (hwdev->glb_attr.cmd_buf_size <= HINIC3_CMDQ_MAX_BUF_SIZE)
			cmdqs->cmd_buf_size = hwdev->glb_attr.cmd_buf_size;
		else
			sdk_warn(hwdev->dev_hdl, "Adjust cmd buf size to %d\n",
					HINIC3_MAX_CMDQ_TYPES);
	}

	cmdqs->cmd_buf_pool = dma_pool_create("hinic3_cmdq", hwdev->dev_hdl, cmdqs->cmd_buf_size,
					      HINIC3_CMDQ_BUF_ALIGN, 0ULL);
	if (!cmdqs->cmd_buf_pool) {
		sdk_err(hwdev->dev_hdl, "Failed to create cmdq buffer pool\n");
		kfree(cmdqs);
		return -ENOMEM;
	}

	return 0;
}

int hinic3_cmdqs_init(struct hinic3_hwdev *hwdev)
{
	struct hinic3_cmdqs *cmdqs = NULL;
	void __iomem *db_base = NULL;
	u8 type, cmdq_type;
	int err = -ENOMEM;

	err = init_cmdqs(hwdev);
	if (err != 0)
		return err;

	cmdqs = hwdev->cmdqs;

	err = create_cmdq_wq(cmdqs);
	if (err != 0)
		goto create_wq_err;

	err = hinic3_alloc_db_addr(hwdev, &db_base, NULL);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to allocate doorbell address\n");
		goto alloc_db_err;
	}

	cmdqs->cmdqs_db_base = (u8 *)db_base;
	for (cmdq_type = HINIC3_CMDQ_SYNC; cmdq_type < cmdqs->cmdq_num; cmdq_type++) {
		err = init_cmdq(&cmdqs->cmdq[cmdq_type], hwdev, cmdq_type);
		if (err != 0) {
			sdk_err(hwdev->dev_hdl, "Failed to initialize cmdq type :%u\n", cmdq_type);
			goto init_cmdq_err;
		}

		if (cmdqs->cmdq_mode == HINIC3_NORMAL_CMDQ)
			cmdq_init_queue_ctxt(cmdqs, &cmdqs->cmdq[cmdq_type],
					     &cmdqs->cmdq[cmdq_type].cmdq_ctxt);
		else /* HINIC3_ENHANCE_CMDQ */
			enhanced_cmdq_init_queue_ctxt(cmdqs, &cmdqs->cmdq[cmdq_type]);
	}

	err = hinic3_set_cmdq_ctxts(hwdev);
	if (err != 0)
		goto init_cmdq_err;

	return 0;

init_cmdq_err:
	for (type = HINIC3_CMDQ_SYNC; type < cmdq_type; type++)
		free_cmdq(&cmdqs->cmdq[type]);

	hinic3_free_db_addr(hwdev, cmdqs->cmdqs_db_base, NULL);

alloc_db_err:
	destroy_cmdq_wq(cmdqs);

create_wq_err:
	dma_pool_destroy(cmdqs->cmd_buf_pool);
	kfree(cmdqs);

	return err;
}

void hinic3_cmdqs_free(struct hinic3_hwdev *hwdev)
{
	struct hinic3_cmdqs *cmdqs = hwdev->cmdqs;
	u8 cmdq_type = HINIC3_CMDQ_SYNC;

	cmdqs->status &= ~HINIC3_CMDQ_ENABLE;

	for (; cmdq_type < cmdqs->cmdq_num; cmdq_type++) {
		hinic3_cmdq_flush_cmd(hwdev, &cmdqs->cmdq[cmdq_type]);
		cmdq_reset_all_cmd_buff(&cmdqs->cmdq[cmdq_type]);
		free_cmdq(&cmdqs->cmdq[cmdq_type]);
	}

	hinic3_free_db_addr(hwdev, cmdqs->cmdqs_db_base, NULL);
	destroy_cmdq_wq(cmdqs);

	dma_pool_destroy(cmdqs->cmd_buf_pool);

	kfree(cmdqs);
}

