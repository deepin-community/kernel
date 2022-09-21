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
** IGA block interface function implementation. 
** Mainly includes set timing, set dclk, set/get active devices.
**
** NOTE:
** The functions in this file are hw layer internal functions, 
** CAN ONLY be called by files under Hw folder. 
******************************************************************************/

#include "CBiosChipShare.h"
#include "../CBiosHwShare.h"
#include "CBiosIGA_Timing.h"
#include "CBiosScaler.h"

extern  CBIOS_U32  OvlKeyIndex[CBIOS_IGACOUNTS][OVL_NUM_E3K];

static CBREGISTER ZXGInitBaseRegs_NonPaired[] =
{
    {SR,(CBIOS_U8)~0xDF, 0x01, 0x01},
    {SR, 0x00, 0x02, 0x0F},
    {SR, 0x00, 0x03, 0x00},
    {SR, 0x00, 0x04, 0x0E},
    {AR, 0x00, 0x00, 0x00},
    {AR, 0x00, 0x01, 0x01},
    {AR, 0x00, 0x02, 0x02},
    {AR, 0x00, 0x03, 0x03},
    {AR, 0x00, 0x04, 0x04},
    {AR, 0x00, 0x05, 0x05},
    {AR, 0x00, 0x06, 0x06},
    {AR, 0x00, 0x07, 0x07},
    {AR, 0x00, 0x08, 0x08},
    {AR, 0x00, 0x09, 0x09},
    {AR, 0x00, 0x0A, 0x0A},
    {AR, 0x00, 0x0B, 0x0B},
    {AR, 0x00, 0x0C, 0x0C},
    {AR, 0x00, 0x0D, 0x0D},
    {AR, 0x00, 0x0E, 0x0E},
    {AR, 0x00, 0x0F, 0x0F},
    {AR, 0x00, 0x10, 0x41},
    {AR, 0x00, 0x11, 0x00},
    {AR, 0x00, 0x12, 0x0F},
    {AR, 0x00, 0x13, 0x00},
    {GR, 0x00, 0x00, 0x00},
    {GR, 0x00, 0x01, 0x00},
    {GR, 0x00, 0x02, 0x00},
    {GR, 0x00, 0x03, 0x00},
    {GR, 0x00, 0x04, 0x00},
    {GR, 0x00, 0x05, 0x00},
    {GR, 0x00, 0x06, 0x05},
    {GR, 0x00, 0x07, 0x0F},
    {GR, 0x00, 0x08, 0xFF},
    {CR, 0x00, 0x0A, 0x00},
    {CR, 0x00, 0x0B, 0x00},
    {CR, 0x00, 0x0E, 0xFF},
    {CR, 0x00, 0x0F, 0x00},
    {CR, 0x00, 0x14, 0x60},
    {CR, 0x00, 0x18, 0xFF},
};

static CBREGISTER ZXGInitBaseRegs_Paired[]=
{
    {CR, 0x00, 0x08, 0x00},
    {CR, 0x00, 0x09, 0x40},
    {CR, 0x00, 0x0C, 0x00},
    {CR, 0x00, 0x0D, 0x00},
    {CR, 0x00, 0x17, 0x80},
};

static CBREGISTER ZXGInitExtBaseRegs[] =
{
    {CR, 0x00, 0x33, 0x08},
    {CR, 0x00, 0x3B, 0x58},
    {CR, 0x00, 0x5E, 0xC0},	
    {CR,(CBIOS_U8)~0x0F, 0x64, 0x00},
    {CR, 0x00, 0x69, 0x00},
};

static CBREGISTER ZXGVesaNonPlanarReg_Paired[] =
{
    {CR,(CBIOS_U8)~0x7F, 0x31, 0x09},
    {CR,(CBIOS_U8)~0x44, 0x32, 0x00},
    {CR,(CBIOS_U8)~0x30, 0x3A, 0x10},
    {CR,(CBIOS_U8)~0x01, 0x66, 0x01},
};

#if 0

