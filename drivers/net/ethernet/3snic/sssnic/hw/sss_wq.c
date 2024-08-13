// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include "sss_kernel.h"
#include "sss_common.h"
#include "sss_hwdev.h"
#include "sss_hw_wq.h"

#define SSS_WQ_MIN_DEPTH		64
#define SSS_WQ_MAX_DEPTH		65536
#define SSS_WQ_MAX_PAGE_NUM	(PAGE_SIZE / sizeof(u64))

static int sss_init_wq_block(struct sss_wq *wq)
{
	int i;

	if (SSS_WQ_IS_0_LEVEL_CLA(wq)) {
		wq->block_paddr = wq->page[0].align_paddr;
		wq->block_vaddr = wq->page[0].align_vaddr;
		return 0;
	}

	if (wq->page_num > SSS_WQ_MAX_PAGE_NUM) {
		sdk_err(wq->dev_hdl, "Wq page num: 0x%x out of range: %lu\n",
			wq->page_num, SSS_WQ_MAX_PAGE_NUM);
		return -EFAULT;
	}

	wq->block_vaddr = dma_zalloc_coherent(wq->dev_hdl, PAGE_SIZE,
					      &wq->block_paddr, GFP_KERNEL);
	if (!wq->block_vaddr) {
		sdk_err(wq->dev_hdl, "Fail to alloc wq block vaddr\n");
		return -ENOMEM;
	}

	for (i = 0; i < wq->page_num; i++)
		wq->block_vaddr[i] = cpu_to_be64(wq->page[i].align_paddr);

	return 0;
}

static void sss_deinit_wq_block(struct sss_wq *wq)
{
	if (!SSS_WQ_IS_0_LEVEL_CLA(wq))
		dma_free_coherent(wq->dev_hdl, PAGE_SIZE, wq->block_vaddr,
				  wq->block_paddr);
}

static int sss_alloc_wq_page(struct sss_wq *wq)
{
	int i;
	int ret;
	int id;

	wq->page = kcalloc(wq->page_num, sizeof(*wq->page), GFP_KERNEL);
	if (!wq->page)
		return -ENOMEM;

	for (id = 0; id < wq->page_num; id++) {
		ret = sss_dma_zalloc_coherent_align(wq->dev_hdl, wq->page_size,
						    wq->page_size, GFP_KERNEL, &wq->page[id]);
		if (ret != 0) {
			sdk_err(wq->dev_hdl, "Fail to alloc wq dma page\n");
			goto dma_page_err;
		}
	}

	ret = sss_init_wq_block(wq);
	if (ret != 0)
		goto block_err;

	return 0;

block_err:
dma_page_err:
	for (i = 0; i < id; i++)
		sss_dma_free_coherent_align(wq->dev_hdl, &wq->page[i]);

	kfree(wq->page);
	wq->page = NULL;

	return -ENOMEM;
}

static void sss_free_wq_page(struct sss_wq *wq)
{
	int i;

	sss_deinit_wq_block(wq);

	for (i = 0; i < wq->page_num; i++)
		sss_dma_free_coherent_align(wq->dev_hdl, &wq->page[i]);

	kfree(wq->page);
	wq->page = NULL;
}

static void sss_init_wq_param(struct sss_hwdev *hwdev, struct sss_wq *wq,
			      u32 q_depth, u16 block_size)
{
	u32 page_size = ALIGN(hwdev->wq_page_size, PAGE_SIZE);

	wq->ci = 0;
	wq->pi = 0;
	wq->dev_hdl = hwdev->dev_hdl;
	wq->q_depth = q_depth;
	wq->id_mask = (u16)(q_depth - 1);
	wq->elem_size = block_size;
	wq->elem_size_shift = (u16)ilog2(wq->elem_size);
	wq->page_size = page_size;
	wq->elem_per_page = min(page_size / block_size, q_depth);
	wq->elem_per_page_shift = (u16)ilog2(wq->elem_per_page);
	wq->elem_per_page_mask = (u16)(wq->elem_per_page - 1);
	wq->page_num =
		(u16)(ALIGN(((u32)q_depth * block_size), page_size) / page_size);
}

int sss_create_wq(void *hwdev, struct sss_wq *wq, u32 q_depth, u16 block_size)
{
	if (!wq || !hwdev) {
		pr_err("Invalid wq or dev_hdl\n");
		return -EINVAL;
	}

	if (q_depth < SSS_WQ_MIN_DEPTH || q_depth > SSS_WQ_MAX_DEPTH ||
	    (q_depth & (q_depth - 1)) != 0) {
		sdk_err(SSS_TO_DEV(hwdev), "Invalid q_depth(%u)\n", q_depth);
		return -EINVAL;
	}

	if (block_size == 0 || (block_size & (block_size - 1)) != 0) {
		sdk_err(SSS_TO_DEV(hwdev), "Invalid block_size(%u)\n", block_size);
		return -EINVAL;
	}

	sss_init_wq_param(hwdev, wq, q_depth, block_size);

	return sss_alloc_wq_page(wq);
}
EXPORT_SYMBOL(sss_create_wq);

void sss_destroy_wq(struct sss_wq *wq)
{
	if (!wq)
		return;

	sss_free_wq_page(wq);
}
EXPORT_SYMBOL(sss_destroy_wq);
