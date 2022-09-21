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
#include "vidsch.h"
#include "vidschi.h"

/* must called sch_mgr->engine_lock locked */
void vidschi_engine_dvfs_power_on(vidsch_mgr_t *sch_mgr)
{
    adapter_t *adapter = sch_mgr->adapter;

    if (!adapter->pwm_level.EnableClockGating)
    {
        return;
    }

    if(!sch_mgr->engine_dvfs_power_on)
    {
        if(!list_empty(&sch_mgr->submitted_task_list) || sch_mgr->last_send_fence_id != sch_mgr->returned_fence_id)
        {
            if(sch_mgr->chip_func->power_clock)
            {
                sch_mgr->chip_func->power_clock(sch_mgr, FALSE);

                sch_mgr->engine_dvfs_power_on = TRUE;
            }
        }
     }

     return;
}

void vidschi_try_power_tuning(adapter_t *adapter, unsigned int gfx_only)
{
    vidschedule_t *schedule = adapter->schedule;

    if (!adapter->pwm_level.EnableClockGating)
    {
        return;
    }

    if(schedule->chip_func->power_tuning)
    {
        schedule->chip_func->power_tuning(adapter, gfx_only);
    }
    return;
}


