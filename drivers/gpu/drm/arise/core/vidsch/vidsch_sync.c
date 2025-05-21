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
#include "gf_adapter.h"
#include "vidsch.h"
#include "vidschi.h"
#include "vidsch_sync.h"
#include "vidsch_workerthread.h"
#include "handle_manager.h"
#include "perfevent.h"


//NOTE: this function can't be sleep as be called in spinlocked
static int vidschi_do_destroy_sync_object(adapter_t *adapter, vidsch_sync_object_t *sync_obj)
{
    switch (sync_obj->type)
    {
    case GF_SYNC_OBJ_TYPE_MUTEX:

        gf_destroy_mutex(sync_obj->mutex.mlock);

        break;

    case GF_SYNC_OBJ_TYPE_SEMPAPHORE:

        gf_destroy_sema(sync_obj->sema.slock);

        break;

    case GF_SYNC_OBJ_TYPE_FENCE:

        vidsch_release_fence_space(adapter, adapter->fence_buf, sync_obj->fence.gpu_addr);

        break;

    case GF_SYNC_OBJ_TYPE_DMAFENCE:

        adapter->drm_cb->fence.dma_sync_object_release(sync_obj->dma.dma_sync_obj);

        break;

    case GF_SYNC_OBJ_TYPE_CPU_NOTIFICATION:

        break;

    default:

        break;
    }

    if(sync_obj->lock)
    {
        gf_destroy_spinlock(sync_obj->lock);
    }

    if(sync_obj->ref_cnt)
    {
        gf_destroy_atomic(sync_obj->ref_cnt);
    }

    if(sync_obj->handle != 0)
    {
        remove_handle(&adapter->hdl_mgr, sync_obj->handle);
    }

    gf_free(sync_obj);

    return S_OK;
}

void vidschi_add_sync_to_defer_destroy(adapter_t *adapter, vidsch_sync_object_t *sync_obj)
{
    gf_spin_lock(adapter->lock);

    list_add_tail(&sync_obj->list_node, &adapter->sync_obj_list);

    gf_spin_unlock(adapter->lock);
}

void vidschi_destroy_defer_sync_obj(adapter_t *adapter)
{
    vidsch_sync_object_t *sync_obj      = NULL;
    vidsch_sync_object_t *sync_obj_next = NULL;

    gf_spin_lock(adapter->lock);

    list_for_each_entry_safe(sync_obj, sync_obj_next, &adapter->sync_obj_list, list_node)
    {
        if(!vidschi_sync_obj_referenced(sync_obj))
        {
            list_del(&sync_obj->list_node);

            vidschi_do_destroy_sync_object(adapter, sync_obj);
        }
    }

    gf_spin_unlock(adapter->lock);
}


int vidsch_create_sync_object(gpu_device_t *gpu_device, gf_create_fence_sync_object_t *create, int binding)
{
    vidsch_sync_object_t  *sync_obj = NULL;
    int                   status    = S_OK;

    vidschi_destroy_defer_sync_obj(gpu_device->adapter);

    if(!binding)
    {
        //add sync object size limit per device
        if(gf_atomic_read(gpu_device->sync_obj_count) > MAX_SYNC_OBJECT_SIZE_PER_DEVICE)
        {
            gf_error("allocate sync obj failed for exceeding the limit\n");
            return E_FAIL;
        }
        else
        {
            gf_atomic_inc(gpu_device->sync_obj_count);
        }
    }

    sync_obj = gf_calloc(sizeof(vidsch_sync_object_t));

    if(sync_obj == NULL)
    {
        gf_error("allocate sync obj failed. pls check if leak happen.\n");
        return E_FAIL;
    }

    list_init_head(&sync_obj->instance_list);

    switch (create->type)
    {
    case GF_SYNC_OBJ_TYPE_MUTEX:

        sync_obj->mutex.mlock         = gf_create_mutex();
        sync_obj->mutex.initial_state = create->mutex.init_state;

        if(create->mutex.init_state)
        {
            gf_mutex_lock(sync_obj->mutex.mlock);
        }

        break;

    case GF_SYNC_OBJ_TYPE_SEMPAPHORE:

        sync_obj->sema.slock       = gf_create_sema(create->semaphore.max_count);
        sync_obj->sema.initial_cnt = create->semaphore.init_count;

        break;

    case GF_SYNC_OBJ_TYPE_FENCE:

        status = vidsch_request_fence_space(gpu_device->adapter, gpu_device->adapter->fence_buf, &sync_obj->fence.virt_addr, &sync_obj->fence.gpu_addr);

        if(status == S_OK)
        {
            create->fence.fence_addr = sync_obj->fence.gpu_addr;

            *sync_obj->fence.virt_addr = create->fence.init_value;
        }
        else
        {
            gf_error("allocate fence space failed. pls check if leak happen.\n");
            goto exit;
        }

        break;

    case GF_SYNC_OBJ_TYPE_CPU_NOTIFICATION:
        gf_error("currently not support cpu notification SyncObject.\n");
        status = E_FAIL;
        goto exit;

    default:
        gf_error("vidsch_create_sync_object: invalid type.\n ");
        status = E_FAIL;
        goto exit;
    }

    create->fence_sync_object =
    sync_obj->handle    = add_handle(&gpu_device->adapter->hdl_mgr, HDL_TYPE_SYNC_OBJ, sync_obj);
    sync_obj->device    = gpu_device;
    sync_obj->type      = create->type;
    sync_obj->binding   = binding;
    sync_obj->lock      = gf_create_spinlock(0);
    sync_obj->ref_cnt   = gf_create_atomic(0);

    if(!sync_obj->binding)
    {
        cm_device_lock(gpu_device);

        list_add(&sync_obj->list_node, &gpu_device->sync_obj_list);

        cm_device_unlock(gpu_device);
    }

exit:

    if(status != S_OK)
    {
        vidschi_do_destroy_sync_object(gpu_device->adapter, sync_obj);
    }

    return status;
}

