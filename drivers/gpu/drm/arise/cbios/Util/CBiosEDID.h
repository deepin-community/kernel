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

#ifndef _CBIOS_EDID_H_
#define _CBIOS_EDID_H_

// some data index in base EDID block
#define MONITORIDINDEX                   0x08
#define MONITORIDLENGTH                  0x04
#define ESTABLISH_TIMINGS_INDEX          0x23
#define STANDARD_TIMINGS_INDEX           0x26
#define DETAILED_TIMINGS_INDEX           0x36
#define EXTENSION_FLAG_INDEX             0x7E
#define EXTFLAGCHECKSUMINDEX             126
#define EXTFLAGCHECKSUMLENTH             2

#define EDIDTIMING                       0x00
#define EDIDTIMINGEXIT                   0xFF

// mode or format count
#define CBIOS_ESTABLISHMODECOUNT         17
#define CBIOS_STDMODECOUNT               8
#define CBIOS_DTLMODECOUNT               4
#define CBIOS_DTDTIMINGCOUNTS            6
#define MAX_SVD_COUNT                    31
#define MAX_EDID_BLOCK_NUM               8
#define MAX_HDMI_VIC_LEN                 7
#define MAX_HDMI_3D_LEN                  31
#define MAX_HDMI_3D_AUDIO_DESC_NUM       8
#define MAX_SVR_LEN                      31
#define CBIOS_HDMI_NORMAL_VIC_COUNTS     107 //for normal VICs only
#define CBIOS_HDMI_EXTENED_VIC_COUNTS    4
#define CBIOS_HDMIFORMATCOUNTS           CBIOS_HDMI_NORMAL_VIC_COUNTS + CBIOS_HDMI_EXTENED_VIC_COUNTS
#define CBIOS_HDMI_AUDIO_FORMAT_COUNTS   16
#define CBIOS_DISPLAYID_TYPE1_MODECOUNT  6

/* For support multiple segment EDID data */
/* Current we only support at most 4 segments */
#define CBIOS_EDIDSEGMENTCOUNT           4
#define CBIOS_EDIDMAXBLOCKCOUNT          (CBIOS_EDIDSEGMENTCOUNT * 2)
#define CBIOS_EDIDDATABYTE               (256*CBIOS_EDIDSEGMENTCOUNT)
#define EDID_BLOCK_SIZE_SPEC             128      /* 128 bytes per EDID spec */

#define CBIOS_3D_VIDEO_FORMAT_MASK          0x015D
#define EDID_VIDEO_INPUT_DEF_BYTE_OFFSET    0x14
#define EDID_VIDEO_INPUT_DEF_DIGITAL        BIT7

#define DID_TYPE1_TIMING_DESCRIPTOR_LENGTH  20

// CEA data block types tags definition
#define CEA_TAG                          0x02
//DisplayID data block types tags definition
#define DISPLAYID_TAG                    0x70

typedef enum _CBIOS_CEA_BLOCK_TAG
{
    RSVD_CEA_BLOCK_TAG1 = 0x00,
    AUDIO_DATA_BLOCK_TAG,
    VIDEO_DATA_BLOCK_TAG,
    VENDOR_SPECIFIC_DATA_BLOCK_TAG,
    SPEAKER_ALLOCATION_DATA_BLOCK_TAG,
    VESA_DTC_DATA_BLOCK_TAG,
    RSVD_CEA_BLOCK_TAG2,
    CEA_EXTENDED_BLOCK_TAG
}CBIOS_CEA_BLOCK_TAG;

