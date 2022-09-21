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
** CRT hw block interface function implementation.
**
** NOTE:
**
******************************************************************************/

#include "CBiosDIU_CRT.h"
#include "CBiosChipShare.h"
#include "../CBiosHwShare.h"

static CBREGISTER NewCRTDetectEnv[] = { 
//notes: CBE has reserved 128 Byte array:pcbe->SavedReg[128]
    //Per Ping, DAC phy need clock to trigger DATA into DAC, 
    //thus we should  enable clock before setting sense data.
    {CR_B,(CBIOS_U8)~0x01,0xFC, 0x00 },     //Turn on DCLK1
    {SR,(CBIOS_U8)~0x01,0x0B, 0x00 },       //Turn on DCLK2
    {SR,(CBIOS_U8)~0x20,0x18, 0x20 },       //Turn on CRT Dac1
    {SR,(CBIOS_U8)~0x02,0x21, 0x02 },       //Turn on CRT Dac1 Sense power
    {SR,(CBIOS_U8)~0x02,0x20, 0x00 },       //CRT DAC not off in Standby mode 
    {SR,(CBIOS_U8)~0x40,0x31, 0x40 },       //DAC1 Sense Data Source Select
    {SR, 0x00,0x4B, 0x94 },        // R sense data
    {SR, 0x00,0x4C, 0x94 },        // G sense data
    {SR, 0x00,0x4D, 0x94 },        // B sense data
};

CBIOS_VOID cbDIU_CRT_SetDacCsc(PCBIOS_VOID  pvcbe, CSC_FORMAT  InFmt, CSC_FORMAT  OutFmt)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM33868_Arise  DacCscValue = {0};

    DacCscValue.Dac_Csc_In_Format = InFmt;
    DacCscValue.Dac_Csc_Out_Format = OutFmt;

    cbMMIOWriteReg32(pcbe, 0x3868, DacCscValue.Value, 0);
}

CBIOS_VOID cbDIU_CRT_SetHVSync(PCBIOS_VOID pvcbe, CBIOS_U8 HVPolarity, CBIOS_U8 IGAIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_ACTIVE_TYPE DevicePort = 0; 
    CBIOS_U8 byTemp = 0;

    DevicePort = pcbe->DispMgr.ActiveDevices[IGAIndex];

    if (DevicePort & (0x1 | 0x40))
    {
        byTemp = 0;
        
        if (HVPolarity & HorNEGATIVE)
        {
            byTemp |= 0x40;
            
        }
        if (HVPolarity & VerNEGATIVE)
        {
            byTemp |= 0x80;
        }

        if(IGAIndex == IGA1)
        {
            cb_WriteU8(pcbe->pAdapterContext,CB_MISC_OUTPUT_WRITE_REG,
                (cb_ReadU8(pcbe->pAdapterContext,CB_MISC_OUTPUT_READ_REG)& ~0xC0) | byTemp);
        }
        else if(IGAIndex == IGA2)
        {  
            cbMMIOWriteReg(pcbe, CR_42, byTemp, 0x3F);
        }
        else if(IGAIndex == IGA3)
        {
            cbMMIOWriteReg(pcbe, CR_43, (byTemp >> 5), 0xF9);
        }
        else if(IGAIndex == IGA4)
        {
            cbMMIOWriteReg(pcbe, CR_55, (byTemp >> 6), 0xFC);
        }
    }
}

