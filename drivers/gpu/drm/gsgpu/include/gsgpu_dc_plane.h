#ifndef __DC_PLANE_H__
#define __DC_PLANE_H__

enum dc_plane_type{
	DC_PLANE_PRIMARY,
	DC_PLANE_OVERLAY,
	DC_PLANE_CURSOR
};

struct dc_primary_plane {
	union plane_address address;
};

struct dc_cursor_info {
	bool enable;
	int x;
	int y;
	union plane_address address;
};

struct dc_plane_update {
	enum dc_plane_type type;
	union {
		struct dc_cursor_info cursor;
		struct dc_primary_plane primary;
	};
};

int gsgpu_dc_plane_init(struct gsgpu_device *adev,
			struct gsgpu_plane *aplane,
			unsigned long possible_crtcs);

int initialize_plane(struct gsgpu_device *adev,
		     struct gsgpu_mode_info *mode_info,
		     int plane_id);

bool dc_submit_plane_update(struct gsgpu_dc *dc, u32 link,
			    struct dc_plane_update *update);

bool dc_crtc_plane_update(struct gsgpu_dc_crtc *crtc,
			  struct dc_plane_update *update);

#endif /* __DC_PLANE_H__ */