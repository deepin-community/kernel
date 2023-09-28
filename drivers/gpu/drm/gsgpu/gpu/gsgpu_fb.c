#include <linux/module.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>

#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/gsgpu_drm.h>
#include "gsgpu.h"

#include <drm/drm_fb_helper.h>

#include <linux/vga_switcheroo.h>

#include "gsgpu_display.h"

/* object hierarchy -
   this contains a helper + a gsgpu fb
   the helper contains a pointer to gsgpu framebuffer baseclass.
*/

static int
gsgpufb_open(struct fb_info *info, int user)
{
	struct gsgpu_fbdev *rfbdev = info->par;
	struct gsgpu_device *adev = rfbdev->adev;
	int ret = pm_runtime_get_sync(adev->ddev->dev);
	if (ret < 0 && ret != -EACCES) {
		pm_runtime_mark_last_busy(adev->ddev->dev);
		pm_runtime_put_autosuspend(adev->ddev->dev);
		return ret;
	}
	return 0;
}

static int
gsgpufb_release(struct fb_info *info, int user)
{
	struct gsgpu_fbdev *rfbdev = info->par;
	struct gsgpu_device *adev = rfbdev->adev;

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);
	return 0;
}

static struct fb_ops gsgpufb_ops = {
	.owner = THIS_MODULE,
	DRM_FB_HELPER_DEFAULT_OPS,
	.fb_open = gsgpufb_open,
	.fb_release = gsgpufb_release,
	FB_DEFAULT_IOMEM_OPS,
	DRM_FB_HELPER_DEFAULT_OPS,
};


int gsgpu_align_pitch(struct gsgpu_device *adev, int width, int cpp, bool tiled)
{
	int aligned = width;

	aligned *= cpp;
	aligned += 255;
	aligned &= ~255;

	return aligned;
}

static void gsgpufb_destroy_pinned_object(struct drm_gem_object *gobj)
{
	struct gsgpu_bo *abo = gem_to_gsgpu_bo(gobj);
	int ret;

	ret = gsgpu_bo_reserve(abo, true);
	if (likely(ret == 0)) {
		gsgpu_bo_kunmap(abo);
		gsgpu_bo_unpin(abo);
		gsgpu_bo_unreserve(abo);
	}
	drm_gem_object_put(gobj);
}

static int gsgpufb_create_pinned_object(struct gsgpu_fbdev *rfbdev,
					 struct drm_mode_fb_cmd2 *mode_cmd,
					 struct drm_gem_object **gobj_p)
{
	struct gsgpu_device *adev = rfbdev->adev;
	const struct drm_format_info *info = drm_get_format_info(adev->ddev, mode_cmd);
	struct drm_gem_object *gobj = NULL;
	struct gsgpu_bo *abo = NULL;
	bool fb_tiled = false; /* useful for testing */
	u32 tiling_flags = 0, domain;
	int ret;
	int aligned_size, size;
	int height = mode_cmd->height;
	u32 cpp;

	cpp = info->cpp[0];

	/* need to align pitch with crtc limits */
	mode_cmd->pitches[0] = gsgpu_align_pitch(adev, mode_cmd->width, cpp,
						  fb_tiled);
	domain = gsgpu_display_supported_domains(adev);

	height = ALIGN(mode_cmd->height, 8);
	size = mode_cmd->pitches[0] * height;
	aligned_size = ALIGN(size, PAGE_SIZE);

	ret = gsgpu_gem_object_create(adev, aligned_size, 0, domain,
				       GSGPU_GEM_CREATE_CPU_ACCESS_REQUIRED |
				       GSGPU_GEM_CREATE_VRAM_CONTIGUOUS |
				       GSGPU_GEM_CREATE_VRAM_CLEARED,
				       ttm_bo_type_device, NULL, &gobj);

	if (ret) {
		pr_err("failed to allocate framebuffer (%d)\n", aligned_size);
		return -ENOMEM;
	}
	abo = gem_to_gsgpu_bo(gobj);

