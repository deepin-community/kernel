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
** HDMI hw block interface function implementation.
**
** NOTE:
**
******************************************************************************/

#include "CBiosDIU_HDMI.h"
#include "CBiosDIU_HDTV.h"
#include "CBiosChipShare.h"
#include "../CBiosHwShare.h"
#include "CBiosDIU_HDAC.h"


CBIOS_U32 HDMI_REG_GEN_CTRL[HDMI_MODU_NUM] = {0x8280,  0x33D70,  0x34470,  0x34B70};
CBIOS_U32 HDMI_REG_INFO_FRAME[HDMI_MODU_NUM] = {0x8284,  0x33D74,  0x34474,  0x34B74};
CBIOS_U32 HDMI_REG_AUDIO_CTRL[HDMI_MODU_NUM] = {0x8294,  0x33D78,  0x34478,  0x34B78};
CBIOS_U32 HDMI_REG_AUDIO_CTSN[HDMI_MODU_NUM] = {0x83A8,  0x33D7C,  0x3447C,  0x34B7C};
CBIOS_U32 HDMI_REG_AUDIO_CTS[HDMI_MODU_NUM] = {0x83AC,  0x33D80,  0x34480,  0x34B80};
CBIOS_U32 HDMI_REG_CTRL[HDMI_MODU_NUM] = {0x336B0, 0x33D88,  0x34488,  0x34B88};
CBIOS_U32 HDMI_REG_SCDC_CTRL[HDMI_MODU_NUM] = {0x336B8, 0x33D84,  0x34484,  0x34B84};


CBIOS_VOID cbDIU_HDMI_WriteFIFO(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE DeviceType, CBIOS_U8 FIFOIndex, CBIOS_U8 *pDataBuff, CBIOS_U32 BuffLen)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DEVICE_COMMON    pDevCommon  = cbGetDeviceCommon(&pcbe->DeviceMgr, DeviceType);
    CBIOS_MONITOR_TYPE      MonitorType = pDevCommon->CurrentMonitorType;
    CBIOS_BOOL              bDPDevice = CBIOS_FALSE;
    CBIOS_U8                SR47Value = cbMMIOReadReg(pcbe, SR_47);
    CBIOS_MODULE_INDEX      HDACModuleIndex = cbGetModuleIndex(pcbe, DeviceType, CBIOS_MODULE_TYPE_HDAC);
    CBIOS_U32               i = 0;

    if (HDACModuleIndex >= HDAC_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, WARNING), "%s: invalid HDAC module index!\n", FUNCTION_NAME));
    }

    if ((MonitorType == CBIOS_MONITOR_TYPE_DP) || (MonitorType == CBIOS_MONITOR_TYPE_PANEL))
    {
        bDPDevice = CBIOS_TRUE;
    }

    if (DeviceType == CBIOS_TYPE_DP1)
    {
        //select LUT
        if (bDPDevice)
        {
            cbMMIOWriteReg(pcbe, SR_47, 0x06, 0xF0);
        }
        else
        {
            cbMMIOWriteReg(pcbe, SR_47, 0x04, 0xF0);
        }
    }
    else if (DeviceType == CBIOS_TYPE_DP2)
    {
        //select LUT
        if (bDPDevice)
        {
            cbMMIOWriteReg(pcbe, SR_47, 0x07, 0xF0);
        }
        else
        {
            cbMMIOWriteReg(pcbe, SR_47, 0x05, 0xF0);
        }
    }
    else if (DeviceType == CBIOS_TYPE_DP3)
    {
        //select LUT
        if (bDPDevice)
        {
            cbMMIOWriteReg(pcbe, SR_47, 0x0E, 0xF0);
        }
        else
        {
            cbMMIOWriteReg(pcbe, SR_47, 0x0C, 0xF0);
        }
    }
    else if (DeviceType == CBIOS_TYPE_DP4)
    {
        //select LUT
        if (bDPDevice)
        {
            cbMMIOWriteReg(pcbe, SR_47, 0x0F, 0xF0);
        }
        else
        {
            cbMMIOWriteReg(pcbe, SR_47, 0x0D, 0xF0);
        }
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDMI/DP device!\n", FUNCTION_NAME));
        return;
    }

    cb_WriteU8(pcbe->pAdapterContext, 0x83C8, FIFOIndex);

    for (i = BuffLen; i > 0; i--)
    {
        cb_WriteU8(pcbe->pAdapterContext, 0x83C9, pDataBuff[i - 1]);
    }

    //restore SR47
    cbMMIOWriteReg(pcbe, SR_47, SR47Value, 0x00);
}

