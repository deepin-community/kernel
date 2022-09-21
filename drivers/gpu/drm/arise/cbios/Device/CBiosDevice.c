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
** CBios general device port functions. Call different port or monitor interface.
**
** NOTE:
** 
******************************************************************************/

#include "CBiosChipShare.h"
#include "../Hw/HwBlock/CBiosDIU_DP.h"
#include "../Hw/HwBlock/CBiosDIU_HDAC.h"
#include "../Hw/HwBlock/CBiosDIU_HDCP.h"

static CBIOS_U32 DevPriorityTbl[]=
{
    CBIOS_TYPE_DP1,
    CBIOS_TYPE_DP2,
    CBIOS_TYPE_DP3,
    CBIOS_TYPE_DP4,
    CBIOS_TYPE_DVO,
    CBIOS_TYPE_CRT,
};

CBIOS_VOID cbDevDeviceHwInit(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;

    if (pDevCommon->pfncbDeviceHwInit)
    {
        pDevCommon->pfncbDeviceHwInit(pcbe, pDevCommon);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "%s: current device: 0x%x has no hw init func.\n", FUNCTION_NAME, pDevCommon->DeviceType));
    }
}

CBIOS_VOID cbInitDeviceArray(PCBIOS_VOID pvcbe, PVCP_INFO pVCP)
{
    PCBIOS_EXTENSION_COMMON  pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32                TmpDev = pcbe->DeviceMgr.SupportDevices;
    CBIOS_ACTIVE_TYPE        CurDevice = CBIOS_TYPE_NONE;
    PCBIOS_DEVICE_COMMON    *ppDeviceCommon = CBIOS_NULL;

    while (TmpDev)
    {
        CurDevice = GET_LAST_BIT(TmpDev);
        ppDeviceCommon = &(cbGetDeviceCommon(&pcbe->DeviceMgr, CurDevice));

        switch (CurDevice)
        {
        case CBIOS_TYPE_CRT:
            *ppDeviceCommon = cbCRTPort_Init(pcbe, pVCP, CBIOS_TYPE_CRT);
            break;
        case CBIOS_TYPE_DVO:
            *ppDeviceCommon = cbDVOPort_Init(pcbe, pVCP, CBIOS_TYPE_DVO);
            break;
        case CBIOS_TYPE_DP1:
        case CBIOS_TYPE_DP2:
        case CBIOS_TYPE_DP3:
        case CBIOS_TYPE_DP4:
            *ppDeviceCommon = cbDPPort_Init(pcbe, pVCP, CurDevice);
            break;
        default:
            break;
        }

        TmpDev &= (~CurDevice);
    }
}

CBIOS_VOID cbDeInitDeviceArray(PCBIOS_VOID pvcbe)
{
    PCBIOS_EXTENSION_COMMON  pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_ACTIVE_TYPE        CurDevice = CBIOS_TYPE_NONE;
    CBIOS_U32                i = 0;

    for (i = 0; i < CBIOS_MAX_DISPLAY_DEVICES_NUM; i++)
    {
        if (pcbe->DeviceMgr.pDeviceArray[i] != CBIOS_NULL)
        {
            CurDevice = pcbe->DeviceMgr.pDeviceArray[i]->DeviceType;
            switch (CurDevice)
            {
            case CBIOS_TYPE_CRT:
                cbCRTPort_DeInit(pcbe, pcbe->DeviceMgr.pDeviceArray[i]);
                break;
            case CBIOS_TYPE_DVO:
                cbDVOPort_DeInit(pcbe, pcbe->DeviceMgr.pDeviceArray[i]);
                break;
            case CBIOS_TYPE_DP1:
            case CBIOS_TYPE_DP2:
            case CBIOS_TYPE_DP3:
            case CBIOS_TYPE_DP4:
                cbDPPort_DeInit(pcbe, pcbe->DeviceMgr.pDeviceArray[i]);
                break;
            default:
                break;
            }

            pcbe->DeviceMgr.pDeviceArray[i] = CBIOS_NULL;
        }
    }
}

CBIOS_U32 cbDevGetPrimaryDevice(CBIOS_U32 Device)
{
    CBIOS_U8 NumOfBit1;
    CBIOS_U32 i;

    NumOfBit1 = cbGetBitsNum(Device);
    if(NumOfBit1 == 1) //only one device
    {
        return Device;
    }
    else if(NumOfBit1 >= 2) //two devices
    {
        for(i = 0; i < sizeofarray(DevPriorityTbl); i++)
        {
            if(Device & DevPriorityTbl[i])
            {
                return DevPriorityTbl[i];
            }
        }
    }

    return 0;
}

