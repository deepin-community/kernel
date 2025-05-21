/*
 * Copyright © 2021 Glenfly Tech Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#include "CBiosDIU_DP.h"
#include "CBiosChipShare.h"
#include "../CBiosHwShare.h"

#define MEASURE_LT_TIME_GPIO    0

CBIOS_U32 DP_REG_MISC1[DP_MODU_NUM] = {0x8210,  0x33CA8,  0x343A8,  0x34AA8};
CBIOS_U32 DP_REG_LINK[DP_MODU_NUM] = {0x8214,  0x33CAC,  0x343AC,  0x34AAC};
CBIOS_U32 DP_REG_GEN_CTRL[DP_MODU_NUM] = {0x8218,  0x33CB0,  0x343B0,  0x34AB0};
CBIOS_U32 DP_REG_EXT_PACKET[DP_MODU_NUM] = {0x821C,  0x33CB4,  0x343B4,  0x34AB4};
CBIOS_U32 DP_REG_ENABLE[DP_MODU_NUM] = {0x8240,  0x33CB8,  0x343B8,  0x34AB8};
CBIOS_U32 DP_REG_HWIDTH_TU[DP_MODU_NUM] = {0x8244,  0x33CBC,  0x343BC,  0x34ABC};
CBIOS_U32 DP_REG_HLINE_DUR[DP_MODU_NUM] = {0x8248,  0x33CC0,  0x343C0,  0x34AC0};
CBIOS_U32 DP_REG_MISC0[DP_MODU_NUM] = {0x824C,  0x33CC4,  0x343C4,  0x34AC4};
CBIOS_U32 DP_REG_HV_TOTAL[DP_MODU_NUM] = {0x8250,  0x33CC8,  0x343C8,  0x34AC8};
CBIOS_U32 DP_REG_HV_START[DP_MODU_NUM] = {0x8254,  0x33CCC,  0x343CC,  0x34ACC};
CBIOS_U32 DP_REG_HV_SYNC[DP_MODU_NUM] = {0x8258,  0x33CD0,  0x343D0,  0x34AD0};
CBIOS_U32 DP_REG_HV_ACTIVE[DP_MODU_NUM] = {0x825C,  0x33CD4,  0x343D4,  0x34AD4};
CBIOS_U32 DP_REG_EPHY_CTRL[DP_MODU_NUM] = {0x82CC,  0x33CD8,  0x343D8,  0x34AD8};
CBIOS_U32 DP_REG_AUX_WRITE0[DP_MODU_NUM] = {0x8310,  0x33CDC,  0x343DC,  0x34ADC};
CBIOS_U32 DP_REG_AUX_WRITE1[DP_MODU_NUM] = {0x8314,  0x33CE0,  0x343E0,  0x34AE0};
CBIOS_U32 DP_REG_AUX_WRITE2[DP_MODU_NUM] = {0x8318,  0x33CE4,  0x343E4,  0x34AE4};
CBIOS_U32 DP_REG_AUX_WRITE3[DP_MODU_NUM] = {0x831C,  0x33CE8,  0x343E8,  0x34AE8};
CBIOS_U32 DP_REG_AUX_READ0[DP_MODU_NUM] = {0x8320,  0x33CEC,  0x343EC,  0x34AEC};
CBIOS_U32 DP_REG_AUX_READ1[DP_MODU_NUM] = {0x8324,  0x33CF0,  0x343F0,  0x34AF0};
CBIOS_U32 DP_REG_AUX_READ2[DP_MODU_NUM] = {0x8328,  0x33CF4,  0x343F4,  0x34AF4};
CBIOS_U32 DP_REG_AUX_READ3[DP_MODU_NUM] = {0x832C,  0x33CF8,  0x343F8,  0x34AF8};
CBIOS_U32 DP_REG_AUX_TIMER[DP_MODU_NUM] = {0x8330,  0x33CFC,  0x343FC,  0x34AFC};
CBIOS_U32 DP_REG_AUX_CMD[DP_MODU_NUM] = {0x8334,  0x33D00,  0x34400,  0x34B00};
CBIOS_U32 DP_REG_MUTE[DP_MODU_NUM] = {0x8338,  0x33D04,  0x34404,  0x34B04};
CBIOS_U32 DP_REG_MAUD[DP_MODU_NUM] = {0x833C,  0x33D08,  0x34408,  0x34B08};
CBIOS_U32 DP_REG_EPHY_MPLL[DP_MODU_NUM] = {0x8340,  0x33D0C,  0x3440C,  0x34B0C};
CBIOS_U32 DP_REG_EPHY_TX[DP_MODU_NUM] = {0x8344,  0x33D10,  0x34410,  0x34B10};
CBIOS_U32 DP_REG_EPHY_MISC[DP_MODU_NUM] = {0x8348,  0x33D14,  0x34414,  0x34B14};
CBIOS_U32 DP_REG_SWING[DP_MODU_NUM] = {0x8368,  0x33D28,  0x34428,  0x34B28};
CBIOS_U32 DP_REG_EPHY_STATUS[DP_MODU_NUM] = {0x836C,  0x33D2C,  0x3442C,  0x34B2C};
CBIOS_U32 DP_REG_LINK_CTRL[DP_MODU_NUM] = {0x334C8, 0x33D3C,  0x3443C,  0x34B3C};
CBIOS_U32 DP_REG_CTRL2[DP_MODU_NUM] = {0x334CC, 0x33D40,  0x34440,  0x34B40};
CBIOS_U32 DP_REG_CTRL[DP_MODU_NUM] = {0x334DC, 0x33D50,  0x34450,  0x34B50};
CBIOS_U32 DP_REG_EPHY_SETTING1[DP_MODU_NUM] = {0x334E0, 0x33D54,  0x34454,  0x34B54};
CBIOS_U32 DP_REG_EPHY_SETTING2[DP_MODU_NUM] = {0x334E4, 0x33D58,  0x34458,  0x34B58};

CBIOS_VOID cbDIU_EDP_ControlVDDSignal(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_BOOL status)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_SR3C_Pair RegSR3CValue, RegSR3CMask;
    REG_SR35_Pair RegSR35Value, RegSR35Mask;

    if(status) //ON
    {
        if(((DPModuleIndex == CBIOS_MODULE_INDEX1) && (pDPMonitorContext->GpioForEDP1Power== 0x0f))
            || ((DPModuleIndex == CBIOS_MODULE_INDEX2) && (pDPMonitorContext->GpioForEDP2Power== 0x0f)))  //invalid
        {
            RegSR3CValue.Value = 0;
            RegSR3CValue.ENVDD = 0;
            RegSR3CMask.Value = 0xFF;
            RegSR3CMask.ENVDD = 0;
            cbMMIOWriteReg(pcbe, SR_3C, RegSR3CValue.Value, RegSR3CMask.Value);
            cbDelayMilliSeconds(2);
        }
        else if(((DPModuleIndex == CBIOS_MODULE_INDEX1) && (pDPMonitorContext->GpioForEDP1Power== 0))
            || ((DPModuleIndex == CBIOS_MODULE_INDEX2) && (pDPMonitorContext->GpioForEDP2Power== 0)))
        {
            RegSR35Value.Value = 0;
            RegSR35Value.GPIO0_Select = 0x1;
            RegSR35Value.GPIO0_Data = 1;
            RegSR35Mask.Value = 0xFF;
            RegSR35Mask.GPIO0_Select = 0;
            RegSR35Mask.GPIO0_Data = 0;
            cbMMIOWriteReg(pcbe, SR_35, RegSR35Value.Value, RegSR35Mask.Value);
        }
        else
        {
            //TODO: GPIO control power sequence
            cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: invalid power gpio index!\n", FUNCTION_NAME));
        }
    }
    else    //OFF
    {
        if(((DPModuleIndex == CBIOS_MODULE_INDEX1) && (pDPMonitorContext->GpioForEDP1Power== 0x0f))
            || ((DPModuleIndex == CBIOS_MODULE_INDEX2) && (pDPMonitorContext->GpioForEDP2Power== 0x0f)))  //invalid
        {
            //cbDelayMilliSeconds(16);
            RegSR3CValue.Value = 0;
            RegSR3CValue.ENVDD = 1;
            RegSR3CMask.Value = 0xFF;
            RegSR3CMask.ENVDD = 0;
            cbMMIOWriteReg(pcbe, SR_3C, RegSR3CValue.Value, RegSR3CMask.Value);
            cbDelayMilliSeconds(500);
        }
        else if(((DPModuleIndex == CBIOS_MODULE_INDEX1) && (pDPMonitorContext->GpioForEDP1Power== 0))
            || ((DPModuleIndex == CBIOS_MODULE_INDEX2) && (pDPMonitorContext->GpioForEDP2Power== 0)))
        {
            RegSR35Value.Value = 0;
            RegSR35Value.GPIO0_Select = 0x1;
            RegSR35Value.GPIO0_Data = 0;
            RegSR35Mask.Value = 0xFF;
            RegSR35Mask.GPIO0_Select = 0;
            RegSR35Mask.GPIO0_Data = 0;
            cbMMIOWriteReg(pcbe, SR_35, RegSR35Value.Value, RegSR35Mask.Value);
            cbDelayMilliSeconds(500);
        }
        else
        {
            //TODO: GPIO control power sequence
            cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: invalid power gpio index!\n", FUNCTION_NAME));
        }
    }
}

CBIOS_VOID cbDIU_EDP_ControlVEESignal(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_BOOL status)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_SR3C_Pair   RegSR3CValue, RegSR3CMask;
    REG_SR35_Pair   RegSR35Value, RegSR35Mask;

    if(status) //ON
    {
        if(((DPModuleIndex == CBIOS_MODULE_INDEX1) && (pDPMonitorContext->GpioForEDP1BackLight== 0x0f))
            || ((DPModuleIndex == CBIOS_MODULE_INDEX2) && (pDPMonitorContext->GpioForEDP2BackLight== 0x0f)))  //invalid
        {
            //cbDelayMilliSeconds(256);
            RegSR3CValue.Value = 0;
            RegSR3CValue.ENVEE = 0;
            RegSR3CMask.Value = 0xFF;
            RegSR3CMask.ENVEE = 0;
            cbMMIOWriteReg(pcbe,SR_3C, RegSR3CValue.Value, RegSR3CMask.Value);
        }
        else if(((DPModuleIndex == CBIOS_MODULE_INDEX1) && (pDPMonitorContext->GpioForEDP1BackLight== 1))
            || ((DPModuleIndex == CBIOS_MODULE_INDEX2) && (pDPMonitorContext->GpioForEDP2BackLight== 1)))
        {
            RegSR35Value.Value = 0;
            RegSR35Value.GPIO0_Select = 0x1;
            RegSR35Value.GPIO0_Data = 1;
            RegSR35Mask.Value = 0xFF;
            RegSR35Mask.GPIO0_Select = 0;
            RegSR35Mask.GPIO0_Data = 0;
            cbMMIOWriteReg(pcbe, SR_B_35, RegSR35Value.Value, RegSR35Mask.Value);
        }
        else
        {
            //TODO: GPIO control power sequence
            cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: invalid backlignt gpio index!\n", FUNCTION_NAME));
        }
    }
    else    //OFF
    {
        if(((DPModuleIndex == CBIOS_MODULE_INDEX1) && (pDPMonitorContext->GpioForEDP1BackLight== 0x0f))
            || ((DPModuleIndex == CBIOS_MODULE_INDEX2) && (pDPMonitorContext->GpioForEDP2BackLight== 0x0f)))  //invalid
        {
            RegSR3CValue.Value = 0;
            RegSR3CValue.ENVEE = 1;
            RegSR3CMask.Value = 0xFF;
            RegSR3CMask.ENVEE = 0;
            cbMMIOWriteReg(pcbe,SR_3C, RegSR3CValue.Value, RegSR3CMask.Value);
            //cbDelayMilliSeconds(256);
        }
        else if(((DPModuleIndex == CBIOS_MODULE_INDEX1) && (pDPMonitorContext->GpioForEDP1BackLight== 1))
            || ((DPModuleIndex == CBIOS_MODULE_INDEX2) && (pDPMonitorContext->GpioForEDP2BackLight== 1)))
        {
            RegSR35Value.Value = 0;
            RegSR35Value.GPIO0_Select = 0x1;
            RegSR35Value.GPIO0_Data = 0;
            RegSR35Mask.Value = 0xFF;
            RegSR35Mask.GPIO0_Select = 0;
            RegSR35Mask.GPIO0_Data = 0;
            cbMMIOWriteReg(pcbe, SR_B_35, RegSR35Value.Value, RegSR35Mask.Value);
        }
        else
        {
            //TODO: GPIO control power sequence
            cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: invalid backlignt gpio index!\n", FUNCTION_NAME));
        }
    }
}

CBIOS_BOOL cbDIU_EDP_WaitforSinkHPDSignal(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_U16 timeout)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U16               counter = 0;
    REG_MM8330              DPAuxTimerRegValue;

    if(DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "Invalid module index in %s\n", FUNCTION_NAME));
        return  CBIOS_FALSE;
    }

    while (counter <= timeout)
    {
        //Per Sean, if mmio 8330[31:30] shows hot plugin, means a rising edge of hpd signal
        //After VDD on, HPD signal will be a rising edge, and vdd off will be falling edge
        DPAuxTimerRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_TIMER[DPModuleIndex]);
        if (DPAuxTimerRegValue.HPD_Status == 0x01)
        {
            return CBIOS_TRUE;
        }
        cbDelayMilliSeconds(1);  //query HPD status every 1ms
        counter += 1;
    }

    return CBIOS_FALSE;
}

CBIOS_VOID cbDIU_DP_DPModeEnable(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_BOOL bEnable)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8240 DPEnableRegValue, DPEnableRegMask;

    if(DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "Invalid module index in %s\n", FUNCTION_NAME));
        return;
    }

    cbTraceEnter(DP);

    DPEnableRegValue.Value = 0;
    DPEnableRegValue.DP_Enable = (bEnable)? 1 : 0;
    DPEnableRegMask.Value = 0xFFFFFFFF;
    DPEnableRegMask.DP_Enable = 0;

    cbMMIOWriteReg32(pcbe, DP_REG_ENABLE[DPModuleIndex], DPEnableRegValue.Value, DPEnableRegMask.Value);

    cbTraceExit(DP);
}

CBIOS_VOID cbDIU_DP_SetInterruptMode(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_BOOL isDPMode)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM82CC DPEphyCtrlRegValue, DPEphyCtrlRegMask;

    if(DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "Invalid module index in %s\n", FUNCTION_NAME));
        return;
    }

    DPEphyCtrlRegValue.Value = 0;
    DPEphyCtrlRegValue.int_mode = (isDPMode)? 0 : 1;
    DPEphyCtrlRegMask.Value = 0xFFFFFFFF;
    DPEphyCtrlRegMask.int_mode = 0;

    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_CTRL[DPModuleIndex], DPEphyCtrlRegValue.Value, DPEphyCtrlRegMask.Value);
}

CBIOS_VOID cbDIU_DP_SetHVSync(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_U8 HVPolarity)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8258 DPPolarityRegValue, DPPolarityRegMask;

    if(DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "Invalid module index in %s\n", FUNCTION_NAME));
        return;
    }

    DPPolarityRegValue.Value = 0;
    if (HVPolarity & HorNEGATIVE)
    {
        DPPolarityRegValue.HSYNC_Polarity = 1;
    }
    if (HVPolarity & VerNEGATIVE)
    {
        DPPolarityRegValue.VSYNC_Polarity = 1;
    }

    DPPolarityRegMask.Value = 0xFFFFFFFF;
    DPPolarityRegMask.HSYNC_Polarity = 0;
    DPPolarityRegMask.VSYNC_Polarity = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_HV_SYNC[DPModuleIndex], DPPolarityRegValue.Value, DPPolarityRegMask.Value);
}

CBIOS_VOID cbDIU_DP_SetMaudNaud(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_U32 LinkSpeed, CBIOS_U32 StreamFormat)
{
    CBIOS_U64 Naud = 0, Maud = 0;
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8240 DPEnableRegValue, DPEnableRegMask;
    REG_MM8338 DPMuteRegValue, DPMuteRegMask;
    REG_MM833C DPMaudRegValue, DPMaudRegMask;
    REG_MM8218 DPCodecSelRegValue, DPCodecSelRegMask;

    if(DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "Invalid module index in %s\n", FUNCTION_NAME));
        return;
    }

    // 1. Audio Enable
    DPMuteRegValue.Value = 0;
    DPMuteRegValue.Audio_Output_Enable = 1;
    DPMuteRegMask.Value = 0xFFFFFFFF;
    DPMuteRegMask.Audio_Output_Enable = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_MUTE[DPModuleIndex], DPMuteRegValue.Value, DPMuteRegMask.Value);

    DPEnableRegValue.Value = 0;
    DPEnableRegValue.Enable_Audio = 1;
    DPEnableRegMask.Value = 0xFFFFFFFF;
    DPEnableRegMask.Enable_Audio = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_ENABLE[DPModuleIndex], DPEnableRegValue.Value, DPEnableRegMask.Value);

    DPCodecSelRegValue.Value = 0;
    DPCodecSelRegMask.Value = 0xFFFFFFFF;
    DPCodecSelRegMask.Audio_Strm_Select = 0;
    if(DPModuleIndex == CBIOS_MODULE_INDEX1)
    {
        DPCodecSelRegValue.Audio_Strm_Select = 0;
    }
    else if(DPModuleIndex == CBIOS_MODULE_INDEX2)
    {
        DPCodecSelRegValue.Audio_Strm_Select = 1;
    }
    cbMMIOWriteReg32(pcbe, DP_REG_GEN_CTRL[DPModuleIndex], DPCodecSelRegValue.Value, DPCodecSelRegMask.Value);

    // 2. Calculate Naud Maud
    Naud = 0x8000; // set Naud = 32768
#if 1 //DP SW MAUD
    Maud = cb_do_div((Naud * 512 * StreamFormat), (LinkSpeed * 100));

    DPMaudRegValue.Value = 0;
    DPMaudRegValue.MAUD = (CBIOS_U32)Maud;
    DPMaudRegMask.Value = 0xFFFFFFFF;
    DPMaudRegMask.MAUD = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_MAUD[DPModuleIndex], DPMaudRegValue.Value, DPMaudRegMask.Value);
#endif
    DPMuteRegValue.Value = 0;
    DPMuteRegValue.NAUD = (CBIOS_U32)Naud;
#if 1 //DP SW MAUD
    DPMuteRegValue.Generate_MAUD = 0; // use SW MAUD
    DPMuteRegValue.Generated_MAUD_Mode = 0; // use HW MAUD mode 0
#else
    DPMuteRegValue.Generate_MAUD = 1; // use HW MAUD
    DPMuteRegValue.Generated_MAUD_Mode = 1; // use HW MAUD
#endif
    DPMuteRegMask.Value = 0xFFFFFFFF;
    DPMuteRegMask.NAUD = 0;
    DPMuteRegMask.Generate_MAUD = 0;
    DPMuteRegMask.Generated_MAUD_Mode = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_MUTE[DPModuleIndex], DPMuteRegValue.Value, DPMuteRegMask.Value);
}

CBIOS_VOID cbDIU_DP_ResetLinkTraining(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8214    DPLinkRegValue, DPLinkRegMask;

    cbTraceEnter(DP);

    if(DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "Invalid module index in %s\n", FUNCTION_NAME));
        return;
    }

    // reset Link Training
    DPLinkRegValue.Value = 0;
    DPLinkRegValue.SW_Set_Link_Train_Fail = 1;
    DPLinkRegMask.Value = 0xFFFFFFFF;
    DPLinkRegMask.SW_Set_Link_Train_Fail = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_LINK[DPModuleIndex], DPLinkRegValue.Value, DPLinkRegMask.Value);

    DPLinkRegValue.Value = 0;
    DPLinkRegValue.SW_Set_Link_Train_Fail = 0;
    DPLinkRegMask.Value = 0xFFFFFFFF;
    DPLinkRegMask.SW_Set_Link_Train_Fail = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_LINK[DPModuleIndex], DPLinkRegValue.Value, DPLinkRegMask.Value);

    DPLinkRegValue.Value = 0;
    DPLinkRegValue.SW_Link_Train_State = 0;
    DPLinkRegValue.SW_Set_Link_Train_Fail = 0;
    DPLinkRegMask.Value = 0xFFFFFFFF;
    DPLinkRegMask.SW_Link_Train_State = 0;
    DPLinkRegMask.SW_Set_Link_Train_Fail = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_LINK[DPModuleIndex], DPLinkRegValue.Value, DPLinkRegMask.Value);

    cbTraceExit(DP);
}

CBIOS_VOID cbDIU_DP_VideoOnOff(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_BOOL bOn)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8240 DPEnableRegValue, DPEnableRegMask;

    cbTraceEnter(DP);

    if(DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "Invalid module index in %s\n", FUNCTION_NAME));
        return;
    }

    DPEnableRegValue.Value = 0;
    DPEnableRegMask.Value = 0xFFFFFFFF;
    DPEnableRegMask.Video_Enable = 0;

    if (bOn)
    {
        DPEnableRegValue.Video_Enable = 1;
        cbMMIOWriteReg32(pcbe, DP_REG_ENABLE[DPModuleIndex], DPEnableRegValue.Value, DPEnableRegMask.Value);
    }
    else
    {
        DPEnableRegValue.Video_Enable = 0;
        cbWaitVBlank(pcbe, DPModuleIndex);
        cbMMIOWriteReg32(pcbe, DP_REG_ENABLE[DPModuleIndex], DPEnableRegValue.Value, DPEnableRegMask.Value);
    }
}

//We should disable both video and audio during link training.
//Disable video : MM8240[3] = 0
//Mute by DP:  MM8338[27-26] = 3; MM8240[24] = 1
CBIOS_VOID cbDIU_DP_DisableVideoAudio(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8240 DPEnableRegValue, DPEnableRegMask;
    REG_MM8338 DPMuteRegValue, DPMuteRegMask;

    if(DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "Invalid module index in %s\n", FUNCTION_NAME));
        return;
    }

    //disable video and audio
    DPMuteRegValue.Value = 0;
    DPMuteRegValue.Audio_Output_Enable = 0;
    DPMuteRegValue.Mute = 1;
    DPMuteRegValue.Mute_Mode =1;
    DPMuteRegMask.Value = 0xFFFFFFFF;
    DPMuteRegMask.Audio_Output_Enable = 0;
    DPMuteRegMask.Mute = 0;
    DPMuteRegMask.Mute_Mode =0;
    cbMMIOWriteReg32(pcbe, DP_REG_MUTE[DPModuleIndex], DPMuteRegValue.Value, DPMuteRegMask.Value);

    DPEnableRegValue.Value = 0;
    DPEnableRegValue.Enable_Audio = 0;
    DPEnableRegValue.Video_Enable = 0;
    DPEnableRegMask.Value = 0xFFFFFFFF;
    DPEnableRegMask.Enable_Audio = 0;
    DPEnableRegMask.Video_Enable = 0;
    cbWaitVBlank(pcbe, DPModuleIndex);
    cbMMIOWriteReg32(pcbe, DP_REG_ENABLE[DPModuleIndex], DPEnableRegValue.Value, DPEnableRegMask.Value);
}

CBIOS_VOID cbDIU_DP_EnableVideoAudio(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8240 DPEnableRegValue, DPEnableRegMask;
    REG_MM8338 DPMuteRegValue, DPMuteRegMask;

    if(DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "Invalid module index in %s\n", FUNCTION_NAME));
        return;
    }

    DPMuteRegValue.Value = 0;
    DPMuteRegValue.Audio_Output_Enable = 1;
    DPMuteRegValue.Mute = 0;
    DPMuteRegValue.Mute_Mode = 1;
    DPMuteRegMask.Value = 0xFFFFFFFF;
    DPMuteRegMask.Audio_Output_Enable = 0;
    DPMuteRegMask.Mute = 0;
    DPMuteRegMask.Mute_Mode = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_MUTE[DPModuleIndex], DPMuteRegValue.Value, DPMuteRegMask.Value);

    DPEnableRegValue.Value = 0;
    DPEnableRegValue.Enable_Audio = 1;
    DPEnableRegValue.Video_Enable = 1;
    DPEnableRegMask.Value = 0xFFFFFFFF;
    DPEnableRegMask.Video_Enable = 0;
    DPEnableRegMask.Enable_Audio = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_ENABLE[DPModuleIndex], DPEnableRegValue.Value, DPEnableRegMask.Value);
}

/***************************************************************************************/
/*******************************  Link Training Interfaces  ****************************/
// Reset Aux, PISO, enable Scramble
static CBIOS_VOID cbDIU_DP_LinkTrainingPreset(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_BOOL bEnhancedMode)
{
    REG_MM8218 DPGenCtrlRegValue, DPGenCtrlRegMask;
    REG_MM8240 DPEnableRegValue, DPEnableRegMask;
    REG_MM8344 DPEphyTxRegValue, DPEphyTxRegMask;

    cbTraceEnter(DP);

    if(DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "Invalid module index in %s\n", FUNCTION_NAME));
        return;
    }

    // Reset Aux
    cbDIU_DP_ResetAUX(pcbe, DPModuleIndex);

    // Reset EPHY PISO in case of speed change
    DPEphyTxRegValue.Value = 0;
    DPEphyTxRegValue.TX_Power_Control_Lane0 = 1;
    DPEphyTxRegValue.TX_Power_Control_Lane1 = 1;
    DPEphyTxRegValue.TX_Power_Control_Lane2 = 1;
    DPEphyTxRegValue.TX_Power_Control_Lane3 = 1;
    DPEphyTxRegMask.Value = 0xFFFFFFFF;
    DPEphyTxRegMask.TX_Power_Control_Lane0 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane1 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane2 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane3 = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);

    cb_DelayMicroSeconds(1);

    DPEphyTxRegValue.Value = 0;
    DPEphyTxRegValue.TX_Power_Control_Lane0 = 0;
    DPEphyTxRegValue.TX_Power_Control_Lane1 = 0;
    DPEphyTxRegValue.TX_Power_Control_Lane2 = 0;
    DPEphyTxRegValue.TX_Power_Control_Lane3 = 0;
    DPEphyTxRegMask.Value = 0xFFFFFFFF;
    DPEphyTxRegMask.TX_Power_Control_Lane0 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane1 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane2 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane3 = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);

    // enable scramble
    DPGenCtrlRegValue.Value = 0;
    DPGenCtrlRegValue.Scramble_enable = 1;
    DPGenCtrlRegValue.Switch_Idle_mode_to_video = 0;
    DPGenCtrlRegValue.Idle_Pattern_Counter = 0;
    DPGenCtrlRegValue.AUX_Length = 0;
    DPGenCtrlRegValue.HW_Link_Train_Fail = 0;
    DPGenCtrlRegMask.Value = 0xFFFFFFFF;
    DPGenCtrlRegMask.Scramble_enable = 0;
    DPGenCtrlRegMask.Switch_Idle_mode_to_video = 0;
    DPGenCtrlRegMask.Idle_Pattern_Counter = 0;
    DPGenCtrlRegMask.AUX_Length = 0;
    DPGenCtrlRegMask.HW_Link_Train_Fail = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_GEN_CTRL[DPModuleIndex], DPGenCtrlRegValue.Value, DPGenCtrlRegMask.Value);

    DPEnableRegValue.Value = 0;
    DPEnableRegValue.DP_Enable = 1;
    DPEnableRegValue.Field_Invert = 0;
    DPEnableRegValue.InfoFrame_FIFO_1_Ready = 0;
    DPEnableRegValue.INFOFRAME_FIFO_2_READY = 0;
    DPEnableRegValue.InfoFrame_FIFO_Select = 0;
    DPEnableRegValue.InfoFrame_FIFO_1_Start_Address = 0;
    DPEnableRegValue.InfoFrame_FIFO_2_Start_Address = 0;
    DPEnableRegValue.InfoFrame_FIFO_1_Length = 0;
    DPEnableRegValue.InfoFrame_FIFO_2_Length = 0;
    DPEnableRegValue.Ext_Packet_Enable = 0;
    DPEnableRegValue.header_of_audio_info_frame_is_from_HDAudio_codec = 0;
    DPEnableRegValue.Main_Link_Status = 0;

    DPEnableRegMask.Value = 0xFFFFFFFF;
    DPEnableRegMask.DP_Enable = 0;
    DPEnableRegMask.Field_Invert = 0;
    DPEnableRegMask.Enhanced_Framing_Mode = 0;
    DPEnableRegMask.InfoFrame_FIFO_1_Ready = 0;
    DPEnableRegMask.INFOFRAME_FIFO_2_READY = 0;
    DPEnableRegMask.InfoFrame_FIFO_Select = 0;
    DPEnableRegMask.InfoFrame_FIFO_1_Start_Address = 0;
    DPEnableRegMask.InfoFrame_FIFO_2_Start_Address = 0;
    DPEnableRegMask.InfoFrame_FIFO_1_Length = 0;
    DPEnableRegMask.InfoFrame_FIFO_2_Length = 0;
    DPEnableRegMask.Ext_Packet_Enable = 0;
    DPEnableRegMask.header_of_audio_info_frame_is_from_HDAudio_codec = 0;
    DPEnableRegMask.Main_Link_Status = 0;

    if (bEnhancedMode)
    {
        DPEnableRegValue.Enhanced_Framing_Mode = 1;
    }
    else
    {
        DPEnableRegValue.Enhanced_Framing_Mode = 0;
    }
    cbMMIOWriteReg32(pcbe, DP_REG_ENABLE[DPModuleIndex], DPEnableRegValue.Value, DPEnableRegMask.Value);

    cbTraceExit(DP);
}

