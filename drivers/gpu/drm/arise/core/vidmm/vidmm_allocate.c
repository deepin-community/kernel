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
#include "global.h"
#include "vidmm.h"
#include "vidmmi.h"
#include "vidsch.h"

static unsigned int vidmmi_allocate_bl_slot(vidmm_mgr_t *mm_mgr, vidmm_allocation_t *allocation)
{
    unsigned int bl_size = allocation->size >> 9;// 1:512 compress rate
    heap_t * pblHeap = &mm_mgr->blheap;

    allocation->bl_node = heap_allocate(pblHeap, bl_size, pblHeap->alignment , 0);

    if(!allocation->bl_node)
    {
         gf_error("bl allocate failed, allo handle0x:%x, bl size:0x%x\n", allocation->handle, bl_size);
         vidmm_dump_bl_heap(mm_mgr->adapter);
         gf_assert(0, "bl allocate failed");
         return 0;
    }
    else
    {
        return gf_do_div(allocation->bl_node->aligned_offset - pblHeap->start ,pblHeap->alignment); //slice index
    }
}

static void vidmmi_free_bl_slot(vidmm_mgr_t *mm_mgr, vidmm_allocation_t *allocation)
{
    heap_release(&mm_mgr->blheap, allocation->bl_node);
}

unsigned int vidmmi_query_max_size_of_local_allocation(adapter_t *adapter)
{
    unsigned int max_size = 0;

    //gf_info("%s() fb segment size-0x%x, Real_vram_size-0x%x\n", __func__,  adapter->Visible_vram_size, adapter->Real_vram_size);
    if(adapter->Visible_vram_size == 512*1024 *1024)
    {
        max_size = (8*1024) * (2*1024); //8k*2k
    }
    else
    {
        max_size = (4*1024) * (2*1024);  //4k*2k
    }

    return max_size;
}

static list_node_t* vidmmi_allocate_segment_memory(vidmm_mgr_t *mm_mgr, int segment_id, unsigned int size, unsigned int alignment, int direction, int secured)
{
    list_node_t         *list_node  = NULL;
    vidmm_segment_t     *segment    = NULL;

    gf_assert(SEGMENT_ID_INVALID != segment_id, "SEGMENT_ID_INVALID != segment_id");

    segment = &mm_mgr->segment[segment_id];

    if(secured && !segment->flags.secure_range)
    {
        return NULL;
    }

    if(segment->flags.small_heap_available && (segment->small_heap_max_allocate_size >= size))
    {
        list_node = heap_allocate(&segment->small_heap, size, alignment, direction);
    }

    if(NULL == list_node)
    {
        list_node = heap_allocate(&segment->heap, size, alignment, direction);
    }

    return list_node;
}

void vidmmi_release_segment_memory(vidmm_mgr_t *mm_mgr, int segment_id, list_node_t *list_node)
{
    vidmm_segment_t *segment = NULL;

    segment = &mm_mgr->segment[segment_id];

    if(list_node)
    {
        if(list_node->aligned_offset < (segment->heap.start + segment->heap.size))
        {
            heap_release(&segment->heap, list_node);
        }
        else
        {
            gf_assert(segment->flags.small_heap_available, "segment->flags.small_heap_available");

            heap_release(&segment->small_heap, list_node);
        }
    }
}

/* NOTE: fill allocation is thread unsafe, this func can only used in vidmm_create_allocation
 *       before add allocation to resident list
 */

static void vidmmi_fill_allocation(vidmm_mgr_t *mm_mgr, vidmm_allocation_t *allocation, unsigned int fill_pattern)
{
    adapter_t *adapter = mm_mgr->adapter;
    vidmm_private_build_paging_buffer_arg_t build_paging_buffer   = {0};

    task_paging_t *paging_task = vidsch_allocate_paging_task(adapter, 16*4*1024, 1);

    int result = S_OK;

    gf_begin_section_trace_event("fill_allocation");
    gf_counter_trace_event("arg_allocation", allocation->handle);
    gf_counter_trace_event("arg_size", allocation->size);

    paging_task->paging_allocation_list[0].allocation = allocation;
    paging_task->paging_allocation_num                = 1;

    allocation->render_count[adapter->paging_engine_index]++;
    allocation->write_render_count[adapter->paging_engine_index]++;

    build_paging_buffer.allocation        = allocation;
    build_paging_buffer.operation         = BUILDING_PAGING_OPERATION_FILL;
    build_paging_buffer.dma_buffer        = paging_task->dma->virt_base_addr;
    build_paging_buffer.dma_size          = paging_task->dma->size;
    build_paging_buffer.multi_pass_offset = 0;
    build_paging_buffer.fill.fill_size    = allocation->size;
    build_paging_buffer.fill.fill_pattern = fill_pattern;
    build_paging_buffer.fill.phy_addr     = allocation->phys_addr;
    build_paging_buffer.fill.segment_id   = allocation->segment_id;

    result = mm_mgr->chip_func->build_paging_buffer(adapter, &build_paging_buffer);

    gf_assert(result == S_OK, "vidmmi_fill_allocation not sucess");

    paging_task->command_length = util_get_ptr_span(build_paging_buffer.dma_buffer, paging_task->dma->virt_base_addr);

    vidsch_submit_paging_task(adapter, paging_task);

    gf_end_section_trace_event(result);
}

/*
 * vidmmi_allocate_video_memroy_try will try muti segment_id specified on prefreered_segment.
 */