CBIOS_VOID cbDevUpdateDeviceModeInfo(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DISP_MODE_PARAMS pModeParams)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;

    if (pDevCommon->pfncbUpdateDeviceModeInfo)
    {
        pDevCommon->pfncbUpdateDeviceModeInfo(pcbe, pDevCommon, pModeParams);
    }
}

CBIOS_VOID cbDevSetModeToDevice(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DISP_MODE_PARAMS pModeParams)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;

    if (pDevCommon->pfncbDeviceSetMode)
    {
        pDevCommon->pfncbDeviceSetMode(pcbe, pDevCommon, pModeParams);
    }
}

CBIOS_STATUS cbDevGetModeTiming(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_GET_MODE_TIMING_PARAM pGetModeTiming)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBiosDestModeParams      pDestModeParams = CBIOS_NULL;
    PCBIOS_TIMING_ATTRIB      pTiming = pGetModeTiming->pTiming;
    CBIOS_STATUS              Status = CBIOS_ER_INTERNAL;

    if(CBIOS_NULL == pGetModeTiming)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: pGetModeTiming is CBIOS_NULL!\n", FUNCTION_NAME));
        Status =  CBIOS_ER_NULLPOINTER;
        goto END;
    }

    if(CBIOS_NULL == pGetModeTiming->pMode || CBIOS_NULL == pGetModeTiming->pTiming)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: pGetModeTiming->pMode or pGetModeTiming->pTiming is CBIOS_NULL!\n", FUNCTION_NAME));
        Status =  CBIOS_ER_INVALID_PARAMETER;
        goto END;
    }

    pDestModeParams = cb_AllocatePagedPool(sizeof(CBiosDestModeParams));
    if(CBIOS_NULL == pDestModeParams)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: Allocat pDestModeParams fail!\n", FUNCTION_NAME));
        Status =  CBIOS_ER_INTERNAL;
        goto END;
    }

    cb_memset(pTiming, 0, sizeof(CBIOS_TIMING_ATTRIB));

    // 1. cbMode_GetHVTiming only use XRes, YRes, InterlaceFlag and RefreshRate to look for the detail timing.
    // Therefore we init others items to 0
    pDestModeParams->Size = sizeof(CBiosDestModeParams);
    pDestModeParams->XRes = pGetModeTiming->pMode->XRes;
    pDestModeParams->YRes = pGetModeTiming->pMode->YRes;
    pDestModeParams->RefreshRate = pGetModeTiming->pMode->RefreshRate;
    pDestModeParams->InterlaceFlag = (pGetModeTiming->pMode->InterlaceProgressiveCaps & 0x2) ? 1 : 0;

    // 2. Get detail timing
    cbMode_GetHVTiming(pcbe,
                       pDestModeParams->XRes,
                       pDestModeParams->YRes,
                       pDestModeParams->RefreshRate,
                       pDestModeParams->InterlaceFlag,
                       pGetModeTiming->DeviceId, 
                       pTiming);

    Status = CBIOS_OK;
    
END:
    if(pDestModeParams != CBIOS_NULL)
    {
        cb_FreePool(pDestModeParams);
        pDestModeParams = CBIOS_NULL;
    }
    
    return Status;
}

CBIOS_STATUS cbDevGetModeFromReg(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_GETTING_MODE_PARAMS pModeParams)
{
    PCBIOS_EXTENSION_COMMON  pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS             csRet = CBIOS_OK;
    CBIOS_U32                IGAIndex = IGA1;
    CBIOS_TIMING_ATTRIB      timing_reg = {0};
    CBIOS_TIMING_FLAGS       flags = {0};

#ifdef CHECK_CHIPENABLE
    if (!cbHWIsChipEnable(pcbe))
    {
        csRet = CBIOS_ER_CHIPDISABLE;
        return csRet;
    }
#endif

    cbGetModeInfoFromReg(pcbe, pModeParams->Device, &timing_reg, &flags, IGAIndex, TIMING_REG_TYPE_CR);

    pModeParams->IGAIndex = IGAIndex;
    pModeParams->DestModeParams.XRes = timing_reg.XRes;
    pModeParams->DestModeParams.YRes = timing_reg.YRes;
    pModeParams->DestModeParams.RefreshRate = timing_reg.RefreshRate;
    pModeParams->DestModeParams.InterlaceFlag = (flags.IsInterlace == 0) ? 0 : 1;

    cb_memcpy(&(pModeParams->DetailedTiming), &timing_reg, sizeof(CBIOS_TIMING_ATTRIB));
    return csRet;
}

