/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_WQ_H
#define HINIC3_WQ_H

struct hinic3_wq {
	u16		cons_idx;
	u16		prod_idx;

	u32		q_depth;
	u16		idx_mask;
	u16		wqebb_size_shift;
	u16		rsvd1;
	u16		num_wq_pages;
	u32		wqebbs_per_page;
	u16		wqebbs_per_page_shift;
	u16		wqebbs_per_page_mask;

	struct hinic3_dma_addr_align *wq_pages;

	dma_addr_t	wq_block_paddr;
	u64		*wq_block_vaddr;

	void		*dev_hdl;
	u32		wq_page_size;
	u16		wqebb_size;
} ____cacheline_aligned;

#define WQ_MASK_IDX(wq, idx)		((idx) & (wq)->idx_mask)
#define WQ_MASK_PAGE(wq, pg_idx)	\
		(((pg_idx) < ((wq)->num_wq_pages)) ? (pg_idx) : 0)
#define WQ_PAGE_IDX(wq, idx)		((idx) >> (wq)->wqebbs_per_page_shift)
#define WQ_OFFSET_IN_PAGE(wq, idx)	((idx) & (wq)->wqebbs_per_page_mask)
#define WQ_GET_WQEBB_ADDR(wq, pg_idx, idx_in_pg)		\
		((u8 *)(wq)->wq_pages[pg_idx].align_vaddr +	\
		 ((idx_in_pg) << (wq)->wqebb_size_shift))
#define WQ_IS_0_LEVEL_CLA(wq)	((wq)->num_wq_pages == 1)

#ifdef static
#undef static
#define LLT_STATIC_DEF_SAVED
#endif

static inline u16 hinic3_wq_free_wqebbs(struct hinic3_wq *wq)
{
	return wq->q_depth - ((wq->q_depth + wq->prod_idx - wq->cons_idx) &
			      wq->idx_mask) - 1;
}

static inline bool hinic3_wq_is_empty(struct hinic3_wq *wq)
{
	return WQ_MASK_IDX(wq, wq->prod_idx) == WQ_MASK_IDX(wq, wq->cons_idx);
}

static inline void *hinic3_wq_get_one_wqebb(struct hinic3_wq *wq, u16 *pi)
{
	*pi = WQ_MASK_IDX(wq, wq->prod_idx);
	wq->prod_idx++;

	return WQ_GET_WQEBB_ADDR(wq, WQ_PAGE_IDX(wq, *pi),
				 WQ_OFFSET_IN_PAGE(wq, *pi));
}

static inline void *hinic3_wq_get_multi_wqebbs(struct hinic3_wq *wq,
					       u16 num_wqebbs, u16 *prod_idx,
					       void **second_part_wqebbs_addr,
					       u16 *first_part_wqebbs_num)
{
	u32 pg_idx, off_in_page;

	*prod_idx = WQ_MASK_IDX(wq, wq->prod_idx);
	wq->prod_idx += num_wqebbs;

	pg_idx = WQ_PAGE_IDX(wq, *prod_idx);
	off_in_page = WQ_OFFSET_IN_PAGE(wq, *prod_idx);

	if ((off_in_page + num_wqebbs) > wq->wqebbs_per_page) {
		/* wqe across wq page boundary */
		*second_part_wqebbs_addr =
			WQ_GET_WQEBB_ADDR(wq, WQ_MASK_PAGE(wq, pg_idx + 1), 0);
		*first_part_wqebbs_num = wq->wqebbs_per_page - off_in_page;
	} else {
		*second_part_wqebbs_addr = NULL;
		*first_part_wqebbs_num = num_wqebbs;
	}

	return WQ_GET_WQEBB_ADDR(wq, pg_idx, off_in_page);
}

static inline void hinic3_wq_put_wqebbs(struct hinic3_wq *wq, u16 num_wqebbs)
{
	wq->cons_idx += num_wqebbs;
}

static inline void *hinic3_wq_wqebb_addr(struct hinic3_wq *wq, u16 idx)
{
	return WQ_GET_WQEBB_ADDR(wq, WQ_PAGE_IDX(wq, idx),
				 WQ_OFFSET_IN_PAGE(wq, idx));
}

static inline void *hinic3_wq_read_one_wqebb(struct hinic3_wq *wq,
					     u16 *cons_idx)
{
	*cons_idx = WQ_MASK_IDX(wq, wq->cons_idx);

	return hinic3_wq_wqebb_addr(wq, *cons_idx);
}

static inline u64 hinic3_wq_get_first_wqe_page_addr(struct hinic3_wq *wq)
{
	return wq->wq_pages[0].align_paddr;
}

static inline void hinic3_wq_reset(struct hinic3_wq *wq)
{
	u16 pg_idx;

	wq->cons_idx = 0;
	wq->prod_idx = 0;

	for (pg_idx = 0; pg_idx < wq->num_wq_pages; pg_idx++)
		memset(wq->wq_pages[pg_idx].align_vaddr, 0, wq->wq_page_size);
}

int hinic3_wq_create(void *hwdev, struct hinic3_wq *wq, u32 q_depth,
		     u16 wqebb_size);
void hinic3_wq_destroy(struct hinic3_wq *wq);

#endif
