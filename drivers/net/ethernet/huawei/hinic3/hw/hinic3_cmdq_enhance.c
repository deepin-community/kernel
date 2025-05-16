// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [COMM]" fmt

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/module.h>

#include "ossl_knl.h"
#include "npu_cmdq_base_defs.h"
#include "hinic3_crm.h"
#include "hinic3_hw.h"
#include "hinic3_hwdev.h"
#include "hinic3_eqs.h"
#include "hinic3_common.h"
#include "hinic3_wq.h"
#include "hinic3_hw_comm.h"
#include "hinic3_cmdq.h"

void enhanced_cmdq_init_queue_ctxt(struct hinic3_cmdqs *cmdqs, struct hinic3_cmdq *cmdq)
{
	struct enhance_cmdq_ctxt_info *ctxt_info = &cmdq->cmdq_enhance_ctxt;
	struct hinic3_wq *wq = &cmdq->wq;
	u64 cmdq_first_block_paddr, pfn;
	u16 start_ci = (u16)wq->cons_idx;
	u32 start_pi = (u16)wq->prod_idx;

	pfn = CMDQ_PFN(hinic3_wq_get_first_wqe_page_addr(wq));

	/* first part 16B */
	if (cmdq->cmdqs->poll) {
		ctxt_info->eq_cfg =
			ENHANCED_CMDQ_SET(pfn, CTXT0_CI_WQE_ADDR) |
			ENHANCED_CMDQ_SET(HINIC3_CEQ_ID_CMDQ, CTXT0_EQ) |
			ENHANCED_CMDQ_SET(0, CTXT0_CEQ_ARM) |
			ENHANCED_CMDQ_SET(0, CTXT0_CEQ_EN) |
			ENHANCED_CMDQ_SET(1, CTXT0_HW_BUSY_BIT);
	} else {
		ctxt_info->eq_cfg =
			ENHANCED_CMDQ_SET(pfn, CTXT0_CI_WQE_ADDR) |
			ENHANCED_CMDQ_SET(HINIC3_CEQ_ID_CMDQ, CTXT0_EQ) |
			ENHANCED_CMDQ_SET(1, CTXT0_CEQ_ARM) |
			ENHANCED_CMDQ_SET(1, CTXT0_CEQ_EN) |
			ENHANCED_CMDQ_SET(1, CTXT0_HW_BUSY_BIT);
	}

	ctxt_info->dfx_pi_ci =
		ENHANCED_CMDQ_SET(0, CTXT1_Q_DIS) |
		ENHANCED_CMDQ_SET(0, CTXT1_ERR_CODE) |
		ENHANCED_CMDQ_SET(start_pi, CTXT1_PI) |
		ENHANCED_CMDQ_SET(start_ci, CTXT1_CI);

	/* second part 16B */
	ctxt_info->pft_thd =
		ENHANCED_CMDQ_SET(CI_HIGN_IDX(start_ci), CTXT2_PFT_CI) |
		ENHANCED_CMDQ_SET(1, CTXT2_O_BIT) |
		ENHANCED_CMDQ_SET(WQ_PREFETCH_MIN, CTXT2_PFT_MIN) |
		ENHANCED_CMDQ_SET(WQ_PREFETCH_MAX, CTXT2_PFT_MAX) |
		ENHANCED_CMDQ_SET(WQ_PREFETCH_THRESHOLD, CTXT2_PFT_THD);
	ctxt_info->pft_ci =
		ENHANCED_CMDQ_SET(pfn, CTXT3_PFT_CI_ADDR) |
		ENHANCED_CMDQ_SET(start_ci, CTXT3_PFT_CI);

	/* third part 16B */
	cmdq_first_block_paddr = cmdqs->wq_block_paddr;
	pfn = WQ_BLOCK_PFN(cmdq_first_block_paddr);

	ctxt_info->ci_cla_addr = ENHANCED_CMDQ_SET(pfn, CTXT4_CI_CLA_ADDR);
}

static void enhance_cmdq_set_completion(struct hinic3_cmdq_enhance_completion *completion,
					const struct hinic3_cmd_buf *buf_out)
{
	completion->sge_resp_hi_addr = upper_32_bits(buf_out->dma_addr);
	completion->sge_resp_lo_addr = lower_32_bits(buf_out->dma_addr);
	completion->sge_resp_len = buf_out->size;
}