static CBIOS_VOID cbSet3DVideoMode(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_3D_STRUCTURE Video3DStruct, CBIOS_U32 IGAIndex)
{
    REG_CR44_Pair    RegCR44Value;
    REG_CR44_Pair    RegCR44Mask;
    switch (Video3DStruct)
    {
    case TOP_AND_BOTTOM:
        RegCR44Value.Value = 0;
        RegCR44Value._3D_Video_Mode = 4;
        RegCR44Mask.Value = 0xF8;
        cbBiosMMIOWriteReg(pcbe, CR_44, RegCR44Value.Value, RegCR44Mask.Value, IGAIndex);
        break;
    case FRAME_PACKING:
    case L_DEPTH:
        RegCR44Value.Value = 0;
        RegCR44Value._3D_Video_Mode = 5;
        RegCR44Mask.Value = 0xF8;
        cbBiosMMIOWriteReg(pcbe, CR_44, RegCR44Value.Value, RegCR44Mask.Value, IGAIndex);
        break;
    case LINE_ALTERNATIVE:
        RegCR44Value.Value = 0;
        RegCR44Value._3D_Video_Mode = 6;
        RegCR44Mask.Value = 0xF8;
        cbBiosMMIOWriteReg(pcbe, CR_44, RegCR44Value.Value, RegCR44Mask.Value, IGAIndex);
        break;
    case SIDE_BY_SIDE_FULL:
    case SIDE_BY_SIDE_HALF:
        RegCR44Value.Value = 0;
        RegCR44Value._3D_Video_Mode = 7;
        RegCR44Mask.Value = 0xF8;
        cbBiosMMIOWriteReg(pcbe, CR_44, RegCR44Value.Value, RegCR44Mask.Value, IGAIndex);
        break;
    default:
        RegCR44Value.Value = 0;
        RegCR44Value._3D_Video_Mode = 0;
        RegCR44Mask.Value = 0xF8;
        cbBiosMMIOWriteReg(pcbe, CR_44, RegCR44Value.Value, RegCR44Mask.Value, IGAIndex);
        break;
    }
}

#endif

static CBIOS_VOID cbSetPixelRepetition(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U8 PixelRepitition, CBIOS_U32 IGAIndex)
{
    REG_CRF1 RegCRF1Value, RegCRF1Mask;

    //check pixel repetition
    if (PixelRepitition == 1)
    {
        if (IGAIndex == IGA1)
        {
            RegCRF1Value.Value = 0;
            RegCRF1Value.IGA1_Pixel_Repetition = 0;
            RegCRF1Mask.Value = 0xFF;
            RegCRF1Mask.IGA1_Pixel_Repetition = 0;
            cbMMIOWriteReg(pcbe, CR_F1, RegCRF1Value.Value, RegCRF1Mask.Value);
        }
        else
        {
            RegCRF1Value.Value = 0;
            RegCRF1Value.IGA2_Pixel_Repetition = 0;
            RegCRF1Mask.Value = 0xFF;
            RegCRF1Mask.IGA2_Pixel_Repetition = 0;
            cbMMIOWriteReg(pcbe, CR_F1, RegCRF1Value.Value, RegCRF1Mask.Value);
        }
    }
    else if (PixelRepitition == 2)
    {
        if (IGAIndex == IGA1)
        {
            RegCRF1Value.Value = 0;
            RegCRF1Value.IGA1_Pixel_Repetition = 1;
            RegCRF1Mask.Value = 0xFF;
            RegCRF1Mask.IGA1_Pixel_Repetition = 0;
            cbMMIOWriteReg(pcbe, CR_F1, RegCRF1Value.Value, RegCRF1Mask.Value);
        }
        else
        {
            RegCRF1Value.Value = 0;
            RegCRF1Value.IGA2_Pixel_Repetition = 1;
            RegCRF1Mask.Value = 0xFF;
            RegCRF1Mask.IGA2_Pixel_Repetition = 0;
            cbMMIOWriteReg(pcbe, CR_F1, RegCRF1Value.Value, RegCRF1Mask.Value);
        }
    }
    else if (PixelRepitition == 4)
    {
       if (IGAIndex == IGA1)
        {
            RegCRF1Value.Value = 0;
            RegCRF1Value.IGA1_Pixel_Repetition = 2;
            RegCRF1Mask.Value = 0xFF;
            RegCRF1Mask.IGA1_Pixel_Repetition = 0;
            cbMMIOWriteReg(pcbe, CR_F1, RegCRF1Value.Value, RegCRF1Mask.Value);
        }
        else
        {
            RegCRF1Value.Value = 0;
            RegCRF1Value.IGA2_Pixel_Repetition = 2;
            RegCRF1Mask.Value = 0xFF;
            RegCRF1Mask.IGA2_Pixel_Repetition = 0;
            cbMMIOWriteReg(pcbe, CR_F1, RegCRF1Value.Value, RegCRF1Mask.Value);
        }
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: wrong pixel repetition\n", FUNCTION_NAME));
    }
}

