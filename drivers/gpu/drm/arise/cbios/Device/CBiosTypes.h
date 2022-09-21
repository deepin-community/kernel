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
** CBios sharing used structures definition.
**
** NOTE:
** The hw only used structure SHOULD be add to hw layer.
******************************************************************************/

#ifndef _CBIOS_TYPES_H_
#define _CBIOS_TYPES_H_

/* from CBios.h */
#include "../CBios.h"

//
// General BITx usage definition
//
#define __BIT(n)  (1 << (n))

#ifndef BIT0
#define BIT0  __BIT(0 )
#define BIT1  __BIT(1 )
#define BIT2  __BIT(2 )
#define BIT3  __BIT(3 )
#define BIT4  __BIT(4 )
#define BIT5  __BIT(5 )
#define BIT6  __BIT(6 )
#define BIT7  __BIT(7 )
#define BIT8  __BIT(8 )
#define BIT9  __BIT(9 )
#define BIT10 __BIT(10)
#define BIT11 __BIT(11)
#define BIT12 __BIT(12)
#define BIT13 __BIT(13)
#define BIT14 __BIT(14)
#define BIT15 __BIT(15)
#define BIT16 __BIT(16)
#define BIT17 __BIT(17)
#define BIT18 __BIT(18)
#define BIT19 __BIT(19)
#define BIT20 __BIT(20)
#define BIT21 __BIT(21)
#define BIT22 __BIT(22)
#define BIT23 __BIT(23)
#define BIT24 __BIT(24)
#define BIT25 __BIT(25)
#define BIT26 __BIT(26)
#define BIT27 __BIT(27)
#define BIT28 __BIT(28)
#define BIT29 __BIT(29)
#define BIT30 __BIT(30)
#define BIT31 __BIT(31)
#endif

// VCP table size
#define VCP_MAX_REG_TABLE_SIZE    100
#define MAX_DVO_DEVICES_NUM                            2

typedef struct _MMIOREGISTER
{
    CBIOS_U32    index;
    CBIOS_U32    value;
    CBIOS_U32    length;
} MMIOREGISTER, *PMMIOREGISTER;

typedef struct _CBIOS_AUTO_TIMING_PARA
{
    CBIOS_U16   XRes;
    CBIOS_U16   YRes;
    CBIOS_U16   RefreshRate;
    CBIOS_BOOL  IsInterlace;
    CBIOS_U8    AutoTimingIndex;
}CBIOS_AUTO_TIMING_PARA, *PCBIOS_AUTO_TIMING_PARA;

typedef struct _CBIOS_TIMING_FLAGS
{
    CBIOS_U8    IsUseAutoTiming :1;
    CBIOS_U8    IsPAL           :1;
    CBIOS_U8    IsNTSC          :1;
    CBIOS_U8    IsInterlace     :1;
    CBIOS_U8    IsDSICmdMode    :1;
}CBIOS_TIMING_FLAGS, *PCBIOS_TIMING_FLAGS;

typedef struct _CBIOS_STREAM_ATTRIBUTE
{
    CBIOS_IN    PCBIOS_SURFACE_ATTRIB    pSurfaceAttr;
    CBIOS_IN    PCBIOS_WINDOW_PARA       pSrcWinPara;
    CBIOS_OUT   CBIOS_U32                dwStride;
    CBIOS_OUT   CBIOS_U32                dwBaseOffset;
    CBIOS_OUT   CBIOS_U32                PixelOffset;
}CBIOS_STREAM_ATTRIBUTE, *PCBIOS_STREAM_ATTRIBUTE;

//clock defines
typedef struct _CBIOS_CLOCK_INFO
{
    CBIOS_U16 Integer;
    CBIOS_U32 Fraction;
    CBIOS_U16 R;
    CBIOS_U16 CP;
    CBIOS_U16 PLLDiv;    //R has only 2 bits, need div again for small clk 
}CBIOS_CLOCK_INFO ,*PCBIOS_CLOCK_INFO;


typedef struct _CBIOS_GETTING_MODE_PARAMS
{
    CBIOS_IN  CBIOS_U32            Device;
    CBIOS_OUT CBIOS_U32            IGAIndex;
    CBIOS_OUT CBiosDestModeParams  DestModeParams;
    CBIOS_OUT CBIOS_TIMING_ATTRIB  DetailedTiming;
}CBIOS_GETTING_MODE_PARAMS,*PCBIOS_GETTING_MODE_PARAMS;


typedef struct _CBIOS_DVO_DEV_CONFIG
{
    CBIOS_U8    DVOTxType;
    CBIOS_U8    DVOSlaveAddr;
} CBIOS_DVO_DEV_CONFIG, *PCBIOS_DVO_DEV_CONFIG;

typedef struct _CBIOS_POST_REGISTER_TABLE
{
    CBREGISTER*    pCRDefault;
    CBREGISTER*    pSRDefault;
    CBREGISTER*    pModeExtRegDefault_TBL;

    CBIOS_U32      sizeofCRDefault;
    CBIOS_U32      sizeofSRDefault;
    CBIOS_U32      sizeofModeExtRegDefault_TBL;
} CBIOS_POST_REGISTER_TABLE;

typedef enum _CBIOS_POST_TABLE_TYPE
{
    VCP_TABLE,
    INIT_TABLE,
    RESTORE_TABLE,
    POST_TABLE_MAX
} CBIOS_POST_TABLE_TYPE;

