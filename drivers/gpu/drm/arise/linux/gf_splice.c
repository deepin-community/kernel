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
#include "gf.h"
#include "gf_disp.h"
#include "gf_cbios.h"
#include "gf_crtc.h"
#include "gf_kms.h"
#include "gf_drmfb.h"
#include "gf_fence.h"
#include "gf_irq.h"
#include "gf_trace.h"
#include "gf_splice.h"

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 13, 0)
/* get_scanout_position() return flags */
#define DRM_SCANOUTPOS_VALID        (1 << 0)
#define DRM_SCANOUTPOS_IN_VBLANK    (1 << 1)
#define DRM_SCANOUTPOS_ACCURATE     (1 << 2)
#endif

static void gf_splice_encoder_enable(struct drm_encoder *encoder);
static void gf_splice_encoder_disable(struct drm_encoder *encoder);

static const int splice_plane_formats[] = {
    DRM_FORMAT_C8,
    DRM_FORMAT_RGB565,
    DRM_FORMAT_XRGB8888,
    DRM_FORMAT_XBGR8888,
    DRM_FORMAT_ARGB8888,
    DRM_FORMAT_ABGR8888,
    DRM_FORMAT_XRGB2101010,
    DRM_FORMAT_XBGR2101010,
    DRM_FORMAT_ARGB2101010,
    DRM_FORMAT_ABGR2101010,
    DRM_FORMAT_YUYV,
    DRM_FORMAT_YVYU,
    DRM_FORMAT_UYVY,
    DRM_FORMAT_VYUY,
    DRM_FORMAT_AYUV,
};

static const int splice_cursor_formats[] = {
    DRM_FORMAT_XRGB8888,
    DRM_FORMAT_ARGB8888,
};

struct drm_crtc* gf_splice_get_crtc_by_source(struct drm_device *dev, gf_splice_source_t *source)
{
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    pipe_t pipe = 0xFFFFFFFF;
    disp_output_type output = source->output_type;
    int crtc_mask = disp_cbios_get_crtc_mask(disp_info, output);
    struct drm_crtc *crtc = NULL;

    list_for_each_entry(crtc, &(dev->mode_config.crtc_list), head)
    {
        pipe = drm_get_crtc_index(crtc);
        if ((1 << pipe) & crtc_mask)
        {
            return crtc;
        }
    }

    return NULL;
}


gf_splice_source_t* gf_splice_get_source_by_connector(gf_splice_manager_t *splice_manager, struct drm_connector *connector)
{
    int i = 0;

    for (i = 0; i < splice_manager->source_num; i++)
    {
        gf_splice_source_t *source = &(splice_manager->sources[i]);
        if (source->connector == connector)
        {
            return source;
        }
    }

    return NULL;
}

bool is_connector_work_in_splice_mode(struct drm_connector *connector)
{
    struct drm_device *dev = connector->dev;
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_splice_manager_t *splice_manager = disp_info->splice_manager;
    gf_splice_source_t *splice_source = gf_splice_get_source_by_connector(splice_manager, connector);
    gf_splice_source_cfg_t *src_config = NULL;

    if (splice_source == NULL)
    {
        DRM_ERROR("can not find source of connector 0x%x", to_gf_connector(connector)->output_type);

        return FALSE;
    }

    src_config = &(splice_source->config);

    if (splice_manager->splice_enable && src_config->enable)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static enum drm_connector_status gf_splice_connector_detect(struct drm_connector *connector, bool force)
{
    //TOOD: iter the source connector list
    struct drm_device *dev = connector->dev;
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_splice_manager_t *splice_manager = disp_info->splice_manager;

    if (splice_manager->splice_enable)
    {
        return connector_status_connected;
    }
    else
    {
        return connector_status_disconnected;
    }
}

bool is_splice_source_active_in_drm(struct drm_device *dev, gf_splice_source_t *source)
{
    struct drm_crtc *source_crtc = gf_splice_get_crtc_by_source(dev, source);

    if (to_gf_crtc(source_crtc)->enabled)
    {
        return TRUE;
    }

    return FALSE;
}

bool is_crtc_work_in_splice_mode(struct drm_crtc *crtc)
{
    struct drm_device *dev = crtc->dev;
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_splice_manager_t *splice_manager = disp_info->splice_manager;
    int i = 0;
    bool ret = FALSE;

    for (i = 0; i < splice_manager->source_num; i++)
    {
        gf_splice_source_t *source = &(splice_manager->sources[i]);
        gf_splice_source_cfg_t *src_config = &(source->config);
        struct drm_crtc *source_crtc = NULL;

        if (!src_config->enable)
        {
            continue;
        }

        source_crtc = gf_splice_get_crtc_by_source(dev, source);
        if (source_crtc == crtc && splice_manager->splice_enable == 1)
        {
           ret = TRUE;
           break;
        }
    }

    return ret;
}

bool is_plane_work_in_splice_mode(struct drm_plane *plane)
{
    struct drm_device *dev = plane->dev;
    struct drm_crtc *crtc = plane->state->crtc;

    if (!crtc)
    {
        return FALSE;
    }

    return is_crtc_work_in_splice_mode(crtc);
}

bool is_splice_target_active_in_drm(struct drm_device *dev)
{
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_splice_manager_t *splice_manager = disp_info->splice_manager;
    struct drm_crtc *target_crtc = splice_manager->target.crtc;

    if (to_gf_crtc(target_crtc)->enabled)
    {
        return TRUE;
    }

    return FALSE;
}

static int gf_splice_connector_dpms(struct drm_connector *connector, int mode)
{
    //TOOD: iter the source connector and do dpms
    if (connector->encoder)
    {
        if (mode == DRM_MODE_DPMS_ON)
        {
            gf_splice_encoder_enable(connector->encoder);
        }
        else
        {
            gf_splice_encoder_disable(connector->encoder);
        }

        connector->dpms = mode;
    }

    return 0;
}

static int  gf_splice_connector_fill_modes(struct drm_connector *connector,
                                           uint32_t maxX, uint32_t maxY)
{
    return  0;
}

static void gf_splice_connector_destroy(struct drm_connector *connector)
{
    gf_connector_t *gf_connector = to_gf_connector(connector);

    drm_connector_unregister(connector);
    drm_connector_cleanup(connector);

    gf_destroy_mutex(gf_connector->conn_mutex);
    gf_free(gf_connector);
}

struct drm_connector_state* gf_splice_connector_duplicate_state(struct drm_connector *connector)
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

static int gf_splice_connector_atomic_set_property(struct drm_connector *connector,
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
        DRM_WARN("Invalid driver-private property '%s'\n", property->name);
    }

    return ret;
}

static int gf_splice_connector_atomic_get_property(struct drm_connector *connector,
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
        DRM_WARN("Invalid driver-private property '%s'\n", property->name);
    }

    return ret;
}

void gf_splice_connector_destroy_state(struct drm_connector *connector, struct drm_connector_state *state)
{
    gf_connector_state_t *gf_conn_state = to_gf_conn_state(state);
    __drm_atomic_helper_connector_destroy_state(state);
    gf_free(gf_conn_state);
}

static int gf_splice_connector_get_modes(struct drm_connector *connector)
{
    struct drm_device *dev = connector->dev;
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_splice_manager_t* manager = disp_info->splice_manager;
    gf_splice_target_cfg_t* target_config = &(manager->target.config);

    struct drm_display_mode *mode;

    //to get splice conector modes.
    if (!manager->splice_enable)
    {
        return 0;
    }

    mode = drm_cvt_mode(dev, target_config->mode_x, target_config->mode_y, target_config->mode_rate/100, false, false, false);
    if (!mode) {
        DRM_ERROR("Failed to create a new display mode\n");
        return 0;
    }

    drm_mode_set_name(mode);
    drm_mode_probed_add(connector, mode);

    return 1;
}

static const struct drm_connector_funcs gf_splice_connector_funcs = {
    .detect = gf_splice_connector_detect,
    .dpms = gf_splice_connector_dpms,
    .fill_modes = drm_helper_probe_single_connector_modes,
    .destroy = gf_splice_connector_destroy,
    .atomic_destroy_state = gf_splice_connector_destroy_state,
    .atomic_duplicate_state = gf_splice_connector_duplicate_state,
    .atomic_set_property = gf_splice_connector_atomic_set_property,
    .atomic_get_property = gf_splice_connector_atomic_get_property,
};

static const struct drm_connector_helper_funcs gf_splice_connector_helper_funcs = {
    .get_modes = gf_splice_connector_get_modes,
};

struct drm_connector* gf_splice_connector_init(gf_splice_manager_t* splice_manager)
{
    disp_info_t *disp_info = (disp_info_t*) splice_manager->private;
    gf_card_t*  gf_card = disp_info->gf_card;
    struct drm_device*  drm = gf_card->drm_dev;
    struct drm_connector*  connector = NULL;
    gf_connector_t* gf_connector = NULL;
    gf_connector_state_t* gf_conn_state = NULL;

