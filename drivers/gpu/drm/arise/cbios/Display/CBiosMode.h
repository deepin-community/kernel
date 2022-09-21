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
** CBios mode module interface prototype and parameter definition.
**
** NOTE:
** The hw dependent function or structure SHOULD NOT be added to this file.
******************************************************************************/

#ifndef _CBIOS_MODE_H_
#define _CBIOS_MODE_H_

#define CBIOS_PROGRESSIVECAP                                   0x01
#define CBIOS_INTERLACECAP                                     0x02
#define CBIOS_PROGRESSIVEVALUE                                 0x01
#define CBIOS_INTERLACEVALUE                                   0x00

#define CBIOS_ADAPTERMODE                                      0
#define CBIOS_DEVICEMODE                                       1

#define CBIOS_ASPECTRATIOCAP4B3                                0x01
#define CBIOS_ASPECTRATIOCAP16B9                               0x02
#define CBIOS_ASPECTRATIOCAP64B27                              0x03
#define CBIOS_ASPECTRATIOCAP256B135                            0x04
#define CBIOS_DEFAULTRATIO                                     0x00

#define CBIOS_NATIVEMODE                                       1
#define CBIOS_NONNATIVEMODE                                    0

#define CBIOS_PERFERREDMODE                                    BIT18
#define CBIOS_NONPERFERREDMODE                                 0

typedef enum _CBIOS_MODE_TIMING_TYPE{  //can extend to support 14 different types  
    CBIOS_MODE_NONEDID_TIMING = 0x0000,    //The mode is not from EDID
    CBIOS_MODE_EST_TIMING = 0x0001,
    CBIOS_MODE_STD_TIMING = 0x0002,
    CBIOS_MODE_DTL_TIMING = 0x0004,
    CBIOS_MODE_SVD_TIMING = 0x0008,        //Detailed timing descriptor
    CBIOS_MODE_DTD_TIMING = 0x0010,        //CBIOS_S16 video descriptor
    CBIOS_MODE_DISPLAYID_TYPE1_TIMING = 0x0020
}CBIOS_MODE_TIMING_TYPE, *PCBIOS_MODE_TIMING_TYPE;

typedef struct _CBIOS_MODE_INFO {
    union {
        struct {
#ifdef __BIG_ENDIAN__
            CBIOS_U16 XResolution;
            CBIOS_U16 YResolution;
#else
            CBIOS_U16 YResolution;
            CBIOS_U16 XResolution;
#endif
        };
        CBIOS_U32 XYResolution;
    };
    CBIOS_U16 Refreshrate;
    CBIOS_BOOL Valid;
}CBIOS_MODE_INFO, *PCBIOS_MODE_INFO;

typedef struct _CBIOS_MODE_INFO_EXT {
    union{
        struct{
#ifdef __BIG_ENDIAN__
            union{
                CBIOS_U16 XResolution;
                CBIOS_U16 HActive;
            };
            union{
                CBIOS_U16 YResolution;
                CBIOS_U16 VActive;
            };
#else            
            union{
                CBIOS_U16 YResolution;
                CBIOS_U16 VActive;
            };
            union{
                CBIOS_U16 XResolution;
                CBIOS_U16 HActive;
            };
#endif            
        };
        CBIOS_U32 XYResolution;
    };
    CBIOS_U16 HBlank;
    CBIOS_U16 HSyncOffset;
    CBIOS_U16 HSyncPulseWidth;
    CBIOS_U16 VBlank;
    CBIOS_U16 VSyncOffset;
    CBIOS_U16 VSyncPulseWidth;
    CBIOS_U16 HImageSize;
    CBIOS_U16 VImageSize;
    CBIOS_U32 PixelClock;
    CBIOS_U16 Refreshrate;
    CBIOS_BOOL Valid;
    CBIOS_U8 InterLaced;
    CBIOS_U8 VSync;
    CBIOS_U8 HSync;
    CBIOS_U8 AspectRatio;    /* 0 means default, 1 means 4:3, 2 means 16:9 */
    CBIOS_U8 IsNativeMode;
}CBIOS_MODE_INFO_EXT, *PCBIOS_MODE_INFO_EXT;

typedef struct _CBios_Equivalent_Device_Mode
{
    CBIOS_U32 XRes;
    CBIOS_U32 YRes;
    CBIOS_U32 RefRateRange[2];
    CBIOS_U32 RefRateToAdd;
}CBiosEquivalentDeviceMode, PCBiosEquivalentDeviceMode;

typedef struct _CBIOS_MODE_FILTER_PARA{
    CBIOS_BOOL bFilterInterlace;
    CBIOS_U32  MaxDclk;
}CBIOS_MODE_FILTER_PARA, *PCBIOS_MODE_FILTER_PARA;


CBIOS_BOOL cbMode_GetHVTiming(PCBIOS_VOID pvcbe,
                              CBIOS_U32 XRes,
                              CBIOS_U32 YRes,
                              CBIOS_U32 Refresh,
                              CBIOS_U32 isInterlace,
                              CBIOS_ACTIVE_TYPE Device,
                              PCBIOS_TIMING_ATTRIB pTiming);
CBIOS_U32 cbMode_GetDefaultModeList(PCBIOS_VOID pvcbe, PCBiosModeInfoExt pModeList, CBIOS_ACTIVE_TYPE Device);
CBIOS_VOID cbMode_GetAdapterModeNum(PCBIOS_VOID pvcbe, CBIOS_U32* AdapterModeNum);
CBIOS_STATUS cbMode_FillAdapterModeList(PCBIOS_VOID pvcbe, PCBiosModeInfoExt pModeList, CBIOS_U32 *pBufferSize);
CBIOS_VOID cbMode_GetFilterPara(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device, PCBIOS_MODE_FILTER_PARA pFilter);
CBIOS_U32 cbMode_GetDeviceModeList(PCBIOS_VOID pvcbe,
                                   CBIOS_ACTIVE_TYPE  Device,
                                   PCBiosModeInfoExt pModeList,
                                   CBIOS_U32 *pBufferSize,
                                   PCBIOS_MODE_FILTER_PARA pFilter);

#endif
