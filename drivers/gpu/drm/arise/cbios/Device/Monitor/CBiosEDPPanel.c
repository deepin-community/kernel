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
#include "CBiosChipShare.h"
#include "CBiosEDPPanel.h"

extern CBIOS_EDP_PANEL_DESC ITN156_Panel_Desc;

#define DEFAULT_EDP_PANEL_INDEX 0
#define ITN156_PANEL_INDEX 0

static PCBIOS_EDP_PANEL_DESC EDPPanelDescTbl[] =
{
    &ITN156_Panel_Desc,
};

static CBIOS_BOOL cbEDPPanel_GetMonitorID(CBIOS_U8 *pEDID, CBIOS_U8 *pMnitorID)
{
    CBIOS_U8 *index = "0ABCDEFGHIJKLMNOPQRSTUVWXYZ[/]^_";
    CBIOS_U8 ProductID[3] = {0};
    CBIOS_BOOL bRet = CBIOS_FALSE;
    CBIOS_U8 *pMonitorIDinEDID = pEDID + 0x08;

    if ((pMonitorIDinEDID == CBIOS_NULL) || (pMnitorID == CBIOS_NULL))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbEDPPanel_GetMonitorID: input buffer is null!\n"));
        return CBIOS_FALSE;
    }

    //get manufacturer ID
    pMnitorID[0] = index[(pMonitorIDinEDID[0] >> 2) & 0x1F];
    pMnitorID[1] = index[((pMonitorIDinEDID[1] >> 5) & 0x07) | ((pMonitorIDinEDID[0] << 3) & 0x18)];
    pMnitorID[2] = index[pMonitorIDinEDID[1] & 0x1F];
    pMnitorID[3] = '\0';

    if (cbItoA((CBIOS_U32)pMonitorIDinEDID[3], ProductID, 16, 2))
    {
        cbStrCat(pMnitorID, ProductID);
        if (cbItoA((CBIOS_U32)pMonitorIDinEDID[2], ProductID, 16, 2))
        {
            cbStrCat(pMnitorID, ProductID);
            bRet = CBIOS_TRUE;
            cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "Monitor ID is: %s\n", pMnitorID));
        }
    }

    return bRet;

}

PCBIOS_EDP_PANEL_DESC cbEDPPanel_GetPanelDescriptor(PCBIOS_VOID pvcbe, PCBIOS_U8 pEdidData)
{
    PCBIOS_EDP_PANEL_DESC pPanelDesc = CBIOS_NULL;
    CBIOS_UCHAR  MonitorID[8];
    CBIOS_U32 i = 0;

    if (cbEDPPanel_GetMonitorID(pEdidData, MonitorID))// use monitor ID to get desc temporary,need sync with sysinfo
    {
        for (i = 0; i <  sizeofarray(EDPPanelDescTbl); i++)
        {
            if (cb_strcmp(MonitorID, EDPPanelDescTbl[i]->MonitorID) == 0)
            {
                pPanelDesc = EDPPanelDescTbl[i];
                break;
            }
        }
    }

    if (pPanelDesc == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(DP, INFO), "Can't get panel descriptor for eDP list. Treat it as DP.\n"));
    }

    return pPanelDesc;
}

CBIOS_STATUS cbEDPPanel_PreInit(PCBIOS_VOID pvcbe)
{
    PCBIOS_EDP_PANEL_DESC pPanelDesc = CBIOS_NULL;
    CBIOS_U32 i = 0;

    for (i = 0; i <  sizeofarray(EDPPanelDescTbl); i++)
    {
        pPanelDesc = EDPPanelDescTbl[i];
        if (pPanelDesc->EDPPreCaps.IsNeedPreInit)
        {
            if(pPanelDesc->EDPPreCaps.pFnEDPPanelPreInit != CBIOS_NULL)
            {
                pPanelDesc->EDPPreCaps.pFnEDPPanelPreInit(pvcbe);
            }
        }
    }

    return CBIOS_OK;
}

CBIOS_STATUS cbEDPPanel_Init(PCBIOS_VOID pvcbe, PCBIOS_EDP_PANEL_DESC pPanelDesc)
{
    CBIOS_STATUS Status = CBIOS_ER_NOT_YET_IMPLEMENTED;

    if (pPanelDesc != CBIOS_NULL)
    {
        if (pPanelDesc->pFnEDPPanelInit != CBIOS_NULL)
        {
            Status = pPanelDesc->pFnEDPPanelInit(pvcbe);
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DP, WARNING), "function %s not implemented.\n", FUNCTION_NAME));
        }
        Status = CBIOS_OK;

    }
    else
    {
        Status = CBIOS_ER_NULLPOINTER;
    }


    return Status;
}


