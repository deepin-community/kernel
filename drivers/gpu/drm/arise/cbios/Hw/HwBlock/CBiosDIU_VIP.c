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
** VIP hw block interface function implementation.
**
** NOTE:
**
******************************************************************************/

#include "CBiosDIU_VIP.h"
#include "CBiosChipShare.h"
#include "../CBiosHwShare.h"

typedef struct _CBIOS_ADV7611_REGISTER
{
    CBIOS_U8            SlaveAddress;
    CBIOS_U8            Offset;
    CBIOS_U8            Data;
    CBIOS_U8            DataMask;
} CBIOS_ADV7611_REGISTER, *PCBIOS_ADV7611_REGISTER;

#define ADV7611_EDID_SLAVE 0x6C
#define ADV7611_REPEATER_SLAVE 0x64
#define ADV7611_IO_SLAVE 0x98
#define ADV7611_HDMI_SLAVE 0x68
//perchip
CBIOS_VIP_MODE vipModeCapsTable[] =
{
    {1920, 1080, 6000, CBIOS_VIP_FMT_RGB444_24BIT_SDR},
    {1920, 1080, 6000, CBIOS_VIP_FMT_YCBCR444_24BIT_SDR},
    {1280, 720,  6000, CBIOS_VIP_FMT_YCBCR422_8BIT_SDR_ES},
};

