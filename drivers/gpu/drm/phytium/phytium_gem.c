// SPDX-License-Identifier: GPL-2.0
/* Phytium X100 display drm driver
 *
 * Copyright (c) 2021 Phytium Limited.
 *
 * Author:
 *	Yang Xun <yangxun@phytium.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/dma-buf.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include "phytium_display_drv.h"
#include "phytium_reg.h"
#include "phytium_gem.h"

struct sg_table *
phytium_gem_prime_get_sg_table(struct drm_gem_object *obj)
{
	struct phytium_gem_object *phytium_gem_obj = to_phytium_gem_obj(obj);
	struct sg_table *sgt;
	struct drm_device *dev = obj->dev;
	int ret;

	sgt = kzalloc(sizeof(*sgt), GFP_KERNEL);
	if (IS_ERR(sgt)) {
		DRM_DEBUG_KMS("malloc sgt fail\n");
		return ERR_PTR(-ENOMEM);
	}

	ret = dma_get_sgtable_attrs(dev->dev, sgt, phytium_gem_obj->vaddr,
				    phytium_gem_obj->iova, phytium_gem_obj->size,
				    DMA_ATTR_WRITE_COMBINE);
	if (ret) {
		DRM_ERROR("failed to allocate sgt, %d\n", ret);
		kfree(sgt);
		return ERR_PTR(ret);
	}

	return sgt;
}

struct drm_gem_object *
phytium_gem_prime_import_sg_table(struct drm_device *dev,
					       struct dma_buf_attachment *attach,
					       struct sg_table *sgt)
{
	struct phytium_gem_object *phytium_gem_obj = NULL;
	struct scatterlist *s;
	dma_addr_t expected;
	int ret, i;

	phytium_gem_obj = kzalloc(sizeof(*phytium_gem_obj), GFP_KERNEL);
	if (IS_ERR(phytium_gem_obj)) {
		DRM_ERROR("failed to allocate phytium_gem_obj\n");
		ret = -ENOMEM;
		goto failed_malloc;
	}

	ret = drm_gem_object_init(dev, &phytium_gem_obj->base, attach->dmabuf->size);
	if (ret) {
		DRM_ERROR("failed to initialize drm gem object: %d\n", ret);
		goto failed_object_init;
	}

	expected = sg_dma_address(sgt->sgl);
	for_each_sg(sgt->sgl, s, sgt->nents, i) {
		if (sg_dma_address(s) != expected) {
			DRM_ERROR("sg_table is not contiguous");
			ret = -EINVAL;
			goto failed_check_continue;
		}
		expected = sg_dma_address(s) + sg_dma_len(s);
	}

	phytium_gem_obj->iova = sg_dma_address(sgt->sgl);
	phytium_gem_obj->sgt = sgt;

	return &phytium_gem_obj->base;
failed_check_continue:
	drm_gem_object_release(&phytium_gem_obj->base);
failed_object_init:
	kfree(phytium_gem_obj);
failed_malloc:
	return ERR_PTR(ret);
}

void *phytium_gem_prime_vmap(struct drm_gem_object *obj)
{
	struct phytium_gem_object *phytium_obj = to_phytium_gem_obj(obj);

	return phytium_obj->vaddr;
}

void phytium_gem_prime_vunmap(struct drm_gem_object *obj, void *vaddr)
{
}

int phytium_gem_prime_mmap(struct drm_gem_object *obj, struct vm_area_struct *vma)
{
	int ret = 0;

	ret = drm_gem_mmap_obj(obj, obj->size, vma);
	if (ret < 0)
		return ret;

	return phytium_gem_mmap_obj(obj, vma);
}

int phytium_gem_suspend(struct drm_device *drm_dev)
{
	struct phytium_display_drm_private *priv = drm_dev->dev_private;
	struct phytium_gem_object *phytium_gem_obj = NULL;

	list_for_each_entry(phytium_gem_obj, &priv->gem_list_head, list) {
		if (!phytium_gem_obj->is_vram)
			continue;

		phytium_gem_obj->vaddr_save = vmalloc(phytium_gem_obj->size);
		if (phytium_gem_obj->vaddr_save == NULL)
			goto malloc_failed;

		memcpy(phytium_gem_obj->vaddr_save, phytium_gem_obj->vaddr, phytium_gem_obj->size);
	}

	return 0;
malloc_failed:
	list_for_each_entry(phytium_gem_obj, &priv->gem_list_head, list) {
		if (!phytium_gem_obj->is_vram)
			continue;

		if (phytium_gem_obj->vaddr_save) {
			vfree(phytium_gem_obj->vaddr_save);
			phytium_gem_obj->vaddr_save = NULL;
		}
	}
	return -ENOMEM;
}

void phytium_gem_resume(struct drm_device *drm_dev)
{
	struct phytium_display_drm_private *priv = drm_dev->dev_private;
	struct phytium_gem_object *phytium_gem_obj = NULL;

	list_for_each_entry(phytium_gem_obj, &priv->gem_list_head, list) {
		if (!phytium_gem_obj->is_vram)
			continue;

		memcpy(phytium_gem_obj->vaddr, phytium_gem_obj->vaddr_save, phytium_gem_obj->size);
		vfree(phytium_gem_obj->vaddr_save);
		phytium_gem_obj->vaddr_save = NULL;
	}
}

void phytium_gem_free_object(struct drm_gem_object *obj)
{
	struct phytium_gem_object *phytium_gem_obj = to_phytium_gem_obj(obj);
	struct drm_device *dev = obj->dev;

	DRM_DEBUG_KMS("free phytium_gem_obj iova:0x%pa size:0x%lx\n",
		      &phytium_gem_obj->iova, phytium_gem_obj->size);
	if (phytium_gem_obj->vaddr) {
		dma_free_attrs(dev->dev, phytium_gem_obj->size, phytium_gem_obj->vaddr,
			       phytium_gem_obj->iova, 0);
		list_del(&phytium_gem_obj->list);
	}
	else if (obj->import_attach)
		drm_prime_gem_destroy(obj, phytium_gem_obj->sgt);
	drm_gem_object_release(obj);
	kfree(phytium_gem_obj);
}

int phytium_gem_mmap_obj(struct drm_gem_object *obj, struct vm_area_struct *vma)
{
	int ret = 0;
	struct phytium_gem_object *phytium_gem_obj = to_phytium_gem_obj(obj);

	/*
	 * Clear the VM_PFNMAP flag that was set by drm_gem_mmap(), and set the
	 * vm_pgoff (used as a fake buffer offset by DRM) to 0 as we want to map
	 * the whole buffer.
	 */
	vma->vm_flags &= ~VM_PFNMAP;
	vma->vm_pgoff = 0;
	vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);

	if (phytium_gem_obj->is_vram)
		ret = dma_mmap_attrs(obj->dev->dev, vma, phytium_gem_obj->vaddr,
				     phytium_gem_obj->iova, vma->vm_end - vma->vm_start,
				     DMA_ATTR_WRITE_COMBINE);
	else
		ret = dma_mmap_attrs(obj->dev->dev, vma, phytium_gem_obj->vaddr,
				     phytium_gem_obj->iova, vma->vm_end - vma->vm_start, 0);
	if (ret)
		drm_gem_vm_close(vma);

	return ret;
}

