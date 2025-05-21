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
#include "../../Hw/HwBlock/CBiosDIU_DP.h"
#include "../../Hw/CBiosHwShare.h"
#include "../../Hw/HwBlock/CBiosDIU_HDCP.h"

#if DP_MONITOR_SUPPORT

static CBIOS_U8 DPFailSafeModeEdid[] =
{
    //For DP EDID corruption issue, set fail-safe mode: 640x480@60Hz, bpc=6
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x04, 0x43, 0x14, 0x81, 0x89, 0xDA, 0x07, 0x00,
    0x0A, 0x12, 0x01, 0x04, 0x95, 0x41, 0x29, 0x78, 0xE2, 0x8F, 0x95, 0xAD, 0x4F, 0x32, 0xB2, 0x25,
    0x0F, 0x50, 0x54, 0x20, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xD6, 0x09, 0x80, 0xA0, 0x20, 0xE0, 0x2D, 0x10, 0x10, 0x60,
    0xA2, 0x00, 0x8A, 0x9A, 0x21, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x44, 0x50, 0x31,
    0x39, 0x30, 0x35, 0x32, 0x38, 0x38, 0x37, 0x45, 0x54, 0x0A, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x41,
    0x42, 0x43, 0x20, 0x33, 0x30, 0x32, 0x30, 0x30, 0x38, 0x57, 0x53, 0x0A, 0x00, 0x00, 0x00, 0xFD,
    0x00, 0x31, 0x56, 0x1D, 0x71, 0x1C, 0x00, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0xF5,
    0x70, 0x10, 0x18, 0x03, 0x00, 0x01, 0x00, 0x0C, 0x8A, 0x02, 0x9A, 0x01, 0x00, 0x0A, 0x40, 0x06,
    0x18, 0x78, 0x3C, 0x75, 0x0D, 0x00, 0x06, 0x88, 0x20, 0x20, 0x40, 0x20, 0x20, 0xB5, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90,
};

static CBIOS_BOOL cbDPMonitor_IsEDPSupported(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX DPModuleIndex)
{
    return (((DPModuleIndex == CBIOS_MODULE_INDEX1) && (pcbe->FeatureSwitch.IsEDP1Enabled))
            || ((DPModuleIndex == CBIOS_MODULE_INDEX2) && (pcbe->FeatureSwitch.IsEDP2Enabled)));
}

static CBIOS_BOOL cbDPMonitor_EDPAuxPowerSeqCtrl(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_BOOL bOn)
{
    if (bOn)
    {
        // eDP power sequence on for aux channel operation
        // 1. enable vdd
        cbDIU_EDP_ControlVDDSignal(pcbe, pDPMonitorContext, DPModuleIndex, CBIOS_TRUE);
        // 2. wait for HDP from sink, timeout is 200ms
        if (!cbDIU_EDP_WaitforSinkHPDSignal(pcbe, DPModuleIndex, 200))
        {
            cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: can't wait HPD high from sink\n", FUNCTION_NAME));
            return CBIOS_FALSE;
        }
    }
    else
    {
        // eDP power sequence off for aux channel operation
        // disable vdd
        cbDIU_EDP_ControlVDDSignal(pcbe, pDPMonitorContext, DPModuleIndex, CBIOS_FALSE);
    }
    return CBIOS_TRUE;
}

static CBIOS_BOOL cbDPMonitor_LinkTrainingHw(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext)
{
    CBIOS_LINK_TRAINING_PARAMS LinkTrainingParams = {0};
    CBIOS_BOOL  bStatus = CBIOS_FALSE;
    CBIOS_EDP_CP_METHOD CPMethod = CBIOS_EDP_CP_DISABLE;
    CBIOS_MODULE_INDEX DPModuleIndex = cbGetModuleIndex(pcbe, pDPMonitorContext->pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);

    cbTraceEnter(DP);

    LinkTrainingParams.bEDP = CBIOS_FALSE;
    LinkTrainingParams.MaxLaneCount = pDPMonitorContext->LaneNumberToUse;
    LinkTrainingParams.MaxLinkSpeed = pDPMonitorContext->LinkSpeedToUse;
    LinkTrainingParams.DpSinkVersion = pDPMonitorContext->DpSinkVersion;
    LinkTrainingParams.TrainingAuxRdInterval = pDPMonitorContext->TrainingAuxRdInterval;
    LinkTrainingParams.bEnableTPS3 = pDPMonitorContext->bEnableTPS3;

    //check CP method
    if (cbDPMonitor_IsEDPSupported(pcbe, DPModuleIndex))
    {
        LinkTrainingParams.bEDP = CBIOS_TRUE;

        if (pcbe->CbiosFlags & DISABLE_EDP_CONTENT_PROTECTION)
        {
            CPMethod = CBIOS_EDP_CP_DISABLE;
        }
        else if (pDPMonitorContext->bSupportASSR && pDPMonitorContext->bSupportAF)
        {
            if (pcbe->CbiosFlags & DEFAULT_USE_EDP_CP_METHOD_3A_ASSR)
            {
                CPMethod = CBIOS_EDP_CP_ASSR;
            }
            else if (pcbe->CbiosFlags & DEFAULT_USE_EDP_CP_METHOD_3B_AF)
            {
                CPMethod = CBIOS_EDP_CP_AF;
            }
        }
        else if (pDPMonitorContext->bSupportASSR)
        {
            CPMethod = CBIOS_EDP_CP_ASSR;
        }
        else if (pDPMonitorContext->bSupportAF)
        {
            CPMethod = CBIOS_EDP_CP_AF;
        }
        else
        {
            CPMethod = CBIOS_EDP_CP_DISABLE;
        }
    }
    else
    {
        LinkTrainingParams.bEDP = CBIOS_FALSE;
        CPMethod = CBIOS_EDP_CP_DISABLE;
    }

    LinkTrainingParams.CPMethod = CPMethod;
    LinkTrainingParams.bEnhancedMode = pDPMonitorContext->EnhancedMode;

    bStatus = cbDIU_DP_LinkTrainingHw(pcbe, DPModuleIndex, &LinkTrainingParams);
    if (bStatus)
    {
        pDPMonitorContext->LinkPassSpeed = LinkTrainingParams.CurrLinkSpeed;
        pDPMonitorContext->LinkPassLaneNum = LinkTrainingParams.CurrLaneCount;
    }

    cbTraceExit(DP);

    return bStatus;
}

static CBIOS_BOOL cbDPMonitor_GetAutoTestDpcdData(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext)
{
    CBIOS_MODULE_INDEX DPModuleIndex = cbGetModuleIndex(pcbe, pDPMonitorContext->pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    AUX_CONTROL     AUX;
    DPCD_REG_00219  DPCD_00219;
    DPCD_REG_00220  DPCD_00220;
    CBIOS_U32  LaneNum = 0, LinkSpeed = 0;

    AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
    AUX.Offset = 0x219;
    AUX.Length = 1;
    if(!cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: Can not get the auto test DPCD link rate data!\n", FUNCTION_NAME));
        return CBIOS_FALSE;
    }

    DPCD_00219.Value = AUX.Data[0] & 0x000000FF;
    if (DPCD_00219.TEST_LINK_RATE == CBIOS_DPCD_LINK_RATE_5400Mbps)
    {
        LinkSpeed = CBIOS_DP_LINK_SPEED_5400Mbps;
    }
    else if (DPCD_00219.TEST_LINK_RATE == CBIOS_DPCD_LINK_RATE_2700Mbps)
    {
        LinkSpeed = CBIOS_DP_LINK_SPEED_2700Mbps;
    }
    else
    {
        LinkSpeed = CBIOS_DP_LINK_SPEED_1620Mbps;
    }

    AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
    AUX.Offset = 0x220;
    AUX.Length = 1;
    if(!cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: Can not get the auto test DPCD lane count data!\n", FUNCTION_NAME));
        return CBIOS_FALSE;
    }
    DPCD_00220.Value = AUX.Data[0] & 0x000000FF;
    if ((DPCD_00220.TEST_LANE_COUNT == 0x01) || (DPCD_00220.TEST_LANE_COUNT == 0x02) || (DPCD_00220.TEST_LANE_COUNT == 0x04))
    {
        LaneNum = DPCD_00220.TEST_LANE_COUNT;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: Invalid lane count:%d!\n", FUNCTION_NAME, DPCD_00220.TEST_LANE_COUNT));
        return CBIOS_FALSE;
    }

    pDPMonitorContext->LaneNumberToUse = LaneNum;
    pDPMonitorContext->LinkSpeedToUse = LinkSpeed;

    return CBIOS_TRUE;
}

static CBIOS_BOOL cbDPMonitor_GetSinkCapsFromSpecificPlace(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, CBIOS_U8 IGAIndex)
{
    PCBiosCustmizedDestTiming pUserTiming = CBIOS_NULL;

    if(pcbe->SpecifyDestTimingSrc[IGAIndex].Flag == 1)
    {
        pUserTiming = &pcbe->SpecifyDestTimingSrc[IGAIndex].UserCustDestTiming;
    }
    else if(pcbe->SpecifyDestTimingSrc[IGAIndex].Flag == 2)
    {
        pUserTiming = &pDPMonitorContext->TestDpcdDataTiming;
    }
    else
    {
        return CBIOS_FALSE;
    }

    if ((pUserTiming->LinkRate == CBIOS_DPCD_LINK_RATE_5400Mbps) ||
        (pUserTiming->LinkRate == CBIOS_DPCD_LINK_RATE_2700Mbps) ||
        (pUserTiming->LinkRate == CBIOS_DPCD_LINK_RATE_1620Mbps))
    {
        pDPMonitorContext->LaneNumberToUse = pUserTiming->LaneCount;
        if(pUserTiming->LinkRate == CBIOS_DPCD_LINK_RATE_5400Mbps)
        {
            pDPMonitorContext->LinkSpeedToUse = CBIOS_DP_LINK_SPEED_5400Mbps;
        }
        else if(pUserTiming->LinkRate == CBIOS_DPCD_LINK_RATE_2700Mbps)
        {
            pDPMonitorContext->LinkSpeedToUse = CBIOS_DP_LINK_SPEED_2700Mbps;
        }
        else
        {
            pDPMonitorContext->LinkSpeedToUse = CBIOS_DP_LINK_SPEED_1620Mbps;
        }

        if(pUserTiming->DClk)
        {
            pDPMonitorContext->bpc = pUserTiming->BitDepthPerComponet;

            pDPMonitorContext->AsyncMode = (pUserTiming->ClockSynAsyn == 0) ? 1 : 0;
            pDPMonitorContext->ColorFormat = pUserTiming->ColorFormat;
            pDPMonitorContext->DynamicRange = pUserTiming->DynamicRange;
            pDPMonitorContext->YCbCrCoefficients = pUserTiming->YCbCrCoefficients;
            pDPMonitorContext->EnhancedMode = pUserTiming->EnhancedFrameMode;
        }

        cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: LinkRate = %d, LaneCount = %d, bpc = %d, Async = %d\n", FUNCTION_NAME,
            pDPMonitorContext->LinkSpeedToUse, pDPMonitorContext->LaneNumberToUse, pDPMonitorContext->bpc, pDPMonitorContext->AsyncMode));
        cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: ColorFormat = %d, DynamicRange = %d, YCbCrCoefficients = %d, EnhancedMode = %d\n", FUNCTION_NAME,
            pDPMonitorContext->ColorFormat,pDPMonitorContext->DynamicRange, pDPMonitorContext->YCbCrCoefficients, pDPMonitorContext->EnhancedMode));

        /*In order to pass dp cts on unigraf DPR-120, use max link rate and max lane count,
          because dpcd timing perhaps don't update, but these two params update everytime.
        */
        if(pcbe->SpecifyDestTimingSrc[IGAIndex].Flag == 2)
        {
            pDPMonitorContext->LinkSpeedToUse = pDPMonitorContext->SinkMaxLinkSpeed;
            pDPMonitorContext->LaneNumberToUse = pDPMonitorContext->SinkMaxLaneCount;
        }

        return CBIOS_TRUE;
    }
    else
    {
        return CBIOS_FALSE;
    }
}

