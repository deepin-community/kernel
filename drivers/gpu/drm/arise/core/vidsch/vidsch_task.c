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
#include "vidsch_submit.h"

#if 0
static int vidschi_update_fence_id_temp(vidsch_mgr_t *sch_mgr, unsigned long long *curr_returned)
{
    unsigned long long new_fence, old_fence;
    unsigned long      flags;

    int  update_new = FALSE;

    flags = gf_spin_lock_irqsave(sch_mgr->fence_lock);

    new_fence = sch_mgr->chip_func->update_fence_id(sch_mgr);

    old_fence = sch_mgr->returned_fence_id;

    /* check if new_returned valid, if not skipped it */
    if(((new_fence >  sch_mgr->returned_fence_id) &&
       (new_fence <= sch_mgr->last_send_fence_id)) &&
       (old_fence <= sch_mgr->last_send_fence_id))
    {
        sch_mgr->returned_fence_id   = new_fence;
        sch_mgr->uncomplete_task_num = sch_mgr->last_send_fence_id - sch_mgr->returned_fence_id;

        update_new = TRUE;
    }
    else
    {
        new_fence = sch_mgr->returned_fence_id;
    }

    gf_spin_unlock_irqrestore(sch_mgr->fence_lock, flags);

    if(new_fence > old_fence)
    {
        gf_get_nsecs(&sch_mgr->returned_timestamp);
    }

    *curr_returned = new_fence;

    return update_new;
}

static void vidschi_dump_task_list(vidsch_mgr_t *sch_mgr, struct list_head *task_list)
{
    task_desc_t *task = NULL;

    int i = 0;

    gf_mutex_lock(sch_mgr->task_list_lock);

    list_for_each_entry(task, task_list, list_item)
    {
        gf_info("task[%02d]: %p, type: %d, cmd_size: %8d, fence_id: %lld. submitted: %d, fake_submit: %d.\n",
            i++, task, task->type, task->cmd_size, task->fence_id, task->submitted, task->fake_submitted);
        gf_info("--------------- context: %p, time: %d, task_id:  %lld.\n",
            task->context, (unsigned int)gf_do_div(task->timestamp, 1000000), task->context_task_id);
    }

    gf_mutex_unlock(sch_mgr->task_list_lock);
}

static void vidschi_dump_dma_info(vidsch_mgr_t *sch_mgr)
{
    heap_t *dma_heap = sch_mgr->dma_heap;

    gf_info("********engine: %d, DMA info\n********************\n", sch_mgr->engine_index);

    heap_dump(dma_heap);

    gf_info("--------dump submitted task list: \n");

    vidschi_dump_task_list(sch_mgr, &sch_mgr->submitted_task_list);

    gf_info("--------dump allocated task list: \n");

    vidschi_dump_task_list(sch_mgr, &sch_mgr->allocated_task_list);

    gf_info("----------------------------------------------------\n");
}
#endif

static void vidschi_dump_allocation_member(struct os_printer *p , int index, vidmm_allocation_t *allocation)
{
    gpu_device_t  *device = allocation->device;
    adapter_t * adapter = device->adapter;

    gf_printf(p, "allo[%03d]: %x, dev: %x, fmt: %8d-%8d-%d-%2d, W-H-P: %8d-%8d-%8d, size: %8dk, gvm: %llx, AT:0x%02x, status %8x, pid:%d, tid: %d, proc:%s.\n",
        index, allocation->handle, allocation->device->handle,
        allocation->hw_format, allocation->compress_format, allocation->flag.swizzled, allocation->bit_count,
        allocation->width, allocation->height, allocation->pitch,
        util_align(allocation->orig_size, util_max(allocation->alignment, adapter->os_page_size)) >> 10,
        allocation->phys_addr, allocation->at_type, allocation->status.temp_unpagable,
        allocation->device->pid, allocation->device->tid, allocation->device->pname);
}