CBIOS_VOID cbDIU_HDMI_ReadFIFO(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE DeviceType, CBIOS_U8 FIFOIndex, CBIOS_U8 *pDataBuff, CBIOS_U32 BuffLen)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DEVICE_COMMON    pDevCommon  = cbGetDeviceCommon(&pcbe->DeviceMgr, DeviceType);
    CBIOS_MONITOR_TYPE      MonitorType = pDevCommon->CurrentMonitorType;
    CBIOS_BOOL              bDPDevice = CBIOS_FALSE;
    CBIOS_U8                SR47Value = cbMMIOReadReg(pcbe, SR_47);
    CBIOS_MODULE_INDEX      HDACModuleIndex = cbGetModuleIndex(pcbe, DeviceType, CBIOS_MODULE_TYPE_HDAC);
    CBIOS_U32               i = 0;

    if (HDACModuleIndex >= HDAC_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDAC module index!\n", FUNCTION_NAME));
    }

    if ((MonitorType == CBIOS_MONITOR_TYPE_DP) || (MonitorType == CBIOS_MONITOR_TYPE_PANEL))
    {
        bDPDevice = CBIOS_TRUE;
    }
    
    if (DeviceType == CBIOS_TYPE_DP1)
    {
        //select LUT
        if (bDPDevice)
        {
            cbMMIOWriteReg(pcbe, SR_47, 0x06, 0xF0);
        }
        else
        {
            cbMMIOWriteReg(pcbe, SR_47, 0x04, 0xF0);
        }
    }
    else if (DeviceType == CBIOS_TYPE_DP2)
    {
        //select LUT
        if (bDPDevice)
        {
            cbMMIOWriteReg(pcbe, SR_47, 0x07, 0xF0);
        }
        else
        {
            cbMMIOWriteReg(pcbe, SR_47, 0x05, 0xF0);
        }
    }
    else if (DeviceType == CBIOS_TYPE_DP3)
    {
        //select LUT
        if (bDPDevice)
        {
            cbMMIOWriteReg(pcbe, SR_47, 0x0E, 0xF0);
        }
        else
        {
            cbMMIOWriteReg(pcbe, SR_47, 0x0C, 0xF0);
        }
    }
    else if (DeviceType == CBIOS_TYPE_DP4)
    {
        //select LUT
        if (bDPDevice)
        {
            cbMMIOWriteReg(pcbe, SR_47, 0x0F, 0xF0);
        }
        else
        {
            cbMMIOWriteReg(pcbe, SR_47, 0x0D, 0xF0);
        }
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDMI/DP device!\n", FUNCTION_NAME));
        return;
    }

    cb_WriteU8(pcbe->pAdapterContext, 0x83C7, FIFOIndex);

    for (i = BuffLen; i > 0; i--)
    {
        pDataBuff[i - 1] = cb_ReadU8(pcbe->pAdapterContext, 0x83C9);
    }

    //restore SR47
    cbMMIOWriteReg(pcbe, SR_47, SR47Value, 0x00);
}

CBIOS_VOID cbDIU_HDMI_SetHDCPDelay(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX HDMIModuleIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8280 HDMIGenCtrlRegValue, HDMIGenCtrlRegMask;

    if (HDMIModuleIndex >= HDMI_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDMI module index!\n", FUNCTION_NAME));
        return;
    }

    HDMIGenCtrlRegValue.Value = 0;
    HDMIGenCtrlRegValue.Delay_for_HDCP = HDMI_DELAY_FOR_HDCP - HDMI_LEADING_GUARD_BAND_PERIOD - 
                                         HDMI_PREAMBLE_PERIOD + HDCP_HW_PROCESS_PERIOD;
    HDMIGenCtrlRegValue.Delay_for_HDCP_SEL = 1;

    HDMIGenCtrlRegMask.Value = 0xFFFFFFFF;
    HDMIGenCtrlRegMask.Delay_for_HDCP = 0;
    HDMIGenCtrlRegMask.Delay_for_HDCP_SEL = 0;

    cbMMIOWriteReg32(pcbe, HDMI_REG_GEN_CTRL[HDMIModuleIndex], HDMIGenCtrlRegValue.Value, HDMIGenCtrlRegMask.Value);
}