int vidsch_destroy_sync_object(adapter_t *adapter, gpu_device_t *gpu_device, unsigned int hSync)
{
    vidsch_sync_object_t  *sync_obj = get_from_handle(&adapter->hdl_mgr, hSync);

    if(sync_obj == NULL)
    {
        gf_error("destroy invalid sync obj: %d.\n", hSync);
        return S_OK;
    }

    if(!sync_obj->binding)
    {
        gf_assert(gpu_device != NULL, "gpu_device != NULL");

        cm_device_lock(gpu_device);

        list_del(&sync_obj->list_node);

        gf_atomic_dec(gpu_device->sync_obj_count);

        cm_device_unlock(gpu_device);
    }

    sync_trace("destroy sync obj: %x.\n", hSync);

    if(vidschi_sync_obj_referenced(sync_obj))
    {
        vidschi_add_sync_to_defer_destroy(adapter, sync_obj);
    }
    else
    {
        vidschi_do_destroy_sync_object(adapter, sync_obj);
    }

    return S_OK;
}

int vidsch_destroy_remained_sync_obj(gpu_device_t *gpu_device)
{
    vidsch_sync_object_t *sync_obj = NULL, *sync_obj_next;

    list_for_each_entry_safe(sync_obj, sync_obj_next, &gpu_device->sync_obj_list, list_node)
    {
        vidsch_destroy_sync_object(gpu_device->adapter, gpu_device, sync_obj->handle);
    }

    vidschi_destroy_defer_sync_obj(gpu_device->adapter);

    return S_OK;
}

static int vidschi_is_fence_sync_object_signaled(vidsch_wait_fence_signaled_arg_t *argu)
{
    task_wait_t                *task       = argu->task;
    vidsch_wait_instance_t     *wait       = argu->wait;
    vidsch_fence_sync_object_t *fence_sync = &wait->sync_obj->fence;

    /* if device have dropped task before, since fence signal cmd also put in gerneal dma task,
     * so dropped task may lead this wait never back, so force set it back if device dropped task.
     */
    if(wait->sync_obj->device->task_dropped)
    {
        *(volatile unsigned long long *)fence_sync->virt_addr = wait->fence_id;
    }

    /* only consider fence back and all previous task submit to sw queue as signaled */
    /* and fence sync have no explicit signal func called by user like mutex, semaphore
     * so consider fence object signaled when first fence back meet.
     */

    if((!wait->signaled) &&
       (*(volatile unsigned long long *)fence_sync->virt_addr >= wait->fence_id) &&
       (task->only_check_status || vidschi_can_submit_task(&task->desc)))
    {
        wait->signaled = TRUE;
    }

    return wait->signaled;
}

/* Server wait must timeout one by one, since when server wait pending queue may pending many wait_instanced based on sync_obj_X.
 * like wait_X_A, wait_X_B, wait_X_C. which in multi-engine pending queue like engine_A, engine_B, engine_C, since A,B,C in seperated thread.
 * we better timeout it by app wait order.
 */
