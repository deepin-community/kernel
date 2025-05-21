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

#define MODE_COLOR_DEPTH_CAPS CBIOS_COLORDEPTH8 | CBIOS_COLORDEPTH16 | CBIOS_COLORDEPTH32XRGB | \
                              CBIOS_COLORDEPTH32ARGB | CBIOS_COLORDEPTH32ABGR | CBIOS_COLORDEPTH2101010ARGB | \
                              CBIOS_COLORDEPTH2101010ABGR | CBIOS_COLORDEPTH16161616ABGRF
#define MODE_INFO_EXT_SIZE    sizeof(CBiosModeInfoExt)
#define TIMING_ATTRIB_SIZE    sizeof(CBIOS_TIMING_ATTRIB)

CBIOS_U8 KortekMonitorID[MONITORIDLENGTH] =
{
    0x2E, 0x8C, 0x19, 0x00
};

static CBIOS_BOOL EstModeSupportTbl[] = {
    CBIOS_TRUE,    // EDID23h[0]: 800x600@60Hz
    CBIOS_FALSE,   // EDID23h[1]: 800x600@56Hz
    CBIOS_TRUE,    // EDID23h[2]: 640x480@75Hz
    CBIOS_TRUE,    // EDID23h[3]: 640x480@72Hz
    CBIOS_FALSE,   // EDID23h[4]: 640x480@67Hz
    CBIOS_TRUE,    // EDID23h[5]: 640x480@60Hz
    CBIOS_FALSE,   // EDID23h[6]: 720x400@88Hz
    CBIOS_FALSE,   // EDID23h[7]: 720x400@70Hz
    CBIOS_TRUE,    // EDID24h[0]: 1280x1024@75Hz
    CBIOS_TRUE,    // EDID24h[1]: 1024x768@75Hz
    CBIOS_TRUE,    // EDID24h[2]: 1024x768@70Hz
    CBIOS_TRUE,    // EDID24h[3]: 1024x768@60Hz
    CBIOS_FALSE,   // EDID24h[4]: 1024x768@87HzInterlace
    CBIOS_FALSE,   // EDID24h[5]: 832x624@75Hz
    CBIOS_TRUE,    // EDID24h[6]: 800x600@75Hz
    CBIOS_TRUE,    // EDID24h[7]: 800x600@72Hz
    CBIOS_FALSE,   // EDID25h[7]: 1152x870@75Hz
};


