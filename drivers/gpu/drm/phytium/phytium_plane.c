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

#include <drm/drmP.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_atomic.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <linux/reservation.h>
#include <linux/dma-buf.h>

#include "phytium_display_drv.h"
#include "phytium_reg.h"
#include "phytium_plane.h"
#include "phytium_fb.h"
#include "phytium_gem.h"
#include "phytium_crtc.h"

#define PHYTIUM_CURS_W_SIZE 32
#define PHYTIUM_CURS_H_SIZE 32

static const unsigned int phytium_primary_formats[] = {
	DRM_FORMAT_ARGB2101010,
	DRM_FORMAT_ABGR2101010,
	DRM_FORMAT_RGBA1010102,
	DRM_FORMAT_BGRA1010102,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_ABGR8888,
	DRM_FORMAT_RGBA8888,
	DRM_FORMAT_BGRA8888,
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_RGBX8888,
	DRM_FORMAT_BGRX8888,
	DRM_FORMAT_ARGB4444,
	DRM_FORMAT_ABGR4444,
	DRM_FORMAT_RGBA4444,
	DRM_FORMAT_BGRA4444,
	DRM_FORMAT_XRGB4444,
	DRM_FORMAT_XBGR4444,
	DRM_FORMAT_RGBX4444,
	DRM_FORMAT_BGRX4444,
	DRM_FORMAT_ARGB1555,
	DRM_FORMAT_ABGR1555,
	DRM_FORMAT_RGBA5551,
	DRM_FORMAT_BGRA5551,
	DRM_FORMAT_XRGB1555,
	DRM_FORMAT_XBGR1555,
	DRM_FORMAT_RGBX5551,
	DRM_FORMAT_BGRX5551,
	DRM_FORMAT_RGB565,
	DRM_FORMAT_BGR565,
	DRM_FORMAT_YUYV,
	DRM_FORMAT_UYVY,
};

static uint64_t phytium_primary_formats_modifiers[] = {
	DRM_FORMAT_MOD_LINEAR,
	DRM_FORMAT_MOD_PHYTIUM_TILE_MODE0_FBCDC,
	DRM_FORMAT_MOD_PHYTIUM_TILE_MODE3_FBCDC,
	DRM_FORMAT_MOD_INVALID
};

static const unsigned int phytium_cursor_formats[] = {
	DRM_FORMAT_ARGB8888,
};

void phytium_plane_destroy(struct drm_plane *plane)
{
	struct phytium_plane *phytium_plane = to_phytium_plane(plane);

	drm_plane_cleanup(plane);
	kfree(phytium_plane);
}

/**
 * phytium_plane_atomic_get_property - fetch plane property value
 * @plane: plane to fetch property for
 * @state: state containing the property value
 * @property: property to look up
 * @val: pointer to write property value into
 *
 * The DRM core does not store shadow copies of properties for
 * atomic-capable drivers.  This entrypoint is used to fetch
 * the current value of a driver-specific plane property.
 */
static int
phytium_plane_atomic_get_property(struct drm_plane *plane,
					const struct drm_plane_state *state,
					struct drm_property *property,
					uint64_t *val)
{
	DRM_DEBUG_KMS("Unknown plane property [PROP:%d:%s]\n", property->base.id, property->name);
	return -EINVAL;
}

/**
 * phytium_plane_atomic_set_property - set plane property value
 * @plane: plane to set property for
 * @state: state to update property value in
 * @property: property to set
 * @val: value to set property to
 *
 * Writes the specified property value for a plane into the provided atomic
 * state object.
 *
 * Returns 0 on success, -EINVAL on unrecognized properties
 */
int
phytium_plane_atomic_set_property(struct drm_plane *plane,
					struct drm_plane_state *state,
					struct drm_property *property,
					uint64_t val)
{
	DRM_DEBUG_KMS("Unknown plane property [PROP:%d:%s]\n", property->base.id, property->name);
	return -EINVAL;
}

