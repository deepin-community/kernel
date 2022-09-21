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


/*****************************************************************************
** DESCRIPTION:
** Down scaler hw block interface implementation.
**
** NOTE:
** The functions in this file are hw layer internal functions, 
** CAN ONLY be called by files under Hw folder. 
******************************************************************************/

#include "CBiosChipShare.h"
#include "CBiosScaler.h"
#include "../CBiosHwShare.h"

static CBREGISTER_IDX HorDisplayShiftReg_INDEX[] = {
    {CR_59, 0xFF}, //CR59[7:0]
    {CR_5B, 0x01}, //CR5B[0]
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX VerDisplayShiftReg_INDEX[] = {
    {CR_5A, 0xFF}, //CR59[7:0]
    {CR_5B, 0xF0}, //CR5B[7:4]
    {MAPMASK_EXIT},
};

static WB_SCL_RATIO_TABLE DSCLTable[] = {
    /* 1280x720 ----> 720x480 */
    {
        WORD_CAT(1280,  720),
        WORD_CAT(720,   480),
        WORD_CAT(9,     16),
        WORD_CAT(12,    18),
    },

    /* 1280x720 ----> 720x576 */
    {
        WORD_CAT(1280,  720),
        WORD_CAT(720,   576),
        WORD_CAT(18,    32),
        WORD_CAT(8,     10),
    },

    /* 1920x1080 ----> 720x480 */
    {
        WORD_CAT(1920,  1080),
        WORD_CAT(720,   480),
        WORD_CAT(6,     16),
        WORD_CAT(8,     18),
    },

    /* 1920x1080 ----> 720x576 */
    {
        WORD_CAT(1920,  1080),
        WORD_CAT(720,   576),
        WORD_CAT(9,     24),
        WORD_CAT(8,     15),
    },

    /* 3840x2160 ----> 720x480 */
    {
        WORD_CAT(3840,  2160),
        WORD_CAT(720,   480),
        WORD_CAT(3,     16),
        WORD_CAT(4,     18),
    },

    /* 3840x2160 ----> 720x576 */
    {
        WORD_CAT(3840,  2160),
        WORD_CAT(720,   576),
        WORD_CAT(6,     32),
        WORD_CAT(4,     15),
    },

    /* 3840x2160 ----> 1920x1080 */
    {
        WORD_CAT(3840,  2160),
        WORD_CAT(1920,  1080),
        WORD_CAT(9,     18),
        WORD_CAT(9,     16),
    },

    /* 3840x2160 ----> 1920x1200 */
    {
        WORD_CAT(3840,  2160),
        WORD_CAT(1920,  1200),
        WORD_CAT(16,    32),
        WORD_CAT(5,     9),
    },

    /* 3840x2160 ----> 1280x720 */
    {
        WORD_CAT(3840,  2160),
        WORD_CAT(1280,  720),
        WORD_CAT(10,    30),
        WORD_CAT(3,     9),
    },

    /* 4096x2160 ----> 1920x1080 */
    {
        WORD_CAT(4096,  2160),
        WORD_CAT(1920,  1080),
        WORD_CAT(15,    32),
        WORD_CAT(5,     10),
    },

    /* 4096x2160 ----> 1920x1200 */
    {
        WORD_CAT(4096,  2160),
        WORD_CAT(1920,  1200),
        WORD_CAT(15,    32),
        WORD_CAT(5,     9),
    },

    /* 4096x2160 ----> 2048x1080 */
    {
        WORD_CAT(4096,  2160),
        WORD_CAT(2048,  1080),
        WORD_CAT(16,    32),
        WORD_CAT(5,     10),
    },

    /* 4096x2160 ----> 2048x1200 */
    {
        WORD_CAT(4096,  2160),
        WORD_CAT(2048,  1200),
        WORD_CAT(16,    32),
        WORD_CAT(5,     9),
    },

    /* 4096x2160 ----> 1280x720 */
    {
        WORD_CAT(4096,  2160),
        WORD_CAT(1280,  720),
        WORD_CAT(5,     16),
        WORD_CAT(7,     21),
    },

    /* 1920x1080 ----> 1280x720 */
    {
        WORD_CAT(1920,  1080),
        WORD_CAT(1280,  720),
        WORD_CAT(12,    18),
        WORD_CAT(10,    15),
    },

};

static CBIOS_U32 DSCLTableSize = sizeof(DSCLTable)/sizeof(DSCLTable[0]);

static CBIOS_U32 REG33278INDEX[CBIOS_IGACOUNTS] = {0x33278, 0x33bf4, 0x342f4, 0x349f4};
static CBIOS_U32 REG33290INDEX[CBIOS_IGACOUNTS] = {0x33290, 0x33c1c, 0x3431c, 0x34a1c};
static CBIOS_U32 REG3327cINDEX[CBIOS_IGACOUNTS] = {0x3327c, 0x33bf8, 0x342f8, 0x349f8};
static CBIOS_U32 REG33280INDEX[CBIOS_IGACOUNTS] = {0x33280, 0x33bfc, 0x342fc, 0x349fc};
static CBIOS_U32 REG33284INDEX[CBIOS_IGACOUNTS] = {0x33284, 0x33c00, 0x34300, 0x34a00};
static CBIOS_U32 REG33288INDEX[CBIOS_IGACOUNTS] = {0x33288, 0x33c04, 0x34304, 0x34a04};
static CBIOS_U32 REG33880INDEX[CBIOS_IGACOUNTS] = {0x33880, 0x33c08, 0x34308, 0x34a08};

static CBIOS_U32 WBCSCIndex[CBIOS_IGACOUNTS] = {0x8264, 0x33bdc, 0x342dc, 0x349dc};

static CBIOS_VOID cbDisplayShiftOnOff(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_BOOL bOn, CBIOS_U32 IGAIndex)
{
    REG_CR5B_Pair RegCR5BValue, RegCR5BMask;
    
    if(bOn)  //Enable Display Shift
    {
        RegCR5BValue.Value = 0;
        RegCR5BValue.Enable_Shift_On = 1;
        RegCR5BMask.Value = 0xFF;
        RegCR5BMask.Enable_Shift_On = 0;
        cbBiosMMIOWriteReg(pcbe,CR_5B, RegCR5BValue.Value, RegCR5BMask.Value, IGAIndex);
    }
    else    //Disable Display Shift
    {
        RegCR5BValue.Value = 0;
        RegCR5BValue.Enable_Shift_On = 0;
        RegCR5BMask.Value = 0xFF;
        RegCR5BMask.Enable_Shift_On = 0;
        cbBiosMMIOWriteReg(pcbe,CR_5B, RegCR5BValue.Value, RegCR5BMask.Value, IGAIndex);
    }
}

static CBIOS_VOID cbSetDisplayShiftValue(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 IGAIndex,
                                         CBIOS_U32 HorShiftPos, CBIOS_U32 VerShiftPos)
{
    CBIOS_U32 HorShiftValue = HorShiftPos >> 3;    // character clocks

    cbMapMaskWrite(pcbe, HorShiftValue, HorDisplayShiftReg_INDEX, IGAIndex);
    cbMapMaskWrite(pcbe, VerShiftPos, VerDisplayShiftReg_INDEX, IGAIndex);

}

CBIOS_STATUS cbPanelScalerOnOff(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DISP_MODE_PARAMS pModeParams)
{
    CBIOS_U32 ScalerXRes = pModeParams->ScalerPara.XRes;
    CBIOS_U32 ScalerYRes = pModeParams->ScalerPara.YRes;
    CBIOS_U32 IGAIndex = pModeParams->IGAIndex;

    REG_SR4F  RegSR4FValue = {0}, RegSR4FMask = {0xff};
    REG_SR49  RegSR49Value = {0}, RegSR49Mask = {0xff};
    REG_SR59  RegSR59Value = {0}, RegSR59Mask = {0xff};
    REG_SR5A  RegSR5AValue = {0}, RegSR5AMask = {0xff};
    REG_SR5B  RegSR5BValue = {0}, RegSR5BMask = {0xff};

    if(pModeParams->ScalerStatusToUse != ENABLE_UPSCALER && pModeParams->ScalerStatusToUse != DISABLE_SCALER)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbPanelScalerOnOff: Invalid Scaler Status\n"));
        return CBIOS_ER_INVALID_PARAMETER;
    }

    //set scaler on off
    if(pModeParams->ScalerStatusToUse == DISABLE_SCALER)
    {
        RegSR4FValue.PS1_Upscaler_Enable = 0;
        RegSR4FValue.PS1_Horizontal_upscaling_Enable = 0;
        RegSR4FValue.PS1_Vertical_upscaling_Enable = 0;
        RegSR4FValue.PS1_Upscaler_Auto_Ratio = 0;
        RegSR4FMask.PS1_Upscaler_Enable = 0;
        RegSR4FMask.PS1_Horizontal_upscaling_Enable = 0;
        RegSR4FMask.PS1_Vertical_upscaling_Enable = 0;
        RegSR4FMask.PS1_Upscaler_Auto_Ratio = 0;

        cbBiosMMIOWriteReg(pcbe, SR_4F, RegSR4FValue.Value, RegSR4FMask.Value, IGAIndex);

        return CBIOS_OK;
    }

    RegSR4FValue.PS1_Upscaler_Auto_Ratio = 1;
    RegSR4FValue.PS1_Upscaler_Enable = 1;
    RegSR4FValue.PS1_Horizontal_upscaling_Enable = 1;
    RegSR4FValue.PS1_Vertical_upscaling_Enable = 1;
    RegSR4FMask.PS1_Upscaler_Auto_Ratio = 0;
    RegSR4FMask.PS1_Upscaler_Enable = 0;
    RegSR4FMask.PS1_Horizontal_upscaling_Enable = 0;
    RegSR4FMask.PS1_Vertical_upscaling_Enable = 0;

    //scaler destination width
    RegSR59Value.PS1_Upscaler_Dest_Width_7_0 = ((ScalerXRes - 1) & 0xff);
    RegSR59Mask.PS1_Upscaler_Dest_Width_7_0  = 0;
    RegSR5AValue.PS1_Upscaler_Dest_width = ((ScalerXRes - 1) >> 8 & 0xf);
    RegSR5AMask.PS1_Upscaler_Dest_width  = 0;
    RegSR49Value.PS1_Scalar_Dest_Width_bit12 = ((ScalerXRes - 1) >> 12 & 0x01);
    RegSR49Mask.PS1_Scalar_Dest_Width_bit12  = 0;

    //scaler destination height
    RegSR5BValue.PS1_Upscaler_Dest_Height_7_0 = ((ScalerYRes - 1) & 0xff);
    RegSR5BMask.PS1_Upscaler_Dest_Height_7_0  = 0;
    RegSR5AValue.PS1_Upscaler_Dest_Height = ((ScalerYRes - 1) >> 8 & 0xf);
    RegSR5AMask.PS1_Upscaler_Dest_Height  = 0;

    cbBiosMMIOWriteReg(pcbe, SR_49, RegSR49Value.Value, RegSR49Mask.Value, IGAIndex);
    cbBiosMMIOWriteReg(pcbe, SR_59, RegSR59Value.Value, RegSR59Mask.Value, IGAIndex);
    cbBiosMMIOWriteReg(pcbe, SR_5A, RegSR5AValue.Value, RegSR5AMask.Value, IGAIndex);
    cbBiosMMIOWriteReg(pcbe, SR_5B, RegSR5BValue.Value, RegSR5BMask.Value, IGAIndex);
    cbBiosMMIOWriteReg(pcbe, SR_4F, RegSR4FValue.Value, RegSR4FMask.Value, IGAIndex);

    return CBIOS_OK;
}