static CBIOS_VOID cbDstTimingRegSetting(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DISP_MODE_PARAMS pModeParams, CBIOS_U32 IGAIndex)
{
    PCBIOS_TIMING_ATTRIB pTiming = &(pModeParams->TargetTiming);
    CBIOS_TIMING_FLAGS TimingFlags = {0};

    cbTraceEnter(GENERIC);

    if (pModeParams->TargetModePara.bInterlace)
    {
        TimingFlags.IsInterlace = 1;
    }

    cbSetSRTimingReg(pcbe, pTiming, IGAIndex, TimingFlags);

    cbProgramDclk(pcbe, (CBIOS_U8)IGAIndex, pTiming->PLLClock);

    //reset h/v counter to avoid wait vblank timeout
    cbBiosMMIOWriteReg(pcbe, CR_23, 0, 0, IGAIndex);
    cbBiosMMIOWriteReg(pcbe, CR_29, 0, 0, IGAIndex);
    cbBiosMMIOWriteReg(pcbe, CR_26, 0, 0, IGAIndex);

    cbMMIOWriteReg(pcbe, CR_B_F5, 0x00, 0xDF); //disable SSC

    cbSetPixelRepetition(pcbe, pModeParams->PixelRepitition, IGAIndex);

    cbTraceExit(GENERIC);
}

static CBIOS_VOID cbDstTimingInitial(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U8 IGAIndex)
{
    if(IGAIndex == IGA1)
    {
        //unlock sr01_0 value.
        cbMMIOWriteReg(pcbe, CR_34, 0x00, 0xDF); 

        cbMMIOWriteReg(pcbe,SR_01,0x01,0xFE); 
        //Lock sr01_0 value to avoid being modified by other apps.
        cbMMIOWriteReg(pcbe, CR_34, 0x20, 0xDF);
    }

    //SRAB(character clock) will affect IGA timing, clear it here
    if(IGAIndex == IGA1)
    {
        cbBiosMMIOWriteReg(pcbe, SR_AB, 0x00, ~0x7, IGA1);
        cbBiosMMIOWriteReg(pcbe, SR_AB, 0x00, ~0x7, IGA2);
    }
    else if(IGAIndex == IGA2)
    {
        cbBiosMMIOWriteReg(pcbe, SR_AB, 0x00, ~0x38, IGA1);
        cbBiosMMIOWriteReg(pcbe, SR_AB, 0x00, ~0x38, IGA2);
    }
}

static CBIOS_VOID cbPlanarset(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U8 IGAIndex)
{
    cbTraceEnter(GENERIC);

    // enhanced mode
    cbLoadtable(pcbe,
                ZXGVesaNonPlanarReg_Paired,
                sizeofarray(ZXGVesaNonPlanarReg_Paired),
                IGAIndex);
    
    if(IGAIndex == IGA2)
    {
        cbMMIOWriteReg(pcbe,SR_30, 0x00, 0xFE);
    }
    else if(IGAIndex == IGA3)
    {
        cbMMIOWriteReg(pcbe,SR_30, 0x00, 0xDF);
    }
    else if(IGAIndex == IGA4)
    {
        //need add IGA4 code later
    }

    cbTraceExit(GENERIC);
}


static CBIOS_VOID cbSetBPSL(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U8 IGAIndex)
{
    if(IGAIndex == IGA1)
    {
        // PS1 timeout
        cbMMIOWriteReg(pcbe,CR_88, 0x20, 0x00);
    }    
    else if(IGAIndex == IGA2)
    {
        // PS2 timeout
        cbMMIOWriteReg(pcbe,CR_BC, 0x24, 0x00);      
    }
    else if(IGAIndex == IGA3)
    {
        // PS3 timeout
        cbMMIOWriteReg(pcbe,CR_93, 0x20, 0x00);      
    }
    else
    {
        //need add IGA4 code later
    }
}