CBIOS_VOID cbDIU_HDMI_SetHVSync(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX HDMIModuleIndex, CBIOS_U8 HVPolarity)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8280 HDMIGenCtrlRegValue, HDMIGenCtrlRegMask;

    if (HDMIModuleIndex >= HDMI_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDMI module index!\n", FUNCTION_NAME));
        return;
    }

    HDMIGenCtrlRegValue.Value = 0;
    if (HVPolarity & HorNEGATIVE)
    {
        HDMIGenCtrlRegValue.HSYNC_Invert_Enable = 1;
    }
    if (HVPolarity & VerNEGATIVE)
    {
        HDMIGenCtrlRegValue.VSYNC_Invert_Enable = 1;
    }

    HDMIGenCtrlRegMask.Value = 0xFFFFFFFF;
    HDMIGenCtrlRegMask.HSYNC_Invert_Enable = 0;
    HDMIGenCtrlRegMask.VSYNC_Invert_Enable = 0;

    cbMMIOWriteReg32(pcbe, HDMI_REG_GEN_CTRL[HDMIModuleIndex], HDMIGenCtrlRegValue.Value, HDMIGenCtrlRegMask.Value);
}

CBIOS_VOID cbDIU_HDMI_SendInfoFrame(PCBIOS_VOID pvcbe, CBIOS_U32 HDMIMaxPacketNum, CBIOS_ACTIVE_TYPE Device, CBIOS_U8 Length)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32 StartAddress = 0;
    CBIOS_MODULE_INDEX HDMIModuleIndex = CBIOS_MODULE_INDEX_INVALID;
    REG_MM8284 HDMIInfoFrameRegValue;

    HDMIModuleIndex = cbGetModuleIndex(pcbe, Device, CBIOS_MODULE_TYPE_HDMI);
    if (HDMIModuleIndex >= HDMI_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDMI module index!\n", FUNCTION_NAME));
        return;
    }

    HDMIInfoFrameRegValue.Value = 0;
    HDMIInfoFrameRegValue.InfoFrame_FIFO_1_Ready = 1;
    HDMIInfoFrameRegValue.Horiz_Blank_Max_Packets = HDMIMaxPacketNum;
    HDMIInfoFrameRegValue.InfoFrame_FIFO_1_Start_Address = StartAddress;
    HDMIInfoFrameRegValue.InfoFrame_FIFO_1_Length = (Length - 1);
    cb_WriteU32(pcbe->pAdapterContext, HDMI_REG_INFO_FRAME[HDMIModuleIndex], HDMIInfoFrameRegValue.Value);
}

CBIOS_VOID cbDIU_HDMI_SetPixelFormat(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX HDMIModuleIndex, CBIOS_U32 OutputSignal)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8280 HDMIGenCtrlRegValue, HDMIGenCtrlRegMask;

    if (HDMIModuleIndex >= HDMI_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDMI module index!\n", FUNCTION_NAME));
        return;
    }

    HDMIGenCtrlRegValue.Value = 0;
    HDMIGenCtrlRegMask.Value = 0xFFFFFFFF;
    HDMIGenCtrlRegMask.Convert_to_YCbCr422_Enable = 0;
    HDMIGenCtrlRegMask.TMDS_Video_Pixel_Format_Select = 0;
    
    // Make the TMDS's source is RGB TV data or YPbPr
    if(OutputSignal == CBIOS_YCBCR422OUTPUT)  // YCbCr 4:2:2 output
    {
        HDMIGenCtrlRegValue.Convert_to_YCbCr422_Enable = 1;
        HDMIGenCtrlRegValue.TMDS_Video_Pixel_Format_Select = 2;
    }
    else if(OutputSignal == CBIOS_YCBCR444OUTPUT) // YCbCr 4:4:4 output
    {
        HDMIGenCtrlRegValue.Convert_to_YCbCr422_Enable = 0;
        HDMIGenCtrlRegValue.TMDS_Video_Pixel_Format_Select = 1;
    }
    else if(OutputSignal == CBIOS_YCBCR420OUTPUT) // YCbCr 4:2:0 output
    {
        HDMIGenCtrlRegValue.Convert_to_YCbCr422_Enable = 0;
        HDMIGenCtrlRegValue.TMDS_Video_Pixel_Format_Select = 3;
    }
    else // For RGB output
    {
        HDMIGenCtrlRegValue.Convert_to_YCbCr422_Enable = 0;
        HDMIGenCtrlRegValue.TMDS_Video_Pixel_Format_Select = 0;
    }
    cbMMIOWriteReg32(pcbe, HDMI_REG_GEN_CTRL[HDMIModuleIndex], HDMIGenCtrlRegValue.Value, HDMIGenCtrlRegMask.Value);


    if(OutputSignal == CBIOS_YCBCR420OUTPUT)
    {
        cbDIU_HDMI_EnableYCbCr420(pcbe, HDMIModuleIndex, CBIOS_TRUE);
    }
    else
    {
        cbDIU_HDMI_EnableYCbCr420(pcbe, HDMIModuleIndex, CBIOS_FALSE);
    }
}

