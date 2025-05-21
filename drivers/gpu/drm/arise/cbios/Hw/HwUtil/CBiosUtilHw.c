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
#include "../CBiosHwShare.h"
#include "../Register/BIU_SBI_registers.h"

//CLK - start
static CBREGISTER_IDX E3K_DCLK1_Integer_MAP[] = {
    {  SR_12,   0xFF},
    {  MAPMASK_EXIT},
};
static CBREGISTER_IDX E3K_DCLK1_Fraction_MAP[] = {
    {  SR_13,    0xFF},
    {  CR_B_EC,  0xFF},
    {  SR_29,    0x0F},
    {  MAPMASK_EXIT},
};
static CBREGISTER_IDX E3K_DCLK1_R_MAP[] = {
    {  SR_10,  0x0C},
    {  MAPMASK_EXIT},
};
static CBREGISTER_IDX E3K_DCLK1_DIV_MAP[] = {
    {  CR_B_D0,  0xC0},
    {  MAPMASK_EXIT},
};

static CBREGISTER_IDX E3K_DCLK2_Integer_MAP[] = {
    {  SR_0E,    0xFF},
    {  MAPMASK_EXIT},
};
static CBREGISTER_IDX E3K_DCLK2_Fraction_MAP[] = {
    {  SR_0F,    0xFF},
    {  CR_F2,    0xFF},
    {  CR_B_E5,  0x0F},
    {  MAPMASK_EXIT},
};
static CBREGISTER_IDX E3K_DCLK2_R_MAP[] = {
    {  SR_29,    0x30},
    {  MAPMASK_EXIT},
};
static CBREGISTER_IDX E3K_DCLK2_DIV_MAP[] = {
    {  CR_B_D1,  0xC0},
    {  MAPMASK_EXIT},
};

static CBREGISTER_IDX E3K_DCLK3_Integer_MAP[] = {
    {  CR_B5,    0xFF},
    {  MAPMASK_EXIT},
};
static CBREGISTER_IDX E3K_DCLK3_Fraction_MAP[] = {
    {  CR_B6,    0xFF},
    {  CR_B7,    0xFF},
    {  CR_B8,    0x0F},
    {  MAPMASK_EXIT},
};
static CBREGISTER_IDX E3K_DCLK3_R_MAP[] = {
    {  CR_B4,    0x03},
    {  MAPMASK_EXIT},
};
static CBREGISTER_IDX E3K_DCLK3_DIV_MAP[] = {
    {  CR_B_D2,  0xC0},
    {  MAPMASK_EXIT},
};

static CBREGISTER_IDX E3K_DCLK4_Integer_MAP[] = {
    {  CR_D6,    0xFF},
    {  MAPMASK_EXIT},
};
static CBREGISTER_IDX E3K_DCLK4_Fraction_MAP[] = {
    {  CR_D5,    0xFC},
    {  CR_D7,    0xFF},
    {  CR_D8,    0x3F},
    {  MAPMASK_EXIT},
};
static CBREGISTER_IDX E3K_DCLK4_R_MAP[] = {
    {  CR_D5,    0x03},
    {  MAPMASK_EXIT},
};
static CBREGISTER_IDX E3K_DCLK4_DIV_MAP[] = {
    {  CR_D9,    0x03},
    {  MAPMASK_EXIT},
};

static CBREGISTER_IDX E3K_ECLK_Integer_MAP[] = {
    {  CR_DF,    0x7F},
    {  CR_DE,    0x10},
    {  MAPMASK_EXIT},
};
static CBREGISTER_IDX E3K_ECLK_R_MAP[] = {
    {  CR_DD,    0x06},
    {  MAPMASK_EXIT},
};

static CBREGISTER_IDX E3K_VCLK_Integer_MAP[] = {
    {  CR_DC,    0x7F},
    {  CR_DB,    0x10},
    {  MAPMASK_EXIT},
};
static CBREGISTER_IDX E3K_VCLK_R_MAP[] = {
    {  CR_DA,    0x06},
    {  MAPMASK_EXIT},
};
//CLK - end

CBIOS_REGISTER_RANGE CBIOS_CR_REGS[] =
{
    {CBIOS_REGISTER_MMIO, 1, 0x8800, 0x88FF},
    {CBIOS_REGISTER_MMIO, 1, 0x8900, 0x89FF},
    {CBIOS_REGISTER_MMIO, 1, 0x9500, 0x95ff},
};


CBIOS_REGISTER_RANGE CBIOS_SR_REGS[] =
{
    {CBIOS_REGISTER_MMIO, 1, 0x8600, 0x86FF},
    {CBIOS_REGISTER_MMIO, 1, 0x8700, 0x87FF},
    {CBIOS_REGISTER_MMIO, 1, 0x9400, 0x94ff},
};

CBIOS_REGISTER_RANGE CBIOS_HDMI_REGS[] =
{
    {CBIOS_REGISTER_MMIO, 1, 0x873B, 0x873B},

    {CBIOS_REGISTER_MMIO, 4, 0x81A0, 0x81A0},
    {CBIOS_REGISTER_MMIO, 4, 0x8194, 0x8194},
    {CBIOS_REGISTER_MMIO, 4, 0x34A4, 0x34B8},
    {CBIOS_REGISTER_MMIO, 4, 0x8280, 0x8298},

};

CBIOS_REGISTER_RANGE CBIOS_SS1_REGS[] =
{
    {CBIOS_REGISTER_MMIO, 1, 0x8864, 0x8867},

    {CBIOS_REGISTER_MMIO, 4, 0x8184, 0x8184},
    {CBIOS_REGISTER_MMIO, 4, 0x8190, 0x819C},
    {CBIOS_REGISTER_MMIO, 4, 0x81A8, 0x81A8},
    {CBIOS_REGISTER_MMIO, 4, 0x81D0, 0x81D8},
    {CBIOS_REGISTER_MMIO, 4, 0x81E4, 0x81E4},
    {CBIOS_REGISTER_MMIO, 4, 0x81EC, 0x81EC},
    {CBIOS_REGISTER_MMIO, 4, 0x81F8, 0x81FC},
    {CBIOS_REGISTER_MMIO, 4, 0x8278, 0x8278},
    {CBIOS_REGISTER_MMIO, 4, 0x828C, 0x828C},
    {CBIOS_REGISTER_MMIO, 4, 0x32C4, 0x32C4},
    {CBIOS_REGISTER_MMIO, 4, 0x30BC, 0x30BC},

};

CBIOS_REGISTER_RANGE CBIOS_SS2_REGS[] =
{
    {CBIOS_REGISTER_MMIO, 1, 0x8864, 0x8867},

    {CBIOS_REGISTER_MMIO, 4, 0x8188, 0x818C},
    {CBIOS_REGISTER_MMIO, 4, 0x81AC, 0x81AC},
    {CBIOS_REGISTER_MMIO, 4, 0x81BC, 0x81BC},
    {CBIOS_REGISTER_MMIO, 4, 0x81CC, 0x81CC},
    {CBIOS_REGISTER_MMIO, 4, 0x81DC, 0x81DC},
    {CBIOS_REGISTER_MMIO, 4, 0x81E0, 0x81E0},
    {CBIOS_REGISTER_MMIO, 4, 0x81F0, 0x81F4},
    {CBIOS_REGISTER_MMIO, 4, 0x8200, 0x8200},
    {CBIOS_REGISTER_MMIO, 4, 0x8204, 0x8204},
    {CBIOS_REGISTER_MMIO, 4, 0x8208, 0x820C},
    {CBIOS_REGISTER_MMIO, 4, 0x8260, 0x8260},
    {CBIOS_REGISTER_MMIO, 4, 0x827C, 0x827C},
    {CBIOS_REGISTER_MMIO, 4, 0x32CC, 0x32CC},
    {CBIOS_REGISTER_MMIO, 4, 0x30FC, 0x30FC},
};

CBIOS_REGISTER_GROUP CBIOS_REGISTER_TABLE[] =
{
    {"CR",     CBIOS_CR_REGS, sizeofarray(CBIOS_CR_REGS)},
    {"SR",     CBIOS_SR_REGS, sizeofarray(CBIOS_SR_REGS)},
    {"PMU",    CBIOS_NULL, 0},
    {"TIMING", CBIOS_NULL, 0},
    {"DSI0",   CBIOS_NULL, 0},
    {"DSI1",   CBIOS_NULL, 0},
    {"MHL",    CBIOS_NULL, 0},
    {"DP",     CBIOS_NULL, 0},
    {"HDMI",   CBIOS_HDMI_REGS, sizeofarray(CBIOS_HDMI_REGS)},
    {"PS1",    CBIOS_NULL, 0},
    {"PS2",    CBIOS_NULL, 0},
    {"SS1",    CBIOS_SS1_REGS, sizeofarray(CBIOS_SS1_REGS)},
    {"SS2",    CBIOS_SS2_REGS, sizeofarray(CBIOS_SS2_REGS)},
};


CBIOS_VOID cbUnlockSR(PCBIOS_EXTENSION_COMMON pcbe)
{
    REG_SR08    RegSR08Value;
    REG_SR08    RegSR08Mask;
    RegSR08Value.Value = 0;
    RegSR08Value.Unlock_Extended_Sequencer = 0x06;
    RegSR08Mask.Value = 0;
    cbMMIOWriteReg(pcbe, SR_08, RegSR08Value.Value, RegSR08Mask.Value);
}

CBIOS_VOID cbUnlockCR(PCBIOS_EXTENSION_COMMON pcbe)
{
    REG_CR38    RegCR38Value;
    REG_CR38    RegCR38Mask;
    REG_CR39    RegCR39Value;
    REG_CR39    RegCR39Mask;
    REG_CR35_Pair    RegCR35Value;
    REG_CR35_Pair    RegCR35Mask;
    REG_CR11_Pair    RegCR11Value;
    REG_CR11_Pair    RegCR11Mask;
    REG_CR35_Pair    RegCR35_BValue;
    REG_CR35_Pair    RegCR35_BMask;
    REG_CR11_Pair    RegCR11_BValue;
    REG_CR11_Pair    RegCR11_BMask;

    RegCR38Value.Value = 0;
    RegCR38Value.CRTC_Register_Lock_1 = 0x48;
    RegCR38Mask.Value = 0x00;
    cbMMIOWriteReg(pcbe, CR_38, RegCR38Value.Value, RegCR38Mask.Value);

    RegCR39Value.Value = 0;
    RegCR39Value.CRTC_Register_Lock_2 = 0xA5;
    RegCR39Mask.Value = 0x00;
    cbMMIOWriteReg(pcbe, CR_39, RegCR39Value.Value, RegCR39Mask.Value);

    RegCR35Value.Value = 0;
    RegCR35Value.Vertical_Timing_Registers_Lock = 0;
    RegCR35Value.Horizontal_Timing_Registers_Lock = 0;
    RegCR35Value.CR01_3C2h_6_Lock = 0;
    RegCR35Value.CR12_3C2h_7_Lock = 0;
    RegCR35Mask.Value = 0x0F;
    cbMMIOWriteReg(pcbe, CR_35, RegCR35Value.Value, RegCR35Mask.Value);

    RegCR11Value.Value = 0;
    RegCR11Value.Lock_Writes_to_CR00_to_CR07 = 0;
    RegCR11Mask.Value = 0xFF;
    RegCR11Mask.Lock_Writes_to_CR00_to_CR07 = 0;
    cbMMIOWriteReg(pcbe, CR_11, RegCR11Value.Value, RegCR11Mask.Value);

    RegCR35_BValue.Value = 0;
    RegCR35_BValue.Vertical_Timing_Registers_Lock = 0;
    RegCR35_BValue.Horizontal_Timing_Registers_Lock = 0;
    RegCR35_BValue.CR01_3C2h_6_Lock = 0;
    RegCR35_BValue.CR12_3C2h_7_Lock = 0;
    RegCR35_BMask.Value = 0x0F;
    cbMMIOWriteReg(pcbe, CR_B_35, RegCR35_BValue.Value, RegCR35_BMask.Value);

    RegCR11_BValue.Value = 0;
    RegCR11_BValue.Lock_Writes_to_CR00_to_CR07 = 0;
    RegCR11_BMask.Value = 0xFF;
    RegCR11_BMask.Lock_Writes_to_CR00_to_CR07 = 0;
    cbMMIOWriteReg(pcbe, CR_B_11, RegCR11_BValue.Value, RegCR11_BMask.Value);


    cbMMIOWriteReg(pcbe, CR_T_35, RegCR35_BValue.Value, RegCR35_BMask.Value);
    cbMMIOWriteReg(pcbe, CR_T_11, RegCR11_BValue.Value, RegCR11_BMask.Value);

    cbMMIOWriteReg(pcbe, CR_F_35, RegCR35_BValue.Value, RegCR35_BMask.Value);
    cbMMIOWriteReg(pcbe, CR_F_11, RegCR11_BValue.Value, RegCR11_BMask.Value);

}