static CBIOS_VOID cbModeEnvSetup(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U8 IGAIndex)
{
    cbTraceEnter(GENERIC);
    // 1. Load Init Base Table
    cbLoadtable(pcbe, ZXGInitBaseRegs_NonPaired, sizeofarray(ZXGInitBaseRegs_NonPaired), CBIOS_NOIGAENCODERINDEX);
    // 2. init primary address
    cbLoadtable(pcbe, ZXGInitBaseRegs_Paired, sizeofarray(ZXGInitBaseRegs_Paired), IGAIndex);

    // 3. timing trigger
    cbBiosMMIOWriteReg(pcbe, CR_17, 0x80, 0x00, IGAIndex);
    // 4. Load Init Ext Base Table
    cbLoadtable(pcbe, ZXGInitExtBaseRegs, sizeofarray(ZXGInitExtBaseRegs), IGAIndex);
    cbTraceExit(GENERIC);
}

#if 0

static  CBIOS_VOID cbSetDacRegisters(PCBIOS_EXTENSION_COMMON pcbe)
{
    CBIOS_U8    byDacIndex;

    if (cb_ReadU8(pcbe->pAdapterContext, 0x83C6)!=0xFF)
    {
        cb_WriteU8(pcbe->pAdapterContext, 0x83C6,0xFF);
    }

    for(byDacIndex=0x0;byDacIndex<=0xF7;byDacIndex++)
    {
        cb_WriteU8(pcbe->pAdapterContext, 0x83C8, byDacIndex);      
        cb_WriteU8(pcbe->pAdapterContext, 0x83C9,  0x80);                    
        cb_WriteU8(pcbe->pAdapterContext, 0x83C9, 0x80);               
        cb_WriteU8(pcbe->pAdapterContext, 0x83C9, 0x80);       
    }

    //patch for mmio fault BEGIN--added by Ramonwang
    cb_ReadU8(pcbe->pAdapterContext, CB_SEQ_ADDR_REG);
    cb_ReadU8(pcbe->pAdapterContext, CB_SEQ_DATA_REG);
    //patch for mmio END
}

#endif

static CBIOS_VOID cbSetSrcTiming(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DISP_MODE_PARAMS pModeParams, CBIOS_U32 IGAIndex)
{
    CBIOS_U32 ulDevices = pcbe->DispMgr.ActiveDevices[IGAIndex];
    CBiosDestModeParams ModeParams = {0};
    CBIOS_TIMING_ATTRIB Timing = {0};
    CBIOS_TIMING_FLAGS  TimingFlags = {0};
    REG_SR47 SR47Value = {0}, SR47Mask = {0xff};
    REG_MM33718_Arise CursorCtrl1RegValue = {0};
    REG_MM33720_Arise CursorBaseAddrRegValue = {0};
    CBIOS_U32 CursorControl1Index[CBIOS_IGACOUNTS] = {0x33718, 0x33bb4, 0x342b4, 0x349b4};
    CBIOS_U32 CursorBaseAddrIndex[CBIOS_IGACOUNTS] = {0x33720, 0x33bbc, 0x342bc, 0x349bc};

    cbTraceEnter(GENERIC);
    
    // if more than one devices assign on one IGA, 
    // do simultaneous view and set timing of primary device
    ulDevices = cbDevGetPrimaryDevice((CBIOS_U32)pcbe->DispMgr.ActiveDevices[IGAIndex]);
    
    // disable gamma
    SR47Value.SP_Gamma_Enable = 0;
    SR47Mask.SP_Gamma_Enable = 0;
    SR47Value.SP_LUT_Interpolation_Enable = 0;
    SR47Mask.SP_LUT_Interpolation_Enable = 0;
    cbBiosMMIOWriteReg(pcbe, SR_47, SR47Value.Value, SR47Mask.Value, IGAIndex);

    // disable HW cursor
    CursorCtrl1RegValue.Cursor_1_Enable = 0;
    CursorBaseAddrRegValue.Cursor_1_Enable_Work_Registers = 1;
    cbMMIOWriteReg32(pcbe, CursorControl1Index[IGAIndex], CursorCtrl1RegValue.Value, 0);
    cbMMIOWriteReg32(pcbe, CursorBaseAddrIndex[IGAIndex], CursorBaseAddrRegValue.Value, 0);

    cbModeEnvSetup(pcbe, (CBIOS_U8)IGAIndex);

    ModeParams.AspectRatioFlag = 0;
    ModeParams.XRes = pModeParams->SrcModePara.XRes;
    ModeParams.YRes = pModeParams->SrcModePara.YRes;
    ModeParams.RefreshRate = pModeParams->TargetModePara.RefRate;
    ModeParams.InterlaceFlag = CBIOS_FALSE;

    cbMode_GetHVTiming(pcbe,
                       ModeParams.XRes,
                       ModeParams.YRes,
                       ModeParams.RefreshRate,
                       ModeParams.InterlaceFlag,
                       ulDevices,
                       &Timing);

    cbSetCRTimingReg(pcbe, &Timing, IGAIndex, TimingFlags);                             

    //Planar Set
    cbPlanarset(pcbe, (CBIOS_U8)IGAIndex);    

    //Set FIFO and Offset register value
    cbSetBPSL(pcbe, (CBIOS_U8)IGAIndex);

    cbTraceExit(GENERIC);
}

