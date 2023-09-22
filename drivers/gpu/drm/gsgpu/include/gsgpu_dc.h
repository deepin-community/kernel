#ifndef __DC_INTERFACE_H__
#define __DC_INTERFACE_H__

#include "gsgpu_dc_irq.h"

#define DC_VER "1.0"
#define DC_DVO_MAXLINK 2

extern const struct gsgpu_ip_block_version dc_ip_block;

struct gsgpu_device;
struct drm_device;
struct dc_cursor_info;

struct dc_cursor_position {
	uint32_t x;
	uint32_t y;
	uint32_t x_hotspot;
	uint32_t y_hotspot;
	/* This parameter indicates whether HW cursor should be enabled */
	bool enable;
};

struct irq_list_head {
	struct list_head head;
	/* In case this interrupt needs post-processing, 'work' will be queued*/
	struct work_struct work;
};

struct gsgpu_link_info {
	bool fine;
	struct gsgpu_dc_crtc *crtc;
	struct gsgpu_dc_i2c *i2c;
	struct gsgpu_dc_encoder *encoder;
	struct gsgpu_dc_connector *connector;
	struct gsgpu_dc_bridge *bridge;
};

struct gsgpu_dc {
	struct drm_device *ddev;
	struct gsgpu_device *adev;
	struct gsgpu_vbios *vbios;
	struct list_head crtc_list;
	struct list_head encoder_list;
	struct list_head connector_list;
	struct gsgpu_link_info *link_info;

	/* base information */
	int chip;
	int ip_version;
	int hw_version;
	int links;
	int max_cursor_size;
	int hdmi_ctrl_reg;

	/* DC meta */
	struct gsgpu_bo *meta_bo;
	uint64_t meta_gpu_addr;
	void *meta_cpu_addr;

	/**
	 * Caches device atomic state for suspend/resume
	 */
	struct drm_atomic_state *cached_state;

	spinlock_t irq_handler_list_table_lock;
	struct irq_list_head irq_handler_list_low_tab[DC_IRQ_SOURCES_NUMBER];
	struct list_head irq_handler_list_high_tab[DC_IRQ_SOURCES_NUMBER];
};

union plane_address {
	struct {
		u32 low_part;
		u32 high_part;
	};
	uint64_t raw;
};

struct dc_timing_info {
	s32 clock;		/* from drm_display_mode::clock*/
	s32 hdisplay;
	s32 hsync_start;
	s32 hsync_end;
	s32 htotal;
	s32 vdisplay;
	s32 vsync_start;
	s32 vsync_end;
	s32 vtotal;
	u32 stride;
	u32 depth;
	u32 use_dma32;
};

struct dc_cursor_move {
	u32 hot_y;
	u32 hot_x;
	u32 x, y;
	bool enable;
};

u32 dc_readl(struct gsgpu_device *adev, u32 reg);
void dc_writel(struct gsgpu_device *adev, u32 reg, u32 val);
u32 dc_readl_locked(struct gsgpu_device *adev, u32 reg);
void dc_writel_locked(struct gsgpu_device *adev, u32 reg, u32 val);

bool dc_submit_timing_update(struct gsgpu_dc *dc, u32 link, struct dc_timing_info *timing);
int dc_register_irq_handlers(struct gsgpu_device *adev);
void handle_cursor_update(struct drm_plane *plane,
			  struct drm_plane_state *old_plane_state);
bool crtc_cursor_set(struct gsgpu_dc_crtc *crtc, struct dc_cursor_info *cursor);

#endif
