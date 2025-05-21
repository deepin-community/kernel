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
#include "../../Hw/CBiosHwShare.h"

extern CBIOS_U8 DSIPanelIndex;

static CBIOS_STATUS cbDSI_WriteCmdRawData(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 RawData, CBIOS_DSI_INDEX DSIIndex)
{
    CBIOS_U32 DataNumInFIFO, i;
    REG_MM3444_MM3574 RegMM3444_MM3574Value;
    REG_MM3404_MM3534 RegMM3404_MM3534Value;
    REG_MM3404_MM3534 RegMM3404_MM3534Mask;
    CBIOS_U32 StatusRegIndex = 0x3444, DataRegIndex = 0x3404;
    CBIOS_STATUS Status;
    cbTraceEnter(DSI);

    if (DSIIndex == DSI_INDEX0)
    {
        StatusRegIndex = 0x3444;
        DataRegIndex   = 0x3404;
    }
    else if (DSIIndex == DSI_INDEX1)
    {
        StatusRegIndex = 0x3574;
        DataRegIndex   = 0x3534;
    }

    for(i = 0; i < 10; i++)
    {
        RegMM3444_MM3574Value.Value = cbReadRegisterU32(pcbe, CBIOS_REGISTER_MMIO, StatusRegIndex);
        DataNumInFIFO = RegMM3444_MM3574Value.DSI_Command_FIFO_Status;

        if (DataNumInFIFO < CBIOS_DSI_CMD_FIFO_DEPTH)
        {
            break;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DSI, WARNING), "cbDSI_WriteCmdRawData: command FIFO full! \n"));
            cb_DelayMicroSeconds(1000);    // just copy 5850 delay time
            continue;
        }
    }

    if (i < 10)
    {
        RegMM3404_MM3534Value.Value = RawData;
        RegMM3404_MM3534Mask.Value = 0;
        cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "DSI: RegIndex: 0x%08x,  Value: 0x%08x \n", DataRegIndex, RegMM3404_MM3534Value.Value));
        cbMMIOWriteReg32(pcbe, DataRegIndex, RegMM3404_MM3534Value.Value, RegMM3404_MM3534Mask.Value);
        Status = CBIOS_OK;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_WriteCmdRawData: write raw data timeout! \n"));
        Status = CBIOS_ER_INTERNAL;
    }

    cbTraceExit(DSI);
    return Status;
}

static CBIOS_STATUS cbDSI_SendShortPacket(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U8 VirtualCh, CBIOS_U8 DataType,
        CBIOS_U8 Data0, CBIOS_U8 Data1, CBIOS_U8 CmdType, CBIOS_DSI_INDEX DSIIndex)
{
    CBIOS_U32     RawData;
    CBIOS_U8      DataIDByte;
    CBIOS_STATUS  Status;
    cbTraceEnter(DSI);

    DataIDByte = (VirtualCh << 6) | DataType;
    RawData = (CBIOS_U32)(DataIDByte | (Data0 << 8) | (Data1 << 16) | (CmdType << 24));

    Status = cbDSI_WriteCmdRawData(pcbe, RawData, DSIIndex);

    cbTraceExit(DSI);
    return Status;
}

static CBIOS_STATUS cbDSI_SendLongPacket(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U8 VirtualCh, CBIOS_U8 DataType,
        CBIOS_U16 WordCount, CBIOS_U8 *pDataBuf, CBIOS_U8 CmdType, CBIOS_DSI_INDEX DSIIndex)
{
    CBIOS_U32     RawData, i, Reminder;
    CBIOS_U8      DataIDByte, LowByte, HighByte;
    CBIOS_STATUS  Status;
    cbTraceEnter(DSI);

    // send packet header first
    DataIDByte = (VirtualCh << 6) | DataType;
    LowByte = (CBIOS_U8)(WordCount & 0xFF);    // LSB first
    HighByte = (CBIOS_U8)((WordCount & 0xFF00) >> 8);
    RawData = (CBIOS_U32)(DataIDByte | (LowByte << 8) | (HighByte << 16) | (CmdType << 24));
    Status = cbDSI_WriteCmdRawData(pcbe, RawData, DSIIndex);

    if (Status != CBIOS_OK)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SendLongPacket: send packet header failed! \n"));
        goto Exit;
    }

    // send packet data
    for (i = 0; i < (CBIOS_U32)(WordCount >> 2); i++)
    {
        RawData = (CBIOS_U32)(pDataBuf[i * 4]) | (CBIOS_U32)(pDataBuf[i * 4 + 1] << 8)
                  | (CBIOS_U32)(pDataBuf[i * 4 + 2] << 16) | (CBIOS_U32)(pDataBuf[i * 4 + 3] << 24);
        Status = cbDSI_WriteCmdRawData(pcbe, RawData, DSIIndex);

        if (Status != CBIOS_OK)
        {
            cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SendLongPacket: send packet data failed! \n"));
            goto Exit;
        }
    }

    Reminder = WordCount % 4;

    if (Reminder != 0)
    {
        RawData = 0;

        for (i = Reminder; i > 0; i--)
        {
            RawData |= (CBIOS_U32)((pDataBuf[WordCount - i]) << ((Reminder - i) * 8));
        }

        Status = cbDSI_WriteCmdRawData(pcbe, RawData, DSIIndex);

        if (Status != CBIOS_OK)
        {
            cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SendLongPacket: send packet data failed! \n"));
            goto Exit;
        }
    }

Exit:
    cbTraceExit(DSI);
    return Status;

}

static CBIOS_STATUS cbDSI_SendDMAPacket(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U8 VirtualCh, CBIOS_U8 DataType,
                                        CBIOS_U16 WordCount,  CBIOS_U8 CmdType, CBIOS_U32  DMAAddr, CBIOS_U8 DCSCommand, CBIOS_DSI_INDEX DSIIndex)
{
    CBIOS_U32     RawData = 0;
    CBIOS_U8      DataIDByte = 0, LowByte = 0, HighByte = 0;
    CBIOS_STATUS  Status;
    cbTraceEnter(DSI);

    // send packet header first
    DataIDByte = (VirtualCh << 6) | DataType;
    LowByte = (CBIOS_U8)(WordCount & 0xFF);    // LSB first
    HighByte = (CBIOS_U8)((WordCount & 0xFF00) >> 8);
    CmdType |= DSI_CMD_TYPE_DMA;
    RawData  = (CBIOS_U32)(DataIDByte | (LowByte << 8) | (HighByte << 16) | (CmdType << 24));
    Status   = cbDSI_WriteCmdRawData(pcbe, RawData, DSIIndex);

    if (Status != CBIOS_OK)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SendDMAPacket: send packet header failed! \n"));
        goto Exit;
    }

    // send dma data address
    Status = cbDSI_WriteCmdRawData(pcbe, DMAAddr, DSIIndex);

    if (Status != CBIOS_OK)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SendDMAPacket:  send dma data address failed! \n"));
        goto Exit;
    }

    //send dcs command
    Status = cbDSI_WriteCmdRawData(pcbe, DCSCommand, DSIIndex);

    if (Status != CBIOS_OK)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SendLongPacket: send dcs command failed! \n"));
        goto Exit;
    }

Exit:
    cbTraceExit(DSI);
    return Status;
}

static CBIOS_BOOL cbDSI_WaitDMAFree(PCBIOS_EXTENSION_COMMON pcbe)
{
    CBIOS_U32    busy = 1;
    CBIOS_U32    i = 10000;
    REG_MM3444_MM3574  RegMM3444_MM3574Value;
    cbTraceEnter(DSI);

    while(i--)
    {
        RegMM3444_MM3574Value.Value =  cbReadRegisterU32(pcbe, CBIOS_REGISTER_MMIO, 0x3444);
        busy = RegMM3444_MM3574Value.DSI_Command_DMA_Busy_Status;

        if(!busy)
        {
            break;
        }
    }

    cbTraceExit(DSI);
    return busy == 0 ? CBIOS_TRUE : CBIOS_FALSE;
}

static CBIOS_VOID cbDSI_SelectMode(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_DSI_MODE Mode, CBIOS_DSI_INDEX DSIIndex)
{
    REG_MM3408_MM3538 RegMM3408_MM3538Value;
    REG_MM3408_MM3538 RegMM3408_MM3538Mask;
    CBIOS_U32 CtrlRegIndex = 0x3408;
    cbTraceEnter(DSI);

    if (DSIIndex == DSI_INDEX0)
    {
        CtrlRegIndex = 0x3408;
    }
    else if (DSIIndex == DSI_INDEX1)
    {
        CtrlRegIndex = 0x3538;
    }

    RegMM3408_MM3538Value.Value = 0;
    RegMM3408_MM3538Mask.Value = 0xFFFFFFFF;
    RegMM3408_MM3538Value.DSI_Mode = (Mode == CBIOS_DSI_CMDMODE) ? 1 : 0;
    RegMM3408_MM3538Mask.DSI_Mode = 0;
    cbMMIOWriteReg32(pcbe, CtrlRegIndex, RegMM3408_MM3538Value.Value, RegMM3408_MM3538Mask.Value);

    cbTraceExit(DSI);
}

static CBIOS_VOID cbDSI_SetOutBpp(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 OutBpp, CBIOS_DSI_INDEX DSIIndex)
{
    CBIOS_U32   val = 0;
    REG_MM3408_MM3538 RegMM3408_MM3538Value;
    REG_MM3408_MM3538 RegMM3408_MM3538Mask;
    CBIOS_U32 CtrlRegIndex = 0x3408;
    cbTraceEnter(DSI);

    if (DSIIndex == DSI_INDEX0)
    {
        CtrlRegIndex = 0x3408;
    }
    else if (DSIIndex == DSI_INDEX1)
    {
        CtrlRegIndex = 0x3538;
    }

    if (OutBpp == 16)
    {
        val = 1;
    }
    else if (OutBpp == 18)
    {
        val = 2;
    }
    else if (OutBpp == 24)
    {
        val = 0;
    }

    RegMM3408_MM3538Value.Value = 0;
    RegMM3408_MM3538Mask.Value = 0xFFFFFFFF;
    RegMM3408_MM3538Value.DSI_Video_Mode_Ouput_Pixel_Format = val;
    RegMM3408_MM3538Mask.DSI_Video_Mode_Ouput_Pixel_Format = 0;
    cbMMIOWriteReg32(pcbe, CtrlRegIndex, RegMM3408_MM3538Value.Value, RegMM3408_MM3538Mask.Value);

    cbTraceExit(DSI);
}

static CBIOS_VOID cbDSI_EoTpEnableDisable(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DSI_CONFIG pDSIConfig, CBIOS_DSI_INDEX DSIIndex)
{
    REG_MM3408_MM3538 RegMM3408_MM3538Value;
    REG_MM3408_MM3538 RegMM3408_MM3538Mask;
    REG_MM345C_MM358C RegMM345C_MM358CValue;
    REG_MM345C_MM358C RegMM345C_MM358CMask;
    CBIOS_U32 CtrlRegIndex = 0x3408, VideoRegIndex = 0x345C;
    cbTraceEnter(DSI);

    if (DSIIndex == DSI_INDEX0)
    {
        CtrlRegIndex  = 0x3408;
        VideoRegIndex = 0x345C;
    }
    else if (DSIIndex == DSI_INDEX1)
    {
        CtrlRegIndex  = 0x3538;
        VideoRegIndex = 0x358C;
    }

    RegMM3408_MM3538Value.Value = 0;
    RegMM3408_MM3538Mask.Value = 0xFFFFFFFF;
    RegMM345C_MM358CValue.Value = 0;
    RegMM345C_MM358CMask.Value = 0xFFFFFFFF;

    if (pDSIConfig->DSIMode == CBIOS_DSI_CMDMODE)
    {
        if (pDSIConfig->isEnableEoTp)
        {
            RegMM3408_MM3538Value.Enable_Command_EoTp_End_of_Transmission_packet = 1;
        }
        else
        {
            RegMM3408_MM3538Value.Enable_Command_EoTp_End_of_Transmission_packet = 0;
        }

        RegMM3408_MM3538Mask.Enable_Command_EoTp_End_of_Transmission_packet = 0;
        cbMMIOWriteReg32(pcbe, CtrlRegIndex, RegMM3408_MM3538Value.Value, RegMM3408_MM3538Mask.Value);
    }
    else if (pDSIConfig->DSIMode == CBIOS_DSI_VIDEOMODE)
    {
        if (pDSIConfig->isEnableEoTp)
        {
            RegMM345C_MM358CValue.DSI_Video_Mode_EoTp_Packet_Enable = 1;
        }
        else
        {
            RegMM345C_MM358CValue.DSI_Video_Mode_EoTp_Packet_Enable = 0;
        }

        RegMM345C_MM358CMask.DSI_Video_Mode_EoTp_Packet_Enable = 0;
        cbMMIOWriteReg32(pcbe, VideoRegIndex, RegMM345C_MM358CValue.Value, RegMM345C_MM358CMask.Value);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_EoTpEnableDisable: wrong dsi mode! \n"));
    }

    cbTraceExit(DSI);
}

static CBIOS_VOID cbDSI_SetDataLaneNum(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 DataLaneNum, CBIOS_DSI_INDEX DSIIndex)
{
    REG_MM340C_MM353C RegMM340C_MM353CValue;
    REG_MM340C_MM353C RegMM340C_MM353CMask;
    CBIOS_U32 PhyRegIndex = 0x340C;
    cbTraceEnter(DSI);

    if (DSIIndex == DSI_INDEX0)
    {
        PhyRegIndex = 0x340C;
    }
    else if (DSIIndex == DSI_INDEX1)
    {
        PhyRegIndex = 0x353C;
    }


    RegMM340C_MM353CValue.Value = 0;
    RegMM340C_MM353CMask.Value = 0xFFFFFFFF;
    RegMM340C_MM353CValue.DSI_Data_Lane_Number = (DataLaneNum - 1);
    RegMM340C_MM353CMask.DSI_Data_Lane_Number = 0;
    cbMMIOWriteReg32(pcbe, PhyRegIndex, RegMM340C_MM353CValue.Value, RegMM340C_MM353CMask.Value);

    cbTraceExit(DSI);
}

static CBIOS_VOID cbDSI_SetClkLaneMode(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_DSI_CLK_LANE_MODE ClkLaneMode, CBIOS_DSI_INDEX DSIIndex)
{
    REG_MM340C_MM353C RegMM340C_MM353CValue;
    REG_MM340C_MM353C RegMM340C_MM353CMask;
    CBIOS_U32 PhyRegIndex = 0x340C;
    cbTraceEnter(DSI);

    if (DSIIndex == DSI_INDEX0)
    {
        PhyRegIndex = 0x340C;
    }
    else if (DSIIndex == DSI_INDEX1)
    {
        PhyRegIndex = 0x353C;
    }

    RegMM340C_MM353CValue.Value = 0;
    RegMM340C_MM353CMask.Value = 0xFFFFFFFF;

    if (ClkLaneMode == CBIOS_DSI_CLK_LANE_SOFTWARE_CTL)
    {
        RegMM340C_MM353CValue.Clock_Lane_Control_Mode = 0;
    }
    else
    {
        RegMM340C_MM353CValue.Clock_Lane_Control_Mode = 1;
    }

    RegMM340C_MM353CMask.Clock_Lane_Control_Mode = 0;
    cbMMIOWriteReg32(pcbe, PhyRegIndex, RegMM340C_MM353CValue.Value, RegMM340C_MM353CMask.Value);

    cbTraceExit(DSI);
}

static CBIOS_VOID cbDSI_ClkLaneHSEnableDisable(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_BOOL isEnable, CBIOS_DSI_INDEX DSIIndex)
{
    REG_MM3410_MM3540 RegMM3410_MM3540Value;
    REG_MM3410_MM3540 RegMM3410_MM3540Mask;
    CBIOS_U32 LPhyRegIndex = 0x3410;
    cbTraceEnter(DSI);

    if (DSIIndex == DSI_INDEX0)
    {
        LPhyRegIndex = 0x3410;
    }
    else if (DSIIndex == DSI_INDEX1)
    {
        LPhyRegIndex = 0x3540;
    }

    RegMM3410_MM3540Value.Value = 0;
    RegMM3410_MM3540Mask.Value = 0xFFFFFFFF;

    if (isEnable)
    {
        RegMM3410_MM3540Value.Enable_Clock_lane_high_speed_clock = 1;
    }
    else
    {
        RegMM3410_MM3540Value.Enable_Clock_lane_high_speed_clock = 0;
    }

    RegMM3410_MM3540Mask.Enable_Clock_lane_high_speed_clock = 0;
    cbMMIOWriteReg32(pcbe, LPhyRegIndex, RegMM3410_MM3540Value.Value, RegMM3410_MM3540Mask.Value);

    cbTraceExit(DSI);
}

static CBIOS_VOID cbDSI_SyncEndEnableDisable(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DSI_CONFIG pDSIConfig, CBIOS_DSI_INDEX DSIIndex)
{
    REG_MM345C_MM358C RegMM345C_MM358CValue;
    REG_MM345C_MM358C RegMM345C_MM358CMask;
    CBIOS_U32 VideoRegIndex = 0x345C;
    cbTraceEnter(DSI);

    if (DSIIndex == DSI_INDEX0)
    {
        VideoRegIndex = 0x345C;
    }
    else if (DSIIndex == DSI_INDEX1)
    {
        VideoRegIndex = 0x358C;
    }

    RegMM345C_MM358CValue.Value = 0;
    RegMM345C_MM358CMask.Value = 0xFFFFFFFF;

    RegMM345C_MM358CValue.HSS_EN = 1;
    RegMM345C_MM358CValue.VSS_EN = 1;

    if (pDSIConfig->SyncPacketType == CBIOS_DSI_SYNC_PULSE)
    {
        RegMM345C_MM358CValue.HSE_EN = 1;
        RegMM345C_MM358CValue.VSE_EN = 1;
    }
    else
    {
        RegMM345C_MM358CValue.HSE_EN = 0;
        RegMM345C_MM358CValue.VSE_EN = 0;
    }

    RegMM345C_MM358CMask.HSS_EN = 0;
    RegMM345C_MM358CMask.HSE_EN = 0;
    RegMM345C_MM358CMask.VSS_EN = 0;
    RegMM345C_MM358CMask.VSE_EN = 0;
    cbMMIOWriteReg32(pcbe, VideoRegIndex, RegMM345C_MM358CValue.Value, RegMM345C_MM358CMask.Value);

    cbTraceExit(DSI);
}

static CBIOS_VOID cbDSI_SetTimeOut(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DSI_CONFIG pDSIConfig, CBIOS_DSI_INDEX DSIIndex)
{
    REG_MM342C_MM355C RegMM342C_MM355CValue;
    REG_MM342C_MM355C RegMM342C_MM355CMask;
    REG_MM3430_MM3560 RegMM3430_MM3560Value;
    REG_MM3430_MM3560 RegMM3430_MM3560Mask;
    REG_MM3434_MM3564 RegMM3434_MM3564Value;
    REG_MM3434_MM3564 RegMM3434_MM3564Mask;
    CBIOS_U32 TARegIndex = 0x342C, HsTxRegIndex = 0x3430, LpRxRegIndex = 0x3434;
    cbTraceEnter(DSI);

    if (DSIIndex == DSI_INDEX0)
    {
        TARegIndex   = 0x342C;
        HsTxRegIndex = 0x3430;
        LpRxRegIndex = 0x3434;
    }
    else if (DSIIndex == DSI_INDEX1)
    {
        TARegIndex   = 0x355C;
        HsTxRegIndex = 0x3560;
        LpRxRegIndex = 0x3564;
    }


    RegMM342C_MM355CValue.DSI_TurnAround_Timeout_Period = pDSIConfig->TurnAroundTimeout;
    RegMM342C_MM355CMask.DSI_TurnAround_Timeout_Period = 0;
    RegMM3430_MM3560Value.DSI_HS_TX_Timeout_Period = pDSIConfig->HS_TXTimeout;
    RegMM3430_MM3560Mask.DSI_HS_TX_Timeout_Period = 0;
    RegMM3434_MM3564Value.DSI_LP_RX_Timeout_Period = pDSIConfig->LP_RXTimeout;
    RegMM3434_MM3564Mask.DSI_LP_RX_Timeout_Period = 0;

    cbMMIOWriteReg32(pcbe, TARegIndex, RegMM342C_MM355CValue.Value, RegMM342C_MM355CMask.Value);
    cbMMIOWriteReg32(pcbe, HsTxRegIndex, RegMM3430_MM3560Value.Value, RegMM3430_MM3560Mask.Value);
    cbMMIOWriteReg32(pcbe, LpRxRegIndex, RegMM3434_MM3564Value.Value, RegMM3434_MM3564Mask.Value);

    cbTraceExit(DSI);
}

