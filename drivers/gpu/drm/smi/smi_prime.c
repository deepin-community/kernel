/*
 * Copyright 2016 SiliconMotion Inc.
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License version 2. See the file COPYING in the main
 * directory of this archive for more details.
 *
 */

#include "smi_dbg.h"
#include "smi_drv.h"

#include <linux/dma-buf.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
#include <drm/drmP.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)

struct sg_table *smi_gem_prime_get_sg_table(struct drm_gem_object *obj)
{
	struct smi_bo *bo = gem_to_smi_bo(obj);
	int npages = bo->bo.num_pages;

	return drm_prime_pages_to_sg(bo->bo.ttm->pages, npages);
}


void *smi_gem_prime_vmap(struct drm_gem_object *obj)
{
	struct smi_bo *bo = gem_to_smi_bo(obj);
	int ret;
	void *base;

	ret = smi_bo_pin(bo, TTM_PL_FLAG_VRAM, NULL);
	if (ret)
		return (NULL);

	base = smi_bo_kmap(bo, true, NULL);
	if (IS_ERR(base)) {
		smi_bo_unpin(bo);
		return (NULL);
	}

	return (base);
}

void smi_gem_prime_vunmap(struct drm_gem_object *obj, void *vaddr)
{
	struct smi_bo *bo = gem_to_smi_bo(obj);

	smi_bo_kunmap(bo);
	smi_bo_unpin(bo);
}

int smi_gem_prime_pin(struct drm_gem_object *obj)
{
	struct smi_bo *bo = gem_to_smi_bo(obj);
	int ret = 0;
	ENTER();

	/* pin buffer into GTT */
	ret = smi_bo_pin(bo, TTM_PL_FLAG_SYSTEM, NULL);
	LEAVE(ret);
}

void smi_gem_prime_unpin(struct drm_gem_object *obj)
{
	struct smi_bo *bo = gem_to_smi_bo(obj);

	smi_bo_unpin(bo);
}
#endif

struct drm_gem_object *smi_gem_prime_import_sg_table(struct drm_device *dev,
						     struct dma_buf_attachment *attach,
						     struct sg_table *sg)
{
	struct smi_device *UNUSED(sdev) = dev->dev_private;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
	struct drm_gem_vram_object *gbo;
#else
	struct smi_bo *bo;
#endif
	int ret = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
	gbo = drm_gem_vram_create(dev, attach->dmabuf->size, PAGE_SIZE);
#else
	gbo = drm_gem_vram_create(dev, &dev->vram_mm->bdev, attach->dmabuf->size, PAGE_SIZE, false);
#endif
	if (IS_ERR(gbo)) {
		ret = PTR_ERR(gbo);
	}

#else
	struct reservation_object *resv = attach->dmabuf->resv;

	ww_mutex_lock(&resv->lock, NULL);
	ret = smi_bo_create(dev, attach->dmabuf->size, PAGE_SIZE, 0, sg, resv, &bo);
	ww_mutex_unlock(&resv->lock);
#endif
	if (ret) {
		return (ERR_PTR(ret));
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
	return (&gbo->bo.base);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
	return (&gbo->gem);
#else
	return (&bo->gem);
#endif
}