static int vidschi_is_server_wait_instance_timeout(task_wait_t *task, vidsch_wait_instance_t *wait, unsigned long long curr_time)
{
    vidsch_sync_object_t  *sync_obj = wait->sync_obj;
    gpu_context_t         *context  = task->desc.context;
    adapter_t             *adapter = context->device->adapter;

    unsigned long long    task_id   = task->desc.context_task_id;
    long long             used_time = 0;
    int timeout = FALSE;
    unsigned long long waittime = adapter->sync_max_server_wait_time_ns;

    gf_spin_lock(sync_obj->lock);

    /* check if instance is first place in instance list. we check timeout in instance list in order. only first can timeout */
    if(wait == list_entry(sync_obj->instance_list.next, vidsch_wait_instance_t, list_node))
    {
        unsigned long long delta_time = 0;

        if((task_id <= context->wait_delta_check_end) &&
           (task_id >  context->wait_delta_check_start))
        {
            {
                /* for general just increase 0.5s, previous is a timeout, which may stop a signal cmd to submit,
                 * So here if previous have timeout, the followed wait more time.
                 * NOTE: this value is Optional
                 * NOTE: big value here may lead timeout use more time. another word screen may stop more time.
                 */
                delta_time = 500000000; //0.5s
            }

            //gf_info("increase delta time: context: %x. engine: %d. sync_obj: %x, delta: %d.\n",
            //    context->handle, context->engine_index, sync_obj->handle, (int)(task_id - context->wait_delta_check_start));
        }

        delta_time += wait->delta_time;
        used_time  = curr_time - task->desc.timestamp;

        if(used_time >= (waittime + delta_time))
        {
            timeout = TRUE;
        }
    }

    /* if timeout happen, increase the follow instance delta_time, to let it wait more times,
     * only timeout the real timeout instance
     */
    if(timeout)
    {
        vidsch_wait_instance_t *instance = NULL;
        vidsch_wait_instance_t *instance_next;

        list_for_each_entry_safe(instance, instance_next, &sync_obj->instance_list, list_node)
        {
            instance->delta_time += gf_do_div(waittime, 2);
        }

        /* if one wait timeout, we need care follow wait. only set follow 100 here, this will enough */
        context->wait_delta_check_start = task_id;
        context->wait_delta_check_end   = context->task_id + 100;
    }

    gf_spin_unlock(sync_obj->lock);

    return timeout;
}

int vidschi_is_server_wait_timeout(task_wait_t *task, unsigned long long curr_time)
{
    vidsch_wait_instance_t *instance = NULL;
    gpu_context_t         *context  = task->desc.context;
    adapter_t             *adapter = context->device->adapter;

    int i = 0, task_timeout = FALSE, instance_timeout = FALSE;
    long long used_time = curr_time - task->desc.timestamp;

    /* first precheck if task exist exceed timeout */
    if(used_time < adapter->sync_max_server_wait_time_ns)
    {
        return FALSE;
    }

    /* if task exceed timeout value, rechck if instance real timeout
     * if any instance timeout then treat task is timeout. because we only timeout first instance in sync_obj,
     * and we support multi-instance in one task, if all instance timeout may lead never timeout.
     */
    for(i = 0; i < task->instance_cnt; i++)
    {
        instance = &task->instances[i];

        if(instance->sync_obj == NULL) continue;

        instance_timeout = vidschi_is_server_wait_instance_timeout(task, &task->instances[i], curr_time);

        if(instance_timeout)
        {
            gf_info("Instance: %p task_id: %lld context: %x, engine:%d, timeout. start time: %lld, curr_time:%lld. instance_delta: %lld\n",
                instance, task->desc.context_task_id, task->desc.context->handle, task->desc.context->engine_index,
                task->desc.timestamp, curr_time, instance->delta_time);

            task_timeout = TRUE;
            break;
        }
    }

    /* if task is timeout, del the instance reference the sync_obj
     */
    if(task_timeout)
    {
        gf_info("Server Wait Timeout.\n");

        vidschi_dump_wait_task(NULL, task, 0, FALSE);

        for(i = 0; i < task->instance_cnt; i++)
        {
            instance = &task->instances[i];

            if(instance->sync_obj != NULL)
            {
                vidschi_sync_obj_del_reference(instance->sync_obj, instance);

                instance->sync_obj = NULL;
            }
        }
    }

    return task_timeout;
}


/* check if sync objected signaled */
/* client_wait means if this function called from client side, this for client/server mixed wait case, and mult-context.
 * above case only client side can not insure the task submit order. So if this wait_task if client wait,
 * we must wait utill ref_cnt == 0, all previous wait on this sync_obj all finished, if server wait we submit wait_task
 * to pending queue to let worker thread do it.
 *
 * for fence obj: there is no multi-context issue, but we must insurance multi-wait in one context submit in order, no matter
 * it signaled or not, So for this add a per context task id, if still some task pending before wait task, we consider wait as
 * unsignaled.
 */

