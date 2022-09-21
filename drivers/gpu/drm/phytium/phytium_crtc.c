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
#include <asm/neon.h>
#include "phytium_display_drv.h"
#include "phytium_reg.h"
#include "phytium_crtc.h"
#include "phytium_plane.h"
#include "phytium_dp.h"

#define MAXKERNELSIZE		9
#define SUBPIXELINDEXBITS	5
#define SUBPIXELCOUNT		(1 << SUBPIXELINDEXBITS)
#define SUBPIXELLOADCOUNT	(SUBPIXELCOUNT / 2 + 1)
#define WEIGHTSTATECOUNT	(((SUBPIXELLOADCOUNT * MAXKERNELSIZE + 1) & ~1) / 2)
#define KERNELTABLESIZE		(SUBPIXELLOADCOUNT * MAXKERNELSIZE * sizeof(uint16_t))
#define PHYALIGN(n, align)	(((n) + ((align) - 1)) & ~((align) - 1))
#define KERNELSTATES		(PHYALIGN(KERNELTABLESIZE + 4, 8))
#define PHYPI			3.14159265358979323846f

#define PHYTIUM_MATH_Add(X, Y)			(float)((X) + (Y))
#define PHYTIUM_MATH_Multiply(X, Y)		(float)((X) * (Y))
#define PHYTIUM_MATH_Divide(X, Y)		(float)((X) / (Y))
#define PHYTIUM_MATH_DivideFromUInteger(X, Y)	((float)(X) / (float)(Y))
#define PHYTIUM_MATH_I2Float(X)		(float)(X)

static int phytium_crtc_gamma_set(struct drm_crtc *crtc, u16 *red,
					   u16 *green, u16 *blue, uint32_t size,
					   struct drm_modeset_acquire_ctx *ctx)
{
	return 0;
}

static void phytium_crtc_destroy(struct drm_crtc *crtc)
{
	struct phytium_crtc *phytium_crtc = to_phytium_crtc(crtc);

	drm_crtc_cleanup(crtc);
	kfree(phytium_crtc);
}

struct drm_crtc_state *
phytium_crtc_atomic_duplicate_state(struct drm_crtc *crtc)
{
	struct phytium_crtc_state *phytium_crtc_state = NULL;

	phytium_crtc_state = kmemdup(crtc->state, sizeof(*phytium_crtc_state),
				     GFP_KERNEL);
	if (IS_ERR(phytium_crtc_state))
		return NULL;
	__drm_atomic_helper_crtc_duplicate_state(crtc,
						 &phytium_crtc_state->base);

	return &phytium_crtc_state->base;
}

void
phytium_crtc_atomic_destroy_state(struct drm_crtc *crtc,
					       struct drm_crtc_state *state)
{
	struct phytium_crtc_state *phytium_crtc_state =
					to_phytium_crtc_state(state);

	phytium_crtc_state = to_phytium_crtc_state(state);
	__drm_atomic_helper_crtc_destroy_state(state);
	kfree(phytium_crtc_state);
}

static const struct drm_crtc_funcs phytium_crtc_funcs = {
	.gamma_set		= phytium_crtc_gamma_set,
	.set_config		= drm_atomic_helper_set_config,
	.destroy		= phytium_crtc_destroy,
	.page_flip		= drm_atomic_helper_page_flip,
	.reset			= drm_atomic_helper_crtc_reset,
	.atomic_duplicate_state = phytium_crtc_atomic_duplicate_state,
	.atomic_destroy_state   = phytium_crtc_atomic_destroy_state,
};

struct filter_blit_array {
	uint8_t kernelSize;
	uint32_t scaleFactor;
	uint32_t *kernelStates;
};

uint32_t phytium_crtc_scaling_get_factor(uint32_t src_size, uint32_t dst_size)
{
	uint32_t factor = 0;

	factor = ((src_size - 1) << SCALE_FACTOR_SRC_OFFSET) / (dst_size - 1);

	return factor;
}