// This table is for new CBIOS setting mode logic.
// When add item to this table, must notice:
// 1. Can't repeat the item in current table.
// 2. Must stored by sort.
static CBIOS_TIMING_ATTRIB AdapterTimingTbl[] =
{
{
    TIMING_ATTRIB_SIZE,
    0,
    640,350,
    8500,
    315000,
    0,
    VerNEGATIVE + HorPOSITIVE,
    832,            /*Hor Total Time*/
    640,            /*Hor Display End*/
    640,            /*Hor Blank Start*/
    640 + 192,      /*Hor Blank End*/
    640 + 32,       /*Hor Sync Start*/
    640 + 32 + 64,  /*Hor Sync End*/
    445,            /*Ver Total Time*/
    350,            /*Ver Display End*/
    350,            /*Ver Blank Start*/
    350 + 95,       /*Ver Blank End*/
    350 + 32,       /*Ver Sync Start*/
    350 + 32 + 3,   /*Ver Sync End*/
},
{
    TIMING_ATTRIB_SIZE,
    0,
    640,400,
    8500,
    315000,
    0,
    VerPOSITIVE + HorNEGATIVE,
    832,
    640,
    640,
    640 + 192,
    640 + 32,
    640 + 32 + 64,
    445,
    400,
    400,
    400 + 45,
    400 + 1,
    400 + 1 + 3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    720,400,
    8500,
    355000,
    0,
    VerPOSITIVE + HorNEGATIVE,
    936,
    720,
    720,
    720 + 216,
    720 + 36,
    720 + 36 + 72,
    446,
    400,
    400,
    400 + 46,
    400 + 1,
    400 + 1 + 3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    640,480,
    6000,
    251750,
    0,
    VerNEGATIVE+HorNEGATIVE,
    100*8,
    80*8,
    81*8,
    (100-1)*8 ,
    82*8,
    (82+12)*8   ,
    525,
    480,
    488+1,
    488+29,
    490,
    490+2,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    640,480,
    7200,
    315000,
    0,
    VerNEGATIVE+HorNEGATIVE,
    104*8,
    80*8,
    81*8,
    (81+22)*8,
    83*8,
    (83+5) *8,
    520,
    480,
    488,
    488+24,
    489,
    489+3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    640,480,
    7500,
    315000,
    0,
    VerNEGATIVE+HorNEGATIVE,
    105*8,
    80*8,
    80*8,
    (80+25)*8,
    82*8,
    (82+8) *8,
    500,
    480,
    480,
    480+20,
    481,
    481+3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    640,480,
    8500,
    360000,
    0,
    VerNEGATIVE+HorNEGATIVE,
    104*8,
    80*8,
    80*8,
    (80+24) *8,
    87*8,
    (87+7) *8,
    509,
    480,
    480,
    480+29,
    481,
    481+3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    800,600,
    5600,
    360000,
    0,
    VerPOSITIVE+HorPOSITIVE,
    1024,
    800,
    800,
    800 + 224,
    800 + 24,
    800 + 24 + 72,
    625,
    600,
    600,
    600 + 25,
    600 + 1,
    600 + 1 + 2,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    800,600,
    6000,
    400000,
    0,
    VerPOSITIVE+HorPOSITIVE,
    132*8,
    100*8,
    100*8,
    (100+32) *8,
    105*8,
    (105+16) *8,
    628,
    600,
    600,
    600+28,
    601,
    601+4,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    800,600,
    7200,
    500000,
    0,
    VerPOSITIVE+HorPOSITIVE,
    130*8,
    100*8,
    100*8,
    (100+30)*8,
    107*8,
    (107+15)*8,
    666,
    600,
    600,
    600+66,
    637,
    637+6,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    800,600,
    7500,
    495000,
    0,
    VerPOSITIVE+HorPOSITIVE,
    132*8,
    100*8,
    100*8,
    (100+32) *8,
    102*8,
    (102+10) *8,
    625,
    600,
    600,
    600+25,
    601,
    601+3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    800,600,
    8500,
    562500,
    0,
    VerPOSITIVE+HorPOSITIVE,
    131*8,
    100*8,
    100*8,
    (100+31)*8,
    104*8,
    (104+8) *8,
    631,
    600,
    600,
    600+31,
    601,
    601+3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    800,600,
    10000,
    672500,
    0,
    VerPOSITIVE+HorNEGATIVE,
    132*8,
    100*8,
    100*8,
    (100+32) *8,
    106*8,
    (106+10) *8,
    639,
    600,
    600,
    600+39,
    603,
    603+4,
},
/** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    800,600,
    12000,
    732500,
    0,
    VerNEGATIVE + HorPOSITIVE,
    960,
    800,
    800,
    800 + 160,
    800 + 48,
    800 + 48 + 32,
    636,
    600,
    600,
    600 + 36,
    600 + 3,
    600 + 3 + 4,
},
********************/
{
    TIMING_ATTRIB_SIZE,
    0,
    848,480,
    6000,
    337500,
    0,
    VerPOSITIVE + HorPOSITIVE,
    1088,
    848,
    848,
    848 + 240,
    848 + 16,
    848 + 16 +112,
    517,
    480,
    480,
    480 + 37,
    480 + 6,
    480 + 6 +8,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1024,768,
    6000,
    650000,
    0,
    VerNEGATIVE+HorNEGATIVE,
    168*8,
    128*8,
    128*8,
    (128+40) *8,
    131*8,
    (131+17) *8,
    806,
    768,
    768,
    768+38,
    771,
    771+6,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1024,768,
    7000,
    750000,
    0,
    VerNEGATIVE+HorNEGATIVE,
    166*8,
    128*8,
    128*8,
    (128+38) *8,
    131*8,
    (131+17) *8,
    806,
    768,
    768,
    768+38,
    771,
    771+6,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1024,768,
    7500,
    787500,
    0,
    VerPOSITIVE+HorPOSITIVE,
    164*8,
    128*8,
    128*8,
    (128+36)*8,
    130*8,
    (130+12) *8,
    800,
    768,
    768,
    768+32,
    769,
    769+3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1024,768,
    8500,
    945000,
    0,
    VerPOSITIVE+HorPOSITIVE,
    172*8,
    128*8,
    128*8,
    (128+44)*8,
    134*8,
    (134+12) *8,
    808,
    768,
    768,
    768+40,
    769,
    769+3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1024,768,
    10000,
    1122500,
    0,
    VerPOSITIVE+HorNEGATIVE,
    172*8,
    128*8,
    128*8,
    (128+44) *8,
    137*8,
    (137+13-1) *8,
    816,
    768,
    768,
    768+48,
    771,
    771+4,
},
/** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1024,768,
    12000,
    1155000,
    0,
    VerNEGATIVE + HorPOSITIVE,
    1184,
    1024,
    1024,
    1024 + 160,
    1024 + 48,
    1024 + 48 + 32,
    813,
    768,
    768,
    768 + 45,
    768 + 3,
    768 + 3 + 4,
},
*******************/
{
    TIMING_ATTRIB_SIZE,
    0,
    1024,768,
    12000,
    1390000,
    0,
    VerPOSITIVE+HorNEGATIVE,
    176*8,
    128*8,
    128*8,
    (128+48) *8,
    138*8,
    (138+14) *8,
    823,
    768,
    768,
    768+55,
    769,
    769+3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1152,864,
    7500,
    1080000,
    0,
    VerPOSITIVE + HorPOSITIVE,
    1600,
    1152,
    1152,
    1152 + 448,
    1152 + 64,
    1152 + 64 + 128,
    900,
    864,
    864,
    864 + 36,
    864 + 1,
    864 + 1 + 3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1280,720,
    5000,
    742500,
    0,
    VerPOSITIVE+HorPOSITIVE,
    1980,
    1280,
    1280,
    1980,
    1280+440,
    (1280+440+40) ,
    750,
    720,
    720,
    720+30,
    725,
    725+5,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1280,720,
    6000,
    742500,
    0,
    VerPOSITIVE+HorPOSITIVE,
    1650,
    1280,
    1280,
    1650 ,
    1280+110,
    (1280+110+40) ,
    750,
    720,
    720,
    720+30,
    725,
    725+5,
},
/** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1280,768,
    6000,
    682500,
    0,
    VerNEGATIVE + HorPOSITIVE,
    1440,
    1280,
    1280,
    1280 + 160,
    1280 + 48,
    1280 + 48 + 32,
    790,
    768,
    768,
    768 + 22,
    768 + 3,
    768 + 3 + 7,
},
*******************/
{
    TIMING_ATTRIB_SIZE,
    0,
    1280,768,
    6000,
    795000,
    0,
    VerPOSITIVE + HorNEGATIVE,
    1664,
    1280,
    1280,
    1280 + 384,
    1280 + 64,
    1280 + 64 + 128,
    798,
    768,
    768,
    768 + 30,
    768 + 3,
    768 + 3 + 7,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1280,768,
    7500,
    1022500,
    0,
    VerPOSITIVE + HorNEGATIVE,
    1696,
    1280,
    1280,
    1280 + 416,
    1280 + 80,
    1280 + 80 + 128,
    805,
    768,
    768,
    768 + 37,
    768 + 3,
    768 + 3 + 7,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1280,768,
    8500,
    1175000,
    0,
    VerPOSITIVE + HorNEGATIVE,
    1712,
    1280,
    1280,
    1280 + 432,
    1280 + 80,
    1280 + 80 + 136,
    809,
    768,
    768,
    768 + 41,
    768 + 3,
    768 + 3 + 7,
},
/** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1280,768,
    12000,
    1402500,
    0,
    VerNEGATIVE + HorPOSITIVE,
    1440,
    1280,
    1280,
    1280 + 160,
    1280 + 48,
    1280 + 48 + 32,
    813,
    768,
    768,
    768 + 45,
    768 + 3,
    768 + 3 + 7,
},
*********************/
/** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1280,800,
    6000,
    710000,
    0,
    VerNEGATIVE + HorPOSITIVE,
    1440,
    1280,
    1280,
    1280 + 160,
    1280 + 48,
    1280 + 48 + 32,
    823,
    800,
    800,
    800 + 23,
    800 + 3,
    800 + 3 + 6,
},
*****************/
{
    TIMING_ATTRIB_SIZE,
    0,
    1280,800,
    6000,
    835000,
    0,
    VerPOSITIVE + HorNEGATIVE,
    1680,
    1280,
    1280,
    1280 + 400,
    1280 + 72,
    1280 + 72 + 128,
    831,
    800,
    800,
    800 + 31,
    800 + 3,
    800 + 3 + 6,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1280,800,
    7500,
    1065000,
    0,
    VerPOSITIVE + HorNEGATIVE,
    1696,
    1280,
    1280,
    1280 + 416,
    1280 + 80,
    1280 + 80 + 128,
    838,
    800,
    800,
    800 + 38,
    800 + 3,
    800 + 3 + 6,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1280,800,
    8500,
    1225000,
    0,
    VerPOSITIVE + HorNEGATIVE,
    1712,
    1280,
    1280,
    1280 + 432,
    1280 + 80,
    1280 + 80 + 136,
    843,
    800,
    800,
    800 + 43,
    800 + 3,
    800 + 3 + 6,
},
/** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1280,800,
    12000,
    1462500,
    0,
    VerNEGATIVE + HorPOSITIVE,
    1440,
    1280,
    1280,
    1280 + 160,
    1280 + 48,
    1280 + 48 + 32,
    847,
    800,
    800,
    800 + 47,
    800 + 3,
    800 + 3 + 6,
},
******************/
{
    TIMING_ATTRIB_SIZE,
    0,
    1280,960,
    6000,
    1080000,
    0,
    VerPOSITIVE + HorPOSITIVE,
    1800,
    1280,
    1280,
    1280 + 520,
    1280 + 96,
    1280 + 96 + 112,
    1000,
    960,
    960,
    960 + 40,
    960 + 1,
    960 + 1 + 3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1280,960,
    8500,
    1485000,
    0,
    VerPOSITIVE + HorPOSITIVE,
    1728,
    1280,
    1280,
    1280 + 448,
    1280 + 64,
    1280 + 64 + 160,
    1011,
    960,
    960,
    960 + 51,
    960 + 1,
    960 + 1 + 3,
},
/*** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1280,960,
    12000,
    1755000,
    0,
    VerNEGATIVE + HorPOSITIVE,
    1440,
    1280,
    1280,
    1280 + 160,
    1280 + 48,
    1280 + 48 + 32,
    1017,
    960,
    960,
    960 + 57,
    960 + 3,
    960 + 3 + 4,
},
*********************/
{
     TIMING_ATTRIB_SIZE,
     0,
     1280,1024,
     6000,
     1080000,
     0,
     VerPOSITIVE+HorPOSITIVE,
     211*8,
     160*8,
     160*8,
     (160+51)*8,
     166*8,
     (166+14) *8,
     1066,
     1024,
     1024,
     1024+42,
     1025,
     1025+3,
},
{
     TIMING_ATTRIB_SIZE,
     0,
     1280,1024,
     7500,
     1350000,
     0,
     VerPOSITIVE+HorPOSITIVE,
     211*8,
     160*8,
     160*8,
     (160+51) *8,
     162*8,
     (162+18) *8,
     1066,
     1024,
     1024,
     1024+42,
     1025,
     1025+3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1280,1024,
    8500,
    1575000,
    0,
    VerPOSITIVE+HorPOSITIVE,
    1728,
    1280,
    1280,
    1280 + 448,
    1280 + 64,
    1280 + 64 + 160,
    1072,
    1024,
    1024,
    1024 + 48,
    1024 + 1,
    1024 + 1 + 3,
},
/*** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1280,1024,
    12000,
    1872500,
    0,
    VerNEGATIVE + HorPOSITIVE,
    1440,
    1280,
    1280,
    1280 + 160,
    1280 + 48,
    1280 + 48 + 32,
    1084,
    1024,
    1024,
    1024 + 60,
    1024 + 3,
    1024 + 3 + 7,
},
**********************/
{
    TIMING_ATTRIB_SIZE,
    0,
    1360,768,
    6000,
    855000,
    0,
    VerPOSITIVE+HorPOSITIVE,
    1792,
    1360,
    1360,
    1360 + 432,
    1360 + 64,
    1360 + 64 + 112,
    795,
    768,
    768,
    768 + 27,
    768 + 3,
    768 + 3 + 6,
},
/*** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1360,768,
    12000,
    1482500,
    0,
    VerNEGATIVE + HorPOSITIVE,
    1520,
    1360,
    1360,
    1360 + 160,
    1360 + 48,
    1360 + 48 + 32,
    813,
    768,
    768,
    768 + 45,
    768 + 3,
    768 + 3 + 5,
},
*********************/
/*** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1366,768,
    6000,
    720000,
    0,
    VerPOSITIVE+HorPOSITIVE,
    1500,
    1366,
    1366,
    1366 + 134,
    1366 + 14,
    1366 + 14 + 56,
    800,
    768,
    768,
    768 + 32,
    768 + 1,
    768 + 1 + 3,
},
******************/
{
    TIMING_ATTRIB_SIZE,
    0,
    1366,768,
    6000,
    855000,
    0,
    VerPOSITIVE+HorPOSITIVE,
    1792,
    1366,
    1366,
    1366 + 426,
    1366 + 70,
    1366 + 70 + 143,
    798,
    768,
    768,
    768 + 30,
    768 + 3,
    768 + 3 + 3,
},
/***** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1400,1050,
    6000,
    1010000,
    0,
    VerNEGATIVE + HorPOSITIVE,
    1560,
    1400,
    1400,
    1400 + 160,
    1400 + 48,
    1400 + 48 + 32,
    1080,
    1050,
    1050,
    1050 + 30,
    1050 + 3,
    1050 + 3 + 4,
},
*********************/
{
    TIMING_ATTRIB_SIZE,
    0,
    1400,1050,
    6000,
    1217500,
    0,
    VerPOSITIVE + HorNEGATIVE,
    1864,
    1400,
    1400,
    1400 + 464,
    1400 + 88,
    1400 + 88 + 144,
    1089,
    1050,
    1050,
    1050 + 39,
    1050 + 3,
    1050 + 3 + 4,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1400,1050,
    7500,
    1560000,
    0,
    VerPOSITIVE + HorNEGATIVE,
    1896,
    1400,
    1400,
    1400 + 496,
    1400 + 104,
    1400 + 104 + 144,
    1099,
    1050,
    1050,
    1050 + 49,
    1050 + 3,
    1050 + 3 + 4,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1400,1050,
    8500,
    1795000,
    0,
    VerPOSITIVE + HorNEGATIVE,
    1912,
    1400,
    1400,
    1400 + 512,
    1400 + 104,
    1400 + 104 + 152,
    1105,
    1050,
    1050,
    1050 + 55,
    1050 + 3,
    1050 + 3 + 4,
},
/******* Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1400,1050,
    12000,
    2080000,
    0,
    VerNEGATIVE + HorPOSITIVE,
    1560,
    1400,
    1400,
    1400 + 160,
    1400 + 48,
    1400 + 48 + 32,
    1112,
    1050,
    1050,
    1050 + 62,
    1050 + 3,
    1050 + 3 + 4,
},
***********************/

/****** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1440,900,
    6000,
    887500,
    0,
    VerNEGATIVE + HorPOSITIVE,
    1600,
    1440,
    1440,
    1440 + 160,
    1440 + 48,
    1440 + 48 + 32,
    926,
    900,
    900,
    900 + 26,
    900 + 3,
    900 + 3 + 6,
},
**********************/
{
    TIMING_ATTRIB_SIZE,
    0,
    1440,900,
    6000,
    1065000,
    0,
    VerPOSITIVE + HorNEGATIVE,
    1904,
    1440,
    1440,
    1440 + 464,
    1440 + 80,
    1440 + 80 + 152,
    934,
    900,
    900,
    900 + 34,
    900 + 3,
    900 + 3 + 6,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1440,900,
    7500,
    1367500,
    0,
    VerPOSITIVE + HorNEGATIVE,
    1936,
    1440,
    1440,
    1440 + 496,
    1440 + 96,
    1440 + 96 + 152,
    942,
    900,
    900,
    900 + 42,
    900 + 3,
    900 + 3 + 6,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1440,900,
    8500,
    1570000,
    0,
    VerPOSITIVE + HorNEGATIVE,
    1952,
    1440,
    1440,
    1440 + 512,
    1440 + 104,
    1440 + 104 + 152,
    948,
    900,
    900,
    900 + 48,
    900 + 3,
    900 + 3 + 6,
},
/***** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1440,900,
    12000,
    1827500,
    0,
    VerNEGATIVE + HorPOSITIVE,
    1600,
    1440,
    1440,
    1440 + 160,
    1440 + 48,
    1440 + 48 + 32,
    953,
    900,
    900,
    900 + 53,
    900 + 3,
    900 + 3 + 6,
},
*********************/
{
    TIMING_ATTRIB_SIZE,
    0,
    1600,900,
    6000,
    1080000,
    0,
    VerPOSITIVE+HorPOSITIVE,
    1800,
    1600,
    1600,
    1600 + 200,
    1600 + 24,
    1600 + 24 + 80,
    1000,
    900,
    900,
    900 + 100,
    900 + 1,
    900 + 1 + 3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1600,1200,
    6000,
    1620000,
    0,
    VerPOSITIVE+HorPOSITIVE,
    2160,
    1600,
    1600,
    1600 + 560,
    1600 + 64,
    1600 + 64 + 192,
    1250,
    1200,
    1200,
    1200 + 50,
    1200 + 1,
    1200 + 1 + 3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1600,1200,
    6500,
    1755000,
    0,
    VerPOSITIVE+HorPOSITIVE,
    2160,
    1600,
    1600,
    1600 + 560,
    1600 + 64,
    1600 + 64 + 192,
    1250,
    1200,
    1200,
    1200 + 50,
    1200 + 1,
    1200 + 1 + 3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1600,1200,
    7000,
    1890000,
    0,
    VerPOSITIVE+HorPOSITIVE,
    2160,
    1600,
    1600,
    1600 + 560,
    1600 + 64,
    1600 + 64 + 192,
    1250,
    1200,
    1200,
    1200 + 50,
    1200 + 1,
    1200 + 1 + 3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1600,1200,
    7500,
    2025000,
    0,
    VerPOSITIVE+HorPOSITIVE,
    2160,
    1600,
    1600,
    1600 + 560,
    1600 + 64,
    1600 + 64 + 192,
    1250,
    1200,
    1200,
    1200 + 50,
    1200 + 1,
    1200 + 1 + 3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1600,1200,
    8500,
    2295000,
    0,
    VerPOSITIVE+HorPOSITIVE,
    2160,
    1600,
    1600,
    1600 + 560,
    1600 + 64,
    1600 + 64 + 192,
    1250,
    1200,
    1200,
    1200 + 50,
    1200 + 1,
    1200 + 1 + 3,
},
/***** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1600,1200,
    12000,
    2682500,
    0,
    VerNEGATIVE + HorPOSITIVE,
    1760,
    1600,
    1600,
    1600 + 160,
    1600 + 48,
    1600 + 48 + 32,
    1271,
    1200,
    1200,
    1200 + 71,
    1200 + 3,
    1200 + 3 + 4,
},
********************/
/***** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1680,1050,
    6000,
    1190000,
    0,
    VerNEGATIVE + HorPOSITIVE,
    1840,
    1680,
    1680,
    1680 + 160,
    1680 + 48,
    1680 + 48 + 32,
    1080,
    1050,
    1050,
    1050 + 30,
    1050 + 3,
    1050 + 3 + 6,
},
******************/
{
    TIMING_ATTRIB_SIZE,
    0,
    1680,1050,
    6000,
    1462500,
    0,
    VerPOSITIVE + HorNEGATIVE,
    2240,
    1680,
    1680,
    1680 + 560,
    1680 + 104,
    1680 + 104 + 176,
    1089,
    1050,
    1050,
    1050 + 39,
    1050 + 3,
    1050 + 3 + 6,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1680,1050,
    7500,
    1870000,
    0,
    VerPOSITIVE + HorNEGATIVE,
    2272,
    1680,
    1680,
    1680 + 592,
    1680 + 120,
    1680 + 120 + 176,
    1099,
    1050,
    1050,
    1050 + 49,
    1050 + 3,
    1050 + 3 + 6,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1680,1050,
    8500,
    2147500,
    0,
    VerPOSITIVE + HorNEGATIVE,
    2288,
    1680,
    1680,
    1680 + 608,
    1680 + 128,
    1680 + 128 + 176,
    1105,
    1050,
    1050,
    1050 + 55,
    1050 + 3,
    1050 + 3 + 6,
},
/***** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1680,1050,
    12000,
    2455000,
    0,
    VerNEGATIVE + HorPOSITIVE,
    1840,
    1680,
    1680,
    1680 + 160,
    1680 + 48,
    1680 + 48 + 32,
    1112,
    1050,
    1050,
    1050 + 62,
    1050 + 3,
    1050 + 3 + 6,
},
*********************/
{
    TIMING_ATTRIB_SIZE,
    0,
    1792,1344,
    6000,
    2047500,
    0,
    VerPOSITIVE + HorNEGATIVE,
    2448,
    1792,
    1792,
    1792 + 656,
    1792 + 128,
    1792 + 128 + 200,
    1394,
    1344,
    1344,
    1344 + 50,
    1344 + 1,
    1344 + 1 + 3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1792,1344,
    7500,
    2610000,
    0,
    VerPOSITIVE + HorNEGATIVE,
    2456,
    1792,
    1792,
    1792 + 664,
    1792 + 96,
    1792 + 96 + 216,
    1417,
    1344,
    1344,
    1344 + 73,
    1344 + 1,
    1344 + 1 + 3,
},
/*** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1792,1344,
    12000,
    3332500,
    0,
    VerNEGATIVE + HorPOSITIVE,
    1952,
    1792,
    1792,
    1792 + 160,
    1792 + 48,
    1792 + 48 + 32,
    1423,
    1344,
    1344,
    1344 + 79,
    1344 + 3,
    1344 + 3 + 4,
},
****************/
{
    TIMING_ATTRIB_SIZE,
    0,
    1856,1392,
    6000,
    2182500,
    0,
    VerPOSITIVE + HorNEGATIVE,
    2528,
    1856,
    1856,
    1856 + 672,
    1856 + 96,
    1856 + 96 + 224,
    1439,
    1392,
    1392,
    1392 + 47,
    1392 + 1,
    1392 + 1 + 3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1856,1392,
    7500,
    2880000,
    0,
    VerPOSITIVE + HorNEGATIVE,
    2560,
    1856,
    1856,
    1856 + 704,
    1856 + 128,
    1856 + 128 + 224,
    1500,
    1392,
    1392,
    1392 + 108,
    1392 + 1,
    1392 + 1 + 3,
},
/**** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1856,1392,
    12000,
    3565000,
    0,
    VerNEGATIVE + HorPOSITIVE,
    2016,
    1856,
    1856,
    1856 + 160,
    1856 + 48,
    1856 + 48 + 32,
    1474,
    1392,
    1392,
    1392 + 82,
    1392 + 3,
    1392 + 3 + 4,
},
****************/
{
    TIMING_ATTRIB_SIZE,
    0,
    1920,1080,
    5000,
    1485000,
    0,
    VerPOSITIVE+HorPOSITIVE,
    2640,
    1920,
    1920,
    2640,
    1920+528,
    (1920+528+44) ,
    1125,
    1080,
    1080,
    1080+45,
    1084,
    1084+5,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1920,1080,
    6000,
    1485000,
    0,
    VerPOSITIVE+HorPOSITIVE,
    2200,
    1920,
    1920,
    2200 ,
    1920+88,
    (1920+88+44) ,
    1125,
    1080,
    1080,
    1080+45,
    1084,
    1084+5,
},
/*{
    TIMING_ATTRIB_SIZE,
    0,
    1920,1080,
    12000,
    3687000,
    0,
    VerPOSITIVE+HorNEGATIVE,
    332*8,
    240*8,
    240*8,
    (240+92) *8,
    259*8,
    (259+27) *8,
    1157,
    1080,
    1080,
    1080+77,
    1081,
    1081+3,
},*/
/*** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1920,1200,
    6000,
    1540000,
    0,
    VerNEGATIVE + HorPOSITIVE,
    2080,
    1920,
    1920,
    1920 + 160,
    1920 + 48,
    1920 + 48 + 32,
    1235,
    1200,
    1200,
    1200 + 35,
    1200 + 3,
    1200 + 3 + 6,
},
**************/
{
    TIMING_ATTRIB_SIZE,
    0,
    1920,1200,
    6000,
    1932500,
    0,
    VerPOSITIVE+HorNEGATIVE,
    2592,
    1920,
    1920,
    1920 + 672,
    1920 + 136,
    1920 + 136 + 200,
    1245,
    1200,
    1200,
    1200 + 45,
    1200 + 3,
    1200 + 3 + 6,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1920,1200,
    7500,
    2452500,
    0,
    VerPOSITIVE+HorNEGATIVE,
    2608,
    1920,
    1920,
    1920 + 688,
    1920 + 136,
    1920 + 136 + 208,
    1255,
    1200,
    1200,
    1200 + 55,
    1200 + 3,
    1200 + 3 + 6,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1920,1200,
    8500,
    2812500,
    0,
    VerPOSITIVE+HorNEGATIVE,
    2624,
    1920,
    1920,
    1920 + 704,
    1920 + 144,
    1920 + 144 + 208,
    1262,
    1200,
    1200,
    1200 + 62,
    1200 + 3,
    1200 + 3 + 6,
},
/**** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1920,1200,
    12000,
    3170000,
    0,
    VerNEGATIVE + HorPOSITIVE,
    2080,
    1920,
    1920,
    1920 + 160,
    1920 + 48,
    1920 + 48 + 32,
    1271,
    1200,
    1200,
    1200 + 71,
    1200 + 3,
    1200 + 3 + 6,
},
**********************/
{
    TIMING_ATTRIB_SIZE,
    0,
    1920,1440,
    6000,
    2340000,
    0,
    VerPOSITIVE+HorNEGATIVE,
    2600,
    1920,
    1920,
    1920 + 680,
    1920 + 128,
    1920 + 128 + 208,
    1500,
    1440,
    1440,
    1440 + 60,
    1440 + 1,
    1440 + 1 + 3,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    1920,1440,
    7500,
    2970000,
    0,
    VerPOSITIVE+HorNEGATIVE,
    2640,
    1920,
    1920,
    1920 + 720,
    1920 + 144,
    1920 + 144 + 224,
    1500,
    1440,
    1440,
    1440 + 60,
    1440 + 1,
    1440 + 1 + 3,
},
/*** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    1920,1440,
    12000,
    3805000,
    0,
    VerNEGATIVE + HorPOSITIVE,
    2080,
    1920,
    1920,
    1920 + 160,
    1920 + 48,
    1920 + 48 + 32,
    1525,
    1440,
    1440,
    1440 + 85,
    1440 + 3,
    1440 + 3 + 4,
},
******************/
{
    TIMING_ATTRIB_SIZE,
    0,
    2048,1152,
    6000,
    1620000,
    0,
    VerPOSITIVE+HorPOSITIVE,
    2250,
    2048,
    2048,
    2048 + 202,
    2048 + 26,
    2048 + 26 + 80,
    1200,
    1152,
    1152,
    1152 + 48,
    1152 + 1,
    1152 + 1 + 3,
},
/*** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    2560,1600,
    6000,
    2685000,
    0,
    VerNEGATIVE + HorPOSITIVE,
    2720,
    2560,
    2560,
    2560 + 160,
    2560 + 48,
    2560 + 48 + 32,
    1646,
    1600,
    1600,
    1600 + 46,
    1600 + 3,
    1600 + 3 + 6,
},
****************/
{
    TIMING_ATTRIB_SIZE,
    0,
    2560,1600,
    6000,
    3485000,
    0,
    VerPOSITIVE+HorNEGATIVE,
    3504,
    2560,
    2560,
    2560 + 944,
    2560 + 192,
    2560 + 192 + 280,
    1658,
    1600,
    1600,
    1600 + 58,
    1600 + 3,
    1600 + 3 + 6,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    2560,1600,
    7500,
    4432500,
    0,
    VerPOSITIVE+HorNEGATIVE,
    3536,
    2560,
    2560,
    2560 + 976,
    2560 + 208,
    2560 + 208 + 280,
    1672,
    1600,
    1600,
    1600 + 72,
    1600 + 3,
    1600 + 3 + 6,
},
{
    TIMING_ATTRIB_SIZE,
    0,
    2560,1600,
    8500,
    5052500,
    0,
    VerPOSITIVE+HorNEGATIVE,
    3536,
    2560,
    2560,
    2560 + 976,
    2560 + 208,
    2560 + 208 + 280,
    1682,
    1600,
    1600,
    1600 + 82,
    1600 + 3,
    1600 + 3 + 6,
}
/** Reduced Blanking
{
    TIMING_ATTRIB_SIZE,
    0,
    2560,1600,
    12000,
    5527500,
    0,
    VerNEGATIVE + HorPOSITIVE,
    2720,
    2560,
    2560,
    2560 + 160,
    2560 + 48,
    2560 + 48 + 32,
    1694,
    1600,
    1600,
    1600 + 94,
    1600 + 3,
    1600 + 3 + 6,
},
***************/
};

