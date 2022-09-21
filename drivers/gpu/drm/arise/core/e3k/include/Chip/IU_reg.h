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

#ifndef _IU_REG_H
#define _IU_REG_H


#ifndef        IU_BLOCKBASE_INF
    #define    IU_BLOCKBASE_INF
    #define    BLOCK_IU_VERSION 1
    #define    BLOCK_IU_TIMESTAMP "2018/10/25 14:55:35"
    #define    IU_BLOCK                                                   0x7
    #define    IU_REG_START                                               0x0
    #define    IU_REG_END                                                 0x40
    #define    IU_REG_LIMIT                                               0x40
#endif


#define        Reg_Iu_Ctrl_Offset                                         0x0
#define        Reg_Iu_Ctrl_Ex_Offset                                      0x1
#define        Reg_Iu_Rt_Size_Offset                                      0x2
#define        Reg_Iu_Mapping_Offset                                      0x3
#define        Reg_Iu_Intp_Mode_Offset                                    0xD
#define        Reg_Iu_Clip_Distance_Offset                                0x12
#define        Reg_Iu_Crf_Baseaddr_Offset                                 0x14
#define        Reg_Iu_Pxpy_Crf_Baseaddr_Offset                            0x1E
#define        Reg_Iu_Pxpy_Lowhigh_Offset                                 0x28
#define        Reg_Iu_Output_Format_Offset                                0x2A
#define        Reg_Iu_Viewportzvalue_Offset                               0x2C
#define        Reg_Iu_Builtin_Attr_Packing_Offset                         0x3C
#define        Reg_Iu_Reserved_Dw_Offset                                  0x3D


