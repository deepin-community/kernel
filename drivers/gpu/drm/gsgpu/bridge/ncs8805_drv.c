// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2021 Loongson Technology Co., Ltd.
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/types.h>
#include <drm/drm_encoder.h>
#include <drm/drm_atomic_helper.h>
#include <linux/i2c.h>
#include <linux/pwm.h>
#include <linux/delay.h>
#include <drm/drm_edid.h>
#include "bridge_phy.h"
#include "ncs8805_drv.h"
#include "gsgpu_backlight.h"
#include "gsgpu_dc_vbios.h"
#include "gsgpu_dc_i2c.h"
#include "gsgpu_dc.h"
#include "gsgpu.h"

static bool legacy_handle_flags = true;
static struct ncs_resources  *ncs_res;

static struct display_mode  mode_current = {
	.mode.htotal = 2200,
	.mode.vtotal = 1125,
	.mode.hdisplay = 1920,
	.mode.vdisplay = 1080,
};

/*
 * Function as i2c_master_send.
 * Returns negative errno, else the number of messages executed.
 */
static bool ncs8805_read_bytes(struct gsgpu_bridge_phy *phy, uint8_t addr,
		uint8_t offset, uint8_t *buf)
{
	struct gsgpu_dc_i2c *i2c = phy->li2c;
	uint8_t ret, w_buf, r_buf;
	struct i2c_msg msgs[] = {
		{
			.flags = 0,
			.addr = addr,
			.len = 1,
			.buf = &w_buf,
		},
		{
			.flags = I2C_M_RD,
			.addr = addr,
			.len = 1,
			.buf = &r_buf,
		}
	};

	r_buf = 0;
	w_buf = offset;

	ret = i2c_transfer(&i2c->adapter, msgs, 2);
	if (ret != 2) {
		DRM_DEBUG("[%s] i2c-0x%02x read reg 0x%02x failed\n",
				PHY_NAME, addr, offset);
		return false;
	}
	*buf = r_buf;
	return true;
}

/*Function as i2c_master_send */
static bool ncs8805_write_bytes(struct gsgpu_bridge_phy *phy, uint8_t addr,
		uint8_t offset, uint8_t val)
{
	struct gsgpu_dc_i2c *i2c = phy->li2c;
	uint8_t  ret, w_buf[2];
	struct i2c_msg msg = {
		.flags = 0,
		.addr = addr,
		.len = 2,
		.buf = w_buf,
	};

	w_buf[0] = offset;
	w_buf[1] = val;

	ret = i2c_transfer(&i2c->adapter, &msg, 1);
	if (ret != 1) {
		DRM_DEBUG("[%s] i2c-0x%02x write reg 0x%02x failed\n",
				PHY_NAME, addr, offset);
		return false;
	}
	return true;
}

static void ncs8805_rx_timing_get(struct gsgpu_bridge_phy *phy,
		struct rx_timing *timing)
{
	uint8_t addr = NCS8805_SLAVE1_ADDR; /* 7 bit addr*/
	uint8_t reg_low, reg_upper;

	/* hdisplay */
	ncs8805_read_bytes(phy, addr, 0xec, &reg_low);
	/* 0:Hsync active high;  1:Hsync active low*/
	timing->hpolarity = reg_low >> 7;

	ncs8805_read_bytes(phy, addr, 0xe0, &reg_upper);
	ncs8805_read_bytes(phy, addr, 0xe1, &reg_low);
	timing->htotal = (reg_upper<<8) | reg_low;

	ncs8805_read_bytes(phy, addr, 0xe4, &reg_upper);
	ncs8805_read_bytes(phy, addr, 0xe5, &reg_low);
	timing->hdisplay = (reg_upper<<8) | reg_low;

	ncs8805_read_bytes(phy, addr, 0xe2, &reg_upper);
	ncs8805_read_bytes(phy, addr, 0xe3, &reg_low);
	timing->hsync_start = timing->htotal - ((reg_upper<<8)|reg_low);

