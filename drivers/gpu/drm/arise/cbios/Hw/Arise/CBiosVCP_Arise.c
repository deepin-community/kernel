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

#include "CBios_Arise.h"
#include "CBiosVCP_Arise.h"

// these macro start with 'VBIOS_' should consistent with VBIOS code
#define VBIOS_TBL_REGMASK            0x0F
#define VBIOS_TBL_BITMASK            0x40
#define VBIOS_TBL_INITDATA           0x20
#define VBIOS_TBL_TABLINK            0x10
#define VBIOS_CR_REG                 0x0
#define VBIOS_SR_REG                 0x1
#define VBIOS_GR_REG                 0x2
#define VBIOS_MISC_REG               0x3
#define VBIOS_AR_REG                 0x4
#define VBIOS_CR_P_REG               0x5
#define VBIOS_SR_P_REG               0x6
#define VBIOS_CR_B_REG               0x7
#define VBIOS_CR_C_REG               0x8
#define VBIOS_CR_D_REG               0x0B
#define VBIOS_SR_B_REG               VBIOS_SR_P_REG

#define  VCP_CRT_CHAR_BYTE   0x04
#define  VCP_DP1_DUAL_MODE_CHAR_BYTE    0x00
#define  VCP_DP2_DUAL_MODE_CHAR_BYTE    0x01
#define  VCP_DP3_DUAL_MODE_CHAR_BYTE    0x02   // need refine
#define  VCP_DP4_DUAL_MODE_CHAR_BYTE    0x03   // need refine
#define  VCP_DVO_CHAR_BYTE   0x0C

#define  VCP_CRT_CHAR_BYTE_E2   0x00
#define  VCP_DP5_DUAL_MODE_CHAR_BYTE_E2    0x02
#define  VCP_DP6_DUAL_MODE_CHAR_BYTE_E2    0x03
#define  VCP_DVO_CHAR_BYTE_E2   0x0C

static CBIOS_U16               VCP_Length  = 0x100;
static CBIOS_U32               VCP_BiosVersion  = 0x1A110000;
static CBIOS_U8                VCP_Version  = 0x00;
static CBIOS_U16               VCP_SubVendorID = 0x6766;
static CBIOS_U16               VCP_SubSystemID = 0x3D02;
static CBIOS_U32               VCP_SupportDevices  = CBIOS_TYPE_DP1 | CBIOS_TYPE_CRT;
static CBIOS_FEATURE_SWITCH    VCP_FeatureSwitch  = {0};
static CBIOS_U8                VCP_CRTInterruptPort = 0x0F;
static CBIOS_U8                VCP_DVOInterruptPort = 0x0F;
static CBIOS_U8                VCP_GpioForEDP1Power = 0x0F;
static CBIOS_U8                VCP_GpioForEDP1BackLight = 0x0F;
static CBIOS_U8                VCP_GpioForEDP2Power = 0x0F;
static CBIOS_U8                VCP_GpioForEDP2BackLight = 0x0F;
static CBIOS_U32               VCP_EClock = 5000000;
static CBIOS_U32               VCP_EVcoLow = 12000000;
static CBIOS_U32               VCP_DVcoLow = 12000000;
static CBIOS_U8                VCP_SLICE_NUM = 2;

//variable VCP data region start, add variable VCP data below
CBIOS_U16 VCP_BOOT_DEVICES_PRIORITY[]  =
{
    VBIOS_TYPE_DP1,
    VBIOS_TYPE_DP2,
    VBIOS_TYPE_DVO,
    VBIOS_TYPE_CRT,
    0xFFFF,
};