typedef enum
{
    IU_CTRL_INSERT_POS_DISABLED              = 0,
    IU_CTRL_INSERT_POS_ENABLED               = 1,
} IU_CTRL_INSERT_POS;
typedef enum
{
    IU_CTRL_INSERT_FACE_DISABLED             = 0,
    IU_CTRL_INSERT_FACE_ENABLED              = 1,

} IU_CTRL_INSERT_FACE;
typedef enum
{
    IU_CTRL_INSERT_ARRAY_ID_DISABLE          = 0,
    IU_CTRL_INSERT_ARRAY_ID_ENABLED          = 1,
} IU_CTRL_INSERT_ARRAY_ID;
typedef enum
{
    IU_CTRL_INSERT_RASTER_MASK_DISABLED      = 0,
    IU_CTRL_INSERT_RASTER_MASK_ENABLED       = 1,

} IU_CTRL_INSERT_RASTER_MASK;
typedef enum
{
    IU_CTRL_INSERT_SAMPLE_ID_DISABLED        = 0,
    IU_CTRL_INSERT_SAMPLE_ID_ENABLED         = 1,

} IU_CTRL_INSERT_SAMPLE_ID;
typedef enum
{
    IU_CTRL_INSERT_SAMPLE_OFFSET_DISABLED    = 0,
    IU_CTRL_INSERT_SAMPLE_OFFSET_ENABLED     = 1,

} IU_CTRL_INSERT_SAMPLE_OFFSET;
typedef enum
{
    IU_CTRL_INSERT_CONSERVATIVE_INNER_MASK_DISABLED= 0,

    IU_CTRL_INSERT_CONSERVATIVE_INNER_MASK_ENABLED= 1,

} IU_CTRL_INSERT_CONSERVATIVE_INNER_MASK;
typedef enum
{
    IU_CTRL_QUAD_2X2_MODE_EN_4X1             = 0,
    IU_CTRL_QUAD_2X2_MODE_EN_2X2             = 1,

} IU_CTRL_QUAD_2X2_MODE_EN;
typedef enum
{
    IU_CTRL_RAST_RULE_TOP_LEFT               = 0,
    IU_CTRL_RAST_RULE_BOTTOM_LEFT            = 1,
    IU_CTRL_RAST_RULE_TOP_RIGHT              = 2,
    IU_CTRL_RAST_RULE_BOTTOM_RIGHT           = 3,
} IU_CTRL_RAST_RULE;
typedef enum
{
    IU_CTRL_SIMD_MODE_SIMD32                 = 0,

    IU_CTRL_SIMD_MODE_SIMD64                 = 1,

} IU_CTRL_SIMD_MODE;
typedef enum
{
    IU_CTRL_EX_EU_BLENDING_EN_DISABLED       = 0,
    IU_CTRL_EX_EU_BLENDING_EN_ENABLED        = 1,
} IU_CTRL_EX_EU_BLENDING_EN;
typedef enum
{
    IU_CTRL_EX_Y_INVERT_EN_DISABLED          = 0,
    IU_CTRL_EX_Y_INVERT_EN_ENABLED           = 1,
} IU_CTRL_EX_Y_INVERT_EN;
typedef enum
{
    IU_CTRL_EX_PRE_ROTATE_EN_DISABLED        = 0,
    IU_CTRL_EX_PRE_ROTATE_EN_ENABLED         = 1,
} IU_CTRL_EX_PRE_ROTATE_EN;
typedef enum
{
    IU_CTRL_EX_PRE_ROTATE_TYPE_CCW_0         = 0,
    IU_CTRL_EX_PRE_ROTATE_TYPE_CCW_90        = 1,
    IU_CTRL_EX_PRE_ROTATE_TYPE_CCW_90_H      = 2,
    IU_CTRL_EX_PRE_ROTATE_TYPE_CCW_90_V      = 3,
    IU_CTRL_EX_PRE_ROTATE_TYPE_CCW_180       = 4,
    IU_CTRL_EX_PRE_ROTATE_TYPE_CCW_270       = 5,
} IU_CTRL_EX_PRE_ROTATE_TYPE;
typedef enum
{
    IU_CTRL_EX_RAST_RESOLUTION_MSAA_1X       = 0,
    IU_CTRL_EX_RAST_RESOLUTION_MSAA_2X       = 1,
    IU_CTRL_EX_RAST_RESOLUTION_MSAA_4X       = 2,
    IU_CTRL_EX_RAST_RESOLUTION_MSAA_8X       = 3,
    IU_CTRL_EX_RAST_RESOLUTION_MSAA_16X      = 4,
} IU_CTRL_EX_RAST_RESOLUTION;
typedef enum
{
    IU_CTRL_EX_API_VERSION_OGL               = 0,
    IU_CTRL_EX_API_VERSION_DX9               = 1,
    IU_CTRL_EX_API_VERSION_DX10              = 2,
    IU_CTRL_EX_API_VERSION_DX11              = 3,
    IU_CTRL_EX_API_VERSION_DX12              = 4,
} IU_CTRL_EX_API_VERSION;
typedef enum
{
    IU_INTP_MODE_ATTR0_MODE_CENTER           = 0,
    IU_INTP_MODE_ATTR0_MODE_SAMPLE           = 1,
    IU_INTP_MODE_ATTR0_MODE_CENTROID         = 2,

    IU_INTP_MODE_ATTR0_MODE_CONSTANT_NORMAL  = 3,

    IU_INTP_MODE_ATTR0_MODE_CONSTANT_BYPASS  = 4,


    IU_INTP_MODE_ATTR0_MODE_EU_INTP_PXPY     = 5,
    IU_INTP_MODE_ATTR0_MODE__EU_INTP_PXINVPY= 6,

} IU_INTP_MODE_ATTR0_MODE;
typedef enum
{
    IU_CLIP_DISTANCE_ATTR0_CLIP_DISTANCE_EN_DISABLED= 0,

    IU_CLIP_DISTANCE_ATTR0_CLIP_DISTANCE_EN_ENABLED= 1,

} IU_CLIP_DISTANCE_ATTR0_CLIP_DISTANCE_EN;
typedef enum
{
    IU_PXPY_LOWHIGH_ATTR0_LOWHIGH_LOW_RG     = 0,
    IU_PXPY_LOWHIGH_ATTR0_LOWHIGH_HIGH_BA    = 1,
} IU_PXPY_LOWHIGH_ATTR0_LOWHIGH;
typedef enum
{
    IU_OUTPUT_FORMAT_ATTR0_FMT_FP            = 0,
    IU_OUTPUT_FORMAT_ATTR0_FMT_HP            = 1,
} IU_OUTPUT_FORMAT_ATTR0_FMT;





typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Ps_Attr_Num               : 6;


        unsigned int Ab_Attr_Num               : 6;


        unsigned int Insert_Pos                : 1;
        unsigned int Insert_Face               : 1;
        unsigned int Insert_Array_Id           : 1;
        unsigned int Insert_Raster_Mask        : 1;


        unsigned int Insert_Sample_Id          : 1;
        unsigned int Insert_Sample_Offset      : 1;


        unsigned int Insert_Conservative_Inner_Mask : 1;


        unsigned int Pack_Z                    : 1;
        unsigned int Pack_W                    : 1;
        unsigned int Quad_2x2_Mode_En          : 1;
        unsigned int Rast_Rule                 : 2;
        unsigned int Rule_For_Msaa_Line        : 2;

        unsigned int Rule_For_A_Line           : 2;


        unsigned int Simd_Mode                 : 1;
        unsigned int Pack_Face                 : 1;
        unsigned int Pack_Sample_Id            : 1;
        unsigned int Post_Depth_Coverage_En    : 1;

    } reg;
} Reg_Iu_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Eu_Blending_En            : 1;
        unsigned int Y_Invert_En               : 1;
        unsigned int Pre_Rotate_En             : 1;

        unsigned int Pre_Rotate_Type           : 3;
        unsigned int Rast_Resolution           : 3;
        unsigned int Api_Version               : 3;

        unsigned int Pixel_Center_Integer_En   : 1;

        unsigned int Fragcoord_Invert_En       : 1;

        unsigned int Msaa_Center_Sample_En     : 1;
        unsigned int Mrt_En                    : 1;

        unsigned int First_Valid_Sample        : 5;






        unsigned int Reserved                  : 11;
    } reg;
} Reg_Iu_Ctrl_Ex;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Rt_Height                 : 15;
        unsigned int Rt_Width                  : 15;
        unsigned int Reserved                  : 2;
    } reg;
} Reg_Iu_Rt_Size;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Attr0_Id                  : 6;
        unsigned int Attr1_Id                  : 6;
        unsigned int Attr2_Id                  : 6;
        unsigned int Attr3_Id                  : 6;
        unsigned int Attr4_Id                  : 6;
        unsigned int Reserved                  : 2;
    } reg;
} Reg_Iu_Mapping;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Attr0_Mode                : 3;
        unsigned int Attr1_Mode                : 3;
        unsigned int Attr2_Mode                : 3;
        unsigned int Attr3_Mode                : 3;
        unsigned int Attr4_Mode                : 3;
        unsigned int Attr5_Mode                : 3;
        unsigned int Attr6_Mode                : 3;
        unsigned int Attr7_Mode                : 3;
        unsigned int Attr8_Mode                : 3;
        unsigned int Attr9_Mode                : 3;
        unsigned int Reserved                  : 2;
    } reg;
} Reg_Iu_Intp_Mode;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Attr0_Clip_Distance_En    : 1;
        unsigned int Attr1_Clip_Distance_En    : 1;
        unsigned int Attr2_Clip_Distance_En    : 1;
        unsigned int Attr3_Clip_Distance_En    : 1;
        unsigned int Attr4_Clip_Distance_En    : 1;
        unsigned int Attr5_Clip_Distance_En    : 1;
        unsigned int Attr6_Clip_Distance_En    : 1;
        unsigned int Attr7_Clip_Distance_En    : 1;
        unsigned int Attr8_Clip_Distance_En    : 1;
        unsigned int Attr9_Clip_Distance_En    : 1;
        unsigned int Attr10_Clip_Distance_En   : 1;
        unsigned int Attr11_Clip_Distance_En   : 1;
        unsigned int Attr12_Clip_Distance_En   : 1;
        unsigned int Attr13_Clip_Distance_En   : 1;
        unsigned int Attr14_Clip_Distance_En   : 1;
        unsigned int Attr15_Clip_Distance_En   : 1;
        unsigned int Attr16_Clip_Distance_En   : 1;
        unsigned int Attr17_Clip_Distance_En   : 1;
        unsigned int Attr18_Clip_Distance_En   : 1;
        unsigned int Attr19_Clip_Distance_En   : 1;
        unsigned int Attr20_Clip_Distance_En   : 1;
        unsigned int Attr21_Clip_Distance_En   : 1;
        unsigned int Attr22_Clip_Distance_En   : 1;
        unsigned int Attr23_Clip_Distance_En   : 1;
        unsigned int Attr24_Clip_Distance_En   : 1;
        unsigned int Attr25_Clip_Distance_En   : 1;
        unsigned int Attr26_Clip_Distance_En   : 1;
        unsigned int Attr27_Clip_Distance_En   : 1;
        unsigned int Attr28_Clip_Distance_En   : 1;
        unsigned int Attr29_Clip_Distance_En   : 1;
        unsigned int Attr30_Clip_Distance_En   : 1;
        unsigned int Attr31_Clip_Distance_En   : 1;
    } reg;
} Reg_Iu_Clip_Distance;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Attr0_Crf_Addr            : 6;
        unsigned int Attr1_Crf_Addr            : 6;
        unsigned int Attr2_Crf_Addr            : 6;
        unsigned int Attr3_Crf_Addr            : 6;
        unsigned int Attr4_Crf_Addr            : 6;
        unsigned int Reserved                  : 2;
    } reg;
} Reg_Iu_Crf_Baseaddr;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Attr0_Pxpy_Crf_Addr       : 6;
        unsigned int Attr1_Pxpy_Crf_Addr       : 6;
        unsigned int Attr2_Pxpy_Crf_Addr       : 6;
        unsigned int Attr3_Pxpy_Crf_Addr       : 6;
        unsigned int Attr4_Pxpy_Crf_Addr       : 6;
        unsigned int Reserved                  : 2;
    } reg;
} Reg_Iu_Pxpy_Crf_Baseaddr;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Attr0_Lowhigh             : 1;
        unsigned int Attr1_Lowhigh             : 1;
        unsigned int Attr2_Lowhigh             : 1;
        unsigned int Attr3_Lowhigh             : 1;
        unsigned int Attr4_Lowhigh             : 1;
        unsigned int Attr5_Lowhigh             : 1;
        unsigned int Attr6_Lowhigh             : 1;
        unsigned int Attr7_Lowhigh             : 1;
        unsigned int Attr8_Lowhigh             : 1;
        unsigned int Attr9_Lowhigh             : 1;
        unsigned int Attr10_Lowhigh            : 1;
        unsigned int Attr11_Lowhigh            : 1;
        unsigned int Attr12_Lowhigh            : 1;
        unsigned int Attr13_Lowhigh            : 1;
        unsigned int Attr14_Lowhigh            : 1;
        unsigned int Attr15_Lowhigh            : 1;
        unsigned int Attr16_Lowhigh            : 1;
        unsigned int Attr17_Lowhigh            : 1;
        unsigned int Attr18_Lowhigh            : 1;
        unsigned int Attr19_Lowhigh            : 1;
        unsigned int Attr20_Lowhigh            : 1;
        unsigned int Attr21_Lowhigh            : 1;
        unsigned int Attr22_Lowhigh            : 1;
        unsigned int Attr23_Lowhigh            : 1;
        unsigned int Attr24_Lowhigh            : 1;
        unsigned int Attr25_Lowhigh            : 1;
        unsigned int Attr26_Lowhigh            : 1;
        unsigned int Attr27_Lowhigh            : 1;
        unsigned int Attr28_Lowhigh            : 1;
        unsigned int Attr29_Lowhigh            : 1;
        unsigned int Attr30_Lowhigh            : 1;
        unsigned int Attr31_Lowhigh            : 1;
    } reg;
} Reg_Iu_Pxpy_Lowhigh;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Attr0_Fmt                 : 1;
        unsigned int Attr1_Fmt                 : 1;
        unsigned int Attr2_Fmt                 : 1;
        unsigned int Attr3_Fmt                 : 1;
        unsigned int Attr4_Fmt                 : 1;
        unsigned int Attr5_Fmt                 : 1;
        unsigned int Attr6_Fmt                 : 1;
        unsigned int Attr7_Fmt                 : 1;
        unsigned int Attr8_Fmt                 : 1;
        unsigned int Attr9_Fmt                 : 1;
        unsigned int Attr10_Fmt                : 1;
        unsigned int Attr11_Fmt                : 1;
        unsigned int Attr12_Fmt                : 1;
        unsigned int Attr13_Fmt                : 1;
        unsigned int Attr14_Fmt                : 1;
        unsigned int Attr15_Fmt                : 1;
        unsigned int Attr16_Fmt                : 1;
        unsigned int Attr17_Fmt                : 1;
        unsigned int Attr18_Fmt                : 1;
        unsigned int Attr19_Fmt                : 1;
        unsigned int Attr20_Fmt                : 1;
        unsigned int Attr21_Fmt                : 1;
        unsigned int Attr22_Fmt                : 1;
        unsigned int Attr23_Fmt                : 1;
        unsigned int Attr24_Fmt                : 1;
        unsigned int Attr25_Fmt                : 1;
        unsigned int Attr26_Fmt                : 1;
        unsigned int Attr27_Fmt                : 1;
        unsigned int Attr28_Fmt                : 1;
        unsigned int Attr29_Fmt                : 1;
        unsigned int Attr30_Fmt                : 1;
        unsigned int Attr31_Fmt                : 1;
    } reg;
} Reg_Iu_Output_Format;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Viewportzmin              : 32;
    } reg;
} Reg_Iu_Viewportzvalue;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Faceid_Dst_Crfaddr        : 9;


        unsigned int Sampleid_Dst_Crfaddr      : 9;


        unsigned int Reserved                  : 14;
    } reg;
} Reg_Iu_Builtin_Attr_Packing;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Iu_Reserved_Dw;

