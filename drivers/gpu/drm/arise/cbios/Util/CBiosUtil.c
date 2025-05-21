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

#define BASEV(b, v)   b##v
#define TO_BASE16(a)  BASEV(0x, a)
#define CBIOS_TRNAS_VER(CBIOS_VER) TO_BASE16(CBIOS_VER)


static CBIOS_U32 cbGetVsyncWidth(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 XRes, CBIOS_U32 YRes)
{
    CBIOS_U32 ulRet = 10;
    CBIOS_U32 ulTemp = 0;

    // Prevent being divided by zero
    if (YRes == 0)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbCalcCustomizedTiming: fata error -- YRes is ZERO!!!\n"));
        return ulRet;
    }
    ulTemp = XRes*100/YRes;
    switch(ulTemp)
    {
        case 133:
            ulRet = 4;
            break;
        case 177:
            ulRet = 5;
            break;
        case 160:
            ulRet = 6;
            break;
        case 166:
            ulRet = 7;
            break;
        default:
            ulRet = 10;
            break;
    }

    return ulRet;
}


CBIOS_STATUS cbGetExtensionSize(CBIOS_U32 *pulExtensionSize)
{
    *pulExtensionSize = sizeof(CBIOS_EXTENSION_COMMON);
    return CBIOS_OK;
}


CBIOS_STATUS cbGetVersion(PCBIOS_EXTENSION_COMMON pcbe,
                                 PCBIOS_VERSION pCbiosVersion)
{
    // Big change such as structure modification
    pCbiosVersion->MajorVersionNumber = CBIOS_TRNAS_VER(CBIOS_VERSION_MAJOR);
    // Means Trunk driver.
    pCbiosVersion->BranchDriverNumber = CBIOS_TRNAS_VER(CBIOS_VERSION_BRANCH);
    // If add new feature, increase this version number
    pCbiosVersion->MediumVersionNumber = CBIOS_TRNAS_VER(CBIOS_VERSION_FEATURE);
    // For bug fixed only. Every release, increase this version number.
    pCbiosVersion->MinorVersionNumber = CBIOS_TRNAS_VER(CBIOS_VERSION_MINOR);

    // Genery we directly return the value in pcbe except specific-chip CBIOS.
    pCbiosVersion->ChipID = (CBIOS_U16)pcbe->ChipID;
    pCbiosVersion->ChipRevID = (CBIOS_U16)pcbe->ChipRevision;

    return CBIOS_OK;
}


