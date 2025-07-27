#ifndef _EDID_H_
#define _EDID_H_

#include "ddk750_display.h"
#include "vdif.h"
#include "../hw_com.h"

/* Set the alignment to 1-bit aligned. Otherwise, it can't get the EDID correctly. */
#pragma pack(1)

#if 0   /* Temporary on hold. To be completed later. */

/* EDID Version 2.0. Not widely used. Usually defined for displays that follow 
   the original VESA Plug & Display (P&D) and Flat Panel Display Interface-2 (FPDI-2)
   Standards
 */
typedef struct _edid_version_2_t
{
    /* 
     * EDID Structure Version/Revision (1 byte) 
     */
    unsigned char version:4;
    unsigned char revision:4;
    
    /* 
     * Vendor / Product ID (7 bytes) 
     */
    unsigned short manufacturerID;              /* ID Manufacturer Name */
    unsigned short productID;                   /* ID Product Code */
    unsigned char manufactureWeek;              /* Week of Manufacture */
    unsigned short manufactureYear;             /* Year of Manufacture */
    
    /* 
     * Manufacturer/Product ID string (32 bytes) 
     */
    unsigned char productName;                  /* This string is compiresed of both manufacture name 
                                                   and model name, which are separated using ASCII code 09h.
                                                   If the entire string is <32 bytes, then it is terminated
                                                   with ASCII code 0Ah and the field is padded with ASCII 
                                                   code 20h 
                                                 */
                                             
    /* 
     * Serial number string (16 bytes) 
     */
    unsigned char serianNumber[16];             /* Serial number string. If <16 bytes then the string is
                                                   terminated with ASCII code 0Ah and the field padded
                                                   with ASCII code 20h
                                                 */
                                             
    /* 
     * Unused/Reserved (8 bytes) 
     */
    unsigned char reserved[8];
    
    /* 
     * Display Interface Parameters (15 bytes) 
     */
    unsigned char secondaryPhysicalIF:4;        /* Look at defaultPhysicalIF description above. */
    unsigned char defaultPhysicalIF:4;          /* Physical Connector Types. Consists of 2 fields. Bit 7-4
                                                   indicates the connector for the default interface. If
                                                   a secondary interface is available, its connector is
                                                   indicated using bit 3-0. Both fields value description
                                                   is listed below:
                                                        0 = None (Not valid for default connector)
                                                        1 = BNC
                                                        2 = 15 pin VGA
                                                        3 = 13w3
                                                        4 = VESA EVC
                                                        5 = VESA P&D-D
                                                        6 = Micro-ribbon Connector (per the VESA P&D standard)
                                                        7 = IEEE-1394 connector
                                                        8 = VESA FPDI-2
                                                        9-E = Reserved
                                                        F = Non-standard connector
                                                 */
    unsigned char secondaryVideoIF:4;           /* Look at the defaultVideoIF description above. */
    unsigned char defaultVideoIF:4;             /* Video Interface Types. Consists of 2 fields. Bit 7-4
                                                   defines the default video interface. Bit 3-0 defines
                                                   the secondary interface. Descriptions of the field is
                                                   listed below:
                                                        0 = None (Not valid for default interface)
                                                        1 = Analog
                                                        2 = Analog w/ sampled pixel clock
                                                        3 = TMDS (Transition Minimized Differential Signaling)
                                                        4 = IEEE-1394-1995
                                                        5 = LVDS
                                                        6 = Parallel
                                                        7-F = Reserved
                                                 */
    unsigned char analogIFDataFormat[4];        /* Analog Interface Data Format */
    unsigned char digitalIFDataFormat[4];       /* Digital Interface Data Format */
    unsigned char secondaryColorEncodingIF:4;   /* Secondary Color Encoding Interface */
    unsigned char defaultColorEncodingIF:4;     /* Color Encoding default Interface */
    unsigned char primarySubChannel1:4;         /* Supported bit-depth of sub-channel 1 ("Green") 
                                                   of the Primary interface. */
    unsigned char primarySubChannel0:4;         /* Supported bit-depth of sub-channel 0 ("Red") 
                                                   of the Primary interface. */
    unsigned char primarySubChannel3:4;         /* Supported bit-depth of sub-channel 3 of the 
                                                   Primary interface. */
    unsigned char primarySubChannel2:4;         /* Supported bit-depth of sub-channel 2 ("Blue") 
                                                   of the Primary interface. */
    unsigned char secondarySubChannel1:4;       /* Supported bit-depth of sub-channel 1 ("Green") 
                                                   of the Secondary interface. */
    unsigned char secondarySubChannel0:4;       /* Supported bit-depth of sub-channel 0 ("Red") 
                                                   of the Secondary interface. */
    unsigned char secondarySubChannel3:4;       /* Supported bit-depth of sub-channel 3 of the 
                                                   Secondary interface. */
    unsigned char secondarySubChannel2:4;       /* Supported bit-depth of sub-channel 2 ("Blue") 
                                                   of the Secondary interface. */
                                                   
    /* 
     * Display Device Description (5 bytes) 
     */
    unsigned char displayType;                  /* Display technology type/subtype*/
    unsigned char displayCharacteristic;        /* Major display characteristics. */
    unsigned char featureSupport[3];            /* Feature Support */
    
    /* 
     * Display Response Time (2 bytes) 
     */
    unsigned char riseTimeResponse:4;           /* Rise time response in seconds */
    unsigned char riseTimeExponent:4;           /* Rise time exponent */
    unsigned char fallTimeResponse:4;           /* Fall time response in seconds */
    unsigned char fallTimeExponent:4;           /* Fall time exponent */
    
    /* 
     * Color / Luminance Description (28 bytes) 
     */
    unsigned char whiteGamme;                   /* (Gamma x 100) - 100, [range 1.00 --> 3.55] */
    unsigned char redGamma;                     /* Color 0 ("red") Gamma (optional). Set to ffh if unused. */
    unsigned char greenGamma;                   /* Color 1 ("green") Gamma (optional). Set to ffh if unused. */
    unsigned char blueGamma;                    /* Color 2 ("blue") Gamma (optional). Set to ffh if unused. */
    unsigned short maxLuminance;                /* Maximum LUminance (white) in units of cd/m^2*10 */
    unsigned char colorConfig;                  /* Standard RGB Model, adjustable Gamma and its offset */
    unsigned char offsetValue;                  /* Color Offset value */
    
    /* Calorimetry and White Point(s) */
    unsigned char redGreenLowBits;              /* Red / Green Low Bits */
}
edid_version_2_t;

