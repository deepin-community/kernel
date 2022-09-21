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

#include "phytium_display_drv.h"
#include "phytium_reg.h"
#include "phytium_dp.h"

/* [reg][ling_rate 1.62->8.1] */
static int vco_val[12][4] = {
	{0x0509, 0x0509, 0x0509, 0x0509}, // CP_PADJ
	{0x0f00, 0x0f00, 0x0f00, 0x0f00}, // CP_IADJ
	{0x0F08, 0x0F08, 0x0F08, 0x0F08}, // FILT_PADJ
	{0x0061, 0x006C, 0x006C, 0x0051}, // INTDIV
	{0x3333, 0x0000, 0x0000, 0x0000}, // FRACDIVL
	{0x0000, 0x0000, 0x0000, 0x0000}, // FRACDIVH
	{0x0042, 0x0048, 0x0048, 0x0036}, // HIGH_THR
	{0x0002, 0x0002, 0x0002, 0x0002}, // PDIAG_CTRL
	{0x0c5e, 0x0c5e, 0x0c5e, 0x0c5e}, // VCOCAL_PLLCNT_START
	{0x00c7, 0x00c7, 0x00c7, 0x00c7}, // LOCK_PEFCNT
	{0x00c7, 0x00c7, 0x00c7, 0x00c7}, // LOCK_PLLCNT_START
	{0x0005, 0x0005, 0x0005, 0x0005}, // LOCK_PLLCNT_THR
};

static int mgnfs_val[4][4][4] = // [link_rate][swing][emphasis]
{
	/* 1.62Gbps */
	{
		{0x0026, 0x001f, 0x0012, 0x0000},
		{0x0013, 0x0013, 0x0000, 0x0000},
		{0x0006, 0x0000, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x0000},
	},

	/* 2.7Gbps */
	{
		{0x0026, 0x001f, 0x0012, 0x0000},
		{0x0013, 0x0013, 0x0000, 0x0000},
		{0x0006, 0x0000, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x0000},
	},

	/* 5.4Gbps */
	{
		{0x0026, 0x0013, 0x005, 0x0000},
		{0x0013, 0x006, 0x0000, 0x0000},
		{0x0006, 0x0000, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x0000},
	},

	/* 8.1Gbps */
	{
		{0x0026, 0x0013, 0x005, 0x0000},
		{0x0013, 0x006, 0x0000, 0x0000},
		{0x0006, 0x0000, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x0000},
	},
};

static int cpost_val[4][4][4] = // [link_rate][swing][emphasis]
{
	/* 1.62Gbps */
	{
		{0x0000, 0x0014, 0x0020, 0x002a},
		{0x0000, 0x0014, 0x001f, 0x0000},
		{0x0000, 0x0013, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x0000},
	},

	/* 2.7Gbps */
	{
		{0x0000, 0x0014, 0x0020, 0x002a},
		{0x0000, 0x0014, 0x001f, 0x0000},
		{0x0000, 0x0013, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x0000},
	},

	/* 5.4Gbps */
	{
		{0x0000, 0x0014, 0x0022, 0x002e},
		{0x0000, 0x0013, 0x0020, 0x0000},
		{0x0000, 0x0013, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x0000},
	},

	/* 8.1Gbps */
	{
		{0x0000, 0x0014, 0x0022, 0x002e},
		{0x0000, 0x0013, 0x0020, 0x0000},
		{0x0000, 0x0013, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x0000},
	},
};

static void x100_phy_writel(struct phytium_dp_device *phytium_dp, u32 address, u32 data)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;

#if DEBUG_LOG
	pr_info("phy address write: 0x%x data:0x%x\n", address, data);
#endif
	phytium_writel_reg(priv, address, PHYTIUM_PHY_ACCESS_ADDRESS(port));
	phytium_writel_reg(priv, data, PHYTIUM_PHY_WRITE_DATA(port));
	phytium_writel_reg(priv, ACCESS_WRITE, PHYTIUM_PHY_ACCESS_CTRL(port));
	udelay(10);
}

static u32 x100_phy_readl(struct phytium_dp_device *phytium_dp, u32 address)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;
	u32 data;

	phytium_writel_reg(priv, address, PHYTIUM_PHY_ACCESS_ADDRESS(port));
	phytium_writel_reg(priv, ACCESS_READ, PHYTIUM_PHY_ACCESS_CTRL(port));
	udelay(10);
	data = phytium_readl_reg(priv, PHYTIUM_PHY_READ_DATA(port));
#if DEBUG_LOG
	pr_info("phy address read: 0x%x data:0x%x\n", address, data);
#endif

	return data;
}