CBIOS_VOID cbDIU_HDMI_SetColorDepth(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX HDMIModuleIndex, CBIOS_U8 ColorDepth)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_BOOL bNeedSetColorDepth = CBIOS_FALSE;
    REG_MM8280 HDMIGenCtrlRegValue, HDMIGenCtrlRegMask;
    REG_MM8294 HDMIAudioCtrlRegValue, HDMIAudioCtrlRegMask;
    
    if (HDMIModuleIndex >= HDMI_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDMI module index!\n", FUNCTION_NAME));
        return;
    }

    HDMIGenCtrlRegValue.Value = 0;
    HDMIGenCtrlRegMask.Value = 0xFFFFFFFF;
    HDMIGenCtrlRegMask.Deep_Color_Mode = 0;

    HDMIAudioCtrlRegValue.Value = 0;
    HDMIAudioCtrlRegMask.Value = 0xFFFFFFFF;
    HDMIAudioCtrlRegMask.DC_Gen_Cntl_Pkt_EN = 0;
    HDMIAudioCtrlRegMask.PP_SELECT = 0;
    HDMIAudioCtrlRegMask.CD = 0; // color depth
    HDMIAudioCtrlRegMask.Default_Phase = 0;

    if (pcbe->ChipCaps.IsSupportDeepColor)
    {
        bNeedSetColorDepth = CBIOS_TRUE;
    }
    else
    {
        bNeedSetColorDepth = CBIOS_FALSE;
    }

    if (bNeedSetColorDepth)
    {
        if (ColorDepth == 30)
        {
            HDMIGenCtrlRegValue.Deep_Color_Mode = 1;

            HDMIAudioCtrlRegValue.DC_Gen_Cntl_Pkt_EN = 1;
            HDMIAudioCtrlRegValue.PP_SELECT = 0;
            HDMIAudioCtrlRegValue.CD = 5;
            HDMIAudioCtrlRegValue.Default_Phase = 0;
        }
        else if (ColorDepth == 36)
        {
            HDMIGenCtrlRegValue.Deep_Color_Mode = 2;

            HDMIAudioCtrlRegValue.DC_Gen_Cntl_Pkt_EN = 1;
            HDMIAudioCtrlRegValue.PP_SELECT = 0;
            HDMIAudioCtrlRegValue.CD = 6;
            HDMIAudioCtrlRegValue.Default_Phase = 0;
        }
        else//set to 24 bit by default
        {
            HDMIGenCtrlRegValue.Deep_Color_Mode = 0;

            HDMIAudioCtrlRegValue.DC_Gen_Cntl_Pkt_EN = 0;
            HDMIAudioCtrlRegValue.PP_SELECT = 0;
            HDMIAudioCtrlRegValue.CD = 0;
            HDMIAudioCtrlRegValue.Default_Phase = 0;
        }

        cbMMIOWriteReg32(pcbe, HDMI_REG_GEN_CTRL[HDMIModuleIndex], HDMIGenCtrlRegValue.Value, HDMIGenCtrlRegMask.Value);
        cbMMIOWriteReg32(pcbe, HDMI_REG_AUDIO_CTRL[HDMIModuleIndex], HDMIAudioCtrlRegValue.Value, HDMIAudioCtrlRegMask.Value);
    }
}

// set HDMI module mode between HDMI mode and DVI mode
CBIOS_VOID cbDIU_HDMI_SetModuleMode(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX HDMIModuleIndex, CBIOS_BOOL bHDMIMode)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8280 HDMIGenCtrlRegValue, HDMIGenCtrlRegMask;

    if (HDMIModuleIndex >= HDMI_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDMI module index!\n", FUNCTION_NAME));
        return;
    }

    HDMIGenCtrlRegValue.Value = 0;
    HDMIGenCtrlRegMask.Value = 0xFFFFFFFF;
    HDMIGenCtrlRegMask.DVI_Mode_during_HDMI_Enable = 0;

    if (!bHDMIMode)
    {
        HDMIGenCtrlRegValue.DVI_Mode_during_HDMI_Enable = 1; // DVI mode
    }
    else
    {
        HDMIGenCtrlRegValue.DVI_Mode_during_HDMI_Enable = 0; // HDMI mode
    }

    cbMMIOWriteReg32(pcbe, HDMI_REG_GEN_CTRL[HDMIModuleIndex], HDMIGenCtrlRegValue.Value, HDMIGenCtrlRegMask.Value);
}