// Calculate the detailed timing according to the CVT algrithm
// Input X resolution, Y resolution, Refresh.
// Output the CBIOS_TIMING_ATTRIB stucture
CBIOS_BOOL cbCalcCustomizedTiming(PCBIOS_EXTENSION_COMMON pcbe,
                                CBIOS_U32 XRes,
                                CBIOS_U32 YRes,
                                CBIOS_U32 RefreshRate,
                                PCBIOS_TIMING_ATTRIB pTiming)
{

    CBIOS_U32 ReqestRefreshRate = RefreshRate;
    CBIOS_U32 HorPixels = XRes;
    CBIOS_U32 VerticalLines = YRes;

    // result variable
    CBIOS_U32 HorTotal;                   // unit is pixels
    CBIOS_U32 HorBlanking;
    CBIOS_U32 HorFrontPorch;
    CBIOS_U32 HorSync;
    CBIOS_U32 HorBackPorch;

    CBIOS_U32 VerTotal;                   // unit is lines
    CBIOS_U32 VerBlanking;
    CBIOS_U32 VerFrontPorch;
    //CBIOS_U32 VerBackPorch;
    CBIOS_U32 VerticalSyncWidth;

    CBIOS_U32 HorFreq;                   // unit is kHz*100
    CBIOS_U32 ActualVerFreq;
    CBIOS_U32 PixelClock;

    // middle temporary variable
    CBIOS_U32 HPeriodEst;                // unit is kHz
    CBIOS_U32 VSyncBP;
    CBIOS_U32 Temp;
    CBIOS_U32 IdealDutyCycle;

    // For recuced blanking middle temopary variable.

    VerticalSyncWidth = cbGetVsyncWidth(pcbe, XRes, YRes);

    // For standard CRT timing calculation
    // Estimate the horizontal period that is multiplied by 100
    // Prevent being divided by zero
    if (ReqestRefreshRate == 0)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "cbCalcCustomizedTiming: fata error -- ReqestRefreshRate is ZERO!!!\n"));
        ReqestRefreshRate = 6000;
    }
    HPeriodEst = 100000000/ReqestRefreshRate - MINVBLANKINGPERIOD;
    Temp = VerticalLines + MINVBPORCH;
    HPeriodEst = cbRound(HPeriodEst*100,Temp, ROUND_NEAREST);

    // Find the number of lines in V sync + back porch
    VSyncBP = cbRound(100*MINVBLANKINGPERIOD,HPeriodEst, ROUND_NEAREST)+1;
    if(VSyncBP < (VerticalSyncWidth + MINVBPORCH))
    {
     VSyncBP = VerticalSyncWidth + MINVBPORCH;
    }
    // Find the number of lines in V back porch
    //VerBackPorch = VSyncBP - VerticalSyncWidth;
    // Find the number of lines in V front porch
    VerFrontPorch = MINVFPORCH;
    // Find total number of lines in Vertical Field Period:
    VerTotal = VerticalLines + VSyncBP + MINVFPORCH;
    // Find the vertical blanking period
    VerBlanking = VerTotal - VerticalLines;
    // Find the ideal blanking duty cycle from the blanking duty cycle equation(%)
    IdealDutyCycle = 100*CPRIME - MPRIME*HPeriodEst/1000;

    // Find the number of pixels in the horizontal blanking time to the nearest
    // double character cell
    if(IdealDutyCycle < 2000)
    {
     HorBlanking = HorPixels*2000;
     HorBlanking = HorBlanking/(10000-2000);
     HorBlanking = cbRound(HorBlanking,2*CELLGRAN, ROUND_NEAREST);
     HorBlanking = HorBlanking*2*CELLGRAN;
    }
    else
    {
     HorBlanking = HorPixels*IdealDutyCycle;
     // Prevent being divided by zero
     if ((10000-IdealDutyCycle) == 0)
     {
         cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbCalcCustomizedTiming: fata error -- (10000-IdealDutyCycle) is ZERO!!!\n"));
         return CBIOS_FALSE;
     }
     HorBlanking = HorBlanking/(10000-IdealDutyCycle);
     HorBlanking = cbRound(HorBlanking,2*CELLGRAN, ROUND_NEAREST);
     HorBlanking = HorBlanking*2*CELLGRAN;
    }
    // Find the total number of pixels in a line
    HorTotal = HorPixels + HorBlanking;
    // Find the Horizontal Sync width, unit is character.
    HorSync = cbRound(HorTotal*HSYNCPERCENT,CELLGRAN*100, ROUND_NEAREST);
    // Convert the unit of H sync width to pixel unit
    HorSync = HorSync*CELLGRAN;
    // Find the Horizontal back porch
    HorBackPorch = HorBlanking/2;
    // Find the H front porch
    HorFrontPorch = HorBlanking-HorSync-HorBackPorch;

    // Find pixel clock frequency(MHz) that was multiplied by 100)
    PixelClock = cbRound(HorTotal*100*100,HPeriodEst, ROUND_NEAREST);
    PixelClock = cbRound(PixelClock,CLOCKSTEP, ROUND_NEAREST);
    PixelClock = PixelClock*CLOCKSTEP;
    // Find actual Horizonal Frequency (kHz), that is multiplied by 100
    // Prevent being divided by zero
    if (HorTotal == 0)
    {
     cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbCalcCustomizedTiming: fata error -- HorTotal is ZERO!!!\n"));
     return CBIOS_FALSE;
    }
    HorFreq = 1000*PixelClock/HorTotal;
    // Find the actual refresh rate(Hz), that was multiplied by 100
    // Prevent being divided by zero
    if (VerTotal == 0)
    {
     cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbCalcCustomizedTiming: fata error -- VerTotal is ZERO!!!\n"));
     return CBIOS_FALSE;
    }
    ActualVerFreq = 1000*HorFreq/VerTotal;

    cb_memset(pTiming, 0, sizeof(CBIOS_TIMING_ATTRIB));
    // Fill in the timing structure
    pTiming->XRes = (CBIOS_U16)HorPixels;
    pTiming->YRes = (CBIOS_U16)VerticalLines;
    pTiming->RefreshRate = (CBIOS_U16)ActualVerFreq;
    pTiming->PixelClock = (CBIOS_U32)PixelClock*100;
    pTiming->HVPolarity = VerPOSITIVE+HorNEGATIVE;
    pTiming->HorTotal = (CBIOS_U16)HorTotal;
    pTiming->HorDisEnd = (CBIOS_U16)HorPixels;
    pTiming->HorBStart = (CBIOS_U16)HorPixels;
    pTiming->HorBEnd =  (CBIOS_U16)HorTotal;
    pTiming->HorSyncStart = (CBIOS_U16)(HorPixels+HorFrontPorch);
    pTiming->HorSyncEnd = (CBIOS_U16)(HorPixels+HorFrontPorch+HorSync);

    pTiming->VerTotal = (CBIOS_U16)(VerTotal);
    pTiming->VerDisEnd = (CBIOS_U16)(VerticalLines);
    pTiming->VerBStart = (CBIOS_U16)(VerticalLines);
    pTiming->VerBEnd = (CBIOS_U16)(VerticalLines+VerBlanking);
    pTiming->VerSyncStart = (CBIOS_U16)(VerticalLines+VerFrontPorch);
    pTiming->VerSyncEnd = (CBIOS_U16)(VerticalLines+VerFrontPorch+VerticalSyncWidth);

    return CBIOS_TRUE;
}


