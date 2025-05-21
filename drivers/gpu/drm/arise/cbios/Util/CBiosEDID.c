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

CBIOS_U8 PHL_24PFL3545_Edid[256] =
{
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x41, 0x0C, 0x10, 0x00, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x17, 0x01, 0x03, 0x80, 0x35, 0x1E, 0x78, 0x2A, 0x47, 0x89, 0xA3, 0x57, 0x47, 0x9F, 0x22,
    0x11, 0x48, 0x4C, 0xBF, 0xEF, 0x80, 0xB3, 0x00, 0x95, 0x00, 0x81, 0x80, 0x81, 0x40, 0x71, 0x4F,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C,
    0x45, 0x00, 0x09, 0x25, 0x21, 0x00, 0x00, 0x1E, 0x66, 0x21, 0x50, 0xB0, 0x51, 0x00, 0x1B, 0x30,
    0x40, 0x70, 0x36, 0x00, 0x09, 0x25, 0x21, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0xFD, 0x00, 0x37,
    0x4C, 0x1E, 0x52, 0x11, 0x00, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFC,
    0x00, 0x42, 0x32, 0x34, 0x50, 0x46, 0x4C, 0x33, 0x35, 0x34, 0x35, 0x2F, 0x54, 0x33, 0x01, 0x31,
    0x02, 0x03, 0x22, 0xF2, 0x4F, 0x9F, 0x14, 0x13, 0x12, 0x11, 0x16, 0x15, 0x90, 0x05, 0x04, 0x03,
    0x02, 0x07, 0x06, 0x01, 0x23, 0x09, 0x07, 0x01, 0x83, 0x01, 0x00, 0x00, 0x65, 0x03, 0x0C, 0x00,
    0x10, 0x00, 0x02, 0x3A, 0x80, 0xD0, 0x72, 0x38, 0x2D, 0x40, 0x10, 0x2C, 0x45, 0x80, 0x09, 0x25,
    0x21, 0x00, 0x00, 0x1E, 0x01, 0x1D, 0x80, 0xD0, 0x72, 0x1C, 0x16, 0x20, 0x10, 0x2C, 0x25, 0x80,
    0x09, 0x25, 0x21, 0x00, 0x00, 0x9E, 0x01, 0x1D, 0x00, 0xBC, 0x52, 0xD0, 0x1E, 0x20, 0xB8, 0x28,
    0x55, 0x40, 0x09, 0x25, 0x21, 0x00, 0x00, 0x18, 0x8C, 0x0A, 0xD0, 0x90, 0x20, 0x40, 0x31, 0x20,
    0x0C, 0x40, 0x55, 0x00, 0x09, 0x25, 0x21, 0x00, 0x00, 0x18, 0x02, 0x3A, 0x80, 0x18, 0x71, 0x38,
    0x2D, 0x40, 0x58, 0x2C, 0x45, 0x00, 0x09, 0x25, 0x21, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x26,
};

//RefRateIndex init to 0FFh, which means not legal index.
CBIOS_HDMI_FORMAT_MTX CEAVideoFormatTable[] =
{
//               1:interlace      1: 4:3; 2: 16:9; 3: 64:27; 4: 256:135
// Video          interlace       Aspect
// codes   H     V        V freq  Ratio
    //normal VICs
    {1,   640,  480, 0, { 6000, 5994}, 1, 1},
    {2,   720,  480, 0, { 6000, 5994}, 1, 1},
    {3,   720,  480, 0, { 6000, 5994}, 2, 1},
    {4,  1280,  720, 0, { 6000, 5994}, 2, 0},
    {5,  1920, 1080, 1, { 6000, 5994}, 2, 0},
    {6,   720,  480, 1, { 6000, 5994}, 1, 1},
    {7,   720,  480, 1, { 6000, 5994}, 2, 1},
    {8,   720,  240, 0, { 6000, 5994}, 1, 1},
    {9,   720,  240, 0, { 6000, 5994}, 2, 1},
    {10, 1440,  480, 1, { 6000, 5994}, 1, 1},
    {11, 1440,  480, 1, { 6000, 5994}, 2, 1},
    {12, 1440,  240, 0, { 6000, 5994}, 1, 1},
    {13, 1440,  240, 0, { 6000, 5994}, 2, 1},
    {14, 1440,  480, 0, { 6000, 5994}, 1, 1},
    {15, 1440,  480, 0, { 6000, 5994}, 2, 1},
    {16, 1920, 1080, 0, { 6000, 5994}, 2, 0},
    {17,  720,  576, 0, { 5000, 0000}, 1, 0},
    {18,  720,  576, 0, { 5000, 0000}, 2, 0},
    {19, 1280,  720, 0, { 5000, 0000}, 2, 0},
    {20, 1920, 1080, 1, { 5000, 0000}, 2, 0},
    {21,  720,  576, 1, { 5000, 0000}, 1, 0},
    {22,  720,  576, 1, { 5000, 0000}, 2, 0},
    {23,  720,  288, 0, { 5000, 0000}, 1, 0},
    {24,  720,  288, 0, { 5000, 0000}, 2, 0},
    {25, 1440,  576, 1, { 5000, 0000}, 1, 0},
    {26, 1440,  576, 1, { 5000, 0000}, 2, 0},
    {27, 1440,  288, 0, { 5000, 0000}, 1, 0},
    {28, 1440,  288, 0, { 5000, 0000}, 2, 0},
    {29, 1440,  576, 0, { 5000, 0000}, 1, 0},
    {30, 1440,  576, 0, { 5000, 0000}, 2, 0},
    {31, 1920, 1080, 0, { 5000, 0000}, 2, 0},
    {32, 1920, 1080, 0, { 2400, 2397}, 2, 0},
    {33, 1920, 1080, 0, { 2500, 0000}, 2, 0},
    {34, 1920, 1080, 0, { 3000, 2997}, 2, 0},
    {35, 2880, 480,  0, { 6000, 5994}, 1, 1},
    {36, 2880, 480,  0, { 6000, 5994}, 2, 1},
    {37, 2880, 576,  0, { 5000, 0000}, 1, 0},
    {38, 2880, 576,  0, { 5000, 0000}, 2, 0},
    {39, 1920, 1080 ,1, { 5000, 0000}, 2, 0},
    {40, 1920, 1080, 1, {10000, 0000}, 2, 0},
    {41, 1280,  720, 0, {10000, 0000}, 2, 0},
    {42,  720,  576, 0, {10000, 0000}, 1, 0},
    {43,  720,  576, 0, {10000, 0000}, 2, 0},
    {44,  720,  576, 1, {10000, 0000}, 1, 0},
    {45,  720,  576, 1, {10000, 0000}, 2, 0},
    {46, 1920, 1080, 1, {12000,11988}, 2, 0},
    {47, 1280,  720, 0, {12000,11988}, 2, 0},
    {48,  720,  480, 0, {12000,11988}, 1, 1},
    {49,  720,  480, 0, {12000,11988}, 2, 1},
    {50,  720,  480, 1, {12000,11988}, 1, 1},
    {51,  720,  480, 1, {12000,11988}, 2, 1},
    {52,  720,  576, 0, {20000, 0000}, 1, 0},
    {53,  720,  576, 0, {20000, 0000}, 2, 0},
    {54,  720,  576, 1, {20000, 0000}, 1, 0},
    {55,  720,  576, 1, {20000, 0000}, 2, 0},
    {56,  720,  480, 0, {24000,23976}, 1, 1},
    {57,  720,  480, 0, {24000,23976}, 2, 1},
    {58,  720,  480, 1, {24000,23976}, 1, 1},
    {59,  720,  480, 1, {24000,23976}, 2, 1},
    {60, 1280,  720, 0, { 2400, 2398}, 2, 0},
    {61, 1280,  720, 0, { 2500, 0000}, 2, 0},
    {62, 1280,  720, 0, { 3000, 2997}, 2, 0},
    {63, 1920, 1080, 0, {12000,11988}, 2, 0},
    {64, 1920, 1080, 0, {10000, 0000}, 2, 0},
// CEA-861-F
    {65, 1280,  720, 0, { 2400, 2398}, 3, 0},
    {66, 1280,  720, 0, { 2500, 0000}, 3, 0},
    {67, 1280,  720, 0, { 3000, 2997}, 3, 0},
    {68, 1280,  720, 0, { 5000, 0000}, 3, 0},
    {69, 1280,  720, 0, { 6000, 5994}, 3, 0},
    {70, 1280,  720, 0, {10000, 0000}, 3, 0},
    {71, 1280,  720, 0, {12000,11988}, 3, 0},
    {72, 1920, 1080, 0, { 2400, 2398}, 3, 0},
    {73, 1920, 1080, 0, { 2500, 0000}, 3, 0},
    {74, 1920, 1080, 0, { 3000, 2997}, 3, 0},
    {75, 1920, 1080, 0, { 5000, 0000}, 3, 0},
    {76, 1920, 1080, 0, { 6000, 5994}, 3, 0},
    {77, 1920, 1080, 0, {10000, 0000}, 3, 0},
    {78, 1920, 1080, 0, {12000,11988}, 3, 0},
    {79, 1680,  720, 0, { 2400, 2398}, 3, 0},
    {80, 1680,  720, 0, { 2500, 0000}, 3, 0},
    {81, 1680,  720, 0, { 3000, 2997}, 3, 0},
    {82, 1680,  720, 0, { 5000, 0000}, 3, 0},
    {83, 1680,  720, 0, { 6000, 5994}, 3, 0},
    {84, 1680,  720, 0, {10000, 0000}, 3, 0},
    {85, 1680,  720, 0, {12000,11988}, 3, 0},
    {86, 2560, 1080, 0, { 2400, 2398}, 3, 0},
    {87, 2560, 1080, 0, { 2500, 0000}, 3, 0},
    {88, 2560, 1080, 0, { 3000, 2997}, 3, 0},
    {89, 2560, 1080, 0, { 5000, 0000}, 3, 0},
    {90, 2560, 1080, 0, { 6000, 5994}, 3, 0},
    {91, 2560, 1080, 0, {10000, 0000}, 3, 0},
    {92, 2560, 1080, 0, {12000,11988}, 3, 0},
    {93, 3840, 2160, 0, { 2400, 2398}, 2, 0},
    {94, 3840, 2160, 0, { 2500, 0000}, 2, 0},
    {95, 3840, 2160, 0, { 3000, 2997}, 2, 0},
    {96, 3840, 2160, 0, { 5000, 0000}, 2, 0},
    {97, 3840, 2160, 0, { 6000, 5994}, 2, 0},
    {98, 4096, 2160, 0, { 2400, 2398}, 4, 0},
    {99, 4096, 2160, 0, { 2500, 0000}, 4, 0},
    {100,4096, 2160, 0, { 3000, 2997}, 4, 0},
    {101,4096, 2160, 0, { 5000, 0000}, 4, 0},
    {102,4096, 2160, 0, { 6000, 5994}, 4, 0},
    {103,3840, 2160, 0, { 2400, 2398}, 3, 0},
    {104,3840, 2160, 0, { 2500, 0000}, 3, 0},
    {105,3840, 2160, 0, { 3000, 2997}, 3, 0},
    {106,3840, 2160, 0, { 5000, 0000}, 3, 0},
    {107,3840, 2160, 0, { 6000, 5994}, 3, 0},

    //normal VIC end
    //extened VIC begin
    { CBIOS_HDMI_NORMAL_VIC_COUNTS + 1, 3840, 2160, 0, { 3000, 2997}, 2, 0},
    { CBIOS_HDMI_NORMAL_VIC_COUNTS + 2, 3840, 2160, 0, { 2500, 0000}, 2, 0},
    { CBIOS_HDMI_NORMAL_VIC_COUNTS + 3, 3840, 2160, 0, { 2400, 2397}, 2, 0},
    { CBIOS_HDMI_NORMAL_VIC_COUNTS + 4, 4096, 2160, 0, { 2400, 0000}, 2, 0},
    //extened VIC end

};

static DETAILEDTIMING_TABLE EDIDPixelClock[]= {
    {EDIDTIMING,0x00,0xFF},
    {EDIDTIMING,0x01,0xFF},
    {EDIDTIMINGEXIT,},
};
static DETAILEDTIMING_TABLE EDIDHorizontalActive[] = {
    {EDIDTIMING,0x02,0xFF},
    {EDIDTIMING,0X04,0XF0},
    {EDIDTIMINGEXIT,},
};
static DETAILEDTIMING_TABLE EDIDHorizontalBlanking[] = {
    {EDIDTIMING,0x03,0xFF},
    {EDIDTIMING,0X04,0X0F},
    {EDIDTIMINGEXIT,},
};
static DETAILEDTIMING_TABLE EDIDVerticalActive[] = {
    {EDIDTIMING,0x05,0xFF},
    {EDIDTIMING,0X07,0XF0},
    {EDIDTIMINGEXIT,},
};
static DETAILEDTIMING_TABLE EDIDVerticalBlanking[] = {
    {EDIDTIMING,0x06,0xFF},
    {EDIDTIMING,0X07,0X0F},
    {EDIDTIMINGEXIT,},
};
static DETAILEDTIMING_TABLE EDIDHorizontalSyncOffset[] = {
    {EDIDTIMING,0x08,0xFF},
    {EDIDTIMING,0x0B,0xC0},
    {EDIDTIMINGEXIT,},
};
static DETAILEDTIMING_TABLE EDIDHorizontalSyncPulseWidth[] = {
    {EDIDTIMING,0x09,0xFF},
    {EDIDTIMING,0x0B,0x30},
    {EDIDTIMINGEXIT,},
};
static DETAILEDTIMING_TABLE EDIDVerticalSyncOffset[] = {
    {EDIDTIMING,0x0A,0xF0},
    {EDIDTIMING,0x0B,0x0C},
    {EDIDTIMINGEXIT,},
};
static DETAILEDTIMING_TABLE EDIDVerticalSyncPulseWidth[] = {
    {EDIDTIMING,0x0A,0x0F},
    {EDIDTIMING,0x0B,0x03},
    {EDIDTIMINGEXIT,},
};
// The following is just for DTV
static DETAILEDTIMING_TABLE EDIDHorizontalImageSize[] = {
    {EDIDTIMING,0x0C,0xFF},
    {EDIDTIMING,0x0E,0xF0},
    {EDIDTIMINGEXIT,},
};
static DETAILEDTIMING_TABLE EDIDVerticalImageSize[] = {
    {EDIDTIMING,0x0D,0xFF},
    {EDIDTIMING,0x0E,0x0F},
    {EDIDTIMINGEXIT,},
};
/*
static DETAILEDTIMING_TABLE EDIDIsInterlaced[] = {
    {EDIDTIMING,0x11,0x80},
    {EDIDTIMINGEXIT,},
};
*/
static CBIOS_Module_EDID_ESTTIMINGS EstTiming1[] = {
    { 0x320, 0x258, 6000},// EDID23h[0]: 800x600@60Hz
    { 0x320, 0x258, 5600},// EDID23h[1]: 800x600@56Hz
    { 0x280, 0x1E0, 7500},// EDID23h[2]: 640x480@75Hz
    { 0x280, 0x1E0, 7200},// EDID23h[3]: 640x480@72Hz
    { 0x280, 0x1E0, 6700},// EDID23h[4]: 640x480@67Hz
    { 0x280, 0x1E0, 6000},// EDID23h[5]: 640x480@60Hz
    { 0x2D0, 0x190, 8800},// EDID23h[6]: 720x400@88Hz
    { 0x2D0, 0x190, 7000},// EDID23h[7]: 720x400@70Hz
};
static CBIOS_Module_EDID_ESTTIMINGS EstTiming2[] = {
    { 0x500, 0x400, 7500},// EDID24h[0]: 1280x1024@75Hz
    { 0x400, 0x300, 7500},// EDID24h[1]: 1024x768@75Hz
    { 0x400, 0x300, 7000},// EDID24h[2]: 1024x768@70Hz
    { 0x400, 0x300, 6000},// EDID24h[3]: 1024x768@60Hz
    { 0x400, 0x300, 8700},// EDID24h[4]: 1024x768@87HzInterlace
    { 0x340, 0x270, 7500},// EDID24h[5]: 832x624@75Hz
    { 0x320, 0x258, 7500},// EDID24h[6]: 800x600@75Hz
    { 0x320, 0x258, 7200},// EDID24h[7]: 800x600@72Hz
};
static CBIOS_Module_EDID_ESTTIMINGS EstTiming3[] = {
    { 0x480, 0x366, 7500},// EDID25h[7]: 1152x870@75Hz
};

static CBIOS_BOOL cbEDIDModule_GetFmtIdxFromSVD(CBIOS_U8 SVD, CBIOS_U8 *pFormatIdx, CBIOS_BOOL *pIsNative)
{
    CBIOS_BOOL bRet = CBIOS_TRUE;

    *pIsNative = CBIOS_FALSE;
/*
According to CEA-861-F:
    If SVD >=1 and SVD <=64 then
        7-bit VIC is defined (7-LSB\u2019s) and NOT a native code
    Elseif SVD >=65 and SVD <=127 then
        8-bit VIC is defined (from first new set)
    Elseif SVD >=129 and SVD <=192 then
        7-bit VIC is defined (7-LSB\u2019s) and IS a native code
    Elseif SVD >=193 and SVD <=253 then
        8-bit VIC is defined (from second new set)
    Elseif SVD == 0/128/254/255 then
        Reserved
    End if
*/
    if (SVD >=1 && SVD <= 64)
    {
        *pFormatIdx = SVD & 0x7F;
    }
    else if (SVD >= 65 && SVD <= 127)
    {
        *pFormatIdx = SVD;
    }
    else if (SVD >= 129 && SVD <= 192)
    {
        *pFormatIdx = SVD & 0x7F;
        *pIsNative = CBIOS_TRUE;
    }
    else if ((SVD >= 193) && (SVD <= 253))
    {
        *pFormatIdx = SVD;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbEDIDModule_GetFmtIdxFromSVD: SVD = %d which is a reserved SVD code\n", SVD));
        bRet = CBIOS_FALSE;
    }

    if ((*pFormatIdx == 0) || (*pFormatIdx > CBIOS_HDMI_NORMAL_VIC_COUNTS))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbEDIDModule_GetFmtIdxFromSVD: FormatIdx = %d which is invalid\n", *pFormatIdx));
        bRet = CBIOS_FALSE;
    }

    return bRet;
}