CBIOS_VOID cbDIU_HDMI_SelectSource(PCBIOS_VOID pvcbe, CBIOS_MODULE *pHDMIModule, CBIOS_MODULE *pNextModule)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
       
    if ((pHDMIModule == CBIOS_NULL) || (pNextModule == CBIOS_NULL))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 2nd param or 3rd param is NULL!\n", FUNCTION_NAME));
        return;
    }

    if ((pcbe->ChipID == CHIPID_E3K) || (pcbe->ChipID == CHIPID_ARISE1020))
    {
        if (pNextModule->Type == CBIOS_MODULE_TYPE_HDTV)
        {
            cbDIU_HDTV_LBBypass(pcbe, pHDMIModule->Index, CBIOS_FALSE);
        }
        else if (pNextModule->Type == CBIOS_MODULE_TYPE_IGA)
        {
            cbDIU_HDTV_LBBypass(pcbe, pHDMIModule->Index, CBIOS_TRUE);
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DP, ERROR),"%s: cannot select HDMI source.\n", FUNCTION_NAME));
        }
    }
}

CBIOS_VOID cbDIU_HDMI_ModuleOnOff(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX HDMIModuleIndex, CBIOS_BOOL bTurnOn)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8280 HDMIGenCtrlRegValue, HDMIGenCtrlRegMask;

    if (HDMIModuleIndex >= HDMI_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDMI module index!\n", FUNCTION_NAME));
        return;
    }

    HDMIGenCtrlRegValue.Value = 0;
    HDMIGenCtrlRegMask.Value = 0xFFFFFFFF;
    HDMIGenCtrlRegMask.HDMI_Reset= 0;

    if (bTurnOn)
    {
        HDMIGenCtrlRegValue.HDMI_Reset= 1;
    }
    cbMMIOWriteReg32(pcbe, HDMI_REG_GEN_CTRL[HDMIModuleIndex], HDMIGenCtrlRegValue.Value, HDMIGenCtrlRegMask.Value);

    cb_DelayMicroSeconds(1);

    HDMIGenCtrlRegValue.Value = 0;
    HDMIGenCtrlRegMask.Value = 0xFFFFFFFF;
    HDMIGenCtrlRegMask.HDMI_Reset= 0;
    
    HDMIGenCtrlRegValue.HDMI_Reset= 0;
    cbMMIOWriteReg32(pcbe, HDMI_REG_GEN_CTRL[HDMIModuleIndex], HDMIGenCtrlRegValue.Value, HDMIGenCtrlRegMask.Value);

    HDMIGenCtrlRegValue.Value = 0;
    HDMIGenCtrlRegMask.Value = 0xFFFFFFFF;
    HDMIGenCtrlRegMask.HDMI_Enable = 0;
    
    if (bTurnOn)
    {
        HDMIGenCtrlRegValue.HDMI_Enable = 1;
    }
    else
    {
        HDMIGenCtrlRegValue.HDMI_Enable = 0;
    }
    cbMMIOWriteReg32(pcbe, HDMI_REG_GEN_CTRL[HDMIModuleIndex], HDMIGenCtrlRegValue.Value, HDMIGenCtrlRegMask.Value);
}

CBIOS_VOID cbDIU_HDMI_DisableVideoAudio(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX HDMIModuleIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8294  HDMIAudioCtrlReg, HDMIAudioCtrlMask;

    if (HDMIModuleIndex >= HDMI_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDMI module index!\n", FUNCTION_NAME));
        return;
    }

    //disable video and audio
    HDMIAudioCtrlReg.Value = 0;
    HDMIAudioCtrlReg.Set_AVMUTE_Enable = 1;
    HDMIAudioCtrlReg.Clear_AVMUTE_Enable = 0;

    HDMIAudioCtrlMask.Value = 0xFFFFFFFF;
    HDMIAudioCtrlMask.Set_AVMUTE_Enable = 0;
    HDMIAudioCtrlMask.Clear_AVMUTE_Enable =0;
    cbMMIOWriteReg32(pcbe, HDMI_REG_AUDIO_CTRL[HDMIModuleIndex], HDMIAudioCtrlReg.Value, HDMIAudioCtrlMask.Value);
}