typedef enum _CBIOS_CEA_EXTENDED_BLOCK_TAG
{
    VIDEO_CAPABILITY_DATA_BLOCK_TAG = 0x00,
    VENDOR_SPECIFIC_VIDEO_DATA_BLOCK_TAG,
    RSVD_VESA_VIDEO_DISPLAY_DEVICE_INFO_DATA_BLOCK_TAG,
    RSVD_VESA_VIDEO_DATA_BLOCK_TAG,
    RSVD_HDMI_VIDEO_DATA_BLOCK,
    COLORIMETRY_DATA_BLOCK_TAG,
    HDR_STATIC_META_DATA_BLOCK,
    HDR_DYNAMIC_META_DATA_BLOCK,
    //8-12 reserved for video-related blocks
    VIDEO_FMT_PREFERENCE_DATA_BLOCK = 0xD,
    YCBCR420_VIDEO_DATA_BLOCK,
    YCBCR420_CAP_MAP_DATA_BLOCK,
    CEA_MISC_AUDIO_FIELDS_TAG,
    VENDOR_SPECIFIC_AUDIO_DATA_BLOCK_TAG,
    RSVD_HDMI_AUDIO_DATA_BLOCK_TAG,
    //19-31 reserved for audio-related blocks
    INFOFRAME_DATA_BLOCK = 0x20,
    //33-255 reserved for general
    HF_EDID_EXTENSION_OVERRIDE_DATA_BLOCK = 0x78,
    HF_SINK_CAPABILITY_DATA_BLOCK,
    MAX_CEA_EXT_DATA_BLOCK_NUM,
}CBIOS_CEA_EXTENDED_BLOCK_TAG;

typedef enum _CBIOS_DISPLAYID_BLOCK_TAG
{
    PRODUCT_IDENTIFICATION_DATA_BLOCK_TAG = 0x00,
    DISPLAY_PARAMETERS_DATA_BLOCK_TAG,
    COLOR_CHARACTERISTICS_DATA_BLCOK_TAG,
    VIDEO_TIMING_MODES_DATA_BLOCK_TYPE1_TAG,
    VIDEO_TIMING_MODES_DATA_BLOCK_TYPE2_TAG,
    VIDEO_TIMING_MODES_DATA_BLOCK_TYPE3_TAG,
    VIDEO_TIMING_MODES_DATA_BLOCK_TYPE4_TAG,
    VESA_TIMING_STANDARD_DATA_BLOCK_TAG,
    CEA_TIMING_STANDARD_DATA_BLOCK_TAG,
    VIDEO_TIMING_RANGE_LIMITS_DATA_BLOCK_TAG,
    PRODUCT_SERIAL_NUMBER_DATA_BLOCK_TAG,
    GENERAL_PURPOSE_ASCII_STRING_DATA_BLOCK_TAG,
    DISPLAY_DEVICE_DATA_BLOCK_TAG,
    INTERFACE_POWER_SEQUENCING_DATA_BLOCK_TAG,
    TRANSFER_CHARACTERISTICS_DATA_BLOCK_TAG,
    DISPLAY_INTERFACE_DATA_BLOCK_TAG,
    STEREO_DISPLAY_INTERFACE_DATA_BLOCK_TAG,
    VIDEO_TIMING_MODES_DATA_BLOCK_TYPE5_TAG,
    TILED_DISPLAY_TOPOLOGY_DATA_BLOCK_TAG,
    VIDEO_TIMING_MODES_DATA_BLOCK_TYPE6_TAG,
    DISPLAYID_VENDOR_SPECIFIC_DATA_BLOCK_TAG = 0x7F,
}CBIOS_DISPLAYID_BLOCK_TAG;

typedef enum _CBIOS_SVR_FLAG
{
    SVR_FLAG_RSVD = 0,
    SVR_FLAG_VIC,
    SVR_FLAG_DTD_INDEX,
}CBIOS_SVR_FLAG;

typedef struct _CBIOS_Module_EDID_ESTTIMINGS
{
     CBIOS_U16    XResolution;       //Horizontal size
     CBIOS_U16    YResolution;       //Vertical Size
     CBIOS_U16    RefreshRate;       //Refresh rate
}CBIOS_Module_EDID_ESTTIMINGS;

typedef struct _CBIOS_EDID_DETAILEDTIMING_TABLE
{
    CBIOS_U8   type;
    CBIOS_U8   index;
    CBIOS_U8   mask;
}DETAILEDTIMING_TABLE;

