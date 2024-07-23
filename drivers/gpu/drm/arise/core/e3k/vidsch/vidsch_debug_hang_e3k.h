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

#ifndef __VIDSCH_DEBUG_HANG_E3K_H__
#define __VIDSCH_DEBUG_HANG_E3K_H__

#include "vidsch_engine_e3k.h"
#include "vidsch_dump_image_e3k.h"

typedef enum DEBUG_BUS_SCOPE {
    WHOLE_SCOPE_DEBUG_BUS  = 0,
    CHIP1_SCOPE_DEBUG_BUS = 1,
    CHIP2_SCOPE_DEBUG_BUS = 2,
    CHIP3_SCOPE_DEBUG_BUS = 3,
}DEBUG_BUS_SCOPE;

typedef struct debug_bus_write_reg
{
    unsigned int     group_start_offset;
    unsigned int     group_end_offset;
    //Now Arise and Arise1020 max only need write 3 register
    //most case only need write 2
    unsigned int     write_reg_number;
    unsigned int     write_reg_offset[3];
    unsigned int     write_reg_mask[3];
}debug_bus_write_reg;

typedef struct debug_bus_readback_reg
{
    unsigned int     mmio_offset_0;
    unsigned int     mmio_offset_1;
    unsigned int     mmio_offset_2;
    unsigned int     mmio_offset_3;
}debug_bus_readback_reg;

typedef struct reg_write_readback_info
{
    debug_bus_write_reg debugBusWrite;
    debug_bus_readback_reg debugBusReadback;
}reg_write_readback_info;

typedef struct debug_bus_info
{
    char             group_name[50];
    DEBUG_BUS_SCOPE  scope;
    unsigned int     gpcIndex;
    reg_write_readback_info regOpinfo;
}debug_bus_info;

