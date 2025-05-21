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

#include "CBiosPHY_DP.h"
#include "CBiosDIU_DP.h"
#include "CBiosDIU_HDTV.h"
#include "CBiosChipShare.h"
#include "../CBiosHwShare.h"

DP_EPHY_MODE cbPHY_DP_GetEphyMode(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8348       DPEphyMiscRegValue;
    DP_EPHY_MODE  Mode;

    DPEphyMiscRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_EPHY_MISC[DPModuleIndex]);

    if(DPEphyMiscRegValue.Driver_Mode == 0)
    {
        Mode = DP_EPHY_TMDS_MODE;
    }
    else
    {
        Mode = DP_EPHY_DP_MODE;
    }

    return  Mode;
}

CBIOS_VOID cbPHY_DP_SelectEphyMode(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, DP_EPHY_MODE DPEphyMode)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_SR3B_Pair    RegSR3BValue;
    REG_SR3B_Pair    RegSR3BMask;
    REG_MM8348       DPEphyMiscRegValue, DPEphyMiscRegMask;

    switch (DPEphyMode)
    {
    case DP_EPHY_TMDS_MODE:
        DPEphyMiscRegValue.Value = 0;
        DPEphyMiscRegValue.Driver_Mode = 0;
        DPEphyMiscRegMask.Value = 0xFFFFFFFF;
        DPEphyMiscRegMask.Driver_Mode = 0;
        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MISC[DPModuleIndex], DPEphyMiscRegValue.Value, DPEphyMiscRegMask.Value);

        RegSR3BValue.Value = 0;
        RegSR3BValue.Sel_DP_TMDS = 1;
        RegSR3BMask.Value = 0xFF;
        RegSR3BMask.Sel_DP_TMDS = 0;
        cbBiosMMIOWriteReg(pcbe, SR_3B, RegSR3BValue.Value, RegSR3BMask.Value, DPModuleIndex);
        break;
    case DP_EPHY_DP_MODE:
        DPEphyMiscRegValue.Value = 0;
        DPEphyMiscRegValue.Driver_Mode = 1;
        DPEphyMiscRegMask.Value = 0xFFFFFFFF;
        DPEphyMiscRegMask.Driver_Mode = 0;
        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MISC[DPModuleIndex], DPEphyMiscRegValue.Value, DPEphyMiscRegMask.Value);

        RegSR3BValue.Value = 0;
        RegSR3BValue.Sel_DP_TMDS = 0;
        RegSR3BMask.Value = 0xFF;
        RegSR3BMask.Sel_DP_TMDS = 0;
        cbBiosMMIOWriteReg(pcbe, SR_3B, RegSR3BValue.Value, RegSR3BMask.Value, DPModuleIndex);
        break;
    default:
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid ephy mode!\n", FUNCTION_NAME));
        break;

    }
}

CBIOS_VOID cbPHY_DP_DPModeOnOff(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_U32 LinkSpeed, CBIOS_BOOL status)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM82CC    DPEphyCtrlRegValue, DPEphyCtrlRegMask;
    REG_MM8340    DPEphyMpllRegValue, DPEphyMpllRegMask;
    REG_MM8344    DPEphyTxRegValue, DPEphyTxRegMask;
    REG_MM8348    DPEphyMiscRegValue, DPEphyMiscRegMask;
    CBIOS_BOOL    bACE = ((pcbe->ChipID == CHIPID_ARISE2030) || (pcbe->ChipID == CHIPID_ARISE2020)) ? CBIOS_TRUE : CBIOS_FALSE;

    cbTraceEnter(DP);

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return;
    }

    if (status)//DP on
    {
        DPEphyTxRegValue.Value = 0;
        DPEphyTxRegValue.TX_Resistance_Value = 8;
        DPEphyTxRegValue.EPHY1_SR_MAN_L0 = 0;
        DPEphyTxRegValue.EPHY1_SR_MAN_L1 = 0;
        DPEphyTxRegValue.EPHY1_SR_MAN_L2 = 0;
        DPEphyTxRegValue.EPHY1_SR_MAN_L3 = 0;
        DPEphyTxRegValue.DIU_EPHY1_AUX_DIAJ = 2;  // {mm82cc[24], mm8344[22:20]} = 4'b1010
        DPEphyTxRegMask.Value = 0xFFFFFFFF;
        DPEphyTxRegMask.TX_Resistance_Value = 0;
        DPEphyTxRegMask.EPHY1_SR_MAN_L0 = 0;
        DPEphyTxRegMask.EPHY1_SR_MAN_L1 = 0;
        DPEphyTxRegMask.EPHY1_SR_MAN_L2 = 0;
        DPEphyTxRegMask.EPHY1_SR_MAN_L3 = 0;
        DPEphyTxRegMask.DIU_EPHY1_AUX_DIAJ = 0;

        DPEphyCtrlRegValue.Value = 0;
        DPEphyCtrlRegValue.DIU_EPHY1_AUX_DIAJ = 1;
        DPEphyCtrlRegValue.TX0T = 0;
        DPEphyCtrlRegValue.TX1T = 0;
        DPEphyCtrlRegValue.TX2T = 0;
        DPEphyCtrlRegValue.TX3T = 0;
        DPEphyCtrlRegMask.Value = 0xFFFFFFFF;
        DPEphyCtrlRegMask.DIU_EPHY1_AUX_DIAJ = 0;
        DPEphyCtrlRegMask.TX0T = 0;
        DPEphyCtrlRegMask.TX1T = 0;
        DPEphyCtrlRegMask.TX2T = 0;
        DPEphyCtrlRegMask.TX3T = 0;
        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);
        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_CTRL[DPModuleIndex], DPEphyCtrlRegValue.Value, DPEphyCtrlRegMask.Value);

        DPEphyMiscRegValue.Value = 0;
        DPEphyMiscRegValue.Driver_Mode = 1;
        DPEphyMiscRegValue.M1V = 0;
        DPEphyMiscRegValue.T1V = 1;
        DPEphyMiscRegValue.TT = 0;
        DPEphyMiscRegMask.Value = 0xFFFFFFFF;
        DPEphyMiscRegMask.Driver_Mode = 0;
        DPEphyMiscRegMask.M1V = 0;
        DPEphyMiscRegMask.T1V = 0;
        DPEphyMiscRegMask.TT = 0;
        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MISC[DPModuleIndex], DPEphyMiscRegValue.Value, DPEphyMiscRegMask.Value);

        if (bACE && LinkSpeed == CBIOS_DP_LINK_SPEED_5400Mbps)
        {
            DPEphyMpllRegValue.Value = 0;
            DPEphyMpllRegValue.TPLL_N_Div = 0;
            DPEphyMpllRegMask.Value = 0xFFFFFFFF;
            DPEphyMpllRegMask.TPLL_N_Div = 0;
            cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);
        }
    }
    else//DP off
    {
        DPEphyMpllRegValue.Value = 0;
        DPEphyMpllRegValue.Bandgap_Power_Down = 0;
        DPEphyMpllRegValue.MPLL_Reg_Power_Down = 0;
        DPEphyMpllRegValue.MPLL_Power_Down = 0;
        DPEphyMpllRegValue.MPLL_PTAT_Current = 1;
        DPEphyMpllRegValue.MPLL_CP_Current = 4;
        DPEphyMpllRegValue.MPLL_N = 0xBE;
        DPEphyMpllRegValue.MPLL_R = 0;
        DPEphyMpllRegValue.MPLL_P = 2;
        DPEphyMpllRegValue.SSC_Enable = 0;
        DPEphyMpllRegValue.SSC_Freq_Spread = 0;
        DPEphyMpllRegValue.Dither = 0;
        DPEphyMpllRegValue.Signal_Profile = 1;
        DPEphyMpllRegValue.Spread_Magnitude = 0;
        DPEphyMpllRegValue.TPLL_Reg_Power_Down = 0;
        DPEphyMpllRegValue.TPLL_Power_Down = 0;
        DPEphyMpllRegValue.TPLL_N_Div = 1;

        DPEphyMpllRegMask.Value = 0;
        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);

        DPEphyMiscRegValue.Value = 0;
        DPEphyMiscRegValue.Driver_Mode = 0;
        DPEphyMiscRegValue.Reserved = 1;
        DPEphyMiscRegValue.RTNBIST = 3;
        DPEphyMiscRegValue.CKHLD = 0;
        DPEphyMiscRegValue.M1V = 0;
        DPEphyMiscRegValue.MT = 1;
        DPEphyMiscRegValue.T1V = 0;
        DPEphyMiscRegValue.TT = 0;
        DPEphyMiscRegValue.EPHY1_TPLL_CP = 0;
        DPEphyMiscRegValue.AUC_Ch_Op_Mode = 0;
        DPEphyMiscRegValue.HPD_Power_Down = 1;
        DPEphyMiscRegValue.TPLL_Reset_Signal = 0;
        DPEphyMiscRegValue.MPLL_SSC_Output = 0;
        DPEphyMiscRegValue.TPLL_Lock_Indicator = 0;
        DPEphyMiscRegValue.MPLL_Lock_Indicator = 0;
        DPEphyMiscRegValue.RTN_Results = 0;
        DPEphyMiscRegMask.Value = 0;
        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MISC[DPModuleIndex], DPEphyMiscRegValue.Value, DPEphyMiscRegMask.Value);

        DPEphyTxRegValue.Value = 0;
        DPEphyTxRegValue.Resistance_Tuning_PD = 0;
        DPEphyTxRegValue.Resistance_Tuning_Reset = 1;
        DPEphyTxRegValue.Resistance_Tuning_Enable = 0;
        DPEphyTxRegValue.TX_Resistance_Set_Enable = 0;
        DPEphyTxRegValue.TX_Resistance_Value = 8;
        DPEphyTxRegValue.TX_Reg_Power_Down_Lane0 = 0;
        DPEphyTxRegValue.TX_Reg_Power_Down_Lane1 = 0;
        DPEphyTxRegValue.TX_Reg_Power_Down_Lane2 = 0;
        DPEphyTxRegValue.TX_Reg_Power_Down_Lane3 = 0;
        DPEphyTxRegValue.Driver_Control_Lane0 = 0;
        DPEphyTxRegValue.Driver_Control_Lane1 = 0;
        DPEphyTxRegValue.Driver_Control_Lane2 = 0;
        DPEphyTxRegValue.Driver_Control_Lane3 = 0;
        DPEphyTxRegValue.EPHY1_SR_MAN_L0 = 0;
        DPEphyTxRegValue.EPHY1_SR_MAN_L1 = 0;
        DPEphyTxRegValue.EPHY1_SR_MAN_L2 = 0;
        DPEphyTxRegValue.EPHY1_SR_MAN_L3 = 0;
        DPEphyTxRegValue.DIU_EPHY1_AUX_DIAJ = 0;
        DPEphyTxRegValue.EPHY_MPLL_CP = 0;
        DPEphyTxRegValue.TX_Power_Control_Lane0 = 3;
        DPEphyTxRegValue.TX_Power_Control_Lane1 = 3;
        DPEphyTxRegValue.TX_Power_Control_Lane2 = 3;
        DPEphyTxRegValue.TX_Power_Control_Lane3 = 3;
        DPEphyTxRegMask.Value = 0;
        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);
    }

    cbTraceExit(DP);
}


