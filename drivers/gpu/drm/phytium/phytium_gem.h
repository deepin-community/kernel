/* SPDX-License-Identifier: GPL-2.0 */
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

#ifndef __PHYTIUM_GEM_H__
#define __PHYTIUM_GEM_H__

#include <drm/drm_gem.h>

struct phytium_gem_object {
	struct drm_gem_object base;
	dma_addr_t iova;
	void *vaddr;
	unsigned long size;
	struct sg_table *sgt;
	unsigned int tiling;
	bool is_vram;
	bool reserve[3];
	struct list_head list;
	void *vaddr_save;
};

#define	to_phytium_gem_obj(obj)		container_of(obj, struct phytium_gem_object, base)

int phytium_gem_mmap_obj(struct drm_gem_object *obj, struct vm_area_struct *vma);
int phytium_gem_mmap(struct file *filp, struct vm_area_struct *vma);
void phytium_gem_free_object(struct drm_gem_object *obj);
struct sg_table *phytium_gem_prime_get_sg_table(struct drm_gem_object *obj);
struct drm_gem_object *phytium_gem_prime_import_sg_table(struct drm_device *dev,
					struct dma_buf_attachment *attach, struct sg_table *sgt);
void phytium_gem_free_object(struct drm_gem_object *obj);
int phytium_gem_dumb_destroy(struct drm_file *file, struct drm_device *dev, unsigned int handle);
struct phytium_gem_object *phytium_gem_create_object(struct drm_device *dev, unsigned long size);
int phytium_gem_dumb_create(struct drm_file *file, struct drm_device *dev,
				     struct drm_mode_create_dumb *args);
void *phytium_gem_prime_vmap(struct drm_gem_object *obj);
void phytium_gem_prime_vunmap(struct drm_gem_object *obj, void *vaddr);
int phytium_gem_prime_mmap(struct drm_gem_object *obj, struct vm_area_struct *vma);
int phytium_gem_suspend(struct drm_device *drm_dev);
void phytium_gem_resume(struct drm_device *drm_dev);
#endif /* __PHYTIUM_GEM_H__ */