static CBIOS_U32 cbEDIDModule_MapMaskGetEdidInfo(CBIOS_U8* DetailedTimings, CBIOS_U32 Base, DETAILEDTIMING_TABLE* TimingTable)
{
    CBIOS_U32 uRet = 0;
    CBIOS_U8 byRegValue = 0;
    DETAILEDTIMING_TABLE *reg = TimingTable;
    CBIOS_U32 i,j=0;

    while(reg->type != EDIDTIMINGEXIT)
    {
        byRegValue = DetailedTimings[18*Base + reg->index];
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

static CBIOS_BOOL cbEDIDModule_ParseDtlTiming(CBIOS_U8 *pEdidDtlData, PCBIOS_MODE_INFO_EXT pDtlTiming)
{
    CBIOS_BOOL bRet = CBIOS_FALSE;
    CBIOS_U16 Ratio = 0;

    pDtlTiming->PixelClock =
        cbEDIDModule_MapMaskGetEdidInfo(pEdidDtlData,0,EDIDPixelClock)*100;
    if(pDtlTiming->PixelClock == 0)
    {
        pDtlTiming->Valid = 0;
        bRet = CBIOS_FALSE;
    }
    else
    {
        pDtlTiming->Valid = 1;
        bRet = CBIOS_TRUE;
        pDtlTiming->HActive =
            (CBIOS_U16)cbEDIDModule_MapMaskGetEdidInfo(pEdidDtlData,0,EDIDHorizontalActive);
        pDtlTiming->HSyncOffset =
            (CBIOS_U16)cbEDIDModule_MapMaskGetEdidInfo(pEdidDtlData,0,EDIDHorizontalSyncOffset);
        pDtlTiming->HSyncPulseWidth =
            (CBIOS_U16)cbEDIDModule_MapMaskGetEdidInfo(pEdidDtlData,0,EDIDHorizontalSyncPulseWidth);
        pDtlTiming->HBlank =
            (CBIOS_U16)cbEDIDModule_MapMaskGetEdidInfo(pEdidDtlData,0,EDIDHorizontalBlanking);

        pDtlTiming->VActive =
            (CBIOS_U16)cbEDIDModule_MapMaskGetEdidInfo(pEdidDtlData,0,EDIDVerticalActive);
        pDtlTiming->VSyncOffset =
            (CBIOS_U16)cbEDIDModule_MapMaskGetEdidInfo(pEdidDtlData,0,EDIDVerticalSyncOffset);
        pDtlTiming->VSyncPulseWidth =
            (CBIOS_U16)cbEDIDModule_MapMaskGetEdidInfo(pEdidDtlData,0,EDIDVerticalSyncPulseWidth);
        pDtlTiming->VBlank =
            (CBIOS_U16)cbEDIDModule_MapMaskGetEdidInfo(pEdidDtlData,0,EDIDVerticalBlanking);
        pDtlTiming->HImageSize =
            (CBIOS_U16)cbEDIDModule_MapMaskGetEdidInfo(pEdidDtlData,0,EDIDHorizontalImageSize);
        pDtlTiming->VImageSize =
            (CBIOS_U16)cbEDIDModule_MapMaskGetEdidInfo(pEdidDtlData,0,EDIDVerticalImageSize);

        if((pDtlTiming->HActive == 0 )|| (pDtlTiming->VActive == 0))
        {
            pDtlTiming->Valid = 0;
            bRet = CBIOS_FALSE;
        }
        else
        {
            if(pDtlTiming->VImageSize != 0)   // Prevent being divided by zero
            {
                Ratio = (CBIOS_U16)((CBIOS_U32)(pDtlTiming->HImageSize * 10000) / pDtlTiming->VImageSize);

                if((Ratio>13000) && (Ratio<14000))  // 4:3
                    pDtlTiming->AspectRatio = 1;
                else    //16:9
                    pDtlTiming->AspectRatio = 2;
            }
            else
            {
                cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "cbEDIDModule_ParseDtlTiming: fata error -- pDtlTiming->VImageSize is ZERO!!!\n"));
                pDtlTiming->AspectRatio = 2;
            }

            // HSync = 1, means HSync is positive.
            // else, means negative. the same as VSync.
            if((pEdidDtlData[0x11]&0x18) == 0x18)
            {
                //V polarity
                if((pEdidDtlData[0x11]&0x04) == 0x04)
                    pDtlTiming->VSync = VerPOSITIVE;
                else
                    pDtlTiming->VSync = VerNEGATIVE;

                //H polarity
                if((pEdidDtlData[0x11]&0x02) == 0x02)
                    pDtlTiming->HSync = HorPOSITIVE;
                else
                    pDtlTiming->HSync = HorNEGATIVE;
            }
            else
            {
                pDtlTiming->HSync = HorNEGATIVE;
                pDtlTiming->VSync = VerNEGATIVE;
            }

            // Prevent being divided by zero
            if ((pDtlTiming->HActive + pDtlTiming->HBlank) == 0)
            {
                cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "cbEDIDModule_ParseDtlTiming: fata error -- (pDtlTiming->HActive + pDtlTiming->HBlank) is ZERO!!!\n"));
                return CBIOS_FALSE;
            }
            if ((pDtlTiming->VActive + pDtlTiming->VBlank) == 0)
            {
                cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "cbEDIDModule_ParseDtlTiming: fata error -- (pDtlTiming->VActive + pDtlTiming->VBlank) is ZERO!!!\n"));
                return CBIOS_FALSE;
            }

            pDtlTiming->Refreshrate = cbCalcRefreshRate(pDtlTiming->PixelClock,
                                                        pDtlTiming->HActive,
                                                        pDtlTiming->HBlank,
                                                        pDtlTiming->VActive,
                                                        pDtlTiming->VBlank);

            //Adjust refreshrate, for CEA mode, CBIOS will add the 5994Hz mode
            //in the end of cbModeModule_GetDeviceMode
            pDtlTiming->Refreshrate = 100 * cbRound(pDtlTiming->Refreshrate, 100, ROUND_NEAREST);

            if(pEdidDtlData[0x11]&0x80)
            {
               pDtlTiming->InterLaced = CBIOS_TRUE;
               pDtlTiming->VActive *= 2;
               pDtlTiming->VBlank *=2;
               pDtlTiming->VBlank += 1;
               pDtlTiming->VSyncOffset *= 2;
               pDtlTiming->VSyncPulseWidth *= 2;
            }
            else
            {
               pDtlTiming->InterLaced = CBIOS_FALSE;
            }
        }
    }
    if(!bRet)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG),"cbEDIDModule_ParseDtlTiming: Dtl Timing invalid!\n"));
    }

    return bRet;
}

/***************************************************************
Function:    cbEDIDModule_GetEstablishMode

Description: Get EDID establish mode

Input:       pEDID, EDID data buffer

Output:      pEstablishMode, statistic establish mode in EDID

Return:      the number of establish mode supported in EDID
***************************************************************/
static CBIOS_U32 cbEDIDModule_GetEstablishMode(CBIOS_U8 *pEDID, PCBIOS_MODE_INFO pEstablishMode)
{
    CBIOS_U32 ulNumOfEstMode = 0;
    CBIOS_U32 i = 0;
    CBIOS_U32 ulMask = 0x01;
    CBIOS_U8 *pEstTimingsInEDID = pEDID + ESTABLISH_TIMINGS_INDEX;


    for(i=0; i<8; i++)
    {
        if( *pEstTimingsInEDID & ulMask )
        {
            ulNumOfEstMode++;
            pEstablishMode[i].Valid = 1;
            pEstablishMode[i].XResolution = EstTiming1[i].XResolution;
            pEstablishMode[i].YResolution = EstTiming1[i].YResolution;
            pEstablishMode[i].Refreshrate = EstTiming1[i].RefreshRate;
        }
        else
        {
            pEstablishMode[i].Valid = 0;
        }
        ulMask = ulMask << 1;
    }

    ulMask = 0x01;
    for(i=0; i<8; i++)
    {
        if( *(pEstTimingsInEDID+1) & ulMask )
        {
            ulNumOfEstMode++;
            pEstablishMode[i+8].Valid = 1;
            pEstablishMode[i+8].XResolution = EstTiming2[i].XResolution;
            pEstablishMode[i+8].YResolution = EstTiming2[i].YResolution;
            pEstablishMode[i+8].Refreshrate = EstTiming2[i].RefreshRate;
        }
        else
        {
            pEstablishMode[i+8].Valid = 0;
        }
        ulMask = ulMask << 1;
    }

    ulMask = 0x80;
    if ( *(pEstTimingsInEDID+2) & ulMask )
    {
        ulNumOfEstMode++;
        pEstablishMode[16].Valid = 1;
        pEstablishMode[16].XResolution = EstTiming3[0].XResolution;
        pEstablishMode[16].YResolution = EstTiming3[0].YResolution;
        pEstablishMode[16].Refreshrate = EstTiming3[0].RefreshRate;
    }
    else
    {
        pEstablishMode[16].Valid = 0;
    }

    return ulNumOfEstMode;
}

/***************************************************************
Function:    cbEDIDModule_GetStandardMode

Description: Get EDID standard mode

Input:       pEDID, EDID data buffer

Output:      pStandardMode, statistic standard mode in EDID

Return:      the number of standard mode supported in EDID
***************************************************************/
static CBIOS_U32 cbEDIDModule_GetStandardMode(CBIOS_U8 *pEDID, PCBIOS_MODE_INFO pStandardMode)
{
    CBIOS_U32 ulNumOfStdMode = 0;
    CBIOS_U32 i = 0;
    CBIOS_U8 *pStdTimingsInEDID = pEDID + STANDARD_TIMINGS_INDEX;

    for ( i=0; i<8; i++)
    {
        if ((*(pStdTimingsInEDID + i*2) == 0x01) ||
            (*(pStdTimingsInEDID + i*2) == 0x00))
        {
            pStandardMode[i].Valid = 0;
            continue;
        }

        ulNumOfStdMode++;
        pStandardMode[i].Valid = 1;
        pStandardMode[i].XResolution = (CBIOS_U16)(*(pStdTimingsInEDID + i*2) + 31) * 8;
        pStandardMode[i].Refreshrate = ((*(pStdTimingsInEDID + i*2 + 1) & 0x3F) + 60) * 100;

        switch((*(pStdTimingsInEDID + i*2 + 1) & 0xC0) >> 6) //Image Aspect Ratio
        {
            case 0: // 16:10 AR
            pStandardMode[i].YResolution = pStandardMode[i].XResolution*10 / 16 * 10 /10;
            break;
            case 1: // 4:3 AR
            pStandardMode[i].YResolution = pStandardMode[i].XResolution*10 / 4 * 3 /10;
            break;
            case 2: // 5:4 AR
            pStandardMode[i].YResolution = pStandardMode[i].XResolution*10 / 5 * 4 /10;
            break;
            case 3: // 16:9 AR
            pStandardMode[i].YResolution = pStandardMode[i].XResolution*10 / 16 * 9 /10;
            break;
        }
    }

    return ulNumOfStdMode;
}

/***************************************************************
Function:    cbEDIDModule_GetDetailedMode

Description: Get detailed timings in base EDID

Input:       pEDID, EDID data buffer
             byTotalModeNum, specify how many 18 byte data blocks want to parse

Output:      pDetailedMode, statistic detailed timings in base EDID

Return:      the number of detailed timings get in base EDID
***************************************************************/
static CBIOS_U32 cbEDIDModule_GetDetailedMode(CBIOS_U8 *pEDID, PCBIOS_MODE_INFO_EXT pDetailedMode, CBIOS_U32 byTotalModeNum)
{
    CBIOS_U32  ulNumOfDtlMode = 0;
    CBIOS_U32  i = 0;
    CBIOS_U8   *pDtlTimingsInBaseEDID = pEDID + DETAILED_TIMINGS_INDEX;

    //get detailed timings in base EDID 18 byte data blocks
    for(i = 0; i < byTotalModeNum; i++, pDtlTimingsInBaseEDID += 18)
    {
        if (pDtlTimingsInBaseEDID + 18 > pEDID + 128)
        {
            break;
        }

        if(!cbEDIDModule_ParseDtlTiming(pDtlTimingsInBaseEDID, &pDetailedMode[i]))
        {
            continue;
        }

        ulNumOfDtlMode++;
    }

    return ulNumOfDtlMode;
}

/***************************************************************
Function:    cbEDIDModule_GetCEADetailedMode

Description: Get detailed timing descriptor in CEA extension

Input:       pEDID, EDID data buffer

Output:      pExtDtlMode, statistic detailed timings in CEA extension

Return:      the number of detailed timings get in CEA extension
***************************************************************/
static CBIOS_U32 cbEDIDModule_GetCEADetailedMode(CBIOS_U8 *pEDID, PCBIOS_MODE_INFO_EXT pCEADtlMode, CBIOS_U32 TotalBlocks)
{
    CBIOS_U32   ulNumOfExtDtlMode = 0;
    CBIOS_U32   i, j = 0;
    CBIOS_U32   BlockIndex = 0;
    CBIOS_U8    DtlTimingOffset = 0;    // Detailed Timing Descriptor offset in extension block
    CBIOS_U8    *pEDIDBlock = CBIOS_NULL;
    CBIOS_BOOL  isNeedPixelRep = CBIOS_FALSE;
    CBIOS_BOOL  isCEAMode = CBIOS_FALSE;
    CBIOS_MODE_INFO_EXT  tmpExtDltTiming;

    if (TotalBlocks > MAX_EDID_BLOCK_NUM)
    {
        // TBD: support for more than 8 blocks
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbEDIDModule_GetCEADetailedMode: Total %d blocks but currently only parse 8 blocks!\n", TotalBlocks));
        //ASSERT(CBIOS_FALSE);

        TotalBlocks = MAX_EDID_BLOCK_NUM;
    }

    //get detailed timings in CEA extension
    for (BlockIndex = 1; BlockIndex < TotalBlocks; BlockIndex++)
    {
        pEDIDBlock = pEDID + BlockIndex * 128;

        //check CEA Tag
        if (pEDIDBlock[0x00] != CEA_TAG)
        {
            continue;
        }

        DtlTimingOffset = pEDIDBlock[0x02];
        if (DtlTimingOffset == 0)
        {
            //no detailed timing block, no reserved data block
            continue;
        }

        // Parse the extension detailed timing descriptors
        i = DtlTimingOffset;
        while ((i + 18) < 128)
        {
            cb_memset(&tmpExtDltTiming, 0, sizeof(CBIOS_MODE_INFO_EXT));
            if (!cbEDIDModule_ParseDtlTiming(&pEDIDBlock[i], &tmpExtDltTiming)
                 || ulNumOfExtDtlMode >= CBIOS_DTDTIMING_BLOCK_CNT)
            {
                break;
            }
            else
            {
                //Check pixel repetition
                //For CEA format 6~9, 21~24, horizontal timing stored in EDID is doubled, so we should add /2 patch to get the real timing
                if (tmpExtDltTiming.HActive == 1440)//check horizontal resolution. Since we only support 2 times pixel repetition, so xres == 1440
                {
                    if ((tmpExtDltTiming.InterLaced && ((tmpExtDltTiming.VActive == 480) || (tmpExtDltTiming.VActive == 576))) //720x480i & 720x576i
                        ||((!tmpExtDltTiming.InterLaced) && ((tmpExtDltTiming.VActive == 240) || (tmpExtDltTiming.VActive == 288))))//720x240p & 720x288p
                    {
                        isNeedPixelRep = CBIOS_TRUE;
                        tmpExtDltTiming.HActive /= 2;
                    }
                }

                //check whether it's CEA mode.
                for(j = 0; j < sizeofarray(CEAVideoFormatTable); j++)
                {
                    if((tmpExtDltTiming.HActive == CEAVideoFormatTable[j].XRes)&&
                        (tmpExtDltTiming.VActive == CEAVideoFormatTable[j].YRes)&&
                        (tmpExtDltTiming.InterLaced == CEAVideoFormatTable[j].Interlace)&&
                        (tmpExtDltTiming.AspectRatio == CEAVideoFormatTable[j].AspectRatio)&&
                        (((tmpExtDltTiming.Refreshrate / 100) == (CEAVideoFormatTable[j].RefRate[0] / 100))||
                        ((tmpExtDltTiming.Refreshrate / 100) == (CEAVideoFormatTable[j].RefRate[1] / 100))))
                    {
                        isCEAMode = CBIOS_TRUE;
                        break;
                    }
                }

                //If current mode is not CEA mode, we will not enable pixel repetition, so restore HActive back.
                if ((!isCEAMode) && isNeedPixelRep)
                {
                    isNeedPixelRep = CBIOS_FALSE;
                    tmpExtDltTiming.HActive *= 2;
                }

                cb_memcpy(&pCEADtlMode[ulNumOfExtDtlMode++], &tmpExtDltTiming, sizeof(CBIOS_MODE_INFO_EXT));

            }
            i += 18;
        }

    }

    return ulNumOfExtDtlMode;
}

