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

#include "CBiosChipShare.h"
#include "CBiosDP.h"
#include "../../Hw/HwBlock/CBiosDIU_DP.h"
#include "../../Hw/HwBlock/CBiosDIU_HDCP.h"
#include "../../Hw/HwBlock/CBiosDIU_HDMI.h"
#include "../../Hw/CBiosHwShare.h"

PCBIOS_VOID cbDPPort_GetDPMonitorContext(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon)
{
    PCBIOS_DP_CONTEXT  pDpContext = container_of(pDevCommon, PCBIOS_DP_CONTEXT, Common);

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & ALL_DP_TYPES)))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return CBIOS_NULL;
    }
    if (pDpContext->Common.SupportMonitorType & (CBIOS_MONITOR_TYPE_DP | CBIOS_MONITOR_TYPE_PANEL))
    {
        return &(pDpContext->DPMonitorContext);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: wrong support monitor type 0x%x\n", FUNCTION_NAME, pDpContext->Common.SupportMonitorType));
        return CBIOS_NULL;
    }
}

PCBIOS_VOID cbDPPort_GetHDMIMonitorContext(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon)
{
    PCBIOS_DP_CONTEXT  pDpContext = container_of(pDevCommon, PCBIOS_DP_CONTEXT, Common);

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & ALL_DP_TYPES)))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return CBIOS_NULL;
    }
    if ((pDpContext->Common.CurrentMonitorType == CBIOS_MONITOR_TYPE_HDMI) || (pDpContext->Common.CurrentMonitorType == CBIOS_MONITOR_TYPE_DVI))
    {
        return &(pDpContext->HDMIMonitorContext);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: wrong monitor type 0x%x\n", FUNCTION_NAME, pDpContext->Common.CurrentMonitorType));
        return CBIOS_NULL;
    }
}

CBIOS_BOOL cbDPPort_IsDeviceInDpMode(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon)
{
    PCBIOS_DP_CONTEXT pDpContext = container_of(pDevCommon, PCBIOS_DP_CONTEXT, Common);
    CBIOS_BOOL isDpMode = CBIOS_FALSE;

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & ALL_DP_TYPES)))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return CBIOS_FALSE;
    }

    if (pDevCommon->DeviceType & ALL_DP_TYPES)
    {
        isDpMode = !pDpContext->DPPortParams.bDualMode;
    }
    else
    {
        isDpMode = CBIOS_FALSE;
    }

    return isDpMode;
}

CBIOS_STATUS cbDPPort_Isr(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_ISR_PARA pDPIsrPara)
{
    PCBIOS_DP_CONTEXT pDpContext = container_of(pDevCommon, PCBIOS_DP_CONTEXT, Common);

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & ALL_DP_TYPES)))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return CBIOS_ER_INVALID_PARAMETER;
    }

    return cbDPMonitor_Isr(pvcbe, &pDpContext->DPMonitorContext, pDPIsrPara);
}

CBIOS_BOOL cbDPPort_WorkThreadMainFunc(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_WORKTHREAD_PARA pDPWorkThreadPara)
{
    PCBIOS_DP_CONTEXT pDpContext = container_of(pDevCommon, PCBIOS_DP_CONTEXT, Common);

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & ALL_DP_TYPES)))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return CBIOS_FALSE;
    }

    return cbDPMonitor_WorkThreadMainFunc(pvcbe, &pDpContext->DPMonitorContext, pDPWorkThreadPara);
}

CBIOS_STATUS cbDPPort_SetNotifications(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_NOTIFICATIONS pDPNotifications)
{
    PCBIOS_DP_CONTEXT pDpContext = container_of(pDevCommon, PCBIOS_DP_CONTEXT, Common);

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & ALL_DP_TYPES)))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return CBIOS_ER_INVALID_PARAMETER;
    }

    return cbDPMonitor_SetNotifications(pvcbe, &pDpContext->DPMonitorContext, pDPNotifications);
}

CBIOS_STATUS  cbDPPort_GetInt(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_INT_PARA  pDPIntPara)
{
    PCBIOS_DP_CONTEXT pDpContext = container_of(pDevCommon, PCBIOS_DP_CONTEXT, Common);

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & ALL_DP_TYPES)))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return CBIOS_ER_INVALID_PARAMETER;
    }

    return  cbDPMonitor_GetInt(pvcbe, &pDpContext->DPMonitorContext, pDPIntPara);
}

