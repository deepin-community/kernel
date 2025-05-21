/*
 * Copyright © 2021 Glenfly Tech Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#ifndef _CBIOS_DIU_DP_H_
#define _CBIOS_DIU_DP_H_

#include "CBiosChipShare.h"
#include "CBiosDeviceShare.h"
#include "../HwUtil/CBiosI2C.h"

#define CBIOS_DP_LINK_SPEED_1620Mbps        1620000
#define CBIOS_DP_LINK_SPEED_2700Mbps        2700000
#define CBIOS_DP_LINK_SPEED_5400Mbps        5400000

#define  DP_MODU_NUM  4

extern CBIOS_U32 DP_REG_MISC1[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_LINK[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_GEN_CTRL[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_EXT_PACKET[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_ENABLE[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_HWIDTH_TU[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_HLINE_DUR[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_MISC0[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_HV_TOTAL[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_HV_START[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_HV_SYNC[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_HV_ACTIVE[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_EPHY_CTRL[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_AUX_WRITE0[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_AUX_WRITE1[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_AUX_WRITE2[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_AUX_WRITE3[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_AUX_READ0[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_AUX_READ1[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_AUX_READ2[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_AUX_READ3[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_AUX_TIMER[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_AUX_CMD[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_MUTE[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_MAUD[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_EPHY_MPLL[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_EPHY_TX[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_EPHY_MISC[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_SWING[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_EPHY_STATUS[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_LINK_CTRL[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_CTRL2[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_CTRL[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_EPHY_SETTING1[DP_MODU_NUM];
extern CBIOS_U32 DP_REG_EPHY_SETTING2[DP_MODU_NUM];

typedef enum _CBIOS_DPCD_LINK_RATE
{
    CBIOS_DPCD_LINK_RATE_1620Mbps    = 0x6,   // 1.62Gbps per lane
    CBIOS_DPCD_LINK_RATE_2700Mbps    = 0xA,   // 2.7 Gbps per lane
    CBIOS_DPCD_LINK_RATE_5400Mbps    = 0x14,  // 5.4 Gbps per lane
}CBIOS_DPCD_LINK_RATE;

typedef enum _CBIOS_AUX_REQUEST_CMD
{
    CBIOS_AUX_REQUEST_I2C_WRITE    = 0,
    CBIOS_AUX_REQUEST_I2C_READ     = 1,
    CBIOS_AUX_REQUEST_I2C_MOT      = 4,
    CBIOS_AUX_REQUEST_NATIVE_WRITE = 8,
    CBIOS_AUX_REQUEST_NATIVE_READ  = 9,
}CBIOS_AUX_REQUEST_CMD;

typedef enum _CBIOS_AUX_REPLY_CMD
{
    CBIOS_AUX_REPLY_ACK      = 0,
    CBIOS_AUX_REPLY_NACK     = 1,
    CBIOS_AUX_REPLY_DEFER    = 2, // Not ready for the write/read request. A uPacket TX may retry later.
    CBIOS_AUX_REPLY_RESERVED = 3,
}CBIOS_AUX_REPLY_CMD;

typedef struct _AUX_CONTROL
{
    CBIOS_U8    I2cPort;    //
    CBIOS_U32   Length;
    CBIOS_U32   Offset;   // register index
    CBIOS_U32   Data[4];  // register data
    CBIOS_U8    Function;      // Special I2c control flag
}AUX_CONTROL, *PAUX_CONTROL;

typedef enum _CBIOS_EDP_CP_METHOD
{
    CBIOS_EDP_CP_DISABLE = 0,
    CBIOS_EDP_CP_ASSR,
    CBIOS_EDP_CP_AF
}CBIOS_EDP_CP_METHOD;

typedef struct _CBIOS_LINK_TRAINING_PARAMS
{
    // input params
    CBIOS_U32   DpSinkVersion;
    CBIOS_U32   MaxLaneCount;
    CBIOS_U32   MaxLinkSpeed;
    CBIOS_BOOL  bEnhancedMode;
    CBIOS_BOOL  bEDP;
    CBIOS_BOOL  bEnableTPS3;
    CBIOS_EDP_CP_METHOD CPMethod;
    CBIOS_U32   TrainingAuxRdInterval;

    // output params
    CBIOS_U32   CurrLaneCount;
    CBIOS_U32   CurrLinkSpeed;

}CBIOS_LINK_TRAINING_PARAMS, *PCBIOS_LINK_TRAINING_PARAMS;

typedef struct _CBIOS_MAIN_LINK_PARAMS
{
    CBIOS_U32  LinkedLaneNumber;  // 1 ~ 4 lanes
    CBIOS_U32  LinkedSpeed;   // 1.62Gbps, 2.7Gbps or 5.4Gbps
    CBIOS_U32  bpc;              // bit per channel
    CBIOS_U32  TUSize;           // 32 ~ 64, default to 48
    CBIOS_BOOL AsyncMode;        // 1 (yes), asynchronous mode
    CBIOS_U32  ColorFormat;      // 0: RGB, 1:YCbCr422, 2:YCbCr444
    CBIOS_U32  DynamicRange;     // 0: VESA range, 1:CEA range
    CBIOS_U32  YCbCrCoefficients;// 0: ITU601, 1: ITU709
    PCBIOS_TIMING_ATTRIB pTiming;
}CBIOS_MAIN_LINK_PARAMS, *PCBIOS_MAIN_LINK_PARAMS;

CBIOS_VOID cbDIU_EDP_ControlVDDSignal(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_BOOL status);
CBIOS_VOID cbDIU_EDP_ControlVEESignal(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_BOOL status);
CBIOS_BOOL cbDIU_EDP_WaitforSinkHPDSignal(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_U16 timeout);

CBIOS_VOID cbDIU_DP_DPModeEnable(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_BOOL bEnable);
CBIOS_VOID cbDIU_DP_SetHVSync(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_U8 HVPolarity);
CBIOS_VOID cbDIU_DP_SetMaudNaud(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_U32 LinkSpeed, CBIOS_U32 StreamFormat);
CBIOS_VOID cbDIU_DP_ResetLinkTraining(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex);
CBIOS_VOID cbDIU_DP_VideoOnOff(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_BOOL bOn);
CBIOS_VOID cbDIU_DP_DisableVideoAudio(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex);
CBIOS_VOID cbDIU_DP_EnableVideoAudio(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex);
CBIOS_VOID cbDIU_DP_SetInterruptMode(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_BOOL isDPMode);

CBIOS_BOOL cbDIU_DP_LinkTrainingHw(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, PCBIOS_LINK_TRAINING_PARAMS pLinkTrainingParams);
CBIOS_BOOL cbDIU_DP_SetUpMainLink(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, PCBIOS_MAIN_LINK_PARAMS pMainLinkParams);

CBIOS_VOID cbDIU_DP_SendInfoFrame(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_U32 Length);

CBIOS_VOID cbDIU_DP_ResetAUX(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex);
CBIOS_BOOL cbDIU_DP_AuxChRW(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, PAUX_CONTROL pAUX);
CBIOS_VOID cbDIU_DP_HWUseAuxChannel(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex);
CBIOS_U32  cbDIU_DP_AuxReadEDID(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, PCBIOS_UCHAR pEDIDBuffer, CBIOS_U32 ulBufferSize);
CBIOS_BOOL cbDIU_DP_AuxReadEDIDOffset(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, PCBIOS_UCHAR pEDIDBuffer, CBIOS_U32 ulBufferSize, CBIOS_U32 ulReadEdidOffset);
CBIOS_STATUS cbDIU_DP_ReadData(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams, CBIOS_U32 Flags, CBIOS_BOOL IsDDC);
CBIOS_BOOL cbDIU_DP_WriteData(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams, CBIOS_BOOL IsDDC);
CBIOS_BOOL cbDIU_DP_IsOn(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex);

#endif
