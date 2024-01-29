#include <linux/pm_runtime.h>

#include <drm/drm_crtc_helper.h>
#include <drm/drm_edid.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/gsgpu_drm.h>

#include "gsgpu.h"
#include "gsgpu_display.h"

static const struct drm_framebuffer_funcs gsgpu_fb_funcs = {
	.destroy = drm_gem_fb_destroy,
	.create_handle = drm_gem_fb_create_handle,
};

uint32_t gsgpu_display_supported_domains(struct gsgpu_device *adev)
{
	uint32_t domain = GSGPU_GEM_DOMAIN_VRAM;

	return domain;
}

int gsgpu_display_framebuffer_init(struct drm_device *dev,
				    struct gsgpu_framebuffer *rfb,
				    const struct drm_mode_fb_cmd2 *mode_cmd,
				    struct drm_gem_object *obj)
{
	int ret;
	rfb->base.obj[0] = obj;
	drm_helper_mode_fill_fb_struct(dev, &rfb->base, mode_cmd);
	ret = drm_framebuffer_init(dev, &rfb->base, &gsgpu_fb_funcs);
	if (ret) {
		rfb->base.obj[0] = NULL;
		return ret;
	}
	return 0;
}

struct drm_framebuffer *
gsgpu_display_user_framebuffer_create(struct drm_device *dev,
				       struct drm_file *file_priv,
				       const struct drm_mode_fb_cmd2 *mode_cmd)
{
	struct drm_gem_object *obj;
	struct gsgpu_framebuffer *gsgpu_fb;
	int ret;

	obj = drm_gem_object_lookup(file_priv, mode_cmd->handles[0]);
	if (obj ==  NULL) {
		dev_err(dev->dev, "No GEM object associated to handle 0x%08X, "
			"can't create framebuffer\n", mode_cmd->handles[0]);
		return ERR_PTR(-ENOENT);
	}

	/* Handle is imported dma-buf, so cannot be migrated to VRAM for scanout */
	uint32_t domains = gsgpu_display_supported_domains(drm_to_adev(dev));
	if (obj->import_attach && !(domains & GSGPU_GEM_DOMAIN_GTT)) {
		DRM_DEBUG_KMS("Cannot create framebuffer from imported dma_buf handle\n");
		return ERR_PTR(-EINVAL);
	}

	gsgpu_fb = kzalloc(sizeof(*gsgpu_fb), GFP_KERNEL);
	if (gsgpu_fb == NULL) {
		drm_gem_object_put(obj);
		return ERR_PTR(-ENOMEM);
	}

	ret = gsgpu_display_framebuffer_init(dev, gsgpu_fb, mode_cmd, obj);
	if (ret) {
		kfree(gsgpu_fb);
		drm_gem_object_put(obj);
		return ERR_PTR(ret);
	}

	return &gsgpu_fb->base;
}

static const struct drm_prop_enum_list gsgpu_audio_enum_list[] = {
	{ GSGPU_AUDIO_DISABLE, "off" },
	{ GSGPU_AUDIO_ENABLE, "on" },
	{ GSGPU_AUDIO_AUTO, "auto" },
};

int gsgpu_display_modeset_create_props(struct gsgpu_device *adev)
{
	int sz;

	drm_mode_create_scaling_mode_property(adev->ddev);

	sz = ARRAY_SIZE(gsgpu_audio_enum_list);
	adev->mode_info.audio_property =
		drm_property_create_enum(adev->ddev, 0,
					 "audio",
					 gsgpu_audio_enum_list, sz);

	return 0;
}

void gsgpu_display_update_priority(struct gsgpu_device *adev)
{
	/* adjustment options for the display watermarks */
	if ((gsgpu_disp_priority == 0) || (gsgpu_disp_priority > 2))
		adev->mode_info.disp_priority = 0;
	else
		adev->mode_info.disp_priority = gsgpu_disp_priority;

}

int gsgpu_display_get_crtc_scanoutpos(struct drm_device *dev, unsigned int pipe,
				      unsigned int flags, int *vpos,
				      int *hpos, ktime_t *stime, ktime_t *etime,
				      const struct drm_display_mode *mode)
{
	int vbl_start, vbl_end, vtotal, ret = 0;
	bool in_vbl = true;

	struct gsgpu_device *adev = dev->dev_private;

	/* preempt_disable_rt() should go right here in PREEMPT_RT patchset. */

	/* Get optional system timestamp before query. */
	if (stime)
		*stime = ktime_get();

	if (gsgpu_display_page_flip_get_scanoutpos(adev, pipe, &vbl_start,
						   &vbl_end, hpos, vpos) == 0)
		ret |= DRM_SCANOUTPOS_VALID;

	/* Get optional system timestamp after query. */
	if (etime)
		*etime = ktime_get();

	/* preempt_enable_rt() should go right here in PREEMPT_RT patchset. */

	/* Valid vblank area boundaries from gpu retrieved? */
	if (vbl_start < 0xffff && vbl_end < 0xffff) {
		/* Yes. */
		ret |= DRM_SCANOUTPOS_ACCURATE;
	} else {
		/* No: Fake something reasonable which gives at least ok results. */
		vbl_start = mode->crtc_vdisplay;
		vbl_end = 0;
	}

	/* Called from driver internal vblank counter query code? */
	if (flags & GET_DISTANCE_TO_VBLANKSTART) {
	    /* Caller wants distance from real vbl_start in *hpos */
	    *hpos = *vpos - vbl_start;
	}

	/* Fudge vblank to start a few scanlines earlier to handle the
	 * problem that vblank irqs fire a few scanlines before start
	 * of vblank. Some driver internal callers need the true vblank
	 * start to be used and signal this via the USE_REAL_VBLANKSTART flag.
	 *
	 * The cause of the "early" vblank irq is that the irq is triggered
	 * by the line buffer logic when the line buffer read position enters
	 * the vblank, whereas our crtc scanout position naturally lags the
	 * line buffer read position.
	 */
	if (!(flags & USE_REAL_VBLANKSTART))
		vbl_start -= adev->mode_info.crtcs[pipe]->lb_vblank_lead_lines;

	/* Test scanout position against vblank region. */
	if ((*vpos < vbl_start) && (*vpos >= vbl_end))
		in_vbl = false;

	/* In vblank? */
	if (in_vbl)
	    ret |= DRM_SCANOUTPOS_IN_VBLANK;

	/* Called from driver internal vblank counter query code? */
	if (flags & GET_DISTANCE_TO_VBLANKSTART) {
		/* Caller wants distance from fudged earlier vbl_start */
		*vpos -= vbl_start;
		return ret;
	}

	/* Check if inside vblank area and apply corrective offsets:
	 * vpos will then be >=0 in video scanout area, but negative
	 * within vblank area, counting down the number of lines until
	 * start of scanout.
	 */

	/* Inside "upper part" of vblank area? Apply corrective offset if so: */
	if (in_vbl && (*vpos >= vbl_start)) {
		vtotal = mode->crtc_vtotal;
		*vpos = *vpos - vtotal;
	}

	/* Correct for shifted end of vbl at vbl_end. */
	*vpos = *vpos - vbl_end;

	return ret;
}
