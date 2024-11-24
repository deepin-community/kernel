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

#include "gf_disp.h"
#include "gf_cbios.h"
#include "gf_atomic.h"
#include "gf_capture_drv.h"
#include "gf_splice.h"
#include "gf_audio.h"

static void gf_encoder_destroy(struct drm_encoder *encoder)
{
    gf_encoder_t *gf_encoder = to_gf_encoder(encoder);

    drm_encoder_cleanup(encoder);
    gf_free(gf_encoder);
}

static gf_connector_t* gf_encoder_get_connector(gf_encoder_t* gf_encoder)
{
    struct drm_encoder *encoder = &gf_encoder->base_encoder;
    struct drm_device *dev = encoder->dev;
    struct drm_connector *connector;
    gf_connector_t *gf_connector = NULL;

    list_for_each_entry(connector, &dev->mode_config.connector_list, head)
    {
        if (connector->encoder == encoder)
        {
            gf_connector = to_gf_connector(connector);
            break;
        }
    }

    return gf_connector;
}

void gf_encoder_disable(struct drm_encoder *encoder)
{
    struct drm_device *dev = encoder->dev;
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_encoder_t *gf_encoder = to_gf_encoder(encoder);
    gf_connector_t *gf_connector = gf_encoder_get_connector(gf_encoder);
    gf_capture_id_t cf_id = GF_CAPTURE_INVALID;

    if (!gf_connector)
    {
        return;
    }

    if (DISP_OUTPUT_DP1 == gf_encoder->output_type)
    {
        cf_id = GF_CAPTURE_WB1;
    }
    else if (DISP_OUTPUT_DP2 == gf_encoder->output_type)
    {
        cf_id = GF_CAPTURE_WB2;
    }
    else if (DISP_OUTPUT_DP3 == gf_encoder->output_type)
    {
        cf_id = GF_CAPTURE_WB3;
    }
    else if (DISP_OUTPUT_DP4 == gf_encoder->output_type)
    {
        cf_id = GF_CAPTURE_WB4;
    }

    if(gf_encoder->enc_dpms != GF_DPMS_OFF)
    {
        if (!(is_crtc_work_in_splice_mode(encoder->crtc) &&
             is_splice_target_active_in_drm(dev)))
        {

            gf_audio_set_connect(gf_connector, 0);

    #if GF_RUN_HDCP_CTS
            if (gf_connector->hdcp_enable)
            {
                disp_cbios_enable_hdcp(disp_info,FALSE, gf_encoder->output_type);
                gf_connector->hdcp_enable = 0;
            }
    #endif

            gf_info("To turn off power of device: 0x%x.\n", gf_encoder->output_type);

            gf_capture_handle_event(disp_info, cf_id, GF_CAPTURE_EVENT_SIGNAL_OFF);

            gf_mutex_lock(gf_connector->conn_mutex);
            disp_cbios_set_dpms(disp_info, gf_encoder->output_type, GF_DPMS_OFF);
            gf_mutex_unlock(gf_connector->conn_mutex);
        }

        gf_encoder->enc_dpms = GF_DPMS_OFF;
    }
}

void gf_encoder_enable(struct drm_encoder *encoder)
{
    struct drm_device *dev = encoder->dev;
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_encoder_t *gf_encoder = to_gf_encoder(encoder);
    gf_connector_t *gf_connector = gf_encoder_get_connector(gf_encoder);
    gf_capture_id_t cf_id = GF_CAPTURE_INVALID;

    if (!gf_connector)
    {
        return;
    }

    if (DISP_OUTPUT_DP1 == gf_encoder->output_type)
    {
        cf_id = GF_CAPTURE_WB1;
    }
    else if (DISP_OUTPUT_DP2 == gf_encoder->output_type)
    {
        cf_id = GF_CAPTURE_WB2;
    }
    else if (DISP_OUTPUT_DP3 == gf_encoder->output_type)
    {
        cf_id = GF_CAPTURE_WB3;
    }
    else if (DISP_OUTPUT_DP4 == gf_encoder->output_type)
    {
        cf_id = GF_CAPTURE_WB4;
    }

    if(gf_encoder->enc_dpms != GF_DPMS_ON)
    {
        gf_info("To turn on power of device: 0x%x.\n", gf_encoder->output_type);

        gf_mutex_lock(gf_connector->conn_mutex);
        disp_cbios_set_dpms(disp_info, gf_encoder->output_type, GF_DPMS_ON);
        gf_mutex_unlock(gf_connector->conn_mutex);
        gf_encoder->enc_dpms = GF_DPMS_ON;

        gf_audio_set_connect(gf_connector, 1);

#if GF_RUN_HDCP_CTS
        if ((!(gf_connector->hdcp_enable))
            &&(gf_connector->monitor_type == UT_OUTPUT_TYPE_HDMI || gf_connector->monitor_type == UT_OUTPUT_TYPE_DVI))
        {
            disp_cbios_enable_hdcp(disp_info, TRUE,  gf_encoder->output_type);
            gf_connector->hdcp_enable = 1;
        }
#endif

        gf_capture_handle_event(disp_info, cf_id, GF_CAPTURE_EVENT_SIGNAL_ON);
    }
}