static CBIOS_VOID cbDIU_DP_SetDPVersion(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_U32 DPVersion)
{
    REG_MM334CC         DPCtrl2RegValue, DPCtrl2RegMask;

    if(DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "Invalid module index in %s\n", FUNCTION_NAME));
        return;
    }

    DPCtrl2RegValue.Value = 0;
    DPCtrl2RegValue.DP_VERSION_VALUE = DPVersion;
    DPCtrl2RegMask.Value = 0xFFFFFFFF;
    DPCtrl2RegMask.DP_VERSION_VALUE = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_CTRL2[DPModuleIndex], DPCtrl2RegValue.Value, DPCtrl2RegMask.Value);
}

CBIOS_BOOL cbDIU_DP_LinkTrainingHw(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, PCBIOS_LINK_TRAINING_PARAMS pLinkTrainingParams)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32               Device = CBIOS_TYPE_NONE;
    PCBIOS_DEVICE_COMMON    pDevCommon = CBIOS_NULL;
    PCBIOS_DP_CONTEXT       pDpContext = CBIOS_NULL;
    CBIOS_BOOL  bStatus = CBIOS_FALSE;
    CBIOS_U32   i = 0;
    CBIOS_U32   bEQFallBack = 0;
    CBIOS_U32   BitRate = 0;
    CBIOS_U32   ulLaneNum, MaxLaneCount = 0;
    CBIOS_U32   BitRateIndex = 0, MaxBitRateIndex = 0;
    AUX_CONTROL AUX;
    REG_MM8214  DPLinkRegValue, DPLinkRegMask;
    REG_MM8218  DPGenCtrlRegValue;
    REG_MM8240  DPEnableRegValue;
    REG_MM82CC  DPEphyCtrlRegValue, DPEphyCtrlRegMask;
    REG_MM8330  DPAuxTimerRegValue;
    REG_MM8338  DPMuteRegValue, DPMuteRegMask;
    REG_MM8368  DPSwingRegValue, DPSwingRegMask;
    REG_MM334C8 DPLinkCtrlRegValue, DPLinkCtrlRegMask;
    REG_MM334E0 DPEphySetting1RegValue, DPEphySetting1RegMask;
    DPCD_REG_00100 DPCD_00100;
    DPCD_REG_00101 DPCD_00101;
    DPCD_REG_0010A DPCD_0010A;
    DPCD_REG_00102 DPCD_00102;
    CBIOS_BOOL    bACE = ((pcbe->ChipID == CHIPID_ARISE2030) || (pcbe->ChipID == CHIPID_ARISE2020)) ? CBIOS_TRUE : CBIOS_FALSE;

    //For DP test 4.3.1.4, if use SW to try 1.62 when 2.7 fail, the ulMaxRetryCount should set to 1.
    CBIOS_U32   ulRetryCount = 0, ulMaxRetryCount = 3;

    if(DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "Invalid module index in %s\n", FUNCTION_NAME));
        return CBIOS_FALSE;
    }

    cbTraceEnter(DP);

    switch (DPModuleIndex)
    {
    case CBIOS_MODULE_INDEX1:
        Device = CBIOS_TYPE_DP1;
        break;
    case CBIOS_MODULE_INDEX2:
        Device = CBIOS_TYPE_DP2;
        break;
    case CBIOS_MODULE_INDEX3:
        Device = CBIOS_TYPE_DP3;
        break;
    case CBIOS_MODULE_INDEX4:
        Device = CBIOS_TYPE_DP4;
        break;
    default:
        break;
    }

    if(Device == CBIOS_TYPE_NONE)
    {
        cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: invalid DP module index, LT failed! \n", FUNCTION_NAME));
        return bStatus;
    }

    pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, Device);
    pDpContext = container_of(pDevCommon, PCBIOS_DP_CONTEXT, Common);

    if(pDpContext->DPPortParams.bRunCTS)
    {
        ulMaxRetryCount = 1;//For DP test 4.3.1.4, if use SW to try 1.62 when 2.7 fail, the ulMaxRetryCount should set to 1.
    }

    MaxLaneCount = pLinkTrainingParams->MaxLaneCount;
    if (pLinkTrainingParams->MaxLinkSpeed == CBIOS_DP_LINK_SPEED_5400Mbps)
    {
        MaxBitRateIndex = 3;
    }
    else if (pLinkTrainingParams->MaxLinkSpeed == CBIOS_DP_LINK_SPEED_2700Mbps)
    {
        MaxBitRateIndex = 2;
    }
    else
    {
        MaxBitRateIndex = 1;
    }

    // set DP version according to sink
    cbDIU_DP_SetDPVersion(pcbe, DPModuleIndex, pLinkTrainingParams->DpSinkVersion);

    BitRateIndex = MaxBitRateIndex;
    ulLaneNum = MaxLaneCount;
    while(ulLaneNum > 0 && BitRateIndex > 0)
    {
        if (BitRateIndex == 3)
        {
            BitRate = CBIOS_DP_LINK_SPEED_5400Mbps;
        }
        else if (BitRateIndex == 2)
        {
            BitRate = CBIOS_DP_LINK_SPEED_2700Mbps;
        }
        else
        {
            BitRate = CBIOS_DP_LINK_SPEED_1620Mbps;
        }

        for (ulRetryCount = 0; ulRetryCount < ulMaxRetryCount; ulRetryCount++)
        {
            //----------------- HW link training core:----------------------------------------
            // reset link training, MM8338[31] used for HW linktraining only.
            DPMuteRegValue.Value = 0;
            DPMuteRegValue.Link_Training_SW_Reset = 1;
            DPMuteRegMask.Value = 0xFFFFFFFF;
            DPMuteRegMask.Link_Training_SW_Reset = 0;
            cbMMIOWriteReg32(pcbe, DP_REG_MUTE[DPModuleIndex], DPMuteRegValue.Value, DPMuteRegMask.Value);

            cb_DelayMicroSeconds(1000); // wait HW Link Training Module Reset to be done especially on FPGA.

            DPMuteRegValue.Value = 0;
            DPMuteRegValue.Link_Training_SW_Reset = 0;
            DPMuteRegMask.Value = 0xFFFFFFFF;
            DPMuteRegMask.Link_Training_SW_Reset = 0;
            cbMMIOWriteReg32(pcbe, DP_REG_MUTE[DPModuleIndex], DPMuteRegValue.Value, DPMuteRegMask.Value);

            // Set MM8214
            DPLinkRegValue.Value = 0;
            DPLinkRegValue.Num_of_Lanes = ulLaneNum;
            DPLinkRegValue.HW_Link_Training_Done = 0;
            DPLinkRegValue.Max_V_swing = 3;
            DPLinkRegValue.Max_Pre_emphasis = 2;
            DPLinkRegValue.SW_Link_Train_Enable = 0;
            DPLinkRegValue.SW_Lane0_Swing = 0;
            DPLinkRegValue.SW_Lane1_Swing = 0;
            DPLinkRegValue.SW_Lane2_Swing = 0;
            DPLinkRegValue.SW_Lane3_Swing = 0;
            DPLinkRegMask.Value = 0xFFFFFFFF;
            DPLinkRegMask.Num_of_Lanes = 0;
            DPLinkRegMask.HW_Link_Training_Done = 0;
            DPLinkRegMask.Max_V_swing = 0;
            DPLinkRegMask.Max_Pre_emphasis = 0;
            DPLinkRegMask.SW_Link_Train_Enable = 0;
            DPLinkRegMask.SW_Lane0_Swing = 0;
            DPLinkRegMask.SW_Lane1_Swing = 0;
            DPLinkRegMask.SW_Lane2_Swing = 0;
            DPLinkRegMask.SW_Lane3_Swing = 0;

            DPLinkCtrlRegValue.Value = 0;
            DPLinkCtrlRegMask.Value = 0xFFFFFFFF;

            DPLinkCtrlRegValue.DPCD102_B5 = 1;
            DPLinkCtrlRegMask.DPCD102_B5 = 0;

            if (pLinkTrainingParams->bEnableTPS3)
            {
                // to support 5.4Gbps Link Rate, need to send Training Pattern 3
                DPLinkCtrlRegValue.EQ_USE_TP3 = 1;
                DPLinkCtrlRegMask.EQ_USE_TP3 = 0;
            }
            else
            {
                DPLinkCtrlRegValue.EQ_USE_TP3 = 0;
                DPLinkCtrlRegMask.EQ_USE_TP3 = 0;
            }

            // choose HW Link Training Speed
            if (BitRate == CBIOS_DP_LINK_SPEED_5400Mbps)
            {
                DPLinkRegValue.Start_Link_Rate_0 = 0;
                DPLinkRegMask.Start_Link_Rate_0 = 0;
                DPLinkCtrlRegValue.Start_LINK_RATE_1 = 1;
                DPLinkCtrlRegMask.Start_LINK_RATE_1 = 0;
            }
            else if (BitRate == CBIOS_DP_LINK_SPEED_2700Mbps)
            {
                DPLinkRegValue.Start_Link_Rate_0 = 1;
                DPLinkRegMask.Start_Link_Rate_0 = 0;
                DPLinkCtrlRegValue.Start_LINK_RATE_1 = 0;
                DPLinkCtrlRegMask.Start_LINK_RATE_1 = 0;
            }
            else
            {
                DPLinkRegValue.Start_Link_Rate_0 = 0;
                DPLinkRegMask.Start_Link_Rate_0 = 0;
                DPLinkCtrlRegValue.Start_LINK_RATE_1 = 0;
                DPLinkCtrlRegMask.Start_LINK_RATE_1 = 0;
            }

            cbMMIOWriteReg32(pcbe, DP_REG_LINK[DPModuleIndex], DPLinkRegValue.Value, DPLinkRegMask.Value);
            cbMMIOWriteReg32(pcbe, DP_REG_LINK_CTRL[DPModuleIndex], DPLinkCtrlRegValue.Value, DPLinkCtrlRegMask.Value);

            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: { %dMbps by %dLane(s) }: initiated...\n", FUNCTION_NAME, (BitRate / 1000), ulLaneNum));
            // Reset Aux, PISO, enable Scramble
            cbDIU_DP_LinkTrainingPreset(pcbe, DPModuleIndex, pLinkTrainingParams->bEnhancedMode);

            // Set DPCD
            // According linklayer compliance test spec,
            // Before link training pattern set, must update LINK_BW_SET and LANE_COUNT_SET fields of DPCD.
            DPCD_00100.Value = 0;
            if (BitRate == CBIOS_DP_LINK_SPEED_5400Mbps)
            {
                DPCD_00100.LINK_BW_SET = CBIOS_DPCD_LINK_RATE_5400Mbps;
            }
            else if (BitRate == CBIOS_DP_LINK_SPEED_2700Mbps)
            {
                DPCD_00100.LINK_BW_SET = CBIOS_DPCD_LINK_RATE_2700Mbps;
            }
            else
            {
                DPCD_00100.LINK_BW_SET = CBIOS_DPCD_LINK_RATE_1620Mbps;
            }

            DPCD_00101.Value = 0;
            DPCD_00101.LANE_COUNT_SET = (CBIOS_U8)ulLaneNum;
            //enhanced mode bit.
            //if eDP alternate framing is enabled, DPCD 101h enhanced frame mode should not be set
            if ((pLinkTrainingParams->bEnhancedMode) && (pLinkTrainingParams->CPMethod != CBIOS_EDP_CP_AF))
            {
                DPCD_00101.ENHANCED_FRAME_EN = 1;
            }
            AUX.Function = CBIOS_AUX_REQUEST_NATIVE_WRITE;
            AUX.Offset = 0x100;
            AUX.Length = 0x2;
            AUX.Data[0] = (DPCD_00101.Value << 8) | DPCD_00100.Value;
            if(!cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
            {
                cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: Aux Error before LT!\n", FUNCTION_NAME));
                goto EndOfLinkTraining;
            }

            //write eDP CP DPCD
            if (pLinkTrainingParams->CPMethod == CBIOS_EDP_CP_ASSR)
            {
                //enable ASSR for source
                DPEphyCtrlRegValue.Value = 0;
                DPEphyCtrlRegValue.EDP_ASSR = 1;
                DPEphyCtrlRegMask.Value = 0xFFFFFFFF;
                DPEphyCtrlRegMask.EDP_ASSR = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_EPHY_CTRL[DPModuleIndex], DPEphyCtrlRegValue.Value, DPEphyCtrlRegMask.Value);

                DPCD_0010A.Value = 0;
                DPCD_0010A.ALTERNATE_SCRAMBER_RESET_ENABLE = 1;
                AUX.Function = CBIOS_AUX_REQUEST_NATIVE_WRITE;
                AUX.Offset = 0x10A;
                AUX.Length = 1;
                AUX.Data[0] = DPCD_0010A.Value;
                if(!cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
                {
                    cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: Aux Error before LT!\n", FUNCTION_NAME));
                    goto EndOfLinkTraining;
                }
            }
            else if (pLinkTrainingParams->CPMethod == CBIOS_EDP_CP_AF)
            {
                DPCD_0010A.Value = 0;
                DPCD_0010A.FRAMING_CHANGE_ENABLE = 1;
                AUX.Function = CBIOS_AUX_REQUEST_NATIVE_WRITE;
                AUX.Offset = 0x10A;
                AUX.Length = 1;
                AUX.Data[0] = DPCD_0010A.Value;
                if(!cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
                {
                    cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: Aux Error before LT!\n", FUNCTION_NAME));
                    goto EndOfLinkTraining;
                }
            }
            else
            {
                //disable ASSR for source
                DPEphyCtrlRegValue.Value = 0;
                DPEphyCtrlRegValue.EDP_ASSR = 0;
                DPEphyCtrlRegMask.Value = 0xFFFFFFFF;
                DPEphyCtrlRegMask.EDP_ASSR = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_EPHY_CTRL[DPModuleIndex], DPEphyCtrlRegValue.Value, DPEphyCtrlRegMask.Value);

                //disable eDP CP
                DPCD_0010A.Value = 0;
                AUX.Function = CBIOS_AUX_REQUEST_NATIVE_WRITE;
                AUX.Offset = 0x10A;
                AUX.Length = 1;
                AUX.Data[0] = DPCD_0010A.Value;
                cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX);
                //here we don't check aux reply since for some monitors,
                //DPCD 0x10A is reserved and will not send back reply
            }

#if MEASURE_LT_TIME_GPIO
            // GPIO2 outputs 1 for measuring purpose
            cbMMIOWriteReg(pcbe,CR_B_DA,0x30,0x8F);
#endif

            cbDIU_DP_HWUseAuxChannel(pcbe, DPModuleIndex);

            // Increase TX amplitude by 60%.
            DPEphyCtrlRegValue.Value = 0;
            DPEphyCtrlRegValue.DIAJ_L0 = 1;
            DPEphyCtrlRegValue.DIAJ_L1 = 1;
            DPEphyCtrlRegValue.DIAJ_L2 = 1;
            DPEphyCtrlRegValue.DIAJ_L3 = 1;
            DPEphyCtrlRegMask.Value = 0xFF000FFF;
            cbMMIOWriteReg32(pcbe, DP_REG_EPHY_CTRL[DPModuleIndex], DPEphyCtrlRegValue.Value, DPEphyCtrlRegMask.Value);

            if (BitRate == CBIOS_DP_LINK_SPEED_5400Mbps)
            {
                DPSwingRegMask.Value = 0xFFFFFFFF;
                DPSwingRegMask.enable_SW_swing_pp = 0;
                DPSwingRegMask.SW_swing_SW_PP_SW_post_cursor_load_index = 0;
                DPSwingRegMask.DP1_SW_swing = 0;
                DPSwingRegMask.DP1_SW_pp = 0;
                DPSwingRegMask.DP1_SW_post_cursor = 0;
                if (bACE)
                {
                    DPEphyCtrlRegMask.Value = 0xFF000FFF;
                    DPEphySetting1RegMask.Value = 0xFFFFFFFF;
                    DPEphySetting1RegMask.EPHY1_SR_SPD = 0;
                    DPEphySetting1RegMask.EPHY1_SR_DLY = 0;
                    DPEphySetting1RegMask.EPHY1_SR_NDLY = 0;


                    DPEphyCtrlRegValue.Value = 0;
                    DPEphyCtrlRegValue.DIAJ_L0 = 1;
                    DPEphyCtrlRegValue.DIAJ_L1 = 1;
                    DPEphyCtrlRegValue.DIAJ_L2 = 1;
                    DPEphyCtrlRegValue.DIAJ_L3 = 1;
                    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_CTRL[DPModuleIndex], DPEphyCtrlRegValue.Value, DPEphyCtrlRegMask.Value);
                    DPEphySetting1RegValue.Value = 0;
                    DPEphySetting1RegValue.EPHY1_SR_SPD = 0;
                    DPEphySetting1RegValue.EPHY1_SR_DLY = 0;
                    DPEphySetting1RegValue.EPHY1_SR_NDLY = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_SETTING1[DPModuleIndex], DPEphySetting1RegValue.Value, DPEphySetting1RegMask.Value);
                    DPSwingRegValue.Value = 0;
                    DPSwingRegValue.enable_SW_swing_pp = 1;
                    DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 1;
                    DPSwingRegValue.DP1_SW_swing = 0x09;
                    DPSwingRegValue.DP1_SW_pp = 0;
                    DPSwingRegValue.DP1_SW_post_cursor = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                    DPEphyCtrlRegValue.Value = 0;
                    DPEphyCtrlRegValue.DIAJ_L0 = 1;
                    DPEphyCtrlRegValue.DIAJ_L1 = 1;
                    DPEphyCtrlRegValue.DIAJ_L2 = 1;
                    DPEphyCtrlRegValue.DIAJ_L3 = 1;
                    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_CTRL[DPModuleIndex], DPEphyCtrlRegValue.Value, DPEphyCtrlRegMask.Value);
                    DPEphySetting1RegValue.Value = 0;
                    DPEphySetting1RegValue.EPHY1_SR_SPD = 0;
                    DPEphySetting1RegValue.EPHY1_SR_DLY = 0;
                    DPEphySetting1RegValue.EPHY1_SR_NDLY = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_SETTING1[DPModuleIndex], DPEphySetting1RegValue.Value, DPEphySetting1RegMask.Value);
                    DPSwingRegValue.Value = 0;
                    DPSwingRegValue.enable_SW_swing_pp = 1;
                    DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 5;
                    DPSwingRegValue.DP1_SW_swing = 0x18;
                    DPSwingRegValue.DP1_SW_pp = 0x9;
                    DPSwingRegValue.DP1_SW_post_cursor = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                    DPEphyCtrlRegValue.Value = 0;
                    DPEphyCtrlRegValue.DIAJ_L0 = 3;
                    DPEphyCtrlRegValue.DIAJ_L1 = 3;
                    DPEphyCtrlRegValue.DIAJ_L2 = 3;
                    DPEphyCtrlRegValue.DIAJ_L3 = 3;
                    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_CTRL[DPModuleIndex], DPEphyCtrlRegValue.Value, DPEphyCtrlRegMask.Value);
                    DPEphySetting1RegValue.Value = 0;
                    DPEphySetting1RegValue.EPHY1_SR_SPD = 0;
                    DPEphySetting1RegValue.EPHY1_SR_DLY = 0;
                    DPEphySetting1RegValue.EPHY1_SR_NDLY = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_SETTING1[DPModuleIndex], DPEphySetting1RegValue.Value, DPEphySetting1RegMask.Value);
                    DPSwingRegValue.Value = 0;
                    DPSwingRegValue.enable_SW_swing_pp = 1;
                    DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 9;
                    DPSwingRegValue.DP1_SW_swing = 0x30;
                    DPSwingRegValue.DP1_SW_pp = 0x1F;
                    DPSwingRegValue.DP1_SW_post_cursor = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                    DPEphyCtrlRegValue.Value = 0;
                    DPEphyCtrlRegValue.DIAJ_L0 = 1;
                    DPEphyCtrlRegValue.DIAJ_L1 = 1;
                    DPEphyCtrlRegValue.DIAJ_L2 = 1;
                    DPEphyCtrlRegValue.DIAJ_L3 = 1;
                    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_CTRL[DPModuleIndex], DPEphyCtrlRegValue.Value, DPEphyCtrlRegMask.Value);
                    DPEphySetting1RegValue.Value = 0;
                    DPEphySetting1RegValue.EPHY1_SR_SPD = 0;
                    DPEphySetting1RegValue.EPHY1_SR_DLY = 0;
                    DPEphySetting1RegValue.EPHY1_SR_NDLY = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_SETTING1[DPModuleIndex], DPEphySetting1RegValue.Value, DPEphySetting1RegMask.Value);
                    DPSwingRegValue.Value = 0;
                    DPSwingRegValue.enable_SW_swing_pp = 1;
                    DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 0x11;
                    DPSwingRegValue.DP1_SW_swing = 0x10;
                    DPSwingRegValue.DP1_SW_pp = 0;
                    DPSwingRegValue.DP1_SW_post_cursor = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                    DPEphyCtrlRegValue.Value = 0;
                    DPEphyCtrlRegValue.DIAJ_L0 = 2;
                    DPEphyCtrlRegValue.DIAJ_L1 = 2;
                    DPEphyCtrlRegValue.DIAJ_L2 = 2;
                    DPEphyCtrlRegValue.DIAJ_L3 = 2;
                    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_CTRL[DPModuleIndex], DPEphyCtrlRegValue.Value, DPEphyCtrlRegMask.Value);
                    DPEphySetting1RegValue.Value = 0;
                    DPEphySetting1RegValue.EPHY1_SR_SPD = 0;
                    DPEphySetting1RegValue.EPHY1_SR_DLY = 0;
                    DPEphySetting1RegValue.EPHY1_SR_NDLY = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_SETTING1[DPModuleIndex], DPEphySetting1RegValue.Value, DPEphySetting1RegMask.Value);
                    DPSwingRegValue.Value = 0;
                    DPSwingRegValue.enable_SW_swing_pp = 1;
                    DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 0x15;
                    DPSwingRegValue.DP1_SW_swing = 0x3C;
                    DPSwingRegValue.DP1_SW_pp = 0x1B;
                    DPSwingRegValue.DP1_SW_post_cursor = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                    DPEphyCtrlRegValue.Value = 0;
                    DPEphyCtrlRegValue.DIAJ_L0 = 1;
                    DPEphyCtrlRegValue.DIAJ_L1 = 1;
                    DPEphyCtrlRegValue.DIAJ_L2 = 1;
                    DPEphyCtrlRegValue.DIAJ_L3 = 1;
                    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_CTRL[DPModuleIndex], DPEphyCtrlRegValue.Value, DPEphyCtrlRegMask.Value);
                    DPEphySetting1RegValue.Value = 0;
                    DPEphySetting1RegValue.EPHY1_SR_SPD = 1;
                    DPEphySetting1RegValue.EPHY1_SR_DLY = 1;
                    DPEphySetting1RegValue.EPHY1_SR_NDLY = 1;
                    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_SETTING1[DPModuleIndex], DPEphySetting1RegValue.Value, DPEphySetting1RegMask.Value);
                    DPSwingRegValue.Value = 0;
                    DPSwingRegValue.enable_SW_swing_pp = 1;
                    DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 0x19;
                    DPSwingRegValue.DP1_SW_swing = 0x30;
                    DPSwingRegValue.DP1_SW_pp = 0x12;
                    DPSwingRegValue.DP1_SW_post_cursor = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                    DPEphyCtrlRegValue.Value = 0;
                    DPEphyCtrlRegValue.DIAJ_L0 = 1;
                    DPEphyCtrlRegValue.DIAJ_L1 = 1;
                    DPEphyCtrlRegValue.DIAJ_L2 = 1;
                    DPEphyCtrlRegValue.DIAJ_L3 = 1;
                    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_CTRL[DPModuleIndex], DPEphyCtrlRegValue.Value, DPEphyCtrlRegMask.Value);
                    DPEphySetting1RegValue.Value = 0;
                    DPEphySetting1RegValue.EPHY1_SR_SPD = 0;
                    DPEphySetting1RegValue.EPHY1_SR_DLY = 0;
                    DPEphySetting1RegValue.EPHY1_SR_NDLY = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_SETTING1[DPModuleIndex], DPEphySetting1RegValue.Value, DPEphySetting1RegMask.Value);
                    DPSwingRegValue.Value = 0;
                    DPSwingRegValue.enable_SW_swing_pp = 1;
                    DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 0x1D;
                    DPSwingRegValue.DP1_SW_swing = 0x19;
                    DPSwingRegValue.DP1_SW_pp = 0;
                    DPSwingRegValue.DP1_SW_post_cursor = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                    DPEphyCtrlRegValue.Value = 0;
                    DPEphyCtrlRegValue.DIAJ_L0 = 1;
                    DPEphyCtrlRegValue.DIAJ_L1 = 1;
                    DPEphyCtrlRegValue.DIAJ_L2 = 1;
                    DPEphyCtrlRegValue.DIAJ_L3 = 1;
                    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_CTRL[DPModuleIndex], DPEphyCtrlRegValue.Value, DPEphyCtrlRegMask.Value);
                    DPEphySetting1RegValue.Value = 0;
                    DPEphySetting1RegValue.EPHY1_SR_SPD = 1;
                    DPEphySetting1RegValue.EPHY1_SR_DLY = 1;
                    DPEphySetting1RegValue.EPHY1_SR_NDLY = 1;
                    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_SETTING1[DPModuleIndex], DPEphySetting1RegValue.Value, DPEphySetting1RegMask.Value);
                    DPSwingRegValue.Value = 0;
                    DPSwingRegValue.enable_SW_swing_pp = 1;
                    DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 0x21;
                    DPSwingRegValue.DP1_SW_swing = 0x1C;
                    DPSwingRegValue.DP1_SW_pp = 0;
                    DPSwingRegValue.DP1_SW_post_cursor = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                    DPEphyCtrlRegValue.Value = 0;
                    DPEphyCtrlRegValue.DIAJ_L0 = 1;
                    DPEphyCtrlRegValue.DIAJ_L1 = 1;
                    DPEphyCtrlRegValue.DIAJ_L2 = 1;
                    DPEphyCtrlRegValue.DIAJ_L3 = 1;
                    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_CTRL[DPModuleIndex], DPEphyCtrlRegValue.Value, DPEphyCtrlRegMask.Value);
                    DPEphySetting1RegValue.Value = 0;
                    DPEphySetting1RegValue.EPHY1_SR_SPD = 0;
                    DPEphySetting1RegValue.EPHY1_SR_DLY = 0;
                    DPEphySetting1RegValue.EPHY1_SR_NDLY = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_SETTING1[DPModuleIndex], DPEphySetting1RegValue.Value, DPEphySetting1RegMask.Value);
                    DPSwingRegValue.Value = 0;
                    DPSwingRegValue.enable_SW_swing_pp = 1;
                    DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 0x25;
                    DPSwingRegValue.DP1_SW_swing = 0x3F;
                    DPSwingRegValue.DP1_SW_pp = 0;
                    DPSwingRegValue.DP1_SW_post_cursor = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);
                }
                else
                {
                    DPSwingRegValue.Value = 0;
                    DPSwingRegValue.enable_SW_swing_pp = 1;
                    DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 1;
                    DPSwingRegValue.DP1_SW_swing = 0x09;
                    DPSwingRegValue.DP1_SW_pp = 0;
                    DPSwingRegValue.DP1_SW_post_cursor = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                    DPSwingRegValue.Value = 0;
                    DPSwingRegValue.enable_SW_swing_pp = 1;
                    DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 5;
                    DPSwingRegValue.DP1_SW_swing = 0x10;
                    DPSwingRegValue.DP1_SW_pp = 0;
                    DPSwingRegValue.DP1_SW_post_cursor = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                    DPSwingRegValue.Value = 0;
                    DPSwingRegValue.enable_SW_swing_pp = 1;
                    DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 9;
                    DPSwingRegValue.DP1_SW_swing = 0x17;
                    DPSwingRegValue.DP1_SW_pp = 0x1C;
                    DPSwingRegValue.DP1_SW_post_cursor = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                    DPSwingRegValue.Value = 0;
                    DPSwingRegValue.enable_SW_swing_pp = 1;
                    DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 0x11;
                    DPSwingRegValue.DP1_SW_swing = 0x11;
                    DPSwingRegValue.DP1_SW_pp = 0;
                    DPSwingRegValue.DP1_SW_post_cursor = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                    DPSwingRegValue.Value = 0;
                    DPSwingRegValue.enable_SW_swing_pp = 1;
                    DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 0x15;
                    DPSwingRegValue.DP1_SW_swing = 0x19;
                    DPSwingRegValue.DP1_SW_pp = 0x08;
                    DPSwingRegValue.DP1_SW_post_cursor = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                    DPSwingRegValue.Value = 0;
                    DPSwingRegValue.enable_SW_swing_pp = 1;
                    DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 0x19;
                    DPSwingRegValue.DP1_SW_swing = 0x33;
                    DPSwingRegValue.DP1_SW_pp = 0x1A;
                    DPSwingRegValue.DP1_SW_post_cursor = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                    DPSwingRegValue.Value = 0;
                    DPSwingRegValue.enable_SW_swing_pp = 1;
                    DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 0x1D;
                    DPSwingRegValue.DP1_SW_swing = 0x1C;
                    DPSwingRegValue.DP1_SW_pp = 0;
                    DPSwingRegValue.DP1_SW_post_cursor = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                    DPSwingRegValue.Value = 0;
                    DPSwingRegValue.enable_SW_swing_pp = 1;
                    DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 0x21;
                    DPSwingRegValue.DP1_SW_swing = 0x14;
                    DPSwingRegValue.DP1_SW_pp = 0;
                    DPSwingRegValue.DP1_SW_post_cursor = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                    DPSwingRegValue.Value = 0;
                    DPSwingRegValue.enable_SW_swing_pp = 1;
                    DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 0x25;
                    DPSwingRegValue.DP1_SW_swing = 0x3F;
                    DPSwingRegValue.DP1_SW_pp = 0;
                    DPSwingRegValue.DP1_SW_post_cursor = 0;
                    cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);
                }
            }
            else
            {
                DPSwingRegValue.Value = 0;
                DPSwingRegValue.enable_SW_swing_pp = 1;
                DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 1;
                DPSwingRegValue.DP1_SW_swing = 0x09;
                DPSwingRegValue.DP1_SW_pp = 0;
                DPSwingRegValue.DP1_SW_post_cursor = 0;

                DPSwingRegMask.Value = 0xFFFFFFFF;
                DPSwingRegMask.enable_SW_swing_pp = 0;
                DPSwingRegMask.SW_swing_SW_PP_SW_post_cursor_load_index = 0;
                DPSwingRegMask.DP1_SW_swing = 0;
                DPSwingRegMask.DP1_SW_pp = 0;
                DPSwingRegMask.DP1_SW_post_cursor = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                DPSwingRegValue.Value = 0;
                DPSwingRegValue.enable_SW_swing_pp = 1;
                DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 5;
                DPSwingRegValue.DP1_SW_swing = 0x15;
                DPSwingRegValue.DP1_SW_pp = 0x06;
                DPSwingRegValue.DP1_SW_post_cursor = 0;

                DPSwingRegMask.Value = 0xFFFFFFFF;
                DPSwingRegMask.enable_SW_swing_pp = 0;
                DPSwingRegMask.SW_swing_SW_PP_SW_post_cursor_load_index = 0;
                DPSwingRegMask.DP1_SW_swing = 0;
                DPSwingRegMask.DP1_SW_pp = 0;
                DPSwingRegMask.DP1_SW_post_cursor = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                DPSwingRegValue.Value = 0;
                DPSwingRegValue.enable_SW_swing_pp = 1;
                DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 9;
                DPSwingRegValue.DP1_SW_swing = 0x1D;
                DPSwingRegValue.DP1_SW_pp = 0x0E;
                DPSwingRegValue.DP1_SW_post_cursor = 0;

                DPSwingRegMask.Value = 0xFFFFFFFF;
                DPSwingRegMask.enable_SW_swing_pp = 0;
                DPSwingRegMask.SW_swing_SW_PP_SW_post_cursor_load_index = 0;
                DPSwingRegMask.DP1_SW_swing = 0;
                DPSwingRegMask.DP1_SW_pp = 0;
                DPSwingRegMask.DP1_SW_post_cursor = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                DPSwingRegValue.Value = 0;
                DPSwingRegValue.enable_SW_swing_pp = 1;
                DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 0x11;
                DPSwingRegValue.DP1_SW_swing = 0x11;
                DPSwingRegValue.DP1_SW_pp = 0;
                DPSwingRegValue.DP1_SW_post_cursor = 0;

                DPSwingRegMask.Value = 0xFFFFFFFF;
                DPSwingRegMask.enable_SW_swing_pp = 0;
                DPSwingRegMask.SW_swing_SW_PP_SW_post_cursor_load_index = 0;
                DPSwingRegMask.DP1_SW_swing = 0;
                DPSwingRegMask.DP1_SW_pp = 0;
                DPSwingRegMask.DP1_SW_post_cursor = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                DPSwingRegValue.Value = 0;
                DPSwingRegValue.enable_SW_swing_pp = 1;
                DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 0x15;
                DPSwingRegValue.DP1_SW_swing = 0x30;
                DPSwingRegValue.DP1_SW_pp = 0x08;
                DPSwingRegValue.DP1_SW_post_cursor = 0;

                DPSwingRegMask.Value = 0xFFFFFFFF;
                DPSwingRegMask.enable_SW_swing_pp = 0;
                DPSwingRegMask.SW_swing_SW_PP_SW_post_cursor_load_index = 0;
                DPSwingRegMask.DP1_SW_swing = 0;
                DPSwingRegMask.DP1_SW_pp = 0;
                DPSwingRegMask.DP1_SW_post_cursor = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                DPSwingRegValue.Value = 0;
                DPSwingRegValue.enable_SW_swing_pp = 1;
                DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 0x19;
                DPSwingRegValue.DP1_SW_swing = 0x39;
                DPSwingRegValue.DP1_SW_pp = 0x1A;
                DPSwingRegValue.DP1_SW_post_cursor = 0;

                DPSwingRegMask.Value = 0xFFFFFFFF;
                DPSwingRegMask.enable_SW_swing_pp = 0;
                DPSwingRegMask.SW_swing_SW_PP_SW_post_cursor_load_index = 0;
                DPSwingRegMask.DP1_SW_swing = 0;
                DPSwingRegMask.DP1_SW_pp = 0;
                DPSwingRegMask.DP1_SW_post_cursor = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                DPSwingRegValue.Value = 0;
                DPSwingRegValue.enable_SW_swing_pp = 1;
                DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 0x1D;
                DPSwingRegValue.DP1_SW_swing = 0x30;
                DPSwingRegValue.DP1_SW_pp = 0;
                DPSwingRegValue.DP1_SW_post_cursor = 0;

                DPSwingRegMask.Value = 0xFFFFFFFF;
                DPSwingRegMask.enable_SW_swing_pp = 0;
                DPSwingRegMask.SW_swing_SW_PP_SW_post_cursor_load_index = 0;
                DPSwingRegMask.DP1_SW_swing = 0;
                DPSwingRegMask.DP1_SW_pp = 0;
                DPSwingRegMask.DP1_SW_post_cursor = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                DPSwingRegValue.Value = 0;
                DPSwingRegValue.enable_SW_swing_pp = 1;
                DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 0x21;
                DPSwingRegValue.DP1_SW_swing = bACE ? 0x3C : 0x39;
                DPSwingRegValue.DP1_SW_pp = bACE ? 0x7 : 0xA;
                DPSwingRegValue.DP1_SW_post_cursor = 0;

                DPSwingRegMask.Value = 0xFFFFFFFF;
                DPSwingRegMask.enable_SW_swing_pp = 0;
                DPSwingRegMask.SW_swing_SW_PP_SW_post_cursor_load_index = 0;
                DPSwingRegMask.DP1_SW_swing = 0;
                DPSwingRegMask.DP1_SW_pp = 0;
                DPSwingRegMask.DP1_SW_post_cursor = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                DPSwingRegValue.Value = 0;
                DPSwingRegValue.enable_SW_swing_pp = 1;
                DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 0x25;
                DPSwingRegValue.DP1_SW_swing = 0x3F;
                DPSwingRegValue.DP1_SW_pp = 0;
                DPSwingRegValue.DP1_SW_post_cursor = 0;

                DPSwingRegMask.Value = 0xFFFFFFFF;
                DPSwingRegMask.enable_SW_swing_pp = 0;
                DPSwingRegMask.SW_swing_SW_PP_SW_post_cursor_load_index = 0;
                DPSwingRegMask.DP1_SW_swing = 0;
                DPSwingRegMask.DP1_SW_pp = 0;
                DPSwingRegMask.DP1_SW_post_cursor = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);
                cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], 0x2687F8, 0xFFC00000);
            }

            if (pLinkTrainingParams->DpSinkVersion >= 0x12)
            {
                DPSwingRegValue.Value = 0;
                DPSwingRegValue.VER1P2 = 1;
                DPSwingRegValue.RD_INTVAL = pLinkTrainingParams->TrainingAuxRdInterval;
                DPSwingRegMask.Value = 0xFFFFFFFF;
                DPSwingRegMask.VER1P2 = 0;
                DPSwingRegMask.RD_INTVAL = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);
            }

            // enable HW link training
            DPLinkRegValue.Value = 0;
            DPLinkRegValue.Start_Link_Training = 1;
            DPLinkRegMask.Value = 0xFFFFFFFF;
            DPLinkRegMask.Start_Link_Training = 0;
            cbMMIOWriteReg32(pcbe, DP_REG_LINK[DPModuleIndex], DPLinkRegValue.Value, DPLinkRegMask.Value);

            // Spec says LT should be done within 10ms.
            // Here we wait 30ms.
            for(i = 0; i <= 600; i++)
            {
                cb_DelayMicroSeconds(250);

                DPLinkRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_LINK[DPModuleIndex]);
                if (DPLinkRegValue.HW_Link_Training_Done)
                {
                    DPEnableRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_ENABLE[DPModuleIndex]);
                    if (DPEnableRegValue.Main_Link_Status == 3)
                    {
                        DPLinkRegValue.Value = 0;
                        DPLinkRegValue.Start_Link_Training = 0;
                        DPLinkRegValue.HW_Link_Training_Done = 0;
                        DPLinkRegMask.Value = 0xFFFFFFFF;
                        DPLinkRegMask.Start_Link_Training = 0;
                        DPLinkRegMask.HW_Link_Training_Done = 0;
                        cbMMIOWriteReg32(pcbe, DP_REG_LINK[DPModuleIndex], DPLinkRegValue.Value, DPLinkRegMask.Value);
                        bStatus = CBIOS_TRUE;
                        break;
                    }
                }
            }

            if (bStatus)
            {
                cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: { %dMbps by %dLane(s) }: Succeed!\n", FUNCTION_NAME, (BitRate / 1000), ulLaneNum));
                goto LinkTrainingDone;
            }
            else
            {
                cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: { %dMbps by %dLane(s) }: Fail!\n", FUNCTION_NAME, (BitRate / 1000), ulLaneNum));

                //clear last link training fail status
                DPLinkRegValue.Value = 0;
                DPLinkRegValue.Start_Link_Training = 0;
                DPLinkRegValue.HW_Link_Training_Done = 0;
                DPLinkRegMask.Value = 0xFFFFFFFF;
                DPLinkRegMask.Start_Link_Training = 0;
                DPLinkRegMask.HW_Link_Training_Done = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_LINK[DPModuleIndex], DPLinkRegValue.Value, DPLinkRegMask.Value);

                //clear link training pattern
                DPCD_00102.Value = 0;
                DPCD_00102.TRAINING_PATTERN_SELECT = 0;
                DPCD_00102.SCRAMBLING_DISABLE = 0;
                AUX.Function = CBIOS_AUX_REQUEST_NATIVE_WRITE;
                AUX.Offset = 0x102;
                AUX.Length = 0x1;
                AUX.Data[0] = DPCD_00102.Value;
                if(!cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
                {
                    cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: Aux Error before LT!\n", FUNCTION_NAME));
                    goto EndOfLinkTraining;
                }

            }

        }//end of retry loop

        AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
        AUX.Offset = 0x202;
        AUX.Length = 0x2;
        if(cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
        {
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "AUX.Data[0] = %x\n", AUX.Data[0]));

            if(BitRateIndex == MaxBitRateIndex && ulLaneNum == MaxLaneCount)
            {
                if((MaxLaneCount == 4 && AUX.Data[0] == 0x1177)||
                    (MaxLaneCount == 2 && ((AUX.Data[0]&0xFF) == 0x17)))
                    {
                        bEQFallBack = 1;
                    }
            }
        }

        if(bEQFallBack)
        {
            ulLaneNum = ulLaneNum >> 1;
            if(ulLaneNum == 0 && BitRateIndex > 0)
            {
                ulLaneNum = MaxLaneCount;
                BitRateIndex --;
                if(BitRateIndex == 0)
                {
                    ulLaneNum = 0;
                }
            }
        }
        else
        {
            BitRateIndex --;
            if(BitRateIndex == 0 && ulLaneNum > 0)
            {
                BitRateIndex = MaxBitRateIndex;
                ulLaneNum = ulLaneNum >> 1;
                if(ulLaneNum == 0)
                {
                    BitRateIndex = 0;
                }
            }
        }
    }//end of lane count loop

