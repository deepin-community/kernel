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
#include "vidmm.h"
#include "vidsch.h"
#include "vidmmi.h"
#include "mm_e3k.h"
#include "vidmmi_e3k.h"
#include "register_e3k.h"
#include "chip_include_e3k.h"

extern int vidmmi_copy_data(struct _vidmm_mgr *mm_mgr, task_paging_t *paging_task,vidmm_allocation_t *dst_allocation,
                            unsigned long long src_addr, unsigned long long dst_addr,
                            unsigned int src_compress, unsigned int dst_compress, unsigned int size);

int vidmm_segment_memory_transfer_e3k(adapter_t *adapter, vidmm_segment_memory_t **dst_pointer, vidmm_segment_memory_t *src, int to_local);

void vidmm_init_mem_settings_e3k(adapter_t *adapter)
{
    Reg_Mmu_Mode                    reg_Mmu_Mode                  = {0};
    Reg_Mxu_Dec_Ctrl                reg_Mxu_Dec_Ctrl              = {0};
    Reg_Mxu_Channel_Control         reg_Mxu_Channel_Control       = {0};
    Reg_Ttbr                        reg_Ttbr                      = {0};
    unsigned char *                 pRegAddr                      = NULL;
    unsigned int pending_buf_len = (adapter->chip_id < CHIP_ARISE1020) ? 0x4 : 0x40;

    //vcp0 decouple disable 0x4c910 0x1
    //if miu numble is larger than 1, we enable decouple, others disable.
    if(adapter->chip_id >= CHIP_ARISE1020 && adapter->chip_id != CHIP_ARISE2030)
    {
        pRegAddr = adapter->mmio + 0x4c910;
        gf_write32(pRegAddr, 0x1);
    }

    //CHIP_ARISE1020 only has one vcp core
    //if(adapter->chip_id < CHIP_ARISE1020)
    //{
        //vcp1 decouple disable 0x4a910 0x1
    //    pRegAddr = adapter->mmio + 0x4a910;
    //    gf_write32(pRegAddr, 0x1);
    //}

    reg_Mmu_Mode.reg.Vmen = 0;//use PA mode
    reg_Mmu_Mode.reg.Video_Size = (adapter->Real_vram_size>> 28) - 1;

    pRegAddr = adapter->mmio + MMIO_MXU_START_ADDRESS + Reg_Mmu_Mode_Offset*4;
    gf_write32(pRegAddr, reg_Mmu_Mode.uint);
    //gf_info(" reg_Mmu_Mode.uint 0x%x readvalue: 0x%x \n", reg_Mmu_Mode.uint, gf_read32(pRegAddr));

    reg_Mxu_Dec_Ctrl.reg.Ddec_Antilock  = 1;
    pRegAddr = adapter->mmio + MMIO_MXU_START_ADDRESS + Reg_Mxu_Dec_Ctrl_Offset*4;
    gf_write32(pRegAddr, reg_Mxu_Dec_Ctrl.uint);
    gf_info("reg_Mxu_Dec_Ctrl.uint 0x%x readvalue: 0x%x \n", reg_Mxu_Dec_Ctrl.uint, gf_read32(pRegAddr));

#ifndef GF_HW_NULL
  //  if(!adapter->hw_caps.local_only)
    {
        unsigned int i = 0;
        unsigned int ttbrn_for_total = (adapter->Real_vram_size + adapter->gart_ram_size)%0x100000000?
                                                                  ((adapter->Real_vram_size + adapter->gart_ram_size)/0x100000000 +1) : (adapter->Real_vram_size + adapter->gart_ram_size)/0x100000000;//per ttbr can index 4G mem, total has 256 ttbr regs for pa mode.

        gf_assert(ttbrn_for_total <= 256,"total ttbr number excced");

        for(i = 0; i < ttbrn_for_total; i++)
        {
            reg_Ttbr.reg.Segment = 1;
            reg_Ttbr.reg.Valid   = 1;
            reg_Ttbr.reg.Addr = (adapter->gart_table_L2->phys_addr + 4096 * i) >> 12;
            pRegAddr = adapter->mmio + MMIO_MMU_START_ADDRESS + (Reg_Mmu_Ttbr_Offset + i)*4;
            gf_write32(pRegAddr, reg_Ttbr.uint);
        }
    }
#endif

    if (adapter->hw_caps.miu_channel_num == 1)
    {
        reg_Mxu_Channel_Control.reg.Miu_Channel0_Disable = 0;
        reg_Mxu_Channel_Control.reg.Miu_Channel1_Disable = 1;
        reg_Mxu_Channel_Control.reg.Miu_Channel2_Disable = 1;

        pRegAddr = adapter->mmio + MMIO_MXU_START_ADDRESS + Reg_Diu_Reserve_Ctrl_Offset*4;
        gf_write32(pRegAddr, 0x1200003);
    }else if (adapter->hw_caps.miu_channel_num == 2)
    {
        reg_Mxu_Channel_Control.reg.Miu_Channel0_Disable = 0;
        reg_Mxu_Channel_Control.reg.Miu_Channel1_Disable = 0;
        reg_Mxu_Channel_Control.reg.Miu_Channel2_Disable = 1;

        pRegAddr = adapter->mmio + MMIO_MXU_START_ADDRESS + Reg_Diu_Reserve_Ctrl_Offset*4;
        gf_write32(pRegAddr, 0x1300003);
    }else if (adapter->hw_caps.miu_channel_num == 3)
    {
        reg_Mxu_Channel_Control.reg.Miu_Channel0_Disable = 0;
        reg_Mxu_Channel_Control.reg.Miu_Channel1_Disable = 0;
        reg_Mxu_Channel_Control.reg.Miu_Channel2_Disable = 0;

        pRegAddr = adapter->mmio + MMIO_MXU_START_ADDRESS + Reg_Diu_Reserve_Ctrl_Offset*4;
        gf_write32(pRegAddr, 0x1360003);
    }else
    {
        gf_assert(0,"bad miu channel num\n");
    }

    pRegAddr = adapter->mmio + MMIO_MXU_START_ADDRESS + Reg_Pending_Buf_Ctrl_0_Offset*4;
    gf_write32(pRegAddr, pending_buf_len);

    reg_Mxu_Channel_Control.reg.Ch_Size = adapter->hw_caps.miu_channel_size;

    pRegAddr =adapter->mmio + MMIO_MXU_START_ADDRESS + Reg_Mxu_Channel_Control_Offset*4;
    gf_write32(pRegAddr, reg_Mxu_Channel_Control.uint);
    //gf_info(" reg_Mxu_Channel_Control.uint 0x%x readvalue: 0x%x \n", reg_Mxu_Channel_Control.uint, gf_read32(pRegAddr));

    //mxu vcp priority higher 0x4908c 0x11000202
    pRegAddr = adapter->mmio + 0x4908c;
    gf_write32(pRegAddr, 0x11000202);

    if (adapter->hw_caps.gf_backdoor_enable)
    {
#if defined(__i386__) || defined(__x86_64__)
        pRegAddr = adapter->mmio + 0x8a70;
        gf_write32(pRegAddr, 0x001a4000);
        pRegAddr = adapter->mmio + 0x8a6c;
        gf_write32(pRegAddr, 0x00000000);
#elif defined (__aarch64__)
        pRegAddr = adapter->mmio + 0x8a70;
        gf_write32(pRegAddr, 0x001a4000);
        pRegAddr = adapter->mmio + 0x8a6c;
        gf_write32(pRegAddr, 0x10000000);
#elif defined (__mips64__) || defined (__loongarch__)
        pRegAddr = adapter->mmio + 0x8a70;
        gf_write32(pRegAddr, 0x001a4000);
        pRegAddr = adapter->mmio + 0x8a6c;
        gf_write32(pRegAddr, 0x20000000);
#endif
    }
    else
    {
#if defined (__mips64__) || defined (__loongarch__)
        // default upstream read length (2048 bit) will cause hang when playing video,
        // set to 512 bit on 1020 when backdoor is off
        if(adapter->chip_id >= CHIP_ARISE1020)
        {
            pRegAddr = adapter->mmio + 0x8a6c;
            gf_write32(pRegAddr, 0x20000000);
        }
#endif
    }
}


