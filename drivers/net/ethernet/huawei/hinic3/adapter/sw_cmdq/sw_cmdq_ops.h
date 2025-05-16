/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef _SW_CMDQ_PRIVATE_H_
#define _SW_CMDQ_PRIVATE_H_

#include "ossl_knl.h"
#include "hinic3_nic_cmdq.h"

struct hinic3_qp_ctxt_header {
	u16	num_queues;
	u16	queue_type;
	u16	start_qid;
	u16	rsvd;
};

struct hinic3_clean_queue_ctxt {
	struct hinic3_qp_ctxt_header cmdq_hdr;
	u32 rsvd;
};

struct hinic3_qp_ctxt_block {
	struct hinic3_qp_ctxt_header	cmdq_hdr;
	union {
		struct hinic3_sq_ctxt	sq_ctxt[HINIC3_Q_CTXT_MAX];
		struct hinic3_rq_ctxt	rq_ctxt[HINIC3_Q_CTXT_MAX];
	};
};

struct hinic3_vlan_ctx {
	u32 func_id;
	u32 qid; /* if qid = 0xFFFF, config current function all queue */
	u32 vlan_id;
	u32 vlan_mode;
	u32 vlan_sel;
};

#endif