void vidschi_dump_general_task_info(struct os_printer *p, task_desc_t *task)
{
    gpu_context_t       *context    = task->context;
    gpu_device_t        *device     = context->device;

    gf_printf(p,"TaskId: %lld, GlobalTaskId:%lld, ContextLastSubmitted: %lld. Device: %x, Context: %x, Engine: %d.\n",
        task->context_task_id, task->global_task_id, context->last_submit_to_sw, device->handle, context->handle, context->engine_index);
    gf_printf(p,"Submitted By Process: PID: %d, TID: %d, %s.\n",
        context->pid, context->tid, context->pname);
}

static void vidschi_dump_render_task(struct os_printer *p, adapter_t * adapter, task_dma_t *render, int idx, int dump_detail)
{
    vidmm_allocation_t  *allocation = NULL;

    unsigned long long  hwctx_addr  = (render->hw_ctx_info != NULL) ?
                                      render->hw_ctx_info->context_buffer_address : 0;

    int i = 0;

    gf_printf(p,"[^^^^  %4d, Dump Render Task]\n", idx);

    vidschi_dump_general_task_info(p,&render->desc);

    gf_printf(p,"RenderType: %d, FenceID: %lld, hwctx addr: 0x%x. allocations: %d, patchs: %d.\n",
            render->dma_type, render->desc.fence_id, hwctx_addr,
            render->allocation_list_size, render->patch_location_list_size);

    if(!adapter->display_debugbus_flag) return;
    gf_msleep(10);

    for(i = 0; i < render->allocation_list_size; i++)
    {
        allocation = render->sch_allocation_list[i].mm_allocation;

        if(allocation == NULL) continue;

        vidschi_dump_allocation_member(p, i, allocation);

        if(dump_detail && (allocation->at_type == 18 || //MM_AT_EUPFSHADER_E3K
           allocation->at_type == 19 || //MM_AT_EUPBSHADER_E3K
           allocation->at_type == 22))  //MM_AT_CONTEXT_E3K
        {
            vidmm_dump_allocation_content(allocation);
        }

        if(dump_detail && allocation->compress_format)
        {
            vidmm_dump_flagbuffer_to_file(NULL, allocation);
        }
    }

    if(!dump_detail) return;

    gf_msleep(20);

    if(render->dma_buffer_node != NULL)
    {
        util_dump_memory(p, render->dma_buffer_node->virt_base_addr, render->command_length, "Dma");

        gf_msleep(10);
    }
}

static void vidschi_dump_paging_task(struct os_printer *p, adapter_t *adapter, task_paging_t *paging, int idx, int dump_detail)
{
    vidmm_allocation_t        *allocation        = NULL;
    vidmm_paging_allocation_t *paging_allocation = NULL;

    int i = 0;

    gf_printf(p, "[^^^^  %4d, Dump Paging Task]: Type: %d, AllocationNum: %d, FenceId: %d.\n",
         idx, paging->paging_type, paging->paging_allocation_list_size, paging->desc.fence_id);

    for(i = 0; i < paging->paging_allocation_list_size; i++)
    {
        paging_allocation = &paging->paging_allocation_list[i];
        allocation        = paging_allocation->allocation;

        if(allocation == NULL) continue;

        gf_printf(p, "addr:%x, segment_id:%d, temp_addr:%x\n", paging_allocation->gpu_virt_addr, paging_allocation->segment_id,
                paging_allocation->temp_allocation != NULL ? paging_allocation->temp_allocation->phys_addr : 0);

        vidschi_dump_allocation_member(p, i, allocation);

        if(dump_detail && (allocation->at_type == 18 || //MM_AT_EUPFSHADER_E3K
           allocation->at_type == 19 || //MM_AT_EUPBSHADER_E3K
           allocation->at_type == 22))  //MM_AT_CONTEXT_E3K
        {
            vidmm_dump_allocation_content(allocation);
        }

        if(dump_detail && allocation->compress_format)
        {
            vidmm_dump_flagbuffer_to_file(NULL, allocation);
        }
    }

    if(!dump_detail)  return;

    gf_msleep(20);

    if(paging->dma != NULL)
    {
        util_dump_memory(p, paging->dma->virt_base_addr, paging->command_length, "Dma");

        gf_msleep(10);
    }
}

