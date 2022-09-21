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
** chip functions prototype and parameter definition.
**
** NOTE:
** This header file CAN ONLY be included by hw layer those files under Hw folder. 
******************************************************************************/

#ifndef _CBIOS_E3K_H_
#define _CBIOS_E3K_H_

#include "../../Device/CBiosChipShare.h"
#include "../CBiosHwShare.h"

#define CBIOS_DP5_HPD_BIT_MASK_ELT    (BIT7)
#define CBIOS_DP6_HPD_BIT_MASK_ELT    (BIT4)
#define CBIOS_ANALOG_HPD_BIT_MASK_ELT (BIT25)

#define FP_DUAL_CHANNEL             BIT0       //1=Dual channel, 0=Single channel
#define FP_SINGLE_CHANNEL           0
#define FP_24_BIT                   BIT1       //1=24 bit color mapping mode, 0=18 bit
#define FP_18_BIT                   0
#define ENABLE_DITHER               BIT2       //1=Enable dither, 0=Disable dither
#define DISABLE_DITHER              0
#define MSB_ColorMapping            BIT3       //1=MSB, 0=LSB
#define LSB_ColorMapping            0

#define PANELTABLE_PANELCHAR                        (LSB_ColorMapping + ENABLE_DITHER + FP_18_BIT + FP_SINGLE_CHANNEL) 
#define PANEL_WITH_EDID_PANELCHAR                   (LSB_ColorMapping + ENABLE_DITHER + FP_18_BIT + FP_SINGLE_CHANNEL) 
#define DefaultSSCRange             300

typedef  union  _CBIOS_FLIP_MODE_EX
{
    CBIOS_U32  Value;
    struct 
    {
        CBIOS_U32  FlipType       : 3;
        CBIOS_U32  FlipImme       : 1;
        CBIOS_U32  TrigOvlMask    : 4;
        CBIOS_U32  TrigStreamMask : 4;
        CBIOS_U32  Reserved       : 20;
    };
}CBIOS_FLIP_MODE_EX, *PCBIOS_FLIP_MODE_EX;

typedef struct _CBIOS_TIMING_REG_Arise
{
    CBIOS_U32    DCLK;           
    CBIOS_U8     HVPolarity;     
    CBIOS_U16    HorTotal;    
    CBIOS_U16    HorDisEnd;     
    CBIOS_U8     HDERemainder;
    CBIOS_U16    HorBStart;     
    CBIOS_U16    HorBEnd;     
    CBIOS_U8     HSyncRemainder;
    CBIOS_U16    HorSyncStart; 
    CBIOS_U8     HSSRemainder;
    CBIOS_U16    HorSyncEnd;     
    CBIOS_U8     HBackPorchRemainder;
    CBIOS_U16    VerTotal;      
    CBIOS_U16    VerDisEnd;    
    CBIOS_U16    VerBStart;  
    CBIOS_U16    VerBEnd;      
    CBIOS_U16    VerSyncStart;  
    CBIOS_U16    VerSyncEnd;   
}CBIOS_TIMING_REG_Arise, *PCBIOS_TIMING_REG_Arise;

CBIOS_VOID cbInitChipAttribute_Arise(PCBIOS_EXTENSION_COMMON pcbe);

CBIOS_VOID cbSetCRTimingReg_Arise(PCBIOS_EXTENSION_COMMON pcbe,
                                PCBIOS_TIMING_ATTRIB pTiming,
                                CBIOS_U32 IGAIndex,
                                CBIOS_TIMING_FLAGS Flags);
CBIOS_VOID cbSetSRTimingReg_Arise(PCBIOS_EXTENSION_COMMON pcbe,
                                PCBIOS_TIMING_ATTRIB pTiming,
                                CBIOS_U32 IGAIndex,
                                CBIOS_TIMING_FLAGS Flags);
CBIOS_VOID cbGetCRTimingReg_Arise(PCBIOS_EXTENSION_COMMON pcbe,
                                PCBIOS_TIMING_ATTRIB pTiming,
                                CBIOS_U32 IGAIndex,
                                PCBIOS_TIMING_FLAGS pFlags);
CBIOS_VOID cbGetSRTimingReg_Arise(PCBIOS_EXTENSION_COMMON pcbe,
                                PCBIOS_TIMING_ATTRIB pTiming,
                                CBIOS_U32 IGAIndex,
                                PCBIOS_TIMING_FLAGS pFlags);
CBIOS_VOID cbGetModeInfoFromReg_Arise(PCBIOS_EXTENSION_COMMON pcbe,
                                CBIOS_ACTIVE_TYPE ulDevice,
                                PCBIOS_TIMING_ATTRIB pTiming,
                                PCBIOS_TIMING_FLAGS pFlags,
                                CBIOS_U32  IGAIndex,
                                CBIOS_TIMING_REG_TYPE TimingRegType);
CBIOS_BOOL cbInitVCP_Arise(PCBIOS_EXTENSION_COMMON pcbe, PVCP_INFO pVCP, PCBIOS_VOID pRomBase);
CBIOS_STATUS cbInterruptEnableDisable_Arise(PCBIOS_EXTENSION_COMMON pcbe,PCBIOS_INT_ENABLE_DISABLE_PARA pIntPara);
CBIOS_STATUS cbCECEnableDisable_Arise(PCBIOS_VOID pvcbe, PCBIOS_CEC_ENABLE_DISABLE_PARA pCECEnableDisablePara);
CBIOS_STATUS cbCheckSurfaceOnDisplay_Arise(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_CHECK_SURFACE_ON_DISP pChkSurfacePara);
CBIOS_STATUS  cbGetDispAddr_Arise(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_GET_DISP_ADDR  pGetDispAddr);
CBIOS_STATUS cbUpdateFrame_Arise(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_UPDATE_FRAME_PARA   pUpdateFrame);
CBIOS_STATUS cbDoCSCAdjust_Arise(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_ACTIVE_TYPE Device, PCBIOS_CSC_ADJUST_PARA pCSCAdjustPara);
CBIOS_STATUS cbAdjustStreamCSC_Arise(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_STREAM_CSC_PARA pStreamCSCPara);
CBIOS_STATUS cbSetGamma_Arise(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_GAMMA_PARA pGammaParam);
CBIOS_VOID cbDisableGamma_Arise(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 IGAIndex);
CBIOS_VOID cbDoGammaConfig_Arise(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_GAMMA_PARA pGammaParam);
CBIOS_VOID cbGetLUT_Arise(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_GAMMA_PARA pGammaParam);
CBIOS_VOID cbSetLUT_Arise(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_GAMMA_PARA pGammaParam);
CBIOS_STATUS cbSetHwCursor_Arise(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_CURSOR_PARA pSetCursor);
CBIOS_VOID cbDisableStream_Arise(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 IGAIndex);
#endif//_CBIOS_E3K_H_