CBREGISTER VCP_CR_DEFAULT_TABLE[]  =
{
    {CR,   (CBIOS_U8)~0xFF, 0x45, 0x00},
    {CR,   (CBIOS_U8)~0xFF, 0x52, 0x00},
    {CR,   (CBIOS_U8)~0xD8, 0x63, 0x00},
    {CR,   (CBIOS_U8)~0x06, 0x67, 0x02},
    {CR,   (CBIOS_U8)~0xFF, 0x6D, 0x33},
    {CR,   (CBIOS_U8)~0xFF, 0x6E, 0x00},
    {CR,   (CBIOS_U8)~0x22, 0x71, 0x22},
    {CR,   (CBIOS_U8)~0xFF, 0x86, 0xA8},
    {CR,   (CBIOS_U8)~0xFF, 0x87, 0x68},
    {CR,   (CBIOS_U8)~0xFF, 0x88, 0x20},
    {CR,   (CBIOS_U8)~0xFF, 0x89, 0xE0},
    {CR,   (CBIOS_U8)~0xFF, 0x8D, 0x68},
    {CR,   (CBIOS_U8)~0xFF, 0xBC, 0x20},
    {CR,   (CBIOS_U8)~0xFF, 0xBD, 0xA8},
    {CR,   (CBIOS_U8)~0xFF, 0xBE, 0x08},
    {CR,   (CBIOS_U8)~0xFF, 0xC0, 0x24},
    {CR_B, (CBIOS_U8)~0x22, 0x71, 0x22},
    {CR_B, (CBIOS_U8)~0xFF, 0x45, 0x00},
    {CR_B, (CBIOS_U8)~0xFF, 0xC0, 0x7F},
    {CR_B, (CBIOS_U8)~0xFF, 0xC1, 0x7F},
    {CR_B, (CBIOS_U8)~0xFF, 0xC4, 0x00},
    {CR_B, (CBIOS_U8)~0xFF, 0xF5, 0x00},
    {CR_B, (CBIOS_U8)~0xFF, 0xF6, 0x00},
    {CR_B, (CBIOS_U8)~0xFF, 0xFC, 0x00},
    {CR_C, (CBIOS_U8)~0xFF, 0x4F, 0xFF},
    {CR_C, (CBIOS_U8)~0x04, 0x53, 0x04},
    {CR_C, (CBIOS_U8)~0xFF, 0x72, 0x20},
    {CR_C, (CBIOS_U8)~0x0F, 0x88, 0x00},
    {CR_C, (CBIOS_U8)~0x04, 0x9D, 0x04},
    {CR_C, (CBIOS_U8)~0x1F, 0xA0, 0x00},
    {CR_C, (CBIOS_U8)~0xC3, 0xA3, 0x00},
    {CR_C, (CBIOS_U8)~0x80, 0xA4, 0x00},
    {CR_C, (CBIOS_U8)~0xF0, 0xA5, 0x00},
    {CR_C, (CBIOS_U8)~0x1E, 0xC2, 0x00},
    {CR_C, (CBIOS_U8)~0x08, 0x70, 0x08},
    {0xFF},
};


CBREGISTER VCP_SR_DEFAULT_TABLE[] =
{
    {SR,   (CBIOS_U8)~0xE0, 0x09, 0x60},
    {SR,   (CBIOS_U8)~0x8D, 0x0B, 0x00},
    {SR,   (CBIOS_U8)~0xF7, 0x0D, 0x50},
    {SR,   (CBIOS_U8)~0xFF, 0x0E, 0xC1},
    {SR,   (CBIOS_U8)~0xFF, 0x0F, 0x6B},
    {SR,   (CBIOS_U8)~0xDF, 0x14, 0x00},
    {SR,   (CBIOS_U8)~0x6E, 0x15, 0x4A},
    {SR,   (CBIOS_U8)~0x20, 0x18, 0x20},
    {SR,   (CBIOS_U8)~0xFF, 0x19, 0x31},
    {SR,   (CBIOS_U8)~0x67, 0x1D, 0x67},
    {SR,   (CBIOS_U8)~0x67, 0x1E, 0x00},
    {SR,   (CBIOS_U8)~0x01, 0x1F, 0x00},
    {SR,   (CBIOS_U8)~0xFF, 0x20, 0x7F},
    {SR,   (CBIOS_U8)~0xFF, 0x21, 0xCE},
    {SR,   (CBIOS_U8)~0xE7, 0x10, 0x84},
    {SR,   (CBIOS_U8)~0x7F, 0x22, 0x0E},
    {SR,   (CBIOS_U8)~0xFF, 0x23, 0xE8},
    {SR,   (CBIOS_U8)~0xFF, 0x24, 0x10},
    {SR,   (CBIOS_U8)~0xFF, 0x25, 0xC8},
    {SR,   (CBIOS_U8)~0x80, 0x26, 0x00},
    {SR,   (CBIOS_U8)~0xFF, 0x27, 0xB3},
    {SR,   (CBIOS_U8)~0xFF, 0x29, 0x00},
    {SR,   (CBIOS_U8)~0xFF, 0x2B, 0x00},
    {SR,   (CBIOS_U8)~0xD0, 0x2D, 0x00},
    {SR,   (CBIOS_U8)~0x3F, 0x30, 0x01},
    {SR,   (CBIOS_U8)~0x01, 0x32, 0x00},
    {SR,   (CBIOS_U8)~0x80, 0x33, 0x80},
    {SR,   (CBIOS_U8)~0xF0, 0x39, 0x30},
    {SR,   (CBIOS_U8)~0x18, 0x3C, 0x18},
    {SR,   (CBIOS_U8)~0xE4, 0x31, 0x00},
    {SR,   (CBIOS_U8)~0x3F, 0x3F, 0x00},
    {SR,   (CBIOS_U8)~0xFF, 0x42, 0x01},
    {SR,   (CBIOS_U8)~0xFF, 0x43, 0x01},
    {SR,   (CBIOS_U8)~0x88, 0x44, 0x80},
    {SR,   (CBIOS_U8)~0x1B, 0x45, 0x00},
    {SR,   (CBIOS_U8)~0xFF, 0x47, 0x01},
    {0xFF},
};



