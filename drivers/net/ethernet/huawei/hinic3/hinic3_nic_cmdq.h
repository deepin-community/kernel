/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_NIC_CMDQ_H
#define HINIC3_NIC_CMDQ_H

#include "ossl_knl.h"
#include "hinic3_hw.h"
#include "hinic3_nic.h"

#define HINIC3_Q_CTXT_MAX		31U /* (2048 - 8) / 64 */
#define HINIC3_QP_CTXT_HEADER_SIZE	16U

enum hinic3_qp_ctxt_type {
	HINIC3_QP_CTXT_TYPE_SQ,
	HINIC3_QP_CTXT_TYPE_RQ,
};

struct hinic3_nic_cmdq_ops {
	u8 (*prepare_cmd_buf_clean_tso_lro_space)(struct hinic3_nic_io *nic_io,
						  struct hinic3_cmd_buf *cmd_buf,
						  enum hinic3_qp_ctxt_type ctxt_type);
	u8 (*prepare_cmd_buf_qp_context_multi_store)(struct hinic3_nic_io *nic_io,
						     struct hinic3_cmd_buf *cmd_buf,
						     enum hinic3_qp_ctxt_type ctxt_type,
						     u16 start_qid, u16 max_ctxts);
	u8 (*prepare_cmd_buf_modify_svlan)(struct hinic3_cmd_buf *cmd_buf,
					   u16 func_id, u16 vlan_tag, u16 q_id, u8 vlan_mode);
	u8 (*prepare_cmd_buf_set_rss_indir_table)(const struct hinic3_nic_io *nic_io,
					      const u32 *indir_table,
						  struct hinic3_cmd_buf *cmd_buf);
	u8 (*prepare_cmd_buf_get_rss_indir_table)(const struct hinic3_nic_io *nic_io,
						  const struct hinic3_cmd_buf *cmd_buf);
	void (*cmd_buf_to_rss_indir_table)(const struct hinic3_cmd_buf *cmd_buf, u32 *indir_table);
};

struct hinic3_sq_ctxt {
	u32	ci_pi;
	u32	drop_mode_sp;
	u32	wq_pfn_hi_owner;
	u32	wq_pfn_lo;

	u32	rsvd0;
	u32	pkt_drop_thd;
	u32	global_sq_id;
	u32	vlan_ceq_attr;

	u32	pref_cache;
	u32	pref_ci_owner;
	u32	pref_wq_pfn_hi_ci;
	u32	pref_wq_pfn_lo;

	u32	rsvd8;
	u32	rsvd9;
	u32	wq_block_pfn_hi;
	u32	wq_block_pfn_lo;
};

struct hinic3_rq_ctxt {
	u32	ci_pi;
	u32	ceq_attr;
	u32	wq_pfn_hi_type_owner;
	u32	wq_pfn_lo;

	u32	rsvd[3];
	u32	cqe_sge_len;

	u32	pref_cache;
	u32	pref_ci_owner;
	u32	pref_wq_pfn_hi_ci;
	u32	pref_wq_pfn_lo;

	u32	pi_paddr_hi;
	u32	pi_paddr_lo;
	u32	wq_block_pfn_hi;
	u32	wq_block_pfn_lo;
};

struct hinic3_nic_cmdq_ops *hinic3_nic_cmdq_get_sw_ops(void);
struct hinic3_nic_cmdq_ops *hinic3_nic_cmdq_get_hw_ops(void);

void hinic3_nic_cmdq_adapt_init(struct hinic3_nic_io *nic_io);
void hinic3_sq_prepare_ctxt(struct hinic3_io_queue *sq, u16 sq_id, struct hinic3_sq_ctxt *sq_ctxt);
void hinic3_rq_prepare_ctxt(struct hinic3_io_queue *rq, struct hinic3_rq_ctxt *rq_ctxt);
#endif
