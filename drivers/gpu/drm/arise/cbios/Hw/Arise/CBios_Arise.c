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
#include "../Register/BIU_SBI_registers.h"
#include "../Register/pmu_registers.h"
#include "../HwBlock/CBiosDIU_HDTV.h"

CBIOS_U32   CheckCnt = 0;

static    CBIOS_U32  PSStartAddrIndex[CBIOS_IGACOUNTS] = {0x81dc, 0x33988, 0x34088, 0x34788};
static    CBIOS_U32  PSCompFifoIndex[CBIOS_IGACOUNTS] = {0x81A4, 0x33910, 0x34010, 0x34710};
static    CBIOS_U32  PSBaseOffsetIndex[CBIOS_IGACOUNTS] = {0x33700, 0x3394C, 0x3404C, 0x3474C};
static    CBIOS_U32  PSRightBaseIndex[CBIOS_IGACOUNTS] = {0x81E0, 0x3398C, 0x3408C, 0x3478C};
static    CBIOS_U32  PSVsyncOffIndex[CBIOS_IGACOUNTS] = {0x81C4, 0x33908, 0x34008, 0x34708};
static    CBIOS_U32  PSStrideIndex[CBIOS_IGACOUNTS] = {0x81C8, 0x3390C, 0x3400C, 0x3470C};
static    CBIOS_U32  PSShadowIndex[CBIOS_IGACOUNTS] = {0x81FC, 0x33924, 0x34024, 0x34724};
//static    CBIOS_U32  PSWinWIndex[CBIOS_IGACOUNTS] = {0x33290, 0x3392C, 0x3402C, 0x3472C};
//static    CBIOS_U32  PSWinHIndex[CBIOS_IGACOUNTS] = {0x33294, 0x33930, 0x34030, 0x34730};
//static    CBIOS_U32  PSScalRatWIndex[CBIOS_IGACOUNTS] = {0x33318, 0x33990, 0x34090, 0x34790};
//static    CBIOS_U32  PSScalRatHIndex[CBIOS_IGACOUNTS] = {0x3331C, 0x33994, 0x34094, 0x34794};
//static    CBIOS_U32  PSScalSrcIndex[CBIOS_IGACOUNTS] = {0x33320, 0x33998, 0x34098, 0x34798};
static    CBIOS_U32  PSCscIndex[CBIOS_IGACOUNTS] = {0x33644, 0x33944, 0x34044, 0x34744};
static    CBIOS_U32  PSCscCoef1[CBIOS_IGACOUNTS] = {0x33634, 0x33934, 0x34034, 0x34734};
static    CBIOS_U32  PSCscCoef2[CBIOS_IGACOUNTS] = {0x33638, 0x33938, 0x34038, 0x34738};
static    CBIOS_U32  PSCscCoef3[CBIOS_IGACOUNTS] = {0x3363c, 0x3393c, 0x3403c, 0x3473c};
static    CBIOS_U32  PSCscCoef4[CBIOS_IGACOUNTS] = {0x33640, 0x33940, 0x34040, 0x34740};
static    CBIOS_U32  PSBLIndex[CBIOS_IGACOUNTS] = {0x33810, 0x33E6C, 0x34574, 0x34c74};

static    CBIOS_U32  SSStartAddrIndex[CBIOS_IGACOUNTS] = {0x81D0, 0x339D4, 0x340D4, 0x347D4};
static    CBIOS_U32  SSCompFifoIndex[CBIOS_IGACOUNTS] = {0x8198, 0x339C8, 0x340C8, 0x347C8};
static    CBIOS_U32  SSBaseOffsetIndex[CBIOS_IGACOUNTS] = {0x33708, 0x33A14, 0x34114, 0x34814};
static    CBIOS_U32  SSEnableIndex[CBIOS_IGACOUNTS] = {0x33834, 0x33E70, 0x34578, 0x34C78};
static    CBIOS_U32  SSRightBaseIndex[CBIOS_IGACOUNTS] = {0x81D4, 0x339D8, 0x340D8, 0x347D8};
static    CBIOS_U32  SSStrideIndex[CBIOS_IGACOUNTS] = {0x81D8, 0x339DC, 0x340DC, 0x347DC};
static    CBIOS_U32  SSFmtSrcWIndex[CBIOS_IGACOUNTS] = {0x8190, 0x339C0, 0x340C0, 0x347C0};
static    CBIOS_U32  SSDstWinIndex[CBIOS_IGACOUNTS] = {0x819C, 0x339CC, 0x340CC, 0x347CC};
static    CBIOS_U32  SSSrcWinHIndex[CBIOS_IGACOUNTS] = {0x81A8, 0x339D0, 0x340D0, 0x347D0};
static    CBIOS_U32  SSDstWinPosIndex[CBIOS_IGACOUNTS] = {0x81F8, 0x339E8, 0x340E8, 0x347E8};
static    CBIOS_U32  SSScalRatioHIndex[CBIOS_IGACOUNTS] = {0x332E0, 0x339F4, 0x340F4, 0x347F4};
static    CBIOS_U32  SSScalRatioVIndex[CBIOS_IGACOUNTS] = {0x332E4, 0x339F8, 0x340F8, 0x347F8};
static    CBIOS_U32  SSCscIndex[CBIOS_IGACOUNTS] = {0x3362C, 0x33A10, 0x34110, 0x34810};
static    CBIOS_U32  SSCscCoef1[CBIOS_IGACOUNTS] = {0x3361c, 0x33a00, 0x34100, 0x34800};
static    CBIOS_U32  SSCscCoef2[CBIOS_IGACOUNTS] = {0x33620, 0x33a04, 0x34104, 0x34804};
static    CBIOS_U32  SSCscCoef3[CBIOS_IGACOUNTS] = {0x33624, 0x33a08, 0x34108, 0x34808};
static    CBIOS_U32  SSCscCoef4[CBIOS_IGACOUNTS] = {0x33628, 0x33a0c, 0x3410c, 0x3480c};
static    CBIOS_U32  SSBLIndex[CBIOS_IGACOUNTS] = {0x33854, 0x33a24, 0x34124, 0x34824};

static    CBIOS_U32  TQSStartAddrIndex[2][CBIOS_IGACOUNTS] = {{0x8430, 0x33A40, 0x34140, 0x34840},
{0x33404, 0x33AF4, 0x341F4, 0x348F4}};
static    CBIOS_U32  TQSCompFifoIndex[2][CBIOS_IGACOUNTS] = {{0x8414, 0x33A3C, 0x3413C, 0x3483C},
{0x33400, 0x33AF0, 0x341F0, 0x348F0}};
static    CBIOS_U32  TQSBaseOffsetIndex[2][CBIOS_IGACOUNTS] = {{0x8498, 0x33A78, 0x34178, 0x34878},
{0x33410, 0x33B00, 0x34200, 0x34900}};
static    CBIOS_U32  TQSEnableIndex[2][CBIOS_IGACOUNTS] = {{0x8458, 0x33A5C, 0x3415C, 0x3485C},
{0x3384C, 0x33B8C, 0x3428C, 0x3498C}};
static    CBIOS_U32  TQSRightBaseIndex[2][CBIOS_IGACOUNTS] = {{0x844C, 0x33A54, 0x34154, 0x34854},
{0x33408, 0x33AF8, 0x341F8, 0x348F8}};
static    CBIOS_U32  TQSStrideIndex[2][CBIOS_IGACOUNTS] = {{0x8438, 0x33A44, 0x34144, 0x34844},
{0x3340C, 0x33AFC, 0x341FC, 0x348FC}};
static    CBIOS_U32  TQSFormatIndex[2][CBIOS_IGACOUNTS] = {{0x33600, 0x33ABC, 0x341BC, 0x348BC},
{0x33480, 0x33B64, 0x34264, 0x34964}};
static    CBIOS_U32  TQSDstWinPosIndex[2][CBIOS_IGACOUNTS] = {{0x8444, 0x33A4C, 0x3414C, 0x3484C},
{0x33418, 0x33B08, 0x34208, 0x34908}};
static    CBIOS_U32  TQSDstWinIndex[2][CBIOS_IGACOUNTS] = {{0x8450, 0x33A58, 0x34158, 0x34858},
{0x3341C, 0x33B0C, 0x3420C, 0x3490C}};
static    CBIOS_U32  TQSSrcWinIndex[2][CBIOS_IGACOUNTS] = {{0x8448, 0x33A50, 0x34150, 0x34850},
{0x33414, 0x33B04, 0x34204, 0x34904}};
static    CBIOS_U32  TQSScalRatioHIndex[2][CBIOS_IGACOUNTS] = {{0x84A4, 0x33A84, 0x34184, 0x34884},
{0x33420, 0x33B10, 0x34210, 0x34910}};
static    CBIOS_U32  TQSScalRatioVIndex[2][CBIOS_IGACOUNTS] = {{0x84A8, 0x33A88, 0x34188, 0x34888},
{0x33424, 0x33B14, 0x34214, 0x34914}};
static    CBIOS_U32  TQSCscIndex[2][CBIOS_IGACOUNTS] = {{0x33614, 0x33AD0, 0x341D0, 0x348D0},
{0x33494, 0x33B78, 0x34278, 0x34978}};
static    CBIOS_U32  TQSCscCoef1[2][CBIOS_IGACOUNTS] = {{0x33604, 0x33ac0, 0x341c0, 0x348c0},
{0x33484, 0x33b68, 0x34268, 0x34968}};
static    CBIOS_U32  TQSCscCoef2[2][CBIOS_IGACOUNTS] = {{0x33608, 0x33ac4, 0x341c4, 0x348c4},
{0x33488, 0x33b6c, 0x3426c, 0x3496c}};
static    CBIOS_U32  TQSCscCoef3[2][CBIOS_IGACOUNTS] = {{0x3360c, 0x33ac8, 0x341c8, 0x348c8},
{0x3348c, 0x33b70, 0x34270, 0x34970}};
static    CBIOS_U32  TQSCscCoef4[2][CBIOS_IGACOUNTS] = {{0x33610, 0x33acc, 0x341cc, 0x348cc},
{0x33490, 0x33b74, 0x34274, 0x34974}};
static    CBIOS_U32  TQSBLIndex[2][CBIOS_IGACOUNTS] = {{0x33858, 0x33ad4, 0x341d4, 0x348d4},
{0x33860, 0x33b84, 0x34284, 0x34984}};


static    CBIOS_U32  BackgndColorIndex[CBIOS_IGACOUNTS] = {0x33670, 0x339A4, 0x340A4, 0x347A4};
static    CBIOS_U32  OneShotTrigIndex[CBIOS_IGACOUNTS] = {0x8200, 0x33958, 0x34058, 0x34758};

CBIOS_U32  OvlKeyIndex[CBIOS_IGACOUNTS][OVL_NUM_E3K] = {
                         {0x33840, 0x8194, 0x8410, 0x332BC},
                         {0x33E80, 0x339C4, 0x33A38, 0x33AE8},
                         {0x34590, 0x340C4, 0x34138, 0x341E8},
                         {0x34C90, 0x347C4, 0x34838, 0x348E8},
};

static    CBIOS_U32  OvlPlaneAlphaIndex[CBIOS_IGACOUNTS][OVL_NUM_E3K] = {
                         {0x33844, 0x332A0, 0x332A4, 0x332C0},
                         {0x33E84, 0x339EC, 0x33A98, 0x33AEC},
                         {0x34594, 0x340EC, 0x34198, 0x341EC},
                         {0x34C94, 0x347EC, 0x34898, 0x348EC},
};

static CBIOS_U32 KeyHighIndex[CBIOS_IGACOUNTS][STREAM_NUM_E3K] =
{
    {0x8188, 0x332E8, 0x843C, 0x33498,},
    {0x3397C, 0x33A20, 0x33A48, 0x33B7C,},
    {0x3407C, 0x34120, 0x34148, 0x3427C,},
    {0x3477C, 0x34820, 0x34848, 0x3497C},
};

static CBIOS_U32 KeyLowIndex[CBIOS_IGACOUNTS][STREAM_NUM_E3K] =
{
    {0x81AC, 0x8184, 0x8400, 0x3342C,},
    {0x33980, 0x339BC, 0x33A34, 0x33B1C,},
    {0x34080, 0x340BC, 0x34134, 0x3421C,},
    {0x34780, 0x347BC, 0x34834, 0x3491C},
};

static CBIOS_U32 CursorCscIndex[CBIOS_IGACOUNTS] = {0x81e4, 0x33bb0, 0x342b0, 0x349b0};
static CBIOS_U32 CursorControl1Index[CBIOS_IGACOUNTS] = {0x33718, 0x33bb4, 0x342b4, 0x349b4};
static CBIOS_U32 CursorControl2Index[CBIOS_IGACOUNTS] = {0x3371c, 0x33bb8, 0x342b8, 0x349b8};
static CBIOS_U32 CursorBaseAddrIndex[CBIOS_IGACOUNTS] = {0x33720, 0x33bbc, 0x342bc, 0x349bc};
//static CBIOS_U32 CursorRightFrameBaseAddrIndex[CBIOS_IGACOUNTS] = {0x33724, 0x33bc0, 0x342c0, 0x349c0};
static CBIOS_U32 CursorEndPixelIndex[CBIOS_IGACOUNTS] = {0x33728, 0x33bc4, 0x342c4, 0x349c4};

static CBIOS_U16 LUTBitWidthIndex[CBIOS_IGACOUNTS] = {CR_E4, CR_E5, CR_E6, CR_B_CD};

//HDMI support table
//CBIOS_TRUE: support, CBIOS_FALSE: not support
CBIOS_BOOL HDMIFormatSupportTable[] =
{
    CBIOS_TRUE,     //640,  480, p, 5994,6000
    CBIOS_TRUE,     //720,  480, p, 5994,6000
    CBIOS_TRUE,     //720,  480, p, 5994,6000
    CBIOS_TRUE,     //1280,  720, p, 5994,6000
    CBIOS_TRUE,     //5,  1920, 1080, i, 5994,6000
    CBIOS_TRUE,     //6,   720,  480, i, 5994,6000
    CBIOS_TRUE,     //7,   720,  480, i, 5994,6000
    CBIOS_TRUE,     //8,   720,  240, p, 5994,6000
    CBIOS_TRUE,     //9,   720,  240, p, 5994,6000
    CBIOS_FALSE,    //10, 2880,  480, i, 5994,6000
    CBIOS_FALSE,    //11, 2880,  480, i, 5994,6000
    CBIOS_FALSE,    //12, 2880,  240, p, 5994,6000
    CBIOS_FALSE,    //13, 2880,  240, p, 5994,6000
    CBIOS_FALSE,    //14, 1440,  480, p, 5994,6000
    CBIOS_FALSE,    //15, 1440,  480, p, 5994,6000
    CBIOS_TRUE,     //16, 1920, 1080, p, 5994,6000
    CBIOS_TRUE,     //17,  720,  576, p, 5000,0000
    CBIOS_TRUE,     //18,  720,  576, p, 5000,0000
    CBIOS_TRUE,     //19, 1280,  720, p, 5000,0000
    CBIOS_TRUE,     //20, 1920, 1080, i, 5000,0000
    CBIOS_TRUE,     //21,  720,  576, i, 5000,0000
    CBIOS_TRUE,     //22,  720,  576, i, 5000,0000
    CBIOS_TRUE,     //23,  720,  288, p, 5000,0000
    CBIOS_TRUE,     //24,  720,  288, p, 5000,0000
    CBIOS_FALSE,    //25, 2880,  576, i, 5000,0000
    CBIOS_FALSE,    //26, 2880,  576, i, 5000,0000
    CBIOS_FALSE,    //27, 2880,  288, p, 5000,0000
    CBIOS_FALSE,    //28, 2880,  288, p, 5000,0000
    CBIOS_FALSE,    //29, 1440,  576, p, 5000,0000
    CBIOS_FALSE,    //30, 1440,  576, p, 5000,0000
    CBIOS_TRUE,     //31, 1920, 1080, p, 5000,0000
    CBIOS_TRUE,     //32, 1920, 1080, p, 2397,2400
    CBIOS_TRUE,     //33, 1920, 1080, p, 2500,0000
    CBIOS_TRUE,     //34, 1920, 1080, p, 2997,3000
    CBIOS_FALSE,    //35, 2880,  480, p, 5994,6000
    CBIOS_FALSE,    //36, 2880,  480, p, 5994,6000
    CBIOS_FALSE,    //37, 2880,  576, p, 5000,0000
    CBIOS_FALSE,    //38, 2880,  576, p, 5000,0000
    CBIOS_FALSE,    //39, 1920, 1080, i, 5000,0000
    CBIOS_FALSE,    //40, 1920, 1080, i, 10000,0000
    CBIOS_FALSE,    //41, 1280,  720, p, 10000,0000
    CBIOS_FALSE,    //42,  720,  576, p, 10000,0000
    CBIOS_FALSE,    //43,  720,  576, p, 10000,0000
    CBIOS_FALSE,    //44,  720,  576, i, 10000,0000
    CBIOS_FALSE,    //45,  720,  576, i, 10000,0000
    CBIOS_FALSE,    //46, 1920, 1080, i, 11988,12000
    CBIOS_FALSE,    //47, 1280,  720, p, 11988,12000
    CBIOS_FALSE,    //48,  720,  480, p, 11988,12000
    CBIOS_FALSE,    //49,  720,  480, p, 11988,12000
    CBIOS_FALSE,    //50,  720,  480, i, 11988,12000
    CBIOS_FALSE,    //51,  720,  480, i, 11988,12000
    CBIOS_FALSE,    //52,  720,  576, p, 20000,0000
    CBIOS_FALSE,    //53,  720,  576, p, 20000,0000
    CBIOS_FALSE,    //54,  720,  576, i, 20000,0000
    CBIOS_FALSE,    //55,  720,  576, i, 20000,0000
    CBIOS_FALSE,    //56,  720,  480, p, 23976,24000
    CBIOS_FALSE,    //57,  720,  480, p, 23976,24000
    CBIOS_FALSE,    //58,  720,  480, i, 23976,24000
    CBIOS_FALSE,    //59,  720,  480, i, 23976,24000
    CBIOS_FALSE,    //60   1280, 720, p,  2398, 2400
    CBIOS_FALSE,    //61,  1280, 720, p,  2500, 0000
    CBIOS_FALSE,    //62,  1280, 720, p,  2997, 3000
    CBIOS_FALSE,    //63,  1920,1080, p, 11988,12000
    CBIOS_FALSE,    //64,  1920,1080, p, 10000, 0000
    CBIOS_FALSE,    //65,  1280, 720, p,  2398, 2400
    CBIOS_FALSE,    //66,  1280, 720, p,  2500, 0000
    CBIOS_FALSE,    //67   1280, 720, p,  3000, 2997
    CBIOS_FALSE,    //68,  1280, 720, p,  5000, 0000
    CBIOS_FALSE,    //69,  1280, 720, p,  5994, 6000
    CBIOS_FALSE,    //70,  1280, 720, p, 10000, 0000
    CBIOS_FALSE,    //71,  1280, 720, p, 11988,12000
    CBIOS_FALSE,    //72,  1920,1080, p,  2398, 2400
    CBIOS_FALSE,    //73,  1920,1080, p,  2500, 0000
    CBIOS_FALSE,    //74,  1920,1080, p,  3000, 2997
    CBIOS_FALSE,    //75,  1920,1080, p,  5000, 0000
    CBIOS_FALSE,    //76,  1920,1080, p,  5994, 6000
    CBIOS_FALSE,    //77,  1920,1080, p, 10000, 0000
    CBIOS_FALSE,    //78,  1920,1080, p, 11988,12000
    CBIOS_FALSE,    //79,  1680, 720, p,  2398, 2400
    CBIOS_FALSE,    //80,  1680, 720, p,  2500, 0000
    CBIOS_FALSE,    //81   1680, 720, p,  3000, 2997
    CBIOS_FALSE,    //82,  1680, 720, p,  5000, 0000
    CBIOS_FALSE,    //83,  1680, 720, p,  5994, 6000
    CBIOS_FALSE,    //84,  1680, 720, p, 10000, 0000
    CBIOS_FALSE,    //85,  1680, 720, p, 11988,12000
    CBIOS_FALSE,    //86,  2560,1080, p,  2398, 2400
    CBIOS_FALSE,    //87,  2560,1080, p,  2500, 0000
    CBIOS_FALSE,    //88,  2560,1080, p,  3000, 2997
    CBIOS_FALSE,    //89,  2560,1080, p,  5000, 0000
    CBIOS_FALSE,    //90,  2560,1080, p,  5994, 6000
    CBIOS_FALSE,    //91,  2560,1080, p, 10000, 0000
    CBIOS_FALSE,    //92,  2560,1080, p, 11988,12000
    CBIOS_TRUE,     //93,  3840,2160, p,  2398, 2400
    CBIOS_TRUE,     //94,  3840,2160, p,  2500, 0000
    CBIOS_TRUE,     //95,  3840,2160, p,  3000, 2997
    CBIOS_TRUE,    //96,  3840,2160, p,  5000, 0000
    CBIOS_TRUE,     //97,  3840,2160, p, 5994, 6000
    CBIOS_TRUE,     //98,  3840,2160, p,  2398, 2400
    CBIOS_TRUE,    //99,  3840,2160, p,  2500, 0000
    CBIOS_TRUE,    //100, 3840,2160, p,  3000, 2997
    CBIOS_TRUE,    //101, 3840,2160, p,  5000, 0000
    CBIOS_TRUE,    //102, 3840,2160, p,  5994, 6000
    CBIOS_TRUE,     //103, 3840,2160, p,  2398, 2400
    CBIOS_TRUE,     //104, 3840,2160, p,  2500, 0000
    CBIOS_TRUE,     //105, 3840,2160, p,  3000, 2997
    CBIOS_TRUE,    //106, 3840,2160, p,  5000, 0000
    CBIOS_TRUE,     //107, 3840,2160, p,  5994, 6000
    // VSDB VIC
    CBIOS_TRUE,     //108, 3840,2160, p,  3000, 0000
    CBIOS_TRUE,     //109, 3840,2160, p,  2500, 0000
    CBIOS_TRUE,     //110, 3840,2160, p,  2400, 0000
    CBIOS_TRUE,     //111, 4096,2160, p,  2400, 0000
};


