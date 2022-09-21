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
** NT35595 type DSI panel descriptor definition, 
** which contains panel config and command lists.
**
** NOTE:
**
******************************************************************************/


#include "CBiosDSIPanel.h"
#include "CBiosDSI.h"


static CBIOS_U8 EnterSleepBuf[0x01] = 
{
    0x10
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

static CBIOS_U8   MipiCmdModeBuf[0x02] = {0xBB,0x10};
static  CBIOS_U8  EnterC2P3Buf[0x02] = {0xFF, 0x23};
static  CBIOS_U8  DmctCtlBuf[0x02] = {0x00, 0x02};
static  CBIOS_U8  CabcDimBuf[0x02] = {0x01, 0x84};
static  CBIOS_U8  DimStepBuf[0x02] ={0x05, 0x22};
static  CBIOS_U8  PwmFreqBuf[0x02] = {0x08, 0x04};
static  CBIOS_U8  DutyCntBuf[0x02] ={0x46, 0x00};
static  CBIOS_U8  StillModeGBuf0[0x02] ={0x17, 0xFF};
static CBIOS_U8   StillModeGBuf1[0x02] ={0x18, 0xFA};
static  CBIOS_U8  StillModeGBuf2[0x02] ={0x19, 0xF8};
static  CBIOS_U8  StillModeGBuf3[0x02] ={0x1A, 0xF5};
static  CBIOS_U8  StillModeGBuf4[0x02] = {0x1B, 0xEE};
static  CBIOS_U8  StillModeGBuf5[0x02] = {0x1C, 0xE1};
static  CBIOS_U8  StillModeGBuf6[0x02] = {0x1D, 0xD5};
static  CBIOS_U8  StillModeGBuf7[0x02] = {0x1E, 0xCD};
static  CBIOS_U8  StillModeGBuf8[0x02] = {0x1F, 0xB9};
static  CBIOS_U8  StillModeGBuf9[0x02] = {0x20, 0xB4}; 
static  CBIOS_U8  MovieModeGBuf0[0x02] ={0x21, 0xFF};
static  CBIOS_U8  MovieModeGBuf1[0x02] ={0x22, 0xFA,};
static  CBIOS_U8  MovieModeGBuf2[0x02] ={0x23, 0xF5};
static  CBIOS_U8  MovieModeGBuf3[0x02] ={0x24, 0xEB};
static  CBIOS_U8  MovieModeGBuf4[0x02] ={0x25, 0xE1};
static  CBIOS_U8  MovieModeGBuf5[0x02] ={0x26, 0xC8};
static  CBIOS_U8  MovieModeGBuf6[0x02] ={0x27, 0xAA};
static  CBIOS_U8  MovieModeGBuf7[0x02] ={0x28, 0x96};
static  CBIOS_U8  MovieModeGBuf8[0x02] ={0x29, 0x73};
static  CBIOS_U8  MovieModeGBuf9[0x02] ={0x2A, 0x66};
static CBIOS_U8  StillModeGamBuf0[0x02] = {0x09, 0x02};
static CBIOS_U8  StillModeGamBuf1[0x02] = {0x0A, 0x06};
static CBIOS_U8  StillModeGamBuf2[0x02] = {0x0B, 0x07};
static CBIOS_U8  StillModeGamBuf3[0x02] = {0x0C, 0x08};
static CBIOS_U8  StillModeGamBuf4[0x02] = {0x0D, 0x09};    
static CBIOS_U8  StillModeGamBuf5[0x02] = {0x0E, 0x0B};  
static CBIOS_U8  StillModeGamBuf6[0x02] = {0x0F, 0x0D}; 
static CBIOS_U8  StillModeGamBuf7[0x02] = {0x10, 0x0D}; 
static CBIOS_U8  StillModeGamBuf8[0x02] = {0x11, 0x12};  
static CBIOS_U8  StillModeGamBuf9[0x02] = {0x12, 0x20}; 
static CBIOS_U8  MovieModeGamBuf[0x02] = {0x33, 0x0C};
static CBIOS_U8  MtpReloadCtlBuf[0x02] = {0xFB, 0x01}; 
static CBIOS_U8  EnterC2P2Buf[0x02] = {0xFF, 0x22};
static CBIOS_U8  AdjustSatuBuf0[0x02] = {0x00, 0x00};
static CBIOS_U8  AdjustSatuBuf1[0x02] = {0x01, 0x04};
static CBIOS_U8  AdjustSatuBuf2[0x02] = {0x02, 0x08};
static CBIOS_U8  AdjustSatuBuf3[0x02] = {0x03, 0x0C};
static CBIOS_U8  AdjustSatuBuf4[0x02] = {0x04, 0x10};
static CBIOS_U8  AdjustSatuBuf5[0x02] = {0x05, 0x14};
static CBIOS_U8  AdjustSatuBuf6[0x02] = {0x06, 0x18};
static CBIOS_U8  AdjustSatuBuf7[0x02] = {0x07, 0x20};
static CBIOS_U8  AdjustSatuBuf8[0x02] = {0x08, 0x24};
static CBIOS_U8  AdjustSatuBuf9[0x02] = {0x09, 0x28};
static CBIOS_U8  AdjustSatuBufA[0x02] = {0x0A, 0x30};
static CBIOS_U8  AdjustSatuBufB[0x02] = {0x0B, 0x38};
static CBIOS_U8  AdjustSatuBufC[0x02] = {0x0C, 0x38};
static CBIOS_U8  AdjustSatuBufD[0x02] = {0x0D, 0x30};
static CBIOS_U8  AdjustSatuBufE[0x02] = {0x0E, 0x28};
static CBIOS_U8  AdjustSatuBufF[0x02] = {0x0F, 0x20};
static CBIOS_U8  AdjustSatuBuf10[0x02] = {0x10, 0x10};
static CBIOS_U8  AdjustSatuBuf11[0x02] = {0x11, 0x00};
static CBIOS_U8  AdjustSatuBuf12[0x02] = {0x12, 0x00};
static CBIOS_U8  AdjustSatuBuf13[0x02] = {0x13, 0x00};
static CBIOS_U8  HueDegBuf0[0x02] = {0x32, 0x10};
static CBIOS_U8  HueDegBuf1[0x02] = {0x33, 0x10};
static CBIOS_U8  HueDegBuf2[0x02] = {0x34, 0x10};
static CBIOS_U8  HueDegBuf3[0x02] = {0x35, 0x10};
static CBIOS_U8  HueDegBuf4[0x02] = {0x36, 0x10};
static CBIOS_U8  HueDegBuf5[0x02] = {0x37, 0x10};
static CBIOS_U8  HueDegBuf6[0x02] = {0x38, 0x10};
static CBIOS_U8  HueDegBuf7[0x02] = {0x39, 0x10};
static CBIOS_U8  HueDegBuf8[0x02] = {0x3A, 0x10};
static CBIOS_U8  HueDegBuf9[0x02] = {0x3B, 0x10};
static CBIOS_U8  HueDegBufA[0x02] = {0x3F, 0x10};
static CBIOS_U8  HueDegBufB[0x02] = {0x40, 0x10};
static CBIOS_U8  HueDegBufC[0x02] = {0x41, 0x10};
static CBIOS_U8  HueDegBufD[0x02] = {0x42, 0x10};
static CBIOS_U8  HueDegBufE[0x02] = {0x43, 0x10};
static CBIOS_U8  HueDegBufF[0x02] = {0x44, 0x10};
static CBIOS_U8  HueDegBuf10[0x02] = {0x45, 0x10};
static CBIOS_U8  HueDegBuf11[0x02] = {0x46, 0x10};
static CBIOS_U8  HueDegBuf12[0x02] = {0x47, 0x10};
static CBIOS_U8  HueDegBuf13[0x02] = {0x48, 0x10};
static CBIOS_U8  HueDegBuf14[0x02] = {0x49, 0x10};
static CBIOS_U8  HueDegBuf15[0x02] = {0x4A, 0x10};
static CBIOS_U8  HueDegBuf16[0x02] = {0x4B, 0x10};
static CBIOS_U8  HueDegBuf17[0x02] = {0x4C, 0x10};
static CBIOS_U8  DisableViBuf[0x02] = {0x1A, 0x00};
static CBIOS_U8  EnableSmartBuf[0x02] = {0x53, 0x01};
static CBIOS_U8  DisableSkinBuf[0x02] = {0x54, 0x00};
static CBIOS_U8  DisableOverBuf[0x02] = {0x55, 0x00};
static CBIOS_U8  EnableSmtCotra[0x02] = {0x56, 0x01};
static CBIOS_U8  EnableEdgeBuf[0x02] = {0x68, 0x01};
static CBIOS_U8  SmtClrGrBuf[0x02] = {0x4D, 0x0E};
static CBIOS_U8  SmtCotraDppBuf[0x02] = {0x58, 0x0E};
static CBIOS_U8  SmtCotraLppBuf[0x02] = {0x59, 0x14};
static CBIOS_U8  TurnOffSmtBuf[0x02] = {0x64, 0x20};
static CBIOS_U8  EdgeLevel2Buf[0x02] = {0x65, 0x02};
static CBIOS_U8  EdgeThdL0Buf[0x02] = {0x69, 0x02};
static CBIOS_U8  MtpRelCmdBuf[0x02] = {0xFB, 0x01};
static CBIOS_U8  EnterCmd1Buf[0x02] = {0xFF, 0x10};
static CBIOS_U8  EnableISCMBuf[0x02] = {0x55, 0x82};
static CBIOS_U8  EnableCmdFunBuf[0x02] = {0x5E, 0x14};
static CBIOS_U8  LcdSleepCmd[0x02] = {0x11, 0x00};
static CBIOS_U8  EnterCmd24Buf[0x02] = {0xFF, 0x24};
static CBIOS_U8  OutputHTHBuf[0x02] = {0xC6, 0x09};
static CBIOS_U8  NoMtpRelBuf[0x02] = {0xFB, 0x01};
static CBIOS_U8  TurnOnTEBuf[0x02] = {0x35, 0x00};
static CBIOS_U8  NoNameFuncBuf[0x02] = {0x51, 0xff};
static CBIOS_U8  LcdDispOnBuf[0x02] = {0x29, 0x00};
static CBIOS_U8  EnableBlBuf[0x02] = {0x53, 0x24};

static CBIOS_DSI_CMD_DESC NT35595_PowerOn_CmdList[] = 
{
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(MipiCmdModeBuf), MipiCmdModeBuf,  {0}},    //MIPI CMD mode
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(EnterC2P3Buf), EnterC2P3Buf,  {0}},    //Enter CMD2 page3
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(DmctCtlBuf), DmctCtlBuf,  {0}},    //DMCT = 1, Select the enable / disable mechanism for CABC and Manual dimming functions.
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(CabcDimBuf), CabcDimBuf,     {0}},    //Enable CABC dimmin and Gamma Dimming
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(DimStepBuf), DimStepBuf,  {0}},    //Moving mode dimming 32step, still mode 8step
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(PwmFreqBuf), PwmFreqBuf,  {0}},    //PWMDIV[7:0] for PWM frequency
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(DutyCntBuf), DutyCntBuf,  {0}},    //PWM duty count for 42.96kHZ
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(StillModeGBuf0), StillModeGBuf0,  {0}},    //CABC still mode PWM - gray 240-255
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(StillModeGBuf1), StillModeGBuf1,  {0}},    //CABC still mode PWM - gray 224-239
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(StillModeGBuf2), StillModeGBuf2,  {0}},    //CABC still mode PWM - gray 208-223
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(StillModeGBuf3), StillModeGBuf3,  {0}},    //CABC still mode PWM - gray 192-207
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(StillModeGBuf4), StillModeGBuf4,  {0}},    //CABC still mode PWM - gray 160-191
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(StillModeGBuf5), StillModeGBuf5,  {0}},    //CABC still mode PWM - gray 128-159
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(StillModeGBuf6), StillModeGBuf6,  {0}},    //CABC still mode PWM - gray 112-127
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(StillModeGBuf7), StillModeGBuf7,  {0}},    //CABC still mode PWM - gray 64-111
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(StillModeGBuf8), StillModeGBuf8,  {0}},    //CABC still mode PWM - gray 16-63
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(StillModeGBuf9), StillModeGBuf9,  {0}},    //CABC still mode PWM - gray 0-15
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(MovieModeGBuf0), MovieModeGBuf0,  {0}},    //CABC movie mode PWM - gray 240-255
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(MovieModeGBuf1), MovieModeGBuf1,  {0}},    //CABC movie mode PWM - gray 224-239
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(MovieModeGBuf2), MovieModeGBuf2,  {0}},    //CABC movie mode PWM - gray 208-223
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(MovieModeGBuf3), MovieModeGBuf3,  {0}},    //CABC movie mode PWM -gray 192-207
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(MovieModeGBuf4), MovieModeGBuf4,  {0}},    //CABC movie mode PWM - gray 160-191
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(MovieModeGBuf5), MovieModeGBuf5,  {0}},    //CABC movie mode PWM - gray 128-159
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(MovieModeGBuf6), MovieModeGBuf6,  {0}},    //CABC movie mode PWM - gray 112-127
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(MovieModeGBuf7), MovieModeGBuf7,  {0}},    //CABC movie mode PWM- gray 64-111
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(MovieModeGBuf8), MovieModeGBuf8,  {0}},    //CABC movie mode PWM - gray 16-63
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(MovieModeGBuf9), MovieModeGBuf9,  {0}},    //CABC movie mode PWM - gray 0-15
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(StillModeGamBuf0), StillModeGamBuf0,  {0}},    //CABC still mode Gamma curve compensation - gray 240-255
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(StillModeGamBuf1), StillModeGamBuf1,  {0}},    //CABC still mode Gamma curve compensation - gray 224-239
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(StillModeGamBuf2), StillModeGamBuf2,  {0}},    //CABC still mode Gamma curve compensation - gray 208-223
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(StillModeGamBuf3), StillModeGamBuf3,  {0}},    //CABC still mode Gamma curve compensation - gray 192-207
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(StillModeGamBuf4), StillModeGamBuf4,  {0}},    //CABC still mode Gamma curve compensation - gray 160-191
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(StillModeGamBuf5), StillModeGamBuf5,  {0}},    //CABC still mode Gamma curve compensation - gray 128-159
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(StillModeGamBuf6), StillModeGamBuf6,  {0}},    //CABC still mode Gamma curve compensation - gray 112-127
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(StillModeGamBuf7), StillModeGamBuf7,  {0}},    //CABC still mode Gamma curve compensation - gray 64-111
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(StillModeGamBuf8), StillModeGamBuf8,  {0}},    //CABC still mode Gamma curve compensation - gray 16-63
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(StillModeGamBuf9), StillModeGamBuf9,  {0}},    //CABC still mode Gamma curve compensation - gray 0-15
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(MovieModeGamBuf), MovieModeGamBuf,  {0}},    //CABC Movie mode Gamma curve compensation
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(MtpReloadCtlBuf), MtpReloadCtlBuf,  {0}},    //MTP do not reload CMD2 P3
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(EnterC2P2Buf), EnterC2P2Buf,  {0}},    //Enter CMD2 page2
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(AdjustSatuBuf0), AdjustSatuBuf0,  {0}},    //parameter to adjust saturation > 0.98
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(AdjustSatuBuf1), AdjustSatuBuf1,  {0}},    //parameter to adjustsaturation 0.95~0.98
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(AdjustSatuBuf2), AdjustSatuBuf2,  {0}},    //parameter to adjustsaturation 0.91~0.95
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(AdjustSatuBuf3), AdjustSatuBuf3,  {0}},    //parameter to adjustsaturation 0.88~0.91
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(AdjustSatuBuf4), AdjustSatuBuf4,  {0}},    //parameter to adjustsaturation 0.85~0.88
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(AdjustSatuBuf5), AdjustSatuBuf5,  {0}},    //parameter to adjustsaturation 0.82~0.85
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(AdjustSatuBuf6), AdjustSatuBuf6,  {0}},    //parameter to adjustsaturation 0.79~0.82
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(AdjustSatuBuf7), AdjustSatuBuf7,  {0}},    //parameter to adjustsaturation 0.76~0.79
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(AdjustSatuBuf8), AdjustSatuBuf8,  {0}},    //parameter to adjustsaturation 0.73~0.76
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(AdjustSatuBuf9), AdjustSatuBuf9,  {0}},    //parameter to adjustsaturation 0.7~0.73
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(AdjustSatuBufA), AdjustSatuBufA,  {0}},    //parameter to adjustsaturation 0.66~0.7
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(AdjustSatuBufB), AdjustSatuBufB,  {0}},    //parameter to adjustsaturation 0.6~0.66
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(AdjustSatuBufC), AdjustSatuBufC,  {0}},    //parameter to adjustsaturation 0.54~0.6
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(AdjustSatuBufD), AdjustSatuBufD,  {0}},    //parameter to adjustsaturation 0.48~0.54
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(AdjustSatuBufE), AdjustSatuBufE,  {0}},    //parameter to adjustsaturation 0.41~0.48
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(AdjustSatuBufF), AdjustSatuBufF,  {0}},    //parameter to adjustsaturation 0.35~0.41
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(AdjustSatuBuf10), AdjustSatuBuf10,  {0}},    //parameter to adjustsaturation 0.26~0.35
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(AdjustSatuBuf11), AdjustSatuBuf11,  {0}},    //parameter to adjustsaturation 0.16~0.26
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(AdjustSatuBuf12), AdjustSatuBuf12,  {0}},    //parameter to adjustsaturation 0.07~0.16
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(AdjustSatuBuf13), AdjustSatuBuf13,  {0}},    //parameter to adjustsaturation < 0.07
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBuf0), HueDegBuf0,  {0}},    //HUE 0~15 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBuf1), HueDegBuf1,  {0}},    //HUE 15~30 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBuf2), HueDegBuf2,  {0}},    //HUE 30~45 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBuf3), HueDegBuf3,  {0}},    //HUE 45~60 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBuf4), HueDegBuf4,  {0}},    //HUE 60~75 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBuf5), HueDegBuf5,  {0}},    //HUE 75~90 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBuf6), HueDegBuf6,  {0}},    //HUE 90~105 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBuf7), HueDegBuf7,  {0}},    //HUE 105~120 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBuf8), HueDegBuf8,  {0}},    //HUE 120~135 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBuf9), HueDegBuf9,  {0}},    //HUE 135~150 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBufA), HueDegBufA,  {0}},    //HUE 150~165 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBufB), HueDegBufB,  {0}},    //HUE 165~180 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBufC), HueDegBufC,  {0}},    //HUE 180~195 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBufD), HueDegBufD,  {0}},    //HUE 195~210 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBufE), HueDegBufE,  {0}},    //HUE 210~225 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBufF), HueDegBufF,  {0}},    //HUE 225~240 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBuf10), HueDegBuf10,  {0}},    //HUE 240~255 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBuf11), HueDegBuf11,  {0}},    //HUE 255~270 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBuf12), HueDegBuf12,  {0}},    //HUE 270~285 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBuf13), HueDegBuf13,  {0}},    //HUE 285~300 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBuf14), HueDegBuf14,  {0}},    //HUE 300~315 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBuf15), HueDegBuf15,  {0}},    //HUE 315~330 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBuf16), HueDegBuf16,  {0}},    //HUE 330~345 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(HueDegBuf17), HueDegBuf17,  {0}},    //HUE 345~360 degree
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(DisableViBuf), DisableViBuf,  {0}},    //Disable Vivid Color
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(EnableSmartBuf), EnableSmartBuf,  {0}},    //Enable Smart Color
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(DisableSkinBuf), DisableSkinBuf,  {0}},    //Disable SKIN_KEEP
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(DisableOverBuf), DisableOverBuf,  {0}},    //Disable Over-Saturation protection
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(EnableSmtCotra), EnableSmtCotra,  {0}},    //Enable Smart Contrast
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(EnableEdgeBuf), EnableEdgeBuf,  {0}},    //Enable Edge Enhancement
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(SmtClrGrBuf), SmtClrGrBuf,  {0}},    //Smart Color global ratio
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(SmtCotraDppBuf), SmtCotraDppBuf,  {0}},    //Smart Contrast Dark part parameter 14
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(SmtCotraLppBuf), SmtCotraLppBuf,  {0}},    //Smart Contrast Light part parameter 20
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(TurnOffSmtBuf), TurnOffSmtBuf,  {0}},    //Turn off Smart Contrast local reference
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(EdgeLevel2Buf), EdgeLevel2Buf,  {0}},    //Edge level 2
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(EdgeThdL0Buf), EdgeThdL0Buf,  {0}},    //Edge THD level 0
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(MtpRelCmdBuf), MtpRelCmdBuf,  {0}},    //MTP do not reload CMD2 P2
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(EnterCmd1Buf), EnterCmd1Buf,  {0}},    //Enter CMD1
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(EnableISCMBuf), EnableISCMBuf,  {0}},    //Enable IE slight level and CABC still mode
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(EnableCmdFunBuf), EnableCmdFunBuf,  {0}},    //Enable CMB function
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 120, sizeof(LcdSleepCmd), LcdSleepCmd,  {0}},    //LCD Sleep out command
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(EnterCmd24Buf), EnterCmd24Buf,  {0}},    //Enter CMD24
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(OutputHTHBuf), OutputHTHBuf,  {0}},    //output Hsync to HSOUT
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(NoMtpRelBuf), NoMtpRelBuf,  {0}},    //No MTP reload at CMD2 P4
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(TurnOnTEBuf), TurnOnTEBuf,  {0}},    //Turn on TE (TEE/ TE H/W pin) function
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(NoNameFuncBuf), NoNameFuncBuf,  {0}},
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(LcdDispOnBuf), LcdDispOnBuf,  {0}},    //LCD Display on
    {0, 0, CBIOS_DSI_LONG_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(EnableBlBuf), EnableBlBuf,  {0}},    //Enable BL 
};