static CBIOS_VOID cbDPMonitor_DetermineLinkTrainingPara(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, CBIOS_BOOL isAutoTest)
{
    CBIOS_U32 MaxSupportClock_2700Mbps = 0, MaxSupportClock_1620Mbps = 0, MaxClockForCurrentLinkSpeed = 0;
    CBIOS_U8  IGAIndex = 0;
    CBIOS_MODULE_INDEX DPModuleIndex = cbGetModuleIndex(pcbe, pDPMonitorContext->pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    PCBIOS_DP_CONTEXT  pDpContext = container_of(pDPMonitorContext->pDevCommon, PCBIOS_DP_CONTEXT, Common);
    PCBIOS_EDPPanel_PARAMS    pEDPPanelDevice =  &(pDPMonitorContext->pDevCommon->DeviceParas.EDPPanelDevice);
    CBIOS_U32  LinkSpeed = 0, LaneNum = 0;
    CBIOS_BOOL  bGotLinkPara = CBIOS_FALSE;

    IGAIndex = DPModuleIndex;

    //for dp cts, if set registry for dp cts, Flag = 2, but some test of DPR120's dpcd timing is invalid.

    if(pDpContext->DPPortParams.bRunCTS)
    {
        pcbe->SpecifyDestTimingSrc[IGAIndex].Flag = 2;
    }

    if (isAutoTest)
    {
        bGotLinkPara = cbDPMonitor_GetAutoTestDpcdData(pcbe, pDPMonitorContext);
    }
    else if (pcbe->SpecifyDestTimingSrc[IGAIndex].Flag & 0x03)
    {
        bGotLinkPara = cbDPMonitor_GetSinkCapsFromSpecificPlace(pcbe, pDPMonitorContext, IGAIndex);
    }

    if(!bGotLinkPara)
    {
        if(pDPMonitorContext->bpc > DP_Default_bpc)
        {
            pDPMonitorContext->bpc = DP_Default_bpc;
        }

        LaneNum = pDPMonitorContext->SinkMaxLaneCount;

        // choose Link Speed according to current mode's pixel clock
        MaxSupportClock_2700Mbps = (CBIOS_DP_LINK_SPEED_2700Mbps / (pDPMonitorContext->bpc * 3)) * LaneNum * 8;
        MaxSupportClock_1620Mbps = (CBIOS_DP_LINK_SPEED_1620Mbps / (pDPMonitorContext->bpc * 3)) * LaneNum * 8;
        if (pDPMonitorContext->TargetTiming.PixelClock < MaxSupportClock_1620Mbps)
        {
            LinkSpeed = CBIOS_DP_LINK_SPEED_1620Mbps;
        }
        else if (pDPMonitorContext->TargetTiming.PixelClock < MaxSupportClock_2700Mbps)
        {
            LinkSpeed = CBIOS_DP_LINK_SPEED_2700Mbps;
        }
        else
        {
            LinkSpeed = CBIOS_DP_LINK_SPEED_5400Mbps;
        }

        if (LinkSpeed > pDPMonitorContext->SinkMaxLinkSpeed)
        {
            LinkSpeed = pDPMonitorContext->SinkMaxLinkSpeed;
        }

        if(cbDPMonitor_IsEDPSupported(pcbe, DPModuleIndex))
        {
            if(pEDPPanelDevice &&pEDPPanelDevice->EDPPanelDesc.EDPCaps.isHardcodeLinkPara)
            {
                LinkSpeed = pEDPPanelDevice->EDPPanelDesc.EDPCaps.LinkSpeed;
                LaneNum = pEDPPanelDevice->EDPPanelDesc.EDPCaps.LaneNum;
            }
            else
            {
                LinkSpeed = pDPMonitorContext->SinkMaxLinkSpeed;
                LaneNum = pDPMonitorContext->SinkMaxLaneCount;
            }
        }

        /* To avoid exceeding max clock when lighten 10bpc or higher, use default bpc*/
        MaxClockForCurrentLinkSpeed = (LinkSpeed / (pDPMonitorContext->bpc * 3)) * LaneNum * 8;
        if(pDPMonitorContext->TargetTiming.PixelClock > MaxClockForCurrentLinkSpeed
            && pDPMonitorContext->bpc > DP_Default_bpc)
        {
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: LinkSpeed(%d) can't support PixelClock(%d) with %d bit, use default bpc\n",
                            FUNCTION_NAME, LinkSpeed, pDPMonitorContext->TargetTiming.PixelClock, pDPMonitorContext->bpc));

            pDPMonitorContext->bpc = DP_Default_bpc;
        }

        pDPMonitorContext->LinkSpeedToUse = LinkSpeed;
        pDPMonitorContext->LaneNumberToUse = LaneNum;
    }

    // determine final link training params according to Source's caps
    if (pDPMonitorContext->LaneNumberToUse > pDPMonitorContext->SourceMaxLaneCount)
    {
        cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: source only supports lane count = %d\n",
            FUNCTION_NAME, pDPMonitorContext->SourceMaxLaneCount));
        pDPMonitorContext->LaneNumberToUse = pDPMonitorContext->SourceMaxLaneCount;
    }

    if (pDPMonitorContext->LinkSpeedToUse > pDPMonitorContext->SourceMaxLinkSpeed)
    {
        cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: source only supports link speed = %d\n",
            FUNCTION_NAME, pDPMonitorContext->SourceMaxLinkSpeed));
        pDPMonitorContext->LinkSpeedToUse = pDPMonitorContext->SourceMaxLinkSpeed;
    }

    if (pDPMonitorContext->bSourceSupportTPS3)
    {
        pDPMonitorContext->bEnableTPS3 = pDPMonitorContext->bSupportTPS3;
    }
    else
    {
        if (pDPMonitorContext->bSupportTPS3)
        {
            cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: source doesn't support TPS3\n", FUNCTION_NAME));
        }
        pDPMonitorContext->bEnableTPS3 = 0;
    }
}

CBIOS_BOOL cbDPMonitor_SetUpMainLink(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_BOOL              bStatus = CBIOS_FALSE;
    PCBIOS_TIMING_ATTRIB    pTiming = &pDPMonitorContext->TargetTiming;
    CBIOS_MODULE_INDEX      DPModuleIndex = cbGetModuleIndex(pcbe, pDPMonitorContext->pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    CBIOS_MODULE_INDEX      HDTVModuleIndex = cbGetModuleIndex(pcbe, pDPMonitorContext->pDevCommon->DeviceType, CBIOS_MODULE_TYPE_HDTV);
    CBIOS_MAIN_LINK_PARAMS  MainLinkParams = {0};

    cbTraceEnter(DP);

    pTiming->XRes *= pDPMonitorContext->PixelRepetition;
    pTiming->HorTotal *= pDPMonitorContext->PixelRepetition;
    pTiming->HorDisEnd *= pDPMonitorContext->PixelRepetition;
    pTiming->HorBStart *= pDPMonitorContext->PixelRepetition;
    pTiming->HorBEnd *= pDPMonitorContext->PixelRepetition;
    pTiming->HorSyncStart *= pDPMonitorContext->PixelRepetition;
    pTiming->HorSyncEnd *= pDPMonitorContext->PixelRepetition;

    if(pDPMonitorContext->bInterlace)
    {
        // HDTV module is used to transfer P -> i, so pixel clock need to be divided by 2
        if (HDTVModuleIndex != CBIOS_MODULE_INDEX_INVALID)
        {
            pTiming->PixelClock /= 2;
        }
        pTiming->YRes /= 2;
        pTiming->VerTotal /= 2;
        pTiming->VerDisEnd /= 2;
        pTiming->VerBStart /= 2;
        pTiming->VerBEnd /= 2;
        pTiming->VerSyncStart /= 2;
        pTiming->VerSyncEnd /= 2;
    }

    MainLinkParams.pTiming = pTiming;
    MainLinkParams.LinkedLaneNumber = pDPMonitorContext->LinkPassLaneNum;
    MainLinkParams.LinkedSpeed = pDPMonitorContext->LinkPassSpeed;
    MainLinkParams.bpc = pDPMonitorContext->bpc;
    MainLinkParams.TUSize = pDPMonitorContext->TUSize;
    MainLinkParams.AsyncMode = pDPMonitorContext->AsyncMode;
    MainLinkParams.ColorFormat = pDPMonitorContext->ColorFormat;
    MainLinkParams.DynamicRange = pDPMonitorContext->DynamicRange;
    MainLinkParams.YCbCrCoefficients = pDPMonitorContext->YCbCrCoefficients;

    bStatus = cbDIU_DP_SetUpMainLink(pcbe, DPModuleIndex, &MainLinkParams);

    cbTraceExit(DP);

    return bStatus;
}

CBIOS_BOOL cbDPMonitor_LinkTraining(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, CBIOS_BOOL isAutoTest)
{
    CBIOS_BOOL    bStatus = CBIOS_FALSE;
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_MODULE_INDEX      DPModuleIndex = cbGetModuleIndex(pvcbe, pDPMonitorContext->pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);

    cbTraceEnter(DP);

    if (pDPMonitorContext->DpAuxWorkingStatus & AUX_WORKING_STATUS_LINKTRAINING)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: this routine re-rentered!!!\n", FUNCTION_NAME));
        return CBIOS_FALSE;
    }
    else if (pDPMonitorContext->DpAuxWorkingStatus & AUX_WORKING_STATUS_EDID_READING)
    {
        cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s:AUX CHANNEL IS BUSY READING EDID!!!\n", FUNCTION_NAME));
    }

    //Get the specified link training parameters
    cbDPMonitor_DetermineLinkTrainingPara(pcbe, pDPMonitorContext, isAutoTest);

    pDPMonitorContext->DpAuxWorkingStatus |= AUX_WORKING_STATUS_LINKTRAINING;

    cbPHY_DP_InitEPHY(pcbe, DPModuleIndex);

    bStatus = cbDPMonitor_LinkTrainingHw(pcbe, pDPMonitorContext);

    if (bStatus)
    {
        pDPMonitorContext->LT_Status = 1;
    }
    pDPMonitorContext->DpAuxWorkingStatus &= ~AUX_WORKING_STATUS_LINKTRAINING;

    cbTraceExit(DP);
    return bStatus;
}

CBIOS_BOOL cbDPMonitor_GetTrainingData(PCBIOS_VOID pvcbe,
                                       CBIOS_MODULE_INDEX DPModuleIndex,
                                       CBIOS_U32 ulLaneNum,
                                       CBIOS_U32 *DPCD202_205,
                                       CBIOS_U32 RequestVoltage[4],
                                       CBIOS_U32 RequestPreEmph[4])
{
    CBIOS_U32 i, DPCD206_207 = 0;
    AUX_CONTROL AUX;
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;

    AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
    AUX.Offset = 0x202;
    AUX.Length = 6;
    if(!cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: failed!\n", FUNCTION_NAME));
        return CBIOS_FALSE;
    }

    *DPCD202_205 = AUX.Data[0];
    DPCD206_207 = AUX.Data[1] & 0x0000FFFF;
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: DPCD_207_202: 0x%04x%08x!\n", FUNCTION_NAME, DPCD206_207, *DPCD202_205));

    for (i = 0; i < ulLaneNum; i++)
    {
        RequestVoltage[i] = (DPCD206_207 >> (i * 4)) & 0x00000003;
        RequestPreEmph[i] = (DPCD206_207 >> (2 + i * 4)) & 0x00000003;

        //Verify ulTemp not indicate illegal voltage swing / pre-emphasis combinations
        while ((RequestVoltage[i] + RequestPreEmph[i]) > 3)
        {
            RequestPreEmph[i]--;
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: RequestVoltage+PreEmph >= 3, lower RequestPreEmph!\n", FUNCTION_NAME));
        }
    }

    return CBIOS_TRUE;
}

CBIOS_BOOL cbDPMonitor_AuxReadEDID(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_UCHAR pEDIDBuffer,
                                   CBIOS_U32 ulBufferSize)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_BOOL              bStatus = CBIOS_FALSE;
    CBIOS_U32               i, j;
    CBIOS_U32               dTemp;
    CBIOS_UCHAR             ucChecksum;
    CBIOS_U32               ulEdidLength = 0;
    PCBIOS_UCHAR            pHardcodedEdidBuffer = CBIOS_NULL;
    CBIOS_ACTIVE_TYPE       Device = pDPMonitorContext->pDevCommon->DeviceType;
    CBIOS_MODULE_INDEX      DPModuleIndex = cbGetModuleIndex(pcbe, Device, CBIOS_MODULE_TYPE_DP);

    cbTraceEnter(DP);

    if (pDPMonitorContext->DpAuxWorkingStatus & AUX_WORKING_STATUS_EDID_READING)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: this routine re-rentered!!!", FUNCTION_NAME));
        return CBIOS_FALSE;
    }
    else if (pDPMonitorContext->DpAuxWorkingStatus & AUX_WORKING_STATUS_LINKTRAINING)
    {
        cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: AUX CHANNEL IS BUSY LINKTRAINING!!!", FUNCTION_NAME));
    }

    pDPMonitorContext->DpAuxWorkingStatus |= AUX_WORKING_STATUS_EDID_READING;

    cb_memset(pEDIDBuffer, 0, ulBufferSize);

    // check if use hardcode EDID
    if (pcbe->DevicesHardcodedEdid & Device)
    {
        pHardcodedEdidBuffer = pcbe->EdidFromRegistry;
        cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: EDID is hardcoded to read from registry!\n", FUNCTION_NAME));
    }
    // check if use faked edid sending from the upper layer
    else if (pDPMonitorContext->pDevCommon->isFakeEdid)
    {
        pHardcodedEdidBuffer = pDPMonitorContext->pDevCommon->EdidData;
        cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: EDID is hardcoded to faked edid\n", FUNCTION_NAME));
    }

    if (pHardcodedEdidBuffer == CBIOS_NULL)
    {
        ulEdidLength = cbDIU_DP_AuxReadEDID(pcbe, DPModuleIndex, pEDIDBuffer, ulBufferSize);
        if (ulEdidLength == 0)
        {
            cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: read EDID fail through aux channel!!\n", FUNCTION_NAME));

            bStatus = CBIOS_FALSE;
            goto exitAuxReadEDID;
        }
        else
        {
            bStatus = CBIOS_TRUE;
        }
    }
    else
    {
        ucChecksum = 0;
        for (i = 0; i < ulBufferSize / 128; i++)
        {
            for (j = 0; j < 128 / 2; j ++)
            {
                dTemp = ((PCBIOS_U16)(pHardcodedEdidBuffer))[(i * 128 + j * 2) / 2];

                ucChecksum += pEDIDBuffer[i * 128 + j * 2 + 0] = (CBIOS_U8) (dTemp >> 0);
                ucChecksum += pEDIDBuffer[i * 128 + j * 2 + 1] = (CBIOS_U8) (dTemp >> 8);
            }

            ulEdidLength = i * 128 + 128;

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
                goto exitAuxReadEDID;
            }
        }
    }

    if (!cbEDIDModule_IsEDIDHeaderValid(pEDIDBuffer, ulEdidLength))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: EDID header is wrong!!\n", FUNCTION_NAME));

        bStatus = CBIOS_FALSE;
        goto exitAuxReadEDID;
    }

    if ((pEDIDBuffer[0x7E] + 1) * 128 > CBIOS_EDIDDATABYTE) // Longer than buffer
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: EDID length exceeds code limitation, discard remaider!\n", FUNCTION_NAME));
        ASSERT(CBIOS_FALSE);
    }