static CBIOS_VOID cbDSI_SetPhyParams(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DSI_PARAMS pDSIParams, CBIOS_U32 IGAIndex, CBIOS_DSI_INDEX DSIIndex)
{
    PCBIOS_DSI_PHY_PARAMS pPhyParams = &pDSIParams->PhyParams;
    PCBIOS_DSI_CONFIG pDSIConfig = &(pDSIParams->DSIPanelDesc.DSIConfig);
    PCBIOS_DSI_PANEL_TABLE pDSIPanelTbl = &(pDSIParams->DSIPanelDesc.DSIPanelTbl);
    CBIOS_U32 tDclk, tBclk, tEclk, tUI;
    CBIOS_S32 value;
    REG_MM3414_MM3544 RegMM3414_MM3544Value;
    REG_MM3414_MM3544 RegMM3414_MM3544Mask;
    REG_MM3418_MM3548 RegMM3418_MM3548Value;
    REG_MM3418_MM3548 RegMM3418_MM3548Mask;
    REG_MM341C_MM354C RegMM341C_MM354CValue;
    REG_MM341C_MM354C RegMM341C_MM354CMask;
    REG_MM3420_MM3550 RegMM3420_MM3550Value;
    REG_MM3420_MM3550 RegMM3420_MM3550Mask;
    REG_MM3424_MM3554 RegMM3424_MM3554Value;
    REG_MM3424_MM3554 RegMM3424_MM3554Mask;
    REG_MM3428_MM3558 RegMM3428_MM3558Value;
    REG_MM3428_MM3558 RegMM3428_MM3558Mask;
    REG_MM3448_MM3578 RegMM3448_MM3578Value;
    REG_MM3448_MM3578 RegMM3448_MM3578Mask;
    CBIOS_U32 LpxTaRegIndex = 0x3414, GoSureRegIndex = 0x3418, WakeUpRegIndex = 0x341C;
    CBIOS_U32 PreZeroRegIndex = 0x3420, TrailRegIndex = 0x3424, ExitRegIndex = 0x3428;
    CBIOS_U32 EphyCtrlRegIndex = 0x3448, EphyTunRegIndex = 0x344C, EphyMiscRegIndex = 0x3450;
    cbTraceEnter(DSI);

    cb_memset(pPhyParams, 0 , sizeof(CBIOS_DSI_PHY_PARAMS));

    if (DSIIndex == DSI_INDEX0)
    {
        LpxTaRegIndex   = 0x3414;
        GoSureRegIndex  = 0x3418;
        WakeUpRegIndex  = 0x341C;
        PreZeroRegIndex = 0x3420;
        TrailRegIndex   = 0x3424;
        ExitRegIndex    = 0x3428;
        EphyCtrlRegIndex    = 0x3448;
        EphyTunRegIndex     = 0x344C;
        EphyMiscRegIndex    = 0x3450;
    }
    else if (DSIIndex == DSI_INDEX1)
    {
        LpxTaRegIndex   = 0x3544;
        GoSureRegIndex  = 0x3548;
        WakeUpRegIndex  = 0x354C;
        PreZeroRegIndex = 0x3550;
        TrailRegIndex   = 0x3554;
        ExitRegIndex    = 0x3558;
        EphyCtrlRegIndex    = 0x3578;
        EphyTunRegIndex     = 0x357C;
        EphyMiscRegIndex    = 0x3580;
    }

    tDclk = khz2ps(pDSIPanelTbl->PanelTiming.DCLK * 10);    // unit: ps, 1ns = 1000ps
    if (pDSIConfig->isDualChannel)
    {
        tDclk *= 2;
    }
    tEclk = khz2ps(1700000 / 10);    // unit: ps, 1ns = 1000ps
    tBclk = tDclk * pDSIPanelTbl->LaneNum * 8 / pDSIPanelTbl->OutBpp;
    tUI = tBclk / 8;
    cbDebugPrint((MAKE_LEVEL(DSI, INFO), "cbDSI_SetPhyParams, pDSIPanelTbl->PanelTiming.DCLK:  %d   \n", pDSIPanelTbl->PanelTiming.DCLK));
    cbDebugPrint((MAKE_LEVEL(DSI, INFO), "cbDSI_SetPhyParams, tDclk:  %d   \n", tDclk));
    cbDebugPrint((MAKE_LEVEL(DSI, INFO), "cbDSI_SetPhyParams, tEclk:  %d   \n", tEclk));
    cbDebugPrint((MAKE_LEVEL(DSI, INFO), "cbDSI_SetPhyParams, tBclk:  %d   \n", tBclk));
    cbDebugPrint((MAKE_LEVEL(DSI, INFO), "cbDSI_SetPhyParams,   tUI:  %d   \n", tUI));

    pPhyParams->LPX = 1000 * (pPhyParams->LPX ? pPhyParams->LPX : 60);
    pPhyParams->HS_PREPARE = pPhyParams->HS_PREPARE ? (1000 * pPhyParams->HS_PREPARE) : (1000 * 50 + 6 * tUI);
    pPhyParams->HS_PREPARE_ZERO = pPhyParams->HS_PREPARE_ZERO ? (1000 * pPhyParams->HS_PREPARE_ZERO) : (1000 * 145 + 10 * tUI);
    pPhyParams->HS_TRAIL = pPhyParams->HS_TRAIL ? (1000 * pPhyParams->HS_TRAIL) : (cb_max(8 * tUI, 1000 * 60 + 4 * tUI));
    pPhyParams->HS_EXIT = 1000 * (pPhyParams->HS_EXIT ? pPhyParams->HS_EXIT : 110);
    pPhyParams->CLK_PREPARE = 1000 * (pPhyParams->CLK_PREPARE ? pPhyParams->CLK_PREPARE : 50);
    pPhyParams->CLK_PREPARE_ZERO = 1000 * (pPhyParams->CLK_PREPARE_ZERO ? pPhyParams->CLK_PREPARE_ZERO : 350);
    pPhyParams->CLK_POST = pPhyParams->CLK_POST ? (1000 * pPhyParams->CLK_POST) : (1000 * 70 + 52 * tUI);
    pPhyParams->CLK_PRE = pPhyParams->CLK_PRE ? (1000 * pPhyParams->CLK_PRE) : (8 * tUI);
    pPhyParams->CLK_TRAIL = 1000 * (pPhyParams->CLK_TRAIL ? pPhyParams->CLK_TRAIL : 70);
    pPhyParams->TA_GET = pPhyParams->TA_GET ? (1000 * pPhyParams->TA_GET) : (5 * pPhyParams->LPX);
    pPhyParams->TA_GO = pPhyParams->TA_GO ? (1000 * pPhyParams->TA_GO) : (4 * pPhyParams->LPX);
    pPhyParams->TA_SURE = pPhyParams->TA_SURE ? (1000 * pPhyParams->TA_SURE) : (pPhyParams->LPX);
    pPhyParams->WakeUp = 1000 * (pPhyParams->WakeUp ? pPhyParams->WakeUp : 110000);

    //LPHY_LPX_and_TA-GET_Period_Register
    value = ceil(pPhyParams->LPX, tBclk) - 1;
    RegMM3414_MM3544Value.LPX_Period = value < 0 ? 0 : value;
    pPhyParams->LPX = (value + 1) * tBclk;
    pPhyParams->LPX = ceil(pPhyParams->LPX , 1000);

    value = ceil(2 * pPhyParams->TA_GET - 9 * tBclk , 2 * tEclk) - 1;
    RegMM3414_MM3544Value.TA_GET_Period = value < 0 ? 0 : value;
    pPhyParams->TA_GET = 2 * (value + 1) * tEclk + 9 * tBclk;
    pPhyParams->TA_GET = ceil(pPhyParams->TA_GET, 2000);
    RegMM3414_MM3544Mask.Value = 0;
    cbMMIOWriteReg32(pcbe, LpxTaRegIndex, RegMM3414_MM3544Value.Value, RegMM3414_MM3544Mask.Value);

    //LPHY_TA-GO_and_TA-SURE_Period_Register
    value = ceil(2 * pPhyParams->TA_GO - 3 * tBclk , 2 * tEclk) - 2;
    RegMM3418_MM3548Value.TA_GO_Period = value < 0 ? 0 : value;
    pPhyParams->TA_GO = 2 * (value + 5) * tEclk + 3 * tBclk;
    pPhyParams->TA_GO = ceil(pPhyParams->TA_GO, 2000);

    value = ceil(pPhyParams->TA_SURE, tEclk) - 4;
    RegMM3418_MM3548Value.TA_SURE_Period = value < 0 ? 0 : value;
    pPhyParams->TA_SURE = (value + 4) * tEclk;
    pPhyParams->TA_SURE = ceil(pPhyParams->TA_SURE, 1000);
    RegMM3418_MM3548Mask.Value = 0;
    cbMMIOWriteReg32(pcbe, GoSureRegIndex, RegMM3418_MM3548Value.Value, RegMM3418_MM3548Mask.Value);

    //LPHY_Wake_Up_Period_Register
    value = ceil(pPhyParams->WakeUp, tBclk) - 1;
    RegMM341C_MM354CValue.Wake_Up_Period = value < 0 ? 0 : value;
    pPhyParams->WakeUp = (value + 1) * tBclk;
    pPhyParams->WakeUp = ceil(pPhyParams->WakeUp, 1000);
    RegMM341C_MM354CMask.Value = 0;
    cbMMIOWriteReg32(pcbe, WakeUpRegIndex, RegMM341C_MM354CValue.Value, RegMM341C_MM354CMask.Value);

    //LPHY_Data/Clock_Lane_HS_Prepare_Period_Register
    value = ceil(pPhyParams->HS_PREPARE_ZERO, tBclk) - 3;
    RegMM3420_MM3550Value.HS_PREPARE_and_HS_ZERO_Period = value < 0 ? 0 : value;
    pPhyParams->HS_PREPARE_ZERO = (value + 3) * tBclk;
    pPhyParams->HS_PREPARE_ZERO = ceil(pPhyParams->HS_PREPARE_ZERO, 1000);

    value = ceil(pPhyParams->CLK_PREPARE_ZERO, tBclk) - 1;
    RegMM3420_MM3550Value.CLK_PREPARE_and_CLK_ZERO_Period = value < 0 ? 0 : value;
    pPhyParams->CLK_PREPARE_ZERO = (value + 3) * tBclk;
    pPhyParams->CLK_PREPARE_ZERO = ceil(pPhyParams->CLK_PREPARE_ZERO, 1000);
    RegMM3420_MM3550Mask.Value = 0;
    cbMMIOWriteReg32(pcbe, PreZeroRegIndex, RegMM3420_MM3550Value.Value, RegMM3420_MM3550Mask.Value);

    //LPHY_Data/Clock_Lane_HS_Trail_Period_Register
    value = ceil(pPhyParams->HS_TRAIL, tBclk) - 1;
    RegMM3424_MM3554Value.HS_TRAIL_Period = value < 0 ? 0 : value;
    pPhyParams->HS_TRAIL = (value + 1) * tBclk;
    pPhyParams->HS_TRAIL = ceil(pPhyParams->HS_TRAIL, 1000);

    value = ceil(pPhyParams->CLK_TRAIL, tBclk) - 1;
    RegMM3424_MM3554Value.CLK_TRAIL_Period = value < 0 ? 0 : value;
    pPhyParams->CLK_TRAIL = (value + 1) * tBclk;
    pPhyParams->CLK_TRAIL = ceil(pPhyParams->CLK_TRAIL, 1000);

    value = ceil(pPhyParams->CLK_POST, tBclk) - 1;
    RegMM3424_MM3554Value.CLK_POST_Period = value < 0 ? 0 : value;
    pPhyParams->CLK_POST = (value + 1) * tBclk;
    pPhyParams->CLK_POST = ceil(pPhyParams->CLK_POST, 1000);

    value = ceil(pPhyParams->CLK_PRE, tBclk) - 1;
    RegMM3424_MM3554Value.CLK_PRE_Period = value < 0 ? 0 : value;
    pPhyParams->CLK_PRE = (value + 1) * tBclk;
    pPhyParams->CLK_PRE = ceil(pPhyParams->CLK_PRE, 1000);

    RegMM3424_MM3554Mask.Value = 0;
    cbMMIOWriteReg32(pcbe, TrailRegIndex, RegMM3424_MM3554Value.Value, RegMM3424_MM3554Mask.Value);

    //LPHY_HS-EXIT_Period_Register
    value = ceil(pPhyParams->HS_EXIT, tBclk) - 1;
    RegMM3428_MM3558Value.Value = 0;
    RegMM3428_MM3558Value.HS_EXIT_Period = value < 0 ? 0 : value;
    pPhyParams->HS_EXIT = (value + 1) * tBclk;
    pPhyParams->HS_EXIT = ceil(pPhyParams->HS_EXIT, 1000);
    RegMM3428_MM3558Mask.Value = 0xFFFFFFFF;
    RegMM3428_MM3558Mask.HS_EXIT_Period = 0;
    cbMMIOWriteReg32(pcbe, ExitRegIndex, RegMM3428_MM3558Value.Value, RegMM3428_MM3558Mask.Value);

    //set MM342C, MM3430, MM3434
    //TODO

    //DSI_EPHY_Control_Register
    RegMM3448_MM3578Value.Value = 0;
    RegMM3448_MM3578Mask.Value = 0xFFFFFFFF;
    value = ceil(pPhyParams->HS_PREPARE, (2 * tUI));
    RegMM3448_MM3578Value.THS_PREPARE = value < 0 ? 0 : value;
    RegMM3448_MM3578Mask.THS_PREPARE = 0;
    pPhyParams->HS_PREPARE = value * 2 * tUI;
    pPhyParams->HS_PREPARE = ceil(pPhyParams->HS_PREPARE, 1000);

    value = ceil(pPhyParams->CLK_PREPARE, (2 * tUI));
    RegMM3448_MM3578Value.TCLK_PREPARE = value < 0 ? 0 : value;
    RegMM3448_MM3578Mask.TCLK_PREPARE = 0;
    pPhyParams->CLK_PREPARE = value * 2 * tUI;
    pPhyParams->CLK_PREPARE = ceil(pPhyParams->CLK_PREPARE, 1000);

    RegMM3448_MM3578Value.Power_down_PLL = 1;   // power on EPHY PLL
    RegMM3448_MM3578Mask.Power_down_PLL = 0;
    RegMM3448_MM3578Value.Clock_Lane_Power_Down = 1;    // power on clock lane
    RegMM3448_MM3578Mask.Clock_Lane_Power_Down = 0;
    RegMM3448_MM3578Value.Power_Down_Data_Lane_0 = 1;    // power on data lane
    RegMM3448_MM3578Mask.Power_Down_Data_Lane_0 = 0;

    if (pDSIPanelTbl->LaneNum > 1)
    {
        RegMM3448_MM3578Value.Power_Down_Data_Lane_1 = 1;
        RegMM3448_MM3578Mask.Power_Down_Data_Lane_1 = 0;
    }

    if (pDSIPanelTbl->LaneNum > 2)
    {
        RegMM3448_MM3578Value.Power_Down_Data_Lane_2 = 1;
        RegMM3448_MM3578Mask.Power_Down_Data_Lane_2 = 0;
    }

    if (pDSIPanelTbl->LaneNum > 3)
    {
        RegMM3448_MM3578Value.Power_Down_Data_Lane_3 = 1;
        RegMM3448_MM3578Mask.Power_Down_Data_Lane_3 = 0;
    }

    RegMM3448_MM3578Value.Power_Down_LP_RX = 1;    // power on lp-rx
    RegMM3448_MM3578Mask.Power_Down_LP_RX = 0;
    //cbMMIOWriteReg32(pcbe, EPhyRegIndex, RegMM3448_MM3578Value.Value, RegMM3448_MM3578Mask.Value);

    if (pDSIConfig->ClkLaneMode == CBIOS_DSI_CLK_LANE_HARDWARE_CTL)    // hardware control
    {
        pPhyParams->tLP2HS = pPhyParams->LPX + \
                             pPhyParams->LPX + \
                             pPhyParams->CLK_PREPARE_ZERO + \
                             pPhyParams->CLK_PRE + \
                             pPhyParams->LPX + \
                             pPhyParams->LPX + \
                             pPhyParams->HS_PREPARE_ZERO + \
                             tBclk / 1000 + \
                             ceil(3 * tBclk , 1000) + \
                             ceil(2 * tBclk , 1000) + \
                             pPhyParams->LPX;

        pPhyParams->tHS2LP = pPhyParams->HS_TRAIL + \
                             pPhyParams->HS_EXIT + \
                             pPhyParams->LPX + \
                             pPhyParams->CLK_POST + \
                             pPhyParams->CLK_TRAIL + \
                             pPhyParams->HS_EXIT + \
                             pPhyParams->LPX + \
                             4 * tBclk / 1000 + \
                             3 * tBclk / 1000 + \
                             2 * tBclk / 1000 + \
                             pPhyParams->LPX;
    }
    else    // software control
    {
        pPhyParams->tLP2HS = pPhyParams->LPX + \
                             pPhyParams->LPX + \
                             pPhyParams->HS_PREPARE_ZERO + \
                             tBclk / 1000 + \
                             3 * tBclk / 1000 + \
                             2 * tBclk / 1000 + \
                             pPhyParams->LPX;

        pPhyParams->tHS2LP = pPhyParams->HS_TRAIL + \
                             pPhyParams->HS_EXIT + \
                             pPhyParams->LPX + \
                             4 * tBclk / 1000 + \
                             3 * tBclk / 1000 + \
                             2 * tBclk / 1000 + \
                             pPhyParams->LPX;
    }
    //set Ephy parameter
    cbMMIOWriteReg32(pcbe, EphyCtrlRegIndex, 0x09df3101, 0x0);
    cbMMIOWriteReg32(pcbe, EphyTunRegIndex, 0xe4030802, 0x0);
    cbMMIOWriteReg32(pcbe, EphyMiscRegIndex, 0x00008000, 0x0);

    cbTraceExit(DSI);
}

