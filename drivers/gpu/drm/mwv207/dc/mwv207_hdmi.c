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
#include <drm/display/drm_scdc_helper.h>
#include <drm/display/drm_hdmi_helper.h>
#include <linux/delay.h>
#include "mwv207_vi.h"
#include "mwv207_vbios.h"

/* hdmi specific, accessed by mwv207_hdmi_read/write */
#define MWV207_HDMI_BASE(idx)   (0x1200000 + 0x40000 * (idx))

/* vi global, accessed by mwv207_output_read/write */
#define MWV207_HDMI_CTRL(idx)   (0x400 + 0x100 * (idx))
#define MWV207_HDMI_PHY(idx)    (0x6000 + 0x800 * (idx))

#define connector_to_hdmi(conn) container_of(conn, struct mwv207_hdmi, base.connector)
#define encoder_to_hdmi(encoder) container_of(encoder, struct mwv207_hdmi, base.encoder)

struct mwv207_hdmi_phy_data {
	u32 min_freq_khz;
	u32 max_freq_khz;
	u32 bpp;
	u8  config[47];
	u8  padding;
} __packed;

static const struct mwv207_hdmi_phy_data
default_phy_data[19] = {
	 { 24000, 47999, 8,
	  { 0xD2, 0x78, 0xB0, 0x01, 0x00, 0x88, 0x02, 0x4F, 0x30, 0x33, 0x65,
	   0x00, 0x01, 0x25, 0x80, 0x6C, 0xF2, 0x67, 0x00, 0x10, 0x8B, 0xF0,
	   0x22, 0x82, 0x8F, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x83, 0x0F, 0x3E,
	   0xF8, 0x00, 0x00}},
	 { 28000, 95999, 8,
	  { 0xD2, 0x3C, 0x50, 0x01, 0x00, 0x88, 0x02, 0x4F, 0x30, 0x33, 0x65,
	   0x00, 0x01, 0x25, 0x80, 0x6C, 0xF2, 0x67, 0x00, 0x10, 0x85, 0xF0,
	   0x22, 0x82, 0x8F, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x83, 0x0F, 0x3E,
	   0xF8, 0x00, 0x00}},
	 { 96000, 143999, 8,
	  { 0xD4, 0x50, 0x30, 0x01, 0x00, 0x88, 0x02, 0x4F, 0x30, 0x33, 0x65,
	   0x00, 0x01, 0x25, 0x80, 0x6C, 0xF2, 0x67, 0x00, 0x10, 0x83, 0xF0,
	   0x22, 0x82, 0x8F, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x83, 0x0F, 0x3E,
	   0xF8, 0x00, 0x00}},
	 { 144000, 296999, 8,
	  { 0xD8, 0x50, 0x10, 0x01, 0x00, 0x88, 0x02, 0x4F, 0x30, 0x33, 0x65,
	   0x00, 0x01, 0x25, 0x80, 0x6C, 0xF2, 0x67, 0x00, 0x10, 0x81, 0xF0,
	   0x22, 0x82, 0x8F, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x83, 0x0F, 0x3E,
	   0xF8, 0x00, 0x00}},
	 { 297000, 340000, 8,
	  { 0xDC, 0x3C, 0x00, 0x01, 0x00, 0x88, 0x02, 0x4F, 0x30, 0x33, 0x65,
	   0x00, 0x01, 0x25, 0x80, 0x6C, 0xF2, 0x67, 0x00, 0x10, 0x80, 0xF0,
	   0x22, 0x82, 0x8F, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x83, 0x0F, 0x3E,
	   0xF8, 0x00, 0x00}},
	 { 340001, 595000, 8,
	  { 0xDC, 0x3C, 0x00, 0x01, 0x00, 0x88, 0x02, 0x4F, 0x30, 0x33, 0x65,
	   0x00, 0x01, 0x25, 0x80, 0x6C, 0xF2, 0x67, 0x00, 0x10, 0x80, 0xC0,
	   0xF4, 0x69, 0x8F, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF,
	   0xFF, 0x00, 0x00}},
	 { 24000, 27999, 10,
	  { 0xD2, 0x7D, 0x90, 0x01, 0x00, 0x88, 0x02, 0x4F, 0x30, 0x33, 0x65,
	   0x00, 0xCD, 0x24, 0x80, 0x6C, 0xF2, 0x67, 0x00, 0x10, 0x99, 0xC0,
	   0x3A, 0x74, 0x8F, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x83, 0x0F, 0x3E,
	   0xF8, 0x00, 0x00}},
	 { 28000, 55999, 10,
	  { 0xD4, 0xC8, 0x70, 0x01, 0x00, 0x88, 0x02, 0x4F, 0x30, 0x33, 0x65,
	   0x00, 0xCD, 0x24, 0x80, 0x6C, 0xF2, 0x67, 0x00, 0x10, 0x97, 0xC0,
	   0x3A, 0x74, 0x8F, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x83, 0x0F, 0x3E,
	   0xF8, 0x00, 0x00}},
	 { 56000, 111999, 10,
	  { 0xD4, 0x64, 0x30, 0x01, 0x00, 0x88, 0x02, 0x4F, 0x30, 0x33, 0x65,
	   0x00, 0xCD, 0x24, 0x80, 0x6C, 0xF2, 0x67, 0x00, 0x10, 0x93, 0xC0,
	   0x3A, 0x74, 0x8F, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x83, 0x0F, 0x3E,
	   0xF8, 0x00, 0x00}},
	 { 112000, 223999, 10,
	  { 0xD8, 0x64, 0x10, 0x01, 0x00, 0x88, 0x02, 0x4F, 0x30, 0x33, 0x65,
	   0x00, 0xCD, 0x24, 0x80, 0x6C, 0xF2, 0x67, 0x00, 0x10, 0x91, 0xC0,
	   0x3A, 0x74, 0x8F, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x83, 0x0F, 0x3E,
	   0xF8, 0x00, 0x00}},
	 { 224000, 272000, 10,
	  { 0xDC, 0x4B, 0x00, 0x01, 0x00, 0x88, 0x02, 0x4F, 0x30, 0x33, 0x65,
	   0x00, 0xCD, 0x24, 0x80, 0x6C, 0xF2, 0x67, 0x00, 0x10, 0x90, 0xC0,
	   0x3A, 0x74, 0x8F, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x83, 0x0F, 0x3E,
	   0xF8, 0x00, 0x00}},
	 { 272001, 340000, 10,
	  { 0xDC, 0x4B, 0x00, 0x01, 0x00, 0x88, 0x02, 0x4F, 0x30, 0x33, 0x65,
	   0x00, 0xCD, 0x24, 0x80, 0x6C, 0xF2, 0x67, 0x00, 0x10, 0x90, 0xC0,
	   0x3D, 0xFA, 0x8F, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x83, 0x0F, 0x3E,
	   0xF8, 0x00, 0x00}},
	 { 340001, 475200, 10,
	  { 0xDC, 0x4B, 0x00, 0x01, 0x00, 0x88, 0x02, 0x4F, 0x30, 0x33, 0x65,
	   0x00, 0xCD, 0x24, 0x80, 0x6C, 0xF2, 0x67, 0x00, 0x10, 0x90, 0xC0,
	   0x3D, 0xFA, 0x8F, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF,
	   0xFF, 0x00, 0x00}},
	 { 24000, 47999, 12,
	  { 0xD1, 0x3C, 0x70, 0x01, 0x00, 0x88, 0x02, 0x4F, 0x30, 0x33, 0x65,
	   0x00, 0xAB, 0x24, 0x80, 0x6C, 0xF2, 0x67, 0x00, 0x10, 0xA7, 0xC0,
	   0x3A, 0x74, 0x8F, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x83, 0x0F, 0x3E,
	   0xF8, 0x00, 0x00}},
	 { 48000, 95999, 12,
	  { 0xD2, 0x3C, 0x30, 0x01, 0x00, 0x88, 0x02, 0x4F, 0x30, 0x33, 0x65,
	   0x00, 0xAB, 0x24, 0x80, 0x6C, 0xF2, 0x67, 0x00, 0x10, 0xA3, 0xC0,
	   0x3A, 0x74, 0x8F, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x83, 0x0F, 0x3E,
	   0xF8, 0x00, 0x00}},
	 { 96000, 197999, 12,
	  { 0xD4, 0x3C, 0x10, 0x01, 0x00, 0x88, 0x02, 0x4F, 0x30, 0x33, 0x65,
	   0x00, 0xAB, 0x24, 0x80, 0x6C, 0xF2, 0x67, 0x00, 0x10, 0xA1, 0xC0,
	   0x3A, 0x74, 0x8F, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x83, 0x0F, 0x3E,
	   0xF8, 0x00, 0x00}},
	 { 198000, 226999, 12,
	  { 0xD8, 0x3C, 0x00, 0x01, 0x00, 0x88, 0x02, 0x4F, 0x30, 0x33, 0x65,
	   0x00, 0xAB, 0x24, 0x80, 0x6C, 0xF2, 0x67, 0x00, 0x10, 0xA0, 0xC0,
	   0x3A, 0x74, 0x8F, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x83, 0x0F, 0x3E,
	   0xF8, 0x00, 0x00}},
	 { 227000, 340000, 12,
	  { 0xD8, 0x3C, 0x00, 0x01, 0x00, 0x88, 0x02, 0x4F, 0x30, 0x33, 0x65,
	   0x00, 0xAB, 0x24, 0x80, 0x6C, 0xF2, 0x67, 0x00, 0x10, 0xA0, 0xC0,
	   0x3D, 0xFA, 0x8F, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x83, 0x0F, 0x3E,
	   0xF8, 0x00, 0x00}},
	 { 340001, 396000, 12,
	  { 0xD8, 0x3C, 0x00, 0x01, 0x00, 0x88, 0x02, 0x4F, 0x30, 0x33, 0x65,
	   0x00, 0xAB, 0x24, 0x80, 0x6C, 0xF2, 0x67, 0x00, 0x10, 0xA0, 0xC0,
	   0x3D, 0xFA, 0x8F, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF,
	   0xFF, 0x00, 0x00}},
};

