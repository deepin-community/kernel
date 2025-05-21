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
#include "gf_plane.h"
#include "gf_disp.h"
#include "gf_drmfb.h"
#include "gf_fence.h"
#include "gf_modifies.h"
#include "gf_splice.h"

#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)

#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
int gf_atomic_helper_update_plane(struct drm_plane *plane,
                   struct drm_crtc *crtc,
                   struct drm_framebuffer *fb,
                   int crtc_x, int crtc_y,
                   unsigned int crtc_w, unsigned int crtc_h,
                   uint32_t src_x, uint32_t src_y,
                   uint32_t src_w, uint32_t src_h,
                   struct drm_modeset_acquire_ctx *ctx)
#else
int gf_atomic_helper_update_plane(struct drm_plane *plane,
                   struct drm_crtc *crtc,
                   struct drm_framebuffer *fb,
                   int crtc_x, int crtc_y,
                   unsigned int crtc_w, unsigned int crtc_h,
                   uint32_t src_x, uint32_t src_y,
                   uint32_t src_w, uint32_t src_h)
#endif
{
    struct drm_plane_state*  plane_state = plane->state;
    const struct drm_plane_helper_funcs *funcs;
    gf_card_t *gf_card = (gf_card_t *)plane->dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;

    gf_assert(!!crtc == !!fb, GF_FUNC_NAME(__func__));

    if (to_gf_plane(plane)->is_cursor == 0)
    {
#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
        return  drm_atomic_helper_update_plane(plane, crtc, fb, crtc_x, crtc_y, crtc_w, crtc_h, src_x, src_y, src_w, src_h, ctx);
#else
        return  drm_atomic_helper_update_plane(plane, crtc, fb, crtc_x, crtc_y, crtc_w, crtc_h, src_x, src_y, src_w, src_h);
#endif
    }

    gf_acquire_display(disp_info, DISP_CURSOR_REF);

    plane_state->crtc = crtc;
    if(crtc->state)
    {
        crtc->state->plane_mask |= (1 << drm_plane_index(plane));
    }

    drm_atomic_set_fb_for_plane(plane_state, fb);
    plane_state->crtc_x = crtc_x;
    plane_state->crtc_y = crtc_y;
    plane_state->crtc_w = crtc_w;
    plane_state->crtc_h = crtc_h;
    plane_state->src_x = src_x;
    plane_state->src_y = src_y;
    plane_state->src_w = src_w;
    plane_state->src_h = src_h;
    to_gf_plane_state(plane_state)->legacy_cursor = 1;

    funcs = plane->helper_private;
    if(funcs && funcs->atomic_check)
    {

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
        gf_assert(funcs->atomic_check(plane, NULL) == 0, GF_FUNC_NAME(__func__));
#else
        gf_assert(funcs->atomic_check(plane, plane_state) == 0, GF_FUNC_NAME(__func__));
#endif
    }

    if(funcs && funcs->atomic_update)
    {
        funcs->atomic_update(plane, NULL);
    }

    plane->old_fb = plane->fb;

    gf_release_display(disp_info, DISP_CURSOR_REF);

    return 0;
}