CBIOS_STATUS cbEDPPanel_DeInit(PCBIOS_VOID pvcbe, PCBIOS_EDP_PANEL_DESC pPanelDesc)
{
    CBIOS_STATUS Status = CBIOS_ER_NOT_YET_IMPLEMENTED;

    if (pPanelDesc != CBIOS_NULL)
    {
        if (pPanelDesc->pFnEDPPanelDeInit != CBIOS_NULL)
        {
            Status = pPanelDesc->pFnEDPPanelDeInit(pvcbe);
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DP, WARNING), "function %s not implemented.\n", FUNCTION_NAME));
        }
        Status = CBIOS_OK;

    }
    else
    {
        Status = CBIOS_ER_NULLPOINTER;
    }


    return Status;
}


CBIOS_STATUS cbEDPPanel_OnOff(PCBIOS_VOID pvcbe, CBIOS_BOOL bTurnOn, PCBIOS_EDP_PANEL_DESC pPanelDesc)
{
    CBIOS_STATUS Status = CBIOS_ER_NOT_YET_IMPLEMENTED;

    if (pPanelDesc != CBIOS_NULL)
    {
        if (pPanelDesc->pFnEDPPanelOnOff != CBIOS_NULL)
        {
            Status = pPanelDesc->pFnEDPPanelOnOff(pvcbe, bTurnOn);
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DP, WARNING), "function %s not implemented.\n", FUNCTION_NAME));
        }
        Status = CBIOS_OK;
    }
    else
    {
        Status = CBIOS_ER_NULLPOINTER;
    }


    return Status;
}


CBIOS_STATUS cbEDPPanel_SetBacklight(PCBIOS_VOID pvcbe, CBIOS_U32 BacklightValue, PCBIOS_EDP_PANEL_DESC pPanelDesc)
{
    PCBIOS_EDP_CAPS pEDPCaps = CBIOS_NULL;
    CBIOS_STATUS Status = CBIOS_OK;
    CBIOS_U32 BlValue = 0;

    if (pPanelDesc != CBIOS_NULL)
    {
        if (pPanelDesc->pFnEDPPanelSetBacklight!= CBIOS_NULL)
        {
            BlValue = BacklightValue;
            pEDPCaps = &(pPanelDesc->EDPCaps);
            if (BacklightValue < pEDPCaps->BacklightMin)
            {
                cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: backlight value: %d is smaller than the min backlight value, use the min.\n", FUNCTION_NAME, BacklightValue));
                BlValue = pEDPCaps->BacklightMin;
            }
            if (BacklightValue > pEDPCaps->BacklightMax)
            {
                cbDebugPrint((MAKE_LEVEL(DP, WARNING), "%s: backlight value: %d is greater than the max backlight value, use the max.\n", FUNCTION_NAME, BacklightValue));
                BlValue = pEDPCaps->BacklightMax;
            }

            Status = pPanelDesc->pFnEDPPanelSetBacklight(pvcbe, BlValue);
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DP, WARNING), "function %s not implemented. use default command list\n", FUNCTION_NAME));
        }
    }
    else
    {
        Status = CBIOS_ER_NULLPOINTER;
    }

    return Status;
}

CBIOS_STATUS cbEDPPanel_GetBacklight(PCBIOS_VOID pvcbe, CBIOS_U32 *pBacklightValue, PCBIOS_EDP_PANEL_DESC pPanelDesc)
{
    CBIOS_STATUS Status = CBIOS_OK;

    if (pPanelDesc != CBIOS_NULL)
    {

        if (pPanelDesc->pFnEDPPanelGetBacklight!= CBIOS_NULL)
        {
            Status = pPanelDesc->pFnEDPPanelGetBacklight(pvcbe, pBacklightValue);
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DP, WARNING), "function %s not implemented. \n", FUNCTION_NAME));

            *pBacklightValue = 0;
        }
    }
    else
    {
        Status = CBIOS_ER_NULLPOINTER;
    }


    return Status;
}

