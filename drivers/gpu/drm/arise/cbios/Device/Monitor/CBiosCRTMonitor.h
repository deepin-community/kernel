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
** CRT monitor interface function prototype and parameter definition.
**
** NOTE:
** CRT monitor ONLY parameters SHOULD be added to CBIOS_CRT_MONITOR_CONTEXT.
******************************************************************************/

#ifndef _CBIOS_CRT_MONITOR_H_
#define _CBIOS_CRT_MONITOR_H_

#include "../../Display/CBiosDisplayManager.h"
#include "../CBiosDeviceShare.h"

typedef struct _CBIOS_CRT_MONITOR_CONTEXT
{
    PCBIOS_DEVICE_COMMON pDevCommon;
}CBIOS_CRT_MONITOR_CONTEXT, *PCBIOS_CRT_MONITOR_CONTEXT;

CBIOS_BOOL cbCRTMonitor_Detect(PCBIOS_VOID pvcbe, PCBIOS_CRT_MONITOR_CONTEXT pCrtMonitorContext, CBIOS_BOOL bHardcodeDetected, CBIOS_U32 FullDetect);
CBIOS_VOID cbCRTMonitor_SetMode(PCBIOS_VOID pvcbe, PCBIOS_CRT_MONITOR_CONTEXT pCrtMonitorContext, PCBIOS_DISP_MODE_PARAMS pModeParams);
CBIOS_VOID cbCRTMonitor_OnOff(PCBIOS_VOID pvcbe, PCBIOS_CRT_MONITOR_CONTEXT pCrtMonitorContext, CBIOS_BOOL bOn, CBIOS_U8 IGAIndex);

#endif