CBIOS_VOID cbPHY_DP_DualModeOnOff(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex, CBIOS_U32 ClockFreq, CBIOS_BOOL bTurnOn)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8214    DPLinkRegValue, DPLinkRegMask;
    REG_MM82CC    DPEphyCtrlRegValue, DPEphyCtrlRegMask;
    REG_MM8340    DPEphyMpllRegValue, DPEphyMpllRegMask;
    REG_MM8344    DPEphyTxRegValue, DPEphyTxRegMask;
    REG_MM8348    DPEphyMiscRegValue, DPEphyMiscRegMask;
    REG_MM8368    DPSwingRegValue, DPSwingRegMask;
    REG_MM334E0   DPEphySetting1RegValue, DPEphySetting1RegMask;
    REG_MM836C    DPEphyStatusRegValue, DPEphyStatusRegMask;
    REG_MM334E4   DPEphySetting2RegValue, DPEphySetting2RegMask;
    CBIOS_BOOL    bACE = ((pcbe->ChipID == CHIPID_ARISE2030) || (pcbe->ChipID == CHIPID_ARISE2020)) ? CBIOS_TRUE : CBIOS_FALSE;

    cbTraceEnter(DP);

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return;
    }

    if (bTurnOn)//on
    {
        /* 1. set parameters before enable TPLL*/

        DPEphyTxRegValue.Value = 0;
        DPEphyTxRegValue.Resistance_Tuning_PD = 0;
        DPEphyTxRegValue.Resistance_Tuning_Reset = 1;
        DPEphyTxRegValue.Resistance_Tuning_Enable = 0;
        DPEphyTxRegValue.TX_Resistance_Set_Enable = 1;
        DPEphyTxRegValue.EPHY1_SR_MAN_L0 = 0;
        DPEphyTxRegValue.EPHY1_SR_MAN_L1 = 0;
        DPEphyTxRegValue.EPHY1_SR_MAN_L2 = 0;
        DPEphyTxRegValue.EPHY1_SR_MAN_L3 = 0;
        DPEphyTxRegValue.DIU_EPHY1_AUX_DIAJ = 0;
        if(ClockFreq > 3400000)
        {
            DPEphyTxRegValue.TX_Resistance_Value = 0xF;
        }
        else
        {
            DPEphyTxRegValue.TX_Resistance_Value = 0xD;
        }

        DPEphyTxRegMask.Value = 0xFFFFFFFF;
        DPEphyTxRegMask.Resistance_Tuning_PD = 0;
        DPEphyTxRegMask.Resistance_Tuning_Reset = 0;
        DPEphyTxRegMask.Resistance_Tuning_Enable = 0;
        DPEphyTxRegMask.TX_Resistance_Set_Enable = 0;
        DPEphyTxRegMask.TX_Resistance_Value = 0;
        DPEphyTxRegMask.EPHY1_SR_MAN_L0 = 0;
        DPEphyTxRegMask.EPHY1_SR_MAN_L1 = 0;
        DPEphyTxRegMask.EPHY1_SR_MAN_L2 = 0;
        DPEphyTxRegMask.EPHY1_SR_MAN_L3 = 0;
        DPEphyTxRegMask.DIU_EPHY1_AUX_DIAJ = 0;

        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);

        DPEphyMiscRegValue.Value = 0;
        DPEphyMiscRegValue.CKHLD = 0;
        DPEphyMiscRegValue.TT = 0;
        DPEphyMiscRegValue.TX_High_Impedance_Lane0 = 1;
        DPEphyMiscRegValue.TX_High_Impedance_Lane1 = 0;
        DPEphyMiscRegValue.TX_High_Impedance_Lane2 = 0;
        DPEphyMiscRegValue.TX_High_Impedance_Lane3 = 0;
        DPEphyMiscRegValue.AUC_Ch_Op_Mode = 0;
        DPEphyMiscRegValue.HPD_Power_Down = 1;

        if (ClockFreq >= 3400000)
        {
            DPEphyMiscRegValue.T1V = 1;
            DPEphyMiscRegValue.MT = 0;
            DPEphyMiscRegValue.EPHY1_TPLL_CP = 0x8;
        }
        else if (ClockFreq >= 1700000)
        {
            DPEphyMiscRegValue.T1V = 0;
            DPEphyMiscRegValue.MT = 1;
            DPEphyMiscRegValue.EPHY1_TPLL_CP = 2;
        }
        else
        {
            DPEphyMiscRegValue.T1V = 0;
            DPEphyMiscRegValue.MT = 1;
            DPEphyMiscRegValue.EPHY1_TPLL_CP = 8;
        }

        DPEphyMiscRegMask.Value = 0xFFFFFFFF;
        DPEphyMiscRegMask.CKHLD = 0;
        DPEphyMiscRegMask.T1V = 0;
        DPEphyMiscRegMask.TT = 0;
        DPEphyMiscRegMask.EPHY1_TPLL_CP = 0;
        DPEphyMiscRegMask.TX_High_Impedance_Lane0 = 0;
        DPEphyMiscRegMask.TX_High_Impedance_Lane1 = 0;
        DPEphyMiscRegMask.TX_High_Impedance_Lane2 = 0;
        DPEphyMiscRegMask.TX_High_Impedance_Lane3 = 0;
        DPEphyMiscRegMask.AUC_Ch_Op_Mode = 0;
        DPEphyMiscRegMask.HPD_Power_Down = 0;
        DPEphyMiscRegMask.MT = 0;

        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MISC[DPModuleIndex], DPEphyMiscRegValue.Value, DPEphyMiscRegMask.Value);

        if (ClockFreq >= 3400000)
        {
            DPEphyMpllRegValue.Value = 0;
            DPEphyMpllRegValue.TPLL_N_Div = 0;
            DPEphyMpllRegMask.Value = 0xFFFFFFFF;
            DPEphyMpllRegMask.TPLL_N_Div = 0;
            cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);
        }
        else if (ClockFreq >= 1700000)
        {
            DPEphyMpllRegValue.Value = 0;
            DPEphyMpllRegValue.TPLL_N_Div = 0;
            DPEphyMpllRegMask.Value = 0xFFFFFFFF;
            DPEphyMpllRegMask.TPLL_N_Div = 0;
            cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);
        }
        else if (ClockFreq >= 850000)
        {
            DPEphyMpllRegValue.Value = 0;
            DPEphyMpllRegValue.TPLL_N_Div = 1;
            DPEphyMpllRegMask.Value = 0xFFFFFFFF;
            DPEphyMpllRegMask.TPLL_N_Div = 0;
            cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);
        }
        else if (ClockFreq >= 425000)
        {
            DPEphyMpllRegValue.Value = 0;
            DPEphyMpllRegValue.TPLL_N_Div = 2;
            DPEphyMpllRegMask.Value = 0xFFFFFFFF;
            DPEphyMpllRegMask.TPLL_N_Div = 0;
            cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);
        }
        else
        {
            DPEphyMpllRegValue.Value = 0;
            DPEphyMpllRegValue.TPLL_N_Div = 3;
            DPEphyMpllRegMask.Value = 0xFFFFFFFF;
            DPEphyMpllRegMask.TPLL_N_Div = 0;
            cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);
        }

        DPEphyStatusRegValue.Value = 0;
        DPEphyStatusRegValue.EPHY1_TPLL_ISEL = 0;
        if ((ClockFreq == 5940000 && (pcbe->ChipID == CHIPID_E3K || pcbe->ChipID == CHIPID_ARISE10C0T)) || (ClockFreq != 5940000 && bACE))
        {
            DPEphyStatusRegValue.TR = 0;
            DPEphyStatusRegValue.TC = 7;
        }
        else
        {
            DPEphyStatusRegValue.TR = 7;
            DPEphyStatusRegValue.TC = 3;
        }
        DPEphyStatusRegMask.Value = 0xFFFFFFFF;
        DPEphyStatusRegMask.EPHY1_TPLL_ISEL = 0;
        DPEphyStatusRegMask.TR = 0;
        DPEphyStatusRegMask.TC = 0;

        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_STATUS[DPModuleIndex], DPEphyStatusRegValue.Value, DPEphyStatusRegMask.Value);

        DPEphySetting1RegValue.Value = 0;
        DPEphySetting1RegValue.EPHY1_FBOOST_2 = 0;
        DPEphySetting1RegValue.EPHY1_FBOOST_1 = 0;

        if (ClockFreq >= 3400000)
        {
            DPEphySetting1RegValue.EPHY1_HDCKBY4 = 1;    // HDMI clock divided by 4 (HDMI 2.0)
            DPEphySetting1RegValue.EPHY1_SR_SPD = 0;
            DPEphySetting1RegValue.EPHY1_SR_DLY = 0;
            DPEphySetting1RegValue.EPHY1_SR_NDLY = 0;
            if (ClockFreq == 5940000 && (pcbe->ChipID == CHIPID_E3K || pcbe->ChipID == CHIPID_ARISE10C0T || bACE))     //signal will be better if set FBOOST = 1 when clock is 594M
            {
                DPEphySetting1RegValue.EPHY1_FBOOST = 2;
            }
            else
            {
                DPEphySetting1RegValue.EPHY1_FBOOST = 1;
            }
        }
        else if(ClockFreq >= 1700000)
        {
            DPEphySetting1RegValue.EPHY1_HDCKBY4 = 0;
            DPEphySetting1RegValue.EPHY1_SR_SPD = 0;
            DPEphySetting1RegValue.EPHY1_SR_DLY = 0;
            DPEphySetting1RegValue.EPHY1_SR_NDLY = 0;
            DPEphySetting1RegValue.EPHY1_FBOOST = 0;
        }
        else if(ClockFreq >= 850000)
        {
            DPEphySetting1RegValue.EPHY1_HDCKBY4 = 0;
            DPEphySetting1RegValue.EPHY1_SR_SPD = 0;
            DPEphySetting1RegValue.EPHY1_SR_DLY = 0;
            DPEphySetting1RegValue.EPHY1_SR_NDLY = 0;
            DPEphySetting1RegValue.EPHY1_FBOOST = 0;
        }
        else if (ClockFreq >= 425000)
        {
            DPEphySetting1RegValue.EPHY1_HDCKBY4 = 0;
            DPEphySetting1RegValue.EPHY1_SR_SPD = 0;
            DPEphySetting1RegValue.EPHY1_SR_DLY = 0;
            DPEphySetting1RegValue.EPHY1_SR_NDLY = 0;
            DPEphySetting1RegValue.EPHY1_FBOOST = 0;
        }
        else
        {
            DPEphySetting1RegValue.EPHY1_HDCKBY4 = 0;
            DPEphySetting1RegValue.EPHY1_SR_SPD = 0;
            DPEphySetting1RegValue.EPHY1_SR_DLY = 0;
            DPEphySetting1RegValue.EPHY1_SR_NDLY = 0;
            DPEphySetting1RegValue.EPHY1_FBOOST = 0;
        }

        DPEphySetting1RegMask.Value = 0xFFFFFFFF;
        DPEphySetting1RegMask.EPHY1_HDCKBY4 = 0;
        DPEphySetting1RegMask.EPHY1_FBOOST = 0;
        DPEphySetting1RegMask.EPHY1_FBOOST_1 = 0;
        DPEphySetting1RegMask.EPHY1_FBOOST_2 = 0;
        DPEphySetting1RegMask.EPHY1_SR_SPD = 0;
        DPEphySetting1RegMask.EPHY1_SR_DLY = 0;
        DPEphySetting1RegMask.EPHY1_SR_NDLY = 0;
        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_SETTING1[DPModuleIndex], DPEphySetting1RegValue.Value, DPEphySetting1RegMask.Value);

        DPEphyCtrlRegValue.Value = 0;
        DPEphyCtrlRegValue.TX0T = 0;
        DPEphyCtrlRegValue.TX1T = 0;
        DPEphyCtrlRegValue.TX2T = 0;
        DPEphyCtrlRegValue.TX3T = 0;
        if(ClockFreq >= 3400000)
        {
            if (ClockFreq == 5940000 && (pcbe->ChipID == CHIPID_E3K || pcbe->ChipID == CHIPID_ARISE10C0T))
            {
                DPEphyCtrlRegValue.DIAJ_L0 = 3;
                DPEphyCtrlRegValue.DIAJ_L1 = 5;
            }
            else
            {
                DPEphyCtrlRegValue.DIAJ_L0 = 4;
                DPEphyCtrlRegValue.DIAJ_L1 = 4;
            }
            DPEphyCtrlRegValue.DIAJ_L2 = 4;
            DPEphyCtrlRegValue.DIAJ_L3 = 2;
        }
        else if(ClockFreq >= 1700000)
        {
            DPEphyCtrlRegValue.DIAJ_L0 = 5;
            DPEphyCtrlRegValue.DIAJ_L1 = 5;
            DPEphyCtrlRegValue.DIAJ_L2 = 5;
            DPEphyCtrlRegValue.DIAJ_L3 = 5;
        }
        else if(ClockFreq >= 850000)
        {
            DPEphyCtrlRegValue.DIAJ_L0 = 3;
            DPEphyCtrlRegValue.DIAJ_L1 = 3;
            DPEphyCtrlRegValue.DIAJ_L2 = 3;
            DPEphyCtrlRegValue.DIAJ_L3 = 3;
        }
        else
        {
            DPEphyCtrlRegValue.DIAJ_L0 = 4;
            DPEphyCtrlRegValue.DIAJ_L1 = 4;
            DPEphyCtrlRegValue.DIAJ_L2 = 4;
            DPEphyCtrlRegValue.DIAJ_L3 = 4;
        }
        DPEphyCtrlRegMask.Value = 0xFFFFFFFF;
        DPEphyCtrlRegMask.TX0T = 0;
        DPEphyCtrlRegMask.TX1T = 0;
        DPEphyCtrlRegMask.TX2T = 0;
        DPEphyCtrlRegMask.TX3T = 0;
        DPEphyCtrlRegMask.DIAJ_L0 = 0;
        DPEphyCtrlRegMask.DIAJ_L1 = 0;
        DPEphyCtrlRegMask.DIAJ_L2 = 0;
        DPEphyCtrlRegMask.DIAJ_L3 = 0;

        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_CTRL[DPModuleIndex], DPEphyCtrlRegValue.Value, DPEphyCtrlRegMask.Value);

        DPEphySetting2RegValue.Value = 0;
        if(ClockFreq >= 3400000)
        {
            DPEphySetting2RegValue.EPHY1_TXDU_L0 = 0x3B;
            DPEphySetting2RegValue.EPHY1_TXDU_L1 = 0x3B;
            DPEphySetting2RegValue.EPHY1_TXDU_L2 = 0x3B;
            DPEphySetting2RegValue.EPHY1_TXDU_L3 = 0x3F;
        }
        else
        {
            DPEphySetting2RegValue.EPHY1_TXDU_L0 = 0x3B;
            DPEphySetting2RegValue.EPHY1_TXDU_L1 = 0x3B;
            DPEphySetting2RegValue.EPHY1_TXDU_L2 = 0x3B;
            DPEphySetting2RegValue.EPHY1_TXDU_L3 = 0x3F;
        }
        DPEphySetting2RegValue.EPHY1_TX_VMR = 0xF;
        DPEphySetting2RegValue.EPHY1_TX_VMX = 0;
        DPEphySetting2RegValue.EPHY1_TX_H1V2= 1;

        DPEphySetting2RegMask.Value = 0xFFFFFFFF;
        DPEphySetting2RegMask.EPHY1_TXDU_L0 = 0;
        DPEphySetting2RegMask.EPHY1_TXDU_L1 = 0;
        DPEphySetting2RegMask.EPHY1_TXDU_L2 = 0;
        DPEphySetting2RegMask.EPHY1_TXDU_L3 = 0;
        DPEphySetting2RegMask.EPHY1_TX_VMR = 0;
        DPEphySetting2RegMask.EPHY1_TX_VMX = 0;
        DPEphySetting2RegMask.EPHY1_TX_H1V2= 0;
        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_SETTING2[DPModuleIndex], DPEphySetting2RegValue.Value, DPEphySetting2RegMask.Value);

        /* 2. Set DRV_Current and PRE_Current. Use SW setting*/

        if(ClockFreq > 3400000)
        {
            DPSwingRegMask.Value = 0xFFFFFFFF;
            DPSwingRegMask.enable_SW_swing_pp = 0;
            DPSwingRegMask.SW_swing_SW_PP_SW_post_cursor_load_index = 0;
            DPSwingRegMask.DP1_SW_swing = 0;
            DPSwingRegMask.DP1_SW_pp = 0;
            DPSwingRegMask.DP1_SW_post_cursor = 0;
            if(ClockFreq == 5940000 && (pcbe->ChipID == CHIPID_E3K || pcbe->ChipID == CHIPID_ARISE10C0T))
            {
                DPSwingRegValue.Value = 0;
                DPSwingRegValue.enable_SW_swing_pp = 1;
                DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 9;
                DPSwingRegValue.DP1_SW_swing = 0x3F;
                DPSwingRegValue.DP1_SW_pp = 0xA;
                DPSwingRegValue.DP1_SW_post_cursor = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                DPSwingRegValue.Value = 0;
                DPSwingRegValue.enable_SW_swing_pp = 1;
                DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 1;
                DPSwingRegValue.DP1_SW_swing = 0x33;
                DPSwingRegValue.DP1_SW_pp = 9;
                DPSwingRegValue.DP1_SW_post_cursor = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                DPSwingRegValue.Value = 0;
                DPSwingRegValue.enable_SW_swing_pp = 1;
                DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 5;
                DPSwingRegValue.DP1_SW_swing = 0x21;
                DPSwingRegValue.DP1_SW_pp = 0;
                DPSwingRegValue.DP1_SW_post_cursor = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);
            }
            else if(ClockFreq == 5940000 && bACE)
            {
                DPSwingRegValue.Value = 0;
                DPSwingRegValue.enable_SW_swing_pp = 1;
                DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 9;
                DPSwingRegValue.DP1_SW_swing = 0x2F;
                DPSwingRegValue.DP1_SW_pp = 0x9;
                DPSwingRegValue.DP1_SW_post_cursor = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                DPSwingRegValue.Value = 0;
                DPSwingRegValue.enable_SW_swing_pp = 1;
                DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 1;
                DPSwingRegValue.DP1_SW_swing = 0x36;
                DPSwingRegValue.DP1_SW_pp = 0xB;
                DPSwingRegValue.DP1_SW_post_cursor = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                DPSwingRegValue.Value = 0;
                DPSwingRegValue.enable_SW_swing_pp = 1;
                DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 5;
                DPSwingRegValue.DP1_SW_swing = 0x21;
                DPSwingRegValue.DP1_SW_pp = 0;
                DPSwingRegValue.DP1_SW_post_cursor = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);
            }
            else
            {
                DPSwingRegValue.Value = 0;
                DPSwingRegValue.enable_SW_swing_pp = 1;
                DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 1;
                DPSwingRegValue.DP1_SW_swing = 0x3F;
                DPSwingRegValue.DP1_SW_pp = 0x18;
                DPSwingRegValue.DP1_SW_post_cursor = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

                DPSwingRegValue.Value = 0;
                DPSwingRegValue.enable_SW_swing_pp = 1;
                DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 5;
                DPSwingRegValue.DP1_SW_swing = 0x21;
                DPSwingRegValue.DP1_SW_pp = 0;
                DPSwingRegValue.DP1_SW_post_cursor = 0;
                cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);
            }
            DPLinkRegValue.Value = 0;
            DPLinkRegValue.Start_Link_Training = 0;
            DPLinkRegValue.Start_Link_Rate_0 = 0;
            DPLinkRegValue.Max_V_swing = 0;
            DPLinkRegValue.Max_Pre_emphasis = 0;
            DPLinkRegValue.SW_Hpd_assert = 0;
            DPLinkRegValue.Num_of_Lanes = 4;
            DPLinkRegValue.SW_Link_Train_Enable = 1;
            DPLinkRegValue.SW_Link_Train_State = 1;
            DPLinkRegValue.Software_Bit_Rate = 0;
            DPLinkRegValue.SW_Lane0_Swing = 0;
            DPLinkRegValue.SW_Lane0_Pre_emphasis = (ClockFreq == 5940000 && bACE) ? 2 : 0;
            DPLinkRegValue.SW_Lane1_Swing = 0;
            DPLinkRegValue.SW_Lane1_Pre_emphasis = 0;
            DPLinkRegValue.SW_Lane2_Swing = 0;
            DPLinkRegValue.SW_Lane2_Pre_emphasis = 0;
            DPLinkRegValue.SW_Lane3_Swing =0;
            DPLinkRegValue.SW_Lane3_Pre_emphasis = 1;
            DPLinkRegValue.SW_Set_Link_Train_Fail = 0;
            DPLinkRegValue.HW_Link_Training_Done = 0;
            if(ClockFreq == 5940000 && (pcbe->ChipID == CHIPID_E3K || pcbe->ChipID == CHIPID_ARISE10C0T))
            {
                DPLinkRegValue.SW_Lane0_Pre_emphasis = 2;
            }
            DPLinkRegMask.Value = 0;
            cbMMIOWriteReg32(pcbe, DP_REG_LINK[DPModuleIndex], DPLinkRegValue.Value, DPLinkRegMask.Value);
        }
        else if(ClockFreq > 1700000)
        {
            DPSwingRegValue.Value = 0;
            DPSwingRegValue.enable_SW_swing_pp = 1;
            DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 1;
            DPSwingRegValue.DP1_SW_swing = 0x30;
            DPSwingRegValue.DP1_SW_pp = 2;
            DPSwingRegValue.DP1_SW_post_cursor = 0;

            DPSwingRegMask.Value = 0xFFFFFFFF;
            DPSwingRegMask.enable_SW_swing_pp = 0;
            DPSwingRegMask.SW_swing_SW_PP_SW_post_cursor_load_index = 0;
            DPSwingRegMask.DP1_SW_swing = 0;
            DPSwingRegMask.DP1_SW_pp = 0;
            DPSwingRegMask.DP1_SW_post_cursor = 0;
            cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

            DPLinkRegValue.Value = 0;
            DPLinkRegValue.Start_Link_Training = 0;
            DPLinkRegValue.Start_Link_Rate_0 = 0;
            DPLinkRegValue.Max_V_swing = 0;
            DPLinkRegValue.Max_Pre_emphasis = 0;
            DPLinkRegValue.SW_Hpd_assert = 0;
            DPLinkRegValue.Num_of_Lanes = 4;
            DPLinkRegValue.SW_Link_Train_Enable = 1;
            DPLinkRegValue.SW_Link_Train_State = 1;
            DPLinkRegValue.Software_Bit_Rate = 0;
            DPLinkRegValue.SW_Lane0_Swing = 0;
            DPLinkRegValue.SW_Lane0_Pre_emphasis = 0;
            DPLinkRegValue.SW_Lane1_Swing = 0;
            DPLinkRegValue.SW_Lane1_Pre_emphasis = 0;
            DPLinkRegValue.SW_Lane2_Swing = 0;
            DPLinkRegValue.SW_Lane2_Pre_emphasis = 0;
            DPLinkRegValue.SW_Lane3_Swing =0;
            DPLinkRegValue.SW_Lane3_Pre_emphasis = 0;
            DPLinkRegValue.SW_Set_Link_Train_Fail = 0;
            DPLinkRegValue.HW_Link_Training_Done = 0;
            DPLinkRegMask.Value = 0;
            cbMMIOWriteReg32(pcbe, DP_REG_LINK[DPModuleIndex], DPLinkRegValue.Value, DPLinkRegMask.Value);
        }
        else if(ClockFreq > 850000)
        {
            DPSwingRegValue.Value = 0;
            DPSwingRegValue.enable_SW_swing_pp = 1;
            DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 1;
            DPSwingRegValue.DP1_SW_swing = 0x2C;
            DPSwingRegValue.DP1_SW_pp = 2;
            DPSwingRegValue.DP1_SW_post_cursor = 0;

            DPSwingRegMask.Value = 0xFFFFFFFF;
            DPSwingRegMask.enable_SW_swing_pp = 0;
            DPSwingRegMask.SW_swing_SW_PP_SW_post_cursor_load_index = 0;
            DPSwingRegMask.DP1_SW_swing = 0;
            DPSwingRegMask.DP1_SW_pp = 0;
            DPSwingRegMask.DP1_SW_post_cursor = 0;
            cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

            DPLinkRegValue.Value = 0;
            DPLinkRegValue.Start_Link_Training = 0;
            DPLinkRegValue.Start_Link_Rate_0 = 0;
            DPLinkRegValue.Max_V_swing = 0;
            DPLinkRegValue.Max_Pre_emphasis = 0;
            DPLinkRegValue.SW_Hpd_assert = 0;
            DPLinkRegValue.Num_of_Lanes = 4;
            DPLinkRegValue.SW_Link_Train_Enable = 1;
            DPLinkRegValue.SW_Link_Train_State = 1;
            DPLinkRegValue.Software_Bit_Rate = 0;
            DPLinkRegValue.SW_Lane0_Swing = 0;
            DPLinkRegValue.SW_Lane0_Pre_emphasis = 0;
            DPLinkRegValue.SW_Lane1_Swing = 0;
            DPLinkRegValue.SW_Lane1_Pre_emphasis = 0;
            DPLinkRegValue.SW_Lane2_Swing = 0;
            DPLinkRegValue.SW_Lane2_Pre_emphasis = 0;
            DPLinkRegValue.SW_Lane3_Swing =0;
            DPLinkRegValue.SW_Lane3_Pre_emphasis = 0;
            DPLinkRegValue.SW_Set_Link_Train_Fail = 0;
            DPLinkRegValue.HW_Link_Training_Done = 0;
            DPLinkRegMask.Value = 0;
            cbMMIOWriteReg32(pcbe, DP_REG_LINK[DPModuleIndex], DPLinkRegValue.Value, DPLinkRegMask.Value);
        }
        else
        {
            DPSwingRegValue.Value = 0;
            DPSwingRegValue.enable_SW_swing_pp = 1;
            DPSwingRegValue.SW_swing_SW_PP_SW_post_cursor_load_index = 1;
            DPSwingRegValue.DP1_SW_swing = 0xD;
            DPSwingRegValue.DP1_SW_pp = 0;
            DPSwingRegValue.DP1_SW_post_cursor = 0;

            DPSwingRegMask.Value = 0xFFFFFFFF;
            DPSwingRegMask.enable_SW_swing_pp = 0;
            DPSwingRegMask.SW_swing_SW_PP_SW_post_cursor_load_index = 0;
            DPSwingRegMask.DP1_SW_swing = 0;
            DPSwingRegMask.DP1_SW_pp = 0;
            DPSwingRegMask.DP1_SW_post_cursor = 0;
            cbMMIOWriteReg32(pcbe, DP_REG_SWING[DPModuleIndex], DPSwingRegValue.Value, DPSwingRegMask.Value);

            DPLinkRegValue.Value = 0;
            DPLinkRegValue.Start_Link_Training = 0;
            DPLinkRegValue.Start_Link_Rate_0 = 0;
            DPLinkRegValue.Max_V_swing = 0;
            DPLinkRegValue.Max_Pre_emphasis = 0;
            DPLinkRegValue.SW_Hpd_assert = 0;
            DPLinkRegValue.Num_of_Lanes = 4;
            DPLinkRegValue.SW_Link_Train_Enable = 1;
            DPLinkRegValue.SW_Link_Train_State = 1;
            DPLinkRegValue.Software_Bit_Rate = 0;
            DPLinkRegValue.SW_Lane0_Swing = 0;
            DPLinkRegValue.SW_Lane0_Pre_emphasis = 0;
            DPLinkRegValue.SW_Lane1_Swing = 0;
            DPLinkRegValue.SW_Lane1_Pre_emphasis = 0;
            DPLinkRegValue.SW_Lane2_Swing = 0;
            DPLinkRegValue.SW_Lane2_Pre_emphasis = 0;
            DPLinkRegValue.SW_Lane3_Swing =0;
            DPLinkRegValue.SW_Lane3_Pre_emphasis = 0;
            DPLinkRegValue.SW_Set_Link_Train_Fail = 0;
            DPLinkRegValue.HW_Link_Training_Done = 0;
            DPLinkRegMask.Value = 0;
            cbMMIOWriteReg32(pcbe, DP_REG_LINK[DPModuleIndex], DPLinkRegValue.Value, DPLinkRegMask.Value);
        }

        /* 3. Enable Bandgap, disable MPLL*/
        DPEphyMpllRegValue.Value = 0;
        DPEphyMpllRegValue.Bandgap_Power_Down = 1;
        DPEphyMpllRegValue.MPLL_Reg_Power_Down = 0;
        DPEphyMpllRegValue.MPLL_Power_Down = 0;

        DPEphyMpllRegMask.Value = 0xFFFFFFFF;
        DPEphyMpllRegMask.Bandgap_Power_Down = 0;
        DPEphyMpllRegMask.MPLL_Reg_Power_Down = 0;
        DPEphyMpllRegMask.MPLL_Power_Down = 0;

        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);

        /* 4. Enable TPLL*/
        DPEphyMpllRegValue.Value = 0;
        DPEphyMpllRegValue.TPLL_Reg_Power_Down = 1;
        DPEphyMpllRegValue.TPLL_Power_Down = 1;

        DPEphyMpllRegMask.Value = 0xFFFFFFFF;
        DPEphyMpllRegMask.TPLL_Reg_Power_Down = 0;
        DPEphyMpllRegMask.TPLL_Power_Down = 0;
        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);

        cb_DelayMicroSeconds(1500);

        /* 5. Check TPLL Lock Status */
        DPEphyMiscRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_EPHY_MISC[DPModuleIndex]);
        if (DPEphyMiscRegValue.TPLL_Lock_Indicator == 0)
        {
            cbDebugPrint((MAKE_LEVEL(DP, ERROR),"%s: TPLL is not locked.\n", FUNCTION_NAME));
        }

        /* 6. Enable TX Power State Machine. */
        DPEphyTxRegValue.Value = 0;
        DPEphyTxRegValue.TX_Power_Control_Lane0 = 3;
        DPEphyTxRegValue.TX_Power_Control_Lane1 = 3;
        DPEphyTxRegValue.TX_Power_Control_Lane2 = 3;
        DPEphyTxRegValue.TX_Power_Control_Lane3 = 3;

        DPEphyTxRegMask.Value = 0xFFFFFFFF;
        DPEphyTxRegMask.TX_Power_Control_Lane0 = 0;
        DPEphyTxRegMask.TX_Power_Control_Lane1 = 0;
        DPEphyTxRegMask.TX_Power_Control_Lane2 = 0;
        DPEphyTxRegMask.TX_Power_Control_Lane3 = 0;

        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);

        cb_DelayMicroSeconds(20);

        DPEphyTxRegValue.Value = 0;
        DPEphyTxRegValue.TX_Power_Control_Lane0 = 2;
        DPEphyTxRegValue.TX_Power_Control_Lane1 = 2;
        DPEphyTxRegValue.TX_Power_Control_Lane2 = 2;
        DPEphyTxRegValue.TX_Power_Control_Lane3 = 2;

        DPEphyTxRegMask.Value = 0xFFFFFFFF;
        DPEphyTxRegMask.TX_Power_Control_Lane0 = 0;
        DPEphyTxRegMask.TX_Power_Control_Lane1 = 0;
        DPEphyTxRegMask.TX_Power_Control_Lane2 = 0;
        DPEphyTxRegMask.TX_Power_Control_Lane3 = 0;

        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);

        DPEphyTxRegValue.Value = 0;
        DPEphyTxRegValue.TX_Reg_Power_Down_Lane0 = 1;
        DPEphyTxRegValue.TX_Reg_Power_Down_Lane1 = 1;
        DPEphyTxRegValue.TX_Reg_Power_Down_Lane2 = 1;
        DPEphyTxRegValue.TX_Reg_Power_Down_Lane3 = 1;

        DPEphyTxRegMask.Value = 0xFFFFFFFF;
        DPEphyTxRegMask.TX_Reg_Power_Down_Lane0 = 0;
        DPEphyTxRegMask.TX_Reg_Power_Down_Lane1 = 0;
        DPEphyTxRegMask.TX_Reg_Power_Down_Lane2 = 0;
        DPEphyTxRegMask.TX_Reg_Power_Down_Lane3 = 0;

        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);

        cbDelayMilliSeconds(2);//2ms

        DPEphyTxRegValue.Value = 0;
        DPEphyTxRegValue.TX_Power_Control_Lane0 = 0;
        DPEphyTxRegValue.TX_Power_Control_Lane1 = 0;
        DPEphyTxRegValue.TX_Power_Control_Lane2 = 0;
        DPEphyTxRegValue.TX_Power_Control_Lane3 = 0;

        DPEphyTxRegMask.Value = 0xFFFFFFFF;
        DPEphyTxRegMask.TX_Power_Control_Lane0 = 0;
        DPEphyTxRegMask.TX_Power_Control_Lane1 = 0;
        DPEphyTxRegMask.TX_Power_Control_Lane2 = 0;
        DPEphyTxRegMask.TX_Power_Control_Lane3 = 0;

        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);

        cb_DelayMicroSeconds(1);

        DPEphyTxRegValue.Value = 0;
        DPEphyTxRegValue.Driver_Control_Lane0 = 1;
        DPEphyTxRegValue.Driver_Control_Lane1 = 1;
        DPEphyTxRegValue.Driver_Control_Lane2 = 1;
        DPEphyTxRegValue.Driver_Control_Lane3 = 1;

        DPEphyTxRegMask.Value = 0xFFFFFFFF;
        DPEphyTxRegMask.Driver_Control_Lane0 = 0;
        DPEphyTxRegMask.Driver_Control_Lane1 = 0;
        DPEphyTxRegMask.Driver_Control_Lane2 = 0;
        DPEphyTxRegMask.Driver_Control_Lane3 = 0;

        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);
    }
    else    // turn off
    {
        DPEphyMpllRegValue.Value = 0;
        DPEphyMpllRegValue.Bandgap_Power_Down = 0;
        DPEphyMpllRegValue.MPLL_Reg_Power_Down = 0;
        DPEphyMpllRegValue.MPLL_Power_Down = 0;
        DPEphyMpllRegValue.TPLL_Reg_Power_Down = 0;
        DPEphyMpllRegValue.TPLL_Power_Down = 0;
        DPEphyMpllRegMask.Value = 0xFFFFFFFF;
        DPEphyMpllRegMask.Bandgap_Power_Down = 0;
        DPEphyMpllRegMask.MPLL_Reg_Power_Down = 0;
        DPEphyMpllRegMask.MPLL_Power_Down = 0;
        DPEphyMpllRegMask.TPLL_Reg_Power_Down = 0;
        DPEphyMpllRegMask.TPLL_Power_Down = 0;

        DPEphyTxRegValue.Value = 0;
        DPEphyTxRegValue.EPHY_MPLL_CP = 0;
        DPEphyTxRegValue.TX_Power_Control_Lane0 = 3;
        DPEphyTxRegValue.TX_Power_Control_Lane1 = 3;
        DPEphyTxRegValue.TX_Power_Control_Lane2 = 3;
        DPEphyTxRegValue.TX_Power_Control_Lane3 = 3;
        DPEphyTxRegValue.TX_Reg_Power_Down_Lane0 = 0;
        DPEphyTxRegValue.TX_Reg_Power_Down_Lane1 = 0;
        DPEphyTxRegValue.TX_Reg_Power_Down_Lane2 = 0;
        DPEphyTxRegValue.TX_Reg_Power_Down_Lane3 = 0;
        DPEphyTxRegValue.Driver_Control_Lane0 = 0;
        DPEphyTxRegValue.Driver_Control_Lane1 = 0;
        DPEphyTxRegValue.Driver_Control_Lane2 = 0;
        DPEphyTxRegValue.Driver_Control_Lane3 = 0;
        DPEphyTxRegMask.Value = 0xFFFFFFFF;
        DPEphyTxRegMask.EPHY_MPLL_CP = 0;
        DPEphyTxRegMask.TX_Power_Control_Lane0 = 0;
        DPEphyTxRegMask.TX_Power_Control_Lane1 = 0;
        DPEphyTxRegMask.TX_Power_Control_Lane2 = 0;
        DPEphyTxRegMask.TX_Power_Control_Lane3 = 0;
        DPEphyTxRegMask.TX_Reg_Power_Down_Lane0 = 0;
        DPEphyTxRegMask.TX_Reg_Power_Down_Lane1 = 0;
        DPEphyTxRegMask.TX_Reg_Power_Down_Lane2 = 0;
        DPEphyTxRegMask.TX_Reg_Power_Down_Lane3 = 0;
        DPEphyTxRegMask.Driver_Control_Lane0 = 0;
        DPEphyTxRegMask.Driver_Control_Lane1 = 0;
        DPEphyTxRegMask.Driver_Control_Lane2 = 0;
        DPEphyTxRegMask.Driver_Control_Lane3 = 0;

        DPEphyMiscRegValue.Value = 0;
        DPEphyMiscRegValue.AUC_Ch_Op_Mode = 0;
        DPEphyMiscRegMask.Value = 0xFFFFFFFF;
        DPEphyMiscRegMask.AUC_Ch_Op_Mode = 0;

        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);
        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);
        cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MISC[DPModuleIndex], DPEphyMiscRegValue.Value, DPEphyMiscRegMask.Value);
    }

    cbTraceExit(DP);
}