float phytium_sint(float x)
{
	const float B = 1.2732395477;
	const float C = -0.4052847346;
	const float P = 0.2310792853;
	float y;

	if (x < 0)
		y = B*x - C*x*x;
	else
		y = B*x + C*x*x;
	if (y < 0)
		y = P * (y * (0 - y) - y) + y;
	else
		y = P * (y * y - y) + y;
	return y;
}

float phytium_sinc_filter(float x, int radius)
{
	float pit, pitd, f1, f2, result;
	float f_radius = PHYTIUM_MATH_I2Float(radius);

	if (x == 0.0f) {
		result = 1.0f;
	} else if ((x < -f_radius) || (x > f_radius)) {
		result = 0.0f;
	} else {
		pit  = PHYTIUM_MATH_Multiply(PHYPI, x);
		pitd = PHYTIUM_MATH_Divide(pit, f_radius);
		f1 = PHYTIUM_MATH_Divide(phytium_sint(pit), pit);
		f2 = PHYTIUM_MATH_Divide(phytium_sint(pitd), pitd);
		result = PHYTIUM_MATH_Multiply(f1, f2);
	}

	return result;
}

int calculate_sync_table(
	uint8_t kernel_size,
	uint32_t src_size,
	uint32_t dst_size,
	struct filter_blit_array *kernel_info)
{
	uint32_t scale_factor;
	float f_scale;
	int kernel_half;
	float f_subpixel_step;
	float f_subpixel_offset;
	uint32_t subpixel_pos;
	int kernel_pos;
	int padding;
	uint16_t *kernel_array;
	int range = 0;

