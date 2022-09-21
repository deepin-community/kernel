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

#ifndef __M4SIM__
#define __M4SIM__

#include "VPP_global_registers_helper.h"
#include "VPP_global_registers.h"

#ifdef __cplusplus
extern "C" {
#endif

extern Vpp_Global_regs* pReg;


typedef long long uint64;
typedef unsigned int uint32;



enum FilmCheckResult
{
    enumFC_DeInterlace = 0,
    enumFC_FrameMerge = 1,
    enumFC_FrameCopy = 2,
};

struct FD_frame_info_struct
{
    unsigned int MD_2v2:1;
    unsigned int MD_2v2_small:1;
    unsigned int MD_Failed_2v2:1;
    unsigned int Counter_2v2:1;
    unsigned int Counter_Failed_2v2:1;
    unsigned int FD_pattern:2;
    unsigned int Sence_change:1;
    unsigned int No_FD_mode: 1;
};

struct FD_detect_info_struct
{
    unsigned int In_2v2_mode:1;
    unsigned int MD_2v2_Counter:8;
    unsigned int C_2v2_Counter:8;
    unsigned int Failed_2v2_counter:4;
    unsigned int FD_Detect_counter:5;
    unsigned int FD_Counter:4;

    unsigned int FD_P01:4;
    unsigned int FD_P10:4;
    unsigned int FD_P00_Copy:4;
};

struct FD_query_dump_info_struct
{
    uint64 m_FD_TopCounter, m_FD_BottomCounter;

    uint32 m_SceneChang_Top;
    uint32 m_SceneChang_Bottom;

    uint32 m_2v2_counter[3];
    uint32 m_2v2_MD_counter[4];

    uint32 index;
    uint32 top_first;
    uint32 ret_fd;

    uint32 Gb_Fd_Detect_Counter_Init;
    uint32 Gb_Fd_Counter_Init;
    uint32 Gb_2v2_Failed_Counter_Th;
    uint32 Gb_2v2_Md_Succeed_counter_Th;
    uint32 Gb_2v2_Sad_Succeed_counter_Th;

    uint32 Gb_2v2_Failed_Md_Counter_Th;
    uint32 Gb_2v2_Sad_Counter_Th;
    uint32 Gb_Scene_Change_Th;
    uint32 Gb_2v2_Md_Counter_Small_Th;
    uint32 Gb_Fd_Counter_Th;
    uint32 Gb_2v2_Sad_Failed_Range;
    uint32 Gb_2v2_Md_Counter_Th;


    uint32 Gb_2v2_Frame_Cnt_Th;
    uint32 Gb_2v2_Failed_Counter_Th2;

};


extern int m4_fd_mode(struct FD_query_dump_info_struct * s,Vpp_Global_regs* _pReg);


#ifdef __cplusplus
}
#endif


#endif
