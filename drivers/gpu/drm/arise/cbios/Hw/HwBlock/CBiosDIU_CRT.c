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
    //{CR_B,(CBIOS_U8)~0x01,0xFC, 0x00 },     //Turn on DCLK1
    {SR,(CBIOS_U8)~0x01,0x0B, 0x00 },       //Turn on DCLK2
    {SR,(CBIOS_U8)~0x20,0x18, 0x20 },       //Turn on CRT Dac1
    {SR,(CBIOS_U8)~0x02,0x21, 0x02 },       //Turn on CRT Dac1 Sense power
    {SR,(CBIOS_U8)~0x02,0x20, 0x00 },       //CRT DAC not off in Standby mode
    {SR,(CBIOS_U8)~0x4C,0x31, 0x44 },       //DAC1 Sense Data Source Select
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
    GPIO_REGISTER Gpio21Value;

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

        if (pcbe->ChipID == CHIPID_ARISE2030)
        {
            // Set GPIO21 for Blue pin
            Gpio21Value.Value = 0;
            Gpio21Value.GPIO_OE = 1;
            if (HVPolarity & VerNEGATIVE)
            {
                Gpio21Value.GPIO_OUT = 1; // if VerNEGATIVE, output = 1
            }
            else
            {
                Gpio21Value.GPIO_OUT = 0; // if VerPOSITIVE, output = 0
            }
            cb_WriteU16(pcbe->pAdapterContext, 0xA005C, Gpio21Value.Value); // GPIO21 = 0xA0058, swap 58 --> 5C
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

typedef struct _CBIOS_DAC_SENSE_PARA
{
    CBIOS_IN  CBIOS_U8  PowerState;
    CBIOS_IN  CBIOS_U8  PrevEdidValid;
    CBIOS_OUT  CBIOS_U8  Connected;           // if don't need dac sense, need return connect status
    CBIOS_OUT  CBIOS_U8  UseNewSense;     // if need dac sense, need return sense path(new or old)
}CBIOS_DAC_SENSE_PARA;

static CBIOS_BOOL  cbIsNeedDacSense(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_DAC_SENSE_PARA *pSensePara)
{
    CBIOS_BOOL  bNeedSense = CBIOS_FALSE;
    CBIOS_BOOL  bRetConnected = CBIOS_FALSE;
    CBIOS_BOOL  bUseNewSense = CBIOS_FALSE;
    REG_SR31_Pair           RegSR31Value;

    if(!pcbe || !pSensePara)
    {
        return CBIOS_FALSE;
    }

    pSensePara->Connected = 0;
    pSensePara->UseNewSense = 0;

    if(pcbe->ChipID != CHIPID_ARISE2030 && pcbe->ChipID != CHIPID_ARISE2020)
    {
        pSensePara->UseNewSense = (pSensePara->PowerState == CBIOS_PM_ON)? 1 : 0;
        return CBIOS_TRUE;
    }

    //patch for arise2030, check CRT's power and connect status
    if(pSensePara->PowerState == CBIOS_PM_ON && (pcbe->DeviceMgr.ConnectedDevices & CBIOS_TYPE_CRT))
    {
        //CRT is connected and turned on, check its binded IGA
        RegSR31Value.Value = cbMMIOReadReg(pcbe, SR_31);
        if(RegSR31Value.DAC1_Source_Select == 0x2)          //source is IGA3
        {
            //It's binded to IGA3, can't do dac sense, check whether previous EDID is valid
            if(pSensePara->PrevEdidValid)
            {
                //previous EDID is valid, and can't read current EDID, it's edid-monitor plug out case
                bNeedSense = CBIOS_FALSE;
                bRetConnected = CBIOS_FALSE;
            }
            else
            {
                //can't detect plug out of non-EDID-monitor
                bNeedSense = CBIOS_FALSE;
                bRetConnected = CBIOS_TRUE;
            }
        }
        else
        {
            //not bind to IGA3, do normal new dac sense
            bNeedSense = CBIOS_TRUE;
            bUseNewSense = CBIOS_TRUE;
        }
    }
    else
    {
        //not connected or not turned on, can switch to IGA1/IGA2 to do old dac sense
        bNeedSense = CBIOS_TRUE;
        bUseNewSense = CBIOS_FALSE;
    }

    if(bNeedSense)
    {
        pSensePara->UseNewSense = (bUseNewSense)? 1 : 0;
    }
    else
    {
        pSensePara->Connected = (bRetConnected)? 1 : 0;
    }

    return bNeedSense;
}

CBIOS_BOOL cbArise2030_DACSense(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32               IGAIndex = pDevCommon->DispSource.ModuleList.IGAModule.Index;
    CBIOS_BOOL              bStatus = CBIOS_FALSE;
    GPIO_REGISTER           Gpio21Value, Gpio22Value;

    if(pDevCommon->PowerState != CBIOS_PM_ON)
    {
        // Set GPIO21
        Gpio21Value.Value = 0;
        Gpio21Value.GPIO_OE = 1;
        Gpio21Value.GPIO_OUT = 0;
        cb_WriteU16(pcbe->pAdapterContext, 0xA005C, Gpio21Value.Value); // GPIO21 = 0xA0058, swap 58 --> 5C
        cb_DelayMicroSeconds(200);//delay 200us
        Gpio21Value.Value = 0;
        Gpio21Value.GPIO_OE = 1;
        Gpio21Value.GPIO_OUT = 1;
        cb_WriteU16(pcbe->pAdapterContext, 0xA005C, Gpio21Value.Value);  //swap 58 --> 5C
        cb_DelayMicroSeconds(200);//delay 200us
        Gpio21Value.Value = 0;
        Gpio21Value.GPIO_OE = 1;
        Gpio21Value.GPIO_OUT = 0;
        cb_WriteU16(pcbe->pAdapterContext, 0xA005C, Gpio21Value.Value);  //swap 58 --> 5C

        // Set GPIO22 Input Enable
        Gpio22Value.Value = 0;
        Gpio22Value.GPIO_IE = 1;
        cb_WriteU16(pcbe->pAdapterContext, 0xA0058, Gpio22Value.Value); // GPIO22 = 0xA005C, swap 5C --> 58
        cb_DelayMicroSeconds(200);//delay 200us

        // Read GPIO22 Input Value
        Gpio22Value.Value = cb_ReadU16(pcbe->pAdapterContext, 0xA0058); // GPIO22 = 0xA005C, swap 5C --> 58
        if(Gpio22Value.GPIO_Data_In == 0) // 0: connect; 1: not connect
        {
            bStatus = CBIOS_TRUE;
        }
    }
    else
    {
        // Set GPIO22 Input Enable
        Gpio22Value.Value = 0;
        Gpio22Value.GPIO_IE = 1;
        cb_WriteU16(pcbe->pAdapterContext, 0xA0058, Gpio22Value.Value); // GPIO22 = 0xA005C, swap 5C --> 58
        cbWaitVSync(pcbe, (CBIOS_U8)IGAIndex);

        // Read GPIO22 Input Value
        Gpio22Value.Value = cb_ReadU16(pcbe->pAdapterContext, 0xA0058); // GPIO22 = 0xA005C, swap 5C --> 58
        if(Gpio22Value.GPIO_Data_In == 0) // 0: connect; 1: not connect
        {
            bStatus = CBIOS_TRUE;
        }
    }
    return bStatus;
}
CBIOS_BOOL cbDIU_CRT_DACSense(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_BOOL  bPrevEdidValid)
{
    PCBIOS_EXTENSION_COMMON pcbe       = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32               IGAIndex = pDevCommon->DispSource.ModuleList.IGAModule.Index;
    CBIOS_BOOL            bStatus = CBIOS_FALSE;
    CBIOS_U8                by3C2;
    REG_SR21                RegSR21Value, RegSR21Mask;
    REG_SR31_Pair           RegSR31Value, RegSR31Mask;
    REG_SR3F                RegSR3FValue, RegSR3FMask;
    REG_CR71_Pair           RegCR71Value, RegCR71Mask;
    CBIOS_DAC_SENSE_PARA  DacSensePara = {0};

    if (pcbe->ChipID == CHIPID_ARISE2030)
    {
        return cbArise2030_DACSense(pcbe, pDevCommon);
    }

    DacSensePara.PowerState = pDevCommon->PowerState;
    DacSensePara.PrevEdidValid = (bPrevEdidValid)? 1 : 0;

    if(!cbIsNeedDacSense(pcbe, &DacSensePara))
    {
        return  (DacSensePara.Connected)? CBIOS_TRUE : CBIOS_FALSE;
    }

    if(DacSensePara.UseNewSense)
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

        cbMMIOWriteReg(pcbe,SR_4B, 0x7A, 0x00);   //R sense
        cbMMIOWriteReg(pcbe,SR_4C, 0x7A, 0x00);   // G sense
        cbMMIOWriteReg(pcbe,SR_4D, 0x7A, 0x00);  // B sense

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
        //Use old DAC1 sense logic when CRT is off. Use IGA2 to sense
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

        cbMMIOWriteReg(pcbe,SR_4B, 0x7A, 0x00);
        cbMMIOWriteReg(pcbe,SR_4C, 0x7A, 0x00);
        cbMMIOWriteReg(pcbe,SR_4D, 0x7A, 0x00);

        cb_DelayMicroSeconds(570);
    }

    // Read result
    by3C2 = cb_ReadU8(pcbe->pAdapterContext, CB_INPUT_STATUS_0_REG);
    if((by3C2 & 0x70) != 0x70)
    {
        bStatus = CBIOS_TRUE;
    }

    // Restore register
    if(DacSensePara.UseNewSense)
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


