// SPDX-License-Identifier: GPL-2.0
/* Phytium display drm driver
 *
 * Copyright (C) 2021-2023, Phytium Technology Co., Ltd.
 */

#include <drm/drm.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fourcc.h>
#include "phytium_display_drv.h"
#include "phytium_gem.h"
#include "phytium_fb.h"


#define	PHYTIUM_MAX_CONNECTOR	1
#define	helper_to_drm_private(x) container_of(x, struct phytium_display_private, fbdev_helper)

static void phytium_fbdev_destroy(struct fb_info *info)
{
	struct drm_fb_helper *helper = info->par;
	struct phytium_display_private *priv = helper_to_drm_private(helper);

	phytium_gem_free_object(&priv->fbdev_phytium_gem->base);
}

static int phytium_fbdev_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	struct drm_fb_helper *helper = info->par;
	struct phytium_display_private *priv = helper_to_drm_private(helper);

	return phytium_gem_mmap_obj(&priv->fbdev_phytium_gem->base, vma);
}

static const struct fb_ops phytium_fbdev_ops = {
	.owner = THIS_MODULE,
	.fb_mmap = phytium_fbdev_mmap,
	.fb_destroy = phytium_fbdev_destroy,
	 DRM_FB_HELPER_DEFAULT_OPS,
	 FB_DEFAULT_IOMEM_OPS,
};

static int
phytium_drm_fbdev_create(struct drm_fb_helper *helper, struct drm_fb_helper_surface_size *sizes)
{
	struct phytium_display_private *priv = helper_to_drm_private(helper);
	struct drm_device *dev = helper->dev;
	unsigned int bytes_per_pixel;
	struct drm_mode_fb_cmd2	mode_cmd = {0};
	struct phytium_framebuffer *phytium_fb =  NULL;
	struct fb_info *fbi =  NULL;
	struct drm_framebuffer *fb = NULL;
	size_t size = 0;
	int ret = 0;
	unsigned long offset;

	bytes_per_pixel = DIV_ROUND_UP(sizes->surface_bpp, 8);
	mode_cmd.width = sizes->surface_width;
	mode_cmd.height = sizes->surface_height;
	mode_cmd.pitches[0] =  ALIGN(sizes->surface_width * bytes_per_pixel, 128);
	mode_cmd.pixel_format = drm_mode_legacy_fb_format(sizes->surface_bpp, sizes->surface_depth);
	size = PAGE_ALIGN(mode_cmd.pitches[0] * mode_cmd.height);

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret < 0) {
		DRM_ERROR("failed to get mutex lock\n");
		return ret;
	}

	priv->fbdev_phytium_gem = phytium_gem_create_object(dev, size);
	if (!priv->fbdev_phytium_gem) {
		DRM_ERROR("failed to create gem object\n");
		return -ENOMEM;
	}
	mutex_unlock(&dev->struct_mutex);

	fbi = drm_fb_helper_alloc_info(helper);
	if (IS_ERR(fbi)) {
		DRM_DEV_ERROR(dev->dev, "Failed to create framebuffer info.");
		ret = PTR_ERR(fbi);
		goto out;
	}

	phytium_fb = phytium_fb_alloc(dev, &mode_cmd, &priv->fbdev_phytium_gem, 1);
	if (IS_ERR(phytium_fb)) {
		DRM_DEV_ERROR(dev->dev, "Failed to alloc DRM framebuffer.\n");
		ret = PTR_ERR(phytium_fb);
		goto out;
	}

	helper->fb = &(phytium_fb->base);
	fbi->par = helper;
	fbi->fbops = &phytium_fbdev_ops;

	fb = helper->fb;
	drm_fb_helper_fill_info(fbi, helper, sizes);

	offset = fbi->var.xoffset * bytes_per_pixel;
	offset += fbi->var.yoffset * fb->pitches[0];
	fbi->screen_base = priv->fbdev_phytium_gem->vaddr + offset;
	fbi->screen_size = priv->fbdev_phytium_gem->base.size;
	fbi->fix.smem_len = priv->fbdev_phytium_gem->base.size;
	DRM_DEBUG_KMS("FB [%dx%d]-%d kvaddr=%pa offset=%ld size=%zu\n", fb->width, fb->height,
		       fb->format->depth, &priv->fbdev_phytium_gem->iova, offset, size);
	fbi->skip_vt_switch = true;

	return 0;
out:
	phytium_gem_free_object(&priv->fbdev_phytium_gem->base);
	return ret;
}

static const struct drm_fb_helper_funcs phytium_drm_fb_helper_funcs = {
	.fb_probe = phytium_drm_fbdev_create,
};

int phytium_drm_fbdev_init(struct drm_device *dev)
{
	struct phytium_display_private *priv = dev->dev_private;
	struct drm_fb_helper *helper;
	int ret;

	if (!dev->mode_config.num_crtc || !dev->mode_config.num_connector)
		return -EINVAL;

	helper = &priv->fbdev_helper;
	drm_fb_helper_prepare(dev, helper, 32, &phytium_drm_fb_helper_funcs);

	ret = drm_fb_helper_init(dev, helper);
	if (ret < 0) {
		DRM_DEV_ERROR(dev->dev, "Failed to initialize drm fb helper -ret %d\n", ret);
		return ret;
	}

	ret = drm_fb_helper_initial_config(helper);
	return 0;
}

void phytium_drm_fbdev_fini(struct drm_device *dev)
{
	struct phytium_display_private *priv = dev->dev_private;
	struct drm_fb_helper *helper;

	helper = &priv->fbdev_helper;
	drm_fb_helper_unregister_info(helper);

	if (helper->fb)
		drm_framebuffer_put(helper->fb);

	drm_fb_helper_fini(helper);
}
