// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#include "osdep.h"
#include "status.h"
#include "hmc.h"
#include "defs.h"
#include "type.h"
#include "protos.h"
#include "vf.h"
#include "virtchnl.h"
#include "pble.h"
#include "main.h"

static int add_pble_prm(struct zxdh_hmc_pble_rsrc *pble_rsrc);

/**
 * zxdh_destroy_pble_prm - destroy prm during module unload
 * @pble_rsrc: pble resources
 */
void zxdh_destroy_pble_prm(struct zxdh_hmc_pble_rsrc *pble_rsrc)
{
	struct zxdh_chunk *chunk;
	struct zxdh_pble_prm *pinfo = &pble_rsrc->pinfo;

	while (!list_empty(&pinfo->clist)) {
		chunk = (struct zxdh_chunk *)pinfo->clist.next;
		list_del(&chunk->list);
		if (chunk->type == PBLE_SD_PAGED)
			zxdh_pble_free_paged_mem(chunk);
		if (chunk->bitmapbuf)
			kfree(chunk->bitmapmem.va);
		kfree(chunk->chunkmem.va);
	}
}

/**
 * zxdh_hmc_init_pble - Initialize pble resources during module load
 * @dev: zxdh_sc_dev struct
 * @pble_rsrc: pble resources
 * @mr: Queue or Memory area
 */
int zxdh_hmc_init_pble(struct zxdh_sc_dev *dev,
		       struct zxdh_hmc_pble_rsrc *pble_rsrc, int mr)
{
	struct zxdh_hmc_info *hmc_info;
	int status = 0;

	hmc_info = dev->hmc_info;
	pble_rsrc->dev = dev;
	pble_rsrc->pble_copy = true;
	pble_rsrc->pble_type = mr;

	if (mr == PBLE_QUEUE) {
		pble_rsrc->unallocated_pble =
			hmc_info->hmc_obj[ZXDH_HMC_IW_PBLE].cnt;
	} else {
		pble_rsrc->unallocated_pble =
			hmc_info->hmc_obj[ZXDH_HMC_IW_PBLE_MR].cnt;
	}

	pble_rsrc->next_fpm_addr = pble_rsrc->fpm_base_addr;

	pble_rsrc->pinfo.pble_shift = PBLE_SHIFT;

	mutex_init(&pble_rsrc->pble_mutex_lock);

	spin_lock_init(&pble_rsrc->pinfo.prm_lock);
	INIT_LIST_HEAD(&pble_rsrc->pinfo.clist);
	if (add_pble_prm(pble_rsrc)) {
		zxdh_destroy_pble_prm(pble_rsrc);
		status = -ENOMEM;
	}

	return status;
}

/**
 * add_sd_direct - add sd direct for pble
 * @pble_rsrc: pble resource ptr
 * @info: page info for sd
 */
static int add_sd_direct(struct zxdh_hmc_pble_rsrc *pble_rsrc,
			 struct zxdh_add_page_info *info)
{
	int ret_code = 0;
	struct sd_pd_idx *idx = &info->idx;
	struct zxdh_chunk *chunk = info->chunk;
	struct zxdh_hmc_info *hmc_info = info->hmc_info;
	struct zxdh_hmc_sd_entry *sd_entry = info->sd_entry;
	u32 offset = 0;
	struct zxdh_pci_f *rf = (struct zxdh_pci_f *)container_of(
		pble_rsrc->dev, struct zxdh_pci_f, sc_dev);

	if (rf->ftype == 1) {
		if (pble_rsrc->pble_type == PBLE_QUEUE) {
			if (!sd_entry->valid &&
			    (hmc_info->pble_hmc_index <
			     hmc_info->hmc_first_entry_pble_mr)) {
				ret_code = zxdh_vf_add_pble_hmc_obj(
					pble_rsrc->dev, hmc_info, pble_rsrc,
					info->pages);
			}
		} else {
			if (!sd_entry->valid &&
			    (hmc_info->pble_mr_hmc_index <
			     hmc_info->hmc_entry_total + 1)) {
				ret_code = zxdh_vf_add_pble_hmc_obj(
					pble_rsrc->dev, hmc_info, pble_rsrc,
					info->pages);
			}
		}
	} else {
		if (pble_rsrc->pble_type == PBLE_QUEUE) {
			if (!sd_entry->valid &&
			    (hmc_info->pble_hmc_index <
			     hmc_info->hmc_first_entry_pble_mr)) {
				ret_code = zxdh_add_pble_hmc_obj(
					hmc_info, pble_rsrc, info->pages);
			}
		} else {
			if (!sd_entry->valid &&
			    (hmc_info->pble_mr_hmc_index <
			     hmc_info->hmc_entry_total + 1)) {
				ret_code = zxdh_add_pble_hmc_obj(
					hmc_info, pble_rsrc, info->pages);
			}
		}
	}