	do {
		/* Compute the scale factor. */
		scale_factor = phytium_crtc_scaling_get_factor(src_size, dst_size);

		/* Same kernel size and ratio as before? */
		if ((kernel_info->kernelSize  == kernel_size) &&
		(kernel_info->scaleFactor == kernel_size)) {
			break;
		}

		/* check the array */
		if (kernel_info->kernelStates == NULL)
			break;

		/* Store new parameters. */
		kernel_info->kernelSize  = kernel_size;
		kernel_info->scaleFactor = scale_factor;

		/* Compute the scale factor. */
		f_scale = PHYTIUM_MATH_DivideFromUInteger(dst_size, src_size);

		/* Adjust the factor for magnification. */
		if (f_scale > 1.0f)
			f_scale = 1.0f;

		/* Calculate the kernel half. */
		kernel_half = (int) (kernel_info->kernelSize >> 1);

		/* Calculate the subpixel step. */
		f_subpixel_step = PHYTIUM_MATH_Divide(1.0f, PHYTIUM_MATH_I2Float(SUBPIXELCOUNT));

		/* Init the subpixel offset. */
		f_subpixel_offset = 0.5f;

		/* Determine kernel padding size. */
		padding = (MAXKERNELSIZE - kernel_info->kernelSize) / 2;

		/* Set initial kernel array pointer. */
		kernel_array = (uint16_t *) (kernel_info->kernelStates + 1);

		/* Loop through each subpixel. */
		for (subpixel_pos = 0; subpixel_pos < SUBPIXELLOADCOUNT; subpixel_pos++) {
			/* Define a temporary set of weights. */
			float fSubpixelSet[MAXKERNELSIZE];

			/* Init the sum of all weights for the current subpixel. */
			float fWeightSum = 0.0f;
			uint16_t weightSum = 0;
			short int adjustCount, adjustFrom;
			short int adjustment;

			/* Compute weights. */
			for (kernel_pos = 0; kernel_pos < MAXKERNELSIZE; kernel_pos++) {
				/* Determine the current index. */
				int index = kernel_pos - padding;

				/* Pad with zeros. */
				if ((index < 0) || (index >= kernel_info->kernelSize)) {
					fSubpixelSet[kernel_pos] = 0.0f;
				} else {
					if (kernel_info->kernelSize == 1) {
						fSubpixelSet[kernel_pos] = 1.0f;
					} else {
						/* Compute the x position for filter function. */
						float fX = PHYTIUM_MATH_Add(
							PHYTIUM_MATH_I2Float(index - kernel_half),
							f_subpixel_offset);
						fX = PHYTIUM_MATH_Multiply(fX, f_scale);

						/* Compute the weight. */
						fSubpixelSet[kernel_pos] = phytium_sinc_filter(fX,
									   kernel_half);
					}

					/* Update the sum of weights. */
					fWeightSum = PHYTIUM_MATH_Add(fWeightSum,
								      fSubpixelSet[kernel_pos]);
				}
			}

			/* Adjust weights so that the sum will be 1.0. */
			for (kernel_pos = 0; kernel_pos < MAXKERNELSIZE; kernel_pos++) {
				/* Normalize the current weight. */
				float fWeight = PHYTIUM_MATH_Divide(fSubpixelSet[kernel_pos],
								    fWeightSum);

				/* Convert the weight to fixed point and store in the table. */
				if (fWeight == 0.0f)
					kernel_array[kernel_pos] = 0x0000;
				else if (fWeight >= 1.0f)
					kernel_array[kernel_pos] = 0x4000;
				else if (fWeight <= -1.0f)
					kernel_array[kernel_pos] = 0xC000;
				else
					kernel_array[kernel_pos] =
						(int16_t) PHYTIUM_MATH_Multiply(fWeight, 16384.0f);
				weightSum += kernel_array[kernel_pos];
			}

			/* Adjust the fixed point coefficients. */
			adjustCount = 0x4000 - weightSum;
			if (adjustCount < 0) {
				adjustCount = -adjustCount;
				adjustment = -1;
			} else {
				adjustment = 1;
			}

			adjustFrom = (MAXKERNELSIZE - adjustCount) / 2;
			for (kernel_pos = 0; kernel_pos < adjustCount; kernel_pos++) {
				range = (MAXKERNELSIZE*subpixel_pos + adjustFrom + kernel_pos) *
					sizeof(uint16_t);
				if ((range >= 0) && (range < KERNELTABLESIZE))
					kernel_array[adjustFrom + kernel_pos] += adjustment;
				else
					DRM_ERROR("%s failed\n", __func__);
			}

			kernel_array += MAXKERNELSIZE;

			/* Advance to the next subpixel. */
			f_subpixel_offset = PHYTIUM_MATH_Add(f_subpixel_offset, -f_subpixel_step);
		}
	} while (0);

	return 0;
}

void phytium_crtc_scaling_config(struct drm_crtc *crtc,
				     struct drm_crtc_state *old_state)
{
	struct drm_device *dev = crtc->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	struct drm_display_mode *mode = &crtc->state->adjusted_mode;
	struct phytium_crtc *phytium_crtc = to_phytium_crtc(crtc);
	int phys_pipe = phytium_crtc->phys_pipe;
	uint32_t scale_factor_x, scale_factor_y, i;
	uint32_t kernelStates[128];
	struct filter_blit_array kernel_info_width;
	void *tmp =  NULL;