int vidmmi_allocate_video_memory_try(vidmm_mgr_t *mm_mgr, vidmm_allocation_t *allocation, vidmm_segment_preference_t *preferred_segment)
{
    list_node_t       *list_node  = NULL;
    int                segment_id = 0;
    int                direction  = 0;
    vidmm_segment_t   *segment    = NULL;
    heap_t            *heap       = NULL;
    int                result     = E_FAIL;

    /*
     * try to allocate list node by preferred segment list order
     */

    if(allocation->flag.try_secure)
    {
        segment_id  = mm_mgr->secure_segment_id;
        segment     = &mm_mgr->segment[segment_id];

        if((segment!= NULL) && segment->flags.secure_range)
        {
            list_node = vidmmi_allocate_segment_memory(mm_mgr, segment_id, allocation->orig_size, allocation->alignment, \
                    direction, allocation->flag.secured);
        }
    }

    if(list_node == NULL)
    {
        segment_id = preferred_segment->segment_id_0;
        direction  = preferred_segment->direction_0;

        if(SEGMENT_ID_INVALID != segment_id)
        {
            list_node = vidmmi_allocate_segment_memory(mm_mgr, segment_id, allocation->orig_size, allocation->alignment, \
                    direction, allocation->flag.secured);
        }
    }

    if(list_node == NULL)
    {
        gf_warning("!!!!!!!!!!!!!!!!!!!  kmd can't allocate first prefered segment %d , try allocate segment : %d !!!!!!!!!!!!!!!!!!!!!",
            preferred_segment->segment_id_0, preferred_segment->segment_id_1);
        segment_id = preferred_segment->segment_id_1;
        direction  = preferred_segment->direction_1;

        if(SEGMENT_ID_INVALID != segment_id)
        {
            list_node = vidmmi_allocate_segment_memory(mm_mgr, segment_id, allocation->orig_size, allocation->alignment, \
                    direction, allocation->flag.secured);
        }
    }

    if(list_node == NULL)
    {
        gf_warning("!!!!!!!!!!!!!!!!!!!  kmd can't allocate first prefered segment %d , try allocate segment : %d !!!!!!!!!!!!!!!!!!!!!",
            preferred_segment->segment_id_0, preferred_segment->segment_id_2);
        segment_id = preferred_segment->segment_id_2;
        direction  = preferred_segment->direction_2;

        if(SEGMENT_ID_INVALID != segment_id)
        {
            list_node = vidmmi_allocate_segment_memory(mm_mgr, segment_id, allocation->orig_size, allocation->alignment, \
                    direction, allocation->flag.secured);
        }
    }

    if(list_node == NULL)
    {
        gf_warning("!!!!!!!!!!!!!!!!!!!  kmd can't allocate first prefered segment %d , try allocate segment : %d !!!!!!!!!!!!!!!!!!!!!",
            preferred_segment->segment_id_0, preferred_segment->segment_id_3);
        segment_id = preferred_segment->segment_id_3;
        direction  = preferred_segment->direction_3;
        segment = &mm_mgr->segment[segment_id];

        if(SEGMENT_ID_INVALID != segment_id)
        {
            list_node = vidmmi_allocate_segment_memory(mm_mgr, segment_id, allocation->orig_size, allocation->alignment, \
                    direction, allocation->flag.secured);
        }
    }

    if(list_node == NULL)
    {
        gf_warning("!!!!!!!!!!!!!!!!!!!  kmd can't allocate first prefered segment %d , try allocate segment : %d !!!!!!!!!!!!!!!!!!!!!",
            preferred_segment->segment_id_0, preferred_segment->segment_id_4);
        segment_id = preferred_segment->segment_id_4;
        direction  = preferred_segment->direction_4;

        if(SEGMENT_ID_INVALID != segment_id)
        {
            list_node = vidmmi_allocate_segment_memory(mm_mgr, segment_id, allocation->orig_size, allocation->alignment, \
                    direction, allocation->flag.secured);
        }
    }

    /*
     * if list_node is NULL, then memory space is not enough.
     */
    if(list_node != NULL)
    {
        segment = &mm_mgr->segment[segment_id];

        allocation->segment_id  = segment_id;
        allocation->size        = util_align(allocation->orig_size, segment->segment_alignment);
        allocation->phys_addr   = list_node->aligned_offset;
        allocation->list_node   = list_node;
        allocation->cpu_phys_addr= allocation->phys_addr - segment->gpu_vm_start + segment->phys_addr_start;

        if(list_node->aligned_offset >= (segment->heap.start + segment->heap.size))
        {
            gf_assert(segment->flags.small_heap_available, "segment->flags.small_heap_available");

            allocation->flag.unpagable = TRUE;
        }

        result = S_OK;
    }
    else
    {
        int id = 0;

        vidmm_trace("%s(): allocate video memory failed.\n", __func__);
        vidmm_trace("requested_size: 0x%x, requeseted_segment:0x%08x.\n", allocation->orig_size, preferred_segment->value);
        for(id=1; id<mm_mgr->segment_cnt; id++)
        {
                segment = &mm_mgr->segment[id];
                heap    = &segment->heap;
                vidmm_trace("heap: ID:%d, size available: 0x%08x, flag: 0x%x\n", id, heap->size, segment->flags);
        }
        result = E_FAIL;
    }

    vidmm_trace("create_video_memroy: allocation:0x%p, handle:0x%x. phys:0x%llx, size:0x%08x compress_format 0x%x segment_id %x at type %x direction %x secure flag %x\n",
        allocation, allocation->handle, allocation->phys_addr, allocation->size, allocation->compress_format, allocation->segment_id, allocation->at_type, allocation->preferred_segment.direction_0, allocation->flag.secured);

    return result;
}

