#include "gsgpu.h"
#include "gsgpu_dc.h"
#include "gsgpu_dc_vbios.h"
#include "lt9721_drv.h"

static bool lt9721_register_volatile(struct device *dev, u32 reg)
{
	switch (reg) {
	case LT9721_REG_PAGE_SELECT:
		return false;
	default:
		return true;
	}
}

static bool lt9721_register_readable(struct device *dev, u32 reg)
{
	switch (reg) {
	case 0x00:
		return false;
	case LT9721_REG_PAGE_SELECT:
	default:
		return true;
	}
}

static const struct regmap_range lt9721_rw_regs_range[] = {
	regmap_reg_range(LT9721_REG_START, LT9721_REG_END),
};

static const struct regmap_range lt9721_ro_regs_range[] = {
	regmap_reg_range(LT9721_REG_CHIP_VERSION_BASE,
			LT9721_REG_CHIP_VERSION_BASE + CHIP_VERSION_LEN),
};

static const struct regmap_range lt9721_vo_regs_range[] = {
	regmap_reg_range(LT9721_REG_START, LT9721_REG_END),
};

static const struct regmap_range_cfg lt9721_regmap_range_cfg[] = {
	{
		.name = "lt9721_registers",
		.range_min = LT9721_REG_START,
		.range_max = LT9721_REG_END,
		.window_start = LT9721_REG_START,
		.window_len = LT9721_REG_PAGE,
		.selector_reg = LT9721_REG_PAGE_SELECT,
		.selector_mask = 0xFF,
		.selector_shift = 0,
	},
};

static bool lt9721_chip_id_verify(struct gsgpu_bridge_phy *phy, char *str)
{
	struct lt9721_device *lt9721_dev;
	u8 version_val[2];

	regmap_bulk_read(phy->phy_regmap, LT9721_REG_CHIP_VERSION_BASE,
				version_val, CHIP_VERSION_LEN);
	if (version_val[0] != 0x16) {
		DRM_ERROR("Invalid lt9721 chip version {%02x}\n", version_val[0]);
		strcpy(str, "Unknown");
		return false;
	}

	phy->chip_version = version_val[1];
	strncpy(str, version_val, ARRAY_SIZE(version_val));

	lt9721_dev = phy->priv;
	if (version_val[1] == 0x06)
		lt9721_dev->ver = LT9721_VER_1;
	else
		lt9721_dev->ver = LT9721_VER_Unknown;

	DRM_INFO("Get chip version: LT9721_VER_U%d\n", lt9721_dev->ver);
	return true;
}

static enum hpd_status lt9721_get_hpd_status(struct gsgpu_bridge_phy *phy)
{
	u32 val;

	regmap_read(phy->phy_regmap, LT9721_REG_LINK_STATUS, &val);

	if (test_bit(LINK_STATUS_OUTPUT_DC_POS, (unsigned long *)&val) ==
			LINK_STATUS_STABLE) {
		return hpd_status_plug_on;
	}

	return hpd_status_plug_off;
}

static struct bridge_phy_misc_funcs lt9721_misc_funcs = {
	.chip_id_verify = lt9721_chip_id_verify,
};

static struct bridge_phy_hpd_funcs lt9721_hpd_funcs = {
	.get_hpd_status = lt9721_get_hpd_status,
};

static const struct regmap_access_table lt9721_read_table = {
	.yes_ranges = lt9721_rw_regs_range,
	.n_yes_ranges = ARRAY_SIZE(lt9721_rw_regs_range),
};

static const struct regmap_access_table lt9721_write_table = {
	.yes_ranges = lt9721_rw_regs_range,
	.n_yes_ranges = ARRAY_SIZE(lt9721_rw_regs_range),
	.no_ranges = lt9721_ro_regs_range,
	.n_no_ranges = ARRAY_SIZE(lt9721_ro_regs_range),
};

static const struct regmap_config lt9721_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.reg_stride = 1,
	.max_register = LT9721_REG_END,
	.ranges = lt9721_regmap_range_cfg,
	.num_ranges = ARRAY_SIZE(lt9721_regmap_range_cfg),

	.fast_io = false,
	.cache_type = REGCACHE_RBTREE,

	.volatile_reg = lt9721_register_volatile,
	.readable_reg = lt9721_register_readable,
	.rd_table = &lt9721_read_table,
	.wr_table = &lt9721_write_table,
};

int lt9721_hdmi_rx_cdr(struct gsgpu_bridge_phy *phy)
{
	regmap_multi_reg_write(phy->phy_regmap, lt9721_HDMIRxCDR,
			ARRAY_SIZE(lt9721_HDMIRxCDR));
	return 0;
}

int lt9721_rx_pll(struct gsgpu_bridge_phy *phy)
{
	regmap_multi_reg_write(phy->phy_regmap, lt9711_rx_pll_cfg,
				ARRAY_SIZE(lt9711_rx_pll_cfg));

	return 0;
}