CBIOS_U32 cbDevNonDestructiveDeviceDetectByEdid(PCBIOS_VOID pvcbe, PCBIOS_UCHAR pEDIDData, CBIOS_U32 EdidBufferSize)
{
    PCBIOS_EXTENSION_COMMON  pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32                DeviceSupported, Devices = 0;
    CBIOS_ACTIVE_TYPE        DeviceType = CBIOS_TYPE_NONE;
    PCBIOS_DEVICE_COMMON     pDevCommon = CBIOS_NULL;

    DeviceSupported = pcbe->DeviceMgr.SupportDevices;

    if (DeviceSupported & CBIOS_TYPE_CRT)
    {
        pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, CBIOS_TYPE_CRT);

        if ((*(pEDIDData+EDID_VIDEO_INPUT_DEF_BYTE_OFFSET)) & EDID_VIDEO_INPUT_DEF_DIGITAL)
        {
            pDevCommon->CurrentMonitorType &= ~CBIOS_MONITOR_TYPE_CRT;
            cb_memset(pDevCommon->EdidData, 0, CBIOS_EDIDDATABYTE);
            pDevCommon->isFakeEdid = CBIOS_FALSE;
        }
        else
        {
            cb_memcpy(pDevCommon->EdidData, pEDIDData, EdidBufferSize);
            Devices |= CBIOS_TYPE_CRT;
            pDevCommon->CurrentMonitorType = CBIOS_MONITOR_TYPE_CRT;
            pDevCommon->DeviceType = CBIOS_TYPE_CRT;
            pDevCommon->isFakeEdid = CBIOS_TRUE;
        }
    }

    if (DeviceSupported & CBIOS_TYPE_MHL)
    {
        pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, CBIOS_TYPE_MHL);

        cb_memcpy(pDevCommon->EdidData, pEDIDData, EdidBufferSize);
        Devices |= CBIOS_TYPE_MHL;
        pDevCommon->CurrentMonitorType = CBIOS_MONITOR_TYPE_MHL;
        pDevCommon->DeviceType = CBIOS_TYPE_MHL;
        pDevCommon->isFakeEdid = CBIOS_TRUE;
    }

    if (DeviceSupported & CBIOS_TYPE_DVO)
    {
        pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, CBIOS_TYPE_DVO);
        DeviceType = CBIOS_TYPE_DVO;

        if (!((*(pEDIDData+EDID_VIDEO_INPUT_DEF_BYTE_OFFSET)) & EDID_VIDEO_INPUT_DEF_DIGITAL))
        {
            //analog monitor, reset buffers
            pDevCommon->CurrentMonitorType &= (~(CBIOS_MONITOR_TYPE_DVI | CBIOS_MONITOR_TYPE_HDMI));
            cb_memset(pDevCommon->EdidData, 0, CBIOS_EDIDDATABYTE);
            pDevCommon->isFakeEdid = CBIOS_FALSE;
        }        
        else
        {
            cb_memcpy(pDevCommon->EdidData, pEDIDData, EdidBufferSize);
            if (pDevCommon->EdidStruct.Attribute.IsCEA861HDMI)
            {
                pDevCommon->CurrentMonitorType = CBIOS_MONITOR_TYPE_HDMI;
            }
            else
            {
                pDevCommon->CurrentMonitorType = CBIOS_MONITOR_TYPE_DVI;
            }

            //update monitor type
            if (!(pDevCommon->CurrentMonitorType & cbGetSupportMonitorType(pcbe, DeviceType)))
            {
                pDevCommon->CurrentMonitorType = cbGetSupportMonitorType(pcbe, DeviceType);
            }
            
            pDevCommon->DeviceType = CBIOS_TYPE_DVO;
            pDevCommon->isFakeEdid = CBIOS_TRUE;

            Devices |= DeviceType;
        }
    }

    if(Devices == 0)
    {
        // Clear signature to make sure IsDeviceChanged return CBIOS_TRUE if a device with EDID is connected.
        cb_memset(&pDevCommon->ConnectedDevSignature, 0, sizeof(CBIOS_DEVICE_SIGNATURE));
    }

    return Devices;
}

