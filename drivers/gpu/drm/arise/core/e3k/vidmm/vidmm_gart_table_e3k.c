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
#include "vidmmi.h"
#include "register_e3k.h"
#include "mm_e3k.h"
#include "chip_include_e3k.h"
#include "vidmmi_e3k.h"

static unsigned long long vidmmi_get_allocation_page_offset_e3k(adapter_t *adapter, vidmm_gart_table_info_t *gart_table, vidmm_allocation_t *allocation)
{
    unsigned long long page_offset = 0;

    gf_assert(allocation->phys_addr >= gart_table->gart_start_addr, "vidmmi_get_allocation_page_offset");
    page_offset = (allocation->phys_addr - gart_table->gart_start_addr)/GPU_PAGE_SIZE_E3K;

    return page_offset;
}

int vidmm_query_gart_table_info_e3k(adapter_t *adapter)
{
    int result = S_OK;
    vidmm_gart_table_info_t *gart_table_L3 = adapter->gart_table_L3;
    vidmm_gart_table_info_t *gart_table_L2 = adapter->gart_table_L2;

    gart_table_L3->size =(adapter->gart_ram_size / GPU_PAGE_SIZE_E3K) * sizeof(int);
    gart_table_L2->size =((adapter->gart_ram_size + adapter->Real_vram_size)/(4*1024*1024)) * sizeof(int);

#ifndef GF_HW_NULL
    gart_table_L3->segment_id =
    gart_table_L2->segment_id = SEGMENT_ID_LOCAL_E3K;
#else
    gart_table_L3->segment_id =
    gart_table_L2->segment_id = SEGMENT_ID_GART_UNSNOOPABLE_E3K;
#endif

    return result;
}

void vidmm_init_gart_table_e3k(adapter_t *adapter)
{
    vidmm_gart_table_info_t *gart_table_L3 = adapter->gart_table_L3;
    vidmm_gart_table_info_t *gart_table_L2 = adapter->gart_table_L2;
    unsigned int L2EntryNumForL3 = gart_table_L3->size/(4*1024);
    unsigned int L2EntryNumForLoal = adapter->Real_vram_size/(4*1024*1024);
    unsigned int L3EntryNum = gart_table_L3->size/4;
    unsigned int            *pte_list   = NULL;
    PTE_L2_t page_entry  = {0};
    PTE_L3_t page_entry_L3 = {0};
    int                     i;
    unsigned long long page_phy_addr = adapter->dummy_page_addr;

    page_phy_addr >>= 12;
    page_entry_L3.pte_4k.Valid =1;
    page_entry_L3.pte_4k.Addr = page_phy_addr & 0xFFFFFFF;


    if(gart_table_L3->virt_addr)
    {
        pte_list = gart_table_L3->virt_addr;

        gart_table_L3->pcie_addr  = gart_table_L3->virt_addr;

        for(i = 0; i < L3EntryNum; i++)
        {
            pte_list[i] =  page_entry_L3.uint;
        }

        gart_table_L3->gart_start_addr = adapter->Real_vram_size;
        gart_table_L3->dirty = TRUE;
    }

    if(gart_table_L2->virt_addr)
    {
        page_entry._4k_d.Valid = TRUE;
        page_entry._4k_d.Segment= 1; // 1 means this pte point to local mem
        page_entry._4k_d.Addr= 0;

        pte_list = gart_table_L2->virt_addr;

        gart_table_L2->pcie_addr  = gart_table_L2->virt_addr;

        for(i = 0; i < L2EntryNumForLoal; i++)
        {
            pte_list[i] = page_entry.uint;
        }

        for(i = 0; i < L2EntryNumForL3; i++)
        {
            page_entry._4k_d.Addr = ((gart_table_L3->phys_addr >> 12) + i) &0xFFFFFF;
            pte_list[i + L2EntryNumForLoal] = page_entry.uint;
        }
        gart_table_L2->dirty = TRUE;
    }
}

void vidmm_deinit_gart_table_e3k(adapter_t *adapter)
{

}