//This definition should keep identical with VBIOS
typedef enum _VBIOS_ACTIVE_TYPE{
    VBIOS_TYPE_NONE        = 0x00,
    VBIOS_TYPE_CRT         = 0x01,
    VBIOS_TYPE_PANEL       = 0x02,
    VBIOS_TYPE_DVO         = 0x20,
    VBIOS_TYPE_DUOVIEW     = 0x80,
    VBIOS_TYPE_DP1         = 0x8000,
    VBIOS_TYPE_DP2         = 0x200,
    VBIOS_TYPE_DP3         = 0x400,
    VBIOS_TYPE_DP4         = 0x800,
} VBIOS_ACTIVE_TYPE, *PVBIOS_ACTIVE_TYPE;

typedef struct _Shadow_Info
{
    CBIOS_BOOL         bSysBiosInfoValid;
    CBIOS_U32   HDTV_TV_Config;
    CBIOS_U32   FBStartingAddr;
    CBIOS_U8    LCDPanelID;
    CBIOS_U8    FBSize;
    CBIOS_U8    DRAMDataRateIdx;
    CBIOS_U8    DRAMMode;
    CBIOS_U32   AlwaysLightOnDev;
    CBIOS_U32   AlwaysLightOnDevMask;
    CBIOS_U32   EngineClockModify;
    CBIOS_U8    LCD2PanelID; 
    CBIOS_U8    TVLayOut;
    CBIOS_U16   SSCCtrl;
    CBIOS_U16   ChipCapability;
    CBIOS_U16   LowTopAddress;
    CBIOS_U32   BootUpDev;
    CBIOS_U32   DetalBootUpDev;
    struct
    {
        CBIOS_U32   SnoopOnly   :1;
        CBIOS_U32   Reserved   :31;
    };
    CBIOS_U32   ECLK;
    CBIOS_U32   VCLK;
    CBIOS_U32   ICLK;
}Shadow_Info,*PShadow_Info;

typedef struct _CBIOS_FEATURE_SWITCH
{
    CBIOS_U32   IsEDP1Enabled           :1;
    CBIOS_U32   IsEDP2Enabled           :1;
    CBIOS_U32   IsDisableCodec1         :1;
    CBIOS_U32   IsDisableCodec2         :1;
    CBIOS_U32   IsEclkConfigEnable      :1;
    CBIOS_U32   IsEDP3Enabled           :1;
    CBIOS_U32   IsEDP4Enabled           :1;
    CBIOS_U32   HPDActiveHighEnable     :1;
    CBIOS_U32   BIUBackdoorEnable       :1;
    CBIOS_U32   RsvdFeatureSwitch       :23;
}CBIOS_FEATURE_SWITCH, *PCBIOS_FEATURE_SWITCH;

typedef struct _CBIOS_MAX_RES_CONFIG
{
    CBIOS_U16    MaxXRes;
    CBIOS_U16    MaxYRes;
    CBIOS_U16    MaxRefresh;    // unit Hz * 100
}CBIOS_MAX_RES_CONFIG, *PCBIOS_MAX_RES_CONFIG;

typedef struct _VCP_INFO
{
    CBIOS_BOOL   bUseVCP;                               //whether we use VCP or not
    CBIOS_U8     Version;
    CBIOS_U16    VCPlength;
    CBIOS_U32    BiosVersion;
    CBIOS_U16    SubVendorID;
    CBIOS_U16    SubSystemID;
    CBIOS_U32    SupportDevices;
    CBIOS_U16    BootDevPriority[16];

    CBIOS_FEATURE_SWITCH  FeatureSwitch;

    CBIOS_U8     CRTCharByte;
    CBIOS_U8     DP1DualModeCharByte;
    CBIOS_U8     DP2DualModeCharByte;
    CBIOS_U8     DP3DualModeCharByte;
    CBIOS_U8     DP4DualModeCharByte;
    CBIOS_U8     DVOCharByte;
    
    CBIOS_U8     CRTInterruptPort;
    CBIOS_U8     DVOInterruptPort;
    CBIOS_U8     GpioForEDP1Power;
    CBIOS_U8     GpioForEDP1BackLight;
    CBIOS_U8     GpioForEDP2Power;
    CBIOS_U8     GpioForEDP2BackLight;

    CBIOS_U32    EClock;
    CBIOS_U32    EVcoLow;
    CBIOS_U32    DVcoLow;
    CBIOS_U32    VClock;

    CBIOS_U8              SupportedDVODevice;
    CBIOS_DVO_DEV_CONFIG  DVODevConfig[MAX_DVO_DEVICES_NUM];
    
    CBIOS_U32    sizeofBootDevPriority;
    CBIOS_U32    sizeofCR_DEFAULT_TABLE;
    CBIOS_U32    sizeofSR_DEFAULT_TABLE;
    
    CBREGISTER*  pCR_DEFAULT_TABLE;
    CBREGISTER*  pSR_DEFAULT_TABLE;

    CBIOS_MAX_RES_CONFIG  DPMaxResConfig[4];
    CBIOS_U8     SliceNum;
    CBIOS_U32    FwVersion;    // firmware version backup
    CBIOS_U8     SignOnMsg[70];
    CBIOS_U8     VBiosEditTime[14];
}VCP_INFO, *PVCP_INFO;
#endif /* _CBIOS_TYPES_H_ */

