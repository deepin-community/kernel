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

#include "gf_adapter.h"
#include "global.h"
#include "vidmm.h"
#include "vidsch.h"
#include "vidmmi.h"

static inline void vidmmi_add_allocation_to_resident_list(vidmm_segment_t *segment, vidmm_allocation_t *allocation)
{
    if(allocation->flag.unpagable)
    {
        list_add_tail(&allocation->list_item, &segment->unpagable_resident_list);

        segment->unpagable_size += allocation->size;
        segment->unpagable_num++;
    }
    else
    {
        list_add_tail(&allocation->list_item, &segment->pagable_resident_list[allocation->priority]);

        segment->pagable_size += allocation->size;
        segment->pagable_num++;
    }

    gf_counter_trace_event(segment->segment_name,
        (segment->pagable_size + segment->unpagable_size + segment->reserved_size) >> 10);
}

static inline void vidmmi_remove_allocation_from_resident_list(vidmm_segment_t *segment, vidmm_allocation_t *allocation)
{
    list_del(&allocation->list_item);

    if(allocation->flag.unpagable)
    {
        segment->unpagable_size -= allocation->size;
        segment->unpagable_num--;
    }
    else
    {
        segment->pagable_size -= allocation->size;
        segment->pagable_num--;
    }

    gf_counter_trace_event(segment->segment_name,
        (segment->pagable_size + segment->unpagable_size + segment->reserved_size) >> 10);
}

void vidmm_add_allocation_to_resident_list(adapter_t *adapter, vidmm_allocation_t *allocation)
{
    vidmm_mgr_t      *mm_mgr  = adapter->mm_mgr;
    vidmm_segment_t  *segment = &mm_mgr->segment[allocation->segment_id];

    gf_assert(allocation->segment_id != SEGMENT_ID_INVALID, "allocation->segment_id != SEGMENT_ID_INVALID");

    gf_mutex_lock(segment->lock);

    vidmmi_add_allocation_to_resident_list(segment, allocation);

    gf_mutex_unlock(segment->lock);
}

void vidmm_remove_allocation_from_resident_list(adapter_t *adapter, vidmm_allocation_t *allocation)
{
    vidmm_mgr_t      *mm_mgr  = adapter->mm_mgr;
    vidmm_segment_t  *segment = &mm_mgr->segment[allocation->segment_id];

    gf_mutex_lock(segment->lock);

    vidmmi_remove_allocation_from_resident_list(segment, allocation);

    gf_mutex_unlock(segment->lock);
}

#define vidmmi_add_allocation_to_cache_list(segment, allocation)      vidmmi_add_allocation_to_resident_list(segment, allocation)
#define vidmmi_remove_allocation_from_cache_list(segment, allocation) vidmmi_remove_allocation_from_resident_list(segment, allocation)

void vidmm_add_allocation_to_cache_list(adapter_t *adapter, vidmm_allocation_t *allocation)
{
    vidmm_mgr_t     *mm_mgr        = adapter->mm_mgr;
    vidmm_segment_t *cache_segment = &mm_mgr->segment[0];

    gf_assert(allocation->segment_id == 0, "add allocation->segment_id == 0");

    gf_mutex_lock(cache_segment->lock);

    vidmmi_add_allocation_to_cache_list(cache_segment, allocation);

    gf_mutex_unlock(cache_segment->lock);
}

void vidmm_remove_allocation_from_cache_list(adapter_t *adapter, vidmm_allocation_t *allocation)
{
    vidmm_mgr_t     *mm_mgr        = adapter->mm_mgr;
    vidmm_segment_t *cache_segment = &mm_mgr->segment[0];

    gf_assert(allocation->segment_id == 0, "remove allocation->segment_id == 0");

    gf_mutex_lock(cache_segment->lock);

    vidmmi_remove_allocation_from_cache_list(cache_segment, allocation);

    gf_mutex_unlock(cache_segment->lock);
}

int vidmmi_copy_data(vidmm_mgr_t *mm_mgr, task_paging_t *paging_task,vidmm_allocation_t *dst_allocation,  
        unsigned long long src_addr, unsigned long long dst_addr, unsigned int src_compress, unsigned int dst_compress, unsigned int size)
{
    vidmm_private_build_paging_buffer_arg_t build_paging_buffer = {0};

    int status = S_OK;

    build_paging_buffer.operation              = BUILDING_PAGING_OPERATION_TRANSFER;
    build_paging_buffer.dma_buffer             = paging_task->dma->virt_base_addr + paging_task->command_length;
    build_paging_buffer.dma_size               = paging_task->dma->size - paging_task->command_length;
    build_paging_buffer.multi_pass_offset      = 0;
    build_paging_buffer.transfer.transfer_size = size;
    build_paging_buffer.transfer.src.phy_addr  = src_addr;
    build_paging_buffer.transfer.dst.phy_addr  = dst_addr;
    build_paging_buffer.allocation             = dst_allocation;

    build_paging_buffer.transfer.src.compress_format  = src_compress;
    build_paging_buffer.transfer.dst.compress_format  = dst_compress;

    status = mm_mgr->chip_func->build_paging_buffer(mm_mgr->adapter, &build_paging_buffer);

    if(status == S_OK)
    {
        /* update cmd_length */
        paging_task->command_length = util_get_ptr_span(build_paging_buffer.dma_buffer, paging_task->dma->virt_base_addr);
    }
    else
    {
        gf_info("Paging DMA size not enough: size: %d, current_size: %d.\n", 
            paging_task->dma->size, util_get_ptr_span(build_paging_buffer.dma_buffer, paging_task->dma->virt_base_addr));
    }

    return status;
}

