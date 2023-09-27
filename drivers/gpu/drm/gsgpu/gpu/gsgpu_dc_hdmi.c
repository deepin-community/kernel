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

static int hdmi_set_avi_infoframe(struct drm_encoder *encoder,
			   struct drm_display_mode *mode)
{
	struct gsgpu_device *adev = encoder->dev->dev_private;
	struct hdmi_avi_infoframe avi_frame;
	u8 buffer[HDMI_INFOFRAME_HEADER_SIZE + HDMI_AVI_INFOFRAME_SIZE];
	int err, val;
	uint8_t *frame;
	int index = encoder->index;

	err = drm_hdmi_avi_infoframe_from_display_mode(&avi_frame, mode, false);
	if (err < 0) {
		DRM_ERROR("failed to setup AVI infoframe: %d\n", err);
		return err;
	}

	err = hdmi_avi_infoframe_pack(&avi_frame, buffer, sizeof(buffer));
	if (err < 0) {
		DRM_ERROR("failed to pack AVI infoframe: %d\n", err);
		return err;
	}

	/* PHY config */
	hdmi_phy_pll_config(adev, index, mode->clock);

	/* set avi infoframe */
	frame = buffer + 3;
	val = (avi_frame.scan_mode << 0) | /*scan_info*/
		(0 << 2) | /*bar_info*/
		(1 << 4) | /*active_format_info_present*/
		(avi_frame.colorspace << 5) | /*RGB_OR_YCBCR*/
		(avi_frame.active_aspect << 8) | /*active_format_aspect_ratio*/
		(2 << 12) | /*picture_aspect_ratio*/
		(avi_frame.colorimetry << 14) | /*colorimetry*/
		(0 << 16) | /*nonuniform_picture_scaling*/
		(0 << 18) | /*RGB_quantization_range*/
		(0 << 20) | /*ectended_colorimetry*/
		(avi_frame.itc << 23) | /*it_content*/
		(avi_frame.video_code << 24); /*video_format_id_code*/
	dc_writel(adev, val, (DC_HDMI_AVI_CONT0_REG + (index*0x10)));

	val = (avi_frame.pixel_repeat << 0) | /*pixel_repetition*/
		(avi_frame.content_type << 4) | /*content_type*/
		(avi_frame.ycc_quantization_range << 6); /*YCC_quantization_range*/
	dc_writel(adev, val, (DC_HDMI_AVI_CONT1_REG + (index*0x10)));

	val = ((avi_frame.top_bar & 0xff) << 0) | /*end_top_bar_l*/
		(((avi_frame.top_bar >> 8) & 0xff) << 8) | /*end_top_bar_h*/
		((avi_frame.bottom_bar & 0xff) << 16) | /*start_bottom_bar_l*/
		(((avi_frame.bottom_bar >> 8) & 0xff) << 24); /*start_bottom_bar_h*/
	dc_writel(adev, val, (DC_HDMI_AVI_CONT2_REG + (index*0x10)));

	val = ((avi_frame.left_bar & 0xff) << 0) | /*end_left_bar_l*/
		(((avi_frame.left_bar >> 8) & 0xff) << 8) | /*end_left_bar_h*/
		((avi_frame.right_bar & 0xff) << 16) | /*start_right_bar_l*/
		(((avi_frame.right_bar >> 8) & 0xff) << 24); /*start_right_bar_h*/
	dc_writel(adev, val, (DC_HDMI_AVI_CONT3_REG + (index*0x10)));

	dc_writel(adev, HDMI_AVI_ENABLE_PACKET | HDMI_AVI_FREQ_EACH_FRAME | HDMI_AVI_UPDATE,
		    (DC_HDMI_AVI_CTRL_REG + (index*0x10)));

	return 0;
}

void dc_hdmi_encoder_enable(struct drm_encoder *encoder)
{
	struct gsgpu_device *adev = encoder->dev->dev_private;
	struct drm_display_mode *mode = &encoder->crtc->state->adjusted_mode;
	u32 value;
	u32 index = encoder->index;

	/* hdmi enable */
	value = dc_readl(adev, DC_HDMI_CTRL_REG + (index*0x10));
	value |= (1 << 0);
	dc_writel(adev, value, DC_HDMI_CTRL_REG + (index*0x10));
	DRM_INFO("hdmi encoder enable %d CTRL reg 0x%x\n",
		 encoder->index, value);

	hdmi_set_avi_infoframe(encoder, mode);
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
