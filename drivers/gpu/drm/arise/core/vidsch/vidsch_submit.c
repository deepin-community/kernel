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
#include "vidmm.h"
#include "vidsch_submit.h"
#include "perfevent.h"

static int vidschi_submit_dma(vidsch_mgr_t *sch_mgr, task_dma_t *task_dma)
{
    adapter_t               *adapter       = sch_mgr->adapter;
    vidschedule_t           *schedule      = adapter->schedule;
    gpu_context_t           *context       = task_dma->desc.context;
    vidmm_allocation_t      *mm_allocation = NULL;
    vidsch_allocation_t     *sch_allocation = NULL;
    unsigned long long      fence_id;
    int                      i, ret = S_OK;

    gf_down_read(schedule->rw_lock);

    if (task_dma->desc.Flags.normal_recovery)
        schedule->chip_func->boost(adapter, ENG_POWER_STATE_E0, ENG_POWER_MIN_HOLDING_TIME_MS * 10, FALSE);
    else
        schedule->chip_func->boost(adapter, sch_mgr->boost_level, ENG_POWER_MIN_HOLDING_TIME_MS, FALSE);

    gf_mutex_lock(sch_mgr->engine_lock);

    //gf_mutex_lock(adapter->hw_reset_lock);

    fence_id = vidschi_inc_send_fence_id(sch_mgr, task_dma->prepare_submit);

    task_dma->prepare_submit = FALSE;

    cm_device_lock(context->device);

    for (i = 0; i < task_dma->allocation_list_size; i++)
    {
        sch_allocation = &task_dma->sch_allocation_list[i];
        mm_allocation  = sch_allocation->mm_allocation;

        if (mm_allocation)
        {
            gf_spin_lock(mm_allocation->lock);

            mm_allocation->fence_id[sch_mgr->engine_index] = fence_id;

            if(sch_allocation->write_operation)
            {
                mm_allocation->write_fence_id[sch_mgr->engine_index] = fence_id;

                mm_allocation->write_render_count[sch_mgr->engine_index]--;
            }

            mm_allocation->render_count[sch_mgr->engine_index]--;

            mm_allocation->status.submit_ref_cnt--;

            gf_spin_unlock(mm_allocation->lock);

            sch_allocation->segment_id = mm_allocation->segment_id;
            sch_allocation->phy_addr   = mm_allocation->phys_addr;
        }
    }

    cm_device_unlock(context->device);

    if (task_dma->patch_location_list_size > 0)
    {
        sch_mgr->chip_func->patch(adapter, task_dma);
    }

    task_dma->desc.cmd_size = task_dma->submit_end_offset - task_dma->submit_start_offset;
    task_dma->desc.fence_id = fence_id;

    if(sch_mgr->dma_reserved_memory->vma->need_flush_cache)
    {
         gf_flush_cache(adapter->os_device.pdev, sch_mgr->dma_reserved_memory->vma, sch_mgr->dma_reserved_memory->pages_mem, 0, task_dma->dma_buffer_node->size);
    }

    /* call chip ddi submit dma buffer */
    /* after dma kick off, allocations ptr referenced in current dma
     * may invalid, maybe freed by other thread. So do no access it. reason as follow:
     *
     *       app-thread                               submit_thread
     *     create_allocation_A
     *     render to allocation_A
     *     --> submit this taskA to thread
     *     destroy allocation A                       submit_taskA
     *     --> add to defer destroy allocation        -->update fence id,
     *     --> wait_allocation_idle to destroy        -->update render_count--;
     *                                                on this point, allocation may idle when fence id back.
     *                                                -->killoff task.
     *                                                (after kick off fence will back anytime,
     *                                                if so allocation can be destroy by other thread,
     *                                                so after kickoff, allocatin ptr in task are invalid)
     *
     */

    vidschi_update_and_release_dma_fence(adapter, &task_dma->desc);

    ret = sch_mgr->chip_func->submit(adapter, sch_mgr, task_dma);

    gf_task_submit_trace_event(adapter->index << 16 | sch_mgr->engine_index, context->handle, task_dma->desc.context_task_id, task_dma->desc.type, fence_id, task_dma->dma_type);

    //gf_mutex_unlock(adapter->hw_reset_lock);
    gf_mutex_unlock(sch_mgr->engine_lock);

    gf_up_read(schedule->rw_lock);

    /* set dma node submitted*/
    vidschi_set_dma_task_submitted(sch_mgr, &task_dma->desc);

    return ret;
}