typedef struct _Iu_regs
{
    Reg_Iu_Ctrl                      reg_Iu_Ctrl;
    Reg_Iu_Ctrl_Ex                   reg_Iu_Ctrl_Ex;
    Reg_Iu_Rt_Size                   reg_Iu_Rt_Size;
    Reg_Iu_Mapping                   reg_Iu_Mapping[10];
    Reg_Iu_Intp_Mode                 reg_Iu_Intp_Mode[5];
    Reg_Iu_Clip_Distance             reg_Iu_Clip_Distance[2];
    Reg_Iu_Crf_Baseaddr              reg_Iu_Crf_Baseaddr[10];
    Reg_Iu_Pxpy_Crf_Baseaddr         reg_Iu_Pxpy_Crf_Baseaddr[10];
    Reg_Iu_Pxpy_Lowhigh              reg_Iu_Pxpy_Lowhigh[2];
    Reg_Iu_Output_Format             reg_Iu_Output_Format[2];
    Reg_Iu_Viewportzvalue            reg_Iu_Viewportzvalue[16];
    Reg_Iu_Builtin_Attr_Packing      reg_Iu_Builtin_Attr_Packing;
    Reg_Iu_Reserved_Dw               reg_Iu_Reserved_Dw[3];
} Iu_regs;

#endif