void vidmmi_release_video_memory(vidmm_mgr_t *mm_mgr, vidmm_allocation_t *allocation)
{
//    vidmm_segment_t *segment = &mm_mgr->segment[allocation->segment_id];

    allocation->snooping_enabled = FALSE;

    /* unmap acctually not needed, just for debug purpose or patch HW bug like HW access out allocation boundary,
     * so when unmap need internally map a dummp page to Gart
     */

    /* if page mem not NULL and have valid segment id, mean Gart memory */
    if((allocation->pages_mem != NULL) && (allocation->segment_id != SEGMENT_ID_INVALID))
    {
        mm_mgr->chip_func->unmap_gart_table(mm_mgr->adapter, allocation);
    }

    if(allocation->list_node)
    {
        vidmmi_release_segment_memory(mm_mgr, allocation->segment_id, allocation->list_node);

        allocation->list_node  = NULL;
        allocation->phys_addr  = 0;
        allocation->size       = 0;
        allocation->segment_id = SEGMENT_ID_INVALID;
    }
}

#define SWAP_OUT_PAGES_SIZE 32*1024*1024

int vidmmi_allocate_system_memory(adapter_t *adapter, vidmm_allocation_t *allocation, int must_success, int paging_locked)
{
    vidmm_mgr_t       *mm_mgr  = adapter->mm_mgr;
    vidmm_segment_t   *segment = NULL;

    int         result  = S_OK, try_times = 5;

    if(NULL == allocation->pages_mem)
    {
        unsigned int page_num  = (allocation->orig_size + adapter->os_page_size - 1) / adapter->os_page_size;
        unsigned int max_pages = must_success ?
                                 mm_mgr->max_pages_num + mm_mgr->emergency_pages :
                                 mm_mgr->max_pages_num;
        int          can_alloc = FALSE;

        gf_mutex_lock(mm_mgr->hw_lock);

        if((mm_mgr->own_pages_num + page_num) < max_pages)
        {
            mm_mgr->own_pages_num += page_num;
            mm_mgr->used_pages_num += page_num;
            can_alloc = TRUE;
        }

        gf_mutex_unlock(mm_mgr->hw_lock);

        if(can_alloc)
        {
            alloc_pages_flags_t alloc_flags = {0};

            int page_size = 0;

            segment = allocation->segment_id ? &mm_mgr->segment[allocation->segment_id] : mm_mgr->paging_segment;

            alloc_flags.dma32      = !adapter->hw_caps.address_mode_64bit;
            alloc_flags.need_flush = !segment->flags.support_snoop;
            alloc_flags.fixed_page = TRUE;
            alloc_flags.need_dma_map = adapter->sys_caps.iommu_enabled;
            page_size = segment->flags.page_64kb_enable ? GF_PAGE_64KB_SIZE : adapter->os_page_size;

            allocation->pages_mem  = gf_allocate_pages_memory(adapter->os_device.pdev, allocation->orig_size, page_size, alloc_flags);

            if(allocation->pages_mem == NULL)
            {
                gf_mutex_lock(mm_mgr->hw_lock);

                mm_mgr->used_pages_num -= page_num;
                mm_mgr->own_pages_num -= page_num;
                gf_mutex_unlock(mm_mgr->hw_lock);
            }
        }

        if(allocation->pages_mem == NULL)
        {
            gf_info("allocate %x system pages failed. already used size: %dK, request size: %dk.\n",
                allocation->handle, mm_mgr->used_pages_num << 2, page_num << 2);
        }

        result = allocation->pages_mem ? S_OK : E_FAIL;
    }

    return result;
}

void vidmmi_release_system_memory(adapter_t *adapter, vidmm_allocation_t *allocation)
{
    vidmm_mgr_t *mm_mgr = adapter->mm_mgr;

    if(allocation->pages_mem)
    {
        int own_page = gf_is_own_pages(allocation->pages_mem);
        unsigned int page_num = (allocation->orig_size + adapter->os_page_size - 1) / adapter->os_page_size;

        gf_free_pages_memory(adapter->os_device.pdev, allocation->pages_mem);

        gf_mutex_lock(mm_mgr->hw_lock);

        mm_mgr->used_pages_num -= page_num;
        if(own_page) mm_mgr->own_pages_num -= page_num;
        gf_mutex_unlock(mm_mgr->hw_lock);

        allocation->pages_mem = NULL;
    }
}

void vidmm_map_gart_table(adapter_t *adapter, vidmm_allocation_t *allocation, int snooping)
{
    vidmm_mgr_t     *mm_mgr = adapter->mm_mgr;

#ifndef GF_HW_NULL
    mm_mgr->chip_func->map_gart_table(adapter, allocation, snooping);

    allocation->snooping_enabled = snooping;
#endif
}

void vidmmi_destroy_allocation(adapter_t *adapter, vidmm_allocation_t *allocation)
{
    vidmm_mgr_t *mm_mgr = adapter->mm_mgr;

    vidmmi_release_video_memory(mm_mgr, allocation);

    vidmmi_release_system_memory(adapter, allocation);

    if(allocation->file_storage)
    {
        gf_release_file_storage(allocation->file_storage);

        allocation->file_storage = NULL;
        mm_mgr->swap_page_num -= (allocation->orig_size + mm_mgr->adapter->os_page_size - 1) / mm_mgr->adapter->os_page_size;
    }

    if(allocation->sync_obj != 0)
    {
        sync_trace("destroy allocation binding fence sync: %x, sync_obj:%x.\n", allocation->handle, allocation->sync_obj);

        vidsch_destroy_sync_object(adapter, allocation->device, allocation->sync_obj);
    }

    if(allocation->compress_format != 0 && allocation->slot_index )
    {
        vidmmi_free_bl_slot(mm_mgr, allocation);
        allocation->slot_index = 0;
    }
    remove_handle(&adapter->hdl_mgr, allocation->handle);

    cm_device_create_num_dec(allocation->device);

    if(allocation->lock)
    {
        gf_destroy_spinlock(allocation->lock);
    }

    gf_free(allocation);
}