	ncs8805_read_bytes(phy, addr, 0xed, &reg_low);
	timing->hsync_end = timing->hsync_start + reg_low;

	/* vdisplay */
	ncs8805_read_bytes(phy, addr, 0xee, &reg_low);
	/* 0:Hsync active high;  1:Hsync active low*/
	timing->vpolarity = reg_low >> 7;

	ncs8805_read_bytes(phy, addr, 0xe6, &reg_upper);
	ncs8805_read_bytes(phy, addr, 0xe7, &reg_low);
	timing->vtotal = (reg_upper<<8) | reg_low;

	ncs8805_read_bytes(phy, addr, 0xea, &reg_upper);
	ncs8805_read_bytes(phy, addr, 0xeb, &reg_low);
	timing->vdisplay = (reg_upper<<8) | reg_low;

	ncs8805_read_bytes(phy, addr, 0xe8, &reg_upper);
	ncs8805_read_bytes(phy, addr, 0xe9, &reg_low);
	timing->vsync_start = timing->vtotal - ((reg_upper<<8)|reg_low);

	ncs8805_read_bytes(phy, addr, 0xef, &reg_low);
	timing->vsync_end = timing->vsync_start + reg_low;

	DRM_DEBUG("[%s] htotal:%d, hdisplay:%d, hsync_start:%d, hsync_end:%d.\n",
			PHY_NAME, timing->htotal, timing->hdisplay,
			timing->hsync_start, timing->hsync_end);
	DRM_DEBUG("[%s] vtotal:%d, vdisplay:%d, vsync_start:%d, vsync_end:%d.\n",
			PHY_NAME, timing->vtotal, timing->vdisplay,
			timing->vsync_start, timing->vsync_end);
}

static int ncs8805_rx_timing_wait(struct gsgpu_bridge_phy *phy)
{
	struct rx_timing timing;
	uint8_t ret, times_active = 10;

	do {
		if (!(--times_active)) {
			DRM_ERROR("[%s] rx timing failed. timeout!.\n", PHY_NAME);
			break;
		}
		msleep(30);
		ncs8805_rx_timing_get(phy, &timing);
		DRM_DEBUG("[%s] rx timing, times_active=%d, %dx%d.\n", PHY_NAME,
				times_active, timing.hdisplay, timing.vdisplay);
		ret = (abs(timing.htotal - mode_current.mode.htotal) <= 2)
			&& (abs(timing.hdisplay - mode_current.mode.hdisplay) <= 2)
			&& (abs(timing.hsync_start - mode_current.mode.hsync_start) <= 2)
			&& (abs(timing.hsync_end - mode_current.mode.hsync_end) <= 2)
			&& (abs(timing.vtotal - mode_current.mode.vtotal) <= 2)
			&& (abs(timing.vdisplay - mode_current.mode.vdisplay) <= 2)
			&& (abs(timing.vsync_start - mode_current.mode.vsync_start) <= 2)
			&& (abs(timing.vsync_end - mode_current.mode.vsync_end) <= 2);
	} while (!ret);

	if (ret)
		DRM_DEBUG("[%s] rx timing complete.\n", PHY_NAME);

	return ret;
}

static int ncs8805_get_resolution_index(uint16_t hdisplay, uint16_t vdisplay)
{
	int index = -1;

	for (index = 0; index < ncs_res->resolution_number; index++) {
		if (ncs_res->resolution_list[index].hdisplay == hdisplay
				&& ncs_res->resolution_list[index].vdisplay == vdisplay)
			return index;
	}

	if (index == -1)
		DRM_DEBUG("[%s] detect auto_mvid_list failed. not find %dx%d.\n",
				PHY_NAME, hdisplay, vdisplay);

	return index;
}

