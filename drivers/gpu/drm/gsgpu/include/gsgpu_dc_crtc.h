#ifndef __DC_CRTC_H__
#define __DC_CRTC_H__

#include "gsgpu_dc.h"

struct gsgpu_dc_crtc {
	struct crtc_resource *resource;
	struct gsgpu_dc *dc;
	struct list_head node;
	int array_mode;
};

enum fb_color_format {
	DC_FB_FORMAT_NONE = 0,
	DC_FB_FORMAT12,
	DC_FB_FORMAT15,
	DC_FB_FORMAT16,
	DC_FB_FORMAT24,
	DC_FB_FORMAT32 = DC_FB_FORMAT24
};

enum cursor_format {
	CUR_FORMAT_NONE,
	CUR_FORMAT_MONO,
	CUR_FORMAT_ARGB8888,
};

struct pixel_clock {
	u32 l2_div;
	u32 l1_loopc;
	u32 l1_frefc;
};

struct gsgpu_dc_crtc *dc_crtc_construct(struct gsgpu_dc *dc, struct crtc_resource *resource);
int gsgpu_dc_crtc_init(struct gsgpu_device *adev,
		       struct drm_plane *plane, uint32_t crtc_index);
u32 dc_vblank_get_counter(struct gsgpu_device *adev, int crtc_num);
int dc_crtc_get_scanoutpos(struct gsgpu_device *adev, int crtc_num,
			   int *vbl_start, int *vbl_end, int *hpos, int *vpos);

void dc_crtc_destroy(struct gsgpu_dc_crtc *crtc);
bool dc_crtc_enable(struct gsgpu_crtc *acrtc, bool enable);
bool dc_crtc_timing_set(struct gsgpu_dc_crtc *crtc, struct dc_timing_info *timing);
bool dc_crtc_vblank_enable(struct gsgpu_dc_crtc *crtc, bool enable);
u32 dc_crtc_get_vblank_counter(struct gsgpu_dc_crtc *crtc);
bool dc_crtc_vblank_ack(struct gsgpu_dc_crtc *crtc);

#endif /* __DC_CRTC_H__ */
