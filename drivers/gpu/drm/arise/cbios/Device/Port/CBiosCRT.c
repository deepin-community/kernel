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
** CRT port interface function implementation.
**
** NOTE:
** 
******************************************************************************/

#include "CBiosChipShare.h"
#include "CBiosCRT.h"

static CBIOS_BOOL cbCRTPort_DeviceDetect(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_BOOL bHardcodeDetected, CBIOS_U32 FullDetect)
{
    PCBIOS_CRT_CONTEXT pCrtContext = container_of(pDevCommon, PCBIOS_CRT_CONTEXT, Common);
    CBIOS_BOOL         bConnected = CBIOS_FALSE;

    if ((pCrtContext == CBIOS_NULL) || (!(pDevCommon->DeviceType & CBIOS_TYPE_CRT)))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return bConnected;
    }

    if ((!bConnected) && (pCrtContext->Common.SupportMonitorType & CBIOS_MONITOR_TYPE_CRT))
    {
        bConnected = cbCRTMonitor_Detect(pcbe, &pCrtContext->CRTMonitorContext, bHardcodeDetected, FullDetect);
    }

    return bConnected;
}

static CBIOS_VOID cbCRTPort_OnOff(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_BOOL bOn)
{
    PCBIOS_CRT_CONTEXT pCrtContext = container_of(pDevCommon, PCBIOS_CRT_CONTEXT, Common);
    CBIOS_U32          IGAIndex = pDevCommon->DispSource.ModuleList.IGAModule.Index;

    if ((pCrtContext == CBIOS_NULL) || (!(pDevCommon->DeviceType & CBIOS_TYPE_CRT)))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "Invalid Para, will skip CRT ON_OFF!\n", FUNCTION_NAME));
        return;
    }

    cbCRTMonitor_OnOff(pcbe, &pCrtContext->CRTMonitorContext, bOn, (CBIOS_U8)IGAIndex);

    pDevCommon->PowerState = (bOn)? CBIOS_PM_ON : CBIOS_PM_OFF;

    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO),"%s: status = %s.\n", FUNCTION_NAME, (bOn)? "On" : "Off"));
}

static CBIOS_VOID cbCRTPort_SetMode(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DISP_MODE_PARAMS pModeParams)
{
    PCBIOS_CRT_CONTEXT pCrtContext = container_of(pDevCommon, PCBIOS_CRT_CONTEXT, Common);
    CBIOS_MONITOR_TYPE MonitorType = pDevCommon->CurrentMonitorType;

    if (MonitorType == CBIOS_MONITOR_TYPE_CRT)
    {
        cbCRTMonitor_SetMode(pcbe, &pCrtContext->CRTMonitorContext, pModeParams);
    }
}

PCBIOS_DEVICE_COMMON cbCRTPort_Init(PCBIOS_VOID pvcbe, PVCP_INFO pVCP, CBIOS_ACTIVE_TYPE DeviceType)
{
    PCBIOS_EXTENSION_COMMON pcbe          = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_CRT_CONTEXT      pCrtContext   = CBIOS_NULL;
    PCBIOS_DEVICE_COMMON    pDeviceCommon = CBIOS_NULL;

    if(DeviceType & ~CBIOS_TYPE_CRT)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"%s: unsupported device type!!!\n", FUNCTION_NAME));
        return CBIOS_NULL;
    }

    pCrtContext = cb_AllocateNonpagedPool(sizeof(CBIOS_CRT_CONTEXT));
   
    if(pCrtContext == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"%s: pCrtContext allocate error!!!\n", FUNCTION_NAME));
        return CBIOS_NULL;
    }

    pDeviceCommon = &pCrtContext->Common;
    pDeviceCommon->DeviceType = DeviceType;
    pDeviceCommon->SupportMonitorType = cbGetSupportMonitorType(pcbe, DeviceType);
    pDeviceCommon->I2CBus = pVCP->CRTCharByte & I2CBUSMASK;
    pDeviceCommon->HPDPin = pVCP->CRTInterruptPort & HPDPORT_MASK;
    pDeviceCommon->CurrentMonitorType = CBIOS_MONITOR_TYPE_NONE;
    pDeviceCommon->PowerState = CBIOS_PM_INVALID;

    cbInitialModuleList(&pDeviceCommon->DispSource.ModuleList);

    pDeviceCommon->pfncbDeviceDetect = (PFN_cbDeviceDetect)cbCRTPort_DeviceDetect;
    pDeviceCommon->pfncbDeviceOnOff = (PFN_cbDeviceOnOff)cbCRTPort_OnOff;
    pDeviceCommon->pfncbDeviceSetMode = (PFN_cbDeviceSetMode)cbCRTPort_SetMode;

    pCrtContext->CRTMonitorContext.pDevCommon = pDeviceCommon;

    return pDeviceCommon;
}

CBIOS_VOID cbCRTPort_DeInit(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon)
{
    PCBIOS_CRT_CONTEXT pCrtContext = container_of(pDevCommon, PCBIOS_CRT_CONTEXT, Common);

    if ((pCrtContext == CBIOS_NULL) || (!(pDevCommon->DeviceType & CBIOS_TYPE_CRT)))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return;
    }

    if (pCrtContext)
    {
        cb_FreePool(pCrtContext);
        pCrtContext = CBIOS_NULL;
    }
}
