//*****************************************************************************
//  Copyright (c) 2021 Glenfly Tech Co., Ltd.. 
//  All Rights Reserved.
//
//  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Glenfly Tech Co., Ltd..;
//  the contents of this file may not be disclosed to third parties, copied or
//  duplicated in any form, in whole or in part, without the prior written
//  permission of Glenfly Tech Co., Ltd..
//
//  The copyright of the source code is protected by the copyright laws of the People's
//  Republic of China and the related laws promulgated by the People's Republic of China
//  and the international covenant(s) ratified by the People's Republic of China. 
//*****************************************************************************

#ifndef _H_GF_GEM_OBJECT_H
#define _H_GF_GEM_OBJECT_H
#if DRM_VERSION_CODE >= KERNEL_VERSION(5,5,0)
#include <drm/drm_drv.h>
#else
#include <drm/drmP.h>
#endif
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
#include "gf_gem_debug.h"
#include "gf_version.h"

struct drm_gf_gem_object;

struct drm_gf_gem_object_ops
{
    int (*get_pages)(struct drm_gf_gem_object *);
    void (*put_pages)(struct drm_gf_gem_object *, struct sg_table *);
    void (*release)(struct drm_gf_gem_object *);
};

struct drm_gf_gem_object
{
    struct drm_gem_object base;
    const struct drm_gf_gem_object_ops *ops;

    unsigned int core_handle;
    struct
    {
        struct mutex lock;
        int pages_pin_count;
        struct sg_table *pages;
    } mm;

    gf_open_allocation_t info;
    gf_map_argu_t map_argu;
    gf_vm_area_t *krnl_vma;
    struct reservation_object *resv;
    struct reservation_object __builtin_resv;

    unsigned int prefault_num;
    unsigned int delay_map;

    unsigned int vmap_mem_type;
//debugfs related things
    gf_gem_debug_info_t  debug;
};


static inline struct drm_gf_gem_object* gf_gem_object_get(struct drm_gf_gem_object* obj)
{
#if DRM_VERSION_CODE < KERNEL_VERSION(4,12,0)
    drm_gem_object_reference(&obj->base);
#else
    drm_gem_object_get(&obj->base);
#endif
    return obj;
}

static inline void gf_gem_object_put(struct drm_gf_gem_object* obj)
{
#if DRM_VERSION_CODE < KERNEL_VERSION(4,12,0)
    drm_gem_object_unreference_unlocked(&obj->base);
#elif DRM_VERSION_CODE < KERNEL_VERSION(5,9,0)
     drm_gem_object_put_unlocked(&obj->base);
#else
    drm_gem_object_put(&obj->base);
#endif
}

#undef __CONCAT
#undef CONCAT
#define __CONCAT(x,y) x##y
#define CONCAT(x,y)     __CONCAT(x,y)

extern struct drm_gf_gem_object* CONCAT(gf_krnl_gem_create_object, DRIVER_NAME)(struct pci_dev *dev,
        gf_create_allocation_t *create, gf_device_debug_info_t **ddev);
        

struct gf_krnl_device_create {
    unsigned int      gpu_device;  //out, virtual device handle
    gf_device_debug_info_t *debug; //out, for debugfs
};

extern int CONCAT(gf_krnl_create_device, DRIVER_NAME)(struct pci_dev *dev,
        struct gf_krnl_device_create *vdev_create);

struct gf_krnl_device_destroy {
    unsigned int      gpu_device;  //in, virtual device handle
    gf_device_debug_info_t *debug; //in, for debugfs
};

extern void CONCAT(gf_krnl_destroy_device,DRIVER_NAME)(struct pci_dev *dev,
        struct gf_krnl_device_destroy *destroy);

extern const struct dma_buf_ops CONCAT(gf_dmabuf_ops,DRIVER_NAME);
#endif

