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
#include "gf_crtc.h"
#include "gf_modifies.h"
#include "gf_plane.h"
#include "gf_drmfb.h"
#include "gf_irq.h"
#include "gf_fbdev.h"
#include "gf_capture_drv.h"
#include "gf_splice.h"
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
#include <drm/drm_plane_helper.h>
#endif

static const struct drm_mode_config_funcs  gf_kms_mode_funcs = {
    .fb_create = gf_fb_create,
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
    .output_poll_changed = gf_fbdev_poll_changed,
#else
#if DRM_VERSION_CODE < KERNEL_VERSION(6, 12, 0)
    .output_poll_changed = drm_fb_helper_output_poll_changed,
#endif
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    .atomic_check = drm_atomic_helper_check,
    .atomic_commit = drm_atomic_helper_commit,
    .atomic_state_alloc = gf_atomic_state_alloc,
    .atomic_state_clear = gf_atomic_state_clear,
    .atomic_state_free   = gf_atomic_state_free,
#endif
};

static const int chx_plane_formats[] = {
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

static const int chx_cursor_formats[] = {
    DRM_FORMAT_XRGB8888,
    DRM_FORMAT_ARGB8888,
};

static  char*  plane_name[] = {
    "PS",
    "SS",
    "TS",
    "FS",
};

static const unsigned int vsync_int_tbl[] = {
    INT_VSYNC1,
    INT_VSYNC2,
    INT_VSYNC3,
    INT_VSYNC4,
};
#define VSYNC_INT_TABLE_LEN (sizeof(vsync_int_tbl)/sizeof(vsync_int_tbl[0]))

static char*  cursor_name = "cursor";

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
static const uint64_t chx_cursor_modifiers[] = {
    DRM_FORMAT_MOD_GF_LINEAR,
    DRM_FORMAT_MOD_GF_INVALID
};

static const uint64_t chx_plane_modifiers[] = {
    DRM_FORMAT_MOD_GF_DISPLAY,
    DRM_FORMAT_MOD_GF_LINEAR,
    DRM_FORMAT_MOD_GF_INVALID
};

#endif

#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)

#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
static const struct drm_mode_config_helper_funcs  gf_kms_mode_helper_funcs = {
#else
static struct drm_mode_config_helper_funcs  gf_kms_mode_helper_funcs = {
#endif
    .atomic_commit_tail = gf_atomic_helper_commit_tail,
};

static const struct drm_plane_funcs gf_plane_funcs = {
    .update_plane = gf_atomic_helper_update_plane,
    .disable_plane = gf_atomic_helper_disable_plane,
    .destroy = gf_plane_destroy,
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
    .set_property = drm_atomic_helper_plane_set_property,
#endif
    .atomic_duplicate_state = gf_plane_duplicate_state,
    .atomic_destroy_state = gf_plane_destroy_state,
    .atomic_set_property = gf_plane_atomic_set_property,
    .atomic_get_property = gf_plane_atomic_get_property,
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
    .format_mod_supported = gf_plane_format_mod_supported,
#endif
};

static  const struct drm_plane_helper_funcs gf_plane_helper_funcs = {
    .prepare_fb = gf_prepare_plane_fb,
    .cleanup_fb = gf_cleanup_plane_fb,
    .atomic_check = gf_plane_atomic_check,
    .atomic_update = gf_plane_atomic_update,
    .atomic_disable = gf_plane_atomic_disable,
};

static const struct drm_crtc_funcs gf_crtc_funcs = {
#if DRM_VERSION_CODE < KERNEL_VERSION(5, 12, 0)
    .gamma_set = drm_atomic_helper_legacy_gamma_set,
#endif
    .destroy = gf_crtc_destroy,
    .set_config = drm_atomic_helper_set_config,
    .page_flip  = drm_atomic_helper_page_flip,
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
    .set_property = drm_atomic_helper_crtc_set_property,
#endif
    .atomic_duplicate_state = gf_crtc_duplicate_state,
    .atomic_destroy_state = gf_crtc_destroy_state,
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
    .get_vblank_counter = gf_get_vblank_counter,
    .enable_vblank = gf_enable_vblank,
    .disable_vblank = gf_disable_vblank,
    .get_vblank_timestamp = drm_crtc_vblank_helper_get_vblank_timestamp,
#endif
};

static const struct drm_crtc_helper_funcs gf_helper_funcs = {
    .mode_set_nofb = gf_crtc_helper_set_mode,
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
    .enable = gf_crtc_helper_enable,
#endif
    .disable = gf_crtc_helper_disable,
    .atomic_check = gf_crtc_helper_check,
    .atomic_begin = gf_crtc_atomic_begin,
    .atomic_flush = gf_crtc_atomic_flush,
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
    .atomic_enable = gf_crtc_atomic_enable,
#endif
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
    .atomic_disable = gf_crtc_atomic_disable,
#endif
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
    .get_scanout_position = gf_crtc_get_scanout_position,
#endif
};

#else

static const struct drm_plane_funcs gf_plane_funcs = {
    .update_plane = gf_update_plane,
    .disable_plane = gf_disable_plane,
    .destroy = gf_legacy_plane_destroy,
};

static const struct drm_crtc_funcs gf_crtc_funcs = {
    .cursor_set = gf_crtc_cursor_set,
    .cursor_move = gf_crtc_cursor_move,
    .gamma_set = gf_crtc_gamma_set,
    .set_config = gf_crtc_helper_set_config,
    .destroy = gf_crtc_destroy,
    .page_flip = gf_crtc_page_flip,
};

static const struct drm_crtc_helper_funcs gf_helper_funcs = {
    .prepare = gf_crtc_prepare,
    .commit = gf_crtc_commit,
    .mode_fixup = gf_crtc_mode_fixup,
    .mode_set  = gf_crtc_mode_set,
    .mode_set_base = gf_crtc_mode_set_base,
    .disable = gf_crtc_disable,
};

#endif

static  void  disp_info_pre_init(disp_info_t*  disp_info)
{
    unsigned int i = 0;

    disp_info->cbios_lock = gf_create_mutex();

    disp_info->intr_lock = gf_create_spinlock(0);
    disp_info->hpd_lock = gf_create_spinlock(0);
    disp_info->hda_lock = gf_create_spinlock(0);
    disp_info->hdcp_lock = gf_create_spinlock(0);
    disp_info->gamma_lock = gf_create_mutex();

    disp_info->cbios_inner_spin_lock = gf_create_spinlock(0);
    disp_info->cbios_aux_mutex = gf_create_mutex();

    for(i = 0;i < MAX_I2CBUS;i++)
    {
        disp_info->cbios_i2c_mutex[i] = gf_create_mutex();
    }

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 35)
    disp_info->wq = create_workqueue("arise");
#endif
}

static  void  disp_info_deinit(disp_info_t*  disp_info)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 35)
    destroy_workqueue(disp_info->wq);
    disp_info->wq = NULL;
#endif

    gf_destroy_mutex(disp_info->cbios_lock);
    disp_info->cbios_lock = NULL;

    gf_destroy_spinlock(disp_info->intr_lock);
    disp_info->intr_lock = NULL;

    gf_destroy_spinlock(disp_info->hpd_lock);
    disp_info->hpd_lock = NULL;

    gf_destroy_spinlock(disp_info->hda_lock);
    disp_info->hda_lock = NULL;

    gf_destroy_spinlock(disp_info->hdcp_lock);
    disp_info->hdcp_lock = NULL;

    gf_destroy_mutex(disp_info->gamma_lock);
    disp_info->gamma_lock = NULL;

#if 0
    int i = 0;
    gf_destroy_spinlock(disp_info->cbios_inner_spin_lock);
    disp_info->cbios_inner_spin_lock = NULL;

    gf_destroy_mutex(disp_info->cbios_aux_mutex);
    disp_info->cbios_aux_mutex = NULL;

    for(i = 0;i < MAX_I2CBUS;i++)
    {
        gf_destroy_mutex(disp_info->cbios_i2c_mutex[i]) ;
        disp_info->cbios_i2c_mutex[i] = NULL;
    }
#endif

}