struct mwv207_hdmi {
	struct mwv207_output base;

	void __iomem    *mmio;
	bool            sink_is_hdmi;
	bool            sink_has_audio;
	int             vic;

	int             irq;

	u8              mc_clkdis;
	bool            rgb_limited_range;
	struct mwv207_hdmi_phy_data phy_data[19];
};

static const u16 csc_coeff_default[3][4] = {
	{ 0x2000, 0x0000, 0x0000, 0x0000 },
	{ 0x0000, 0x2000, 0x0000, 0x0000 },
	{ 0x0000, 0x0000, 0x2000, 0x0000 }
};

static const u16 csc_coeff_rgb_full_to_rgb_limited[3][4] = {
	{ 0x1b7c, 0x0000, 0x0000, 0x0020 },
	{ 0x0000, 0x1b7c, 0x0000, 0x0020 },
	{ 0x0000, 0x0000, 0x1b7c, 0x0020 }
};

static inline u8 mwv207_hdmi_readb(struct mwv207_hdmi *hdmi, u32 reg)
{
	return readl(hdmi->mmio + reg * 4);
}

static inline void mwv207_hdmi_writeb(struct mwv207_hdmi *hdmi, u8 value, u32 reg)
{
	writel_relaxed(value, hdmi->mmio + reg * 4);
}

static inline void mwv207_hdmi_modb(struct mwv207_hdmi *hdmi, u8 value, u8 mask, u32 reg)
{
	u8 rvalue = mwv207_hdmi_readb(hdmi, reg);

	rvalue = (rvalue & ~mask) | (value & mask);
	mwv207_hdmi_writeb(hdmi, rvalue, reg);
}