static int ncs8805_auto_mvid_detect(struct gsgpu_bridge_phy *phy)
{
	int index, ret = 0;
	uint8_t reg_hig, reg_mid, reg_low;
	struct ncs_resolution_cfg *cfg;
	uint8_t times_active = 10;

	index = ncs8805_get_resolution_index(mode_current.mode.hdisplay,
			mode_current.mode.vdisplay);
	if (index == -1)
		return ret;
	cfg = &ncs_res->resolution_list[index];

	do {
		if (!(--times_active)) {
			DRM_ERROR("[%s] mvid detect failed. timeout!.\n", PHY_NAME);
			break;
		}
		msleep(30);
		reg_hig = reg_mid = reg_low = 0;
		ncs8805_read_bytes(phy, NCS8805_SLAVE1_ADDR,
				NCS8805_REG_NVID_HIG_ADDR, &reg_hig);
		ncs8805_read_bytes(phy, NCS8805_SLAVE1_ADDR,
				NCS8805_REG_NVID_MID_ADDR, &reg_mid);
		ncs8805_read_bytes(phy, NCS8805_SLAVE1_ADDR,
				NCS8805_REG_NVID_LOW_ADDR, &reg_low);
		if ((reg_hig <= cfg->lcd_p.auto_mvid_hig_max
					&& reg_hig >= cfg->lcd_p.auto_mvid_hig_min)
				&& (reg_mid <= cfg->lcd_p.auto_mvid_mid_max
					&& reg_mid >= cfg->lcd_p.auto_mvid_mid_min)
				&& (reg_low <= cfg->lcd_p.auto_mvid_low_max
					&& reg_low >= cfg->lcd_p.auto_mvid_low_min)) {
			ret = 1;
		}
		DRM_DEBUG("[%s] auto mvid detect, times_active=%d, reg=0x%x,0x%x,0x%x.\n",
				PHY_NAME, times_active, reg_hig, reg_mid, reg_low);
	} while (!ret);
	if (ret)
		DRM_DEBUG("[%s] auto mvid detect complete.\n", PHY_NAME);

	return ret;
}

static int ncs8805_dp_lock_wait(struct gsgpu_bridge_phy *phy)
{
	int ret = 0;
	uint8_t reg_f, reg_g;
	uint8_t times_active = 10;

	do {
		if (!(--times_active)) {
			DRM_ERROR("[%s] dp lock failed. timeout!.\n", PHY_NAME);
			break;
		}
		msleep(30);
		ncs8805_read_bytes(phy, NCS8805_SLAVE1_ADDR, 0x81, &reg_f);
		ncs8805_read_bytes(phy, NCS8805_SLAVE1_ADDR, 0x83, &reg_g);
		if (reg_f == 0x77 && reg_g == 0x81)
			ret = 1;
		DRM_DEBUG("[%s] dp lock, times_active=%d, reg=0x%x,0x%x.\n",
				PHY_NAME, times_active, reg_f, reg_g);
	} while (!ret);
	if (ret)
		DRM_DEBUG("[%s] dp lock complete.\n", PHY_NAME);

	return ret;
}

static int ncs8805_tx_timing_wait(struct gsgpu_bridge_phy *phy)
{
	uint8_t addr, reg, ret;
	uint8_t times_active = 10;

	addr = NCS8805_SLAVE1_ADDR;
	do {
		if (!(--times_active)) {
			DRM_ERROR("[%s] tx timing failed. timeout!.\n", PHY_NAME);
			break;
		}
		msleep(30);
		ncs8805_read_bytes(phy, addr, 0x81, &reg);
		ret = (reg != 0x7) && (reg != 0x77);
		DRM_DEBUG("[%s] tx timing, times_active=%d, reg=0x%x.\n",
				PHY_NAME, times_active, reg);
	} while (ret);

	if (!ret)
		DRM_DEBUG("[%s] tx timing complete.\n", PHY_NAME);

	return !ret;
}