void vidmm_destroy_defer_allocation(adapter_t *adapter)
{
    vidmm_mgr_t        *mm_mgr     = adapter->mm_mgr;
    vidmm_allocation_t *allocation = NULL;
    vidmm_allocation_t *allocation_next;
    struct list_head    destroy_list;

    INIT_LIST_HEAD(&destroy_list);
    gf_mutex_lock(mm_mgr->list_lock);
    list_splice(&mm_mgr->defer_destroy_list, &destroy_list);
    list_init_head(&mm_mgr->defer_destroy_list);
    gf_mutex_unlock(mm_mgr->list_lock);

    list_for_each_entry_safe(allocation, allocation_next, &destroy_list, destroy_list_item)
    {
        int idle;
        /*lock allocation because prevent free allocation before unlock in other thread*/
        if(gf_spin_try_lock(allocation->lock) == GF_LOCK_FAILED) continue;
        idle = vidsch_is_allocation_idle(adapter, ALL_ENGINE_MASK, allocation);
        gf_spin_unlock(allocation->lock);

        if(idle)
        {
            list_del(&allocation->destroy_list_item);
            if(allocation->segment_id != SEGMENT_ID_INVALID)
            {
                vidmm_remove_allocation_from_resident_list(adapter, allocation);
            }
            else if(allocation->pages_mem != NULL)
            {
                vidmm_remove_allocation_from_cache_list(adapter, allocation);
            }
            vidmmi_destroy_allocation(adapter, allocation);
        }
    }

    gf_mutex_lock(mm_mgr->list_lock);
    list_splice(&destroy_list,&mm_mgr->defer_destroy_list);
    gf_mutex_unlock(mm_mgr->list_lock);
}