static void mwv207_hdmi_mask_writeb(struct mwv207_hdmi *hdmi, u8 data, unsigned int reg,
			     u8 shift, u8 mask)
{
	mwv207_hdmi_modb(hdmi, data << shift, mask, reg);
}

static int mwv207_hdmi_phy_data_from_cfg(struct mwv207_hdmi *hdmi)
{
	const struct mwv207_vdat *vdat;
	int i;

	vdat = mwv207_vbios_vdat(hdmi->base.jdev, 0xfff0 + hdmi->base.idx);
	if (!vdat)
		return -ENOENT;
	if (vdat->len != sizeof(hdmi->phy_data))
		return -EINVAL;
	memcpy(hdmi->phy_data, vdat->dat, vdat->len);
	for (i = 0; i < 19; ++i) {
		hdmi->phy_data[i].min_freq_khz = le32_to_cpu(hdmi->phy_data[i].min_freq_khz);
		hdmi->phy_data[i].max_freq_khz = le32_to_cpu(hdmi->phy_data[i].max_freq_khz);
		hdmi->phy_data[i].bpp = le32_to_cpu(hdmi->phy_data[i].bpp);
	}
	return 0;
}

static void mwv207_hdmi_phy_data_init(struct mwv207_hdmi *hdmi)
{
	if (!mwv207_hdmi_phy_data_from_cfg(hdmi))
		return;

	DRM_INFO("mwv207: hdmi%d use default phy data", hdmi->base.idx);

	BUILD_BUG_ON(sizeof(hdmi->phy_data) != sizeof(default_phy_data));

	memcpy(hdmi->phy_data, default_phy_data, sizeof(hdmi->phy_data));
}

static void mwv207_hdmi_phy_configure_data(struct mwv207_hdmi *hdmi, int kfreq, int bpp)
{
	struct mwv207_output *output = &hdmi->base;
	u32 regbase = MWV207_HDMI_PHY(output->idx);
	int i, cfg, offset;

	for (i = 0; i < 19; ++i) {
		if (kfreq >= hdmi->phy_data[i].min_freq_khz
				&& kfreq <= hdmi->phy_data[i].max_freq_khz
				&& bpp == hdmi->phy_data[i].bpp)
			break;
	}
	if (i >= 19) {
		pr_warn("mwv207: no matching hdmi phydata found");
		return;
	}

	cfg = i;
	mwv207_output_write(output, regbase + 0x70, 0xc8);
	mwv207_output_write(output, regbase + 0x74, 0x2);
	for (i = 0; i < 47; i++) {
		offset = 0x4 + i * 4;
		if (offset == 0x84 || offset == 0x88 || offset == 0xB8)
			continue;

		mwv207_output_write(output, regbase + offset,
				hdmi->phy_data[cfg].config[i]);
	}
}

static void mwv207_hdmi_phy_switch(struct mwv207_hdmi *hdmi, int enable)
{
	struct mwv207_output *output = &hdmi->base;
	u32 regbase = MWV207_HDMI_PHY(output->idx);

	if (enable) {
		mwv207_output_write(output, regbase + 0x84, 0x80);
		mwv207_output_modify(output, MWV207_HDMI_CTRL(output->idx),
				1 << 28, 0 << 28);
	} else {
		mwv207_output_modify(output, MWV207_HDMI_CTRL(output->idx),
				1 << 28, 1 << 28);
		mwv207_output_write(output, regbase + 0x84, 0x00);
	}
}

static void mwv207_hdmi_phy_power_on(struct mwv207_hdmi *hdmi)
{
	u8 val;
	int i;

	mwv207_hdmi_phy_switch(hdmi, 1);

	for (i = 0; i < 5; ++i) {
		val = mwv207_hdmi_readb(hdmi, 0x3004) & 0x01;
		if (val)
			break;
		usleep_range(1000, 2000);
	}

	if (!val)
		pr_warn("mwv207: hdmi PHY PLL failed to lock\n");
}

static void mwv207_hdmi_phy_power_off(struct mwv207_hdmi *hdmi)
{
	u8 val;
	int i;

	mwv207_hdmi_phy_switch(hdmi, 0);

	for (i = 0; i < 5; ++i) {
		val = mwv207_hdmi_readb(hdmi, 0x3004);
		if (!(val & 0x01))
			break;

		usleep_range(1000, 2000);
	}

	if (val & 0x01)
		pr_warn("mwv207: PHY failed to power down\n");
}

static void mwv207_hdmi_phy_configure(struct mwv207_hdmi *hdmi, int kfreq, int bpp)
{
	mwv207_hdmi_phy_power_off(hdmi);

	mwv207_hdmi_phy_configure_data(hdmi, kfreq, bpp);

	mwv207_hdmi_phy_power_on(hdmi);
}

static void mwv207_hdmi_clear_overflow(struct mwv207_hdmi *hdmi)
{
	u8 val;

	mwv207_hdmi_writeb(hdmi, (u8) ~0x02, 0x4002);

	val = mwv207_hdmi_readb(hdmi, 0x1000);
	mwv207_hdmi_writeb(hdmi, val, 0x1000);
}

static void mwv207_hdmi_ih_mutes(struct mwv207_hdmi *hdmi)
{
	u8 ih_mute;

	ih_mute = mwv207_hdmi_readb(hdmi, 0x01FF) | 0x2 | 0x1;

	mwv207_hdmi_writeb(hdmi, ih_mute, 0x01FF);

	mwv207_hdmi_writeb(hdmi, 0xff, 0x0807);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x10D2);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x10D6);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x10DA);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x3006);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x3027);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x3028);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x3102);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x3302);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x3404);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x3505);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x5008);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x7E05);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x7E06);

	mwv207_hdmi_writeb(hdmi, 0xff, 0x0180);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x0181);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x0182);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x0183);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x0184);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x0185);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x0186);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x0187);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x0188);
	mwv207_hdmi_writeb(hdmi, 0xff, 0x0189);

	ih_mute &= ~(0x2 | 0x1);
	mwv207_hdmi_writeb(hdmi, ih_mute, 0x01FF);
}