CBIOS_BOOL cbDevDeviceDetect(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_BOOL bHardcodeDetect, CBIOS_U32 FullDetect)
{
    PCBIOS_EXTENSION_COMMON pcbe   = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_BOOL  bConnected = CBIOS_FALSE;

    if (!pDevCommon->pfncbDeviceDetect)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "%s: current device: 0x%x has no detect func.\n", FUNCTION_NAME, pDevCommon->DeviceType));
    }
    else
    {
        bConnected = pDevCommon->pfncbDeviceDetect(pcbe, pDevCommon, bHardcodeDetect, FullDetect);
    }

    return bConnected;
}

CBIOS_STATUS cbDevGetEdidFromBuffer(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_PARAM_GET_EDID pCBParamGetEdid)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U8* EdidBuffer = CBIOS_NULL, *pEdidData = CBIOS_NULL;
    CBIOS_U32 CopySize = 0, EdidSize = 0;
    CBIOS_STATUS bRetStatus = CBIOS_ER_INTERNAL;
    CBIOS_ACTIVE_TYPE Device = pCBParamGetEdid->DeviceId;

    CopySize = pCBParamGetEdid->EdidBufferLen;
    EdidBuffer = pCBParamGetEdid->EdidBuffer;

    //check hardcoded EDID
    if (pcbe->DevicesHardcodedEdid & Device)
    {
        pEdidData = pcbe->EdidFromRegistry;
        EdidSize = CBIOS_EDIDDATABYTE;
    }
    //not hardcoded, get EDID from buffer, do not read EDID again to prevent I2C bus re-enter
    else
    {
        pEdidData = pDevCommon->EdidData;
        EdidSize = CBIOS_EDIDDATABYTE;
    }

    if(cbEDIDModule_IsEDIDValid(pEdidData) && CopySize <= EdidSize)
    {
        cb_memcpy(EdidBuffer, pEdidData, CopySize);
        bRetStatus = CBIOS_OK;
    }
    else
    {
        bRetStatus = CBIOS_ER_EDID_INVALID;
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: Not a valid EDID!\n", FUNCTION_NAME));
    }

    return bRetStatus;
}

CBIOS_STATUS cbDevSetEdid(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_PARAM_SET_EDID pCBParamSetEdid)
{
    PCBIOS_UCHAR            pEDIDData = CBIOS_NULL;
    CBIOS_STATUS            status  = CBIOS_OK;

    pEDIDData = pDevCommon->EdidData;

    // When the EdidBuffer == NULL, we unlock the Edid buffer in CBIOS
    if((pCBParamSetEdid->EdidBuffer == CBIOS_NULL) || (pCBParamSetEdid->EdidBufferLen == 0))
    {
        cb_memset(pEDIDData, 0, CBIOS_EDIDDATABYTE);
        cb_memset(&(pDevCommon->ConnectedDevSignature), 0, sizeof(CBIOS_DEVICE_SIGNATURE));
        pDevCommon->isFakeEdid = CBIOS_FALSE;
        return CBIOS_OK;
    }
    
    if(pCBParamSetEdid->EdidBufferLen <= CBIOS_EDIDDATABYTE)
    {
        if(!cbEDIDModule_IsEDIDValid(pCBParamSetEdid->EdidBuffer))
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: EDID is invalid\n", FUNCTION_NAME));
            status = CBIOS_ER_INVALID_PARAMETER;
        }
        else
        {
            cb_memset(pEDIDData, 0, CBIOS_EDIDDATABYTE);
            cb_memset(&(pDevCommon->ConnectedDevSignature), 0, sizeof(CBIOS_DEVICE_SIGNATURE));

            cb_memcpy(pEDIDData, pCBParamSetEdid->EdidBuffer, pCBParamSetEdid->EdidBufferLen);
            pDevCommon->isFakeEdid = CBIOS_TRUE;
        }
    }
    else
    {
        status = CBIOS_ER_INVALID_PARAMETER;
    }

    return status;
}

CBIOS_STATUS cbDevGetSupportedMonitorType(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_QUERY_MONITOR_TYPE_PER_PORT pCBiosQueryMonitorTypePerPort)
{
    if(pCBiosQueryMonitorTypePerPort->bConnected)
    {
        pCBiosQueryMonitorTypePerPort->MonitorType = pDevCommon->CurrentMonitorType;
    }
    else
    {
        pCBiosQueryMonitorTypePerPort->MonitorType = pDevCommon->SupportMonitorType;
    }
    return CBIOS_OK;
}

