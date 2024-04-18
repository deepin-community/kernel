// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2020 ChangSha JingJiaMicro Electronics Co., Ltd.
 * All rights reserved.
 *
 * Author:
 *      shanjinkui <shanjinkui@jingjiamicro.com>
 *
 * The software and information contained herein is proprietary and
 * confidential to JingJiaMicro Electronics. This software can only be
 * used by JingJiaMicro Electronics Corporation. Any use, reproduction,
 * or disclosure without the written permission of JingJiaMicro
 * Electronics Corporation is strictly prohibited.
 */
#include <linux/version.h>
#include <linux/delay.h>
#include <drm/drm_crtc.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_vblank.h>
#include <drm/drm_plane.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_gem_atomic_helper.h>
#include "mwv207_bo.h"
#include "mwv207_va.h"
#include "mwv207_vi.h"
#include "mwv207_vbios.h"

#define MWV207_WIN_BASE(idx)     (0x990000 + 0x100 * (idx) + ((idx) > 1 ? 0xC00 : 0))

#define MWV207_VA_BASE(idx)      (0x990000 + 0x100 * (idx) + ((idx) > 1 ? 0x600 : 0))

#define crtc_to_va(crtc) container_of(crtc, struct mwv207_va, crtc)
#define plane_to_va(plane) (((plane)->type == DRM_PLANE_TYPE_PRIMARY) \
		? container_of(plane, struct mwv207_va, primary)    \
		: container_of(plane, struct mwv207_va, cursor))
struct mwv207_va {
	struct drm_crtc   crtc;
	struct drm_plane  primary;
	struct drm_plane  cursor;
	void __iomem   *mmio;
	struct mwv207_device *jdev;
	struct drm_pending_vblank_event *event;
	int idx;
	uint16_t lutdata[768];
};

static u32 rgb_formats[] = {
	DRM_FORMAT_RGB565,
	DRM_FORMAT_RGB888,
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_XRGB2101010,
};

static uint64_t format_modifiers[] = {
	DRM_FORMAT_MOD_LINEAR,
	DRM_FORMAT_MOD_INVALID
};

static u32 cursor_formats[] = {
	DRM_FORMAT_ARGB8888,
};

static inline u32 mwv207_va_read(struct mwv207_va *va, u32 reg)
{
	return readl(va->mmio + reg);
}

static inline void mwv207_va_write(struct mwv207_va *va, u32 reg, u32 value)
{
	writel_relaxed(value, va->mmio + reg);
}

static inline void mwv207_va_modify(struct mwv207_va *va, u32 reg,
		u32 mask, u32 value)
{
	u32 rvalue = mwv207_va_read(va, reg);

	rvalue = (rvalue & ~mask) | (value & mask);
	mwv207_va_write(va, reg, rvalue);
}

static bool mwv207_plane_format_mod_supported(struct drm_plane *plane,
						  uint32_t format, uint64_t modifier)
{
	if (modifier == DRM_FORMAT_MOD_LINEAR)
		return true;
	return false;
}

static int mwv207_plane_atomic_check(struct drm_plane *plane,
		struct drm_atomic_state *state)
{

	return 0;
}

static int mwv207_plane_prepare_fb(struct drm_plane *plane,
				       struct drm_plane_state *new_state)
{
	struct mwv207_bo *jbo;
	int ret;

	if (!new_state->fb)
		return 0;

	jbo = mwv207_bo_from_gem(new_state->fb->obj[0]);

	ret = mwv207_bo_reserve(jbo, false);
	if (ret)
		return ret;

	jbo->flags |= (1<<0);
	ret = mwv207_bo_pin_reserved(jbo, 0x2);
	mwv207_bo_unreserve(jbo);
	if (ret)
		return ret;

	mwv207_bo_ref(jbo);

	drm_gem_plane_helper_prepare_fb(plane, new_state);

	return 0;
}

static void mwv207_plane_cleanup_fb(struct drm_plane *plane,
				       struct drm_plane_state *old_state)
{
	struct mwv207_bo *jbo;

	if (!old_state->fb)
		return;

	jbo = mwv207_bo_from_gem(old_state->fb->obj[0]);
	if (!jbo)
		return;

	mwv207_bo_unpin(jbo);
	mwv207_bo_unref(jbo);
}

