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
#ifndef MWV207_BO_H_T4ETFCB2
#define MWV207_BO_H_T4ETFCB2

#include <linux/version.h>
#include <linux/types.h>
#include <drm/drm_gem.h>
#include <drm/ttm/ttm_bo.h>
#include <drm/ttm/ttm_tt.h>
#include <drm/ttm/ttm_placement.h>
#include "mwv207.h"
#include "mwv207_drm.h"

#undef MWV207_DEBUG_BO_MIGRATION

#define to_jbo(bo) container_of(bo, struct mwv207_bo, tbo)
struct mwv207_bo {
	struct ttm_buffer_object tbo;
	struct ttm_placement placement;
	struct ttm_bo_kmap_obj kmap;
	struct ttm_place placements[3];
	u32    domain;
	u32    flags;
	struct iosys_map map;
	int    map_count;
	void  *kptr;
#ifdef MWV207_DEBUG_BO_MIGRATION
	int  toggle;
#endif
};

struct mwv207_ttm_tt {
	struct ttm_tt             ttm;
	struct mwv207_device     *jdev;
};

#define to_gtt(ttm_tt) container_of(ttm_tt, struct mwv207_ttm_tt, ttm)

static inline struct mwv207_bo *mwv207_bo_from_gem(struct drm_gem_object *gobj)
{
	struct ttm_buffer_object *tbo = container_of(gobj, struct ttm_buffer_object, base);

	return container_of(tbo, struct mwv207_bo, tbo);
}

static inline struct drm_gem_object *mwv207_gem_from_bo(struct mwv207_bo *jbo)
{
	return &jbo->tbo.base;
}

static inline u64 mwv207_bo_size(struct mwv207_bo *jbo)
{
	return jbo->tbo.resource->size;
}

static inline u64 mwv207_bo_mmap_offset(struct drm_gem_object *gobj)
{
	struct mwv207_bo *jbo = mwv207_bo_from_gem(gobj);

	return drm_vma_node_offset_addr(&jbo->tbo.base.vma_node);
}

static inline u64 mwv207_bo_gpu_phys(struct mwv207_bo *jbo)
{
	return jbo->tbo.resource->start << PAGE_SHIFT;
}

void mwv207_bo_placement_from_domain(struct mwv207_bo *jbo, u32 domain, bool pinned);

bool mwv207_ttm_bo_is_mwv207_bo(struct ttm_buffer_object *tbo);
void mwv207_bo_placement(struct mwv207_bo *jbo, int domain);

int mwv207_bo_create(struct mwv207_device *jdev,
		  u64 size, u64 align, enum ttm_bo_type type,
		  u32 domain, u32 flags,
		  struct mwv207_bo **pbo);

int mwv207_bo_reserve(struct mwv207_bo *jbo, bool no_intr);
void mwv207_bo_unreserve(struct mwv207_bo *jbo);

int mwv207_bo_pin_reserved(struct mwv207_bo *jbo, u32 domain);
int mwv207_bo_unpin_reserved(struct mwv207_bo *jbo);

int mwv207_bo_pin(struct mwv207_bo *jbo, u32 domain);
int mwv207_bo_unpin(struct mwv207_bo *jbo);

int mwv207_bo_kmap_reserved(struct mwv207_bo *jbo, void **ptr);
void mwv207_bo_kunmap_reserved(struct mwv207_bo *jbo);

struct mwv207_bo *mwv207_bo_ref(struct mwv207_bo *jbo);
void   mwv207_bo_unref(struct mwv207_bo *jbo);

int mwv207_ttm_init(struct mwv207_device *jdev);
void mwv207_ttm_fini(struct mwv207_device *jdev);


#endif