/******** The following tables are for HDTV encoders ********/
/*
static CBREGISTER_IDX TriLevelSyncWidth_INDEX[] = {
    {SR_B_71,0x3F},
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX BlankLevel_INDEX[] = {
    {SR_B_72,0xFF},
    {SR_B_73,0x03},
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX SyncDelayReg_INDEX[] = {
    {SR_B_7E,0xff},
    {SR_B_7D,0x07},
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX HSyncToHActive_INDEX[] = {
    {SR_B_8A,0xFF},
    {SR_B_89,0x07},
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX HSyncWidth_INDEX[] = {
    {SR_B_A3, 0xFF},
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX HSyncDelay_INDEX[] = {
    {SR_B_AA,0x07},
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX BroadPulseReg_INDEX[] = {
    {SR_B_D0,0x7F},
    {SR_B_D1,0xFF},
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX HalfSyncReg_INDEX[] = {
    {SR_B_D2,0xFF},
    {SR_B_D3,0x07},
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX HDTVHDEReg_INDEX[] = {
    {SR_B_E4,0xFF},
    {SR_B_E5,0x07},
    {MAPMASK_EXIT},
};
*/
static CBREGISTER_IDX HSS2Reg_INDEX[] = {
    {SR_64,0xff},   //SR64[7:0]
    {SR_66,0x08},   //SR66[3]
    {SR_67,0x08},   //SR67[3]
    {SR_2E,0x40},   //SR2E[6]
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX HT2Reg_INDEX[] = {
    {SR_60,0xff},   //SR60[7:0]
    {SR_66,0x01},   //SR66[0]
    {SR_67,0x01},   //SR67[0]
    {SR_2E,0x01},   //SR2E[0]
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX HSE2Reg_INDEX[] = {
    {SR_65,0x3f},   //SR65[5:0]
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX HDE2Reg_INDEX[] = {
    {SR_61,0xff},   //SR61[7:0]
    {SR_66,0x02},   //SR66[1]
    {SR_67,0x02},   //SR67[1]
    {SR_2E,0x02},   //SR2E[1]
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX HBS2Reg_INDEX[] = {
    {SR_62,0xff},   //SR62[7:0]
    {SR_66,0x04},   //SR66[2]
    {SR_67,0x04},   //SR67[2]
    {SR_2E,0x04},   //SR2E[2]
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX HBE2Reg_INDEX[] = {
    {SR_63,0x7f},   //SR63[6:0]
    {SR_2E,0x38},   //SR2E[5:3]
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX VT2Reg_INDEX[] = {
    {SR_68,0xff},   //SR68[7:0]
    {SR_6E,0x0f},   //SR6E[3:0]
    {SR_2F,0X01},   //SR2F[0]
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX VDE2Reg_INDEX[] = {
    {SR_69,0xff},   //SR69[7:0]
    {SR_6E,0xf0},   //SR6E[7:4]
    {SR_2F,0X02},   //SR2F[1]
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX VBS2Reg_INDEX[] = {
    {SR_6A,0xFF},   //SR6A[7:0]
    {SR_6F,0x0f},   //SR6F[3:0]
    {SR_2F,0X04},   //SR2F[2]
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX VBE2Reg_INDEX[] = {
    {SR_6B,0xff},   //SR6B[7:0]
    {SR_2F,0X08},   //SR2F[3]
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX VSS2Reg_INDEX[] = {
    {SR_6C,0xff},   //SR6C[7:0]
    {SR_6F,0xf0},   //SR6F[7:4]
    {SR_2F,0X10},   //SR2F[4]
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX VSE2Reg_INDEX[] = {
    {SR_6D,0x0f},   //SR6D[3:0]
    {SR_2F,0x60},   //SR2F[6:5]
    {MAPMASK_EXIT},
};

static CBREGISTER_IDX HorSyncStartReg_INDEX[] = {
    {    CR_04,0xFF     }, //CR04[7:0]
    {    CR_5D,0x10     }, //CR5D[4]
    {    CR_5F, 0x08    }, //CR5F[3]
    {    CR_74, 0x40    }, //CR74[6]
    {    MAPMASK_EXIT},
};

static CBREGISTER_IDX HorTotalReg_INDEX[] = {
    {    CR_00, 0xFF    }, //CR00[7:0]
    {    CR_5D, 0x01    }, //CR5D[0]
    {    CR_5F, 0x01    }, //CR5F[0]
    {    CR_74, 0x01    }, //CR74[0]
    {    MAPMASK_EXIT},
};

static CBREGISTER_IDX HorSyncEndReg_INDEX[] = {
    {    CR_05, 0x1F    }, //CR05[4:0]
    {    CR_5D, 0x20    }, //CR5D[5]
    {    MAPMASK_EXIT},
};

static CBREGISTER_IDX HorDisEndReg_INDEX[] = {
    {    CR_01, 0xFF    }, //CR01[7:0]
    {    CR_5D, 0x02    }, //CR5D[1]
    {    CR_5F, 0x02    }, //CR5F[1]
    {    CR_74, 0x02    }, //CR74[1]
    {    MAPMASK_EXIT},
};

static CBREGISTER_IDX HorBStartReg_INDEX[] = {
    {    CR_02, 0xFF    }, //CR02[7:0]
    {    CR_5D, 0x04    }, //CR5D[2]
    {    CR_5F, 0x04    }, //CR5F[2]
    {    CR_74, 0x04    }, //CR74[2]
    {    MAPMASK_EXIT},
};

static CBREGISTER_IDX HorBEndReg_INDEX[] = {
    {    CR_03, 0x1F    }, //CR03[4:0]
    {    CR_05, 0x80    }, //CR05[7]
    {    CR_5D, 0x08    }, //CR5D[3]
    {    CR_74, 0x38    }, //CR74[5:3]
    {    MAPMASK_EXIT},
};

static CBREGISTER_IDX VerTotalReg_INDEX[] = {
    {    CR_06, 0xFF    }, //CR06[7:0]
    {    CR_07, 0x21    }, //CR07[0], CR07[5]
    {    CR_63, 0xC0    }, //CR63[7:6]
    {    CR_79, 0x04    }, //CR79[2]
    {    MAPMASK_EXIT},
};

static CBREGISTER_IDX VerDisEndReg_INDEX[] = {
    {    CR_12, 0xFF    }, //CR12[7:0]
    {    CR_07, 0x42    }, //CR07[1], CR07[6]
    {    CR_5E, 0x03    }, //CR5E[1:0]
    {    CR_79, 0x08    }, //CR79[3]
    {    MAPMASK_EXIT},
};
static CBREGISTER_IDX VerBStartReg_INDEX[] = {
    {    CR_15, 0xFF    }, //CR15[7:0]
    {    CR_07, 0x08    }, //CR07[3]
    {    CR_09, 0x20    }, //CR09[5]
    {    CR_5E, 0x0C    }, //CR5E[3:2]
    {    CR_79, 0x10    }, //CR79[4]
    {    MAPMASK_EXIT},
};

static CBREGISTER_IDX VerBEndReg_INDEX[] = {
    {    CR_16, 0xFF    }, //CR16[7:0]
    {    CR_79, 0x20    }, //CR79[5]
    {    MAPMASK_EXIT},
};

static CBREGISTER_IDX VerSyncStartReg_INDEX[] = {
    {    CR_10, 0xFF    }, //CR10[7:0]
    {    CR_07, 0x84    }, //CR07[2], CR07[7]
    {    CR_5E, 0x30    }, //CR5E[5:4]
    {    CR_79, 0x01    }, //CR79[0]
    {    MAPMASK_EXIT},
};

static CBREGISTER_IDX VerSyncEndReg_INDEX[] = {
    {    CR_11, 0x0F    }, //CR11[3:0]
    {    CR_79, 0x42    }, //CR79[1], CR79[6]
    {    MAPMASK_EXIT},
};

extern CBIOS_U32 HDTV_REG_LB[4];

CBIOS_VOID cbInitChipAttribute_Arise(PCBIOS_EXTENSION_COMMON pcbe)
{
    PCBIOS_CHIP_FUNC_TABLE pFuncTbl = &(pcbe->ChipFuncTbl);

    //init chip dependent functions
    pFuncTbl->pfncbGetModeInfoFromReg = (PFN_cbGetModeInfoFromReg)cbGetModeInfoFromReg_Arise;
    pFuncTbl->pfncbInitVCP = (PFN_cbInitVCP)cbInitVCP_Arise;
    pFuncTbl->pfncbDoHDTVFuncSetting = (PFN_cbDoHDTVFuncSetting)cbDoHDTVFuncSetting_Arise;
    pFuncTbl->pfncbLoadSSC = CBIOS_NULL;
    pFuncTbl->pfncbEnableSpreadSpectrum = CBIOS_NULL;
    pFuncTbl->pfncbDisableSpreadSpectrum = CBIOS_NULL;
    pFuncTbl->pfncbSetGamma = (PFN_cbSetGamma)cbSetGamma_Arise;
    pFuncTbl->pfncbInterruptEnableDisable = (PFN_cbInterruptEnableDisable)cbInterruptEnableDisable_Arise;
    pFuncTbl->pfncbCECEnableDisable = (PFN_cbCECEnableDisable)cbCECEnableDisable_Arise;
    pFuncTbl->pfncbCheckSurfaceOnDisplay = (PFN_cbCheckSurfaceOnDisplay)cbCheckSurfaceOnDisplay_Arise;
    pFuncTbl->pfncbGetDispAddr = (PFN_cbGetDispAddr)cbGetDispAddr_Arise;

    pFuncTbl->pfncbSetHwCursor = (PFN_cbSetHwCursor)cbSetHwCursor_Arise;
    pFuncTbl->pfncbSetCRTimingReg = (PFN_cbSetTimingReg)cbSetCRTimingReg_Arise;
    pFuncTbl->pfncbSetSRTimingReg = (PFN_cbSetTimingReg)cbSetSRTimingReg_Arise;
    pFuncTbl->pfncbDoCSCAdjust = (PFN_cbDoCSCAdjust)cbDoCSCAdjust_Arise;
    pFuncTbl->pfncbAdjustStreamCSC = (PFN_cbAdjustStreamCSC)cbAdjustStreamCSC_Arise;

    pFuncTbl->pfncbUpdateFrame = (PFN_cbUpdateFrame)cbUpdateFrame_Arise;
    pFuncTbl->pfncbDisableStream = (PFN_cbDisableStream)cbDisableStream_Arise;
    //tables
    pcbe->pHDMISupportedFormatTable = HDMIFormatSupportTable;
}

CBIOS_VOID cbSetSRTimingReg_Arise(PCBIOS_EXTENSION_COMMON pcbe,
                                PCBIOS_TIMING_ATTRIB pTiming,
                                CBIOS_U32 IGAIndex,
                                CBIOS_TIMING_FLAGS Flags)
{
    CBIOS_U32   ulBlankingTime = 0,ulSyncWidth= 0, ulBackPorchWidth = 0, ulSyncStart2BlankEnd = 0;
    CBIOS_TIMING_REG_Arise   TimingReg;
    CBIOS_U8 Value = 0x80;

    cb_memset(&TimingReg, 0, sizeof(CBIOS_TIMING_REG_Arise));
    //================================================================//
    //************Start Setting Horizontal Timing Registers***********//
    //================================================================//

    //(1) Adjust (H Bottom/Right Border + H Front Porch) period: CR72[2:0] = (Hor sync start position in pixel)%8
    TimingReg.HSSRemainder = pTiming->HorSyncStart % 8;
    cbBiosMMIOWriteReg(pcbe, CR_72, TimingReg.HSSRemainder, 0xF8, IGAIndex);

    //patch the timing no sync width
    if(pTiming->HorSyncEnd <= pTiming->HorSyncStart)
    {
        pTiming->HorSyncEnd = pTiming->HorSyncStart + 1;
    }

    //(2) Adjust HSYNC period: CR72[5:3] = (the number of pixel in Hor sync time)%8
    ulSyncWidth = pTiming->HorSyncEnd - pTiming->HorSyncStart;
    TimingReg.HSyncRemainder = ulSyncWidth % 8;
    ulSyncWidth = cbRound(ulSyncWidth, CELLGRAN, ROUND_DOWN);
    cbBiosMMIOWriteReg(pcbe, CR_72, (TimingReg.HSyncRemainder) << 3, 0xC7, IGAIndex);

    //(3) Adjust (H Back Porch + H Top/Left Border) period: CR73[2:0] = (the number of pixel in (Hor back porch + H top/left border))%8
    ulBackPorchWidth = pTiming->HorTotal - pTiming->HorSyncEnd;
    TimingReg.HBackPorchRemainder = ulBackPorchWidth % 8;
    cbBiosMMIOWriteReg(pcbe, CR_73, TimingReg.HBackPorchRemainder, 0xF8, IGAIndex);
    if((ulBackPorchWidth > 25) && (ulBackPorchWidth < 32))
    {
        cbBiosMMIOWriteReg(pcbe, CR_59, 0x01, 0x0, IGAIndex);//set H DisplayShift to 1, to resolve display skew to left
        cbBiosMMIOWriteReg(pcbe, CR_5B, 0x08, ~0x08, IGAIndex);//Enable Display Shift
    }

    //(4) Start Horizontal Sync Position = HSync Start(in pixel)/8
    TimingReg.HorSyncStart = (CBIOS_U16)cbRound(pTiming->HorSyncStart, CELLGRAN, ROUND_DOWN);
    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.HorSyncStart, HSS2Reg_INDEX, IGAIndex);

    //(5) Htotal = (number of character clocks in one scan line) - 5 - (CR72[2:0]+CR72[5:3]+CR73[2:0])/8
    TimingReg.HorTotal = (CBIOS_U16)cbRound(pTiming->HorTotal, CELLGRAN, ROUND_DOWN) - 5;
    TimingReg.HorTotal -= (CBIOS_U16)cbRound(TimingReg.HSSRemainder + TimingReg.HSyncRemainder +
                                             TimingReg.HBackPorchRemainder, CELLGRAN, ROUND_DOWN);
    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.HorTotal, HT2Reg_INDEX, IGAIndex);

    //(6) End Horizontal Sync Position = {Start Horizontal Sync Position + Hsync time(in pixel)/8}[5:0]
    TimingReg.HorSyncEnd = (CBIOS_U16)cbRound(pTiming->HorSyncEnd, CELLGRAN, ROUND_DOWN);
    if (TimingReg.HorSyncEnd > TimingReg.HorTotal + 3)
    {
        TimingReg.HorSyncEnd = TimingReg.HorTotal + 3;
    }
    TimingReg.HorSyncEnd &= 0x1F;
    if(ulSyncWidth > 32)
    {
        TimingReg.HorSyncEnd |= 0x20;//sr65[5]set when Hsynwidth>32char
    }
    else
    {
        TimingReg.HorSyncEnd &= ~0x20;//sr65[5]set when Hsynwidth>32char
    }
    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.HorSyncEnd, HSE2Reg_INDEX, IGAIndex);


    //(7) Adjust HDE:CR58[2:0] = (the number of pixel for 1 line of active display)%8 - 1
    TimingReg.HDERemainder = pTiming->HorDisEnd % 8;
    if (TimingReg.HDERemainder != 0)
    {
        TimingReg.HDERemainder = (TimingReg.HDERemainder - 1) & 0x07;
        cbBiosMMIOWriteReg(pcbe, CR_58, TimingReg.HDERemainder, 0xF8, IGAIndex);
        //enable HDE adjust
        cbBiosMMIOWriteReg(pcbe, CR_58, 0x08, 0xF7, IGAIndex);
    }
    else
    {
        //disable HDE adjust
        cbBiosMMIOWriteReg(pcbe, CR_58, 0x00, 0xF0, IGAIndex);
    }


    //(8) Horizontal Display End = (the number of pixel for 1 line of active display)/8 - 1, rounded up to the nearest integer.
    TimingReg.HorDisEnd = (CBIOS_U16)cbRound(pTiming->HorDisEnd, CELLGRAN, ROUND_UP) - 1;
    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.HorDisEnd, HDE2Reg_INDEX, IGAIndex);

    //(9) Horizontal Blank Start
    TimingReg.HorBStart = (CBIOS_U16)cbRound(pTiming->HorBStart, CELLGRAN, ROUND_UP);
    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.HorBStart, HBS2Reg_INDEX, IGAIndex);

    //(10) Horizontal Blank End
    TimingReg.HorBEnd = (CBIOS_U16)cbRound(pTiming->HorBEnd, CELLGRAN, ROUND_DOWN);
    if (TimingReg.HorBEnd > TimingReg.HorTotal + 3)
    {
        TimingReg.HorBEnd = TimingReg.HorTotal + 3;
    }

    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.HorBEnd, HBE2Reg_INDEX, IGAIndex);

    //================================================================//
    //************Start Setting Vertical Timing Registers*************//
    //================================================================//
    TimingReg.VerTotal = pTiming->VerTotal - 2;//VerTotalTime-2
    cbMapMaskWrite(pcbe,(CBIOS_U32) TimingReg.VerTotal,VT2Reg_INDEX, IGAIndex);

    TimingReg.VerDisEnd = pTiming->VerDisEnd - 1;//HorAddrTime-1
    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.VerDisEnd, VDE2Reg_INDEX, IGAIndex);

    TimingReg.VerBStart = pTiming->VerBStart - 1;//VerBlankStart
    //According to spec, both CR15 and SR6A need -1, move the -1 patch to cbConvertVesaTimingToInternalTiming_dst
    //Add this patch for HDMI to pass golden file check. Later SE will help measure the output timing for HDMI
    // to see whether this patch is still needed.
    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.VerBStart, VBS2Reg_INDEX, IGAIndex);

    ulBlankingTime = pTiming->VerBEnd - pTiming->VerBStart;
    TimingReg.VerBEnd = TimingReg.VerBStart + ulBlankingTime;//(VerBlankStart+VerBlankTime) and 00FFh
    if (TimingReg.VerBEnd > TimingReg.VerTotal)
    {
        TimingReg.VerBEnd = TimingReg.VerTotal;
    }

    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.VerBEnd, VBE2Reg_INDEX, IGAIndex);

    //According to spec, SR6C[7-0] = [scan line counter value at which the vertical sync pulse (FLM) becomes active] -1
    //CR10 needn't -1
    TimingReg.VerSyncStart = pTiming->VerSyncStart - 1;//VerSyncStart
    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.VerSyncStart, VSS2Reg_INDEX, IGAIndex);

    //patch the timing no sync width
    if(pTiming->VerSyncEnd <= pTiming->VerSyncStart)
    {
        pTiming->VerSyncEnd = pTiming->VerSyncStart + 1;
    }

    //SR6D = SR6C+VerSyncTime
    ulSyncWidth = pTiming->VerSyncEnd - pTiming->VerSyncStart;
    TimingReg.VerSyncEnd = TimingReg.VerSyncStart + ulSyncWidth; //(VerSyncStart+VerSyncTime) and 000Fh
    if (TimingReg.VerSyncEnd > TimingReg.VerTotal)
    {
        TimingReg.VerSyncEnd = TimingReg.VerTotal;
    }
    TimingReg.VerSyncEnd &= 0x3F;
    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.VerSyncEnd, VSE2Reg_INDEX, IGAIndex);

    //patch the timing which vsyncwidth add vbackporch is less than 8
    ulSyncStart2BlankEnd = pTiming->VerBEnd - pTiming->VerSyncStart;
    if(ulSyncStart2BlankEnd < 8 && ulSyncStart2BlankEnd >= 4)
    {
        Value = 0x40;
    }
    else if(ulSyncStart2BlankEnd < 4)
    {
        Value = 0x00;
    }

    if(IGAIndex == IGA1)
    {
        cb_WriteU8(pcbe->pAdapterContext, CB_CRT_ADDR_REG, 0x8f);
        cb_WriteU8(pcbe->pAdapterContext, CB_CRT_DATA_REG, Value);
    }
    else
    {
        cbBiosMMIOWriteReg(pcbe, CR_8F, Value, (CBIOS_U8)~0xc0, IGAIndex);
    }

    //Fix Arise1020/2030 VGA Hsync Timing issue
    if(pcbe->DispMgr.ActiveDevices[IGAIndex] == CBIOS_TYPE_CRT)
    {
        cbMMIOWriteReg(pcbe, SR_18, 0x80, 0x7F);
    }
}