LinkTrainingDone:
    //link training succeeded, update current link speed and lane count
    if (bStatus)
    {
        pLinkTrainingParams->CurrLinkSpeed = BitRate;
        pLinkTrainingParams->CurrLaneCount = ulLaneNum;
    }

    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: Polling Loop Count ==: %d, equivalent time (i*50us) == %dus!\n", FUNCTION_NAME, i, (i * 50)));

#if MEASURE_LT_TIME_GPIO
    // GPIO2 outputs 0 for measuring purpose
    cbMMIOWriteReg(pcbe,CR_B_DA,0x20,0x8F);
#endif
EndOfLinkTraining:
    if (!bStatus)
    {
        DPGenCtrlRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_GEN_CTRL[DPModuleIndex]);
        DPAuxTimerRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_TIMER[DPModuleIndex]);
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: LT fail: mm8218==0x%08x, mm8330==0x%08x\n", FUNCTION_NAME, DPGenCtrlRegValue.Value, DPAuxTimerRegValue.Value));

        if (DPGenCtrlRegValue.HW_Link_Train_Fail)
        {
            cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: LT failed due to Aux error!\n", FUNCTION_NAME));

            // Clear TRAINING_PATTERN_SET byte
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: write DPCD_102 0x00 to abort LT and recover Aux\n", FUNCTION_NAME));

            DPCD_00102.Value = 0;
            DPCD_00102.TRAINING_PATTERN_SELECT = 0;
            DPCD_00102.SCRAMBLING_DISABLE = 0;
            AUX.Function = CBIOS_AUX_REQUEST_NATIVE_WRITE;
            AUX.Offset = 0x102;
            AUX.Length = 0x1;
            AUX.Data[0] = DPCD_00102.Value;
            if(!cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
            {
                cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: Write DPCD error to abort LT!\n", FUNCTION_NAME));
                bStatus = CBIOS_FALSE;
            }
        }
    }

    cbDIU_DP_HWUseAuxChannel(pcbe, DPModuleIndex);

    cbTraceExit(DP);

    return bStatus;
}

