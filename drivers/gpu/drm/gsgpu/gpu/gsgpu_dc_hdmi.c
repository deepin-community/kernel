#include "gsgpu.h"
#include "gsgpu_dc.h"
#include "gsgpu_dc_hdmi.h"
#include "gsgpu_dc_crtc.h"
#include "gsgpu_dc_reg.h"

void hdmi_phy_pll_config(struct gsgpu_device *adev, int index, int clock)
{
	int val;
	int count = 0;

	if (adev->chip == dev_2k2000 && index == 1)
		return;

	dc_writel(adev, CURRENT_REG(DC_HDMI_PHY_PLLCFG_REG, index), 0x0);
	dc_writel(adev, CURRENT_REG(DC_HDMI_PHY_CTRL_REG, index), 0xf02);

	if (clock >= 170000)
		val = (0x0 << 13) | (0x28 << 6) | (0x10 << 1) | (0 << 0);
	else if (clock >= 85000  && clock < 170000)
		val = (0x1 << 13) | (0x28 << 6) | (0x8 << 1) | (0 << 0);
	else if (clock >= 42500 && clock < 85000)
		val = (0x2 << 13) | (0x28 << 6) | (0x4 << 1) | (0 << 0);
	else if (clock >= 21250 && clock < 42500)
		val = (0x3 << 13) | (0x28 << 6) | (0x2 << 1) | (0 << 0);

	dc_writel(adev, CURRENT_REG(DC_HDMI_PHY_PLLCFG_REG, index), val);
	val |= (1 << 0);
	dc_writel(adev, CURRENT_REG(DC_HDMI_PHY_PLLCFG_REG, index), val);

	/* wait pll lock */
	while (!(dc_readl(adev, CURRENT_REG(DC_HDMI_PHY_PLLCFG_REG, index)) & 0x10000)) {
		count++;
		if (count >= 1000) {
			DRM_ERROR("GSGPU HDMI PHY PLL lock failed\n");
			return;
		}
	}

	dc_writel(adev, CURRENT_REG(DC_HDMI_PHY_CTRL_REG, index), 0xf03);
}

void gsgpu_hdmi_suspend(struct gsgpu_device *adev)
{
	u32 link = 0;

	adev->dc->hdmi_ctrl_reg = dc_readl(adev, DC_HDMI_CTRL_REG);
	for (link = 0; link < 2; link++) {
		dc_crtc_enable(adev->mode_info.crtcs[link], false);
		dc_writel(adev, DC_HDMI_CTRL_REG + (link * 0x10), 0);
		dc_writel(adev, DC_HDMI_ZONEIDLE_REG + (link * 0x10), 0);
	}
}

int gsgpu_hdmi_resume(struct gsgpu_device *adev)
{
	u32 link = 0;
	u32 reg_val;

	for (link = 0; link < 2; link++) {
		dc_crtc_enable(adev->mode_info.crtcs[link], true);
		dc_writel(adev, DC_HDMI_CTRL_REG + (link * 0x10), adev->dc->hdmi_ctrl_reg);
		dc_writel(adev, DC_HDMI_ZONEIDLE_REG + (link * 0x10), 0x00400040);

		dc_writel(adev, DC_HDMI_AUDIO_NCFG_REG + (link * 0x10), 6272);
		dc_writel(adev, DC_HDMI_AUDIO_CTSCFG_REG + (link * 0x10), 0x80000000);
		dc_writel(adev, DC_HDMI_AUDIO_INFOFRAME_REG + (link * 0x10), 0x11);
		reg_val = dc_readl(adev, DC_HDMI_AUDIO_INFOFRAME_REG + (link * 0x10));
		reg_val |= 0x4;
		dc_writel(adev, DC_HDMI_AUDIO_INFOFRAME_REG + (link * 0x10), reg_val);
		dc_writel(adev, DC_HDMI_AUDIO_SAMPLE_REG + (link * 0x10), 0x1);
	}

	return 0;
}

int gsgpu_hdmi_init(struct gsgpu_device *adev)
{
	u32 val;
	u32 link = 0;

	for (link = 0; link < 2; link++) {
		/* enable hdmi */
		dc_writel(adev, DC_HDMI_CTRL_REG + (link * 0x10), 0x287);

		/* hdmi zone idle */
		dc_writel(adev, DC_HDMI_ZONEIDLE_REG + (link * 0x10), 0x00400040);

		//Audio N
		// 44.1KHz * 4, dynamic update N && CTS value
		dc_writel(adev, DC_HDMI_AUDIO_NCFG_REG + (link * 0x10), 6272);

		//Enable Send CTS
		dc_writel(adev, DC_HDMI_AUDIO_CTSCFG_REG + (link * 0x10), 0x80000000);

		//Audio AIF
		//enable AIF,set freq,and set CC = 1, CA = 0
		dc_writel(adev, DC_HDMI_AUDIO_INFOFRAME_REG + (link * 0x10), 0x11);

		//Update AIF
		val = dc_readl(adev, DC_HDMI_AUDIO_INFOFRAME_REG + (link * 0x10));
		val |= 0x4;
		dc_writel(adev, DC_HDMI_AUDIO_INFOFRAME_REG + (link * 0x10), val);

		//Audio Sample Packet
		dc_writel(adev, DC_HDMI_AUDIO_SAMPLE_REG + (link * 0x10), 0x1);
	}

	return 0;
}
