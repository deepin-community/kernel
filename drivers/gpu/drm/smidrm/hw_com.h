// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023, SiliconMotion Inc.


#ifndef LYNX_HW_COM_H__
#define LYNX_HW_COM_H__



/* Maximum parameters those can be saved in the mode table. */
#define MAX_MODE_TABLE_ENTRIES              60
#define PITCH(width, bpp)               (((width) * (bpp) / 8 + 15) & ~15)

/*
 * ID of the modeInfoID used in the userDataParam_t
 */
#define MODE_INFO_INDEX         0x01
#define EDID_DEVICE_I2C_ADDRESS             0xA0
#define TOTAL_EDID_REGISTERS                128
#define READ_EDID_CONTINUOUS



/* Set the alignment to 1-bit aligned. Otherwise, the memory compare used in the
   modeext to compare timing will not work. */
#pragma pack(1)

typedef enum _disp_path_t
{
    SMI0_PATH = 0,
    SMI1_PATH  = 1,
}
disp_path_t;

typedef enum _disp_control_t
{
    CHANNEL0_CTRL = 0,
    CHANNEL1_CTRL = 1,
    CHANNEL2_CTRL = 2,
    ERROR_CTRL = 3,
}
disp_control_t;

typedef enum _disp_state_t
{
    DISP_OFF = 0,
    DISP_ON  = 1,
}
disp_state_t;
typedef enum _DPMS_t
{
    DPMS_ON,
    DPMS_STANDBY,
    DPMS_SUSPEND,
    DPMS_OFF
}
DPMS_t;

typedef enum _DISP_DPMS_t
{
    DISP_DPMS_ON,
    DISP_DPMS_STANDBY,
    DISP_DPMS_SUSPEND,
    DISP_DPMS_OFF
}
DISP_DPMS_t;

typedef enum{
    INDEX_HDMI0 = 0,
    INDEX_HDMI1,
    INDEX_HDMI2,
} hdmi_index;

typedef enum{
    INDEX_DP0 = 0,
    INDEX_DP1,
    INDEX_DP_PHY
} dp_index;


typedef enum _spolarity_t
{
    NEG, /* negative */
    POS, /* positive */
}
spolarity_t;

typedef struct _mode_parameter_t
{
    /* Horizontal timing. */
    unsigned long horizontal_total;
    unsigned long horizontal_display_end;
    unsigned long horizontal_sync_start;
    unsigned long horizontal_sync_width;
    spolarity_t horizontal_sync_polarity;

    /* Vertical timing. */
    unsigned long vertical_total;
    unsigned long vertical_display_end;
    unsigned long vertical_sync_start;
    unsigned long vertical_sync_height;
    spolarity_t vertical_sync_polarity;

    /* Refresh timing. */
    unsigned long pixel_clock;
    unsigned long horizontal_frequency;
    unsigned long vertical_frequency;
    
    /* Clock Phase. This clock phase only applies to Panel. */
    spolarity_t clock_phase_polarity;

}
mode_parameter_t;

typedef struct _userDataParam_t
{
    unsigned long size;         /* Size of this parameter */
    unsigned char modeInfoID;   /* Mode information ID */
    
    /* As the modeInfoID might be expanded in the future to support more stuffs as necessary, 
       the following parameter list might be expanded in the future. */
    union
    {
        unsigned char index;    /* The index of the mode timing of the given resolution:
                                        0   - The first mode timing found in the table
                                        1   - The second mode timing found in the table
                                        etc...
                                 */
        /* Add more user data information parameters here as necessary. */
    } paramInfo;
} userDataParam_t;

/* 
 * Structure that is used as userData pointer in the logicalMode_t 
 */
