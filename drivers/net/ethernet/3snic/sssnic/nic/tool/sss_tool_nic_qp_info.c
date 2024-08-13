// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [TOOL]" fmt

#include "sss_kernel.h"
#include "sss_nic_tx.h"
#include "sss_tool_comm.h"
#include "sss_tool_nic.h"

static int sss_tool_get_wqe_info(struct sss_nic_dev *nic_dev,
				 u16 q_id, u16 wqe_id, u16 wqebb_cnt,
				 u8 *out_buff, const u16 *out_len,
				 enum sss_nic_queue_type q_type)
{
	u32 i;
	void *src_wqebb = NULL;
	u32 offset;
	struct sss_nic_io_queue *queue = NULL;
	struct sss_nic_io *nic_io = NULL;

	nic_io = nic_dev->nic_io;
	if (!nic_io) {
		tool_err("Fail to get wqe info, nic_io is NULL.\n");
		return -EINVAL;
	}

	if (q_id >= nic_io->max_qp_num) {
		tool_err("Fail to get wqe info, q_id[%u] > num_qps_cfg[%u].\n",
			 q_id, nic_io->max_qp_num);
		return -EINVAL;
	}

	if (q_type == SSSNIC_RQ)
		queue = &nic_io->rq_group[q_id];
	else
		queue = &nic_io->sq_group[q_id];

	if ((wqe_id + wqebb_cnt) > queue->wq.q_depth) {
		tool_err("Fail to get wqe info, (idx[%u] + idx[%u]) > q_depth[%u].\n",
			 wqe_id, wqebb_cnt, queue->wq.q_depth);
		return -EINVAL;
	}

	if (*out_len != (queue->wq.elem_size * wqebb_cnt)) {
		tool_err("Fail to get wqe info, out len :%u is not equal to %d\n",
			 *out_len, (queue->wq.elem_size * wqebb_cnt));
		return -EINVAL;
	}

	for (i = 0; i < wqebb_cnt; i++) {
		src_wqebb = sss_wq_wqebb_addr(&queue->wq,
					      (u16)SSS_WQ_MASK_ID(&queue->wq, wqe_id + i));
		offset = queue->wq.elem_size * i;
		memcpy(out_buff + offset, src_wqebb, queue->wq.elem_size);
	}

	return 0;
}

static void sss_tool_get_sq_info(struct sss_nic_io *nic_io, u16 q_id,
				 struct sss_tool_sq_info *sq_info)
{
	struct sss_nic_io_queue *sq = NULL;

	sq = &nic_io->sq_group[q_id];

	sq_info->q_depth = sq->wq.q_depth;
	sq_info->q_id = q_id;
	sq_info->pi = sss_nic_get_sq_local_pi(sq);
	sq_info->doorbell.map_addr = (u64 *)sq->db_addr;
	sq_info->fi = sss_nic_get_sq_hw_ci(sq);
	sq_info->wqebb_size = sq->wq.elem_size;
	sq_info->ci = sss_nic_get_sq_local_ci(sq);
	sq_info->ci_addr = sq->tx.ci_addr;
	sq_info->slq_handle = sq;
	sq_info->cla_addr = sq->wq.block_paddr;
}

static void sss_tool_get_rq_info(struct sss_nic_io *nic_io, u16 q_id,
				 struct sss_tool_rq_info *rq_info)
{
	struct sss_nic_io_queue *rq = NULL;

	rq = &nic_io->rq_group[q_id];

	rq_info->msix_idx = rq->msix_id;
	rq_info->hw_pi = cpu_to_be16(*rq->rx.pi_vaddr);
	rq_info->buf_len = nic_io->rx_buff_len;
	rq_info->wqebb_size = rq->wq.elem_size;
	rq_info->slq_handle = rq;
	rq_info->q_id = q_id;
	rq_info->ci_cla_tbl_addr = rq->wq.block_paddr;
	rq_info->q_depth = (u16)rq->wq.q_depth;
	rq_info->ci_wqe_page_addr = sss_wq_get_first_wqe_page_addr(&rq->wq);
}

