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
** CBios path manager interface function implementation. 
** Generate display source path, DIU module index and devices combination.
**
** NOTE:
**The hw dependent function or structure SHOULD NOT be added to this file.
******************************************************************************/

#include "CBiosPathManager.h"
#include "CBiosChipShare.h"
#include "../Hw/HwBlock/CBiosDIU_HDCP.h"
#include "../Hw/HwBlock/CBiosDIU_HDTV.h"
#include "../Hw/HwBlock/CBiosDIU_HDMI.h"


/*
Per Leo, this function was initially designed to filter unvalid simul view devices combination
but now CBIOS will get the supported simul devices(according to customer's request) from VBIOS
So now just leave this function here, if CBIOS need add rules to filter, just change this function.
*/

//for two devices comb, mask is the device which support hpd, for one device comb, mask keep the old definition
static CBIOS_DEVICE_COMB DeviceCombs[] = 
{
    {CBIOS_TYPE_CRT,                                   CBIOS_TYPE_NONE, CBIOS_TYPE_NONE, CBIOS_TYPE_CRT},
    {CBIOS_TYPE_DVO,                                   CBIOS_TYPE_NONE, CBIOS_TYPE_NONE, CBIOS_TYPE_DVO},
    {CBIOS_TYPE_DP1,                                   CBIOS_TYPE_DP1,  CBIOS_TYPE_NONE, CBIOS_TYPE_NONE},
    {CBIOS_TYPE_DP2,                                   CBIOS_TYPE_NONE, CBIOS_TYPE_DP2,  CBIOS_TYPE_NONE},
    {CBIOS_TYPE_CRT + CBIOS_TYPE_DP1,                  CBIOS_TYPE_DP1,  CBIOS_TYPE_NONE, CBIOS_TYPE_CRT },
    {CBIOS_TYPE_CRT + CBIOS_TYPE_DP2,                  CBIOS_TYPE_NONE, CBIOS_TYPE_DP2,  CBIOS_TYPE_CRT },
    {CBIOS_TYPE_DVO + CBIOS_TYPE_DP1,                  CBIOS_TYPE_DP1,  CBIOS_TYPE_NONE, CBIOS_TYPE_DVO },
    {CBIOS_TYPE_DVO + CBIOS_TYPE_DP2,                  CBIOS_TYPE_NONE, CBIOS_TYPE_DP2,  CBIOS_TYPE_DVO },
    {CBIOS_TYPE_DP1 + CBIOS_TYPE_DP2,                  CBIOS_TYPE_DP1,  CBIOS_TYPE_DP2,  CBIOS_TYPE_NONE},
    {CBIOS_TYPE_CRT + CBIOS_TYPE_DP1 + CBIOS_TYPE_DP2, CBIOS_TYPE_DP1,  CBIOS_TYPE_DP2,  CBIOS_TYPE_CRT},
    {CBIOS_TYPE_DVO + CBIOS_TYPE_DP1 + CBIOS_TYPE_DP2, CBIOS_TYPE_DP1,  CBIOS_TYPE_DP2,  CBIOS_TYPE_DVO},
};