void vidschi_dump_task(struct os_printer *p, adapter_t *adapter, task_desc_t *task, int idx, int dump_detail)
{
    switch (task->type)
    {
    case task_type_render:
        vidschi_dump_render_task(p, adapter, task->task_render, idx, dump_detail);
        break;

    case task_type_sync_wait:
        vidschi_dump_wait_task(p, task->task_wait, idx, dump_detail);
        break;

    case task_type_sync_signal:
        gf_printf(p, "signal task.\n");
        break;

    case task_type_paging:
        vidschi_dump_paging_task(p, adapter, task->task_paging, idx, dump_detail);
        break;

    default:
        gf_error("Unknown Task type: %d.\n", task->type);
        break;
    }
}

 dma_node_t *vidschi_allocate_dma_node(vidsch_mgr_t *sch_mgr, unsigned int dma_size)
{
    vidmm_segment_memory_t  *dma_reserved_memory = sch_mgr->dma_reserved_memory;
    heap_t                  *dma_heap            = sch_mgr->dma_heap;
    dma_node_t              *dma                 = gf_calloc(sizeof(dma_node_t));
    int                     sleep_cnt            = 0;

    while(dma->list_node == NULL)
    {
        unsigned long long  uncomplete_task = 0ll;

        vidsch_release_completed_tasks(sch_mgr->adapter, sch_mgr->engine_index, &uncomplete_task);

        dma->list_node = heap_allocate(dma_heap, dma_size, 0, 0);

        if (dma->list_node == NULL)
        {
            if(uncomplete_task == 0ll)
            {
                if(sleep_cnt > 500)
                {
                    gf_info("use 500 * 20ms try allocate dma (size: %d) failed.\n", dma_size);

                    //vidschi_dump_dma_info(sch_mgr);

                    gf_info("try another 500 * 20ms to allocate dma.\n");

                    sleep_cnt = 0;
                }

                gf_msleep(20);

                sleep_cnt++;
            }
            else
            {
                sleep_cnt = 0;

                vidsch_wait_fence_back(sch_mgr->adapter, sch_mgr->engine_index, uncomplete_task);
            }
        }
    }

    dma->gpu_phy_base_addr = dma->list_node->aligned_offset;
    dma->virt_base_addr    = (unsigned char *)dma_reserved_memory->vma->virt_addr + dma->list_node->aligned_offset - dma_reserved_memory->gpu_virt_addr;
    dma->size              = dma->list_node->aligned_size;
    dma->cmd_size          = dma_size;

    //gf_info("allocate dma:%p gvirt_addr: %x, size: %d, request_size: %d.\n",
    //    dma, dma->gpu_phy_base_addr, dma->size, dma->cmd_size);

    return dma;
}

static inline void vidschi_release_dma_node(vidsch_mgr_t *sch_mgr, dma_node_t *dma)
{
    //gf_info("release dma:%p gvirt_addr: %x, size: %d, request_size: %d.\n",
    //    dma, dma->gpu_phy_base_addr, dma->size, dma->cmd_size);

    heap_release(sch_mgr->dma_heap, dma->list_node);

    gf_free(dma);
}

static inline void vidschi_init_task_desc(vidsch_mgr_t *sch_mgr, task_desc_t *task, gpu_context_t *context, int task_type, int cmd_size)
{
    task->context  = context;
    task->type     = task_type;
    task->cmd_size = cmd_size;

    if(context != NULL)
    {
        task->context_task_id = ++context->task_id;

        gf_atomic_inc(context->ref_cnt);
    }

    gf_mutex_lock(sch_mgr->task_list_lock);

    list_add_tail(&task->list_item, &sch_mgr->allocated_task_list);

    gf_mutex_unlock(sch_mgr->task_list_lock);

    gf_get_nsecs(&task->timestamp);
}

static inline void vidschi_deinit_task_desc(vidsch_mgr_t *sch_mgr, task_desc_t *task, int list_locked)
{
    gpu_context_t *context = task->context;

    if(context != NULL)
    {
        gf_atomic_dec(context->ref_cnt);
        task->context = NULL;
    }

    if(!list_locked) gf_mutex_lock(sch_mgr->task_list_lock);

    list_del(&task->list_item);

    if(!list_locked) gf_mutex_unlock(sch_mgr->task_list_lock);
}

