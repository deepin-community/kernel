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

int mwv207_output_late_register(struct drm_connector *connector)
{
	struct mwv207_output *output = connector_to_output(connector);

	output->ddc = mwv207_i2c_create(output->jdev, output->i2c_chan);
	if (!output->ddc) {
		DRM_ERROR("Failed to create i2c adapter\n");
		return -ENODEV;
	}

	return 0;
}

void mwv207_output_early_unregister(struct drm_connector *connector)
{
	struct mwv207_output *output = connector_to_output(connector);

	mwv207_i2c_destroy(output->ddc);
}

int mwv207_output_get_modes(struct drm_connector *connector)
{
	struct mwv207_output *output = connector_to_output(connector);
	struct edid *edid;
	int count;

	if (output->edidforce && output->edid)
		edid = output->edid;
	else
		edid = drm_get_edid(connector, output->ddc);

	output->has_audio = drm_detect_monitor_audio(edid);
	drm_connector_update_edid_property(connector, edid);
	count = drm_add_edid_modes(connector, edid);

	if (!(output->edidforce && output->edid))
		kfree(edid);

	return count;
}

void mwv207_output_set_crtc(struct drm_encoder *encoder, struct drm_crtc *crtc)
{
	struct mwv207_output *output = encoder_to_output(encoder);

	output->cur_crtc = crtc;
}

int mwv207_vi_init(struct mwv207_device *jdev)
{
	int ret;

	mutex_init(&jdev->gpio_lock);

	ret = mwv207_edp_init(jdev);
	if (ret)
		return ret;
	ret = mwv207_hdmi_init(jdev);
	if (ret)
		return ret;
	ret = mwv207_vga_init(jdev);
	if (ret)
		return ret;
	ret = mwv207_dvo_init(jdev);
	if (ret)
		return ret;

	return 0;
}