#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
int disp_create_blend_mode_property(struct drm_plane *plane, unsigned int supported_modes)
{
    struct drm_device *dev = plane->dev;
    struct drm_property *prop;
    gf_plane_t* gf_plane = to_gf_plane(plane);
    static const struct drm_prop_enum_list props[] = {
        { DRM_MODE_BLEND_PIXEL_NONE, "None" },
        { DRM_MODE_BLEND_PREMULTI, "Pre-multiplied" },
        { DRM_MODE_BLEND_COVERAGE, "Coverage" },
    };
    unsigned int valid_mode_mask = BIT(DRM_MODE_BLEND_PIXEL_NONE) |
                                                              BIT(DRM_MODE_BLEND_PREMULTI)   |
                                                              BIT(DRM_MODE_BLEND_COVERAGE);
    int i = 0, j = 0;

    if ((supported_modes & ~valid_mode_mask) ||((supported_modes & BIT(DRM_MODE_BLEND_PREMULTI)) == 0))
    {
        return -EINVAL;
    }

    prop = drm_property_create(dev, DRM_MODE_PROP_ENUM, "pixel blend mode", hweight32(supported_modes));
    if (!prop)
    {
        return -ENOMEM;
    }

    for (i = 0; i < ARRAY_SIZE(props); i++)
    {
        int ret;

        if (!(BIT(props[i].type) & supported_modes))
        {
            continue;
        }

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 18, 0)
        ret = drm_property_add_enum(prop, props[i].type, props[i].name);
#else
        ret = drm_property_add_enum(prop, j++, props[i].type, props[i].name);
#endif

        if (ret)
        {
            drm_property_destroy(dev, prop);
            return ret;
        }
    }

    drm_object_attach_property(&plane->base, prop, DRM_MODE_BLEND_PREMULTI);
    gf_plane->blend_mode_property = prop;

    if(plane->state)
    {
        to_gf_plane_state(plane->state)->pixel_blend_mode = DRM_MODE_BLEND_PREMULTI;
    }

    return 0;
}
#endif

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 18, 0)
int disp_create_alpha_property(struct drm_plane *plane)
{
    struct drm_property *prop;
    gf_plane_t* gf_plane = to_gf_plane(plane);

    prop = drm_property_create_range(plane->dev, 0, "alpha", 0, DRM_BLEND_ALPHA_OPAQUE);
    if (!prop)
    {
        return -ENOMEM;
    }

    drm_object_attach_property(&plane->base, prop, DRM_BLEND_ALPHA_OPAQUE);
    gf_plane->alpha_property = prop;

    if (plane->state)
    {
        to_gf_plane_state(plane->state)->alpha = DRM_BLEND_ALPHA_OPAQUE;
    }

    return 0;
}
#endif

int disp_create_alpha_source_property(struct drm_plane* plane)
{
    struct drm_device *dev = plane->dev;
    gf_plane_t* gf_plane = to_gf_plane(plane);
    struct drm_property *prop;
    int i = 0;

    static const struct drm_prop_enum_list  props[] = {
        { DRM_MODE_BLEND_CURR_LAYER_ALPHA, "Use alpha of current layer" },
        { DRM_MODE_BLEND_LOWER_LAYER_ALPHA, "Use alpha of lower layer" },
    };

    prop = drm_property_create(dev, DRM_MODE_PROP_ENUM, "blend alpha source", ARRAY_SIZE(props));
    if (!prop)
    {
        return -ENOMEM;
    }

    for (i = 0; i < ARRAY_SIZE(props); i++)
    {
        int ret;

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 18, 0)
        ret = drm_property_add_enum(prop, props[i].type, props[i].name);
#else
        ret = drm_property_add_enum(prop, i, props[i].type, props[i].name);
#endif

        if (ret)
        {
            drm_property_destroy(dev, prop);
            return ret;
        }
    }

    drm_object_attach_property(&plane->base, prop, DRM_MODE_BLEND_CURR_LAYER_ALPHA);
    gf_plane->alpha_source_prop = prop;

    if(plane->state)
    {
        to_gf_plane_state(plane->state)->alpha_source = DRM_MODE_BLEND_CURR_LAYER_ALPHA;
    }

    return 0;
}

void disp_create_plane_property(struct drm_device* dev, gf_plane_t* gf_plane)
{
    int  zpos = 0;

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 13, 0)
    drm_plane_create_rotation_property(&gf_plane->base_plane, DRM_ROTATE_0, DRM_ROTATE_0);
#else
    drm_plane_create_rotation_property(&gf_plane->base_plane, DRM_MODE_ROTATE_0, DRM_MODE_ROTATE_0);
#endif
#else
    if (GF_PLANE_PS == gf_plane->plane_type)
    {
        if (!dev->mode_config.rotation_property)
        {
            dev->mode_config.rotation_property = drm_mode_create_rotation_property(dev, DRM_ROTATE_0);
        }

        if(dev->mode_config.rotation_property)
        {
            drm_object_attach_property(&gf_plane->base_plane.base, dev->mode_config.rotation_property, DRM_ROTATE_0);
        }
    }

    if(gf_plane->base_plane.state)
    {
        gf_plane->base_plane.state->rotation = DRM_ROTATE_0;
    }
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 18, 0)
    drm_plane_create_alpha_property(&gf_plane->base_plane);
#else
    disp_create_alpha_property(&gf_plane->base_plane);
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
     drm_plane_create_blend_mode_property(&gf_plane->base_plane,
                                            BIT(DRM_MODE_BLEND_PIXEL_NONE) |
                                            BIT(DRM_MODE_BLEND_PREMULTI) |
                                            BIT(DRM_MODE_BLEND_COVERAGE));
#else
    disp_create_blend_mode_property(&gf_plane->base_plane,
                                    BIT(DRM_MODE_BLEND_PIXEL_NONE) |
                                    BIT(DRM_MODE_BLEND_PREMULTI) |
                                    BIT(DRM_MODE_BLEND_COVERAGE));
#endif

    disp_create_alpha_source_property(&gf_plane->base_plane);

    zpos = (gf_plane->is_cursor)? 32 : gf_plane->plane_type;
    drm_plane_create_zpos_immutable_property(&gf_plane->base_plane, zpos); //we do not support dynamic plane order
}


static gf_plane_t*  disp_gene_plane_create(disp_info_t* disp_info,  int  index, GF_PLANE_TYPE  type, int is_cursor)
{
    gf_card_t*  gf_card = disp_info->gf_card;
    struct  drm_device*  drm = gf_card->drm_dev;
    gf_plane_t*  gf_plane = NULL;
    gf_plane_state_t*  gf_pstate = NULL;
    int  ret = 0;
    const int*  formats = NULL;
    const uint64_t *modifiers = NULL;
    int  fmt_count = 0;
    int  drm_ptype;
    char* name;

    gf_plane = gf_calloc(sizeof(gf_plane_t));
    if (!gf_plane)
    {
        goto fail;
    }

    gf_plane->crtc_index = index;
    if(is_cursor)
    {
        gf_plane->plane_type = GF_MAX_PLANE;
        gf_plane->is_cursor = 1;
        gf_plane->can_window = 1;
    }
    else
    {
        gf_plane->plane_type = type;
        gf_plane->can_window = (type != GF_PLANE_PS)? 1 : 0;
        gf_plane->can_up_scale = (disp_info->up_scale_plane_mask[index] & (1 << type))? 1 : 0;
        gf_plane->can_down_scale = (disp_info->down_scale_plane_mask[index] & (1 << type))? 1 : 0;
    }

    gf_pstate = gf_calloc(sizeof(gf_plane_state_t));
    if (!gf_pstate)
    {
        goto fail;
    }

    gf_pstate->base_pstate.plane = &gf_plane->base_plane;
    gf_plane->base_plane.state = &gf_pstate->base_pstate;

    if(is_cursor)
    {
        formats = chx_cursor_formats;
        fmt_count = sizeof(chx_cursor_formats)/sizeof(chx_cursor_formats[0]);
    #if  DRM_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
        modifiers = chx_cursor_modifiers;
    #endif
        name = cursor_name;
        drm_ptype = DRM_PLANE_TYPE_CURSOR;
    }
    else
    {
        formats = chx_plane_formats;
        fmt_count = sizeof(chx_plane_formats)/sizeof(chx_plane_formats[0]);
    #if  DRM_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
        modifiers = chx_plane_modifiers;
    #endif
        name = plane_name[type];
        drm_ptype = (type == GF_PLANE_PS)? DRM_PLANE_TYPE_PRIMARY : DRM_PLANE_TYPE_OVERLAY;
    }

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
    ret = drm_universal_plane_init(drm, &gf_plane->base_plane,
                                    (1 << index), &gf_plane_funcs,
                                    formats, fmt_count,
                                    drm_ptype,
                                    "IGA%d-%s", (index+1), name);
#else
    ret = drm_universal_plane_init(drm, &gf_plane->base_plane,
                                    (1 << index), &gf_plane_funcs,
                                    formats, fmt_count, modifiers,
                                    drm_ptype,
                                    "IGA%d-%s", (index+1), name);
#endif

    if(ret)
    {
        goto fail;
    }

    drm_plane_helper_add(&gf_plane->base_plane, &gf_plane_helper_funcs);

    disp_create_plane_property(drm, gf_plane);

    DRM_DEBUG_KMS("plane=%d,name=%s\n", gf_plane->base_plane.index, gf_plane->base_plane.name);
    return  gf_plane;

fail:
    if(gf_plane)
    {
        gf_free(gf_plane);
        gf_plane = NULL;
    }
    if(gf_pstate)
    {
        gf_free(gf_pstate);
        gf_pstate = NULL;
    }
    return NULL;
}

