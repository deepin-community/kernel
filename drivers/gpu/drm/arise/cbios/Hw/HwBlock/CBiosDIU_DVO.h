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
** DVO hw block interface function prototype.
**
** NOTE:
**
******************************************************************************/

#ifndef _CBIOS_DIU_DVO_H_
#define _CBIOS_DIU_DVO_H_

#include "../../Device/CBiosDeviceShare.h"

CBIOS_VOID cbDIU_DVO_SetHVSync(PCBIOS_VOID pvcbe, CBIOS_U8 HVPolarity);
CBIOS_VOID cbDIU_DVO_VideoOnOff(PCBIOS_VOID pvcbe, CBIOS_BOOL bOn, CBIOS_U8 IGAIndex);
CBIOS_STATUS cbDIU_DVO_CheckDaughterCardType(PCBIOS_VOID pvcbe, PVCP_INFO pVCP, PCBIOS_VOID pvDvoContext);

#endif