static CBIOS_DSI_CMD_DESC NT35595_PowerOff_CmdList[] = 
{
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 0,   sizeof(DisplayOffBuf), DisplayOffBuf,  {0}},   
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 120, sizeof(EnterSleepBuf), EnterSleepBuf,  {0}},   
};

static CBIOS_DSI_CMD_DESC NT35595_DisplayOn_CmdList[] = 
{
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 10, sizeof(DisplayOnBuf), DisplayOnBuf,    {0}},
};

static CBIOS_DSI_CMD_DESC NT35595_DisplayOff_CmdList[] = 
{
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 10, sizeof(DisplayOffBuf), DisplayOffBuf,  {0}},
};

static CBIOS_DSI_CMD_DESC NT35595_Backlight_CmdList[] = 
{
    {0, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(BacklightBuf), BacklightBuf,  {0}},   
};

#define NT35595_INDEX_IOVDD        0
#define NT35595_INDEX_AVDDP        1
#define NT35595_INDEX_AVDDN        2
#define NT35595_INDEX_RESX         3
#define NT35595_INDEX_BLEN         4

// GPIO PORTS for board version A0 & A1
GPIO_PORT NT35595_GPIO_PORTS_A1[] = 
{
    {CBIOS_GPIO_AUD,  18},  //IOVDD
    {CBIOS_GPIO_VSOC, 57},  //AVDDP
    {CBIOS_GPIO_VSOC, 58},  //AVDDN
    {CBIOS_GPIO_GFX,  10},  //RESX
    {CBIOS_GPIO_VSUS, 46},  //DSI_BLEN
};