/***************************************************************
Function:    cbEDIDModule_GetSVDMode

Description: Get short video descriptor in video data block of CEA extension

Input:       pSVDDataInEDID, short video descriptor data buffer
             BlockIndex, the index of data block

Output:      pEDIDStruct, statistic video format in short video descriptor and SVD data block

Return:      the number of video format get in short video descriptor
***************************************************************/
static CBIOS_U32 cbEDIDModule_GetSVDMode(CBIOS_U8 *pSVDDataInEDID, PCBIOS_EDID_STRUCTURE_DATA pEDIDStruct, CBIOS_U32 BlockIndex)
{
    CBIOS_U32   ulNumOfSVDMode = 0;
    CBIOS_U32   PayloadLength = 0;
    PCBIOS_HDMI_FORMAT_DESCRIPTOR pCEAVideoFormat = CBIOS_NULL;
    CBIOS_U32   j = 0;
    CBIOS_U8    SVD = 0;
    CBIOS_BOOL  IsNative = CBIOS_FALSE;
    CBIOS_BOOL  Status = CBIOS_FALSE;

    //decode short video descriptor
    PayloadLength = pSVDDataInEDID[0] & 0x1F;

    pEDIDStruct->Attribute.SVDData[BlockIndex - 1].SVDCount = (CBIOS_U8)PayloadLength;
    cb_memcpy(pEDIDStruct->Attribute.SVDData[BlockIndex - 1].SVD, &pSVDDataInEDID[1], PayloadLength);

    pCEAVideoFormat = pEDIDStruct->HDMIFormat;
    for (j = 0; j < PayloadLength; j++)
    {
        CBIOS_U8 FormatNum = 0;

        SVD = pSVDDataInEDID[1 + j];
        Status = cbEDIDModule_GetFmtIdxFromSVD(SVD, &FormatNum, &IsNative);
        if (!Status)
        {
            continue;
        }

        //for formats listed in SVD, use default refresh rate
        pCEAVideoFormat[FormatNum - 1].RefreshIndex = CEAVideoFormatTable[FormatNum - 1].DefaultRefRateIndex;

        if (!pCEAVideoFormat[FormatNum - 1].IsSupported)
        {
            pCEAVideoFormat[FormatNum - 1].BlockIndex = (CBIOS_U8)BlockIndex;
            pCEAVideoFormat[FormatNum - 1].IsSupported = CBIOS_TRUE;
            pCEAVideoFormat[FormatNum - 1].IsNative = (IsNative == CBIOS_TRUE)? 1 : 0;
            ulNumOfSVDMode++;
        }
    }

    return ulNumOfSVDMode;
}

/***************************************************************
Function:    cbEDIDModule_GetSVDMode

Description: Get short audio descriptor in audio data block of CEA extension

Input:       pAudioFormatDataInEDID, short audio descriptor data buffer

Output:      pCEAAudioFormat, statistic audio format in short audio descriptor

Return:      the number of audio format get in short audio descriptor
***************************************************************/
static CBIOS_U32 cbEDIDModule_GetHDMIAudioFormat(CBIOS_U8 *pAudioFormatDataInEDID, PCBIOS_HDMI_AUDIO_INFO pCEAAudioFormat)
{
    CBIOS_U32   ulNumOfAudioFormat = 0;
    CBIOS_U32   PayloadLength = 0;
    CBIOS_U32   AudioFormatCode = 0;
    CBIOS_U32   MaxBitRateIndex = 0;
    CBIOS_U32   j = 0;

    //decode short video descriptor
    PayloadLength = pAudioFormatDataInEDID[0] & 0x1F;

    for (j = 0; j < PayloadLength/3; j++)
    {
        if(ulNumOfAudioFormat >= CBIOS_HDMI_AUDIO_FORMAT_COUNTS)
        {
            break;
        }

        AudioFormatCode = (pAudioFormatDataInEDID[1 + j * 3] >> 3) & 0xF;
        if ((AudioFormatCode > 0) && (AudioFormatCode < 16))
        {
            if (AudioFormatCode < 15)
            {
                pCEAAudioFormat[ulNumOfAudioFormat].Format = AudioFormatCode;
            }

            pCEAAudioFormat[ulNumOfAudioFormat].MaxChannelNum = (pAudioFormatDataInEDID[1 + j * 3] & 0x7) + 1;
            pCEAAudioFormat[ulNumOfAudioFormat].SampleRateUnit = pAudioFormatDataInEDID[2 + j * 3] & 0x7F;
            if (AudioFormatCode == 1)
            {
                pCEAAudioFormat[ulNumOfAudioFormat].BitDepth.BD_16bit = pAudioFormatDataInEDID[3 + j * 3] & 0x1;
                pCEAAudioFormat[ulNumOfAudioFormat].BitDepth.BD_20bit = (pAudioFormatDataInEDID[3 + j * 3] >> 1) & 0x1;
                pCEAAudioFormat[ulNumOfAudioFormat].BitDepth.BD_24bit = (pAudioFormatDataInEDID[3 + j * 3] >> 2) & 0x1;
            }
            else if (AudioFormatCode <= 8)
            {
                MaxBitRateIndex = pAudioFormatDataInEDID[3 + j * 3];
                pCEAAudioFormat[ulNumOfAudioFormat].MaxBitRate = MaxBitRateIndex * 8;
            }
            else if (AudioFormatCode <= 13)
            {
                pCEAAudioFormat[ulNumOfAudioFormat].AudioFormatDependValue = pAudioFormatDataInEDID[3 + j * 3];
            }
            else if (AudioFormatCode == 14)
            {
                pCEAAudioFormat[ulNumOfAudioFormat].Profile.Value = pAudioFormatDataInEDID[3 + j * 3] & 0x7;
            }
            else
            {
                if (((pAudioFormatDataInEDID[3 + j * 3] >> 3) & 0x1F) == 1)
                {
                    pCEAAudioFormat[ulNumOfAudioFormat].Format = CBIOS_AUDIO_FORMAT_HE_AAC;
                }
                else if(((pAudioFormatDataInEDID[3 + j * 3] >> 3) & 0x1F) == 2)
                {
                    pCEAAudioFormat[ulNumOfAudioFormat].Format = CBIOS_AUDIO_FORMAT_HE_AAC_V2;
                }
                else if(((pAudioFormatDataInEDID[3 + j * 3] >> 3) & 0x1F) == 3)
                {
                    pCEAAudioFormat[ulNumOfAudioFormat].Format = CBIOS_AUDIO_FORMAT_MPEG_SURROUND;
                }
            }

            ulNumOfAudioFormat++;
        }
    }

    return ulNumOfAudioFormat;
}

static CBIOS_VOID cbEDIDPatchHDMIAudio(PCBIOS_EDID_STRUCTURE_DATA pEDIDStruct, PCBIOS_HDMI_AUDIO_INFO pCEAAudioFormat)
{
    CBIOS_U32 i = 0;
    CBIOS_U32 AudioFormatNum;
    CBIOS_BOOL PcmSupport = CBIOS_FALSE;
    AudioFormatNum = pEDIDStruct->TotalHDMIAudioFormatNum;

    if(!pEDIDStruct->Attribute.IsCEA861Audio)
    {
        return;
    }
    for(i = 0; i < AudioFormatNum; i++)
    {
	if(pCEAAudioFormat[i].Format == CBIOS_AUDIO_FORMAT_LPCM)
        {
            PcmSupport = CBIOS_TRUE;
            break;
        }
    }
    //if all AudioFormat not support LPCM,but it indicates support basic audio,should patch here
    if(!PcmSupport)
    {
        if(i >= CBIOS_HDMI_AUDIO_FORMAT_COUNTS)
        {
            i = CBIOS_HDMI_AUDIO_FORMAT_COUNTS - 1;
        }
        pCEAAudioFormat[i].Format = CBIOS_AUDIO_FORMAT_LPCM;
        pCEAAudioFormat[i].MaxChannelNum = 2;//two channel
        pCEAAudioFormat[i].SampleRateUnit = 7;//sample rates of 32kHz,44.1kHz,48kHz
        pCEAAudioFormat[i].BitDepth.BD_16bit = 1;//16 bits
        if(i != CBIOS_HDMI_AUDIO_FORMAT_COUNTS -1 )
        {
            pEDIDStruct->TotalHDMIAudioFormatNum = pEDIDStruct->TotalHDMIAudioFormatNum + 1;
        }
    }
}

static CBIOS_VOID cbEDIDPatchCEAModes(CBIOS_U8 *pEDID, PCBIOS_EDID_STRUCTURE_DATA pEDIDStruct, PCBIOS_U32 pModeNumOfCEABlock)
{
    PCBIOS_MONITOR_MISC_ATTRIB pMonitorAttrib = &(pEDIDStruct->Attribute);
    PCBIOS_HDMI_FORMAT_DESCRIPTOR pCEAVideoFormat = pEDIDStruct->HDMIFormat;
    CBIOS_UCHAR                MonitorID[8] = {0};
    CBIOS_U32                  i = 0;

    if ((pMonitorAttrib == CBIOS_NULL) || (pCEAVideoFormat == CBIOS_NULL))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: pMonitorAttrib is null.\n", FUNCTION_NAME));
        return;
    }

    cbEDIDModule_GetMonitorID(pEDID, MonitorID);

    // patch for SKY 24X1Q monitor, filter 1920x1080@24Hz mode as it can't display
    if ((!cb_strcmp(MonitorID, (CBIOS_UCHAR*)"SKY2380")) && (!cb_strcmp(pMonitorAttrib->MonitorName, (CBIOS_UCHAR*)"24X1Q")))
    {
        pCEAVideoFormat[31].IsSupported = CBIOS_FALSE;
        *pModeNumOfCEABlock = *pModeNumOfCEABlock - 1;
        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "%s: filter 1080p@24Hz, ModeNumOfCEABlock = %d\n", FUNCTION_NAME, *pModeNumOfCEABlock));
    }

    // patch for BenQ EL2870U monitor, filter 3840x2160@29.97Hz mode in DTDTimings for no sound issue.
    if ((!cb_strcmp(MonitorID, (CBIOS_UCHAR*)"BNQ7949")) && (!cb_strcmp(pMonitorAttrib->MonitorName, (CBIOS_UCHAR*)"BenQ EL2870U")))
    {
        for (i = 0; i < CBIOS_DTDTIMING_BLOCK_CNT; i++)
        {
            if ((pEDIDStruct->DTDTimings[i].Valid) &&
                (pEDIDStruct->DTDTimings[i].XResolution == 3840) &&
                (pEDIDStruct->DTDTimings[i].YResolution == 2160) &&
                ((pEDIDStruct->DTDTimings[i].Refreshrate >= (3000 - 50)) &&
                (pEDIDStruct->DTDTimings[i].Refreshrate <= (3000 + 50))))
            {
                pEDIDStruct->DTDTimings[i].Valid = CBIOS_FALSE;
                *pModeNumOfCEABlock = *pModeNumOfCEABlock - 1;
                cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "%s: filter 3840x2160@29.97Hz, ModeNumOfCEABlock = %d\n", FUNCTION_NAME, *pModeNumOfCEABlock));
                break;
            }
        }
    }

}

/***************************************************************
Function:    cbEDIDModule_ParseHDMIVSDB

Description: Decode vendor specific data block of CEA extension

Input:       pVSDBDataInEDID, vendor specific data buffer

Output:      pVSDBData, decoded VSDB data

Return:      the total length of vendor specific data block
***************************************************************/

/*
                                                HDMI Vendor Specific Data Block
    -------------------------------------------------------------------------------------------------------------------------
    |Byte # |      7      |      6      |      5      |      4      |      3      |      2      |      1      |      0      |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |   0   |     Vendor-specific tag code (=3)       |                             Length (=N)                             |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |   1   |                                                                                                               |
    |-------|                         24-bit IEEE Registration Identifier (0x000C03)                                        |
    |   2   |                                 (least significant byte first)                                                |
    |-------|                                                                                                               |
    |   3   |                                                                                                               |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |   4   |                       A                               |           B                                           |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |   5   |                       C                               |           D                                           |
    |-------|---------------------------------------------------------------------------------------------------------------|-----------
    |       |   Supports  |     DC_     |    DC_      |    DC_      |     DC_      |    Rsvd     |     Rsvd   |    DVI_     |  extension
    |   6   |    _AI      |     48bit   |    36bit    |    30bit    |     Y444     |    (0)      |     (0)    |    Dual     |  fields
    |-------|---------------------------------------------------------------------------------------------------------------|
    |   7   |                                                Max_TMDS_Clock                                                 |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |       |    Latency_ |   I_Latency_|   HDMI_V    |   Rsvd      |              |             |            |             |
    |   8   |    Fields_  |   Fields_   |   ideo_pre  |   (0)       |     CNC3     |    CNC2     |     CNC1   |     CNC0    |
    |       |    Present  |   Present   |   sent      |             |              |             |            |             |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |  (9)  |                                         Video_Latency                                                         |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |  (10) |                                         Audio_Latency                                                         |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |  (11) |                                   Interlaced_Video_Latency                                                    |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |  (12) |                                   Interlaced_Audio_Latency                                                    |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |  (13) |    3D_pres  |      3D_Multi_present     |          Image_Size        |    Rsvd     |     Rsvd   |     Rsvd    |
    |       |      ent    |                           |                            |    (0)      |     (0)    |     (0)     |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |  (14) |              HDMI_VIC_LEN               |                               HDMI_3D_LEN                           |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |  (15) |(if HDMI_VIC_LEN > 0)                                                                                          |
    |       |                                                  HDMI_VIC_1                                                   |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |   ... |                                                     ...                                                       |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |       |                                                  HDMI_VIC_M                                                   |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |       |(if 3D_Multi_present = 01 or 10)                                                                               |
    |       |                                             3D_Structure_ALL_15...8                                           |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |       |                                             3D_Structure_ALL_7...0                                            |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |       |(if 3D_Multi_present = 10)                                                                                     |
    |       |                                                 3D_MASK_15...8                                                |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |       |                                                 3D_MASK_7...                                                  |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |       |                 2D_VIC_order_1                       |                       3D_Structure_1                   |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |       |                 3D_Detail_1 ***                      |                       Reserved(0) ***                  |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |       |                                                     ...                                                       |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |       |                 2D_VIC_order_L                       |                       3D_Structure_L                   |
    |-------|---------------------------------------------------------------------------------------------------------------|
    |()*...N|                                                 Reserved (0)**                                                |
    -------------------------------------------------------------------------------------------------------------------------------------
*/