static int vidschi_is_sync_object_signaled(task_wait_t *task, vidsch_wait_instance_t *wait, int client_wait)
{
    adapter_t *adapter = task->desc.context->device->adapter;
    vidsch_sync_object_t  *sync_obj = wait->sync_obj;
    int                   signaled  = FALSE;

    switch (sync_obj->type)
    {
    case GF_SYNC_OBJ_TYPE_MUTEX:

        if(client_wait && gf_atomic_read(wait->sync_obj->ref_cnt) >= 1)
        {
            break;
        }

        if(gf_mutex_trylock(wait->sync_obj->mutex.mlock) == GF_LOCKED)
        {
            signaled = TRUE;
        }

        break;

    case GF_SYNC_OBJ_TYPE_SEMPAPHORE:

        if(client_wait && (gf_atomic_read(wait->sync_obj->ref_cnt) >= wait->sync_obj->sema.initial_cnt))
        {
            break;
        }

        if(gf_down_trylock(wait->sync_obj->sema.slock) == GF_LOCKED)
        {
            signaled = TRUE;
        }

        break;

    case GF_SYNC_OBJ_TYPE_DMAFENCE:
        if (adapter->drm_cb->fence.dma_sync_object_is_signaled(sync_obj->dma.dma_sync_obj))
        {
            signaled = TRUE;
        }
        break;
    case GF_SYNC_OBJ_TYPE_FENCE:
    {
        vidsch_wait_fence_signaled_arg_t argu = {0};

        argu.task = task;
        argu.wait = wait;

        if(vidschi_is_fence_sync_object_signaled(&argu))
        {
            signaled = TRUE;
        }

        break;
    }

    case GF_SYNC_OBJ_TYPE_CPU_NOTIFICATION:

        break;

    default:

        break;
    }

    return signaled;
}

/* client wait will wait untill fence conditon meet or timeout expired */
#define DEFAULT_CLIENT_WAIT_TIMEOUT   10000

static int vidschi_wait_fence_sync_object_signaled(task_wait_t *task, vidsch_wait_instance_t *instance)
{
    gpu_context_t              *context   = task->desc.context;
    adapter_t                  *adapter   = context->device->adapter;
    vidsch_mgr_t               *sch_mgr   = adapter->sch_mgr[context->engine_index];
    condition_func_t           condition  = (condition_func_t)&vidschi_is_fence_sync_object_signaled;
    unsigned int               msec       = gf_do_div(instance->timeout, 1000 * 1000);
    unsigned int               e_status   = 0, status = 0;

    vidsch_wait_fence_signaled_arg_t argu = {0};

    argu.task = task;
    argu.wait = instance;

    /* if timeout <= 5ms, warning it, since 5ms, maybe always timeout */
    if(msec <= 5)
    {
        gf_info("Warning: context: %x, TID: %d, client wait timeout: %dms, %lldns reset it to %ds.\n",
            context->handle, context->tid, msec, instance->timeout, DEFAULT_CLIENT_WAIT_TIMEOUT);
        msec = DEFAULT_CLIENT_WAIT_TIMEOUT;
    }

try_again:

    e_status = gf_wait_event_thread_safe(sch_mgr->fence_event, condition, &argu, msec);

    if(e_status == GF_EVENT_BACK)
    {
        status = GF_SYNC_OBJ_CONDITION_SATISFIED;
    }
    else if(e_status == GF_EVENT_TIMEOUT)
    {
        if(vidschi_can_submit_task(&task->desc))
        {
            status = GF_SYNC_OBJ_TIMEOUT_EXPIRED;
        }
        else
        {
            goto try_again;
        }
    }
    else if(e_status == GF_EVENT_SIGNAL)
    {
        goto try_again;
    }

    return status;
}


static int vidschi_client_wait_instance_signaled(task_wait_t *task, vidsch_wait_instance_t *instance)
{
    adapter_t *adapter = task->desc.context->device->adapter;
    vidsch_sync_object_t *sync_obj = instance->sync_obj;

    int ret = 0, status = GF_SYNC_OBJ_CONDITION_SATISFIED;

    switch (sync_obj->type)
    {
    case GF_SYNC_OBJ_TYPE_MUTEX:

        gf_mutex_lock(sync_obj->mutex.mlock);

        break;

    case GF_SYNC_OBJ_TYPE_SEMPAPHORE:

        gf_down(sync_obj->sema.slock);

        break;

    case GF_SYNC_OBJ_TYPE_FENCE:

        status = vidschi_wait_fence_sync_object_signaled(task, instance);

        break;

    case GF_SYNC_OBJ_TYPE_DMAFENCE:

        ret = adapter->drm_cb->fence.dma_sync_object_wait(adapter->drm_cb_argu, sync_obj->dma.dma_sync_obj, 1000);
        if (ret > 0)
            status = GF_SYNC_OBJ_ALREAD_SIGNALED;
        else if (!ret)
            status = GF_SYNC_OBJ_TIMEOUT_EXPIRED;
        else
            status = GF_SYNC_OBJ_ERROR;

        break;

    case GF_SYNC_OBJ_TYPE_CPU_NOTIFICATION:

        break;

    default:

        break;
    }

    return status;
}