CBIOS_VOID cbDIU_HDMI_EnableVideoAudio(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX HDMIModuleIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8294  HDMIAudioCtrlReg, HDMIAudioCtrlMask;

    if (HDMIModuleIndex >= HDMI_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDMI module index!\n", FUNCTION_NAME));
        return;
    }

    //disable video and audio
    HDMIAudioCtrlReg.Value = 0;
    HDMIAudioCtrlReg.Set_AVMUTE_Enable = 0;
    HDMIAudioCtrlReg.Clear_AVMUTE_Enable = 1;

    HDMIAudioCtrlMask.Value = 0xFFFFFFFF;
    HDMIAudioCtrlMask.Set_AVMUTE_Enable = 0;
    HDMIAudioCtrlMask.Clear_AVMUTE_Enable =0;
    cbMMIOWriteReg32(pcbe, HDMI_REG_AUDIO_CTRL[HDMIModuleIndex], HDMIAudioCtrlReg.Value, HDMIAudioCtrlMask.Value);
}

CBIOS_VOID cbDIU_HDMI_SetCTSN(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX HDMIModuleIndex, CBIOS_MODULE_INDEX HDACModuleIndex, CBIOS_U32 StreamFormat)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32   N = 0;
    CBIOS_BOOL  bCloseDummyAudio = CBIOS_FALSE;
    REG_MM8294  HDMIAudioCtrlRegValue, HDMIAudioCtrlRegMask;
    REG_MM829C  HDACPacket2RegValue, HDACPacket2RegMask;
    REG_MM82AC  HDACChStatusCtrlRegValue, HDACChStatusCtrlRegMask;
    REG_MM83A8  HDACCtsNRegValue, HDACCtsNRegMask; 
    REG_MM83AC  HDACCtsRegValue, HDACCtsRegMask;
    REG_MM336B4 RegMM336B4Value, RegMM336B4Mask;
    REG_MM336B0 RegMM336B0Value, RegMM336B0Mask;

    if (HDMIModuleIndex >= HDMI_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDMI module index!\n", FUNCTION_NAME));
        return;
    }

    if (HDACModuleIndex >= HDAC_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDAC module index!\n", FUNCTION_NAME));
        return;
    }
    // 1. Audio enable
    HDMIAudioCtrlRegValue.Value = 0;
    HDMIAudioCtrlRegValue.HDMI_Audio_Enable = 1;
    HDMIAudioCtrlRegMask.Value = 0xFFFFFFFF;
    HDMIAudioCtrlRegMask.HDMI_Audio_Enable = 0;
    cbMMIOWriteReg32(pcbe, HDMI_REG_AUDIO_CTRL[HDMIModuleIndex], HDMIAudioCtrlRegValue.Value, HDMIAudioCtrlRegMask.Value);

    // 2. Audio Source select
    // DP1/DP2/MHL Dual Mode
    HDMIAudioCtrlRegValue.Value = 0;
    HDMIAudioCtrlRegValue.Select_HDMI_Audio_Source = (HDMIModuleIndex == CBIOS_MODULE_INDEX1)? 0 : 1;
    HDMIAudioCtrlRegMask.Value = 0xFFFFFFFF;
    HDMIAudioCtrlRegMask.Select_HDMI_Audio_Source = 0;
    cbMMIOWriteReg32(pcbe, HDMI_REG_AUDIO_CTRL[HDMIModuleIndex], HDMIAudioCtrlRegValue.Value, HDMIAudioCtrlRegMask.Value);

    // 3. Set CTS/N
    switch(StreamFormat)
    {
    case 192000:
        N = 24576;
        break;
    case 176400:
        N = 25088;
        break;
    case 96000:
        N = 12288;
        break;
    case 88200:
        N = 12544;
        break;
    case 48000:
        N = 6144;
        break;
    case 44100:
        N = 6272;
        break;
    case 32000:
        N = 4096;
        break;
    default:
        N = 6144;
        break;
    }

    HDACCtsNRegValue.Value = 0;
    HDACCtsNRegValue.N = N;
    HDACCtsNRegMask.Value = 0xFFFFFFFF;
    HDACCtsNRegMask.N = 0;
    cbMMIOWriteReg32(pcbe, HDMI_REG_AUDIO_CTSN[HDMIModuleIndex], HDACCtsNRegValue.Value, HDACCtsNRegMask.Value);

    // set hw CTS
    HDACCtsRegValue.Value = 0;
    HDACCtsRegValue.CTS_Select = 0;
    HDACCtsRegMask.Value = 0xFFFFFFFF;
    HDACCtsRegMask.CTS_Select = 0;
    cbMMIOWriteReg32(pcbe, HDMI_REG_AUDIO_CTS[HDMIModuleIndex], HDACCtsRegValue.Value, HDACCtsRegMask.Value);
    
    // 4. Set hw ACR ratio & ACR enable [28]
    HDACPacket2RegValue.Value = 0;
    HDACPacket2RegValue.CODEC1_ACR_ENABLE = 1;
    HDACPacket2RegMask.Value = 0xFFFFFFFF;
    HDACPacket2RegMask.CODEC1_ACR_ENABLE = 0;
    cbMMIOWriteReg32(pcbe, HDAC_REG_PACKET2[HDACModuleIndex], HDACPacket2RegValue.Value, HDACPacket2RegMask.Value);

    HDMIAudioCtrlRegValue.Value = 0;
    HDMIAudioCtrlRegValue.ACR_ratio_select = 1;
    HDMIAudioCtrlRegMask.Value = 0xFFFFFFFF;
    HDMIAudioCtrlRegMask.ACR_ratio_select = 0;
    cbMMIOWriteReg32(pcbe, HDMI_REG_AUDIO_CTRL[HDMIModuleIndex], HDMIAudioCtrlRegValue.Value, HDMIAudioCtrlRegMask.Value);

    if((pcbe->ChipID == CHIPID_E3K) || (pcbe->ChipID == CHIPID_ARISE1020)) //for 480p to play 192k audio
    {
        HDACChStatusCtrlRegValue.Value = 0;
        HDACChStatusCtrlRegValue.multiple_sample = 1;
        HDACChStatusCtrlRegMask.Value = 0xFFFFFFFF;
        HDACChStatusCtrlRegMask.multiple_sample = 0;
        cbMMIOWriteReg32(pcbe, HDAC_REG_CHSTATUS_CTRL[HDACModuleIndex], HDACChStatusCtrlRegValue.Value, HDACChStatusCtrlRegMask.Value);

        RegMM336B0Value.Value = 0;
        RegMM336B0Value.PKTLEG_SEL_DISABLE = 0;
        RegMM336B0Value.PKTLEG_VBLANK = 0XFF;
        RegMM336B0Mask.Value = 0xFFFFFFFF;
        RegMM336B0Mask.PKTLEG_SEL_DISABLE = 0;
        RegMM336B0Mask.PKTLEG_VBLANK = 0;
        cbMMIOWriteReg32(pcbe, HDMI_REG_CTRL[HDMIModuleIndex], RegMM336B0Value.Value, RegMM336B0Mask.Value);
    }

    // Short Audio patch
    if(bCloseDummyAudio)
    {
        HDACChStatusCtrlRegValue.Value = 0;
        HDACChStatusCtrlRegValue.Always_Output_Audio = 0;
        HDACChStatusCtrlRegMask.Value = 0xFFFFFFFF;
        HDACChStatusCtrlRegMask.Always_Output_Audio = 0;
        cbMMIOWriteReg32(pcbe, HDAC_REG_CHSTATUS_CTRL[HDACModuleIndex], HDACChStatusCtrlRegValue.Value, HDACChStatusCtrlRegMask.Value);
    }
}


