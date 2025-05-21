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
#include "gf_fence.h"
#include "gf_driver.h"
#include "gf_gem.h"
#include "gf_gem_priv.h"
#include "gf_disp.h"
#include "gf_trace.h"

static const char *engine_name[MAX_ENGINE_COUNT] = {
    "ring0",
    "ring1",
    "ring2",
    "ring3",
    "ring4",
    "ring5",
    "ring6",
    "ring7",
    "ring8",
    "ring9",
};

static bool gf_dma_fence_signaled(dma_fence_t *base)
{
    struct gf_dma_fence *fence = to_gf_fence(base);

    return gf_core_interface->is_fence_back(fence->driver->adapter, gf_engine_by_fence(fence->driver, base), fence->value);
}

static bool gf_dma_fence_enable_signaling(dma_fence_t *base)
{
    bool ret = true;
    struct gf_dma_fence *fence = to_gf_fence(base);
    struct gf_dma_fence_driver *driver = fence->driver;
    struct gf_dma_fence_context *context = gf_context_by_fence(driver, base);

    trace_gfx_dma_fence_enable_signaling(base);

    assert_spin_locked(&context->lock);
    if (gf_dma_fence_signaled(base))
        return false;

    spin_lock(&driver->lock);
    if (gf_dma_fence_signaled(base))
    {
        ret = false;
        goto unlock;
    }
    dma_fence_get(base);
    list_add_tail(&fence->link, &driver->fence_list);
    gf_get_nsecs(&fence->enqueue_time);
    atomic_inc(&driver->fence_count);

unlock:
    spin_unlock(&driver->lock);
    return ret;
}

signed long gf_dma_fence_wait(dma_fence_t *base, bool intr, signed long timeout)
{
    trace_gfx_dma_fence_wait(base, intr, timeout);

    return dma_fence_default_wait(base, intr, timeout);
}

static void gf_dma_fence_free(struct rcu_head *rcu)
{
    dma_fence_t *base = container_of(rcu, dma_fence_t, rcu);
    struct gf_dma_fence *fence = to_gf_fence(base);

    if (!fence->magic_number)
    {
        atomic_dec(&fence->driver->fence_alloc_count);
        kmem_cache_free(fence->driver->fence_slab, fence);
    }
    else
    {
        gf_info("detect someone trying to write magic_number:0x%llx\n", fence->magic_number);
    }
}

static void gf_dma_fence_release(dma_fence_t *base)
{
    call_rcu(&base->rcu, gf_dma_fence_free);
}

static const char *gf_dma_fence_get_driver_name(dma_fence_t *fence)
{
    return "gf";
}

static const char *gf_dma_fence_get_timeline_name(dma_fence_t *fence_)
{
    struct gf_dma_fence *fence = to_gf_fence(fence_);

    if (gf_dma_fence_signaled(fence_))
        return "signaled";
    else if (fence->value == fence->initialize_value)
        return "swq";
    else
        return engine_name[gf_engine_by_fence(fence->driver, fence_)];
}

static dma_fence_ops_t gf_dma_fence_ops = {
    .get_driver_name    = gf_dma_fence_get_driver_name,
    .enable_signaling   = gf_dma_fence_enable_signaling,
    .signaled           = gf_dma_fence_signaled,
    .wait               = gf_dma_fence_wait,
    .release            = gf_dma_fence_release,
    .get_timeline_name  = gf_dma_fence_get_timeline_name,
};

static void *gf_dma_fence_create_cb(void *driver_, unsigned int engine_index, unsigned long long initialize_value)
{
    unsigned int seq;
    struct gf_dma_fence *fence;
    struct gf_dma_fence_driver *driver = driver_;
    struct gf_dma_fence_context *context = gf_context_by_engine(driver, engine_index);

    fence = kmem_cache_alloc(driver->fence_slab, GFP_KERNEL);
    if (fence == NULL)
        return NULL;

    seq = ++context->sync_seq;
    fence->initialize_value =
    fence->value            = initialize_value;
    fence->driver = driver;
    fence->magic_number = 0;
    gf_get_nsecs(&fence->create_time);
    INIT_LIST_HEAD(&fence->link);
    dma_fence_init(&fence->base, &gf_dma_fence_ops, &context->lock, context->id, seq);

    atomic_inc(&driver->fence_alloc_count);

    return fence;
}