static CBIOS_U32 cbEDIDModule_ParseHDMIVSDB(CBIOS_U8 *pVSDBDataInEDID, PCBIOS_HDMI_VSDB_EXTENTION pVSDBData)
{
    CBIOS_U8    *pCurByte = pVSDBDataInEDID;
    CBIOS_U32   i = 0;
    CBIOS_U32   PayloadLen = 0;
    CBIOS_U8    dataLow = 0;
    CBIOS_U8    dataHigh = 0;

    if ((pVSDBDataInEDID == CBIOS_NULL) || (pVSDBData == CBIOS_NULL))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbEDIDModule_ParseVSDB: NULL pointer!\n"));
        return 0;
    }

    //initialize CBIOS_HDMI_VSDB_EXTENTION
    cb_memset(pVSDBData, 0, sizeof(CBIOS_HDMI_VSDB_EXTENTION));
    pVSDBData->HDMI3DMask = 0xFFFF;

    //check tag and length
    cb_memcpy(&(pVSDBData->Tag), pCurByte++, sizeof(pVSDBData->Tag));

    if ((pVSDBData->Tag.VSDBTag != VENDOR_SPECIFIC_DATA_BLOCK_TAG)
        ||(pVSDBData->Tag.VSDBLength < 5))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbEDIDModule_ParseHDMIVSDB: invalid VSDB data!\n"));
        pVSDBData->Tag.VSDBLength = 0;
    }
    else//tag OK
    {
        PayloadLen = 0;
        //Byte 1-5, VSDB header
        pVSDBData->VSDBHeader.RegistrationIDByte0 = *(pCurByte++);
        pVSDBData->VSDBHeader.RegistrationIDByte1 = *(pCurByte++);
        pVSDBData->VSDBHeader.RegistrationIDByte2 = *(pCurByte++);
        dataHigh = *(pCurByte++);
        dataLow = *(pCurByte++);
        pVSDBData->VSDBHeader.SourcePhyAddr = (CBIOS_U16) (dataHigh << 8) | dataLow;

        PayloadLen += 5;
        if (PayloadLen >= pVSDBData->Tag.VSDBLength)
        {
            goto VSDB_DONE;
        }

        //byte 6
        cb_memcpy(&(pVSDBData->SupportCaps), pCurByte, sizeof(CBIOS_U8));
        pCurByte++;

        PayloadLen++;
        if (PayloadLen >= pVSDBData->Tag.VSDBLength)
        {
            goto VSDB_DONE;
        }

        //byte 7, max TMDS clock in MHz / 5
        pVSDBData->MaxTMDSClock = (CBIOS_U16)(*pCurByte) * 5;//max tmds clock in MHz
        pCurByte++;

        PayloadLen++;
        if (PayloadLen >= pVSDBData->Tag.VSDBLength)
        {
            goto VSDB_DONE;
        }

        //byte 8
        cb_memcpy(&(pVSDBData->PresentFlags), pCurByte, sizeof(CBIOS_U8));
        pCurByte++;

        PayloadLen++;
        if (PayloadLen >= pVSDBData->Tag.VSDBLength)
        {
            goto VSDB_DONE;
        }

        //These 2 bytes are present only if LatencyFieldsPresent is set to 1
        if (pVSDBData->LatencyFieldsPresent)
        {
            pVSDBData->VideoLatency = *(pCurByte++);
            pVSDBData->AudioLatency = *(pCurByte++);

            PayloadLen += 2;
            if (PayloadLen >= pVSDBData->Tag.VSDBLength)
            {
                goto VSDB_DONE;
            }
        }
        else
        {
            pVSDBData->VideoLatency = 0;
            pVSDBData->AudioLatency = 0;
        }

        //These 2 bytes are present only if ILatencyFieldsPresent is set to 1
        //In fact, if ILatencyFieldsPresent == 1, LatencyFieldsPresent must = 1
        if (pVSDBData->ILatencyFieldsPresent)
        {
            pVSDBData->InterlacedVideoLatency = *(pCurByte++);
            pVSDBData->InterlacedAudioLatency = *(pCurByte++);

            PayloadLen += 2;
            if (PayloadLen >= pVSDBData->Tag.VSDBLength)
            {
                goto VSDB_DONE;
            }
        }
        else
        {
            pVSDBData->InterlacedVideoLatency = 0;
            pVSDBData->InterlacedAudioLatency = 0;
        }

        // If HDMIVideoPresent =1 then additional video format capabilities are described by using the fields starting after the Latency area.
        if (pVSDBData->HDMIVideoPresent)
        {
            CBIOS_U32 HDMI3DFormatLength = 0;
            CBIOS_U32 HDMI3DFormatCount = 0;
            CBIOS_U8 ExtVICCnt = 0;


            cb_memcpy(&(pVSDBData->HDMI3DPresentFlags), pCurByte++, sizeof(pVSDBData->HDMI3DPresentFlags));

            PayloadLen++;
            if (PayloadLen >= pVSDBData->Tag.VSDBLength)
            {
                goto VSDB_DONE;
            }

            //HDMI_3D_LEN[5bits] indicates the total length of following 3D video format capabilities,
            //including 3D_Structure_ALL_15...0, 3D_MASK_15...0, 2D_VIC_order_X,3D_Structure_X and 3D_Detail_X fields.
            pVSDBData->HDMI3DLen = (*pCurByte) & 0x1F;

            //HDMI_VIC_LEN [3bits] indicates the total length of HDMI_VIC_X
            pVSDBData->HDMIVICLen = (*pCurByte >> 5) & 0x07;
            pCurByte++;

            PayloadLen++;
            if (PayloadLen >= pVSDBData->Tag.VSDBLength)
            {
                goto VSDB_DONE;
            }

            //HDMI_VIC_X
            for (i = 0; i < pVSDBData->HDMIVICLen; i++)
            {

                if ((*pCurByte >= 1) && (*pCurByte <= CBIOS_HDMI_EXTENED_VIC_COUNTS))
                {
                    pVSDBData->HDMIVIC[ExtVICCnt++] = *pCurByte;
                }

                pCurByte++;

            }
            pVSDBData->HDMIVICLen = ExtVICCnt;

            PayloadLen += i;
            if (PayloadLen >= pVSDBData->Tag.VSDBLength)
            {
                goto VSDB_DONE;
            }

            HDMI3DFormatLength = pVSDBData->HDMI3DLen;

            // If 3D_Multi_present = 01, 3D_Structure_ALL_15...0 is present
            // and assigns 3D formats to all of the VICs listed in the first 16 entries in the EDID.
            // 3D_MASK_15...0 is not present.
            if (pVSDBData->HDMI3DMultiPresent == 0x01)
            {
                // 2 Byte 3D_Structure_ALL_15...8 and 3D_Structure_ALL_7...0
                dataHigh = *(pCurByte++);
                dataLow = *(pCurByte++);

                pVSDBData->HDMI3DStructAll = (dataHigh << 8) | dataLow;

                // 3D_Multi_Present = 01, 3D_MASK_X is not present
                pVSDBData->HDMI3DMask = 0xFFFF;

                //subtract 3D_Structure_ALL_15...0 length from HDMI_3D_LEN
                HDMI3DFormatLength -= 2;

                PayloadLen += 2;
                if (PayloadLen >= pVSDBData->Tag.VSDBLength)
                {
                    goto VSDB_DONE;
                }
            }
            else if (pVSDBData->HDMI3DMultiPresent == 0x02)
            {
                // If 3D_Multi_present = 10,
                // 3D_Structure_ALL_15...0 and 3D_MASK_15...0 are present and assign 3D formats to
                // some of the VICs listed in the first 16 entries in the EDID.

                // 2 Byte 3D_Structure_ALL_15...8 and 3D_Structure_ALL_7...0
                dataHigh = *(pCurByte++);
                dataLow = *(pCurByte++);

                pVSDBData->HDMI3DStructAll = (dataHigh << 8) | dataLow;

                // 2 Byte 3D_MASK_15...8 and 3D_MASK_7...0
                dataHigh = *(pCurByte++);
                dataLow = *(pCurByte++);

                pVSDBData->HDMI3DMask = (dataHigh << 8) | dataLow;

                //subtract 3D_Structure_ALL_15...0 and 3D_MASK_15...0 length from HDMI_3D_LEN
                HDMI3DFormatLength -= 4;

                PayloadLen += 4;
                if (PayloadLen >= pVSDBData->Tag.VSDBLength)
                {
                    goto VSDB_DONE;
                }
            }
            else
            {
                // 3D_Multi_Present = 00 or 11, 3D_Structure_ALL_15...0 and 3D_MASK_15...0 are not present
                pVSDBData->HDMI3DStructAll = 0x0000;
                pVSDBData->HDMI3DMask = 0xFFFF;
            }

            if (pVSDBData->HDMI3DLen < HDMI3DFormatLength)
            {
                cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "cbEDIDModule_ParseHDMIVSDB: HDMI_3D_LEN error!\n"));
            }
            else
            {
                i = 0;
                HDMI3DFormatCount = 0;
                while (i < HDMI3DFormatLength)
                {
                    // 2D_VIC_order_X: a pointer to a particular VIC in the EDID based on the order in which the VICs are stored in the EDID.
                    pVSDBData->HDMI3DForamt[HDMI3DFormatCount].HDMI2DVICOrder = (*pCurByte >> 4) & 0x0F;
                    // 3D_Structure_X: This field indicates the 3D capability for the corresponding VIC code indicated by 2D_VIC_order_X.
                    pVSDBData->HDMI3DForamt[HDMI3DFormatCount].HDMI3DStructure = (*pCurByte) & 0x0F;
                    pCurByte++;
                    i++;

                    // 3D_Detail_X: This field indicates the 3D capability for the corresponding VIC code indicated by 2D_VIC_order_X.
                    // If If 3D_Structure_X is 0000~0111, this field shall not be present
                    // If 3D_Structure_X is 1000~1111 (including Side-by-Side (Half)), this field shall be present
                    if (pVSDBData->HDMI3DForamt[HDMI3DFormatCount].HDMI3DStructure >= 0x08)
                    {
                        pVSDBData->HDMI3DForamt[HDMI3DFormatCount].HDMI3DDetail = ((*pCurByte++) >> 4) & 0x0F;
                        i++;
                    }
                    HDMI3DFormatCount++;
                }
                pVSDBData->HDMI3DFormatCount = HDMI3DFormatCount;

                PayloadLen += HDMI3DFormatLength;
                if (PayloadLen >= pVSDBData->Tag.VSDBLength)
                {
                    goto VSDB_DONE;
                }
            }


        }

    }

VSDB_DONE:

    //check payload length
    if (PayloadLen != pVSDBData->Tag.VSDBLength)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "cbEDIDModule_ParseHDMIVSDB: payload length error!\n"));
    }

    //block total length = payload len + 1
    return (pVSDBData->Tag.VSDBLength + 1);

}

//HFVSDB

/***************************************************************
Function:    cbEDIDModule_ParseHFSCDS

Description: Decode HF-VSDB or HF-SCDB of CEA extension

Input:       pSCDSDataInEDID, HDMI Forum vendor specific data buffer, or Sink Capability data block buffer

Output:      pSCDSData, decoded Sink capability data structure

Return:      the total length of HDMI Forum vendor specific data block, or Sink capability data block
***************************************************************/

/*
                                            SCDS (Sink Capability Data Structure)
    |-----------|---------------------------------------------------------------------------------------------------------------|
    |    PB1    |                                                   Version (=1)                                                |
    |-----------|---------------------------------------------------------------------------------------------------------------|
    |    PB2    |                                              Max_TMDS_Character_Rate                                          |
    |-----------|---------------------------------------------------------------------------------------------------------------|
    |           |   SCDC_     |    RR_      |    Rsvd     |    Rsvd     |  LTE_340Mcsc| Independent |     Dual    |   3D_OSD_   |
    |    PB3    |   Present   |    Capable  |    (0)      |    (0)      |  _scramble  | _view       |     _View   |   Disparity |
    |-----------|---------------------------------------------------------------------------------------------------------------|
    |           |    Rsvd     |      Rsvd   |    Rsvd     |    Rsvd     |    Rsvd     |   DC_48Bit  |   DC_36Bit  |   DC_30Bit  |
    |    PB4    |    (0)      |      (0)    |    (0)      |    (0)      |    (0)      |   _420      |   _420      |   _420      |
    |-----------|---------------------------------------------------------------------------------------------------------------|
    |   ...N    |                                                   ...                                                         |
    |---------------------------------------------------------------------------------------------------------------------------|

** Sinks shall include only one SCDS, in HF-VSDB or HF-SCDB

                                        HDMI Forum Vendor Specific Data Block
    -----------------------------------------------------------------------------------------------------------------------------
    |Byte\Bit # |      7      |      6      |      5      |      4      |      3      |      2      |      1      |      0      |
    |-----------|---------------------------------------------------------------------------------------------------------------|
    |     0     |     Vendor-specific tag code (=3)       |                             Length (=N)                             |
    |-----------|---------------------------------------------------------------------------------------------------------------|
    |     1     |                                           IEEE OUI, Third Octet  (0xD8)                                       |
    |-----------|---------------------------------------------------------------------------------------------------------------|
    |     2     |                                           IEEE OUI, Second Octet (0x5D)                                       |
    |-----------|---------------------------------------------------------------------------------------------------------------|
    |     3     |                                           IEEE OUI, First Octet  (0xC4)                                       |
    |-----------|---------------------------------------------------------------------------------------------------------------|
    |   4..n    |                                                   ** SCDS                                                     |
    |-----------|---------------------------------------------------------------------------------------------------------------|


                                        HDMI Forum Sink Capability Data Block
    -----------------------------------------------------------------------------------------------------------------------------
    |Byte\Bit # |      7      |      6      |      5      |      4      |      3      |      2      |      1      |      0      |
    |-----------|---------------------------------------------------------------------------------------------------------------|
    |     0     |     Vendor-specific tag code (=7)       |                             Length (=N)                             |
    |-----------|---------------------------------------------------------------------------------------------------------------|
    |     1     |                                           Extended Tag Code (0x79)                                            |
    |-----------|---------------------------------------------------------------------------------------------------------------|
    |     2     |                                                   Reserved                                                    |
    |-----------|---------------------------------------------------------------------------------------------------------------|
    |     3     |                                                   Reserved                                                    |
    |-----------|---------------------------------------------------------------------------------------------------------------|
    |   4..n    |                                                   ** SCDS                                                     |
    |-----------|---------------------------------------------------------------------------------------------------------------|
*/

static CBIOS_U32 cbEDIDModule_ParseHFSCDS(CBIOS_U8 *pSCDSDataInEDID, PCBIOS_HF_SCDS_DATA pSCDSData)
{
    CBIOS_U8    *pCurByte = pSCDSDataInEDID;
    CBIOS_U32   PayloadLen = 0;

    if ((pSCDSDataInEDID == CBIOS_NULL) || (pSCDSData == CBIOS_NULL))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: NULL pointer!\n", FUNCTION_NAME));
        return 0;
    }

    cb_memset(pSCDSData, 0, sizeof(CBIOS_HF_SCDS_DATA));

    cb_memcpy(&(pSCDSData->Tag), pCurByte++, sizeof(pSCDSData->Tag));
    //byte 0 Tag and length
    if (pSCDSData->Tag.CTATag == VENDOR_SPECIFIC_DATA_BLOCK_TAG
        && pSCDSData->Tag.Length >= 7)
    {
        // HF-VSDB
        // byte 1-3, The IEEE Organizationally Unique Identifier (OUI) of C4-5D-D8
        pSCDSData->HFVSDBOUI.IEEEOUIByte0 = *(pCurByte++);
        pSCDSData->HFVSDBOUI.IEEEOUIByte1 = *(pCurByte++);
        pSCDSData->HFVSDBOUI.IEEEOUIByte2 = *(pCurByte++);
        PayloadLen += 3;
    }
    else if (pSCDSData->Tag.CTATag == CEA_EXTENDED_BLOCK_TAG
        && *pCurByte == HF_SINK_CAPABILITY_DATA_BLOCK
        && pSCDSData->Tag.Length >= 7)
    {
        // HF-SCDB
        // byte 1, Extended Tag Code
        pSCDSData->ExtTagCode = *(pCurByte++);
        // byte 2~3, reserved
        pCurByte += 2;
        PayloadLen += 3;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: no valid HF-VSDB or HF-SCDB data!\n", FUNCTION_NAME));
        pSCDSData->Tag.Length = 0;
    }

    if (pSCDSData->Tag.Length)  //tag ok
    {
        //byte 4  version
        pSCDSData->Version = *(pCurByte++);
        PayloadLen++;

        //byte 5  Max_TMDS_Character_Rate in MHz / 5
        pSCDSData->MaxTMDSCharacterRate = (CBIOS_U16)(*pCurByte) * 5;
        pCurByte++;
        PayloadLen++;

        //byte 6-7
        cb_memcpy(&(pSCDSData->SupportCaps), pCurByte, sizeof(CBIOS_U16));
        // Patch for Issue 21429, Issue21458: SCDCPresent is false that can't call cbHDMIMonitor_SCDC_Configure().
        if (pSCDSData->MaxTMDSCharacterRate > 340)
        {
            pSCDSData->IsSCDCPresent = CBIOS_TRUE;
        }
        pCurByte += 2;   
        PayloadLen += 2;
    }

    //check payload length
    if (PayloadLen != pSCDSData->Tag.Length)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "%s: payload length error !\n", FUNCTION_NAME));
    }

    //block total length = payload len + 1
    return (pSCDSData->Tag.Length + 1);
}