static void mwv207_hdmi_phy_setup_hpd(struct mwv207_hdmi *hdmi)
{

	mwv207_hdmi_writeb(hdmi, 0x02, 0x3007);
	mwv207_hdmi_writeb(hdmi, 0x1, 0x0104);

	mwv207_hdmi_writeb(hdmi, ~0x02, 0x3006);

	mwv207_hdmi_writeb(hdmi, 0x1, 0x0104);
	mwv207_hdmi_writeb(hdmi, ~0x1, 0x0184);
}

static irqreturn_t mwv207_hdmi_hard_irq(int irq, void *dev_id)
{
	struct mwv207_hdmi *hdmi = dev_id;
	irqreturn_t ret = IRQ_NONE;
	u8 intr_stat;

	intr_stat = mwv207_hdmi_readb(hdmi, 0x0104);
	if (intr_stat) {
		mwv207_hdmi_writeb(hdmi, ~0, 0x0184);
		return IRQ_WAKE_THREAD;
	}

	return ret;
}

static irqreturn_t mwv207_hdmi_irq(int irq, void *dev_id)
{
	u8 intr_stat, phy_int_pol, phy_pol_mask, phy_stat;
	struct mwv207_hdmi *hdmi = dev_id;

	intr_stat = mwv207_hdmi_readb(hdmi, 0x0104);
	phy_int_pol = mwv207_hdmi_readb(hdmi, 0x3007);
	phy_stat = mwv207_hdmi_readb(hdmi, 0x3004);

	phy_pol_mask = 0;
	if (intr_stat & 0x1)
		phy_pol_mask |= 0x02;
	if (intr_stat & 0x4)
		phy_pol_mask |= 0x10;
	if (intr_stat & 0x8)
		phy_pol_mask |= 0x20;
	if (intr_stat & 0x10)
		phy_pol_mask |= 0x40;
	if (intr_stat & 0x20)
		phy_pol_mask |= 0x80;

	if (phy_pol_mask)
		mwv207_hdmi_modb(hdmi, ~phy_int_pol, phy_pol_mask, 0x3007);

	if (intr_stat & 0x1)
		drm_helper_hpd_irq_event(&hdmi->base.jdev->base);

	mwv207_hdmi_writeb(hdmi, intr_stat, 0x0104);
	mwv207_hdmi_writeb(hdmi, ~0x1, 0x0184);

	return IRQ_HANDLED;
}

static void mwv207_hdmi_switch(struct mwv207_output *output, bool on)
{
	mwv207_output_modify(output, MWV207_HDMI_CTRL(output->idx), 1, on ? 1 : 0);
}

static void mwv207_hdmi_select_crtc(struct mwv207_output *output)
{

	mwv207_output_modify(output, MWV207_HDMI_CTRL(output->idx),
			0x3 << 4, drm_crtc_index(output->cur_crtc) << 4);

	jdev_modify(output->jdev, 0x9b003c, 0b111 << (8 + output->idx * 4),
			drm_crtc_index(output->cur_crtc) << (8 + output->idx * 4));
}

static void mwv207_hdmi_disable_overflow_interrupts(struct mwv207_hdmi *hdmi)
{
	mwv207_hdmi_writeb(hdmi, 0x03, 0x0182);
}

static bool mwv207_hdmi_support_scdc(struct mwv207_hdmi *hdmi,
				 const struct drm_display_info *display)
{

	if (!display->hdmi.scdc.supported ||
	    !display->hdmi.scdc.scrambling.supported)
		return false;

	if (!display->hdmi.scdc.scrambling.low_rates &&
	    display->max_tmds_clock <= 340000)
		return false;

	return true;
}