static void gf_dma_fence_attach_bo_cb(void *driver_, void *bo_, void *fence_, int readonly)
{
    struct drm_gf_gem_object *bo = bo_;
    struct gf_dma_fence *fence = fence_;

    reservation_object_lock(bo->resv, NULL);
    if (!readonly)
    {
#if  DRM_VERSION_CODE >= KERNEL_VERSION(5,19,0)
        if (reservation_object_reserve_shared(bo->resv,1) == 0)
#endif
        {
            reservation_object_add_excl_fence(bo->resv, &fence->base);
        }
    }
#if DRM_VERSION_CODE < KERNEL_VERSION(5,0,0)
    else if (reservation_object_reserve_shared(bo->resv) == 0)
#else
    else if (reservation_object_reserve_shared(bo->resv,1) == 0)
#endif
    {
        reservation_object_add_shared_fence(bo->resv, &fence->base);
    }
    reservation_object_unlock(bo->resv);
}

static void gf_dma_fence_release_cb(void *driver, void *fence_)
{
    struct gf_dma_fence *fence = fence_;

    dma_fence_put(&fence->base);
}

static void gf_dma_fence_post_event_cb(void *driver_)
{
    struct gf_dma_fence_driver *driver = driver_;

    if (driver && atomic_read(&driver->fence_count) > 0)
    {
        gf_thread_wake_up(driver->event);
    }
}

static void gf_dma_fence_update_value_cb(void *driver_, void *fence_, unsigned long long value)
{
    struct gf_dma_fence *fence = fence_;

    fence->value = value;
}

struct gf_dma_sync_object_fence_cb
{
    dma_fence_cb_t cb;
    struct gf_dma_sync_object *sync_obj;
};

struct gf_dma_sync_object
{
    dma_fence_t base;

    spinlock_t lock;
    unsigned num_fences;
    atomic_t num_pending;
    dma_fence_t **fences;

    dma_fence_cb_t cb;
    void (*callback)(void *);
    void *arg;

    struct gf_dma_sync_object_fence_cb *fence_cb;
};

static const char *gf_dma_sync_object_get_driver_name(dma_fence_t *fence)
{
    return "gf_dma_sync_object";
}

static const char *gf_dma_sync_object_get_timeline_name(dma_fence_t *fence)
{
    return "unbound";
}

static void gf_dma_sync_object_fence_cb_func(dma_fence_t *f, dma_fence_cb_t *cb)
{
    struct gf_dma_sync_object_fence_cb *fence_cb = container_of(cb, struct gf_dma_sync_object_fence_cb, cb);
    struct gf_dma_sync_object *sync_obj = fence_cb->sync_obj;

    if (atomic_dec_and_test(&sync_obj->num_pending))
    {
        dma_fence_signal(&sync_obj->base);
    }

    dma_fence_put(&sync_obj->base);
}

static bool gf_dma_sync_object_enable_signaling(dma_fence_t *fence)
{
    struct gf_dma_sync_object *sync_obj = container_of(fence, struct gf_dma_sync_object, base);
    unsigned i;

    for (i = 0; i < sync_obj->num_fences; ++i)
    {
        sync_obj->fence_cb[i].sync_obj = sync_obj;
        dma_fence_get(&sync_obj->base);
        if (dma_fence_add_callback(sync_obj->fences[i], &sync_obj->fence_cb[i].cb, gf_dma_sync_object_fence_cb_func))
        {
            dma_fence_put(&sync_obj->base);
            if (atomic_dec_and_test(&sync_obj->num_pending))
            {
                return false;
            }
        }
    }

    return true;
}

static bool gf_dma_sync_object_signaled(dma_fence_t *fence)
{
    struct gf_dma_sync_object *sync_obj = container_of(fence, struct gf_dma_sync_object, base);

    return atomic_read(&sync_obj->num_pending) <= 0;
}

static void gf_dma_sync_object_release(dma_fence_t *fence)
{
    struct gf_dma_sync_object *sync_obj = container_of(fence, struct gf_dma_sync_object, base);
    unsigned i;

    for (i = 0; i < sync_obj->num_fences; ++i)
    {
        dma_fence_put(sync_obj->fences[i]);
    }

    gf_free(sync_obj);
}

static dma_fence_ops_t gf_dma_sync_object_ops =
{
    .get_driver_name = gf_dma_sync_object_get_driver_name,
    .get_timeline_name = gf_dma_sync_object_get_timeline_name,
    .enable_signaling = gf_dma_sync_object_enable_signaling,
    .signaled = gf_dma_sync_object_signaled,
    .wait = dma_fence_default_wait,
    .release = gf_dma_sync_object_release,
};

