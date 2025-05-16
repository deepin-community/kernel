// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#include "nic_mpu_cmd.h"
#include "nic_npu_cmd.h"
#include "hinic3_nic_cmdq.h"
#include "sw_cmdq_ops.h"

static void hinic3_qp_prepare_cmdq_header(struct hinic3_qp_ctxt_header *qp_ctxt_hdr,
					  enum hinic3_qp_ctxt_type ctxt_type, u16 num_queues,
					  u16 q_id)
{
	qp_ctxt_hdr->queue_type = ctxt_type;
	qp_ctxt_hdr->num_queues = num_queues;
	qp_ctxt_hdr->start_qid = q_id;
	qp_ctxt_hdr->rsvd = 0;

	hinic3_cpu_to_be32(qp_ctxt_hdr, sizeof(*qp_ctxt_hdr));
}

static u8 prepare_cmd_buf_qp_context_multi_store(struct hinic3_nic_io *nic_io,
						 struct hinic3_cmd_buf *cmd_buf,
						 enum hinic3_qp_ctxt_type ctxt_type,
	u16 start_qid, u16 max_ctxts)
{
	struct hinic3_qp_ctxt_block *qp_ctxt_block = NULL;
	u16 i;

	qp_ctxt_block = cmd_buf->buf;

	hinic3_qp_prepare_cmdq_header(&qp_ctxt_block->cmdq_hdr, ctxt_type,
				      max_ctxts, start_qid);

	for (i = 0; i < max_ctxts; i++) {
		if (ctxt_type == HINIC3_QP_CTXT_TYPE_RQ)
			hinic3_rq_prepare_ctxt(&nic_io->rq[start_qid + i],
					       &qp_ctxt_block->rq_ctxt[i]);
		else
			hinic3_sq_prepare_ctxt(&nic_io->sq[start_qid + i], start_qid + i,
					       &qp_ctxt_block->sq_ctxt[i]);
	}

	return (u8)HINIC3_UCODE_CMD_MODIFY_QUEUE_CTX;
}

static u8 prepare_cmd_buf_clean_tso_lro_space(struct hinic3_nic_io *nic_io,
					      struct hinic3_cmd_buf *cmd_buf,
					      enum hinic3_qp_ctxt_type ctxt_type)
{
	struct hinic3_clean_queue_ctxt *ctxt_block = NULL;

	ctxt_block = cmd_buf->buf;
	ctxt_block->cmdq_hdr.num_queues = nic_io->max_qps;
	ctxt_block->cmdq_hdr.queue_type = ctxt_type;
	ctxt_block->cmdq_hdr.start_qid = 0;

	hinic3_cpu_to_be32(ctxt_block, sizeof(*ctxt_block));

	cmd_buf->size = sizeof(*ctxt_block);
	return (u8)HINIC3_UCODE_CMD_CLEAN_QUEUE_CONTEXT;
}

static u8 prepare_cmd_buf_set_rss_indir_table(const struct hinic3_nic_io *nic_io,
					      const u32 *indir_table,
					      struct hinic3_cmd_buf *cmd_buf)
{
	u32 i, size;
	u32 *temp = NULL;
	struct nic_rss_indirect_tbl *indir_tbl = NULL;

	indir_tbl = (struct nic_rss_indirect_tbl *)cmd_buf->buf;
	cmd_buf->size = sizeof(struct nic_rss_indirect_tbl);
	memset(indir_tbl, 0, sizeof(*indir_tbl));

	for (i = 0; i < NIC_RSS_INDIR_SIZE; i++)
		indir_tbl->entry[i] = (u16)(*(indir_table + i));

	size = sizeof(indir_tbl->entry) / (sizeof(u32));
	temp = (u32 *)indir_tbl->entry;
	for (i = 0; i < size; i++)
		temp[i] = cpu_to_be32(temp[i]);

	return (u8)HINIC3_UCODE_CMD_SET_RSS_INDIR_TABLE;
}

static u8 prepare_cmd_buf_get_rss_indir_table(const struct hinic3_nic_io *nic_io,
					      const struct hinic3_cmd_buf *cmd_buf)
{
	(void)nic_io;
	memset(cmd_buf->buf, 0, cmd_buf->size);

	return (u8)HINIC3_UCODE_CMD_GET_RSS_INDIR_TABLE;
}

static void cmd_buf_to_rss_indir_table(const struct hinic3_cmd_buf *cmd_buf, u32 *indir_table)
{
	u32 i;
	u16 *indir_tbl = NULL;

	indir_tbl = (u16 *)cmd_buf->buf;
	for (i = 0; i < NIC_RSS_INDIR_SIZE; i++)
		indir_table[i] = *(indir_tbl + i);
}

static u8 prepare_cmd_buf_modify_svlan(struct hinic3_cmd_buf *cmd_buf,
				       u16 func_id, u16 vlan_tag, u16 q_id, u8 vlan_mode)
{
	struct nic_vlan_ctx *vlan_ctx = NULL;

	cmd_buf->size = sizeof(struct nic_vlan_ctx);
	vlan_ctx = (struct nic_vlan_ctx *)cmd_buf->buf;

	vlan_ctx->func_id = func_id;
	vlan_ctx->qid = q_id;
	vlan_ctx->vlan_tag = vlan_tag;
	vlan_ctx->vlan_sel = 0; /* TPID0 in IPSU */
	vlan_ctx->vlan_mode = vlan_mode;

	hinic3_cpu_to_be32(vlan_ctx, sizeof(struct nic_vlan_ctx));
	return (u8)HINIC3_UCODE_CMD_MODIFY_VLAN_CTX;
}

struct hinic3_nic_cmdq_ops *hinic3_nic_cmdq_get_sw_ops(void)
{
	static struct hinic3_nic_cmdq_ops cmdq_sw_ops = {
		.prepare_cmd_buf_clean_tso_lro_space = prepare_cmd_buf_clean_tso_lro_space,
		.prepare_cmd_buf_qp_context_multi_store = prepare_cmd_buf_qp_context_multi_store,
		.prepare_cmd_buf_modify_svlan = prepare_cmd_buf_modify_svlan,
		.prepare_cmd_buf_set_rss_indir_table = prepare_cmd_buf_set_rss_indir_table,
		.prepare_cmd_buf_get_rss_indir_table = prepare_cmd_buf_get_rss_indir_table,
		.cmd_buf_to_rss_indir_table = cmd_buf_to_rss_indir_table,
	};

	return &cmdq_sw_ops;
}