int vidmm_create_allocation(gpu_device_t *device, vidmm_create_allocation_arg_t *create)
{
    adapter_t        *adapter  = device->adapter;
    vidmm_mgr_t      *mm_mgr   = adapter->mm_mgr;
    int index = 0, result = S_OK;

    if(create->allocation_count <= 0)
    {
        result = E_INVALIDARG;
        gf_error("%s: create->allocation_count <= 0.\n", __func__);
        return result;
    }

    vidmm_destroy_defer_allocation(adapter);


    /* Three steps when create an allocation
     * 1. describe allocation to get create info from chip layer.
     * 2. create gpu virtual address range.
     * 3. create system memory and map it for gart memory. (local already have vram bind to, no need)
     */

    for(index = 0; index < create->allocation_count; index++)
    {
        vidmm_describe_allocation_t desc_info   = {0};
        vidmm_allocation_t          *allocation = gf_calloc(sizeof(vidmm_allocation_t));
        vidmm_segment_t             *segment    = NULL;

        int create_system_cache = FALSE;

        if(allocation == NULL)
        {
            gf_error("%s: allocate allocation struct memory failed.\n", __func__);
            result = E_OUTOFMEMORY;
            goto exit;
        }

        allocation->handle              = add_handle(&adapter->hdl_mgr, HDL_TYPE_ALLOCATION, allocation);
        allocation->ref_count           = 1;
        allocation->device              = device;

        list_init_head(&allocation->device_ref_list);
        list_init_head(&allocation->list_item);
        list_init_head(&allocation->device_ref_list);


        allocation->lock             = gf_create_spinlock(0);

        cm_device_create_num_inc(device);

        /* Step 1: describe allocation */
        if(create->create_type == VIDMM_CREATE_TYPE_ESCAPE)
        {
            gf_create_alloc_info_t *desc = create->info_list + index;

            allocation->orig_size       = desc->Size;
            allocation->hw_format       = desc->HwFormat;
            allocation->at_type         = desc->AtType;
            allocation->priority        = desc->Priority;
            allocation->alignment       = desc->Alignment;
            allocation->preferred_segment_raw.value = desc->PreferredSegment;

            allocation->bit_count       = desc->BitCount;
            allocation->width           = desc->Width;
            allocation->height          = desc->Height;
            allocation->pitch           = desc->Pitch;

            allocation->flag.cpu_visible = desc->CpuVisible;
            allocation->flag.primary     = desc->Primary;
            allocation->flag.swizzled    = desc->Swizzled;
            allocation->flag.video       = desc->Video;
            allocation->flag.overlay     = desc->Overlay;
            allocation->flag.camera      = desc->Camera;
            allocation->flag.none_snooping = desc->bNonSnoop;
            allocation->flag.unpagable   = desc->Unpagable | !adapter->ctl_flags.paging_enable;
            allocation->flag.fence_sync  = desc->FenceSync;

            desc_info.create_info = desc;
        }
        else if(create->create_type == VIDMM_CREATE_TYPE_GF)
        {
            gf_create_allocation_t *create_hint = create->create_list[index].create_data;

            allocation->flag.fence_sync = create_hint->fence_sync;

            allocation->flag.primary = create_hint->primary;

            desc_info.create_gf = create->create_list + index;
        }
        else if(create->create_type == VIDMM_CREATE_TYPE_RENAME)
        {
            vidmm_allocation_t *src_allocation = create->rename_list[index].reference;

            allocation->alignment = src_allocation->alignment;
            allocation->orig_size = src_allocation->orig_size;
            allocation->hw_format = src_allocation->hw_format;
            allocation->bit_count = src_allocation->bit_count;
            allocation->priority  = src_allocation->priority;
            allocation->pitch     = src_allocation->pitch;
            allocation->at_type   = src_allocation->at_type;
            allocation->width     = src_allocation->width;
            allocation->height    = src_allocation->height;
            allocation->flag      = src_allocation->flag;

            allocation->aligned_width  = src_allocation->aligned_width;
            allocation->aligned_height = src_allocation->aligned_height;
            allocation->tiled_width    = src_allocation->tiled_width;
            allocation->tiled_height   = src_allocation->tiled_height;
            allocation->compress_format= src_allocation->compress_format;

            allocation->preferred_segment_raw.value = src_allocation->preferred_segment_raw.value;
            allocation->preferred_segment.value     = src_allocation->preferred_segment.value;
        }
        else
        {
            gf_error("create_type = %d", create->create_type);
        }

        if(create->create_type != VIDMM_CREATE_TYPE_RENAME)
        {
            desc_info.allocation = allocation;

            result = mm_mgr->chip_func->describe_allocation(adapter, &desc_info);

            if(result != S_OK)
            {
                gf_error("%s: describe_allocation failed.\n", __func__);
                goto exit;
            }
        }

        /* Step 2: create video memory for allocation */

        if(allocation->flag.primary)
        {
            /* direction = 1 have problem when vt switch do not know why? */
            allocation->preferred_segment.direction_0 = 0;
        }

#ifdef GF_HW_NULL
        if(adapter->hw_caps.local_only == FALSE)
        {
            allocation->preferred_segment.segment_id_0 = 2;
        }
#endif
        result = vidmmi_allocate_video_memory_try(mm_mgr, allocation, &allocation->preferred_segment);

        if(result != S_OK)
        {
            int force_create = FALSE;

            /* rename not do paging, just return fail */
            if(!adapter->ctl_flags.paging_enable || (create->create_type == VIDMM_CREATE_TYPE_RENAME))
            {
                gf_error("%s: allocate video memory failed while paging disabled, need size:%lld, AT:%d,prefer:%d:%d:%d, prefered raw:%d:%d:%d\n",
                    __func__,allocation->size, allocation->at_type,
                    allocation->preferred_segment.segment_id_0,allocation->preferred_segment.segment_id_1,allocation->preferred_segment.segment_id_2,
                    allocation->preferred_segment_raw.segment_id_0,allocation->preferred_segment_raw.segment_id_1,allocation->preferred_segment_raw.segment_id_2);
                goto exit;
            }

            if(allocation->flag.primary || allocation->flag.unpagable || allocation->compress_format != 0)
            {
                force_create = TRUE;
            }
            else
            {
                create_system_cache = TRUE;
            }

            if(force_create)
            {
                int paging_result, try_times = 0;

                segment = &mm_mgr->segment[allocation->preferred_segment.segment_id_0];

            try_again:

                gf_mutex_lock(adapter->paging_lock);

                paging_result = vidmmi_segment_unresident_allocations(mm_mgr, segment, FALSE);

                gf_mutex_unlock(adapter->paging_lock);

                result = vidmmi_allocate_video_memory_try(mm_mgr, allocation, &allocation->preferred_segment);

                if(result != S_OK)
                {
                    if(paging_result == S_OK)
                    {
                        goto try_again;
                    }
                     else if(try_times < 100)
                    {
                        gf_msleep(10);
                        try_times++;
                        if(try_times == 100)
                        {
                            if((allocation->backup_segment_id != 0)&&(allocation->preferred_segment.segment_id_1 == 0))
                            {
                                allocation->preferred_segment.segment_id_1 = allocation->backup_segment_id;
                                gf_info("force create allocation[0x%d-%dK] failed by not enough local memory, add segment %d to preferred and try again.\n", allocation->handle, allocation->orig_size/1024, allocation->backup_segment_id);
                            }
                        }
                        goto try_again;
                    }
                    else
                    {
                        gf_info("force create failed. size: %dk. preferred_segment: %x,\n", allocation->orig_size >> 10, allocation->preferred_segment.value);
                        goto exit;
                    }
                }
            }
        }


        /*[E3K]: allcoate burst length for compressformat allocation*/
        if(allocation->compress_format != 0)
        {
            allocation->slot_index = vidmmi_allocate_bl_slot(mm_mgr, allocation);
        }


        /* Step 3: create system memory and map it to gart table */
        segment = &mm_mgr->segment[allocation->segment_id];

        if(create_system_cache || segment->flags.require_system_pages)
        {
            if (create->create_type == VIDMM_CREATE_TYPE_GF && create->create_list[index].import_pages_mem)
            {
                gf_mutex_lock(mm_mgr->hw_lock);
                mm_mgr->used_pages_num += (allocation->size + adapter->os_page_size - 1) / adapter->os_page_size;
                gf_mutex_unlock(mm_mgr->hw_lock);

                allocation->pages_mem = create->create_list[index].import_pages_mem;
                result = S_OK;
            }
            else
            {
                result = vidmmi_allocate_system_memory(adapter, allocation, FALSE, FALSE);
            }

            if(result == S_OK)
            {
                if(!create_system_cache)
                {
                    vidmm_map_gart_table(adapter, allocation, segment->flags.support_snoop);
                }
            }
            else
            {
                gf_error("%s: allocate system memory failed.\n", __func__);
                goto exit;
            }
        }

        /* create finished, zero video memory */
        if(allocation->segment_id != SEGMENT_ID_INVALID)
        {
            /* Patch:
             * zero video memory for 2d and video to fix garbage issue
             * zero the follow video memory..
             *    (1) on chip memory
             *    (2) dynamic memory for elite
             */
            int is_fill_alloc = FALSE;

            segment = &mm_mgr->segment[allocation->segment_id];

            if(allocation->compress_format != 0 || allocation->flag.force_clear)
            {
                is_fill_alloc = TRUE;
            }
            else if(segment->flags.system_phys_mem_reserved || segment->flags.system_pages_reserved || segment->flags.chip_phys_mem_reserved)
            {
                is_fill_alloc = TRUE;

                if(create->create_type == VIDMM_CREATE_TYPE_GF)
                {
                   gf_create_allocation_t *create_hint = create->create_list[index].create_data;
                   if(create_hint->usage_mask & GF_USAGE_CREATE_PIXMAP)
                   {
                      is_fill_alloc = FALSE;
                   }
                }
            }

            if(is_fill_alloc)
            {
               allocation->flag.force_clear = TRUE;
               vidmmi_fill_allocation(mm_mgr, allocation, 0);
            }
        }

        /* step4: create sync object and binding it to this allocation */
        if(allocation->flag.fence_sync)
        {
            gf_create_fence_sync_object_t create_sync = {0};
            int status;

            create_sync.type             = GF_SYNC_OBJ_TYPE_FENCE;
            create_sync.fence.init_value = 0; /* initialized value set to 0 */

            status = vidsch_create_sync_object(device, &create_sync, TRUE);

            if(status == S_OK)
            {
                allocation->sync_obj   = create_sync.fence_sync_object;
                allocation->fence_addr = create_sync.fence.fence_addr;
            }
            else
            {
                gf_error("create allocation binding FenceSyncOject failed. device: %x, allocation: %x.\n", device->handle, allocation->handle);
            }

            sync_trace("device: %x, allocation:%x,  %dx%d@%d, sync_obj: %x, fence_addr:%llx.\n",
                 device->handle, allocation->handle, allocation->width, allocation->height, allocation->bit_count,
                 allocation->sync_obj, allocation->fence_addr);
        }

        if(create->create_type == VIDMM_CREATE_TYPE_ESCAPE)
        {
            gf_create_alloc_info_t *desc = create->info_list + index;
            if(allocation->flag.unpagable)
            {
                desc->Address = allocation->phys_addr;
                desc->PhysAddr= allocation->cpu_phys_addr;
            }
            else
            {
                desc->Address = 0x0ll;
                desc->PhysAddr= 0x0ll;
            }

            desc->FenceSyncObject   = allocation->sync_obj;
            desc->FenceAddress      = allocation->fence_addr;

            desc->hAllocation   = allocation->handle;
            desc->Size          = allocation->size;
        }
        else if(create->create_type == VIDMM_CREATE_TYPE_GF)
        {
            gf_create_allocation_t *create_hint = create->create_list[index].create_data;

            create_hint->width           = allocation->width;
            create_hint->height          = allocation->height;

            create_hint->width_aligned   = allocation->aligned_width;
            create_hint->height_aligned  = allocation->aligned_height;

            create_hint->tiled_width     = allocation->tiled_width;
            create_hint->tiled_height    = allocation->tiled_height;

            create_hint->size            = allocation->size;
            create_hint->bit_cnt         = allocation->bit_count;
            create_hint->pitch           = allocation->pitch;
            create_hint->tiled           = allocation->flag.swizzled;
            create_hint->unpagable       = allocation->flag.unpagable;
            create_hint->compressed      = (allocation->compress_format != 0) ? 1:0;
            create_hint->allocation      = allocation->handle;
            create_hint->gpu_virt_addr   = allocation->phys_addr;
            create_hint->tiled_width     = allocation->tiled_width;
            create_hint->sync_obj        = allocation->sync_obj;
            create_hint->fence_addr      = allocation->fence_addr;
            create_hint->hw_format       = allocation->hw_format;
            create_hint->compressed      = allocation->compress_format != 0 ? 1 : 0;
            create_hint->compress_format = allocation->compress_format;
            create_hint->segment_id      = allocation->segment_id;
            create_hint->force_clear     = allocation->flag.force_clear;
            create_hint->has_pages       = segment->flags.require_system_pages;
        }
        else if (create->create_type == VIDMM_CREATE_TYPE_RENAME)
        {
            create->rename_list[index].Size = allocation->size;
            create->rename_list[index].hAllocation = allocation->handle;
        }
    }

#define VIDMM_GET_ALLOCATION(create, index) \
    (((create)->create_type == VIDMM_CREATE_TYPE_GF) ? (create)->create_list[(index)].create_data->allocation : \
        ((create)->create_type == VIDMM_CREATE_TYPE_ESCAPE ? (create)->info_list[(index)].hAllocation : \
         (create)->rename_list[(index)].hAllocation))

exit:
    if(result == S_OK)
    {
        vidmm_allocation_t *allocation = NULL;

        for(index = 0; index < create->allocation_count; index++)
        {
            allocation = get_from_handle(&adapter->hdl_mgr, VIDMM_GET_ALLOCATION(create, index));

            allocation->bo = create->bos ? create->bos[index] : NULL;

            if(allocation->segment_id != SEGMENT_ID_INVALID)
            {
                cm_device_add_reference(device,allocation);
                vidmm_add_allocation_to_resident_list(adapter, allocation);
            }
            else
            {
                vidmm_add_allocation_to_cache_list(adapter, allocation);
            }
        }
    }
    else
    {
        for(index = 0; index < create->allocation_count; index++)
        {
            vidmm_allocation_t *allocation = get_from_handle(&adapter->hdl_mgr, VIDMM_GET_ALLOCATION(create, index));

            if(allocation)
            {
                cm_device_del_reference(device, allocation);

                vidmmi_destroy_allocation(adapter, allocation);
            }
        }
    }

    return result;
}

