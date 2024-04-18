/*
* SPDX-License-Identifier: GPL
*
* Copyright (c) 2020 ChangSha JingJiaMicro Electronics Co., Ltd.
* All rights reserved.
*
* Author:
*      shanjinkui <shanjinkui@jingjiamicro.com>
*
* The software and information contained herein is proprietary and
* confidential to JingJiaMicro Electronics. This software can only be
* used by JingJiaMicro Electronics Corporation. Any use, reproduction,
* or disclosure without the written permission of JingJiaMicro
* Electronics Corporation is strictly prohibited.
*/
#include "mwv207_vi.h"

#define MWV207_DAC_HPD_VS_CLC(_chan)       (0x64 + (_chan) * 0x14)
#define MWV207_DAC_HPD_PULSE_H(_chan)      (0x68 + (_chan) * 0x14)
#define MWV207_DAC_HPD_PULSE_L(_chan)      (0x6C + (_chan) * 0x14)
#define MWV207_DAC_HPD_DET_PER(_chan)      (0x70 + (_chan) * 0x14)
#define MWV207_DAC_HPD_DET_THR(_chan)      (0x74 + (_chan) * 0x14)

#define output_to_vga(output) container_of(output, struct mwv207_vga, base)

static uint mwv207_vga_load_detect = 1;
module_param(mwv207_vga_load_detect, uint, 0444);
MODULE_PARM_DESC(mwv207_vga_load_detect, "Enable vga load detect if set to 1, default is 1");

struct mwv207_vga {
	struct mwv207_output base;
	spinlock_t load_detect_lock;
	bool active;
};

static void mwv207_vga_set_detect_mode(struct mwv207_output *output, bool active)
{
	u32 val, status, load_mode;
	int i;

	mwv207_output_write(output, 0xA4, 0);

	val = 0x3ff * 7 / 10;
	status = mwv207_output_read(output, 0xA8);
	mwv207_output_write(output, 0xA8, status);

	status = val << 20 | val << 10 | val;
	mwv207_output_write(output, 0xAC, status);

	load_mode = active ? 1 : 0;
	status = load_mode << 20 | load_mode << 16 | load_mode << 12 | 0x1 << 8 | 0x1 << 4 | 0x1;
	mwv207_output_write(output, 0xAC, status);

	for (i = 0; i < 3; i++) {
		if (active) {
			mwv207_output_write(output, MWV207_DAC_HPD_VS_CLC(i),  256);
			mwv207_output_write(output, MWV207_DAC_HPD_PULSE_H(i), 48);
			mwv207_output_write(output, MWV207_DAC_HPD_DET_PER(i), 188);
			mwv207_output_write(output, MWV207_DAC_HPD_DET_THR(i), 164);
		} else {
			mwv207_output_write(output, MWV207_DAC_HPD_PULSE_H(i), 1600000);
			mwv207_output_write(output, MWV207_DAC_HPD_PULSE_L(i), 798400000);
			mwv207_output_write(output, MWV207_DAC_HPD_DET_PER(i), 1600128);
			mwv207_output_write(output, MWV207_DAC_HPD_DET_THR(i), 8128);
		}
	}

	mwv207_output_write(output, 0xA4, 0x111111);
}

static void mwv207_vga_switch(struct mwv207_output *output, bool on)
{
	struct mwv207_vga *vga = output_to_vga(output);

	if (!output->jdev->lite || !mwv207_vga_load_detect) {
		mwv207_output_modify(output, 0x0, 1 << 31, (on ? 1 : 0) << 31);
		return;
	}

	spin_lock(&vga->load_detect_lock);
	mwv207_output_modify(output, 0x0, 1 << 31, (on ? 1 : 0) << 31);
	mwv207_vga_set_detect_mode(output, on);
	vga->active = on;
	spin_unlock(&vga->load_detect_lock);
}

static void mwv207_vga_config(struct mwv207_output *output)
{
	struct drm_display_mode *mode = &output->cur_crtc->state->adjusted_mode;
	int hpol, vpol;

	hpol = (mode->flags & DRM_MODE_FLAG_PHSYNC) ? 0 : 1;
	vpol = (mode->flags & DRM_MODE_FLAG_PVSYNC) ? 0 : 1;

	mwv207_output_write(output, 0x4C, 0);
	mwv207_output_modify(output, 0x0, 0x7 << 16, 0x7 << 16);
	mwv207_output_modify(output, 0x0, 0x1 << 2, hpol << 2);
	mwv207_output_modify(output, 0x0, 0x1 << 3, vpol << 3);

	mwv207_output_write(output, 0x4, 0x1666);
	mwv207_output_write(output, 0x18, 0x1666);
	mwv207_output_write(output, 0x2C, 0x1666);
}

static void mwv207_vga_select_crtc(struct mwv207_output *output)
{

	mwv207_output_modify(output, 0x0, 0x3 << 24,
			drm_crtc_index(output->cur_crtc) << 24);

	jdev_modify(output->jdev, 0x9b0038, 0xf << 16,
			drm_crtc_index(output->cur_crtc) << 16);
}
static enum drm_mode_status mwv207_vga_mode_valid(struct mwv207_output *output,
		const struct drm_display_mode *mode)
{

	if (mode->clock > 193250)
		return MODE_CLOCK_HIGH;

	return MODE_OK;
}

static enum drm_mode_status mwv207_vga_connector_mode_valid(struct drm_connector *connector,
		struct drm_display_mode *mode)
{
	return mwv207_vga_mode_valid(connector_to_output(connector), mode);
}