CBIOS_STATUS cbSetScaler(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DISP_MODE_PARAMS pModeParams)
{
    if(pcbe->ChipCaps.IsSupportUpScaling)
    {
        cbPanelScalerOnOff(pcbe, pModeParams);
    }

    if(cbIsNeedCentering(pcbe, pModeParams))
    {
        cbEnableCentering(pcbe, pModeParams);
    }
    else
    {
        cbDisableCentering(pcbe, pModeParams->IGAIndex);
    }

    return CBIOS_OK;
}

CBIOS_BOOL cbIsNeedCentering(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DISP_MODE_PARAMS pModeParams)
{
    CBIOS_U32 ScaleXRes, ScaleYRes;
    CBIOS_U32 TargetXRes, TargetYRes;

    ScaleXRes = pModeParams->ScalerPara.XRes;
    ScaleYRes = pModeParams->ScalerPara.YRes;
    TargetXRes = pModeParams->TargetModePara.XRes;
    TargetYRes = pModeParams->TargetModePara.YRes;
    
    if ((TargetXRes > ScaleXRes && TargetYRes >= ScaleYRes)
        || (TargetXRes >= ScaleXRes && TargetYRes > ScaleYRes))
    {
        return CBIOS_TRUE;
    }
    else
    {
        return CBIOS_FALSE;
    }
}

