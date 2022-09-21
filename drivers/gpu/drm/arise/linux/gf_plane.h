//*****************************************************************************
//  Copyright (c) 2021 Glenfly Tech Co., Ltd.. 
//  All Rights Reserved.
//
//  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Glenfly Tech Co., Ltd..;
//  the contents of this file may not be disclosed to third parties, copied or
//  duplicated in any form, in whole or in part, without the prior written
//  permission of Glenfly Tech Co., Ltd..
//
//  The copyright of the source code is protected by the copyright laws of the People's
//  Republic of China and the related laws promulgated by the People's Republic of China
//  and the international covenant(s) ratified by the People's Republic of China. 
//*****************************************************************************

#ifndef  _GF_PLANE_H_
#define _GF_PLANE_H_

#include "gf_disp.h"
#include "gf_cbios.h"
#include "gf_fence.h"

#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)

int gf_atomic_helper_update_plane(struct drm_plane *plane,
				   struct drm_crtc *crtc,
				   struct drm_framebuffer *fb,
				   int crtc_x, int crtc_y,
				   unsigned int crtc_w, unsigned int crtc_h,
				   uint32_t src_x, uint32_t src_y,
				   uint32_t src_w, uint32_t src_h,
				   struct drm_modeset_acquire_ctx *ctx);

int gf_atomic_helper_disable_plane(struct drm_plane *plane, struct drm_modeset_acquire_ctx *ctx);

#else

int gf_atomic_helper_update_plane(struct drm_plane *plane,
				   struct drm_crtc *crtc,
				   struct drm_framebuffer *fb,
				   int crtc_x, int crtc_y,
				   unsigned int crtc_w, unsigned int crtc_h,
				   uint32_t src_x, uint32_t src_y,
				   uint32_t src_w, uint32_t src_h);

int gf_atomic_helper_disable_plane(struct drm_plane *plane);

#endif

void gf_plane_destroy(struct drm_plane *plane);

void  gf_plane_destroy_state(struct drm_plane *plane, struct drm_plane_state *state);

struct drm_plane_state*  gf_plane_duplicate_state(struct drm_plane *plane);

int gf_plane_atomic_get_property(struct drm_plane *plane,
				const struct drm_plane_state *state,
				struct drm_property *property,
				uint64_t *val);

int  gf_plane_atomic_set_property(struct drm_plane *plane,
				struct drm_plane_state *state,
				struct drm_property *property,
				uint64_t val);

int gf_plane_atomic_check(struct drm_plane *plane, struct drm_plane_state *state);

void gf_plane_atomic_update(struct drm_plane *plane,  struct drm_plane_state *old_state);

void gf_plane_atomic_disable(struct drm_plane *plane, struct drm_plane_state *old_state);

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
int gf_prepare_plane_fb(struct drm_plane *plane, struct drm_plane_state *new_state);
void gf_cleanup_plane_fb(struct drm_plane *plane,  struct drm_plane_state *old_state);
void drm_atomic_set_fence_for_plane_gf(struct drm_plane_state *plane_state, dma_fence_t *fence);
#else
int gf_prepare_plane_fb(struct drm_plane *plane, const struct drm_plane_state *new_state);
void gf_cleanup_plane_fb(struct drm_plane *plane, const struct drm_plane_state *old_state);
void drm_atomic_set_fence_for_plane_gf(const struct drm_plane_state *plane_state, dma_fence_t *fence);
#endif

#else

int gf_update_plane(struct drm_plane *plane, struct drm_crtc *crtc,
		   struct drm_framebuffer *fb, int crtc_x, int crtc_y,
		   unsigned int crtc_w, unsigned int crtc_h,
		   uint32_t src_x, uint32_t src_y,
		   uint32_t src_w, uint32_t src_h);

int gf_disable_plane(struct drm_plane *plane);

void gf_legacy_plane_destroy(struct drm_plane* plane);

#endif

#endif
