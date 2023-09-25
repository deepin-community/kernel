#include <linux/pm_runtime.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>

#include "gsgpu.h"
#include "gsgpu_dc_plane.h"
#include "gsgpu_dc_crtc.h"
#include "gsgpu_dc.h"
#include "gsgpu_display.h"
#include "gsgpu_dc_encoder.h"
#include "gsgpu_dc_connector.h"
#include "gsgpu_dc_vbios.h"
#include "gsgpu_dc_reg.h"
#include "gsgpu_dc_hdmi.h"
#include "bridge_phy.h"

static const struct gsgpu_display_funcs dc_display_funcs = {
	.vblank_get_counter = dc_vblank_get_counter,
	.page_flip_get_scanoutpos = dc_crtc_get_scanoutpos,
};

u32 dc_readl(struct gsgpu_device *adev, u32 reg)
{
	return readl(adev->loongson_dc_rmmio + reg);
}

void dc_writel(struct gsgpu_device *adev, u32 reg, u32 val)
{
	writel(val, adev->loongson_dc_rmmio + reg);
}

u32 dc_readl_locked(struct gsgpu_device *adev, u32 reg)
{
	u32 val;
	unsigned long flags;

	spin_lock_irqsave(&adev->dc_mmio_lock, flags);
	val = readl(adev->loongson_dc_rmmio + reg);
	spin_unlock_irqrestore(&adev->dc_mmio_lock, flags);

	return val;
}

void dc_writel_locked(struct gsgpu_device *adev, u32 reg, u32 val)
{
	unsigned long flags;

	spin_lock_irqsave(&adev->dc_mmio_lock, flags);
	writel(val, adev->loongson_dc_rmmio + reg);
	spin_unlock_irqrestore(&adev->dc_mmio_lock, flags);
}

static bool dc_links_init(struct gsgpu_dc *dc)
{
	struct header_resource *header_res = NULL;
	struct crtc_resource *crtc_resource;
	struct encoder_resource *encoder_resource;
	struct connector_resource *connector_resource;
	struct gsgpu_link_info *link_info;
	s32 links, i;

	if (IS_ERR_OR_NULL(dc))
		return false;

	header_res = dc_get_vbios_resource(dc->vbios, 0, GSGPU_RESOURCE_HEADER);
	links = header_res->links;

	link_info = kzalloc(sizeof(*link_info) * links, GFP_KERNEL);
	if (IS_ERR_OR_NULL(link_info))
		return false;

	dc->link_info = link_info;

	for (i = 0; i < links; i++) {
		crtc_resource = dc_get_vbios_resource(dc->vbios, i, GSGPU_RESOURCE_CRTC);
		link_info[i].crtc = dc_crtc_construct(dc, crtc_resource);
		if (!link_info[i].crtc) {
			DRM_ERROR("link-%d  crtc construct failed \n", i);
			continue;
		}

		encoder_resource = dc_get_vbios_resource(dc->vbios, i, GSGPU_RESOURCE_ENCODER);
		link_info[i].encoder = dc_encoder_construct(dc, encoder_resource);
		if (!link_info[i].encoder) {
			DRM_ERROR("link-%d  encoder construct failed \n", i);
			continue;
		}

		connector_resource = dc_get_vbios_resource(dc->vbios,
						i, GSGPU_RESOURCE_CONNECTOR);
		link_info[i].bridge = dc_bridge_construct(dc,
					encoder_resource, connector_resource);
		if (!link_info[i].bridge) {
			DRM_ERROR("link-%d bridge construct failed\n", i);
			continue;
		}
		link_info[i].connector = dc_connector_construct(dc, connector_resource);
		if (!link_info[i].connector) {
			DRM_ERROR("link-%d  encoder construct failed \n", i);
			continue;
		}

		link_info[i].fine = true;
		dc->links++;
	}

	return true;
}

static void dc_link_exit(struct gsgpu_dc *dc)
{
	u32 i;
	struct gsgpu_link_info *link_info;

	if (IS_ERR_OR_NULL(dc))
		return;

	link_info = dc->link_info;

	for (i = 0; i < dc->links; i++) {
		dc_crtc_destroy(link_info[i].crtc);

		link_info[i].fine = false;
	}

	kfree(link_info);
	dc->link_info = NULL;
	dc->links = 0;
}

static struct gsgpu_dc *dc_construct(struct gsgpu_device *adev)
{
	struct gsgpu_dc *dc;
	bool status;

	if (IS_ERR_OR_NULL(adev))
		return false;

	dc = kzalloc(sizeof(*dc), GFP_KERNEL);

	if (IS_ERR_OR_NULL(dc))
		return ERR_PTR(-ENOMEM);

	dc->adev = adev;