static int lt9721_tx_phy(struct gsgpu_bridge_phy *phy)
{
	regmap_multi_reg_write(phy->phy_regmap, lt9721_tx_phy_cfg,
					ARRAY_SIZE(lt9721_tx_phy_cfg));
	return 0;
}

static int lt9721_tx_pll_cfg(struct gsgpu_bridge_phy *phy)
{
	regmap_multi_reg_write(phy->phy_regmap, lt9721_pll_cfg_seq,
					ARRAY_SIZE(lt9721_pll_cfg_seq));
	return 0;
}

int lt9721_hdmi_format(struct gsgpu_bridge_phy *phy)
{
	regmap_write(phy->phy_regmap, 0x8817, 0x08);
	regmap_write(phy->phy_regmap, 0x8818, 0x20);
	regmap_write(phy->phy_regmap, 0x881d, 0x36);
	regmap_write(phy->phy_regmap, 0x881f, 0x4e);
	regmap_write(phy->phy_regmap, 0x8820, 0x66);

	return 0;
}

static struct bridge_phy_helper lt9721_helper_funcs = {
	.regmap_cfg = &lt9721_regmap_config,
	.misc_funcs = &lt9721_misc_funcs,
	.hpd_funcs = &lt9721_hpd_funcs,
};

static int lt9721_bl_ctrl(struct gsgpu_bridge_phy *phy, int mode)
{
	if (mode == DRM_MODE_DPMS_ON) {
		regmap_write(phy->phy_regmap, 0x7044, 0x0c);
		regmap_write(phy->phy_regmap, 0x80d1, 0x0c);
		regmap_write(phy->phy_regmap, 0x80d3, 0x00);
		regmap_write(phy->phy_regmap, 0x80d3, 0x30);
		regmap_write(phy->phy_regmap, 0x80c8, 0x20);
		regmap_write(phy->phy_regmap, 0x80c9, 0x00);
		regmap_write(phy->phy_regmap, 0x80ca, 0x27);
		regmap_write(phy->phy_regmap, 0x80cb, 0x10);
		regmap_write(phy->phy_regmap, 0x80cc, 0x00);
		regmap_write(phy->phy_regmap, 0x80cd, 0x27);
		regmap_write(phy->phy_regmap, 0x80ce, 0x10);
	} else {
		regmap_write(phy->phy_regmap, 0x80d1, 0x04);
		regmap_write(phy->phy_regmap, 0x80cc, 0x00);
		regmap_write(phy->phy_regmap, 0x80cd, 0x00);
		regmap_write(phy->phy_regmap, 0x80cd, 0x00);
		regmap_write(phy->phy_regmap, 0x80ce, 0x00);
	}

	return 0;
}

static int lt9721_mode_set(struct gsgpu_bridge_phy *phy,
				const struct drm_display_mode *mode,
				const struct drm_display_mode *adj_mode)
{
	lt9721_hdmi_format(phy);
	regmap_write(phy->phy_regmap, 0x80d1, 0x0c);
	regmap_write(phy->phy_regmap, 0x881e, 0x30);

	return 0;
}

static int lt9721_video_input_cfg(struct gsgpu_bridge_phy *phy)
{
	lt9721_tx_phy(phy);
	lt9721_tx_pll_cfg(phy);

	regmap_write(phy->phy_regmap, 0x7021, 0x0d);
	regmap_write(phy->phy_regmap, 0x7021, 0x0f);

	regmap_write(phy->phy_regmap, 0x881e, 0x20);
	return 0;
}

static int lt9721_video_output_cfg(struct gsgpu_bridge_phy *phy)
{
	lt9721_hdmi_rx_cdr(phy);
	lt9721_rx_pll(phy);
	return 0;
}

static const struct bridge_phy_cfg_funcs lt9721_cfg_funcs = {
	.backlight_ctrl = lt9721_bl_ctrl,
	.video_input_cfg = lt9721_video_input_cfg,
	.video_output_cfg = lt9721_video_output_cfg,
};

int bridge_phy_lt9721_init(struct gsgpu_dc_bridge *res)
{
	struct gsgpu_bridge_phy *lt9721_phy;
	struct lt9721_device *lt9721_dev;
	u32 feature;
	int ret;

	feature = SUPPORT_HPD | SUPPORT_DDC | SUPPORT_HDMI_AUX;
	lt9721_phy = bridge_phy_alloc(res);

	lt9721_dev = kmalloc(sizeof(struct lt9721_device), GFP_KERNEL);
	if (IS_ERR(lt9721_dev))
		return PTR_ERR(lt9721_dev);

	lt9721_phy->priv = lt9721_dev;

	ret = bridge_phy_register(lt9721_phy, &lt9721_cfg_funcs, feature,
					&lt9721_helper_funcs);

	lt9721_bl_ctrl(lt9721_phy, DRM_MODE_DPMS_ON);
	lt9721_mode_set(lt9721_phy, NULL, NULL);

	return 0;
}