void vidsch_submit_paging_task(adapter_t *adapter, task_paging_t *paging_task)
{
    vidschedule_t      *schedule   = adapter->schedule;
    vidsch_mgr_t       *sch_mgr    = adapter->sch_mgr[adapter->paging_engine_index];
    vidmm_allocation_t *allocation = NULL;

    int i;

    if(paging_task->command_length > 0)
    {
        task_dma_t         task_dma    = {0};
        unsigned long long fence_id;

        task_dma.dma_type            = PAGING_DMA;
        task_dma.need_hwctx_switch   = FALSE;
        task_dma.command_length      = paging_task->command_length;
        task_dma.dma_buffer_node     = paging_task->dma;
        task_dma.submit_start_offset = 0;
        task_dma.submit_end_offset   = paging_task->command_length;

        if(sch_mgr->dma_reserved_memory->vma->need_flush_cache)
        {
            gf_flush_cache(adapter->os_device.pdev, sch_mgr->dma_reserved_memory->vma, sch_mgr->dma_reserved_memory->pages_mem, 0, task_dma.dma_buffer_node->size);
        }

        gf_down_read(schedule->rw_lock);

        schedule->chip_func->boost(adapter, sch_mgr->boost_level, ENG_POWER_MIN_HOLDING_TIME_MS, FALSE);

        /* lock engine */
        gf_mutex_lock(sch_mgr->engine_lock);

        fence_id = vidschi_inc_send_fence_id(sch_mgr, FALSE);

        paging_task->desc.fence_id = fence_id;
        paging_task->desc.cmd_size = paging_task->command_length;

        task_dma.desc.fence_id     = fence_id;

        /* udpate fence id must locked by engine_lock to make sure the fence id is increased */
        for(i = 0; i < paging_task->paging_allocation_num; i++)
        {
            allocation = paging_task->paging_allocation_list[i].allocation;

            cm_device_lock_allocation(allocation->device, allocation);

            allocation->fence_id[sch_mgr->engine_index] = fence_id;

            allocation->write_fence_id[sch_mgr->engine_index] = fence_id;

            allocation->last_paging                     = fence_id;

            allocation->render_count[sch_mgr->engine_index]--;

            allocation->write_render_count[sch_mgr->engine_index]--;

            cm_device_unlock_allocation(allocation->device, allocation);
        }

        /* call chip submit dma buffer */
        sch_mgr->chip_func->submit(adapter, sch_mgr, &task_dma);

        gf_task_submit_trace_event(adapter->index << 16 | sch_mgr->engine_index, 0, ptr_to_ptr64(&paging_task->desc), task_type_paging, fence_id, PAGING_DMA);

        /* unlock engine */
        gf_mutex_unlock(sch_mgr->engine_lock);

        gf_up_read(schedule->rw_lock);

        /* set dma node submitted*/
        vidschi_set_dma_task_submitted(sch_mgr, &paging_task->desc);
    }
    else
    {
        /* cmd size == 0, mean paging task is for paging gart memory to system pages,
         * since gart segment memory data already in system pages, so only update allocation info
         */

        for(i = 0; i < paging_task->paging_allocation_num; i++)
        {
            allocation = paging_task->paging_allocation_list[i].allocation;

            cm_device_lock_allocation(allocation->device, allocation);

            allocation->render_count[sch_mgr->engine_index]--;

            allocation->write_render_count[sch_mgr->engine_index]--;

            cm_device_unlock_allocation(allocation->device, allocation);
        }

        gf_task_submit_trace_event(adapter->index << 16 | sch_mgr->engine_index, 0, ptr_to_ptr64(&paging_task->desc), task_type_paging, 0, PAGING_DMA);

        vidsch_notify_interrupt(adapter, 0); //render_count update, wait up waiter

        vidschi_set_dma_task_fake_submitted(sch_mgr, &paging_task->desc);
    }

}

#define VIDSCH_MAX_PAGING_IN_DMA_SIZE 16*1024

