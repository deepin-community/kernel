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
** HDCP hw block interface function prototype.
**
** NOTE:
** 
******************************************************************************/

#ifndef _CBIOS_DIU_HDCP_H_
#define _CBIOS_DIU_HDCP_H_

#include "../../Device/CBiosDeviceShare.h"

#define CBIOS_HW_BKSV_LIST_FIFO_SIZE        4

#define CBIOS_HDCP_REAUTH_TRY_COUNT_MAX     3

#define HDCP_MODU_NUM  4

typedef enum
{
    CBIOS_HDCP_VERSION_1x,
    CBIOS_HDCP_VERSION_2x,
}CBIOS_HDCP_VERSION;

typedef enum
{
    CBIOS_HW_I2C_Freq_50kHz = 1,
    CBIOS_HW_I2C_Freq_80kHz,
    CBIOS_HW_I2C_Freq_100kHz,
    CBIOS_HW_I2C_Freq_200kHz,
    CBIOS_HW_I2C_Freq_300kHz,
    CBIOS_HW_I2C_Freq_400kHz,
    CBIOS_HW_I2C_Freq_1MHz,
}CBIOS_HW_I2C_Freq;

typedef enum
{
    CBIOS_HDMI_HDCP1x_INT_AUTH_FAIL          = 0,
    CBIOS_HDMI_HDCP1x_INT_Ri_VERIF_FAIL,
    CBIOS_HDMI_HDCP1x_INT_Pj_VERIF_FAIL,
    CBIOS_HDMI_HDCP1x_INT_KSV_READY,
    CBIOS_HDMI_HDCP1x_INT_BKSV_INVALID,
    CBIOS_HDMI_HDCP1x_INT_V_LIST_CHECK_FAIL,
    CBIOS_HDMI_HDCP1x_INT_Ri_VERIF_TIMEOUT,
    CBIOS_HDMI_HDCP1x_INT_MAX_CASCADE,
    CBIOS_HDMI_HDCP1x_INT_MAX_DEVICES,
    CBIOS_HDMI_HDCP1x_INT_ZERO_DEVICE,
    CBIOS_HDMI_HDCP1x_INT_RESERVED1,
    CBIOS_HDMI_HDCP1x_INT_RESERVED2,
    CBIOS_HDMI_HDCP1x_INT_RESERVED3,
    CBIOS_HDMI_HDCP1x_INT_RESERVED4,
    CBIOS_HDMI_HDCP1x_INT_RESERVED5,
    CBIOS_HDMI_HDCP1x_INT_AUTH_PASS,
}CBIOS_HDMI_HDCP1x_INT_SRC;

typedef enum
{
    CBIOS_DP_HDCP1x_INT_AUTH_FAIL,
    CBIOS_DP_HDCP1x_INT_NOT_HDCP_CAPABLE,
    CBIOS_DP_HDCP1x_INT_BKSV_INVALID,
    CBIOS_DP_HDCP1x_INT_RO_INVALID,
    CBIOS_DP_HDCP1x_INT_KSV_VERIF_TIMEOUT,
    CBIOS_DP_HDCP1x_INT_MAX_CASCADE,
    CBIOS_DP_HDCP1x_INT_MAX_DEVS,
    CBIOS_DP_HDCP1x_INT_ZERO_DEVS,
    CBIOS_DP_HDCP1x_INT_KSV_READY,
    CBIOS_DP_HDCP1x_INT_V_LIST_CHECK_FAIL,
    CBIOS_DP_HDCP1x_INT_AUX_FAIL,
    CBIOS_DP_HDCP1x_INT_RESERVED1,
    CBIOS_DP_HDCP1x_INT_RESERVED2,
    CBIOS_DP_HDCP1x_INT_RESERVED3,
    CBIOS_DP_HDCP1x_INT_RESERVED4,
    CBIOS_DP_HDCP1x_INT_AUTH_PASS,
}CBIOS_DP_HDCP1x_INT_SRC;