#endif

/* Restore alignment */
#pragma pack()

/**************************************************************
 *  Function Prototypes
 **************************************************************/

/*
 *  edidGetVersion
 *      This function gets the EDID version
 *
 *  Input:
 *      pEDIDBuffer - Buffer that contains the EDID structure of the monitor
 *      pRevision   - Revision of the EDIE (if exist)
 *
 *  Output:
 *      Revision number of the given EDID buffer.
 */
unsigned char edidGetVersion(
    unsigned char *pEDIDBuffer,
    unsigned char *pRevision
);

/*
 *  edidGetProductInfo
 *      This function gets the vendor and product information.
 *
 *  Input:
 *      pEDIDBuffer         - Buffer that contains the EDID structure of the monitor [in]
 *      pManufacturerName   - Pointer to a 3 byte length variable to store the manufacturer name [out]
 *      pProductCode        - Pointer to a variable to store the product code [out]
 *      pSerialNumber       - Pointer to a variable to store the serial number [out]
 *      pWeekOfManufacture  - Pointer to a variable to store the week of manufacture [out]
 *      pYearOfManufacture  - Pointer to a variable to store the year of manufacture 
 *                            or model year (if WeekOfManufacture is 0xff) [out]
 *
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long edidGetProductInfo(
    unsigned char *pEDIDBuffer,
    char *pManufacturerName,
    unsigned short *pProductCode,
    unsigned long *pSerialNumber,
    unsigned char *pWeekOfManufacture,
    unsigned short *pYearOfManufacture
);

/*
 *  edidCheckMonitorInputSignal
 *      This function checks whether the monitor is expected analog/digital 
 *      input signal.
 *
 *  Input:
 *      pEDIDBuffer     - Buffer that contains the EDID structure of the monitor
 *
 *  Output:
 *      0   - Analog
 *      1   - Digital
 */
unsigned char edidCheckMonitorInputSignal(
    unsigned char *pEDIDBuffer
);