static int sss_tool_get_queue_info(struct sss_nic_dev *nic_dev, u16 q_id,
				   void *out_buff, enum sss_nic_queue_type q_type)
{
	struct sss_nic_io *nic_io = NULL;

	nic_io = nic_dev->nic_io;
	if (!nic_io) {
		tool_err("Fail to get wqe info, nic_io is NULL.\n");
		return -EINVAL;
	}

	if (q_id >= nic_io->max_qp_num) {
		tool_err("Fail to get rq info, input q_id(%u) is larger than max qp num:%u\n",
			 q_id, nic_io->max_qp_num);
		return -EINVAL;
	}

	(q_type == SSSNIC_RQ) ? sss_tool_get_rq_info(nic_io, q_id, out_buff) :
		sss_tool_get_sq_info(nic_io, q_id, out_buff);

	return 0;
}

static bool sss_tool_check_input_pointer(struct sss_nic_dev *nic_dev,
					 const void *in_buf, void *out_buf, u32 *out_len)
{
	if (!SSS_CHANNEL_RES_VALID(nic_dev)) {
		tool_err("Invalid input param nic_dev\n");
		return false;
	}

	if (!in_buf || !out_buf || !out_len) {
		tool_err("Invalid input param,in_buf/out_buf/out_len\n");
		return false;
	}

	return true;
}

int sss_tool_get_tx_info(struct sss_nic_dev *nic_dev, const void *in_buf,
			 u32 in_len, void *out_buf, u32 *out_len)
{
	u16 q_id;
	struct sss_tool_sq_info sq_info = {0};

	if (!sss_tool_check_input_pointer(nic_dev, in_buf, out_buf, out_len))
		return -EINVAL;

	if (in_len != sizeof(in_len)) {
		tool_err("Fail to get tx info, in len :%u is not equal to %lu\n",
			 in_len, sizeof(in_len));
		return -EINVAL;
	}

	if (*out_len != sizeof(sq_info)) {
		tool_err("Fail to get tx info, out len :%u is not equal to %lu\n",
			 *out_len, sizeof(sq_info));
		return -EINVAL;
	}

	q_id = (u16)(*((u32 *)in_buf));

	return sss_tool_get_queue_info(nic_dev, q_id, out_buf, SSSNIC_SQ);
}

int sss_tool_get_tx_wqe_info(struct sss_nic_dev *nic_dev, const void *in_buf,
			     u32 in_len, void *out_buf, u32 *out_len)
{
	u16 wqebb_cnt = 1;
	const struct sss_tool_wqe_info *info = in_buf;

	if (!sss_tool_check_input_pointer(nic_dev, in_buf, out_buf, out_len))
		return -EINVAL;

	if (in_len != sizeof(*info)) {
		tool_err("Fail to get tx wqe info, in len %u is not equal to %lu\n",
			 in_len, sizeof(*info));
		return -EINVAL;
	}

	return sss_tool_get_wqe_info(nic_dev, (u16)info->q_id, (u16)info->wqe_id, wqebb_cnt,
				     out_buf, (u16 *)out_len, SSSNIC_SQ);
}

int sss_tool_get_rx_info(struct sss_nic_dev *nic_dev, const void *in_buf,
			 u32 in_len, void *out_buf, u32 *out_len)
{
	int ret;
	u16 q_id;
	struct sss_tool_rq_info *rq_info = out_buf;

	if (!sss_tool_check_input_pointer(nic_dev, in_buf, out_buf, out_len))
		return -EINVAL;

	if (in_len != sizeof(u32)) {
		tool_err("Invalid in len: %u is not equal to %lu\n",
			 in_len, sizeof(u32));
		return -EINVAL;
	}

	if (*out_len != sizeof(*rq_info)) {
		tool_err("Invalid out len: %u is not equal to %lu\n",
			 *out_len, sizeof(*rq_info));
		return -EINVAL;
	}

	q_id = (u16)(*((u32 *)in_buf));

	ret = sss_tool_get_queue_info(nic_dev, q_id, out_buf, SSSNIC_RQ);
	if (ret != 0) {
		tool_err("Fail to get rq info, ret: %d.\n", ret);
		return ret;
	}

	rq_info->pending_limt = nic_dev->rq_desc_group[q_id].last_pending_limt;
	rq_info->msix_vector = nic_dev->rq_desc_group[q_id].irq_id;
	rq_info->delta = (u16)nic_dev->rq_desc_group[q_id].delta;
	rq_info->sw_pi = nic_dev->rq_desc_group[q_id].pi;
	rq_info->coalesc_timer_cfg = nic_dev->rq_desc_group[q_id].last_coal_timer;
	rq_info->ci = (u16)(nic_dev->rq_desc_group[q_id].ci &
			    nic_dev->rq_desc_group[q_id].qid_mask);

	return 0;
}