struct drm_plane_state *
phytium_plane_atomic_duplicate_state(struct drm_plane *plane)
{
	struct drm_plane_state *state = NULL;
	struct phytium_plane_state *phytium_state = NULL;

	phytium_state = kmemdup(plane->state, sizeof(*phytium_state), GFP_KERNEL);

	if (IS_ERR(phytium_state))
		return NULL;

	state = &phytium_state->base;
	if (state->fb)
		drm_framebuffer_reference(state->fb);

	return state;
}

void
phytium_plane_atomic_destroy_state(struct drm_plane *plane, struct drm_plane_state *state)
{
	struct phytium_plane_state *phytium_state = to_phytium_plane_state(state);

	if (state->fb)
		drm_framebuffer_unreference(state->fb);
	kfree(phytium_state);
}

const struct drm_plane_funcs phytium_plane_funcs = {
	.update_plane		= drm_atomic_helper_update_plane,
	.disable_plane		= drm_atomic_helper_disable_plane,
	.destroy		= phytium_plane_destroy,
	.reset			= drm_atomic_helper_plane_reset,
	.atomic_get_property	= phytium_plane_atomic_get_property,
	.atomic_set_property	= phytium_plane_atomic_set_property,
	.atomic_duplicate_state	= phytium_plane_atomic_duplicate_state,
	.atomic_destroy_state	= phytium_plane_atomic_destroy_state,
};

static int phytium_plane_prepare_fb(struct drm_plane *plane,
					       struct drm_plane_state *state)
{
	struct dma_buf *dma_buf;
	struct dma_fence *fence;

	if (!state->fb)
		return 0;
	dma_buf = to_phytium_framebuffer(state->fb)->phytium_gem_obj[0]->base.dma_buf;
	if (dma_buf) {
		fence = reservation_object_get_excl_rcu(dma_buf->resv);
		drm_atomic_set_fence_for_plane(state, fence);
	}

	return 0;
}

static int
phytium_plane_atomic_check(struct drm_plane *plane, struct drm_plane_state *state)
{
	struct drm_framebuffer *fb = state->fb;
	struct drm_crtc *crtc = state->crtc;
	struct drm_crtc_state *crtc_state;
	int src_x, src_y, src_w, src_h;
	unsigned long base_offset;
	struct phytium_crtc *phytium_crtc = to_phytium_crtc(crtc);

	if ((!fb) || (!crtc))
		return 0;

	crtc_state = drm_atomic_get_crtc_state(state->state, crtc);
	if (IS_ERR(crtc_state))
		return PTR_ERR(crtc_state);

	if (plane->type == DRM_PLANE_TYPE_CURSOR) {
		src_w = state->src_w >> 16;
		src_h = state->src_h >> 16;
		if (phytium_crtc->scale_enable)
			return -EINVAL;
		if ((src_w != PHYTIUM_CURS_W_SIZE) || (src_h != PHYTIUM_CURS_W_SIZE)) {
			DRM_INFO("Invalid cursor size(%d, %d)\n", src_w, src_h);
			return -EINVAL;
		}
	} else if (plane->type == DRM_PLANE_TYPE_PRIMARY) {
		src_x = state->src_x >> 16;
		src_y = state->src_y >> 16;
		src_w = state->src_w >> 16;
		src_h = state->src_h >> 16;

		base_offset = src_x * fb->format->cpp[0] + src_y*fb->pitches[0];
		if (base_offset & (~ADDRESS_MASK)) {
			DRM_ERROR("fb base address is not aligned by 128 byte\n");
			return -EINVAL;
		}

		if (src_w != state->crtc_w || src_h != state->crtc_h) {
			DRM_ERROR("scale not support: crtc_w(0x%x)/h(0x%x) src_w(0x%x)/h(0x%x)\n",
				   state->crtc_w, state->crtc_h, src_w, src_h);
			return -EINVAL;
		}

		if ((state->crtc_x < 0) || (state->crtc_y < 0)) {
			DRM_ERROR("crtc_x(0x%x)/y(0x%x) of drm plane state is invalid\n",
				   state->crtc_x, state->crtc_y);
			return -EINVAL;
		}

		if ((state->crtc_x + state->crtc_w > crtc_state->adjusted_mode.hdisplay)
			|| (state->crtc_y + state->crtc_h > crtc_state->adjusted_mode.vdisplay)) {
			DRM_ERROR("plane out of crtc region\n");
			return -EINVAL;
		}
	}

	return 0;
}