/*
 *  edidGetAnalogSignalInfo
 *      This function gets the analog video input signal information
 *
 *  Input:
 *      pEDIDBuffer             - Buffer that contains the EDID structure of the monitor
 *      pRefWhiteAboveBlank     - Pointer to a variable to store the reference white above blank
 *                                value. The value is in milliVolt.
 *      pSyncLevelBelowBlank    - Pointer to a variable to store the Sync tip level below blank
 *                                The value is also in milliVolt
 *      pBlank2BlackSetup       - Pointer to a variable to store the Blank to black setup or
 *                                pedestal per appropriate Signal Level Standard flag. 
 *                                1 means that the display expect the setup.
 *      pSeparateSyncSupport    - Pointer to a variable to store the flag to indicate that the
 *                                monitor supports separate sync.
 *      pCompositeSyncSupport   - Pointer to a variable to store a flag to indicate that the
 *                                monitor supports composite sync.
 *      pSyncOnGreenSupport     - Pointer to a variable to store a flag to indicate that
 *                                the monitor supports sync on green video.
 *      pVSyncSerrationRequired - Pointer to a variable to store a flag to indicate that serration
 *                                of the VSync pulse is required when composite sync or
 *                                sync-on-green video is used.
 *
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long edidGetAnalogSignalInfo(
    unsigned char *pEDIDBuffer,
    unsigned short *pRefWhiteAboveBlank,
    unsigned short *pSyncLevelBelowBlank,
    unsigned char *pBlank2BlackSetup,
    unsigned char *pSeparateSyncSupport,
    unsigned char *pCompositeSyncSupport,
    unsigned char *pSyncOnGreenSupport,
    unsigned char *pVSyncSerrationRequired
);

/*
 *  edidGetDigitalSignalInfo
 *      This function gets the digital video input signal information
 *
 *  Input:
 *      pEDIDBuffer     - Buffer that contains the EDID structure of the monitor
 *      pDFP1xSupport   - Pointer to a variable to store the flag to indicate that
 *                        the mointor interface is signal compatible with VESA
 *                        DFP 1.x TMDS CRGB, 1 pixel/clock, up to 8 bits / color
 *                        MSB aligned, DE active high
 *
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long edidGetDigitalSignalInfo(
    unsigned char *pEDIDBuffer,
    unsigned char *pDFP1xSupport
);

/*
 *  edidGetDisplaySize
 *      This function gets the display sizes in cm.
 *
 *  Input:
 *      pEDIDBuffer         - Buffer that contains the EDID structure of the monitor
 *      pMaxHorzImageSize   - Pointer to a variable to store the maximum horizontal 
 *                            image size to the nearest centimeter. A value of 0
 *                            indicates that the size is indeterminate size.
 *      pMaxVertImageSize   - Pointer to a variable to store the maximum vertical
 *                            image size to the nearest centimeter. A value of 0
 *                            indicates that the size is indeterminate size.
 *
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long edidGetDisplaySize(
    unsigned char *pEDIDBuffer,
    unsigned char *pMaxHorzImageSize,
    unsigned char *pMaxVertImageSize
);

/*
 *  edidGetPowerManagementSupport
 *      This function gets the monitor's power management support.
 *
 *  Input:
 *      pEDIDBuffer     - Buffer that contains the EDID structure of the monitor
 *      pStandBy        - Pointer to a variable to store the flag to indicate that
 *                        standby power mode is supported.
 *      pSuspend        - Pointer to a variable to store the flag to indicate that
 *                        suspend power mode is supported.
 *      pLowPower       - Pointer to a variable to store the flag to indicate that
 *                        the display consumes low power when it receives a timing
 *                        signal that is outside its declared active operating range.
 *
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long edidGetPowerManagementSupport(
    unsigned char *pEDIDBuffer,
    unsigned char *pStandBy,
    unsigned char *pSuspend,
    unsigned char *pLowPower
);

/*
 *  edidGetDisplayType
 *      This function gets the display type.
 *
 *  Input:
 *      pEDIDBuffer     - Buffer that contains the EDID structure of the monitor
 *
 *  Output:
 *      0   - Monochrome / grayscale display
 *      1   - RGB Color Display
 *      2   - Non-RGB multicolor display, e.g. R/G/Y
 *      3   - Undefined
 */
unsigned char edidGetDisplayType(
    unsigned char *pEDIDBuffer
);

/*
 *  edidChecksRGBUsage
 *      This function checks if the display is using the sRGB standard default
 *      color space as its primary color space. 
 *
 *  Input:
 *      pEDIDBuffer     - Buffer that contains the EDID structure of the monitor
 *
 *  Output:
 *      0   - Does not use sRGB as its primary color space
 *      1   - Use sRGB as its primary color space
 */
