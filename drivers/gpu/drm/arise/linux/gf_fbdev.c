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
#include "gf_fbdev.h"
#include "gf_debugfs.h"
#include <drm/drm_fb_helper.h>
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
#include <linux/async.h>
#endif

#define __STR(x)    #x
#define STR(x)      __STR(x)

static int gf_drm_fb_set_par(struct fb_info *info)
{
    struct drm_fb_helper *helper = info->par;
    struct drm_device *dev = helper->dev;
    struct gf_fbdev *fbdev = to_gf_fbdev(helper);
    gf_card_t* gf = (gf_card_t*)dev->dev_private;

    gf_info("To set par on drm fb.\n");

    return drm_fb_helper_set_par(info);
}

static struct fb_ops gfb_ops = {
    .owner = THIS_MODULE,
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
    DRM_FB_HELPER_DEFAULT_OPS,
#endif
    .fb_set_par = gf_drm_fb_set_par,
    .fb_fillrect = drm_fb_helper_cfb_fillrect,
    .fb_copyarea = drm_fb_helper_cfb_copyarea,
    .fb_imageblit = drm_fb_helper_cfb_imageblit,
#else
    .fb_check_var = drm_fb_helper_check_var,
    .fb_set_par = drm_fb_helper_set_par,
    .fb_pan_display = drm_fb_helper_pan_display,
    .fb_blank = drm_fb_helper_blank,
    .fb_debug_enter = drm_fb_helper_debug_enter,
    .fb_debug_leave = drm_fb_helper_debug_leave,
    .fb_setcmap = drm_fb_helper_setcmap,

    .fb_fillrect = cfb_fillrect,
    .fb_copyarea = cfb_copyarea,
    .fb_imageblit = cfb_imageblit,
#endif
};

#if DRM_VERSION_CODE < KERNEL_VERSION(5, 2, 0)
static bool gf_fb_initial_config(struct drm_fb_helper *fb_helper,
        struct drm_fb_helper_crtc **crtcs,
        struct drm_display_mode **modes,
        struct drm_fb_offset *offsets,
        bool *enabled, int width, int height)
{
    struct drm_device *dev = fb_helper->dev;
    gf_card_t *gf = dev->dev_private;


    return false;
}
#endif

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
static void gf_crtc_fb_gamma_set(struct drm_crtc *crtc, u16 red, u16 green, u16 blue, int reqno)
{
    DRM_DEBUG_KMS("crtc=%d,red=%d,green=%d,blue=%d\n", to_gf_crtc(crtc)->pipe, red, green, blue);
}

static void gf_crtc_fb_gamma_get(struct drm_crtc *crtc, u16 *red, u16 *green, u16 *blue, int reqno)
{
    DRM_DEBUG_KMS("crtc=%d\n", to_gf_crtc(crtc)->pipe);
}
#endif