struct fill_garttable_arg
{
    adapter_t    *adapter;
    unsigned long long page_offset;
    unsigned long long base_index;
    unsigned int *gart_pages;
    PTE_L3_t     page_entry;
    unsigned long long mask;
    unsigned long long addr;
    unsigned long long boundary;

    //duplicate hang only:
    //To restore gart-table, only need to update pte.Addr fields
    int          update_addr_only;
};

static int vidmm_fill_garttable_e3k(void *arg, int pg_start, int pg_cnt, unsigned long long dma_addr)
{
    int i, j;
    struct fill_garttable_arg *fga = arg;
    unsigned long long page_phy_addr = dma_addr >> 12;
    unsigned int GPUPAGES_PER_CPUPAGE = fga->adapter->os_page_size/ GPU_PAGE_SIZE_E3K;

    for (i = 0; i < pg_cnt; i++)
    {
        for (j = 0; j < GPUPAGES_PER_CPUPAGE; j++)
        {

            if(fga->update_addr_only)
            {
               fga->page_entry.uint = fga->gart_pages[fga->base_index + (i + pg_start) * GPUPAGES_PER_CPUPAGE + j];
            }

            fga->page_entry.pte_4k.Addr = (page_phy_addr + i * GPUPAGES_PER_CPUPAGE + j) & 0xFFFFFFF;
            fga->gart_pages[fga->base_index + (i + pg_start) * GPUPAGES_PER_CPUPAGE + j] = fga->page_entry.uint;
            fga->mask |= fga->addr ^ (((fga->page_offset + (i + pg_start) * GPUPAGES_PER_CPUPAGE + j) << 12) + fga->boundary);
        }
    }

    return 0;
}


void vidmm_map_gart_table_e3k(adapter_t *adapter, vidmm_allocation_t *allocation, int snooping)
{
    vidmm_gart_table_info_t *gart_table_L3 = adapter->gart_table_L3;
    struct os_pages_memory  *pages_mem  = allocation->pages_mem;
    unsigned int            *gart_pages = gart_table_L3->pcie_addr;
    struct fill_garttable_arg arg = {0, };

    arg.adapter    = adapter;
    arg.gart_pages = gart_pages;
    arg.boundary = gart_table_L3->gart_start_addr;
    arg.page_entry.pte_4k.Valid = 1;
    arg.page_entry.pte_4k.Segment = 0;
    arg.page_entry.pte_4k.NS      = 1;
    arg.page_entry.pte_4k.Snoop   = snooping? TRUE: FALSE;
    arg.page_offset = vidmmi_get_allocation_page_offset_e3k(adapter, gart_table_L3, allocation);
    arg.base_index = arg.page_offset;
    arg.addr = (arg.page_offset << 12) + arg.boundary;
    arg.mask = 0;

#ifdef VMI_MODE
    gf_set_pages_addr(pages_mem, arg.page_offset * GPU_PAGE_SIZE_E3K);
#endif

    gf_pages_memory_for_each_continues(pages_mem, &arg, vidmm_fill_garttable_e3k);

    gf_mutex_lock(adapter->gart_table_lock);

    // Mask will be the or of old mask and new mask and
    // the diff between static part of old address and new address
    if (gart_table_L3->dirty)
    {
        gart_table_L3->gart_table_dirty_mask =
            (gart_table_L3->gart_table_dirty_mask | arg.mask |
            (gart_table_L3->gart_table_dirty_addr ^ (arg.addr & ~arg.mask)));
        gart_table_L3->gart_table_dirty_addr =
            gart_table_L3->gart_table_dirty_addr & ~gart_table_L3->gart_table_dirty_mask;
    }
    else
    {
        gart_table_L3->gart_table_dirty_mask = arg.mask;
        gart_table_L3->gart_table_dirty_addr = arg.addr & ~arg.mask;
    }

    /* flush write_combine buffer */
    gf_flush_wc();

    gart_table_L3->dirty = TRUE;

    gf_mutex_unlock(adapter->gart_table_lock);
}

/*
 * when unmap gart table, fill the corresponding gart table entry with dumy page addr
 */