static u64 mwv207_plane_fb_addr(struct drm_plane *plane)
{
	struct drm_framebuffer *fb;
	struct mwv207_bo *bo;
	u32 src_x, src_y;
	u64 fbaddr;

	src_x = plane->state->src_x >> 16;
	src_y = plane->state->src_y >> 16;

	fb = plane->state->fb;
	bo = mwv207_bo_from_gem(fb->obj[0]);
	fbaddr = mwv207_bo_gpu_phys(bo);
	fbaddr += src_x * fb->format->cpp[0] + fb->pitches[0] * src_y;

	return fbaddr;
}

static void mwv207_primary_atomic_update(struct drm_plane *plane,
		struct drm_atomic_state *old_state)
{
	struct mwv207_va *va = plane_to_va(plane);
	struct drm_framebuffer *fb;
	u32 src_w, src_h;
	u64 fbaddr;

	if (!plane->state->fb || !plane->state->crtc)
		return;

	src_w = plane->state->src_w >> 16;
	src_h = plane->state->src_h >> 16;
	fbaddr = mwv207_plane_fb_addr(plane);
	fb = plane->state->fb;

	mwv207_va_write(va, 0x434, fb->pitches[0] >> 4);

	mwv207_va_write(va, 0x43C, ((src_h - 1) << 16) | (src_w - 1) | 0x3);

	mwv207_va_write(va, 0x4F8, (src_w - 1) << 16);

	mwv207_va_write(va, 0x438, fbaddr >> 6);

	switch (fb->format->format) {
	case DRM_FORMAT_RGB565:
		mwv207_va_modify(va, 0x430, 1, 1);
		break;
	case DRM_FORMAT_XRGB2101010:
	case DRM_FORMAT_ARGB2101010:
		mwv207_va_modify(va, 0x430, 1 << 1, 1 << 1);
		break;
	case DRM_FORMAT_ARGB8888:
		mwv207_va_modify(va, 0x430, 0x3, 0);
		break;
	default:
		break;
	}
}

static void mwv207_primary_atomic_disable(struct drm_plane *plane,
		struct drm_atomic_state *old_state)
{

}

static void mwv207_cursor_switch(struct mwv207_va *va, bool on)
{
	mwv207_va_modify(va, 0x478, 0xff, on ? 6 : 0);
}

static void mwv207_cursor_atomic_update(struct drm_plane *plane,
		struct drm_atomic_state *old_state)
{
	int crtc_x, crtc_y, pos_x, pos_y, hot_x, hot_y;
	struct mwv207_va *va = plane_to_va(plane);
	u64 fbaddr;

	if (!plane->state->fb || !plane->state->crtc)
		return;

	crtc_x = plane->state->crtc_x;
	crtc_y = plane->state->crtc_y;
	fbaddr = mwv207_plane_fb_addr(plane);

	mwv207_va_write(va, 0x4B4, fbaddr >> 6);

	pos_x = crtc_x < 0 ? 0 : crtc_x;
	hot_x = crtc_x < 0 ? -crtc_x : 0;
	pos_y = crtc_y < 0 ? 0 : crtc_y;
	hot_y = crtc_y < 0 ? -crtc_y : 0;
	mwv207_va_write(va, 0x454, (hot_x & 0x3f) | ((hot_y & 0x3f) << 16));
	mwv207_va_write(va, 0x4A8, (pos_x & 0xffff) | ((pos_y & 0xffff) << 16));

	mwv207_cursor_switch(va, 1);
}

static void mwv207_cursor_atomic_disable(struct drm_plane *plane,
		struct drm_atomic_state *old_state)
{
	struct mwv207_va *va = plane_to_va(plane);

	mwv207_cursor_switch(va, 0);
}

static const struct drm_plane_funcs mwv207_plane_funcs = {
	.reset                  = drm_atomic_helper_plane_reset,
	.update_plane	        = drm_atomic_helper_update_plane,
	.disable_plane	        = drm_atomic_helper_disable_plane,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state   = drm_atomic_helper_plane_destroy_state,
	.format_mod_supported   = mwv207_plane_format_mod_supported,
	.destroy	        = drm_plane_cleanup,
};