void vidmm_reference_allocation(adapter_t *adapter, gpu_device_t *device, vidmm_allocation_t *allocation)
{
    if (device)
    {
        cm_device_lock_allocation(device, allocation);
    }
    else
    {
        gf_spin_lock(allocation->lock);
    }

    ++allocation->ref_count;

    if (device)
    {
        cm_device_unlock_allocation(device, allocation);
    }
    else
    {
        gf_spin_unlock(allocation->lock);
    }
}


void vidmm_unreference_allocation(adapter_t *adapter, gpu_device_t *device, vidmm_allocation_t *allocation)
{
    int can_destroy = FALSE, idle = FALSE;
    vidmm_mgr_t *mm_mgr = adapter->mm_mgr;

    if (device)
    {
        cm_device_lock_allocation(device, allocation);
    }
    else
    {
        gf_spin_lock(allocation->lock);
    }

    if(--allocation->ref_count == 0)
    {
        can_destroy = TRUE;

        allocation->status.wait_for_destroy = TRUE;

        idle = vidsch_is_allocation_idle(adapter, ALL_ENGINE_MASK, allocation);
    }
    else
    {
        can_destroy = FALSE;
    }

    if (device)
    {
        cm_device_unlock_allocation(device, allocation);
    }
    else
    {
        gf_spin_unlock(allocation->lock);
    }

    if(can_destroy)
    {
        if(idle)
        {
            if(allocation->segment_id != SEGMENT_ID_INVALID)
            {
                vidmm_remove_allocation_from_resident_list(adapter, allocation);
            }
            else if(allocation->pages_mem != NULL)
            {
                vidmm_remove_allocation_from_cache_list(adapter, allocation);
            }
            vidmmi_destroy_allocation(adapter, allocation);
        }
        else
        {
            gf_mutex_lock(mm_mgr->list_lock);

            list_add_tail(&allocation->destroy_list_item, &mm_mgr->defer_destroy_list);

            gf_mutex_unlock(mm_mgr->list_lock);
        }
    }
}