task_dma_t *vidsch_allocate_task_dma(adapter_t *adapter, unsigned int engine_index, vidsch_allocate_task_dma_t *dma_arg)
{
    unsigned int          offset           = 0;
    char                  *memory          = NULL;
    task_dma_t            *task_dma        = NULL;
    unsigned int          allocations_raw_size = dma_arg->allocation_count * sizeof(gf_allocation_list_t);
    unsigned int          allocations_size = dma_arg->allocation_count * sizeof(vidsch_allocation_t);
    unsigned int          patchs_size      = dma_arg->patch_location_count * sizeof(gf_patchlocation_list_t);
    unsigned int          syncs_raw_size   = dma_arg->sync_object_count * sizeof(gf_syncobj_list_t);
    unsigned int          syncs_size       = dma_arg->sync_object_count * sizeof(vidsch_sync_object_reference_t);
    vidsch_mgr_t          *vidsch          = adapter->sch_mgr[engine_index];

    unsigned int total_size = util_align(sizeof(task_dma_t), 16) +
                              util_align(allocations_raw_size, 16) +
                              util_align(syncs_raw_size, 16) +
                              util_align(allocations_size, 16) +
                              util_align(patchs_size, 16) +
                              util_align(syncs_size, 16);

    memory = gf_calloc(total_size);
    if(memory == NULL)
    {
        gf_error("func vidsch_allocate_task_dma allocated memory failed, size:%x.\n", total_size);
    }

    task_dma = (task_dma_t *)(memory + offset);
    offset += util_align(sizeof(task_dma_t), 16);

    if(dma_arg->allocation_count)
    {
        task_dma->allocation_list_raw   = (gf_allocation_list_t*)(memory + offset);
        offset += util_align(allocations_raw_size, 16);

        task_dma->sch_allocation_list   = (vidsch_allocation_t *)(memory + offset);
        task_dma->allocation_list_size  = dma_arg->allocation_count;
        offset += util_align(allocations_size, 16);
    }

    if(dma_arg->patch_location_count)
    {
        task_dma->patch_location_list      = (gf_patchlocation_list_t*)(memory + offset);
        task_dma->patch_location_list_size = dma_arg->patch_location_count;
        task_dma->patch_start_offset       = 0;
        task_dma->patch_end_offset         = dma_arg->patch_location_count;
        offset += util_align(patchs_size, 16);
    }

    if(dma_arg->sync_object_count)
    {
        task_dma->sync_object_list_raw     = (gf_syncobj_list_t*)(memory + offset);
        offset += util_align(syncs_raw_size, 16);

        task_dma->sync_object_list         = (vidsch_sync_object_reference_t *)(memory + offset);
        task_dma->sync_object_list_size    = dma_arg->sync_object_count;

        offset += util_align(syncs_size, 16);
    }

    gf_assert(offset == total_size, "offset == total_size");

    task_dma->dma_buffer_node = vidschi_allocate_dma_node(vidsch, dma_arg->command_length);

    task_dma->command_length      = dma_arg->command_length;
    task_dma->submit_start_offset = 0;
    task_dma->submit_end_offset   = dma_arg->command_length;
    task_dma->dma_type            = dma_arg->dma_type;
    task_dma->hw_ctx_info         = NULL;
    task_dma->need_hwctx_switch   = (dma_arg->dma_type == DRAW_3D_DMA) ? TRUE:FALSE;
    task_dma->desc.task_render    = task_dma;

    /* if hw engine no hw ctx, then no need hwctx swicth */
    if(vidsch->engine_caps & ENGINE_CAPS_NO_HWCTX)
    {
        task_dma->need_hwctx_switch = FALSE;
    }

    vidschi_init_task_desc(vidsch, &task_dma->desc, dma_arg->context, task_type_render, dma_arg->command_length);
    task_dma->desc.Flags = dma_arg->Flags;

    gf_task_create_trace_event(adapter->index << 16 | dma_arg->context->engine_index, dma_arg->context->handle, task_dma->desc.context_task_id, task_dma->desc.type);

    return task_dma;
}