CBIOS_STATUS cbPathMgrGetDevComb(PCBIOS_VOID pvcbe, PCBIOS_GET_DEV_COMB pDevComb)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DEVICE_COMB pDeviceComb = pDevComb->pDeviceComb;
    CBIOS_ACTIVE_TYPE  Devices = pDeviceComb->Devices;
    CBIOS_U32  i = 0;    

    pDeviceComb->Iga1Dev = CBIOS_TYPE_NONE;
    pDeviceComb->Iga2Dev = CBIOS_TYPE_NONE;
    pDeviceComb->Iga3Dev = CBIOS_TYPE_NONE;
    pDeviceComb->Iga4Dev = CBIOS_TYPE_NONE;

    if (Devices == CBIOS_TYPE_NONE)
    {
        return  CBIOS_OK;
    }

    if (pcbe->bRunOnQT)
    {
        pDeviceComb->Iga1Dev = CBIOS_TYPE_CRT;
        pDevComb->bSupported = CBIOS_TRUE;

        return  CBIOS_OK;
    }
    
    if ((pcbe->ChipID == CHIPID_E3K) || (pcbe->ChipID == CHIPID_ARISE1020))
    {
        if ((cbGetBitsNum(Devices) > CBIOS_IGACOUNTS) || (Devices & ~(pcbe->DeviceMgr.SupportDevices)))
        {
            pDevComb->bSupported = CBIOS_FALSE;
            return CBIOS_ER_INVALID_PARAMETER;
        }
        else
        {
            if(Devices & CBIOS_TYPE_DP1)
            {
                pDevComb->pDeviceComb->Iga1Dev = CBIOS_TYPE_DP1;
            }

            if(Devices & CBIOS_TYPE_DP2)
            {
                pDevComb->pDeviceComb->Iga2Dev = CBIOS_TYPE_DP2;
            }

            if(Devices & CBIOS_TYPE_DP3)
            {
                pDevComb->pDeviceComb->Iga3Dev = CBIOS_TYPE_DP3;
            }

            if(Devices & CBIOS_TYPE_DP4)
            {
                pDevComb->pDeviceComb->Iga4Dev = CBIOS_TYPE_DP4;
            }
            else if(Devices & CBIOS_TYPE_CRT)
            {
                pDevComb->pDeviceComb->Iga4Dev = CBIOS_TYPE_CRT;
            }
            pDevComb->bSupported = CBIOS_TRUE;
            return CBIOS_OK;
        }
    }
    else
    {
        for (i = 0; i < sizeofarray(DeviceCombs); i++)
        {
            if ((DeviceCombs[i].Devices) == Devices)
            {
                break;
            }
        }

        if(i != sizeofarray(DeviceCombs))
        {
            pDevComb->pDeviceComb->Iga1Dev = DeviceCombs[i].Iga1Dev;
            pDevComb->pDeviceComb->Iga2Dev = DeviceCombs[i].Iga2Dev;
            pDevComb->pDeviceComb->Iga3Dev = DeviceCombs[i].Iga3Dev;
            pDevComb->bSupported = CBIOS_TRUE;
        }
        else
        {
            pDevComb->bSupported = CBIOS_FALSE;
        }
    }
    
    return  CBIOS_OK;
}

CBIOS_STATUS cbPathMgrGetIgaMask(PCBIOS_VOID pvcbe, PCBIOS_GET_IGA_MASK pGetIgaMask)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;

    if (pcbe->bRunOnQT)
    {
        pGetIgaMask->IgaMask = 1 << IGA1;
        return  CBIOS_OK;
    }

    if ((pcbe->ChipID == CHIPID_E3K) || (pcbe->ChipID == CHIPID_ARISE1020))
    {
        switch(pGetIgaMask->DeviceId)
        {
        case CBIOS_TYPE_DP1:
            pGetIgaMask->IgaMask = 1 << IGA1;
            break;
        case CBIOS_TYPE_DP2:
            pGetIgaMask->IgaMask = 1 << IGA2;
            break;
        case CBIOS_TYPE_DP3:
            pGetIgaMask->IgaMask = 1 << IGA3;
            break;
        case CBIOS_TYPE_DP4:
            pGetIgaMask->IgaMask = 1 << IGA4;
            break;
        case CBIOS_TYPE_CRT:
            pGetIgaMask->IgaMask = 1 << IGA4;
            break;
        default:
            pGetIgaMask->IgaMask = (1 << IGA1) | (1 << IGA2) | (1 << IGA3) | (1 << IGA4);
            cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "%s: invalid device: 0x%x.\n", FUNCTION_NAME, pGetIgaMask->DeviceId));
            break; 
        }
    }
    else
    {
        switch(pGetIgaMask->DeviceId)
        {
        case CBIOS_TYPE_CRT:
        case CBIOS_TYPE_DVO:
            pGetIgaMask->IgaMask = 1 << IGA3;
            break;
        case CBIOS_TYPE_DP1:
            pGetIgaMask->IgaMask = 1 << IGA1;
            break;
        case CBIOS_TYPE_DP2:
            pGetIgaMask->IgaMask = 1 << IGA2;
            break;
        default:
            pGetIgaMask->IgaMask = (1 << IGA3) | (1 << IGA2);
            break; 
        }
    }
    return  CBIOS_OK;
}


