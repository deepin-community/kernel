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

#ifndef __PHYTIUM_DEBUGFS_H__
#define __PHYTIUM_DEBUGFS_H__

int phytium_debugfs_register(struct phytium_display_drm_private *priv);
int phytium_debugfs_connector_add(struct drm_connector *connector);

#endif /* __PHYTIUM_DEBUGFS_H__ */