CBIOS_BOOL cbDIU_DP_SetUpMainLink(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, PCBIOS_MAIN_LINK_PARAMS pMainLinkParams)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_BOOL              bStatus = CBIOS_FALSE;
    PCBIOS_TIMING_ATTRIB    pTiming = pMainLinkParams->pTiming;
    CBIOS_U64               ulTURatio, ulTemp1, ulTemp2, ulTemp3;
    REG_MM8210              DPMisc1RegValue, DPMisc1RegMask;
    REG_MM8218              DPGenCtrlRegValue, DPGenCtrlRegMask;
    REG_MM821C              DPExtPacketRegValue, DPExtPacketRegMask;
    REG_MM8240              DPEnableRegValue, DPEnableRegMask;
    REG_MM8244              DPHWidthTURegValue, DPHWidthTURegMask;
    REG_MM8248              DPHLineDurRegValue, DPHLineDurRegMask;
    REG_MM824C              DPMisc0RegValue, DPMisc0RegMask;
    REG_MM8250              DPHVTotalRegValue, DPHVTotalRegMask;
    REG_MM8254              DPHVStartRegValue, DPHVStartRegMask;
    REG_MM8258              DPHVSyncRegValue, DPHVSyncRegMask;
    REG_MM825C              DPHVActiveRegValue, DPHVActiveRegMask;
    REG_MM334DC             DPCtrlRegValue;
    REG_SR3A_Pair           RegSR3AValue;
    REG_SR3A_Pair           RegSR3AMask;
    AUX_CONTROL             AUX;
    DPCD_REG_00100          DPCD_0100;

    cbTraceEnter(DP);

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return bStatus;
    }

    DPEnableRegValue.Value = 0;
    DPEnableRegValue.Field_Invert = 0;
    DPEnableRegMask.Value = 0xFFFFFFFF;
    DPEnableRegMask.Field_Invert = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_ENABLE[DPModuleIndex], DPEnableRegValue.Value, DPEnableRegMask.Value);

    #if 1
    DPCtrlRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_CTRL[DPModuleIndex]);
    if (DPCtrlRegValue.LINK_bit_rate_status_1_0 == 0)
    {
        if (pMainLinkParams->LinkedSpeed != CBIOS_DP_LINK_SPEED_1620Mbps)
        {
            cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: why current link speed:%d is not sync with REG:0x%x?\n",
                            FUNCTION_NAME, pMainLinkParams->LinkedSpeed, DP_REG_CTRL[DPModuleIndex]));
            pMainLinkParams->LinkedSpeed = CBIOS_DP_LINK_SPEED_1620Mbps;
        }
    }
    else if (DPCtrlRegValue.LINK_bit_rate_status_1_0 == 1)
    {
        if (pMainLinkParams->LinkedSpeed != CBIOS_DP_LINK_SPEED_2700Mbps)
        {
            cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: why current link speed:%d is not sync with REG:0x%x?\n",
                            FUNCTION_NAME, pMainLinkParams->LinkedSpeed, DP_REG_CTRL[DPModuleIndex]));
            pMainLinkParams->LinkedSpeed = CBIOS_DP_LINK_SPEED_2700Mbps;
        }
    }
    else
    {
        if (pMainLinkParams->LinkedSpeed != CBIOS_DP_LINK_SPEED_5400Mbps)
        {
            cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: why current link speed:%d is not sync with REG:0x%x?\n",
                            FUNCTION_NAME, pMainLinkParams->LinkedSpeed, DP_REG_CTRL[DPModuleIndex]));
            pMainLinkParams->LinkedSpeed = CBIOS_DP_LINK_SPEED_5400Mbps;
        }
    }
    #endif

    AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
    AUX.Offset = 0x100;
    AUX.Length = 1;
    if(cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
    {
        DPCD_0100.Value = (CBIOS_U8)AUX.Data[0];

        switch(DPCD_0100.LINK_BW_SET)
        {
            case CBIOS_DPCD_LINK_RATE_1620Mbps:
                if (pMainLinkParams->LinkedSpeed != CBIOS_DP_LINK_SPEED_1620Mbps)
                {
                    cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: why current link speed:%d is not sync with DPCD100?\n",
                            FUNCTION_NAME, pMainLinkParams->LinkedSpeed));
                    pMainLinkParams->LinkedSpeed = CBIOS_DP_LINK_SPEED_1620Mbps;
                }
                break;
            case CBIOS_DPCD_LINK_RATE_2700Mbps:
                if (pMainLinkParams->LinkedSpeed != CBIOS_DP_LINK_SPEED_2700Mbps)
                {
                    cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: why current link speed:%d is not sync with DPCD100?\n",
                            FUNCTION_NAME, pMainLinkParams->LinkedSpeed));
                    pMainLinkParams->LinkedSpeed = CBIOS_DP_LINK_SPEED_2700Mbps;
                }
                break;
            case CBIOS_DPCD_LINK_RATE_5400Mbps:
                if (pMainLinkParams->LinkedSpeed != CBIOS_DP_LINK_SPEED_5400Mbps)
                {
                    cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: why current link speed:%d is not sync with DPCD100?\n",
                            FUNCTION_NAME, pMainLinkParams->LinkedSpeed));
                    pMainLinkParams->LinkedSpeed = CBIOS_DP_LINK_SPEED_5400Mbps;
                }
                break;
        }
    }

    // TU: Transfer Unit(used to carry main video stream data during its horizontal active period).
    //     TU has 32 to 64 symbols per lane. Currently we default to 48.
    // MM8244, H width and TU size/ratio, MM821C[30]:Bit-12 of HDisp
    // TU ratio = (Dclk * bpp) * 32768 / (Ls_clk * Lane# * 8)
    //  bpp: bit per pixel
    //  bpc: bit per channel
    if (pMainLinkParams->ColorFormat == 1) // YUV422 format
    {
        ulTemp1 = pTiming->PixelClock*2*pMainLinkParams->bpc;
    }
    else
    {
        ulTemp1 = pTiming->PixelClock*3*pMainLinkParams->bpc;
    }

    ulTemp2 = pMainLinkParams->LinkedLaneNumber * pMainLinkParams->LinkedSpeed * 8;

    if (ulTemp2 == 0) // Prevent being divided by zero
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: fata error -- LaneNumber or LinkSpeed is ZERO!!!\n", FUNCTION_NAME));
        bStatus = CBIOS_FALSE;
        goto Exit;
    }
    ulTURatio = cb_do_div((ulTemp1<<15), ulTemp2);

    DPHWidthTURegValue.Value = 0;
    if (pMainLinkParams->LinkedLaneNumber == 0) // Prevent being divided by zero
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: fata error -- LaneNumber is ZERO!!!\n", FUNCTION_NAME));
        bStatus = CBIOS_FALSE;
        goto Exit;
    }
    DPHWidthTURegValue.Horiz_Width = pTiming->HorDisEnd / pMainLinkParams->LinkedLaneNumber - 1;
    DPHWidthTURegValue.TU_Size = (pMainLinkParams->TUSize - 1);
    DPHWidthTURegValue.TU_Ratio = (CBIOS_U32)ulTURatio;
    DPHWidthTURegMask.Value = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_HWIDTH_TU[DPModuleIndex], DPHWidthTURegValue.Value, DPHWidthTURegMask.Value);

    DPExtPacketRegValue.Value = 0;
    DPExtPacketRegValue.Horizontal_Width_bit12to11 = ((pTiming->HorDisEnd / pMainLinkParams->LinkedLaneNumber - 1) >> 11);
    DPExtPacketRegMask.Value = 0xFFFFFFFF;
    DPExtPacketRegMask.Horizontal_Width_bit12to11 = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EXT_PACKET[DPModuleIndex], DPExtPacketRegValue.Value, DPExtPacketRegMask.Value);

    // MM8248, H line duration: H * Ls_clk / Dclk
    // ulTemp2 = pDevCommon->DeviceParas.DPDevice.LinkSpeed / pTiming->DCLK;
    // ulTemp3 = ((ulTemp1 * ulTemp2) & 0x00000FFF ) << 15;
    ulTemp1 = pTiming->HorTotal - pTiming->HorDisEnd;

    if (pTiming->PixelClock == 0) // Prevent being divided by zero
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: fata error -- PixelClock is ZERO!!!\n", FUNCTION_NAME));
        bStatus = CBIOS_FALSE;
        goto Exit;
    }

    /*previous setting -35, will lead to audio abnormal with some specific timing(ex. 800x600@75), change it to 64*/
    ulTemp3 = (cb_do_div(ulTemp1 * pMainLinkParams->LinkedSpeed, pTiming->PixelClock) & 0x00000FFF) - 64;

    ulTemp2 = pTiming->HorTotal;
    ulTemp1 = cb_do_div(ulTemp2 * pMainLinkParams->LinkedSpeed, pTiming->PixelClock)  - 64;

    DPHLineDurRegValue.Value = 0;
    DPHLineDurRegValue.Horiz_Line_Duration = (CBIOS_U32)ulTemp1;
    DPHLineDurRegValue.HBLANK_Duration = (CBIOS_U32)ulTemp3;
    DPHLineDurRegMask.Value = 0xFFFFFFFF;
    DPHLineDurRegMask.Horiz_Line_Duration = 0;
    DPHLineDurRegMask.HBLANK_Duration = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_HLINE_DUR[DPModuleIndex], DPHLineDurRegValue.Value, DPHLineDurRegMask.Value);

    DPEnableRegValue.Value = 0;
    DPEnableRegValue.Generate_MVID = pMainLinkParams->AsyncMode ? 1 : 0;
    DPEnableRegMask.Value = 0xFFFFFFFF;
    DPEnableRegMask.Generate_MVID = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_ENABLE[DPModuleIndex], DPEnableRegValue.Value, DPEnableRegMask.Value);

    // M/N = Dclk / Ls_clk, but N must be 32768 to work with Parade DP
    // So, N = 32768, M = Dclk * 32768 / Ls_clk
    // Mvid and Nvid are not needed if in asynchronous mode, set it anyway
    if (pMainLinkParams->LinkedSpeed == 0) // Prevent being divided by zero
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: fata error -- LinkSpeed is ZERO!!!\n", FUNCTION_NAME));
        bStatus = CBIOS_FALSE;
        goto Exit;
    }
    ulTemp1 = cb_do_div(32768 * ((CBIOS_U64)(pTiming->PixelClock)), pMainLinkParams->LinkedSpeed); // N = 32768

    DPMisc0RegValue.Value = 0;
    DPMisc0RegValue.MVID = (CBIOS_U32)ulTemp1;

    if (pMainLinkParams->bpc == 6)
        DPMisc0RegValue.MISC0_Bit_depth = 0;
    else if (pMainLinkParams->bpc == 8)
        DPMisc0RegValue.MISC0_Bit_depth = 1;
    else if (pMainLinkParams->bpc == 10)
        DPMisc0RegValue.MISC0_Bit_depth = 2;
    else if (pMainLinkParams->bpc == 12)
        DPMisc0RegValue.MISC0_Bit_depth = 3;
    else if (pMainLinkParams->bpc == 16)
        DPMisc0RegValue.MISC0_Bit_depth = 4;

    DPMisc0RegValue.MISC0_Sync_Clk = pMainLinkParams->AsyncMode ? 0 : 1;
    DPMisc0RegValue.MISC0_Component_Format = pMainLinkParams->ColorFormat;

    // bit[27] "VESA/CEA range"
    if (pMainLinkParams->DynamicRange == 1)
    {
        DPMisc0RegValue.MISC0_Dynamic_Range = 1;
    }

    // bit[28] "YCbCr colorimetry" ITU-R BT601-5/ITU-R BT709-5
    if (pMainLinkParams->YCbCrCoefficients == 1)
    {
        DPMisc0RegValue.MISC0_YCbCr_Colorimetry = 1;
    }

    DPMisc0RegMask.Value = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_MISC0[DPModuleIndex], DPMisc0RegValue.Value, DPMisc0RegMask.Value);

    // MM8250, DP HTotal attribute data
    DPHVTotalRegValue.H_Total = pTiming->HorTotal;
    DPHVTotalRegValue.V_Total = pTiming->VerTotal;
    DPHVTotalRegMask.Value = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_HV_TOTAL[DPModuleIndex], DPHVTotalRegValue.Value, DPHVTotalRegMask.Value);

    // MM8254, DP H/V start attribute data (from leading edge of Sync)
    DPHVStartRegValue.H_Start = (pTiming->HorTotal - pTiming->HorSyncStart);
    DPHVStartRegValue.V_Start = (pTiming->VerTotal - pTiming->VerSyncStart);
    DPHVStartRegMask.Value = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_HV_START[DPModuleIndex], DPHVStartRegValue.Value, DPHVStartRegMask.Value);

    // MM8258, DP H/V Sync Polarity, Width attribute data
    DPHVSyncRegValue.HSYNC_Width = (pTiming->HorSyncEnd - pTiming->HorSyncStart);
    DPHVSyncRegValue.VSYNC_Width = (pTiming->VerSyncEnd - pTiming->VerSyncStart);

    RegSR3AValue.Value = 0;
    RegSR3AValue.DP_PHY_Test_Mode_Select = 0;
    RegSR3AValue.FP_DP_HYSNC_POL = 0;
    RegSR3AValue.FP_DP_VYSNC_POL = 0;
    RegSR3AMask.Value = 0x38;
    cbMMIOWriteReg(pcbe, SR_3A, RegSR3AValue.Value, RegSR3AMask.Value);

    DPHVSyncRegValue.HSYNC_Polarity = 0;
    DPHVSyncRegValue.VSYNC_Polarity = 0;

    if (pTiming->HVPolarity & HorNEGATIVE)
    {
        RegSR3AValue.Value = 0;
        RegSR3AValue.DP_PHY_Test_Mode_Select = 0;
        RegSR3AValue.FP_DP_HYSNC_POL = 0;
        RegSR3AValue.FP_DP_VYSNC_POL = 1;
        RegSR3AMask.Value = 0x38;
        cbMMIOWriteReg(pcbe, SR_3A, RegSR3AValue.Value, RegSR3AMask.Value);
        DPHVSyncRegValue.HSYNC_Polarity = 1;
    }
    if (pTiming->HVPolarity & VerNEGATIVE)
    {
        RegSR3AValue.Value = 0;
        RegSR3AValue.DP_PHY_Test_Mode_Select = 0;
        RegSR3AValue.FP_DP_HYSNC_POL = 1;
        RegSR3AValue.FP_DP_VYSNC_POL = 0;
        RegSR3AMask.Value = 0x38;
        cbMMIOWriteReg(pcbe, SR_3A, RegSR3AValue.Value, RegSR3AMask.Value);
        DPHVSyncRegValue.VSYNC_Polarity = 1;
    }
    DPHVSyncRegMask.Value = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_HV_SYNC[DPModuleIndex], DPHVSyncRegValue.Value, DPHVSyncRegMask.Value);

    // MM825C, DP H/V display attribute data
    DPHVActiveRegValue.Acitve_Width = pTiming->HorDisEnd;
    DPHVActiveRegValue.Active_Height = pTiming->VerDisEnd;
    DPHVActiveRegMask.Value = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_HV_ACTIVE[DPModuleIndex], DPHVActiveRegValue.Value, DPHVActiveRegMask.Value);

    // MM8210, DP Nvid attribute data, N=32768
    DPMisc1RegValue.Value = 0;
    DPMisc1RegValue.NVID = 0x008000;
    DPMisc1RegValue.MISC1_Even = 0;
    DPMisc1RegValue.MISC1_Stereo_Video = 0;
    DPMisc1RegValue.MISC1_Reserved = 0;
    DPMisc1RegMask.Value = 0xFFFFFFFF;
    DPMisc1RegMask.NVID = 0;
    DPMisc1RegMask.MISC1_Even = 0;
    DPMisc1RegMask.MISC1_Stereo_Video = 0;
    DPMisc1RegMask.MISC1_Reserved = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_MISC1[DPModuleIndex], DPMisc1RegValue.Value, DPMisc1RegMask.Value);

    // MM8218, DP input and general control
    DPGenCtrlRegValue.Value = 0;
    DPGenCtrlRegValue.DELAY = 0x20;
    DPGenCtrlRegMask.Value = 0xFFFFFFFF;
    DPGenCtrlRegMask.DELAY = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_GEN_CTRL[DPModuleIndex], DPGenCtrlRegValue.Value, DPGenCtrlRegMask.Value);

    bStatus = CBIOS_TRUE;

    cbTraceExit(DP);
