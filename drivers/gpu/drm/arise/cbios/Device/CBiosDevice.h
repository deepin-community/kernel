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
** CBios general device port functions prototype.
**
** NOTE:
** The hw dependent function or structure SHOULD NOT be added to this file.
******************************************************************************/

#ifndef _CBIOS_DEVICE_H_
#define _CBIOS_DEVICE_H_

#include "CBiosDeviceShare.h"

CBIOS_VOID   cbInitDeviceArray(PCBIOS_VOID pvcbe, PVCP_INFO pVCP);
CBIOS_VOID   cbDeInitDeviceArray(PCBIOS_VOID pvcbe);
CBIOS_VOID   cbDevDeviceHwInit(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon);
CBIOS_U32    cbDevGetPrimaryDevice(CBIOS_U32 Device);
CBIOS_VOID   cbDevUpdateDeviceModeInfo(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DISP_MODE_PARAMS pModeParams);
CBIOS_VOID   cbDevSetModeToDevice(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DISP_MODE_PARAMS pModeParams);

CBIOS_STATUS cbDevGetModeTiming(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_GET_MODE_TIMING_PARAM pGetModeTiming);
CBIOS_STATUS cbDevGetModeFromReg(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_GETTING_MODE_PARAMS pModeParams);
CBIOS_STATUS cbDevGetEdidFromBuffer(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_PARAM_GET_EDID pCBParamGetEdid);
CBIOS_STATUS cbDevSetEdid(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_PARAM_SET_EDID pCBParamSetEdid);
CBIOS_STATUS cbDevGetSupportedMonitorType(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_QUERY_MONITOR_TYPE_PER_PORT pCBiosQueryMonitorTypePerPort);
CBIOS_STATUS cbDevQueryMonitorAttribute(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBiosMonitorAttribute pMonitorAttribute);
CBIOS_STATUS cbDevQueryMonitor3DCapability(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_MONITOR_3D_CAPABILITY_PARA p3DCapability);
CBIOS_STATUS cbDevGetDeviceModeList(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBiosModeInfoExt pModeList, CBIOS_U32 *pBufferSize);
CBIOS_STATUS cbDevGetDeviceModeListBufferSize(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_U32 *pBufferSize);
CBIOS_STATUS cbGetDeviceName(PCBIOS_VOID pvcbe,   PCBIOS_GET_DEVICE_NAME  pGetName);
CBIOS_BOOL cbDevDeviceDetect(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_BOOL bHardcodeDetect, CBIOS_U32 FullDetect);
CBIOS_U32    cbDevNonDestructiveDeviceDetectByEdid(PCBIOS_VOID pvcbe, PCBIOS_UCHAR pEDIDData, CBIOS_U32 EdidBufferSize);
CBIOS_STATUS cbDevSetDisplayDevicePowerState(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_PM_STATUS PMState);
CBIOS_STATUS cbDevGetDisplayDevicePowerState(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_PM_STATUS pPMState);

// DP only interfaces
CBIOS_STATUS cbDevAccessDpcdData(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBiosAccessDpcdDataParams pAccessDpcdDataParams);

// HD audio
CBIOS_STATUS cbDevSetHDACodecPara(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_HDAC_PARA pCbiosHDACPara);
CBIOS_STATUS cbDevSetHDACConnectStatus(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_HDAC_PARA pCbiosHDACPara);
CBIOS_U32    cbDevGetHDAFormatList(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_HDMI_AUDIO_INFO pHDAFormatList);

// DSI only interfaces
CBIOS_STATUS cbDevDSISendWriteCmd(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DSI_WRITE_PARA_INTERNAL pDSIWriteParams);
CBIOS_STATUS cbDevDSISendReadCmd(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DSI_READ_PARA_INTERNAL pDSIReadParams);
CBIOS_STATUS cbDevDSIDisplayUpdate(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DSI_UPDATE_PARA pDSIUpdatePara);
CBIOS_STATUS cbDevDSIPanelSetCabc(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_U32 CabcValue);
CBIOS_STATUS cbDevDSIPanelSetBacklight(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_U32 BacklightValue);
CBIOS_STATUS cbDevDSIPanelGetBacklight(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_U32 *pBacklightValue);

CBIOS_STATUS cbDevContentProtectionOnOff(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_U8 IGAIndex, PCBiosContentProtectionOnOffParams pContentProtectionOnOffParams);
CBIOS_STATUS cbDevGetHDCPStatus(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_HDCP_STATUS_PARA pCBiosHdcpStatusParams);
CBIOS_STATUS cbDevHDCPWorkThread(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_U8 IGAIndex, PCBIOS_HDCP_WORK_PARA pCBiosHdcpWorkParams);
CBIOS_STATUS cbDevHDCPIsr(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_HDCP_ISR_PARA pHdcpIsrParam);
CBIOS_STATUS cbDevDPIsr(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_ISR_PARA pDPIsrPara);
CBIOS_BOOL cbDevDPWorkThreadMainFunc(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_WORKTHREAD_PARA pDPWorkThreadPara);
CBIOS_STATUS cbDevDPSetNotifications(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_NOTIFICATIONS pDPNotifications);
CBIOS_BOOL cbDevHdmiSCDCWorkThreadMainFunc(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon);

CBIOS_STATUS  cbDevGetDPIntType(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_INT_PARA pDpIntPara);
CBIOS_STATUS  cbDevDPHandleIrq(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_HANDLE_IRQ_PARA pDPHandleIrqPara);
CBIOS_STATUS cbDevDPGetCustomizedTiming(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_CUSTOMIZED_TIMING pDPCustomizedTiming);
#endif