int vidschi_client_wait(task_wait_t *task)
{
    vidsch_wait_instance_t *instance = NULL;
    int i = 0, status = GF_SYNC_OBJ_ALREAD_SIGNALED;

    for(i = 0; i < task->instance_cnt; i++)
    {
        instance = &task->instances[i];

        if(instance->sync_obj == NULL) continue;

        status = vidschi_client_wait_instance_signaled(task, &task->instances[i]);

        if((status == GF_SYNC_OBJ_ALREAD_SIGNALED) ||
           (status == GF_SYNC_OBJ_CONDITION_SATISFIED))
        {
            vidschi_sync_obj_del_reference(instance->sync_obj, instance);

            instance->sync_obj = NULL;
        }

    }

    return status;
}

int vidschi_try_wait(task_wait_t *wait_task, int client_wait)
{
    vidsch_wait_instance_t *instance = NULL;

    int i, lock_failed = FALSE;

    /* when lock failed, do we readly need unlock previous locked sync_obj.
     * I don't think so, consider this wait_t1 (mutex_A, mutex_B),  wait_t2(mutex_A)
     * when t1, Mutex_B lock failed, if we unlock mutex_A, next t2 can get the mutex_A
     * this not right, So I think, when try lock we should loop all wait instance, and lock
     * all can locked. and get these lock to wait unlocked object.
     */

    for(i = 0; i < wait_task->instance_cnt; i++)
    {
        instance = &wait_task->instances[i];

        if(instance->sync_obj == NULL) continue;

        if(vidschi_is_sync_object_signaled(wait_task, instance, client_wait))
        {
            vidschi_sync_obj_del_reference(instance->sync_obj, instance);

            instance->sync_obj = NULL;
        }
        else
        {
            lock_failed |= TRUE;
        }
    }

    return lock_failed ? E_FAIL : S_OK;
}

static task_wait_t *vidschi_allocate_wait_task(gpu_context_t *context, gf_wait_fence_sync_object_t *wait)
{
    adapter_t              *adapter  = context->device->adapter;
    task_wait_t            *wait_task= NULL;
    vidsch_wait_instance_t *instance = NULL;
    vidsch_sync_object_t   *sync_obj = NULL;
    unsigned int           hSync  = 0;

    /* only_check_status:
     * in fence sync, spec said if timeout == 0, means no wait just return current fence status,
     * impl by this no matter lient/server wait, if all obj is fence sync and all timeout == 0,
     * consider it as client wait, and return its status ASAP. but still a problem we only can return
     * one status
     */

    wait_task = gf_calloc(sizeof(task_wait_t));

    wait_task->desc.context         = context;
    wait_task->desc.type            = task_type_sync_wait;
    wait_task->desc.task_wait       = wait_task;
    wait_task->instance_cnt         = 1;
    wait_task->only_check_status    = TRUE;
    wait_task->instances            = wait_task->instances_builtin;

    gf_atomic_inc(context->ref_cnt);

    gf_get_nsecs(&wait_task->desc.timestamp);

    {
        hSync    = wait->fence_sync_object;
        instance = &wait_task->instances[0];

        if(hSync == 0) goto done;

        instance->sync_obj =
        sync_obj           = get_from_handle(&adapter->hdl_mgr, hSync);

        if(sync_obj == NULL)
        {
            gf_error("sync task: invalid sync handle 0x%x.\n", hSync);
            goto done;
        }
        vidschi_sync_obj_add_reference(sync_obj, instance);

        if(instance->sync_obj->type == GF_SYNC_OBJ_TYPE_FENCE)
        {
            instance->fence_id = wait->fence_value;
            instance->timeout  = wait->timeout;

            if(instance->timeout == 0)
            {
                wait_task->only_check_status &= TRUE;
            }
            else
            {
                wait_task->only_check_status = FALSE;
            }
        }
    }
done:
    return wait_task;
}

void vidschi_release_wait_task(task_wait_t *task)
{
    vidsch_wait_instance_t *instance = NULL;
    int i = 0;

    for(i = 0; i < task->instance_cnt; i++)
    {
        instance = &task->instances[i];

        if(instance->sync_obj == NULL) continue;

        vidschi_sync_obj_del_reference(instance->sync_obj, instance);

        instance->sync_obj = NULL;
    }

    gf_atomic_dec(task->desc.context->ref_cnt);

    if (task->instances != task->instances_builtin)
    {
        gf_free(task->instances);
    }
    gf_free(task);
}

