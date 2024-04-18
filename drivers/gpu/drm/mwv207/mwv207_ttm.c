/*
* SPDX-License-Identifier: GPL
*
* Copyright (c) 2020 ChangSha JingJiaMicro Electronics Co., Ltd.
* All rights reserved.
*
* Author:
*      shanjinkui <shanjinkui@jingjiamicro.com>
*
* The software and information contained herein is proprietary and
* confidential to JingJiaMicro Electronics. This software can only be
* used by JingJiaMicro Electronics Corporation. Any use, reproduction,
* or disclosure without the written permission of JingJiaMicro
* Electronics Corporation is strictly prohibited.
*/
#include <linux/version.h>
#include <linux/delay.h>
#include <drm/drm.h>
#include <drm/drm_print.h>
#include <drm/drm_file.h>
#include <drm/drm_debugfs.h>
#include <drm/ttm/ttm_bo.h>
#include <drm/ttm/ttm_tt.h>
#include <drm/ttm/ttm_range_manager.h>
#include <drm/ttm/ttm_placement.h>

#include "mwv207.h"
#include "mwv207_bo.h"
#include "mwv207_drm.h"
#include "mwv207_sched.h"

static int mwv207_mem_visible(struct mwv207_device *jdev, struct ttm_resource *mem)
{
	if (mem->mem_type == TTM_PL_SYSTEM ||
		mem->mem_type == TTM_PL_TT)
		return true;
	if (mem->mem_type != TTM_PL_VRAM)
		return false;

	return (((mem->start << PAGE_SHIFT) + mem->size) < jdev->visible_vram_size);
}

static void mwv207_evict_flags(struct ttm_buffer_object *bo,
		struct ttm_placement *placement)
{
	struct mwv207_device *jdev;
	struct mwv207_bo *jbo;
	static const struct ttm_place placements = {
		.fpfn = 0,
		.lpfn = 0,
		.flags = 0,
		.mem_type = TTM_PL_SYSTEM
	};

	if (!mwv207_ttm_bo_is_mwv207_bo(bo)) {
		placement->placement = &placements;
		placement->busy_placement = &placements;
		placement->num_placement = 1;
		placement->num_busy_placement = 1;
		return;
	}

	jbo = to_jbo(bo);
	jdev = ddev_to_jdev(jbo->tbo.base.dev);

	if (bo->resource->mem_type == TTM_PL_VRAM) {
		if (jdev->visible_vram_size != jdev->vram_size
			&& mwv207_mem_visible(jdev, bo->resource)) {
			jbo->flags &= ~((1<<0) | (1<<2));
			mwv207_bo_placement_from_domain(jbo,
					0x2 | 0x4,
					false);
			BUG_ON(!(jbo->placements[0].mem_type == TTM_PL_VRAM));
			BUG_ON(!(jbo->placements[1].mem_type == TTM_PL_TT));
			jbo->placements[0].fpfn = jdev->visible_vram_size >> PAGE_SHIFT;
			jbo->placements[0].lpfn = 0;
			jbo->placement.placement = &jbo->placements[0];
			jbo->placement.busy_placement = &jbo->placements[1];
			jbo->placement.num_placement = 1;
			jbo->placement.num_busy_placement = 1;
		} else
			mwv207_bo_placement_from_domain(jbo, 0x4, false);
	} else
		mwv207_bo_placement_from_domain(jbo, 0x1, false);

	*placement = jbo->placement;
}

static int mwv207_ttm_io_mem_reserve(struct ttm_device *bdev,
		struct ttm_resource *mem)
{
	struct mwv207_device *jdev = bdev_to_jdev(bdev);
	size_t bus_size = (size_t)mem->size;