static int mwv207_vga_detect_load(struct mwv207_output *output)
{
	struct mwv207_vga *vga = output_to_vga(output);
	u32 r_state, g_state, b_state, value;
	bool active;

	if (mwv207_i2c_probe(output->ddc))
		return connector_status_connected;

	spin_lock(&vga->load_detect_lock);
	value = mwv207_output_read(output, 0xA8);
	active = vga->active;
	spin_unlock(&vga->load_detect_lock);

	r_state = (value >> 8) & 0x1;
	g_state = (value >> 4) & 0x1;
	b_state = (value >> 0) & 0x1;
	DRM_DEBUG_DRIVER("active %d r_state = 0x%x, g_state = 0x%x, b_state = 0x%x",
			 active, r_state, g_state, b_state);

	if (active)
		return (r_state && g_state && b_state) ?
			connector_status_disconnected : connector_status_connected;

	return (r_state || g_state || b_state) ?
		connector_status_connected : connector_status_disconnected;
}

static int mwv207_vga_detect_ctx(struct drm_connector *connector,
			  struct drm_modeset_acquire_ctx *ctx,
			  bool force)
{
	struct mwv207_output *output = connector_to_output(connector);

	if (!output->jdev->lite || !mwv207_vga_load_detect)
		return mwv207_i2c_probe(output->ddc) ?
				connector_status_connected : connector_status_disconnected;

	return mwv207_vga_detect_load(output);
}

static void mwv207_vga_destroy(struct drm_connector *conn)
{
	drm_connector_unregister(conn);
	drm_connector_cleanup(conn);
}

static const struct drm_connector_helper_funcs mwv207_vga_connector_helper_funcs = {
	.get_modes  = mwv207_output_get_modes,
	.mode_valid = mwv207_vga_connector_mode_valid,
	.detect_ctx = mwv207_vga_detect_ctx
};

static const struct drm_connector_funcs mwv207_vga_connector_funcs = {
	.reset                  = drm_atomic_helper_connector_reset,
	.fill_modes             = drm_helper_probe_single_connector_modes,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state   = drm_atomic_helper_connector_destroy_state,
	.destroy                = mwv207_vga_destroy,
	.late_register          = mwv207_output_late_register,
	.early_unregister       = mwv207_output_early_unregister,
};

static enum drm_mode_status mwv207_vga_encoder_mode_valid(struct drm_encoder *encoder,
		const struct drm_display_mode *mode)
{
	struct mwv207_output *output = encoder_to_output(encoder);

	return mwv207_vga_mode_valid(output, mode);
}

static int mwv207_vga_encoder_atomic_check(struct drm_encoder *encoder,
		struct drm_crtc_state *crtc_state,
		struct drm_connector_state *conn_state)
{
	return 0;
}

static void mwv207_vga_encoder_enable(struct drm_encoder *encoder)
{
	struct mwv207_output *output = encoder_to_output(encoder);

	mwv207_vga_select_crtc(output);

	mwv207_vga_config(output);

	mwv207_vga_switch(output, true);
}

static void mwv207_vga_encoder_disable(struct drm_encoder *encoder)
{
	struct mwv207_output *output = encoder_to_output(encoder);

	mwv207_vga_switch(output, false);
}

static void mwv207_vga_encoder_reset(struct drm_encoder *encoder)
{
	struct mwv207_output *output = encoder_to_output(encoder);

	if (output->jdev->lite && !mwv207_vga_load_detect) {
		mwv207_output_write(output, 0xA4, 0);
		mwv207_output_modify(output, 0x58, 0x1 << 16, 0);
	}

	mwv207_vga_encoder_disable(encoder);
}

static const struct drm_encoder_funcs mwv207_vga_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
	.reset   = mwv207_vga_encoder_reset,
};

static const struct drm_encoder_helper_funcs mwv207_vga_encoder_helper_funcs = {
	.mode_valid      = mwv207_vga_encoder_mode_valid,
	.atomic_check    = mwv207_vga_encoder_atomic_check,
	.enable          = mwv207_vga_encoder_enable,
	.disable         = mwv207_vga_encoder_disable,
};

int mwv207_vga_init(struct mwv207_device *jdev)
{
	struct mwv207_output *output;
	struct mwv207_vga *vga;
	int ret;

	vga = devm_kzalloc(jdev->dev, sizeof(*vga), GFP_KERNEL);
	if (!vga)
		return -ENOMEM;

	spin_lock_init(&vga->load_detect_lock);
	output = &vga->base;
	output->jdev = jdev;
	output->idx  = 0;
	output->mmio = jdev->mmio + 0x9a0000;
	output->i2c_chan = 4;

	output->connector.polled = DRM_CONNECTOR_POLL_CONNECT | DRM_CONNECTOR_POLL_DISCONNECT;
	ret = drm_connector_init(&jdev->base, &output->connector,
			&mwv207_vga_connector_funcs, DRM_MODE_CONNECTOR_VGA);
	if (ret)
		return ret;
	drm_connector_helper_add(&output->connector, &mwv207_vga_connector_helper_funcs);

	output->encoder.possible_crtcs = (1 << jdev->base.mode_config.num_crtc) - 1;
	ret = drm_encoder_init(&jdev->base, &output->encoder,
			&mwv207_vga_encoder_funcs, DRM_MODE_ENCODER_DAC,
			"vga-%d", output->idx);
	if (ret)
		return ret;
	drm_encoder_helper_add(&output->encoder, &mwv207_vga_encoder_helper_funcs);

	ret = drm_connector_attach_encoder(&output->connector, &output->encoder);
	if (ret)
		return ret;

	return drm_connector_register(&output->connector);
}