	ret = gsgpu_bo_reserve(abo, false);
	if (unlikely(ret != 0))
		goto out_unref;

	if (tiling_flags) {
		ret = gsgpu_bo_set_tiling_flags(abo,
						 tiling_flags);
		if (ret)
			dev_err(adev->dev, "FB failed to set tiling flags\n");
	}

	ret = gsgpu_bo_pin(abo, domain);
	if (ret) {
		gsgpu_bo_unreserve(abo);
		goto out_unref;
	}

	ret = gsgpu_ttm_alloc_gart(&abo->tbo);
	if (ret) {
		gsgpu_bo_unreserve(abo);
		dev_err(adev->dev, "%p bind failed\n", abo);
		goto out_unref;
	}

	ret = gsgpu_bo_kmap(abo, NULL);
	gsgpu_bo_unreserve(abo);
	if (ret) {
		goto out_unref;
	}

	//for (i = 0; i < abo->tbo.ttm->num_pages; i++) {
	//	pr_err("page[%d], 0x%lX\n", i, page_to_pfn(abo->tbo.ttm->pages[i]));
	//}

	*gobj_p = gobj;
	return 0;
out_unref:
	gsgpufb_destroy_pinned_object(gobj);
	*gobj_p = NULL;
	return ret;
}

static int gsgpufb_create(struct drm_fb_helper *helper,
			   struct drm_fb_helper_surface_size *sizes)
{
	struct gsgpu_fbdev *rfbdev = (struct gsgpu_fbdev *)helper;
	struct gsgpu_device *adev = rfbdev->adev;
	struct fb_info *info;
	struct drm_framebuffer *fb = NULL;
	struct drm_mode_fb_cmd2 mode_cmd;
	struct drm_gem_object *gobj = NULL;
	struct gsgpu_bo *abo = NULL;
	unsigned long tmp;
	int ret;

	mode_cmd.width = sizes->surface_width;
	mode_cmd.height = sizes->surface_height;

	if (sizes->surface_bpp == 24)
		sizes->surface_bpp = 32;

	mode_cmd.pixel_format = drm_mode_legacy_fb_format(sizes->surface_bpp,
							  sizes->surface_depth);

	ret = gsgpufb_create_pinned_object(rfbdev, &mode_cmd, &gobj);
	if (ret) {
		DRM_ERROR("failed to create fbcon object %d\n", ret);
		return ret;
	}

	abo = gem_to_gsgpu_bo(gobj);

	/* okay we have an object now allocate the framebuffer */
	info = drm_fb_helper_alloc_info(helper);
	if (IS_ERR(info)) {
		ret = PTR_ERR(info);
		goto out;
	}

	info->par = rfbdev;
	info->skip_vt_switch = true;

	ret = gsgpu_display_framebuffer_init(adev->ddev, &rfbdev->rfb,
					     &mode_cmd, gobj);
	if (ret) {
		DRM_ERROR("failed to initialize framebuffer %d\n", ret);
		goto out;
	}

	fb = &rfbdev->rfb.base;

	/* setup helper */
	rfbdev->helper.fb = fb;

	strcpy(info->fix.id, "gsgpudrmfb");

	info->fbops = &gsgpufb_ops;

	/* not alloc from VRAM but GTT */
	tmp = gsgpu_bo_gpu_offset(abo) - adev->gmc.vram_start;
	info->fix.smem_start = adev->gmc.aper_base + tmp;
	info->fix.smem_len = gsgpu_bo_size(abo);
	info->screen_base = gsgpu_bo_kptr(abo);
	info->screen_size = gsgpu_bo_size(abo);

	drm_fb_helper_fill_info(info, &rfbdev->helper, sizes);

	/* Use default scratch pixmap (info->pixmap.flags = FB_PIXMAP_SYSTEM) */

	if (info->screen_base == NULL) {
		ret = -ENOSPC;
		goto out;
	}

