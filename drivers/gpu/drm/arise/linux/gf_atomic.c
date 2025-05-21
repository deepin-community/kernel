/*
 * Copyright © 2021 Glenfly Tech Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */
#include "gf_atomic.h"
#include "gf_disp.h"
#include "gf_kms.h"
#include "gf_sink.h"
#include "gf_splice.h"
#include "gf_pm.h"

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
    gf_crtc_state_t* crtc_state, *cur_crtc_state;

    cur_crtc_state = to_gf_crtc_state(crtc->state);

    crtc_state = gf_calloc(sizeof(gf_crtc_state_t));
    if (!crtc_state)
    {
        return NULL;
    }

    if(cur_crtc_state->sink)
    {
        crtc_state->sink = cur_crtc_state->sink;
        gf_sink_get(crtc_state->sink);
    }

    __drm_atomic_helper_crtc_duplicate_state(crtc, &crtc_state->base_cstate);

    return &crtc_state->base_cstate;
}

void gf_crtc_destroy_state(struct drm_crtc *crtc, struct drm_crtc_state *s)
{
    gf_crtc_state_t*  state = to_gf_crtc_state(s);

    if(state->sink)
    {
        gf_sink_put(state->sink);
    }

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

    if (!state || !state->active)
    {
        return 0;
    }

    if (state->enable &&
        !(state->plane_mask & 1 << drm_plane_index(crtc->primary))) {
        DRM_DEBUG_ATOMIC("can't enable crtc-%d without enabling primary plane.\n", crtc->index);
        return -EINVAL;
    }

    if (!state->mode_changed && !drm_mode_equal(&state->adjusted_mode, &crtc->state->adjusted_mode))
    {
        DRM_DEBUG_ATOMIC("Adj mode changed on crtc-%d, need do full modeset.\n", crtc->index);
        state->mode_changed = 1;
    }
    else if (!state->mode_changed && state->connectors_changed)
    {
        DRM_DEBUG_ATOMIC("Connectors changed on crtc-%d, need do full modeset.\n", crtc->index);
        state->mode_changed = 1;
    }

    return 0;
}

int gf_connector_atomic_set_property(struct drm_connector *connector,
                                     struct drm_connector_state *state,
                                     struct drm_property *property,
                                     uint64_t val)
{
    int ret = 0;
    gf_connector_t* gf_conn = to_gf_connector(connector);
    gf_connector_state_t* gf_conn_state = to_gf_conn_state(state);

    ret = gf_splice_set_connector_property(state, property, val);
    if (ret)
    {

        DRM_ERROR("Invalid driver-private property '%s'\n", property->name);
    }

    return ret;
}

int gf_connector_atomic_get_property(struct drm_connector *connector,
                                     const struct drm_connector_state *state,
                                     struct drm_property *property,
                                     uint64_t *val)
{
    int ret = 0;
    gf_connector_t* gf_conn = to_gf_connector(connector);
    gf_connector_state_t* gf_conn_state = to_gf_conn_state(state);

    ret = gf_splice_get_connector_property(state, property, val);
    if (ret)
    {
        DRM_ERROR("Invalid driver-private property '%s'\n", property->name);
    }

    return ret;
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

    gf_splice_duplicate_connector_state(&gf_conn_state->base_conn_state, connector->state);

    return &gf_conn_state->base_conn_state;
}

void gf_connector_destroy_state(struct drm_connector *connector, struct drm_connector_state *state)
{
    gf_connector_state_t *gf_conn_state = to_gf_conn_state(state);
    __drm_atomic_helper_connector_destroy_state(state);
    gf_free(gf_conn_state);
}



static void gf_update_crtc_sink(struct drm_atomic_state *old_state)
{
    struct drm_crtc *crtc;
    struct drm_crtc_state *old_crtc_state, *new_crtc_state;
    gf_crtc_state_t *new_gf_crtc_state;
    struct drm_connector_state *new_conn_state;
    struct drm_connector *connector;
    gf_connector_t *gf_connector = NULL;
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
    struct drm_connector_state *old_conn_state;
#endif
    int i, j;

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
    for_each_crtc_in_state(old_state, crtc, old_crtc_state, i)
    {
        new_crtc_state = crtc->state;
#else
    for_each_oldnew_crtc_in_state(old_state, crtc, old_crtc_state, new_crtc_state, i)
    {
#endif
        new_gf_crtc_state = to_gf_crtc_state(new_crtc_state);
        gf_connector = NULL;

        if (new_crtc_state->active && drm_atomic_crtc_needs_modeset(new_crtc_state))
        {
            //find the first crtc matching connector
        #if DRM_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
            for_each_connector_in_state(old_state, connector, old_conn_state, j)
            {
                new_conn_state = connector->state;
        #else
            for_each_new_connector_in_state(old_state, connector, new_conn_state, j)
            {
        #endif
                if (new_conn_state->crtc == crtc)
                {
                    gf_connector = to_gf_connector(connector);
                    break;
                }
            }

            if (gf_connector && new_gf_crtc_state->sink != gf_connector->sink)
            {
                gf_sink_put(new_gf_crtc_state->sink);
                new_gf_crtc_state->sink = gf_connector->sink;
                gf_sink_get(new_gf_crtc_state->sink);
            }
        }

        if (old_crtc_state->active &&
            drm_atomic_crtc_needs_modeset(new_crtc_state))
        {
            if (!new_crtc_state->active)
            {
                gf_sink_put(new_gf_crtc_state->sink);
                new_gf_crtc_state->sink = NULL;
            }
        }
    }
}

void gf_atomic_helper_commit_tail(struct drm_atomic_state *old_state)
{
    struct drm_device *dev = old_state->dev;
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t*)gf_card->disp_info;

    struct drm_crtc *crtc;
    struct drm_crtc_state *old_crtc_state, *new_crtc_state;
    int i;

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
    uint32_t flags = DRM_PLANE_COMMIT_NO_DISABLE_AFTER_MODESET;
#else
    bool flags = false;
#endif

    gf_acquire_display(disp_info, DISP_FLIP_REF);

    drm_atomic_helper_commit_modeset_disables(dev, old_state);

    drm_atomic_helper_commit_modeset_enables(dev, old_state);

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
    for_each_crtc_in_state(old_state, crtc, old_crtc_state, i)
    {
        new_crtc_state = crtc->state;
#else
    for_each_oldnew_crtc_in_state(old_state, crtc, old_crtc_state, new_crtc_state, i)
    {
#endif
        if (old_crtc_state->active && !new_crtc_state->active)
        {
            gf_rpm_put_noidle(dev->dev);
        }

        if (new_crtc_state->active && !old_crtc_state->active)
        {
            gf_rpm_get_noresume(dev->dev);
        }
    }

    drm_atomic_helper_commit_planes(dev, old_state, flags);

    gf_atomic_commit_connector_prop(dev, old_state);

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

    gf_release_display(disp_info, DISP_FLIP_REF);

    gf_rpm_mark_last_busy(dev->dev);
}


#endif