static CBIOS_ADV7611_REGISTER  ADV7611RegTable[5][43] =
{
    // 0---1920x1080p@60Hz, RGB444-24bit-SDR-SS
    {
        {0x98, 0xf4, 0x80, 0xff},
        {0x98, 0xf5, 0x7c, 0xff},
        {0x98, 0xf8, 0x4c, 0xff},
        {0x98, 0xf9, 0x64, 0xff},
        {0x98, 0xfa, 0x6c, 0xff},
        {0x98, 0xfb, 0x68, 0xff},
        {0x98, 0xfd, 0x44, 0xff},
        {0x98, 0x00, 0x1e, 0x3f}, // 0x00[5:0]: VID_STD = 0x1e  1920x1080p@60Hz mode
        {0x98, 0x01, 0x05, 0x7f}, // 0x01[3:0]: PRIM_MODE = 0x5; 0x01[6:4]: V_FREQ=3'b000, vertical frequency=60Hz;
        {0x98, 0x02, 0xf2, 0xff},
        {0x98, 0x03, 0x40, 0xff},
        {0x98, 0x04, 0x42, 0xe6},
        {0x98, 0x05, 0x08, 0x1f},
        {0x98, 0x06, 0x86, 0x8f},
        {0x98, 0x0b, 0x00, 0x03},
        {0x98, 0x0c, 0x00, 0x25},
        {0x98, 0x14, 0x3f, 0x3f},
        {0x98, 0x15, 0x00, 0x1e},
        {0x98, 0x19, 0x83, 0xdf},
        {0x98, 0x33, 0x40, 0x40},
        {0x44, 0xba, 0x01, 0x03},
        {0x44, 0xc9, 0x05, 0x05},
        {0x64, 0x40, 0x81, 0xff},
        {0x68, 0x8d, 0x0b, 0xff},   //0x8d[7:0]:EQ_DYN1_LF = 0x0b,  Default LF gain equalizer settings for dynamic mode range 1
        {0x68, 0x8e, 0x80, 0xff},   //0x8e[7:0]:EQ_DYN1_HF = 0x80, Default HF gain equalizer settings for dynamic mode range 1
        {0x68, 0x96, 0x00, 0x01},   //0x96[0]:EQ_DTN_EN=1'b0, Disable HDMI equalizer dynamic mode
        {0x68, 0x9b, 0x03, 0xff},
        {0x68, 0xc1, 0x01, 0xff},
        {0x68, 0xc2, 0x01, 0xff},
        {0x68, 0xc3, 0x01, 0xff},
        {0x68, 0xc4, 0x01, 0xff},
        {0x68, 0xc5, 0x01, 0xff},
        {0x68, 0xc6, 0x01, 0xff},
        {0x68, 0xc7, 0x01, 0xff},
        {0x68, 0xc8, 0x01, 0xff},
        {0x68, 0xc9, 0x01, 0xff},
        {0x68, 0xca, 0x01, 0xff},
        {0x68, 0xcb, 0x01, 0xff},
        {0x68, 0xcc, 0x01, 0xff},
        {0x68, 0x00, 0x00, 0x07},
        {0x68, 0x83, 0x00, 0x01},
        {0x68, 0x6c, 0x06, 0xff},//hpd
        {0xff, 0xff, 0xff, 0xff},
    },
    // 1 ---1280x720p@60Hz, YCbCr422-8bit-SDR-ES
    {
        {0x98, 0xf4, 0x80, 0xff},
        {0x98, 0xf5, 0x7c, 0xff},
        {0x98, 0xf8, 0x4c, 0xff},
        {0x98, 0xf9, 0x64, 0xff},
        {0x98, 0xfa, 0x6c, 0xff},
        {0x98, 0xfb, 0x68, 0xff},
        {0x98, 0xfd, 0x44, 0xff},
        {0x98, 0x00, 0x13, 0x3f}, // 0x00[5:0]: VID_STD = 0x13  1280x720p@60Hz mode
        {0x98, 0x01, 0x05, 0x7f}, // 0x01[3:0]: PRIM_MODE = 0x5; 0x01[6:4]: V_FREQ=3'b000, vertical frequency=60Hz;
        {0x98, 0x02, 0xf4, 0xff}, // 0x02[1]: RGB_OUT=1'b0, YPbPr color space output;
        {0x98, 0x03, 0x20, 0xff}, // 0x03[7:0]: OP_FORMAT_SEL=0x20, 8bit 4:2:2 SDR ITU-656 mode output;
        {0x98, 0x04, 0x82, 0xe6}, // 0x04[7:5]: OP_CH_SEL=3'b100, P[7:0]=YCbCr8;
        {0x98, 0x05, 0x0c, 0x1f}, // 0x05[2]: AVCODE_INSERT_EN=1'b1, insert AV codes into data stream;
        //{0x98, 0x06, 0x86, 0x8f},
        {0x98, 0x0b, 0x00, 0x03},
        {0x98, 0x0c, 0x00, 0x25},
        {0x98, 0x14, 0x3f, 0x3f},
        {0x98, 0x15, 0x00, 0x1e},
        {0x98, 0x19, 0xc3, 0xdf}, // 0x19[6]: LLC_DLL_DOUBLE=1'b1, Double LLC frequency;
        {0x98, 0x33, 0x40, 0x40},
        {0x44, 0xba, 0x01, 0x03},
        {0x44, 0xc9, 0x05, 0x05},
        {0x64, 0x40, 0x81, 0xff},
        {0x68, 0x8d, 0x0b, 0xff},   //0x8d[7:0]:EQ_DYN1_LF = 0x0b,  Default LF gain equalizer settings for dynamic mode range 1
        {0x68, 0x8e, 0x80, 0xff},   //0x8e[7:0]:EQ_DYN1_HF = 0x80, Default HF gain equalizer settings for dynamic mode range 1
        {0x68, 0x96, 0x00, 0x01},   //0x96[0]:EQ_DTN_EN=1'b0, Disable HDMI equalizer dynamic mode
        {0x68, 0x9b, 0x03, 0xff},
        {0x68, 0xc1, 0x01, 0xff},
        {0x68, 0xc2, 0x01, 0xff},
        {0x68, 0xc3, 0x01, 0xff},
        {0x68, 0xc4, 0x01, 0xff},
        {0x68, 0xc5, 0x01, 0xff},
        {0x68, 0xc6, 0x01, 0xff},
        {0x68, 0xc7, 0x01, 0xff},
        {0x68, 0xc8, 0x01, 0xff},
        {0x68, 0xc9, 0x01, 0xff},
        {0x68, 0xca, 0x01, 0xff},
        {0x68, 0xcb, 0x01, 0xff},
        {0x68, 0xcc, 0x01, 0xff},
        {0x68, 0x00, 0x00, 0x07},
        {0x68, 0x83, 0x00, 0x01},
        {0x68, 0x6c, 0x06, 0xff},//hpd
        {0xff, 0xff, 0xff, 0xff},
    },
    // 2 ---800x600p@60Hz, YCbCr422-8bit-DDR-ES
    {
        {0x98, 0xf4, 0x80, 0xff},
        {0x98, 0xf5, 0x7c, 0xff},
        {0x98, 0xf8, 0x4c, 0xff},
        {0x98, 0xf9, 0x64, 0xff},
        {0x98, 0xfa, 0x6c, 0xff},
        {0x98, 0xfb, 0x68, 0xff},
        {0x98, 0xfd, 0x44, 0xff},
        {0x98, 0x00, 0x01, 0x3f}, // 0x00[5:0]: VID_STD = 0x01  800x600@60Hz mode
        {0x98, 0x01, 0x06, 0x7f}, // 0x01[3:0]: PRIM_MODE = 0x6; 0x01[6:4]: V_FREQ=3'b000, vertical frequency=60Hz;
        {0x98, 0x02, 0xf4, 0xff}, // 0x02[1]: RGB_OUT=1'b0, YPbPr color space output;
        {0x98, 0x03, 0x20, 0xff}, // 0x03[7:0]: OP_FORMAT_SEL=0x20, 8bit 4:2:2 DDR ITU-656 mode output;
        {0x98, 0x04, 0x82, 0xe6}, // 0x04[7:5]: OP_CH_SEL=3'b100, P[7:0]=YCbCr8;
        {0x98, 0x05, 0x0c, 0x1f}, // 0x05[2]: AVCODE_INSERT_EN=1'b1, insert AV codes into data stream;
        //{0x98, 0x06, 0x86, 0x8f},
        {0x98, 0x0b, 0x00, 0x03},
        {0x98, 0x0c, 0x00, 0x25},
        {0x98, 0x14, 0x3f, 0x3f},
        {0x98, 0x15, 0x00, 0x1e},
        //{0x98, 0x19, 0xc3, 0xdf}, // 0x19[6]: LLC_DLL_DOUBLE=1'b1, Double LLC frequency;
        {0x98, 0x19, 0x83, 0xdf}, // 0x19[6]: LLC_DLL_DOUBLE=1'b0, Normal LLC frequency, not double;
        {0x98, 0x33, 0x40, 0x40},
        {0x44, 0xba, 0x01, 0x03},
        {0x44, 0xc9, 0x05, 0x05},
        {0x64, 0x40, 0x81, 0xff},
        {0x68, 0x8d, 0x0b, 0xff},   //0x8d[7:0]:EQ_DYN1_LF = 0x0b,  Default LF gain equalizer settings for dynamic mode range 1
        {0x68, 0x8e, 0x80, 0xff},   //0x8e[7:0]:EQ_DYN1_HF = 0x80, Default HF gain equalizer settings for dynamic mode range 1
        {0x68, 0x96, 0x00, 0x01},   //0x96[0]:EQ_DTN_EN=1'b0, Disable HDMI equalizer dynamic mode
        {0x68, 0x9b, 0x03, 0xff},
        {0x68, 0xc1, 0x01, 0xff},
        {0x68, 0xc2, 0x01, 0xff},
        {0x68, 0xc3, 0x01, 0xff},
        {0x68, 0xc4, 0x01, 0xff},
        {0x68, 0xc5, 0x01, 0xff},
        {0x68, 0xc6, 0x01, 0xff},
        {0x68, 0xc7, 0x01, 0xff},
        {0x68, 0xc8, 0x01, 0xff},
        {0x68, 0xc9, 0x01, 0xff},
        {0x68, 0xca, 0x01, 0xff},
        {0x68, 0xcb, 0x01, 0xff},
        {0x68, 0xcc, 0x01, 0xff},
        {0x68, 0x00, 0x00, 0x07},
        {0x68, 0x83, 0x00, 0x01},
        {0x68, 0x6c, 0x06, 0xff},//hpd
        {0xff, 0xff, 0xff, 0xff},
    },
    // 3---1920x1080p@60Hz, YCbCr444-24bit-SDR-SS
    {
        {0x98, 0xf4, 0x80, 0xff},
        {0x98, 0xf5, 0x7c, 0xff},
        {0x98, 0xf8, 0x4c, 0xff},
        {0x98, 0xf9, 0x64, 0xff},
        {0x98, 0xfa, 0x6c, 0xff},
        {0x98, 0xfb, 0x68, 0xff},
        {0x98, 0xfd, 0x44, 0xff},
        {0x98, 0x00, 0x1e, 0x3f}, // 0x00[5:0]: VID_STD = 0x1e  1920x1080p@60Hz mode
        {0x98, 0x01, 0x05, 0x7f}, // 0x01[3:0]: PRIM_MODE = 0x5; 0x01[6:4]: V_FREQ=3'b000, vertical frequency=60Hz;
        {0x98, 0x02, 0xf0, 0xff}, // 0x02[1]: RGB_OUT=1'b0, YPbPr color space output;
        {0x98, 0x03, 0x40, 0xff}, // 0x03[7:0]: OP_FORMAT_SEL=0x40, 24bit 444 SDR mode output;
        {0x98, 0x04, 0x42, 0xe6}, // 0x04[7:5]: OP_CH_SEL=3'b010, P[23:0]=UYV888=CbYCr888;
        {0x98, 0x05, 0x08, 0x1f},
        {0x98, 0x06, 0x86, 0x8f},
        {0x98, 0x0b, 0x00, 0x03},
        {0x98, 0x0c, 0x00, 0x25},
        {0x98, 0x14, 0x3f, 0x3f},
        {0x98, 0x15, 0x00, 0x1e},
        {0x98, 0x19, 0x83, 0xdf},
        {0x98, 0x33, 0x40, 0x40},
        {0x44, 0xba, 0x01, 0x03},
        {0x44, 0xc9, 0x05, 0x05},
        {0x64, 0x40, 0x81, 0xff},
        {0x68, 0x8d, 0x0b, 0xff},   //0x8d[7:0]:EQ_DYN1_LF = 0x0b,  Default LF gain equalizer settings for dynamic mode range 1
        {0x68, 0x8e, 0x80, 0xff},   //0x8e[7:0]:EQ_DYN1_HF = 0x80, Default HF gain equalizer settings for dynamic mode range 1
        {0x68, 0x96, 0x00, 0x01},   //0x96[0]:EQ_DTN_EN=1'b0, Disable HDMI equalizer dynamic mode
        {0x68, 0x9b, 0x03, 0xff},
        {0x68, 0xc1, 0x01, 0xff},
        {0x68, 0xc2, 0x01, 0xff},
        {0x68, 0xc3, 0x01, 0xff},
        {0x68, 0xc4, 0x01, 0xff},
        {0x68, 0xc5, 0x01, 0xff},
        {0x68, 0xc6, 0x01, 0xff},
        {0x68, 0xc7, 0x01, 0xff},
        {0x68, 0xc8, 0x01, 0xff},
        {0x68, 0xc9, 0x01, 0xff},
        {0x68, 0xca, 0x01, 0xff},
        {0x68, 0xcb, 0x01, 0xff},
        {0x68, 0xcc, 0x01, 0xff},
        {0x68, 0x00, 0x00, 0x07},
        {0x68, 0x83, 0x00, 0x01},
        {0x68, 0x6c, 0x06, 0xff},//hpd
        {0xff, 0xff, 0xff, 0xff},
    },
    // 4 ---1280x720p@60Hz, YCbCr422-8bit-DDR-ES
    {
        {0x98, 0xf4, 0x80, 0xff},
        {0x98, 0xf5, 0x7c, 0xff},
        {0x98, 0xf8, 0x4c, 0xff},
        {0x98, 0xf9, 0x64, 0xff},
        {0x98, 0xfa, 0x6c, 0xff},
        {0x98, 0xfb, 0x68, 0xff},
        {0x98, 0xfd, 0x44, 0xff},
        {0x98, 0x00, 0x13, 0x3f}, // 0x00[5:0]: VID_STD = 0x13  1280x720p@60Hz mode
        {0x98, 0x01, 0x05, 0x7f}, // 0x01[3:0]: PRIM_MODE = 0x5; 0x01[6:4]: V_FREQ=3'b000, vertical frequency=60Hz;
        {0x98, 0x02, 0xf4, 0xff}, // 0x02[1]: RGB_OUT=1'b0, YPbPr color space output;
        {0x98, 0x03, 0x20, 0xff}, // 0x03[7:0]: OP_FORMAT_SEL=0x20, 8bit 4:2:2 DDR ITU-656 mode output;
        {0x98, 0x04, 0x82, 0xe6}, // 0x04[7:5]: OP_CH_SEL=3'b100, P[7:0]=YCbCr8;
        {0x98, 0x05, 0x0c, 0x1f}, // 0x05[2]: AVCODE_INSERT_EN=1'b1, insert AV codes into data stream;
        //{0x98, 0x06, 0x86, 0x8f},
        {0x98, 0x0b, 0x00, 0x03},
        {0x98, 0x0c, 0x00, 0x25},
        {0x98, 0x14, 0x3f, 0x3f},
        {0x98, 0x15, 0x00, 0x1e},
        {0x98, 0x19, 0x83, 0xdf}, // 0x19[6]: LLC_DLL_DOUBLE=1'b0, Normal LLC frequency, not double;
        {0x98, 0x33, 0x40, 0x40},
        {0x44, 0xba, 0x01, 0x03},
        {0x44, 0xc9, 0x05, 0x05},
        {0x64, 0x40, 0x81, 0xff},
        {0x68, 0x8d, 0x0b, 0xff},   //0x8d[7:0]:EQ_DYN1_LF = 0x0b,  Default LF gain equalizer settings for dynamic mode range 1
        {0x68, 0x8e, 0x80, 0xff},   //0x8e[7:0]:EQ_DYN1_HF = 0x80, Default HF gain equalizer settings for dynamic mode range 1
        {0x68, 0x96, 0x00, 0x01},   //0x96[0]:EQ_DTN_EN=1'b0, Disable HDMI equalizer dynamic mode
        {0x68, 0x9b, 0x03, 0xff},
        {0x68, 0xc1, 0x01, 0xff},
        {0x68, 0xc2, 0x01, 0xff},
        {0x68, 0xc3, 0x01, 0xff},
        {0x68, 0xc4, 0x01, 0xff},
        {0x68, 0xc5, 0x01, 0xff},
        {0x68, 0xc6, 0x01, 0xff},
        {0x68, 0xc7, 0x01, 0xff},
        {0x68, 0xc8, 0x01, 0xff},
        {0x68, 0xc9, 0x01, 0xff},
        {0x68, 0xca, 0x01, 0xff},
        {0x68, 0xcb, 0x01, 0xff},
        {0x68, 0xcc, 0x01, 0xff},
        {0x68, 0x00, 0x00, 0x07},
        {0x68, 0x83, 0x00, 0x01},
        {0x68, 0x6c, 0x06, 0xff},//hpd
        {0xff, 0xff, 0xff, 0xff},
    },
};

