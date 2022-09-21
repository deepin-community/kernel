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
** CBios interrupt service routine functions implementation.
**
** NOTE:
** The print, delay and mutex lock SHOULD NOT be called in isr functions.
******************************************************************************/

#include "CBiosChipShare.h"
#include "../CBiosHwShare.h"
#include "../Register/BIU_SBI_registers.h"
#include "../HwBlock/CBiosDIU_DP.h"

CBIOS_STATUS cbGetInterruptInfo(PCBIOS_VOID pvcbe, PCBIOS_INTERRUPT_INFO pIntInfo)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8574 RegMM8574Value;
    REG_MM8578 RegMM8578Value;

    /* NOTE 1: any write to MM8574, both MM8574 & MM8578 will copied from MM8504 & MM8548 and MM8504 & MM8548 are cleared */
    /* NOTE 2: For register 8574 do not use mask write func cbMMIOWriteReg32, one more read may have side effect for this ISR register*/
    cb_WriteU32(pcbe->pAdapterContext, 0x8574, 0);

    RegMM8574Value.Value = cb_ReadU32(pcbe->pAdapterContext, 0x8574);
    RegMM8578Value.Value = cb_ReadU32(pcbe->pAdapterContext, 0x8578);

    pIntInfo->InterruptType = 0;
    // MM8504
    if (RegMM8574Value.VSYNC1_INT)
    {
        pIntInfo->InterruptType |= CBIOS_VSYNC_1_INT;
    }

    if (RegMM8574Value.VSYNC2_INT)
    {
        pIntInfo->InterruptType |= CBIOS_VSYNC_2_INT;
    }

    if (RegMM8574Value.VSYNC3_INT)
    {
        pIntInfo->InterruptType |= CBIOS_VSYNC_3_INT;
    }

    if (RegMM8574Value.VSYNC4_INT)
    {
        pIntInfo->InterruptType |= CBIOS_VSYNC_4_INT;
    }

    if (RegMM8574Value.DP1_INT)
    {
        pIntInfo->InterruptType |= CBIOS_DP_1_INT;
    }

    if (RegMM8574Value.DP2_INT)
    {
        pIntInfo->InterruptType |= CBIOS_DP_2_INT;
    }

    if (RegMM8574Value.DP3_INT)
    {
        pIntInfo->InterruptType |= CBIOS_DP_3_INT;
    }

    if (RegMM8574Value.DP4_INT)
    {
        pIntInfo->InterruptType |= CBIOS_DP_4_INT;
    }

    if (RegMM8574Value.HDCP_INT)
    {
        pIntInfo->InterruptType |= CBIOS_HDCP_INT;
    }

    if (RegMM8574Value.HDA_AUDIO_INT)
    {
        pIntInfo->InterruptType |= CBIOS_HDA_AUDIO_INT;
    }

    if (RegMM8574Value.HDA_CODEC_INT)
    {
        pIntInfo->InterruptType |= CBIOS_HDA_CODEC_INT;
    }

    if (RegMM8574Value.CORRECTABLE_ERR_INT)
    {
        pIntInfo->InterruptType |= CBIOS_CORRECTABLE_ERR_INT;
    }
    
    if (RegMM8574Value.NON_FATAL_ERR_INT)
    {
        pIntInfo->InterruptType |= CBIOS_NON_FATAL_ERR_INT;
    }
    
    if (RegMM8574Value.FATAL_ERR_INT)
    {
        pIntInfo->InterruptType |= CBIOS_FATAL_ERR_INT;
    }

    if (RegMM8574Value.UNSUPPORTED_ERR_INT)
    {
        pIntInfo->InterruptType |= CBIOS_UNSUPPORTED_ERR_INT;
    }

    pIntInfo->AdvancedIntType = 0;
    // MM8548
    if (RegMM8578Value.FENCE_CMD_INT)
    {
        pIntInfo->AdvancedIntType |= CBIOS_FENCE_INT;
    }
    
    return CBIOS_OK;
}