static void ncs8805_working_ctrl(struct gsgpu_bridge_phy *phy, uint8_t enable)
{
	uint8_t addr, reg;
	bool ret = false;
	struct gsgpu_backlight *ls_bl = NULL;

	ls_bl = phy->adev->mode_info.backlights[1];
	addr = NCS8805_SLAVE1_ADDR;

	if (!phy)
		return;

	if (!ncs8805_read_bytes(phy, addr, 0x71, &reg))
		return;

	if (enable == NCS8805_WORKING_DISABLE) {
		/* 1:enable  idle */
		ret = ncs8805_write_bytes(phy, addr, 0x71, reg|(0x1<<1));
	} else {
		if (!ls_bl || !ls_bl->hw_enabled) {
			DRM_INFO("[%s] backlight off or not register,so working mode not set.\n",
					PHY_NAME);
			return;
		} else {
			if (!legacy_handle_flags) {
				if (ncs8805_tx_timing_wait(phy)
						&& ncs8805_rx_timing_wait(phy)
						&& ncs8805_dp_lock_wait(phy)
						&& ncs8805_auto_mvid_detect(phy))
					/* 0:disable idle */
					ret = ncs8805_write_bytes(phy, addr, 0x71, reg & (~(0x1<<1)));
			} else {
				ret = ncs8805_write_bytes(phy, addr, 0x71, reg & (~(0x1<<1)));
			}
		}
	}
	if (ret) {
		DRM_INFO("[%s] Enter %s mode.\n", PHY_NAME,
				enable == NCS8805_WORKING_DISABLE ? "idle" : "working");
	} else {
		DRM_ERROR("[%s] Enter %s mode failed.\n", PHY_NAME,
				enable == NCS8805_WORKING_DISABLE ? "idle" : "working");
	}
}

static bool ncs8805_mode_config(struct gsgpu_bridge_phy *phy,
		struct display_mode *d_mode)
{
	int index, i;
	struct ncs_resolution_cfg *cfg;

	DRM_DEBUG("[%s] mode config %dx%d.\n", PHY_NAME,
			d_mode->mode.hdisplay, d_mode->mode.vdisplay);

	index = ncs8805_get_resolution_index(d_mode->mode.hdisplay,
			d_mode->mode.vdisplay);
	if (index == -1)
		return false;
	cfg = &ncs_res->resolution_list[index];

	ncs8805_working_ctrl(phy, NCS8805_WORKING_DISABLE);
	for (i = 0; i < cfg->reg_number; i++) {
		DRM_DEBUG("[%s] write reg:<%02d> addr=0x%02x, reg=0x%02x, val=0x%02x.\n",
				PHY_NAME, i, cfg->reg_list[i].addr,
				cfg->reg_list[i].offset, cfg->reg_list[i].val);
		if (!ncs8805_write_bytes(phy, cfg->reg_list[i].addr,
					cfg->reg_list[i].offset, cfg->reg_list[i].val)) {
			DRM_ERROR("[%s] write reg failed: addr=0x%x, reg=0x%x, val=0x%x.\n",
					PHY_NAME, cfg->reg_list[i].addr,
					cfg->reg_list[i].offset, cfg->reg_list[i].val);
		}
	}

	DRM_DEBUG("[%s] %dx%d: ncs8805 config reg num:%d.\n",
			PHY_NAME, d_mode->mode.hdisplay, d_mode->mode.vdisplay, i);
	return true;
}

static int ncs8805_bl_ctrl(struct gsgpu_bridge_phy *phy, int mode)
{
	struct gsgpu_backlight *ls_bl = NULL;
	ls_bl =  phy->adev->mode_info.backlights[1];

	if (!ls_bl) {
		DRM_ERROR("[%s] loongson backlight not register or not present.\n", PHY_NAME);
		return 0;
	}

	if (mode == DRM_MODE_DPMS_ON) {
		/*fix display abnormal when automatic switching screen.*/
		if (!legacy_handle_flags)
			ncs8805_mode_config(phy, &mode_current);
		/* Edp power on */
		BACKLIGHT_DEFAULT_METHOD_FORCE_OPEN(ls_bl);
		DRM_DEBUG("[%s] backlight power on.\n", PHY_NAME);
		/* Exit idle mode */
		ncs8805_working_ctrl(phy, NCS8805_WORKING_ENABLE);
	} else {
		/* Enter idle mode */
		ncs8805_working_ctrl(phy, NCS8805_WORKING_DISABLE);
		/* Edp power dowm */
		BACKLIGHT_DEFAULT_METHOD_FORCE_CLOSE(ls_bl);
		DRM_DEBUG("[%s] backlight power off.\n", PHY_NAME);
	}
	return 0;
}


