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

#include "gf_irq_e3k.h"

static void gf_translate_interrupt_bits(int sw2hw, intr_info_t* info, int* masks)
{
    if(!info || !masks)
    {
        return;
    }

    if(sw2hw)
    {
        info->biu_intr_bits = 0;
        info->csp_intr_bits = 0;

        info->biu_intr_bits |= (*masks & INT_VSYNC1)? VSYNC1_INT : 0;
        info->biu_intr_bits |= (*masks & INT_VSYNC2)? VSYNC2_INT : 0;
        info->biu_intr_bits |= (*masks & INT_VSYNC3)? VSYNC3_INT : 0;
        info->biu_intr_bits |= (*masks & INT_VSYNC4)? VSYNC4_INT : 0;
        info->biu_intr_bits |= (*masks & INT_DP1)? DP1_INT : 0;
        info->biu_intr_bits |= (*masks & INT_DP2)? DP2_INT : 0;
        info->biu_intr_bits |= (*masks & INT_DP3)? DP3_INT : 0;
        info->biu_intr_bits |= (*masks & INT_DP4)? DP4_INT : 0;
        info->biu_intr_bits |= (*masks & INT_VIP1)? VIP1_INT : 0;
        info->biu_intr_bits |= (*masks & INT_VIP2)? VIP2_INT : 0;
        info->biu_intr_bits |= (*masks & INT_VIP3)? VIP3_INT : 0;
        info->biu_intr_bits |= (*masks & INT_VIP4)? VIP4_INT : 0;
        info->biu_intr_bits |= (*masks & INT_HDCODEC)? HDCODEC_INT : 0;
        info->biu_intr_bits |= (*masks & INT_HDCP)? HDCP_INT : 0;
       
        info->csp_intr_bits |= (*masks & INT_FENCE)? ENGINE_FENCE_INT : 0;    
        info->csp_intr_bits |= (*masks & INT_FE_HANG_VD0)? FE_HANG_VD0_INT : 0;
        info->csp_intr_bits |= (*masks & INT_BE_HANG_VD0)? BE_HANG_VD0_INT : 0;
        info->csp_intr_bits |= (*masks & INT_FE_ERROR_VD0)? FE_ERROR_VD0_INT : 0;
        info->csp_intr_bits |= (*masks & INT_BE_ERROR_VD0)? BE_ERROR_VD0_INT : 0;
        info->csp_intr_bits |= (*masks & INT_FE_HANG_VD1)? FE_HANG_VD1_INT : 0;
        info->csp_intr_bits |= (*masks & INT_BE_HANG_VD1)? BE_HANG_VD1_INT : 0;
        info->csp_intr_bits |= (*masks & INT_FE_ERROR_VD1)? FE_ERROR_VD1_INT : 0;
        info->csp_intr_bits |= (*masks & INT_BE_ERROR_VD1)? BE_ERROR_VD1_INT : 0;
    }
    else
    {
        *masks = 0;
        *masks |= (info->biu_intr_bits & VSYNC1_INT)? INT_VSYNC1 : 0;
        *masks |= (info->biu_intr_bits & VSYNC2_INT)? INT_VSYNC2 : 0;
        *masks |= (info->biu_intr_bits & VSYNC3_INT)? INT_VSYNC3 : 0;
        *masks |= (info->biu_intr_bits & VSYNC4_INT)? INT_VSYNC4 : 0;
        *masks |= (info->biu_intr_bits & DP1_INT)? INT_DP1 : 0;
        *masks |= (info->biu_intr_bits & DP2_INT)? INT_DP2 : 0;
        *masks |= (info->biu_intr_bits & DP3_INT)? INT_DP3 : 0;
        *masks |= (info->biu_intr_bits & DP4_INT)? INT_DP4 : 0;
        *masks |= (info->biu_intr_bits & VIP1_INT)? INT_VIP1 : 0;
        *masks |= (info->biu_intr_bits & VIP2_INT)? INT_VIP2 : 0;
        *masks |= (info->biu_intr_bits & VIP3_INT)? INT_VIP3 : 0;
        *masks |= (info->biu_intr_bits & VIP4_INT)? INT_VIP4 : 0;
        *masks |= (info->biu_intr_bits & HDCODEC_INT)? INT_HDCODEC : 0;
        *masks |= (info->biu_intr_bits & HDCP_INT)? INT_HDCP: 0;

        *masks |= (info->csp_intr_bits & ENGINE_FENCE_INT)? INT_FENCE : 0;
        *masks |= (info->csp_intr_bits & FE_HANG_VD0_INT)? INT_FE_HANG_VD0 : 0;
        *masks |= (info->csp_intr_bits & BE_HANG_VD0_INT)? INT_BE_HANG_VD0 : 0;
        *masks |= (info->csp_intr_bits & FE_ERROR_VD0_INT)? INT_FE_ERROR_VD0 : 0;
        *masks |= (info->csp_intr_bits & BE_ERROR_VD0_INT)? INT_BE_ERROR_VD0 : 0;
        *masks |= (info->csp_intr_bits & FE_HANG_VD1_INT)? INT_FE_HANG_VD1 : 0;
        *masks |= (info->csp_intr_bits & BE_HANG_VD1_INT)? INT_BE_HANG_VD1 : 0;
        *masks |= (info->csp_intr_bits & FE_ERROR_VD1_INT)? INT_FE_ERROR_VD1 : 0;
        *masks |= (info->csp_intr_bits & BE_ERROR_VD1_INT)? INT_BE_ERROR_VD1 : 0;
    }
}