static void mwv207_hdmi_av_composer(struct mwv207_hdmi *hdmi, const struct drm_display_mode *mode)
{
	const struct drm_display_info *display = &hdmi->base.connector.display_info;
	const struct drm_hdmi_info *hdmi_info = &display->hdmi;
	int hblank, vblank, h_de_hs, v_de_vs, hsync_len, vsync_len;
	struct mwv207_output *output = &hdmi->base;
	unsigned int vdisplay, hdisplay;
	u32 tmdsclock, rate;
	u8 inv_val, bytes;

	tmdsclock = mode->clock * 1000;

	inv_val = (mwv207_hdmi_support_scdc(hdmi, display) &&
		    (tmdsclock > 340000000 ||
		     hdmi_info->scdc.scrambling.low_rates) ? 0x80 : 0x00);

	inv_val |= mode->flags & DRM_MODE_FLAG_PVSYNC ? 0x40 : 0x00;

	inv_val |= mode->flags & DRM_MODE_FLAG_PHSYNC ? 0x20 : 0x00;

	inv_val |= 0x10;

	if (hdmi->vic == 39)
		inv_val |= 0x2;
	else
		inv_val |= mode->flags & DRM_MODE_FLAG_INTERLACE ? 0x2 : 0x0;

	inv_val |= mode->flags & DRM_MODE_FLAG_INTERLACE ? 0x1 : 0x0;

	inv_val |= hdmi->sink_is_hdmi ? 0x8 : 0x0;

	mwv207_hdmi_writeb(hdmi, inv_val, 0x1000);

	rate = drm_mode_vrefresh(mode) * 1000;
	mwv207_hdmi_writeb(hdmi, rate >> 16, 0x1010);
	mwv207_hdmi_writeb(hdmi, rate >> 8, 0x100F);
	mwv207_hdmi_writeb(hdmi, rate, 0x100E);

	mwv207_output_modify(output, MWV207_HDMI_CTRL(output->idx), 0x1 << 9,
			(mode->flags & DRM_MODE_FLAG_PHSYNC ? 1 : 0) << 9);
	mwv207_output_modify(output, MWV207_HDMI_CTRL(output->idx), 0x1 << 8,
			(mode->flags & DRM_MODE_FLAG_PVSYNC ? 1 : 0) << 8);
	mwv207_output_modify(output, MWV207_HDMI_CTRL(output->idx), 0x1 << 10, 1 << 10);

	hdisplay = mode->hdisplay;
	hblank = mode->htotal - mode->hdisplay;
	h_de_hs = mode->hsync_start - mode->hdisplay;
	hsync_len = mode->hsync_end - mode->hsync_start;

	vdisplay = mode->vdisplay;
	vblank = mode->vtotal - mode->vdisplay;
	v_de_vs = mode->vsync_start - mode->vdisplay;
	vsync_len = mode->vsync_end - mode->vsync_start;

	if (mode->flags & DRM_MODE_FLAG_INTERLACE) {
		vdisplay /= 2;
		vblank /= 2;
		v_de_vs /= 2;
		vsync_len /= 2;
	}

	if (mwv207_hdmi_support_scdc(hdmi, display)) {
		if (tmdsclock > 340000000 ||
		    hdmi_info->scdc.scrambling.low_rates) {
			drm_scdc_readb(hdmi->base.ddc, SCDC_SINK_VERSION, &bytes);
			drm_scdc_writeb(hdmi->base.ddc, SCDC_SOURCE_VERSION,
				min_t(u8, bytes, 0x1));

			drm_scdc_set_scrambling(&hdmi->base.connector, 1);

			mwv207_hdmi_writeb(hdmi, (u8)~0x02, 0x4002);
			mwv207_hdmi_writeb(hdmi, 1, 0x10E1);
		} else {
			mwv207_hdmi_writeb(hdmi, 0, 0x10E1);
			mwv207_hdmi_writeb(hdmi, (u8)~0x02, 0x4002);
			drm_scdc_set_scrambling(&hdmi->base.connector, 0);
		}
	}

	mwv207_hdmi_writeb(hdmi, hdisplay >> 8, 0x1002);
	mwv207_hdmi_writeb(hdmi, hdisplay, 0x1001);

	mwv207_hdmi_writeb(hdmi, vdisplay >> 8, 0x1006);
	mwv207_hdmi_writeb(hdmi, vdisplay, 0x1005);

	mwv207_hdmi_writeb(hdmi, hblank >> 8, 0x1004);
	mwv207_hdmi_writeb(hdmi, hblank, 0x1003);

	mwv207_hdmi_writeb(hdmi, vblank >> 8, 0x102E);
	mwv207_hdmi_writeb(hdmi, vblank, 0x1007);

	mwv207_hdmi_writeb(hdmi, h_de_hs >> 8, 0x1009);
	mwv207_hdmi_writeb(hdmi, h_de_hs, 0x1008);

	mwv207_hdmi_writeb(hdmi, v_de_vs >> 8, 0x102F);
	mwv207_hdmi_writeb(hdmi, v_de_vs, 0x100C);

	mwv207_hdmi_writeb(hdmi, hsync_len >> 8, 0x100B);
	mwv207_hdmi_writeb(hdmi, hsync_len, 0x100A);

	mwv207_hdmi_writeb(hdmi, vsync_len, 0x100D);
}

static void mwv207_hdmi_enable_video_path(struct mwv207_hdmi *hdmi)
{

	mwv207_hdmi_writeb(hdmi, 12, 0x1011);
	mwv207_hdmi_writeb(hdmi, 32, 0x1012);
	mwv207_hdmi_writeb(hdmi, 1, 0x1013);

	mwv207_hdmi_writeb(hdmi, 0x0B, 0x1014);
	mwv207_hdmi_writeb(hdmi, 0x16, 0x1015);
	mwv207_hdmi_writeb(hdmi, 0x21, 0x1016);

	hdmi->mc_clkdis |= 0x40 | 0x8 | 0x10 | 0x4 | 0x2;
	hdmi->mc_clkdis &= ~0x1;
	mwv207_hdmi_writeb(hdmi, hdmi->mc_clkdis, 0x4001);

	hdmi->mc_clkdis &= ~0x2;
	mwv207_hdmi_writeb(hdmi, hdmi->mc_clkdis, 0x4001);

	if (hdmi->rgb_limited_range) {
		hdmi->mc_clkdis &= ~0x10;
		mwv207_hdmi_writeb(hdmi, hdmi->mc_clkdis, 0x4001);

		mwv207_hdmi_writeb(hdmi, 0x1, 0x4004);
	} else {
		hdmi->mc_clkdis |= 0x10;
		mwv207_hdmi_writeb(hdmi, hdmi->mc_clkdis, 0x4001);

		mwv207_hdmi_writeb(hdmi, 0x0, 0x4004);
	}
}

static void mwv207_hdmi_enable_audio_clk(struct mwv207_hdmi *hdmi, bool enable)
{
	if (enable)
		hdmi->mc_clkdis &= ~0x8;
	else
		hdmi->mc_clkdis |= 0x8;
	mwv207_hdmi_writeb(hdmi, hdmi->mc_clkdis, 0x4001);
}

