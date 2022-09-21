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

#ifndef __REGISTER_E3K_H__
#define __REGISTER_E3K_H__

#include "core_import.h"

#define AR_INIT_REG     0x83DA
#define AR_INDEX        0x83C0
#define AR_DATA         0x83C1
#define CR_INDEX        0x83D4
#define CR_DATA         0x83D5
#define SR_INDEX        0x83C4
#define SR_DATA         0x83C5

//mmioOffset of registers
#define MMIO_OFFSET_SR_GROUP_A_E3K  0x8600
#define MMIO_OFFSET_SR_GROUP_B_E3K  0x8700
#define MMIO_OFFSET_CR_GROUP_A_E3K  0x8800
#define MMIO_OFFSET_CR_GROUP_B_E3K  0x8900
#define MMIO_OFFSET_CR_GROUP_C_E3K  0x8A00
#define MMIO_OFFSET_CR_GROUP_D_E3K  0x8B00  // for 4-channel MIU CR_D registers
#define MMIO_OFFSET_CR_GROUP_D0_E3K 0x8C00  // MIU channel 0 CR_D registers
#define MMIO_OFFSET_CR_GROUP_D1_E3K 0x8D00  // MIU channel 1 CR_D registers
#define MMIO_OFFSET_CR_GROUP_D2_E3K 0x8E00  // MIU channel 2 CR_D registers
#define MMIO_OFFSET_CR_GROUP_D3_E3K 0x8F00  // MIU channel 3 CR_D registers
#define MMIO_OFFSET_SR_GROUP_T_E3K   0x9400
#define MMIO_OFFSET_CR_GROUP_T_E3K   0x9500

/* same to  CBIOS_REGISTER_TYPE */
enum
{
    /* CR_SET = 0,     //CR registers to be set, used in s3postreg */
    /* SR_SET,         //SR registers to be set */
    /* GR_SET,         //GR registers to be set */
    CR=0,                /*CR registers           */
    SR,                /*SR registers           */
    AR,
    GR,
    MISC,
    CR_X,
    SR_X,
    CR_B,               /* B-set of CR */
    CR_C,              /* C-set of CR */
    CR_D,               /* D-set of CR */
    CR_T,               /* CR on IGA3  */
    SR_T,               /* SR on IGA3  */
    SR_B,
    CR_D_0,
    CR_D_1,
    CR_D_2,
    CR_D_3,
    RESERVED=0xFF
};

static __inline__ void write_reg_e3k(unsigned char *mmio, int type, unsigned char index, unsigned char value, unsigned char mask)
{
    unsigned int  offset = 0;
    unsigned char temp   = 0;

    switch(type)
    {
    case CR:
        offset = MMIO_OFFSET_CR_GROUP_A_E3K + index;
        break;
    case CR_B:
        offset = MMIO_OFFSET_CR_GROUP_B_E3K + index;
        break;
    case CR_C:
        offset = MMIO_OFFSET_CR_GROUP_C_E3K + index;
        break;
    case SR:
        offset = MMIO_OFFSET_SR_GROUP_A_E3K + index;
        break;
    case SR_B:
        offset = MMIO_OFFSET_SR_GROUP_B_E3K + index;
        break;
    case SR_T:
        offset = MMIO_OFFSET_SR_GROUP_T_E3K + index;
        break;
    case CR_T:
        offset = MMIO_OFFSET_CR_GROUP_T_E3K + index;
        break;
    default:
        gf_assert(0, NULL);
        break;
    }

    temp   = gf_read8(mmio + offset);
    temp   = (temp & mask) | (value & ~mask);

    gf_write8(mmio + offset, temp);
}

static __inline__ unsigned char read_reg_e3k(unsigned char *mmio, int type, unsigned char index)
{
    unsigned int  offset = 0;
    unsigned char temp   = 0;

    switch(type)
    {
    case CR:
        offset = MMIO_OFFSET_CR_GROUP_A_E3K + index;
        break;
    case CR_B:
        offset = MMIO_OFFSET_CR_GROUP_B_E3K + index;
        break;
    case CR_C:
        offset = MMIO_OFFSET_CR_GROUP_C_E3K + index;
        break;
    case SR:
        offset = MMIO_OFFSET_SR_GROUP_A_E3K + index;
        break;
    case SR_B:
        offset = MMIO_OFFSET_SR_GROUP_B_E3K + index;
        break;
    case AR:
        gf_read8(mmio + AR_INIT_REG);
        gf_write8(mmio + AR_INDEX, index);
        temp = gf_read8(mmio + AR_DATA);
        return temp;
    default:
        gf_assert(0, NULL);
        break;
    }

    temp = gf_read8(mmio + offset);

    return temp;
}
#endif

