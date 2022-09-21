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
** HDTV hw block interface function implementation.
**
** NOTE:
**
******************************************************************************/

#include "CBiosDIU_HDTV.h"
#include "CBiosChipShare.h"
#include "../CBiosHwShare.h"

CBIOS_U32 HDTV_REG_LB[HDTV_MODU_NUM] = {0x33694,  0x33C34,  0x34334,  0x34A34};
CBIOS_U32 HDTV_REG_CTRL[HDTV_MODU_NUM] = {0x338B8,  0x33E88,  0x34598,  0x34C98};
CBIOS_U32 HDTV_REG_SYNC_DELAY[HDTV_MODU_NUM] = {0x338C0,  0x33E90,  0x345A0,  0x34CA0};
CBIOS_U32 HDTV_REG_LEFT_BLANK[HDTV_MODU_NUM] = {0x338CC,  0x33E9C,  0x345AC,  0x34CAC};
CBIOS_U32 HDTV_REG_HSYNC[HDTV_MODU_NUM] = {0x338D0,  0x33EA0,  0x345B0,  0x34CB0};
CBIOS_U32 HDTV_REG_BROAD_PULSE[HDTV_MODU_NUM] = {0x338D4,  0x33EA4,  0x345B4,  0x34CB4};
CBIOS_U32 HDTV_REG_HALF_SYNC[HDTV_MODU_NUM] = {0x338D8,  0x33EA8,  0x345B8,  0x34CB8};
CBIOS_U32 HDTV_REG_HDE[HDTV_MODU_NUM] = {0x338E4,  0x33EB4,  0x345C4,  0x34CC4};
CBIOS_U32 HDTV_REG_ENABLE[HDTV_MODU_NUM] = {0x338FC,  0x33ECC,  0x345DC,  0x34CDC};


CBIOS_VOID cbDIU_HDTV_ModuleOnOff(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX HDTVModuleIndex, CBIOS_BOOL bTurnOn)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_VOID)pvcbe;
    REG_MM338B8   HDTVCtrlRegValue, HDTVCtrlRegMask;
    REG_MM338FC   HDTVEnableRegValue, HDTVEnableRegMask;

    if (HDTVModuleIndex >= HDTV_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid HDTV module index!\n", FUNCTION_NAME));
        return;
    }

    if (bTurnOn)
    {
        HDTVCtrlRegValue.Value = 0;
        HDTVCtrlRegValue.HDTV_Timing_Enable_Control = 1;
        HDTVCtrlRegMask.Value = 0xFFFFFFFF;
        HDTVCtrlRegMask.HDTV_Timing_Enable_Control = 0;
        cbMMIOWriteReg32(pcbe, HDTV_REG_CTRL[HDTVModuleIndex], HDTVCtrlRegValue.Value, HDTVCtrlRegMask.Value);

        HDTVEnableRegValue.Value = 0;
        HDTVEnableRegValue.HDTV_Enable = 1;
        HDTVEnableRegMask.Value = 0xFFFFFFFF;
        HDTVEnableRegMask.HDTV_Enable = 0;
        cbMMIOWriteReg32(pcbe, HDTV_REG_ENABLE[HDTVModuleIndex], HDTVEnableRegValue.Value, HDTVEnableRegMask.Value);
    }
    else
    {
        HDTVEnableRegValue.Value = 0;
        HDTVEnableRegValue.HDTV_Enable = 0;
        HDTVEnableRegMask.Value = 0xFFFFFFFF;
        HDTVEnableRegMask.HDTV_Enable = 0;
        cbMMIOWriteReg32(pcbe, HDTV_REG_ENABLE[HDTVModuleIndex], HDTVEnableRegValue.Value, HDTVEnableRegMask.Value);

        HDTVCtrlRegValue.Value = 0;
        HDTVCtrlRegValue.HDTV_Timing_Enable_Control = 0;
        HDTVCtrlRegMask.Value = 0xFFFFFFFF;
        HDTVCtrlRegMask.HDTV_Timing_Enable_Control = 0;
        cbMMIOWriteReg32(pcbe, HDTV_REG_CTRL[HDTVModuleIndex], HDTVCtrlRegValue.Value, HDTVCtrlRegMask.Value);
    }
}

