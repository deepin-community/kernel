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

#include "CBiosPathManager.h"
#include "CBiosChipShare.h"
#include "../Hw/HwBlock/CBiosDIU_HDCP.h"
#include "../Hw/HwBlock/CBiosDIU_HDTV.h"
#include "../Hw/HwBlock/CBiosDIU_DP.h"
#include "../Hw/HwBlock/CBiosDIU_HDMI.h"


CBIOS_STATUS cbPathMgrGetDevComb(PCBIOS_VOID pvcbe, PCBIOS_GET_DEV_COMB pDevComb)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DEVICE_COMB pDeviceComb = pDevComb->pDeviceComb;
    CBIOS_ACTIVE_TYPE  Devices = pDeviceComb->Devices;
    CBIOS_ACTIVE_TYPE  TempDev = CBIOS_TYPE_NONE;
    CBIOS_GET_IGA_MASK GetIgaMask = {0};
    CBIOS_U32          IgaMask = 0, IgaIndex = 0;
    CBIOS_U32          DevOnIga[CBIOS_IGACOUNTS] = {0};
    CBIOS_BOOL         bAssigned = CBIOS_FALSE;

    if (Devices == CBIOS_TYPE_NONE)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Devices is NONE ! \n", FUNCTION_NAME));
        return  CBIOS_OK;
    }

    if (pcbe->bRunOnQT)
    {
        pDeviceComb->Iga1Dev = CBIOS_TYPE_CRT;
        pDevComb->bSupported = CBIOS_TRUE;

        return  CBIOS_OK;
    }

    if ((cbGetBitsNum(Devices) > pcbe->DispMgr.IgaCount) || (Devices & ~(pcbe->DeviceMgr.SupportDevices)))
    {
        pDevComb->bSupported = CBIOS_FALSE;
        return CBIOS_ER_INVALID_PARAMETER;
    }

    GetIgaMask.Size = sizeof(CBIOS_GET_IGA_MASK);
    while(Devices)
    {
        bAssigned = CBIOS_FALSE;

        //select a high priority device if more than one device
        TempDev = cbDevGetPrimaryDevice(Devices);
        Devices &= ~TempDev;
        GetIgaMask.DeviceId = (CBIOS_U32)TempDev;
        cbPathMgrGetIgaMask(pcbe, &GetIgaMask);

        IgaMask = GetIgaMask.IgaMask;
        while(IgaMask)
        {
            IgaIndex = cbGetLastBitIndex(IgaMask);
            IgaMask &= ~(1 << IgaIndex);
            if(DevOnIga[IgaIndex] == CBIOS_TYPE_NONE)
            {
                DevOnIga[IgaIndex] = TempDev;
                bAssigned = CBIOS_TRUE;
                break;
            }
        }

        if (!bAssigned)
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: Can't assign IGA for device: 0x%x\n", FUNCTION_NAME, TempDev));
            break;
        }
    }

    if(bAssigned)
    {
        pDeviceComb->Iga1Dev = DevOnIga[IGA1];
        pDeviceComb->Iga2Dev = DevOnIga[IGA2];
        pDeviceComb->Iga3Dev = DevOnIga[IGA3];
        pDeviceComb->Iga4Dev = DevOnIga[IGA4];
        pDevComb->bSupported = CBIOS_TRUE;

        return CBIOS_OK;
    }
    else
    {
        pDevComb->bSupported = CBIOS_FALSE;
        return CBIOS_ER_INVALID_PARAMETER;
    }

}

CBIOS_STATUS cbPathMgrGetIgaMask(PCBIOS_VOID pvcbe, PCBIOS_GET_IGA_MASK pGetIgaMask)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;

    if (pcbe->bRunOnQT)
    {
        pGetIgaMask->IgaMask = 1 << IGA1;
        return  CBIOS_OK;
    }

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
        if(pcbe->DispMgr.IgaCount == 4)
        {
            pGetIgaMask->IgaMask = 1 << IGA4;
        }
        else if(pcbe->DispMgr.IgaCount == 3)
        {
            pGetIgaMask->IgaMask = 1 << IGA3;
        }
        else
        {
            pGetIgaMask->IgaMask = 1 << IGA2;
        }
        break;
    default:
        pGetIgaMask->IgaMask = (1 << IGA1) | (1 << IGA2) | (1 << IGA3) | (1 << IGA4);
        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "%s: invalid device: 0x%x.\n", FUNCTION_NAME, pGetIgaMask->DeviceId));
        break;
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

CBIOS_VOID cbPathMgrSelectDIUPath(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device, CBIOS_U32 IGAIndex, PCBiosDestModeParams pDestModeParams, CBIOS_BOOL bForceHDTV)
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

                if(bForceHDTV || bInterlace || ((pDestModeParams->XRes == 720) && (pDestModeParams->YRes == 576)))
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

            if(bForceHDTV || bInterlace || ((pDestModeParams->XRes == 720) && (pDestModeParams->YRes == 576)))
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
        if(ModuleIndex >= DP_MODU_NUM)
        {
            ModuleIndex = CBIOS_MODULE_INDEX_INVALID;
        }
        break;
    case CBIOS_MODULE_TYPE_MHL:
        ModuleIndex = pSource->ModuleList.MHLModule.Index;
        break;
    case CBIOS_MODULE_TYPE_HDMI:
        ModuleIndex = pSource->ModuleList.HDMIModule.Index;
        if(ModuleIndex >= HDMI_MODU_NUM)
        {
            ModuleIndex = CBIOS_MODULE_INDEX_INVALID;
        }
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
        if((CBIOS_U32)ModuleIndex >= pcbe->DispMgr.IgaCount)
        {
            ModuleIndex = CBIOS_MODULE_INDEX_INVALID;
        }
        break;
    default:
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid module type: %d!\n", FUNCTION_NAME, ModuleType));
        break;
    }

    if (ModuleIndex == CBIOS_MODULE_INDEX_INVALID)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Invalid module %d for device 0x%x!\n", FUNCTION_NAME, ModuleType, Device));
    }

    return ModuleIndex;
}


CBIOS_BOOL cbPathMgrModuleOnOff(PCBIOS_VOID pvcbe, CBIOS_MODULE **pModulePath, CBIOS_BOOL bTurnOn)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32 i = 0;

    if ((pModulePath == CBIOS_NULL) || (*pModulePath == CBIOS_NULL))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: the 2nd param is null!\n", FUNCTION_NAME));
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