void ncs8805_prepare(struct gsgpu_bridge_phy *phy)
{
	ncs8805_working_ctrl(phy, NCS8805_WORKING_DISABLE);
}

static int ncs8805_mode_set(struct gsgpu_bridge_phy *phy,
		const struct drm_display_mode *mode,
		const struct drm_display_mode *adj_mode)
{
	if (!phy)
		return 1;

	if (!memcmp(mode, &mode_current.mode, sizeof(struct drm_display_mode)))
		return 0;

	memcpy(&mode_current.mode, mode, sizeof(struct drm_display_mode));
	if (!legacy_handle_flags)
		ncs8805_mode_config(phy, &mode_current);

	return 0;
}

void ncs8805_commit(struct gsgpu_bridge_phy *phy)
{
	ncs8805_working_ctrl(phy, NCS8805_WORKING_ENABLE);
}

static struct bridge_phy_cfg_funcs ncs8805_cfg_funcs = {
	.backlight_ctrl = ncs8805_bl_ctrl,
	.prepare = ncs8805_prepare,
	.commit = ncs8805_commit,
	.mode_set = ncs8805_mode_set,
};

static int ncs8805_get_modes(struct gsgpu_bridge_phy *phy,
			     struct drm_connector *connector)
{
	struct edid *edid;
	unsigned int count = 0;
	int size = sizeof(u8) * EDID_LENGTH * 2;

	edid = kmalloc(size, GFP_KERNEL);
	if (edid) {
		memcpy(edid, ncs_res->edid, size);
		drm_connector_update_edid_property(connector, edid);
		count = drm_add_edid_modes(connector, edid);
		kfree(edid);
	} else {
		DRM_ERROR("[%s] resources is invalid.\n", PHY_NAME);
	}
	return count;
}

static struct bridge_phy_ddc_funcs ncs8805_ddc_funcs = {
	.get_modes = ncs8805_get_modes,
};

static struct bridge_phy_helper ncs8805_helper_funcs = {
	.ddc_funcs = &ncs8805_ddc_funcs,
};

static bool ncs8805_resources_valid_check(struct ext_encoder_resources *encoder_res)
{
	if (gsgpu_vbios_checksum(encoder_res->data, encoder_res->data_size)
			== encoder_res->data_checksum)
		return true;
	return false;
}

int bridge_phy_ncs8805_init(struct gsgpu_dc_bridge *dc_bridge)
{
	struct gsgpu_bridge_phy *ncs8805_phy;
	struct ext_encoder_resources  *ext_resource;
	u32 feature;
	int ret = -1;

	if (gsgpu_vbios_version(dc_bridge->dc->vbios) >= VBIOS_VERSION_V1_1) {
		legacy_handle_flags = false;
		ext_resource = dc_get_vbios_resource(dc_bridge->dc->vbios, 1, GSGPU_RESOURCE_EXT_ENCODER);
		if (ncs8805_resources_valid_check(ext_resource)) {
			ncs_res = (struct ncs_resources *)ext_resource->data;
			DRM_INFO("[%s] resources version: %d.%d.%d\n", PHY_NAME,
					(ncs_res->version >> 16) & 0xFF,
					(ncs_res->version >> 8) & 0xFF,
					(ncs_res->version >> 0) & 0xFF);
		} else {
			DRM_ERROR("[%s] resources is invalid.\n", PHY_NAME);
			return ret;
		}
	}

	feature = SUPPORT_DDC | SUPPORT_HPD;
	ncs8805_phy = bridge_phy_alloc(dc_bridge);
	ret = bridge_phy_register(ncs8805_phy, &ncs8805_cfg_funcs, feature,
			&ncs8805_helper_funcs);

	return ret;
}

int bridge_phy_ncs8805_remove(struct gsgpu_dc_bridge *phy)
{
	return 0;
}