CBIOS_VOID cbDIU_HDMI_ConfigScrambling(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX HDMIModuleIndex, CBIOS_BOOL bEnableScrambling)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM336B0 HDMICtrlRegValue, HDMICtrlRegMask;

    if (HDMIModuleIndex >= HDMI_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDMI module index!\n", FUNCTION_NAME));
        return;
    }

    HDMICtrlRegValue.Value = 0;
    HDMICtrlRegValue.HDMI1_SCRAMBLE_EN = (bEnableScrambling)? 1 : 0;
    HDMICtrlRegMask.Value = 0xFFFFFFFF;
    HDMICtrlRegMask.HDMI1_SCRAMBLE_EN = 0;

    cbMMIOWriteReg32(pcbe, HDMI_REG_CTRL[HDMIModuleIndex], HDMICtrlRegValue.Value, HDMICtrlRegMask.Value);
}

CBIOS_VOID cbDIU_HDMI_EnableReadRequest(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX HDMIModuleIndex, CBIOS_BOOL bEnableRR)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM336B8 HDMIScdcCtrlRegValue, HDMIScdcCtrlRegMask;

    if (HDMIModuleIndex >= HDMI_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDMI module index!\n", FUNCTION_NAME));
        return;
    }
    
	
    HDMIScdcCtrlRegValue.Value = 0;
    HDMIScdcCtrlRegMask.Value = 0xFFFFFFFF;
    HDMIScdcCtrlRegMask.HDMI1_SCDC_RR_ENABLE = 0;
    HDMIScdcCtrlRegMask.HDMI1_SCDC_HW_DRV_START_ENABLE = 0;
    HDMIScdcCtrlRegMask.HDMI1_SCDC_HW_DRV_STOP_ENABLE = 0;
    HDMIScdcCtrlRegMask.HDMI1_SCDC_START_STOP_ENABLE = 0;

    if(bEnableRR)
    {
        HDMIScdcCtrlRegValue.HDMI1_SCDC_RR_ENABLE = 0;
        HDMIScdcCtrlRegValue.HDMI1_SCDC_HW_DRV_START_ENABLE = 0;
        HDMIScdcCtrlRegValue.HDMI1_SCDC_HW_DRV_STOP_ENABLE = 0;
        HDMIScdcCtrlRegValue.HDMI1_SCDC_START_STOP_ENABLE = 1;

        cbMMIOWriteReg32(pcbe, HDMI_REG_SCDC_CTRL[HDMIModuleIndex], HDMIScdcCtrlRegValue.Value, HDMIScdcCtrlRegMask.Value);
        cbDebugPrint((MAKE_LEVEL(HDMI, DEBUG), "%s: Enable Source Read Request!\n", FUNCTION_NAME));
    }
    else
    {
        HDMIScdcCtrlRegValue.HDMI1_SCDC_RR_ENABLE = 0;
        HDMIScdcCtrlRegValue.HDMI1_SCDC_HW_DRV_START_ENABLE = 0;
        HDMIScdcCtrlRegValue.HDMI1_SCDC_HW_DRV_STOP_ENABLE = 0;
        HDMIScdcCtrlRegValue.HDMI1_SCDC_START_STOP_ENABLE = 0;        

        cbMMIOWriteReg32(pcbe, HDMI_REG_SCDC_CTRL[HDMIModuleIndex], HDMIScdcCtrlRegValue.Value, HDMIScdcCtrlRegMask.Value);
        cbDebugPrint((MAKE_LEVEL(HDMI, DEBUG), "%s: Disable Source Read Request!\n", FUNCTION_NAME));
    }
}

