// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/module.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_nic_cfg.h"
#include "sss_nic_vf_cfg.h"
#include "sss_nic_mag_cfg.h"
#include "sss_nic_rss_cfg.h"
#include "sss_nic_io_define.h"
#include "sss_nic_cfg_define.h"
#include "sss_nic_io.h"
#include "sss_nic_event.h"

#define SSSNIC_DEAULT_DROP_THD_OFF           0
#define SSSNIC_DEAULT_DROP_THD_ON            (0xFFFF)
#define SSSNIC_DEAULT_TX_CI_PENDING_LIMIT    1
#define SSSNIC_DEAULT_TX_CI_COALESCING_TIME  1
#define SSSNIC_WQ_PREFETCH_MIN			1
#define SSSNIC_WQ_PREFETCH_MAX			4
#define SSSNIC_WQ_PREFETCH_THRESHOLD		256
#define SSSNIC_Q_CTXT_MAX		31 /* (2048 - 8) / 64 */

/* performance: ci addr RTE_CACHE_SIZE(64B) alignment */
#define SSSNIC_CI_Q_ADDR_SIZE			(64)

#define SSSNIC_CI_TABLE_SIZE(num_qps, pg_sz)	\
			(ALIGN((num_qps) * SSSNIC_CI_Q_ADDR_SIZE, pg_sz))

#define SSSNIC_CI_PADDR(base_paddr, qid)	((base_paddr) + \
						(qid) * SSSNIC_CI_Q_ADDR_SIZE)

#define SSSNIC_CI_VADDR(base_addr, qid)		((u8 *)(base_addr) + \
						(qid) * SSSNIC_CI_Q_ADDR_SIZE)

#define SSSNIC_SQ_CTX_SIZE(num_sqs)	((u16)(sizeof(struct sss_nic_qp_ctx_header) \
				+ (num_sqs) * sizeof(struct sss_nic_sq_ctx)))

#define SSSNIC_RQ_CTX_SIZE(num_rqs)	((u16)(sizeof(struct sss_nic_qp_ctx_header) \
				+ (num_rqs) * sizeof(struct sss_nic_rq_ctx)))

#define SSSNIC_CI_ID_HIGH_SHIFH				12
#define SSSNIC_CI_HIGN_ID(val)		((val) >> SSSNIC_CI_ID_HIGH_SHIFH)

#define SSSNIC_SQ_CTX_MODE_SP_FLAG_SHIFT			0
#define SSSNIC_SQ_CTX_MODE_PKT_DROP_SHIFT			1

#define SSSNIC_SQ_CTX_MODE_SP_FLAG_MASK			0x1U
#define SSSNIC_SQ_CTX_MODE_PKT_DROP_MASK			0x1U