CBIOS_VOID cbSetCRTimingReg_Arise(PCBIOS_EXTENSION_COMMON pcbe,
                                PCBIOS_TIMING_ATTRIB pTiming,
                                CBIOS_U32 IGAIndex,
                                CBIOS_TIMING_FLAGS Flags)
{
    CBIOS_U32   ulBlankingTime = 0,ulSyncWidth= 0,ulBackPorchWidth = 0;
    CBIOS_TIMING_REG_Arise  TimingReg;

    //patch for unlocking CR: CR is locked  for some reason, so unlock CR here temporarily
    cbBiosMMIOWriteReg(pcbe, CR_11, 0, 0x7f, IGAIndex);

    cb_memset(&TimingReg, 0, sizeof(CBIOS_TIMING_REG_Arise));

    //================================================================//
    //************Start Setting Horizontal Timing Registers***********//
    //================================================================//

    ulSyncWidth = cbRound(pTiming->HorSyncEnd - pTiming->HorSyncStart, CELLGRAN, ROUND_DOWN);
    TimingReg.HorSyncStart = (CBIOS_U16)cbRound(pTiming->HorSyncStart, CELLGRAN, ROUND_UP);



    //=========================Set HorSyncStart: CR04, CR5D, CR5F===================//
    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.HorSyncStart, HorSyncStartReg_INDEX, IGAIndex);
    //===================================================================//


    TimingReg.HorTotal = (CBIOS_U16)cbRound(pTiming->HorTotal, CELLGRAN, ROUND_UP) - 5;


    //========================Set HorTotal: CR00, CR5D, CR5F=====================//
    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.HorTotal, HorTotalReg_INDEX, IGAIndex);
    //==================================================================//


    TimingReg.HorSyncEnd = (CBIOS_U16)cbRound(pTiming->HorSyncEnd, CELLGRAN, ROUND_DOWN);
    if (TimingReg.HorSyncEnd > TimingReg.HorTotal + 3)
    {
        TimingReg.HorSyncEnd = TimingReg.HorTotal + 3;
    }
    TimingReg.HorSyncEnd &= 0x1F;
    if(ulSyncWidth > 32)
    {
        TimingReg.HorSyncEnd |= 0x20;//sr65[5]set when Hsynwidth>32char
    }
    else
    {
        TimingReg.HorSyncEnd &= ~0x20;//sr65[5]set when Hsynwidth>32char
    }


    //=============================Set HorSyncEnd: CR05, CR5D======================//
    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.HorSyncEnd, HorSyncEndReg_INDEX, IGAIndex);
    //======================================================================//

    ulBackPorchWidth = pTiming->HorTotal - pTiming->HorSyncEnd;
    if ((ulBackPorchWidth < 32) && (ulBackPorchWidth % 8 != 0))
    {
        TimingReg.HorDisEnd = (CBIOS_U16)cbRound(pTiming->HorDisEnd, CELLGRAN, ROUND_UP);
    }
    else
    {
        TimingReg.HorDisEnd = (CBIOS_U16)cbRound(pTiming->HorDisEnd, CELLGRAN, ROUND_UP) - 1;
    }

    //=======================Set HorDisEnd: CR01, CR5D, CR5F=======================//
    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.HorDisEnd, HorDisEndReg_INDEX, IGAIndex);
    //====================================================================//


    TimingReg.HorBStart = (CBIOS_U16)cbRound(pTiming->HorBStart, CELLGRAN, ROUND_UP);

    //=====================Set HorBStart: CR02, CR5D, CR5F========================//
    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.HorBStart, HorBStartReg_INDEX, IGAIndex);
    //===================================================================//

    TimingReg.HorBEnd = (CBIOS_U16)cbRound(pTiming->HorBEnd, CELLGRAN, ROUND_DOWN);

    if (TimingReg.HorBEnd > TimingReg.HorTotal + 3)
    {
        TimingReg.HorBEnd = TimingReg.HorTotal + 3;
    }
    TimingReg.HorBEnd &= 0x3F;
    if(ulBlankingTime > 64)
    {
        TimingReg.HorBEnd |= 0x40;
    }
    else
    {
        TimingReg.HorBEnd &= ~0x40;
    }

    //==================Set HorBEnd: CR03, CR05, CR5D=========================//
    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.HorBEnd, HorBEndReg_INDEX, IGAIndex);
    //================================================================//



    //================================================================//
    //************Start Setting Vertical Timing Registers*************//
    //================================================================//
    TimingReg.VerTotal = pTiming->VerTotal - 2;//VerTotalTime-2


    //======================Set VerTotal: CR06, CR07, CR63======================//
    cbMapMaskWrite(pcbe, (CBIOS_U32) TimingReg.VerTotal, VerTotalReg_INDEX, IGAIndex);
    //================================================================//


    TimingReg.VerDisEnd = pTiming->VerDisEnd - 1;//HorAddrTime-1

    //====================Set VerDisEnd: CR12, CR07, CR5E==========================//
    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.VerDisEnd, VerDisEndReg_INDEX, IGAIndex);
    //====================================================================//



    TimingReg.VerBStart = pTiming->VerBStart - 1;//VerBlankStart
    //According to spec, both CR15 and SR6A need -1, move the -1 patch to cbConvertVesaTimingToInternalTiming_dst
    //Add this patch for HDMI to pass golden file check. Later SE will help measure the output timing for HDMI
    // to see whether this patch is still needed.
    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.VerBStart, VerBStartReg_INDEX, IGAIndex);

    ulBlankingTime = pTiming->HorBEnd - pTiming->HorBStart;
    TimingReg.VerBEnd = TimingReg.VerBStart + ulBlankingTime;//(VerBlankStart+VerBlankTime) and 00FFh
    if (TimingReg.VerBEnd > TimingReg.VerTotal)
    {
        TimingReg.VerBEnd = TimingReg.VerTotal;
    }
    TimingReg.VerBEnd &= 0xFF;
    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.VerBEnd, VerBEndReg_INDEX, IGAIndex);

    //According to spec, SR6C[7-0] = [scan line counter value at which the vertical sync pulse (FLM) becomes active] -1
    //CR10 needn't -1
    TimingReg.VerSyncStart = pTiming->VerSyncStart;//VerSyncStart
    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.VerSyncStart, VerSyncStartReg_INDEX, IGAIndex);

    //SR6D = SR6C+VerSyncTime
    ulSyncWidth = pTiming->VerSyncEnd - pTiming->VerSyncStart;
    TimingReg.VerSyncEnd = TimingReg.VerSyncStart + ulSyncWidth; //(VerSyncStart+VerSyncTime) and 000Fh
    if (TimingReg.VerSyncEnd > TimingReg.VerTotal)
    {
        TimingReg.VerSyncEnd = TimingReg.VerTotal;
    }
    TimingReg.VerSyncEnd &= 0x3F;
    cbMapMaskWrite(pcbe, (CBIOS_U32)TimingReg.VerSyncEnd, VerSyncEndReg_INDEX, IGAIndex);

}

CBIOS_VOID cbGetCRTimingReg_Arise(PCBIOS_EXTENSION_COMMON pcbe,
                                PCBIOS_TIMING_ATTRIB pTiming,
                                CBIOS_U32 IGAIndex,
                                PCBIOS_TIMING_FLAGS pFlags)
{
    CBIOS_U32 temp = 0;
    CBIOS_U8 ClockType = CBIOS_DCLK1TYPE;
    CBIOS_U32 CurrentFreq = 0;
    CBIOS_U32   remainder = 0;

    cb_memset(pTiming, 0, sizeof(CBIOS_TIMING_ATTRIB));
    cb_memset(pFlags, 0, sizeof(CBIOS_TIMING_FLAGS));

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
    cbGetProgClock(pcbe, &CurrentFreq, ClockType);
    pTiming->PLLClock = CurrentFreq;

    //================================================================//
    //************Start Getting Horizontal Timing Registers***********//
    //================================================================//
    pTiming->HorTotal = (CBIOS_U16)cbMapMaskRead(pcbe, HorTotalReg_INDEX, IGAIndex); //Horizontal Total
    pTiming->HorTotal += 5;
    pTiming->HorTotal *= 8;

    pTiming->HorDisEnd = (CBIOS_U16)cbMapMaskRead(pcbe, HorDisEndReg_INDEX, IGAIndex); //Horizontal Display End
    pTiming->HorDisEnd++;
    pTiming->HorDisEnd *= 8;

    pTiming->HorBStart = (CBIOS_U16)cbMapMaskRead(pcbe, HorBStartReg_INDEX, IGAIndex); //Horizontal Blank Start
    pTiming->HorBStart *= 8;

    temp = cbMapMaskRead(pcbe, HorBEndReg_INDEX, IGAIndex); //Horizontal Blank End
    pTiming->HorBEnd =  (temp & 0x3F) | ((pTiming->HorBStart/8) & ~0x3F);
    if (temp & BIT6)
    {
        pTiming->HorBEnd += 0x40;
    }
    if (pTiming->HorBEnd <= (pTiming->HorBStart/8))
    {
        pTiming->HorBEnd += 0x40;
    }
    if (pTiming->HorBEnd >= (pTiming->HorTotal/8) - 2)
    {
        pTiming->HorBEnd += 2;
    }
    pTiming->HorBEnd *= 8;

    pTiming->HorSyncStart = (CBIOS_U16)cbMapMaskRead(pcbe, HorSyncStartReg_INDEX, IGAIndex); //Horizontal Sync Start
    pTiming->HorSyncStart *= 8;

    temp = cbMapMaskRead(pcbe, HorSyncEndReg_INDEX, IGAIndex); //Horizontal Sync End
    pTiming->HorSyncEnd =  (temp & 0x1F) | ((pTiming->HorSyncStart/8) & ~0x1F);
    if (temp & BIT5)
    {
        pTiming->HorSyncEnd += 0x20;
    }
    if (pTiming->HorSyncEnd <= (pTiming->HorSyncStart/8))
    {
        pTiming->HorSyncEnd += 0x20;
    }
    pTiming->HorSyncEnd *= 8;

    //================================================================//
    //************Start Getting Vertical Timing Registers*************//
    //================================================================//
    pTiming->VerTotal = (CBIOS_U16)cbMapMaskRead(pcbe, VerTotalReg_INDEX, IGAIndex); //Vertical Total
    pTiming->VerTotal +=2;

    pTiming->VerDisEnd = (CBIOS_U16)cbMapMaskRead(pcbe, VerDisEndReg_INDEX, IGAIndex); //Vertical Display End
    pTiming->VerDisEnd++;

    pTiming->VerBStart = (CBIOS_U16)cbMapMaskRead(pcbe, VerBStartReg_INDEX, IGAIndex); //Vertical Blank Start
    pTiming->VerBStart++;

    temp = cbMapMaskRead(pcbe, VerBEndReg_INDEX, IGAIndex); //Vertical Blank End
    temp++;

    pTiming->VerBEnd = (temp & 0xFF) | (pTiming->VerBStart & ~(0xFF));
    if (pTiming->VerBEnd <= pTiming->VerBStart )
    {
        pTiming->VerBEnd += 0x100;
    }
    if(pTiming->VerBEnd >= pTiming->VerTotal - 2)
    {
        pTiming->VerBEnd += 1;
    }

    pTiming->VerSyncStart = (CBIOS_U16)cbMapMaskRead(pcbe, VerSyncStartReg_INDEX, IGAIndex); //Vertical Sync Start

    temp = cbMapMaskRead(pcbe, VerSyncEndReg_INDEX, IGAIndex); //Vertical Sync End
    pTiming->VerSyncEnd = (temp & 0x0F) | (pTiming->VerSyncStart & ~(0x0F));
    if (pTiming->VerSyncEnd <= pTiming->VerSyncStart)
    {
        pTiming->VerSyncEnd += 0x10;
    }

    //calculate the result value
    pTiming->XRes = pTiming->HorDisEnd;
    pTiming->YRes = pTiming->VerDisEnd;

    temp = (CBIOS_U32)pTiming->HorTotal * (CBIOS_U32)pTiming->VerTotal;
    pTiming->RefreshRate = pTiming->PLLClock * 100 / temp;
    remainder = (pTiming->PLLClock * 100) % temp;
    pTiming->RefreshRate = pTiming->RefreshRate * 100 + remainder * 100 / temp;
}

CBIOS_VOID cbGetSRTimingReg_Arise(PCBIOS_EXTENSION_COMMON pcbe,
                                PCBIOS_TIMING_ATTRIB pTiming,
                                CBIOS_U32 IGAIndex,
                                PCBIOS_TIMING_FLAGS pFlags)
{
    CBIOS_U32 temp = 0;
    CBIOS_U8 ClockType = CBIOS_DCLK1TYPE;
    CBIOS_U32 CurrentFreq = 0;
    CBIOS_U32   remainder = 0;
    CBIOS_U32 HSSRemainder = 0;
    CBIOS_U32 HSyncRemainder = 0;
    CBIOS_U32 HBackPorchRemainder = 0;
    CBIOS_U32 HDisEndRemainder = 0;
    CBIOS_U32 HorTotalReg = 0;

    cb_memset(pTiming, 0, sizeof(CBIOS_TIMING_ATTRIB));
    cb_memset(pFlags, 0, sizeof(CBIOS_TIMING_FLAGS));

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

    cbGetProgClock(pcbe, &CurrentFreq, ClockType);
    pTiming->PLLClock = CurrentFreq;

    //================================================================//
    //************Start Getting Horizontal Timing Registers***********//
    //================================================================//
    HSSRemainder = cbBiosMMIOReadReg(pcbe, CR_72, IGAIndex) & 0x07;
    temp = cbBiosMMIOReadReg(pcbe, CR_72, IGAIndex);
    HSyncRemainder = (temp >> 3) & 0x07;
    HBackPorchRemainder = cbBiosMMIOReadReg(pcbe, CR_73, IGAIndex) & 0x07;

    temp = cbBiosMMIOReadReg(pcbe, CR_58, IGAIndex);
    if(temp & BIT3)
    {
        HDisEndRemainder = (temp & 0x07) + 1;
    }

    pTiming->HorSyncStart = (CBIOS_U16)cbMapMaskRead(pcbe, HSS2Reg_INDEX, IGAIndex); //Horizontal Sync Start
    pTiming->HorSyncStart *= 8;
    pTiming->HorSyncStart += (CBIOS_U16)HSSRemainder;

    pTiming->HorTotal = (CBIOS_U16)cbMapMaskRead(pcbe, HT2Reg_INDEX, IGAIndex); //Horizontal Total
    HorTotalReg = pTiming->HorTotal;
    pTiming->HorTotal = (pTiming->HorTotal + 5) * 8 + (CBIOS_U16)(HSSRemainder + HSyncRemainder + HBackPorchRemainder);


    temp = cbMapMaskRead(pcbe, HSE2Reg_INDEX, IGAIndex); //Horizontal Sync End
    pTiming->HorSyncEnd =  (temp & 0x1F) | ((pTiming->HorSyncStart/8) & ~(0x1F));
    if (temp & BIT5)
    {
        pTiming->HorSyncEnd += 0x20;
    }
    if (pTiming->HorSyncEnd <= (pTiming->HorSyncStart/8))
    {
        pTiming->HorSyncEnd += 0x20;
    }
    pTiming->HorSyncEnd *= 8;
    pTiming->HorSyncEnd += (CBIOS_U16)(HSyncRemainder + HSSRemainder);

    pTiming->HorDisEnd = (CBIOS_U16)cbMapMaskRead(pcbe, HDE2Reg_INDEX, IGAIndex); //Horizontal Display End
    //rounded up to the nearest integer
    if(HDisEndRemainder)
    {
        pTiming->HorDisEnd = pTiming->HorDisEnd  * 8;
    }
    else
    {
        pTiming->HorDisEnd = (pTiming->HorDisEnd + 1) * 8;
    }
    pTiming->HorDisEnd += (CBIOS_U16)HDisEndRemainder;

    pTiming->HorBStart = (CBIOS_U16)cbMapMaskRead(pcbe, HBS2Reg_INDEX, IGAIndex); //Horizontal Blank Start
    pTiming->HorBStart *= 8;

    pTiming->HorBEnd = (CBIOS_U16)cbMapMaskRead(pcbe, HBE2Reg_INDEX, IGAIndex); //Horizontal Blank End
    if (pTiming->HorBEnd >= HorTotalReg + 3)
    {
        pTiming->HorBEnd += 0x02;
        pTiming->HorBEnd *= 8;
        pTiming->HorBEnd += (CBIOS_U16)(HSSRemainder + HSyncRemainder + HBackPorchRemainder);
    }
    else
    {
        pTiming->HorBEnd *= 8;
    }

    //================================================================//
    //************Start Getting Vertical Timing Registers*************//
    //================================================================//
    pTiming->VerTotal = (CBIOS_U16)cbMapMaskRead(pcbe, VT2Reg_INDEX, IGAIndex); //Vertical Total
    pTiming->VerTotal +=2;

    pTiming->VerDisEnd = (CBIOS_U16)cbMapMaskRead(pcbe, VDE2Reg_INDEX, IGAIndex); //Vertical Display End
    pTiming->VerDisEnd++;

    pTiming->VerBStart = (CBIOS_U16)cbMapMaskRead(pcbe, VBS2Reg_INDEX, IGAIndex); //Vertical Blank Start
    pTiming->VerBStart++;

    temp = cbMapMaskRead(pcbe, VBE2Reg_INDEX, IGAIndex); //Vertical Blank End
    temp++;
    pTiming->VerBEnd = (temp & 0xFF) | (pTiming->VerBStart & ~(0xFF));
    if (pTiming->VerBEnd <= pTiming->VerBStart )
    {
        pTiming->VerBEnd += 0x100;
    }
    if(pTiming->VerBEnd >= pTiming->VerTotal - 2)
    {
        pTiming->VerBEnd += 1;
    }

    pTiming->VerSyncStart = (CBIOS_U16)cbMapMaskRead(pcbe, VSS2Reg_INDEX, IGAIndex); //Vertical Sync Start
    pTiming->VerSyncStart++;

    temp = cbMapMaskRead(pcbe, VSE2Reg_INDEX, IGAIndex); //Vertical Sync End
    temp++;
    pTiming->VerSyncEnd = (temp & 0x0F) | (pTiming->VerSyncStart & ~(0x0F));
    if (pTiming->VerSyncEnd <= pTiming->VerSyncStart)
    {
        pTiming->VerSyncEnd += 0x10;
    }

    //calculate the result value
    pTiming->XRes = pTiming->HorDisEnd;
    pTiming->YRes = pTiming->VerDisEnd;

    temp = (CBIOS_U32)pTiming->HorTotal * (CBIOS_U32)pTiming->VerTotal;
    pTiming->RefreshRate = pTiming->PLLClock * 100 / temp;
    remainder = (pTiming->PLLClock * 100) % temp;
    pTiming->RefreshRate = pTiming->RefreshRate * 100 + remainder * 100 / temp;
}


CBIOS_VOID cbGetModeInfoFromReg_Arise(PCBIOS_EXTENSION_COMMON pcbe,
                                CBIOS_ACTIVE_TYPE ulDevice,
                                PCBIOS_TIMING_ATTRIB pTiming,
                                PCBIOS_TIMING_FLAGS pFlags,
                                CBIOS_U32  IGAIndex,
                                CBIOS_TIMING_REG_TYPE TimingRegType)
{
    REG_SR70_B    RegSR70_BValue;
    REG_MM33694   RegMM33694;


    if(TIMING_REG_TYPE_CR == TimingRegType)
    {
        cbGetCRTimingReg_Arise(pcbe, pTiming, IGAIndex, pFlags);
    }
    else if(TIMING_REG_TYPE_SR == TimingRegType)
    {
        cbGetSRTimingReg_Arise(pcbe, pTiming, IGAIndex, pFlags);
    }
    else
    {
        cbGetSRTimingReg_Arise(pcbe, pTiming, IGAIndex, pFlags);
    }

    // judge if timing mode is interlaced or not
    switch (ulDevice)
    {
    case CBIOS_TYPE_CRT:
    case CBIOS_TYPE_DVO:
        pFlags->IsInterlace = 0;
        break;
    case CBIOS_TYPE_HDTV:
        RegSR70_BValue.Value = 0;
        RegSR70_BValue.Value = cbMMIOReadReg(pcbe, SR_B_70);
        if (RegSR70_BValue.Progressive_Mode_Enable == 0)
        {
            pFlags->IsInterlace = 1;
        }
        else
        {
            pFlags->IsInterlace = 0;
        }
        break;
    case CBIOS_TYPE_DP1:
    case CBIOS_TYPE_DP2:
    case CBIOS_TYPE_DP3:
    case CBIOS_TYPE_DP4:
        RegMM33694.Value = cb_ReadU32(pcbe->pAdapterContext, HDTV_REG_LB[IGAIndex]);
        if (RegMM33694.LB1_BYPASS == 0) // HDTV source
        {
            RegSR70_BValue.Value = 0;
            RegSR70_BValue.Value = cbMMIOReadReg(pcbe, SR_B_70);
            if (0)//RegSR70_BValue.Progressive_Mode_Enable == 0) //can't find this define in e3k register spec, just comment interlace mode
            {
                pFlags->IsInterlace = 1;
            }
            else
            {
                pFlags->IsInterlace = 0;
            }
        }
        else
        {
            pFlags->IsInterlace = 0;
        }
        break;
    case CBIOS_TYPE_TV:
        pFlags->IsInterlace = 1;
        break;

    default:
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "unknown device type!\n"));
        return;
    }
}