CBIOS_STATUS cbGetCECInterruptInfo(PCBIOS_VOID pvcbe, PCBIOS_CEC_INTERRUPT_INFO pCECIntInfo)
{
    CBIOS_STATUS    Status = CBIOS_OK;
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CEC_MISC_REG2   CECMiscReg2;

    if (pCECIntInfo == CBIOS_NULL)
    {
        Status = CBIOS_ER_NULLPOINTER;
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "cbGetCECInterruptInfo: pCECIntInfo is NULL!"));
    }
    else if (!pcbe->ChipCaps.IsSupportCEC)
    {
        Status = CBIOS_ER_HARDWARE_LIMITATION;
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "cbGetCECInterruptInfo: Can't support CEC!"));
    }
    else if (!(pCECIntInfo->InterruptBitMask & BIT6))
    {
        Status = CBIOS_OK;
        pCECIntInfo->InterruptType = INVALID_CEC_INTERRUPT;
    }
    else
    {
        //set to invalid interrupt by default
        pCECIntInfo->InterruptType = INVALID_CEC_INTERRUPT;

        //first check CEC1
        CECMiscReg2.CECMiscReg2Value = cb_ReadU32(pcbe->pAdapterContext, 0x33150);

        if (CECMiscReg2.FolReceiveReady)
        {
            pCECIntInfo->CEC1MsgReceived = 1;
            pCECIntInfo->InterruptType = NORMAL_CEC_INTERRUPT;
        }
        else
        {
            pCECIntInfo->CEC1MsgReceived = 0;
        }


        //CEC2
        CECMiscReg2.CECMiscReg2Value = cb_ReadU32(pcbe->pAdapterContext, 0x33e3c);

        if (CECMiscReg2.FolReceiveReady)
        {
            pCECIntInfo->CEC2MsgReceived = 1;
            pCECIntInfo->InterruptType = NORMAL_CEC_INTERRUPT;
        }
        else
        {
            pCECIntInfo->CEC2MsgReceived = 0;
        }

        //CEC3
        CECMiscReg2.CECMiscReg2Value = cb_ReadU32(pcbe->pAdapterContext, 0x34540);

        if (CECMiscReg2.FolReceiveReady)
        {
            pCECIntInfo->CEC3MsgReceived = 1;
            pCECIntInfo->InterruptType = NORMAL_CEC_INTERRUPT;
        }
        else
        {
            pCECIntInfo->CEC3MsgReceived = 0;
        }

        //CEC4
        CECMiscReg2.CECMiscReg2Value = cb_ReadU32(pcbe->pAdapterContext, 0x34c40);

        if (CECMiscReg2.FolReceiveReady)
        {
            pCECIntInfo->CEC4MsgReceived = 1;
            pCECIntInfo->InterruptType = NORMAL_CEC_INTERRUPT;
        }
        else
        {
            pCECIntInfo->CEC4MsgReceived = 0;
        }

        Status = CBIOS_OK;

    }
    return Status;
}

