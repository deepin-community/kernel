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

enum drm_connector_status
gf_connector_detect_internal(struct drm_connector *connector, bool force, int full_detect)
{
    struct drm_device *dev = connector->dev;
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_connector_t *gf_connector = to_gf_connector(connector);
    disp_output_type output = gf_connector->output_type;
    int detected_output = 0;
    unsigned char* edid = NULL;
    enum drm_connector_status conn_status;

#ifdef ENABLE_HDMI4_VGA_ON_IGA4
        struct drm_connector* tmp_conn = NULL;
#endif

#if GF_RUN_HDCP_CTS
    if((!full_detect)&&(output & DISP_OUTPUT_DP_TYPES)&&gf_connector->detected)
    {
        conn_status = connector->status;
    }
    else
    {
        if(gf_connector->hpd_out &&(output & DISP_OUTPUT_DP_TYPES))
        {
            gf_connector->hpd_out = 0;
            conn_status = connector_status_disconnected;
        }
        else
        {
            detected_output = disp_cbios_detect_connected_output(disp_info, output, full_detect);
            conn_status = (detected_output & output)? connector_status_connected : connector_status_disconnected;
            gf_connector->edid_changed = 0;
            gf_connector->detected = 1;
        }
    }
#else
    detected_output = disp_cbios_detect_connected_output(disp_info, output, full_detect);

    conn_status = (detected_output & output)? connector_status_connected : connector_status_disconnected;

    gf_connector->edid_changed = 0;
#endif

#ifdef ENABLE_HDMI4_VGA_ON_IGA4
        if(gf_connector->output_type == disp_info->conflict_low && disp_info->conflict_high && conn_status == connector_status_connected)
        {
            list_for_each_entry(tmp_conn, &dev->mode_config.connector_list, head)
            {
                if(to_gf_connector(tmp_conn)->output_type == disp_info->conflict_high && 
                   tmp_conn->status == connector_status_connected)
                {
                    conn_status = connector_status_disconnected;
                    break;
                }
            }
        }
#endif

    if(conn_status == connector_status_connected)
    {
        if(connector->status != connector_status_connected)
        {
            edid = disp_cbios_read_edid(disp_info, output);
            gf_connector->edid_changed = 1;
        }
        else if(gf_connector->compare_edid)
        {
            edid = disp_cbios_read_edid(disp_info, output);
            if((!gf_connector->edid_data && edid) ||
                (gf_connector->edid_data && !edid) ||
                (gf_connector->edid_data && edid && gf_memcmp(gf_connector->edid_data, edid, EDID_BUF_SIZE)))
            {
                gf_connector->edid_changed = 1;
            }
        }
        
        if(gf_connector->edid_changed)
        {
            if(gf_connector->edid_data)
            {
                gf_free(gf_connector->edid_data);
            }
            gf_connector->edid_data = edid;
            disp_cbios_get_connector_attrib(disp_info, gf_connector);
        }
        else if(edid)
        {
            gf_free(edid);
            edid = NULL;
        }
    }
    else
    {
        if(gf_connector->edid_data)
        {
            gf_free(gf_connector->edid_data);
            gf_connector->edid_data = NULL;
        }
        gf_connector->monitor_type = UT_OUTPUT_TYPE_NONE;
        gf_connector->support_audio = 0;
    }

    return conn_status;
}

static enum drm_connector_status
gf_connector_detect(struct drm_connector *connector, bool force)
{
    return gf_connector_detect_internal(connector, force, 0);
}