CBIOS_STATUS  cbDPPort_HandleIrq(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_HANDLE_IRQ_PARA pDPHandleIrqPara)
{
    PCBIOS_DP_CONTEXT pDpContext = container_of(pDevCommon, PCBIOS_DP_CONTEXT, Common);

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & ALL_DP_TYPES)))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return CBIOS_ER_INVALID_PARAMETER;
    }

    return  cbDPMonitor_HandleIrq(pvcbe, &pDpContext->DPMonitorContext, pDPHandleIrqPara);
}

CBIOS_STATUS cbDPPort_GetCustomizedTiming(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_CUSTOMIZED_TIMING pDPCustomizedTiming)
{
    PCBIOS_DP_CONTEXT pDpContext = container_of(pDevCommon, PCBIOS_DP_CONTEXT, Common);

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & ALL_DP_TYPES)))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return CBIOS_ER_INVALID_PARAMETER;
    }

    return cbDPMonitor_GetCustomizedTiming(pvcbe, &pDpContext->DPMonitorContext, pDPCustomizedTiming);
}

static CBIOS_VOID cbDPPort_HwInit(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon)
{
    CBIOS_MODULE_INDEX DPModuleIndex = CBIOS_MODULE_INDEX_INVALID;

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & ALL_DP_TYPES)))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return;
    }
    DPModuleIndex = cbGetModuleIndex(pcbe, pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);

    if(!(cbDIU_DP_IsOn(pcbe, DPModuleIndex) || cbDIU_HDMI_IsOn(pcbe, DPModuleIndex)))
    {
        cbPHY_DP_InitEPHY(pcbe, DPModuleIndex);
        cbPHY_DP_DeInitEPHY(pcbe, DPModuleIndex);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: EPHY has been initialized for 0x%x!\n", FUNCTION_NAME, pDevCommon->DeviceType));
    }
}

static CBIOS_BOOL cbDPPort_DeviceDetect(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_BOOL bHardcodeDetected, CBIOS_U32 FullDetect)
{
    CBIOS_BOOL         bConnected = CBIOS_FALSE;
    PCBIOS_DP_CONTEXT  pDpContext = container_of(pDevCommon, PCBIOS_DP_CONTEXT, Common);
    CBIOS_MODULE_INDEX DPModuleIndex = CBIOS_MODULE_INDEX_INVALID;
    CBIOS_MODULE_INDEX HDMIModuleIndex = CBIOS_MODULE_INDEX_INVALID;
    DP_EPHY_MODE  Mode;

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & ALL_DP_TYPES)))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return CBIOS_FALSE;
    }

    DPModuleIndex = cbGetModuleIndex(pcbe, pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    HDMIModuleIndex = cbGetModuleIndex(pcbe, pDevCommon->DeviceType, CBIOS_MODULE_TYPE_HDMI);
    if(CBIOS_MODULE_INDEX_INVALID == DPModuleIndex)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "Invalid DP index in detect device 0x%x!\n", pDevCommon->DeviceType));
        return CBIOS_FALSE;
    }

    Mode = cbPHY_DP_GetEphyMode(pcbe, DPModuleIndex);

    pDpContext->DPPortParams.bDualMode = cbDualModeDetect(pcbe, pDevCommon);
    //Some monitors(like samsung UA40) can't get EDID by I2C after change brightness through DDX, and bDualMode is FALSE
    //So it has no chance to enter cbHDMIMonitor_Detect function to handle scramble, just clear it here when found bDualMode is FALSE
    if (!pDpContext->DPPortParams.bDualMode && CBIOS_MODULE_INDEX_INVALID != HDMIModuleIndex)
    {
        pDpContext->HDMIMonitorContext.ScramblingEnable = CBIOS_FALSE;
        cbDIU_HDMI_ConfigScrambling(pcbe, HDMIModuleIndex, CBIOS_FALSE);
    }

    if ((!bConnected) && (pDpContext->DPPortParams.bDualMode)
        && (pDpContext->Common.SupportMonitorType & (CBIOS_MONITOR_TYPE_HDMI | CBIOS_MONITOR_TYPE_DVI)))
    {
        bConnected = cbHDMIMonitor_Detect(pcbe, &pDpContext->HDMIMonitorContext, bHardcodeDetected, FullDetect);
        if (bConnected)
        {
            cbPHY_DP_SelectEphyMode(pcbe, DPModuleIndex, DP_EPHY_TMDS_MODE);
            pDpContext->DPPortParams.DPEphyMode = DP_EPHY_TMDS_MODE;
            cbDIU_DP_SetInterruptMode(pcbe, DPModuleIndex, CBIOS_FALSE);
        }
    }

