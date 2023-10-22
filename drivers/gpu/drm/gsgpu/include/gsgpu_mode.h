#ifndef __GSGPU_MODE_H__
#define __GSGPU_MODE_H__

#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_edid.h>
#include <drm/drm_encoder.h>
#include <drm/drm_fixed.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_plane_helper.h>

#include "gsgpu_irq.h"
#include "gsgpu_dc_irq.h"

struct gsgpu_bo;
struct gsgpu_device;
struct gsgpu_encoder;

#define to_gsgpu_crtc(x) container_of(x, struct gsgpu_crtc, base)
#define to_gsgpu_connector(x) container_of(x, struct gsgpu_connector, base)
#define to_gsgpu_encoder(x) container_of(x, struct gsgpu_encoder, base)
#define to_gsgpu_framebuffer(x) container_of(x, struct gsgpu_framebuffer, base)
#define to_gsgpu_plane(x)	container_of(x, struct gsgpu_plane, base)

#define GSGPU_MAX_HPD_PINS 3
#define GSGPU_MAX_CRTCS 2
#define GSGPU_MAX_PLANES 4

enum gsgpu_rmx_type {
	RMX_OFF,
	RMX_FULL,
	RMX_CENTER,
	RMX_ASPECT
};

enum gsgpu_flip_status {
	GSGPU_FLIP_NONE,
	GSGPU_FLIP_PENDING,
	GSGPU_FLIP_SUBMITTED
};

struct gsgpu_display_funcs {
	/* get frame count */
	u32 (*vblank_get_counter)(struct gsgpu_device *adev, int crtc);
	/* set backlight level */
	void (*backlight_set_level)(struct gsgpu_encoder *gsgpu_encoder,
				    u8 level);
	/* get backlight level */
	u8 (*backlight_get_level)(struct gsgpu_encoder *gsgpu_encoder);
	/* hotplug detect */
	bool (*hpd_sense)(struct gsgpu_device *adev);
	u32 (*hpd_get_gpio_reg)(struct gsgpu_device *adev);
	/* pageflipping */
	void (*page_flip)(struct gsgpu_device *adev,
			  int crtc_id, u64 crtc_base, bool async);
	int (*page_flip_get_scanoutpos)(struct gsgpu_device *adev, int crtc,
					int *vbl_start, int *vbl_end, int *hpos, int *vpos);
};

struct gsgpu_framebuffer {
	struct drm_framebuffer base;

	/* caching for later use */
	uint64_t address;
};

struct gsgpu_fbdev {
	struct drm_fb_helper helper;
	struct gsgpu_framebuffer rfb;
	struct list_head fbdev_list;
	struct gsgpu_device *adev;
};

struct gsgpu_crtc {
	struct drm_crtc base;
	int crtc_id;
	bool enabled;
	bool can_tile;
	uint32_t crtc_offset;
	enum dc_irq_source irq_source_vsync;
	struct drm_gem_object *cursor_bo;
	uint64_t cursor_addr;
	int cursor_x;
	int cursor_y;
	int cursor_hot_x;
	int cursor_hot_y;
	int cursor_width;
	int cursor_height;
	int max_cursor_width;
	int max_cursor_height;
	struct mutex cursor_lock;
	struct gsgpu_flip_work *pflip_works;
	enum gsgpu_flip_status pflip_status;
	u32 lb_vblank_lead_lines;
	struct drm_display_mode hw_mode;
	struct drm_pending_vblank_event *event;
};

struct gsgpu_plane {
	struct drm_plane base;
	enum drm_plane_type plane_type;
};

struct gsgpu_encoder {
	struct drm_encoder base;
	uint32_t encoder_enum;
	uint32_t encoder_id;
	struct gsgpu_bridge_phy *bridge;
};

enum gsgpu_connector_audio {
	GSGPU_AUDIO_DISABLE = 0,
	GSGPU_AUDIO_ENABLE = 1,
	GSGPU_AUDIO_AUTO = 2
};

struct gsgpu_connector {
	struct drm_connector base;
	uint32_t connector_id;
	uint32_t devices;
	enum gsgpu_connector_audio audio;
	int num_modes;
	int pixel_clock_mhz;
	struct mutex hpd_lock;
	enum dc_irq_source irq_source_i2c;
	enum dc_irq_source irq_source_hpd;
	enum dc_irq_source irq_source_vga_hpd;
};

struct gsgpu_mode_info {
	bool mode_config_initialized;
	struct gsgpu_crtc *crtcs[GSGPU_MAX_CRTCS];
	struct gsgpu_plane *planes[GSGPU_MAX_PLANES];
	struct gsgpu_connector *connectors[2];
	struct gsgpu_encoder *encoders[2];
	struct gsgpu_backlight *backlights[2];
	/* underscan */
	struct drm_property *underscan_property;
	struct drm_property *underscan_hborder_property;
	struct drm_property *underscan_vborder_property;
	/* audio */
	struct drm_property *audio_property;
	/* pointer to fbdev info structure */
	struct gsgpu_fbdev *rfbdev;
	int			num_crtc; /* number of crtcs */
	int			num_hpd; /* number of hpd pins */
	int			num_i2c;
	int			disp_priority;
	const struct gsgpu_display_funcs *funcs;
	const enum drm_plane_type *plane_type;
};

/* Driver internal use only flags of gsgpu_display_get_crtc_scanoutpos() */
#define DRM_SCANOUTPOS_VALID        (1 << 0)
#define DRM_SCANOUTPOS_IN_VBLANK    (1 << 1)
#define DRM_SCANOUTPOS_ACCURATE     (1 << 2)
#define USE_REAL_VBLANKSTART		(1 << 30)
#define GET_DISTANCE_TO_VBLANKSTART	(1 << 31)

int gsgpu_display_get_crtc_scanoutpos(struct drm_device *dev,
			unsigned int pipe, unsigned int flags, int *vpos,
			int *hpos, ktime_t *stime, ktime_t *etime,
			const struct drm_display_mode *mode);

int gsgpu_display_framebuffer_init(struct drm_device *dev,
				   struct gsgpu_framebuffer *rfb,
				   const struct drm_mode_fb_cmd2 *mode_cmd,
				   struct drm_gem_object *obj);

int gsgpu_fbdev_init(struct gsgpu_device *adev);
void gsgpu_fbdev_fini(struct gsgpu_device *adev);
void gsgpu_fbdev_set_suspend(struct gsgpu_device *adev, int state);
int gsgpu_fbdev_total_size(struct gsgpu_device *adev);
bool gsgpu_fbdev_robj_is_fb(struct gsgpu_device *adev, struct gsgpu_bo *robj);
int gsgpu_align_pitch(struct gsgpu_device *adev, int width, int bpp, bool tiled);
int gsgpu_display_modeset_create_props(struct gsgpu_device *adev);

#endif /* __GSGPU_MODE_H__ */
