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
#include "gf_disp.h"
#include "gf_cbios.h"
#include "gf_atomic.h"
#include "gf_i2c.h"
#include "gf_kms.h"
#include "gf_sink.h"
#include "gf_splice.h"

gf_connector_t* gf_get_connector_by_device_id(disp_info_t*  disp_info, int device_id)
{
    gf_card_t *gf_card = (gf_card_t *)disp_info->gf_card;
    struct drm_device *drm = gf_card->drm_dev;
    gf_connector_t *gf_conn = NULL;
    struct drm_connector *connector = NULL;

    if(!drm || !device_id)
    {
        return gf_conn;
    }

    list_for_each_entry(connector, &drm->mode_config.connector_list, head)
    {
        if((to_gf_connector(connector))->output_type == device_id)
        {
            gf_conn = to_gf_connector(connector);
            break;
        }
    }

    return gf_conn;
}

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
    struct gf_sink_create_data sink_create_data = {0};
    bool sink_edid_valid = FALSE;

#ifdef ENABLE_HDMI4_VGA_ON_IGA4
    struct drm_connector* tmp_conn = NULL;
#endif

    if (gf_connector->output_type == DISP_OUTPUT_SPLICE)
    {
        return connector->funcs->detect(connector, force);
    }

#if GF_RUN_HDCP_CTS
    if((!full_detect) && (output & DISP_OUTPUT_DP_TYPES) && gf_connector->detected)
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
    gf_mutex_lock(gf_connector->conn_mutex);
    detected_output = disp_cbios_detect_connected_output(disp_info, output, full_detect);
    gf_mutex_unlock(gf_connector->conn_mutex);

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
            // fix kos brightness disbale. if crt detect by DAC and not edid, so restore previous edid in sink.
            if (!edid && gf_connector->output_type == DISP_OUTPUT_CRT)
            {
                gf_connector->edid_changed = 0;
            }
            else
            {
                gf_connector->edid_changed = 1;
            }
        }
        else if(gf_connector->compare_edid)
        {
            edid = disp_cbios_read_edid(disp_info, output);
            sink_edid_valid = gf_sink_is_edid_valid(gf_connector->sink);
            if ((edid && !sink_edid_valid) ||
                (!edid && sink_edid_valid) ||
                (edid && sink_edid_valid && gf_memcmp(gf_connector->sink->edid_data, edid, EDID_BUF_SIZE)))
            {
                gf_connector->edid_changed = 1;
            }
        }

        if(gf_connector->edid_changed)
        {
            if(gf_connector->sink)
            {
                gf_sink_put(gf_connector->sink);
                gf_connector->sink = NULL;
            }

            sink_create_data.output_type = gf_connector->output_type;
            gf_connector->sink = gf_sink_create(&sink_create_data);

            if (edid)
            {
                gf_memcpy(gf_connector->sink->edid_data, edid, EDID_BUF_SIZE);
            }

            disp_cbios_get_connector_attrib(disp_info, gf_connector);
        }

        if (edid)
        {
            gf_free(edid);
            edid = NULL;
        }
    }
    else
    {
        if(gf_connector->sink && gf_connector->output_type != DISP_OUTPUT_CRT)
        {
            gf_sink_put(gf_connector->sink);
            gf_connector->sink = NULL;
        }
    }

    if (is_connector_work_in_splice_mode(connector))
    {
        gf_connector->source_status = conn_status;
        return connector_status_disconnected;
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
    int real_num = 0, added_num = 0;
    int i = 0, skip_create = 0;
    void* cb_mode_list = NULL;
    int  output = gf_connector->output_type;
    struct drm_display_mode *drm_mode = NULL;
    struct edid *drm_edid = NULL;

    if (is_connector_work_in_splice_mode(connector))
    {
#if DRM_VERSION_CODE >= KERNEL_VERSION(4,19,0)
        drm_connector_update_edid_property(connector, NULL);
#else
        drm_mode_connector_update_edid_property(connector, NULL);
#endif

        goto END;
    }

    if (gf_sink_is_edid_valid(gf_connector->sink))
    {
       drm_edid = (struct edid*)gf_connector->sink->edid_data;
    }

#if DRM_VERSION_CODE >= KERNEL_VERSION(4,19,0)
    drm_connector_update_edid_property(connector, drm_edid);
#else
    drm_mode_connector_update_edid_property(connector, drm_edid);
#endif

    cb_mode_list = disp_cbios_get_device_modelist(disp_info, output, &real_num);

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
        if(DISP_OK != disp_cbios_cbmode_to_drmmode(disp_info, output, cb_mode_list, i, drm_mode))
        {
            skip_create = 1;
            continue;
        }

        skip_create = 0;
        drm_mode_set_name(drm_mode);
        drm_mode_probed_add(connector, drm_mode);
        added_num++;
    }
