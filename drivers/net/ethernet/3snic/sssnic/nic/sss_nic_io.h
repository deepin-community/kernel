/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_IO_H
#define SSS_NIC_IO_H

#include "sss_hw.h"
#include "sss_hw_wq.h"
#include "sss_nic_io_define.h"

#define SSSNIC_RQ_WQEBB_SHIFT		3
#define SSSNIC_CQE_SIZE_SHIFT		4
#define SSSNIC_SQ_WQEBB_SHIFT		4
#define SSSNIC_MIN_QUEUE_DEPTH		128
#define SSSNIC_MAX_RX_QUEUE_DEPTH	16384
#define SSSNIC_MAX_TX_QUEUE_DEPTH	65536
#define SSSNIC_SQ_WQEBB_SIZE		BIT(SSSNIC_SQ_WQEBB_SHIFT)

/* ******************** DOORBELL DEFINE INFO ******************** */
#define DB_INFO_CFLAG_SHIFT		23
#define DB_INFO_QID_SHIFT		0
#define DB_INFO_TYPE_SHIFT		27
#define DB_INFO_NON_FILTER_SHIFT	22
#define DB_INFO_COS_SHIFT		24

#define DB_INFO_COS_MASK		0x7U
#define DB_INFO_QID_MASK		0x1FFFU
#define DB_INFO_CFLAG_MASK		0x1U
#define DB_INFO_TYPE_MASK		0x1FU
#define DB_INFO_NON_FILTER_MASK		0x1U
#define SSSNIC_DB_INFO_SET(val, member)	\
		(((u32)(val) & DB_INFO_##member##_MASK) << \
		 DB_INFO_##member##_SHIFT)

#define DB_PI_HIGH_MASK			0xFFU
#define DB_PI_LOW_MASK			0xFFU
#define DB_PI_HI_SHIFT			8
#define SRC_TYPE			1
#define DB_PI_HIGH(pi)		(((pi) >> DB_PI_HI_SHIFT) & DB_PI_HIGH_MASK)
#define DB_PI_LOW(pi)			((pi) & DB_PI_LOW_MASK)
#define DB_ADDR(queue, pi)	((u64 *)((queue)->db_addr) + DB_PI_LOW(pi))

#define sss_nic_get_sq_local_pi(sq) SSS_WQ_MASK_ID(&(sq)->wq, (sq)->wq.pi)
#define sss_nic_get_sq_local_ci(sq) SSS_WQ_MASK_ID(&(sq)->wq, (sq)->wq.ci)
#define sss_nic_get_sq_hw_ci(sq)                                               \
	SSS_WQ_MASK_ID(&(sq)->wq, sss_hw_cpu16(*(u16 *)(sq)->tx.ci_addr))

#define sss_nic_get_rq_local_pi(rq) SSS_WQ_MASK_ID(&(rq)->wq, (rq)->wq.pi)
#define sss_nic_get_rq_local_ci(rq) SSS_WQ_MASK_ID(&(rq)->wq, (rq)->wq.ci)

/* CFLAG_DATA_PATH */
#define RQ_CFLAG_DP			1
#define SQ_CFLAG_DP			0

enum sss_nic_queue_type {
	SSSNIC_SQ,
	SSSNIC_RQ,
	SSSNIC_MAX_QUEUE_TYPE
};

struct sss_nic_db {
	u32 db_info;
	u32 pi_hi;
};

enum sss_nic_rq_wqe_type {
	SSSNIC_COMPACT_RQ_WQE,
	SSSNIC_NORMAL_RQ_WQE,
	SSSNIC_EXTEND_RQ_WQE,
};

int sss_nic_io_resource_init(struct sss_nic_io *nic_io);
int sss_nic_init_qp_info(struct sss_nic_io *nic_io, struct sss_nic_qp_info *qp_info);
int sss_nic_alloc_qp(struct sss_nic_io *nic_io,
		     struct sss_irq_desc *qp_msix_arry, struct sss_nic_qp_info *qp_info);
void sss_nic_io_resource_deinit(struct sss_nic_io *nic_io);
void sss_nic_free_qp(struct sss_nic_io *nic_io, struct sss_nic_qp_info *qp_info);
void sss_nic_deinit_qp_info(struct sss_nic_io *nic_io, struct sss_nic_qp_info *qp_info);
int sss_nic_init_qp_ctx(struct sss_nic_io *nic_io);
void sss_nic_deinit_qp_ctx(void *hwdev);

/* *
 * @brief sss_nic_write_db - write doorbell
 * @param queue: nic io queue
 * @param cos: cos index
 * @param cflag: 0--sq, 1--rq
 * @param pi: product index
 */
static inline void sss_nic_write_db(struct sss_nic_io_queue *queue,
				    int cos, u8 cflag, u16 pi)
{
	struct sss_nic_db doorbell;

	doorbell.db_info = SSSNIC_DB_INFO_SET(SRC_TYPE, TYPE) | SSSNIC_DB_INFO_SET(cflag, CFLAG) |
			   SSSNIC_DB_INFO_SET(cos, COS) | SSSNIC_DB_INFO_SET(queue->qid, QID);
	doorbell.pi_hi = DB_PI_HIGH(pi);
	doorbell.db_info = sss_hw_be32(doorbell.db_info);
	doorbell.pi_hi = sss_hw_be32(doorbell.pi_hi);

	/* make sure write correctly db to reg */
	wmb();

	writeq(*((u64 *)&doorbell), DB_ADDR(queue, pi));
}

#endif