#if DP_MONITOR_SUPPORT
    if ((!bConnected) && (!pDpContext->DPPortParams.bDualMode)
        && (pDpContext->Common.SupportMonitorType & (CBIOS_MONITOR_TYPE_DP | CBIOS_MONITOR_TYPE_PANEL)))
    {
        pDpContext->DPPortParams.bDualMode = CBIOS_FALSE;

        cbPHY_DP_SelectEphyMode(pcbe, DPModuleIndex, DP_EPHY_DP_MODE);

        bConnected = cbDPMonitor_Detect(pcbe, &pDpContext->DPMonitorContext, bHardcodeDetected, FullDetect);
        if (bConnected)
        {
            pDpContext->DPPortParams.DPEphyMode = DP_EPHY_DP_MODE;
            cbDIU_DP_SetInterruptMode(pcbe, DPModuleIndex, CBIOS_TRUE);
        }
    }
#endif

    if(!bConnected)
    {
        cbPHY_DP_SelectEphyMode(pcbe, DPModuleIndex, Mode);
    }

    if(bConnected)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Monitor(0x%x) on Device(0x%x) is detected!\n",
                     FUNCTION_NAME, pDevCommon->CurrentMonitorType, pDevCommon->DeviceType));
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Device(0x%x) is not detected!\n",
                     FUNCTION_NAME, pDevCommon->DeviceType));
    }

    return bConnected;
}