	if (mode->hdisplay != mode->crtc_hdisplay || mode->vdisplay != mode->crtc_vdisplay) {
		phytium_crtc->src_width = mode->hdisplay;
		phytium_crtc->src_height = mode->vdisplay;
		phytium_crtc->dst_width = mode->crtc_hdisplay;
		phytium_crtc->dst_height = mode->crtc_vdisplay;

		phytium_crtc->dst_x = (mode->crtc_hdisplay - phytium_crtc->dst_width) / 2;
		phytium_crtc->dst_y = (mode->crtc_vdisplay - phytium_crtc->dst_height) / 2;

		scale_factor_x = phytium_crtc_scaling_get_factor(phytium_crtc->src_width,
								 phytium_crtc->dst_width);
		scale_factor_y = phytium_crtc_scaling_get_factor(phytium_crtc->src_height,
								 phytium_crtc->dst_height);
		if (scale_factor_y > (SCALE_FACTOR_Y_MAX << SCALE_FACTOR_SRC_OFFSET))
			scale_factor_y = (SCALE_FACTOR_Y_MAX << SCALE_FACTOR_SRC_OFFSET);

		phytium_writel_reg(priv, scale_factor_x & SCALE_FACTOR_X_MASK,
				   PHYTIUM_DC_FRAMEBUFFER_SCALE_FACTOR_X(phys_pipe));
		phytium_writel_reg(priv, scale_factor_y & SCALE_FACTOR_Y_MASK,
				   PHYTIUM_DC_FRAMEBUFFER_SCALE_FACTOR_Y(phys_pipe));
		phytium_writel_reg(priv, FRAMEBUFFER_TAP,
				   PHYTIUM_DC_FRAMEBUFFER_SCALECONFIG(phys_pipe));

		tmp = kmalloc(KERNELSTATES, GFP_KERNEL);
		if (IS_ERR(tmp)) {
			DRM_ERROR("malloc %ld failed\n", KERNELSTATES);
			return;
		}

		memset(&kernel_info_width, 0, sizeof(struct filter_blit_array));
		kernel_info_width.kernelStates = tmp;
		memset(kernel_info_width.kernelStates, 0, KERNELSTATES);
		kernel_neon_begin();
		calculate_sync_table(FRAMEBUFFER_HORIZONTAL_FILTER_TAP, phytium_crtc->src_width,
				     phytium_crtc->dst_width, &kernel_info_width);
		memset(kernelStates, 0, sizeof(kernelStates));
		memcpy(kernelStates, kernel_info_width.kernelStates + 1, KERNELSTATES - 4);
		kernel_neon_end();
		phytium_writel_reg(priv, HORI_FILTER_INDEX,
				   PHYTIUM_DC_FRAMEBUFFER_HORI_FILTER_INDEX(phys_pipe));
		for (i = 0; i < 128; i++) {
			phytium_writel_reg(priv, kernelStates[i],
					   PHYTIUM_DC_FRAMEBUFFER_HORI_FILTER(phys_pipe));
		}

		memset(&kernel_info_width, 0, sizeof(struct filter_blit_array));
		kernel_info_width.kernelStates = tmp;
		memset(kernel_info_width.kernelStates, 0, KERNELSTATES);
		kernel_neon_begin();
		calculate_sync_table(FRAMEBUFFER_FILTER_TAP, phytium_crtc->src_height,
				     phytium_crtc->dst_height, &kernel_info_width);
		memset(kernelStates, 0, sizeof(kernelStates));
		memcpy(kernelStates, kernel_info_width.kernelStates + 1, KERNELSTATES - 4);
		kernel_neon_end();
		phytium_writel_reg(priv, VERT_FILTER_INDEX,
				   PHYTIUM_DC_FRAMEBUFFER_VERT_FILTER_INDEX(phys_pipe));
		for (i = 0; i < 128; i++)
			phytium_writel_reg(priv, kernelStates[i],
					   PHYTIUM_DC_FRAMEBUFFER_VERT_FILTER(phys_pipe));
		phytium_writel_reg(priv, INITIALOFFSET,
				   PHYTIUM_DC_FRAMEBUFFER_INITIALOFFSET(phys_pipe));
		kfree(tmp);
		phytium_crtc->scale_enable = true;
	} else {
		phytium_crtc->scale_enable = false;
	}
}

static void
phytium_crtc_atomic_enable(struct drm_crtc *crtc,
				     struct drm_crtc_state *old_state)
{
	struct drm_device *dev = crtc->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	struct drm_display_mode *mode = &crtc->state->adjusted_mode;
	struct drm_atomic_state *state = old_state->state;
	struct drm_connector_state *new_conn_state;
	struct drm_connector *conn;
	struct phytium_crtc *phytium_crtc = to_phytium_crtc(crtc);
	int phys_pipe = phytium_crtc->phys_pipe;
	int timeout = 100;
	int config = 0, i = 0;