#else

static gf_plane_t*  disp_gene_plane_create(disp_info_t* disp_info,  int  index, GF_PLANE_TYPE  type, int is_cursor)
{
    gf_card_t*  gf_card = disp_info->gf_card;
    struct  drm_device*  drm = gf_card->drm_dev;
    gf_plane_t*  gf_plane = NULL;
    int  ret = 0;
    const int*  formats = 0;
    int  fmt_count = 0;

    gf_plane = gf_calloc(sizeof(gf_plane_t));
    if (!gf_plane)
    {
        gf_error("Alloc plane failed. plane type = %d, crtc index = %d.\n", type, index);
        return  NULL;
    }

    gf_plane->crtc_index = index;
    gf_plane->plane_type = type;
    gf_plane->can_window = (type != GF_PLANE_PS)? 1 : 0;
    gf_plane->can_up_scale = (disp_info->up_scale_plane_mask[index] & (1 << type))? 1 : 0;
    gf_plane->can_down_scale = (disp_info->down_scale_plane_mask[index] & (1 << type))? 1 : 0;

    formats = chx_plane_formats;
    fmt_count = sizeof(chx_plane_formats)/sizeof(chx_plane_formats[0]);

    ret = drm_plane_init(drm, &gf_plane->base_plane, (1 << index), &gf_plane_funcs, formats, fmt_count, FALSE);

    if(ret)
    {
        gf_error("Init plane failed. plane type = %d, crtc index = %d.\n", type, index);
        gf_free(gf_plane);
        gf_plane = NULL;
    }
    return  gf_plane;
}
#endif

void  disp_irq_init(disp_info_t* disp_info)
{
    gf_card_t*  gf_card = disp_info->gf_card;
    struct drm_device*  drm = gf_card->drm_dev;

    INIT_WORK(&disp_info->hotplug_work, gf_hotplug_work_func);
    INIT_WORK(&disp_info->dp_irq_work, gf_dp_irq_work_func);
    INIT_WORK(&disp_info->hda_work, gf_hda_work_func);
    INIT_WORK(&disp_info->hdcp_work, gf_hdcp_work_func);
    disp_info->irq_chip_func = &irq_chip_funcs;

    drm->vblank_disable_immediate = true;

    drm->max_vblank_count = 0xFFFF;

#if DRM_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
    drm->driver->get_vblank_counter = gf_get_vblank_counter;
    drm->driver->enable_vblank   = gf_enable_vblank;
    drm->driver->disable_vblank  = gf_disable_vblank;
    drm->driver->get_vblank_timestamp = gf_get_vblank_timestamp;
#endif

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 13, 0)
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 4, 0)
    drm->driver->get_scanout_position = gf_legacy_get_crtc_scanoutpos;
#else
    drm->driver->get_scanout_position = gf_get_crtc_scanoutpos;
#endif
#elif DRM_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
    drm->driver->get_scanout_position = gf_get_crtc_scanoutpos_kernel_4_10;
#endif

    if(gf_card->support_msi && !gf_card->pdev->msi_enabled)
    {
        pci_enable_msi(gf_card->pdev);
    }
}

void  disp_irq_deinit(disp_info_t* disp_info)
{
    gf_card_t*  gf_card = disp_info->gf_card;

    cancel_work_sync(&disp_info->dp_irq_work);
    cancel_work_sync(&disp_info->hotplug_work);
    cancel_work_sync(&disp_info->hda_work);
    cancel_work_sync(&disp_info->hdcp_work);

    if(gf_card->pdev->msi_enabled)
    {
        pci_disable_msi(gf_card->pdev);
    }
}

void disp_irq_install(disp_info_t* disp_info)
{
    gf_card_t*  gf_card = disp_info->gf_card;
    struct drm_device*  drm = gf_card->drm_dev;

#if DRM_VERSION_CODE < KERNEL_VERSION(5, 14, 0)
    drm_irq_install(drm, drm->pdev->irq);
#else
    gf_irq_install(drm);
#endif

    disp_info->irq_enabled = 1;
}

void disp_irq_uninstall(disp_info_t* disp_info)
{
    gf_card_t*  gf_card = disp_info->gf_card;
    struct drm_device*  drm = gf_card->drm_dev;

    disp_info->irq_enabled = 0;

#if DRM_VERSION_CODE < KERNEL_VERSION(5, 14, 0)
    drm_irq_uninstall(drm);
#else
    gf_irq_uninstall(drm);
#endif
}

#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
static int  disp_crtc_init(disp_info_t* disp_info, unsigned int index)
{
    gf_card_t*  gf_card = disp_info->gf_card;
    struct drm_device*  drm = gf_card->drm_dev;
    gf_crtc_t*  gf_crtc = NULL;
    gf_crtc_state_t*  crtc_state = NULL;
    gf_plane_t*   gf_plane[GF_MAX_PLANE] = {NULL};
    gf_plane_t*   gf_cursor = NULL;
    int       ret = 0, type = 0;

    gf_crtc = gf_calloc(sizeof(gf_crtc_t));
    if (!gf_crtc)
    {
        return -ENOMEM;
    }

    crtc_state = gf_calloc(sizeof(gf_crtc_state_t));
    if (!crtc_state)
    {
        ret = -ENOMEM;
        goto fail;
    }

    gf_crtc->pipe = index;
    gf_crtc->base_crtc.state = &crtc_state->base_cstate;
    crtc_state->base_cstate.crtc = &gf_crtc->base_crtc;
    gf_crtc->crtc_dpms = 0;

    if (index >= VSYNC_INT_TABLE_LEN)
    {
        gf_error("index excceds vsync int table length\n");
        goto fail;
    }
    gf_crtc->vsync_int = vsync_int_tbl[index];

    gf_crtc->support_scale = disp_info->scale_support;

    gf_crtc->plane_cnt = disp_info->num_plane[index];

    for(type = 0; type < gf_crtc->plane_cnt; type++)
    {
        gf_plane[type] = disp_gene_plane_create(disp_info, index, type, 0);
    }

    gf_cursor = disp_gene_plane_create(disp_info, index, 0, 1);

    ret = drm_crtc_init_with_planes(drm, &gf_crtc->base_crtc,
                                    &gf_plane[GF_PLANE_PS]->base_plane,
                                    &gf_cursor->base_plane,
                                    &gf_crtc_funcs,
                                    "IGA%d", (index + 1));

    if(ret)
    {
        goto  fail;
    }

    drm_crtc_helper_add(&gf_crtc->base_crtc, &gf_helper_funcs);

    drm_mode_crtc_set_gamma_size(&gf_crtc->base_crtc, 256);

    drm_crtc_enable_color_mgmt(&gf_crtc->base_crtc, 0, 1, 256);

    DRM_DEBUG_KMS("crtc=%d,name=%s\n", gf_crtc->base_crtc.index, gf_crtc->base_crtc.name);
    return  0;

    fail:
        if(gf_crtc)
        {
            gf_free(gf_crtc);
            gf_crtc = NULL;
        }
        if(crtc_state)
        {
            gf_free(crtc_state);
            crtc_state = NULL;
        }

        return  ret;
}

#else

