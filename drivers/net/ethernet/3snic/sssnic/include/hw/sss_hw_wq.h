/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HW_WQ_H
#define SSS_HW_WQ_H
#include <linux/types.h>

#include "sss_hw_common.h"

struct sss_wq {
	u16		ci;
	u16		pi;

	u32		q_depth;
	u16		id_mask;
	u16		elem_size_shift;
	u16		page_num;
	u16		rsvd1;
	u32		elem_per_page;
	u16		elem_per_page_shift;
	u16		elem_per_page_mask;

	struct sss_dma_addr_align *page;

	dma_addr_t	block_paddr;
	u64		*block_vaddr;

	void		*dev_hdl;
	u32		page_size;
	u16		elem_size;
	u16		rsvd2;
} ____cacheline_aligned;

#define SSS_WQ_MASK_ID(wq, id)		((id) & (wq)->id_mask)
#define SSS_WQ_MASK_PAGE(wq, pg_id)	\
		((pg_id) < (wq)->page_num ? (pg_id) : 0)
#define SSS_WQ_PAGE_ID(wq, id)		((id) >> (wq)->elem_per_page_shift)
#define SSS_WQ_OFFSET_IN_PAGE(wq, id)	((id) & (wq)->elem_per_page_mask)
#define SSS_WQ_GET_WQEBB_ADDR(wq, pg_id, id_in_pg)		\
		((u8 *)(wq)->page[pg_id].align_vaddr +	\
		((id_in_pg) << (wq)->elem_size_shift))
#define SSS_WQ_IS_0_LEVEL_CLA(wq)	((wq)->page_num == 1)

static inline u16 sss_wq_free_wqebb(struct sss_wq *wq)
{
	return wq->q_depth - ((wq->q_depth + wq->pi - wq->ci) & wq->id_mask) - 1;
}

static inline bool sss_wq_is_empty(struct sss_wq *wq)
{
	return SSS_WQ_MASK_ID(wq, wq->pi) == SSS_WQ_MASK_ID(wq, wq->ci);
}

static inline void *sss_wq_get_one_wqebb(struct sss_wq *wq, u16 *pi)
{
	*pi = SSS_WQ_MASK_ID(wq, wq->pi);
	wq->pi++;

	return SSS_WQ_GET_WQEBB_ADDR(wq, SSS_WQ_PAGE_ID(wq, *pi),
				     SSS_WQ_OFFSET_IN_PAGE(wq, *pi));
}

static inline void *sss_wq_get_multi_wqebb(struct sss_wq *wq,
					   u16 num_wqebbs, u16 *pi,
		void **second_part_wqebbs_addr,
		u16 *first_part_wqebbs_num)
{
	u32 pg_id;
	u32 off_in_page;

	*pi = SSS_WQ_MASK_ID(wq, wq->pi);
	wq->pi += num_wqebbs;

	pg_id = SSS_WQ_PAGE_ID(wq, *pi);
	off_in_page = SSS_WQ_OFFSET_IN_PAGE(wq, *pi);

	if (off_in_page + num_wqebbs > wq->elem_per_page) {
		/* wqe across wq page boundary */
		*second_part_wqebbs_addr =
			SSS_WQ_GET_WQEBB_ADDR(wq, SSS_WQ_MASK_PAGE(wq, pg_id + 1), 0);
		*first_part_wqebbs_num = wq->elem_per_page - off_in_page;
	} else {
		*second_part_wqebbs_addr = NULL;
		*first_part_wqebbs_num = num_wqebbs;
	}

	return SSS_WQ_GET_WQEBB_ADDR(wq, pg_id, off_in_page);
}

static inline void sss_update_wq_ci(struct sss_wq *wq, u16 num_wqebbs)
{
	wq->ci += num_wqebbs;
}

static inline void *sss_wq_wqebb_addr(struct sss_wq *wq, u16 id)
{
	return SSS_WQ_GET_WQEBB_ADDR(wq, SSS_WQ_PAGE_ID(wq, id),
				     SSS_WQ_OFFSET_IN_PAGE(wq, id));
}

static inline void *sss_wq_read_one_wqebb(struct sss_wq *wq, u16 *ci)
{
	*ci = SSS_WQ_MASK_ID(wq, wq->ci);

	return sss_wq_wqebb_addr(wq, *ci);
}

static inline u64 sss_wq_get_first_wqe_page_addr(struct sss_wq *wq)
{
	return wq->page[0].align_paddr;
}

static inline void sss_wq_reset(struct sss_wq *wq)
{
	u16 pg_id;

	wq->ci = 0;
	wq->pi = 0;

	for (pg_id = 0; pg_id < wq->page_num; pg_id++)
		memset(wq->page[pg_id].align_vaddr, 0, wq->page_size);
}

int sss_create_wq(void *hwdev, struct sss_wq *wq, u32 q_depth, u16 block_size);
void sss_destroy_wq(struct sss_wq *wq);
#endif