#ifndef _CBIOS_BUILD_VCP_BIN_FILE

CBIOS_U32 cbGetDefaultRegTblSize(CBREGISTER* pRegTable)
{
    CBIOS_U32 Table_Size = 0, i = 0;
    CBREGISTER* pReg = pRegTable;

    for (i = 0; i < VCP_MAX_REG_TABLE_SIZE ; i++)
    {
        if (pReg->type != 0xFF)
        {
            Table_Size++;
            pReg++;
        }
        else
        {
            break;
        }
    }

    return Table_Size;
}

CBIOS_U32 cbGetBootDevPrioritySize(PCBIOS_U16 pVCPBootDevPriority)
{
    CBIOS_U32 Size = 0;
    while((*pVCPBootDevPriority) != 0xFFFF)
    {
        Size ++;
        pVCPBootDevPriority ++;

        if (Size >= 16)
            break;
    }
    return Size;
}


//----------------------------------------------------------------------//
// CBIOS_U32 cbGetVCPRegTbl_dst( )
//
// Convert BIOS VCP ROM image to our register table
// 1) Get the number of register lenght
// 2) Copy the register value over to our reg table
//
//-----------------------------------------------------------------------//
CBIOS_U32 cbGetRomRegTbl(PCBIOS_VOID VCPBase, CBIOS_U16 TblOffset, CBREGISTER** pRegTable)
{
    CBIOS_U8* pVCPTblBase;
    CBIOS_U8* pBYTE;
    CBIOS_U8* pHead;
    CBIOS_U32 RegCounter = 0;
    CBIOS_U32 ItemLength = 0;
    CBIOS_U32 i,Count;
    CBIOS_U8  regtype = 0;
    CBREGISTER* pReg;

    pVCPTblBase = (PCBIOS_U8)VCPBase + TblOffset - VCP_OFFSET;

    // 1.scan table to get memory size.
    pBYTE = pVCPTblBase;
    do{
        // BIOS table format
        // flags in head(REGTYPE + BITMASK + INITDATA + TABLINK)
        pHead = pBYTE;

        ItemLength = 1;        //always has index
        if(*pHead & VBIOS_TBL_BITMASK)   //include Mask
            ItemLength++;
        if(*pHead & VBIOS_TBL_INITDATA)  //include value
            ItemLength++;

        // The next member will indicate the # of registers
        // number of regs in this table
        Count = *(pHead + 1);
        RegCounter += Count;   //total regs

        //point to next table
        pBYTE += 1 + 1 + ItemLength*Count; //head+count+regs
    }while(*pHead & VBIOS_TBL_TABLINK);

    // 2. Allocate memory
    if(RegCounter == 0)
        return RegCounter;
    pReg = *pRegTable = cb_AllocateNonpagedPool(RegCounter * sizeof(CBREGISTER));
    if(pReg == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"pRegTable allocate error!\n"));
        return 0;
    }

    // 3. Fill reg table
    pBYTE = pVCPTblBase;

    do{
        pHead = pBYTE;

        switch(*pHead & VBIOS_TBL_REGMASK)
        {
        case VBIOS_CR_REG:
            regtype = CR;
            break;
        case VBIOS_SR_REG:
            regtype = SR;
            break;
        case VBIOS_GR_REG:
            regtype = GR;
            break;
        case VBIOS_MISC_REG:
            regtype = MISC;
            break;
        case VBIOS_AR_REG:
            regtype = AR;
            break;
        case VBIOS_CR_P_REG:
        case VBIOS_CR_B_REG:
            regtype = CR_B;
            break;
        case VBIOS_SR_B_REG:
            regtype = SR_B;
            break;
        case VBIOS_CR_C_REG:
            regtype = CR_C;
            break;
        case VBIOS_CR_D_REG:
            regtype = CR_D;
            break;
        }

        pBYTE++;

        Count = *(pBYTE++);

        for(i = 0;i<Count;i++)
        {
            pReg->type = regtype;

            pReg->index = *(pBYTE++);

            if(*pHead & VBIOS_TBL_BITMASK)
                pReg->mask = ~(*(pBYTE++));
            else
                pReg->mask = 0x00;

            if(*pHead & VBIOS_TBL_INITDATA)
                pReg->value = *(pBYTE++);
            else
                pReg->value = 0x00;

            pReg++;
        }

    }while(*pHead & VBIOS_TBL_TABLINK);

    return RegCounter;
}