CBIOS_VOID cbWaitForBlank(PCBIOS_EXTENSION_COMMON pcbe)
{
    if(pcbe->bRunOnQT)
    {
        return;
    }
    else
    {
        CBIOS_S32 i;
        for (i=0;i<65536;i++)
        {
            //3xA,VSYNC (0=display, 1=blank)
//            if((cb_ReadU8(pcbe->pAdapterContext, CB_INPUT_STATUS_1_REG)&0x08)!=0)
            if((cbMMIOReadReg(pcbe, CR_1A)&0x08)!=0)
                break;
        }
    }
}

CBIOS_VOID cbWaitForDisplay(PCBIOS_EXTENSION_COMMON pcbe)
{
    if(pcbe->bRunOnQT)
    {
        return;
    }
    else
    {
        CBIOS_S32 i;
        for (i=0;i<65536;i++)
        {
            //3xA,VSYNC (0=display, 1=blank)
//            if((cb_ReadU8(pcbe->pAdapterContext, CB_INPUT_STATUS_1_REG)&0x08)==0)
            if((cbMMIOReadReg(pcbe, CR_1A)&0x08)==0)
                break;
        }
    }
}

CBIOS_VOID cbWaitForActive(PCBIOS_EXTENSION_COMMON pcbe)
{
    //Wait for the start of blank signal, or we are already in blank.
    cbWaitForBlank(pcbe);
    //Wait for the display signal.
    cbWaitForDisplay(pcbe);
}

CBIOS_VOID cbWaitForInactive(PCBIOS_EXTENSION_COMMON pcbe)
{
    //Wait for the display signal, or we are already in display.
    cbWaitForDisplay(pcbe);
    //Wait for the start of the blank signal.
    cbWaitForBlank(pcbe);
}

CBIOS_U32 cbGetParallelRegIndex(PCBIOS_PARALLEL_REGISTER pRegTbl, CBIOS_U32 RegNameIndex, CBIOS_U32 ModuleIndex)
{
    CBIOS_U32    i = 0;
    CBIOS_U32    ParallelRegIndex = 0xFFFFFFFF;

    while((i < PARALLEL_TABLE_MAX_SIZE) && (pRegTbl[i].RegNameIndex != PARALLEL_TABLE_END_INDEX))
    {
        if(pRegTbl[i].RegNameIndex == RegNameIndex)
        {
            ParallelRegIndex = pRegTbl[RegNameIndex].RegIndex[ModuleIndex];
            break;
        }
        i++;
    }

    return ParallelRegIndex;
}

CBIOS_U8 cbBiosMMIOReadReg(PCBIOS_EXTENSION_COMMON pcbe,
                   CBIOS_U16 type_index,
                   CBIOS_U32 IGAModuleIndex)
{
    CBIOS_U8 type = (CBIOS_U8) ((type_index&0xFF00) >> 8);
    CBIOS_U8 index = (CBIOS_U8) (type_index&0x00FF);
    CBIOS_U8 inType = type;

    if ((IGAModuleIndex == IGA2) || (IGAModuleIndex == CBIOS_MODULE_INDEX2))
    {
        if(type == CR)
        {
            inType = CR_B;
        }
        else if (type == SR)
        {
            inType = SR_B;
        }
    }
    else if ((IGAModuleIndex == IGA3) || (IGAModuleIndex == CBIOS_MODULE_INDEX3))
    {
        if(type == CR)
        {
            inType = CR_T;
        }
        else if(type == SR)
        {
            inType = SR_T;
        }
    }
    else if ((IGAModuleIndex == IGA4) || (IGAModuleIndex == CBIOS_MODULE_INDEX4))
    {
        if(type == CR)
        {
            inType = CR_F;
        }
        else if(type == SR)
        {
            inType = SR_F;
        }
    }

    return cbMMIOReadReg(pcbe, (((CBIOS_U16)inType<<8)|index));
}

CBIOS_VOID cbBiosMMIOWriteReg(PCBIOS_EXTENSION_COMMON pcbe,
                        CBIOS_U16 type_index,
                        CBIOS_U8 value,
                        CBIOS_U8 mask,
                        CBIOS_U32 IGAModuleIndex)
{
    CBIOS_U8 type = (CBIOS_U8) ((type_index&0xFF00) >> 8);
    CBIOS_U8 index = (CBIOS_U8) (type_index&0x00FF);
    CBIOS_U8 inType = type;

    if ((IGAModuleIndex == IGA2) || (IGAModuleIndex == CBIOS_MODULE_INDEX2))
    {
        if(type == CR)
        {
            inType = CR_B;
        }
        else if (type == SR)
        {
            inType = SR_B;
        }
    }
    else if ((IGAModuleIndex == IGA3) || (IGAModuleIndex == CBIOS_MODULE_INDEX3))
    {
        if(type == CR)
        {
            inType = CR_T;
        }
        else if(type == SR)
        {
            inType = SR_T;
        }
    }
    else if((IGAModuleIndex == IGA4) || (IGAModuleIndex == CBIOS_MODULE_INDEX4))
    {
        if(type == CR)
        {
            inType = CR_F;
        }
        else if(type == SR)
        {
            inType = SR_F;
        }
    }
    cbMMIOWriteReg(pcbe, (((CBIOS_U16)inType<<8)|index), value, mask);
}

static CBIOS_U32  cbMMIOGetGroupOffset(CBIOS_U8  type)
{
    CBIOS_U32  MmioOffset = 0;
    switch(type)
    {
    case CR:
        MmioOffset = MMIO_OFFSET_CR_GROUP_A;
        break;
    case CR_B:
        MmioOffset = MMIO_OFFSET_CR_GROUP_B;
        break;
    case CR_C:
        MmioOffset = MMIO_OFFSET_CR_GROUP_C;
        break;
    case CR_D:
        MmioOffset = MMIO_OFFSET_CR_GROUP_D;
        break;
    case CR_D_0:
        MmioOffset = MMIO_OFFSET_CR_GROUP_D0;
        break;
    case CR_D_1:
        MmioOffset = MMIO_OFFSET_CR_GROUP_D1;
        break;
    case CR_D_2:
        MmioOffset = MMIO_OFFSET_CR_GROUP_D2;
        break;
    case CR_D_3:
        MmioOffset = MMIO_OFFSET_CR_GROUP_D3;
        break;
    case CR_T:
        MmioOffset = MMIO_OFFSET_CR_GROUP_T;
        break;
    case CR_F:
        MmioOffset = MMIO_OFFSET_CR_GROUP_F;
        break;
    case SR:
        MmioOffset = MMIO_OFFSET_SR_GROUP_A;
        break;
    case SR_B:
        MmioOffset = MMIO_OFFSET_SR_GROUP_B;
        break;
    case SR_T:
        MmioOffset = MMIO_OFFSET_SR_GROUP_T;
        break;
    case SR_F:
        MmioOffset = MMIO_OFFSET_SR_GROUP_F;
        break;
    default:
        MmioOffset = 0xFFFFFFFF;
        break;
    }

    return MmioOffset;
}

CBIOS_U8 cbMMIOReadReg(PCBIOS_EXTENSION_COMMON pcbe,
                   CBIOS_U16 type_index)
{
    CBIOS_U64  oldIrql = 0;
    CBIOS_U32  mmioAddress;
    CBIOS_U8   type = (CBIOS_U8) ((type_index&0xFF00) >> 8);
    CBIOS_U8   index = (CBIOS_U8) (type_index&0x00FF);
    CBIOS_U8   byRet  = 0;

    if(type == MISC)
    {
        byRet = cb_ReadU8(pcbe->pAdapterContext, CB_MISC_OUTPUT_READ_REG);
    }
    else if(type == AR)
    {
        oldIrql = cb_AcquireSpinLock(pcbe->pSpinLock);

        cb_ReadU8(pcbe->pAdapterContext, CB_ATTR_INITIALIZE_REG);
        cb_WriteU8(pcbe->pAdapterContext, CB_ATTR_ADDR_REG, index);
        byRet = cb_ReadU8(pcbe->pAdapterContext, CB_ATTR_DATA_READ_REG);

        cb_ReleaseSpinLock(pcbe->pSpinLock, oldIrql );
    }
    else if(type == GR)
    {
        oldIrql = cb_AcquireSpinLock(pcbe->pSpinLock);

        cb_WriteU8(pcbe->pAdapterContext, CB_GRAPH_ADDR_REG, index);
        byRet = cb_ReadU8(pcbe->pAdapterContext, CB_GRAPH_DATA_REG);

        cb_ReleaseSpinLock(pcbe->pSpinLock, oldIrql );
    }
    else
    {
        mmioAddress = cbMMIOGetGroupOffset(type);
        if(mmioAddress != 0xFFFFFFFF)
        {
            byRet = cb_ReadU8(pcbe->pAdapterContext, mmioAddress+index);
        }
    }

    return byRet;
}


CBIOS_VOID cbMMIOWriteReg(PCBIOS_EXTENSION_COMMON pcbe,
                        CBIOS_U16 type_index,
                        CBIOS_U8 value,
                        CBIOS_U8 mask)
{
    CBIOS_U64  oldIrql = 0;
    CBIOS_U32  mmioAddress;
    CBIOS_U8   type = (CBIOS_U8) ((type_index&0xFF00) >> 8);
    CBIOS_U8   index = (CBIOS_U8) (type_index&0x00FF);
    CBIOS_U8   byTemp;         //Temp value, also save register's old value

    if((value & mask) != 0)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "**Register mask wrong, index=%x, value=%x, mask=%x**\n", type_index, value, mask));
        return;
    }

    if(mask == 0xFF)
    {
        return;
    }

    if(type == MISC)
    {
        byTemp = cb_ReadU8(pcbe->pAdapterContext, CB_MISC_OUTPUT_READ_REG);
        byTemp = (byTemp & mask) | (value);
        cb_WriteU8(pcbe->pAdapterContext, CB_MISC_OUTPUT_WRITE_REG,byTemp);
    }
    else if(type == AR)
    {
        oldIrql = cb_AcquireSpinLock(pcbe->pSpinLock);

        cb_ReadU8(pcbe->pAdapterContext, CB_ATTR_INITIALIZE_REG);
        cb_WriteU8(pcbe->pAdapterContext, CB_ATTR_ADDR_REG, index);
        byTemp = cb_ReadU8(pcbe->pAdapterContext, CB_ATTR_DATA_READ_REG);
        byTemp = (byTemp & mask) | (value);
        cb_ReadU8(pcbe->pAdapterContext, CB_ATTR_INITIALIZE_REG);
        cb_WriteU8(pcbe->pAdapterContext, CB_ATTR_ADDR_REG, index);
        cb_WriteU8(pcbe->pAdapterContext, CB_ATTR_DATA_WRITE_REG, byTemp);

        cb_ReleaseSpinLock(pcbe->pSpinLock, oldIrql );
    }
    else if(type == GR)
    {
        oldIrql = cb_AcquireSpinLock(pcbe->pSpinLock);

        cb_WriteU8(pcbe->pAdapterContext, CB_GRAPH_ADDR_REG, index);
        byTemp = cb_ReadU8(pcbe->pAdapterContext, CB_GRAPH_DATA_REG);
        byTemp = (byTemp & mask) | (value);
        cb_WriteU8(pcbe->pAdapterContext, CB_GRAPH_DATA_REG, byTemp);

        cb_ReleaseSpinLock(pcbe->pSpinLock, oldIrql );
    }
    else
    {
        mmioAddress = cbMMIOGetGroupOffset(type);
        if(mmioAddress != 0xFFFFFFFF)
        {
            mmioAddress += index;
            byTemp = cb_ReadU8(pcbe->pAdapterContext, mmioAddress);
            byTemp = (byTemp & mask) | (value);
            cb_WriteU8(pcbe->pAdapterContext, mmioAddress, byTemp);
        }
    }
}


CBIOS_VOID cbMMIOWriteReg32(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 Index, CBIOS_U32 Value, CBIOS_U32 Mask)
{
    CBIOS_U32 ulTemp = 0;

    if(Mask == 0xFFFFFFFF)
    {
        return;
    }

    ulTemp = cb_ReadU32(pcbe->pAdapterContext, Index) & Mask;

    Value &= (~Mask);
    Value |= ulTemp;

    cb_WriteU32(pcbe->pAdapterContext, Index, Value);
}


///////////////////////////////////////////////////////////////////////////////
//Read registers value into map masked value
//BIOS func name:MAPMASK_EBX_Read
///////////////////////////////////////////////////////////////////////////////
// Input IGAEncoderIndex = 0: For IGA1.
//                       = 1: For IGA2.
//                       = 2: Reserve
//                       = 3: Reserve
//                       = 4: Reserve
//                       = 5: Reserve
//                       = 6: HDTVEncoder1.
//                       = 7: HDTVEncoder2.
//                       = 8: NonPaired.
CBIOS_U32 cbMapMaskRead(PCBIOS_EXTENSION_COMMON pcbe, CBREGISTER_IDX *regTable, CBIOS_U32 IGAEncoderIndex)
{
    CBIOS_U32 uRet = 0;
    CBIOS_U8 byRegValue = 0;
    CBREGISTER_IDX *reg = regTable;
    CBIOS_U32 i,j=0;

    while(reg->type_index != MAPMASK_EXIT)
    {
        byRegValue = cbBiosMMIOReadReg(pcbe, reg->type_index, IGAEncoderIndex);
        for(i = 0;i<8;i++)
        {
            if( reg->mask & 1<<i )
            {
                uRet += byRegValue&(1<<i)? 1<<j : 0;
                j++;
            }
        }
        reg++;
    }
    return uRet;
}