static CBIOS_VOID cbDSI_IrqEnableDisable(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 BitMask, CBIOS_BOOL bEnable, CBIOS_DSI_INDEX DSIIndex)
{
    REG_MM3408_MM3538 RegMM3408_MM3538Value;
    REG_MM3408_MM3538 RegMM3408_MM3538Mask;
    REG_MM340C_MM353C RegMM340C_MM353CValue;
    REG_MM340C_MM353C RegMM340C_MM353CMask;
    REG_MM3438_MM3568 RegMM3438_MM3568Value;
    REG_MM3438_MM3568 RegMM3438_MM3568Mask;
    CBIOS_U32 CtrlRegIndex = 0x3408, PhyRegIndex = 0x340C, IntRegIndex = 0x3438;
    cbTraceEnter(DSI);

    if (DSIIndex == DSI_INDEX0)
    {
        CtrlRegIndex = 0x3408;
        PhyRegIndex  = 0x340C;
        IntRegIndex  = 0x3438;
    }
    else if (DSIIndex == DSI_INDEX1)
    {
        CtrlRegIndex = 0x3538;
        PhyRegIndex  = 0x353C;
        IntRegIndex  = 0x3568;
    }

    RegMM3408_MM3538Value.Value = 0;
    RegMM3408_MM3538Mask.Value = 0xFFFFFFFF;
    RegMM340C_MM353CValue.Value = 0;
    RegMM340C_MM353CMask.Value = 0xFFFFFFFF;
    RegMM3438_MM3568Value.Value = 0;
    RegMM3438_MM3568Mask.Value = 0xFFFFFFFF;

    // trigger
    if (BitMask & (DSI_IRQ_ULPS_TRIGGER | DSI_IRQ_RESET_TRIGGER | DSI_IRQ_TE_TRIGGER | DSI_IRQ_OTHER_TRIGGER))
    {
        if (bEnable)
        {
            RegMM340C_MM353CValue.Receive_Trigger_Message_Interrupt_Enable = 1;
            RegMM3408_MM3538Value.DSI_Read_Data_Ready_Interrupt_Enable = 1;
        }
        else
        {
            RegMM340C_MM353CValue.Receive_Trigger_Message_Interrupt_Enable = 0;
            RegMM3408_MM3538Value.DSI_Read_Data_Ready_Interrupt_Enable = 0;
        }

        RegMM340C_MM353CMask.Receive_Trigger_Message_Interrupt_Enable = 0;
        RegMM3408_MM3538Mask.DSI_Read_Data_Ready_Interrupt_Enable = 0;
        cbMMIOWriteReg32(pcbe, PhyRegIndex, RegMM340C_MM353CValue.Value, RegMM340C_MM353CMask.Value);
        cbMMIOWriteReg32(pcbe, CtrlRegIndex, RegMM3408_MM3538Value.Value, RegMM3408_MM3538Mask.Value);

        // clear the interrupt status
        if (BitMask & DSI_IRQ_ULPS_TRIGGER)
        {
            RegMM3438_MM3568Value.Receive_ULPS_Trigger_Interrupt_Status = 1;
            RegMM3438_MM3568Mask.Receive_ULPS_Trigger_Interrupt_Status = 0;
        }

        if (BitMask & DSI_IRQ_RESET_TRIGGER)
        {
            RegMM3438_MM3568Value.Receive_Reset_Trigger_Interrupt_Status = 1;
            RegMM3438_MM3568Mask.Receive_Reset_Trigger_Interrupt_Status = 0;
        }

        if (BitMask & DSI_IRQ_TE_TRIGGER)
        {
            RegMM3438_MM3568Value.Receive_TE_Trigger_Interrupt_Status = 1;
            RegMM3438_MM3568Mask.Receive_TE_Trigger_Interrupt_Status = 0;
        }

        if (BitMask & DSI_IRQ_OTHER_TRIGGER)
        {
            RegMM3438_MM3568Value.Receive_Other_Trigger_or_Unrecognized_Stauts = 1;
            RegMM3438_MM3568Mask.Receive_Other_Trigger_or_Unrecognized_Stauts = 0;
        }

        cbMMIOWriteReg32(pcbe, IntRegIndex, RegMM3438_MM3568Value.Value, RegMM3438_MM3568Mask.Value);
    }

    // read back data
    if (BitMask & DSI_IRQ_DSI_READBACK)
    {
        if (bEnable)
        {
            RegMM3408_MM3538Value.DSI_Read_Data_Ready_Interrupt_Enable = 1;
        }
        else
        {
            RegMM3408_MM3538Value.DSI_Read_Data_Ready_Interrupt_Enable = 0;
        }

        RegMM3408_MM3538Mask.DSI_Read_Data_Ready_Interrupt_Enable = 0;
        cbMMIOWriteReg32(pcbe, CtrlRegIndex, RegMM3408_MM3538Value.Value, RegMM3408_MM3538Mask.Value);

        // clear the status
        RegMM3438_MM3568Value.DSI_Read_Back_Data_Interrupt_Status = 1;
        RegMM3438_MM3568Mask.DSI_Read_Back_Data_Interrupt_Status = 0;
        cbMMIOWriteReg32(pcbe, IntRegIndex, RegMM3438_MM3568Value.Value, RegMM3438_MM3568Mask.Value);
    }
    cbTraceExit(DSI);
}

static CBIOS_STATUS cbDSI_SendOneFrameData(PCBIOS_VOID pvcbe, CBIOS_DSI_INDEX DSIIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS Status;
    CBIOS_U8 CmdType = DSI_CMD_TYPE_VIDEO | DSI_CMD_TYPE_LAST_GROUP;
    CBIOS_U8 DataType = DSI_DCS_LONG_WRITE;
    cbTraceEnter(DSI);

    Status = cbDSI_SendShortPacket(pcbe, 0, DataType, DSI_DCS_WRITE_MEMORY_START, DSI_DCS_WRITE_MEMORY_CONTINUE, CmdType, DSIIndex);

    cbTraceExit(DSI);
    return Status;
}

static CBIOS_STATUS cbDSI_SetPXL_WC(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 OutBpp, CBIOS_U32 HorDispWidth, CBIOS_DSI_INDEX DSIIndex)
{
    CBIOS_U32    PXL_WC = 0;
    CBIOS_STATUS Status = CBIOS_OK;
    REG_MM345C_MM358C RegMM345C_MM358CValue;
    REG_MM345C_MM358C RegMM345C_MM358CMask;
    CBIOS_U32 VideoRegIndex = 0x345C;
    cbTraceEnter(DSI);

    if (DSIIndex == DSI_INDEX0)
    {
        VideoRegIndex = 0x345C;
    }
    else if (DSIIndex == DSI_INDEX1)
    {
        VideoRegIndex = 0x358C;
    }

    if (OutBpp == 16)
    {
        PXL_WC = 2 * HorDispWidth;
    }
    else if (OutBpp == 18)
    {
        if (HorDispWidth % 4 == 0)
        {
            PXL_WC = HorDispWidth * 18 / 8;
        }
        else
        {
            PXL_WC = (HorDispWidth + 4 - (HorDispWidth % 4)) * 18 / 8;
        }
    }
    else if (OutBpp == 24)
    {
        PXL_WC = 3 * HorDispWidth;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_PartialUpdate: Unsupport DSI out bpp!\n"));
        Status = CBIOS_ER_INVALID_PARAMETER;
    }

    RegMM345C_MM358CValue.Value = 0;
    RegMM345C_MM358CMask.Value = 0xFFFFFFFF;
    RegMM345C_MM358CValue.PXL_WC = PXL_WC;
    RegMM345C_MM358CMask.PXL_WC = 0;
    cbMMIOWriteReg32(pcbe, VideoRegIndex, RegMM345C_MM358CValue.Value, RegMM345C_MM358CMask.Value);

    cbTraceExit(DSI);
    return Status;
}

#if 0
// ref to MIPI-DCS 1.02.00 spec section 6.24 for Address mode definition
static CBIOS_STATUS cbDSI_SetAddrMode(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U8 AddrMode, CBIOS_DSI_INDEX DSIIndex)
{
    CBIOS_STATUS Status;
    CBIOS_U8 Buf[2] = {0};
    CBIOS_DSI_WRITE_PARA_INTERNAL DSIWriteParams;
    cbTraceEnter(DSI);

    cb_memset(&DSIWriteParams, 0, sizeof(CBIOS_DSI_WRITE_PARA_INTERNAL));
    DSIWriteParams.DSIIndex = DSIIndex;
    DSIWriteParams.VirtualCh = 0;
    DSIWriteParams.PacketType = CBIOS_DSI_SHORT_PACKET;
    DSIWriteParams.ContentType = CBIOS_DSI_CONTENT_DCS;
    DSIWriteParams.pDataBuf = Buf;
    DSIWriteParams.DataLen = 0x02;
    DSIWriteParams.bNeedAck = CBIOS_TRUE;
    DSIWriteParams.bHSModeOnly = CBIOS_FALSE;

    Buf[0] = DSI_DCS_SET_ADDRESS_MODE;
    Buf[1] = AddrMode;

    Status = cbDSI_SendWriteCmd(pcbe, &DSIWriteParams);

    if (Status != CBIOS_OK)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SetAddrMode failed!\n"));
    }

    cbTraceExit(DSI);
    return Status;
}
#endif

// ref to MIPI-DCS 1.02.00 spec section 6.25
static CBIOS_STATUS cbDSI_SetColumnAddr(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U16 StartCol, CBIOS_U16 EndCol, CBIOS_DSI_INDEX DSIIndex)
{
    CBIOS_STATUS Status;
    CBIOS_U8 Buf[5] = {0};
    CBIOS_DSI_WRITE_PARA_INTERNAL DSIWriteParams;
    cbTraceEnter(DSI);

    cb_memset(&DSIWriteParams, 0, sizeof(CBIOS_DSI_WRITE_PARA_INTERNAL));
    DSIWriteParams.DSIIndex = DSIIndex;
    DSIWriteParams.VirtualCh = 0;
    DSIWriteParams.PacketType = CBIOS_DSI_LONG_PACKET;
    DSIWriteParams.ContentType = CBIOS_DSI_CONTENT_DCS;
    DSIWriteParams.pDataBuf = Buf;
    DSIWriteParams.DataLen = 0x05;
    DSIWriteParams.bNeedAck = CBIOS_FALSE;
    DSIWriteParams.bHSModeOnly = CBIOS_FALSE;

    Buf[0] = DSI_DCS_SET_COLUMN_ADDRESS;
    Buf[1] = (CBIOS_U8)((StartCol & 0xFF00) >> 8);
    Buf[2] = (CBIOS_U8)(StartCol & 0x00FF);
    Buf[3] = (CBIOS_U8)((EndCol & 0xFF00) >> 8);
    Buf[4] = (CBIOS_U8)(EndCol & 0x00FF);

    Status = cbDSI_SendWriteCmd(pcbe, &DSIWriteParams);

    if (Status != CBIOS_OK)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SetColumnAddr failed!\n"));
    }

    cbTraceExit(DSI);
    return Status;
}

// ref to MIPI-DCS 1.02.00 spec section 6.29
static CBIOS_STATUS cbDSI_SetPageAddr(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U16 StartPage, CBIOS_U16 EndPage, CBIOS_DSI_INDEX DSIIndex)
{
    CBIOS_STATUS Status;
    CBIOS_U8 Buf[5] = {0};
    CBIOS_DSI_WRITE_PARA_INTERNAL DSIWriteParams;
    cbTraceEnter(DSI);

    cb_memset(&DSIWriteParams, 0, sizeof(CBIOS_DSI_WRITE_PARA_INTERNAL));
    DSIWriteParams.DSIIndex = DSIIndex;
    DSIWriteParams.VirtualCh = 0;
    DSIWriteParams.PacketType = CBIOS_DSI_LONG_PACKET;
    DSIWriteParams.ContentType = CBIOS_DSI_CONTENT_DCS;
    DSIWriteParams.pDataBuf = Buf;
    DSIWriteParams.DataLen = 0x05;
    DSIWriteParams.bNeedAck = CBIOS_FALSE;
    DSIWriteParams.bHSModeOnly = CBIOS_FALSE;

    Buf[0] = DSI_DCS_SET_PAGE_ADDRESS;
    Buf[1] = (CBIOS_U8)((StartPage & 0xFF00) >> 8);
    Buf[2] = (CBIOS_U8)(StartPage & 0x00FF);
    Buf[3] = (CBIOS_U8)((EndPage & 0xFF00) >> 8);
    Buf[4] = (CBIOS_U8)(EndPage & 0x00FF);

    Status = cbDSI_SendWriteCmd(pcbe, &DSIWriteParams);

    if (Status != CBIOS_OK)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SetPageAddr failed!\n"));
    }

    cbTraceExit(DSI);
    return Status;
}

#if 0
// ref to MIPI-DCS 1.02.00 spec section 6.35-6.37
static CBIOS_STATUS cbDSI_EnableDisableTE(PCBIOS_VOID pvcbe, CBIOS_BOOL bTurnOn, CBIOS_U16 Scanline, CBIOS_U8 TearMode, CBIOS_DSI_INDEX DSIIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_DSI_WRITE_PARA_INTERNAL  DSIWriteParams;
    CBIOS_U8     Buf[3] = {0};
    CBIOS_STATUS Status;
    cbTraceEnter(DSI);

    cb_memset(&DSIWriteParams, 0, sizeof(CBIOS_DSI_WRITE_PARA_INTERNAL));
    DSIWriteParams.DSIIndex = DSIIndex;
    DSIWriteParams.VirtualCh = 0;
    DSIWriteParams.ContentType = CBIOS_DSI_CONTENT_DCS;
    DSIWriteParams.pDataBuf = Buf;
    DSIWriteParams.bNeedAck = CBIOS_TRUE;
    DSIWriteParams.bHSModeOnly = CBIOS_FALSE;

    if (!bTurnOn)    // disable TE function
    {
        Buf[0] = DSI_DCS_SET_TEAR_OFF;
        DSIWriteParams.DataLen = 0x01;
        DSIWriteParams.PacketType = CBIOS_DSI_SHORT_PACKET;
        Status = cbDSI_SendWriteCmd(pcbe, &DSIWriteParams);
    }
    else    // enable TE
    {
        if (Scanline > 0)
        {
            Buf[0] = DSI_DCS_SET_TEAR_SCANLINE;
            Buf[1] = (CBIOS_U8)(Scanline & 0xFF);
            Buf[2] = (CBIOS_U8)(Scanline >> 8) & 0xFF;
            DSIWriteParams.DataLen = 0x03;
            DSIWriteParams.PacketType = CBIOS_DSI_LONG_PACKET;
            Status = cbDSI_SendWriteCmd(pcbe, &DSIWriteParams);
        }
        else
        {
            Buf[0] = DSI_DCS_SET_TEAR_ON;
            Buf[1] = TearMode;    // TearMode == 0: VSYNC, TearMode == 1: VSYNC+HSYNC
            DSIWriteParams.DataLen = 0x02;
            DSIWriteParams.PacketType = CBIOS_DSI_SHORT_PACKET;
            Status = cbDSI_SendWriteCmd(pcbe, &DSIWriteParams);
        }
    }

    if (Status != CBIOS_OK)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_EnableDisableTE failed!\n"));
    }

    cbTraceExit(DSI);
    return Status;
}
#endif

// This function is not really send DCS set_tear_on cmd, it just send a TE BTA to panel, so that host can receive TE signal from panel
CBIOS_STATUS cbDSI_SetTE_BTA(PCBIOS_VOID pvcbe, CBIOS_DSI_INDEX DSIIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS Status;
    // when we set DSI_CMD_TYPE_TE flag, we should also set DSI_CMD_TYPE_NEED_ACK, that hw can know we want to send TE BTA
    CBIOS_U8     CmdType = 0;
    CBIOS_U8  DataBuf[3]  = {0};
    PCBIOS_DEVICE_COMMON pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, CBIOS_TYPE_DSI);
    CBIOS_U16 ScanLine = (CBIOS_U16)pDevCommon->DeviceParas.DSIDevice.DSIPanelDesc.DSIPanelTbl.PanelTiming.YResolution;
    cbTraceEnter(DSI);

    //set TE on
    CmdType = DSI_CMD_TYPE_LAST_GROUP | DSI_CMD_TYPE_LP;
    Status = cbDSI_SendShortPacket(pcbe, 0, DSI_DCS_SHORT_WRITE_PARAM, DSI_DCS_SET_TEAR_ON, 0, CmdType, DSIIndex);
    if (Status != CBIOS_OK)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "Set tear on without BTA failed!\n"));
    }

    //set TE scan line
    CmdType = DSI_CMD_TYPE_LAST_GROUP | DSI_CMD_TYPE_LP;
    DataBuf[0] = DSI_DCS_SET_TEAR_SCANLINE;
    DataBuf[1] = (ScanLine >> 8) & 0xff;
    DataBuf[2] = ScanLine & 0xff;
    Status = cbDSI_SendLongPacket(pcbe, 0, DSI_DCS_LONG_WRITE, 3, DataBuf, CmdType, DSIIndex);
    if (Status != CBIOS_OK)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "Set tear scanline failed!\n"));
    }

    //set BTA
    CmdType = DSI_CMD_TYPE_LAST_GROUP | DSI_CMD_TYPE_LP | DSI_CMD_TYPE_TE | DSI_CMD_TYPE_NEED_ACK;
    Status = cbDSI_SendShortPacket(pcbe, 0, DSI_DCS_SHORT_WRITE_PARAM, DSI_DCS_SET_TEAR_ON, 0, CmdType, DSIIndex);
    if (Status != CBIOS_OK)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "Set tear on with BTA failed!\n"));
    }

    cbTraceExit(DSI);
    return Status;
}

#if 0
// ref to MIPI-DSI spec 8.8.10
static CBIOS_STATUS cbDSI_SetMaxReturnSize(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U16 MaxSize, CBIOS_DSI_INDEX DSIIndex)
{
    CBIOS_STATUS Status;
    CBIOS_U8     CmdType = DSI_CMD_TYPE_LAST_GROUP | DSI_CMD_TYPE_LP | DSI_CMD_TYPE_NEED_ACK;
    CBIOS_U8     Buf[2] = {0};
    cbTraceEnter(DSI);

    // LS byte first
    Buf[0] = (CBIOS_U8)(MaxSize & 0xFF);
    Buf[1] = (CBIOS_U8)((MaxSize >> 8) & 0xFF);

    Status = cbDSI_SendShortPacket(pcbe, 0, DSI_SET_MAXIMUM_RETURN_PACKET_SIZE, Buf[0], Buf[1], CmdType, DSIIndex);

    if (Status != CBIOS_OK)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SetMaxReturnSize failed!\n"));
    }

    cbTraceExit(DSI);
    return Status;
}
#endif

static CBIOS_STATUS cbDSI_GetReceivedPayload(PCBIOS_VOID pvcbe, CBIOS_U8 *pDataBuf, CBIOS_U16 *pBufLen, CBIOS_DSI_INDEX DSIIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS Status = CBIOS_OK;
    CBIOS_U8  DataType;
    CBIOS_U16 WordCount;
    CBIOS_U32 RawData, i, Reminder;
    CBIOS_U32 DataRegIndex = 0x3404;
    cbTraceEnter(DSI);

    if (DSIIndex == DSI_INDEX0)
    {
        DataRegIndex   = 0x3404;
    }
    else if (DSIIndex == DSI_INDEX1)
    {
        DataRegIndex   = 0x3534;
    }

    if ((pDataBuf == CBIOS_NULL) || (pBufLen == CBIOS_NULL))
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_ReadBackData: data buffer or buffer length is null! \n"));
        Status = CBIOS_ER_NULLPOINTER;
        goto Exit;
    }

    // first parse the data type
    RawData = cbReadRegisterU32(pcbe, CBIOS_REGISTER_MMIO, DataRegIndex);
    DataType = (CBIOS_U8)(RawData & 0x3F);

    if ((DataType == DSI_RX_DCS_SHORT_READ_RESPONSE_1BYTE) || (DataType == DSI_RX_GENERIC_SHORT_READ_RESPONSE_1BYTE))
    {
        pDataBuf[0] = (CBIOS_U8)((RawData & 0xFF00) >> 8);
        *pBufLen = 1;
        goto Exit;
    }
    else if ((DataType == DSI_RX_DCS_SHORT_READ_RESPONSE_2BYTE) || (DataType == DSI_RX_GENERIC_SHORT_READ_RESPONSE_2BYTE))
    {
        pDataBuf[0] = (CBIOS_U8)((RawData & 0xFF00) >> 8);
        pDataBuf[1] = (CBIOS_U8)((RawData & 0xFF0000) >> 16);
        *pBufLen = 2;
        goto Exit;
    }
    else if ((DataType == DSI_RX_DCS_LONG_READ_RESPONSE) || (DataType == DSI_RX_GENERIC_LONG_READ_RESPONSE))
    {
        WordCount = (CBIOS_U16)((RawData & 0xFFFF00) >> 8);    // get the wordcount

        for (i = 0; i < (CBIOS_U32)(WordCount >> 2); i++)
        {
            *((CBIOS_U32 *)(pDataBuf + i * 4)) = cbReadRegisterU32(pcbe, CBIOS_REGISTER_MMIO, DataRegIndex);
        }

        Reminder = WordCount % 4;

        if (Reminder != 0)
        {
            RawData = cbReadRegisterU32(pcbe, CBIOS_REGISTER_MMIO, DataRegIndex);

            for (i = Reminder; i > 0; i--)
            {
                *((CBIOS_U8 *)(pDataBuf + WordCount - i)) = (CBIOS_U8)((RawData >> (Reminder - i) * 8) & 0xFF);
            }
        }

        *pBufLen = WordCount;
    }

Exit:
    cbTraceExit(DSI);
    return Status;
}

