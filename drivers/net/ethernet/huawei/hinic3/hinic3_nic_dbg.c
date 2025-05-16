// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/types.h>

#include "ossl_knl.h"
#include "hinic3_crm.h"
#include "hinic3_hw.h"
#include "hinic3_mt.h"
#include "hinic3_nic_qp.h"
#include "hinic3_nic_io.h"
#include "hinic3_srv_nic.h"
#include "hinic3_nic.h"

int hinic3_dbg_get_wqe_info(void *hwdev, u16 q_id, u16 idx, u16 wqebb_cnt,
			    u8 *wqe, const u16 *wqe_size, enum hinic3_queue_type q_type)
{
	struct hinic3_io_queue *queue = NULL;
	struct hinic3_nic_io *nic_io = NULL;
	void *src_wqebb = NULL;
	u32 i, offset;

	if (!hwdev) {
		pr_err("hwdev is NULL.\n");
		return -EINVAL;
	}

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;
	if (q_id >= nic_io->num_qps) {
		pr_err("q_id[%u] > num_qps_cfg[%u].\n", q_id, nic_io->num_qps);
		return -EINVAL;
	}

	queue = (q_type == HINIC3_SQ) ? &nic_io->sq[q_id] : &nic_io->rq[q_id];

	if ((idx + wqebb_cnt) > queue->wq.q_depth) {
		pr_err("(idx[%u] + idx[%u]) > q_depth[%u].\n", idx, wqebb_cnt, queue->wq.q_depth);
		return -EINVAL;
	}

	if (*wqe_size != (queue->wq.wqebb_size * wqebb_cnt)) {
		pr_err("Unexpect out buf size from user :%u, expect: %d\n",
		       *wqe_size, (queue->wq.wqebb_size * wqebb_cnt));
		return -EINVAL;
	}

	for (i = 0; i < wqebb_cnt; i++) {
		src_wqebb = hinic3_wq_wqebb_addr(&queue->wq, (u16)WQ_MASK_IDX(&queue->wq, idx + i));
		offset = queue->wq.wqebb_size * i;
		memcpy(wqe + offset, src_wqebb, queue->wq.wqebb_size);
	}

	return 0;
}

int hinic3_dbg_get_sq_info(void *hwdev, u16 q_id, struct nic_sq_info *sq_info,
			   u32 msg_size)
{
	struct hinic3_nic_io *nic_io = NULL;
	struct hinic3_io_queue *sq = NULL;

	if (!hwdev || !sq_info) {
		pr_err("hwdev or sq_info is NULL.\n");
		return -EINVAL;
	}

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;
	if (q_id >= nic_io->num_qps) {
		nic_err(nic_io->dev_hdl, "Input queue id(%u) is larger than the actual queue number\n",
			q_id);
		return -EINVAL;
	}

	if (msg_size != sizeof(*sq_info)) {
		nic_err(nic_io->dev_hdl, "Unexpect out buf size from user :%u, expect: %lu\n",
			msg_size, sizeof(*sq_info));
		return -EINVAL;
	}

	sq = &nic_io->sq[q_id];
	if (!sq)
		return -EINVAL;

	sq_info->q_id = q_id;
	sq_info->pi = hinic3_get_sq_local_pi(sq);
	sq_info->ci = hinic3_get_sq_local_ci(sq);
	sq_info->fi = hinic3_get_sq_hw_ci(sq);
	sq_info->q_depth = sq->wq.q_depth;
	sq_info->wqebb_size = sq->wq.wqebb_size;

	sq_info->ci_addr = sq->cons_idx_addr;

	sq_info->cla_addr = sq->wq.wq_block_paddr;
	sq_info->slq_handle = sq;

	sq_info->doorbell.map_addr = (u64 *)sq->db_addr;

	return 0;
}

int hinic3_dbg_get_rq_info(void *hwdev, u16 q_id, struct nic_rq_info *rq_info,
			   u32 msg_size)
{
	struct hinic3_nic_io *nic_io = NULL;
	struct hinic3_io_queue *rq = NULL;

	if (!hwdev || !rq_info) {
		pr_err("hwdev or rq_info is NULL.\n");
		return -EINVAL;
	}

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;
	if (q_id >= nic_io->num_qps) {
		nic_err(nic_io->dev_hdl, "Input queue id(%u) is larger than the actual queue number\n",
			q_id);
		return -EINVAL;
	}

	if (msg_size != sizeof(*rq_info)) {
		nic_err(nic_io->dev_hdl, "Unexpect out buf size from user: %u, expect: %lu\n",
			msg_size, sizeof(*rq_info));
		return -EINVAL;
	}

	rq = &nic_io->rq[q_id];
	if (!rq)
		return -EINVAL;

	rq_info->q_id = q_id;

	rq_info->wqebb_size = rq->wq.wqebb_size;
	rq_info->q_depth = (u16)rq->wq.q_depth;

	rq_info->buf_len = nic_io->rx_buff_len;

	rq_info->slq_handle = rq;

	rq_info->ci_wqe_page_addr = hinic3_wq_get_first_wqe_page_addr(&rq->wq);
	rq_info->ci_cla_tbl_addr = rq->wq.wq_block_paddr;

	rq_info->msix_idx = rq->msix_entry_idx;

	return 0;
}