exitAuxReadEDID:
    pDPMonitorContext->DpAuxWorkingStatus &= ~AUX_WORKING_STATUS_EDID_READING;
    if (!bStatus)
    {
        // Corrupted EDID, zero it to avoid be analyzed elsewhere without check corruption.
        cbDumpBuffer(pcbe, pEDIDBuffer, ulBufferSize);
        cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: Corrupted EDID, zero entire EDID buffer!\n", FUNCTION_NAME));
        cb_memset(pEDIDBuffer, 0, ulBufferSize);
    }

    cbTraceExit(DP);
    return bStatus;
}

CBIOS_BOOL cbDPMonitor_AuxReadEDIDOffset(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_UCHAR pEDIDBuffer,
                                         CBIOS_U32 ulBufferSize, CBIOS_U32 ulReadEdidOffset)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_BOOL              bStatus = CBIOS_FALSE;
    PCBIOS_UCHAR            pHardcodedEdidBuffer = CBIOS_NULL;
    CBIOS_ACTIVE_TYPE       Device = pDPMonitorContext->pDevCommon->DeviceType;
    CBIOS_MODULE_INDEX      DPModuleIndex = cbGetModuleIndex(pcbe, Device, CBIOS_MODULE_TYPE_DP);

    if ((DPModuleIndex != CBIOS_MODULE_INDEX1) && (DPModuleIndex != CBIOS_MODULE_INDEX2)&&
        (DPModuleIndex != CBIOS_MODULE_INDEX3) && (DPModuleIndex != CBIOS_MODULE_INDEX4))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: Invalid DP index!!!", FUNCTION_NAME));
        return CBIOS_FALSE;
    }

    if (pDPMonitorContext->DpAuxWorkingStatus & AUX_WORKING_STATUS_EDID_READING)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: this routine re-rentered!!!", FUNCTION_NAME));
        return CBIOS_FALSE;
    }
    else if (pDPMonitorContext->DpAuxWorkingStatus & AUX_WORKING_STATUS_LINKTRAINING)
    {
        cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: AUX CHANNEL IS BUSY LINKTRAINING!!!", FUNCTION_NAME));
    }

    pDPMonitorContext->DpAuxWorkingStatus |= AUX_WORKING_STATUS_EDID_READING;

    cb_memset(pEDIDBuffer, 0, ulBufferSize);

    if (DPModuleIndex == CBIOS_MODULE_INDEX1)
    {
        //check hardcoded EDID
        if (pcbe->DevicesHardcodedEdid & CBIOS_TYPE_DP1)
        {
            pHardcodedEdidBuffer = pcbe->EdidFromRegistry;
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: DP1 EDID is hardcoded to read from registry!\n", FUNCTION_NAME));
        }
        else if (pcbe->CbiosFlags & HARDCODE_DP1_EDID_ALL_ZERO_BYTES)
        {
            // Already zero the buffer, return CBIOS_FALSE diretly.
            pDPMonitorContext->DpAuxWorkingStatus &= ~AUX_WORKING_STATUS_EDID_READING;
            cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: DP1 EDID is hardcoded to all zero bytes!\n", FUNCTION_NAME));
            bStatus = CBIOS_FALSE;
            goto ExitFunc;
        }
    }
    else if (DPModuleIndex == CBIOS_MODULE_INDEX2)
    {
        //check hardcoded EDID
        if (pcbe->DevicesHardcodedEdid & CBIOS_TYPE_DP2)
        {
            pHardcodedEdidBuffer = pcbe->EdidFromRegistry;
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: DP2 EDID is hardcoded to read from registry!\n", FUNCTION_NAME));
        }
    }
    else if (DPModuleIndex == CBIOS_MODULE_INDEX3)
    {
        //check hardcoded EDID
        if (pcbe->DevicesHardcodedEdid & CBIOS_TYPE_DP3)
        {
            pHardcodedEdidBuffer = pcbe->EdidFromRegistry;
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: DP3 EDID is hardcoded to read from registry!\n", FUNCTION_NAME));
        }
    }
    else if (DPModuleIndex == CBIOS_MODULE_INDEX4)
    {
        //check hardcoded EDID
        if (pcbe->DevicesHardcodedEdid & CBIOS_TYPE_DP4)
        {
            pHardcodedEdidBuffer = pcbe->EdidFromRegistry;
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: DP4 EDID is hardcoded to read from registry!\n", FUNCTION_NAME));
        }
    }


    if (pHardcodedEdidBuffer != CBIOS_NULL)
    {
        cb_memcpy(pEDIDBuffer, pHardcodedEdidBuffer + ulReadEdidOffset, ulBufferSize);
        bStatus = CBIOS_TRUE;
    }
    else
    {
        bStatus = cbDIU_DP_AuxReadEDIDOffset(pcbe, DPModuleIndex, pEDIDBuffer, ulBufferSize, ulReadEdidOffset);
    }

ExitFunc:

    pDPMonitorContext->DpAuxWorkingStatus &= ~AUX_WORKING_STATUS_EDID_READING;

    return bStatus;
}

static CBIOS_BOOL cbDPMonitor_GetDPCDVersion(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext)
{
    CBIOS_MODULE_INDEX  DPModuleIndex = cbGetModuleIndex(pcbe, pDPMonitorContext->pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    CBIOS_BOOL  bStatus = CBIOS_FALSE;
    AUX_CONTROL AUX;

    AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
    AUX.Offset = 0;
    AUX.Length = 0x1;
    if(cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX) == CBIOS_TRUE)
    {
        pDPMonitorContext->DpSinkVersion = AUX.Data[0] & 0xFF;
        bStatus = CBIOS_TRUE;
    }

    return bStatus;
}

static CBIOS_BOOL cbDPMonitor_HDCP_ReadBKsv(CBIOS_VOID* pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_U8 pBksv, PCBIOS_BOOL bRepeater)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_MODULE_INDEX  DPModuleIndex = cbGetModuleIndex(pcbe, pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    CBIOS_BOOL bRet = CBIOS_TRUE;
    CBIOS_U8 BCaps = 0;
    AUX_CONTROL AUX;

    AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
    AUX.Offset = 0x68000;
    AUX.Length = 0x5;
    bRet = cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX);
    if(bRet)
    {
        cb_memcpy(pBksv, (PCBIOS_U8)AUX.Data, 5);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: Read BKsv failed!\n", FUNCTION_NAME));

        return bRet;
    }

    AUX.Offset = 0x68028;
    AUX.Length = 0x1;
    bRet = cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX);
    if(bRet)
    {
        BCaps = AUX.Data[0] & 0xFF;
        *bRepeater = BCaps & 0x2;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: Read BCaps failed!\n", FUNCTION_NAME));
    }

    return bRet;
}