static int  disp_crtc_init(disp_info_t* disp_info, unsigned int index)
{
    gf_card_t*  gf_card = disp_info->gf_card;
    struct drm_device*  drm = gf_card->drm_dev;
    gf_crtc_t*  gf_crtc = NULL;
    gf_plane_t*   gf_plane[GF_MAX_PLANE] = {NULL};
    int       ret = 0, type = 0, i = 0;

    gf_crtc = gf_calloc(sizeof(gf_crtc_t));
    if (!gf_crtc)
    {
        return -ENOMEM;
    }

    gf_crtc->pipe = index;

    gf_crtc->support_scale = disp_info->scale_support;

    gf_crtc->plane_cnt = disp_info->num_plane[index];

    gf_crtc->crtc_dpms = 0;

    if (index >= VSYNC_INT_TABLE_LEN)
    {
        gf_error("index excceds vsync int table length\n");
        goto fail;
    }
    gf_crtc->vsync_int = vsync_int_tbl[index];

    ret = drm_crtc_init(drm, &gf_crtc->base_crtc, &gf_crtc_funcs);

    if(ret)
    {
        goto  fail;
    }

    drm_mode_crtc_set_gamma_size(&gf_crtc->base_crtc, 256);
    for (i = 0; i < 256; i ++)
    {
        gf_crtc->lut_entry[i] = (i << 2) + (i << 12) + (i << 22);
    }

    drm_crtc_helper_add(&gf_crtc->base_crtc, &gf_helper_funcs);

    //in legacy kms, plane is stand for overlay exclude primary stream and cursor
    for(type = GF_PLANE_SS; type < gf_crtc->plane_cnt; type++)
    {
        gf_plane[type] = disp_gene_plane_create(disp_info, index, type, 0);
    }

    return  0;

fail:
    for(type = 0; type < gf_crtc->plane_cnt; type++)
    {
        if(gf_plane[type])
        {
            gf_free(gf_plane[type]);
            gf_plane[type] = NULL;
        }
    }

    if(gf_crtc)
    {
        gf_free(gf_crtc);
        gf_crtc = NULL;
    }

    return  ret;
}

#endif

static int disp_output_init(disp_info_t* disp_info)
{
    unsigned int  support_output = disp_info->support_output;
    struct drm_connector* connector = NULL;
    struct drm_encoder* encoder = NULL;
    int  output;
    int  ret = 0;

    while (support_output)
    {
        output = GET_LAST_BIT(support_output);

        connector = disp_connector_init(disp_info, output);
        encoder = disp_encoder_init(disp_info, output);
        if (connector && encoder)
        {
        #if DRM_VERSION_CODE >= KERNEL_VERSION(4,19,0)
            drm_connector_attach_encoder(connector, encoder);
        #else
            drm_mode_connector_attach_encoder(connector, encoder);
        #endif
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
            drm_connector_register(connector);
#endif
        }
        else
        {
            ret = -ENOMEM;
        }

        support_output &= (~output);
    }

#ifdef ENABLE_HDMI4_VGA_ON_IGA4
    disp_info->conflict_high = DISP_OUTPUT_DP4 & disp_info->support_output;
    disp_info->conflict_low = DISP_OUTPUT_CRT & disp_info->support_output;
#endif

    return ret;
}

static void  disp_hotplug_init(disp_info_t* disp_info)
{
    gf_card_t*  gf_card = disp_info->gf_card;
    struct drm_device*  drm = gf_card->drm_dev;

    //at boot/resume stage, no hpd event for all output, we need poll the hpd outputs once
    drm_helper_hpd_irq_event(drm);

    //enable hot plug interrupt
    gf_hot_plug_intr_onoff(disp_info, 1);
}

static void  disp_polling_init(disp_info_t* disp_info)
{
    gf_card_t*  gf_card = disp_info->gf_card;
    struct drm_device*  drm = gf_card->drm_dev;

    INIT_DELAYED_WORK(&drm->mode_config.output_poll_work, gf_output_poll_work_func);
    drm->mode_config.poll_enabled = 1;

    gf_poll_enable(disp_info);
}

int disp_get_pipe_from_crtc(gf_file_t *priv, gf_kms_get_pipe_from_crtc_t *get)
{
    gf_card_t *gf = priv->card;
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
    struct drm_crtc *drmmode_crtc = drm_crtc_find(gf->drm_dev, get->crtc_id);
#else
    struct drm_crtc *drmmode_crtc = drm_crtc_find(gf->drm_dev, (struct drm_file*)priv->parent_file, get->crtc_id);
#endif

    if (!drmmode_crtc)
        return -ENOENT;

    get->pipe = to_gf_crtc(drmmode_crtc)->pipe;

    return 0;
}

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)

void  gf_disp_suspend_helper(struct drm_device *dev)
{
    struct drm_crtc *crtc = NULL;
    struct drm_encoder *encoder = NULL;
    struct drm_connector* connector = NULL;
    const struct drm_connector_funcs *conn_funcs;
    const struct drm_encoder_helper_funcs *encoder_funcs;
    const struct drm_crtc_helper_funcs *crtc_funcs;
    bool  enc_in_use, has_valid_encoder;

    list_for_each_entry(crtc, &dev->mode_config.crtc_list, head)
    {
        has_valid_encoder = false;

        list_for_each_entry(encoder, &dev->mode_config.encoder_list, head)
        {
            enc_in_use = false;

            if(encoder->crtc != crtc)
            {
                continue;
            }

            list_for_each_entry(connector, &dev->mode_config.connector_list, head)
            {
                if(connector->encoder != encoder)
                {
                    continue;
                }

                enc_in_use = true;
                has_valid_encoder = true;

                conn_funcs = connector->funcs;
                if(conn_funcs->dpms)
                {
                    (*conn_funcs->dpms)(connector, DRM_MODE_DPMS_OFF);
                }
            }

            if(enc_in_use)
            {
                encoder_funcs = encoder->helper_private;
                if(encoder_funcs->disable)
                {
                    (*encoder_funcs->disable)(encoder);
                }
                else if(encoder_funcs->dpms)
                {
                    (*encoder_funcs->dpms)(encoder, DRM_MODE_DPMS_OFF);
                }
            }
        }

        if(has_valid_encoder)
        {
            crtc_funcs = crtc->helper_private;
            if(crtc_funcs->disable)
            {
                (*crtc_funcs->disable)(crtc);
            }
            else if(crtc_funcs->dpms)
            {
                (*crtc_funcs->dpms)(crtc, DRM_MODE_DPMS_OFF);
            }
        }
    }
}

#endif

int disp_suspend(struct drm_device *dev)
{
    gf_card_t *gf = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf->disp_info;
    int ret = 0;
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    struct drm_atomic_state *state;
#endif

    gf_hot_plug_intr_onoff(disp_info, 0);

    gf_poll_disable(disp_info);

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    state = drm_atomic_helper_suspend(dev);
    ret = PTR_ERR_OR_ZERO(state);

    if (ret)
    {
        DRM_ERROR("Suspending crtc's failed with %i\n", ret);
    }
    else
    {
        disp_info->modeset_restore_state = state;
    }
#else
    gf_disp_suspend_helper(dev);
#endif

    cancel_work_sync(&disp_info->dp_irq_work);
    cancel_work_sync(&disp_info->hotplug_work);
    cancel_work_sync(&disp_info->hda_work);
    cancel_work_sync(&disp_info->hdcp_work);

    return ret;
}

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)

void disp_vblank_save(struct drm_device* dev)
{
    struct drm_crtc* crtc;
    list_for_each_entry(crtc, &dev->mode_config.crtc_list, head)
    {
        drm_crtc_vblank_off(crtc);
    }
}

void disp_vblank_restore(struct drm_device* dev)
{
    struct drm_crtc* crtc;
    list_for_each_entry(crtc, &dev->mode_config.crtc_list, head)
    {
        drm_crtc_vblank_on(crtc);
    }
}

#endif

void disp_pre_resume(struct drm_device *dev)
{
    gf_card_t  *gf = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf->disp_info;

    disp_cbios_init_hw(disp_info);
}

void disp_post_resume(struct drm_device *dev)
{
    gf_card_t  *gf = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf->disp_info;
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    struct drm_atomic_state *state = disp_info->modeset_restore_state;
    struct drm_crtc_state *crtc_state;
    struct drm_plane_state *plane_state;
    struct drm_modeset_acquire_ctx ctx;
#endif
    struct drm_crtc *crtc;
    struct drm_plane *plane;
    gf_plane_t *gf_plane;
    int i, ret;

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    disp_info->modeset_restore_state = NULL;
    if (state)
    {
        state->acquire_ctx = &ctx;
    }
#endif

    drm_mode_config_reset(dev);

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    mutex_lock(&dev->mode_config.mutex);
    drm_modeset_acquire_init(&ctx, 0);

    while (1)
    {
        ret = drm_modeset_lock_all_ctx(dev, &ctx);
        if (ret != -EDEADLK)
            break;
        drm_modeset_backoff(&ctx);
    }

    if (!ret && state)
    {
        if (gf->flags & GF_S4_RESUME)
        {
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
            for_each_plane_in_state(state, plane, plane_state, i)
#else
            for_each_new_plane_in_state(state, plane, plane_state, i)
#endif
            {
                if (plane_state->crtc != NULL && plane_state->fb != NULL)
                {
                    gf_plane = to_gf_plane(plane);
                    gf_plane->resume_from_s4 = 1;
                }
            }

            gf->flags &= ~GF_S4_RESUME;
        }

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
        ret = drm_atomic_helper_commit_duplicated_state(state, &ctx);
#else
        ret = drm_atomic_commit(state);
#endif
    }
    drm_modeset_drop_locks(&ctx);
    drm_modeset_acquire_fini(&ctx);
    mutex_unlock(&dev->mode_config.mutex);

    if (ret){
        DRM_ERROR("Restoring old state failed with %i\n", ret);
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
        drm_atomic_state_free(state);
#endif
    }
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
    if (state)
        drm_atomic_state_put(state);
#endif
#endif

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
    drm_helper_resume_force_mode(dev);
#endif

    gf_poll_enable(disp_info);

    disp_probe_connector_after_resume(dev);

    gf_hot_plug_intr_onoff(disp_info, 1);
}