typedef struct _userData_t
{
    unsigned long signature;    /* Signature of the userData pointer. 
                                   It has to be filled with user data Signature to be a valid
                                   structure pointer. The signature can be obtained by
                                   calling ddk770_getUserDataSignature function.
                                 */
    unsigned long size;         /* Size of this data structure. */
    userDataParam_t paramList;  /* List of parameters those are associated with this userData 
                                   Currently, only one modeInfoID is supported. Later on, when
                                   the ID is expanded, then the paramList might be expanded
                                   as an array. */
} 
userData_t;

typedef struct _logicalMode_t
{
    unsigned long x;            /* X resolution */
    unsigned long y;            /* Y resolution */
    unsigned long bpp;          /* Bits per pixel */
    unsigned long hz;           /* Refresh rate */

    unsigned long baseAddress;  /* Offset from beginning of frame buffer.
                                   It is used to control the starting location of a mode.
                                   Calling function must initialize this field.
                                 */

    unsigned long pitch;        /* Mode pitch in byte.
                                   If initialized to 0, setMode function will set
                                   up this field.
                                   If not zero, setMode function will use this value.
                                 */
	unsigned long valid_edid;
    disp_control_t dispCtrl;    /* SECONDARY or PRIMARY display control channel */
    
    /* These two parameters are used in the setModeEx. */
    unsigned long xLCD;         /* Panel width */
    unsigned long yLCD;         /* Panel height */
    
    void *userData;             /* Not used now, set it to 0 (for future used only) */

}
logicalMode_t;

/* Standard Timing Structure */
typedef struct _standard_timing_t
{
    unsigned char horzActive;       /* (Horizontal active pixels / 8 ) - 31 */
    struct
    {
        unsigned char refreshRate:6;    /* Refresh Rate (Hz) - 60; Range 60 --> 123 */
        unsigned char aspectRatio:2;    /* Aspect Ratio:
                                            Bit 7   Bit 6   Operation
                                              0       0        1:1      (prior Version 1.3)
                                              0       0       16:10     (Version 1.3)
                                              0       1        4:3
                                              1       0        5:4
                                              1       1       16:9
                                         */
    } stdTimingInfo;
} standard_timing_t;

/* Detailed Timing Description Structure */
typedef struct _detailed_timing_t
{
    unsigned short pixelClock;
    unsigned char horzActive;
    unsigned char horzBlanking;
    struct 
    {
        unsigned char horzBlankingMSB:4;
        unsigned char horzActiveMSB:4;
    } horzActiveBlanking;
    unsigned char vertActive;
    unsigned char vertBlanking;
    struct 
    {
        unsigned char vertBlankingMSB:4;
        unsigned char vertActiveMSB:4;
    } vertActiveBlanking;
    unsigned char horzSyncOffset;
    unsigned char horzSyncPulseWidth;
    struct
    {
        unsigned char syncWidth:4;
        unsigned char syncOffset:4;
    } verticalSyncInfo;
    struct
    {
        unsigned char vertSyncWidth:2;
        unsigned char vertSyncOffset:2;
        unsigned char horzSyncWidth:2;
        unsigned char horzSyncOffset:2;
    } syncAuxInfo;
    unsigned char horzImageSize;
    unsigned char vertImageSize;
    struct
    {
        unsigned char vertImageSizeMSB:4;
        unsigned char horzImageSizeMSB:4;
    } horzVertImageSize;
    unsigned char horzBorder;
    unsigned char vertBorder;
    struct
    {
        unsigned char interleaved:1;
        unsigned char horzSyncFlag:1;
        unsigned char vertSyncFlag:1;
        unsigned char connectionType:2;
        unsigned char stereo:2;
        unsigned char interlaced:1;
    } flags;
} detailed_timing_t;

typedef struct _color_point_t
{
    unsigned char whitePointIndex;
    struct
    {
        unsigned char whiteYLowBits:2;
        unsigned char whiteXLowBits:2;
        unsigned char reserved:4;
    } whiteLowBits;
    unsigned char whiteX;
    unsigned char whiteY;
    unsigned char gamma;
} color_point_t;