CBIOS_VOID cbPHY_DP_InitEPHY(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8240     DPEnableRegValue, DPEnableRegMask;
    REG_MM8214     DPLinkRegValue, DPLinkRegMask;
    REG_MM82CC     DPEphyCtrlRegValue, DPEphyCtrlRegMask;
    REG_MM8340     DPEphyMpllRegValue, DPEphyMpllRegMask;
    REG_MM8344     DPEphyTxRegValue, DPEphyTxRegMask;
    REG_MM8348     DPEphyMiscRegValue, DPEphyMiscRegMask;
    REG_MM836C     DPEphyStatusRegValue, DPEphyStatusRegMask;
    REG_MM334CC    DPCtrl2RegValue, DPCtrl2RegMask;
    REG_MM334E0    DPEphySetting1RegValue, DPEphySetting1RegMask;
    REG_MM334E4    DPEphySetting2RegValue, DPEphySetting2RegMask;
    REG_MM334C8    DPLinkCtrlRegValue, DPLinkCtrlRegMask;
    CBIOS_BOOL    bACE = ((pcbe->ChipID == CHIPID_ARISE2030) || (pcbe->ChipID == CHIPID_ARISE2020)) ? CBIOS_TRUE : CBIOS_FALSE;

    cbTraceEnter(DP);

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return;
    }

    //Enable DP
    DPEnableRegValue.Value = 0;
    DPEnableRegValue.DP_Enable = 0;
    DPEnableRegMask.Value = 0xFFFFFFFF;
    DPEnableRegMask.DP_Enable = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_ENABLE[DPModuleIndex], DPEnableRegValue.Value, DPEnableRegMask.Value);

    DPLinkRegValue.Value = 0;
    DPLinkRegValue.Max_V_swing = 3;
    DPLinkRegValue.Max_Pre_emphasis = 2;
    DPLinkRegMask.Value = 0xFFFFFFFF;
    DPLinkRegMask.Max_V_swing = 0;
    DPLinkRegMask.Max_Pre_emphasis = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_LINK[DPModuleIndex], DPLinkRegValue.Value, DPLinkRegMask.Value);

    DPLinkCtrlRegValue.Value = 0;
    DPLinkCtrlRegValue.MAX_POST_EMPHASIS = 0;
    DPLinkCtrlRegValue.DP_SUPPORT_POST_CURSOR = 0;
    DPLinkCtrlRegMask.Value = 0xFFFFFFFF;
    DPLinkCtrlRegMask.MAX_POST_EMPHASIS = 0;
    DPLinkCtrlRegMask.DP_SUPPORT_POST_CURSOR = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_LINK_CTRL[DPModuleIndex], DPLinkCtrlRegValue.Value, DPLinkCtrlRegMask.Value);

    // Select EPHY MPLL Clock as DP clock
    DPEphyCtrlRegValue.Value = 0;
    DPEphyCtrlRegValue.DP_Clock_Debug = 0;
    DPEphyCtrlRegValue.check_sync_cnt = 0x1;//HW don't check sync counter for aux receiver
    DPEphyCtrlRegValue.DIAJ_L0 = 0x4;
    DPEphyCtrlRegValue.DIAJ_L1 = 0x4;
    DPEphyCtrlRegValue.DIAJ_L2 = 0x4;
    DPEphyCtrlRegValue.DIAJ_L3 = 0x4;
    DPEphyCtrlRegMask.Value = 0xFFFFFFFF;
    DPEphyCtrlRegMask.DP_Clock_Debug = 0;
    DPEphyCtrlRegMask.check_sync_cnt = 0x0;
    DPEphyCtrlRegMask.DIAJ_L0 = 0;
    DPEphyCtrlRegMask.DIAJ_L1 = 0;
    DPEphyCtrlRegMask.DIAJ_L2 = 0;
    DPEphyCtrlRegMask.DIAJ_L3 = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_CTRL[DPModuleIndex], DPEphyCtrlRegValue.Value, DPEphyCtrlRegMask.Value);

    // Disable Bandgap, MPLL and TPLL
    DPEphyMpllRegValue.Value = 0;
    DPEphyMpllRegValue.Bandgap_Power_Down = 0;
    DPEphyMpllRegValue.MPLL_Reg_Power_Down = 0;
    DPEphyMpllRegValue.MPLL_Power_Down = 0;
    DPEphyMpllRegValue.TPLL_Reg_Power_Down = 0;
    DPEphyMpllRegValue.TPLL_Power_Down = 0;
    DPEphyMpllRegMask.Value = 0xFFFFFFFF;
    DPEphyMpllRegMask.Bandgap_Power_Down = 0;
    DPEphyMpllRegMask.MPLL_Reg_Power_Down = 0;
    DPEphyMpllRegMask.MPLL_Power_Down = 0;
    DPEphyMpllRegMask.TPLL_Reg_Power_Down = 0;
    DPEphyMpllRegMask.TPLL_Power_Down = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);

    // Bandgap power up
    DPEphyMpllRegValue.Value = 0;
    DPEphyMpllRegValue.Bandgap_Power_Down = 1;
    DPEphyMpllRegMask.Value = 0xFFFFFFFF;
    DPEphyMpllRegMask.Bandgap_Power_Down = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);


    //MPLL & SSC
    DPEphyMpllRegValue.Value = 0;
    DPEphyMpllRegValue.MPLL_R = 0;
    DPEphyMpllRegMask.Value = 0xFFFFFFFF;
    DPEphyMpllRegMask.MPLL_R = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);

    //MPLL PTAT Current = 16uA. mm8340[4:3] = 2'b01
    //MPLL CP Current = 8uA. {mm8340[7:5], mm8344[23]} = 4'b1000
    DPEphyMpllRegValue.Value = 0;
    DPEphyMpllRegValue.MPLL_PTAT_Current = 1;
    DPEphyMpllRegValue.MPLL_CP_Current = 4;
    DPEphyTxRegValue.Value = 0;
    DPEphyTxRegValue.EPHY_MPLL_CP = 0;

    DPEphyMpllRegMask.Value = 0xFFFFFFFF;
    DPEphyMpllRegMask.MPLL_PTAT_Current = 0;
    DPEphyMpllRegMask.MPLL_CP_Current = 0;
    DPEphyTxRegMask.Value = 0xFFFFFFFF;
    DPEphyTxRegMask.EPHY_MPLL_CP = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);

    //MPLL N, P. MPLL Clk=ref_clk*(N+2)/P. ref_clk=13.5MHz=27MHz/2=input_clk/R.
    //RBR:  N=0xBE, P=16
    //HBR:  N=0x9E, P=8
    //HBR2: N=0x9E, P=4
    DPEphyMpllRegValue.Value = 0;
    DPEphyMpllRegValue.MPLL_N = 0xBE;    // RBR 1.62G
    DPEphyMpllRegValue.MPLL_P = 2;       // RBR 1.62G
    DPEphyMpllRegMask.Value = 0xFFFFFFFF;
    DPEphyMpllRegMask.MPLL_N = 0;
    DPEphyMpllRegMask.MPLL_P = 0;
    DPCtrl2RegValue.Value = 0;
    DPCtrl2RegValue.SW_MPLL_M_1 = 0x9E;  // HBR 2.7G
    DPCtrl2RegValue.SW_MPLL_P_1 = 1;     // HBR 2.7G
    DPCtrl2RegValue.SW_MPLL_M_2 = 0x9E;  // HBR2 5.4G
    DPCtrl2RegValue.SW_MPLL_P_2 = 0;     // HBR2 5.4G
    DPCtrl2RegMask.Value = 0xFFFFFFFF;
    DPCtrl2RegMask.SW_MPLL_M_1 = 0;
    DPCtrl2RegMask.SW_MPLL_P_1 = 0;
    DPCtrl2RegMask.SW_MPLL_M_2 = 0;
    DPCtrl2RegMask.SW_MPLL_P_2 = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);
    cbMMIOWriteReg32(pcbe, DP_REG_CTRL2[DPModuleIndex], DPCtrl2RegValue.Value, DPCtrl2RegMask.Value);

    //MPLL loop filter R and C
    DPEphyStatusRegValue.Value = 0;
    DPEphyStatusRegValue.MR = 0;
    DPEphyStatusRegValue.MC = 0;
    DPEphyStatusRegMask.Value = 0xFFFFFFFF;
    DPEphyStatusRegMask.MR = 0;
    DPEphyStatusRegMask.MC = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_STATUS[DPModuleIndex], DPEphyStatusRegValue.Value, DPEphyStatusRegMask.Value);


    // SSC OFF
    DPEphyMpllRegValue.Value = 0;
    DPEphyMpllRegValue.SSC_Enable = 0;
    DPEphyMpllRegMask.Value = 0xFFFFFFFF;
    DPEphyMpllRegMask.SSC_Enable = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);

    // SSC Frequency Spread Magnitude peak-to-peak control = 0.25%. SSC Frequency Spread = down_spread. SSC Dither Control = dither_off. SSC Modulating Signal Profile = asymmetric_triangular
    DPEphyMpllRegValue.Value = 0;
    DPEphyMpllRegValue.SSC_Freq_Spread = 0;
    DPEphyMpllRegValue.Dither = 0;
    DPEphyMpllRegValue.Signal_Profile = 1;
    DPEphyMpllRegValue.Spread_Magnitude = 0;
    DPEphyMpllRegMask.Value = 0xFFFFFFFF;
    DPEphyMpllRegMask.SSC_Freq_Spread = 0;
    DPEphyMpllRegMask.Dither = 0;
    DPEphyMpllRegMask.Signal_Profile = 0;
    DPEphyMpllRegMask.Spread_Magnitude = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);

    // MPLL Power Up
    DPEphyMpllRegValue.Value = 0;
    DPEphyMpllRegValue.MPLL_Power_Down = 1;
    DPEphyMpllRegMask.Value = 0xFFFFFFFF;
    DPEphyMpllRegMask.MPLL_Power_Down = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);

    // MPLL Regulator Power Up
    DPEphyMpllRegValue.Value = 0;
    DPEphyMpllRegValue.MPLL_Reg_Power_Down = 1;
    DPEphyMpllRegMask.Value = 0xFFFFFFFF;
    DPEphyMpllRegMask.MPLL_Reg_Power_Down = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);
    cb_DelayMicroSeconds(1000);
    // Check MPLL Lock Indicator
    DPEphyMiscRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_EPHY_MISC[DPModuleIndex]);
    if (DPEphyMiscRegValue.MPLL_Lock_Indicator == 0)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR),"%s: MPLL is not locked.\n", FUNCTION_NAME));
    }

    // TPLL
    // TPLL Charge pump current = 8uA
    DPEphyMiscRegValue.Value = 0;
    DPEphyMiscRegValue.EPHY1_TPLL_CP = 8;
    DPEphyMiscRegMask.Value = 0xFFFFFFFF;
    DPEphyMiscRegMask.EPHY1_TPLL_CP = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MISC[DPModuleIndex], DPEphyMiscRegValue.Value, DPEphyMiscRegMask.Value);

    // TPLL Nx
    DPEphyMpllRegValue.Value = 0;
    DPEphyMpllRegValue.TPLL_N_Div = 1;
    DPEphyMpllRegMask.Value = 0xFFFFFFFF;
    DPEphyMpllRegMask.TPLL_N_Div = 0;
    DPCtrl2RegValue.Value = 0;
    DPCtrl2RegValue.SW_NX_P = 1;
    DPCtrl2RegValue.SW_NX_P_1 = 1;
    DPCtrl2RegValue.SW_MPLL_P_2 = 0;
    DPCtrl2RegMask.Value = 0xFFFFFFFF;
    DPCtrl2RegMask.SW_NX_P = 0;
    DPCtrl2RegMask.SW_NX_P_1 = 0;
    DPCtrl2RegMask.SW_NX_P_2 = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);
    cbMMIOWriteReg32(pcbe, DP_REG_CTRL2[DPModuleIndex], DPCtrl2RegValue.Value, DPCtrl2RegMask.Value);

    // TPLL FBoost, VCO frequency boost.
    DPEphySetting1RegValue.Value = 0;
    DPEphySetting1RegValue.EPHY1_FBOOST = 0;
    DPEphySetting1RegValue.EPHY1_FBOOST_1 = 0;
    DPEphySetting1RegValue.EPHY1_FBOOST_2 = 1;
    DPEphySetting1RegValue.EPHY1_HDCKBY4 = 0;
    DPEphySetting1RegMask.Value = 0xFFFFFFFF;
    DPEphySetting1RegMask.EPHY1_FBOOST = 0;
    DPEphySetting1RegMask.EPHY1_FBOOST_1 = 0;
    DPEphySetting1RegMask.EPHY1_FBOOST_2 = 0;
    DPEphySetting1RegMask.EPHY1_HDCKBY4 = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_SETTING1[DPModuleIndex], DPEphySetting1RegValue.Value, DPEphySetting1RegMask.Value);

    // TPLL bias current, loop filter R and C
    DPEphyStatusRegValue.Value = 0;
    DPEphyStatusRegValue.EPHY1_TPLL_ISEL = 0;
    DPEphyStatusRegValue.TR = 0;
    DPEphyStatusRegValue.TC = 7;
    DPEphyStatusRegMask.Value = 0xFFFFFFFF;
    DPEphyStatusRegMask.EPHY1_TPLL_ISEL = 0;
    DPEphyStatusRegMask.TR = 0;
    DPEphyStatusRegMask.TC = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_STATUS[DPModuleIndex], DPEphyStatusRegValue.Value, DPEphyStatusRegMask.Value);

    // TPLL Power Up
    DPEphyMpllRegValue.Value = 0;
    DPEphyMpllRegValue.TPLL_Power_Down = 1;
    DPEphyMpllRegMask.Value = 0xFFFFFFFF;
    DPEphyMpllRegMask.TPLL_Power_Down = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);

    // TPLL Regulator Power Up
    DPEphyMpllRegValue.Value = 0;
    DPEphyMpllRegValue.TPLL_Reg_Power_Down = 1;
    DPEphyMpllRegMask.Value = 0xFFFFFFFF;
    DPEphyMpllRegMask.TPLL_Reg_Power_Down = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);
    cb_DelayMicroSeconds(1100);
    // Check TPLL Lock Indicator
    // DPEphyMiscRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_EPHY_MISC[DPModuleIndex]);
    // if (DPEphyMiscRegValue.TPLL_Lock_Indicator == 0)
    // {
    //     cbDebugPrint((MAKE_LEVEL(DP, ERROR),"%s: TPLL is not locked.\n", FUNCTION_NAME));
    // }

    // RTN
    // Disable RTN BIST.
    DPEphyMiscRegValue.Value = 0;
    DPEphyMiscRegValue.RTNBIST = 0;
    DPEphyMiscRegMask.Value = 0xFFFFFFFF;
    DPEphyMiscRegMask.RTNBIST = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MISC[DPModuleIndex], DPEphyMiscRegValue.Value, DPEphyMiscRegMask.Value);

    // Auto Calibration
    // Disable resistance overwrite manually mode.
    DPEphyTxRegValue.Value = 0;
    DPEphyTxRegValue.TX_Resistance_Set_Enable = 1;
    DPEphyTxRegMask.Value = 0xFFFFFFFF;
    DPEphyTxRegMask.TX_Resistance_Set_Enable = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);
    // Power down RTN.
    DPEphyTxRegValue.Value = 0;
    DPEphyTxRegValue.Resistance_Tuning_PD = 0;
    DPEphyTxRegMask.Value = 0xFFFFFFFF;
    DPEphyTxRegMask.Resistance_Tuning_PD = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);

    // Disable RTN.
    DPEphyTxRegValue.Value = 0;
    DPEphyTxRegValue.Resistance_Tuning_Enable = 0;
    DPEphyTxRegMask.Value = 0xFFFFFFFF;
    DPEphyTxRegMask.Resistance_Tuning_Enable = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);

    // Reset RTN.
    DPEphyTxRegValue.Value = 0;
    DPEphyTxRegValue.Resistance_Tuning_Reset = 0;
    DPEphyTxRegMask.Value = 0xFFFFFFFF;
    DPEphyTxRegMask.Resistance_Tuning_Reset = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);

    // delay for at least 10ns
    cb_DelayMicroSeconds(1);
    // Power up RTN.
    DPEphyTxRegValue.Value = 0;
    DPEphyTxRegValue.Resistance_Tuning_PD = 1;
    DPEphyTxRegMask.Value = 0xFFFFFFFF;
    DPEphyTxRegMask.Resistance_Tuning_PD = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);

    // delay for at least 10ns
    cb_DelayMicroSeconds(1);
    // De-assert RTN reset.
    DPEphyTxRegValue.Value = 0;
    DPEphyTxRegValue.Resistance_Tuning_Reset = 1;
    DPEphyTxRegMask.Value = 0xFFFFFFFF;
    DPEphyTxRegMask.Resistance_Tuning_Reset = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);

    // delay for at least 160ns
    cb_DelayMicroSeconds(1);
    // Enable RTN.
    DPEphyTxRegValue.Value = 0;
    DPEphyTxRegValue.Resistance_Tuning_Enable = 1;
    DPEphyTxRegMask.Value = 0xFFFFFFFF;
    DPEphyTxRegMask.Resistance_Tuning_Enable = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);

    // delay for at least 20us
    cb_DelayMicroSeconds(1);
    // Disable RTN.
    DPEphyTxRegValue.Value = 0;
    DPEphyTxRegValue.Resistance_Tuning_Enable = 0;
    DPEphyTxRegMask.Value = 0xFFFFFFFF;
    DPEphyTxRegMask.Resistance_Tuning_Enable = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);

    // delay for at least 40ns
    cb_DelayMicroSeconds(1);
    // Power down RTN to hold the result.
    DPEphyTxRegValue.Value = 0;
    DPEphyTxRegValue.Resistance_Tuning_PD = 0;
    DPEphyTxRegMask.Value = 0xFFFFFFFF;
    DPEphyTxRegMask.Resistance_Tuning_PD = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);

    // Read RTN result.
    DPEphyMiscRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, DP_REG_EPHY_MISC[DPModuleIndex]);
    cbDebugPrint((MAKE_LEVEL(DP, DEBUG), "%s: RTN value = 0x%x.\n", FUNCTION_NAME, DPEphyMiscRegValue.RTN_Results));

    // Force on SSC_PDB
    // RTN select bit. 1: lower the termination resistance.
    DPEphyMiscRegValue.Value = 0;
    DPEphyMiscRegValue.M1V = 0;
    DPEphyMiscRegValue.MT = 1;
    DPEphyMiscRegMask.Value = 0xFFFFFFFF;
    DPEphyMiscRegMask.M1V = 0;
    DPEphyMiscRegMask.MT = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MISC[DPModuleIndex], DPEphyMiscRegValue.Value, DPEphyMiscRegMask.Value);

    // TX
    // TX PISO. CKHLD = 2'b00, no delay.
    DPEphyMiscRegValue.Value = 0;
    DPEphyMiscRegValue.CKHLD = 0;
    DPEphyMiscRegMask.Value = 0xFFFFFFFF;
    DPEphyMiscRegMask.CKHLD = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MISC[DPModuleIndex], DPEphyMiscRegValue.Value, DPEphyMiscRegMask.Value);

    // TX driver voltage control, TX_H1V2 = 2'b00(1.16v), {mm8348[16], mm34E4[29]}
    // All the 4 lanes have the same control.
    DPEphyMiscRegValue.Value = 0;
    DPEphyMiscRegValue.TX_High_Impedance_Lane0 = 0;
    DPEphyMiscRegValue.TX_High_Impedance_Lane1 = 0;
    DPEphyMiscRegValue.TX_High_Impedance_Lane2 = 0;
    DPEphyMiscRegValue.TX_High_Impedance_Lane3 = 0;
    DPEphyMiscRegMask.Value = 0xFFFFFFFF;
    DPEphyMiscRegMask.TX_High_Impedance_Lane0 = 0;
    DPEphyMiscRegMask.TX_High_Impedance_Lane1 = 0;
    DPEphyMiscRegMask.TX_High_Impedance_Lane2 = 0;
    DPEphyMiscRegMask.TX_High_Impedance_Lane3 = 0;
    DPEphySetting2RegValue.Value = 0;
    DPEphySetting2RegValue.EPHY1_TX_H1V2 = 0;
    DPEphySetting2RegMask.Value = 0xFFFFFFFF;
    DPEphySetting2RegMask.EPHY1_TX_H1V2 = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MISC[DPModuleIndex], DPEphyMiscRegValue.Value, DPEphyMiscRegMask.Value);
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_SETTING2[DPModuleIndex], DPEphySetting2RegValue.Value, DPEphySetting2RegMask.Value);

    // TX driver slew rate
    DPEphySetting1RegValue.Value = 0;
    DPEphySetting1RegValue.EPHY1_SR_SPD = 0;
    DPEphySetting1RegValue.EPHY1_SR_DLY = 0;
    DPEphySetting1RegValue.EPHY1_SR_NDLY = 0;
    DPEphySetting1RegMask.Value = 0xFFFFFFFF;
    DPEphySetting1RegMask.EPHY1_SR_SPD = 0;
    DPEphySetting1RegMask.EPHY1_SR_DLY = 0;
    DPEphySetting1RegMask.EPHY1_SR_NDLY = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_SETTING1[DPModuleIndex], DPEphySetting1RegValue.Value, DPEphySetting1RegMask.Value);

    // TX output duty-cycle adjust
    DPEphySetting2RegValue.Value = 0;
    if (bACE)
    {
        DPEphySetting2RegValue.EPHY1_TXDU_L0 = 0x3C;
        DPEphySetting2RegValue.EPHY1_TXDU_L1 = 0x3C;
        DPEphySetting2RegValue.EPHY1_TXDU_L2 = 0x3C;
        DPEphySetting2RegValue.EPHY1_TXDU_L3 = 0x3C;
    }
    else
    {
        DPEphySetting2RegValue.EPHY1_TXDU_L0 = 0x3F;
        DPEphySetting2RegValue.EPHY1_TXDU_L1 = 0x3F;
        DPEphySetting2RegValue.EPHY1_TXDU_L2 = 0x3F;
        DPEphySetting2RegValue.EPHY1_TXDU_L3 = 0x3F;
    }

    DPEphySetting2RegValue.EPHY1_TX_VMR = 0xF;
    DPEphySetting2RegValue.EPHY1_TX_VMX = 0;
    DPEphySetting2RegMask.Value = 0xE0000000;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_SETTING2[DPModuleIndex], DPEphySetting2RegValue.Value, DPEphySetting2RegMask.Value);

    // TX Driver Initial
    // TX_PWR_Lx[1:0]=2'b11, REGPDB_Lx=0, EIDLEB_Lx=0.
    DPEphyTxRegValue.Value = 0;
    DPEphyTxRegValue.TX_Reg_Power_Down_Lane0 = 0;
    DPEphyTxRegValue.TX_Reg_Power_Down_Lane1 = 0;
    DPEphyTxRegValue.TX_Reg_Power_Down_Lane2 = 0;
    DPEphyTxRegValue.TX_Reg_Power_Down_Lane3 = 0;
    DPEphyTxRegValue.Driver_Control_Lane0 = 0;
    DPEphyTxRegValue.Driver_Control_Lane1 = 0;
    DPEphyTxRegValue.Driver_Control_Lane2 = 0;
    DPEphyTxRegValue.Driver_Control_Lane3 = 0;
    DPEphyTxRegValue.TX_Power_Control_Lane0 = 3;
    DPEphyTxRegValue.TX_Power_Control_Lane1 = 3;
    DPEphyTxRegValue.TX_Power_Control_Lane2 = 3;
    DPEphyTxRegValue.TX_Power_Control_Lane3 = 3;
    DPEphyTxRegMask.Value = 0xFFFFFFFF;
    DPEphyTxRegMask.TX_Reg_Power_Down_Lane0 = 0;
    DPEphyTxRegMask.TX_Reg_Power_Down_Lane1 = 0;
    DPEphyTxRegMask.TX_Reg_Power_Down_Lane2 = 0;
    DPEphyTxRegMask.TX_Reg_Power_Down_Lane3 = 0;
    DPEphyTxRegMask.Driver_Control_Lane0 = 0;
    DPEphyTxRegMask.Driver_Control_Lane1 = 0;
    DPEphyTxRegMask.Driver_Control_Lane2 = 0;
    DPEphyTxRegMask.Driver_Control_Lane3 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane0 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane1 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane2 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane3 = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);

    // TX_PWR_Lx[1:0]=2'b10, REGPDB_Lx=1, EIDLEB_Lx=0.
    DPEphyTxRegValue.Value = 0;
    DPEphyTxRegValue.TX_Reg_Power_Down_Lane0 = 1;
    DPEphyTxRegValue.TX_Reg_Power_Down_Lane1 = 1;
    DPEphyTxRegValue.TX_Reg_Power_Down_Lane2 = 1;
    DPEphyTxRegValue.TX_Reg_Power_Down_Lane3 = 1;
    DPEphyTxRegValue.Driver_Control_Lane0 = 0;
    DPEphyTxRegValue.Driver_Control_Lane1 = 0;
    DPEphyTxRegValue.Driver_Control_Lane2 = 0;
    DPEphyTxRegValue.Driver_Control_Lane3 = 0;
    DPEphyTxRegValue.TX_Power_Control_Lane0 = 2;
    DPEphyTxRegValue.TX_Power_Control_Lane1 = 2;
    DPEphyTxRegValue.TX_Power_Control_Lane2 = 2;
    DPEphyTxRegValue.TX_Power_Control_Lane3 = 2;
    DPEphyTxRegMask.Value = 0xFFFFFFFF;
    DPEphyTxRegMask.TX_Reg_Power_Down_Lane0 = 0;
    DPEphyTxRegMask.TX_Reg_Power_Down_Lane1 = 0;
    DPEphyTxRegMask.TX_Reg_Power_Down_Lane2 = 0;
    DPEphyTxRegMask.TX_Reg_Power_Down_Lane3 = 0;
    DPEphyTxRegMask.Driver_Control_Lane0 = 0;
    DPEphyTxRegMask.Driver_Control_Lane1 = 0;
    DPEphyTxRegMask.Driver_Control_Lane2 = 0;
    DPEphyTxRegMask.Driver_Control_Lane3 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane0 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane1 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane2 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane3 = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);
    // delay for at least 1.5ms
    cb_DelayMicroSeconds(1500);

    // TX_PWR_Lx[1:0]=2'b00, REGPDB_Lx=1, EIDLEB_Lx=0.
    DPEphyTxRegValue.Value = 0;
    DPEphyTxRegValue.TX_Reg_Power_Down_Lane0 = 1;
    DPEphyTxRegValue.TX_Reg_Power_Down_Lane1 = 1;
    DPEphyTxRegValue.TX_Reg_Power_Down_Lane2 = 1;
    DPEphyTxRegValue.TX_Reg_Power_Down_Lane3 = 1;
    DPEphyTxRegValue.Driver_Control_Lane0 = 0;
    DPEphyTxRegValue.Driver_Control_Lane1 = 0;
    DPEphyTxRegValue.Driver_Control_Lane2 = 0;
    DPEphyTxRegValue.Driver_Control_Lane3 = 0;
    DPEphyTxRegValue.TX_Power_Control_Lane0 = 0;
    DPEphyTxRegValue.TX_Power_Control_Lane1 = 0;
    DPEphyTxRegValue.TX_Power_Control_Lane2 = 0;
    DPEphyTxRegValue.TX_Power_Control_Lane3 = 0;
    DPEphyTxRegMask.Value = 0xFFFFFFFF;
    DPEphyTxRegMask.TX_Reg_Power_Down_Lane0 = 0;
    DPEphyTxRegMask.TX_Reg_Power_Down_Lane1 = 0;
    DPEphyTxRegMask.TX_Reg_Power_Down_Lane2 = 0;
    DPEphyTxRegMask.TX_Reg_Power_Down_Lane3 = 0;
    DPEphyTxRegMask.Driver_Control_Lane0 = 0;
    DPEphyTxRegMask.Driver_Control_Lane1 = 0;
    DPEphyTxRegMask.Driver_Control_Lane2 = 0;
    DPEphyTxRegMask.Driver_Control_Lane3 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane0 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane1 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane2 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane3 = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);
    // delay for at least 100ns
    cb_DelayMicroSeconds(1);

    // TX_PWR_Lx[1:0]=2'b00, REGPDB_Lx=1, EIDLEB_Lx=1.
    DPEphyTxRegValue.Value = 0;
    DPEphyTxRegValue.TX_Reg_Power_Down_Lane0 = 1;
    DPEphyTxRegValue.TX_Reg_Power_Down_Lane1 = 1;
    DPEphyTxRegValue.TX_Reg_Power_Down_Lane2 = 1;
    DPEphyTxRegValue.TX_Reg_Power_Down_Lane3 = 1;
    DPEphyTxRegValue.Driver_Control_Lane0 = 1;
    DPEphyTxRegValue.Driver_Control_Lane1 = 1;
    DPEphyTxRegValue.Driver_Control_Lane2 = 1;
    DPEphyTxRegValue.Driver_Control_Lane3 = 1;
    DPEphyTxRegValue.TX_Power_Control_Lane0 = 0;
    DPEphyTxRegValue.TX_Power_Control_Lane1 = 0;
    DPEphyTxRegValue.TX_Power_Control_Lane2 = 0;
    DPEphyTxRegValue.TX_Power_Control_Lane3 = 0;
    DPEphyTxRegMask.Value = 0xFFFFFFFF;
    DPEphyTxRegMask.TX_Reg_Power_Down_Lane0 = 0;
    DPEphyTxRegMask.TX_Reg_Power_Down_Lane1 = 0;
    DPEphyTxRegMask.TX_Reg_Power_Down_Lane2 = 0;
    DPEphyTxRegMask.TX_Reg_Power_Down_Lane3 = 0;
    DPEphyTxRegMask.Driver_Control_Lane0 = 0;
    DPEphyTxRegMask.Driver_Control_Lane1 = 0;
    DPEphyTxRegMask.Driver_Control_Lane2 = 0;
    DPEphyTxRegMask.Driver_Control_Lane3 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane0 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane1 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane2 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane3 = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);

    // AUX Initial
    // AUX_CTRL_Lx[1:0]=2'b00. AUX CH power off.
    DPEphyMiscRegValue.Value = 0;
    DPEphyMiscRegValue.AUC_Ch_Op_Mode = 0;
    DPEphyMiscRegMask.Value = 0xFFFFFFFF;
    DPEphyMiscRegMask.AUC_Ch_Op_Mode = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MISC[DPModuleIndex], DPEphyMiscRegValue.Value, DPEphyMiscRegMask.Value);

    // AUX_CTRL_Lx[1:0]=2'b01. The CMOP is on; the TX and RX are off.
    DPEphyMiscRegValue.Value = 0;
    DPEphyMiscRegValue.AUC_Ch_Op_Mode = 1;
    DPEphyMiscRegMask.Value = 0xFFFFFFFF;
    DPEphyMiscRegMask.AUC_Ch_Op_Mode = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MISC[DPModuleIndex], DPEphyMiscRegValue.Value, DPEphyMiscRegMask.Value);

    // delay for at least 1.5ms
    cb_DelayMicroSeconds(1500);

    // AUX_CTRL_Lx[1:0]=2'b10. HW controls operation.
    DPEphyMiscRegValue.Value = 0;
    DPEphyMiscRegValue.AUC_Ch_Op_Mode = 2;
    DPEphyMiscRegMask.Value = 0xFFFFFFFF;
    DPEphyMiscRegMask.AUC_Ch_Op_Mode = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MISC[DPModuleIndex], DPEphyMiscRegValue.Value, DPEphyMiscRegMask.Value);

    // HPD Power Up.
    DPEphyMiscRegValue.Value = 0;
    DPEphyMiscRegValue.HPD_Power_Down = 1;
    DPEphyMiscRegMask.Value = 0xFFFFFFFF;
    DPEphyMiscRegMask.HPD_Power_Down = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MISC[DPModuleIndex], DPEphyMiscRegValue.Value, DPEphyMiscRegMask.Value);

    cbTraceExit(DP);
    return;
}