static int vidschi_resident_task_allocations(vidsch_mgr_t *sch_mgr, task_dma_t *task_dma)
{
    adapter_t          *adapter    = sch_mgr->adapter;
    gpu_context_t      *context    = task_dma->desc.context;
    vidmm_allocation_t *allocation = NULL;
    unsigned long long last_paging = 0x0ll;

    int i, unresident_num = 0, result = S_OK;

    cm_device_lock(context->device);

    /* first loop to check if task need do paging and estimate the paging allocation num,
     * NOTE: the num is not real num, since this loop not use paging lock, the real num may lower than this value.
     *       WHY not lock this loop?  for none-paging task need not wait pervious paging finish, better for performance
     */
    for(i = 0; i < task_dma->allocation_list_size; i++)
    {
        allocation = task_dma->sch_allocation_list[i].mm_allocation;

        if(allocation != NULL)
        {
            gf_spin_lock(allocation->lock);

            /* mark allocation temp unpagable since allocation referenced by submit */
            allocation->status.submit_ref_cnt++;

            /* if allocation not resident need do paging in */
            if(allocation->segment_id == SEGMENT_ID_INVALID)
            {
                unresident_num++;
            }

            if(allocation->last_paging > last_paging)
            {
                last_paging = allocation->last_paging;
            }

            gf_spin_unlock(allocation->lock);
        }
    }

    cm_device_unlock(context->device);

    /* if need paging, then enter paging path, paging will re-loop the allocation list, to calc the real paging allocation num */
    if(unresident_num > 0)
    {
        task_paging_t             *paging_task       = vidsch_allocate_paging_task(adapter, VIDSCH_MAX_PAGING_IN_DMA_SIZE, unresident_num);
        vidmm_paging_allocation_t *paging_allocation = NULL;
        int                        real_unresident_num = 0;

        paging_task->must_success = TRUE;

        gf_mutex_lock(adapter->paging_lock);

        for(i = 0; i < task_dma->allocation_list_size; i++)
        {
            allocation = task_dma->sch_allocation_list[i].mm_allocation;

            /* since allocation segment memory only can be changed by paging, we already in paging,
             * this value never changed, no need lock
             */

            /* TODO: we can make resident lru list here if needed */
            if((allocation != NULL) &&
               (allocation->segment_id == SEGMENT_ID_INVALID) )
            {
                 /*
                 WHY: do not redundancy add same allocation into the paging_allocation_list,
                 since after allocation paging in success and before this paging_task submit, allocation's segment id is alway SEGMENT_ID_INVALID,
                 if re-call vidmm_resident_one_allocation for same allocation will cause paging assert(no pags_mem ,file_store and segment id is SEGMENT_ID_INVALID).
                 */
                int j=0, found=0;

                for(j = 0; j < real_unresident_num; j++)
                {
                     if(paging_task->paging_allocation_list[j].allocation == allocation)
                     {
                           found = 1;
                           break;
                      }
                }

                if(!found)
                    paging_task->paging_allocation_list[real_unresident_num++].allocation = allocation;
            }
        }

        for(i = 0; i < real_unresident_num; i++)
        {
            paging_allocation = &paging_task->paging_allocation_list[i];
 try_again:
            result = vidmm_resident_one_allocation(context->device, paging_task, paging_allocation, 0);

            if(result == S_OK)
            {
                /* successfully resident one allocation, try next */
                continue;
            }
            else if(result == E_FAIL)
            {
                /* for force local allocation, the resident fail will probably make the dma submit failed,
                 * patch the backup segment id to this allocation's preferred segment, make the resident success.
                 */
                allocation = paging_allocation->allocation;
                if((allocation->backup_segment_id != 0)&&(allocation->preferred_segment.segment_id_1 == 0))
                {
                   allocation->preferred_segment.segment_id_1 = allocation->backup_segment_id;
                   gf_info("resident allocation[0x%d-%dK] failed by not enough local memory, add segment %d to preferred and try again.\n",
                                             allocation->handle, allocation->orig_size/1024, allocation->backup_segment_id);
                   goto try_again;
                }

                /* no enought video memory, resident failed. return fail to caller,
                 * caller can go split or wait a while and try again
                 */
                break;
            }
            else if(result == E_OUTOFMEMORY)
            {
                /* system pages exhausted when paging out, can not do anything return error */
                break;
            }
            else if(result == E_INSUFFICIENT_DMA_BUFFER)
            {
                /* dma_buffer not enough, will submit success resident allocations, and also return fail
                 * to caller to notify not all unresident allocations resident, caller can try resident again
                 */
                gf_info("Resident task: %lld[context: %x] failed, since dma buffer not enough.\n",
                    task_dma->desc.context_task_id, context->handle);

                gf_info("  Task Total allocations: %d, need resident: %d, success resident: %d\n",
                    task_dma->allocation_list_size, real_unresident_num, i);
                break;
            }
        }

        gf_info("resident %d/%d allocation, cmd_length:%d/%d...\n",
                i, real_unresident_num, paging_task->command_length, paging_task->dma->size);

        paging_task->paging_allocation_num = i; /* udpate num to real success num */

        vidsch_task_inc_reference(adapter, adapter->paging_engine_index, &paging_task->desc);

        /* no matter submit success or fail submit paging task, since submit also release paging dma resource */
        vidsch_submit_paging_task(adapter, paging_task);

        /* mark success allocation resident and add it to resident list */
        for(i = 0; i < paging_task->paging_allocation_num; i++)
        {
            paging_allocation = &paging_task->paging_allocation_list[i];
            allocation        = paging_allocation->allocation;

            cm_device_lock_allocation(context->device, allocation);

            allocation->segment_id = paging_allocation->segment_id;
            allocation->list_node  = paging_allocation->list_node;
            allocation->size       = paging_allocation->size;
            allocation->phys_addr  = paging_allocation->gpu_virt_addr;

            cm_device_unlock_allocation(context->device, allocation);

            vidmm_add_allocation_to_resident_list(adapter, allocation);
        }

        if(result != S_OK)
        {
            cm_device_lock(context->device);

            for(i = 0; i < task_dma->allocation_list_size; i++)
            {
                allocation = task_dma->sch_allocation_list[i].mm_allocation;

                if(allocation != NULL)
                {
                    /* resident failed, dma can not submit, so clear the reference */
                    gf_spin_lock(allocation->lock);

                    allocation->status.submit_ref_cnt--;

                    gf_spin_unlock(allocation->lock);
                }
            }

            cm_device_unlock(context->device);
        }

        if(paging_task->command_length > 0)
        {
            last_paging = paging_task->desc.fence_id;
        }

        vidsch_task_dec_reference(adapter, adapter->paging_engine_index, &paging_task->desc);

        gf_mutex_unlock(adapter->paging_lock);
    }

    task_dma->prepare_succeeded = (result == S_OK);

    /* multi-engine sync: wait paging back, if task referenced allocation paging before by other engine need wait it back*/
    if(context->engine_index != adapter->paging_engine_index)
    {
        vidsch_wait_fence_back(adapter, adapter->paging_engine_index, last_paging);
    }

    return result;

}