///////////////////////////////////////////////////////////////////////////////
//Write value into masked registers
//BIOS func name:MAPMASK_EBX_write
///////////////////////////////////////////////////////////////////////////////
// Input IGAEncoderIndex = 0: For IGA1.
//                       = 1: For IGA2.
//                       = 2: Reserve
//                       = 3: Reserve
//                       = 4: Reserve
//                       = 5: Reserve
//                       = 6: HDTVEncoder1.
//                       = 7: HDTVEncoder2.
CBIOS_VOID cbMapMaskWrite(PCBIOS_EXTENSION_COMMON pcbe,CBIOS_U32 Value,CBREGISTER_IDX *regTable, CBIOS_U32 IGAEncoderIndex)
{
    CBIOS_U8 byWritten = 0;
    CBREGISTER_IDX *reg = regTable;
    CBIOS_U32 i,j=0;
    // cbDebugPrint((DBG_LEVEL_DEBUG_MSG,"Enter:cbMapMaskWrite.\n"));
    while(reg->type_index != MAPMASK_EXIT)
    {
        byWritten = 0;
        for(i = 0;i<8;i++)
        {
            if( reg->mask & 1<<i )
            {
                byWritten += Value & (1<<j)? 1<<i: 0;
                j++;
            }
        }
        cbBiosMMIOWriteReg(pcbe, reg->type_index, byWritten, ~(reg->mask), IGAEncoderIndex);
        reg++;
    }
    //  cbDebugPrint((DBG_LEVEL_DEBUG_MSG,"Exit:cbMapMaskWrite.\n"));
}

CBIOS_VOID cbLoadtable(PCBIOS_EXTENSION_COMMON pcbe,
                     CBREGISTER      *pRegTable,
                     CBIOS_U32            Table_Size,
                     CBIOS_U32            IGAEncoderIndex)
{
    CBREGISTER  *pReg;
    CBIOS_U32       i;
    CBIOS_U8        bData;
    CBIOS_U8        index;
    CBIOS_U8        mask;
    CBIOS_U8        type;

    pReg = pRegTable;
    for( i = 0; i < Table_Size && pReg; i++,pReg++)
    {
        index = pReg->index;
        mask =  pReg->mask;
        bData = pReg->value;
        type =  pReg->type;
        type &= ZXG_POST_TYPE_MASK;
        cbBiosMMIOWriteReg(pcbe, (((CBIOS_U16)type<<8)|index), bData, mask,IGAEncoderIndex);
    }
}


CBIOS_VOID cbLoadMemoryTimingTbl(PCBIOS_EXTENSION_COMMON pcbe,
                     MMIOREGISTER*    pRegTable,
                     CBIOS_U32        Table_Size)
{
    MMIOREGISTER*   pReg = CBIOS_NULL;
    CBIOS_U32       i = 0;
    CBIOS_U32       data = 0;
    CBIOS_U32       index = 0;

    pReg = pRegTable;
    for( i = 0; (i < Table_Size) && pReg; i++,pReg++)
    {
        index = pReg->index;
        data = pReg->value;
        if(pReg->length == 32)
        {
            cb_WriteU32(pcbe->pAdapterContext, index, data);
        }
        else if(pReg->length == 16)
        {
            cb_WriteU16(pcbe->pAdapterContext, index, (CBIOS_U16)data);
        }
        else
        {
            cb_WriteU8(pcbe->pAdapterContext, index, (CBIOS_U8)data);
        }
    }
}

CBIOS_VOID cbSaveRegTableU8(PCBIOS_EXTENSION_COMMON pcbe, CBREGISTER *pRegTable, const CBIOS_U32 TableSize, CBIOS_U8* SavedRegTable)
{
    CBIOS_U32 i = 0;

    for(i = 0; i < TableSize; i++)
    {
        SavedRegTable[i] = (cbBiosMMIOReadReg(pcbe, (((CBIOS_U16)pRegTable[i].type << 8) | pRegTable[i].index),
                                              CBIOS_NOIGAENCODERINDEX))&((CBIOS_U8)~pRegTable[i].mask);

        cbMMIOWriteReg(pcbe, (((CBIOS_U16)pRegTable[i].type << 8) | pRegTable[i].index),
                       pRegTable[i].value, pRegTable[i].mask);
    }
}

CBIOS_VOID cbRestoreRegTableU8(PCBIOS_EXTENSION_COMMON pcbe, const CBREGISTER *pRegTable, const CBIOS_U32 TableSize, CBIOS_U8* SavedRegTable)
{
    CBIOS_U32 i = 0;

    for(i = 0; i < TableSize; i++)
    {
        cbMMIOWriteReg(pcbe, (((CBIOS_U16)pRegTable[i].type<<8) | pRegTable[i].index), SavedRegTable[i], pRegTable[i].mask);
    }
}

CBIOS_VOID cbSaveRegTableU32(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_REGISTER32 *pRegTable, const CBIOS_U32 TableSize)
{
    CBIOS_U32 i = 0;

    for(i = 0; i < TableSize; i++)
    {
        pRegTable[i].value = cbReadRegisterU32(pcbe, pRegTable[i].type, pRegTable[i].index) & (~pRegTable[i].mask);
    }
}

CBIOS_VOID cbRestoreRegTableU32(PCBIOS_EXTENSION_COMMON pcbe, const CBIOS_REGISTER32 *pRegTable, const CBIOS_U32 TableSize)
{
    CBIOS_U32 i = 0;

    for(i = 0; i < TableSize; i++)
    {
        cbWriteRegisterU32(pcbe, pRegTable[i].type, pRegTable[i].index, pRegTable[i].mask);
    }
}

///////////////////////////////////////////////////////////////////////////////
//       CalClock - Find Integral/Fractional part of PLL divider value
//                  and PLL R value
//       Entry: EDI = Clock / 10000
//       Exit:  Databuf.Integer = Intergal part
//              Databuf.Fraction = Fractional part
//              Databuf.R = R value
///////////////////////////////////////////////////////////////////////////////
static CBIOS_BOOL cbCalCLK(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 ClockFreq, CBIOS_U32 ClockType, PCBIOS_CLOCK_INFO pClock, CBIOS_BOOL bGivenR)//ClockFreq(in MHz)*10000
{
    CBIOS_S8  i;
    CBIOS_U32 iClkFreq;
    CBIOS_U64 uTemp;
    CBIOS_U32 ulVcoValue = 0;
    CBIOS_U32 ulRefFreq;
    CBIOS_S8  R_range = 7;    // 3-bits

    R_range = 6;    // max div 64

    ulVcoValue = 12000000;

    if(bGivenR && pClock->R > 0 && pClock->R <= R_range)
    {
        iClkFreq = ClockFreq << pClock->R;
    }
    else
    {
        for(i = R_range ; i >= 0 ; i--)
        {

            iClkFreq = ClockFreq << i;
            if ((iClkFreq < ulVcoValue) || (iClkFreq > 2*ulVcoValue))
            {
                continue;
            }
            else
            {
                break;
            }
        }
        if( i < 0 )
        {
            //out of range
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbCalCLK: fata error -- out of range!!!\n"));
            return CBIOS_FALSE;
        }

        pClock->R = (i > 3) ? 3 : i;
        pClock->PLLDiv = (i > 3) ? (i - 3) : 0;
    }

    if(pcbe->ChipCaps.Is24MReferenceClock)
        ulRefFreq = RefFreq24;
    else
        ulRefFreq = RefFreq27;

    // Xinwei's suggestion to set CP = 0;
    pClock->CP = 0;// (iClkFreq <= Vco_Ref) ? 1 : 2;

    // Prevent being divided by zero
    if (ulRefFreq == 0)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbCalCLK: fata error -- ulRefFreq is ZERO!!!\n"));
        return CBIOS_FALSE;
    }
    uTemp = iClkFreq / ulRefFreq;
    pClock->Integer = (CBIOS_U8)uTemp - 2;

    uTemp = iClkFreq % ulRefFreq;
    uTemp = uTemp<<20;

    uTemp = cb_do_div(uTemp, ulRefFreq);
    pClock->Fraction = (CBIOS_U32) uTemp;

    return CBIOS_TRUE;
}

static CBIOS_VOID cbCalFreq(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 *ClockFreq, PCBIOS_CLOCK_INFO pClock)
{
    CBIOS_U16 Integer = 0;
    CBIOS_U64 Fraction = 0;
    CBIOS_U16 R = 0;
    CBIOS_U32 ulRefFreq = 0;

    if (pClock == CBIOS_NULL)
    {
        //input para error
        return;
    }
    Integer = pClock->Integer;
    Fraction = pClock->Fraction;
    R = pClock->R;

    ulRefFreq = (pcbe->ChipCaps.Is24MReferenceClock) ? RefFreq24 : RefFreq27;

    *ClockFreq = (((Integer+2)*ulRefFreq) + (CBIOS_U32)((Fraction*ulRefFreq)>>20))>>(R + pClock->PLLDiv);
}