static void x100_dp_phy_set_lane_and_rate(struct phytium_dp_device *phytium_dp,
							uint8_t link_lane_count,
							uint32_t link_rate)
{
	int port = phytium_dp->port%3;
	int i = 0, data, tmp, tmp1, index = 0, mask;

	if (port == 0 || port == 1) {
		/* set pma powerdown */
		data = 0;
		mask = 0;
		for (i = 0; i < phytium_dp->source_max_lane_count; i++) {
			data |= (A3_POWERDOWN3 << i*A3_POWERDOWN3_SHIFT);
			mask |= (((1<<A3_POWERDOWN3_SHIFT) - 1) << (i*A3_POWERDOWN3_SHIFT));
		}
		if (port == 0) {
			tmp = x100_phy_readl(phytium_dp, PHYTIUM_PHY0_PMA0_POWER);
			tmp = (tmp & (~mask)) | data;
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PMA0_POWER, tmp);
		} else {
			tmp = x100_phy_readl(phytium_dp, PHYTIUM_PHY0_PMA1_POWER);
			tmp = (tmp & (~mask)) | data;
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PMA1_POWER, tmp);
		}

		/* lane pll disable */
		data = 0;
		mask = 0;
		for (i = 0; i < phytium_dp->source_max_lane_count; i++) {
			data |= (PLL_EN << i*PLL_EN_SHIFT);
			mask |= (((1<<PLL_EN_SHIFT) - 1) << (i*PLL_EN_SHIFT));
		}
		mask = (mask << (port*PLL_EN_SHIFT*4));
		data = (data << (port*PLL_EN_SHIFT*4));
		tmp = x100_phy_readl(phytium_dp, PHYTUIM_PHY0_PLL_EN);
		tmp = (tmp & (~mask));
		x100_phy_writel(phytium_dp, PHYTUIM_PHY0_PLL_EN, tmp);

		/* pma pll disable */
		mask = (CONTROL_ENABLE << (port*CONTROL_ENABLE_SHIFT));
		data = (CONTROL_ENABLE << (port*CONTROL_ENABLE_SHIFT));
		tmp = x100_phy_readl(phytium_dp, PHYTIUM_PHY0_PMA_CONTROL);
		tmp = (tmp & (~mask));
		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PMA_CONTROL, tmp);

		/* read pma pll disable state */
		mdelay(2);
		x100_phy_readl(phytium_dp, PHYTIUM_PHY0_PMA_CONTROL2);

		/* config link rate */
		switch (link_rate) {
		case 810000:
			tmp = PLL_LINK_RATE_810000;
			tmp1 = HSCLK_LINK_RATE_810000;
			index = 3;
			break;
		case 540000:
			tmp = PLL_LINK_RATE_540000;
			tmp1 = HSCLK_LINK_RATE_540000;
			index = 2;
			break;
		case 270000:
			tmp = PLL_LINK_RATE_270000;
			tmp1 = HSCLK_LINK_RATE_270000;
			index = 1;
			break;
		case 162000:
			tmp = PLL_LINK_RATE_162000;
			tmp1 = HSCLK_LINK_RATE_162000;
			index = 0;
			break;
		default:
			DRM_ERROR("phytium dp rate(%d) not support\n", link_rate);
			tmp = PLL_LINK_RATE_162000;
			tmp1 = HSCLK_LINK_RATE_162000;
			index = 0;
			break;
		}
		if (port == 0) {
			/* config analog pll for link0 */
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_CLK_SEL, tmp);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_HSCLK0_SEL, HSCLK_LINK_0);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_HSCLK0_DIV, tmp1);

			/* config digital pll for link0 */
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLLDRC0_CTRL, PLLDRC_LINK0);

			/* common for all rate */
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_DSM_M0, PLL0_DSM_M0);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_VCOCAL_START,
					   PLL0_VCOCAL_START);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_VCOCAL_CTRL,
					   PLL0_VCOCAL_CTRL);

			/* different for all rate */
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_CP_PADJ,
					   vco_val[0][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_CP_IADJ,
					   vco_val[1][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_CP_FILT_PADJ,
					   vco_val[2][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_INTDIV,
					   vco_val[3][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_FRACDIVL,
					   vco_val[4][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_FRACDIVH,
					   vco_val[5][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_HIGH_THR,
					   vco_val[6][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_PDIAG_CTRL,
					   vco_val[7][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_VCOCAL_PLLCNT_START,
					   vco_val[8][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_LOCK_PEFCNT,
					   vco_val[9][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_LOCK_PLLCNT_START,
					   vco_val[10][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_LOCK_PLLCNT_THR,
					   vco_val[11][index]);

			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_TX_PSC_A0,
					   PLL0_TX_PSC_A0);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_TX_PSC_A2,
					   PLL0_TX_PSC_A2);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_TX_PSC_A3,
					   PLL0_TX_PSC_A3);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_RX_PSC_A0,
					   PLL0_RX_PSC_A0);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_RX_PSC_A2,
					   PLL0_RX_PSC_A2);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_RX_PSC_A3,
					   PLL0_RX_PSC_A3);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_RX_PSC_CAL,
					   PLL0_RX_PSC_CAL);

			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_XCVR_CTRL,
					   PLL0_XCVR_CTRL);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_RX_GCSM1_CTRL,
					   PLL0_RX_GCSM1_CTRL);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_RX_GCSM2_CTRL,
					   PLL0_RX_GCSM2_CTRL);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_RX_PERGCSM_CTRL,
					   PLL0_RX_PERGCSM_CTRL);
		} else if (port == 1) {
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_CLK_SEL, tmp);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_HSCLK1_SEL, HSCLK_LINK_1);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_HSCLK1_DIV, tmp1);

			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLLDRC1_CTRL, PLLDRC_LINK1);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_DSM_M0, PLL1_DSM_M0);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_VCOCAL_START,
					   PLL1_VCOCAL_START);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_VCOCAL_CTRL,
					   PLL1_VCOCAL_CTRL);

			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_CP_PADJ,
					   vco_val[0][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_CP_IADJ,
					   vco_val[1][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_CP_FILT_PADJ,
					   vco_val[2][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_INTDIV,
					   vco_val[3][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_FRACDIVL,
					   vco_val[4][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_FRACDIVH,
					   vco_val[5][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_HIGH_THR,
					   vco_val[6][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_PDIAG_CTRL,
					   vco_val[7][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_VCOCAL_PLLCNT_START,
					   vco_val[8][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_LOCK_PEFCNT,
					   vco_val[9][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_LOCK_PLLCNT_START,
					   vco_val[10][index]);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_LOCK_PLLCNT_THR,
					   vco_val[11][index]);

			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_TX_PSC_A0,
					   PLL1_TX_PSC_A0);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_TX_PSC_A2,
					   PLL1_TX_PSC_A2);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_TX_PSC_A3,
					   PLL1_TX_PSC_A3);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_RX_PSC_A0,
					   PLL1_RX_PSC_A0);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_RX_PSC_A2,
					   PLL1_RX_PSC_A2);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_RX_PSC_A3,
					   PLL1_RX_PSC_A3);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_RX_PSC_CAL,
					   PLL1_RX_PSC_CAL);

			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_XCVR_CTRL,
					   PLL1_XCVR_CTRL);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_RX_GCSM1_CTRL,
					   PLL1_RX_GCSM1_CTRL);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_RX_GCSM2_CTRL,
					   PLL1_RX_GCSM2_CTRL);
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_RX_PERGCSM_CTRL,
					   PLL1_RX_PERGCSM_CTRL);
		}

		/* pma pll enable */
		data = 0;
		mask = 0;
		mask = (CONTROL_ENABLE << (port*CONTROL_ENABLE_SHIFT));
		data = (CONTROL_ENABLE << (port*CONTROL_ENABLE_SHIFT));
		tmp = x100_phy_readl(phytium_dp, PHYTIUM_PHY0_PMA_CONTROL);
		tmp = (tmp & (~mask)) | data;
		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PMA_CONTROL, tmp);

		/* lane pll enable */
		data = 0;
		mask = 0;
		for (i = 0; i < phytium_dp->source_max_lane_count; i++) {
			data |= (PLL_EN << i*PLL_EN_SHIFT);
			mask |= (((1<<PLL_EN_SHIFT) - 1) << (i*PLL_EN_SHIFT));
		}
		mask = (mask << (port*PLL_EN_SHIFT*4));
		data = (data << (port*PLL_EN_SHIFT*4));
		tmp = x100_phy_readl(phytium_dp, PHYTUIM_PHY0_PLL_EN);
		tmp = (tmp & (~mask)) | data;
		x100_phy_writel(phytium_dp, PHYTUIM_PHY0_PLL_EN, tmp);

		/* set pma power active */
		data = 0;
		mask = 0;
		for (i = 0; i < phytium_dp->source_max_lane_count; i++) {
			data |= (A0_ACTIVE << i*A0_ACTIVE_SHIFT);
			mask |= (((1<<A0_ACTIVE_SHIFT) - 1) << (i*A0_ACTIVE_SHIFT));
		}
		if (port == 0) {
			tmp = x100_phy_readl(phytium_dp, PHYTIUM_PHY0_PMA0_POWER);
			tmp = (tmp & (~mask)) | data;
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PMA0_POWER, tmp);
		} else {
			tmp = x100_phy_readl(phytium_dp, PHYTIUM_PHY0_PMA1_POWER);
			tmp = (tmp & (~mask)) | data;
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PMA1_POWER, tmp);
		}
	} else {
		/* set pma powerdown */
		mask = PHY1_A3_POWERDOWN3_MASK << PHY1_A3_POWERDOWN3_SHIFT;
		data = PHY1_A3_POWERDOWN3 << PHY1_A3_POWERDOWN3_SHIFT;
		tmp = x100_phy_readl(phytium_dp, PHYTIUM_PHY1_PMA_MISC);
		tmp = (tmp & (~mask)) | data;
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PMA_MISC, tmp);

		/* lane pll disable */
		mask = PHY1_PLL_EN_MASK << PHY1_PLL_EN_SHIFT;
		data = PHY1_PLL_EN << PHY1_PLL_EN_SHIFT;
		tmp = x100_phy_readl(phytium_dp, PHYTIUM_PHY1_PMA_MISC);
		tmp = (tmp & (~mask));
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PMA_MISC, tmp);

		/* pma pll disable */
		mask = (CONTROL_ENABLE << 0*CONTROL_ENABLE_SHIFT); // link config
		tmp = x100_phy_readl(phytium_dp, PHYTIUM_PHY1_PMA_CONTROL);
		tmp = (tmp & (~mask));
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PMA_CONTROL, tmp);

		/* read pma pll disable state */
		mdelay(2);
		x100_phy_readl(phytium_dp, PHYTIUM_PHY1_PMA_CONTROL2);

		/* config link rate */
		switch (link_rate) {
		case 810000:
			tmp = PLL_LINK_RATE_810000;
			tmp1 = HSCLK_LINK_RATE_810000;
			index = 3;
			break;
		case 540000:
			tmp = PLL_LINK_RATE_540000;
			tmp1 = HSCLK_LINK_RATE_540000;
			index = 2;
			break;
		case 270000:
			tmp = PLL_LINK_RATE_270000;
			tmp1 = HSCLK_LINK_RATE_270000;
			index = 1;
			break;
		case 162000:
			tmp = PLL_LINK_RATE_162000;
			tmp1 = HSCLK_LINK_RATE_162000;
			index = 0;
			break;
		default:
			DRM_ERROR("phytium dp rate(%d) not support\n", link_rate);
			tmp = PLL_LINK_RATE_162000;
			tmp1 = HSCLK_LINK_RATE_162000;
			index = 0;
			break;
		}

		/* config analog pll for link0 */
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL_CLK_SEL, tmp);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_HSCLK_SEL, HSCLK_LINK_0);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_HSCLK_DIV, tmp1);

		/* config digital pll for link0 */
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLLDRC_CTRL, PLLDRC_LINK0);

		/* common for all rate */
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_DSM_M0, PLL0_DSM_M0);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_VCOCAL_START, PLL0_VCOCAL_START);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_VCOCAL_CTRL, PLL0_VCOCAL_CTRL);

		/* different for all rate */
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_CP_PADJ, vco_val[0][index]);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_CP_IADJ, vco_val[1][index]);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_CP_FILT_PADJ, vco_val[2][index]);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_INTDIV, vco_val[3][index]);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_FRACDIVL, vco_val[4][index]);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_FRACDIVH, vco_val[5][index]);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_HIGH_THR, vco_val[6][index]);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_PDIAG_CTRL, vco_val[7][index]);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_VCOCAL_PLLCNT_START,
				   vco_val[8][index]);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_LOCK_PEFCNT, vco_val[9][index]);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_LOCK_PLLCNT_START,
				   vco_val[10][index]);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_LOCK_PLLCNT_THR,
				   vco_val[11][index]);

		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_TX_PSC_A0, PLL0_TX_PSC_A0);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_TX_PSC_A2, PLL0_TX_PSC_A2);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_TX_PSC_A3, PLL0_TX_PSC_A3);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_RX_PSC_A0, PLL0_RX_PSC_A0);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_RX_PSC_A2, PLL0_RX_PSC_A2);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_RX_PSC_A3, PLL0_RX_PSC_A3);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_RX_PSC_CAL, PLL0_RX_PSC_CAL);

		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_XCVR_CTRL, PLL0_XCVR_CTRL);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_RX_GCSM1_CTRL,
				   PLL0_RX_GCSM1_CTRL);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_RX_GCSM2_CTRL,
				   PLL0_RX_GCSM2_CTRL);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_RX_PERGCSM_CTRL,
				   PLL0_RX_PERGCSM_CTRL);

		/* pma pll enable */
		data = 0;
		mask = 0;
		mask = (CONTROL_ENABLE << (0*CONTROL_ENABLE_SHIFT));
		data = (CONTROL_ENABLE << (0*CONTROL_ENABLE_SHIFT));
		tmp = x100_phy_readl(phytium_dp, PHYTIUM_PHY1_PMA_CONTROL);
		tmp = (tmp & (~mask)) | data;
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PMA_CONTROL, tmp);

		/* lane pll enable */
		mask = PHY1_PLL_EN_MASK << PHY1_PLL_EN_SHIFT;
		data = PHY1_PLL_EN << PHY1_PLL_EN_SHIFT;
		tmp = x100_phy_readl(phytium_dp, PHYTIUM_PHY1_PMA_MISC);
		tmp = ((tmp & (~mask)) | data);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PMA_MISC, tmp);

		/* set pma power active */
		mask = PHY1_A0_ACTIVE_MASK << PHY1_A0_ACTIVE_SHIFT;
		data = PHY1_A0_ACTIVE << PHY1_A0_ACTIVE_SHIFT;
		tmp = x100_phy_readl(phytium_dp, PHYTIUM_PHY1_PMA_MISC);
		tmp = (tmp & (~mask)) | data;
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PMA_MISC, tmp);
	}
}

