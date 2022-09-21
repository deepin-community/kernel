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
** CBios chip dependent function prototype.
**
** NOTE:
** This header file CAN be included by sw layer. 
******************************************************************************/

#ifndef _CBIOS_CHIP_FUNC_H_
#define _CBIOS_CHIP_FUNC_H_

#include "../CBios.h"
#include "../Device/CBiosChipShare.h"
#include "../Display/CBiosDisplayManager.h"

typedef CBIOS_VOID 
(*PFN_cbSetTimingReg)(PCBIOS_VOID pvcbe,
                      PCBIOS_TIMING_ATTRIB pTiming,
                      CBIOS_U32 IGAIndex,
                      CBIOS_TIMING_FLAGS Flags);

typedef CBIOS_VOID 
(*PFN_cbGetModeInfoFromReg)(PCBIOS_VOID pvcbe,
                             CBIOS_ACTIVE_TYPE ulDevice,
                             PCBIOS_TIMING_ATTRIB pTiming,
                             PCBIOS_TIMING_FLAGS pFlags,
                             CBIOS_U32  IGAIndex,
                             CBIOS_TIMING_REG_TYPE TimingRegType);

typedef CBIOS_BOOL
(*PFN_cbInitVCP) (PCBIOS_VOID pvcbe, PCBIOS_VOID pVCP, PCBIOS_VOID pRomBase);

typedef CBIOS_VOID 
(*PFN_cbDoHDTVFuncSetting) (PCBIOS_VOID pvcbe, 
                            PCBIOS_DISP_MODE_PARAMS pModeParams,
                            CBIOS_U32 IGAIndex, 
                            CBIOS_ACTIVE_TYPE ulDevices);

typedef CBIOS_VOID
(*PFN_cbLoadSSC) (PCBIOS_VOID pvcbe, CBIOS_U32 CenterFreq, CBIOS_U8 IGAIndex, CBIOS_ACTIVE_TYPE LCDType);

typedef CBIOS_VOID
(*PFN_cbEnableSpreadSpectrum) (PCBIOS_VOID pvcbe, CBIOS_U8 IGAIndex);

typedef CBIOS_VOID 
(*PFN_cbDisableSpreadSpectrum) (PCBIOS_VOID pvcbe, CBIOS_U8 IGAIndex);

typedef  CBIOS_STATUS
(*PFN_cbUpdateFrame)(PCBIOS_VOID pvcbe, PCBIOS_UPDATE_FRAME_PARA pUpdateFrame);

typedef  CBIOS_STATUS
(*PFN_cbDoCSCAdjust)(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device, PCBIOS_CSC_ADJUST_PARA pCSCAdjustPara);

typedef  CBIOS_STATUS
(*PFN_cbAdjustStreamCSC)(PCBIOS_VOID pvcbe, PCBIOS_STREAM_CSC_PARA pCSCAdjustPara);

typedef  CBIOS_STATUS
(*PFN_cbCheckSurfaceOnDisplay)(PCBIOS_VOID pvcbe, PCBIOS_CHECK_SURFACE_ON_DISP pChkSurfacePara);

typedef  CBIOS_STATUS
(*PFN_cbGetDispAddr)(PCBIOS_VOID pvcbe, PCBIOS_GET_DISP_ADDR  pGetDispAddr);

typedef  CBIOS_STATUS
(*PFN_cbSetHwCursor)(PCBIOS_VOID pvcbe, PCBIOS_CURSOR_PARA pSetCursor);

typedef CBIOS_STATUS
(*PFN_cbInterruptEnableDisable)(PCBIOS_VOID pvcbe, PCBIOS_INT_ENABLE_DISABLE_PARA pIntPara);

typedef CBIOS_STATUS
(*PFN_cbCECEnableDisable)(PCBIOS_VOID pvcbe, PCBIOS_CEC_ENABLE_DISABLE_PARA pCECEnableDisablePara);

typedef CBIOS_STATUS
(*PFN_cbSetGamma)(CBIOS_VOID* pvcbe, PCBIOS_GAMMA_PARA pGammaParam);


typedef  CBIOS_VOID 
(*PFN_cbDisableStream)(PCBIOS_VOID pvcbe, CBIOS_U32  IGAIndex);