CBIOS_VOID cbPHY_DP_DeInitEPHY(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8340     DPEphyMpllRegValue, DPEphyMpllRegMask;
    REG_MM8344     DPEphyTxRegValue, DPEphyTxRegMask;
    REG_MM8348     DPEphyMiscRegValue, DPEphyMiscRegMask;

    cbTraceEnter(DP);

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return;
    }

    // Disable Bandgap, MPLL and TPLL
    DPEphyMpllRegValue.Value = 0;
    DPEphyMpllRegValue.Bandgap_Power_Down = 0;
    DPEphyMpllRegValue.MPLL_Reg_Power_Down = 0;
    DPEphyMpllRegValue.MPLL_Power_Down = 0;
    DPEphyMpllRegValue.TPLL_Reg_Power_Down = 0;
    DPEphyMpllRegValue.TPLL_Power_Down = 0;
    DPEphyMpllRegMask.Value = 0xFFFFFFFF;
    DPEphyMpllRegMask.Bandgap_Power_Down = 0;
    DPEphyMpllRegMask.MPLL_Reg_Power_Down = 0;
    DPEphyMpllRegMask.MPLL_Power_Down = 0;
    DPEphyMpllRegMask.TPLL_Reg_Power_Down = 0;
    DPEphyMpllRegMask.TPLL_Power_Down = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);


    // Disable TX Driver
    DPEphyTxRegValue.Value = 0;
    DPEphyTxRegValue.TX_Reg_Power_Down_Lane0 = 0;
    DPEphyTxRegValue.TX_Reg_Power_Down_Lane1 = 0;
    DPEphyTxRegValue.TX_Reg_Power_Down_Lane2 = 0;
    DPEphyTxRegValue.TX_Reg_Power_Down_Lane3 = 0;
    DPEphyTxRegValue.Driver_Control_Lane0 = 0;
    DPEphyTxRegValue.Driver_Control_Lane1 = 0;
    DPEphyTxRegValue.Driver_Control_Lane2 = 0;
    DPEphyTxRegValue.Driver_Control_Lane3 = 0;
    DPEphyTxRegValue.TX_Power_Control_Lane0 = 3;
    DPEphyTxRegValue.TX_Power_Control_Lane1 = 3;
    DPEphyTxRegValue.TX_Power_Control_Lane2 = 3;
    DPEphyTxRegValue.TX_Power_Control_Lane3 = 3;
    DPEphyTxRegMask.Value = 0xFFFFFFFF;
    DPEphyTxRegMask.TX_Reg_Power_Down_Lane0 = 0;
    DPEphyTxRegMask.TX_Reg_Power_Down_Lane1 = 0;
    DPEphyTxRegMask.TX_Reg_Power_Down_Lane2 = 0;
    DPEphyTxRegMask.TX_Reg_Power_Down_Lane3 = 0;
    DPEphyTxRegMask.Driver_Control_Lane0 = 0;
    DPEphyTxRegMask.Driver_Control_Lane1 = 0;
    DPEphyTxRegMask.Driver_Control_Lane2 = 0;
    DPEphyTxRegMask.Driver_Control_Lane3 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane0 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane1 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane2 = 0;
    DPEphyTxRegMask.TX_Power_Control_Lane3 = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_TX[DPModuleIndex], DPEphyTxRegValue.Value, DPEphyTxRegMask.Value);

    // Disable AUX
    DPEphyMiscRegValue.Value = 0;
    DPEphyMiscRegValue.AUC_Ch_Op_Mode = 0;
    DPEphyMiscRegMask.Value = 0xFFFFFFFF;
    DPEphyMiscRegMask.AUC_Ch_Op_Mode = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MISC[DPModuleIndex], DPEphyMiscRegValue.Value, DPEphyMiscRegMask.Value);

    cbTraceExit(DP);
    return;
}