static void x100_dp_phy_set_lane_setting(struct phytium_dp_device *phytium_dp,
					 uint32_t link_rate,
					 uint8_t train_set)
{
	int port = phytium_dp->port%3;
	int voltage_swing = 0;
	int pre_emphasis = 0, link_rate_index = 0;

	switch (train_set & DP_TRAIN_VOLTAGE_SWING_MASK) {
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_0:
	default:
		voltage_swing = 0;
		break;
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_1:
		voltage_swing = 1;
		break;
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_2:
		voltage_swing = 2;
		break;
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_3:
		voltage_swing = 3;
		break;
	}
	switch (train_set & DP_TRAIN_PRE_EMPHASIS_MASK) {
	case DP_TRAIN_PRE_EMPH_LEVEL_0:
	default:
		pre_emphasis = 0;
		break;
	case DP_TRAIN_PRE_EMPH_LEVEL_1:
		pre_emphasis = 1;
		break;
	case DP_TRAIN_PRE_EMPH_LEVEL_2:
		pre_emphasis = 2;
		break;
	case DP_TRAIN_PRE_EMPH_LEVEL_3:
		pre_emphasis = 3;
		break;
	}

	switch (link_rate) {
	case 810000:
		link_rate_index = 3;
		break;
	case 540000:
		link_rate_index = 2;
		break;
	case 270000:
		link_rate_index = 1;
		break;
	case 162000:
		link_rate_index = 0;
		break;
	default:
		DRM_ERROR("phytium dp rate(%d) not support\n", link_rate);
		link_rate_index = 2;
		break;
	}

	if (port == 0) {
		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_TX_DIAG_ACYA, LOCK);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_TX_TXCC_CTRL, TX_TXCC_CTRL);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_TX_DRV, TX_DRV);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_TX_MGNFS,
				mgnfs_val[link_rate_index][voltage_swing][pre_emphasis]);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_TX_CPOST,
				cpost_val[link_rate_index][voltage_swing][pre_emphasis]);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL0_TX_DIAG_ACYA, UNLOCK);

	} else if (port == 1) {
		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_TX_DIAG_ACYA, LOCK);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_TX_TXCC_CTRL, TX_TXCC_CTRL);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_TX_DRV, TX_DRV);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_TX_MGNFS,
				mgnfs_val[link_rate_index][voltage_swing][pre_emphasis]);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_TX_CPOST,
				cpost_val[link_rate_index][voltage_swing][pre_emphasis]);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_TX_CPOST1,
				cpost_val[link_rate_index][voltage_swing][pre_emphasis]);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL1_TX_DIAG_ACYA, UNLOCK);
	} else {
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_TX_DIAG_ACYA, LOCK);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_TX_TXCC_CTRL, TX_TXCC_CTRL);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_TX_DRV, TX_DRV);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_TX_MGNFS,
				mgnfs_val[link_rate_index][voltage_swing][pre_emphasis]);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_TX_CPOST,
				cpost_val[link_rate_index][voltage_swing][pre_emphasis]);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL0_TX_DIAG_ACYA, UNLOCK);
	}
}