CBIOS_STATUS cbInterruptEnableDisable_Arise(PCBIOS_EXTENSION_COMMON pcbe,PCBIOS_INT_ENABLE_DISABLE_PARA pIntPara)
{
    CBIOS_U32 Status = CBIOS_OK;
    CBIOS_U32 RegValue = 0;
    REG_MM8508 RegMM8508Value;
    REG_MM8508 RegMM8508Mask;
    REG_MM854C RegMM854CValue;
    REG_MM854C RegMM854CMask;

    RegMM8508Value.Value = 0;
    RegMM8508Mask.Value = 0xFFFFFFFF;
    RegMM854CValue.Value = 0;
    RegMM854CMask.Value = 0xFFFFFFFF;

    if (pIntPara->bEnableInt)
    {
       RegValue = 1;
    }
    else
    {
       RegValue = 0;
    }

    if (pIntPara->bUpdAllInt)
    {
        RegMM8508Value.Value = (pIntPara->bEnableInt) ? 0xFFFFFFFF : 0;
        RegMM8508Mask.Value = 0;
        // Enable/Disable all BIU interrupts
        cbMMIOWriteReg32(pcbe, 0x8508, RegMM8508Value.Value, RegMM8508Mask.Value);
    }
    else
    {
        if (pIntPara->IntFlags & CBIOS_VSYNC_1_INT)
        {
            RegMM8508Value.VSYNC1_INT_EN = RegValue;
            RegMM8508Mask.VSYNC1_INT_EN = 0;
        }
        if (pIntPara->IntFlags & CBIOS_VSYNC_2_INT)
        {
            RegMM8508Value.VSYNC2_INT_EN = RegValue;
            RegMM8508Mask.VSYNC2_INT_EN = 0;
        }
        if (pIntPara->IntFlags & CBIOS_VSYNC_3_INT)
        {
            RegMM8508Value.VSYNC3_INT_EN = RegValue;
            RegMM8508Mask.VSYNC3_INT_EN = 0;
        }
        if (pIntPara->IntFlags & CBIOS_VSYNC_4_INT)
        {
            RegMM8508Value.VSYNC4_INT_EN = RegValue;
            RegMM8508Mask.VSYNC4_INT_EN = 0;
        }
        if ((pIntPara->IntFlags & CBIOS_DP_1_INT) && (pcbe->DeviceMgr.SupportDevices & CBIOS_TYPE_DP1))
        {
            RegMM8508Value.DP1_INT_EN = RegValue;
            RegMM8508Mask.DP1_INT_EN = 0;
        }
        if ((pIntPara->IntFlags & CBIOS_DP_2_INT) && (pcbe->DeviceMgr.SupportDevices & CBIOS_TYPE_DP2))
        {
            RegMM8508Value.DP2_INT_EN = RegValue;
            RegMM8508Mask.DP2_INT_EN = 0;
        }
        if ((pIntPara->IntFlags & CBIOS_DP_3_INT) && (pcbe->DeviceMgr.SupportDevices & CBIOS_TYPE_DP3))
        {
            RegMM8508Value.DP3_INT_EN = RegValue;
            RegMM8508Mask.DP3_INT_EN = 0;
        }
        if ((pIntPara->IntFlags & CBIOS_DP_4_INT) && (pcbe->DeviceMgr.SupportDevices & CBIOS_TYPE_DP4))
        {
            RegMM8508Value.DP4_INT_EN = RegValue;
            RegMM8508Mask.DP4_INT_EN = 0;
        }
        if (pIntPara->IntFlags & CBIOS_HDCP_INT)
        {
            RegMM8508Value.HDCP_INT_EN = RegValue;
            RegMM8508Mask.HDCP_INT_EN = 0;
        }
        if (pIntPara->IntFlags & CBIOS_HDA_CODEC_INT)
        {
            RegMM8508Value.INT_VIP_INT_EN = RegValue;
            RegMM8508Mask.INT_VIP_INT_EN = 0;

            // Arise1020 use mb INT bit as CODEC INT
            RegMM8508Value.INT_MB_INT_EN = RegValue;
            RegMM8508Mask.INT_MB_INT_EN = 0;
        }
        /*
        if (pIntPara->IntFlags & CBIOS_VCP_TIMEOUT_INT)
        {
            RegMM8508Value.VCP_TIMEOUT_INT_EN = RegValue;
            RegMM8508Mask.VCP_TIMEOUT_INT_EN = 0;
        }
        if(pIntPara->IntFlags & CBIOS_MSVD_TIMEOUT_INT)
        {
            RegMM8508Value.MSVD_TIMEOUT_INT_EN = RegValue;
            RegMM8508Mask.MSVD_TIMEOUT_INT_EN = 0;
        }*/

        // Enable/Disable specified BIU interrupts
        cbMMIOWriteReg32(pcbe, 0x8508, RegMM8508Value.Value, RegMM8508Mask.Value);
    }

    if (pIntPara->bUpdAllAdvInt)
    {
        RegMM854CValue.Value = (pIntPara->bEnableInt) ? 0xFFFFFFFF : 0;
        RegMM854CMask.Value = 0;
        // Enable/Disable all Advanced interrupts
        cbMMIOWriteReg32(pcbe, 0x854C, RegMM854CValue.Value, RegMM854CMask.Value);
    }
    else
    {
        if (pIntPara->AdvancedIntFlags & CBIOS_FENCE_INT)
        {
            RegMM854CValue.FENCE_CMD_INT_EN = RegValue;
            RegMM854CMask.FENCE_CMD_INT_EN = 0;
        }
        /*
        if (pIntPara->AdvancedIntFlags & CBIOS_PAGE_FAULT_INT)
        {
            RegMM854CValue.Page_Fault_Int = RegValue;
            RegMM854CMask.Page_Fault_Int = 0;
        }
        if (pIntPara->AdvancedIntFlags & CBIOS_MXU_INVALID_ADDR_FAULT_INT)
        {
            RegMM854CValue.MXU_Invalid_Address_Fault_Int = RegValue;
            RegMM854CMask.MXU_Invalid_Address_Fault_Int = 0;
        }
        if (pIntPara->AdvancedIntFlags & CBIOS_MSVD0_INT)
        {
            RegMM854CValue.VCP_cmd_Int = RegValue;
            RegMM854CMask.VCP_cmd_Int = 0;
        }
        if (pIntPara->AdvancedIntFlags & CBIOS_MSVD1_INT)
        {
            RegMM854CValue.Dump_cmd_Int = RegValue;
            RegMM854CMask.Dump_cmd_Int = 0;
        }*/

        // Enable/Disable specified advanced interrupts
        cbMMIOWriteReg32(pcbe, 0x854C, RegMM854CValue.Value, RegMM854CMask.Value);
    }

    if(pIntPara->bEnableInt)
    {
       //Enable PCIE bus interrupts report. CRA0_C=0x01
       cbMMIOWriteReg(pcbe, CR_C_A0, 0x01, ~0x01);
    }
    else if(pIntPara->bUpdAllAdvInt && pIntPara->bUpdAllInt)
    {
        cbMMIOWriteReg(pcbe, CR_C_A0, 0x00, ~0x01);
    }

    return Status;
}

CBIOS_STATUS cbCheckSurfaceOnDisplay_Arise(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_CHECK_SURFACE_ON_DISP pChkSurfacePara)
{
    CBIOS_STREAM_ATTRIBUTE  StreamAttr = {0};
    CBIOS_STREAM_TP         StreamType = pChkSurfacePara->StreamType;
    CBIOS_U32               IGAIndex = pChkSurfacePara->IGAIndex;
    CBIOS_U64           PhyAddr = 0;
    CBIOS_U32               BaseOffset = 0;

    CBIOS_U32               AddrRegValue = 0;
    REG_MM81DC_Arise          PSStartAddrValue = {0};
    REG_MM81D0_Arise          SSStartAddrValue = {0};

    CBIOS_U32               BaseOffsetRegValue = 0;
    REG_MM33700_Arise         PSBaseOffsetValue = {0};
    REG_MM33708_Arise         SSBaseOffsetValue = {0};

    CBIOS_BOOL              bOnDisplay = CBIOS_TRUE;

    CheckCnt++;

    if(pChkSurfacePara->bChkAfterEnable)
    {
        StreamAttr.pSurfaceAttr = pChkSurfacePara->pSurfaceAttr;
        StreamAttr.pSrcWinPara    = pChkSurfacePara->pSrcWindow;
        cbGetStreamAttribute(&StreamAttr);
        PhyAddr = StreamAttr.pSurfaceAttr->StartAddr;
        BaseOffset = StreamAttr.dwBaseOffset;
    }
    else
    {
        PhyAddr = STREAM_DISABLE_FAKE_ADDR;
        BaseOffset = 0;
    }

    if(StreamType == CBIOS_STREAM_PS)
    {
        PSStartAddrValue.Value = cb_ReadU32(pcbe->pAdapterContext, PSStartAddrIndex[IGAIndex]);
        PSBaseOffsetValue.Value = cb_ReadU32(pcbe->pAdapterContext, PSBaseOffsetIndex[IGAIndex]);
        AddrRegValue = PSStartAddrValue.PS1_Start_Address;
        BaseOffsetRegValue = PSBaseOffsetValue.Base_Offset;
    }
    else if(StreamType == CBIOS_STREAM_SS)
    {
        SSStartAddrValue.Value = cb_ReadU32(pcbe->pAdapterContext, SSStartAddrIndex[IGAIndex]);
        SSBaseOffsetValue.Value = cb_ReadU32(pcbe->pAdapterContext, SSBaseOffsetIndex[IGAIndex]);
        AddrRegValue = SSStartAddrValue.SS_Start_Address0;
        BaseOffsetRegValue = SSBaseOffsetValue.SS_Base_Offset;
    }
    else if(StreamType == CBIOS_STREAM_TS)
    {
        SSStartAddrValue.Value = cb_ReadU32(pcbe->pAdapterContext, TQSStartAddrIndex[0][IGAIndex]);
        SSBaseOffsetValue.Value = cb_ReadU32(pcbe->pAdapterContext, TQSBaseOffsetIndex[0][IGAIndex]);
        AddrRegValue = SSStartAddrValue.SS_Start_Address0;
        BaseOffsetRegValue = SSBaseOffsetValue.SS_Base_Offset;
    }
    else
    {
        SSStartAddrValue.Value = cb_ReadU32(pcbe->pAdapterContext, TQSStartAddrIndex[1][IGAIndex]);
        SSBaseOffsetValue.Value = cb_ReadU32(pcbe->pAdapterContext, TQSBaseOffsetIndex[1][IGAIndex]);
        AddrRegValue = SSStartAddrValue.SS_Start_Address0;
        BaseOffsetRegValue = SSBaseOffsetValue.SS_Base_Offset;
    }

    if(AddrRegValue != (PhyAddr >> 5) ||
        BaseOffsetRegValue != (BaseOffset >> 5))
    {
        bOnDisplay = CBIOS_FALSE;
    }

    pChkSurfacePara->bOnDisplay = bOnDisplay;

    if(CheckCnt <= 6)
    {
        if(pChkSurfacePara->pSrcWindow)
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC,DEBUG), "Check %d: Addr=%x,Pitch=%d,FixedAddr=%x, OnDisp=%d.\n\n",
                                    StreamType, pChkSurfacePara->pSurfaceAttr->StartAddr, pChkSurfacePara->pSurfaceAttr->Pitch,
                                    PhyAddr, bOnDisplay));
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC,DEBUG), "Check %d(disable): Addr=%x, OnDisp=%d.\n\n", StreamType,
                                    pChkSurfacePara->pSurfaceAttr->StartAddr, bOnDisplay));
        }
    }

    return  CBIOS_OK;
}

CBIOS_STATUS  cbGetDispAddr_Arise(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_GET_DISP_ADDR  pGetDispAddr)
{
    CBIOS_STREAM_TP         StreamType = pGetDispAddr->StreamType;
    CBIOS_U32               IGAIndex = pGetDispAddr->IGAIndex;
    CBIOS_U64               AddrRegValue = 0;
    REG_MM81DC_Arise          PSStartAddrValue = {0};
    REG_MM81D0_Arise          SSStartAddrValue = {0};

    if(StreamType == CBIOS_STREAM_PS)
    {
        PSStartAddrValue.Value = cb_ReadU32(pcbe->pAdapterContext, PSStartAddrIndex[IGAIndex]);
        AddrRegValue = PSStartAddrValue.PS1_Start_Address;
    }
    else if(StreamType == CBIOS_STREAM_SS)
    {
        SSStartAddrValue.Value = cb_ReadU32(pcbe->pAdapterContext, SSStartAddrIndex[IGAIndex]);
        AddrRegValue = SSStartAddrValue.SS_Start_Address0;
    }
    else if(StreamType == CBIOS_STREAM_TS)
    {
        SSStartAddrValue.Value = cb_ReadU32(pcbe->pAdapterContext, TQSStartAddrIndex[0][IGAIndex]);
        AddrRegValue = SSStartAddrValue.SS_Start_Address0;
    }
    else
    {
        SSStartAddrValue.Value = cb_ReadU32(pcbe->pAdapterContext, TQSStartAddrIndex[1][IGAIndex]);
        AddrRegValue = SSStartAddrValue.SS_Start_Address0;
    }

    pGetDispAddr->DispAddr = AddrRegValue << 5;


    return  CBIOS_OK;
}

CBIOS_BOOL  cbUpdateOverlay_Arise(PCBIOS_EXTENSION_COMMON pcbe,
                                     CBIOS_U32  IGAIndex,
                                     CBIOS_U32  OvlIndex,
                                     CBIOS_STREAM_TP  StreamType,
                                     PCBIOS_OVERLAY_INFO  pOverlayInfo,
                                     PCBIOS_FLIP_MODE  pFlipMode)
{
    REG_MM33670_Arise  BackgndValue = {0};
    REG_MM33840_Arise  OvlKeyValue = {0};
    REG_MM33844_Arise  OvlPlaneAlphaValue = {0};
    CBIOS_U32  KeyMode = 0;
    CBIOS_U8  Kp = 0, Ks = 0;

    if(OvlIndex == CBIOS_OVERLAY0)
    {
        BackgndValue.Backgnd_Color_Value = 0; // black background
        BackgndValue.Backgnd_Color_Ycbcr = 0;
    }

    if(pFlipMode->FlipType == CBIOS_PLANE_FLIP_WITH_DISABLE)
    {
        KeyMode = 0;
        Kp = 8;
        Ks = 0;
    }
    else
    {
        KeyMode = cbGetHwColorKey(pOverlayInfo, &Ks, &Kp);

        //invert alpha
        if((pOverlayInfo->KeyMode == CBIOS_CONSTANT_ALPHA && pOverlayInfo->ConstantAlphaBlending.bInvertAlpha) ||
            (pOverlayInfo->KeyMode == CBIOS_ALPHA_BLENDING && pOverlayInfo->AlphaBlending.bInvertAlpha))
        {
            OvlKeyValue.Invert_Alpha_Or_Ka = 1;
        }

        //pre-multi
        if(pOverlayInfo->KeyMode == CBIOS_ALPHA_BLENDING && pOverlayInfo->AlphaBlending.bPremulBlend)
        {
            OvlPlaneAlphaValue.Ovl0_Alpha_Blend_Mode = 1;
        }

        //plane alpha
        if(pOverlayInfo->KeyMode == CBIOS_ALPHA_BLENDING)
        {
            OvlPlaneAlphaValue.Ovl0_Plane_Alpha =
                pOverlayInfo->AlphaBlending.bUsePlaneAlpha ? pOverlayInfo->AlphaBlending.PlaneValue : 0xff;
        }
        if(pOverlayInfo->KeyMode == CBIOS_CONSTANT_ALPHA)
        {
            OvlPlaneAlphaValue.Ovl0_Plane_Alpha =
                pOverlayInfo->ConstantAlphaBlending.bUsePlaneAlpha ? pOverlayInfo->ConstantAlphaBlending.PlaneValue : 0xff;
        }

        //mix alpha
        if(pOverlayInfo->KeyMode == CBIOS_ALPHA_BLENDING)
        {
            OvlKeyValue.Alpha_Select = pOverlayInfo->AlphaBlending.AlphaMixType;
        }

        //color key select
        if(pOverlayInfo->KeyMode == CBIOS_COLOR_KEY)
        {
            OvlKeyValue.Color_Key_Sel = pOverlayInfo->ColorKey.ColorKeyType;
        }
    }

    OvlKeyValue.Ka_3to0_Or_Ks = Ks;
    OvlKeyValue.Ka_7to4_Or_Kp = Kp;
    OvlKeyValue.Key_Mode = KeyMode;
    OvlKeyValue.Ovl0_Input_Stream = StreamType;

    if(OvlIndex == CBIOS_OVERLAY0)
    {
        cbMMIOWriteReg32(pcbe, BackgndColorIndex[IGAIndex], BackgndValue.Value, 0);
    }

    cbMMIOWriteReg32(pcbe, OvlKeyIndex[IGAIndex][OvlIndex], OvlKeyValue.Value, 0);
    cbMMIOWriteReg32(pcbe, OvlPlaneAlphaIndex[IGAIndex][OvlIndex], OvlPlaneAlphaValue.Value, 0);

    return  CBIOS_TRUE;
}


/*
 *  flip mode is set by the corresponding stream
 *  so flip mode is no need in the parameters' list
 */
CBIOS_BOOL  cbUpdateOVLKey_Arise(PCBIOS_EXTENSION_COMMON pcbe,
                                     CBIOS_U32  IGAIndex,
                                     CBIOS_OVL_KEY_INFO *pInfo)
{
    REG_MM8188_Arise PSKeyHighValue = {0};
    REG_MM81AC_Arise  PSKeyLowValue = {0};

    if  (CBIOS_OVL_KEY_TYPE_ALPHA == pInfo->Type)
    {
        PSKeyLowValue.PS_B_Cb_Low_Or_Alpha_Key = pInfo->AlphaKey;
        PSKeyHighValue.Value = 0;
    }
    else if (CBIOS_OVL_KEY_TYPE_COLOR == pInfo->Type)
    {
        PSKeyLowValue.Value = pInfo->ColorKey;
        PSKeyHighValue.Value = 0;
    }
    else if (CBIOS_OVL_KEY_TYPE_CHROME == pInfo->Type)
    {
        PSKeyLowValue.Value = pInfo->ChromeKey.LowColor;
        PSKeyHighValue.Value = pInfo->ChromeKey.UpColor;
    }
    else
    {
        return CBIOS_FALSE;
    }

    cbMMIOWriteReg32(pcbe, KeyHighIndex[IGAIndex][pInfo->Index], PSKeyHighValue.Value, 0);
    cbMMIOWriteReg32(pcbe, KeyLowIndex[IGAIndex][pInfo->Index], PSKeyLowValue.Value, 0);

    return CBIOS_TRUE;
}