CBIOS_VOID cbConvertEdidTimingToTableTiming(CBIOS_IN PCBIOS_EXTENSION_COMMON pcbe,
                                          CBIOS_IN CBIOS_MODE_INFO_EXT* pEDIDDetailTiming,
                                          CBIOS_OUT PCBIOS_TIMING_ATTRIB pTiming)
{
    cb_memset(pTiming, 0, sizeof(CBIOS_TIMING_ATTRIB));

    // Fill in the timing structure
    pTiming->XRes = (CBIOS_U16)pEDIDDetailTiming->XResolution;
    pTiming->YRes = (CBIOS_U16)pEDIDDetailTiming->YResolution;
    pTiming->RefreshRate = (CBIOS_U16)pEDIDDetailTiming->Refreshrate;
    pTiming->PixelClock = (CBIOS_U32)pEDIDDetailTiming->PixelClock;
    pTiming->HVPolarity = pEDIDDetailTiming->HSync+pEDIDDetailTiming->VSync;

    pTiming->HorTotal = pEDIDDetailTiming->HActive+pEDIDDetailTiming->HBlank;
    pTiming->HorDisEnd = pEDIDDetailTiming->HActive;
    pTiming->HorBStart = pEDIDDetailTiming->HActive;

    pTiming->HorBEnd = pEDIDDetailTiming->HActive+pEDIDDetailTiming->HBlank;

    pTiming->HorSyncStart = pEDIDDetailTiming->HActive + pEDIDDetailTiming->HSyncOffset;
    pTiming->HorSyncEnd = pEDIDDetailTiming->HActive +
                                     pEDIDDetailTiming->HSyncOffset +
                                     pEDIDDetailTiming->HSyncPulseWidth;

    pTiming->VerTotal = (CBIOS_U16)(pEDIDDetailTiming->VActive+pEDIDDetailTiming->VBlank);
    pTiming->VerDisEnd = (CBIOS_U16)(pEDIDDetailTiming->VActive);
    pTiming->VerBStart = (CBIOS_U16)(pEDIDDetailTiming->VActive);
    pTiming->VerBEnd = (CBIOS_U16)(pEDIDDetailTiming->VActive+pEDIDDetailTiming->VBlank);
    pTiming->VerSyncStart = (CBIOS_U16)(pEDIDDetailTiming->VActive+pEDIDDetailTiming->VSyncOffset);
    pTiming->VerSyncEnd = (CBIOS_U16)(pTiming->VerSyncStart+pEDIDDetailTiming->VSyncPulseWidth);


}

CBIOS_U32 cbConvertCBiosDevBit2VBiosDevBit(CBIOS_U32 CBiosDevices)
{
    CBIOS_U32 temp = 0;

    if(CBiosDevices & CBIOS_TYPE_CRT)
    {
        temp |= VBIOS_TYPE_CRT;
    }

    if(CBiosDevices & CBIOS_TYPE_DVO)
    {
        temp |= VBIOS_TYPE_DVO;
    }

    if(CBiosDevices & CBIOS_TYPE_DUOVIEW)
    {
        temp |= VBIOS_TYPE_DUOVIEW;
    }

    if(CBiosDevices & CBIOS_TYPE_DP1)
    {
        temp |= VBIOS_TYPE_DP1;
    }

    if(CBiosDevices & CBIOS_TYPE_DP2)
    {
        temp |= VBIOS_TYPE_DP2;
    }

    if(CBiosDevices & CBIOS_TYPE_DP3)
    {
        temp |= VBIOS_TYPE_DP3;
    }

    if(CBiosDevices & CBIOS_TYPE_DP4)
    {
        temp |= VBIOS_TYPE_DP4;
    }

    return temp;
}

CBIOS_U32 cbConvertVBiosDevBit2CBiosDevBit(CBIOS_U32 VBiosDevices)
{
    CBIOS_U32 temp = 0;

    if(VBiosDevices & VBIOS_TYPE_CRT)
    {
        temp |= CBIOS_TYPE_CRT;
    }

    if(VBiosDevices & VBIOS_TYPE_DVO)
    {
        temp |= CBIOS_TYPE_DVO;
    }

    if(VBiosDevices & VBIOS_TYPE_DUOVIEW)
    {
        temp |= CBIOS_TYPE_DUOVIEW;
    }

    if(VBiosDevices & VBIOS_TYPE_DP1)
    {
        temp |= CBIOS_TYPE_DP1;
    }

    if(VBiosDevices & VBIOS_TYPE_DP2)
    {
        temp |= CBIOS_TYPE_DP2;
    }

    if(VBiosDevices & VBIOS_TYPE_DP3)
    {
        temp |= CBIOS_TYPE_DP3;
    }

    if(VBiosDevices & VBIOS_TYPE_DP4)
    {
        temp |= CBIOS_TYPE_DP4;
    }

    return temp;
}



// Function name: cbCalcRefreshRate
// Function description: calculate refresh rate from DClock and HActive, HBlank, VActive and VBlank
// Input: DClock -- (0, 65536) * 100; See detailed timing descriptor part of EDID spec.
//        HActive, HBlank, VActive and VBlank -- (0, 65536)
CBIOS_U16 cbCalcRefreshRate(CBIOS_U32 PixelClock, CBIOS_U16 HActive, CBIOS_U16 HBlank, CBIOS_U16 VActive, CBIOS_U16 VBlank)
{
    CBIOS_U32    Temp = PixelClock;

    if (((HActive + HBlank) == 0) ||
        ((VActive + VBlank) == 0))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbCalcRefreshRate: fatal error, divisor is zero!\n"));
        return 0;
    }
    else
    {
        Temp = (Temp * 100) / (HActive + HBlank);
        Temp = (Temp * 100) / (VActive + VBlank);
    }

    return (CBIOS_U16)Temp;
}

