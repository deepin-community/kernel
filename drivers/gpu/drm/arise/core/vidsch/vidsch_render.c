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
#include "vidsch_render.h"
#include "vidsch_submit.h"
#include "vidsch_workerthread.h"
#include "vidsch_sync.h"
#include "vidmm.h"
#include "global.h"
#include "perfevent.h"

/*
** vidschi_can_submit_in_main_thread()
** check task queue empty
** check if touch flip queue
** check if worker thread is disabled
*/
/* this function must not re-entry, guranteed by kernel interface lock */
static int vidschi_can_submit_in_main_thread(adapter_t *adapter, vidsch_mgr_t *sch_mgr, task_dma_t *task_dma)
{
    return CAN_SUBMIT_IN_MAIN_THREAD;
}

void vidschi_emit_dma_fence(vidsch_mgr_t *sch_mgr, task_desc_t *task)
{
    adapter_t  *adapter = sch_mgr->adapter;

    gf_assert(!task->dma_fence, "already emit fence");
    if (adapter->drm_cb)
    {
        task->dma_fence = adapter->drm_cb->fence.create(
                adapter->drm_cb_argu, sch_mgr->engine_index, 0xffffffffffffffffULL);

        if (task->type == task_type_render)
        {
            int                   i;
            task_dma_t            *render              = task->task_render;
            vidsch_allocation_t   *sch_allocation_list = render->sch_allocation_list;
            vidmm_allocation_t    *allocation          = NULL;

            for (i = 0; i < render->allocation_list_size; i++)
            {
                if (sch_allocation_list[i].mm_allocation == NULL) continue;

                allocation = sch_allocation_list[i].mm_allocation;

                if (allocation->bo)
                {
                    adapter->drm_cb->fence.attach_buffer(
                            adapter->drm_cb_argu, allocation->bo,
                            task->dma_fence, sch_allocation_list[i].write_operation ? 0 : 1);

                    if (sch_allocation_list[i].write_operation)
                    {
                        adapter->drm_cb->kms.check_touch_primary(adapter->drm_cb_argu, allocation->bo);
                    }
                }
            }
        }
    }
}

void vidschi_update_and_release_dma_fence(adapter_t *adapter, task_desc_t *task)
{
    if (adapter->drm_cb && task->dma_fence)
    {
        adapter->drm_cb->fence.update_value(
                adapter->drm_cb_argu, task->dma_fence, task->fence_id);

        adapter->drm_cb->fence.release(
                adapter->drm_cb_argu, task->dma_fence);

        task->dma_fence = NULL;
    }
}

void vidschi_notify_dma_fence(adapter_t *adapter)
{
    if (adapter->drm_cb)
    {
        adapter->drm_cb->fence.notify_event(adapter->drm_cb_argu);
    }
}

int vidschi_send_to_submit_queue(vidsch_mgr_t *sch_mgr, task_dma_t *task_dma)
{
    adapter_t  *adapter = sch_mgr->adapter;
    int        status   = S_OK;

    /* if task have hwctx, need check if hwctx invalid, since previous task may submit by worker thread,
     * and thread may encounter problem, like paging failed, which lead task dropped. since hwctx switch
     * task may depend on previous task, so we set hwctx invalid if drop task. and skip all follow task based on this hwctx
     */
    if((task_dma->hw_ctx_info != NULL) && task_dma->hw_ctx_info->is_invalid)
    {
        status =  E_FAIL;
    }

    if(vidschi_can_submit_task(&task_dma->desc) &&
       vidschi_can_submit_in_main_thread(adapter, sch_mgr, task_dma))
    {
        task_dma->desc.context->last_submit_to_sw = task_dma->desc.context_task_id;

        status = vidschi_prepare_and_submit_dma(sch_mgr, task_dma);
    }
    else
    {
        vidschi_add_task_to_pending_queue(sch_mgr, &task_dma->desc);
    }

    return status;
}

static void vidsch_dma_sync_object_trigger(void *arg)
{
    vidsch_mgr_t *sch_mgr = arg;

    util_wakeup_event_thread(sch_mgr->worker_thread);
}


