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
** CRT monitor interface function implementation.
**
** NOTE:
** 
******************************************************************************/

#include "CBiosChipShare.h"
#include "CBiosCRTMonitor.h"
#include "../../Hw/HwBlock/CBiosDIU_CRT.h"

CBIOS_BOOL cbCRTMonitor_Detect(PCBIOS_VOID pvcbe, PCBIOS_CRT_MONITOR_CONTEXT pCrtMonitorContext, CBIOS_BOOL bHardcodeDetected, CBIOS_U32 FullDetect)
{
    PCBIOS_EXTENSION_COMMON pcbe         = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DEVICE_COMMON    pDevCommon   = pCrtMonitorContext->pDevCommon;
    CBIOS_BOOL              bGotEdid     = CBIOS_FALSE;
    CBIOS_BOOL              IsDevChanged = CBIOS_FALSE;
    CBIOS_BOOL              bConnected   = CBIOS_FALSE;

    if (bHardcodeDetected)
    {
        pDevCommon->CurrentMonitorType = CBIOS_MONITOR_TYPE_CRT;
        bConnected = CBIOS_TRUE;
        goto EXIT;
    }

    bGotEdid = cbGetDeviceEDID(pcbe, pDevCommon, &IsDevChanged, FullDetect);

    if ((*(pDevCommon->EdidData + EDID_VIDEO_INPUT_DEF_BYTE_OFFSET)) & EDID_VIDEO_INPUT_DEF_DIGITAL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: why come here??? Current device should be a CRT monitor!\n", FUNCTION_NAME));
        bGotEdid = CBIOS_FALSE;
    }

    if (bGotEdid)
    {
        if (IsDevChanged)
        {
            cbEDIDModule_ParseEDID(pDevCommon->EdidData, &pDevCommon->EdidStruct, CBIOS_EDIDDATABYTE);
        }

        pDevCommon->CurrentMonitorType = CBIOS_MONITOR_TYPE_CRT;
        pDevCommon->isFakeEdid = CBIOS_FALSE;
        bConnected = CBIOS_TRUE;
    }
    else
    {
        //for the case:if plugout monitor before suspend,and then plugin hdmi monitor before resume,after resume,4k@60 can't be lighted.
        //rootcause:if plugout monitor,hpd thread will detect no device,so some attributes stored in pDevCommon will be cleared,
        //plugin monitor before resume,os will do nothing,so it will not update attributes stored in pDevCommon
        //then after resume,driver will not enter some hdmi module related codes,so monitor can't light
        //so not memset pDevCommon->EdidStruct when device is not connected
        cbClearEdidRelatedData(pcbe, pDevCommon);

        if (cbDIU_CRT_DACSense(pcbe, pCrtMonitorContext))
        {
            pDevCommon->CurrentMonitorType = CBIOS_MONITOR_TYPE_CRT;
            bConnected = CBIOS_TRUE;
        }
        else
        {
            pDevCommon->CurrentMonitorType = CBIOS_MONITOR_TYPE_NONE;
            bConnected = CBIOS_FALSE;
        }
    }

EXIT:
    return bConnected;
}

CBIOS_VOID cbCRTMonitor_SetMode(PCBIOS_VOID pvcbe, PCBIOS_CRT_MONITOR_CONTEXT pCrtMonitorContext, PCBIOS_DISP_MODE_PARAMS pModeParams)
{
    CBIOS_U8 HVPolarity = pModeParams->TargetTiming.HVPolarity;
    CBIOS_U8 IGAIndex = (CBIOS_U8)pModeParams->IGAIndex;

    cbDIU_CRT_SetHVSync(pvcbe, HVPolarity, IGAIndex);

    cbDIU_CRT_SetDacCsc(pvcbe, CSC_FMT_RGB, pModeParams->IGAOutColorSpace);
}

CBIOS_VOID cbCRTMonitor_OnOff(PCBIOS_VOID pvcbe, PCBIOS_CRT_MONITOR_CONTEXT pCrtMonitorContext, CBIOS_BOOL bOn, CBIOS_U8 IGAIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;

    cbDIU_CRT_DACOnOff(pcbe, bOn, IGAIndex);
    cbDIU_CRT_SyncOnOff(pcbe, bOn);
}