static CBIOS_VOID cbDSI_ClearReadFIFO(PCBIOS_EXTENSION_COMMON pcbe,  CBIOS_DSI_INDEX DSIIndex)
{
    CBIOS_U32 ReadDataNum, i;
    REG_MM3444_MM3574 RegMM3444_MM3574Value;
    REG_MM3404_MM3534 RegMM3404_MM3534Value;
    CBIOS_U32 StatusRegIndex = 0x3444, DataRegIndex = 0x3404;
    cbTraceEnter(DSI);

    if (DSIIndex == DSI_INDEX0)
    {
        StatusRegIndex = 0x3444;
        DataRegIndex   = 0x3404;
    }
    else if (DSIIndex == DSI_INDEX1)
    {
        StatusRegIndex = 0x3574;
        DataRegIndex   = 0x3534;
    }

    RegMM3444_MM3574Value.Value = cbReadRegisterU32(pcbe, CBIOS_REGISTER_MMIO, StatusRegIndex);
    ReadDataNum = RegMM3444_MM3574Value.DSI_Read_Back_Data_Number;

    for(i = 0; i < ReadDataNum; i+=4)
    {
        RegMM3404_MM3534Value.Value = cbReadRegisterU32(pcbe, CBIOS_REGISTER_MMIO, DataRegIndex);
        cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "cbDSI_ClearReadFIFO: uncaught read data: 0x%08x! \n", RegMM3404_MM3534Value.Value));
    }

    cbTraceExit(DSI);
}

/* DCS command interfaces start */

CBIOS_STATUS cbDSI_SendWriteCmd(PCBIOS_VOID pvcbe, PCBIOS_DSI_WRITE_PARA_INTERNAL pDSIWriteParams)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS            Status;
    CBIOS_U8                DataType;
    CBIOS_U32               i;
    CBIOS_BOOL              bReceiveACK;
    CBIOS_DSI_INDEX         DSIIndex = pDSIWriteParams->DSIIndex;
    CBIOS_U16               DataLen = pDSIWriteParams->DataLen;
    PCBIOS_U8               pDataBuf = pDSIWriteParams->pDataBuf;
    CBIOS_U8                VirtualCh = pDSIWriteParams->VirtualCh;
    CBIOS_U8                CmdType = DSI_CMD_TYPE_LAST_GROUP;
    REG_MM3438_MM3568       RegMM3438_MM3568Value;
    REG_MM3438_MM3568       RegMM3438_MM3568Mask;
    CBIOS_U32               IntRegIndex = 0x3438;
    cbTraceEnter(DSI);

    cbDSI_ClearReadFIFO(pcbe, DSIIndex);

    if (DSIIndex == DSI_INDEX0)
    {
        IntRegIndex  = 0x3438;
    }
    else if (DSIIndex == DSI_INDEX1)
    {
        IntRegIndex  = 0x3568;
    }

    if (pDSIWriteParams->bNeedAck)
    {
        CmdType |= DSI_CMD_TYPE_NEED_ACK;
    }

    if (!pDSIWriteParams->bHSModeOnly)
    {
        CmdType |= DSI_CMD_TYPE_LP;
    }

    if (pDSIWriteParams->PacketType == CBIOS_DSI_SHORT_PACKET)
    {
        if (pDSIWriteParams->ContentType == CBIOS_DSI_CONTENT_DCS)
        {
            if (DataLen == 1)
            {
                DataType = DSI_DCS_SHORT_WRITE;
                Status = cbDSI_SendShortPacket(pcbe, VirtualCh, DataType, pDataBuf[0], 0, CmdType, DSIIndex);
            }
            else if (DataLen == 2)
            {
                DataType = DSI_DCS_SHORT_WRITE_PARAM;
                Status = cbDSI_SendShortPacket(pcbe, VirtualCh, DataType, pDataBuf[0], pDataBuf[1], CmdType, DSIIndex);
            }
            else
            {
                cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SendWriteCmd: Data length is invaild for DCS write short packet! \n"));
                Status = CBIOS_ER_INVALID_PARAMETER;
                goto Exit;
            }
        }
        else if (pDSIWriteParams->ContentType == CBIOS_DSI_CONTENT_GEN)
        {
            if (DataLen == 0)
            {
                DataType = DSI_GENERIC_SHORT_WRITE_0_PARAM;
                Status = cbDSI_SendShortPacket(pcbe, VirtualCh, DataType, 0, 0, CmdType, DSIIndex);
            }
            else if (DataLen == 1)
            {
                DataType = DSI_GENERIC_SHORT_WRITE_1_PARAM;
                Status = cbDSI_SendShortPacket(pcbe, VirtualCh, DataType, pDataBuf[0], 0, CmdType, DSIIndex);
            }
            else if (DataLen == 2)
            {
                DataType = DSI_GENERIC_SHORT_WRITE_2_PARAM;
                Status = cbDSI_SendShortPacket(pcbe, VirtualCh, DataType, pDataBuf[0], pDataBuf[1], CmdType, DSIIndex);
            }
            else
            {
                cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SendWriteCmd: Data length is invaild for GEN write short packet! \n"));
                Status = CBIOS_ER_INVALID_PARAMETER;
                goto Exit;
            }
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SendWriteCmd: ContentType is invaild! \n"));
            Status = CBIOS_ER_INVALID_PARAMETER;
            goto Exit;
        }
    }
    else if (pDSIWriteParams->PacketType == CBIOS_DSI_LONG_PACKET)
    {
        if (pDSIWriteParams->ContentType == CBIOS_DSI_CONTENT_DCS)
        {
            DataType = DSI_DCS_LONG_WRITE;
            Status = cbDSI_SendLongPacket(pcbe, VirtualCh, DataType, DataLen, pDataBuf, CmdType, DSIIndex);
        }
        else if (pDSIWriteParams->ContentType == CBIOS_DSI_CONTENT_GEN)
        {
            DataType = DSI_GENERIC_LONG_WRITE;
            Status = cbDSI_SendLongPacket(pcbe, VirtualCh, DataType, DataLen, pDataBuf, CmdType, DSIIndex);
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SendWriteCmd: ContentType is invaild! \n"));
            Status = CBIOS_ER_INVALID_PARAMETER;
            goto Exit;
        }
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SendWriteCmd: PacketType is invaild! \n"));
        Status = CBIOS_ER_INVALID_PARAMETER;
        goto Exit;
    }

    if (pDSIWriteParams->bNeedAck)
    {
        //wait for ACK
        // Fix me: the delay time may be not correct
        bReceiveACK = CBIOS_FALSE;

        for (i = 0; i < CBIOS_DSI_POLLING_TIMEOUT; i++)
        {
            RegMM3438_MM3568Value.Value = cbReadRegisterU32(pcbe, CBIOS_REGISTER_MMIO, IntRegIndex);

            if (RegMM3438_MM3568Value.Receive_Other_Trigger_or_Unrecognized_Stauts)
            {
                if (RegMM3438_MM3568Value.Receive_Trigger_Data == DSI_TRIGGER_MSG_ACK)
                {
                    bReceiveACK = CBIOS_TRUE;
                    cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "cbDSI_SendWriteCmd: Receive the ACK success! \n"));
                    Status = CBIOS_OK;
                    // clear the status
                    RegMM3438_MM3568Value.Value = 0;
                    RegMM3438_MM3568Mask.Value = 0xFFFFFFFF;
                    RegMM3438_MM3568Value.Receive_Other_Trigger_or_Unrecognized_Stauts = 1;
                    RegMM3438_MM3568Mask.Receive_Other_Trigger_or_Unrecognized_Stauts = 0;
                    cbMMIOWriteReg32(pcbe, IntRegIndex, RegMM3438_MM3568Value.Value, RegMM3438_MM3568Mask.Value);
                    break;
                }
            }
            else
            {
                cbDelayMilliSeconds(1);
            }
        }

        if (!bReceiveACK)
        {
            cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SendWriteCmd: Receive the ACK failed! \n"));
            Status = CBIOS_ER_INTERNAL;
        }
    }

Exit:
    cbTraceExit(DSI);
    return Status;
}


CBIOS_STATUS cbDSI_SendReadCmd(PCBIOS_VOID pvcbe, PCBIOS_DSI_READ_PARA_INTERNAL pDSIReadParams)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_DSI_INDEX         DSIIndex = pDSIReadParams->DSIIndex;
    CBIOS_U16               DataLen = pDSIReadParams->DataLen;
    PCBIOS_U8               pDataBuf = pDSIReadParams->pDataBuf;
    CBIOS_U8                VirtualCh = pDSIReadParams->VirtualCh;
    CBIOS_U8                CmdType = DSI_CMD_TYPE_LAST_GROUP | DSI_CMD_TYPE_NEED_ACK;
    CBIOS_U8                DataType;
    CBIOS_STATUS            Status;
    CBIOS_BOOL              bGetResponse = CBIOS_FALSE;
    CBIOS_U32               i;
    REG_MM3438_MM3568       RegMM3438_MM3568Value;
    REG_MM3438_MM3568       RegMM3438_MM3568Mask;
    CBIOS_U32               IntRegIndex = 0x3438;
    cbTraceEnter(DSI);

    cbDSI_ClearReadFIFO(pcbe, DSIIndex);

    if (DSIIndex == DSI_INDEX0)
    {
        IntRegIndex  = 0x3438;
    }
    else if (DSIIndex == DSI_INDEX1)
    {
        IntRegIndex  = 0x3568;
    }

    if (!pDSIReadParams->bHSModeOnly)
    {
        CmdType |= DSI_CMD_TYPE_LP;
    }

    // clear read back data interrupt status
    RegMM3438_MM3568Value.Value = 0;
    RegMM3438_MM3568Mask.Value = 0xFFFFFFFF;
    RegMM3438_MM3568Value.DSI_Read_Back_Data_Interrupt_Status = 1;
    RegMM3438_MM3568Mask.DSI_Read_Back_Data_Interrupt_Status = 0;
    cbMMIOWriteReg32(pcbe, IntRegIndex, RegMM3438_MM3568Value.Value, RegMM3438_MM3568Mask.Value);

    // send read cmd
    if (pDSIReadParams->ContentType == CBIOS_DSI_CONTENT_DCS)
    {
        DataType = DSI_DCS_READ;
        Status = cbDSI_SendShortPacket(pcbe, VirtualCh, DataType, pDataBuf[0], 0, CmdType, DSIIndex);
    }
    else if (pDSIReadParams->ContentType == CBIOS_DSI_CONTENT_GEN)
    {
        if (DataLen == 0)
        {
            DataType = DSI_GENERIC_READ_REQUEST_0_PARAM;
            Status = cbDSI_SendShortPacket(pcbe, VirtualCh, DataType, 0, 0, CmdType, DSIIndex);
        }
        else if (DataLen == 1)
        {
            DataType = DSI_GENERIC_READ_REQUEST_1_PARAM;
            Status = cbDSI_SendShortPacket(pcbe, VirtualCh, DataType, pDataBuf[0], 0, CmdType, DSIIndex);
        }
        else if (DataLen == 2)
        {
            DataType = DSI_GENERIC_READ_REQUEST_2_PARAM;
            Status = cbDSI_SendShortPacket(pcbe, VirtualCh, DataType, pDataBuf[0], pDataBuf[1], CmdType, DSIIndex);
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SendGenReadCmd: Data length is invaild for generic read short packet! \n"));
            Status = CBIOS_ER_INVALID_PARAMETER;
            goto Exit;
        }
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SendReadCmd: ContentType is invaild! \n"));
        Status = CBIOS_ER_INVALID_PARAMETER;
        goto Exit;
    }

    // get read back payload
    if (Status == CBIOS_OK)
    {
        for (i = 0; i < CBIOS_DSI_POLLING_TIMEOUT; i++)
        {
            RegMM3438_MM3568Value.Value = cbReadRegisterU32(pcbe, CBIOS_REGISTER_MMIO, IntRegIndex);

            if (RegMM3438_MM3568Value.DSI_Read_Back_Data_Interrupt_Status)
            {
                bGetResponse = CBIOS_TRUE;
                cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "cbDSI_SendReadCmd: Receive the read back data! \n"));
                // clear the status
                RegMM3438_MM3568Value.DSI_Read_Back_Data_Interrupt_Status = 1;
                RegMM3438_MM3568Mask.DSI_Read_Back_Data_Interrupt_Status = 0;
                cbMMIOWriteReg32(pcbe, IntRegIndex, RegMM3438_MM3568Value.Value, RegMM3438_MM3568Mask.Value);
                break;
            }
            else
            {
                cbDelayMilliSeconds(1);
            }
        }

        if (bGetResponse)
        {
            Status = cbDSI_GetReceivedPayload(pcbe, pDSIReadParams->pReceivedPayloadBuf, &(pDSIReadParams->ReceivedPayloadLen), DSIIndex);
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SendReadCmd: get read back data failed! \n"));
            Status = CBIOS_ER_INTERNAL;
            goto Exit;
        }
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SendReadCmd: send read cmd failed! \n"));
    }

Exit:
    cbTraceExit(DSI);
    return Status;
}


CBIOS_STATUS cbDSI_SetDisplayOnOff(PCBIOS_VOID pvcbe, CBIOS_BOOL bTurnOn, CBIOS_DSI_INDEX DSIIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS Status;
    CBIOS_U8     Buf[0x01] = {DSI_DCS_SET_DISPLAY_ON};
    CBIOS_DSI_WRITE_PARA_INTERNAL DSIWriteParams;
    cbTraceEnter(DSI);

    cb_memset(&DSIWriteParams, 0, sizeof(CBIOS_DSI_WRITE_PARA_INTERNAL));
    DSIWriteParams.DSIIndex = DSIIndex;
    DSIWriteParams.VirtualCh = 0;
    DSIWriteParams.PacketType = CBIOS_DSI_SHORT_PACKET;
    DSIWriteParams.ContentType = CBIOS_DSI_CONTENT_DCS;
    DSIWriteParams.pDataBuf = Buf;
    DSIWriteParams.DataLen = 0x01;
    DSIWriteParams.bNeedAck = CBIOS_FALSE;
    DSIWriteParams.bHSModeOnly = CBIOS_FALSE;

    if (bTurnOn)
    {
        Buf[0] = DSI_DCS_SET_DISPLAY_ON;
    }
    else
    {
        Buf[0] = DSI_DCS_SET_DISPLAY_OFF;
    }

    Status = cbDSI_SendWriteCmd(pcbe, &DSIWriteParams);

    if (Status != CBIOS_OK)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SetDisplayOnOff failed!\n"));
    }

    cbTraceExit(DSI);
    return Status;
}

CBIOS_STATUS cbDSI_PanelSoftReset(PCBIOS_VOID pvcbe, CBIOS_DSI_INDEX DSIIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS Status = CBIOS_OK;
    CBIOS_U8     Buf[0x01] = {DSI_DCS_SOFT_RESET};
    CBIOS_DSI_WRITE_PARA_INTERNAL DSIWriteParams;
    cbTraceEnter(DSI);

    cb_memset(&DSIWriteParams, 0, sizeof(CBIOS_DSI_WRITE_PARA_INTERNAL));
    DSIWriteParams.DSIIndex = DSIIndex;
    DSIWriteParams.VirtualCh = 0;
    DSIWriteParams.PacketType = CBIOS_DSI_SHORT_PACKET;
    DSIWriteParams.ContentType = CBIOS_DSI_CONTENT_DCS;
    DSIWriteParams.pDataBuf = Buf;
    DSIWriteParams.DataLen = 0x01;
    DSIWriteParams.bNeedAck = CBIOS_FALSE;
    DSIWriteParams.bHSModeOnly = CBIOS_FALSE;

    Status = cbDSI_SendWriteCmd(pcbe, &DSIWriteParams);

    if (Status != CBIOS_OK)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_PanelSoftReset failed!\n"));
    }

    cbTraceExit(DSI);
    return Status;
}

/* Switch the display module between the idle, invert, normal, partial and sleep mode described in MIPI-DCS spec*/
static CBIOS_STATUS cbDSI_DCSSwitchMode(PCBIOS_VOID pvcbe, CBIOS_DSI_DCS_MODE Mode, CBIOS_BOOL bEnter, CBIOS_DSI_INDEX DSIIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS Status;
    CBIOS_U8     Buf[1] = {0};
    CBIOS_DSI_WRITE_PARA_INTERNAL DSIWriteParams;
    cbTraceEnter(DSI);

    cb_memset(&DSIWriteParams, 0, sizeof(CBIOS_DSI_WRITE_PARA_INTERNAL));
    DSIWriteParams.DSIIndex = DSIIndex;
    DSIWriteParams.VirtualCh = 0;
    DSIWriteParams.PacketType = CBIOS_DSI_SHORT_PACKET;
    DSIWriteParams.ContentType = CBIOS_DSI_CONTENT_DCS;
    DSIWriteParams.pDataBuf = Buf;
    DSIWriteParams.DataLen = 0x01;
    DSIWriteParams.bNeedAck = CBIOS_FALSE;
    DSIWriteParams.bHSModeOnly = CBIOS_FALSE;

    switch (Mode)
    {
    case CBIOS_DSI_DCS_IDLE_MODE:
        if (bEnter)
        {
            Buf[0] = DSI_DCS_ENTER_IDLE_MODE;
        }
        else
        {
            Buf[0] = DSI_DCS_EXIT_IDLE_MODE;
        }

        break;

    case CBIOS_DSI_DCS_INVERT_MODE:
        if (bEnter)
        {
            Buf[0] = DSI_DCS_ENTER_INVERT_MODE;
        }
        else
        {
            Buf[0] = DSI_DCS_EXIT_INVERT_MODE;
        }

        break;

    case CBIOS_DSI_DCS_NORMAL_MODE:
        if (bEnter)
        {
            Buf[0] = DSI_DCS_ENTER_NORMAL_MODE;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DSI, WARNING), "cbDSI_DCSSwitchMode: there is no exit_normal_mode cmd, send nop instead! \n"));
            Buf[0] = DSI_DCS_NOP;
        }

        break;

    case CBIOS_DSI_DCS_PARTIAL_MODE:
        if (bEnter)
        {
            Buf[0] = DSI_DCS_ENTER_PARTIAL_MODE;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DSI, WARNING), "cbDSI_DCSSwitchMode: there is no exit_partial_mode cmd, send nop instead! \n"));
            Buf[0] = DSI_DCS_NOP;
        }

        break;

    case CBIOS_DSI_DCS_SLEEP_MODE:
        if (bEnter)
        {
            Buf[0] = DSI_DCS_ENTER_SLEEP_MODE;
        }
        else
        {
            Buf[0] = DSI_DCS_EXIT_SLEEP_MODE;
        }

        break;
    }

    Status = cbDSI_SendWriteCmd(pcbe, &DSIWriteParams);

    cbTraceExit(DSI);
    return Status;
}