typedef enum
{
    CBIOS_HDCP2x_INT_MASK_MSG_ID                = 0x00000001,
    CBIOS_HDCP2x_INT_MASK_GET_RECID             = 0x00000002,
    CBIOS_HDCP2x_INT_MASK_KSVFF                 = 0x00000004,
    CBIOS_HDCP2x_INT_MASK_NOT_HDCP22            = 0x00000008,
    CBIOS_HDCP2x_INT_MASK_IS_HDCP22             = 0x00000010,
    CBIOS_HDCP2x_INT_MASK_CSM_FAIL              = 0x00000040,
    CBIOS_HDCP2x_INT_MASK_AUTH_FAIL             = 0x00000080,
    CBIOS_HDCP2x_INT_MASK_I2C_FAIL              = 0x00000100,
    CBIOS_HDCP2x_INT_MASK_DEV_ZERO              = 0x00000200,
    CBIOS_HDCP2x_INT_MASK_CSM_PASS              = 0x00000400,
    CBIOS_HDCP2x_INT_MASK_AUTH_PASS             = 0x00000800,
    CBIOS_HDCP2x_INT_MASK_SEQ_NUM_M_ROLLOVER    = 0x00001000,
    CBIOS_HDCP2x_INT_MASK_M_RETRY_NUMOUT        = 0x00002000,
    CBIOS_HDCP2x_INT_MASK_M_TIMEOUT             = 0x00004000,
    CBIOS_HDCP2x_INT_MASK_M_FAIL                = 0x00008000,
    CBIOS_HDCP2x_INT_MASK_NONZERO_SEQ_NUM_V     = 0x00010000,
    CBIOS_HDCP2x_INT_MASK_SEQ_NUM_V_ROLLOVER    = 0x00020000,
    CBIOS_HDCP2x_INT_MASK_V_FAIL                = 0x00040000,
    CBIOS_HDCP2x_INT_MASK_MAX_CAS               = 0x00080000,
    CBIOS_HDCP2x_INT_MASK_MAX_DEVS              = 0x00100000,
    CBIOS_HDCP2x_INT_MASK_WAIT_RECID_TIMEOUT    = 0x00200000,
    CBIOS_HDCP2x_INT_MASK_REAUTH_REQ            = 0x00400000,
    CBIOS_HDCP2x_INT_MASK_L_FAIL                = 0x00800000,
    CBIOS_HDCP2x_INT_MASK_LC_RETRY_NUMOUT       = 0x01000000,
    CBIOS_HDCP2x_INT_MASK_LC_TIMEOUT            = 0x02000000,
    CBIOS_HDCP2x_INT_MASK_EKH_TIMEOUT           = 0x04000000,
    CBIOS_HDCP2x_INT_MASK_H_FAIL                = 0x08000000,
    CBIOS_HDCP2x_INT_MASK_H_TIMEOUT             = 0x10000000,
    CBIOS_HDCP2x_INT_MASK_CERT_FAIL             = 0x20000000,
    CBIOS_HDCP2x_INT_MASK_RECID_INVALID         = 0x40000000,
    CBIOS_HDCP2x_INT_MASK_CERT_TIMEOUT          = 0x80000000,
}CBIOS_HDCP2x_INT_MASK;

typedef struct
{
    CBIOS_HDCP_VERSION HdcpVersion;
    CBIOS_HDCP_AUTHENTICATION_STATUS AuthStatus;
    CBIOS_U8 BKsv[5];
    CBIOS_BOOL bRepeater;
    PCBIOS_U8 pBKsvList;
    CBIOS_U32 BKsvListBufferSize;
    CBIOS_U8 DeviceCount;
    PCBIOS_U8 pRevocationList;
    CBIOS_U32 RevocationListBufferSize;
    CBIOS_BOOL bRepeaterAuthPass;
    CBIOS_BOOL HdcpReAuthTryCount;
}CBIOS_HDCP_CONTEXT, *PCBIOS_HDCP_CONTEXT;

CBIOS_VOID cbHDCP_Init(PCBIOS_VOID pvcbe, PCBIOS_VOID *ppHdcpContext);
CBIOS_VOID cbHDCP_DeInit(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device);
CBIOS_VOID cbHDCP_OnOff(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device, CBIOS_U8 IGAIndex, CBIOS_BOOL bTurnOn);
CBIOS_VOID cbHDCP_Isr(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device);
CBIOS_HDCP_AUTHENTICATION_STATUS cbHDCP_GetStatus(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device);
CBIOS_BOOL cbHDCP_GetHDCPBKsv(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device, PCBIOS_U8 pBKsv, PCBIOS_BOOL bRepeater);
CBIOS_VOID cbHDCP_WorkThread(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device, CBIOS_U8 IGAIndex);
#endif