static void vidschi_release_task_dma(vidsch_mgr_t *sch_mgr, task_dma_t *render)
{
    vidschi_deinit_render_sync_object_list(sch_mgr, render);

    vidschi_release_dma_node(sch_mgr, render->dma_buffer_node);

    render->dma_buffer_node = NULL;

    gf_free(render);
}

task_paging_t *vidsch_allocate_paging_task(adapter_t *adapter, int dma_size, int allocation_num)
{
    vidsch_mgr_t  *paging_engine = adapter->sch_mgr[adapter->paging_engine_index];
    task_paging_t *paging_task   = NULL;

    unsigned int   total_size = util_align(sizeof(task_paging_t), 16) +
                                util_align(sizeof(vidmm_paging_allocation_t) * allocation_num, 16);

    paging_task = gf_calloc(total_size);

    paging_task->paging_allocation_list      = (vidmm_paging_allocation_t *)((char *)paging_task + util_align(sizeof(task_paging_t), 16));
    paging_task->paging_allocation_list_size = allocation_num;
    paging_task->dma                         = vidschi_allocate_dma_node(paging_engine, dma_size);
    paging_task->desc.hang_index  = -1;
    paging_task->desc.task_paging = paging_task;

    vidschi_init_task_desc(paging_engine, &paging_task->desc, NULL, task_type_paging, dma_size);

    gf_task_create_trace_event(adapter->index << 16 | paging_engine->engine_index, 0, ptr_to_ptr64(&paging_task->desc), paging_task->desc.type);

    return paging_task;
}

static void vidschi_release_paging_task(vidsch_mgr_t *paging_engine, task_paging_t *paging_task)
{
    adapter_t *adapter = paging_engine->adapter;

    vidschi_release_dma_node(paging_engine, paging_task->dma);

    vidmm_release_temp_paging_memory(adapter, paging_task);

    gf_free(paging_task);
}


static inline void vidschi_do_release_task(vidsch_mgr_t *sch_mgr, task_desc_t *task)
{
    switch (task->type)
    {
    case task_type_render:

        vidschi_release_task_dma(sch_mgr, task->task_render);
        break;

    case task_type_paging:

        vidschi_release_paging_task(sch_mgr, task->task_paging);
        break;

    default:
        gf_error("SOMETHING WRONG task: %d, can not put in dma submit list.\n", task->type);
        break;
   }
}

void vidschi_set_dma_task_fake_submitted(vidsch_mgr_t *sch_mgr, task_desc_t *task)
{
#if 0
    dma_node_t *dma = NULL;
    switch (task->type)
    {
    case task_type_render:

        dma = task->task_render->dma_buffer_node;
        break;

    case task_type_paging:

        dma = task->task_paging->dma;
        break;

    default:
        break;
    }

    if(dma != NULL)
    {
        gf_info("fake sub dma:%p gvirt_addr: %x, size: %d, request_size: %d. task: %d\n",
            dma, dma->gpu_phy_base_addr, dma->size, dma->cmd_size, task->type);
    }
#endif

    task->fake_submitted = TRUE;

    if(task->ref_cnt == 0)
    {
        vidschi_deinit_task_desc(sch_mgr, task, FALSE);

        vidschi_do_release_task(sch_mgr, task);
    }
}

void vidschi_set_dma_task_submitted(vidsch_mgr_t *sch_mgr, task_desc_t *task)
{
#if 0
    dma_node_t *dma = NULL;

    switch (task->type)
    {
    case task_type_render:

        dma = task->task_render->dma_buffer_node;
        break;

    case task_type_paging:

        dma = task->task_paging->dma;
        break;

    default:
        break;
    }

    if(dma != NULL)
    {
        gf_info("submitted dma:%p gvirt_addr: %x, size: %d, request_size: %d. task: %d\n",
            dma, dma->gpu_phy_base_addr, dma->size, dma->cmd_size, task->type);
    }
#endif

    /* update submit timestamp */
    gf_get_nsecs(&task->timestamp);

    task->submitted = TRUE;

    gf_mutex_lock(sch_mgr->task_list_lock);

    list_del(&task->list_item);

    list_add_tail(&task->list_item, &sch_mgr->submitted_task_list);

    gf_mutex_unlock(sch_mgr->task_list_lock);
}