void vidmm_unmap_gart_table_e3k(adapter_t *adapter, vidmm_allocation_t *allocation)
{
    vidmm_gart_table_info_t *gart_table_L3 = adapter->gart_table_L3;
    unsigned int            *gart_pages    = gart_table_L3->pcie_addr;
    unsigned long long      boundary       = gart_table_L3->gart_start_addr;
    unsigned int            page_count     = allocation->size / adapter->os_page_size;
    unsigned long long      page_offset    = 0;
    unsigned long long      page_phy_addr;
    unsigned long long      addr = 0;
    unsigned long long      mask = 0;

    int i = 0, j = 0;
    int gart_page_num_per_kernel_page = adapter->os_page_size / GPU_PAGE_SIZE_E3K;

    PTE_L3_t page_entry = {0};

    page_entry.pte_4k.Valid = 1;
    page_entry.pte_4k.Segment = 0;
    page_entry.pte_4k.NS      = 1;

    page_offset            = vidmmi_get_allocation_page_offset_e3k(adapter, gart_table_L3, allocation);
    addr                   = (page_offset << 12) + boundary;

    for(i = 0; i < page_count; i++)
    {
        page_phy_addr = adapter->dummy_page_addr;
        page_phy_addr >>= 12;

#if defined(__mips64__) || defined(__loongarch__)
        gf_assert(!(page_phy_addr & 0xFFFFFFFFF0000000), "cpu PA need under 36bits");
        gf_assert(!(page_phy_addr &0x3), "page_phy_addr not 16k");
#endif

        for(j = 0; j < gart_page_num_per_kernel_page; j++)
        {
            //set sys page PA
            page_entry.pte_4k.Addr = (page_phy_addr + j) & 0xFFFFFFF; //how could we ensure cpu PA is under 36bits?

            gart_pages[page_offset + i * gart_page_num_per_kernel_page + j] = page_entry.uint;

            mask |= addr ^ (((i * gart_page_num_per_kernel_page + j + page_offset) << 12) + boundary);
        }
    }

    gf_mutex_lock(adapter->gart_table_lock);
    // Mask will be the or of old mask and new mask and
    // the diff between static part of old address and new address
    if (gart_table_L3->dirty)
    {
        gart_table_L3->gart_table_dirty_mask =
            (gart_table_L3->gart_table_dirty_mask | mask |
            (gart_table_L3->gart_table_dirty_addr ^ (addr & ~mask)));
        gart_table_L3->gart_table_dirty_addr =
            gart_table_L3->gart_table_dirty_addr & ~gart_table_L3->gart_table_dirty_mask;
    }
    else
    {
        gart_table_L3->gart_table_dirty_mask = mask;
        gart_table_L3->gart_table_dirty_addr = addr & ~mask;
    }

    /* flush write_combine buffer */
    gf_flush_wc();
    gart_table_L3->dirty = TRUE;
    gf_mutex_unlock(adapter->gart_table_lock);
}

void vidmm_gart_table_set_snoop_e3k(adapter_t *adapter, vidmm_allocation_t *allocation, int snooping)
{

}

void vidmm_update_gart_table_pte_e3k(adapter_t *adapter, struct os_pages_memory *memory, unsigned int *gart_table_L3, unsigned long gpu_va)
{
    struct fill_garttable_arg arg = {0, };

    arg.adapter    = adapter;
    arg.gart_pages = gart_table_L3;
    arg.boundary   = adapter->Real_vram_size;
    arg.page_offset = (gpu_va - adapter->Real_vram_size)/GPU_PAGE_SIZE_E3K;
    arg.base_index = arg.page_offset;
    arg.addr = (arg.page_offset << 12) + arg.boundary;
    arg.mask = 0;
    arg.update_addr_only = 1;

#ifdef VMI_MODE
    gf_set_pages_addr(memory, arg.page_offset * GPU_PAGE_SIZE_E3K);
#endif

    gf_pages_memory_for_each_continues(memory,
                                       &arg,
                                       vidmm_fill_garttable_e3k);
}

