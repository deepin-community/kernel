#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/regmap.h>

#include "gsgpu.h"
#include "gsgpu_dc.h"
#include "gsgpu_dc_vbios.h"
#include "bridge_phy.h"

#define LT8619_REG_START                        0x0000
#define LT8619_REG_END                          0x80FE
#define LT8619_REG_PAGE_SELECT                  0xFF
#define EDID_DATA_SIZE                          256
#define WAIT_TIMES                              10000

/* REG 0x6000 */
#define LT8619_REG_CHIP_ID                      0x6000

/* REG 0x6004 */

/* Analog register clock enable(Address range: 0x6080 ~ 0x60FE) */
#define ANALOG_REG_CLK_EN_BIT                   BIT(7)
/* Control register clock enable(Address range: 0x6010 ~ 0x607F) */
#define CONTROL_REG_CLK_EN_BIT                  BIT(6)
/* HDMI register clock enable(Address range: 0x8000 ~ 0x809F) */
#define HDMI_REG_CLK_EN_BIT                     BIT(5)
/* HDCP register clock enable(Address range: 0x80A0 ~ 0x80CF) */
#define HDCP_REG_CLK_EN_BIT                     BIT(4)
/* LVDS PLL lock detect module clock enable */
#define LVDS_PLL_LOCK_DETECT_CLK_EN_BIT         BIT(1)

/* REG 0x6006 */
#define LVDS_TX_CLK_EN_BIT                      BIT(7)
#define BT_TX_CLK_EN_BIT                        BIT(6)
#define EDID_SHADOW_CLK_EN_BIT                  BIT(5)
#define DUAL_PORT_CLK_EN_BIT                    BIT(4)
#define CEC_CTL_SYS_CLK_EN_BIT                  BIT(3)
#define VIDEO_CHECK_SYS_CLK_EN_BIT              BIT(2)
#define VIDEO_CHECK_PIX_CLK_EN_BIT              BIT(1)
#define INTERRUPT_PROCESS_SYS_CLK_EN_BIT        BIT(0)

/* REG 0x600A */
#define LVDS_PLL_LOCK_DETECT_CTL_SW_RST         BIT(6)

/* REG 0x600C */
#define EDID_SHADOW_SW_RST_BIT                  BIT(6)
#define VIDEO_CHK_SW_RST_BIT                    BIT(2)

/* REG 0x600D */
#define LVDS_TX_CTL_SW_RST                      BIT(2)
#define BT_TX_CTL_SW_RST                        BIT(1)
#define BT_TX_AFIFO_SW_RST                      BIT(0)

/* REG 0x600E */
#define HDMI_RX_CALIBRATION_BIT                 BIT(7)
#define HDMI_RX_PLL_BIT                         BIT(6)
#define HDMI_RX_PI0_BIT                         BIT(5)
#define HDMI_RX_PI1_BIT                         BIT(4)
#define HDMI_RX_PI2_BIT                         BIT(3)
#define HDMI_RX_AUDIO_PLL_BIT                   BIT(2)
#define LVDS_PLL_SW_RST_BIT                     BIT(1)
#define LVDS_TX_CLK_GEN_RESET                   BIT(0)

/* REG 0x60A0 */
/* 1 = Power down, 0 = Power up */
#define LVDS_PLL_PD                             BIT(7)
/* 1 = Adaptive BW tracking PLL, 0 = Second order passive LPF PLL */
#define LVDS_PLL_ADAPRT_BW_EN                   BIT(6)
/* 1 = High BW, 0 = Low BW */
#define LVDS_PLL_CP_CUR_SEL                     BIT(5)
/* This bit controls the operation of PLL locking EN */
#define LVDS_PLL_LOCK_EN                        BIT(4)

/* Reference clock selection:
 * 1 = Pixel clock as reference, 0 = Double clock as reference
 */
#define LVDS_PLL_PIX_CLK_SEL                    BIT(3)
/* Pixel clock selection: 1 = 2 x lvds clock, 0 = 1 x lvds clock */
#define LVDS_PLL_DUAL_MODE_EN                   BIT(2)
/* Pixel clock selection: 1 = 4 x lvds clock, 0 = 2 x lvds clock */
#define LVDS_PLL_HALF_MODE_EN                   BIT(1)
/* BT clock clock selection */
#define LVDS_PLL_DOUB_MODE_EN                   BIT(0)

/* REG 0x60A8 */
#define RGB888_TTL_EN_BIT                       BIT(3)

/* 1 = Input is HDMI, 0 = Input is DVI */
#define RX_IS_HDMI_BIT                          BIT(1)
/* 1 = Hsync is detected and is stable, 0 = No hsync detected or not stable */
#define RX_HDMI_HSYNC_STABLE_BIT                BIT(0)

/* Set to enable clock data recovery bandwidth adaptation */
#define CDR_BW_ADAP_EN_BIT                      BIT(3)