/*
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
        if(DISP_OK != disp_cbios_3dmode_to_drmmode(disp_info, output, mode_buf, i, drm_mode))
        {
            skip_create = 1;
            continue;
        }

        skip_create = 0;
        drm_mode_set_name(drm_mode);
        drm_mode_probed_add(connector, drm_mode);
        added_num++;
    }
*/
END:

    if(skip_create && drm_mode)
    {
        drm_mode_destroy(dev, drm_mode);
    }

    if(cb_mode_list)
    {
        gf_free(cb_mode_list);
        cb_mode_list = NULL;
    }

    return added_num;
}

static enum drm_mode_status
#if DRM_VERSION_CODE >= KERNEL_VERSION(6, 15, 0)
gf_connector_mode_valid(struct drm_connector *connector, const struct drm_display_mode *mode)
#else
gf_connector_mode_valid(struct drm_connector *connector, struct drm_display_mode *mode)
#endif
{
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

    drm_connector_unregister(connector);
    drm_connector_cleanup(connector);

    if(gf_connector->sink)
    {
        gf_sink_put(gf_connector->sink);
        gf_connector->sink = NULL;
    }

    gf_i2c_adapter_destroy(gf_connector->i2c_adapter);
    gf_connector->i2c_adapter= NULL;

    gf_destroy_mutex(gf_connector->conn_mutex);
    gf_connector->conn_mutex = NULL;

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
    .atomic_set_property = gf_connector_atomic_set_property,
    .atomic_get_property = gf_connector_atomic_get_property,
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

void disp_probe_connector_after_resume(struct drm_device *dev)
{
    struct  drm_connector* connector = NULL;
    gf_connector_t* gf_connector = NULL;
    enum drm_connector_status old_status;
    bool connector_changed = false;

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
    struct drm_connector_list_iter conn_iter;
#endif

    mutex_lock(&dev->mode_config.mutex);

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
    drm_connector_list_iter_begin(dev, &conn_iter);
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
    drm_for_each_connector_iter(connector, &conn_iter)
#else

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    drm_for_each_connector(connector, dev)
#else
    list_for_each_entry(connector, &dev->mode_config.connector_list, head)
#endif

#endif
    {
        gf_connector = to_gf_connector(connector);

        if (gf_connector->monitor_type != UT_OUTPUT_TYPE_PANEL)
        {
            old_status = connector->status;

            gf_connector->compare_edid = 1;
            connector->status = gf_connector_detect_internal(connector, 0, 0);

            if (gf_connector->edid_changed || old_status != connector->status)
            {
                connector_changed = true;
            }

            gf_connector->compare_edid = 0;
       }
    }

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
    drm_connector_list_iter_end(&conn_iter);
#endif

    mutex_unlock(&dev->mode_config.mutex);

    if (connector_changed)
    {
        drm_kms_helper_hotplug_event(dev);
    }
}


#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
static void disp_create_connector_property(disp_info_t* disp_info, gf_connector_t* gf_connector)
{
    struct drm_property *primary_card_prop, *global_id_proph, *global_id_propl;
    gf_card_t* gf_card = NULL;
    adapter_info_t* adapter_info = NULL;
    unsigned int primary = 0, pci_id = 0, internal_id = 0; //pci_id = (vend << 16) | device, internal_id = (bdf << 16) + internal_output_id

    if (!disp_info || !gf_connector)
    {
        return;
    }

    gf_card = disp_info->gf_card;
    adapter_info = disp_info->adp_info;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0)
    if(vga_default_device() == gf_card->pdev)
#endif
    {
        primary = 1;
    }

    pci_id = adapter_info->ven_dev;

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 2, 0)
    internal_id = (unsigned int)pci_dev_id(gf_card->pdev);
