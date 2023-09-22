#include "gsgpu.h"
#include "gsgpu_dc_crtc.h"
#include "gsgpu_dc_plane.h"
#include "gsgpu_dc.h"
#include "gsgpu_dc_resource.h"
#include "gsgpu_dc_reg.h"

static int get_cursor_position(struct drm_plane *plane, struct drm_crtc *crtc,
			       struct dc_cursor_position *position)
{
	struct gsgpu_crtc *gsgpu_crtc = to_gsgpu_crtc(crtc);
	int x, y;
	int xorigin = 0, yorigin = 0;

	if (!crtc || !plane->state->fb) {
		position->enable = false;
		position->x = 0;
		position->y = 0;
		return 0;
	}

	if ((plane->state->crtc_w > gsgpu_crtc->max_cursor_width) ||
	    (plane->state->crtc_h > gsgpu_crtc->max_cursor_height)) {
		DRM_ERROR("%s: bad cursor width or height %d x %d\n",
			  __func__,
			  plane->state->crtc_w,
			  plane->state->crtc_h);
		return -EINVAL;
	}

	x = plane->state->crtc_x;
	y = plane->state->crtc_y;

	if (x <= -gsgpu_crtc->max_cursor_width ||
	    y <= -gsgpu_crtc->max_cursor_height)
		return 0;

	if (x < 0) {
		xorigin = min(-x, gsgpu_crtc->max_cursor_width - 1);
		x = 0;
	}

	if (y < 0) {
		yorigin = min(-y, gsgpu_crtc->max_cursor_height - 1);
		y = 0;
	}

	position->enable = true;
	position->x = x;
	position->y = y;
	position->x_hotspot = xorigin;
	position->y_hotspot = yorigin;

	return 0;
}

static bool dc_crtc_cursor_move(struct gsgpu_device *adev, int link,
				struct dc_cursor_move *move)
{
	struct gsgpu_crtc *acrtc = adev->mode_info.crtcs[link];
	u32 cfg_val = 0;
	u32 pos_val = 0;

	if (IS_ERR_OR_NULL(move))
		return false;

	if (link >= DC_DVO_MAXLINK)
		return false;

	DRM_DEBUG_DRIVER("crusor link%d cur move x%d y%d hotx%d hoty%d enable%d\n",
		link, move->x, move->y, move->hot_x, move->hot_y, move->enable);

	mutex_lock(&acrtc->cursor_lock);
	if (link == 0)
		cfg_val = dc_readl(adev, DC_CURSOR0_CFG_REG);
	else
		cfg_val = dc_readl(adev, DC_CURSOR1_CFG_REG);

	cfg_val &= ~(DC_CURSOR_POS_HOT_X_MASK << DC_CURSOR_POS_HOT_X_SHIFT);
	cfg_val &= ~(DC_CURSOR_POS_HOT_Y_MASK << DC_CURSOR_POS_HOT_Y_SHIFT);
	if (move->enable == false) {
		cfg_val &= ~DC_CURSOR_FORMAT_MASK;
		pos_val = 0x0;
	} else {
		cfg_val |= ((CUR_FORMAT_ARGB8888 & DC_CURSOR_FORMAT_MASK) << DC_CURSOR_FORMAT_SHIFT);
		cfg_val |= ((link & DC_CURSOR_DISPLAY_MASK) << DC_CURSOR_DISPLAY_SHIFT);
		cfg_val |= ((move->hot_x & DC_CURSOR_POS_HOT_X_MASK) << DC_CURSOR_POS_HOT_X_SHIFT);
		cfg_val |= ((move->hot_y & DC_CURSOR_POS_HOT_Y_MASK) << DC_CURSOR_POS_HOT_Y_SHIFT);
		pos_val = ((move->x & DC_CURSOR_POS_X_MASK) << DC_CURSOR_POS_X_SHIFT);
		pos_val |= ((move->y & DC_CURSOR_POS_Y_MASK) << DC_CURSOR_POS_Y_SHIFT);
	}

	if (link == 0) {
		dc_writel(adev, DC_CURSOR0_CFG_REG, cfg_val);
		dc_writel(adev, DC_CURSOR0_POSITION_REG, pos_val);
	} else {
		dc_writel(adev, DC_CURSOR1_CFG_REG, cfg_val);
		dc_writel(adev, DC_CURSOR1_POSITION_REG, pos_val);
	}
	mutex_unlock(&acrtc->cursor_lock);