	for_each_new_connector_in_state(state, conn, new_conn_state, i) {
		if (new_conn_state->crtc != crtc)
			continue;

		switch (conn->display_info.bpc) {
		case 10:
			phytium_crtc->bpc = DP_RGB101010;
			break;
		case 6:
			phytium_crtc->bpc = DP_RGB666;
			break;
		default:
			phytium_crtc->bpc = DP_RGB888;
			break;
		}
	}

	/* config pix clock */
	timeout = 100;
	phytium_writel_reg(priv, (mode->clock & PIX_CLOCK_MASK) | FLAG_REQUEST,
			   PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(phys_pipe));

	do {
		mdelay(10);
		timeout--;
		config = phytium_readl_reg(priv, PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(phys_pipe));
	} while ((!(config & FLAG_REPLY)) && timeout);
	if (timeout == 0)
		DRM_ERROR("config pix clock(%d kHz) failed\n", mode->clock);
	phytium_writel_reg(priv, mode->clock & PIX_CLOCK_MASK,
			   PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(phys_pipe));
	mdelay(20);

	phytium_crtc_scaling_config(crtc, old_state);
	config = ((mode->crtc_hdisplay & HDISPLAY_END_MASK) << HDISPLAY_END_SHIFT)
		| ((mode->crtc_htotal&HDISPLAY_TOTAL_MASK) << HDISPLAY_TOTAL_SHIFT);
	phytium_writel_reg(priv, config, PHYTIUM_DC_HDISPLAY(phys_pipe));
	config = ((mode->crtc_hsync_start & HSYNC_START_MASK) << HSYNC_START_SHIFT)
			| ((mode->crtc_hsync_end & HSYNC_END_MASK) << HSYNC_END_SHIFT)
			| HSYNC_PULSE_ENABLED;
	config |= (mode->flags & DRM_MODE_FLAG_PHSYNC) ? 0 : HSYNC_NEGATIVE;
	phytium_writel_reg(priv, config, PHYTIUM_DC_HSYNC(phys_pipe));
	config = ((mode->crtc_vdisplay & VDISPLAY_END_MASK) << VDISPLAY_END_SHIFT)
		| ((mode->crtc_vtotal & VDISPLAY_TOTAL_MASK) << VDISPLAY_TOTAL_SHIFT);
	phytium_writel_reg(priv, config, PHYTIUM_DC_VDISPLAY(phys_pipe));
	config = ((mode->crtc_vsync_start & VSYNC_START_MASK) << VSYNC_START_SHIFT)
		| ((mode->crtc_vsync_end & VSYNC_END_MASK) << VSYNC_END_SHIFT)
		| VSYNC_PULSE_ENABLED;
	config |= (mode->flags & DRM_MODE_FLAG_PVSYNC) ? 0 : VSYNC_NEGATIVE;
	phytium_writel_reg(priv, config, PHYTIUM_DC_VSYNC(phys_pipe));
	config = PANEL_DATAENABLE_ENABLE | PANEL_DATA_ENABLE | PANEL_CLOCK_ENABLE;
	phytium_writel_reg(priv, config, PHYTIUM_DC_PANEL_CONFIG(phys_pipe));
	config = phytium_crtc->bpc | OUTPUT_DP;
	phytium_writel_reg(priv, config, PHYTIUM_DC_DP_CONFIG(phys_pipe));

	config = phytium_readl_reg(priv, PHYTIUM_DC_FRAMEBUFFER_CONFIG(phys_pipe));

	if (crtc->state->active)
		config |= FRAMEBUFFER_OUTPUT | FRAMEBUFFER_RESET;
	else
		config &= (~(FRAMEBUFFER_OUTPUT | FRAMEBUFFER_RESET));