typedef struct _CBIOS_HDMI_FORMAT_DESCRIPTOR
{
    CBIOS_U8 IsNative;
    CBIOS_U8 IsSupported;
    CBIOS_U8 BlockIndex;    /* Currently support only 4 blocks */
    CBIOS_U8 RefreshIndex;
    union
    {
        HDMI_3D_STUCTURE_ALL    Video3DSupportStructs;
        CBIOS_U16               Video3DSupportCaps;
    };
    struct
    {
        CBIOS_U8    IsSupportYCbCr420       :1;
        CBIOS_U8    IsSupportOtherFormats   :1; /* RGB, YCbCr4:4:4, YCbCr4:2:2 */
        CBIOS_U8    RsvdBits                :6;
    };
    CBIOS_U8 SVDIndexInVideoBlock;
}CBIOS_HDMI_FORMAT_DESCRIPTOR, *PCBIOS_HDMI_FORMAT_DESCRIPTOR;

typedef struct _CBIOS_HDMI_3D_FORMAT
{
    struct
    {
        CBIOS_U8    HDMI3DStructure             :4;
        CBIOS_U8    HDMI2DVICOrder              :4;
    };
    struct
    {
        CBIOS_U8    RsvdBits                    :4;
        CBIOS_U8    HDMI3DDetail                :4;
    };
}CBIOS_HDMI_3D_FORMAT, *PCBIOS_HDMI_3D_FORMAT;


typedef struct _CBIOS_HDMI_VSDB_EXTENTION
{
    struct
    {
        CBIOS_U8        VSDBLength              :5;
        CBIOS_U8        VSDBTag                 :3;
    }Tag;
    struct
    {
        CBIOS_U8        RegistrationIDByte0;
        CBIOS_U8        RegistrationIDByte1;
        CBIOS_U8        RegistrationIDByte2;
        CBIOS_U16       SourcePhyAddr;

    }VSDBHeader;
    union
    {
        struct
        {
            CBIOS_U8    IsSupportDualLink       :1;
            CBIOS_U8    RsvdBits0               :2;
            CBIOS_U8    IsSupportY444           :1;
            CBIOS_U8    IsSupport30Bit          :1;
            CBIOS_U8    IsSupport36Bit          :1;
            CBIOS_U8    IsSupport48Bit          :1;
            CBIOS_U8    IsSupportAI             :1;
        };
        CBIOS_U8        SupportCaps;
    };
    CBIOS_U16           MaxTMDSClock;
    union
    {
        struct
        {
            CBIOS_U8    CNC0_Graphics           :1;
            CBIOS_U8    CNC1_Photo              :1;
            CBIOS_U8    CNC2_Cinema             :1;
            CBIOS_U8    CNC3_Game               :1;
            CBIOS_U8    RsvdBits1               :1;
            CBIOS_U8    HDMIVideoPresent        :1;
            CBIOS_U8    ILatencyFieldsPresent   :1;
            CBIOS_U8    LatencyFieldsPresent    :1;
        };
        CBIOS_U8        PresentFlags;
    };
    CBIOS_U8            VideoLatency;
    CBIOS_U8            AudioLatency;
    CBIOS_U8            InterlacedVideoLatency;
    CBIOS_U8            InterlacedAudioLatency;
    union
    {
        struct
        {
            CBIOS_U8    RsvdBits2               :3;
            CBIOS_U8    ImageSize               :2;
            CBIOS_U8    HDMI3DMultiPresent      :2;
            CBIOS_U8    HDMI3DPresent           :1;
        };
        CBIOS_U8        HDMI3DPresentFlags;
    };
    struct
    {
        CBIOS_U8        HDMI3DLen               :5;
        CBIOS_U8        HDMIVICLen              :3;
    };
    CBIOS_U8            HDMIVIC[MAX_HDMI_VIC_LEN];
    CBIOS_U16           HDMI3DStructAll;
    CBIOS_U16           HDMI3DMask;
    CBIOS_HDMI_3D_FORMAT    HDMI3DForamt[MAX_HDMI_3D_LEN];
    CBIOS_U32           HDMI3DFormatCount;
}CBIOS_HDMI_VSDB_EXTENTION, *PCBIOS_HDMI_VSDB_EXTENTION;