static void mwv207_hdmi_config_AVI(struct mwv207_hdmi *hdmi,
		struct drm_connector *connector,
		const struct drm_display_mode *mode)
{
	struct hdmi_avi_infoframe frame;
	u8 val;

	drm_hdmi_avi_infoframe_from_display_mode(&frame, connector, mode);

	drm_hdmi_avi_infoframe_quant_range(&frame, connector, mode,
			hdmi->rgb_limited_range ?
			HDMI_QUANTIZATION_RANGE_LIMITED :
			HDMI_QUANTIZATION_RANGE_FULL);

	frame.colorspace = HDMI_COLORSPACE_RGB;
	frame.colorimetry = HDMI_COLORIMETRY_NONE;
	frame.extended_colorimetry =
		HDMI_EXTENDED_COLORIMETRY_XV_YCC_601;

	val = (frame.scan_mode & 3) << 4 | (frame.colorspace & 3);
	if (frame.active_aspect & 15)
		val |= 0x40;
	if (frame.top_bar || frame.bottom_bar)
		val |= 0x08;
	if (frame.left_bar || frame.right_bar)
		val |= 0x04;
	mwv207_hdmi_writeb(hdmi, val, 0x1019);

	val = ((frame.colorimetry & 0x3) << 6) |
	      ((frame.picture_aspect & 0x3) << 4) |
	      (frame.active_aspect & 0xf);
	mwv207_hdmi_writeb(hdmi, val, 0x101A);

	val = ((frame.extended_colorimetry & 0x7) << 4) |
	      ((frame.quantization_range & 0x3) << 2) |
	      (frame.nups & 0x3);
	if (frame.itc)
		val |= 0x80;
	mwv207_hdmi_writeb(hdmi, val, 0x101B);

	val = frame.video_code & 0x7f;
	mwv207_hdmi_writeb(hdmi, val, 0x101C);

	val = ((1 << 4) & 0xF0);
	mwv207_hdmi_writeb(hdmi, val, 0x10E0);

	val = ((frame.ycc_quantization_range & 0x3) << 2) |
	      (frame.content_type & 0x3);
	mwv207_hdmi_writeb(hdmi, val, 0x1017);

	mwv207_hdmi_writeb(hdmi, frame.top_bar & 0xff, 0x101D);
	mwv207_hdmi_writeb(hdmi, (frame.top_bar >> 8) & 0xff, 0x101E);
	mwv207_hdmi_writeb(hdmi, frame.bottom_bar & 0xff, 0x101F);
	mwv207_hdmi_writeb(hdmi, (frame.bottom_bar >> 8) & 0xff, 0x1020);
	mwv207_hdmi_writeb(hdmi, frame.left_bar & 0xff, 0x1021);
	mwv207_hdmi_writeb(hdmi, (frame.left_bar >> 8) & 0xff, 0x1022);
	mwv207_hdmi_writeb(hdmi, frame.right_bar & 0xff, 0x1023);
	mwv207_hdmi_writeb(hdmi, (frame.right_bar >> 8) & 0xff, 0x1024);
}

static void mwv207_hdmi_config_vendor_specific_infoframe(struct mwv207_hdmi *hdmi,
		struct drm_connector *connector,
		const struct drm_display_mode *mode)
{
	struct hdmi_vendor_infoframe frame;
	u8 buffer[10];
	ssize_t err;

	err = drm_hdmi_vendor_infoframe_from_display_mode(&frame, connector,
							  mode);
	if (err < 0)
		return;

	err = hdmi_vendor_infoframe_pack(&frame, buffer, sizeof(buffer));
	if (err < 0) {
		DRM_ERROR("Failed to pack vendor infoframe: %zd\n", err);
		return;
	}
	mwv207_hdmi_mask_writeb(hdmi, 0, 0x10B3, 3, 0x08);

	mwv207_hdmi_writeb(hdmi, buffer[2], 0x102A);

	mwv207_hdmi_writeb(hdmi, buffer[4], 0x1029);
	mwv207_hdmi_writeb(hdmi, buffer[5], 0x1030);
	mwv207_hdmi_writeb(hdmi, buffer[6], 0x1031);

	mwv207_hdmi_writeb(hdmi, buffer[7], 0x1032);
	mwv207_hdmi_writeb(hdmi, buffer[8], 0x1033);

	if (frame.s3d_struct >= HDMI_3D_STRUCTURE_SIDE_BY_SIDE_HALF)
		mwv207_hdmi_writeb(hdmi, buffer[9], 0x1034);

	mwv207_hdmi_writeb(hdmi, 1, 0x10B4);

	mwv207_hdmi_writeb(hdmi, 0x11, 0x10B5);

	mwv207_hdmi_mask_writeb(hdmi, 1, 0x10B3, 3, 0x08);
}

static void mwv207_hdmi_config_drm_infoframe(struct mwv207_hdmi *hdmi,
				      const struct drm_connector *connector)
{
	const struct drm_connector_state *conn_state = connector->state;
	struct hdmi_drm_infoframe frame;
	u8 buffer[30];
	ssize_t err;
	int i;

	mwv207_hdmi_modb(hdmi, 0x00, 0x80, 0x10E3);

	err = drm_hdmi_infoframe_set_hdr_metadata(&frame, conn_state);
	if (err < 0)
		return;

	err = hdmi_drm_infoframe_pack(&frame, buffer, sizeof(buffer));
	if (err < 0)
		return;

	mwv207_hdmi_writeb(hdmi, frame.version, 0x1168);
	mwv207_hdmi_writeb(hdmi, frame.length, 0x1169);

	for (i = 0; i < frame.length; i++)
		mwv207_hdmi_writeb(hdmi, buffer[4 + i], 0x116A + i);

	mwv207_hdmi_writeb(hdmi, 1, 0x1167);
	mwv207_hdmi_modb(hdmi, 0x80, 0x80, 0x10E3);
}

static void mwv207_hdmi_video_packetize(struct mwv207_hdmi *hdmi)
{
	unsigned int output_select = 0x3;
	unsigned int remap_size = 0x0;
	unsigned int color_depth = 4;
	u8 val, vp_conf;

	val = ((color_depth << 4) & 0xF0);
	mwv207_hdmi_writeb(hdmi, val, 0x0801);

	mwv207_hdmi_modb(hdmi, 0x1, 0x1, 0x0802);

	vp_conf = 0x00 | 0x4;

	mwv207_hdmi_modb(hdmi, vp_conf, 0x10 | 0x4, 0x0804);

	mwv207_hdmi_modb(hdmi, 1 << 5, 0x20, 0x0802);

	mwv207_hdmi_writeb(hdmi, remap_size, 0x0803);

	vp_conf = 0x40 | 0x00 |	0x0;

	mwv207_hdmi_modb(hdmi, vp_conf, 0x40 | 0x20 | 0x8, 0x0804);

	mwv207_hdmi_modb(hdmi, 0x2 | 0x4, 0x2 | 0x4, 0x0802);

	mwv207_hdmi_modb(hdmi, output_select, 0x3, 0x0804);
}