	INIT_LIST_HEAD(&dc->crtc_list);
	INIT_LIST_HEAD(&dc->encoder_list);
	INIT_LIST_HEAD(&dc->connector_list);

	status = dc_vbios_init(dc);
	if (!status) {
		kfree(dc);
		DRM_ERROR("GSGPU dc init vbios failed\n");
		return NULL;
	}

	if (dc_links_init(dc) == false) {
		DRM_ERROR("GSGPU dc init links failed\n");
		kfree(dc);
		dc = NULL;
	}

	return dc;
}

static void dc_destruct(struct gsgpu_dc *dc)
{
	if (IS_ERR_OR_NULL(dc))
		return;

	dc_link_exit(dc);
	dc_vbios_exit(dc->vbios);

	kfree(dc);
	dc = NULL;
}

bool dc_submit_timing_update(struct gsgpu_dc *dc, u32 link, struct dc_timing_info *timing)
{
	if (IS_ERR_OR_NULL(dc) || (link >= dc->links))
		return false;

	return dc_crtc_timing_set(dc->link_info[link].crtc, timing);
}

bool dc_submit_plane_update(struct gsgpu_dc *dc, u32 link, struct dc_plane_update *update)
{
	if (IS_ERR_OR_NULL(dc) || (link >= dc->links))
		return false;

	return dc_crtc_plane_update(dc->link_info[link].crtc, update);
}

static void manage_dc_interrupts(struct gsgpu_device *adev,
				 struct gsgpu_crtc *acrtc,
				 bool enable)
{
	if (enable) {
		drm_crtc_vblank_on(&acrtc->base);
		gsgpu_irq_get(adev, &adev->vsync_irq, acrtc->crtc_id);
	} else {
		gsgpu_irq_put(adev, &adev->vsync_irq, acrtc->crtc_id);
		drm_crtc_vblank_off(&acrtc->base);
	}
}

static int gsgpu_dc_meta_enable(struct gsgpu_device *adev, bool clear)
{
	uint64_t meta_gpu_addr;
	int r, i;

	if (adev->dc->meta_bo == NULL) {
		dev_err(adev->dev, "No VRAM object for PCIE DC_META.\n");
		return -EINVAL;
	}

	r = gsgpu_bo_reserve(adev->dc->meta_bo, false);
	if (unlikely(r != 0))
		return r;

	r = gsgpu_bo_pin(adev->dc->meta_bo, GSGPU_GEM_DOMAIN_VRAM);
	if (r) {
		gsgpu_bo_unreserve(adev->dc->meta_bo);
		return r;
	}

	r = gsgpu_bo_kmap(adev->dc->meta_bo, &adev->dc->meta_cpu_addr);
	if (r)
		gsgpu_bo_unpin(adev->dc->meta_bo);
	gsgpu_bo_unreserve(adev->dc->meta_bo);

	adev->dc->meta_gpu_addr = gsgpu_bo_gpu_offset(adev->dc->meta_bo);

	if (clear)
		memset(adev->dc->meta_cpu_addr, 0x00, 0x200000);

	meta_gpu_addr = adev->dc->meta_gpu_addr;
	for (i = 0; i < adev->mode_info.num_crtc; i++) {
		dc_writel(adev, CURRENT_REG(DC_CRTC_META0_REG_L, i), (u32)meta_gpu_addr);
		dc_writel(adev, CURRENT_REG(DC_CRTC_META0_REG_H, i), (meta_gpu_addr >> 32));
		dc_writel(adev, CURRENT_REG(DC_CRTC_META1_REG_L, i), (u32)meta_gpu_addr);
		dc_writel(adev, CURRENT_REG(DC_CRTC_META1_REG_H, i), (meta_gpu_addr >> 32));
	}

	DRM_INFO("DC META of 2M enabled (table at 0x%016llX).\n",
		 (unsigned long long)adev->dc->meta_gpu_addr);

	return 0;
}

static int gsgpu_dc_meta_disable(struct gsgpu_device *adev)
{
	int r, i;

	if (adev->dc->meta_bo == NULL) {
		dev_err(adev->dev, "No VRAM object for PCIE DC_META.\n");
		return -EINVAL;
	}

	for (i = 0; i < adev->mode_info.num_crtc; i++) {
		dc_writel(adev, CURRENT_REG(DC_CRTC_META0_REG_L, i), 0);
		dc_writel(adev, CURRENT_REG(DC_CRTC_META0_REG_H, i), 0);
		dc_writel(adev, CURRENT_REG(DC_CRTC_META1_REG_L, i), 0);
		dc_writel(adev, CURRENT_REG(DC_CRTC_META1_REG_H, i), 0);
	}

	r = gsgpu_bo_reserve(adev->dc->meta_bo, true);
	if (likely(r == 0)) {
		gsgpu_bo_kunmap(adev->dc->meta_bo);
		gsgpu_bo_unpin(adev->dc->meta_bo);
		gsgpu_bo_unreserve(adev->dc->meta_bo);
		adev->dc->meta_cpu_addr = NULL;
	}

	return 0;
}