CBIOS_BOOL cbUpdatePrimaryStream_Arise(PCBIOS_EXTENSION_COMMON pcbe,
                                     CBIOS_U32 IGAIndex,
                                     PCBIOS_STREAM_PARA pStreamPara,
                                     PCBIOS_FLIP_MODE  pFlipMode)
{
    PCBIOS_SURFACE_ATTRIB    pSurfaceAttrib = CBIOS_NULL;
    PCBIOS_DISP_MODE_PARAMS  pModeParams = pcbe->DispMgr.pModeParams[IGAIndex];
    CBIOS_U64                ullStartAddress = 0;
    CBIOS_U32                dwBitCnt = 0, dwFetchCount = 0, DacColorMode = 0;
    PCBIOS_WINDOW_PARA   pSrcWin = CBIOS_NULL, pDstWin = CBIOS_NULL;
    CBIOS_STREAM_ATTRIBUTE   StreamAttr = {0};
    REG_MM81A4_Arsie PSCompFifoValue = {0};
    REG_MM81DC_Arise  PSStartAddrValue = {0};
    REG_MM33700_Arise  PSBaseOffsetValue = {0};
    REG_MM81C4_Arise  PSVsyncOff = {0};
    REG_MM81E0_Arise  PSRightBaseValue = {0};
    REG_MM81C8_Arise  PSStrideValue = {0};
    REG_MM81FC_Arise  PSShadowValue = {0};
    REG_MM33644_Arise  PSCscValue = {0};
    REG_MM33810_Arise PS1BLIIndexValue = {0};
    REG_MM33E6C_Arise PS2BLIIndexValue = {0};

    //use mmio
    cbBiosMMIOWriteReg(pcbe, CR_67, 0x08, ~0x08, IGAIndex);
    cbBiosMMIOWriteReg(pcbe, CR_64, 0x40, ~0x40, IGAIndex);

    if(pFlipMode->FlipType == CBIOS_PLANE_FLIP_WITH_DISABLE)
    {
        ullStartAddress = STREAM_DISABLE_FAKE_ADDR;
    }
    else
    {
        pSurfaceAttrib = &pStreamPara->SurfaceAttrib;
        pSrcWin = &pStreamPara->SrcWindow;
        pDstWin = &pStreamPara->DispWindow;

        StreamAttr.pSurfaceAttr = pSurfaceAttrib;
        StreamAttr.pSrcWinPara  = pSrcWin;
        cbGetStreamAttribute(&StreamAttr);
        ullStartAddress = pSurfaceAttrib->StartAddr;
    }

    PSStartAddrValue.PS1_Start_Address = (CBIOS_U32)(ullStartAddress >> 5);

    if(pFlipMode->FlipType == CBIOS_PLANE_FLIP_WITH_DISABLE)
    {
        PSShadowValue.PS_Disable = 1;
        PSBaseOffsetValue.Base_Offset = 0;
        cbMMIOWriteReg32(pcbe, PSBaseOffsetIndex[IGAIndex], PSBaseOffsetValue.Value, 0);
        cbMMIOWriteReg32(pcbe, PSStartAddrIndex[IGAIndex], PSStartAddrValue.Value, 0);
        cbMMIOWriteReg32(pcbe, PSShadowIndex[IGAIndex], PSShadowValue.Value, 0);
        return  CBIOS_TRUE;
    }

    PSBaseOffsetValue.Base_Offset = StreamAttr.dwBaseOffset >> 5;
    PSStrideValue.PS_Pixel_Offset = StreamAttr.PixelOffset;
    PSStrideValue.PS_Pixel_Offset_Bit4 = StreamAttr.PixelOffset >> 4;
    PSStrideValue.PS_Stride = StreamAttr.dwStride;

    if(pSurfaceAttrib->SurfaceFmt & (CBIOS_FMT_A2B10G10R10 | CBIOS_FMT_A8B8G8R8 | CBIOS_FMT_X8B8G8R8))
    {
        PSStrideValue.PS_Abgr_En = 1; //DX10 ABGR format enabled for PS1
    }
    if(pSurfaceAttrib->SurfaceFmt & (CBIOS_FMT_YCRYCB422_16BIT | CBIOS_FMT_YCRYCB422_32BIT))
    {
        PSStrideValue.PS_Uyvy422 = 1;
    }
    if(pSurfaceAttrib->SurfaceFmt & (CBIOS_FMT_YCBCR8888_32BIT | CBIOS_FMT_YCBCR2101010_32BIT))
    {
        PSStrideValue.PS_Ycbcr_Mode = 1;
    }

    if(pSurfaceAttrib->bCompress)
    {
        PSCompFifoValue.PS_Compress_Type = pSurfaceAttrib->Range_Type;
        if (0 == IGAIndex)
        {
            PS1BLIIndexValue.PS1_BL_IDX = pSurfaceAttrib->BLIndex;
        }
        else
        {
            PS2BLIIndexValue.PS2_BL_IDX = pSurfaceAttrib->BLIndex;
        }
        PSShadowValue.PS_Compress_Enable = 1;
    }
    PSCompFifoValue.PS_Fifo_Depth = 0; //256
    if(pSrcWin->WinSize == pDstWin->WinSize)
    {
        PSCompFifoValue.PS_Share_Scl_Fifo = 1;
    }

    DacColorMode = cbGetHwDacMode(pcbe, pSurfaceAttrib->SurfaceFmt);

    if(pSurfaceAttrib->SurfaceFmt == CBIOS_FMT_P8)
    {
        dwBitCnt = 8;
    }
    else if(pSurfaceAttrib->SurfaceFmt & CBIOS_STREAM_16BPP_FMTS)
    {
        dwBitCnt = 16;
    }
    else
    {
        dwBitCnt = 32;
    }

    dwFetchCount = (pModeParams->SrcModePara.XRes & 0xFFFF)* dwBitCnt;
    if(pSurfaceAttrib->b3DMode && pModeParams->Video3DStruct == SIDE_BY_SIDE_HALF)
    {
        dwFetchCount /= 2;
    }
    dwFetchCount = (dwFetchCount + 255) / 256;

    PSShadowValue.DAC_Color_Mode = DacColorMode;
    PSShadowValue.PS_L1_10to0 = dwFetchCount & 0x7ff;
    PSShadowValue.PS_L1_Bit11 = (dwFetchCount >> 11) & 0x1;
    PSShadowValue.Enable_PS_L1 = 1;
    PSShadowValue.PS_Read_Length = 2;
    PSShadowValue.PS_Disable = 0;
    if(pSurfaceAttrib->b3DMode)
    {
        PSShadowValue.PS_3D_Video_Mode = cbGetHw3DVideoMode(pSurfaceAttrib->Surface3DPara.Video3DStruct);
    }

    if(pSurfaceAttrib->SurfaceFmt &  CBIOS_STREAM_FMT_YUV_SPACE)
    {
        if(pModeParams->TargetModePara.YRes >= 720)
        {
            PSCscValue.PS_Data_In_Fmt = CSC_FMT_YCBCR709;
        }
        else
        {
            PSCscValue.PS_Data_In_Fmt = CSC_FMT_YCBCR601;
        }
    }
    else
    {
        PSCscValue.PS_Data_In_Fmt = CSC_FMT_RGB;
    }
    PSCscValue.PS_Data_Out_Fmt = CSC_FMT_RGB;

    if(pSurfaceAttrib->b3DMode)
    {
        PSRightBaseValue.PS_Right_Frame_Base = ((pSurfaceAttrib->Surface3DPara.RightFrameOffset & ~0xf) >> 4);
    }

    if(pFlipMode->FlipImme)
    {
        PSVsyncOff.VsyncOff = 1;
    }

    cbMMIOWriteReg32(pcbe, PSVsyncOffIndex[IGAIndex], PSVsyncOff.Value, 0);

    cbMMIOWriteReg32(pcbe, PSCompFifoIndex[IGAIndex], PSCompFifoValue.Value, 0);
    cbMMIOWriteReg32(pcbe, PSStartAddrIndex[IGAIndex], PSStartAddrValue.Value, 0);
    cbMMIOWriteReg32(pcbe, PSBaseOffsetIndex[IGAIndex], PSBaseOffsetValue.Value, 0);
    cbMMIOWriteReg32(pcbe, PSRightBaseIndex[IGAIndex], PSRightBaseValue.Value, 0);
    cbMMIOWriteReg32(pcbe, PSStrideIndex[IGAIndex], PSStrideValue.Value, 0);
    cbMMIOWriteReg32(pcbe, PSShadowIndex[IGAIndex], PSShadowValue.Value, 0);
    cbMMIOWriteReg32(pcbe, PSCscIndex[IGAIndex], PSCscValue.Value, 0);

    if (0 == IGAIndex)
    {
        cbMMIOWriteReg32(pcbe, PSBLIndex[IGAIndex], PS1BLIIndexValue.Value, 0);
    }
    else
    {
        cbMMIOWriteReg32(pcbe, PSBLIndex[IGAIndex], PS2BLIIndexValue.Value, 0);
    }


    return CBIOS_TRUE;
}

CBIOS_BOOL cbUpdateSecondStream_Arise(PCBIOS_EXTENSION_COMMON pcbe,
                                    CBIOS_U32 IGAIndex,
                                    PCBIOS_STREAM_PARA pStreamPara,
                                    PCBIOS_FLIP_MODE pFlipMode)
{
    PCBIOS_SURFACE_ATTRIB     pSurfaceAttrib = CBIOS_NULL;
    PCBIOS_DISP_MODE_PARAMS   pModeParams = pcbe->DispMgr.pModeParams[IGAIndex];
    PCBIOS_WINDOW_PARA   pSrcWin = CBIOS_NULL, pDstWin = CBIOS_NULL;
    CBIOS_U64                ullStartAddress = 0;
    CBIOS_STREAM_ATTRIBUTE    StreamAttr = {0};
    REG_MM8198_Arise    SSCompFifoValue = {0};
    REG_MM81D0_Arise    SSStartAddrValue = {0};
    REG_MM33708_Arise    SSBaseOffsetValue = {0};
    REG_MM33834_Arise    SSEnableValue = {0};
    REG_MM81D4_Arise    SSRightBaseValue = {0};
    REG_MM81D8_Arise    SSStrideValue = {0};
    REG_MM8190_Arise    SSFmtSrcWValue = {0};
    REG_MM819C_Arise    SSDstWinValue = {0};
    REG_MM81A8_Arise    SSSrcWinHValue = {0};
    REG_MM81F8_Arise    SSDstWinPosValue = {0};
    REG_MM332E0_Arise    SSScalRatioHValue = {0};
    REG_MM332E4_Arise    SSScalRatioVValue = {0};
    REG_MM3362C_Arise    SSCscValue = {0};
    REG_MM33854_Arise    SSBLIndexValue = {0};

    if(pFlipMode->FlipType == CBIOS_PLANE_FLIP_WITH_DISABLE)
    {
        ullStartAddress = STREAM_DISABLE_FAKE_ADDR;
    }
    else
    {
        pSurfaceAttrib = &pStreamPara->SurfaceAttrib;
        pSrcWin = &pStreamPara->SrcWindow;
        pDstWin = &pStreamPara->DispWindow;

        StreamAttr.pSurfaceAttr = pSurfaceAttrib;
        StreamAttr.pSrcWinPara  = pSrcWin;
        cbGetStreamAttribute(&StreamAttr);
        ullStartAddress = pSurfaceAttrib->StartAddr;
    }

    SSStartAddrValue.SS_Start_Address0 = (CBIOS_U32)(ullStartAddress >> 5);

    SSEnableValue.SS_Use_Mmio_En = 1;
    if(pFlipMode->FlipType == CBIOS_PLANE_FLIP_WITH_DISABLE)
    {
        SSEnableValue.SS_Enable = 0;
        SSBaseOffsetValue.SS_Base_Offset = 0;
        cbMMIOWriteReg32(pcbe, SSBaseOffsetIndex[IGAIndex], SSBaseOffsetValue.Value, 0);
        cbMMIOWriteReg32(pcbe, SSStartAddrIndex[IGAIndex], SSStartAddrValue.Value, 0);
        cbMMIOWriteReg32(pcbe, SSEnableIndex[IGAIndex], SSEnableValue.Value, 0);
        return  CBIOS_TRUE;
    }

    SSEnableValue.SS_Enable = 1;

    SSBaseOffsetValue.SS_Base_Offset = StreamAttr.dwBaseOffset >> 5;
    SSStrideValue.SS_Stride = StreamAttr.dwStride;
    SSStrideValue.SS_Pixel_Offset = StreamAttr.PixelOffset;
    SSStrideValue.SS_Read_Length = 3; //256 bits
    if(pSurfaceAttrib->SurfaceFmt & (CBIOS_FMT_A2B10G10R10 | CBIOS_FMT_A8B8G8R8 | CBIOS_FMT_X8B8G8R8))
    {
        SSStrideValue.SS_Abgr_En = 1; //DX10 ABGR format enabled for PS1
    }

    if(pSurfaceAttrib->bCompress)
    {
        SSCompFifoValue.SS_Comp_Mode_Enable = 1;
        SSCompFifoValue.SS_Compress_Type = pSurfaceAttrib->Range_Type;
        SSBLIndexValue.SS1_burst_index = pSurfaceAttrib->BLIndex;
    }
    SSCompFifoValue.SS_Fifo_Depth_Control = 0; //256
    if(pSrcWin->WinSize == pDstWin->WinSize)
    {
        SSCompFifoValue.SS_Share_Scaler_Fifo = 1;
    }

    if(pSurfaceAttrib->SurfaceFmt & (CBIOS_FMT_YCRYCB422_16BIT | CBIOS_FMT_YCRYCB422_32BIT))
    {
        SSFmtSrcWValue.SS_Uyvy422 = 1;
    }
    if(pSurfaceAttrib->SurfaceFmt & (CBIOS_FMT_YCBCR8888_32BIT | CBIOS_FMT_YCBCR2101010_32BIT))
    {
        SSFmtSrcWValue.SS_YCbCr_Mode = 1;
    }
    SSFmtSrcWValue.SS_Input_Format = cbGetHwDacMode(pcbe, pSurfaceAttrib->SurfaceFmt);
    SSFmtSrcWValue.SS_YCbCr_Mode = 0; //?
    SSFmtSrcWValue.SS_Src_Line_Width = pSrcWin->WinSize & 0xFFFF;

    SSSrcWinHValue.SS_Line_Height = (pSrcWin->WinSize >> 16) & 0xFFFF;

    SSDstWinValue.SS_Dest_Width = pDstWin->WinSize & 0xFFFF;
    SSDstWinValue.SS_Dest_Height = (pDstWin->WinSize >> 16) & 0xFFFF;

    SSDstWinPosValue.SS_X_Start = (pDstWin->Position & 0xFFFF) + 1;
    SSDstWinPosValue.SS_Y_Start = ((pDstWin->Position >> 16) & 0xFFFF) + 1;

    if((pSrcWin->WinSize & 0xFFFF) == (pDstWin->WinSize & 0xFFFF))
    {
        SSScalRatioHValue.SS_Hacc = 0x100000;
    }
    else
    {
        SSScalRatioHValue.SS_Hacc = ((1048576 * (pSrcWin->WinSize & 0xFFFF)) /(pDstWin->WinSize & 0xFFFF)) & 0x1FFFFF;
    }
    if((pSrcWin->WinSize & 0xFFFF0000) == (pDstWin->WinSize & 0xFFFF0000))
    {
        SSScalRatioVValue.SS_Vacc = 0x100000;
    }
    else
    {
        SSScalRatioVValue.SS_Vacc = ((1048576 * ((pSrcWin->WinSize >> 16) & 0xFFFF)) /((pDstWin->WinSize >> 16) & 0xFFFF)) & 0x1FFFFF;
    }

    if(pSurfaceAttrib->SurfaceFmt &  CBIOS_STREAM_FMT_YUV_SPACE)
    {
        if(pModeParams->TargetModePara.YRes >= 720)
        {
            SSCscValue.SS_Data_In_Format = CSC_FMT_YCBCR709;
        }
        else
        {
            SSCscValue.SS_Data_In_Format = CSC_FMT_YCBCR601;
        }
    }
    else
    {
        SSCscValue.SS_Data_In_Format = CSC_FMT_RGB;
    }
    SSCscValue.SS_Data_Out_Format = CSC_FMT_RGB;

    if(pSurfaceAttrib->b3DMode)
    {
        SSRightBaseValue.SS_Roffset = pSurfaceAttrib->Surface3DPara.RightFrameOffset;
    }

    if(pFlipMode->FlipImme)
    {
        SSRightBaseValue.SS_Vsync_Off_Flip = 1;
    }

    cbMMIOWriteReg32(pcbe, SSRightBaseIndex[IGAIndex], SSRightBaseValue.Value, 0);

    cbMMIOWriteReg32(pcbe, SSStartAddrIndex[IGAIndex], SSStartAddrValue.Value, 0);
    cbMMIOWriteReg32(pcbe, SSBaseOffsetIndex[IGAIndex], SSBaseOffsetValue.Value, 0);
    cbMMIOWriteReg32(pcbe, SSStrideIndex[IGAIndex], SSStrideValue.Value, 0);
    cbMMIOWriteReg32(pcbe, SSCompFifoIndex[IGAIndex], SSCompFifoValue.Value, 0);
    cbMMIOWriteReg32(pcbe, SSFmtSrcWIndex[IGAIndex], SSFmtSrcWValue.Value, 0);
    cbMMIOWriteReg32(pcbe, SSDstWinIndex[IGAIndex], SSDstWinValue.Value, 0);
    cbMMIOWriteReg32(pcbe, SSDstWinPosIndex[IGAIndex], SSDstWinPosValue.Value, 0);
    cbMMIOWriteReg32(pcbe, SSSrcWinHIndex[IGAIndex], SSSrcWinHValue.Value, 0);
    cbMMIOWriteReg32(pcbe, SSScalRatioHIndex[IGAIndex], SSScalRatioHValue.Value, 0);
    cbMMIOWriteReg32(pcbe, SSScalRatioVIndex[IGAIndex], SSScalRatioVValue.Value, 0);
    cbMMIOWriteReg32(pcbe, SSCscIndex[IGAIndex], SSCscValue.Value, 0);
    cbMMIOWriteReg32(pcbe, SSEnableIndex[IGAIndex], SSEnableValue.Value, 0);
    cbMMIOWriteReg32(pcbe, SSBLIndex[IGAIndex], SSBLIndexValue.Value, 0);

    return  CBIOS_TRUE;
}