static CBiosModeInfoExt AdapterModeList[] =
{
{
    MODE_INFO_EXT_SIZE,
    640,
    480,
    6000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_ADAPTERMODE,
    CBIOS_TYPE_NONE,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {0},
},
{
    MODE_INFO_EXT_SIZE,
    800,
    600,
    6000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_ADAPTERMODE,
    CBIOS_TYPE_NONE,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {0},
},
{
    MODE_INFO_EXT_SIZE,
    1024,
    768,
    6000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_ADAPTERMODE,
    CBIOS_TYPE_NONE,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {0},
},
{
    MODE_INFO_EXT_SIZE,
    1280,
    720,
    6000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_ADAPTERMODE,
    CBIOS_TYPE_NONE,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {0},
},
{
	MODE_INFO_EXT_SIZE,
	1280,
	800,
	6000,
	CBIOS_PROGRESSIVECAP,
	CBIOS_ADAPTERMODE,
	CBIOS_TYPE_NONE,
	MODE_COLOR_DEPTH_CAPS,
	0,
	0,
	0,
},
{
    MODE_INFO_EXT_SIZE,
    1280,
    1024,
    6000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_ADAPTERMODE,
    CBIOS_TYPE_NONE,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {0},
},
{
	MODE_INFO_EXT_SIZE,
	1366,
	768,
	6000,
	CBIOS_PROGRESSIVECAP,
	CBIOS_ADAPTERMODE,
	CBIOS_TYPE_NONE,
	MODE_COLOR_DEPTH_CAPS,
	0,
	0,
	0,
},
{
	MODE_INFO_EXT_SIZE,
	1440,
	900,
	6000,
	CBIOS_PROGRESSIVECAP,
	CBIOS_ADAPTERMODE,
	CBIOS_TYPE_NONE,
	MODE_COLOR_DEPTH_CAPS,
	0,
	0,
	0,
},
{
	MODE_INFO_EXT_SIZE,
	1680,
	1050,
	6000,
	CBIOS_PROGRESSIVECAP,
	CBIOS_ADAPTERMODE,
	CBIOS_TYPE_NONE,
	MODE_COLOR_DEPTH_CAPS,
	0,
	0,
	0,
},
{
    MODE_INFO_EXT_SIZE,
    1920,
    1080,
    6000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_ADAPTERMODE,
    CBIOS_TYPE_NONE,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {0},
},
};