// GPIO PORTS for board version A2 & A3
GPIO_PORT NT35595_GPIO_PORTS_A2[] = 
{
    {CBIOS_GPIO_VSUS, 48},  //IOVDD
    {CBIOS_GPIO_VSOC, 57},  //AVDD+
    {CBIOS_GPIO_VSOC, 58},  //AVDD-
    {CBIOS_GPIO_VSUS, 33},  //RESX
    {CBIOS_GPIO_VSUS, 46},  //DSI_BLEN
};

extern CBIOS_DSI_PANEL_DESC NT35595_Panel_Desc;

static CBIOS_STATUS cbNT35595_GetGpioPorts(PCBIOS_VOID pvcbe, PGPIO_PORT *pGpioPorts, CBIOS_U32 *pNumPorts)
{
    CBIOS_BOARD_VERSION BoardVersion = cbGetBoardVersion(pvcbe);
    CBIOS_STATUS Status = CBIOS_OK;
    
    *pGpioPorts = CBIOS_NULL;
    *pNumPorts = 0;
    if (BoardVersion <= CBIOS_BOARD_VERSION_1)
    {
        *pGpioPorts = NT35595_GPIO_PORTS_A1;
        *pNumPorts = sizeofarray(NT35595_GPIO_PORTS_A1);
    }
    else if (BoardVersion == CBIOS_BOARD_VERSION_2)
    {
        *pGpioPorts = NT35595_GPIO_PORTS_A2;
        *pNumPorts = sizeofarray(NT35595_GPIO_PORTS_A2);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "%s: invalid board version: 0x%x!\n", FUNCTION_NAME, BoardVersion));
        Status = CBIOS_ER_INTERNAL;
    }

    return Status;
}