/* allocate temp paging video memory */
int vidmmi_allocate_temp_paging_memory(vidmm_mgr_t *mm_mgr, vidmm_allocation_t *allocation)
{
    adapter_t       *adapter        = mm_mgr->adapter;
    vidmm_segment_t *paging_segment = mm_mgr->paging_segment;

    int status = S_OK;

    //allocation->list_node = heap_allocate(&paging_segment->heap, allocation->orig_size, allocation->alignment, 0);

    while(allocation->list_node == NULL)
    {
        unsigned long long  uncomplete_paging = 0ll;

        vidsch_release_completed_tasks(adapter, adapter->paging_engine_index, &uncomplete_paging);

        allocation->list_node = heap_allocate(&paging_segment->heap, allocation->orig_size, allocation->alignment, 0);

        if(allocation->list_node == NULL)
        {
            if(uncomplete_paging == 0ll)
            {
                gf_info("allocate temp paging memory failed. want size: %dk, paging heap size: %dk.\n", 
                    util_align(allocation->orig_size, adapter->os_page_size) >> 10, paging_segment->gpu_vm_size >> 10);

                gf_info("paging heap status: \n");

                heap_dump(&paging_segment->heap);

                status = E_FAIL;
                break;
            }

            vidsch_wait_fence_back(adapter, adapter->paging_engine_index, uncomplete_paging);
        }
    }

    if(allocation->list_node != NULL)
    {
        allocation->segment_id = 0;
        allocation->size       = util_align(allocation->orig_size, adapter->os_page_size);
        allocation->phys_addr  = allocation->list_node->aligned_offset;
    }

    return status;
}

void vidmmi_release_temp_paging_memory(vidmm_mgr_t *mm_mgr, vidmm_allocation_t *temp_allocation)
{
    vidmm_segment_t    *paging_segment  = mm_mgr->paging_segment;

    if(temp_allocation->list_node != NULL)
    {
        heap_release(&paging_segment->heap, temp_allocation->list_node);

        temp_allocation->list_node = NULL;
    }

    if(temp_allocation->pages_mem != NULL)
    {
        vidmmi_release_system_memory(mm_mgr->adapter, temp_allocation);

        temp_allocation->pages_mem == NULL;
    }

    gf_free(temp_allocation);
}

/* release complete temp paging allocation in paging task */
void vidmm_release_temp_paging_memory(adapter_t *adapter, void *ptask)
{
    vidmm_mgr_t        *mm_mgr          = adapter->mm_mgr;
    vidmm_allocation_t *temp_allocation = NULL;
    task_paging_t      *paging_task     = ptask;

    int i;

    for(i = 0; i < paging_task->paging_allocation_num; i++)
    {
        temp_allocation = paging_task->paging_allocation_list[i].temp_allocation;

        if(temp_allocation != NULL)
        {
            vidmmi_release_temp_paging_memory(mm_mgr, temp_allocation);
        }
    }
}

/*  paging out: move allocation data to system pages. 2 conditions here.
 *  1. gart segment acctually is system pages, no need move data.
 *  2. local segment, need allocate system pages, and copy data to it.
 *  NOTE: paging_out do not release video memory, video memory only can released after copy finished.
 */

int vidmmi_paging_out(vidmm_mgr_t *mm_mgr, task_paging_t *paging_task, vidmm_paging_allocation_t *paging_allocation)
{
    vidmm_segment_t    *segment         = &mm_mgr->segment[paging_allocation->segment_id];
    vidmm_allocation_t *temp_allocation = NULL;

    int result = S_OK;

    if(segment->flags.require_system_pages)
    {
         vidmm_allocation_t *allocation = paging_allocation->allocation;

         if(allocation->compress_format != 0)
         {
             gf_warning("Paging OUT compressed PCIE allocation. handle: %x process: %s.\n", allocation->handle, allocation->device->pname);
         }
         /* gart segment alreay in system pages, no need paging out */
         return S_OK;
    }

    gf_assert(paging_allocation->allocation->pages_mem == NULL, "paging_allocation->allocation->pages_mem == NULL");

    /* local to paging memory then to system cache */
    temp_allocation = gf_calloc(sizeof(vidmm_allocation_t));

    if(temp_allocation == NULL)
    {
        return E_OUTOFMEMORY;
    }

    result = vidmmi_allocate_system_memory(mm_mgr->adapter, paging_allocation->allocation, paging_task->must_success, TRUE);

    if(S_OK != result)
    {
        gf_free(temp_allocation);

        return E_OUTOFMEMORY;
    }

    temp_allocation->orig_size = paging_allocation->size;
    temp_allocation->alignment = paging_allocation->allocation->alignment;
    temp_allocation->pages_mem = paging_allocation->allocation->pages_mem;

    result = vidmmi_allocate_temp_paging_memory(mm_mgr, temp_allocation);

    if(S_OK != result)
    {
        goto exit;
    }

    vidmm_map_gart_table(mm_mgr->adapter, temp_allocation, mm_mgr->paging_segment->flags.support_snoop);

    paging_task->paging_type = 2;

    result = vidmmi_copy_data(mm_mgr,
                              paging_task,
                              temp_allocation,
                              paging_allocation->gpu_virt_addr,
                              temp_allocation->phys_addr,
                              paging_allocation->allocation->compress_format,
                              temp_allocation->compress_format,
                              paging_allocation->size);
exit:
    if(result == S_OK)
    {
        temp_allocation->pages_mem = NULL;

        paging_allocation->temp_allocation = temp_allocation;
    }
    else
    {
        paging_allocation->allocation->pages_mem = NULL;

        vidmmi_release_temp_paging_memory(mm_mgr, temp_allocation);
    }

    return result;
}