void vidmm_query_segment_info_e3k(adapter_t *adapter, vidmm_chip_segment_info_t *info)
{
    vidmm_segment_desc_t *segments_desc = NULL;
    vidmm_segment_desc_t *segment_desc  = NULL;
    unsigned long long boundry_for_unvisible_segment_low=0llu;
    int size_for_secure_range = 0;

    //use default memory size
    if(adapter->gart_ram_size == 0)
    {
        adapter->gart_ram_size = GART_MEMORY_SIZE_E3K;
    }

    size_for_secure_range =
    adapter->Visible_vram_size - ((adapter->Visible_vram_size - 2 * SECURE_RANGE_BUFFER_SIZE)&~0xFFFFll);   //secure range should be 64k aligned

    info->cpu_visible_vidmm_size    = adapter->Visible_vram_size;
    info->cpu_unvisible_vidmm_size  = adapter->Real_vram_size - adapter->Visible_vram_size;
    info->segment_cnt               = SEGMENT_ID_MAX_E3K;
    segments_desc                   = info->segment_desc;

    /*      GPU Memory Layout
            -------------------------
            |  L                    |
            |  O   cpu visiable     |
            |  C ----------------------> Visible_vram_size
            |  A   cpu unvisiable   |
            |  L                    |
            +-----------------------+--> Real_vram_size
            |  P                    |
            |  C    none-snoop      |
            |    -------------------|
            |  I     snoop          |
            |  E                    |
            +-----------------------+
    */

    /* init local cpu visiable segment*/
    segment_desc                   = &segments_desc[SEGMENT_ID_LOCAL_E3K];
    segment_desc->segment_id       = SEGMENT_ID_LOCAL_E3K;
    segment_desc->segment_name     = "Segment Local";
    segment_desc->gpu_vm_start     = 0;
    segment_desc->gpu_vm_size      =
    adapter->hw_caps.secure_range_enable ? (adapter->Visible_vram_size -size_for_secure_range):adapter->Visible_vram_size;

    segment_desc->reserved_vm_size = 0;
    segment_desc->small_heap_size  = SMALL_HEAP_SIZE_E3K;
    segment_desc->small_heap_max_allocate_size = SMALL_HEAP_MAX_ALLOCATE_SIZE_E3K;
    segment_desc->segment_alignment      = adapter->os_page_size;//drm_gem_private_object_init has PAGE_SIZE check.
    segment_desc->flags.cpu_visible      = 1;
    segment_desc->flags.support_aperture = 0;
    segment_desc->flags.mtrr             = 1;
    segment_desc->flags.small_heap_available = 1;
    segment_desc->flags.system_pages_reserved = 0;
    segment_desc->flags.chip_phys_mem_reserved = 1;


    /*hack secure range from local cpu visiable end*/
    segment_desc                   = &segments_desc[SEGMENT_ID_SECURE_RANGE_E3K];
    segment_desc->segment_id       = SEGMENT_ID_SECURE_RANGE_E3K;
    segment_desc->segment_name     = "Segment Secure range";
    segment_desc->gpu_vm_start     = adapter->hw_caps.secure_range_enable ? (adapter->Visible_vram_size - size_for_secure_range):0;
    segment_desc->gpu_vm_size      = adapter->hw_caps.secure_range_enable ? SECURE_RANGE_BUFFER_SIZE:0;
    segment_desc->segment_alignment      = adapter->os_page_size;//drm_gem_private_object_init has PAGE_SIZE check.
    segment_desc->flags.cpu_visible      = 1;
    segment_desc->flags.support_aperture = 0;
    segment_desc->flags.mtrr             = 1;
    segment_desc->flags.secure_range = 1;
    segment_desc->flags.chip_phys_mem_reserved = 1;

    segment_desc                   = &segments_desc[SEGMENT_ID_SECURE_RANGE_E3K_1];
    segment_desc->segment_id       = SEGMENT_ID_SECURE_RANGE_E3K_1;
    segment_desc->segment_name     = "Segment Secure range 1";
    segment_desc->gpu_vm_start     = adapter->hw_caps.secure_range_enable ? (adapter->Visible_vram_size - size_for_secure_range + SECURE_RANGE_BUFFER_SIZE):0;
    segment_desc->gpu_vm_size      = adapter->hw_caps.secure_range_enable ? SECURE_RANGE_BUFFER_SIZE:0;
    segment_desc->segment_alignment      = adapter->os_page_size;//drm_gem_private_object_init has PAGE_SIZE check.
    segment_desc->flags.cpu_visible      = 1;
    segment_desc->flags.support_aperture = 0;
    segment_desc->flags.mtrr             = 1;
    segment_desc->flags.secure_range = 1;
    segment_desc->flags.chip_phys_mem_reserved = 1;

    /* init pcie unsnoopable segment */
    segment_desc                   = &segments_desc[SEGMENT_ID_GART_UNSNOOPABLE_E3K];
    segment_desc->segment_id       = SEGMENT_ID_GART_UNSNOOPABLE_E3K;
    segment_desc->segment_name     = "Segment Unsnoopable";
    segment_desc->gpu_vm_start     = adapter->Real_vram_size;
    segment_desc->gpu_vm_size      = adapter->gart_ram_size - SNOOPABLE_SEGMENT_RATION_E3K(adapter->gart_ram_size);
    segment_desc->reserved_vm_size = 0;
    segment_desc->small_heap_size  = SMALL_HEAP_SIZE_E3K;
    segment_desc->small_heap_max_allocate_size = SMALL_HEAP_MAX_ALLOCATE_SIZE_GART_E3K;

    segment_desc->segment_alignment         = adapter->os_page_size;
    segment_desc->flags.require_system_pages = 1;
    segment_desc->flags.cpu_visible          = 1;
    if(adapter->hw_caps.snoop_only)
    {
        segment_desc->flags.support_snoop        = 1;
    }
    else
    {
        segment_desc->flags.support_snoop        = 0;
    }

    segment_desc->flags.small_heap_available = 1;

    /* init pcie snoopable segment */
    segment_desc                   = &segments_desc[SEGMENT_ID_GART_SNOOPABLE_E3K];
    segment_desc->segment_id       = SEGMENT_ID_GART_SNOOPABLE_E3K;
    segment_desc->segment_name     = "Segment Snoopable";
    segment_desc->gpu_vm_start     = adapter->Real_vram_size + segments_desc[SEGMENT_ID_GART_UNSNOOPABLE_E3K].gpu_vm_size;
    segment_desc->gpu_vm_size      = SNOOPABLE_SEGMENT_RATION_E3K(adapter->gart_ram_size);
    segment_desc->reserved_vm_size = 0;
    segment_desc->small_heap_size  = SMALL_HEAP_SIZE_E3K;
    segment_desc->small_heap_max_allocate_size = SMALL_HEAP_MAX_ALLOCATE_SIZE_GART_E3K;

    segment_desc->segment_alignment          = adapter->os_page_size;
    segment_desc->flags.require_system_pages = 1;
    segment_desc->flags.cpu_visible          = 1;
    segment_desc->flags.support_snoop        = 1;
    segment_desc->flags.small_heap_available = 1;

    /*init local cpu unvisiable segment*/
    boundry_for_unvisible_segment_low = (adapter->Real_vram_size > 0x100000000) ? 0x100000000 : adapter->Real_vram_size;//patch for video limit to 4G issue.
    gf_assert(boundry_for_unvisible_segment_low >= adapter->Visible_vram_size, "boundry_for_unvisible_segment_low > adapter->Visible_vram_size");

    segment_desc                   = &segments_desc[SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K];
    segment_desc->segment_id       = SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K;
    segment_desc->segment_name     = "Segment Local CPU unvisiable";
    segment_desc->gpu_vm_start     = adapter->Visible_vram_size;
    segment_desc->gpu_vm_size      = boundry_for_unvisible_segment_low - adapter->Visible_vram_size;

    segment_desc->segment_alignment      = adapter->os_page_size;//drm_gem_private_object_init has PAGE_SIZE check.
    segment_desc->flags.cpu_visible      = 0;
    segment_desc->flags.support_aperture = 0;
    segment_desc->flags.mtrr             = 0;
    segment_desc->flags.small_heap_available = 0;
    segment_desc->flags.system_pages_reserved = 0;
    segment_desc->flags.chip_phys_mem_reserved = 1;

    /*init local cpu unvisiable segment 1*/
    segment_desc                   = &segments_desc[SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K_1];
    segment_desc->segment_id       = SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K_1;
    segment_desc->segment_name     = "Segment Local CPU unvisiable 1";
    segment_desc->gpu_vm_start     = boundry_for_unvisible_segment_low;
    segment_desc->gpu_vm_size      = adapter->Real_vram_size - boundry_for_unvisible_segment_low;

    segment_desc->segment_alignment      = adapter->os_page_size;//drm_gem_private_object_init has PAGE_SIZE check.
    segment_desc->flags.cpu_visible      = 0;
    segment_desc->flags.support_aperture = 0;
    segment_desc->flags.mtrr             = 0;
    segment_desc->flags.small_heap_available = 0;
    segment_desc->flags.system_pages_reserved = 0;
    segment_desc->flags.chip_phys_mem_reserved = 1;

    /* init paging segment */
    if(!adapter->hw_caps.snoop_only)
        info->paging_segment_id   = SEGMENT_ID_GART_UNSNOOPABLE_E3K;
    else
        info->paging_segment_id   = SEGMENT_ID_GART_SNOOPABLE_E3K;
    info->paging_segment_size = PAGING_SEGMENT_SIZE_E3K;
}