static const struct drm_plane_helper_funcs mwv207_primary_helper_funcs = {
	.prepare_fb     = mwv207_plane_prepare_fb,
	.cleanup_fb     = mwv207_plane_cleanup_fb,
	.atomic_check   = mwv207_plane_atomic_check,
	.atomic_update  = mwv207_primary_atomic_update,
	.atomic_disable = mwv207_primary_atomic_disable,
};

static const struct drm_plane_helper_funcs mwv207_cursor_helper_funcs = {
	.prepare_fb     = mwv207_plane_prepare_fb,
	.cleanup_fb     = mwv207_plane_cleanup_fb,
	.atomic_check   = mwv207_plane_atomic_check,
	.atomic_update  = mwv207_cursor_atomic_update,
	.atomic_disable = mwv207_cursor_atomic_disable,
};

static void mwv207_va_misc_reset(struct mwv207_va *va)
{
	struct mwv207_device *jdev = va->jdev;

	jdev_write(jdev, (0x9b001c), 0xfffff);
	jdev_write(jdev, (0x9b0020), 0xfffff);
	jdev_write(jdev, (0x9b0024), 0xfffff);

	jdev_write(jdev, (0x9b0028), 0x03020100);

	jdev_write(jdev, MWV207_WIN_BASE(va->idx) + 0x2c, 0);
}

static int mwv207_va_lut_fifo_wait(struct mwv207_va *va, u32 used)
{
	int i;

	mb();
	for (i = 0; i < 120; ++i) {
		if (mwv207_va_read(va, 0x450) <= used)
			return 0;
		usleep_range(1000, 1001);
	}

	return -EBUSY;
}

static void mwv207_va_lut_fifo_activate(struct mwv207_va *va, int rgb)
{

	mb();
	mwv207_va_write(va, 0x44C, rgb);
	mwv207_va_write(va, 0x448, 1);
	udelay(2);
	mwv207_va_write(va, 0x448, 0);
	udelay(2);
}

static int mwv207_va_lut_fifo_input(struct mwv207_va *va,  u32 value)
{
	int ret;

	ret = mwv207_va_lut_fifo_wait(va, 10);
	if (ret)
		return ret;

	mb();
	mwv207_va_write(va, 0x444, value);

	return 0;
}

static void mwv207_va_lut_enable(struct mwv207_va *va)
{
	u32 active_ram, value;
	int i, ram, rgb;
	u16 *data;

	mwv207_va_modify(va, 0x460, 1 << 31, 0 << 31);
	value = (va->lutdata[384] * 15 / 8) | 0x80000000;
	jdev_write(va->jdev, MWV207_WIN_BASE(va->idx) + 0x38, value);

	for (ram = 0; ram < 2; ram++) {
		active_ram = mwv207_va_read(va, 0x440);
		for (rgb = 0; rgb < 3; rgb++) {
			data = &va->lutdata[rgb * 256];
			mwv207_va_lut_fifo_activate(va, rgb);
			for (i = 0; i < 1024; i++) {

				value = min_t(u32, 1024 - 5, (data[i / 4] << 2) | 0x03);
				if (mwv207_va_lut_fifo_input(va, value))
					pr_warn("mwv207: failed to write lut");
			}

			if (mwv207_va_lut_fifo_wait(va, 0))
				pr_warn("mwv207: failed to configure lut");
		}

		mb();
		mwv207_va_write(va, 0x440, 1 - active_ram);
	}
}

static void mwv207_mode_fixup(struct drm_display_mode *adjusted_mode, struct drm_display_mode *mode)
{
	memcpy(adjusted_mode, mode, sizeof(*adjusted_mode));

	mode->crtc_hdisplay     = adjusted_mode->crtc_hdisplay * 2;
	mode->crtc_hblank_start = adjusted_mode->crtc_hblank_start * 2;
	mode->crtc_hblank_end   = adjusted_mode->crtc_hblank_end * 2;
	mode->crtc_hsync_start  = adjusted_mode->crtc_hsync_start * 2;
	mode->crtc_hsync_end    = adjusted_mode->crtc_hsync_end * 2;
	mode->crtc_htotal       = adjusted_mode->crtc_htotal * 2;
	mode->crtc_clock        = adjusted_mode->crtc_clock * 2;
}