void vidsch_task_inc_reference(adapter_t *adapter, int engine_index, task_desc_t *desc)
{
    vidsch_mgr_t *sch_mgr = adapter->sch_mgr[engine_index];

    gf_mutex_lock(sch_mgr->task_list_lock);

    desc->ref_cnt++;

    gf_mutex_unlock(sch_mgr->task_list_lock);
}

void vidsch_task_dec_reference(adapter_t *adapter, int engine_index, task_desc_t *task)
{
    vidsch_mgr_t *sch_mgr = adapter->sch_mgr[engine_index];

    int do_release_task = FALSE;

    gf_mutex_lock(sch_mgr->task_list_lock);

    task->ref_cnt--;

    if((task->type == task_type_paging) && task->fake_submitted && (task->ref_cnt == 0))
    {
        do_release_task = TRUE;
    }

    gf_mutex_unlock(sch_mgr->task_list_lock);

    if(do_release_task)
    {
        vidschi_deinit_task_desc(sch_mgr, task, FALSE);

        vidschi_do_release_task(sch_mgr, task);
    }
    else
    {
        vidsch_release_completed_tasks(adapter, engine_index, NULL);
    }
}

void vidsch_release_completed_tasks(adapter_t *adapter, int engine_index, unsigned long long *uncompleted_dma)
{
    vidsch_mgr_t  *sch_mgr = adapter->sch_mgr[engine_index];
    task_desc_t   *task;

try_next:

    task = NULL;

    gf_mutex_lock(sch_mgr->task_list_lock);

    if(!list_empty(&sch_mgr->submitted_task_list))
    {
        task = list_entry(sch_mgr->submitted_task_list.next, task_desc_t, list_item);

        if((task->ref_cnt == 0) && vidsch_is_fence_back(sch_mgr->adapter, sch_mgr->engine_index, task->fence_id))
        {
            vidschi_deinit_task_desc(sch_mgr, task, TRUE);
        }
        else
        {
            if(task->timestamp < sch_mgr->returned_timestamp)
            {
                task->timestamp = sch_mgr->returned_timestamp;
            }

            if(uncompleted_dma != NULL)
            {
                *uncompleted_dma = task->fence_id;
            }

            task = NULL;
        }
    }

    gf_mutex_unlock(sch_mgr->task_list_lock);

    if(task != NULL)
    {
        vidschi_do_release_task(sch_mgr, task);

        goto try_next;
    }
}

void vidschi_release_allocated_tasks(vidsch_mgr_t *sch_mgr)
{
    task_desc_t   *task;

try_next:

    task = NULL;

    gf_mutex_lock(sch_mgr->task_list_lock);

    if(!list_empty(&sch_mgr->allocated_task_list))
    {
        task = list_entry(sch_mgr->allocated_task_list.next, task_desc_t, list_item);

        vidschi_deinit_task_desc(sch_mgr, task, TRUE);
    }

    gf_mutex_unlock(sch_mgr->task_list_lock);

    if(task != NULL)
    {
        vidschi_do_release_task(sch_mgr, task);

        goto try_next;
    }
}