bool gf_encoder_mode_fixup_internal(disp_info_t*  disp_info,
                                   int output_type,
                                   const struct drm_display_mode *mode,
                                   struct drm_display_mode *adjusted_mode)
{
    unsigned int dev_mode_size = 0, dev_real_num = 0, i = 0, matched = 0;
    void * dev_mode_buf = NULL;
    PCBiosModeInfoExt pcbios_mode = NULL, matched_mode = NULL;

    dev_mode_size = disp_cbios_get_modes_size(disp_info, output_type);
    if(dev_mode_size)
    {
        dev_mode_buf = gf_calloc(dev_mode_size);
        if(dev_mode_buf)
        {
            dev_real_num = disp_cbios_get_modes(disp_info, output_type, dev_mode_buf, dev_mode_size);
            for(i = 0; i < dev_real_num; i++)
            {
                pcbios_mode = (PCBiosModeInfoExt)dev_mode_buf + i;
                if((pcbios_mode->XRes == mode->hdisplay) &&
                   (pcbios_mode->YRes == mode->vdisplay) &&
                   (pcbios_mode->RefreshRate/100 == drm_mode_vrefresh(mode)) &&
                   ((mode->flags & DRM_MODE_FLAG_INTERLACE) ? (pcbios_mode->InterlaceProgressiveCaps == 0x02) : (pcbios_mode->InterlaceProgressiveCaps == 0x01)))
                {
                    matched = 1;
                    break;
                }
            }
        }
    }

    if(!matched && disp_info->scale_support)
    {
        for(i = 0; i < dev_real_num; i++)
        {
            pcbios_mode = (PCBiosModeInfoExt)dev_mode_buf + i;
            if(pcbios_mode->XRes >= mode->hdisplay && pcbios_mode->YRes >= mode->vdisplay &&
               pcbios_mode->RefreshRate/100 >= drm_mode_vrefresh(mode))
            {
                if(pcbios_mode->isPreferredMode)
                {
                    matched_mode = pcbios_mode;
                    break;
                }
                else if(!matched_mode)
                {
                    matched_mode = pcbios_mode;
                }
            }
        }

        if(matched_mode)
        {
            if(adjusted_mode)
            {
                disp_cbios_cbmode_to_drmmode(disp_info, output_type, matched_mode, 0, adjusted_mode);
            }
            matched = 1;
        }
    }

    if (adjusted_mode && matched)
    {
#if DRM_VERSION_CODE < KERNEL_VERSION(5,9,0)
        adjusted_mode->vrefresh = drm_mode_vrefresh(adjusted_mode);
#endif
        disp_cbios_get_mode_timing(disp_info, output_type, adjusted_mode);
    }

    if (dev_mode_buf)
    {
        gf_free(dev_mode_buf);
        dev_mode_buf = NULL;
    }

    return (matched)? TRUE : FALSE;
}

static bool gf_encoder_mode_fixup(struct drm_encoder *encoder,
                                   const struct drm_display_mode *mode,
                                   struct drm_display_mode *adjusted_mode)
{
    struct drm_device* dev = encoder->dev;
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;
    gf_encoder_t *gf_encoder = to_gf_encoder(encoder);

    return gf_encoder_mode_fixup_internal(disp_info, gf_encoder->output_type, mode, adjusted_mode);
}

