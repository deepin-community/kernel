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
** R63417 type DSI panel descriptor definition, 
** which contains panel config and command lists.
**
** NOTE:
**
******************************************************************************/


#include "CBiosDSIPanel.h"
#include "CBiosDSI.h"

static CBIOS_U8 PasswordBuf[0x2] = 
{
    0xB0, 0x00
};

static CBIOS_U8 RemoveNVMBuf[0x2] = 
{
    0xD6, 0x01
};

static CBIOS_U8 CommonSetBuf[0x18] = 
{
    0xCE, 0x6C, 0x40, 0x43,
    0x49, 0x55, 0x62, 0x71,
    0x82, 0x94, 0xA8, 0xB9,
    0xCB, 0xDB, 0xE9, 0xF5,
    0xFC, 0xFF, 0x04, 0xD3, 
    0x00, 0x00, 0x54, 0x24
};

static CBIOS_U8 CABCStillBuf[0x07] = 
{
    0xB9, 0x03, 0x82, 0x3C, 
    0x10, 0x3C, 0x87
};

static CBIOS_U8 CABCMovieBuf[0x07] = 
{
    0xBA, 0x03, 0x78, 0x64, 
    0x10, 0x64, 0xB4
};

static CBIOS_U8 SREManual0Buf[0x04] = 
{
    0xBB, 0x01, 0x00, 0x00
};

static CBIOS_U8 M7ColorEnhanceBuf[0x21] = 
{
    0xCA, 0x01, 0x02, 0x9A,
    0xA4, 0xB8, 0xB4, 0xB0,
    0xA4, 0x08, 0x28, 0x05,
    0x87, 0xB0, 0x50, 0x01,
    0xFF, 0x05, 0xF8, 0x0C,
    0x0C, 0x50, 0x40, 0x13,
    0x13, 0xF0, 0x08, 0x10,
    0x10, 0x3F, 0x3F, 0x3F,
    0x3F
};

static CBIOS_U8 BrightnessCtrlBuf[0x2] = 
{
    0x55, 0x42
};

static CBIOS_U8 PWMBuf[0x2] = 
{
    0x53, 0x0C
};

static CBIOS_U8 BlueShiftBuf1[0x1F] = 
{
    0xC7, 0x01, 0x0B, 0x12,
    0x1B, 0x2A, 0x3A, 0x45,
    0x56, 0x3A, 0x42, 0x4E,
    0x5B, 0x64, 0x6C, 0x75,
    0x01, 0x0B, 0x12, 0x1A,
    0x29, 0x37, 0x41, 0x52,
    0x36, 0x3F, 0x4C, 0x59,
    0x62, 0x6A, 0x74
};

static CBIOS_U8 BlueShiftBuf2[0x14] = 
{
    0xC8, 0x01, 0x00, 0xF4,
    0x00, 0x00, 0xFC, 0x00,
    0x00, 0xF7, 0x00, 0x00,
    0xFC, 0x00, 0x00, 0xFF,
    0x00, 0x00, 0xFC, 0x0F
};

static CBIOS_U8 SourceTimingBuf[0x17]= 
{
    0xC4, 0x70, 0x0C, 0x0C,
    0x55, 0x55, 0x00, 0x00,
    0x00, 0x00, 0x05, 0x05,
    0x00, 0x0C, 0x0C, 0x55,
    0x55, 0x00, 0x00, 0x00,
    0x00, 0x05, 0x05
};

static CBIOS_U8 LockBuf[0x02] = 
{
    0xB0, 0x03
};

static CBIOS_U8 NopBuf[0x02] = 
{
    0x00, 0x00
};

static CBIOS_U8 EnterSleepBuf[0x01] = 
{
    0x10
};

static CBIOS_U8 ExitSleepBuf[0x01] = 
{
    0x11
};

static CBIOS_U8 DisplayOffBuf[0x01] = 
{
    0x28
};

static CBIOS_U8 DisplayOnBuf[0x01] = 
{
    0x29
};

static CBIOS_U8 BacklightBuf[0x02] = 
{
    0x51, 0xFF
};