static task_wait_t *vidsch_prepare_render_allocation_list(vidsch_mgr_t *sch_mgr, task_dma_t *render)
{
    adapter_t             *adapter             = sch_mgr->adapter;
    gpu_context_t         *context             = render->desc.context;
    gpu_device_t          *device              = context->device;
    vidsch_allocation_t   *sch_allocation_list = render->sch_allocation_list;
    vidmm_allocation_t    *allocation          = NULL;
    int                   i, engine_index = context->engine_index;
    task_wait_t          *wait_task = NULL;
    vidsch_wait_instance_t *instance = NULL;
    vidsch_sync_object_t  *sync_obj = NULL;

    vidschi_destroy_defer_sync_obj(adapter);

    for (i = 0; i < render->allocation_list_size; i++)
    {
        allocation = sch_allocation_list[i].mm_allocation;
        if (allocation && allocation->bo)
        {
            void *dma_sync_obj = NULL;

            if (adapter->drm_cb)
            {
                dma_sync_obj = adapter->drm_cb->fence.dma_sync_object_create(adapter->drm_cb_argu,
                    allocation->bo, sch_allocation_list[i].write_operation, engine_index, vidsch_dma_sync_object_trigger, sch_mgr);
            }

            if (dma_sync_obj)
            {
                if (!wait_task)
                {
                    wait_task = gf_calloc(sizeof(task_wait_t));

                    gf_get_nsecs(&wait_task->desc.timestamp);
                    gf_atomic_inc(context->ref_cnt);
                    wait_task->desc.context         = context;
                    wait_task->desc.type            = task_type_sync_wait;
                    wait_task->desc.task_wait       = wait_task;
                    wait_task->instance_cnt         = 0;
                    wait_task->only_check_status    = FALSE;
                    wait_task->desc.context_task_id = ++context->task_id;
                    if (render->allocation_list_size <= sizeof(wait_task->instances_builtin)/sizeof(wait_task->instances_builtin[0]))
                    {
                        wait_task->instances        = wait_task->instances_builtin;
                    }
                    else
                    {
                        wait_task->instances        = gf_malloc(render->allocation_list_size * sizeof(*instance));
                    }
                    gf_task_create_trace_event(adapter->index << 16 | engine_index, context->handle, wait_task->desc.context_task_id, wait_task->desc.type);
                }
                sync_obj = gf_calloc(sizeof(vidsch_sync_object_t));
                list_init_head(&sync_obj->instance_list);
                sync_obj->type = GF_SYNC_OBJ_TYPE_DMAFENCE;
                sync_obj->dma.dma_sync_obj = dma_sync_obj;
                sync_obj->ref_cnt = gf_create_atomic(0);
                sync_obj->lock = gf_create_spinlock(0);
                sync_obj->binding = TRUE;
                sync_obj->device = device;

                instance = &wait_task->instances[wait_task->instance_cnt++];
                instance->sync_obj = sync_obj;
                vidschi_sync_obj_add_reference(sync_obj, instance);
                vidschi_add_sync_to_defer_destroy(adapter, sync_obj);
            }
        }
    }

    return wait_task;
}

static void vidschi_init_render_allocation_list(vidsch_mgr_t *sch_mgr, task_dma_t *render)
{
    adapter_t             *adapter             = sch_mgr->adapter;
    gpu_context_t         *context             = render->desc.context;
    gpu_device_t          *device              = context->device;
    vidsch_allocation_t   *sch_allocation_list = render->sch_allocation_list;
    vidmm_allocation_t    *allocation          = NULL;
    gf_allocation_list_t  *allocation_list     = render->allocation_list_raw;
    int                   i, engine_index = context->engine_index;

    cm_device_lock(device);

    for (i = 0; i < render->allocation_list_size; i++)
    {
        if (!allocation_list[i].hAllocation) continue;

        allocation = vidmm_get_from_gem_handle(adapter, device, allocation_list[i].hAllocation);

        if (allocation == NULL) continue;

        gf_spin_lock(allocation->lock);

        allocation->render_count[engine_index]++;

        if(allocation_list[i].WriteOperation)
        {
            gf_counter_trace_event("var_write_allocation", allocation->handle);
            allocation->write_render_count[engine_index]++;
            sch_allocation_list[i].write_operation = 1;
        }
        else
        {
            gf_counter_trace_event("var_read_allocation", allocation->handle);
        }

        gf_spin_unlock(allocation->lock);

        sch_allocation_list[i].mm_allocation = allocation;
    }

    cm_device_unlock(device);
}