CBIOS_VOID cbDIU_HDTV_LBBypass(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX HDTVModuleIndex, CBIOS_BOOL isBypass)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_VOID)pvcbe;
    REG_MM33694  HDTVLineBufRegValue, HDTVLineBufRegMask;

    if (HDTVModuleIndex >= HDTV_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid HDTV module index!\n", FUNCTION_NAME));
        return;
    }

    HDTVLineBufRegValue.Value = 0;
    HDTVLineBufRegValue.LB1_BYPASS = (isBypass)? 1 : 0;
    HDTVLineBufRegMask.Value = 0xFFFFFFFF;
    HDTVLineBufRegMask.LB1_BYPASS = 0;
    cbMMIOWriteReg32(pcbe, HDTV_REG_LB[HDTVModuleIndex], HDTVLineBufRegValue.Value, HDTVLineBufRegMask.Value);
}


CBIOS_VOID cbDoHDTVFuncSetting_Arise(PCBIOS_VOID pvcbe, 
                             PCBIOS_DISP_MODE_PARAMS pModeParams,
                             CBIOS_U32 IGAIndex,
                             CBIOS_ACTIVE_TYPE ulDevices)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_VOID)pvcbe;
    CBIOS_U32   HSyncWidth, VSyncWidth, HSyncToHActive, BlankLevel;
    CBIOS_U32   HSyncDelay = 0;
    CBIOS_U32   HorizontalDisplayEnd, HorizontalTotal;
    REG_MM338B8 HDTVCtrlRegValue, HDTVCtrlRegMask;
    REG_MM338C0 HDTVSyncDelayRegValue, HDTVSyncDelayRegMask;
    REG_MM338CC HDTVLeftBlankRegValue, HDTVLeftBlankRegMask;
    REG_MM338D0 HDTVHsyncRegValue, HDTVHsyncRegMask;
    REG_MM338D4 HDTVBroadPulseRegValue, HDTVBroadPulseRegMask;
    REG_MM338D8 HDTVHalfSyncRegValue, HDTVHalfSyncRegMask;
    REG_MM338E4 HDTVHdeRegValue, HDTVHdeRegMask;
    REG_MM338FC HDTVEnableRegValue, HDTVEnableRegMask;
    PCBIOS_TIMING_ATTRIB pTimingReg = &pModeParams->TargetTiming;
    CBIOS_MODULE_INDEX HDTVModuleIndex = IGAIndex;

    if (HDTVModuleIndex >= HDTV_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid HDTV module index!\n", FUNCTION_NAME));
        return;
    }
    
    // always not compatible with ITU470
    HDTVCtrlRegValue.Value = 0;
    HDTVCtrlRegValue.ITU470_SELECT = 0;
    HDTVCtrlRegMask.Value = 0xFFFFFFFF;
    HDTVCtrlRegMask.ITU470_SELECT = 0;
    cbMMIOWriteReg32(pcbe, HDTV_REG_CTRL[HDTVModuleIndex], HDTVCtrlRegValue.Value, HDTVCtrlRegMask.Value);

    HSyncWidth           = (pTimingReg->HorSyncEnd - pTimingReg->HorSyncStart);
    HorizontalDisplayEnd = (CBIOS_U32)pTimingReg->HorDisEnd;
    HorizontalTotal      = (CBIOS_U32)pTimingReg->HorTotal;
    HSyncToHActive       = (pTimingReg->HorTotal - pTimingReg->HorSyncStart);
    VSyncWidth           = (pTimingReg->VerSyncEnd - pTimingReg->VerSyncStart);
    
    if(pTimingReg->XRes == 720)
    {
        HSyncDelay = 05;
    }
    else
    {
        HSyncDelay = 0;
    }

    if ((pModeParams->TargetModePara.bInterlace)
        && (pTimingReg->YRes == 576))
    {
        BlankLevel = 126;
    }
    else
    {
        BlankLevel = 252;
    }
    
    // HDTV Tri-Level SYNC Width, HDTV BLANK Level
    HDTVCtrlRegValue.Value = 0;
    HDTVCtrlRegValue.Trilevel_Sync_Width = HSyncWidth - 6;
    HDTVCtrlRegValue.Blank_Level = BlankLevel;
    HDTVCtrlRegMask.Value = 0xFFFFFFFF;
    HDTVCtrlRegMask.Trilevel_Sync_Width = 0;
    HDTVCtrlRegMask.Blank_Level = 0;
    cbMMIOWriteReg32(pcbe, HDTV_REG_CTRL[HDTVModuleIndex], HDTVCtrlRegValue.Value, HDTVCtrlRegMask.Value);
        
    // HDTV SYNC Delay
    HDTVSyncDelayRegValue.Value = 0;
    HDTVSyncDelayRegMask.Value = 0xFFFFFFFF;
    HDTVSyncDelayRegMask.HDTV_SYNC_Delay_7to0 = 0;
    HDTVSyncDelayRegMask.HDTV_SYNC_Delay_10to8 = 0;
    if (pModeParams->TargetModePara.bInterlace)
    {
        HDTVSyncDelayRegValue.HDTV_SYNC_Delay_7to0 = (HorizontalTotal/2) & 0xFF;
        HDTVSyncDelayRegValue.HDTV_SYNC_Delay_10to8 = ((HorizontalTotal/2) >> 8 ) & 0x07;
    }
    else
    {   
        HDTVSyncDelayRegValue.HDTV_SYNC_Delay_7to0 = 0;
        HDTVSyncDelayRegValue.HDTV_SYNC_Delay_10to8 = 0;
    }
    cbMMIOWriteReg32(pcbe, HDTV_REG_SYNC_DELAY[HDTVModuleIndex], HDTVSyncDelayRegValue.Value, HDTVSyncDelayRegMask.Value);

    // HDTV Left Blank Pixels, this value of HDTV and HDMI are different.
    HDTVLeftBlankRegValue.Value = 0;
    HDTVLeftBlankRegValue.Left_Blank_Pixels_7to0 = (HSyncToHActive - 1) & 0xFF;
    HDTVLeftBlankRegValue.Left_Blank_Pixels_10to8 = ((HSyncToHActive - 1) >> 8 ) & 0x07;
    HDTVLeftBlankRegMask.Value = 0xFFFFFFFF;
    HDTVLeftBlankRegMask.Left_Blank_Pixels_7to0 = 0;
    HDTVLeftBlankRegMask.Left_Blank_Pixels_10to8 = 0;
    cbMMIOWriteReg32(pcbe, HDTV_REG_LEFT_BLANK[HDTVModuleIndex], HDTVLeftBlankRegValue.Value, HDTVLeftBlankRegMask.Value);

    // HDTV Digital HSYNC Width and HDTV HSYNC Delay
    HDTVHsyncRegValue.Value = 0;
    HDTVHsyncRegValue.HDTV_Digital_HSYNC_Width = (HSyncWidth - 1);
    HDTVHsyncRegValue.HDTV_HSYNC_Delay = HSyncDelay;
    HDTVHsyncRegMask.Value = 0xFFFFFFFF;
    HDTVHsyncRegMask.HDTV_Digital_HSYNC_Width = 0;
    HDTVHsyncRegMask.HDTV_HSYNC_Delay = 0;
    cbMMIOWriteReg32(pcbe, HDTV_REG_HSYNC[HDTVModuleIndex], HDTVHsyncRegValue.Value, HDTVHsyncRegMask.Value);

    // HDTV Broad Pulse
    HDTVBroadPulseRegValue.Value = 0;
    HDTVBroadPulseRegMask.Value = 0xFFFFFFFF;
    HDTVBroadPulseRegMask.Hdtv_Broad_Pulse_6to0 = 0;
    HDTVHalfSyncRegValue.Value = 0;
    HDTVHalfSyncRegMask.Value = 0xFFFFFFFF;
    HDTVHalfSyncRegMask.Hdtv_Broad_Pulse_14to7 = 0;
    
    if (pModeParams->TargetModePara.bInterlace)
    {
          HDTVBroadPulseRegValue.Hdtv_Broad_Pulse_6to0 = (HorizontalTotal*VSyncWidth/2) & 0x7F;
          HDTVHalfSyncRegValue.Hdtv_Broad_Pulse_14to7 = ((HorizontalTotal*VSyncWidth/2) >> 7) & 0xFF;
    }
    else
    {
        HDTVBroadPulseRegValue.Hdtv_Broad_Pulse_6to0 = (HorizontalTotal*VSyncWidth) & 0x7F;
        HDTVHalfSyncRegValue.Hdtv_Broad_Pulse_14to7 = ((HorizontalTotal*VSyncWidth) >> 7) & 0xFF;
    }
    cbMMIOWriteReg32(pcbe, HDTV_REG_BROAD_PULSE[HDTVModuleIndex], HDTVBroadPulseRegValue.Value, HDTVBroadPulseRegMask.Value);
    cbMMIOWriteReg32(pcbe, HDTV_REG_HALF_SYNC[HDTVModuleIndex], HDTVHalfSyncRegValue.Value, HDTVHalfSyncRegMask.Value);

    // HDTV Half SYNC, only applies to SMPTE274M 1080i
    if ((pModeParams->TargetModePara.bInterlace)
        && (pModeParams->TargetModePara.XRes == 1920)
        && (pModeParams->TargetModePara.YRes == 1080))
    {
        HDTVHalfSyncRegValue.Value = 0;
        HDTVHalfSyncRegValue.Hdtv_Half_Sync_10to0 = (HorizontalTotal/2);
        HDTVHalfSyncRegMask.Value = 0xFFFFFFFF;
        HDTVHalfSyncRegMask.Hdtv_Half_Sync_10to0 = 0;
        cbMMIOWriteReg32(pcbe, HDTV_REG_HALF_SYNC[HDTVModuleIndex], HDTVHalfSyncRegValue.Value, HDTVHalfSyncRegMask.Value);
    }

    // HDTV HDE
    HDTVHdeRegValue.Value = 0;
    HDTVHdeRegValue.HDTV_HDE_10to0 = (HorizontalDisplayEnd-1);
    HDTVHdeRegMask.Value = 0xFFFFFFFF;
    HDTVHdeRegMask.HDTV_HDE_10to0 = 0;
    cbMMIOWriteReg32(pcbe, HDTV_REG_HDE[HDTVModuleIndex], HDTVHdeRegValue.Value, HDTVHdeRegMask.Value);

    //Disable first in case SRB_8F.HDTV_Enable already set somewhere,which will causing timing incorrect
    //and the final result is display show abnormal, like image sperated into three part.
    HDTVEnableRegValue.Value = 0;
    HDTVEnableRegValue.HDTV_Enable = 0;
    HDTVEnableRegMask.Value = 0xFFFFFFFF;
    HDTVEnableRegMask.HDTV_Enable = 0;
    cbMMIOWriteReg32(pcbe, HDTV_REG_ENABLE[HDTVModuleIndex], HDTVEnableRegValue.Value, HDTVEnableRegMask.Value);

    if (pModeParams->TargetModePara.bInterlace)
    {
        HDTVCtrlRegValue.Value = 0;
        HDTVCtrlRegValue.Progressive_Mode_Enable = 0;
        HDTVCtrlRegMask.Value = 0xFFFFFFFF;
        HDTVCtrlRegMask.Progressive_Mode_Enable = 0;
        cbMMIOWriteReg32(pcbe, HDTV_REG_CTRL[HDTVModuleIndex], HDTVCtrlRegValue.Value, HDTVCtrlRegMask.Value);

        if (((pTimingReg->XRes == 720) &&(pTimingReg->YRes == 576)) || ((pTimingReg->XRes == 720) &&(pTimingReg->YRes == 480)))
        {
            HDTVCtrlRegValue.Value = 0;
            HDTVCtrlRegValue._576i_480i_enable = 1;
            HDTVCtrlRegMask.Value = 0xFFFFFFFF;
            HDTVCtrlRegMask._576i_480i_enable = 0;
            cbMMIOWriteReg32(pcbe, HDTV_REG_CTRL[HDTVModuleIndex], HDTVCtrlRegValue.Value, HDTVCtrlRegMask.Value);
        }
        else if ((pTimingReg->XRes == 1920) &&(pTimingReg->YRes == 1080))
        {
            HDTVCtrlRegValue.Value = 0;
            HDTVCtrlRegValue.SMPTE_274M_Enable = 1;
            HDTVCtrlRegMask.Value = 0xFFFFFFFF;
            HDTVCtrlRegMask.SMPTE_274M_Enable = 0;
            cbMMIOWriteReg32(pcbe, HDTV_REG_CTRL[HDTVModuleIndex], HDTVCtrlRegValue.Value, HDTVCtrlRegMask.Value);
        }
    }
    else
    {
        HDTVCtrlRegValue.Value = 0;
        HDTVCtrlRegValue.Progressive_Mode_Enable = 1;
        HDTVCtrlRegMask.Value = 0xFFFFFFFF;
        HDTVCtrlRegMask.Progressive_Mode_Enable = 0;
        cbMMIOWriteReg32(pcbe, HDTV_REG_CTRL[HDTVModuleIndex], HDTVCtrlRegValue.Value, HDTVCtrlRegMask.Value);
    }
}
