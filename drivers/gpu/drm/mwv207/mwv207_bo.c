// SPDX-License-Identifier: GPL-2.0+
/*
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
#include <linux/io-mapping.h>
#include <drm/drm_file.h>
#include "mwv207_drm.h"
#include "mwv207_gem.h"
#include "mwv207_bo.h"

static void mwv207_ttm_bo_destroy(struct ttm_buffer_object *bo)
{
	struct mwv207_bo *jbo = to_jbo(bo);

	WARN_ON_ONCE(jbo->map_count > 0);
	if (bo->base.import_attach)
		drm_prime_gem_destroy(&bo->base, NULL);
	drm_gem_object_release(&jbo->tbo.base);
	kfree(jbo);
}

bool mwv207_ttm_bo_is_mwv207_bo(struct ttm_buffer_object *jbo)
{
	if (jbo->destroy == &mwv207_ttm_bo_destroy)
		return true;
	return false;
}

void mwv207_bo_placement_from_domain(struct mwv207_bo *jbo, u32 domain, bool pinned)
{
	struct mwv207_device *jdev = bdev_to_jdev(jbo->tbo.bdev);
	u32 c = 0;

	jbo->placement.placement = jbo->placements;
	jbo->placement.busy_placement = jbo->placements;

	if (domain & 0x2) {
		jbo->placements[c].fpfn = 0;

		if (jbo->flags & (1<<2))
			jbo->placements[c].lpfn = 0x100000000ul >> PAGE_SHIFT;
		else
			jbo->placements[c].lpfn = 0;

		if (jbo->flags & (1<<0))
			jbo->placements[c].lpfn = jdev->visible_vram_size >> PAGE_SHIFT;
		jbo->placements[c].flags = 0;
		jbo->placements[c].mem_type = TTM_PL_VRAM;

#ifdef MWV207_DEBUG_BO_MIGRATION
		if (!pinned) {
			if (jbo->placements[c].lpfn == 0
					|| jbo->placements[c].lpfn >= jdev->vram_size >> PAGE_SHIFT)
				jbo->placements[c].lpfn = jdev->vram_size >> PAGE_SHIFT;
			if (jbo->toggle & 0x1)
				jbo->placements[c].fpfn = jbo->placements[c].lpfn / 2;
			else
				jbo->placements[c].lpfn = jbo->placements[c].lpfn / 2;
			jbo->toggle ^= 0x1;
		}
#endif
		c++;
	}
	if (domain & 0x4) {
		jbo->placements[c].fpfn = 0;
		jbo->placements[c].lpfn = 0;
		jbo->placements[c].flags = 0;
		jbo->placements[c].mem_type = TTM_PL_TT;
		c++;
	}
	if (domain & 0x1) {
		jbo->placements[c].fpfn = 0;
		jbo->placements[c].lpfn = 0;
		jbo->placements[c].flags = 0;
		jbo->placements[c].mem_type = TTM_PL_SYSTEM;
		c++;
	}
	if (!c) {
		jbo->placements[c].fpfn = 0;
		jbo->placements[c].lpfn = 0;
		jbo->placements[c].flags = 0;
		jbo->placements[c].mem_type = TTM_PL_SYSTEM;
		c++;
	}

	BUG_ON(c > 3);
	jbo->placement.num_placement = c;
	jbo->placement.num_busy_placement = c;
}

int mwv207_bo_create(struct mwv207_device *jdev,
		  u64 size, u64 align, enum ttm_bo_type type,
		  u32 domain, u32 flags,
		  struct mwv207_bo **pbo)
{
	struct mwv207_bo *jbo;
	int ret;

	*pbo = NULL;
	if ((flags & (1<<2)) && !(domain & 0x2))
		return -EINVAL;

	jbo = kzalloc(sizeof(struct mwv207_bo), GFP_KERNEL);
	if (jbo == NULL)
		return -ENOMEM;
	size = roundup(size, PAGE_SIZE);
	ret = drm_gem_object_init(&jdev->base, &jbo->tbo.base, size);
	if (unlikely(ret)) {
		kfree(jbo);
		return ret;
	}
	jbo->flags  = flags;
	jbo->domain = domain;

	jbo->tbo.base.funcs = &mwv207_gem_object_funcs;

	jbo->tbo.bdev = &jdev->bdev;
	mwv207_bo_placement_from_domain(jbo, domain, false);

	ret = ttm_bo_init_validate(&jdev->bdev, &jbo->tbo, type,
			&jbo->placement, align >> PAGE_SHIFT,
			type != ttm_bo_type_kernel,
			NULL, NULL, &mwv207_ttm_bo_destroy);

	if (unlikely(ret))
		return ret;
	*pbo = jbo;
	return 0;
}

int mwv207_bo_reserve(struct mwv207_bo *jbo, bool no_intr)
{
	int ret;

	ret = ttm_bo_reserve(&jbo->tbo, !no_intr, false, NULL);
	if (unlikely(ret)) {
		if (ret != -ERESTARTSYS)
			DRM_ERROR("bo reserve failed\n");
		return ret;
	}
	return 0;
}

void mwv207_bo_unreserve(struct mwv207_bo *jbo)
{
	ttm_bo_unreserve(&jbo->tbo);
}

int mwv207_bo_kmap_reserved(struct mwv207_bo *jbo, void **ptr)
{
	bool is_iomem;
	int ret;

	if ((!jbo->flags) & (1<<0))
		return -EPERM;

	if (jbo->kptr) {
		if (ptr)
			*ptr = jbo->kptr;
		jbo->map_count++;
		return 0;
	}

	ret = ttm_bo_kmap(&jbo->tbo, 0, PFN_UP(jbo->tbo.base.size), &jbo->kmap);

	if (ret)
		return ret;
	jbo->kptr = ttm_kmap_obj_virtual(&jbo->kmap, &is_iomem);
	if (ptr)
		*ptr = jbo->kptr;
	jbo->map_count = 1;
	return 0;
}

void mwv207_bo_kunmap_reserved(struct mwv207_bo *jbo)
{
	if (WARN_ON_ONCE(jbo->kptr == NULL))
		return;
	jbo->map_count--;
	if (jbo->map_count > 0)
		return;
	jbo->kptr = NULL;
	ttm_bo_kunmap(&jbo->kmap);
}

void mwv207_bo_unref(struct mwv207_bo *jbo)
{
	if (jbo == NULL)
		return;

	ttm_bo_put(&jbo->tbo);
	jbo = NULL;
}

struct mwv207_bo *mwv207_bo_ref(struct mwv207_bo *jbo)
{
	if (jbo == NULL)
		return NULL;

	ttm_bo_get(&jbo->tbo);
	return jbo;
}

int mwv207_bo_pin_reserved(struct mwv207_bo *jbo, u32 domain)
{
	struct ttm_operation_ctx ctx = {false, false};
	int ret;

	if (jbo->tbo.pin_count) {
		ttm_bo_pin(&jbo->tbo);
		return 0;
	}

	jbo->flags |= (1<<2);
	mwv207_bo_placement_from_domain(jbo, domain, true);
	ret = ttm_bo_validate(&jbo->tbo, &jbo->placement, &ctx);
	if (ret)
		return ret;

	ttm_bo_pin(&jbo->tbo);

	return 0;
}

int mwv207_bo_unpin_reserved(struct mwv207_bo *jbo)
{
	struct ttm_operation_ctx ctx = {false, false};
	int ret;

	if (WARN_ON_ONCE(!jbo->tbo.pin_count))
		return 0;

	ttm_bo_unpin(&jbo->tbo);
	if (jbo->tbo.pin_count)
		return 0;

	ret = ttm_bo_validate(&jbo->tbo, &jbo->placement, &ctx);
	if (unlikely(ret))
		DRM_ERROR("validate failed for unpin");

	return ret;
}

int mwv207_bo_pin(struct mwv207_bo *jbo, u32 domain)
{
	int ret;

	ret = mwv207_bo_reserve(jbo, true);
	if (ret)
		return ret;
	ret = mwv207_bo_pin_reserved(jbo, domain);
	mwv207_bo_unreserve(jbo);
	return ret;
}

int mwv207_bo_unpin(struct mwv207_bo *jbo)
{
	int ret;

	ret = mwv207_bo_reserve(jbo, true);
	if (ret)
		return ret;
	ret = mwv207_bo_unpin_reserved(jbo);
	mwv207_bo_unreserve(jbo);
	return ret;
}