/***************************************************************
Function:    cbEDIDModule_ParseCEAExtBlock

Description: Parse extended data block of CEA extension

Input:       pExtBlockDataInEDID, extended block data buffer

Output:      pEDIDStruct, decoded extend block data

Return:      the total length of vendor specific data block
***************************************************************/
static CBIOS_U32 cbEDIDModule_ParseCEAExtBlock(CBIOS_U8 *pExtBlockDataInEDID, PCBIOS_EDID_STRUCTURE_DATA pEDIDStruct, CBIOS_U32 BlockIndex)
{
    CBIOS_U32   PayloadLen = 0;
    CBIOS_U8    ExtTagCode = 0;
    CBIOS_U8    FormatIndex = 0;
    PCBIOS_HDMI_FORMAT_DESCRIPTOR pCEAVideoFormat = CBIOS_NULL;
    PCBIOS_CEA_SVD_DATA pSVDData = CBIOS_NULL;
    PCBIOS_CEA_EXTENED_BLOCK pCEAExtData = CBIOS_NULL;
    CBIOS_U8    SVD = 0;
    CBIOS_BOOL  IsNative = CBIOS_FALSE;
    CBIOS_BOOL  Status = CBIOS_FALSE;
    CBIOS_U8    i = 0, j = 0;

    pCEAVideoFormat = pEDIDStruct->HDMIFormat;
    pSVDData = &pEDIDStruct->Attribute.SVDData[BlockIndex - 1];

    //check tag code
    if (((((*pExtBlockDataInEDID) >> 5) & 0x07) != CEA_EXTENDED_BLOCK_TAG)
        || (((*pExtBlockDataInEDID) & 0x1F) == 0))
    {
        PayloadLen = 0;
    }
    else
    {
        PayloadLen = (*pExtBlockDataInEDID) & 0x1F;
        ExtTagCode = pExtBlockDataInEDID[1];

        if(ExtTagCode == VIDEO_CAPABILITY_DATA_BLOCK_TAG)
        {
            //Video Capability Data Block
            pCEAExtData = &pEDIDStruct->Attribute.ExtDataBlock[VIDEO_CAPABILITY_DATA_BLOCK_TAG];
            cb_memcpy(&pCEAExtData->VideoCapabilityData, &pExtBlockDataInEDID[2], sizeof(CBIOS_VIDEO_CAPABILITY_DATA));
        }
        else if(ExtTagCode == VENDOR_SPECIFIC_VIDEO_DATA_BLOCK_TAG)
        {
            //Vendor Specific Video Data Block
        }
        else if(ExtTagCode == COLORIMETRY_DATA_BLOCK_TAG)
        {
            //Colorimetry Data Block
            pCEAExtData = &pEDIDStruct->Attribute.ExtDataBlock[COLORIMETRY_DATA_BLOCK_TAG];
            cb_memcpy(&pCEAExtData->ColorimetryData, &pExtBlockDataInEDID[2], sizeof(CBIOS_COLORIMETRY_DATA));
        }
        else if(ExtTagCode == HDR_STATIC_META_DATA_BLOCK)
        {

            cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: EDID HDR data\n", FUNCTION_NAME));

        }
        else if(ExtTagCode == VIDEO_FMT_PREFERENCE_DATA_BLOCK)
        {
            /*
                Video Format Preference Data Block
                VFPDB indicates the order of preference for(selected)Video Formats listed as DTDs and/or
                SVDs throughout the entire EDID. When present, the VFPDB shall take precedence over preferred
                indications defined elsewhere in CEA-861-F.
            */
            CBIOS_U8 SVR = 0;
            for (i = 0; i < PayloadLen - 1; i++)
            {
                SVR = pExtBlockDataInEDID[2 + i];
                if(((SVR >= 1) && (SVR <= 127)) || ((SVR >= 193) && (SVR <= 253)))
                {
                    //Interpret as a VIC
                    pEDIDStruct->Attribute.ShortVideoRef[i].SVRValue = SVR;
                    pEDIDStruct->Attribute.ShortVideoRef[i].SVRFlag = SVR_FLAG_VIC;
                }
                else if((SVR >= 129) && (SVR <= 144))
                {
                    //Interpret as the Kth DTD in the EDID, where K = SVR -128
                    pEDIDStruct->Attribute.ShortVideoRef[i].SVRValue = SVR - 128;
                    pEDIDStruct->Attribute.ShortVideoRef[i].SVRFlag = SVR_FLAG_DTD_INDEX;
                }
                else
                {
                    cbDebugPrint((MAKE_LEVEL(GENERIC,INFO), "Reserved SVR!\n"));
                }
            }
        }
        else if(ExtTagCode == YCBCR420_VIDEO_DATA_BLOCK)
        {
            /*
                According to CEA-861-F 7.5.10
                A YCBCR4:2:0 Video Data Block (Y420VDB) lists Video Formats, supported by the Sink, that only allow
                YCBCR4:2:0 sampling mode (i.e., do not support RGB, YCBCR4:4:4, or YCBCR4:2:2 sampling modes).
            */
            for (i = 0; i < PayloadLen - 1; i++)
            {
                SVD = pExtBlockDataInEDID[2 + i];
                Status = cbEDIDModule_GetFmtIdxFromSVD(SVD, &FormatIndex, &IsNative);
                if (!Status)
                {
                    continue;
                }

                pCEAVideoFormat[FormatIndex - 1].IsSupportYCbCr420 = 1;
                pCEAVideoFormat[FormatIndex - 1].IsSupportOtherFormats = 0;
            }
        }
        else if(ExtTagCode == YCBCR420_CAP_MAP_DATA_BLOCK)
        {
            /*
                According to CEA-861-F 7.5.11
                A YCBCR4:2:0 Capability Map Data Block (Y420CMDB) indicates exactly which SVDs, listed in one or
                more regular Video Data Blocks (see Section 7.5.1), also support YCBCR4:2:0 sampling mode - in
                addition to other modes such as RGB, YCBCR4:4:4, and/or YCBCR4:2:2. The Y420CMDB does not indicate which RGB,
                YCBCR 4:4:4, and/or YCBCR4:2:2 modes are supported.
            */

            CBIOS_U8 YCbCr420CapMap = 0;
            CBIOS_U8 Step = 0;

            if (PayloadLen == 1)
            {
                // payload length == 1, all regular SVDs supports YCbCr420
                for (i = 0; i < pSVDData->SVDCount; i++)
                {
                    SVD = pSVDData->SVD[i];
                    Status = cbEDIDModule_GetFmtIdxFromSVD(SVD, &FormatIndex, &IsNative);
                    if (!Status)
                    {
                        continue;
                    }

                    pCEAVideoFormat[FormatIndex - 1].IsSupportYCbCr420 = 1;
                    pCEAVideoFormat[FormatIndex - 1].IsSupportOtherFormats = 1;
                }
            }
            else
            {
                for (i = 0; i < PayloadLen - 1; i++)
                {
                    YCbCr420CapMap = pExtBlockDataInEDID[2 + i];

                    for (j = 0; j < 8; j++)
                    {
                        if (((YCbCr420CapMap >> j) & 0x1) && (j + Step < pSVDData->SVDCount))
                        {
                            SVD = pSVDData->SVD[j+Step];
                            Status = cbEDIDModule_GetFmtIdxFromSVD(SVD, &FormatIndex, &IsNative);
                            if (!Status)
                            {
                                continue;
                            }

                            pCEAVideoFormat[FormatIndex - 1].IsSupportYCbCr420 = 1;
                            pCEAVideoFormat[FormatIndex - 1].IsSupportOtherFormats = 1;
                        }
                    }
                    Step += 8;
                }
            }
        }
        else if(ExtTagCode == VENDOR_SPECIFIC_AUDIO_DATA_BLOCK_TAG)
        {
            //Vendor Specific Audio Data Block
        }
        else if(ExtTagCode == RSVD_HDMI_AUDIO_DATA_BLOCK_TAG)
        {
            //HDMI Audio Data Block
            pCEAExtData = &pEDIDStruct->Attribute.ExtDataBlock[RSVD_HDMI_AUDIO_DATA_BLOCK_TAG];

            pCEAExtData->HDMIAudioData.MaxStreamCount = pExtBlockDataInEDID[2] & 0x3;
            pCEAExtData->HDMIAudioData.SupportsMSNonMixed = (pExtBlockDataInEDID[2] >> 2) & 0x1;
            pCEAExtData->HDMIAudioData.NumHDMI3DAD = pExtBlockDataInEDID[3] & 0x7;

            if(pCEAExtData->HDMIAudioData.NumHDMI3DAD > 0)
            {
                CBIOS_U32   AudioFormatCode = 0;
                for(i = 0;i < pCEAExtData->HDMIAudioData.NumHDMI3DAD;i++)
                {
                    AudioFormatCode = pExtBlockDataInEDID[4 + i*4] & 0xF;
                    pCEAExtData->HDMIAudioData.HDMI3DAudioDesc[i].Format = AudioFormatCode;
                    pCEAExtData->HDMIAudioData.HDMI3DAudioDesc[i].MaxChannelNum = (pExtBlockDataInEDID[4 + i*4 + 1] & 0x1F) + 1;
                    pCEAExtData->HDMIAudioData.HDMI3DAudioDesc[i].SampleRateUnit = pExtBlockDataInEDID[4 + i*4 + 2] & 0x7F;

                    if (AudioFormatCode == 1)
                    {
                        pCEAExtData->HDMIAudioData.HDMI3DAudioDesc[i].BitDepth.BD_16bit = pExtBlockDataInEDID[4 + i*4 + 3] & 0x1;
                        pCEAExtData->HDMIAudioData.HDMI3DAudioDesc[i].BitDepth.BD_20bit = (pExtBlockDataInEDID[4 + i*4 + 3] >> 1) & 0x1;
                        pCEAExtData->HDMIAudioData.HDMI3DAudioDesc[i].BitDepth.BD_24bit = (pExtBlockDataInEDID[4 + i*4 + 3] >> 2) & 0x1;
                    }
                    else if (AudioFormatCode <= 8)
                    {
                        pCEAExtData->HDMIAudioData.HDMI3DAudioDesc[i].MaxBitRate = pExtBlockDataInEDID[4 + i*4 + 3] * 8;
                    }
                    else if (AudioFormatCode <= 13)
                    {
                        pCEAExtData->HDMIAudioData.HDMI3DAudioDesc[i].AudioFormatDependValue = pExtBlockDataInEDID[4 + i*4 + 3];
                    }
                    else if (AudioFormatCode == 14)
                    {
                        pCEAExtData->HDMIAudioData.HDMI3DAudioDesc[i].Profile.Value = pExtBlockDataInEDID[4 + i*4 + 3] & 0x7;
                    }
                    else
                    {
                        if (((pExtBlockDataInEDID[4 + i*4 + 3] >> 3) & 0x1F) == 1)
                        {
                            pCEAExtData->HDMIAudioData.HDMI3DAudioDesc[i].Format = CBIOS_AUDIO_FORMAT_HE_AAC;
                        }
                        else if(((pExtBlockDataInEDID[4 + i*4 + 3] >> 3) & 0x1F) == 2)
                        {
                            pCEAExtData->HDMIAudioData.HDMI3DAudioDesc[i].Format = CBIOS_AUDIO_FORMAT_HE_AAC_V2;
                        }
                        else if(((pExtBlockDataInEDID[4 + i*4 + 3] >> 3) & 0x1F) == 3)
                        {
                            pCEAExtData->HDMIAudioData.HDMI3DAudioDesc[i].Format = CBIOS_AUDIO_FORMAT_MPEG_SURROUND;
                        }
                    }
                }
            }
        }
        else if(ExtTagCode == INFOFRAME_DATA_BLOCK)
        {
            //InfoFrame Data Block
            CBIOS_U8 Index = 2;
            CBIOS_U8 TypeCode = 0;
            CBIOS_U8 SIDHeader = 0;
            CBIOS_U8 SupportPriority = 0;

            //InfoFrame Processing Descriptor
            CBIOS_U8 Len = (pExtBlockDataInEDID[Index++] >> 5) & 0x7;
            pEDIDStruct->Attribute.NumOfAdditionalVSIFs = pExtBlockDataInEDID[Index++];
            Index += Len;

            /*
             Declared InfoFrame types shall be listed in order of priority;
             meaning that the first is the one that the display manufacturer
             has identified as most desirable. Sources may use this infomation
             as a basis for selecting which InfoFrame to send(e.g. in case where
             the Sources may not be capable of delivering all defined or
             supported InfoFrame types).
            */

            for(;Index < PayloadLen;)
            {
                SIDHeader = pExtBlockDataInEDID[Index++];
                Len = (SIDHeader >> 5) & 0x7;
                TypeCode = SIDHeader & 0x1F;

                if(TypeCode == VENDOR_SPECIFIC_INFO_FRAME_TYPE)
                {
                    if((pExtBlockDataInEDID[Index + 1] == 0x03) &&
                       (pExtBlockDataInEDID[Index + 2] == 0x0c) &&
                       (pExtBlockDataInEDID[Index + 3] == 0x00))
                    {
                        pEDIDStruct->Attribute.InfoFrameSupCaps.HDMISpecificInfoFrameCaps.bSupport = CBIOS_TRUE;
                        pEDIDStruct->Attribute.InfoFrameSupCaps.HDMISpecificInfoFrameCaps.Priority = SupportPriority;
                    }
                    else if((pExtBlockDataInEDID[Index + 1] == 0xD8) &&
                       (pExtBlockDataInEDID[Index + 2] == 0x5D) &&
                       (pExtBlockDataInEDID[Index + 3] == 0xC4))
                    {
                        pEDIDStruct->Attribute.InfoFrameSupCaps.HFSpecificInfoFrameCaps.bSupport = CBIOS_TRUE;
                        pEDIDStruct->Attribute.InfoFrameSupCaps.HFSpecificInfoFrameCaps.Priority = SupportPriority;
                    }

                    Index += 3;//24-bit OUI Registration Identifier
                }
                else if(TypeCode == AVI_INFO_FRAME_TYPE)
                {
                    pEDIDStruct->Attribute.InfoFrameSupCaps.AVIInfoFrameCaps.bSupport = CBIOS_TRUE;
                    pEDIDStruct->Attribute.InfoFrameSupCaps.AVIInfoFrameCaps.Priority = SupportPriority;
                }
                else if(TypeCode == SOURCE_PRODUCT_DESCRIPTION_INFO_FRAME_TYPE)
                {
                    pEDIDStruct->Attribute.InfoFrameSupCaps.SourceProductDescInfoFrameCaps.bSupport = CBIOS_TRUE;
                    pEDIDStruct->Attribute.InfoFrameSupCaps.SourceProductDescInfoFrameCaps.Priority = SupportPriority;
                }
                else if(TypeCode == AUDIO_INFO_FRAME_TYPE)
                {
                    pEDIDStruct->Attribute.InfoFrameSupCaps.AudioInfoFrameCaps.bSupport = CBIOS_TRUE;
                    pEDIDStruct->Attribute.InfoFrameSupCaps.AudioInfoFrameCaps.Priority = SupportPriority;
                }
                else if(TypeCode == MPEG_SOURCE_INFO_FRAME_TYPE)
                {
                    pEDIDStruct->Attribute.InfoFrameSupCaps.MpegSourceInfoFrameCaps.bSupport = CBIOS_TRUE;
                    pEDIDStruct->Attribute.InfoFrameSupCaps.MpegSourceInfoFrameCaps.Priority = SupportPriority;
                }
                else if(TypeCode == NTSC_VBI_INFO_FRAME_TYPE)
                {
                    pEDIDStruct->Attribute.InfoFrameSupCaps.NTSCVBIInfoFrameCaps.bSupport = CBIOS_TRUE;
                    pEDIDStruct->Attribute.InfoFrameSupCaps.NTSCVBIInfoFrameCaps.Priority = SupportPriority;
                }

                SupportPriority++;
                Index += Len;
            }
        }
        else if(ExtTagCode == HF_EDID_EXTENSION_OVERRIDE_DATA_BLOCK)
        {
            // HF-EEODB for ext block num, already parsed by cbEDIDModule_GetExtBlockNum
            PayloadLen = (*pExtBlockDataInEDID) & 0x1F;
        }
        else if(ExtTagCode == HF_SINK_CAPABILITY_DATA_BLOCK)
        {
            PayloadLen = cbEDIDModule_ParseHFSCDS(pExtBlockDataInEDID, &(pEDIDStruct->Attribute.HFSCDSData)) - 1;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbEDIDModule_ParseCEAExtBlock: ExtTagCode = 0x%x which is not parsed yet!\n", ExtTagCode));
        }
    }

    //return total block len
    return PayloadLen + 1;
}

/***************************************************************
Function:    cbEDIDModule_GetHDMIVICMode

Description: Get HDMI VIC mode number in VSDB

Input:       pVSDBData, VSDB data buffer

Output:      pCEAVideoFormat, statistic HDMI VIC format in VSDB

Return:      the number of HDMI VIC mode
***************************************************************/
static CBIOS_U32 cbEDIDModule_GetHDMIVICMode(PCBIOS_HDMI_VSDB_EXTENTION pVSDBData, PCBIOS_HDMI_FORMAT_DESCRIPTOR pCEAVideoFormat)
{
    CBIOS_U32   ulNumOfHDMIVICMode = 0;
    CBIOS_U32   i = 0;

    //extended VICs
    for (i = 0; i < pVSDBData->HDMIVICLen; i++)
    {
        CBIOS_U16 FormatIndex = pVSDBData->HDMIVIC[i] + CBIOS_HDMI_NORMAL_VIC_COUNTS - 1;
        if ((FormatIndex < CBIOS_HDMIFORMATCOUNTS) && (!pCEAVideoFormat[FormatIndex].IsSupported))
        {
            pCEAVideoFormat[FormatIndex].IsSupported = CBIOS_TRUE;
            pCEAVideoFormat[FormatIndex].BlockIndex = (CBIOS_U8)0;
            pCEAVideoFormat[FormatIndex].IsNative = CBIOS_FALSE;
            pCEAVideoFormat[FormatIndex].RefreshIndex= CEAVideoFormatTable[FormatIndex].DefaultRefRateIndex;
            ulNumOfHDMIVICMode++;
        }
    }

    return ulNumOfHDMIVICMode;

}

/***************************************************************
Function:    cbEDIDModule_Get3DFormat

Description: Set some format 3D attribute if the monitor support 3D video

Input:       pSVDDataInEDID, short video descriptor data buffer
             pVSDBData, VSDB data buffer

Output:      pCEAVideoFormat, set some format 3D attribute through VSDB data

Return:      the number of 3D video mandatory formats that not in SVD
***************************************************************/
static CBIOS_U32 cbEDIDModule_Get3DFormat(CBIOS_U8 *pSVDDataInEDID, PCBIOS_HDMI_VSDB_EXTENTION pVSDBData, PCBIOS_HDMI_FORMAT_DESCRIPTOR pCEAVideoFormat)
{
    CBIOS_BOOL  b50HzSupport = CBIOS_FALSE, b60HzSupport = CBIOS_FALSE;
    CBIOS_U32   ulNumOf3DFormat = 0;
    CBIOS_U8    formatNum = 0;
    CBIOS_U32   SVDPayloadLength = 0;
    CBIOS_U32   i = 0;
    CBIOS_U8    SVDList[MAX_SVD_COUNT];
    CBIOS_BOOL  IsNative = CBIOS_FALSE;
    CBIOS_BOOL  Status = CBIOS_FALSE;

    SVDPayloadLength = pSVDDataInEDID[0] & 0x1F;
    cb_memset(SVDList, 0, MAX_SVD_COUNT);
    cb_memcpy(SVDList, pSVDDataInEDID+1, (sizeof(CBIOS_U8)*SVDPayloadLength));

    // first check if 59.94/60Hz or 50Hz 2D video format support
    for (i = 0; i < SVDPayloadLength; i++)
    {
        Status = cbEDIDModule_GetFmtIdxFromSVD(SVDList[i], &formatNum, &IsNative);
        if (!Status)
        {
            continue;
        }

        if ((formatNum >= 1) && (formatNum <= 16))
        {
            b60HzSupport = CBIOS_TRUE;
        }
        else if ((formatNum >= 17) && (formatNum <= 31))
        {
            b50HzSupport = CBIOS_TRUE;
        }
    }

    // Set 3D video mandatory formats
    if (b60HzSupport)
    {
        // An HDMI sink which supports at least one 59.94/60Hz 2D video format shall support all of below

        // format 32, 1920x1080p@23.98/24Hz
        if( !pCEAVideoFormat[31].IsSupported)
        {
            pCEAVideoFormat[31].IsSupported=CBIOS_TRUE;
            pCEAVideoFormat[31].BlockIndex = (CBIOS_U8)0;
            pCEAVideoFormat[31].IsNative = CBIOS_FALSE;
            pCEAVideoFormat[31].RefreshIndex= CEAVideoFormatTable[31].DefaultRefRateIndex;
            ulNumOf3DFormat++;
        }
        pCEAVideoFormat[31].Video3DSupportStructs.FramePacking = 1;
        pCEAVideoFormat[31].Video3DSupportStructs.TopAndBottom = 1;

        // format 5, 1920x1080i@59.94/60Hz
        if( !pCEAVideoFormat[4].IsSupported)
        {
            pCEAVideoFormat[4].IsSupported=CBIOS_TRUE;
            pCEAVideoFormat[4].BlockIndex = (CBIOS_U8)0;
            pCEAVideoFormat[4].IsNative = CBIOS_FALSE;
            pCEAVideoFormat[4].RefreshIndex= CEAVideoFormatTable[4].DefaultRefRateIndex;
            ulNumOf3DFormat++;
        }
        pCEAVideoFormat[4].Video3DSupportStructs.SideBySideHalf = 1;

        //format 4, 1280x720p@59.94/60Hz
        if( !pCEAVideoFormat[3].IsSupported)
        {
            pCEAVideoFormat[3].IsSupported=CBIOS_TRUE;
            pCEAVideoFormat[3].BlockIndex = (CBIOS_U8)0;
            pCEAVideoFormat[3].IsNative = CBIOS_FALSE;
            pCEAVideoFormat[3].RefreshIndex= CEAVideoFormatTable[3].DefaultRefRateIndex;
            ulNumOf3DFormat++;
        }
        pCEAVideoFormat[3].Video3DSupportStructs.FramePacking = 1;
        pCEAVideoFormat[3].Video3DSupportStructs.TopAndBottom = 1;

    }

    if (b50HzSupport)
    {
        //format 32, 1920x1080p@23.98/24Hz
        if( !pCEAVideoFormat[31].IsSupported)
        {
            pCEAVideoFormat[31].IsSupported=CBIOS_TRUE;
            pCEAVideoFormat[31].BlockIndex = (CBIOS_U8)0;
            pCEAVideoFormat[31].IsNative = CBIOS_FALSE;
            pCEAVideoFormat[31].RefreshIndex= CEAVideoFormatTable[31].DefaultRefRateIndex;
            ulNumOf3DFormat++;
        }
        pCEAVideoFormat[31].Video3DSupportStructs.FramePacking = 1;
        pCEAVideoFormat[31].Video3DSupportStructs.TopAndBottom = 1;

        // format 20, 1920x1080i@50Hz
        if( !pCEAVideoFormat[19].IsSupported)
        {
            pCEAVideoFormat[19].IsSupported=CBIOS_TRUE;
            pCEAVideoFormat[19].BlockIndex = (CBIOS_U8)0;
            pCEAVideoFormat[19].IsNative = CBIOS_FALSE;
            pCEAVideoFormat[19].RefreshIndex= CEAVideoFormatTable[19].DefaultRefRateIndex;
            ulNumOf3DFormat++;
        }
        pCEAVideoFormat[19].Video3DSupportStructs.SideBySideHalf = 1;

        //format 19, 1280x720p@50Hz
        if( !pCEAVideoFormat[18].IsSupported)
        {
            pCEAVideoFormat[18].IsSupported=CBIOS_TRUE;
            pCEAVideoFormat[18].BlockIndex = (CBIOS_U8)0;
            pCEAVideoFormat[18].IsNative = CBIOS_FALSE;
            pCEAVideoFormat[18].RefreshIndex= CEAVideoFormatTable[18].DefaultRefRateIndex;
            ulNumOf3DFormat++;
        }
        pCEAVideoFormat[18].Video3DSupportStructs.FramePacking = 1;
        pCEAVideoFormat[18].Video3DSupportStructs.TopAndBottom = 1;
    }

    //Additional 3D formats
    //check 3D_structure_all_X
    for (i = 0; (i < SVDPayloadLength) && (i < 16); i++)
    {
        Status = cbEDIDModule_GetFmtIdxFromSVD(SVDList[i], &formatNum, &IsNative);
        if (!Status)
        {
            continue;
        }

        if (pVSDBData->HDMI3DMask & (1 << i))
        {
            pCEAVideoFormat[formatNum - 1].Video3DSupportCaps |= pVSDBData->HDMI3DStructAll;
        }
    }

    //check 3D structure
    for (i = 0; i < pVSDBData->HDMI3DFormatCount; i++)
    {
        Status = cbEDIDModule_GetFmtIdxFromSVD(SVDList[pVSDBData->HDMI3DForamt[i].HDMI2DVICOrder], &formatNum, &IsNative);
        if (!Status)
        {
            continue;
        }

        pCEAVideoFormat[formatNum - 1].Video3DSupportCaps |= 1 << pVSDBData->HDMI3DForamt[i].HDMI3DStructure;
    }

    return ulNumOf3DFormat;
}