    gf_connector = gf_calloc(sizeof(gf_connector_t));

    gf_conn_state = gf_calloc(sizeof(gf_connector_state_t));
    if (!gf_conn_state)
    {
        if (gf_connector)
        {
            gf_free(gf_connector);
        }
        return NULL;
    }

    gf_connector->base_connector.state = &gf_conn_state->base_conn_state;
    gf_conn_state->base_conn_state.connector = &gf_connector->base_connector;

    connector = &gf_connector->base_connector;

    connector->stereo_allowed = FALSE;
    connector->interlace_allowed = FALSE;

    gf_connector->conn_mutex = gf_create_mutex();
    gf_connector->output_type = DISP_OUTPUT_SPLICE;

    drm_connector_init(drm, connector, &gf_splice_connector_funcs, DRM_MODE_CONNECTOR_HDMIA);
    drm_connector_helper_add(connector, &gf_splice_connector_helper_funcs);

    connector->polled = DRM_CONNECTOR_POLL_HPD;

    gf_splice_attach_connector_property(connector);

    return connector;
}

static void gf_splice_encoder_destroy(struct drm_encoder *encoder)
{
    gf_encoder_t *gf_encoder = to_gf_encoder(encoder);

    drm_encoder_cleanup(encoder);

    gf_free(gf_encoder);
}

static void gf_splice_encoder_disable(struct drm_encoder *encoder)
{
    struct drm_device *dev = encoder->dev;
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_splice_manager_t *splice_manager = disp_info->splice_manager;
    gf_encoder_t *gf_encoder = to_gf_encoder(encoder);
    int i = 0;

    if (gf_encoder->enc_dpms == GF_DPMS_OFF)
    {
        return;
    }

    for (i = 0; i < splice_manager->source_num; i++)
    {
        gf_splice_source_t *source = &(splice_manager->sources[i]);
        gf_splice_source_cfg_t *src_config = &(source->config);
        gf_connector_t *gf_connector = NULL;

        if (!src_config->enable)
        {
            continue;
        }

        gf_connector = to_gf_connector(source->connector);
        if (is_splice_source_active_in_drm(dev, source) && splice_manager->splice_enable == 0)
        {
            continue;
        }
        disp_cbios_set_hdac_connect_status(disp_info, gf_connector->output_type, FALSE, FALSE);

        gf_usleep_range(1000, 1100); //delay 1 ms

        disp_cbios_set_dpms(disp_info, gf_connector->output_type, GF_DPMS_OFF);
    }

    gf_encoder->enc_dpms = GF_DPMS_OFF;
}

static void gf_splice_encoder_enable(struct drm_encoder *encoder)
{
    struct drm_device *dev = encoder->dev;
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_splice_manager_t *splice_manager = disp_info->splice_manager;
    gf_encoder_t *gf_encoder = to_gf_encoder(encoder);
    int i = 0;

    if (gf_encoder->enc_dpms == GF_DPMS_ON)
    {
        return;
    }

    for (i = 0; i < splice_manager->source_num; i++)
    {
        gf_splice_source_t *source = &(splice_manager->sources[i]);
        gf_splice_source_cfg_t *src_config = &(source->config);
        gf_connector_t *gf_connector = NULL;

        if (!src_config->enable)
        {
            continue;
        }

        gf_connector = to_gf_connector(source->connector);
        disp_cbios_set_dpms(disp_info, gf_connector->output_type, GF_DPMS_ON);

        if (gf_connector->support_audio)
        {
            disp_cbios_set_hdac_connect_status(disp_info, gf_connector->output_type, TRUE, TRUE);
        }
    }

    gf_encoder->enc_dpms = GF_DPMS_ON;
}

static bool gf_splice_encoder_mode_fixup(struct drm_encoder *encoder,
                                   const struct drm_display_mode *mode,
                                   struct drm_display_mode *adjusted_mode)
{

    //TODO:

    return TRUE;
}

static void gf_splice_encoder_atomic_mode_set(struct drm_encoder *encoder,
                                struct drm_crtc_state *crtc_state,
                                struct drm_connector_state *conn_state)
{
    struct drm_device *dev = encoder->dev;
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t* disp_info = (disp_info_t *)gf_card->disp_info;
    gf_splice_manager_t *manager = disp_info->splice_manager;
    int flag = 0;
    int i  = 0;

    flag |= UPDATE_ENCODER_MODE_FLAG;

    for (i = 0; i < manager->source_num; i++)
    {
        gf_splice_source_t *source = &(manager->sources[i]);
        gf_splice_source_cfg_t *src_config = &(source->config);
        struct drm_crtc *splice_crtc = NULL;

        if (!src_config->enable)
        {
            continue;
        }

        splice_crtc = gf_splice_get_crtc_by_source(dev, source);

        disp_cbios_set_mode(disp_info, drm_get_crtc_index(splice_crtc), &(source->config.drm_mode), &(source->config.hw_mode), flag);
    }
}


static const struct drm_encoder_funcs gf_splice_encoder_funcs =
{
    .destroy = gf_splice_encoder_destroy,
};

static const struct drm_encoder_helper_funcs gf_splice_encoder_helper_funcs =
{
    .disable = gf_splice_encoder_disable,
    .enable = gf_splice_encoder_enable,
    .mode_fixup = gf_splice_encoder_mode_fixup,
    .atomic_mode_set = gf_splice_encoder_atomic_mode_set,
};

struct drm_encoder* gf_splice_encoder_init(gf_splice_manager_t *splice_manager)
{
    disp_info_t *disp_info = (disp_info_t*) splice_manager->private;
    gf_card_t*  gf_card = disp_info->gf_card;
    struct drm_device* drm = gf_card->drm_dev;
    struct drm_encoder* encoder = NULL;
    gf_encoder_t* gf_encoder = NULL;
    unsigned int target_crtc_index = splice_manager->target.crtc_index;

    gf_encoder = gf_calloc(sizeof(gf_encoder_t));

    if (!gf_encoder)
    {
        return NULL;
    }

    encoder = &gf_encoder->base_encoder;
    encoder->possible_clones = 0;
    encoder->possible_crtcs = 1 << target_crtc_index;

    drm_encoder_init(drm, encoder, &gf_splice_encoder_funcs, DRM_MODE_ENCODER_DAC, NULL);
    drm_encoder_helper_add(encoder, &gf_splice_encoder_helper_funcs);

    gf_encoder->output_type = DISP_OUTPUT_SPLICE;
    gf_encoder->enc_dpms = GF_DPMS_OFF;

    return encoder;
}

static int gf_splice_update_plane(struct drm_plane *plane,
                   struct drm_crtc *crtc,
                   struct drm_framebuffer *fb,
                   int crtc_x, int crtc_y,
                   unsigned int crtc_w, unsigned int crtc_h,
                   uint32_t src_x, uint32_t src_y,
                   uint32_t src_w, uint32_t src_h,
                   struct drm_modeset_acquire_ctx *ctx)
{

    struct drm_plane_state*  plane_state = plane->state;
    const struct drm_plane_helper_funcs *funcs;
    gf_card_t *gf_card = (gf_card_t *)plane->dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;

