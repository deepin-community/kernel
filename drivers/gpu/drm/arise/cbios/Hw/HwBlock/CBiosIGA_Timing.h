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
** IGA block interface prototype.
**
** NOTE:
** This header file CAN ONLY be included by hw layer those files under Hw folder. 
******************************************************************************/

#ifndef _CBIOS_IGA_TIMING_H_
#define _CBIOS_IGA_TIMING_H_

CBIOS_VOID cbProgramDclk(PCBIOS_VOID pvcbe, CBIOS_U8 IGAIndex, CBIOS_U32 ClockFreq);
CBIOS_VOID cbIGA_HW_SetMode(PCBIOS_VOID pvcbe, PCBIOS_DISP_MODE_PARAMS pModeParams);
CBIOS_VOID cbIGA_UpdateActiveDeviceToReg(PCBIOS_VOID pvcbe, PCBIOS_DISPLAY_MANAGER pDispMgr);
CBIOS_U32  cbIGA_GetActiveDeviceFromReg(PCBIOS_VOID pvcbe);

#endif