CBIOS_PARALLEL_REGISTER VIP_Parallel_Reg_Tbl[] =
{
    {VIP_REG0,  {0x33730,  0x33734,  0x33738,  0x3373C}},
    {VIP_REG1,  {0x33740,  0x33770,  0x337A0,  0x337D0}},
    {VIP_REG2,  {0x33744,  0x33774,  0x337A4,  0x337D4}},
    {VIP_REG3,  {0x33748,  0x33778,  0x337A8,  0x337D8}},
    {VIP_REG4,  {0x3374C,  0x3377C,  0x337AC,  0x337DC}},
    {VIP_REG5,  {0x33750,  0x33780,  0x337B0,  0x337E0}},
    {VIP_REG6,  {0x33754,  0x33784,  0x337B4,  0x337E4}},
    {VIP_REG7,  {0x33758,  0x33788,  0x337B8,  0x337E8}},
    {VIP_REG8,  {0x3375C,  0x3378C,  0x337BC,  0x337EC}},
    {VIP_REG9,  {0x33760,  0x33790,  0x337C0,  0x337F0}},
    {VIP_REG10, {0x33764,  0x33794,  0x337C4,  0x337F4}},
    {VIP_REG11, {0x33768,  0x33798,  0x337C8,  0x337F8}},
    {VIP_REG12, {0x3376C,  0x3379C,  0x337CC,  0x337FC}},
    {PARALLEL_TABLE_END_INDEX, {0, 0, 0, 0}},
};

CBIOS_PARALLEL_REGISTER VIP_IIC_Parallel_Reg_Tbl[] =
{
    {VIP_IIC_RW_DATA_REG,  {0x33888,  0x33890,  0x33898,  0x338A0}},
    {VIP_IIC_CONTROL_REG,  {0x3388C,  0x33894,  0x3389C,  0x338A4}},
    {VIP_IIC_IE,  {CR_EE,  CR_EF,  CR_B_C7,  CR_B_C8}},
    {PARALLEL_TABLE_END_INDEX, {0, 0, 0, 0}},
};

CBIOS_STATUS cbVIPI2CDataRead(PCBIOS_VOID pvcbe, PCBIOS_PARAM_VIPI2C_DATA pCbiosVIPI2CParams);
CBIOS_STATUS cbVIPI2CDataWrite(PCBIOS_VOID pvcbe, PCBIOS_PARAM_VIPI2C_DATA pCbiosVIPI2CParams);

CBIOS_U8 TestVIPEDID[256] =
{
  0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x59, 0x30, 0x34, 0x12, 0x00, 0x00, 0x00, 0x00, 
  0x09, 0x1E, 0x01, 0x03, 0x80, 0x30, 0x1B, 0x78, 0x2E, 0x35, 0x85, 0xA6, 0x56, 0x48, 0x9A, 0x24,
  0x12, 0x50, 0x54, 0x21, 0x08, 0x00, 0x81, 0x80, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 
  0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C, 
  0x45, 0x00, 0xDC, 0x0C, 0x11, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x31, 0x31, 0x31, 
  0x31, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x56, 
  0x49, 0x50, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x10, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x3B, 
  0x02, 0x03, 0x16, 0xF1, 0x43, 0x90, 0x04, 0x01, 0x23, 0x09, 0x06, 0x03, 0x83, 0x01, 0x00, 0x00, 
  0x65, 0x03, 0x0C, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xDF
};

