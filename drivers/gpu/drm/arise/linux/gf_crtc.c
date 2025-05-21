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
#include "gf_crtc.h"
#include "gf_fence.h"
#include "gf_drmfb.h"
#include "gf_splice.h"

void gf_crtc_destroy(struct drm_crtc *crtc)
{
    gf_crtc_t *gf_crtc = to_gf_crtc(crtc);

    drm_crtc_cleanup(crtc);

    gf_free(gf_crtc);
}

void gf_crtc_dpms_onoff_helper(struct drm_crtc *crtc, int dpms_on)
{
    struct drm_device *drm_dev = crtc->dev;
    gf_card_t *gf_card = drm_dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_crtc_t *gf_crtc = to_gf_crtc(crtc);
    int status = dpms_on? 1 : 0;

    if(gf_crtc->crtc_dpms && !status)
    {
        //turn off crtc
        if (!(is_crtc_work_in_splice_mode(crtc) &&
            is_splice_target_active_in_drm(drm_dev)))
        {
            disp_cbios_turn_onoff_screen(disp_info, gf_crtc->pipe, 0);
            disp_cbios_turn_onoff_iga(disp_info, gf_crtc->pipe, 0);
        }

        gf_crtc->crtc_dpms = status;
    }
    else if(!gf_crtc->crtc_dpms && status)
    {
        //turn on crtc
        if (!(is_crtc_work_in_splice_mode(crtc) &&
            is_splice_target_active_in_drm(drm_dev)))
        {
            disp_cbios_turn_onoff_iga(disp_info, gf_crtc->pipe, 1);
            disp_cbios_turn_onoff_screen(disp_info, gf_crtc->pipe, 1);
        }

        gf_crtc->crtc_dpms = status;
    }
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)

static void  gf_update_active_connector(struct drm_crtc *crtc)
{
    struct drm_device *  drm_dev = crtc->dev;
    gf_card_t*  gf_card = drm_dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t*)gf_card->disp_info;
    struct drm_connector* connector = NULL;
    gf_connector_t*  gf_connector = NULL;
    int i = 0;
    unsigned int active[MAX_CORE_CRTCS] = {0};

    if(!crtc->state->connector_mask || !crtc->state->active)
    {
        return;
    }

    gf_memcpy(active, disp_info->active_output, sizeof(disp_info->active_output));

    active[crtc->index] = 0;

    list_for_each_entry(connector, &drm_dev->mode_config.connector_list, head)
    {
        if((crtc->state->connector_mask & (1 << drm_connector_index(connector))) && (connector->state->crtc == crtc))
        {
            gf_connector = to_gf_connector(connector);

            for(i = 0; i < disp_info->num_crtc; i++)
            {
                active[i] &= ~gf_connector->output_type;
            }

            active[crtc->index] |= gf_connector->output_type;
        }
    }

    if(!disp_cbios_update_output_active(disp_info, active))
    {
        gf_memcpy(disp_info->active_output, active, sizeof(disp_info->active_output));
    }
}

void  gf_crtc_helper_set_mode(struct drm_crtc *crtc)
{
    struct drm_device *  drm_dev = crtc->dev;
    gf_card_t*  gf_card = drm_dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;
    struct drm_crtc_state * crtc_state = crtc->state;
    struct drm_display_mode* mode = &crtc->state->mode;
    struct drm_display_mode* adj_mode = &crtc_state->adjusted_mode;
    int flag = 0;
    struct task_struct *cur_task = current;
     //in atomic set phase, atomic state is updated to state of crtc/encoder/connector,
    //so we can't roll back mode setting, that means all parameter check should be placed in
    //atomic check function, and now all para is correct, we only need flush them to HW register
    //but we still add para check code here tempararily, it will be removed after code stable.

    DRM_DEBUG_KMS("crtc=%d\n", crtc->index);

    if(cur_task)
    {
        gf_info("Task [%s] set mode to crtc %d.\n", cur_task->comm, crtc->index);
    }

    gf_update_active_connector(crtc);

    flag |= UPDATE_CRTC_MODE_FLAG;

    disp_cbios_set_mode(disp_info, drm_crtc_index(crtc), mode, adj_mode, flag);
}

void  gf_crtc_helper_disable(struct drm_crtc *crtc)
{
    gf_info("CRTC disable: to turn off screen of crtc: %d\n", crtc->index);

    gf_crtc_dpms_onoff_helper(crtc, 0);

    drm_crtc_vblank_off(crtc);
}