static void phytium_plane_atomic_update(struct drm_plane *plane,
						   struct drm_plane_state *old_state)
{
	int i, num_planes = 0;
	struct drm_framebuffer *fb, *old_fb;
	struct drm_device *dev = plane->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	struct phytium_plane *phytium_plane = to_phytium_plane(plane);
	struct phytium_framebuffer *phytium_fb = NULL;
	struct phytium_gem_object *phytium_gem_obj;
	int config;
	int phys_pipe = phytium_plane->phys_pipe;
	int src_x, src_y, crtc_x, crtc_y, crtc_w, crtc_h;
	unsigned long base_offset;

	DRM_DEBUG_KMS("update plane: type=%d\n", plane->type);
	if (!plane->state->crtc || !plane->state->fb)
		return;

	src_x = plane->state->src_x >> 16;
	src_y = plane->state->src_y >> 16;
	crtc_x = plane->state->crtc_x;
	crtc_y = plane->state->crtc_y;
	crtc_w = plane->state->crtc_w;
	crtc_h = plane->state->crtc_h;
	fb = plane->state->fb;
	old_fb = old_state->fb;

	if (fb) {
		num_planes = drm_format_num_planes(fb->format->format);
		drm_framebuffer_reference(fb);
	}

	if (old_fb)
		drm_framebuffer_unreference(old_fb);

