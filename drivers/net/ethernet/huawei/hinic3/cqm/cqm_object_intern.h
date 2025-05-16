/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef CQM_OBJECT_INTERN_H
#define CQM_OBJECT_INTERN_H

#include "ossl_knl.h"
#include "cqm_object.h"

#define CQM_CQ_DEPTH_MAX           32768
#define CQM_CQ_DEPTH_MIN           256

/* linkwqe */
#define CQM_LINK_WQE_CTRLSL_VALUE  2
#define CQM_LINK_WQE_LP_VALID      1
#define CQM_LINK_WQE_LP_INVALID    0
#define CQM_LINK_WQE_OWNER_VALID   1
#define CQM_LINK_WQE_OWNER_INVALID 0

#define CQM_ADDR_COMBINE(high_addr, low_addr) \
	((((dma_addr_t)(high_addr)) << 32) + ((dma_addr_t)(low_addr)))
#define CQM_ADDR_HI(addr) ((u32)((u64)(addr) >> 32))
#define CQM_ADDR_LW(addr) ((u32)((u64)(addr) & 0xffffffff))

#define CQM_QPC_LAYOUT_TABLE_SIZE  16
struct tag_cqm_qpc_layout_table_node {
	u32 type;
	u32 size;
	u32 offset;
	struct tag_cqm_object *object;
};

struct tag_cqm_qpc_mpt_info {
	struct tag_cqm_qpc_mpt common;
	/* Different service has different QPC.
	 * The large QPC/mpt will occupy some continuous indexes in bitmap.
	 */
	u32 index_count;
	struct tag_cqm_qpc_layout_table_node qpc_layout_table[CQM_QPC_LAYOUT_TABLE_SIZE];
};

struct tag_cqm_nonrdma_qinfo {
	struct tag_cqm_queue common;
	u32 wqe_size;
	/* Number of WQEs in each buffer (excluding link WQEs)
	 * For SRQ, the value is the number of WQEs contained in a container.
	 */
	u32 wqe_per_buf;
	u32 q_ctx_size;
	/* When different services use CTXs of different sizes,
	 * a large CTX occupies multiple consecutive indexes in the bitmap.
	 */
	u32 index_count;

	/* add for srq */
	u32 container_size;
};

struct tag_cqm_rdma_qinfo {
	struct tag_cqm_queue common;
	bool room_header_alloc;
	/* This field is used to temporarily record the new object_size during
	 * CQ resize.
	 */
	u32 new_object_size;
	u32 q_ctx_size;
	/* When different services use CTXs of different sizes,
	 * a large CTX occupies multiple consecutive indexes in the bitmap.
	 */
	u32 index_count;
};

struct tag_cqm_rdma_table {
	struct tag_cqm_mtt_rdmarc common;
	struct tag_cqm_buf buf;
};

void cqm_container_free(u8 *srq_head_container, u8 *srq_tail_container,
			struct tag_cqm_queue *common);
s32 cqm_container_create(struct tag_cqm_object *object, u8 **container_addr, bool link);
s32 cqm_share_recv_queue_create(struct tag_cqm_object *object);
void cqm_share_recv_queue_delete(struct tag_cqm_object *object);
s32 cqm_qpc_mpt_create(struct tag_cqm_object *object, bool low2bit_align_en);
void cqm_qpc_mpt_delete(struct tag_cqm_object *object);
s32 cqm_nonrdma_queue_create(struct tag_cqm_object *object);
void cqm_nonrdma_queue_delete(struct tag_cqm_object *object);
s32 cqm_rdma_queue_create(struct tag_cqm_object *object);
void cqm_rdma_queue_delete(struct tag_cqm_object *object);
s32 cqm_rdma_table_create(struct tag_cqm_object *object);
void cqm_rdma_table_delete(struct tag_cqm_object *object);
u8 *cqm_rdma_table_offset_addr(struct tag_cqm_object *object, u32 offset, dma_addr_t *paddr);

#endif /* CQM_OBJECT_INTERN_H */