/* current wait sync support wait multi-obj in one task, and one sync_obj can both support
 * client/server wait, for easy implementation. we consider this multi-obj as a unity,
 * we abstract these multi-obj as wait_task, and use this task to do client/server wait.
 */
int vidsch_wait_sync_object(gpu_context_t *context, gf_wait_fence_sync_object_t *wait)
{
    adapter_t              *adapter  = context->device->adapter;
    vidsch_mgr_t           *sch_mgr  = adapter->sch_mgr[context->engine_index];
    task_wait_t            *wait_task= NULL;

    int                    status    = S_OK;

    wait_task = vidschi_allocate_wait_task(context, wait);

    status = vidschi_try_wait(wait_task, TRUE);

    if(wait_task->only_check_status)
    {
        //gf_info("sync wait, only check status 0x%x last_submit_to_sw %lld task id %lld.\n", status, context->last_submit_to_sw, context->task_id);

        wait->status = (status == S_OK) ? GF_SYNC_OBJ_ALREAD_SIGNALED : GF_SYNC_OBJ_TIMEOUT_EXPIRED;

        vidschi_release_wait_task(wait_task);

        return TRUE;
    }

    /* GO real wait path */
    wait_task->desc.context_task_id = ++context->task_id;

    if (adapter->ctl_flags.perf_event_enable)
    {
        vidsch_wait_instance_t *instance = NULL;
        gf_perf_event_t        perf_event = {0};
        unsigned long long      timestamp  = 0;
        unsigned int            i;

        gf_memset(&perf_event, 0, sizeof(gf_perf_event_t));

        gf_get_nsecs(&timestamp);
        perf_event.header.timestamp_high = timestamp >> 32;
        perf_event.header.timestamp_low  = (timestamp) & 0xffffffff;

        perf_event.header.size = sizeof(gf_perf_event_wait_start_t);
        perf_event.header.type = GF_PERF_EVENT_WAIT_START;

        perf_event.wait_start.engine_idx   = context->engine_index;
        perf_event.wait_start.gpu_context  = context->handle;
        perf_event.wait_start.task_id_high = wait_task->desc.context_task_id >> 32;
        perf_event.wait_start.task_id_low  = wait_task->desc.context_task_id & 0xffffffff;

        for (i = 0; i < wait_task->instance_cnt; i++)
        {
            instance = &wait_task->instances[i];

            if(instance->sync_obj == NULL) continue;

            perf_event.wait_start.instance[i].handle           = instance->sync_obj->handle;
            perf_event.wait_start.instance[i].fence_value_high = instance->fence_id >> 32;
            perf_event.wait_start.instance[i].fence_value_low  = instance->fence_id & 0xffffffff;
            perf_event.wait_start.instance[i].timeout          = instance->timeout;
        }

        perf_event_add_event(adapter, &perf_event);
    }

    gf_task_create_trace_event(adapter->index << 16 | context->engine_index, context->handle, wait_task->desc.context_task_id, wait_task->desc.type);

    if((status == S_OK) && vidschi_can_submit_task(&wait_task->desc))
    {
        wait->status = GF_SYNC_OBJ_ALREAD_SIGNALED;

        context->last_submit_to_sw = wait_task->desc.context_task_id;

        gf_task_submit_trace_event(adapter->index << 16 | context->engine_index, context->handle, wait_task->desc.context_task_id, wait_task->desc.type, 0, wait->status);

        vidschi_release_wait_task(wait_task);
    }
    else if(wait->server_wait && adapter->ctl_flags.worker_thread_enable)
    {
        vidschi_add_task_to_pending_queue(sch_mgr, &wait_task->desc);

        wait->status = GF_SYNC_OBJ_WAIT_ON_SERVER;
    }
    else
    {
        wait->status = vidschi_client_wait(wait_task);

        context->last_submit_to_sw = wait_task->desc.context_task_id;

        gf_task_submit_trace_event(adapter->index << 16 | context->engine_index, context->handle, wait_task->desc.context_task_id, wait_task->desc.type, 0, wait->status);

        vidschi_release_wait_task(wait_task);
    }

    if ((adapter->ctl_flags.perf_event_enable)&&(wait->status != GF_SYNC_OBJ_WAIT_ON_SERVER))
    {
        gf_perf_event_t        perf_event = {0};
        unsigned long long      timestamp  = 0;
        unsigned long long      task_id    = context->last_submit_to_sw;

        gf_memset(&perf_event, 0, sizeof(gf_perf_event_t));
        gf_get_nsecs(&timestamp);

        perf_event.header.timestamp_high    = timestamp >> 32;
        perf_event.header.timestamp_low     = (timestamp) & 0xffffffff;

        perf_event.header.size              = sizeof(gf_perf_event_wait_finish_t);
        perf_event.header.type              = GF_PERF_EVENT_WAIT_FINISH;
        perf_event.wait_finish.engine_idx   = context->engine_index;
        perf_event.wait_finish.gpu_context  = context->handle;
        perf_event.wait_finish.status       = wait->status;
        perf_event.wait_finish.task_id_high = task_id >> 32;
        perf_event.wait_finish.task_id_low  = task_id & 0xffffffff;

        perf_event_add_event(adapter, &perf_event);
    }

    sync_trace("wait hSync: %x, context:%x, thread: %d, status:%d, waitValue:%lld.\n",
        wait->fence_sync_object, context->handle, context->tid, wait->status, wait->fence_value);

    return S_OK;
}