/* prepare dma, paging in task_dma allocations and patch the cmd buffer
 * return: S_OK
 *         E_FAIL mean video memory not enough, caller can go split or wait a while and try again.
 *         E_OUTOFMEMORY mean fatal error, system pages not enough. this lead render fail
 */

#define VISCH_MAX_PREPARE_TRY_TIMES 1000

static int vidschi_prepare_task_dma(vidsch_mgr_t *sch_mgr, task_dma_t *task_dma, int must_success)
{
    int try_times = VISCH_MAX_PREPARE_TRY_TIMES, result;

try_again:

    result = vidschi_resident_task_allocations(sch_mgr, task_dma);

    if(result == S_OK)
    {
        return result;
    }
    else if(result == E_FAIL)
    {
        if(must_success && (try_times > 0))
        {
            gf_msleep(5);

            try_times--;

            goto try_again;
        }
        else
        {
            return result;
        }
    }
    else if(result == E_INSUFFICIENT_DMA_BUFFER)
    {
        if(try_times > 0)
        {
            try_times--;

            gf_info("Engine[%d] Resident task: %lld %4dth try failed, since dma buffer not enough. try again\n",
                 sch_mgr->engine_index, task_dma->desc.context_task_id, VISCH_MAX_PREPARE_TRY_TIMES - try_times);

            goto try_again;
        }
        else
        {
            gf_info("resident task failed, since dma buffer not enough. \n");

            return E_FAIL; //notify fail mean GPU resource not enough, like video memory.
        }
    }
    else if(result == E_OUTOFMEMORY)
    {
        return result;
    }

    return result;
}