	if (ret_code)
		return ret_code;

	chunk->type = PBLE_SD_CONTIGOUS;

	offset = idx->rel_pd_idx << HMC_PAGED_BP_SHIFT;
	chunk->size = info->pages << HMC_PAGED_BP_SHIFT;

	chunk->vaddr = sd_entry->u.bp.addr.va + offset;
	chunk->pa = sd_entry->u.bp.addr.pa + offset; //
	chunk->fpm_addr = pble_rsrc->next_fpm_addr;

	return 0;
}

/**
 * fpm_to_idx - given fpm address, get pble index
 * @pble_rsrc: pble resource management
 * @addr: fpm address for index
 */
static u32 fpm_to_idx(struct zxdh_hmc_pble_rsrc *pble_rsrc, u64 addr)
{
	u64 idx;

	idx = (addr - (pble_rsrc->fpm_base_addr)) >> 3;

	return (u32)idx;
}

/**
 * add_pble_prm - add a sd entry for pble resoure
 * @pble_rsrc: pble resource management
 */
static int add_pble_prm(struct zxdh_hmc_pble_rsrc *pble_rsrc)
{
	struct zxdh_sc_dev *dev = pble_rsrc->dev;
	struct zxdh_hmc_sd_entry *sd_entry;
	struct zxdh_hmc_info *hmc_info;
	struct zxdh_chunk *chunk;
	struct zxdh_add_page_info info;
	struct sd_pd_idx *idx = &info.idx;
	int ret_code = 0;
	struct zxdh_virt_mem chunkmem;
	u32 pages;

	if (pble_rsrc->unallocated_pble < PBLE_PER_PAGE)
		return -ENOMEM;

	chunkmem.size = sizeof(*chunk);
	chunkmem.va = kzalloc(chunkmem.size, GFP_KERNEL);
	if (!chunkmem.va)
		return -ENOMEM;

	chunk = chunkmem.va;
	chunk->chunkmem = chunkmem;
	hmc_info = dev->hmc_info;
	chunk->dev = dev;
	chunk->fpm_addr = pble_rsrc->next_fpm_addr;

	if (pble_rsrc->pble_type == PBLE_QUEUE) {
		sd_entry =
			&hmc_info->sd_table
				 .sd_entry[hmc_info->pble_hmc_index]; /* code */
	} else {
		sd_entry = &hmc_info->sd_table
				    .sd_entry[hmc_info->pble_mr_hmc_index];
	}

	idx->pd_idx =
		(u32)((pble_rsrc->next_fpm_addr - pble_rsrc->fpm_base_addr) /
		      ZXDH_HMC_PAGED_BP_SIZE); //4096
	idx->rel_pd_idx = (idx->pd_idx % ZXDH_HMC_PD_CNT_IN_SD); // 512
	pages = (idx->rel_pd_idx) ? (ZXDH_HMC_PD_CNT_IN_SD - idx->rel_pd_idx) :
					  ZXDH_HMC_PD_CNT_IN_SD;

	pages = (u32)min(pages, pble_rsrc->unallocated_pble >>
					PBLE_512_SHIFT); // PBLE_512_SHIFT==9

	info.chunk = chunk;
	info.hmc_info = hmc_info;
	info.pages = pages;
	info.sd_entry = sd_entry;

	ret_code = add_sd_direct(pble_rsrc, &info);

	if (ret_code)
		goto error;

	ret_code = zxdh_prm_add_pble_mem(&pble_rsrc->pinfo, chunk);
	if (ret_code)
		goto error;

	pble_rsrc->next_fpm_addr += chunk->size;
	pble_rsrc->unallocated_pble -= (u32)(chunk->size >> 3);
	pble_rsrc->free_pbles += (u32)(chunk->size >> 3);

	sd_entry->valid = true;
	list_add(&chunk->list, &pble_rsrc->pinfo.clist);

	return 0;

error:
	if (chunk->bitmapbuf)
		kfree(chunk->bitmapmem.va);
	kfree(chunk->chunkmem.va);

	return ret_code;
}