static void gf_dma_sync_object_callback(dma_fence_t *fence, dma_fence_cb_t *cb)
{
    struct gf_dma_sync_object *sync_obj = container_of(cb, struct gf_dma_sync_object, cb);

    sync_obj->callback(sync_obj->arg);
}

static int gf_dma_sync_object_is_signaled_cb(void *sync_obj_)
{
    struct gf_dma_sync_object *sync_obj = sync_obj_;

    return dma_fence_is_signaled(&sync_obj->base);
}

static struct gf_dma_sync_object *gf_dma_sync_object_alloc(struct gf_dma_fence_context *context, int max_fence_count, void (*callback)(void*), void* arg)
{
    int extra_size = max_fence_count * (sizeof(struct gf_dma_sync_object_fence_cb) + sizeof(dma_fence_t*));
    struct gf_dma_sync_object *sync_obj = gf_calloc(sizeof(*sync_obj) + extra_size);

    spin_lock_init(&sync_obj->lock);
    dma_fence_init(&sync_obj->base, &gf_dma_sync_object_ops, &sync_obj->lock, context->id, ++context->sync_seq);

    sync_obj->fence_cb = (void*)(sync_obj + 1);
    sync_obj->fences = (void*)(sync_obj->fence_cb + max_fence_count);

    sync_obj->num_fences = 0;
    atomic_set(&sync_obj->num_pending, 0);
    sync_obj->callback = callback;
    sync_obj->arg = arg;

    return sync_obj;
}

static void* gf_dma_sync_object_create_cb(void *driver_, void *bo_, int write, int engine, void (*callback)(void*), void* arg)
{
    struct gf_dma_fence_driver *driver = driver_;
    struct gf_dma_fence_context *context = gf_context_by_engine(driver, engine);
    struct drm_gf_gem_object *bo = bo_;
    struct gf_dma_sync_object *sync_obj = NULL;
    int ret;
    int i, fence_count = 0;
    dma_fence_t **fences = NULL;

    ret = reservation_get_fences(bo->resv, write, &fence_count, &fences);
    gf_assert(ret == 0, GF_FUNC_NAME(__func__));

    for (i = 0; i < fence_count; i++)
    {
        if (fences[i]->context != context->id)
        {
            if (!sync_obj)
            {
                sync_obj = gf_dma_sync_object_alloc(context, fence_count - i, callback, arg);
            }

            sync_obj->fences[sync_obj->num_fences++] = fences[i];
        }
        else
        {
            dma_fence_put(fences[i]);
        }
    }

    if (fences)
        kfree(fences);

    if (sync_obj)
    {
        atomic_set(&sync_obj->num_pending, sync_obj->num_fences);

        if (dma_fence_add_callback(&sync_obj->base, &sync_obj->cb, gf_dma_sync_object_callback))
        {
            dma_fence_put(&sync_obj->base);
            sync_obj = NULL;
        }
    }

    return sync_obj;
}

static void gf_dma_sync_object_release_cb(void *sync_obj_)
{
    struct gf_dma_sync_object *sync_obj = sync_obj_;

    dma_fence_put(&sync_obj->base);
}

static void gf_fence_dump(struct gf_dma_fence *fence)
{
    gf_info("  DmaFence %p baseid:%lld context:%lld value:%lld flags:0x%x\n", fence,fence->driver->base_context_id, fence->base.context, fence->value, fence->base.flags);
}

void gf_check_touch_primary(void *_driver, void *_bo)
{
    struct drm_gf_gem_object *bo = (struct drm_gf_gem_object *)_bo;
    struct gf_dma_fence_driver *driver = (struct gf_dma_fence_driver *)_driver;
    gf_card_t *gf_card = driver->gf_card;
    disp_info_t* disp_info = (disp_info_t *)gf_card->disp_info;
    int i;

    for (i = 0; i < disp_info->num_crtc; i++)
    {
        if (bo && bo->info.gpu_virt_addr == gf_card->primary_addr[i])
        {
           spin_lock(&driver->lock);
           if (driver->primary_bo[i])
           {
               gf_gem_object_put(driver->primary_bo[i]);
           }
           driver->primary_bo[i] = gf_gem_object_get(bo);
           spin_unlock(&driver->lock);
           //gf_info("try draw to bo %llx while it's used for flip in crtc %d\n", bo, i);

           gf_acquire_display(disp_info, DISP_ONSCREENDRAW_REF);
       }

    }
}