static CBIOS_VOID cbDPPort_OnOff(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_BOOL bOn)
{
    PCBIOS_DP_CONTEXT     pDpContext = container_of(pDevCommon, PCBIOS_DP_CONTEXT, Common);
    CBIOS_MODULE_INDEX    DPModuleIndex = CBIOS_MODULE_INDEX_INVALID;
    CBIOS_DISPLAY_SOURCE *pSource = CBIOS_NULL;
    CBIOS_BOOL            bDPMode = 0;

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & ALL_DP_TYPES)))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return;
    }

    DPModuleIndex = cbGetModuleIndex(pcbe, pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    if(CBIOS_MODULE_INDEX_INVALID == DPModuleIndex)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "Invalid DP index in onoff device 0x%x!\n", pDevCommon->DeviceType));
        return;
    }

    bDPMode = (pDpContext->DPPortParams.DPEphyMode == DP_EPHY_DP_MODE) ? CBIOS_TRUE : CBIOS_FALSE;
    pSource = &(pDevCommon->DispSource);

    if (pSource->bIsSrcChanged)
    {
        cbPHY_DP_SelectPhySource(pcbe, pDpContext->DPPortParams.DPEphyMode, pSource, pDpContext->Common.CurrentMonitorType);
        cbPathMgrSelectDIUSource(pcbe, pSource);

        // source select done, clear flag
        pSource->bIsSrcChanged = CBIOS_FALSE;
    }

    if(bOn)
    {
        cbPathMgrModuleOnOff(pcbe, pSource->ModulePath, CBIOS_TRUE);
        cbDIU_DP_DPModeEnable(pcbe, DPModuleIndex, bDPMode);
    }

    if(bDPMode)
    {
        if(bOn)
        {
            if ((pDpContext->Common.CurrentMonitorType == CBIOS_MONITOR_TYPE_DP) || (pDpContext->Common.CurrentMonitorType == CBIOS_MONITOR_TYPE_PANEL))
            {
            #if DP_MONITOR_SUPPORT
                cbPHY_DP_InitEPHY(pcbe, DPModuleIndex);
                cbDPMonitor_OnOff(pcbe, &pDpContext->DPMonitorContext, bOn);
            #endif
            }
        }
        else
        {
            if ((pDpContext->Common.CurrentMonitorType == CBIOS_MONITOR_TYPE_DP) || (pDpContext->Common.CurrentMonitorType == CBIOS_MONITOR_TYPE_PANEL))
            {
            #if DP_MONITOR_SUPPORT
                cbDPMonitor_OnOff(pcbe, &pDpContext->DPMonitorContext, bOn);
            #endif
            }
        }
    }
    else
    {
        if(bOn)
        {
            if ((pDpContext->Common.CurrentMonitorType == CBIOS_MONITOR_TYPE_HDMI) || (pDpContext->Common.CurrentMonitorType == CBIOS_MONITOR_TYPE_DVI))
            {
                cbHDMIMonitor_OnOff(pcbe, &pDpContext->HDMIMonitorContext, bOn);
                cbPHY_DP_InitEPHY(pcbe, DPModuleIndex);
                cbPHY_DP_DualModeOnOff(pcbe, DPModuleIndex, pDpContext->HDMIMonitorContext.HDMIClock, bOn);
            }
        }
        else
        {
            if ((pDpContext->Common.CurrentMonitorType == CBIOS_MONITOR_TYPE_HDMI) || (pDpContext->Common.CurrentMonitorType == CBIOS_MONITOR_TYPE_DVI))
            {
                //Wait vblank before turning off device to avoid flashing white garbage
                cbWaitNonFullVBlank(pcbe, DPModuleIndex);
                cbHDMIMonitor_OnOff(pcbe, &pDpContext->HDMIMonitorContext, bOn);
                cbPHY_DP_DualModeOnOff(pcbe, DPModuleIndex, pDpContext->HDMIMonitorContext.HDMIClock, bOn);
            }
        }
    }

    if(!bOn)
    {
        cbPathMgrModuleOnOff(pcbe, pSource->ModulePath, CBIOS_FALSE);
    }

    pDevCommon->PowerState = bOn ? CBIOS_PM_ON : CBIOS_PM_OFF;

    cbDebugPrint((MAKE_LEVEL(DP, INFO),"%s: status = %s.\n", FUNCTION_NAME, (bOn)? "On" : "Off"));
}

static CBIOS_VOID cbDPPort_QueryMonitorAttribute(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBiosMonitorAttribute pMonitorAttribute)
{
    PCBIOS_DP_CONTEXT pDpContext = container_of(pDevCommon, PCBIOS_DP_CONTEXT, Common);

    cbTraceEnter(DP);

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & ALL_DP_TYPES)))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return ;
    }

    if (pDpContext->Common.CurrentMonitorType & (CBIOS_MONITOR_TYPE_HDMI | CBIOS_MONITOR_TYPE_DVI))
    {
        cbHDMIMonitor_QueryAttribute(pcbe, &pDpContext->HDMIMonitorContext, pMonitorAttribute);
    }
    else if (pDpContext->Common.CurrentMonitorType & (CBIOS_MONITOR_TYPE_DP | CBIOS_MONITOR_TYPE_PANEL))
    {
        cbDPMonitor_QueryAttribute(pcbe, &pDpContext->DPMonitorContext, pMonitorAttribute);
    }

    cbTraceExit(DP);
}

