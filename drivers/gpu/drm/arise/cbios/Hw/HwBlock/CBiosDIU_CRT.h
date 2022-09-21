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
** CRT hw block interface function prototype.
**
** NOTE:
**
******************************************************************************/

#ifndef _CBIOS_DIU_CRT_H_
#define _CBIOS_DIU_CRT_H_

#include "../../Device/CBiosDeviceShare.h"
#include "../../Device/Monitor/CBiosCRTMonitor.h"

CBIOS_VOID cbDIU_CRT_SetHVSync(PCBIOS_VOID pvcbe, CBIOS_U8 HVPolarity, CBIOS_U8 IGAIndex);
CBIOS_VOID cbDIU_CRT_SetDacCsc(PCBIOS_VOID  pvcbe, CSC_FORMAT  InFmt, CSC_FORMAT  OutFmt);
CBIOS_VOID cbDIU_CRT_DACOnOff(PCBIOS_VOID pvcbe, CBIOS_BOOL bOn, CBIOS_U8 IGAIndex);
CBIOS_VOID cbDIU_CRT_SyncOnOff(PCBIOS_VOID pvcbe, CBIOS_BOOL bOn);
CBIOS_BOOL cbDIU_CRT_DACSense(PCBIOS_VOID pvcbe, PCBIOS_CRT_MONITOR_CONTEXT pCrtMonitorContext);

#endif