static CBIOS_VOID cbDPMonitor_SetSinkPowerState(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_BOOL bPowerOn)
{
    AUX_CONTROL AUX = {0};
    DPCD_REG_00600 DPCD00600;
    REG_MM8330    DPAuxTimerRegValue;
    CBIOS_U8      counter = 0, plugin = 0;

    cbTraceEnter(DP);

    for(counter = 0; counter < 5; counter++)
    {
        DPAuxTimerRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_TIMER[DPModuleIndex]);
        if((DPAuxTimerRegValue.HPD_Status == 1) || (DPAuxTimerRegValue.HPD_Status == 3)) // plugin
        {
            plugin = 1;
            break;
        }
        cb_DelayMicroSeconds(200); // delay 200us
    }
    if(!plugin)
    {
        cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: Aux can't work when DP is unplugged, mm%x==%08x!!!\n",
            FUNCTION_NAME, DP_REG_AUX_TIMER[DPModuleIndex], DPAuxTimerRegValue.Value));

        return;
    }

    DPCD00600.Value = 0;
    if (bPowerOn)
    {
        DPCD00600.SET_POWER = 1; // normal operation mode
    }
    else
    {
        DPCD00600.SET_POWER = 2; // power down mode
    }

    AUX.Function = CBIOS_AUX_REQUEST_NATIVE_WRITE;
    AUX.Offset = 0x600;
    AUX.Length = 0x1;
    AUX.Data[0] = DPCD00600.Value;

    if(bPowerOn)
    {
        // Source must try at least 3 times if no reply/response from Sink
        // Not all Sink device implemented this feature, so can't exit here
        if(!cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
        {
            cbDelayMilliSeconds(10);
            if(!cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
            {
                cbDelayMilliSeconds(20);
                if(!cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
                {
                    // 20090331_BC For DP compliance test 4.4.3
                    // QuantumData 882 won't ACK this power on request even after 3 attempts
                    // But if we started link training too early, the test failed
                    // The test checks by monitoring the link training registers for certain period
                    // so put in some delay here.  Do not reduce this number (40ms).
                    // Note that this would not affect normal operation since the Sink would usually
                    // respond to the very first power on request already
                    cbDelayMilliSeconds(40);
                }
            }
        }
    }
    else
    {
        if(!cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
        {
            cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: Write DPCD power state D3 error!\n", FUNCTION_NAME));
        }
    }
    cbTraceExit(DP);
}

static CBIOS_BOOL cbDPMonitor_GetSinkCaps(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext)
{
    CBIOS_MODULE_INDEX  DPModuleIndex = cbGetModuleIndex(pcbe, pDPMonitorContext->pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    CBIOS_BOOL          bStatus = CBIOS_FALSE;
    AUX_CONTROL         AUX;
    DPCD_REG_00001      DPCD_00001 = {0};
    DPCD_REG_00002      DPCD_00002 = {0};
    DPCD_REG_00005      DPCD_00005 = {0};
    DPCD_REG_00007      DPCD_00007 = {0};
    DPCD_REG_0000D      DPCD_0000D = {0};
    DPCD_REG_0000E      DPCD_0000E = {0};
    DPCD_REG_00200      DPCD_00200 = {0};
    CBIOS_U32           SinkCount = 0;

    cbTraceEnter(DP);

    // Read DP monitor Caps
    // In order to meet CTS item: 4.2.2.2, read 16 bytes although some of them are needless here.
    AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
    AUX.Offset = 0;
    AUX.Length = 16;
    if(cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
    {
        DPCD_00001.Value = (AUX.Data[0] >> 8) & 0x000000FF;
        if (DPCD_00001.MAX_LINK_RATE >= CBIOS_DPCD_LINK_RATE_5400Mbps)
        {
            // MAX_LANE_RATE 0x14 ought to be 5.4Gbps.
            pDPMonitorContext->SinkMaxLinkSpeed = CBIOS_DP_LINK_SPEED_5400Mbps;

            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: link speed DP Sink max supports is 5.4Gpbs\n", FUNCTION_NAME));
        }
        else if (DPCD_00001.MAX_LINK_RATE == CBIOS_DPCD_LINK_RATE_2700Mbps)
        {
            pDPMonitorContext->SinkMaxLinkSpeed = CBIOS_DP_LINK_SPEED_2700Mbps;
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: link speed DP Sink max supports is 2.7Gpbs\n", FUNCTION_NAME));
        }
        else
        {
            pDPMonitorContext->SinkMaxLinkSpeed = CBIOS_DP_LINK_SPEED_1620Mbps;
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: link speed DP Sink max supports is 1.62Gpbs\n", FUNCTION_NAME));
        }

        // Check if anyone want to over write link speed in Registry.
        if (pcbe->CbiosFlags & HARDCODE_DP1_MAX_LINKSPEED_1620)
        {
            pDPMonitorContext->SinkMaxLinkSpeed = CBIOS_DP_LINK_SPEED_1620Mbps;
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: link speed is hardcoded to 1.62Gpbs\n", FUNCTION_NAME));
        }
        else if (pcbe->CbiosFlags & HARDCODE_DP1_MAX_LINKSPEED_2700)
        {
            pDPMonitorContext->SinkMaxLinkSpeed = CBIOS_DP_LINK_SPEED_2700Mbps;
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: link speed is hardcoded to 2.7Gbps\n", FUNCTION_NAME));
        }
        else if (pcbe->CbiosFlags & HARDCODE_DP1_MAX_LINKSPEED_5400)
        {
            pDPMonitorContext->SinkMaxLinkSpeed = CBIOS_DP_LINK_SPEED_5400Mbps;
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: link speed is hardcoded to 5.4Gbps\n", FUNCTION_NAME));
        }

        DPCD_00002.Value = (AUX.Data[0] >> 16) & 0x000000FF;
        if ((DPCD_00002.MAX_LANE_COUNT == 0x01) || (DPCD_00002.MAX_LANE_COUNT == 0x02) || (DPCD_00002.MAX_LANE_COUNT == 0x04))
        {
            pDPMonitorContext->SinkMaxLaneCount= DPCD_00002.MAX_LANE_COUNT;
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: lane count DP Sink max supports is %d\n", FUNCTION_NAME, DPCD_00002.MAX_LANE_COUNT));
        }
        else
        {
            pDPMonitorContext->SinkMaxLaneCount = 4;
        }

        // DP 1.2
        if (pDPMonitorContext->DpSinkVersion >= 0x12)
        {
            pDPMonitorContext->bSupportTPS3 = DPCD_00002.TPS3_SUPPORTED;
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: bSupportTPS3 = %d\n",
                FUNCTION_NAME, pDPMonitorContext->bSupportTPS3));
        }
        else
        {
            pDPMonitorContext->bSupportTPS3 = CBIOS_FALSE;
        }

        // DP 1.1
        if (pDPMonitorContext->DpSinkVersion >= 0x11)
        {
            pDPMonitorContext->bSupportEnhanceMode = DPCD_00002.ENHANCED_FRAME_CAP;
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: bSupportEnhanceMode = %d\n",
                FUNCTION_NAME, pDPMonitorContext->bSupportEnhanceMode));
        }
        else
        {
            pDPMonitorContext->bSupportEnhanceMode = CBIOS_FALSE;
        }

        // Check if anyone want to over write lane count in Registry.
        if (pcbe->CbiosFlags & HARDCODE_DP1_LANECOUNT_1)
        {
            pDPMonitorContext->SinkMaxLaneCount = 1;
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: lane count is hardcoded to 1!\n", FUNCTION_NAME));
        }
        else if (pcbe->CbiosFlags & HARDCODE_DP1_LANECOUNT_2)
        {
            pDPMonitorContext->SinkMaxLaneCount = 2;
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: lane count is hardcoded to 2!\n", FUNCTION_NAME));
        }
        else if (pcbe->CbiosFlags & HARDCODE_DP1_LANECOUNT_4)
        {
            pDPMonitorContext->SinkMaxLaneCount = 4;
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: lane count is hardcoded to 4!\n", FUNCTION_NAME));
        }

        // check eDP content protection
        DPCD_0000D.Value = (AUX.Data[3] >> 8) & 0x000000FF;
        //it's BIT0 listed in DP 1.2 draft and eDP 1.2 draft. But bit4 in eDP 1.3. Need confirm
        if (DPCD_0000D.ALTERNATE_SCRAMBLER_RESET_CAPABLE)
        {
            pDPMonitorContext->bSupportASSR = CBIOS_TRUE;
        }
        else
        {
            pDPMonitorContext->bSupportASSR = CBIOS_FALSE;
        }

        if (DPCD_0000D.FRAMING_CHANGE_CAPABLE)
        {
            pDPMonitorContext->bSupportAF = CBIOS_TRUE;
        }
        else
        {
            pDPMonitorContext->bSupportAF = CBIOS_FALSE;
        }

        // DP 1.2
        if (pDPMonitorContext->DpSinkVersion >= 0x12)
        {
            DPCD_0000E.Value = (AUX.Data[3] >> 16) & 0x000000FF;
            pDPMonitorContext->TrainingAuxRdInterval = DPCD_0000E.TRAINING_AUX_RD_INTERVAL;
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: TrainingAuxRdInterval = %d\n", FUNCTION_NAME, pDPMonitorContext->TrainingAuxRdInterval));
        }

        // CTS: 4.2.2.7
        DPCD_00005.Value = (AUX.Data[1] >>  8) & 0x000000FF;
        DPCD_00007.Value = (AUX.Data[1] >> 24) & 0x000000FF;
        if (DPCD_00005.DWN_STRM_PORT_PRESENT)
        {
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: downstream port count = %d\n", FUNCTION_NAME, DPCD_00007.DWN_STRM_PORT_COUNT));

            AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
            AUX.Offset = 0x200;
            AUX.Length = 1;
            if(cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
            {
                DPCD_00200.Value = AUX.Data[0] & 0x000000FF;
                SinkCount = (DPCD_00200.SINK_COUNT_6 << 6) + DPCD_00200.SINK_COUNT_5_0;
                cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: SinkCount = %d\n", FUNCTION_NAME, SinkCount));
            }
            else
            {
                cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: cannot get DPCD(offset = 0x%x, len = %d)\n",
                    FUNCTION_NAME, AUX.Offset, AUX.Length));
            }
        }
        bStatus = CBIOS_TRUE;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: cannot get sink caps through aux channel!\n", FUNCTION_NAME));
    }

    //for DP1.4 CTS 4.2.2.7 and 4.2.2.8, need read extended capability
    if(DPCD_0000E.EXTENDED_RECEIVER_CAPABILITY_FIELD_PRESENT)
    {
        AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
        AUX.Offset = 0x2200;
        AUX.Length = 16;
        if(cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
        {
            bStatus = CBIOS_TRUE;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: cannot get sink extended caps through aux channel!\n", FUNCTION_NAME));
        }
    }

    pDPMonitorContext->bpc = DP_Default_bpc;
    pDPMonitorContext->EnhancedMode = 0x01;
    pDPMonitorContext->AsyncMode = 0x01;
    pDPMonitorContext->ColorFormat = 0;
    pDPMonitorContext->DynamicRange = 0;
    pDPMonitorContext->YCbCrCoefficients = 0;
    pDPMonitorContext->TUSize = DP_Default_TUSize;
    pDPMonitorContext->LT_Status = 0;

    cbTraceExit(DP);
    return bStatus;
}

CBIOS_U32 cbDPMonitor_GetMaxSupportedDclk(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext)
{
    CBIOS_U32 MaxDclk = 0;
    CBIOS_U32 Bpc = DP_Default_bpc;

    if (pDPMonitorContext == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 2nd param is invalid\n", FUNCTION_NAME));
        return 0;
    }

    if(pDPMonitorContext->bpc > DP_Default_bpc)
    {
        Bpc = DP_Default_bpc;
    }
    else
    {
        Bpc = pDPMonitorContext->bpc;
    }

    MaxDclk = (pDPMonitorContext->SinkMaxLinkSpeed / (Bpc * 3))
                           * pDPMonitorContext->SinkMaxLaneCount * 8;

    return MaxDclk;
}

CBIOS_VOID cbDPMonitor_GetMonitorParamsFromEdid(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, CBIOS_BOOL IsUpdateDevSignature)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DEVICE_COMMON    pDevCommon = pDPMonitorContext->pDevCommon;
    CBIOS_MODULE_INDEX      DPModuleIndex = cbGetModuleIndex(pcbe, pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    CBIOS_BOOL              IsEDP = CBIOS_FALSE;
    PCBIOS_U8               pEdidBuffer = pDevCommon->EdidData;

    if (IsUpdateDevSignature)
    {
        cbUpdateDeviceSignature(pcbe, pDevCommon);
        pDevCommon->isFakeEdid = CBIOS_FALSE;
    }

    if(cbDPMonitor_IsEDPSupported(pcbe, DPModuleIndex))
    {
        IsEDP = CBIOS_TRUE;
    }

    if(IsEDP)
    {
        pDevCommon->CurrentMonitorType = CBIOS_MONITOR_TYPE_PANEL;
    }
    else
    {
        pDevCommon->CurrentMonitorType = CBIOS_MONITOR_TYPE_DP;
    }

    // Check if HDMI
    if (pDevCommon->EdidStruct.Attribute.IsCEA861HDMI)
    {
        cbDebugPrint((MAKE_LEVEL(DP, INFO), "%s: HDMI device on dual mode DP port!\n", FUNCTION_NAME));
    }

    pDPMonitorContext->bpc = DP_Default_bpc;
    //: Should check EDID 1.4 here since majority of DP monitors don't have extended EDID block
    if((pEdidBuffer[0x12] == 0x01) && (pEdidBuffer[0x13] == 0x04))
    {
        if (pEdidBuffer[0x14] & 0x80)
        {
            if((pEdidBuffer[0x14] & 0x70) == 0x10)
            {
                pDPMonitorContext->bpc = 6;
            }
            else if((pEdidBuffer[0x14] & 0x70) == 0x20)
            {
                pDPMonitorContext->bpc = 8;
            }
            else if((pEdidBuffer[0x14] & 0x70) == 0x30)
            {
                pDPMonitorContext->bpc = 10;
            }
            else if((pEdidBuffer[0x14] & 0x70) == 0x40)
            {
                pDPMonitorContext->bpc = 12;
            }
            else if((pEdidBuffer[0x14] & 0x70) == 0x50)
            {
                pDPMonitorContext->bpc = 14;
            }
            else if((pEdidBuffer[0x14] & 0x70) == 0x60)
            {
                pDPMonitorContext->bpc = 16;
            }
        }
    }

    cbDebugPrint((MAKE_LEVEL(DP, INFO), "%s: DP monitor supported bpc = %d\n", FUNCTION_NAME, pDPMonitorContext->bpc));
}

CBIOS_BOOL cbDPMonitor_Detect(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, CBIOS_BOOL bHardcodeDetected, CBIOS_U32 FullDetect)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_BOOL              bConnected = CBIOS_FALSE;
    CBIOS_BOOL              IsDevChanged = CBIOS_FALSE;
    REG_MM8340              DPEphyMpllRegValue;
    REG_MM8348              DPEphyMiscRegValue;
    CBIOS_U8                AuxReadEDIDTime;
    CBIOS_BOOL              bGetEDID = CBIOS_FALSE;
    PCBIOS_DEVICE_COMMON    pDevCommon = pDPMonitorContext->pDevCommon;
    CBIOS_MODULE_INDEX      DPModuleIndex = cbGetModuleIndex(pcbe, pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    PCBIOS_EDP_PANEL_DESC   pPanelDesc = &(pDevCommon->DeviceParas.EDPPanelDevice.EDPPanelDesc);

    cb_AcquireMutex(pcbe->pAuxMutex);

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        bConnected = CBIOS_FALSE;
        goto EXIT;
    }

    //save mm8340 value
    DPEphyMpllRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_EPHY_MPLL[DPModuleIndex]);
    // save mm8348 value
    DPEphyMiscRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_EPHY_MISC[DPModuleIndex]);

    if(cbDPMonitor_IsEDPSupported(pcbe, DPModuleIndex))
    {
        cbEDPPanel_PreInit(pcbe);
    }

    cbPHY_DP_AuxPowerOn(pcbe, DPModuleIndex);

    cbDIU_DP_ResetAUX(pcbe, DPModuleIndex);

    if(cbDPMonitor_IsEDPSupported(pcbe, DPModuleIndex))
    {
        if (pDevCommon->PowerState != CBIOS_PM_ON) //if eDP already power on, skip aux power sequence
        {
            cbDPMonitor_EDPAuxPowerSeqCtrl(pcbe, pDPMonitorContext, DPModuleIndex, CBIOS_TRUE);
        }
    }

    //DP LinkCompTest 4.4.3, "Sink shall not respond to any AUX requests
    //prior to initial power save mode exit request."
    //So, we must set sink device to D0 state, otherwise, any AUX requests
    //may fail.
    cbDPMonitor_SetSinkPowerState(pcbe, DPModuleIndex, CBIOS_TRUE);

    if (cbDPMonitor_GetDPCDVersion(pcbe, pDPMonitorContext))
    {
        bConnected = CBIOS_TRUE;

        cbDPMonitor_GetSinkCaps(pcbe, pDPMonitorContext);

        //Following is patch for the cbDPMonitor_AuxReadEDID fail due to the conflict usage of Aux Channel between Reading EDID and sending CP_IRQ signal ,
        //which causes the EDID process being interrupted and fail.
        for (AuxReadEDIDTime = 0; AuxReadEDIDTime < 3; AuxReadEDIDTime++)
        {
            bGetEDID = cbGetDeviceEDID(pcbe, pDevCommon, &IsDevChanged, FullDetect);
            if (bGetEDID)
            {
                break;
            }
        }
        if(!bGetEDID)
        {
            cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: Can't get EDID of DP single mode device!\n", FUNCTION_NAME));
            cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: Use fail-safe EDID!\n", FUNCTION_NAME));

            cb_memcpy(pDevCommon->EdidData, DPFailSafeModeEdid, sizeof(DPFailSafeModeEdid));
            IsDevChanged = CBIOS_TRUE;
        }
    }
    else
    {
        bConnected = CBIOS_FALSE;
    }

    if(cbDPMonitor_IsEDPSupported(pcbe, DPModuleIndex))
    {
        if (pDevCommon->PowerState != CBIOS_PM_ON)
        {
            cbDPMonitor_EDPAuxPowerSeqCtrl(pcbe, pDPMonitorContext, DPModuleIndex, CBIOS_FALSE);
        }
    }

    if(bConnected)
    {
        if (IsDevChanged)
        {
            cbEDIDModule_ParseEDID(pDevCommon->EdidData, &(pDevCommon->EdidStruct), CBIOS_EDIDDATABYTE);
            cbEDIDModule_SADPatch(&(pDevCommon->EdidStruct));
            cbDPMonitor_HDCP_ReadBKsv(pcbe, pDevCommon, ((PCBIOS_HDCP_CONTEXT)pDevCommon->pHDCPContext)->BKsv,
                                        &(((PCBIOS_HDCP_CONTEXT)pDevCommon->pHDCPContext)->bRepeater));
        }
        cbDPMonitor_GetMonitorParamsFromEdid(pcbe, pDPMonitorContext, IsDevChanged);
    }
    else
    {
        //for the case:if plugout monitor before suspend,and then plugin hdmi monitor before resume,after resume,4k@60 can't be lighted.
        //rootcause:if plugout monitor,hpd thread will detect no device,so some attributes stored in pDevCommon will be cleared,
        //plugin monitor before resume,os will do nothing,so it will not update attributes stored in pDevCommon
        //then after resume,driver will not enter some hdmi module related codes,so monitor can't light
        //so not memset pDevCommon->EdidStruct when device is not connected
        cbClearEdidRelatedData(pcbe, pDevCommon);
        cb_memset(((PCBIOS_HDCP_CONTEXT)pDevCommon->pHDCPContext)->BKsv, 0, sizeof(((PCBIOS_HDCP_CONTEXT)pDevCommon->pHDCPContext)->BKsv));
    }

    //restore mm 8340 value
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, 0);
    // restore mm8348 value
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MISC[DPModuleIndex], DPEphyMiscRegValue.Value, 0);
    // HW uses AUX
    cbDIU_DP_HWUseAuxChannel(pcbe, DPModuleIndex);

    if(bConnected && cbDPMonitor_IsEDPSupported(pcbe, DPModuleIndex))
    {
        if(cbEDPPanel_GetPanelDescriptor(pcbe, pDevCommon->EdidData) != CBIOS_NULL)
        {
            cb_memcpy(pPanelDesc, cbEDPPanel_GetPanelDescriptor(pcbe, pDevCommon->EdidData), sizeof(CBIOS_EDP_PANEL_DESC));
            cbEDPPanel_Init(pcbe, pPanelDesc);
        }
    }