static void gsgpu_dc_meta_set(struct gsgpu_device *adev)
{
	uint64_t meta_gpu_addr;
	int dc_meta_size = 0x200000;
	int i;
	int r;

	r = gsgpu_bo_create_kernel(adev, dc_meta_size,
				   GSGPU_GEM_COMPRESSED_SIZE,
				   GSGPU_GEM_DOMAIN_VRAM, &adev->dc->meta_bo,
				   &meta_gpu_addr, &adev->dc->meta_cpu_addr);
	if (r) {
		dev_warn(adev->dev, "(%d) create dc meta bo failed\n", r);
		return;
	}

	adev->dc->meta_gpu_addr = meta_gpu_addr;
	memset(adev->dc->meta_cpu_addr, 0x00, dc_meta_size);
	for (i = 0; i < adev->mode_info.num_crtc; i++) {
		dc_writel(adev, CURRENT_REG(DC_CRTC_META0_REG_L, i), (u32)meta_gpu_addr);
		dc_writel(adev, CURRENT_REG(DC_CRTC_META0_REG_H, i), (meta_gpu_addr >> 32));
		dc_writel(adev, CURRENT_REG(DC_CRTC_META1_REG_L, i), (u32)meta_gpu_addr);
		dc_writel(adev, CURRENT_REG(DC_CRTC_META1_REG_H, i), (meta_gpu_addr >> 32));
	}

	DRM_INFO("Config crtc number:%d meta addr 0x%llx\n", i, meta_gpu_addr);
}

static void gsgpu_dc_meta_free(struct gsgpu_device *adev)
{
	int i;

	gsgpu_bo_free_kernel(&adev->dc->meta_bo,
			     &adev->dc->meta_gpu_addr,
			     &adev->dc->meta_cpu_addr);

	for (i = 0; i < adev->mode_info.num_crtc; i++) {
		dc_writel(adev, CURRENT_REG(DC_CRTC_META0_REG_L, i), 0);
		dc_writel(adev, CURRENT_REG(DC_CRTC_META0_REG_H, i), 0);
		dc_writel(adev, CURRENT_REG(DC_CRTC_META1_REG_L, i), 0);
		dc_writel(adev, CURRENT_REG(DC_CRTC_META1_REG_H, i), 0);
	}
	adev->dc->meta_bo = NULL;

	DRM_INFO("Free crtc number:%d meta addr\n", i);
}

static int gsgpu_dc_atomic_commit(struct drm_device *dev,
				  struct drm_atomic_state *state,
				  bool nonblock)
{
	return drm_atomic_helper_commit(dev, state, nonblock);
}

static const struct drm_mode_config_funcs gsgpu_dc_mode_funcs = {
	.fb_create = gsgpu_display_user_framebuffer_create,
	.output_poll_changed = drm_fb_helper_output_poll_changed,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = gsgpu_dc_atomic_commit,
};

static bool modeset_required(struct drm_crtc_state *crtc_state)
{
	if (!drm_atomic_crtc_needs_modeset(crtc_state))
		return false;

	if (!crtc_state->enable)
		return false;

	return crtc_state->active;
}

static bool modereset_required(struct drm_crtc_state *crtc_state)
{
	if (!drm_atomic_crtc_needs_modeset(crtc_state))
		return false;

	return !crtc_state->enable || !crtc_state->active;
}

static void prepare_flip_isr(struct gsgpu_crtc *acrtc)
{
	assert_spin_locked(&acrtc->base.dev->event_lock);
	WARN_ON(acrtc->event);

	acrtc->event = acrtc->base.state->event;

	/* Set the flip status */
	acrtc->pflip_status = GSGPU_FLIP_SUBMITTED;

	/* Mark this event as consumed */
	acrtc->base.state->event = NULL;

	DRM_DEBUG_DRIVER("crtc:%d, pflip_stat:GSGPU_FLIP_SUBMITTED\n",
						 acrtc->crtc_id);
}