static void mwv207_va_config_timing(struct mwv207_va *va)
{
	struct drm_display_mode *m      = &va->crtc.state->adjusted_mode;
	struct drm_display_mode *native = NULL;
	u32 hsbegin, hsend, hblankbegin, hblankend, htotal, hhalf;
	u32 vblank_1field_begin, vblank_1field_end;
	u32 vblank_2field_begin, vblank_2field_end;
	u32 vs_1field_begin, vs_1field_end;
	u32 vs_2field_begin, vs_2field_end;
	u32 is_interleaved;
	u32 vfrontporch, vsync, vtotal;
	int ret, value;

	ret = mwv207_vbios_set_pll(va->jdev, va->idx + MWV207_PLL_GRAPH0, m->crtc_clock);
	if (ret) {
		DRM_ERROR("mwv207: failed to set crtc clock: %d", ret);
		return;
	}

	is_interleaved = DRM_MODE_FLAG_INTERLACE & m->flags ? 1 : 0;
	if (is_interleaved && m->crtc_hdisplay == 720) {
		native = kmalloc(sizeof(*m), GFP_KERNEL);
		if (!native) {
			DRM_ERROR("mwv207: failed to kmalloc native mode\r\n");
			return;
		}
		mwv207_mode_fixup(m, native);
		m = native;
	}

	hblankbegin = 0;
	hblankend   = m->crtc_htotal - m->crtc_hdisplay;
	htotal      = m->crtc_htotal;
	hsbegin     = m->crtc_hsync_start - m->crtc_hdisplay;
	hsend       = m->crtc_hsync_end - m->crtc_hdisplay;
	hhalf       = is_interleaved ? (htotal / 2 + hsbegin) : htotal / 2;

	vblank_1field_begin = 0;
	vblank_1field_end   = is_interleaved ?
			((m->crtc_vtotal - m->crtc_vdisplay) * 2)
			: (m->crtc_vtotal - m->crtc_vdisplay);

	vblank_2field_begin =
	(m->crtc_vdisplay + (m->crtc_vtotal - m->crtc_vdisplay)) * 2;
	vblank_2field_end =
	(m->crtc_vdisplay + (m->crtc_vtotal - m->crtc_vdisplay)) * 2 +
	(m->crtc_vtotal - m->crtc_vdisplay + 1) * 2;

	vtotal          = m->crtc_vtotal;
	vfrontporch     = m->crtc_vsync_start - m->crtc_vdisplay;
	vsync           = m->crtc_vsync_end - m->crtc_vsync_start;
	vs_1field_begin = is_interleaved ? (vfrontporch * 2) : (vfrontporch);
	vs_1field_end   = is_interleaved ? ((vfrontporch + vsync) * 2)
			: (vfrontporch + vsync);
	vs_2field_begin =
		(m->crtc_vdisplay + (m->crtc_vtotal - m->crtc_vdisplay)) * 2 +
		vfrontporch * 2 + 1;
	vs_2field_end   =
		(m->crtc_vdisplay + (m->crtc_vtotal - m->crtc_vdisplay)) * 2 +
		(vfrontporch + vsync) * 2 + 1;

	mwv207_va_write(va, 0x400, hsbegin | (hsend << 16));
	mwv207_va_write(va, 0x404, hblankbegin | (hblankend << 16));
	mwv207_va_write(va, 0x408, htotal | (hhalf << 16));

	mwv207_va_write(va, 0x40C, vblank_1field_begin | (vblank_1field_end << 16));
	mwv207_va_write(va, 0x458, vblank_1field_begin | (vblank_1field_end << 16));
	mwv207_va_write(va, 0x410, vblank_2field_begin | (vblank_2field_end << 16));
	mwv207_va_write(va, 0x45C, vblank_2field_begin | (vblank_2field_end << 16));

	mwv207_va_write(va, 0x418, vs_1field_begin | (vs_1field_end << 16));
	mwv207_va_write(va, 0x41C, vs_2field_begin | (vs_2field_end << 16));

	mwv207_va_modify(va, 0x414, 0xffff, vtotal);

	value = is_interleaved;
	if (is_interleaved && m->crtc_hdisplay == 1440)
		value |= 0x2;
	mwv207_va_write(va, 0x464, value);

	mwv207_va_write(va, 0x420, 1);

	value = mwv207_va_read(va, 0x2a0);
	value &= ~0x6;
	value |= 1;
	mwv207_va_write(va, 0x2a0, value);

	kfree(native);
}