CBIOS_STATUS cbDevQueryMonitorAttribute(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBiosMonitorAttribute pMonitorAttribute)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;

    /*
    ** 1. get basic monitor attributes
    */
    pMonitorAttribute->MonitorType = pDevCommon->CurrentMonitorType;
    pMonitorAttribute->MonitorCaps = pDevCommon->EdidStruct.Attribute.CEA861MonitorCaps;
    pMonitorAttribute->bSupportBLCtrl = CBIOS_FALSE;
    pMonitorAttribute->bSupportHDAudio = pDevCommon->EdidStruct.Attribute.IsCEA861Audio;
    pMonitorAttribute->SupportBPC.IsSupport8BPC = CBIOS_TRUE;
    // get monitor screen image size
    pMonitorAttribute->MonitorHorSize = pDevCommon->EdidStruct.Attribute.MonitorHorSize;
    pMonitorAttribute->MonitorVerSize = pDevCommon->EdidStruct.Attribute.MonitorVerSize;
    if (cbEDIDModule_IsEDIDValid(pDevCommon->EdidData))
    {
        cbEDIDModule_GetMonitorID(pDevCommon->EdidData, pMonitorAttribute->MonitorID);
    }

    /*
    ** 2. get special monitor attributes
    */
    if (pDevCommon->pfncbQueryMonitorAttribute)
    {
        pDevCommon->pfncbQueryMonitorAttribute(pcbe, pDevCommon, pMonitorAttribute);
    }
    return CBIOS_OK;
}

CBIOS_STATUS cbDevQueryMonitor3DCapability(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_MONITOR_3D_CAPABILITY_PARA p3DCapability)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32 Status = CBIOS_OK;
    
    Status = cbEDIDModule_GetMonitor3DCaps(&(pDevCommon->EdidStruct), p3DCapability, pcbe->ChipCaps.IsSupport3DVideo);
    return Status;
}

CBIOS_STATUS cbGetDeviceName(PCBIOS_VOID pvcbe,   PCBIOS_GET_DEVICE_NAME  pGetName)
{
    CBIOS_U32  Status = CBIOS_OK;
    CBIOS_ACTIVE_TYPE  DeviceType = (CBIOS_ACTIVE_TYPE)pGetName->DeviceId;
    
    switch(DeviceType)
    {
    case  CBIOS_TYPE_CRT:
        pGetName->pDeviceName = "CRT1";
        break;
    case CBIOS_TYPE_TV:
        pGetName->pDeviceName = "TV";
        break;
    case CBIOS_TYPE_HDTV:
        pGetName->pDeviceName = "HDTV";
        break;
    case CBIOS_TYPE_DP1:
        pGetName->pDeviceName = "DP1";
        break;
    case CBIOS_TYPE_DP2:
        pGetName->pDeviceName = "DP2";
        break;
    case CBIOS_TYPE_DP3:
        pGetName->pDeviceName = "DP3";
        break;
    case CBIOS_TYPE_DP4:
        pGetName->pDeviceName = "DP4";
        break;
    case  CBIOS_TYPE_DVO:
        pGetName->pDeviceName = "DVO";
        break;
    default:
        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "Invalid device type 0x%x in query name.\n", DeviceType));
        pGetName->pDeviceName = "NONE";
        break;
    }

    return  Status;
}

CBIOS_STATUS cbDevGetDeviceModeList(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBiosModeInfoExt pModeList, CBIOS_U32 *pBufferSize)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32 ulModeNum;
    CBIOS_MODE_FILTER_PARA ModeFilter = {0};

    cbMode_GetFilterPara(pcbe, pDevCommon->DeviceType, &ModeFilter);

    ulModeNum = cbMode_GetDeviceModeList(pcbe, 
                                         pDevCommon->DeviceType, 
                                         pModeList, 
                                         pBufferSize,
                                         &ModeFilter);

    if(ulModeNum == 0)
    {
        ulModeNum = cbMode_GetDefaultModeList(pcbe, pModeList, pDevCommon->DeviceType);
    }

    *pBufferSize = ulModeNum*sizeof(CBiosModeInfoExt);

    return CBIOS_OK;
}

CBIOS_STATUS cbDevGetDeviceModeListBufferSize(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_U32 *pBufferSize)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32 ulNumOfMode = 0;
    CBIOS_MODE_FILTER_PARA ModeFilter = {0};

    cbMode_GetFilterPara(pcbe, pDevCommon->DeviceType, &ModeFilter);
    
    ulNumOfMode = cbMode_GetDeviceModeList(pcbe, 
                                           pDevCommon->DeviceType, 
                                           CBIOS_NULL, 
                                           CBIOS_NULL,
                                           &ModeFilter);

    if(ulNumOfMode == 0)
    {
        ulNumOfMode = cbMode_GetDefaultModeList(pcbe, CBIOS_NULL, pDevCommon->DeviceType);
    }

    *pBufferSize = ulNumOfMode * sizeof(CBiosModeInfoExt);

    return CBIOS_OK;
}