CBIOS_BOOL cbProgClock(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 ClockFreq, CBIOS_U32 ClockType, CBIOS_U8 IGAIndex)
{
    CBIOS_CLOCK_INFO ClkInfo;
    CBIOS_U32 ClockTypeRep = ClockType;

    REG_SR15    RegSR15Value, RegSR15Mask;
    REG_CRDA    RegCRDAValue, RegCRDAMask;
    REG_CRDB    RegCRDBValue, RegCRDBMask;
    REG_CRDD    RegCRDDValue, RegCRDDMask;
    REG_CRDE    RegCRDEValue, RegCRDEMask;
    REG_CRE0    RegCRE0Value, RegCRE0Mask;
    REG_CRE2    RegCRE2Value, RegCRE2Mask;
    REG_CRFD_B  RegCRFD_B_Value, RegCRFD_B_Mask;

    cb_memset(&ClkInfo, 0, sizeof(CBIOS_CLOCK_INFO));

    cbTraceEnter(GENERIC);

    if(pcbe->bRunOnQT)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO),"Exit: cbProgClock for QT PCIID !\n"));
        return CBIOS_TRUE;
    }

    if(pcbe->ChipID == CHIPID_ARISE10C0T && (ClockType == CBIOS_ECLKTYPE || ClockType == CBIOS_VCLKTYPE))
    {
        //Don't support set ECLK or VCLK by driver for Arise 10C0t card
        return CBIOS_TRUE;
    }

    if((ClockType == CBIOS_ECLKTYPE) || (ClockType == CBIOS_VCLKTYPE))
    {
        ClockFreq *= 2;    //EPLL/VPLL always output ECLK2x/VCLK2x
    }

    if(ClockType >= CBIOS_INVALID_CLK)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbProgClock: Invalid ClockType !!\n"));
        return CBIOS_FALSE;
    }

    if (!cbCalCLK(pcbe, ClockFreq, ClockType, &ClkInfo, CBIOS_FALSE))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbCalCLK failure, clocktype is %d !\n", ClockType));
        return CBIOS_FALSE;
    }

    switch (ClockTypeRep)
    {
    case CBIOS_DCLK1TYPE:
        cbMapMaskWrite(pcbe,ClkInfo.Integer, E3K_DCLK1_Integer_MAP, CBIOS_NOIGAENCODERINDEX);
        cbMapMaskWrite(pcbe,ClkInfo.Fraction, E3K_DCLK1_Fraction_MAP, CBIOS_NOIGAENCODERINDEX);
        cbMapMaskWrite(pcbe,ClkInfo.R, E3K_DCLK1_R_MAP, CBIOS_NOIGAENCODERINDEX);
        cbMapMaskWrite(pcbe,ClkInfo.PLLDiv, E3K_DCLK1_DIV_MAP, CBIOS_NOIGAENCODERINDEX);

        //Load DClk1
        cb_DelayMicroSeconds(50);

        RegSR15Value.Value = 0;
        RegSR15Value.DCLK1_M_R_Load = 1;
        RegSR15Mask.Value = 0xFF;
        RegSR15Mask.DCLK1_M_R_Load = 0;
        cbMMIOWriteReg(pcbe,SR_15,RegSR15Value.Value, RegSR15Mask.Value);

        cb_DelayMicroSeconds(50);

        RegSR15Value.Value = 0;
        RegSR15Value.DCLK1_M_R_Load = 0;
        RegSR15Mask.Value = 0xFF;
        RegSR15Mask.DCLK1_M_R_Load = 0;
        cbMMIOWriteReg(pcbe,SR_15,RegSR15Value.Value, RegSR15Mask.Value);

        cb_DelayMicroSeconds(50);

        RegSR15Value.Value = 0;
        RegSR15Value.DCLK1_PLL_LOAD = 0;
        RegSR15Mask.Value = 0xFF;
        RegSR15Mask.DCLK1_PLL_LOAD = 0;
        cbMMIOWriteReg(pcbe,SR_15, RegSR15Value.Value, RegSR15Mask.Value);

        cb_DelayMicroSeconds(50);

        RegSR15Value.Value = 0;
        RegSR15Value.DCLK1_PLL_LOAD = 1;
        RegSR15Mask.Value = 0xFF;
        RegSR15Mask.DCLK1_PLL_LOAD = 0;
        cbMMIOWriteReg(pcbe,SR_15, RegSR15Value.Value, RegSR15Mask.Value);

        cb_DelayMicroSeconds(50);

        break;
    case CBIOS_DCLK2TYPE:
        cbMapMaskWrite(pcbe,ClkInfo.Integer, E3K_DCLK2_Integer_MAP, CBIOS_NOIGAENCODERINDEX);
        cbMapMaskWrite(pcbe,ClkInfo.Fraction, E3K_DCLK2_Fraction_MAP, CBIOS_NOIGAENCODERINDEX);
        cbMapMaskWrite(pcbe,ClkInfo.R, E3K_DCLK2_R_MAP, CBIOS_NOIGAENCODERINDEX);
        cbMapMaskWrite(pcbe,ClkInfo.PLLDiv, E3K_DCLK2_DIV_MAP, CBIOS_NOIGAENCODERINDEX);

        //Load DClk2
        cb_DelayMicroSeconds(50);

        RegSR15Value.Value = 0;
        RegSR15Value.DCLK2_M_R_Load = 1;
        RegSR15Mask.Value = 0xFF;
        RegSR15Mask.DCLK2_M_R_Load = 0;
        cbMMIOWriteReg(pcbe,SR_15, RegSR15Value.Value, RegSR15Mask.Value);

        cb_DelayMicroSeconds(50);

        RegSR15Value.Value = 0;
        RegSR15Value.DCLK2_M_R_Load = 0;
        RegSR15Mask.Value = 0xFF;
        RegSR15Mask.DCLK2_M_R_Load = 0;
        cbMMIOWriteReg(pcbe,SR_15, RegSR15Value.Value, RegSR15Mask.Value);

        cb_DelayMicroSeconds(50);

        RegSR15Value.Value = 0;
        RegSR15Value.DCLK2_PLL_LOAD = 0;
        RegSR15Mask.Value = 0xFF;
        RegSR15Mask.DCLK2_PLL_LOAD = 0;
        cbMMIOWriteReg(pcbe,SR_15, RegSR15Value.Value, RegSR15Mask.Value);

        cb_DelayMicroSeconds(50);

        RegSR15Value.Value = 0;
        RegSR15Value.DCLK2_PLL_LOAD = 1;
        RegSR15Mask.Value = 0xFF;
        RegSR15Mask.DCLK2_PLL_LOAD = 0;
        cbMMIOWriteReg(pcbe,SR_15, RegSR15Value.Value, RegSR15Mask.Value);

        cb_DelayMicroSeconds(50);
        break;
    case CBIOS_DCLK3TYPE:
        cbMapMaskWrite(pcbe,ClkInfo.Integer, E3K_DCLK3_Integer_MAP, CBIOS_NOIGAENCODERINDEX);
        cbMapMaskWrite(pcbe,ClkInfo.Fraction, E3K_DCLK3_Fraction_MAP, CBIOS_NOIGAENCODERINDEX);
        cbMapMaskWrite(pcbe,ClkInfo.R, E3K_DCLK3_R_MAP, CBIOS_NOIGAENCODERINDEX);
        cbMapMaskWrite(pcbe,ClkInfo.PLLDiv, E3K_DCLK3_DIV_MAP, CBIOS_NOIGAENCODERINDEX);

        //Load DClk3
        cb_DelayMicroSeconds(50);

        RegSR15Value.Value = 0;
        RegSR15Value.DCLK3_M_R_Load = 1;
        RegSR15Mask.Value = 0xFF;
        RegSR15Mask.DCLK3_M_R_Load = 0;
        cbMMIOWriteReg(pcbe,SR_15, RegSR15Value.Value, RegSR15Mask.Value);

        cb_DelayMicroSeconds(50);

        RegSR15Value.Value = 0;
        RegSR15Value.DCLK3_M_R_Load = 0;
        RegSR15Mask.Value = 0xFF;
        RegSR15Mask.DCLK3_M_R_Load = 0;
        cbMMIOWriteReg(pcbe,SR_15, RegSR15Value.Value, RegSR15Mask.Value);

        cb_DelayMicroSeconds(50);

        RegSR15Value.Value = 0;
        RegSR15Value.DCLK3_PLL_LOAD = 0;
        RegSR15Mask.Value = 0xFF;
        RegSR15Mask.DCLK3_PLL_LOAD = 0;
        cbMMIOWriteReg(pcbe,SR_15, RegSR15Value.Value, RegSR15Mask.Value);

        cb_DelayMicroSeconds(50);

        RegSR15Value.Value = 0;
        RegSR15Value.DCLK3_PLL_LOAD = 1;
        RegSR15Mask.Value = 0xFF;
        RegSR15Mask.DCLK3_PLL_LOAD = 0;
        cbMMIOWriteReg(pcbe,SR_15, RegSR15Value.Value, RegSR15Mask.Value);

        cb_DelayMicroSeconds(50);

        break;
    case CBIOS_DCLK4TYPE:
        cbMapMaskWrite(pcbe,ClkInfo.Integer, E3K_DCLK4_Integer_MAP, CBIOS_NOIGAENCODERINDEX);
        cbMapMaskWrite(pcbe,ClkInfo.Fraction, E3K_DCLK4_Fraction_MAP, CBIOS_NOIGAENCODERINDEX);
        cbMapMaskWrite(pcbe,ClkInfo.R, E3K_DCLK4_R_MAP, CBIOS_NOIGAENCODERINDEX);
        cbMapMaskWrite(pcbe,ClkInfo.PLLDiv, E3K_DCLK4_DIV_MAP, CBIOS_NOIGAENCODERINDEX);

        //Load DClk4
        cb_DelayMicroSeconds(50);

        RegCRFD_B_Value.Value = 0;
        RegCRFD_B_Value.LOAD_DCLK4 = 1;
        RegCRFD_B_Mask.Value = 0xFF;
        RegCRFD_B_Mask.LOAD_DCLK4 = 0;
        cbMMIOWriteReg(pcbe, CR_B_FD, RegCRFD_B_Value.Value, RegCRFD_B_Mask.Value);

        cb_DelayMicroSeconds(50);

        RegCRFD_B_Value.Value = 0;
        RegCRFD_B_Value.LOAD_DCLK4 = 0;
        RegCRFD_B_Mask.Value = 0xFF;
        RegCRFD_B_Mask.LOAD_DCLK4 = 0;
        cbMMIOWriteReg(pcbe, CR_B_FD, RegCRFD_B_Value.Value, RegCRFD_B_Mask.Value);

        cb_DelayMicroSeconds(50);

        RegCRFD_B_Value.Value = 0;
        RegCRFD_B_Value.SOFT_LOAD_D4CLK = 0;
        RegCRFD_B_Mask.Value = 0xFF;
        RegCRFD_B_Mask.SOFT_LOAD_D4CLK = 0;
        cbMMIOWriteReg(pcbe, CR_B_FD, RegCRFD_B_Value.Value, RegCRFD_B_Mask.Value);

        cb_DelayMicroSeconds(50);

        RegCRFD_B_Value.Value = 0;
        RegCRFD_B_Value.SOFT_LOAD_D4CLK = 1;
        RegCRFD_B_Mask.Value = 0xFF;
        RegCRFD_B_Mask.SOFT_LOAD_D4CLK = 0;
        cbMMIOWriteReg(pcbe, CR_B_FD, RegCRFD_B_Value.Value, RegCRFD_B_Mask.Value);

        cb_DelayMicroSeconds(50);
        break;
    case ECLKTYPE_Post:
    case CBIOS_ECLKTYPE:
        // EPLL setting sequence:
        // RST down -> set PLL regs -> load setting -> wait 1us -> RST up
        // ECLK only use Integer part, hardcode CKOUT1 by HW
        RegCRE2Value.Value = 0;
        RegCRE2Value.EPLL_SW_CTRL = 1;
        RegCRE2Value.EPLL_SW_LOAD = 0;
        RegCRE2Value.EPLL_RST_DOWN = 1;
        RegCRE2Mask.Value = 0xFF;
        RegCRE2Mask.EPLL_SW_CTRL = 0;
        RegCRE2Mask.EPLL_SW_LOAD = 0;
        RegCRE2Mask.EPLL_RST_DOWN = 0;
        cbMMIOWriteReg(pcbe, CR_E2, RegCRE2Value.Value, RegCRE2Mask.Value);

        RegCRDDValue.Value = 0;
        RegCRDDValue.EPLL_Band_Sel = 1;
        RegCRDDValue.EPLL_PU_Down = 0;
        RegCRDDMask.Value = 0xFF;
        RegCRDDMask.EPLL_Band_Sel = 0;
        RegCRDDMask.EPLL_PU_Down = 0;
        cbMMIOWriteReg(pcbe, CR_DD, RegCRDDValue.Value, RegCRDDMask.Value);

        RegCRDEValue.Value = 0;
        RegCRDEValue.EPLL_Fraction_EN = 0;
        RegCRDEMask.Value = 0xFF;
        RegCRDEMask.EPLL_Fraction_EN = 0;
        cbMMIOWriteReg(pcbe, CR_DE, RegCRDEValue.Value, RegCRDEMask.Value);

        cbMapMaskWrite(pcbe,ClkInfo.Integer, E3K_ECLK_Integer_MAP, CBIOS_NOIGAENCODERINDEX);
        cbMapMaskWrite(pcbe,ClkInfo.R, E3K_ECLK_R_MAP, CBIOS_NOIGAENCODERINDEX);

        // load ECLK
        RegCRE2Value.Value = 0;
        RegCRE2Value.EPLL_SW_LOAD = 1;
        RegCRE2Mask.Value = 0xFF;
        RegCRE2Mask.EPLL_SW_LOAD = 0;
        cbMMIOWriteReg(pcbe, CR_E2, RegCRE2Value.Value, RegCRE2Mask.Value);

        cb_DelayMicroSeconds(1);

        RegCRE2Value.Value = 0;
        RegCRE2Value.EPLL_SW_CTRL = 1;
        RegCRE2Value.EPLL_SW_LOAD = 0;
        RegCRE2Value.EPLL_RST_DOWN = 0;
        RegCRE2Mask.Value = 0xFF;
        RegCRE2Mask.EPLL_SW_CTRL = 0;
        RegCRE2Mask.EPLL_SW_LOAD = 0;
        RegCRE2Mask.EPLL_RST_DOWN = 0;
        cbMMIOWriteReg(pcbe, CR_E2, RegCRE2Value.Value, RegCRE2Mask.Value);

        break;
    case CBIOS_VCLKTYPE:
        //VPLL setting sequence:
        // RST down -> set PLL regs -> load setting -> wait 1us -> RST up
        //VCLK only use Integer part, hardcode CKOUT1 by HW
        RegCRE0Value.Value = 0;
        RegCRE0Value.VPLL_SW_CTRL = 1;
        RegCRE0Value.VPLL_SW_LOAD = 0;
        RegCRE0Value.VPLL_RST_DOWN = 1;
        RegCRE0Mask.Value = 0xFF;
        RegCRE0Mask.VPLL_SW_CTRL = 0;
        RegCRE0Mask.VPLL_SW_LOAD = 0;
        RegCRE0Mask.VPLL_RST_DOWN = 0;
        cbMMIOWriteReg(pcbe, CR_E0, RegCRE0Value.Value, RegCRE0Mask.Value);

        RegCRDAValue.Value = 0;
        RegCRDAValue.VPLL_Band_Sel = 1;
        RegCRDAValue.VPLL_PU_Down = 0;
        RegCRDAMask.Value = 0xFF;
        RegCRDAMask.VPLL_Band_Sel = 0;
        RegCRDAMask.VPLL_PU_Down = 0;
        cbMMIOWriteReg(pcbe, CR_DA, RegCRDAValue.Value, RegCRDAMask.Value);

        RegCRDBValue.Value = 0;
        RegCRDBValue.VPLL_Fraction_EN = 0;
        RegCRDBMask.Value = 0xFF;
        RegCRDBMask.VPLL_Fraction_EN = 0;
        cbMMIOWriteReg(pcbe, CR_DB, RegCRDBValue.Value, RegCRDBMask.Value);

        cbMapMaskWrite(pcbe,ClkInfo.Integer, E3K_VCLK_Integer_MAP, CBIOS_NOIGAENCODERINDEX);
        cbMapMaskWrite(pcbe,ClkInfo.R, E3K_VCLK_R_MAP, CBIOS_NOIGAENCODERINDEX);

        // load VCLK
        RegCRE0Value.Value = 0;
        RegCRE0Value.VPLL_SW_LOAD = 1;
        RegCRE0Mask.Value = 0xFF;
        RegCRE0Mask.VPLL_SW_LOAD = 0;
        cbMMIOWriteReg(pcbe, CR_E0, RegCRE0Value.Value, RegCRE0Mask.Value);

        cb_DelayMicroSeconds(1);

        RegCRE0Value.Value = 0;
        RegCRE0Value.VPLL_SW_CTRL = 1;
        RegCRE0Value.VPLL_SW_LOAD = 0;
        RegCRE0Value.VPLL_RST_DOWN = 0;
        RegCRE0Mask.Value = 0xFF;
        RegCRE0Mask.VPLL_SW_CTRL = 0;
        RegCRE0Mask.VPLL_SW_LOAD = 0;
        RegCRE0Mask.VPLL_RST_DOWN = 0;
        cbMMIOWriteReg(pcbe, CR_E0, RegCRE0Value.Value, RegCRE0Mask.Value);

        break;

    default:
        break;
    }
    cbTraceExit(GENERIC);
    return CBIOS_TRUE;//??? not found in BIOS code
}


