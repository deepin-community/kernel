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
#include "../../Hw/HwBlock/CBiosDIU_HDMI.h"
#include "../../Hw/HwBlock/CBiosDIU_HDAC.h"
#include "../../Hw/HwBlock/CBiosDIU_HDCP.h"
#include "../../Hw/CBiosHwShare.h"

 static CBIOS_U8 cbHDMIMonitor_HDACGetCAValue(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_ACTIVE_TYPE DeviceType)
 {
     CBIOS_U8 CA_Value = 0;
     PCBIOS_DEVICE_COMMON pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, DeviceType);
     PCBIOS_MONITOR_MISC_ATTRIB pMonitorAttrib = &(pDevCommon->EdidStruct.Attribute);

     //pMonitorAttrib->SpeakerAllocationData
     //  bit |  0  |  1  |  2  |  3  |  4  |  5  |  6
     //-------------------------------------------------
     //      | FLR | LFE | FC  | RLR | RC  | FLRC| RLRC

     if(pMonitorAttrib->IsCEA861Monitor)
     {
         // refer to CEA-861D table 20
         if(pMonitorAttrib->SpeakerAllocationData & 0x02)    // Lower Frequency Effect
             CA_Value |= BIT0;

         if(pMonitorAttrib->SpeakerAllocationData & 0x04)     // Front Center
             CA_Value |= BIT1;

         if((pMonitorAttrib->SpeakerAllocationData & 0x60) == 0)
         {
             if(pMonitorAttrib->SpeakerAllocationData & 0x10)     // Rear Center
                 CA_Value |= BIT2;

             if(pMonitorAttrib->SpeakerAllocationData & 0x08)    // Rear Left/Rear Right
                 CA_Value |= BIT3;
         }

         if((pMonitorAttrib->SpeakerAllocationData & 0x40) && (pMonitorAttrib->SpeakerAllocationData & 0x08))
             CA_Value |= BIT4;

         if(pMonitorAttrib->SpeakerAllocationData & 0x20)   // Front Left Center/Front Right Center
         {
             CA_Value |= BIT4;
             if(pMonitorAttrib->SpeakerAllocationData & 0x10)
                 CA_Value |= BIT3;

             if(pMonitorAttrib->SpeakerAllocationData & 0x08)
                 CA_Value |= (BIT2 | BIT3);
         }
     }

     return CA_Value;
 }

static CBIOS_VOID cbHDMIMonitor_GenerateAVIInfoFrameData(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DISP_MODE_PARAMS pModeParams, PCBIOS_AVI_INFO_FRAME_DATA pAVIInfoFrameData)
{
    CBIOS_U8    VICCode = pModeParams->VICCode;
    PCBIOS_HDMI_FORMAT_MTX  pCurrentFormat = CBIOS_NULL;
    CBIOS_BOOL  bIsCEAMode = CBIOS_FALSE;

    cb_memset(pAVIInfoFrameData, 0, sizeof(CBIOS_AVI_INFO_FRAME_DATA));

    if ((VICCode > 0) && (VICCode <= CBIOS_HDMIFORMATCOUNTS))
    {
        bIsCEAMode = CBIOS_TRUE;
        pCurrentFormat = &(pcbe->pHDMIFormatTable[VICCode - 1]);
    }
    else
    {
        bIsCEAMode = CBIOS_FALSE;
    }


    //See CEA-861-F Table 9
    // S1  S0   |   Scan Information
    // -----------------------------
    //  0   0   |   No Data
    //  0   1   |   Overscan
    //  1   0   |   Underscan
    //  1   1   |   Reserved
    pAVIInfoFrameData->ScanInfo = 2;    //hardcode to underscan


    // B1   B0  |   Bar Info
    // ---------------------------------
    //  0   0   |   Bar data not valid
    //  0   1   |   Vert. Bar info valid
    //  1   0   |   Hor. Bar Info valid
    //  1   1   |   V & H Bar Info valid
    pAVIInfoFrameData->BarInfo = 0;     //Bar data not valid


    //aspect ratio related

    // M1  M0   |   Picture Aspect Ratio
    // ---------------------------------
    //  0   0   |   No Data
    //  0   1   |    4:3
    //  1   0   |   16:9
    //  1   1   |   Reserved
    if ((bIsCEAMode) && (pCurrentFormat->AspectRatio >= 0) && (pCurrentFormat->AspectRatio <= 2))
    {
        pAVIInfoFrameData->PictureAspectRatio = pCurrentFormat->AspectRatio;
    }
    else
    {
        pAVIInfoFrameData->PictureAspectRatio = 0;
    }

    //  A0  |   Active Format Info Present
    //------------------------------------
    //  0   |   No Data
    //  1   |   R0...R3 valid
    pAVIInfoFrameData->ActFormatInfoPreset = 0;    //active format info not valid

    //Effective only when ActFormatInfoPreset = 1
    //  R3  R2  R1  R0  |   Active Format Aspect Ratio
    //-------------------------------------------------
    //  1   0   0   0   |   Same as picture aspect ratio
    //  1   0   0   1   |    4:3 (Center)
    //  1   0   1   0   |   16:9 (Center)
    //  1   0   1   1   |   14:9 (Center)
    //  Other Values    |   Per DVB AFD active_format
    //                  |   field in ETSI
    pAVIInfoFrameData->ActFormatAspectRatio = 0x08; //Same as picture aspect ratio


    //Color format related

    // Y2  Y1  Y0   |   RGB or YCbCr
    // --------------------------
    //  0   0   0   |   RGB
    //  0   0   1   |   YCbCr 4:2:2
    //  0   1   0   |   YCbCr 4:4:4
    //  0   1   1   |   YCbCr 4:2:0
    //  1   0   0   |   Reserved
    //  1   0   1   |   Reserved
    //  1   1   0   |   Reserved
    //  1   1   1   |   IDO-Defined
    if (pModeParams->TargetModePara.OutputSignal == CBIOS_YCBCR422OUTPUT)
    {
        pAVIInfoFrameData->ColorFormat = 1;
    }
    else if (pModeParams->TargetModePara.OutputSignal == CBIOS_YCBCR444OUTPUT)
    {
        pAVIInfoFrameData->ColorFormat = 2;
    }
    else if (pModeParams->TargetModePara.OutputSignal == CBIOS_YCBCR420OUTPUT)
    {
        pAVIInfoFrameData->ColorFormat = 3;
    }
    else
    {
        pAVIInfoFrameData->ColorFormat = 0;//set to RGB by default
    }

    //      C1  C0  |   Colorimetry
    //-----------------------------------
    //      0   0   |   No Data
    //      0   1   |   ITU-601
    //      1   0   |   ITU-709
    //      1   1   |   Ext colorimetry valid
    //              |   See EC2...EC0

    //  EC2 EC1 EC0 |   Extended Colorimetry
    //-------------------------------------------
    //   0   0   0  |   xvYCC601
    //   0   0   1  |   xvYCC709
    //   0   1   0  |   sYCC601
    //   0   1   1  |   AdobeYCC601
    //   1   0   0  |   AdobeRGB
    //   1   0   1  |   ITU-R BT.2020 Y'cC'bcC'rc
    //   1   1   0  |   ITU-R BT.2020 R'G'B' or Y'C'bC'r
    //   1   1   1  |   Reserved

    if(bIsCEAMode && VICCode > 1)
    {
        if(pAVIInfoFrameData->ColorFormat == 0)//RGB
        {
            if((pModeParams->IsAdobe) && pModeParams->ColorimetryCaps.IsSupportAdobeRGB)
            {
                pAVIInfoFrameData->Colorimetry = 0x03;  //use extended colorimetry
                pAVIInfoFrameData->ExtendedColorimetry = 0x04;  //AdobeRGB
            }
            else if((pModeParams->IsBT2020) && pModeParams->ColorimetryCaps.IsSupportBT2020RGB)
            {
                pAVIInfoFrameData->Colorimetry = 0x03;  //use extended colorimetry
                pAVIInfoFrameData->ExtendedColorimetry = 0x06;  //BT2020RGB
            }
            else
            {
                pAVIInfoFrameData->Colorimetry = 0;
                pAVIInfoFrameData->ExtendedColorimetry = 0;
            }
        }
        else if(((pAVIInfoFrameData->ColorFormat == 1) ||    //YCbCr422
                (pAVIInfoFrameData->ColorFormat == 2)  ||    //YCbCr444
                (pAVIInfoFrameData->ColorFormat == 3)))      //YCbCr420
        {
            if(pModeParams->IsxvYCC)
            {
                if(pCurrentFormat->YRes < 720 && pModeParams->ColorimetryCaps.IsSupportxvYCC601) //SDTV, use BT601
                {
                    pAVIInfoFrameData->Colorimetry = 0x03;
                    pAVIInfoFrameData->ExtendedColorimetry = 0x00;  //xvYCC601
                }
                else if(pCurrentFormat->YRes >= 720 && pModeParams->ColorimetryCaps.IsSupportxvYCC709)
                {
                    pAVIInfoFrameData->Colorimetry = 0x03;
                    pAVIInfoFrameData->ExtendedColorimetry = 0x01;  //xvYCC709
                }
                else
                {
                    pAVIInfoFrameData->Colorimetry = 0;
                    pAVIInfoFrameData->ExtendedColorimetry = 0;
                }
            }
            else if(pModeParams->IsAdobe)
            {
                if(pModeParams->ColorimetryCaps.IsSupportAdobeYCC601)
                {
                    pAVIInfoFrameData->Colorimetry = 0x03;
                    pAVIInfoFrameData->ExtendedColorimetry = 0x03;  //AdobeYCC601
                }
                else
                {
                    pAVIInfoFrameData->Colorimetry = 0;
                    pAVIInfoFrameData->ExtendedColorimetry = 0;
                }
            }
            else if((pModeParams->IsBT2020) && pModeParams->ColorimetryCaps.IsSupportBT2020YCC)
            {
                pAVIInfoFrameData->Colorimetry = 0x03;
                pAVIInfoFrameData->ExtendedColorimetry = 0x06;  //BT2020Y'C'bC'r
            }
            else
            {
                pAVIInfoFrameData->Colorimetry = 0;
                pAVIInfoFrameData->ExtendedColorimetry = 0;
            }
        }
        else
        {
            pAVIInfoFrameData->Colorimetry = 0;
            pAVIInfoFrameData->ExtendedColorimetry = 0;
        }

        pAVIInfoFrameData->RGBQuantRange = 1;   //RGB range is limited during CEA mode
    }
    else
    {
        //Colorimetry = 0: use default Colorimetry
        //720 and above: BT709
        //576 and below: BT601
        pAVIInfoFrameData->Colorimetry = 0;
        pAVIInfoFrameData->ExtendedColorimetry = 0;

        //Quant range
        //  Q1  Q0  |   RGB Quant Range
        //------------------------------
        //  0   0   |   Default
        //  0   1   |   Limited range
        //  1   0   |   Full Range
        //  1   1   |   Reserved
        pAVIInfoFrameData->RGBQuantRange = 0;   //depends on video format
    }

    //If a Sink declares a selectable RGB Quantization Range (QS=1) in EDID VCDB
    //It shall expect Limited Range values if it receives Q=1 and it shall expect Full Range values if it receives Q=2
    //Here,we set Q=2 to send Full Range pixel values
    if(pModeParams->VideoCapability.bRGBQuantRange)
    {
        pAVIInfoFrameData->RGBQuantRange = 2;
    }

    //hardcode here
    //  SC1 SC0 |   Non-Uniform Picture Scaling
    //------------------------------------------
    //  0   0   |   No known non-uniform scaling
    //  0   1   |   Scaled horizontally
    //  1   0   |   Scaled vertically
    //  1   1   |   Scaled both H & V
    pAVIInfoFrameData->NonUniformScaling = 0;


    //  ITC |   IT Content
    //--------------------
    //   0  |   No Data
    //   1  |   IT Content
    pAVIInfoFrameData->ITContent = 1;

    //  ITC |  CN1  CN0 |   Content Type
    //-------------------------------------
    //  0   |   0    0  |   No Data
    //  1   |   0    0  |   Graphics
    //  X   |   0    1  |   Photo
    //  X   |   1    0  |   Cinema
    //  X   |   1    1  |   Game
    //"X" denotes don't care.
    pAVIInfoFrameData->ContentType = 0; //set to graphics by default


    //VIC
    if ((VICCode > 0) && (VICCode <= CBIOS_HDMI_NORMAL_VIC_COUNTS))
    {
        pAVIInfoFrameData->VICCode = VICCode;
    }
    else if(pModeParams->IsEnable3DVideo)//extended formats and 3D video, the source shall set AVI infoframe VIC with the corresponding "Equivalent CEA-861-F VIC"
    {
        if((VICCode - CBIOS_HDMI_NORMAL_VIC_COUNTS) == 1)
        {
            pAVIInfoFrameData->VICCode = 95;
        }
        else if((VICCode - CBIOS_HDMI_NORMAL_VIC_COUNTS) == 2)
        {
            pAVIInfoFrameData->VICCode = 94;
        }
        else if((VICCode - CBIOS_HDMI_NORMAL_VIC_COUNTS) == 3)
        {
            pAVIInfoFrameData->VICCode = 93;
        }
        else if((VICCode - CBIOS_HDMI_NORMAL_VIC_COUNTS) == 4)
        {
            pAVIInfoFrameData->VICCode = 98;
        }
    }
    else//none-CEA mode or extended formats, set VIC to 0 here, extended VIC will be programmed in VSIF
    {
        pAVIInfoFrameData->VICCode = 0;
    }

    //pixel repetition factor
    //now we only support repeat once
    //  PR3 PR2 PR1 PR0 |   Pixel Repeat Factor
    //------------------------------------------
    //   0   0   0   0  |   No Repetition
    //   Other values   |   Repeat times
    if (bIsCEAMode)
    {
        pAVIInfoFrameData->PixelRepeatFactor = pModeParams->PixelRepitition - 1;
    }
    else
    {
        pAVIInfoFrameData->PixelRepeatFactor = 0;//no repeat
    }


    //  YQ1 YQ0 |   YCbCr Quant Range
    //----------------------------------
    //   0   0  |   Limited Range
    //   0   1  |   Full Range
    //   Other  |   Reserved
    pAVIInfoFrameData->YCCQuantRange = 0;
}