static CBIOS_VOID cbDSI_SetDSITimingHW(PCBIOS_VOID pvcbe, PCBIOS_DSI_PARAMS pDSIParams, PCBIOS_TIMING_ATTRIB pTiming, CBIOS_U32 IGAIndex, CBIOS_DSI_INDEX DSIIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32  tDclk, tBclk;
    CBIOS_BOOL isHFPEnterLP, isHSAEnterLP, isHBPEnterLP;
    CBIOS_U16  HorFrontPorch, HorSyncAct, HorBackPorch;
    CBIOS_DSI_VIDEO_TIMING_REG DSITimingReg;
    PCBIOS_DSI_PHY_PARAMS pPhyParams;
    PCBIOS_DSI_CONFIG pDSIConfig = &(pDSIParams->DSIPanelDesc.DSIConfig);
    PCBIOS_DSI_PANEL_TABLE pDSIPanelTbl = &(pDSIParams->DSIPanelDesc.DSIPanelTbl);
    PCBIOS_TIMING_ATTRIB pTimingAttrib;
    REG_MM345C_MM358C RegMM345C_MM358CValue;
    REG_MM345C_MM358C RegMM345C_MM358CMask;
    REG_MM3460_MM3590 RegMM3460_MM3590Value;
    REG_MM3460_MM3590 RegMM3460_MM3590Mask;
    REG_MM3464_MM3594 RegMM3464_MM3594Value;
    REG_MM3464_MM3594 RegMM3464_MM3594Mask;
    REG_MM3468_MM3598 RegMM3468_MM3598Value;
    REG_MM3468_MM3598 RegMM3468_MM3598Mask;
    REG_MM34C0_MM34C8 RegMM34C0_MM34C8Value;
    REG_MM34C0_MM34C8 RegMM34C0_MM34C8Mask;
    CBIOS_U32 VideoReg1Index = 0x345C, VideoReg2Index = 0x3460;
    CBIOS_U32 VideoReg3Index = 0x3464, VideoReg4Index = 0x3468;
    CBIOS_U32 VideoReg5Index = 0x34c0;
    CBIOS_TIMING_ATTRIB  TimingAttrib = {0};
    cbTraceEnter(DSI);

    if (DSIIndex == DSI_INDEX0)
    {
        VideoReg1Index = 0x345C;
        VideoReg2Index = 0x3460;
        VideoReg3Index = 0x3464;
        VideoReg4Index = 0x3468;
        VideoReg5Index = 0x34c0;
    }
    else if (DSIIndex == DSI_INDEX1)
    {
        VideoReg1Index = 0x358C;
        VideoReg2Index = 0x3590;
        VideoReg3Index = 0x3594;
        VideoReg4Index = 0x3598;
        VideoReg5Index = 0x34c8;
    }

    cb_memset(&DSITimingReg, 0, sizeof(CBIOS_DSI_VIDEO_TIMING_REG));
    pPhyParams = &(pDSIParams->PhyParams);
    pTimingAttrib = &TimingAttrib;
    cb_memcpy(pTimingAttrib, pTiming, sizeof(CBIOS_TIMING_ATTRIB));
    if (pDSIConfig->isDualChannel)
    {
        pTimingAttrib->PixelClock /= 2;
        pTimingAttrib->HorTotal /= 2;
        pTimingAttrib->HorDisEnd /= 2;
        pTimingAttrib->HorBStart /= 2;
        pTimingAttrib->HorBEnd /= 2;
        pTimingAttrib->HorSyncStart /= 2;
        pTimingAttrib->HorSyncEnd /= 2;
    }
    tDclk = 10000000 / (pTimingAttrib->PixelClock) * 1000;    // unit: ps, 1ns = 1000ps
    tBclk = tDclk * pDSIPanelTbl->LaneNum * 8 / pDSIPanelTbl->OutBpp;    // unit: ps, 1ns = 1000ps


    HorFrontPorch = pTimingAttrib->HorSyncStart - pTimingAttrib->HorBStart;
    HorSyncAct    = pTimingAttrib->HorSyncEnd - pTimingAttrib->HorSyncStart;
    HorBackPorch  = pTimingAttrib->HorBEnd - pTimingAttrib->HorSyncEnd;

    cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "%s: HorFrontPorch:  %d\n ", FUNCTION_NAME, HorFrontPorch));
    cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "%s: HorSyncAct:  %d\n ", FUNCTION_NAME, HorSyncAct));
    cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "%s: HorBackPorch:  %d\n ", FUNCTION_NAME, HorBackPorch));

    // calc *_WC
    if (pDSIPanelTbl->OutBpp == 16)
    {
        DSITimingReg.PXL_WC = 2 * pTimingAttrib->HorDisEnd;
        DSITimingReg.HFP_WC = 2 * HorFrontPorch - 11;
        DSITimingReg.HSA_WC = 2 * HorSyncAct - 10;

        if (pDSIConfig->SyncPacketType == CBIOS_DSI_SYNC_PULSE)
        {
            DSITimingReg.HBP_WC = 2 * pTimingAttrib->HorTotal - DSITimingReg.PXL_WC - DSITimingReg.HFP_WC - DSITimingReg.HSA_WC - 32;
        }
        else
        {
            DSITimingReg.HBP_WC = 2 * pTimingAttrib->HorTotal - DSITimingReg.PXL_WC - DSITimingReg.HFP_WC - 22;
        }
    }
    else if (pDSIPanelTbl->OutBpp == 18)
    {
        if (pTimingAttrib->HorDisEnd % 4 == 0)
        {
            DSITimingReg.PXL_WC = pTimingAttrib->HorDisEnd * 18 / 8;
        }
        else
        {
            DSITimingReg.PXL_WC = (pTimingAttrib->HorDisEnd + 4 - (pTimingAttrib->HorDisEnd % 4)) * 18 / 8;
        }

        DSITimingReg.HFP_WC = cbRound(HorFrontPorch * 18, 8, ROUND_UP) - 11;
        DSITimingReg.HSA_WC = cbRound(HorSyncAct * 18, 8, ROUND_UP) - 10;

        if (pDSIConfig->SyncPacketType == CBIOS_DSI_SYNC_PULSE)
        {
            DSITimingReg.HBP_WC = pTimingAttrib->HorTotal * 18 / 8 - DSITimingReg.PXL_WC - DSITimingReg.HFP_WC - DSITimingReg.HSA_WC - 32;
        }
        else
        {
            DSITimingReg.HBP_WC = pTimingAttrib->HorTotal * 18 / 8 - DSITimingReg.PXL_WC - DSITimingReg.HFP_WC - 22;
        }
    }
    else if (pDSIPanelTbl->OutBpp == 24)
    {
        DSITimingReg.PXL_WC = 3 * pTimingAttrib->HorDisEnd;
        DSITimingReg.HFP_WC = 3 * HorFrontPorch - 11;
        DSITimingReg.HSA_WC = 3 * HorSyncAct - 10;

        if (pDSIConfig->SyncPacketType == CBIOS_DSI_SYNC_PULSE)
        {
            DSITimingReg.HBP_WC = pTimingAttrib->HorTotal * 3 - DSITimingReg.PXL_WC - DSITimingReg.HFP_WC - DSITimingReg.HSA_WC - 32;
        }
        else
        {
            DSITimingReg.HBP_WC = pTimingAttrib->HorTotal * 3 - DSITimingReg.PXL_WC - DSITimingReg.HFP_WC - 22;
        }
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DSI, WARNING), "cbDSI_SetVideoModeTiming: Unsupport DSI video mode bpp!\n"));
    }

    // calc *LP2HS
    DSITimingReg.HSS_LP2HS = (CBIOS_S32)(pTimingAttrib->HorSyncStart * tDclk - 1000 * pPhyParams->tLP2HS);
    DSITimingReg.HSE_LP2HS = (CBIOS_S32)(pTimingAttrib->HorSyncEnd * tDclk - 1000 * pPhyParams->tLP2HS);
    DSITimingReg.PXL_LP2HS = (CBIOS_S32)(pTimingAttrib->HorBEnd * tDclk - 1000 * pPhyParams->tLP2HS);

    if ((DSITimingReg.HSS_LP2HS <= 0) || (DSITimingReg.HSE_LP2HS <= 0) || (DSITimingReg.PXL_LP2HS <= 0))
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_SetVideoModeTiming: Error: *LP2HS is negative!\n"));
    }

    DSITimingReg.HSS_LP2HS = ceil(DSITimingReg.HSS_LP2HS, tBclk);
    DSITimingReg.HSE_LP2HS = ceil(DSITimingReg.HSE_LP2HS, tBclk);
    DSITimingReg.PXL_LP2HS = ceil(DSITimingReg.PXL_LP2HS, tBclk);

    // set DSI timing registers
    RegMM345C_MM358CValue.Value = 0;
    RegMM345C_MM358CMask.Value = 0xFFFFFFFF;
    RegMM345C_MM358CValue.PXL_WC = DSITimingReg.PXL_WC;
    RegMM345C_MM358CMask.PXL_WC = 0;
    cbMMIOWriteReg32(pcbe, VideoReg1Index, RegMM345C_MM358CValue.Value, RegMM345C_MM358CMask.Value);

    RegMM3460_MM3590Value.HFP_WC = DSITimingReg.HFP_WC;
    RegMM3460_MM3590Value.HSA_WC = DSITimingReg.HSA_WC;
    RegMM3460_MM3590Mask.Value = 0;
    cbMMIOWriteReg32(pcbe, VideoReg2Index, RegMM3460_MM3590Value.Value, RegMM3460_MM3590Mask.Value);

    RegMM3464_MM3594Value.HBP_WC = DSITimingReg.HBP_WC;
    RegMM3464_MM3594Value.HSS_LP2HS = DSITimingReg.HSS_LP2HS;
    RegMM3464_MM3594Mask.Value = 0;
    cbMMIOWriteReg32(pcbe, VideoReg3Index, RegMM3464_MM3594Value.Value, RegMM3464_MM3594Mask.Value);

    RegMM3468_MM3598Value.HSE_LP2HS = DSITimingReg.HSE_LP2HS;
    RegMM3468_MM3598Value.PXL_LP2HS = DSITimingReg.PXL_LP2HS;
    RegMM3468_MM3598Mask.Value = 0;
    cbMMIOWriteReg32(pcbe, VideoReg4Index, RegMM3468_MM3598Value.Value, RegMM3468_MM3598Mask.Value);

    // set video mode lp
    if (pDSIConfig->DSIMode == CBIOS_DSI_VIDEOMODE)
    {
        isHFPEnterLP = pDSIConfig->isHFPEnterLP;
        isHSAEnterLP = pDSIConfig->isHSAEnterLP;
        isHBPEnterLP = pDSIConfig->isHBPEnterLP;

        if ((HorFrontPorch * tDclk) <= (pPhyParams->tHS2LP + pPhyParams->tLP2HS) * 1000)
        {
            isHFPEnterLP = CBIOS_FALSE;
        }

        if (pDSIConfig->SyncPacketType == CBIOS_DSI_SYNC_PULSE)
        {
            if ((HorSyncAct * tDclk) <= (pPhyParams->tHS2LP + pPhyParams->tLP2HS) * 1000)
            {
                isHSAEnterLP = CBIOS_FALSE;
            }
        }

        if (pDSIConfig->SyncPacketType == CBIOS_DSI_SYNC_PULSE)
        {
            if ((HorBackPorch * tDclk) <= (pPhyParams->tHS2LP + pPhyParams->tLP2HS) * 1000)
            {
                isHBPEnterLP = CBIOS_FALSE;
            }
        }
        else
        {
            if ((HorSyncAct + HorBackPorch) * tDclk <= (pPhyParams->tHS2LP + pPhyParams->tLP2HS) * 1000)
            {
                isHBPEnterLP = CBIOS_FALSE;
            }
        }
    }
    else
    {
        isHFPEnterLP = CBIOS_FALSE;
        isHSAEnterLP = CBIOS_FALSE;
        isHBPEnterLP = CBIOS_FALSE;
    }

    RegMM345C_MM358CValue.HFP_LP = isHFPEnterLP;
    RegMM345C_MM358CValue.HSA_LP = isHSAEnterLP;
    RegMM345C_MM358CValue.HBP_LP = isHBPEnterLP;
    RegMM345C_MM358CValue.DSI_Video_Mode_Enable = 1;
    RegMM345C_MM358CMask.HFP_LP = 0;
    RegMM345C_MM358CMask.HSA_LP = 0;
    RegMM345C_MM358CMask.HBP_LP = 0;
    RegMM345C_MM358CMask.DSI_Video_Mode_Enable = 0;
    cbMMIOWriteReg32(pcbe, VideoReg1Index, RegMM345C_MM358CValue.Value, RegMM345C_MM358CMask.Value);

    if (pDSIConfig->DSIMode == CBIOS_DSI_CMDMODE)
    {
        RegMM34C0_MM34C8Value.Value = 0;
        RegMM34C0_MM34C8Value.DSI_CMD_Mode_Enable = 1;
        RegMM34C0_MM34C8Mask.Value = 0xFFFFFFFF;
        RegMM34C0_MM34C8Mask.DSI_CMD_Mode_Enable = 0;
        cbMMIOWriteReg32(pcbe, VideoReg5Index, RegMM34C0_MM34C8Value.Value, RegMM34C0_MM34C8Mask.Value);
    }
    else
    {
        RegMM34C0_MM34C8Value.Value = 0;
        RegMM34C0_MM34C8Value.DSI_CMD_Mode_Enable = 0;
        RegMM34C0_MM34C8Mask.Value = 0xFFFFFFFF;
        RegMM34C0_MM34C8Mask.DSI_CMD_Mode_Enable = 0;
        cbMMIOWriteReg32(pcbe, VideoReg5Index, RegMM34C0_MM34C8Value.Value, RegMM34C0_MM34C8Mask.Value);
    }
    cbTraceExit(DSI);
}

CBIOS_VOID cbDSI_ControllerInit(PCBIOS_VOID pvcbe, CBIOS_U32 IGAIndex, CBIOS_DSI_INDEX DSIIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM3400_MM3530 RegMM3400_MM3530Value;
    REG_MM3400_MM3530 RegMM3400_MM3530Mask;
    REG_MM3408_MM3538 RegMM3408_MM3538Value;
    REG_MM3408_MM3538 RegMM3408_MM3538Mask;
    REG_MM34A0        RegMM34A0Value;
    REG_MM34A0        RegMM34A0Mask;
    PCBIOS_DSI_CONFIG pDSIConfig = CBIOS_NULL;
    PCBIOS_DSI_PANEL_TABLE pDSIPanelTbl = CBIOS_NULL;
    PCBIOS_DSI_PARAMS pDSIParams = CBIOS_NULL;
    CBIOS_U32         IrqBitMask = DSI_IRQ_TE_TRIGGER | DSI_IRQ_OTHER_TRIGGER | DSI_IRQ_DSI_READBACK;
    CBIOS_U32         DMARegIndex = 0x3400, CtrlRegIndex = 0x3408;
    PCBIOS_DEVICE_COMMON pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, CBIOS_TYPE_DSI);
    cbTraceEnter(DSI);

    pDSIParams = &(pDevCommon->DeviceParas.DSIDevice);
    pDSIConfig = &(pDSIParams->DSIPanelDesc.DSIConfig);
    pDSIPanelTbl = &(pDSIParams->DSIPanelDesc.DSIPanelTbl);


    if (DSIIndex == DSI_INDEX0)
    {
        DMARegIndex  = 0x3400;
        CtrlRegIndex = 0x3408;
    }
    else if (DSIIndex == DSI_INDEX1)
    {
        DMARegIndex  = 0x3530;
        CtrlRegIndex = 0x3538;
    }

    RegMM3400_MM3530Value.Value = 0;
    RegMM3400_MM3530Mask.Value = 0xFFFFFFFF;
    RegMM3408_MM3538Value.Value = 0;
    RegMM3408_MM3538Mask.Value = 0xFFFFFFFF;

    //DSI clock Enable
    RegMM34A0Value.Value = 0;
    RegMM34A0Value.DSI_Clock_Enable = 1;
    RegMM34A0Mask.Value = 0xFFFFFFFF;
    RegMM34A0Mask.DSI_Clock_Enable = 0;
    cbMMIOWriteReg32(pcbe, 0x34a0, RegMM34A0Value.Value, RegMM34A0Mask.Value);

    // first set DSI mode bit load control bit
    RegMM3408_MM3538Value.DSI_Mode_Bit_Load_Control = 1;    // directly load
    RegMM3408_MM3538Value.IGA_VSYNC_Previous_Signal_Select = 2;
    RegMM3408_MM3538Value.IGA_VSYNC_Previous_Time =  7;
    RegMM3408_MM3538Mask.DSI_Mode_Bit_Load_Control = 0;
    RegMM3408_MM3538Mask.IGA_VSYNC_Previous_Signal_Select = 0;
    RegMM3408_MM3538Mask.IGA_VSYNC_Previous_Time = 0;
    cbMMIOWriteReg32(pcbe, CtrlRegIndex, RegMM3408_MM3538Value.Value, RegMM3408_MM3538Mask.Value);

    // set data lane num, clock lane mode and turn on clk lane high speed
    cbDSI_SetDataLaneNum(pcbe, pDSIPanelTbl->LaneNum, DSIIndex);
    cbDSI_SetClkLaneMode(pcbe, pDSIConfig->ClkLaneMode, DSIIndex);

    // turn on clk lane high speed
    cbDSI_ClkLaneHSEnableDisable(pcbe, CBIOS_TRUE, DSIIndex);

    cbDSI_EoTpEnableDisable(pcbe, pDSIConfig, DSIIndex);
    cbDSI_SetTimeOut(pcbe, pDSIConfig, DSIIndex);

    // still need to set DMA threshold?
    RegMM3400_MM3530Value.DMA_FIFO_Threshold = pDSIConfig->DMAThreshold;
    RegMM3400_MM3530Mask.DMA_FIFO_Threshold = 0;
    cbMMIOWriteReg32(pcbe, DMARegIndex, RegMM3400_MM3530Value.Value, RegMM3400_MM3530Mask.Value);

    cbDSI_SetOutBpp(pcbe, pDSIPanelTbl->OutBpp, DSIIndex);

    // init phy
    cbDSI_SetPhyParams(pcbe, pDSIParams, IGAIndex, DSIIndex);

    if (pDSIConfig->DSIMode == CBIOS_DSI_VIDEOMODE)
    {
        cbDSI_SyncEndEnableDisable(pcbe, pDSIConfig, DSIIndex);
    }

    // enable irq
    cbDSI_IrqEnableDisable(pcbe, IrqBitMask, CBIOS_TRUE, DSIIndex);

    RegMM3408_MM3538Value.Value = 0;
    RegMM3408_MM3538Value.DSI_Output_Enable = 1;
    RegMM3408_MM3538Mask.Value = 0xFFFFFFFF;
    RegMM3408_MM3538Mask.DSI_Output_Enable = 0;
    cbMMIOWriteReg32(pcbe, CtrlRegIndex, RegMM3408_MM3538Value.Value, RegMM3408_MM3538Mask.Value);

    //cbDSI_SetAddrMode(pcbe, 0, DSIIndex);

    cbDSI_SelectMode(pcbe, pDSIConfig->DSIMode, DSIIndex);

    //cbDSI_SetMaxReturnSize(pcbe, CBIOS_DSI_MAX_RETURN_PACKET_SIZE, DSIIndex);

    //cbDSI_EnableDisableTE(pcbe, CBIOS_TRUE, 0, 0, DSIIndex);

    cbTraceExit(DSI);
}

CBIOS_VOID cbDSI_Init(PCBIOS_VOID pvcbe, CBIOS_U32 IGAIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DEVICE_COMMON pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, CBIOS_TYPE_DSI);
    PCBIOS_DSI_PANEL_DESC pPanelDesc = &pDevCommon->DeviceParas.DSIDevice.DSIPanelDesc;
    cbTraceEnter(DSI);

    cbDSI_ControllerInit(pcbe, IGA1, DSI_INDEX0);

    if(pPanelDesc->DSIConfig.isDualChannel)
    {
        cbDSI_ControllerInit(pcbe, IGA1, DSI_INDEX1);
    }

    if((pPanelDesc->DSIConfig.DSIMode == CBIOS_DSI_CMDMODE) && (pPanelDesc->DSIConfig.TEType == CBIOS_DSI_TE_PAD))
    {
        REG_MM349C RegMM349CValue;
        REG_MM349C RegMM349CMask;
        //detect whether GFX GPIO6 used as GPIO mode or TE Pad mode
        //For GFX GPIO6 we will only read/write through 0xd8110052; not by SR_4A[6:5]
        // we won't force exec cbFreeGPIO(pvcbe, CBIOS_GPIO_GFX, 6); only detect and report this fatal error
        //temporary skip this judgement
        /*
        dwGP18EnableReg = cbReadRegisterU32(pcbe, CBIOS_REGISTER_GPIO, 0x0050);
        if(dwGP18EnableReg & BIT19)
        {
            cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_Init: Detect GFX GPIO6 already enabled as GPIO Mode!\n"));
        }
        */
        RegMM349CValue.Value = 0;
        RegMM349CValue.DSI_TE_IEN = 1;
        RegMM349CMask.Value = 0xFFFFFFFF;
        RegMM349CMask.DSI_TE_IEN = 0;
        cbMMIOWriteReg32(pcbe, 0x349c, RegMM349CValue.Value, RegMM349CMask.Value);
    }
    if(pPanelDesc->DSIConfig.DSIMode == CBIOS_DSI_CMDMODE)
    {
        REG_MM3294_MM32AC RegMM3294Value;
        REG_MM3294_MM32AC RegMM3294Mask;

        RegMM3294Value.Value = 0;
        RegMM3294Value.Vcnt_Reset_Value_Sel = 1;
        RegMM3294Mask.Value = 0xFFFFFFFF;
        RegMM3294Mask.Vcnt_Reset_Value_Sel = 0;
        cbMMIOWriteReg32(pcbe, 0x3294, RegMM3294Value.Value, RegMM3294Mask.Value);
    }

    cbTraceExit(DSI);
}