EXIT:

    cb_ReleaseMutex(pcbe->pAuxMutex);

    return bConnected;
}

CBIOS_VOID cbDPMonitor_UpdateModeInfo(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_DISP_MODE_PARAMS pModeParams)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32 IGAIndex = pModeParams->IGAIndex;

    if (0 == pcbe->SpecifyDestTimingSrc[IGAIndex].Flag)
    {
        if (CBIOS_RGBOUTPUT == pModeParams->TargetModePara.OutputSignal)
        {
            pDPMonitorContext->ColorFormat = 0;
        }
        else if(CBIOS_YCBCR422OUTPUT == pModeParams->TargetModePara.OutputSignal)
        {
            pDPMonitorContext->ColorFormat = 1;
        }
        else if(CBIOS_YCBCR444OUTPUT == pModeParams->TargetModePara.OutputSignal)
        {
            pDPMonitorContext->ColorFormat = 2;
        }
        // YUV420 -- may be need to change here in future
        else if(CBIOS_YCBCR420OUTPUT == pModeParams->TargetModePara.OutputSignal)
        {
            cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: DP doesn't support YUV420 output up to DP1.2\n", FUNCTION_NAME));
            pDPMonitorContext->ColorFormat = 0;
        }
    }

    // save timing and interlace flag to DP monitor context for link training used
    cb_memcpy(&(pDPMonitorContext->TargetTiming), &(pModeParams->TargetTiming), sizeof(CBIOS_TIMING_ATTRIB));
    pDPMonitorContext->bInterlace = pModeParams->TargetModePara.bInterlace;
    pDPMonitorContext->PixelRepetition = pModeParams->PixelRepitition;
}

CBIOS_VOID cbDPMonitor_SetMode(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_DISP_MODE_PARAMS pModeParams)
{
    PCBIOS_EXTENSION_COMMON pcbe          = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DEVICE_COMMON    pDevCommon    = pDPMonitorContext->pDevCommon;
    CBIOS_MODULE_INDEX      DPModuleIndex = cbGetModuleIndex(pcbe, pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    CBIOS_U8                HVPolarity    = pModeParams->TargetTiming.HVPolarity;

    if (DPModuleIndex == CBIOS_MODULE_INDEX_INVALID)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return;
    }

    cbDIU_DP_SetHVSync(pcbe, DPModuleIndex, HVPolarity);
}

CBIOS_VOID cbDPMonitor_QueryAttribute(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBiosMonitorAttribute pMonitorAttribute)
{
    PCBIOS_EXTENSION_COMMON pcbe          = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_EDP_PANEL_DESC pEDPPanelDesc = &(pDPMonitorContext->pDevCommon->DeviceParas.EDPPanelDevice.EDPPanelDesc);
    CBIOS_MODULE_INDEX      DPModuleIndex = cbGetModuleIndex(pcbe, pDPMonitorContext->pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);

    if (pDPMonitorContext->bpc == 6)
    {
        pMonitorAttribute->SupportBPC.IsSupport6BPC = CBIOS_TRUE;
    }
    else if (pDPMonitorContext->bpc == 8)
    {
        pMonitorAttribute->SupportBPC.IsSupport8BPC = CBIOS_TRUE;
    }
    else if (pDPMonitorContext->bpc == 10)
    {
        pMonitorAttribute->SupportBPC.IsSupport10BPC = CBIOS_TRUE;
    }
    else if (pDPMonitorContext->bpc == 12)
    {
        pMonitorAttribute->SupportBPC.IsSupport12BPC = CBIOS_TRUE;
    }
    else if (pDPMonitorContext->bpc == 14)
    {
        pMonitorAttribute->SupportBPC.IsSupport14BPC = CBIOS_TRUE;
    }
    else if (pDPMonitorContext->bpc == 16)
    {
        pMonitorAttribute->SupportBPC.IsSupport16BPC = CBIOS_TRUE;
    }

    if(cbDPMonitor_IsEDPSupported(pcbe, DPModuleIndex) && pEDPPanelDesc)
    {
        pMonitorAttribute->bSupportBLCtrl = pEDPPanelDesc->EDPCaps.isBLCtrlSupport;
    }
}