static void x100_dp_phy_init(struct phytium_dp_device *phytium_dp)
{
	int port = phytium_dp->port;
	int i = 0, data, tmp, mask;

	if (port == 0 || port == 1) {
		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_APB_RESET, APB_RESET);

		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PIPE_RESET, RESET);

		/* config lane to dp mode */
		data = 0;
		mask = 0;
		for (i = 0; i < phytium_dp->source_max_lane_count; i++) {
			data |= (LANE_BIT << i*LANE_BIT_SHIFT);
			mask |= (((1<<LANE_BIT_SHIFT) - 1) << (i*LANE_BIT_SHIFT));
		}
		mask = (mask << (port*LANE_BIT_SHIFT*4));
		data = (data << (port*LANE_BIT_SHIFT*4));
		tmp = x100_phy_readl(phytium_dp, PHYTIUM_PHY0_MODE);
		tmp = (tmp & (~mask)) | data;
		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_MODE, tmp);

		/* config lane master or slave */
		data = 0;
		mask = 0;
		for (i = 0; i < phytium_dp->source_max_lane_count; i++) {
			data |= (LANE_MASTER << i*LANE_MASTER_SHIFT);
			mask |= (((1<<LANE_MASTER_SHIFT) - 1) << (i*LANE_MASTER_SHIFT));
		}
		mask = (mask << (port*LANE_MASTER_SHIFT*4));
		data = (data << (port*LANE_MASTER_SHIFT*4));
		tmp = x100_phy_readl(phytium_dp, PHYTIUM_PHY0_LINK_CFG);
		tmp = (tmp & (~mask)) | data;
		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_LINK_CFG, tmp);

		/* pll clock enable */
		data = 0;
		mask = 0;
		for (i = 0; i < phytium_dp->source_max_lane_count; i++) {
			data |= (PLL_EN << i*PLL_EN_SHIFT);
			mask |= (((1<<PLL_EN_SHIFT) - 1) << (i*PLL_EN_SHIFT));
		}
		mask = (mask << (port*PLL_EN_SHIFT*4));
		data = (data << (port*PLL_EN_SHIFT*4));
		tmp = x100_phy_readl(phytium_dp, PHYTUIM_PHY0_PLL_EN);
		tmp = (tmp & (~mask)) | data;
		x100_phy_writel(phytium_dp, PHYTUIM_PHY0_PLL_EN, tmp);

		/* config input 20 bit */
		data = 0;
		mask = 0;
		for (i = 0; i < phytium_dp->source_max_lane_count; i++) {
			data |= (BIT_20 << i*BIT_20_SHIFT);
			mask |= (((1<<BIT_20_SHIFT) - 1) << (i*BIT_20_SHIFT));
		}
		mask = (mask << (port*BIT_20_SHIFT*4));
		data = (data << (port*BIT_20_SHIFT*4));
		tmp = x100_phy_readl(phytium_dp, PHYTUIM_PHY0_PMA_WIDTH);
		tmp = (tmp & (~mask)) | data;
		x100_phy_writel(phytium_dp, PHYTUIM_PHY0_PMA_WIDTH, tmp);

		/* config lane active power state */
		data = 0;
		mask = 0;
		for (i = 0; i < phytium_dp->source_max_lane_count; i++) {
			data |= (A0_ACTIVE << i*A0_ACTIVE_SHIFT);
			mask |= (((1<<A0_ACTIVE_SHIFT) - 1) << (i*A0_ACTIVE_SHIFT));
		}
		if (port == 0) {
			tmp = x100_phy_readl(phytium_dp, PHYTIUM_PHY0_PMA0_POWER);
			tmp = (tmp & (~mask)) | data;
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PMA0_POWER, tmp);
		} else {
			tmp = x100_phy_readl(phytium_dp, PHYTIUM_PHY0_PMA1_POWER);
			tmp = (tmp & (~mask)) | data;
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PMA1_POWER, tmp);
		}

		/* link reset */
		mask = (LINK_RESET_MASK << (0*LINTK_RESET_SHIFT)) |
			(LINK_RESET_MASK << (1*LINTK_RESET_SHIFT));
		data = (LINK_RESET << (0*LINTK_RESET_SHIFT)) |
			(LINK_RESET << (1*LINTK_RESET_SHIFT));
		tmp = (data & mask);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_LINK_RESET, tmp);

		/* config double link */
		if (port == 0)
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL_CFG, SINGLE_LINK);
		else if (port == 1)
			x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PLL_CFG, DOUBLE_LINK);

		/* pipe reset */
		x100_phy_writel(phytium_dp, PHYTIUM_PHY0_PIPE_RESET, RESET_DEASSERT);
	} else {
		/* APB reset */
		tmp = x100_phy_readl(phytium_dp, PHYTIUM_PHY1_APB_RESET);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_APB_RESET, tmp & (~PHY1_APB_RESET));
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_APB_RESET, tmp | PHY1_APB_RESET);

		/* pipe reset */
		tmp = x100_phy_readl(phytium_dp, PHYTIUM_PHY1_PIPE_RESET);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PIPE_RESET,
				   tmp & (~PHY1_PIPE_RESET_DEASSERT));

		/* config MODE/SEL to dp */
		mask = PHY1_DP_LANE_BIT << PHY1_DP_LANE_BIT_SHIFT;
		data = PHY1_DP_LANE_BIT << PHY1_DP_LANE_BIT_SHIFT;
		tmp = x100_phy_readl(phytium_dp, PHYTIUM_PHY1_SEL);
		tmp = (tmp & (~mask));
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_SEL, tmp);

		/* config pll clock enable,  input 20 bit, lane active power state */
		tmp = (PHY1_PLL_EN << PHY1_PLL_EN_SHIFT) | (PHY1_BIT_20 << PHY1_BIT_20_SHIFT)
			| (PHY1_A0_ACTIVE << PHY1_A0_ACTIVE_SHIFT);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PMA_MISC, tmp);

		/* config single link */
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PLL_CFG, SINGLE_LINK);

		/* pipe reset */
		tmp = x100_phy_readl(phytium_dp, PHYTIUM_PHY1_PIPE_RESET);
		x100_phy_writel(phytium_dp, PHYTIUM_PHY1_PIPE_RESET,
				   tmp | PHY1_PIPE_RESET_DEASSERT);
	}
}

