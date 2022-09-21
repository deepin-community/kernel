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

#ifndef _SPIN_REGISTER_H
#define _SPIN_REGISTER_H


#ifndef        SPIN_BLOCKBASE_INF
    #define    SPIN_BLOCKBASE_INF
    #define    BLOCK_SPIN_VERSION 1
    #define    BLOCK_SPIN_TIMESTAMP "2017/9/27 15:04:28"
    #define    SPIN_BLOCK                                                 0x2
    #define    SPIN_REG_START                                             0x0
    #define    SPIN_REG_END                                               0x8
    #define    SPIN_REG_LIMIT                                             0x8
#endif


#define        Reg_Sp_Dummy1_Offset                                       0x0
#define        Reg_Sp_Dummy2_Offset                                       0x1
#define        Reg_Sp_Reserved_Offset                                     0x2







typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Sp_Dummy1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Sp_Dummy2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Sp_Reserved;

typedef struct _Spin_regs
{
    Reg_Sp_Dummy1                    reg_Sp_Dummy1;
    Reg_Sp_Dummy2                    reg_Sp_Dummy2;
    Reg_Sp_Reserved                  reg_Sp_Reserved[6];
} Spin_regs;

#endif