int sss_tool_get_rx_wqe_info(struct sss_nic_dev *nic_dev, const void *in_buf,
			     u32 in_len, void *out_buf, u32 *out_len)
{
	u16 wqebb_cnt = 1;
	const struct sss_tool_wqe_info *info = in_buf;

	if (!sss_tool_check_input_pointer(nic_dev, in_buf, out_buf, out_len))
		return -EINVAL;

	if (in_len != sizeof(struct sss_tool_wqe_info)) {
		tool_err("Fail to get rx wqe info, in len: %u is not equal to %lu\n",
			 in_len, sizeof(struct sss_tool_wqe_info));
		return -EINVAL;
	}

	return sss_tool_get_wqe_info(nic_dev, (u16)info->q_id, (u16)info->wqe_id, wqebb_cnt,
				     out_buf, (u16 *)out_len, SSSNIC_RQ);
}

int sss_tool_get_rx_cqe_info(struct sss_nic_dev *nic_dev, const void *in_buf,
			     u32 in_len, void *out_buf, u32 *out_len)
{
	u16 wqe_id = 0;
	u16 q_id = 0;
	const struct sss_tool_wqe_info *info = in_buf;

	if (!sss_tool_check_input_pointer(nic_dev, in_buf, out_buf, out_len))
		return -EINVAL;

	if (in_len != sizeof(struct sss_tool_wqe_info)) {
		tool_err("Fail to get rx cqe info, in len: %u is not equal to %lu\n",
			 in_len, sizeof(struct sss_tool_wqe_info));
		return -EINVAL;
	}

	if (*out_len != sizeof(struct sss_nic_cqe)) {
		tool_err("Fail to get rx cqe info, out len: %u is not equal to %lu\n",
			 *out_len, sizeof(struct sss_nic_cqe));
		return -EINVAL;
	}

	wqe_id = (u16)info->wqe_id;
	q_id = (u16)info->q_id;

	if (q_id >= nic_dev->qp_res.qp_num || wqe_id >= nic_dev->rq_desc_group[q_id].q_depth) {
		tool_err("Fail to get rx cqe info, q_id[%u] >= %u, or wqe idx[%u] >= %u.\n",
			 q_id, nic_dev->qp_res.qp_num, wqe_id,
			 nic_dev->rq_desc_group[q_id].q_depth);
		return -EFAULT;
	}

	memcpy(out_buf, nic_dev->rq_desc_group[q_id].rx_desc_group[wqe_id].cqe,
	       sizeof(struct sss_nic_cqe));

	return 0;
}

int sss_tool_get_q_num(struct sss_nic_dev *nic_dev, const void *in_buf, u32 in_len,
		       void *out_buf, u32 *out_len)
{
	if (!SSS_CHANNEL_RES_VALID(nic_dev)) {
		tool_err("Fail to get queue number, netdev is down\n");
		return -EFAULT;
	}

	if (!out_buf || !out_len) {
		tool_err("Invalid param, use null pointer.\n");
		return -EINVAL;
	}

	if (*out_len != sizeof(nic_dev->qp_res.qp_num)) {
		tool_err("Invalid out len: %u is not equal to %lu\n",
			 *out_len, sizeof(nic_dev->qp_res.qp_num));
		return -EINVAL;
	}

	*((u16 *)out_buf) = nic_dev->qp_res.qp_num;

	return 0;
}

int sss_tool_get_inter_num(struct sss_nic_dev *nic_dev, const void *in_buf,
			   u32 in_len, void *out_buf, u32 *out_len)
{
	u16 intr_num = sss_nic_intr_num(nic_dev->hwdev);

	if (!out_buf || !out_len) {
		tool_err("Invalid param, use null pointer\n");
		return -EFAULT;
	}

	if (*out_len != sizeof(intr_num)) {
		tool_err("Invalid out len:%u is not equal to %lu\n",
			 *out_len, sizeof(intr_num));
		return -EFAULT;
	}

	*(u16 *)out_buf = intr_num;

	return 0;
}