CBIOS_VOID cbLoadDefaultVCPData(PVCP_INFO pVCP, CBIOS_U32 rom_len)
{
    pVCP->VCPlength = VCP_Length;
    pVCP->BiosVersion = VCP_BiosVersion;
    pVCP->Version = VCP_Version;
    pVCP->SubVendorID = VCP_SubVendorID;
    pVCP->SubSystemID = VCP_SubSystemID;
    pVCP->SupportDevices = VCP_SupportDevices;

    cb_memcpy(&pVCP->FeatureSwitch, &VCP_FeatureSwitch, sizeof(CBIOS_FEATURE_SWITCH));

    pVCP->CRTCharByte = (CBIOS_U8)VCP_CRT_CHAR_BYTE;
    pVCP->DP1DualModeCharByte = (CBIOS_U8)VCP_DP1_DUAL_MODE_CHAR_BYTE;
    pVCP->DP2DualModeCharByte = (CBIOS_U8)VCP_DP2_DUAL_MODE_CHAR_BYTE;
    pVCP->DP3DualModeCharByte = (CBIOS_U8)VCP_DP3_DUAL_MODE_CHAR_BYTE;
    pVCP->DP4DualModeCharByte = (CBIOS_U8)VCP_DP4_DUAL_MODE_CHAR_BYTE;
    pVCP->DVOCharByte = (CBIOS_U8)VCP_DVO_CHAR_BYTE;

    pVCP->CRTInterruptPort = VCP_CRTInterruptPort;
    pVCP->DVOInterruptPort = VCP_DVOInterruptPort;
    pVCP->GpioForEDP1Power = VCP_GpioForEDP1Power;
    pVCP->GpioForEDP1BackLight = VCP_GpioForEDP1BackLight;
    pVCP->GpioForEDP2Power = VCP_GpioForEDP2Power;
    pVCP->GpioForEDP2BackLight = VCP_GpioForEDP2BackLight;

    pVCP->EClock = VCP_EClock;
    pVCP->EVcoLow = VCP_EVcoLow;
    pVCP->DVcoLow = VCP_DVcoLow;

    pVCP->SliceNum = VCP_SLICE_NUM;

    pVCP->sizeofBootDevPriority = cbGetBootDevPrioritySize(VCP_BOOT_DEVICES_PRIORITY);
    pVCP->sizeofCR_DEFAULT_TABLE = cbGetDefaultRegTblSize(VCP_CR_DEFAULT_TABLE);
    pVCP->sizeofSR_DEFAULT_TABLE = cbGetDefaultRegTblSize(VCP_SR_DEFAULT_TABLE);
    pVCP->pCR_DEFAULT_TABLE = cb_AllocateNonpagedPool(sizeof(VCP_CR_DEFAULT_TABLE));
    pVCP->pSR_DEFAULT_TABLE = cb_AllocateNonpagedPool(sizeof(VCP_SR_DEFAULT_TABLE));
    cb_memcpy(pVCP->pCR_DEFAULT_TABLE, VCP_CR_DEFAULT_TABLE, sizeof(VCP_CR_DEFAULT_TABLE));
    cb_memcpy(pVCP->pSR_DEFAULT_TABLE, VCP_SR_DEFAULT_TABLE, sizeof(VCP_SR_DEFAULT_TABLE));

    //Note: temp fix for arise1020 can not get the rom image
    if (0x100000 == rom_len)
    {
        pVCP->FeatureSwitch.bNonSimulChip = 1;
        pVCP->FeatureSwitch.IsDisableCodec2 = 1;
        pVCP->FeatureSwitch.IsEclkConfigEnable = 1;
    }

}