static const debug_bus_info  debug_bus_info_E3K[] =
{
    //group_name scope gpcIndex {{group_start_offset,group_start_offset,write_reg_number,write_reg_offset[3],write_reg_mask[3]},
    //                          {mmio_offset_0,mmio_offset_1,mmio_offset_2,mmio_offset_3}}

    //GPC0
    "GFVD0 group"     ,CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x2000,0x201F,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "CMU group"       ,CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x2800,0x29FF,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "MIU2 group"      ,CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x3000,0x301F,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "BIU0 group"      ,CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x3800,0x3809,3,0x88C8,0x88CC,0x88CF,0x3800,0x0700,0x00FF },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "BIU1 group"      ,CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x3C00,0x3F7F,3,0x88C8,0x88CC,0x88CF,0x3800,0x0700,0x00FF },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },

    "SLICE0 EUC group",CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x0000,0x00FF,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE0 PEA group",CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x0100,0x017F,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE0 PEB group",CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x0180,0x01FF,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE0 TU  group",CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x0200,0x023F,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE0 FFU group",CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x0280,0x02FF,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },

    "SLICE1 EUC group",CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x0400,0x04FF,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE1 PEA group",CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x0500,0x057F,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE1 PEB group",CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x0580,0x05FF,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE1 TU  group",CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x0600,0x063F,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE1 FFU group",CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x0680,0x06FF,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },

    "SLICE2 EUC group",CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x0800,0x08FF,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE2 PEA group",CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x0900,0x097F,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE2 PEB group",CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x0980,0x09FF,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE2 TU  group",CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x0A00,0x0A3F,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE2 FFU group",CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x0A80,0x0AFF,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },

    "SLICE3 EUC group",CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x0C00,0x0CFF,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE3 PEA group",CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x0D00,0x0D7F,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE3 PEB group",CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x0D80,0x0DFF,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE3 TU  group",CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x0E00,0x0E3F,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE3 FFU group",CHIP1_SCOPE_DEBUG_BUS, 0,{ { 0x0E80,0x0EFF,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },

    "GPC CENTRAL SPTFE group", CHIP1_SCOPE_DEBUG_BUS,0,{ { 0x1000,0x103F,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "GPC CENTRAL SGTBE group", CHIP1_SCOPE_DEBUG_BUS,0,{ { 0x1800,0x183F,2,0x88C8,0x88C9,0,0x3F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },

    //GPC1
    "GFVD1 group"     ,CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x2000,0x201F,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },
    "MXUB group"      ,CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x2800,0x28FF,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },
    "MIU1 group"      ,CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x3000,0x301F,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },
    "L2 group"        ,CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x3800,0x387F,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },

    "SLICE0 EUC group",CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x0000,0x00FF,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },
    "SLICE0 PEA group",CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x0100,0x017F,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },
    "SLICE0 PEB group",CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x0180,0x01FF,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },
    "SLICE0 TU  group",CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x0200,0x023F,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },
    "SLICE0 FFU group",CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x0280,0x02FF,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },

    "SLICE1 EUC group",CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x0400,0x04FF,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },
    "SLICE1 PEA group",CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x0500,0x057F,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },
    "SLICE1 PEB group",CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x0580,0x05FF,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },
    "SLICE1 TU  group",CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x0600,0x063F,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },
    "SLICE1 FFU group",CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x0680,0x06FF,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },

    "SLICE2 EUC group",CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x0800,0x08FF,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },
    "SLICE2 PEA group",CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x0900,0x097F,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },
    "SLICE2 PEB group",CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x0980,0x09FF,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },
    "SLICE2 TU  group",CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x0A00,0x0A3F,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },
    "SLICE2 FFU group",CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x0A80,0x0AFF,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },

    "SLICE3 EUC group",CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x0C00,0x0CFF,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },
    "SLICE3 PEA group",CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x0D00,0x0D7F,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },
    "SLICE3 PEB group",CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x0D80,0x0DFF,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },
    "SLICE3 TU  group",CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x0E00,0x0E3F,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },
    "SLICE3 FFU group",CHIP2_SCOPE_DEBUG_BUS, 1,{ { 0x0E80,0x0EFF,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },

    "GPC CENTRAL SPTFE group", CHIP2_SCOPE_DEBUG_BUS,1,{ { 0x1000,0x103F,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },
    "GPC CENTRAL SGTBE group", CHIP2_SCOPE_DEBUG_BUS,1,{ { 0x1800,0x183F,2,0x88CA,0x88CB,0,0x3F00,0x00FF,0 },{ 0x88C4,0x88C5,0x88C6,0x88C7 } },

    //GPC2
    "VPP group"       ,CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x2000,0x207F,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },
    "MIU0 group"      ,CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x2800,0x281F,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },
    "DIU group"       ,CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x3000,0x3EE6,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },

    "SLICE0 EUC group",CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x0000,0x00FF,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },
    "SLICE0 PEA group",CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x0100,0x017F,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },
    "SLICE0 PEB group",CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x0180,0x01FF,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },
    "SLICE0 TU  group",CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x0200,0x023F,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },
    "SLICE0 FFU group",CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x0280,0x02FF,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },

    "SLICE1 EUC group",CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x0400,0x04FF,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },
    "SLICE1 PEA group",CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x0500,0x057F,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },
    "SLICE1 PEB group",CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x0580,0x05FF,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },
    "SLICE1 TU  group",CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x0600,0x063F,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },
    "SLICE1 FFU group",CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x0680,0x06FF,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },

    "SLICE2 EUC group",CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x0800,0x08FF,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },
    "SLICE2 PEA group",CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x0900,0x097F,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },
    "SLICE2 PEB group",CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x0980,0x09FF,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },
    "SLICE2 TU  group",CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x0A00,0x0A3F,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },
    "SLICE2 FFU group",CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x0A80,0x0AFF,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },

    "SLICE3 EUC group",CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x0C00,0x0CFF,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },
    "SLICE3 PEA group",CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x0D00,0x0D7F,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },
    "SLICE3 PEB group",CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x0D80,0x0DFF,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },
    "SLICE3 TU  group",CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x0E00,0x0E3F,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },
    "SLICE3 FFU group",CHIP3_SCOPE_DEBUG_BUS, 2,{ { 0x0E80,0x0EFF,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },

    "GPC CENTRAL SPTFE group", CHIP3_SCOPE_DEBUG_BUS,2,{ { 0x1000,0x103F,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },
    "GPC CENTRAL SGTBE group", CHIP3_SCOPE_DEBUG_BUS,2,{ { 0x1800,0x183F,2,0x88CC,0x88CF,0,0x3F00,0x00FF,0 },{ 0x88FC,0x88FD,0x88FE,0x88FF } },
};

static const debug_bus_info  debug_bus_info_Arise1020[] =
{
    //group_name scope gpcIndex {{group_start_offset,group_start_offset,write_reg_number,write_reg_offset[3],write_reg_mask[3]},
    //                          {mmio_offset_0,mmio_offset_1,mmio_offset_2,mmio_offset_3}}

    //GPC0
    "GFVD0 group"     ,CHIP1_SCOPE_DEBUG_BUS,0,{ { 0x2000,0x27FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "CMU group"       ,CHIP1_SCOPE_DEBUG_BUS,0,{ { 0x2800,0x2FFF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    //BIU need keep 0xA01F0[31] = 1, so change 0x3800 to 0x80003800 fit this
    "BIU Peri group"  ,CHIP1_SCOPE_DEBUG_BUS,0,{ { 0x3800,0x3809,3,0x88C8,0x88C9,0xA01F0,0x7F00,0x00FF,0x7FFF},{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "BIU PCIE group"  ,CHIP1_SCOPE_DEBUG_BUS,0,{ { 0x3C00,0x3F7F,3,0x88C8,0x88C9,0xA01F0,0x7F00,0x00FF,0x7FFF},{ 0x88C0,0x88C1,0x88C2,0x88C3 } },

    "MIU0 group"      ,CHIP2_SCOPE_DEBUG_BUS,0,{ { 0x3000,0x37FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "VPP group"       ,CHIP2_SCOPE_DEBUG_BUS,0,{ { 0x4000,0x47FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "DIU group"       ,CHIP2_SCOPE_DEBUG_BUS,0,{ { 0x5000,0x5FFF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },

    "MXUB group"      ,CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x6800,0x68BF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "L2 group"        ,CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x7800,0x7FFF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },

    "SLICE0 EUC group",CHIP1_SCOPE_DEBUG_BUS,0,{ { 0x0000,0x00FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE0 PEA group",CHIP1_SCOPE_DEBUG_BUS,0,{ { 0x0100,0x017F,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE0 PEB group",CHIP1_SCOPE_DEBUG_BUS,0,{ { 0x0180,0x01FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE0 TU  group",CHIP1_SCOPE_DEBUG_BUS,0,{ { 0x0200,0x027F,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE0 FFU group",CHIP1_SCOPE_DEBUG_BUS,0,{ { 0x0280,0x02FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },

    "SLICE1 EUC group",CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x0400,0x04FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE1 PEA group",CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x0500,0x057F,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE1 PEB group",CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x0580,0x05FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE1 TU  group",CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x0600,0x067F,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE1 FFU group",CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x0680,0x06FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },

    "GPC CENTRAL SPTFE group", CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x1000,0x13FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "GPC CENTRAL SGTBE group", CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x1800,0x1FFF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
};

static const debug_bus_info  debug_bus_info_Arise2030[] =
{
    //group_name scope gpcIndex {{group_start_offset,group_end_offset,write_reg_number,write_reg_offset[3],write_reg_mask[3]},
    //                          {mmio_offset_0,mmio_offset_1,mmio_offset_2,mmio_offset_3}}

    //GPC0
    "S3VD0 group"     ,CHIP1_SCOPE_DEBUG_BUS,0,{ { 0x2000,0x27FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "CMU group"       ,CHIP1_SCOPE_DEBUG_BUS,0,{ { 0x2800,0x2FFF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    //BIU need keep 0xA01F0[31] = 1, so change 0x3800 to 0x80003800 fit this
    "BIU Peri group"  ,CHIP1_SCOPE_DEBUG_BUS,0,{ { 0x3800,0x3809,3,0x88C8,0x88C9,0xA01F0,0x7F00,0x00FF,0x7FFF},{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "BIU PCIE group"  ,CHIP1_SCOPE_DEBUG_BUS,0,{ { 0x3C00,0x3F7F,3,0x88C8,0x88C9,0xA01F0,0x7F00,0x00FF,0x7FFF},{ 0x88C0,0x88C1,0x88C2,0x88C3 } },

    "MIU0 group"      ,CHIP2_SCOPE_DEBUG_BUS,0,{ { 0x3000,0x37FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "VPP group"       ,CHIP2_SCOPE_DEBUG_BUS,0,{ { 0x4000,0x47FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "DIU group"       ,CHIP2_SCOPE_DEBUG_BUS,0,{ { 0x5000,0x5FFF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },

    "MXUB group"      ,CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x6800,0x6FFF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "MIU1 group"      ,CHIP2_SCOPE_DEBUG_BUS,0,{ { 0x7000,0x77FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "L2 group"        ,CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x7800,0x7FFF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },

    "SLICE0 EUC group",CHIP1_SCOPE_DEBUG_BUS,0,{ { 0x0000,0x00FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE0 PEA group",CHIP1_SCOPE_DEBUG_BUS,0,{ { 0x0100,0x017F,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE0 PEB group",CHIP1_SCOPE_DEBUG_BUS,0,{ { 0x0180,0x01FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE0 TU  group",CHIP1_SCOPE_DEBUG_BUS,0,{ { 0x0200,0x027F,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE0 FFU group",CHIP1_SCOPE_DEBUG_BUS,0,{ { 0x0280,0x02FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },

    "SLICE1 EUC group",CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x0400,0x04FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE1 PEA group",CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x0500,0x057F,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE1 PEB group",CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x0580,0x05FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE1 TU  group",CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x0600,0x067F,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE1 FFU group",CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x0680,0x06FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },

    "SLICE2 EUC group",CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x0800,0x08FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE2 PEA group",CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x0900,0x097F,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE2 PEB group",CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x0980,0x09FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE2 TU  group",CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x0A00,0x0A7F,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE2 FFU group",CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x0A80,0x0AFF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },

    "SLICE3 EUC group",CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x0C00,0x0CFF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE3 PEA group",CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x0D00,0x0D7F,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE3 PEB group",CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x0D80,0x0DFF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE3 TU  group",CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x0E00,0x0E7F,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "SLICE3 FFU group",CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x0E80,0x0EFF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },

    "GPC CENTRAL SPTFE group", CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x1000,0x13FF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
    "GPC CENTRAL SGTBE group", CHIP3_SCOPE_DEBUG_BUS,0,{ { 0x1800,0x1BFF,2,0x88C8,0x88C9,0,0x7F00,0x00FF,0 },{ 0x88C0,0x88C1,0x88C2,0x88C3 } },
};

#define COMPATIBLE_TO_WINDOW                1

#define SAVE_BEFORE_PREHANG                 1 //for prehangdump:save resource,DMA,CONTEXT
#define SAVE_AFTER_PREHANG                  2 //for prehangdump:save ringbuffer and modify DMA.context restore,fence commands
#define SAVE_BEFORE_POSTHANG                3 //for posthangdump:save CONTEXT
#define SAVE_AFTER_POSTHANG                 4 //for posthangdump:save resource,DMA,ringbuffer and modify DMA.context restore,fence commands
#define SAVE_RING_BUFFER                    5 //save ringbuffer only.
#define SAVE_FORMAT_BUFFER                  6 //save formatbuffer only.


// Debug hang fence offset
#define DH_FENCE_OFFSET         (0x40)
#define DH_FENCE_VALUE          (0x12345678)
#define DH_XBUFFER_SIZE         (0x10000)
#define DUPLICATE_TIME_OUT      (10) //10s

//multi dma number

#define     MAX_HANG_DUMP_DATA_POOL_NUMBER         5
#define     HANG_DUMP_CONTEXT_BUFFER_OFFSET        1
#define     HANG_DUMP_DMA_BUFFER_OFFSET            2
#define     HANG_DUMP_RING_BUFFER_OFFSET           3
#define     HANG_DUMP_DH_COMMON_INFO_OFFSET        4
#define     HANG_DUMP_FORMAT_BUFFER_OFFSET         5

typedef struct dh_file_info_e3k
{
    large_inter FbNotCpuVisibleSize;
    unsigned int  FbCpuVisibleSize;
    unsigned int  dma_for_hang_size;
    unsigned int  context_for_hang_size;
    unsigned int  ring_for_hang_size;
}dh_file_info_e3k;

typedef struct dh_common_info_e3k
{
    unsigned int    current_hang_dump_index;
    unsigned int    hang_dump_counter;
    unsigned int    hang_dma_index;
    unsigned long long hang_dma_fence_id;
    unsigned int    hang_rb_index;
    unsigned long long fence_id[MAX_HANG_DUMP_DATA_POOL_NUMBER];
    unsigned int    rb_index[MAX_HANG_DUMP_DATA_POOL_NUMBER];

    //reserved for ringbuffer/fence_buffer/flush_fifo_buffer when real kickoff
    unsigned int ring_buffer[2048];
    unsigned long long fence_buffer;
    unsigned long long flush_fifo_buffer;
}dh_common_info_e3k;


typedef struct dh_rb_info_e3k
{
    EngineSatus_e3k status;
    unsigned int         last_rb_size;
    unsigned int         last_rb_index;
}dh_rb_info_e3k;

//this will replace above dh_file_info_e3k for compatible mode
typedef struct submit_info
{
    unsigned long long ring_buffer_gpu_va;
    unsigned long long kickoff_ring_buffer_gpu_va;
    unsigned long long dma_buffer_gpu_va;
    unsigned long long context_buffer_gpu_va;
    unsigned long long fence_buffer_gpu_va;

    void* ring_buffer_cpu_va;
    void* kickoff_ring_buffer_cpu_va;
    void* dma_buffer_cpu_va;
    void* context_buffer_cpu_va;
    void* fence_buffer_cpu_va;

    int rb_index;

    EngineSatus_e3k status;
} submit_info_t;

typedef struct _dup_hang_ctx
{
    struct os_file     *file;
    adapter_t          *adapter;
    unsigned int        device_id;
    unsigned int        slice_mask;
    unsigned long long  local_memory_size;
    unsigned long long  pcie_memory_size;
    unsigned long long  local_visible_size;
    unsigned long long  local_unvisible_size;

    unsigned long long  record_number;

    unsigned long long  dma_buffer_offset;
    unsigned long long  single_dma_size_in_byte;

    unsigned long long  context_buffer_offset;
    unsigned long long  single_context_buffer_size_in_byte;

    unsigned long long  ring_buffer_offset;
    unsigned long long  single_ring_buffer_size_in_byte;

    unsigned long long  gart_table_l3_offset;
    unsigned long long  gart_table_l2_offset;
    unsigned long long  dummy_page_entry;
    unsigned long long  bl_buffer_offset;

    void                *local_visible_memory_cpu_va;

    submit_info_t       submit_info;
}dup_hang_ctx_t;

#define HangDump_SingleDmaBufferSize        (64 * 1024)                    // 64k
#define HangDump_ContextBufferBlockSize     (CONTEXT_BUFFER_SIZE)          // 0xc000
// Ringbuffer size;0x30 DWORD, RingBufferBlockSize = 0xA4 Byte
#define HangDump_RingBufferBlockSize        ((sizeof(dh_rb_info_e3k)) + sizeof(RINGBUFFER_COMMANDS_E3K))

extern int use_hw_dump;

extern void vidsch_duplicate_hang_e3k(adapter_t *adapter);
extern void vidsch_dump_hang_e3k(adapter_t *adapter);
extern int vidsch_save_misc_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr,  unsigned int what_to_save);
extern void vidsch_handle_dbg_hang_dump_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr, task_dma_t *task_dma);

typedef union
{
    struct
    {
        unsigned int wait                  : 1; // wait after submit.
        unsigned int print_hang            : 1; // print out logged hang info.
        unsigned int reset_hw              : 1; // reset hardware.
        unsigned int pre_hang_dump         : 1; // before hang,Save DMA,RingBuffer,context and all resources into framebuffer mirror
        unsigned int post_hang_dump        : 1; // After hang,Save DMA,RingBuffer,context and all resources into framebuffer mirror
        unsigned int duplicate_hang        : 1; // resore the enviroment saved by PreHangDump/PostHangDump to duplcate the hang
        unsigned int internal_dump_hw      : 1; // Dump framebuffer mirror to file so that duplicat the hang .
        unsigned int internal_block_submit : 1; // block sumbit during replaying
        unsigned int print_debug_bus       : 1; // print out debug bus info.
        unsigned int start_test_dump       : 1; // start to test hang dump code
        unsigned int multi_dma_number      : 5; // 0-single dma
        unsigned int Reserved              : 17;
    };
    unsigned int uint;
} reg_debug_mode_e3k;

extern reg_debug_mode_e3k debug_mode_e3k;
extern void vidsch_display_debugbus_info_e3k(adapter_t *adapter, struct os_printer *p, int video);
extern void vidsch_dump_hang_compatible_e3k(adapter_t *adapter);
extern void vidsch_duplicate_hang_compatible_e3k(adapter_t * adapter);
extern unsigned char *vidschi_get_dump_data_cpu_virt_addr_e3k(adapter_t *adapter, unsigned int type, unsigned int index);
extern void vidschi_copy_mem_e3k(adapter_t *adapter,
                                 unsigned long long dst_phys_addr,
                                 void *dst_virt_addr,
                                 unsigned long long src_phys_addr,
                                 void *src_virt_addr,
                                 unsigned int total_size);

void vidsch_trigger_duplicate_hang_e3k(adapter_t *adapter);

#endif