///////////////////////////////////////////////////////////////////////////////
//  GetProgClock - This function returns the clock value.
//
//  Entry:  CL = (00h) Select MCLK
//           (01h) Select DCLK1
//           (02h) Select DCLK2
//           (03h) Select TVCLK
//           (04h) Select ECLK
//           (05h) Select ICLK
//  Exit:   EDI = (??h) Current clock frequency
///////////////////////////////////////////////////////////////////////////////
CBIOS_U32 cbGetProgClock(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 *ClockFreq, CBIOS_U32 ClockType)
{
    CBIOS_CLOCK_INFO ClkInfo = {0};
    CBIOS_U32 RegD130 = 0;
    CBIOS_U32 D300_Value = 0;
    CBIOS_BOOL bUseNewMclk = CBIOS_FALSE;

    if(ClockType >= CBIOS_INVALID_CLK)
    {
        return -1;
    }
    if ((pcbe->ChipID == CHIPID_ARISE1020 || pcbe->ChipID == CHIPID_ARISE2030) && ClockType == CBIOS_MCLKTYPE)
    {
        D300_Value = cb_ReadU32(pcbe->pAdapterContext, 0xD300);
        bUseNewMclk = ((D300_Value & 0xF) == 0xA) ? CBIOS_TRUE : CBIOS_FALSE;
    }

    switch (ClockType)
    {
    case CBIOS_DCLK1TYPE:
        ClkInfo.Integer = (CBIOS_U8)cbMapMaskRead(pcbe, E3K_DCLK1_Integer_MAP, CBIOS_NOIGAENCODERINDEX);
        ClkInfo.Fraction = (CBIOS_U32)cbMapMaskRead(pcbe, E3K_DCLK1_Fraction_MAP, CBIOS_NOIGAENCODERINDEX);
        ClkInfo.R = (CBIOS_U8)cbMapMaskRead(pcbe, E3K_DCLK1_R_MAP, CBIOS_NOIGAENCODERINDEX);
        ClkInfo.PLLDiv = (CBIOS_U8)cbMapMaskRead(pcbe, E3K_DCLK1_DIV_MAP, CBIOS_NOIGAENCODERINDEX);
        break;
    case CBIOS_DCLK2TYPE:
        ClkInfo.Integer = (CBIOS_U8)cbMapMaskRead(pcbe, E3K_DCLK2_Integer_MAP, CBIOS_NOIGAENCODERINDEX);
        ClkInfo.Fraction = (CBIOS_U32)cbMapMaskRead(pcbe, E3K_DCLK2_Fraction_MAP, CBIOS_NOIGAENCODERINDEX);
        ClkInfo.R = (CBIOS_U8)cbMapMaskRead(pcbe, E3K_DCLK2_R_MAP, CBIOS_NOIGAENCODERINDEX);
        ClkInfo.PLLDiv = (CBIOS_U8)cbMapMaskRead(pcbe, E3K_DCLK2_DIV_MAP, CBIOS_NOIGAENCODERINDEX);
        break;
    case CBIOS_DCLK3TYPE:
        ClkInfo.Integer = (CBIOS_U8)cbMapMaskRead(pcbe, E3K_DCLK3_Integer_MAP, CBIOS_NOIGAENCODERINDEX);
        ClkInfo.Fraction = (CBIOS_U32)cbMapMaskRead(pcbe, E3K_DCLK3_Fraction_MAP, CBIOS_NOIGAENCODERINDEX);
        ClkInfo.R = (CBIOS_U8)cbMapMaskRead(pcbe, E3K_DCLK3_R_MAP, CBIOS_NOIGAENCODERINDEX);
        ClkInfo.PLLDiv = (CBIOS_U8)cbMapMaskRead(pcbe, E3K_DCLK3_DIV_MAP, CBIOS_NOIGAENCODERINDEX);
        break;
    case CBIOS_DCLK4TYPE:
        ClkInfo.Integer = (CBIOS_U8)cbMapMaskRead(pcbe, E3K_DCLK4_Integer_MAP, CBIOS_NOIGAENCODERINDEX);
        ClkInfo.Fraction = (CBIOS_U32)cbMapMaskRead(pcbe, E3K_DCLK4_Fraction_MAP, CBIOS_NOIGAENCODERINDEX);
        ClkInfo.R = (CBIOS_U8)cbMapMaskRead(pcbe, E3K_DCLK4_R_MAP, CBIOS_NOIGAENCODERINDEX);
        ClkInfo.PLLDiv = (CBIOS_U8)cbMapMaskRead(pcbe, E3K_DCLK4_DIV_MAP, CBIOS_NOIGAENCODERINDEX);
        break;
    case CBIOS_ECLKTYPE:
        if (pcbe->ChipID == CHIPID_ARISE10C0T)
        {
            *ClockFreq = cb_ReadU32(pcbe->pAdapterContext, 0xD3E4);
        }
        else
        {
            ClkInfo.Integer = (CBIOS_U8)cbMapMaskRead(pcbe, E3K_ECLK_Integer_MAP, CBIOS_NOIGAENCODERINDEX);
            ClkInfo.R = (CBIOS_U8)cbMapMaskRead(pcbe, E3K_ECLK_R_MAP, CBIOS_NOIGAENCODERINDEX);
            ClkInfo.Fraction = 0;
            ClkInfo.PLLDiv = 0;
        }
        break;
    case CBIOS_VCLKTYPE:
        if (pcbe->ChipID == CHIPID_ARISE10C0T)
        {
            *ClockFreq = cb_ReadU32(pcbe->pAdapterContext, 0xD3E8);
        }
        else
        {
            ClkInfo.Integer = (CBIOS_U8)cbMapMaskRead(pcbe, E3K_VCLK_Integer_MAP, CBIOS_NOIGAENCODERINDEX);
            ClkInfo.R = (CBIOS_U8)cbMapMaskRead(pcbe, E3K_VCLK_R_MAP, CBIOS_NOIGAENCODERINDEX);
            ClkInfo.Fraction = 0;
            ClkInfo.PLLDiv = 0;
        }
        break;
    case CBIOS_MCLKTYPE:
        if(bUseNewMclk)
        {
            *ClockFreq = (D300_Value & 0xFFF00)>>8;
        }
        else
        {
            RegD130 = cb_ReadU32(pcbe->pAdapterContext, 0xd130);
            ClkInfo.Integer = ((RegD130 >> 0x7) & 0x7F) | ((RegD130 & 0x10)? 0x80:0);
            ClkInfo.R = (RegD130 >> 0x11) & 0x3;
            ClkInfo.Fraction = 0;
            ClkInfo.PLLDiv = 0;
        }
        break;
    default:
        break;
    }

    if ((pcbe->ChipID == CHIPID_ARISE10C0T && (ClockType == CBIOS_ECLKTYPE || ClockType == CBIOS_VCLKTYPE)) || bUseNewMclk)
    {
        (*ClockFreq) *= 10000;
    }
    else
    {
        cbCalFreq(pcbe,ClockFreq,&ClkInfo);

        if((ClockType == CBIOS_ECLKTYPE) || (ClockType == CBIOS_VCLKTYPE))
        {
            (*ClockFreq) /= 2;    //EPLL/VPLL always output ECLK2x/VCLK2x
        }
    }

    return CBIOS_TRUE;
}

CBIOS_BOOL cbWaitNonFullVBlank(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U8 IGAIndex)
{
    CBIOS_GET_HW_COUNTER  GetCnt = {0};
    CBIOS_U32 i = 0, oldcnt, newcnt;
    CBIOS_UCHAR byte;

    if (pcbe->bRunOnQT)
    {
        return CBIOS_TRUE;
    }

    byte= cbBiosMMIOReadReg(pcbe, CR_34, IGAIndex);
    if (byte & VBLANK_ACTIVE_CR34)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "Skip wait, IGA %d already in VBlank.\n", IGAIndex));
        return CBIOS_TRUE;
    }

    GetCnt.IgaIndex = IGAIndex;
    GetCnt.bGetPixelCnt = 1;

    cbHwGetCounter(pcbe, &GetCnt);
    oldcnt = GetCnt.Value[CBIOS_COUNTER_PIXEL];
    cb_DelayMicroSeconds(1);
    cbHwGetCounter(pcbe, &GetCnt);
    newcnt = GetCnt.Value[CBIOS_COUNTER_PIXEL];

    if(oldcnt == newcnt)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "Skip wait IGA %d VBlank as no timing.\n", IGAIndex));
        return  CBIOS_TRUE;
    }

    for(i = 0; i < 800; i++)
    {
        byte= cbBiosMMIOReadReg(pcbe, CR_34, IGAIndex);
        if (byte & VBLANK_ACTIVE_CR34)
        {
            break;
        }

        cb_DelayMicroSeconds(50);
    }

    if(i == 800)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "Wait IGA %d NonFullVBlank time out!\n", IGAIndex));
    }

    return (i == 800)? CBIOS_FALSE : CBIOS_TRUE;
}


/*
Routine Description:
    Wait for the vertical blanking interval on the chip based on IGAs
    It is a paired register, so it is the caller responsiblity to
    set the correct IGA first before call here.

Return Value:
    Always CBIOS_TRUE
*/
CBIOS_BOOL cbWaitVBlank(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U8 IGAIndex)
{
    CBIOS_GET_HW_COUNTER  GetCnt = {0};
    CBIOS_U32 i = 0, timeout = 0, oldcnt, newcnt;
    CBIOS_UCHAR byte;

    if (pcbe->bRunOnQT)
    {
        return CBIOS_TRUE;
    }

    GetCnt.IgaIndex = IGAIndex;
    GetCnt.bGetPixelCnt = 1;

    cbHwGetCounter(pcbe, &GetCnt);
    oldcnt = GetCnt.Value[CBIOS_COUNTER_PIXEL];
    cb_DelayMicroSeconds(1);
    cbHwGetCounter(pcbe, &GetCnt);
    newcnt = GetCnt.Value[CBIOS_COUNTER_PIXEL];

    if(oldcnt == newcnt)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "Skip wait IGA %d VBlank as no timing.\n", IGAIndex));
        return  CBIOS_TRUE;
    }

    //wait for not-VBlank area
    //vblank is about (0.4ms ~ 2ms), display active is about (7ms ~ 18ms)
    //wait display active, one step is 1ms, max 15ms
    for(i = 0; i < 16; i++)
    {
        byte= cbBiosMMIOReadReg(pcbe, CR_34, IGAIndex);
        if (!(byte & VBLANK_ACTIVE_CR34))
        {
            break;
        }
        cb_DelayMicroSeconds(1000);
    }

    if(i == 16)
    {
        timeout = 1;
        goto End;
    }

    //wait vblank, one step is 0.05ms, max 40ms
    for(i = 0; i < 800; i++)
    {
        byte= cbBiosMMIOReadReg(pcbe, CR_34, IGAIndex);
        if (byte & VBLANK_ACTIVE_CR34)
        {
            break;
        }

        cb_DelayMicroSeconds(50);
    }

    if(i == 800)
    {
        timeout = 1;
        goto End;
    }

End:
    if (timeout)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "Wait IGA %d VBlank time out!\n", IGAIndex));
    }

    return CBIOS_TRUE;
}  //cbWait_VBlank

/*
Routine Description:
    Wait for the vertical Sync on the chip based on IGAs
    It is a paired register, so it is the caller responsiblity to
    set the correct IGA first before call here.

Return Value:
    Always CBIOS_TRUE
*/
CBIOS_BOOL cbWaitVSync(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U8 IGAIndex)
{

    CBIOS_U32 i = 0; // Record the loop times.
    CBIOS_UCHAR byte;

    if (pcbe->bRunOnQT)
    {
        return CBIOS_TRUE;
    }

    //wait for not-VSYNC area
    for (i = 0; i < CBIOS_VBLANK_RETRIES; i++)
    {
        byte= cbBiosMMIOReadReg(pcbe, CR_33, IGAIndex);
        if (!(byte & VSYNC_ACTIVE_CR33))
        {
            break;
        }
    }

    // Now wait VSYNC again.
    for (i = 0; i < CBIOS_VBLANK_RETRIES; i++)
    {
        byte= cbBiosMMIOReadReg(pcbe, CR_33, IGAIndex);
        if (byte & VSYNC_ACTIVE_CR33)
        {
            break;
        }
    }

    if (i == CBIOS_VBLANK_RETRIES)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "wait VSYNC time out!\n"));
    }

    return (CBIOS_TRUE);

}  //cbWait_VSync