#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
int gf_atomic_helper_disable_plane(struct drm_plane *plane, struct drm_modeset_acquire_ctx *ctx)
#else
int gf_atomic_helper_disable_plane(struct drm_plane *plane)
#endif
{
    struct drm_plane_state*  plane_state = plane->state;
    const struct drm_plane_helper_funcs *funcs;

    if(to_gf_plane(plane)->is_cursor == 0)
    {
#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
        return  drm_atomic_helper_disable_plane(plane, ctx);
#else
        return  drm_atomic_helper_disable_plane(plane);
#endif
    }
    if((plane_state->crtc)&&(plane_state->crtc->state))
    {
        plane_state->crtc->state->plane_mask &= ~(1 << drm_plane_index(plane));
    }
    plane_state->crtc = NULL;
    drm_atomic_set_fb_for_plane(plane_state, NULL);
    plane_state->crtc_x = 0;
    plane_state->crtc_y = 0;
    plane_state->crtc_w = 0;
    plane_state->crtc_h = 0;
    plane_state->src_x = 0;
    plane_state->src_y = 0;
    plane_state->src_w = 0;
    plane_state->src_h = 0;
    to_gf_plane_state(plane_state)->legacy_cursor = 1;

    funcs = plane->helper_private;
    if(funcs && funcs->atomic_check)
    {
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
        gf_assert(funcs->atomic_check(plane, NULL) == 0, GF_FUNC_NAME(__func__));
#else
        gf_assert(funcs->atomic_check(plane, plane_state) == 0, GF_FUNC_NAME(__func__));
#endif
    }

    if(funcs && funcs->atomic_disable)
    {
        funcs->atomic_disable(plane, NULL);
    }

    plane->old_fb = plane->fb;

    return 0;
}

void gf_plane_destroy(struct drm_plane *plane)
{
    drm_plane_cleanup(plane);
    gf_free(to_gf_plane(plane));
}

void  gf_plane_destroy_state(struct drm_plane *plane, struct drm_plane_state *state)
{
    gf_plane_state_t* gf_pstate = to_gf_plane_state(state);

    __drm_atomic_helper_plane_destroy_state(state);

    gf_free(gf_pstate);
}

struct drm_plane_state*  gf_plane_duplicate_state(struct drm_plane *plane)
{
    struct drm_plane_state *state;
    gf_plane_state_t *gf_pstate;

    gf_pstate = gf_calloc(sizeof(gf_plane_state_t));

    if (!gf_pstate)
    {
        return NULL;
    }

    state = &gf_pstate->base_pstate;

    __drm_atomic_helper_plane_duplicate_state(plane, state);

    if(plane->state)
    {
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
        gf_pstate->pixel_blend_mode = to_gf_plane_state(plane->state)->pixel_blend_mode;
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 18, 0)
        gf_pstate->alpha = to_gf_plane_state(plane->state)->alpha;
#endif
#endif
        gf_pstate->alpha_source = to_gf_plane_state(plane->state)->alpha_source;
    }

    return state;
}

//get/set driver private property, we can add this later
int gf_plane_atomic_get_property(struct drm_plane *plane,
                const struct drm_plane_state *state,
                struct drm_property *property,
                uint64_t *val)
{
    int ret = -EINVAL;
    gf_plane_t* gf_plane = to_gf_plane(plane);
    gf_plane_state_t* gf_plane_state = to_gf_plane_state(state);

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
    if(property == gf_plane->blend_mode_property)
    {
        *val = gf_plane_state->pixel_blend_mode;
        ret = 0;
    }
#endif

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 18, 0)
    if(property == gf_plane->alpha_property)
    {
        *val = gf_plane_state->alpha;
        ret = 0;
    }
#endif

    if(property == gf_plane->alpha_source_prop)
    {
        *val = gf_plane_state->alpha_source;
        ret = 0;
    }

    if(ret)
    {
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
        DRM_WARN("Invalid driver-private property '%s'\n", property->name);
#endif
    }
    return ret;
}

int  gf_plane_atomic_set_property(struct drm_plane *plane,
                struct drm_plane_state *state,
                struct drm_property *property,
                uint64_t val)
{
    int ret = -EINVAL;
    gf_plane_t* gf_plane = to_gf_plane(plane);
    gf_plane_state_t* gf_plane_state = to_gf_plane_state(state);

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
    if(property == gf_plane->blend_mode_property)
    {
        gf_plane_state->pixel_blend_mode = val;
        ret = 0;
    }
#endif

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 18, 0)
    if(property == gf_plane->alpha_property)
    {
        gf_plane_state->alpha = val;
        ret = 0;
    }
