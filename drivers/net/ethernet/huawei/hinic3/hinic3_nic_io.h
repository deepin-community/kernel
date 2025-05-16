/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_NIC_IO_H
#define HINIC3_NIC_IO_H

#include "hinic3_crm.h"
#include "hinic3_common.h"
#include "hinic3_wq.h"

#define HINIC3_MAX_TX_QUEUE_DEPTH	65536
#define HINIC3_MAX_RX_QUEUE_DEPTH	16384

#define HINIC3_MIN_QUEUE_DEPTH		128

#define HINIC3_SQ_WQEBB_SHIFT		4
#define HINIC3_RQ_WQEBB_SHIFT		3

#define HINIC3_SQ_WQEBB_SIZE		BIT(HINIC3_SQ_WQEBB_SHIFT)
#define HINIC3_CQE_SIZE_SHIFT		4

enum hinic3_rq_wqe_type {
	HINIC3_COMPACT_RQ_WQE,
	HINIC3_NORMAL_RQ_WQE,
	HINIC3_EXTEND_RQ_WQE,
};

struct hinic3_io_queue {
	struct hinic3_wq wq;
	union {
		u8 wqe_type; /* for rq */
		u8 owner; /* for sq */
	};
	u8			rsvd1;
	u16			rsvd2;

	u16 q_id;
	u16 msix_entry_idx;

	u8 __iomem *db_addr;
	void *cons_idx_addr;
} ____cacheline_aligned;

struct hinic3_nic_db {
	u32 db_info;
	u32 pi_hi;
};

struct hinic3_tx_rx_ops {
	void (*tx_set_wqebb_cnt)(void *wqe_combo, u32 offload, u16 num_sge);
	void (*tx_set_wqe_task)(void *wqe_combo, void *offload_info);
	void (*rx_get_cqe_info)(void *rx_cqe, void *cqe_info, u8 cqe_mode);
	bool (*rx_cqe_done)(void *rxq, void **rx_cqe);
};

struct hinic3_rq_ci_wb {
	union {
		struct {
			u16 cqe_num;
			u16 hw_ci;
		} bs;
		u32 value;
	} dw0;

	u32 rsvd[3];
};

#ifdef static
#undef static
#define LLT_STATIC_DEF_SAVED
#endif

/* *
 * @brief hinic3_get_sq_free_wqebbs - get send queue free wqebb
 * @param sq: send queue
 * @retval : number of free wqebb
 */
static inline u16 hinic3_get_sq_free_wqebbs(struct hinic3_io_queue *sq)
{
	return hinic3_wq_free_wqebbs(&sq->wq);
}

/* *
 * @brief hinic3_update_sq_local_ci - update send queue local consumer index
 * @param sq: send queue
 * @param wqe_cnt: number of wqebb
 */
static inline void hinic3_update_sq_local_ci(struct hinic3_io_queue *sq,
					     u16 wqebb_cnt)
{
	hinic3_wq_put_wqebbs(&sq->wq, wqebb_cnt);
}

/* *
 * @brief hinic3_get_sq_local_ci - get send queue local consumer index
 * @param sq: send queue
 * @retval : local consumer index
 */
static inline u16 hinic3_get_sq_local_ci(const struct hinic3_io_queue *sq)
{
	return WQ_MASK_IDX(&sq->wq, sq->wq.cons_idx);
}

/* *
 * @brief hinic3_get_sq_local_pi - get send queue local producer index
 * @param sq: send queue
 * @retval : local producer index
 */
static inline u16 hinic3_get_sq_local_pi(const struct hinic3_io_queue *sq)
{
	return WQ_MASK_IDX(&sq->wq, sq->wq.prod_idx);
}

/* *
 * @brief hinic3_get_sq_hw_ci - get send queue hardware consumer index
 * @param sq: send queue
 * @retval : hardware consumer index
 */
static inline u16 hinic3_get_sq_hw_ci(const struct hinic3_io_queue *sq)
{
	return WQ_MASK_IDX(&sq->wq,
			   hinic3_hw_cpu16(*(u16 *)sq->cons_idx_addr));
}

/* *
 * @brief hinic3_get_rq_hw_ci - get recv queue hardware consumer index
 * @param rq: recv queue
 * @retval : hardware consumer index
 */