static CBIOS_VOID cbSetDstTiming(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DISP_MODE_PARAMS pModeParams, CBIOS_U32 IGAIndex)
{
    cbTraceEnter(GENERIC);
    
    cbDstTimingInitial(pcbe, (CBIOS_U8)IGAIndex);
    
    cbDstTimingRegSetting(pcbe, pModeParams, IGAIndex);

    cbTraceExit(GENERIC);
}


CBIOS_VOID cbProgramDclk(PCBIOS_VOID pvcbe, CBIOS_U8 IGAIndex, CBIOS_U32 ClockFreq)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U8         ClockType = CBIOS_DCLK1TYPE;
    CBIOS_U32        CurrentFreq;
    REG_SR1B         RegSR1BValue;
    REG_SR1B         RegSR1BMask;
    REG_SR30_Pair    RegSR30Value;
    REG_SR30_Pair    RegSR30Mask;

    // Program DClk
    if(IGAIndex == IGA1)
    {
        ClockType = CBIOS_DCLK1TYPE;
    }
    else if(IGAIndex == IGA2)    
    {
        ClockType = CBIOS_DCLK2TYPE;
    }
    else if(IGAIndex == IGA3)    
    {
        ClockType = CBIOS_DCLK3TYPE;
    }
    else if(IGAIndex == IGA4)
    {
        ClockType = CBIOS_DCLK4TYPE;
    }

    // Only IGA1 contains VGA logic, so there are two Dclk sources types for IGA1;
    // SR1B[7] for selecting IGA1 source;
    // Now do not skip program dclk temporarily since for down scale and deep color case,
    // the values get from cbGetProgClock may not correct
    // Need refine

    cbGetProgClock(pcbe, &CurrentFreq, ClockType);

    //force IGA1 Dclk source from SR12/13
    if(IGAIndex == IGA1)
    {
        RegSR1BValue.Value = 0;
        RegSR1BValue.Controller1_DCLK_Parm_Source = 1;
        RegSR1BMask.Value = 0xFF;
        RegSR1BMask.Controller1_DCLK_Parm_Source = 0;
        cbMMIOWriteReg(pcbe,SR_1B, RegSR1BValue.Value, RegSR1BMask.Value);
    }

    //Confirm that controller1 use dclk1, controller2 use dclk2, controller3 use dclk3
    //controller4 is cre3[7], its default value is 0.
    RegSR30Value.Value = 0;
    RegSR30Value.Controller1_DCLK = 0;
    RegSR30Value.Controller2_DCLK = 0;
    RegSR30Value.Controller3_DCLK = 0;
    RegSR30Mask.Value = 0xA7;
    cbMMIOWriteReg(pcbe,SR_30, RegSR30Value.Value, RegSR30Mask.Value); 

    cbProgClock(pcbe, ClockFreq, ClockType, IGAIndex);
}