	if (phytium_crtc->scale_enable)
		config |= FRAMEBUFFER_SCALE_ENABLE;
	else
		config &= (~FRAMEBUFFER_SCALE_ENABLE);

	phytium_writel_reg(priv, config,
			   PHYTIUM_DC_FRAMEBUFFER_CONFIG(phys_pipe));

	drm_crtc_vblank_on(crtc);
}

static void
phytium_crtc_atomic_disable(struct drm_crtc *crtc,
				       struct drm_crtc_state *old_state)
{
	struct drm_device *dev = crtc->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	struct phytium_crtc *phytium_crtc = to_phytium_crtc(crtc);
	int reset_timeout = 100;
	int config = 0;
	int phys_pipe = phytium_crtc->phys_pipe;

	drm_crtc_vblank_off(crtc);

	config = phytium_readl_reg(priv, PHYTIUM_DC_CLOCK_CONTROL(phys_pipe));
	phytium_writel_reg(priv, config | SOFT_RESET, PHYTIUM_DC_CLOCK_CONTROL(phys_pipe));
	phytium_writel_reg(priv, 0, PHYTIUM_DC_CLOCK_CONTROL(phys_pipe));
	do {
		config = phytium_readl_reg(priv, PHYTIUM_DC_CLOCK_IDLE(phys_pipe));
		if (config | IS_IDLE)
			break;
		mdelay(1);
		reset_timeout--;
	} while (reset_timeout);

	/* reset pix clock */
	reset_timeout = 100;
	phytium_writel_reg(priv, (0x0 & PIX_CLOCK_MASK) | FLAG_REQUEST,
			   PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(phys_pipe));
	do {
		mdelay(10);
		reset_timeout--;
		config = phytium_readl_reg(priv, PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(phys_pipe));
	} while ((!(config & FLAG_REPLY)) && reset_timeout);
	if (reset_timeout == 0)
		DRM_ERROR("reset pix clock failed\n");
	phytium_writel_reg(priv, 0x0 & PIX_CLOCK_MASK,
			   PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(phys_pipe));
	mdelay(20);

	config = phytium_readl_reg(priv, PHYTIUM_DC_CLOCK_CONTROL(phys_pipe));
	phytium_writel_reg(priv, config | SOFT_RESET, PHYTIUM_DC_CLOCK_CONTROL(phys_pipe));
	phytium_writel_reg(priv, 0, PHYTIUM_DC_CLOCK_CONTROL(phys_pipe));
	do {
		config = phytium_readl_reg(priv, PHYTIUM_DC_CLOCK_IDLE(phys_pipe));
		if (config | IS_IDLE)
			break;
		mdelay(1);
		reset_timeout--;
	} while (reset_timeout);

	/* reset dcreq */
	phytium_writel_reg(priv, DCREQ_PLAN_A, PHYTIUM_DCREQ_PLAN(phys_pipe));
	phytium_writel_reg(priv, 0, PHYTIUM_DCREQ_CONTROL(phys_pipe));
	phytium_writel_reg(priv, DCREQ_RESET, PHYTIUM_DCREQ_RESET(phys_pipe));
	msleep(20);
	phytium_writel_reg(priv, (~DCREQ_RESET)&DCREQ_RESET_MASK,
			   PHYTIUM_DCREQ_RESET(phys_pipe));
}