static CBIOS_TIMING_ATTRIB HDMIFormatTimingTbl[] =
{
//normal VIC begin
//if new VICs are added, make sure to update CBIOS_HDMI_NORMAL_VIC_COUNTS
//1
{
    TIMING_ATTRIB_SIZE,
    1,
    640,480,                               //XResolution,  YResolution
    6000,                                  //RefreshRate
    252000,                                 //DCLK
    1,
    VerNEGATIVE+HorNEGATIVE,               //HVPolarity:   Hor/Ver Sync Polarity(MISC:11000000B)
    800,                                 //HorTotal:     SR66_0+SR60=Round(Value/8)-5
    640,                                  //HorDisEnd:    SR66_1+SR61=Round(Value/8)-1
    640,                                    //HorBStart:    SR66_2+SR62=Round(Value/8)
    800 ,                  //HorBEnd:      SR66[3]+SR65[7]+SR63[0-4]=Round(Value/8)&   0x003F*/
    656,                                    //HorSyncStart: SR66[4]+SR64=Round(Value/8)
    (656+96)   ,                    //HorSyncEnd:    SR66[5]+SR65[0-4] =Round(Value/8) &   0x001F*/
    525,                                 //VerTotal:     SR6E[0-3]+SR68=Value-2
    480,                                 //VerDisEnd:   SR6E[4-7]+SR69=Value-1
    480,                                   //VerBStart:    SR6F[0-3]+SR6A=Value
    525,                   //VerBEnd:      SR6B
    490,                                   //VerSyncStart: SR6F[4-7]+SR6C
    490+2,                     //VerSyncEnd:   SR6B
},
//2
{
    TIMING_ATTRIB_SIZE,
    2,
    720,480,
    6000,
    270270,
    1,
    VerNEGATIVE+HorNEGATIVE,
    858,
    720,
    720,
    858 ,
    736,
    (736+62) ,
    525,
    480,
    480,
    525,
    489,
    489+6,
},
//3
{
    TIMING_ATTRIB_SIZE,
    3,
    720,480,
    6000,
    270270,
    2,
    VerNEGATIVE+HorNEGATIVE,
    858,
    720,
    720,
    858 ,
    736,
    (736+62) ,
    525,
    480,
    480,
    525,
    489,
    489+6,
},
//4
{
    TIMING_ATTRIB_SIZE,
    4,
    1280,720,
    6000,
    742500,
    2,
    VerPOSITIVE+HorPOSITIVE,
    1650,
    1280,
    1280,
    1650 ,
    1280+110,
    (1280+110+40) ,
    750,
    720,
    720,
    720+30,
    725,
    725+5,
},
//5
{
    TIMING_ATTRIB_SIZE,
    5,
    1920,1080,
    6000,
    742500,
    2,
    VerPOSITIVE+HorPOSITIVE,
    2200,
    1920,
    1920,
    2200 ,
    1920+88,
    (1920+88+44) ,
    1125,
    1080,
    1080,
    1080+22+23,
    1084,
    1084+10,
},
//6
{
    TIMING_ATTRIB_SIZE,
    6,
    720,480,
    6000,
    270270,
    1,
    VerNEGATIVE+HorNEGATIVE,
    858,
    720,
    720,
    858 ,
    720+38/2,
    720+38/2+124/2 ,
    525,
    480,
    480,
    525,
    489-1, //adjust to correct 480i timing
    489-1+6,
},
//7
{
    TIMING_ATTRIB_SIZE,
    7,
    720,480,
    6000,
    270270,
    2,
    VerNEGATIVE+HorNEGATIVE,
    858,
    720,
    720,
    858,
    720+38/2,
    720+38/2+124/2 ,
    525,
    480,
    480,
    525,
    489-1, //adjust to correct 480i timing
    489-1+6,
},
//8
{
    TIMING_ATTRIB_SIZE,
    8,
    720,240,
    6000,
    270270,
    1,
    VerNEGATIVE+HorNEGATIVE,
    858,
    720,
    858,
    858,
    720+38/2,
    720+38/2+124/2,
    262,
    240,
    240,
    240+22,
    244,
    244+3,
},
//9
{
    TIMING_ATTRIB_SIZE,
    9,
    720,240,
    6000,
    270270,
    2,
    VerNEGATIVE+HorNEGATIVE,
    858,
    720,
    858,
    858,
    720+38/2,
    720+38/2+124/2,
    262,
    240,
    240,
    240+22,
    244,
    244+3,
},
//10
{
    TIMING_ATTRIB_SIZE,
    10,
    2880,480,
    6000,
    540540,
    1,
    VerNEGATIVE+HorNEGATIVE,
    3432,
    2880,
    2880,
    3432 ,
    2880+76,
    (2880+76+248) ,
    525,
    480,
    480,
    525,
    488,
    488+6,
},
//11
{
    TIMING_ATTRIB_SIZE,
    11,
    2880,480,
    6000,
    540540,
    2,
    VerNEGATIVE+HorNEGATIVE,
    3432,
    2880,
    2880,
    3432,
    2880+76,
    (2880+76+248) ,
    525,
    480,
    480,
    525,
    488,
    488+6,
},
//12
{
    TIMING_ATTRIB_SIZE,
    12,
    2880,240,
    6000,
    540540,
    1,
    VerNEGATIVE+HorNEGATIVE,
    3432,
    2880,
    2880,
    3432,
    2880+76,
    (2880+76+248) ,
    262,
    240,
    240,
    240+22,
    244,
    244+3,
},
//13
{
    TIMING_ATTRIB_SIZE,
    13,
    2880,240,
    6000,
    540540,
    2,
    VerNEGATIVE+HorNEGATIVE,
    3432,
    2880,
    2880,
    3432 ,
    2880+76,
    (2880+76+248) ,
    263,
    240,
    240,
    240+23,
    245,
    245+3,
},
//14
{
    TIMING_ATTRIB_SIZE,
    14,
    1440,480,
    6000,
    540540,
    1,
    VerNEGATIVE+HorNEGATIVE,
    1716,
    1440,
    1440,
    1716,
    1472,
    (1472+124) ,
    525,
    480,
    480,
    480+45,
    490,
    490+6,
},
//15
{
    TIMING_ATTRIB_SIZE,
    15,
    1440,480,
    6000,
    540540,
    2,
    VerNEGATIVE+HorNEGATIVE,
    1716,
    1440,
    1440,
    1716,
    1472,
    (1472+124) ,
    525,
    480,
    480,
    480+45,
    490,
    490+6,
},
//16
{
    TIMING_ATTRIB_SIZE,
    16,
    1920,1080,
    6000,
    1485000,
    2,
    VerPOSITIVE+HorPOSITIVE,
    2200,
    1920,
    1920,
    2200 ,
    1920+88,
    (1920+88+44) ,
    1125,
    1080,
    1080,
    1080+45,
    1084,
    1084+5,
},
//17
{
    TIMING_ATTRIB_SIZE,
    17,
    720,576,
    5000,
    270000,
    1,
    VerNEGATIVE+HorNEGATIVE,
    864,
    720,
    720,
    864,
    732,
    (732+64) ,
    625,
    576,
    576,
    625,
    581,
    581+5,
},
//18
{
    TIMING_ATTRIB_SIZE,
    18,
    720,576,
    5000,
    270000,
    2,
    VerNEGATIVE+HorNEGATIVE,
    864,
    720,
    720,
    864 ,
    732,
    (732+64) ,
    625,
    576,
    576,
    625,
    581,
    581+5,
},
//19
{
    TIMING_ATTRIB_SIZE,
    19,
    1280,720,
    5000,
    742500,
    2,
    VerPOSITIVE+HorPOSITIVE,
    1980,
    1280,
    1280,
    1980,
    1280+440,
    (1280+440+40) ,
    750,
    720,
    720,
    720+30,
    725,
    725+5,
},
//20
{
    TIMING_ATTRIB_SIZE,
    20,
    1920,1080,
    5000,
    742500,
    2,
    VerPOSITIVE+HorPOSITIVE,
    2640,
    1920,
    1920,
    2640,
    1920+528,
    (1920+528+44) ,
    1125,
    1080,
    1080,
    1080+22+23,
    1084,
    1084+10,
},
//21
{
    TIMING_ATTRIB_SIZE,
    21,
    720,576,
    5000,
    270000,
    1,
    VerNEGATIVE+HorNEGATIVE,
    864,
    720,
    720,
    864,
    732,
    (732+63) ,
    625,
    576,
    576,
    625,
    580,   //adjust to correct 576i timing
    580+6,
},
//22
{
    TIMING_ATTRIB_SIZE,
    22,
    720,576,
    5000,
    270000,
    2,
    VerNEGATIVE+HorNEGATIVE,
    864,
    720,
    720,
    864 ,
    732,
    (732+63) ,
    625,
    576,
    576,
    625,
    580,//adjust to correct 576i timing
    580+6,
},
//23
{
    TIMING_ATTRIB_SIZE,
    23,
    720,288,
    5000,
    270000,
    1,
    VerNEGATIVE+HorNEGATIVE,
    864,
    720,
    720,
    864,
    732,
    (732+63) ,
    312,
    288,
    288,
    312,
    290,
    290+3,
},
//24
{
    TIMING_ATTRIB_SIZE,
    24,
    720,288,
    5000,
    270000,
    2,
    VerNEGATIVE+HorNEGATIVE,
    864,
    720,
    720,
    864,
    732,
    (732+63) ,
    312,
    288,
    288,
    312,
    290,
    290+3,
},
//25
{
    TIMING_ATTRIB_SIZE,
    25,
    2880,576,
    5000,
    540000,
    1,
    VerNEGATIVE+HorNEGATIVE,
    3456,
    2880,
    2880,
    3456,
    2880+48,
    (2880+48+252) ,
    625,
    576,
    576,
    625,
    580,
    580+6,
},
//26
{
    TIMING_ATTRIB_SIZE,
    26,
    2880,576,
    5000,
    540000,
    2,
    VerNEGATIVE+HorNEGATIVE,
    3456,
    2880,
    2880,
    3456,
    2880+48,
    (2880+48+252) ,
    625,
    576,
    576,
    625,
    580,
    580+6,
},
//27
{
    TIMING_ATTRIB_SIZE,
    27,
    2880,288,
    5000,
    540000,
    1,
    VerNEGATIVE+HorNEGATIVE,
    3456,
    2880,
    2880,
    3456,
    2880+48,
    (2880+48+252) ,
    312,
    288,
    288,
    312,
    290,
    290+3,
},
//28
{
    TIMING_ATTRIB_SIZE,
    28,
    2880,288,
    5000,
    540000,
    2,
    VerNEGATIVE+HorNEGATIVE,
    3456,
    2880,
    2880,
    3456,
    2880+48,
    (2880+48+252) ,
    313,
    288,
    288,
    313,
    291,
    291+3,
},
//29
{
    TIMING_ATTRIB_SIZE,
    29,
    1440,576,
    5000,
    540000,
    1,
    VerNEGATIVE+HorNEGATIVE,
    1728,
    1440,
    1440,
    1728,
    1440+24,
    (1440+24+128) ,
    625,
    576,
    576,
    625,
    581,
    581+5,
},
//30
{
    TIMING_ATTRIB_SIZE,
    30,
    1440,576,
    5000,
    540000,
    2,
    VerNEGATIVE+HorNEGATIVE,
    1728,
    1440,
    1440,
    1728,
    1440+24,
    (1440+24+128) ,
    625,
    576,
    576,
    625,
    581,
    581+ 5,
},
//31
{
    TIMING_ATTRIB_SIZE,
    31,
    1920,1080,
    5000,
    1485000,
    2,
    VerPOSITIVE+HorPOSITIVE,
    2640,
    1920,
    1920,
    2640,
    1920+528,
    (1920+528+44) ,
    1125,
    1080,
    1080,
    1080+45,
    1084,
    1084+5,
},
//32
{
    TIMING_ATTRIB_SIZE,
    32,
    1920,1080,
    2397,
    742500,
    2,
    VerPOSITIVE+HorPOSITIVE,
    2750,
    1920,
    1920,
    2750,
    1920+638,
    (1920+638+44) ,
    1125,
    1080,
    1080,
    1080+45,
    1084,
    1084+5,
},
//33
{
    TIMING_ATTRIB_SIZE,
    33,
    1920,1080,
    2500,
    742500,
    2,
    VerPOSITIVE+HorPOSITIVE,
    2640,
    1920,
    1920,
    2640,
    1920+528,
    (1920+528+44) ,
    1125,
    1080,
    1080,
    1080+45,
    1084,
    1084+5,
},
//34
{
    TIMING_ATTRIB_SIZE,
    34,
    1920,1080,
    3000,
    742500,
    2,
    VerPOSITIVE+HorPOSITIVE,
    2200,//2640-5*8,
    1920,
    1920,
    2200,
    1920+88,
    (1920+88+44),//(1920+528+44) ,
    1125,
    1080,
    1080,
    1080+45,
    1084,
    1084+5,
},
//35 2880x480p@60Hz 4:3
{
    TIMING_ATTRIB_SIZE,
    35,
    2880, 480,
    6000,
    1081080,
    1,
    VerNEGATIVE+HorNEGATIVE,
    3432,
    2880,
    2880,
    3432,
    2880+64,
    2880+64+248,
    525,
    480,
    480,
    525,
    480+9,
    480+9+6,
},
//36 2880x480p@60Hz 16:9
{
    TIMING_ATTRIB_SIZE,
    36,
    2880, 480,
    6000,
    1081080,
    2,
    VerNEGATIVE+HorNEGATIVE,
    3432,
    2880,
    2880,
    3432,
    2880+64,
    2880+64+248,
    525,
    480,
    480,
    525,
    480+9,
    480+9+6,
},
//37 2880x576p@50Hz 4:3
{
    TIMING_ATTRIB_SIZE,
    37,
    2880, 576,
    5000,
    1080000,
    1,
    VerNEGATIVE+HorNEGATIVE,
    3456,
    2880,
    2880,
    3456,
    2880+48,
    2880+48+256,
    625,
    576,
    576,
    625,
    576+5,
    576+5+5,
},
//38 2880x576p@50Hz 16:9
{
    TIMING_ATTRIB_SIZE,
    38,
    2880, 576,
    5000,
    1080000,
    2,
    VerNEGATIVE+HorNEGATIVE,
    3456,
    2880,
    2880,
    3456,
    2880+48,
    2880+48+256,
    625,
    576,
    576,
    625,
    576+5,
    576+5+5,
},
//39 1920x1080i@50Hz 16:9
{
    TIMING_ATTRIB_SIZE,
    39,
    1920, 1080,
    5000,
    720000,
    2,
    VerPOSITIVE+HorPOSITIVE,
    2304,
    1920,
    1920,
    2304,
    1920+32,
    1920+32+168,
    1250,
    1080,
    1080,
    1250,
    1080+46,
    1080+46+10,
},
//40 1920x1080i@100Hz 16:9
{
    TIMING_ATTRIB_SIZE,
    40,
    1920, 1080,
    10000,
    1485000,
    2,
    VerPOSITIVE+HorPOSITIVE,
    2640,
    1920,
    1920,
    2640,
    1920+528,
    1920+528+44,
    1125,
    1080,
    1080,
    1125,
    1080+4,
    1080+4+10,
},
//41 1280x720p@100Hz 16:9
{
    TIMING_ATTRIB_SIZE,
    41,
    1280, 720,
    10000,
    1485000,
    2,
    VerPOSITIVE+HorPOSITIVE,
    1980,
    1280,
    1280,
    1980,
    1280+440,
    1280+440+40,
    750,
    720,
    720,
    750,
    720+5,
    720+5+5,
},
//42 720*576p@100Hz 4:3
{
    TIMING_ATTRIB_SIZE,
    42,
    720, 576,
    10000,
    540000,
    1,
    VerNEGATIVE+HorNEGATIVE,
    864,
    720,
    720,
    864,
    720+12,
    720+12+64,
    625,
    576,
    576,
    625,
    576+5,
    576+5+5,
},

//43 720*576p@100Hz 16:9
{
    TIMING_ATTRIB_SIZE,
    43,
    720, 576,
    10000,
    540000,
    2,
    VerNEGATIVE+HorNEGATIVE,
    864,
    720,
    720,
    864,
    720+12,
    720+12+64,
    625,
    576,
    576,
    625,
    576+5,
    576+5+5,
},
//44 720(1440)*576i@100Hz 4:3
{
    TIMING_ATTRIB_SIZE,
    44,
    720, 576,
    10000,
    540000,
    1,
    VerNEGATIVE+HorNEGATIVE,
    864,
    720,
    720,
    864,
    720+12,
    720+12+63,
    625,
    576,
    576,
    625,
    576+5-1,
    576+5+5,
},
//45 720(1440)*576i@100Hz 16:9
{
    TIMING_ATTRIB_SIZE,
    45,
    720, 576,
    10000,
    540000,
    2,
    VerNEGATIVE+HorNEGATIVE,
    864,
    720,
    720,
    864,
    720+12,
    720+12+63,
    625,
    576,
    576,
    625,
    576+5-1,
    576+5+5,
},
//46 1920x1080i@120Hz 16:9
{
    TIMING_ATTRIB_SIZE,
    46,
    1920, 1080,
    10000,
    1485000,
    2,
    VerPOSITIVE+HorPOSITIVE,
    2200,
    1920,
    1920,
    2200,
    1920+88,
    1920+88+44,
    1125,
    1080,
    1080,
    1125,
    1080+4,
    1080+4+10,
},
//47 1280x720p@120Hz 16:9
{
    TIMING_ATTRIB_SIZE,
    47,
    1280,720,
    12000,
    1485000,
    2,
    VerPOSITIVE+HorPOSITIVE,
    1650,
    1280,
    1280,
    1650 ,
    1280+110,
    (1280+110+40) ,
    750,
    720,
    720,
    720+30,
    725,
    725+5,
},
//48 720x480p@120Hz 4:3
{
    TIMING_ATTRIB_SIZE,
    48,
    720,480,
    12000,
    540540,
    1,
    VerNEGATIVE+HorNEGATIVE,
    858,
    720,
    720,
    858 ,
    736,
    (736+62) ,
    525,
    480,
    480,
    525,
    489,
    489+6,
},
//49 720x480p@120Hz 16:9
{
    TIMING_ATTRIB_SIZE,
    49,
    720,480,
    12000,
    540540,
    2,
    VerNEGATIVE+HorNEGATIVE,
    858,
    720,
    720,
    858 ,
    736,
    (736+62) ,
    525,
    480,
    480,
    525,
    489,
    489+6,
},
//50 720(1440)x480i@120Hz 4:3
{
    TIMING_ATTRIB_SIZE,
    50,
    720,480,
    12000,
    540540,
    1,
    VerNEGATIVE+HorNEGATIVE,
    858,
    720,
    720,
    858 ,
    720+38/2,
    720+38/2+124/2,
    525,
    480,
    480,
    525,
    489-1,
    489-1+6,
},
//51 720(1440)x480i@120Hz 16:9
{
    TIMING_ATTRIB_SIZE,
    51,
    720,480,
    12000,
    540540,
    2,
    VerNEGATIVE+HorNEGATIVE,
    858,
    720,
    720,
    858 ,
    720+38/2,
    720+38/2+124/2,
    525,
    480,
    480,
    525,
    489-1,
    489-1+6,
},
//52 720x576p@200Hz, 4:3
{
    TIMING_ATTRIB_SIZE,
    52,
    720,576,
    20000,
    1080000,
    1,
    VerNEGATIVE+HorNEGATIVE,
    864,
    720,
    720,
    864,
    732,
    (732+64) ,
    625,
    576,
    576,
    625,
    581,
    581+5,
},
//53 720x576p@200Hz, 16:9
{
    TIMING_ATTRIB_SIZE,
    53,
    720,576,
    20000,
    1080000,
    2,
    VerNEGATIVE+HorNEGATIVE,
    864,
    720,
    720,
    864,
    732,
    (732+64) ,
    625,
    576,
    576,
    625,
    581,
    581+5,
},
//54 720(1440)x576i@200Hz, 4:3
{
    TIMING_ATTRIB_SIZE,
    54,
    720,576,
    20000,
    1080000,
    1,
    VerNEGATIVE+HorNEGATIVE,
    864,
    720,
    720,
    864,
    732,
    (732+63) ,
    625,
    576,
    576,
    625,
    581-1,
    581+5,
},
//55 720(1440)x576i@200Hz, 16:9
{
    TIMING_ATTRIB_SIZE,
    55,
    720,576,
    20000,
    1080000,
    2,
    VerNEGATIVE+HorNEGATIVE,
    864,
    720,
    720,
    864,
    732,
    (732+63) ,
    625,
    576,
    576,
    625,
    581-1,
    581+5,
},
//56 720x480p@240Hz, 4:3
{
    TIMING_ATTRIB_SIZE,
    56,
    720,480,
    24000,
    1081080,
    1,
    VerNEGATIVE+HorNEGATIVE,
    858,
    720,
    720,
    858,
    736,
    (736+62) ,
    525,
    480,
    480,
    525,
    480+9,
    480+9+6,
},
//57 720x480p@240Hz, 16:9
{
    TIMING_ATTRIB_SIZE,
    57,
    720,480,
    24000,
    1081080,
    2,
    VerNEGATIVE+HorNEGATIVE,
    858,
    720,
    720,
    858,
    736,
    (736+62) ,
    525,
    480,
    480,
    525,
    480+9,
    480+9+6,
},
//58 720(1440)x480i@240Hz, 4:3
{
    TIMING_ATTRIB_SIZE,
    58,
    720,480,
    24000,
    1081080,
    1,
    VerNEGATIVE+HorNEGATIVE,
    858,
    720,
    720,
    858,
    720+19,
    (720+19+62) ,
    525,
    480,
    480,
    525,
    480+9-1,
    480+9-1+6,
},
//59 720(1440)x480i@240Hz, 16:9
{
    TIMING_ATTRIB_SIZE,
    59,
    720,480,
    24000,
    1081080,
    2,
    VerNEGATIVE+HorNEGATIVE,
    858,
    720,
    720,
    858,
    720+19,
    (720+19+62) ,
    525,
    480,
    480,
    525,
    480+9-1,
    480+9-1+6,
},
//60 1280x720p@24Hz, 16:9
{
    TIMING_ATTRIB_SIZE,
    60,
    1280,720,
    2400,
    594000,
    2,
    VerPOSITIVE+HorPOSITIVE,
    3300,
    1280,
    1280,
    3300,
    1280+1760,
    1280+1760+40,
    750,
    720,
    720,
    750,
    725,
    725+5,
},
//61 1280x720p@25Hz, 16:9
{
    TIMING_ATTRIB_SIZE,
    61,
    1280,720,
    2500,
    742500,
    2,
    VerPOSITIVE+HorPOSITIVE,
    3960,
    1280,
    1280,
    3960,
    1280+2420,
    1280+2420+40,
    750,
    720,
    720,
    750,
    725,
    725+5,
},
//62 1280x720p@30Hz, 16:9
{
    TIMING_ATTRIB_SIZE,
    62,
    1280,720,
    3000,
    742500,
    2,
    VerPOSITIVE+HorPOSITIVE,
    3300,
    1280,
    1280,
    3300,
    1280+1760,
    1280+1760+40,
    750,
    720,
    720,
    750,
    725,
    725+5,
},
//63 1920x1080p@120Hz, 16:9
{
    TIMING_ATTRIB_SIZE,
    63,
    1920,1080,
    12000,
    2970000,
    2,
    VerPOSITIVE+HorPOSITIVE,
    2200,
    1920,
    1920,
    2200,
    1920+88,
    1920+88+44,
    1125,
    1080,
    1080,
    1125,
    1080+4,
    1084+5,
},
//64 1920x1080p@100Hz, 16:9
{
    TIMING_ATTRIB_SIZE,
    64,
    1920,1080,
    10000,
    2970000,
    2,
    VerPOSITIVE+HorPOSITIVE,
    2640,
    1920,
    1920,
    2640,
    1920+528,
    1920+528+44,
    1125,
    1080,
    1080,
    1125,
    1080+4,
    1084+5,
},

// CEA-861-F
//65 1280x720p@24Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    65,
    1280,720,
    2400,
    594000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    3300,
    1280,
    1280,
    3300,
    1280+1760,
    1280+1760+40,
    750,
    720,
    720,
    750,
    725,
    725+5,
},
//66 1280x720p@25Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    66,
    1280,720,
    2500,
    742500,
    3,
    VerPOSITIVE+HorPOSITIVE,
    3960,
    1280,
    1280,
    3960,
    1280+2420,
    1280+2420+40,
    750,
    720,
    720,
    750,
    725,
    725+5,
},
//67 1280x720p@30Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    67,
    1280,720,
    3000,
    742500,
    3,
    VerPOSITIVE+HorPOSITIVE,
    3300,
    1280,
    1280,
    3300,
    1280+1760,
    1280+1760+40,
    750,
    720,
    720,
    750,
    725,
    725+5,
},
//68 1280x720p@50Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    68,
    1280,720,
    5000,
    742500,
    3,
    VerPOSITIVE+HorPOSITIVE,
    1980,
    1280,
    1280,
    1980,
    1280+440,
    (1280+440+40) ,
    750,
    720,
    720,
    720+30,
    725,
    725+5,
},
//69 1280x720p@60Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    69,
    1280,720,
    6000,
    742500,
    3,
    VerPOSITIVE+HorPOSITIVE,
    1650,
    1280,
    1280,
    1650 ,
    1280+110,
    (1280+110+40) ,
    750,
    720,
    720,
    720+30,
    725,
    725+5,
},
//70 1280x720p@100Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    70,
    1280, 720,
    10000,
    1485000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    1980,
    1280,
    1280,
    1980,
    1280+440,
    1280+440+40,
    750,
    720,
    720,
    750,
    720+5,
    720+5+5,
},
//71 1280x720p@120Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    71,
    1280,720,
    12000,
    1485000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    1650,
    1280,
    1280,
    1650 ,
    1280+110,
    (1280+110+40) ,
    750,
    720,
    720,
    720+30,
    725,
    725+5,
},
//72 1920x1080p@24Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    72,
    1920,1080,
    2397,
    742500,
    3,
    VerPOSITIVE+HorPOSITIVE,
    2750,
    1920,
    1920,
    2750,
    1920+638,
    (1920+638+44) ,
    1125,
    1080,
    1080,
    1080+45,
    1084,
    1084+5,
},
//73 1920x1080p@25Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    73,
    1920,1080,
    2500,
    742500,
    3,
    VerPOSITIVE+HorPOSITIVE,
    2640,
    1920,
    1920,
    2640,
    1920+528,
    (1920+528+44) ,
    1125,
    1080,
    1080,
    1080+45,
    1084,
    1084+5,
},
//74 1920x1080p@30Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    74,
    1920,1080,
    3000,
    741760,
    3,
    VerPOSITIVE+HorPOSITIVE,
    2200,//2640-5*8,
    1920,
    1920,
    2200,
    1920+88,
    (1920+88+44),//(1920+528+44) ,
    1125,
    1080,
    1080,
    1080+45,
    1084,
    1084+5,
},
//75 1920x1080p@50Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    75,
    1920,1080,
    5000,
    1485000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    2640,
    1920,
    1920,
    2640,
    1920+528,
    (1920+528+44) ,
    1125,
    1080,
    1080,
    1080+45,
    1084,
    1084+5,
},
//76 1920x1080p@60Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    76,
    1920,1080,
    6000,
    1485000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    2200,
    1920,
    1920,
    2200 ,
    1920+88,
    (1920+88+44) ,
    1125,
    1080,
    1080,
    1080+45,
    1084,
    1084+5,
},
//77 1920x1080p@100Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    77,
    1920,1080,
    10000,
    2970000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    2640,
    1920,
    1920,
    2640,
    1920+528,
    1920+528+44,
    1125,
    1080,
    1080,
    1125,
    1080+4,
    1084+5,
},
//78 1920x1080p@120Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    78,
    1920,1080,
    12000,
    2970000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    2200,
    1920,
    1920,
    2200,
    1920+88,
    1920+88+44,
    1125,
    1080,
    1080,
    1125,
    1080+4,
    1084+5,
},
//79 1680x720p@24Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    79,
    1680,720,
    2400,
    594000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    3300,
    1680,
    1680,
    3300,
    1680+1360,
    1680+1360+40,
    750,
    720,
    720,
    750,
    720+5,
    720+5+5,
},
//80 1680x720p@25Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    80,
    1680,720,
    2500,
    594000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    3168,
    1680,
    1680,
    3168,
    1680+1228,
    1680+1228+40,
    750,
    720,
    720,
    750,
    720+5,
    720+5+5,
},
//81 1680x720p@30Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    81,
    1680,720,
    3000,
    594000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    2640,
    1680,
    1680,
    2640,
    1680+700,
    1680+700+40,
    750,
    720,
    720,
    750,
    720+5,
    720+5+5,
},
//82 1680x720p@50Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    82,
    1680,720,
    5000,
    825000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    2200,
    1680,
    1680,
    2200,
    1680+260,
    1680+260+40,
    750,
    720,
    720,
    750,
    720+5,
    720+5+5,
},
//83 1680x720p@60Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    83,
    1680,720,
    6000,
    990000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    2200,
    1680,
    1680,
    2200,
    1680+260,
    1680+260+40,
    750,
    720,
    720,
    750,
    720+5,
    720+5+5,
},
//84 1680x720p@100Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    84,
    1680,720,
    10000,
    1650000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    2000,
    1680,
    1680,
    2000,
    1680+60,
    1680+60+40,
    825,
    720,
    720,
    825,
    720+5,
    720+5+5,
},
//85 1680x720p@120Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    85,
    1680,720,
    12000,
    1980000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    2000,
    1680,
    1680,
    2000,
    1680+60,
    1680+60+40,
    825,
    720,
    720,
    825,
    720+5,
    720+5+5,
},
//86 2560x1080p@24Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    86,
    2560,1080,
    2400,
    990000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    3750,
    2560,
    2560,
    3750,
    2560+998,
    2560+998+44,
    1100,
    1080,
    1080,
    1100,
    1080+4,
    1080+4+5,
},
//87 2560x1080p@25Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    87,
    2560,1080,
    2500,
    900000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    3200,
    2560,
    2560,
    3200,
    2560+448,
    2560+448+44,
    1125,
    1080,
    1080,
    1125,
    1080+4,
    1080+4+5,
},
//88 2560x1080p@30Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    88,
    2560,1080,
    3000,
    1188000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    3520,
    2560,
    2560,
    3520,
    2560+768,
    2560+768+44,
    1125,
    1080,
    1080,
    1125,
    1080+4,
    1080+4+5,
},
//89 2560x1080p@50Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    89,
    2560,1080,
    5000,
    1856250,
    3,
    VerPOSITIVE+HorPOSITIVE,
    3300,
    2560,
    2560,
    3300,
    2560+548,
    2560+548+44,
    1125,
    1080,
    1080,
    1125,
    1080+4,
    1080+4+5,
},
//90 2560x1080p@60Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    90,
    2560,1080,
    6000,
    1980000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    3000,
    2560,
    2560,
    3000,
    2560+248,
    2560+248+44,
    1100,
    1080,
    1080,
    1100,
    1080+4,
    1080+4+5,
},
//91 2560x1080p@100Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    91,
    2560,1080,
    10000,
    3712500,
    3,
    VerPOSITIVE+HorPOSITIVE,
    2970,
    2560,
    2560,
    2970,
    2560+218,
    2560+218+44,
    1250,
    1080,
    1080,
    1250,
    1080+4,
    1080+4+5,
},
//92 2560x1080p@120Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    92,
    2560,1080,
    12000,
    4950000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    3300,
    2560,
    2560,
    3300,
    2560+548,
    2560+548+44,
    1250,
    1080,
    1080,
    1250,
    1080+4,
    1080+4+5,
},
//93 3840x2160p@24Hz, 16:9
{
    TIMING_ATTRIB_SIZE,
    93,
    3840, 2160,
    2400,
    2970000,
    2,
    VerPOSITIVE+HorPOSITIVE,
    3840+1660,
    3840,
    3840,
    3840+1660,
    3840+1276,
    3840+1276+88,
    2160+90,
    2160,
    2160,
    2160+90,
    2160+8,
    2160+8+10
},
//94 3840x2160p@25Hz, 16:9
{
    TIMING_ATTRIB_SIZE,
    94,
    3840, 2160,
    2500,
    2970000,
    2,
    VerPOSITIVE+HorPOSITIVE,
    3840+1440,
    3840,
    3840,
    3840+1440,
    3840+1056,
    3840+1056+88,
    2160+90,
    2160,
    2160,
    2160+90,
    2160+8,
    2160+8+10
},
//95 3840x2160p@30Hz, 16:9
{
    TIMING_ATTRIB_SIZE,
    95,
    3840, 2160,
    3000,
    2970000,
    2,
    VerPOSITIVE+HorPOSITIVE,
    3840+560,
    3840,
    3840,
    3840+560,
    3840+176,
    3840+176+88,
    2160+90,
    2160,
    2160,
    2160+90,
    2160+8,
    2160+8+10
},
//96 3840x2160p@50Hz, 16:9
{
    TIMING_ATTRIB_SIZE,
    96,
    3840, 2160,
    5000,
    5940000,
    2,
    VerPOSITIVE+HorPOSITIVE,
    3840+1440,
    3840,
    3840,
    3840+1440,
    3840+1056,
    3840+1056+88,
    2160+90,
    2160,
    2160,
    2160+90,
    2160+8,
    2160+8+10
},
//97 3840x2160p@60Hz, 16:9
{
    TIMING_ATTRIB_SIZE,
    97,
    3840, 2160,
    6000,
    5940000,
    2,
    VerPOSITIVE+HorPOSITIVE,
    3840+560,
    3840,
    3840,
    3840+560,
    3840+176,
    3840+176+88,
    2160+90,
    2160,
    2160,
    2160+90,
    2160+8,
    2160+8+10
},
//98 4096x2160p@24Hz, 256:135
{
    TIMING_ATTRIB_SIZE,
    98,
    4096, 2160,
    2400,
    2970000,
    4,
    VerPOSITIVE+HorPOSITIVE,
    4096+1404,
    4096,
    4096,
    4096+1404,
    4096+1020,
    4096+1020+88,
    2160+90,
    2160,
    2160,
    2160+90,
    2160+8,
    2160+8+10
},
//99 4096x2160p@25Hz, 256:135
{
    TIMING_ATTRIB_SIZE,
    99,
    4096, 2160,
    2500,
    2970000,
    4,
    VerPOSITIVE+HorPOSITIVE,
    5280,
    4096,
    4096,
    5280,
    4096+968,
    4096+968+88,
    2250,
    2160,
    2160,
    2250,
    2160+8,
    2160+8+10
},
//100 4096x2160p@30Hz, 256:135
{
    TIMING_ATTRIB_SIZE,
    100,
    4096, 2160,
    3000,
    2970000,
    4,
    VerPOSITIVE+HorPOSITIVE,
    4400,
    4096,
    4096,
    4400,
    4096+88,
    4096+88+88,
    2250,
    2160,
    2160,
    2250,
    2160+8,
    2160+8+10
},
//101 4096x2160p@50Hz, 256:135
{
    TIMING_ATTRIB_SIZE,
    101,
    4096, 2160,
    5000,
    5940000,
    4,
    VerPOSITIVE+HorPOSITIVE,
    5280,
    4096,
    4096,
    5280,
    4096+968,
    4096+968+88,
    2250,
    2160,
    2160,
    2250,
    2160+8,
    2160+8+10
},
//102 4096x2160p@60Hz, 256:135
{
    TIMING_ATTRIB_SIZE,
    102,
    4096, 2160,
    6000,
    5940000,
    4,
    VerPOSITIVE+HorPOSITIVE,
    4400,
    4096,
    4096,
    4400,
    4096+88,
    4096+88+88,
    2250,
    2160,
    2160,
    2250,
    2160+8,
    2160+8+10
},
//103 3840x2160p@24Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    103,
    3840, 2160,
    2400,
    2970000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    3840+1660,
    3840,
    3840,
    3840+1660,
    3840+1276,
    3840+1276+88,
    2160+90,
    2160,
    2160,
    2160+90,
    2160+8,
    2160+8+10
},
//104 3840x2160p@25Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    104,
    3840, 2160,
    2500,
    2970000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    3840+1440,
    3840,
    3840,
    3840+1440,
    3840+1056,
    3840+1056+88,
    2160+90,
    2160,
    2160,
    2160+90,
    2160+8,
    2160+8+10
},
//105 3840x2160p@30Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    105,
    3840, 2160,
    3000,
    2970000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    3840+560,
    3840,
    3840,
    3840+560,
    3840+176,
    3840+176+88,
    2160+90,
    2160,
    2160,
    2160+90,
    2160+8,
    2160+8+10
},
//106 3840x2160p@50Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    106,
    3840, 2160,
    5000,
    5940000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    3840+1440,
    3840,
    3840,
    3840+1440,
    3840+1056,
    3840+1056+88,
    2160+90,
    2160,
    2160,
    2160+90,
    2160+8,
    2160+8+10
},
//107 3840x2160p@60Hz, 64:27
{
    TIMING_ATTRIB_SIZE,
    107,
    3840, 2160,
    6000,
    5940000,
    3,
    VerPOSITIVE+HorPOSITIVE,
    3840+560,
    3840,
    3840,
    3840+560,
    3840+176,
    3840+176+88,
    2160+90,
    2160,
    2160,
    2160+90,
    2160+8,
    2160+8+10
},
//normal VIC end
//extended resolution begin
//3840x2160p@30Hz, 16:9
{
    TIMING_ATTRIB_SIZE,
    108,
    3840, 2160,
    3000,
    2970000,
    2,
    VerPOSITIVE+HorPOSITIVE,
    3840+560,
    3840,
    3840,
    3840+560,
    3840+176,
    3840+176+88,
    2160+90,
    2160,
    2160,
    2160+90,
    2160+8,
    2160+8+10
},
//3840x2160p@25Hz, 16:9
{
    TIMING_ATTRIB_SIZE,
    109,
    3840, 2160,
    2500,
    2970000,
    2,
    VerPOSITIVE+HorPOSITIVE,
    3840+1440,
    3840,
    3840,
    3840+1440,
    3840+1056,
    3840+1056+88,
    2160+90,
    2160,
    2160,
    2160+90,
    2160+8,
    2160+8+10
},
//3840x2160p@24Hz, 16:9
{
    TIMING_ATTRIB_SIZE,
    110,
    3840, 2160,
    2400,
    2970000,
    2,
    VerPOSITIVE+HorPOSITIVE,
    3840+1660,
    3840,
    3840,
    3840+1660,
    3840+1276,
    3840+1276+88,
    2160+90,
    2160,
    2160,
    2160+90,
    2160+8,
    2160+8+10
},
//4096x2160p@24Hz, 16:9(SMPTE)
{
    TIMING_ATTRIB_SIZE,
    111,
    4096, 2160,
    2400,
    2970000,
    2,
    VerPOSITIVE+HorPOSITIVE,
    4096+1404,
    4096,
    4096,
    4096+1404,
    4096+1020,
    4096+1020+88,
    2160+90,
    2160,
    2160,
    2160+90,
    2160+8,
    2160+8+10
},
};