static CBIOS_VOID cbPathMgrGeneratePath(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_DISPLAY_SOURCE *pSource)
{
    CBIOS_MODULE_LIST *pModuleList = CBIOS_NULL;
    CBIOS_U32                index = 0;

    if (pSource == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: the 2nd param is NULL!\n", FUNCTION_NAME));
        return;
    }

    pModuleList = &pSource->ModuleList;

    cb_memset(pSource->ModulePath, 0, sizeof(CBIOS_MODULE*) * CBIOS_MAX_MODULE_NUM);
    if (pModuleList->HDCPModule.Index != CBIOS_MODULE_INDEX_INVALID)
    {
        pSource->ModulePath[index] = &pModuleList->HDCPModule;
        index++;
    }

    if (pModuleList->HDMIModule.Index != CBIOS_MODULE_INDEX_INVALID)
    {
        pSource->ModulePath[index] = &pModuleList->HDMIModule;
        index++;
    }

    if (pModuleList->HDTVModule.Index != CBIOS_MODULE_INDEX_INVALID)
    {
        pSource->ModulePath[index] = &pModuleList->HDTVModule;
        index++;
    }

    if (pModuleList->IGAModule.Index != CBIOS_MODULE_INDEX_INVALID)
    {
        pSource->ModulePath[index] = &pModuleList->IGAModule;
        index++;
    }
}