#define SSSNIC_SET_SQ_CTX_MODE(val, member)		\
			(((val) & SSSNIC_SQ_CTX_MODE_##member##_MASK) \
				<< SSSNIC_SQ_CTX_MODE_##member##_SHIFT)

#define SSSNIC_SQ_CTX_PI_ID_SHIFT				0
#define SSSNIC_SQ_CTX_CI_ID_SHIFT				16

#define SSSNIC_SQ_CTX_PI_ID_MASK				0xFFFFU
#define SSSNIC_SQ_CTX_CI_ID_MASK				0xFFFFU

#define SSSNIC_SET_SQ_CTX_CI_PI(val, member)	\
			(((val) & SSSNIC_SQ_CTX_##member##_MASK) \
				<< SSSNIC_SQ_CTX_##member##_SHIFT)

#define SSSNIC_SQ_CTX_WQ_PAGE_HI_PFN_SHIFT			0
#define SSSNIC_SQ_CTX_WQ_PAGE_OWNER_SHIFT			23

#define SSSNIC_SQ_CTX_WQ_PAGE_HI_PFN_MASK			0xFFFFFU
#define SSSNIC_SQ_CTX_WQ_PAGE_OWNER_MASK			0x1U

#define SSSNIC_SET_SQ_CTX_WQ_PAGE(val, member)		\
			(((val) & SSSNIC_SQ_CTX_WQ_PAGE_##member##_MASK) \
				<< SSSNIC_SQ_CTX_WQ_PAGE_##member##_SHIFT)

#define SSSNIC_SQ_CTX_GLOBAL_SQ_ID_SHIFT			0

#define SSSNIC_SQ_CTX_GLOBAL_SQ_ID_MASK			0x1FFFU

#define SSSNIC_SET_SQ_CTX_GLOBAL_QUEUE_ID(val, member)	\
			(((val) & SSSNIC_SQ_CTX_##member##_MASK) \
				<< SSSNIC_SQ_CTX_##member##_SHIFT)

#define SSSNIC_SQ_CTX_PKT_DROP_THD_ON_SHIFT			0
#define SSSNIC_SQ_CTX_PKT_DROP_THD_OFF_SHIFT			16

#define SSSNIC_SQ_CTX_PKT_DROP_THD_ON_MASK			0xFFFFU
#define SSSNIC_SQ_CTX_PKT_DROP_THD_OFF_MASK			0xFFFFU

#define SSSNIC_SET_SQ_CTX_PKT_DROP_THD(val, member)	\
			(((val) & SSSNIC_SQ_CTX_PKT_DROP_##member##_MASK) \
				<< SSSNIC_SQ_CTX_PKT_DROP_##member##_SHIFT)

#define SSSNIC_SQ_CTX_PREF_CACHE_THRESHOLD_SHIFT		0
#define SSSNIC_SQ_CTX_PREF_CACHE_MAX_SHIFT			14
#define SSSNIC_SQ_CTX_PREF_CACHE_MIN_SHIFT			25

#define SSSNIC_SQ_CTX_PREF_CACHE_THRESHOLD_MASK		0x3FFFU
#define SSSNIC_SQ_CTX_PREF_CACHE_MAX_MASK			0x7FFU
#define SSSNIC_SQ_CTX_PREF_CACHE_MIN_MASK			0x7FU

#define SSSNIC_SQ_CTX_PREF_CI_HI_SHIFT			0
#define SSSNIC_SQ_CTX_PREF_OWNER_SHIFT			4

#define SSSNIC_SQ_CTX_PREF_CI_HI_MASK			0xFU
#define SSSNIC_SQ_CTX_PREF_OWNER_MASK			0x1U

#define SSSNIC_SQ_CTX_PREF_WQ_PFN_HI_SHIFT		0
#define SSSNIC_SQ_CTX_PREF_CI_LOW_SHIFT			20

#define SSSNIC_SQ_CTX_PREF_WQ_PFN_HI_MASK		0xFFFFFU
#define SSSNIC_SQ_CTX_PREF_CI_LOW_MASK			0xFFFU

#define SSSNIC_SET_SQ_CTX_PREF(val, member)		\
			(((val) & SSSNIC_SQ_CTX_PREF_##member##_MASK) \
				<< SSSNIC_SQ_CTX_PREF_##member##_SHIFT)

#define SSSNIC_RQ_CTX_WQ_PAGE_HI_PFN_SHIFT			0
#define SSSNIC_RQ_CTX_WQ_PAGE_WQE_TYPE_SHIFT		28
#define SSSNIC_RQ_CTX_WQ_PAGE_OWNER_SHIFT			31

#define SSSNIC_RQ_CTX_WQ_PAGE_HI_PFN_MASK			0xFFFFFU
#define SSSNIC_RQ_CTX_WQ_PAGE_WQE_TYPE_MASK			0x3U
#define SSSNIC_RQ_CTX_WQ_PAGE_OWNER_MASK			0x1U

#define SSSNIC_SET_RQ_CTX_WQ_PAGE(val, member)		\
			(((val) & SSSNIC_RQ_CTX_WQ_PAGE_##member##_MASK) << \
				SSSNIC_RQ_CTX_WQ_PAGE_##member##_SHIFT)

#define SSSNIC_SQ_CTX_VLAN_TAG_SHIFT				0
#define SSSNIC_SQ_CTX_VLAN_TYPE_SEL_SHIFT			16
#define SSSNIC_SQ_CTX_VLAN_INSERT_MODE_SHIFT		19
#define SSSNIC_SQ_CTX_VLAN_CEQ_EN_SHIFT				23

#define SSSNIC_SQ_CTX_VLAN_TAG_MASK					0xFFFFU
#define SSSNIC_SQ_CTX_VLAN_TYPE_SEL_MASK			0x7U
#define SSSNIC_SQ_CTX_VLAN_INSERT_MODE_MASK			0x3U
#define SSSNIC_SQ_CTX_VLAN_CEQ_EN_MASK				0x1U

#define SSSNIC_SET_SQ_CTX_VLAN_CEQ(val, member)		\
			(((val) & SSSNIC_SQ_CTX_VLAN_##member##_MASK) \
				<< SSSNIC_SQ_CTX_VLAN_##member##_SHIFT)

#define SSSNIC_RQ_CTX_PI_ID_SHIFT				0
#define SSSNIC_RQ_CTX_CI_ID_SHIFT				16

#define SSSNIC_RQ_CTX_PI_ID_MASK				0xFFFFU
#define SSSNIC_RQ_CTX_CI_ID_MASK				0xFFFFU

#define SSSNIC_SET_RQ_CTX_CI_PI(val, member)	\
			(((val) & SSSNIC_RQ_CTX_##member##_MASK) \
				<< SSSNIC_RQ_CTX_##member##_SHIFT)

#define SSSNIC_RQ_CTX_CEQ_ATTR_INTR_SHIFT		21
#define SSSNIC_RQ_CTX_CEQ_ATTR_EN_SHIFT			31

#define SSSNIC_RQ_CTX_CEQ_ATTR_INTR_MASK		0x3FFU
#define SSSNIC_RQ_CTX_CEQ_ATTR_EN_MASK			0x1U

#define SSSNIC_SET_RQ_CTX_CEQ_ATTR(val, member)	\
			(((val) & SSSNIC_RQ_CTX_CEQ_ATTR_##member##_MASK) \
				<< SSSNIC_RQ_CTX_CEQ_ATTR_##member##_SHIFT)

#define SSSNIC_SQ_CTX_WQ_BLOCK_PFN_HI_SHIFT		0

#define SSSNIC_SQ_CTX_WQ_BLOCK_PFN_HI_MASK		0x7FFFFFU

#define SSSNIC_SET_SQ_CTX_WQ_BLOCK(val, member)	\
			(((val) & SSSNIC_SQ_CTX_WQ_BLOCK_##member##_MASK) \
				<< SSSNIC_SQ_CTX_WQ_BLOCK_##member##_SHIFT)

#define SSSNIC_RQ_CTX_PREF_CACHE_THRESHOLD_SHIFT	0
#define SSSNIC_RQ_CTX_PREF_CACHE_MAX_SHIFT			14
#define SSSNIC_RQ_CTX_PREF_CACHE_MIN_SHIFT			25

#define SSSNIC_RQ_CTX_PREF_CACHE_THRESHOLD_MASK		0x3FFFU
#define SSSNIC_RQ_CTX_PREF_CACHE_MAX_MASK			0x7FFU
#define SSSNIC_RQ_CTX_PREF_CACHE_MIN_MASK			0x7FU

#define SSSNIC_RQ_CTX_PREF_CI_HI_SHIFT				0
#define SSSNIC_RQ_CTX_PREF_OWNER_SHIFT				4

#define SSSNIC_RQ_CTX_PREF_CI_HI_MASK				0xFU
#define SSSNIC_RQ_CTX_PREF_OWNER_MASK				0x1U

#define SSSNIC_RQ_CTX_PREF_WQ_PFN_HI_SHIFT			0
#define SSSNIC_RQ_CTX_PREF_CI_LOW_SHIFT				20

#define SSSNIC_RQ_CTX_PREF_WQ_PFN_HI_MASK			0xFFFFFU
#define SSSNIC_RQ_CTX_PREF_CI_LOW_MASK				0xFFFU

#define SSSNIC_SET_RQ_CTX_PREF(val, member)			\
			(((val) & SSSNIC_RQ_CTX_PREF_##member##_MASK) << \
					SSSNIC_RQ_CTX_PREF_##member##_SHIFT)

#define SSSNIC_RQ_CTX_CQE_LEN_SHIFT					28

#define SSSNIC_RQ_CTX_CQE_LEN_MASK					0x3U

#define SSSNIC_SET_RQ_CTX_CQE_LEN(val, member)		\
			(((val) & SSSNIC_RQ_CTX_##member##_MASK) << \
					SSSNIC_RQ_CTX_##member##_SHIFT)

#define SSSNIC_RQ_CTX_WQ_BLOCK_PFN_HI_SHIFT			0

#define SSSNIC_RQ_CTX_WQ_BLOCK_PFN_HI_MASK			0x7FFFFFU

#define SSSNIC_SET_RQ_CTX_WQ_BLOCK(val, member)		\
			(((val) & SSSNIC_RQ_CTX_WQ_BLOCK_##member##_MASK) << \
					SSSNIC_RQ_CTX_WQ_BLOCK_##member##_SHIFT)

#define SSSNIC_WQ_PAGE_PFN(page_addr)		((page_addr) >> 12)
#define SSSNIC_WQ_BLOCK_PFN(page_addr)		((page_addr) >> 9)

enum sss_nic_qp_ctx_type {
	SSSNIC_QP_CTX_TYPE_SQ,
	SSSNIC_QP_CTX_TYPE_RQ,
};

struct sss_nic_qp_ctx_header {
	u16	q_num;
	u16	q_type;
	u16	start_qid;
	u16	rsvd;
};

struct sss_nic_clear_q_ctx {
	struct sss_nic_qp_ctx_header	ctrlq_hdr;
	u32				rsvd;
};

struct sss_nic_rq_ctx {
	u32	ci_pi;
	u32	ceq_attr;
	u32	hi_wq_pfn;
	u32	lo_wq_pfn;

	u32	rsvd[3];
	u32	cqe_sge_len;

	u32	pref_cache;
	u32	pref_ci_owner;
	u32	hi_pref_wq_pfn_ci;
	u32	lo_pref_wq_pfn;

	u32	pi_paddr_hi;
	u32	pi_paddr_lo;
	u32	hi_wq_block_pfn;
	u32	lo_wq_block_pfn;
};

struct sss_nic_sq_ctx {
	u32	ci_pi;
	u32	drop_mode_sp;
	u32	hi_wq_pfn;
	u32	lo_wq_pfn;

	u32	rsvd0;
	u32	pkt_drop_thd;
	u32	global_sq_id;
	u32	vlan_ceq_attr;

	u32	pref_cache;
	u32	pref_ci_owner;
	u32	hi_pref_wq_pfn_ci;
	u32	lo_pref_wq_pfn;

	u32	rsvd8;
	u32	rsvd9;
	u32	hi_wq_block_pfn;
	u32	lo_wq_block_pfn;
};

struct sss_nic_rq_ctx_block {
	struct sss_nic_qp_ctx_header	ctrlq_hdr;
	struct sss_nic_rq_ctx		rq_ctxt[SSSNIC_Q_CTXT_MAX];
};

struct sss_nic_sq_ctx_block {
	struct sss_nic_qp_ctx_header	ctrlq_hdr;
	struct sss_nic_sq_ctx		sq_ctxt[SSSNIC_Q_CTXT_MAX];
};

static int sss_nic_create_sq(struct sss_nic_io *nic_io,
			     struct sss_nic_io_queue *sq,
			     u16 qid, u32 sq_depth, u16 msix_id)
{
	int ret = 0;

	sq->qid = qid;
	sq->msix_id = msix_id;
	sq->owner = 1;

	ret = sss_create_wq(nic_io->hwdev, &sq->wq, sq_depth,
			    (u16)BIT(SSSNIC_SQ_WQEBB_SHIFT));
	if (ret != 0)
		nic_err(nic_io->dev_hdl, "Fail to create sq(%u) wq\n", qid);

	return ret;
}

static void sss_nic_destroy_sq(struct sss_nic_io_queue *sq)
{
	sss_destroy_wq(&sq->wq);
}

static int sss_nic_create_rq(struct sss_nic_io *nic_io,
			     struct sss_nic_io_queue *rq,
			     u16 qid, u32 rq_depth, u16 msix_id)
{
	int ret = 0;

	rq->qid = qid;
	rq->msix_id = msix_id;
	rq->wqe_type = SSSNIC_NORMAL_RQ_WQE;

	rq->rx.pi_vaddr = dma_zalloc_coherent(nic_io->dev_hdl, PAGE_SIZE,
					      &rq->rx.pi_daddr, GFP_KERNEL);
	if (!rq->rx.pi_vaddr) {
		nic_err(nic_io->dev_hdl, "Fail to allocate rq pi virt addr\n");
		return -ENOMEM;
	}

	ret = sss_create_wq(nic_io->hwdev, &rq->wq, rq_depth,
			    (u16)BIT(SSSNIC_RQ_WQEBB_SHIFT + SSSNIC_NORMAL_RQ_WQE));
	if (ret != 0) {
		nic_err(nic_io->dev_hdl, "Fail to create rq(%u) wq\n", qid);
		dma_free_coherent(nic_io->dev_hdl, PAGE_SIZE, rq->rx.pi_vaddr,
				  rq->rx.pi_daddr);
		return ret;
	}

	return 0;
}

static void sss_nic_destroy_rq(struct sss_nic_io *nic_io,
			       struct sss_nic_io_queue *rq)
{
	dma_free_coherent(nic_io->dev_hdl, PAGE_SIZE, rq->rx.pi_vaddr,
			  rq->rx.pi_daddr);

	sss_destroy_wq(&rq->wq);
}

static int sss_nic_create_qp(struct sss_nic_io *nic_io,
			     struct sss_nic_io_queue *rq, struct sss_nic_io_queue *sq,
			     u32 rq_depth, u32 sq_depth, u16 qid, u16 qp_msix_id)
{
	int ret = 0;

	ret = sss_nic_create_rq(nic_io, rq, qid, rq_depth, qp_msix_id);
	if (ret != 0) {
		nic_err(nic_io->dev_hdl, "Fail to create rq, qid: %u\n", qid);
		return ret;
	}

	ret = sss_nic_create_sq(nic_io, sq, qid, sq_depth, qp_msix_id);
	if (ret != 0) {
		nic_err(nic_io->dev_hdl, "Fail to create sq, qid: %u\n", qid);
		sss_nic_destroy_rq(nic_io, rq);
	}

	return ret;
}

static void sss_nic_destroy_qp(struct sss_nic_io *nic_io,
			       struct sss_nic_io_queue *rq, struct sss_nic_io_queue *sq)
{
	sss_nic_destroy_rq(nic_io, rq);
	sss_nic_destroy_sq(sq);
}

int sss_nic_io_resource_init(struct sss_nic_io *nic_io)
{
	void __iomem *db_base = NULL;
	int ret = 0;

	nic_io->max_qp_num = sss_get_max_sq_num(nic_io->hwdev);

	nic_io->ci_base_vaddr = dma_zalloc_coherent(nic_io->dev_hdl,
						    SSSNIC_CI_TABLE_SIZE(nic_io->max_qp_num,
									 PAGE_SIZE),
				&nic_io->ci_base_daddr, GFP_KERNEL);
	if (!nic_io->ci_base_vaddr) {
		nic_err(nic_io->dev_hdl, "Fail to alloc ci dma buf\n");
		return -ENOMEM;
	}

	ret = sss_alloc_db_addr(nic_io->hwdev, &db_base);
	if (ret != 0) {
		nic_err(nic_io->dev_hdl, "Fail to alloc sq doorbell\n");
		goto out;
	}
	nic_io->sq_db_addr = (u8 *)db_base;

	ret = sss_alloc_db_addr(nic_io->hwdev, &db_base);
	if (ret != 0) {
		nic_err(nic_io->dev_hdl, "Fail to alloc rq doorbell\n");
		sss_free_db_addr(nic_io->hwdev, nic_io->sq_db_addr);
		goto out;
	}
	nic_io->rq_db_addr = (u8 *)db_base;

	return 0;

out:
	dma_free_coherent(nic_io->dev_hdl,
			  SSSNIC_CI_TABLE_SIZE(nic_io->max_qp_num, PAGE_SIZE),
			  nic_io->ci_base_vaddr, nic_io->ci_base_daddr);
	nic_io->ci_base_vaddr = NULL;

	return -ENOMEM;
}

void sss_nic_io_resource_deinit(struct sss_nic_io *nic_io)
{
	dma_free_coherent(nic_io->dev_hdl,
			  SSSNIC_CI_TABLE_SIZE(nic_io->max_qp_num, PAGE_SIZE),
			  nic_io->ci_base_vaddr, nic_io->ci_base_daddr);

	sss_free_db_addr(nic_io->hwdev, nic_io->sq_db_addr);
	sss_free_db_addr(nic_io->hwdev, nic_io->rq_db_addr);
}

int sss_nic_alloc_qp(struct sss_nic_io *nic_io,
		     struct sss_irq_desc *qp_msix_arry, struct sss_nic_qp_info *qp_info)
{
	u16 i;
	u16 qid;
	int ret = 0;
	struct sss_nic_io_queue *rq_group = NULL;
	struct sss_nic_io_queue *sq_group = NULL;

	if (qp_info->qp_num > nic_io->max_qp_num || qp_info->qp_num == 0)
		return -EINVAL;

	rq_group = kcalloc(qp_info->qp_num, sizeof(*rq_group), GFP_KERNEL);
	if (!rq_group)
		return -ENOMEM;

	sq_group = kcalloc(qp_info->qp_num, sizeof(*sq_group), GFP_KERNEL);
	if (!sq_group) {
		ret = -ENOMEM;
		nic_err(nic_io->dev_hdl, "Fail to allocate sq\n");
		goto alloc_sq_err;
	}

	for (qid = 0; qid < qp_info->qp_num; qid++) {
		ret = sss_nic_create_qp(nic_io, &rq_group[qid], &sq_group[qid],
					qp_info->rq_depth, qp_info->sq_depth, qid,
					qp_msix_arry[qid].msix_id);
		if (ret != 0) {
			nic_err(nic_io->dev_hdl,
				"Fail to allocate qp %u, err: %d\n", qid, ret);
			goto create_qp_err;
		}
	}

	qp_info->rq_group = rq_group;
	qp_info->sq_group = sq_group;

	return 0;

create_qp_err:
	for (i = 0; i < qid; i++)
		sss_nic_destroy_qp(nic_io, &rq_group[i], &sq_group[i]);

	kfree(sq_group);

alloc_sq_err:
	kfree(rq_group);

	return ret;
}

void sss_nic_free_qp(struct sss_nic_io *nic_io, struct sss_nic_qp_info *qp_info)
{
	u16 qid;

	for (qid = 0; qid < qp_info->qp_num; qid++)
		sss_nic_destroy_qp(nic_io, &qp_info->rq_group[qid],
				   &qp_info->sq_group[qid]);

	kfree(qp_info->rq_group);
	kfree(qp_info->sq_group);
	qp_info->rq_group = NULL;
	qp_info->sq_group = NULL;
}

static void sss_nic_init_db_info(struct sss_nic_io *nic_io,
				 struct sss_nic_qp_info *qp_info)
{
	u16 qid;
	u16 *ci_addr = NULL;

	for (qid = 0; qid < nic_io->active_qp_num; qid++) {
		qp_info->rq_group[qid].db_addr = nic_io->rq_db_addr;
		qp_info->sq_group[qid].db_addr = nic_io->sq_db_addr;
		qp_info->sq_group[qid].tx.ci_addr =
			SSSNIC_CI_VADDR(nic_io->ci_base_vaddr, qid);
		ci_addr = (u16 *)qp_info->sq_group[qid].tx.ci_addr;
		*ci_addr = 0;
	}
}

int sss_nic_init_qp_info(struct sss_nic_io *nic_io,
			 struct sss_nic_qp_info *qp_info)
{
	nic_io->rq_group = qp_info->rq_group;
	nic_io->sq_group = qp_info->sq_group;
	nic_io->active_qp_num = qp_info->qp_num;

	sss_nic_init_db_info(nic_io, qp_info);

	return sss_nic_init_qp_ctx(nic_io);
}

void sss_nic_deinit_qp_info(struct sss_nic_io *nic_io,
			    struct sss_nic_qp_info *qp_info)
{
	qp_info->qp_num = nic_io->active_qp_num;
	qp_info->rq_group = nic_io->rq_group;
	qp_info->sq_group = nic_io->sq_group;

	sss_nic_deinit_qp_ctx(nic_io->hwdev);
}

static void sss_nic_fill_qp_ctx_ctrlq_header(struct sss_nic_qp_ctx_header *qp_ctx_hdr,
					     enum sss_nic_qp_ctx_type ctx_type,
					     u16 queue_num, u16 qid)
{
	qp_ctx_hdr->rsvd = 0;
	qp_ctx_hdr->start_qid = qid;
	qp_ctx_hdr->q_num = queue_num;
	qp_ctx_hdr->q_type = ctx_type;
	sss_cpu_to_be32(qp_ctx_hdr, sizeof(*qp_ctx_hdr));
}

static void sss_nic_fill_sq_ctx_ctrlq_body(struct sss_nic_io_queue *sq, u16 qid,
					   struct sss_nic_sq_ctx *sq_ctx)
{
	u16 ci_start;
	u16 pi_start;
	u32 lo_wq_block_pfn;
	u32 hi_wq_block_pfn;
	u32 lo_wq_page_pfn;
	u32 hi_wq_page_pfn;
	u64 wq_block_pfn;
	u64 wq_page_addr;
	u64 wq_page_pfn;

	pi_start = sss_nic_get_sq_local_pi(sq);
	ci_start = sss_nic_get_sq_local_ci(sq);

	wq_block_pfn = SSSNIC_WQ_BLOCK_PFN(sq->wq.block_paddr);
	lo_wq_block_pfn = lower_32_bits(wq_block_pfn);
	hi_wq_block_pfn = upper_32_bits(wq_block_pfn);

	wq_page_addr = sss_wq_get_first_wqe_page_addr(&sq->wq);
	wq_page_pfn = SSSNIC_WQ_PAGE_PFN(wq_page_addr);
	lo_wq_page_pfn = lower_32_bits(wq_page_pfn);
	hi_wq_page_pfn = upper_32_bits(wq_page_pfn);

	sq_ctx->rsvd0 = 0;

	sq_ctx->drop_mode_sp =
		SSSNIC_SET_SQ_CTX_MODE(0, SP_FLAG) |
		SSSNIC_SET_SQ_CTX_MODE(0, PKT_DROP);

	sq_ctx->ci_pi =
		SSSNIC_SET_SQ_CTX_CI_PI(ci_start, CI_ID) |
		SSSNIC_SET_SQ_CTX_CI_PI(pi_start, PI_ID);

	sq_ctx->global_sq_id =
		SSSNIC_SET_SQ_CTX_GLOBAL_QUEUE_ID(qid, GLOBAL_SQ_ID);

	sq_ctx->pkt_drop_thd =
		SSSNIC_SET_SQ_CTX_PKT_DROP_THD(SSSNIC_DEAULT_DROP_THD_ON, THD_ON) |
		SSSNIC_SET_SQ_CTX_PKT_DROP_THD(SSSNIC_DEAULT_DROP_THD_OFF, THD_OFF);

	sq_ctx->vlan_ceq_attr =
		SSSNIC_SET_SQ_CTX_VLAN_CEQ(0, CEQ_EN) |
		SSSNIC_SET_SQ_CTX_VLAN_CEQ(1, INSERT_MODE);

	sq_ctx->pref_ci_owner =
		SSSNIC_SET_SQ_CTX_PREF(SSSNIC_CI_HIGN_ID(ci_start), CI_HI) |
		SSSNIC_SET_SQ_CTX_PREF(1, OWNER);

	sq_ctx->pref_cache =
		SSSNIC_SET_SQ_CTX_PREF(SSSNIC_WQ_PREFETCH_MIN, CACHE_MIN) |
		SSSNIC_SET_SQ_CTX_PREF(SSSNIC_WQ_PREFETCH_MAX, CACHE_MAX) |
		SSSNIC_SET_SQ_CTX_PREF(SSSNIC_WQ_PREFETCH_THRESHOLD, CACHE_THRESHOLD);

	sq_ctx->lo_pref_wq_pfn = lo_wq_page_pfn;

	sq_ctx->hi_pref_wq_pfn_ci =
		SSSNIC_SET_SQ_CTX_PREF(ci_start, CI_LOW) |
		SSSNIC_SET_SQ_CTX_PREF(hi_wq_page_pfn, WQ_PFN_HI);

	sq_ctx->lo_wq_pfn = lo_wq_page_pfn;

	sq_ctx->hi_wq_pfn =
		SSSNIC_SET_SQ_CTX_WQ_PAGE(hi_wq_page_pfn, HI_PFN) |
		SSSNIC_SET_SQ_CTX_WQ_PAGE(1, OWNER);

	sq_ctx->lo_wq_block_pfn = lo_wq_block_pfn;

	sq_ctx->hi_wq_block_pfn =
		SSSNIC_SET_SQ_CTX_WQ_BLOCK(hi_wq_block_pfn, PFN_HI);

	sss_cpu_to_be32(sq_ctx, sizeof(*sq_ctx));
}

static void sss_nic_fill_rq_ctx_ctrlq_body(struct sss_nic_io_queue *rq,
					   struct sss_nic_rq_ctx *rq_ctx)
{
	u16 wqe_type = rq->wqe_type;
	u16 ci_start = (u16)((u32)sss_nic_get_rq_local_ci(rq) << wqe_type);
	u16 pi_start = (u16)((u32)sss_nic_get_rq_local_pi(rq) << wqe_type);
	u64 wq_page_addr = sss_wq_get_first_wqe_page_addr(&rq->wq);
	u64 wq_page_pfn = SSSNIC_WQ_PAGE_PFN(wq_page_addr);
	u64 wq_block_pfn = SSSNIC_WQ_BLOCK_PFN(rq->wq.block_paddr);
	u32 lo_wq_page_pfn = lower_32_bits(wq_page_pfn);
	u32 hi_wq_page_pfn = upper_32_bits(wq_page_pfn);
	u32 lo_wq_block_pfn = lower_32_bits(wq_block_pfn);
	u32 hi_wq_block_pfn = upper_32_bits(wq_block_pfn);

	rq_ctx->ceq_attr = SSSNIC_SET_RQ_CTX_CEQ_ATTR(0, EN) |
			   SSSNIC_SET_RQ_CTX_CEQ_ATTR(rq->msix_id, INTR);

	rq_ctx->ci_pi =
		SSSNIC_SET_RQ_CTX_CI_PI(ci_start, CI_ID) |
		SSSNIC_SET_RQ_CTX_CI_PI(pi_start, PI_ID);

	rq_ctx->pref_cache =
		SSSNIC_SET_RQ_CTX_PREF(SSSNIC_WQ_PREFETCH_MIN, CACHE_MIN) |
		SSSNIC_SET_RQ_CTX_PREF(SSSNIC_WQ_PREFETCH_MAX, CACHE_MAX) |
		SSSNIC_SET_RQ_CTX_PREF(SSSNIC_WQ_PREFETCH_THRESHOLD, CACHE_THRESHOLD);

	rq_ctx->pref_ci_owner =
		SSSNIC_SET_RQ_CTX_PREF(SSSNIC_CI_HIGN_ID(ci_start), CI_HI) |
		SSSNIC_SET_RQ_CTX_PREF(1, OWNER);

	rq_ctx->lo_wq_pfn = lo_wq_page_pfn;

	rq_ctx->hi_wq_pfn =
		SSSNIC_SET_RQ_CTX_WQ_PAGE(hi_wq_page_pfn, HI_PFN) |
		SSSNIC_SET_RQ_CTX_WQ_PAGE(1, OWNER);

	if (wqe_type == SSSNIC_EXTEND_RQ_WQE) {
		rq_ctx->hi_wq_pfn |=
			SSSNIC_SET_RQ_CTX_WQ_PAGE(0, WQE_TYPE);
	} else if (wqe_type == SSSNIC_NORMAL_RQ_WQE) {
		rq_ctx->cqe_sge_len = SSSNIC_SET_RQ_CTX_CQE_LEN(1, CQE_LEN);
		rq_ctx->hi_wq_pfn |=
			SSSNIC_SET_RQ_CTX_WQ_PAGE(2, WQE_TYPE);
	} else {
		pr_err("Invalid rq wqe type: %u", wqe_type);
	}

	rq_ctx->lo_pref_wq_pfn = lo_wq_page_pfn;
	rq_ctx->hi_pref_wq_pfn_ci =
		SSSNIC_SET_RQ_CTX_PREF(hi_wq_page_pfn, WQ_PFN_HI) |
		SSSNIC_SET_RQ_CTX_PREF(ci_start, CI_LOW);

	rq_ctx->lo_wq_block_pfn = lo_wq_block_pfn;
	rq_ctx->hi_wq_block_pfn =
		SSSNIC_SET_RQ_CTX_WQ_BLOCK(hi_wq_block_pfn, PFN_HI);

	rq_ctx->pi_paddr_lo = lower_32_bits(rq->rx.pi_daddr);
	rq_ctx->pi_paddr_hi = upper_32_bits(rq->rx.pi_daddr);

	sss_cpu_to_be32(rq_ctx, sizeof(*rq_ctx));
}

static int sss_nic_send_sq_ctx_by_ctrlq(struct sss_nic_io *nic_io,
					struct sss_ctrl_msg_buf *msg_buf, u16 qid)
{
	u16 i;
	u16 max_qp;
	u64 out_param = 0;
	int ret;
	struct sss_nic_sq_ctx_block *sq_ctx_block = msg_buf->buf;

	max_qp = min(nic_io->active_qp_num - qid, SSSNIC_Q_CTXT_MAX);
	sss_nic_fill_qp_ctx_ctrlq_header(&sq_ctx_block->ctrlq_hdr,
					 SSSNIC_QP_CTX_TYPE_SQ, max_qp, qid);

	for (i = 0; i < max_qp; i++)
		sss_nic_fill_sq_ctx_ctrlq_body(&nic_io->sq_group[qid + i], qid + i,
					       &sq_ctx_block->sq_ctxt[i]);

	msg_buf->size = SSSNIC_SQ_CTX_SIZE(max_qp);

	ret = sss_ctrlq_direct_reply(nic_io->hwdev, SSS_MOD_TYPE_L2NIC,
				     SSSNIC_CTRLQ_OPCODE_MODIFY_QUEUE_CTX,
				     msg_buf, &out_param, 0, SSS_CHANNEL_NIC);
	if (ret != 0 || out_param != 0) {
		nic_err(nic_io->dev_hdl,
			"Fail to set sq ctxt, ret: %d, out_param: 0x%llx\n",
			ret, out_param);

		return -EFAULT;
	}

	return 0;
}

static int sss_nic_send_sq_ctx_to_hw(struct sss_nic_io *nic_io)
{
	int ret = 0;
	u16 qid = 0;
	u16 max_qp;
	struct sss_ctrl_msg_buf *msg_buf = NULL;

	msg_buf = sss_alloc_ctrlq_msg_buf(nic_io->hwdev);
	if (!msg_buf) {
		nic_err(nic_io->dev_hdl, "Fail to allocate cmd buf\n");
		return -ENOMEM;
	}

	while (qid < nic_io->active_qp_num) {
		max_qp = min(nic_io->active_qp_num - qid, SSSNIC_Q_CTXT_MAX);
		ret = sss_nic_send_sq_ctx_by_ctrlq(nic_io, msg_buf, qid);
		if (ret) {
			nic_err(nic_io->dev_hdl,
				"Fail to set sq ctx, qid: %u\n", qid);
			break;
		}

		qid += max_qp;
	}

	sss_free_ctrlq_msg_buf(nic_io->hwdev, msg_buf);

	return ret;
}

static int sss_nic_send_rq_ctx_by_ctrlq(struct sss_nic_io *nic_io,
					struct sss_ctrl_msg_buf *msg_buf, u16 qid)
{
	u16 i;
	u16 max_qp;
	u64 out_param = 0;
	int ret;
	struct sss_nic_rq_ctx_block *rq_ctx_block = msg_buf->buf;

	rq_ctx_block = msg_buf->buf;
	max_qp = min(nic_io->active_qp_num - qid, SSSNIC_Q_CTXT_MAX);

	sss_nic_fill_qp_ctx_ctrlq_header(&rq_ctx_block->ctrlq_hdr,
					 SSSNIC_QP_CTX_TYPE_RQ, max_qp, qid);

	for (i = 0; i < max_qp; i++)
		sss_nic_fill_rq_ctx_ctrlq_body(&nic_io->rq_group[qid + i],
					       &rq_ctx_block->rq_ctxt[i]);

	msg_buf->size = SSSNIC_RQ_CTX_SIZE(max_qp);

	ret = sss_ctrlq_direct_reply(nic_io->hwdev, SSS_MOD_TYPE_L2NIC,
				     SSSNIC_CTRLQ_OPCODE_MODIFY_QUEUE_CTX,
				     msg_buf, &out_param, 0, SSS_CHANNEL_NIC);
	if (ret != 0 || out_param != 0) {
		nic_err(nic_io->dev_hdl,
			"Fail to set rq ctx, ret: %d, out_param: 0x%llx\n",
			ret, out_param);

		return -EFAULT;
	}

	return 0;
}

static int sss_nic_send_rq_ctx_to_hw(struct sss_nic_io *nic_io)
{
	int ret = 0;
	u16 qid = 0;
	u16 max_qp;
	struct sss_ctrl_msg_buf *msg_buf = NULL;

	msg_buf = sss_alloc_ctrlq_msg_buf(nic_io->hwdev);
	if (!msg_buf) {
		nic_err(nic_io->dev_hdl, "Fail to allocate cmd buf\n");
		return -ENOMEM;
	}

	while (qid < nic_io->active_qp_num) {
		max_qp = min(nic_io->active_qp_num - qid, SSSNIC_Q_CTXT_MAX);

		ret = sss_nic_send_rq_ctx_by_ctrlq(nic_io, msg_buf, qid);
		if (ret) {
			nic_err(nic_io->dev_hdl,
				"Fail to set rq ctx, qid: %u\n", qid);
			break;
		}

		qid += max_qp;
	}

	sss_free_ctrlq_msg_buf(nic_io->hwdev, msg_buf);

	return ret;
}

static int sss_nic_reset_hw_offload_ctx(struct sss_nic_io *nic_io,
					enum sss_nic_qp_ctx_type ctx_type)
{
	int ret = 0;
	u64 out_param = 0;
	struct sss_ctrl_msg_buf *msg_buf = NULL;
	struct sss_nic_clear_q_ctx *ctx_block = NULL;

	msg_buf = sss_alloc_ctrlq_msg_buf(nic_io->hwdev);
	if (!msg_buf) {
		nic_err(nic_io->dev_hdl, "Fail to allocate cmd buf\n");
		return -ENOMEM;
	}

	ctx_block = msg_buf->buf;
	ctx_block->ctrlq_hdr.start_qid = 0;
	ctx_block->ctrlq_hdr.q_type = ctx_type;
	ctx_block->ctrlq_hdr.q_num = nic_io->max_qp_num;

	sss_cpu_to_be32(ctx_block, sizeof(*ctx_block));

	msg_buf->size = sizeof(*ctx_block);

	ret = sss_ctrlq_direct_reply(nic_io->hwdev, SSS_MOD_TYPE_L2NIC,
				     SSSNIC_CTRLQ_OPCODE_CLEAN_QUEUE_CONTEXT,
				     msg_buf, &out_param, 0, SSS_CHANNEL_NIC);
	if (ret != 0 || out_param != 0) {
		nic_err(nic_io->dev_hdl,
			"Fail to clean queue offload ctxt, ret: %d, out_param: 0x%llx\n",
			ret, out_param);

		ret = -EFAULT;
	}

	sss_free_ctrlq_msg_buf(nic_io->hwdev, msg_buf);

	return ret;
}

static int sss_nic_reset_hw_qp_offload_ctx(struct sss_nic_io *nic_io)
{
	int ret;

	ret = sss_nic_reset_hw_offload_ctx(nic_io, SSSNIC_QP_CTX_TYPE_SQ);
	if (ret != 0)
		return ret;

	ret = sss_nic_reset_hw_offload_ctx(nic_io, SSSNIC_QP_CTX_TYPE_RQ);

	return ret;
}

static int sss_nic_set_hw_intr_attr(struct sss_nic_io *nic_io, u16 qid)
{
	struct sss_nic_mbx_intr_attr cmd_ci_attr = {0};
	u16 out_len = sizeof(cmd_ci_attr);
	int ret;

	cmd_ci_attr.func_id = sss_get_global_func_id(nic_io->hwdev);
	cmd_ci_attr.dma_attr_off  = 0;
	cmd_ci_attr.pending_limit = SSSNIC_DEAULT_TX_CI_PENDING_LIMIT;
	cmd_ci_attr.coalescing_time  = SSSNIC_DEAULT_TX_CI_COALESCING_TIME;
	cmd_ci_attr.intr_en = 1;
	cmd_ci_attr.intr_id = nic_io->sq_group[qid].msix_id;
	cmd_ci_attr.l2nic_sqn = qid;
	cmd_ci_attr.ci_addr = SSSNIC_CI_PADDR(nic_io->ci_base_daddr, qid) >> 0x2;

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_io->hwdev, SSSNIC_MBX_OPCODE_SQ_CI_ATTR_SET,
					     &cmd_ci_attr, sizeof(cmd_ci_attr), &cmd_ci_attr,
					     &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_ci_attr)) {
		nic_err(nic_io->dev_hdl,
			"Fail to set ci attr table, ret: %d, status: 0x%x, out_len: 0x%x\n",
			ret, cmd_ci_attr.head.state, out_len);
		return -EFAULT;
	}

	return 0;
}

static int sss_nic_set_qp_intr_attr(struct sss_nic_io *nic_io)
{
	u16 qid;
	int ret;

	for (qid = 0; qid < nic_io->active_qp_num; qid++) {
		ret = sss_nic_set_hw_intr_attr(nic_io, qid);
		if (ret != 0) {
			nic_err(nic_io->dev_hdl, "Fail to set ci table, qid:%u\n", qid);
			return ret;
		}
	}

	return 0;
}

int sss_nic_init_qp_ctx(struct sss_nic_io *nic_io)
{
	u32 rq_depth;
	int ret;

	ret = sss_nic_send_sq_ctx_to_hw(nic_io);
	if (ret != 0) {
		nic_err(nic_io->dev_hdl, "Fail to send sq ctx to hw\n");
		return ret;
	}

	ret = sss_nic_send_rq_ctx_to_hw(nic_io);
	if (ret != 0) {
		nic_err(nic_io->dev_hdl, "Fail to send rq ctx to hw\n");
		return ret;
	}

	ret = sss_nic_reset_hw_qp_offload_ctx(nic_io);
	if (ret != 0) {
		nic_err(nic_io->dev_hdl, "Fail to reset qp offload ctx\n");
		return ret;
	}

	rq_depth = nic_io->rq_group[0].wq.q_depth << nic_io->rq_group[0].wqe_type;
	ret = sss_chip_set_root_ctx(nic_io->hwdev, rq_depth, nic_io->sq_group[0].wq.q_depth,
				    nic_io->rx_buff_len, SSS_CHANNEL_NIC);
	if (ret != 0) {
		nic_err(nic_io->dev_hdl, "Fail to set root context\n");
		return ret;
	}

	ret = sss_nic_set_qp_intr_attr(nic_io);
	if (ret != 0) {
		sss_chip_clean_root_ctx(nic_io->hwdev, SSS_CHANNEL_NIC);
		nic_err(nic_io->dev_hdl, "Fail to set ci table\n");
	}

	return ret;
}

void sss_nic_deinit_qp_ctx(void *hwdev)
{
	if (!hwdev)
		return;
	sss_chip_clean_root_ctx(hwdev, SSS_CHANNEL_NIC);
}
EXPORT_SYMBOL_GPL(sss_nic_deinit_qp_ctx);