static void x100_dp_hw_poweron_panel(struct phytium_dp_device *phytium_dp)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;
	int timeout = 100, config = 0;

	phytium_writel_reg(priv, FLAG_REQUEST | CMD_BACKLIGHT | PANEL_POWER_ENABLE,
			   PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(port));

	do {
		mdelay(10);
		timeout--;
		config = phytium_readl_reg(priv, PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(port));
	} while ((!(config & FLAG_REPLY)) && timeout);
	if (timeout == 0)
		DRM_ERROR("%s failed\n", __func__);

	phytium_writel_reg(priv, 0,
			   PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(port));
	mdelay(20);
}

static void x100_dp_hw_poweroff_panel(struct phytium_dp_device *phytium_dp)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;
	int timeout = 100, config = 0;

	phytium_writel_reg(priv, FLAG_REQUEST | CMD_BACKLIGHT | PANEL_POWER_DISABLE,
			   PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(port));

	do {
		mdelay(10);
		timeout--;
		config = phytium_readl_reg(priv, PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(port));
	} while ((!(config & FLAG_REPLY)) && timeout);
	if (timeout == 0)
		DRM_ERROR("%s failed\n", __func__);

	phytium_writel_reg(priv, 0,
			   PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(port));
	mdelay(20);
}

static void x100_dp_hw_enable_backlight(struct phytium_dp_device *phytium_dp)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;
	int timeout = 100, config = 0;

	phytium_writel_reg(priv, FLAG_REQUEST | CMD_BACKLIGHT | BACKLIGHT_ENABLE,
			   PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(port));

	do {
		mdelay(10);
		timeout--;
		config = phytium_readl_reg(priv, PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(port));
	} while ((!(config & FLAG_REPLY)) && timeout);
	if (timeout == 0)
		DRM_ERROR("%s failed\n", __func__);

	phytium_writel_reg(priv, 0,
			   PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(port));
	mdelay(20);
}

static void x100_dp_hw_disable_backlight(struct phytium_dp_device *phytium_dp)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;
	int timeout = 100, config = 0;

	phytium_writel_reg(priv, FLAG_REQUEST | CMD_BACKLIGHT | BACKLIGHT_DISABLE,
			   PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(port));

	do {
		mdelay(10);
		timeout--;
		config = phytium_readl_reg(priv, PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(port));
	} while ((!(config & FLAG_REPLY)) && timeout);
	if (timeout == 0)
		DRM_ERROR("%s failed\n", __func__);

	phytium_writel_reg(priv, 0,
			   PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(port));
	mdelay(20);
}

static uint32_t x100_dp_hw_get_backlight(struct phytium_dp_device *phytium_dp)
{
	return phytium_dp->panel.level;
}

static int x100_dp_hw_set_backlight(struct phytium_dp_device *phytium_dp, uint32_t level)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;
	int timeout = 100, config = 0;
	int ret = 0;

	if (level > BACKLIGHT_MAX) {
		ret = -EINVAL;
		goto out;
	}

	config = FLAG_REQUEST | CMD_BACKLIGHT | ((level & BACKLIGHT_MASK) << BACKLIGHT_SHIFT);
	phytium_writel_reg(priv, config, PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(port));

	do {
		mdelay(10);
		timeout--;
		config = phytium_readl_reg(priv, PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(port));
	} while ((!(config & FLAG_REPLY)) && timeout);
	if (timeout == 0) {
		DRM_ERROR("%s failed\n", __func__);
		ret = -EIO;
	}

	phytium_writel_reg(priv, 0,
			   PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(port));
	mdelay(20);

out:
	return ret;
}

static void x100_dp_hw_set_test_pattern(struct phytium_dp_device *phytium_dp,
						     uint8_t lane_count,
						     uint8_t test_pattern,
						     uint8_t *custom_pattern,
						     uint32_t custom_pattern_size)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port, val = 0, tmp = 0, i;

	if ((test_pattern == PHYTIUM_PHY_TP_80BIT_CUSTOM)
	    && custom_pattern && (custom_pattern_size > 0)) {
		val = *(int *)custom_pattern;
		phytium_writel_reg(priv, val, PHYTIUM_DP_CUSTOM_80BIT_PATTERN_0(port));
		val = *(int *)(custom_pattern + 4);
		phytium_writel_reg(priv, val, PHYTIUM_DP_CUSTOM_80BIT_PATTERN_1(port));
		val = *(short int *)(custom_pattern + 8);
		phytium_writel_reg(priv, val, PHYTIUM_DP_CUSTOM_80BIT_PATTERN_2(port));
	}

	if (test_pattern == PHYTIUM_PHY_TP_D10_2 || test_pattern == PHYTIUM_PHY_TP_PRBS7
		|| test_pattern == PHYTIUM_PHY_TP_80BIT_CUSTOM)
		phytium_writel_reg(priv, SCRAMBLING_DISABLE, PHYTIUM_DP_SCRAMBLING_DISABLE(port));
	else
		phytium_writel_reg(priv, SCRAMBLING_ENABLE, PHYTIUM_DP_SCRAMBLING_DISABLE(port));

	tmp = test_pattern - PHYTIUM_PHY_TP_NONE + TEST_PATTERN_NONE;
	val = 0;
	for (i = 0; i < lane_count; i++)
		val |= (tmp << (TEST_PATTERN_LANE_SHIFT * i));
	phytium_writel_reg(priv, val, PHYTIUM_DP_LINK_QUAL_PATTERN_SET(port));
}