typedef struct _monitor_desc_t
{
    unsigned short flag1;               /* Flag = 0000h when block used as descriptor */
    unsigned char flag2;                /* Flag = 00h when block used as descriptor */
    unsigned char dataTypeTag;          /* Data type tag:
                                                FFh: Monitor Serial Number - Stored as ASCII, 
                                                     code page #437, <= 13bytes
                                                FEh: ASCII String - Stored as ASCII, code 
                                                     page #437, <= 13 bytes
                                                FDh: Monitor range limits, binary coded
                                                FCh: Monitor name, stored as ASCII, code page #437
                                                FBh: Descriptor contains additional color point data
                                                FAh: Descriptor contains additional Standard Timing
                                                     Identifications
                                                F9h-11h: Currently undefined
                                                10h: Dummy descriptor, used to indicate 
                                                     that the descriptor space is unused
                                                0Fh-00h: Descriptor defined by manufacturer.
                                         */
    unsigned char flag3;                /* Flag = 00h when block used as descriptor */
    union
    {
        unsigned char serialNo[13];
        unsigned char dataString[13];
        struct
        {
            unsigned char minVertRate;
            unsigned char maxVertRate;
            unsigned char minHorzFrequency;
            unsigned char maxHorzFrequency;
            unsigned char maxPixelClock;        /* Binary coded clock rate in MHz / 10 
                                                   Max Pixel Clock those are not a multiple of 10MHz
                                                   is rounded up to a multiple of 10MHz
                                                 */
            unsigned char secondaryTimingFlag;  /* 00h = No secondary timing formula supported
                                                   (Support for default GTF indicated in feature byte)
                                                   02h = Channel1 GTF curve supported
                                                   All other = Reserved for future timing formula
                                                               definitions.
                                                 */
            union
            {
                unsigned char constantStr[7];
                struct
                {
                    unsigned char reserved;
                    unsigned char startFrequency;
                    unsigned char cParam;
                    unsigned short mParam;
                    unsigned char kParam;
                    unsigned char jParam;
                } cmkjParam;
            } secondaryTimingInfo;
        } monitorRange;
        
        unsigned char monitorName[13];
        struct
        {
            color_point_t white[2];
            unsigned char reserved[3];          /* Set to (0Ah, 20h, 20h)*/
        } colorPoint;
        
        struct
        {
            standard_timing_t stdTiming[6];
            unsigned char reserved;             /* Set to 0Ah */
        } stdTimingExt;
        
        unsigned char manufactureSpecifics[13];
    } descriptor; 
}
monitor_desc_t;

/* EDID Structuve Version 1. Within version 1 itself, there are 4 revisions mentioned
   below: 
    - EDID 1.0: The original 128-byte data format
    - EDID 1.1: Added definitions for monitor descriptors as an alternate use of the space 
                originally reserved for detailed timings, as well as definitions for 
                previously unused fields.
    - EDID 1.2: Added definitions to existing data fields.
    - EDID 1.3: Added definitions for secondary GTF curve coefficients. This is the new
                baseline for EDID data structures, which is recommended for all new monitor
                designs.
 */