Exit:
    return bStatus;
}

CBIOS_VOID cbDIU_DP_SendInfoFrame(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_U32 Length)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32  StartAddress = 0;
    REG_MM8240 DPEnableInfoFrameRegValue, DPEnableInfoFrameRegMask;
    REG_MM8338 DPMuteRegValue, DPMuteRegMask;

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return;
    }

    // InfoFrame from FIFO
    DPMuteRegValue.Value = 0;
    DPMuteRegValue.Audio_InfoFrame = 0;
    DPMuteRegMask.Value = 0xFFFFFFFF;
    DPMuteRegMask.Audio_InfoFrame = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_MUTE[DPModuleIndex], DPMuteRegValue.Value, DPMuteRegMask.Value);

    DPEnableInfoFrameRegValue.Value = 0;
    DPEnableInfoFrameRegValue.InfoFrame_FIFO_Select = 0; // select FIFO 1
    DPEnableInfoFrameRegValue.InfoFrame_FIFO_1_Ready = 1;
    DPEnableInfoFrameRegValue.InfoFrame_FIFO_1_Start_Address = StartAddress;
    DPEnableInfoFrameRegValue.InfoFrame_FIFO_1_Length = Length - 1;
    DPEnableInfoFrameRegMask.Value = 0xFFFFFFFF;
    DPEnableInfoFrameRegMask.InfoFrame_FIFO_Select = 0;
    DPEnableInfoFrameRegMask.InfoFrame_FIFO_1_Ready = 0;
    DPEnableInfoFrameRegMask.InfoFrame_FIFO_1_Start_Address = 0;
    DPEnableInfoFrameRegMask.InfoFrame_FIFO_1_Length = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_ENABLE[DPModuleIndex], DPEnableInfoFrameRegValue.Value, DPEnableInfoFrameRegMask.Value);
}