CBIOS_VOID cbDSI_ControllerOnOff(PCBIOS_VOID pvcbe, CBIOS_BOOL bTurnOn, CBIOS_DSI_INDEX DSIIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM3408_MM3538 RegMM3408_MM3538Value;
    REG_MM3408_MM3538 RegMM3408_MM3538Mask;
    REG_MM345C_MM358C RegMM345C_MM358CValue;
    REG_MM345C_MM358C RegMM345C_MM358CMask;
    REG_MM3448_MM3578 RegMM3448_MM3578Value;
    REG_MM3448_MM3578 RegMM3448_MM3578Mask;
    REG_MM34C0_MM34C8 RegMM34C0_MM34C8Value;
    REG_MM34C0_MM34C8 RegMM34C0_MM34C8Mask;
    PCBIOS_DSI_PARAMS pDSIParams = CBIOS_NULL;
    PCBIOS_DSI_CONFIG pDSIConfig = CBIOS_NULL;
    PCBIOS_DSI_PANEL_TABLE pDSIPanelTbl = CBIOS_NULL;
    CBIOS_U32 CtrlRegIndex = 0x3408, VideoRegIndex = 0x345C, EPhyRegIndex = 0x3448, VideoReg5Index = 0x34C0;
    PCBIOS_DEVICE_COMMON pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, CBIOS_TYPE_DSI);
    cbTraceEnter(DSI);

    if (DSIIndex == DSI_INDEX0)
    {
        CtrlRegIndex  = 0x3408;
        VideoRegIndex = 0x345C;
        EPhyRegIndex  = 0x3448;
        VideoReg5Index = 0x34C0;
    }
    else if (DSIIndex == DSI_INDEX1)
    {
        CtrlRegIndex  = 0x3538;
        VideoRegIndex = 0x358C;
        EPhyRegIndex  = 0x3578;
        VideoReg5Index = 0x34C8;
    }

    RegMM3408_MM3538Value.Value = 0;
    RegMM3408_MM3538Mask.Value = 0xFFFFFFFF;
    RegMM345C_MM358CValue.Value = 0;
    RegMM345C_MM358CMask.Value = 0xFFFFFFFF;
    RegMM3448_MM3578Value.Value = 0;
    RegMM3448_MM3578Mask.Value = 0xFFFFFFFF;
    RegMM34C0_MM34C8Value.Value = 0;
    RegMM34C0_MM34C8Mask.Value = 0xFFFFFFFF;
    pDSIParams = &(pDevCommon->DeviceParas.DSIDevice);
    pDSIConfig = &(pDSIParams->DSIPanelDesc.DSIConfig);
    pDSIPanelTbl = &(pDSIParams->DSIPanelDesc.DSIPanelTbl);

    if (bTurnOn)
    {

        RegMM3408_MM3538Value.DSI_Output_Enable = 1;
        RegMM3408_MM3538Mask.DSI_Output_Enable = 0;

        if (pDSIConfig->DSIMode == CBIOS_DSI_VIDEOMODE)
        {
            RegMM345C_MM358CValue.DSI_Video_Mode_Enable = 1;
            RegMM345C_MM358CMask.DSI_Video_Mode_Enable = 0;
            RegMM34C0_MM34C8Value.DSI_CMD_Mode_Enable = 0;
            RegMM34C0_MM34C8Mask.DSI_CMD_Mode_Enable = 0;
        }
        else if (pDSIConfig->DSIMode == CBIOS_DSI_CMDMODE)
        {
            RegMM345C_MM358CValue.DSI_Video_Mode_Enable = 0;
            RegMM345C_MM358CMask.DSI_Video_Mode_Enable = 0;
            RegMM34C0_MM34C8Value.DSI_CMD_Mode_Enable = 1;
            RegMM34C0_MM34C8Mask.DSI_CMD_Mode_Enable = 0;
        }

        cbMMIOWriteReg32(pcbe, CtrlRegIndex, RegMM3408_MM3538Value.Value, RegMM3408_MM3538Mask.Value);
        cbMMIOWriteReg32(pcbe, VideoRegIndex, RegMM345C_MM358CValue.Value, RegMM345C_MM358CMask.Value);
        cbMMIOWriteReg32(pcbe, VideoReg5Index, RegMM34C0_MM34C8Value.Value, RegMM34C0_MM34C8Mask.Value);

    }
    else    // turn off
    {

        RegMM3408_MM3538Value.DSI_Output_Enable = 0;
        RegMM3408_MM3538Mask.DSI_Output_Enable = 0;

        if (pDSIConfig->DSIMode == CBIOS_DSI_VIDEOMODE)
        {
            RegMM345C_MM358CValue.DSI_Video_Mode_Enable = 0;
            RegMM345C_MM358CMask.DSI_Video_Mode_Enable = 0;
        }
        else if (pDSIConfig->DSIMode == CBIOS_DSI_CMDMODE)
        {
            RegMM34C0_MM34C8Value.DSI_CMD_Mode_Enable = 0;
            RegMM34C0_MM34C8Mask.DSI_CMD_Mode_Enable = 0;
        }

        RegMM3448_MM3578Value.Power_down_PLL = 0;           // power off EPHY PLL
        RegMM3448_MM3578Mask.Power_down_PLL = 0;
        RegMM3448_MM3578Value.Clock_Lane_Power_Down = 0;    // power off clock lane
        RegMM3448_MM3578Mask.Clock_Lane_Power_Down = 0;
        RegMM3448_MM3578Value.Power_Down_Data_Lane_0 = 0;   // power off data lane 0
        RegMM3448_MM3578Mask.Power_Down_Data_Lane_0 = 0;

        if (pDSIPanelTbl->LaneNum > 1)
        {
            RegMM3448_MM3578Value.Power_Down_Data_Lane_1 = 0;
            RegMM3448_MM3578Mask.Power_Down_Data_Lane_1 = 0;
        }

        if (pDSIPanelTbl->LaneNum > 2)
        {
            RegMM3448_MM3578Value.Power_Down_Data_Lane_2 = 0;
            RegMM3448_MM3578Mask.Power_Down_Data_Lane_2 = 0;
        }

        if (pDSIPanelTbl->LaneNum > 3)
        {
            RegMM3448_MM3578Value.Power_Down_Data_Lane_3 = 0;
            RegMM3448_MM3578Mask.Power_Down_Data_Lane_3 = 0;
        }

        RegMM3448_MM3578Value.Power_Down_LP_RX = 0;         // power off lp-rx
        RegMM3448_MM3578Mask.Power_Down_LP_RX = 0;
        cbMMIOWriteReg32(pcbe, CtrlRegIndex, RegMM3408_MM3538Value.Value, RegMM3408_MM3538Mask.Value);
        cbMMIOWriteReg32(pcbe, VideoRegIndex, RegMM345C_MM358CValue.Value, RegMM345C_MM358CMask.Value);
        cbMMIOWriteReg32(pcbe, VideoReg5Index, RegMM34C0_MM34C8Value.Value, RegMM34C0_MM34C8Mask.Value);
        cbMMIOWriteReg32(pcbe, EPhyRegIndex, RegMM3448_MM3578Value.Value, RegMM3448_MM3578Mask.Value);
    }

    cbTraceExit(DSI);
}

static CBIOS_STATUS cbDSI_DisplayUpdateByDMA(PCBIOS_VOID pvcbe, PCBIOS_DSI_DMAUPDATE_PARA pUpdateParams, CBIOS_DSI_INDEX DSIIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS Status = CBIOS_OK;
    CBIOS_U32    UpdateSize = 0, WriteStride = 0,  FetchStride = 0, StartSize = 0, Counter = 0;
    CBIOS_U8     CmdType = 0, DCSCmd = 0, DataId = 0;
    PCBIOS_DSI_WINDOW  pUpdateWin = CBIOS_NULL;
    PCBIOS_DEVICE_COMMON pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, CBIOS_TYPE_DSI);
    CBIOS_U8    OutBpp = (CBIOS_U8)pDevCommon->DeviceParas.DSIDevice.DSIPanelDesc.DSIPanelTbl.OutBpp;
    CBIOS_U32  XRes = pDevCommon->DeviceParas.DSIDevice.DSIPanelDesc.DSIPanelTbl.PanelTiming.XResolution;
    CBIOS_U32  AlignedXRes = 0;
    CBIOS_U8    VirtualCh = 0; //might need fix
    CBIOS_U32   FBAddr = pUpdateParams->DMABaseAddr;
    CBIOS_U32     StartCol = 0, EndCol = 0, StartRow = 0,  EndRow = 0;
    CBIOS_U32     i  = 0;
    REG_MM3440_MM3570  RegMM3440Value, RegMM3440Mask;
    REG_MM3480         RegMM3480Value, RegMM3480Mask;
    cbTraceEnter(DSI);

    pUpdateWin = &(pUpdateParams->DMAUpdateWindow);

    // set the updated window on Panel
    StartCol =   pUpdateWin->XStart;
    EndCol =   pUpdateWin->XStart + pUpdateWin->WinWidth  -  1;
    StartRow = pUpdateWin->YStart;
    EndRow =  pUpdateWin->YStart + pUpdateWin->WinHeight - 1;

    cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "cbDSI_DisplayUpdateByDMA: pUpdateWin->XStart: %d \n", pUpdateWin->XStart));
    cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "cbDSI_DisplayUpdateByDMA: pUpdateWin->YStart: %d \n", pUpdateWin->YStart));
    cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "cbDSI_DisplayUpdateByDMA: pUpdateWin->WinWidth: %d \n", pUpdateWin->WinWidth));
    cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "cbDSI_DisplayUpdateByDMA: pUpdateWin->WinHeight: %d \n", pUpdateWin->WinHeight));

    cbDSI_SetColumnAddr(pcbe, (CBIOS_U16)StartCol, (CBIOS_U16)EndCol, DSIIndex);
    cbDSI_SetPageAddr(pcbe, (CBIOS_U16)StartRow, (CBIOS_U16)EndRow, DSIIndex);

    //select DMA FIFO:  DSI1 share SS1 FIFO; DSI2 share TS1 FIFO
    RegMM3480Value.Value = 0;
    RegMM3480Mask.Value = 0xFFFFFFFF;

    if (DSIIndex == DSI_INDEX0)
    {
        RegMM3480Value.SS_DMA1_SEL = 1;
        RegMM3480Mask.SS_DMA1_SEL = 0;
    }
    else
    {
        RegMM3480Value.TS_DMA2_SEL = 1;
        RegMM3480Mask.TS_DMA2_SEL = 0;
    }

    cbMMIOWriteReg32(pcbe, 0x3480, RegMM3480Value.Value, RegMM3480Mask.Value);


    //set DMA bits
    RegMM3440Value.Value = 0;
    RegMM3440Mask.Value = 0xFFFFFFFF;

    //As OutBpp in Our code only include the RGB data, for DMA fetch pixel ought to add the Alpha
    if(OutBpp == 24)
    {
        OutBpp = 32;
    }

    if (OutBpp == 32)
    {
        RegMM3440Value.DMA_Pixel_Dataformat_32bpp = 1;
        RegMM3440Mask.DMA_Pixel_Dataformat_32bpp = 0;
        cbMMIOWriteReg32(pcbe, 0x3440, RegMM3440Value.Value, RegMM3440Mask.Value);
    }
    else if (OutBpp == 16)
    {
        RegMM3440Value.DMA_Pixel_Dataformat_16bpp = 1;
        RegMM3440Mask.DMA_Pixel_Dataformat_16bpp = 0;
        cbMMIOWriteReg32(pcbe, 0x3440, RegMM3440Value.Value, RegMM3440Mask.Value);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_DisplayUpdateByDMA: Wrong OutBpp for DMA test! OutBpp: %d \n", OutBpp));
    }

    if(pUpdateParams->isDMAAligned)
    {
        UpdateSize = pUpdateParams->DMAStride * pUpdateWin->WinHeight  * OutBpp / 8;

        if(UpdateSize % 2)
        {
            cbDebugPrint((MAKE_LEVEL(DSI, WARNING), "cbDSI_DisplayUpdateByDMA: ought to be  2 pixel data aligned \n"));
        }

        WriteStride = pUpdateWin->WinWidth  * OutBpp / 8;
        FetchStride  = (XRes * OutBpp / 8 + 256 - 1) & ~(256 - 1);
        AlignedXRes = FetchStride / (OutBpp / 8);
        FBAddr = pUpdateParams->DMABaseAddr + (pUpdateWin->YStart * AlignedXRes +  pUpdateWin->XStart ) * OutBpp / 8;

        cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "cbDSI_DisplayUpdateByDMA: UpdateSize: %d, WriteStride: %d (Byte)\n", UpdateSize, WriteStride ));

        for(i = 0; i < pUpdateWin->WinHeight; i++)
        {
            if (i == 0)
            {
                DCSCmd = DSI_DCS_WRITE_MEMORY_START;
            }
            else
            {
                DCSCmd = DSI_DCS_WRITE_MEMORY_CONTINUE;
            }

            CmdType = DSI_CMD_TYPE_DMA | DSI_CMD_TYPE_LAST_GROUP ;
            DataId = DSI_DCS_LONG_WRITE;

            cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "DMA, i:  %d \n", i));

            cbDSI_SendDMAPacket(pcbe, VirtualCh, DataId, (CBIOS_U16)WriteStride, CmdType, FBAddr, DCSCmd, DSIIndex);
            FBAddr += FetchStride;
        }
    }
    else
    {
        UpdateSize = pUpdateWin->WinWidth * pUpdateWin->WinHeight * OutBpp / 8;

        if(UpdateSize % 2)
        {
            cbDebugPrint((MAKE_LEVEL(DSI, WARNING), "cbDSI_DisplayUpdateByDMA: ought to be  2 pixel data aligned \n"));
        }

        cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "cbDSI_DisplayUpdateByDMA: UpdateSize: %d, WriteStride: %d  (Byte)\n", UpdateSize, pUpdateParams->DMAStride));

        FBAddr = pUpdateParams->DMABaseAddr;
        WriteStride = pUpdateParams->DMAStride;

        if(UpdateSize < WriteStride)
        {
            StartSize = UpdateSize;
        }
        else
        {
            StartSize = WriteStride;
        }

        UpdateSize = UpdateSize - StartSize;
        CmdType = DSI_CMD_TYPE_DMA | DSI_CMD_TYPE_LAST_GROUP ;
        DataId = DSI_DCS_LONG_WRITE;
        DCSCmd = DSI_DCS_WRITE_MEMORY_START;
        cbDSI_SendDMAPacket(pcbe, VirtualCh, DataId, (CBIOS_U16)StartSize, CmdType, FBAddr, DCSCmd, DSIIndex);

        DCSCmd = DSI_DCS_WRITE_MEMORY_CONTINUE;

        while(UpdateSize > 0)
        {
            FBAddr += StartSize;

            if(UpdateSize < WriteStride)
            {
                StartSize = UpdateSize;
            }
            else
            {
                StartSize = WriteStride;
            }

            UpdateSize = UpdateSize - StartSize;

            cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "DMA Counter:  %d  \n", Counter++));
            cbDSI_SendDMAPacket(pcbe, VirtualCh, DataId, (CBIOS_U16)StartSize, CmdType, FBAddr, DCSCmd, DSIIndex);
        }
    }

    //check DMA Bit
    Status = cbDSI_WaitDMAFree(pcbe);
    cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "cbDSI_DisplayUpdateByDMA: DMAStatus:  %d \n", Status));

    cbTraceExit(DSI);
    return Status;
}

