/* SPDX-License-Identifier: GPL-2.0 */
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

#ifndef __PHYTIUM_FB_H__
#define __PHYTIUM_FB_H__

struct phytium_framebuffer {
	struct drm_framebuffer base;
	struct phytium_gem_object *phytium_gem_obj[PHYTIUM_FORMAT_MAX_PLANE];
};

#define	to_phytium_framebuffer(fb)	container_of(fb, struct phytium_framebuffer, base)

struct phytium_framebuffer *phytium_fb_alloc(struct drm_device *dev,
						   const struct drm_mode_fb_cmd2 *mode_cmd,
						   struct phytium_gem_object **phytium_gem_obj,
						   unsigned int num_planes);

struct drm_framebuffer *phytium_fb_create(struct drm_device *dev, struct drm_file *file_priv,
						 const struct drm_mode_fb_cmd2 *mode_cmd);
#endif /* __PHYTIUM_FB_H__ */