typedef struct _CBIOS_HF_SCDS_DATA
{
    struct
    {
        CBIOS_U8        Length    :5;
        CBIOS_U8        CTATag    :3;
    }Tag;

    union
    {
        struct // for HF-VSDB
        {
            CBIOS_U8    IEEEOUIByte0;
            CBIOS_U8    IEEEOUIByte1;
            CBIOS_U8    IEEEOUIByte2;
        }HFVSDBOUI;

        struct // for HF-SCDB
        {
            CBIOS_U8    ExtTagCode;
            CBIOS_U16   Reserved;
        };
    };

    // Sink Capability Data Structure (SCDS)
    CBIOS_U8            Version;
    CBIOS_U16           MaxTMDSCharacterRate;
    union
    {
        struct
        {
            CBIOS_U16   IsSupport3DOSDDisparity         :1;
            CBIOS_U16   IsSupport3DDualView             :1;
            CBIOS_U16   IsSupport3DIndependentView      :1;
            CBIOS_U16   IsSupportLTE340McscScramble     :1;
            CBIOS_U16   RsvdBits1                       :1;
            CBIOS_U16   RsvdBits2                       :1;
            CBIOS_U16   IsRRCapable                     :1;
            CBIOS_U16   IsSCDCPresent                   :1;
            CBIOS_U16   IsSupportDC30Bit420             :1;
            CBIOS_U16   IsSupportDC36Bit420             :1;
            CBIOS_U16   IsSupportDC48Bit420             :1;
            CBIOS_U16   RsvdBits3                       :5;
        };
        CBIOS_U16       SupportCaps;
    };
}CBIOS_HF_SCDS_DATA, *PCBIOS_HF_SCDS_DATA;

typedef struct _CBIOS_COLORIMETRY_DATA
{
    union
    {
        struct
        {
            CBIOS_U8        IsSupportxvYCC601       :1;
            CBIOS_U8        IsSupportxvYCC709       :1;
            CBIOS_U8        IsSupportsYCC601        :1;
            CBIOS_U8        IsSupportAdobeYCC601    :1;
            CBIOS_U8        IsSupportAdobeRGB       :1;
            CBIOS_U8        IsSupportBT2020cYCC     :1;
            CBIOS_U8        IsSupportBT2020YCC      :1;
            CBIOS_U8        IsSupportBT2020RGB      :1;
        };
        CBIOS_U8 ColorimetrySupportFlags;
    };
    union
    {
        struct
        {
            CBIOS_U8        MetaData0               :1;
            CBIOS_U8        MetaData1               :1;
            CBIOS_U8        MetaData2               :1;
            CBIOS_U8        MetaData3               :1;
            CBIOS_U8        RsvdBits2               :4;
        };
        CBIOS_U8 ColorimetryMetadataSupportFlags;
    };
}CBIOS_COLORIMETRY_DATA, *PCBIOS_COLORIMETRY_DATA;

typedef struct _CBIOS_VIDEO_CAPABILITY_DATA
{
    struct
    {
        CBIOS_U8        CEScanInfo          :2;
        CBIOS_U8        ITScanInfo          :2;
        CBIOS_U8        PTScanInfo          :2;
        CBIOS_U8        bYCCQuantRange      :1;
        CBIOS_U8        bRGBQuantRange      :1;
    };
}CBIOS_VIDEO_CAPABILITY_DATA, *PCBIOS_VIDEO_CAPABILITY_DATA;