void vidmmi_try_destroy_one_allocation(gpu_device_t *device, vidmm_allocation_t *allocation)
{
    adapter_t          *adapter     = device->adapter;

    cm_device_del_reference(device, allocation);

    vidmm_unreference_allocation(adapter, device, allocation);
}


void vidmm_destroy_allocation(gpu_device_t *device, vidmm_destroy_allocatin_arg_t *data)
{
//    adapter_t          *adapter     = device->adapter;
//    vidmm_mgr_t        *mm_mgr      = adapter->mm_mgr;
    vidmm_allocation_t *allocation  = NULL;

    int allocation_count = 0;
    int i = 0;

    /* NOTE: here not protect resource list when destroy resource, since general when destroy resource called,
     *       this resource is invalid, if on destroy, resource used by create, this should be a user mode driver bug.
     */

    allocation_count = data->allocation_count;

    for(i = 0; i < allocation_count; i++)
    {
        allocation = data->allocation_list[i];

        //gf_assert(NULL != allocation);

        if (allocation)
        {
            allocation->bo = NULL;

            vidmmi_try_destroy_one_allocation(device, allocation);
        }
        else
        {
            gf_info("try destroy NULL allocation\n");
        }
    }
}


int vidmm_get_allocation_info(adapter_t *adapter, vidmm_get_allocation_info_t *info)
{
    vidmm_mgr_t        *mm_mgr     = adapter->mm_mgr;
    vidmm_allocation_t *allocation = info->allocation;

    info->gpu_vm_addr = allocation->phys_addr;
    info->pitch       = allocation->pitch;
    info->tiled       = allocation->flag.swizzled;
    info->segment_id  = allocation->segment_id;
    info->hw_format   = allocation->hw_format;

    mm_mgr->chip_func->get_allocation_info(adapter, info);

    return S_OK;
}

void vidmmi_dump_video_memory(vidmm_mgr_t *mm_mgr, unsigned int segment_id)
{
    vidmm_segment_t *segment = &mm_mgr->segment[segment_id];

    if(NULL != segment)
    {
        heap_dump(&segment->heap);
    }
}

vidmm_segment_memory_t *vidmm_allocate_segment_memory(adapter_t *adapter, unsigned int segment_id, unsigned int size, int direction)
{
    vidmm_mgr_t             *mm_mgr          = adapter->mm_mgr;
    vidmm_segment_t         *segment         = &mm_mgr->segment[segment_id];
    vidmm_segment_memory_t  *segment_memory  = NULL;
    list_node_t             *list_node       = NULL;
    struct os_pages_memory  *pages_mem       = NULL;

    segment_memory = gf_calloc(sizeof(vidmm_segment_memory_t));
    if(segment_memory == NULL)
    {
        gf_error("%s(): allocate system memory failed\n", __func__);
        goto exit;
    }

    segment_memory->segment_id = segment_id;
    list_node  = vidmmi_allocate_segment_memory(mm_mgr, segment_id, size, adapter->os_page_size, direction, 0);
    if(list_node == NULL)
    {
        gf_error("%s(): allocate memory from segment failed\n", __func__);
        gf_free(segment_memory);
        segment_memory = NULL;
        goto exit;
    }

    segment_memory->gpu_virt_addr = list_node->aligned_offset;

    /* allocate system pages  */
    if(segment->flags.require_system_pages)
    {
        alloc_pages_flags_t alloc_flags  = {0};
        vidmm_allocation_t  *allocation   = NULL;
        int                 page_size;

        alloc_flags.dma32      = !adapter->hw_caps.address_mode_64bit;
        alloc_flags.need_flush = !segment->flags.support_snoop;
        alloc_flags.fixed_page = TRUE;
        alloc_flags.need_dma_map = adapter->sys_caps.iommu_enabled;
        page_size = segment->flags.page_64kb_enable ? GF_PAGE_64KB_SIZE : adapter->os_page_size;

        allocation = gf_calloc(sizeof(vidmm_allocation_t));
        if (allocation == NULL)
        {
            goto exit;
        }
        pages_mem = gf_allocate_pages_memory(adapter->os_device.pdev, list_node->aligned_size, page_size, alloc_flags);
        if(pages_mem == NULL)
        {
            gf_error("%s(): allocate pages failed\n", __func__);
            vidmmi_release_segment_memory(mm_mgr, segment_id, list_node);
            gf_free(segment_memory);
            segment_memory = NULL;
            goto exit;
        }

        /* map gart table */
        allocation->phys_addr  = segment_memory->gpu_virt_addr;
        allocation->size       = list_node->aligned_size;
        allocation->pages_mem  = pages_mem;

        vidmm_map_gart_table(adapter, allocation, segment->flags.support_snoop);
        gf_free(allocation);
    }

    segment_memory->list_node = list_node;
    segment_memory->pages_mem = pages_mem;

    segment->reserved_size += list_node->aligned_size;

exit:
    return segment_memory;
}

