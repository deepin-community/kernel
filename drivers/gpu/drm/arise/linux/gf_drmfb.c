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

#include "gf_drmfb.h"

static void gf_drm_framebuffer_destroy(struct drm_framebuffer *fb)
{
    struct drm_device *dev = fb->dev;
    struct drm_gf_framebuffer *gfb = to_gfb(fb);

    drm_framebuffer_cleanup(fb);
    gf_gem_object_put(gfb->obj);
    gf_free(gfb);
}

static int gf_drm_framebuffer_create_handle(struct drm_framebuffer *fb,
                        struct drm_file *file,
                        unsigned int *handle)
{
    struct drm_gf_framebuffer *gfb = to_gfb(fb);

    return drm_gem_handle_create(file, &gfb->obj->base, handle);
}

static int gf_drm_framebuffer_dirty(struct drm_framebuffer *fb,
                        struct drm_file *file,
                        unsigned int flags,
                        unsigned color,
                        struct drm_clip_rect *clips,
                        unsigned int num_clips)
{
    return 0;
}

static const struct drm_framebuffer_funcs gf_fb_funcs =
{
    .destroy = gf_drm_framebuffer_destroy,
    .create_handle = gf_drm_framebuffer_create_handle,
    .dirty = gf_drm_framebuffer_dirty,
};

struct drm_gf_framebuffer*
__gf_framebuffer_create(struct drm_device *dev,
                        struct drm_mode_fb_cmd2 *mode_cmd,
                        struct drm_gf_gem_object *obj)
{
    int ret;
    struct drm_gf_framebuffer *gfb;
    gfb = gf_calloc(sizeof(*gfb));
    if (!gfb)
        return ERR_PTR(-ENOMEM);

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
    drm_helper_mode_fill_fb_struct(&gfb->base, mode_cmd);
#else
    drm_helper_mode_fill_fb_struct(dev, &gfb->base, mode_cmd);
#endif
    gfb->obj = obj;

    ret = drm_framebuffer_init(dev, &gfb->base, &gf_fb_funcs);
    if (ret)
        goto err_free;

    return gfb;
err_free:
    gf_free(gfb);
    return ERR_PTR(ret);
}

struct drm_framebuffer *
gf_fb_create(struct drm_device *dev,
              struct drm_file *file,
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 5, 0) && !defined (PHYTIUM_2000)
              struct drm_mode_fb_cmd2 *user_mode_cmd
#else
              const struct drm_mode_fb_cmd2 *user_mode_cmd
#endif
              )
{
    struct drm_gf_framebuffer *fb;
    struct drm_gf_gem_object *obj;
    struct drm_mode_fb_cmd2 mode_cmd = *user_mode_cmd;

    obj = gf_drm_gem_object_lookup(dev, file, mode_cmd.handles[0]);
    if (!obj)
        return ERR_PTR(-ENOENT);

    fb = __gf_framebuffer_create(dev, &mode_cmd, obj);
    if (IS_ERR(fb))
        gf_gem_object_put(obj);

    return &fb->base;
}

