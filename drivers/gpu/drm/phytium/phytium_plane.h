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

#ifndef __PHYTIUM_PLANE_H__
#define __PHYTIUM_PLANE_H__

struct phytium_plane {
	struct drm_plane base;
	int phys_pipe;
	unsigned long iova[PHYTIUM_FORMAT_MAX_PLANE];
	unsigned long size[PHYTIUM_FORMAT_MAX_PLANE];
	unsigned int format;
	unsigned int tiling[PHYTIUM_FORMAT_MAX_PLANE];
	unsigned int swizzle;
	unsigned int uv_swizzle;
	unsigned int rot_angle;

	/* only for cursor */
	bool enable;
	bool reserve[3];
	unsigned int cursor_x;
	unsigned int cursor_y;
	unsigned int cursor_hot_x;
	unsigned int cursor_hot_y;
};

struct phytium_plane_state {
	struct drm_plane_state base;
};

#define	to_phytium_plane(x)		container_of(x, struct phytium_plane, base)
#define	to_phytium_plane_state(x)	container_of(x, struct phytium_plane_state, base)

struct phytium_plane *phytium_primary_plane_create(struct drm_device *dev, int pipe);
struct phytium_plane *phytium_cursor_plane_create(struct drm_device *dev, int pipe);
#endif /* __PHYTIUM_PLANE_H__ */