//Test only
CBIOS_U8 GBDRangeData[6] =
{
    0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF
};

static CBIOS_VOID cbHDMIMonitor_GenerateInfoFrameData(PCBIOS_EXTENSION_COMMON pcbe,
                                                      PCBIOS_HDMI_MONITOR_CONTEXT pHDMIMonitorContext,
                                                      PCBIOS_DISP_MODE_PARAMS pModeParams)
{
    PCBIOS_HDMI_INFO_FRAME_DATA pInfoFrameData = &pHDMIMonitorContext->HDMIInfoFrame;
    CBIOS_U32 HBlank = 0, HDMIMaxPacketNum = 2;

    //fill AVI infoframe data bytes
    cbHDMIMonitor_GenerateAVIInfoFrameData(pcbe, pModeParams, &pInfoFrameData->AVIInfoFrameData);

    //fill HF-VSIF when enable 3D OSD Disparity, Dual View or Independent View
    if(pModeParams->IsEnable3DOSDDisparity || pModeParams->IsEnable3DDualView || pModeParams->IsEnable3DIndependentView)
    {
        pInfoFrameData->HFVSIFPara.Valid3D = CBIOS_TRUE;
        pInfoFrameData->HFVSIFPara.Video3DFStruct = pModeParams->Video3DStruct;
        if(pModeParams->IsEnable3DOSDDisparity)
        {
            pInfoFrameData->HFVSIFPara.IsEnable3DOSDDisparity = CBIOS_TRUE;
            cb_memcpy(&pInfoFrameData->HFVSIFPara.DisparityPara, &pModeParams->Video3DDisparity, sizeof(pModeParams->Video3DDisparity));
        }

        if(pModeParams->IsEnable3DDualView)
        {
            pInfoFrameData->HFVSIFPara.IsEnable3DDualView = CBIOS_TRUE;
        }

        if(pModeParams->IsEnable3DIndependentView)
        {
            pInfoFrameData->HFVSIFPara.IsEnable3DIndependentView = CBIOS_TRUE;
            pInfoFrameData->HFVSIFPara.IndependentViewPara.Dependency = pModeParams->Viedo3DIndependentCaps.Dependency;
            pInfoFrameData->HFVSIFPara.IndependentViewPara.Preferred2D = pModeParams->Viedo3DIndependentCaps.Preferred2D;
        }
    }
    else if ((pModeParams->IsEnable3DVideo) ||(pModeParams->VICCode > CBIOS_HDMI_NORMAL_VIC_COUNTS))
    {
        //fill VSIF data when enable 3D video or for extended VICs
        if (pModeParams->IsEnable3DVideo)
        {
            pInfoFrameData->VSIFPara.VSIFVideoFormat = VSIF_3D_FORMAT_INDICATION_PRESENT;
            pInfoFrameData->VSIFPara.VSIF3DFormatPara.Video3DStruct = pModeParams->Video3DStruct;
        }
        else if (pModeParams->VICCode > CBIOS_HDMI_NORMAL_VIC_COUNTS)
        {
            pInfoFrameData->VSIFPara.VSIFVideoFormat = VSIF_EXTENDED_FORMAT_PRESENT;
            pInfoFrameData->VSIFPara.VSIFExtFormatPara.VICCode = pModeParams->VICCode - CBIOS_HDMI_NORMAL_VIC_COUNTS;
        }
    }
    else
    {
        pInfoFrameData->VSIFPara.VSIFVideoFormat = VSIF_NO_ADDITIONAL_PRESENT;
    }

    //xvYCC
    if (pModeParams->IsxvYCC)
    {
        //hardcode here, for test only
        pInfoFrameData->GamutMetadata.FormatFlag = RANGE_GBD_STRUCT;
        pInfoFrameData->GamutMetadata.GBDColorPrecision = 8;

        if (pModeParams->TargetModePara.YRes >= 720)
        {
            pInfoFrameData->GamutMetadata.GBDData.RangeGBD.ColorSpace = GBD_RANGE_COLOR_SPACE_XVYCC_709;
        }
        else
        {
            pInfoFrameData->GamutMetadata.GBDData.RangeGBD.ColorSpace = GBD_RANGE_COLOR_SPACE_XVYCC_601;
        }
        pInfoFrameData->GamutMetadata.GBDData.RangeGBD.pRangeData = GBDRangeData;
        pInfoFrameData->GamutMetadata.GBDData.RangeGBD.RangeDataSize = sizeof(GBDRangeData);
    }

    // Calculate MAX packets in H balnk
    HBlank = pModeParams->TargetTiming.HorBEnd - pModeParams->TargetTiming.HorBStart;
    HBlank *= pModeParams->PixelRepitition;

    HDMIMaxPacketNum = (HBlank - HDMI_DELAY_FOR_HDCP - HDMI_LEADING_GUARD_BAND_PERIOD -
                        HDMI_MIN_CTL_PERIOD - HDMI_TRAILING_GUARD_BAND_PERIOD) / 32;
    pInfoFrameData->HDMIMaxPacketNum = cb_min(HDMIMaxPacketNum, 15);

    cbDebugPrint((MAKE_LEVEL(HDMI, INFO), "VICCode=%d.\n", pModeParams->VICCode));
    if ((pModeParams->VICCode > 0) && (pModeParams->VICCode <= CBIOS_HDMIFORMATCOUNTS))
    {
        pInfoFrameData->bIsCEAMode = CBIOS_TRUE;
    }
    else
    {
        pInfoFrameData->bIsCEAMode = CBIOS_FALSE;
    }

}