CBIOS_U32 cbDevGetHDAFormatList(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_HDMI_AUDIO_INFO pHDAFormatList)
{
    CBIOS_U32 HDAFormatNum = 0;

    if ((pDevCommon->EdidStruct.Attribute.IsCEA861Audio) || (pDevCommon->EdidStruct.TotalHDMIAudioFormatNum != 0))
    {
        HDAFormatNum = pDevCommon->EdidStruct.TotalHDMIAudioFormatNum;
        if (pHDAFormatList != CBIOS_NULL)
        {
            cb_memcpy(pHDAFormatList, pDevCommon->EdidStruct.HDMIAudioFormat, HDAFormatNum * sizeof(CBIOS_HDMI_AUDIO_INFO));
        }
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "%s: Device:0x%x doesn't support HD audio!\n", FUNCTION_NAME, pDevCommon->DeviceType));
        HDAFormatNum = 0;
    }

    return HDAFormatNum;
}

CBIOS_STATUS cbDevSetDisplayDevicePowerState(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_PM_STATUS PMState)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_BOOL bTurnOn = CBIOS_FALSE;
    CBIOS_STATUS Status = CBIOS_OK;

    if(PMState == CBIOS_PM_ON)
    {
        bTurnOn = CBIOS_TRUE;
    }
    else
    {
        bTurnOn = CBIOS_FALSE;
    }

    if (pDevCommon->pfncbDeviceOnOff)
    {
        pDevCommon->pfncbDeviceOnOff(pcbe, pDevCommon, bTurnOn);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "%s: current device: 0x%x has no func to turn on/off.\n", FUNCTION_NAME, pDevCommon->DeviceType));
    }

    if (bTurnOn)
    {
#if DVO_PORT_SUPPORT
        if (pDevCommon->DeviceType & CBIOS_TYPE_DVO)
        {

        }
#endif
    }

    return Status;
}

CBIOS_STATUS cbDevGetDisplayDevicePowerState(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_PM_STATUS pPMState)
{
    //Assume invalid parameter
    *pPMState = CBIOS_PM_INVALID;

    *pPMState = pDevCommon->PowerState;

    if((*pPMState) == CBIOS_PM_INVALID)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: PM State is Invalid!\n", FUNCTION_NAME));
        return CBIOS_ER_INVALID_PARAMETER;
    }
    else
    {
        return CBIOS_OK;
    }
}

CBIOS_STATUS cbDevDSISendWriteCmd(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DSI_WRITE_PARA_INTERNAL pDSIWriteParams)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    return cbDSI_SendWriteCmd(pcbe, pDSIWriteParams);
}

CBIOS_STATUS cbDevDSISendReadCmd(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DSI_READ_PARA_INTERNAL pDSIReadParams)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    return cbDSI_SendReadCmd(pcbe, pDSIReadParams);
}

CBIOS_STATUS cbDevDSIDisplayUpdate(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DSI_UPDATE_PARA pDSIUpdatePara)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    return cbDSI_DisplayUpdate(pcbe, pDSIUpdatePara);
}

CBIOS_STATUS cbDevDSIPanelSetCabc(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_U32 CabcValue)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DSI_PANEL_DESC pDSIPanelDesc = &(pDevCommon->DeviceParas.DSIDevice.DSIPanelDesc);
    
    return cbDSIPanel_SetCabc(pcbe, CabcValue, pDSIPanelDesc);
}

CBIOS_STATUS cbDevDSIPanelSetBacklight(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_U32 BacklightValue)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DSI_PANEL_DESC pDSIPanelDesc = &(pDevCommon->DeviceParas.DSIDevice.DSIPanelDesc);

    return cbDSIPanel_SetBacklight(pcbe, BacklightValue, pDSIPanelDesc);
}

CBIOS_STATUS cbDevDSIPanelGetBacklight(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_U32 *pBacklightValue)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DSI_PANEL_DESC pDSIPanelDesc = &(pDevCommon->DeviceParas.DSIDevice.DSIPanelDesc);

    return cbDSIPanel_GetBacklight(pcbe, pBacklightValue, pDSIPanelDesc);
}