static int gf_connector_get_modes(struct drm_connector *connector)
{
    struct drm_device *dev = connector->dev;
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_connector_t *gf_connector = to_gf_connector(connector);
    int dev_mode_size = 0, adapter_mode_size = 0, dev_real_num = 0, adapter_real_num = 0, real_num = 0, added_num = 0;
    int i = 0, skip_create = 0;
    void* mode_buf = NULL;
    void* adapter_mode_buf = NULL; 
    void* merge_mode_buf = NULL;
    void* cb_mode_list = NULL;
    int  output = gf_connector->output_type;
    struct drm_display_mode *drm_mode = NULL;

    dev_mode_size = disp_cbios_get_modes_size(disp_info, output);

    if(!dev_mode_size)
    {
        goto END;
    }

    mode_buf = gf_calloc(dev_mode_size);

    if(!mode_buf)
    {
        goto END;
    }

#if DRM_VERSION_CODE >= KERNEL_VERSION(4,19,0)
    drm_connector_update_edid_property(connector, (struct edid*)gf_connector->edid_data);
#else
    drm_mode_connector_update_edid_property(connector, (struct edid*)gf_connector->edid_data);
#endif
    
    dev_real_num = disp_cbios_get_modes(disp_info, output, mode_buf, dev_mode_size);

    cb_mode_list = mode_buf;
    real_num = dev_real_num;
    
    if(disp_info->scale_support)
    {
        adapter_mode_size = disp_cbios_get_adapter_modes_size(disp_info);
    
        if(adapter_mode_size != 0)
        {
            adapter_mode_buf = gf_calloc(adapter_mode_size);
        
            if(!adapter_mode_buf)
            {
                goto END;
            }
            adapter_real_num = disp_cbios_get_adapter_modes(disp_info, adapter_mode_buf, adapter_mode_size);
        
            merge_mode_buf = gf_calloc((dev_real_num + adapter_real_num) * sizeof(CBiosModeInfoExt));
    
            if(!merge_mode_buf)
            {
                goto END;
            }
        
            real_num = disp_cbios_merge_modes(merge_mode_buf, adapter_mode_buf, adapter_real_num, mode_buf, dev_real_num);
            cb_mode_list = merge_mode_buf;
        }
    }

    for (i = 0; i < real_num; i++)
    {
        if(!skip_create)
        {
            drm_mode = drm_mode_create(dev);
        }
        if(!drm_mode)
        {
            skip_create = 0;
            break;
        }
        if(S_OK != disp_cbios_cbmode_to_drmmode(disp_info, output, cb_mode_list, i, drm_mode))
        {
            skip_create = 1;
            continue;
        }
        
        skip_create = 0;
        drm_mode_set_name(drm_mode);
        drm_mode_probed_add(connector, drm_mode);
        added_num++;
    }

    gf_free(mode_buf);
    mode_buf = NULL;

    dev_mode_size = disp_cbios_get_3dmode_size(disp_info, output);
    mode_buf = gf_calloc(dev_mode_size);
    if(!mode_buf)
    {
        goto END;
    }

    real_num = disp_cbios_get_3dmodes(disp_info, output, mode_buf, dev_mode_size);

    for(i = 0; i < real_num; i++)
    {
        if(!skip_create)
        {
            drm_mode = drm_mode_create(dev);
        }
        if(!drm_mode)
        {
            skip_create = 0;
            break;
        }
        if(S_OK != disp_cbios_3dmode_to_drmmode(disp_info, output, mode_buf, i, drm_mode))
        {
            skip_create = 1;
            continue;
        }

        skip_create = 0;
        drm_mode_set_name(drm_mode);
        drm_mode_probed_add(connector, drm_mode);
        added_num++;
    }

END:

    if(skip_create && drm_mode)
    {
        drm_mode_destroy(dev, drm_mode);
    }

    if(mode_buf)
    {
        gf_free(mode_buf);
        mode_buf = NULL;
    }

    if(adapter_mode_buf)
    {
        gf_free(adapter_mode_buf);
        adapter_mode_buf = NULL;
    }

    if(merge_mode_buf)
    {
        gf_free(merge_mode_buf);
        merge_mode_buf = NULL;
    }

    return added_num;
}

enum drm_mode_status 
gf_connector_mode_valid(struct drm_connector *connector, struct drm_display_mode *mode)
{
    struct drm_device *dev = connector->dev;
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    adapter_info_t *adp_info = disp_info->adp_info;
    gf_connector_t *gf_connector = to_gf_connector(connector);
    int max_clock;

    if (gf_connector->output_type == DISP_OUTPUT_CRT)
    {
        max_clock = 400000;    // 400 MHz
    }
    else
    {
        max_clock = 600000;    // 600 MHz
    }

    if (mode->clock > max_clock)
    {
        return MODE_CLOCK_HIGH;
    }

    return MODE_OK;
}

static void gf_connector_destroy(struct drm_connector *connector)
{
    gf_connector_t *gf_connector = to_gf_connector(connector);

    if(gf_connector->edid_data)
    {
        gf_free(gf_connector->edid_data);
        gf_connector->edid_data = NULL;
    }
    drm_connector_unregister(connector);
    drm_connector_cleanup(connector);
    gf_free(gf_connector);
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 3, 0)
static int gf_connector_dpms(struct drm_connector *connector, int mode)
#else
static void gf_connector_dpms(struct drm_connector *connector, int mode)
#endif
{
    if(connector->encoder)
    {
        if(mode == DRM_MODE_DPMS_ON)
        {
            gf_encoder_enable(connector->encoder);
        }
        else
        {
            gf_encoder_disable(connector->encoder);
        }

        connector->dpms = mode;
    }  

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 3, 0)
    return 0;