static CBIOS_VOID cbHDMIMonitor_SetAVIInfoFrame(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_AVI_INFO_FRAME_DATA pAVIInfoFrameData, CBIOS_ACTIVE_TYPE DeviceType, PCBIOS_U8 FIFOIndex)
{
    CBIOS_U8 ucAVIData[32], i = 0, checksum = 0;
    CBIOS_U32 DataLen = 0;
    CBIOS_U32 Version = 0x2;

    cb_memset(ucAVIData, 0, 32);

    /*
        According to CEA-861-F 6.4
        A version 3 AVI InfoFrame is only used when either of the most-significant bits Y2 or VIC7 are set to '1'

        Note for HDMI Specification Version 2.0:
        it is always the case that Y2=0 (see previous bullet).
        Also, no VIC codes above 107 have been defined in CEA-861-F (thus VIC7=0).
        Therefore, the "version 3" AVI InfoFrame is not applicable for Source Devices
        built according to This Specification, and Source Devices
        shall always transmit a "version 2" AVI InfoFrame.
    */
    if((pAVIInfoFrameData->ColorFormat > 3) || (pAVIInfoFrameData->VICCode > 127))
    {
        Version = 0x3;    // Version = 0x03
    }
    else
    {
        Version = 0x2;    // Version = 0x02
    }

    ucAVIData[0] = 0x82;    // Packet Type = 0x82
    ucAVIData[1] = (CBIOS_U8)Version;
    ucAVIData[2] = 0x0D;    // Length = 0x0D

    DataLen = cb_min(CBIOS_AVI_INFO_FRAME_LEN, sizeof(CBIOS_AVI_INFO_FRAME_DATA));
    cb_memcpy(&ucAVIData[4], pAVIInfoFrameData, DataLen);

    // count check sum
    for(i = 0; i < 32; i++)
    {
        checksum += ucAVIData[i];
    }

    ucAVIData[3] = 0x100 - checksum;

    cbDIU_HDMI_WriteFIFO(pcbe, DeviceType, *FIFOIndex, ucAVIData, 32);
    (*FIFOIndex)++;
}

static CBIOS_VOID cbHDMIMonitor_SetAudioInfoFrame(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_ACTIVE_TYPE DeviceType, PCBIOS_U8 FIFOIndex)
{
    CBIOS_MODULE_INDEX HDACModuleIndex = CBIOS_MODULE_INDEX_INVALID;
    CBIOS_U8 ucAAIData[32], i = 0, checksum = 0;
    CBIOS_U32 NumofChannels = 0;

    cb_memset(ucAAIData, 0, 32);

    HDACModuleIndex = cbGetModuleIndex(pcbe, DeviceType, CBIOS_MODULE_TYPE_HDAC);
    if (HDACModuleIndex == CBIOS_MODULE_INDEX_INVALID)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDAC module index!\n", FUNCTION_NAME));
        return;
    }

    NumofChannels = cbDIU_HDAC_GetChannelNums(pcbe, HDACModuleIndex);

    ucAAIData[0] = 0x84;	// Packet Type = 0x84
    ucAAIData[1] = 0x01;	// Version number = 0x01
    ucAAIData[2] = 0x0A;	// Length = 0x0A

    // CC2 CC1 CC0  |   Audio Channel Count
    // ---------------------------------------
    //  0   0   0   |   Refer to Stream Header
    //  0   0   1   |   2ch
    //  0   1   0   |   3ch
    //  0   1   1   |   4ch
    //  1   0   0   |   5ch
    //  1   0   1   |   6ch
    //  1   1   0   |   7ch
    //  1   1   1   |   8ch
    ucAAIData[4] |= NumofChannels;

    // CT3 CT2 CT1 CT0  |   Audio Coding Type
    // -------------------------------------------
    //  0   0   0   0   |   Refer to Stream Header
    //  0   0   0   1   |   IEC 60958 PCM[30,31]
    //  0   0   1   0   |   AC-3
    //  0   0   1   1   |   MPEG1 (Layer 1 & 2)
    ucAAIData[4] |= 0x00;

    // SS1 SS0  |   Sample Size
    // --------------------------
    //  0   0   |   Refer to Stream Header
    //  0   1   |   16 bit
    //  1   0   |   20 bit
    //  1   1   |   24 bit
    ucAAIData[5] |= 0x00;

    // SF2 SF1 SF0  |   Sampling Frequency
    // ---------------------------------------
    //  0   0   0   |   Refer to Stream Header
    //  0   0   1   |   32 kHz
    //  0   1   0   |   44.1 kHz(CD)
    //  0   1   1   |   48 kHz
    //  1   0   0   |   88.2 kHz
    //  1   0   1   |   96 kHz
    //  1   1   0   |   176.4 kHz
    //  1   1   1   |   192 kHz
    ucAAIData[5] |= 0x00;

    ucAAIData[6] = 0x00;

    // CA
    if(NumofChannels == 1)
    {
        ucAAIData[7] = 0;
    }
    else
    {
        ucAAIData[7] = cbHDMIMonitor_HDACGetCAValue(pcbe, DeviceType);
    }
    ucAAIData[8] = 0x00;

    //all others are 0

    // count check sum
    for(i = 0; i < 32; i++)
        checksum += ucAAIData[i];

    ucAAIData[3] = 0x100 - checksum;

    cbDIU_HDMI_WriteFIFO(pcbe, DeviceType, *FIFOIndex, ucAAIData, 32);
    (*FIFOIndex)++;

}