CBIOS_STATUS cbDevSetHDACodecPara(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_HDAC_PARA pCbiosHDACPara)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;

    cbDIU_HDAC_SetHDACodecPara(pcbe, pCbiosHDACPara);

    return CBIOS_OK;
}

CBIOS_STATUS cbDevSetHDACConnectStatus(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_HDAC_PARA pCbiosHDACPara)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;

    cbDIU_HDAC_SetConnectStatus(pcbe, pCbiosHDACPara);

    return CBIOS_OK;
}

CBIOS_STATUS cbDevAccessDpcdData(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBiosAccessDpcdDataParams pAccessDpcdDataParams)
{
#if DP_MONITOR_SUPPORT
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32 offset, BufferSize, i;    
    CBIOS_UCHAR *DataBuffer = CBIOS_NULL;
    CBIOS_MODULE_INDEX DPModuleIndex = CBIOS_MODULE_INDEX_INVALID;
    AUX_CONTROL AUX = {0};

    offset = pAccessDpcdDataParams->StartingAddress;
    DataBuffer = pAccessDpcdDataParams->pOutBuffer;
    BufferSize = pAccessDpcdDataParams->NumOfBytes;

    DPModuleIndex = cbGetModuleIndex(pcbe, pAccessDpcdDataParams->RequestConnectedDevId, CBIOS_MODULE_TYPE_DP);
    if (DPModuleIndex == CBIOS_MODULE_INDEX_INVALID)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return CBIOS_ER_INVALID_PARAMETER;
    }

#ifdef CHECK_CHIPENABLE
    if (!cbHWIsChipEnable(pcbe))
        return CBIOS_ER_CHIPDISABLE;
#endif
    if(pAccessDpcdDataParams->pOutBuffer == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: Data output buffer pointer is CBIOS_NULL!\n", FUNCTION_NAME));
        return CBIOS_ER_NULLPOINTER;
    }

    if (pAccessDpcdDataParams->ReadWriteFlag == 0)
    {
        for (i=0; i<BufferSize; i++)
        {
            AUX.Function = CBIOS_AUX_REQUEST_NATIVE_READ;
            AUX.Offset = offset;
            AUX.Length = 1;
            if(cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
            {                    
                *(DataBuffer+i) = (CBIOS_UCHAR) (AUX.Data[0] & 0x000000FF);
                offset++;
            }
            else
            {
                cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: IO read error!\n", FUNCTION_NAME));
                return CBIOS_ER_IO;
            }
        }
    }
    else if (pAccessDpcdDataParams->ReadWriteFlag == 1)
    {
        for (i=0; i<BufferSize; i++)
        {
            AUX.Function = CBIOS_AUX_REQUEST_NATIVE_WRITE;
            AUX.Offset = offset;
            AUX.Length = 1;
            AUX.Data[0] = *(DataBuffer+i);
            if(cbDIU_DP_AuxChRW(pcbe, DPModuleIndex, &AUX))
            {                    
                offset++;
            }
            else
            {
                cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: IO write error!\n", FUNCTION_NAME));
                return CBIOS_ER_IO;
            }
        }
    }        

#endif
    return CBIOS_OK;
}

CBIOS_STATUS cbDevContentProtectionOnOff(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_U8 IGAIndex, PCBiosContentProtectionOnOffParams pContentProtectionOnOffParams)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS Status = CBIOS_ER_INVALID_PARAMETER;
    CBIOS_MODULE_INDEX HDCPModuleIndex = CBIOS_MODULE_INDEX_INVALID;

    HDCPModuleIndex = cbGetModuleIndex(pcbe, pContentProtectionOnOffParams->DevicesId, CBIOS_MODULE_TYPE_HDCP);
    if (HDCPModuleIndex != CBIOS_MODULE_INDEX_INVALID)
    {
        cbHDCP_OnOff(pcbe, pContentProtectionOnOffParams->DevicesId, IGAIndex, pContentProtectionOnOffParams->bHdcpStatus);
        Status = CBIOS_OK;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        Status = CBIOS_ER_INVALID_PARAMETER;
    }
    return Status;
}

