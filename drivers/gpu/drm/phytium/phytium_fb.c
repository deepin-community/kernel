// SPDX-License-Identifier: GPL-2.0
/* Phytium X100 display drm driver
 *
 * Copyright (c) 2021 Phytium Limited.
 *
 * Author:
 *	Yang Xun <yangxun@phytium.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <drm/drm_gem.h>
#include <drm/drm_modeset_helper.h>
#include "phytium_display_drv.h"
#include "phytium_reg.h"
#include "phytium_fb.h"
#include "phytium_gem.h"

static int
phytium_fb_create_handle(struct drm_framebuffer *fb, struct drm_file *file_priv,
				   unsigned int *handle)
{
	struct phytium_framebuffer *phytium_fb = to_phytium_framebuffer(fb);

	return drm_gem_handle_create(file_priv, &phytium_fb->phytium_gem_obj[0]->base, handle);
}

static void phytium_fb_destroy(struct drm_framebuffer *fb)
{
	struct phytium_framebuffer *phytium_fb = to_phytium_framebuffer(fb);
	int i, num_planes;
	struct drm_gem_object *obj = NULL;

	num_planes = drm_format_num_planes(fb->format->format);

	for (i = 0; i < num_planes; i++) {
		obj = &phytium_fb->phytium_gem_obj[i]->base;
		if (obj)
			drm_gem_object_unreference_unlocked(obj);
	}

	drm_framebuffer_cleanup(fb);
	kfree(phytium_fb);
}

static struct drm_framebuffer_funcs viv_fb_funcs = {
	.create_handle	= phytium_fb_create_handle,
	.destroy	= phytium_fb_destroy,
};

struct phytium_framebuffer *
phytium_fb_alloc(struct drm_device *dev, const struct drm_mode_fb_cmd2 *mode_cmd,
		       struct phytium_gem_object **phytium_gem_obj, unsigned int num_planes)
{
	struct phytium_framebuffer *phytium_fb;
	int ret = 0, i;

	phytium_fb = kzalloc(sizeof(*phytium_fb), GFP_KERNEL);
	if (IS_ERR(phytium_fb))
		return ERR_PTR(-ENOMEM);

	drm_helper_mode_fill_fb_struct(dev, &phytium_fb->base, mode_cmd);

	ret = drm_framebuffer_init(dev, &phytium_fb->base, &viv_fb_funcs);

	if (ret) {
		DRM_ERROR("Failed to initialize framebuffer: %d\n", ret);
		kfree(phytium_fb);
		return ERR_PTR(ret);
	}

	for (i = 0; i < num_planes; i++)
		phytium_fb->phytium_gem_obj[i] = phytium_gem_obj[i];

	return phytium_fb;
}

struct drm_framebuffer *
phytium_fb_create(struct drm_device *dev, struct drm_file *file_priv,
			 const struct drm_mode_fb_cmd2 *mode_cmd)
{
	int ret = 0, i, num_planes;
	struct drm_gem_object *obj;
	unsigned int hsub, vsub, size;
	struct phytium_gem_object *phytium_gem_obj[PHYTIUM_FORMAT_MAX_PLANE] = {0};
	struct phytium_framebuffer *phytium_fb;

	hsub = drm_format_horz_chroma_subsampling(mode_cmd->pixel_format);
	vsub = drm_format_vert_chroma_subsampling(mode_cmd->pixel_format);
	num_planes = min(drm_format_num_planes(mode_cmd->pixel_format), PHYTIUM_FORMAT_MAX_PLANE);
	for (i = 0; i < num_planes; i++) {
		unsigned int height = mode_cmd->height / (i ? vsub : 1);

		size = height * mode_cmd->pitches[i] + mode_cmd->offsets[i];
		obj = drm_gem_object_lookup(file_priv, mode_cmd->handles[i]);
		if (IS_ERR(obj)) {
			DRM_ERROR("Failed to lookup GEM object\n");
			ret = -ENXIO;
			goto error;
		}

		if (obj->size < size) {
			drm_gem_object_unreference_unlocked(obj);
			ret = -EINVAL;
			goto error;
		}

		phytium_gem_obj[i] = to_phytium_gem_obj(obj);
		switch (mode_cmd->modifier[i]) {
		case DRM_FORMAT_MOD_PHYTIUM_TILE_MODE0_FBCDC:
			switch (mode_cmd->pixel_format) {
			case DRM_FORMAT_ARGB4444:
			case DRM_FORMAT_ABGR4444:
			case DRM_FORMAT_RGBA4444:
			case DRM_FORMAT_BGRA4444:
			case DRM_FORMAT_XRGB4444:
			case DRM_FORMAT_XBGR4444:
			case DRM_FORMAT_RGBX4444:
			case DRM_FORMAT_BGRX4444:
			case DRM_FORMAT_ARGB1555:
			case DRM_FORMAT_ABGR1555:
			case DRM_FORMAT_RGBA5551:
			case DRM_FORMAT_BGRA5551:
			case DRM_FORMAT_XRGB1555:
			case DRM_FORMAT_XBGR1555:
			case DRM_FORMAT_RGBX5551:
			case DRM_FORMAT_BGRX5551:
			case DRM_FORMAT_RGB565:
			case DRM_FORMAT_BGR565:
			case DRM_FORMAT_YUYV:
			case DRM_FORMAT_UYVY:
				phytium_gem_obj[i]->tiling = FRAMEBUFFER_TILE_MODE0;
				break;
			default:
				DRM_ERROR("TILE_MODE0_FBCDC not support DRM_FORMAT %d",
					   mode_cmd->pixel_format);
				ret = -EINVAL;
				goto error;
			}
			break;
		case DRM_FORMAT_MOD_PHYTIUM_TILE_MODE3_FBCDC:
			switch (mode_cmd->pixel_format) {
			case DRM_FORMAT_ARGB2101010:
			case DRM_FORMAT_ABGR2101010:
			case DRM_FORMAT_RGBA1010102:
			case DRM_FORMAT_BGRA1010102:
			case DRM_FORMAT_ARGB8888:
			case DRM_FORMAT_ABGR8888:
			case DRM_FORMAT_RGBA8888:
			case DRM_FORMAT_BGRA8888:
			case DRM_FORMAT_XRGB8888:
			case DRM_FORMAT_XBGR8888:
			case DRM_FORMAT_RGBX8888:
			case DRM_FORMAT_BGRX8888:
				phytium_gem_obj[i]->tiling = FRAMEBUFFER_TILE_MODE3;
				break;
			default:
				DRM_ERROR("TILE_MODE3_FBCDC not support DRM_FORMAT %d",
					   mode_cmd->pixel_format);
				ret = -EINVAL;
				goto error;
			}
			break;
		case DRM_FORMAT_MOD_LINEAR:
			phytium_gem_obj[i]->tiling = FRAMEBUFFER_LINEAR;
			break;
		default:
			DRM_ERROR("unsupported fb modifier 0x%llx\n", mode_cmd->modifier[0]);
			ret = -EINVAL;
			goto error;
		}
	}

	phytium_fb = phytium_fb_alloc(dev, mode_cmd, phytium_gem_obj, i);
	if (IS_ERR(phytium_fb)) {
		DRM_DEBUG_KMS("phytium_fb_alloc failed\n");
		ret = PTR_ERR(phytium_fb);
		goto error;
	}

	return &phytium_fb->base;
error:
	for (i--; i >= 0; i--)
		drm_gem_object_unreference_unlocked(&phytium_gem_obj[i]->base);

	return ERR_PTR(ret);
}
