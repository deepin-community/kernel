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
** CRT port interface function prototype and CBIOS_CRT_CONTEXT definition.
**
** NOTE:
** CRT port ONLY parameters SHOULD be added to CBIOS_CRT_CONTEXT.
******************************************************************************/

#ifndef _CBIOS_CRT_H_
#define _CBIOS_CRT_H_

#include "../Monitor/CBiosCRTMonitor.h"

typedef struct _CBIOS_CRT_CONTEXT
{
    CBIOS_DEVICE_COMMON        Common;

    // monitor contexts
    CBIOS_CRT_MONITOR_CONTEXT  CRTMonitorContext;
}CBIOS_CRT_CONTEXT, *PCBIOS_CRT_CONTEXT;

PCBIOS_DEVICE_COMMON cbCRTPort_Init(PCBIOS_VOID pvcbe, PVCP_INFO pVCP, CBIOS_ACTIVE_TYPE DeviceType);
CBIOS_VOID cbCRTPort_DeInit(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon);
#endif