#else
    internal_id = (unsigned int)PCI_DEVID(gf_card->pdev->bus->number, gf_card->pdev->devfn);
#endif

    internal_id <<= 16;
    internal_id |= gf_connector->output_id;

    gf_info("Create property for dev 0x%x, primary = %d, pci_id = 0x%x, internal_id = 0x%x.\n", gf_connector->output_type, primary, pci_id, internal_id);

    if(!disp_info->primary_card_prop)
    {
        primary_card_prop = drm_property_create_bool(gf_card->drm_dev, DRM_MODE_PROP_IMMUTABLE, "primarycard");
        if(primary_card_prop)
        {
            disp_info->primary_card_prop = primary_card_prop;
        }
    }

    if(disp_info->primary_card_prop)
    {
        drm_object_attach_property(&gf_connector->base_connector.base, disp_info->primary_card_prop, primary);
    }

    if(!disp_info->global_id_proph)
    {
        global_id_proph = drm_property_create_range(gf_card->drm_dev, DRM_MODE_PROP_IMMUTABLE, "global_id_h", 0, UINT_MAX);
        if(global_id_proph)
        {
            disp_info->global_id_proph = global_id_proph;
        }
    }

    if(!disp_info->global_id_propl)
    {
        global_id_propl = drm_property_create_range(gf_card->drm_dev, DRM_MODE_PROP_IMMUTABLE, "global_id_l", 0, UINT_MAX);
        if(global_id_propl)
        {
            disp_info->global_id_propl = global_id_propl;
        }
    }

    if(disp_info->global_id_proph)
    {
        drm_object_attach_property(&gf_connector->base_connector.base, disp_info->global_id_proph, pci_id);
    }

    if(disp_info->global_id_propl)
    {
        drm_object_attach_property(&gf_connector->base_connector.base, disp_info->global_id_propl, internal_id);
    }

    gf_splice_attach_connector_property(&gf_connector->base_connector);
}
#endif

struct drm_connector* disp_connector_init(disp_info_t* disp_info, disp_output_type output)
{
    gf_card_t *gf_card = disp_info->gf_card;
    struct drm_device *drm = gf_card->drm_dev;
    struct drm_connector *connector = NULL;
    gf_connector_t *gf_connector = NULL;
    int  conn_type = 0;
    struct gf_i2c_adapter *gf_adapter = NULL;
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    gf_connector_state_t *gf_conn_state = NULL;
#endif

    gf_connector = gf_calloc(sizeof(gf_connector_t));
    if (!gf_connector)
    {
        DRM_ERROR("failed to alloc gf connector\n");
        goto failed;
    }

#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    gf_conn_state = gf_calloc(sizeof(gf_connector_state_t));
    if (!gf_conn_state)
    {
        DRM_ERROR("failed to alloc gf connector state\n");
        goto failed;
    }

    gf_connector->base_connector.state = &gf_conn_state->base_conn_state;
    gf_conn_state->base_conn_state.connector = &gf_connector->base_connector;
#endif

    gf_connector->output_type = output;
    if(gf_connector->output_type >= 0x100)
    {
        gf_connector->output_id = gf_connector->output_type >> 8;
    }
    else
    {
        gf_connector->output_id = gf_connector->output_type;
    }

    disp_info->output_id_masks |= gf_connector->output_id;

    connector = &gf_connector->base_connector;

