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

#ifndef _H_GF_FENCE_H
#define _H_GF_FENCE_H

#include "gf_driver.h"

#if DRM_VERSION_CODE < KERNEL_VERSION(4,10,0)
#include <linux/fence.h>
#else
#include <linux/dma-fence.h>
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(5,4,0)
#include <linux/dma-resv.h>
#define reservation_object                   dma_resv
#define reservation_object_init              dma_resv_init
#define reservation_object_fini              dma_resv_fini
#define reservation_object_lock              dma_resv_lock
#define reservation_object_add_excl_fence    dma_resv_add_excl_fence
#define reservation_object_reserve_shared    dma_resv_reserve_shared
#define reservation_object_add_shared_fence  dma_resv_add_shared_fence
#define reservation_object_unlock            dma_resv_unlock
#define reservation_object_get_fences_rcu    dma_resv_get_fences_rcu
#define reservation_object_get_excl_rcu      dma_resv_get_excl_rcu
#else
#include <linux/reservation.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
#define reservation_object_lock(resv, a) ww_mutex_lock(&(resv)->lock, (a))
#define reservation_object_unlock(resv) ww_mutex_unlock(&(resv)->lock)
#endif

#if DRM_VERSION_CODE < KERNEL_VERSION(4,10,0)
typedef struct fence dma_fence_t;
typedef struct fence_ops dma_fence_ops_t;
typedef struct fence_cb dma_fence_cb_t;
#define dma_fence_init fence_init
#define dma_fence_get fence_get
#define dma_fence_put fence_put
#define dma_fence_wait fence_wait
#define dma_fence_signal fence_signal
#define dma_fence_default_wait fence_default_wait
#define dma_fence_context_alloc fence_context_alloc
#define dma_fence_is_signaled fence_is_signaled
#define dma_fence_wait_timeout fence_wait_timeout
#define dma_fence_add_callback fence_add_callback
#else
typedef struct dma_fence dma_fence_t;
typedef struct dma_fence_ops dma_fence_ops_t;
typedef struct dma_fence_cb dma_fence_cb_t;
#endif

#if defined PHYTIUM_2000
//nothing
#elif DRM_VERSION_CODE < KERNEL_VERSION(4,7,0) || (defined(YHQILIN) && (DRM_SUBVERSION_CODE < 20200710))
static inline dma_fence_t *reservation_object_get_excl_rcu(struct reservation_object *obj)
{
    dma_fence_t *fence;
    unsigned retry = 1;

    if (!rcu_access_pointer(obj->fence_excl))
        return NULL;

    while(retry)
    {
        unsigned seq = read_seqcount_begin(&obj->seq);
        rcu_read_lock();
        fence = rcu_dereference(obj->fence_excl);
        retry = read_seqcount_retry(&obj->seq, seq);
        if (retry)
            goto unlock;
        if (fence && !fence_get_rcu(fence))
        {
            retry = 1;
        }
unlock:
        rcu_read_unlock();
    }
    return fence;
}
#endif

struct gf_dma_fence_driver;

struct gf_dma_fence
{
    struct gf_dma_fence_driver *driver;
    dma_fence_t base;
    unsigned long long initialize_value;
    unsigned long long value;

    unsigned long long create_time;
    unsigned long long enqueue_time;
    struct list_head link;
};

#define to_gf_fence(fence) \
    container_of(fence, struct gf_dma_fence, base)

struct gf_dma_fence_context
{
    unsigned long long id;
    spinlock_t  lock;
    unsigned int sync_seq;
};

struct gf_dma_fence_driver
{
    void                    *adapter;
    void                    *os_thread;
    struct os_wait_event    *event;
    int                     timeout_msec;
    int                     can_freeze;
    spinlock_t              lock;
    struct list_head        fence_list;
    atomic_t                fence_count;
    unsigned long long      base_context_id;
    struct gf_dma_fence_context *context;
    struct kmem_cache       *fence_slab;
    atomic_t 	           fence_alloc_count;
};

#define gf_engine_by_fence(driver, fence) \
    ((fence)->context - (driver)->base_context_id)

#define gf_context_by_fence(driver, fence) \
    ((driver)->context + (fence)->context - (driver)->base_context_id)

#define gf_context_by_engine(driver, engine) \
    ((driver)->context + (engine))


int gf_dma_fence_driver_init(void *adapter, struct gf_dma_fence_driver *driver);
void gf_dma_fence_driver_fini(struct gf_dma_fence_driver *driver);
signed long gf_dma_fence_wait(dma_fence_t *base, bool intr, signed long timeout);
signed long gf_gem_fence_await_reservation(struct reservation_object *resv, int exclude_self, long timeout, int write);

void gf_dma_track_fences_dump(gf_card_t *card);
void gf_dma_track_fences_clear(gf_card_t *card);
#endif