CBIOS_VOID cbPathMgrSelectDIUPath(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device, CBIOS_U32 IGAIndex, PCBiosDestModeParams pDestModeParams)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_DISPLAY_SOURCE   *pSrc = CBIOS_NULL;
    CBIOS_BOOL              isHDMIDevice = CBIOS_FALSE;
    PCBIOS_DEVICE_COMMON    pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, Device);
    CBIOS_U32               bInterlace = pDestModeParams->InterlaceFlag;
    CBIOS_MODULE_INDEX      ModuleIndex = CBIOS_MODULE_INDEX_INVALID;

    pSrc = &(pDevCommon->DispSource);
    isHDMIDevice = pDevCommon->EdidStruct.Attribute.IsCEA861HDMI;

    if (IGAIndex == IGA1)
    {
        pSrc->ModuleList.IGAModule.Index = CBIOS_MODULE_INDEX1;
    }
    else if (IGAIndex == IGA2)
    {
        pSrc->ModuleList.IGAModule.Index = CBIOS_MODULE_INDEX2;
    }
    else if (IGAIndex == IGA3)
    {
        pSrc->ModuleList.IGAModule.Index = CBIOS_MODULE_INDEX3;
    }
    else if(IGAIndex == IGA4)
    {
        pSrc->ModuleList.IGAModule.Index = CBIOS_MODULE_INDEX4;
    }
    else
    {
        pSrc->ModuleList.IGAModule.Index = CBIOS_MODULE_INDEX_INVALID;
    }

    switch (Device)
    {
    case CBIOS_TYPE_DP1: // currently DVI mode doesn't support interlaced timing
    case CBIOS_TYPE_DP2:
    case CBIOS_TYPE_DP3:
    case CBIOS_TYPE_DP4:
        ModuleIndex = CBIOS_MODULE_INDEX_INVALID;
        if (Device == CBIOS_TYPE_DP1)
        {
            ModuleIndex = CBIOS_MODULE_INDEX1;
        }
        else if (Device == CBIOS_TYPE_DP2)
        {
            ModuleIndex = CBIOS_MODULE_INDEX2;
        }
        else if (Device == CBIOS_TYPE_DP3)
        {
            ModuleIndex = CBIOS_MODULE_INDEX3;
        }
        else if (Device == CBIOS_TYPE_DP4)
        {
            ModuleIndex = CBIOS_MODULE_INDEX4;
        }
        
        pSrc->ModuleList.HDTVModule.Index = CBIOS_MODULE_INDEX_INVALID;
        if (pDevCommon->CurrentMonitorType & (CBIOS_MONITOR_TYPE_HDMI | CBIOS_MONITOR_TYPE_DVI))
        {
            if (isHDMIDevice)
            {
                pSrc->ModuleList.HDMIModule.Index = ModuleIndex;
                if (Device == CBIOS_TYPE_DP1)
                {
                    pSrc->ModuleList.HDACModule.Index = CBIOS_MODULE_INDEX1;
                }
                else if (Device == CBIOS_TYPE_DP2)
                {
                    pSrc->ModuleList.HDACModule.Index = CBIOS_MODULE_INDEX2;
                }
                else
                {
                    pSrc->ModuleList.HDACModule.Index = CBIOS_MODULE_INDEX_INVALID;
                }

                if(bInterlace || ((pDestModeParams->XRes == 720) && (pDestModeParams->YRes == 576)))
                {
                    pSrc->ModuleList.HDTVModule.Index = ModuleIndex;
                }
            }
            else
            {
                pSrc->ModuleList.HDMIModule.Index = ModuleIndex;
                pSrc->ModuleList.HDACModule.Index = CBIOS_MODULE_INDEX_INVALID;
            }
        }
        else
        {
            pSrc->ModuleList.HDMIModule.Index = CBIOS_MODULE_INDEX_INVALID;
            if (Device == CBIOS_TYPE_DP1)
            {
                pSrc->ModuleList.HDACModule.Index = CBIOS_MODULE_INDEX1;
            }
            else if (Device == CBIOS_TYPE_DP2)
            {
                pSrc->ModuleList.HDACModule.Index = CBIOS_MODULE_INDEX2;
            }
            else
            {
                pSrc->ModuleList.HDACModule.Index = CBIOS_MODULE_INDEX_INVALID;
            }

            if(bInterlace || ((pDestModeParams->XRes == 720) && (pDestModeParams->YRes == 576)))
            {
                pSrc->ModuleList.HDTVModule.Index = ModuleIndex;
            }
        }
        cbPathMgrGeneratePath(pcbe, pSrc);
        break;
    case CBIOS_TYPE_MHL: // currently DVI mode doesn't support interlaced timing
        pSrc->ModuleList.HDCPModule.Index = CBIOS_MODULE_INDEX1;
        pSrc->ModuleList.HDTVModule.Index = CBIOS_MODULE_INDEX_INVALID;

        if (isHDMIDevice)
        {
            pSrc->ModuleList.HDMIModule.Index = CBIOS_MODULE_INDEX1;
            pSrc->ModuleList.HDACModule.Index = CBIOS_MODULE_INDEX1;
        }
        else
        {
            pSrc->ModuleList.HDMIModule.Index = CBIOS_MODULE_INDEX_INVALID;
            pSrc->ModuleList.HDACModule.Index = CBIOS_MODULE_INDEX_INVALID;
        }

        cbPathMgrGeneratePath(pcbe, pSrc);
        break;
    case CBIOS_TYPE_CRT:
    case CBIOS_TYPE_DVO:
    case CBIOS_TYPE_DSI:
        cbPathMgrGeneratePath(pcbe, pSrc);
        break;
    default:
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"%s: invalid device type: 0x%x.\n", FUNCTION_NAME, Device));
        return;
    }

    pSrc->bIsSrcChanged = CBIOS_TRUE;
}