static int vidmm_query_segment_mem_e3k(struct _vidmm_mgr *mm_mgr, gf_query_info_t *info)
{
    adapter_t* adapter=mm_mgr->adapter;
    unsigned int id = (info->argu & 0xFF);
    unsigned int is_kernel = (info->argu & 0x80000000);
    vidmm_segment_t* segment=NULL;
    unsigned long long total_size=0,used_size=0;
    unsigned int alloc_num=0;
    gf_segment_mem_t seg_mem={};

    int cnt = vidmm_get_segment_count(adapter);
    if (id > cnt - 1)
    {
        gf_info("invalid  segment id %x\n", id);
        return -1;
    }
    segment = &mm_mgr->segment[id];
    gf_mutex_lock(segment->lock);
    total_size= segment->gpu_vm_size >> 10;
    used_size= (segment->pagable_size + segment->unpagable_size + segment->reserved_size) >> 10;
    alloc_num=(segment->pagable_num + segment->unpagable_num);
    gf_mutex_unlock(segment->lock);

    switch (info->type)
    {
    case GF_QUERY_SEGMENT_FREE_SIZE:
        info->value=total_size-used_size;
        break;
    case GF_QUERY_SEGMENT_MEM_INFO:
        if(info->buf==NULL)
        {
            gf_info("query buffer is null \n");
            return -1;
        }

        seg_mem.segment_id = id;
        seg_mem.segment_total_size = total_size;
        seg_mem.segment_used_size = used_size;
        seg_mem.segment_alloc_num = alloc_num;

        if(is_kernel)
        {
            gf_memcpy(info->buf, &seg_mem, sizeof(gf_segment_mem_t));
        }
        else
        {
            gf_copy_to_user(info->buf, &seg_mem, sizeof(gf_segment_mem_t));
        }
        break;
    default:
        break;
    }
    return 0;
}