static CBIOS_VOID cbHDMIMonitor_SetHFVendorSpecificInfoFrame(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_ACTIVE_TYPE DeviceType, PCBIOS_HF_VENDOR_SPECIFIC_INFOFRAME_PARA pHFVSIFPara,
PCBIOS_U8 FIFOIndex)
{
    CBIOS_U8    VSInfoFrame[32];
    CBIOS_U32   i = 0;
    CBIOS_U32   Length = 0;
    CBIOS_U8    Checksum = 0;
    CBIOS_U8    *pHFVSIFPayLoad = CBIOS_NULL;

    cb_memset(VSInfoFrame, 0, sizeof(VSInfoFrame));

    //Packet Header

    //Packet Type
    VSInfoFrame[0] = 0x81;

    //Version
    VSInfoFrame[1] = 0x01;

    //Length should be programed later

    //Checksum will be programmed later

    //Packet content begin
    Length = 0;
    pHFVSIFPayLoad = &(VSInfoFrame[4]);

    //24bit IEEE Registration identifier
    *(pHFVSIFPayLoad++) = 0xD8;
    *(pHFVSIFPayLoad++) = 0x5D;
    *(pHFVSIFPayLoad++) = 0xC4;
    Length += 3;

    //Version
    *(pHFVSIFPayLoad++) = 0x1;
    Length++;

    //3D_Valid
    *(pHFVSIFPayLoad++) = pHFVSIFPara->Valid3D;
    Length++;

    if(pHFVSIFPara->Valid3D)
    {
        CBIOS_U8 Data = 0;

        //3D_F_STRUCTURE
        Data |= (pHFVSIFPara->Video3DFStruct & 0xF) << 4;

        if(pHFVSIFPara->IsEnable3DDualView || pHFVSIFPara->IsEnable3DIndependentView)
        {
            //3D AddtionalInfo_Present
            Data |= (1 << 3);
        }

        if(pHFVSIFPara->IsEnable3DOSDDisparity && (pHFVSIFPara->DisparityPara.DisparityLength > 0))
        {
            //3D_DisparityData_Present
            Data |= (1 << 2);
        }

        //Assume 3D_Meta_present = 0
        Data |= (0 << 1);

        *(pHFVSIFPayLoad++) = Data;
        Length++;

        //3D_F_Ext_Data
        if (pHFVSIFPara->Video3DFStruct == 8)
        {
            *(pHFVSIFPayLoad++) = (0x00 << 4) & 0xF0;
            Length++;
        }
        else if (pHFVSIFPara->Video3DFStruct > 8)
        {
            //now hardcode here first, it should be read from VSDB
            *(pHFVSIFPayLoad++) = (0x04 << 4) & 0xF0;
            Length++;
        }

        if(pHFVSIFPara->IsEnable3DDualView || pHFVSIFPara->IsEnable3DIndependentView)
        {
            Data = 0;

            //3D Dual_View
            if(pHFVSIFPara->IsEnable3DDualView)
            {
                Data |= (1 << 4);
            }

            //Independent View
            if(pHFVSIFPara->IsEnable3DIndependentView)
            {
                Data |= ((pHFVSIFPara->IndependentViewPara.Dependency & 0x3) << 2);
                Data |= (pHFVSIFPara->IndependentViewPara.Preferred2D & 0x3);
            }

            *(pHFVSIFPayLoad++) = Data;
            Length++;
        }

        if(pHFVSIFPara->IsEnable3DOSDDisparity && (pHFVSIFPara->DisparityPara.DisparityLength > 0))
        {
            Data = 0;

            //3D_DisparityData_Version
            Data |= ((pHFVSIFPara->DisparityPara.Version & 0x7) << 5);
            Data |= ((pHFVSIFPara->DisparityPara.DisparityLength & 0x1F) << 5);
            *(pHFVSIFPayLoad++) = Data;
            Length++;

            //Disparity Data
            cb_memcpy(pHFVSIFPayLoad, &pHFVSIFPara->DisparityPara.DisplarityData[0], pHFVSIFPara->DisparityPara.DisparityLength);
            Length += pHFVSIFPara->DisparityPara.DisparityLength;
            pHFVSIFPayLoad += pHFVSIFPara->DisparityPara.DisparityLength;
        }
    }

    //Length
    VSInfoFrame[2] = Length & 0x1F;

    //Checksum
    for (i = 0; i < 32; i++)
    {
        Checksum += VSInfoFrame[i];
    }
    VSInfoFrame[3] = 0xFF - Checksum + 1;

    cbDIU_HDMI_WriteFIFO(pcbe, DeviceType, *FIFOIndex, VSInfoFrame, 32);
    (*FIFOIndex)++;
}

static CBIOS_VOID cbHDMIMonitor_SetVendorSpecificInfoFrame(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_ACTIVE_TYPE DeviceType, PCBIOS_VENDOR_SPECIFIC_INFOFRAME_PARA pVSIFPara,
PCBIOS_U8 FIFOIndex)
{
    CBIOS_U8    VSInfoFrame[32];
    CBIOS_U32   i = 0;
    CBIOS_U32   Length = 0;
    CBIOS_U8    Checksum = 0;
    CBIOS_U8    *pVSIFPayLoad = CBIOS_NULL;

    cb_memset(VSInfoFrame, 0, sizeof(VSInfoFrame));

    //Packet Header

    //Packet Type
    VSInfoFrame[0] = 0x81;

    //Version
    VSInfoFrame[1] = 0x01;

    //Length should be programed later

    //Checksum will be programmed later

    //Packet content begin
    Length = 0;
    pVSIFPayLoad = &(VSInfoFrame[4]);

    //24bit IEEE Registration identifier
    *(pVSIFPayLoad++) = 0x03;
    *(pVSIFPayLoad++) = 0x0C;
    *(pVSIFPayLoad++) = 0x00;
    Length += 3;

    //  PB4[7-5]    |   HDMI_Video_Format
    //----------------------------------------------------
    //  0   0   0   |   No Additional info
    //  0   0   1   |   Extended resolution format present
    //  0   1   0   |   3D format indication present
    *(pVSIFPayLoad++) = (pVSIFPara->VSIFVideoFormat << 5) & 0xE0;
    Length++;

    if (pVSIFPara->VSIFVideoFormat == VSIF_3D_FORMAT_INDICATION_PRESENT)
    {

        //3D structure, 3D_Meta_present
        //Assume 3D_Meta_present = 0
        *(pVSIFPayLoad++) = (pVSIFPara->VSIF3DFormatPara.Video3DStruct << 4) & 0xF0;
        Length++;

        if (pVSIFPara->VSIF3DFormatPara.Video3DStruct == 8)
        {
            *(pVSIFPayLoad++) = (0x00 << 4) & 0xF0;
            Length++;
        }
        else if (pVSIFPara->VSIF3DFormatPara.Video3DStruct > 8)
        {
            //now hardcode here first, it should be read from VSDB
            *(pVSIFPayLoad++) = (0x04 << 4) & 0xF0;
            Length++;
        }

        //3D_Metadata
        //TBD
    }
    else if (pVSIFPara->VSIFVideoFormat == VSIF_EXTENDED_FORMAT_PRESENT)
    {
        //VIC
        *(pVSIFPayLoad++) = pVSIFPara->VSIFExtFormatPara.VICCode;
        Length++;
    }
    else
    {
        //HDMI_Video_Format: no additional info
    }

    //Length
    VSInfoFrame[2] = Length & 0x1F;

    //Checksum
    for (i = 0; i < 32; i++)
    {
        Checksum += VSInfoFrame[i];
    }
    VSInfoFrame[3] = 0xFF - Checksum + 1;

    cbDIU_HDMI_WriteFIFO(pcbe, DeviceType, *FIFOIndex, VSInfoFrame, 32);
    (*FIFOIndex)++;
}


static CBIOS_VOID cbHDMIMonitor_SetGamutMetadataPacket(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_ACTIVE_TYPE DeviceType, PCBIOS_GAMUT_METADATA_PARA pGamutMetadata, PCBIOS_U8 FIFOIndex)
{
    CBIOS_U8    GamutMetadataPacket[32];
    CBIOS_U8    ColorPrecision = 0;

    cb_memset(GamutMetadataPacket, 0, sizeof(GamutMetadataPacket));

    //get ColorPrecision
    if (pGamutMetadata->GBDColorPrecision == 10)
    {
        ColorPrecision = 0x01;
    }
    else if (pGamutMetadata->GBDColorPrecision == 12)
    {
        ColorPrecision = 0x02;
    }
    else//set to 8 bit by default
    {
        ColorPrecision = 0x00;
    }

    //haeder byte 0, packet type
    GamutMetadataPacket[0] = 0x0A;

    //haeder byte 1
    //Next_Field, GBD_Profile, Affected_Gamut_Seq_Num
    //now hardcode here
    GamutMetadataPacket[1] = 0x80;

    //haeder byte 2
    //No_Crnt_GBD, Packet_Seq, Current_Gamut_Seq_Num
    //now hardcode here
    GamutMetadataPacket[2] = 0x30;

    //Packet body. P0 only now
    if (pGamutMetadata->FormatFlag == VERTICES_FACETS_GBD_STRUCT)
    {
        PCBIOS_VERTICES_GBD_PARA    pGBDPara = &(pGamutMetadata->GBDData.VerticesGBD);
        CBIOS_U32   VerticesSize = 0;

        //byte 0
        GamutMetadataPacket[3] = ((pGBDPara->ColorSpace & 0x07)
            | ((ColorPrecision << 3) & 0x18)
            | ((pGamutMetadata->FormatFlag << 7) & 0x80));

        //byte 1, Number_Vertices_H
        GamutMetadataPacket[4] = (CBIOS_U8)((pGBDPara->VerticesNum >> 8) & 0xff);

        //byte 2, Number_Vertices_L
        GamutMetadataPacket[5] = (CBIOS_U8)(pGBDPara->VerticesNum & 0xff);

        //byte 3 ... VSIZE+2: Packed_GBD_Vertices_Data[0...VSIZE-1]
        VerticesSize = cbRound(3 * pGBDPara->VerticesNum * pGamutMetadata->GBDColorPrecision, 8, ROUND_UP);
        VerticesSize = cb_min(VerticesSize, pGBDPara->VerticesDataSize);
        cb_memcpy(&(GamutMetadataPacket[6]), pGBDPara->pVerticesData, VerticesSize);

        if (pGBDPara->bFacetMode)
        {
            //byte VSIZE+5 ... VSIZE+FSIZE+4, Packed_GBD_Facets_Data[0...FSIZE - 1]
            //not defined yet
        }

    }
    else if (pGamutMetadata->FormatFlag == RANGE_GBD_STRUCT)
    {
        PCBIOS_RANGE_GBD_PARA   pGBDPara = &(pGamutMetadata->GBDData.RangeGBD);
        CBIOS_U32   DataSize = 0;

        //byte 0
        GamutMetadataPacket[3] = ((pGBDPara->ColorSpace & 0x07)
            | ((ColorPrecision << 3) & 0x18)
            | ((pGamutMetadata->FormatFlag << 7) & 0x80));

        //byte 1...N, Packed_Range_Data
        DataSize = cbRound(pGamutMetadata->GBDColorPrecision * 3 * 2, 8, ROUND_UP);
        DataSize = cb_min(DataSize, pGBDPara->RangeDataSize);
        cb_memcpy(&(GamutMetadataPacket[4]), pGBDPara->pRangeData, DataSize);
    }

    cbDIU_HDMI_WriteFIFO(pcbe, DeviceType, *FIFOIndex, GamutMetadataPacket, 32);
    (*FIFOIndex)++;
}