	for (i = 0; i < num_planes; i++) {
		phytium_fb = to_phytium_framebuffer(fb);
		phytium_gem_obj = phytium_fb->phytium_gem_obj[i];
		phytium_plane->iova[i] =  phytium_gem_obj->iova + fb->offsets[i];
		phytium_plane->size[i] = phytium_gem_obj->size - fb->offsets[i];
		phytium_plane->tiling[i] = phytium_gem_obj->tiling;

		if (i == 0) {
			switch (fb->format->format) {
			case DRM_FORMAT_ARGB2101010:
			case DRM_FORMAT_ABGR2101010:
			case DRM_FORMAT_RGBA1010102:
			case DRM_FORMAT_BGRA1010102:
				phytium_plane->format = FRAMEBUFFER_FORMAT_ARGB2101010;
				break;

			case DRM_FORMAT_ARGB8888:
			case DRM_FORMAT_ABGR8888:
			case DRM_FORMAT_RGBA8888:
			case DRM_FORMAT_BGRA8888:
				phytium_plane->format = FRAMEBUFFER_FORMAT_ARGB8888;
				break;

			case DRM_FORMAT_XRGB8888:
			case DRM_FORMAT_XBGR8888:
			case DRM_FORMAT_RGBX8888:
			case DRM_FORMAT_BGRX8888:
				phytium_plane->format = FRAMEBUFFER_FORMAT_XRGB8888;
				break;

			case DRM_FORMAT_ARGB4444:
			case DRM_FORMAT_ABGR4444:
			case DRM_FORMAT_RGBA4444:
			case DRM_FORMAT_BGRA4444:
				phytium_plane->format = FRAMEBUFFER_FORMAT_ARGB4444;
				break;

			case DRM_FORMAT_XRGB4444:
			case DRM_FORMAT_XBGR4444:
			case DRM_FORMAT_RGBX4444:
			case DRM_FORMAT_BGRX4444:
				phytium_plane->format = FRAMEBUFFER_FORMAT_XRGB4444;
				break;

			case DRM_FORMAT_ARGB1555:
			case DRM_FORMAT_ABGR1555:
			case DRM_FORMAT_RGBA5551:
			case DRM_FORMAT_BGRA5551:
				phytium_plane->format = FRAMEBUFFER_FORMAT_ARGB1555;
				break;

			case DRM_FORMAT_XRGB1555:
			case DRM_FORMAT_XBGR1555:
			case DRM_FORMAT_RGBX5551:
			case DRM_FORMAT_BGRX5551:
				phytium_plane->format = FRAMEBUFFER_FORMAT_XRGB1555;
				break;

			case DRM_FORMAT_RGB565:
			case DRM_FORMAT_BGR565:
				phytium_plane->format = FRAMEBUFFER_FORMAT_RGB565;
				break;

			case DRM_FORMAT_YUYV:
				phytium_plane->format = FRAMEBUFFER_FORMAT_YUYV;
				break;

			case DRM_FORMAT_UYVY:
				phytium_plane->format = FRAMEBUFFER_FORMAT_UYVY;
				break;

			default:
				DRM_ERROR("unsupported pixel format (format = %d)\n",
					  fb->format->format);
				return;
			}

			switch (fb->format->format) {
			case DRM_FORMAT_ARGB2101010:
			case DRM_FORMAT_ARGB8888:
			case DRM_FORMAT_XRGB8888:
			case DRM_FORMAT_ARGB4444:
			case DRM_FORMAT_XRGB4444:
			case DRM_FORMAT_ARGB1555:
			case DRM_FORMAT_XRGB1555:
			case DRM_FORMAT_RGB565:
				phytium_plane->swizzle = FRAMEBUFFER_SWIZZLE_ARGB;
				phytium_plane->uv_swizzle = FRAMEBUFFER_UVSWIZZLE_DISABLE;
				break;

			case DRM_FORMAT_ABGR2101010:
			case DRM_FORMAT_ABGR8888:
			case DRM_FORMAT_XBGR8888:
			case DRM_FORMAT_ABGR4444:
			case DRM_FORMAT_XBGR4444:
			case DRM_FORMAT_ABGR1555:
			case DRM_FORMAT_XBGR1555:
			case DRM_FORMAT_BGR565:
				phytium_plane->swizzle = FRAMEBUFFER_SWIZZLE_ABGR;
				phytium_plane->uv_swizzle = FRAMEBUFFER_UVSWIZZLE_DISABLE;
				break;

			case DRM_FORMAT_RGBA1010102:
			case DRM_FORMAT_RGBA8888:
			case DRM_FORMAT_RGBX8888:
			case DRM_FORMAT_RGBA4444:
			case DRM_FORMAT_RGBX4444:
			case DRM_FORMAT_RGBA5551:
			case DRM_FORMAT_RGBX5551:
				phytium_plane->swizzle = FRAMEBUFFER_SWIZZLE_RGBA;
				phytium_plane->uv_swizzle = FRAMEBUFFER_UVSWIZZLE_DISABLE;
				break;

			case DRM_FORMAT_BGRA1010102:
			case DRM_FORMAT_BGRA8888:
			case DRM_FORMAT_BGRX8888:
			case DRM_FORMAT_BGRA4444:
			case DRM_FORMAT_BGRX4444:
			case DRM_FORMAT_BGRA5551:
			case DRM_FORMAT_BGRX5551:
				phytium_plane->swizzle = FRAMEBUFFER_SWIZZLE_BGRA;
				phytium_plane->uv_swizzle = FRAMEBUFFER_UVSWIZZLE_DISABLE;
				break;

			case DRM_FORMAT_YUYV:
			case DRM_FORMAT_UYVY:
				phytium_plane->swizzle = FRAMEBUFFER_SWIZZLE_ARGB;
				phytium_plane->uv_swizzle = FRAMEBUFFER_UVSWIZZLE_DISABLE;
				break;

			default:
				DRM_ERROR("unsupported pixel format (format = %d)\n",
					   fb->format->format);
				return;
			}
		}
	}