typedef struct _CBIOS_CHIP_FUNC_TABLE
{
    PFN_cbSetTimingReg                              pfncbSetCRTimingReg;
    PFN_cbSetTimingReg                              pfncbSetSRTimingReg;
    PFN_cbGetModeInfoFromReg                        pfncbGetModeInfoFromReg;
    PFN_cbInitVCP                                   pfncbInitVCP;
    PFN_cbDoHDTVFuncSetting                         pfncbDoHDTVFuncSetting;
    PFN_cbLoadSSC                                   pfncbLoadSSC;
    PFN_cbEnableSpreadSpectrum                      pfncbEnableSpreadSpectrum;
    PFN_cbDisableSpreadSpectrum                     pfncbDisableSpreadSpectrum;
    PFN_cbUpdateFrame                               pfncbUpdateFrame;
    PFN_cbDoCSCAdjust                               pfncbDoCSCAdjust;
    PFN_cbAdjustStreamCSC                           pfncbAdjustStreamCSC;
    PFN_cbCheckSurfaceOnDisplay                     pfncbCheckSurfaceOnDisplay;
    PFN_cbGetDispAddr                               pfncbGetDispAddr;
    PFN_cbSetHwCursor                               pfncbSetHwCursor;
    PFN_cbInterruptEnableDisable                    pfncbInterruptEnableDisable;
    PFN_cbCECEnableDisable                          pfncbCECEnableDisable;
    PFN_cbSetGamma                                  pfncbSetGamma;
    PFN_cbDisableStream                             pfncbDisableStream;
}CBIOS_CHIP_FUNC_TABLE, *PCBIOS_CHIP_FUNC_TABLE;


CBIOS_VOID   cbSetCRTimingReg(PCBIOS_VOID pvcbe, PCBIOS_TIMING_ATTRIB pTiming, CBIOS_U32 IGAIndex, CBIOS_TIMING_FLAGS Flags);
CBIOS_VOID   cbSetSRTimingReg(PCBIOS_VOID pvcbe, PCBIOS_TIMING_ATTRIB pTiming, CBIOS_U32 IGAIndex, CBIOS_TIMING_FLAGS Flags);
CBIOS_VOID   cbGetModeInfoFromReg(PCBIOS_VOID pvcbe,
                                          CBIOS_ACTIVE_TYPE ulDevice,
                                          PCBIOS_TIMING_ATTRIB pTiming,
                                          PCBIOS_TIMING_FLAGS pFlags,
                                          CBIOS_U32  IGAIndex,
                                          CBIOS_TIMING_REG_TYPE TimingRegType);
CBIOS_BOOL   cbInitVCP(PCBIOS_VOID pvcbe, PCBIOS_VOID pVCPInfo, PCBIOS_VOID pRomBase);
CBIOS_VOID   cbDoHDTVFuncSetting(PCBIOS_VOID pvcbe, PCBIOS_DISP_MODE_PARAMS pModeParams, CBIOS_U32 IGAIndex, CBIOS_ACTIVE_TYPE ulDevices);
CBIOS_VOID   cbLoadSSC(PCBIOS_VOID pvcbe, CBIOS_U32 CenterFreq, CBIOS_U8 IGAIndex, CBIOS_ACTIVE_TYPE LCDType);
CBIOS_VOID   cbEnableSpreadSpectrum(PCBIOS_VOID pvcbe, CBIOS_U8 IGAIndex);
CBIOS_VOID   cbDisableSpreadSpectrum(PCBIOS_VOID pvcbe, CBIOS_U8 IGAIndex);
CBIOS_STATUS cbInterruptEnableDisable(PCBIOS_VOID pvcbe, PCBIOS_INT_ENABLE_DISABLE_PARA pIntPara);
CBIOS_STATUS cbCECEnableDisable(PCBIOS_VOID pvcbe, PCBIOS_CEC_ENABLE_DISABLE_PARA pCECEnableDisablePara);
CBIOS_STATUS cbSetGamma(PCBIOS_VOID pvcbe, PCBIOS_GAMMA_PARA pGammaParam);
CBIOS_STATUS cbUpdateFrame(PCBIOS_VOID pvcbe, PCBIOS_UPDATE_FRAME_PARA pUpdateFrame);
CBIOS_STATUS cbDoCSCAdjust(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device, PCBIOS_CSC_ADJUST_PARA pCSCAdjustPara);
CBIOS_STATUS cbAdjustStreamCSC(PCBIOS_VOID pvcbe, PCBIOS_STREAM_CSC_PARA pCSCAdjustPara);
CBIOS_STATUS cbCheckSurfaceOnDisplay(PCBIOS_VOID pvcbe, PCBIOS_CHECK_SURFACE_ON_DISP pChkSurfacePara);
CBIOS_STATUS cbGetDispAddr(PCBIOS_VOID  pvcbe, PCBIOS_GET_DISP_ADDR pGetDispAddr);
CBIOS_STATUS cbSetHwCursor(PCBIOS_VOID pvcbe, PCBIOS_CURSOR_PARA pSetCursor);
CBIOS_VOID   cbDisableStream(PCBIOS_VOID pvcbe, CBIOS_U32 IGAIndex);
#endif//_CBIOS_CHIP_FUNC_H_
