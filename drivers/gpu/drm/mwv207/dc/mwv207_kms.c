/*
* SPDX-License-Identifier: GPL
*
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
#include <drm/drm_print.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_vblank.h>
#include <drm/drm_atomic.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_probe_helper.h>
#include "mwv207_gem.h"
#include "mwv207_kms.h"
#include "mwv207_va.h"
#include "mwv207_vi.h"

static const struct drm_framebuffer_funcs fb_funcs = {
	.destroy = drm_gem_fb_destroy,
	.create_handle = drm_gem_fb_create_handle,
};

int mwv207_framebuffer_init(struct mwv207_device *jdev, struct drm_framebuffer *fb,
		const struct drm_mode_fb_cmd2 *mode_cmd, struct drm_gem_object *gobj)
{
	int ret;

	fb->obj[0] = gobj;
	drm_helper_mode_fill_fb_struct(&jdev->base, fb, mode_cmd);

	ret = drm_framebuffer_init(&jdev->base, fb, &fb_funcs);
	return ret;
}

static struct drm_framebuffer *mwv207_framebuffer_create(
		struct drm_device *dev,
		struct drm_file *filp,
		const struct drm_mode_fb_cmd2 *mode_cmd)
{
	struct drm_gem_object *gobj = drm_gem_object_lookup(filp, mode_cmd->handles[0]);
	struct drm_framebuffer *fb;
	int err = -ENOENT;

	if (!gobj) {
		DRM_DEBUG_DRIVER("fbdev failed to lookup gem object, handle = %d\n",
				mode_cmd->handles[0]);
		return ERR_PTR(-ENOENT);
	}

	fb = kzalloc(sizeof(*fb), GFP_KERNEL);
	if (!fb) {
		err = -ENOMEM;
		goto err_unref;
	}

	err = mwv207_framebuffer_init(dev->dev_private, fb, mode_cmd, gobj);
	if (err)
		goto err_free;

	return fb;
err_free:
	kfree(fb);
err_unref:
	mwv207_gem_object_put(gobj);
	return ERR_PTR(err);
}

static void mwv207_atomic_prepare_vblank(struct drm_device *dev,
		struct drm_atomic_state *old_state)
{
	struct drm_crtc_state *new_crtc_state;
	struct drm_crtc *crtc;
	int i;

	for_each_new_crtc_in_state(old_state, crtc, new_crtc_state, i)
		mwv207_crtc_prepare_vblank(crtc);
}

static void mwv207_atomic_commit_tail(struct drm_atomic_state *old_state)
{
	struct drm_device *dev = old_state->dev;

	drm_atomic_helper_commit_modeset_disables(dev, old_state);
	drm_atomic_helper_commit_planes(dev, old_state, 0);
	drm_atomic_helper_commit_modeset_enables(dev, old_state);

	mwv207_atomic_prepare_vblank(dev, old_state);

	drm_atomic_helper_commit_hw_done(old_state);

	drm_atomic_helper_wait_for_vblanks(dev, old_state);
	drm_atomic_helper_cleanup_planes(dev, old_state);
}

static struct drm_mode_config_helper_funcs mwv207_mode_config_helper_funcs = {
	.atomic_commit_tail = mwv207_atomic_commit_tail,
};

static const struct drm_mode_config_funcs mwv207_mode_config_funcs = {
	.fb_create     = mwv207_framebuffer_create,
	.atomic_check  = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
	.output_poll_changed = drm_fb_helper_output_poll_changed,
};

static void mwv207_modeset_init(struct drm_device *dev)
{
	drm_mode_config_init(dev);

	dev->mode_config.min_width = 20;
	dev->mode_config.min_height = 20;

	dev->mode_config.max_width = 4096 * 4;
	dev->mode_config.max_height = 4096 * 4;

	dev->mode_config.preferred_depth = 24;
	dev->mode_config.prefer_shadow = 1;
	dev->mode_config.fb_modifiers_not_supported = false;

	dev->mode_config.async_page_flip = true;

	dev->mode_config.funcs = &mwv207_mode_config_funcs;
	dev->mode_config.helper_private = &mwv207_mode_config_helper_funcs;

	dev->mode_config.cursor_width = 64;
	dev->mode_config.cursor_height = 64;
}

int mwv207_kms_suspend(struct mwv207_device *jdev)
{
	return drm_mode_config_helper_suspend(&jdev->base);
}

int mwv207_kms_resume(struct mwv207_device *jdev)
{
	int ret;

	ret = drm_mode_config_helper_resume(&jdev->base);
	if (ret)
		pr_warn("mwv207: failed to resume kms, ret: %d", ret);

	return ret;
}

int mwv207_kms_init(struct mwv207_device *jdev)
{
	int ret;

	mwv207_modeset_init(&jdev->base);

	ret = mwv207_va_init(jdev);
	if (ret)
		goto cleanup;

	ret = mwv207_vi_init(jdev);
	if (ret)
		goto cleanup;

	jdev->va_irq = irq_find_mapping(jdev->irq_domain, 42);
	BUG_ON(jdev->va_irq == 0);
	ret = request_irq(jdev->va_irq, mwv207_va_handle_vblank, 0, "mwv207_va", jdev);
	if (ret)
		goto cleanup;

	ret = drm_vblank_init(&jdev->base, jdev->base.mode_config.num_crtc);
	if (ret)
		goto free_irq;

	drm_mode_config_reset(&jdev->base);

	drm_kms_helper_poll_init(&jdev->base);

	DRM_INFO("number of crtc:      %d\n", jdev->base.mode_config.num_crtc);
	DRM_INFO("number of encoder:   %d\n", jdev->base.mode_config.num_encoder);
	DRM_INFO("number of connector: %d\n", jdev->base.mode_config.num_connector);

	return 0;
free_irq:
	free_irq(jdev->va_irq, jdev);
cleanup:
	drm_mode_config_cleanup(&jdev->base);
	return ret;
}

void mwv207_kms_fini(struct mwv207_device *jdev)
{
	drm_kms_helper_poll_fini(&jdev->base);
	drm_atomic_helper_shutdown(&jdev->base);
	free_irq(jdev->va_irq, jdev);
	drm_mode_config_cleanup(&jdev->base);
}
