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
** Audio codec hw block interface function prototype and parameter definition.
**
** NOTE:
** 
******************************************************************************/

#ifndef _CBIOS_DIU_HDAC_H_
#define _CBIOS_DIU_HDAC_H_

#include "../../Device/CBiosDeviceShare.h"

#define  HDAC_MODU_NUM  2

extern CBIOS_U32 HDAC_REG_PACKET1[HDAC_MODU_NUM];
extern CBIOS_U32 HDAC_REG_PACKET2[HDAC_MODU_NUM];
extern CBIOS_U32 HDAC_REG_MODE_RESP[HDAC_MODU_NUM];
extern CBIOS_U32 HDAC_REG_SW_RESP[HDAC_MODU_NUM];
extern CBIOS_U32 HDAC_REG_CHSTATUS_CTRL[HDAC_MODU_NUM];
extern CBIOS_U32 HDAC_REG_SUP_PARA[HDAC_MODU_NUM];
extern CBIOS_U32 HDAC_REG_SAMP_RATE[HDAC_MODU_NUM];
extern CBIOS_U32 HDAC_REG_CONVERT_CAP[HDAC_MODU_NUM];
extern CBIOS_U32 HDAC_REG_PIN_WIDGET_CAP[HDAC_MODU_NUM];
extern CBIOS_U32 HDAC_REG_PIN_SENSE[HDAC_MODU_NUM];
extern CBIOS_U32 HDAC_REG_ELD_BUF[HDAC_MODU_NUM];
extern CBIOS_U32 HDAC_REG_CTRL_WRITE[HDAC_MODU_NUM];
extern CBIOS_U32 HDAC_REG_READ_SEL[HDAC_MODU_NUM];
extern CBIOS_U32 HDAC_REG_READ_OUT[HDAC_MODU_NUM];

typedef struct _AUDIO_CLOCK_TABLE
{
    CBIOS_U32 StreamFormat;
    CBIOS_U64 AudioPacketClock;
}AUDIO_CLOCK_TABLE,*PAUDIO_CLOCK_TABLE;

typedef union _HDAC_NONSNOOP_SETTING
{
    CBIOS_U16      NonSnoopFlags;
    struct
    {
        CBIOS_U16  bStreamNonSnoop      :1;
        CBIOS_U16  bBDLNonSnoop         :1;
        CBIOS_U16  bCORBNonSnoop        :1;
        CBIOS_U16  bRIRBNonSnoop        :1;
        CBIOS_U16  bDMAPositionNonSnoop :1;
        CBIOS_U16  Reserved             :11;
    };
}HDAC_NONSNOOP_SETTING,*PHDAC_NONSNOOP_SETTING;

CBIOS_VOID cbDIU_HDAC_SetHDACodecPara(PCBIOS_VOID pvcbe, PCBIOS_HDAC_PARA pCbiosHDACPara);
CBIOS_VOID cbDIU_HDAC_SetStatus(PCBIOS_VOID pvcbe);
CBIOS_U32  cbDIU_HDAC_GetChannelNums(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX HDACModuleIndex);
CBIOS_VOID cbDIU_HDAC_SetConnectStatus(PCBIOS_VOID pvcbe, PCBIOS_HDAC_PARA pCbiosHDACPara);
#endif