static void mwv207_hdmi_update_csc_coeffs(struct mwv207_hdmi *hdmi)
{
	const u16 (*csc_coeff)[3][4] = &csc_coeff_default;
	unsigned int i;
	u32 csc_scale = 1;

	if (hdmi->rgb_limited_range)
		csc_coeff = &csc_coeff_rgb_full_to_rgb_limited;
	else
		csc_coeff  = &csc_coeff_default;

	for (i = 0; i < ARRAY_SIZE(csc_coeff_default[0]); i++) {
		u16 coeff_a = (*csc_coeff)[0][i];
		u16 coeff_b = (*csc_coeff)[1][i];
		u16 coeff_c = (*csc_coeff)[2][i];

		mwv207_hdmi_writeb(hdmi, coeff_a & 0xff, 0x4103 + i * 2);
		mwv207_hdmi_writeb(hdmi, coeff_a >> 8, 0x4102 + i * 2);
		mwv207_hdmi_writeb(hdmi, coeff_b & 0xff, 0x410B + i * 2);
		mwv207_hdmi_writeb(hdmi, coeff_b >> 8, 0x410A + i * 2);
		mwv207_hdmi_writeb(hdmi, coeff_c & 0xff, 0x4113 + i * 2);
		mwv207_hdmi_writeb(hdmi, coeff_c >> 8, 0x4112 + i * 2);
	}

	mwv207_hdmi_modb(hdmi, csc_scale, 0x03, 0x4101);
}

static void mwv207_hdmi_video_csc(struct mwv207_hdmi *hdmi)
{
	int color_depth = 0x00;

	mwv207_hdmi_writeb(hdmi, 0, 0x4100);
	mwv207_hdmi_modb(hdmi, color_depth, 0xF0, 0x4101);

	mwv207_hdmi_update_csc_coeffs(hdmi);
}

static void mwv207_hdmi_video_sample(struct mwv207_hdmi *hdmi)
{
	int color_format = 1;
	u8 val;

	val = 0x00 | ((color_format << 0) & 0x1F);
	mwv207_hdmi_writeb(hdmi, val, 0x0200);

	val = 0x4 | 0x2 | 0x1;
	mwv207_hdmi_writeb(hdmi, val, 0x0201);
	mwv207_hdmi_writeb(hdmi, 0x0, 0x0202);
	mwv207_hdmi_writeb(hdmi, 0x0, 0x0203);
	mwv207_hdmi_writeb(hdmi, 0x0, 0x0204);
	mwv207_hdmi_writeb(hdmi, 0x0, 0x0205);
	mwv207_hdmi_writeb(hdmi, 0x0, 0x0206);
	mwv207_hdmi_writeb(hdmi, 0x0, 0x0207);
}

static void mwv207_hdmi_tx_hdcp_config(struct mwv207_hdmi *hdmi)
{
	u8 de = 0x10;

	mwv207_hdmi_modb(hdmi, 0x0, 0x4, 0x5000);

	mwv207_hdmi_modb(hdmi, de, 0x10, 0x5009);

	mwv207_hdmi_modb(hdmi, 0x2, 0x2, 0x5001);
}

static void mwv207_hdmi_set_timing(struct mwv207_hdmi *hdmi)
{
	struct drm_display_mode *mode;

	mwv207_hdmi_disable_overflow_interrupts(hdmi);

	mode = &hdmi->base.cur_crtc->state->adjusted_mode;
	hdmi->vic = drm_match_cea_mode(mode);

	hdmi->rgb_limited_range = hdmi->sink_is_hdmi &&
		drm_default_rgb_quant_range(mode) ==
		HDMI_QUANTIZATION_RANGE_LIMITED;

	mwv207_hdmi_av_composer(hdmi, mode);

	mwv207_hdmi_phy_configure(hdmi, mode->clock, 8);

	mwv207_hdmi_enable_video_path(hdmi);

	if (hdmi->sink_has_audio) {
		mwv207_hdmi_enable_audio_clk(hdmi, 1);

	}

	if (hdmi->sink_is_hdmi) {
		struct drm_connector *conn = &hdmi->base.connector;

		mwv207_hdmi_config_AVI(hdmi, conn, mode);
		mwv207_hdmi_config_vendor_specific_infoframe(hdmi, conn, mode);
		mwv207_hdmi_config_drm_infoframe(hdmi, conn);
	}

	mwv207_hdmi_video_packetize(hdmi);
	mwv207_hdmi_video_csc(hdmi);
	mwv207_hdmi_video_sample(hdmi);
	mwv207_hdmi_tx_hdcp_config(hdmi);

	mwv207_hdmi_clear_overflow(hdmi);
}

static int mwv207_hdmi_get_modes(struct drm_connector *connector)
{
	struct mwv207_hdmi *hdmi = connector_to_hdmi(connector);
	int count;

	count = mwv207_output_get_modes(connector);

	if (connector->edid_blob_ptr) {
		struct edid *edid;

		edid = (struct edid *)connector->edid_blob_ptr->data;
		hdmi->sink_is_hdmi = drm_detect_hdmi_monitor(edid);
		hdmi->sink_has_audio = drm_detect_monitor_audio(edid);
	} else {
		hdmi->sink_is_hdmi = false;
		hdmi->sink_has_audio = false;
	}

	return count;
}

static enum drm_mode_status mwv207_hdmi_mode_valid(struct mwv207_hdmi *hdmi,
		const struct drm_display_mode *mode)
{

	if (mode->clock >= 384000)
		return MODE_CLOCK_HIGH;

	if (mode->clock > 594000)
		return MODE_CLOCK_HIGH;

	if (!hdmi->sink_is_hdmi && mode->clock >= 165000)
		return MODE_CLOCK_HIGH;

	return MODE_OK;
}

static enum drm_mode_status mwv207_hdmi_connector_mode_valid(struct drm_connector *connector,
		struct drm_display_mode *mode)
{
	struct mwv207_hdmi *hdmi = connector_to_hdmi(connector);