static int disp_mode_config_init(disp_info_t* disp_info)
{
    gf_card_t*  gf_card = disp_info->gf_card;
    adapter_info_t* adapter_info = &gf_card->adapter_info;
    struct drm_device*  drm = gf_card->drm_dev;

    drm_mode_config_init(drm);

    drm->mode_config.min_width = 0;
    drm->mode_config.min_height = 0;

    drm->mode_config.max_width = 3840*4;   // 4*4k
    drm->mode_config.max_height = 2160*4;
    drm->mode_config.cursor_width = 128;
    drm->mode_config.cursor_height = 128;

    drm->mode_config.preferred_depth = 24;
    drm->mode_config.prefer_shadow = 1;

#if  DRM_VERSION_CODE < KERNEL_VERSION(5, 14, 0)
    drm->mode_config.allow_fb_modifiers = TRUE;
#endif

#if  DRM_VERSION_CODE < KERNEL_VERSION(6, 2, 0)
    drm->mode_config.fb_base = adapter_info->fb_bus_addr;
#endif

    drm->mode_config.funcs = &gf_kms_mode_funcs;

    drm->mode_config.async_page_flip = TRUE;

#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    drm->mode_config.helper_private = &gf_kms_mode_helper_funcs;
#endif

    return 0;
}

static void  disp_turn_off_crtc_output(disp_info_t* disp_info)
{
    unsigned int devices[MAX_CORE_CRTCS] = {0};
    unsigned int index, detect_devices = 0;

    gf_info("To turn off igas and devices.\n");

    disp_cbios_sync_vbios(disp_info);
    disp_cbios_get_active_devices(disp_info, devices);
    for(index = 0; index < disp_info->num_crtc; index++)
    {
        detect_devices |= devices[index];
    }
    disp_cbios_detect_connected_output(disp_info, detect_devices, 0);
    disp_cbios_set_dpms(disp_info, detect_devices, GF_DPMS_OFF);

    for(index = 0; index < disp_info->num_crtc; index++)
    {
        disp_cbios_turn_onoff_screen(disp_info, index, 0);
        disp_cbios_turn_onoff_iga(disp_info, index, 0);
    }
}

static void disp_info_print(disp_info_t* disp_info)
{
    adapter_info_t* adapter_info = disp_info->adp_info;
    unsigned char* pmpversion = disp_info->pmp_version;
    unsigned int value = 0;
    int  vbiosVer = 0;
    int  pmpdatelen = 0;
    int  pmptimelen = 0;

    vbiosVer = disp_info->vbios_version;
    if(vbiosVer && (vbiosVer != 0xffffffff))
    {
        gf_info("displayinfo Vbios Version:%02x.%02x.%02x.%02x\n", (vbiosVer>>24)&0xff,(vbiosVer>>16)&0xff,(vbiosVer>>8)&0xff,vbiosVer&0xff);
    }

    if(*pmpversion)
    {
        // pmpVersion:str1 -> pmp version,str2 -> pmp build date,str3 -> pmp build time
        pmpdatelen = gf_strlen(pmpversion) + 1;
        pmptimelen = gf_strlen(pmpversion + pmpdatelen) + 1;
        gf_info("displayinfo PMP Version:%s Build Time:%s %s\n",pmpversion,(pmpversion + pmpdatelen),(pmpversion  + pmpdatelen + pmptimelen) );
    }

    gf_info("displayinfo Driver Version:%02x.%02x.%02x%s\n",DRIVER_MAJOR,DRIVER_MINOR,DRIVER_PATCHLEVEL,DRIVER_CLASS);
    gf_info("displayinfo Driver Release Date:%s\n",DRIVER_DATE);
    gf_info("displayinfo FB Size:%d M\n",adapter_info->total_mem_size_mb);
    gf_info("displayinfo Chip Slice Mask:0x%x\n",adapter_info->chip_slice_mask);
    gf_info("displayinfo MIU channel num:%d\n",adapter_info->chan_num);

     //becuse the Unit from cbios is KHZ,so divide 1000 to MHZ
    if(DISP_OK == disp_cbios_get_clock(disp_info, GF_QUERY_CORE_CLOCK, &value))
    {
        gf_info("displayinfo Coreclk:%dMHz\n",value / 1000);
    }
    if (DISP_OK == disp_cbios_get_clock(disp_info, GF_QUERY_ENGINE_CLOCK, &value))
    {
        //gf_info("displayinfo Eclk:%dMHz\n", value / 1000);
    }
    if(DISP_OK == disp_cbios_get_clock(disp_info, GF_QUERY_VCLK, &value))
    {
        //gf_info("displayinfo Vclk:%dMHz\n", (value + 500)/1000);
    }
    if(DISP_OK == disp_cbios_get_clock(disp_info, GF_QUERY_MCLK, &value))
    {
        value = gf_calc_double_standard_mclk(value);
        gf_info("displayinfo Mclk:%dMHz\n", value / 2);
    }
}