CBIOS_STATUS cbDevGetHDCPStatus(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_HDCP_STATUS_PARA pCBiosHdcpStatusParams)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS Status = CBIOS_ER_INVALID_PARAMETER;
    CBIOS_MODULE_INDEX HDCPModuleIndex = CBIOS_MODULE_INDEX_INVALID;

    HDCPModuleIndex = cbGetModuleIndex(pcbe, pCBiosHdcpStatusParams->DevicesId, CBIOS_MODULE_TYPE_HDCP);
    if (HDCPModuleIndex != CBIOS_MODULE_INDEX_INVALID)
    {
        pCBiosHdcpStatusParams->HdcpStatus = cbHDCP_GetStatus(pcbe, pCBiosHdcpStatusParams->DevicesId);
        cbHDCP_GetHDCPBKsv(pcbe, pCBiosHdcpStatusParams->DevicesId, pCBiosHdcpStatusParams->BKsv, &pCBiosHdcpStatusParams->bRepeater);
        Status = CBIOS_OK;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: device:0x%x doesn't support HDCP!\n", FUNCTION_NAME, pCBiosHdcpStatusParams->DevicesId));
        Status = CBIOS_ER_INVALID_PARAMETER;
    }
    return Status;
}

CBIOS_STATUS cbDevHDCPWorkThread(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_U8 IGAIndex, PCBIOS_HDCP_WORK_PARA pCBiosHdcpWorkParams)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS Status = CBIOS_ER_INVALID_PARAMETER;
    CBIOS_MODULE_INDEX HDCPModuleIndex = CBIOS_MODULE_INDEX_INVALID;

    HDCPModuleIndex = cbGetModuleIndex(pcbe, pCBiosHdcpWorkParams->DevicesId, CBIOS_MODULE_TYPE_HDCP);
    if (HDCPModuleIndex != CBIOS_MODULE_INDEX_INVALID)
    {
        cbHDCP_WorkThread(pcbe, pCBiosHdcpWorkParams->DevicesId, IGAIndex);
        Status = CBIOS_OK;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: device:0x%x doesn't support HDCP!\n", FUNCTION_NAME, pCBiosHdcpWorkParams->DevicesId));
        Status = CBIOS_ER_INVALID_PARAMETER;
    }
    return Status;
}

CBIOS_STATUS cbDevHDCPIsr(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_HDCP_ISR_PARA pHdcpIsrParam)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS Status = CBIOS_ER_INVALID_PARAMETER;
    CBIOS_MODULE_INDEX HDCPModuleIndex = CBIOS_MODULE_INDEX_INVALID;

    HDCPModuleIndex = cbGetModuleIndex(pcbe, pHdcpIsrParam->DevicesId, CBIOS_MODULE_TYPE_HDCP);
    if (HDCPModuleIndex != CBIOS_MODULE_INDEX_INVALID)
    {
        cbHDCP_Isr(pcbe, pHdcpIsrParam->DevicesId);
        Status = CBIOS_OK;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: device:0x%x doesn't support HDCP!\n", FUNCTION_NAME, pHdcpIsrParam->DevicesId));
        Status = CBIOS_ER_INVALID_PARAMETER;
    }
    return Status;
}

CBIOS_BOOL cbDevHdmiSCDCWorkThreadMainFunc(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon)
{
    CBIOS_BOOL bRet = CBIOS_FALSE;
    
    bRet = cbHDMIMonitor_SCDC_Handler(pvcbe, pDevCommon);
    
    return bRet;
}

CBIOS_STATUS cbDevDPIsr(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_ISR_PARA pDPIsrPara)
{
    return cbDPPort_Isr(pvcbe, pDevCommon, pDPIsrPara);
}

CBIOS_BOOL cbDevDPWorkThreadMainFunc(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_WORKTHREAD_PARA pDPWorkThreadPara)
{
    return cbDPPort_WorkThreadMainFunc(pvcbe, pDevCommon, pDPWorkThreadPara);
}

CBIOS_STATUS cbDevDPSetNotifications(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_NOTIFICATIONS pDPNotifications)
{
    return cbDPPort_SetNotifications(pvcbe, pDevCommon, pDPNotifications);
}

CBIOS_STATUS  cbDevGetDPIntType(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_INT_PARA pDpIntPara)
{
    return  cbDPPort_GetInt(pvcbe, pDevCommon, pDpIntPara);
}

CBIOS_STATUS  cbDevDPHandleIrq(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_HANDLE_IRQ_PARA pDPHandleIrqPara)
{
    return  cbDPPort_HandleIrq(pvcbe, pDevCommon, pDPHandleIrqPara);
}

CBIOS_STATUS cbDevDPGetCustomizedTiming(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DP_CUSTOMIZED_TIMING pDPCustomizedTiming)
{
    return cbDPPort_GetCustomizedTiming(pvcbe, pDevCommon, pDPCustomizedTiming);
}