CBIOS_BOOL cbUpdateThdFourStream_Arise(PCBIOS_EXTENSION_COMMON pcbe,
                                    CBIOS_U32 IGAIndex,
                                    CBIOS_STREAM_TP  StreamIndex,
                                    PCBIOS_STREAM_PARA pStreamPara,
                                    PCBIOS_FLIP_MODE pFlipMode)
{
    PCBIOS_SURFACE_ATTRIB     pSurfaceAttrib = CBIOS_NULL;
    PCBIOS_DISP_MODE_PARAMS   pModeParams = pcbe->DispMgr.pModeParams[IGAIndex];
    PCBIOS_WINDOW_PARA   pSrcWin = CBIOS_NULL, pDstWin = CBIOS_NULL;
    CBIOS_U64                 ullStartAddress = 0;
    CBIOS_U32            dwIndex = StreamIndex - CBIOS_STREAM_TS;
    CBIOS_STREAM_ATTRIBUTE    StreamAttr = {0};
    REG_MM8430_Arise  TQSStartAddrValue = {0};
    REG_MM8414_Arise  TQSCompFifoValue = {0};
    REG_MM8498_Arise  TQSBaseOffsetValue = {0};
    REG_MM8458_Arise  TQSEnableValue = {0};
    REG_MM844C_Arise  TQSRightBaseValue = {0};
    REG_MM8438_Arise  TQSStrideValue = {0};
    REG_MM33600_Arise  TQSFormatValue = {0};
    REG_MM8444_Arise  TQSDstWinPosValue = {0};
    REG_MM8450_Arise  TQSDstWinValue = {0};
    REG_MM8448_Arise  TQSSrcWinValue = {0};
    REG_MM84A4_Arise  TQSScalRatioHValue = {0};
    REG_MM84A8_Arise  TQSScalRatioVValue = {0};
    REG_MM33614_Arise  TQSCscValue = {0};
    REG_MM33858_Arise TQSBLIndexValue = {0};

    if(StreamIndex != CBIOS_STREAM_TS && StreamIndex != CBIOS_STREAM_FS)
    {
        return  CBIOS_FALSE;
    }

    //TS and QS can only use mmio

    if(pFlipMode->FlipType == CBIOS_PLANE_FLIP_WITH_DISABLE)
    {
        ullStartAddress = STREAM_DISABLE_FAKE_ADDR;
    }
    else
    {
        pSurfaceAttrib = &pStreamPara->SurfaceAttrib;
        pSrcWin = &pStreamPara->SrcWindow;
        pDstWin = &pStreamPara->DispWindow;

        StreamAttr.pSurfaceAttr = pSurfaceAttrib;
        StreamAttr.pSrcWinPara  = pSrcWin;
        cbGetStreamAttribute(&StreamAttr);
        ullStartAddress = pSurfaceAttrib->StartAddr;
    }

    TQSStartAddrValue.TS_FB_Start_Address_0 = (CBIOS_U32)(ullStartAddress >> 5);

    if(pFlipMode->FlipType == CBIOS_PLANE_FLIP_WITH_DISABLE)
    {
        TQSEnableValue.TS_En = 0;
        TQSBaseOffsetValue.TS_Win1_Base_Offset = 0;
        cbMMIOWriteReg32(pcbe, TQSBaseOffsetIndex[dwIndex][IGAIndex], TQSBaseOffsetValue.Value, 0);
        cbMMIOWriteReg32(pcbe, TQSStartAddrIndex[dwIndex][IGAIndex], TQSStartAddrValue.Value, 0);
        cbMMIOWriteReg32(pcbe, TQSEnableIndex[dwIndex][IGAIndex], TQSEnableValue.Value, 0);
        return  CBIOS_TRUE;
    }

    TQSEnableValue.TS_En = 1;

    TQSBaseOffsetValue.TS_Win1_Base_Offset = StreamAttr.dwBaseOffset >> 5;
    TQSStrideValue.TS_Stride = StreamAttr.dwStride;
    TQSStrideValue.TS_Start_Address_Byte_Offset = StreamAttr.PixelOffset;
    TQSStrideValue.TS_Read_Length = 3; //256 bits
    if(pSurfaceAttrib->SurfaceFmt & (CBIOS_FMT_A2B10G10R10 | CBIOS_FMT_A8B8G8R8 | CBIOS_FMT_X8B8G8R8))
    {
        TQSStrideValue.TS_Abgr_En = 1; //DX10 ABGR format enabled for PS1
    }

    if(pSurfaceAttrib->bCompress)
    {
        TQSCompFifoValue.TS_Comp_Enable = 1;
        TQSCompFifoValue.TS_Comp_Type = pSurfaceAttrib->Range_Type;
        TQSBLIndexValue.TS_win1_burst_index = pSurfaceAttrib->BLIndex;
    }
    TQSCompFifoValue.TS_Fifo_Depth = 0; //256
    if(pSrcWin->WinSize == pDstWin->WinSize)
    {
        TQSCompFifoValue.TS_Share_Scaler_Fifo = 1;
    }

    if(pSurfaceAttrib->SurfaceFmt & (CBIOS_FMT_YCRYCB422_16BIT | CBIOS_FMT_YCRYCB422_32BIT))
    {
        TQSFormatValue.TS_YCbCr_Order = 1;
    }
    if(pSurfaceAttrib->SurfaceFmt & (CBIOS_FMT_YCBCR8888_32BIT | CBIOS_FMT_YCBCR2101010_32BIT))
    {
        TQSFormatValue.TS_444_YCbCr_Order = 1;
    }
    TQSFormatValue.TS_Win1_Format = cbGetHwDacMode(pcbe, pSurfaceAttrib->SurfaceFmt);

    TQSSrcWinValue.TS_Win1_Src_W = pSrcWin->WinSize & 0xFFFF;

    TQSSrcWinValue.TS_Win1_Src_H = (pSrcWin->WinSize >> 16) & 0xFFFF;

    TQSDstWinValue.TS_Win1_Dst_W = pDstWin->WinSize & 0xFFFF;
    TQSDstWinValue.TS_Win1_Dst_H = (pDstWin->WinSize >> 16) & 0xFFFF;

    TQSDstWinPosValue.TS_X_Start = (pDstWin->Position & 0xFFFF) + 1;
    TQSDstWinPosValue.TS_Y_Start = ((pDstWin->Position >> 16) & 0xFFFF) + 1;

    if((pSrcWin->WinSize & 0xFFFF) == (pDstWin->WinSize & 0xFFFF))
    {
        TQSScalRatioHValue.TS_Win1_Hacc = 0x100000;
    }
    else
    {
        TQSScalRatioHValue.TS_Win1_Hacc = ((1048576 * (pSrcWin->WinSize & 0xFFFF)) /(pDstWin->WinSize & 0xFFFF)) & 0x1FFFFF;
    }
    if((pSrcWin->WinSize & 0xFFFF0000) == (pDstWin->WinSize & 0xFFFF0000))
    {
        TQSScalRatioVValue.TS_Win1_Vacc = 0x100000;
    }
    else
    {
        TQSScalRatioVValue.TS_Win1_Vacc = ((1048576 * ((pSrcWin->WinSize >> 16) & 0xFFFF)) /((pDstWin->WinSize >> 16) & 0xFFFF)) & 0x1FFFFF;
    }

    if(pSurfaceAttrib->SurfaceFmt &  CBIOS_STREAM_FMT_YUV_SPACE)
    {
        if(pModeParams->TargetModePara.YRes >= 720)
        {
            TQSCscValue.TS_In_Format = CSC_FMT_YCBCR709;
        }
        else
        {
            TQSCscValue.TS_In_Format = CSC_FMT_YCBCR601;
        }
    }
    else
    {
        TQSCscValue.TS_In_Format = CSC_FMT_RGB;
    }
    TQSCscValue.TS_Out_Format = CSC_FMT_RGB;

    if(pSurfaceAttrib->b3DMode)
    {
        TQSRightBaseValue.TS_Win1_Roffset = pSurfaceAttrib->Surface3DPara.RightFrameOffset;
    }

    if(pFlipMode->FlipImme)
    {
        TQSRightBaseValue.TS_Win1_Vsync_Off_Flip = 1;
    }

    cbMMIOWriteReg32(pcbe, TQSRightBaseIndex[dwIndex][IGAIndex], TQSRightBaseValue.Value, 0);

    cbMMIOWriteReg32(pcbe, TQSStartAddrIndex[dwIndex][IGAIndex], TQSStartAddrValue.Value, 0);
    cbMMIOWriteReg32(pcbe, TQSBaseOffsetIndex[dwIndex][IGAIndex], TQSBaseOffsetValue.Value, 0);
    cbMMIOWriteReg32(pcbe, TQSStrideIndex[dwIndex][IGAIndex], TQSStrideValue.Value, 0);
    cbMMIOWriteReg32(pcbe, TQSCompFifoIndex[dwIndex][IGAIndex], TQSCompFifoValue.Value, 0);
    cbMMIOWriteReg32(pcbe, TQSFormatIndex[dwIndex][IGAIndex], TQSFormatValue.Value, 0);
    cbMMIOWriteReg32(pcbe, TQSDstWinIndex[dwIndex][IGAIndex], TQSDstWinValue.Value, 0);
    cbMMIOWriteReg32(pcbe, TQSDstWinPosIndex[dwIndex][IGAIndex], TQSDstWinPosValue.Value, 0);
    cbMMIOWriteReg32(pcbe, TQSSrcWinIndex[dwIndex][IGAIndex], TQSSrcWinValue.Value, 0);
    cbMMIOWriteReg32(pcbe, TQSScalRatioHIndex[dwIndex][IGAIndex], TQSScalRatioHValue.Value, 0);
    cbMMIOWriteReg32(pcbe, TQSScalRatioVIndex[dwIndex][IGAIndex], TQSScalRatioVValue.Value, 0);
    cbMMIOWriteReg32(pcbe, TQSCscIndex[dwIndex][IGAIndex], TQSCscValue.Value, 0);
    cbMMIOWriteReg32(pcbe, TQSEnableIndex[dwIndex][IGAIndex], TQSEnableValue.Value, 0);
    cbMMIOWriteReg32(pcbe, TQSBLIndex[dwIndex][IGAIndex], TQSBLIndexValue.Value, 0);

    return  CBIOS_TRUE;
}

CBIOS_STATUS cbDoCSCAdjust_Arise(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_ACTIVE_TYPE Device, PCBIOS_CSC_ADJUST_PARA pCSCAdjustPara)
{
    PCBIOS_CSC_ADJUST_PARA  pAdjustPara = pCSCAdjustPara;
    CBIOS_S64               CSCm[3][3];
    CSC_INDEX               CSCIndex = 0;
    REG_MM33698_Arise         CSCCoef1RegValue = {0}, CSCCoef1RegMask = {0xffffffff};
    REG_MM3369C_Arise         CSCCoef2RegValue = {0}, CSCCoef2RegMask = {0xffffffff};
    REG_MM336A0_Arise        CSCCoef3RegValue = {0}, CSCCoef3RegMask = {0xffffffff};
    REG_MM336A4_Arise        CSCCoef4RegValue = {0}, CSCCoef4RegMask = {0xffffffff};
    REG_MM336A8_Arise         CSCCoef5RegValue = {0}, CSCCoef5RegMask = {0xffffffff};
    REG_MM33694_Arise         LBCscFormatRegValue = {0}, LBCscFormatRegMask = {0xffffffff};
    REG_MM33868_Arise         DACCscFormatRegValue = {0}, DACCscFormatRegMask = {0xffffffff};

    cb_memset((PCBIOS_VOID)(CSCm), 0, sizeof(CSCm));

    if(CBIOS_TYPE_DP1 == Device)
    {
        CSCIndex = CSC_INDEX_DP1;
    }
    else if(CBIOS_TYPE_DP2 == Device)
    {
        CSCIndex = CSC_INDEX_DP2;
    }
    else if(CBIOS_TYPE_DP3 == Device)
    {
        CSCIndex = CSC_INDEX_DP3;
    }
    else if(CBIOS_TYPE_DP4 == Device)
    {
        CSCIndex = CSC_INDEX_DP4;
    }
    else
    {
        CSCIndex = CSC_INDEX_CRT;
    }

    LBCscFormatRegMask.Value = 0xffffffff;
    DACCscFormatRegMask.Value = 0xffffffff;

    if (CSC_INDEX_CRT == CSCIndex)
    {
        DACCscFormatRegValue.Dac_Csc_In_Format = pAdjustPara->InputFormat;
        DACCscFormatRegMask.Dac_Csc_In_Format = 0;
        DACCscFormatRegValue.Dac_Csc_Out_Format = pAdjustPara->OutputFormat;
        DACCscFormatRegMask.Dac_Csc_Out_Format = 0;
    }
    else
    {
        LBCscFormatRegValue.LB1_CSC_IN_FMT = pAdjustPara->InputFormat;
        LBCscFormatRegMask.LB1_CSC_IN_FMT = 0;
        LBCscFormatRegValue.LB1_CSC_OUT_FMT = pAdjustPara->OutputFormat;
        LBCscFormatRegMask.LB1_CSC_OUT_FMT = 0;
    }

    if(!pAdjustPara->Flag.bProgrammable)
    {
        if (CSC_INDEX_CRT == CSCIndex)
        {
            DACCscFormatRegValue.Dac_Csc_Program = 0;
            DACCscFormatRegMask.Dac_Csc_Program = 0;
        }
        else
        {
            LBCscFormatRegValue.LB1_PROGRAMMBLE = 0;
            LBCscFormatRegMask.LB1_PROGRAMMBLE = 0;
        }
    }
    else
    {
        if (CSC_INDEX_CRT == CSCIndex)
        {
            DACCscFormatRegValue.Dac_Csc_Program = 1;
            DACCscFormatRegMask.Dac_Csc_Program = 0;
        }
        else
        {
            LBCscFormatRegValue.LB1_PROGRAMMBLE = 1;
            LBCscFormatRegMask.LB1_PROGRAMMBLE = 0;
        }

        if(pAdjustPara->Bright > 255 || pAdjustPara->Bright < -255)
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "CSC bright value should be [-255,255]!\n"));
        }
        else
        {
            if(pAdjustPara->Bright >= 0)
            {
                CSCCoef5RegValue.LB1_BRIGHT = pAdjustPara->Bright;
            }
            else
            {
                CSCCoef5RegValue.LB1_BRIGHT = (CBIOS_U32)(1 << CSC_BRIGHT_MODULE) + pAdjustPara->Bright;  //get brightness 2's complement
            }
            CSCCoef5RegMask.LB1_BRIGHT = 0;
        }

        if(cbGetCscCoefMatrix(pAdjustPara,
                              pCSCAdjustPara->InputFormat,
                              pCSCAdjustPara->OutputFormat,
                              CSCm))
        {
            CSCCoef1RegValue.LB1_COEF_F1 = cbTran_CSCm_to_coef_fx(CSCm[0][0]);
            CSCCoef1RegValue.LB1_COEF_F2 = cbTran_CSCm_to_coef_fx(CSCm[0][1]);
            CSCCoef2RegValue.LB1_COEF_F3 = cbTran_CSCm_to_coef_fx(CSCm[0][2]);
            CSCCoef2RegValue.LB1_COEF_F4 = cbTran_CSCm_to_coef_fx(CSCm[1][0]);
            CSCCoef3RegValue.LB1_COEF_F5 = cbTran_CSCm_to_coef_fx(CSCm[1][1]);
            CSCCoef3RegValue.LB1_COEF_F6 = cbTran_CSCm_to_coef_fx(CSCm[1][2]);
            CSCCoef4RegValue.LB1_COEF_F7 = cbTran_CSCm_to_coef_fx(CSCm[2][0]);
            CSCCoef4RegValue.LB1_COEF_F8 = cbTran_CSCm_to_coef_fx(CSCm[2][1]);
            CSCCoef5RegValue.LB1_COEF_F9 = cbTran_CSCm_to_coef_fx(CSCm[2][2]);

            CSCCoef1RegMask.Value = 0;
            CSCCoef2RegMask.Value = 0;
            CSCCoef3RegMask.Value = 0;
            CSCCoef4RegMask.Value = 0;
            CSCCoef5RegMask.LB1_COEF_F9 = 0;
        }
        else // illegal para,use fixed coef do csc
        {
            if (CSC_INDEX_CRT == CSCIndex)
            {
                DACCscFormatRegValue.Dac_Csc_Program = 0;
                DACCscFormatRegMask.Dac_Csc_Program = 0;
            }
            else
            {
                LBCscFormatRegValue.LB1_PROGRAMMBLE = 0;
                LBCscFormatRegMask.LB1_PROGRAMMBLE = 0;
            }
        }

        if(CSC_INDEX_DP1 == CSCIndex)
        {
            cbMMIOWriteReg32(pcbe, 0x33698, CSCCoef1RegValue.Value, CSCCoef1RegMask.Value);
            cbMMIOWriteReg32(pcbe, 0x3369c, CSCCoef2RegValue.Value, CSCCoef2RegMask.Value);
            cbMMIOWriteReg32(pcbe, 0x336a0, CSCCoef3RegValue.Value, CSCCoef3RegMask.Value);
            cbMMIOWriteReg32(pcbe, 0x336a4, CSCCoef4RegValue.Value, CSCCoef4RegMask.Value);
            cbMMIOWriteReg32(pcbe, 0x336a8, CSCCoef5RegValue.Value, CSCCoef5RegMask.Value);
        }
        else if(CSC_INDEX_DP2 == CSCIndex)
        {
            cbMMIOWriteReg32(pcbe, 0x33c38, CSCCoef1RegValue.Value, CSCCoef1RegMask.Value);
            cbMMIOWriteReg32(pcbe, 0x33c3c, CSCCoef2RegValue.Value, CSCCoef2RegMask.Value);
            cbMMIOWriteReg32(pcbe, 0x33c40, CSCCoef3RegValue.Value, CSCCoef3RegMask.Value);
            cbMMIOWriteReg32(pcbe, 0x33c44, CSCCoef4RegValue.Value, CSCCoef4RegMask.Value);
            cbMMIOWriteReg32(pcbe, 0x33c48, CSCCoef5RegValue.Value, CSCCoef5RegMask.Value);
        }
        else if(CSC_INDEX_DP3 == CSCIndex)
        {
            cbMMIOWriteReg32(pcbe, 0x34338, CSCCoef1RegValue.Value, CSCCoef1RegMask.Value);
            cbMMIOWriteReg32(pcbe, 0x3433c, CSCCoef2RegValue.Value, CSCCoef2RegMask.Value);
            cbMMIOWriteReg32(pcbe, 0x34340, CSCCoef3RegValue.Value, CSCCoef3RegMask.Value);
            cbMMIOWriteReg32(pcbe, 0x34344, CSCCoef4RegValue.Value, CSCCoef4RegMask.Value);
            cbMMIOWriteReg32(pcbe, 0x34348, CSCCoef5RegValue.Value, CSCCoef5RegMask.Value);
        }
        else if(CSC_INDEX_DP4 == CSCIndex)
        {
            cbMMIOWriteReg32(pcbe, 0x34a38, CSCCoef1RegValue.Value, CSCCoef1RegMask.Value);
            cbMMIOWriteReg32(pcbe, 0x34a3c, CSCCoef2RegValue.Value, CSCCoef2RegMask.Value);
            cbMMIOWriteReg32(pcbe, 0x34a40, CSCCoef3RegValue.Value, CSCCoef3RegMask.Value);
            cbMMIOWriteReg32(pcbe, 0x34a44, CSCCoef4RegValue.Value, CSCCoef4RegMask.Value);
            cbMMIOWriteReg32(pcbe, 0x34a48, CSCCoef5RegValue.Value, CSCCoef5RegMask.Value);
        }
        else
        {
            cbMMIOWriteReg32(pcbe, 0x3386c, CSCCoef1RegValue.Value, CSCCoef1RegMask.Value);
            cbMMIOWriteReg32(pcbe, 0x33870, CSCCoef2RegValue.Value, CSCCoef2RegMask.Value);
            cbMMIOWriteReg32(pcbe, 0x33874, CSCCoef3RegValue.Value, CSCCoef3RegMask.Value);
            cbMMIOWriteReg32(pcbe, 0x33878, CSCCoef4RegValue.Value, CSCCoef4RegMask.Value);
            cbMMIOWriteReg32(pcbe, 0x3387c, CSCCoef5RegValue.Value, CSCCoef5RegMask.Value);
        }
     }

    if(CSC_INDEX_DP1 == CSCIndex)
    {
        cbMMIOWriteReg32(pcbe, 0x33694, LBCscFormatRegValue.Value, LBCscFormatRegMask.Value);
    }
    else if(CSC_INDEX_DP2 == CSCIndex)
    {
        cbMMIOWriteReg32(pcbe, 0x33c34, LBCscFormatRegValue.Value, LBCscFormatRegMask.Value);
    }
    else if(CSC_INDEX_DP3 == CSCIndex)
    {
        cbMMIOWriteReg32(pcbe, 0x34334, LBCscFormatRegValue.Value, LBCscFormatRegMask.Value);
    }
    else if(CSC_INDEX_DP4 == CSCIndex)
    {
        cbMMIOWriteReg32(pcbe, 0x34a34, LBCscFormatRegValue.Value, LBCscFormatRegMask.Value);
    }
    else
    {
        cbMMIOWriteReg32(pcbe, 0x33868, DACCscFormatRegValue.Value, DACCscFormatRegMask.Value);
    }

    return CBIOS_OK;
}

