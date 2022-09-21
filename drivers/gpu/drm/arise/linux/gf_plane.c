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

#include "gf_plane.h"
#include "gf_drmfb.h"
#include "gf_fence.h"

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

    gf_assert(plane && plane_state, GF_FUNC_NAME(__func__));
    gf_assert(!!crtc == !!fb, GF_FUNC_NAME(__func__));
    
    if(to_gf_plane(plane)->is_cursor == 0)
    {
#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)        
        return  drm_atomic_helper_update_plane(plane, crtc, fb, crtc_x, crtc_y, crtc_w, crtc_h, src_x, src_y, src_w, src_h, ctx);
#else
        return  drm_atomic_helper_update_plane(plane, crtc, fb, crtc_x, crtc_y, crtc_w, crtc_h, src_x, src_y, src_w, src_h);
#endif
    }

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
        gf_assert(funcs->atomic_check(plane, plane_state) == 0, GF_FUNC_NAME(__func__));
    }

    if(funcs && funcs->atomic_update)
    {
        funcs->atomic_update(plane, NULL);
    }

    plane->old_fb = plane->fb;

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
    
    gf_assert(plane && plane_state, GF_FUNC_NAME(__func__));
    
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
        gf_assert(funcs->atomic_check(plane, plane_state) == 0, GF_FUNC_NAME(__func__));
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

int gf_plane_atomic_check(struct drm_plane *plane, struct drm_plane_state *state)
{
    int  status = 0;
    unsigned int src_w, src_h, dst_w, dst_h;
    gf_plane_t* gf_plane = to_gf_plane(plane);
    
    DRM_DEBUG_KMS("plane=%d\n", plane->index);

    src_w = (state->src_w >> 16) & 0xFFFF;
    src_h = (state->src_h >> 16) & 0xFFFF;

    dst_w = state->crtc_w;
    dst_h = state->crtc_h;

    if(!gf_plane->can_window && (state->crtc_x != 0 || state->crtc_y != 0))
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
    signed long timeout;
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
        dma_fence_t *fence = reservation_object_get_excl_rcu(gfb->obj->resv);
        if (fence)
        {
            drm_atomic_set_fence_for_plane_gf(new_state, fence);
        }
    }

    return  0;
}

void gf_plane_atomic_update_internal(struct drm_plane *plane,  struct drm_plane_state *old_state)
{
    gf_card_t *card = plane->dev->dev_private;
    gf_plane_state_t*  gf_plane_state = to_gf_plane_state(plane->state);

    DRM_DEBUG_KMS("plane=%d, crtc=%d\n", plane->index, (to_gf_plane(plane))->crtc_index);

    if (to_gf_plane(plane)->is_cursor)
    {
        gf_cursor_update_t arg = {0, };

        arg.crtc        = to_gf_plane(plane)->crtc_index;

        if(gf_plane_state->legacy_cursor)
        {
            arg.vsync_on = 0;
        }
        else
        {
            arg.vsync_on = 1;
        }

        if (plane->state->crtc && !gf_plane_state->disable)
        {
            arg.bo          = plane->state->fb ? to_gfb(plane->state->fb)->obj : NULL;
            arg.pos_x       = plane->state->crtc_x;
            arg.pos_y       = plane->state->crtc_y;
            arg.width       = plane->state->crtc_w;
            arg.height      = plane->state->crtc_h;
        }

        disp_cbios_update_cursor(card->disp_info, &arg);
    }
    else
    {
        gf_crtc_flip_t arg = {0};

        arg.crtc        = to_gf_plane(plane)->crtc_index;
        arg.stream_type = to_gf_plane(plane)->plane_type;
        
        if (plane->state->crtc && !gf_plane_state->disable)
        {
            arg.fb = plane->state->fb;
            arg.crtc_x = plane->state->crtc_x;
            arg.crtc_y = plane->state->crtc_y;
            arg.crtc_w = plane->state->crtc_w;
            arg.crtc_h = plane->state->crtc_h;
            arg.src_x = plane->state->src_x >> 16;
            arg.src_y = plane->state->src_y >> 16;
            arg.src_w = plane->state->src_w >> 16;
            arg.src_h = plane->state->src_h >> 16;
            if(plane->type == DRM_PLANE_TYPE_OVERLAY)
            {
#if  DRM_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
                arg.blend_mode = gf_plane_state->pixel_blend_mode;
#if  DRM_VERSION_CODE < KERNEL_VERSION(4, 18, 0)
                arg.const_alpha = gf_plane_state->alpha;
#else
                arg.const_alpha = plane->state->alpha;
#endif
#else
                arg.const_alpha = plane->state->alpha;
                arg.blend_mode = plane->state->pixel_blend_mode;
#endif
                arg.blend_alpha_source = gf_plane_state->alpha_source;
            }
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
        }
    }
}

void gf_plane_atomic_update(struct drm_plane* plane, struct drm_plane_state* old_state)
{
    DRM_DEBUG_KMS("Update plane=%d\n", plane->index);

    to_gf_plane_state(plane->state)->disable = 0;

    if (old_state && old_state->state->legacy_cursor_update)
    {
        to_gf_plane_state(plane->state)->legacy_cursor = 1;
    }

    gf_plane_atomic_update_internal(plane, old_state);

    to_gf_plane_state(plane->state)->legacy_cursor = 0;
}

void gf_plane_atomic_disable(struct drm_plane *plane, struct drm_plane_state *old_state)
{
    DRM_DEBUG_KMS("Disable plane=%d\n", plane->index);

    to_gf_plane_state(plane->state)->disable = 1;

    if (old_state && old_state->state->legacy_cursor_update)
    {
        to_gf_plane_state(plane->state)->legacy_cursor = 1;
    }

    gf_plane_atomic_update_internal(plane, old_state);

    to_gf_plane_state(plane->state)->legacy_cursor = 0;
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
