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
#include "vidmm.h"
#include "vidmmi.h"
#include "mm_e3k.h"
#include "vidmmi_e3k.h"
#include "register_e3k.h"
#include "chip_include_e3k.h"

void vidmm_init_mem_settings_e3k(adapter_t *adapter)
{
    Reg_Mmu_Mode                    reg_Mmu_Mode                  = {0};
    Reg_Mxu_Dec_Ctrl                reg_Mxu_Dec_Ctrl              = {0};
    Reg_Mxu_Channel_Control         reg_Mxu_Channel_Control       = {0};
    Reg_Ttbr                        reg_Ttbr                      = {0};
    DWORD                           dwAdapterMemorySize           = 0;
    unsigned char *                 pRegAddr                      = NULL;
    vidmm_segment_t                 *segment                      = NULL;
    Reg_Argument_Fl2                ArgumentBuffer                = {0};

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
        unsigned int ttbrn_for_total = (adapter->Real_vram_size + adapter->pcie_vram_size)%0x100000000? 
                                                                  ((adapter->Real_vram_size + adapter->pcie_vram_size)/0x100000000 +1) : (adapter->Real_vram_size + adapter->pcie_vram_size)/0x100000000;//per ttbr can index 4G mem, total has 256 ttbr regs for pa mode.

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

    if(adapter->hw_caps.svm_enable)
    {
        reg_Ttbr.reg.Segment = 1;
        reg_Ttbr.reg.Valid   = 1;
        reg_Ttbr.reg.Addr = adapter->gart_table_SVM_L2->phys_addr >> 12;
        pRegAddr = adapter->mmio + MMIO_MMU_START_ADDRESS + (Reg_Mmu_Ttbr_Offset +255)*4;
        gf_write32(pRegAddr, reg_Ttbr.uint);
    }

    if (adapter->hw_caps.miu_channel_num == 1)
    {
        reg_Mxu_Channel_Control.reg.Miu_Channel0_Disable = 0;
        reg_Mxu_Channel_Control.reg.Miu_Channel1_Disable = 1;
        reg_Mxu_Channel_Control.reg.Miu_Channel2_Disable = 1;

        pRegAddr = adapter->mmio + MMIO_MXU_START_ADDRESS + 0x1E*4;
        gf_write32(pRegAddr, 0x1200003);
    }else if (adapter->hw_caps.miu_channel_num == 2)
    {
        reg_Mxu_Channel_Control.reg.Miu_Channel0_Disable = 0;
        reg_Mxu_Channel_Control.reg.Miu_Channel1_Disable = 0;
        reg_Mxu_Channel_Control.reg.Miu_Channel2_Disable = 1;

        pRegAddr = adapter->mmio + MMIO_MXU_START_ADDRESS + 0x1E*4;
        gf_write32(pRegAddr, 0x1300003);
    }else if (adapter->hw_caps.miu_channel_num == 3)
    {
        reg_Mxu_Channel_Control.reg.Miu_Channel0_Disable = 0;
        reg_Mxu_Channel_Control.reg.Miu_Channel1_Disable = 0;
        reg_Mxu_Channel_Control.reg.Miu_Channel2_Disable = 0;

        pRegAddr = adapter->mmio + MMIO_MXU_START_ADDRESS + 0x1E*4;
        gf_write32(pRegAddr, 0x1360003);
    }else
    {
        gf_assert(0,"bad miu channel num\n");
    }

    pRegAddr = adapter->mmio + MMIO_MXU_START_ADDRESS + 0x1B*4;
    gf_write32(pRegAddr, 0x4);

    reg_Mxu_Channel_Control.reg.Ch_Size = adapter->hw_caps.miu_channel_size;

    pRegAddr =adapter->mmio + MMIO_MXU_START_ADDRESS + Reg_Mxu_Channel_Control_Offset*4;
    gf_write32(pRegAddr, reg_Mxu_Channel_Control.uint);
    //gf_info(" reg_Mxu_Channel_Control.uint 0x%x readvalue: 0x%x \n", reg_Mxu_Channel_Control.uint, gf_read32(pRegAddr));
}


void vidmm_query_segment_info_e3k(adapter_t *adapter, vidmm_chip_segment_info_t *info)
{
    vidmm_mgr_t          *mm_mgr        = adapter->mm_mgr;
    vidmm_segment_desc_t *segments_desc = NULL;
    vidmm_segment_desc_t *segment_desc  = NULL;
    unsigned long long boundry_for_unvisible_segment_low=0llu;
    int size_for_secure_range = 0;

    //use default memory size 
    if(adapter->pcie_vram_size == 0)
    {
        adapter->pcie_vram_size = GART_MEMORY_SIZE_E3K;
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
    segment_desc->gpu_vm_size      = adapter->pcie_vram_size - SNOOPABLE_SEGMENT_RATION_E3K(adapter->pcie_vram_size);
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
    segment_desc->gpu_vm_size      = SNOOPABLE_SEGMENT_RATION_E3K(adapter->pcie_vram_size);
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

vidmm_chip_func_t   vidmm_chip_func =
{
    .query_segment_info       = vidmm_query_segment_info_e3k,
    .mem_setting              = vidmm_init_mem_settings_e3k,
    .init_gart_table          = vidmm_init_gart_table_e3k,
    .init_svm_gart_table      = vidmm_init_svm_gart_table_e3k,
    .deinit_svm_gart_table    = vidmm_deinit_svm_gart_table_e3k,    
    .deinit_gart_table        = vidmm_deinit_gart_table_e3k,
    .query_gart_table_info    = vidmm_query_gart_table_info_e3k,
    .describe_allocation      = vidmm_describe_allocation_e3k,
    .build_paging_buffer      = vidmm_build_page_buffer_e3k,
    .map_gart_table           = vidmm_map_gart_table_e3k,
    .map_svm_gart_table       = vidmm_map_svm_gart_table_e3k,
    .unmap_gart_table         = vidmm_unmap_gart_table_e3k,
    .unmap_svm_gart_table     = vidmm_unmap_svm_gart_table_e3k,
    .set_snooping             = vidmm_gart_table_set_snoop_e3k,
    .get_allocation_info      = vidmm_get_allocation_info_e3k,
};