static CBiosModeInfoExt TV_DefaultModeList[] =
{
{
    MODE_INFO_EXT_SIZE,
    704,
    576,
    5000,
    CBIOS_INTERLACECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_TV,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {0},
},
};

static CBiosModeInfoExt HDTV_DefaultModeList[] =
{
{
    MODE_INFO_EXT_SIZE,
    720,
    576,
    5000,
    CBIOS_PROGRESSIVECAP|CBIOS_INTERLACECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_HDTV,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {1},
},
{
    MODE_INFO_EXT_SIZE,
    1280,
    720,
    5000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_HDTV,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {1},
},
{
    MODE_INFO_EXT_SIZE,
    1920,
    1080,
    5000,
    CBIOS_PROGRESSIVECAP|CBIOS_INTERLACECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_HDTV,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {1},
},
};

static CBiosModeInfoExt DSI_DefaultModeList[] =
{
{
    MODE_INFO_EXT_SIZE,
    640,
    480,
    6000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_DSI,
    MODE_COLOR_DEPTH_CAPS,
    0,
    CBIOS_NATIVEMODE,
    {(CBIOS_DTL_TIMING << 2)},
},
};

static CBiosModeInfoExt DP_DefaultModeList[] =
{
{
    MODE_INFO_EXT_SIZE,
    640,
    480,
    6000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_DP1,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {1},
},
{
    MODE_INFO_EXT_SIZE,
    720,
    480,
    5994,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_DP1,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {1},
},
{
    MODE_INFO_EXT_SIZE,
    720,
    480,
    6000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_DP1,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {1},
},
{
    MODE_INFO_EXT_SIZE,
    720,
    576,
    5000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_DP1,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {1},
},
{
    MODE_INFO_EXT_SIZE,
    1280,
    720,
    5000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_DP1,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {1},
},
{
    MODE_INFO_EXT_SIZE,
    1280,
    720,
    5994,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_DP1,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {1},
},
{
    MODE_INFO_EXT_SIZE,
    1280,
    720,
    6000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_DP1,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {1},
},
{
    MODE_INFO_EXT_SIZE,
    1920,
    1080,
    5000,
    CBIOS_INTERLACECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_DP1,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {1},
},
{
    MODE_INFO_EXT_SIZE,
    1920,
    1080,
    5994,
    CBIOS_INTERLACECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_DP1,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {1},
},
{
    MODE_INFO_EXT_SIZE,
    1920,
    1080,
    6000,
    CBIOS_INTERLACECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_DP1,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {1},
},
};

static CBiosModeInfoExt MHL_DefaultModeList[] =
{
{
    MODE_INFO_EXT_SIZE,
    640,
    480,
    5994,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_MHL,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {0},
},
{
    MODE_INFO_EXT_SIZE,
    640,
    480,
    6000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_MHL,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {0},
},
{
    MODE_INFO_EXT_SIZE,
    720,
    480,
    5994,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_MHL,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {0},
},
{
    MODE_INFO_EXT_SIZE,
    720,
    480,
    6000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_MHL,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {0},
},
{
    MODE_INFO_EXT_SIZE,
    720,
    576,
    5000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_MHL,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {0},
},
{
    MODE_INFO_EXT_SIZE,
    1280,
    720,
    5994,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_MHL,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {0},
},
{
    MODE_INFO_EXT_SIZE,
    1280,
    720,
    6000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_MHL,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {0},
},
{
    MODE_INFO_EXT_SIZE,
    1280,
    720,
    5000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_MHL,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {0},
},
};

static CBiosModeInfoExt Device_DefaultModeList[] =
{
{
    MODE_INFO_EXT_SIZE,
    640,
    480,
    6000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_NONE,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {(CBIOS_MODE_STD_TIMING << 2)},
},
{
    MODE_INFO_EXT_SIZE,
    800,
    600,
    6000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_NONE,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {(CBIOS_MODE_STD_TIMING << 2)},
},
{
    MODE_INFO_EXT_SIZE,
    1024,
    768,
    6000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_NONE,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {(CBIOS_MODE_STD_TIMING << 2)},
},
{
    MODE_INFO_EXT_SIZE,
    1280,
    1024,
    6000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_NONE,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {(CBIOS_MODE_STD_TIMING << 2)},
},
{
    MODE_INFO_EXT_SIZE,
    1920,
    1080,
    6000,
    CBIOS_PROGRESSIVECAP,
    CBIOS_DEVICEMODE,
    CBIOS_TYPE_NONE,
    MODE_COLOR_DEPTH_CAPS,
    0,
    0,
    {((CBIOS_MODE_DTL_TIMING << 2) | (1 << 18))},
}
};