static int gf_get_intr_enable_mask_e3k(disp_info_t* disp_info)
{
    adapter_info_t*  adapter = disp_info->adp_info;
    intr_info_t intr_info = {0};
    int masks = 0;
  
    intr_info.biu_intr_bits = gf_read32(adapter->mmio + INTR_EN_REG);
    intr_info.csp_intr_bits = gf_read32(adapter->mmio + ADV_INTR_EN_REG);

    gf_translate_interrupt_bits(0, &intr_info, &masks);

    return masks;    
}

static void gf_set_intr_enable_mask_e3k(disp_info_t* disp_info, int masks)
{
    adapter_info_t*  adapter = disp_info->adp_info;
    intr_info_t intr_info = {0};
    
    gf_translate_interrupt_bits(1, &intr_info, &masks);
    
    if((masks & INT_FENCE) == 0)
    {
        gf_error("Found INT_FENCE disable by user! mask = 0x%08x when set mask\n", masks);
#ifdef _DEBUG_      
        gf_dump_stack();
#endif
    }

    gf_write32(adapter->mmio + INTR_EN_REG, intr_info.biu_intr_bits);
    gf_write32(adapter->mmio + ADV_INTR_EN_REG, intr_info.csp_intr_bits);
}

static void gf_enable_msi_e3k(disp_info_t* disp_info)
{
    unsigned int        tmp;
    adapter_info_t*  adapter = disp_info->adp_info;

    //when MSI is turned on from registry,the MSI address should have be written here by OS
    tmp = gf_read32(adapter->mmio + 0x804C);

    if (tmp != 0)
    {
        tmp = gf_read32(adapter->mmio + 0x8048);
        if ((tmp & 0x10000) == 0)
        {
            // Before turn on MSI we need clear all pending interrupts, clear all BIU interrupts
            gf_write32(adapter->mmio + INTR_SHADOW_REG, 0);

            gf_write32(adapter->mmio + 0x8048, tmp | 0x10000);
        }
    }
}

static void gf_disable_msi_e3k(disp_info_t* disp_info)
{
    unsigned int        temp;
    adapter_info_t*  adapter = disp_info->adp_info;
    
    temp = gf_read32(adapter->mmio + 0x8048);
    temp &= ~0x10000;
    
    gf_write32(adapter->mmio + 0x8048, temp);
}

static int gf_disable_interrupt_e3k(disp_info_t* disp_info)
{
    adapter_info_t*  adapter = disp_info->adp_info;
    int intr_masks;
    unsigned char mm8aa0;
     
    intr_masks = gf_get_intr_enable_mask_e3k(disp_info); //save enabled interrupts, later will restore it in enable_interrupt

    gf_write32(adapter->mmio + INTR_EN_REG, 0); //Disable all BIU interrupts

    gf_wmb();

    //Disable all Advanced interrupts
    gf_write32(adapter->mmio + ADV_INTR_EN_REG, 0x00);
    gf_wmb();

    //Disable adapter interrupts
    mm8aa0 = gf_read8(adapter->mmio + 0x8AA0);
    gf_write8(adapter->mmio + 0x8AA0, mm8aa0 & 0xFE);
    gf_wmb();

    gf_write32(adapter->mmio + INTR_SHADOW_REG, 0x00);   //Clear all interrupts with any write to MM8574

    return intr_masks;
}

static void gf_enable_interrupt_e3k(disp_info_t* disp_info, int intr_masks)
{
    adapter_info_t*  adapter = disp_info->adp_info;
    intr_info_t intr_info = {0};
    unsigned char mm8aa0;
    
    gf_translate_interrupt_bits(1, &intr_info, &intr_masks);
    
    gf_write32(adapter->mmio + INTR_EN_REG, intr_info.biu_intr_bits);   //Enable BIU specified interrupts
    gf_write32(adapter->mmio + ADV_INTR_EN_REG, intr_info.csp_intr_bits);   //Enable advanced interrupts
    gf_wmb();

    //enable adapter interrupt;
    mm8aa0 = gf_read8(adapter->mmio + 0x8AA0);
    gf_write8(adapter->mmio + 0x8AA0, mm8aa0 | 0x01);

    gf_wmb();
    gf_write32(adapter->mmio + INTR_SHADOW_REG, 0x00);   //Clear all interrupts with any write to MM8574
}

static int gf_get_interrupt_mask_e3k(disp_info_t* disp_info)
{
    adapter_info_t*  adapter = disp_info->adp_info;
    intr_info_t intr_info = {0};
    int intr_masks = 0;

    gf_write32(adapter->mmio + INTR_SHADOW_REG, 0);
    gf_mb();

    intr_info.biu_intr_bits = gf_read32(adapter->mmio + INTR_SHADOW_REG);
    intr_info.csp_intr_bits = gf_read32(adapter->mmio + ADV_SHADOW_REG);
    gf_translate_interrupt_bits(0, &intr_info, &intr_masks);

    return intr_masks;
}

irq_chip_funcs_t irq_chip_funcs = 
{
    .get_intr_enable_mask = gf_get_intr_enable_mask_e3k,
    .set_intr_enable_mask = gf_set_intr_enable_mask_e3k,
    .enable_msi = gf_enable_msi_e3k,
    .disable_msi = gf_disable_msi_e3k,
    .disable_interrupt = gf_disable_interrupt_e3k,
    .enable_interrupt = gf_enable_interrupt_e3k,
    .get_interrupt_mask = gf_get_interrupt_mask_e3k,
};