static void gsgpu_dc_do_flip(struct drm_crtc *crtc,
			      struct drm_framebuffer *fb,
			      uint32_t target)
{
	unsigned long flags;
	uint32_t target_vblank;
	int r, vpos, hpos;
	struct dc_plane_update plane;
	struct gsgpu_crtc *acrtc = to_gsgpu_crtc(crtc);
	struct gsgpu_framebuffer *afb = to_gsgpu_framebuffer(fb);
	struct gsgpu_bo *abo = gem_to_gsgpu_bo(fb->obj[0]);
	struct gsgpu_device *adev = crtc->dev->dev_private;
	uint64_t crtc_array_mode, crtc_address;

	/* Prepare wait for target vblank early - before the fence-waits */
	target_vblank = target - (uint32_t)drm_crtc_vblank_count(crtc) +
			gsgpu_get_vblank_counter_kms(crtc);

	/* TODO This might fail and hence better not used, wait
	 * explicitly on fences instead
	 * and in general should be called for
	 * blocking commit to as per framework helpers
	 */
	r = gsgpu_bo_reserve(abo, true);
	if (unlikely(r != 0)) {
		DRM_ERROR("failed to reserve buffer before flip\n");
		WARN_ON(1);
	}

	/* Wait for all fences on this FB */
	WARN_ON(dma_resv_wait_timeout_rcu(abo->tbo.resv, true, false,
					  MAX_SCHEDULE_TIMEOUT) < 0);

	gsgpu_bo_unreserve(abo);

	/* Wait until we're out of the vertical blank period before the one
	 * targeted by the flip
	 */
	while ((acrtc->enabled &&
		(gsgpu_display_get_crtc_scanoutpos(adev->ddev, acrtc->crtc_id,
						    0, &vpos, &hpos, NULL,
						    NULL, &crtc->hwmode)
		 & (DRM_SCANOUTPOS_VALID | DRM_SCANOUTPOS_IN_VBLANK)) ==
		(DRM_SCANOUTPOS_VALID | DRM_SCANOUTPOS_IN_VBLANK) &&
		(int)(target_vblank - gsgpu_get_vblank_counter_kms(crtc)) > 0)) {
		usleep_range(1000, 1100);
	}

	/* Flip */
	spin_lock_irqsave(&crtc->dev->event_lock, flags);

	WARN_ON(acrtc->pflip_status != GSGPU_FLIP_NONE);

	if (acrtc->base.state->event)
		prepare_flip_isr(acrtc);

	spin_unlock_irqrestore(&crtc->dev->event_lock, flags);

	crtc_array_mode = GSGPU_TILING_GET(abo->tiling_flags, ARRAY_MODE);
	switch (crtc_array_mode) {
	case 0:
		crtc_address = afb->address + crtc->y * afb->base.pitches[0] + ALIGN(crtc->x, 8) * 4;
		break;
	case 2:
		crtc_address = afb->address + crtc->y * afb->base.pitches[0] + ALIGN(crtc->x, 8) * 4 * 4;
		break;
	}

	plane.type = DC_PLANE_PRIMARY;
	plane.primary.address.low_part = lower_32_bits(crtc_address);
	plane.primary.address.high_part = upper_32_bits(crtc_address);

	dc_submit_plane_update(adev->dc, acrtc->crtc_id, &plane);

	DRM_DEBUG_DRIVER("%s Flipping to hi: 0x%x, low: 0x%x \n",
			 __func__,
			 upper_32_bits(afb->address),
			 lower_32_bits(afb->address));
}