#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)

void gf_encoder_atomic_mode_set(struct drm_encoder *encoder,
                                struct drm_crtc_state *crtc_state,
                                struct drm_connector_state *conn_state)
{
    struct drm_device* dev = encoder->dev;
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;
    struct drm_display_mode* mode = &crtc_state->mode;
    struct drm_display_mode* adj_mode = &crtc_state->adjusted_mode;
    struct drm_crtc*  crtc = NULL;
    int  flag = 0;

    //in atomic set phase, atomic state is updated to state of crtc/encoder/connector,
    //so we can't roll back mode setting, that means all parameter check should be placed in
    //atomic check function, and now all para is correct, we only need flush them to HW register
    //but we still add para check code here tempararily, it will be removed after code stable.

    crtc = encoder->crtc;

    if (!crtc)
    {
        DRM_ERROR("crtc is NULL\n");
        return;
    }

    DRM_DEBUG_KMS("encoder=%d,crtc=%d\n", encoder->index, crtc->index);
    flag |= UPDATE_ENCODER_MODE_FLAG;

    disp_cbios_set_mode(disp_info, drm_crtc_index(crtc), mode, adj_mode, flag);
}

#else

void  gf_encoder_mode_set(struct drm_encoder *encoder,
                          struct drm_display_mode *mode,
                          struct drm_display_mode *adjusted_mode)
{
    struct drm_device* dev = encoder->dev;
    struct drm_crtc* crtc = encoder->crtc;
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;
    int flag = 0;

    if(!crtc)
    {
        gf_assert(0, GF_FUNC_NAME(__func__));
    }

    flag = UPDATE_ENCODER_MODE_FLAG;

    disp_cbios_set_mode(disp_info, to_gf_crtc(crtc)->pipe, mode, adjusted_mode, flag);
}

#endif

static const struct drm_encoder_funcs gf_encoder_funcs =
{
    .destroy = gf_encoder_destroy,
};

#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)

static const struct drm_encoder_helper_funcs gf_encoder_helper_funcs =
{
    .disable = gf_encoder_disable,
    .enable = gf_encoder_enable,
    .mode_fixup = gf_encoder_mode_fixup,
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
    .atomic_mode_set = gf_encoder_atomic_mode_set,
#endif
};

#else

static const struct drm_encoder_helper_funcs gf_encoder_helper_funcs =
{
    .mode_fixup = gf_encoder_mode_fixup,
    .prepare = gf_encoder_disable,
    .commit = gf_encoder_enable,
    .mode_set = gf_encoder_mode_set,
    .disable = gf_encoder_disable,
};

#endif

struct drm_encoder* disp_encoder_init(disp_info_t* disp_info, disp_output_type output)
{
    gf_card_t*  gf_card = disp_info->gf_card;
    struct drm_device* drm = gf_card->drm_dev;
    struct drm_encoder* encoder = NULL;
    gf_encoder_t* gf_encoder = NULL;
    int encoder_type = 0;

    gf_encoder = gf_calloc(sizeof(gf_encoder_t));
    if (!gf_encoder)
    {
        return NULL;
    }

    encoder = &gf_encoder->base_encoder;
    encoder->possible_clones = 0;
    encoder->possible_crtcs = disp_cbios_get_crtc_mask(disp_info, output);

    switch (output)
    {
    case DISP_OUTPUT_CRT:
        encoder_type = DRM_MODE_ENCODER_DAC;
        break;
    case DISP_OUTPUT_DP1:
    case DISP_OUTPUT_DP2:
    case DISP_OUTPUT_DP3:
    case DISP_OUTPUT_DP4:
        encoder_type = DRM_MODE_ENCODER_TMDS;
        break;
    default:
        encoder_type = DRM_MODE_ENCODER_NONE;
        break;
    }

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)  || defined PHYTIUM_2000
    drm_encoder_init(drm, encoder, &gf_encoder_funcs, encoder_type, NULL);
#else
    drm_encoder_init(drm, encoder, &gf_encoder_funcs, encoder_type);
#endif
    drm_encoder_helper_add(encoder, &gf_encoder_helper_funcs);
    gf_encoder->output_type = output;
    gf_encoder->enc_dpms = GF_DPMS_OFF;

    return encoder;
}