typedef struct _edid_version_1_t
{
    /* Header (8 bytes) */
    unsigned char header[8];            /* Header */
    
    /* Vendor / Product ID (10 bytes) */
    unsigned short manufacturerID;      /* Manufacture Identification */
    unsigned short productCode;         /* Product code Identification */ 
    unsigned long serialNumber;         /* Serial Number */
    unsigned char weekOfManufacture;    /* Week of Manufacture */
    unsigned char yearOfManufacture;    /* Year of Manufacture */
    
    /* EDID Structure version / revision (2 bytes) */
    unsigned char version;              /* EDID Structure Version no. */
    unsigned char revision;             /* Revision Version no. */
    
    /* Basic Display Parameters / Features (5 bytes) */
    union
    {
        struct
        {
            unsigned char vsyncSerration:1;
            unsigned char syncOnGreenSupport:1;
            unsigned char compositeSyncSupport:1;
            unsigned char separateSyncSupport:1;
            unsigned char blank2Black:1;
            unsigned char signalLevelStd:2;
            unsigned char inputSignal:1;    /* Input Signal: Analog = 0, Digital = 1 */
        } analogSignal;
        struct
        {
            unsigned char dfp1Support:1;
            unsigned char reserved:6;
            unsigned char inputSignal:1;    /* Input Signal: Analog = 0, Digital = 1 */
        } digitalSignal;
    } videoInputDefinition;             /* Video Input Definition */

    unsigned char maxHorzImageSize;     /* Maximum Horizontal Image Size */
    unsigned char maxVertImageSize;     /* Maximum Vertical Image Size */
    unsigned char displayTransferChar;  /* Display Transfer Characteristic (Gamma) */
    struct
    {
        unsigned char defaultGTFSupport:1;  /* Support timings based on the GTF standard using
                                               default GTF parameters 
                                             */ 
        unsigned char preferredTiming:1;    /* If set, the display's preferred timing mode is
                                               indicated in the first detailed timing block.
                                             */
        unsigned char sRGBSupport:1;        /* If set, the display uses the sRGB standard
                                               default color space as its primary color space
                                             */
        unsigned char displayType:2;        /* Display Type:
                                                    00  - Monochrome / Grayscale display
                                                    01  - RGB Color Display
                                                    10  - Non-RGB multicolor display, e.g. R/G/Y
                                                    11  - Undefined
                                             */
        unsigned char lowPowerSupport:1;    /* If set, the display consumes much less power when
                                               it receives a timing signal outside its active
                                               operating range. It will revert to normal if the
                                               timing returns to the normal operating range.
                                             */
        unsigned char suspendSupport:1;     /* Support Suspend as defined in VESA DPMS spec. */
        unsigned char standbySupport:1;     /* Support Standby as defined in VESA DPMS spec. */
    } featureSupport;                   /* Feature Support */
    
    /* Color Characteristics (10 bytes) */
    struct
    {
        unsigned char greenYLowBits:2;
        unsigned char greenXLowBits:2;
        unsigned char redYLowBits:2;
        unsigned char redXLowBits:2;
    } redGreenLowBits;                  /* Red/Green Low Bits */
    struct
    {
        unsigned char whiteYLowBits:2;
        unsigned char whiteXLowBits:2;
        unsigned char blueYLowBits:2;
        unsigned char blueXLowBits:2;
    } blueWhiteLowBits;                 /* Blue/White Low Bits */
    unsigned char redX;                 /* Red-x bits 9-2 */
    unsigned char redY;                 /* Red-y bits 9-2 */
    unsigned char greenX;               /* Green-x bits 9-2 */
    unsigned char greenY;               /* Green-y bits 9-2 */
    unsigned char blueX;                /* Blue-x bits 9-2 */
    unsigned char blueY;                /* Blue-y bits 9-2 */
    unsigned char whiteX;               /* White-x bits 9-2 */
    unsigned char whiteY;               /* White-y bits 9-2 */
    
    /* Established Timings (3 bytes) */
    unsigned char estTiming[3];         /* Established Timings */
    
    /* Standard Timing Identification (16 bytes) */
    standard_timing_t stdTiming[8];     /* Standard Timings Identification */

    /* Detailed Timing Descriptions (72 bytes) */
    union
    {
        detailed_timing_t detailTiming[4];  /* Detailed Timing Descriptor */
        monitor_desc_t monitorDesc[4];  /* Monitor descriptor */
    } miscInformation;
    
    /* Extension Flag (1 byte) */
    unsigned char extFlag;              /* Number of (optional) 1280byte EDID
                                           extension blocks to follow. 
                                         */
                                         
    /* Checksum (1 byte) */
    unsigned char checksum;             /* The 1-byte sum of all 128 bytes in 
                                           this EDID block shall equal zero. 
                                         */
}
edid_version_1_t;

/* Restore alignment */
#pragma pack()

#endif

