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
#include "vidsch.h"
#include "perfevent.h"

void vidmm_get_map_allocation_info(adapter_t *adapter, vidmm_allocation_t *allocation, gf_map_argu_t *argu)
{
    vidmm_mgr_t     *mm_mgr  = adapter->mm_mgr;
    vidmm_segment_t *segment = &mm_mgr->segment[allocation->segment_id];

    argu->size = allocation->size;

    if (allocation->pages_mem)
    {
        argu->memory = allocation->pages_mem;
        argu->flags.mem_type = GF_SYSTEM_RAM;
        argu->offset = 0;

        if (segment->flags.support_snoop)
        {
            argu->flags.cache_type = GF_MEM_WRITE_BACK;
        }
        else if (allocation->flag.cacheable)
        {
            argu->flags.cache_type = GF_MEM_WRITE_BACK;
        }
        else if (segment->flags.support_manual_flush && (allocation->segment_id != 0))
        {
            argu->flags.cache_type = GF_MEM_WRITE_BACK;
        }
        else
        {
            argu->flags.cache_type = GF_MEM_WRITE_COMBINED;
        }
    }
    else if (segment->flags.system_phys_mem_reserved)
    {
        if(segment->flags.support_snoop || segment->flags.support_manual_flush)
        {
            argu->flags.cache_type = GF_MEM_WRITE_BACK;
        }
        else
        {
            argu->flags.cache_type = GF_MEM_WRITE_COMBINED;
        }

        argu->flags.mem_type   = GF_SYSTEM_IO;
        argu->phys_addr        = allocation->phys_addr - segment->gpu_vm_start + segment->phys_addr_start;
    }
    else
    {
        if(segment->flags.support_snoop)
        {
            argu->flags.cache_type = GF_MEM_WRITE_BACK;
        }
        else if(segment->flags.support_manual_flush && (allocation->segment_id != 0))
        {
            argu->flags.cache_type = GF_MEM_WRITE_BACK;
        }
        else
        {
            argu->flags.cache_type = GF_MEM_WRITE_COMBINED;
        }

        argu->flags.mem_type   = GF_SYSTEM_IO;
        argu->phys_addr        = allocation->phys_addr + adapter->vidmm_bus_addr;

        if(argu->phys_addr >= (adapter->vidmm_bus_addr + adapter->Visible_vram_size))
        {
            gf_assert(0, "vidmm_get_map_allocation_info map invisable allocation!!!"); 
        }
    }
}