static int gfb_create(struct drm_fb_helper *helper, struct drm_fb_helper_surface_size *sizes)
{
    int ret = 0;
    struct fb_info *info;
    struct gf_fbdev *fbdev = to_gf_fbdev(helper);
    struct drm_gf_framebuffer *fb = fbdev->fb;
    struct drm_device *dev = helper->dev;
    gf_card_t *gf = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf->disp_info;
    struct drm_mode_fb_cmd2 mode_cmd = {0, };
    struct drm_gf_gem_object *obj = NULL;
    gf_create_allocation_t create = {0, };

    DRM_DEBUG_KMS("fb_width=%d,fb_height=%d,surface_width=%d,surface_height=%d,surface_bpp=%d,surface_depth=%d\n",
            sizes->fb_width, sizes->fb_height, sizes->surface_width, sizes->surface_height, sizes->surface_bpp, sizes->surface_depth);

    mutex_lock(&dev->struct_mutex);

    if (fb &&
          (sizes->fb_width > fb->base.width ||
           sizes->fb_height > fb->base.height))
    {
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
        drm_framebuffer_unreference(&fb->base);
#else
        drm_framebuffer_put(&fb->base);
#endif
        fbdev->fb = NULL;
        fb = NULL;
    }

    if (!fb)
    {
        create.device   = fbdev->gpu_device;
        create.width    = sizes->surface_width;
        create.height   = sizes->surface_height;
        create.format   = (sizes->surface_bpp == 32) ? GF_FORMAT_B8G8R8A8_UNORM : GF_FORMAT_B5G6R5_UNORM;
        create.tiled    = 0;
        create.unpagable    = TRUE;
        create.usage_mask   = GF_USAGE_FORCE_LOCAL | GF_USAGE_DISPLAY_SURFACE | GF_USAGE_FRAMEBUFFER;
        create.access_hint  = GF_ACCESS_CPU_ALMOST;
        create.primary      = 1;

        obj = gf_drm_gem_create_object(gf, &create, &fbdev->debug);
        gf_assert(obj != NULL, GF_FUNC_NAME(__func__));

        mode_cmd.width = sizes->surface_width;
        mode_cmd.height = sizes->surface_height;
        mode_cmd.pitches[0] = obj->info.pitch;
        mode_cmd.pixel_format = drm_mode_legacy_fb_format(sizes->surface_bpp, sizes->surface_depth);
        fb = __gf_framebuffer_create(dev, &mode_cmd, obj);
        fbdev->fb = fb;
    }
    else
    {
        sizes->fb_width = fb->base.width;
        sizes->fb_height = fb->base.height;
    }

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
    info = framebuffer_alloc(0, &dev->pdev->dev);
    gf_assert(info != NULL, GF_FUNC_NAME(__func__));

    info->par = helper;
    info->flags = FBINFO_DEFAULT | FBINFO_CAN_FORCE_OUTPUT;
    info->fbops = &gfb_ops;

    ret = fb_alloc_cmap(&info->cmap, 256, 0);
    gf_assert(ret == 0, GF_FUNC_NAME(__func__));

    info->apertures = alloc_apertures(1);
    gf_assert(info->apertures != NULL, GF_FUNC_NAME(__func__));

    info->apertures->ranges[0].base = dev->mode_config.fb_base;
    info->apertures->ranges[0].size = obj->info.pitch * obj->info.height;
    helper->fbdev = info;
#else
    info = drm_fb_helper_alloc_fbi(helper);
    info->par = helper;
    info->flags = FBINFO_DEFAULT;
    info->fbops = &gfb_ops;
#endif

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
    info->flags |= FBINFO_CAN_FORCE_OUTPUT;
#endif

    info->skip_vt_switch = true;
    fbdev->helper.fb = &fb->base;
    gf_vsprintf(info->fix.id, "drmfb");

    info->fix.smem_start = obj->info.cpu_phy_addr;
    info->fix.smem_len = obj->info.pitch * obj->info.aligned_height;

//map fbdev fb  gem object
    info->screen_base = gf_gem_object_vmap(obj);
    info->screen_size = obj->info.pitch * obj->info.aligned_height;

    info->skip_vt_switch = true;
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
    drm_fb_helper_fill_fix(info, fb->base.pitches[0], fb->base.depth);
    drm_fb_helper_fill_var(info, &fbdev->helper, sizes->fb_width, sizes->fb_height);
#elif DRM_VERSION_CODE < KERNEL_VERSION(5, 2, 0)
    drm_fb_helper_fill_fix(info, fb->base.pitches[0], fb->base.format->depth);
    drm_fb_helper_fill_var(info, &fbdev->helper, sizes->fb_width, sizes->fb_height);
#else
    drm_fb_helper_fill_info(info, &fbdev->helper, sizes);
#endif

    DRM_DEBUG_KMS("screen_base=0x%p,screen_size=0x%lx,smem_start=0x%lx,smem_len=0x%x,xres=%d,yres=%d\n",
            info->screen_base, info->screen_size, info->fix.smem_start, info->fix.smem_len, info->var.xres, info->var.yres);

    mutex_unlock(&dev->struct_mutex);
    return 0;
}

static const struct drm_fb_helper_funcs gf_fb_helper_funcs = {
#if DRM_VERSION_CODE < KERNEL_VERSION(5, 2, 0)
    .initial_config     = gf_fb_initial_config,
#endif
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
    .gamma_set          = gf_crtc_fb_gamma_set,
    .gamma_get          = gf_crtc_fb_gamma_get,
#endif    
    .fb_probe           = gfb_create,
};