CBIOS_VOID cbEnableCentering(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DISP_MODE_PARAMS pModeParams)
{
    CBIOS_U32 HorShiftPos = 0;
    CBIOS_U32 VerShiftPos = 0;

    HorShiftPos = (pModeParams->TargetModePara.XRes - pModeParams->ScalerPara.XRes) >> 1;
    VerShiftPos = (pModeParams->TargetModePara.YRes - pModeParams->ScalerPara.YRes) >> 1;

    cbSetDisplayShiftValue(pcbe, pModeParams->IGAIndex, HorShiftPos, VerShiftPos);
    cbDisplayShiftOnOff(pcbe, CBIOS_TRUE, pModeParams->IGAIndex);
}

CBIOS_VOID cbDisableCentering(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 IGAIndex)
{
    cbDisplayShiftOnOff(pcbe, CBIOS_FALSE, IGAIndex);
}

static PWB_SCL_RATIO_TABLE cbGetDownScalerRatio(CBIOS_U32 Src, CBIOS_U32 Dst)
{
    CBIOS_U32   i = 0;

    for (i = 0; i < DSCLTableSize; i++)
    {
        if (DSCLTable[i].SrcSize == Src && DSCLTable[i].DstSize == Dst)
        {
            return &DSCLTable[i];
        }
    }

    return CBIOS_NULL;
}