static void gsgpu_dc_commit_planes(struct drm_atomic_state *state,
				   struct drm_device *dev,
				   struct drm_crtc *pcrtc,
				   bool *wait_for_vblank)
{
	struct drm_plane *plane;
	struct drm_plane_state *old_plane_state, *new_plane_state;
	struct gsgpu_crtc *acrtc = to_gsgpu_crtc(pcrtc);
	struct drm_crtc_state *new_pcrtc_state =
			drm_atomic_get_new_crtc_state(state, pcrtc);
	struct gsgpu_device *adev = dev->dev_private;
	struct gsgpu_dc_crtc *dc_crtc = adev->dc->link_info[acrtc->crtc_id].crtc;
	struct drm_framebuffer *modeset_fbs[1];
	int planes_count = 0;
	uint64_t tiling_flags = 0;
	unsigned long flags;
	uint32_t i;
	int x, y, cpp, ret;

	/* update planes when needed */
	for_each_oldnew_plane_in_state(state, plane, old_plane_state, new_plane_state, i) {
		struct drm_crtc *crtc = new_plane_state->crtc;
		struct drm_crtc_state *new_crtc_state;
		struct drm_framebuffer *fb = new_plane_state->fb;
		bool pflip_needed;

		if (plane->type == DRM_PLANE_TYPE_CURSOR) {
			handle_cursor_update(plane, old_plane_state);
			continue;
		}

		if (!fb || !crtc || pcrtc != crtc)
			continue;

		new_crtc_state = drm_atomic_get_new_crtc_state(state, crtc);
		if (!new_crtc_state->active) {
			dc_crtc_enable(acrtc, false);
			continue;
		} else
			dc_crtc_enable(acrtc, true);

		x = plane->state->crtc->x;
		y = plane->state->crtc->y;
		pflip_needed = !state->allow_modeset;

		spin_lock_irqsave(&crtc->dev->event_lock, flags);
		if (acrtc->pflip_status != GSGPU_FLIP_NONE) {
			DRM_ERROR("%s: acrtc %d, already busy\n",
				  __func__, acrtc->crtc_id);
			/* In commit tail framework this cannot happen */
			WARN_ON(1);
		}
		spin_unlock_irqrestore(&crtc->dev->event_lock, flags);

		if (!pflip_needed) {
			planes_count++;
			modeset_fbs[0] = fb;
		} else if (new_crtc_state->planes_changed) {
			/* Assume even ONE crtc with immediate flip means
			 * entire can't wait for VBLANK
			 * TODO Check if it's correct
			 */
			*wait_for_vblank =
				new_pcrtc_state->pageflip_flags & DRM_MODE_PAGE_FLIP_ASYNC ?
				false : true;

			/* TODO: Needs rework for multiplane flip */
			if (plane->type == DRM_PLANE_TYPE_PRIMARY)
				drm_crtc_vblank_get(crtc);

			gsgpu_dc_do_flip(crtc, fb,
				(uint32_t)drm_crtc_vblank_count(crtc) + *wait_for_vblank);
		}
	}

	if (planes_count) {
		unsigned long flags;
		struct dc_timing_info timing;
		struct dc_plane_update plane;
		struct drm_display_mode *mode = &pcrtc->mode;
		uint64_t address;

		struct gsgpu_framebuffer *afb =
				to_gsgpu_framebuffer(modeset_fbs[0]);

		struct gsgpu_bo *bo = gem_to_gsgpu_bo(afb->base.obj[0]);
		ret = gsgpu_bo_reserve(bo, false);
		if (unlikely(ret)) {
			if (ret != -ERESTARTSYS)
				DRM_ERROR("Unable to reserve buffer: %d\n", ret);
			return;
		}
		gsgpu_bo_get_tiling_flags(bo, &tiling_flags);
		gsgpu_bo_unreserve(bo);
		dc_crtc->array_mode = GSGPU_TILING_GET(tiling_flags, ARRAY_MODE);

		if (new_pcrtc_state->event) {
			drm_crtc_vblank_get(pcrtc);
			spin_lock_irqsave(&pcrtc->dev->event_lock, flags);
			prepare_flip_isr(acrtc);
			spin_unlock_irqrestore(&pcrtc->dev->event_lock, flags);
		}

		cpp = afb->base.format->cpp[0];
		timing.depth = afb->base.format->cpp[0] << 3;
		timing.stride = afb->base.pitches[0];
		timing.clock = mode->clock;
		timing.hdisplay = mode->hdisplay;
		timing.htotal = mode->htotal;
		timing.hsync_start = mode->hsync_start;
		timing.hsync_end  = mode->hsync_end;
		timing.vdisplay = mode->vdisplay;
		timing.vtotal = mode->vtotal;
		timing.vsync_start = mode->vsync_start;
		timing.vsync_end = mode->vsync_end;
		timing.use_dma32 = 0;

		if (x != 0 && x % 64 && dc_crtc->array_mode == 0)
			timing.use_dma32 = 1;

		dc_submit_timing_update(adev->dc, acrtc->crtc_id, &timing);

		DRM_DEBUG_DRIVER("gsgpu crtc-%d hdisplay %d vdisplay %d x %d y %d cpp %d stride %d\n",
				 acrtc->crtc_id, timing.hdisplay, timing.vdisplay,
				 x, y, cpp, timing.stride);

		plane.type = DC_PLANE_PRIMARY;
		switch (dc_crtc->array_mode) {
		case 0:
			address = afb->address + y * timing.stride + ALIGN(x, 8) * cpp;
			break;
		case 2:
			y = (y + 3) & ~3;
			x = ALIGN(x, 8);
			address = afb->address + y * timing.stride + x * cpp * 4;
			break;
		}

		plane.primary.address.low_part = lower_32_bits(address);
		plane.primary.address.high_part = upper_32_bits(address);
		dc_submit_plane_update(adev->dc, acrtc->crtc_id, &plane);
	}
}

