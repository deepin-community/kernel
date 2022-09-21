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
** DP PHY interface function prototype and parameter definition.
**
** NOTE:
**
******************************************************************************/

#ifndef _CBIOS_PHY_DP_H_
#define _CBIOS_PHY_DP_H_

#include "../../Device/CBiosDeviceShare.h"

#define DP1_EPHYINITIALIZED  0x0A
#define DP2_EPHYINITIALIZED  0xA0

typedef enum _DP_EPHY_MODE
{
    DP_EPHY_MODE_UNINITIALIZED = 0,
    DP_EPHY_TMDS_MODE,
    DP_EPHY_DP_MODE,
    DP_EHPY_MODE_LAST
}DP_EPHY_MODE;

CBIOS_BOOL cbPHY_DP_IsEPHYInitialized(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex);
CBIOS_VOID cbPHY_DP_SetEPHYInitialized(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex);
CBIOS_VOID cbPHY_DP_InitEPHY(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex);
CBIOS_VOID cbPHY_DP_DeInitEPHY(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex);
CBIOS_VOID cbPHY_DP_SelectPhySource(PCBIOS_VOID pvcbe, DP_EPHY_MODE DPEphyMode, PCBIOS_DISPLAY_SOURCE pDispSource, CBIOS_MONITOR_TYPE MonitorType);
CBIOS_VOID cbPHY_DP_SelectEphyMode(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, DP_EPHY_MODE DPEphyMode);
DP_EPHY_MODE cbPHY_DP_GetEphyMode(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex);
CBIOS_VOID cbPHY_DP_DPModeOnOff(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex,  CBIOS_U32 LinkSpeed, CBIOS_BOOL status);
CBIOS_VOID cbPHY_DP_DualModeOnOff(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_U32 ClockFreq, CBIOS_BOOL bTurnOn);
CBIOS_VOID cbPHY_DP_AuxPowerOn(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex);
#endif