static inline u16 hinic3_get_rq_hw_ci(const struct hinic3_io_queue *rq)
{
	u16 hw_ci;
	u32 rq_ci_wb;

	rq_ci_wb = hinic3_hw_cpu32(*(u32 *)rq->cons_idx_addr);
	hw_ci = ((struct hinic3_rq_ci_wb *) &rq_ci_wb)->dw0.bs.hw_ci;

	return WQ_MASK_IDX(&rq->wq, hw_ci);
}

/* *
 * @brief hinic3_get_sq_one_wqebb - get send queue wqe with single wqebb
 * @param sq: send queue
 * @param pi: return current pi
 * @retval : wqe base address
 */
static inline void *hinic3_get_sq_one_wqebb(struct hinic3_io_queue *sq, u16 *pi)
{
	return hinic3_wq_get_one_wqebb(&sq->wq, pi);
}

/* *
 * @brief hinic3_get_sq_multi_wqebb - get send queue wqe with multiple wqebbs
 * @param sq: send queue
 * @param wqebb_cnt: wqebb counter
 * @param pi: return current pi
 * @param second_part_wqebbs_addr: second part wqebbs base address
 * @param first_part_wqebbs_num: number wqebbs of first part
 * @retval : first part wqebbs base address
 */
static inline void *hinic3_get_sq_multi_wqebbs(struct hinic3_io_queue *sq,
					       u16 wqebb_cnt, u16 *pi,
					       void **second_part_wqebbs_addr,
					       u16 *first_part_wqebbs_num)
{
	return hinic3_wq_get_multi_wqebbs(&sq->wq, wqebb_cnt, pi,
					  second_part_wqebbs_addr,
					  first_part_wqebbs_num);
}

/* *
 * @brief hinic3_get_and_update_sq_owner - get and update send queue owner bit
 * @param sq: send queue
 * @param curr_pi: current pi
 * @param wqebb_cnt: wqebb counter
 * @retval : owner bit
 */
static inline u16 hinic3_get_and_update_sq_owner(struct hinic3_io_queue *sq,
						 u16 curr_pi, u16 wqebb_cnt)
{
	u16 owner = sq->owner;

	if (unlikely(curr_pi + wqebb_cnt >= sq->wq.q_depth))
		sq->owner = !sq->owner;

	return owner;
}

/* *
 * @brief hinic3_get_sq_wqe_with_owner - get send queue wqe with owner
 * @param sq: send queue
 * @param wqebb_cnt: wqebb counter
 * @param pi: return current pi
 * @param owner: return owner bit
 * @param second_part_wqebbs_addr: second part wqebbs base address
 * @param first_part_wqebbs_num: number wqebbs of first part
 * @retval : first part wqebbs base address
 */
static inline void *hinic3_get_sq_wqe_with_owner(struct hinic3_io_queue *sq,
						 u16 wqebb_cnt, u16 *pi,
						 u16 *owner,
						 void **second_part_wqebbs_addr,
						 u16 *first_part_wqebbs_num)
{
	void *wqe = hinic3_wq_get_multi_wqebbs(&sq->wq, wqebb_cnt, pi,
					       second_part_wqebbs_addr,
					       first_part_wqebbs_num);

	*owner = sq->owner;
	if (unlikely(*pi + wqebb_cnt >= sq->wq.q_depth))
		sq->owner = !sq->owner;

	return wqe;
}

/* *
 * @brief hinic3_rollback_sq_wqebbs - rollback send queue wqe
 * @param sq: send queue
 * @param wqebb_cnt: wqebb counter
 * @param owner: owner bit
 */
static inline void hinic3_rollback_sq_wqebbs(struct hinic3_io_queue *sq,
					     u16 wqebb_cnt, u16 owner)
{
	if (owner != sq->owner)
		sq->owner = (u8)owner;
	sq->wq.prod_idx -= wqebb_cnt;
}

/* *
 * @brief hinic3_rq_wqe_addr - get receive queue wqe address by queue index
 * @param rq: receive queue
 * @param idx: wq index
 * @retval: wqe base address
 */
static inline void *hinic3_rq_wqe_addr(struct hinic3_io_queue *rq, u16 idx)
{
	return hinic3_wq_wqebb_addr(&rq->wq, idx);
}

/* *
 * @brief hinic3_update_rq_local_ci - update receive queue local consumer index
 * @param sq: receive queue
 * @param wqe_cnt: number of wqebb
 */