task_desc_t *vidschi_uncompleted_task_exceed_wait_time(vidsch_mgr_t *sch_mgr, unsigned long long max_wait_time)
{
    adapter_t   *adapter = sch_mgr->adapter;
    task_desc_t *task    = NULL;
    int         exceed   = FALSE;
    static  unsigned long long fence_timeout_cnt = 0;
    unsigned long long wait_time = 0;
#if 0
    unsigned long long returned_fence_id;
    unsigned int new_fence = 0;
    new_fence = vidschi_update_fence_id_temp(sch_mgr, &returned_fence_id);

    if(new_fence)
    {

        gf_wake_up_event(sch_mgr->fence_event);

        if((sch_mgr->emergency_task_pool.task_num > 0) ||
           (sch_mgr->normal_task_pool.task_num > 0))
        {
            util_wakeup_event_thread(sch_mgr->worker_thread);
        }
    }
#endif
    /* first release completed task in queue */
    vidsch_release_completed_tasks(adapter, sch_mgr->engine_index, NULL);

    gf_mutex_lock(sch_mgr->task_list_lock);

    if(!list_empty(&sch_mgr->submitted_task_list))
    {
        unsigned long long curr_time  = 0ll;

        task = list_entry(sch_mgr->submitted_task_list.next, task_desc_t, list_item);
        wait_time = task->Flags.normal_recovery ? max_wait_time : adapter->hw_hang_fast_timeout_ns;

        gf_get_nsecs(&curr_time);

        /* NOTE: all timestamp is unsigned value, so if x - y < 0, since the type it minus value still a
         * unsigned which will be big vale, So not use if(x - y > delta) but if(x + delta > y) to avoid overflow issue
         */

        if((task->timestamp + wait_time) < curr_time && sch_mgr->returned_fence_id < task->fence_id)
        {

            unsigned long long  new_fence_id = sch_mgr->chip_func->update_fence_id(sch_mgr);

            if(new_fence_id >= task->fence_id){

                fence_timeout_cnt++;

                if ((fence_timeout_cnt <= 10) ||(adapter->debugfs_mask & GF_DEBUGFS_FAKE_HANG))
                {
                    gf_info("fence timeout: engine[%d], fence_id:%lld, dma_size: %d. used_time: %dms, submit_time: %dms, curr_time: %dms, returned_fence_id-%lld, new_fence_id-%lld\n",
                             sch_mgr->engine_index,
                             task->fence_id,
                             task->cmd_size,
                             (unsigned int)gf_do_div(curr_time - task->timestamp, 1000000),
                             (unsigned int)gf_do_div(task->timestamp, 1000000),
                             (unsigned int)gf_do_div(curr_time, 1000000),
                             sch_mgr->returned_fence_id,
                             new_fence_id);
                }
                vidsch_notify_interrupt(adapter, 0);
                /*patch lost interrupt*/
                gf_disable_interrupt(adapter->os_device.pdev);
                gf_enable_interrupt(adapter->os_device.pdev);
           }else {
               gf_info("timeout detect: engine[%d], fence_id:%lld, dma_size: %d. used_time: %dms, submit_time: %dms, curr_time: %dms, returned_fence_id-%lld, new_fence_id-%lld\n",
                        sch_mgr->engine_index,
                        task->fence_id,
                        task->cmd_size,
                        (unsigned int)gf_do_div(curr_time - task->timestamp, 1000000),
                        (unsigned int)gf_do_div(task->timestamp, 1000000),
                        (unsigned int)gf_do_div(curr_time, 1000000),
                        sch_mgr->returned_fence_id,
                        new_fence_id);

               exceed = TRUE;
               task->ref_cnt++;
           }
       }
    }

    gf_mutex_unlock(sch_mgr->task_list_lock);

    return exceed ? task : NULL;
}

void vidschi_set_uncompleted_task_dropped(vidsch_mgr_t *sch_mgr, unsigned long long dropped_task)
{
    adapter_t   *adapter = sch_mgr->adapter;
    task_desc_t *task = NULL;

    gf_mutex_lock(sch_mgr->task_list_lock);

    list_for_each_entry(task, &sch_mgr->submitted_task_list, list_item)
    {
        if(task->fence_id <= dropped_task)
        {
            task->reset_dropped = TRUE;

            if(adapter->display_debugbus_flag)
            {
                gf_info("task: %p, cmd_size: %d, fence_id: %lld, type: %d, dropped.\n",
                    task, task->cmd_size, task->fence_id, task->type);
            }

            if(task->type == task_type_render)
            {
                task_dma_t    *render  = task->task_render;
                gpu_context_t *context = task->context;

                if(adapter->display_debugbus_flag)
                {
                    gf_info("task: %p, context: %x, tid: %d, allocations: %d, patch locations: %d, sync objects: %d.\n",
                        render, context->handle, context->tid,
                        render->allocation_list_size, render->patch_location_list_size, render->sync_object_list_size);
                }
            }
        }
    }

    gf_mutex_unlock(sch_mgr->task_list_lock);
}