static CBIOS_BOOL cbPHY_DP_SelectTMDSModeSource(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX DPModuleIndex, PCBIOS_DISPLAY_SOURCE pDispSource, CBIOS_MONITOR_TYPE MonitorType)
{
    REG_SR3A_Pair    RegSR3AValue;
    REG_SR3A_Pair    RegSR3AMask;
    CBIOS_BOOL       Ret = CBIOS_FALSE;
    CBIOS_MODULE **pModulePath = pDispSource->ModulePath;

    if (pModulePath == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 3rd param is NULL!\n", FUNCTION_NAME));
        return Ret;
    }

    switch (pModulePath[0]->Type)
    {
    case CBIOS_MODULE_TYPE_HDCP:
        switch (pModulePath[1]->Type)
        {
        case CBIOS_MODULE_TYPE_HDMI:
            if(MonitorType == CBIOS_MONITOR_TYPE_HDMI)
            {
                RegSR3AValue.Value = 0;
                RegSR3AValue.DP_PHY_Source_Sel = 4;
            }
            else if(MonitorType == CBIOS_MONITOR_TYPE_DVI)
            {
                RegSR3AValue.Value = 0;
                RegSR3AValue.DP_PHY_Source_Sel = 4; //PS background overlay will affect DE of DVI timing, force to HDMI mode can fix this issue
            }
            RegSR3AMask.Value = 0xFF;
            RegSR3AMask.DP_PHY_Source_Sel = 0;
            cbBiosMMIOWriteReg(pcbe, SR_3A, RegSR3AValue.Value, RegSR3AMask.Value, DPModuleIndex);

            Ret = CBIOS_TRUE;
            break;
        case CBIOS_MODULE_TYPE_IGA://should set DP_PHY_Source_Sel to 0,otherwise, like DVI can't light.
            if (pModulePath[1]->Index != CBIOS_MODULE_INDEX_INVALID)
            {
                RegSR3AValue.Value = 0;
                RegSR3AValue.DP_PHY_Source_Sel = 0;
                RegSR3AMask.Value = 0xFF;
                RegSR3AMask.DP_PHY_Source_Sel = 0;
                cbBiosMMIOWriteReg(pcbe, SR_3A, RegSR3AValue.Value, RegSR3AMask.Value, DPModuleIndex);

                Ret = CBIOS_TRUE;
            }
            break;
         default:
            break;
        }
        break;
    default:
        break;
    }

    if (!Ret)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: cannot select DP TMDS mode source!!!\n", FUNCTION_NAME));
    }
    return Ret;
}