#endif

    if(property == gf_plane->alpha_source_prop)
    {
        gf_plane_state->alpha_source = val;
        ret = 0;
    }

    if(ret)
    {
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
        DRM_WARN("Invalid driver-private property '%s'\n", property->name);
#endif
    }
    return ret;
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
int gf_plane_atomic_check(struct drm_plane *plane, struct drm_atomic_state *state)
#else
int gf_plane_atomic_check(struct drm_plane *plane, struct drm_plane_state *new_state)
#endif
{
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
    struct drm_plane_state *new_state = NULL;
#endif
    gf_plane_t* gf_plane = to_gf_plane(plane);
    unsigned int src_w, src_h, dst_w, dst_h;
    int  status = 0;

    DRM_DEBUG_KMS("plane=%d\n", plane->index);

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
    if (gf_plane->is_cursor)
    {
        new_state = plane->state;
    }
    else
    {
        new_state = drm_atomic_get_new_plane_state(state, plane);
    }
#endif

    src_w = (new_state->src_w >> 16) & 0xFFFF;
    src_h = (new_state->src_h >> 16) & 0xFFFF;

    dst_w = new_state->crtc_w;
    dst_h = new_state->crtc_h;

    if(!gf_plane->can_window && (new_state->crtc_x != 0 || new_state->crtc_y != 0))
    {
        status = -EINVAL;
        goto END;
    }

    if(!gf_plane->can_up_scale)
    {
        if((src_w < dst_w) || (src_h < dst_h))
        {
            status = -EINVAL;
            goto END;
        }
    }

    if(!gf_plane->can_down_scale)
    {
        if((src_w > dst_w) || (src_h > dst_h))
        {
            status = -EINVAL;
            goto END;
        }
    }
    //max cursor size if 128x128
    if (gf_plane->is_cursor)
    {
        if((dst_w > 128) || (dst_h > 128))
        {
            status = -EINVAL;
            goto END;
        }
    }

END:
    return  status;
}