CBIOS_VOID cbDPMonitor_OnOff(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, CBIOS_BOOL bOn)
{
    PCBIOS_EXTENSION_COMMON pcbe          = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DEVICE_COMMON    pDevCommon    = pDPMonitorContext->pDevCommon;
    CBIOS_MODULE_INDEX      DPModuleIndex = cbGetModuleIndex(pcbe, pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    PCBIOS_EDP_PANEL_DESC   pEDPPanelDesc = &(pDevCommon->DeviceParas.EDPPanelDevice.EDPPanelDesc);
    CBIOS_BOOL              bEDPMode      = cbDPMonitor_IsEDPSupported(pcbe, DPModuleIndex);

    cb_AcquireMutex(pcbe->pAuxMutex);

    if (bOn)
    {
        cbDPMonitor_SetDither(pcbe, pDPMonitorContext->bpc, CBIOS_TRUE, DPModuleIndex);
        // 1. start EDP panel power supply
        if (bEDPMode)
        {
            cbDPMonitor_EDPAuxPowerSeqCtrl(pcbe, pDPMonitorContext, DPModuleIndex, bOn);
        }

        // 2. turn on DP EPHY
        cbPHY_DP_DPModeOnOff(pcbe, DPModuleIndex, pDPMonitorContext->LinkPassSpeed, bOn);

        // 3. Set Sink device to D0(normal operation mode) state
        cbDPMonitor_SetSinkPowerState(pcbe, DPModuleIndex, CBIOS_TRUE);
        cbDelayMilliSeconds(2);

        // 4. turn off DP video before Link Training
        cbDIU_DP_VideoOnOff(pcbe, DPModuleIndex, CBIOS_FALSE);

        // 5. do Link Training
        cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: ON. Need to do Link Training !\n", FUNCTION_NAME));
        if (cbDPMonitor_LinkTraining(pcbe, pDPMonitorContext, CBIOS_FALSE))
        {
            CBIOS_BOOL status;
            status = cbDPMonitor_SetUpMainLink(pcbe, pDPMonitorContext);
            if(!status)
            {
                cbDebugPrint((MAKE_LEVEL(DP, WARNING),"%s: setting up Main Link failed!\n", FUNCTION_NAME));
            }
        }
        else
        {
            // Even if link training failed, still need to set correct DP path since sink would try to request link training again via hot IRQ.
            cbDebugPrint((MAKE_LEVEL(DP, WARNING),"%s: skip setting up Main Link because Link Training failed!\n", FUNCTION_NAME));
        }

        cbDelayMilliSeconds(2);

        // 6. turn on DP video after Link Training
        cbDIU_DP_EnableVideoAudio(pcbe, DPModuleIndex);

        // 7. turn on EDP panel back light
        if (bEDPMode)
        {
            cbEDPPanel_OnOff(pcbe, bOn, pEDPPanelDesc);
        }
    }
    else
    {
        // 1. turn off EDP panel back light
        if (bEDPMode)
        {
            cbEDPPanel_OnOff(pcbe, bOn, pEDPPanelDesc);
            cbDIU_EDP_ControlVEESignal(pcbe, pDPMonitorContext, DPModuleIndex, bOn);
            cbDelayMilliSeconds(2);
        }

        //disable both Video and Audio during link training
        cbDIU_DP_DisableVideoAudio(pcbe, DPModuleIndex);

        // 2. reset Link Training
        cbDIU_DP_ResetLinkTraining(pcbe, DPModuleIndex);
        pDPMonitorContext->LT_Status = 0;

        // 3. Set Sink device to D3 (power saving) state
        cbDPMonitor_SetSinkPowerState(pcbe, DPModuleIndex, CBIOS_FALSE);

        // 4. turn off DP EPHY
        cbPHY_DP_DPModeOnOff(pcbe, DPModuleIndex, pDPMonitorContext->LinkPassSpeed, bOn);

        // 5. reset AUX
        cbDIU_DP_ResetAUX(pcbe, DPModuleIndex);

        // 6. stop EDP panel power supply
        if (bEDPMode)
        {
            cbDPMonitor_EDPAuxPowerSeqCtrl(pcbe, pDPMonitorContext, DPModuleIndex, bOn);
        }

        cbDPMonitor_SetDither(pcbe, pDPMonitorContext->bpc, CBIOS_FALSE, DPModuleIndex);
    }

    cb_ReleaseMutex(pcbe->pAuxMutex);
}

static CBIOS_BOOL cbDPMonitor_ProcTestEdidRead(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext)
{
    CBIOS_MODULE_INDEX DPModuleIndex = cbGetModuleIndex(pcbe, pDPMonitorContext->pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    AUX_CONTROL    AUX = {0};
    DPCD_REG_00260 DPCD00260;
    CBIOS_BOOL     bStatus = CBIOS_TRUE;
    PCBIOS_U8      pEDIDData = CBIOS_NULL;

    // Write Ack to test response fields of DPCD
    DPCD00260.Value = 0;
    DPCD00260.TEST_ACK = 1;
    AUX.Function = CBIOS_AUX_REQUEST_NATIVE_WRITE;
    AUX.Offset = 0x260;
    AUX.Length = 1;
    AUX.Data[0] = DPCD00260.Value;
    if(!cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
    {
        cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: Write ack to TEST_RESPONSE fields of DPCD!\n", FUNCTION_NAME));
        bStatus = CBIOS_FALSE;
    }

    // 4.2.2.3
    // 4.2.2.4
    // 4.2.2.5
    pEDIDData = cb_AllocateNonpagedPool(CBIOS_EDIDDATABYTE);
    if(pEDIDData == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: pEDIDData allocate error.\n", FUNCTION_NAME));
        return CBIOS_FALSE;
    }

    if(cbDPMonitor_AuxReadEDID(pcbe, pDPMonitorContext, pEDIDData, CBIOS_EDIDDATABYTE))
    {
        cb_memcpy(pDPMonitorContext->pDevCommon->EdidData, pEDIDData, CBIOS_EDIDDATABYTE);
    }
    else
    {
        // CTS item: 4.2.2.4
        cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: Automated Test: EDID Corruption Test, use fake safe-mode EDID.\n", FUNCTION_NAME));
        cb_memcpy(pDPMonitorContext->pDevCommon->EdidData, DPFailSafeModeEdid, sizeof(DPFailSafeModeEdid));
    }

    pDPMonitorContext->pDevCommon->isFakeEdid = CBIOS_TRUE;

    cb_FreePool(pEDIDData);

    return bStatus;
}

static CBIOS_BOOL cbDPMonitor_ProcTestLinkTraining(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext)
{
    CBIOS_MODULE_INDEX DPModuleIndex = cbGetModuleIndex(pcbe, pDPMonitorContext->pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    AUX_CONTROL    AUX = {0};
    DPCD_REG_00260 DPCD00260;
    CBIOS_BOOL     bStatus = CBIOS_TRUE;

    // Write Ack to test response fields of DPCD
    DPCD00260.Value = 0;
    DPCD00260.TEST_ACK = 1;
    AUX.Function = CBIOS_AUX_REQUEST_NATIVE_WRITE;
    AUX.Offset = 0x260;
    AUX.Length = 1;
    AUX.Data[0] = DPCD00260.Value;
    if(!cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
    {
        cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: Write ack to TEST_RESPONSE fields of DPCD!\n", FUNCTION_NAME));
        bStatus = CBIOS_FALSE;
    }
    //4.3.1.2
    //4.3.1.3
    //4.3.1.4
    //4.3.1.5
    //4.3.1.6
    //4.3.1.7
    //4.3.1.8
    //4.3.1.9
    //4.3.1.10
    //4.3.1.11
    //4.3.1.12

    cbDPMonitor_LinkTraining(pcbe, pDPMonitorContext, CBIOS_TRUE);

    pDPMonitorContext->LT_Status = 1;

    return bStatus;
}

static CBIOS_BOOL cbDPMonitor_GetAutoTestDpcdTiming(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext)
{
    CBIOS_MODULE_INDEX DPModuleIndex = cbGetModuleIndex(pcbe, pDPMonitorContext->pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    AUX_CONTROL    AUX = {0};
    DPCD_REG_00219 DPCD00219;
    DPCD_REG_00220 DPCD00220;
    DPCD_REG_00221 DPCD00221;
    DPCD_REG_00222 DPCD00222;
    DPCD_REG_00223 DPCD00223;
    DPCD_REG_00224 DPCD00224;
    DPCD_REG_00225 DPCD00225;
    DPCD_REG_00226 DPCD00226;
    DPCD_REG_00227 DPCD00227;
    DPCD_REG_00228 DPCD00228;
    DPCD_REG_00229 DPCD00229;
    DPCD_REG_0022A DPCD0022A;
    DPCD_REG_0022B DPCD0022B;
    DPCD_REG_0022C DPCD0022C;
    DPCD_REG_0022D DPCD0022D;
    DPCD_REG_0022E DPCD0022E;
    DPCD_REG_0022F DPCD0022F;
    DPCD_REG_00230 DPCD00230;
    DPCD_REG_00231 DPCD00231;
    DPCD_REG_00232 DPCD00232;
    DPCD_REG_00233 DPCD00233;
    DPCD_REG_00234 DPCD00234;
    CBIOS_U32 RefreshRate = 0;
    CBIOS_BOOL     bStatus = CBIOS_TRUE;

    cb_memset(&pDPMonitorContext->TestDpcdDataTiming, 0, sizeof(CBiosCustmizedDestTiming));

    AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
    AUX.Offset = 0x219;
    AUX.Length = 1;
    if(cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
    {
        DPCD00219.Value = AUX.Data[0] & 0x000000FF;
        pDPMonitorContext->TestDpcdDataTiming.LinkRate = DPCD00219.TEST_LINK_RATE;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s(%d): read DPCD reg(Offset = 0x%x, len = %d) fail?\n",
            FUNCTION_NAME, LINE_NUM, AUX.Offset, AUX.Length));
        bStatus = CBIOS_FALSE;
    }

    AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
    AUX.Offset = 0x220;
    AUX.Length = 16;
    if(cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
    {
        DPCD00220.Value = (AUX.Data[0] & 0x000000FF) >> 0;
        DPCD00221.Value = (AUX.Data[0] & 0x0000FF00) >> 8;
        DPCD00222.Value = (AUX.Data[0] & 0x00FF0000) >> 16;
        DPCD00223.Value = (AUX.Data[0] & 0xFF000000) >> 24;
        DPCD00224.Value = (AUX.Data[1] & 0x000000FF) >> 0;
        DPCD00225.Value = (AUX.Data[1] & 0x0000FF00) >> 8;
        DPCD00226.Value = (AUX.Data[1] & 0x00FF0000) >> 16;
        DPCD00227.Value = (AUX.Data[1] & 0xFF000000) >> 24;
        DPCD00228.Value = (AUX.Data[2] & 0x000000FF) >> 0;
        DPCD00229.Value = (AUX.Data[2] & 0x0000FF00) >> 8;
        DPCD0022A.Value = (AUX.Data[2] & 0x00FF0000) >> 16;
        DPCD0022B.Value = (AUX.Data[2] & 0xFF000000) >> 24;
        DPCD0022C.Value = (AUX.Data[3] & 0x000000FF) >> 0;
        DPCD0022D.Value = (AUX.Data[3] & 0x0000FF00) >> 8;
        DPCD0022E.Value = (AUX.Data[3] & 0x00FF0000) >> 16;
        DPCD0022F.Value = (AUX.Data[3] & 0xFF000000) >> 24;

        pDPMonitorContext->TestDpcdDataTiming.LaneCount = DPCD00220.TEST_LANE_COUNT;
        pDPMonitorContext->TestDpcdDataTiming.TestPattern = DPCD00221.TEST_PATTERN;
        pDPMonitorContext->TestDpcdDataTiming.HorTotal = (DPCD00222.TEST_H_TOTAL_15_8 << 8) + DPCD00223.TEST_H_TOTAL_7_0;
        pDPMonitorContext->TestDpcdDataTiming.VerTotal = (DPCD00224.TEST_V_TOTAL_15_8 << 8) + DPCD00225.TEST_V_TOTAL_7_0;
        pDPMonitorContext->TestDpcdDataTiming.HorSyncStart = (DPCD00226.TEST_H_START_15_8 << 8) + DPCD00227.TEST_H_START_7_0;
        pDPMonitorContext->TestDpcdDataTiming.VerSyncStart = (DPCD00228.TEST_V_START_15_8 << 8) + DPCD00229.TEST_V_START_7_0;
        pDPMonitorContext->TestDpcdDataTiming.HSyncPolarity = DPCD0022A.TEST_HSYNC_POLARITY;
        pDPMonitorContext->TestDpcdDataTiming.HorSyncWidth = (DPCD0022A.TEST_HSYNC_WIDTH_14_8 << 8) + DPCD0022B.TEST_HSYNC_WIDTH_7_0;
        pDPMonitorContext->TestDpcdDataTiming.VSyncPolarity = DPCD0022C.TEST_VSYNC_POLARITY;
        pDPMonitorContext->TestDpcdDataTiming.VerSyncWidth = (DPCD0022C.TEST_VSYNC_WIDTH_14_8 << 8) + DPCD0022D.TEST_VSYNC_WIDTH_7_0;
        pDPMonitorContext->TestDpcdDataTiming.HorWidth = (DPCD0022E.TEST_H_WIDTH_15_8 << 8) + DPCD0022F.TEST_H_WIDTH_7_0;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s(%d): read DPCD reg(Offset = 0x%x, len = %d) fail\n",
            FUNCTION_NAME, LINE_NUM, AUX.Offset, AUX.Length));
        bStatus = CBIOS_FALSE;
    }

    AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
    AUX.Offset = 0x230;
    AUX.Length = 5;
    if(cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
    {
        DPCD00230.Value = (AUX.Data[0] & 0x000000FF) >> 0;
        DPCD00231.Value = (AUX.Data[0] & 0x0000FF00) >> 8;
        DPCD00232.Value = (AUX.Data[0] & 0x00FF0000) >> 16;
        DPCD00233.Value = (AUX.Data[0] & 0xFF000000) >> 24;
        DPCD00234.Value = (AUX.Data[1] & 0x000000FF) >> 0;

        pDPMonitorContext->TestDpcdDataTiming.VerWidth = (DPCD00230.TEST_V_WIDTH_15_8 << 8) + DPCD00231.TEST_V_WIDTH_7_0;
        pDPMonitorContext->TestDpcdDataTiming.ClockSynAsyn = DPCD00232.TEST_SYNCHRONOUS_CLOCK;
        pDPMonitorContext->TestDpcdDataTiming.ColorFormat = DPCD00232.TEST_COLOR_FORMAT;
        pDPMonitorContext->TestDpcdDataTiming.DynamicRange = DPCD00232.TEST_DYNAMIC_RANGE;
        pDPMonitorContext->TestDpcdDataTiming.YCbCrCoefficients = DPCD00232.TEST_YCBCR_COEFFICIENTS;
        if (DPCD00232.TEST_BIT_DEPTH == 0)
        {
            pDPMonitorContext->TestDpcdDataTiming.BitDepthPerComponet = 6;
        }
        else if (DPCD00232.TEST_BIT_DEPTH == 1)
        {
            pDPMonitorContext->TestDpcdDataTiming.BitDepthPerComponet = 8;
        }
        else if (DPCD00232.TEST_BIT_DEPTH == 2)
        {
            pDPMonitorContext->TestDpcdDataTiming.BitDepthPerComponet = 10;
        }
        else if (DPCD00232.TEST_BIT_DEPTH == 3)
        {
            pDPMonitorContext->TestDpcdDataTiming.BitDepthPerComponet = 12;
        }
        else if (DPCD00232.TEST_BIT_DEPTH == 4)
        {
            pDPMonitorContext->TestDpcdDataTiming.BitDepthPerComponet = 16;
        }
        pDPMonitorContext->TestDpcdDataTiming.IsInterlaced = DPCD00233.TEST_INTERLACED;
        RefreshRate = DPCD00234.TEST_REFRESH_RATE_NUMERATOR;
        pDPMonitorContext->TestDpcdDataTiming.DClk = pDPMonitorContext->TestDpcdDataTiming.HorTotal * pDPMonitorContext->TestDpcdDataTiming.VerTotal
            * RefreshRate / 100;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s(%d): read DPCD reg(Offset = 0x%x, len = %d) fail?\n",
            FUNCTION_NAME, LINE_NUM, AUX.Offset, AUX.Length));
        bStatus = CBIOS_FALSE;
    }

    // Enhanced Frame Mode supported flag cannot be gotten from DPCD Test Request Field
    pDPMonitorContext->TestDpcdDataTiming.EnhancedFrameMode = pDPMonitorContext->bSupportEnhanceMode;

    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: LinkRate = 0x%x\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.LinkRate));
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: LaneCount = %d\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.LaneCount));
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: TestPattern = %d\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.TestPattern));
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: ClockSynAsyn = %d\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.ClockSynAsyn));
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: DynamicRange = %d\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.DynamicRange));
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: ColorFormat = %d\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.ColorFormat));
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: YCbCrCoefficients = %d\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.YCbCrCoefficients));
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: EnhancedFrameMode = %d\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.EnhancedFrameMode));
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: BitDepthPerComponet = %d\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.BitDepthPerComponet));
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: IsInterlaced = %d\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.IsInterlaced));
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: HorTotal = %d\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.HorTotal));
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: HorWidth = %d\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.HorWidth));
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: HorSyncStart = %d\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.HorSyncStart));
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: HorSyncWidth = %d\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.HorSyncWidth));
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: HSyncPolarity = %d\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.HSyncPolarity));
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: VerTotal = %d\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.VerTotal));
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: VerWidth = %d\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.VerWidth));
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: VerSyncStart = %d\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.VerSyncStart));
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: VerSyncWidth = %d\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.VerSyncWidth));
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: VSyncPolarity = %d\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.VSyncPolarity));
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: DClk = %d\n", FUNCTION_NAME, pDPMonitorContext->TestDpcdDataTiming.DClk));

    return bStatus;
}