typedef struct _CBIOS_HDMI_AUDIO_INFO
{
    CBIOS_HDMI_AUDIO_FORMAT_TYPE Format;
    CBIOS_U32                    MaxChannelNum;
    union
    {
        struct
        {
            CBIOS_U32  SR_32kHz                 :1; /* Bit0 = 1, support sample rate of 32kHz */
            CBIOS_U32  SR_44_1kHz               :1; /* Bit1 = 1, support sample rate of 44.1kHz */
            CBIOS_U32  SR_48kHz                 :1; /* Bit2 = 1, support sample rate of 48kHz */
            CBIOS_U32  SR_88_2kHz               :1; /* Bit3 = 1, support sample rate of 88.2kHz */
            CBIOS_U32  SR_96kHz                 :1; /* Bit4 = 1, support sample rate of 96kHz */
            CBIOS_U32  SR_176_4kHz              :1; /* Bit5 = 1, support sample rate of 176.4kHz */
            CBIOS_U32  SR_192kHz                :1; /* Bit6 = 1, support sample rate of 192kHz */
            CBIOS_U32  Reserved                 :25;
        }SampleRate;

        CBIOS_U32                SampleRateUnit;
    };

    union
    {
        CBIOS_U32 Unit;

        // for audio format: LPCM
        struct
        {
            CBIOS_U32  BD_16bit                 :1; /* Bit0 = 1, support bit depth of 16 bits */
            CBIOS_U32  BD_20bit                 :1; /* Bit1 = 1, support bit depth of 20 bits */
            CBIOS_U32  BD_24bit                 :1; /* Bit2 = 1, support bit depth of 24 bits */
            CBIOS_U32  Reserved                 :29;
        }BitDepth;

        // for audio format: AC-3, MPEG-1, MP3, MPED-2, AAC LC, DTS, ATRAC
        CBIOS_U32                MaxBitRate; // unit: kHz

        // for audio format: DSD, E-AC-3, DTS-HD, MLP, DST
        CBIOS_U32                AudioFormatDependValue; /* for these audio formats, this value is defined in
                                                            it's corresponding format-specific documents*/

        // for audio format: WMA Pro
        struct
        {
            CBIOS_U32  Value                    :3;
            CBIOS_U32  Reserved                 :29;
        }Profile;
    };
}CBIOS_HDMI_AUDIO_INFO, *PCBIOS_HDMI_AUDIO_INFO;

typedef struct _CBIOS_HDMI_3D_AUDIO_DATA
{
    CBIOS_U8 MaxStreamCount         :2;
    CBIOS_U8 SupportsMSNonMixed     :1;
    CBIOS_U8 Rsvd1                  :5;
    CBIOS_U8 NumHDMI3DAD            :3;
    CBIOS_U8 Rsvd2                  :5;

    CBIOS_HDMI_AUDIO_INFO HDMI3DAudioDesc[MAX_HDMI_3D_AUDIO_DESC_NUM];
}CBIOS_HDMI_3D_AUDIO_DATA, *PCBIOS_HDMI_3D_AUDIO_DATA;


typedef struct _CBIOS_CEA_EXTENED_BLOCK
{
    CBIOS_U8        BlockLength             :5;
    CBIOS_U8        BlockTag                :3;
    CBIOS_U8        ExtTag;
    union
    {
        CBIOS_COLORIMETRY_DATA          ColorimetryData;
        CBIOS_VIDEO_CAPABILITY_DATA     VideoCapabilityData;
        CBIOS_HDMI_3D_AUDIO_DATA        HDMIAudioData;
    };
}CBIOS_CEA_EXTENED_BLOCK, *PCBIOS_CEA_EXTENED_BLOCK;

typedef struct _CBIOS_CEA_SVD_DATA
{
    CBIOS_U8        SVDCount;
    CBIOS_U8        SVD[MAX_SVD_COUNT];
}CBIOS_CEA_SVD_DATA, *PCBIOS_CEA_SVD_DATA;

typedef struct _CBIOS_INFO_FRAME_CAPS
{
    CBIOS_U8 bSupport   :1;
    CBIOS_U8 Priority   :1;
}CBIOS_INFO_FRAME_CAPS, *PCBIOS_INFO_FRAME_CAPS;