#if 0
static CBIOS_BOOL cbHDMIMonitor_HDCP_ReadData(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_U8* Buffer, CBIOS_U8 Offset, CBIOS_U32 Num)
{
    CBIOS_PARAM_I2C_DATA I2CParams;
    CBIOS_U8 I2CBUSNum = (CBIOS_U8)pDevCommon->I2CBus;
    CBIOS_BOOL bRet = CBIOS_FALSE;

    cb_memset(&I2CParams, 0, sizeof(CBIOS_PARAM_I2C_DATA));

    cb_AcquireMutex(pcbe->pI2CMutex[I2CBUSNum]);

    I2CParams.RequestType = 0xFF;
    I2CParams.PortNumber = I2CBUSNum;
    I2CParams.SlaveAddress = 0x74;
    I2CParams.Buffer = Buffer;
    I2CParams.BufferLen = Num;
    I2CParams.OffSet = Offset;
    I2CParams.bHDCPEnable = 1;

    bRet = cbHWI2CDataRead(pcbe, &I2CParams);

    cb_ReleaseMutex(pcbe->pI2CMutex[I2CBUSNum]);

    if(bRet != CBIOS_OK)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: HDCP read failed!\n", FUNCTION_NAME));

        return CBIOS_FALSE;
    }

    return CBIOS_TRUE;
}

static CBIOS_BOOL cbHDMIMonitor_HDCP_ReadBKsv(CBIOS_VOID* pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_U8 pBksv, PCBIOS_BOOL bRepeater)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_BOOL bRet = CBIOS_TRUE;
    CBIOS_U8 BCaps = 0;

    bRet = cbHDMIMonitor_HDCP_ReadData(pcbe, pDevCommon, pBksv, 0, 5);
    if(!bRet)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: Read BKsv failed!\n", FUNCTION_NAME));

        return bRet;
    }

    bRet = cbHDMIMonitor_HDCP_ReadData(pcbe, pDevCommon, &BCaps, 0x40, 1);

    if(!bRet)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: Read BCaps failed!\n", FUNCTION_NAME));
    }
    else
    {
        *bRepeater = BCaps & 0x40;
    }

    return bRet;
}
#endif

static CBIOS_BOOL cbHDMIMonitor_SCDC_ReadData(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_U8* Buffer, CBIOS_U8 Offset, CBIOS_U32 Num)
{
    CBIOS_PARAM_I2C_DATA I2CParams;
    CBIOS_U8 I2CBUSNum = (CBIOS_U8)pDevCommon->I2CBus;
    CBIOS_BOOL bRet = CBIOS_FALSE;

    cb_memset(&I2CParams, 0, sizeof(CBIOS_PARAM_I2C_DATA));

    cb_AcquireMutex(pcbe->pI2CMutex[I2CBUSNum]);

    I2CParams.RequestType = 0xFF;
    I2CParams.PortNumber = I2CBUSNum;
    I2CParams.SlaveAddress = 0xA8;
    I2CParams.Buffer = Buffer;
    I2CParams.BufferLen = Num;
    I2CParams.OffSet = Offset;
    I2CParams.bHDCPEnable = 1;

    bRet = cbHWI2CDataRead(pcbe, &I2CParams);

    cb_ReleaseMutex(pcbe->pI2CMutex[I2CBUSNum]);

    if(bRet != CBIOS_OK)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: SCDC read failed!\n", FUNCTION_NAME));

        return CBIOS_FALSE;
    }

    return CBIOS_TRUE;
}

static CBIOS_BOOL cbHDMIMonitor_SCDC_WriteData(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_U8* Buffer, CBIOS_U8 Offset, CBIOS_U32 Num)
{
    CBIOS_PARAM_I2C_DATA I2CParams;
    CBIOS_U8 I2CBUSNum = (CBIOS_U8)pDevCommon->I2CBus;
    CBIOS_BOOL bRet = CBIOS_FALSE;

    cb_memset(&I2CParams, 0, sizeof(CBIOS_PARAM_I2C_DATA));

    cb_AcquireMutex(pcbe->pI2CMutex[I2CBUSNum]);

    I2CParams.RequestType = 0xFF;
    I2CParams.PortNumber = I2CBUSNum;
    I2CParams.SlaveAddress = 0xA8;
    I2CParams.Buffer = Buffer;
    I2CParams.BufferLen = Num;
    I2CParams.OffSet = Offset;
    I2CParams.bHDCPEnable = 1;

    bRet = cbHWI2CDataWrite(pcbe, &I2CParams);

    cb_ReleaseMutex(pcbe->pI2CMutex[I2CBUSNum]);

    if(bRet != CBIOS_OK)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: SCDC write failed!\n", FUNCTION_NAME));

        return CBIOS_FALSE;
    }

    return CBIOS_TRUE;
}

static CBIOS_BOOL cbHDMIMonitor_SCDC_ReadUpdateFlags(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_SCDC_UPDATE_FLAGS pUpdateFlags)
{
    CBIOS_BOOL  bRet = CBIOS_FALSE;

    pUpdateFlags->UpdateFlags = 0;
    bRet = cbHDMIMonitor_SCDC_ReadData(pcbe, pDevCommon, (CBIOS_U8*)(&(pUpdateFlags->UpdateFlags)), 0x10, 0x2);//Offset = 0x10:Update_0, Offset = 0x11:Update_1

    if(pUpdateFlags->UpdateFlags != 0)
    {
        //write 1 to clear corresponding bits
        cbHDMIMonitor_SCDC_WriteData(pcbe, pDevCommon, (CBIOS_U8*)(&(pUpdateFlags->UpdateFlags)), 0x10, 0x2);
        cbDebugPrint((MAKE_LEVEL(HDMI, DEBUG), "Clear SCDC corresponding Update Flags bits\n"));
    }

    cbDebugPrint((MAKE_LEVEL(HDMI, DEBUG), "SCDC Update Flags: 0x%04X, Status_Update: %X, CED_Update: %X, RR_Test: %X!\n",
                    pUpdateFlags->UpdateFlags, pUpdateFlags->Update_0.StatusUpdate, pUpdateFlags->Update_0.CEDUpdate, pUpdateFlags->Update_0.RRTest));

    return bRet;
}

static CBIOS_BOOL cbHDMIMonitor_SCDC_ReadStatusFlags(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_SCDC_STATUS_FLAGS pStatusFlags)
{
    CBIOS_BOOL  bRet = CBIOS_FALSE;

    pStatusFlags->Status_Flags = 0;
    bRet = cbHDMIMonitor_SCDC_ReadData(pcbe, pDevCommon, &(pStatusFlags->ScramblerStatus), 0x21, 0x1);//Offset = 0x40:Status_0, Offset = 0x41:Status_1

    if(!bRet)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: SCDC read data failed !\n", FUNCTION_NAME));

        return CBIOS_FALSE;
    }

    cbDebugPrint((MAKE_LEVEL(HDMI, DEBUG), "SCDC Scrambler Status: 0x%02X, Scrambling Status: %1X !\n",
                    pStatusFlags->ScramblerStatus, pStatusFlags->Scrambling_Status));

    bRet = cbHDMIMonitor_SCDC_ReadData(pcbe, pDevCommon, (CBIOS_U8*)(&(pStatusFlags->Status_Flags)), 0x40, 0x2);//Offset = 0x40:Status_0, Offset = 0x41:Status_1

    if(!bRet)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: SCDC read data failed !\n", FUNCTION_NAME));

        return CBIOS_FALSE;
    }

    cbDebugPrint((MAKE_LEVEL(HDMI, DEBUG), "SCDC Status Flags: 0x%04X, Clock_Detected: %X, Ch0_Locked: %X, Ch1_Locked: %X, Ch2_Locked: %X!\n",
                    pStatusFlags->Status_Flags, pStatusFlags->Status_Flags_0.ClockDetected, pStatusFlags->Status_Flags_0.Ch0_Locked,
                    pStatusFlags->Status_Flags_0.Ch1_Locked, pStatusFlags->Status_Flags_0.Ch2_Locked));

    return bRet;
}