/* fake submit: use to update allocation/device info when submit failed or split */
void vidschi_fake_submit_dma(vidsch_mgr_t *sch_mgr, task_dma_t *task_dma, int discard)
{
    adapter_t          *adapter    = sch_mgr->adapter;
    gpu_context_t      *context    = task_dma->desc.context;
    vidmm_allocation_t *allocation = NULL;

    int i;

    cm_device_lock(context->device);

    for (i = 0; i < task_dma->allocation_list_size; i++)
    {
        allocation = task_dma->sch_allocation_list[i].mm_allocation;

        if (allocation != NULL)
        {
            gf_spin_lock(allocation->lock);

            allocation->render_count[sch_mgr->engine_index]--;

            if(task_dma->sch_allocation_list[i].write_operation)
            {
                allocation->write_render_count[sch_mgr->engine_index]--;
            }

            if(task_dma->prepare_succeeded)
            {
                allocation->status.submit_ref_cnt--;
            }

            gf_spin_unlock(allocation->lock);
        }
    }

    cm_device_unlock(context->device);

    if(task_dma->prepare_submit)
    {
        unsigned long flags;

        flags = gf_spin_lock_irqsave(sch_mgr->fence_lock);

        sch_mgr->prepare_task_num--;

        gf_spin_unlock_irqrestore(sch_mgr->fence_lock, flags);

        task_dma->prepare_submit = FALSE;
    }

    task_dma->desc.fence_id = 0;

    gf_task_submit_trace_event(adapter->index << 16 | sch_mgr->engine_index, context->handle, task_dma->desc.context_task_id, task_dma->desc.type, 0, task_dma->dma_type);

    vidschi_update_and_release_dma_fence(adapter, &task_dma->desc);

    vidschi_set_dma_task_fake_submitted(sch_mgr, &task_dma->desc);

    vidsch_notify_interrupt(adapter, 0);
}

#ifdef GF_HW_NULL
static void vidschi_fake_submit_dma_with_fence(vidsch_mgr_t *sch_mgr, task_dma_t *dma_task)
{
    unsigned long long completed_fence_id = dma_task->desc.fence_id;

    gf_debug("fake submit dma task with fence_id: %lld\n", completed_fence_id);
    if(sch_mgr->chip_func->set_fence_id)
    {
        sch_mgr->chip_func->set_fence_id(sch_mgr, completed_fence_id);
    }
    vidschi_fake_submit_dma(sch_mgr, dma_task, FALSE);
}
#endif

