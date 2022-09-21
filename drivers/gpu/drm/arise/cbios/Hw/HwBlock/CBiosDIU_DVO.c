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
** DVO hw block interface function implementation.
**
** NOTE:
**
******************************************************************************/

#include "CBiosDIU_DVO.h"
#include "CBiosChipShare.h"
#include "../CBiosHwShare.h"

static DVO_CARD_ID_PARA DVODaughterCardIDTable[] = 
{
    {TX_VT1636,  0x02, 0x45},
    {TX_CH7301C, 0x4A, 0x95},
    {TX_VT1632,  0x02, 0x92},
    {TX_AD9389B, 0x43, 0x7E},
    {TX_CH7305,  0x4A, 0x81},
};

static CBIOS_VOID cbDIU_DVO_GetCardIDPara(TX_TYPE CardType, PCBIOS_U8 pCardIDOffset, PCBIOS_U8 pCardIDValue)
{
    CBIOS_U32 i;

    for (i = 0; i < MAX_DVO_DEVICES_NUM; i++)
    {
        if (CardType == DVODaughterCardIDTable[i].DVOCardType)
        {
            *pCardIDOffset = DVODaughterCardIDTable[i].DVOCardIDOffset;
            *pCardIDValue  = DVODaughterCardIDTable[i].DVOCardIDValue;
            break;
        }
    }

    if ( i == MAX_DVO_DEVICES_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "%s: can't fine daughter card ID para!!\n", FUNCTION_NAME));
    }
}

CBIOS_VOID cbDIU_DVO_SetHVSync(PCBIOS_VOID pvcbe, CBIOS_U8 HVPolarity)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U8 byTemp = 0;
    REG_SR2D RegSR2DMask;

    // set polarity for DVO
    if (HVPolarity & HorNEGATIVE)
    {
        byTemp |= 0x40;

    }
    if (HVPolarity & VerNEGATIVE)
    {
        byTemp |= 0x80;
    }
    RegSR2DMask.Value = 0xFF;
    RegSR2DMask.Invert_DVO1_HSYNC = 0;
    RegSR2DMask.Invert_DVO1_VSYNC = 0;
    cbMMIOWriteReg(pcbe, SR_2D, byTemp, RegSR2DMask.Value);
}

CBIOS_VOID cbDIU_DVO_VideoOnOff(PCBIOS_VOID pvcbe, CBIOS_BOOL bOn, CBIOS_U8 IGAIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_SR2B                RegSR2BValue, RegSR2BMask;
    REG_SR32                RegSR32Value, RegSR32Mask;

    if (bOn)
    {
        RegSR2BValue.Value = 0;
        RegSR2BMask.Value = 0xFF;
        
        if (IGAIndex == IGA1)
        {
            RegSR2BValue.DVO1_Source_Select = 0;
            RegSR2BMask.DVO1_Source_Select = 0;
        }
        else if (IGAIndex == IGA2)
        {
            RegSR2BValue.DVO1_Source_Select = 1;
            RegSR2BMask.DVO1_Source_Select = 0;
        }
        else if (IGAIndex == IGA3)
        {
            RegSR2BValue.DVO1_Source_Select = 2;
            RegSR2BMask.DVO1_Source_Select = 0;
        }
        cbMMIOWriteReg(pcbe,SR_2B, RegSR2BValue.Value, RegSR2BMask.Value);
        
        //Set to DVO1 only. enable pad
        //DVO and UART share the same port
        //if turn on DVO, we should disable UART
        //otherwise will cause TDR
        RegSR32Value.Value = 0;
        RegSR32Value.DVO_1_DAT_Pad_Out = 1;
        RegSR32Value.DVO_1_HSYNC_VSYNC_Pad_Out = 1;
        RegSR32Value.DVO_1_DEN_CLK_Pad_Out = 1;
        RegSR32Value.DVO_1_CLKB_Pad_Out = 1;
        RegSR32Mask.Value = 0xE0;
        cbMMIOWriteReg(pcbe,SR_32, RegSR32Value.Value, RegSR32Mask.Value);
    }
    else
    {
        RegSR2BValue.Value = 0;
        RegSR2BValue.DVO1_Source_Select = 0;
        RegSR2BValue.Reserved_7to2 = 0;
        RegSR2BMask.Value = 0xF0;
        cbMMIOWriteReg(pcbe,SR_2B, RegSR2BValue.Value, RegSR2BMask.Value);

        RegSR32Value.Value = 0;
        RegSR32Value.DVO_1_DAT_Pad_Out = 0;
        RegSR32Value.DVO_1_HSYNC_VSYNC_Pad_Out = 0;
        RegSR32Value.DVO_1_DEN_CLK_Pad_Out = 0;
        RegSR32Value.DVO_1_CLKB_Pad_Out = 0;
        RegSR32Mask.Value = 0xE0;
        cbMMIOWriteReg(pcbe,SR_32, RegSR32Value.Value, RegSR32Mask.Value);
    }
}

CBIOS_STATUS cbDIU_DVO_CheckDaughterCardType(PCBIOS_VOID pvcbe, PVCP_INFO pVCP, PCBIOS_VOID pvDvoContext)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DVO_CONTEXT pDvoContext = (PCBIOS_DVO_CONTEXT)pvDvoContext;
    CBIOS_U8             i      =  0;
    CBIOS_U8             Result = 0;
    CBIOS_U8             CardIDOffset = 0;
    CBIOS_U8             CardIDValue  = 0;
    TX_TYPE              DVOTxType = TX_NONE;
    CBIOS_BOOL           status     = CBIOS_TRUE;
    CBIOS_MODULE_I2C_PARAMS  I2CParams;

    cb_memset(&I2CParams, 0, sizeof(CBIOS_MODULE_I2C_PARAMS));

    for(i = 0; i < MAX_DVO_DEVICES_NUM; i++)
    {
        if (pVCP->SupportedDVODevice & pVCP->DVODevConfig[i].DVOTxType)     
        {
            cbDIU_DVO_GetCardIDPara(pVCP->DVODevConfig[i].DVOTxType, &CardIDOffset, &CardIDValue);
            
            I2CParams.ConfigType = CONFIG_I2C_BY_BUSNUM;
            I2CParams.I2CBusNum = (CBIOS_U8)pDvoContext->Common.I2CBus;
            I2CParams.SlaveAddress = pVCP->DVODevConfig[i].DVOSlaveAddr;
            I2CParams.BufferLen = 1;
            I2CParams.OffSet = CardIDOffset;
            I2CParams.Buffer = &Result;
            status = cbI2CModule_ReadData(pcbe, &I2CParams);
            if(status && (Result == CardIDValue))
            {
                DVOTxType = pVCP->DVODevConfig[i].DVOTxType;
                pDvoContext->Common.SupportMonitorType = cbGetSupportMonitorType(pcbe, CBIOS_TYPE_DVO);
                pDvoContext->DVODevice.DVOTxType = pVCP->DVODevConfig[i].DVOTxType;
                pDvoContext->DVODevice.DVOSlaveAddr = pVCP->DVODevConfig[i].DVOSlaveAddr;
                break;
            }
        }
    }

    if(DVOTxType == TX_NONE)
    {
        pcbe->DeviceMgr.SupportDevices &= ~CBIOS_TYPE_DVO;
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: none daughter card detected!!\n", FUNCTION_NAME));
    }

    return  CBIOS_OK;
}
