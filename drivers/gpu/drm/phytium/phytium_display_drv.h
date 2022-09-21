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

#ifndef __PHYTIUM_DISPLAY_DRV_H__
#define __PHYTIUM_DISPLAY_DRV_H__

#include <drm/drmP.h>
#include <drm/drm_fb_helper.h>

#define DEBUG_LOG 0

#define PHYTIUM_FORMAT_MAX_PLANE	3
#define DP_MAX_DOWNSTREAM_PORTS		0x10

#define DRV_NAME	"dc"
#define DRV_DESC	"phytium dc"
#define DRV_DATE	"20201220"
#define DRV_MAJOR	1
#define DRV_MINOR	1

/* come from GPU */
#define	DRM_FORMAT_MOD_VENDOR_PHYTIUM	0x92

/* dc:mode0 8x8 16bpp  gpu: FBCDC_8X8_V10 */
#define	DRM_FORMAT_MOD_PHYTIUM_TILE_MODE0_FBCDC	fourcc_mod_code(PHYTIUM, 21)
/* dc:mode3 8x4 32bpp  gpu: FBCDC_16X4_v10 */
#define	DRM_FORMAT_MOD_PHYTIUM_TILE_MODE3_FBCDC	fourcc_mod_code(PHYTIUM, 22)

#define	PIPE_MASK_SHIFT			0x0
#define	PIPE_MASK_MASK			0x7
#define	EDP_MASK_SHIFT			0x3
#define	EDP_MASK_MASK			0x7

struct phytium_device_info {
	unsigned char pipe_mask;
	unsigned char num_pipes;
	unsigned char total_pipes;
	unsigned char edp_mask;
};

struct phytium_display_drm_private {
	/* hw */
	void __iomem *regs;
	void __iomem *vram_addr;
	struct phytium_device_info info;
	bool vram_support;
	bool reserve[3];

	/* drm */
	struct drm_device *dev;

	/* fb_dev */
	struct drm_fb_helper fbdev_helper;
	struct phytium_gem_object *fbdev_phytium_gem;

	int save_reg[3];
	struct list_head gem_list_head;

	struct work_struct hotplug_work;
	spinlock_t hotplug_irq_lock;
};

static inline unsigned int
phytium_readl_reg(struct phytium_display_drm_private *priv, unsigned int offset)
{
	unsigned int data;

	data = readl(priv->regs + offset);
#if DEBUG_LOG
	pr_info("Read 32'h%08x 32'h%08x\n", offset, data);
#endif
	return data;
}

static inline void
phytium_writel_reg(struct phytium_display_drm_private *priv, unsigned int data,
			   unsigned int offset)
{

	writel(data, priv->regs + offset);
#if DEBUG_LOG
	pr_info("Write 32'h%08x 32'h%08x\n", offset, data);
#endif
}

static inline void
phytium_writeb_reg(struct phytium_display_drm_private *priv, unsigned char data,
			   unsigned int offset)
{
	writeb(data, priv->regs + offset);
#if DEBUG_LOG
	pr_info("Write 32'h%08x 8'h%08x\n", offset, data);
#endif
}

#define for_each_pipe(__dev_priv, __p) \
	for ((__p) = 0; (__p) < __dev_priv->info.total_pipes; (__p)++)

#define for_each_pipe_masked(__dev_priv, __p) \
	for ((__p) = 0; (__p) < __dev_priv->info.total_pipes; (__p)++) \
		for_each_if((__dev_priv->info.pipe_mask) & BIT(__p))

int phytium_get_virt_pipe(struct phytium_display_drm_private *priv, int phys_pipe);
int phytium_get_phys_pipe(struct phytium_display_drm_private *priv, int virt_pipe);

extern int dc_fake_mode_enable;
extern int dc_fast_training_check;
extern int num_source_rates;
extern int source_max_lane_count;
extern int link_dynamic_adjust;

#endif /* __PHYTIUM_DISPLAY_DRV_H__ */