CBIOS_STATUS cbAdjustStreamCSC_Arise(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_STREAM_CSC_PARA pStreamCSCPara)
{
    PCBIOS_STREAM_CSC_PARA  pAdjustPara = pStreamCSCPara;
    CBIOS_S64               CSCm[3][3];
    CBIOS_U32               IGAIndex = pAdjustPara->IGAIndex;
    CBIOS_U32               StreamType = pAdjustPara->StreamType;
    REG_MM33634_Arise         CSCCoef1RegValue = {0}, CSCCoef1RegMask = {0xffffffff};
    REG_MM33638_Arise         CSCCoef2RegValue = {0}, CSCCoef2RegMask = {0xffffffff};
    REG_MM3363C_Arise         CSCCoef3RegValue = {0}, CSCCoef3RegMask = {0xffffffff};
    REG_MM33640_Arise         CSCCoef4RegValue = {0}, CSCCoef4RegMask = {0xffffffff};
    REG_MM33644_Arise         CscFormatRegValue = {0}, CscFormatRegMask = {0xffffffff};
    REG_MM8200_Arise          OneShotTrigValue = {0};
    CBIOS_U32               CSCCoef1RegIndex = 0;
    CBIOS_U32               CSCCoef2RegIndex = 0;
    CBIOS_U32               CSCCoef3RegIndex = 0;
    CBIOS_U32               CSCCoef4RegIndex = 0;
    CBIOS_U32               CscFormatRegIndex = 0;

    cb_memset((PCBIOS_VOID)(CSCm), 0, sizeof(CSCm));

    if (CBIOS_STREAM_PS == StreamType)
    {
        CSCCoef1RegIndex = PSCscCoef1[IGAIndex];
        CSCCoef2RegIndex = PSCscCoef2[IGAIndex];
        CSCCoef3RegIndex = PSCscCoef3[IGAIndex];
        CSCCoef4RegIndex = PSCscCoef4[IGAIndex];
        CscFormatRegIndex = PSCscIndex[IGAIndex];
        OneShotTrigValue.PS_Enable_Work = 1;
    }
    else if (CBIOS_STREAM_SS == StreamType)
    {
        CSCCoef1RegIndex = SSCscCoef1[IGAIndex];
        CSCCoef2RegIndex = SSCscCoef2[IGAIndex];
        CSCCoef3RegIndex = SSCscCoef3[IGAIndex];
        CSCCoef4RegIndex = SSCscCoef4[IGAIndex];
        CscFormatRegIndex = SSCscIndex[IGAIndex];
        OneShotTrigValue.SS_Enable_Work = 1;
    }
    else if (CBIOS_STREAM_TS == StreamType)
    {
        CSCCoef1RegIndex = TQSCscCoef1[0][IGAIndex];
        CSCCoef2RegIndex = TQSCscCoef2[0][IGAIndex];
        CSCCoef3RegIndex = TQSCscCoef3[0][IGAIndex];
        CSCCoef4RegIndex = TQSCscCoef4[0][IGAIndex];
        CscFormatRegIndex = TQSCscIndex[0][IGAIndex];
        OneShotTrigValue.TS_Enable_Work = 1;
    }
    else if (CBIOS_STREAM_FS == StreamType)
    {
        CSCCoef1RegIndex = TQSCscCoef1[1][IGAIndex];
        CSCCoef2RegIndex = TQSCscCoef2[1][IGAIndex];
        CSCCoef3RegIndex = TQSCscCoef3[1][IGAIndex];
        CSCCoef4RegIndex = TQSCscCoef4[1][IGAIndex];
        CscFormatRegIndex = TQSCscIndex[1][IGAIndex];
        OneShotTrigValue.QS_Enable_Work = 1;
    }

    CscFormatRegValue.PS_Data_In_Fmt = pAdjustPara->InputFormat;
    CscFormatRegMask.PS_Data_In_Fmt = 0;
    CscFormatRegValue.PS_Data_Out_Fmt = pAdjustPara->OutputFormat;
    CscFormatRegMask.PS_Data_Out_Fmt = 0;

    if(!pAdjustPara->Flag.bProgrammable)
    {
        CscFormatRegValue.PS_Program = 0;
        CscFormatRegMask.PS_Program = 0;
    }
    else
    {

        CscFormatRegValue.PS_Program = 1;
        CscFormatRegMask.PS_Program = 0;

        if(pAdjustPara->Bright > 255 || pAdjustPara->Bright < -255)
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "CSC bright value should be [-255,255]!\n"));
        }
        else
        {
            if(pAdjustPara->Bright >= 0)
            {
                CscFormatRegValue.PS_Bright = pAdjustPara->Bright;
            }
            else
            {
                CscFormatRegValue.PS_Bright = (CBIOS_U32)(1 << CSC_BRIGHT_MODULE) + pAdjustPara->Bright;  //get brightness 2's complement
            }
            CscFormatRegMask.PS_Bright = 0;
        }

        if(cbGetCscCoefMatrix((PCBIOS_CSC_ADJUST_PARA)pAdjustPara,
                              pAdjustPara->InputFormat,
                              pAdjustPara->OutputFormat,
                              CSCm))
        {
            CSCCoef1RegValue.PS_F1 = cbTran_CSCm_to_coef_fx(CSCm[0][0]);
            CSCCoef1RegValue.PS_F2 = cbTran_CSCm_to_coef_fx(CSCm[0][1]);
            CSCCoef2RegValue.PS_F3 = cbTran_CSCm_to_coef_fx(CSCm[0][2]);
            CSCCoef2RegValue.PS_F4 = cbTran_CSCm_to_coef_fx(CSCm[1][0]);
            CSCCoef3RegValue.PS_F5 = cbTran_CSCm_to_coef_fx(CSCm[1][1]);
            CSCCoef3RegValue.PS_F6 = cbTran_CSCm_to_coef_fx(CSCm[1][2]);
            CSCCoef4RegValue.PS_F7 = cbTran_CSCm_to_coef_fx(CSCm[2][0]);
            CSCCoef4RegValue.PS_F8 = cbTran_CSCm_to_coef_fx(CSCm[2][1]);
            CscFormatRegValue.PS_F9 = cbTran_CSCm_to_coef_fx(CSCm[2][2]);

            CSCCoef1RegMask.Value = 0;
            CSCCoef2RegMask.Value = 0;
            CSCCoef3RegMask.Value = 0;
            CSCCoef4RegMask.Value = 0;
            CscFormatRegMask.PS_F9 = 0;
        }
        else // illegal para,use fixed coef do csc
        {
            CscFormatRegValue.PS_Program = 0;
            CscFormatRegMask.PS_Program = 0;
        }

        cbMMIOWriteReg32(pcbe, CSCCoef1RegIndex, CSCCoef1RegValue.Value, CSCCoef1RegMask.Value);
        cbMMIOWriteReg32(pcbe, CSCCoef2RegIndex, CSCCoef2RegValue.Value, CSCCoef2RegMask.Value);
        cbMMIOWriteReg32(pcbe, CSCCoef3RegIndex, CSCCoef3RegValue.Value, CSCCoef3RegMask.Value);
        cbMMIOWriteReg32(pcbe, CSCCoef4RegIndex, CSCCoef4RegValue.Value, CSCCoef4RegMask.Value);
     }

    cbMMIOWriteReg32(pcbe, CscFormatRegIndex, CscFormatRegValue.Value, CscFormatRegMask.Value);
    cbMMIOWriteReg32(pcbe, OneShotTrigIndex[IGAIndex], OneShotTrigValue.Value, 0);

    return CBIOS_OK;
}

CBIOS_VOID cbTriggerPlanes_Arise(PCBIOS_EXTENSION_COMMON pcbe,
                             CBIOS_U32  IGAIndex,
                             PCBIOS_FLIP_MODE  pFlipMode)
{
    PCBIOS_FLIP_MODE_EX     pFlipModeEx = (PCBIOS_FLIP_MODE_EX)pFlipMode;
    REG_MM8200_Arise         OneShotTrigValue = {0};
    CBIOS_U32               i = 0;

    for(i = 0; i < OVL_NUM_E3K; i++)
    {
        if(pFlipModeEx->TrigOvlMask & (1 << i))
        {
            if(i == CBIOS_OVERLAY0)
            {
                OneShotTrigValue.Ovl0_Enable_Work = 1;
            }
            else if(i == CBIOS_OVERLAY1)
            {
                OneShotTrigValue.Ovl1_Enable_Work = 1;
            }
            else if(i == CBIOS_OVERLAY2)
            {
                OneShotTrigValue.Ovl2_Enable_Work = 1;
            }
            else if(i == CBIOS_OVERLAY3)
            {
                OneShotTrigValue.Ovl3_Enable_Work = 1;
            }
        }
    }

    for(i = 0; i < STREAM_NUM_E3K; i++)
    {
        if(pFlipModeEx->TrigStreamMask & (1 << i))
        {
            if(i == CBIOS_STREAM_PS)
            {
                OneShotTrigValue.PS_Enable_Work = 1;
            }
            else if(i == CBIOS_STREAM_SS)
            {
                OneShotTrigValue.SS_Enable_Work = 1;
            }
            else if(i == CBIOS_STREAM_TS)
            {
                OneShotTrigValue.TS_Enable_Work = 1;
            }
            else if(i == CBIOS_STREAM_FS)
            {
                OneShotTrigValue.QS_Enable_Work = 1;
            }
        }
    }

    cbMMIOWriteReg32(pcbe, OneShotTrigIndex[IGAIndex], OneShotTrigValue.Value, 0);
}

CBIOS_BOOL cbUpdateOnePlane_Arise(PCBIOS_EXTENSION_COMMON pcbe,
                             PCBIOS_PLANE_PARA pPlanePara,
                             CBIOS_U32 IGAIndex)
{
    CBIOS_BOOL    bStatus = CBIOS_TRUE;

    if(pPlanePara->pInputStream)
    {
        if(pPlanePara->StreamType == CBIOS_STREAM_PS)
        {
            bStatus = cbUpdatePrimaryStream_Arise(pcbe, IGAIndex, pPlanePara->pInputStream, &pPlanePara->FlipMode);
        }
        else if(pPlanePara->StreamType == CBIOS_STREAM_SS)
        {
            bStatus = cbUpdateSecondStream_Arise(pcbe, IGAIndex, pPlanePara->pInputStream, &pPlanePara->FlipMode);
        }
        else if((pPlanePara->StreamType == CBIOS_STREAM_TS) || (pPlanePara->StreamType == CBIOS_STREAM_FS))
        {
            bStatus = cbUpdateThdFourStream_Arise(pcbe, IGAIndex, pPlanePara->StreamType,
                                                                  pPlanePara->pInputStream, &pPlanePara->FlipMode);
        }
    }

    if(pPlanePara->pOverlayInfo || (pPlanePara->FlipMode.FlipType == CBIOS_PLANE_FLIP_WITH_DISABLE))
    {
        cbUpdateOverlay_Arise(pcbe, IGAIndex, pPlanePara->PlaneIndex, pPlanePara->StreamType,
                                                   pPlanePara->pOverlayInfo, &pPlanePara->FlipMode);
    }

    if (pPlanePara->pOVLKeyInfo)
    {
        cbUpdateOVLKey_Arise(pcbe, IGAIndex, pPlanePara->pOVLKeyInfo);
    }

    cbTriggerPlanes_Arise(pcbe, IGAIndex, &pPlanePara->FlipMode);

    return  bStatus;
}

CBIOS_STATUS cbUpdatePlanePerTrig_Arise(PCBIOS_EXTENSION_COMMON pcbe,
                                    PCBIOS_UPDATE_FRAME_PARA   pUpdateFramePara)
{
    CBIOS_STATUS   Status = CBIOS_OK;
    CBIOS_U32    i = 0, TrigIndex = 0xFFFFFFFF, TrigOvlMask = 0, TrigStreamMask = 0;
    PCBIOS_PLANE_PARA   pPlane = CBIOS_NULL, *ppPlane = CBIOS_NULL;
    PCBIOS_FLIP_MODE_EX   pFlipModeEx = CBIOS_NULL;

    ppPlane = pUpdateFramePara->pPlanePara;

    if(pUpdateFramePara->TrigMode.OneShotTrig)
    {
        for(i = 0; i < PLANE_NUM_E3K; i++)
        {
            pPlane = ppPlane[i];

            if(!pPlane)
            {
                continue;
            }

            TrigOvlMask |= (1 << pPlane->PlaneIndex);
            TrigStreamMask |= (1 << pPlane->StreamType);
            if (pPlane->pOVLKeyInfo)
            {
                TrigStreamMask |= (1 << pPlane->pOVLKeyInfo->Index);
            }

            pFlipModeEx = (PCBIOS_FLIP_MODE_EX)(&pPlane->FlipMode);

            if(pPlane->PlaneIndex != pUpdateFramePara->TrigMode.TrigPlane)
            {
                pFlipModeEx->TrigOvlMask = 0;
                pFlipModeEx->TrigStreamMask = 0;
                if(cbUpdateOnePlane_Arise(pcbe, pPlane, pUpdateFramePara->IGAIndex) != CBIOS_TRUE)
                {
                    Status = CBIOS_ER_INVALID_PARAMETER;
                    break;
                }
            }
            else if(pPlane->PlaneIndex == pUpdateFramePara->TrigMode.TrigPlane)
            {
                TrigIndex = i;
            }
        }
        if(Status == CBIOS_OK && TrigIndex != 0xFFFFFFFF)
        {
            pPlane = ppPlane[TrigIndex];
            pFlipModeEx = (PCBIOS_FLIP_MODE_EX)(&pPlane->FlipMode);
            pFlipModeEx->TrigOvlMask = TrigOvlMask;
            pFlipModeEx->TrigStreamMask = TrigStreamMask;
            if(cbUpdateOnePlane_Arise(pcbe, pPlane, pUpdateFramePara->IGAIndex) != CBIOS_TRUE)
            {
                Status = CBIOS_ER_INVALID_PARAMETER;
            }
        }
    }
    else
    {
         for(i = 0; i < PLANE_NUM_E3K; i++)
        {
            pPlane = ppPlane[i];
            if(pPlane)
            {
                pFlipModeEx = (PCBIOS_FLIP_MODE_EX)(&pPlane->FlipMode);
                pFlipModeEx->TrigOvlMask = 1 << pPlane->PlaneIndex;
                pFlipModeEx->TrigStreamMask = 1 << pPlane->StreamType;
                if (pPlane->pOVLKeyInfo)
                {
                    pFlipModeEx->TrigStreamMask |= (1 << pPlane->pOVLKeyInfo->Index);
                }
                if(cbUpdateOnePlane_Arise(pcbe, pPlane, pUpdateFramePara->IGAIndex) != CBIOS_TRUE)
                {
                    Status = CBIOS_ER_INVALID_PARAMETER;
                    break;
                }
            }
        }
    }
    return  Status;
}

CBIOS_STATUS   cbValidatePlanePara_Arise(PCBIOS_EXTENSION_COMMON pcbe,  PCBIOS_UPDATE_FRAME_PARA   pUpdateFrame)
{
    CBIOS_U32  i = 0, iga = 0;
    CBIOS_BOOL  bPlaneValid = CBIOS_FALSE;
    CBIOS_BOOL  bTrigPlaneValid = CBIOS_FALSE;
    PCBIOS_PLANE_PARA  pPlane = CBIOS_NULL;
    CBIOS_STATUS   Status = CBIOS_OK;

    if(pUpdateFrame == CBIOS_NULL)
    {
        Status = CBIOS_ER_INVALID_PARAMETER;
        goto  DONE;
    }

    iga = pUpdateFrame->IGAIndex;

    for(i = 0; i < PLANE_NUM_E3K; i++)
    {
        if(pUpdateFrame->pPlanePara[i] != CBIOS_NULL)
        {
            bPlaneValid = CBIOS_TRUE;
            break;
        }
    }

    if(!bPlaneValid)
    {
        Status = CBIOS_ER_INVALID_PARAMETER;
        goto  DONE;
    }

    for(i = 0; i < PLANE_NUM_E3K; i++)
    {
        if(pUpdateFrame->pPlanePara[i])
        {
            pPlane = pUpdateFrame->pPlanePara[i];
            if(pPlane->PlaneIndex == pUpdateFrame->TrigMode.TrigPlane)
            {
                bTrigPlaneValid = CBIOS_TRUE;
            }
            if(pPlane->FlipMode.FlipType == CBIOS_PLANE_INVALID_FLIP)
            {
                Status = CBIOS_ER_INVALID_PARAMETER;
                goto  DONE;
            }
            if(pPlane->FlipMode.FlipImme && pUpdateFrame->TrigMode.OneShotTrig)
            {
                Status = CBIOS_ER_INVALID_PARAMETER;
                goto  DONE;
            }
            if((pPlane->pInputStream == CBIOS_NULL) && (pPlane->pOverlayInfo == CBIOS_NULL)
                && (pPlane->FlipMode.FlipType != CBIOS_PLANE_FLIP_WITH_DISABLE))
            {
                Status = CBIOS_ER_INVALID_PARAMETER;
                goto  DONE;
            }
            if(pPlane->FlipMode.FlipType == CBIOS_PLANE_INVALID_FLIP)
            {
                Status = CBIOS_ER_INVALID_PARAMETER;
                goto  DONE;
            }
            if(pPlane->pInputStream)
            {
                CBIOS_BOOL  bSupportUpScale = pcbe->DispMgr.UpScaleStreamMask[iga] & (1 << i);
                CBIOS_BOOL  bSupportDownScale = pcbe->DispMgr.DownScaleStreamMask[iga] & (1 << i);
                if(!bSupportUpScale && !bSupportDownScale &&
                    (pPlane->pInputStream->SrcWindow.WinSize!= pPlane->pInputStream->DispWindow.WinSize))
                {
                    Status = CBIOS_ER_INVALID_PARAMETER;
                    goto  DONE;
                }
            }
        }
    }

    if(pUpdateFrame->TrigMode.OneShotTrig && !bTrigPlaneValid)
    {
        Status = CBIOS_ER_INVALID_PARAMETER;
    }

DONE:
    return  Status;
}

CBIOS_STATUS cbUpdateFrame_Arise(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_UPDATE_FRAME_PARA   pUpdateFrame)
{
    CBIOS_STATUS         Status = CBIOS_OK;

    if(cbValidatePlanePara_Arise(pcbe, pUpdateFrame) != CBIOS_OK)
    {
        return  CBIOS_ER_INVALID_PARAMETER;
    }

    Status = cbUpdatePlanePerTrig_Arise(pcbe, pUpdateFrame);

    return  Status;
}

CBIOS_STATUS cbSetGamma_Arise(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_GAMMA_PARA pGammaParam)
{
    if((!pGammaParam) || (!pGammaParam->Flags.bDisableGamma && !pGammaParam->pGammaTable))
    {
        return CBIOS_ER_INVALID_PARAMETER;
    }

    // always use IO path
    pGammaParam->Flags.bUseIO = 1;

    if(1 == pGammaParam->Flags.bDisableGamma)
    {
       cbDisableGamma_Arise(pcbe, pGammaParam->IGAIndex);
    }
    else
    {
        cbWaitVBlank(pcbe, (CBIOS_U8)pGammaParam->IGAIndex);
        if(pGammaParam->Flags.bConfigGamma)
        {
            cbDoGammaConfig_Arise(pcbe, pGammaParam);
        }

        if(pGammaParam->Flags.bSetLUT)
        {
            cbSetLUT_Arise(pcbe, pGammaParam);
        }

        if(pGammaParam->Flags.bGetLUT)
        {
            cbGetLUT_Arise(pcbe, pGammaParam);
        }
    }

    return CBIOS_OK;
}

CBIOS_VOID cbDisableGamma_Arise(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 IGAIndex)
{
    REG_SR47 SR47Value = {0}, SR47Mask = {0xff};

    SR47Value.SP_Gamma_Enable = 0;
    SR47Mask.SP_Gamma_Enable = 0;
    SR47Value.SP_LUT_Interpolation_Enable = 0;
    SR47Mask.SP_LUT_Interpolation_Enable = 0;

    cbBiosMMIOWriteReg(pcbe, SR_47, SR47Value.Value, SR47Mask.Value, IGAIndex);
}

CBIOS_VOID cbDoGammaConfig_Arise(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_GAMMA_PARA pGammaParam)
{
    REG_SR47    SR47Value = {0}, SR47Mask = {0xff};
    REG_CRE2    CRE2Value = {0}, CRE2Mask = {0xff};
    REG_CRE4    CRE4Value = {0}, CRE4Mask = {0xff};

    SR47Value.SP_Gamma_Enable = 1;
    SR47Mask.SP_Gamma_Enable = 0;

    if (pGammaParam->Flags.bInterpolation)
    {
        SR47Value.SP_LUT_Interpolation_Enable = 1;
        SR47Mask.SP_LUT_Interpolation_Enable = 0;
    }
    else
    {
        SR47Value.SP_LUT_Interpolation_Enable = 0;
        SR47Mask.SP_LUT_Interpolation_Enable = 0;
    }

    if (!pGammaParam->Flags.bUseIO)
    {
        //use mmio to R/W LUT
        CRE2Value.LUT_MMIO_EN = 1;
        CRE2Mask.LUT_MMIO_EN = 0;
    }
    else
    {
        CRE2Value.LUT_MMIO_EN = 0;
        CRE2Mask.LUT_MMIO_EN = 0;

        if (!pGammaParam->Flags.bInterpolation)
        {
            CRE4Value.LUT1_BIT_width = 2;
            CRE4Mask.LUT1_BIT_width = 0;
        }
        else
        {
            CRE4Value.LUT1_BIT_width = 1;
            CRE4Mask.LUT1_BIT_width = 0;
        }
        cbMMIOWriteReg(pcbe, LUTBitWidthIndex[pGammaParam->IGAIndex], CRE4Value.Value, CRE4Mask.Value);
    }

    cbBiosMMIOWriteReg(pcbe, SR_47, SR47Value.Value, SR47Mask.Value, pGammaParam->IGAIndex);
    cbMMIOWriteReg(pcbe, CR_E2, CRE2Value.Value, CRE2Mask.Value);
}