CBIOS_VOID cbParseVCPInfo(PCBIOS_EXTENSION_COMMON pcbe, PVCP_INFO pVCP, PVCP_INIT_DATA pVCPInitData, PCBIOS_VOID VCPBase, CBIOS_U32 RomLength)
{
    CBIOS_U32  i = 0;
    PCBIOS_U16 pVCP_BootDevPriorityAddr = CBIOS_NULL;
    PCBIOS_U8  pByte = CBIOS_NULL;
    VCP_FEATURESWITCH FeatureSwitch = {0};
    VCP_EDP_POWERCONFIG eDPPowerConfig = {0};

    pVCP->Version = pVCPInitData->VCP_Version;
    pVCP->VCPlength = cb_swab16(pVCPInitData->VCP_Length);
    pVCP->BiosVersion = cb_swab32(pVCPInitData->VCP_BiosVersion);
    pVCP->SubVendorID = cb_swab16(pVCPInitData->VCP_SubVendorID);
    pVCP->SubSystemID = cb_swab16(pVCPInitData->VCP_SubSystemID);
    pVCP->SupportDevices = cb_swab16(pVCPInitData->VCP_SupportDevices);
    pVCP->SupportDevices = cbConvertVBiosDevBit2CBiosDevBit(pVCP->SupportDevices);

    // feature switch
    FeatureSwitch = cb_swab32(pVCPInitData->VCP_FeatureSwitch);
    pVCP->FeatureSwitch.IsEDP1Enabled = FeatureSwitch.IsEDP1Enabled;
    pVCP->FeatureSwitch.IsEDP2Enabled = FeatureSwitch.IsEDP2Enabled;
    pVCP->FeatureSwitch.IsDisableCodec1 = FeatureSwitch.IsDisableCodec1;
    pVCP->FeatureSwitch.IsDisableCodec2 = FeatureSwitch.IsDisableCodec2;
    pVCP->FeatureSwitch.HPDActiveHighEnable = FeatureSwitch.HPDActiveHighEnable;
    pVCP->FeatureSwitch.BIUBackdoorEnable = FeatureSwitch.IsBIUBackdoorEnable;
    pVCP->FeatureSwitch.bNonSimulChip = FeatureSwitch.NonSimulateChip;

    eDPPowerConfig = cb_swab32(pVCPInitData->VCP_EDPPowerConfig);
    pVCP->GpioForEDP1Power = eDPPowerConfig.eDP1PowerCtrlGPIO;
    pVCP->GpioForEDP2Power = eDPPowerConfig.eDP2PowerCtrlGPIO;
    pVCP->GpioForEDP1BackLight = eDPPowerConfig.eDP1BackLightGPIO;
    pVCP->GpioForEDP2BackLight = eDPPowerConfig.eDP2BackLightGPIO;

    pVCP->CRTCharByte = (CBIOS_U8)(pVCPInitData->VCP_PortConfig.DACCharByte);
    if (pVCP->CRTCharByte == 0x0F)
    {
        pVCP->CRTCharByte = I2CBUS_INVALID;
    }

    pVCP->DP1DualModeCharByte = (CBIOS_U8)(pVCPInitData->VCP_PortConfig.DP1DualModeCharByte);
    if (pVCP->DP1DualModeCharByte == 0x0F)
    {
        pVCP->DP1DualModeCharByte = I2CBUS_INVALID;
    }

    pVCP->DP2DualModeCharByte = (CBIOS_U8)(pVCPInitData->VCP_PortConfig.DP2DualModeCharByte);
    if (pVCP->DP2DualModeCharByte == 0x0F)
    {
        pVCP->DP2DualModeCharByte = I2CBUS_INVALID;
    }

    pVCP->DP3DualModeCharByte = (CBIOS_U8)(pVCPInitData->VCP_PortConfig.DP3DualModeCharByte);
    if (pVCP->DP3DualModeCharByte == 0x0F)
    {
        pVCP->DP3DualModeCharByte = I2CBUS_INVALID;
    }

    pVCP->DP4DualModeCharByte = (CBIOS_U8)(pVCPInitData->VCP_PortConfig.DP4DualModeCharByte);
    if (pVCP->DP4DualModeCharByte == 0x0F)
    {
        pVCP->DP4DualModeCharByte = I2CBUS_INVALID;
    }

    pVCP->DVOCharByte = (CBIOS_U8)(pVCPInitData->VCP_PortConfig.DVOCharByte);
    if (pVCP->DVOCharByte == 0x0F)
    {
        pVCP->DVOCharByte = I2CBUS_INVALID;
    }

    pVCP->CRTInterruptPort = (CBIOS_U8)(pVCPInitData->VCP_PortConfig.DACGPIOByte);
    if (pVCP->CRTInterruptPort == 0x0F)
    {
        pVCP->CRTInterruptPort = INVALID_GPIO;
    }

    pVCP->DVOInterruptPort = (CBIOS_U8)(pVCPInitData->VCP_PortConfig.DVOGPIOByte);
    if (pVCP->DVOInterruptPort == 0x0F)
    {
        pVCP->DVOInterruptPort = INVALID_GPIO;
    }

    pVCP->EClock = cb_swab32(pVCPInitData->VCP_ChipClock.EClock);
    pVCP->EVcoLow = cb_swab32(pVCPInitData->VCP_ChipClock.EVcoLow);
    pVCP->DVcoLow = cb_swab32(pVCPInitData->VCP_ChipClock.DVcoLow);

    if (RomLength < VBIOS_IMAGE_SIZE)
    {
        pVCP->sizeofCR_DEFAULT_TABLE = cbGetDefaultRegTblSize(VCP_CR_DEFAULT_TABLE);
        pVCP->sizeofSR_DEFAULT_TABLE = cbGetDefaultRegTblSize(VCP_SR_DEFAULT_TABLE);
        pVCP->pCR_DEFAULT_TABLE = cb_AllocateNonpagedPool(sizeof(VCP_CR_DEFAULT_TABLE));
        pVCP->pSR_DEFAULT_TABLE = cb_AllocateNonpagedPool(sizeof(VCP_SR_DEFAULT_TABLE));
        cb_memcpy(pVCP->pCR_DEFAULT_TABLE, VCP_CR_DEFAULT_TABLE, sizeof(VCP_CR_DEFAULT_TABLE));
        cb_memcpy(pVCP->pSR_DEFAULT_TABLE, VCP_SR_DEFAULT_TABLE, sizeof(VCP_SR_DEFAULT_TABLE));
    }
    else
    {
        pVCP->sizeofCR_DEFAULT_TABLE = cbGetRomRegTbl(VCPBase, cb_swab16(pVCPInitData->VCP_CRDefaultTableOffset), &pVCP->pCR_DEFAULT_TABLE);
        pVCP->sizeofSR_DEFAULT_TABLE = cbGetRomRegTbl(VCPBase, cb_swab16(pVCPInitData->VCP_SRDefaultTableOffset), &pVCP->pSR_DEFAULT_TABLE);

        pVCP_BootDevPriorityAddr = (PCBIOS_U16)((PCBIOS_U8)VCPBase + cb_swab16(pVCPInitData->VCP_BootDevPriorityOffset) - VCP_OFFSET);
        pVCP->sizeofBootDevPriority = cb_min(cbGetBootDevPrioritySize(pVCP_BootDevPriorityAddr), 16);
        for(i = 0; i < (pVCP->sizeofBootDevPriority); i++)
        {
            pVCP->BootDevPriority[i] = cb_swab16(pVCP_BootDevPriorityAddr[i]);
        }

        if (pVCPInitData->VCP_DVOConfig.DVODeviceType.ActVT1636)
        {
            pVCP->SupportedDVODevice |= TX_VT1636;
        }
        if (pVCPInitData->VCP_DVOConfig.DVODeviceType.ActCH7301C)
        {
            pVCP->SupportedDVODevice |= TX_CH7301C;
        }
        if (pVCPInitData->VCP_DVOConfig.DVODeviceType.ActVT1632A)
        {
            pVCP->SupportedDVODevice |= TX_VT1632;
        }
        if (pVCPInitData->VCP_DVOConfig.DVODeviceType.ActAD9389B)
        {
            pVCP->SupportedDVODevice |= TX_AD9389B;
        }
        if (pVCPInitData->VCP_DVOConfig.DVODeviceType.ActCH7305)
        {
            pVCP->SupportedDVODevice |= TX_CH7305;
        }

        //Get all DVO Device Slave Address from VCP
        cb_memset(pVCP->DVODevConfig, 0, MAX_DVO_DEVICES_NUM * sizeof(CBIOS_DVO_DEV_CONFIG));
        pByte = (PCBIOS_U8)(VCPBase) + pVCPInitData->VCP_DVODevConfigOffset;
        for(i = 0; i < MAX_DVO_DEVICES_NUM; i++)
        {
            switch(*(PCBIOS_U8)(pByte++))
            {
            case 0x01:
                pVCP->DVODevConfig[i].DVOTxType = TX_VT1636;
                break;
            case 0x02:
                pVCP->DVODevConfig[i].DVOTxType = TX_CH7301C;
                break;
            case 0x04:
                pVCP->DVODevConfig[i].DVOTxType = TX_VT1632;
                break;
            case 0x08:
                pVCP->DVODevConfig[i].DVOTxType = TX_AD9389B;
                break;
            case 0x10:
                pVCP->DVODevConfig[i].DVOTxType = TX_CH7305;
                break;
            default:
                pVCP->DVODevConfig[i].DVOTxType = TX_NONE;
            }

            pVCP->DVODevConfig[i].DVOSlaveAddr = *(PCBIOS_U8)(pByte++);
        }
    }

    // Get Max Resolution configs for 4 DP ports from VCP
    if (pVCPInitData->VCP_Version > 0x06)
    {
        for (i = 0; i < 4; i++)
        {
            pVCP->DPMaxResConfig[i].MaxXRes = cb_swab16(pVCPInitData->VCP_DPMaxResConfig[i].MaxXRes);
            pVCP->DPMaxResConfig[i].MaxYRes = cb_swab16(pVCPInitData->VCP_DPMaxResConfig[i].MaxYRes);
            pVCP->DPMaxResConfig[i].MaxRefresh = cb_swab16(pVCPInitData->VCP_DPMaxResConfig[i].MaxRefresh) * 100;
            cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG),"cbParseVCPInfo: Get port %d res config = %d x %d @ %d.\n",
                          i, pVCP->DPMaxResConfig[i].MaxXRes, pVCP->DPMaxResConfig[i].MaxYRes, pVCP->DPMaxResConfig[i].MaxRefresh));
        }

        pVCP->SliceNum = pVCPInitData->VCP_SliceNum;
    }

    if (pVCPInitData->VCP_Version > 0x07)
    {
        pVCP->FwVersion = cb_swab32(pVCPInitData->VCP_FwVersion);
        cb_memset(pVCP->SignOnMsg, 0, sizeofarray(pVCP->SignOnMsg));
        cb_memcpy(pVCP->SignOnMsg, pVCPInitData->VCP_SignOnMsg, sizeofarray(pVCPInitData->VCP_SignOnMsg));
        cb_memset(pVCP->VBiosEditTime, 0, sizeofarray(pVCP->VBiosEditTime));
        cb_memcpy(pVCP->VBiosEditTime, pVCPInitData->VCP_VBiosEditTime, sizeofarray(pVCPInitData->VCP_VBiosEditTime));
        cb_memset(pVCP->FwName, 0, sizeofarray(pVCP->FwName));
        cb_memcpy(pVCP->FwName, pVCPInitData->VCP_FwName, sizeofarray(pVCPInitData->VCP_FwName));

        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO),"Firmware version: %08x \n", pVCP->FwVersion));
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO),"Firmware name: %s \n", pVCP->FwName));
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO),"Firmware SignOnMsg: %s \n", pVCP->SignOnMsg));
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO),"Firmware modified time (mm/dd/yy hr:mi): %s \n", pVCP->VBiosEditTime));
    }
}