CBIOS_STATUS cbNT35595_Init(PCBIOS_VOID pvcbe)
{
    CBIOS_STATUS Status = CBIOS_OK;
    CBIOS_U32 Requested = 0, i = 0, NumPorts = 0;
    PGPIO_PORT pNT35595GpioPorts = CBIOS_NULL;

    cbNT35595_GetGpioPorts(pvcbe, &pNT35595GpioPorts, &NumPorts);

    for (i = 0; i < NumPorts; i++)
    {
        if (cbRequestGPIO(pvcbe, pNT35595GpioPorts[i].GPIOType, pNT35595GpioPorts[i].GPIOIndex) == CBIOS_OK)
        {
            cbSetGPIODirectionOutput(pvcbe, pNT35595GpioPorts[i].GPIOType, pNT35595GpioPorts[i].GPIOIndex, 0);
            Requested |= (1 << i);
            Status = CBIOS_OK;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "Request GPIO fail! Type: %x, Index: %d\n", pNT35595GpioPorts[i].GPIOType, pNT35595GpioPorts[i].GPIOIndex));
            Status = CBIOS_ER_INTERNAL;
            break;
        }
    }

    if (Status != CBIOS_OK)//request fail, free requested GPIOs
    {
        for (i = 0; i < NumPorts; i++)
        {
            if (i & Requested)
            {
                cbFreeGPIO(pvcbe, pNT35595GpioPorts[i].GPIOType, pNT35595GpioPorts[i].GPIOIndex);
            }
        }
    }

    return Status;
}

