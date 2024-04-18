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
#include <drm/drm_gem_ttm_helper.h>
#include <drm/drm_prime.h>
#include <drm/drm_utils.h>
#include "mwv207_drm.h"
#include "mwv207_gem.h"
#include "mwv207_bo.h"
#include "mwv207_ttm.h"

static vm_fault_t mwv207_gem_fault(struct vm_fault *vmf)
{
	struct ttm_buffer_object *bo = vmf->vma->vm_private_data;
	vm_fault_t ret;

	ret = ttm_bo_vm_reserve(bo, vmf);
	if (ret)
		goto unlock_resv;

	ret = mwv207_bo_fault_reserve_notify(bo);
	if (ret)
		goto unlock_resv;

	ret = ttm_bo_vm_fault_reserved(vmf, vmf->vma->vm_page_prot, TTM_BO_VM_NUM_PREFAULT);
	if (ret == VM_FAULT_RETRY && !(vmf->flags & FAULT_FLAG_RETRY_NOWAIT))
		return ret;

unlock_resv:
	dma_resv_unlock(bo->base.resv);
	return ret;
}

static const struct vm_operations_struct mwv207_ttm_vm_ops = {
	.fault = mwv207_gem_fault,
	.open = ttm_bo_vm_open,
	.close = ttm_bo_vm_close,
	.access = ttm_bo_vm_access
};

int mwv207_gem_prime_pin(struct drm_gem_object *gem)
{
	struct mwv207_bo *jbo = mwv207_bo_from_gem(gem);
	int ret;

	ret = mwv207_bo_reserve(jbo, true);
	if (ret)
		return ret;
	ret = mwv207_bo_pin_reserved(jbo, 0x2);
	mwv207_bo_unreserve(jbo);
	return ret;
}

void mwv207_gem_prime_unpin(struct drm_gem_object *gem)
{
	struct mwv207_bo *jbo = mwv207_bo_from_gem(gem);
	int ret;

	ret = mwv207_bo_reserve(jbo, true);
	if (ret)
		return;
	ret = mwv207_bo_unpin_reserved(jbo);
	mwv207_bo_unreserve(jbo);
}

int mwv207_gem_prime_vmap(struct drm_gem_object *gobj, struct iosys_map *map)
{
	struct mwv207_bo *jbo = mwv207_bo_from_gem(gobj);
	int ret;

	if (jbo->kptr) {
		jbo->map_count++;
		goto out;
	}

	ret = mwv207_bo_pin_reserved(jbo, 0x2);
	if (unlikely(ret)) {
		drm_err(gobj->dev, "pin %p for vmap failed\n", jbo);
		return ret;
	}

	ret = ttm_bo_vmap(&jbo->tbo, &jbo->map);
	if (ret) {
		drm_err(gobj->dev, "ttm bo vmap failed\n");
		mwv207_bo_unpin_reserved(jbo);
		return ret;
	}
	jbo->map_count = 1;

out:
	*map = jbo->map;

	return 0;
}

void mwv207_gem_prime_vunmap(struct drm_gem_object *gobj, struct iosys_map *map)
{
	struct mwv207_bo *jbo = mwv207_bo_from_gem(gobj);

	if (unlikely(!jbo->map_count)) {
		drm_warn(gobj->dev, "%p is not mapped\n", jbo);
		return;
	}

	jbo->map_count--;
	if (jbo->map_count == 0) {
		ttm_bo_vunmap(&jbo->tbo, &jbo->map);

		mwv207_bo_unpin_reserved(jbo);
	}
}

const struct drm_gem_object_funcs mwv207_gem_object_funcs = {
	.free = mwv207_gem_free_object,
	.export = drm_gem_prime_export,
	.pin = mwv207_gem_prime_pin,
	.unpin = mwv207_gem_prime_unpin,
	.vmap = mwv207_gem_prime_vmap,
	.vunmap = mwv207_gem_prime_vunmap,
	.mmap = drm_gem_ttm_mmap,
	.vm_ops = &mwv207_ttm_vm_ops,
};