static CBIOS_BOOL cbHDMIMonitor_SCDC_ReadCEDData(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_SCDC_CED_DATA pCEDData)
{
    CBIOS_BOOL  bRet = CBIOS_FALSE;

    bRet = cbHDMIMonitor_SCDC_ReadData(pcbe, pDevCommon, (CBIOS_U8*)pCEDData, 0x50, 0x7);//Offset = 0x50:Character Error Detection start offset

    cbDebugPrint((MAKE_LEVEL(HDMI, INFO), "Ch0_Valid:%X, Ch0_ERR_Counter:%d, Ch1_Valid:%X, Ch1_ERR_Counter:%d, Ch2_Valid:%X, Ch2_ERR_Counter:%d !\n",
                pCEDData->Ch0_Valid, (((pCEDData->Ch0_Err_CountBits14To8 << 8) | pCEDData->Ch0_Err_CountBits7To0) & 0x7FFF),
                pCEDData->Ch1_Valid, (((pCEDData->Ch1_Err_CountBits14To8 << 8) | pCEDData->Ch1_Err_CountBits7To0) & 0x7FFF),
                pCEDData->Ch2_Valid, (((pCEDData->Ch2_Err_CountBits14To8 << 8) | pCEDData->Ch2_Err_CountBits7To0) & 0x7FFF)));

    return bRet;
}

CBIOS_BOOL cbHDMIMonitor_SCDC_Handler(CBIOS_VOID* pvcbe, PCBIOS_DEVICE_COMMON pDevCommon)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_SCDC_UPDATE_FLAGS UpdateFlags;
    CBIOS_BOOL bRet = CBIOS_FALSE;

    if(pDevCommon->EdidStruct.Attribute.HFSCDSData.IsSCDCPresent)
    {
        cb_memset(&UpdateFlags, 0, sizeof(UpdateFlags));
        bRet = cbHDMIMonitor_SCDC_ReadUpdateFlags(pcbe, pDevCommon, &UpdateFlags);

        if(UpdateFlags.Update_0.StatusUpdate)
        {
            CBIOS_SCDC_STATUS_FLAGS StatusFlags;

            cb_memset(&StatusFlags, 0, sizeof(StatusFlags));
            bRet = cbHDMIMonitor_SCDC_ReadStatusFlags(pcbe, pDevCommon, &StatusFlags);
        }

        if(UpdateFlags.Update_0.CEDUpdate)
        {
            CBIOS_SCDC_CED_DATA CEDData;

            cb_memset(&CEDData, 0, sizeof(CEDData));
            bRet = cbHDMIMonitor_SCDC_ReadCEDData(pcbe, pDevCommon, &CEDData);
        }
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "SCDC is not present!\n"));
    }

    return bRet;
}

static CBIOS_VOID cbHDMIMonitor_SCDC_Configure(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_HDMI_MONITOR_CONTEXT pHDMIMonitorContext)
{
    CBIOS_U8                Data = 0;
    CBIOS_U8                ReadBackData = 0;
    CBIOS_BOOL              bRet = CBIOS_FALSE;
    PCBIOS_DEVICE_COMMON    pDevCommon = pHDMIMonitorContext->pDevCommon;

    //Offset = 0x1: Sink Version
    bRet = cbHDMIMonitor_SCDC_ReadData(pcbe, pDevCommon, &Data, 0x1, 0x1);

    if(!bRet)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: SCDC read Sink Version failed!\n", FUNCTION_NAME));
        return;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, DEBUG), "Sink Version %X\n", Data));
    }

    Data = 1;// Version 1

    //Offset = 0x2: Source Version
    bRet = cbHDMIMonitor_SCDC_WriteData(pcbe, pDevCommon, &Data, 0x2, 0x1);
    ReadBackData = 0;
    bRet = cbHDMIMonitor_SCDC_ReadData(pcbe, pDevCommon, &ReadBackData, 0x2, 0x1);

    if(bRet && (Data == ReadBackData))
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, DEBUG), "SCDC write Source Version successfully!\n"));
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: SCDC write Source Version failed!\n", FUNCTION_NAME));
    }

    Data = 0;
    if(pHDMIMonitorContext->ScramblingEnable)
    {
        Data |= (1 << 0);
    }
    if(pHDMIMonitorContext->TMDSBitClockRatio)
    {
        Data |= (1 << 1);
    }

    //Offset = 0x20: SCDC TMDS_Config
    cb_DelayMicroSeconds(20000);//add a delay here to wait sink status stable
    bRet = cbHDMIMonitor_SCDC_WriteData(pcbe, pDevCommon, &Data, 0x20, 0x1);
    //per HDMI2.0 spec, need wait 1-100ms before send TMDS signal
    cb_DelayMicroSeconds(20000);
    ReadBackData = 0;
    bRet = cbHDMIMonitor_SCDC_ReadData(pcbe, pDevCommon, &ReadBackData, 0x20, 0x1);

    if(bRet && (Data == ReadBackData))
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, DEBUG), "SCDC write TMDS_Config successfully!\n"));
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: SCDC write TMDS_Config failed!\n", FUNCTION_NAME));
    }

    // We write SCDC config 0x30 here, VBIOS clear SCDC config 0x20 when reboot os,
    //The ViewSonic VP2780 monitor will show ViewSonic logo. To fix bug 6231.
/*
    Data = 0;
    if(pHDMIMonitorContext->ReadRequestEnable)
    {
        Data |= (1 << 0);
    }

    //Offset = 0x30: SCDC Config_0
    bRet = cbHDMIMonitor_SCDC_WriteData(pcbe, pDevCommon, &Data, 0x30, 0x1);
    ReadBackData = 0;
    bRet = cbHDMIMonitor_SCDC_ReadData(pcbe, pDevCommon, &ReadBackData, 0x30, 0x1);

    if(bRet && (Data == ReadBackData))
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, DEBUG), "SCDC write Config_0 successfully!\n"));
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: SCDC write Config_0 failed!\n", FUNCTION_NAME));
    }
*/
}


CBIOS_VOID cbHDMIMonitor_SetInfoFrame(PCBIOS_VOID pvcbe, PCBIOS_HDMI_INFO_FRAME_DATA pInfoFrameData, CBIOS_ACTIVE_TYPE DeviceType)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U8 FIFOIndex = 0;

    // AVI InfoFrame
    cbHDMIMonitor_SetAVIInfoFrame(pcbe, &(pInfoFrameData->AVIInfoFrameData), DeviceType, &FIFOIndex);

    // Audio InfoFrame
    cbHDMIMonitor_SetAudioInfoFrame(pcbe, DeviceType, &FIFOIndex);

    /*
        3D OSD Disparity Indication
        3D Dual-View Signaling
        3d Independent View Signaling
    */
    if(pInfoFrameData->HFVSIFPara.IsEnable3DOSDDisparity || pInfoFrameData->HFVSIFPara.IsEnable3DDualView || pInfoFrameData->HFVSIFPara.IsEnable3DIndependentView)
    {
        cbHDMIMonitor_SetHFVendorSpecificInfoFrame(pcbe, DeviceType, &(pInfoFrameData->HFVSIFPara), &FIFOIndex);
    }
    else
    {
        //if don't send HF-VSIF, always send VSIF
        cbHDMIMonitor_SetVendorSpecificInfoFrame(pcbe, DeviceType, &(pInfoFrameData->VSIFPara), &FIFOIndex);
    }

    //xvYCC
    if (pInfoFrameData->GamutMetadata.FormatFlag)
    {
        cbHDMIMonitor_SetGamutMetadataPacket(pcbe, DeviceType, &(pInfoFrameData->GamutMetadata), &FIFOIndex);
    }

    //Send
    cbDIU_HDMI_SendInfoFrame(pcbe, pInfoFrameData->HDMIMaxPacketNum, DeviceType, FIFOIndex);
}

