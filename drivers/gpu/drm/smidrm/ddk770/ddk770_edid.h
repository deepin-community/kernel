#ifndef _DDK770_EDID_H_
#define _DDK770_EDID_H_

#include "ddk770_display.h"
#include "../hw_com.h"

#define CEA_EXT     0x02
#define VENDOR_BLOCK    0x03

#define HDMI_IEEE_OUI 0x000c03

/* Sync polarity */
typedef enum _vdif_sync_polarity_t
{
    VDIF_SYNC_NEGATIVE = 0,
    VDIF_SYNC_POSITIVE
} vdif_sync_polarity_t; 

/* Scan type */
typedef enum _vdif_scan_type_t
{
    VDIF_NONINTERLACED = 0,
    VDIF_INTERLACED
} vdif_scan_type_t;

/* Monitor Timing Information */
typedef struct _video_display_information_format_t
{
    unsigned long pixelClock;
    unsigned long characterWidth;
    vdif_scan_type_t scanType; 
    
    unsigned long horizontalFrequency;
    vdif_sync_polarity_t horizontalSyncPolarity;
    unsigned long horizontalTotal;
    unsigned long horizontalActive;
    unsigned long horizontalBlankStart;
    unsigned long horizontalBlankTime;
    unsigned long horizontalSyncStart;
    unsigned long horizontalRightBorder;
    unsigned long horizontalFrontPorch;
    unsigned long horizontalSyncWidth;
    unsigned long horizontalBackPorch;
    unsigned long horizontalLeftBorder;
    
    unsigned long verticalFrequency;
    vdif_sync_polarity_t verticalSyncPolarity; 
    unsigned long verticalTotal;
    unsigned long verticalActive;
    unsigned long verticalBlankStart;
    unsigned long verticalBlankTime;
    unsigned long verticalSyncStart;
    unsigned long verticalBottomBorder;
    unsigned long verticalFrontPorch;
    unsigned long verticalSyncHeight;
    unsigned long verticalBackPorch;
    unsigned long verticalTopBorder;
} vdif_t;



/* Restore alignment */
#pragma pack()

/**************************************************************
 *  Function Prototypes
 **************************************************************/

/*
 *  ddk770_edidGetVersion
 *      This function gets the EDID version
 *
 *  Input:
 *      pEDIDBuffer - Buffer that contains the EDID structure of the monitor
 *      pRevision   - Revision of the EDIE (if exist)
 *
 *  Output:
 *      Revision number of the given EDID buffer.
 */
unsigned char ddk770_edidGetVersion(
    unsigned char *pEDIDBuffer,
    unsigned char *pRevision
);

/*
 *  ddk770_edidGetProductInfo
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
long ddk770_edidGetProductInfo(
    unsigned char *pEDIDBuffer,
    char *pManufacturerName,
    unsigned short *pProductCode,
    unsigned long *pSerialNumber,
    unsigned char *pWeekOfManufacture,
    unsigned short *pYearOfManufacture
);

/*
 *  ddk770_edidCheckMonitorInputSignal
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
unsigned char ddk770_edidCheckMonitorInputSignal(
    unsigned char *pEDIDBuffer
);

/*
 *  ddk770_edidGetAnalogSignalInfo
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
long ddk770_edidGetAnalogSignalInfo(
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
 *  ddk770_edidGetDigitalSignalInfo
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
long ddk770_edidGetDigitalSignalInfo(
    unsigned char *pEDIDBuffer,
    unsigned char *pDFP1xSupport
);

/*
 *  ddk770_edidGetDisplaySize
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
long ddk770_edidGetDisplaySize(
    unsigned char *pEDIDBuffer,
    unsigned char *pMaxHorzImageSize,
    unsigned char *pMaxVertImageSize
);

/*
 *  ddk770_edidGetPowerManagementSupport
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
long ddk770_edidGetPowerManagementSupport(
    unsigned char *pEDIDBuffer,
    unsigned char *pStandBy,
    unsigned char *pSuspend,
    unsigned char *pLowPower
);

/*
 *  ddk770_edidGetDisplayType
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
unsigned char ddk770_edidGetDisplayType(
    unsigned char *pEDIDBuffer
);

/*
 *  ddk770_edidChecksRGBUsage
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
unsigned char ddk770_edidChecksRGBUsage(
    unsigned char *pEDIDBuffer
);

/*
 *  ddk770_edidIsPreferredTimingAvailable
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
unsigned char ddk770_edidIsPreferredTimingAvailable(
    unsigned char *pEDIDBuffer
);

/*
 *  ddk770_edidIsDefaultGTFSupported
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
unsigned char ddk770_edidIsDefaultGTFSupported(
    unsigned char *pEDIDBuffer
);

/*
 *  ddk770_edidGetColorCharacteristic
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
 *      To get the White color characteristic, use the ddk770_edidGetWhitePoint
 */
void ddk770_edidGetColorCharacteristic(
    unsigned char *pEDIDBuffer,
    unsigned short *pRedX,
    unsigned short *pRedY,
    unsigned short *pGreenX,
    unsigned short *pGreenY,
    unsigned short *pBlueX,
    unsigned short *pBlueY
);