/**
 * free_lvl2 - fee level 2 pble
 * @pble_rsrc: pble resource management
 * @palloc: level 2 pble allocation
 */
static void free_lvl2(struct zxdh_hmc_pble_rsrc *pble_rsrc,
		      struct zxdh_pble_alloc *palloc)
{
	u32 i;
	struct zxdh_pble_level2 *lvl2 = &palloc->level2;
	struct zxdh_pble_info *root = &lvl2->root;
	struct zxdh_pble_info *leaf = lvl2->leaf;

	for (i = 0; i < lvl2->leaf_cnt; i++, leaf++) {
		if (!leaf) {
			pr_err("[zxdh_rdma] [%s]:%d leaf:%d is null\n", __func__, __LINE__, i);
			break;
		}
		if (leaf->addr)
			zxdh_prm_return_pbles(&pble_rsrc->pinfo,
					      &leaf->chunkinfo);
		else
			break;
	}

	if (root->addr)
		zxdh_prm_return_pbles(&pble_rsrc->pinfo, &root->chunkinfo);

	vfree(lvl2->leafmem.va);
	lvl2->leaf = NULL;
	lvl2->leafmem.va = NULL;
}

/**
 * get_lvl2_pble - get level 2 pble resource
 * @pble_rsrc: pble resource management
 * @palloc: level 2 pble allocation
 */
static int get_lvl2_pble(struct zxdh_hmc_pble_rsrc *pble_rsrc,
			 struct zxdh_pble_alloc *palloc)
{
	u32 lf4k, lflast, total, i;
	u32 pblcnt = PBLE_PER_PAGE;
	u64 *addr;
	struct zxdh_pble_level2 *lvl2 = &palloc->level2;
	struct zxdh_pble_info *root = &lvl2->root;
	struct zxdh_pble_info *leaf;
	int ret_code;
	u64 fpm_addr;
	dma_addr_t paaddr;

	/* number of full 512 (4K) leafs) */
	lf4k = palloc->total_cnt >> 9;
	lflast = palloc->total_cnt % PBLE_PER_PAGE;
	total = (lflast == 0) ? lf4k : lf4k + 1;
	lvl2->leaf_cnt = total;

	lvl2->leafmem.size = (sizeof(*leaf) * total);
	lvl2->leafmem.va = vzalloc(lvl2->leafmem.size);
	if (!lvl2->leafmem.va)
	{
		pr_info("[zxdh_rdma] %s %d failed to alloc lvl2->leafmem size:0x%x \n", __func__, __LINE__, lvl2->leafmem.size);
		return -ENOMEM;
	}

	lvl2->leaf = lvl2->leafmem.va;
	leaf = lvl2->leaf;
	ret_code = zxdh_prm_get_pbles(&pble_rsrc->pinfo, &root->chunkinfo,
				      total << 3, &root->addr, &fpm_addr,
				      &paaddr);
	if (ret_code) {
		vfree(lvl2->leafmem.va);
		lvl2->leaf = NULL;
		pr_info("[zxdh_rdma] %s %d faile to get lvl2 pble\n", __func__, __LINE__);
		return -ENOMEM;
	}

	root->smmu_fpm_addr = fpm_addr;
	root->pa = paaddr;
	root->idx = fpm_to_idx(pble_rsrc, fpm_addr);
	root->cnt = total;
	addr = root->addr;
	for (i = 0; i < total; i++, leaf++) {
		pblcnt = (lflast && ((i + 1) == total)) ? lflast :
								PBLE_PER_PAGE;
		ret_code = zxdh_prm_get_pbles(&pble_rsrc->pinfo,
					      &leaf->chunkinfo, pblcnt << 3,
					      &leaf->addr, &fpm_addr, &paaddr);
		if (ret_code)
			goto error;

		leaf->idx = fpm_to_idx(pble_rsrc, fpm_addr);
		leaf->smmu_fpm_addr = fpm_addr;
		leaf->pa = paaddr;
		leaf->cnt = pblcnt;
		*addr = (u64)leaf->idx;
		addr++;
	}

	if (pble_rsrc->pble_copy) {
		zxdh_cqp_config_pble_table_cmd(pble_rsrc->dev, root, total << 3,
					       pble_rsrc->pble_type);
	}

	palloc->level = PBLE_LEVEL_2;
	pble_rsrc->stats_lvl2++;
	return 0;

error:
	free_lvl2(pble_rsrc, palloc);