unsigned char edidChecksRGBUsage(
    unsigned char *pEDIDBuffer
);

/*
 *  edidIsPreferredTimingAvailable
 *      This function checks whether the preffered timing mode is available.
 *      Use of preferred timing mode is required by EDID structure version 1
 *      Revision 3 and higher. 
 *
 *  Input:
 *      pEDIDBuffer     - Buffer that contains the EDID structure of the monitor
 *
 *  Output:
 *      0   - Preferred Timing is not available
 *      1   - Preferred Timing is available
 */
unsigned char edidIsPreferredTimingAvailable(
    unsigned char *pEDIDBuffer
);

/*
 *  edidIsDefaultGTFSupported
 *      This function checks whether the display supports timings based on the
 *      GTF standard using default GTF parameter values. 
 *
 *  Input:
 *      pEDIDBuffer     - Buffer that contains the EDID structure of the monitor
 *
 *  Output:
 *      0   - Default GTF is not supported
 *      1   - Default GTF is supported
 */
unsigned char edidIsDefaultGTFSupported(
    unsigned char *pEDIDBuffer
);

/*
 *  edidGetColorCharacteristic
 *      This function gets the chromaticity and white point values expressed as
 *      an integer value which represents the actual value times 1000.
 *
 *  Input:
 *      pEDIDBuffer - Buffer that contains the EDID structure of the monitor
 *      pRedX   - Pointer to a variable to store the Red X values
 *      pRedY   - Pointer to a variable to store the Red Y values
 *      pGreenX - Pointer to a variable to store the Green X values
 *      pGreenY - Pointer to a variable to store the Green Y values
 *      pBlueX  - Pointer to a variable to store the Blue X values
 *      pBlueY  - Pointer to a variable to store the Blue Y values
 *
 *  Note:
 *      To get the White color characteristic, use the edidGetWhitePoint
 */
void edidGetColorCharacteristic(
    unsigned char *pEDIDBuffer,
    unsigned short *pRedX,
    unsigned short *pRedY,
    unsigned short *pGreenX,
    unsigned short *pGreenY,
    unsigned short *pBlueX,
    unsigned short *pBlueY
);

/*
 *  edidGetWhitePoint
 *      This function gets the white point.
 *      To get the default white point, set the index to 0. For multiple white point,
 *      call this function multiple times to check if more than 1 white point is supported.
 *
 *  Input:
 *      pEDIDBuffer         - Buffer that contains the EDID structure of the monitor
 *      pWhitePointIndex    - Pointer to a variable that contains the white point index 
 *                            to be retrieved.
 *      pWhiteX             - Pointer to a variable to store the White X value
 *      pWhiteY             - Pointer to a variable to store the White Y value
 *      pWhiteGamma         - Pointer to a variable to store the White Gamma value
 *
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long edidGetWhitePoint(
    unsigned char *pEDIDBuffer,
    unsigned char *pWhitePointIndex,
    unsigned short *pWhiteX,
    unsigned short *pWhiteY,
    unsigned short *pWhiteGamma
);

/*
 *  edidGetExtension
 *      This function gets the number of (optional) EDID extension blocks to follow
 *      the given EDID buffer.
 *
 *  Input:
 *      pEDIDBuffer     - Buffer that contains the EDID structure of the monitor
 *
 *  Output:
 *      Total number of EDID Extension to follow the given EDID buffer.
 */
unsigned char edidGetExtension(
    unsigned char *pEDIDBuffer
);