void  gf_crtc_helper_enable(struct drm_crtc *crtc)
{
    gf_info("CRTC enable: to turn on screen of crtc: %d\n", crtc->index);

    drm_crtc_vblank_on(crtc);

    gf_crtc_dpms_onoff_helper(crtc, 1);
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
void  gf_crtc_atomic_enable(struct drm_crtc *crtc, struct drm_atomic_state *state)
#else
void  gf_crtc_atomic_enable(struct drm_crtc *crtc, struct drm_crtc_state *old_crtc_state)
#endif
{
    gf_crtc_t *gf_crtc = to_gf_crtc(crtc);

    gf_info("CRTC atomic enable: to turn on vblank and screen of crtc: %d\n", crtc->index);

    drm_crtc_vblank_on(crtc);

    gf_crtc_dpms_onoff_helper(crtc, 1);

    gf_crtc->enabled = 1;
}

#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
void  gf_crtc_atomic_disable(struct drm_crtc *crtc, struct drm_atomic_state *state)
#else
void  gf_crtc_atomic_disable(struct drm_crtc *crtc, struct drm_crtc_state *old_crtc_state)
#endif
{
    gf_crtc_t *gf_crtc = to_gf_crtc(crtc);
    u64 old_cnt;
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
    struct drm_crtc_state *old_crtc_state = drm_atomic_get_old_crtc_state(state, crtc);
#endif

    gf_info("CRTC atomic disable: to turn off vblank and screen of crtc: %d\n", crtc->index);

    //disable all planes on this crtc without flush event
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
    drm_atomic_helper_disable_planes_on_crtc(old_crtc_state, false);
#else
    drm_atomic_helper_disable_planes_on_crtc(crtc, false);
#endif

    if (0 == drm_crtc_vblank_get(crtc))
    {
        old_cnt = drm_crtc_vblank_count(crtc);

        if(0 == wait_event_timeout(crtc->dev->vblank[crtc->index].queue,
                        old_cnt != drm_crtc_vblank_count(crtc),
                        msecs_to_jiffies(40)))
        {
            gf_info("Wait vblank queue timeout after disable planes of crtc %d\n", crtc->index);
        }

        drm_crtc_vblank_put(crtc);
    }

    gf_crtc_dpms_onoff_helper(crtc, 0);

    drm_crtc_vblank_off(crtc);

    gf_crtc->enabled = 0;
}

void gf_crtc_update_lut(struct drm_crtc_state *crtc_state)
{
    struct drm_crtc *crtc = crtc_state->crtc;
    gf_crtc_t *gf_crtc = to_gf_crtc(crtc);
    struct drm_device *dev = crtc->dev;
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;
    struct drm_color_lut *lut = NULL;
    int *gamma = NULL;
    unsigned int r, g, b;
    unsigned int i = 0, lut_len = 0;

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
    lut_len = drm_color_lut_size(crtc_state->gamma_lut);
#else
    lut_len = crtc_state->gamma_lut->length / sizeof(*lut);
#endif

    if (lut_len != COLOR_LEGACY_LUT_LENGTH)
    {
        DRM_DEBUG_KMS("gamma size not supported\n");
        return;
    }

    lut = (struct drm_color_lut *)crtc_state->gamma_lut->data;

    gamma = gf_calloc(sizeof(int) * COLOR_LEGACY_LUT_LENGTH);

    if (!gamma)
    {
        DRM_ERROR("alloc gamma table failed\n");
        return;
    }

    for (i = 0; i < COLOR_LEGACY_LUT_LENGTH; i++)
    {
        r = drm_color_lut_extract(lut[i].red, 16);
        g = drm_color_lut_extract(lut[i].green, 16);
        b = drm_color_lut_extract(lut[i].blue, 16);
        gamma[i] = ((b >> 6) & 0x3FF) + ((g << 4) & 0xFFC00) + ((r << 14) & 0x3FF00000);
    }

    gf_mutex_lock(disp_info->gamma_lock);

    disp_cbios_set_gamma(disp_info, gf_crtc->pipe, gamma);

    gf_mutex_unlock(disp_info->gamma_lock);

    gf_free(gamma);
}


#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
void gf_crtc_atomic_begin(struct drm_crtc *crtc, struct drm_atomic_state *state)
#else
void gf_crtc_atomic_begin(struct drm_crtc *crtc, struct drm_crtc_state *old_crtc_state)
#endif
{
    struct drm_crtc_state *crtc_state = NULL;
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
    crtc_state = drm_atomic_get_new_crtc_state(state, crtc);
#else
    crtc_state = crtc->state;
#endif
    //do some prepare on specified crtc before update planes
    //for intel chip, it will wait until scan line is not in (vblank-100us) ~ vblank
    //will implement it later
    if (crtc_state->color_mgmt_changed ||
       drm_atomic_crtc_needs_modeset(crtc_state))
    {
        if (crtc_state->gamma_lut)
        {
            gf_crtc_update_lut(crtc_state);
        }
    }
}

static bool gf_need_wait_vblank(struct drm_crtc *crtc, struct drm_crtc_state *old_crtc_state)
{
    struct drm_plane *plane;
    struct drm_plane_state *old_plane_state;
    int i;
    bool need_wait = false;

    if ((is_crtc_work_in_splice_mode(crtc) &&
         is_splice_target_active_in_drm(crtc->dev)))
    {
        need_wait = false;
        goto End;
    }

    if(crtc->state && drm_atomic_crtc_needs_modeset(crtc->state))
    {
        need_wait = true;
        goto End;
    }

    if(!old_crtc_state || !old_crtc_state->state)
    {
        need_wait = true;
        goto End;
    }

#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
#if  DRM_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
    if(crtc->state && crtc->state->async_flip)
#else
    if(crtc->state && (crtc->state->pageflip_flags & DRM_MODE_PAGE_FLIP_ASYNC))
#endif
    {
        gf_crtc_state_t* gf_cstate = to_gf_crtc_state(crtc->state);
        if(!gf_cstate->keep_vsync)
        {
            need_wait = false;
            goto End;
        }
    }
#endif

    for(i = 0; i < crtc->dev->mode_config.num_total_plane; i++)
    {
        if(old_crtc_state->state->planes[i].ptr)
        {
            plane = old_crtc_state->state->planes[i].ptr;
            old_plane_state = old_crtc_state->state->planes[i].state;
            if(plane->state->crtc == crtc && crtc->state && (crtc->state->plane_mask & (1 << plane->index)))
            {
                if(plane->state->fb != old_plane_state->fb)
                {
                    need_wait = true;
                    break;
                }
            }
        }
    }

End:
    if(need_wait)
    {
        if(drm_crtc_vblank_get(crtc) != 0)
        {
            need_wait = false;
        }
    }

    return need_wait;
}
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
void gf_crtc_atomic_flush(struct drm_crtc *crtc, struct drm_atomic_state *state)
#else
void gf_crtc_atomic_flush(struct drm_crtc *crtc, struct drm_crtc_state *old_crtc_state)
#endif
{
    struct drm_pending_vblank_event *event = crtc->state->event;
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
   struct drm_crtc_state *old_crtc_state = drm_atomic_get_old_crtc_state(state,crtc);
#endif
    DRM_DEBUG_KMS("crtc=%d\n", crtc->index);
    //do some flush work on spcified crtc after planes update finished.
    //install vblank event(flip complete) to queue, then it will be signaled at vblank interrupt
    if (event)
    {
        crtc->state->event = NULL;

        spin_lock_irq(&crtc->dev->event_lock);
        if (gf_need_wait_vblank(crtc, old_crtc_state))
        {
            // should hold a reference for arm_vblank
            drm_crtc_arm_vblank_event(crtc, event);
        }
        else
        {
            drm_crtc_send_vblank_event(crtc, event);
        }
        spin_unlock_irq(&crtc->dev->event_lock);
    }
}

#else

int gf_crtc_cursor_set(struct drm_crtc *crtc,
                        struct drm_file *file,
                        uint32_t handle,
                        uint32_t width, uint32_t height)
{
    struct drm_device *dev = crtc->dev;
    struct drm_gem_object* obj = NULL;
    struct drm_gf_gem_object* cursor_bo = NULL;
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;
    gf_crtc_t *gf_crtc = to_gf_crtc(crtc);
    gf_cursor_update_t arg = {0, };
    gf_map_argu_t  map = {0};
    int ret;

    arg.crtc        = to_gf_crtc(crtc)->pipe;

/* if we want to turn off the cursor ignore width and height */
    if (!handle)
    {
        DRM_DEBUG_KMS("cursor off\n");

        if (gf_crtc->cursor_bo)
        {
            gf_gem_object_put(gf_crtc->cursor_bo);
            gf_crtc->cursor_bo = NULL;
        }

        goto  Finish;
    }

#if DRM_VERSION_CODE < KERNEL_VERSION(4,7,0) && !defined PHYTIUM_2000
    obj = drm_gem_object_lookup(dev, file, handle);
#else
    obj = drm_gem_object_lookup(file, handle);
#endif

    if (obj == NULL)
    {
        return -ENOENT;
    }
    cursor_bo = to_gf_bo(obj);

    if ((width != 64 || height != 64) &&
        (width != 128 || height != 128))
    {
        gf_gem_object_put(cursor_bo);
        DRM_ERROR("wrong cursor size, width=%d height=%d!\n", width, height);
        return  -EINVAL;
    }

    if (obj->size < width * height * 4)
    {
        gf_gem_object_put(cursor_bo);
        DRM_ERROR("buffer is to small\n");
        return  -ENOMEM;
    }

    if (gf_crtc->cursor_bo)
    {
        gf_gem_object_put(gf_crtc->cursor_bo);
    }

    gf_crtc->cursor_bo= cursor_bo;
    gf_crtc->cursor_w = width;
    gf_crtc->cursor_h = height;

    arg.vsync_on = 0;
    arg.pos_x   = gf_crtc->cursor_x;
    arg.pos_y   = gf_crtc->cursor_y;
    arg.width   = gf_crtc->cursor_w;
    arg.height  = gf_crtc->cursor_h;
    arg.bo      = cursor_bo;

Finish:
    return disp_cbios_update_cursor(disp_info, &arg);
}

int gf_crtc_cursor_move(struct drm_crtc *crtc, int x, int y)
{
    gf_crtc_t *gf_crtc = to_gf_crtc(crtc);
    struct drm_device *dev = crtc->dev;
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;
    gf_cursor_update_t arg = {0, };

    gf_crtc->cursor_x = x;
    gf_crtc->cursor_y = y;

    arg.crtc        = to_gf_crtc(crtc)->pipe;
    arg.vsync_on    = 0;
    arg.pos_x       = gf_crtc->cursor_x;
    arg.pos_y       = gf_crtc->cursor_y;
    arg.width       = gf_crtc->cursor_w;
    arg.height      = gf_crtc->cursor_h;
    arg.bo          = gf_crtc->cursor_bo;

    return disp_cbios_update_cursor(disp_info, &arg);
}
#ifdef PHYTIUM_2000
int gf_crtc_gamma_set(struct drm_crtc *crtc, u16 *red, u16 *green,
                      u16 *blue, uint32_t size)
#else
void gf_crtc_gamma_set(struct drm_crtc *crtc, u16 *red, u16 *green,
                       u16 *blue, uint32_t start, uint32_t size)
#endif
{
    int i = 0;
#ifdef PHYTIUM_2000
    uint32_t start = 0;
#endif
    int end = (start + size > 256) ? 256 : start + size;
    gf_crtc_t *gf_crtc = to_gf_crtc(crtc);
    struct drm_device *dev = crtc->dev;
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;

    for (i = start; i < end; i++) {
        gf_crtc->lut_entry[i] = (blue[i] >> 6) | ((green[i] << 4) & 0xFFC00) | ((red[i] << 14) & 0x3FF00000);
    }

    gf_mutex_lock(disp_info->gamma_lock);

    disp_cbios_set_gamma(disp_info, gf_crtc->pipe, gf_crtc->lut_entry);

    gf_mutex_unlock(disp_info->gamma_lock);

#ifdef PHYTIUM_2000
     return 0;
#endif
}

static void gf_swap_changed_encoder_crtc(struct drm_device *dev, bool update_old, bool clear_new)
{
    struct drm_crtc*  crtc = NULL;
    struct drm_connector  *connector = NULL;
    struct drm_encoder  *encoder = NULL, *temp_enc = NULL;
    gf_connector_t* gf_connector= NULL;
    gf_encoder_t*  gf_encoder = NULL;

    list_for_each_entry(connector, &dev->mode_config.connector_list, head)
    {
        gf_connector = to_gf_connector(connector);
        temp_enc = connector->encoder;

        if(update_old)
        {
            if(gf_connector->new_encoder)
            {
                connector->encoder = &gf_connector->new_encoder->base_encoder;
            }
            else
            {
                connector->encoder = NULL;
            }
        }

        if(temp_enc && !clear_new)
        {
            gf_connector->new_encoder = to_gf_encoder(temp_enc);
        }
        else
        {
            gf_connector->new_encoder = NULL;
        }
    }

    list_for_each_entry(encoder, &dev->mode_config.encoder_list, head)
    {
        gf_encoder = to_gf_encoder(encoder);
        crtc = encoder->crtc;

        if(update_old)
        {
            if(gf_encoder->new_crtc)
            {
                encoder->crtc = &gf_encoder->new_crtc->base_crtc;
            }
            else
            {
                encoder->crtc = NULL;
            }
        }

        if(crtc && !clear_new)
        {
            gf_encoder->new_crtc = to_gf_crtc(crtc);
        }
        else
        {
            gf_encoder->new_crtc = NULL;
        }
    }
}

static void gf_init_new_encoder_crtc(struct drm_device *dev)
{
    struct drm_connector  *connector = NULL;
    struct drm_encoder  *encoder = NULL;
    gf_connector_t* gf_connector= NULL;
    gf_encoder_t*  gf_encoder = NULL;

    list_for_each_entry(connector, &dev->mode_config.connector_list, head)
    {
        gf_connector = to_gf_connector(connector);

        gf_connector->new_encoder = NULL;
    }

    list_for_each_entry(encoder, &dev->mode_config.encoder_list, head)
    {
        gf_encoder = to_gf_encoder(encoder);

        gf_encoder->new_crtc = NULL;
    }
}

static void gf_drm_disable_unused_functions(struct drm_device *dev)
{
    struct drm_encoder *encoder;
    struct drm_crtc *crtc;
    struct drm_connector* connector;
    const struct drm_encoder_helper_funcs *encoder_funcs;
    const struct drm_crtc_helper_funcs *crtc_funcs;
    struct drm_framebuffer *old_fb;

    drm_warn_on_modeset_not_all_locked(dev);

    list_for_each_entry(connector, &dev->mode_config.connector_list, head)
    {
        if (connector->encoder && (connector->status == connector_status_disconnected))
        {
            connector->encoder = NULL;
        }
    }

    list_for_each_entry(encoder, &dev->mode_config.encoder_list, head)
    {
        encoder_funcs = encoder->helper_private;
        if (!drm_helper_encoder_in_use(encoder))
        {
            if (encoder_funcs->disable)
            {
                (*encoder_funcs->disable)(encoder);
            }
            else
            {
                (*encoder_funcs->dpms)(encoder, DRM_MODE_DPMS_OFF);
            }

            if (encoder->bridge)
            {
                encoder->bridge->funcs->disable(encoder->bridge);
            }
        }
    }

    list_for_each_entry(crtc, &dev->mode_config.crtc_list, head)
    {
        crtc_funcs = crtc->helper_private;

        crtc->enabled = drm_helper_crtc_in_use(crtc);
        if (!crtc->enabled)
        {
            if (crtc_funcs->disable)
            {
                (*crtc_funcs->disable)(crtc);
            }
            else
            {
                (*crtc_funcs->dpms)(crtc, DRM_MODE_DPMS_OFF);
            }

            old_fb = drm_get_crtc_primary_fb(crtc);

            drm_set_crtc_primary_fb(crtc, NULL);

            if(crtc_funcs->mode_set_base && old_fb != NULL)
            {
                (*crtc_funcs->mode_set_base)(crtc, 0, 0, old_fb);
            }
        }
    }
}

static void gf_drm_crtc_disable_helper(struct drm_crtc *crtc)
{
    struct drm_device *dev = crtc->dev;
    struct drm_connector *connector;
    struct drm_encoder *encoder;

    /* Decouple all encoders and their attached connectors from this crtc */
    list_for_each_entry(encoder, &dev->mode_config.encoder_list, head)
    {
        if (encoder->crtc != crtc)
        {
            continue;
        }

        encoder->crtc = NULL;

        list_for_each_entry(connector, &dev->mode_config.connector_list, head)
        {
            if (connector->encoder != encoder)
            {
                continue;
            }
            connector->encoder = NULL;
        }
    }
    gf_drm_disable_unused_functions(dev);
}

int gf_crtc_helper_set_config(struct drm_mode_set *set)
{
    struct drm_device *dev;
    struct drm_crtc *crtc = NULL;
    struct drm_framebuffer *old_fb = NULL;
    struct drm_connector  *connector = NULL;
    struct drm_encoder*  encoder = NULL;
    gf_connector_t* gf_connector= NULL;
    bool mode_changed = FALSE, fb_changed = FALSE;
    bool pre_swap = TRUE;
    const struct drm_crtc_helper_funcs *crtc_funcs;
    struct drm_mode_set save_set;
    int ret = 0, ro = 0, conn_masks = 0;

    if(!set || !set->crtc)
    {
        return -EINVAL;
    }

    if(!set->crtc->helper_private)
    {
        return -EINVAL;
    }

    crtc_funcs = set->crtc->helper_private;

    if(!set->mode)
    {
        set->fb = NULL;
    }

    if(!set->fb)
    {
        gf_drm_crtc_disable_helper(set->crtc);
        return 0;
    }

    dev = set->crtc->dev;

    save_set.crtc = set->crtc;
    save_set.mode = &set->crtc->mode;
    save_set.x = set->crtc->x;
    save_set.y = set->crtc->y;
    save_set.fb = drm_get_crtc_primary_fb(set->crtc);

    gf_init_new_encoder_crtc(dev);

    /* We should be able to check here if the fb has the same properties
    * and then just flip_or_move it */
    if(drm_get_crtc_primary_fb(set->crtc) != set->fb)
    {
        /* If we have no fb then treat it as a full mode set */
        if(drm_get_crtc_primary_fb(set->crtc) == NULL)
        {
            DRM_DEBUG_KMS("crtc has no fb, full mode set\n");
            mode_changed = TRUE;
        }
        else if(set->fb == NULL)
        {
            mode_changed = TRUE;
        }
        else
        {
            fb_changed = TRUE;
        }
    }

    if(set->x != set->crtc->x || set->y != set->crtc->y)
    {
        fb_changed = TRUE;
    }

    if(set->mode && !drm_mode_equal(set->mode, &set->crtc->mode))
    {
        drm_mode_debug_printmodeline(set->mode);
        mode_changed = TRUE;
    }

    for(ro = 0; ro < set->num_connectors; ro++)
    {
        if(set->connectors[ro])
        {
            conn_masks |= to_gf_connector(set->connectors[ro])->output_type;
        }
    }

    list_for_each_entry(connector, &dev->mode_config.connector_list, head)
    {
        const struct drm_connector_helper_funcs *connector_funcs = connector->helper_private;
        gf_connector = to_gf_connector(connector);

        if(gf_connector->output_type & conn_masks)
        {
            encoder = connector_funcs->best_encoder(connector);
            gf_connector->new_encoder = to_gf_encoder(encoder);
        }
        else
        {
            if(connector->encoder && connector->encoder->crtc != set->crtc)
            {
                gf_connector->new_encoder = to_gf_encoder(connector->encoder);
            }
            else
            {
                gf_connector->new_encoder = NULL;
            }
        }

        if (gf_connector->new_encoder && (&gf_connector->new_encoder->base_encoder != connector->encoder))
        {
            mode_changed = TRUE;
        }
    }

    list_for_each_entry(connector, &dev->mode_config.connector_list, head)
    {
        gf_connector = to_gf_connector(connector);
        if(gf_connector->new_encoder)
        {
            crtc = gf_connector->new_encoder->base_encoder.crtc;

            if(gf_connector->output_type & conn_masks)
            {
                crtc = set->crtc;
            }

            /* Make sure the new CRTC will work with the encoder */
            if(crtc && !drm_encoder_crtc_ok(&gf_connector->new_encoder->base_encoder, crtc))
            {
                ret = -EINVAL;
                goto  Fail;
            }

            if(crtc != gf_connector->new_encoder->base_encoder.crtc)
            {
                mode_changed = TRUE;
            }

            gf_connector->new_encoder->new_crtc = (crtc)? to_gf_crtc(crtc) : NULL;

            if(&gf_connector->new_encoder->base_encoder != connector->encoder)
            {
                if(connector->encoder)
                {
                    to_gf_encoder(connector->encoder)->new_crtc = NULL;
                }
            }
        }
        else if(connector->encoder)
        {
            to_gf_encoder(connector->encoder)->new_crtc = NULL;
            mode_changed = TRUE;
        }
    }

    //swap old state and new state
    gf_swap_changed_encoder_crtc(dev, TRUE, FALSE);

    pre_swap = FALSE;

    if (mode_changed)
    {
        if(drm_helper_crtc_in_use(set->crtc))
        {
            drm_mode_debug_printmodeline(set->mode);

            old_fb = drm_get_crtc_primary_fb(set->crtc);
            drm_set_crtc_primary_fb(set->crtc, set->fb);

            if(!drm_crtc_helper_set_mode(set->crtc, set->mode, set->x, set->y, old_fb))
            {
                DRM_ERROR("failed to set mode on [CRTC:%d]\n", set->crtc->base.id);
                drm_set_crtc_primary_fb(set->crtc, old_fb);
                ret = -EINVAL;
                goto Fail;
            }
        }

        gf_drm_disable_unused_functions(dev);
    }
    else if(fb_changed)
    {
        old_fb = drm_get_crtc_primary_fb(set->crtc);
        drm_set_crtc_primary_fb(set->crtc, set->fb);

        ret = crtc_funcs->mode_set_base(set->crtc, set->x, set->y, old_fb);
        if (ret != 0)
        {
            drm_set_crtc_primary_fb(set->crtc, old_fb);
            ret = -EINVAL;
            goto Fail;
        }
    }

    gf_swap_changed_encoder_crtc(dev, FALSE, TRUE);

    return 0;

Fail:
    if(pre_swap)
    {
        gf_swap_changed_encoder_crtc(dev, FALSE, TRUE);
    }
    else
    {
        gf_swap_changed_encoder_crtc(dev, TRUE, TRUE);
        /* Try to restore the config */
        if (mode_changed)
        {
            drm_crtc_helper_set_mode(save_set.crtc, save_set.mode, save_set.x, save_set.y, save_set.fb);
        }
        else if(fb_changed)
        {
            crtc_funcs->mode_set_base(save_set.crtc, save_set.x, save_set.y, save_set.fb);
        }
    }

    return ret;
}

void  gf_crtc_prepare(struct drm_crtc *crtc)
{
    struct drm_device* dev = crtc->dev;
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;
    gf_crtc_t* gf_crtc = to_gf_crtc(crtc);
    struct  drm_encoder*  encoder = NULL;
    gf_encoder_t*  gf_encoder = NULL;
    unsigned int   active[MAX_CORE_CRTCS];

    gf_crtc_dpms_onoff_helper(crtc, 0);

    gf_memcpy(active, disp_info->active_output, sizeof(active));

    active[gf_crtc->pipe] = 0;

    list_for_each_entry(encoder, &dev->mode_config.encoder_list, head)
    {
        gf_encoder = to_gf_encoder(encoder);
        if(encoder->crtc == crtc)
        {
            active[gf_crtc->pipe] |= gf_encoder->output_type;
        }
    }

    if(!disp_cbios_update_output_active(disp_info, active))
    {
        gf_memcpy(disp_info->active_output, active, sizeof(active));
    }
}

void gf_crtc_disable(struct drm_crtc *crtc)
{
    gf_crtc_dpms_onoff_helper(crtc, 0);
}

void  gf_crtc_commit(struct drm_crtc *crtc)
{
    gf_crtc_dpms_onoff_helper(crtc, 1);
}

bool  gf_crtc_mode_fixup(struct drm_crtc *crtc,
                         const struct drm_display_mode *mode,
                         struct drm_display_mode *adjusted_mode)
{
    return  TRUE;
}

int  gf_crtc_mode_set(struct drm_crtc *crtc, struct drm_display_mode *mode,
                      struct drm_display_mode *adjusted_mode, int x, int y,
                      struct drm_framebuffer *old_fb)
{
    struct drm_device* dev = crtc->dev;
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;
    gf_crtc_t* gf_crtc = to_gf_crtc(crtc);
    gf_crtc_flip_t arg = {0};
    int flag = UPDATE_CRTC_MODE_FLAG;

    disp_cbios_set_mode(disp_info, gf_crtc->pipe, mode, adjusted_mode, flag);

    arg.crtc = to_gf_crtc(crtc)->pipe;
    arg.stream_type = GF_PLANE_PS;
    arg.fb = drm_get_crtc_primary_fb(crtc);
    arg.crtc_x = 0;
    arg.crtc_y = 0;
    arg.crtc_w = mode->hdisplay;
    arg.crtc_h = mode->vdisplay;
    arg.src_x = x;
    arg.src_y = y;
    arg.src_w = mode->hdisplay;
    arg.src_h = mode->vdisplay;

    return disp_cbios_crtc_flip(disp_info, &arg);
}

int  gf_crtc_mode_set_base(struct drm_crtc *crtc, int x, int y,
                           struct drm_framebuffer *old_fb)
{
    struct drm_device* dev = crtc->dev;
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;
    gf_crtc_t* gf_crtc = to_gf_crtc(crtc);
    gf_crtc_flip_t arg = {0};
    int  ret = 0;

    arg.crtc = to_gf_crtc(crtc)->pipe;
    arg.stream_type = GF_PLANE_PS;
    arg.fb = drm_get_crtc_primary_fb(crtc);
    arg.crtc_x = 0;
    arg.crtc_y = 0;
    arg.crtc_w = crtc->mode.hdisplay;
    arg.crtc_h = crtc->mode.vdisplay;
    arg.src_x = x;
    arg.src_y = y;
    arg.src_w = crtc->mode.hdisplay;
    arg.src_h = crtc->mode.vdisplay;

    ret = disp_cbios_crtc_flip(disp_info, &arg);
    if(!ret)
    {
        crtc->x = x;
        crtc->y = y;
    }

    return  ret;
}

struct gf_flip_work
{
    struct work_struct work;
    struct drm_pending_vblank_event *event;
    struct drm_crtc *crtc;
    struct drm_framebuffer *old_fb;
    int stream_type;
    dma_fence_t *fence;
};

static void gf_crtc_flip_work_func(struct work_struct *w)
{
    struct gf_flip_work *work = container_of(w, struct gf_flip_work, work);
    struct drm_crtc *crtc = work->crtc;
    struct drm_gf_framebuffer *fb = to_gfb(drm_get_crtc_primary_fb(crtc));
    gf_card_t *gf = crtc->dev->dev_private;
    u32 vblank_count;
    gf_crtc_flip_t arg = {0};
    int ret;

    // paging
    gf_core_interface->prepare_and_mark_unpagable(gf->adapter, fb->obj->core_handle, &fb->obj->info);

    // wait fence
    if (work->fence)
    {
        dma_fence_wait(work->fence, FALSE);
        dma_fence_put(work->fence);
        work->fence = NULL;
    }

    arg.crtc = to_gf_crtc(crtc)->pipe;
    arg.stream_type = work->stream_type;
    arg.fb = &fb->base;
    arg.crtc_x = 0;
    arg.crtc_y = 0;
    arg.crtc_w = crtc->mode.hdisplay;
    arg.crtc_h = crtc->mode.vdisplay;
    arg.src_x = crtc->x;
    arg.src_y = crtc->y;
    arg.src_w = crtc->mode.hdisplay;
    arg.src_h = crtc->mode.vdisplay;

    disp_cbios_crtc_flip(gf->disp_info, &arg);

    // wait vblank
    vblank_count = drm_crtc_vblank_count(crtc);
    ret = wait_event_timeout(crtc->dev->vblank[drm_get_crtc_index(crtc)].queue,
            vblank_count != drm_crtc_vblank_count(crtc),
            msecs_to_jiffies(50));
    drm_crtc_vblank_put(crtc);

    // complete
    gf_core_interface->mark_pagable(gf->adapter, to_gfb(work->old_fb)->obj->core_handle);
    drm_framebuffer_unreference(work->old_fb);
    work->old_fb = NULL;

    spin_lock_irq(&crtc->dev->event_lock);
    to_gf_crtc(crtc)->flip_work = NULL;
    if (work->event)
    {
        drm_crtc_send_vblank_event(crtc, work->event);
    }
    spin_unlock_irq(&crtc->dev->event_lock);

    // mark pageable
    gf_free(work);
}

int gf_crtc_page_flip(struct drm_crtc *crtc, struct drm_framebuffer *fb, struct drm_pending_vblank_event *event, uint32_t flags)
{
    int ret;
    struct drm_plane *plane = crtc->primary;
    struct drm_framebuffer *old_fb = drm_get_crtc_primary_fb(crtc);
    struct drm_device* dev = crtc->dev;
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;
    gf_crtc_t *gf_crtc = to_gf_crtc(crtc);
    struct gf_flip_work *work;

    if (!old_fb || !to_gfb(old_fb)->obj)
        return -EBUSY;

    if (fb->pixel_format != crtc->primary->fb->pixel_format)
        return -EINVAL;

    work = gf_calloc(sizeof(*work));
    if (!work)
        return -ENOMEM;

    work->event = event;
    work->crtc = crtc;
    work->old_fb = old_fb;
    work->stream_type = GF_STREAM_PS;
    ret = drm_crtc_vblank_get(crtc);
    if (ret)
        goto free_work;

    spin_lock_irq(&crtc->dev->event_lock);
    if (gf_crtc->flip_work)
    {
        DRM_DEBUG_DRIVER("flip queue: crtc already busy\n");
        spin_unlock_irq(&crtc->dev->event_lock);

        drm_crtc_vblank_put(crtc);
        gf_free(work);
        return -EBUSY;
    }
    gf_crtc->flip_work = work;
    spin_unlock_irq(&crtc->dev->event_lock);

    work->fence = gf_reservation_object_get_excl_rcu(to_gfb(fb)->obj->resv);
    drm_framebuffer_reference(work->old_fb);
    drm_set_crtc_primary_fb(crtc, fb);

    INIT_WORK(&work->work, gf_crtc_flip_work_func);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35)
    queue_work(system_unbound_wq, &work->work);
#else
    queue_work(disp_info->wq, &work->work);
#endif

    return 0;
free_work:
    gf_free(work);
    return ret;
}
#endif