CBIOS_BOOL cbHDCPDDCciPortWrite(PCBIOS_EXTENSION_COMMON pcbe,CBIOS_UCHAR Port, CBIOS_UCHAR Offset, const PCBIOS_UCHAR pWriteDataBuf, CBIOS_U32 DataLen, CBIOS_S32 HDCPChannel)
{
    CBIOS_U32 i, j;
    CBIOS_U32 maxloop = MAXI2CWAITLOOP;
    CBIOS_U8  I2CDELAY = pcbe->I2CDelay;
    CBIOS_U32 HdcpCtrl1Reg, HdcpCtrl2Reg;

    cbTraceEnter(GENERIC);
    switch(HDCPChannel)
    {
    case HDCP1:
        {
            HdcpCtrl1Reg = HDCPCTL1_DEST;
            HdcpCtrl2Reg = HDCPCTL2_DEST;
    	}
        break;
    case HDCP2:
        {
            HdcpCtrl1Reg = HDCP2_CTL1_DEST;
            HdcpCtrl2Reg = HDCP2_CTL2_DEST;
    	}
        break;
    case HDCP3:
        {
            HdcpCtrl1Reg = HDCP3_CTL1_DEST;
            HdcpCtrl2Reg = HDCP3_CTL2_DEST;
    	}
        break;
    case HDCP4:
        {
            HdcpCtrl1Reg = HDCP4_CTL1_DEST;
            HdcpCtrl2Reg = HDCP4_CTL2_DEST;
    	}
        break;
    default:
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP channel: %d\n", FUNCTION_NAME, HDCPChannel));
            return CBIOS_FALSE;
    	}
        break;
    }

    //start
    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl2Reg,cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) | HDCP_I2C_START_DEST);//set START & WDATA_AV
    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl2Reg,cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) | HDCP_I2C_WDAV_DEST);

    j = 0;
    while(cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) & (HDCP_I2C_START_DEST | HDCP_I2C_WDAV_DEST)) //query START & WDATA_AV until they are zero
    {
        if(j < maxloop)
        {
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!\n", FUNCTION_NAME, LINE_NUM));
            return CBIOS_FALSE;
        }
    }

    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl1Reg,
        (cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl1Reg)&0xff00ffff)|((CBIOS_U32)(Port&~1))<<16);//write the I2C data,first byte should be I2C address
    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl2Reg,
        cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) |HDCP_I2C_WDAV_DEST);//set WDATA_AV

    j = 0;
    while(cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) & HDCP_I2C_WDAV_DEST) //query WDATA_AV until they are zero
    {
        if(j < maxloop)
        {
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!\n", FUNCTION_NAME, LINE_NUM));
            return CBIOS_FALSE;
        }
    }

    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl1Reg,
        (cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl1Reg)&0xff00ffff)|((CBIOS_U32)(Offset))<<16);//write the I2C data,first byte should be I2C address
    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl2Reg,
        cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) |HDCP_I2C_WDAV_DEST);//set WDATA_AV

    j = 0;
    while(cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) & HDCP_I2C_WDAV_DEST) //query WDATA_AV until they are zero
    {
        if(j < maxloop)
        {
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!\n", FUNCTION_NAME, LINE_NUM));
            return CBIOS_FALSE;
        }
    }

    for(i = 0;i<DataLen;i++)
    {
       //write data
        cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl1Reg,
            (cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl1Reg)&0xff00ffff)|((CBIOS_U32)*(pWriteDataBuf+i))<<16);//write the I2C data,first byte should be I2C address
        cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl2Reg,
            cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) | HDCP_I2C_WDAV_DEST);//set WDATA_AV

        j = 0;
        while(cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) & HDCP_I2C_WDAV_DEST) //query WDATA_AV until they are zero
        {
            if(j < maxloop)
            {
                cb_DelayMicroSeconds(I2CDELAY);
                j++;
            }
            else
            {
                cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!\n", FUNCTION_NAME, LINE_NUM));
                return CBIOS_FALSE;
            }
        }
    }
    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl2Reg,cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) | HDCP_I2C_STOP_DEST);//set stop & WDATA_AV;HW bug
    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl2Reg,cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) | HDCP_I2C_WDAV_DEST);

    j = 0;
    while(cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) & (HDCP_I2C_STOP_DEST )) //query stop until they are zero
    {
        if(j < maxloop)
        {
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!\n", FUNCTION_NAME, LINE_NUM));
            return CBIOS_FALSE;
        }
    }

    cbTraceExit(GENERIC);

    return CBIOS_TRUE;
}

CBIOS_BOOL cbHDCPDDCciPortRead(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_UCHAR Port,  CBIOS_UCHAR Offset, const PCBIOS_UCHAR pReadDataBuf, CBIOS_U32 DataLen, CBIOS_S32 HDCPChannel)
{
    CBIOS_U32 i, j;
    CBIOS_U32 maxloop = MAXI2CWAITLOOP;
    CBIOS_U8  I2CDELAY = pcbe->I2CDelay;
    CBIOS_U32 HdcpCtrl1Reg, HdcpCtrl2Reg;

    cbTraceEnter(GENERIC);
    switch(HDCPChannel)
    {
    case HDCP1:
        {
            HdcpCtrl1Reg = HDCPCTL1_DEST;
            HdcpCtrl2Reg = HDCPCTL2_DEST;
    	}
        break;
    case HDCP2:
        {
            HdcpCtrl1Reg = HDCP2_CTL1_DEST;
            HdcpCtrl2Reg = HDCP2_CTL2_DEST;
    	}
        break;
    case HDCP3:
        {
            HdcpCtrl1Reg = HDCP3_CTL1_DEST;
            HdcpCtrl2Reg = HDCP3_CTL2_DEST;
    	}
        break;
    case HDCP4:
        {
            HdcpCtrl1Reg = HDCP4_CTL1_DEST;
            HdcpCtrl2Reg = HDCP4_CTL2_DEST;
    	}
        break;
    default:
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP channel: %d\n", FUNCTION_NAME, HDCPChannel));
            return CBIOS_FALSE;
    	}
        break;
    }

    //start
    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl2Reg,cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) | HDCP_I2C_START_DEST);//set START & WDATA_AV
    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl2Reg,cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) | HDCP_I2C_WDAV_DEST);

    j = 0;
    while(cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) & (HDCP_I2C_START_DEST | HDCP_I2C_WDAV_DEST )) //query START until they are zero
    {
        if(j < maxloop)
        {
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!\n", FUNCTION_NAME, LINE_NUM));
            return CBIOS_FALSE;
        }
    }

    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl1Reg,(cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl1Reg) & 0xff00ffff) | (Port & ~1) << 16);//write the I2C address
    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl2Reg,cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) | HDCP_I2C_WDAV_DEST);//set WDATA_AV

    j = 0;
    while(cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) & HDCP_I2C_WDAV_DEST) //query WDATA_AV until they are zero
    {
        if(j < maxloop)
        {
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!\n", FUNCTION_NAME, LINE_NUM));
            return CBIOS_FALSE;
        }
    }

    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl1Reg,(cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl1Reg) & 0xff00ffff) | (Offset << 16));//write the I2C address
    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl2Reg,cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) | HDCP_I2C_WDAV_DEST);//set WDATA_AV

    j = 0;
    while(cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) & HDCP_I2C_WDAV_DEST) //query WDATA_AV until they are zero
    {
        if(j < maxloop)
        {
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!\n", FUNCTION_NAME, LINE_NUM));
            return CBIOS_FALSE;
        }
    }

   //start
    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl2Reg,cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) | HDCP_I2C_START_DEST);//set START & WDATA_AV
    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl2Reg,cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) | HDCP_I2C_WDAV_DEST);
    j = 0;
    while(cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) & (HDCP_I2C_START_DEST | HDCP_I2C_WDAV_DEST )) //query START until they are zero
    {
        if(j < maxloop)
        {
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!\n", FUNCTION_NAME, LINE_NUM));
            return CBIOS_FALSE;
        }
    }

    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl1Reg,(cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl1Reg) & 0xff00ffff) | (Port |1) << 16);//write the I2C address
    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl2Reg,cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) | HDCP_I2C_WDAV_DEST);//set WDATA_AV

    j = 0;
    while(cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) & HDCP_I2C_WDAV_DEST) //query WDATA_AV until they are zero
    {
        if(j < maxloop)
        {
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!\n", FUNCTION_NAME, LINE_NUM));
            return CBIOS_FALSE;
        }
    }
    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl2Reg,cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) & ~HDCP_I2C_RDAV_DEST);//clear RDATA_AV firstly

    for(i = 0;i < DataLen;i++)
    {
        cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl2Reg,cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) | HDCP_I2C_WDAV_DEST);//set WDATA_AV

        j = 0;
        while(cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) & HDCP_I2C_WDAV_DEST) //query WDATA_AV until they are zero
        {
            if(j < maxloop)
            {
                cb_DelayMicroSeconds(I2CDELAY);
                j++;
            }
            else
            {
                cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!\n", FUNCTION_NAME, LINE_NUM));
                return CBIOS_FALSE;
            }
        }

        j = 0;
        while((cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) & HDCP_I2C_RDAV_DEST) == 0) //query RDATA_AV until they are zero
        {
            if(j < maxloop)
            {
                cb_DelayMicroSeconds(I2CDELAY);
                j++;
            }
            else
            {
                cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!\n", FUNCTION_NAME, LINE_NUM));
                return CBIOS_FALSE;
            }
        }

        *(pReadDataBuf+i) = (CBIOS_UCHAR)((cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl1Reg) & 0x0000ff00) >> 8);//read the I2C data
        cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl2Reg,cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) & ~HDCP_I2C_RDAV_DEST);//clear RDATA_AV
    }

    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl2Reg,cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) | HDCP_I2C_STOP_DEST);//set stop & WDATA_AV;HW bug
    cb_WriteU32(pcbe->pAdapterContext,HdcpCtrl2Reg,cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) | HDCP_I2C_WDAV_DEST);

    j = 0;
    while(cb_ReadU32(pcbe->pAdapterContext,HdcpCtrl2Reg) & (HDCP_I2C_STOP_DEST | HDCP_I2C_WDAV_DEST)) //query STOP until they are zero
    {
        if(j < maxloop)
        {
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!\n", FUNCTION_NAME, LINE_NUM));
            return CBIOS_FALSE;
        }
    }

    cbTraceExit(GENERIC);
    return CBIOS_TRUE;
}

#if CBIOS_CHECK_HARDWARE_STATUS
CBIOS_BOOL cbIsMMIOWell(PCBIOS_EXTENSION_COMMON pcbe)
{
    CBIOS_BOOL  bResult = CBIOS_TRUE;
    CBIOS_UCHAR CR6B    = cb_ReadU8(pcbe->pAdapterContext, 0x886B);

    cb_WriteU8(pcbe->pAdapterContext, 0x886B, 0xAA);
    bResult = (0xAA == cb_ReadU8(pcbe->pAdapterContext, 0x886B));
    cb_WriteU8(pcbe->pAdapterContext, 0x886B, CR6B);

    return bResult;
}
#endif


CBIOS_BOOL cbDumpRegisters(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_DUMP_TYPE DumpType)
{
    CBIOS_U32 GroupIndex = 0;
    CBIOS_REGISTER_GROUP *pRegGroup = CBIOS_NULL;
    CBIOS_U32 i = 0,j = 0;
    CBIOS_U8 Values[256];
    CBIOS_U32 DValues[128];
    CBIOS_U16 IndexLen=0;
    CBIOS_REGISTER_GROUP *pRegTable = CBIOS_NULL;
    CBIOS_U32 TableLen = 0;

    pRegTable = CBIOS_REGISTER_TABLE;
    TableLen  = sizeofarray(CBIOS_REGISTER_TABLE);
    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "======Dump registers======\n"));

    for (GroupIndex = 0; GroupIndex < TableLen; GroupIndex++)
    {
        if (DumpType & (1 << GroupIndex))
        {
            pRegGroup = &pRegTable[GroupIndex];
            if(pRegGroup == CBIOS_NULL)
            {
                cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "=====pRegGroup NULL\n"));
                return CBIOS_FALSE;
            }

            if(CBIOS_NULL == (pRegGroup->pRegRange))
            {
                cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "=====DumpType %d is not implemented, skip it!\n", DumpType));
                continue;
            }

            cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "=============[Begin %s group]=============\n", pRegGroup->GroupName));

            for (i = 0; i < pRegGroup->RangeNum; i++)
            {
                if(pRegGroup->pRegRange[i].RegLen == 1)
                {
                    IndexLen = pRegGroup->pRegRange[i].EndIndex - pRegGroup->pRegRange[i].StartIndex + 1;
                    for(j = 0;j < IndexLen;j++)
                    {
                        Values[j] = cb_ReadU8(pcbe->pAdapterContext,pRegGroup->pRegRange[i].StartIndex + j);
                    }

                    if(1 == IndexLen)
                    {
                        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "[0x%04x]: %02x\n",pRegGroup->pRegRange[i].StartIndex,Values[0]));
                    }
                    else{
                        cbPrintU8String(Values,IndexLen,pRegGroup->pRegRange[i].StartIndex);
                    }
                }

                if(pRegGroup->pRegRange[i].RegLen == 4)
                {
                    IndexLen = (pRegGroup->pRegRange[i].EndIndex - pRegGroup->pRegRange[i].StartIndex)/4 + 1;
                    for(j = 0;j < IndexLen;j++)
                    {
                        DValues[j] = cb_ReadU32(pcbe->pAdapterContext, pRegGroup->pRegRange[i].StartIndex + 4*j);
                    }

                    if(1 == IndexLen)
                    {
                        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "[0x%04x]: %08x\n",pRegGroup->pRegRange[i].StartIndex,DValues[0]));
                    }
                    else
                    {
                        cbPrintU32String(DValues,IndexLen,pRegGroup->pRegRange[i].StartIndex);
                    }
                }
            }

            cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "=============[End   %s group]=============\n\n", pRegGroup->GroupName));
        }
    }

    return CBIOS_TRUE;
}