static int mwv207_gem_create(struct mwv207_device *jdev,
		u64 size, u64 align, u32 preferred_domain,
		u32 flags, struct drm_gem_object **gobj)
{
	struct mwv207_bo *jbo;
	int ret;

	*gobj = NULL;
retry:

	ret = mwv207_bo_create(jdev, size, align, ttm_bo_type_device,
			preferred_domain, flags, &jbo);
	if (ret) {
		if (ret != -ERESTARTSYS) {
			if (flags & (1<<0)) {
				flags &= ~(1<<0);
				goto retry;
			}
			if (preferred_domain == 0x2) {
				preferred_domain |= 0x1;
				goto retry;
			}
			DRM_DEBUG("Failed to allocate GEM object (%lld, %d, %llu, %d)\n",
				  size, preferred_domain, align, ret);
		}
		return ret;
	}

	*gobj = &jbo->tbo.base;
	(*gobj)->funcs = &mwv207_gem_object_funcs;

	return 0;
}

int mwv207_gem_dumb_create(struct drm_file *file, struct drm_device *dev,
		     struct drm_mode_create_dumb *args)
{
	struct mwv207_device *jdev = dev->dev_private;
	struct drm_gem_object *gobj;
	u32 handle;
	int ret;

	args->pitch = ALIGN(args->width * DIV_ROUND_UP(args->bpp, 8), 64);
	args->size = args->pitch * args->height;
	ret = mwv207_gem_create(jdev, args->size, 0x10000,
			0x2,
			(1<<0),
			&gobj);
	if (ret)
		return ret;

	ret = drm_gem_handle_create(file, gobj, &handle);
	mwv207_gem_object_put(gobj);
	if (ret)
		return ret;

	args->handle = handle;
	return 0;
}

void mwv207_gem_free_object(struct drm_gem_object *gobj)
{
	struct mwv207_bo *jbo;

	if (!gobj)
		return;

	jbo = mwv207_bo_from_gem(gobj);
	mwv207_bo_unref(jbo);
}

int  mwv207_gem_create_ioctl(struct drm_device *dev, void *data,
			    struct drm_file *filp)
{
	struct mwv207_device *jdev = dev->dev_private;
	union drm_mwv207_gem_create *args = data;
	struct drm_gem_object *gobj;
	int ret;

	if (args->in.size == 0)
		return -EINVAL;
	if (args->in.alignment & (args->in.alignment - 1))
		return -EINVAL;
	if (args->in.preferred_domain & ~0x7)
		return -EINVAL;
	if (args->in.flags & ~((1<<0)|(1<<1)|(1<<2)))
		return -EINVAL;

	ret = mwv207_gem_create(jdev, args->in.size, args->in.alignment,
			args->in.preferred_domain,
			args->in.flags, &gobj);
	if (ret)
		return ret;

	ret = drm_gem_handle_create(filp, gobj, &args->out.handle);
	mwv207_gem_object_put(gobj);

	return ret;
}

int  mwv207_gem_mmap_ioctl(struct drm_device *dev, void *data,
			    struct drm_file *filp)
{
	union drm_mwv207_gem_mmap *args = data;
	struct drm_gem_object *obj;

	if (args->in.pad)
		return -EINVAL;

	obj = drm_gem_object_lookup(filp, args->in.handle);
	if (!obj)
		return -ENOENT;
	args->out.offset = mwv207_bo_mmap_offset(obj);
	mwv207_gem_object_put(obj);

	return 0;
}

int  mwv207_gem_wait_ioctl(struct drm_device *dev, void *data,
			    struct drm_file *filp)
{
	struct drm_mwv207_gem_wait *args = (struct drm_mwv207_gem_wait *)data;
	long timeout;
	bool write;
	int ret;

	if (args->op & ~(0x00000002 | 0x00000001))
		return -EINVAL;

	write = args->op & 0x00000002;
	timeout = drm_timeout_abs_to_jiffies(args->timeout);

	ret = drm_gem_dma_resv_wait(filp, args->handle, write, timeout);
	if (ret == -ETIME)
		ret = timeout ? -ETIMEDOUT : -EBUSY;

	return ret;
}

