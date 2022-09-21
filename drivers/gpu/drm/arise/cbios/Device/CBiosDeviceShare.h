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
** Defines common function pointer for each device port.
** Defines CBIOS_DEVICE_COMMON.
**
** NOTE:
** 
******************************************************************************/

#ifndef _CBIOS_DEVICE_SHARE_H_
#define _CBIOS_DEVICE_SHARE_H_

#include "CBiosShare.h"
#include "../Display/CBiosMode.h"
#include "../Util/CBiosEDID.h"
#include "Port/CBiosDSI.h"
#include "../Display/CBiosPathManager.h"
#include "Monitor/CBiosEDPPanel.h"

#define CBIOS_MAX_DISPLAY_DEVICES_NUM                  32

typedef CBIOS_VOID 
(*PFN_cbDeviceHwInit)(PCBIOS_VOID pvcbe, PCBIOS_VOID pDevCommon);

typedef CBIOS_BOOL
(*PFN_cbDeviceDetect)(PCBIOS_VOID pvcbe, PCBIOS_VOID pDevCommon, CBIOS_BOOL bHardcodeDetected, CBIOS_U32 FullDetect);

typedef CBIOS_VOID 
(*PFN_cbDeviceOnOff)(PCBIOS_VOID pvcbe, PCBIOS_VOID pDevCommon, CBIOS_BOOL bOn);

typedef CBIOS_VOID 
(*PFN_cbQueryMonitorAttribute)(PCBIOS_VOID pvcbe, PCBIOS_VOID pDevCommon, PCBiosMonitorAttribute pMonitorAttribute);

typedef CBIOS_VOID 
(*PFN_cbUpdateDeviceModeInfo)(PCBIOS_VOID pvcbe, PCBIOS_VOID pDevCommon, PCBIOS_VOID pModeParams);

typedef CBIOS_VOID 
(*PFN_cbDeviceSetMode)(PCBIOS_VOID pvcbe, PCBIOS_VOID pDevCommon, PCBIOS_VOID pModeParams);

typedef struct _CBIOS_DEVICE_SIGNATURE
{
    CBIOS_U8 MonitorID[MONITORIDLENGTH];
    CBIOS_U8 ExtFlagChecksum[CBIOS_EDIDMAXBLOCKCOUNT][EXTFLAGCHECKSUMLENTH];
}CBIOS_DEVICE_SIGNATURE, *PCBIOS_DEVICE_SIGNATURE;

typedef struct _CBIOS_DEVICE_COMMON
{
    //Common attribute with fixed value
    struct
    {
        CBIOS_ACTIVE_TYPE           DeviceType;
        CBIOS_MONITOR_TYPE          SupportMonitorType;
        CBIOS_U32                   I2CBus; // real I2C bus used to read EDID from monitor
        CBIOS_U8                    HPDPin;
        PCBIOS_VOID                 pHDCPContext;                  
    };

    //Common attribute with variable value
    struct
    {
        CBIOS_MONITOR_TYPE          CurrentMonitorType;
        CBIOS_U8                    PowerState;
        CBIOS_UCHAR                 EdidData[CBIOS_EDIDDATABYTE];
        CBIOS_EDID_STRUCTURE_DATA   EdidStruct;
        CBIOS_DEVICE_SIGNATURE      ConnectedDevSignature; // stores the signature of real device.
        CBIOS_DISPLAY_SOURCE        DispSource;
        CBIOS_BOOL                  isFakeEdid;
        CBIOS_MAX_RES_CONFIG        MaxResConfig;
    };

    union
    {
        CBIOS_DSI_PARAMS            DSIDevice;
        CBIOS_EDPPanel_PARAMS       EDPPanelDevice;
    }DeviceParas;

    //common funcs for each device port
    struct
    {
        PFN_cbDeviceHwInit          pfncbDeviceHwInit;
        PFN_cbDeviceDetect          pfncbDeviceDetect;
        PFN_cbDeviceOnOff           pfncbDeviceOnOff;
        PFN_cbDeviceSetMode         pfncbDeviceSetMode;

        PFN_cbQueryMonitorAttribute pfncbQueryMonitorAttribute;
        PFN_cbUpdateDeviceModeInfo  pfncbUpdateDeviceModeInfo;
    };
}CBIOS_DEVICE_COMMON, *PCBIOS_DEVICE_COMMON;

typedef struct _CBIOS_DEVICE_MANAGER
{
    CBIOS_ACTIVE_TYPE    SupportDevices;
    CBIOS_U32            ConnectedDevices;
    PCBIOS_DEVICE_COMMON pDeviceArray[CBIOS_MAX_DISPLAY_DEVICES_NUM];
}CBIOS_DEVICE_MANAGER, *PCBIOS_DEVICE_MANAGER;

#define cbGetDeviceCommon(pDevMgr, Device)      ((pDevMgr)->pDeviceArray[cbConvertDeviceBit2Index(Device)])

static inline void cbInitialModuleList(PCBIOS_MODULE_LIST pModuleList)
{
    pModuleList->DPModule.Type = CBIOS_MODULE_TYPE_DP;
    pModuleList->DPModule.Index = CBIOS_MODULE_INDEX_INVALID;
    pModuleList->MHLModule.Type = CBIOS_MODULE_TYPE_MHL;
    pModuleList->MHLModule.Index = CBIOS_MODULE_INDEX_INVALID;
    pModuleList->HDCPModule.Type = CBIOS_MODULE_TYPE_HDCP;
    pModuleList->HDCPModule.Index = CBIOS_MODULE_INDEX_INVALID;
    pModuleList->HDMIModule.Type = CBIOS_MODULE_TYPE_HDMI;
    pModuleList->HDMIModule.Index = CBIOS_MODULE_INDEX_INVALID;
    pModuleList->HDTVModule.Type = CBIOS_MODULE_TYPE_HDTV;
    pModuleList->HDACModule.Index = CBIOS_MODULE_INDEX_INVALID;
    pModuleList->HDACModule.Type = CBIOS_MODULE_TYPE_HDAC;
    pModuleList->HDTVModule.Index = CBIOS_MODULE_INDEX_INVALID;
    pModuleList->IGAModule.Type = CBIOS_MODULE_TYPE_IGA;
    pModuleList->IGAModule.Index = CBIOS_MODULE_INDEX_INVALID;
}

#define IS_SUPPORT_4K_MODE        1

extern CBIOS_U8 FPGAHDMIEdid[256];
#if IS_SUPPORT_4K_MODE
extern CBIOS_U8 Fake4KEdid[256];
#endif

CBIOS_BOOL cbGetDeviceEDID(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_BOOL *pIsDevChanged, CBIOS_U32 FullDetect);
CBIOS_BOOL cbIsDeviceChangedByEdid(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_UCHAR pEDIDData);
CBIOS_BOOL cbUpdateDeviceSignature(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon);
CBIOS_MONITOR_TYPE cbGetSupportMonitorType(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE devices);
CBIOS_VOID cbClearEdidRelatedData(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon);
CBIOS_BOOL cbDualModeDetect(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon);
PCBIOS_VOID cbGetDPMonitorContext(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon);
PCBIOS_VOID cbGetHDMIMonitorContext(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon);
CBIOS_STATUS cbGetDeviceELD(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE DeviceType, PCBIOS_ELD_MEM_STRUCT pELD);
#endif