static int disp_modeset_create_properties(disp_info_t *disp_info)
{
    gf_card_t *gf_card = disp_info->gf_card;
    struct drm_device *drm = gf_card->drm_dev;

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
    disp_info->splice_active_prop =
        drm_property_create_range(drm, 0, "splice active", 0, 1);
    if (!disp_info->splice_active_prop)
    {
        return -ENOMEM;
    }

    disp_info->mode_x_prop =
        drm_property_create_range(drm, 0, "splice mode x", 0, 16384);
    if (!disp_info->mode_x_prop)
    {
        return -ENOMEM;
    }

    disp_info->mode_y_prop =
        drm_property_create_range(drm, 0, "splice mode y", 0, 8640);
    if (!disp_info->mode_y_prop)
    {
        return -ENOMEM;
    }

    disp_info->mode_rate_prop =
        drm_property_create_range(drm, 0, "splice mode rate", 0, 12000);
    if (!disp_info->mode_rate_prop)
    {
        return -ENOMEM;
    }

    disp_info->crtc_x_prop =
        drm_property_create_range(drm, 0, "splice crtc x", 0, 16384);
    if (!disp_info->crtc_x_prop)
    {
        return -ENOMEM;
    }

    disp_info->crtc_y_prop =
        drm_property_create_range(drm, 0, "splice crtc y", 0, 8640);
    if (!disp_info->crtc_y_prop)
    {
        return -ENOMEM;
    }

    disp_info->crtc_w_prop =
        drm_property_create_range(drm, 0, "splice crtc w", 0, 8192);
    if (!disp_info->crtc_w_prop)
    {
        return -ENOMEM;
    }

    disp_info->crtc_h_prop =
        drm_property_create_range(drm, 0, "splice crtc h", 0, 4320);
    if (!disp_info->crtc_h_prop)
    {
        return -ENOMEM;
    }

    disp_info->splice_trigger_prop =
        drm_property_create_range(drm, 0, "splice trigger", 0, 0xFFFF);
    if (!disp_info->splice_trigger_prop)
    {
        return -ENOMEM;
    }

#endif

    return 0;
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
void gf_disp_state_timer_fn(struct timer_list *t)
#else
void gf_disp_state_timer_fn(unsigned long data)
#endif
{
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
    disp_state_info_t *pstate_info = from_timer(pstate_info, t, state_timer);
#else
    disp_state_info_t *pstate_info  = (disp_state_info_t  *)data;
#endif
    disp_info_t *disp_info = (disp_info_t *)pstate_info->disp_info;
    gf_card_t *gf_card  = (gf_card_t *)disp_info->gf_card;
    struct  drm_device *drm_dev = gf_card->drm_dev;
    struct drm_crtc *crtc = NULL;
    bool all_crtcs_off = true;

    list_for_each_entry(crtc, &(drm_dev->mode_config.crtc_list), head)
    {
        if (to_gf_crtc(crtc)->enabled)
        {
            all_crtcs_off = false;
            break;
        }

    }

    if (all_crtcs_off)
    {
        gf_core_interface->disp_state_update(gf_card->adapter, DISP_LONGIDLE_STATE);

    }
    else
    {
        gf_core_interface->disp_state_update(gf_card->adapter, DISP_SHORTIDLE_STATE);
    }

    if (all_crtcs_off)
    {
        atomic_set(&pstate_info->curr_state, DISP_LONGIDLE_STATE);
    }
    else
    {
        atomic_set(&pstate_info->curr_state, DISP_SHORTIDLE_STATE);
    }
}

int disp_init_state_info(disp_info_t *disp_info)
{
    disp_state_info_t *pstate_info = NULL;

    pstate_info = gf_calloc(sizeof(disp_state_info_t));

    if (!pstate_info)
    {
        return -1;
    }

    disp_info->state_info = pstate_info;
    pstate_info->disp_info = disp_info;

    pstate_info->ref_lock = gf_create_spinlock(0);

    atomic_set(&pstate_info->curr_state, DISP_INIT_STATE);

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
    timer_setup(&pstate_info->state_timer, gf_disp_state_timer_fn, 0);
#else
    setup_timer(&pstate_info->state_timer, gf_disp_state_timer_fn, (unsigned long)pstate_info);
#endif

    return 0;
}

void disp_deinit_state_info(disp_info_t* disp_info)
{
    disp_state_info_t* pstate_info = disp_info->state_info;

#if DRM_VERSION_CODE >= KERNEL_VERSION(6, 15, 0)
    timer_delete_sync(&pstate_info->state_timer);
#else
    del_timer_sync(&pstate_info->state_timer);
#endif

    gf_destroy_spinlock(pstate_info->ref_lock);
    pstate_info->ref_lock = NULL;

    gf_free(pstate_info);

    disp_info->state_info = NULL;
}

int  gf_init_modeset(struct drm_device *dev)
{
    gf_card_t*  gf_card = dev->dev_private;
    adapter_info_t* adapter_info = &gf_card->adapter_info;
    krnl_adapter_init_info_t* a_info = &gf_card->a_info;
    disp_info_t*  disp_info = NULL;
    unsigned int  index = 0, ret = -1;

    //drm_debug = 0xffffffff;

    disp_info = gf_calloc(sizeof(disp_info_t));

    if(!disp_info)
    {
        return  ret;
    }

    gf_card->disp_info = disp_info;
    disp_info->gf_card = gf_card;
    disp_info->adp_info = adapter_info;
    adapter_info->init_render = 1;

    gf_core_interface->get_adapter_info(gf_card->adapter, adapter_info);

    disp_info_pre_init(disp_info);

    disp_init_cbios(disp_info);

    disp_cbios_init_hw(disp_info);

    disp_cbios_query_vbeinfo(disp_info);

    disp_cbios_get_slice_num(disp_info);

    gf_core_interface->update_adapter_info(gf_card->adapter, adapter_info, a_info);

    disp_cbios_get_crtc_resource(disp_info);

    disp_cbios_get_crtc_caps(disp_info);

    disp_turn_off_crtc_output(disp_info);

    disp_irq_init(disp_info);

    if (disp_info->num_crtc)
    {

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
        //NOTE: support splice from 4.19.0
        ret = drm_vblank_init(dev, disp_info->num_crtc + 1);
#else
        ret = drm_vblank_init(dev, disp_info->num_crtc);
#endif
        if (ret)
        {
            goto err_vblk;
        }
    }

    disp_mode_config_init(disp_info);

    ret = disp_modeset_create_properties(disp_info);
    if (ret)
    {
        goto err_props;
    }

    for(index = 0; index < disp_info->num_crtc; index++)
    {
        ret = disp_crtc_init(disp_info, index);
        if (ret)
        {
            goto err_crtc;
        }
    }

    disp_output_init(disp_info);

    gf_splice_manager_init(disp_info, disp_info->num_crtc);

    disp_polling_init(disp_info);

    disp_hotplug_init(disp_info);

    disp_capture_init(disp_info);

    disp_init_state_info(disp_info);

    disp_info_print(disp_info);

    return  ret;

err_props:
err_crtc:

    //drm_vblank_cleanup(dev);

    drm_mode_config_cleanup(dev);

err_vblk:
    disp_capture_deinit(disp_info);

    disp_irq_deinit(disp_info);

    disp_cbios_cleanup(disp_info);

    disp_info_deinit(disp_info);

    gf_free(disp_info);

    gf_card->disp_info = NULL;

    return  ret;
}

void  gf_deinit_modeset(struct drm_device *dev)
{
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t*)gf_card->disp_info;

    if(!disp_info)
    {
        return;
    }

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
    drm_atomic_helper_shutdown(dev);
#endif

    disp_capture_deinit(disp_info);

    disp_irq_deinit(disp_info);

    drm_kms_helper_poll_fini(dev);

    gf_splice_manager_deinit(disp_info);

    drm_mode_config_cleanup(dev);

    disp_cbios_cleanup(disp_info);

    disp_deinit_state_info(disp_info);

    disp_info_deinit(disp_info);

    gf_free(disp_info);

    gf_card->disp_info = NULL;
}

int gf_debugfs_crtc_dump(struct seq_file* file, struct drm_device* dev, int index)
{
    struct drm_crtc* crtc = NULL;
    gf_crtc_t*  gf_crtc = NULL;
    struct drm_display_mode  *mode, *hwmode;
    struct drm_plane*  plane = NULL;
    gf_plane_t* gf_plane= NULL;
    struct drm_gf_gem_object *obj = NULL;
    int enabled, h, v;

    list_for_each_entry(crtc, &dev->mode_config.crtc_list, head)
    {
        if(to_gf_crtc(crtc)->pipe == index)
        {
            gf_crtc = to_gf_crtc(crtc);
            break;
        }
    }

    if(!gf_crtc)
    {
        return 0;
    }

#if  DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
    mutex_lock(&dev->mode_config.mutex);
    drm_modeset_lock(&dev->mode_config.connection_mutex, NULL);
    enabled = drm_helper_crtc_in_use(crtc);
    drm_modeset_unlock(&dev->mode_config.connection_mutex);
    mutex_unlock(&dev->mode_config.mutex);
#else
    enabled = (crtc->state->enable && crtc->state->mode_blob);
#endif

    if(!enabled)
    {
        seq_printf(file, "IGA status: disabled.\n");
        return 0;
    }

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
    mode = &crtc->mode;
    hwmode = &crtc->hwmode;
#else
    mode = &crtc->state->mode;
    hwmode = &crtc->state->adjusted_mode;
#endif

    h = mode->hdisplay;
    v = mode->vdisplay;

    seq_printf(file, "IGA status: enabled.\n");
    seq_printf(file, "SW timing: active: %d x %d. total: %d x %d. clock: %dk\n",
         h, v, mode->htotal, mode->vtotal, mode->clock);
    seq_printf(file, "HW timing: active: %d x %d. total: %d x %d. clock: %dk\n",
        hwmode->hdisplay, hwmode->vdisplay, hwmode->htotal, hwmode->vtotal, hwmode->clock);

#if  DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
    //primary
#if  DRM_VERSION_CODE >= KERNEL_VERSION(3, 15, 0)
    obj = (crtc->primary->fb)? to_gfb(crtc->primary->fb)->obj : NULL;
#else
    obj = (crtc->fb)? to_gfb(crtc->fb)->obj : NULL;
#endif
    seq_printf(file, "IGA%d-PS: src window: [%d, %d, %d, %d], dst window: [0, 0, %d, %d], handle: 0x%x, gpu vt addr: 0x%llx.\n",
                      (gf_crtc->pipe+1), crtc->x, crtc->y, crtc->x+h, crtc->y + v, h, v, obj->info.allocation, obj->info.gpu_virt_addr);
    //overlay
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 3, 0)
    drm_for_each_legacy_plane(plane, dev)
#elif DRM_VERSION_CODE >= KERNEL_VERSION(3, 15, 0)
    drm_for_each_legacy_plane(plane, &dev->mode_config.plane_list)
#else
    list_for_each_entry(plane, &dev->mode_config.plane_list, head)