static CBIOS_U32 cbEDIDModule_GetDisplayIDType1DetailedMode(CBIOS_U8 *pType1TimingInEDID, PCBIOS_MODE_INFO_EXT pDisplayIDDtlMode)
{
    CBIOS_U32   i = 0;
    CBIOS_U32   PayloadLen = 0;
    CBIOS_U32   ulNumOfModes = 0;

    PayloadLen = pType1TimingInEDID[2];

    for (i = 0; i < PayloadLen/DID_TYPE1_TIMING_DESCRIPTOR_LENGTH; i++)
    {
        if(ulNumOfModes >= CBIOS_DISPLAYID_TYPE1_MODECOUNT)
        {
            break;
        }

        pDisplayIDDtlMode[ulNumOfModes].PixelClock = (((pType1TimingInEDID[5 + i*20] << 16) | (pType1TimingInEDID[4 + i*20] << 8) | (pType1TimingInEDID[3 + i*20])) * 100);
        if(pDisplayIDDtlMode[ulNumOfModes].PixelClock == 0)
        {
            pDisplayIDDtlMode[ulNumOfModes].Valid = 0;
            continue;
        }
        else
        {
            pDisplayIDDtlMode[ulNumOfModes].Valid = 1;
            pDisplayIDDtlMode[ulNumOfModes].HActive = (((pType1TimingInEDID[8 + i*20] << 8) | (pType1TimingInEDID[7 + i*20])) + 1);
            pDisplayIDDtlMode[ulNumOfModes].HBlank = (((pType1TimingInEDID[10 + i*20] << 8) | (pType1TimingInEDID[9 + i*20])) + 1);
            pDisplayIDDtlMode[ulNumOfModes].HSyncOffset= ((((pType1TimingInEDID[12 + i*20] & 0x7F) << 8) | (pType1TimingInEDID[11 + i*20])) + 1);
            pDisplayIDDtlMode[ulNumOfModes].HSyncPulseWidth = (((pType1TimingInEDID[14 + i*20] << 8) | (pType1TimingInEDID[13 + i*20])) + 1);
            pDisplayIDDtlMode[ulNumOfModes].HSync = (pType1TimingInEDID[12 + i*20] & 0x80) ? HorPOSITIVE : HorNEGATIVE;
            pDisplayIDDtlMode[ulNumOfModes].VActive = (((pType1TimingInEDID[16 + i*20] << 8) | (pType1TimingInEDID[15 + i*20])) + 1);
            pDisplayIDDtlMode[ulNumOfModes].VBlank = (((pType1TimingInEDID[18 + i*20] << 8) | (pType1TimingInEDID[17 + i*20])) + 1);
            pDisplayIDDtlMode[ulNumOfModes].VSyncOffset = ((((pType1TimingInEDID[20 + i*20] & 0x7F) << 8) | (pType1TimingInEDID[19 + i*20])) + 1);
            pDisplayIDDtlMode[ulNumOfModes].VSyncPulseWidth = (((pType1TimingInEDID[22 + i*20] << 8) | (pType1TimingInEDID[21 + i*20])) + 1);
            pDisplayIDDtlMode[ulNumOfModes].VSync = (pType1TimingInEDID[20 + i*20] & 0x80) ? VerPOSITIVE : VerNEGATIVE;
            pDisplayIDDtlMode[ulNumOfModes].InterLaced = (pType1TimingInEDID[6 + i*20] & 0x10) ? CBIOS_TRUE : CBIOS_FALSE;

            if((pDisplayIDDtlMode[ulNumOfModes].HActive == 0 )|| (pDisplayIDDtlMode[ulNumOfModes].VActive == 0))
            {
                pDisplayIDDtlMode[ulNumOfModes].Valid = 0;
                continue;
            }

            pDisplayIDDtlMode[ulNumOfModes].Refreshrate = cbCalcRefreshRate(pDisplayIDDtlMode[ulNumOfModes].PixelClock,
                                                          pDisplayIDDtlMode[ulNumOfModes].HActive,
                                                          pDisplayIDDtlMode[ulNumOfModes].HBlank,
                                                          pDisplayIDDtlMode[ulNumOfModes].VActive,
                                                          pDisplayIDDtlMode[ulNumOfModes].VBlank);
            pDisplayIDDtlMode[ulNumOfModes].Refreshrate = 100 * cbRound(pDisplayIDDtlMode[ulNumOfModes].Refreshrate, 100, ROUND_NEAREST);

            ulNumOfModes++;
        }
    }
    return ulNumOfModes;
}

CBIOS_U32 cbEDIDModule_GetExtBlockNum(CBIOS_U8 *pEDID)
{
    CBIOS_U32 ExtBlockNum = 0;

    if (!pEDID)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid EDID\n", FUNCTION_NAME));
        return 0;
    }

    if (!pEDID[0x7E])  // Extension Flag
    {
        ExtBlockNum = 0;
    }
    else
    {
        if ((pEDID[0x84] >> 5 == CEA_EXTENDED_BLOCK_TAG) &&
            (pEDID[0x85] == HF_EDID_EXTENSION_OVERRIDE_DATA_BLOCK))  // be HF-EEODB
        {
            ExtBlockNum = pEDID[0x86];
        }
        else
        {
            ExtBlockNum = pEDID[0x7E];
        }
    }

    if(ExtBlockNum > (CBIOS_EDIDMAXBLOCKCOUNT - 1))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "%s: block num > %d, need refine!\n", FUNCTION_NAME, CBIOS_EDIDMAXBLOCKCOUNT));
        ExtBlockNum = CBIOS_EDIDMAXBLOCKCOUNT - 1;
    }

    return ExtBlockNum;
}

/***************************************************************
Function:    cbEDIDModule_GetCEADetailedMode

Description:

Input:       pEDID, EDID data buffer

Output:      pEDIDStruct, store decoded CEA data

Return:      the number of mode get in CEA extension
***************************************************************/
static CBIOS_U32 cbEDIDModule_GetCEA861Mode(CBIOS_U8 *pEDID, PCBIOS_EDID_STRUCTURE_DATA pEDIDStruct, CBIOS_U32 TotalBlocks)
{
    CBIOS_U32   BlockIndex = 0;
    CBIOS_U8    *pEDIDBlock = CBIOS_NULL;
    CBIOS_U8    DetailedTimingOffset = 0, SVDDataOffset = 0;
    CBIOS_U8    AudioFormatDataOffset = 0;
    CBIOS_U32   PayloadLength = 0;
    CBIOS_U32   i = 0;
    CBIOS_U32   ulModeNumOfCEABlock = 0;

    if (TotalBlocks > MAX_EDID_BLOCK_NUM)
    {
        // TBD: support for more than 8 blocks
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbEDIDModule_GetCEA861Mode: Total %d blocks but currently only parse 8 blocks!\n", TotalBlocks));
        //ASSERT(CBIOS_FALSE);

        TotalBlocks = MAX_EDID_BLOCK_NUM;
    }

    //parse extension blocks
    for (BlockIndex = 1; BlockIndex < TotalBlocks; BlockIndex++)
    {
        pEDIDBlock = pEDID + BlockIndex * 128;

        //check CEA Tag
        if (pEDIDBlock[0x00] != CEA_TAG)
        {
            continue;
        }

        // Check CEA861 Version.
        if (pEDIDBlock[0x01] == 0x00)
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "cbEDIDModule_GetCEA861Mode: EDID block%d indicates invalid CEA861 block version!\n", BlockIndex));
        }

        DetailedTimingOffset = pEDIDBlock[0x02];
        if (DetailedTimingOffset == 0)
        {
            //no detailed timing block, no reserved data block
            continue;
        }

        //parse data blocks
        for (i = 4; i < DetailedTimingOffset;)
        {
            if (((pEDIDBlock[i] >> 5) & 0x07) == AUDIO_DATA_BLOCK_TAG)
            {
                //audio data block
                AudioFormatDataOffset = (CBIOS_U8)i;
                PayloadLength = pEDIDBlock[i++] & 0x1F;
                pEDIDStruct->TotalHDMIAudioFormatNum = cbEDIDModule_GetHDMIAudioFormat(&pEDIDBlock[AudioFormatDataOffset], pEDIDStruct->HDMIAudioFormat);
                i += PayloadLength;
            }
            else if (((pEDIDBlock[i] >> 5) & 0x07) == VIDEO_DATA_BLOCK_TAG)
            {
                //decode short video descriptor
                SVDDataOffset = (CBIOS_U8)i;
                PayloadLength = pEDIDBlock[i++] & 0x1F;
                ulModeNumOfCEABlock += cbEDIDModule_GetSVDMode(&pEDIDBlock[SVDDataOffset], pEDIDStruct, BlockIndex);
                i += PayloadLength;
            }
            else if ((((pEDIDBlock[i] >> 5) & 0x07) == VENDOR_SPECIFIC_DATA_BLOCK_TAG))//now consider HDMI VSDB and HDMI Forum VSDB only, ignore other VSDBs
            {
                if((pEDIDBlock[i + 1] == 0x03) &&
                   (pEDIDBlock[i + 2] == 0x0c) &&
                   (pEDIDBlock[i + 3] == 0x00))
                {
                    i += cbEDIDModule_ParseHDMIVSDB(&pEDIDBlock[i], &(pEDIDStruct->Attribute.VSDBData));
                }
                else if((pEDIDBlock[i + 1] == 0xD8) &&
                        (pEDIDBlock[i + 2] == 0x5D) &&
                        (pEDIDBlock[i + 3] == 0xC4))
                {
                    i += cbEDIDModule_ParseHFSCDS(&pEDIDBlock[i], &(pEDIDStruct->Attribute.HFSCDSData));
                }
                else
                {
                    PayloadLength = pEDIDBlock[i++] & 0x1F;
                    i += PayloadLength;
                }

            }
            else if (((pEDIDBlock[i] >> 5) & 0x07) == SPEAKER_ALLOCATION_DATA_BLOCK_TAG)
            {
                //This payload is preceded by a Tag Code Byte that includes a tag equal to 4 and a length of 3
                i += 4;
            }
            else if (((pEDIDBlock[i] >> 5) & 0x07) == CEA_EXTENDED_BLOCK_TAG)
            {
                i += cbEDIDModule_ParseCEAExtBlock(&pEDIDBlock[i], pEDIDStruct, BlockIndex);
            }
            else
            {
                PayloadLength = pEDIDBlock[i++] & 0x1F;
                i += PayloadLength;
            }
        }
    }
    //some monitor's AUDIO_DATA_BLOCK not support LPCM,but it indicates support basic audio, should patch here
    cbEDIDPatchHDMIAudio(pEDIDStruct,&pEDIDStruct->HDMIAudioFormat[0]);

    // get the detailed timing in CEA extension
    ulModeNumOfCEABlock += cbEDIDModule_GetCEADetailedMode(pEDID, pEDIDStruct->DTDTimings, TotalBlocks);

    // get the 3D video mandatory formats
    if (pEDIDStruct->Attribute.VSDBData.HDMI3DPresent)
    {
        ulModeNumOfCEABlock += cbEDIDModule_Get3DFormat(&pEDIDBlock[SVDDataOffset],
                                                        &(pEDIDStruct->Attribute.VSDBData),
                                                        pEDIDStruct->HDMIFormat);
    }

    // get HDMI VIC mode
    ulModeNumOfCEABlock += cbEDIDModule_GetHDMIVICMode(&(pEDIDStruct->Attribute.VSDBData),
                                                       pEDIDStruct->HDMIFormat);

    //check if modes support YCbCr420 but not listed in svd exist. if so, add it
    for(i=0; i<CBIOS_HDMIFORMATCOUNTS; i++)
    {
        if(pEDIDStruct->HDMIFormat[i].IsSupportYCbCr420 && (!pEDIDStruct->HDMIFormat[i].IsSupported))
        {
            pEDIDStruct->HDMIFormat[i].IsSupported=CBIOS_TRUE;
            pEDIDStruct->HDMIFormat[i].BlockIndex = (CBIOS_U8)0;
            pEDIDStruct->HDMIFormat[i].IsNative = CBIOS_FALSE;
            pEDIDStruct->HDMIFormat[i].RefreshIndex= CEAVideoFormatTable[i].DefaultRefRateIndex;
            ulModeNumOfCEABlock++;
        }
    }

    //patch for some monitor can't display some CEA modes.
    cbEDIDPatchCEAModes(pEDID, pEDIDStruct, &ulModeNumOfCEABlock);

    return ulModeNumOfCEABlock;

}

static CBIOS_U32 cbEDIDModule_GetDisplayIDMode(CBIOS_U8 *pEDID, PCBIOS_EDID_STRUCTURE_DATA pEDIDStruct)
{
    CBIOS_U32   TotalBlocks = 0, BlockIndex = 0;
    CBIOS_U8    *pEDIDBlock = CBIOS_NULL;
    CBIOS_U32   PayloadLength = 0;
    CBIOS_U32   i = 0;
    CBIOS_U32   ulModeNumOfDisplayIDBlock = 0;

    TotalBlocks =  1 + cbEDIDModule_GetExtBlockNum(pEDID); // Ext. blocks plus base block.
    if (TotalBlocks > MAX_EDID_BLOCK_NUM)
    {
        // TBD: support for more than 8 blocks
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbEDIDModule_GetDisplayIDMode: Total %d blocks but currently only parse 4 blocks!\n", TotalBlocks));
        //ASSERT(CBIOS_FALSE);

        TotalBlocks = MAX_EDID_BLOCK_NUM;
    }

    //parse extension blocks
    for (BlockIndex = 1; BlockIndex < TotalBlocks; BlockIndex++)
    {
        pEDIDBlock = pEDID + BlockIndex * 128;
        //check DisplayID Tag
        if (pEDIDBlock[0x00] != DISPLAYID_TAG)
        {
            continue;
        }
        // check Section Size
        if(pEDIDBlock[2] <= 0)
        {
            continue;
        }

        for(i = 5; i < 128;)
        {
            if(pEDIDBlock[i] == VIDEO_TIMING_MODES_DATA_BLOCK_TYPE1_TAG)
            {
                PayloadLength = pEDIDBlock[i+2];
                ulModeNumOfDisplayIDBlock += cbEDIDModule_GetDisplayIDType1DetailedMode(&pEDIDBlock[i],pEDIDStruct->DisplayID_TYPE1_Timings);
                i = i + 3 + PayloadLength;
            }
            else
            {
                //TODO for other block
                PayloadLength = pEDIDBlock[i+2];
                i = i + 3 + PayloadLength;
            }
        }
    }
    return ulModeNumOfDisplayIDBlock;
}

/***************************************************************
Function:    cbEDIDModule_SetNativeFlag

Description: Set native mode flag of detailed timings in base and CEA extension EDID

Input:       PCBIOS_EDID_STRUCTURE_DATA, parsed EDID structure data buffer

Output:

Return:
***************************************************************/
static CBIOS_VOID cbEDIDModule_SetNativeFlag(PCBIOS_EDID_STRUCTURE_DATA pEDIDStruct)
{
    CBIOS_U32 NativeModeNum = 0;
    CBIOS_U32 i;

    NativeModeNum = pEDIDStruct->Attribute.TotalNumberOfNativeFormat;

    for (i = 0; i < CBIOS_DTLMODECOUNT; i++)
    {
        if ((NativeModeNum > 0) && (pEDIDStruct->DtlTimings[i].Valid))
        {
            pEDIDStruct->DtlTimings[i].IsNativeMode = CBIOS_TRUE;
            NativeModeNum--;

            if (NativeModeNum <= 0)
            {
                break;
            }
        }
    }

    if (NativeModeNum > 0)
    {
        for (i = 0; i < CBIOS_DTDTIMINGCOUNTS; i++)
        {
            if ((NativeModeNum > 0) && (pEDIDStruct->DTDTimings[i].Valid))
            {
                pEDIDStruct->DTDTimings[i].IsNativeMode = CBIOS_TRUE;
                NativeModeNum--;

                if(NativeModeNum <= 0)
                {
                    break;
                }
            }
        }
    }

    return;
}