void gsgpu_dc_atomic_commit_tail(struct drm_atomic_state *state)
{
	struct drm_device *dev = state->dev;
	struct gsgpu_device *adev = dev->dev_private;
	uint32_t i, j;
	struct drm_crtc *crtc;
	struct drm_crtc_state *old_crtc_state, *new_crtc_state;
	unsigned long flags;
	bool wait_for_vblank = true;
	int crtc_disable_count = 0;

	/*
	 * We evade vblanks and pflips on crtc that
	 * should be changed. We do it here to flush & disable
	 * interrupts before drm_swap_state is called in drm_atomic_helper_commit
	 * it will update crtc->dm_crtc_state->stream pointer which is used in
	 * the ISRs.
	 */
	for_each_oldnew_crtc_in_state(state, crtc, old_crtc_state, new_crtc_state, i) {
		struct gsgpu_crtc *acrtc = to_gsgpu_crtc(crtc);

		if (drm_atomic_crtc_needs_modeset(new_crtc_state)) {
			manage_dc_interrupts(adev, acrtc, false);
		}
	}

	drm_atomic_helper_update_legacy_modeset_state(dev, state);

	/* update changed items */
	for_each_oldnew_crtc_in_state(state, crtc, old_crtc_state, new_crtc_state, i) {
		struct gsgpu_crtc *acrtc = to_gsgpu_crtc(crtc);

		DRM_DEBUG_DRIVER(
			"gsgpu_crtc id:%d crtc_state_flags: enable:%d, active:%d, "
			"planes_changed:%d, mode_changed:%d,active_changed:%d,"
			"connectors_changed:%d\n",
			acrtc->crtc_id,
			new_crtc_state->enable,
			new_crtc_state->active,
			new_crtc_state->planes_changed,
			new_crtc_state->mode_changed,
			new_crtc_state->active_changed,
			new_crtc_state->connectors_changed);

		/* handles headless hotplug case, updating new_state and
		 * aconnector as needed
		 */
		if (modeset_required(new_crtc_state)) {
			DRM_DEBUG_DRIVER("Atomic commit: SET crtc id %d: [%p]\n", acrtc->crtc_id, acrtc);
			pm_runtime_get_noresume(dev->dev);
			acrtc->enabled = true;
			acrtc->hw_mode = new_crtc_state->mode;
			crtc->hwmode = new_crtc_state->mode;
		} else if (modereset_required(new_crtc_state)) {
			DRM_DEBUG_DRIVER("gsgpu_crtc id:%d enable:%d, active:%d\n", acrtc->crtc_id,
				new_crtc_state->enable,	new_crtc_state->active);
			acrtc->enabled = false;
		}
	} /* for_each_crtc_in_state() */

	for_each_oldnew_crtc_in_state(state, crtc, old_crtc_state, new_crtc_state, i) {
		/*
		 * loop to enable interrupts on newly arrived crtc
		 */
		struct gsgpu_crtc *acrtc = to_gsgpu_crtc(crtc);
		bool modeset_needed;

		if (old_crtc_state->active && !new_crtc_state->active)
			crtc_disable_count++;

		modeset_needed = modeset_required(new_crtc_state);

		if (!modeset_needed)
			continue;

		manage_dc_interrupts(adev, acrtc, true);
	}

	/* update planes when needed per crtc*/
	for_each_new_crtc_in_state(state, crtc, new_crtc_state, j) {
		gsgpu_dc_commit_planes(state, dev, crtc, &wait_for_vblank);
	}

	/*
	 * send vblank event on all events not handled in flip and
	 * mark consumed event for drm_atomic_helper_commit_hw_done
	 */
	spin_lock_irqsave(&adev->ddev->event_lock, flags);
	for_each_new_crtc_in_state(state, crtc, new_crtc_state, i) {

		if (new_crtc_state->event)
			drm_send_event_locked(dev, &new_crtc_state->event->base);

		new_crtc_state->event = NULL;
	}
	spin_unlock_irqrestore(&adev->ddev->event_lock, flags);

	drm_atomic_helper_commit_hw_done(state);

	if (wait_for_vblank)
		drm_atomic_helper_wait_for_flip_done(dev, state);

	drm_atomic_helper_cleanup_planes(dev, state);

	/* Finally, drop a runtime PM reference for each newly disabled CRTC,
	 * so we can put the GPU into runtime suspend if we're not driving any
	 * displays anymore
	 */
	for (i = 0; i < crtc_disable_count; i++)
		pm_runtime_put_autosuspend(dev->dev);
	pm_runtime_mark_last_busy(dev->dev);
}

static struct drm_mode_config_helper_funcs gsgpu_dc_mode_config_helper = {
	.atomic_commit_tail = gsgpu_dc_atomic_commit_tail
};