static void phytium_crtc_update_timing_for_drm_display_mode(struct drm_display_mode *drm_mode,
						       const struct drm_display_mode *native_mode)
{
	if (native_mode->clock == drm_mode->clock &&
		native_mode->htotal == drm_mode->htotal &&
		native_mode->vtotal == drm_mode->vtotal) {
		drm_mode->crtc_hdisplay = native_mode->crtc_hdisplay;
		drm_mode->crtc_vdisplay = native_mode->crtc_vdisplay;
		drm_mode->crtc_clock = native_mode->crtc_clock;
		drm_mode->crtc_hblank_start = native_mode->crtc_hblank_start;
		drm_mode->crtc_hblank_end = native_mode->crtc_hblank_end;
		drm_mode->crtc_hsync_start =  native_mode->crtc_hsync_start;
		drm_mode->crtc_hsync_end = native_mode->crtc_hsync_end;
		drm_mode->crtc_htotal = native_mode->crtc_htotal;
		drm_mode->crtc_hskew = native_mode->crtc_hskew;
		drm_mode->crtc_vblank_start = native_mode->crtc_vblank_start;
		drm_mode->crtc_vblank_end = native_mode->crtc_vblank_end;
		drm_mode->crtc_vsync_start = native_mode->crtc_vsync_start;
		drm_mode->crtc_vsync_end = native_mode->crtc_vsync_end;
		drm_mode->crtc_vtotal = native_mode->crtc_vtotal;
	}
}

static int
phytium_crtc_atomic_check(struct drm_crtc *crtc, struct drm_crtc_state *crtc_state)
{
	struct drm_plane_state *new_plane_state = NULL;
	int ret = 0;
	struct drm_atomic_state *state = crtc_state->state;
	struct drm_connector *connector;
	struct drm_connector_state *new_con_state;
	uint32_t i;
	struct phytium_dp_device *phytium_dp = NULL;

	for_each_new_connector_in_state(state, connector, new_con_state, i) {
		if (new_con_state->crtc == crtc) {
			phytium_dp = connector_to_dp_device(connector);
			break;
		}
	}
	if (phytium_dp)
		phytium_crtc_update_timing_for_drm_display_mode(&crtc_state->adjusted_mode,
								&phytium_dp->native_mode);

	new_plane_state = drm_atomic_get_new_plane_state(crtc_state->state,
							crtc->primary);
	if (crtc_state->enable && new_plane_state && !new_plane_state->crtc) {
		ret = -EINVAL;
		goto fail;
	}

	return 0;
fail:
	return ret;
}

static void
phytium_crtc_atomic_begin(struct drm_crtc *crtc,
				    struct drm_crtc_state *old_crtc_state)
{
	struct drm_device *dev = crtc->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	struct phytium_crtc *phytium_crtc = to_phytium_crtc(crtc);
	int phys_pipe = phytium_crtc->phys_pipe;
	int config = 0;

	config = phytium_readl_reg(priv, PHYTIUM_DC_FRAMEBUFFER_CONFIG(phys_pipe));
	if (config & FRAMEBUFFER_RESET) {
		phytium_writel_reg(priv, config | FRAMEBUFFER_VALID_PENDING,
				   PHYTIUM_DC_FRAMEBUFFER_CONFIG(phys_pipe));
	}
}

static void phytium_crtc_atomic_flush(struct drm_crtc *crtc,
				struct drm_crtc_state *old_crtc_state)
{
	struct drm_device *dev = crtc->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	struct phytium_crtc *phytium_crtc = to_phytium_crtc(crtc);
	struct phytium_crtc_state *phytium_crtc_state = NULL;
	int phys_pipe = phytium_crtc->phys_pipe;
	int config;

	DRM_DEBUG_KMS("crtc->state active:%d enable:%d\n",
		       crtc->state->active, crtc->state->enable);
	phytium_crtc_state = to_phytium_crtc_state(crtc->state);
	config = phytium_readl_reg(priv, PHYTIUM_DC_FRAMEBUFFER_CONFIG(phys_pipe));

	phytium_writel_reg(priv, config&(~FRAMEBUFFER_VALID_PENDING),
			   PHYTIUM_DC_FRAMEBUFFER_CONFIG(phys_pipe));
	if (crtc->state->event) {
		DRM_DEBUG_KMS("vblank->refcount:%d\n",
			       atomic_read(&dev->vblank[0].refcount));
		spin_lock_irq(&dev->event_lock);
		if (drm_crtc_vblank_get(crtc) == 0)
			drm_crtc_arm_vblank_event(crtc, crtc->state->event);
		else
			drm_crtc_send_vblank_event(crtc, crtc->state->event);
		crtc->state->event = NULL;
		spin_unlock_irq(&dev->event_lock);
	}
}