static CBIOS_VOID cbDPPort_UpdateModeInfo(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DISP_MODE_PARAMS pModeParams)
{
    PCBIOS_DP_CONTEXT pDpContext = container_of(pDevCommon, PCBIOS_DP_CONTEXT, Common);
    CBIOS_U32 PixelRepitition = pModeParams->PixelRepitition;
    CBIOS_U32 DiuPLLClock = 0;
    CBIOS_MODULE_INDEX HDTVModuleIndex = CBIOS_MODULE_INDEX_INVALID;

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & ALL_DP_TYPES)))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return ;
    }

    HDTVModuleIndex = cbGetModuleIndex(pcbe, pDpContext->Common.DeviceType, CBIOS_MODULE_TYPE_HDTV);
    if (pDpContext->Common.CurrentMonitorType & (CBIOS_MONITOR_TYPE_DP | CBIOS_MONITOR_TYPE_PANEL))
    {
        if ((HDTVModuleIndex != CBIOS_MODULE_INDEX_INVALID) && (pModeParams->TargetModePara.bInterlace))
        {
            pModeParams->TargetTiming.PixelClock *= 2;
        }
        DiuPLLClock = pModeParams->TargetTiming.PixelClock;
        pModeParams->TargetTiming.PLLClock = DiuPLLClock;

        cbDPMonitor_UpdateModeInfo(pcbe, &pDpContext->DPMonitorContext, pModeParams);
    }

    if (pDpContext->Common.CurrentMonitorType & (CBIOS_MONITOR_TYPE_HDMI | CBIOS_MONITOR_TYPE_DVI))
    {
        // patch to 1080i don't bypass HDTV module
        if ((HDTVModuleIndex != CBIOS_MODULE_INDEX_INVALID) && (pModeParams->TargetModePara.bInterlace))
        {
            if((pModeParams->TargetModePara.XRes == 1920) && (pModeParams->TargetModePara.YRes == 1080))
            {
                pModeParams->TargetTiming.PixelClock *= 2;
            }
        }
        // determine DIU PLL clock
        DiuPLLClock = pModeParams->TargetTiming.PixelClock;

        if (pModeParams->BitPerComponent == 8) // 24bit
        {
            if (PixelRepitition == 1)
            {
                DiuPLLClock *= 1;
            }
            else if (PixelRepitition == 2)
            {
                DiuPLLClock *= 2;
            }
            else if (PixelRepitition == 4)
            {
                DiuPLLClock *= 4;
            }
            pDpContext->HDMIMonitorContext.HDMIClock = DiuPLLClock;
        }
        else if (pModeParams->BitPerComponent == 10) // 30bit
        {
            if (PixelRepitition == 1)
            {
                DiuPLLClock = (DiuPLLClock * 5)/2;
            }
            else if (PixelRepitition == 2)
            {
                DiuPLLClock *= 5;
            }
            else if (PixelRepitition == 4)
            {
                DiuPLLClock *= 10;
            }
            pDpContext->HDMIMonitorContext.HDMIClock = DiuPLLClock/2;
        }
        else if (pModeParams->BitPerComponent == 12) // 36bit
        {
            if (PixelRepitition == 1)
            {
                DiuPLLClock *= 3;
                pDpContext->HDMIMonitorContext.HDMIClock = DiuPLLClock/2;
            }
            else if (PixelRepitition == 2)
            {
                DiuPLLClock *= 3;
                pDpContext->HDMIMonitorContext.HDMIClock = DiuPLLClock;
            }
            else if (PixelRepitition == 4)
            {
                DiuPLLClock *= 6;
                pDpContext->HDMIMonitorContext.HDMIClock = DiuPLLClock;
            }
        }

        if (HDTVModuleIndex != CBIOS_MODULE_INDEX_INVALID && (pModeParams->TargetModePara.bInterlace))
        {
            pDpContext->HDMIMonitorContext.HDMIClock /= 2;
        }

        if(CBIOS_YCBCR420OUTPUT == pModeParams->TargetModePara.OutputSignal)
        {
             pDpContext->HDMIMonitorContext.HDMIClock /= 2;
        }
        pModeParams->TargetTiming.PLLClock = DiuPLLClock;

        cbHDMIMonitor_UpdateModeInfo(pcbe, &pDpContext->HDMIMonitorContext, pModeParams);

    }

}

static CBIOS_VOID cbDPPort_SetMode(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DISP_MODE_PARAMS pModeParams)
{
    PCBIOS_DP_CONTEXT  pDpContext  = container_of(pDevCommon, PCBIOS_DP_CONTEXT, Common);
    CBIOS_MONITOR_TYPE MonitorType = CBIOS_MONITOR_TYPE_NONE;
    CBIOS_MODULE_INDEX HDTVModuleIndex = CBIOS_MODULE_INDEX_INVALID;

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & ALL_DP_TYPES)))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return ;
    }

    MonitorType = pDevCommon->CurrentMonitorType;
    HDTVModuleIndex = cbGetModuleIndex(pcbe, pDevCommon->DeviceType, CBIOS_MODULE_TYPE_HDTV);

    if (HDTVModuleIndex != CBIOS_MODULE_INDEX_INVALID)
    {
        // Enable HDTV block function.
        cbDoHDTVFuncSetting(pcbe, pModeParams, pModeParams->IGAIndex, pDevCommon->DeviceType);
    }

    if ((MonitorType == CBIOS_MONITOR_TYPE_HDMI) || (MonitorType == CBIOS_MONITOR_TYPE_DVI))
    {
        cbHDMIMonitor_SetMode(pcbe, &pDpContext->HDMIMonitorContext, pModeParams);
    }
    else if ((MonitorType == CBIOS_MONITOR_TYPE_DP) || (MonitorType == CBIOS_MONITOR_TYPE_PANEL))
    {
        cbDPMonitor_SetMode(pcbe, &pDpContext->DPMonitorContext, pModeParams);
    }
}

