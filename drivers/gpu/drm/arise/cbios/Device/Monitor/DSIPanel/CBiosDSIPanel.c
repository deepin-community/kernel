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
** DSI panel interface function implementation.
**
** NOTE:
**
******************************************************************************/


#include "CBiosDSI.h"
#include "CBiosDSIPanel.h"


extern CBIOS_DSI_PANEL_DESC HX8392A_Panel_Desc;
extern CBIOS_DSI_PANEL_DESC R63417_Panel_Desc;
extern CBIOS_DSI_PANEL_DESC NT35595_Panel_Desc;
extern CBIOS_DSI_PANEL_DESC R63319_Panel_Desc;

#define DEFAULT_PANEL_INDEX 3

#define HX8392A_PANEL_INDEX 0
#define NT35595_PANEL_INDEX 2

static PCBIOS_DSI_PANEL_DESC DSIPanelDescTbl[] = 
{
    &HX8392A_Panel_Desc,
    &R63417_Panel_Desc,
    &NT35595_Panel_Desc,
    &R63319_Panel_Desc,
};


PCBIOS_DSI_PANEL_DESC cbDSIPanel_GetPanelDescriptor(PCBIOS_VOID pvcbe)
{
    PCBIOS_DSI_PANEL_DESC pPanelDesc = CBIOS_NULL;
    CBIOS_U32 PanelIndex = DEFAULT_PANEL_INDEX;

    if (cbGetPlatformConfigurationU32(pvcbe, (CBIOS_UCHAR*)"panel_index", &PanelIndex, 1))
    {
        if (PanelIndex >=  sizeofarray(DSIPanelDescTbl))
        {
            PanelIndex = DEFAULT_PANEL_INDEX;
            cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "Invalid panel index, use default index!\n"));
        }
    }
    else
    {
        PanelIndex = DEFAULT_PANEL_INDEX;
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "Can't get panel index, use default index!\n"));
    }

#if ELT2K_HARDCODE_DSI_CMDMODE
    PanelIndex = HX8392A_PANEL_INDEX;
#endif

    pPanelDesc = DSIPanelDescTbl[PanelIndex];

    if (pPanelDesc == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "Can't get panel descriptor!\n"));
    }

    return pPanelDesc;
}

CBIOS_STATUS cbDSIPanel_Init(PCBIOS_VOID pvcbe, PCBIOS_DSI_PANEL_DESC pPanelDesc)
{
    CBIOS_STATUS Status = CBIOS_ER_NOT_YET_IMPLEMENTED;
    
    if (pPanelDesc != CBIOS_NULL)
    {
        if (pPanelDesc->pFnDSIPanelInit != CBIOS_NULL)
        {
            Status = pPanelDesc->pFnDSIPanelInit(pvcbe);
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DSI, WARNING), "function %s not implemented. use default command list\n", FUNCTION_NAME));
        }
        Status = CBIOS_OK;
        
    }
    else
    {
        Status = CBIOS_ER_NULLPOINTER;
    }
    
 
    return Status;
}


CBIOS_STATUS cbDSIPanel_DeInit(PCBIOS_VOID pvcbe, PCBIOS_DSI_PANEL_DESC pPanelDesc)
{
    CBIOS_STATUS Status = CBIOS_ER_NOT_YET_IMPLEMENTED;
    
    if (pPanelDesc != CBIOS_NULL)
    {
        if (pPanelDesc->pFnDSIPanelDeInit != CBIOS_NULL)
        {
            Status = pPanelDesc->pFnDSIPanelDeInit(pvcbe);
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DSI, WARNING), "function %s not implemented. use default command list\n", FUNCTION_NAME));
        }
        Status = CBIOS_OK;
        
    }
    else
    {
        Status = CBIOS_ER_NULLPOINTER;
    }
    
 
    return Status;
}


CBIOS_STATUS cbDSIPanel_OnOff(PCBIOS_VOID pvcbe, CBIOS_BOOL bTurnOn, PCBIOS_DSI_PANEL_DESC pPanelDesc)
{
    CBIOS_STATUS Status = CBIOS_ER_NOT_YET_IMPLEMENTED;

    if (pPanelDesc != CBIOS_NULL)
    {
        if (pPanelDesc->pFnDSIPanelOnOff != CBIOS_NULL)
        {
            Status = pPanelDesc->pFnDSIPanelOnOff(pvcbe, bTurnOn);
        }
        else
        {
            if (bTurnOn)
            {
                /**
                IOVDD ON
                >0ms
                cbDelayMilliSeconds(1);
                AVDD ON
                >2ms
                cbDelayMilliSeconds(3);
                DCDC_EN ON
                >2ms
                cbDelayMilliSeconds(3);
                RESX
                >5ms
                cbDelayMilliSeconds(6);
                **/
                cbDSI_SendCmdList(pvcbe, pPanelDesc->pPowerOnCmdList, pPanelDesc->PowerOnCmdListSize);
            }
            else
            { 
                cbDSI_SendCmdList(pvcbe, pPanelDesc->pPowerOffCmdList, pPanelDesc->PowerOffCmdListSize);
                /**
                DCDC_EN OFF
                >1ms
                cbDelayMilliSeconds(2);
                AVDD OFF
                >0ms
                cbDelayMilliSeconds(1);
                IOVDD OFF 
                **/
            }
            cbDebugPrint((MAKE_LEVEL(DSI, WARNING), "function %s not implemented. use default command list\n", FUNCTION_NAME));
        }
        Status = CBIOS_OK;
    }
    else
    {
        Status = CBIOS_ER_NULLPOINTER;
    }
    
 
    return Status;
}