static CBIOS_DSI_CMD_DESC R63417_PowerOn_CmdList[] = 
{
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_GEN, 0,   sizeof(PasswordBuf),       PasswordBuf,       {1}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_GEN, 0,   sizeof(RemoveNVMBuf),      RemoveNVMBuf,      {1}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_GEN, 0,   sizeof(CommonSetBuf),      CommonSetBuf,      {1}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_GEN, 0,   sizeof(CABCStillBuf),      CABCStillBuf,      {1}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_GEN, 0,   sizeof(CABCMovieBuf),      CABCMovieBuf,      {1}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_GEN, 0,   sizeof(SREManual0Buf),     SREManual0Buf,     {1}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_GEN, 0,   sizeof(M7ColorEnhanceBuf), M7ColorEnhanceBuf, {1}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_DCS, 0,   sizeof(BrightnessCtrlBuf), BrightnessCtrlBuf, {1}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_DCS, 0,   sizeof(PWMBuf),            PWMBuf,            {1}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_GEN, 0,   sizeof(BlueShiftBuf1),     BlueShiftBuf1,     {1}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_GEN, 0,   sizeof(BlueShiftBuf2),     BlueShiftBuf2,     {1}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_GEN, 0,   sizeof(SourceTimingBuf),   SourceTimingBuf,   {1}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_GEN, 0,   sizeof(LockBuf),           LockBuf,           {1}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_DCS, 0,   sizeof(NopBuf),            NopBuf,            {1}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_DCS, 0,   sizeof(NopBuf),            NopBuf,            {1}},
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 120, sizeof(ExitSleepBuf),      ExitSleepBuf,      {1}},
};

static CBIOS_DSI_CMD_DESC R63417_PowerOff_CmdList[] = 
{
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 0,   sizeof(DisplayOffBuf), DisplayOffBuf, {1}},
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 120, sizeof(EnterSleepBuf), EnterSleepBuf, {1}},
};

static CBIOS_DSI_CMD_DESC R63417_DisplayOn_CmdList[] = 
{
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 10, sizeof(DisplayOnBuf), DisplayOnBuf,   {1}},
};

static CBIOS_DSI_CMD_DESC R63417_DisplayOff_CmdList[] = 
{
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 10, sizeof(DisplayOffBuf), DisplayOffBuf, {1}},
};

static CBIOS_DSI_CMD_DESC R63417_Backlight_CmdList[] = 
{
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(BacklightBuf), BacklightBuf, {1}},
};

CBIOS_DSI_PANEL_DESC R63417_Panel_Desc = 
{
    /*.VersionNum = */CBIOS_DSI_VERSION,
    /*.DSIConfig = */
    {
        /*.DSIMode = */CBIOS_DSI_CMDMODE,
        /*.ClkLaneMode = */CBIOS_DSI_CLK_LANE_HARDWARE_CTL,
        /*.SyncPacketType = */CBIOS_DSI_SYNC_PULSE,
        /*.TEType = */CBIOS_DSI_TE_TRIGGER,
        /*.TurnAroundTimeout = */0x1000000,
        /*.HS_TXTimeout = */0x1000000,
        /*.LP_RXTimeout = */0x1000000,
        /*.DMAThreshold = */0x400,
        /*.BacklightMax = */255,
        /*.BacklightMin = */0,
        /*.ConfigFlags = */{0x10,},
    },
    /*.DSIPanelTbl = */
    {
        /*.LaneNum = */4,
        /*.OutBpp = */24,
        /*.PanelTiming = */
        {
            /*.XResolution = */1080,
            /*.YResolution = */1920,
            /*.DCLK = */14040,
            /*.HorTotal = */1200,
            /*.HorDisEnd = */1080,
            /*.HorBStart = */1080,
            /*.HorBEnd = */1200,
            /*.HorSyncStart = */(1080+64),
            /*.HorSyncEnd = */(1080+64+8),
            /*.VerTotal = */1950,
            /*.VerDisEnd = */1920,
            /*.VerBStart = */1920,
            /*.VerBEnd = */1950,
            /*.VerSyncStart = */(1920+16),
            /*.VerSyncEnd = */(1920+16+2),
            /*.HVPolarity = */DSI_VNEGATIVE + DSI_HNEGATIVE,
        },
    },
    /*.PowerOnCmdListSize = */sizeofarray(R63417_PowerOn_CmdList),
    /*.PowerOffCmdListSize = */sizeofarray(R63417_PowerOff_CmdList),
    /*.DisplayOnCmdListSize = */sizeofarray(R63417_DisplayOn_CmdList),
    /*.DisplayOffCmdListSize = */sizeofarray(R63417_DisplayOff_CmdList),
    /*.BacklightCmdListSize = */sizeofarray(R63417_Backlight_CmdList),
    /*.pPowerOnCmdList = */R63417_PowerOn_CmdList,
    /*.pPowerOffCmdList = */R63417_PowerOff_CmdList,
    /*.pDisplayOnCmdList = */R63417_DisplayOn_CmdList,
    /*.pDisplayOffCmdList = */R63417_DisplayOff_CmdList,  
    /*.pBacklightCmdList = */R63417_Backlight_CmdList,
    /*.pFnDSIPanelOnOff = */CBIOS_NULL,
    /*.pFnDSIPanelSetBacklight = */CBIOS_NULL,
    /*.pFnDSIPanelGetBacklight = */CBIOS_NULL,
    /*.pFnDSIPanelSetCABC = */CBIOS_NULL,
    /*.pFnDSIPanelInit = */CBIOS_NULL,
    /*.pFnDSIPanelDeInit = */CBIOS_NULL,
};