static void vidschi_init_render_sync_object_list(vidsch_mgr_t *sch_mgr, task_dma_t *render)
{
    adapter_t                        *adapter       = sch_mgr->adapter;
    vidsch_sync_object_reference_t   *sch_sync_list = render->sync_object_list;
    vidsch_sync_object_t             *sync_obj      = NULL;
    gf_syncobj_list_t                *sync_list     = render->sync_object_list_raw;

    int i;

    for(i = 0; i < render->sync_object_list_size; i++)
    {
        sync_obj = get_from_handle(&adapter->hdl_mgr, sync_list[i].hSyncObject);

        if(sync_obj == NULL) continue;

        vidschi_sync_obj_inc_reference(sync_obj);

        sch_sync_list[i].sync_object  = sync_obj;
        sch_sync_list[i].patch_offset = sync_list[i].PatchOffset;
        sch_sync_list[i].fence_value  = sync_list[i].FenceValue;

        if (adapter->ctl_flags.perf_event_enable)
        {
            gf_perf_event_t      perf_event = {0};
            unsigned long long    timestamp  = 0;
            gf_memset(&perf_event, 0, sizeof(gf_perf_event_t));

            gf_get_nsecs(&timestamp);
            perf_event.header.timestamp_high = timestamp >> 32;
            perf_event.header.timestamp_low  = (timestamp) & 0xffffffff;

            perf_event.header.size = sizeof(gf_perf_event_sync_event_t);
            perf_event.header.type = GF_PERF_EVENT_SYNC_EVENT;

            perf_event.sync_event.engine_idx         = render->engine_index;
            perf_event.sync_event.type               = sync_obj->type;
            perf_event.sync_event.handle             = sync_obj->handle;
            perf_event.sync_event.fence_value_high   = sch_sync_list[i].fence_value >> 32;
            perf_event.sync_event.fence_value_low    = sch_sync_list[i].fence_value & 0xffffffff;
            perf_event.sync_event.gpu_context        = render->desc.context->handle;
            perf_event.sync_event.ctx_task_id_high   = render->desc.context_task_id >> 32;
            perf_event.sync_event.ctx_task_id_low    = render->desc.context_task_id & 0xffffffff;

            perf_event_add_event(adapter, &perf_event);
        }
    }
}

void vidschi_deinit_render_sync_object_list(vidsch_mgr_t *sch_mgr, task_dma_t *render)
{
    vidsch_sync_object_reference_t   *sch_sync_list = render->sync_object_list;
    vidsch_sync_object_t             *sync_obj      = NULL;

    int i, force_signal;

    force_signal = render->desc.reset_dropped|render->desc.fake_submitted;

    for(i = 0; i < render->sync_object_list_size; i++)
    {
        sync_obj = sch_sync_list[i].sync_object;

        if(sync_obj == NULL) continue;

        if(force_signal)
        {
            gf_info("render task: %p, force signal object[%2d]: %x.\n", render, i, sync_obj->handle);

            vidschi_force_signal_fence_sync_object(sync_obj, sch_sync_list[i].fence_value);
        }

        vidschi_sync_obj_dec_reference(sync_obj);
    }
}