    if (to_gf_plane(plane)->is_cursor == 0)
    {
        return  drm_atomic_helper_update_plane(plane, crtc, fb, crtc_x, crtc_y, crtc_w, crtc_h, src_x, src_y, src_w, src_h, ctx);
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

static int gf_splice_disable_plane(struct drm_plane *plane, struct drm_modeset_acquire_ctx *ctx)
{
    struct drm_plane_state*  plane_state = plane->state;
    const struct drm_plane_helper_funcs *funcs;

    if(to_gf_plane(plane)->is_cursor == 0)
    {
        return  drm_atomic_helper_disable_plane(plane, ctx);
    }

    if ((plane_state->crtc)&&(plane_state->crtc->state))
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

static void gf_splice_plane_destroy(struct drm_plane *plane)
{
    drm_plane_cleanup(plane);
    gf_free(to_gf_plane(plane));
}

static struct drm_plane_state* gf_splice_plane_duplicate_state(struct drm_plane *plane)
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


static void gf_splice_plane_destroy_state(struct drm_plane *plane, struct drm_plane_state *state)
{
    gf_plane_state_t* gf_pstate = to_gf_plane_state(state);

    __drm_atomic_helper_plane_destroy_state(state);

    gf_free(gf_pstate);
}

static int  gf_splice_plane_atomic_set_property(struct drm_plane *plane,
                struct drm_plane_state *state,
                struct drm_property *property,
                uint64_t val)
{
    //TODO: set property

    return 0;
}


static int gf_splice_plane_atomic_get_property(struct drm_plane *plane,
                const struct drm_plane_state *state,
                struct drm_property *property,
                uint64_t *val)
{

    //TODO: get property
    return 0;
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
static bool gf_splice_plane_format_mod_supported(struct drm_plane *plane, uint32_t format,
                                   uint64_t modifier)
{

    return TRUE;

}
#endif

static void drm_atomic_set_fence_for_plane_gf(struct drm_plane_state *plane_state, dma_fence_t *fence)
{
    if (plane_state->fence) {
        dma_fence_put(fence);

        return;
    }

    plane_state->fence = fence;
}

static int gf_splice_prepare_plane_fb(struct drm_plane *plane, struct drm_plane_state *new_state)
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


static void gf_splice_cleanup_plane_fb(struct drm_plane *plane, struct drm_plane_state *old_state)
{
    gf_card_t *gf = plane->dev->dev_private;
    struct drm_framebuffer *fb = old_state->fb;

    if (fb && to_gfb(fb)->obj)
    {
        gf_core_interface->mark_pagable(gf->adapter, to_gfb(fb)->obj->core_handle);
    }
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
int gf_splice_plane_atomic_check(struct drm_plane *plane, struct drm_atomic_state *state)
#else
int gf_splice_plane_atomic_check(struct drm_plane *plane, struct drm_plane_state *new_state)
#endif
{

    return 0;
}


static void gf_splice_plane_atomic_update_internal(struct drm_plane *plane,  struct drm_plane_state *new_state, struct drm_plane_state *old_state)
{
    //TODO
    gf_card_t *gf_card = plane->dev->dev_private;
    struct drm_device *drm_dev = gf_card->drm_dev;
    gf_plane_state_t *gf_plane_state = to_gf_plane_state(plane->state);
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_splice_manager_t *splice_manager = disp_info->splice_manager;
    int i = 0;

    for (i = 0; i < splice_manager->source_num; i++)
    {
        gf_splice_source_t *source = &(splice_manager->sources[i]);
        gf_connector_state_t *gf_conn_state = NULL;
        struct drm_crtc *splice_crtc = NULL;
        gf_splice_source_cfg_t *src_config = &(source->config);

        if (!src_config->enable)
        {
            continue;
        }

        if (is_splice_source_active_in_drm(drm_dev, source) && splice_manager->splice_enable == 0)
        {
            continue;
        }

        splice_crtc = gf_splice_get_crtc_by_source(drm_dev, source);

        if (to_gf_plane(plane)->is_cursor)
        {
            gf_cursor_update_t arg = {0, };
            arg.crtc = drm_get_crtc_index(splice_crtc);
            arg.vsync_on = gf_plane_state->legacy_cursor ? 0 : 1;

            if (new_state->crtc_x >= src_config->crtc_x && new_state->crtc_x <= (src_config->crtc_x  + src_config->crtc_w) &&
                new_state->crtc_y >= src_config->crtc_y && new_state->crtc_y <= (src_config->crtc_y  + src_config->crtc_h))
            {
                if (plane->state->crtc && !gf_plane_state->disable)
                {
                    arg.bo          = new_state->fb ? to_gfb(new_state->fb)->obj : NULL;
                    arg.pos_x       = new_state->crtc_x - src_config->crtc_x;
                    arg.pos_y       = new_state->crtc_y - src_config->crtc_y;
                    arg.width       = new_state->crtc_w;
                    arg.height      = new_state->crtc_h;
                }

                source->cursor_status |= GF_SPLICE_CURSOR_ACTIVE;

                disp_cbios_update_cursor(disp_info, &arg);
            }
            else if (source->cursor_status & GF_SPLICE_CURSOR_ACTIVE)
            {
                source->cursor_status &= ~GF_SPLICE_CURSOR_ACTIVE;

                disp_cbios_update_cursor(disp_info, &arg);
            }

        }
        else
        {
            gf_crtc_flip_t arg = {0};

            arg.crtc = drm_get_crtc_index(splice_crtc);
            arg.stream_type = to_gf_plane(plane)->plane_type;

            if (new_state->crtc && !gf_plane_state->disable)
            {
                arg.fb = new_state->fb;
                arg.crtc_x = 0;
                arg.crtc_y = 0;
                arg.crtc_w = src_config->crtc_w;
                arg.crtc_h = src_config->crtc_h;
                arg.src_x = src_config->crtc_x;
                arg.src_y = src_config->crtc_y;
                arg.src_w = src_config->crtc_w;
                arg.src_h = src_config->crtc_h;

    #if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
    #if  DRM_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
                if(plane->state->crtc->state->async_flip)
    #else
                if(plane->state->crtc->state->pageflip_flags & DRM_MODE_PAGE_FLIP_ASYNC)
    #endif
                {
                    /*
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
                    */

                    gf_crtc_state_t* gf_cstate = to_gf_crtc_state(plane->state->crtc->state);

                    arg.async_flip = 0;
                    gf_cstate->keep_vsync = 1;

                }
    #endif

            }

            disp_cbios_crtc_flip(disp_info, &arg);
        }
    }

}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
void gf_splice_plane_atomic_update(struct drm_plane* plane, struct drm_atomic_state* state)
#else
void gf_splice_plane_atomic_update(struct drm_plane* plane, struct drm_plane_state* old_state)
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

    gf_pstate->disable = 0;

    gf_splice_plane_atomic_update_internal(plane, new_state, old_state);
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
static void gf_splice_plane_atomic_disable(struct drm_plane *plane, struct drm_atomic_state *state)
#else
static void gf_splice_plane_atomic_disable(struct drm_plane *plane, struct drm_plane_state *old_state)
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

    gf_splice_plane_atomic_update_internal(plane, new_state, old_state);
}

static const struct drm_plane_funcs gf_splice_plane_funcs = {
    .update_plane = gf_splice_update_plane,
    .disable_plane = gf_splice_disable_plane,
    .destroy = gf_splice_plane_destroy,
    .atomic_duplicate_state = gf_splice_plane_duplicate_state,
    .atomic_destroy_state = gf_splice_plane_destroy_state,
    .atomic_set_property = gf_splice_plane_atomic_set_property,
    .atomic_get_property = gf_splice_plane_atomic_get_property,
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
    .format_mod_supported = gf_splice_plane_format_mod_supported,
#endif
};

static const struct drm_plane_helper_funcs gf_splice_plane_helper_funcs = {
    .prepare_fb = gf_splice_prepare_plane_fb,
    .cleanup_fb = gf_splice_cleanup_plane_fb,
    .atomic_check = gf_splice_plane_atomic_check,
    .atomic_update = gf_splice_plane_atomic_update,
    .atomic_disable = gf_splice_plane_atomic_disable,
};

static gf_plane_t* gf_splice_plane_create(gf_splice_manager_t* splice_manager, int is_cursor)
{
    disp_info_t *disp_info = (disp_info_t*) splice_manager->private;
    gf_card_t* gf_card = disp_info->gf_card;
    struct drm_device* drm = gf_card->drm_dev;
    gf_plane_t *gf_plane = NULL;
    gf_plane_state_t* gf_pstate = NULL;
    const int* formats = 0;
    char* name;
    int fmt_count = 0;
    int drm_ptype, ret = 0;
    unsigned int target_crtc_index = splice_manager->target.crtc_index;

    gf_plane = gf_calloc(sizeof(gf_plane_t));
    if (!gf_plane)
    {
        DRM_ERROR("failed to alloc gf plane\n");
        goto fail;
    }

    gf_plane->crtc_index = target_crtc_index;

    if (is_cursor)
    {
        gf_plane->plane_type = GF_SPLICE_CURSOR_PLANE;
        gf_plane->is_cursor = 1;
        gf_plane->can_window = 1;
    }
    else
    {
        gf_plane->plane_type = GF_SPLICE_PRIMARY_PLANE;
        gf_plane->can_window =  0;
        gf_plane->can_up_scale = 0;
        gf_plane->can_down_scale = 0;
    }

    gf_pstate = gf_calloc(sizeof(gf_plane_state_t));
    if (!gf_pstate)
    {
        DRM_ERROR("failed to alloc gf plane state\n");
        goto fail;
    }

    gf_pstate->base_pstate.plane = &gf_plane->base_plane;
    gf_plane->base_plane.state = &gf_pstate->base_pstate;

    if(is_cursor)
    {
        formats = splice_cursor_formats;
        fmt_count = sizeof(splice_cursor_formats)/sizeof(splice_cursor_formats[0]);
        name = "cursor";
        drm_ptype = DRM_PLANE_TYPE_CURSOR;
    }
    else
    {
        formats = splice_plane_formats;
        fmt_count = sizeof(splice_plane_formats)/sizeof(splice_plane_formats[0]);
        name = "PS";
        drm_ptype = DRM_PLANE_TYPE_PRIMARY;
    }

    ret = drm_universal_plane_init(drm, &gf_plane->base_plane,
                                    (1 << target_crtc_index), &gf_splice_plane_funcs,
                                    formats, fmt_count, NULL,
                                    drm_ptype,
                                    "IGA%d-%s", (target_crtc_index + 1), name);
    if (ret)
    {
        DRM_ERROR("failed to init drm plane\n");
        goto fail;
    }

    drm_plane_helper_add(&gf_plane->base_plane, &gf_splice_plane_helper_funcs);

    disp_create_plane_property(drm, gf_plane);

    DRM_DEBUG_KMS("plane=%d,name=%s\n", gf_plane->base_plane.index, gf_plane->base_plane.name);

    return gf_plane;

fail:
    if (gf_plane)
    {
        gf_free(gf_plane);
        gf_plane = NULL;
    }
    if (gf_pstate)
    {
        gf_free(gf_pstate);
        gf_pstate = NULL;
    }

    return NULL;
}


static int gf_splice_crtc_legacy_gamma_set(struct drm_crtc *crtc,
                                           u16 *red, u16 *green, u16 *blue,
                                           uint32_t size,
                                           struct drm_modeset_acquire_ctx *ctx)

{
    //TODO: handle gamma set
    return 0;
}

static void gf_splice_crtc_destroy(struct drm_crtc *crtc)
{
    gf_crtc_t *gf_crtc = to_gf_crtc(crtc);

    drm_crtc_cleanup(crtc);

    gf_free(gf_crtc);
}


static int gf_splice_crtc_set_config(struct drm_mode_set *set, struct drm_modeset_acquire_ctx *ctx)
{

    //NOTE: set config

    return drm_atomic_helper_set_config(set, ctx);
}

static int gf_splice_crtc_page_flip(struct drm_crtc *crtc,
                                    struct drm_framebuffer *fb,
                                    struct drm_pending_vblank_event *event,
                                    uint32_t flags,
                                    struct drm_modeset_acquire_ctx *ctx)
{
    //NOTE: page_flip

    return drm_atomic_helper_page_flip(crtc, fb, event, flags, ctx);
}

static struct drm_crtc_state* gf_splice_crtc_duplicate_state(struct drm_crtc *crtc)
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

static void gf_splice_crtc_destroy_state(struct drm_crtc *crtc, struct drm_crtc_state *s)
{
    gf_crtc_state_t*  state = to_gf_crtc_state(s);

    __drm_atomic_helper_crtc_destroy_state(s);
    gf_free(state);
}


#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
u32 gf_splice_get_vblank_counter(struct drm_crtc *crtc)
#else
u32 gf_splice_get_vblank_counter(struct drm_device *dev, pipe_t pipe)
#endif
{
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
    struct drm_device *dev = crtc->dev;
    unsigned int pipe = crtc->index;
#endif

    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_splice_manager_t *manager = disp_info->splice_manager;
    gf_splice_target_t *target = &manager->target;
    gf_splice_source_t *source = &(manager->sources[target->config.vblank_source]);
    struct drm_crtc *vbl_src_crtc = gf_splice_get_crtc_by_source(dev, source);
    gf_get_counter_t  gf_counter;
    int  vblank_cnt = 0;
    int i = 0;

    pipe = drm_get_crtc_index(vbl_src_crtc);

    gf_memset(&gf_counter, 0, sizeof(gf_get_counter_t));
    gf_counter.crtc_index = pipe;
    gf_counter.vblk = &vblank_cnt;
    disp_cbios_get_counter(disp_info, &gf_counter);

    //return (u32)(vblank_cnt - source->last_vblank);
    return (u32)vblank_cnt;
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
int gf_splice_enable_vblank(struct drm_crtc *crtc)
#else
int gf_splice_enable_vblank(struct drm_device *dev, pipe_t pipe)
#endif
{
    //TODO: splice
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
    struct drm_device *dev = crtc->dev;
    unsigned int pipe = crtc->index;
#endif

    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;
    gf_splice_manager_t *manager = disp_info->splice_manager;
    irq_chip_funcs_t* chip_func = (irq_chip_funcs_t*)disp_info->irq_chip_func;
    int  intrrpt = 0, intr_en = 0;
    unsigned long flags = 0;
    int i = 0;

    if(!chip_func || !chip_func->get_intr_enable_mask || !chip_func->set_intr_enable_mask)
    {
        return 0;
    }

    for (i = 0; i < manager->source_num; i++)
    {
        gf_splice_source_t *source = &(manager->sources[i]);
        gf_splice_source_cfg_t *src_config = &(source->config);
        struct drm_crtc *splice_crtc = NULL;

        if (!src_config->enable)
        {
            continue;
        }

        splice_crtc = gf_splice_get_crtc_by_source(dev, source);
        pipe = drm_get_crtc_index(splice_crtc);

        if(pipe == IGA1)
        {
            intrrpt = INT_VSYNC1;
        }
        else if(pipe == IGA2)
        {
            intrrpt = INT_VSYNC2;
        }
        else if(pipe == IGA3)
        {
            intrrpt = INT_VSYNC3;
        }
        else if(pipe == IGA4)
        {
            intrrpt = INT_VSYNC4;
        }

        flags = gf_spin_lock_irqsave(disp_info->intr_lock);

        if(disp_info->irq_enabled)
        {
            intr_en = chip_func->get_intr_enable_mask(disp_info);
            intr_en |= intrrpt;
            chip_func->set_intr_enable_mask(disp_info, intr_en);
        }
        else
        {
            disp_info->intr_en_bits |= intrrpt;
        }

        gf_spin_unlock_irqrestore(disp_info->intr_lock, flags);

        trace_gfx_vblank_onoff(gf_card->index << 16 | pipe, 1);

    }

    return  0;
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
void  gf_splice_disable_vblank(struct drm_crtc *crtc)
#else
void  gf_splice_disable_vblank(struct drm_device *dev, pipe_t pipe)
#endif
{
    //TODO: splice
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
    struct drm_device *dev = crtc->dev;
    unsigned int pipe = crtc->index;
#endif
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_splice_manager_t *splice_manager = disp_info->splice_manager;
    irq_chip_funcs_t *chip_func = (irq_chip_funcs_t*)disp_info->irq_chip_func;
    int  intrrpt = 0, intr_en = 0;
    unsigned long flags = 0;
    int i = 0;

    if (!chip_func || !chip_func->get_intr_enable_mask || !chip_func->set_intr_enable_mask)
    {
        return;
    }

    for (i = 0; i < splice_manager->source_num; i++)
    {
        gf_splice_source_t *source = &(splice_manager->sources[i]);
        gf_splice_source_cfg_t *src_config = &(source->config);
        struct drm_crtc *splice_crtc = NULL;

        if (!src_config->enable)
        {
            continue;
        }

        if (is_splice_source_active_in_drm(dev, source) && splice_manager->splice_enable == 0)
        {
            continue;
        }

        splice_crtc = gf_splice_get_crtc_by_source(dev, source);
        pipe = drm_get_crtc_index(splice_crtc);

        if(pipe == IGA1)
        {
            intrrpt = INT_VSYNC1;
        }
        else if(pipe == IGA2)
        {
            intrrpt = INT_VSYNC2;
        }
        else if(pipe == IGA3)
        {
            intrrpt = INT_VSYNC3;
        }
        else if(pipe == IGA4)
        {
            intrrpt = INT_VSYNC4;
        }

        flags = gf_spin_lock_irqsave(disp_info->intr_lock);

        if (disp_info->irq_enabled)
        {
            intr_en = chip_func->get_intr_enable_mask(disp_info);
            intr_en &= ~intrrpt;
            chip_func->set_intr_enable_mask(disp_info, intr_en);
        }
        else
        {
            disp_info->intr_en_bits &= ~intrrpt;
        }

        gf_spin_unlock_irqrestore(disp_info->intr_lock, flags);

        trace_gfx_vblank_onoff(gf_card->index << 16 | pipe, 0);
    }
}

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 4, 0)
int gf_splice_crtc_get_scanout_position(struct drm_device *dev,
                                               int pipe,  unsigned int flags,
                                               int *vpos, int *hpos,
                                               ktime_t *stime, ktime_t *etime)
{
    //NOTE: not support 4.4 kernel
    return 0;
}
#elif DRM_VERSION_CODE < KERNEL_VERSION(4, 13, 0)
int gf_splice_crtc_get_scanout_position(struct drm_device *dev,
                                        int pipe,  unsigned int flags,
                                        int *vpos, int *hpos,
                                        ktime_t *stime, ktime_t *etime)
{
    //TODO: support 4.13 kernel
    return 0;
}
#else
#if DRM_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
bool gf_splice_crtc_get_scanout_position(struct drm_device *dev,
                                         unsigned int pipe, bool in_vblank_irq,
                                         int *vpos, int *hpos,
                                         ktime_t *stime, ktime_t *etime,
                                         const struct drm_display_mode *mode)
#else
bool gf_splice_crtc_get_scanout_position(struct drm_crtc *crtc,
                                         bool in_vblank_irq,
                                         int *vpos, int *hpos,
                                         ktime_t *stime, ktime_t *etime,
                                         const struct drm_display_mode *mode)
#endif
{
    //TODO: use vlank source as the vblank source right now
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
    struct drm_device *dev = crtc->dev;
    unsigned int pipe = crtc->index;
#endif
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_splice_manager_t *splice_manager = disp_info->splice_manager;
    gf_get_counter_t  gf_counter;
    gf_splice_target_t *target = &splice_manager->target;
    gf_splice_source_t *source = &(splice_manager->sources[target->config.vblank_source]);
    struct drm_crtc *splice_crtc = gf_splice_get_crtc_by_source(dev, source);
    int in_vblank = 0, ret = 0, status = 0;

    pipe = drm_get_crtc_index(splice_crtc);
    mode = &(source->config.hw_mode);

    gf_memset(&gf_counter, 0, sizeof(gf_get_counter_t));
    gf_counter.crtc_index = pipe;
    gf_counter.hpos = hpos;
    gf_counter.vpos = vpos;
    gf_counter.in_vblk = &in_vblank;

    if (stime)
    {
        *stime = ktime_get();
    }

    status = disp_cbios_get_counter(disp_info, &gf_counter);

    if (etime)
    {
        *etime = ktime_get();
    }

    if (status == S_OK)
    {
        ret = DRM_SCANOUTPOS_VALID | DRM_SCANOUTPOS_ACCURATE;
        if (in_vblank)
        {
            *vpos -= mode->crtc_vblank_end;
            if(*hpos)
            {
                *hpos -= mode->crtc_htotal;
                *vpos += 1;
            }
            ret |= DRM_SCANOUTPOS_IN_VBLANK;
        }
    }

    return ret;
}
#endif

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 13, 0)
int gf_splice_get_vblank_timestamp(struct drm_device *dev,
                                   pipe_t pipe,
                                   int *max_error,
                                   struct timeval *time,
                                   unsigned flags)
{
   //TODO: support 4.13 kernel


    return 0;
}

#else

#if DRM_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
bool gf_splice_get_vblank_timestamp(struct drm_device *dev,
                                    pipe_t pipe,
                                    int *max_error,
                                #if DRM_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
                                    ktime_t *vblank_time,
                                #else
                                    struct timeval *vblank_time,
                                #endif
                                    bool in_vblank_irq)
{

#else
bool gf_splice_get_vblank_timestamp(struct drm_crtc *crtc,
                                    int *max_error,
                                    ktime_t *vblank_time,
                                    bool in_vblank_irq)
{
    struct drm_device *dev = crtc->dev;
    unsigned int pipe = crtc->index;
#endif

    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_splice_manager_t *manager = disp_info->splice_manager;
    gf_splice_target_t *target = &manager->target;
    gf_splice_source_t *source = &(manager->sources[target->config.vblank_source]);
    const struct drm_display_mode *mode;
    int vpos, hpos, i;
    ktime_t stime, etime;
    int delta_ns, duration_ns;
    bool vbl_status;

    mode = &(source->config.hw_mode);

    for (i = 0; i < GF_DRM_TIMESTAMP_MAXRETRIES; i++) {
       /*
       * Get vertical and horizontal scanout position vpos, hpos,
       * and bounding timestamps stime, etime, pre/post query.
       */

#if DRM_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
        vbl_status =  gf_splice_crtc_get_scanout_position(dev, pipe, in_vblank_irq,
                                                          &vpos, &hpos,
                                                          &stime, &etime,
                                                          mode);
#else
        vbl_status =  gf_splice_crtc_get_scanout_position(crtc, in_vblank_irq,
                                                          &vpos, &hpos,
                                                          &stime, &etime,
                                                          mode);
#endif
       /* Return as no-op if scanout query unsupported or failed. */
        if (!vbl_status) {
            DRM_DEBUG("crtc %u : scanoutpos query failed.\n", pipe);
            return false;
        }

        /* Compute uncertainty in timestamp of scanout position query. */
        duration_ns = ktime_to_ns(etime) - ktime_to_ns(stime);

        /* Accept result with <  max_error nsecs timing uncertainty. */
        if (duration_ns <= *max_error)
            break;
    }

    /* Noisy system timing? */
    if (i == GF_DRM_TIMESTAMP_MAXRETRIES) {
        DRM_DEBUG("crtc %u: Noisy timestamp %d us > %d us [%d reps].\n",
                  pipe, duration_ns/1000, *max_error/1000, i);
    }

    /* Return upper bound of timestamp precision error. */
    *max_error = duration_ns;


    /* Convert scanout position into elapsed time at raw_time query
     * since start of scanout at first display scanline. delta_ns
     * can be negative if start of scanout hasn't happened yet.
     */
    delta_ns = div_s64(1000000LL * (vpos * mode->crtc_htotal + hpos),
                       mode->crtc_clock);

    /* Subtract time delta from raw timestamp to get final
     * vblank_time timestamp for end of vblank.
     */
    *vblank_time = ktime_sub_ns(etime, delta_ns);

    return true;

}
#endif

static void gf_splice_crtc_helper_set_mode(struct drm_crtc *crtc)
{
    struct drm_device *dev = crtc->dev;
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_splice_manager_t *manager = disp_info->splice_manager;
    gf_splice_target_t *target = &manager->target;
    gf_splice_source_t *vblank_source = &(manager->sources[target->config.vblank_source]);
    struct drm_crtc *vblank_crtc = gf_splice_get_crtc_by_source(dev, vblank_source);
    gf_get_counter_t  gf_counter;
    int  vblank_cnt = 0;
    int flag = 0;
    int i  = 0, j = 0;

    flag |= UPDATE_CRTC_MODE_FLAG;

    for (i = 0; i < manager->source_num; i++)
    {
        gf_splice_source_t *source = &(manager->sources[i]);
        gf_splice_source_cfg_t *src_config = &(source->config);
        unsigned int active[MAX_CORE_CRTCS] = {0};
        struct drm_crtc *splice_crtc = NULL;
        unsigned int crtc_index = 0;

        if (!src_config->enable)
        {
            continue;
        }

        splice_crtc = gf_splice_get_crtc_by_source(dev, source);
        crtc_index = drm_get_crtc_index(splice_crtc);

        gf_memcpy(active, disp_info->active_output, sizeof(disp_info->active_output));
        active[crtc_index] = 0;

        for(j = 0; j < disp_info->num_crtc; j++)
        {
            active[j] &= ~source->output_type;
        }

        active[crtc_index] |= source->output_type;
        if (!disp_cbios_update_output_active(disp_info, active))
        {
            gf_memcpy(disp_info->active_output, active, sizeof(disp_info->active_output));
        }

        disp_cbios_set_mode(disp_info, crtc_index, &(source->config.drm_mode), &(source->config.hw_mode), flag);
    }

    gf_memset(&gf_counter, 0, sizeof(gf_get_counter_t));
    gf_counter.crtc_index = drm_get_crtc_index(vblank_crtc);
    gf_counter.vblk = &vblank_cnt;
    disp_cbios_get_counter(disp_info, &gf_counter);

    vblank_source->last_vblank = vblank_cnt;
}

static void gf_splice_crtc_dpms_onoff_helper(struct drm_crtc *crtc, int dpms_on)
{
    struct drm_device *drm_dev = crtc->dev;
    gf_card_t *gf_card = drm_dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_splice_manager_t *splice_manager = disp_info->splice_manager;
    gf_crtc_t *gf_crtc = to_gf_crtc(crtc);
    int status = dpms_on? 1 : 0;
    int i = 0;

    if (gf_crtc->crtc_dpms == status)
    {
        return;
    }

    for (i = 0; i < splice_manager->source_num; i++)
    {
        gf_splice_source_t *source = &(splice_manager->sources[i]);
        gf_splice_source_cfg_t *src_config = &(source->config);
        struct drm_crtc *source_crtc = NULL;
        gf_crtc_t *gf_source_crtc = NULL;

        if (!src_config->enable)
        {
            continue;
        }

        source_crtc = gf_splice_get_crtc_by_source(drm_dev, source);
        gf_source_crtc = to_gf_crtc(source_crtc);

        if (is_splice_source_active_in_drm(drm_dev, source) && splice_manager->splice_enable == 0)
        {
            continue;
        }

        if(!status)
        {
            //turn off
            disp_cbios_turn_onoff_screen(disp_info, gf_source_crtc->pipe, 0);
            disp_cbios_turn_onoff_iga(disp_info, gf_source_crtc->pipe, 0);
        }
        else if(status)
        {
            //turn on
            disp_cbios_turn_onoff_iga(disp_info, gf_source_crtc->pipe, 1);
            disp_cbios_turn_onoff_screen(disp_info, gf_source_crtc->pipe, 1);
        }
    }

    gf_crtc->crtc_dpms = status;
}


static void gf_splice_crtc_helper_disable(struct drm_crtc *crtc)
{

    gf_splice_crtc_dpms_onoff_helper(crtc, 0);
    drm_crtc_vblank_off(crtc);
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
static int gf_splice_crtc_helper_atomic_check(struct drm_crtc *crtc, struct drm_atomic_state *atomic_state)
#else
static int gf_splice_crtc_helper_atomic_check(struct drm_crtc *crtc, struct drm_crtc_state *state)
#endif
{
    //NOTE: splice crtc helper atomic_check
    return 0;
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
void gf_splice_crtc_helper_atomic_begin(struct drm_crtc *crtc, struct drm_atomic_state *state)
#else
void gf_splice_crtc_helper_atomic_begin(struct drm_crtc *crtc, struct drm_crtc_state *old_crtc_state)
#endif
{
    //NOTE: splice crtc helper atomic_begin

    return;
}

static bool gf_splice_need_wait_vblank(struct drm_crtc *crtc, struct drm_crtc_state *old_crtc_state)
{
    struct drm_plane *plane;
    struct drm_plane_state *old_plane_state;
    int i;
    bool need_wait = false;

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
void gf_splice_crtc_helper_atomic_flush(struct drm_crtc *crtc, struct drm_atomic_state *state)
#else
void gf_splice_crtc_helper_atomic_flush(struct drm_crtc *crtc, struct drm_crtc_state *old_crtc_state)
#endif
{
    //TODO: splice crtc helper atomic_flush
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
        if (gf_splice_need_wait_vblank(crtc, old_crtc_state))
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

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
void  gf_splice_crtc_helper_atomic_enable(struct drm_crtc *crtc, struct drm_atomic_state *state)
#else
void  gf_splice_crtc_helper_atomic_enable(struct drm_crtc *crtc, struct drm_crtc_state *old_crtc_state)
#endif
{
    gf_crtc_t *gf_crtc = to_gf_crtc(crtc);

    //NOTE: splice crtc helper atomic_enable
    gf_info("CRTC atomic enable: to turn on vblank and screen of crtc: %d\n", crtc->index);

    drm_crtc_vblank_on(crtc);

    gf_splice_crtc_dpms_onoff_helper(crtc, 1);

    gf_crtc->enabled = 1;
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
void  gf_splice_crtc_helper_atomic_disable(struct drm_crtc *crtc, struct drm_atomic_state *state)
#else
void  gf_splice_crtc_helper_atomic_disable(struct drm_crtc *crtc, struct drm_crtc_state *old_crtc_state)
#endif
{
    //NOTE: splice crtc helper atomic_disable
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

    if(0 == drm_crtc_vblank_get(crtc))
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

    gf_splice_crtc_dpms_onoff_helper(crtc, 0);

    drm_crtc_vblank_off(crtc);

    gf_crtc->enabled = 0;
}

static const struct drm_crtc_funcs gf_splice_crtc_funcs = {
#if DRM_VERSION_CODE < KERNEL_VERSION(5, 12, 0)
    .gamma_set = gf_splice_crtc_legacy_gamma_set,
#endif
    .destroy = gf_splice_crtc_destroy,
    .set_config = gf_splice_crtc_set_config,
    .page_flip  = gf_splice_crtc_page_flip,
    .atomic_duplicate_state = gf_splice_crtc_duplicate_state,
    .atomic_destroy_state = gf_splice_crtc_destroy_state,
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
    .get_vblank_counter = gf_splice_get_vblank_counter,
    .enable_vblank = gf_splice_enable_vblank,
    .disable_vblank = gf_splice_disable_vblank,
    .get_vblank_timestamp = gf_splice_get_vblank_timestamp,
#endif
};

static const struct drm_crtc_helper_funcs gf_splice_crtc_helper_funcs = {
    .mode_set_nofb = gf_splice_crtc_helper_set_mode,
    .disable = gf_splice_crtc_helper_disable,
    .atomic_check = gf_splice_crtc_helper_atomic_check,
    .atomic_begin = gf_splice_crtc_helper_atomic_begin,
    .atomic_flush = gf_splice_crtc_helper_atomic_flush,
    .atomic_enable = gf_splice_crtc_helper_atomic_enable,
    .atomic_disable = gf_splice_crtc_helper_atomic_disable,
#if DRM_VERISON_CODE >= KERNEL_VERSION(5, 7, 0)
    .get_scanout_position = gf_splice_crtc_get_scanout_position,
#endif
};

static void gf_splice_attach_state_info_property(struct drm_connector *connector)
{
    struct drm_device *dev = connector->dev;
    gf_connector_t* gf_conn = to_gf_connector(connector);
    gf_connector_state_t* gf_state = to_gf_conn_state(connector->state);
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;

    if (gf_conn->output_type == DISP_OUTPUT_SPLICE)
    {
        return;
    }

    drm_object_attach_property(&connector->base, disp_info->splice_active_prop, 0);
    gf_state->splice_active = 0;
}

static void gf_splice_attach_mode_info_property(struct drm_connector *connector)
{
    struct drm_device *dev = connector->dev;
    gf_connector_state_t* gf_state = to_gf_conn_state(connector->state);
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;

    drm_object_attach_property(&connector->base, disp_info->mode_x_prop, 0);
    gf_state->splice_mode_x = 0;

    drm_object_attach_property(&connector->base, disp_info->mode_y_prop, 0);
    gf_state->splice_mode_y = 0;

    drm_object_attach_property(&connector->base, disp_info->mode_rate_prop, 0);
    gf_state->splice_mode_rate = 0;
}

static void gf_splice_attach_size_info_property(struct drm_connector *connector)
{
    struct drm_device *dev = connector->dev;
    gf_connector_t* gf_conn = to_gf_connector(connector);
    gf_connector_state_t* gf_state = to_gf_conn_state(connector->state);
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;

    if (gf_conn->output_type == DISP_OUTPUT_SPLICE)
    {
        return;
    }

    drm_object_attach_property(&connector->base, disp_info->crtc_x_prop, 0);
    gf_state->splice_crtc_x = 0;

    drm_object_attach_property(&connector->base, disp_info->crtc_y_prop, 0);
    gf_state->splice_crtc_y = 0;

    drm_object_attach_property(&connector->base, disp_info->crtc_w_prop, 0);
    gf_state->splice_crtc_w = 0;

    drm_object_attach_property(&connector->base, disp_info->crtc_h_prop, 0);
    gf_state->splice_crtc_h = 0;
}

static void gf_splice_attach_trigger_property(struct drm_connector *connector)
{
    struct drm_device *dev = connector->dev;
    gf_connector_t* gf_conn = to_gf_connector(connector);
    gf_connector_state_t* gf_state = to_gf_conn_state(connector->state);
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;

    if(gf_conn->output_type != DISP_OUTPUT_SPLICE)
    {
        return;
    }

    drm_object_attach_property(&connector->base, disp_info->splice_trigger_prop, 0);
    gf_state->splice_trigger = 0;
}

void gf_splice_attach_connector_property(struct drm_connector *connector)
{
    gf_splice_attach_state_info_property(connector);
    gf_splice_attach_mode_info_property(connector);
    gf_splice_attach_size_info_property(connector);

    gf_splice_attach_trigger_property(connector);
}

static int gf_splice_init_sources(gf_splice_manager_t *splice_manager)
{
    disp_info_t *disp_info = (disp_info_t*) splice_manager->private;
    gf_card_t* gf_card = disp_info->gf_card;
    struct  drm_device* drm_dev = gf_card->drm_dev;
    struct drm_connector* connector = NULL;
    gf_splice_source_t *sources = NULL;
    int i = 0;

    sources = gf_calloc(sizeof(gf_splice_source_t) * disp_info->num_output);
    if (!sources)
    {
        DRM_ERROR("allocate splice sources failed\n");
        return -ENOMEM;
    }

    list_for_each_entry(connector, &drm_dev->mode_config.connector_list, head)
    {
        gf_connector_t*  gf_connector = to_gf_connector(connector) ;

        sources[i].connector   = connector;
        sources[i].output_type = gf_connector->output_type;
        i++;
    }

    splice_manager->sources  = sources;
    splice_manager->source_num = disp_info->num_output;

    return 0;
}

static void gf_splice_init_encoders_and_connectors(gf_splice_manager_t *splice_manager)
{
    struct drm_connector *connector = NULL;
    struct drm_encoder* encoder = NULL;

    connector = gf_splice_connector_init(splice_manager);

    encoder = gf_splice_encoder_init(splice_manager);

    drm_connector_attach_encoder(connector, encoder);

    splice_manager->target.connector = connector;
}

static void gf_splice_init_crtc_and_planes(gf_splice_manager_t *splice_manager)
{
    disp_info_t *disp_info = (disp_info_t*) splice_manager->private;
    gf_card_t* gf_card = disp_info->gf_card;
    struct drm_device* drm = gf_card->drm_dev;
    gf_plane_t* gf_plane = NULL;
    gf_plane_t* gf_cursor = NULL;
    gf_crtc_t* gf_crtc = NULL;
    gf_crtc_state_t* crtc_state = NULL;
    gf_splice_target_t *target = &(splice_manager->target);
    int ret = 0;

    gf_plane = gf_splice_plane_create(splice_manager, 0);

    gf_cursor = gf_splice_plane_create(splice_manager, 1);

    gf_crtc = gf_calloc(sizeof(gf_crtc_t));
    if (!gf_crtc)
    {
        gf_error("failed to alloc gf crtc\n");
        goto failed;
    }

    crtc_state = gf_calloc(sizeof(gf_crtc_state_t));
    if (!crtc_state)
    {
        gf_error("failed to alloc gf crtc state\n");
        goto failed;
    }

    gf_crtc->base_crtc.state = &crtc_state->base_cstate;
    crtc_state->base_cstate.crtc = &gf_crtc->base_crtc;

    gf_crtc->pipe = target->crtc_index;
    gf_crtc->crtc_dpms = 0;
    gf_crtc->support_scale = disp_info->scale_support;
    gf_crtc->plane_cnt = GF_SPLICE_PLANE_NUM;
    gf_crtc->vsync_int = 0;

    ret = drm_crtc_init_with_planes(drm, &gf_crtc->base_crtc,
                                    &gf_plane->base_plane,
                                    &gf_cursor->base_plane,
                                    &gf_splice_crtc_funcs, "IGA%d", (gf_crtc->pipe + 1));
    if (ret)
    {
        DRM_ERROR("failed to init crtc\n");
        goto  failed;
    }

    drm_crtc_helper_add(&gf_crtc->base_crtc, &gf_splice_crtc_helper_funcs);

    drm_mode_crtc_set_gamma_size(&gf_crtc->base_crtc, 256);

    drm_crtc_enable_color_mgmt(&gf_crtc->base_crtc, 0, 1, 256);

    splice_manager->target.crtc = &gf_crtc->base_crtc;

    return;
failed:
    if (crtc_state)
    {
        gf_free(crtc_state);
        crtc_state = NULL;
    }

    if (gf_crtc)
    {
        gf_free(gf_crtc);
        gf_crtc = NULL;
    }
}

int gf_splice_find_source_hw_mode(gf_splice_manager_t *splice_manager,
                                   gf_splice_source_t *source)
{
    disp_info_t *disp_info = (disp_info_t*)splice_manager->private;
    struct drm_display_mode *hw_mode = &(source->config.hw_mode);
    struct drm_display_mode *mode = &(source->config.drm_mode);
    int output  = source->output_type;
    int ret = -1;

    drm_mode_copy(hw_mode, mode);
    if(gf_encoder_mode_fixup_internal(disp_info, output, (const struct drm_display_mode*)mode, hw_mode))
    {
        ret = 0;
    }

    return ret;
}

int gf_splice_validate_source_params(gf_splice_manager_t *splice_manager)
{
    disp_info_t *disp_info = (disp_info_t*)splice_manager->private;
    gf_splice_source_t *sources = splice_manager->sources;
    CBiosModeInfoExt *pcbios_mode = NULL;
    void* pmode_list = NULL;
    struct drm_connector *connector = NULL;
    gf_connector_t *gf_conn = NULL;
    gf_connector_state_t *gf_conn_state  = NULL;
    gf_splice_source_cfg_t *src_config = NULL;
    int i = 0, j = 0, ret = 0, mode_num = 0;
    int active_src_num = 0;
    int trigger_ids = splice_manager->trigger_value;

    for (i = 0; i < splice_manager->source_num; i++)
    {
        gf_splice_source_cfg_t *src_config = NULL;
        connector = sources[i].connector;
        gf_conn = to_gf_connector(connector);

        src_config = &(sources[i].config);

        gf_memcpy(&(sources[i].restored_config), &(sources[i].config), sizeof(gf_splice_source_cfg_t));
        sources[i].has_restored = TRUE;

        if ((trigger_ids & gf_conn->output_id) == 0)
        {
            src_config->enable = FALSE;
            continue;
        }
        trigger_ids &= ~gf_conn->output_id;

        gf_conn_state = to_gf_conn_state(connector->state);

        src_config->enable = TRUE;

        active_src_num++;

        if (connector->status != connector_status_connected)
        {
            DRM_ERROR("source: 0x%x splice connector is not connected!\n", gf_conn->output_type);
            ret = -2;
            goto END;
        }

        if (!gf_conn_state->splice_mode_x || !gf_conn_state->splice_mode_y || !gf_conn_state->splice_mode_rate)
        {
            DRM_ERROR("source: 0x%x Invalid splice mode para!\n", gf_conn->output_type);
            ret = -3;
            goto END;
        }

        src_config->crtc_x = gf_conn_state->splice_crtc_x;
        src_config->crtc_y = gf_conn_state->splice_crtc_y;
        src_config->crtc_w = gf_conn_state->splice_crtc_w;
        src_config->crtc_h = gf_conn_state->splice_crtc_h;

        gf_info("## User set splice mode for device 0x%x, mode %dx%d@%d.\n", gf_conn->output_type,
                gf_conn_state->splice_mode_x, gf_conn_state->splice_mode_y, gf_conn_state->splice_mode_rate);

        pmode_list = disp_cbios_get_device_modelist(disp_info, gf_conn->output_type, &mode_num);
        if (!pmode_list || !mode_num)
        {
            DRM_ERROR("source: 0x%x get modelist failed\n", gf_conn->output_type);
            ret = -4;
            goto END;
        }

        for (j = 0; j < mode_num; j++)
        {
            pcbios_mode = (CBiosModeInfoExt*)pmode_list + j;
            if (pcbios_mode->XRes == gf_conn_state->splice_mode_x
                && pcbios_mode->YRes == gf_conn_state->splice_mode_y)
            {
                ret = disp_cbios_cbmode_to_drmmode(disp_info, gf_conn->output_type, pcbios_mode, 0, &(src_config->drm_mode));
                if (!ret)
                {
                    int temp = src_config->drm_mode.clock * 1000/src_config->drm_mode.htotal;
                    if (gf_conn_state->splice_mode_rate == temp * 100/src_config->drm_mode.vtotal)
                    {
                        break;
                    }
                }
            }
        }

        if(j == mode_num)
        {
            DRM_ERROR("source: 0x%x find user set splice mode:%dx%d@%d failed\n", gf_conn->output_type,
                      gf_conn_state->splice_mode_x, gf_conn_state->splice_mode_y, gf_conn_state->splice_mode_rate);
            ret = -5;
            goto END;
        }

        ret = gf_splice_find_source_hw_mode(splice_manager, &sources[i]);
        if (ret)
        {
            DRM_ERROR("source: 0x%x find hw splice mode error\n", sources[i].output_type);
            ret = -6;
            goto END;
        }

        if(pmode_list)
        {
            gf_free(pmode_list);
            pmode_list = NULL;
        }
    }

    if (!active_src_num)
    {
        DRM_ERROR("Non active splice source exists\n");
        ret = -7;
        goto END;
    }

    if(trigger_ids)
    {
        DRM_ERROR("Invalid trigger value 0x%x\n", splice_manager->trigger_value);
        ret = -8;
        goto END;
    }

END:
    if (pmode_list)
    {
        gf_free(pmode_list);
        pmode_list = NULL;
    }

    return ret;
}

int gf_splice_validate_target_params(gf_splice_manager_t *splice_manager)
{
    gf_splice_source_t *sources = splice_manager->sources;
    gf_splice_target_t *target = &splice_manager->target;
    struct drm_connector *connector = NULL;
    gf_connector_state_t *gf_conn_state  = NULL;
    gf_splice_source_cfg_t *src_config = NULL;
    int min_refresh = 0xFFFF, vbl_source_index = 0;
    unsigned int tmp_target_x = 0, tmp_target_y = 0, tmp_target_refresh = 0;
    int i = 0;

    connector = target->connector;
    gf_conn_state = to_gf_conn_state(connector->state);

    for (i = 0; i < splice_manager->source_num; i++)
    {
        gf_splice_source_cfg_t *src_config = &(sources[i].config);

        if (!src_config->enable)
        {
            continue;
        }

        if ((gf_conn_state->splice_mode_x % src_config->drm_mode.hdisplay) != 0 ||
            (gf_conn_state->splice_mode_y % src_config->drm_mode.vdisplay) != 0)
        {
            DRM_ERROR("splice target mode error\n");
            return -1;
        }
        break;
    }

    tmp_target_x = gf_conn_state->splice_mode_x;
    tmp_target_y = gf_conn_state->splice_mode_y;
    tmp_target_refresh = gf_conn_state->splice_mode_rate;

    for (i = 0; i < splice_manager->source_num; i++)
    {
        gf_splice_source_cfg_t *src_config = &(sources[i].config);

        if (!src_config->enable)
        {
            continue;
        }

        connector = sources[i].connector;
        gf_conn_state = to_gf_conn_state(connector->state);

        if ((src_config->crtc_x + src_config->crtc_w) > tmp_target_x ||
            (src_config->crtc_y + src_config->crtc_h) > tmp_target_y)
        {
            DRM_ERROR("splice source size exceeds target mode\n");
            return -2;
        }

        if (min_refresh > gf_conn_state->splice_mode_rate)
        {
            min_refresh = gf_conn_state->splice_mode_rate;
            vbl_source_index = i;
        }
    }

    target->config.vblank_source = vbl_source_index;

    target->config.mode_x = tmp_target_x;
    target->config.mode_y = tmp_target_y;
    target->config.mode_rate = tmp_target_refresh;

    return 0;
}

void gf_splice_reset_state(gf_splice_manager_t *splice_manager, bool need_restore)
{
    gf_connector_t *gf_connector= NULL;
    struct drm_connector *connector = NULL;
    gf_connector_state_t *gf_conn_state  = NULL;
    int i = 0;

    if (!splice_manager)
    {
        return;
    }

    for (i = 0; i < splice_manager->source_num; i++)
    {
        gf_splice_source_t *source = &(splice_manager->sources[i]);

        if (source->has_restored && need_restore)
        {
            gf_memcpy(&(source->config), &(source->restored_config),  sizeof(gf_splice_source_cfg_t));

            source->has_restored = FALSE;
        }

        connector = source->connector;
        gf_conn_state = to_gf_conn_state(connector->state);
        //connector->status = connector_status_connected;

        gf_conn_state->splice_active = 0;
        gf_conn_state->splice_mode_x = 0;
        gf_conn_state->splice_mode_y = 0;
        gf_conn_state->splice_mode_rate= 0;
        gf_conn_state->splice_crtc_x = 0;
        gf_conn_state->splice_crtc_y= 0;
        gf_conn_state->splice_crtc_w = 0;
        gf_conn_state->splice_crtc_h = 0;
    }

    connector = splice_manager->target.connector;
    gf_conn_state = to_gf_conn_state(connector->state);
    connector->status = connector_status_disconnected;
    gf_conn_state->splice_mode_x = 0;
    gf_conn_state->splice_mode_y = 0;
    gf_conn_state->splice_mode_rate = 0;
    gf_conn_state->splice_trigger = 0;
}

int gf_splice_handle_source_status(struct drm_connector *connector)
{
    struct drm_device *dev = connector->dev;
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_connector_t *gf_connector = to_gf_connector(connector);
    gf_splice_manager_t *splice_manager = disp_info->splice_manager;
    int changed = 0;

    if (connector->polled == DRM_CONNECTOR_POLL_HPD)
    {
        gf_connector_detect_internal(connector, 0, 1);
    }
    else
    {
        gf_connector_detect_internal(connector, 0, 0);
    }

    if (gf_connector->source_status == connector_status_disconnected)
    {
        gf_info("source 0x%x plug out, exit splice mode\n", gf_connector->output_type);
        splice_manager->splice_enable = 0;
        gf_splice_reset_state(splice_manager, FALSE);
        changed = 1;
    }

    return changed;
}

void gf_splice_manager_handle_hpd(struct work_struct* work)
{
    gf_splice_manager_t *splice_manager = container_of(work, gf_splice_manager_t, splice_trigger_work);
    disp_info_t *disp_info = (disp_info_t *)splice_manager->private;
    gf_card_t *gf_card = (gf_card_t *)disp_info->gf_card;
    struct drm_device *dev = gf_card->drm_dev;
    struct drm_mode_config *mode_config = &dev->mode_config;
    gf_connector_t *gf_connector= NULL;
    struct drm_connector *connector = NULL;
    struct drm_crtc *target_crtc = splice_manager->target.crtc;
    int i = 0, ret = 0;

    if (splice_manager->trigger_value == 0 && splice_manager->splice_enable == 1)
    {
        mutex_lock(&mode_config->mutex);

        splice_manager->splice_enable = 0;

        gf_splice_reset_state(splice_manager, FALSE);

        mutex_unlock(&mode_config->mutex);

        drm_kms_helper_hotplug_event(dev);

        gf_info("splice mode exit\n");
    }
   else if (splice_manager->trigger_value && splice_manager->splice_enable == 0)
   {
        mutex_lock(&mode_config->mutex);

        ret = gf_splice_validate_source_params(splice_manager);
        if (ret)
        {
            gf_splice_reset_state(splice_manager, TRUE);
            mutex_unlock(&mode_config->mutex);
            return;
        }

        ret = gf_splice_validate_target_params(splice_manager);
        if (ret)
        {
            gf_splice_reset_state(splice_manager, TRUE);
            mutex_unlock(&mode_config->mutex);
            return;
        }

        for (i = 0; i < splice_manager->source_num; i++)
        {
            gf_splice_source_t *source = &(splice_manager->sources[i]);
            gf_splice_source_cfg_t *src_config = &(source->config);
            connector = source->connector;
            if (src_config->enable)
            {
                connector->status = connector_status_disconnected;
            }
        }

        splice_manager->target.connector->status = connector_status_connected;
        splice_manager->splice_enable = 1;

        mutex_unlock(&mode_config->mutex);

        drm_kms_helper_hotplug_event(dev);

        gf_info("splice mode enter\n");
    }
}


int gf_splice_manager_init(disp_info_t *disp_info, unsigned int crtc_index)
{
    gf_splice_manager_t *splice_manager = NULL;
    int ret  = 0;

    splice_manager = gf_calloc(sizeof(gf_splice_manager_t));
    if (!splice_manager)
    {
        DRM_ERROR("splice manager allocate fail\n");
        return -ENOMEM;
    }

    splice_manager->splice_enable = 0;

    disp_info->splice_manager = splice_manager;
    splice_manager->private = disp_info;

    splice_manager->trigger_value = 0;
    INIT_WORK(&splice_manager->splice_trigger_work, gf_splice_manager_handle_hpd);

    ret = gf_splice_init_sources(splice_manager);
    if (ret)
    {
        goto alloc_fail;
    }

    splice_manager->target.crtc_index = crtc_index;
    gf_splice_init_crtc_and_planes(splice_manager);

    gf_splice_init_encoders_and_connectors(splice_manager);

    return 0;

alloc_fail:
    gf_free(splice_manager->sources);
    splice_manager->sources = NULL;

    gf_free(splice_manager);
    disp_info->splice_manager = NULL;

    return ret;
}

void gf_splice_manager_deinit(disp_info_t *disp_info)
{
    gf_splice_manager_t *splice_manager = disp_info->splice_manager;

    if (!splice_manager)
    {
        return;
    }

    if (splice_manager->sources)
    {
        gf_free(splice_manager->sources);
    }
    splice_manager->sources = NULL;

    gf_free(splice_manager);
    disp_info->splice_manager = NULL;
}

void gf_atomic_commit_connector_prop(struct drm_device *dev, struct drm_atomic_state *old_state)
{
    struct drm_connector *connector = NULL;
    struct drm_connector_state *old_conn_state, *new_conn_state;
    unsigned int old_trg, new_trg;
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    int i = 0;

    for_each_oldnew_connector_in_state(old_state, connector, old_conn_state, new_conn_state, i)
    {
        old_trg = to_gf_conn_state(old_conn_state)->splice_trigger;
        new_trg = to_gf_conn_state(new_conn_state)->splice_trigger;

        if(to_gf_connector(connector)->output_type == DISP_OUTPUT_SPLICE
            && old_trg != new_trg && disp_info->splice_manager)
        {
            if(new_trg == 0)
            {
                gf_info("To trigger splice disable\n");
                disp_info->splice_manager->trigger_value = 0;
                schedule_work(&disp_info->splice_manager->splice_trigger_work);
            }
            else if((new_trg & disp_info->output_id_masks) == new_trg && (new_trg & (new_trg-1)) != 0)
            {
                gf_info("To trigger splice enable, value = 0x%x\n", new_trg);
                disp_info->splice_manager->trigger_value = new_trg;
                schedule_work(&disp_info->splice_manager->splice_trigger_work);
            }
            else
            {
                //not store invalid value
                to_gf_conn_state(new_conn_state)->splice_trigger = old_trg;
            }
        }
    }
}

#endif