static CBiosEquivalentDeviceMode Equivalent_DeviceMode_Table[] =
{
    {720, 480, {5900, 6000-1}, 6000},
    {720, 480, {6000, 6000}, 5994},
    {1280, 720, {5900, 6000-1}, 6000},
    {1280, 720, {6000, 6000}, 5994},
    {1920, 1080, {2900, 3000-1}, 3000},
    {1920, 1080, {3000, 3000}, 2997},
    {1920, 1080, {5900, 6000-1}, 6000},
    {1920, 1080, {6000, 6000}, 5994},
};

/***************************************************************
Function:    cbMode_CompareMode

Description: Compare two mode XRes, YRes, Refresh Rate and Interlace/Progress

Input:       pMode1, pMode2: two modes need to compare

Output:

Return:      Positive: if Mode1 >  Mode2
             Zero    : if Mode1 == Mode2
             Negative: if Mode1 <  Mode2
***************************************************************/
static CBIOS_S32 cbMode_CompareMode(PCBIOS_MODE_INFO_EXT pMode1, PCBIOS_MODE_INFO_EXT pMode2)
{
    CBIOS_S32 retValue;

    if ((pMode1->XResolution > pMode2->XResolution)||
       ((pMode1->XResolution == pMode2->XResolution)&&
        (pMode1->YResolution > pMode2->YResolution))||
       ((pMode1->XResolution == pMode2->XResolution)&&
        (pMode1->YResolution == pMode2->YResolution)&&
        (pMode1->Refreshrate > pMode2->Refreshrate)))
    {
        retValue = 1;
    }
    else if ((pMode1->XResolution == pMode2->XResolution)&&
             (pMode1->YResolution == pMode2->YResolution)&&
             (pMode1->Refreshrate == pMode2->Refreshrate))
    {
        if (pMode1->InterLaced == pMode2->InterLaced)
        {
            retValue = 0;
        }
        else if ((pMode1->InterLaced == CBIOS_FALSE)&&
                 (pMode2->InterLaced == CBIOS_TRUE))
        {
            retValue = 1;
        }
        else
        {
            retValue = -1;
        }
    }
    else
    {
        retValue = -1;
    }

    return retValue;
}

static CBIOS_BOOL cbMode_AddMode(PCBiosModeInfoExt pModeList, PCBiosModeInfoExt pMode, CBIOS_U32 CurrentModeNum, CBIOS_ACTIVE_TYPE Device)
{
    CBIOS_MODE_INFO_EXT ModeList, ModeInsert;
    CBIOS_BOOL          bRet = CBIOS_FALSE;
    CBIOS_U32           index = 0xFFFFFFFF, i = 0;
    CBIOS_S32           sCmp;

    cb_memset(&ModeList, 0, sizeof(CBIOS_MODE_INFO_EXT));
    cb_memset(&ModeInsert, 0, sizeof(CBIOS_MODE_INFO_EXT));

    pMode->DeviceFlags = Device;

    ModeInsert.XResolution = (CBIOS_U16)pMode->XRes;
    ModeInsert.YResolution = (CBIOS_U16)pMode->YRes;
    ModeInsert.Refreshrate = (CBIOS_U16)pMode->RefreshRate;
    ModeInsert.InterLaced = (pMode->InterlaceProgressiveCaps & 0x2) ? CBIOS_TRUE : CBIOS_FALSE;


    for(i = 0; i< CurrentModeNum; i++)
    {
        ModeList.XResolution = (CBIOS_U16)pModeList[i].XRes;
        ModeList.YResolution = (CBIOS_U16)pModeList[i].YRes;
        ModeList.Refreshrate = (CBIOS_U16)pModeList[i].RefreshRate;
        ModeList.InterLaced = (pModeList[i].InterlaceProgressiveCaps & 0x2) ? CBIOS_TRUE : CBIOS_FALSE;
        sCmp = cbMode_CompareMode(&ModeList, &ModeInsert);
        if (sCmp <= 0)
        {
            break;
        }
    }
    if(i == CurrentModeNum)
    {
        cb_memcpy(&pModeList[i], pMode, sizeof(CBiosModeInfoExt));
        bRet = CBIOS_TRUE;
    }
    else
    {
        if (sCmp < 0)
        {
            // insert mode to modelist
            for(index = CurrentModeNum; index > i; index--)
            {
                cb_memcpy(&pModeList[index], &pModeList[index - 1], sizeof(CBiosModeInfoExt));
            }
            cb_memcpy(&pModeList[i], pMode, sizeof(CBiosModeInfoExt));
            bRet = CBIOS_TRUE;
        }
        else if (sCmp == 0)
        {
            // added mode already exists in modelist, only merge mode flags
            pModeList[i].InterlaceProgressiveCaps |= pMode->InterlaceProgressiveCaps;
            pModeList[i].AdapterDeviceFlags |= pMode->AdapterDeviceFlags;
            pModeList[i].DeviceFlags |= pMode->DeviceFlags;
            pModeList[i].ColorDepthCaps |= pMode->ColorDepthCaps;
            pModeList[i].AspectRatioCaps |= pMode->AspectRatioCaps;
            pModeList[i].NativeModeFlags |= pMode->NativeModeFlags;
            pModeList[i].ModeFlags |= pMode->ModeFlags;
            bRet= CBIOS_FALSE;
        }
    }

    return bRet;
}

#if 0
static CBIOS_BOOL cbMode_DeleteMode(PCBiosModeInfoExt pModeList, CBIOS_U16 XRes, CBIOS_U16 YRes, CBIOS_U16 RefRate, CBIOS_BOOL isInterlace)
{
    CBIOS_BOOL          bFound = CBIOS_FALSE;
    CBIOS_U32           index = 0, i = 0;

    while(pModeList[i].XRes)
    {
        if ((pModeList[i].XRes == XRes) &&
            (pModeList[i].YRes == YRes) &&
            (pModeList[i].RefreshRate == RefRate))
        {
            if (((pModeList[i].InterlaceProgressiveCaps == CBIOS_INTERLACECAP) && isInterlace) ||
                ((pModeList[i].InterlaceProgressiveCaps == CBIOS_PROGRESSIVECAP) && (!isInterlace)))
            {
                index = i;
                bFound = CBIOS_TRUE;
                break;
            }
            else
            {
                i++;
                continue;
            }
        }
        else
        {
            i++;
            continue;
        }
    }

    if (bFound)
    {
        if (pModeList[index+1].XRes)
        {
            do
            {
                cb_memcpy(&pModeList[i], &pModeList[i+1], sizeof(CBiosModeInfoExt));
                i++;
            }while(pModeList[i+1].XRes);
            cb_memset(&pModeList[i], 0, sizeof(CBiosModeInfoExt));
        }
        else
        {
            cb_memset(&pModeList[index], 0, sizeof(CBiosModeInfoExt));
        }

    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "cbMode_DeleteMode: Delete mode not found in modelist!\n"));
    }

    return bFound;
}
#endif

static CBIOS_BOOL cbMode_FindModeInModeList(PCBiosModeInfoExt pModeList, CBIOS_U32 ModeNum, CBIOS_U32 XRes, CBIOS_U32 YRes, CBIOS_U32 RefRate, CBIOS_U32 InterlaceProgressiveCaps, PCBIOS_U32 pIndex)
{
    CBIOS_BOOL  bRet = CBIOS_FALSE;
    CBIOS_U32   index = 0;

    for(index = 0; index < ModeNum; index++)
    {
        if((pModeList[index].XRes == XRes) && (pModeList[index].YRes == YRes) && (pModeList[index].RefreshRate == RefRate) && (pModeList[index].InterlaceProgressiveCaps == InterlaceProgressiveCaps))
        {
            if(pIndex)
            {
                *pIndex = index;
            }
            bRet = CBIOS_TRUE;
        }
    }

    return bRet;
}


/*********

per CEA861-F spec
if A Video Timing with a vertical frequency that is an integer multiple of 6.00 Hz
(i.e., 24.00, 30.00, 60.00, 120.00 or 240.00 Hz) is considered to be the same as a
Video Timing with the equivalent detailed timing information but where the vertical
frequency is adjusted by a factor of 1000/1001

here Add 5994(6000) for 720X480,  add 5994(6000) for 1280X720, add 5994(6000)  and 2997(3000) for 1920X1080 mode

*********/


CBIOS_VOID cbMode_AddEquivalentDeviceMode(PCBiosModeInfoExt pModeList, PCBIOS_U32 pModeNum, PCBiosModeInfoExt pDeviceMode, CBIOS_ACTIVE_TYPE  Device, CBIOS_U32 MaxMode)
{
    CBiosModeInfoExt ModeToInsert = {0};
    CBIOS_U32        XRes, YRes, MinRefreshRate, MaxRefreshRate;
    CBIOS_U32        OriginModeNum = *pModeNum;
    CBIOS_U32        ModeNum = OriginModeNum;
    CBIOS_BOOL       found = CBIOS_FALSE;
    CBIOS_U32        index = 0;


    for(index = 0; index < (sizeof(Equivalent_DeviceMode_Table)/sizeof(Equivalent_DeviceMode_Table[0])); index++)
    {
        XRes = Equivalent_DeviceMode_Table[index].XRes;
        YRes = Equivalent_DeviceMode_Table[index].YRes;
        MinRefreshRate = Equivalent_DeviceMode_Table[index].RefRateRange[0];
        MaxRefreshRate = Equivalent_DeviceMode_Table[index].RefRateRange[1];
        if((pDeviceMode->XRes == XRes) && (pDeviceMode->YRes == YRes) && (pDeviceMode->RefreshRate >= MinRefreshRate) && (pDeviceMode->RefreshRate <= MaxRefreshRate))
        {
            found = CBIOS_TRUE;
            ModeNum++;
            break;
        }

    }

    if(found && pModeList && (MaxMode >= ModeNum) && (index < sizeof(Equivalent_DeviceMode_Table)/sizeof(Equivalent_DeviceMode_Table[0])))
    {
        cb_memcpy(&ModeToInsert, pDeviceMode, sizeof(CBiosModeInfoExt));
        ModeToInsert.RefreshRate = Equivalent_DeviceMode_Table[index].RefRateToAdd;
        ModeToInsert.ModeFlags |= 0x2;
        if(cbMode_FindModeInModeList(pModeList, OriginModeNum, ModeToInsert.XRes, ModeToInsert.YRes, ModeToInsert.RefreshRate, ModeToInsert.InterlaceProgressiveCaps, CBIOS_NULL))
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "cbMode_AddEquivalentDeviceMode: add mode failed! this mode already exists!\n"));
            ModeNum--;
        }
        else if(!cbMode_AddMode(pModeList, &ModeToInsert, OriginModeNum, Device))
        {
            ModeNum--;
        }
    }

    *pModeNum = ModeNum;

    return;
}


static CBIOS_VOID cbMode_FillAdapterModeBuffer(PCBIOS_EXTENSION_COMMON pcbe,
                                               PCBiosModeInfoExt pModeList,
                                               CBIOS_U32 XRes,
                                               CBIOS_U32 YRes,
                                               CBIOS_U32 RefRate)
{
    if(pModeList != CBIOS_NULL)
    {
        pModeList->XRes = XRes;
        pModeList->YRes = YRes;
        pModeList->RefreshRate = RefRate;
        pModeList->InterlaceProgressiveCaps = CBIOS_PROGRESSIVECAP;

        pModeList->AdapterDeviceFlags = CBIOS_ADAPTERMODE;
        //pModeList->DeviceFlags =
        pModeList->ColorDepthCaps =  CBIOS_COLORDEPTH16 |
                                     CBIOS_COLORDEPTH32XRGB |
                                     CBIOS_COLORDEPTH32ARGB |
                                     CBIOS_COLORDEPTH32ABGR |
                                     CBIOS_COLORDEPTH2101010ARGB |
                                     CBIOS_COLORDEPTH2101010ABGR |
                                     CBIOS_COLORDEPTH16161616ABGRF;
        //pModeList->AspectRatioCaps =
        //pModeList->NativeModeFlags
        //pModeList->ModeFlags
    }
}



// All timing value is progressive, even though request is interlace mode.
CBIOS_BOOL cbMode_GetHVTiming(PCBIOS_VOID pvcbe,
                              CBIOS_U32 XRes,
                              CBIOS_U32 YRes,
                              CBIOS_U32 Refresh,
                              CBIOS_U32 isInterlace,
                              CBIOS_ACTIVE_TYPE Device,
                              PCBIOS_TIMING_ATTRIB pTiming)

{
    PCBIOS_EXTENSION_COMMON    pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DEVICE_COMMON       pDevCommon  = cbGetDeviceCommon(&pcbe->DeviceMgr, Device);
    PCBIOS_EDID_STRUCTURE_DATA pEdidStruct = CBIOS_NULL;
    PCBIOS_MONITOR_MISC_ATTRIB pMonitorAttr = CBIOS_NULL;
    PCBIOS_MODE_INFO_EXT       pModeInfo = CBIOS_NULL;
    PCBIOS_TIMING_ATTRIB       pTimingTbl = CBIOS_NULL;
    CBIOS_U32 ulTimingBlock = 0, ulTmIdx = 0;
    CBIOS_U32 NumOfTimings = 0, i;
    CBIOS_BOOL  bFound = CBIOS_FALSE;

    cbTraceEnter(GENERIC);

    pEdidStruct = &(pDevCommon->EdidStruct);
    pMonitorAttr = &(pEdidStruct->Attribute);

    // Set the default refresh rate to 60Hz.
    if((Refresh == 0) || (Refresh == 1))
    {
        Refresh = 6000;
    }

    // first find timing in EDID
    if((cbEDIDModule_IsEDIDValid(pDevCommon->EdidData))&&
       (cbEDIDModule_SearchTmInEdidStruct(XRes, YRes, Refresh, isInterlace,
                                          pEdidStruct, &ulTimingBlock, &ulTmIdx)))
    {
        if(ulTimingBlock == 3) // Means CEA861B format block
        {
            CBIOS_BOOL  byRefIndex = 0;
            byRefIndex = pEdidStruct->HDMIFormat[ulTmIdx].RefreshIndex;

            cb_memcpy(pTiming, &HDMIFormatTimingTbl[ulTmIdx], sizeof(CBIOS_TIMING_ATTRIB));

            if (byRefIndex == 1)
            {
                pTiming->PixelClock = pTiming->PixelClock / 1001UL * 1000UL;
                pTiming->RefreshRate = (CBIOS_U16)((CBIOS_U32)pTiming->RefreshRate * 1000 / 1001);
            }
            bFound = CBIOS_TRUE;
        }
        else if (ulTimingBlock == 4)//DTD
        {
            pModeInfo = &(pDevCommon->EdidStruct.DTDTimings[ulTmIdx]);
            cbConvertEdidTimingToTableTiming(pcbe, pModeInfo,  pTiming);
            bFound = CBIOS_TRUE;
        }
        else if(ulTimingBlock == 2) // Means Detailed timing block.
        {
            //Patch for DFI: Kortek monitor, for 1280x1024 mode, use VESA timing instead of detailed timing
            if (cbIsSameMonitor(pDevCommon->EdidData, KortekMonitorID)
                && (XRes == 1280) && (YRes == 1024))
            {
                bFound = CBIOS_FALSE;
            }
            else
            {
                pModeInfo = &(pDevCommon->EdidStruct.DtlTimings[ulTmIdx]);
                cbConvertEdidTimingToTableTiming(pcbe, pModeInfo, pTiming);
                bFound = CBIOS_TRUE;
            }
        }
        else if(ulTimingBlock == 5)
        {
            pModeInfo = &(pDevCommon->EdidStruct.DisplayID_TYPE1_Timings[ulTmIdx]);
            cbConvertEdidTimingToTableTiming(pcbe, pModeInfo,  pTiming);
            bFound = CBIOS_TRUE;
        }
    }

    if(!bFound)
    {
        pTimingTbl = AdapterTimingTbl;
        NumOfTimings = sizeofarray(AdapterTimingTbl);

        //patch for HDMI 640*480@60HZ timing.When set 640*480 of DP and HDMI device,use HDMI timing table
        //since AdapterTiming also have this mode
        if((XRes== 640) && (YRes == 480) && (Refresh == 6000)
            && (pMonitorAttr->IsCEA861HDMI))
        {
            pTimingTbl = HDMIFormatTimingTbl;
            NumOfTimings = sizeofarray(HDMIFormatTimingTbl);
        }
    }


    if((!bFound)&&
       (pTimingTbl != CBIOS_NULL))
    {

        for(i=0; i<NumOfTimings; i++)
        {
            if((pTimingTbl[i].XRes == XRes) && (pTimingTbl[i].YRes == YRes))
            {
                if ((Refresh == 0) || ((pTimingTbl[i].RefreshRate/100) == (Refresh/100)))
                {
                    bFound = CBIOS_TRUE;
                    break;
                }

            }
        }
        if(bFound)
        {
            cb_memcpy(pTiming, &pTimingTbl[i],sizeof(CBIOS_TIMING_ATTRIB));
        }
        else if(!(Device & CBIOS_TYPE_CRT))
        {

            pTimingTbl = HDMIFormatTimingTbl;
            NumOfTimings = sizeofarray(HDMIFormatTimingTbl);

            if (pTimingTbl != CBIOS_NULL)
            {
                CBIOS_U32 RefRateIndex = 0;
                CBIOS_U32 RefRate = 0;

                for (i = 0; i < NumOfTimings; i++)
                {
                    if((pTimingTbl[i].XRes == XRes) &&
                        (pTimingTbl[i].YRes == YRes) &&
                        (pcbe->pHDMIFormatTable[i].Interlace == isInterlace))
                    {
                        if (Refresh == 0) //default refrate
                        {
                            RefRateIndex = pcbe->pHDMIFormatTable[i].DefaultRefRateIndex;
                            bFound = CBIOS_TRUE;
                        }
                        else
                        {
                            for (RefRateIndex = 0; RefRateIndex < 2; RefRateIndex++)
                            {
                                RefRate = pcbe->pHDMIFormatTable[i].RefRate[RefRateIndex];
                                if ((RefRate / 100) == (Refresh / 100))
                                {
                                    bFound = CBIOS_TRUE;
                                    break;
                                }
                            }
                        }

                        if (bFound)
                        {
                            break; //for (i = 0; i < NumOfTimings; i++)
                        }
                    }//if((pTiming[i].XResolution == pTmParam->XRes)...
                }//for (i = 0; i < NumOfTimings; i++)

                if (bFound)
                {
                    cb_memcpy(pTiming, &pTimingTbl[i], sizeof(CBIOS_TIMING_ATTRIB));

                    if ((RefRateIndex % 2) == 1)//fractional refreshrate
                    {
                        pTiming->PixelClock = pTiming->PixelClock / 1001UL * 1000UL;
                        pTiming->RefreshRate = (CBIOS_U16)((CBIOS_U32)pTiming->RefreshRate * 1000 / 1001);
                    }
                }
            }

        }
    }

    if((!bFound) && (XRes != 0) && (YRes != 0) && (Refresh != 0))
    {
        //Customize timing not support interlace mode.
        //Timing value has no difference between interlace mode and non-interlace mode.
        cbCalcCustomizedTiming(pcbe, XRes, YRes, Refresh, pTiming);
        bFound = CBIOS_TRUE;
    }

    if(!bFound)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbMode_GetHVTiming:Funtion failure!\n"));
    }

    cbTraceExit(GENERIC);

    return bFound;
}


