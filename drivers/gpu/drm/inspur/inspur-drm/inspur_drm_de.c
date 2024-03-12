// SPDX-License-Identifier: GPL-2.0-or-later

#include <drm/drm_atomic_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_fourcc.h>

#include <drm/drm_atomic.h>
#include <drm/drm_gem_vram_helper.h>


#include "inspur_drm_drv.h"
#include "inspur_drm_regs.h"

struct inspur_dislay_pll_config {
	unsigned long hdisplay;
	unsigned long vdisplay;
	u32 pll1_config_value;
	u32 pll2_config_value;
};

static const struct inspur_dislay_pll_config inspur_pll_table[] = {
	{ 640, 480, CRT_PLL1_NS_25MHZ, CRT_PLL2_NS_25MHZ },
	{ 800, 600, CRT_PLL1_NS_40MHZ, CRT_PLL2_NS_40MHZ },
	{ 1024, 768, CRT_PLL1_NS_65MHZ, CRT_PLL2_NS_65MHZ },
	{ 1280, 1024, CRT_PLL1_NS_108MHZ, CRT_PLL2_NS_108MHZ },
	{ 1920, 1080, CRT_PLL1_NS_148MHZ, CRT_PLL2_NS_148MHZ },
};

#define PADDING(align, data) (((data) + (align) - 1) & (~((align) - 1)))

static int inspur_plane_atomic_check(struct drm_plane *plane,
				    struct drm_atomic_state *atom_state)
{
	struct drm_plane_state *state = drm_atomic_get_new_plane_state(atom_state, plane);
	struct drm_framebuffer *fb = state->fb;
	struct drm_crtc *crtc = state->crtc;
	struct drm_crtc_state *crtc_state;
	u32 src_w = state->src_w >> 16;
	u32 src_h = state->src_h >> 16;

	if (!crtc || !fb)
		return 0;

	crtc_state = drm_atomic_get_crtc_state(state->state, crtc);
	if (IS_ERR(crtc_state))
		return PTR_ERR(crtc_state);

	if (src_w != state->crtc_w || src_h != state->crtc_h) {
		DRM_DEBUG_ATOMIC("scale not support\n");
		return -EINVAL;
	}

	if (state->crtc_x < 0 || state->crtc_y < 0) {
		DRM_DEBUG_ATOMIC("crtc_x/y of drm_plane state is invalid\n");
		return -EINVAL;
	}

	if (!crtc_state->enable)
		return 0;

	if (state->crtc_x + state->crtc_w >
	    crtc_state->adjusted_mode.hdisplay ||
	    state->crtc_y + state->crtc_h >
	    crtc_state->adjusted_mode.vdisplay) {
		DRM_DEBUG_ATOMIC("visible portion of plane is invalid\n");
		return -EINVAL;
	}

	if (state->fb->pitches[0] % 16 != 0) {
		DRM_DEBUG_ATOMIC("wrong stride with 16-byte aligned\n");
		return -EINVAL;
	}

	return 0;
}

static void inspur_plane_atomic_update(struct drm_plane *plane,
				      struct drm_atomic_state *old_state)
{
	struct drm_plane_state *state = plane->state;
	u32 reg;
	int ret;
	s64 gpu_addr = 0;
	unsigned int line_l;
	struct inspur_drm_private *priv = plane->dev->dev_private;
	struct drm_gem_vram_object *gbo;

	if (!state->fb)
		return;

	gbo = drm_gem_vram_of_gem(state->fb->obj[0]);

	ret = drm_gem_vram_pin(gbo, DRM_GEM_VRAM_PL_FLAG_VRAM);
	if (ret) {
		DRM_ERROR("failed to pin bo: %d", ret);
		return;
	}
	gpu_addr = drm_gem_vram_offset(gbo);
	if (gpu_addr < 0) {
		drm_gem_vram_unpin(gbo);
		return;
	}

	writel(gpu_addr, priv->mmio + INSPUR_CRT_FB_ADDRESS);

	reg = state->fb->width * (state->fb->format->cpp[0]);