CBIOS_VOID cbDumpModeInfo(PCBIOS_EXTENSION_COMMON pcbe)
{
    CBIOS_U32 IGAIndex = 0;
    PCBIOS_DISP_MODE_PARAMS pModeParams = CBIOS_NULL;
    PCBIOS_MODE_SRC_PARA    pSrcPara = CBIOS_NULL;
    PCBIOS_MODE_TARGET_PARA pTargetPara = CBIOS_NULL;
    PCBIOS_TIMING_ATTRIB    pTiming = CBIOS_NULL;

    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "===============[ Dump Mode INFO ]===============\n"));

    for (IGAIndex = 0; IGAIndex < pcbe->DispMgr.IgaCount; IGAIndex++)
    {
        pModeParams = pcbe->DispMgr.pModeParams[IGAIndex];

        if(CBIOS_NULL == pModeParams)
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "Mode parameter is NULL\n"));
            return;
        }

        if(CBIOS_TYPE_NONE == pcbe->DispMgr.ActiveDevices[IGAIndex])
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "no device connected on IGA %d\n", IGAIndex));
            continue;
        }

        pSrcPara = &(pModeParams->SrcModePara);
        pTargetPara = &(pModeParams->TargetModePara);
        pTiming = &(pModeParams->TargetTiming);

        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "------------ IGA%d Mode INFO ------------\n", IGAIndex));
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "Source Mode: XRes = %d  YRes = %d\n", pSrcPara->XRes, pSrcPara->YRes));

        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "Target Mode: XRes = %d  YRes = %d  RefRate = %d  bInterlace = %d  AspectRatioFlag = %d  OutputSignal = %d\n",
        pTargetPara->XRes, pTargetPara->YRes, pTargetPara->RefRate, pTargetPara->bInterlace, pTargetPara->AspectRatioFlag, pTargetPara->OutputSignal));


        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "Target Timing: XRes = %d  YRes = %d  RefreshRate = %d  PixelClock = %x  AspectRatio = %x  HVPolarity = %x\n",
        pTiming->XRes, pTiming->YRes, pTiming->RefreshRate, pTiming->PixelClock, pTiming->AspectRatio, pTiming->HVPolarity));
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "Target Timing: HorTotal = %d  HorDisEnd = %d  HorBStart = %d  HorBEnd = %d  HorSyncStart = %d  HorSyncEnd = %d\n",
        pTiming->HorTotal, pTiming->HorDisEnd, pTiming->HorBStart, pTiming->HorBEnd, pTiming->HorSyncStart, pTiming->HorSyncEnd));
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "Target Timing: VerTotal = %d  VerDisEnd = %d  VerBStart = %d  VerBEnd = %d  VerSyncStart = %d  VerSyncEnd = %d\n",
        pTiming->VerTotal, pTiming->VerDisEnd, pTiming->VerBStart, pTiming->VerBEnd, pTiming->VerSyncStart, pTiming->VerSyncEnd));

        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "IsEnable3DVideo: %d  IsxvYCC: %d  IsSingleBuffer: %d\n",
        pModeParams->IsEnable3DVideo, pModeParams->IsxvYCC, pModeParams->IsSingleBuffer));

        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "Video3DStruct: %d  VICCode: %d  PixelRepitition: %d  BitPerComponent: %d\n\n",
        pModeParams->Video3DStruct, pModeParams->VICCode, pModeParams->PixelRepitition, pModeParams->BitPerComponent));
    }

    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "===============[ End Mode INFO ]===============\n\n"));

}

CBIOS_VOID cbDumpClockInfo(PCBIOS_EXTENSION_COMMON pcbe)
{
    CBIOS_U32 ClockFreq = 0;
    CBIOS_U32 RegPMU9038Value;

    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "===============[ Dump Clock INFO ]===============\n"));
    cbGetProgClock(pcbe, &ClockFreq, CBIOS_DCLK1TYPE);
    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "DCLK1: %x\n", ClockFreq));
    cbGetProgClock(pcbe, &ClockFreq, CBIOS_DCLK2TYPE);
    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "DCLK2: %x\n", ClockFreq));
    cbGetProgClock(pcbe, &ClockFreq, CBIOS_ECLKTYPE);
    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "ECLK: %x\n", ClockFreq));
    cbGetProgClock(pcbe, &ClockFreq, CBIOS_CPUFRQTYPE);
    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "CPUFRQ: %x\n", ClockFreq));

    RegPMU9038Value = cbReadRegisterU32(pcbe, CBIOS_REGISTER_PMU, 0x9038);
    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "DIUM_CLK_DIV: %x\n", (RegPMU9038Value & 0x1F)));
    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "===============[ End Clock INFO ]===============\n\n"));
}


CBIOS_BOOL cbDisableHdcp(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 ulHDCPNum)
{
    PCBIOS_VOID pAp = pcbe->pAdapterContext;
    CBIOS_U32 HdcpReg = 0;
    CBIOS_U32 HdcpStatus = 0;
    CBIOS_U32 TmpStatus = 0;
    CBIOS_U32 i, j, maxloop = MAXI2CWAITLOOP * 10;
    CBIOS_BOOL  bRet = CBIOS_TRUE;

    if(ulHDCPNum == HDCP1)
        HdcpReg = HDCPCTL2_DEST;
    else if(ulHDCPNum == HDCP2)
        HdcpReg = HDCP2_CTL2_DEST;

    if(HdcpReg != 0)
    {
        HdcpStatus = cb_ReadU32(pAp, HdcpReg);

        //Software Requests I2C Access
        cb_WriteU32(pAp, HdcpReg, HdcpStatus | HDCP_I2C_REQUEST_DEST);

        i = 0;
        while(i < maxloop)
        {
            j = 0;
            while(j < maxloop)
            {
                TmpStatus = cb_ReadU32(pAp, HdcpReg);
                if(!(TmpStatus & HDCP_I2C_STATUS_DEST)) //wait i2c idle
                    break;

                cb_DelayMicroSeconds(4);
                j++;
            }

            if(j < maxloop)
                break;
            i++;
        }

        //disable HDCP I2C function if take control, else restore bit[1].
        if(i < maxloop)
        {
            cb_WriteU32(pAp, HdcpReg, TmpStatus&(~HDCP_I2C_ENABLE_DEST));
            bRet = CBIOS_TRUE;
        }
        else
        {
            cb_WriteU32(pAp, HdcpReg, TmpStatus&(~HDCP_I2C_REQUEST_DEST));
            bRet = CBIOS_FALSE;
        }
    }

    return bRet;
}

CBIOS_VOID cbEnableHdcpStatus(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 ulHDCPNum)
{
    PCBIOS_VOID pAp = pcbe->pAdapterContext;
    CBIOS_U32 HdcpReg = 0;
    CBIOS_U32 HdcpStatus = 0;

    if(ulHDCPNum == HDCP1)
        HdcpReg = HDCPCTL2_DEST;
    else if(ulHDCPNum == HDCP2)
        HdcpReg = HDCP2_CTL2_DEST;

    if(HdcpReg != 0)
    {
        HdcpStatus = cb_ReadU32(pAp, HdcpReg);
        if(HdcpStatus & HDCP_I2C_ENABLE_DEST)
            return;

        HdcpStatus &= ~HDCP_I2C_REQUEST_DEST;
        HdcpStatus |= HDCP_I2C_ENABLE_DEST;
        cb_WriteU32(pAp, HdcpReg, HdcpStatus);
    }

    return;
}

