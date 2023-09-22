#ifndef __GSGPU_DISPLAY_H__
#define __GSGPU_DISPLAY_H__

uint32_t gsgpu_display_supported_domains(struct gsgpu_device *adev);
struct drm_framebuffer *
gsgpu_display_user_framebuffer_create(struct drm_device *dev,
				       struct drm_file *file_priv,
				       const struct drm_mode_fb_cmd2 *mode_cmd);

#endif /* __GSGPU_DISPLAY_H__ */
