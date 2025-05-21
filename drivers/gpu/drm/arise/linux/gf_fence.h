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
#ifndef _H_GF_FENCE_H
#define _H_GF_FENCE_H

#include "gf_driver.h"

#if DRM_VERSION_CODE < KERNEL_VERSION(4,10,0)
#include <linux/fence.h>
#else
#include <linux/dma-fence.h>
#endif


#if DRM_VERSION_CODE < KERNEL_VERSION(4,11,0)
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

#if DRM_VERSION_CODE >= KERNEL_VERSION(5,4,0)
#include <linux/dma-resv.h>
#define reservation_object                   dma_resv
#define reservation_object_init              dma_resv_init
#define reservation_object_fini              dma_resv_fini
#define reservation_object_lock              dma_resv_lock
#define reservation_object_unlock            dma_resv_unlock

#if DRM_VERSION_CODE < KERNEL_VERSION(5,19,0)
#define reservation_object_reserve_shared    dma_resv_reserve_shared
#define reservation_object_add_excl_fence    dma_resv_add_excl_fence
#define reservation_object_add_shared_fence  dma_resv_add_shared_fence
#else
#define reservation_object_reserve_shared    dma_resv_reserve_fences
static inline void reservation_object_add_excl_fence(struct reservation_object *obj, dma_fence_t *fence)
{
    dma_resv_add_fence(obj, fence, DMA_RESV_USAGE_WRITE);
}
static inline void reservation_object_add_shared_fence(struct reservation_object *obj, dma_fence_t *fence)
{
    dma_resv_add_fence(obj, fence, DMA_RESV_USAGE_READ);
}
#endif

// __reservation_object_get_fences_rcu should be a help function just in this file, shouldn't call it anywhere else.
#if DRM_VERSION_CODE >= KERNEL_VERSION(5,14,0)
#define __reservation_object_get_fences_rcu  dma_resv_get_fences
#if DRM_VERSION_CODE < KERNEL_VERSION(5,17,0)
#define gf_reservation_object_get_excl_rcu    dma_resv_get_excl_unlocked
#elif DRM_VERSION_CODE >= KERNEL_VERSION(5,19,0)
static inline dma_fence_t* gf_reservation_object_get_excl_rcu(struct reservation_object *obj)
{
    dma_fence_t **fences = NULL;
    dma_fence_t *fence = NULL;
    int count, i, ret;
    int max_index = 0;

    ret = dma_resv_get_fences(obj, DMA_RESV_USAGE_WRITE, &count, &fences);
    if (ret != 0 || count < 1)
        return NULL;

    for (i = 1; i < count; i++)
    {
        if(fences[max_index]->seqno < fences[i]->seqno)
        {
            dma_fence_put(fences[max_index]);
            max_index = i;
        }
        else
            dma_fence_put(fences[i]);
    }

    return fences[max_index];
}
#else // DRM_VERSION_CODE < KERNEL_VERSION(5,17,0)
static inline dma_fence_t* gf_reservation_object_get_excl_rcu(struct reservation_object *obj)
{
    dma_fence_t *fence;
    rcu_read_lock();
    fence = dma_resv_excl_fence(obj);
    if (fence)
        dma_fence_get(fence);
    rcu_read_unlock();

    return fence;
}
#endif // DRM_VERSION_CODE < KERNEL_VERSION(5,17,0)
#else // DRM_VERSION_CODE >= KERNEL_VERSION(5,14,0)
#define __reservation_object_get_fences_rcu    dma_resv_get_fences_rcu
#define gf_reservation_object_get_excl_rcu      dma_resv_get_excl_rcu
#endif // DRM_VERSION_CODE >= KERNEL_VERSION(5,14,0)

#else // DRM_VERSION_CODE >= KERNEL_VERSION(5,4,0)
#include <linux/reservation.h>
#define __reservation_object_get_fences_rcu  reservation_object_get_fences_rcu
#if DRM_VERSION_CODE >= KERNEL_VERSION(4,10,0)
#define gf_reservation_object_get_excl_rcu reservation_object_get_excl_rcu
#else //DRM_VERSION_CODE >= KERNEL_VERSION(4,10,0)
static inline dma_fence_t *gf_reservation_object_get_excl_rcu(struct reservation_object *obj)
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
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(5,19,0)
static inline int reservation_get_fences(struct reservation_object *obj, bool write, unsigned int *num_fences, dma_fence_t ***fences)
{
    enum dma_resv_usage usage = write ? DMA_RESV_USAGE_READ: DMA_RESV_USAGE_WRITE;

    return dma_resv_get_fences(obj, usage, num_fences, fences);
}
#elif DRM_VERSION_CODE < KERNEL_VERSION(5,18,0)
static inline int reservation_get_fences(struct reservation_object *obj, bool write, unsigned int *num_fences, dma_fence_t ***fences)
{
    int ret = 0;
    dma_fence_t *excl = NULL;

    while(*num_fences)
        dma_fence_put((*fences)[--(*num_fences)]);

    if (write) {
        ret = __reservation_object_get_fences_rcu(obj, &excl, num_fences, fences);
        if (ret)
            return ret;
        if (excl) {
            // append excl fence to the array
            *fences = krealloc(*fences, (*num_fences + 1) * sizeof(void*), GFP_KERNEL);
            (*fences)[(*num_fences)++] = excl;
        }
    } else {
        excl = gf_reservation_object_get_excl_rcu(obj);
        if (excl) {
            *fences = krealloc(*fences, sizeof(void*), GFP_KERNEL);
            (*fences)[0] = excl;
            *num_fences = 1;
        }
    }
    return ret;
}
#else
#define reservation_get_fences dma_resv_get_fences
#endif

struct gf_dma_fence_driver;

struct gf_dma_fence
{
    unsigned long long magic_number;
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
    atomic_t                fence_alloc_count;
    gf_card_t               *gf_card;
    void*                   primary_bo[MAX_CORE_CRTCS];
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