//add this interface to make kernel 4.9 and above version compatible
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
void drm_atomic_set_fence_for_plane_gf(struct drm_plane_state *plane_state, dma_fence_t *fence)
#else
void drm_atomic_set_fence_for_plane_gf(const struct drm_plane_state *plane_state, dma_fence_t *fence)
#endif
{
    if (plane_state->fence) {
        dma_fence_put(fence);

        return;
    }

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
    plane_state->fence = fence;
#endif
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
int gf_prepare_plane_fb(struct drm_plane *plane, struct drm_plane_state *new_state)
#else
int gf_prepare_plane_fb(struct drm_plane *plane, const struct drm_plane_state *new_state)
#endif
{
    struct drm_framebuffer *fb = new_state->fb;
    struct drm_gf_framebuffer *gfb = to_gfb(fb);
    gf_card_t *gf = plane->dev->dev_private;

    if (!fb || !gfb->obj)
    {
        return 0;
    }

    /* resident */
    gf_core_interface->prepare_and_mark_unpagable(gf->adapter, gfb->obj->core_handle, &gfb->obj->info);

    if (plane->state->fb != fb)
    {
        dma_fence_t *fence = gf_reservation_object_get_excl_rcu(gfb->obj->resv);
        if (fence)
        {
            drm_atomic_set_fence_for_plane_gf(new_state, fence);
        }
    }

    return  0;
}

static bool gf_plane_can_async_flip(struct drm_plane_state *new_state, struct drm_plane_state *old_state)
{
    struct drm_gf_framebuffer *new_gfb = new_state->fb? to_gfb(new_state->fb) : NULL;
    struct drm_gf_framebuffer *old_gfb = old_state->fb? to_gfb(old_state->fb) : NULL;

    if (!old_gfb || !new_gfb)
    {
        return true;
    }

    if (!!new_gfb->obj->info.compress_format != !!old_gfb->obj->info.compress_format)
    {
        return false;
    }

    if (new_state->fb->pitches[0] != old_state->fb->pitches[0])
    {
        return false;
    }

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
    if (new_state->fb->pixel_format != old_state->fb->pixel_format)
#else
    if (new_state->fb->format->format != old_state->fb->format->format)
#endif
    {
        return false;
    }

    return true;
}

void gf_plane_atomic_update_internal(struct drm_plane *plane,  struct drm_plane_state *new_state, struct drm_plane_state *old_state)
{
    gf_card_t *card = plane->dev->dev_private;
    gf_plane_state_t*  gf_plane_state = to_gf_plane_state(plane->state);

    DRM_DEBUG_KMS("plane=%d, crtc=%d\n", plane->index, (to_gf_plane(plane))->crtc_index);

    if ((is_plane_work_in_splice_mode(plane) &&
          is_splice_target_active_in_drm(plane->dev)))
    {
        return;
    }

    if (to_gf_plane(plane)->is_cursor)
    {
        gf_cursor_update_t arg = {0, };

        arg.crtc        = to_gf_plane(plane)->crtc_index;

        if (gf_plane_state->legacy_cursor)
        {
            arg.vsync_on = 0;
        }
        else
        {
            arg.vsync_on = 1;
        }

        if (plane->state->crtc && !gf_plane_state->disable)
        {
            arg.bo          = new_state->fb ? to_gfb(new_state->fb)->obj : NULL;
            arg.pos_x       = new_state->crtc_x;
            arg.pos_y       = new_state->crtc_y;
            arg.width       = new_state->crtc_w;
            arg.height      = new_state->crtc_h;
        }

        disp_cbios_update_cursor(card->disp_info, &arg);
    }
    else
    {
        gf_crtc_flip_t arg = {0};

        arg.crtc        = to_gf_plane(plane)->crtc_index;
        arg.stream_type = to_gf_plane(plane)->plane_type;

        if (new_state->crtc && !gf_plane_state->disable)
        {
            arg.fb = new_state->fb;
            arg.crtc_x = new_state->crtc_x;
            arg.crtc_y = new_state->crtc_y;
            arg.crtc_w = new_state->crtc_w;
            arg.crtc_h = new_state->crtc_h;
            arg.src_x = new_state->src_x >> 16;
            arg.src_y = new_state->src_y >> 16;
            arg.src_w = new_state->src_w >> 16;
            arg.src_h = new_state->src_h >> 16;
            if(plane->type == DRM_PLANE_TYPE_OVERLAY)
            {
#if  DRM_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
                arg.blend_mode = gf_plane_state->pixel_blend_mode;
#if  DRM_VERSION_CODE < KERNEL_VERSION(4, 18, 0)
                arg.const_alpha = gf_plane_state->alpha;
#else
                arg.const_alpha = new_state->alpha;
#endif
#else
                arg.const_alpha = new_state->alpha;
                arg.blend_mode = new_state->pixel_blend_mode;
#endif
                arg.blend_alpha_source = gf_plane_state->alpha_source;
            }

#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
#if  DRM_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
            if(plane->state->crtc->state->async_flip)
#else
            if(plane->state->crtc->state->pageflip_flags & DRM_MODE_PAGE_FLIP_ASYNC)
#endif
            {
                if (gf_plane_can_async_flip(new_state, old_state))
                {
                    arg.async_flip = 1;
                }
                else
                {
                    gf_crtc_state_t* gf_cstate = to_gf_crtc_state(plane->state->crtc->state);
                    arg.async_flip = 0;
                    gf_cstate->keep_vsync = 1;
                }

            }
#endif
        }

        disp_cbios_crtc_flip(card->disp_info, &arg);

        {
            gf_perf_event_t perf_event = {0, };
            unsigned long long timestamp;

            gf_get_nsecs(&timestamp);
            perf_event.header.timestamp_high = timestamp >> 32;
            perf_event.header.timestamp_low = timestamp & 0xffffffff;

            perf_event.header.size = sizeof(gf_perf_event_ps_flip_t);
            perf_event.header.type = GF_PERF_EVENT_PS_FLIP;
            perf_event.ps_flip_event.iga_idx = to_gf_plane(plane)->crtc_index + 1;

            gf_core_interface->perf_event_add_event(card->adapter, &perf_event);
            card->fps_count++;
        }
    }
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
void gf_plane_atomic_update(struct drm_plane* plane, struct drm_atomic_state* state)
#else
void gf_plane_atomic_update(struct drm_plane* plane, struct drm_plane_state* old_state)
#endif
{
    struct drm_plane_state *new_state = NULL;
    gf_plane_t* gf_plane = to_gf_plane(plane);
    gf_plane_state_t* gf_pstate = to_gf_plane_state(plane->state);
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
    struct drm_plane_state* old_state = NULL;
#endif

    DRM_DEBUG_KMS("Update plane=%d\n", plane->index);

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
    if (gf_plane->is_cursor || state == NULL)
    {
        new_state = plane->state;
    }
    else
    {
        new_state = drm_atomic_get_new_plane_state(state, plane);
        old_state = drm_atomic_get_old_plane_state(state, plane);
    }
#else
    new_state = plane->state;
#endif

    if (gf_plane->resume_from_s4)
    {
        gf_pstate->disable = 1;
        gf_plane->resume_from_s4 = 0;
    }
    else
    {
        gf_pstate->disable = 0;
    }

    gf_plane_atomic_update_internal(plane, new_state, old_state);
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
void gf_plane_atomic_disable(struct drm_plane *plane, struct drm_atomic_state *state)
#else
void gf_plane_atomic_disable(struct drm_plane *plane, struct drm_plane_state *old_state)
#endif
{
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
    struct drm_plane_state *old_state = NULL;
#endif
    struct drm_plane_state *new_state = NULL;
    gf_plane_state_t* gf_pstate = to_gf_plane_state(plane->state);

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
    gf_plane_t* gf_plane = to_gf_plane(plane);

    if (gf_plane->is_cursor || state == NULL)
    {
        new_state = plane->state;
    }
    else
    {
        new_state = drm_atomic_get_new_plane_state(state, plane);
        old_state = drm_atomic_get_old_plane_state(state, plane);
    }
#else
    new_state = plane->state;
#endif

    DRM_DEBUG_KMS("Disable plane=%d\n", plane->index);

    gf_pstate->disable = 1;

    gf_plane_atomic_update_internal(plane, new_state, old_state);
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
void gf_cleanup_plane_fb(struct drm_plane *plane, struct drm_plane_state *old_state)
#else
void gf_cleanup_plane_fb(struct drm_plane *plane, const struct drm_plane_state *old_state)
#endif
{
    gf_card_t *gf = plane->dev->dev_private;
    struct drm_framebuffer *fb = old_state->fb;

    if (fb && to_gfb(fb)->obj)
    {
        gf_core_interface->mark_pagable(gf->adapter, to_gfb(fb)->obj->core_handle);
    }
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
bool gf_plane_format_mod_supported(struct drm_plane *plane, uint32_t format,
                                   uint64_t modifier)
{
    int i;
    bool ret = false;

    switch(modifier)
    {
    case DRM_FORMAT_MOD_GF_DISPLAY:
    case DRM_FORMAT_MOD_GF_LINEAR:
    case DRM_FORMAT_MOD_GF_TILED:
    case DRM_FORMAT_MOD_GF_COMPRESS:
    case DRM_FORMAT_MOD_GF_TILED_COMPRESS:
    case DRM_FORMAT_MOD_GF_LOCAL:
    case DRM_FORMAT_MOD_GF_PCIE:
    case DRM_FORMAT_MOD_GF_RB_ALT:
    case DRM_FORMAT_MOD_GF_INVALID:
        /* TODO: should do something after sync? */
        ret = true;
        goto check_done;
    default:
        break;
    }

    /* check that modifier is on the list of the plane's supported modifiers. */
    for (i = 0; i < plane->modifier_count; i++)
    {
        if (modifier == plane->modifiers[i])
            break;
    }
    if (i == plane->modifier_count)
    {
        ret = false;
        goto check_done;
    }

    /*
      TODO: check the format and modifier whether being supported
    */

check_done:
    if (!ret)
    {
        gf_info("^^^ not support modifier %lld\n", modifier);
    }
    return ret;
}

#endif

#else

int gf_update_plane(struct drm_plane *plane, struct drm_crtc *crtc,
           struct drm_framebuffer *fb, int crtc_x, int crtc_y,
           unsigned int crtc_w, unsigned int crtc_h,
           uint32_t src_x, uint32_t src_y,
           uint32_t src_w, uint32_t src_h)
{
    struct drm_device *dev = plane->dev;
    gf_crtc_t *gf_crtc = to_gf_crtc(crtc);
    gf_plane_t *gf_plane = to_gf_plane(plane);
    gf_card_t *card = dev->dev_private;
    int pipe = gf_plane->crtc_index;
    int ret = 0;
    int primary_w = crtc->mode.hdisplay, primary_h = crtc->mode.vdisplay;
    gf_crtc_flip_t arg = {0};

    src_x = src_x >> 16;
    src_y = src_y >> 16;
    src_w = src_w >> 16;
    src_h = src_h >> 16;

    if (crtc_x >= primary_w || crtc_y >= primary_h)
    {
        return -EINVAL;
    }

    /* Don't modify another pipe's plane */
    if (pipe != gf_crtc->pipe)
    {
        return -EINVAL;
    }

    if ((crtc_x + crtc_w) > primary_w)
    {
        crtc_w = primary_w - crtc_x;
    }

    if (crtc_y + crtc_h > primary_h)
    {
        crtc_h = primary_h - crtc_y;
    }

    if (!crtc_w || !crtc_h) /* Again, nothing to display */
    {
        goto out;
    }

    if(!gf_plane->can_window && (crtc_w != primary_w || crtc_h != primary_h))
    {
        return  -EINVAL;
    }

    if(!gf_plane->can_up_scale && (src_w < crtc_w || src_h < crtc_h))
    {
        return  -EINVAL;
    }

    if(!gf_plane->can_down_scale && (src_w > crtc_w || src_h > crtc_h))
    {
        return  -EINVAL;
    }

    arg.fb = fb;
    arg.crtc = to_gf_crtc(crtc)->pipe;
    arg.stream_type = gf_plane->plane_type;
    arg.crtc_x = crtc_x;
    arg.crtc_y = crtc_y;
    arg.crtc_w = crtc_w;
    arg.crtc_h = crtc_h;
    arg.src_x = src_x;
    arg.src_y = src_y;
    arg.src_w = src_w;
    arg.src_h = src_h;

    mutex_lock(&dev->struct_mutex);

    //need add wait fence back? intel is to pin fence
    disp_cbios_crtc_flip(card->disp_info, &arg);

    //need add wait_vblank if fb changed
    disp_wait_for_vblank(card->disp_info, pipe, 50);

    gf_plane->src_pos = src_x | (src_y << 16);
    gf_plane->src_size = src_w | (src_h << 16);
    gf_plane->dst_pos = crtc_x | (crtc_y << 16);
    gf_plane->dst_size = crtc_w | (crtc_h << 16);

    mutex_unlock(&dev->struct_mutex);
out:
    return ret;
}

int gf_disable_plane(struct drm_plane *plane)
{
    return 0;
}

void gf_legacy_plane_destroy(struct drm_plane* plane)
{
    gf_disable_plane(plane);
    drm_plane_cleanup(plane);
    gf_free(to_gf_plane(plane));
}

#endif
