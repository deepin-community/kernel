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
** HX8392A type DSI panel descriptor definition, 
** which contains panel config and command lists.
**
** NOTE:
**
******************************************************************************/

#include "CBiosDSIPanel.h"
#include "CBiosDSI.h"

static CBIOS_U8 PasswordBuf[0x4] = 
{
    0xB9, 0xFF, 0x83, 0x92
};

static CBIOS_U8 PowerBuf[0x0E] =
{
    0xB1, 0x7C, 0x00, 0x44,
    0x14, 0x00, 0x11, 0x11,
    0x24, 0x2C, 0x3F, 0x3F,
    0x42, 0x72
};

static CBIOS_U8 MpuBuf[0x18] = 
{
    0xB4, 0x00, 0x00, 0x05,
    0x00, 0xA0, 0x05, 0x16,
    0x9D, 0x30, 0x03, 0x16,
    0x00, 0x03, 0x03, 0x00,
    0x1B, 0x04, 0x07, 0x07,
    0x01, 0x00, 0x1A, 0x77
};

static CBIOS_U8 DrivingBuf[0x18] = 
{
    0xD8, 0x00, 0x00, 0x04,
    0x00, 0xA0, 0x04, 0x16,
    0x9D, 0x30, 0x03, 0x16,
    0x00, 0x03, 0x03, 0x00,
    0x1B, 0x04, 0x07, 0x07,
    0x01, 0x00, 0x1A, 0x77
};

static CBIOS_U8 DisplayBuf[0x0D] = 
{
    0xB2, 0x08, 0xC8, 0x05,
    0x0F, 0x08, 0x44, 0x00,
    0xFF, 0x05, 0x0F, 0x04,
    0x20
};

static CBIOS_U8 VcomBuf[0x02] = 
{
    0xB6, 0x70
};

static CBIOS_U8 GammaRBuf[0x23] = 
{
    0xE0, 0x03, 0x1B, 0x22,
    0x3B, 0x3B, 0x3F, 0x2B,
    0x47, 0x05, 0x0B, 0x0E,
    0x10, 0x13, 0x11, 0x12,
    0x11, 0x1A, 0x03, 0x1B,
    0x22, 0x3B, 0x3B, 0x3F,
    0x2B, 0x47, 0x05, 0x0B,
    0x0E, 0x10, 0x13, 0x11,
    0x12, 0x11, 0x1A
};

static CBIOS_U8 GammaGBuf[0x23] = 
{
    0xE1, 0x03, 0x1B, 0x22,
    0x3B, 0x3B, 0x3F, 0x2B,
    0x47, 0x05, 0x0B, 0x0E,
    0x10, 0x13, 0x11, 0x12,
    0x11, 0x1A, 0x03, 0x1B,
    0x22, 0x3B, 0x3B, 0x3F,
    0x2B, 0x47, 0x05, 0x0B,
    0x0E, 0x10, 0x13, 0x11,
    0x12, 0x11, 0x1A
};

static CBIOS_U8 GammaBBuf[0x23] = 
{
    0xE2, 0x03, 0x1B, 0x22,
    0x3B, 0x3B, 0x3F, 0x2B,
    0x47, 0x05, 0x0B, 0x0E,
    0x10, 0x13, 0x11, 0x12,
    0x11, 0x1A, 0x03, 0x1B,
    0x22, 0x3B, 0x3B, 0x3F,
    0x2B, 0x47, 0x05, 0x0B,
    0x0E, 0x10, 0x13, 0x11,
    0x12, 0x11, 0x1A
};

static CBIOS_U8 PixelFormatBuf[0x02] = 
{
    0x3A, 0x77
};

static CBIOS_U8 MIPIBuf[0x03] = 
{
    0xBA, 0x13, 0x83 //0xBA, 0x13, 0x83
};

static CBIOS_U8 DisplayModeBuf[0x02] = 
{
    0xC2, 0x08      //panel display mode: 
};

static CBIOS_U8 InterPowerBuf[0x05] = 
{
    0xBF, 0x05, 0xE0, 0x02,
    0x00
};

static CBIOS_U8 TConBuf[0x03] = 
{
    0xC7, 0x00, 0x40
};

static CBIOS_U8 PanelBuf[0x02] = 
{
    0xCC, 0x08
};

static CBIOS_U8 EqBuf[0x02] = 
{
    0xD4, 0x0C
};

static CBIOS_U8 GckEqBuf[0x16] = 
{
    0xD5, 0x00, 0x08, 0x08,
    0x00, 0x44, 0x55, 0x66,
    0x77, 0xCC, 0xCC, 0xCC,
    0xCC, 0x00, 0x77, 0x66,
    0x55, 0x44, 0xCC, 0xCC,
    0xCC, 0xCC
};

static CBIOS_U8 C0Buf[0x03] = 
{
    0xC0, 0x01, 0x94
};