	DRM_INFO("fb mappable at 0x%lX\n",  info->fix.smem_start);
	DRM_INFO("vram apper at 0x%lX\n",  (unsigned long)adev->gmc.aper_base);
	DRM_INFO("size %lu\n", (unsigned long)gsgpu_bo_size(abo));
	DRM_INFO("fb depth is %d\n", fb->format->depth);
	DRM_INFO("   pitch is %d\n", fb->pitches[0]);
	DRM_INFO("screen_base 0x%llX\n", (u64)info->screen_base);
	DRM_INFO("screen_size 0x%lX\n", info->screen_size);

	vga_switcheroo_client_fb_set(adev->pdev, info);
	return 0;

out:
	if (abo) {

	}
	if (fb && ret) {
		drm_gem_object_put(gobj);
		drm_framebuffer_unregister_private(fb);
		drm_framebuffer_cleanup(fb);
		kfree(fb);
	}
	return ret;
}

static int gsgpu_fbdev_destroy(struct drm_device *dev, struct gsgpu_fbdev *rfbdev)
{
	struct gsgpu_framebuffer *rfb = &rfbdev->rfb;

	drm_fb_helper_unregister_info(&rfbdev->helper);

	if (rfb->base.obj[0]) {
		gsgpufb_destroy_pinned_object(rfb->base.obj[0]);
		rfb->base.obj[0] = NULL;
		drm_framebuffer_unregister_private(&rfb->base);
		drm_framebuffer_cleanup(&rfb->base);
	}
	drm_fb_helper_fini(&rfbdev->helper);

	return 0;
}

static const struct drm_fb_helper_funcs gsgpu_fb_helper_funcs = {
	.fb_probe = gsgpufb_create,
};

int gsgpu_fbdev_init(struct gsgpu_device *adev)
{
	struct gsgpu_fbdev *rfbdev;
	int bpp_sel = 32;
	int ret;

	/* don't init fbdev on hw without DCE */
	if (!adev->mode_info.mode_config_initialized)
		return 0;

	/* don't init fbdev if there are no connectors */
	if (list_empty(&adev->ddev->mode_config.connector_list))
		return 0;

	/* select 8 bpp console on low vram cards */
	if (adev->gmc.real_vram_size <= (32*1024*1024))
		bpp_sel = 8;

	rfbdev = kzalloc(sizeof(struct gsgpu_fbdev), GFP_KERNEL);
	if (!rfbdev)
		return -ENOMEM;

	rfbdev->adev = adev;
	adev->mode_info.rfbdev = rfbdev;

	drm_fb_helper_prepare(adev->ddev, &rfbdev->helper, bpp_sel,
			      &gsgpu_fb_helper_funcs);

	ret = drm_fb_helper_init(adev->ddev, &rfbdev->helper);
	if (ret) {
		kfree(rfbdev);
		return ret;
	}

	drm_fb_helper_initial_config(&rfbdev->helper);

	return 0;
}

void gsgpu_fbdev_fini(struct gsgpu_device *adev)
{
	if (!adev->mode_info.rfbdev)
		return;

	gsgpu_fbdev_destroy(adev->ddev, adev->mode_info.rfbdev);
	kfree(adev->mode_info.rfbdev);
	adev->mode_info.rfbdev = NULL;
}

void gsgpu_fbdev_set_suspend(struct gsgpu_device *adev, int state)
{
	if (adev->mode_info.rfbdev)
		drm_fb_helper_set_suspend_unlocked(&adev->mode_info.rfbdev->helper,
						   state);
}

int gsgpu_fbdev_total_size(struct gsgpu_device *adev)
{
	struct gsgpu_bo *robj;
	int size = 0;

	if (!adev->mode_info.rfbdev)
		return 0;

	robj = gem_to_gsgpu_bo(adev->mode_info.rfbdev->rfb.base.obj[0]);
	size += gsgpu_bo_size(robj);
	return size;
}

bool gsgpu_fbdev_robj_is_fb(struct gsgpu_device *adev, struct gsgpu_bo *robj)
{
	if (!adev->mode_info.rfbdev)
		return false;
	if (robj == gem_to_gsgpu_bo(adev->mode_info.rfbdev->rfb.base.obj[0]))
		return true;
	return false;
}