CBIOS_STATUS cbAdv7611SetEDID(PCBIOS_VOID pvcbe, CBIOS_U8 VIPPortIndex, CBIOS_U8 *pEDIDBuffer, CBIOS_U32 Size)
{
    CBIOS_U32 i = 0;
    CBIOS_U8 data = 0;
    CBIOS_PARAM_VIPI2C_DATA CbiosVIPI2CParams = {0};

    if ((Size > 512) || ((Size & 0x7F) != 0) || (pEDIDBuffer == CBIOS_NULL))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "Invalid EDID!! VIPPortIndex=%d\n", VIPPortIndex));
        return CBIOS_FALSE;
    }


    //disable internal edid
    CbiosVIPI2CParams.PortNumber = VIPPortIndex;
    CbiosVIPI2CParams.SlaveAddress = ADV7611_REPEATER_SLAVE;
    CbiosVIPI2CParams.Offset = 0x77;
    data = 0x00;
    CbiosVIPI2CParams.Buffer = &data;
    CbiosVIPI2CParams.BufferLen = 1;

    cbVIPI2CDataWrite(pvcbe, &CbiosVIPI2CParams);

    
    //write EDID
    for (i = 0; i < Size; i++)
    {
        CbiosVIPI2CParams.SlaveAddress = ADV7611_EDID_SLAVE;
        CbiosVIPI2CParams.Offset = (CBIOS_U8)i;
        CbiosVIPI2CParams.Buffer = pEDIDBuffer + i;
        CbiosVIPI2CParams.BufferLen = 1; 

        cbVIPI2CDataWrite(pvcbe, &CbiosVIPI2CParams);
    }

    //enable internal EDID
    CbiosVIPI2CParams.SlaveAddress = ADV7611_REPEATER_SLAVE;
    CbiosVIPI2CParams.Offset = 0x74;
    data = 0x03;
    CbiosVIPI2CParams.Buffer = &data;
    CbiosVIPI2CParams.BufferLen = 1;

    cbVIPI2CDataWrite(pvcbe, &CbiosVIPI2CParams);

    //hotplug assert
    CbiosVIPI2CParams.SlaveAddress = ADV7611_IO_SLAVE;
    CbiosVIPI2CParams.Offset = 0x20;
    CbiosVIPI2CParams.Buffer = &data;
    CbiosVIPI2CParams.BufferLen = 1;
    cbVIPI2CDataRead(pvcbe, &CbiosVIPI2CParams);
    data |= 0x80;
    cbVIPI2CDataWrite(pvcbe, &CbiosVIPI2CParams);
    cb_DelayMicroSeconds(100000);//delay 100ms to avoid flash blue screen when source device setting mode

    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "set edid done!\n"));

    return CBIOS_TRUE;

}
CBIOS_STATUS cbSetVIPCard(PCBIOS_VOID pvcbe, PCBIOS_PARAM_VIPSETCARD_DATA pCbiosVIPSetCardParams)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_PARAM_VIPI2C_DATA CbiosVIPI2CParams = {0};
    CBIOS_U8 VIPI2CData;
    CBIOS_U32 i, j;
    CBIOS_U8 VIPPortIndex = pCbiosVIPSetCardParams->PortNumber;

    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "cbSetVIPCard: VIP=%d, %dx%d@%dHz,  VIPFormat=%d\n",
        VIPPortIndex, pCbiosVIPSetCardParams->XRes, pCbiosVIPSetCardParams->YRes, pCbiosVIPSetCardParams->RefreshRate / 100, pCbiosVIPSetCardParams->VIPFormat));

    if(VIPPortIndex > 3)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "NOT support this VIP Port: VIPPortIndex=%d\n", VIPPortIndex));
        return CBIOS_ER_NOT_YET_IMPLEMENTED;
    }

    if((pCbiosVIPSetCardParams->XRes == 1920) && (pCbiosVIPSetCardParams->YRes == 1080) && (pCbiosVIPSetCardParams->RefreshRate == 6000)
        && (pCbiosVIPSetCardParams->VIPFormat == CBIOS_VIP_FMT_RGB444_24BIT_SDR))
    {
        i = 0;
    }
    else if((pCbiosVIPSetCardParams->XRes == 1280) && (pCbiosVIPSetCardParams->YRes == 720) && (pCbiosVIPSetCardParams->RefreshRate == 6000)
        && (pCbiosVIPSetCardParams->VIPFormat == CBIOS_VIP_FMT_YCBCR422_8BIT_SDR_ES))
    {
        i = 1;
    }
    else if((pCbiosVIPSetCardParams->XRes == 800) && (pCbiosVIPSetCardParams->YRes == 600) && (pCbiosVIPSetCardParams->RefreshRate == 6000)
        && (pCbiosVIPSetCardParams->VIPFormat == CBIOS_VIP_FMT_YCBCR422_8BIT_DDR_ES))
    {
        i = 2;
    }
    else if((pCbiosVIPSetCardParams->XRes == 1920) && (pCbiosVIPSetCardParams->YRes == 1080) && (pCbiosVIPSetCardParams->RefreshRate == 6000)
        && (pCbiosVIPSetCardParams->VIPFormat == CBIOS_VIP_FMT_YCBCR444_24BIT_SDR))
    {
        i = 3;
    }
    else if((pCbiosVIPSetCardParams->XRes == 1280) && (pCbiosVIPSetCardParams->YRes == 720) && (pCbiosVIPSetCardParams->RefreshRate == 6000)
        && (pCbiosVIPSetCardParams->VIPFormat == CBIOS_VIP_FMT_YCBCR422_8BIT_DDR_ES))
    {
        i = 4;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "NOT support this format: %dx%d@%dHz,  Format=%d\n",
            pCbiosVIPSetCardParams->XRes, pCbiosVIPSetCardParams->YRes, pCbiosVIPSetCardParams->RefreshRate / 100, pCbiosVIPSetCardParams->VIPFormat));
        return CBIOS_ER_NOT_YET_IMPLEMENTED;
    }

    CbiosVIPI2CParams.PortNumber = VIPPortIndex;
    CbiosVIPI2CParams.Buffer = &VIPI2CData;
    CbiosVIPI2CParams.BufferLen = 1;    //read/write one byte

    for(j=0; ADV7611RegTable[i][j].SlaveAddress != 0xFF; j++)
    {
        CbiosVIPI2CParams.SlaveAddress = ADV7611RegTable[i][j].SlaveAddress;
        CbiosVIPI2CParams.Offset = ADV7611RegTable[i][j].Offset;

        cbVIPI2CDataRead(pcbe, &CbiosVIPI2CParams);
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "VIP=%d, I2C read, offset:0x%02x,  value:0x%02x \n",VIPPortIndex, CbiosVIPI2CParams.Offset, VIPI2CData));

        VIPI2CData = (VIPI2CData & ~ADV7611RegTable[i][j].DataMask) | (ADV7611RegTable[i][j].Data & ADV7611RegTable[i][j].DataMask);
        CbiosVIPI2CParams.Buffer = &VIPI2CData;
        cbVIPI2CDataWrite(pcbe, &CbiosVIPI2CParams);
        cb_DelayMicroSeconds(1000);//delay 1ms
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "VIP=%d, I2C write, offset:0x%02x,  data:0x%02x,  value:0x%02x \n",VIPPortIndex, CbiosVIPI2CParams.Offset, ADV7611RegTable[i][j].Data, VIPI2CData));
    }

    cbAdv7611SetEDID(pcbe, VIPPortIndex, TestVIPEDID, sizeof(TestVIPEDID));

    return CBIOS_OK;
}

CBIOS_STATUS cbAdv7611Init(PCBIOS_VOID pvcbe, CBIOS_U8 VIPPortIndex)
{

    CBIOS_PARAM_VIPI2C_DATA CbiosVIPI2CParams = {0};
    CBIOS_U8 data = 0;

    //reset
    CbiosVIPI2CParams.PortNumber = VIPPortIndex;
    CbiosVIPI2CParams.SlaveAddress = ADV7611_IO_SLAVE;
    CbiosVIPI2CParams.Offset = 0xFF;
    data = 0x80;
    CbiosVIPI2CParams.Buffer = &data;
    CbiosVIPI2CParams.BufferLen = 1;

    cbVIPI2CDataWrite(pvcbe, &CbiosVIPI2CParams);
    //wait at least 5ms
    cbDelayMilliSeconds(10);
    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "ADV7611 reset!\n"));
    //should move mode independent adv7611 init setting here

    return CBIOS_OK;
 
}