/***************************************************************
Function:    cbEDIDModule_GetMonitorSize

Description: Get monitor image size.

Input:       pEDID, EDID data buffer

Output:

Return:      Monitor DPI
***************************************************************/
static CBIOS_VOID cbEDIDModule_GetMonitorSize(CBIOS_U8 *pEDID, PCBIOS_MONITOR_MISC_ATTRIB pMonitorAttrib)
{
    CBIOS_U16   HImageSize = 0, VImageSize = 0;
    CBIOS_U8   *pDtlTimingsInBaseEDID = pEDID + DETAILED_TIMINGS_INDEX;

    HImageSize  = (CBIOS_U16)cbEDIDModule_MapMaskGetEdidInfo(pDtlTimingsInBaseEDID,0,EDIDHorizontalImageSize);
    VImageSize  = (CBIOS_U16)cbEDIDModule_MapMaskGetEdidInfo(pDtlTimingsInBaseEDID,0,EDIDVerticalImageSize);
    if ((HImageSize != 0) && (VImageSize != 0))
    {
        pMonitorAttrib->MonitorHorSize = HImageSize;    // monitor screen image horizontal size in millimeter(mm)
        pMonitorAttrib->MonitorVerSize = VImageSize;    // monitor screen image vertical size in millimeter(mm)
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbEDIDModule_GetMonitorDPI: error -- monitor image size is zero!\n"));
    }

    return;

}

/***************************************************************
Function:    cbEDIDModule_GetMonitorAttrib

Description: Get monitor attribute

Input:       pEDID, EDID data buffer

Output:      pMonitorAttrib, store decoded monitor attributes

Return:      CEA861 MonitorCaps
***************************************************************/
CBIOS_U32 cbEDIDModule_GetMonitorAttrib(CBIOS_U8 *pEDID, PCBIOS_MONITOR_MISC_ATTRIB pMonitorAttrib, CBIOS_U32 TotalBlocks)
{
    CBIOS_BOOL  bIsHDMIDevice = CBIOS_FALSE;
    CBIOS_U32   BlockIndex = 0;
    CBIOS_U8    *pEDIDBlock = CBIOS_NULL;
    CBIOS_U32   i = 0, j = 0;
    CBIOS_U8    *pDtlTimingsInBaseEDID = pEDID + DETAILED_TIMINGS_INDEX;
    CBIOS_U8    SADCnt = 0;

    if(!cbEDIDModule_IsEDIDValid(pEDID))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: Not valid EDID!\n", FUNCTION_NAME));
        return CBIOS_ER_EDID_INVALID;
    }

    // Manufacture name, EDID base block offset 0x08~0x09
    cb_memcpy(pMonitorAttrib->ManufactureName, &pEDID[0x08], 0x02);
    // ProductCode, EDID base block offset 0x0a~0x0b
    cb_memcpy(pMonitorAttrib->ProductCode, &pEDID[0x0A], 0x02);

    // scan base EDID
    for (i = 0; i < 4; i++)
    {
        // detail timing
        if ((pDtlTimingsInBaseEDID[i*18] != 0x00) &&
            (pDtlTimingsInBaseEDID[i*18+0x1] != 0x00))
        {
            CBIOS_U8 byte17 = 0;
            byte17 = (pDtlTimingsInBaseEDID[i*18+0x11]&0x61);

            if(byte17)
            {
                pMonitorAttrib->bStereoViewSupport = CBIOS_TRUE;
                switch(byte17)
                {
                    case 0x20:
                        pMonitorAttrib->StereoViewType = FIELD_SEQ_RIGHT;
                        break;
                    case 0x40:
                        pMonitorAttrib->StereoViewType = FIELD_SEQ_LEFT;
                        break;
                    case 0x21:
                        pMonitorAttrib->StereoViewType = TWO_WAY_RIGHT;
                        break;
                    case 0x41:
                        pMonitorAttrib->StereoViewType = TWO_WAY_LEFT;
                        break;
                    case 0x60:
                        pMonitorAttrib->StereoViewType = FOUR_WAY;
                        break;
                    case 0x61:
                        pMonitorAttrib->StereoViewType = SIDE_BY_SIDE_INTERLEAVE;
                        break;
                    default:
                        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "cbEDIDModule_GetMonitorAttrib: No matched StereoViewType, byte17 is %d!\n", byte17));
                        break;
                }
            }
        }


        //  the monitor descriptor block
        if((pDtlTimingsInBaseEDID[i*18] == 0x00) &&
           (pDtlTimingsInBaseEDID[i*18+0x1] == 0x00) &&
           (pDtlTimingsInBaseEDID[i*18+0x2] == 0x00) &&
           (pDtlTimingsInBaseEDID[i*18+0x3] == 0xFC) &&
           (pDtlTimingsInBaseEDID[i*18+0x4] == 0x00))
        {
            for (j = 0; j < 13; j++)
            {
                if (pDtlTimingsInBaseEDID[i*18+5+j] == 0x0A)   // end signature of monitor name
                {
                    break;
                    }

                pMonitorAttrib->MonitorName[j] = pDtlTimingsInBaseEDID[i*18+5+j];
            }
        }

    }

    // get monitor screen image horizontal and vertical size in base EDID detailed timing
    cbEDIDModule_GetMonitorSize(pEDID, pMonitorAttrib);

    pMonitorAttrib->CEA861MonitorCaps = 0;

    // check EDID version 1.4, since majority of DP monitors don't have extended EDID block
    if ((pEDID[0x12] == 0x01) && (pEDID[0x13] == 0x04))
    {
        if (pEDID[0x14] & 0x80)
        {
            pMonitorAttrib->IsCEA861RGB = 1;
            if ((pEDID[0x18] & 0x18) == 0x08)
            {
                pMonitorAttrib->IsCEA861YCbCr444 = 1;
            }
            else if ((pEDID[0x18] & 0x18) == 0x10)
            {
                pMonitorAttrib->IsCEA861YCbCr422 = 1;
            }
            else if ((pEDID[0x18] & 0x18) == 0x18)
            {
                pMonitorAttrib->IsCEA861YCbCr444 = 1;
                pMonitorAttrib->IsCEA861YCbCr422 = 1;
            }
        }
    }

    if (TotalBlocks > MAX_EDID_BLOCK_NUM)
    {
        // TBD: support for more than 8 blocks
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbEDIDModule_GetCEAMonitorCaps: Total %d blocks but currently only parse 8 blocks!\n", TotalBlocks));
        //ASSERT(CBIOS_FALSE);

        TotalBlocks = MAX_EDID_BLOCK_NUM;
    }

    for (BlockIndex = 1; BlockIndex < TotalBlocks; BlockIndex++)
    {
        pEDIDBlock = pEDID + BlockIndex * 128;

        //check CEA Tag
        if (pEDIDBlock[0x00] != CEA_TAG)
        {
            continue;
        }

        // Check CEA861 Version.
        if (pEDIDBlock[0x01] == 0x00)
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "cbEDIDModule_GetCEAMonitorCaps: EDID block%d indicates invalid CEA861 block version!\n", BlockIndex));
            continue;
        }

        pMonitorAttrib->IsCEA861Monitor = CBIOS_TRUE;
        pMonitorAttrib->IsCEA861RGB = CBIOS_TRUE;

        // Initialize the CEA861 misc attribution.
        pMonitorAttrib->Tag = pEDIDBlock[0x00];
        pMonitorAttrib->RevisionNumber = pEDIDBlock[0x01];
        pMonitorAttrib->OffsetOfDetailedTimingBlock = pEDIDBlock[0x02];
        pMonitorAttrib->TotalNumberOfNativeFormat = pEDIDBlock[0x03] & 0x0F;
        pMonitorAttrib->IsCEA861UnderScan = (pEDIDBlock[0x03] & 0x80) >> 7;
        pMonitorAttrib->IsCEA861Audio = (pEDIDBlock[0x03] & 0x40) >> 6;
        pMonitorAttrib->IsCEA861YCbCr444 = (pEDIDBlock[0x03] & 0x20) >> 5;
        pMonitorAttrib->IsCEA861YCbCr422 = (pEDIDBlock[0x03] & 0x10) >> 4;

        for(i = 4; i < pMonitorAttrib->OffsetOfDetailedTimingBlock;)
        {
            if (((pEDIDBlock[i] >> 5) & 0x07) == VENDOR_SPECIFIC_DATA_BLOCK_TAG)
            {
                CBIOS_U8 PayloadLength = pEDIDBlock[i++] & 0x1F;

                if ((pEDIDBlock[i] == 0x03)
                    && (pEDIDBlock[i + 1] == 0x0C)
                    && (pEDIDBlock[i + 2] == 0x00))
                {
                    bIsHDMIDevice = CBIOS_TRUE;
                }

                i += PayloadLength;
            }
            else if (((pEDIDBlock[i] >> 5) & 0x07) == AUDIO_DATA_BLOCK_TAG)
            {
                //audio data block
                CBIOS_U8 PayloadLength = pEDIDBlock[i++] & 0x1F;
                SADCnt += PayloadLength/3;  // 3bytes makes one audio format

                if (SADCnt > 15)
                {
                    SADCnt = 15; //ELD spec define, at most 15 SADs
                }
                if (SADCnt > pMonitorAttrib->SAD_Count)
                {
                    cb_memcpy(((CBIOS_U8 *)pMonitorAttrib->CEA_SADs + pMonitorAttrib->SAD_Count * 3),
                           &pEDIDBlock[i], (SADCnt - pMonitorAttrib->SAD_Count)*3 );
                }

                pMonitorAttrib->SAD_Count = SADCnt;

                i += PayloadLength;
            }
            else if (((pEDIDBlock[i] >> 5) & 0x07) == SPEAKER_ALLOCATION_DATA_BLOCK_TAG)
            {
                //This payload is preceded by a Tag Code Byte that includes a tag equal to 4 and a length of 3
                pMonitorAttrib->SpeakerAllocationData = pEDIDBlock[i+1];
                i += 4;
            }
            else
            {
                CBIOS_U32 PayloadLength = (pEDIDBlock[i++] & 0x1F);

                i += PayloadLength;
            }
        }

        if (bIsHDMIDevice
            && (pMonitorAttrib->IsCEA861UnderScan
            | pMonitorAttrib->IsCEA861Audio
            | pMonitorAttrib->IsCEA861YCbCr422
            | pMonitorAttrib->IsCEA861YCbCr444))
        {
            pMonitorAttrib->IsCEA861HDMI = CBIOS_TRUE;
        }
    }

    return pMonitorAttrib->CEA861MonitorCaps;

}

/***************************************************************
Function:    cbEDIDModule_GetMonitor3DCaps

Description: Get monitor 3D capability

Input:       pEDIDStruct, decoded EDID timings and attributes
             IsSupport3DVideo, whether the monitor supports 3D video

Output:      p3DCapability, store decoded monitor 3D capabilitys

Return:
***************************************************************/
CBIOS_STATUS cbEDIDModule_GetMonitor3DCaps(PCBIOS_EDID_STRUCTURE_DATA pEDIDStruct,
                                           PCBIOS_MONITOR_3D_CAPABILITY_PARA p3DCapability,
                                           CBIOS_U32 IsHWSupportHDMI3DVideo)
{
    PCBIOS_HDMI_FORMAT_DESCRIPTOR   pSupportFormat = CBIOS_NULL;
    CBIOS_U32 Monitor3DModeNum = 0;
    PCBIOS_3D_VIDEO_MODE_LIST pModeList = p3DCapability->pMonitor3DModeList;
    CBIOS_U32 i;

    p3DCapability->bStereoViewSupport = pEDIDStruct->Attribute.bStereoViewSupport;
    p3DCapability->StereoViewType = pEDIDStruct->Attribute.StereoViewType;

    if (IsHWSupportHDMI3DVideo && pEDIDStruct->Attribute.VSDBData.HDMI3DPresent)
    {
        p3DCapability->bIsSupport3DVideo = CBIOS_TRUE;
        pSupportFormat = pEDIDStruct->HDMIFormat;

        for (i = 0; i < CBIOS_HDMIFORMATCOUNTS; i++)
        {
            if ((pSupportFormat[i].IsSupported)
                && (pSupportFormat[i].Video3DSupportCaps & CBIOS_3D_VIDEO_FORMAT_MASK))
            {
                Monitor3DModeNum++;

                if (pModeList != CBIOS_NULL)
                {
                    pModeList->XRes = CEAVideoFormatTable[i].XRes;
                    pModeList->YRes = CEAVideoFormatTable[i].YRes;
                    pModeList->RefreshRate = CEAVideoFormatTable[i].RefRate[pSupportFormat[i].RefreshIndex];
                    pModeList->bIsInterlace = (CBIOS_BOOL)CEAVideoFormatTable[i].Interlace;
                    pModeList->SupportCaps = pSupportFormat[i].Video3DSupportCaps & CBIOS_3D_VIDEO_FORMAT_MASK;
                    pModeList->IsSupport3DOSDDisparity = pEDIDStruct->Attribute.HFSCDSData.IsSupport3DOSDDisparity;
                    pModeList->IsSupport3DDualView = pEDIDStruct->Attribute.HFSCDSData.IsSupport3DDualView;
                    pModeList->IsSupport3DIndependentView = pEDIDStruct->Attribute.HFSCDSData.IsSupport3DIndependentView;
                    pModeList++;
                }

            }
        }

        //if no 3D mode is supported, set support flags to not support 3D video
        if (Monitor3DModeNum == 0)
        {
            p3DCapability->bIsSupport3DVideo = CBIOS_FALSE;
        }
        p3DCapability->Monitor3DModeNum = Monitor3DModeNum;
    }
    else
    {
        p3DCapability->bIsSupport3DVideo = CBIOS_FALSE;
        p3DCapability->Monitor3DModeNum = 0;
    }

    return CBIOS_OK;
}

/***************************************************************
Function:    cbEDIDModule_IsEDIDValid

Description: if the EDID header is valid

Input:       pEDID, EDID data buffer

Output:

Return:      CBIOS_TRUE if EDID header vaild
             CBIOS_FALSE if EDID header invaild
***************************************************************/
CBIOS_BOOL cbEDIDModule_IsEDIDHeaderValid(CBIOS_U8  *pEDIDBuffer, CBIOS_U32 ulBufferSize)
{
    CBIOS_U8 EDID1header[] = {0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};
    CBIOS_BOOL  bRet = CBIOS_FALSE;

    if(ulBufferSize < sizeof(EDID1header))
        return CBIOS_FALSE;

    if((pEDIDBuffer != CBIOS_NULL) &&
        (!cb_memcmp(pEDIDBuffer, EDID1header, sizeof(EDID1header))))
        bRet = CBIOS_TRUE;

    return bRet;
}

/***************************************************************
Function:    cbEDIDModule_IsEDIDValid

Description: if the EDID data is CBIOS_NULL, the header is not correct
             or the check sum is not zero, return false;
             otherwise return true.

Input:       pEDID, EDID data buffer

Output:

Return:      CBIOS_TRUE if EDID vaild
             CBIOS_FALSE if EDID is null, header is not correct
             or check sum is not zero
***************************************************************/
CBIOS_BOOL cbEDIDModule_IsEDIDValid(CBIOS_U8 *pEDID)
{
    CBIOS_U32 ulBufferSize;
    CBIOS_U8  byTemp;
    CBIOS_U32 i;
    CBIOS_BOOL  bRet = CBIOS_FALSE;

    ulBufferSize = 256;
    if((pEDID != CBIOS_NULL) && cbEDIDModule_IsEDIDHeaderValid(pEDID, ulBufferSize))
    {
        byTemp = 0;
        for (i = 0 ; i < ulBufferSize ; i++)
        {
            byTemp = byTemp + pEDID[i];
            //if checksum of 128 or 256 bytes is 0, success.
            if(((i == 127) && (byTemp == 0)) ||((i == 255) && (byTemp == 0)))
            {
                break;
            }
        }

        if(byTemp == 0)
        {
            bRet = CBIOS_TRUE;
        }
    }

    return bRet;
}

/***************************************************************
Function:    cbEDIDModule_GetMonitorID

Description: Get monitor ID manufacturer name

Input:       pEDID, monitor EDID data buffer

Output:      pMnitorID, monitor ID manufacturer name buffer

Return:      CBIOS_TRUE if get monitor ID successfully
             CBIOS_FALSE if get monitor ID failed
***************************************************************/
CBIOS_BOOL cbEDIDModule_GetMonitorID(CBIOS_U8 *pEDID, CBIOS_U8 *pMonitorID)
{
    CBIOS_U8 *index = "0ABCDEFGHIJKLMNOPQRSTUVWXYZ[/]^_";
    CBIOS_U8 ProductID[3] = {0};
    CBIOS_BOOL bRet = CBIOS_FALSE;
    CBIOS_U8 *pMonitorIDinEDID = pEDID + MONITORIDINDEX;

    if ((pMonitorIDinEDID == CBIOS_NULL) || (pMonitorID == CBIOS_NULL))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbEDIDModule_GetMonitorID: input buffer is null!\n"));
        return CBIOS_FALSE;
    }

    //get manufacturer ID
    pMonitorID[0] = index[(pMonitorIDinEDID[0] >> 2) & 0x1F];
    pMonitorID[1] = index[((pMonitorIDinEDID[1] >> 5) & 0x07) | ((pMonitorIDinEDID[0] << 3) & 0x18)];
    pMonitorID[2] = index[pMonitorIDinEDID[1] & 0x1F];
    pMonitorID[3] = '\0';

    if (cbItoA((CBIOS_U32)pMonitorIDinEDID[3], ProductID, 16, 2))
    {
        cbStrCat(pMonitorID, ProductID);
        if (cbItoA((CBIOS_U32)pMonitorIDinEDID[2], ProductID, 16, 2))
        {
            cbStrCat(pMonitorID, ProductID);
            bRet = CBIOS_TRUE;
            cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "Monitor ID is: %s\n", pMonitorID));
        }
    }

    return bRet;

}