CBIOS_VOID cbDIU_CRT_DACOnOff(PCBIOS_VOID pvcbe, CBIOS_BOOL bOn, CBIOS_U8 IGAIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_SR18      RegSR18Value, RegSR18Mask;
    REG_SR27      RegSR27Value, RegSR27Mask;
    REG_SR31_Pair RegSR31Value, RegSR31Mask;

    RegSR18Value.Value = 0;
    RegSR18Mask.Value = 0xFF;
    RegSR27Value.Value = 0;
    RegSR27Mask.Value = 0xFF;
    RegSR31Value.Value = 0;
    RegSR31Mask.Value = 0xFF;
    
    if(bOn)
    {
        RegSR27Value.DAC1_Gain_Adjust = 0x03;
        RegSR27Mask.DAC1_Gain_Adjust = 0;
        cbMMIOWriteReg(pcbe, SR_27, RegSR27Value.Value, RegSR27Mask.Value);

        //select DAC source
        if(IGAIndex == IGA1)
        {
            RegSR31Value.DAC1_Source_Select = 0;
        }
        else if(IGAIndex == IGA2)
        {
            RegSR31Value.DAC1_Source_Select = 1;
        }
        else if(IGAIndex == IGA3)
        {
            RegSR31Value.DAC1_Source_Select = 2;
        }
        else if(IGAIndex == IGA4)
        {
            RegSR31Value.DAC1_Source_Select = 3;
        }
        RegSR31Mask.DAC1_Source_Select = 0;
        cbMMIOWriteReg(pcbe, SR_31, RegSR31Value.Value, RegSR31Mask.Value);

        //power up DAC
        RegSR18Value.DAC1_Power_Up = 1;
        RegSR18Mask.DAC1_Power_Up = 0;
        cbMMIOWriteReg(pcbe, SR_18, RegSR18Value.Value, RegSR18Mask.Value);
        cbMMIOWriteReg(pcbe, SR_32, 0x40, ~0x40);
    }
    else
    {
        //power down DAC
        RegSR18Value.DAC1_Power_Up = 0;
        RegSR18Mask.DAC1_Power_Up = 0;
        cbMMIOWriteReg(pcbe, SR_18, RegSR18Value.Value, RegSR18Mask.Value);
    }
}

CBIOS_VOID cbDIU_CRT_SyncOnOff(PCBIOS_VOID pvcbe, CBIOS_BOOL bOn)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_SR0D RegSR0DValue, RegSR0DMask;

    if(bOn) 
    {
         RegSR0DValue.Value = 0;
         RegSR0DValue.CRT1_HSYNC = 0;
         RegSR0DValue.CRT1_VSYNC = 0;
         RegSR0DMask.Value = 0xFF;
         RegSR0DMask.CRT1_HSYNC = 0;
         RegSR0DMask.CRT1_VSYNC = 0;
         cbMMIOWriteReg(pcbe,SR_0D, RegSR0DValue.Value, RegSR0DMask.Value);
    }
    else
    {
         RegSR0DValue.Value = 0;
         RegSR0DValue.CRT1_HSYNC = 1;
         RegSR0DValue.CRT1_VSYNC = 1;
         RegSR0DMask.Value = 0xFF;
         RegSR0DMask.CRT1_HSYNC = 0;
         RegSR0DMask.CRT1_VSYNC = 0;
         cbMMIOWriteReg(pcbe,SR_0D, RegSR0DValue.Value, RegSR0DMask.Value);
    }
}