/*  paging in: move allocation data from system pages to video. 2 conditions here.
 *  1. gart segment acctually is system pages, no need move data.
 *  2. local segment, need allocate local video memory, and copy data to it.
 *  NOTE: paging_out do not release system pages, system pages only can released after copy finished.
 */

int vidmmi_paging_in(vidmm_mgr_t *mm_mgr, task_paging_t *paging_task, vidmm_paging_allocation_t *paging_allocation)
{
    vidmm_segment_t    *segment         = &mm_mgr->segment[paging_allocation->segment_id];
    vidmm_allocation_t *allocation      = paging_allocation->allocation;
    vidmm_allocation_t *temp_allocation = NULL;

    int result = S_OK;

    if(paging_allocation->allocation->pages_mem == NULL)
    {
        gf_assert(allocation->file_storage != NULL, "allocation->file_storage != NULL");

        result = vidmmi_allocate_system_memory(mm_mgr->adapter, allocation, TRUE, TRUE);

        if(result == S_OK)
        {
//        gf_info("shrink swapin: allocation:%x, size: %dk.\n", allocation->handle, allocation->size >> 10);
            /* swap file_storage to pages */
            if(gf_pages_memory_swapin(allocation->pages_mem, allocation->file_storage))
            {
                vidmmi_release_system_memory(mm_mgr->adapter, allocation);

                return E_OUTOFMEMORY;
            }
            else
            {
                allocation->file_storage = NULL;
            }
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        if (!paging_task->save_restore)
        {
            vidmm_remove_allocation_from_cache_list(mm_mgr->adapter, allocation);
        }
    }

    gf_assert(paging_allocation->allocation->pages_mem != NULL, "paging_allocation->allocation->pages_mem != NULL");

    if(segment->flags.require_system_pages)
    {
         /* gart segment acctually is system pages, no need copy */
         vidmm_allocation_t *allocation = paging_allocation->allocation;

         allocation->phys_addr = paging_allocation->gpu_virt_addr;
         allocation->size      = paging_allocation->size;

         vidmm_map_gart_table(mm_mgr->adapter, allocation, segment->flags.support_snoop);

         return S_OK;
    }

    /* local to paging memory then to system cache */
    temp_allocation = gf_calloc(sizeof(vidmm_allocation_t));

    if(temp_allocation == NULL)
    {
        if(!paging_task->save_restore)
        {
            vidmm_add_allocation_to_cache_list(mm_mgr->adapter, allocation);
        }

        return E_OUTOFMEMORY;
    }

    temp_allocation->orig_size = paging_allocation->size;
    temp_allocation->alignment = paging_allocation->allocation->alignment;
    temp_allocation->pages_mem = paging_allocation->allocation->pages_mem;

    result = vidmmi_allocate_temp_paging_memory(mm_mgr, temp_allocation);

    if(S_OK != result)
    {
        result = E_FAIL;
        goto exit;
    }

    vidmm_map_gart_table(mm_mgr->adapter, temp_allocation, mm_mgr->paging_segment->flags.support_snoop);

    paging_task->paging_type = 1;

    result = vidmmi_copy_data(mm_mgr,
                              paging_task,
                              allocation,
                              temp_allocation->phys_addr,
                              paging_allocation->gpu_virt_addr,
                              temp_allocation->compress_format,
                              allocation->compress_format,
                              paging_allocation->size);
exit:
    if(result == S_OK)
    {
        paging_allocation->allocation->pages_mem = NULL;
        paging_allocation->temp_allocation       = temp_allocation;
    }
    else
    {
        temp_allocation->pages_mem = NULL;

        vidmmi_release_temp_paging_memory(mm_mgr, temp_allocation);
    }

    return result;
}

#define VIDMM_MAX_PAGING_OUT_MEMORY_SIZE     4*1024*1024 //4M
#define VIDMM_MAX_PAGING_OUT_ALLOCATION_NUM  32
#define VIDMM_MAX_PAGING_OUT_DMA_SIZE        32*1024

int vidmmi_segment_unresident_allocations(vidmm_mgr_t *mm_mgr, vidmm_segment_t *segment, int must_success)
{
    adapter_t                 *adapter           = mm_mgr->adapter;
    vidmm_allocation_t        *candidate         = NULL;
    vidmm_allocation_t        *candidate_next    = NULL;
    vidmm_paging_allocation_t *paging_allocation = NULL;

    int index, priority, vram_size = 0, result = E_FAIL;

    task_paging_t *paging_task = vidsch_allocate_paging_task(
                                 adapter,
                                 VIDMM_MAX_PAGING_OUT_DMA_SIZE,
                                 VIDMM_MAX_PAGING_OUT_ALLOCATION_NUM);

    gf_assert(paging_task != NULL, "paging_task != NULL");

    paging_task->must_success = must_success;

    /* Step 1. select paging out allocations */

    gf_mutex_lock(segment->lock);

    for(priority = PDISCARD; priority < PALL; priority++)
    {
        /* loop for pagable list to select candidate list to paging out */
        list_for_each_entry_safe(candidate, candidate_next, &segment->pagable_resident_list[priority], list_item)
        {
            cm_device_lock_allocation(candidate->device, candidate);

            if(!candidate->status.temp_unpagable &&
               !vidsch_allocation_in_hw_queue(adapter, ALL_ENGINE_MASK, candidate))
            {
                gf_assert(candidate->segment_id != 0, "candidate->segment_id != 0");

                /* remove selected allocation from resident list */
                vidmmi_remove_allocation_from_resident_list(segment, candidate);

                /* mark it referenced by paging, then this allocation can not released
                           * because the paging out allocation maybe destroyed after submit paging task 
                           * and when waiting fence back, so reference count need be increased twice,
                           * one for submit paging task, one for sync lock
                           */

                candidate->render_count[adapter->paging_engine_index] += 2;
                candidate->write_render_count[adapter->paging_engine_index]++;

                paging_allocation = &paging_task->paging_allocation_list[paging_task->paging_allocation_num++];

                paging_allocation->allocation    = candidate;
                paging_allocation->segment_id    = candidate->segment_id;
                paging_allocation->list_node     = candidate->list_node;
                paging_allocation->gpu_virt_addr = candidate->phys_addr;
                paging_allocation->size          = candidate->size;

                /* NOTE: here clear segment id, to tell user allocation is not resident, 
                         segment id = 0 mean allocation is unstable, must wait allocation idle
                         if want access member like pages_mem, list node, size, must first set 
                         unpagable flag and then wait it idle or just access it in paging lock
                 */
                candidate->segment_id = SEGMENT_ID_INVALID;
                candidate->list_node  = NULL;
                candidate->phys_addr  = 0;
                candidate->size       = 0;

                cm_device_unlock_allocation(candidate->device, candidate);

                /* use list_node size but not allocation size here, since the listnode size is real size */
                vram_size += paging_allocation->list_node->size;

                if((vram_size >= VIDMM_MAX_PAGING_OUT_MEMORY_SIZE) || 
                   (paging_task->paging_allocation_num == paging_task->paging_allocation_list_size))
                {
                    goto do_paging_out;
                }
            }
            else
            {
                cm_device_unlock_allocation(candidate->device, candidate);
            }
        }
    }

do_paging_out:

    gf_mutex_unlock(segment->lock);

    /* Step 2. build paging out cmd buffer */

    if(paging_task->paging_allocation_num > 0)
    {
        for(index = 0; index < paging_task->paging_allocation_num; index++)
        {
            paging_allocation = &paging_task->paging_allocation_list[index];
            gf_begin_section_trace_event("unresident_allocation");
            gf_counter_trace_event("arg_allocation", paging_allocation->allocation->handle);
            gf_counter_trace_event("arg_segment", paging_allocation->segment_id);
            gf_counter_trace_event("arg_size", paging_allocation->size);
            result = vidmmi_paging_out(mm_mgr, paging_task, paging_allocation);
            gf_end_section_trace_event(result);

            if(result == E_FAIL)
            {
                gf_info("paging out failed. paging heap is not enough for temp paging buffer\n");
                break;
            }
            else if(result == E_OUTOFMEMORY)
            {
                /* out of memory mean system pages not enough, can not do page out anymore, return error */
                gf_info("paging out failed. outof memory: %d, %dk\n", segment->segment_id, paging_allocation->size >> 10);
                break;
            }
            else if(result == E_INSUFFICIENT_DMA_BUFFER)
            {
                /* just reminder here, but can not be here, since the dma buffer allocate 8K for 8M video memory, enough */
                gf_info("paging out one allocation failed. dma buffer not enough, segment_id: %d, size: %dk\n", 
                    segment->segment_id, paging_allocation->size >> 10);

                gf_info("Unresident %d allocations failed. dma buffer not enough, success_num: %d.\n", 
                    paging_task->paging_allocation_num, index);
                break;
            }
        }

        /* if system pages not enough, we need restore failed candidate allocations, 
         * but still paging out the success allocations 
         */
        if(result != S_OK)
        {
            int i = index;

            gf_mutex_lock(segment->lock);

            /* restore paging failed allocations */
            for(i = index; i < paging_task->paging_allocation_num; i++)
            {
                paging_allocation = &paging_task->paging_allocation_list[i];
                candidate         = paging_allocation->allocation;

                cm_device_lock_allocation(candidate->device, candidate);

                candidate->segment_id = paging_allocation->segment_id;
                candidate->list_node  = paging_allocation->list_node;
                candidate->phys_addr  = paging_allocation->gpu_virt_addr;
                candidate->size       = paging_allocation->size;

                /* NOTE: here also add failed paging out allocation to the list head */
                list_add(&candidate->list_item, &segment->pagable_resident_list[candidate->priority]);

                /* the render count increased twice befor */
                candidate->render_count[adapter->paging_engine_index] -= 2;
                candidate->write_render_count[adapter->paging_engine_index]--;

                cm_device_unlock_allocation(candidate->device, candidate);

                paging_allocation->allocation = NULL;
                paging_allocation->segment_id = SEGMENT_ID_INVALID;

                segment->pagable_size += candidate->size;
                segment->pagable_num++;
                gf_counter_trace_event(segment->segment_name,
                    (segment->pagable_size + segment->unpagable_size + segment->reserved_size) >> 10);
            }

            gf_mutex_unlock(segment->lock);

            /* notify a FENCE event to wakeup thread which wait for render_count dec, since here no real fence interrupt */
            vidsch_notify_interrupt(adapter, 0);

            /* udpate allocation num to real num */
            paging_task->paging_allocation_num = index;

            /* if any allocation paging out success we still treat segment paging out success */
            if(paging_task->paging_allocation_num > 0)
            {
                result = S_OK;
            }
        }
    }
    else
    {
        /* not found suitable allocation to paging out, return FAIL, failed allowed caller wait a while and try again */
        result = E_FAIL;
    }

    /* Step 3: submit paging out task and wait it back */
    vidsch_task_inc_reference(adapter, adapter->paging_engine_index, &paging_task->desc);

    vidsch_submit_paging_task(adapter, paging_task);

    vidsch_wait_fence_back(adapter, adapter->paging_engine_index, paging_task->desc.fence_id);

    /* Step 4: release video memory */
    for(index = 0; index < paging_task->paging_allocation_num; index++)
    {
        paging_allocation = &paging_task->paging_allocation_list[index];
        candidate         = paging_allocation->allocation;


        if(paging_allocation->list_node)
        {
            if(paging_allocation->list_node->aligned_offset < (segment->heap.start + segment->heap.size))
            {
                heap_release(&segment->heap, paging_allocation->list_node);
            }
            else
            {
                gf_assert(segment->flags.small_heap_available, "segment->flags.small_heap_available");

                heap_release(&segment->small_heap, paging_allocation->list_node);
            }
        }


        /* mark allocation resident in system cache */
        vidmm_add_allocation_to_cache_list(adapter, paging_allocation->allocation);

        cm_device_lock_allocation(candidate->device, candidate);

        candidate->render_count[adapter->paging_engine_index]--;

        cm_device_unlock_allocation(candidate->device, candidate);
    }

    vidsch_task_dec_reference(adapter, adapter->paging_engine_index, &paging_task->desc);

    return result;
}

int vidmm_segment_unresident_allocations(adapter_t *adapter, unsigned int segment_id)
{
    int ret = S_OK;
    vidmm_segment_t *segment = &adapter->mm_mgr->segment[segment_id];
    gf_mutex_lock(adapter->paging_lock);

    ret = vidmmi_segment_unresident_allocations(adapter->mm_mgr, segment, 1);

    gf_mutex_unlock(adapter->paging_lock);
    return ret;
}

int vidmm_resident_one_allocation(gpu_device_t *device, void *ptask, vidmm_paging_allocation_t *paging_allocation, unsigned int segment_id)
{
    adapter_t          *adapter        = device->adapter;
    vidmm_mgr_t        *mm_mgr         = adapter->mm_mgr;
    vidmm_allocation_t *temp_allocation = NULL;
    vidmm_allocation_t *allocation     = paging_allocation->allocation;
    vidmm_segment_t    *segment        = NULL;
    task_paging_t      *paging_task    = ptask;

    unsigned int segment_id_list[MAX_SEGMENT_ID];
    unsigned int segment_id_count = 0, index;

    int try_again, result = E_FAIL;

    if(segment_id != SEGMENT_ID_INVALID)
    {
        segment_id_list[0] = segment_id;
        segment_id_count   = 1;
    }
    else
    {
        if(allocation->preferred_segment.segment_id_0)
        {
            segment_id_list[segment_id_count++] = allocation->preferred_segment.segment_id_0;
        }
        if(allocation->preferred_segment.segment_id_1)
        {
            segment_id_list[segment_id_count++] = allocation->preferred_segment.segment_id_1;
        }
        if(allocation->preferred_segment.segment_id_2)
        {
            segment_id_list[segment_id_count++] = allocation->preferred_segment.segment_id_2;
        }
        if(allocation->preferred_segment.segment_id_3)
        {
            segment_id_list[segment_id_count++] = allocation->preferred_segment.segment_id_3;
        }
        if(allocation->preferred_segment.segment_id_4)
        {
            segment_id_list[segment_id_count++] = allocation->preferred_segment.segment_id_4;
        }
    }

    temp_allocation = (vidmm_allocation_t *)gf_calloc(sizeof(vidmm_allocation_t));
    gf_assert(temp_allocation != NULL, "temp_allocation != NULL");

    temp_allocation->orig_size = allocation->orig_size;
    temp_allocation->alignment = allocation->alignment;
    temp_allocation->preferred_segment.value = allocation->preferred_segment.value;

    for(index = 0; index < segment_id_count; index++)
    {
        segment = &mm_mgr->segment[segment_id_list[index]];

        try_again = TRUE;

        while(try_again)
        {
            /* Step 1. try to allocate video memory */
            result = vidmmi_allocate_video_memory_try(mm_mgr, temp_allocation, &temp_allocation->preferred_segment);

            if(result == S_OK)
            {
                paging_allocation->segment_id    = temp_allocation->segment_id;
                paging_allocation->gpu_virt_addr = temp_allocation->phys_addr;
                paging_allocation->size          = temp_allocation->size;
                paging_allocation->list_node     = temp_allocation->list_node;

                /* Step 3. paging in allocation to list_node */
                gf_begin_section_trace_event("resident_allocation");
                gf_counter_trace_event("arg_allocation", paging_allocation->allocation->handle);
                gf_counter_trace_event("arg_segment_id", paging_allocation->segment_id);
                gf_counter_trace_event("arg_size", paging_allocation->size);
                result = vidmmi_paging_in(mm_mgr, paging_task, paging_allocation);
                gf_end_section_trace_event(result);

                if(result == S_OK)
                {
                    /* paging in success mark it referenced by render cmd */
                    cm_device_lock_allocation(allocation->device, allocation);

                    allocation->render_count[adapter->paging_engine_index]++;

                    allocation->write_render_count[adapter->paging_engine_index]++;

                    cm_device_unlock_allocation(allocation->device, allocation);

                    gf_free(temp_allocation);

                    return result;
                }
                else
                {
                    /* system memory exhausted or dma buffer not enough, return fail */
                    vidmmi_release_segment_memory(mm_mgr, temp_allocation->segment_id, temp_allocation->list_node);

                    temp_allocation->list_node  = NULL;
                    temp_allocation->segment_id = SEGMENT_ID_INVALID;

                    gf_free(temp_allocation);

                    return result;
                }
            }
            else
            {
                /* Step 2. paging out some video memory */

                /* paging out allocation to system cache to release some segment memory */

                result = vidmmi_segment_unresident_allocations(mm_mgr, segment, paging_task->must_success);

                if(result == S_OK)
                {
                    /* some segment memory released, try_again */
                    continue;
                }
                else if(result == E_FAIL)
                {
                    /* can not paging out segment memory, try another segment */
                    break;
                }
                else
                {
                    /* system memory exhausted, can not do paging anymore, return. */
                    gf_free(temp_allocation);
                    return result;
                }
            }
        }
    }
    gf_free(temp_allocation);
    return result;
}

/* prepare alloation used for lock allocation */
int vidmmi_prepare_one_allocation(gpu_device_t *device, vidmm_allocation_t *allocation)
{
    adapter_t                 *adapter           = device->adapter;
    vidmm_paging_allocation_t *paging_allocation = NULL;
    task_paging_t             *paging_task       = NULL;

    int result =S_OK;

    gf_mutex_lock(adapter->paging_lock);

    //hold the paging_lock for double check, in case of multi-thread issue.
    if(allocation->segment_id == SEGMENT_ID_INVALID)
    {
        paging_task = vidsch_allocate_paging_task(adapter, 8*1024, 1);

        paging_task->paging_allocation_num = 1;
        paging_task->must_success          = TRUE;

        vidsch_task_inc_reference(adapter, adapter->paging_engine_index, &paging_task->desc);

        paging_allocation = &paging_task->paging_allocation_list[0];

        paging_allocation->allocation = allocation;

        result = vidmm_resident_one_allocation(device, paging_task, paging_allocation, 0);

        vidsch_submit_paging_task(adapter, paging_task);

        vidsch_wait_fence_back(adapter, adapter->paging_engine_index, paging_task->desc.fence_id);

        if(result == S_OK)
        {
            allocation->list_node  = paging_allocation->list_node;
            allocation->phys_addr  = paging_allocation->gpu_virt_addr;
            allocation->segment_id = paging_allocation->segment_id;
            allocation->size       = paging_allocation->size;

            vidmm_add_allocation_to_resident_list(adapter, allocation);
        }

        vidsch_task_dec_reference(adapter, adapter->paging_engine_index, &paging_task->desc);
    }
    gf_mutex_unlock(adapter->paging_lock);	

    return result;
}

int vidmmi_get_swapable_pages_num(vidmm_mgr_t *mm_mgr)
{
    vidmm_segment_t *shrink_segment = &mm_mgr->segment[mm_mgr->shrink_segment_id];
    vidmm_segment_t *cache_segment  = &mm_mgr->segment[SEGMENT_ID_INVALID];

    return (cache_segment->pagable_size + shrink_segment->pagable_size) >> mm_mgr->adapter->os_page_shift;
}
/* swapout system cache to shmem 
 * return the real swap out size.
 */
#define MAX_SWAP_NUM 32

int vidmmi_do_swap_out(vidmm_mgr_t *mm_mgr, int need_pages)
{
    adapter_t       *adapter = mm_mgr->adapter;
    vidmm_segment_t *segment = &mm_mgr->segment[SEGMENT_ID_INVALID];
    vidmm_allocation_t *allocation, *allocation_next;
    vidmm_allocation_t *swap_list[MAX_SWAP_NUM];

    int i, real_swap_pages = 0, try_times = 20, swap_pages, swap_num, priority;

try_again:

    swap_num   = 0;
    swap_pages = real_swap_pages;

    /* STEP 1: select swap candidate list */
    gf_mutex_lock(segment->lock);

    for(priority = PDISCARD; priority < PALL; priority++)
    {
        list_for_each_entry_safe(allocation, allocation_next, &segment->pagable_resident_list[priority], list_item)
        {
            cm_device_lock_allocation(allocation->device, allocation);

            /* select a target to swap it to shmem */
            if(!allocation->status.temp_unpagable)
            {
                /* remove it from the system cache list */
                vidmmi_remove_allocation_from_cache_list(segment, allocation);

                /* mark allocation referenced */
                allocation->render_count[adapter->paging_engine_index]++;

                allocation->write_render_count[adapter->paging_engine_index]++;

                swap_list[swap_num++] = allocation;
                swap_pages += (allocation->orig_size + adapter->os_page_size - 1) / adapter->os_page_size;
            }

            cm_device_unlock_allocation(allocation->device, allocation);

            if((swap_num >= MAX_SWAP_NUM) || (swap_pages >= need_pages))
            {
                goto do_swap_out;
            }
        }
    }

do_swap_out:

    gf_mutex_unlock(segment->lock);

    /* STEP 2: try swap select allocations out */
    for(i = 0; i < swap_num; i++)
    {
        allocation = swap_list[i];

        /* swap system pages to shmem */
        allocation->file_storage = gf_pages_memory_swapout(allocation->pages_mem);

        //gf_info("shrink swapout %2d: allocation:%x, size: %dk.\n", i, allocation->handle, allocation->size >> 10);
        if(allocation->file_storage != NULL)
        {
            /* successful swapout, release allocation pages */
            vidmmi_release_system_memory(adapter, allocation);

            real_swap_pages += (allocation->orig_size + adapter->os_page_size - 1) / adapter->os_page_size;
        }
        else
        {
            /* swap out failed, restore it to cache list */
            vidmm_add_allocation_to_cache_list(adapter, allocation);
        }

        /* swap finished, unmark the reference */
        cm_device_lock_allocation(allocation->device, allocation);

        allocation->render_count[adapter->paging_engine_index]--;

        allocation->write_render_count[adapter->paging_engine_index]--;

        cm_device_unlock_allocation(allocation->device, allocation);
    }

    /* STEP3: check if need swap more */
    if((real_swap_pages < need_pages))
    {
        /* two way for swap more
         * 1. if previous swap_num == MAX mean cache segment have enough pages, still swap from it
         * 2. if previous swap_num not enough mean cache segment almost exhausted, need paging out some to it and try again
         */
        if(swap_num == MAX_SWAP_NUM)
        {
            goto try_again;
        }
        else if(try_times-- > 0)
        {
            vidmm_segment_t *shrink_segment = &mm_mgr->segment[mm_mgr->shrink_segment_id];

            vidmmi_segment_unresident_allocations(mm_mgr, shrink_segment, FALSE);

            goto try_again;
        }
    }

    vidsch_notify_interrupt(adapter, 0);
    //gf_info("swap size: want: %dk, real: %dk\n", need_pages << 2, real_swap_pages << 2);
    return real_swap_pages;
}

/* register swap func to OS to let swap pages memory to shmem,  when system pages exhausted */
int vidmm_shrink(void *shrink_argu, int nr_to_scan)
{
    adapter_t       *adapter = shrink_argu;
    vidmm_mgr_t     *mm_mgr  = adapter->mm_mgr;

    int used_pages;

    if(gf_mutex_trylock(adapter->paging_lock) == GF_LOCK_FAILED)
    {
        return 0;
    }

    //gf_info("shrink happen. nr:%d.\n", nr_to_scan);

    if(nr_to_scan == 0)
    {
        used_pages = vidmmi_get_swapable_pages_num(mm_mgr);
    }
    else
    {
        used_pages = vidmmi_do_swap_out(mm_mgr, nr_to_scan);
//        used_pages = (vidmmi_do_swap_out(mm_mgr, nr_to_scan) > 0) ? 
//                     vidmmi_get_swapable_pages_num(mm_mgr) : 0;
    }

    gf_mutex_unlock(adapter->paging_lock);

    return used_pages;
}

/* NOTE: save/restore do not use lock since save/restore used by pm func, 
 *       when here all app-thread/gf-thread all freezing
 */

int vidmm_save_allocation(gpu_device_t *device, vidmm_allocation_t *allocation)
{
    adapter_t       *adapter     = device->adapter;
    vidmm_mgr_t     *mm_mgr      = adapter->mm_mgr;
    vidmm_segment_t *segment     = &mm_mgr->segment[allocation->segment_id];
    task_paging_t   *paging_task = NULL;

    vidmm_paging_allocation_t *paging_allocation = NULL;

    int result = S_OK;

    /* only need save allocation which locate in local(VRAM on chip)
     * 1. if pages_mem != NULL means allocation currently locate in system, gart or system cache
     * 2. elite dynamic acctually is system memory, so no need save
     * 3. elite1000 reserved system memory.
     */
    if((allocation->pages_mem != NULL) || segment->flags.system_phys_mem_reserved || segment->flags.system_pages_un_reserved)
    {
        return result;
    }

    paging_task = vidsch_allocate_paging_task(adapter, 8*1024, 1);

    paging_task->paging_allocation_num = 1;
    paging_task->save_restore          = 1;

    paging_allocation = &paging_task->paging_allocation_list[0];

    paging_allocation->allocation    = allocation;
    paging_allocation->segment_id    = allocation->segment_id;
    paging_allocation->list_node     = allocation->list_node;
    paging_allocation->gpu_virt_addr = allocation->phys_addr;
    paging_allocation->size          = allocation->size;

    gf_begin_section_trace_event("save_allocation");
    gf_counter_trace_event("arg_allocation", allocation->handle);
    gf_counter_trace_event("arg_segment_id", allocation->segment_id);
    gf_counter_trace_event("arg_size", allocation->size);
    result = vidmmi_paging_out(mm_mgr, paging_task, paging_allocation);
    gf_end_section_trace_event(result);

    if(result == S_OK)
    {
        allocation->render_count[adapter->paging_engine_index]++;

        allocation->write_render_count[adapter->paging_engine_index]++;
    }
    else
    {
        gf_info("save allocation %x failed. size:%dk, errno:%d.\n", allocation->handle, allocation->size >> 10, result);
    }

    allocation->status.need_restore = TRUE;

    vidsch_task_inc_reference(adapter, adapter->paging_engine_index, &paging_task->desc);

    vidsch_submit_paging_task(adapter, paging_task);

    vidsch_wait_fence_back(adapter, adapter->paging_engine_index, paging_task->desc.fence_id);

    vidsch_task_dec_reference(adapter, adapter->paging_engine_index, &paging_task->desc);

    return result;
}

void vidmm_restore_allocation(gpu_device_t *device, vidmm_allocation_t *allocation)
{
    adapter_t       *adapter     = device->adapter;
    vidmm_mgr_t     *mm_mgr      = adapter->mm_mgr;
    task_paging_t   *paging_task = NULL;

    vidmm_paging_allocation_t *paging_allocation = NULL;

    int result;

    if(!allocation->status.need_restore)
    {
        return;
    }

    paging_task = vidsch_allocate_paging_task(adapter, 8*1024, 1);

    paging_task->paging_allocation_num = 1;
    paging_task->save_restore          = 1;

    paging_allocation = &paging_task->paging_allocation_list[0];

    paging_allocation->allocation    = allocation;
    paging_allocation->segment_id    = allocation->segment_id;
    paging_allocation->list_node     = allocation->list_node;
    paging_allocation->gpu_virt_addr = allocation->phys_addr;
    paging_allocation->size          = allocation->size;

    gf_begin_section_trace_event("restore_allocation");
    gf_counter_trace_event("arg_allocation", allocation->handle);
    gf_counter_trace_event("arg_segment_id", allocation->segment_id);
    gf_counter_trace_event("arg_size", allocation->size);
    result = vidmmi_paging_in(mm_mgr, paging_task, paging_allocation);
    gf_end_section_trace_event(result);

    if(result == S_OK)
    {
        allocation->render_count[adapter->paging_engine_index]++;

        allocation->write_render_count[adapter->paging_engine_index]++;
    }
    else
    {
        gf_info("restore allocation %x failed. size:%dk, errno:%d.\n", allocation->handle, allocation->size >> 10, result);
    }

    allocation->status.need_restore = FALSE;

    vidsch_task_inc_reference(adapter, adapter->paging_engine_index, &paging_task->desc);

    vidsch_submit_paging_task(adapter, paging_task);

    vidsch_wait_fence_back(adapter, adapter->paging_engine_index, paging_task->desc.fence_id);

    vidsch_task_dec_reference(adapter, adapter->paging_engine_index, &paging_task->desc);
}


void vidmm_prepare_and_mark_unpagable(adapter_t *adapter, vidmm_allocation_t *allocation, gf_open_allocation_t *info)
{
    gf_spin_lock(allocation->lock);
    ++allocation->status.force_unpagable;
    gf_spin_unlock(allocation->lock);

    if (allocation->segment_id == SEGMENT_ID_INVALID)
    {
        vidmmi_prepare_one_allocation(allocation->device, allocation);
    }

    if (info)
    {
        vidmm_fill_allocation_info(adapter, allocation, info);
    }
}

void vidmm_mark_pagable(adapter_t *adapter, vidmm_allocation_t *allocation)
{
    gf_spin_lock(allocation->lock);
    --allocation->status.force_unpagable;
    gf_spin_unlock(allocation->lock);
}