CBIOS_U32 cbMode_GetDefaultModeList(PCBIOS_VOID pvcbe, PCBiosModeInfoExt pModeList, CBIOS_ACTIVE_TYPE  Device)
{
    CBIOS_U32          ulModeNum = 0;
    CBIOS_U32          i = 0;

    switch(Device)
    {
    case CBIOS_TYPE_TV:
        ulModeNum = sizeofarray(TV_DefaultModeList);
        if (pModeList != CBIOS_NULL)
        {
            for (i = 0; i < ulModeNum; i++)
            {
                cbMode_AddMode(pModeList, &(TV_DefaultModeList[i]), i, Device);
            }
        }
        break;

    case CBIOS_TYPE_HDTV:
        ulModeNum = sizeofarray(HDTV_DefaultModeList);
        if (pModeList != CBIOS_NULL)
        {
            for (i = 0; i < ulModeNum; i++)
            {
                cbMode_AddMode(pModeList, &(HDTV_DefaultModeList[i]), i, Device);
            }
        }
        break;

    case CBIOS_TYPE_DSI:
        ulModeNum = sizeofarray(DSI_DefaultModeList);
        if (pModeList != CBIOS_NULL)
        {
            for (i = 0; i < ulModeNum; i++)
            {
                cbMode_AddMode(pModeList, &(DSI_DefaultModeList[i]), i, Device);
            }
        }
        break;

    case CBIOS_TYPE_DP1:
    case CBIOS_TYPE_DP2:
    case CBIOS_TYPE_DP3:
    case CBIOS_TYPE_DP4:
        ulModeNum = sizeofarray(DP_DefaultModeList);
        if (pModeList != CBIOS_NULL)
        {
            for (i = 0; i < ulModeNum; i++)
            {
                cbMode_AddMode(pModeList, &(DP_DefaultModeList[i]), i, Device);
            }
        }
        break;

    case CBIOS_TYPE_MHL:
        ulModeNum = sizeofarray(MHL_DefaultModeList);
        if (pModeList != CBIOS_NULL)
        {
            for (i = 0; i < ulModeNum; i++)
            {
                cbMode_AddMode(pModeList, &(MHL_DefaultModeList[i]), i, Device);
            }
        }
        break;

    case CBIOS_TYPE_DVO:
    case CBIOS_TYPE_CRT:
    default:
        ulModeNum = sizeofarray(Device_DefaultModeList);
        if (pModeList != CBIOS_NULL)
        {
            for (i = 0; i < ulModeNum; i++)
            {
                cbMode_AddMode(pModeList, &(Device_DefaultModeList[i]), i, Device);
            }
        }
        break;
    }

    cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "cbMode_GetDefaultModeList: Total mode num is %d!\n", ulModeNum));
    return ulModeNum;
}


// Get the mode counts from new CBIOS adapter mode timing table.
// This mode list does not include the interlace mode.
CBIOS_VOID cbMode_GetAdapterModeNum(PCBIOS_VOID pvcbe, CBIOS_U32* AdapterModeNum)
{
    CBIOS_U32 ulModeCount = sizeofarray(AdapterModeList);

    *AdapterModeNum = ulModeCount;

    return;
}

CBIOS_STATUS cbMode_FillAdapterModeList(PCBIOS_VOID pvcbe, PCBiosModeInfoExt pModeList, CBIOS_U32 *pBufferSize)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32 ulAdapterModeCount = sizeofarray(AdapterModeList);
    PCBiosModeInfoExt pAdapterModeList = AdapterModeList;

    CBIOS_U32 ulModeCount = 0, i = 0;
    CBIOS_U32 XRes = 0, YRes = 0, RefRate = 0;
    CBIOS_U32 ulTemp;

    if (pModeList == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbMode_FillAdapterModeList: pModeList is null.\n"));
        return CBIOS_ER_NULLPOINTER;
    }

    cb_memset(pModeList, 0, *pBufferSize);

    for (i = 0; i < ulAdapterModeCount; i++)
    {
        XRes = pAdapterModeList[i].XRes;
        YRes = pAdapterModeList[i].YRes;
        RefRate = pAdapterModeList[i].RefreshRate;

        // add registry to control some 120Hz modes
        //such as 1024*768, 1280*1024, 1600*1200, 1920*1080
        if(pAdapterModeList[i].RefreshRate == 12000)
        {
            if (NO_ERROR == cb_GetRegistryParameters(pcbe->pAdapterContext, KEYNAME_DW_HARDCODED_SPECIALMODE, CBIOS_FALSE, &ulTemp))
            {
                if(!ulTemp)
                {
                    continue;
                }
            }
            else
            {
                continue;
            }
        }

        cbMode_FillAdapterModeBuffer(pcbe, &pModeList[ulModeCount], XRes, YRes, RefRate);
        ulModeCount++;
    }

    *pBufferSize = ulModeCount*sizeof(CBiosModeInfoExt);
    return CBIOS_OK;
}


CBIOS_VOID cbMode_GetFilterPara(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device, PCBIOS_MODE_FILTER_PARA pFilter)
{
    PCBIOS_EXTENSION_COMMON    pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_MONITOR_TYPE         MonitorType = CBIOS_MONITOR_TYPE_NONE;
    PCBIOS_DEVICE_COMMON       pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, Device);
    PCBIOS_VOID                pDPMonitorContext = CBIOS_NULL;


    MonitorType = pDevCommon->CurrentMonitorType;

    switch(MonitorType)
    {
    case CBIOS_MONITOR_TYPE_DVI:
        pFilter->MaxDclk = 1650000;
        break;
    case CBIOS_MONITOR_TYPE_HDMI:
        pFilter->MaxDclk = pcbe->ChipLimits.ulMaxHDMIClock;
        break;
    case CBIOS_MONITOR_TYPE_MHL:
        pFilter->MaxDclk = pcbe->ChipLimits.ulMaxMHLClock;
        break;
    case CBIOS_MONITOR_TYPE_DP:
        pDPMonitorContext = cbGetDPMonitorContext(pcbe, pDevCommon);
        pFilter->MaxDclk = cbDPMonitor_GetMaxSupportedDclk(pcbe, pDPMonitorContext);
        break;
    case CBIOS_MONITOR_TYPE_CRT:
        pFilter->MaxDclk = 4000000;
        break;
    default:
        pFilter->MaxDclk = 3300000;
        break;
    }

    if((pcbe->ChipID == CHIPID_ARISE2030) || (pcbe->ChipID == CHIPID_ARISE2020))
    {
        if((Device == CBIOS_TYPE_DP2) && (MonitorType == CBIOS_MONITOR_TYPE_HDMI))
        {
            pFilter->MaxDclk = 3400000;
        }
    }

    if(pFilter->MaxDclk > pcbe->ChipLimits.ulMaxIGAClock)
    {
        pFilter->MaxDclk = pcbe->ChipLimits.ulMaxIGAClock;
    }

    pFilter->bFilterInterlace = CBIOS_TRUE;
}