static int dc_mode_config_init(struct gsgpu_device *adev)
{
	int r;

	adev->mode_info.mode_config_initialized = true;

	adev->ddev->mode_config.funcs = (void *)&gsgpu_dc_mode_funcs;
	adev->ddev->mode_config.helper_private = &gsgpu_dc_mode_config_helper;

	adev->ddev->mode_config.max_width = 16384;
	adev->ddev->mode_config.max_height = 16384;

	adev->ddev->mode_config.preferred_depth = 24;
	adev->ddev->mode_config.prefer_shadow = 1;
	/* indicate support of immediate flip */
	adev->ddev->mode_config.async_page_flip = true;

	adev->ddev->mode_config.fb_base = adev->gmc.aper_base;

	r = gsgpu_display_modeset_create_props(adev);
	if (r)
		return r;

	return 0;
}

static void gsgpu_attach_encoder_connector(struct gsgpu_device *adev)
{
	struct gsgpu_connector *lconnector;
	struct gsgpu_encoder *lencoder;
	int i;

	for (i = 0; i < adev->mode_info.num_crtc; i++) {
		lconnector = adev->mode_info.connectors[i];
		lencoder = adev->mode_info.encoders[i];
		drm_connector_attach_encoder(&lconnector->base, &lencoder->base);
	}
}

static void gsgpu_display_print_display_setup(struct drm_device *dev)
{
	struct gsgpu_device *adev = dev->dev_private;
	struct drm_crtc *crtc;
	struct gsgpu_crtc *gsgpu_crtc;
	struct drm_connector *connector;
	struct gsgpu_connector *gsgpu_connector;
	struct drm_encoder *encoder;
	struct gsgpu_encoder *gsgpu_encoder;

	DRM_DEBUG_DRIVER("GSGPU DC revision %d\n", adev->dc_revision);
	DRM_INFO("GSGPU Display Crtcs\n");
	list_for_each_entry(crtc, &dev->mode_config.crtc_list, head) {
		gsgpu_crtc = to_gsgpu_crtc(crtc);
		DRM_INFO("Crtc %d: name:%s\n", crtc->index, crtc->name);
	}

	DRM_INFO("GSGPU Display Connectors\n");
	list_for_each_entry(connector, &dev->mode_config.connector_list, head) {
		gsgpu_connector = to_gsgpu_connector(connector);
		DRM_INFO("Connector %d: name:%s\n", connector->index, connector->name);
	}

	DRM_INFO("GSGPU Display Encoders\n");
	list_for_each_entry(encoder, &dev->mode_config.encoder_list, head) {
		gsgpu_encoder = to_gsgpu_encoder(encoder);
		DRM_INFO("Encoder %d: name:%s\n", encoder->index, encoder->name);
	}
}

static int dc_initialize_drm_device(struct gsgpu_device *adev)
{
	struct gsgpu_mode_info *mode_info = &adev->mode_info;
	int32_t total_primary_planes;
	int32_t i;

	if (dc_mode_config_init(adev)) {
		DRM_ERROR("Failed to initialize mode config\n");
		return -1;
	}

	total_primary_planes = 2;
	for (i = (total_primary_planes - 1); i >= 0; i--) {
		if (initialize_plane(adev, mode_info, i)) {
			DRM_ERROR("KMS: Failed to initialize primary plane\n");
			goto fail;
		}
	}

	for (i = 0; i < 2; i++) {
		if (gsgpu_dc_crtc_init(adev, &mode_info->planes[i]->base, i)) {
			DRM_ERROR("KMS: Failed to initialize crtc\n");
			goto fail;
		}
	}

	for (i = 0; i < 2; i++) {
		gsgpu_i2c_init(adev, i);
		if (gsgpu_dc_encoder_init(adev, i)) {
			DRM_ERROR("KMS: Failed to initialize encoder\n");
			goto fail;
		}

		if (gsgpu_dc_bridge_init(adev, i)) {
			DRM_ERROR("KMS: Failed to initialize bridge\n");
			goto fail;
		}

		if (gsgpu_dc_connector_init(adev, i)) {
			DRM_ERROR("KMS: Failed to initialize connector\n");
			goto fail;
		}
	}

	gsgpu_attach_encoder_connector(adev);
	if (dc_register_irq_handlers(adev)) {
		DRM_ERROR("DC: Failed to initialize IRQ\n");
		goto fail;
	}

	return 0;

fail:
	for (i = 0; i < 2/*max_planes*/; i++)
		kfree(mode_info->planes[i]);
	return -1;
}

static void gsgpu_dc_fini(struct gsgpu_device *adev)
{
	drm_mode_config_cleanup(adev->ddev);
	/*
	 * TODO: pageflip, vlank interrupt
	 *
	 * gsgpu_dc_irq_fini(adev);
	 */

	return;
}

#define DC_MAX_PLANES 4
static const enum drm_plane_type plane_type[DC_MAX_PLANES] = {
	DRM_PLANE_TYPE_PRIMARY,
	DRM_PLANE_TYPE_PRIMARY,
};

