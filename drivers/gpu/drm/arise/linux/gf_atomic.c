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

#include "gf_atomic.h"

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)

struct drm_atomic_state* gf_atomic_state_alloc(struct drm_device *dev)
{
	gf_ato_state_t *state = gf_calloc(sizeof(gf_ato_state_t));

	if (!state || drm_atomic_state_init(dev, &state->base_ato_state) < 0) 
        {
		gf_free(state);
		return NULL;
	}

	return &state->base_ato_state;
}

void gf_atomic_state_clear(struct drm_atomic_state *s)
{
	gf_ato_state_t *state = to_gf_atomic_state(s);
	drm_atomic_state_default_clear(s);
}

void gf_atomic_state_free(struct drm_atomic_state *s)
{
    	gf_ato_state_t *state = to_gf_atomic_state(s);
	drm_atomic_state_default_release(s);
        gf_free(state);
}

struct drm_crtc_state* gf_crtc_duplicate_state(struct drm_crtc *crtc)
{
    gf_crtc_state_t* crtc_state;

    crtc_state = gf_calloc(sizeof(gf_crtc_state_t));
    if (!crtc_state)
    {
        return NULL;
    }

    __drm_atomic_helper_crtc_duplicate_state(crtc, &crtc_state->base_cstate);

    return &crtc_state->base_cstate;
}

void gf_crtc_destroy_state(struct drm_crtc *crtc, struct drm_crtc_state *s)
{
    gf_crtc_state_t*  state = to_gf_crtc_state(s);
    __drm_atomic_helper_crtc_destroy_state(s);
    gf_free(state);
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
int  gf_crtc_helper_check(struct drm_crtc *crtc, struct drm_atomic_state *atomic_state)
#else
int gf_crtc_helper_check(struct drm_crtc *crtc, struct drm_crtc_state *state)
#endif
{
    //a patch for HDMI & CRT on IGA1, when switch from IGA1->HDMI1 to IGA1->CRT with same timing, 
    //framework will not call crtc/encoder_set_mode, so we will meet active device/path error
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
    struct drm_crtc_state *state = drm_atomic_get_new_crtc_state(atomic_state,  crtc);
#endif
    if(!state || state->mode_changed || !state->active)
    {
        return 0;
    }

    if(!drm_mode_equal(&state->adjusted_mode, &crtc->state->adjusted_mode))
    {
        gf_info("Adj mode changed on crtc-%d, need do full modeset.\n", crtc->index);
        state->mode_changed = 1;
    }
    else if(state->connectors_changed)
    {
        gf_info("Connectors changed on crtc-%d, need do full modeset.\n", crtc->index);
        state->mode_changed = 1;
    }    

    return 0;
}

struct drm_connector_state* gf_connector_duplicate_state(struct drm_connector *connector)
{
    gf_connector_state_t* gf_conn_state;

    gf_conn_state = gf_calloc(sizeof(gf_connector_state_t));
    if (!gf_conn_state)
    {
        return NULL;
    }

    __drm_atomic_helper_connector_duplicate_state(connector, &gf_conn_state->base_conn_state);

    return &gf_conn_state->base_conn_state;
}

void gf_connector_destroy_state(struct drm_connector *connector, struct drm_connector_state *state)
{
    gf_connector_state_t *gf_conn_state = to_gf_conn_state(state);
    __drm_atomic_helper_connector_destroy_state(state);
    gf_free(gf_conn_state);
}

void gf_atomic_helper_commit_tail(struct drm_atomic_state *old_state)
{
    struct drm_device *dev = old_state->dev;

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
    uint32_t flags = DRM_PLANE_COMMIT_NO_DISABLE_AFTER_MODESET;
#else
    bool flags = false;
#endif

    drm_atomic_helper_commit_modeset_disables(dev, old_state);

    drm_atomic_helper_commit_modeset_enables(dev, old_state);

    drm_atomic_helper_commit_planes(dev, old_state, flags);

    drm_atomic_helper_commit_hw_done(old_state);

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
    drm_atomic_helper_fake_vblank(old_state);
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
    drm_atomic_helper_wait_for_flip_done(dev, old_state);
#else
    drm_atomic_helper_wait_for_vblanks(dev, old_state);
#endif

    drm_atomic_helper_cleanup_planes(dev, old_state);
}


#endif