typedef struct _CBIOS_INFO_FRAME_SUPPORT_CAPS
{
    CBIOS_INFO_FRAME_CAPS HDMISpecificInfoFrameCaps;
    CBIOS_INFO_FRAME_CAPS HFSpecificInfoFrameCaps;
    CBIOS_INFO_FRAME_CAPS AVIInfoFrameCaps;
    CBIOS_INFO_FRAME_CAPS SourceProductDescInfoFrameCaps;
    CBIOS_INFO_FRAME_CAPS AudioInfoFrameCaps;
    CBIOS_INFO_FRAME_CAPS MpegSourceInfoFrameCaps;
    CBIOS_INFO_FRAME_CAPS NTSCVBIInfoFrameCaps;
}CBIOS_INFO_FRAME_SUPPORT_CAPS, *PCBIOS_INFO_FRAME_SUPPORT_CAPS;

typedef struct _CBIOS_SVR_DESC
{
    CBIOS_U8 SVRFlag;
    CBIOS_U8 SVRValue;
}CBIOS_SVR_DESC, *PCBIOS_SVR_DESC;

typedef struct _CBIOS_FAKE_EDID_PARAMS
{
    CBIOS_MODE_INFO_EXT DtlTiming;
    CBIOS_BOOL          bProvideDtlTimingEDID;
    CBIOS_U8            DtlTimingEDID[16];
}CBIOS_FAKE_EDID_PARAMS, *PCBIOS_FAKE_EDID_PARAMS;

typedef struct _CBIOS_MONITOR_MISC_ATTRIB
{
    union
    {
        struct
        {
            CBIOS_U32 IsCEA861Monitor    :1;
            CBIOS_U32 IsCEA861RGB        :1;
            CBIOS_U32 IsCEA861YCbCr422   :1;
            CBIOS_U32 IsCEA861YCbCr444   :1;
            CBIOS_U32 IsCEA861Audio      :1;
            CBIOS_U32 IsCEA861UnderScan  :1;
            CBIOS_U32 IsCEA861HDMI       :1;
            CBIOS_U32 RsvdBits           :25;
        };
        CBIOS_U32     CEA861MonitorCaps;
    };
    CBIOS_U32                       TotalNumberOfNativeFormat;
    CBIOS_U8                        Tag;
    CBIOS_U8                        RevisionNumber;
    CBIOS_U8                        OffsetOfDetailedTimingBlock;
    CBIOS_HDMI_VSDB_EXTENTION       VSDBData;
    CBIOS_HF_SCDS_DATA              HFSCDSData;
    CBIOS_CEA_SVD_DATA              SVDData[CBIOS_EDIDMAXBLOCKCOUNT - 1];
    CBIOS_CEA_EXTENED_BLOCK         ExtDataBlock[MAX_CEA_EXT_DATA_BLOCK_NUM];
    CBIOS_BOOL                      bStereoViewSupport;    // stereo Viewing Support for row-interlace
    CBIOS_STEREO_VIEW               StereoViewType;        // stereo view type
    CBIOS_U8                        ManufactureName[2];
    CBIOS_U8                        ProductCode[2];
    CBIOS_U8                        MonitorName[16];
    CBIOS_U8                        SAD_Count;             // at most 15 SADs
    CBIOS_U8                        CEA_SADs[15][3];
    CBIOS_U8                        SpeakerAllocationData;
    CBIOS_U16                       MonitorHorSize;        // monitor screen image horizontal size in millimeter(mm)
    CBIOS_U16                       MonitorVerSize;        // monitor screen image vertical size in millimeter(mm)
    CBIOS_U8                        NumOfAdditionalVSIFs;  // Number of additional VSIFs that can be received simultaneously
    CBIOS_INFO_FRAME_SUPPORT_CAPS   InfoFrameSupCaps;
    CBIOS_SVR_DESC                  ShortVideoRef[MAX_SVR_LEN];
}CBIOS_MONITOR_MISC_ATTRIB, *PCBIOS_MONITOR_MISC_ATTRIB;