//For HDMI and DP:Patch to disable 24bit for L_PCM audio format to pass HLK wave test
CBIOS_VOID cbEDIDModule_SADPatch(PCBIOS_EDID_STRUCTURE_DATA pEDIDStruct)
{
    CBIOS_U32 Index = 0;
    CBIOS_U8 SADCnt = pEDIDStruct->Attribute.SAD_Count;
    CBIOS_U32 HDMIAudioFormatNum = pEDIDStruct->TotalHDMIAudioFormatNum;
    PCBIOS_MONITOR_MISC_ATTRIB pMonitorAttrib = &(pEDIDStruct->Attribute);

    //modify SAD to disable 24bit for L_PCM audio format
    for (Index = 0; Index < SADCnt; Index++)
    {
        if (((pMonitorAttrib->CEA_SADs[Index][0] >> 3) & 0xF) == 1)
        {
            if ((pMonitorAttrib->CEA_SADs[Index][2] >> 2) & 0x1)
            {
                pMonitorAttrib->CEA_SADs[Index][2] &= 0xFB;
            }
        }
    }

    for (Index = 0; Index < HDMIAudioFormatNum; Index++)
    {
        if (pEDIDStruct->HDMIAudioFormat[Index].Format == CBIOS_AUDIO_FORMAT_LPCM)
        {
            pEDIDStruct->HDMIAudioFormat[Index].BitDepth.BD_24bit = 0;
        }
    }
}

CBIOS_VOID cbEDIDModule_Patch(CBIOS_U8 *pEDID, CBIOS_U32 TotalBlocks)
{
    PCBIOS_MONITOR_MISC_ATTRIB pMonitorAttrib = CBIOS_NULL;
    CBIOS_UCHAR                MonitorID[8] = {0};

    pMonitorAttrib = cb_AllocateNonpagedPool(sizeof(CBIOS_MONITOR_MISC_ATTRIB));
    if (pMonitorAttrib == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: pMonitorAttrib allocate error.\n", FUNCTION_NAME));
        return;
    }

    cbEDIDModule_GetMonitorAttrib(pEDID, pMonitorAttrib, TotalBlocks);
    cbEDIDModule_GetMonitorID(pEDID, MonitorID);

    // PHILIPS 24PFL3545 monitor exist a vertical garbage of mode 1920x1080i@50Hz as Hsync offset
    // is not correct in EDID CEA extension. Patch this issue by correct EDID data.
    if ((!cb_strcmp(MonitorID, (CBIOS_UCHAR*)"PHL0010")) && (!cb_strcmp(pMonitorAttrib->MonitorName, (CBIOS_UCHAR*)"B24PFL3545/T3")) && (pMonitorAttrib->IsCEA861HDMI))
    {
        cb_memcpy(pEDID, PHL_24PFL3545_Edid, CBIOS_EDIDDATABYTE);
    }

    cb_FreePool(pMonitorAttrib);
}


/***************************************************************
Function:    cbEDIDModule_ParseEDID

Description: parse the whole EDID data

Input:       pEDID, EDID data buffer
             ulBufferSize, specify the size of EDID data buffer

Output:      pEDIDStruct, store decoded EDID timings and attributes

Return:
***************************************************************/
CBIOS_BOOL cbEDIDModule_ParseEDID(CBIOS_U8 *pEDID, PCBIOS_EDID_STRUCTURE_DATA pEDIDStruct, CBIOS_U32 ulBufferSize)
{
    CBIOS_U32   ulModeNum = 0;
    CBIOS_U32   BlockNum = ulBufferSize / EDID_BLOCK_SIZE_SPEC;
    CBIOS_BOOL  bRet = CBIOS_FALSE;


    if (cbEDIDModule_IsEDIDValid(pEDID))
    {
        cbEDIDModule_Patch(pEDID, BlockNum);

        cb_memset(pEDIDStruct, 0, sizeof(CBIOS_EDID_STRUCTURE_DATA));

        ulModeNum += cbEDIDModule_GetEstablishMode(pEDID, pEDIDStruct->EstTimings);
        ulModeNum += cbEDIDModule_GetStandardMode(pEDID, pEDIDStruct->StdTimings);
        ulModeNum += cbEDIDModule_GetDetailedMode(pEDID, pEDIDStruct->DtlTimings, 4);

        cbEDIDModule_GetMonitorAttrib(pEDID, &(pEDIDStruct->Attribute), BlockNum);

        if (ulBufferSize > 128)
        {
            ulModeNum += cbEDIDModule_GetCEA861Mode(pEDID, pEDIDStruct, BlockNum);
            if (pEDIDStruct->Attribute.TotalNumberOfNativeFormat > 0)
            {
                cbEDIDModule_SetNativeFlag(pEDIDStruct);
            }
        }
        if (ulBufferSize > 128)
        {
            ulModeNum += cbEDIDModule_GetDisplayIDMode(pEDID, pEDIDStruct);
        }

        bRet = CBIOS_TRUE;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "cbEDIDModule_ParseEDID: EDID is invalid!\n"));
    }

    pEDIDStruct->TotalModeNum = ulModeNum;

    return bRet;
}

/***************************************************************
Function:    cbEDIDModule_SearchTmInEdidStruct

Description: Search a specific timing in decoded EDID struct where
             stores all the timings in EDID

Input:       XResolution, YResolution, RefreshRate, InterlaceFlag

Output:      pTmBlock, will tell caller in whick block find the timing.
             pTmBlock == 0, Establish timing block.
                      == 1, Standard timing block.
                      == 2, Detailed timing block.
                      == 3, HDMI Format timing block.
                      == 4, Detailed timing descriptor.
             pTmIndex will tell caller the index number of responding timing.

Return:      CBIOS_TRUE if found the responding timing, else, return CBIOS_FALSE.
***************************************************************/
CBIOS_BOOL cbEDIDModule_SearchTmInEdidStruct(CBIOS_U32 XResolution,
                                             CBIOS_U32 YResolution,
                                             CBIOS_U32 RefreshRate,
                                             CBIOS_U32 InterlaceFlag,
                                             PCBIOS_EDID_STRUCTURE_DATA pEDIDStruct,
                                             PCBIOS_U32 pTmBlock,
                                             PCBIOS_U32 pTmIndex)
{
    CBIOS_U8 byRefRateIndex = 0;
    CBIOS_BOOL bRet = CBIOS_FALSE;
    CBIOS_U32 i = 0;

    for(i=0; i<CBIOS_DTLMODECOUNT; i++)
    {
        if((pEDIDStruct->DtlTimings[i].Valid)&&
           (pEDIDStruct->DtlTimings[i].XResolution == XResolution)&&
           (pEDIDStruct->DtlTimings[i].YResolution == YResolution)&&
           (pEDIDStruct->DtlTimings[i].InterLaced == InterlaceFlag)&&
           ((pEDIDStruct->DtlTimings[i].Refreshrate >= (RefreshRate - 50))&&
            (pEDIDStruct->DtlTimings[i].Refreshrate <= (RefreshRate + 50))))
        {
            *pTmIndex = i;
            *pTmBlock = 2;
            bRet = CBIOS_TRUE;
            break;

        }
    }
    if ((!bRet) && pEDIDStruct->Attribute.IsCEA861Monitor)
    {
        for (i = 0; i < CBIOS_DTDTIMINGCOUNTS; i++)
        {
            if ((pEDIDStruct->DTDTimings[i].Valid) &&
                (pEDIDStruct->DTDTimings[i].XResolution == XResolution) &&
                (pEDIDStruct->DTDTimings[i].YResolution == YResolution) &&
                (pEDIDStruct->DTDTimings[i].InterLaced == InterlaceFlag) &&
                ((pEDIDStruct->DTDTimings[i].Refreshrate >= (RefreshRate - 50)) &&
                (pEDIDStruct->DTDTimings[i].Refreshrate <= (RefreshRate + 50))))
            {
                *pTmIndex = i;
                *pTmBlock = 4;
                bRet = CBIOS_TRUE;
                break;
            }
        }
    }
    if((!bRet) && ((pEDIDStruct->Attribute.IsCEA861Monitor)||
       (pEDIDStruct->Attribute.IsCEA861HDMI)))
    {
        for(i=0; i<CBIOS_HDMIFORMATCOUNTS; i++)
        {
            byRefRateIndex = pEDIDStruct->HDMIFormat[i].RefreshIndex%2;
            if((pEDIDStruct->HDMIFormat[i].IsSupported)&&
               (CEAVideoFormatTable[i].XRes == XResolution)&&
               (CEAVideoFormatTable[i].YRes == YResolution)&&
               (CEAVideoFormatTable[i].Interlace == InterlaceFlag)&&
               (CEAVideoFormatTable[i].RefRate[byRefRateIndex] == RefreshRate))
            {
                *pTmIndex = i;
                *pTmBlock = 3;
                bRet = CBIOS_TRUE;
                break;
            }
        }
    }
    if(!bRet)
    {
        for(i=0; i<CBIOS_STDMODECOUNT; i++)
        {
            if((pEDIDStruct->StdTimings[i].Valid)&&
               (pEDIDStruct->StdTimings[i].XResolution == XResolution)&&
               (pEDIDStruct->StdTimings[i].YResolution == YResolution)&&
               (pEDIDStruct->StdTimings[i].Refreshrate == RefreshRate))
            {
                *pTmIndex = i;
                *pTmBlock = 1;
                bRet = CBIOS_TRUE;
                break;
            }
        }
    }
    if(!bRet)
    {
        for(i=0; i<CBIOS_ESTABLISHMODECOUNT; i++)
        {
            if((pEDIDStruct->EstTimings[i].Valid)&&
               (pEDIDStruct->EstTimings[i].XResolution == XResolution)&&
               (pEDIDStruct->EstTimings[i].YResolution == YResolution)&&
               (pEDIDStruct->EstTimings[i].Refreshrate == RefreshRate))
            {
                *pTmIndex = i;
                *pTmBlock = 0;
                bRet = CBIOS_TRUE;
                break;
            }
        }
    }
    if(!bRet)
    {
        for(i=0; i<CBIOS_DISPLAYID_TYPE1_MODECOUNT; i++)
        {
            if((pEDIDStruct->DisplayID_TYPE1_Timings[i].Valid)&&
               (pEDIDStruct->DisplayID_TYPE1_Timings[i].XResolution == XResolution)&&
               (pEDIDStruct->DisplayID_TYPE1_Timings[i].YResolution == YResolution)&&
               (pEDIDStruct->DisplayID_TYPE1_Timings[i].InterLaced == InterlaceFlag)&&
               ((pEDIDStruct->DisplayID_TYPE1_Timings[i].Refreshrate >= (RefreshRate - 50))&&
                (pEDIDStruct->DisplayID_TYPE1_Timings[i].Refreshrate <= (RefreshRate + 50))))
            {
                *pTmIndex = i;
                *pTmBlock = 5;
                bRet = CBIOS_TRUE;
                break;
            }
        }
    }

    return bRet;
}

/***************************************************************
Function:    cbEDIDModule_FakeDetailedTiming

Description: Fake Detailed Timing EDID data according to detailed timing.

Input:       pDtlTiming, detailed timing

Output:      pEdid, faked EDID buffer.
***************************************************************/
static CBIOS_VOID cbEDIDModule_FakeDetailedTiming(CBIOS_U8 *pEdid, PCBIOS_MODE_INFO_EXT pDtlTiming)
{
    CBIOS_U16 HorBlanking = pDtlTiming->HBlank;
    CBIOS_U16 VerBlanking = pDtlTiming->VBlank;
    CBIOS_U16 HorSyncOffset = pDtlTiming->HSyncOffset;
    CBIOS_U16 HorSyncWidth = pDtlTiming->HSyncPulseWidth;
    CBIOS_U16 VerSyncOffset = pDtlTiming->VSyncOffset;
    CBIOS_U16 VerSyncWidth = pDtlTiming->VSyncPulseWidth;

    pEdid[0x38] = (CBIOS_U8)(pDtlTiming->HActive & 0xFF);
    pEdid[0x39] = (CBIOS_U8)(HorBlanking & 0xFF);
    pEdid[0x3A] = (CBIOS_U8)(((pDtlTiming->HActive & 0xF00) >> 4) | ((HorBlanking & 0xF00) >> 8));
    pEdid[0x3B] = (CBIOS_U8)(pDtlTiming->VActive & 0xFF);
    pEdid[0x3C] = (CBIOS_U8)(VerBlanking & 0xFF);
    pEdid[0x3D] = (CBIOS_U8)(((pDtlTiming->VActive & 0xF00) >> 4) | ((VerBlanking & 0xF00) >> 8));
    pEdid[0x3E] = (CBIOS_U8)(HorSyncOffset & 0xFF);
    pEdid[0x3F] = (CBIOS_U8)(HorSyncWidth & 0xFF);
    pEdid[0x40] = (CBIOS_U8)((VerSyncOffset & 0x0F) << 4 | (VerSyncWidth & 0x0F));
    pEdid[0x41] = (CBIOS_U8)(((HorSyncOffset & 0x300) >> 2) | ((HorSyncWidth & 0x300) >> 4)
                            | ((VerSyncOffset & 0x30) >> 2) | ((VerSyncWidth & 0x30) >> 4));
    pEdid[0x42] = 0x90;
    pEdid[0x43] = 0x2C;
    pEdid[0x44] = 0x11;
    pEdid[0x47] = 0x18; // Digital Separate

    // H polarity
    if (pDtlTiming->HSync == HorPOSITIVE)
    {
        pEdid[0x47] |= 0x2;
    }

    // V polarity
    if (pDtlTiming->VSync == VerPOSITIVE)
    {
        pEdid[0x47] |= 0x4;
    }
}

/***************************************************************
Function:    cbEDIDModule_FakePanelEDID

Description: Fake EDID data for Panel Device since it doesn't have
             EDID data in its local side.

Input:       pFakeEdidParam, params used to fake EDID
             EdidBufferLength, the length of edid buffer (currently just support 128 bytes)

Output:      pEdid, faked EDID buffer.

Return:      CBIOS_TRUE if fake panel EDID success, else, CBIOS_FALSE.
***************************************************************/
CBIOS_BOOL cbEDIDModule_FakePanelEDID(PCBIOS_FAKE_EDID_PARAMS pFakeEdidParam, PCBIOS_U8 pEdid, const CBIOS_U32 EdidBufferLength)
{
    CBIOS_BOOL bRet = CBIOS_FALSE;
    CBIOS_U32 i = 0;
    CBIOS_U8  CheckSum = 0;

    if ((pFakeEdidParam == CBIOS_NULL) || (pEdid == CBIOS_NULL))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: the 1st or 2nd param is NULL!\n", FUNCTION_NAME));
        return bRet;
    }

    if (EdidBufferLength != 128)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: currently just support faking 128-byte EDID!\n", FUNCTION_NAME));
        return bRet;
    }

    //fake 128 Bytes EDID only
    cb_memset(pEdid, 0, EdidBufferLength);

    //EDID header
    pEdid[0] = 0x00;
    pEdid[1] = 0xFF;
    pEdid[2] = 0xFF;
    pEdid[3] = 0xFF;
    pEdid[4] = 0xFF;
    pEdid[5] = 0xFF;
    pEdid[6] = 0xFF;
    pEdid[7] = 0x00;

    //ID Manufacturer Name. Compressed ASCII.  Expands to 'MS_' for Microsoft
    pEdid[8] = 0x36;
    pEdid[9] = 0x7f;

    //ID Product Code
    pEdid[0x0A] = 0x00;
    pEdid[0x0B] = 0x00;

    //ID Serial Number
    pEdid[0x0C] = 0x00;
    pEdid[0x0D] = 0x00;
    pEdid[0x0E] = 0x00;
    pEdid[0x0F] = 0x00;

    //Week and Year
    pEdid[0x10] = 1;
    pEdid[0x11] = 10;

    //Version
    pEdid[0x12] = 0x01;
    pEdid[0x13] = 0x03;

    //Mark LCD as digital device
    pEdid[0x14] = 0x80;

    //Max. Horizontal Image Size, cm
    pEdid[0x15] = 40;

    //Max. Vertical Image Size, cm
    pEdid[0x16] = 30;

    //Display Transfer (gamma)
    pEdid[0x17] = 0;

    //Add Preferred Timing Mode flag
    pEdid[0x18] |= 0x02;

    //Color Characteristics
    pEdid[0x19] = 0x78;
    pEdid[0x1A] = 0xA0;
    pEdid[0x1B] = 0x56;
    pEdid[0x1C] = 0x48;
    pEdid[0x1D] = 0x9A;
    pEdid[0x1E] = 0x26;
    pEdid[0x1F] = 0x12;
    pEdid[0x20] = 0x48;
    pEdid[0x21] = 0x4C;
    pEdid[0x22] = 0xFF;

    //DCLK
    pEdid[0x36] = (CBIOS_U8)(pFakeEdidParam->DtlTiming.PixelClock);
    pEdid[0x37] = (CBIOS_U8)(pFakeEdidParam->DtlTiming.PixelClock >> 8);

    //detailed timing
    if (pFakeEdidParam->bProvideDtlTimingEDID)
    {
        cb_memcpy(pEdid + 0x38, pFakeEdidParam->DtlTimingEDID, 16);
    }
    else
    {
        cbEDIDModule_FakeDetailedTiming(pEdid, &(pFakeEdidParam->DtlTiming));
    }

    //second 18 byte data block, display product name descriptor
    //flags and tag
    pEdid[0x48] = 0x00;
    pEdid[0x49] = 0x00;
    pEdid[0x4A] = 0x00;
    pEdid[0x4B] = 0xFC;    //display product name tag
    pEdid[0x4C] = 0x00;
    //fake monitor model name, "XXX LCD"
    pEdid[0x4D] = 0x5A;
    pEdid[0x4E] = 0x58;
    pEdid[0x4F] = 0x47;
    pEdid[0x50] = 0x20;
    pEdid[0x51] = 0x4C;
    pEdid[0x52] = 0x43;
    pEdid[0x53] = 0x44;
    pEdid[0x54] = 0x0A;
    pEdid[0x55] = 0x20;
    pEdid[0x56] = 0x20;
    pEdid[0x57] = 0x20;
    pEdid[0x58] = 0x20;
    pEdid[0x59] = 0x20;

    //no extension block
    pEdid[0x7E] = 0;

    //checksum
    for (i = 0; i < EdidBufferLength - 1; i++)
    {
        CheckSum += pEdid[i];
    }

    CheckSum = 0xFF - CheckSum + 1;
    pEdid[0x7F] = CheckSum;

    bRet = CBIOS_TRUE;
    return bRet;
}