CBIOS_STATUS cbNT35595_DeInit(PCBIOS_VOID pvcbe)
{
    CBIOS_STATUS Status = CBIOS_OK;
    CBIOS_U32 i = 0, NumPorts = 0;
    PGPIO_PORT pNT35595GpioPorts = CBIOS_NULL;

    cbNT35595_GetGpioPorts(pvcbe, &pNT35595GpioPorts, &NumPorts);

    if (pNT35595GpioPorts == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "%s: cannot get GPIO ports for NT35595\n", FUNCTION_NAME));
        Status = CBIOS_ER_INTERNAL;
        return Status;
    }

    for (i = 0; i < NumPorts; i++)
    {
        cbFreeGPIO(pvcbe, pNT35595GpioPorts[i].GPIOType, pNT35595GpioPorts[i].GPIOIndex);
    }
    
    return Status;
}

CBIOS_STATUS cbNT35595_PanelPowerOnOff(PCBIOS_VOID pvcbe, CBIOS_BOOL bTurnOn)
{
    CBIOS_STATUS Status = CBIOS_OK;
    CBIOS_U32 NumPorts = 0;
    PGPIO_PORT pNT35595GpioPorts = CBIOS_NULL;

    cbNT35595_GetGpioPorts(pvcbe, &pNT35595GpioPorts, &NumPorts);

    if (pNT35595GpioPorts == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "%s: cannot get GPIO ports for NT35595\n", FUNCTION_NAME));
        Status = CBIOS_ER_INTERNAL;
        return Status;
    }

    if(bTurnOn)
    {
        //XRES=L
        cbWriteGPIO(pvcbe, pNT35595GpioPorts[NT35595_INDEX_RESX].GPIOType, pNT35595GpioPorts[NT35595_INDEX_RESX].GPIOIndex, 0);
        cbDelayMilliSeconds(1);
        
        //IOVDD On --config 1.8v
        cbWriteGPIO(pvcbe, pNT35595GpioPorts[NT35595_INDEX_IOVDD].GPIOType, pNT35595GpioPorts[NT35595_INDEX_IOVDD].GPIOIndex, 1);
        cbDelayMilliSeconds(2);
        
        //DSI_BL_EN
        cbWriteGPIO(pvcbe, pNT35595GpioPorts[NT35595_INDEX_BLEN].GPIOType, pNT35595GpioPorts[NT35595_INDEX_BLEN].GPIOIndex, 1);
        cbDelayMilliSeconds(2);
        
        //AVDD+ ON
        cbWriteGPIO(pvcbe, pNT35595GpioPorts[NT35595_INDEX_AVDDP].GPIOType, pNT35595GpioPorts[NT35595_INDEX_AVDDP].GPIOIndex, 1);
        cbDelayMilliSeconds(1);
        
        //AVDD- ON
        cbWriteGPIO(pvcbe, pNT35595GpioPorts[NT35595_INDEX_AVDDN].GPIOType, pNT35595GpioPorts[NT35595_INDEX_AVDDN].GPIOIndex, 1);
        cbDelayMilliSeconds(15);
        
        //XRES=H
        cbWriteGPIO(pvcbe, pNT35595GpioPorts[NT35595_INDEX_RESX].GPIOType, pNT35595GpioPorts[NT35595_INDEX_RESX].GPIOIndex, 1);
        cbDelayMilliSeconds(1);
        
        //XRES=L
        cbWriteGPIO(pvcbe, pNT35595GpioPorts[NT35595_INDEX_RESX].GPIOType, pNT35595GpioPorts[NT35595_INDEX_RESX].GPIOIndex, 0);
        cbDelayMilliSeconds(1);
        
        //XRES=H
        cbWriteGPIO(pvcbe, pNT35595GpioPorts[NT35595_INDEX_RESX].GPIOType, pNT35595GpioPorts[NT35595_INDEX_RESX].GPIOIndex, 1);
        cbDelayMilliSeconds(15);
        

        
        Status = cbDSI_SendCmdList(pvcbe, NT35595_PowerOn_CmdList, sizeofarray(NT35595_PowerOn_CmdList));
    }
    else
    {
        Status = cbDSI_SendCmdList(pvcbe, NT35595_PowerOff_CmdList, sizeofarray(NT35595_PowerOff_CmdList));
        
        cbDelayMilliSeconds(10);
        
        //DSI_BL_EN OFF
        cbWriteGPIO(pvcbe, pNT35595GpioPorts[NT35595_INDEX_BLEN].GPIOType, pNT35595GpioPorts[NT35595_INDEX_BLEN].GPIOIndex, 0);
        
        //AVDD- OFF
        cbWriteGPIO(pvcbe, pNT35595GpioPorts[NT35595_INDEX_AVDDN].GPIOType, pNT35595GpioPorts[NT35595_INDEX_AVDDN].GPIOIndex, 0);
        cbDelayMilliSeconds(1);

        //AVDD+ OFF
        cbWriteGPIO(pvcbe, pNT35595GpioPorts[NT35595_INDEX_AVDDP].GPIOType, pNT35595GpioPorts[NT35595_INDEX_AVDDP].GPIOIndex, 0);
        cbDelayMilliSeconds(1);

        //XRES=L
        cbWriteGPIO(pvcbe, pNT35595GpioPorts[NT35595_INDEX_RESX].GPIOType, pNT35595GpioPorts[NT35595_INDEX_RESX].GPIOIndex, 0);
        cbDelayMilliSeconds(1);

        //config 1.8v -- IOVDD OFF
        cbWriteGPIO(pvcbe, pNT35595GpioPorts[NT35595_INDEX_IOVDD].GPIOType, pNT35595GpioPorts[NT35595_INDEX_IOVDD].GPIOIndex, 0);
    }

    return Status;
}