/***************************************************************************************/
/*******************************  Aux Channel Interfaces  ******************************/
CBIOS_VOID cbDIU_DP_ResetAUX(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8214    DPLinkRegValue, DPLinkRegMask;
    REG_MM8338    DPMuteRegValue, DPMuteRegMask;

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return;
    }

    DPMuteRegValue.Value = 0;
    DPMuteRegMask.Value = 0xFFFFFFFF;
    DPMuteRegMask.AUX_SW_Reset = 0;

    DPMuteRegValue.AUX_SW_Reset = 1;
    cbMMIOWriteReg32(pcbe, DP_REG_MUTE[DPModuleIndex], DPMuteRegValue.Value, DPMuteRegMask.Value);

    cb_DelayMicroSeconds(1000); // wait AUX Reset to be done

    DPMuteRegValue.AUX_SW_Reset = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_MUTE[DPModuleIndex], DPMuteRegValue.Value, DPMuteRegMask.Value);

    DPLinkRegValue.Value = 0;
    DPLinkRegMask.Value = 0xFFFFFFFF;
    DPLinkRegMask.SW_Hpd_assert = 0;

    DPLinkRegValue.SW_Hpd_assert = 1;
    cbMMIOWriteReg32(pcbe, DP_REG_LINK[DPModuleIndex], DPLinkRegValue.Value, DPLinkRegMask.Value);

    DPLinkRegValue.SW_Hpd_assert = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_LINK[DPModuleIndex], DPLinkRegValue.Value, DPLinkRegMask.Value);

    cb_DelayMicroSeconds(200);
}

static CBIOS_VOID cbDIU_DP_ClearAuxReadBuffer(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX DPModuleIndex)
{
    REG_MM8320    DPAuxRead0RegValue;
    REG_MM8324    DPAuxRead1RegValue;
    REG_MM8328    DPAuxRead2RegValue;
    REG_MM832C    DPAuxRead3RegValue;

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return;
    }

    DPAuxRead0RegValue.Value = 0;
    DPAuxRead0RegValue.AUX_Read_Bytes_3_0 = 0;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_READ0[DPModuleIndex], DPAuxRead0RegValue.Value);
    DPAuxRead1RegValue.Value = 0;
    DPAuxRead1RegValue.AUX_Read_Bytes_7_4 = 0;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_READ1[DPModuleIndex], DPAuxRead1RegValue.Value);
    DPAuxRead2RegValue.Value = 0;
    DPAuxRead2RegValue.AUX_Read_Bytes_11_8 = 0;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_READ2[DPModuleIndex], DPAuxRead2RegValue.Value);
    DPAuxRead3RegValue.Value = 0;
    DPAuxRead3RegValue.AUX_Read_Bytes_15_12 = 0;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_READ3[DPModuleIndex], DPAuxRead3RegValue.Value);
}

static CBIOS_VOID cbDIU_DP_ClearAuxWriteBuffer(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX DPModuleIndex)
{
    REG_MM8310    DPAuxWrite0RegValue;
    REG_MM8314    DPAuxWrite1RegValue;
    REG_MM8318    DPAuxWrite2RegValue;
    REG_MM831C    DPAuxWrite3RegValue;

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return;
    }

    DPAuxWrite0RegValue.Value = 0;
    DPAuxWrite0RegValue.AUX_Write_Bytes_3_0 = 0;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_WRITE0[DPModuleIndex], DPAuxWrite0RegValue.Value);
    DPAuxWrite1RegValue.Value = 0;
    DPAuxWrite1RegValue.AUX_Write_Bytes_7_4 = 0;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_WRITE1[DPModuleIndex], DPAuxWrite1RegValue.Value);
    DPAuxWrite2RegValue.Value = 0;
    DPAuxWrite2RegValue.AUX_Write_Bytes_11_8 = 0;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_WRITE2[DPModuleIndex], DPAuxWrite2RegValue.Value);
    DPAuxWrite3RegValue.Value = 0;
    DPAuxWrite3RegValue.AUX_Write_Bytes_15_12 = 0;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_WRITE3[DPModuleIndex], DPAuxWrite3RegValue.Value);
}

static CBIOS_VOID cbDIU_DP_SWAuxRequest(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX DPModuleIndex)
{
    REG_MM8330    DPAuxTimerRegValue, DPAuxTimerRegMask;

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return;
    }

    DPAuxTimerRegValue.Value = 0;
    DPAuxTimerRegValue.SW_AUX = 1;
    DPAuxTimerRegValue.AUX_Request = 1;
    DPAuxTimerRegValue.AUX_DRDY = 0;
    DPAuxTimerRegValue.AUX_Timeout = 0;
    DPAuxTimerRegMask.Value = 0xFFFFFFFF;
    DPAuxTimerRegMask.SW_AUX = 0;
    DPAuxTimerRegMask.AUX_Request = 0;
    DPAuxTimerRegMask.AUX_DRDY = 0;
    DPAuxTimerRegMask.AUX_Timeout = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_AUX_TIMER[DPModuleIndex], DPAuxTimerRegValue.Value, DPAuxTimerRegMask.Value);
}

CBIOS_VOID cbDIU_DP_HWUseAuxChannel(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8330    DPAuxTimerRegValue, DPAuxTimerRegMask;

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return;
    }

    DPAuxTimerRegValue.Value = 0;
    DPAuxTimerRegValue.SW_AUX = 0;
    DPAuxTimerRegValue.AUX_Request = 0;
    DPAuxTimerRegValue.AUX_DRDY = 0;
    DPAuxTimerRegValue.AUX_Timeout = 0;

    DPAuxTimerRegMask.Value = 0xFFFFFFFF;
    DPAuxTimerRegMask.SW_AUX = 0;
    DPAuxTimerRegMask.AUX_Request = 0;
    DPAuxTimerRegMask.AUX_DRDY = 0;
    DPAuxTimerRegMask.AUX_Timeout = 0;

    cbMMIOWriteReg32(pcbe, DP_REG_AUX_TIMER[DPModuleIndex], DPAuxTimerRegValue.Value, DPAuxTimerRegMask.Value);
}

// IsNative = 1: native aux read/write
// IsNative = 0: I2C over aux read/write
static CBIOS_BOOL cbDIU_DP_CheckAuxReplyStatus(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_BOOL IsNative)
{
    // 20090326_BC
    // 4.2.1.2 will not send AUX STOP so we won't get AUX ready usually.
    // But sometimes, after ~600us, HW would set AUX ready with timeout bit.
    // For the EDID read intermittent error cited below we cannot rely on timeout bit checking.
    // To workaround this problem, reduce counter from 13 to 11.
    // So resend the request after 550us instead of 650us.
    CBIOS_U32     i, j, counter;
    CBIOS_BOOL    bStatus = CBIOS_FALSE;
    REG_MM8330    DPAuxTimerRegValue;

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return  CBIOS_FALSE;
    }

    //increase the waiting time in case EDID can't be read correctly
    //now SW will resend the request after 10ms
    counter = IsNative ? 10 : 100;

    for(i = 0; i < 10; i++)
    {
        for (j = 1; j < counter; j++)
        {
            cb_DelayMicroSeconds(100);
            // Need to check timeout (bit 3) as well
            // But when reading EDID sometime, timeout got set but the correct data did arrive,
            // making us throwing away good data and end up with bad checksum.
            // Going back to old method (no timeout check).
            DPAuxTimerRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_TIMER[DPModuleIndex]) & 0x3C000007;

            // [29:26]: Reply AUX command status
            // [3:0]: AUX command status
            if (DPAuxTimerRegValue.SW_AUX && DPAuxTimerRegValue.AUX_DRDY)
            {
                if ((DPAuxTimerRegValue.AUX_Command == CBIOS_AUX_REPLY_ACK) ||
                    (DPAuxTimerRegValue.AUX_Command == CBIOS_AUX_REPLY_NACK))
                {
                    if (DPAuxTimerRegValue.AUX_Command == CBIOS_AUX_REPLY_NACK)
                    {
                        cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: Sink send NACK reply, just abort current aux transaction\n", FUNCTION_NAME));
                    }

                    bStatus = CBIOS_TRUE;
                    break;
                }
            }
        }

        if (bStatus)
        {
            break;
        }
        else
        {

#if DP_RESET_AUX_WHEN_DEFER
            cbDIU_DP_ResetAUX(pcbe, DPModuleIndex);
#endif
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: Failed! i=%d, j=%d, mm%x=0x%08x\n", FUNCTION_NAME,
                          i, j, DP_REG_AUX_TIMER[DPModuleIndex], DPAuxTimerRegValue.Value));

            // I2C over AUX channel is much slower than native AUX channel transaction
            // I2C R/W over AUX channel defer case
            if (DPAuxTimerRegValue.AUX_Command == CBIOS_AUX_REPLY_DEFER)
            {
                cbDelayMilliSeconds(1);
            }

            // Can satisfy 4.2.1.1 &4.2.1.2 compliance test cause 600 us has passed
            cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
        }
    }

    return bStatus;
}