int vidsch_render(gpu_context_t *context, vidsch_render_t *render_data)
{
    adapter_t                   *adapter  = context->device->adapter;
    vidsch_mgr_t                *sch_mgr  = adapter->sch_mgr[context->engine_index];
    task_dma_t                  *task_dma = 0;
    task_wait_t                 *task_wait = NULL;
    vidsch_allocate_task_dma_t  task_arg  = {0};
    int                         status = 0;
    gf_cmdbuf_t                 *cmdbuf_array = NULL;

    if (render_data->command_length == 0)
    {
        return S_OK;
    }

    gf_begin_section_trace_event("vidsch_render");

    if (adapter->ctl_flags.perf_event_enable)
    {
        gf_perf_event_t            perf_event = {0};
        unsigned long long          timestamp = 0;

        perf_event.header.size = sizeof(gf_perf_event_dma_buffer_queued_t);
        perf_event.header.type = GF_PERF_EVENT_DMA_BUFFER_QUEUED;
        perf_event.header.pid = gf_get_current_pid();
        perf_event.header.tid = gf_get_current_tid();

        gf_get_nsecs(&timestamp);
        perf_event.header.timestamp_high = timestamp >> 32;
        perf_event.header.timestamp_low = timestamp & 0xffffffff;

        perf_event.dma_buffer_queued.gpu_context = context->handle;
        perf_event.dma_buffer_queued.engine_idx = context->engine_index;
        perf_event.dma_buffer_queued.dma_idx_low = render_data->render_counter & 0xffffffff;
        perf_event.dma_buffer_queued.dma_idx_high = (render_data->render_counter >> 32) & 0xffffffff;

        perf_event_add_event(adapter, &perf_event);
    }

    task_arg.context               = context;
    task_arg.command_length        = render_data->command_length;
    task_arg.allocation_count      = render_data->allocation_list_size;
    task_arg.patch_location_count  = render_data->patch_location_list_size;
    task_arg.sync_object_count     = render_data->sync_object_list_size;

    cmdbuf_array = render_data->cmdbuf_array;
    task_arg.Flags = render_data->flags;

    //check the cmd is normal Xserver 2D command or CompositeBlt command
    // if compositblt, the dmatpye set to DRAW_3D_BLT
    // first check Flag3dbltCmd, since hwc maybe set Flag3dbltCmd & Flag2dCmd both.
    if (render_data->flags.Flag3dbltCmd)
    {
        task_arg.dma_type = DRAW_3D_BLT;
    }
    else if (render_data->flags.Flag2dCmd)
    {
        task_arg.dma_type = DRAW_2D_DMA;
    }
    else
    {
        task_arg.dma_type = DRAW_3D_DMA;
    }

    task_dma = vidsch_allocate_task_dma(adapter, context->engine_index, &task_arg);

    if (context->is_kernel)
    {
        if (render_data->allocation_list_size > 0)
            gf_memcpy(task_dma->allocation_list_raw, render_data->allocation_list,
                    render_data->allocation_list_size * sizeof(gf_allocation_list_t));

        if (render_data->patch_location_list_size > 0)
            gf_memcpy(task_dma->patch_location_list, render_data->patch_location_list,
                    render_data->patch_location_list_size * sizeof(gf_patchlocation_list_t));

        if (render_data->sync_object_list_size > 0)
            gf_memcpy(task_dma->sync_object_list_raw, render_data->sync_object_list,
                    render_data->sync_object_list_size * sizeof(gf_syncobj_list_t));
    }
    else
    {
        if (render_data->allocation_list_size > 0)
            gf_copy_from_user(task_dma->allocation_list_raw, render_data->allocation_list,
                    render_data->allocation_list_size * sizeof(gf_allocation_list_t));

        if (render_data->patch_location_list_size > 0)
            gf_copy_from_user(task_dma->patch_location_list, render_data->patch_location_list,
                    render_data->patch_location_list_size * sizeof(gf_patchlocation_list_t));

        if (render_data->sync_object_list_size > 0)
            gf_copy_from_user(task_dma->sync_object_list_raw, render_data->sync_object_list,
                    render_data->sync_object_list_size * sizeof(gf_syncobj_list_t));
    }

    vidschi_init_render_allocation_list(sch_mgr, task_dma);

    vidschi_init_render_sync_object_list(sch_mgr, task_dma);

    task_wait = vidsch_prepare_render_allocation_list(sch_mgr, task_dma);

    if (task_wait)
    {
        /* swap task_dma & task_wait's task_id */
        unsigned long long temp = task_wait->desc.context_task_id;
        task_wait->desc.context_task_id = task_dma->desc.context_task_id;
        task_dma->desc.context_task_id = temp;

        /* always add to pending queue, even if client_wait, otherwise last_submit_to_sw will incorrect */
        if (adapter->ctl_flags.worker_thread_enable)
        {
            vidschi_add_task_to_pending_queue(sch_mgr, &task_wait->desc);
        }
        else
        {
            vidschi_client_wait(task_wait);
            context->last_submit_to_sw = task_wait->desc.context_task_id;
            vidschi_release_wait_task(task_wait);
        }
    }

    if(task_dma->need_hwctx_switch)
    {
        task_dma->hw_ctx_info = cm_get_hwctx_buffer(context->device, context, render_data->flags.HWCtxBufIndex);
    }

    do
    {
        int i;
        char *dst = task_dma->dma_buffer_node->virt_base_addr;

        for (i = 0; i < render_data->cmdbuf_count; i++)
        {
            if (context->is_kernel)
            {
                gf_memcpy(dst, (void*)(unsigned long)cmdbuf_array[i].ptr, cmdbuf_array[i].size);
            }
            else
            {
                gf_copy_from_user(dst, (void*)(unsigned long)cmdbuf_array[i].ptr, cmdbuf_array[i].size);
            }
            dst += cmdbuf_array[i].size;
        }
        gf_assert(dst - (char*)task_dma->dma_buffer_node->virt_base_addr == render_data->command_length, "incorrect command_length");
    }
    while(0);

    task_dma->render_counter = render_data->render_counter;

    sch_mgr->chip_func->render(context, task_dma);

    task_dma->desc.need_dma_fence = 1;

    gf_wmb();

    status = vidschi_send_to_submit_queue(sch_mgr, task_dma);

    gf_end_section_trace_event(status);

    return status;
}