CBIOS_VOID cbDIU_HDMI_EnableYCbCr420(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX HDMIModuleIndex, CBIOS_BOOL bEnable420)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM336B0 HDMICtrlRegValue, HDMICtrlRegMask;

    if (HDMIModuleIndex >= HDMI_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDMI module index!\n", FUNCTION_NAME));
        return;
    }

    HDMICtrlRegValue.Value = 0;
    HDMICtrlRegValue.HDMI1_YC_420_EN = (bEnable420)? 1 : 0;
    HDMICtrlRegValue.HDMI1_YC_420_MODE = (bEnable420)? 1 : 0;
    HDMICtrlRegMask.Value = 0xFFFFFFFF;
    HDMICtrlRegMask.HDMI1_YC_420_EN = 0;
    HDMICtrlRegMask.HDMI1_YC_420_MODE = 0;
    cbMMIOWriteReg32(pcbe, HDMI_REG_CTRL[HDMIModuleIndex], HDMICtrlRegValue.Value, HDMICtrlRegMask.Value);
}

CBIOS_VOID cbDIU_HDMI_EnableClkLane(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX HDMIModuleIndex, CBIOS_BOOL bEnableClkLane)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM336B0 HDMICtrlRegValue, HDMICtrlRegMask;

    if (HDMIModuleIndex >= HDMI_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDMI module index!\n", FUNCTION_NAME));
        return;
    }

    HDMICtrlRegValue.Value = 0;
    HDMICtrlRegValue.HDMI1_CLK_LANE_EN = (bEnableClkLane)? 1 : 0;
    HDMICtrlRegMask.Value = 0xFFFFFFFF;
    HDMICtrlRegMask.HDMI1_CLK_LANE_EN = 0;
    cbMMIOWriteReg32(pcbe, HDMI_REG_CTRL[HDMIModuleIndex], HDMICtrlRegValue.Value, HDMICtrlRegMask.Value);
}