#endif
    {
        gf_plane = to_gf_plane(plane);
        if(gf_plane->crtc_index != gf_crtc->pipe)
        {
            continue;
        }

        if(!plane->crtc || !plane->fb)
        {
            seq_printf(file, "IGA%d-%s: disabled.\n", (gf_crtc->pipe+1), plane_name[gf_plane->plane_type]);
        }
        else
        {
            int src_x = gf_plane->src_pos & 0xFFFF;
            int src_y = (gf_plane->src_pos >> 16) & 0xFFFF;
            int src_w = gf_plane->src_size & 0xFFFF;
            int src_h = (gf_plane->src_size >> 16) & 0xFFFF;
            int dst_x = gf_plane->dst_pos & 0xFFFF;
            int dst_y = (gf_plane->dst_pos >> 16) & 0xFFFF;
            int dst_w = gf_plane->dst_size & 0xFFFF;
            int dst_h = (gf_plane->dst_size >> 16) & 0xFFFF;
            obj = to_gfb(plane->fb)->obj;
            seq_printf(file, "IGA%d-%s: src window: [%d, %d, %d, %d], dst window: [%d, %d, %d, %d], handle: 0x%x, gpu vt addr: 0x%llx.\n",
                      (gf_crtc->pipe+1), plane_name[gf_plane->plane_type], src_x, src_y, src_x + src_w, src_y + src_h,
                      dst_x, dst_y, dst_x + dst_w, dst_y + dst_h, obj->info.allocation, obj->info.gpu_virt_addr);
        }
    }
    //cursor
    if(!gf_crtc->cursor_bo)
    {
        seq_printf(file, "IGA%d-cursor: disabled.\n", (gf_crtc->pipe+1));
    }
    else
    {
        obj = gf_crtc->cursor_bo;
        seq_printf(file, "IGA%d-cursor: src window: [%d, %d, %d, %d], dst window: [%d, %d, %d, %d], handle: 0x%x, gpu vt addr: 0x%llx.\n",
                    (gf_crtc->pipe+1), 0, 0, gf_crtc->cursor_w, gf_crtc->cursor_h, gf_crtc->cursor_x, gf_crtc->cursor_y,
                    gf_crtc->cursor_x + gf_crtc->cursor_w, gf_crtc->cursor_y + gf_crtc->cursor_h, obj->info.allocation, obj->info.gpu_virt_addr);
    }
#else
    list_for_each_entry(plane, &dev->mode_config.plane_list, head)
    {
        gf_plane = to_gf_plane(plane);
        if(gf_plane->crtc_index != gf_crtc->pipe)
        {
            continue;
        }

        if(!gf_plane->base_plane.state->crtc || ! gf_plane->base_plane.state->fb)
        {
            seq_printf(file, "%s: disabled.\n", gf_plane->base_plane.name);
        }
        else
        {
            int src_x = gf_plane->base_plane.state->src_x >> 16;
            int src_y = gf_plane->base_plane.state->src_y >> 16;
            int src_w = gf_plane->base_plane.state->src_w >> 16;
            int src_h = gf_plane->base_plane.state->src_h >> 16;
            int dst_x = gf_plane->base_plane.state->crtc_x;
            int dst_y = gf_plane->base_plane.state->crtc_y;
            int dst_w = gf_plane->base_plane.state->crtc_w;
            int dst_h = gf_plane->base_plane.state->crtc_h;
            obj = to_gfb(gf_plane->base_plane.state->fb)->obj;
            seq_printf(file, "%s: src window: [%d, %d, %d, %d], dst window: [%d, %d, %d, %d], handle: 0x%x, gpu vt addr: 0x%llx.\n",
                      gf_plane->base_plane.name, src_x, src_y, src_x + src_w, src_y + src_h,
                      dst_x, dst_y, dst_x + dst_w, dst_y + dst_h, obj->info.allocation, obj->info.gpu_virt_addr);
        }
    }
#endif

    return 0;
}

int gf_debugfs_clock_dump(struct seq_file* file, struct drm_device* dev)
{
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t*)gf_card->disp_info;
    unsigned int value = 0;

    if(DISP_OK == disp_cbios_get_clock(disp_info, GF_QUERY_ENGINE_CLOCK, &value))
    {
        seq_printf(file, "Engine clock = %dMHz.\n", value/1000);
    }

    return 0;
}

void gf_chip_fan_init_legacy(void* dispi, int index)
{
    disp_info_t*  disp_info = (disp_info_t*)dispi;
    adapter_info_t*  adp_info = disp_info->adp_info;
    int ctrl_reg_8x000, ctrl_reg_8x00c, ctrl_reg_8x014;
    unsigned int *pRegAddr_8x000,*pRegAddr_8x00c,*pRegAddr_8x014;
    int fanbase;

    if(adp_info->mmio_size < 0x8F024)
    {
        gf_info("Can't access temper sense register range from 0x8F000 to 0x8F024!\n");
        return;
    }

    if(index == 1)
    {
        fanbase = 0x8c000;
    }
    else if(index == 0)
    {
        fanbase = 0x8d000;
    }
    else
    {
        return;
    }

    pRegAddr_8x000   = (unsigned int*)(adp_info->mmio  + fanbase + 0x000);
    pRegAddr_8x00c   = (unsigned int*)(adp_info->mmio  + fanbase + 0x00c);
    pRegAddr_8x014   = (unsigned int*)(adp_info->mmio  + fanbase + 0x014);

    ctrl_reg_8x00c = gf_read32(pRegAddr_8x00c);
    ctrl_reg_8x00c |= 0x3;
    gf_write32(pRegAddr_8x00c, ctrl_reg_8x00c);

    ctrl_reg_8x014 = 0x7D00;
    gf_write32(pRegAddr_8x014, ctrl_reg_8x014);

    ctrl_reg_8x000 = gf_read32(pRegAddr_8x000);
    ctrl_reg_8x000 |= 0x3;
    gf_write32(pRegAddr_8x000, ctrl_reg_8x000);

    return;
}

int gf_get_chip_fanspeed(void* dispi, int index)
{
    disp_info_t*  disp_info = (disp_info_t*)dispi;
    adapter_info_t*  adp_info = disp_info->adp_info;
    unsigned int fan_duty = 0;
    int val;

    val = gf_read32(adp_info->mmio + 0xd001c) - 0x10;
    if (val < 0)
        fan_duty = 0;
    else
        fan_duty = ((val & 0x0f) / 2 * 10 + 84 + (val >> 4) * 8 * 10) / 10;

    return fan_duty;
}

int gf_get_chip_fanspeed_legacy(void* dispi, int index)
{
    static int fanspeed = 0;
    disp_info_t*  disp_info = (disp_info_t*)dispi;
    adapter_info_t*  adp_info = disp_info->adp_info;
    int ctrl_reg_8x000, ctrl_reg_8x008, ctrl_reg_8x014, out_8x01c;
    unsigned int *pRegAddr_8x000, *pRegAddr_8x004, *pRegAddr_8x008, *pRegAddr_8x014, *pRegAddr_8x01c;
    int temp = 0, fanbase = 0;

    if(adp_info->mmio_size < 0x8F024)
    {
        gf_info("Can't access temper sense register range from 0x8F000 to 0x8F024!\n");
        return -1;
    }

    if(index == 1)
    {
        fanspeed = gf_read32(adp_info->mmio + 0xd3dc);
        if (fanspeed)
            return fanspeed;

        fanbase = 0x8c000;
    }
    else if(index == 0)
    {
        fanbase = 0x8d000;
    }
    else
    {
        return -1;
    }

    pRegAddr_8x000   = (unsigned int*)(adp_info->mmio  + fanbase + 0x000);
    pRegAddr_8x004   = (unsigned int*)(adp_info->mmio  + fanbase + 0x004);
    pRegAddr_8x008   = (unsigned int*)(adp_info->mmio  + fanbase + 0x008);
    pRegAddr_8x014   = (unsigned int*)(adp_info->mmio  + fanbase + 0x014);
    pRegAddr_8x01c   = (unsigned int*)(adp_info->mmio  + fanbase + 0x01c);

    if((gf_read32(pRegAddr_8x004) & 0x2) != 0)
    {
        out_8x01c = gf_read32(pRegAddr_8x01c);
        if(out_8x01c != 0)
        {
            temp = 16000 * 60;
            temp = (unsigned int)(temp / out_8x01c);
            fanspeed = temp;
        }
        else
        {
            fanspeed = -1;
        }
    }
    else
    {
        ctrl_reg_8x014 = 0x0;
        gf_write32(pRegAddr_8x014, ctrl_reg_8x014);

        fanspeed = -1;
    }

    ctrl_reg_8x014 = 0x7D00;
    gf_write32(pRegAddr_8x014, ctrl_reg_8x014);

    ctrl_reg_8x008 = gf_read32(pRegAddr_8x008);
    ctrl_reg_8x008 |= 0x3;
    gf_write32(pRegAddr_8x008, ctrl_reg_8x008);

    ctrl_reg_8x000 = gf_read32(pRegAddr_8x000);
    ctrl_reg_8x000 |= 0x3;
    gf_write32(pRegAddr_8x000, ctrl_reg_8x000);
    return fanspeed;
}