	line_l = state->fb->pitches[0];
	writel(INSPUR_FIELD(INSPUR_CRT_FB_WIDTH_WIDTH, reg) |
	       INSPUR_FIELD(INSPUR_CRT_FB_WIDTH_OFFS, line_l),
	       priv->mmio + INSPUR_CRT_FB_WIDTH);

	/* SET PIXEL FORMAT */
	reg = readl(priv->mmio + INSPUR_CRT_DISP_CTL);
	reg &= ~INSPUR_CRT_DISP_CTL_FORMAT_MASK;
	reg |= INSPUR_FIELD(INSPUR_CRT_DISP_CTL_FORMAT,
			    state->fb->format->cpp[0] * 8 / 16);
	writel(reg, priv->mmio + INSPUR_CRT_DISP_CTL);
}

static const u32 channel_formats1[] = {
	DRM_FORMAT_RGB565, DRM_FORMAT_BGR565, DRM_FORMAT_RGB888,
	DRM_FORMAT_BGR888, DRM_FORMAT_XRGB8888, DRM_FORMAT_XBGR8888,
	DRM_FORMAT_RGBA8888, DRM_FORMAT_BGRA8888, DRM_FORMAT_ARGB8888,
	DRM_FORMAT_ABGR8888
};

static struct drm_plane_funcs inspur_plane_funcs = {
	.update_plane = drm_atomic_helper_update_plane,
	.disable_plane = drm_atomic_helper_disable_plane,
	.destroy = drm_plane_cleanup,
	.reset = drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_plane_destroy_state,
};

static const struct drm_plane_helper_funcs inspur_plane_helper_funcs = {
	.atomic_check = inspur_plane_atomic_check,
	.atomic_update = inspur_plane_atomic_update,
};

static struct drm_plane *inspur_plane_init(struct inspur_drm_private *priv)
{
	struct drm_device *dev = priv->dev;
	struct drm_plane *plane;
	int ret = 0;

	plane = devm_kzalloc(dev->dev, sizeof(*plane), GFP_KERNEL);
	if (!plane) {
		DRM_ERROR("failed to alloc memory when init plane\n");
		return ERR_PTR(-ENOMEM);
	}
	ret = drm_universal_plane_init(dev, plane, 1, &inspur_plane_funcs,
				       channel_formats1,
				       ARRAY_SIZE(channel_formats1),
				       NULL, DRM_PLANE_TYPE_PRIMARY, NULL);
	if (ret) {
		DRM_ERROR("failed to init plane: %d\n", ret);
		return ERR_PTR(ret);
	}

	drm_plane_helper_add(plane, &inspur_plane_helper_funcs);
	return plane;
}

static void inspur_crtc_dpms(struct drm_crtc *crtc, int dpms)
{
	struct inspur_drm_private *priv = crtc->dev->dev_private;
	unsigned int reg;

	reg = readl(priv->mmio + INSPUR_CRT_DISP_CTL);
	reg &= ~INSPUR_CRT_DISP_CTL_DPMS_MASK;
	reg |= INSPUR_FIELD(INSPUR_CRT_DISP_CTL_DPMS, dpms);
	reg &= ~INSPUR_CRT_DISP_CTL_TIMING_MASK;
	if (dpms == INSPUR_CRT_DPMS_ON)
		reg |= INSPUR_CRT_DISP_CTL_TIMING(1);
	writel(reg, priv->mmio + INSPUR_CRT_DISP_CTL);
}

static void inspur_crtc_atomic_enable(struct drm_crtc *crtc,
				     struct drm_atomic_state *old_state)
{
	unsigned int reg;
	struct inspur_drm_private *priv = crtc->dev->dev_private;

	inspur_set_power_mode(priv, INSPUR_PW_MODE_CTL_MODE_MODE0);

	/* Enable display power gate & LOCALMEM power gate */
	reg = readl(priv->mmio + INSPUR_CURRENT_GATE);
	reg &= ~INSPUR_CURR_GATE_LOCALMEM_MASK;
	reg &= ~INSPUR_CURR_GATE_DISPLAY_MASK;
	reg |= INSPUR_CURR_GATE_LOCALMEM(1);
	reg |= INSPUR_CURR_GATE_DISPLAY(1);
	inspur_set_current_gate(priv, reg);
	inspur_crtc_dpms(crtc, INSPUR_CRT_DPMS_ON);
}