static int mwv207_crtc_enable_vblank(struct drm_crtc *crtc)
{
	struct mwv207_va *va = crtc_to_va(crtc);

	jdev_modify(va->jdev, (0x9907f0),
			1 << (12 + va->idx), 1 << (12 + va->idx));
	jdev_modify(va->jdev, (0x9907f8),
			1 << (12 + va->idx), 1 << (12 + va->idx));
	return 0;
}

static void mwv207_crtc_disable_vblank(struct drm_crtc *crtc)
{
	struct mwv207_va *va = crtc_to_va(crtc);

	jdev_modify(va->jdev, (0x9907f0),
			1 << (12 + va->idx), 0 << (12 + va->idx));
	jdev_modify(va->jdev, (0x9907f8),
			1 << (12 + va->idx), 0 << (12 + va->idx));
}

static int mwv207_crtc_atomic_check(struct drm_crtc *crtc,
			      struct drm_atomic_state *state)
{
	struct drm_crtc_state *crtc_state = drm_atomic_get_new_crtc_state(state, crtc);

	if (crtc_state->gamma_lut && drm_color_lut_size(crtc_state->gamma_lut) != 256)
		return -EINVAL;

	return 0;
}

static void mwv207_crtc_atomic_enable(struct drm_crtc *crtc,
		      struct drm_atomic_state *old_crtc_state)
{
	struct mwv207_va *va = crtc_to_va(crtc);
	struct drm_encoder *encoder;

	drm_for_each_encoder_mask(encoder, crtc->dev, crtc->state->encoder_mask)
		mwv207_output_set_crtc(encoder, crtc);

	mwv207_va_config_timing(va);
	drm_crtc_vblank_on(crtc);
}

static void mwv207_crtc_atomic_disable(struct drm_crtc *crtc,
		       struct drm_atomic_state *old_crtc_state)
{
	struct mwv207_va *va = crtc_to_va(crtc);

	drm_crtc_vblank_off(crtc);

	mwv207_va_write(va, 0x2c4, 0);
	mwv207_va_write(va, 0x2c0, 0);
	mwv207_va_write(va, 0x2a0, 2);
	mwv207_va_write(va, 0x2c4, 1);

}

static void mwv207_crtc_atomic_flush(struct drm_crtc *crtc,
		       struct drm_atomic_state *old_crtc_state)
{
	struct mwv207_va *va = crtc_to_va(crtc);
	struct drm_color_lut *lut;
	int i;

	if (crtc->state->color_mgmt_changed) {
		if (crtc->state->gamma_lut) {
			lut = (struct drm_color_lut *)crtc->state->gamma_lut->data;
			for (i = 0; i < 256; i++) {
				va->lutdata[i] = drm_color_lut_extract(lut[i].red, 8);
				va->lutdata[i + 256] = drm_color_lut_extract(lut[i].green, 8);
				va->lutdata[i + 512] = drm_color_lut_extract(lut[i].blue, 8);
			}
			mwv207_va_lut_enable(va);
		}
	}
}

static void mwv207_va_reset(struct drm_crtc *crtc)
{
	struct mwv207_va *va = crtc_to_va(crtc);

	mwv207_cursor_switch(va, 0);

	mwv207_va_lut_enable(va);
	mwv207_va_misc_reset(va);
	drm_atomic_helper_crtc_reset(crtc);
}

static const struct drm_crtc_funcs mwv207_crtc_funcs = {
	.reset                  = mwv207_va_reset,
	.destroy                = drm_crtc_cleanup,
	.set_config             = drm_atomic_helper_set_config,
	.atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state   = drm_atomic_helper_crtc_destroy_state,
	.page_flip              = drm_atomic_helper_page_flip,
	.enable_vblank          = mwv207_crtc_enable_vblank,
	.disable_vblank         = mwv207_crtc_disable_vblank,
};

static const struct drm_crtc_helper_funcs mwv207_crtc_helper_funcs = {
	.atomic_check   = mwv207_crtc_atomic_check,
	.atomic_flush   = mwv207_crtc_atomic_flush,
	.atomic_enable  = mwv207_crtc_atomic_enable,
	.atomic_disable = mwv207_crtc_atomic_disable,
};