CBIOS_STATUS cbAdv7611DeInit(PCBIOS_VOID pvcbe, CBIOS_U8 VIPPortIndex)
{
    CBIOS_PARAM_VIPI2C_DATA CbiosVIPI2CParams = {0};
    CBIOS_U8 data = 0;


    //disable internal edid
    CbiosVIPI2CParams.PortNumber = VIPPortIndex;
    CbiosVIPI2CParams.SlaveAddress = ADV7611_REPEATER_SLAVE;
    CbiosVIPI2CParams.Offset = 0x74;
    data = 0x00;
    CbiosVIPI2CParams.Buffer = &data;
    CbiosVIPI2CParams.BufferLen = 1;

    cbVIPI2CDataWrite(pvcbe, &CbiosVIPI2CParams);

    //hotplug de-assert
    CbiosVIPI2CParams.SlaveAddress = ADV7611_IO_SLAVE;
    CbiosVIPI2CParams.Offset = 0x20;
    CbiosVIPI2CParams.Buffer = &data;
    CbiosVIPI2CParams.BufferLen = 1;
    cbVIPI2CDataRead(pvcbe, &CbiosVIPI2CParams);
    data &= ~0x80;
    cbVIPI2CDataWrite(pvcbe, &CbiosVIPI2CParams);

    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "hpd out, clear EDID\n"));
    
    return CBIOS_OK;
}

CBIOS_STATUS cbVIPQueryCaps(PCBIOS_VOID pvcbe, PCBIOS_VIP_CTRL_DATA pCbiosVIPCtlData)
{
    pCbiosVIPCtlData->caps.supportModeNum  = sizeof(vipModeCapsTable)/sizeof(CBIOS_VIP_MODE);
    pCbiosVIPCtlData->caps.mode  = vipModeCapsTable;

    return CBIOS_OK;
}

CBIOS_STATUS cbVIPSetMode(PCBIOS_VOID pvcbe, PCBIOS_VIP_CTRL_DATA pCbiosVIPCtlData)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U8 VIPPortIndex = pCbiosVIPCtlData->vip;
    CBIOS_U32  VIP33740RegIndex;
    CBIOS_U32  VIP33744RegIndex;
    CBIOS_U32  VIP33748RegIndex;
    CBIOS_U32  VIP3374CRegIndex;
    CBIOS_U32  VIP33750RegIndex;
    CBIOS_U32  VIP33760RegIndex;

    REG_MM33740_Arise VIP33740RegValue, VIP33740RegMask;
    REG_MM33744_Arise VIP33744RegValue, VIP33744RegMask;
    REG_MM33748_Arise VIP33748RegValue, VIP33748RegMask;
    REG_MM3374C_Arise VIP3374CRegValue, VIP3374CRegMask;
    REG_MM33750_Arise VIP33750RegValue, VIP33750RegMask;
    REG_MM33760_Arise VIP33760RegValue, VIP33760RegMask;

    CBIOS_PARAM_VIPSETCARD_DATA CbiosVIPSetCardParams = {0};
    CBIOS_STATUS status = CBIOS_OK;

    VIP33740RegIndex = cbGetParallelRegIndex(VIP_Parallel_Reg_Tbl, VIP_REG1, VIPPortIndex);
    VIP33744RegIndex = cbGetParallelRegIndex(VIP_Parallel_Reg_Tbl, VIP_REG2, VIPPortIndex);
    VIP33748RegIndex = cbGetParallelRegIndex(VIP_Parallel_Reg_Tbl, VIP_REG3, VIPPortIndex);
    VIP3374CRegIndex = cbGetParallelRegIndex(VIP_Parallel_Reg_Tbl, VIP_REG4, VIPPortIndex);
    VIP33750RegIndex = cbGetParallelRegIndex(VIP_Parallel_Reg_Tbl, VIP_REG5, VIPPortIndex);
    VIP33760RegIndex = cbGetParallelRegIndex(VIP_Parallel_Reg_Tbl, VIP_REG9, VIPPortIndex);
    
    //put adv7611 init here first
    cbAdv7611Init(pvcbe, VIPPortIndex);

    if(((pCbiosVIPCtlData->modeSet.fmt != CBIOS_VIP_FMT_RGB444_24BIT_SDR) &&
        (pCbiosVIPCtlData->modeSet.fmt != CBIOS_VIP_FMT_YCBCR444_24BIT_SDR) &&
        (pCbiosVIPCtlData->modeSet.fmt != CBIOS_VIP_FMT_YCBCR422_8BIT_SDR_ES) &&
        (pCbiosVIPCtlData->modeSet.fmt != CBIOS_VIP_FMT_YCBCR422_8BIT_DDR_ES)) ||
       ((pCbiosVIPCtlData->modeSet.xRes > 1920) || (pCbiosVIPCtlData->modeSet.yRes > 1080)))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "NOT support this format: %dx%d,  VIPFormat=%d\n",pCbiosVIPCtlData->modeSet.xRes, pCbiosVIPCtlData->modeSet.yRes, pCbiosVIPCtlData->modeSet.fmt));
        return CBIOS_ER_NOT_YET_IMPLEMENTED;
    }

    // VIP Card configure
    CbiosVIPSetCardParams.PortNumber = VIPPortIndex;
    CbiosVIPSetCardParams.XRes =  pCbiosVIPCtlData->modeSet.xRes;
    CbiosVIPSetCardParams.YRes =  pCbiosVIPCtlData->modeSet.yRes;
    CbiosVIPSetCardParams.RefreshRate =  pCbiosVIPCtlData->modeSet.refs;
    CbiosVIPSetCardParams.VIPFormat =  pCbiosVIPCtlData->modeSet.fmt;
#if 0
    vip_card = cbFindVipCard(pCbiosVIPCtlData->modeSet.vCard);

    if (vip_card != CBIOS_NULL)
    {
        vip_card->setMode(pvcbe, pCbiosVIPCtlData);
    }
