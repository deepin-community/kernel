/* SPDX-License-Identifier: GPL-2.0 */

#ifndef INSPUR_DRM_DRV_H
#define INSPUR_DRM_DRV_H

#include <linux/version.h>
#include <drm/drm_atomic.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem.h>
#include <drm/drm_gem_vram_helper.h>
#include <linux/pci.h>
#include <drm/drm_vblank.h>
#include <drm/drm_drv.h>

#include <drm/drm_framebuffer.h>
#include <drm/drm_edid.h>
#include <drm/drm_aperture.h>

#include <linux/delay.h>
#include <drm/drm_gem_framebuffer_helper.h>

struct drm_device;
struct drm_gem_object;

#define inspur_framebuffer drm_framebuffer
#define BPP16_RED    0x0000f800
#define BPP16_GREEN  0x000007e0
#define BPP16_BLUE   0x0000001f
#define BPP16_WHITE  0x0000ffff
#define BPP16_GRAY   0x00008410
#define BPP16_YELLOW 0x0000ffe0
#define BPP16_CYAN   0x000007ff
#define BPP16_PINK   0x0000f81f
#define BPP16_BLACK  0x00000000
struct inspur_fbdev {
	struct drm_fb_helper helper;
	struct inspur_framebuffer *fb;
	int size;
};

struct inspur_cursor {
	struct drm_gem_vram_object *gbo[2];
	unsigned int next_index;
};

struct inspur_drm_private {
	/* hw */
	void __iomem *mmio;
	void __iomem *fb_map;
	unsigned long fb_base;
	unsigned long fb_size;

	/* drm */
	struct drm_device *dev;

	bool mode_config_initialized;
	struct drm_atomic_state *suspend_state;

	/* fbdev */
	struct inspur_fbdev *fbdev;

	/* hw cursor */
	struct inspur_cursor cursor;
};

#define to_inspur_framebuffer(x) container_of(x, struct inspur_framebuffer, fb)

void inspur_set_power_mode(struct inspur_drm_private *priv,
			   unsigned int power_mode);
void inspur_set_current_gate(struct inspur_drm_private *priv,
			     unsigned int gate);
int inspur_load(struct drm_device *dev, unsigned long flags);
void inspur_unload(struct drm_device *dev);

int inspur_de_init(struct inspur_drm_private *priv);
int inspur_vdac_init(struct inspur_drm_private *priv);

int inspur_gem_create(struct drm_device *dev, u32 size, bool iskernel,
		      struct drm_gem_object **obj);

int inspur_dumb_create(struct drm_file *file, struct drm_device *dev,
		       struct drm_mode_create_dumb *args);

extern const struct drm_mode_config_funcs inspur_mode_funcs;

#endif
