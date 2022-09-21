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

#ifndef _MMU_REGISTERS_H
#define _MMU_REGISTERS_H


#ifndef        MMU_BLOCKBASE_INF
    #define    MMU_BLOCKBASE_INF
    #define    BLOCK_MMU_VERSION 1
    #define    BLOCK_MMU_TIMESTAMP "2017/8/17 15:52:06"
    #define    MMU_BLOCK                                                  0x12
    #define    MMU_REG_START                                              0x0
    #define    MMU_REG_END                                                0x1000
    #define    MMU_REG_LIMIT                                              0x1000
#endif


#define        Reg_Mmu_Ttbr_Offset                                        0x0







typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Valid                     : 1;
        unsigned int Ns                        : 1;
        unsigned int Snoop                     : 1;
        unsigned int Segment                   : 1;
        unsigned int Addr                      : 24;
        unsigned int Reserved                  : 4;
    } reg;
} Reg_Ttbr;

typedef struct _Group_Mmu_Ttbr
{
    Reg_Ttbr                         reg_Ttbr;
} Reg_Mmu_Ttbr_Group;

typedef struct _Mmu_regs
{
    Reg_Mmu_Ttbr_Group               reg_Mmu_Ttbr[4096];
} Mmu_regs;

#endif