static enum drm_mode_status
phytium_crtc_mode_valid(struct drm_crtc *crtc, const struct drm_display_mode *mode)
{
	if (mode->crtc_clock > PIX_CLOCK_MAX)
		return MODE_CLOCK_HIGH;

	if(mode->hdisplay > HDISPLAY_END_MAX)
		return MODE_BAD_HVALUE;

	if(mode->vdisplay > VDISPLAY_END_MAX)
		return MODE_BAD_VVALUE;

	if (mode->flags & DRM_MODE_FLAG_INTERLACE)
		return MODE_NO_INTERLACE;

	return MODE_OK;
}

static const struct drm_crtc_helper_funcs phytium_crtc_helper_funcs = {
	.mode_valid	= phytium_crtc_mode_valid,
	.atomic_check	= phytium_crtc_atomic_check,
	.atomic_begin	= phytium_crtc_atomic_begin,
	.atomic_flush	= phytium_crtc_atomic_flush,
	.atomic_enable	= phytium_crtc_atomic_enable,
	.atomic_disable	= phytium_crtc_atomic_disable,
};

int phytium_crtc_init(struct drm_device *dev, int phys_pipe)
{
	struct phytium_crtc *phytium_crtc;
	struct phytium_crtc_state *phytium_crtc_state;
	struct phytium_plane *phytium_primary_plane = NULL;
	struct phytium_plane *phytium_cursor_plane = NULL;
	int ret;

	phytium_crtc = kzalloc(sizeof(*phytium_crtc), GFP_KERNEL);
	if (IS_ERR(phytium_crtc)) {
		ret = -ENOMEM;
		goto failed_malloc_crtc;
	}

	phytium_crtc_state = kzalloc(sizeof(*phytium_crtc_state), GFP_KERNEL);
	if (IS_ERR(phytium_crtc_state)) {
		ret = -ENOMEM;
		goto failed_malloc_crtc_state;
	}

	phytium_crtc_state->base.crtc = &phytium_crtc->base;
	phytium_crtc->base.state = &phytium_crtc_state->base;
	phytium_crtc->phys_pipe = phys_pipe;

	phytium_primary_plane = phytium_primary_plane_create(dev, phys_pipe);
	if (IS_ERR(phytium_primary_plane)) {
		ret = PTR_ERR(phytium_primary_plane);
		DRM_ERROR("create primary plane failed, phys_pipe(%d)\n", phys_pipe);
		goto failed_create_primary;
	}

	phytium_cursor_plane = phytium_cursor_plane_create(dev, phys_pipe);
	if (IS_ERR(phytium_cursor_plane)) {
		ret = PTR_ERR(phytium_cursor_plane);
		DRM_ERROR("create cursor plane failed, phys_pipe(%d)\n", phys_pipe);
		goto failed_create_cursor;
	}

	ret = drm_crtc_init_with_planes(dev, &phytium_crtc->base,
					&phytium_primary_plane->base,
					&phytium_cursor_plane->base,
					&phytium_crtc_funcs,
					"phys_pipe %d", phys_pipe);

	if (ret) {
		DRM_ERROR("init crtc with plane failed, phys_pipe(%d)\n", phys_pipe);
		goto failed_crtc_init;
	}
	drm_crtc_helper_add(&phytium_crtc->base, &phytium_crtc_helper_funcs);
	drm_crtc_vblank_reset(&phytium_crtc->base);

	return 0;

failed_crtc_init:
failed_create_cursor:
	/* drm_mode_config_cleanup() will free any crtcs/planes already initialized */
failed_create_primary:
	kfree(phytium_crtc_state);
failed_malloc_crtc_state:
	kfree(phytium_crtc);
failed_malloc_crtc:
	return ret;
}