	if (plane->type == DRM_PLANE_TYPE_PRIMARY) {
		if (phytium_plane->tiling[0] == FRAMEBUFFER_LINEAR) {
			phytium_writel_reg(priv, DCREQ_MODE_LINEAR,
					   PHYTIUM_DCREQ_PLANE0_CONFIG(phys_pipe));
		} else {
			config = DCREQ_NO_LOSSY;
			if (phytium_plane->tiling[0] == FRAMEBUFFER_TILE_MODE0)
				config |= DCREQ_TILE_TYPE_MODE0;
			else if (phytium_plane->tiling[0] == FRAMEBUFFER_TILE_MODE3)
				config |= DCREQ_TILE_TYPE_MODE3;
			else
				config |= DCREQ_TILE_TYPE_MODE0;

			switch (phytium_plane->format) {
			case FRAMEBUFFER_FORMAT_ARGB8888:
			case FRAMEBUFFER_FORMAT_XRGB8888:
				config |= DCREQ_COLOURFORMAT_BGRA8888;
				break;
			case FRAMEBUFFER_FORMAT_ARGB2101010:
				config |= DCREQ_COLOURFORMAT_ARGB2101010;
				break;
			case FRAMEBUFFER_FORMAT_XRGB4444:
			case FRAMEBUFFER_FORMAT_ARGB4444:
				config |= DCREQ_COLOURFORMAT_ARGB4444;
				break;
			case FRAMEBUFFER_FORMAT_XRGB1555:
			case FRAMEBUFFER_FORMAT_ARGB1555:
				config |= DCREQ_COLOURFORMAT_ARGB1555;
				break;
			case FRAMEBUFFER_FORMAT_RGB565:
				config |= DCREQ_COLOURFORMAT_RGB565;
				break;
			case FRAMEBUFFER_FORMAT_YUYV:
				config |= DCREQ_COLOURFORMAT_YUYV;
				break;
			case FRAMEBUFFER_FORMAT_UYVY:
				config |= DCREQ_COLOURFORMAT_UYVY;
				break;
			}
			config |= DCREQ_ARGBSWIZZLE_ARGB;
			config |= DCREQ_MODE_TILE;
			phytium_writel_reg(priv, phytium_plane->iova[0] & 0xffffffff,
					   PHYTIUM_DCREQ_PLANE0_ADDR_START(phys_pipe));
			phytium_writel_reg(priv, (phytium_plane->iova[0] + phytium_plane->size[0]) &
				0xffffffff, PHYTIUM_DCREQ_PLANE0_ADDR_END(phys_pipe));
			phytium_writel_reg(priv, config, PHYTIUM_DCREQ_PLANE0_CONFIG(phys_pipe));
		}

		/* config dc */
		/* Y */
		base_offset = src_x * fb->format->cpp[0] + src_y*fb->pitches[0];
		phytium_writel_reg(priv, (phytium_plane->iova[0] >> PREFIX_SHIFT) & PREFIX_MASK,
				   PHYTIUM_DCREQ_PIX_DMA_PREFIX(phys_pipe));
		phytium_writel_reg(priv, (phytium_plane->iova[0] + base_offset) & ADDRESS_MASK,
				   PHYTIUM_DC_FRAMEBUFFER_Y_ADDRESS(phys_pipe));
		phytium_writel_reg(priv, ALIGN(fb->pitches[0], 128),
				   PHYTIUM_DC_FRAMEBUFFER_Y_STRIDE(phys_pipe));

		/* U */
		phytium_writel_reg(priv, phytium_plane->iova[1] & 0xffffffff,
				   PHYTIUM_DC_FRAMEBUFFER_U_ADDRESS(phys_pipe));
		phytium_writel_reg(priv, ALIGN(fb->pitches[1], 128),
				   PHYTIUM_DC_FRAMEBUFFER_U_STRIDE(phys_pipe));

		/* V */
		phytium_writel_reg(priv, phytium_plane->iova[2] & 0xffffffff,
				   PHYTIUM_DC_FRAMEBUFFER_V_ADDRESS(phys_pipe));
		phytium_writel_reg(priv, ALIGN(fb->pitches[2], 128),
				   PHYTIUM_DC_FRAMEBUFFER_V_STRIDE(phys_pipe));

		/* size */
		phytium_writel_reg(priv, (crtc_w & WIDTH_MASK) | ((crtc_h&HEIGHT_MASK)
				   << HEIGHT_SHIFT), PHYTIUM_DC_FRAMEBUFFER_SIZE(phys_pipe));
		/* config */
		config = phytium_readl_reg(priv, PHYTIUM_DC_FRAMEBUFFER_CONFIG(phys_pipe));
		config &= ~(FRAMEBUFFER_FORMAT_MASK << FRAMEBUFFER_FORMAT_SHIFT);
		config |= (phytium_plane->format << FRAMEBUFFER_FORMAT_SHIFT);
		config &= ~(1 << FRAMEBUFFER_UVSWIZZLE_SHIFT);
		config |= (phytium_plane->uv_swizzle << FRAMEBUFFER_UVSWIZZLE_SHIFT);
		config &= ~(FRAMEBUFFER_SWIZZLE_MASK << FRAMEBUFFER_SWIZZLE_SHIFT);
		config |= (phytium_plane->swizzle << FRAMEBUFFER_SWIZZLE_SHIFT);
		config &= ~(FRAMEBUFFER_TILE_MODE_MASK << FRAMEBUFFER_TILE_MODE_SHIFT);
		config |= (phytium_plane->tiling[0] << FRAMEBUFFER_TILE_MODE_SHIFT);
		config &= (~FRAMEBUFFER_CLEAR);
		phytium_writel_reg(priv, config, PHYTIUM_DC_FRAMEBUFFER_CONFIG(phys_pipe));
	} else if (plane->type == DRM_PLANE_TYPE_CURSOR) {
		phytium_plane->enable = 1;
		phytium_plane->cursor_hot_x = fb->hot_x;
		phytium_plane->cursor_hot_y = fb->hot_y;
		phytium_plane->cursor_x = plane->state->crtc_x + fb->hot_x;
		phytium_plane->cursor_y = plane->state->crtc_y + fb->hot_y;

		config = CURSOR_FORMAT_ARGB8888 |
			((phytium_plane->cursor_hot_y & CURSOR_HOT_Y_MASK) << CURSOR_HOT_Y_SHIFT) |
			((phytium_plane->cursor_hot_x & CURSOR_HOT_X_MASK) << CURSOR_HOT_X_SHIFT);

		phytium_writel_reg(priv, config, PHYTIUM_DC_CURSOR_CONFIG(phys_pipe));
		phytium_writel_reg(priv,
				   ((phytium_plane->cursor_x & CURSOR_X_MASK) << CURSOR_X_SHIFT) |
				   ((phytium_plane->cursor_y & CURSOR_Y_MASK) << CURSOR_Y_SHIFT),
				   PHYTIUM_DC_CURSOR_LOCATION(phys_pipe));
		phytium_writel_reg(priv, phytium_plane->iova[0],
			PHYTIUM_DC_CURSOR_ADDRESS(phys_pipe));
	}
}