CBIOS_U32 cbGetHwDacMode(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_FORMAT Format)
{
    CBIOS_U32   dwDacMode = 0;

    switch(Format)
    {
    case  CBIOS_FMT_P8:
        dwDacMode = 0;
        break;
    case   CBIOS_FMT_R5G6B5:
    case   CBIOS_FMT_A1R5G5B5:
        dwDacMode = 1;
        break;
    case   CBIOS_FMT_A8R8G8B8:
    case   CBIOS_FMT_A8B8G8R8:
    case   CBIOS_FMT_X8R8G8B8:
    case   CBIOS_FMT_X8B8G8R8:
        dwDacMode = 4;
        break;
    case   CBIOS_FMT_A2R10G10B10:
    case   CBIOS_FMT_A2B10G10R10:
        dwDacMode =  5;
        break;
    case   CBIOS_FMT_CRYCBY422_16BIT:
    case   CBIOS_FMT_YCRYCB422_16BIT:
        dwDacMode = 2;
        break;
    case   CBIOS_FMT_CRYCBY422_32BIT:
    case   CBIOS_FMT_YCRYCB422_32BIT:
        dwDacMode = 6;
        break;
    case   CBIOS_FMT_YCBCR8888_32BIT:
    case   CBIOS_FMT_CRYCB8888_32BIT:
        dwDacMode = 3;
        break;
    case   CBIOS_FMT_YCBCR2101010_32BIT:
    case   CBIOS_FMT_CRYCB2101010_32BIT:
        dwDacMode = 7;
        break;
    default:
        dwDacMode = 4;
        break;
    }

    return  dwDacMode;
}

CBIOS_U32 cbGetHw3DVideoMode(CBIOS_3D_STRUCTURE VideoFormat)
{
    CBIOS_U32   Hw3DVideoMode = 0;

    switch(VideoFormat)
    {
    case FRAME_PACKING:
        Hw3DVideoMode = 5;
        break;
    case L_DEPTH:
        Hw3DVideoMode = 5;
        break;
    case L_DEPTH_GRAPHICS:
        Hw3DVideoMode = 0;
        break;
    case LINE_ALTERNATIVE:
        Hw3DVideoMode = 6;
        break;
    case SIDE_BY_SIDE_FULL:
        Hw3DVideoMode = 7;
        break;
    case SIDE_BY_SIDE_HALF:
        Hw3DVideoMode = 7;
        break;
    case TOP_AND_BOTTOM:
        Hw3DVideoMode = 4;
        break;
    case FIELD_ALTERNATIVE:
        Hw3DVideoMode = 0;
        break;
    default:
        Hw3DVideoMode = 0;
        break;
    }
    return Hw3DVideoMode;
}

CBIOS_VOID cbGetStreamAttribute(PCBIOS_STREAM_ATTRIBUTE pStreamAttr)
{
    PCBIOS_SURFACE_ATTRIB  pSurfaceAttr = pStreamAttr->pSurfaceAttr;
    PCBIOS_WINDOW_PARA     pSrcWindow = pStreamAttr->pSrcWinPara;
    CBIOS_U32        startX = 0, startY = 0, dwBitCnt = 0;;

    if(pSurfaceAttr->SurfaceFmt == CBIOS_FMT_P8)
    {
        dwBitCnt = 8;
    }
    else if(pSurfaceAttr->SurfaceFmt & CBIOS_STREAM_16BPP_FMTS)
    {
        dwBitCnt = 16;
    }
    else
    {
        dwBitCnt = 32;
    }

    if(pSrcWindow) //for flip with disable case, no srcWindow para
    {
        startX = pSrcWindow->Position & 0xFFFF;
        startY = (pSrcWindow->Position >> 16) & 0xFFFF;
    }

    pStreamAttr->dwStride = (pSurfaceAttr->Pitch)>>5;

    pStreamAttr->dwBaseOffset = startY * pSurfaceAttr->Pitch + startX * dwBitCnt/8;
    pStreamAttr->PixelOffset = (pStreamAttr->dwBaseOffset & 0x1f)*8/dwBitCnt;

    pStreamAttr->dwBaseOffset &= ~0x1f;
}

CBIOS_VOID  cbMulti_Matrix_CSC(CBIOS_S64 Multi_A[][3],CBIOS_S64 Multi_B[][3],CBIOS_S64 Multi_C[][3])
{
    CBIOS_U32 i,j,k;
    CBIOS_S64 data;

    for(i=0; i < 3; i++)
    {
        for(j=0; j < 3; j++)
        {
            data = 0;
            for(k = 0; k < 3; k++)
            {
                data += Multi_A[i][k] * Multi_B[k][j];
            }
            Multi_C[i][j] = data;
        }
    }
}

CBIOS_U32 cbTran_CSCm_to_coef_fx(CBIOS_S64 coefX)
{
    CBIOS_U32 Value = 0;
    CBIOS_U32 flag = 0;

    if(coefX < 0)
    {
        flag = 1;
        coefX = 0 - coefX;
    }

    Value = coefX>>(CSC_TOTAL_MAXTRIX_SHIFT_BITS - 9);

    if(flag == 1) //get negative value's 2's complement
    {
        Value = (CBIOS_U32)(1<<CSC_COEF_MODULE) - Value;
    }
    return Value;
}