PCBIOS_DEVICE_COMMON cbDPPort_Init(PCBIOS_VOID pvcbe, PVCP_INFO pVCP, CBIOS_ACTIVE_TYPE DeviceType)
{
    PCBIOS_EXTENSION_COMMON pcbe          = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DP_CONTEXT    pDpContext = CBIOS_NULL;
    PCBIOS_DEVICE_COMMON pDeviceCommon = CBIOS_NULL;
    CBIOS_MODULE_INDEX   DPIndex;
    CBIOS_U32   ulTemp = 0;

    if(!(DeviceType & ALL_DP_TYPES))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"%s: unsupported device type!!!\n", FUNCTION_NAME));
        return CBIOS_NULL;
    }

    pDpContext = cb_AllocateNonpagedPool(sizeof(CBIOS_DP_CONTEXT));
    if(pDpContext == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"%s: pDpContext allocate error!!!\n", FUNCTION_NAME));
        return CBIOS_NULL;
    }

    pDeviceCommon = &pDpContext->Common;

    pDeviceCommon->DeviceType = DeviceType;
    pDeviceCommon->SupportMonitorType = cbGetSupportMonitorType(pcbe, DeviceType);
    pDeviceCommon->CurrentMonitorType = CBIOS_MONITOR_TYPE_NONE;
    pDeviceCommon->PowerState = CBIOS_PM_INVALID;
    cbHDCP_Init(pvcbe, &pDeviceCommon->pHDCPContext);

    cbInitialModuleList(&pDeviceCommon->DispSource.ModuleList);

    pDeviceCommon->pfncbDeviceHwInit = (PFN_cbDeviceHwInit)cbDPPort_HwInit;
    pDeviceCommon->pfncbUpdateDeviceModeInfo = (PFN_cbUpdateDeviceModeInfo)cbDPPort_UpdateModeInfo;
    pDeviceCommon->pfncbQueryMonitorAttribute = (PFN_cbQueryMonitorAttribute)cbDPPort_QueryMonitorAttribute;
    pDeviceCommon->pfncbDeviceDetect = (PFN_cbDeviceDetect)cbDPPort_DeviceDetect;
    pDeviceCommon->pfncbDeviceOnOff = (PFN_cbDeviceOnOff)cbDPPort_OnOff;
    pDeviceCommon->pfncbDeviceSetMode = (PFN_cbDeviceSetMode)cbDPPort_SetMode;

    if (DeviceType == CBIOS_TYPE_DP1)
    {
        pDeviceCommon->I2CBus = pVCP->DP1DualModeCharByte & I2CBUSMASK;
        pDeviceCommon->HPDPin = CBIOS_VIRTUAL_GPIO_FOR_DP1;
        pDeviceCommon->DispSource.ModuleList.DPModule.Index = CBIOS_MODULE_INDEX1;
        pDeviceCommon->DispSource.ModuleList.HDMIModule.Index = CBIOS_MODULE_INDEX1;
        pDeviceCommon->DispSource.ModuleList.HDCPModule.Index = CBIOS_MODULE_INDEX1;
        pDeviceCommon->DispSource.ModuleList.HDACModule.Index = CBIOS_MODULE_INDEX1;
    }
    else if (DeviceType == CBIOS_TYPE_DP2)
    {
        pDeviceCommon->I2CBus = pVCP->DP2DualModeCharByte & I2CBUSMASK;
        pDeviceCommon->HPDPin = CBIOS_VIRTUAL_GPIO_FOR_DP2;
        pDeviceCommon->DispSource.ModuleList.DPModule.Index = CBIOS_MODULE_INDEX2;
        pDeviceCommon->DispSource.ModuleList.HDMIModule.Index = CBIOS_MODULE_INDEX2;
        pDeviceCommon->DispSource.ModuleList.HDCPModule.Index = CBIOS_MODULE_INDEX2;
        pDeviceCommon->DispSource.ModuleList.HDACModule.Index = CBIOS_MODULE_INDEX2;
    }
    else if (DeviceType == CBIOS_TYPE_DP3)
    {
        pDeviceCommon->I2CBus = pVCP->DP3DualModeCharByte & I2CBUSMASK;
        pDeviceCommon->DispSource.ModuleList.DPModule.Index = CBIOS_MODULE_INDEX3;
        pDeviceCommon->DispSource.ModuleList.HDMIModule.Index = CBIOS_MODULE_INDEX3;
        pDeviceCommon->DispSource.ModuleList.HDCPModule.Index = CBIOS_MODULE_INDEX3;
        pDeviceCommon->DispSource.ModuleList.HDACModule.Index = CBIOS_MODULE_INDEX_INVALID;
    }
    else if (DeviceType == CBIOS_TYPE_DP4)
    {
        pDeviceCommon->I2CBus = pVCP->DP4DualModeCharByte & I2CBUSMASK;
        pDeviceCommon->DispSource.ModuleList.DPModule.Index = CBIOS_MODULE_INDEX4;
        pDeviceCommon->DispSource.ModuleList.HDMIModule.Index = CBIOS_MODULE_INDEX4;
        pDeviceCommon->DispSource.ModuleList.HDCPModule.Index = CBIOS_MODULE_INDEX4;
        pDeviceCommon->DispSource.ModuleList.HDACModule.Index = CBIOS_MODULE_INDEX_INVALID;
    }

    pDpContext->HDMIMonitorContext.pDevCommon = pDeviceCommon;
    pDpContext->DPMonitorContext.pDevCommon = pDeviceCommon;
    pDpContext->DPMonitorContext.GpioForEDP1Power = pVCP->GpioForEDP1Power;
    pDpContext->DPMonitorContext.GpioForEDP2Power = pVCP->GpioForEDP2Power;
    pDpContext->DPMonitorContext.GpioForEDP1BackLight = pVCP->GpioForEDP1BackLight;
    pDpContext->DPMonitorContext.GpioForEDP2BackLight = pVCP->GpioForEDP2BackLight;
    DPIndex = pDeviceCommon->DispSource.ModuleList.DPModule.Index;
    cb_memcpy(&pDeviceCommon->MaxResConfig, &pVCP->DPMaxResConfig[DPIndex], sizeof(CBIOS_MAX_RES_CONFIG));

    pDpContext->DPMonitorContext.SourceMaxLaneCount = 4;
    pDpContext->DPMonitorContext.SourceMaxLinkSpeed = CBIOS_DP_LINK_SPEED_5400Mbps;
    pDpContext->DPMonitorContext.bSourceSupportTPS3 = CBIOS_TRUE;

    if (NO_ERROR == cb_GetRegistryParameters(pcbe->pAdapterContext, KEYNAME_DW_DP_RUN_CTS, CBIOS_FALSE, &ulTemp))
    {
        if(ulTemp)
        {
            pDpContext->DPPortParams.bRunCTS = CBIOS_TRUE;//run dp cts
        }
        else
        {
            pDpContext->DPPortParams.bRunCTS = CBIOS_FALSE;
        }
    }
    else
    {
        pDpContext->DPPortParams.bRunCTS = CBIOS_FALSE;
    }

    return &pDpContext->Common;
}

CBIOS_VOID cbDPPort_DeInit(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon)
{
    PCBIOS_DP_CONTEXT pDpContext = container_of(pDevCommon, PCBIOS_DP_CONTEXT, Common);

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & ALL_DP_TYPES)))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return ;
    }

    cbHDCP_DeInit(pvcbe, pDevCommon->DeviceType);
    cb_FreePool(pDpContext);
    pDpContext = CBIOS_NULL;
}

