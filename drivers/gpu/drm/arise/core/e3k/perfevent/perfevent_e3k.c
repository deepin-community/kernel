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
#include "perfeventi.h"
#include "perfevent.h"
#include "../vidsch/vidsch_engine_e3k.h"


static int perf_direct_get_counter_e3k(adapter_t *adapter, gf_miu_list_item_t *miu_table, unsigned int miu_table_length,int force_read)
{
    gf_assert(0, NULL);
    return 0;
}

static int perf_calculate_engine_usage_e3k(adapter_t * adapter, gf_hwq_info * phwq_info)
{
    unsigned int  engine           = 0;
    hwq_event_mgr_t *hwq_event_mgr = adapter->hwq_event_mgr;
    hwq_event_info  *p_hwq_event   = NULL;
    unsigned int usage             = 0;

    for(engine = 0; engine < adapter->active_engine_count; engine++)
    {

        p_hwq_event   = (hwq_event_info*)(hwq_event_mgr->hwq_event)+engine;
        usage=p_hwq_event->engine_usage;
        //gf_info(" EngineNum=%d usage %d \n", engine, usage);
        if(engine==RB_INDEX_GFXL || engine==RB_INDEX_GFXH)
        {
            if(usage > phwq_info->Usage_3D)
            {
                phwq_info->Usage_3D=usage;
            }
        }

        if(engine==RB_INDEX_VCP0 || engine==RB_INDEX_VCP1)
        {
            if(usage > phwq_info->Usage_VCP)
            {
                phwq_info->Usage_VCP=usage;
            }
        }

        if(engine==RB_INDEX_VPP)
        {
            if(usage > phwq_info->Usage_VPP)
            {
                phwq_info->Usage_VPP=usage;
            }
        }
    }
    return 0;
}


static int perf_calculate_engine_usage_ext_e3k(adapter_t * adapter, gfx_hwq_info_ext *phwq_info_ext)
{
    unsigned int         usage            = 0;
    unsigned int  engine           = 0;
    hwq_event_info       *p_hwq_event;
    hwq_event_mgr_t *hwq_event_mgr = adapter->hwq_event_mgr;

    if( !(adapter->ctl_flags.hwq_event_enable) || !(adapter->hwq_event_mgr) )
    {
        return -1;
    }

    gf_memset(phwq_info_ext,0,sizeof(gfx_hwq_info_ext));
    for(engine = 0; engine < adapter->active_engine_count; engine++)
    {

        p_hwq_event   = (hwq_event_info*)(hwq_event_mgr->hwq_event)+engine;
        usage=p_hwq_event->engine_usage;
        //gf_info(" EngineNum=%d usage %d \n", engine, usage);
        switch(engine)
        {
            case RB_INDEX_GFXL:
                phwq_info_ext->Usage_3D=usage;
                break;
            case RB_INDEX_GFXH:
                phwq_info_ext->Usage_3D_High=usage;
                break;
            case RB_INDEX_VCP0:
                phwq_info_ext->Usage_VCP0=usage;
                break;
            case RB_INDEX_VCP1:
                phwq_info_ext->Usage_VCP1=usage;
                break;
            case RB_INDEX_VPP:
                phwq_info_ext->Usage_VPP=usage;
                break;
            default:
                break;
        }
    }
    return 0;
}


perf_chip_func_t   perf_chip_func =
{
    .direct_get_miu_counter = perf_direct_get_counter_e3k,
    .calculate_engine_usage = perf_calculate_engine_usage_e3k,
    .calculate_engine_usage_ext = perf_calculate_engine_usage_ext_e3k,
};