static CBIOS_BOOL cbHDCPEdidRead(PCBIOS_EXTENSION_COMMON pcbe,
                        CBIOS_U8 I2CAddress,
                        CBIOS_U8 I2CSubAddr,
                        PCBIOS_UCHAR pReadDataBuf,
                        CBIOS_U32 DataLen,
                        CBIOS_S32 HDCPChannel,
                        CBIOS_U8 nSegNum)
{
    CBIOS_BOOL bResult = CBIOS_TRUE;
    CBIOS_U32 i, j;
    CBIOS_U32 maxloop = MAXI2CWAITLOOP;
    PCBIOS_VOID pAp = pcbe->pAdapterContext;
    CBIOS_U8  I2CDELAY = pcbe->I2CDelay;
    CBIOS_U32 HdcpCtrl1Reg, HdcpCtrl2Reg;

    cbTraceEnter(GENERIC);
    if (HDCPChannel == HDCP1)
    {
        HdcpCtrl1Reg = HDCPCTL1_DEST;
        HdcpCtrl2Reg = HDCPCTL2_DEST;
    }
    else if (HDCPChannel == HDCP2)
    {
        HdcpCtrl1Reg = HDCP2_CTL1_DEST;
        HdcpCtrl2Reg = HDCP2_CTL2_DEST;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP channel: %d\n", FUNCTION_NAME, HDCPChannel));
        return CBIOS_FALSE;
    }

    if(nSegNum != 0)
    {
        // Write the extension segment index
        cb_WriteU32(pAp,HdcpCtrl2Reg,
            cb_ReadU32(pAp,HdcpCtrl2Reg) | (HDCP_I2C_START_DEST));//set START&WDATA_AV
        cb_WriteU32(pAp,HdcpCtrl2Reg,
            cb_ReadU32(pAp,HdcpCtrl2Reg) | (HDCP_I2C_WDAV_DEST));

        j = 0;
        while(j < maxloop)//query START & WDATA_AV until they are zero
        {
            if(!(cb_ReadU32(pAp,HdcpCtrl2Reg) & (HDCP_I2C_START_DEST | HDCP_I2C_WDAV_DEST)))
                break;
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        if(j >= maxloop)
            return CBIOS_FALSE;

        cb_WriteU32(pAp,HdcpCtrl1Reg,
            (cb_ReadU32(pAp,HdcpCtrl1Reg)&0xff00ffff)|((CBIOS_U32)(0x60&~1))<<16);//write the I2C data,first byte should be I2C address
        cb_WriteU32(pAp,HdcpCtrl2Reg,
            cb_ReadU32(pAp,HdcpCtrl2Reg) |HDCP_I2C_WDAV_DEST);//set WDATA_AV
        j = 0;
        while(j < maxloop)//query WDATA_AV until they are zero
        {
            if(!(cb_ReadU32(pAp,HdcpCtrl2Reg) & HDCP_I2C_WDAV_DEST))
                break;
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        if(j >= maxloop)
            return CBIOS_FALSE;

       //write segment index
        cb_WriteU32(pAp,HdcpCtrl1Reg,
            (cb_ReadU32(pAp,HdcpCtrl1Reg)&0xff00ffff)|((CBIOS_U32)nSegNum)<<16);//write the I2C data,first byte should be I2C address
        cb_WriteU32(pAp,HdcpCtrl2Reg,
            cb_ReadU32(pAp,HdcpCtrl2Reg) | HDCP_I2C_WDAV_DEST);//set WDATA_AV
        j = 0;
        while(j < maxloop)//query WDATA_AV until they are zero
        {
            if(!(cb_ReadU32(pAp,HdcpCtrl2Reg) & HDCP_I2C_WDAV_DEST))
                break;
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        if(j >= maxloop)
            return CBIOS_FALSE;

        cb_WriteU32(pAp,HdcpCtrl2Reg,
            cb_ReadU32(pAp,HdcpCtrl2Reg) | (HDCP_I2C_STOP_DEST));//set stop & WDATA_AV;HW bug
        cb_WriteU32(pAp,HdcpCtrl2Reg,
            cb_ReadU32(pAp,HdcpCtrl2Reg) | (HDCP_I2C_WDAV_DEST));
        j = 0;
        while(j < maxloop)//query stop until they are zero
        {
            if(!(cb_ReadU32(pAp,HdcpCtrl2Reg) & HDCP_I2C_STOP_DEST))
                break;
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        if(j >= maxloop)
            return CBIOS_FALSE;
    }
        // write device addr
    cb_WriteU32(pAp,HdcpCtrl2Reg,cb_ReadU32(pAp,HdcpCtrl2Reg) |
                    (HDCP_I2C_START_DEST));//set START & WDATA_AV
    cb_WriteU32(pAp,HdcpCtrl2Reg,cb_ReadU32(pAp,HdcpCtrl2Reg) |
                    (HDCP_I2C_WDAV_DEST));
    j = 0;
    while(j < maxloop)//query START & WDATA_AV until they are zero
    {
        if(!(cb_ReadU32(pAp,HdcpCtrl2Reg) & (HDCP_I2C_START_DEST | HDCP_I2C_WDAV_DEST)))
            break;
        cb_DelayMicroSeconds(I2CDELAY);
        j++;
    }
    if(j >= maxloop)
        return CBIOS_FALSE;

    cb_WriteU32(pAp,HdcpCtrl1Reg, (cb_ReadU32(pAp,HdcpCtrl1Reg)&0xff00ffff)|((CBIOS_U32)I2CAddress)<<16);//write the I2C data,first byte should be I2C address
    cb_WriteU32(pAp,HdcpCtrl2Reg,cb_ReadU32(pAp,HdcpCtrl2Reg) |
                                    HDCP_I2C_WDAV_DEST);//set WDATA_AV
    j = 0;
    while(j < maxloop)//query WDATA_AV until they are zero
    {
        if(!(cb_ReadU32(pAp,HdcpCtrl2Reg) & HDCP_I2C_WDAV_DEST))
            break;
        cb_DelayMicroSeconds(I2CDELAY);
        j++;
    }
    if(j >= maxloop)
        return CBIOS_FALSE;

    //write sub addr
    cb_WriteU32(pAp,HdcpCtrl1Reg, (cb_ReadU32(pAp,HdcpCtrl1Reg)&0xff00ffff)|((CBIOS_U32)I2CSubAddr)<<16);//write the I2C data,first byte should be I2C address
    cb_WriteU32(pAp,HdcpCtrl2Reg,cb_ReadU32(pAp,HdcpCtrl2Reg) |  HDCP_I2C_WDAV_DEST);//set WDATA_AV

    j = 0;
    while(j < maxloop)//query WDATA_AV until they are zero
    {
        if(!(cb_ReadU32(pAp,HdcpCtrl2Reg) & HDCP_I2C_WDAV_DEST))
            break;
        cb_DelayMicroSeconds(I2CDELAY);
        j++;
    }
    if(j >= maxloop)
        return CBIOS_FALSE;

    cb_WriteU32(pAp,HdcpCtrl2Reg,cb_ReadU32(pAp,HdcpCtrl2Reg) |
                    (HDCP_I2C_STOP_DEST));//set stop & WDATA_AV;HW bug
    cb_WriteU32(pAp,HdcpCtrl2Reg,cb_ReadU32(pAp,HdcpCtrl2Reg) |
                    (HDCP_I2C_WDAV_DEST));
    j = 0;
    while(j < maxloop)//query stop until they are zero
    {
        if(!(cb_ReadU32(pAp,HdcpCtrl2Reg) & HDCP_I2C_STOP_DEST))
            break;
        cb_DelayMicroSeconds(I2CDELAY);
        j++;
    }
    if(j >= maxloop)
        return CBIOS_FALSE;

    //write device addr  : restart; then write
    cb_WriteU32(pAp,HdcpCtrl2Reg,cb_ReadU32(pAp,HdcpCtrl2Reg) |
                    (HDCP_I2C_START_DEST));//set START & WDATA_AV
    cb_WriteU32(pAp,HdcpCtrl2Reg,cb_ReadU32(pAp,HdcpCtrl2Reg) |
                    (HDCP_I2C_WDAV_DEST));
    j = 0;
    while(j < maxloop)//query START & WDATA_AV until they are zero
    {
        if(!(cb_ReadU32(pAp,HdcpCtrl2Reg) & (HDCP_I2C_START_DEST | HDCP_I2C_WDAV_DEST)))
            break;
        cb_DelayMicroSeconds(I2CDELAY);
        j++;
    }
    if(j >= maxloop)
        return CBIOS_FALSE;

    cb_WriteU32(pAp,HdcpCtrl1Reg,(cb_ReadU32(pAp,HdcpCtrl1Reg)&0xff00ffff)|((CBIOS_U32)(I2CAddress+1)) << 16);//write the I2C address
    cb_WriteU32(pAp,HdcpCtrl2Reg,cb_ReadU32(pAp,HdcpCtrl2Reg) |
                                HDCP_I2C_WDAV_DEST);//set WDATA_AV
    j = 0;
    while(j < maxloop)//query WDATA_AV until they are zero
    {
        if(!(cb_ReadU32(pAp,HdcpCtrl2Reg) & HDCP_I2C_WDAV_DEST))
            break;
        cb_DelayMicroSeconds(I2CDELAY);
        j++;
    }
    if(j >= maxloop)
        return CBIOS_FALSE;


    for(i = 0;i < DataLen;i++)
    {
        //cbWriteU32(pAp,HDCPCTL2_DEST,cb_ReadU32(pAp,HDCPCTL2_DEST) |
                                  // HDCP_I2C_RDREQ_DEST);//set RDREQ
        cb_WriteU32(pAp,HdcpCtrl2Reg,cb_ReadU32(pAp,HdcpCtrl2Reg) |
                                HDCP_I2C_WDAV_DEST);//set WDATA_AV
        j = 0;
        while(j < maxloop)//query WDATA_AV until they are zero
        {
            if(!(cb_ReadU32(pAp,HdcpCtrl2Reg) & HDCP_I2C_WDAV_DEST))
                break;
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        if(j >= maxloop)
            return CBIOS_FALSE;

        j = 0;
        while(j < maxloop)//query RDATA_AV until they are not zero
        {
            if(cb_ReadU32(pAp,HdcpCtrl2Reg) & HDCP_I2C_RDAV_DEST)
                break;
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        if(j >= maxloop)
            return CBIOS_FALSE;

        *pReadDataBuf++ = (CBIOS_UCHAR)((cb_ReadU32(pAp,HdcpCtrl1Reg) & 0x0000ff00) >> 8);//read the I2C data

        cb_WriteU32(pAp,HdcpCtrl2Reg,cb_ReadU32(pAp,HdcpCtrl2Reg) &~
                                    HDCP_I2C_RDAV_DEST);//clear RDATA_AV
    }

    cb_WriteU32(pAp,HdcpCtrl2Reg,cb_ReadU32(pAp,HdcpCtrl2Reg) |
                                (HDCP_I2C_STOP_DEST));//set STOP
    cb_WriteU32(pAp,HdcpCtrl2Reg,cb_ReadU32(pAp,HdcpCtrl2Reg) |
                                (HDCP_I2C_WDAV_DEST));

    return bResult;
}

CBIOS_BOOL cbHDCPProxyGetEDID(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U8 EDIDData[], CBIOS_U32 ulReadEdidOffset, CBIOS_U32 ulBufferSize, CBIOS_U32 ulHDCPNum, CBIOS_U8 nSegNum)
{
    CBIOS_U8 edid_address[] = {0xA0,0x00,0xA2,0x20,0xA6,0x20};
    CBIOS_U8 byTemp;
    CBIOS_U32 i = 0;
    CBIOS_BOOL  bFound = CBIOS_FALSE;
    CBIOS_BOOL  bRet = CBIOS_FALSE;

    if(nSegNum == 0)
    {
        for(i= 0; i<3*2;i +=2)
        {
            bRet = cbHDCPEdidRead(pcbe,edid_address[i],0x0,&byTemp,1,ulHDCPNum, nSegNum);
            if(bRet == CBIOS_FALSE)
            {
               cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG),("Can't Read the edid data!\n")));
               return CBIOS_FALSE;
            }

            if(byTemp == edid_address[i+1])
            {
                bFound = CBIOS_TRUE;
                break;
            }
        }
    }
    else
    {
        i = 0;
        bFound = CBIOS_TRUE;
    }

    if (!bFound)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG),("Can't find the edid address!\n")));
        return CBIOS_FALSE;
    }

    bRet=cbHDCPEdidRead(pcbe,edid_address[i],(CBIOS_U8)ulReadEdidOffset,EDIDData,ulBufferSize,ulHDCPNum,nSegNum);
    return bRet;

}

CBIOS_BOOL cbNormalI2cGetEDID(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U8 I2CBUSNum, CBIOS_U8 EDIDData[], CBIOS_U32 ulReadEdidOffset, CBIOS_U32 ulBufferSize, CBIOS_U8 nSegNum)
{
    CBIOS_U8 edid_address[] = {0xA0,0x00,0xA2,0x20,0xA6,0x20};
    CBIOS_U8 byTemp;
    CBIOS_U8 I2CAddress = 0;
    CBIOS_U32 i = 0, j = 0;
    CBIOS_BOOL  bFound = CBIOS_FALSE;
    CBIOS_BOOL  bRet = CBIOS_FALSE;
    CBIOS_BOOL  bStatus = CBIOS_TRUE;
    CBIOS_MODULE_I2C_PARAMS I2CParams;
    cb_memset(&I2CParams, 0, sizeof(CBIOS_MODULE_I2C_PARAMS));

    I2CParams.I2CBusNum = I2CBUSNum;
    I2CParams.ConfigType = CONFIG_I2C_BY_BUSNUM;

    // Sometimes I2C is not stably at begining of start, so edid may be incorrct.
    // Here, we read edid maximum three times if edid sum check failed.
    for(j=1; j<4; j++)
    {
        //First write segment index
        if(nSegNum != 0)
        {
            I2CAddress = edid_address[0];
            bFound = CBIOS_TRUE;
        }
        else if(nSegNum == 0)
        {
            I2CParams.SlaveAddress = 0x60;
            I2CParams.Buffer = &nSegNum;
            I2CParams.BufferLen = 1;
            cbI2CModule_WriteDDCCIData(pcbe, &I2CParams);
            for(i= 0; i<3*2;i +=2)
            {
                I2CParams.SlaveAddress = edid_address[i];
                I2CParams.OffSet = 0x0;
                I2CParams.Buffer = &byTemp;
                I2CParams.BufferLen = 1;
                bRet = cbI2CModule_ReadData(pcbe, &I2CParams);
                if(bRet == CBIOS_FALSE)
                {
                   cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG),("Can't Read the edid data!\n")));
                   bFound = CBIOS_FALSE;
                   break;
                }

                if(byTemp == edid_address[i+1])
                {
                    I2CAddress = edid_address[i];
                    bFound = CBIOS_TRUE;
                    break;
                }
            }
        }
        if (!bFound)
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG),("Can't find the edid address!\n")));
            bStatus = CBIOS_FALSE;
            break;
        }

        //First write segment index
        if(nSegNum != 0)
        {
            I2CParams.SlaveAddress = 0x60;
            I2CParams.Buffer = &nSegNum;
            I2CParams.BufferLen = 1;
            cbI2CModule_WriteDDCCIData(pcbe, &I2CParams);
        }
        I2CParams.SlaveAddress = I2CAddress;
        I2CParams.OffSet = (CBIOS_U8)ulReadEdidOffset;
        I2CParams.BufferLen = ulBufferSize;
        I2CParams.Buffer = EDIDData;
        bRet = cbI2CModule_ReadData(pcbe, &I2CParams);

        if(!bRet)
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG),("Read EDID error!\n")));
            bStatus = CBIOS_FALSE;
            break;
        }

        // Judge if need check the extension block.
        if ((ulBufferSize == 256) && (EDIDData[0x7E] == 0))
            ulBufferSize = ulBufferSize / 2;

        if((ulBufferSize % 128 == 0) && (ulReadEdidOffset == 0))    //full edid
        {
            byTemp = 0;

            for (i = 0 ; i < 128 ; i++)
            {
                byTemp += EDIDData[i];
            }

            if(byTemp == 0)
            {
                if((EDIDData[0] == 0x00)&&(EDIDData[1] == 0xff))
                {
                    cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG),("EDID reading is OK!\n")));
                }
                else if((EDIDData[0]&0xF0) == 0x20)
                {
                    cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG),("EDID reading is OK!\n")));
                }
                else
                {
                    cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING),("EDID reading OK, But the version wrong!\n")));
                }
                bStatus = CBIOS_TRUE;
                break;
            }
            else
            {
                cb_DelayMicroSeconds(j * 50);
                continue;
            }
        }
        else            //part of EDID
        {
            bStatus = CBIOS_TRUE;
            break;
        }
    }

    return bStatus;
}

CBIOS_VOID cbWritePort80(PCBIOS_VOID pvcbe,CBIOS_U8 value)
{

}

CBIOS_BOOL cbNeedSetHdaudioToLocal(PCBIOS_VOID pvcbe)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_BOOL ret = CBIOS_FALSE;

    if(cbMMIOReadReg(pcbe, CR_C_9B) == 0x3D
       && cbMMIOReadReg(pcbe, CR_C_9A) == 0x40)
    {
        ret = CBIOS_TRUE;
    }

    return ret;
}