int phytium_gem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret = 0;

	ret = drm_gem_mmap(filp, vma);
	if (ret < 0)
		return ret;

	return phytium_gem_mmap_obj(vma->vm_private_data, vma);
}

int phytium_gem_dumb_destroy(struct drm_file *file, struct drm_device *dev, uint32_t handle)
{
	return drm_gem_dumb_destroy(file, dev, handle);
}

struct phytium_gem_object *phytium_gem_create_object(struct drm_device *dev, unsigned long size)
{
	struct phytium_gem_object *phytium_gem_obj = NULL;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int ret = 0;

	phytium_gem_obj = kzalloc(sizeof(*phytium_gem_obj), GFP_KERNEL);
	if (IS_ERR(phytium_gem_obj)) {
		DRM_ERROR("failed to allocate phytium_gem_obj\n");
		ret = -ENOMEM;
		goto error;
	}

	ret = drm_gem_object_init(dev, &phytium_gem_obj->base, size);
	if (ret) {
		DRM_ERROR("failed to initialize drm gem object: %d\n", ret);
		goto failed_object_init;
	}

	phytium_gem_obj->vaddr = dma_alloc_attrs(dev->dev, size, &phytium_gem_obj->iova,
						 GFP_KERNEL, 0);
	if (phytium_gem_obj->vaddr == NULL) {
		DRM_ERROR("fail to allocate buffer with size %lx\n", size);
		ret = -ENOMEM;
		goto failed_dma_alloc;
	}

	phytium_gem_obj->size = size;
	phytium_gem_obj->is_vram = priv->vram_support;
	list_add_tail(&phytium_gem_obj->list, &priv->gem_list_head);
	DRM_DEBUG_KMS("phytium_gem_obj iova:0x%pa size:0x%lx\n",
		       &phytium_gem_obj->iova, phytium_gem_obj->size);
	return phytium_gem_obj;
failed_dma_alloc:
	drm_gem_object_unreference_unlocked(&phytium_gem_obj->base);
	return ERR_PTR(ret);
failed_object_init:
	kfree(phytium_gem_obj);
error:
	return ERR_PTR(ret);
}

int phytium_gem_dumb_create(struct drm_file *file, struct drm_device *dev,
				     struct drm_mode_create_dumb *args)
{
	int size = 0;
	struct phytium_gem_object *phytium_gem_obj = NULL;
	int ret = 0;

	args->pitch = ALIGN(args->width*DIV_ROUND_UP(args->bpp, 8), 128);
	args->size = args->pitch * args->height;
	size = PAGE_ALIGN(args->size);
	phytium_gem_obj = phytium_gem_create_object(dev, size);
	if (IS_ERR(phytium_gem_obj))
		return PTR_ERR(phytium_gem_obj);
	ret = drm_gem_handle_create(file, &phytium_gem_obj->base, &args->handle);
	if (ret) {
		DRM_ERROR("failed to drm_gem_handle_create\n");
		goto failed_gem_handle;
	}
	drm_gem_object_unreference_unlocked(&phytium_gem_obj->base);

	return 0;
failed_gem_handle:
	phytium_gem_free_object(&phytium_gem_obj->base);
	return ret;
}