void mwv207_crtc_prepare_vblank(struct drm_crtc *crtc)
{
	struct drm_pending_vblank_event *event = crtc->state->event;
	struct mwv207_va *va = crtc_to_va(crtc);
	unsigned long flags;

	if (event) {
		if (crtc->state->active) {
			WARN_ON(drm_crtc_vblank_get(crtc) != 0);
			spin_lock_irqsave(&crtc->dev->event_lock, flags);
			va->event = event;
			spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
		} else {
			spin_lock_irqsave(&crtc->dev->event_lock, flags);
			drm_crtc_send_vblank_event(crtc, crtc->state->event);
			spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
		}
		crtc->state->event = NULL;
	}
}

void mwv207_crtc_finish_vblank(struct drm_crtc *crtc)
{
	struct mwv207_va *va = crtc_to_va(crtc);
	int put_vblank = false;
	unsigned long flags;

	spin_lock_irqsave(&crtc->dev->event_lock, flags);
	if (va->event) {
		drm_crtc_send_vblank_event(crtc, va->event);
		va->event = NULL;
		put_vblank = true;
	}
	spin_unlock_irqrestore(&crtc->dev->event_lock, flags);

	if (put_vblank)
		drm_crtc_vblank_put(crtc);
}

irqreturn_t mwv207_va_handle_vblank(int irq, void *data)
{
	struct mwv207_device *jdev = data;
	struct drm_crtc *crtc;
	u32 fired;

	fired = jdev_read(jdev, (0x9907f4));
	if (!fired)
		return IRQ_NONE;
	jdev_write(jdev, (0x9907f4), fired);

	fired = (fired >> 12) & 0xf;
	drm_for_each_crtc(crtc, &jdev->base) {
		if (fired & (1 << drm_crtc_index(crtc))) {
			drm_crtc_handle_vblank(crtc);
			mwv207_crtc_finish_vblank(crtc);
		}
	}

	return IRQ_HANDLED;
}

static int mwv207_va_init_single(struct mwv207_device *jdev, int idx)
{
	struct mwv207_va *va;
	uint16_t *lut;
	int ret, i;

	va = devm_kzalloc(jdev->dev, sizeof(*va), GFP_KERNEL);
	if (!va)
		return -ENOMEM;

	va->mmio = jdev->mmio + MWV207_VA_BASE(idx);
	va->jdev = jdev;
	va->idx = idx;

	ret = drm_universal_plane_init(&jdev->base, &va->primary, 1 << idx,
			&mwv207_plane_funcs,
			rgb_formats, ARRAY_SIZE(rgb_formats),
			format_modifiers, DRM_PLANE_TYPE_PRIMARY, "primary_plane_%d", idx);
	if (ret)
		return ret;
	ret = drm_universal_plane_init(&jdev->base, &va->cursor, 1 << idx,
			&mwv207_plane_funcs,
			cursor_formats, ARRAY_SIZE(cursor_formats),
			format_modifiers, DRM_PLANE_TYPE_CURSOR, "cursor_%d", idx);
	if (ret)
		return ret;

	drm_plane_helper_add(&va->primary, &mwv207_primary_helper_funcs);
	drm_plane_helper_add(&va->cursor, &mwv207_cursor_helper_funcs);

	ret = drm_crtc_init_with_planes(&jdev->base, &va->crtc,
			&va->primary, &va->cursor, &mwv207_crtc_funcs, "crtc_%d",
			idx);
	if (ret)
		return ret;

	drm_crtc_helper_add(&va->crtc, &mwv207_crtc_helper_funcs);

	lut = &va->lutdata[0];
	for (i = 0; i < 256; i++)
		lut[i] = lut[i + 256] = lut[i + 512] = i;
	drm_crtc_enable_color_mgmt(&va->crtc, 768, true, 768);
	ret = drm_mode_crtc_set_gamma_size(&va->crtc, 256);

	BUG_ON(drm_crtc_index(&va->crtc) != va->idx);

	return ret;
}

int mwv207_va_init(struct mwv207_device *jdev)
{
	int i, nr, ret;

	nr = jdev->lite ? 2 : 4;
	for (i = 0; i < nr; i++) {
		ret = mwv207_va_init_single(jdev, i);
		if (ret)
			return ret;
	}

	return 0;
}
