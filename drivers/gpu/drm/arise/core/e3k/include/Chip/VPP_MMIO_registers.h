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

#ifndef _VPP_MMIO_REGISTERS_H
#define _VPP_MMIO_REGISTERS_H


#ifndef        VPP_MMIO_BLOCKBASE_INF
    #define    VPP_MMIO_BLOCKBASE_INF
    #define    BLOCK_VPP_MMIO_VERSION 1
    #define    BLOCK_VPP_MMIO_TIMESTAMP "2013-09-12 15:44:40"
    #define    VPP_MMIO_BLOCK                                             0x0
    #define    VPP_MMIO_REG_START                                         0x0
    #define    VPP_MMIO_REG_END                                           0x4
    #define    VPP_MMIO_REG_LIMIT                                         0x4
#endif


#define        Reg_M4_Reg_Offset                                          0x0







typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Fd_Mode                : 2;
        unsigned int Gb_Bls_Gain               : 10;
        unsigned int Reserved_M4               : 20;
    } reg;
} Reg_M4_Reg;

typedef struct _Vpp_Mmio_regs
{
    Reg_M4_Reg                       reg_M4_Reg[4];
} Vpp_Mmio_regs;

#endif