CBIOS_U32 cbDIU_DP_AuxReadEDID(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, PCBIOS_UCHAR pEDIDBuffer, CBIOS_U32 ulBufferSize)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_BOOL              bStatus = CBIOS_FALSE;
    CBIOS_U32               i, j;
    CBIOS_U32               dTemp, ulEdidLength = 0;
    CBIOS_UCHAR             ucChecksum;
    REG_MM8334              DPAuxCmdRegValue;
    AUX_CONTROL             AUX;
    DPCD_REG_00260          DPCD_00260;
    DPCD_REG_00261          DPCD_00261;
    CBIOS_U32               EdidBlockNum = 0;
    const CBIOS_U32         EdidBlockBufferSize = 256;
    PCBIOS_UCHAR            pEdidTempBuffer = pEDIDBuffer;

    cbTraceEnter(DP);

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return  0;
    }

    // AUX write, 0 byte, address 050h (A0h >> 1), address transaction only
    DPAuxCmdRegValue.Value = 0;
    DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_WRITE | CBIOS_AUX_REQUEST_I2C_MOT;
    DPAuxCmdRegValue.SW_AUX_Length = 0;
    DPAuxCmdRegValue.SW_AUX_Addr = 0x00050;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);
    cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
    if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
        goto exitAuxReadEDID;

    // AUX write, 1 byte, address 050h (A0h >> 1), set EDID offset 0
    cbDIU_DP_ClearAuxWriteBuffer(pcbe, DPModuleIndex);
    DPAuxCmdRegValue.Value = 0;
    DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_WRITE | CBIOS_AUX_REQUEST_I2C_MOT;
    DPAuxCmdRegValue.SW_AUX_Length = 1;
    DPAuxCmdRegValue.SW_AUX_Addr = 0x00050;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);
    cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
    if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
        goto exitAuxReadEDID;

    // Parade DP RX can't trasnmit more than 7 bytes at a time... Be reminded!
    // AUX read, 0 byte, I2C read repeated start
    DPAuxCmdRegValue.Value = 0;
    DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_READ | CBIOS_AUX_REQUEST_I2C_MOT;
    DPAuxCmdRegValue.SW_AUX_Length = 0;
    DPAuxCmdRegValue.SW_AUX_Addr = 0x00050;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);
    cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
    if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
        goto exitAuxReadEDID;

    // AUX I2C read, 2 bytes, address 050h (A0h >> 1)
    DPAuxCmdRegValue.Value = 0;
    DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_READ | CBIOS_AUX_REQUEST_I2C_MOT;
    DPAuxCmdRegValue.SW_AUX_Length = 2;
    DPAuxCmdRegValue.SW_AUX_Addr = 0x00050;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);

    ucChecksum = 0;
    for (i = 0; i < EdidBlockBufferSize / 128; i++)
    {
        for (j = 0; j < 128 / 2; j ++)
        {
            cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
            if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
            {
                cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: Read EDID 0x%x failed!\n", FUNCTION_NAME, 2*j));
            }
            dTemp = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_READ0[DPModuleIndex]);

            ucChecksum += pEDIDBuffer[i * 128 + j * 2 + 0] = (CBIOS_U8) (dTemp >> 0);
            ucChecksum += pEDIDBuffer[i * 128 + j * 2 + 1] = (CBIOS_U8) (dTemp >> 8);
        }

        ulEdidLength += 128;

        // Must check checksum before check extension flag in case EDID is corrupted
        if (ucChecksum == 0)
        {
            if (ulEdidLength == (pEDIDBuffer[0x7E] + 1) * 128) // Extension flag
            {
                bStatus = CBIOS_TRUE;
                break;
            }
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: checksum == 0x%02x, wrong!!\n", FUNCTION_NAME, ucChecksum));
            bStatus = CBIOS_FALSE;
            if (ulEdidLength == (pEDIDBuffer[0x7E] + 1) * 128) // Extension flag
            {
                break;
            }
        }
    }

    // AUX read, 0 byte, without MOT => I2C stop
    DPAuxCmdRegValue.Value = 0;
    DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_READ;
    DPAuxCmdRegValue.SW_AUX_Length = 0;
    DPAuxCmdRegValue.SW_AUX_Addr = 0x00050;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);
    cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
    if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: Send I2C STOP failed!\n", FUNCTION_NAME));
        bStatus = CBIOS_TRUE;
        goto exitAuxReadEDID;
    }

    // if EdidBlockNum > 2, read the rest edid data.
    EdidBlockNum = pEDIDBuffer[0x7E] + 1;
    pEdidTempBuffer = pEDIDBuffer + EdidBlockBufferSize;
    if (EdidBlockNum > 2)
    {
        // AUX write, 0 byte, address 030h (60h >> 1), address transaction only
        DPAuxCmdRegValue.Value = 0;
        DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_WRITE | CBIOS_AUX_REQUEST_I2C_MOT;
        DPAuxCmdRegValue.SW_AUX_Length = 0;
        DPAuxCmdRegValue.SW_AUX_Addr = 0x00030;
        cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);
        cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
        if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
            goto exitAuxReadEDID;

        // AUX write, 1 byte, address 030h (60h >> 1), set EDID segment num 1
        cbDIU_DP_ClearAuxWriteBuffer(pcbe, DPModuleIndex);
        DPAuxCmdRegValue.Value = 0;
        DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_WRITE | CBIOS_AUX_REQUEST_I2C_MOT;
        DPAuxCmdRegValue.SW_AUX_Length = 1;
        DPAuxCmdRegValue.SW_AUX_Addr = 0x00030;
        cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);
        cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_WRITE0[DPModuleIndex], 1);
        cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
        if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
            goto exitAuxReadEDID;

        // AUX write, 0 byte, address 050h (A0h >> 1), address transaction only
        DPAuxCmdRegValue.Value = 0;
        DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_WRITE | CBIOS_AUX_REQUEST_I2C_MOT;
        DPAuxCmdRegValue.SW_AUX_Length = 0;
        DPAuxCmdRegValue.SW_AUX_Addr = 0x00050;
        cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);
        cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
        if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
            goto exitAuxReadEDID;

        // AUX write, 1 byte, address 050h (A0h >> 1), set EDID offset 0
        cbDIU_DP_ClearAuxWriteBuffer(pcbe, DPModuleIndex);
        DPAuxCmdRegValue.Value = 0;
        DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_WRITE | CBIOS_AUX_REQUEST_I2C_MOT;
        DPAuxCmdRegValue.SW_AUX_Length = 1;
        DPAuxCmdRegValue.SW_AUX_Addr = 0x00050;
        cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);
        cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
        if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
            goto exitAuxReadEDID;

        // Parade DP RX can't trasnmit more than 7 bytes at a time... Be reminded!
        // AUX read, 0 byte, I2C read repeated start
        DPAuxCmdRegValue.Value = 0;
        DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_READ | CBIOS_AUX_REQUEST_I2C_MOT;
        DPAuxCmdRegValue.SW_AUX_Length = 0;
        DPAuxCmdRegValue.SW_AUX_Addr = 0x00050;
        cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);
        cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
        if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
            goto exitAuxReadEDID;

        // AUX I2C read, 2 bytes, address 050h (A0h >> 1)
        DPAuxCmdRegValue.Value = 0;
        DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_READ | CBIOS_AUX_REQUEST_I2C_MOT;
        DPAuxCmdRegValue.SW_AUX_Length = 2;
        DPAuxCmdRegValue.SW_AUX_Addr = 0x00050;
        cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);

        ucChecksum = 0;
        for (i = 0; i < EdidBlockBufferSize / 128; i++)
        {
            for (j = 0; j < 128 / 2; j ++)
            {
                cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
                if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
                {
                    cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: Read EDID 0x%x failed!\n", FUNCTION_NAME, 2*j));
                }
                dTemp = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_READ0[DPModuleIndex]);

                ucChecksum += pEdidTempBuffer[i * 128 + j * 2 + 0] = (CBIOS_U8) (dTemp >> 0);
                ucChecksum += pEdidTempBuffer[i * 128 + j * 2 + 1] = (CBIOS_U8) (dTemp >> 8);
            }

            ulEdidLength += 128;

            // Must check checksum before check extension flag in case EDID is corrupted
            if (ucChecksum == 0)
            {
                if (ulEdidLength == (pEDIDBuffer[0x7E] + 1) * 128) // Extension flag
                {
                    bStatus = CBIOS_TRUE;
                    break;
                }
            }
            else
            {
                cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: checksum == 0x%02x, wrong!!\n", FUNCTION_NAME, ucChecksum));
                bStatus = CBIOS_FALSE;
                if (ulEdidLength == (pEDIDBuffer[0x7E] + 1) * 128) // Extension flag
                {
                    break;
                }
            }
        }

        // AUX read, 0 byte, without MOT => I2C stop
        DPAuxCmdRegValue.Value = 0;
        DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_READ;
        DPAuxCmdRegValue.SW_AUX_Length = 0;
        DPAuxCmdRegValue.SW_AUX_Addr = 0x00050;
        cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);
        cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
        if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
        {
            cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: Send I2C STOP failed!\n", FUNCTION_NAME));
            bStatus = CBIOS_TRUE;
            goto exitAuxReadEDID;
        }
    }

    // For CTS EDID read test item: 4.2.2.3
    // Should always check the checksum of last edid block according to linklayer compliance test spec.
    DPCD_00260.Value = 0;
    DPCD_00260.TEST_EDID_CHECKSUM_WRITE = 1;
    DPCD_00261.Value = 0;
    DPCD_00261.TEST_EDID_CHECKSUM = pEDIDBuffer[ulEdidLength - 1];

    AUX.Function = CBIOS_AUX_REQUEST_NATIVE_WRITE;
    AUX.Offset = 0x260;
    AUX.Length = 0x2;
    AUX.Data[0] = (DPCD_00261.Value << 8) | DPCD_00260.Value;
    if(!cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: Write EDID checksum to TEST_RESPONSE and TEST_EDID_CHECKSUM fields of DPCD failed!\n", FUNCTION_NAME));
    }

exitAuxReadEDID:
    cbTraceExit(DP);
    return bStatus ? ulEdidLength : 0;
}

CBIOS_BOOL cbDIU_DP_AuxReadEDIDOffset(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, PCBIOS_UCHAR pEDIDBuffer, CBIOS_U32 ulBufferSize, CBIOS_U32 ulReadEdidOffset)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_BOOL              bStatus = CBIOS_FALSE;
    CBIOS_U32               i = 0;
    REG_MM8334              DPAuxCmdRegValue;

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return  CBIOS_FALSE;
    }

    //AUX write, 0 byte, address 050h (A0h >> 1), address transaction only
    DPAuxCmdRegValue.Value = 0;
    DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_WRITE | CBIOS_AUX_REQUEST_I2C_MOT;;
    DPAuxCmdRegValue.SW_AUX_Length = 0;
    DPAuxCmdRegValue.SW_AUX_Addr = 0x00050;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);
    cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
    if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
    {
        bStatus = CBIOS_FALSE;
        goto ExitFunc;
    }

    cbDIU_DP_ClearAuxWriteBuffer(pcbe, DPModuleIndex);
    DPAuxCmdRegValue.Value = 0;
    DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_WRITE | CBIOS_AUX_REQUEST_I2C_MOT;
    DPAuxCmdRegValue.SW_AUX_Length = 1;
    DPAuxCmdRegValue.SW_AUX_Addr = 0x00050;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_WRITE0[DPModuleIndex], ulReadEdidOffset);
    cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
    if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
    {
        bStatus = CBIOS_FALSE;
        goto ExitFunc;
    }

    // Parade DP RX can't trasnmit more than 7 bytes at a time... Be reminded!
    // AUX read, 0 byte, I2C read repeated start
    DPAuxCmdRegValue.Value = 0;
    DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_READ | CBIOS_AUX_REQUEST_I2C_MOT;
    DPAuxCmdRegValue.SW_AUX_Length = 0;
    DPAuxCmdRegValue.SW_AUX_Addr = 0x00050;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);
    cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
    if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
    {
        bStatus = CBIOS_FALSE;
        goto ExitFunc;
    }

    // AUX I2C read, 1 bytes, address 050h (A0h >> 1)
    DPAuxCmdRegValue.Value = 0;
    DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_READ | CBIOS_AUX_REQUEST_I2C_MOT;
    DPAuxCmdRegValue.SW_AUX_Length = 1;
    DPAuxCmdRegValue.SW_AUX_Addr = 0x00050;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);

    bStatus = CBIOS_TRUE;
    for (i = 0; i < ulBufferSize; i++)
    {
        cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
        if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
        {
            cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: Read EDID %d failed!\n", FUNCTION_NAME, i + ulReadEdidOffset));
            bStatus = CBIOS_FALSE;
            // AUX read, 0 byte, without MOT => I2C stop
            DPAuxCmdRegValue.Value = 0;
            DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_READ;
            DPAuxCmdRegValue.SW_AUX_Length = 0;
            DPAuxCmdRegValue.SW_AUX_Addr = 0x00050;
            cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);
            cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
            goto ExitFunc;

        }

        pEDIDBuffer[i] = (CBIOS_U8)cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_READ0[DPModuleIndex]);
    }

    // AUX read, 0 byte, without MOT => I2C stop
    DPAuxCmdRegValue.Value = 0;
    DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_READ;
    DPAuxCmdRegValue.SW_AUX_Length = 0;
    DPAuxCmdRegValue.SW_AUX_Addr = 0x00050;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);
    cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
    if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: Send I2C STOP failed!\n", FUNCTION_NAME));
        bStatus = CBIOS_TRUE;
        goto ExitFunc;
    }

ExitFunc:

    return bStatus;
}

static CBIOS_BOOL cbDIU_DP_WaitAuxReady(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX DPModuleIndex)
{
    CBIOS_U32     i = 0;
    REG_MM8330    DPAuxTimerRegValue;

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return  CBIOS_FALSE;
    }

    DPAuxTimerRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_TIMER[DPModuleIndex]);

    if ((DPAuxTimerRegValue.HPD_Status == 0) || (DPAuxTimerRegValue.HPD_Status == 2)) // unplug
    {
        cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: Aux can't work when DP is unplugged, mm%x==%08x!!!\n",
            FUNCTION_NAME, DP_REG_AUX_TIMER[DPModuleIndex], DPAuxTimerRegValue.Value));

        return CBIOS_FALSE;
    }

    while ((DPAuxTimerRegValue.AUX_Request) || // Data not sent yet
           (((DPAuxTimerRegValue.Value & 0xE) != 0x0) && (!(DPAuxTimerRegValue.Value& 0xC)))) // If not the first aux cmd, data should be ready or time out
    {
        cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: Aux channel is busy, mm%x==%08x, wait 100us!\n",
            FUNCTION_NAME, DP_REG_AUX_TIMER[DPModuleIndex], DPAuxTimerRegValue.Value));
        i++;
        if (i > 15) // 1.5ms time out
        {
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: Aux channel is busy for 15ms, time out!\n", FUNCTION_NAME));
            return CBIOS_FALSE;
        }
        cb_DelayMicroSeconds(100);
        DPAuxTimerRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_TIMER[DPModuleIndex]);
    }

    return CBIOS_TRUE;
}