static void cmdq_set_wqe_buf_desc(
	struct hinic3_enhanced_cmdq_wqe *enhanced_wqe,
	const struct hinic3_cmdq_cmd_param *cmd_buf, u32 len)
{
	enhanced_wqe->buf_desc[0].sge_send_hi_addr = upper_32_bits(cmd_buf->buf_in->dma_addr + len);
	enhanced_wqe->buf_desc[0].sge_send_lo_addr = lower_32_bits(cmd_buf->buf_in->dma_addr + len);
	enhanced_wqe->buf_desc[0].len = len;

	enhanced_wqe->buf_desc[1].sge_send_hi_addr =
		upper_32_bits(cmd_buf->buf_in->dma_addr + (len << 1));
	enhanced_wqe->buf_desc[1].sge_send_lo_addr =
		lower_32_bits(cmd_buf->buf_in->dma_addr + (len << 1));
	enhanced_wqe->buf_desc[1].len = cmd_buf->buf_in->size - (len << 1);
}

void enhanced_cmdq_set_wqe(struct hinic3_cmdq_wqe *wqe, enum hinic3_cmdq_cmd_type cmd_type,
			const struct hinic3_cmdq_cmd_param *cmd_buf, int wrapped)
{
	struct hinic3_enhanced_cmdq_wqe *enhanced_wqe = NULL;
	u32 len = 0;

	if (!wqe || !cmd_buf || !cmd_buf->buf_in) {
		pr_err("wqe or buf_in is null\n");
		return;
	}

	enhanced_wqe = &wqe->enhanced_cmdq_wqe;
	len = cmd_buf->buf_in->size / 3;  /* Wqe should be 64B aligned, so we fill 3 sges  */

	enhanced_wqe->ctrl_sec.header = ENHANCE_CMDQ_WQE_HEADER_SET(len, SEND_SGE_LEN) |
		ENHANCE_CMDQ_WQE_HEADER_SET(BUFDESC_ENHANCE_CMD_LEN, BDSL) |
		ENHANCE_CMDQ_WQE_HEADER_SET(DATA_SGE, DF) |
		ENHANCE_CMDQ_WQE_HEADER_SET(NORMAL_WQE_TYPE, DN) |
		ENHANCE_CMDQ_WQE_HEADER_SET(COMPACT_WQE_TYPE, EC) |
		ENHANCE_CMDQ_WQE_HEADER_SET((u32)wrapped, HW_BUSY_BIT);

	enhanced_wqe->ctrl_sec.sge_send_hi_addr = upper_32_bits(cmd_buf->buf_in->dma_addr);
	enhanced_wqe->ctrl_sec.sge_send_lo_addr = lower_32_bits(cmd_buf->buf_in->dma_addr);

	cmdq_set_wqe_buf_desc(enhanced_wqe, cmd_buf, len);

	enhanced_wqe->completion.cs_format = ENHANCE_CMDQ_WQE_CS_SET(cmd_buf->cmd, CMD) |
		ENHANCE_CMDQ_WQE_CS_SET(HINIC3_ACK_TYPE_CMDQ, ACK_TYPE) |
		ENHANCE_CMDQ_WQE_CS_SET((cmd_buf->mod == HINIC3_MOD_ROCE), RN) |
		ENHANCE_CMDQ_WQE_CS_SET(cmd_buf->mod, MOD);

	switch (cmd_type) {
	case HINIC3_CMD_TYPE_DIRECT_RESP:
		enhanced_wqe->completion.cs_format |= ENHANCE_CMDQ_WQE_CS_SET(INLINE_DATA, CF);
		break;
	case HINIC3_CMD_TYPE_SGE_RESP:
		if (cmd_buf->buf_out) {
			enhanced_wqe->completion.cs_format |=
				ENHANCE_CMDQ_WQE_CS_SET(SGE_RESPONSE, CF);
			enhance_cmdq_set_completion(&enhanced_wqe->completion, cmd_buf->buf_out);
		}
		break;
	case HINIC3_CMD_TYPE_ASYNC:
		break;
	default:
		break;
	}
}