void vidschi_force_signal_fence_sync_object(vidsch_sync_object_t *sync_obj, unsigned long long fence_value)
{
    gpu_device_t *device = sync_obj->device;
    adapter_t *adapter = device->adapter;
    vidsch_mgr_t *sch_mgr = NULL;
    unsigned int engine;

    if(sync_obj->type == GF_SYNC_OBJ_TYPE_FENCE)
    {
        vidsch_fence_sync_object_t *fence = &sync_obj->fence;

//        gf_info("force signal fence sync object: %x, curr_value: %lld, signal_value: %lld.\n",
//            sync_obj->handle, *fence->virt_addr, fence_value);

        if(fence->virt_addr == NULL)
        {
            gf_error(" fence->virt_addr = NULL, hSync = %x.\n", sync_obj->handle);
        }
        else if(*fence->virt_addr < fence_value)
        {
            *fence->virt_addr = fence_value;
        }

        for(engine = 0; engine < adapter->active_engine_count; engine++)
        {
            sch_mgr = adapter->sch_mgr[engine];
            if (sch_mgr == NULL) continue;
            gf_wake_up_event(sch_mgr->fence_event);
        }
    }
}

int vidschi_try_signal(task_signal_t *signal)
{
    vidsch_sync_object_t *sync_obj = signal->sync_obj;

    int  status = S_OK;

    if(!vidschi_can_submit_task(&signal->desc))
    {
        return E_FAIL;
    }

    switch (sync_obj->type)
    {
    case GF_SYNC_OBJ_TYPE_MUTEX:

        gf_mutex_unlock(sync_obj->mutex.mlock);

        break;

    case GF_SYNC_OBJ_TYPE_SEMPAPHORE:

        gf_up(sync_obj->sema.slock);

        break;

    case GF_SYNC_OBJ_TYPE_FENCE:

        gf_assert(0, "GF_SYNC_OBJ_TYPE_FENCE");

        break;

    case GF_SYNC_OBJ_TYPE_CPU_NOTIFICATION:

        break;

    default:

        break;
    }

    return status;
}

int vidsch_signal_sync_object(gpu_context_t *context, unsigned int hSync)
{
    adapter_t             *adapter  = context->device->adapter;
    vidsch_mgr_t          *sch_mgr  = adapter->sch_mgr[context->engine_index];
    vidsch_sync_object_t  *sync_obj = get_from_handle(&adapter->hdl_mgr, hSync);
    task_signal_t         *signal   = gf_calloc(sizeof(task_signal_t));

    signal->desc.type            = task_type_sync_signal;
    signal->desc.context         = context;
    signal->desc.context_task_id = ++context->task_id;
    signal->desc.task_signal     = signal;
    signal->sync_obj             = sync_obj;

    gf_task_create_trace_event(adapter->index << 16 | context->engine_index, context->handle, signal->desc.context_task_id, signal->desc.type);

    if(vidschi_try_signal(signal) != S_OK)
    {
        vidschi_add_task_to_pending_queue(sch_mgr, &signal->desc);
    }
    else
    {
        context->last_submit_to_sw = signal->desc.context_task_id;
        gf_task_submit_trace_event(adapter->index << 16 | context->engine_index, context->handle, signal->desc.context_task_id, signal->desc.type, 0, 0);
        gf_free(signal);
    }

    /* wakeup all engines worker thread to check pending task queue */
    vidsch_notify_interrupt(adapter, 0);

    return S_OK;
}