#endif

    status = cbSetVIPCard(pvcbe, &CbiosVIPSetCardParams);
    if (CBIOS_OK != status)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "set vip card mode : %dx%d,  VIPFormat=%d failed !!!\n",pCbiosVIPCtlData->modeSet.xRes, pCbiosVIPCtlData->modeSet.yRes, pCbiosVIPCtlData->modeSet.fmt));
        return status;
    }

    // vip configure
    VIP33750RegValue.Value = 0;
    VIP33750RegValue.fifo_threshold = 10;
    VIP33750RegValue.fifo_high_threshold = 50;
    VIP33750RegValue.VIP_input_data_select = 0xFF;
    VIP33750RegValue.reserved_2 = 1;
    VIP33750RegValue.VIP_DVO_enable = 1;
    VIP33750RegMask.Value = 0;
    cbMMIOWriteReg32(pcbe, VIP33750RegIndex, VIP33750RegValue.Value, VIP33750RegMask.Value);

    VIP33744RegValue.Value = 0;
    VIP33744RegValue.REG_auto_gen = 1;
    if((pCbiosVIPCtlData->modeSet.fmt == CBIOS_VIP_FMT_RGB444_24BIT_SDR) || (pCbiosVIPCtlData->modeSet.fmt == CBIOS_VIP_FMT_YCBCR444_24BIT_SDR))
    {
        VIP33744RegValue.VIP_hde_enable = 1;
        VIP33744RegValue.VIP_sync_enable = 1;
    }
    VIP33744RegMask.Value = 0;
    cbMMIOWriteReg32(pcbe, VIP33744RegIndex, VIP33744RegValue.Value, VIP33744RegMask.Value);

    VIP33760RegValue.Value = 0;
    if((pCbiosVIPCtlData->modeSet.fmt == CBIOS_VIP_FMT_RGB444_24BIT_SDR) || (pCbiosVIPCtlData->modeSet.fmt == CBIOS_VIP_FMT_YCBCR444_24BIT_SDR))
    {
        VIP33760RegValue.FB_stride = (pCbiosVIPCtlData->modeSet.xRes * 4) >> 6;
    }
    else
    {
        VIP33760RegValue.FB_stride = (pCbiosVIPCtlData->modeSet.xRes * 2) >> 6;
    }
    VIP33760RegMask.Value = 0;
    cbMMIOWriteReg32(pcbe, VIP33760RegIndex, VIP33760RegValue.Value, VIP33760RegMask.Value);

    VIP33748RegValue.Value = 0;
    VIP33748RegValue.VIP_ver_window_length = pCbiosVIPCtlData->modeSet.yRes;
    VIP33748RegValue.crop_window_enable = 1;
    VIP33748RegMask.Value = 0;
    cbMMIOWriteReg32(pcbe, VIP33748RegIndex, VIP33748RegValue.Value, VIP33748RegMask.Value);

    VIP3374CRegValue.Value = 0;
    if(pCbiosVIPCtlData->modeSet.fmt == CBIOS_VIP_FMT_YCBCR422_8BIT_SDR_ES)
    {
        VIP3374CRegValue.VIP_hor_length = pCbiosVIPCtlData->modeSet.xRes * 2;
    }
    else
    {
        VIP3374CRegValue.VIP_hor_length = pCbiosVIPCtlData->modeSet.xRes;
    }
    VIP3374CRegMask.Value = 0;
    cbMMIOWriteReg32(pcbe, VIP3374CRegIndex, VIP3374CRegValue.Value, VIP3374CRegMask.Value);

    VIP33740RegValue.Value = 0;
    VIP33740RegMask.Value = 0XFFFFFFFF;
    if(pCbiosVIPCtlData->modeSet.fmt == CBIOS_VIP_FMT_RGB444_24BIT_SDR)
    {
        VIP33740RegValue.video_mode = VIP_FMT_RGB444_24BIT_SDR;
        VIP33740RegMask.video_mode = 0;
    }
    else if(pCbiosVIPCtlData->modeSet.fmt  == CBIOS_VIP_FMT_YCBCR444_24BIT_SDR)
    {
        VIP33740RegValue.video_mode = VIP_FMT_YCBCR444_24BIT_SDR;
        VIP33740RegMask.video_mode = 0;
    }
    else if(pCbiosVIPCtlData->modeSet.fmt == CBIOS_VIP_FMT_YCBCR422_8BIT_SDR_ES)
    {
        VIP33740RegValue.video_mode = VIP_FMT_YCBCR422_8BIT_SDR_ES;
        VIP33740RegMask.video_mode = 0;
    }
    else if(pCbiosVIPCtlData->modeSet.fmt == CBIOS_VIP_FMT_YCBCR422_8BIT_DDR_ES)
    {
        VIP33740RegValue.video_mode = VIP_FMT_YCBCR422_8BIT_DDR_ES;
        VIP33740RegValue.header_type = 1;

        VIP33740RegMask.video_mode = 0;
        VIP33740RegMask.header_type = 1;
    }
    cbMMIOWriteReg32(pcbe, VIP33740RegIndex, VIP33740RegValue.Value, VIP33740RegMask.Value);

   return status;
}


CBIOS_STATUS cbVIPSetBuffer(PCBIOS_VOID pvcbe, PCBIOS_VIP_CTRL_DATA pCbiosVIPCtlData)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U8 VIPPortIndex = pCbiosVIPCtlData->vip;
    CBIOS_U32  VIP33754RegIndex;
    CBIOS_U32  VIP33758RegIndex;
    CBIOS_U32  VIP3375CRegIndex;

    REG_MM33754_Arise VIP33754RegValue, VIP33754RegMask;
    REG_MM33758_Arise VIP33758RegValue, VIP33758RegMask;
    REG_MM3375C_Arise VIP3375CRegValue, VIP3375CRegMask;

    VIP33754RegIndex = cbGetParallelRegIndex(VIP_Parallel_Reg_Tbl, VIP_REG6, VIPPortIndex);
    VIP33758RegIndex = cbGetParallelRegIndex(VIP_Parallel_Reg_Tbl, VIP_REG7, VIPPortIndex);
    VIP3375CRegIndex = cbGetParallelRegIndex(VIP_Parallel_Reg_Tbl, VIP_REG8, VIPPortIndex);

    VIP33754RegValue.Value = 0;
    VIP33754RegValue.FB_mode = pCbiosVIPCtlData->fbSet.num -1;
    VIP33754RegMask.Value = 0XFFFFFFFF;
    VIP33754RegMask.FB_mode = 0;

    if (pCbiosVIPCtlData->fbSet.idx == 0)
    {
        VIP33754RegValue.FB_base0 = (CBIOS_U32)(pCbiosVIPCtlData->fbSet.addr >> 6);
        VIP33754RegMask.FB_base0 = 0;
        cbMMIOWriteReg32(pcbe, VIP33754RegIndex, VIP33754RegValue.Value, VIP33754RegMask.Value);
    }
    else if (pCbiosVIPCtlData->fbSet.idx == 1)
    {
        VIP33758RegValue.Value = 0;
        VIP33758RegValue.FB_base1 = (CBIOS_U32)(pCbiosVIPCtlData->fbSet.addr >> 6);
        VIP33758RegMask.Value = 0XFFFFFFFF;
        VIP33758RegMask.FB_base1 = 0;
        cbMMIOWriteReg32(pcbe, VIP33758RegIndex, VIP33758RegValue.Value, VIP33758RegMask.Value);
    }
    else if (pCbiosVIPCtlData->fbSet.idx == 2)
    {
        VIP3375CRegValue.Value = 0;
        VIP3375CRegValue.FB_base2 = (CBIOS_U32)(pCbiosVIPCtlData->fbSet.addr >> 6);
        VIP3375CRegMask.Value = 0XFFFFFFFF;
        VIP3375CRegMask.FB_base2 = 0;
        cbMMIOWriteReg32(pcbe, VIP3375CRegIndex, VIP3375CRegValue.Value, VIP3375CRegMask.Value);
    }

    if (pCbiosVIPCtlData->fbSet.idx == 2 || pCbiosVIPCtlData->fbSet.idx  == 3)
    {
        cbMMIOWriteReg32(pcbe, VIP33754RegIndex, VIP33754RegValue.Value, VIP33754RegMask.Value);
    }

    return CBIOS_OK;
}


CBIOS_STATUS cbVIPEnable(PCBIOS_VOID pvcbe, PCBIOS_VIP_CTRL_DATA pCbiosVIPCtlData)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U8 VIPPortIndex = pCbiosVIPCtlData->vip;

    CBIOS_U32  VIP33740RegIndex;
    CBIOS_U32  VIP3376CRegIndex;

    REG_MM33740_Arise VIP33740RegValue, VIP33740RegMask;
    REG_MM3376C_Arise VIP3376CRegValue, VIP3376CRegMask;

    VIP33740RegIndex = cbGetParallelRegIndex(VIP_Parallel_Reg_Tbl, VIP_REG1, VIPPortIndex);
    VIP3376CRegIndex = cbGetParallelRegIndex(VIP_Parallel_Reg_Tbl, VIP_REG12, VIPPortIndex);

    VIP3376CRegValue.Value = 0;
    VIP3376CRegValue.EOF_INT_enable = pCbiosVIPCtlData->enable;

    VIP3376CRegMask.Value = 0xFFFFFFFF;
    VIP3376CRegMask.EOF_INT_enable = 0;
    cbMMIOWriteReg32(pcbe, VIP3376CRegIndex, VIP3376CRegValue.Value, VIP3376CRegMask.Value);

    VIP33740RegValue.Value = 0;
    VIP33740RegValue.VIP_enable = pCbiosVIPCtlData->enable;
    VIP33740RegValue.SCE = 1;

    VIP33740RegMask.Value = 0XFFFFFFFF;
    VIP33740RegMask.SCE = 0;
    VIP33740RegMask.VIP_enable = 0;
    cbMMIOWriteReg32(pcbe, VIP33740RegIndex, VIP33740RegValue.Value, VIP33740RegMask.Value);

    //adv7611 disable
    if (!pCbiosVIPCtlData->enable)
    {
        cbAdv7611DeInit(pcbe, VIPPortIndex);
    }
    
    
    return CBIOS_OK;
}