CBIOS_BOOL cbDIU_CRT_DACSense(PCBIOS_VOID pvcbe, PCBIOS_CRT_MONITOR_CONTEXT pCrtMonitorContext)
{
    PCBIOS_EXTENSION_COMMON pcbe       = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DEVICE_COMMON    pDevCommon = pCrtMonitorContext->pDevCommon;
    CBIOS_U32               IGAIndex = pDevCommon->DispSource.ModuleList.IGAModule.Index;
    CBIOS_U32               bStatus = CBIOS_FALSE;
    CBIOS_U8                by3C2;
    REG_SR21                RegSR21Value;
    REG_SR21                RegSR21Mask;
    REG_SR31_Pair           RegSR31Value;
    REG_SR31_Pair           RegSR31Mask;
    REG_SR3F                RegSR3FValue;
    REG_SR3F                RegSR3FMask;
    REG_CR71_Pair           RegCR71Value;
    REG_CR71_Pair           RegCR71Mask;
    REG_SR4B                RegSR4BValue;
    REG_SR4B                RegSR4BMask;
    REG_SR4C                RegSR4CValue;
    REG_SR4C                RegSR4CMask;
    REG_SR4D                RegSR4DValue;
    REG_SR4D                RegSR4DMask;

    if(pDevCommon->PowerState == CBIOS_PM_ON)
    {
        //Use new DAC1 sense logic when CRT is on.
        RegSR21Value.Value = 0;
        RegSR21Value.DAC1_SENSE_Power_Down_Enable = 0;
        RegSR21Mask.Value = 0xFF;
        RegSR21Mask.DAC1_SENSE_Power_Down_Enable = 0;
        cbMMIOWriteReg(pcbe,SR_21, RegSR21Value.Value, RegSR21Mask.Value);
        
        RegSR31Value.Value = 0;
        RegSR31Value.DAC1_SENSE_Source = 0;
        RegSR31Mask.Value = 0xFF;
        RegSR31Mask.DAC1_SENSE_Source = 0;
        cbMMIOWriteReg(pcbe,SR_31, RegSR31Value.Value, RegSR31Mask.Value);
        
        RegSR3FValue.Value = 0;
        RegSR3FValue.SENSE_Mode = 0;
        RegSR3FMask.Value = 0xFF;
        RegSR3FMask.SENSE_Mode = 0;
        cbMMIOWriteReg(pcbe,SR_3F, RegSR3FValue.Value, RegSR3FMask.Value);
        
        RegCR71Value.Value = 0;
        RegCR71Value.DLYSEL = 1;
        RegCR71Value.SENSEL = 0;
        RegCR71Value.SENWIDTH = 3;
        RegCR71Mask.Value = 0xFF;
        RegCR71Mask.DLYSEL = 0;
        RegCR71Mask.SENSEL = 0;
        RegCR71Mask.SENWIDTH = 0;
        cbMMIOWriteReg(pcbe,CR_71, RegCR71Value.Value, RegCR71Mask.Value);
        
        RegSR4BValue.Value = 0;
        RegSR4BValue.R_SENSE = 0x7A;
        RegSR4BMask.Value = 0xFF;
        RegSR4BMask.R_SENSE = 0;
        cbMMIOWriteReg(pcbe,SR_4B, RegSR4BValue.Value, RegSR4BMask.Value);
        RegSR4CValue.Value = 0;
        RegSR4CValue.G_SENSE = 0x7A;
        RegSR4CMask.Value = 0xFF;
        RegSR4CMask.G_SENSE = 0;
        cbMMIOWriteReg(pcbe,SR_4C, RegSR4CValue.Value, RegSR4CMask.Value);
        RegSR4DValue.Value = 0;
        RegSR4DValue.B_SENSE = 0x7A;
        RegSR4DMask.Value = 0xFF;
        RegSR4DMask.B_SENSE = 0;
        cbMMIOWriteReg(pcbe,SR_4D, RegSR4DValue.Value, RegSR4DMask.Value);
        
        RegSR3FValue.Value = 0;
        RegSR3FValue.B_Sense_1to0 = 3;
        RegSR3FValue.G_Sense_1to0 = 3;
        RegSR3FValue.R_Sense_1to0 = 3;
        RegSR3FMask.Value = 0xFF;
        RegSR3FMask.B_Sense_1to0 = 0;
        RegSR3FMask.G_Sense_1to0 = 0;
        RegSR3FMask.R_Sense_1to0 = 0;
        cbMMIOWriteReg(pcbe,SR_3F, RegSR3FValue.Value, RegSR3FMask.Value);
        
        RegSR21Value.Value = 0;
        RegSR21Value.SEN_SEL = 0;
        RegSR21Mask.Value = 0xFF;
        RegSR21Mask.SEN_SEL = 0;
        cbMMIOWriteReg(pcbe,SR_21, RegSR21Value.Value, RegSR21Mask.Value);
        
        RegSR31Value.Value = 0;
        RegSR31Value.DAC1_SENSE_Source = 1;
        RegSR31Mask.Value = 0xFF;
        RegSR31Mask.DAC1_SENSE_Source = 0;
        cbMMIOWriteReg(pcbe,SR_31, RegSR31Value.Value, RegSR31Mask.Value);
        
        RegSR21Value.Value = 0;
        RegSR21Value.DAC1_SENSE_Power_Down_Enable = 1;
        RegSR21Mask.Value = 0xFF;
        RegSR21Mask.DAC1_SENSE_Power_Down_Enable = 0;
        cbMMIOWriteReg(pcbe,SR_21, RegSR21Value.Value, RegSR21Mask.Value);

        if (IGAIndex != CBIOS_MODULE_INDEX_INVALID)
        {
            cbWaitVSync(pcbe, (CBIOS_U8)IGAIndex);
        }
    }
    else
    {
        //Use old DAC1 sense logic when CRT is off.
        cbSaveRegTableU8(pcbe, NewCRTDetectEnv, sizeofarray(NewCRTDetectEnv), pcbe->SavedReg);
        
        RegSR21Value.Value = 0;
        RegSR21Value.SEN_SEL = 0;
        RegSR21Mask.Value = 0xFF;
        RegSR21Mask.SEN_SEL = 0;
        cbMMIOWriteReg(pcbe,SR_21, RegSR21Value.Value, RegSR21Mask.Value);
        RegSR3FValue.Value = 0;
        RegSR3FValue.SENSE_Mode = 1;
        RegSR3FMask.Value = 0xFF;
        RegSR3FMask.SENSE_Mode = 0;
        cbMMIOWriteReg(pcbe,SR_3F, RegSR3FValue.Value, RegSR3FMask.Value);
        RegSR4BValue.Value = 0;
        RegSR4BValue.R_SENSE = 0x7A;
        RegSR4BMask.Value = 0xFF;
        RegSR4BMask.R_SENSE = 0;
        cbMMIOWriteReg(pcbe,SR_4B, RegSR4BValue.Value, RegSR4BMask.Value);
        RegSR4CValue.Value = 0;
        RegSR4CValue.G_SENSE = 0x7A;
        RegSR4CMask.Value = 0xFF;
        RegSR4CMask.G_SENSE = 0;
        cbMMIOWriteReg(pcbe,SR_4C, RegSR4CValue.Value, RegSR4CMask.Value);
        RegSR4DValue.Value = 0;
        RegSR4DValue.B_SENSE = 0x7A;
        RegSR4DMask.Value = 0xFF;
        RegSR4DMask.B_SENSE = 0;
        cbMMIOWriteReg(pcbe,SR_4D, RegSR4DValue.Value, RegSR4DMask.Value);

        cb_DelayMicroSeconds(570);
    }

    // Read result
    by3C2 = cb_ReadU8(pcbe->pAdapterContext, CB_INPUT_STATUS_0_REG);
    if((by3C2 & 0x70) != 0x70)
    {
        bStatus = CBIOS_TRUE;
    }

    // Restore register
    if(pDevCommon->PowerState == CBIOS_PM_ON)
    {
        //Use new DAC1 sense logic when CRT is on.
        RegSR21Value.Value = 0;
        RegSR21Value.DAC1_SENSE_Power_Down_Enable = 0;
        RegSR21Mask.Value = 0xFF;
        RegSR21Mask.DAC1_SENSE_Power_Down_Enable = 0;
        cbMMIOWriteReg(pcbe,SR_21, RegSR21Value.Value, RegSR21Mask.Value);
        RegSR31Value.Value = 0;
        RegSR31Value.DAC1_SENSE_Source = 0;
        RegSR31Mask.Value= 0xFF;
        RegSR31Mask.DAC1_SENSE_Source = 0;
        cbMMIOWriteReg(pcbe,SR_31, RegSR31Value.Value, RegSR31Mask.Value);
    }
    else
    {
        //Use old DAC1 sense logic when CRT is off.
        cbRestoreRegTableU8(pcbe, NewCRTDetectEnv, sizeofarray(NewCRTDetectEnv), pcbe->SavedReg);
    }

    return bStatus;
}