#endif
}

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
//in gf chip, connector is fixed to encoder, so we just pick up the 1st encoder from table
static struct drm_encoder *gf_best_encoder(struct drm_connector *connector)
{
    struct drm_mode_object* obj = NULL;
    int encoder_id = connector->encoder_ids[0];

    obj = drm_mode_object_find(connector->dev, encoder_id, DRM_MODE_OBJECT_ENCODER);
    gf_assert(obj != NULL, "obj = NULL");

    return  obj_to_encoder(obj);
}
#endif

static const struct drm_connector_funcs gf_connector_funcs = 
{
    .detect = gf_connector_detect,
    .dpms = gf_connector_dpms,
    .fill_modes = drm_helper_probe_single_connector_modes,
    .destroy = gf_connector_destroy,
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    .atomic_destroy_state = gf_connector_destroy_state,
    .atomic_duplicate_state = gf_connector_duplicate_state,
#endif
};

static const struct drm_connector_helper_funcs gf_connector_helper_funcs = 
{
    .get_modes = gf_connector_get_modes,
    .mode_valid = gf_connector_mode_valid,
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
    .best_encoder = gf_best_encoder,
#endif
};

struct drm_connector* disp_connector_init(disp_info_t* disp_info, disp_output_type output)
{
    gf_card_t*  gf_card = disp_info->gf_card;
    struct drm_device*  drm = gf_card->drm_dev;
    struct drm_connector*  connector = NULL;
    gf_connector_t* gf_connector = NULL;
    int  conn_type = 0;

    gf_connector = gf_calloc(sizeof(gf_connector_t));
    if (!gf_connector)
    {
        return NULL;
    }

#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    {
        gf_connector_state_t* gf_conn_state = NULL;
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
    }
#endif
    connector = &gf_connector->base_connector;

    if (output == DISP_OUTPUT_CRT)
    {
        conn_type = DRM_MODE_CONNECTOR_VGA;
        connector->stereo_allowed = FALSE;
        connector->interlace_allowed = FALSE;
    }
    else if ((output == DISP_OUTPUT_DP1) || (output == DISP_OUTPUT_DP2) ||
                 (output == DISP_OUTPUT_DP3) || (output == DISP_OUTPUT_DP4))
    {
        conn_type = DRM_MODE_CONNECTOR_DisplayPort;
        connector->stereo_allowed = TRUE;
        connector->interlace_allowed = TRUE;
    }
    else
    {
        conn_type = DRM_MODE_CONNECTOR_Unknown;
    }
    
    drm_connector_init(drm, connector, &gf_connector_funcs, conn_type);
    drm_connector_helper_add(connector, &gf_connector_helper_funcs);
    gf_connector->output_type = output;
    connector->doublescan_allowed = FALSE;

    if(output & disp_info->supp_polling_outputs)
    {
        connector->polled = DRM_CONNECTOR_POLL_CONNECT | DRM_CONNECTOR_POLL_DISCONNECT;
    }
    else if(output & disp_info->supp_hpd_outputs)
    {
        connector->polled = DRM_CONNECTOR_POLL_HPD;
        if(output == DISP_OUTPUT_DP1)
        {
            gf_connector->hpd_int_bit = INT_DP1;
            gf_connector->hda_codec_index = (1 << 0);
            gf_connector->hdcp_index = (1 << 0);
        }
        else if(output == DISP_OUTPUT_DP2)
        {
            gf_connector->hpd_int_bit = INT_DP2;
            gf_connector->hda_codec_index = (1 << 1);
            gf_connector->hdcp_index = (1 << 1);
        }
        else if(output == DISP_OUTPUT_DP3)
        {
            gf_connector->hpd_int_bit = INT_DP3;
            gf_connector->hdcp_index = (1 << 2);
        }
        else if(output == DISP_OUTPUT_DP4)
        {
            gf_connector->hpd_int_bit = INT_DP4;
            gf_connector->hdcp_index = (1 << 3);
        }
        gf_connector->hdcp_enable = 0;
        gf_connector->detected = 0;
        gf_connector->hpd_out = 0;
    }

    return connector;
}



