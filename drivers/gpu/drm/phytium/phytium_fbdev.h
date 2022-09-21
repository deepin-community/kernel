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

#ifndef	_PHYTIUM_FBDEV_H
#define _PHYTIUM_FBDEV_H

int phytium_drm_fbdev_init(struct drm_device *dev);
void phytium_drm_fbdev_fini(struct drm_device *dev);

#endif /* _PHYTIUM_FBDEV_H */