CBIOS_VOID cbGetLUT_Arise(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_GAMMA_PARA pGammaParam)
{
    PCBIOS_LUT      pGammaTable = pGammaParam->pGammaTable;
    REG_MM332AC_Arise MM332ACValue = {0}, MM332ACMask = {0xff};
    CBIOS_U32       i;

    if(IGA1 == pGammaParam->IGAIndex)
    {
        cbMMIOWriteReg(pcbe, SR_47, 0x01, 0xF0);        //LUT1 read and write
    }
    else if(IGA2 == pGammaParam->IGAIndex)
    {
        cbMMIOWriteReg(pcbe, SR_47, 0x02, 0xF0);        //LUT2 read and write
    }
    else if(IGA3 == pGammaParam->IGAIndex)
    {
        cbMMIOWriteReg(pcbe, SR_47, 0x09, 0xF0);        //LUT3 read and write
    }
    else
    {
        cbMMIOWriteReg(pcbe, SR_47, 0x0a, 0xF0);        //LUT4 read and write
    }

    if (!pGammaParam->Flags.bUseIO)
    {
        // set index
        cbWaitVBlank(pcbe, (CBIOS_U8)pGammaParam->IGAIndex);
        MM332ACValue.LUT_SSC_HDMI_INFO_DP_INFO_Read_current_index = (CBIOS_UCHAR)(pGammaParam->FisrtEntryIndex);
        MM332ACMask.LUT_SSC_HDMI_INFO_DP_INFO_Read_current_index = 0;
        cbMMIOWriteReg32(pcbe, 0x332ac, MM332ACValue.Value, MM332ACMask.Value);

        //read LUT
        for (i = 0; i < pGammaParam->EntryNum; i++)
        {
            pGammaTable[i].RgbLong = cb_ReadU32(pcbe->pAdapterContext, 0x81a0);
        }
    }
    else
    {
        if(!pGammaParam->Flags.bInterpolation)
        {
            cbWaitVBlank(pcbe, (CBIOS_U8)pGammaParam->IGAIndex);

            cb_WriteU8(pcbe->pAdapterContext, 0x83c7, (CBIOS_UCHAR)pGammaParam->FisrtEntryIndex); // index auto-increments

            for ( i = 0; i < pGammaParam->EntryNum; i++)
            {
                CBIOS_U8    Byte1, Byte2, Byte3, Byte4;
                CBIOS_U16   R_10, G_10, B_10;

                Byte1 = cb_ReadU8(pcbe->pAdapterContext, 0x83c9);
                Byte2 = cb_ReadU8(pcbe->pAdapterContext, 0x83c9);
                Byte3 = cb_ReadU8(pcbe->pAdapterContext, 0x83c9);
                Byte4 = cb_ReadU8(pcbe->pAdapterContext, 0x83c9);

                R_10 = ((Byte2 >> 4) & 0x0f) | ((Byte1 & 0x3f) << 4);
                G_10 = ((Byte3 >> 2) & 0x3f) | ((Byte2 & 0x0f) << 6);
                B_10 = Byte4 | ((Byte3 & 0x03) << 8);

                pGammaTable[i].RgbLong = (R_10 << 20) | (G_10 << 10) | B_10;
            }
        }
        else
        {
            cbWaitVBlank(pcbe, (CBIOS_U8)pGammaParam->IGAIndex);

            cb_WriteU8(pcbe->pAdapterContext, 0x83c6, 0xff);
            cb_WriteU8(pcbe->pAdapterContext, 0x83c7, (CBIOS_UCHAR)pGammaParam->FisrtEntryIndex);

            for ( i = 0; i < pGammaParam->EntryNum; i++)
            {
                pGammaTable[i].RgbArray.Red = cb_ReadU8(pcbe->pAdapterContext, 0x83c9);
                pGammaTable[i].RgbArray.Green = cb_ReadU8(pcbe->pAdapterContext, 0x83c9);
                pGammaTable[i].RgbArray.Blue = cb_ReadU8(pcbe->pAdapterContext, 0x83c9);
                pGammaTable[i].RgbArray.Unused = 0;
            }
        }
    }

    //restore SR_47
    cbMMIOWriteReg(pcbe, SR_47, 0x00, 0xF0);
}

CBIOS_VOID cbSetLUT_Arise(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_GAMMA_PARA pGammaParam)
{
    REG_CR33_Pair   CR33Value = {0}, CR33Mask = {0xff};
    REG_MM332AC_Arise MM332ACValue = {0}, MM332ACMask = {0xffffffff};
    PCBIOS_LUT      pGammaTable = pGammaParam->pGammaTable;
    CBIOS_U32       FirstEntry = pGammaParam->FisrtEntryIndex;
    CBIOS_U32       i = 0;

    if(IGA1 == pGammaParam->IGAIndex)
    {
        cbMMIOWriteReg(pcbe, SR_47, 0x01, 0xF0);        //LUT1 read and write
    }
    else if(IGA2 == pGammaParam->IGAIndex)
    {
        cbMMIOWriteReg(pcbe, SR_47, 0x02, 0xF0);        //LUT2 read and write
    }
    else if(IGA3 == pGammaParam->IGAIndex)
    {
        cbMMIOWriteReg(pcbe, SR_47, 0x09, 0xF0);        //LUT3 read and write
    }
    else
    {
        cbMMIOWriteReg(pcbe, SR_47, 0x0a, 0xF0);        //LUT4 read and write
    }

    //Clear CR33 bit4, enable DAC write.
    CR33Value.Lock_DAC_Writes = 0;
    CR33Mask.Lock_DAC_Writes = 0;
    cbBiosMMIOWriteReg(pcbe, CR_33, CR33Value.Value, CR33Mask.Value, pGammaParam->IGAIndex);

    if (!pGammaParam->Flags.bUseIO)
    {
        // set index
        MM332ACValue.LUT_SSC_HDMI_INFO_DP_INFO_write_current_index = (CBIOS_UCHAR)FirstEntry;
        MM332ACMask.LUT_SSC_HDMI_INFO_DP_INFO_write_current_index = 0;
        cbMMIOWriteReg32(pcbe, 0x332ac, MM332ACValue.Value, MM332ACMask.Value);

        //write lut
        for ( i = 0; i < pGammaParam->EntryNum; i++)
        {
            cb_WriteU32(pcbe->pAdapterContext, 0x81a0, pGammaTable[i].RgbLong);
        }
    }
    else
    {
        if(!pGammaParam->Flags.bInterpolation)
        {
            cb_WriteU8(pcbe->pAdapterContext, 0x83c8, (CBIOS_UCHAR)FirstEntry); // index auto-increments

            for ( i = 0; i < pGammaParam->EntryNum; i++)
            {
                CBIOS_U8    Byte1, Byte2, Byte3, Byte4;
                CBIOS_U16   R_10, G_10, B_10;

                B_10 = (CBIOS_U16)(pGammaTable[i].RgbLong & 0x000003FF);
                G_10 = (CBIOS_U16)((pGammaTable[i].RgbLong>>10) & 0x000003FF);
                R_10 = (CBIOS_U16)((pGammaTable[i].RgbLong>>20) & 0x000003FF);

                Byte1 = (CBIOS_U8)((R_10 & 0x03F0) >>4) ;
                Byte2 = (CBIOS_U8)(((R_10 & 0x000F)<<4) |((G_10 & 0x03C0)>>6));
                Byte3 = (CBIOS_U8)(((G_10 & 0x003F)<<2) |((B_10 & 0x0300)>>8));
                Byte4 = (CBIOS_U8)(B_10 & 0x00FF);

                cb_WriteU8(pcbe->pAdapterContext, 0x83c9, Byte1);                      //3C9h
                cb_WriteU8(pcbe->pAdapterContext, 0x83c9, Byte2);                      //3C9h
                cb_WriteU8(pcbe->pAdapterContext, 0x83c9, Byte3);                      //3C9h
                cb_WriteU8(pcbe->pAdapterContext, 0x83c9, Byte4);                      //3C9h
            }
        }
        else
        {
            cb_WriteU8(pcbe->pAdapterContext, 0x83c6, 0xff);
            cb_WriteU8(pcbe->pAdapterContext, 0x83c8, (CBIOS_UCHAR)FirstEntry);

            for ( i = 0; i < pGammaParam->EntryNum; i++)
            {
                cb_WriteU8(pcbe->pAdapterContext, 0x83c9, pGammaTable[i].RgbArray.Red);
                cb_WriteU8(pcbe->pAdapterContext, 0x83c9, pGammaTable[i].RgbArray.Green);
                cb_WriteU8(pcbe->pAdapterContext, 0x83c9, pGammaTable[i].RgbArray.Blue);
            }
        }
    }

    //restore SR_47
    cbMMIOWriteReg(pcbe, SR_47, 0x00, 0xF0);
}

CBIOS_STATUS cbCECEnableDisable_Arise(PCBIOS_VOID pvcbe, PCBIOS_CEC_ENABLE_DISABLE_PARA pCECEnableDisablePara)
{
    PCBIOS_EXTENSION_COMMON pcbe = pvcbe;
    CBIOS_U32   CECMiscReg = 0;
    CBIOS_STATUS Status = CBIOS_OK;
    CBIOS_CEC_INDEX CECIndex = CBIOS_CEC_INDEX1;

    if (pCECEnableDisablePara == CBIOS_NULL)
    {
        Status = CBIOS_ER_NULLPOINTER;
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "CBiosCECEnableDisable_E3K: pCECEnableDisablePara is NULL!"));
    }
    else if (!pcbe->ChipCaps.IsSupportCEC)
    {
        Status = CBIOS_ER_HARDWARE_LIMITATION;
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "CBiosCECEnableDisable_E3K: Can't support CEC!"));
    }
    else if (pCECEnableDisablePara->CECIndex >= CBIOS_CEC_INDEX_COUNT)
    {
        Status = CBIOS_ER_INVALID_PARAMETER;
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "CBiosCECEnableDisable_E3K: invalid CEC index!"));
    }
    else
    {
        CECIndex = pCECEnableDisablePara->CECIndex;
        if (CECIndex == CBIOS_CEC_INDEX1)
        {
            CECMiscReg = 0x33148;
        }
        else if (CECIndex == CBIOS_CEC_INDEX2)
        {
            CECMiscReg = 0x33e34;
        }
        else if (CECIndex == CBIOS_CEC_INDEX3)
        {
            CECMiscReg = 0x34538;
        }
        else if (CECIndex == CBIOS_CEC_INDEX4)
        {
            CECMiscReg = 0X34c38;
        }


        if (pCECEnableDisablePara->bEnableCEC)
        {
            cbMMIOWriteReg32(pcbe, CECMiscReg, BIT18, ~BIT18);
            pcbe->CECPara[CECIndex].CECEnable = CBIOS_TRUE;
            //allocate logical address
            cbCECAllocateLogicalAddr(pcbe, CECIndex);
        }
        else
        {
            cbMMIOWriteReg32(pcbe, CECMiscReg, 0x00000000, ~BIT18);
            pcbe->CECPara[CECIndex].CECEnable = CBIOS_FALSE;
        }

        Status = CBIOS_OK;

    }

    return Status;

}

static CBIOS_STATUS cbUpdateCursorColor_Arise(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 IGAIndex, CBIOS_U32 Bg, CBIOS_U32 Fg)
{
    CBIOS_STATUS    bStatus = CBIOS_OK;

    REG_CR4A_Pair   Cr4a    = {0};
    REG_CR4B_Pair   Cr4b    = {0};
    CBIOS_U32       MmioBase = 0;

    if(IGA1 == IGAIndex)
    {
        MmioBase = MMIO_OFFSET_CR_GROUP_A;
    }
    else if(IGA2 == IGAIndex)
    {
        MmioBase = MMIO_OFFSET_CR_GROUP_B;
    }
    else if(IGA3 == IGAIndex)
    {
        MmioBase = MMIO_OFFSET_CR_GROUP_T;
    }
    else
    {
        MmioBase = MMIO_OFFSET_CR_GROUP_F;
    }

    // foreground color
    cbBiosMMIOReadReg(pcbe, CR_45, IGAIndex);//reset cr4a to '0' by reading cr45.
    Cr4a.HW_Cursor_Color_Foreground_Stack = Fg & 0xff;
    cb_WriteU8(pcbe->pAdapterContext, MmioBase + 0x4a, Cr4a.Value);
    Cr4a.HW_Cursor_Color_Foreground_Stack = (Fg >> 8)& 0xff;
    cb_WriteU8(pcbe->pAdapterContext, MmioBase + 0x4a, Cr4a.Value);
    Cr4a.HW_Cursor_Color_Foreground_Stack = (Fg >> 16)& 0xff;
    cb_WriteU8(pcbe->pAdapterContext, MmioBase + 0x4a, Cr4a.Value);
    Cr4a.HW_Cursor_Color_Foreground_Stack = (Fg >> 24)& 0xff;
    cb_WriteU8(pcbe->pAdapterContext, MmioBase + 0x4a, Cr4a.Value);
    // background color
    cbBiosMMIOReadReg(pcbe, CR_45, IGAIndex);//reset cr4a to '0' by reading cr45.
    Cr4b.HW_Cursor_Color_Background_Stack = Bg & 0xff;
    cb_WriteU8(pcbe->pAdapterContext, MmioBase + 0x4b, Cr4b.Value);
    Cr4b.HW_Cursor_Color_Background_Stack = (Bg >> 8) & 0xff;
    cb_WriteU8(pcbe->pAdapterContext, MmioBase + 0x4b, Cr4b.Value);
    Cr4b.HW_Cursor_Color_Background_Stack = (Bg >> 16) & 0xff;
    cb_WriteU8(pcbe->pAdapterContext, MmioBase + 0x4b, Cr4b.Value);
    Cr4b.HW_Cursor_Color_Background_Stack = (Bg >> 24) & 0xff;
    cb_WriteU8(pcbe->pAdapterContext, MmioBase + 0x4b, Cr4b.Value);

    return bStatus;
}

CBIOS_STATUS cbSetHwCursor_Arise(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_CURSOR_PARA pSetCursor)
{
    CBIOS_STATUS    bStatus = CBIOS_OK;
    CBIOS_U32       IGAIndex = pSetCursor->IGAIndex;
    PCBIOS_DISP_MODE_PARAMS  pModeParams = pcbe->DispMgr.pModeParams[IGAIndex];
    CBIOS_U32       CursorBaseAddr = 0;
    REG_MM33718_Arise CursorCtrl1RegValue = {0};
    REG_MM3371C_Arise CursorCtrl2RegValue = {0};
    REG_MM33720_Arise CursorBaseAddrRegValue = {0};
    REG_MM33728_Arise CursorEndPixelRegValue = {0};
    REG_MM81E4_Arise  CursorCscRegValue = {0};
    CBIOS_U32       CursorSize = 0;
    CBIOS_U8        StartX = 0;
    CBIOS_U8        StartY = 0;

    CursorCtrl1RegValue.Value = 0;

    // enable MMIO setting
    CursorCtrl1RegValue.Cursor_1_mmio_reg_en = 1;

    //check position
    if(!pSetCursor->bDisable)
    {
        if(pSetCursor->Position.PositionX < 0)
        {
            StartX = -pSetCursor->Position.PositionX;
            pSetCursor->Position.PositionX = 0;
        }
        if(pSetCursor->Position.PositionY < 0)
        {
            StartY = -pSetCursor->Position.PositionY;
            pSetCursor->Position.PositionY = 0;
        }
        if(pSetCursor->Position.CursorSize == CBIOS_CURSOR_SIZE_64x64)
        {
            CursorSize = 64;
        }
        else
        {
            CursorSize = 128;
        }
        if(StartX >= CursorSize || StartY >= CursorSize)
        {
            pSetCursor->bDisable = 1;
        }
    }

    if(pSetCursor->bDisable)
    {
        CursorCtrl1RegValue.Cursor_1_Enable = 0;
        CursorBaseAddrRegValue.Cursor_1_Enable_Work_Registers = 1;
        cbMMIOWriteReg32(pcbe, CursorControl1Index[IGAIndex], CursorCtrl1RegValue.Value, 0);
        cbMMIOWriteReg32(pcbe, CursorBaseAddrIndex[IGAIndex], CursorBaseAddrRegValue.Value, 0);
        return CBIOS_OK;
    }

    //enable cursor
    CursorCtrl1RegValue.Cursor_1_Enable = 1;

    //position
    CursorCtrl2RegValue.Cursor_1_X_Origin = pSetCursor->Position.PositionX;
    CursorCtrl2RegValue.Cursor_1_Y_Origin = pSetCursor->Position.PositionY;

    //start pixel
    CursorCtrl1RegValue.Cursor_1_Display_Start_X = StartX;
    CursorCtrl1RegValue.Cursor_1_Display_Start_Y = StartY;
    CursorEndPixelRegValue.Cursor_1_X_end = CursorSize;
    CursorEndPixelRegValue.Cursor_1_Y_end = CursorSize;

    //size
    CursorCtrl1RegValue.Cursor_1_Size = pSetCursor->Position.CursorSize;

    //type
    CursorCtrl1RegValue.Cursor_1_Type = pSetCursor->CursorAttrib.Type;
    if(pSetCursor->CursorAttrib.Type == CBIOS_MONO_CURSOR)
    {
        cbUpdateCursorColor_Arise(pcbe, pSetCursor->IGAIndex, pSetCursor->CursorAttrib.Color.BackGround, pSetCursor->CursorAttrib.Color.ForeGround);
    }

    //address
    CursorBaseAddr = (CBIOS_U32)(pSetCursor->CursorAttrib.CurAddr >> 5);
    CursorBaseAddrRegValue.Cursor_1_Base_Address = CursorBaseAddr;

    //rotation
    if (pSetCursor->bhMirror)
    {
        CursorCtrl1RegValue.Cursor_1_X_Rotation = 1;
    }
    if (pSetCursor->bvMirror)
    {
        CursorCtrl1RegValue.Cursor_1_Y_Rotation = 1;
    }

    //format
    CursorCscRegValue.Cursor1_Csc_output_Fmt = pModeParams->IGAOutColorSpace;

    CursorBaseAddrRegValue.Cursor_1_Enable_Work_Registers = 1;

    cbMMIOWriteReg32(pcbe, CursorCscIndex[IGAIndex], CursorCscRegValue.Value, 0);
    cbMMIOWriteReg32(pcbe, CursorControl1Index[IGAIndex], CursorCtrl1RegValue.Value, 0);
    cbMMIOWriteReg32(pcbe, CursorControl2Index[IGAIndex], CursorCtrl2RegValue.Value, 0);
    cbMMIOWriteReg32(pcbe, CursorEndPixelIndex[IGAIndex], CursorEndPixelRegValue.Value, 0);
    cbMMIOWriteReg32(pcbe, CursorBaseAddrIndex[IGAIndex], CursorBaseAddrRegValue.Value, 0);

    return  bStatus;
}

/*****************************************************************************************/
//
//Function:cbGetSysBiosInfo
//Discription:
//      This function will init pcbe scratch pad and related HW registers from system bios info
//Return:
//      TRUE --- Success, init success, in system bios info valid case
//      FALSE--- Fail, system bios info invalid, init fail
/*****************************************************************************************/
CBIOS_BOOL cbGetSysBiosInfo(PCBIOS_EXTENSION_COMMON pcbe)
{
    CBIOS_BOOL status = CBIOS_TRUE;

    if(pcbe->SysBiosInfo.bSysBiosInfoValid == CBIOS_TRUE)
    {
       pcbe->SysBiosInfo.BootUpDev = cbConvertVBiosDevBit2CBiosDevBit(pcbe->SysBiosInfo.BootUpDev);

       cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "\nCBIOS:cbGetSysBiosInfo:pcbe->SysBiosInfo.BootUpDev = 0x%x!\n", pcbe->SysBiosInfo.BootUpDev));
    }
    else
    {
        status = CBIOS_FALSE;
    }
    return status;
}
CBIOS_U8  cbGetCheckSum(CBIOS_U8* pByte, CBIOS_U32 uLength)
{
    CBIOS_U8 ByteVal=0;
    CBIOS_U32 i;
    for(i=0;i<uLength;i++)
    {
        ByteVal+=pByte[i];
    }
    return ByteVal;
}

CBIOS_VOID cbDisableStream_Arise(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 IGAIndex)
{
    REG_MM81FC_Arise     PSShadow = {0};
    REG_MM81C4_Arise     PSVsyncOff = {0};
    REG_MM33834_Arise    SSEnable = {0};
    REG_MM81D4_Arise     SSRightBase = {0};
    REG_MM8458_Arise     TQSEnable = {0};
    REG_MM844C_Arise     TQSRightBase = {0};
    REG_MM8200_Arise     OneShotTrig = {0};

    //use mmio
    cbBiosMMIOWriteReg(pcbe, CR_67, 0x08, ~0x08, IGAIndex);
    cbBiosMMIOWriteReg(pcbe, CR_64, 0x40, ~0x40, IGAIndex);

    PSShadow.PS_Disable = 1;
    PSVsyncOff.VsyncOff = 1;
    SSEnable.SS_Enable = 0;
    SSRightBase.SS_Vsync_Off_Flip = 1;
    TQSEnable.TS_En = 0;
    TQSRightBase.TS_Win1_Vsync_Off_Flip = 1;

    // vsync off
    cbMMIOWriteReg32(pcbe, PSVsyncOffIndex[IGAIndex], PSVsyncOff.Value, ~(PSVsyncOff.Value));
    cbMMIOWriteReg32(pcbe, SSRightBaseIndex[IGAIndex], SSRightBase.Value, ~(SSRightBase.Value));
    cbMMIOWriteReg32(pcbe, TQSRightBaseIndex[0][IGAIndex], TQSRightBase.Value, ~(TQSRightBase.Value));
    cbMMIOWriteReg32(pcbe, TQSRightBaseIndex[1][IGAIndex], TQSRightBase.Value, ~(TQSRightBase.Value));

    // disable stream
    cbMMIOWriteReg32(pcbe, PSShadowIndex[IGAIndex], PSShadow.Value, ~(PSShadow.Value));
    cbMMIOWriteReg32(pcbe, SSEnableIndex[IGAIndex], SSEnable.Value, ~(SSEnable.Value));
    cbMMIOWriteReg32(pcbe, TQSEnableIndex[0][IGAIndex], TQSEnable.Value, ~(TQSEnable.Value));
    cbMMIOWriteReg32(pcbe, TQSEnableIndex[1][IGAIndex], TQSEnable.Value, ~(TQSEnable.Value));

    OneShotTrig.Iga_Enable_Work = 1;
    cbMMIOWriteReg32(pcbe, OneShotTrigIndex[IGAIndex], OneShotTrig.Value, ~(OneShotTrig.Value));

}