CBIOS_BOOL cbHDMIMonitor_Detect(PCBIOS_VOID pvcbe, PCBIOS_HDMI_MONITOR_CONTEXT pHDMIMonitorContext, CBIOS_BOOL bHardcodeDetected, CBIOS_U32 FullDetect)
{
    PCBIOS_EXTENSION_COMMON    pcbe           = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_BOOL                 bConnected     = CBIOS_FALSE;
    CBIOS_BOOL                 IsDevChanged   = CBIOS_FALSE;
    PCBIOS_DEVICE_COMMON       pDevCommon     = pHDMIMonitorContext->pDevCommon;
    PCBIOS_UCHAR               pEDIDData      = pDevCommon->EdidData;
    PCBIOS_MONITOR_MISC_ATTRIB pMonitorAttrib = &(pDevCommon->EdidStruct.Attribute);
    PCBIOS_DP_CONTEXT     pDpContext = container_of(pDevCommon, PCBIOS_DP_CONTEXT, Common);
    CBIOS_MODULE_INDEX HDMIModuleIndex = cbGetModuleIndex(pcbe, pDevCommon->DeviceType, CBIOS_MODULE_TYPE_HDMI);
    CBIOS_SCDC_STATUS_FLAGS     SCDCStatusFlags;

    /*
    ** 1. get EDID data
    */
    if (bHardcodeDetected)
    {
        // use hardcoded EDID data instead of reading from monitor side
#if IS_SUPPORT_4K_MODE
        cb_memcpy(pDevCommon->EdidData, Fake4KEdid, sizeof(Fake4KEdid));
#else
        cb_memcpy(pDevCommon->EdidData, FPGAHDMIEdid, sizeof(FPGAHDMIEdid));
#endif
        pDevCommon->TotalBlockNum = cbEDIDModule_GetExtBlockNum(pDevCommon->EdidData) + 1;
        IsDevChanged = CBIOS_TRUE;
        bConnected = CBIOS_TRUE;
    }

    if ((!bConnected) && (cbGetDeviceEDID(pcbe, pDevCommon, &IsDevChanged, FullDetect)))
    {
        bConnected = CBIOS_TRUE;
    }

    /*
    ** 2. parse EDID data
    */
    if(bConnected)
    {
        if (IsDevChanged)
        {
            cbEDIDModule_ParseEDID(pDevCommon->EdidData, &(pDevCommon->EdidStruct), EDID_BLOCK_SIZE_SPEC * pDevCommon->TotalBlockNum);
            cbEDIDModule_SADPatch(&(pDevCommon->EdidStruct));
            cbUpdateDeviceSignature(pcbe, pDevCommon);
            pDevCommon->isFakeEdid = CBIOS_FALSE;
        }

        if (FullDetect && HDMIModuleIndex != CBIOS_MODULE_INDEX_INVALID
            && pDevCommon->PowerState == CBIOS_PM_ON
            && (pcbe->DeviceMgr.ConnectedDevices & pDevCommon->DeviceType))
        {
            //NOTE: fix issue13048, as the client will not set mode when hotplug bettween two monitors which has the same mode and one support
            // scramble while the other does not.
            //Some monitor support 4K@60 but not set IsSupportLTE340McscScramble, the sramble will be disable, add patch here to check clock.
            if(pHDMIMonitorContext->HDMIClock > 3400000)
            {
                pHDMIMonitorContext->ScramblingEnable = CBIOS_TRUE;//enable scrambling if both source and sink device support scrambling
            }
            else
            {
                pHDMIMonitorContext->ScramblingEnable = CBIOS_FALSE;
            }
            cbDIU_HDMI_ConfigScrambling(pcbe, HDMIModuleIndex, pHDMIMonitorContext->ScramblingEnable);

            if(pDevCommon->EdidStruct.Attribute.HFSCDSData.IsSCDCPresent)
            {
                SCDCStatusFlags.ScramblerStatus = 0;
                if(cbHDMIMonitor_SCDC_ReadData(pcbe, pDevCommon, &(SCDCStatusFlags.ScramblerStatus), 0x21, 0x1))
                {
                    //if scramble was enabled, but scramble_status is not 1, should reconfig SCDC
                    if((pHDMIMonitorContext->ScramblingEnable && !SCDCStatusFlags.Scrambling_Status)
                        || (!pHDMIMonitorContext->ScramblingEnable && SCDCStatusFlags.Scrambling_Status))
                    {
                        //per HDMI2.0 spec,before write TMDS config,source should suspend transmission of TMDS clock and data
                        cbPHY_DP_DualModeOnOff(pcbe, HDMIModuleIndex, pDpContext->HDMIMonitorContext.HDMIClock, 0);
                        cbHDMIMonitor_SCDC_Configure(pcbe, pHDMIMonitorContext);
                        cbPHY_DP_DualModeOnOff(pcbe, HDMIModuleIndex, pDpContext->HDMIMonitorContext.HDMIClock, 1);
                    }
                }
            }
        }

        if (!(pEDIDData[EDID_VIDEO_INPUT_DEF_BYTE_OFFSET] & EDID_VIDEO_INPUT_DEF_DIGITAL))
        {
            cbDebugPrint((MAKE_LEVEL(HDMI, WARNING), "%s: why come here??? Current device is a CRT monitor according to EDID!\n", FUNCTION_NAME));
        }

        // Check if HDMI
        if (pMonitorAttrib->IsCEA861HDMI)
        {
            pDevCommon->CurrentMonitorType = CBIOS_MONITOR_TYPE_HDMI;
        }
        else
        {
            pDevCommon->CurrentMonitorType = CBIOS_MONITOR_TYPE_DVI;
        }

        if (!bHardcodeDetected)
        {
            if ((pDevCommon->CurrentMonitorType == CBIOS_MONITOR_TYPE_HDMI) &&
                (pMonitorAttrib->VSDBData.VSDBHeader.SourcePhyAddr != CEC_INVALID_PHYSICAL_ADDR))
            {
                CBIOS_CEC_INDEX CECIndex = CBIOS_CEC_INDEX1;

                CECIndex = CBIOS_CEC_INDEX1;

                //get physical address
                pcbe->CECPara[CECIndex].PhysicalAddr = pMonitorAttrib->VSDBData.VSDBHeader.SourcePhyAddr;

                if (pcbe->CECPara[CECIndex].CECEnable)
                {
                    //allocate logical address
                    cbCECAllocateLogicalAddr(pcbe, CECIndex);
                }
            }
        }
    }
    else
    {
        //for the case:if plugout monitor before suspend,and then plugin hdmi monitor before resume,after resume,4k@60 can't be lighted.
        //rootcause:if plugout monitor,hpd thread will detect no device,so some attributes stored in pDevCommon will be cleared,
        //plugin monitor before resume,os will do nothing,so it will not update attributes stored in pDevCommon
        //then after resume,driver will not enter some hdmi module related codes,so monitor can't light
        //so not memset pDevCommon->EdidStruct when device is not connected
        cbClearEdidRelatedData(pcbe, pDevCommon);
        cb_memset(((PCBIOS_HDCP_CONTEXT)pDevCommon->pHDCPContext)->BKsv, 0, sizeof(((PCBIOS_HDCP_CONTEXT)pDevCommon->pHDCPContext)->BKsv));
    }

    return bConnected;
}

CBIOS_VOID cbHDMIMonitor_UpdateModeInfo(PCBIOS_VOID pvcbe, PCBIOS_HDMI_MONITOR_CONTEXT pHDMIMonitorContext, PCBIOS_DISP_MODE_PARAMS pModeParams)
{
    PCBIOS_EXTENSION_COMMON pcbe            = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DEVICE_COMMON    pDevCommon      = pHDMIMonitorContext->pDevCommon;
    CBIOS_U32               ClockFreq       = pHDMIMonitorContext->HDMIClock;

    cbHDMIMonitor_GenerateInfoFrameData(pcbe, pHDMIMonitorContext, pModeParams);

    cbDebugPrint((MAKE_LEVEL(HDMI, DEBUG), "%s: CharR = %d.%d Mcsc!\n", FUNCTION_NAME, ClockFreq /10000, ClockFreq % 10000 / 1000));

    if(ClockFreq > 3400000)
    {
        pHDMIMonitorContext->TMDSBitClockRatio = 1;//1:40

        //When the TMDS Bit Rate is greater than 3.4Gbps, scrambling shall be enabled by the Source
        pHDMIMonitorContext->ScramblingEnable = CBIOS_TRUE;
    }
    else
    {
        pHDMIMonitorContext->TMDSBitClockRatio = 0;//1:10

        pHDMIMonitorContext->ScramblingEnable = CBIOS_FALSE;
    }

    if(pcbe->ChipCaps.bSupportReadRequest)
    {
        pHDMIMonitorContext->ReadRequestEnable = CBIOS_TRUE;
    }
    else
    {
        pHDMIMonitorContext->ReadRequestEnable = CBIOS_FALSE;
    }
}