/*
 *  edidReadMonitor
 *      This function reads the EDID structure from the attached monitor
 *
 *  Input:
 *      displayPath - Display device which EDID to be read from.
 *      pEDIDBuffer - Buffer that contains the EDID structure of the monitor
 *      bufferSize  - The EDID Buffer size index (usually 128-bytes)
 *      edidExtNo   - Extension Index of the EDID Structure to be read
 *
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long edidReadMonitor(
    disp_path_t displayPath,
    unsigned char *pEDIDBuffer,
    unsigned long bufferSize,
    unsigned char edidExtNo
);

/*
 *  edidReadMonitor
 *      This function reads the EDID structure from the attached monitor
 *
 *  Input:
 *      displayPath - Display device which EDID to be read from.
 *      pEDIDBuffer - Buffer that contains the EDID structure of the monitor
 *      bufferSize  - The EDID Buffer size index (usually 128-bytes)
 *      edidExtNo   - Extension Index of the EDID Structure to be read
 *      sclGpio     - GPIO pin used as the I2C Clock (SCL)
 *      sdaGpio     - GPIO pin used as the I2C Data (SDA)
 *
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long ddk750_edidReadMonitorEx(
    disp_path_t displayPath,
    unsigned char *pEDIDBuffer,
    unsigned long bufferSize,
    unsigned char edidExtNo,
    unsigned char sclGpio,
    unsigned char sdaGpio
);

long ddk750_edidReadMonitorEx_HW(
    disp_path_t displayPath,
    unsigned char *pEDIDBuffer,
    unsigned long bufferSize,
    unsigned char edidExtNo
);

/*
 *  edidGetHeader
 *      This function gets the EDID Header
 *
 *  Input:
 *      pEDIDBuffer - Buffer that contains the EDID structure of the monitor
 *
 *  Output:
 *      0 get header success; -1 fail.
 */
unsigned char ddk750_edidGetHeader(
    unsigned char *pEDIDBuffer
);

long ddk750_edidHeaderReadMonitorExHwI2C(void);


/*
 *  edidGetEstablishedTiming
 *      This function gets the established timing list from the given EDID buffer,
 *      table, and timing index.
 *
 *  Input:
 *      pEDIDBuffer     - Buffer that contains the EDID structure of the monitor (In)
 *      pEstTableIndex  - Pointer to the Established Timing Table index  (In/Out)
 *      pIndex          - Pointer to the Establihsed Timing Index (In/Out)
 *      pWidth          - Pointer to a variable that to store the horizontal active / width
 *                        value of the retrieved timing (Out)
 *      pHeight         - Pointer to a variable to store the vertical active / height
 *                        value of the retrieved timing (Out)
 *      pRefreshRate    - Pointer to a variable to store the vertical frequency value
 *                        of the retrieved timing (out)
 *      pSource         - Pointer to a variable to store the standard timing source:
 *                          0 - VESA
 *                          1 - IBM
 *                          2 - Apple
 *
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long edidGetEstablishedTiming(
    unsigned char *pEDIDBuffer,
    /*unsigned char *pEstTableIndex,*/
    unsigned char *pIndex,
    unsigned long *pWidth,
    unsigned long *pHeight,
    unsigned long *pRefreshRate,
    unsigned char *pSource
);

