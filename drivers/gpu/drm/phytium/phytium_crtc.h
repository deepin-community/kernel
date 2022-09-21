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

#ifndef __PHYTIUM_CRTC_H__
#define __PHYTIUM_CRTC_H__

struct phytium_crtc {
	struct drm_crtc base;
	int phys_pipe;
	unsigned int bpc;

	/* scale */
	uint32_t src_width;
	uint32_t src_height;
	uint32_t dst_width;
	uint32_t dst_height;
	uint32_t dst_x;
	uint32_t dst_y;
	bool scale_enable;
	bool reserve[3];
};

struct phytium_crtc_state {
	struct drm_crtc_state base;
};

#define to_phytium_crtc(x) container_of(x, struct phytium_crtc, base)
#define to_phytium_crtc_state(x) container_of(x, struct phytium_crtc_state, base)

int phytium_crtc_init(struct drm_device *dev, int pipe);
#endif /* __PHYTIUM_CRTC_H__ */