void gf_fbdev_disable_vesa(gf_card_t *gf)
{
    struct fb_info *vesafb = NULL;
    bus_config_t bus_config = {0, };
    int i, vesafb_found = 0, vga16fb_found = 0;

    gf_get_bus_config(gf->pdev, &bus_config);
    for(i = 0; i < num_registered_fb; i++)
    {
        vesafb = registered_fb[i];
        if((vesafb != NULL) && 
            (vesafb->fix.smem_start >= bus_config.mem_start_addr[0]) && 
            (vesafb->fix.smem_start <  bus_config.mem_end_addr[0]))
        {
            vesafb_found = 1;
            break;
        }

        if((vesafb != NULL) &&
            (vesafb->fix.smem_start == 0xA0000))
        {
            vga16fb_found = 1;
            break;
        }
    }

    if (vesafb_found || vga16fb_found)
    {
        gf_info("%s detected, conflicted with current fb, remove %s.\n", vesafb->fix.id, vesafb->fix.id);

        unregister_framebuffer(vesafb);
    }
}

int gf_fbdev_init(gf_card_t *gf)
{
    int ret;
    struct gf_fbdev *fbdev;

    DRM_DEBUG_KMS("gf=%p\n", gf);

    fbdev = gf_calloc(sizeof(*fbdev));
    gf_assert(fbdev != NULL, GF_FUNC_NAME(__func__));

    ret = gf_core_interface->create_device(gf->adapter, NULL, &fbdev->gpu_device);
    gf_assert(ret == 0, GF_FUNC_NAME(__func__));

    fbdev->debug = gf_debugfs_add_device_node(gf->debugfs_dev, gf_get_current_pid(), fbdev->gpu_device);
    drm_fb_helper_prepare(gf->drm_dev, &fbdev->helper, &gf_fb_helper_funcs);

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
    ret = drm_fb_helper_init(gf->drm_dev, &fbdev->helper, ((disp_info_t*)(gf->disp_info))->num_crtc, 4);
#elif DRM_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
    ret = drm_fb_helper_init(gf->drm_dev, &fbdev->helper, 4);
#else
    ret = drm_fb_helper_init(gf->drm_dev, &fbdev->helper);
#endif    
    gf_assert(ret == 0, GF_FUNC_NAME(__func__));

#if DRM_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
    drm_fb_helper_single_add_all_connectors(&fbdev->helper);
#endif

    drm_fb_helper_initial_config(&fbdev->helper, 32);

    gf->fbdev = fbdev;

    return 0;
}


int gf_fbdev_deinit(gf_card_t *gf)
{
    struct gf_fbdev *fbdev = gf->fbdev;
    struct drm_gf_framebuffer *fb = fbdev->fb;
    struct fb_info *info;

    if (!fbdev)
    {
        return 0;
    }

    info = fbdev->helper.fbdev;
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
    unregister_framebuffer(info);
#else
    drm_fb_helper_unregister_fbi(&fbdev->helper);
#endif

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
    iounmap(info->screen_base);
    if (info->cmap.len)
        fb_dealloc_cmap(&info->cmap);
    framebuffer_release(info);
#elif DRM_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
    drm_fb_helper_release_fbi(&fbdev->helper);
#endif    

    if (fb->obj)
    {
        gf_gem_object_vunmap(fb->obj);
        gf_gem_object_put(fb->obj);
        fb->obj = NULL;
        drm_framebuffer_unregister_private(&fb->base);
        drm_framebuffer_cleanup(&fb->base);
    }
    drm_fb_helper_fini(&fbdev->helper);
    gf_free(fbdev);

    return 0;
}

void gf_fbdev_set_suspend(gf_card_t *gf, int state)
{
    struct gf_fbdev *fbdev = gf->fbdev;
    struct fb_info *info;

    if (fbdev)
    {
        console_lock();
        
        info = fbdev->helper.fbdev;

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
        if (info)
            fb_set_suspend(info, state);
#else
        drm_fb_helper_set_suspend(&fbdev->helper, state);
#endif

        console_unlock();
    }
}

void gf_fbdev_poll_changed(struct drm_device *dev)
{
    gf_card_t*  gf = dev->dev_private;
    struct gf_fbdev *fbdev = gf->fbdev;

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
    if (fbdev && fbdev->fb)
#else
    if (fbdev)
#endif
    {
        drm_fb_helper_hotplug_event(&fbdev->helper);
    }
}
