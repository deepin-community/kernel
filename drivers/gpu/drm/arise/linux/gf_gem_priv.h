/*
 * Copyright © 2021 Glenfly Tech Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */
#ifndef __GF_GEM_PRIV_H__
#define __GF_GEM_PRIV_H__
#include <drm/drm_vma_manager.h>
#include <drm/drm_gem.h>
#if DRM_VERSION_CODE >= KERNEL_VERSION(5,4,0)
#include <linux/dma-resv.h>
#define reservation_object  dma_resv
#else
#include <linux/reservation.h>
#endif
#include <linux/dma-buf.h>
#include "gf_types.h"
#include "gf_driver.h"
#include "kernel_import.h"
#include "os_interface.h"
#include "gf_gem_debug.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0) || \
    DRM_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)) && !defined(YHQILIN)
#include <linux/pfn_t.h>
#else
typedef struct {
    u64 val;
} pfn_t;
#endif


#define to_gf_bo(gem) container_of(gem, struct drm_gf_gem_object, base)
#define dmabuf_to_gf_bo(dmabuf) to_gf_bo((struct drm_gem_object*)((dmabuf)->priv))

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0)
#define bo_resv(bo)  ((bo)->resv)
#else
#define bo_resv(bo)  ((bo)->base.resv)
#endif

static inline unsigned int gf_gem_get_core_handle(gf_file_t *file, unsigned int handle)
{
    struct drm_gem_object *gem;
    struct drm_file *drm_file = file->parent_file;

    spin_lock(&drm_file->table_lock);
    gem = idr_find(&drm_file->object_idr, handle);
    spin_unlock(&drm_file->table_lock);

    if (!gem)
        return 0;

    return to_gf_bo(gem)->core_handle;
}

static inline struct drm_gf_gem_object *gf_gem_get_object(gf_file_t *file, unsigned int handle)
{
    struct drm_gem_object *gem;
    struct drm_file *drm_file = file->parent_file;

    spin_lock(&drm_file->table_lock);
    gem = idr_find(&drm_file->object_idr, handle);
    spin_unlock(&drm_file->table_lock);

    if (!gem)
        return NULL;

    return to_gf_bo(gem);
}

struct drm_gf_driver
{
    struct mutex lock;
    struct drm_file *file_priv;
    struct drm_driver base;
};
#define to_drm_gf_driver(drm_driver) container_of((drm_driver), struct drm_gf_driver, base)

extern struct drm_gf_gem_object* gf_drm_gem_create_object(gf_card_t *gf, gf_create_allocation_t *create, gf_device_debug_info_t **ddev);

extern int gf_drm_gem_create_object_ioctl(struct drm_file *file, gf_create_allocation_t *create);
extern int gf_drm_gem_create_resource_ioctl(struct drm_file *file, gf_create_resource_t *create);

extern int gf_gem_mmap_gtt(struct drm_file *file, struct drm_device *dev, uint32_t handle, uint64_t *offset);
extern int gf_gem_mmap_gtt_ioctl(struct drm_file *file, gf_drm_gem_map_t *args);
extern int gf_drm_gem_mmap(struct file *filp, struct vm_area_struct *vma);

extern void gf_gem_free_object(struct drm_gem_object *gem_obj);
extern int gf_gem_open(struct drm_gem_object *gem_obj, struct drm_file *file);
#if DRM_VERSION_CODE < KERNEL_VERSION(6,6,0)
extern int gf_gem_prime_fd_to_handle(struct drm_device *dev, struct drm_file *file_priv, int prime_fd, uint32_t *handle);
#endif

extern struct drm_gem_object *gf_gem_prime_import(struct drm_device *dev, struct dma_buf *dma_buf);

#if DRM_VERSION_CODE >= KERNEL_VERSION(5,4,0)
extern struct dma_buf *gf_gem_prime_export(struct drm_gem_object *gem_obj, int flags);
#else
extern struct dma_buf *gf_gem_prime_export(struct drm_device *dev, struct drm_gem_object *gem_obj, int flags);
#endif

#if DRM_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
extern void *gf_gem_prime_vmap(struct drm_gem_object *gem_obj);
extern void gf_gem_prime_vunmap(struct drm_gem_object *gem_obj, void *vaddr);
#endif

extern int gf_gem_dumb_create(struct drm_file *file, struct drm_device *dev, struct drm_mode_create_dumb *args);

extern signed long gf_gem_object_begin_cpu_access(struct drm_gf_gem_object *allocation, long timeout, int write);
extern void gf_gem_object_end_cpu_access(struct drm_gf_gem_object *allocation, int write);
extern int gf_gem_object_begin_cpu_access_ioctl(struct drm_file *file, gf_drm_gem_begin_cpu_access_t *args);
extern void gf_gem_object_end_cpu_access_ioctl(struct drm_file *file, gf_drm_gem_end_cpu_access_t *args);

extern unsigned int gf_get_from_gem_handle(void *file_, unsigned int handle);
extern void* gf_gem_object_vmap(struct drm_gf_gem_object *obj);
extern void gf_gem_object_vunmap(struct drm_gf_gem_object *obj);

static inline struct drm_gf_gem_object *
gf_drm_gem_object_lookup(struct drm_device *dev, struct drm_file *filp,
                      u32 handle)
{
    struct drm_gem_object *obj;
#if DRM_VERSION_CODE < KERNEL_VERSION(4,7,0) && !defined (PHYTIUM_2000)
    obj = drm_gem_object_lookup(dev, filp, handle);
#else
    obj = drm_gem_object_lookup(filp, handle);
#endif

    if (!obj)
        return NULL;
    return to_gf_bo(obj);
}

extern const struct vm_operations_struct gf_gem_vm_ops;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 17, 0)
typedef int vm_fault_t;

static inline vm_fault_t vmf_insert_mixed(struct vm_area_struct *vma,
                unsigned long addr,
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 5, 0) && !(defined(YHQILIN) && DRM_VERSION_CODE == KERNEL_VERSION(4,9,0))
                pfn_t pfn)
#else
                unsigned long pfn)
#endif
{
    int err = vm_insert_mixed(vma, addr, pfn);

    if (err == -ENOMEM)
        return VM_FAULT_OOM;
    if (err < 0 && err != -EBUSY)
        return VM_FAULT_SIGBUS;

    return VM_FAULT_NOPAGE;
}

static inline vm_fault_t vmf_insert_pfn(struct vm_area_struct *vma,
                unsigned long addr, unsigned long pfn)
{
    int err = vm_insert_pfn(vma, addr, pfn);

    if (err == -ENOMEM)
        return VM_FAULT_OOM;
    if (err < 0 && err != -EBUSY)
        return VM_FAULT_SIGBUS;

    return VM_FAULT_NOPAGE;
}

#endif
#endif