static CBIOS_BOOL cbDPMonitor_ProcTestPattern(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext)
{
    CBIOS_MODULE_INDEX DPModuleIndex = cbGetModuleIndex(pcbe, pDPMonitorContext->pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    AUX_CONTROL    AUX = {0};
    DPCD_REG_00260 DPCD00260;
    CBIOS_BOOL     bStatus = CBIOS_FALSE;

    //4.4.1.1
    //4.4.1.2
    //4.4.1.3
    //4.4.2
    // Currently not support automated test for pattern test
    // Clear TRAINING_PATTERN_SET byte
    // Write NACK to test response fields of DPCD.
    DPCD00260.Value = 0;
    DPCD00260.TEST_NACK = 1;
    AUX.Function = CBIOS_AUX_REQUEST_NATIVE_WRITE;
    AUX.Offset = 0x260;
    AUX.Length = 1;
    AUX.Data[0] = DPCD00260.Value;
    if(!cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
    {
        cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: Write no ack to TEST_RESPONSE fields of DPCD!\n", FUNCTION_NAME));
        bStatus = CBIOS_FALSE;
    }

    //Currently we can not support automation test for pattern test.
    //If we support this feature, the following code path should be went through.
    if(!cbDPMonitor_GetAutoTestDpcdTiming(pcbe, pDPMonitorContext))
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: Can not get request pattern fields of DPCD!\n", FUNCTION_NAME));
        bStatus = CBIOS_FALSE;
    }
    else
    {
        bStatus = CBIOS_TRUE;
    }

    return bStatus;
}

static CBIOS_BOOL cbDPMonitor_HandleTestRequest(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_DP_HANDLE_IRQ_PARA pDPHandleIrqPara)
{
    CBIOS_MODULE_INDEX DPModuleIndex = cbGetModuleIndex(pcbe, pDPMonitorContext->pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    AUX_CONTROL    AUX = {0};
    DPCD_REG_00218 DPCD00218;
    CBIOS_BOOL     bStatus = CBIOS_TRUE;

    // Read Automated detail request.
    AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
    AUX.Offset = 0x218;
    AUX.Length = 1;
    if(cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
    {
        DPCD00218.Value = AUX.Data[0] & 0x000000FF;

        if(DPCD00218.TEST_EDID_READ)
        {
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: Test Edid Read.\n", FUNCTION_NAME));
            if(cbDPMonitor_ProcTestEdidRead(pcbe, pDPMonitorContext))
            {
                pDPHandleIrqPara->bNeedDetect = 1;
                pDPHandleIrqPara->bNeedCompEdid = 1;
            }
        }

        if(DPCD00218.TEST_LINK_TRAINING)
        {
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: Test Link Training.\n", FUNCTION_NAME));
            cbDPMonitor_ProcTestLinkTraining(pcbe, pDPMonitorContext);
        }

        if(DPCD00218.TEST_PATTERN)
        {
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: Test Pattern.\n", FUNCTION_NAME));
            if(cbDPMonitor_ProcTestPattern(pcbe, pDPMonitorContext))
            {
                if (cbDPMonitor_LinkTraining(pcbe, pDPMonitorContext, CBIOS_FALSE))
                {
                    cbDPMonitor_SetUpMainLink(pcbe, pDPMonitorContext);
                }
                pDPHandleIrqPara->bNeedDetect = 1;
                pDPHandleIrqPara->bNeedCompEdid = 1;
            }
        }
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s(%d): why read DPCD reg TEST_REQUEST fail?\n", FUNCTION_NAME, LINE_NUM));
        bStatus = CBIOS_FALSE;
    }

    return bStatus;
}

static CBIOS_BOOL cbDPMonitor_PutNextCBusRecvEvent_locked(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, DP_RECV_EVENT *pRecvEvent)
{
    CBIOS_BOOL bRet = CBIOS_TRUE;
    CBIOS_U64  oldIrql = 0;

    oldIrql = cb_AcquireSpinLock(pcbe->pSpinLock);

    if (!QUEUE_FULL(pDPMonitorContext->RecvQueue))
    {
        if (pRecvEvent != CBIOS_NULL)
        {
            pDPMonitorContext->RecvQueue.Queue[pDPMonitorContext->RecvQueue.Tail] = *pRecvEvent;
            ADVANCE_QUEUE_TAIL(pDPMonitorContext->RecvQueue);
        }

        bRet = CBIOS_TRUE;
    }
    else
    {
        //queue is full
        bRet = CBIOS_FALSE;
    }

    cb_ReleaseSpinLock(pcbe->pSpinLock, oldIrql);
    return bRet;
}

static CBIOS_BOOL cbDPMonitor_IsRecvEventQueueEmpty_locked(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext)
{
    CBIOS_BOOL bRet = CBIOS_TRUE;
    CBIOS_U64  oldIrql = cb_AcquireSpinLock(pcbe->pSpinLock);

    if (QUEUE_EMPTY(pDPMonitorContext->RecvQueue))
    {
        bRet = CBIOS_TRUE;
    }
    else
    {
        bRet = CBIOS_FALSE;
    }

    cb_ReleaseSpinLock(pcbe->pSpinLock, oldIrql);
    return bRet;
}

static CBIOS_BOOL cbDPMonitor_GetNextCBusRecvEvent_locked(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, DP_RECV_EVENT *pRecvEvent)
{
    CBIOS_BOOL bRet = CBIOS_TRUE;
    CBIOS_U64  oldIrql = cb_AcquireSpinLock(pcbe->pSpinLock);

    if (QUEUE_EMPTY(pDPMonitorContext->RecvQueue))
    {
        bRet = CBIOS_FALSE;
    }
    else
    {
        cb_memcpy(pRecvEvent, &(pDPMonitorContext->RecvQueue.Queue[pDPMonitorContext->RecvQueue.Head]), sizeof(DP_RECV_EVENT));
        ADVANCE_QUEUE_HEAD(pDPMonitorContext->RecvQueue)
    }

    cb_ReleaseSpinLock(pcbe->pSpinLock, oldIrql);
    return bRet;
}

static CBIOS_VOID cbDPMonitor_NotifyDPEvent(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, CBIOS_U8 EventCode, CBIOS_U8 EventParam)
{
    CBIOS_DP_NOTIFY_EVENT_PARA NotifyPara;
    NotifyPara.Size = sizeof(CBIOS_DP_NOTIFY_EVENT_PARA);
    NotifyPara.DeviceId = pDPMonitorContext->pDevCommon->DeviceType;
    NotifyPara.EventCode = EventCode;
    NotifyPara.EventParam = EventParam;

    cbTraceEnter(DP);

    if (pDPMonitorContext->Notifications.NotifyDPEvent != CBIOS_NULL)
    {
        pDPMonitorContext->Notifications.NotifyDPEvent(pDPMonitorContext->Notifications.Private, &NotifyPara);
    }

    cbTraceExit(DP);
}


static CBIOS_BOOL cbDPMonitor_ProcAutomatedTestRequest(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext)
{
    CBIOS_MODULE_INDEX DPModuleIndex = cbGetModuleIndex(pcbe, pDPMonitorContext->pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    AUX_CONTROL    AUX = {0};
    DPCD_REG_00218 DPCD00218;
    CBIOS_BOOL     bStatus = CBIOS_TRUE;

    // Read Automated detail request.
    AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
    AUX.Offset = 0x218;
    AUX.Length = 1;
    if(cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
    {
        DPCD00218.Value = AUX.Data[0] & 0x000000FF;

        if(DPCD00218.TEST_EDID_READ)
        {
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: Test Edid Read.\n", FUNCTION_NAME));
            if(cbDPMonitor_ProcTestEdidRead(pcbe, pDPMonitorContext))
            {
                cbDPMonitor_NotifyDPEvent(pcbe, pDPMonitorContext, CBIOS_DP_EVENT_TEST_EDID, 0);
            }
        }

        if(DPCD00218.TEST_LINK_TRAINING)
        {
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: Test Link Training.\n", FUNCTION_NAME));
            cbDPMonitor_ProcTestLinkTraining(pcbe, pDPMonitorContext);
        }

        if(DPCD00218.TEST_PATTERN)
        {
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: Test Pattern.\n", FUNCTION_NAME));
            if(cbDPMonitor_ProcTestPattern(pcbe, pDPMonitorContext))
            {
                if (cbDPMonitor_LinkTraining(pcbe, pDPMonitorContext, CBIOS_FALSE))
                {
                    cbDPMonitor_SetUpMainLink(pcbe, pDPMonitorContext);
                }
                cbDPMonitor_NotifyDPEvent(pcbe, pDPMonitorContext, CBIOS_DP_EVENT_TEST_PATTERN, 0);
            }
        }
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s(%d): why read DPCD reg TEST_REQUEST fail?\n", FUNCTION_NAME, LINE_NUM));
        bStatus = CBIOS_FALSE;
    }

    return bStatus;
}


static CBIOS_BOOL cbDPMonitor_ProcLinkStatusCheck(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext)
{
    CBIOS_MODULE_INDEX DPModuleIndex = cbGetModuleIndex(pcbe, pDPMonitorContext->pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    AUX_CONTROL AUX = {0};
    CBIOS_U32 ulLaneNum, i;
    CBIOS_BOOL bNeedRetraining = CBIOS_FALSE;
    DPCD_REG_00202 DPCD_00202;
    DPCD_REG_00203 DPCD_00203;
    DPCD_REG_00204 DPCD_00204;
    //DPCD_REG_00205 DPCD_00205;
    CBIOS_BOOL bStatus = CBIOS_TRUE;

    //4.3.2.1
    //4.3.2.2
    //4.3.2.3
    //4.3.2.4

    // Link maintenance hot IRQ signal, need to check link/sink status field of DPCD.
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: Not a automated test IRQ, Link maitanance IRQ!\n", FUNCTION_NAME));
    ulLaneNum = pDPMonitorContext->LinkPassLaneNum;

    AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
    AUX.Offset = 0x202;
    AUX.Length = 4;
    if(cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
    {
        DPCD_00202.Value = (AUX.Data[0] & 0x000000FF) >> 0;
        DPCD_00203.Value = (AUX.Data[0] & 0x0000FF00) >> 8;
        DPCD_00204.Value = (AUX.Data[0] & 0x00FF0000) >> 16;
        //DPCD_00205.Value = (AUX.Data[0] & 0xFF000000) >> 24;

        // 20090317_BC Don't rely on register 204[7] because it's cleared once read, and
        //               we have no idea whether here is the first read instance
        //if(DPCD_00204.LINK_STATUS_UPDATED)  // Link status and Adjust request updated since the last read.
        if(pDPMonitorContext->pDevCommon->PowerState == CBIOS_PM_ON)
        {
            for (i = 0; i < ulLaneNum; i++)
            {
                if ((i == 0) && (!DPCD_00202.LANE0_CR_DONE || !DPCD_00202.LANE0_CHANNEL_EQ_DONE || !DPCD_00202.LANE0_SYMBOL_LOCKED))
                {
                    bNeedRetraining = CBIOS_TRUE;
                    break;
                }
                else if ((i == 1) && (!DPCD_00202.LANE1_CR_DONE || !DPCD_00202.LANE1_CHANNEL_EQ_DONE || !DPCD_00202.LANE1_SYMBOL_LOCKED))
                {
                    bNeedRetraining = CBIOS_TRUE;
                    break;
                }
                else if ((i == 2) && (!DPCD_00203.LANE2_CR_DONE || !DPCD_00203.LANE2_CHANNEL_EQ_DONE || !DPCD_00203.LANE2_SYMBOL_LOCKED))
                {
                    bNeedRetraining = CBIOS_TRUE;
                    break;
                }
                else if ((i == 3) && (!DPCD_00203.LANE3_CR_DONE || !DPCD_00203.LANE3_CHANNEL_EQ_DONE || !DPCD_00203.LANE3_SYMBOL_LOCKED))
                {
                    bNeedRetraining = CBIOS_TRUE;
                    break;
                }
            }

            if(!bNeedRetraining)
            {
                // Check lane align status
                if(!DPCD_00204.INTERLANE_ALIGN_DONE)
                {
                    bNeedRetraining = CBIOS_TRUE;
                }
                /*// check receive port status
                else if(!DPCD_00205.RECEIVE_PORT_0_STATUS)
                {
                    bNeedRetraining = CBIOS_TRUE;
                }
                else if(!DPCD_00205.RECEIVE_PORT_1_STATUS)
                {
                    bNeedRetraining = CBIOS_TRUE;
                }*/
            }

            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: bNeedRetraining: %d.\n", FUNCTION_NAME, bNeedRetraining));

            if(bNeedRetraining)
            {
                if (cbDPMonitor_LinkTraining(pcbe, pDPMonitorContext, CBIOS_FALSE))
                {
                    CBIOS_BOOL status;
                    status = cbDPMonitor_SetUpMainLink(pcbe, pDPMonitorContext);
                    if(!status)
                    {
                        cbDebugPrint((MAKE_LEVEL(DP, WARNING),"%s: setting up Main Link failed!\n", FUNCTION_NAME));
                    }
                }
                else
                {
                    cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: skip Setting Up MainLink because Link Training failed!\n", FUNCTION_NAME));
                }

                pDPMonitorContext->LT_Status = 0;
            }
        }
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: Can not get sink link status!\n", FUNCTION_NAME));
        bStatus = CBIOS_FALSE;
    }

    return bStatus;
}

static CBIOS_BOOL cbDPMonitor_ProcRecvEvents_Int(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, const PDP_RECV_EVENT pRecvEvent)
{
    CBIOS_BOOL bRet = CBIOS_TRUE;
    CBIOS_MODULE_INDEX DPModuleIndex = cbGetModuleIndex(pcbe, pDPMonitorContext->pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    const DP_RECV_EVENT RecvEvent = *pRecvEvent;
    AUX_CONTROL    AUX = {0};
    DPCD_REG_00201 DPCD00201;

    cbTraceEnter(DP);

    switch (RecvEvent.HpdStatus)
    {
    case 1:
    {
        if(cbDualModeDetect(pcbe, pDPMonitorContext->pDevCommon))
        {
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: HDMI monitor hotplug in!\n", FUNCTION_NAME));
            cbDPMonitor_NotifyDPEvent(pcbe, pDPMonitorContext, CBIOS_DP_EVENT_HDMI_HOTPLUG_IN, 0);
        }
        else
        {
            // power on aux channel
            cbPHY_DP_SelectEphyMode(pcbe, DPModuleIndex, DP_EPHY_DP_MODE);
            cbPHY_DP_AuxPowerOn(pcbe, DPModuleIndex);

            AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
            AUX.Offset = 0x201;
            AUX.Length = 1;
            if(cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
            {
                DPCD00201.Value = AUX.Data[0] & 0x000000FF;
                if(DPCD00201.AUTOMATED_TEST_REQUEST)
                {
                    // clear AUTOMATED_TEST_REQUEST bit
                    AUX.Function = CBIOS_AUX_REQUEST_NATIVE_WRITE;
                    AUX.Offset = 0x201;
                    AUX.Length = 1;
                    AUX.Data[0] = DPCD00201.Value;
                    cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX);

                    cbDPMonitor_ProcAutomatedTestRequest(pcbe, pDPMonitorContext);
                }
                // normal hotplug event
                else
                {
                    cbDPMonitor_NotifyDPEvent(pcbe, pDPMonitorContext, CBIOS_DP_EVENT_DP_HOTPLUG_IN, 0);
                }
            }
            else
            {
                cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: I2C and Aux all fail, regard it as HDMI monitor hotplug in!\n", FUNCTION_NAME));
                cbDPMonitor_NotifyDPEvent(pcbe, pDPMonitorContext, CBIOS_DP_EVENT_HDMI_HOTPLUG_IN, 0);
            }
        }
    }
    break;

    case 2:
    {
        if(cbDPPort_IsDeviceInDpMode(pcbe, pDPMonitorContext->pDevCommon))
        {
            cbDPMonitor_NotifyDPEvent(pcbe, pDPMonitorContext, CBIOS_DP_EVENT_DP_HOTPLUG_OUT, 0);
        }
        else
        {
            cbDPMonitor_NotifyDPEvent(pcbe, pDPMonitorContext, CBIOS_DP_EVENT_HDMI_HOTPLUG_OUT, 0);
        }
    }
    break;

    case 3:
    {
        // power on aux channel
        cbPHY_DP_SelectEphyMode(pcbe, DPModuleIndex, DP_EPHY_DP_MODE);
        cbPHY_DP_AuxPowerOn(pcbe, DPModuleIndex);

        AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
        AUX.Offset = 0x200;
        AUX.Length = 2;
        if(cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
        {
            DPCD00201.Value = (AUX.Data[0] & 0x0000FF00) >> 8;
            if(DPCD00201.AUTOMATED_TEST_REQUEST)
            {
                // clear AUTOMATED_TEST_REQUEST bit
                AUX.Function = CBIOS_AUX_REQUEST_NATIVE_WRITE;
                AUX.Offset = 0x201;
                AUX.Length = 1;
                AUX.Data[0] = DPCD00201.Value;
                cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX);

                cbDPMonitor_ProcAutomatedTestRequest(pcbe, pDPMonitorContext);
            }
            else
            {
                cbDPMonitor_ProcLinkStatusCheck(pcbe, pDPMonitorContext);
            }
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s(%d): read DPCD reg(Offset = 0x%x, len = %d) fail?\n",
                FUNCTION_NAME, LINE_NUM, AUX.Offset, AUX.Length));
            bRet = CBIOS_FALSE;
        }
    }
    break;

    default:
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "## unkown RecvEvent.HpdStatus = %d\n", RecvEvent.HpdStatus));
        bRet = CBIOS_FALSE;
        break;
    }

    cbTraceExit(DP);
    return bRet;
}

static CBIOS_BOOL cbDPMonitor_ProcRecvEvents(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext)
{
    CBIOS_BOOL bRet = CBIOS_TRUE;
    DP_RECV_EVENT RecvEvent;

    cbTraceEnter(DP);
    cb_AcquireMutex(pcbe->pAuxMutex);

    if(cbDPMonitor_GetNextCBusRecvEvent_locked(pcbe, pDPMonitorContext, &RecvEvent) == CBIOS_TRUE)
    {
        switch (RecvEvent.EventType)
        {
        case 0:
        {
            cbDPMonitor_ProcRecvEvents_Int(pcbe, pDPMonitorContext, &RecvEvent);
        }
        break;

        default:
            cbDebugPrint((MAKE_LEVEL(DP, ERROR), "unkown RecvEvent.EventType\n"));
            break;
        }
    }
    else
    {
        // empty queue
        bRet = CBIOS_FALSE;
    }

    cb_ReleaseMutex(pcbe->pAuxMutex);

    cbTraceExit(DP);
    return bRet;
}

CBIOS_STATUS cbDPMonitor_Isr(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_DP_ISR_PARA pDPIsrPara)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS            Status = CBIOS_OK;
    CBIOS_MODULE_INDEX      DPModuleIndex = cbGetModuleIndex(pcbe, pDPMonitorContext->pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    DP_RECV_EVENT           RecvEvent;
    CBIOS_U32     HPDStatus = 0;
    REG_MM8330  DPAuxTimerRegValue;

    if (DPModuleIndex == CBIOS_MODULE_INDEX_INVALID)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return CBIOS_ER_INTERNAL;
    }
    cb_memset(&RecvEvent, 0, sizeof(DP_RECV_EVENT));

    DPAuxTimerRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_TIMER[DPModuleIndex]);
    HPDStatus = DPAuxTimerRegValue.HPD_Status;

    if (HPDStatus == 0)
    {
        cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s(%d): why got HPD status: %d\n", FUNCTION_NAME, LINE_NUM, HPDStatus));
    }
    else if ((HPDStatus == 1) || (HPDStatus == 2) || (HPDStatus == 3))
    {
        cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s(%d): HPD status: %d\n", FUNCTION_NAME, LINE_NUM, HPDStatus));

        RecvEvent.EventType = 0;
        RecvEvent.HpdStatus = HPDStatus;

        if(!cbDPMonitor_PutNextCBusRecvEvent_locked(pcbe, pDPMonitorContext, &RecvEvent))
        {
            cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s(%d): Recv Queue is Full\n", FUNCTION_NAME, LINE_NUM));
        }
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s(%d): unknown HPD status: %d\n", FUNCTION_NAME, LINE_NUM, HPDStatus));
    }

    return Status;
}

CBIOS_BOOL cbDPMonitor_WorkThreadMainFunc(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_DP_WORKTHREAD_PARA pDPWorkThreadPara)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_BOOL bWorkThreadBusy = CBIOS_FALSE;

    cbTraceEnter(DP);

    if (!cbDPMonitor_IsRecvEventQueueEmpty_locked(pcbe, pDPMonitorContext))
    {
        bWorkThreadBusy = CBIOS_TRUE;
    }
    else
    {
        goto Exit;
    }

    while (cbDPMonitor_ProcRecvEvents(pcbe, pDPMonitorContext));

    cbTraceExit(DP);

Exit:
    return bWorkThreadBusy;
}

CBIOS_STATUS cbDPMonitor_SetNotifications(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_DP_NOTIFICATIONS pDPNotifications)
{
    CBIOS_STATUS            Status = CBIOS_OK;

    cbTraceEnter(DP);

    pDPMonitorContext->Notifications.Size = pDPNotifications->Size;
    pDPMonitorContext->Notifications.DeviceId = pDPNotifications->DeviceId;
    pDPMonitorContext->Notifications.Private = pDPNotifications->Private;
    pDPMonitorContext->Notifications.NotifyDPEvent = pDPNotifications->NotifyDPEvent;

    cbTraceExit(DP);
    return Status;
}

CBIOS_STATUS  cbDPMonitor_GetInt(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_DP_INT_PARA pDPIntPara)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_MODULE_INDEX      DPModuleIndex = cbGetModuleIndex(pcbe, pDPMonitorContext->pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    CBIOS_U32     HPDStatus = 0;
    REG_MM8330  DPAuxTimerRegValue;

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        return CBIOS_ER_INTERNAL;
    }

    pDPIntPara->IntType = CBIOS_DP_INT_STATUS_NO_INT;

    DPAuxTimerRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_AUX_TIMER[DPModuleIndex]);
    HPDStatus = DPAuxTimerRegValue.HPD_Status;

    if (HPDStatus == 1)
    {
        pDPIntPara->IntType = CBIOS_DP_INT_STATUS_IN;
    }
    else if (HPDStatus == 2)
    {
        if(cbDPPort_IsDeviceInDpMode(pcbe, pDPMonitorContext->pDevCommon))
        {
            pDPIntPara->IntType = CBIOS_DP_INT_STATUS_DP_OUT;
        }
        else
        {
            pDPIntPara->IntType = CBIOS_DP_INT_STATUS_HDMI_OUT;
        }
    }
    else if(HPDStatus == 3)
    {
        pDPIntPara->IntType = CBIOS_DP_INT_IRQ;
    }

    return  CBIOS_OK;
}