CBIOS_BOOL  cbGetCscCoefMatrix(PCBIOS_CSC_ADJUST_PARA pAdajust,CBIOS_U32 informat, CBIOS_U32 outformat,CBIOS_S64 CSCm[][3])
{
    CBIOS_S64           ADJm[3][3];
    CBIOS_S64           HUEm[3][3];
    CBIOS_S64           STRm[3][3];
    CBIOS_S64           CNTm[3][3];
    CBIOS_S64           temp_matrix[3][3];
    CBIOS_U32           i,j;
    CBIOS_U32           CscMode;

    cb_memset((PCBIOS_VOID)(ADJm), 0, sizeof(ADJm));
    cb_memset((PCBIOS_VOID)(HUEm), 0, sizeof(HUEm));
    cb_memset((PCBIOS_VOID)(STRm), 0, sizeof(STRm));
    cb_memset((PCBIOS_VOID)(CNTm), 0, sizeof(CNTm));
    cb_memset((PCBIOS_VOID)(temp_matrix), 0, sizeof(temp_matrix));

    // step 1 ,get ADJm
    if(pAdajust->Contrast  > CSC_MAX_CONTRAST * CSC_CONTRAST_MATRIX_EXPAND_TIMES)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"CSC Adjust illagal Constract value,!\n"));
        return CBIOS_FALSE;
    }
    if(pAdajust->Saturation > CSC_MAX_SATURATION * CSC_SATURATION_MATRIX_EXPAND_TIMES)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"CSC Adjust illagal Saturation value,!\n"));
        return CBIOS_FALSE;
    }
    if(pAdajust->Hue > CSC_MAX_HUE || pAdajust->Hue < CSC_MIN_HUE) //expand cosHue,not Hue itself.
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"CSC Adjust illagal Hue value %d,!\n",pAdajust->Hue));
        return CBIOS_FALSE;
    }
    //these Matrix is define by CSC spec.
    //set Saturation Matrix,to make sure precision,expand CSC_SATURATION_MATRIX_EXPAND_TIMES times
    STRm[0][0] = CSC_SATURATION_MATRIX_EXPAND_TIMES;
    STRm[1][1] = pAdajust->Saturation;
    STRm[2][2] = pAdajust->Saturation;

    // set Contrast Matrixs,to make sure precision,expand CSC_CONTRAST_MATRIX_EXPAND_TIMES times
    CNTm[0][0] = pAdajust->Contrast;
    CNTm[1][1] = pAdajust->Contrast;
    CNTm[2][2] = pAdajust->Contrast;

    //set Hue Matrixs
    //make sure precision,expand CSC_HUE_MATRIX_EXPAND_TIMES times
    //map cos/sin (-180',180') to  cos [0-360],then we can get cosX by cos[X+180]. X unit is degree
    HUEm[0][0] =  CSC_HUE_MATRIX_EXPAND_TIMES;
    HUEm[1][1] =  CSC_cos[pAdajust->Hue + 180];
    HUEm[1][2] = -CSC_sin[pAdajust->Hue + 180];
    HUEm[2][1] =  CSC_sin[pAdajust->Hue + 180];
    HUEm[2][2] =  CSC_cos[pAdajust->Hue + 180];

   //Get ADJm = HUEm * STRm * CNTm
    ADJm[0][0] = HUEm[0][0] * STRm[0][0] * CNTm[0][0];
    ADJm[1][1] = HUEm[1][1] * STRm[1][1] * CNTm[1][1];
    ADJm[1][2] = HUEm[1][2] * STRm[2][2] * CNTm[2][2];
    ADJm[2][1] = HUEm[2][1] * STRm[1][1] * CNTm[1][1];
    ADJm[2][2] = HUEm[2][2] * STRm[2][2] * CNTm[2][2];

    // step 2,get CSCm
    // construct hash value CscMode use 7*informat + outformat as hash formula
    // then we can choose proper method to get CSC Matrixs directly.
    CscMode = CSC_MODE_HASH_VALUE * informat + outformat;

    switch(CscMode)
    {
        case CSC_RGB_TO_RGB:
            //RGB->RGB  use ycbcr709 as intermediary to get CSCm
            cbMulti_Matrix_CSC(ADJm,YCbCr709_fr_RGB,temp_matrix);
           //here,to prevent spillage
            for(i=0;i<3;i++)
            {
                for(j = 0 ;j<3 ;j++)
                {
                    temp_matrix[i][j] = temp_matrix[i][j] >> CSC_YCBCR_RGB_MATRIX_SHIFT_BITS;
                }
            }
            cbMulti_Matrix_CSC(RGB_fr_YCbCr709,temp_matrix,CSCm);
            break;
        case CSC_RGB_TO_YCBCR601:
            cbMulti_Matrix_CSC(ADJm,YCbCr601_fr_RGB,CSCm);
            break;
        case CSC_RGB_TO_YCBCR709:
            cbMulti_Matrix_CSC(ADJm,YCbCr709_fr_RGB,CSCm);
            break;
        case CSC_YCBCR601_TO_RGB:
            cbMulti_Matrix_CSC(RGB_fr_YCbCr601,ADJm,CSCm);
            break;
        case CSC_YCBCR601_TO_YCBCR601:
            cbMulti_Matrix_CSC(ADJm,Balance_Times_matrix,CSCm);
            break;
        case CSC_YCBCR601_TO_YCBCR709:
            cbMulti_Matrix_CSC(YCbCr709_fr_YCbCr601,ADJm,CSCm);
            cbMulti_Matrix_CSC(ADJm,YCbCr709_fr_YCbCr601,CSCm);
            break;
        case CSC_YCBCR709_TO_RGB:
            cbMulti_Matrix_CSC(RGB_fr_YCbCr709,ADJm,CSCm);
            break;
        case CSC_YCBCR709_TO_YCBCR601:
            cbMulti_Matrix_CSC(YCbCr601_fr_YCbCr709,ADJm,CSCm);
            cbMulti_Matrix_CSC(ADJm,YCbCr601_fr_YCbCr709,CSCm);
            break;
        case CSC_YCBCR709_TO_YCBCR709:
            cbMulti_Matrix_CSC(ADJm,Balance_Times_matrix,CSCm);
            break;
        default :
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"CSC don't support informat %d to outformat %d  with Adjust yet!\n",informat,outformat));
            return CBIOS_FALSE;
            break;
        }
    return CBIOS_TRUE;
}

