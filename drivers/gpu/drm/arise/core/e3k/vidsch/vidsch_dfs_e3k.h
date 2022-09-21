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

#ifndef __PM_DFS_E3K_H__

#include "gf_adapter.h"
#include "vidsch.h"
#include "vidschi.h"
#include "chip_include_e3k.h"
#include "util.h"

typedef struct _elite3000_dfs_table
{
    unsigned long index;
    unsigned long freq;      //in Hz
    unsigned long freq_rel;  //real freq

    union
    {
        struct
        {
            unsigned int bp:1; //bypass mode select
            unsigned int od:2; //output divider
            unsigned int bs:1;  //band control, alway use 1 is ok
            unsigned int f:8;  //feed back value
            unsigned int d:4; //C3d/vcp/vpp internal divider 
            unsigned int reserve0:16;
        };
        unsigned int dfs_value0;
    };
    union
    {
        struct
        {
            unsigned int r:19; //input divider, tie to 0, means divider is 1
            unsigned int reserve1:13;
        };
        unsigned int dfs_value1;
    };
}elite3000_dfs_table;

typedef struct Reg_Pll_load
{
    unsigned char auto_load_pll_en:1;
    unsigned char soft_load_set:1;
    unsigned char soft_load_rst:1;
    unsigned char soft_clk:1;    
    unsigned char reserve:4;        
}Reg_Pll_load;    

typedef enum
{
    PLL_DIU_D3_PLLOK=0,//diu
    PLL_DIU_D2_PLLOK,
    PLL_DIU_D1_PLLOK,    
    PLL_DIU_D4_PLLOK,    
    M2PLL_LOCK,//miu
    M1PLL_LOCK,
    M0PLL_LOCK,
    BIU_PMP_PCLK_LOCKED,
    VPLL_LOCK,   
    EPLL_LOCK,        
}PLL_LOCK;

#define ELITE3000_DFS(_idx,_freq,_freq_r,_bp,_od,_bs,_r,_f,_d)    {\
    .index  = _idx,         \
    .freq   = _freq,   \
    .freq_rel = _freq_r,  \
    .bp     = _bp,  \
    .od     = _od,  \
    .bs     = _bs,   \
    .r        = _r,     \
    .f        = _f,      \
    .d       = _d,    \
}


void vidsch_parking_to_PCLK_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr, unsigned int to_parking);
void vidsch_program_divider_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr, unsigned int index);
void vidsch_program_PLL_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr, unsigned int index);

void vidsch_dfs_init_e3k(adapter_t *adapter);

int vidsch_dfs_set_e3k(adapter_t *adapter, gf_dvfs_set_t *dfs_set);
void vidsch_dfs_tuning_e3k(adapter_t *adapter);

int vidsch_power_clock_on_off_e3k(vidsch_mgr_t *sch_mgr, unsigned int off);
void vidsch_power_tuning_e3k(adapter_t *adapter, unsigned int gfx_only);
int vidsch_query_dfs_clamp_e3k(adapter_t *adapter, gf_dvfs_clamp_status_t *dfs_clamp);
#endif
