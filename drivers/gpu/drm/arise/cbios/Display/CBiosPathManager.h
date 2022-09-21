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
** CBios path manager interface function prototype.
**
** NOTE:
**The hw dependent function or structure SHOULD NOT be added to this file.
******************************************************************************/

#ifndef _CBIOS_PATH_MANAGER_H_
#define _CBIOS_PATH_MANAGER_H_

#include "../Device/CBiosShare.h"

//***************************************************************
//               DIU module definition
//***************************************************************
#define CBIOS_MAX_MODULE_NUM       16

typedef enum _CBIOS_MODULE_TYPE
{
    CBIOS_MODULE_TYPE_NONE,
    CBIOS_MODULE_TYPE_DP,
    CBIOS_MODULE_TYPE_MHL,
    CBIOS_MODULE_TYPE_HDMI,
    CBIOS_MODULE_TYPE_HDTV,
    CBIOS_MODULE_TYPE_HDCP,
    CBIOS_MODULE_TYPE_HDAC,
    CBIOS_MODULE_TYPE_IGA
}CBIOS_MODULE_TYPE, *PCBIOS_MODULE_TYPE;

typedef enum _CBIOS_MODULE_INDEX
{
    CBIOS_MODULE_INDEX1,
    CBIOS_MODULE_INDEX2,
    CBIOS_MODULE_INDEX3,
    CBIOS_MODULE_INDEX4,
    CBIOS_MODULE_NUM,
    CBIOS_MODULE_INDEX_INVALID = 0xFF
}CBIOS_MODULE_INDEX, *PCBIOS_MODULE_INDEX;

typedef struct _CBIOS_MODULE
{
    CBIOS_MODULE_TYPE  Type;
    CBIOS_MODULE_INDEX Index;
}CBIOS_MODULE, *PCBIOS_MODULE;

typedef struct _CBIOS_MODULE_LIST
{
    CBIOS_MODULE DPModule;
    CBIOS_MODULE MHLModule;

    CBIOS_MODULE HDCPModule;
    CBIOS_MODULE HDMIModule;
    CBIOS_MODULE HDACModule;
    CBIOS_MODULE HDTVModule;
    CBIOS_MODULE IGAModule;
}CBIOS_MODULE_LIST, *PCBIOS_MODULE_LIST;

typedef struct _CBIOS_DISPLAY_SOURCE
{
    CBIOS_MODULE_LIST       ModuleList;
    CBIOS_MODULE           *ModulePath[CBIOS_MAX_MODULE_NUM]; // Link modules from PHY module to IGA module
    CBIOS_BOOL              bIsSrcChanged;   // If any parameters listed above changes, this variable must be set to CBIOS_TRUE
                                             // When source is updated, this variable will be set to CBIOS_FALSE
}CBIOS_DISPLAY_SOURCE, *PCBIOS_DISPLAY_SOURCE;

CBIOS_STATUS cbPathMgrGetDevComb(PCBIOS_VOID pvcbe, PCBIOS_GET_DEV_COMB pDevComb);
CBIOS_STATUS cbPathMgrGetIgaMask(PCBIOS_VOID pvcbe, PCBIOS_GET_IGA_MASK pGetIgaMask);
CBIOS_VOID cbPathMgrSelectDIUPath(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device, CBIOS_U32 IGAIndex, PCBiosDestModeParams pDestModeParams);
CBIOS_MODULE_INDEX cbGetModuleIndex(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device, CBIOS_MODULE_TYPE ModuleType);
CBIOS_BOOL cbPathMgrSelectDIUSource(PCBIOS_VOID pvcbe, CBIOS_DISPLAY_SOURCE *pSource);
CBIOS_BOOL cbPathMgrModuleOnOff(PCBIOS_VOID pvcbe, CBIOS_MODULE **pModulePath, CBIOS_BOOL bTurnOn);
#endif