CBIOS_STATUS cbDSIPanel_DisplayOnOff(PCBIOS_VOID pvcbe, CBIOS_BOOL bTurnOn, PCBIOS_DSI_PANEL_DESC pPanelDesc)
{
    CBIOS_STATUS Status = CBIOS_OK;
    if (bTurnOn)
    {
        cbDSI_SendCmdList(pvcbe, pPanelDesc->pDisplayOnCmdList, pPanelDesc->DisplayOnCmdListSize);
    }
    else
    { 
        cbDSI_SendCmdList(pvcbe, pPanelDesc->pDisplayOffCmdList, pPanelDesc->DisplayOffCmdListSize);
    }

    return Status;
}

CBIOS_STATUS cbDSIPanel_SetBacklight(PCBIOS_VOID pvcbe, CBIOS_U32 BacklightValue, PCBIOS_DSI_PANEL_DESC pPanelDesc)
{
    PCBIOS_DSI_CONFIG pDSIConfig = CBIOS_NULL;
    CBIOS_STATUS Status = CBIOS_OK;
    CBIOS_U8 BlValue = 0;
    PCBIOS_U8 pBlDataBuf = CBIOS_NULL;

    if (pPanelDesc != CBIOS_NULL)
    {
        
        if (pPanelDesc->pFnDSIPanelSetBacklight!= CBIOS_NULL)
        {
            Status = pPanelDesc->pFnDSIPanelSetBacklight(pvcbe, BacklightValue);
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DSI, WARNING), "function %s not implemented. use default command list\n", FUNCTION_NAME));
            BlValue = (CBIOS_U8)BacklightValue;
            pDSIConfig = &(pPanelDesc->DSIConfig);
            if (BacklightValue < pDSIConfig->BacklightMin)
            {
                BlValue = (CBIOS_U8)pDSIConfig->BacklightMin;
            }
            if (BacklightValue > pDSIConfig->BacklightMax)
            {
                BlValue = (CBIOS_U8)pDSIConfig->BacklightMax;
            }

            BlValue = (CBIOS_U8)BlValue * (pDSIConfig->BacklightMax - pDSIConfig->BacklightMin + 1) / 256 + pDSIConfig->BacklightMin;

            pBlDataBuf = pPanelDesc->pBacklightCmdList[0].pDataBuf;
            pBlDataBuf[1] = BlValue;
            
            Status = cbDSI_SendCmdList(pvcbe, pPanelDesc->pBacklightCmdList, pPanelDesc->BacklightCmdListSize);

        }
    }
    else
    {
        Status = CBIOS_ER_NULLPOINTER;
    }

    return Status;
}

CBIOS_STATUS cbDSIPanel_GetBacklight(PCBIOS_VOID pvcbe, CBIOS_U32 *pBacklightValue, PCBIOS_DSI_PANEL_DESC pPanelDesc)
{
    CBIOS_STATUS Status = CBIOS_OK;

    if (pPanelDesc != CBIOS_NULL)
    {
        
        if (pPanelDesc->pFnDSIPanelGetBacklight!= CBIOS_NULL)
        {
            Status = pPanelDesc->pFnDSIPanelGetBacklight(pvcbe, pBacklightValue);
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DSI, WARNING), "function %s not implemented. \n", FUNCTION_NAME));
                      
            *pBacklightValue = 0;
        }
    }
    else
    {
        Status = CBIOS_ER_NULLPOINTER;
    }


    return Status;
}


CBIOS_STATUS cbDSIPanel_SetCabc(PCBIOS_VOID pvcbe, CBIOS_U32 CabcValue, PCBIOS_DSI_PANEL_DESC pPanelDesc)
{
    CBIOS_STATUS Status = CBIOS_OK;
    
    if (pPanelDesc != CBIOS_NULL)
    {
            
        if (pPanelDesc->pFnDSIPanelSetCABC!= CBIOS_NULL)
        {
            Status = pPanelDesc->pFnDSIPanelSetCABC(pvcbe, CabcValue);
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DSI, WARNING), "function %s not implemented. \n", FUNCTION_NAME));
        }
    }
    else
    {
       Status = CBIOS_ER_NULLPOINTER;
    }

    return Status;
}




