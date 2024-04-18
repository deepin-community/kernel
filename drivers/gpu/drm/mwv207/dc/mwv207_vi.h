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
#include "mwv207.h"
#include <drm/drm_edid.h>
#include <drm/drm_atomic.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_atomic_helper.h>
#include <linux/i2c.h>

#ifndef MWV207_DAL_H_OUVHEJAX
#define MWV207_DAL_H_OUVHEJAX

struct i2c_adapter;

struct mwv207_output {
	struct drm_connector connector;
	struct drm_encoder   encoder;

	struct drm_crtc      *cur_crtc;

	/* mapped to video interface global control regs
	 * which are shared among all outputs
	 */
	void __iomem         *mmio;
	struct mwv207_device *jdev;

	struct i2c_adapter   *ddc;
	struct edid          *edid;
	bool                 edidforce;
	int                  i2c_chan;

	/* local index, e.g. hdmi0/1/2/3, edp0/1 */
	int                  idx;

	bool                 has_audio;
};
#define connector_to_output(conn) container_of(conn, struct mwv207_output, connector)
#define encoder_to_output(encoder) container_of(encoder, struct mwv207_output, encoder)

/* Note: any resources allocated by following funcs should be:
 * 1. devres managed or
 * 2. cleaned up by drm_mode_config_cleanup
 */
int mwv207_vi_init(struct mwv207_device *jdev);
int mwv207_edp_init(struct mwv207_device *jdev);
int mwv207_hdmi_init(struct mwv207_device *jdev);
int mwv207_vga_init(struct mwv207_device *jdev);
int mwv207_dvo_init(struct mwv207_device *jdev);

struct i2c_adapter *mwv207_i2c_create(struct mwv207_device *jdev, int i2c_chan);
void mwv207_i2c_destroy(struct i2c_adapter *adapter);
bool mwv207_i2c_probe(struct i2c_adapter *i2c_bus);

int mwv207_output_late_register(struct drm_connector *connector);
void mwv207_output_early_unregister(struct drm_connector *connector);

int mwv207_output_get_modes(struct drm_connector *connector);
void mwv207_output_set_crtc(struct drm_encoder *encoder, struct drm_crtc *crtc);

static inline u32 mwv207_output_read(struct mwv207_output *output, u32 reg)
{
	BUG_ON(reg >= 0x8000);
	return readl(output->mmio + reg);
}

static inline void mwv207_output_write(struct mwv207_output *output, u32 reg, u32 value)
{
	BUG_ON(reg >= 0x8000);
	writel_relaxed(value, output->mmio + reg);
}

static inline void mwv207_output_modify(struct mwv207_output *output, u32 reg,
		u32 mask, u32 value)
{
	u32 rvalue;

	BUG_ON(reg >= 0x8000);
	rvalue = mwv207_output_read(output, reg);
	rvalue = (rvalue & ~mask) | (value & mask);
	mwv207_output_write(output, reg, rvalue);
}

#endif
