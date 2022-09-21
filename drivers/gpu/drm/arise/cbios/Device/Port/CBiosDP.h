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
** DP port interface function prototype and parameter definition.
**
** NOTE:
** DP port ONLY parameters SHOULD be added to CBIOS_DP_CONTEXT.
******************************************************************************/

#ifndef _CBIOS_DP_H_
#define _CBIOS_DP_H_

#include "../CBiosDeviceShare.h"
#include "../Monitor/CBiosDPMonitor.h"
#include "../Monitor/CBiosHDMIMonitor.h"
#include "../../Hw/HwBlock/CBiosPHY_DP.h"

typedef struct _CBIOS_DP_PORT_PARAMS
{
    CBIOS_BOOL   bDualMode;          // 1 (yes), connect to HDMI/DVI via a connector

    DP_EPHY_MODE DPEphyMode;
    CBIOS_BOOL   bRunCTS;
}CBIOS_DP_PORT_PARAMS, *PCBIOS_DP_PORT_PARAMS;

typedef struct _CBIOS_DP_CONTEXT
{
    CBIOS_DEVICE_COMMON         Common;
    CBIOS_DP_PORT_PARAMS        DPPortParams;

    // monitor contexts
    CBIOS_DP_MONITOR_CONTEXT    DPMonitorContext;
    CBIOS_HDMI_MONITOR_CONTEXT  HDMIMonitorContext;
}CBIOS_DP_CONTEXT, *PCBIOS_DP_CONTEXT;


PCBIOS_DEVICE_COMMON cbDPPort_Init(PCBIOS_VOID pvcbe, PVCP_INFO pVCP, CBIOS_ACTIVE_TYPE DeviceType);
CBIOS_VOID cbDPPort_DeInit(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon);
CBIOS_BOOL cbDPPort_IsDeviceInDpMode(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon);
PCBIOS_VOID cbDPPort_GetDPMonitorContext(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon);
PCBIOS_VOID cbDPPort_GetHDMIMonitorContext(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon);
CBIOS_STATUS cbDPPort_Isr(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_ISR_PARA pDPIsrPara);
CBIOS_BOOL cbDPPort_WorkThreadMainFunc(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_WORKTHREAD_PARA pDPWorkThreadPara);
CBIOS_STATUS cbDPPort_SetNotifications(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_NOTIFICATIONS pDPNotifications);
CBIOS_STATUS  cbDPPort_GetInt(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_INT_PARA  pDPIntPara);
CBIOS_STATUS  cbDPPort_HandleIrq(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_HANDLE_IRQ_PARA pDPHandleIrqPara);
CBIOS_STATUS cbDPPort_GetCustomizedTiming(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_CUSTOMIZED_TIMING pDPCustomizedTiming);
#endif