CBIOS_U32 cbMode_GetDeviceModeList(PCBIOS_VOID pvcbe,
                                   CBIOS_ACTIVE_TYPE  Device,
                                   PCBiosModeInfoExt pModeList,
                                   CBIOS_U32 *pBufferSize,
                                   PCBIOS_MODE_FILTER_PARA pFilter)
{
    PCBIOS_EXTENSION_COMMON    pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DEVICE_COMMON       pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, Device);
    PCBIOS_EDID_STRUCTURE_DATA pEdidStruct = &(pDevCommon->EdidStruct);

    CBIOS_U32  i = 0;
    CBIOS_U32  EstIndex = 0, StdIndex = 0, DtlIndex = 0, CEAIndex = 0, DTDIndex = 0, DIDT1Index = 0;
    CBIOS_U32  ulModeNum = 0, MaxModeNum = 0, TotalModeCount = 0;
    CBIOS_U8   byRefRateIndex = 0;

    CBIOS_U32  AspectRatioCaps = 0;
    CBIOS_U32  IntProgCaps = 0;
    CBIOS_U32  NativeModeFlags = 0;
    CBIOS_U32  IsCEAMode = 0;
    CBIOS_U32  PreferredModeFlags = 0;

    CBIOS_BOOL *pbModeUsed;
    CBIOS_BOOL *pbEstModeUsed;
    CBIOS_BOOL *pbStdModeUsed;
    CBIOS_BOOL *pbDtlModeUsed;
    CBIOS_BOOL *pbHDMIFormatUsed;
    CBIOS_BOOL *pbDTDModeUsed;
    CBIOS_BOOL *pbDisplayIDT1ModeUsed;
    CBIOS_BOOL bFilterThisMode = CBIOS_FALSE;

    CBIOS_MODE_INFO_EXT MaxMode, tempBuffer,SupportedMaxMode = {0};
    CBiosModeInfoExt    ModeToInsert = {0};
    CBIOS_TIMING_ATTRIB Timing = {0};
    CBIOS_MODE_TIMING_TYPE TimingType = CBIOS_MODE_NONEDID_TIMING, tempType, curType;

    if ((pModeList != CBIOS_NULL) && (pBufferSize != CBIOS_NULL))
    {
        // calculate the mode num of buffer can contain
        MaxModeNum = (*pBufferSize) / sizeof(CBiosModeInfoExt);
        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG),"cbMode_GetDeviceModeList: BufferSize = %d. MaxModeNum = %d\n", *pBufferSize, MaxModeNum));
    }

    TotalModeCount = CBIOS_ESTABLISHMODECOUNT+CBIOS_STDMODECOUNT+CBIOS_DTLMODECOUNT+CBIOS_HDMIFORMATCOUNTS+CBIOS_DTDTIMINGCOUNTS+CBIOS_DISPLAYID_TYPE1_MODECOUNT;
    pbModeUsed = cb_AllocateNonpagedPool(TotalModeCount*sizeof(CBIOS_BOOL));
    if(pbModeUsed == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"pbModeUsed allocate error.\n"));
        return ulModeNum;
    }

    for(i = 0; i < TotalModeCount; i++)
    {
        *(pbModeUsed+i) = CBIOS_FALSE;
    }
    pbEstModeUsed = pbModeUsed;
    pbStdModeUsed = pbEstModeUsed + CBIOS_ESTABLISHMODECOUNT;
    pbDtlModeUsed = pbStdModeUsed + CBIOS_STDMODECOUNT;
    pbHDMIFormatUsed = pbDtlModeUsed + CBIOS_DTLMODECOUNT;
    pbDTDModeUsed = pbHDMIFormatUsed + CBIOS_HDMIFORMATCOUNTS;
    pbDisplayIDT1ModeUsed= pbDTDModeUsed + CBIOS_DTDTIMINGCOUNTS;

    SupportedMaxMode.XResolution = pDevCommon->MaxResConfig.MaxXRes;
    SupportedMaxMode.YResolution = pDevCommon->MaxResConfig.MaxYRes;
    SupportedMaxMode.Refreshrate = pDevCommon->MaxResConfig.MaxRefresh;

    ulModeNum = 0;

    while(CBIOS_TRUE)
    {
        EstIndex = 0xFF;
        StdIndex = 0xFF;
        DtlIndex = 0xFF;
        CEAIndex = 0xFF;
        DTDIndex = 0xFF;
        DIDT1Index = 0xFF;
        NativeModeFlags = CBIOS_NONNATIVEMODE;
        PreferredModeFlags = CBIOS_NONPERFERREDMODE;
        AspectRatioCaps = CBIOS_DEFAULTRATIO;
        TimingType = CBIOS_MODE_NONEDID_TIMING;
        IsCEAMode = 0;
        bFilterThisMode = CBIOS_FALSE;
        cb_memset(&MaxMode, 0, sizeof(CBIOS_MODE_INFO_EXT));
        cb_memset(&tempBuffer, 0, sizeof(CBIOS_MODE_INFO_EXT));

        // Get the maximum mode sorted by X, Y, Refrsh Rate.
        for(i=0; i<CBIOS_ESTABLISHMODECOUNT; i++)
        {
            if((pEdidStruct->EstTimings[i].Valid) && (EstModeSupportTbl[i]) && (!(*(pbEstModeUsed+i))))
            {
                tempBuffer.XResolution = pEdidStruct->EstTimings[i].XResolution;
                tempBuffer.YResolution = pEdidStruct->EstTimings[i].YResolution;
                tempBuffer.Refreshrate = pEdidStruct->EstTimings[i].Refreshrate;
                tempBuffer.InterLaced = CBIOS_FALSE;

                if ((cbMode_CompareMode(&tempBuffer, &MaxMode)) > 0)
                {
                    MaxMode.XResolution = tempBuffer.XResolution;
                    MaxMode.YResolution = tempBuffer.YResolution;
                    MaxMode.Refreshrate = tempBuffer.Refreshrate;
                    MaxMode.InterLaced = tempBuffer.InterLaced;
                    cbMode_GetHVTiming(pcbe,
                                       MaxMode.XResolution,
                                       MaxMode.YResolution,
                                       MaxMode.Refreshrate,
                                       MaxMode.InterLaced,
                                       Device,
                                       &Timing);
                    MaxMode.PixelClock = Timing.PixelClock;
                    EstIndex = i;
                    TimingType = CBIOS_MODE_EST_TIMING;
                }
            }
        }
        for(i=0; i<CBIOS_STDMODECOUNT; i++)
        {
            if((pEdidStruct->StdTimings[i].Valid) && (!(*(pbStdModeUsed+i))))
            {
                tempBuffer.XResolution = pEdidStruct->StdTimings[i].XResolution;
                tempBuffer.YResolution = pEdidStruct->StdTimings[i].YResolution;
                tempBuffer.Refreshrate = pEdidStruct->StdTimings[i].Refreshrate;
                tempBuffer.InterLaced = CBIOS_FALSE;

                if ((cbMode_CompareMode(&tempBuffer, &MaxMode)) > 0)
                {
                    MaxMode.XResolution = tempBuffer.XResolution;
                    MaxMode.YResolution = tempBuffer.YResolution;
                    MaxMode.Refreshrate = tempBuffer.Refreshrate;
                    MaxMode.InterLaced = tempBuffer.InterLaced;
                    cbMode_GetHVTiming(pcbe,
                                       MaxMode.XResolution,
                                       MaxMode.YResolution,
                                       MaxMode.Refreshrate,
                                       MaxMode.InterLaced,
                                       Device,
                                       &Timing);
                    MaxMode.PixelClock = Timing.PixelClock;
                    StdIndex = i;
                    TimingType = CBIOS_MODE_STD_TIMING;
                }
                else if ((cbMode_CompareMode(&tempBuffer, &MaxMode)) == 0)
                {
                    StdIndex = i;
                    TimingType |= CBIOS_MODE_STD_TIMING;
                }
            }
        }
        for(i=0; i<CBIOS_DTLMODECOUNT; i++)
        {
            if((pEdidStruct->DtlTimings[i].Valid)&&(!(*(pbDtlModeUsed + i))))
            {
                if ((cbMode_CompareMode(&(pEdidStruct->DtlTimings[i]), &MaxMode)) > 0)
                {
                    MaxMode.XResolution = pEdidStruct->DtlTimings[i].XResolution;
                    MaxMode.YResolution = pEdidStruct->DtlTimings[i].YResolution;
                    MaxMode.Refreshrate = pEdidStruct->DtlTimings[i].Refreshrate;
                    MaxMode.InterLaced = pEdidStruct->DtlTimings[i].InterLaced;
                    MaxMode.PixelClock = pEdidStruct->DtlTimings[i].PixelClock;
                    DtlIndex = i;
                    TimingType = CBIOS_MODE_DTL_TIMING;
                }
                else if ((cbMode_CompareMode(&(pEdidStruct->DtlTimings[i]), &MaxMode)) == 0)
                {
                    DtlIndex = i;
                    TimingType |= CBIOS_MODE_DTL_TIMING;
                }
            }
        }
        if((pEdidStruct->Attribute.IsCEA861Monitor)||
           (pEdidStruct->Attribute.IsCEA861HDMI))
        {
            for(i=0; i<CBIOS_HDMIFORMATCOUNTS; i++)
            {
                if((pEdidStruct->HDMIFormat[i].IsSupported) &&
                   (pcbe->pHDMISupportedFormatTable[i]) && (!(*(pbHDMIFormatUsed+i))))
                {
                    byRefRateIndex = pEdidStruct->HDMIFormat[i].RefreshIndex % 2;
                    tempBuffer.XResolution = pcbe->pHDMIFormatTable[i].XRes;
                    tempBuffer.YResolution = pcbe->pHDMIFormatTable[i].YRes;
                    tempBuffer.Refreshrate = pcbe->pHDMIFormatTable[i].RefRate[byRefRateIndex];
                    tempBuffer.InterLaced = pcbe->pHDMIFormatTable[i].Interlace;

                    if ((cbMode_CompareMode(&tempBuffer, &MaxMode)) > 0)
                    {
                        MaxMode.XResolution = tempBuffer.XResolution;
                        MaxMode.YResolution = tempBuffer.YResolution;
                        MaxMode.Refreshrate = tempBuffer.Refreshrate;
                        MaxMode.InterLaced = tempBuffer.InterLaced;
                        cbMode_GetHVTiming(pcbe,
                                       MaxMode.XResolution,
                                       MaxMode.YResolution,
                                       MaxMode.Refreshrate,
                                       MaxMode.InterLaced,
                                       Device,
                                       &Timing);
                        MaxMode.PixelClock = Timing.PixelClock;
                        CEAIndex = i;
                        TimingType = CBIOS_MODE_SVD_TIMING;
                    }
                    else if((cbMode_CompareMode(&tempBuffer, &MaxMode)) == 0)
                    {
                        CEAIndex = i;
                        TimingType |= CBIOS_MODE_SVD_TIMING;
                    }
                }
            }
            for (i = 0; i < CBIOS_DTDTIMINGCOUNTS; i++)
            {
                if((pEdidStruct->DTDTimings[i].Valid)&&(!(*(pbDTDModeUsed+i))))
                {
                    if ((cbMode_CompareMode(&(pEdidStruct->DTDTimings[i]), &MaxMode)) > 0)
                    {
                        MaxMode.XResolution = pEdidStruct->DTDTimings[i].XResolution;
                        MaxMode.YResolution = pEdidStruct->DTDTimings[i].YResolution;
                        MaxMode.Refreshrate = pEdidStruct->DTDTimings[i].Refreshrate;
                        MaxMode.InterLaced = pEdidStruct->DTDTimings[i].InterLaced;
                        MaxMode.PixelClock = pEdidStruct->DTDTimings[i].PixelClock;
                        DTDIndex = i;
                        TimingType = CBIOS_MODE_DTD_TIMING;
                    }
                    else if ((cbMode_CompareMode(&(pEdidStruct->DTDTimings[i]), &MaxMode)) == 0)
                    {
                        DTDIndex = i;
                        TimingType |= CBIOS_MODE_DTD_TIMING;
                    }
                }
            }
        }

        for (i = 0; i < CBIOS_DISPLAYID_TYPE1_MODECOUNT; i++)
        {
            if((pEdidStruct->DisplayID_TYPE1_Timings[i].Valid)&&(!(*(pbDisplayIDT1ModeUsed+i))))
            {
                if ((cbMode_CompareMode(&(pEdidStruct->DisplayID_TYPE1_Timings[i]), &MaxMode)) > 0)
                {
                    MaxMode.XResolution = pEdidStruct->DisplayID_TYPE1_Timings[i].XResolution;
                    MaxMode.YResolution = pEdidStruct->DisplayID_TYPE1_Timings[i].YResolution;
                    MaxMode.Refreshrate = pEdidStruct->DisplayID_TYPE1_Timings[i].Refreshrate;
                    MaxMode.InterLaced = pEdidStruct->DisplayID_TYPE1_Timings[i].InterLaced;
                    MaxMode.PixelClock = pEdidStruct->DisplayID_TYPE1_Timings[i].PixelClock;
                    DIDT1Index = i;
                    TimingType = CBIOS_MODE_DISPLAYID_TYPE1_TIMING;
                }
                else if ((cbMode_CompareMode(&(pEdidStruct->DisplayID_TYPE1_Timings[i]), &MaxMode)) == 0)
                {
                    DIDT1Index = i;
                    TimingType |= CBIOS_MODE_DISPLAYID_TYPE1_TIMING;
                }
            }
        }

        if (MaxMode.InterLaced == CBIOS_TRUE)
        {
            IntProgCaps = CBIOS_INTERLACECAP;
        }
        else
        {
            IntProgCaps = CBIOS_PROGRESSIVECAP;
        }

        tempType = TimingType;
        while(tempType)
        {
            curType = GET_LAST_BIT(tempType);
            switch (curType)
            {
            case CBIOS_MODE_DTD_TIMING:

                if (DTDIndex != 0xFF)
                {
                    if (pEdidStruct->DTDTimings[DTDIndex].IsNativeMode)
                    {
                        NativeModeFlags |= CBIOS_NATIVEMODE;
                    }

                    *(pbDTDModeUsed + DTDIndex) = CBIOS_TRUE;
                }
                tempType &= (~curType);
                break;

            case CBIOS_MODE_SVD_TIMING:

                if (CEAIndex != 0xFF)
                {
                    NativeModeFlags |= pEdidStruct->HDMIFormat[CEAIndex].IsNative;

                    IsCEAMode = 1;     //Means is a CE mode.

                    if(pcbe->pHDMIFormatTable[CEAIndex].AspectRatio == 1)
                    {
                        AspectRatioCaps = CBIOS_ASPECTRATIOCAP4B3;
                    }
                    else if(pcbe->pHDMIFormatTable[CEAIndex].AspectRatio == 3)
                    {
                        AspectRatioCaps = CBIOS_ASPECTRATIOCAP64B27;
                    }
                    else if(pcbe->pHDMIFormatTable[CEAIndex].AspectRatio == 4)
                    {
                        AspectRatioCaps = CBIOS_ASPECTRATIOCAP256B135;
                    }
                    else
                    {
                        AspectRatioCaps = CBIOS_ASPECTRATIOCAP16B9;
                    }
                    *(pbHDMIFormatUsed+CEAIndex) = CBIOS_TRUE;
                }
                tempType &= (~curType);
                break;

            case CBIOS_MODE_DTL_TIMING:

                if (DtlIndex != 0xFF)
                {
                    if (pEdidStruct->DtlTimings[DtlIndex].IsNativeMode)
                    {
                        NativeModeFlags |= CBIOS_NATIVEMODE;
                    }

                    if (DtlIndex == 0)
                    {
                        PreferredModeFlags = CBIOS_PERFERREDMODE;
                    }

                    *(pbDtlModeUsed+DtlIndex) = CBIOS_TRUE;
                }
                tempType &= (~curType);
                break;

            case CBIOS_MODE_STD_TIMING:

                if (StdIndex != 0xFF)
                {
                    *(pbStdModeUsed+StdIndex) = CBIOS_TRUE;
                }
                tempType &= (~curType);
                break;

            case CBIOS_MODE_EST_TIMING:

                if (EstIndex != 0xFF)
                {
                    *(pbEstModeUsed+EstIndex) = CBIOS_TRUE;
                }
                tempType &= (~curType);
                break;

            case CBIOS_MODE_DISPLAYID_TYPE1_TIMING:

                if(DIDT1Index != 0xFF)
                {
                    *(pbDisplayIDT1ModeUsed + DIDT1Index) = CBIOS_TRUE;
                }
                tempType &= (~curType);
                break;

            default:
                tempType &= (~curType);
                break;
            }
        }

        if ((DTDIndex == 0xFF) &&
            (CEAIndex == 0xFF) &&
            (DtlIndex == 0xFF) &&
            (StdIndex == 0xFF) &&
            (EstIndex == 0xFF) &&
            (DIDT1Index == 0xFF))
        {
            break;
        }

        // check whether to filter the mode
        if ((pFilter->bFilterInterlace) && (MaxMode.InterLaced))
        {
            bFilterThisMode = CBIOS_TRUE;
        }
        if (MaxMode.PixelClock > pFilter->MaxDclk)
        {
            bFilterThisMode = CBIOS_TRUE;
            cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "----Mode pixel clk(%d) greater than the max DClk(%d), filter this mode-----\n",
                MaxMode.PixelClock, pFilter->MaxDclk));
        }

        if ((SupportedMaxMode.XResolution) && (SupportedMaxMode.YResolution) && (SupportedMaxMode.Refreshrate))
        {
            if((MaxMode.XResolution > SupportedMaxMode.XResolution) || (MaxMode.YResolution > SupportedMaxMode.YResolution))
            {
                bFilterThisMode = CBIOS_TRUE;
                cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG),"cbMode_GetDeviceModeList: Filter mode %d x %d @ %d as it is greater than config max mode.\n",
                              MaxMode.XResolution, MaxMode.YResolution, MaxMode.Refreshrate));
            }

            if((MaxMode.XResolution == SupportedMaxMode.XResolution) && (MaxMode.YResolution == SupportedMaxMode.YResolution))
            {
                if(MaxMode.Refreshrate > SupportedMaxMode.Refreshrate)
                {
                    cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG),"cbMode_GetDeviceModeList: Filter mode %d x %d @ %d as it is greater than config max mode.\n",
                              MaxMode.XResolution, MaxMode.YResolution, MaxMode.Refreshrate));
                    bFilterThisMode = CBIOS_TRUE;
                }
            }
        }

        if (!bFilterThisMode)
        {
            ulModeNum++;
            ModeToInsert.XRes = MaxMode.XResolution;
            ModeToInsert.YRes = MaxMode.YResolution;
            ModeToInsert.RefreshRate = MaxMode.Refreshrate;
            ModeToInsert.InterlaceProgressiveCaps = IntProgCaps;
            ModeToInsert.AdapterDeviceFlags = CBIOS_DEVICEMODE;
            ModeToInsert.DeviceFlags = Device;
            ModeToInsert.ColorDepthCaps = CBIOS_COLORDEPTH16 |
                                          CBIOS_COLORDEPTH32XRGB |
                                          CBIOS_COLORDEPTH32ARGB |
                                          CBIOS_COLORDEPTH32ABGR |
                                          CBIOS_COLORDEPTH2101010ARGB |
                                          CBIOS_COLORDEPTH2101010ABGR |
                                          CBIOS_COLORDEPTH16161616ABGRF;
            ModeToInsert.AspectRatioCaps = AspectRatioCaps;
            ModeToInsert.NativeModeFlags = NativeModeFlags;
            ModeToInsert.ModeFlags = IsCEAMode;
            ModeToInsert.ModeFlags |= PreferredModeFlags;
            ModeToInsert.ModeFlags |= (TimingType << 2);

            if (pModeList != CBIOS_NULL)    // insert mode
            {
                // if space is enough, insert a item to mode list.
                if( ulModeNum <= MaxModeNum)
                {
                    if (!cbMode_AddMode(pModeList, &ModeToInsert, ulModeNum - 1, Device))
                    {
                        //insert mode fail, decrease mode num
                        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "cbMode_GetDeviceModeList: add mode failed!\n"));
                        ulModeNum--;
                    }

                }
                else
                {
                    cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbMode_GetDeviceModeList: buffer size for mode list is not enough!\n ulModeNum = %d MaxModeNum = %d\n", ulModeNum, MaxModeNum));
                    cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "DTDIndex = %d CEAIndex = %d DtlIndex = %d StdIndex = %d EstIndex = %d DIDT1Index = %d\n ", DTDIndex, CEAIndex, DtlIndex, StdIndex, EstIndex,DIDT1Index));
                    ulModeNum--;
                    break;
                }
            }

            /*********
            per CEA861-F spec
            if A Video Timing with a vertical frequency that is an integer multiple of 6.00 Hz
            (i.e., 24.00, 30.00, 60.00, 120.00 or 240.00 Hz) is considered to be the same as a
            Video Timing with the equivalent detailed timing information but where the vertical
            frequency is adjusted by a factor of 1000/1001

            here Add 5994(6000) for 720X480,  add 5994(6000) for 1280X720, add 5994(6000)  and 2997(3000) for 1920X1080 mode
            *********/
            if(Device & ALL_DP_TYPES)
            {
                cbMode_AddEquivalentDeviceMode(pModeList, &ulModeNum, &ModeToInsert, Device, MaxModeNum);
            }
        }
    }

    cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "cbMode_GetDeviceModeList: Total mode num is %d!\n", ulModeNum));
    cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "pbModeUsed = 0x%08X\n", pbModeUsed));

    cb_FreePool(pbModeUsed);
    return ulModeNum;
}