static CBIOS_BOOL cbEnableWBDownscaler(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_WB_PARA pWBPara)
{
    PWB_SCL_RATIO_TABLE pRatio = CBIOS_NULL;
    REG_MM33290_Arise     MM33290Value = {0};
    REG_MM3327C_Arise     MM3327CValue = {0};
    REG_MM33280_Arise     MM33280Value = {0};
    REG_MM33284_Arise     MM33284Value = {0};
    REG_MM33288_Arise     MM33288Value = {0};
    REG_MM33880_Arise     MM33880Value = {0};
    CBIOS_U32           Stride = 0;
    CBIOS_U32           IGAIndex = pWBPara->IGAIndex;

    /* get proper ratio in the table */
    pRatio = cbGetDownScalerRatio(pWBPara->SrcSize, pWBPara->DSCL.DstSize);
    if (!pRatio)
    {
        pWBPara->bNotSupport = CBIOS_TRUE;
        return CBIOS_FALSE;
    }

    /* 1. address */
    MM33290Value.WriteBackBaseAddress = (CBIOS_U32)(pWBPara->DstBaseAddr >> 6);

    /* 2. ratio */
    MM3327CValue.DOWNSCALING_EN = 1;
    //horizontal
    MM3327CValue.HOR_INC = LOW_WORD(pRatio->HRatio) & 0xf;
    MM3327CValue.REG_HOR_INC = (LOW_WORD(pRatio->HRatio) >> 4) & 0x01;
    MM3327CValue.HOR_MODULAR = HIGH_WORD(pRatio->HRatio) & 0x1f;
    MM33280Value.REG_HOR_MODULAR_5 = (HIGH_WORD(pRatio->HRatio) >> 5) & 0x01;
    //vertical
    MM3327CValue.VER_INC = LOW_WORD(pRatio->VRatio) & 0xf;
    MM3327CValue.VER_MODULAR = HIGH_WORD(pRatio->VRatio) & 0xf;
    MM3327CValue.REG_VER_MODULAR = (HIGH_WORD(pRatio->VRatio) >> 4) & 0x01;

    MM3327CValue.SCALING_RCP = (1 << 18) / ( HIGH_WORD(pRatio->HRatio) * HIGH_WORD(pRatio->VRatio) );

    /* 3. format */
    MM3327CValue.DST_FORMAT = 1;
    MM33280Value.REG_DAT_FORMAT = 0;

    /* 4. clip */
    MM33280Value.clip_left_10to0 = 0;
    MM33280Value.clip_left_11 = 0;
    MM33280Value.clip_right_11to0 = (LOW_WORD(pWBPara->SrcSize) - 1) & 0xfff;
    MM33280Value.clip_right_12 = ( (LOW_WORD(pWBPara->SrcSize) - 1) >> 12 ) & 0x01;

    /* 5. p2p or p2i 
     *       P2P double buffer : REG_SCL_MODE_0 = 0, stride = stride*2
     *       P2P single buffer : not support
     *       P2I single buffer : REG_SCL_MODE_0 = 0, stride = stride*2
     *       P2I double buffer : not support
     */
     //512 bits aligned
     Stride = (LOW_WORD(pWBPara->DSCL.DstSize) * 4 + 63) & ~63;
     if (CBIOS_WB_P2P == pWBPara->DSCL.Mode)
     {
        MM33280Value.DOUBLE_FB = 1;
        MM33280Value.REG_SCL_MODE_0 = 0;
        Stride = Stride * 2 / 64;

        MM33288Value.OFFSET = ( ( LOW_WORD(pWBPara->DSCL.DstSize) * HIGH_WORD(pWBPara->DSCL.DstSize) * 4 + 63) & ~63 ) / 64;
     }
     else if (CBIOS_WB_P2I == pWBPara->DSCL.Mode)
     {
        if (pWBPara->DSCL.bDoubleBuffer)
        {
            pWBPara->bNotSupport = CBIOS_TRUE;
            return CBIOS_FALSE;
        }
        else
        {
            MM33280Value.DOUBLE_FB = 0;
            MM33280Value.REG_SCL_MODE_0 = 0;
            Stride = Stride * 2 / 64;
        }
     }
     else
     {
        pWBPara->bNotSupport = CBIOS_TRUE;
        return CBIOS_FALSE;
     }

     MM33288Value.STRIDE = Stride & 0x1ff;
     MM33880Value.REG_STRIDE_11_9 = (Stride >> 9) & 0x7;

     /* 6. reset */
     MM33280Value.FIFO_RST_EN = 1;
     MM33280Value.SCALING_SW_RESET = 1;

     /* 7. enable */
     MM33280Value.REG_SCL_MODE_1 = 1;

     cbMMIOWriteReg32(pcbe, REG33290INDEX[IGAIndex], MM33290Value.Value, 0);
     cbMMIOWriteReg32(pcbe, REG3327cINDEX[IGAIndex], MM3327CValue.Value, 0);
     cbMMIOWriteReg32(pcbe, REG33280INDEX[IGAIndex], MM33280Value.Value, 0);
     cbMMIOWriteReg32(pcbe, REG33284INDEX[IGAIndex], MM33284Value.Value, 0);
     cbMMIOWriteReg32(pcbe, REG33288INDEX[IGAIndex], MM33288Value.Value, 0);
     cbMMIOWriteReg32(pcbe, REG33880INDEX[IGAIndex], MM33880Value.Value, 0);

     return CBIOS_TRUE;
}