	switch (mem->mem_type) {
	case TTM_PL_SYSTEM:
	case TTM_PL_TT:

		return 0;
	case TTM_PL_VRAM:

		mem->bus.offset = mem->start << PAGE_SHIFT;
		if (mem->bus.offset + bus_size > jdev->visible_vram_size)
			return -EINVAL;
		mem->bus.is_iomem = true;
		mem->bus.offset += jdev->vram_bar_base;
		mem->bus.caching = ttm_write_combined;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void mwv207_ttm_tt_destroy(struct ttm_device *bdev, struct ttm_tt *ttm)
{
	struct mwv207_ttm_tt *gtt = to_gtt(ttm);

	ttm_tt_fini(&gtt->ttm);
	kfree(gtt);
}

static struct ttm_tt *mwv207_ttm_tt_create(struct ttm_buffer_object *bo,
		u32 page_flags)
{
	struct mwv207_device *jdev;
	struct mwv207_ttm_tt *gtt;

	jdev = ddev_to_jdev(bo->base.dev);
	gtt = kzalloc(sizeof(struct mwv207_ttm_tt), GFP_KERNEL);
	if (gtt == NULL)
		return NULL;

	gtt->jdev = jdev;

	if (ttm_sg_tt_init(&gtt->ttm, bo, page_flags, ttm_cached)) {
		kfree(gtt);
		return NULL;
	}
	return &gtt->ttm;
}

static int mwv207_ttm_tt_populate(struct ttm_device *bdev,
			struct ttm_tt *ttm,
			struct ttm_operation_ctx *ctx)
{
	struct mwv207_device *jdev = bdev_to_jdev(bdev);

	return ttm_pool_alloc(&jdev->bdev.pool, ttm, ctx);
}

static void mwv207_ttm_tt_unpopulate(struct ttm_device *bdev, struct ttm_tt *ttm)
{
	struct mwv207_device *jdev = bdev_to_jdev(bdev);

	return ttm_pool_free(&jdev->bdev.pool, ttm);
}

static int mwv207_ttm_job_submit(struct mwv207_job *mjob, struct dma_fence **fence)
{
	*fence = &mjob->base.s_fence->finished;

	mwv207_job_get(mjob);
	drm_sched_entity_push_job(&mjob->base);

	return 0;
}

static struct mwv207_job *mwv207_ttm_job_alloc(struct ttm_buffer_object *bo)
{
	struct mwv207_job *mjob = mwv207_job_alloc();
	struct mwv207_dma_cmd *dma_cmd;
	struct mwv207_tvb *mtvb;
	int ret;

	if (!mjob)
		return ERR_PTR(-ENOMEM);

	dma_cmd = kmalloc(sizeof(*dma_cmd), GFP_KERNEL);
	if (!dma_cmd) {
		ret = -ENOMEM;
		goto err;
	}
	dma_cmd->src.offset = 0;
	dma_cmd->dst.offset = 0;
	dma_cmd->height = 1;
	dma_cmd->src.stride = bo->resource->size;
	dma_cmd->dst.stride = bo->resource->size;
	dma_cmd->width = bo->resource->size;

	mjob->cmds = (char *)dma_cmd;
	mjob->cmd_size = sizeof(*dma_cmd);
	mjob->engine_entity = ddev_to_jdev(bo->base.dev)->dma_entity;
	mjob->is_dma = true;

	mjob->mtvb = kzalloc(sizeof(struct mwv207_tvb), GFP_KERNEL);
	if (!mjob->mtvb) {
		ret = -ENOMEM;
		goto err;
	}
	mtvb = &mjob->mtvb[0];
	list_add_tail(&mtvb->base.head, &mjob->tvblist);

	ret = drm_sched_job_init(&mjob->base, mjob->engine_entity, NULL);
	if (ret)
		goto err;

	drm_sched_job_arm(&mjob->base);

	ret = drm_sched_job_add_implicit_dependencies(&mjob->base, &bo->base, true);

	if (ret)
		goto err;

	return mjob;
err:
	mwv207_job_put(mjob);
	return ERR_PTR(ret);
}

static int mwv207_move_vram_gtt(struct ttm_buffer_object *bo, bool evict,
				struct ttm_operation_ctx *ctx,
				struct ttm_resource *new_mem)
{
	struct ttm_tt *ttm = bo->ttm;
	struct mwv207_ttm_tt *gtt = to_gtt(ttm);
	struct mwv207_dma_cmd *dma_cmd;
	struct mwv207_job *mjob;
	struct dma_fence *fence;
	int ret;

	mjob = mwv207_ttm_job_alloc(bo);
	if (IS_ERR(mjob))
		return PTR_ERR(mjob);

	dma_cmd = (struct mwv207_dma_cmd *)mjob->cmds;
	dma_cmd->src.base = bo->resource->start << PAGE_SHIFT;
	dma_cmd->src.pg_nr_type = MWV207_DMA_NR_PAGES_VRAM(PFN_UP(bo->resource->size));
	dma_cmd->dst.base = (u64)gtt->ttm.dma_address;
	dma_cmd->dst.pg_nr_type = MWV207_DMA_NR_PAGES_RAM(ttm->num_pages);

	ret = mwv207_ttm_job_submit(mjob, &fence);
	if (!ret)
		ret = ttm_bo_move_accel_cleanup(bo, fence, evict, true, new_mem);

	mwv207_job_put(mjob);
	return ret;
}

static int mwv207_move_gtt_vram(struct ttm_buffer_object *bo, bool evict,
				struct ttm_operation_ctx *ctx,
				struct ttm_resource *new_mem)
{
	struct ttm_tt *ttm = bo->ttm;
	struct mwv207_ttm_tt *gtt = to_gtt(ttm);
	struct mwv207_dma_cmd *dma_cmd;
	struct mwv207_job *mjob;
	struct dma_fence *fence;
	int ret;

	mjob = mwv207_ttm_job_alloc(bo);
	if (IS_ERR(mjob))
		return PTR_ERR(mjob);

	dma_cmd = (struct mwv207_dma_cmd *)mjob->cmds;
	dma_cmd->src.base = (u64)gtt->ttm.dma_address;
	dma_cmd->src.pg_nr_type = MWV207_DMA_NR_PAGES_RAM(ttm->num_pages);
	dma_cmd->dst.base = new_mem->start << PAGE_SHIFT;
	dma_cmd->dst.pg_nr_type = MWV207_DMA_NR_PAGES_VRAM(PFN_UP(bo->resource->size));

	ret = mwv207_ttm_job_submit(mjob, &fence);
	if (!ret)
		ret = ttm_bo_move_accel_cleanup(bo, fence, evict, true, new_mem);

	mwv207_job_put(mjob);
	return ret;
}

static int mwv207_move_vram_vram(struct ttm_buffer_object *bo, bool evict,
				struct ttm_operation_ctx *ctx,
				struct ttm_resource *new_mem)
{
	struct mwv207_dma_cmd *dma_cmd;
	struct mwv207_job *mjob;
	struct dma_fence *fence;
	int ret;

	mjob = mwv207_ttm_job_alloc(bo);
	if (IS_ERR(mjob))
		return PTR_ERR(mjob);

	dma_cmd = (struct mwv207_dma_cmd *)mjob->cmds;
	dma_cmd->src.base = bo->resource->start << PAGE_SHIFT;
	dma_cmd->src.pg_nr_type = MWV207_DMA_NR_PAGES_VRAM(PFN_UP(bo->resource->size));
	dma_cmd->dst.base = new_mem->start << PAGE_SHIFT;
	dma_cmd->dst.pg_nr_type = MWV207_DMA_NR_PAGES_VRAM(PFN_UP(new_mem->size));

	ret = mwv207_ttm_job_submit(mjob, &fence);
	if (!ret)
		ret = ttm_bo_move_accel_cleanup(bo, fence, evict, true, new_mem);

	mwv207_job_put(mjob);
	return ret;
}

static int mwv207_bo_move(struct ttm_buffer_object *bo, bool evict,
		struct ttm_operation_ctx *ctx,
		struct ttm_resource *new_mem,
		struct ttm_place *hop)
{
	struct ttm_resource *old_mem = bo->resource;
	struct mwv207_bo *jbo = to_jbo(bo);
	struct mwv207_device *jdev;
	int ret;

	ret = ttm_bo_wait_ctx(bo, ctx);
	if (ret)
		return ret;

	if (WARN_ON_ONCE(jbo->tbo.pin_count > 0))
		return -EINVAL;

	jdev = ddev_to_jdev(jbo->tbo.base.dev);
	if (!old_mem || (old_mem->mem_type == TTM_PL_SYSTEM && bo->ttm == NULL)) {
		ttm_bo_move_null(bo, new_mem);
		goto out;
	}

	if (old_mem->mem_type == TTM_PL_SYSTEM &&
		new_mem->mem_type == TTM_PL_TT) {
		ttm_bo_move_null(bo, new_mem);
		goto out;
	}

	if (old_mem->mem_type == TTM_PL_TT &&
		new_mem->mem_type == TTM_PL_SYSTEM) {
		ttm_resource_free(bo, &bo->resource);
		ttm_bo_assign_mem(bo, new_mem);
		goto out;
	}

	if ((old_mem->mem_type == TTM_PL_SYSTEM &&
		 new_mem->mem_type == TTM_PL_VRAM) ||
		(old_mem->mem_type == TTM_PL_VRAM &&
		 new_mem->mem_type == TTM_PL_SYSTEM)) {
		hop->fpfn = 0;
		hop->lpfn = 0;
		hop->mem_type = TTM_PL_TT;
		hop->flags = 0;
		return -EMULTIHOP;
	}

	if (old_mem->mem_type == TTM_PL_TT && new_mem->mem_type == TTM_PL_VRAM) {
		ret = mwv207_move_gtt_vram(bo, evict, ctx, new_mem);
	} else if (old_mem->mem_type == TTM_PL_VRAM && new_mem->mem_type == TTM_PL_TT) {
		ret = mwv207_move_vram_gtt(bo, evict, ctx, new_mem);
	} else if (old_mem->mem_type == TTM_PL_VRAM && new_mem->mem_type == TTM_PL_VRAM) {
		ret = mwv207_move_vram_vram(bo, evict, ctx, new_mem);
	} else {
		ret = -EINVAL;
	}
	if (ret) {
		ret = ttm_bo_move_memcpy(bo, ctx, new_mem);
		if (ret)
			return ret;
	}

	if (!mwv207_mem_visible(jdev, new_mem))
		jbo->flags &= ~(1<<0);

out:
	return 0;
}

static void mwv207_bo_move_notify(struct ttm_buffer_object *bo,
		unsigned int old_type,
		struct ttm_resource *new_mem)
{
	struct mwv207_bo *jbo;
	struct mwv207_device *jdev;

	if (!mwv207_ttm_bo_is_mwv207_bo(bo))
		return;
	jbo  = to_jbo(bo);
	jdev = ddev_to_jdev(jbo->tbo.base.dev);
}

int mwv207_bo_fault_reserve_notify(struct ttm_buffer_object *bo)
{
	struct ttm_operation_ctx ctx = {
		.interruptible = true,
		.no_wait_gpu = false
	};
	struct mwv207_device *jdev;
	struct mwv207_bo *jbo;
	u64 size, offset;
	u32 domain;
	int ret;

	if (!mwv207_ttm_bo_is_mwv207_bo(bo))
		return 0;
	if (bo->resource->mem_type != TTM_PL_VRAM)
		return 0;

	jbo  = to_jbo(bo);
	jdev = ddev_to_jdev(jbo->tbo.base.dev);
	size = bo->resource->size;
	offset = bo->resource->start << PAGE_SHIFT;

	jbo->flags |= (1<<0);

	if ((offset + size) <= jdev->visible_vram_size)
		return 0;

	domain = 0x2;
retry:
	mwv207_bo_placement_from_domain(jbo, domain, false);
	ret = ttm_bo_validate(bo, &jbo->placement, &ctx);
	if (unlikely(ret)) {
		if (ret != -ERESTARTSYS) {
			if (domain == 0x2) {

				domain = 0x1;
				goto retry;
			}
		}
		return ret;
	}

	offset = bo->resource->start << PAGE_SHIFT;
	if (bo->resource->mem_type == TTM_PL_VRAM &&
		(offset + size) > jdev->visible_vram_size) {
		DRM_ERROR("bo validation goes crazy");
		return -EINVAL;
	}

	return ret;
}

static void mwv207_bo_delete_mem_notify(struct ttm_buffer_object *bo)
{
	unsigned int old_type = TTM_PL_SYSTEM;

	if (bo->resource)
		old_type = bo->resource->mem_type;
	mwv207_bo_move_notify(bo, old_type, NULL);
}

static struct ttm_device_funcs mwv207_bo_driver = {
	.ttm_tt_create = &mwv207_ttm_tt_create,
	.ttm_tt_populate = &mwv207_ttm_tt_populate,
	.ttm_tt_unpopulate = &mwv207_ttm_tt_unpopulate,
	.ttm_tt_destroy = &mwv207_ttm_tt_destroy,
	.eviction_valuable = ttm_bo_eviction_valuable,
	.evict_flags = &mwv207_evict_flags,
	.move = &mwv207_bo_move,
	.io_mem_reserve = &mwv207_ttm_io_mem_reserve,
	.delete_mem_notify = &mwv207_bo_delete_mem_notify,
};

int mwv207_ttm_init(struct mwv207_device *jdev)
{
	int ret;

	ret = ttm_device_init(&jdev->bdev,
			&mwv207_bo_driver, jdev->dev,
			jdev->base.anon_inode->i_mapping,
			jdev->base.vma_offset_manager, true,
			true);
	if (ret) {
		DRM_ERROR("failed to initialize buffer object driver(%d).\n", ret);
		return ret;
	}

	ret = ttm_range_man_init(&jdev->bdev, TTM_PL_TT, true,
			0x80000000ULL >> PAGE_SHIFT);
	if (ret) {
		DRM_ERROR("failed to initialize GTT heap.\n");
		goto out_no_vram;
	}

	ret = ttm_range_man_init(&jdev->bdev, TTM_PL_VRAM, false,
			jdev->vram_size >> PAGE_SHIFT);
	if (ret) {
		DRM_ERROR("failed to initialize VRAM heap.\n");
		goto out_no_vram;
	}
	DRM_INFO("mwv207: %lldM of VRAM memory size\n", jdev->vram_size / (1024 * 1024));
	return 0;

out_no_vram:
	ttm_device_fini(&jdev->bdev);
	return ret;
}

void mwv207_ttm_fini(struct mwv207_device *jdev)
{
	ttm_range_man_fini(&jdev->bdev, TTM_PL_VRAM);
	ttm_range_man_fini(&jdev->bdev, TTM_PL_TT);
	ttm_device_fini(&jdev->bdev);
	DRM_INFO("mwv207: ttm finalized\n");
}