static void gf_touch_primary_done(struct gf_dma_fence_driver *driver)
{
   int i,j, fence_count = 0;
   struct drm_gf_gem_object *bo = NULL;
   dma_fence_t **fences = NULL;
   int ret = 0;
   gf_card_t *gf_card = driver->gf_card;
   disp_info_t* disp_info = (disp_info_t *)gf_card->disp_info;

   for (i = 0; i < disp_info->num_crtc; i++)
   {
        bo = driver->primary_bo[i];
        if (bo && bo_resv(bo))
        {
           int touch_primary_done = 1;
           ret = reservation_get_fences(bo_resv(bo), 1, &fence_count, &fences);
           gf_assert(ret == 0, "");

           for (j = 0; j < fence_count; j++)
           {
               if (!gf_dma_fence_signaled(fences[j]))
               {
                   touch_primary_done = 0;
               }
               dma_fence_put(fences[j]);
           }

           if (touch_primary_done)
           {
               spin_lock(&driver->lock);
               gf_gem_object_put(driver->primary_bo[i]);
               driver->primary_bo[i] = NULL;
               spin_unlock(&driver->lock);
               //gf_info("draw to bo %llx in crtc %d has done\n", bo, i);

               gf_release_display(disp_info, DISP_ONSCREENDRAW_REF);
           }

           if (fences)
           {
              kfree(fences);
              fences = NULL;
              fence_count = 0;
           }
        }
   }
}

static long gf_dma_sync_object_wait_cb(void *driver, void *sync_obj_, unsigned long long timeout)
{
    struct gf_dma_sync_object *sync_obj = sync_obj_;

    return dma_fence_wait_timeout(&sync_obj->base, FALSE, gf_do_div(timeout * HZ, 1000));
}

static void gf_dma_sync_object_dump_cb(void *sync_obj_)
{
    int i;
    struct gf_dma_sync_object *sync_obj = sync_obj_;

    gf_info("FenceArray %p, count:%d, flags:%x signal:%d\n",
            sync_obj, sync_obj->num_fences, sync_obj->base.flags, dma_fence_is_signaled(&sync_obj->base));

    for (i = 0; i < sync_obj->num_fences; i++)
    {
        dma_fence_t *fence = sync_obj->fences[i];

        if (fence && fence->ops == &gf_dma_fence_ops)
        {
            gf_fence_dump(to_gf_fence(fence));
        }
    }
}

static gf_drm_callback_t gf_drm_callback =
{
    .fence = {
        .create       = gf_dma_fence_create_cb,
        .attach_buffer= gf_dma_fence_attach_bo_cb,
        .update_value = gf_dma_fence_update_value_cb,
        .release      = gf_dma_fence_release_cb,
        .notify_event = gf_dma_fence_post_event_cb,

        .dma_sync_object_create         = gf_dma_sync_object_create_cb,
        .dma_sync_object_release        = gf_dma_sync_object_release_cb,
        .dma_sync_object_is_signaled    = gf_dma_sync_object_is_signaled_cb,
        .dma_sync_object_wait           = gf_dma_sync_object_wait_cb,
        .dma_sync_object_dump           = gf_dma_sync_object_dump_cb,
    },

    .gem = {
        .get_from_handle = gf_get_from_gem_handle,
    },

    .kms = {
        .check_touch_primary = gf_check_touch_primary,
    },
};

static int gf_dma_fence_event_thread(void *data)
{
    int ret = 0;
    struct list_head process_list;
    struct gf_dma_fence *fence, *tmp;
    struct gf_dma_fence_driver *driver = data;
    int try_freeze_num = 0;

    gf_set_freezable();
    INIT_LIST_HEAD(&process_list);

    do {
        ret = gf_thread_wait(driver->event, driver->timeout_msec);
        gf_touch_primary_done(driver);

try_again1:
        spin_lock(&driver->lock);
        list_splice_init(&driver->fence_list, &process_list);
        spin_unlock(&driver->lock);

try_again2:
        list_for_each_entry_safe(fence, tmp, &process_list, link)
        {
            if (gf_dma_fence_signaled(&fence->base))
            {
                list_del(&fence->link);
                atomic_dec(&driver->fence_count);
                dma_fence_signal(&fence->base);
                dma_fence_put(&fence->base);
            }
        }

        if (driver->can_freeze)
        {
            if(gf_freezing())
            {
                if (!list_empty(&process_list))
                {
                    gf_msleep(10); //relase CPU wait 10ms, and try_again

                    if (++try_freeze_num > 100)
                    {
                        gf_info("dma fence event thread! try freezing count %d \n", try_freeze_num);
                        try_freeze_num = 0;
                    }
                    goto try_again2;
                }else if(!list_empty(&driver->fence_list)){
                    goto try_again1;
                }
                gf_info("sleep fence thread! freezing\n");
            }

            spin_lock(&driver->lock);
            list_splice_init(&process_list, &driver->fence_list);
            spin_unlock(&driver->lock);
            if(gf_try_to_freeze())
                gf_info("dma fence thread resumed!\n");
        }
    } while(!gf_thread_should_stop());

    return 0;
}