CBIOS_BOOL cbCalculateCheckSumOfVCP(PVCP_INIT_DATA pVCPInitData)
{
    CBIOS_U32 i = 0;
    CBIOS_U32 result = 0;
    PCBIOS_U32 pDWORD = (PCBIOS_U32)pVCPInitData;
    CBIOS_U32 lenth = *(pDWORD + 2);
    CBIOS_U32 version = *(pDWORD + 1);
    CBIOS_U32 num = lenth/sizeof(CBIOS_U32);
    if(version > 0x0D)
    {
        for(i = 0; i < num; i++)
        {
            result += pDWORD[i];
        }
        if(result == 0)
        {
            return CBIOS_TRUE;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "CheckSum is not correct! \n"));
            return CBIOS_FALSE;
        }
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "no CheckSum, default correct! \n"));
        return CBIOS_TRUE;
    }
}

CBIOS_BOOL cbInitVCP_Arise(PCBIOS_EXTENSION_COMMON pcbe, PVCP_INFO pVCP, PCBIOS_VOID pRomBase)
{
    PVCP_INIT_DATA       pVCPInitData;
    PCBIOS_VOID         pRomVCPBase = (PCBIOS_VOID)((PCBIOS_CHAR)pRomBase + VCP_OFFSET);

    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "vcp_init_data size = %ld\n", sizeof(VCP_INIT_DATA)));
    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "pRomBase addr = %x\n", (PCBIOS_CHAR)(pRomBase)));

    if((pRomBase != CBIOS_NULL) && (cb_swab16(*(CBIOS_U16*)(pRomVCPBase))== VCP_TAG))
    {
        pcbe->bUseDefaultVCP = CBIOS_FALSE;

        //VCP not used by default, need discuss with Talen when it is used.
        pVCP->bUseVCP = CBIOS_TRUE;
        pVCPInitData = (PVCP_INIT_DATA)(pRomVCPBase);
        cbParseVCPInfo(pcbe, pVCP, pVCPInitData, pRomVCPBase, pcbe->RomImageLength);
    }
    else
    {
        // load default VCP data
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "cbInitVCP: Enter! Load default VCP data\n"));
        pcbe->bUseDefaultVCP = CBIOS_TRUE;
        //VCP not used by default, need discuss with Talen when it is used.
        pVCP->bUseVCP = CBIOS_TRUE;
        cbLoadDefaultVCPData(pVCP, pcbe->RomImageLength);
    }

    if(pcbe->bRunOnQT)
    {
        pVCP->SupportDevices = CBIOS_TYPE_CRT;
    }

    if (pVCP->SupportDevices & CBIOS_TYPE_DVO)
    {
        pcbe->HPDDeviceMasks |= CBIOS_TYPE_DVO;
    }

    if (pVCP->SupportDevices & CBIOS_TYPE_DP1)
    {
        pcbe->HPDDeviceMasks |= CBIOS_TYPE_DP1;
    }

    if (pVCP->SupportDevices & CBIOS_TYPE_DP2)
    {
        pcbe->HPDDeviceMasks |= CBIOS_TYPE_DP2;
    }

    if (pVCP->SupportDevices & CBIOS_TYPE_DP3)
    {
        pcbe->HPDDeviceMasks |= CBIOS_TYPE_DP3;
    }

    if (pVCP->SupportDevices & CBIOS_TYPE_DP4)
    {
        pcbe->HPDDeviceMasks |= CBIOS_TYPE_DP4;
    }

    pcbe->bFilterUnsupportedModeFunction = CBIOS_FALSE;

    return CBIOS_TRUE;
}


#else
__attribute__((section(".fakemain"))) CBIOS_VOID FakeMainForBuildVCPFile( )
{
    return;
}
#endif