CBIOS_U32 cbGetHwColorKey(PCBIOS_OVERLAY_INFO pOverlayInfo, PCBIOS_U8 pKs, PCBIOS_U8 pKp)
{
    CBIOS_U32   dwKeyMode = 0;
    switch(pOverlayInfo->KeyMode)
    {
    case CBIOS_WINDOW_KEY:
        {
            dwKeyMode  = 0;
            *pKs = pOverlayInfo->WindowKey.Kb;
            *pKp = pOverlayInfo->WindowKey.Ka;
        }
        break;
    case CBIOS_ALPHA_KEY:
        {
            dwKeyMode = (pOverlayInfo->AlphaKey.bAOverB)? 5 : 1;
            *pKs = pOverlayInfo->AlphaKey.Kb;
            *pKp = pOverlayInfo->AlphaKey.Ka;
        }
        break;
    case CBIOS_COLOR_KEY:
        {
             dwKeyMode = (pOverlayInfo->ColorKey.bAOverB)? 6 : 2;
             // Keep Kp1=0 and Ks1=8.
             *pKs = (pOverlayInfo->ColorKey.bAOverB)? 0x00 : 0x08;
             *pKp = (pOverlayInfo->ColorKey.bAOverB)? 0x08 : 0x00;
        }
        break;
    case CBIOS_ALPHA_BLENDING:
        {
            dwKeyMode = (pOverlayInfo->AlphaBlending.bUseAAlpha)? 8 : 12;
            *pKs = 0;
            *pKp = 0;
        }
        break;
    case CBIOS_CONSTANT_ALPHA:
        {
            dwKeyMode = 9;
            *pKs = pOverlayInfo->ConstantAlphaBlending.ConstantAlpha & 0x0F;
            *pKp = (pOverlayInfo->ConstantAlphaBlending.ConstantAlpha >> 4) & 0x0F;
        }
        break;
    case CBIOS_CHROMA_KEY:
        {
            dwKeyMode = 7;
            *pKs = 0;
            *pKp = 0x08;
        }
        break;
    case CBIOS_INVALID_KEYING_MODE:
         break;
    default:
         break;
    }
    return dwKeyMode;
}


CBIOS_BOOL cbCECAllocateLogicalAddr(PCBIOS_VOID pvcbe, CBIOS_CEC_INDEX CECIndex)
{
    CBIOS_CEC_MESSAGE CECMessage;
    CBIOS_U8    LogicalAddrTable[3] = {4, 8, 11};
    CBIOS_U8    i;
    CBIOS_BOOL  bGetLogicalAddr = CBIOS_FALSE;
    CBIOS_U8    LogicalAddr = CEC_UNREGISTERED_DEVICE;
    CBIOS_BOOL  bStaus = CBIOS_FALSE;
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;

    //try previous address first
    if (pcbe->CECPara[CECIndex].LogicalAddr != CEC_UNREGISTERED_DEVICE)
    {
        cb_memset(&CECMessage, 0, sizeof(CBIOS_CEC_MESSAGE));
        CECMessage.SourceAddr = pcbe->CECPara[CECIndex].LogicalAddr;
        CECMessage.DestAddr = pcbe->CECPara[CECIndex].LogicalAddr;
        CECMessage.bBroadcast = CBIOS_FALSE;
        CECMessage.RetryCnt = 5;
        CECMessage.CmdLen = 0;
        if (CBIOS_OK != cbHWCECTransmitMessage(pcbe, &CECMessage, CECIndex))
        {
            bGetLogicalAddr = CBIOS_TRUE;
            LogicalAddr = CECMessage.SourceAddr;
        }
    }


    //polling logical address
    if (!bGetLogicalAddr)
    {
        for (i = 0; i < sizeofarray(LogicalAddrTable); i++)
        {
            cb_memset(&CECMessage, 0, sizeof(CBIOS_CEC_MESSAGE));
            CECMessage.SourceAddr = LogicalAddrTable[i];
            CECMessage.DestAddr = LogicalAddrTable[i];
            CECMessage.bBroadcast = CBIOS_FALSE;
            CECMessage.RetryCnt = 5;
            CECMessage.CmdLen = 0;
            if (CBIOS_OK != cbHWCECTransmitMessage(pcbe, &CECMessage, CECIndex))
            {
                bGetLogicalAddr = CBIOS_TRUE;
                LogicalAddr = CECMessage.SourceAddr;
                break;
            }
        }

    }
    if (bGetLogicalAddr)
    {
        //broadcast physical address
        cb_memset(&CECMessage, 0, sizeof(CBIOS_CEC_MESSAGE));
        CECMessage.SourceAddr = LogicalAddr;
        CECMessage.DestAddr = CEC_BROADCAST_ADDRESS;
        CECMessage.bBroadcast = CBIOS_TRUE;
        CECMessage.RetryCnt = 5;
        CECMessage.CmdLen = 4;
        //parameter 1: opcode
        CECMessage.Command[0] = CEC_REPORT_PHYSCAL_ADDR;
        //parameter 2-3: physical address MSB
        CECMessage.Command[1] = (CBIOS_U8)((pcbe->CECPara[CECIndex].PhysicalAddr >> 8)& 0xFF);
        CECMessage.Command[2] = (CBIOS_U8)(pcbe->CECPara[CECIndex].PhysicalAddr & 0xFF);
        //parameter 4: device type
        CECMessage.Command[3] = 4;  //playback device

        if (CBIOS_OK == cbHWCECTransmitMessage(pcbe, &CECMessage, CECIndex))
        {
            bStaus = CBIOS_TRUE;

        }
        else
        {
            bStaus = CBIOS_FALSE;
        }


    }

    if (bStaus)
    {
        pcbe->CECPara[CECIndex].LogicalAddr = CECMessage.SourceAddr;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "cbCECAllocateLogicalAddr: allocate logical address fail!\n"));
        pcbe->CECPara[CECIndex].LogicalAddr = CEC_UNREGISTERED_DEVICE;
    }

    return bStaus;


}


