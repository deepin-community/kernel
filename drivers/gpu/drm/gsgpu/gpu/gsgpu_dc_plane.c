#include <drm/drm_atomic_helper.h>
#include "gsgpu.h"
#include "gsgpu_display.h"
#include "gsgpu_dc_plane.h"

static const uint32_t rgb_formats[] = {
	DRM_FORMAT_RGB565,
	DRM_FORMAT_RGB888,
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_ARGB8888,
};

static const u32 cursor_formats[] = {
	DRM_FORMAT_ARGB8888
};

static int dc_plane_helper_prepare_fb(struct drm_plane *plane,
				      struct drm_plane_state *new_state)
{
	struct gsgpu_framebuffer *afb;
	struct drm_gem_object *obj;
	struct gsgpu_device *adev;
	struct gsgpu_bo *rbo;
	uint32_t domain;
	u64 fb_addr;
	void *fb_vaddr;
	int r;

	if (!new_state->fb) {
		DRM_DEBUG_DRIVER("No FB bound\n");
		return 0;
	}

	afb = to_gsgpu_framebuffer(new_state->fb);
	obj = new_state->fb->obj[0];
	rbo = gem_to_gsgpu_bo(obj);
	adev = gsgpu_ttm_adev(rbo->tbo.bdev);
	r = gsgpu_bo_reserve(rbo, false);
	if (unlikely(r != 0))
		return r;

	if (plane->type != DRM_PLANE_TYPE_CURSOR)
		domain = gsgpu_display_supported_domains(adev);
	else
		domain = GSGPU_GEM_DOMAIN_VRAM;

	r = gsgpu_bo_pin(rbo, domain);
	if (unlikely(r != 0)) {
		if (r != -ERESTARTSYS)
			DRM_ERROR("Failed to pin framebuffer with error %d\n", r);
		gsgpu_bo_unreserve(rbo);
		return r;
	}

	r = gsgpu_bo_kmap(rbo, &fb_vaddr);
	if (unlikely(r != 0)) {
		gsgpu_bo_unpin(rbo);
		gsgpu_bo_unreserve(rbo);
		DRM_ERROR("0x%px kmap failed\n", rbo);
		return r;
	}

	if (gsgpu_use_system_ram) {
		fb_addr = virt_to_phys(fb_vaddr);
		fb_addr = (fb_addr & 0x1ffffffffffffULL);
		afb->address = fb_addr;
	} else {
		afb->address = gsgpu_bo_gpu_offset(rbo);
	}

	if (plane->type == DRM_PLANE_TYPE_PRIMARY) {
		DRM_DEBUG_DRIVER("fb kernel virtual addr: 0x%px\n", fb_vaddr);
		DRM_DEBUG_DRIVER("fb physical addr: 0x%llx\n", afb->address);
	}

	gsgpu_bo_unreserve(rbo);
	gsgpu_bo_ref(rbo);

	return 0;
}

static void dc_plane_helper_cleanup_fb(struct drm_plane *plane,
				       struct drm_plane_state *old_state)
{
	struct gsgpu_bo *rbo;
	int r;

	if (!old_state->fb)
		return;

	rbo = gem_to_gsgpu_bo(old_state->fb->obj[0]);
	r = gsgpu_bo_reserve(rbo, false);
	if (unlikely(r)) {
		DRM_ERROR("failed to reserve rbo before unpin\n");
		return;
	}

	gsgpu_bo_unpin(rbo);
	gsgpu_bo_unreserve(rbo);
	gsgpu_bo_unref(&rbo);
}

static int dc_plane_atomic_check(struct drm_plane *plane,
				 struct drm_atomic_state *state)
{
	return 0;
}

static const struct drm_plane_helper_funcs dc_plane_helper_funcs = {
	.prepare_fb = dc_plane_helper_prepare_fb,
	.cleanup_fb = dc_plane_helper_cleanup_fb,
	.atomic_check = dc_plane_atomic_check,
};

static void dc_plane_destroy(struct drm_plane *plane)
{
	struct gsgpu_plane *p = to_gsgpu_plane(plane);

	drm_plane_cleanup(plane);
	kfree(p);
}

static const struct drm_plane_funcs dc_plane_funcs = {
	.update_plane = drm_atomic_helper_update_plane,
	.disable_plane = drm_atomic_helper_disable_plane,
	.destroy = dc_plane_destroy,
	.reset = drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_plane_destroy_state,
};

int gsgpu_dc_plane_init(struct gsgpu_device *adev,
			struct gsgpu_plane *aplane,
			unsigned long possible_crtcs)
{
	int res = -EPERM;

	switch (aplane->base.type) {
	case DRM_PLANE_TYPE_PRIMARY:
		res = drm_universal_plane_init(
				adev->ddev,
				&aplane->base,
				possible_crtcs,
				&dc_plane_funcs,
				rgb_formats,
				ARRAY_SIZE(rgb_formats),
				NULL, aplane->base.type, NULL);
		break;
	case DRM_PLANE_TYPE_CURSOR:
		res = drm_universal_plane_init(
				adev->ddev,
				&aplane->base,
				possible_crtcs,
				&dc_plane_funcs,
				cursor_formats,
				ARRAY_SIZE(cursor_formats),
				NULL, aplane->base.type, NULL);
		break;
	default:
		break;
	}

	drm_plane_helper_add(&aplane->base, &dc_plane_helper_funcs);

	return res;
}

int initialize_plane(struct gsgpu_device *adev,
		     struct gsgpu_mode_info *mode_info,
		     int plane_id)
{
	struct gsgpu_plane *plane;
	unsigned long possible_crtcs;
	int ret = 0;

	plane = kzalloc(sizeof(struct gsgpu_plane), GFP_KERNEL);
	mode_info->planes[plane_id] = plane;

	if (!plane) {
		DRM_ERROR("KMS: Failed to allocate plane\n");
		return -ENOMEM;
	}
	plane->base.type = mode_info->plane_type[plane_id];

	possible_crtcs = 1 << plane_id;

	ret = gsgpu_dc_plane_init(adev, mode_info->planes[plane_id], possible_crtcs);

	if (ret) {
		DRM_ERROR("KMS: Failed to initialize plane\n");
		return ret;
	}

	return ret;
}