static int dc_early_init(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	adev->mode_info.num_crtc = 2;
	adev->mode_info.num_i2c = 2;
	adev->mode_info.num_hpd = 3;
	adev->mode_info.plane_type = plane_type;

	dc_set_irq_funcs(adev);

	if (adev->mode_info.funcs == NULL)
		adev->mode_info.funcs = &dc_display_funcs;

	return 0;
}

static int dc_late_init(void *handle)
{
	return 0;
}

static int dc_sw_init(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	adev->dc = dc_construct(adev);
	if (adev->dc) {
		DRM_INFO("GSGPU Display Core initialized with v%s!\n", DC_VER);
	} else {
		DRM_INFO("GSGPU Display Core failed to init with v%s!\n", DC_VER);
		goto error;
	}

	DRM_INFO("GSGPU DC construct links:%d", adev->dc->links);
	DRM_DEBUG_DRIVER("GSGPU DC sw init success!\n");

	return 0;

error:
	gsgpu_dc_fini(adev);
	return -1;
}

static int dc_sw_fini(void *handle)
{
	return 0;
}

static int dc_hw_init(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	if (gsgpu_dc_irq_init(adev)) {
		DRM_ERROR("Failed to initialize IRQ support.\n");
		goto error;
	}

	if (dc_initialize_drm_device(adev)) {
		DRM_ERROR("Failed to initialize sw for display support.\n");
		goto error;
	}
	gsgpu_display_print_display_setup(adev->ddev);

	adev->mode_info.num_crtc = adev->dc->links;
	adev->ddev->mode_config.cursor_width = 32;
	adev->ddev->mode_config.cursor_height = 32;

	if (drm_vblank_init(adev->ddev, adev->dc->links)) {
		DRM_ERROR("Failed to initialize vblank.\n");
		goto error;
	}

	drm_mode_config_reset(adev->ddev);
	drm_kms_helper_poll_init(adev->ddev);
	gsgpu_dc_meta_set(adev);
	gsgpu_dc_hpd_init(adev);

	DRM_DEBUG_DRIVER("GSGPU DC hw init success!\n");

	return 0;
error:
	gsgpu_dc_fini(adev);
	return -1;
}

static int dc_hw_fini(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	gsgpu_dc_irq_fini(adev);
	gsgpu_dc_meta_free(adev);
	gsgpu_dc_fini(adev);
	return 0;
}

static int dc_suspend(void *handle)
{
	struct gsgpu_device *adev = handle;

	WARN_ON(adev->dc->cached_state);

	adev->dc->cached_state = drm_atomic_helper_suspend(adev->ddev);

	gsgpu_hdmi_suspend(adev);
	gsgpu_dc_meta_disable(adev);
	gsgpu_dc_hpd_disable(adev);

	return 0;
}

static int dc_resume(void *handle)
{
	struct gsgpu_device *adev = handle;
	struct drm_device *ddev = adev->ddev;
	struct gsgpu_dc *dc = adev->dc;
	struct drm_crtc *crtc;
	struct drm_crtc_state *new_crtc_state;
	int i;

	for_each_new_crtc_in_state(dc->cached_state, crtc, new_crtc_state, i)
		new_crtc_state->active_changed = true;

	drm_atomic_helper_resume(ddev, dc->cached_state);

	dc->cached_state = NULL;

	gsgpu_dc_meta_enable(adev, true);
	gsgpu_dc_hpd_init(adev);
	gsgpu_hdmi_resume(adev);

	return 0;
}

static bool dc_is_idle(void *handle)
{
	return true;
}

static int dc_wait_for_idle(void *handle)
{
	return 0;
}

static bool dc_check_soft_reset(void *handle)
{
	return false;
}

static int dc_soft_reset(void *handle)
{
	return 0;
}

static const struct gsgpu_ip_funcs gsgpu_dc_funcs = {
	.name = "display",
	.early_init = dc_early_init,
	.late_init = dc_late_init,
	.sw_init = dc_sw_init,
	.sw_fini = dc_sw_fini,
	.hw_init = dc_hw_init,
	.hw_fini = dc_hw_fini,
	.suspend = dc_suspend,
	.resume = dc_resume,
	.is_idle = dc_is_idle,
	.wait_for_idle = dc_wait_for_idle,
	.check_soft_reset = dc_check_soft_reset,
	.soft_reset = dc_soft_reset,
};

const struct gsgpu_ip_block_version dc_ip_block = {
	.type = GSGPU_IP_BLOCK_TYPE_DCE,
	.major = 1,
	.minor = 0,
	.rev = 0,
	.funcs = &gsgpu_dc_funcs,
};