//Convert Device Word to Device Index
CBIOS_U32 cbConvertDeviceBit2Index(CBIOS_U32 DeviceBit)
{
    CBIOS_U32 DevMask = 0x00000001;
    CBIOS_U32 i;

    if(cbGetBitsNum(DeviceBit) != 1)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbConvertDeviceBit2Index: Device bit error!\n"));
        ASSERT(CBIOS_FALSE);
    }

    for(i=0; i<CBIOS_MAX_DISPLAY_DEVICES_NUM; i++)
    {
        if(DeviceBit & DevMask)
            break;

        DevMask <<= 1;
    }

    return i;
}


CBIOS_U8 cbGetBitsNum(CBIOS_U32 N)
{
    CBIOS_U8 Num;

    for(Num=0; N!=0; Num++)
    {
        N &= ~(GET_LAST_BIT(N));
    }

    return Num;
}


CBIOS_U32 cbGetLastBitIndex(CBIOS_U32 i)
{
    CBIOS_U32 index = i;
    index = (index-1) & ~index;
    index = (index&0x55555555) + ((index>>1)&0x55555555);
    index = (index&0x33333333)+ ((index>>2)&0x33333333);
    index = (index&0x0F0F0F0F)+ ((index>>4)&0x0F0F0F0F);
    index = (index&0xFF) + ((index&0xFF00) >> 8) + ((index&0xFF0000) >> 16) + ((index&0xFF000000) >> 24);
    return index;
}


CBIOS_VOID cbDumpBuffer(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U8 Buffer[], CBIOS_U32 ulLen)
{
    CBIOS_U32   i;
    CBIOS_U8    ucChecksum = 0;

    cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG),"     00, 01, 02, 03, 04, 05, 06, 07, 08, 09, 0a, 0b, 0c, 0d, 0e, 0f\n\n"));
    for (i = 0; (i + 1) * 16 <= ulLen; i++)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG),"%02x:  %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\n", i,\
            Buffer[i*16+0x00], Buffer[i*16+0x01], Buffer[i*16+0x02], Buffer[i*16+0x03],\
            Buffer[i*16+0x04], Buffer[i*16+0x05], Buffer[i*16+0x06], Buffer[i*16+0x07],\
            Buffer[i*16+0x08], Buffer[i*16+0x09], Buffer[i*16+0x0a], Buffer[i*16+0x0b],\
            Buffer[i*16+0x0c], Buffer[i*16+0x0d], Buffer[i*16+0x0e], Buffer[i*16+0x0f]));
    }

    for (i = 0; i < ulLen; i++)
    {
        ucChecksum += Buffer[i];
    }

    cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "cbDumpBuffer: checksum == 0x%02x\n", ucChecksum));
}


CBIOS_BOOL cbIsSameMonitor(CBIOS_U8  *pCurDeviceEDID, CBIOS_U8 *pMonitorID)
{
    CBIOS_BOOL bRet = CBIOS_FALSE;

    if ((pCurDeviceEDID == CBIOS_NULL) || (pMonitorID == CBIOS_NULL))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbIsSameMonitor: Null pointer!\n" ));
        ASSERT(CBIOS_FALSE);
        bRet = CBIOS_FALSE;
        return bRet;
    }

    if (!cb_strncmp(&pCurDeviceEDID[MONITORIDINDEX], pMonitorID, MONITORIDLENGTH))
    {
        bRet = CBIOS_TRUE;
    }
    else
    {
        bRet = CBIOS_FALSE;
    }

    return bRet;
}

