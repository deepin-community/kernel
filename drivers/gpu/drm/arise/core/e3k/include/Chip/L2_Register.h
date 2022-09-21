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

#ifndef _L2_REGISTER_H
#define _L2_REGISTER_H


#ifndef        L2_BLOCKBASE_INF
    #define    L2_BLOCKBASE_INF
    #define    BLOCK_L2_VERSION 1
    #define    BLOCK_L2_TIMESTAMP "2019/3/1 9:46:22"
    #define    L2_BLOCK                                                   0xB
    #define    L2_REG_START                                               0x0
    #define    L2_REG_END                                                 0x10
    #define    L2_REG_LIMIT                                               0x10
#endif


#define        Reg_Rb0_Offset                                             0x0
#define        Reg_Rb1_Offset                                             0x1
#define        Reg_Rb2_Offset                                             0x2
#define        Reg_Argument_Offset                                        0x3
#define        Reg_L2_Performance_Offset                                  0x4
#define        Reg_L2_Reserved_Offset                                     0x5
#define        Reg_L2_Reserved1_Offset                                    0xC








typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved_0                : 20;
        unsigned int Reserved_1                : 12;
    } reg;
} Reg_Rb0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved_0                : 20;
        unsigned int Reserved_1                : 12;
    } reg;
} Reg_Rb1;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved_0                : 20;
        unsigned int Reserved_1                : 12;
    } reg;
} Reg_Rb2;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved_0                : 20;
        unsigned int Reserved_1                : 12;
    } reg;
} Reg_Argument;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int L2_Dist_Ctrl              : 1;




        unsigned int Reserved0                 : 1;
        unsigned int L2_Latency_Buf_Dist_Ctrl : 1;








        unsigned int Reserved1                 : 1;
        unsigned int L2_Slc_Osd_Num_Ctrl       : 4;




        unsigned int L2_Tu_Block_By_Refcnt     : 1;









        unsigned int L2_Fully_W_Opt            : 1;






        unsigned int Reserved2                 : 22;
    } reg;
} Reg_L2_Performance;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_L2_Reserved;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_L2_Reserved1;

typedef struct _L2_regs
{
    Reg_Rb0                          reg_Rb0;
    Reg_Rb1                          reg_Rb1;
    Reg_Rb2                          reg_Rb2;
    Reg_Argument                     reg_Argument;
    Reg_L2_Performance               reg_L2_Performance;
    Reg_L2_Reserved                  reg_L2_Reserved[7];
    Reg_L2_Reserved1                 reg_L2_Reserved1[4];
} L2_regs;

#endif
