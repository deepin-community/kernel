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
** HDTV hw block interface function prototype.
**
** NOTE:
** 
******************************************************************************/

#ifndef _CBIOS_DIU_HDTV_H_
#define _CBIOS_DIU_HDTV_H_

#include "../../Display/CBiosDisplayManager.h"
#include "../../Device/CBiosDeviceShare.h"

#define HDTV_MODU_NUM 4

CBIOS_VOID cbDIU_HDTV_ModuleOnOff(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX HDTVModuleIndex, CBIOS_BOOL bTurnOn);
CBIOS_VOID cbDIU_HDTV_LBBypass(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX HDTVModuleIndex, CBIOS_BOOL isBypass);
CBIOS_VOID cbDoHDTVFuncSetting_Arise(PCBIOS_VOID pvcbe, PCBIOS_DISP_MODE_PARAMS pModeParams, CBIOS_U32 IGAIndex, CBIOS_ACTIVE_TYPE ulDevices);
#endif