static CBIOS_BOOL cbU8ToString(CBIOS_U8 Src, CBIOS_U8* buf)
{
    CBIOS_U8 hb;
    CBIOS_U8 lb;

    hb=(Src&0xf0)>>4;
    if( hb>=0 && hb<=9 )
        hb += 0x30;
    else if( hb>=10 &&hb <=15 )
        hb = hb -10 + 'A';
    else
        return CBIOS_FALSE;

    lb = Src&0x0f;
    if( lb>=0 && lb<=9 )
        lb += 0x30;
    else if( lb>=10 && lb<=15 )
        lb = lb - 10 + 'A';
    else
        return CBIOS_FALSE;

    buf[0] = hb;
    buf[1] = lb;

    return CBIOS_TRUE;
}


static CBIOS_BOOL cbHex2String(CBIOS_U8 *pSrc, CBIOS_U8* buf, CBIOS_U32 nL)
{
    CBIOS_U8 Str[2];
    CBIOS_U32 i = 0;

    for(i=0;i<nL;i++)
    {
        cbU8ToString(pSrc[i], Str);
        buf[i*4] = Str[0];
        buf[i*4+1] = Str[1];
        buf[i*4+2] = ' ';
        buf[i*4+3] = ' ';

        if(0 ==(i+1)%16 )
        {
            buf[i*4+3] = '\n';
        }
    }

    buf[i*4-1] = '\0';

    return CBIOS_TRUE;
}


static CBIOS_BOOL cbHexDW2String(CBIOS_U32 *pSrc, CBIOS_U8* buf, CBIOS_U32 nL)
{
    CBIOS_U32 i = 0;
    CBIOS_U8 Str[2];

    for(i=0; i<nL; i++)
    {
        cbU8ToString((CBIOS_U8)( (pSrc[i]&0xff000000) >> 24), Str);
        buf[i*10]= Str[0];
        buf[i*10+1]= Str[1];
        cbU8ToString((CBIOS_U8)( (pSrc[i]&0x00ff0000) >> 16), Str);
        buf[i*10+2]= Str[0];
        buf[i*10+3]= Str[1];
        cbU8ToString((CBIOS_U8)( (pSrc[i]&0x0000ff00) >> 8), Str);
        buf[i*10+4]= Str[0];
        buf[i*10+5]= Str[1];
        cbU8ToString((CBIOS_U8)( pSrc[i]&0x000000ff), Str);
        buf[i*10+6]= Str[0];
        buf[i*10+7]= Str[1];
        buf[i*10+8] = ' ';
        buf[i*10+9] = ' ';

        if(0 == (i+1)%4)
        {
            buf[i*10+9] = '\n';
        }
    }

    buf[i*10-1] = '\0';

    return CBIOS_TRUE;
}

CBIOS_BOOL cbPrintU8String(CBIOS_U8 *Src,CBIOS_U32 Len,CBIOS_U16 Start)
{
    CBIOS_UCHAR Buf[72];
    CBIOS_U32 n = Len / 16;
    CBIOS_U32 m = Len % 16;
    CBIOS_U32 nL = 0;

    for(nL = 0;nL < n;nL++)
    {
        cb_memset(Buf,0,72);
        cbHex2String(&Src[nL*16], Buf,16);
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "[0x%04x]: %s\n",Start,Buf));
        Start += 16;
    }

    if(m)
    {
        cb_memset(Buf,0,72);
        cbHex2String(&Src[nL*16], Buf,m);
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "[0x%04x]: %s\n",Start,Buf));
    }

    return CBIOS_TRUE;
}

CBIOS_BOOL cbPrintU32String(CBIOS_U32 *Src,CBIOS_U32 Len,CBIOS_U16 Start)
{
    CBIOS_UCHAR Buf[56];
    CBIOS_U32 n = Len / 4;
    CBIOS_U32 m = Len % 4;
    CBIOS_U32 nL = 0;

    for(nL = 0;nL < n;nL++)
    {
        cb_memset(Buf,0,56);
        cbHexDW2String(&Src[nL*4], Buf,4);
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "[0x%04x]: %s\n",Start,Buf));
        Start += 16;
    }

    if(m)
    {
        cb_memset(Buf,0,56);
        cbHexDW2String(&Src[nL*4], Buf,m);
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "[0x%04x]: %s\n",Start,Buf));
    }

    return CBIOS_TRUE;
}


//--------------------------------------------------------------------------
//  cbDbgPrintNull
//      This function do nothing but just return, for prevent driver
//  pass null function pointer to CBIOS to use
//--------------------------------------------------------------------------
CBIOS_VOID cbDbgPrintNull(CBIOS_U32 DebugPrintLevel, PCBIOS_CHAR DebugMessage, ...)
{
    return;
}

CBIOS_BOOL cbGetFakeEdidFlag(PCBIOS_VOID pvcbe)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32 FakeEdidFlag = CBIOS_TRUE;

    if (cbGetPlatformConfigurationU32(pcbe, (CBIOS_UCHAR*)"fake_hdmi_edid", &FakeEdidFlag, 1))
    {
        if ((FakeEdidFlag != 0) && (FakeEdidFlag != 1))
        {
            FakeEdidFlag = CBIOS_TRUE;
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "Invalid fake edid flag, fake edid by default!\n"));
        }
        else
        {
            if(FakeEdidFlag)
            {
                cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "Use fake edid\n"));
            }
            else
            {
                cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "Don't use fake edid\n"));
            }
        }
    }
    else
    {
        FakeEdidFlag = CBIOS_TRUE;
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "Can't get hdmi fake edid flag, fake edid by default\n"));
    }

    return (FakeEdidFlag)? CBIOS_TRUE : CBIOS_FALSE;
}