int gf_get_chip_temp(void *dispi)
{
    disp_info_t*  disp_info = (disp_info_t*)dispi;
    adapter_info_t*  adp_info = disp_info->adp_info;
    int val, temper, temper_1, temper_2;

    val = gf_read32(adp_info->mmio + 0xD3F4);
    if (!val)
        return val;

    temper_1 = -9 * val * val / 100;
    temper_2 = 2754 * val / 10;

    temper = temper_1 + temper_2 - 44596;
    temper /= 1000;

    return temper;
}

int gf_get_chip_temp_legacy(void *dispi)
{
    disp_info_t*  disp_info = (disp_info_t*)dispi;
    adapter_info_t*  adp_info = disp_info->adp_info;
    int ctrl_reg_8f018, irq_en_8f008, irq_status_8f004, irq_clr_8f00c, out_8f020;
    int temper;

    temper = gf_read32(adp_info->mmio + 0xD3D4);
    if (temper)
        return temper;

    if(adp_info->mmio_size < 0x8F024)
    {
        gf_info("Can't access temper sense register range from 0x8F000 to 0x8F024!\n");
        return 0;
    }

    ctrl_reg_8f018 = gf_read32(adp_info->mmio + 0x8F018);
    ctrl_reg_8f018 &= 0xFFFFF3FF;
    gf_write32(adp_info->mmio + 0x8F018, ctrl_reg_8f018);
    ctrl_reg_8f018 = gf_read32(adp_info->mmio + 0x8F018);
    ctrl_reg_8f018 |= 0x400;
    gf_write32(adp_info->mmio + 0x8F018, ctrl_reg_8f018);

    irq_en_8f008 = gf_read32(adp_info->mmio + 0x8F008);
    irq_en_8f008 |= 0x80;
    gf_write32(adp_info->mmio + 0x8F008, irq_en_8f008);

    irq_status_8f004 = gf_read32(adp_info->mmio + 0x8F004);
    irq_status_8f004 |= 0x80;
    gf_write32(adp_info->mmio + 0x8F004, irq_status_8f004);

    while((gf_read32(adp_info->mmio + 0x8F004) & 0x80) == 0);

    irq_clr_8f00c = gf_read32(adp_info->mmio + 0x8F00C);
    irq_clr_8f00c |= 0x80;
    gf_write32(adp_info->mmio + 0x8F00C, irq_clr_8f00c);

    out_8f020 = gf_read32(adp_info->mmio + 0x8F020);
    out_8f020 &= 0x3FF;

    temper = (27540 - 9 * out_8f020) * out_8f020;
    temper = (int)gf_do_div(temper, 100);
    temper -= 44596;
    temper = (int)gf_do_div(temper, 1000);

    return temper;
}

unsigned int gf_calc_double_standard_mclk(unsigned int mclk)
{
    unsigned int mclk_standard_freq[] = {2133, 2400, 2666, 2933, 3200};
    unsigned int n, min, freq;

    mclk = 2 * ((mclk + 500) / 1000);
    min = 0xffffffff;

    for (n = 0; n < sizeof(mclk_standard_freq) / sizeof(mclk_standard_freq[0]); n++)
    {
        if (mclk > mclk_standard_freq[n])
            freq = mclk - mclk_standard_freq[n];
        else
            freq = mclk_standard_freq[n] - mclk;

        if (min >= freq)
            min = freq;
        else
            break;
    }

    return mclk_standard_freq[n - 1];
}

int gf_debugfs_displayinfo_dump(struct seq_file* file, struct drm_device* dev)
{
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t*)gf_card->disp_info;
    adapter_info_t* adapter_info = &gf_card->adapter_info;
    unsigned char* pmpversion = disp_info->pmp_version;
    unsigned char* fwname = disp_info->firmware_name;
    unsigned int value = 0;
    int  vbiosVer = 0;
    int  pmpdatelen = 0;
    int  pmptimelen = 0;
    int  fwVer = disp_info->firmware_version;

    vbiosVer = disp_info->vbios_version;
    if(vbiosVer && (vbiosVer != 0xffffffff))
    {
        seq_printf(file, "Vbios Version:%02x.%02x.%02x.%02x\n", (vbiosVer>>24)&0xff,(vbiosVer>>16)&0xff,(vbiosVer>>8)&0xff,vbiosVer&0xff);
    }

    if(*pmpversion)
    {
        // pmpVersion:str1 -> pmp version,str2 -> pmp build date,str3 -> pmp build time
        pmpdatelen = gf_strlen(pmpversion) + 1;
        pmptimelen = gf_strlen(pmpversion + pmpdatelen) + 1;
        seq_printf(file, "PMP Version:%s Build Time:%s %s\n",pmpversion,(pmpversion + pmpdatelen),(pmpversion  + pmpdatelen + pmptimelen) );
    }

    if(*fwname)
    {
        seq_printf(file, "Firmware Version:%02x.%02x.%02x.%02x\n", (fwVer>>24)&0xff,(fwVer>>16)&0xff,(fwVer>>8)&0xff,fwVer&0xff);
        seq_printf(file, "Firmware Name:%s\n", fwname);
    }

    seq_printf(file,"Driver Version:%02x.%02x.%02x%s\n",DRIVER_MAJOR,DRIVER_MINOR,DRIVER_PATCHLEVEL,DRIVER_CLASS);
    seq_printf(file,"Driver Release Date:%s\n",DRIVER_DATE);
    seq_printf(file,"FB Size:%d M\n",adapter_info->total_mem_size_mb);
    seq_printf(file,"Chip Slice Mask:0x%x\n",adapter_info->chip_slice_mask);
    seq_printf(file,"MIU channel num:%d\n",adapter_info->chan_num);

     //becuse the Unit from cbios is KHZ,so divide 1000 to MHZ
    if(DISP_OK == disp_cbios_get_clock(disp_info, GF_QUERY_CORE_CLOCK, &value))
    {
        seq_printf(file, "Coreclk:%dMHz\n",value / 1000);
    }
    if(DISP_OK == disp_cbios_get_clock(disp_info, GF_QUERY_ENGINE_CLOCK, &value))
    {
        seq_printf(file, "Eclk:%dMHz\n", value / 1000);
    }
    if(DISP_OK == disp_cbios_get_clock(disp_info, GF_QUERY_VCLK, &value))
    {
        seq_printf(file, "Vclk:%dMHz\n", (value + 500)/1000);
    }
    if(DISP_OK == disp_cbios_get_clock(disp_info, GF_QUERY_MCLK, &value))
    {
        value = gf_calc_double_standard_mclk(value);
        seq_printf(file, "Mclk:%dMHz\n", value / 2);
    }

    seq_printf(file, "Temper: %d degree\n", gf_get_chip_temp(gf_card->disp_info));

    return 0;
}

void gf_acquire_display(disp_info_t *disp_info, unsigned int ref_type)
{
    gf_card_t *gf_card = disp_info->gf_card;
    disp_state_info_t *pstate_info = disp_info->state_info;
    unsigned long flags = 0;
    bool request = false;

    flags = gf_spin_lock_irqsave(pstate_info->ref_lock);

    if (!pstate_info->ref_count)
    {
        request = true;
    }

    pstate_info->ref_count |= (1 << ref_type);

    gf_spin_unlock_irqrestore(pstate_info->ref_lock, flags);

    if (request)
    {
    #if DRM_VERSION_CODE >= KERNEL_VERSION(6, 15, 0)
        timer_delete_sync(&pstate_info->state_timer);
    #else
        del_timer_sync(&pstate_info->state_timer);
    #endif

        if (atomic_read(&pstate_info->curr_state) != DISP_BUSY_STATE)
        {
            gf_core_interface->disp_state_update(gf_card->adapter, DISP_BUSY_STATE);
            atomic_set(&pstate_info->curr_state, DISP_BUSY_STATE);
        }
    }

}

void gf_release_display(disp_info_t *disp_info, unsigned int ref_type)
{
    disp_state_info_t *pstate_info = disp_info->state_info;
    unsigned long flags = 0;
    bool notify = false;

    if (ref_type >= DISP_MAX_REF)
    {
        return;
    }

    flags = gf_spin_lock_irqsave(pstate_info->ref_lock);

    if (pstate_info->ref_count == (1 << ref_type))
    {
        notify = true;
    }

    pstate_info->ref_count &= ~(1 << ref_type);

    gf_spin_unlock_irqrestore(pstate_info->ref_lock, flags);

    if (notify)
    {
        mod_timer(&pstate_info->state_timer, jiffies+HZ);
    }
}