static void phytium_plane_atomic_disable(struct drm_plane *plane,
						    struct drm_plane_state *old_state)
{
	struct drm_device *dev = plane->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	struct phytium_plane *phytium_plane = to_phytium_plane(plane);
	struct drm_framebuffer *old_fb;
	int phys_pipe = phytium_plane->phys_pipe;
	int config;

	old_fb = old_state->fb;
	if (old_fb)
		drm_framebuffer_unreference(old_fb);

	if (plane->type == DRM_PLANE_TYPE_PRIMARY) {
		phytium_writel_reg(priv, CLEAR_VALUE_RED,
				   PHYTIUM_DC_FRAMEBUFFER_CLEARVALUE(phys_pipe));
		config = phytium_readl_reg(priv, PHYTIUM_DC_FRAMEBUFFER_CONFIG(phys_pipe));
		config |= FRAMEBUFFER_CLEAR;
		phytium_writel_reg(priv, config, PHYTIUM_DC_FRAMEBUFFER_CONFIG(phys_pipe));
	} else if (plane->type == DRM_PLANE_TYPE_CURSOR) {
		phytium_writel_reg(priv, CURSOR_FORMAT_DISABLED,
				   PHYTIUM_DC_CURSOR_CONFIG(phys_pipe));
	}
}

const struct drm_plane_helper_funcs phytium_plane_helper_funcs = {
	.prepare_fb =  phytium_plane_prepare_fb,
	.atomic_check	= phytium_plane_atomic_check,
	.atomic_update	= phytium_plane_atomic_update,
	.atomic_disable	= phytium_plane_atomic_disable,
};