    if (output == DISP_OUTPUT_CRT)
    {
        conn_type = DRM_MODE_CONNECTOR_VGA;
        connector->stereo_allowed = FALSE;
        connector->interlace_allowed = FALSE;
    }
    else if (output == DISP_OUTPUT_DP2)
    {
        conn_type = DRM_MODE_CONNECTOR_DisplayPort;
        connector->stereo_allowed = TRUE;
        connector->interlace_allowed = TRUE;
    }
    else if (output & (DISP_OUTPUT_DP_TYPES & (~DISP_OUTPUT_DP2)))
    {
        conn_type = DRM_MODE_CONNECTOR_HDMIA;
        connector->stereo_allowed = TRUE;
        connector->interlace_allowed = TRUE;
    }
    else
    {
        DRM_ERROR("Unknown output\n");
        goto failed;
    }

    gf_connector->conn_mutex = gf_create_mutex();

    gf_adapter = gf_i2c_adapter_create(drm, connector);
    if (IS_ERR(gf_adapter))
    {
        DRM_ERROR("failed to create zx i2c adapter\n");
        goto failed;
    }

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
    drm_connector_init_with_ddc(drm, connector, &gf_connector_funcs, conn_type, &gf_adapter->adapter);
#else
    drm_connector_init(drm, connector, &gf_connector_funcs, conn_type);
#endif

    drm_connector_helper_add(connector, &gf_connector_helper_funcs);

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

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    disp_create_connector_property(disp_info, gf_connector);
#endif

    return connector;

failed:
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    if (gf_conn_state)
    {
        gf_free(gf_conn_state);
        gf_conn_state = NULL;
    }
#endif

    if (gf_connector)
    {
        if (gf_connector->conn_mutex)
        {
            gf_destroy_mutex(gf_connector->conn_mutex);
            gf_connector->conn_mutex = NULL;
        }

        gf_free(gf_connector);
        gf_connector = NULL;
    }

    return NULL;
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)

static void __gf_restore_drm_connector_state(struct drm_connector *connector,
                                             struct drm_modeset_acquire_ctx *ctx)
{

    int ret = 0;
    struct drm_device *dev = connector->dev;
    struct drm_atomic_state *state = drm_atomic_state_alloc(dev);
    struct drm_crtc *crtc = connector->encoder->crtc;
    struct drm_plane *plane = crtc->primary;
    struct drm_connector_state *conn_state;
    struct drm_crtc_state *crtc_state;
    struct drm_plane_state *plane_state;

    if (!state)
    {
        return;
    }

    state->acquire_ctx = ctx;

    conn_state = drm_atomic_get_connector_state(state, connector);
    if (IS_ERR(conn_state))
    {
        ret = PTR_ERR(conn_state);
        goto out;
    }

    crtc_state = drm_atomic_get_crtc_state(state, crtc);
    if (IS_ERR(crtc_state))
    {
        ret = PTR_ERR(crtc_state);
        goto out;
    }

    crtc_state->mode_changed = TRUE;

    plane_state = drm_atomic_get_plane_state(state, plane);
    if (IS_ERR(plane_state))
    {
        ret = PTR_ERR(plane_state);
        goto out;
    }

    ret = drm_atomic_commit(state);

out:

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
    drm_atomic_state_put(state);
#else
    drm_atomic_state_free(state);
#endif

    if (ret)
    {
        DRM_DEBUG("Restoring old connector state failed with %d\n", ret);
    }
}

void gf_restore_drm_connector_state(struct drm_device *dev, struct drm_connector *connector,
                                    struct drm_modeset_acquire_ctx *ctx)
{
    gf_connector_t  *gf_connector =  to_gf_connector(connector);
    struct drm_crtc *crtc = NULL;
    gf_crtc_state_t *gf_crtc_state = NULL;

    if (!gf_connector->sink || !connector->encoder)
    {
        return;
    }

    crtc = connector->encoder->crtc;
    if (!crtc)
    {
        return;
    }

    gf_crtc_state = to_gf_crtc_state(crtc->state);

    if (gf_crtc_state->sink != gf_connector->sink)
    {
        gf_info("To restore connector mode after plug in.\n");
        __gf_restore_drm_connector_state(connector, ctx);
    }
}

#endif