static CBIOS_STATUS cbDSI_DisplayPartialUpdate(PCBIOS_VOID pvcbe, PCBIOS_DSI_HOSTUPDATE_PARA pUpdateParams, CBIOS_BOOL bPartial, CBIOS_DSI_INDEX DSIIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS Status = CBIOS_OK;
    PCBIOS_DSI_WINDOW  pUpdateWin = CBIOS_NULL, pLastUpdateWin = CBIOS_NULL;
    PCBIOS_DEVICE_COMMON pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, CBIOS_TYPE_DSI);
    PCBIOS_DSI_PANEL_DESC pPanelDesc = &pDevCommon->DeviceParas.DSIDevice.DSIPanelDesc;
    CBIOS_U16    StartCol, EndCol, StartRow, EndRow;
    REG_MM3408_MM3538  RegMM3408_MM3538Value, RegMM3408_MM3538Mask;
    REG_MM34BC_MM35EC  RegMM34BC_MM35ECValue, RegMM34BC_MM35ECMask;
    REG_MM34C0_MM34C8  RegMM34C0_MM34C8Value, RegMM34C0_MM34C8Mask;
    REG_MM34C4_MM34CC  RegMM34C4_MM34CCValue, RegMM34C4_MM34CCMask;
    CBIOS_U32          MiscRegIndex = 0x34BC, CtrlRegIndex = 0x3408, PosRegIndex = 0x34C0, SizeRegIndex = 0x34C4;
    PCBIOS_TIMING_ATTRIB pTargetTiming = &(pDevCommon->DeviceParas.DSIDevice.TargetTiming);
    CBIOS_U32 MaxWidth = pTargetTiming->XRes;
    CBIOS_U32 MaxHeight = pTargetTiming->YRes;
    cbTraceEnter(DSI);

    if (pPanelDesc->DSIConfig.isDualChannel)
    {
        MaxWidth /= 2;
    }

    pUpdateWin = &(pUpdateParams->HostUpdateWindow[DSIIndex]);
    pLastUpdateWin = &(pDevCommon->DeviceParas.DSIDevice.HostUpdatePara.HostUpdateWindow[DSIIndex]);

    if (DSIIndex == DSI_INDEX0)
    {
        MiscRegIndex = 0x34BC;
        CtrlRegIndex = 0x3408;
        PosRegIndex  = 0x34C0;
        SizeRegIndex = 0x34C4;
    }
    else if (DSIIndex == DSI_INDEX1)
    {
        MiscRegIndex = 0x35EC;
        CtrlRegIndex = 0x3538;
        PosRegIndex  = 0x34C8;
        SizeRegIndex = 0x34CC;
    }

    if (bPartial)    // 1: normal->partial mode; 2.partial mode, update para changed
    {
        // IGA programming
        RegMM34BC_MM35ECValue.Value = 0;
        RegMM34BC_MM35ECMask.Value = 0xFFFFFFFF;
        RegMM34BC_MM35ECValue.DSI_Partial_Timing_Enable = 1;
        RegMM34BC_MM35ECMask.DSI_Partial_Timing_Enable = 0;
        RegMM34BC_MM35ECValue.Y_START = pUpdateWin->YStart;
        RegMM34BC_MM35ECMask.Y_START = 0;
        RegMM34BC_MM35ECValue.Y_HEIGHT = pUpdateWin->WinHeight;
        RegMM34BC_MM35ECMask.Y_HEIGHT = 0;
        cbMMIOWriteReg32(pcbe, MiscRegIndex, RegMM34BC_MM35ECValue.Value, RegMM34BC_MM35ECMask.Value);

        // DSI programming
        RegMM34C0_MM34C8Value.Value = 0;
        RegMM34C0_MM34C8Mask.Value = 0xFFFFFFFF;
        RegMM34C0_MM34C8Value.X_START = pUpdateWin->XStart;
        RegMM34C0_MM34C8Mask.X_START = 0;
        RegMM34C0_MM34C8Value.Y_START = pUpdateWin->YStart;
        RegMM34C0_MM34C8Mask.Y_START = 0;
        cbMMIOWriteReg32(pcbe, PosRegIndex, RegMM34C0_MM34C8Value.Value, RegMM34C0_MM34C8Mask.Value);

        RegMM34C4_MM34CCValue.Value = 0;
        RegMM34C4_MM34CCMask.Value = 0xFFFFFFFF;
        RegMM34C4_MM34CCValue.Width = pUpdateWin->WinWidth;
        RegMM34C4_MM34CCMask.Width = 0;
        RegMM34C4_MM34CCValue.Height = pUpdateWin->WinHeight;
        RegMM34C4_MM34CCMask.Height = 0;
        cbMMIOWriteReg32(pcbe, SizeRegIndex, RegMM34C4_MM34CCValue.Value, RegMM34C4_MM34CCMask.Value);

        Status = cbDSI_SetPXL_WC(pcbe, pDevCommon->DeviceParas.DSIDevice.DSIPanelDesc.DSIPanelTbl.OutBpp, pUpdateWin->WinWidth, DSIIndex);


        RegMM3408_MM3538Value.Value = 0;
        RegMM3408_MM3538Mask.Value = 0xFFFFFFFF;
        RegMM3408_MM3538Value.DCS_DSI_16BPP = 0;
        RegMM3408_MM3538Mask.DCS_DSI_16BPP = 0;
        RegMM3408_MM3538Value.ENTER_HS = pUpdateParams->isEnterHS;
        RegMM3408_MM3538Mask.ENTER_HS = 0;
        cbMMIOWriteReg32(pcbe, CtrlRegIndex, RegMM3408_MM3538Value.Value, RegMM3408_MM3538Mask.Value);


        // enter partial mode and send frame data
        StartCol = pUpdateWin->XStart;
        EndCol = StartCol + pUpdateWin->WinWidth  -  1;
        StartRow = pUpdateWin->YStart;
        EndRow = StartRow + pUpdateWin->WinHeight - 1;

        cbDSI_SetColumnAddr(pcbe, StartCol, EndCol, DSIIndex);
        cbDSI_SetPageAddr(pcbe, StartRow, EndRow, DSIIndex);
        //cbDSI_DCSSwitchMode(pcbe, CBIOS_DSI_DCS_PARTIAL_MODE, CBIOS_TRUE, DSIIndex);  //not applied for NT35595

        // save update window paras to pcbe
        cb_memcpy(pLastUpdateWin, pUpdateWin,  sizeof(CBIOS_DSI_WINDOW));

    }
    else    // 1: partial->normal mode;
    {
        // IGA programming
        RegMM34BC_MM35ECValue.Value = 0;
        RegMM34BC_MM35ECMask.Value = 0xFFFFFFFF;
        RegMM34BC_MM35ECValue.DSI_Partial_Timing_Enable = 0;
        RegMM34BC_MM35ECMask.DSI_Partial_Timing_Enable = 0;
        RegMM34BC_MM35ECValue.Y_START = 0;
        RegMM34BC_MM35ECMask.Y_START = 0;
        RegMM34BC_MM35ECValue.Y_HEIGHT = 0;
        RegMM34BC_MM35ECMask.Y_HEIGHT = 0;
        cbMMIOWriteReg32(pcbe, 0x34BC, RegMM34BC_MM35ECValue.Value, RegMM34BC_MM35ECMask.Value);

        // DSI programming
        RegMM34C0_MM34C8Value.Value = 0;
        RegMM34C0_MM34C8Mask.Value = 0xFFFFFFFF;
        RegMM34C0_MM34C8Value.X_START = 0;
        RegMM34C0_MM34C8Mask.X_START = 0;
        RegMM34C0_MM34C8Value.Y_START = 0;
        RegMM34C0_MM34C8Mask.Y_START = 0;
        cbMMIOWriteReg32(pcbe, PosRegIndex, RegMM34C0_MM34C8Value.Value, RegMM34C0_MM34C8Mask.Value);

        RegMM34C4_MM34CCValue.Value = 0;
        RegMM34C4_MM34CCMask.Value = 0xFFFFFFFF;
        RegMM34C4_MM34CCValue.Width = 0;
        RegMM34C4_MM34CCMask.Width = 0;
        RegMM34C4_MM34CCValue.Height = 0;
        RegMM34C4_MM34CCMask.Height = 0;
        cbMMIOWriteReg32(pcbe, SizeRegIndex, RegMM34C4_MM34CCValue.Value, RegMM34C4_MM34CCMask.Value);

        Status = cbDSI_SetPXL_WC(pcbe, pDevCommon->DeviceParas.DSIDevice.DSIPanelDesc.DSIPanelTbl.OutBpp, pTargetTiming->HorDisEnd, DSIIndex);

        // save update window paras to pcbe
        cb_memcpy(pLastUpdateWin, pUpdateWin, sizeof(CBIOS_DSI_WINDOW));

        //enter normal mode and send frame data
        StartCol = 0;
        EndCol = MaxWidth -  1;
        StartRow = 0;
        EndRow = MaxHeight  -  1;
        cbDSI_SetColumnAddr(pcbe, StartCol, EndCol, DSIIndex);
        cbDSI_SetPageAddr(pcbe, StartRow, EndRow, DSIIndex);
        cbDSI_DCSSwitchMode(pcbe, CBIOS_DSI_DCS_NORMAL_MODE, CBIOS_TRUE, DSIIndex);
    }

    cbTraceExit(DSI);
    return Status;
}

static CBIOS_STATUS cbDSI_DisplayUpdateByHost(PCBIOS_VOID pvcbe, PCBIOS_DSI_HOSTUPDATE_PARA pUpdateParams,  CBIOS_DSI_INDEX DSIIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS Status = CBIOS_OK;
    CBIOS_BOOL   isPartialUpdate, isWindowChanged;
    PCBIOS_DSI_WINDOW  pUpdateWin = CBIOS_NULL, pLastUpdateWin = CBIOS_NULL;
    PCBIOS_DEVICE_COMMON pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, CBIOS_TYPE_DSI);
    PCBIOS_TIMING_ATTRIB pTargetTiming = &(pDevCommon->DeviceParas.DSIDevice.TargetTiming);
    cbTraceEnter(DSI);

    pUpdateWin = &(pUpdateParams->HostUpdateWindow[DSIIndex]);
    pLastUpdateWin = &(pDevCommon->DeviceParas.DSIDevice.HostUpdatePara.HostUpdateWindow[DSIIndex]);

    // first judge is partial update or normal update
    if ((pUpdateWin->XStart == 0) && (pUpdateWin->YStart == 0)
        && (pUpdateWin->WinWidth == pTargetTiming->XRes)
        && (pUpdateWin->WinHeight == pTargetTiming->YRes))
    {
        isPartialUpdate = CBIOS_FALSE;
    }
    else
    {
        isPartialUpdate = CBIOS_TRUE;
    }

    // then judge whether the window changed
    if ((pUpdateWin->XStart == pLastUpdateWin->XStart)
        && (pUpdateWin->YStart == pLastUpdateWin->YStart)
        && (pUpdateWin->WinWidth == pLastUpdateWin->WinWidth)
        && (pUpdateWin->WinHeight == pLastUpdateWin->WinHeight))
    {
        isWindowChanged = CBIOS_FALSE;
    }
    else
    {
        isWindowChanged = CBIOS_TRUE;
    }

    if ((pUpdateWin->XStart == 0) && (pUpdateWin->YStart == 0)
        && (pUpdateWin->WinWidth == 0) && (pUpdateWin->WinHeight == 0))
    {
        isWindowChanged = CBIOS_FALSE;
    }

    cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "pUpdateWin->XStart :  %d \n", pUpdateWin->XStart ));
    cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "pUpdateWin->YStart:  %d \n", pUpdateWin->YStart));
    cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "pUpdateWin->WinWidth:  %d \n", pUpdateWin->WinWidth));
    cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "pUpdateWin->WinHeight:  %d \n", pUpdateWin->WinHeight));
    cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "isWindowChanged:  %d \n", isWindowChanged));
    cbDebugPrint((MAKE_LEVEL(DSI, DEBUG), "isPartialUpdate:  %d \n", isPartialUpdate));

    if ((pUpdateParams->isEnablePartialUpdate) && (isWindowChanged))
    {
        cbDSI_DisplayPartialUpdate(pcbe, pUpdateParams, isPartialUpdate, DSIIndex);
    }

    cbDSI_SendOneFrameData(pcbe, DSIIndex);

    cbTraceExit(DSI);
    return Status;
}

static CBIOS_STATUS cbDSI_GetPanelUpdatePara(PCBIOS_VOID pvcbe,  PCBIOS_DSI_PANELUPDATE_PARA pPanelUpdateParams)
{
    CBIOS_STATUS Status = CBIOS_OK;
    pPanelUpdateParams->Size = sizeof(CBIOS_DSI_PANELUPDATE_PARA);
    pPanelUpdateParams->PanelClumnAlign  = 2;   //clumn start align
    pPanelUpdateParams->PanelRowAlign    = 2;   //page addr start align
    pPanelUpdateParams->PanelWidthAlign  = 2;   //window width need to be divisible by 2
    pPanelUpdateParams->PanelHeightAlign = 2;   //window height need to be divisible by 2

    return Status;
}

static CBIOS_STATUS cbDSI_AdjustPanelUpdateWindow(PCBIOS_VOID pvcbe,  PCBIOS_DSI_PANELUPDATE_PARA pPanelUpdateParams, PCBIOS_DSI_WINDOW  pUpdateWin)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS Status = CBIOS_OK;
    PCBIOS_DEVICE_COMMON pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, CBIOS_TYPE_DSI);
    PCBIOS_DSI_PANEL_DESC pPanelDesc = &pDevCommon->DeviceParas.DSIDevice.DSIPanelDesc;
    PCBIOS_TIMING_ATTRIB pTargetTiming = &(pDevCommon->DeviceParas.DSIDevice.TargetTiming);
    CBIOS_U32 MaxWidth = pTargetTiming->XRes;
    CBIOS_U32 MaxHeight = pTargetTiming->YRes;

    if (pPanelDesc->DSIConfig.isDualChannel)
    {
        MaxWidth /= 2;
    }

    if(pPanelUpdateParams && pUpdateWin)
    {
        //check UpdateWindow  Para
        if(pUpdateWin->XStart > MaxWidth)
        {
            cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_AdjustPanelUpdateWindow Invalid pUpdateWin->XStart! \n"));
            Status = CBIOS_ER_INVALID_PARAMETER;
        }
        if(pUpdateWin->YStart > MaxHeight)
        {
            cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_AdjustPanelUpdateWindow Invalid pUpdateWin->YStart! \n"));
            Status = CBIOS_ER_INVALID_PARAMETER;
        }
        if((CBIOS_U32)(pUpdateWin->XStart + pUpdateWin->WinWidth) > MaxWidth)
        {
            pUpdateWin->WinWidth = MaxWidth - pUpdateWin->XStart;
        }
        if((CBIOS_U32)(pUpdateWin->YStart + pUpdateWin->WinHeight) > MaxHeight)
        {
            pUpdateWin->WinHeight = MaxHeight - pUpdateWin->YStart;
        }

        //check UpdateWindow according to Panel Parital Update Align requirement
        if(pUpdateWin->XStart % pPanelUpdateParams->PanelClumnAlign)
        {
            pUpdateWin->WinWidth += (pUpdateWin->XStart % pPanelUpdateParams->PanelClumnAlign);
            pUpdateWin->XStart -= (pUpdateWin->XStart % pPanelUpdateParams->PanelClumnAlign);
        }
        if(pUpdateWin->WinWidth % pPanelUpdateParams->PanelWidthAlign)
        {
            pUpdateWin->WinWidth += (pPanelUpdateParams->PanelWidthAlign - (pUpdateWin->WinWidth % pPanelUpdateParams->PanelWidthAlign));
        }
        if(pUpdateWin->YStart % pPanelUpdateParams->PanelRowAlign)
        {
            pUpdateWin->WinHeight += (pUpdateWin->YStart % pPanelUpdateParams->PanelRowAlign);
            pUpdateWin->YStart -= (pUpdateWin->YStart % pPanelUpdateParams->PanelRowAlign);
        }
        if(pUpdateWin->WinHeight % pPanelUpdateParams->PanelHeightAlign)
        {
            pUpdateWin->WinHeight += (pPanelUpdateParams->PanelHeightAlign - (pUpdateWin->WinHeight % pPanelUpdateParams->PanelHeightAlign));
        }
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_AdjustPanelUpdateWindow Invalid parameter! \n"));
        Status = CBIOS_ER_INVALID_PARAMETER;
    }

    return Status;
}

static CBIOS_STATUS cbDSI_GetHostUpdatePara(PCBIOS_VOID pvcbe, PCBIOS_DSI_HOSTUPDATE_PARA pHostUpdateParams,  PCBIOS_DSI_UPDATE_PARA pUpdateParams)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS Status = CBIOS_OK;
    PCBIOS_DEVICE_COMMON pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, CBIOS_TYPE_DSI);
    PCBIOS_DSI_PANEL_DESC pPanelDesc = &pDevCommon->DeviceParas.DSIDevice.DSIPanelDesc;
    PCBIOS_TIMING_ATTRIB pTargetTiming = &(pDevCommon->DeviceParas.DSIDevice.TargetTiming);
    PCBIOS_DSI_WINDOW  pUpdateWin = CBIOS_NULL;

    pHostUpdateParams->Size = sizeof(CBIOS_DSI_HOSTUPDATE_PARA);
    pHostUpdateParams->isEnablePartialUpdate = pUpdateParams->DSIUpdateConfig.bEnablePartialUpdate;
    pHostUpdateParams->isEnterHS = pUpdateParams->DSIUpdateConfig.bEnterHS;

    cbDSI_GetPanelUpdatePara(pcbe, &(pHostUpdateParams->PanelUpdatePara));

    if (pPanelDesc->DSIConfig.isDualChannel)
    {
        //only in left part
        if((pUpdateParams->UpdateWindow.XStart + pUpdateParams->UpdateWindow.WinWidth) < (pTargetTiming->XRes / 2))
        {
            pUpdateWin = &(pHostUpdateParams->HostUpdateWindow[DSI_INDEX0]);
            cb_memcpy(pUpdateWin, &(pUpdateParams->UpdateWindow), sizeof(CBIOS_DSI_WINDOW));
            cbDSI_AdjustPanelUpdateWindow(pcbe, &(pHostUpdateParams->PanelUpdatePara), pUpdateWin);
        }
        else if (pUpdateParams->UpdateWindow.XStart >=  (pTargetTiming->XRes / 2))   //only in right part
        {
            pUpdateWin = &(pHostUpdateParams->HostUpdateWindow[DSI_INDEX1]);
            pUpdateWin->XStart = pUpdateParams->UpdateWindow.XStart - pTargetTiming->XRes / 2 ;
            pUpdateWin->YStart = pUpdateParams->UpdateWindow.YStart;
            pUpdateWin->WinWidth = pUpdateParams->UpdateWindow.WinWidth;
            pUpdateWin->WinHeight = pUpdateParams->UpdateWindow.WinHeight;
            cbDSI_AdjustPanelUpdateWindow(pcbe, &(pHostUpdateParams->PanelUpdatePara), pUpdateWin);
        }
        else //both in left part and right part
        {
            pUpdateWin = &(pHostUpdateParams->HostUpdateWindow[DSI_INDEX0]);
            pUpdateWin->XStart = pUpdateParams->UpdateWindow.XStart;
            pUpdateWin->YStart = pUpdateParams->UpdateWindow.YStart;
            pUpdateWin->WinWidth = (pTargetTiming->XRes / 2 - 1) - pUpdateParams->UpdateWindow.XStart  + 1;
            pUpdateWin->WinHeight = pUpdateParams->UpdateWindow.WinHeight;
            cbDSI_AdjustPanelUpdateWindow(pcbe, &(pHostUpdateParams->PanelUpdatePara), pUpdateWin);

            pUpdateWin = &(pHostUpdateParams->HostUpdateWindow[DSI_INDEX1]);
            pUpdateWin->XStart = 0; //pTargetTiming->XRes / 2 ;
            pUpdateWin->YStart = pUpdateParams->UpdateWindow.YStart;
            pUpdateWin->WinWidth = (pUpdateParams->UpdateWindow.WinWidth) -  (pTargetTiming->XRes / 2)  +  pUpdateParams->UpdateWindow.XStart;
            pUpdateWin->WinHeight = pUpdateParams->UpdateWindow.WinHeight;
            cbDSI_AdjustPanelUpdateWindow(pcbe, &(pHostUpdateParams->PanelUpdatePara), pUpdateWin);
        }

    }
    else
    {
        pUpdateWin = &(pHostUpdateParams->HostUpdateWindow[DSI_INDEX0]);
        cb_memcpy(pUpdateWin, &(pUpdateParams->UpdateWindow), sizeof(CBIOS_DSI_WINDOW));

        cbDSI_AdjustPanelUpdateWindow(pcbe, &(pHostUpdateParams->PanelUpdatePara), pUpdateWin);
    }

    return Status;
}

static CBIOS_VOID cbDSI_WaitForFlip(PCBIOS_EXTENSION_COMMON pcbe)
{
    CBIOS_U8 uCR3A_before = 0, uCR3E_before = 0;
    CBIOS_U8 uCR3A_after = 0, uCR3E_after = 0;
    CBIOS_U32  TryTimes = 0;

    while(TryTimes < 20)
    {
        uCR3A_before = cbMMIOReadReg(pcbe, CR_3A);
        uCR3E_before=cbMMIOReadReg(pcbe, CR_3E);

        cbDelayMilliSeconds(1);

        uCR3A_after = cbMMIOReadReg(pcbe, CR_3A);
        uCR3E_after=cbMMIOReadReg(pcbe, CR_3E);
        if((uCR3A_after != uCR3A_before) || (uCR3E_after != uCR3E_before))
        {
            TryTimes++;
        }
        else
        {
            break;
        }
    }

    return;
}

static CBIOS_STATUS cbDSI_GetDMAUpdatePara(PCBIOS_VOID pvcbe, PCBIOS_DSI_DMAUPDATE_PARA pDMAUpdateParams,  PCBIOS_DSI_UPDATE_PARA pUpdateParams)
{
    CBIOS_STATUS Status = CBIOS_OK;
    if(pDMAUpdateParams && pUpdateParams)
    {
        pDMAUpdateParams->Size = sizeof(CBIOS_DSI_DMAUPDATE_PARA);
        pDMAUpdateParams->DMABaseAddr = pUpdateParams->DSIUpdateConfig.DMABaseAddr;
        pDMAUpdateParams->DMAStride = pUpdateParams->DSIUpdateConfig.DMAStride;
        pDMAUpdateParams->isDMAAligned = pUpdateParams->DSIUpdateConfig.bDMAAligned;
        cb_memcpy(&(pDMAUpdateParams->DMAUpdateWindow),  &(pUpdateParams->UpdateWindow), sizeof(CBIOS_DSI_WINDOW));
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "cbDSI_GetDMAUpdatePara Invalid parameter! \n"));
        Status = CBIOS_ER_INVALID_PARAMETER;
    }
    return Status;
}

CBIOS_STATUS cbDSI_DisplayUpdate(PCBIOS_VOID pvcbe, PCBIOS_DSI_UPDATE_PARA pUpdateParams)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS Status = CBIOS_OK;
    PCBIOS_DEVICE_COMMON pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, CBIOS_TYPE_DSI);
    PCBIOS_DSI_PANEL_DESC pPanelDesc = &pDevCommon->DeviceParas.DSIDevice.DSIPanelDesc;
    CBIOS_DSI_INDEX DSIIndex = DSI_INDEX0, MaxDSIIndex = DSI_INDEX0;
    CBIOS_BOOL bDMAUpdate = pUpdateParams->DSIUpdateConfig.bDMAUpdate;
    CBIOS_DSI_HOSTUPDATE_PARA DSIHostUpdatePara = {0};
    CBIOS_DSI_DMAUPDATE_PARA   DSIDMAUpdatePara = {0};
    cbTraceEnter(DSI);

    if(bDMAUpdate)
    {
        Status = cbDSI_GetDMAUpdatePara(pcbe,  &DSIDMAUpdatePara, pUpdateParams);
    }
    else
    {
        Status = cbDSI_GetHostUpdatePara(pcbe, &DSIHostUpdatePara, pUpdateParams);
    }

    if (pPanelDesc->DSIConfig.isDualChannel)
    {
        MaxDSIIndex = DSI_INDEX1;
    }

    cbDSI_WaitForFlip(pcbe);

    for (DSIIndex = DSI_INDEX0; DSIIndex <= MaxDSIIndex; DSIIndex++)
    {
        if(bDMAUpdate)
        {
            Status = cbDSI_DisplayUpdateByDMA(pcbe, &DSIDMAUpdatePara, DSIIndex);
        }
        else
        {
            Status = cbDSI_DisplayUpdateByHost(pcbe, &DSIHostUpdatePara, DSIIndex);
        }

    }

    cbTraceExit(DSI);
    return Status;
}