static void inspur_crtc_atomic_disable(struct drm_crtc *crtc,
				      struct drm_atomic_state *old_state)
{
	unsigned int reg;
	struct inspur_drm_private *priv = crtc->dev->dev_private;

	inspur_crtc_dpms(crtc, INSPUR_CRT_DPMS_OFF);

	inspur_set_power_mode(priv, INSPUR_PW_MODE_CTL_MODE_SLEEP);

	/* Enable display power gate & LOCALMEM power gate */
	reg = readl(priv->mmio + INSPUR_CURRENT_GATE);
	reg &= ~INSPUR_CURR_GATE_LOCALMEM_MASK;
	reg &= ~INSPUR_CURR_GATE_DISPLAY_MASK;
	reg |= INSPUR_CURR_GATE_LOCALMEM(0);
	reg |= INSPUR_CURR_GATE_DISPLAY(0);
	inspur_set_current_gate(priv, reg);
}

static enum drm_mode_status
inspur_crtc_mode_valid(struct drm_crtc *crtc,
		       const struct drm_display_mode *mode)
{
	int i = 0;
	int vrefresh = drm_mode_vrefresh(mode);

	if (vrefresh < 59 || vrefresh > 61)
		return MODE_NOCLOCK;

	for (i = 0; i < ARRAY_SIZE(inspur_pll_table); i++) {
		if (inspur_pll_table[i].hdisplay == mode->hdisplay &&
		    inspur_pll_table[i].vdisplay == mode->vdisplay)
			return MODE_OK;
	}

	return MODE_BAD;
}

static void set_vclock_inspur(struct drm_device *dev, unsigned long pll)
{
	u32 val;
	struct inspur_drm_private *priv = dev->dev_private;

	val = readl(priv->mmio + CRT_PLL1_NS);
	val &= ~(CRT_PLL1_NS_OUTER_BYPASS(1));
	writel(val, priv->mmio + CRT_PLL1_NS);

	val = CRT_PLL1_NS_INTER_BYPASS(1) | CRT_PLL1_NS_POWERON(1);
	writel(val, priv->mmio + CRT_PLL1_NS);

	writel(pll, priv->mmio + CRT_PLL1_NS);

	usleep_range(1000, 2000);

	val = pll & ~(CRT_PLL1_NS_POWERON(1));
	writel(val, priv->mmio + CRT_PLL1_NS);

	usleep_range(1000, 2000);

	val &= ~(CRT_PLL1_NS_INTER_BYPASS(1));
	writel(val, priv->mmio + CRT_PLL1_NS);

	usleep_range(1000, 2000);

	val |= CRT_PLL1_NS_OUTER_BYPASS(1);
	writel(val, priv->mmio + CRT_PLL1_NS);
}

static void get_pll_config(unsigned long x, unsigned long y,
			   u32 *pll1, u32 *pll2)
{
	int i;
	int count = ARRAY_SIZE(inspur_pll_table);

	for (i = 0; i < count; i++) {
		if (inspur_pll_table[i].hdisplay == x &&
		    inspur_pll_table[i].vdisplay == y) {
			*pll1 = inspur_pll_table[i].pll1_config_value;
			*pll2 = inspur_pll_table[i].pll2_config_value;
			return;
		}
	}

	/* if found none, we use default value */
	*pll1 = CRT_PLL1_NS_25MHZ;
	*pll2 = CRT_PLL2_NS_25MHZ;
}

/*
 * This function takes care the extra registers and bit fields required to
 * setup a mode in board.
 * Explanation about Display Control register:
 * FPGA only supports 7 predefined pixel clocks, and clock select is
 * in bit 4:0 of new register 0x802a8.
 */
