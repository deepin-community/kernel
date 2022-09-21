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
** DVO port interface function prototype and parameter definition.
**
** NOTE:
** The daughter card's function and parameter SHOULD be added to corresponding monitor file.
******************************************************************************/

#ifndef _CBIOS_DVO_H_
#define _CBIOS_DVO_H_

#include "../CBiosDeviceShare.h"

#define SUPPORTDVODEVICE    TX_NONE


//Hardcode DVO SlaveAddress
#define VT1636_SLAVE_ADDRESS      0x80
#define CH7301_SLAVE_ADDRESS      0xEC
#define VT1632_SLAVE_ADDRESS      0x10
#define AD9389_SLAVE_ADDRESS      0x72
#define CH7305_SLAVE_ADDRESS      0xEA

typedef struct _TXRegType
{
    CBIOS_U8 index;
    CBIOS_U8 mask;
    CBIOS_U8 data;
}TXRegType, *PTXRegType;

typedef enum TX_TYPE
{
    TX_NONE     = 0,       //   No transmitter and without LCD function
    TX_VT1636   = 0x01,
    TX_CH7301C  = 0x02,
    TX_VT1632   = 0x04,
    TX_AD9389B  = 0x08,
    TX_CH7305   = 0x10,
} TX_TYPE;

typedef struct _DVO_CARD_ID_PARA
{
    TX_TYPE  DVOCardType;
    CBIOS_U8 DVOCardIDOffset;
    CBIOS_U8 DVOCardIDValue;
} DVO_CARD_ID_PARA;

typedef struct _CBIOS_DVO_CONTEXT
{
    CBIOS_DEVICE_COMMON     Common;
    CBIOS_DVO_DEV_CONFIG    DVODevice;
}CBIOS_DVO_CONTEXT, *PCBIOS_DVO_CONTEXT;

CBIOS_VOID cbDVOPort_GetSupportMonitorType(PCBIOS_VOID pvcbe, PCBIOS_MONITOR_TYPE pMonitorType);
PCBIOS_DEVICE_COMMON cbDVOPort_Init(PCBIOS_VOID pvcbe, PVCP_INFO pVCP, CBIOS_ACTIVE_TYPE DeviceType);
CBIOS_VOID cbDVOPort_DeInit(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon);
#endif