	return -ENOMEM;
}

/**
 * get_lvl1_pble - get level 1 pble resource
 * @pble_rsrc: pble resource management
 * @palloc: level 1 pble allocation
 */
static int get_lvl1_pble(struct zxdh_hmc_pble_rsrc *pble_rsrc,
			 struct zxdh_pble_alloc *palloc)
{
	int ret_code;
	u64 fpm_addr;
	dma_addr_t paaddr;
	struct zxdh_pble_info *lvl1 = &palloc->level1;

	ret_code = zxdh_prm_get_pbles(&pble_rsrc->pinfo, &lvl1->chunkinfo,
				      palloc->total_cnt << 3, &lvl1->addr,
				      &fpm_addr, &paaddr);
	if (ret_code)
		return -ENOMEM;

	palloc->level = PBLE_LEVEL_1;
	lvl1->idx = fpm_to_idx(pble_rsrc, fpm_addr);
	lvl1->cnt = palloc->total_cnt;
	lvl1->smmu_fpm_addr = fpm_addr;
	lvl1->pa = paaddr;
	pble_rsrc->stats_lvl1++;

	return 0;
}

/**
 * get_lvl1_lvl2_pble - calls get_lvl1 and get_lvl2 pble routine
 * @pble_rsrc: pble resources
 * @palloc: contains all inforamtion regarding pble (idx + pble addr)
 * @level1_only: flag for a level 1 PBLE
 */
static int get_lvl1_lvl2_pble(struct zxdh_hmc_pble_rsrc *pble_rsrc,
			      struct zxdh_pble_alloc *palloc, bool level1_only)
{
	int status = 0;

	status = get_lvl1_pble(pble_rsrc, palloc);
	if (!status || level1_only || palloc->total_cnt <= PBLE_PER_PAGE)
		return status;

	status = get_lvl2_pble(pble_rsrc, palloc);

	return status;
}

/**
 * zxdh_get_pble - allocate pbles from the prm
 * @pble_rsrc: pble resources
 * @palloc: contains all inforamtion regarding pble (idx + pble addr)
 * @pble_cnt: #of pbles requested
 * @level1_only: true if only pble level 1 to acquire
 */
int zxdh_get_pble(struct zxdh_hmc_pble_rsrc *pble_rsrc,
		  struct zxdh_pble_alloc *palloc, u32 pble_cnt,
		  bool level1_only)
{
	int status = 0;
	int max_sds = 0;
	int i;

	palloc->total_cnt = pble_cnt;
	palloc->level = PBLE_LEVEL_0;

	mutex_lock(&pble_rsrc->pble_mutex_lock);

	/*check first to see if we can get pble's without acquiring
	 * additional sd's
	 */
	status = get_lvl1_lvl2_pble(pble_rsrc, palloc, level1_only);
	if (!status)
		goto exit;

	max_sds = (palloc->total_cnt >> 18) + 1;
	for (i = 0; i < max_sds; i++) {
		status = add_pble_prm(pble_rsrc);
		if (status)
		{
			pr_err("[zxdh_rdma] %s[%d]: failed to add pble_chunck\n", __func__, __LINE__);
			break;
		}

		if (pble_rsrc->free_pbles < palloc->total_cnt)
			continue;

		status = get_lvl1_lvl2_pble(pble_rsrc, palloc, level1_only);
		/* if level1_only, only go through it once */
		if (!status || level1_only)
			break;
	}

exit:
	if (!status) {
		pble_rsrc->allocdpbles += pble_cnt;
		pble_rsrc->free_pbles -= pble_cnt;
		pble_rsrc->stats_alloc_ok++;
	} else {
		pble_rsrc->stats_alloc_fail++;
	}
	mutex_unlock(&pble_rsrc->pble_mutex_lock);

	return status;
}

/**
 * zxdh_free_pble - put pbles back into prm
 * @pble_rsrc: pble resources
 * @palloc: contains all information regarding pble resource being freed
 */
void zxdh_free_pble(struct zxdh_hmc_pble_rsrc *pble_rsrc,
		    struct zxdh_pble_alloc *palloc)
{
	pble_rsrc->free_pbles += palloc->total_cnt;

	if (palloc->level == PBLE_LEVEL_2)
		free_lvl2(pble_rsrc, palloc);
	else
		zxdh_prm_return_pbles(&pble_rsrc->pinfo,
				      &palloc->level1.chunkinfo);
	pble_rsrc->stats_alloc_freed++;
}