/*
 *  edidGetStandardTiming
 *      This function gets the standard timing from the given EDID buffer and
 *      calculates the width, height, and vertical frequency from that timing.
 *
 *  Input:
 *      pEDIDBuffer     - Buffer that contains the EDID structure of the monitor
 *      pStdTimingIndex - Pointer to a standard timing index to be retrieved
 *      pWidth          - Pointer to a variable that to store the horizontal active / width
 *                        value of the retrieved timing (Out)
 *      pHeight         - Pointer to a variable to store the vertical active / height
 *                        value of the retrieved timing (Out)
 *      pRefreshRate    - Pointer to a variable to store the vertical frequency value
 *                        of the retrieved timing (out)
 *
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long edidGetStandardTiming(
    unsigned char *pEDIDBuffer,
    unsigned char *pStdTimingIndex,
    unsigned long *pWidth,
    unsigned long *pHeight,
    unsigned long *pRefreshRate
);

/*
 *  edidGetDetailedTiming
 *      This function gets the detailed timing from the given EDID buffer.
 *
 *  Input:
 *      pEDIDBuffer             - Buffer that contains the EDID structure of the monitor
 *      pDetailedTimingIndex    - Pointer to a detailed timing index to be retrieved
 *      pModeParameter          - Pointer to a mode_parameter_t structure that will be
 *                                filled with the detailed timing.
 *
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long edidGetDetailedTiming(
    unsigned char *pEDIDBuffer,
    unsigned char *pDetailedTimingIndex,
    vdif_t *pVDIF
);

/*
 *  edidGetMonitorSerialNumber
 *      This function gets the monitor serial number from the EDID structure.
 *
 *  Input:
 *      pEDIDBuffer             - Buffer that contains the EDID structure of the monitor
 *      pMonitorSerialNumber    - Pointer to a buffer to store the serial number 
 *                                retrieved from the EDID
 *      bufferSize              - The size of the buffer to store the serial number.
 *                                The maximum size required is 13 bytes.
 *
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long edidGetMonitorSerialNumber(
    unsigned char *pEDIDBuffer,
    char *pMonitorSerialNumber,
    unsigned char bufferSize
);

/*
 *  edidGetDataString
 *      This function gets the data string from the EDID structure.
 *
 *  Input:
 *      pEDIDBuffer             - Buffer that contains the EDID structure of the monitor
 *      pMonitorSerialNumber    - Pointer to a buffer to store the data string 
 *                                retrieved from the EDID
 *      bufferSize              - The size of the buffer to store the data string
 *                                The maximum size required is 13 bytes.
 *
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long edidGetDataString(
    unsigned char *pEDIDBuffer,
    char *pDataString,
    unsigned char bufferSize
);

/*
 *  edidGetMonitorRangeLimit
 *      This function gets the monitor range limits from the EDID structure.
 *      Only EDID version 1 and revision 1 or above supports this feature.
 *
 *  Input:
 *      pEDIDBuffer         - Buffer that contains the EDID structure of the monitor
 *      pMinVerticalRate    - Pointer to a variable to store the Minimum Vertical Rate (Hz)
 *      pMaxVerticalRate    - Pointer to a variable to store the Maximum Vertical Rate (Hz)
 *      pMinHorzFreq        - Pointer to a variable to store the Minimum Horz. Freq (kHz)
 *      pMaxHorzFreq        - Pointer to a variable to store the Maximum Horz. Freq (kHz)
 *      pMaxPixelClock      - Pointer to a variable to store the Maximum Pixel Clock (Hz)
 *
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long edidGetMonitorRangeLimit(
    unsigned char *pEDIDBuffer,
    unsigned char *pMinVerticalRate,
    unsigned char *pMaxVerticalRate,
    unsigned char *pMinHorzFreq,
    unsigned char *pMaxHorzFreq,
    unsigned long *pMaxPixelClock
);

/*
 *  edidGetSecondaryTimingSupport
 *      This function gets the secondary GTF timing support.
 *
 *  Input:
 *      pEDIDBuffer             - Buffer that contains the EDID structure of the monitor
 *      pStartFrequency         - Pointer to a variable to store the start frequency of 
 *                                the secondary GTF
 *      pOffset                 - Pointer to a variable to store the Offset (C) value of
 *                                the secondary GTF
 *      pGradient               - Pointer to a variable to store the Gradient (M) value of
 *                                the secondary GTF
 *      pScalingFactor          - Pointer to a variable to store the Scaling Factor (K)
 *                                value of the secondary GTF
 *      pScalingFactorWeight    - Pointer to a variable to store the Scaling Factore Weight (J)
 *                                value of the secondary GTF
 *
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long edidGetSecondaryTimingSupport(
    unsigned char *pEDIDBuffer,
    unsigned short *pStartFrequency,
    unsigned char *pOffset,
    unsigned short *pGradient,
    unsigned char *pScalingFactor,
    unsigned char *pScalingFactorWeight
);

/*
 *  edidGetMonitorName
 *      This function gets the monitor name from the EDID structure.
 *
 *  Input:
 *      pEDIDBuffer             - Buffer that contains the EDID structure of the monitor
 *      pMonitorName            - Pointer to a buffer to store the monitor name 
 *                                retrieved from the EDID
 *      bufferSize              - The size of the buffer to store the monitor name
 *                                The maximum size required is 13 bytes. 
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long edidGetMonitorName(
    unsigned char *pEDIDBuffer,
    char *pMonitorName,
    unsigned char bufferSize
);

/*
 *  edidGetPreferredTiming
 *      This function gets the preferred/native timing of the monitor
 *
 *  Input:
 *      pEDIDBuffer         - Buffer that contains the EDID structure of the monitor
 *      pWidth              - Pointer to an unsigned long buffer to store the width 
 *                            of the preferred (native) timing.
 *      pHeight             - Pointer to an unsigned long buffer to store the height
 *                            of the preferred (native) timing.
 *      pVerticalFrequency  - Pointer to an unsigned long buffer to store the refresh
 *                            rate of the preferred (native) timing.
 * 
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long edidGetPreferredTiming(
    unsigned char *pEDIDBuffer,
    unsigned long *pWidth,
    unsigned long *pHeight,
    unsigned long *pVerticalFrequency
);

#endif  /* _EDID_H_ */



