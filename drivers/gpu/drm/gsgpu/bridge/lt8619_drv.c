#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/regmap.h>

#include <asm/loongson.h>
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

static int lt8619_config_rgb888_phy(struct gsgpu_bridge_phy *phy, bool enable)
{
	int ret;
	unsigned int val;

	val = enable ? RGB888_TTL_EN_BIT : 0;

	ret = regmap_update_bits(phy->phy_regmap, 0x60A8,
				 RGB888_TTL_EN_BIT, val);

	DRM_DEBUG_DRIVER("%s: reg=0x%x, val=0x%x\n", __func__, 0x60A8, val);

	return ret;
}

static void lt8619_edid_shadow_clk_enable(struct gsgpu_bridge_phy *phy)
{
	unsigned int val;

	regmap_read(phy->phy_regmap, 0x6006, &val);

	if (val & EDID_SHADOW_CLK_EN_BIT)
		DRM_DEBUG_DRIVER("edid shadow clk is enabled\n");
	else
		DRM_DEBUG_DRIVER("edid shadow clk is not enabled\n");

	regmap_update_bits(phy->phy_regmap, 0x6006,
			   EDID_SHADOW_CLK_EN_BIT,
			   EDID_SHADOW_CLK_EN_BIT);
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

static int lt8619_enable_cdr_bandwidth_adaptation(struct gsgpu_bridge_phy *phy)
{
	int ret;

	ret = regmap_update_bits(phy->phy_regmap, 0x802C,
				 CDR_BW_ADAP_EN_BIT,
				 CDR_BW_ADAP_EN_BIT);

	return ret;
}

static void lt8619_wait_rx_pll_locked(struct gsgpu_bridge_phy *phy)
{
	unsigned int val = 0;
	unsigned int count = 0;

	do {
		regmap_read(phy->phy_regmap, 0x8087, &val);
		count++;
		udelay(5);
	} while (((val & RX_PLL_LOCKED_BIT) == 0) && (count < WAIT_TIMES));

	if (count >= WAIT_TIMES)
		DRM_ERROR("HDMI RX PLL IS NOT LOCKED");
	else
		DRM_DEBUG_DRIVER("HDMI RX PLL LOCKED\n");
}

/*
 * When HDMI signal is stable, LVDS PLL lock status needs to be detected.
 * If it is not locked, LVDS PLL needs to be reset.
 */
static void lt8619_wait_hdmi_stable(struct gsgpu_bridge_phy *phy)
{
	unsigned int val = 0;
	unsigned int count = 0;

	do {
		regmap_read(phy->phy_regmap, 0x8044, &val);
		count++;
		mdelay(50);
	} while ((!(val & RX_CLK_STABLE_BIT)) && (count < 5));

	DRM_DEBUG_DRIVER("HDMI clock signal stabled\n");

	lt8619_wait_rx_pll_locked(phy);

	count = 0;
	DRM_DEBUG_DRIVER("Wait HDMI HSync detect and stable\n");

	do {
		regmap_read(phy->phy_regmap, 0x8013, &val);
		count++;
		mdelay(50);
	} while ((!(val & RX_HDMI_HSYNC_STABLE_BIT)) && (count < 5));

	DRM_DEBUG_DRIVER("HDMI HSync stabled\n");
}

/* TMDS clock frequency indicator */
static void lt8619_read_hdmi_clock_frequency(struct gsgpu_bridge_phy *phy,
					     unsigned int *pfreq)
{
	unsigned int up, mid, low;
	unsigned int freq;

	regmap_read(phy->phy_regmap, 0x8044, &up);
	regmap_read(phy->phy_regmap, 0x8045, &mid);
	regmap_read(phy->phy_regmap, 0x8046, &low);

	freq = ((up & 0x07) << 16) + (mid << 8) + low;

	if (pfreq)
		*pfreq = freq;
	else
		DRM_DEBUG_DRIVER("HDMI clock frequency: %dkHz\n", freq);
}

/* LVDS TX controller module soft reset */
static void lt8619_lvds_tx_ctl_soft_reset(struct gsgpu_bridge_phy *phy)
{
	regmap_update_bits(phy->phy_regmap, 0x600D, LVDS_TX_CTL_SW_RST, 0);
	mdelay(100);
	regmap_update_bits(phy->phy_regmap, 0x600D,
			   LVDS_TX_CTL_SW_RST,
			   LVDS_TX_CTL_SW_RST);
	mdelay(100);
	DRM_DEBUG_DRIVER("LVDS TX controller module soft reset finished\n");
}

static void lt8619_edid_shadow_soft_reset(struct gsgpu_bridge_phy *phy)
{
	regmap_update_bits(phy->phy_regmap, 0x600C, EDID_SHADOW_SW_RST_BIT, 0);
	mdelay(100);
	regmap_update_bits(phy->phy_regmap, 0x600C,
			   EDID_SHADOW_SW_RST_BIT,
			   EDID_SHADOW_SW_RST_BIT);
	mdelay(100);
	DRM_DEBUG_DRIVER("%s\n", __func__);
}

/* Video check logic soft reset */
static void lt8619_vid_chk_soft_reset(struct gsgpu_bridge_phy *phy)
{
	regmap_update_bits(phy->phy_regmap, 0x600C, VIDEO_CHK_SW_RST_BIT, 0);
	mdelay(100);
	regmap_update_bits(phy->phy_regmap, 0x600C,
			   VIDEO_CHK_SW_RST_BIT,
			   VIDEO_CHK_SW_RST_BIT);
	mdelay(100);
}

/*
 * Read HDMI Timing information
 */
static ssize_t lt8619_read_hdmi_timings(struct gsgpu_bridge_phy *phy,
					struct lt8619_timing *buf)
{
	unsigned int pos = 0;
	unsigned int high, low;
	unsigned int polarity;
	unsigned int h_front_porch, h_back_porch, h_total, h_active;
	unsigned int v_front_porch, v_back_porch, v_total, v_active;
	unsigned int v_sync, h_sync;

	/* horizontal sync width */
	regmap_read(phy->phy_regmap, 0x6014, &high);
	regmap_read(phy->phy_regmap, 0x6015, &low);
	h_sync = (high << 8) + low;
	/* horizontal back porch */
	regmap_read(phy->phy_regmap, 0x6018, &high);
	regmap_read(phy->phy_regmap, 0x6019, &low);
	h_back_porch = (high << 8) + low;
	/* horizontal front porch */
	regmap_read(phy->phy_regmap, 0x601A, &high);
	regmap_read(phy->phy_regmap, 0x601B, &low);
	h_front_porch = (high << 8) + low;
	/* horizontal total */
	regmap_read(phy->phy_regmap, 0x601C, &high);
	regmap_read(phy->phy_regmap, 0x601D, &low);
	h_total = (high << 8) + low;
	/* horizontal active */
	regmap_read(phy->phy_regmap, 0x6020, &high);
	regmap_read(phy->phy_regmap, 0x6021, &low);
	h_active = (high << 8) + low;

	/* vertical total */
	regmap_read(phy->phy_regmap, 0x601E, &high);
	regmap_read(phy->phy_regmap, 0x601F, &low);
	v_total = (high << 8) + low;
	/* vertical active */
	regmap_read(phy->phy_regmap, 0x6022, &high);
	regmap_read(phy->phy_regmap, 0x6023, &low);
	v_active = (high << 8) + low;
	/* vertical back porch */
	regmap_read(phy->phy_regmap, 0x6016, &v_back_porch);
	/* vertical front porch */
	regmap_read(phy->phy_regmap, 0x6017, &v_front_porch);
	/* vertical sync width */
	regmap_read(phy->phy_regmap, 0x6013, &v_sync);

	/* The vsync polarity and hsync polarity */
	regmap_read(phy->phy_regmap, 0x6024, &polarity);

	if (buf) {
		buf->polarity = polarity;
		buf->h_front_porch = h_front_porch;
		buf->h_back_porch = h_back_porch;
		buf->h_total = h_total;
		buf->h_active = h_active;
		buf->v_front_porch = v_front_porch;
		buf->v_back_porch = v_back_porch;
		buf->v_total = v_total;
		buf->v_active = v_active;
		buf->v_sync = v_sync;
		buf->h_sync = h_sync;
		DRM_DEBUG_DRIVER("h_front_porch-%d h_back_porch-%d h_total-%d\n",
				 h_front_porch, h_back_porch, h_total);
		DRM_DEBUG_DRIVER("h_active-%d h_sync-%d\n", h_active, h_sync);
		DRM_DEBUG_DRIVER("v_front_porch-%d v_back_porch-%d v_total-%d\n",
				 v_front_porch, v_back_porch, v_total);
		DRM_DEBUG_DRIVER("v_active-%d v_sync-%d\n", v_active, v_sync);
	} else {
		DRM_DEBUG_DRIVER("%ux%u\n", h_active, v_active);
		DRM_DEBUG_DRIVER("%u, %u, %u, %u\n",
				 h_front_porch, h_back_porch, h_total, h_sync);
		DRM_DEBUG_DRIVER("%u, %u, %u, %u\n",
				 v_front_porch, v_back_porch, v_total, v_sync);
	}

	return pos;
}

static void lt8619_turn_on_lvds(struct gsgpu_bridge_phy *phy)
{
	/* bit2 = 0 => turn on LVDS C */
	regmap_write(phy->phy_regmap, 0x60BA, 0x18);

	/* bit2 = 0 => turn on LVDS D */
	regmap_write(phy->phy_regmap, 0x60C0, 0x18);
}

static void lt8619_turn_off_lvds(struct gsgpu_bridge_phy *phy)
{
	/* bit2= 1 => turn off LVDS C */
	regmap_write(phy->phy_regmap, 0x60BA, 0x44);

	/* bit2= 1 => turn off LVDS D */
	regmap_write(phy->phy_regmap, 0x60C0, 0x44);
}

static void lt8619_hdmi_rx_reset(struct gsgpu_bridge_phy *phy)
{
	regmap_write(phy->phy_regmap, 0x600E, 0xBF);
	regmap_write(phy->phy_regmap, 0x6009, 0xFD);
	mdelay(50);

	regmap_write(phy->phy_regmap, 0x600E, 0xFF);
	regmap_write(phy->phy_regmap, 0x6009, 0xFF);
	mdelay(50);

	regmap_write(phy->phy_regmap, 0x600E, 0xC7);
	regmap_write(phy->phy_regmap, 0x6009, 0x0F);
	mdelay(100);

	regmap_write(phy->phy_regmap, 0x600E, 0xFF);
	mdelay(100);
	regmap_write(phy->phy_regmap, 0x6009, 0x8F);
	mdelay(100);
	regmap_write(phy->phy_regmap, 0x6009, 0xFF);
	mdelay(100);
}

/* LVDS PLL lock detect control logic Soft Reset */
static void lt8619_lvds_pll_lock_ctl_soft_reset(struct gsgpu_bridge_phy *phy)
{
	regmap_update_bits(phy->phy_regmap, 0x600A,
			   LVDS_PLL_LOCK_DETECT_CTL_SW_RST, 0);
	mdelay(100);
	regmap_update_bits(phy->phy_regmap, 0x600A,
			   LVDS_PLL_LOCK_DETECT_CTL_SW_RST,
			   LVDS_PLL_LOCK_DETECT_CTL_SW_RST);
}

/* LVDS PLL Soft Reset */
static void lt8619_lvds_pll_soft_reset(struct gsgpu_bridge_phy *phy)
{
	regmap_update_bits(phy->phy_regmap, 0x600E, LVDS_PLL_SW_RST_BIT, 0);
	mdelay(100);
	regmap_update_bits(phy->phy_regmap, 0x600E, LVDS_PLL_SW_RST_BIT,
						LVDS_PLL_SW_RST_BIT);

	DRM_DEBUG_DRIVER("%s\n", __func__);
}

static void lt8619_wait_lvds_pll_locked(struct gsgpu_bridge_phy *phy)
{
	unsigned int val = 0;
	unsigned int val1 = 0;
	unsigned int count = 0;

	regmap_read(phy->phy_regmap, 0x8087, &val);
	while ((val & 0x20) == 0x00) {
		regmap_read(phy->phy_regmap, 0x600E, &val1);
		regmap_write(phy->phy_regmap, 0x600E, val1 & 0xFD);
		mdelay(50);
		regmap_write(phy->phy_regmap, 0x600E, 0xFF);
		count++;

		if (count > 50)
			break;
	}

	if (count >= 50)
		DRM_ERROR("LVDS PLL is NOT locked\n");
	else
		DRM_DEBUG_DRIVER("LVDS PLL locked\n");
}

static void lt8619_config_lvds_pll(struct gsgpu_bridge_phy *phy)
{
	/* LVDS TX controller module clock enable */
	regmap_write(phy->phy_regmap, 0x60A0,
		     LVDS_PLL_ADAPRT_BW_EN |
		     LVDS_PLL_LOCK_EN |
		     LVDS_PLL_PIX_CLK_SEL);
}

static void lt8619_config_color_space(struct gsgpu_bridge_phy *phy)
{
	unsigned int temp_csc;

	regmap_read(phy->phy_regmap, 0x8071, &temp_csc);

	/* if the color space is not RGB, we need convert it */
	if ((temp_csc & COLOR_SPACE_MASK) == CSC_YCBCR_422) {
		/* enable YCbCr to RGB clk */
		regmap_write(phy->phy_regmap, 0x6007, 0x8C);
		/* YUV422 to YUV444 enable */
		regmap_write(phy->phy_regmap, 0x6052, 0x01);
		/* 0x40:YUV to RGB enable; */
		regmap_write(phy->phy_regmap, 0x6053, 0x40 + 0x30);
		DRM_DEBUG_DRIVER("HDMI Color Space is YUV422\n");
	} else if ((temp_csc & COLOR_SPACE_MASK) == CSC_YCBCR_444) {
		/* enable YCbCr to RGB clk */
		regmap_write(phy->phy_regmap, 0x6007, 0x8C);
		/* YUV444 */
		regmap_write(phy->phy_regmap, 0x6052, 0x00);
		/* 0x40:YUV to RGB enable; */
		regmap_write(phy->phy_regmap, 0x6053, 0x40 + 0x30);
		DRM_DEBUG_DRIVER("HDMI Color Space is YUV444\n");
	} else if ((temp_csc & COLOR_SPACE_MASK) == CSC_RGB) {
		/* 0x00: bypass ColorSpace conversion */
		regmap_write(phy->phy_regmap, 0x6007, 0x80);
		regmap_write(phy->phy_regmap, 0x6052, 0x00);
		regmap_write(phy->phy_regmap, 0x6053, 0x00);
		DRM_DEBUG_DRIVER("HDMI Color Space is RGB");
	}
}

static int lt8619_lvds_config(struct gsgpu_bridge_phy *phy)
{
	int ret;

	regmap_write(phy->phy_regmap, 0x6059, 0x40);

	regmap_write(phy->phy_regmap, 0x60A4, 0x01);
	regmap_write(phy->phy_regmap, 0x605C, 0x01);

	/* LVDS channel output current settings */
	regmap_write(phy->phy_regmap, 0x60B0, 0x66);
	regmap_write(phy->phy_regmap, 0x60B1, 0x66);
	regmap_write(phy->phy_regmap, 0x60B2, 0x66);
	regmap_write(phy->phy_regmap, 0x60B3, 0x66);
	regmap_write(phy->phy_regmap, 0x60B4, 0x66);

	regmap_write(phy->phy_regmap, 0x60B5, 0x41);
	regmap_write(phy->phy_regmap, 0x60B6, 0x41);
	regmap_write(phy->phy_regmap, 0x60B7, 0x41);
	regmap_write(phy->phy_regmap, 0x60B8, 0x4c);
	regmap_write(phy->phy_regmap, 0x60B9, 0x41);

	regmap_write(phy->phy_regmap, 0x60BB, 0x41);
	regmap_write(phy->phy_regmap, 0x60BC, 0x41);
	regmap_write(phy->phy_regmap, 0x60BD, 0x41);
	regmap_write(phy->phy_regmap, 0x60BE, 0x4c);
	regmap_write(phy->phy_regmap, 0x60BF, 0x41);

	ret = regmap_write(phy->phy_regmap, 0x6080, 0x08);
	if (ret)
		return ret;

	ret = regmap_write(phy->phy_regmap, 0x6089, 0x88);
	if (ret)
		return ret;

	ret = regmap_write(phy->phy_regmap, 0x608B, 0x90);
	if (ret)
		return ret;

	regmap_write(phy->phy_regmap, 0x60A1, 0xb0);
	regmap_write(phy->phy_regmap, 0x60A2, 0x10);

	return 0;
}

void lt8619_bridge_enable(struct gsgpu_bridge_phy *phy)
{
	lt8619_hdmi_rx_reset(phy);
	lt8619_vid_chk_soft_reset(phy);

	lt8619_edid_shadow_soft_reset(phy);
	lt8619_edid_shadow_clk_enable(phy);
	lt8619_enable_cdr_bandwidth_adaptation(phy);

	lt8619_wait_hdmi_stable(phy);
	lt8619_read_hdmi_timings(phy, NULL);
	lt8619_read_hdmi_clock_frequency(phy, NULL);

	lt8619_config_color_space(phy);
	lt8619_config_lvds_pll(phy);

	lt8619_lvds_pll_lock_ctl_soft_reset(phy);
	lt8619_lvds_pll_soft_reset(phy);
	lt8619_lvds_tx_ctl_soft_reset(phy);
	lt8619_config_rgb888_phy(phy, false);
	lt8619_lvds_config(phy);

	/*
	 * When HDMI signal is stable, then we wait LVDS PLL locked.
	 * If it is not locked, LVDS PLL needs to be reset.
	 */
	lt8619_wait_lvds_pll_locked(phy);
	lt8619_turn_on_lvds(phy);
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