static void x100_dp_hw_set_train_pattern(struct phytium_dp_device *phytium_dp,
					    uint8_t train_pattern)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port, tmp = 0;

	/* Scrambling is disabled for TPS1/TPS2/3 and enabled for TPS4 */
	if (train_pattern == DP_TRAINING_PATTERN_4
	    || train_pattern == DP_TRAINING_PATTERN_DISABLE) {
		phytium_writel_reg(priv, SCRAMBLING_ENABLE, PHYTIUM_DP_SCRAMBLING_DISABLE(port));
		phytium_writel_reg(priv, SCRAMBLER_RESET, PHYTIUM_DP_FORCE_SCRAMBLER_RESET(port));
	} else {
		phytium_writel_reg(priv, SCRAMBLING_DISABLE, PHYTIUM_DP_SCRAMBLING_DISABLE(port));
	}
	switch (train_pattern) {
	case DP_TRAINING_PATTERN_DISABLE:
		tmp = TRAINING_OFF;
		break;
	case DP_TRAINING_PATTERN_1:
		tmp = TRAINING_PATTERN_1;
		break;
	case DP_TRAINING_PATTERN_2:
		tmp = TRAINING_PATTERN_2;
		break;
	case DP_TRAINING_PATTERN_3:
		tmp = TRAINING_PATTERN_3;
		break;
	case DP_TRAINING_PATTERN_4:
		tmp = TRAINING_PATTERN_4;
		break;
	default:
		tmp = TRAINING_OFF;
		break;
	}

	phytium_writel_reg(priv, tmp, PHYTIUM_DP_TRAINING_PATTERN_SET(port));
}

static void x100_dp_hw_set_link(struct phytium_dp_device *phytium_dp,
					 uint8_t link_lane_count, uint32_t link_rate)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;

	phytium_writel_reg(priv, link_lane_count,
			   PHYTIUM_DP_LANE_COUNT_SET(port));
	phytium_writel_reg(priv,
			   drm_dp_link_rate_to_bw_code(link_rate),
			   PHYTIUM_DP_LINK_BW_SET(port));

	if (drm_dp_enhanced_frame_cap(phytium_dp->dpcd))
		phytium_writel_reg(priv, ENHANCED_FRAME_ENABLE,
				   PHYTIUM_DP_ENHANCED_FRAME_EN(port));
	else
		phytium_writel_reg(priv, ENHANCED_FRAME_DISABLE,
				   PHYTIUM_DP_ENHANCED_FRAME_EN(port));
	x100_dp_phy_set_lane_and_rate(phytium_dp, link_lane_count, link_rate);
}

void x100_dp_hw_set_lane_setting(struct phytium_dp_device *phytium_dp,
				 uint32_t link_rate, uint8_t train_set)
{
	x100_dp_phy_set_lane_setting(phytium_dp, link_rate, train_set);
}

static int X100_rate[] = {162000, 270000, 540000, 810000};

static void x100_dp_set_source_rate_and_lane_count(struct phytium_dp_device *phytium_dp)
{
	phytium_dp->source_rates = X100_rate;
	phytium_dp->num_source_rates = num_source_rates;

	if (phytium_dp->port == 0)
		phytium_dp->source_max_lane_count = source_max_lane_count;
	else if (phytium_dp->port == 1)
		phytium_dp->source_max_lane_count = source_max_lane_count;
	else if (phytium_dp->port == 2)
		phytium_dp->source_max_lane_count = 1;
	else
		phytium_dp->source_max_lane_count = 1;
}

void x100_dp_hw_get_hpd_state(struct phytium_dp_device *phytium_dp)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;
	uint32_t val = 0, raw_state = 0;

	val = phytium_readl_reg(priv, PHYTIUM_DP_INTERRUPT_RAW_STATUS(port));

	/* maybe miss hpd, so used for clear PHYTIUM_DP_INTERRUPT_RAW_STATUS */
	phytium_readl_reg(priv, PHYTIUM_DP_INTERRUPT_STATUS(port));
	raw_state = phytium_readl_reg(priv, PHYTIUM_DP_SINK_HPD_STATE(port));
	if (val & HPD_EVENT)
		phytium_dp->dp_hpd_state.hpd_event_state = true;

	if (val & HPD_IRQ)
		phytium_dp->dp_hpd_state.hpd_irq_state = true;

	if (raw_state & HPD_CONNECT)
		phytium_dp->dp_hpd_state.hpd_raw_state = true;
	else
		phytium_dp->dp_hpd_state.hpd_raw_state = false;
}

void x100_dp_hw_hpd_irq_setup(struct phytium_dp_device *phytium_dp, bool enable)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;

	phytium_dp->dp_hpd_state.hpd_irq_enable = enable;
	if (enable)
		phytium_writel_reg(priv, HPD_OTHER_MASK, PHYTIUM_DP_INTERRUPT_MASK(port));
	else
		phytium_writel_reg(priv, HPD_IRQ_MASK|HPD_EVENT_MASK|HPD_OTHER_MASK,
				   PHYTIUM_DP_INTERRUPT_MASK(port));
}

void x100_dp_hw_disable_audio(struct phytium_dp_device *phytium_dp)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;

	phytium_writel_reg(priv, SECONDARY_STREAM_DISABLE,
				   PHYTIUM_DP_SECONDARY_STREAM_ENABLE(port));
}

void x100_dp_hw_enable_audio(struct phytium_dp_device *phytium_dp)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;

	phytium_writel_reg(priv, SECONDARY_STREAM_ENABLE,
			   PHYTIUM_DP_SECONDARY_STREAM_ENABLE(port));

}

static void x100_dp_hw_audio_shutdown(struct phytium_dp_device *phytium_dp)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;

	phytium_writel_reg(priv, SEC_AUDIO_DISABLE, PHYTIUM_DP_SEC_AUDIO_ENABLE(port));
}

static int x100_dp_hw_audio_digital_mute(struct phytium_dp_device *phytium_dp)
{
	struct phytium_display_drm_private *priv = phytium_dp->dev->dev_private;
	int port = phytium_dp->port;
	int ret = 0;

	if (phytium_readl_reg(priv, PHYTIUM_DP_SECONDARY_STREAM_ENABLE(port)) == 0) {
		ret = -ENODEV;
		goto out;
	}
	phytium_writel_reg(priv, CHANNEL_MUTE, PHYTIUM_DP_SEC_CHANNEL_COUNT(port));

out:
	return ret;
}