CBIOS_STATUS cbVIPCtl(PCBIOS_VOID pvcbe, PCBIOS_VIP_CTRL_DATA pCbiosVIPCtlData)
{
    CBIOS_U8 VIPPortIndex = pCbiosVIPCtlData->vip;

    if(VIPPortIndex > 3)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "NOT support this VIP Port: VIPPortIndex=%d\n", VIPPortIndex));
        return CBIOS_ER_NOT_YET_IMPLEMENTED;
    }

    if (pCbiosVIPCtlData->cmd == CBIOS_VIP_QUERY_CAPS)
    {
        return cbVIPQueryCaps(pvcbe, pCbiosVIPCtlData);
    }

    if (pCbiosVIPCtlData->cmd == CBIOS_VIP_SET_MODE)
    {
        return cbVIPSetMode(pvcbe, pCbiosVIPCtlData);
    }

    if (pCbiosVIPCtlData->cmd == CBIOS_VIP_SET_BUFFER)
    {
        return cbVIPSetBuffer(pvcbe, pCbiosVIPCtlData);
    }

    if (pCbiosVIPCtlData->cmd == CBIOS_VIP_ENABLE)
    {
        return cbVIPEnable(pvcbe, pCbiosVIPCtlData);
    }

    return CBIOS_ER_NOT_YET_IMPLEMENTED;
}


CBIOS_STATUS cbVIPI2CDataRead(PCBIOS_VOID pvcbe, PCBIOS_PARAM_VIPI2C_DATA pCbiosVIPI2CParams)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U8* Buffer;
    CBIOS_U32 BufferLen;

    CBIOS_U32 i= 0, j= 0;
    CBIOS_U32 maxloop = MAXI2CWAITLOOP;
    CBIOS_U32  I2CDELAY = 200;
    CBIOS_U32 VIPnI2CDataReg, VIPnI2CCtrlReg;
    CBIOS_U16 VIPnI2CIEReg;
    CBIOS_U8 VIPPortIndex = pCbiosVIPI2CParams->PortNumber;

//cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s():PortNumber=%d, SlaveAddress=0x%x, Offset=0x%x, Buffer=0x%x, BufferLen=%d.\n",
//    FUNCTION_NAME, pCbiosVIPI2CParams->PortNumber, pCbiosVIPI2CParams->SlaveAddress, pCbiosVIPI2CParams->Offset, *pCbiosVIPI2CParams->Buffer, pCbiosVIPI2CParams->BufferLen));

    if(VIPPortIndex > 3)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "NOT support this VIP Port: VIPPortIndex=%d\n", VIPPortIndex));
        return CBIOS_ER_NOT_YET_IMPLEMENTED;
    }

    BufferLen = pCbiosVIPI2CParams->BufferLen;
    Buffer = pCbiosVIPI2CParams->Buffer;
    cb_memset(Buffer,0,BufferLen);

    VIPnI2CDataReg = cbGetParallelRegIndex(VIP_IIC_Parallel_Reg_Tbl, VIP_IIC_RW_DATA_REG, VIPPortIndex);
    VIPnI2CCtrlReg = cbGetParallelRegIndex(VIP_IIC_Parallel_Reg_Tbl, VIP_IIC_CONTROL_REG, VIPPortIndex);
    VIPnI2CIEReg = (CBIOS_U16)cbGetParallelRegIndex(VIP_IIC_Parallel_Reg_Tbl, VIP_IIC_IE, VIPPortIndex);

    cbMMIOWriteReg(pcbe,VIPnI2CIEReg, 0x0C, ~0x0C);
    cb_DelayMicroSeconds(I2CDELAY);

    cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, VIPI2C_FUNCTION_ENABLE, ~VIPI2C_FUNCTION_ENABLE);
    cb_DelayMicroSeconds(I2CDELAY);
    cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, 0x800, ~0xE00); // 200KHz
    cb_DelayMicroSeconds(I2CDELAY);

    //start
    cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, VIPI2C_START_REQUEST, ~(VIPI2C_START_REQUEST));
    cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, VIPI2C_WDAV, ~(VIPI2C_WDAV)); //set START & WDATA_AV

    j = 0;
    while(cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg) & (VIPI2C_START_REQUEST | VIPI2C_WDAV)) //query START until they are zero
    {
        if(j < maxloop)
        {
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!mmio %x=%x.\n", FUNCTION_NAME, LINE_NUM, VIPnI2CCtrlReg, cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg)));
            return CBIOS_FALSE;
        }
    }

    cbMMIOWriteReg32(pcbe, VIPnI2CDataReg, pCbiosVIPI2CParams->SlaveAddress & ~1, ~0xFF); //write the I2C address, write slave address
    cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, VIPI2C_WDAV, ~(VIPI2C_WDAV)); //set WDATA_AV

    j = 0;
    while(cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg) & VIPI2C_WDAV) //query WDATA_AV until they are zero
    {
        if(j < maxloop)
        {
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!mmio %x=%x.\n", FUNCTION_NAME, LINE_NUM, VIPnI2CCtrlReg, cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg)));
            return CBIOS_FALSE;
        }
    }

    cbMMIOWriteReg32(pcbe, VIPnI2CDataReg, pCbiosVIPI2CParams->Offset, ~0xFF); //write the I2C address, write offset
    cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, VIPI2C_WDAV, ~(VIPI2C_WDAV)); //set WDATA_AV

    j = 0;
    while(cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg) & VIPI2C_WDAV) //query WDATA_AV until they are zero
    {
        if(j < maxloop)
        {
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!mmio %x=%x.\n", FUNCTION_NAME, LINE_NUM, VIPnI2CCtrlReg, cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg)));
            return CBIOS_FALSE;
        }
    }

    //start
    cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, VIPI2C_START_REQUEST, ~(VIPI2C_START_REQUEST));
    cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, VIPI2C_WDAV, ~(VIPI2C_WDAV)); //set START & WDATA_AV

    j = 0;
    while(cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg) & (VIPI2C_START_REQUEST | VIPI2C_WDAV)) //query START until they are zero
    {
        if(j < maxloop)
        {
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!mmio %x=%x.\n", FUNCTION_NAME, LINE_NUM, VIPnI2CCtrlReg, cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg)));
            return CBIOS_FALSE;
        }
    }

    cbMMIOWriteReg32(pcbe, VIPnI2CDataReg, pCbiosVIPI2CParams->SlaveAddress | 1, ~0xFF); //write the I2C address, write slave address + 1
    cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, VIPI2C_WDAV, ~(VIPI2C_WDAV)); //set WDATA_AV

    j = 0;
    while(cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg) & VIPI2C_WDAV) //query WDATA_AV until they are zero
    {
        if(j < maxloop)
        {
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!mmio %x=%x.\n", FUNCTION_NAME, LINE_NUM, VIPnI2CCtrlReg, cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg)));
            return CBIOS_FALSE;
        }
    }

    for(i = 0;i < BufferLen;i++)
    {
        cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, VIPI2C_WDAV, ~(VIPI2C_WDAV)); //set WDATA_AV

        j = 0;
        while(cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg) & VIPI2C_WDAV) //query WDATA_AV until they are zero
        {
            if(j < maxloop)
            {
                cb_DelayMicroSeconds(I2CDELAY);
                j++;
            }
            else
            {
                cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!mmio %x=%x.\n", FUNCTION_NAME, LINE_NUM, VIPnI2CCtrlReg, cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg)));
                return CBIOS_FALSE;
            }
        }

        j = 0;
        while((cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg) & VIPI2C_READ_FINISHED) == 0) //query RDATA_AV until it is 1
        {
            if(j < maxloop)
            {
                cb_DelayMicroSeconds(I2CDELAY);
                j++;
            }
            else
            {
                cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!mmio %x=%x.\n", FUNCTION_NAME, LINE_NUM, VIPnI2CCtrlReg, cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg)));
                return CBIOS_FALSE;
            }
        }

        *(Buffer+i) = (CBIOS_UCHAR)((cb_ReadU32(pcbe->pAdapterContext,VIPnI2CDataReg) & 0x00ff0000) >> 16);//read the I2C data
        cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, ~VIPI2C_READ_FINISHED, ~VIPI2C_READ_FINISHED); //clear RDATA_AV
    }

    //stop
    cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, VIPI2C_STOP_REQUEST, ~(VIPI2C_STOP_REQUEST));
    cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, VIPI2C_WDAV, ~(VIPI2C_WDAV)); //set stop & WDATA_AV

    j = 0;
    while(cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg) & (VIPI2C_STOP_REQUEST | VIPI2C_WDAV)) //query STOP until they are zero
    {
        if(j < maxloop)
        {
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!mmio %x=%x.\n", FUNCTION_NAME, LINE_NUM, VIPnI2CCtrlReg, cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg)));
            return CBIOS_FALSE;
        }
    }

    return CBIOS_TRUE;
}

