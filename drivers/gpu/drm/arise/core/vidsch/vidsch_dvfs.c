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

int vidsch_query_dvfs_clamp(adapter_t *adapter, gf_dvfs_clamp_status_t* dvfs_clamp)
{
    vidschedule_t *schedule = adapter->schedule;
    int status = -1;
	
    if(schedule->chip_func->query_dvfs_clamp)
    {
        status = schedule->chip_func->query_dvfs_clamp(adapter, dvfs_clamp);
    }

    return status;
}


int vidsch_dvfs_set(adapter_t *adapter, gf_dvfs_set_t *dvfs_set)
{
    vidschedule_t *schedule = adapter->schedule;
    int status = -1;

    adapter->pm_caps.dvfs_force = dvfs_set->dvfs_force;

    if(adapter->pm_caps.dvfs_force && schedule->chip_func->set_dvfs)
    {
        status = schedule->chip_func->set_dvfs(adapter, dvfs_set);
    }

    return status;
}

void vidsch_try_dvfs_tuning(adapter_t *adapter)
{
    vidschedule_t *schedule = adapter->schedule;

    if(!adapter->pm_caps.dvfs_force && schedule->chip_func->dvfs_tuning)
    {
       schedule->chip_func->dvfs_tuning(adapter);
    }
}