static unsigned int display_ctrl_adjust(struct drm_device *dev,
					struct drm_display_mode *mode,
					unsigned int ctrl)
{
	unsigned long x, y;
	u32 pll1;		/* bit[31:0] of PLL */
	u32 pll2;		/* bit[63:32] of PLL */
	struct inspur_drm_private *priv = dev->dev_private;

	x = mode->hdisplay;
	y = mode->vdisplay;

	get_pll_config(x, y, &pll1, &pll2);
	writel(pll2, priv->mmio + CRT_PLL2_NS);
	set_vclock_inspur(dev, pll1);

	/*
	 * inspur has to set up the top-left and bottom-right
	 * registers as well.
	 * Note that normal chip only use those two register for
	 * auto-centering mode.
	 */
	writel(INSPUR_FIELD(INSPUR_CRT_AUTO_CENTERING_TL_TOP, 0) |
	       INSPUR_FIELD(INSPUR_CRT_AUTO_CENTERING_TL_LEFT, 0),
	       priv->mmio + INSPUR_CRT_AUTO_CENTERING_TL);

	writel(INSPUR_FIELD(INSPUR_CRT_AUTO_CENTERING_BR_BOTTOM, y - 1) |
	       INSPUR_FIELD(INSPUR_CRT_AUTO_CENTERING_BR_RIGHT, x - 1),
	       priv->mmio + INSPUR_CRT_AUTO_CENTERING_BR);

	/*
	 * Assume common fields in ctrl have been properly set before
	 * calling this function.
	 * This function only sets the extra fields in ctrl.
	 */

	/* Set bit 25 of display controller: Select CRT or VGA clock */
	ctrl &= ~INSPUR_CRT_DISP_CTL_CRTSELECT_MASK;
	ctrl &= ~INSPUR_CRT_DISP_CTL_CLOCK_PHASE_MASK;

	ctrl |= INSPUR_CRT_DISP_CTL_CRTSELECT(INSPUR_CRTSELECT_CRT);

	/* clock_phase_polarity is 0 */
	ctrl |= INSPUR_CRT_DISP_CTL_CLOCK_PHASE(0);

	writel(ctrl, priv->mmio + INSPUR_CRT_DISP_CTL);

	return ctrl;
}

static void inspur_crtc_mode_set_nofb(struct drm_crtc *crtc)
{
	unsigned int val;
	struct drm_display_mode *mode = &crtc->state->mode;
	struct drm_device *dev = crtc->dev;
	struct inspur_drm_private *priv = dev->dev_private;
	int width = mode->hsync_end - mode->hsync_start;
	int height = mode->vsync_end - mode->vsync_start;

	//writel(format_pll_reg(), priv->mmio + INSPUR_CRT_PLL_CTRL);
	writel(INSPUR_FIELD(INSPUR_CRT_HORZ_TOTAL_TOTAL, mode->htotal - 1) |
	       INSPUR_FIELD(INSPUR_CRT_HORZ_TOTAL_DISP_END, mode->hdisplay - 1),
	       priv->mmio + INSPUR_CRT_HORZ_TOTAL);

	writel(INSPUR_FIELD(INSPUR_CRT_HORZ_SYNC_WIDTH, width) |
	       INSPUR_FIELD(INSPUR_CRT_HORZ_SYNC_START, mode->hsync_start - 1),
	       priv->mmio + INSPUR_CRT_HORZ_SYNC);

	writel(INSPUR_FIELD(INSPUR_CRT_VERT_TOTAL_TOTAL, mode->vtotal - 1) |
	       INSPUR_FIELD(INSPUR_CRT_VERT_TOTAL_DISP_END, mode->vdisplay - 1),
	       priv->mmio + INSPUR_CRT_VERT_TOTAL);

	writel(INSPUR_FIELD(INSPUR_CRT_VERT_SYNC_HEIGHT, height) |
	       INSPUR_FIELD(INSPUR_CRT_VERT_SYNC_START, mode->vsync_start - 1),
	       priv->mmio + INSPUR_CRT_VERT_SYNC);

	val = INSPUR_FIELD(INSPUR_CRT_DISP_CTL_VSYNC_PHASE, 0);
	val |= INSPUR_FIELD(INSPUR_CRT_DISP_CTL_HSYNC_PHASE, 0);
	val |= INSPUR_CRT_DISP_CTL_TIMING(1);
	val |= INSPUR_CRT_DISP_CTL_PLANE(1);

	display_ctrl_adjust(dev, mode, val);
}