int x100_dp_hw_audio_hw_params(struct phytium_dp_device *phytium_dp, struct audio_info audio_info)
{
	struct phytium_display_drm_private *priv = phytium_dp->dev->dev_private;
	int port = phytium_dp->port;
	int ret = 0, data_window = 0;
	const struct dp_audio_n_m *n_m = NULL;
	uint32_t fs, ws, fs_accurac;

	DRM_DEBUG_KMS("%s:set port%d sample_rate(%d) channels(%d) sample_width(%d)\n",
			__func__, phytium_dp->port, audio_info.sample_rate,
			audio_info.channels, audio_info.sample_width);

	phytium_writel_reg(priv, INPUT_SELECT_I2S, PHYTIUM_DP_SEC_INPUT_SELECT(port));
	phytium_writel_reg(priv, APB_CLOCK/audio_info.sample_rate,
			   PHYTIUM_DP_SEC_DIRECT_CLKDIV(port));
	phytium_writel_reg(priv, audio_info.channels & CHANNEL_MASK,
			   PHYTIUM_DP_SEC_CHANNEL_COUNT(port));
	phytium_writel_reg(priv, CHANNEL_MAP_DEFAULT, PHYTIUM_DP_SEC_CHANNEL_MAP(port));
	data_window = 90*(phytium_dp->link_rate)/100
			*(phytium_dp->mode.htotal - phytium_dp->mode.hdisplay)
			/phytium_dp->mode.clock/4;
	phytium_writel_reg(priv, data_window, PHYTIUM_DP_SEC_DATA_WINDOW(port));
	phytium_writel_reg(priv, 0xb5, PHYTIUM_DP_SEC_CS_CATEGORY_CODE(port));
	n_m = phytium_dp_audio_get_n_m(phytium_dp->link_rate, audio_info.sample_rate);
	if (n_m == NULL) {
		DRM_NOTE("can not get n_m for link_rate(%d) and sample_rate(%d)\n",
			   phytium_dp->link_rate, audio_info.sample_rate);
		goto out;
	}

	phytium_writel_reg(priv, n_m->m, PHYTIUM_DP_SEC_MAUD(port));
	phytium_writel_reg(priv, n_m->n, PHYTIUM_DP_SEC_NAUD(port));
	phytium_writel_reg(priv, CLOCK_MODE_SYNC, PHYTIUM_DP_SEC_CLOCK_MODE(port));
	phytium_writel_reg(priv, CS_SOURCE_FORMAT_DEFAULT,
			   PHYTIUM_DP_SEC_CS_SOURCE_FORMAT(port));

	switch (audio_info.sample_rate) {
	case 32000:
		fs = ORIG_FREQ_32000;
		fs_accurac = SAMPLING_FREQ_32000;
		break;
	case 44100:
		fs = ORIG_FREQ_44100;
		fs_accurac = SAMPLING_FREQ_44100;
		break;
	case 48000:
		fs = ORIG_FREQ_48000;
		fs_accurac = SAMPLING_FREQ_48000;
		break;
	case 96000:
		fs = ORIG_FREQ_96000;
		fs_accurac = SAMPLING_FREQ_96000;
		break;
	case 176400:
		fs = ORIG_FREQ_176400;
		fs_accurac = SAMPLING_FREQ_176400;
		break;
	case 192000:
		fs = ORIG_FREQ_192000;
		fs_accurac = SAMPLING_FREQ_192000;
		break;
	default:
		DRM_ERROR("dp not support sample_rate %d\n", audio_info.sample_rate);
		goto out;
	}

	switch (audio_info.sample_width) {
	case 16:
		ws = WORD_LENGTH_16;
		break;
	case 18:
		ws = WORD_LENGTH_18;
		break;
	case 20:
		ws = WORD_LENGTH_20;
		break;
	case 24:
		ws = WORD_LENGTH_24;
		break;
	default:
		DRM_ERROR("dp not support sample_width %d\n", audio_info.sample_width);
		goto out;
	}

	phytium_writel_reg(priv, ((fs&ORIG_FREQ_MASK)<<ORIG_FREQ_SHIFT)
			   | ((ws&WORD_LENGTH_MASK) << WORD_LENGTH_SHIFT),
			   PHYTIUM_DP_SEC_CS_LENGTH_ORIG_FREQ(port));
	phytium_writel_reg(priv, (fs_accurac&SAMPLING_FREQ_MASK) << SAMPLING_FREQ_SHIFT,
			    PHYTIUM_DP_SEC_CS_FREQ_CLOCK_ACCURACY(port));
	phytium_writel_reg(priv, SEC_AUDIO_ENABLE, PHYTIUM_DP_SEC_AUDIO_ENABLE(port));
	phytium_dp->audio_info = audio_info;

	return 0;

out:
	phytium_writel_reg(priv, SEC_AUDIO_DISABLE, PHYTIUM_DP_SEC_AUDIO_ENABLE(port));
	return ret;
}

void x100_dp_hw_disable_video(struct phytium_dp_device *phytium_dp)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;

	phytium_writel_reg(priv, SST_MST_SOURCE_0_DISABLE,
			   PHYTIUM_DP_VIDEO_STREAM_ENABLE(port));
}

void x100_dp_hw_enable_video(struct phytium_dp_device *phytium_dp)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;

	phytium_writel_reg(priv, SST_MST_SOURCE_0_ENABLE,
			   PHYTIUM_DP_VIDEO_STREAM_ENABLE(port));
	phytium_writel_reg(priv, LINK_SOFT_RESET, PHYTIUM_DP_SOFT_RESET(port));
}

bool x100_dp_hw_video_is_enable(struct phytium_dp_device *phytium_dp)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;

	return phytium_readl_reg(priv, PHYTIUM_DP_VIDEO_STREAM_ENABLE(port)) ? true : false;
}

