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

#ifndef _MIU_REG_H
#define _MIU_REG_H


#ifndef        MIU_BLOCKBASE_INF
    #define    MIU_BLOCKBASE_INF
    #define    BLOCK_MIU_VERSION 1
    #define    BLOCK_MIU_TIMESTAMP "2011-7-13 11:14:15"
    #define    MIU_BLOCK                                                  0xF
    #define    MIU_REG_START                                              0x0
    #define    MIU_REG_END                                                0x38
    #define    MIU_REG_LIMIT                                              0x38
#endif


#define        Reg_Skip_Offset                                            0x0
#define        Reg_Dynamic_Fb_Control_Offset                              0x35
#define        Reg_Dynamic_Fb_Index_Offset                                0x36
#define        Reg_Dynamic_Fb_Content_Offset                              0x37







typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Skip;

typedef union
{
    struct
    {
        unsigned int Dsref_En                  : 1;
        unsigned int Pd_En                     : 1;
        unsigned int Dynamic_Fb_En             : 1;
        unsigned int Reserved                  : 13;
        unsigned int Dsref_Cyc                 : 16;
    } reg;
    unsigned int uint;
} Reg_Dynamic_Fb_Control;

typedef union
{
    struct
    {
        unsigned int Index                     : 7;
        unsigned int Reserved                  : 25;
    } reg;
    unsigned int uint;
} Reg_Dynamic_Fb_Index;

typedef union
{
    struct
    {
        unsigned int Reserved                  : 16;
        unsigned int Page_Disable              : 1;
        unsigned int Address                   : 15;
    } reg;
    unsigned int uint;
} Reg_Dynamic_Fb_Content;

typedef struct _Miu_regs
{
    Reg_Skip                         reg_Skip[53];
    Reg_Dynamic_Fb_Control           reg_Dynamic_Fb_Control;
    Reg_Dynamic_Fb_Index             reg_Dynamic_Fb_Index;
    Reg_Dynamic_Fb_Content           reg_Dynamic_Fb_Content;
} Miu_regs;

#endif