int vidmm_segment_memory_transfer_e3k(adapter_t *adapter, vidmm_segment_memory_t **dst_pointer, vidmm_segment_memory_t *src, int to_local)
{
    int alloc_new_segment = 0, result = S_OK;
    unsigned int need_copy_size = 0;
    task_paging_t *paging_task = NULL;
    vidmm_segment_memory_t *dst = NULL;

    if (!src || !dst_pointer)
        goto error;

    if (!(*dst_pointer))
    {
        // If dst_pointer not point a vaild segment_memory, we will alloc a new segment memory to *dst_pointer
        *dst_pointer = vidmm_allocate_segment_memory(adapter, to_local ? SEGMENT_ID_LOCAL_E3K : SEGMENT_ID_GART_UNSNOOPABLE_E3K, src->list_node->aligned_size, 0);
        if (!(*dst_pointer))
        {
            result = E_OUTOFMEMORY;
            goto error;
        }
        alloc_new_segment = 1;
    }

    dst = *dst_pointer;
    //We check segment memory size here.
    need_copy_size = src->list_node->aligned_size > dst->list_node->aligned_size ?
                                dst->list_node->aligned_size : src->list_node->aligned_size;
    //gf_info("segment_memory_transfers src size:0x%llx, dst size:0x%llx\n", src->list_node->aligned_size, dst->list_node->aligned_size);

    paging_task = vidsch_allocate_paging_task(adapter, 8*1024, 0);
    paging_task->paging_allocation_num = 0;
    paging_task->paging_type = 2;
    result = vidmmi_copy_data(adapter->mm_mgr,
                                paging_task,
                                NULL,
                                src->gpu_virt_addr,
                                dst->gpu_virt_addr,
                                CP_OFF,
                                CP_OFF,
                                need_copy_size);
    if(result)
        goto error;

    vidsch_task_inc_reference(adapter, adapter->paging_engine_index, &paging_task->desc);
    vidsch_submit_paging_task(adapter, paging_task);
    vidsch_wait_fence_back(adapter, adapter->paging_engine_index, paging_task->desc.fence_id);
    vidsch_task_dec_reference(adapter, adapter->paging_engine_index, &paging_task->desc);

    return result;

error:
    if (alloc_new_segment)
    {
        vidmm_release_segment_memory(adapter, dst);
        *dst_pointer = NULL;
    }
    return result;

}

vidmm_chip_func_t   vidmm_chip_func =
{
    .query_segment_info       = vidmm_query_segment_info_e3k,
    .mem_setting              = vidmm_init_mem_settings_e3k,
    .init_gart_table          = vidmm_init_gart_table_e3k,
    .deinit_gart_table        = vidmm_deinit_gart_table_e3k,
    .query_gart_table_info    = vidmm_query_gart_table_info_e3k,
    .describe_allocation      = vidmm_describe_allocation_e3k,
    .build_paging_buffer      = vidmm_build_page_buffer_e3k,
    .map_gart_table           = vidmm_map_gart_table_e3k,
    .unmap_gart_table         = vidmm_unmap_gart_table_e3k,
    .set_snooping             = vidmm_gart_table_set_snoop_e3k,
    .get_allocation_info      = vidmm_get_allocation_info_e3k,
    .query_info               = vidmm_query_segment_mem_e3k,
    .segment_memory_transfer  = vidmm_segment_memory_transfer_e3k,
};