CBIOS_VOID cbHDMIMonitor_SetMode(PCBIOS_VOID pvcbe, PCBIOS_HDMI_MONITOR_CONTEXT pHDMIMonitorContext, PCBIOS_DISP_MODE_PARAMS pModeParams)
{
    PCBIOS_EXTENSION_COMMON pcbe            = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DEVICE_COMMON    pDevCommon      = pHDMIMonitorContext->pDevCommon;
    CBIOS_MODULE_INDEX      HDMIModuleIndex = cbGetModuleIndex(pcbe, pDevCommon->DeviceType, CBIOS_MODULE_TYPE_HDMI);
    CBIOS_U8                HVPolarity      = pModeParams->TargetTiming.HVPolarity;
    CBIOS_BOOL              bHDMIDevice     = pDevCommon->EdidStruct.Attribute.IsCEA861HDMI;
    CBIOS_U32               ClockFreq       = pHDMIMonitorContext->HDMIClock;
    CBIOS_BOOL              bHDCPCapable    = CBIOS_TRUE;

    if (HDMIModuleIndex == CBIOS_MODULE_INDEX_INVALID)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDMI module index!\n", FUNCTION_NAME));
        return;
    }

    cbDIU_HDMI_SetHVSync(pcbe, HDMIModuleIndex, HVPolarity);

    if (bHDMIDevice)
    {
        // avoid HBlank being too small to not transmit any packet(32 bytes).
        if (pModeParams->TargetTiming.HorBEnd - pModeParams->TargetTiming.HorBStart <
            HDMI_DELAY_FOR_HDCP + HDMI_LEADING_GUARD_BAND_PERIOD + HDMI_MIN_CTL_PERIOD + HDMI_TRAILING_GUARD_BAND_PERIOD + 32)
        {
            bHDCPCapable = CBIOS_FALSE;
        }
        cbDIU_HDMI_SetHDCPDelay(pcbe, HDMIModuleIndex, bHDCPCapable);
        cbDIU_HDMI_SetPixelFormat(pcbe, HDMIModuleIndex, pModeParams->TargetModePara.OutputSignal);
        cbDIU_HDMI_SetColorDepth(pcbe, HDMIModuleIndex, pModeParams->BitPerComponent * 3);

        //HDMI CLK LANE is used for PHY to generate 1/4 TMDS Clock Rate when TMDS Character Rates above 340Mcsc
        if(ClockFreq > 3400000)
        {
            cbDIU_HDMI_EnableClkLane(pcbe, HDMIModuleIndex, CBIOS_TRUE);
        }
        else
        {
            cbDIU_HDMI_EnableClkLane(pcbe, HDMIModuleIndex, CBIOS_FALSE);
        }
    }
    else
    {
        cbDIU_HDMI_EnableClkLane(pcbe, HDMIModuleIndex, CBIOS_FALSE);
    }
}

CBIOS_VOID cbHDMIMonitor_OnOff(PCBIOS_VOID pvcbe, PCBIOS_HDMI_MONITOR_CONTEXT pHDMIMonitorContext, CBIOS_BOOL bOn)
{
    PCBIOS_EXTENSION_COMMON pcbe            = (PCBIOS_EXTENSION_COMMON)pvcbe;
    //CBIOS_HDAC_PARA         HDACPara        = {0};
    PCBIOS_DEVICE_COMMON    pDevCommon      = pHDMIMonitorContext->pDevCommon;
    CBIOS_MODULE_INDEX      HDMIModuleIndex = cbGetModuleIndex(pcbe, pDevCommon->DeviceType, CBIOS_MODULE_TYPE_HDMI);
    CBIOS_MODULE_INDEX      IGAIndex        = cbGetModuleIndex(pcbe, pDevCommon->DeviceType, CBIOS_MODULE_TYPE_IGA);
    PCBIOS_DISP_MODE_PARAMS pModeParams     = CBIOS_NULL;
    CBIOS_BOOL              bHDMIDevice     = pDevCommon->EdidStruct.Attribute.IsCEA861HDMI;
    CBIOS_BOOL              bSCDCPresent    = pDevCommon->EdidStruct.Attribute.HFSCDSData.IsSCDCPresent;
    PCBIOS_CEA_EXTENED_BLOCK pCEAExtData    = &pDevCommon->EdidStruct.Attribute.ExtDataBlock[VIDEO_CAPABILITY_DATA_BLOCK_TAG];
    CBIOS_BOOL              bQS             = pCEAExtData->VideoCapabilityData.bRGBQuantRange;
    CBIOS_CSC_ADJUST_PARA   CSCAdjustPara   = {0};

    if (HDMIModuleIndex == CBIOS_MODULE_INDEX_INVALID)
    {
        cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid HDMI module index!\n", FUNCTION_NAME));
        return;
    }

    if (bOn)
    {
        cbDIU_HDMI_SetModuleMode(pcbe, HDMIModuleIndex, bHDMIDevice);

        if (bHDMIDevice)
        {
            //HDACPara.DeviceId = pDevCommon->DeviceType;
            //HDACPara.Size = sizeof(CBIOS_HDAC_PARA);
            //HDACPara.bPresent = CBIOS_TRUE;
            //HDACPara.bEldValid = CBIOS_TRUE;

            cbHDMIMonitor_SetInfoFrame(pcbe, &pHDMIMonitorContext->HDMIInfoFrame, pDevCommon->DeviceType);
            //cbHDMIMonitor_SetHDACConnectStatus(pcbe, &HDACPara);

            if(bSCDCPresent)
            {
                cbHDMIMonitor_SCDC_Configure(pcbe, pHDMIMonitorContext);
            }

            if(pcbe->ChipCaps.bSupportScrambling)
            {
                cbDIU_HDMI_ConfigScrambling(pcbe, HDMIModuleIndex, pHDMIMonitorContext->ScramblingEnable);
            }

            if(pcbe->ChipCaps.bSupportReadRequest)
            {
                cbDIU_HDMI_EnableReadRequest(pcbe, HDMIModuleIndex, pHDMIMonitorContext->ReadRequestEnable);
            }
        }

        if (IGAIndex == CBIOS_MODULE_INDEX_INVALID)
        {
            cbDebugPrint((MAKE_LEVEL(HDMI, ERROR), "%s: invalid IGA module index!\n", FUNCTION_NAME));
            return;
        }
        pModeParams     = pcbe->DispMgr.pModeParams[IGAIndex];

        CSCAdjustPara.InputFormat = pModeParams->TargetModePara.DevInColorSpace;
        if(pModeParams->TargetModePara.OutputSignal != CBIOS_RGBOUTPUT)
        {
            if(pModeParams->TargetModePara.YRes >= 720)
            {
                CSCAdjustPara.OutputFormat = CSC_FMT_YCBCR709;
            }
            else
            {
                CSCAdjustPara.OutputFormat = CSC_FMT_YCBCR601;
            }
        }
        else
        {
            //If a Sink declares a selectable RGB Quantization Range (QS=1) in EDID VCDB
            //It supports the reception of either type of RGB Quantization Range(Limited Range and Full Range)
            //Here, we set CSC_FMT_RGB to send Full Range pixel values
            if(bQS)
            {
                CSCAdjustPara.OutputFormat = CSC_FMT_RGB;
            }
            else
            {
                if((pHDMIMonitorContext->HDMIInfoFrame.bIsCEAMode == CBIOS_TRUE) && (pModeParams->VICCode > 1))
                {
                    CSCAdjustPara.OutputFormat = CSC_FMT_LIMITED_RGB;
                }
                else
                {
                    CSCAdjustPara.OutputFormat = CSC_FMT_RGB;
                }
            }
        }
        CSCAdjustPara.Flag.bProgrammable = 0;
        cbDoCSCAdjust(pcbe, pDevCommon->DeviceType, &CSCAdjustPara);

        cbDIU_HDMI_EnableVideoAudio(pcbe, HDMIModuleIndex);
    }
    else
    {
        cbDIU_HDMI_DisableVideoAudio(pcbe, HDMIModuleIndex);

        if(pcbe->ChipCaps.bSupportScrambling)
        {
            cbDIU_HDMI_ConfigScrambling(pcbe, HDMIModuleIndex, CBIOS_FALSE);
        }

        if(pcbe->ChipCaps.bSupportReadRequest)
        {
            cbDIU_HDMI_EnableReadRequest(pcbe, HDMIModuleIndex, CBIOS_FALSE);
        }
    }
}

CBIOS_VOID cbHDMIMonitor_QueryAttribute(PCBIOS_VOID pvcbe, PCBIOS_HDMI_MONITOR_CONTEXT pHDMIMonitorContext, PCBiosMonitorAttribute pMonitorAttribute)
{
    PCBIOS_EXTENSION_COMMON    pcbe           = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DEVICE_COMMON       pDevCommon     = pHDMIMonitorContext->pDevCommon;
    PCBIOS_MONITOR_MISC_ATTRIB pMonitorAttrib = &(pDevCommon->EdidStruct.Attribute);

    cbTraceEnter(HDMI);

    if ((pDevCommon->CurrentMonitorType == CBIOS_MONITOR_TYPE_HDMI) && (pcbe->ChipCaps.IsSupportDeepColor))
    {
        if (pMonitorAttrib->VSDBData.IsSupport30Bit)
        {
            pMonitorAttribute->SupportBPC.IsSupport10BPC = CBIOS_TRUE;
        }

        if (pMonitorAttrib->VSDBData.IsSupport36Bit)
        {
            pMonitorAttribute->SupportBPC.IsSupport12BPC = CBIOS_TRUE;
        }

        if (pMonitorAttrib->VSDBData.IsSupport48Bit)
        {
            pMonitorAttribute->SupportBPC.IsSupport16BPC = CBIOS_TRUE;
        }
    }

    cbTraceExit(HDMI);
}