/* REG 0x8044 */
/* Clock stable status indicator */
#define RX_CLK_STABLE_BIT                       BIT(3)

/* REG 0x8071 */
/* Packet byte 1 of AVI information
 * {Y1, Y0} of {0,Y1,Y0,A0,B1,B0,S1,S0}:
 */
#define COLOR_SPACE_MASK                        GENMASK(6, 5)
#define CSC_RGB                                 0x00
#define CSC_YCBCR_422                           0x20
#define CSC_YCBCR_444                           0x40
#define CSC_FUTURE                              0x60

/* TMDS clock frequency, unit: kHz
 * +------+--------+--------+--------+
 * | REG  | 0x8044 | 0x8045 | 0x8046 |
 * +------+--------+--------+--------+
 * | bits |  18:16 |  15:8  |  7:0   |
 * +------+--------+--------+--------+
 */

/* REG 0x8087 */
#define LVDS_PLL_LOCKED_BIT                     BIT(5)
#define RX_PLL_LOCKED_BIT                       BIT(4)

struct lt8619_timing {
	int polarity;
	int h_front_porch;
	int h_back_porch;
	int h_total;
	int h_active;
	int v_front_porch;
	int v_back_porch;
	int v_total;
	int v_active;
	int v_sync;
	int h_sync;
};

static const struct regmap_range_cfg lt8619_ranges[] = {
	{
		.name = "lt8619_register_range",
		.range_min = LT8619_REG_START,
		.range_max = LT8619_REG_END,
		.selector_reg = LT8619_REG_PAGE_SELECT,
		.selector_mask = 0xFF,
		.selector_shift = 0,
		.window_start = 0,
		.window_len = 0x100,
	},
};

static const struct regmap_config lt8619_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.reg_stride = 1,
	.max_register = LT8619_REG_END,
	.ranges = lt8619_ranges,
	.num_ranges = ARRAY_SIZE(lt8619_ranges),
	.fast_io = false,
};

static void lt8619_read_revision(struct gsgpu_bridge_phy *phy)
{
	unsigned int chip_id = 0;
	int ret;

	ret = regmap_bulk_read(phy->phy_regmap,
			       LT8619_REG_CHIP_ID, &chip_id, 4);
	if (ret) {
		DRM_ERROR("failed to read revision: %d\n", ret);
		return;
	}

	DRM_DEV_INFO(&phy->li2c->ddc_client->dev, "LT8619 vision: 0x%x\n",
		     chip_id);
}

static void lt8619_lvds_tx_clk_enable(struct gsgpu_bridge_phy *phy)
{
	regmap_update_bits(phy->phy_regmap, 0x6006,
			   LVDS_TX_CLK_EN_BIT,
			   LVDS_TX_CLK_EN_BIT);

	regmap_update_bits(phy->phy_regmap, 0x6006,
			   BT_TX_CLK_EN_BIT,
			   BT_TX_CLK_EN_BIT);

	DRM_DEBUG_DRIVER("LVDS TX controller module clock enabled\n");
}

static int lt8619_enable_sys_clk(struct gsgpu_bridge_phy *phy, bool is_lvds)
{
	u32 val = ANALOG_REG_CLK_EN_BIT |
		  CONTROL_REG_CLK_EN_BIT |
		  HDMI_REG_CLK_EN_BIT |
		  HDCP_REG_CLK_EN_BIT;
	int ret;

	if (is_lvds)
		val |= LVDS_PLL_LOCK_DETECT_CLK_EN_BIT;

	ret = regmap_write(phy->phy_regmap, 0x6004, val);

	return ret;
}

static int lt8619_reg_init(struct gsgpu_bridge_phy *phy)
{
	lt8619_enable_sys_clk(phy, true);
	lt8619_lvds_tx_clk_enable(phy);

	return 0;
}

static const struct bridge_phy_cfg_funcs lt8619_cfg_funcs = {
	.reg_init = lt8619_reg_init,
};

int bridge_phy_lt8619_init(struct gsgpu_dc_bridge *dc_bridge)
{
	struct regmap *regmap;
	int ret;

	struct gsgpu_bridge_phy *lt8619_phy = bridge_phy_alloc(dc_bridge);
	struct gsgpu_dc_i2c *li2c = lt8619_phy->li2c;
	struct i2c_client *client = li2c->ddc_client;

	lt8619_phy->cfg_funcs = &lt8619_cfg_funcs;

	regmap = devm_regmap_init_i2c(client, &lt8619_regmap_config);
	if (IS_ERR(regmap)) {
		DRM_ERROR("Failed to regmap: %d\n", ret);
		return -EINVAL;
	}

	lt8619_phy->phy_regmap = regmap;

	lt8619_read_revision(lt8619_phy);
	lt8619_enable_sys_clk(lt8619_phy, true);
	lt8619_lvds_tx_clk_enable(lt8619_phy);

	DRM_INFO("[Bridge_phy] lt8619 init finish\n");

	return 0;
}