struct phytium_plane *phytium_primary_plane_create(struct drm_device *dev, int phys_pipe)
{
	struct phytium_plane *phytium_primary_plane = NULL;
	struct phytium_plane_state *phytium_primary_plane_state = NULL;
	int ret = 0;
	unsigned int flags = 0;

	phytium_primary_plane = kzalloc(sizeof(*phytium_primary_plane), GFP_KERNEL);
	if (IS_ERR(phytium_primary_plane)) {
		ret = -ENOMEM;
		goto failed_malloc_plane;
	}

	phytium_primary_plane_state = kzalloc(sizeof(*phytium_primary_plane_state), GFP_KERNEL);
	if (IS_ERR(phytium_primary_plane_state)) {
		ret = -ENOMEM;
		goto failed_malloc_plane_state;
	}
	phytium_primary_plane_state->base.plane = &phytium_primary_plane->base;
	phytium_primary_plane_state->base.rotation = DRM_MODE_ROTATE_0;
	phytium_primary_plane->base.state = &phytium_primary_plane_state->base;
	phytium_primary_plane->phys_pipe = phys_pipe;

	ret = drm_universal_plane_init(dev, &phytium_primary_plane->base, 0x0,
				       &phytium_plane_funcs, phytium_primary_formats,
				       ARRAY_SIZE(phytium_primary_formats),
				       phytium_primary_formats_modifiers,
				       DRM_PLANE_TYPE_PRIMARY, "primary %d", phys_pipe);

	if (ret)
		goto failed_plane_init;

	flags = DRM_MODE_ROTATE_0;
	drm_plane_create_rotation_property(&phytium_primary_plane->base, DRM_MODE_ROTATE_0, flags);
	drm_plane_helper_add(&phytium_primary_plane->base, &phytium_plane_helper_funcs);

	return phytium_primary_plane;
failed_plane_init:
	kfree(phytium_primary_plane_state);
failed_malloc_plane_state:
	kfree(phytium_primary_plane);
failed_malloc_plane:
	return ERR_PTR(ret);
}

struct phytium_plane *phytium_cursor_plane_create(struct drm_device *dev, int phys_pipe)
{
	struct phytium_plane *phytium_cursor_plane = NULL;
	struct phytium_plane_state *phytium_cursor_plane_state = NULL;
	int ret = 0;
	unsigned int flags = 0;

	phytium_cursor_plane = kzalloc(sizeof(*phytium_cursor_plane), GFP_KERNEL);
	if (IS_ERR(phytium_cursor_plane)) {
		ret = -ENOMEM;
		goto failed_malloc_plane;
	}

	phytium_cursor_plane_state = kzalloc(sizeof(*phytium_cursor_plane_state), GFP_KERNEL);
	if (IS_ERR(phytium_cursor_plane_state)) {
		ret = -ENOMEM;
		goto failed_malloc_plane_state;
	}
	phytium_cursor_plane_state->base.plane = &phytium_cursor_plane->base;
	phytium_cursor_plane_state->base.rotation = DRM_MODE_ROTATE_0;
	phytium_cursor_plane->base.state = &phytium_cursor_plane_state->base;
	phytium_cursor_plane->phys_pipe = phys_pipe;

	ret = drm_universal_plane_init(dev, &phytium_cursor_plane->base, 0x0,
				       &phytium_plane_funcs,
				       phytium_cursor_formats, ARRAY_SIZE(phytium_cursor_formats),
				       NULL, DRM_PLANE_TYPE_CURSOR, "cursor %d", phys_pipe);

	if (ret)
		goto failed_plane_init;

	flags = DRM_MODE_ROTATE_0;
	drm_plane_create_rotation_property(&phytium_cursor_plane->base, DRM_MODE_ROTATE_0, flags);
	drm_plane_helper_add(&phytium_cursor_plane->base, &phytium_plane_helper_funcs);

	return phytium_cursor_plane;
failed_plane_init:
	kfree(phytium_cursor_plane_state);
failed_malloc_plane_state:
	kfree(phytium_cursor_plane);
failed_malloc_plane:
	return ERR_PTR(ret);
}