static CBIOS_VOID cbPHY_DP_SelectDPModeSource(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX DPModuleIndex, PCBIOS_DISPLAY_SOURCE pDispSource)
{
    CBIOS_U32     i = 0;
    CBIOS_MODULE *pHDTVModule = CBIOS_NULL;
    CBIOS_MODULE *pIGAModule  = CBIOS_NULL;
    CBIOS_MODULE **pModulePath = pDispSource->ModulePath;

    if (pModulePath == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 2nd param is NULL!\n", FUNCTION_NAME));
        return;
    }

    while (pModulePath[i])
    {
        if (pModulePath[i]->Type == CBIOS_MODULE_TYPE_HDTV)
        {
            pHDTVModule = pModulePath[i];
        }
        else if (pModulePath[i]->Type == CBIOS_MODULE_TYPE_IGA)
        {
            pIGAModule = pModulePath[i];
            break;
        }
        i++;
    }

    // HDTV1 is hardcoded to DP1 by HW design
    if (pHDTVModule && pIGAModule)
    {
        cbDIU_HDTV_LBBypass(pcbe, DPModuleIndex, CBIOS_FALSE);
    }
    else if (pIGAModule)
    {
        cbDIU_HDTV_LBBypass(pcbe, DPModuleIndex, CBIOS_TRUE);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: cannot select DP mode source!\n", FUNCTION_NAME));
    }
}