CBIOS_VOID cbIGA_HW_SetMode(PCBIOS_VOID pvcbe, PCBIOS_DISP_MODE_PARAMS pModeParams)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32 IGAIndex = pModeParams->IGAIndex;
    REG_CR42_Pair RegCR42Value, RegCR42Mask;
    REG_SR31_Pair RegSR31Value, RegSR31Mask;

    RegSR31Value.Value = 0;
    RegSR31Value.Controller1_is_Flat_Panel_Source = 1;
    RegSR31Mask.Value = 0xFF;
    RegSR31Mask.Controller1_is_Flat_Panel_Source = 0;
    cbBiosMMIOWriteReg(pcbe, SR_31, RegSR31Value.Value, RegSR31Mask.Value, IGAIndex);

    // source timing
    cbSetSrcTiming(pcbe, pModeParams, IGAIndex);

    // dst timing
    cbSetDstTiming(pcbe, pModeParams, IGAIndex); 

    cbSetScaler(pcbe, pModeParams);

    //Bandwidth issue: The point enable/disable Reduced_Blanking during disable stream to avoid FIFO underflow
    RegCR42Value.Value = 0;
    RegCR42Value.Reduced_Blanking_Enable = 1;
    RegCR42Mask.Value = 0xFF;
    RegCR42Mask.Reduced_Blanking_Enable = 0;
    cbBiosMMIOWriteReg(pcbe, CR_42, RegCR42Value.Value, RegCR42Mask.Value, IGAIndex);

    //one shot enable
    if (IGA1 == IGAIndex)
    {
        cbMMIOWriteReg32(pcbe, 0x840c, 0xfff, 0);   //ps, ss, ts ,qs, cursor, write back and overlay
    }
    else if (IGA2 == IGAIndex)
    {
        cbMMIOWriteReg32(pcbe, 0x33a2c, 0xfff, 0);
    }
    else if (IGA3 == IGAIndex)
    {
        cbMMIOWriteReg32(pcbe, 0x3412c, 0xfff, 0);
    }
    else
    {
        cbMMIOWriteReg32(pcbe, 0x3482c, 0xfff, 0);
    }

    {
        //set default value kp = 8, ks = 0, to 4 OVLs, temp add it at bring up
        CBIOS_U32   OvlIndex = CBIOS_OVERLAY0;
        REG_MM33840_Arise  OvlKeyValue = {0};
        OvlKeyValue.Ka_3to0_Or_Ks = 0;
        OvlKeyValue.Ka_7to4_Or_Kp = 8;
        OvlKeyValue.Key_Mode = 0; // B over A window key
        OvlKeyValue.Ovl0_Enable_Work = 1;
        for(; OvlIndex < OVL_NUM_E3K; OvlIndex++)
        {
            OvlKeyValue.Ovl0_Input_Stream = OvlIndex;
            cbMMIOWriteReg32(pcbe, OvlKeyIndex[IGAIndex][OvlIndex], OvlKeyValue.Value, 0);
        }
    }
}

// Update the CR6B scratch pad Regiseter for compatible with VBIOS.
CBIOS_VOID cbIGA_UpdateActiveDeviceToReg(PCBIOS_VOID pvcbe, PCBIOS_DISPLAY_MANAGER pDispMgr)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32 Devices, i;

    cbTraceEnter(GENERIC);

    Devices = CBIOS_TYPE_NONE;
    for (i = IGA1; i < CBIOS_IGACOUNTS; i++)
    {
        Devices |= pDispMgr->ActiveDevices[i];
    }
    
    if(MORE_THAN_1BIT(Devices))
    {
        Devices |= CBIOS_TYPE_DUOVIEW;
    }

    Devices = cbConvertCBiosDevBit2VBiosDevBit(Devices);
    
    cbMMIOWriteReg(pcbe, CR_6B, (CBIOS_U8)Devices, 0x00);

    Devices = Devices >> 8;
    cbMMIOWriteReg(pcbe, CR_6C, (CBIOS_U8)Devices, 0x00);
    
    cbTraceExit(GENERIC);
}


CBIOS_U32 cbIGA_GetActiveDeviceFromReg(PCBIOS_VOID pvcbe)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32 wTemp = 0;
    wTemp = (CBIOS_U32) cbMMIOReadReg(pcbe,CR_6C);
    wTemp = wTemp << 8;
    wTemp |= (CBIOS_U32) cbMMIOReadReg(pcbe,CR_6B);

    wTemp = cbConvertVBiosDevBit2CBiosDevBit(wTemp);
    wTemp &= pcbe->DeviceMgr.SupportDevices;
    return wTemp;
}

