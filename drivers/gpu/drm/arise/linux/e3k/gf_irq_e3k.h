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

#ifndef _GF_IRQ_E3K_
#define _GF_IRQ_E3K_

#include "../gf_irq.h"


#define INTR_EN_REG  0x8508
#define ADV_INTR_EN_REG 0x854c
#define INTR_SHADOW_REG  0x8574
#define ADV_SHADOW_REG  0x8578

typedef enum _BIU_INTR_BIT
{
    VSYNC1_INT  = 0x1,
    VSYNC2_INT  = 0x4,
    VSYNC3_INT  = 0x8,
    VSYNC4_INT  = 0x10,
    DP1_INT     = 0x80,
    DP2_INT     = 0x100,
    DP3_INT     = 0x10000,
    DP4_INT     = 0x20000,
    HDCP_INT   = 0x400000,
    VIP1_INT    = 0x1000000,
    HDCODEC_INT = 0x4000000,
    VIP2_INT    = 0x8000000,
    VIP3_INT    = 0x10000000,
    VIP4_INT    = 0x40000000,
}BIU_INTR_BIT;

typedef enum _CSP_INTR_BIT
{
    FE_HANG_VD0_INT  = 0x1,
    BE_HANG_VD0_INT  = 0x2,
    FE_ERROR_VD0_INT = 0x4,
    BE_ERROR_VD0_INT = 0x8,
    FE_HANG_VD1_INT  = 0x10,
    BE_HANG_VD1_INT  = 0x20,
    FE_ERROR_VD1_INT = 0x40,
    BE_ERROR_VD1_INT = 0x80,
    ENGINE_FENCE_INT = 0x8000000,
}CSP_INTR_BIT;

typedef struct _intr_info
{
    unsigned int biu_intr_bits;
    unsigned int csp_intr_bits;
}intr_info_t;

#endif