static CBIOS_BOOL cbDIU_DP_ReadDPCD(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 addr, CBIOS_U32 numData, CBIOS_MODULE_INDEX DPModuleIndex)
{
    CBIOS_BOOL    bStatus = CBIOS_FALSE;
    REG_MM8320    DPAuxRead0RegValue;
    REG_MM8324    DPAuxRead1RegValue;
    REG_MM8328    DPAuxRead2RegValue;
    REG_MM832C    DPAuxRead3RegValue;
    REG_MM8334    DPAuxCmdRegValue;

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return  CBIOS_FALSE;
    }

    if(!cbDIU_DP_WaitAuxReady(pcbe, DPModuleIndex))
    {
        return CBIOS_FALSE;
    }

    cbDIU_DP_ClearAuxReadBuffer(pcbe, DPModuleIndex);

    DPAuxCmdRegValue.Value = 0;
    DPAuxCmdRegValue.SW_AUX_Addr = addr;
    DPAuxCmdRegValue.SW_AUX_Length = numData;
    DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_NATIVE_READ;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);

    cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
    bStatus = cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_TRUE);

    DPAuxRead0RegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_READ0[DPModuleIndex]);
    DPAuxRead1RegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_READ1[DPModuleIndex]);
    DPAuxRead2RegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_READ2[DPModuleIndex]);
    DPAuxRead3RegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_READ3[DPModuleIndex]);
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: addr(0x%x), numData(%d), data-3_0, data-7_4, data-11_8, data-15_12 = %08x, %08x, %08x, %08x\n",
        FUNCTION_NAME, addr, numData, DPAuxRead0RegValue.Value, DPAuxRead1RegValue.Value, DPAuxRead2RegValue.Value, DPAuxRead3RegValue.Value));

    return bStatus;
}

static CBIOS_BOOL cbDIU_DP_WriteDPCD(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 addr, CBIOS_U32 numData, CBIOS_MODULE_INDEX DPModuleIndex)
{
    CBIOS_BOOL    bStatus = CBIOS_FALSE;
    REG_MM8310    DPAuxWrite0RegValue;
    REG_MM8314    DPAuxWrite1RegValue;
    REG_MM8334    DPAuxCmdRegValue;

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return  CBIOS_FALSE;
    }

    if(!cbDIU_DP_WaitAuxReady(pcbe, DPModuleIndex))
    {
        return CBIOS_FALSE;
    }

    DPAuxWrite0RegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_WRITE0[DPModuleIndex]);
    DPAuxWrite1RegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_WRITE1[DPModuleIndex]);
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: addr, numData, data3_0, data7_4 = %04x, %04x, %08x, %08x!!!\n",
        FUNCTION_NAME, addr, numData, DPAuxWrite0RegValue.Value, DPAuxWrite1RegValue.Value));

    DPAuxCmdRegValue.Value = 0;
    DPAuxCmdRegValue.SW_AUX_Addr = addr;
    DPAuxCmdRegValue.SW_AUX_Length = numData;
    DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_NATIVE_WRITE;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);

    cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
    bStatus = cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_TRUE);

    return bStatus;
}

static CBIOS_VOID cbDIU_DP_SetAuxWriteBuffer(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX DPModuleIndex, PAUX_CONTROL pAUX)
{
    CBIOS_U32     i = 0;
    REG_MM8310    DPAuxWrite0RegValue;
    REG_MM8314    DPAuxWrite1RegValue;
    REG_MM8318    DPAuxWrite2RegValue;
    REG_MM831C    DPAuxWrite3RegValue;

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return;
    }

    //  first clear write buffer
    cbDIU_DP_ClearAuxWriteBuffer(pcbe, DPModuleIndex);

    // Clear Data[i] over pAUX->Lenth
    for(i = pAUX->Length; i < 0x10; i++)
        *((PCBIOS_U8)(pAUX->Data) + i) = 0x00;

    DPAuxWrite0RegValue.Value = pAUX->Data[0];
    DPAuxWrite1RegValue.Value = pAUX->Data[1];
    DPAuxWrite2RegValue.Value = pAUX->Data[2];
    DPAuxWrite3RegValue.Value = pAUX->Data[3];

    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_WRITE0[DPModuleIndex], DPAuxWrite0RegValue.Value);
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_WRITE1[DPModuleIndex], DPAuxWrite1RegValue.Value);
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_WRITE2[DPModuleIndex], DPAuxWrite2RegValue.Value);
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_WRITE3[DPModuleIndex], DPAuxWrite3RegValue.Value);
}

static CBIOS_VOID cbDIU_DP_GetAuxReadBuffer(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX DPModuleIndex, PAUX_CONTROL pAUX)
{
    CBIOS_U32    i = 0;

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return;
    }

    pAUX->Data[0] = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_READ0[DPModuleIndex]);
    pAUX->Data[1] = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_READ1[DPModuleIndex]);
    pAUX->Data[2] = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_READ2[DPModuleIndex]);
    pAUX->Data[3] = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_READ3[DPModuleIndex]);

    // Clear Data[i] over pAUX->Lenth
    for(i = pAUX->Length; i < 0x10; i++)
    {
        *((PCBIOS_U8)(pAUX->Data) + i) = 0x00;
    }
}

static CBIOS_BOOL cbDIU_DP_AuxReplyStatus(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX DPModuleIndex)
{
    CBIOS_BOOL    bStatus = CBIOS_FALSE;
    REG_MM8330    DPAuxTimerRegValue;

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return  CBIOS_FALSE;
    }

    DPAuxTimerRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_TIMER[DPModuleIndex]);

    if(DPAuxTimerRegValue.AUX_DRDY)
        bStatus = CBIOS_TRUE;

    return bStatus;
}

CBIOS_BOOL cbDIU_DP_AuxChRW(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, PAUX_CONTROL pAUX)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_BOOL    bStatus = CBIOS_FALSE;
    CBIOS_U32     Retry = 0, MAX_RETRY_COUNT = 8;
    REG_MM82CC    DPEphyCtrlRegValue;
    REG_MM8334    DPAuxCmdRegValue;

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return  CBIOS_FALSE;
    }

    cbTraceEnter(DP);

    if(pAUX->Length > 0x10)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s : AUX Command Length > 0x10 error!!\n", FUNCTION_NAME));
        return CBIOS_FALSE;
    }

    DPEphyCtrlRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_EPHY_CTRL[DPModuleIndex]);
    if(!DPEphyCtrlRegValue.check_sync_cnt)//check 82cc Check_Sync_Cnt bit
    {
        DPEphyCtrlRegValue.check_sync_cnt = 1;//HW don't check sync counter for aux receiver
        cb_WriteU32(pcbe->pAdapterContext, DP_REG_EPHY_CTRL[DPModuleIndex], DPEphyCtrlRegValue.Value);
    }

    cbDIU_DP_ResetAUX(pcbe, (CBIOS_U8)DPModuleIndex);

    // AUX CH as I2C transaction
    if(pAUX->Function == CBIOS_AUX_REQUEST_I2C_READ || pAUX->Function == CBIOS_AUX_REQUEST_I2C_WRITE)
    {
        while(bStatus == CBIOS_FALSE && Retry <= MAX_RETRY_COUNT)
        {
            DPAuxCmdRegValue.Value = 0;
            DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_WRITE | CBIOS_AUX_REQUEST_I2C_MOT;
            DPAuxCmdRegValue.SW_AUX_Length = 0;
            DPAuxCmdRegValue.SW_AUX_Addr = (pAUX->I2cPort >> 1) & 0xFF; // I2C slave address
            cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value); // write command to command buffer
            cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_WRITE0[DPModuleIndex], pAUX->Offset);

            cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
            bStatus = cbDIU_DP_AuxReplyStatus(pcbe, DPModuleIndex);

            // write I2C offset
            DPAuxCmdRegValue.Value = 0;
            DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_WRITE | CBIOS_AUX_REQUEST_I2C_MOT; // I2C R/W command;
            DPAuxCmdRegValue.SW_AUX_Length = 0x1;                       // I2C offset length
            DPAuxCmdRegValue.SW_AUX_Addr = (pAUX->I2cPort >> 1) & 0xFF; // I2C slave address
            cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value); // write command to command buffer
            cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_WRITE0[DPModuleIndex], pAUX->Offset);

            cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
            bStatus = cbDIU_DP_AuxReplyStatus(pcbe, DPModuleIndex);

            DPAuxCmdRegValue.Value = 0;
            DPAuxCmdRegValue.SW_AUX_CMD = pAUX->Function;
            DPAuxCmdRegValue.SW_AUX_Length = pAUX->Function;
            DPAuxCmdRegValue.SW_AUX_Addr = (pAUX->I2cPort >> 1) & 0xFF; // I2C slave address

            if(pAUX->Function == CBIOS_AUX_REQUEST_I2C_WRITE)
                cbDIU_DP_SetAuxWriteBuffer(pcbe, DPModuleIndex, pAUX);

            cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value); // write command to command buffer

            cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
            bStatus = cbDIU_DP_AuxReplyStatus(pcbe, DPModuleIndex);
            Retry++;
        }

        if(pAUX->Function == CBIOS_AUX_REQUEST_I2C_READ)
            cbDIU_DP_GetAuxReadBuffer(pcbe, DPModuleIndex, pAUX);
    }
    else // AUX CH
    {
        if (pAUX->Function == CBIOS_AUX_REQUEST_NATIVE_READ)
        {
            bStatus = cbDIU_DP_ReadDPCD(pcbe, pAUX->Offset, pAUX->Length, DPModuleIndex);
            cbDIU_DP_GetAuxReadBuffer(pcbe, DPModuleIndex, pAUX);
        }
        else if (pAUX->Function == CBIOS_AUX_REQUEST_NATIVE_WRITE)
        {
            cbDIU_DP_SetAuxWriteBuffer(pcbe, DPModuleIndex, pAUX);
            bStatus = cbDIU_DP_WriteDPCD(pcbe, pAUX->Offset, pAUX->Length, DPModuleIndex);
        }
    }

    // return AUX CH control to HW
    cbDIU_DP_HWUseAuxChannel(pcbe, DPModuleIndex);
    cbTraceExit(DP);

    return bStatus;
}

CBIOS_STATUS cbDIU_DP_ReadData(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams, CBIOS_U32 Flags, CBIOS_BOOL IsDDC)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32     i, ulLength = 0;
    CBIOS_STATUS  bStatus = CBIOS_ER_INTERNAL;
    CBIOS_U8      SlaveAddress = pCBModuleI2CParams->SlaveAddress;
    CBIOS_U32     BufferLen = pCBModuleI2CParams->BufferLen;
    CBIOS_U8*     buffer = pCBModuleI2CParams->Buffer;
    CBIOS_U32     ulReadEdidOffset = pCBModuleI2CParams->OffSet;
    REG_MM8334    DPAuxCmdRegValue;

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return  CBIOS_ER_INVALID_PARAMETER;
    }

    // DP MCCS, need to follow I2C-over-AUX syntax
    cbDIU_DP_ClearAuxReadBuffer(pcbe, DPModuleIndex);
    SlaveAddress >>= 1;

    if(!IsDDC)
    {
        //AUX write, 0 byte, address 050h (A0h >> 1), address transaction only
        DPAuxCmdRegValue.Value = 0;
        DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_WRITE | CBIOS_AUX_REQUEST_I2C_MOT;
        DPAuxCmdRegValue.SW_AUX_Length = 0;
        DPAuxCmdRegValue.SW_AUX_Addr = SlaveAddress;
        cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);
        cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
        if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
            goto ExitAuxDdcRead;
        // Transmit
        cbDIU_DP_ClearAuxWriteBuffer(pcbe, DPModuleIndex);
        DPAuxCmdRegValue.Value = 0;
        DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_WRITE | CBIOS_AUX_REQUEST_I2C_MOT;
        DPAuxCmdRegValue.SW_AUX_Length = 1;
        DPAuxCmdRegValue.SW_AUX_Addr = SlaveAddress;
        cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);
        cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_WRITE0[DPModuleIndex], ulReadEdidOffset);
        cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
        if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
            goto ExitAuxDdcRead;
    }

    // 1. Start, 0 byte, adress transaction only
    DPAuxCmdRegValue.Value = 0;
    DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_READ | CBIOS_AUX_REQUEST_I2C_MOT;
    DPAuxCmdRegValue.SW_AUX_Length = 0;
    DPAuxCmdRegValue.SW_AUX_Addr = SlaveAddress;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);

    cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
    if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
        goto ExitAuxDdcRead;

    // 2. Receive every Byte
    DPAuxCmdRegValue.Value = 0;
    DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_READ | CBIOS_AUX_REQUEST_I2C_MOT;
    DPAuxCmdRegValue.SW_AUX_Length = 1;
    DPAuxCmdRegValue.SW_AUX_Addr = SlaveAddress;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);

    for (i = 0; i < BufferLen; i++)
    {
        cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
        if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
            goto ExitAuxDdcRead;
        buffer[i] = (CBIOS_U8)cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_READ0[DPModuleIndex]);

        if ((i == 1) && (Flags & 0x000000001))
        {
            ulLength = buffer[i] & ~0x80;
            if (ulLength > (BufferLen - 3))
            {
                cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: Buffer is too small!\n", FUNCTION_NAME));
                bStatus = CBIOS_ER_BUFFER_TOO_SMALL;
                goto ExitAuxDdcRead;
            }
            BufferLen = ulLength + 3;
        }
    }

    // 3. Stop, 0 byte, adress transaction only
    DPAuxCmdRegValue.Value = 0;
    DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_READ;
    DPAuxCmdRegValue.SW_AUX_Length = 0;
    DPAuxCmdRegValue.SW_AUX_Addr = SlaveAddress;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);

    cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
    if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
        goto ExitAuxDdcRead;

    bStatus = CBIOS_OK;

ExitAuxDdcRead:
    cbDIU_DP_HWUseAuxChannel(pcbe, DPModuleIndex);
    return bStatus;
}

CBIOS_BOOL cbDIU_DP_WriteData(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams, CBIOS_BOOL IsDDC)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32     i;
    CBIOS_STATUS  bStatus = CBIOS_FALSE;
    CBIOS_U8      SlaveAddress = pCBModuleI2CParams->SlaveAddress;
    REG_MM8334    DPAuxCmdRegValue;
    CBIOS_U32     ulReadEdidOffset = pCBModuleI2CParams->OffSet;

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return  CBIOS_FALSE;
    }

    // DP MCCS, need to follow I2C-over-AUX syntax
    cbDIU_DP_ClearAuxWriteBuffer(pcbe, DPModuleIndex);
    SlaveAddress >>= 1;

    // 1. Start, 0 byte, adress transaction only
    DPAuxCmdRegValue.Value = 0;
    DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_WRITE | CBIOS_AUX_REQUEST_I2C_MOT;
    DPAuxCmdRegValue.SW_AUX_Length = 0;
    DPAuxCmdRegValue.SW_AUX_Addr = SlaveAddress;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);

    cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
    if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
        goto ExitAuxDdcWrite;

    // 2. Transmit every Byte
    DPAuxCmdRegValue.Value = 0;
    DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_WRITE | CBIOS_AUX_REQUEST_I2C_MOT;
    DPAuxCmdRegValue.SW_AUX_Length = 1;
    DPAuxCmdRegValue.SW_AUX_Addr = SlaveAddress;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);
    if(!IsDDC)
    {
        cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_WRITE0[DPModuleIndex], ulReadEdidOffset);
        cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
        if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
            goto ExitAuxDdcWrite;
    }
    for (i = 0; i < pCBModuleI2CParams->BufferLen; i++)
    {
        cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_WRITE0[DPModuleIndex], pCBModuleI2CParams->Buffer[i]);
        cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
        if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
            goto ExitAuxDdcWrite;
    }

    // 3. Stop, 0 byte, adress transaction only
    DPAuxCmdRegValue.Value = 0;
    DPAuxCmdRegValue.SW_AUX_CMD = CBIOS_AUX_REQUEST_I2C_WRITE;
    DPAuxCmdRegValue.SW_AUX_Length = 1;
    DPAuxCmdRegValue.SW_AUX_Addr = SlaveAddress;
    cb_WriteU32(pcbe->pAdapterContext, DP_REG_AUX_CMD[DPModuleIndex], DPAuxCmdRegValue.Value);

    cbDIU_DP_SWAuxRequest(pcbe, DPModuleIndex);
    if (!cbDIU_DP_CheckAuxReplyStatus(pcbe, DPModuleIndex, CBIOS_FALSE))
        goto ExitAuxDdcWrite;

    bStatus = CBIOS_TRUE;

ExitAuxDdcWrite:
    cbDIU_DP_HWUseAuxChannel(pcbe, DPModuleIndex);
    return bStatus;
}

CBIOS_BOOL cbDIU_DP_IsOn(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8240     DPEnableRegValue;
    CBIOS_BOOL     status = CBIOS_FALSE;

    if (DPModuleIndex == CBIOS_MODULE_INDEX_INVALID)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return status;
    }

    DPEnableRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_ENABLE[DPModuleIndex]);

    if(DPEnableRegValue.DP_Enable)
    {
        status = CBIOS_TRUE;
    }
    else
    {
        status = CBIOS_FALSE;
    }

    return status;
}