static task_dma_t *vidsch_allocate_split_dma(adapter_t *adapter, unsigned int engine_index, task_dma_t *task_dma)
{
    task_dma_t *split_dma = NULL;
    char* memory          = NULL;
    unsigned int offset   = 0;
    unsigned int          allocations_size = 0;//task_dma->allocation_list_size * sizeof(vidsch_allocation_t);
    unsigned int          patchs_size         = 0;//task_dma->patch_location_list_size * sizeof(gf_patchlocation_list_t);
    unsigned int          syncs_size          = 0;//task_dma->sync_object_list_size * sizeof(vidsch_sync_object_reference_t);
    vidsch_mgr_t       *vidsch               = adapter->sch_mgr[engine_index];
    gpu_context_t      *context             = task_dma->desc.context;

    unsigned int total_size = util_align(sizeof(task_dma_t), 16) +
                              util_align(allocations_size, 16) +
                              util_align(patchs_size, 16) +
                              util_align(syncs_size, 16);

    memory = gf_calloc(total_size);
    if(memory == NULL)
    {
        gf_error("func vidsch_allocate_split_dma allocated memory failed, size:%x.\n", total_size);
    }

    split_dma = (task_dma_t *)(memory + offset);
    offset += util_align(sizeof(task_dma_t), 16);

    gf_memcpy(split_dma,task_dma,sizeof(task_dma_t));

    //dma fence can not release multiple times, and task_dma will submit after all split dma submited(whatever success or not)
    //just keep task_dma own dma fence can handle all the case
    split_dma->desc.dma_fence = NULL;

    gf_atomic_inc(context->ref_cnt);  //to avoid del context, although it would not happened.

    split_dma->desc.list_item.prev = &(split_dma->desc.list_item);//not disturb task_dma's order
    split_dma->desc.list_item.next = &(split_dma->desc.list_item);
//    split_dma->dump = 1;

    if(0 && task_dma->allocation_list_size)
    {
        split_dma->sch_allocation_list   = (vidsch_allocation_t *)(memory + offset);
        split_dma->allocation_list_size  = task_dma->allocation_list_size;
        offset += util_align(allocations_size, 16);
    }

    if(0 && task_dma->patch_location_list_size)
    {
        split_dma->patch_location_list      = (gf_patchlocation_list_t *)(memory + offset);
        split_dma->patch_location_list_size = task_dma->patch_location_list_size;
        split_dma->patch_start_offset       = 0;
        split_dma->patch_end_offset         = task_dma->patch_end_offset;
        offset += util_align(patchs_size, 16);
    }

    if(0 && task_dma->sync_object_list_size)
    {
        split_dma->sync_object_list         = (vidsch_sync_object_reference_t *)(memory + offset);
        split_dma->sync_object_list_size    =  task_dma->sync_object_list_size;

        offset += util_align(syncs_size, 16);
    }

    gf_assert(offset == total_size, "offset == total_size");

    split_dma->dma_buffer_node = vidschi_allocate_dma_node(vidsch, task_dma->command_length);
    gf_memcpy(split_dma->dma_buffer_node->virt_base_addr,task_dma->dma_buffer_node->virt_base_addr,task_dma->command_length);
//    gf_memcpy(split_dma->patch_location_list,task_dma->patch_location_list,patchs_size);

    return split_dma;

}