CBIOS_STATUS cbVIPI2CDataWrite(PCBIOS_VOID pvcbe, PCBIOS_PARAM_VIPI2C_DATA pCbiosVIPI2CParams)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U8* Buffer;
    CBIOS_U32 BufferLen;

    CBIOS_U32 i= 0, j= 0;
    CBIOS_U32 maxloop = MAXI2CWAITLOOP;
    CBIOS_U32  I2CDELAY = 200;
    CBIOS_U32 VIPnI2CDataReg, VIPnI2CCtrlReg;
    CBIOS_U16 VIPnI2CIEReg;
    CBIOS_U8 VIPPortIndex = pCbiosVIPI2CParams->PortNumber;

//cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s():PortNumber=%d, SlaveAddress=0x%x, Offset=0x%x, Buffer=0x%x, BufferLen=%d.\n",
//    FUNCTION_NAME, pCbiosVIPI2CParams->PortNumber, pCbiosVIPI2CParams->SlaveAddress, pCbiosVIPI2CParams->Offset, *pCbiosVIPI2CParams->Buffer, pCbiosVIPI2CParams->BufferLen));

    if(VIPPortIndex > 3)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "NOT support this VIP Port: VIPPortIndex=%d\n", VIPPortIndex));
        return CBIOS_ER_NOT_YET_IMPLEMENTED;
    }

    BufferLen = pCbiosVIPI2CParams->BufferLen;
    Buffer = pCbiosVIPI2CParams->Buffer;

    VIPnI2CDataReg = cbGetParallelRegIndex(VIP_IIC_Parallel_Reg_Tbl, VIP_IIC_RW_DATA_REG, VIPPortIndex);
    VIPnI2CCtrlReg = cbGetParallelRegIndex(VIP_IIC_Parallel_Reg_Tbl, VIP_IIC_CONTROL_REG, VIPPortIndex);
    VIPnI2CIEReg = (CBIOS_U16)cbGetParallelRegIndex(VIP_IIC_Parallel_Reg_Tbl, VIP_IIC_IE, VIPPortIndex);

    cbMMIOWriteReg(pcbe,VIPnI2CIEReg, 0x0C, ~0x0C);
    cb_DelayMicroSeconds(I2CDELAY);

    cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, VIPI2C_FUNCTION_ENABLE, ~VIPI2C_FUNCTION_ENABLE);
    cb_DelayMicroSeconds(I2CDELAY);
    cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, 0x800, ~0xE00); // 200KHz
    cb_DelayMicroSeconds(I2CDELAY);

    //start
    cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, VIPI2C_START_REQUEST, ~(VIPI2C_START_REQUEST));
    cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, VIPI2C_WDAV, ~(VIPI2C_WDAV)); //set START & WDATA_AV

    j = 0;
    while(cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg) & (VIPI2C_START_REQUEST | VIPI2C_WDAV)) //query START & WDATA_AV until they are zero
    {
        if(j < maxloop)
        {
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!mmio %x=%x.\n", FUNCTION_NAME, LINE_NUM, VIPnI2CCtrlReg, cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg)));
            return CBIOS_FALSE;
        }
    }

    cbMMIOWriteReg32(pcbe, VIPnI2CDataReg, pCbiosVIPI2CParams->SlaveAddress & ~1, ~0xFF); //write the I2C data, write slave address
    cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, VIPI2C_WDAV, ~(VIPI2C_WDAV)); //set WDATA_AV

    j = 0;
    while(cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg) & VIPI2C_WDAV) //query WDATA_AV until they are zero
    {
        if(j < maxloop)
        {
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!mmio %x=%x.\n", FUNCTION_NAME, LINE_NUM, VIPnI2CCtrlReg, cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg)));
            return CBIOS_FALSE;
        }
    }

    cbMMIOWriteReg32(pcbe, VIPnI2CDataReg, pCbiosVIPI2CParams->Offset, ~0xFF); //write the I2C data, write offset
    cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, VIPI2C_WDAV, ~(VIPI2C_WDAV)); //set WDATA_AV

    j = 0;
    while(cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg) & VIPI2C_WDAV) //query WDATA_AV until they are zero
    {
        if(j < maxloop)
        {
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!mmio %x=%x.\n", FUNCTION_NAME, LINE_NUM, VIPnI2CCtrlReg, cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg)));
            return CBIOS_FALSE;
        }
    }

    for(i = 0;i < BufferLen;i++)
    {
       //write data
        cbMMIOWriteReg32(pcbe, VIPnI2CDataReg, *(Buffer+i), ~0xFF); //write the I2C data
        cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, VIPI2C_WDAV, ~(VIPI2C_WDAV)); //set WDATA_AV

        j = 0;
        while(cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg) & VIPI2C_WDAV) //query WDATA_AV until they are zero
        {
            if(j < maxloop)
            {
                cb_DelayMicroSeconds(I2CDELAY);
                j++;
            }
            else
            {
                cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!mmio %x=%x.\n", FUNCTION_NAME, LINE_NUM, VIPnI2CCtrlReg, cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg)));
                return CBIOS_FALSE;
            }
        }
    }

    cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, VIPI2C_STOP_REQUEST, ~(VIPI2C_STOP_REQUEST));
    cbMMIOWriteReg32(pcbe, VIPnI2CCtrlReg, VIPI2C_WDAV, ~(VIPI2C_WDAV)); //set stop & WDATA_AV

    j = 0;
    while(cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg) & (VIPI2C_STOP_REQUEST | VIPI2C_WDAV)) //query stop until they are zero
    {
        if(j < maxloop)
        {
            cb_DelayMicroSeconds(I2CDELAY);
            j++;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s(%d):wait time out!mmio %x=%x.\n", FUNCTION_NAME, LINE_NUM, VIPnI2CCtrlReg, cb_ReadU32(pcbe->pAdapterContext,VIPnI2CCtrlReg)));
            return CBIOS_FALSE;
        }
    }

    cbTraceExit(GENERIC);

    return CBIOS_TRUE;
}