CBIOS_MODULE_INDEX cbGetModuleIndex(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device, CBIOS_MODULE_TYPE ModuleType)
{
    PCBIOS_EXTENSION_COMMON pcbe        = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_MODULE_INDEX      ModuleIndex = CBIOS_MODULE_INDEX_INVALID;
    CBIOS_DISPLAY_SOURCE   *pSource     = CBIOS_NULL;
    PCBIOS_DEVICE_COMMON    pDevCommon  = cbGetDeviceCommon(&pcbe->DeviceMgr, Device);

    if (pDevCommon == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: pDevCommon is NULL!\n", FUNCTION_NAME));
        return ModuleIndex;
    }

    pSource = &(pDevCommon->DispSource);

    switch (ModuleType)
    {
    case CBIOS_MODULE_TYPE_DP:
        ModuleIndex = pSource->ModuleList.DPModule.Index;
        break;
    case CBIOS_MODULE_TYPE_MHL:
        ModuleIndex = pSource->ModuleList.MHLModule.Index;
        break;
    case CBIOS_MODULE_TYPE_HDMI:
        ModuleIndex = pSource->ModuleList.HDMIModule.Index;
        break;
    case CBIOS_MODULE_TYPE_HDTV:
        ModuleIndex = pSource->ModuleList.HDTVModule.Index;
        break;
    case CBIOS_MODULE_TYPE_HDCP:
        ModuleIndex = pSource->ModuleList.HDCPModule.Index;
        break;
    case CBIOS_MODULE_TYPE_HDAC:
        ModuleIndex = pSource->ModuleList.HDACModule.Index;
        break;
    case CBIOS_MODULE_TYPE_IGA:
        ModuleIndex = pSource->ModuleList.IGAModule.Index;
        break;
    default:
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid module type: %d!\n", FUNCTION_NAME, ModuleType));
        break;
    }

    if (ModuleIndex == CBIOS_MODULE_INDEX_INVALID)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "%s: invalid module index of module: %d!\n", FUNCTION_NAME, ModuleType));
    }

    return ModuleIndex;
}


CBIOS_BOOL cbPathMgrModuleOnOff(PCBIOS_VOID pvcbe, CBIOS_MODULE **pModulePath, CBIOS_BOOL bTurnOn)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32 i = 0;

    if ((pModulePath == CBIOS_NULL) || (*pModulePath == CBIOS_NULL))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: the 2nd param is null!\n", FUNCTION_NAME));
        return CBIOS_FALSE;
    }

    if (bTurnOn)
    {
        while (pModulePath[i])
        {
            ++i;
        }
        --i;
    }

    while (pModulePath[i])
    {
        switch (pModulePath[i]->Type)
        {
        case CBIOS_MODULE_TYPE_HDMI:
            cbDIU_HDMI_ModuleOnOff(pcbe, pModulePath[i]->Index, bTurnOn);
            break;
        case CBIOS_MODULE_TYPE_HDTV:
            cbDIU_HDTV_ModuleOnOff(pcbe, pModulePath[i]->Index, bTurnOn);
            break;
        case CBIOS_MODULE_TYPE_HDCP:
        default:
            cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "%s: module %d does not need to be on/ff here!\n", FUNCTION_NAME, pModulePath[i]->Index));
            break;
        }

        if (bTurnOn)
        {
            if (i)
            {
                --i;
            }
            else
            {
                break;
            }
        }
        else
        {
            ++i;
        }
    }
    return CBIOS_TRUE;
}

CBIOS_BOOL cbPathMgrSelectDIUSource(PCBIOS_VOID pvcbe, CBIOS_DISPLAY_SOURCE *pSource)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_MODULE *pCurModule  = CBIOS_NULL;
    CBIOS_MODULE *pNextModule = CBIOS_NULL;
    CBIOS_U32 i = 0;

    if (pSource == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: the 2nd param is null!\n", FUNCTION_NAME));
        return CBIOS_FALSE;
    }

    while (pSource->ModulePath[i])
    {
        pCurModule = pSource->ModulePath[i];
        pNextModule = pSource->ModulePath[i + 1];

        switch (pCurModule->Type)
        {
        case CBIOS_MODULE_TYPE_HDMI:
            cbDIU_HDMI_SelectSource(pcbe, pCurModule, pNextModule);
            break;
        case CBIOS_MODULE_TYPE_HDCP:
        default:
            cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "%s: module %d does not need to select source here!\n", FUNCTION_NAME, pCurModule->Index));
            break;
        }
        i++;
    }

    return CBIOS_TRUE;
}