int vidsch_fence_value(gpu_device_t *device, gf_fence_value_t *value)
{
    adapter_t             *adapter  = device->adapter;
    vidsch_sync_object_t  *sync_obj = get_from_handle(&adapter->hdl_mgr, value->fence_sync_object);

    if(sync_obj == NULL)
    {
        gf_info("%s, send a invalidate hSyncObject: %x.\n", __func__, value->fence_sync_object);

        return E_INVALIDARG;
    }

    if(value->opcode == GF_GET_FENCE_VALUE)
    {
        value->fence_value  = sync_obj->fence.shared_value;
        value->appid       = sync_obj->fence.appid;
        value->flip_limit   = sync_obj->fence.flip_limit;
        value->discard_sync  =sync_obj->fence.discard;
        if(value->flip_limit > 0)
        {
            sync_trace("get flipcounter pid %d  appid %d flip_limit %d \n", gf_get_current_pid(),value->appid, value->flip_limit);
        }
    }
    else if(value->opcode == GF_SET_FENCE_VALUE)
    {
        if(value->fence_value != 0 && (value->fence_value < sync_obj->fence.shared_value))
        {
            gf_info("pid %d : decrease fence value (%lld -> %lld), maybe a bug...?\n", gf_get_current_pid(), sync_obj->fence.shared_value, value->fence_value);
        }

        sync_obj->fence.discard         = value->discard_sync;
        if (value->fence_value > 0)
        {
            sync_obj->fence.shared_value     = value->fence_value;
        }

        if(value->appid > 0)
        {
            sync_obj->fence.appid        = value->appid;
            sync_obj->fence.flip_limit    = value->flip_limit;
        }

        if(value->flip_limit > 0)
        {
            sync_trace("set flipcounter pid %d  appid %d flip_limit %d \n", gf_get_current_pid(),value->appid, value->flip_limit);
        }
    }
    else
    {
        gf_error("unkonwn Op: %s.\n", __func__);
    }

    return S_OK;
}

int vidsch_is_fence_signaled(adapter_t *adapter, unsigned int hSync, unsigned long long wait_value)
{
    int ret = FALSE;
    vidsch_sync_object_t  *sync_obj = get_from_handle(&adapter->hdl_mgr, hSync);
    if (sync_obj->type == GF_SYNC_OBJ_TYPE_FENCE)
    {
        if (*(volatile unsigned long long *)sync_obj->fence.virt_addr >= wait_value)
        {
            ret = TRUE;
        }
    }
    return ret;
}

void vidsch_sync_obj_inc_reference(adapter_t *adapter, unsigned int hSync)
{
    vidsch_sync_object_t *sync_obj = get_from_handle(&adapter->hdl_mgr, hSync);

    vidschi_sync_obj_inc_reference(sync_obj);
}

void vidsch_sync_obj_dec_reference(adapter_t *adapter, unsigned int hSync)
{
    vidsch_sync_object_t *sync_obj = get_from_handle(&adapter->hdl_mgr, hSync);

    vidschi_sync_obj_dec_reference(sync_obj);
}

void vidsch_force_signal_fence_sync_object(adapter_t *adapter, unsigned int hSync, unsigned long long fence_value)
{
    vidsch_sync_object_t *sync_obj = get_from_handle(&adapter->hdl_mgr, hSync);

    if(sync_obj)
    {
        vidschi_force_signal_fence_sync_object(sync_obj, fence_value);
    }
    else
    {
        gf_error("sync_obj is NULL, hSync = %x.\n", hSync);
    }
}

void vidschi_dump_wait_task(struct os_printer *p, task_wait_t *wait, int idx, int dump_detail)
{
    adapter_t *adapter = wait->desc.context->device->adapter;
    vidsch_sync_object_t   *sync_obj = NULL;
    vidsch_wait_instance_t *instance = NULL;
    int                    index;

    gf_printf(p,"[^^^^  %4d, Dump Wait Task]\n", idx);

    vidschi_dump_general_task_info(p, &wait->desc);

    for(index = 0; index < wait->instance_cnt; index++)
    {
        instance = &wait->instances[index];
        sync_obj = instance->sync_obj;

        if (sync_obj == NULL) continue;

        if(sync_obj->type == GF_SYNC_OBJ_TYPE_FENCE)
        {
            gf_printf(p, "instance[%d], sync obj: %x, instance deltatime: %d, WaitV: %lld, CurrV: %lld.\n",
                index, sync_obj->handle, instance->delta_time, instance->fence_id, *instance->sync_obj->fence.virt_addr);
        }
        else if(sync_obj->type == GF_SYNC_OBJ_TYPE_DMAFENCE)
        {
            adapter->drm_cb->fence.dma_sync_object_dump(sync_obj->dma.dma_sync_obj);
        }
    }
}