static int vidschi_split_dma(vidsch_mgr_t *sch_mgr, task_dma_t *task_dma)
{
    adapter_t               *adapter  = sch_mgr->adapter;
    gpu_context_t           *context  = task_dma->desc.context;
    unsigned int            dma_start = task_dma->submit_start_offset;
    unsigned int            dma_end   = task_dma->submit_end_offset;
    unsigned int            patch_start = task_dma->patch_start_offset;
    unsigned int            patch_end   = task_dma->patch_end_offset;
    gf_patchlocation_list_t    *patch_location_list = NULL, *temp_patch = NULL;
    vidsch_allocation_t         *sch_allocation_list = NULL;
    vidmm_allocation_t        *allocation   = NULL;
    task_dma_t                       *split_dma;
    int                                can_split = FALSE;
    unsigned int                i = 0, j = 0, k = 0, ret = S_OK;
    unsigned int                *patch_per_slot = NULL, *cur_patch_per_slot = NULL;
    int                                engine_index = sch_mgr->engine_index;
    int                                end = dma_start;

    vidsch_trace("vidsch: enter vidschi_split_dma. \n");
    /* check split point, if no, return fail */
    for (i = patch_start; i < patch_end; i++)
    {
        patch_location_list = task_dma->patch_location_list + i;
        if (patch_location_list->SplitOffset > dma_start &&
            patch_location_list->SplitOffset < dma_end)
        {
            can_split = TRUE;
            break;
        }
    }

    if (!can_split || !adapter->ctl_flags.split_enable)
    {
        return E_INVALIDARG;
    }

    patch_per_slot = gf_calloc((sch_mgr->slot_count+1) * sizeof(unsigned int) * 2);

    gf_memset(patch_per_slot, -1, (sch_mgr->slot_count+1) * sizeof(unsigned int) * 2);

    cur_patch_per_slot = patch_per_slot + sch_mgr->slot_count + 1;

    sch_allocation_list = gf_calloc(task_dma->allocation_list_size * sizeof(vidsch_allocation_t));

 //   split_dma.sch_allocation_list = sch_allocation_list;

    do{
        vidmm_trace("vidsch: vidschi_split: split dma from %d. \n", patch_start);

        split_dma = vidsch_allocate_split_dma(adapter, engine_index, task_dma);
        split_dma->desc.task_render = split_dma;
        split_dma->sch_allocation_list = sch_allocation_list;

        vidsch_task_inc_reference(adapter, engine_index,&(split_dma->desc));

        gf_memset(sch_allocation_list, 0, sizeof(vidsch_allocation_t) * task_dma->allocation_list_size);
        gf_memset(cur_patch_per_slot, -1, (sch_mgr->slot_count+1) * sizeof(unsigned int));

        /* loop find split point from start, split dma buffer */
        for (i = patch_start; i < patch_end+1; i++)
        {
            patch_location_list = task_dma->patch_location_list + i;

            if ((i == patch_end) || patch_location_list->SplitOffset > dma_start)
            {
                for(j = 1; j <= sch_mgr->slot_count; j++)
                {
                    if((-1 != patch_per_slot[j]) || (-1 != cur_patch_per_slot[j]))
                    {
                        k = (patch_per_slot[j] != -1) ? patch_per_slot[j] : cur_patch_per_slot[j];
                        temp_patch = task_dma->patch_location_list + k;
                        sch_allocation_list[temp_patch->AllocationIndex].mm_allocation =
                            task_dma->sch_allocation_list[temp_patch->AllocationIndex].mm_allocation;
                        sch_allocation_list[temp_patch->AllocationIndex].write_operation =
                            task_dma->sch_allocation_list[temp_patch->AllocationIndex].write_operation;
                    }
                }

                split_dma->submit_start_offset   = dma_start;
                split_dma->patch_start_offset    = patch_start;
                split_dma->patch_end_offset      = i;
                if(i < patch_end)
                {
                    split_dma->submit_end_offset =
                    end = patch_location_list->SplitOffset;
                    patch_start = i;
                    dma_start = patch_location_list->SplitOffset;
                }
                else
                {
                    /* last split dma */
                split_dma->submit_end_offset =
                end = dma_end;
                }

                break;
            }
            else
            {
                gf_assert(patch_location_list->SplitOffset == dma_start, "patch_location_list->SplitOffset == dma_start");
                gf_assert(patch_location_list->SlotId > 0, "patch_location_list->SlotId > 0");
                gf_assert(patch_location_list->SlotId <= sch_mgr->slot_count, "patch_location_list->SlotId <= sch_mgr->slot_count");

                if(patch_location_list->AllocationIndex)
                {
                    patch_per_slot[patch_location_list->SlotId] = i;
                    cur_patch_per_slot[patch_location_list->SlotId] = i;
                }
                else
                {
                    /* Free Slot case */
                    patch_per_slot[patch_location_list->SlotId] = -1;
                }
            }
        }

        /* new task generated, mark render_cnt++*/
        cm_device_lock(context->device);

        for(i = 0; i <split_dma->allocation_list_size; i++)
        {
            allocation = split_dma->sch_allocation_list[i].mm_allocation;

            if(allocation != NULL)
            {
                gf_spin_lock(allocation->lock);

                allocation->render_count[sch_mgr->engine_index]++;

                if(split_dma->sch_allocation_list[i].write_operation)
                {
                    allocation->write_render_count[sch_mgr->engine_index]++;
                }

                gf_spin_unlock(allocation->lock);
            }
        }

        cm_device_unlock(context->device);

        {
        int can_prepare = FALSE;


        for(i = 0; i < 2000; i++)
        {
            can_prepare = vidschi_can_prepare_task_dma(sch_mgr, split_dma);

            if(can_prepare) break;

            gf_msleep(1);
        }

        if(can_prepare)
        {
            ret = vidschi_prepare_task_dma(sch_mgr, split_dma, TRUE);

            if(ret == S_OK)
            {
                vidschi_submit_dma(sch_mgr, split_dma);
            }
            else
            {
                int i;
                /* since split dma is part of task_dma, not discard */
                vidschi_fake_submit_dma(sch_mgr, split_dma, FALSE);
                gf_error("vidschi_split_dma: prepare_task_dma failed. %d \n", ret);

                gf_info("vidschi_split_dma: split_dma info: device: %s, type: %d, size: %d.\n", split_dma->desc.context->device->pname, split_dma->dma_type, split_dma->desc.cmd_size);
                gf_info("vidschi_split_dma: split dma allocation info:\n");
                for(i = 0; i < split_dma->allocation_list_size; i++)
                {
                    allocation = split_dma->sch_allocation_list[i].mm_allocation;
                    if (allocation)
                    {
                        gf_info("allo: %x, refcount:%d, dev: %x, fmt: %3d-%2d-%d-%2d, W-H-P: %8d-%4d-%8d, size: %6dk, gvm: %08x, AT:0x%02x, status: %8x.\n",
                            allocation->handle, allocation->ref_count, allocation->device->handle,
                            allocation->hw_format, allocation->compress_format, allocation->flag.swizzled, allocation->bit_count,
                            allocation->width, allocation->height, allocation->pitch,
                            util_align(allocation->orig_size, util_max(allocation->alignment, adapter->os_page_size)) >> 10,
                            allocation->phys_addr, allocation->at_type, allocation->status.temp_unpagable);
                    }
                }

                vidmm_dump_resource(adapter);
                cm_dump_resource(adapter);
                break;
            }
        }
        else
        {
            /* can not prepare since hw queue issue */
            vidschi_fake_submit_dma(sch_mgr, split_dma, FALSE);

            gf_error("vidschi_split_dma: can_prepare_task_dma failed. can not be here.\n");

            break;
        }
        }

        vidsch_wait_fence_back(adapter, engine_index, split_dma->desc.fence_id);
        vidsch_task_dec_reference(adapter, engine_index,&(split_dma->desc));

    } while (end != task_dma->submit_end_offset);

    if(patch_per_slot)
    {
        gf_free(patch_per_slot);
    }

    if(sch_allocation_list)
    {
        gf_free(sch_allocation_list);
    }

    vidsch_trace("vidsch: leave vidschi_split_dma. \n");

    return ret;
}