static CBIOS_BOOL cbBypassWBDownscaler(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_WB_PARA pWBPara)
{
    REG_MM33290_Arise     MM33290Value = {0}, MM33290RegMask = {0xFFFFFFFF};
    REG_MM3327C_Arise     MM3327CValue = {0}, MM3327CRegMask = {0xFFFFFFFF};
    REG_MM33280_Arise     MM33280Value = {0}, MM33280RegMask = {0xFFFFFFFF};
    REG_MM33284_Arise     MM33284Value = {0}, MM33284RegMask = {0xFFFFFFFF};
    REG_MM33288_Arise     MM33288Value = {0}, MM33288RegMask = {0xFFFFFFFF};
    REG_MM33880_Arise     MM33880Value = {0}, MM33880RegMask = {0xFFFFFFFF};

    CBIOS_U32           Stride = 0;
    CBIOS_U32           IGAIndex = pWBPara->IGAIndex;

    /* 1. address */
    if (pWBPara->bSetAddrOP)
    {
        MM33290Value.WriteBackBaseAddress = (CBIOS_U32)(pWBPara->DstBaseAddr >> 6);

        MM33290RegMask.WriteBackBaseAddress = 0;
    }


    if (pWBPara->bSetModeOP && pWBPara->bEnableOP)
    {
        /* 2. diable downscaler */
        MM3327CValue.DOWNSCALING_EN = 0;
        MM3327CRegMask.DOWNSCALING_EN = 0;

        /* 3. format */
        MM3327CValue.DST_FORMAT = 1;
        MM3327CRegMask.DST_FORMAT = 0;

        MM33280Value.REG_DAT_FORMAT = 0;
        MM33280RegMask.REG_DAT_FORMAT = 0;

        /* 4. clip */
        MM33280Value.clip_left_10to0 = 0;
        MM33280Value.clip_left_11 = 0;
        MM33280Value.clip_right_11to0 = (LOW_WORD(pWBPara->SrcSize) - 1) & 0xfff;
        MM33280Value.clip_right_12 = ( (LOW_WORD(pWBPara->SrcSize) - 1) >> 12 ) & 0x01;

        MM33280RegMask.clip_left_10to0 = 0;
        MM33280RegMask.clip_left_11 = 0;
        MM33280RegMask.clip_right_11to0 = 0;
        MM33280RegMask.clip_right_12 = 0;

        /* 5. strid
         *    512 bits aligned
         */
        Stride = (LOW_WORD(pWBPara->SrcSize) * 4 + 63) & ~63;
        Stride = Stride / 64;

        MM33288Value.STRIDE = Stride & 0x1ff;
        MM33288RegMask.STRIDE = 0;

        MM33880Value.REG_STRIDE_11_9 = (Stride >> 9) & 0x7;
        MM33280Value.DOUBLE_FB = 0;
        MM33280Value.REG_SCL_MODE_0 = 1;

        MM33880RegMask.REG_STRIDE_11_9 = 0;
        MM33280RegMask.DOUBLE_FB = 0;
        MM33280RegMask.REG_SCL_MODE_0 = 0;

        /* 6. reset */
        MM33280Value.FIFO_RST_EN = 1;
        MM33280Value.SCALING_SW_RESET = 1;

        MM33280RegMask.FIFO_RST_EN = 0;
        MM33280RegMask.SCALING_SW_RESET = 0;

        /* 7. enable */
        MM33280Value.REG_SCL_MODE_1 = 1;
        MM33280RegMask.REG_SCL_MODE_1 = 0;

    }


    if (pWBPara->bSetAddrOP)
    {
        cbMMIOWriteReg32(pcbe, REG33290INDEX[IGAIndex], MM33290Value.Value, MM33290RegMask.Value);
    }

    if (pWBPara->bSetModeOP && pWBPara->bEnableOP)
    {
        cbMMIOWriteReg32(pcbe, REG3327cINDEX[IGAIndex], MM3327CValue.Value, MM3327CRegMask.Value);
        cbMMIOWriteReg32(pcbe, REG33280INDEX[IGAIndex], MM33280Value.Value, MM33280RegMask.Value);
        cbMMIOWriteReg32(pcbe, REG33284INDEX[IGAIndex], MM33284Value.Value, MM33284RegMask.Value);
        cbMMIOWriteReg32(pcbe, REG33288INDEX[IGAIndex], MM33288Value.Value, MM33288RegMask.Value);
        cbMMIOWriteReg32(pcbe, REG33880INDEX[IGAIndex], MM33880Value.Value, MM33880RegMask.Value);
    }

    return CBIOS_TRUE;
}