static CBIOS_U8 C6Buf[0x05] = 
{
    0xC6, 0x45, 0x02, 0x10,
    0x04
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
static CBIOS_U8 TEEnable[0x02] =
{
    0x35, 0x00
};

static CBIOS_DSI_CMD_DESC HX8392A_PowerOn_CmdList[] = 
{
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_DCS, 0,   sizeof(PasswordBuf),    PasswordBuf,    {0}},
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 120, sizeof(ExitSleepBuf),   ExitSleepBuf,   {0}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_DCS, 0,   sizeof(PowerBuf),       PowerBuf,       {0}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_DCS, 0,   sizeof(MpuBuf),         MpuBuf,         {0}},
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 0,   sizeof(VcomBuf),        VcomBuf,        {0}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_DCS, 0,   sizeof(MIPIBuf),        MIPIBuf,        {0}},
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 0,   sizeof(DisplayModeBuf), DisplayModeBuf, {0}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_DCS, 0,   sizeof(InterPowerBuf),  InterPowerBuf,  {0}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_DCS, 0,   sizeof(C0Buf),          C0Buf,          {0}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_GEN, 0,   sizeof(C6Buf),          C6Buf,          {0}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_DCS, 0,   sizeof(TConBuf),        TConBuf,        {0}},
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 0,   sizeof(PanelBuf),       PanelBuf,       {0}},
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 0,   sizeof(EqBuf),          EqBuf,          {0}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_DCS, 0,   sizeof(GckEqBuf),       GckEqBuf,       {0}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_DCS, 0,   sizeof(DrivingBuf),     DrivingBuf,     {0}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_GEN, 0,   sizeof(GammaRBuf),      GammaRBuf,      {0}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_GEN, 0,   sizeof(GammaGBuf),      GammaGBuf,      {0}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_GEN, 0,   sizeof(GammaBBuf),      GammaBBuf,      {0}},
    {0, 0, CBIOS_DSI_LONG_PACKET,  CBIOS_DSI_CONTENT_GEN, 0,   sizeof(DisplayBuf),     DisplayBuf,     {0}},
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_GEN, 0,   sizeof(PixelFormatBuf), PixelFormatBuf, {0}},
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 0,   sizeof(DisplayOnBuf),   DisplayOnBuf,   {0}},
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 0,   sizeof(TEEnable),       TEEnable,       {0}},
};


static CBIOS_DSI_CMD_DESC HX8392A_PowerOff_CmdList[] = 
{
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 0,   sizeof(DisplayOffBuf), DisplayOffBuf, {0}},
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 120, sizeof(EnterSleepBuf), EnterSleepBuf, {0}},
};

static CBIOS_DSI_CMD_DESC HX8392A_DisplayOn_CmdList[] = 
{
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 10,   sizeof(DisplayOnBuf), DisplayOnBuf, {0}},
};

static CBIOS_DSI_CMD_DESC HX8392A_DisplayOff_CmdList[] = 
{
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 10,   sizeof(DisplayOffBuf), DisplayOffBuf, {0}},
};

static CBIOS_DSI_CMD_DESC HX8392A_Backlight_CmdList[] = 
{
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(BacklightBuf), BacklightBuf, {0}},
};

CBIOS_DSI_PANEL_DESC HX8392A_Panel_Desc = 
{
    /*.VersionNum = */CBIOS_DSI_VERSION,
    /*.DSIConfig =*/ 
    {
        /*.DSIMode = */CBIOS_DSI_CMDMODE,
        /*.ClkLaneMode = */CBIOS_DSI_CLK_LANE_HARDWARE_CTL,
        /*.SyncPacketType = */CBIOS_DSI_SYNC_PULSE,
        /*.TEType = */CBIOS_DSI_TE_PAD,
        /*.TurnAroundTimeout = */0x1000000,
        /*.HS_TXTimeout = */0x1000000,
        /*.LP_RXTimeout = */0x1000000,
        /*.DMAThreshold = */0x400,
        /*.BacklightMax = */255,
        /*.BacklightMin = */0,
        /*.ConfigFlags = */{0x1F,},
    },
    /*.DSIPanelTbl =*/ 
    {
        /*.LaneNum = */4,
        /*.OutBpp = */24,
        /*.PanelTiming =*/ 
        {
            /*.XResolution = */720,
            /*.YResolution = */1280,
            /*.DCLK = */5775,
            /*.HorTotal = */880,
            /*.HorDisEnd = */720,
            /*.HorBStart = */720,
            /*.HorBEnd = */880,
            /*.HorSyncStart = */(720+64),
            /*.HorSyncEnd = */(720+64+32),
            /*.VerTotal = */1312,
            /*.VerDisEnd = */1280,
            /*.VerBStart = */1280,
            /*.VerBEnd = */1312,
            /*.VerSyncStart = */(1280+16),
            /*.VerSyncEnd = */(1280+16+4),
            /*.HVPolarity = */DSI_VNEGATIVE + DSI_HNEGATIVE,
        },
    },
    /*.PowerOnCmdListSize = */sizeofarray(HX8392A_PowerOn_CmdList),
    /*.PowerOffCmdListSize = */sizeofarray(HX8392A_PowerOff_CmdList),
    /*.DisplayOnCmdListSize = */sizeofarray(HX8392A_DisplayOn_CmdList),
    /*.DisplayOffCmdListSize = */sizeofarray(HX8392A_DisplayOff_CmdList),
    /*.BacklightCmdListSize = */sizeofarray(HX8392A_Backlight_CmdList),
    /*.pPowerOnCmdList = */HX8392A_PowerOn_CmdList,
    /*.pPowerOffCmdList = */HX8392A_PowerOff_CmdList,
    /*.pDisplayOnCmdList = */HX8392A_DisplayOn_CmdList,
    /*.pDisplayOffCmdList = */HX8392A_DisplayOff_CmdList,
    /*.pBacklightCmdList = */HX8392A_Backlight_CmdList,
    /*.pFnDSIPanelOnOff = */CBIOS_NULL,
    /*.pFnDSIPanelSetBacklight = */CBIOS_NULL,
    /*.pFnDSIPanelGetBacklight = */CBIOS_NULL,
    /*.pFnDSIPanelSetCABC = */CBIOS_NULL,
    /*.pFnDSIPanelInit = */CBIOS_NULL,
    /*.pFnDSIPanelDeInit = */CBIOS_NULL,
};