static inline void hinic3_update_rq_local_ci(struct hinic3_io_queue *rq,
					     u16 wqebb_cnt)
{
	hinic3_wq_put_wqebbs(&rq->wq, wqebb_cnt);
}

/* *
 * @brief hinic3_get_rq_local_ci - get receive queue local ci
 * @param rq: receive queue
 * @retval: receive queue local ci
 */
static inline u16 hinic3_get_rq_local_ci(const struct hinic3_io_queue *rq)
{
	return WQ_MASK_IDX(&rq->wq, rq->wq.cons_idx);
}

/* *
 * @brief hinic3_get_rq_local_pi - get receive queue local pi
 * @param rq: receive queue
 * @retval: receive queue local pi
 */
static inline u16 hinic3_get_rq_local_pi(const struct hinic3_io_queue *rq)
{
	return WQ_MASK_IDX(&rq->wq, rq->wq.prod_idx);
}

/* ******************** DB INFO ******************** */
#define DB_INFO_QID_SHIFT		0
#define DB_INFO_NON_FILTER_SHIFT	22
#define DB_INFO_CFLAG_SHIFT		23
#define DB_INFO_COS_SHIFT		24
#define DB_INFO_TYPE_SHIFT		27

#define DB_INFO_QID_MASK		0x1FFFU
#define DB_INFO_NON_FILTER_MASK		0x1U
#define DB_INFO_CFLAG_MASK		0x1U
#define DB_INFO_COS_MASK		0x7U
#define DB_INFO_TYPE_MASK		0x1FU
#define DB_INFO_SET(val, member)	\
		(((u32)(val) & DB_INFO_##member##_MASK) << \
		 DB_INFO_##member##_SHIFT)

#define DB_PI_LOW_MASK			0xFFU
#define DB_PI_HIGH_MASK			0xFFU
#define DB_PI_LOW(pi)			((pi) & DB_PI_LOW_MASK)
#define DB_PI_HI_SHIFT			8
#define DB_PI_HIGH(pi)		(((pi) >> DB_PI_HI_SHIFT) & DB_PI_HIGH_MASK)
#define DB_ADDR(queue, pi)	((u64 *)((queue)->db_addr) + DB_PI_LOW(pi))
#define SRC_TYPE			1

/* CFLAG_DATA_PATH */
#define SQ_CFLAG_DP			0
#define RQ_CFLAG_DP			1
/* *
 * @brief hinic3_write_db - write doorbell
 * @param queue: nic io queue
 * @param cos: cos index
 * @param cflag: 0--sq, 1--rq
 * @param pi: product index
 */
static inline void hinic3_write_db(struct hinic3_io_queue *queue, int cos,
				   u8 cflag, u16 pi)
{
	struct hinic3_nic_db db;

	db.db_info = DB_INFO_SET(SRC_TYPE, TYPE) | DB_INFO_SET(cflag, CFLAG) |
			DB_INFO_SET(cos, COS) | DB_INFO_SET(queue->q_id, QID);
	db.pi_hi = DB_PI_HIGH(pi);
	/* Data should be written to HW in Big Endian Format */
	db.db_info = hinic3_hw_be32(db.db_info);
	db.pi_hi = hinic3_hw_be32(db.pi_hi);

	wmb(); /* Write all before the doorbell */

	writeq(*((u64 *)(u8 *)&db), DB_ADDR(queue, pi));
}

struct hinic3_dyna_qp_params {
	u16	num_qps;
	u32	sq_depth;
	u32	rq_depth;

	struct hinic3_io_queue *sqs;
	struct hinic3_io_queue *rqs;
};

int hinic3_alloc_qps(void *hwdev, struct irq_info *qps_msix_arry,
		     struct hinic3_dyna_qp_params *qp_params);
void hinic3_free_qps(void *hwdev, struct hinic3_dyna_qp_params *qp_params);
int hinic3_init_qps(void *hwdev, struct hinic3_dyna_qp_params *qp_params);
void hinic3_deinit_qps(void *hwdev, struct hinic3_dyna_qp_params *qp_params);
int hinic3_init_nicio_res(void *hwdev);
void hinic3_deinit_nicio_res(void *hwdev);
int hinic3_get_rq_wqe_type(void *hwdev);
#endif