int vidschi_prepare_and_submit_dma(vidsch_mgr_t *sch_mgr, task_dma_t *task_dma)
{
    hw_ctxbuf_t     *hw_ctx  = task_dma->hw_ctx_info;
    int             result   = S_OK;
    int             discard  = FALSE;

    gf_begin_section_trace_event("vidschi_prepare_and_submit_dma");

    if(hw_ctx && hw_ctx->is_invalid)
    {
        vidschi_fake_submit_dma(sch_mgr, task_dma, TRUE);

        gf_end_section_trace_event(E_FAIL);
        return E_FAIL;
    }

    result = vidschi_prepare_task_dma(sch_mgr, task_dma, sch_mgr->adapter->ctl_flags.split_enable);

    if(result == S_OK)
    {
        vidschi_submit_dma(sch_mgr, task_dma);
    }
    else if(result == E_FAIL)
    {
        /* if prepare failed by video memory not enough, do split */
        result = vidschi_split_dma(sch_mgr, task_dma);

        /* if task can not do split, go general way */
        if(result == E_INVALIDARG)
        {
            result = vidschi_prepare_task_dma(sch_mgr, task_dma, TRUE);

            if(result == S_OK)
            {
                vidschi_submit_dma(sch_mgr, task_dma);
            }
            else
            {
                /* prepare failed discard this task */
                discard = TRUE;
            }
        }
        else if(result == S_OK)
        {
            /* if split happen and success, since split task dma to multi new task and submitted,
             * So here only fake submit to update the allocation info
             */
            vidschi_fake_submit_dma(sch_mgr, task_dma, FALSE);
        }
        else
        {
            /* split may failed partially, so here wait subimtted part back. seems no need wait,
             * but still add here to make sure there is no issue
             */
            vidsch_wait_fence_back(sch_mgr->adapter, sch_mgr->engine_index, task_dma->desc.fence_id);

            discard = TRUE;
        }
    }
    else
    {
        /* if prepare task failed by pages not enough, discard the task */
        discard = TRUE;
    }

    if(discard)
    {
        /* submit fail, mark hw_ctx invalid and discard this task_dma */
        gpu_context_t *context = task_dma->desc.context;

        gf_info("submit failed, set hwctx invalid.\n");

        context->device->task_dropped  = TRUE;

        if(hw_ctx)
        {
            hw_ctx->is_invalid = TRUE;
        }

        vidschi_fake_submit_dma(sch_mgr, task_dma, TRUE);
    }

#ifdef GF_HW_NULL
    vidschi_fake_submit_dma_with_fence(sch_mgr, task_dma);
#endif

    gf_end_section_trace_event(result);
    return result;
}


