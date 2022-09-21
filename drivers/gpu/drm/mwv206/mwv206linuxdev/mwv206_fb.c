/*
 * JM7200 GPU driver
 *
 * Copyright (c) 2018 ChangSha JingJiaMicro Electronics Co., Ltd.
 *
 * Author:
 *      rfshen <jjwgpu@jingjiamicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/vmalloc.h>
#include <linux/device.h>
#include <linux/ctype.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <linux/uaccess.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <drm/drmP.h>
#include "mwv206dev.h"
#include "mwv206reg.h"
#include "mwv206kconfig.h"
#include "common.h"
#include "mwv206kdevconfig.h"
#include "mwv206displaymode.h"
#include "cputype.h"
#include "mwv206_fb_interpret_edid.h"

#define MWV206_FB_MAXHEIGHT 1080
#define MWV206_FB_MAXWIDTH  1920
#define MWV206_CRTC_PRIMARY 0
#define MWV206_CRTC_ZOOM    1

#define JMF_DISPLAY_HORIZONTAL_SHORTER(_dm_zoom, _dm_prim) \
	((_dm_zoom)->hactive <= (_dm_prim)->hactive)

#define JMF_DISPLAY_VERTICAL_SHORTER(_dm_zoom, _dm_prim) \
	((_dm_zoom)->vactive <= (_dm_prim)->vactive)

#define JMF_SET_HORIZONTAL_VAR(_var, _dm) \
do {\
	(_var)->xres = (_dm)->hactive;\
	(_var)->xres_virtual = (_dm)->hactive;\
} while (0)

#define JMF_SET_VERTICAL_VAR(_var, _dm) \
do {\
	(_var)->yres = (_dm)->vactive;\
	(_var)->yres_virtual = (_dm)->vactive;\
} while (0)

static int FUNC206LXDEV081(unsigned V206FB010, unsigned red, unsigned green,
		unsigned blue, unsigned transp, struct fb_info *info);
static void FUNC206LXDEV004(struct fb_info *info, const struct fb_fillrect *rect);
static void FUNC206LXDEV003(struct fb_info *info, const struct fb_copyarea *area);
static void FUNC206LXDEV015(struct fb_info *p, const struct fb_image *image);

static struct fb_ops mwv206fb_ops = {
	.owner              = THIS_MODULE,
	.fb_setcolreg       = FUNC206LXDEV081,
	.fb_fillrect        = FUNC206LXDEV004,
	.fb_copyarea        = FUNC206LXDEV003,
	.fb_imageblit       = FUNC206LXDEV015,
};

static int fb_probe_edid = 1;
module_param(fb_probe_edid, int, 0644);
MODULE_PARM_DESC(fb_probe_edid, "Pick resolution by parsing edid");

static void FUNC206LXDEV004(struct fb_info *info, const struct fb_fillrect *rect)
{
	struct V206DEV026 *V206FB005 = info->par;
	V206DEV025 *pDev = pci_get_drvdata(V206FB005->V206DEV103);
	int ret;

	if (drm_can_sleep()) {
		ret = FUNC206LXDEV154(pDev, (unsigned long)V206FB005->mmio_base,
				V206FB005->V206DEV181,
				V206FB005->V206DEV182,
				V206FB005->V206KG2D001,
				rect->dx, rect->dy,
				rect->width, rect->height,
				rect->color,
				0xffffffff,
				rect->rop);
		if (ret == 0) {
			return;
		}
	}

	cfb_fillrect(info, rect);
}

static void FUNC206LXDEV003(struct fb_info *info, const struct fb_copyarea *area)
{
	struct V206DEV026 *V206FB005 = info->par;
	V206DEV025 *pDev = pci_get_drvdata(V206FB005->V206DEV103);
	int ret;

	if (drm_can_sleep()) {
		ret = FUNC206LXDEV145(pDev, (unsigned long)V206FB005->mmio_base,
				V206FB005->V206DEV181, V206FB005->V206DEV181,
				V206FB005->V206DEV182, V206FB005->V206DEV182,
				V206FB005->V206KG2D001,
				area->sx, area->sy,
				area->dx, area->dy,
				area->width, area->height,
				0xffffffff, GXcopy);
		if (ret == 0) {
			return;
		}
	}

	cfb_copyarea(info, area);
}

static void FUNC206LXDEV015(struct fb_info *info, const struct fb_image *image)
{
	cfb_imageblit(info, image);
}

static int FUNC206LXDEV094(V206DEV025 *pDev, int crtc,
		V206IOCTL161 *dmode,  unsigned int addr)
{
	struct V206DEV026 *V206FB005 = pDev->fb_info;
	V206IOCTL159 V206DEV143;

	V206DEV143.addr = addr;
	V206DEV143.V206FB011 = crtc;
	V206DEV143.format = V206FB005->pixel_format;
	V206DEV143.V206KG2D033 = V206FB005->V206DEV182;
	V206DEV143.width = dmode->hactive;
	V206DEV143.height = dmode->vactive;
	V206DEV143.vsync = 1;

	return FUNC206HAL388(pDev, (long)&V206DEV143);
}

static int FUNC206LXDEV081(unsigned V206FB010, unsigned red, unsigned green,
		unsigned blue, unsigned transp, struct fb_info *info)
{
	if (V206FB010 >= info->cmap.len) {
		return 1;
	}

	if (V206FB010 < 16) {
		if (info->var.bits_per_pixel == 32) {
			red >>= 8;
			green >>= 8;
			blue >>= 8;
			((u32 *)(info->pseudo_palette))[V206FB010] =
				(red << info->var.red.offset)       |
				(green << info->var.green.offset)   |
				(blue << info->var.blue.offset);
		}
	}

	return 0;
}


static void mwv206fb_pick_res(struct V206DEV026 *V206FB005, struct fb_info *info)
{
	struct fb_var_screeninfo *pvar = &info->var;
	V206IOCTL161 *pdm_prim =  &V206FB005->V206DEV183[MWV206_CRTC_PRIMARY];
	V206IOCTL161 *pdm_zoom =  &V206FB005->V206DEV183[MWV206_CRTC_ZOOM];

	if (V206FB005->zoom_port && JMF_DISPLAY_HORIZONTAL_SHORTER(pdm_zoom, pdm_prim)) {
		JMF_SET_HORIZONTAL_VAR(pvar, pdm_zoom);
	} else {
		JMF_SET_HORIZONTAL_VAR(pvar, pdm_prim);
	}
	if (V206FB005->zoom_port && JMF_DISPLAY_VERTICAL_SHORTER(pdm_zoom, pdm_prim)) {
		JMF_SET_VERTICAL_VAR(pvar, pdm_zoom);
	} else {
		JMF_SET_VERTICAL_VAR(pvar, pdm_prim);
	}
	if (pvar->xres > MWV206_FB_MAXWIDTH) {
		pvar->xres = MWV206_FB_MAXWIDTH;
		pvar->xres_virtual = MWV206_FB_MAXWIDTH;
	}
	if (pvar->yres > MWV206_FB_MAXHEIGHT) {
		pvar->yres = MWV206_FB_MAXHEIGHT;
		pvar->yres_virtual = MWV206_FB_MAXHEIGHT;
	}
}


static int FUNC206LXDEV080(struct fb_info *info)
{
	struct V206DEV026 *V206FB005 = info->par;
#if defined(_MWV206_SKIP_VT_SWITCH_)
	info->skip_vt_switch = true;
#endif
	info->pseudo_palette = V206FB005->pseudo_palette;
	info->flags = FBINFO_HWACCEL_COPYAREA | FBINFO_HWACCEL_FILLRECT;
	info->fbops = &mwv206fb_ops;
	info->screen_base = V206FB005->V206DEV178;
	info->screen_size = V206FB005->screen_size;

	strlcpy(info->fix.id, V206FB005->name, sizeof(info->fix.id));
	info->fix.smem_start = V206FB005->V206DEV177 + V206FB005->V206DEV181;
	info->fix.smem_len = V206FB005->V206DEV179;
	info->fix.type = FB_TYPE_PACKED_PIXELS;
	info->fix.visual = FB_VISUAL_TRUECOLOR;
	info->fix.mmio_start = V206FB005->V206DEV180;
	info->fix.mmio_len = V206FB005->mmio_len;
	info->fix.accel = FB_ACCEL_NONE;
	info->fix.line_length = V206FB005->V206DEV182;


	info->var.activate = FB_ACTIVATE_NOW;
	info->var.vmode = FB_VMODE_NONINTERLACED;

	mwv206fb_pick_res(V206FB005, info);
	if (V206FB005->pixel_format == V206FB002) {
		info->var.bits_per_pixel = 32;
		info->var.red.length = 8;
		info->var.green.length = 8;
		info->var.blue.length = 8;
		info->var.transp.length = 8;
		info->var.red.offset = 16;
		info->var.green.offset = 8;
		info->var.blue.offset = 0;
		info->var.transp.offset = 24;
	}

	return 0;
}

static void FUNC206LXDEV084(V206DEV025 *priv, int crtc,
		struct mwv206_port_config *V206FB007)
{
	V206IOCTL172 port;
	V206IOCTL173 V206FB009;
	uint8_t V206FB008;

	V206DEV005("[INFO] %s: type = %d, index = %d.\n", __FUNCTION__, type, index);

	if (!V206DEVCONFIG036(V206FB007->flags)) {
		port.enable = 0;
	} else {
		port.enable = 1;
	}

	V206FB008 = mwv206_get_portid(&priv->V206DEV105, V206FB007);

	port.V206FB008 = V206FB008;
	port.V206FB011 = crtc;
	port.dualpixel = V206DEVCONFIG037(V206FB007->flags);
	if ((1 == port.dualpixel)
			&& ((MWV206_DP_HDMI_0 == V206FB008) || (MWV206_DP_HDMI_2 == V206FB008))) {
		port.dualpixel = 0;
	}
	FUNC206HAL393(priv, (long)&port);

	if (V206FB008 == MWV206_DP_LVDS_0 || V206FB008 == MWV206_DP_LVDS_1) {
		V206FB009.V206FB008 = V206FB008;
		V206FB009.type = MWV206K_DP_LVDS_MODE;
		V206FB009.value = V206FB007->bitmode;
		FUNC206HAL394(priv, (long)&V206FB009);
	}
}

static void mwv206fb_setdisplayport_off(V206DEV025 *priv, int crtc,
		struct mwv206_port_config *V206FB007)
{
	V206IOCTL172 port;
	uint8_t V206FB008;

	port.enable = 0;
	V206FB008 = mwv206_get_portid(&priv->V206DEV105, V206FB007);

	port.V206FB008 = V206FB008;
	port.V206FB011 = crtc;
	FUNC206HAL393(priv, (long)&port);
}

static int mwv206fb_get_edid(V206DEV025 *pDev, unsigned char *edid,
			struct mwv206_port_config *port)
{
	int ret = 0;

	if (V206DEVCONFIG042(port->flags) == V206DEVCONFIG013) {
		memset(edid, 0, V206CONFIG010);
		memcpy(edid, port->edid, sizeof(port->edid));
		return 0;
	} else if (V206DEVCONFIG042(port->flags) == V206DEVCONFIG012) {
		return -1;
	}


	if (port->i2cchan >= 8) {
		return -1;
	}

	FUNC206LXDEV151(pDev, 1);
	if (pDev->V206DEV147.connect_status[port->i2cchan]) {
		memcpy(edid, pDev->V206DEV147.edid[port->i2cchan], V206CONFIG010);
		ret = 0;
	} else {
		ret = -1;
	}
	FUNC206LXDEV151(pDev, 0);

	return ret;
}

static int mwv206fb_dvi_hdmi_parse(V206DEV025 *pDev, struct mwv206_port_config *port)
{
	unsigned char edid[V206CONFIG010];
	int flags = 0;

	if (mwv206fb_get_edid(pDev, edid, port)) {
		MWV206_PORT_SET_HDMI(flags);
		return flags;
	}

	if (mwv206_edid_is_dvi(edid)) {
		MWV206_PORT_SET_DVI(flags);
	} else {
		MWV206_PORT_SET_HDMI(flags);
		if (edid[131] & (1 << 6)) {
			MWV206_PORT_SET_AUDIO(flags);
		}
	}

	return flags;
}

int mwv206fb_clear(struct V206DEV026 *V206FB005)
{
	struct fb_info *info = V206FB005->info;
	struct fb_fillrect rect;

	rect.dx = 0;
	rect.dy = 0;
	rect.width = V206FB005->width;
	rect.height = V206FB005->height;
	rect.color = 0;
	rect.rop = 3;
	FUNC206LXDEV004(info, &rect);

	return 0;
}

static int mwv206fb_sethdmimode(V206DEV025 *priv, int V206FB011,
		struct mwv206_port_config *port, V206IOCTL161 *p_mode)
{
	V206IOCTL168 V206FB004;
	int mode = p_mode->mode;
	int V206HDMIAUDIO027;

	V206HDMIAUDIO027 = port - &priv->V206DEV105.hdmi[0];

	V206FB004.V206HDMIAUDIO027 = V206HDMIAUDIO027;
	V206FB004.V206FB011 = V206FB011;
	V206FB004.mode = mode;
	V206FB004.dualpixel = 0;
	if ((mode <= MWV206K_DPMODE_FIRST) || (mode >  MWV206K_DPMODE_USER)) {
		V206KDEBUG002("[ERROR] Invalid HDMI mode!.\n");
		return -ENOMEM;
	}

	if (!V206DEVCONFIG036(port->flags)) {
		return 0;
	}
	if (mode == MWV206K_DPMODE_USER) {
		V206FB004.htotal = p_mode->htotal;
		V206FB004.hactive = p_mode->hactive;
		V206FB004.hfrontporch = p_mode->hfrontporch;
		V206FB004.hsync = p_mode->hsync;
		V206FB004.hpol = p_mode->hpol;
		V206FB004.vtotal = p_mode->vtotal;
		V206FB004.vactive = p_mode->vactive;
		V206FB004.vfrontporch = p_mode->vfrontporch;
		V206FB004.vsync = p_mode->vsync;
		V206FB004.vpol = p_mode->vpol;
		V206FB004.framerate = p_mode->framerate;
		V206FB004.dualpixel = 0;
	} else {
		V206FB004.htotal = modeparams[mode][V206DISPMODE001];
		V206FB004.hactive = modeparams[mode][V206DISPMODE002];
		V206FB004.hfrontporch = modeparams[mode][V206DISPMODE003];
		V206FB004.hsync = modeparams[mode][V206DISPMODE004];
		V206FB004.hpol = modeparams[mode][V206DISPMODE012];
		V206FB004.vtotal = modeparams[mode][V206DISPMODE006];
		V206FB004.vactive = modeparams[mode][V206DISPMODE007];
		V206FB004.vfrontporch = modeparams[mode][V206DISPMODE008];
		V206FB004.vsync = modeparams[mode][V206DISPMODE009];
		V206FB004.vpol = modeparams[mode][V206DISPMODE013];
		V206FB004.framerate = modeparams[mode][V206DISPMODE011];
		V206FB004.dualpixel = 0;
	}
	V206FB004.flags = mwv206fb_dvi_hdmi_parse(priv, port);
	FUNC206HAL396(priv, (long)&V206FB004, 0, 1);

	return 0;
}


static int mwv206fb_mode_in_modelist(struct fb_videomode *mode, struct fb_modelist *modelist)
{
	struct fb_modelist *ml;
	struct list_head *pos;

	list_for_each(pos, &modelist->list) {
		ml = list_entry(pos, struct fb_modelist, list);
		if (fb_mode_is_equal(mode, &ml->mode)) {
			return 1;
		}
	}

	return 0;
}

static void mwv206fb_modelist_intersect(struct fb_modelist *dst, struct fb_modelist *src)
{
	struct list_head *pos, *n;
	struct fb_modelist *ml;

	list_for_each_safe(pos, n, &dst->list) {
		ml = list_entry(pos, struct fb_modelist, list);
		if (!mwv206fb_mode_in_modelist(&ml->mode, src)) {
			list_del(pos);
			kfree(pos);
		}
	}
}

static void mwv206fb_videomode_to_displaymode(V206IOCTL161 *dmode,
		struct fb_videomode *vmode, int crtc)
{
	dmode->V206FB011 = crtc;
	dmode->hactive = vmode->xres;
	dmode->vactive = vmode->yres;
	dmode->hsync = vmode->hsync_len;
	dmode->hfrontporch = vmode->right_margin;
	dmode->htotal = vmode->xres + vmode->right_margin + vmode->hsync_len + vmode->left_margin;
	dmode->vsync = vmode->vsync_len;
	dmode->vtotal = vmode->yres + vmode->lower_margin + vmode->vsync_len + vmode->upper_margin;
	dmode->vfrontporch = vmode->lower_margin;
	if (vmode->refresh != 0) {
		dmode->framerate = vmode->refresh;
	} else {
		dmode->framerate = PICOS2KHZ(vmode->pixclock) * 1000 / dmode->vtotal / dmode->htotal;
		V206KDEBUG003("mwv206:caculated refresh rate from timing parameters is: %d", dmode->framerate);
	}
	dmode->V206DEV079 = 0;
	dmode->hpol = vmode->sync & FB_SYNC_HOR_HIGH_ACT ? MWV206_POSITIVE : MWV206_NEGATIVE;
	dmode->vpol = vmode->sync & FB_SYNC_VERT_HIGH_ACT ? MWV206_POSITIVE : MWV206_NEGATIVE;
	dmode->mode = MWV206K_DPMODE_USER;
}

static void mwv206fb_set_zoom_info(V206DEV025 *pDev, struct V206DEV026 *V206FB005,
		struct mwv206_port_config *port)
{
	V206IOCTL161 *dmode = &V206FB005->V206DEV183[1];
	struct fb_var_screeninfo var;
	struct fb_videomode mode;


	if (V206FB005->zoom_port) {
		return;
	}

	if (V206DEVCONFIG042(port->flags) != V206DEVCONFIG013) {
		V206KDEBUG003("mwv206: port zoom enabled but no edid defined, ignored\n");
		return;
	}

	memset(&var, 0, sizeof(var));
	if (fb_parse_edid(port->edid, &var)
		|| ((var.xres * var.yres) >
		(V206FB005->maxwidth * V206FB005->maxheight))) {
		V206KDEBUG003("mwv206: failed to parse port edid, zoom ignored");
		return;
	}

	fb_var_to_videomode(&mode, &var);
	if (mode.vmode & FB_VMODE_INTERLACED) {
		V206KDEBUG003("mwv206: first detailed timing mode for zoom is interlaced\n");
		return;
	}

	mwv206fb_videomode_to_displaymode(dmode, &mode, MWV206_CRTC_ZOOM);

	V206FB005->zoom_port = port;
}


static void mwv206fb_set_defparam(V206DEV025 *pDev, struct V206DEV026 *V206FB005)
{
	struct mwv206_fb_config *fb = &pDev->V206DEV105.fb;
	V206IOCTL161 *dmode = &V206FB005->V206DEV183[0];
	if (fb->mode == MWV206K_DPMODE_USER || fb->mode == CFG_FB_MODE_USER) {
		dmode->mode = MWV206K_DPMODE_USER;
		dmode->V206FB011 = MWV206_CRTC_PRIMARY;
		dmode->V206DEV079 = fb->V206DEV079;
		dmode->framerate = fb->rfsrate;
		dmode->htotal = fb->htotal;
		dmode->hactive = fb->hactive;
		dmode->hfrontporch = fb->hfrontporch;
		dmode->hpol = fb->hpol;
		dmode->hsync = fb->hsync;

		dmode->vtotal = fb->vtotal;
		dmode->vactive = fb->vactive;
		dmode->vfrontporch = fb->vfrontporch;
		dmode->vpol = fb->vpol;
		dmode->vsync = fb->vsync;
	} else if (fb->mode < MWV206K_DPMODE_LAST) {
		dmode->mode = fb->mode;
		dmode->V206FB011 = MWV206_CRTC_PRIMARY;
		dmode->V206DEV079 = 0;
		dmode->htotal = modeparams[fb->mode][V206DISPMODE001],
		dmode->hactive = modeparams[fb->mode][V206DISPMODE002],
		dmode->hfrontporch = modeparams[fb->mode][V206DISPMODE003] + \
							modeparams[fb->mode][V206DISPMODE005],
		dmode->hsync = modeparams[fb->mode][V206DISPMODE004],
		dmode->vtotal = modeparams[fb->mode][V206DISPMODE006],
		dmode->vactive = modeparams[fb->mode][V206DISPMODE007],
		dmode->vfrontporch = modeparams[fb->mode][V206DISPMODE008] + \
							modeparams[fb->mode][V206DISPMODE010],
		dmode->vsync = modeparams[fb->mode][V206DISPMODE009],
		dmode->framerate = modeparams[fb->mode][V206DISPMODE011],
		dmode->hpol = modeparams[fb->mode][V206DISPMODE012];
		dmode->vpol = modeparams[fb->mode][V206DISPMODE013];
	} else {
		V206KDEBUG002("[ERROR]mwv206: fb display param out of range!\n");
	}
}


static void mwv206fb_pick_modes(V206DEV025 *pDev, struct V206DEV026 *V206FB005, struct fb_modelist *modelist)
{
	V206IOCTL161 *dmode = &V206FB005->V206DEV183[0];
	int offset = INT_MAX;
	int d, distance = INT_MAX;
	struct fb_modelist *ml, *best;
	struct list_head *pos;

	best = NULL;
	list_for_each(pos, &modelist->list) {
		ml = list_entry(pos, struct fb_modelist, list);
		if (ml->mode.xres > V206FB005->maxwidth || ml->mode.yres > V206FB005->maxheight) {
			continue;
		}

		if (ml->mode.vmode & FB_VMODE_INTERLACED) {
			continue;
		}

		d = (V206FB005->maxwidth - ml->mode.xres) + (V206FB005->maxheight - ml->mode.yres);
		if (d < distance) {
			distance = d;
			offset = ml->mode.refresh;
			best = ml;
		} else if (d == distance) {
			best = (abs(offset - 60) < abs(ml->mode.refresh - 60)) ? best : ml;
		}
	}
	if (best) {
		mwv206fb_videomode_to_displaymode(dmode, &best->mode, MWV206_CRTC_PRIMARY);
	} else {
		V206KDEBUG003("mwv206: best is null ,use default res");
		mwv206fb_set_defparam(pDev, V206FB005);
	}
}


static void mwv206fb_parse_modes(V206DEV025 *pDev, struct V206DEV026 *V206FB005)
{
	struct fb_info *info = V206FB005->info;
	struct mwv206_port_config *port;
	struct fb_monspecs *specs = &info->monspecs;
	static unsigned char edid[V206CONFIG010];
	struct fb_modelist	common_modes;
	struct fb_modelist	modes;
	int first = 1;

	if (!fb_probe_edid) {
		mwv206fb_set_defparam(pDev, V206FB005);
		return;
	}

	INIT_LIST_HEAD(&common_modes.list);


	mwv206_edid_custom_detect(pDev);

	port_for_each(&pDev->V206DEV105, port) {

		if (!V206DEVCONFIG036(port->flags)) {
			continue;
		}

		if (V206DEVCONFIG040(port->flags)) {
			mwv206fb_set_zoom_info(pDev, V206FB005, port);
			continue;
		}

		if (mwv206fb_get_edid(pDev, edid, port)) {
			continue;
		}

		mwv206fb_edid_to_monspecs(edid, specs);
		if (!specs->modedb) {
			V206KDEBUG003("mwv206: specs->modedb = NULL!\n");
			continue;
		}

		if (first) {
			fb_videomode_to_modelist(specs->modedb, specs->modedb_len,
					&common_modes.list);
			first = 0;
			continue;
		}

		fb_videomode_to_modelist(specs->modedb, specs->modedb_len, &modes.list);
		mwv206fb_modelist_intersect(&common_modes, &modes);
		mwv206fb_destroy_modedb(specs->modedb);
		fb_destroy_modelist(&modes.list);
	}
	mwv206fb_pick_modes(pDev, V206FB005, &common_modes);
	fb_destroy_modelist(&common_modes.list);
}

static void mwv206fb_pre_init(V206DEV025 *priv)
{
	struct V206DEV026 *V206FB005 = priv->fb_info;
	int i, crtc;
	struct mwv206_port_config *V206FB007;

	FUNC206HAL236(priv, priv->V206DEV044[0], 0x40000000);

	FUNC206LXDEV143(priv, 0);
	port_for_each(&priv->V206DEV105, V206FB007) {
		crtc = V206FB005->zoom_port == V206FB007 ? MWV206_CRTC_ZOOM : MWV206_CRTC_PRIMARY;
		if (port_is_hdmi(&priv->V206DEV105, V206FB007) || port_is_lvds(&priv->V206DEV105, V206FB007)) {
			continue;
		}
		if (!V206DEVCONFIG036(V206FB007->flags)) {
			continue;
		}
		mwv206fb_setdisplayport_off(priv, crtc, V206FB007);
	}
	if (priv->pm.V206DEV109 == 0) {
		mwv206fb_clear(V206FB005);
	}

	mwv206fb_parse_modes(priv, V206FB005);


	FUNC206HAL390(priv, (long)&V206FB005->V206DEV183[0]);

	V206KDEBUG003("[INFO] mwv206: mode = %d scrid = %d "\
		"hactive = %d vactive = %d framerate = %d\n",
		V206FB005->V206DEV183[0].mode,
		V206FB005->V206DEV183[0].V206FB011,
		V206FB005->V206DEV183[0].hactive,
		V206FB005->V206DEV183[0].vactive,
		V206FB005->V206DEV183[0].framerate);

	FUNC206LXDEV094(priv, MWV206_CRTC_PRIMARY,
			&V206FB005->V206DEV183[0], V206FB005->V206DEV181);

	if (V206FB005->zoom_port) {
		FUNC206HAL390(priv, (long)&V206FB005->V206DEV183[1]);
		FUNC206LXDEV094(priv, MWV206_CRTC_ZOOM,
				&V206FB005->V206DEV183[1], V206FB005->V206DEV181);
		V206KDEBUG003("[INFO] mwv206: mode = %d scrid = %d "\
			"hactive = %d vactive = %d framerate = %d\n",
			V206FB005->V206DEV183[1].mode,
			V206FB005->V206DEV183[1].V206FB011,
			V206FB005->V206DEV183[1].hactive,
			V206FB005->V206DEV183[1].vactive,
			V206FB005->V206DEV183[1].framerate);
	}


	for (i = 0; i < MAX_VIDEO_SCREEN_NUMBER; i++) {
		FUNC206HAL423(priv, i);
		FUNC206HAL422(priv, i);
	}

}

static void mwv206fb_post_init(V206DEV025 *priv)
{
	struct V206DEV026 *V206FB005 = priv->fb_info;
	struct mwv206_port_config *V206FB007;
	int  mode, crtc;

	port_for_each(&priv->V206DEV105, V206FB007) {
		crtc = V206FB005->zoom_port == V206FB007 ? MWV206_CRTC_ZOOM : MWV206_CRTC_PRIMARY;
		mode = V206FB005->zoom_port == V206FB007 ? 1 : 0;
		if (port_is_hdmi(&priv->V206DEV105, V206FB007)) {
			mwv206fb_sethdmimode(priv, crtc, V206FB007, &V206FB005->V206DEV183[mode]);
		}
		FUNC206LXDEV084(priv, crtc, V206FB007);
	}

	mwv206_resethdmiphy(priv, 0xf);
	FUNC206LXDEV143(priv, 1);
}

int mwv206fb_active(V206DEV025 *priv)
{
	mwv206fb_pre_init(priv);
	mwv206fb_post_init(priv);
	return 0;
}



int mwv206fb_init_early(struct pci_dev *V206DEV103)
{
	struct apertures_struct *ap;
	int i;

	ap = alloc_apertures(6);
	if (ap == NULL) {
		return -ENOMEM;
	}

	for (i = 0; i < 6; i++) {
		ap->ranges[i].base = pci_resource_start(V206DEV103, i);
		ap->ranges[i].size = pci_resource_len(V206DEV103, i);
	}
	remove_conflicting_framebuffers(ap, "MWV206FB", true);
	kfree(ap);

	return 0;
}

int mwv206fb_register(struct pci_dev *V206DEV103)
{
	V206DEV025 *priv = pci_get_drvdata(V206DEV103);
	struct fb_info *info;
	struct V206DEV026 *V206FB005;
	unsigned pitch_mask = 64 - 1;
	unsigned int offset;
	int ret = 0;
	unsigned int V206DEV043, V206FB006;

#if defined(__mips__) || defined(__loongarch__)

	int i, max_fb;
	if (num_registered_fb > 0) {
		max_fb = num_registered_fb;
		for (i = 0; i < max_fb; i++) {
			if (NULL != registered_fb[i]) {
				unregister_framebuffer(registered_fb[i]);
			}
		}
	}
#endif

	info = framebuffer_alloc(sizeof(struct V206DEV026), &V206DEV103->dev);
	if (!info) {
		V206KDEBUG002("[ERROR] Allocation frame buffer failed.\n");
		return -ENOMEM;
	}


	V206DEV043 = priv->V206DEV043;
	V206FB006 = priv->V206DEV044[0];


	V206FB005 = info->par;
	V206FB005->info = info;
	V206FB005->V206DEV103 = V206DEV103;
	priv->fb_info = V206FB005;



	snprintf(V206FB005->name, sizeof(V206FB005->name), "MWV206");
	V206FB005->pixel_format = V206FB002;
	V206FB005->maxwidth = MWV206_FB_MAXWIDTH;
	V206FB005->maxheight = MWV206_FB_MAXHEIGHT;
	V206FB005->V206KG2D001 = 32;
	V206FB005->V206DEV182 = ((V206FB005->maxwidth * V206FB005->V206KG2D001 / 8) + pitch_mask) & ~pitch_mask;
	V206FB005->screen_size = V206FB005->V206DEV182 * V206FB005->maxheight;


	V206FB005->V206DEV177 = MWV206_GET_PCI_BAR_STARTADDR(V206DEV103, V206FB006);
	V206FB005->V206DEV180 = MWV206_GET_PCI_BAR_STARTADDR(V206DEV103, V206DEV043);

	if (V206FB005->screen_size > MWV206_GET_PCI_BAR_LEN(V206DEV103, V206FB006)) {
		V206KDEBUG002("[ERROR] PCIE BAR%d space not enough\n", V206FB006);
		ret = -ENOMEM;
		goto err0;
	}
	V206FB005->V206DEV179 = V206FB005->screen_size;
	V206FB005->mmio_len = MWV206_GET_PCI_BAR_LEN(V206DEV103, V206DEV043);

	V206FB005->mmio_base = (void *)priv->V206DEV033;


	if (fb_alloc_cmap(&info->cmap, 256, 0)) {
		ret = -ENOMEM;
		goto err1;
	}


	offset = FUNC206HAL223(priv, -V206FB005->screen_size, 64 * 1024);
	if (offset == 0) {
		V206KDEBUG002("[ERROR] allocate from framebuffer failure\n");
		ret = -ENOMEM;
		goto err2;
	}
	if (offset >= priv->V206DEV032[0]) {
		V206KDEBUG002("[ERROR] mwv206_mem_alloc allocate framebuffer address is to large\n");
		ret = -ENOMEM;
		goto err2;
	}

	V206KDEBUG003("[INFO] frame buffer offset: 0x%x\n", offset);
	V206FB005->V206DEV181 = offset;

	if (V206CTYPE010(priv->V206DEV028) || V206CTYPE009(priv->V206DEV028)
			|| V206CTYPE011(priv->V206DEV028)) {
		V206FB005->V206DEV178 = ioremap_wc(V206FB005->V206DEV177 + offset, V206FB005->V206DEV179);
	} else {
		V206FB005->V206DEV178 = ioremap(V206FB005->V206DEV177 + offset, V206FB005->V206DEV179);
	}

	V206KDEBUG003("[INFO] Frame buffer BAR%d - PHY: 0x%lx, offset: 0x%x, VIR: %p, size: 0x%lx\n",
			V206FB006, V206FB005->V206DEV177, offset, V206FB005->V206DEV178, V206FB005->V206DEV179);


	mwv206fb_pre_init(priv);
	FUNC206LXDEV080(info);
	ret = register_framebuffer(info);
	if (ret < 0) {
		V206KDEBUG002("[ERROR] mwv206fb (%s): could not register framebuffer\n",
				pci_name(V206FB005->V206DEV103));
		goto err3;
	}
	mwv206fb_post_init(priv);


	mdelay(1);
	V206KDEBUG003("[INFO] mwv206fb_register done.\n");
	return 0;

err3:
	FUNC206HAL226(priv, V206FB005->V206DEV181);
err2:
	fb_dealloc_cmap(&info->cmap);
err1:
	iounmap(V206FB005->V206DEV178);
err0:
	framebuffer_release(info);
	return ret;
}

void FUNC206LXDEV085(struct pci_dev *V206DEV103)
{
	V206DEV025 *priv = pci_get_drvdata(V206DEV103);
	struct V206DEV026 *V206FB005;
	struct fb_info *info;

	V206FB005 = priv->fb_info;
	if (V206FB005 == NULL) {
		return;
	}

	info = V206FB005->info;
	unregister_framebuffer(info);
	FUNC206HAL226(priv, V206FB005->V206DEV181);
	fb_dealloc_cmap(&info->cmap);
	iounmap(V206FB005->V206DEV178);
	framebuffer_release(info);
}