CBIOS_STATUS cbDSI_SendCmdList(PCBIOS_VOID pvcbe, PCBIOS_DSI_CMD_DESC pCmdList, CBIOS_U32 CmdCount)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_DSI_WRITE_PARA_INTERNAL DSIWriteParams;
    CBIOS_STATUS Status = CBIOS_OK;
    CBIOS_U32 i = 0;
    cbTraceEnter(DSI);

    for (i = 0; i < CmdCount; i++)
    {
        cb_memset(&DSIWriteParams, 0, sizeof(CBIOS_DSI_WRITE_PARA_INTERNAL));
        DSIWriteParams.DSIIndex = pCmdList[i].DSIIndex;
        DSIWriteParams.VirtualCh = (CBIOS_U8)pCmdList[i].VirtualCh;
        DSIWriteParams.PacketType = pCmdList[i].PacketType;
        DSIWriteParams.ContentType = pCmdList[i].ContentType;
        DSIWriteParams.DataLen = (CBIOS_U16)pCmdList[i].DataLen;
        DSIWriteParams.DSIFlags = pCmdList[i].DSIFlags;
        DSIWriteParams.pDataBuf = pCmdList[i].pDataBuf;

        Status = cbDSI_SendWriteCmd(pcbe, &DSIWriteParams);

        if (pCmdList[i].WaitTime != 0)
        {
            cbDelayMilliSeconds(pCmdList[i].WaitTime);
        }
    }

    cbTraceExit(DSI);
    return Status;
}

#define POLL_PANEL_STATE_NUM  20
#define CLK_CTRL_EXCH_NUM     10
CBIOS_VOID cbDSI_R63319Patch(PCBIOS_VOID pvcbe)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DEVICE_COMMON pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, CBIOS_TYPE_DSI);
    PCBIOS_DSI_PANEL_DESC pPanelDesc = &pDevCommon->DeviceParas.DSIDevice.DSIPanelDesc;
    REG_MM3444_MM3574 RegMM3444_MM3574Value;
    REG_MM3404_MM3534 RegMM3404_MM3534Value;
    REG_MM3438_MM3568 RegMM3438_MM3568Value;
    CBIOS_U32   i = 0, j = 0;
    CBIOS_BOOL  bResult = CBIOS_FALSE;

    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "enter cbDSIPanel_R63319Patch!\n"));
    cbDSIPanel_DisplayOnOff(pcbe, CBIOS_FALSE, pPanelDesc);

    for(i = 0; i < CLK_CTRL_EXCH_NUM; i++)
    {

        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "i: %d \n", i));
        bResult = CBIOS_FALSE;

        cbWaitForActive(pcbe);
        cbDSI_SetClkLaneMode(pcbe, CBIOS_DSI_CLK_LANE_SOFTWARE_CTL, DSI_INDEX1);
        cbDSI_SetClkLaneMode(pcbe, CBIOS_DSI_CLK_LANE_SOFTWARE_CTL, DSI_INDEX0);


        cbDSI_ClearReadFIFO(pcbe, DSI_INDEX1);

        for(j = 0; j < POLL_PANEL_STATE_NUM; j++ )
        {
            cbMMIOWriteReg32(pcbe, 0x3534, 0x16000005, 0x0);
            cbDelayMilliSeconds(50);
            RegMM3438_MM3568Value.Value = cb_ReadU32(pcbe->pAdapterContext,0x3568);
            RegMM3444_MM3574Value.Value = cb_ReadU32(pcbe->pAdapterContext,0x3574);
            RegMM3404_MM3534Value.Value = cb_ReadU32(pcbe->pAdapterContext,0x3534);
            cbMMIOWriteReg32(pcbe, 0x3568, RegMM3438_MM3568Value.Value, 0x0);

            cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "RegMM3438_MM3568Value: 0x%08x\n", RegMM3438_MM3568Value.Value));
            cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "RegMM3444_MM3574Value: 0x%08x\n", RegMM3444_MM3574Value.Value));
            cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "RegMM3404_MM3534Value: 0x%08x\n", RegMM3404_MM3534Value.Value));

            if(RegMM3438_MM3568Value.Receive_Other_Trigger_or_Unrecognized_Stauts)
            {
                bResult = CBIOS_FALSE;
                break;
            }
            else
            {
                cbDSI_ClearReadFIFO(pcbe, DSI_INDEX1);
            }
        }

        if(j >= POLL_PANEL_STATE_NUM)
        {
            bResult = CBIOS_TRUE;
        }

        if(bResult)
        {
            break;
        }
        else
        {
            cbDSI_SetClkLaneMode(pcbe, CBIOS_DSI_CLK_LANE_HARDWARE_CTL, DSI_INDEX1);
            cbDSI_SetClkLaneMode(pcbe, CBIOS_DSI_CLK_LANE_HARDWARE_CTL, DSI_INDEX0);
        }

        cbDelayMilliSeconds(10);

    }

    cbDSIPanel_DisplayOnOff(pcbe, CBIOS_TRUE, pPanelDesc);
    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "Exit cbDSIPanel_R63319Patch, bResult: %d\n", bResult));
}

static CBIOS_BOOL cbDSIPort_PanelDetect(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon)
{
    PCBIOS_U8 pEdid = pDevCommon->EdidData;
    CBIOS_FAKE_EDID_PARAMS FakeEdidParams = {0};
    CBIOS_BOOL bConnected = CBIOS_FALSE;
    PCBIOS_DSI_PARAMS pDSIParams = CBIOS_NULL;
    PCBIOS_DSI_PANEL_TABLE pDSIPanelTbl = CBIOS_NULL;

    if(!(pcbe->DeviceMgr.SupportDevices & pDevCommon->DeviceType))
    {
        //Not support, then return not connected
        return CBIOS_FALSE;
    }

    //check EDID
    if (!cbEDIDModule_IsEDIDHeaderValid(pEdid, CBIOS_EDIDDATABYTE))
    {
        //Integrated panel, use fake EDID
        cbDebugPrint((MAKE_LEVEL(DSI, INFO),(" Use FakeEDID\n")));

        cb_memset(pEdid, 0, CBIOS_EDIDDATABYTE);

        pDSIParams = &(pDevCommon->DeviceParas.DSIDevice);
        pDSIPanelTbl = &(pDSIParams->DSIPanelDesc.DSIPanelTbl);

        FakeEdidParams.bProvideDtlTimingEDID = CBIOS_FALSE;
        FakeEdidParams.DtlTiming.PixelClock = pDSIPanelTbl->PanelTiming.DCLK;
        FakeEdidParams.DtlTiming.HActive = (CBIOS_U16)pDSIPanelTbl->PanelTiming.HorDisEnd;
        FakeEdidParams.DtlTiming.VActive = (CBIOS_U16)pDSIPanelTbl->PanelTiming.VerDisEnd;
        FakeEdidParams.DtlTiming.HBlank = pDSIPanelTbl->PanelTiming.HorBEnd - pDSIPanelTbl->PanelTiming.HorBStart;
        FakeEdidParams.DtlTiming.VBlank = pDSIPanelTbl->PanelTiming.VerBEnd - pDSIPanelTbl->PanelTiming.VerBStart;
        FakeEdidParams.DtlTiming.HSyncOffset = pDSIPanelTbl->PanelTiming.HorSyncStart - pDSIPanelTbl->PanelTiming.HorBStart;
        FakeEdidParams.DtlTiming.HSyncPulseWidth = pDSIPanelTbl->PanelTiming.HorSyncEnd - pDSIPanelTbl->PanelTiming.HorSyncStart;
        FakeEdidParams.DtlTiming.VSyncOffset = pDSIPanelTbl->PanelTiming.VerSyncStart - pDSIPanelTbl->PanelTiming.VerBStart;
        FakeEdidParams.DtlTiming.VSyncPulseWidth = pDSIPanelTbl->PanelTiming.VerSyncEnd - pDSIPanelTbl->PanelTiming.VerSyncStart;
        if (pDSIPanelTbl->PanelTiming.HVPolarity & DSI_HPOSITIVE)
        {
            FakeEdidParams.DtlTiming.HSync = HorPOSITIVE;
        }
        else
        {
            FakeEdidParams.DtlTiming.HSync = HorNEGATIVE;
        }
        if (pDSIPanelTbl->PanelTiming.HVPolarity & DSI_VPOSITIVE)
        {
            FakeEdidParams.DtlTiming.VSync = VerPOSITIVE;
        }
        else
        {
            FakeEdidParams.DtlTiming.VSync = VerNEGATIVE;
        }

        cbEDIDModule_FakePanelEDID(&FakeEdidParams, pEdid, 128);
        cbEDIDModule_ParseEDID(pEdid, &(pDevCommon->EdidStruct), CBIOS_EDIDDATABYTE);

        pDevCommon->isFakeEdid = CBIOS_TRUE;
        pDevCommon->CurrentMonitorType = CBIOS_MONITOR_TYPE_PANEL;
    }

    bConnected = CBIOS_TRUE;
    return bConnected;
}

CBIOS_VOID cbDSIPort_HwInit(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon)
{
    PCBIOS_DSI_PANEL_DESC pPanelDesc = CBIOS_NULL;

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & CBIOS_TYPE_DSI)))
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return;
    }

    pPanelDesc = &pDevCommon->DeviceParas.DSIDevice.DSIPanelDesc;

    if (!pDevCommon->DeviceParas.DSIDevice.bInitialized)
    {
        cb_memcpy(pPanelDesc, cbDSIPanel_GetPanelDescriptor(pcbe), sizeof(CBIOS_DSI_PANEL_DESC));
        cbDSIPanel_Init(pcbe, pPanelDesc);
        pDevCommon->DeviceParas.DSIDevice.bInitialized = CBIOS_TRUE;
    }

    if(cbDSIPort_PanelDetect(pcbe, pDevCommon))
    {
        cbDSI_Init(pcbe, IGA1);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DSI, INFO), "%s: can't detect DSI panel, remove DSI from support devices!\n", FUNCTION_NAME));
        pcbe->DeviceMgr.SupportDevices &= ~(CBIOS_TYPE_DSI);
    }
}

CBIOS_VOID cbDSIPort_QueryMonitorAttribute(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBiosMonitorAttribute pMonitorAttribute)
{
    cbTraceEnter(DSI);

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & CBIOS_TYPE_DSI)))
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return;
    }

    pMonitorAttribute->bSupportBLCtrl = pDevCommon->DeviceParas.DSIDevice.DSIPanelDesc.DSIConfig.isBLCtrlSupport;
    pMonitorAttribute->MaxBLLevel = pDevCommon->DeviceParas.DSIDevice.DSIPanelDesc.DSIConfig.BacklightMax;
    pMonitorAttribute->MinBLLevel = pDevCommon->DeviceParas.DSIDevice.DSIPanelDesc.DSIConfig.BacklightMin;

    cbTraceExit(DSI);
}


static CBIOS_VOID cbDSIPort_HW_SetMode(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DISP_MODE_PARAMS pModeParams)
{
    PCBIOS_DSI_PARAMS pDSIParams = CBIOS_NULL;
    PCBIOS_DSI_CONFIG pDSIConfig = CBIOS_NULL;
    PCBIOS_TIMING_ATTRIB pTiming = &(pModeParams->TargetTiming);

    pDSIParams = &(pDevCommon->DeviceParas.DSIDevice);
    pDSIConfig = &(pDSIParams->DSIPanelDesc.DSIConfig);

    if(pDSIConfig->isDualChannel)
    {
        cbDSI_SetDSITimingHW(pcbe, pDSIParams, pTiming, IGA1, DSI_INDEX0);
        cbDSI_SetDSITimingHW(pcbe, pDSIParams, pTiming, IGA1, DSI_INDEX1);

        //enable dual-dsi timing
    }
    else
    {
        cbDSI_SetDSITimingHW(pcbe, pDSIParams, pTiming, IGA1, DSI_INDEX0);
        //disable dual-dsi timing
    }

}

static CBIOS_U32 cbDSIPort_CalPLLCLK(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_U32 PixelClock)
{
    PCBIOS_DSI_PANEL_DESC pPanelDesc = &pDevCommon->DeviceParas.DSIDevice.DSIPanelDesc;
    CBIOS_U32  PLLClk = 0;
    if(pPanelDesc->DSIPanelTbl.OutBpp == 24)
    {
        switch(pPanelDesc->DSIPanelTbl.LaneNum )
        {
        case 3:
            PLLClk = PixelClock;
            break;
        case 1:
        case 2:
        case 4:
            PLLClk = PixelClock * 3;
            break;
        default:
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbDSIPort_CalPLLCLK: Wrong LaneNum \n"));
            break;
        }
    }
    else if (pPanelDesc->DSIPanelTbl.OutBpp == 16)
    {
        switch(pPanelDesc->DSIPanelTbl.LaneNum )
        {
        case 1:
        case 3:
            PLLClk = PixelClock * 2;
            break;
        case 2:
        case 4:
            PLLClk = PixelClock ;
            break;
        default:
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbDSIPort_CalPLLCLK: Wrong LaneNum \n"));
            break;
        }
    }
    else if (pPanelDesc->DSIPanelTbl.OutBpp == 18)
    {
        switch(pPanelDesc->DSIPanelTbl.LaneNum )
        {
        case 3:
            PLLClk = PixelClock * 3;
            break;
        case 1:
        case 2:
        case 4:
            PLLClk = PixelClock * 9;
            break;
        default:
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbDSIPort_CalPLLCLK: Wrong LaneNum \n"));
            break;
        }
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbDSIPort_CalPLLCLK: Wrong OutBpp \n"));
    }

    return PLLClk;
}

static CBIOS_VOID cbDSIPort_UpdateModeInfo(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DISP_MODE_PARAMS pModeParams)
{
    PCBIOS_DSI_PARAMS pDSIParams = CBIOS_NULL;
    PCBIOS_TIMING_ATTRIB pTiming = &(pModeParams->TargetTiming);

    cbTraceEnter(DSI);

    pDSIParams = &(pDevCommon->DeviceParas.DSIDevice);
    pTiming->PLLClock = cbDSIPort_CalPLLCLK(pcbe, pDevCommon, pTiming->PixelClock);
    // save target timing
    cb_memcpy(&(pDSIParams->TargetTiming), pTiming, sizeof(CBIOS_TIMING_ATTRIB));

    cbTraceExit(DSI);
}

static CBIOS_VOID cbDSIPort_SetMode(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DISP_MODE_PARAMS pModeParams)
{
    cbTraceEnter(DSI);

    cbDSIPort_HW_SetMode(pcbe, pDevCommon, pModeParams);

    cbTraceExit(DSI);
}

CBIOS_BOOL cbDSIPort_DeviceDetect(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_BOOL bHardcodeDetected, CBIOS_U32 FullDetect)
{
    CBIOS_BOOL  bConnected = CBIOS_FALSE;

    cbTraceEnter(DSI);

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & CBIOS_TYPE_DSI)))
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return bConnected;
    }

    bConnected = cbDSIPort_PanelDetect(pcbe, pDevCommon);

    cbTraceExit(DSI);

    return bConnected;
}

CBIOS_VOID cbDSIPort_OnOff(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_BOOL bOn)
{
    PCBIOS_DSI_PANEL_DESC pPanelDesc = CBIOS_NULL;
    cbTraceEnter(DSI);

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & CBIOS_TYPE_DSI)))
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return;
    }

    pPanelDesc = &pDevCommon->DeviceParas.DSIDevice.DSIPanelDesc;

    if (bOn)
    {
        cbDSI_ControllerInit(pcbe, IGA1, DSI_INDEX0);
        cbDSI_ControllerOnOff(pcbe, CBIOS_TRUE, DSI_INDEX0);

        if(pPanelDesc->DSIConfig.isDualChannel)
        {
            cbDSI_ControllerInit(pcbe, IGA1, DSI_INDEX1);
            cbDSI_ControllerOnOff(pcbe, CBIOS_TRUE, DSI_INDEX1);
        }

        cbDSIPanel_OnOff(pcbe, CBIOS_TRUE, pPanelDesc);

        pDevCommon->PowerState = CBIOS_PM_ON;
    }
    else // turn off
    {
        // panel turn off
        cbDSIPanel_OnOff(pcbe, CBIOS_FALSE, pPanelDesc);

        cbDSI_ControllerOnOff(pcbe, CBIOS_FALSE, DSI_INDEX0);
        if(pPanelDesc->DSIConfig.isDualChannel)
        {
            cbDSI_ControllerOnOff(pcbe, CBIOS_FALSE, DSI_INDEX1);
        }
        pDevCommon->PowerState = CBIOS_PM_OFF;
    }

    cbTraceExit(DSI);
}

PCBIOS_DEVICE_COMMON cbDSIPort_Init(PCBIOS_VOID pvcbe, PVCP_INFO pVCP, CBIOS_ACTIVE_TYPE DeviceType)
{
    PCBIOS_EXTENSION_COMMON pcbe          = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DEVICE_COMMON    pDeviceCommon = CBIOS_NULL;

    if(DeviceType & ~CBIOS_TYPE_DSI)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"%s: unsupported device type!!!\n", FUNCTION_NAME));
        return CBIOS_NULL;
    }

    pDeviceCommon = cb_AllocateNonpagedPool(sizeof(CBIOS_DEVICE_COMMON));
    if(pDeviceCommon == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"%s: pDeviceCommon allocate error!!!\n", FUNCTION_NAME));
        return CBIOS_NULL;
    }

    pDeviceCommon->DeviceType = DeviceType;
    pDeviceCommon->SupportMonitorType = cbGetSupportMonitorType(pcbe, DeviceType);
    pDeviceCommon->I2CBus = I2CBUS_INVALID;
    pDeviceCommon->HPDPin = 0;
    pDeviceCommon->CurrentMonitorType = CBIOS_MONITOR_TYPE_NONE;
    pDeviceCommon->PowerState = CBIOS_PM_INVALID;

    cbInitialModuleList(&pDeviceCommon->DispSource.ModuleList);

    pDeviceCommon->pfncbDeviceHwInit = (PFN_cbDeviceHwInit)cbDSIPort_HwInit;
    pDeviceCommon->pfncbQueryMonitorAttribute = (PFN_cbQueryMonitorAttribute)cbDSIPort_QueryMonitorAttribute;
    pDeviceCommon->pfncbDeviceDetect = (PFN_cbDeviceDetect)cbDSIPort_DeviceDetect;
    pDeviceCommon->pfncbDeviceOnOff = (PFN_cbDeviceOnOff)cbDSIPort_OnOff;
    pDeviceCommon->pfncbUpdateDeviceModeInfo = (PFN_cbUpdateDeviceModeInfo)cbDSIPort_UpdateModeInfo;
    pDeviceCommon->pfncbDeviceSetMode = (PFN_cbDeviceSetMode)cbDSIPort_SetMode;

    return pDeviceCommon;
}

CBIOS_VOID cbDSIPort_DeInit(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon)
{
    PCBIOS_DSI_PARAMS    pDSIParams = CBIOS_NULL;

    if ((pDevCommon == CBIOS_NULL) || (!(pDevCommon->DeviceType & CBIOS_TYPE_DSI)))
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "%s: the 2nd param is invalid!\n", FUNCTION_NAME));
        return;
    }

    pDSIParams = &(pDevCommon->DeviceParas.DSIDevice);

    if (pDSIParams->pPanelDataBuf != CBIOS_NULL)
    {
        cb_FreePool(pDSIParams->pPanelDataBuf);
        pDSIParams->pPanelDataBuf = CBIOS_NULL;
    }

    cb_FreePool(pDevCommon);
}