/*
 *  ddk770_edidGetWhitePoint
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
long ddk770_edidGetWhitePoint(
    unsigned char *pEDIDBuffer,
    unsigned char *pWhitePointIndex,
    unsigned short *pWhiteX,
    unsigned short *pWhiteY,
    unsigned short *pWhiteGamma
);

/*
 *  ddk770_edidGetExtension
 *      This function gets the number of (optional) EDID extension blocks to follow
 *      the given EDID buffer.
 *
 *  Input:
 *      pEDIDBuffer     - Buffer that contains the EDID structure of the monitor
 *
 *  Output:
 *      Total number of EDID Extension to follow the given EDID buffer.
 */
unsigned char ddk770_edidGetExtension(
    unsigned char *pEDIDBuffer
);

/*
 *  ddk770_edidReadMonitor
 *      This function reads the EDID structure from the attached monitor
 *
 *  Input:
 *      pEDIDBuffer - Buffer that contains the EDID structure of the monitor
 *      bufferSize  - The EDID Buffer size index (usually 128-bytes)
 *      edidExtNo   - Extension Index of the EDID Structure to be read
 *      i2cNumber   - 0 = I2c0 and 1 = 12c1
 *
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long ddk770_edidReadMonitor(
    unsigned char *pEDIDBuffer,
    unsigned long bufferSize,
    unsigned char edidExtNo,
    unsigned char i2cNumber
);

/*
 *  ddk770_edidReadMonitor
 *      This function reads the EDID structure from the attached monitor
 *
 *  Input:
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
long ddk770_edidReadMonitorEx(
    unsigned char *pEDIDBuffer,
    unsigned long bufferSize,
    unsigned char edidExtNo,
    unsigned char sclGpio,
    unsigned char sdaGpio
);

/*
 *  This function is same as editReadMonitorEx(), but using HW I2C.
 *
 *  Input:
 *      pEDIDBuffer - Buffer that contains the EDID structure of the monitor
 *      bufferSize  - The EDID Buffer size index (usually 128-bytes)
 *      edidExtNo   - Extension Index of the EDID Structure to be read
 *      i2cNumber   - 0 = I2C0 and 1 = I2C1
 *
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long ddk770_edidReadMonitorExHwI2C(
    unsigned char *pEDIDBuffer,
    unsigned long bufferSize,
    unsigned char edidExtNo,
    unsigned char i2cNumber
);

/*
 *  ddk770_edidGetEstablishedTiming
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
long ddk770_edidGetEstablishedTiming(
    unsigned char *pEDIDBuffer,
    /*unsigned char *pEstTableIndex,*/
    unsigned char *pIndex,
    unsigned long *pWidth,
    unsigned long *pHeight,
    unsigned long *pRefreshRate,
    unsigned char *pSource
);

/*
 *  ddk770_edidGetStandardTiming
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
long ddk770_edidGetStandardTiming(
    unsigned char *pEDIDBuffer,
    unsigned char *pStdTimingIndex,
    unsigned long *pWidth,
    unsigned long *pHeight,
    unsigned long *pRefreshRate
);

/*
 *  ddk770_edidGetDetailedTiming
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
long ddk770_edidGetDetailedTiming(
    unsigned char *pEDIDBuffer,
    unsigned char *pDetailedTimingIndex,
    vdif_t *pVDIF
);

/*
 *  ddk770_edidGetMonitorSerialNumber
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
long ddk770_edidGetMonitorSerialNumber(
    unsigned char *pEDIDBuffer,
    char *pMonitorSerialNumber,
    unsigned char bufferSize
);

/*
 *  ddk770_edidGetDataString
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
long ddk770_edidGetDataString(
    unsigned char *pEDIDBuffer,
    char *pDataString,
    unsigned char bufferSize
);

/*
 *  ddk770_edidGetMonitorRangeLimit
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
long ddk770_edidGetMonitorRangeLimit(
    unsigned char *pEDIDBuffer,
    unsigned char *pMinVerticalRate,
    unsigned char *pMaxVerticalRate,
    unsigned char *pMinHorzFreq,
    unsigned char *pMaxHorzFreq,
    unsigned long *pMaxPixelClock
);

/*
 *  ddk770_edidGetChannel1TimingSupport
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
long ddk770_edidGetChannel1TimingSupport(
    unsigned char *pEDIDBuffer,
    unsigned short *pStartFrequency,
    unsigned char *pOffset,
    unsigned short *pGradient,
    unsigned char *pScalingFactor,
    unsigned char *pScalingFactorWeight
);

/*
 *  ddk770_edidGetMonitorName
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
long ddk770_edidGetMonitorName(
    unsigned char *pEDIDBuffer,
    char *pMonitorName,
    unsigned char bufferSize
);

/*
 *  ddk770_edidGetPreferredTiming
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
long ddk770_edidGetPreferredTiming(
    unsigned char *pEDIDBuffer,
    unsigned long *pWidth,
    unsigned long *pHeight,
    unsigned long *pVerticalFrequency
);

#endif  /* _EDID_H_ */