void x100_dp_hw_config_video(struct phytium_dp_device *phytium_dp)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;
	unsigned long link_bw, date_rate = 0;
	struct drm_display_info *display_info = &phytium_dp->connector.display_info;
	unsigned char tu_size = 64;
	unsigned long data_per_tu = 0;
	int symbols_per_tu, frac_symbols_per_tu, symbol_count, udc, value;

	/* cal M/N and tu_size */
	phytium_writel_reg(priv, phytium_dp->mode.crtc_clock/10, PHYTIUM_DP_M_VID(port));
	phytium_writel_reg(priv, phytium_dp->link_rate/10, PHYTIUM_DP_N_VID(port));
	link_bw = phytium_dp->link_rate * phytium_dp->link_lane_count;
	date_rate = (phytium_dp->mode.crtc_clock * display_info->bpc * 3)/8;

	/* mul 10 for register setting */
	data_per_tu = 10*tu_size * date_rate/link_bw;
	symbols_per_tu = (data_per_tu/10)&0xff;
	frac_symbols_per_tu = (data_per_tu%10*16/10) & 0xf;
	phytium_writel_reg(priv, frac_symbols_per_tu<<24 | symbols_per_tu<<16 | tu_size,
			   PHYTIUM_DP_TRANSFER_UNIT_SIZE(port));

	symbol_count = (phytium_dp->mode.crtc_hdisplay*display_info->bpc*3 + 7)/8;
	udc = (symbol_count + phytium_dp->link_lane_count - 1)/phytium_dp->link_lane_count;
	phytium_writel_reg(priv, udc, PHYTIUM_DP_DATA_COUNT(port));

	/* config main stream attributes */
	phytium_writel_reg(priv, phytium_dp->mode.crtc_htotal,
			   PHYTIUM_DP_MAIN_LINK_HTOTAL(port));
	phytium_writel_reg(priv, phytium_dp->mode.crtc_hdisplay,
			   PHYTIUM_DP_MAIN_LINK_HRES(port));
	phytium_writel_reg(priv,
			   phytium_dp->mode.crtc_hsync_end - phytium_dp->mode.crtc_hsync_start,
			   PHYTIUM_DP_MAIN_LINK_HSWIDTH(port));
	phytium_writel_reg(priv, phytium_dp->mode.crtc_htotal - phytium_dp->mode.crtc_hsync_start,
			   PHYTIUM_DP_MAIN_LINK_HSTART(port));
	phytium_writel_reg(priv, phytium_dp->mode.crtc_vtotal,
			   PHYTIUM_DP_MAIN_LINK_VTOTAL(port));
	phytium_writel_reg(priv, phytium_dp->mode.crtc_vdisplay,
			   PHYTIUM_DP_MAIN_LINK_VRES(port));
	phytium_writel_reg(priv,
			   phytium_dp->mode.crtc_vsync_end - phytium_dp->mode.crtc_vsync_start,
			   PHYTIUM_DP_MAIN_LINK_VSWIDTH(port));
	phytium_writel_reg(priv, phytium_dp->mode.crtc_vtotal - phytium_dp->mode.crtc_vsync_start,
			   PHYTIUM_DP_MAIN_LINK_VSTART(port));

	value = 0;
	if (phytium_dp->mode.flags & DRM_MODE_FLAG_PHSYNC)
		value = value & (~HSYNC_POLARITY_LOW);
	else
		value = value | HSYNC_POLARITY_LOW;

	if (phytium_dp->mode.flags & DRM_MODE_FLAG_PVSYNC)
		value = value & (~VSYNC_POLARITY_LOW);
	else
		value = value | VSYNC_POLARITY_LOW;
	phytium_writel_reg(priv, value, PHYTIUM_DP_MAIN_LINK_POLARITY(port));

	switch (display_info->bpc) {
	case 10:
		value = (MISC0_BIT_DEPTH_10BIT << MISC0_BIT_DEPTH_OFFSET);
		break;
	case 6:
		value = (MISC0_BIT_DEPTH_6BIT << MISC0_BIT_DEPTH_OFFSET);
		break;
	default:
		value = (MISC0_BIT_DEPTH_8BIT << MISC0_BIT_DEPTH_OFFSET);
		break;
	}
	value |= (MISC0_COMPONENT_FORMAT_RGB << MISC0_COMPONENT_FORMAT_SHIFT)
		| MISC0_SYNCHRONOUS_CLOCK;
	phytium_writel_reg(priv, value, PHYTIUM_DP_MAIN_LINK_MISC0(port));
	phytium_writel_reg(priv, 0, PHYTIUM_DP_MAIN_LINK_MISC1(port));

	value = USER_ODDEVEN_POLARITY_HIGH | USER_DATA_ENABLE_POLARITY_HIGH;
	if (phytium_dp->mode.flags & DRM_MODE_FLAG_PHSYNC)
		value = value | USER_HSYNC_POLARITY_HIGH;
	else
		value = value & (~USER_HSYNC_POLARITY_HIGH);
	if (phytium_dp->mode.flags & DRM_MODE_FLAG_PVSYNC)
		value = value | USER_VSYNC_POLARITY_HIGH;
	else
		value = value & (~USER_VSYNC_POLARITY_HIGH);
	phytium_writel_reg(priv, value, PHYTIUM_DP_USER_SYNC_POLARITY(port));
}

void x100_dp_hw_disable_output(struct phytium_dp_device *phytium_dp)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;

	phytium_writel_reg(priv, TRANSMITTER_OUTPUT_DISABLE,
			   PHYTIUM_DP_TRANSMITTER_OUTPUT_ENABLE(port));
	phytium_writel_reg(priv, LINK_SOFT_RESET, PHYTIUM_DP_SOFT_RESET(port));
}

void x100_dp_hw_enable_output(struct phytium_dp_device *phytium_dp)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;

	phytium_writel_reg(priv, LINK_SOFT_RESET, PHYTIUM_DP_SOFT_RESET(port));
	phytium_writel_reg(priv, TRANSMITTER_OUTPUT_ENABLE,
			   PHYTIUM_DP_TRANSMITTER_OUTPUT_ENABLE(port));
}

void x100_dp_hw_enable_input_source(struct phytium_dp_device *phytium_dp)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;

	phytium_writel_reg(priv, VIRTUAL_SOURCE_0_ENABLE,
			   PHYTIUM_INPUT_SOURCE_ENABLE(port));
}

void x100_dp_hw_disable_input_source(struct phytium_dp_device *phytium_dp)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;

	phytium_writel_reg(priv, (~VIRTUAL_SOURCE_0_ENABLE)&VIRTUAL_SOURCE_0_ENABLE_MASK,
			   PHYTIUM_INPUT_SOURCE_ENABLE(port));
}

bool x100_dp_hw_output_is_enable(struct phytium_dp_device *phytium_dp)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;

	return phytium_readl_reg(priv, PHYTIUM_DP_TRANSMITTER_OUTPUT_ENABLE(port)) ? true : false;
}

void x100_dp_hw_init(struct phytium_dp_device *phytium_dp)
{
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;

	x100_dp_set_source_rate_and_lane_count(phytium_dp);
	x100_dp_phy_init(phytium_dp);
	phytium_writel_reg(priv, AUX_CLK_DIVIDER, PHYTIUM_DP_AUX_CLK_DIVIDER(port));
	phytium_dp->fast_train_support = false;
}

static struct phytium_dp_func x100_dp_funcs = {
	.dp_hw_init = x100_dp_hw_init,
	.dp_hw_enable_output = x100_dp_hw_enable_output,
	.dp_hw_disable_output = x100_dp_hw_disable_output,
	.dp_hw_output_is_enable = x100_dp_hw_output_is_enable,
	.dp_hw_enable_input_source = x100_dp_hw_enable_input_source,
	.dp_hw_disable_input_source = x100_dp_hw_disable_input_source,
	.dp_hw_get_hpd_state = x100_dp_hw_get_hpd_state,
	.dp_hw_hpd_irq_setup = x100_dp_hw_hpd_irq_setup,
	.dp_hw_set_test_pattern = x100_dp_hw_set_test_pattern,
	.dp_hw_set_link = x100_dp_hw_set_link,
	.dp_hw_set_lane_setting = x100_dp_hw_set_lane_setting,
	.dp_hw_set_train_pattern = x100_dp_hw_set_train_pattern,
	.dp_hw_disable_video = x100_dp_hw_disable_video,
	.dp_hw_enable_video = x100_dp_hw_enable_video,
	.dp_hw_video_is_enable = x100_dp_hw_video_is_enable,
	.dp_hw_config_video = x100_dp_hw_config_video,
	.dp_hw_enable_audio = x100_dp_hw_enable_audio,
	.dp_hw_disable_audio = x100_dp_hw_disable_audio,
	.dp_hw_audio_shutdown = x100_dp_hw_audio_shutdown,
	.dp_hw_audio_digital_mute = x100_dp_hw_audio_digital_mute,
	.dp_hw_audio_hw_params = x100_dp_hw_audio_hw_params,
	.dp_hw_enable_backlight = x100_dp_hw_enable_backlight,
	.dp_hw_disable_backlight = x100_dp_hw_disable_backlight,
	.dp_hw_get_backlight = x100_dp_hw_get_backlight,
	.dp_hw_set_backlight = x100_dp_hw_set_backlight,
	.dp_hw_poweron_panel = x100_dp_hw_poweron_panel,
	.dp_hw_poweroff_panel = x100_dp_hw_poweroff_panel,
};

void x100_dp_func_register(struct phytium_dp_device *phytium_dp)
{
	phytium_dp->funcs = &x100_dp_funcs;
}
