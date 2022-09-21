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

#ifndef  _GF_ATOMIC_H_
#define _GF_ATOMIC_H_

#include "gf_disp.h"
#include "gf_cbios.h"

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)

struct drm_atomic_state* gf_atomic_state_alloc(struct drm_device *dev);

void gf_atomic_state_clear(struct drm_atomic_state *s);

void gf_atomic_state_free(struct drm_atomic_state *s);

struct drm_crtc_state* gf_crtc_duplicate_state(struct drm_crtc *crtc);

void gf_crtc_destroy_state(struct drm_crtc *crtc, struct drm_crtc_state *s);
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
    int gf_crtc_helper_check(struct drm_crtc *crtc, struct drm_atomic_state *state);
#else
    int gf_crtc_helper_check(struct drm_crtc *crtc, struct drm_crtc_state *state);
#endif
struct drm_connector_state* gf_connector_duplicate_state(struct drm_connector *connector);

void gf_connector_destroy_state(struct drm_connector *connector, struct drm_connector_state *state);

void gf_atomic_helper_commit_tail(struct drm_atomic_state *old_state);

#endif

#endif