#define  CBIOS_DTDTIMING_BLOCK_CNT  (CBIOS_DTDTIMINGCOUNTS*2)

typedef struct _CBIOS_EDID_STRUCTURE_DATA {
    CBIOS_U8          Version;
    CBIOS_MODE_INFO EstTimings[CBIOS_ESTABLISHMODECOUNT];
    CBIOS_MODE_INFO StdTimings[CBIOS_STDMODECOUNT];
    CBIOS_MODE_INFO_EXT DtlTimings[CBIOS_DTLMODECOUNT];
    CBIOS_MONITOR_MISC_ATTRIB Attribute;
    CBIOS_HDMI_FORMAT_DESCRIPTOR HDMIFormat[CBIOS_HDMIFORMATCOUNTS];
    CBIOS_HDMI_AUDIO_INFO HDMIAudioFormat[CBIOS_HDMI_AUDIO_FORMAT_COUNTS];
    CBIOS_MODE_INFO_EXT DTDTimings[CBIOS_DTDTIMING_BLOCK_CNT]; //may meet two CEA data block(edid has 4 block)
    CBIOS_U32         TotalModeNum;    // total number of modes that supported in EDID
    CBIOS_U32         TotalHDMIAudioFormatNum; // total number of hdmi audio formats that supported in EDID
    CBIOS_MODE_INFO_EXT DisplayID_TYPE1_Timings[CBIOS_DISPLAYID_TYPE1_MODECOUNT];
} CBIOS_EDID_STRUCTURE_DATA, *PCBIOS_EDID_STRUCTURE_DATA;

CBIOS_U32 cbEDIDModule_GetExtBlockNum(CBIOS_U8 *pEDID);
CBIOS_U32 cbEDIDModule_GetMonitorAttrib(CBIOS_U8 *pEDID, PCBIOS_MONITOR_MISC_ATTRIB pMonitorAttrib, CBIOS_U32 TotalBlocks);
CBIOS_STATUS cbEDIDModule_GetMonitor3DCaps(PCBIOS_EDID_STRUCTURE_DATA pEDIDStruct,
                                           PCBIOS_MONITOR_3D_CAPABILITY_PARA p3DCapability,
                                           CBIOS_U32 IsSupport3DVideo);
CBIOS_BOOL cbEDIDModule_GetMonitorID(CBIOS_U8 *pEDID, CBIOS_U8 *pMonitorID);
CBIOS_BOOL cbEDIDModule_IsEDIDHeaderValid(CBIOS_U8  *pEDIDBuffer, CBIOS_U32 ulBufferSize);
CBIOS_BOOL cbEDIDModule_IsEDIDValid(CBIOS_U8 *pEDID);
CBIOS_BOOL cbEDIDModule_ParseEDID(CBIOS_U8 *pEDID, PCBIOS_EDID_STRUCTURE_DATA pEDIDStruct, CBIOS_U32 ulBufferSize);
CBIOS_VOID cbEDIDModule_SADPatch(PCBIOS_EDID_STRUCTURE_DATA pEDIDStruct);
CBIOS_BOOL cbEDIDModule_SearchTmInEdidStruct(CBIOS_U32 XResolution,
                                             CBIOS_U32 YResolution,
                                             CBIOS_U32 RefreshRate,
                                             CBIOS_U32 InterlaceFlag,
                                             PCBIOS_EDID_STRUCTURE_DATA pEDIDStruct,
                                             PCBIOS_U32 pTmBlock,
                                             PCBIOS_U32 pTmIndex);
CBIOS_BOOL cbEDIDModule_FakePanelEDID(PCBIOS_FAKE_EDID_PARAMS pFakeEdidParam, PCBIOS_U8 pEdid, const CBIOS_U32 EdidBufferLength);

#endif
