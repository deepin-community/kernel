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
#include "vidmm.h"
#include "context.h"
#include "global.h"

int pm_save_state(adapter_t *adapter, int need_save_memory)
{
    int ret = S_OK;
    ret = cm_save(adapter, need_save_memory);

    if (ret != S_OK)
    {
        return ret;
    }

    ret = vidsch_save(adapter);

    if (ret != S_OK)
    {
        return ret;
    }

    ret = vidmm_save(adapter);

    return ret;
    /* system will suspend, set our card mode as uninitialize */
}


//temp use only, will remove after resume stable.
static inline void util_print_time(char *info)
{
    long time_sec = 0, time_usec = 0;
    
    gf_getsecs(&time_sec, &time_usec);
    gf_info("%s %ld(ms)\n",info,(time_sec * 1000 + gf_do_div(time_usec, 1000)));
}

int pm_restore_state(adapter_t *adapter)
{  
    int ret = S_OK;

    //temp use only, will remove all this function called after resume stable.
    util_print_time("pm_restore_state enter, cur time");
    
    glb_init_chip_interface(adapter);
    util_print_time("glb_init_chip_interface finish, cur time");

    if(ret != S_OK)
    {
        gf_error("dispmgr restore error\n");
        return ret;
    }

    vidmm_restore(adapter);
    util_print_time("vidmm_restore finish, cur time");

    vidsch_restore(adapter);
    util_print_time("vidsch_restore finish, cur time");

    cm_restore(adapter);
    util_print_time("cm_restore finish, cur time");

    return ret;
}