CBIOS_STATUS cbDPMonitor_HandleIrq(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_DP_HANDLE_IRQ_PARA pDPHandleIrqPara)
{
    CBIOS_STATUS Ret = CBIOS_OK;
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_MODULE_INDEX DPModuleIndex = cbGetModuleIndex(pcbe, pDPMonitorContext->pDevCommon->DeviceType, CBIOS_MODULE_TYPE_DP);
    CBIOS_DP_INT_TYPE   int_type = pDPHandleIrqPara->IntType;
    AUX_CONTROL    AUX = {0};
    DPCD_REG_00201 DPCD00201;
    DP_EPHY_MODE  Mode;

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return CBIOS_ER_INTERNAL;
    }

    pDPHandleIrqPara->bNeedDetect = 0;
    pDPHandleIrqPara->bNeedCompEdid = 0;

    switch (int_type)
    {
    case CBIOS_DP_INT_STATUS_IN:
    {
        if(cbDualModeDetect(pcbe, pDPMonitorContext->pDevCommon))
        {
            cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: HDMI monitor hotplug in!\n", FUNCTION_NAME));
            pDPHandleIrqPara->bNeedDetect = 1;
            pDPHandleIrqPara->bNeedCompEdid = 1;
        }
        else
        {
            cb_AcquireMutex(pcbe->pAuxMutex);
            Mode = cbPHY_DP_GetEphyMode(pcbe, DPModuleIndex);  //save ephy mode
            // power on aux channel
            cbPHY_DP_SelectEphyMode(pcbe, DPModuleIndex, DP_EPHY_DP_MODE);
            cbPHY_DP_AuxPowerOn(pcbe, DPModuleIndex);

            AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
            AUX.Offset = 0x201;
            AUX.Length = 1;
            if(cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
            {
                DPCD00201.Value = AUX.Data[0] & 0x000000FF;
                if(DPCD00201.AUTOMATED_TEST_REQUEST)
                {
                    // clear AUTOMATED_TEST_REQUEST bit
                    AUX.Function = CBIOS_AUX_REQUEST_NATIVE_WRITE;
                    AUX.Offset = 0x201;
                    AUX.Length = 1;
                    AUX.Data[0] = DPCD00201.Value;
                    cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX);

                    cbDPMonitor_HandleTestRequest(pcbe, pDPMonitorContext, pDPHandleIrqPara);
                }
                // normal hotplug event
                else
                {
                    pDPHandleIrqPara->bNeedDetect = 1;
                    pDPHandleIrqPara->bNeedCompEdid = 1;
                }
            }
            else
            {
                pDPHandleIrqPara->bNeedDetect = 1;
                pDPHandleIrqPara->bNeedCompEdid = 1;
            }

            cbPHY_DP_SelectEphyMode(pcbe, DPModuleIndex, Mode);   //restore ephy mode
            cb_ReleaseMutex(pcbe->pAuxMutex);
        }
    }
    break;
    case CBIOS_DP_INT_IRQ:
    {
        cb_AcquireMutex(pcbe->pAuxMutex);

        // power on aux channel
        cbPHY_DP_SelectEphyMode(pcbe, DPModuleIndex, DP_EPHY_DP_MODE);
        cbPHY_DP_AuxPowerOn(pcbe, DPModuleIndex);

        AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
        AUX.Offset = 0x200;
        AUX.Length = 2;
        if(cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
        {
            DPCD00201.Value = (AUX.Data[0] & 0x0000FF00) >> 8;
            if(DPCD00201.AUTOMATED_TEST_REQUEST)
            {
                // clear AUTOMATED_TEST_REQUEST bit
                AUX.Function = CBIOS_AUX_REQUEST_NATIVE_WRITE;
                AUX.Offset = 0x201;
                AUX.Length = 1;
                AUX.Data[0] = DPCD00201.Value;
                cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX);

                cbDPMonitor_HandleTestRequest(pcbe, pDPMonitorContext, pDPHandleIrqPara);
            }
            else
            {
                cbDPMonitor_ProcLinkStatusCheck(pcbe, pDPMonitorContext);
            }
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s(%d): read DPCD reg(Offset = 0x%x, len = %d) fail?\n",
                                       FUNCTION_NAME, LINE_NUM, AUX.Offset, AUX.Length));
            Ret = CBIOS_ER_INTERNAL;
        }

        cb_ReleaseMutex(pcbe->pAuxMutex);
    }
    break;
    default:
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "## unkown RecvEvent.HpdStatus = %d\n", int_type));
        Ret = CBIOS_ER_INVALID_HOTPLUG;
        break;
    }

    return Ret;
}

CBIOS_STATUS cbDPMonitor_GetCustomizedTiming(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_DP_CUSTOMIZED_TIMING pDPCustomizedTiming)
{
    CBIOS_STATUS            Status = CBIOS_OK;
    PCBiosCustmizedDestTiming pCustomizedTiming = &pDPMonitorContext->TestDpcdDataTiming;

    cbTraceEnter(DP);

    if (pCustomizedTiming->HorWidth && pCustomizedTiming->VerWidth)
    {
        cb_memcpy(&pDPCustomizedTiming->CustmizedTiming, pCustomizedTiming, sizeof(CBiosCustmizedDestTiming));
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid Customized Timing\n", FUNCTION_NAME));
        Status = CBIOS_ER_INTERNAL;
    }

    cbTraceExit(DP);
    return Status;
}

CBIOS_VOID cbDPMonitor_SetDither(PCBIOS_VOID pvcbe, CBIOS_U32 bpc, CBIOS_BOOL bOn, CBIOS_MODULE_INDEX DPModuleIndex )
{
    PCBIOS_EXTENSION_COMMON pcbe          = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_SR36 SR36_value, SR36_mask;

    if(bOn)
    {
        switch(bpc)
        {
        case 5:
            SR36_value.Value = 0;
            SR36_value.Dither_Pattern_Select = 1;
            SR36_value.Dither_EN = 1;
            SR36_value.Dither_Bit_Select = 3;
            break;

        case 6:
            SR36_value.Value = 0;
            SR36_value.Dither_Pattern_Select = 1;
            SR36_value.Dither_EN = 1;
            SR36_value.Dither_Bit_Select = 4;
            break;

        case 7:
            SR36_value.Value = 0;
            SR36_value.Dither_Pattern_Select = 1;
            SR36_value.Dither_EN = 1;
            SR36_value.Dither_Bit_Select = 5;
            break;

        case 8:
            SR36_value.Value = 0;
            SR36_value.Dither_Pattern_Select = 1;
            SR36_value.Dither_EN = 1;
            SR36_value.Dither_Bit_Select = 6;
            break;

        case 9:
            SR36_value.Value = 0;
            SR36_value.Dither_Pattern_Select = 1;
            SR36_value.Dither_EN = 1;
            SR36_value.Dither_Bit_Select = 7;
            break;

        default:
            SR36_value.Value = 0;
            break;
        }
    }
    else
    {
        SR36_value.Value = 0;
    }

    SR36_mask.Value = 0xFF;
    SR36_mask.Dither_Pattern_Select = 0;
    SR36_mask.Dither_EN = 0;
    SR36_mask.Dither_Bit_Select = 0;

    if(DPModuleIndex == CBIOS_MODULE_INDEX2)
    {
        cbMMIOWriteReg(pcbe,SR_B_36, SR36_value.Value, SR36_mask.Value);
    }
    else
    {
        cbMMIOWriteReg(pcbe,SR_36, SR36_value.Value, SR36_mask.Value);
    }
}

#endif