CBIOS_VOID cbPHY_DP_SelectPhySource(PCBIOS_VOID pvcbe, DP_EPHY_MODE DPEphyMode, PCBIOS_DISPLAY_SOURCE pDispSource, CBIOS_MONITOR_TYPE MonitorType)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_MODULE_INDEX DPModuleIndex = CBIOS_MODULE_INDEX_INVALID;

    if (pDispSource == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: the 2nd param is NULL!\n", FUNCTION_NAME));
        return;
    }

    DPModuleIndex = pDispSource->ModuleList.DPModule.Index;

    cbPHY_DP_SelectEphyMode(pcbe, DPModuleIndex, DPEphyMode);

    if (DPEphyMode == DP_EPHY_TMDS_MODE)
    {
        cbPHY_DP_SelectTMDSModeSource(pcbe, DPModuleIndex, pDispSource, MonitorType);
    }
    else if (DPEphyMode == DP_EPHY_DP_MODE)
    {
        cbPHY_DP_SelectDPModeSource(pcbe, DPModuleIndex, pDispSource);
    }
    else
    {
        ASSERT(CBIOS_FALSE);
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: unsupported ephy mode!\n", FUNCTION_NAME));
    }
}

CBIOS_VOID cbPHY_DP_AuxPowerOn(PCBIOS_VOID pvcbe, CBIOS_MODULE_INDEX DPModuleIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    REG_MM8340    DPEphyMpllRegValue, DPEphyMpllRegMask;
    REG_MM8348    DPEphyMiscRegValue, DPEphyMiscRegMask;

    if (DPModuleIndex >= DP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(DP, ERROR), "%s: invalid DP module index!\n", FUNCTION_NAME));
        return;
    }

    // 1. enable Bandgap
    DPEphyMpllRegValue.Value = 0;
    DPEphyMpllRegValue.Bandgap_Power_Down = 1;
    DPEphyMpllRegMask.Value = 0xFFFFFFFF;
    DPEphyMpllRegMask.Bandgap_Power_Down = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MPLL[DPModuleIndex], DPEphyMpllRegValue.Value, DPEphyMpllRegMask.Value);

    cb_DelayMicroSeconds(20);

    // 2. start AUX channel, CMOP on, Tx/Rx off
    DPEphyMiscRegValue.Value = 0;
    DPEphyMiscRegValue.AUC_Ch_Op_Mode = 1;
    DPEphyMiscRegMask.Value = 0xFFFFFFFF;
    DPEphyMiscRegMask.AUC_Ch_Op_Mode = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MISC[DPModuleIndex], DPEphyMiscRegValue.Value, DPEphyMiscRegMask.Value);

    cbDelayMilliSeconds(2);

    DPEphyMiscRegValue.Value = 0;
    DPEphyMiscRegValue.AUC_Ch_Op_Mode = 2;
    DPEphyMiscRegMask.Value = 0xFFFFFFFF;
    DPEphyMiscRegMask.AUC_Ch_Op_Mode = 0;
    cbMMIOWriteReg32(pcbe, DP_REG_EPHY_MISC[DPModuleIndex], DPEphyMiscRegValue.Value, DPEphyMiscRegMask.Value);
}