	return mwv207_hdmi_mode_valid(hdmi, mode);
}

static int mwv207_hdmi_detect_ctx(struct drm_connector *connector,
			  struct drm_modeset_acquire_ctx *ctx,
			  bool force)
{
	struct mwv207_hdmi *hdmi = connector_to_hdmi(connector);
	u32 value;

	value = mwv207_hdmi_readb(hdmi, 0x3004);
	if (value & 0x2)
		return connector_status_connected;
	else
		return connector_status_disconnected;
}

static void mwv207_hdmi_destroy(struct drm_connector *conn)
{
	struct mwv207_hdmi *hdmi = connector_to_hdmi(conn);

	free_irq(hdmi->irq, hdmi);
	drm_connector_unregister(conn);
	drm_connector_cleanup(conn);
}

static const struct drm_connector_helper_funcs mwv207_hdmi_connector_helper_funcs = {
	.get_modes  = mwv207_hdmi_get_modes,
	.mode_valid = mwv207_hdmi_connector_mode_valid,
	.detect_ctx = mwv207_hdmi_detect_ctx
};

static const struct drm_connector_funcs mwv207_hdmi_connector_funcs = {
	.reset                  = drm_atomic_helper_connector_reset,
	.fill_modes             = drm_helper_probe_single_connector_modes,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state   = drm_atomic_helper_connector_destroy_state,
	.destroy                = mwv207_hdmi_destroy,
	.late_register          = mwv207_output_late_register,
	.early_unregister       = mwv207_output_early_unregister,
};

static enum drm_mode_status mwv207_hdmi_encoder_mode_valid(struct drm_encoder *encoder,
		const struct drm_display_mode *mode)
{
	struct mwv207_hdmi *hdmi = encoder_to_hdmi(encoder);

	return mwv207_hdmi_mode_valid(hdmi, mode);
}

static int mwv207_hdmi_encoder_atomic_check(struct drm_encoder *encoder,
		struct drm_crtc_state *crtc_state,
		struct drm_connector_state *conn_state)
{
	return 0;
}

static void mwv207_hdmi_encoder_enable(struct drm_encoder *encoder)
{
	struct mwv207_output *output = encoder_to_output(encoder);

	mwv207_hdmi_select_crtc(output);

	mwv207_hdmi_set_timing(encoder_to_hdmi(encoder));

	mwv207_hdmi_switch(output, true);
}

static void mwv207_hdmi_encoder_disable(struct drm_encoder *encoder)
{
	struct mwv207_output *output = encoder_to_output(encoder);

	mwv207_hdmi_switch(output, false);
}

static void mwv207_hdmi_encoder_reset(struct drm_encoder *encoder)
{
	mwv207_hdmi_encoder_disable(encoder);
	mwv207_hdmi_ih_mutes(encoder_to_hdmi(encoder));
	mwv207_hdmi_phy_setup_hpd(encoder_to_hdmi(encoder));
}

static const struct drm_encoder_funcs mwv207_hdmi_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
	.reset   = mwv207_hdmi_encoder_reset,
};

static const struct drm_encoder_helper_funcs mwv207_hdmi_encoder_helper_funcs = {
	.mode_valid      = mwv207_hdmi_encoder_mode_valid,
	.atomic_check    = mwv207_hdmi_encoder_atomic_check,
	.enable          = mwv207_hdmi_encoder_enable,
	.disable         = mwv207_hdmi_encoder_disable,
};

static int mwv207_hdmi_init_single(struct mwv207_device *jdev, int idx)
{
	struct mwv207_output *output;
	struct mwv207_hdmi *hdmi;
	int ret;

	hdmi = devm_kzalloc(jdev->dev, sizeof(*hdmi), GFP_KERNEL);
	if (!hdmi)
		return -ENOMEM;

	output = &hdmi->base;
	output->jdev = jdev;
	output->idx  = idx;
	output->mmio = jdev->mmio + 0x9a0000;
	output->i2c_chan = 0 + idx;
	hdmi->mmio = jdev->mmio + MWV207_HDMI_BASE(idx);
	hdmi->mc_clkdis = 0x7f;

	mwv207_hdmi_phy_data_init(hdmi);

	output->connector.polled = DRM_CONNECTOR_POLL_HPD;
	ret = drm_connector_init(&jdev->base, &output->connector,
			&mwv207_hdmi_connector_funcs, DRM_MODE_CONNECTOR_HDMIA);
	if (ret)
		return ret;
	drm_connector_helper_add(&output->connector, &mwv207_hdmi_connector_helper_funcs);

	output->encoder.possible_crtcs = (1 << jdev->base.mode_config.num_crtc) - 1;
	ret = drm_encoder_init(&jdev->base, &output->encoder,
			&mwv207_hdmi_encoder_funcs, DRM_MODE_ENCODER_TMDS,
			"hdmi-%d", output->idx);
	if (ret)
		return ret;
	drm_encoder_helper_add(&output->encoder, &mwv207_hdmi_encoder_helper_funcs);

	ret = drm_connector_attach_encoder(&output->connector, &output->encoder);
	if (ret)
		return ret;

	hdmi->irq = irq_find_mapping(jdev->irq_domain, output->idx + 32);
	BUG_ON(hdmi->irq == 0);

	ret = request_threaded_irq(hdmi->irq,
			mwv207_hdmi_hard_irq, mwv207_hdmi_irq,
			IRQF_SHARED, output->encoder.name, hdmi);
	if (ret)
		return ret;

	ret =  drm_connector_register(&output->connector);
	if (ret)
		free_irq(hdmi->irq, hdmi);

	return ret;
}

int mwv207_hdmi_init(struct mwv207_device *jdev)
{
	int i, ret;

	for (i = 0; i < 4; i++) {

		if (jdev_read(jdev, MWV207_HDMI_BASE(i)) != 0x21)
			continue;
		ret = mwv207_hdmi_init_single(jdev, i);
		if (ret)
			return ret;
	}

	return 0;
}
