/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2021 Intel Corporation
 */

#ifndef __DRM_BUDDY_H__
#define __DRM_BUDDY_H__

#include <linux/gpu_buddy.h>

struct drm_printer;

/*
 * Compatibility aliases for the pre-rename DRM buddy allocator API, so
 * that out-of-tree modules written against the old drm_buddy_* interface
 * keep building after the allocator was moved one level up and renamed
 * to gpu_buddy_*.
 */
#define drm_buddy			gpu_buddy
#define drm_buddy_block			gpu_buddy_block
#define drm_buddy_free_tree		gpu_buddy_free_tree

#define DRM_BUDDY_RANGE_ALLOCATION	GPU_BUDDY_RANGE_ALLOCATION
#define DRM_BUDDY_TOPDOWN_ALLOCATION	GPU_BUDDY_TOPDOWN_ALLOCATION
#define DRM_BUDDY_CONTIGUOUS_ALLOCATION	GPU_BUDDY_CONTIGUOUS_ALLOCATION
#define DRM_BUDDY_CLEAR_ALLOCATION	GPU_BUDDY_CLEAR_ALLOCATION
#define DRM_BUDDY_CLEARED		GPU_BUDDY_CLEARED
#define DRM_BUDDY_TRIM_DISABLE		GPU_BUDDY_TRIM_DISABLE

#define DRM_BUDDY_CLEAR_TREE		GPU_BUDDY_CLEAR_TREE
#define DRM_BUDDY_DIRTY_TREE		GPU_BUDDY_DIRTY_TREE
#define DRM_BUDDY_MAX_FREE_TREES	GPU_BUDDY_MAX_FREE_TREES

#define DRM_BUDDY_HEADER_OFFSET		GPU_BUDDY_HEADER_OFFSET
#define DRM_BUDDY_HEADER_STATE		GPU_BUDDY_HEADER_STATE
#define DRM_BUDDY_ALLOCATED		GPU_BUDDY_ALLOCATED
#define DRM_BUDDY_FREE			GPU_BUDDY_FREE
#define DRM_BUDDY_SPLIT			GPU_BUDDY_SPLIT
#define DRM_BUDDY_HEADER_CLEAR		GPU_BUDDY_HEADER_CLEAR
#define DRM_BUDDY_HEADER_UNUSED		GPU_BUDDY_HEADER_UNUSED
#define DRM_BUDDY_HEADER_ORDER		GPU_BUDDY_HEADER_ORDER

#define DRM_BUDDY_MAX_ORDER		GPU_BUDDY_MAX_ORDER

#define drm_buddy_init			gpu_buddy_init
#define drm_buddy_fini			gpu_buddy_fini
#define drm_get_buddy			gpu_get_buddy
#define drm_buddy_alloc_blocks		gpu_buddy_alloc_blocks
#define drm_buddy_block_trim		gpu_buddy_block_trim
#define drm_buddy_reset_clear		gpu_buddy_reset_clear
#define drm_buddy_free_block		gpu_buddy_free_block
#define drm_buddy_free_list		gpu_buddy_free_list
#define drm_buddy_block_offset		gpu_buddy_block_offset
#define drm_buddy_block_order		gpu_buddy_block_order
#define drm_buddy_block_state		gpu_buddy_block_state
#define drm_buddy_block_is_allocated	gpu_buddy_block_is_allocated
#define drm_buddy_block_is_clear	gpu_buddy_block_is_clear
#define drm_buddy_block_is_free		gpu_buddy_block_is_free
#define drm_buddy_block_is_split	gpu_buddy_block_is_split
#define drm_buddy_block_size		gpu_buddy_block_size

/* DRM-specific GPU Buddy Allocator print helpers */
void drm_buddy_print(struct gpu_buddy *mm, struct drm_printer *p);
void drm_buddy_block_print(struct gpu_buddy *mm,
			   struct gpu_buddy_block *block,
			   struct drm_printer *p);
#endif