long gf_gem_fence_await_reservation(struct reservation_object *resv, int exclude_self, long timeout, int write)
{
    int ret = 0;
    dma_fence_t **fences = NULL;
    unsigned int fence_count = 0, i;

    if (timeout == 0)
        timeout = 1;

    ret = reservation_get_fences(resv, write, &fence_count, &fences);
    if (ret)
        return (long)ret;

    for (i = 0; i < fence_count; i++)
    {
        if (exclude_self && fences[i]->ops == &gf_dma_fence_ops)
            continue;

        timeout = dma_fence_wait_timeout(fences[i], 0, timeout);
        if (timeout < 0)
            break;
    }

    for (i = 0; i < fence_count; i++)
    {
        dma_fence_put(fences[i]);
    }

    if (fences)
        kfree(fences);

    if (timeout <= 0)
        gf_warning("wait fences timeout or interrupted:%d\n", timeout);

    return timeout;
}

int gf_dma_fence_driver_init(void *adapter, struct gf_dma_fence_driver *driver)
{
    int i;
    int engine_count = MAX_ENGINE_COUNT;

    driver->adapter = adapter;
    driver->base_context_id = dma_fence_context_alloc(engine_count);
    driver->context = gf_calloc(sizeof(struct gf_dma_fence_context) * engine_count);
    spin_lock_init(&driver->lock);
    driver->fence_slab = kmem_cache_create(
        "gf_dma_fence", sizeof(struct gf_dma_fence), 0, SLAB_HWCACHE_ALIGN, NULL);
    gf_assert(driver->fence_slab != NULL, GF_FUNC_NAME(__func__));
    for (i = 0; i < engine_count; i++)
    {
        driver->context[i].id = driver->base_context_id + i;
        driver->context[i].sync_seq = 0;
        spin_lock_init(&driver->context[i].lock);
    }
    gf_core_interface->set_callback_func(adapter, OS_CALLBACK_DRM_CB, &gf_drm_callback, driver);

    atomic_set(&driver->fence_count, 0);
    driver->timeout_msec = -1;
    driver->can_freeze = TRUE;
    driver->event = gf_create_event(0);
    atomic_set(&driver->fence_alloc_count, 0);
    INIT_LIST_HEAD(&driver->fence_list);
    driver->os_thread = gf_create_thread(gf_dma_fence_event_thread, driver, "dma_fenced");

    return 0;
}

void gf_dma_fence_driver_fini(struct gf_dma_fence_driver *driver)
{
    gf_destroy_thread(driver->os_thread);

    gf_destroy_event(driver->event);
    driver->event = NULL;

    if(driver->context)
    {
        gf_free(driver->context);
        driver->context = NULL;
    }

    rcu_barrier();
    kmem_cache_destroy(driver->fence_slab);
    driver->fence_slab = NULL;
}

void gf_dma_track_fences_dump(gf_card_t *card)
{
    struct gf_dma_fence_driver *driver = card->fence_drv;
    struct gf_dma_fence *fence;
    unsigned long long now;

    spin_lock(&driver->lock);
    gf_get_nsecs(&now);
    gf_info("now:%lld, fence_alloc_count: %d, queue_count: %d\n", now, atomic_read(&driver->fence_alloc_count), atomic_read(&driver->fence_count));
    list_for_each_entry(fence, &driver->fence_list, link)
    {
        gf_info(" fence:%p value:%lld context:%d seqno:%d create:%lld enqueue:%lld.\n",
                fence, fence->value, fence->base.context, fence->base.seqno, fence->create_time, fence->enqueue_time);
    }
    spin_unlock(&driver->lock);
}

void gf_dma_track_fences_clear(gf_card_t *card)
{
}