CBIOS_STATUS cbSetWriteback(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_WB_PARA pWBPara)
{
    CBIOS_U32           IGAIndex = pWBPara->IGAIndex;
    REG_MM33278_Arise     MM33278Value = {0}, MM33278Mask = {0xffffffff};
    REG_MM33280_Arise     MM33280Value = {0}, MM33280Mask = {0xffffffff};
    REG_MM8264          MM8264Value = {0}, MM8264Mask = {0xffffffff};
    CBIOS_BOOL          Ret = CBIOS_TRUE;

    if (pWBPara->bEnableOP && pWBPara->bEnable == 0)
    {
        MM33280Value.REG_SCL_MODE_1 = 0;
        MM33280Mask.REG_SCL_MODE_1 = 0;

        MM33278Value.Value = 0;
        MM33278Value.Enable_Work_Register = 1;
        MM33278Mask.Value = 0xffffffff;
        MM33278Mask.Enable_Work_Register = 0;

        cbMMIOWriteReg32(pcbe, REG33280INDEX[IGAIndex], MM33280Value.Value, MM33280Mask.Value);
        cbMMIOWriteReg32(pcbe, REG33278INDEX[IGAIndex], MM33278Value.Value, MM33278Mask.Value);

        return CBIOS_OK;
    }

    if (pWBPara->bUpdateImme)
    {
        MM33278Value.Vsync_Off_Flip = 1;
    }
    else
    {
        MM33278Value.Vsync_Off_Flip = 0;
    }
    MM33278Mask.Vsync_Off_Flip = 0;


    /* configure down scaler */
    if (pWBPara->bByPass)
    {
        Ret = cbBypassWBDownscaler(pcbe, pWBPara);
    }
    else
    {
        Ret = cbEnableWBDownscaler(pcbe, pWBPara);
    }


    if (pWBPara->bSetModeOP)
    {
        /* CSC */
        MM8264Value.CSC_DATA_IN_FMT = CSC_FMT_RGB;
        MM8264Value.CSC_DATA_OUT_FMT = pWBPara->CscOutFmt;
        MM8264Mask.CSC_DATA_IN_FMT = 0;
        MM8264Mask.CSC_DATA_OUT_FMT = 0;
        cbMMIOWriteReg32(pcbe, WBCSCIndex[IGAIndex], MM8264Value.Value, MM8264Mask.Value);
    }

    /* trigger them */
    MM33278Value.Enable_Work_Register = 1;
    MM33278Mask.Value = 0xffffffff;
    MM33278Mask.Enable_Work_Register = 0;

    cbMMIOWriteReg32(pcbe, REG33278INDEX[IGAIndex], MM33278Value.Value, MM33278Mask.Value);

    return (Ret == CBIOS_TRUE) ? CBIOS_OK : CBIOS_ER_INVALID_PARAMETER;
}