	return true;
}

bool crtc_cursor_set(struct gsgpu_dc_crtc *crtc, struct dc_cursor_info *cursor)
{
	struct gsgpu_device *adev = crtc->dc->adev;
	struct gsgpu_crtc *acrtc;
	int value = 0;
	u32 addr_l, addr_h;
	u32 link;

	if (IS_ERR_OR_NULL(crtc) || IS_ERR_OR_NULL(cursor))
		return false;

	link = crtc->resource->base.link;
	if (link >= DC_DVO_MAXLINK)
		return false;

	acrtc = adev->mode_info.crtcs[link];

	mutex_lock(&acrtc->cursor_lock);
	if (link == 0)
		value = dc_readl(adev, DC_CURSOR0_CFG_REG);
	else
		value = dc_readl(adev, DC_CURSOR1_CFG_REG);

	value &= DC_CURSOR_MODE_CLEAN;
	switch (cursor->x) {
	case 32:
		value |= DC_CURSOR_MODE_32x32;
		break;
	case 64:
		value |= DC_CURSOR_MODE_64x64;
		break;
	default:
		DRM_ERROR("cursor x %u y %u not supported\n",
			  cursor->x, cursor->y);
		return false;
	}

	value |= (link & DC_CURSOR_DISPLAY_MASK) << DC_CURSOR_DISPLAY_SHIFT;
	value |= (CUR_FORMAT_ARGB8888 & DC_CURSOR_FORMAT_MASK) << DC_CURSOR_FORMAT_SHIFT;

	addr_l = cursor->address.low_part;
	addr_h = cursor->address.high_part;

	if (link == 0) {
		dc_writel(adev, DC_CURSOR0_CFG_REG, value);
		dc_writel(adev, DC_CURSOR0_LADDR_REG, addr_l);
		dc_writel(adev, DC_CURSOR0_HADDR_REG, addr_h);
	} else {
		dc_writel(adev, DC_CURSOR1_CFG_REG, value);
		dc_writel(adev, DC_CURSOR1_LADDR_REG, addr_l);
		dc_writel(adev, DC_CURSOR1_HADDR_REG, addr_h);
	}
	mutex_unlock(&acrtc->cursor_lock);

	return true;
}

void handle_cursor_update(struct drm_plane *plane,
			  struct drm_plane_state *old_plane_state)
{
	struct gsgpu_device *adev = plane->dev->dev_private;
	struct gsgpu_framebuffer *afb = to_gsgpu_framebuffer(plane->state->fb);
	struct drm_crtc *crtc = afb ? plane->state->crtc : old_plane_state->crtc;
	struct gsgpu_crtc *gsgpu_crtc = to_gsgpu_crtc(crtc);
	uint64_t address = afb ? afb->address : 0;
	struct dc_cursor_position position;
	struct dc_plane_update dc_plane;
	struct dc_cursor_move move = {0};
	int ret;

	if (!plane->state->fb && !old_plane_state->fb)
		return;

	DRM_DEBUG_DRIVER("%s: crtc_id=%d with size %d to %d\n",
			 __func__,
			 gsgpu_crtc->crtc_id,
			 plane->state->crtc_w,
			 plane->state->crtc_h);

	ret = get_cursor_position(plane, crtc, &position);
	if (ret)
		return;

	if (!position.enable) {
		/* turn off cursor */
		dc_crtc_cursor_move(adev, gsgpu_crtc->crtc_id, &move);
		return;
	}

	dc_plane.type = DC_PLANE_CURSOR;
	dc_plane.cursor.x = plane->state->crtc_w;
	dc_plane.cursor.y = plane->state->crtc_h;
	dc_plane.cursor.address.low_part = lower_32_bits(address);
	dc_plane.cursor.address.high_part = upper_32_bits(address);

	move.enable = position.enable;
	move.hot_x = position.x_hotspot;
	move.hot_y = position.y_hotspot;
	move.x = position.x;
	move.y = position.y;

	if (!dc_submit_plane_update(adev->dc, gsgpu_crtc->crtc_id, &dc_plane))
		DRM_ERROR("DC failed to set cursor attributes\n");
	if (!dc_crtc_cursor_move(adev, gsgpu_crtc->crtc_id, &move))
		DRM_ERROR("DC failed to set cursor position\n");

	return;
}