CBIOS_DSI_PANEL_DESC NT35595_Panel_Desc = 
{
    /*.VersionNum = */CBIOS_DSI_VERSION,
    /*.DSIConfig =*/ 
    {
        /*.DSIMode = */CBIOS_DSI_CMDMODE,
        /*.ClkLaneMode = */CBIOS_DSI_CLK_LANE_HARDWARE_CTL,
        /*.SyncPacketType = */CBIOS_DSI_SYNC_PULSE,
#ifdef ELT2K_DIU_FPGA
        /*.TEType = */CBIOS_DSI_TE_PAD,
#else
        /*.TEType = */CBIOS_DSI_TE_PAD,
#endif
        /*.TurnAroundTimeout = */0x1000000,
        /*.HS_TXTimeout = */0x1000000,
        /*.LP_RXTimeout = */0x1000000,
        /*.DMAThreshold = */0x400,
        /*.BacklightMax =*/ 255,
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
#ifdef ELT2K_DIU_FPGA
            /*.DCLK = */5400,
#else
            /*.DCLK = */15550, //14040,
#endif
            /*.HorTotal = */1200 + 128,
            /*.HorDisEnd = */1080,
            /*.HorBStart = */1080,
            /*.HorBEnd = */1200 + 128,
            /*.HorSyncStart = */(1080+64 ) + 32,
            /*.HorSyncEnd = */(1080+64+32) + 64,
            /*.VerTotal = */1950,
            /*.VerDisEnd = */1920,
            /*.VerBStart = */1920,
            /*.VerBEnd = */1950,
            /*.VerSyncStart = */(1920+16),
            /*.VerSyncEnd = */(1920+16+2),
            /*.HVPolarity = */DSI_VNEGATIVE + DSI_HNEGATIVE,
        },
    },
    /*.PowerOnCmdListSize = */sizeofarray(NT35595_PowerOn_CmdList),
    /*.PowerOffCmdListSize = */sizeofarray(NT35595_PowerOff_CmdList),
    /*.DisplayOnCmdListSize = */sizeofarray(NT35595_DisplayOn_CmdList),
    /*.DisplayOffCmdListSize = */sizeofarray(NT35595_DisplayOff_CmdList),
    /*.BacklightCmdListSize = */sizeofarray(NT35595_Backlight_CmdList),
    /*.pPowerOnCmdList = */NT35595_PowerOn_CmdList,
    /*.pPowerOffCmdList = */NT35595_PowerOff_CmdList,
    /*.pDisplayOnCmdList = */NT35595_DisplayOn_CmdList,
    /*.pDisplayOffCmdList = */NT35595_DisplayOff_CmdList,
    /*.pBacklightCmdList = */NT35595_Backlight_CmdList,
    /*.pFnDSIPanelOnOff = */cbNT35595_PanelPowerOnOff,
    /*.pFnDSIPanelSetBacklight = */CBIOS_NULL,
    /*.pFnDSIPanelGetBacklight = */CBIOS_NULL,
    /*.pFnDSIPanelSetCABC = */CBIOS_NULL,
    /*.pFnDSIPanelInit = */cbNT35595_Init,
    /*.pFnDSIPanelDeInit = */CBIOS_NULL,
};
