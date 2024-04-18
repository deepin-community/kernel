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

static void mwv207_dvo_switch(struct mwv207_output *output, bool on)
{
	mwv207_output_modify(output, 0x200,
			0x1 << 0, (on ? 1 : 0) << 0);
}

static void mwv207_dvo_config(struct mwv207_output *output)
{
	struct drm_display_mode *mode = &output->cur_crtc->state->adjusted_mode;
	int hpol, vpol;

	hpol = (mode->flags & DRM_MODE_FLAG_PHSYNC) ? 0 : 1;
	vpol = (mode->flags & DRM_MODE_FLAG_PVSYNC) ? 0 : 1;

	mwv207_output_modify(output, 0x200, 0x1 << 9, hpol << 9);
	mwv207_output_modify(output, 0x200, 0x1 << 8, vpol << 8);

}

static void mwv207_dvo_select_crtc(struct mwv207_output *output)
{

	mwv207_output_modify(output, 0x200, 0x7 << 16,
			drm_crtc_index(output->cur_crtc) << 16);

	jdev_modify(output->jdev, 0x9b003c, 0xf,
			drm_crtc_index(output->cur_crtc));
}
static enum drm_mode_status mwv207_dvo_mode_valid(struct mwv207_output *output,
		const struct drm_display_mode *mode)
{

	if (mode->clock > 162000)
		return MODE_CLOCK_HIGH;

	return MODE_OK;
}

static enum drm_mode_status mwv207_dvo_connector_mode_valid(struct drm_connector *connector,
		struct drm_display_mode *mode)
{
	return mwv207_dvo_mode_valid(connector_to_output(connector), mode);
}

static int mwv207_dvo_detect_ctx(struct drm_connector *connector,
			  struct drm_modeset_acquire_ctx *ctx,
			  bool force)
{
	struct mwv207_output *output = connector_to_output(connector);

	if (mwv207_i2c_probe(output->ddc))
		return connector_status_connected;
	else
		return connector_status_disconnected;
}

static void mwv207_dvo_destroy(struct drm_connector *conn)
{
	drm_connector_unregister(conn);
	drm_connector_cleanup(conn);
}

static const struct drm_connector_helper_funcs mwv207_dvo_connector_helper_funcs = {
	.get_modes  = mwv207_output_get_modes,
	.mode_valid = mwv207_dvo_connector_mode_valid,
	.detect_ctx = mwv207_dvo_detect_ctx
};

static const struct drm_connector_funcs mwv207_dvo_connector_funcs = {
	.reset                  = drm_atomic_helper_connector_reset,
	.fill_modes             = drm_helper_probe_single_connector_modes,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state   = drm_atomic_helper_connector_destroy_state,
	.destroy                = mwv207_dvo_destroy,
	.late_register          = mwv207_output_late_register,
	.early_unregister       = mwv207_output_early_unregister,
};

static enum drm_mode_status mwv207_dvo_encoder_mode_valid(struct drm_encoder *encoder,
		const struct drm_display_mode *mode)
{
	struct mwv207_output *output = encoder_to_output(encoder);

	return mwv207_dvo_mode_valid(output, mode);
}

static int mwv207_dvo_encoder_atomic_check(struct drm_encoder *encoder,
		struct drm_crtc_state *crtc_state,
		struct drm_connector_state *conn_state)
{
	return 0;
}

static void mwv207_dvo_encoder_enable(struct drm_encoder *encoder)
{
	struct mwv207_output *output = encoder_to_output(encoder);

	mwv207_dvo_select_crtc(output);

	mwv207_dvo_config(output);

	mwv207_dvo_switch(output, true);
}

static void mwv207_dvo_encoder_disable(struct drm_encoder *encoder)
{
	struct mwv207_output *output = encoder_to_output(encoder);

	mwv207_dvo_switch(output, false);
}

static void mwv207_dvo_encoder_reset(struct drm_encoder *encoder)
{
	mwv207_dvo_encoder_disable(encoder);
}

static const struct drm_encoder_funcs mwv207_dvo_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
	.reset   = mwv207_dvo_encoder_reset,
};

static const struct drm_encoder_helper_funcs mwv207_dvo_encoder_helper_funcs = {
	.mode_valid      = mwv207_dvo_encoder_mode_valid,
	.atomic_check    = mwv207_dvo_encoder_atomic_check,
	.enable          = mwv207_dvo_encoder_enable,
	.disable         = mwv207_dvo_encoder_disable,
};

int mwv207_dvo_init(struct mwv207_device *jdev)
{
	struct mwv207_output *output;
	int ret;

	output = devm_kzalloc(jdev->dev, sizeof(*output), GFP_KERNEL);
	if (!output)
		return -ENOMEM;

	output->jdev = jdev;
	output->idx  = 0;
	output->mmio = jdev->mmio + 0x9a0000;
	output->i2c_chan = 5;

	output->connector.polled = DRM_CONNECTOR_POLL_CONNECT | DRM_CONNECTOR_POLL_DISCONNECT;
	ret = drm_connector_init(&jdev->base, &output->connector,
			&mwv207_dvo_connector_funcs, DRM_MODE_CONNECTOR_DVII);
	if (ret)
		return ret;
	drm_connector_helper_add(&output->connector, &mwv207_dvo_connector_helper_funcs);

	output->encoder.possible_crtcs = (1 << jdev->base.mode_config.num_crtc) - 1;
	ret = drm_encoder_init(&jdev->base, &output->encoder,
			&mwv207_dvo_encoder_funcs, DRM_MODE_ENCODER_DAC,
			"dvo-%d", output->idx);
	if (ret)
		return ret;
	drm_encoder_helper_add(&output->encoder, &mwv207_dvo_encoder_helper_funcs);

	ret = drm_connector_attach_encoder(&output->connector, &output->encoder);
	if (ret)
		return ret;

	return drm_connector_register(&output->connector);
}
