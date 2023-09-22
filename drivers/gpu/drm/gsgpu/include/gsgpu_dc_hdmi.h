#ifndef __GSGPU_HDMI_H__
#define __GSGPU_HDMI_H__

int gsgpu_hdmi_init(struct gsgpu_device *adev);
void gsgpu_hdmi_suspend(struct gsgpu_device *adev);
int gsgpu_hdmi_resume(struct gsgpu_device *adev);
void dc_hdmi_encoder_enable(struct drm_encoder *encoder);
void dc_hdmi_encoder_mode_set(struct drm_encoder *encoder,
			      struct drm_display_mode *mode,
			      struct drm_display_mode *adjusted_mode);
void hdmi_phy_pll_config(struct gsgpu_device *adev, int index, int clock);

#endif /* __GSGPU_HDMI_H__ */