CBIOS_STATUS cbGetHDCPInterruptInfo(PCBIOS_VOID pvcbe, PCBIOS_HDCP_INFO_PARA pHdcpInfoParam)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS Status = CBIOS_OK;
    REG_MM82C8  RegMM82C8Value, RegMM33C70Value, RegMM34370Value, RegMM34A70Value;
    REG_MM8374  RegMM8374Value, RegMM33C78Value, RegMM34378Value, RegMM34A78Value;
    REG_MM3368C RegMM3368CValue, RegMM33C88Value, RegMM34388Value, RegMM34A88Value;

    if(pHdcpInfoParam == CBIOS_NULL)
    {
        Status = CBIOS_ER_NULLPOINTER;
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: pHdcpInfoParam is NULL!", FUNCTION_NAME));
    }
    else if(pHdcpInfoParam->InterruptType != CBIOS_HDCP_INT)
    {
        Status = CBIOS_ER_INVALID_PARAMETER;

        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: InterruptType invalid!", FUNCTION_NAME));
    }
    else
    {
        pHdcpInfoParam->IntDevicesId = CBIOS_TYPE_NONE;
        RegMM82C8Value.Value = cb_ReadU32(pcbe->pAdapterContext, 0x82C8);
        RegMM33C70Value.Value = cb_ReadU32(pcbe->pAdapterContext, 0x33C70);
        RegMM34370Value.Value = cb_ReadU32(pcbe->pAdapterContext, 0x34370);
        RegMM34A70Value.Value = cb_ReadU32(pcbe->pAdapterContext, 0x34A70);

        if(RegMM82C8Value.HDCP1_Interrupt)
        {
            pHdcpInfoParam->IntDevicesId |= CBIOS_TYPE_DP1;
            RegMM82C8Value.HDCP1_Interrupt = 0;
            cb_WriteU32(pcbe->pAdapterContext, 0x82C8, RegMM82C8Value.Value);
        }

        if(RegMM33C70Value.HDCP1_Interrupt)
        {
            pHdcpInfoParam->IntDevicesId |= CBIOS_TYPE_DP2;
            RegMM33C70Value.HDCP1_Interrupt = 0;
            cb_WriteU32(pcbe->pAdapterContext, 0x33C70, RegMM33C70Value.Value);
        }

        if(RegMM34370Value.HDCP1_Interrupt)
        {
            pHdcpInfoParam->IntDevicesId |= CBIOS_TYPE_DP3;
            RegMM34370Value.HDCP1_Interrupt = 0;
            cb_WriteU32(pcbe->pAdapterContext, 0x34370, RegMM34370Value.Value);
        }

        if(RegMM34A70Value.HDCP1_Interrupt)
        {
            pHdcpInfoParam->IntDevicesId |= CBIOS_TYPE_DP4;
            RegMM34A70Value.HDCP1_Interrupt = 0;
            cb_WriteU32(pcbe->pAdapterContext, 0x34A70, RegMM34A70Value.Value);
        }

        RegMM8374Value.Value = cb_ReadU32(pcbe->pAdapterContext, 0x8374);
        RegMM33C78Value.Value = cb_ReadU32(pcbe->pAdapterContext, 0x33C78);
        RegMM34378Value.Value = cb_ReadU32(pcbe->pAdapterContext, 0x34378);
        RegMM34A78Value.Value = cb_ReadU32(pcbe->pAdapterContext, 0x34A78);
        
        if(RegMM8374Value.DP_HDCP_INT)
        {
            pHdcpInfoParam->IntDevicesId |= CBIOS_TYPE_DP1;
            RegMM8374Value.DP_HDCP_INT = 0;
            cb_WriteU32(pcbe->pAdapterContext, 0x8374, RegMM8374Value.Value);
        }

        if(RegMM33C78Value.DP_HDCP_INT)
        {
            pHdcpInfoParam->IntDevicesId |= CBIOS_TYPE_DP2;
            RegMM33C78Value.DP_HDCP_INT = 0;
            cb_WriteU32(pcbe->pAdapterContext, 0x33C78, RegMM33C78Value.Value);
        }
        
        if(RegMM34378Value.DP_HDCP_INT)
        {
            pHdcpInfoParam->IntDevicesId |= CBIOS_TYPE_DP3;
            RegMM34378Value.DP_HDCP_INT = 0;
            cb_WriteU32(pcbe->pAdapterContext, 0x34378, RegMM34378Value.Value);
        }
        
        if(RegMM34A78Value.DP_HDCP_INT)
        {
            pHdcpInfoParam->IntDevicesId |= CBIOS_TYPE_DP4;
            RegMM34A78Value.DP_HDCP_INT = 0;
            cb_WriteU32(pcbe->pAdapterContext, 0x34A78, RegMM34A78Value.Value);
        }

        RegMM3368CValue.Value = cb_ReadU32(pcbe->pAdapterContext, 0x368C);
        RegMM33C88Value.Value = cb_ReadU32(pcbe->pAdapterContext, 0x33C88);
        RegMM34388Value.Value = cb_ReadU32(pcbe->pAdapterContext, 0x34388);
        RegMM34A88Value.Value = cb_ReadU32(pcbe->pAdapterContext, 0x34A88);

        if(RegMM3368CValue.Value != 0)
        {
            pHdcpInfoParam->IntDevicesId |= CBIOS_TYPE_DP1;
        }

        if(RegMM33C88Value.Value != 0)
        {
            pHdcpInfoParam->IntDevicesId |= CBIOS_TYPE_DP2;
        }

        if(RegMM34388Value.Value != 0)
        {
            pHdcpInfoParam->IntDevicesId |= CBIOS_TYPE_DP3;
        }

        if(RegMM34A88Value.Value != 0)
        {
            pHdcpInfoParam->IntDevicesId |= CBIOS_TYPE_DP4;
        }
    }

    return Status;
}

CBIOS_STATUS cbGetHDACInterruptInfo(PCBIOS_VOID pvcbe, PCBIOS_HDAC_INFO_PARA pHdacInfoParam)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS Status = CBIOS_OK;
    REG_MM8288 RegMM8288Value;

    if(pHdacInfoParam == CBIOS_NULL)
    {
        Status = CBIOS_ER_NULLPOINTER;
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: pHdacInfoParam is NULL!", FUNCTION_NAME));
    }
    else if(pHdacInfoParam->InterruptType != CBIOS_HDA_CODEC_INT)
    {
        Status = CBIOS_ER_INVALID_PARAMETER;

        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: InterruptType invalid!", FUNCTION_NAME));
    }
    else
    {
        pHdacInfoParam->IntDevicesId = CBIOS_TYPE_NONE;
        RegMM8288Value.Value = cb_ReadU32(pcbe->pAdapterContext, 0x8288);

        if(RegMM8288Value.Int_Src_Codec1)
        {
            pHdacInfoParam->IntDevicesId |= CBIOS_TYPE_DP1;
            RegMM8288Value.Int_Src_Codec1 = 0;
            cb_WriteU32(pcbe->pAdapterContext, 0x8288, RegMM8288Value.Value);
        }

        if(RegMM8288Value.Int_Src_Codec2)
        {
            pHdacInfoParam->IntDevicesId |= CBIOS_TYPE_DP2;
            RegMM8288Value.Int_Src_Codec2 = 0;
            cb_WriteU32(pcbe->pAdapterContext, 0x8288, RegMM8288Value.Value);
        }   
    }

    return Status;
}