void vidmm_release_segment_memory(adapter_t *adapter, vidmm_segment_memory_t *segment_memory)
{
    vidmm_mgr_t      *mm_mgr  = adapter->mm_mgr;
    vidmm_segment_t  *segment = &mm_mgr->segment[segment_memory->segment_id];

    if(segment_memory->pages_mem)
    {
        gf_free_pages_memory(adapter->os_device.pdev, segment_memory->pages_mem);

        segment_memory->pages_mem = NULL;
    }

    segment->reserved_size -= segment_memory->list_node->aligned_size;

    vidmmi_release_segment_memory(mm_mgr, segment_memory->segment_id, segment_memory->list_node);

    gf_free(segment_memory);
}

/*
 *  For map segment memory, you can set your own cache flags, but this cache flag may conflict with segment caps.
 *  like you may set wb type to a none snoop segment, on this case, user should explictly do flush cache.
 *  So after map segment, user should check flags vma->need_flush_cache, if set TRUE, user must flush cache manually
 *  before GPU use to avoid coherence issue.
 */

void* vidmm_map_segment_memory(adapter_t *adapter, void *filp, vidmm_segment_memory_t *segment_memory, vidmm_map_flags_t *flags)
{
    vidmm_mgr_t     *mm_mgr     = adapter->mm_mgr;
    unsigned int     segment_id = segment_memory->segment_id;
    vidmm_segment_t *segment    = &mm_mgr->segment[segment_id];
    gf_map_argu_t   map        = {0};
    gf_vm_area_t   *vma        = NULL;

    if(flags->mem_space == GF_MEM_KERNEL)
    {
        vma = segment_memory->vma;
    }
    else
    {
        gf_assert(0, "vidmm_map_segment_memory only used for kernel map");
    }

    if(vma)
    {
        return vma->virt_addr;
    }

    map.size             = segment_memory->list_node->aligned_size;
    map.flags.mem_space  = flags->mem_space;
    map.flags.read_only  = flags->read_only;
    map.flags.write_only = flags->write_only;

    if(segment_memory->pages_mem)
    {
#ifdef VMI_MODE
        gf_set_pages_addr(segment_memory->pages_mem, adapter->vidmm_bus_addr + segment_memory->gpu_virt_addr);
#endif
        map.flags.mem_type   = GF_SYSTEM_RAM;
        map.memory           = segment_memory->pages_mem;
        map.offset           = 0;

        if(flags->cache_type)
        {
            map.flags.cache_type = flags->cache_type;
        }
        else if(segment->flags.support_snoop || segment->flags.support_manual_flush)
        {
            map.flags.cache_type = GF_MEM_WRITE_BACK;
        }
        else
        {
            map.flags.cache_type = GF_MEM_WRITE_COMBINED;
        }

        vma = gf_map_pages_memory(filp, &map);

        if(segment->flags.support_snoop && (vma->flags.cache_type == GF_MEM_WRITE_BACK))
        {
            if(!segment_memory->snooping_enabled)
            {
                vidmm_allocation_t *allocation = NULL;

                allocation = gf_calloc(sizeof(vidmm_allocation_t));

                allocation->phys_addr = segment_memory->gpu_virt_addr;
                allocation->size      = segment_memory->list_node->aligned_size;
                allocation->pages_mem = segment_memory->pages_mem;

                mm_mgr->chip_func->set_snooping(adapter, allocation, TRUE);

                segment_memory->snooping_enabled = TRUE;
                gf_free(allocation);
            }
        }
    }
    else if(segment->flags.system_phys_mem_reserved)
    {
        if(segment->flags.support_snoop || segment->flags.support_manual_flush)
        {
            map.flags.cache_type = GF_MEM_WRITE_BACK;
        }
        else
        {
            map.flags.cache_type = GF_MEM_WRITE_COMBINED;
        }

        map.flags.mem_type   = GF_SYSTEM_IO;
        map.phys_addr        = segment_memory->gpu_virt_addr - segment->gpu_vm_start + segment->phys_addr_start;

        vma = gf_map_io_memory(filp, &map);
    }
    else
    {
        if(flags->cache_type)
        {
            map.flags.cache_type = flags->cache_type;
        }
        else
        {
            map.flags.cache_type = GF_MEM_WRITE_COMBINED;
        }

        map.flags.mem_type   = GF_SYSTEM_IO;
        map.phys_addr        = adapter->vidmm_bus_addr + segment_memory->gpu_virt_addr - segment->gpu_vm_start + segment->phys_addr_start;

        vma = gf_map_io_memory(filp, &map);
    }

    /* put need flush check in the last, maybe future we also map io as wb */
    if((!segment_memory->snooping_enabled) && (vma->flags.cache_type == GF_MEM_WRITE_BACK))
    {
        vma->need_flush_cache = TRUE;
    }

    segment_memory->vma = vma;

    return vma->virt_addr;
}

void vidmm_unmap_segment_memory(adapter_t *adapter, vidmm_segment_memory_t *segment_memory, enum gf_mem_space mem_space)
{
    gf_vm_area_t *vma = NULL;

    if(mem_space == GF_MEM_KERNEL)
    {
        vma = segment_memory->vma;

        segment_memory->vma = NULL;
    }
    else if(mem_space == GF_MEM_USER)
    {
        gf_assert(0, "only for kernel unmap");
    }

    if(vma == NULL)
    {
        return;
    }

    switch(vma->flags.mem_type)
    {
    case GF_SYSTEM_IO:
        gf_unmap_io_memory(vma);
        break;
    case GF_SYSTEM_RAM:
        gf_unmap_pages_memory(vma);
        break;
    default:
        gf_assert(0, "unmap with vma->flags.mem_type");
    }
}