static void inspur_crtc_atomic_begin(struct drm_crtc *crtc,
				    struct drm_atomic_state *old_state)
{
	unsigned int reg;
	struct drm_device *dev = crtc->dev;
	struct inspur_drm_private *priv = dev->dev_private;

	inspur_set_power_mode(priv, INSPUR_PW_MODE_CTL_MODE_MODE0);

	/* Enable display power gate & LOCALMEM power gate */
	reg = readl(priv->mmio + INSPUR_CURRENT_GATE);
	reg &= ~INSPUR_CURR_GATE_DISPLAY_MASK;
	reg &= ~INSPUR_CURR_GATE_LOCALMEM_MASK;
	reg |= INSPUR_CURR_GATE_DISPLAY(1);
	reg |= INSPUR_CURR_GATE_LOCALMEM(1);
	inspur_set_current_gate(priv, reg);

	/* We can add more initialization as needed. */
}

static void inspur_crtc_atomic_flush(struct drm_crtc *crtc,
				    struct drm_atomic_state *old_state)

{
	unsigned long flags;

	spin_lock_irqsave(&crtc->dev->event_lock, flags);
	if (crtc->state->event)
		drm_crtc_send_vblank_event(crtc, crtc->state->event);
	crtc->state->event = NULL;
	spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
}

static int inspur_crtc_enable_vblank(struct drm_crtc *crtc)
{
	struct inspur_drm_private *priv = crtc->dev->dev_private;

	writel(INSPUR_RAW_INTERRUPT_EN_VBLANK(1),
	       priv->mmio + INSPUR_RAW_INTERRUPT_EN);

	return 0;
}

static void inspur_crtc_disable_vblank(struct drm_crtc *crtc)
{
	struct inspur_drm_private *priv = crtc->dev->dev_private;

	writel(INSPUR_RAW_INTERRUPT_EN_VBLANK(0),
	       priv->mmio + INSPUR_RAW_INTERRUPT_EN);
}

static const struct drm_crtc_funcs inspur_crtc_funcs = {
	.page_flip = drm_atomic_helper_page_flip,
	.set_config = drm_atomic_helper_set_config,
	.destroy = drm_crtc_cleanup,
	.reset = drm_atomic_helper_crtc_reset,
	.atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_crtc_destroy_state,
	.enable_vblank = inspur_crtc_enable_vblank,
	.disable_vblank = inspur_crtc_disable_vblank,

};

static const struct drm_crtc_helper_funcs inspur_crtc_helper_funcs = {
	.mode_set_nofb = inspur_crtc_mode_set_nofb,
	.atomic_begin = inspur_crtc_atomic_begin,
	.atomic_flush = inspur_crtc_atomic_flush,
	.atomic_enable = inspur_crtc_atomic_enable,
	.atomic_disable = inspur_crtc_atomic_disable,
	.mode_valid = inspur_crtc_mode_valid,
};

int inspur_de_init(struct inspur_drm_private *priv)
{
	struct drm_device *dev = priv->dev;
	struct drm_crtc *crtc;
	struct drm_plane *plane;
	int ret;

	plane = inspur_plane_init(priv);
	if (IS_ERR(plane)) {
		DRM_ERROR("failed to create plane: %ld\n", PTR_ERR(plane));
		return PTR_ERR(plane);
	}

	crtc = devm_kzalloc(dev->dev, sizeof(*crtc), GFP_KERNEL);
	if (!crtc) {
		DRM_ERROR("failed to alloc memory when init crtc\n");
		return -ENOMEM;
	}

	ret = drm_crtc_init_with_planes(dev, crtc, plane,
					NULL, &inspur_crtc_funcs, NULL);
	if (ret) {
		DRM_ERROR("failed to init crtc: %d\n", ret);
		return ret;
	}

	ret = drm_mode_crtc_set_gamma_size(crtc, 256);
	if (ret) {
		DRM_ERROR("failed to set gamma size: %d\n", ret);
		return ret;
	}
	drm_crtc_helper_add(crtc, &inspur_crtc_helper_funcs);

	return 0;
}
