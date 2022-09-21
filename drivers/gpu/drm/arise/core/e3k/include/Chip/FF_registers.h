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

#ifndef _FF_REGISTERS_H
#define _FF_REGISTERS_H


#ifndef        FF_BLOCKBASE_INF
    #define    FF_BLOCKBASE_INF
    #define    BLOCK_FF_VERSION 1
    #define    BLOCK_FF_TIMESTAMP "11/22/2018 3:03:21 PM"
    #define    FF_BLOCK                                                   0x6
    #define    FF_REG_START                                               0x0
    #define    FF_REG_END                                                 0xD0
    #define    FF_REG_LIMIT                                               0xD0
#endif


#define        Reg_Ff_Glb_Ctrl_Offset                                     0x0
#define        Reg_Reserved_1_Offset                                      0x1
#define        Reg_Reserved_2_Offset                                      0x2
#define        Reg_Reserved_3_Offset                                      0x3
#define        Reg_Resolution_Ctrl_Offset                                 0x4
#define        Reg_Dzs_Ctrl_Offset                                        0x5
#define        Reg_Zs_View_Ctrl_Offset                                    0x6
#define        Reg_Zs_Mipmap_Offset                                       0x7
#define        Reg_Zv_Size_Offset                                         0x8
#define        Reg_Zv_Depth_Offset                                        0x9
#define        Reg_Zv_Desc_Base0_Offset                                   0xA
#define        Reg_Sv_Desc_Base0_Offset                                   0xB
#define        Reg_Sv_Bl_Slot_Idx_Offset                                  0xC
#define        Reg_Ffc_Config_Offset                                      0xD
#define        Reg_Ffc_Global_Range_Config_Offset                         0xE
#define        Reg_Reserved_4_Offset                                      0xF
#define        Reg_Ffc_Dsig_Base_Addr_Offset                              0x10
#define        Reg_Rt_Addr_Ctrl_Offset                                    0x18
#define        Reg_Rt_Ctrl_Offset                                         0x30
#define        Reg_Rast_Ctrl_Offset                                       0x48
#define        Reg_Sg_Ctrl_Offset                                         0x49
#define        Reg_Zs_Req_Ctrl_Offset                                     0x4A
#define        Reg_Zs_Ctl_Offset                                          0x4B
#define        Reg_Ff_Stencil_Group_Offset                                0x4C
#define        Reg_Depth_Bound_Start_Offset                               0x4E
#define        Reg_Depth_Bound_End_Offset                                 0x4F
#define        Reg_Viewport_Zmin_Offset                                   0x50
#define        Reg_Viewport_Zmax_Offset                                   0x60
#define        Reg_User_Msaa_Pattern_Group_Offset                         0x70
#define        Reg_Multi_Res_Blk_Group_Offset                             0x74
#define        Reg_Vr_Zrange_Group_Offset                                 0xAA
#define        Reg_Blend_Ctl_Offset                                       0xB0
#define        Reg_Fast_Clear_Color_Offset                                0xC8
#define        Reg_Tasbe_Ctrl_Offset                                      0xCC
#define        Reg_Reserved_5_Offset                                      0xCD
#define        Reg_Reserved_6_Offset                                      0xCE
#define        Reg_Reserved_7_Offset                                      0xCF


typedef enum
{
    FF_GLB_CTRL_TBR_EN_DISABLE               = 0,
    FF_GLB_CTRL_TBR_EN_ENABLE                = 1,
} FF_GLB_CTRL_TBR_EN;
typedef enum
{
    FF_GLB_CTRL_TBR_MODE_PER_DRAW            = 0,
    FF_GLB_CTRL_TBR_MODE_PER_FRAME           = 1,
} FF_GLB_CTRL_TBR_MODE;
typedef enum
{
    FF_GLB_CTRL_TILE_SIZE_256X128            = 0,
    FF_GLB_CTRL_TILE_SIZE_128X128            = 1,
    FF_GLB_CTRL_TILE_SIZE_128X64             = 2,
    FF_GLB_CTRL_TILE_SIZE_64X64              = 3,
    FF_GLB_CTRL_TILE_SIZE_64X32              = 4,
    FF_GLB_CTRL_TILE_SIZE_32X32              = 5,
    FF_GLB_CTRL_TILE_SIZE_32X16              = 6,
    FF_GLB_CTRL_TILE_SIZE_16X16              = 7,
    FF_GLB_CTRL_TILE_SIZE_16X8               = 8,
    FF_GLB_CTRL_TILE_SIZE_8X8                = 9,
} FF_GLB_CTRL_TILE_SIZE;
typedef enum
{
    FF_GLB_CTRL_FFC_CONFIG_MODE_ZS_D_U       = 0,



    FF_GLB_CTRL_FFC_CONFIG_MODE_D_ONLY       = 1,
    FF_GLB_CTRL_FFC_CONFIG_MODE_ZS_ONLY      = 2,
    FF_GLB_CTRL_FFC_CONFIG_MODE_UAV_ONLY     = 3,
    FF_GLB_CTRL_FFC_CONFIG_MODE_ZS_D         = 4,
    FF_GLB_CTRL_FFC_CONFIG_MODE_ZS_U         = 5,



    FF_GLB_CTRL_FFC_CONFIG_MODE_D_U          = 6,


    FF_GLB_CTRL_FFC_CONFIG_MODE_SW_CONFIG    = 7,


} FF_GLB_CTRL_FFC_CONFIG_MODE;
typedef enum
{
    RESOLUTION_CTRL_DST_RESOLUTION_MODE_1X   = 0,
    RESOLUTION_CTRL_DST_RESOLUTION_MODE_2X   = 1,
    RESOLUTION_CTRL_DST_RESOLUTION_MODE_4X   = 2,
    RESOLUTION_CTRL_DST_RESOLUTION_MODE_8X   = 3,
    RESOLUTION_CTRL_DST_RESOLUTION_MODE_16X  = 4,
} RESOLUTION_CTRL_DST_RESOLUTION_MODE;
typedef enum
{
    RESOLUTION_CTRL_MSAA_MODE_MSAA_1X        = 0,
    RESOLUTION_CTRL_MSAA_MODE_MSAA_2X        = 1,
    RESOLUTION_CTRL_MSAA_MODE_MSAA_4X        = 2,
    RESOLUTION_CTRL_MSAA_MODE_MSAA_8X        = 3,
    RESOLUTION_CTRL_MSAA_MODE_MSAA_16X       = 4,
} RESOLUTION_CTRL_MSAA_MODE;
typedef enum
{
    RESOLUTION_CTRL_MULTI_RES_EN_DISABLED    = 0,
    RESOLUTION_CTRL_MULTI_RES_EN_ENABLED     = 1,
} RESOLUTION_CTRL_MULTI_RES_EN;
typedef enum
{
    RESOLUTION_CTRL_PS_SAMPLE_FREQ_EN_DISABLED= 0,
    RESOLUTION_CTRL_PS_SAMPLE_FREQ_EN_ENABLED= 1,
} RESOLUTION_CTRL_PS_SAMPLE_FREQ_EN;
typedef enum
{
    DZS_CTRL_SRC_BITS_MODE_NORMAL            = 0,

    DZS_CTRL_SRC_BITS_MODE_IS_1BPP           = 1,


    DZS_CTRL_SRC_BITS_MODE_IS_4BPP           = 2,



    DZS_CTRL_SRC_BITS_MODE_UNUSED            = 3,
} DZS_CTRL_SRC_BITS_MODE;
typedef enum
{
    DZS_CTRL_THREAD_MODE_SIMD32              = 0,
    DZS_CTRL_THREAD_MODE_SIMD64              = 1,
} DZS_CTRL_THREAD_MODE;
typedef enum
{
    ZS_MIPMAP_RESOURCE_TYPE_1D_BUFFER        = 0,
    ZS_MIPMAP_RESOURCE_TYPE_1D_TEXTURE       = 1,
    ZS_MIPMAP_RESOURCE_TYPE_2D_TEXTURE       = 2,
    ZS_MIPMAP_RESOURCE_TYPE_3D_TEXTURE       = 3,
    ZS_MIPMAP_RESOURCE_TYPE_CUBIC_TEXTURE    = 4,
    ZS_MIPMAP_RESOURCE_TYPE_1D_TEXTURE_ARRAY= 5,
    ZS_MIPMAP_RESOURCE_TYPE_2D_TEXTURE_ARRAY= 6,
    ZS_MIPMAP_RESOURCE_TYPE_CUBE_TEXTURE_ARRAY= 7,
    ZS_MIPMAP_RESOURCE_TYPE_RAW_BUFFER       = 8,
    ZS_MIPMAP_RESOURCE_TYPE_STRUCTURED_BUFFER= 9,
} ZS_MIPMAP_RESOURCE_TYPE;
typedef enum
{
    ZV_DEPTH_FORMAT_Z16                      = 0,
    ZV_DEPTH_FORMAT_Z24                      = 1,
    ZV_DEPTH_FORMAT_Z32F                     = 2,
} ZV_DEPTH_FORMAT;
typedef enum
{
    ZV_DEPTH_L1CACHABLE_DISABLE              = 0,
    ZV_DEPTH_L1CACHABLE_ENABLE               = 1,
} ZV_DEPTH_L1CACHABLE;
typedef enum
{
    ZV_DEPTH_L2CACHABLE_DISABLE              = 0,
    ZV_DEPTH_L2CACHABLE_ENABLE               = 1,
} ZV_DEPTH_L2CACHABLE;
typedef enum
{
    FFC_CONFIG_FFC_ACK_DELAY_DISABLE         = 0,
    FFC_CONFIG_FFC_ACK_DELAY_HALF            = 1,
    FFC_CONFIG_FFC_ACK_DELAY_QUARTER         = 2,
} FFC_CONFIG_FFC_ACK_DELAY;
typedef enum
{
    RT_DEPTH_CRF_MODE_FP                     = 0,
    RT_DEPTH_CRF_MODE_HP                     = 1,
} RT_DEPTH_CRF_MODE;
typedef enum
{
    RT_MISC_DITHER_EN_DISABLE                = 0,
    RT_MISC_DITHER_EN_ENABLE                 = 1,
} RT_MISC_DITHER_EN;
typedef enum
{
    RT_MISC_EU_BLEND_ENABLE_DISABLE          = 0,
    RT_MISC_EU_BLEND_ENABLE_ENABLE           = 1,
} RT_MISC_EU_BLEND_ENABLE;
typedef enum
{
    RT_MISC_EU_EMIT_COLOR_DISABLE            = 0,
    RT_MISC_EU_EMIT_COLOR_ENABLE             = 1,
} RT_MISC_EU_EMIT_COLOR;
typedef enum
{
    RT_MISC_EUB_FORMAT_UNORM8                = 0,
    RT_MISC_EUB_FORMAT_SNORM8                = 1,
    RT_MISC_EUB_FORMAT_UINT8                 = 2,
    RT_MISC_EUB_FORMAT_SINT8                 = 3,
    RT_MISC_EUB_FORMAT_UINT8_SCALED          = 4,
    RT_MISC_EUB_FORMAT_SINT8_SCALED          = 5,
    RT_MISC_EUB_FORMAT_UNORM16               = 6,
    RT_MISC_EUB_FORMAT_SNORM16               = 7,
    RT_MISC_EUB_FORMAT_FP16                  = 8,
    RT_MISC_EUB_FORMAT_UINT16                = 9,
    RT_MISC_EUB_FORMAT_SINT16                = 10,
    RT_MISC_EUB_FORMAT_UINT16_SCALED         = 11,
    RT_MISC_EUB_FORMAT_SINT16_SCALED         = 12,
    RT_MISC_EUB_FORMAT_FP32                  = 13,
    RT_MISC_EUB_FORMAT_FIXED                 = 14,
    RT_MISC_EUB_FORMAT_UNORM10               = 15,
    RT_MISC_EUB_FORMAT_UNORM12               = 16,
    RT_MISC_EUB_FORMAT_FP24                  = 17,
    RT_MISC_EUB_FORMAT_UNORM8_8              = 18,
    RT_MISC_EUB_FORMAT_SNORM8_8              = 19,
    RT_MISC_EUB_FORMAT_SNORM10               = 20,
    RT_MISC_EUB_FORMAT_UNORM24               = 21,
    RT_MISC_EUB_FORMAT_UNORM10_6             = 22,
    RT_MISC_EUB_FORMAT_SNORM10_6             = 23,
    RT_MISC_EUB_FORMAT_UINT32                = 24,
    RT_MISC_EUB_FORMAT_UINT64                = 25,
    RT_MISC_EUB_FORMAT_SINT32                = 26,
} RT_MISC_EUB_FORMAT;
typedef enum
{
    RT_MISC_SWIZZLE_3D_EN_DISABLE            = 0,
    RT_MISC_SWIZZLE_3D_EN_ENABLE             = 1,
} RT_MISC_SWIZZLE_3D_EN;
typedef enum
{
    RT_MISC_MIPMAP_EN_DISABLE                = 0,
    RT_MISC_MIPMAP_EN_ENABLE                 = 1,
} RT_MISC_MIPMAP_EN;
typedef enum
{
    RT_MISC_RESOURCE_TYPE_1D_BUFFER          = 0,
    RT_MISC_RESOURCE_TYPE_1D_TEXTURE         = 1,
    RT_MISC_RESOURCE_TYPE_2D_TEXTURE         = 2,
    RT_MISC_RESOURCE_TYPE_3D_TEXTURE         = 3,
    RT_MISC_RESOURCE_TYPE_CUBIC_TEXTURE      = 4,
    RT_MISC_RESOURCE_TYPE_1D_TEXTURE_ARRAY   = 5,
    RT_MISC_RESOURCE_TYPE_2D_TEXTURE_ARRAY   = 6,
    RT_MISC_RESOURCE_TYPE_CUBE_TEXTURE_ARRAY= 7,
    RT_MISC_RESOURCE_TYPE_RAW_BUFFER         = 8,
    RT_MISC_RESOURCE_TYPE_STRUCTURED_BUFFER  = 9,
} RT_MISC_RESOURCE_TYPE;
typedef enum
{
    RT_MISC_L1CACHABLE_DISABLE               = 0,
    RT_MISC_L1CACHABLE_ENABLE                = 1,
} RT_MISC_L1CACHABLE;
typedef enum
{
    RT_MISC_L2CACHABLE_DISABLE               = 0,
    RT_MISC_L2CACHABLE_ENABLE                = 1,
} RT_MISC_L2CACHABLE;
typedef enum
{
    RAST_CTRL_RASTER_MODE_NORMAL             = 0,
    RAST_CTRL_RASTER_MODE_CONSERVATIVE       = 1,
} RAST_CTRL_RASTER_MODE;
typedef enum
{
    RAST_CTRL_CHECK_BOARD_256X32             = 0,
    RAST_CTRL_CHECK_BOARD_128X32             = 1,
    RAST_CTRL_CHECK_BOARD_64X32              = 2,
    RAST_CTRL_CHECK_BOARD_32X32              = 3,
    RAST_CTRL_CHECK_BOARD_XX32               = 4,
    RAST_CTRL_CHECK_BOARD_256X64             = 5,
    RAST_CTRL_CHECK_BOARD_128X64             = 6,
    RAST_CTRL_CHECK_BOARD_64X64              = 7,
    RAST_CTRL_CHECK_BOARD_32X64              = 8,
    RAST_CTRL_CHECK_BOARD_16X64              = 9,
    RAST_CTRL_CHECK_BOARD_256X16             = 10,
    RAST_CTRL_CHECK_BOARD_128X16             = 11,
    RAST_CTRL_CHECK_BOARD_64X16              = 12,
    RAST_CTRL_CHECK_BOARD_32X16              = 13,
    RAST_CTRL_CHECK_BOARD_16X16              = 14,
} RAST_CTRL_CHECK_BOARD;
typedef enum
{
    RAST_CTRL_API_VERSION_OGL                = 0,
    RAST_CTRL_API_VERSION_DX9                = 1,
    RAST_CTRL_API_VERSION_DX10               = 2,
    RAST_CTRL_API_VERSION_DX11               = 3,
    RAST_CTRL_API_VERSION_DX12               = 4,
} RAST_CTRL_API_VERSION;
typedef enum
{
    RAST_CTRL_ROV_EN_DISABLED                = 0,
    RAST_CTRL_ROV_EN_ENABLED                 = 1,
} RAST_CTRL_ROV_EN;
typedef enum
{
    RAST_CTRL_EUB_EN_DISABLED                = 0,
    RAST_CTRL_EUB_EN_ENABLED                 = 1,
} RAST_CTRL_EUB_EN;
typedef enum
{
    RAST_CTRL_MSAA_EN_MSAA_OFF               = 0,
    RAST_CTRL_MSAA_EN_MSAA_DEFALUT           = 1,
    RAST_CTRL_MSAA_EN_MSAA_USER_DEFINE       = 2,
    RAST_CTRL_MSAA_EN_MSAA_CENTER_SAMPLE     = 3,
} RAST_CTRL_MSAA_EN;
typedef enum
{
    RAST_CTRL_AA_EN_NORMAL                   = 0,
    RAST_CTRL_AA_EN_AA                       = 1,
} RAST_CTRL_AA_EN;
typedef enum
{
    SG_CTRL_RULE_FOR_TRIANGLE_TOP_LEFT       = 0,
    SG_CTRL_RULE_FOR_TRIANGLE_TOP_RIGHT      = 1,
    SG_CTRL_RULE_FOR_TRIANGLE_BOTTOM_LEFT    = 2,
    SG_CTRL_RULE_FOR_TRIANGLE_BOTTON_RIGHT   = 3,
} SG_CTRL_RULE_FOR_TRIANGLE;
typedef enum
{
    SG_CTRL_STIPPLE_EN_DISABLE               = 0,
    SG_CTRL_STIPPLE_EN_ENABLE                = 1,
} SG_CTRL_STIPPLE_EN;
typedef enum
{
    SG_CTRL_COMPRESS_EN_DISABLE              = 0,
    SG_CTRL_COMPRESS_EN_ENABLE               = 1,
} SG_CTRL_COMPRESS_EN;
typedef enum
{
    SG_CTRL_EVALUATE_MODE_EN_DISABLE         = 0,
    SG_CTRL_EVALUATE_MODE_EN_ENABLE          = 1,
} SG_CTRL_EVALUATE_MODE_EN;
typedef enum
{
    SG_CTRL_DISABLE_GPC_SPLIT_FALSE          = 0,
    SG_CTRL_DISABLE_GPC_SPLIT_TRUE           = 1,
} SG_CTRL_DISABLE_GPC_SPLIT;
typedef enum
{
    SG_CTRL_DISABLE_SLICE_SPLIT_FALSE        = 0,
    SG_CTRL_DISABLE_SLICE_SPLIT_TRUE         = 1,
} SG_CTRL_DISABLE_SLICE_SPLIT;
typedef enum
{
    ZS_REQ_CTRL_PS_OUTPUT_OMASK_DISABLE      = 0,
    ZS_REQ_CTRL_PS_OUTPUT_OMASK_ENABLE       = 1,
} ZS_REQ_CTRL_PS_OUTPUT_OMASK;
typedef enum
{
    ZS_REQ_CTRL_D_ALLOCATION_MODULE_ZL2      = 0,
    ZS_REQ_CTRL_D_ALLOCATION_MODULE_ZL3      = 1,
} ZS_REQ_CTRL_D_ALLOCATION_MODULE;
typedef enum
{
    ZS_REQ_CTRL_A2C_EN_DISABLE               = 0,
    ZS_REQ_CTRL_A2C_EN_ENABLE                = 1,
} ZS_REQ_CTRL_A2C_EN;
typedef enum
{
    ZS_REQ_CTRL_D_ALLOC_EN_DISABLE           = 0,
    ZS_REQ_CTRL_D_ALLOC_EN_ENABLE            = 1,
} ZS_REQ_CTRL_D_ALLOC_EN;
typedef enum
{
    ZS_REQ_CTRL_OROMASK_DISABLE              = 0,
    ZS_REQ_CTRL_OROMASK_ENABLE               = 1,
} ZS_REQ_CTRL_OROMASK;
typedef enum
{
    ZS_CTL_Z_CMP_MODE_NEVER                  = 0,
    ZS_CTL_Z_CMP_MODE_LESS                   = 1,
    ZS_CTL_Z_CMP_MODE_EQUAL                  = 2,
    ZS_CTL_Z_CMP_MODE_LESSEQUAL              = 3,
    ZS_CTL_Z_CMP_MODE_GREATER                = 4,
    ZS_CTL_Z_CMP_MODE_NOTEQUAL               = 5,
    ZS_CTL_Z_CMP_MODE_GREATEREQUAL           = 6,
    ZS_CTL_Z_CMP_MODE_ALWAYS                 = 7,
} ZS_CTL_Z_CMP_MODE;
typedef enum
{
    ZS_CTL_Z_16_OPTIMIZED_EN_DISABLE         = 0,
    ZS_CTL_Z_16_OPTIMIZED_EN_ENABLE          = 1,
} ZS_CTL_Z_16_OPTIMIZED_EN;
typedef enum
{
    STENCIL_STATE_ST_OP_SFAIL_KEEP           = 0,
    STENCIL_STATE_ST_OP_SFAIL_ZERO           = 1,
    STENCIL_STATE_ST_OP_SFAIL_REPLACE        = 2,
    STENCIL_STATE_ST_OP_SFAIL_INCRSAT        = 3,
    STENCIL_STATE_ST_OP_SFAIL_DECRSAT        = 4,
    STENCIL_STATE_ST_OP_SFAIL_INVERT         = 5,
    STENCIL_STATE_ST_OP_SFAIL_INCRSAT1       = 6,
    STENCIL_STATE_ST_OP_SFAIL_DECRSAT1       = 7,
} STENCIL_STATE_ST_OP_SFAIL;
typedef enum
{
    RES_BLK_REF_RESOLUTION_PS_REF_LEVEL_RES16X= 0,
    RES_BLK_REF_RESOLUTION_PS_REF_LEVEL_RES8X= 1,
    RES_BLK_REF_RESOLUTION_PS_REF_LEVEL_RES4X= 2,
    RES_BLK_REF_RESOLUTION_PS_REF_LEVEL_RES2X= 3,
    RES_BLK_REF_RESOLUTION_PS_REF_LEVEL_RES1X= 4,
    RES_BLK_REF_RESOLUTION_PS_REF_LEVEL_RES1_2X= 5,
    RES_BLK_REF_RESOLUTION_PS_REF_LEVEL_RES1_4X= 6,
    RES_BLK_REF_RESOLUTION_PS_REF_LEVEL_RES1_8X= 7,
    RES_BLK_REF_RESOLUTION_PS_REF_LEVEL_RES1_16X= 8,
} RES_BLK_REF_RESOLUTION_PS_REF_LEVEL;
typedef enum
{
    BS_BLEND_CTL_RGB_SBLEND_ZERO             = 0,
    BS_BLEND_CTL_RGB_SBLEND_ONE              = 1,
    BS_BLEND_CTL_RGB_SBLEND_SRCCOLOR         = 2,
    BS_BLEND_CTL_RGB_SBLEND_INVSRCCOLOR      = 3,

    BS_BLEND_CTL_RGB_SBLEND_SRCALPHA         = 4,
    BS_BLEND_CTL_RGB_SBLEND_INVSRCALPHA      = 5,

    BS_BLEND_CTL_RGB_SBLEND_DESTALPHA        = 6,
    BS_BLEND_CTL_RGB_SBLEND_INVDESTALPHA     = 7,

    BS_BLEND_CTL_RGB_SBLEND_DESTCOLOR        = 8,
    BS_BLEND_CTL_RGB_SBLEND_INVDESTCOLOR     = 9,

    BS_BLEND_CTL_RGB_SBLEND_SRCALPHASAT      = 10,

    BS_BLEND_CTL_RGB_SBLEND_CONSTANTCOLOR    = 11,

    BS_BLEND_CTL_RGB_SBLEND_INVCONSTANTCOLOR= 12,

    BS_BLEND_CTL_RGB_SBLEND_CONSTANTALPHA    = 13,

    BS_BLEND_CTL_RGB_SBLEND_INVCONSTANTALPHA= 14,

    BS_BLEND_CTL_RGB_SBLEND_SRCCONSTANTALPHA= 15,

    BS_BLEND_CTL_RGB_SBLEND_SRC1_COLOR       = 16,
    BS_BLEND_CTL_RGB_SBLEND_INVSRC1_COLOR    = 17,
    BS_BLEND_CTL_RGB_SBLEND_SRC1_ALPHA       = 18,
    BS_BLEND_CTL_RGB_SBLEND_INVSRC1_ALPHA    = 19,
} BS_BLEND_CTL_RGB_SBLEND;
typedef enum
{
    BS_BLEND_CTL_RGB_BLEND_OP_ADD            = 0,
    BS_BLEND_CTL_RGB_BLEND_OP_SUBTRACT       = 1,
    BS_BLEND_CTL_RGB_BLEND_OP_REVSUBTRACT    = 2,
    BS_BLEND_CTL_RGB_BLEND_OP_MAX_OP         = 3,
    BS_BLEND_CTL_RGB_BLEND_OP_MIN_OP         = 4,
} BS_BLEND_CTL_RGB_BLEND_OP;
typedef enum
{
    TASBE_CTRL_MSAA_STATE_MSAA_1X            = 0,
    TASBE_CTRL_MSAA_STATE_MSAA_2X            = 1,
    TASBE_CTRL_MSAA_STATE_MSAA_4X            = 2,
    TASBE_CTRL_MSAA_STATE_MSAA_8X            = 3,
    TASBE_CTRL_MSAA_STATE_MSAA_16X           = 4,
} TASBE_CTRL_MSAA_STATE;
typedef enum
{
    TASBE_CTRL_AA_EN_FALSE                   = 0,
    TASBE_CTRL_AA_EN_TRUE                    = 1,
} TASBE_CTRL_AA_EN;
typedef enum
{
    TASBE_CTRL_RASTER_MODE_NORMAL            = 0,
    TASBE_CTRL_RASTER_MODE_CONSERVATIVE      = 1,
} TASBE_CTRL_RASTER_MODE;





typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Tbr_En                    : 1;
        unsigned int Tbr_Mode                  : 1;
        unsigned int Tile_Size                 : 4;
        unsigned int Ffc_Config_Mode           : 3;
        unsigned int Ffc_U_Start               : 5;

        unsigned int Reserved                  : 18;
    } reg;
} Reg_Ff_Glb_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Reserved_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Reserved_2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Reserved_3;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Dst_Resolution_Mode       : 3;
        unsigned int Msaa_Mode                 : 3;
        unsigned int Multi_Res_En              : 1;



        unsigned int Ps_Sample_Freq_En         : 1;
        unsigned int Reserved                  : 24;
    } reg;
} Reg_Resolution_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Src_Read_Alloc_En         : 1;
        unsigned int Fc_St_Wr_Mask             : 8;
        unsigned int L2l_Blit_Optimization_En : 1;

        unsigned int Src_Bits_Mode             : 2;
        unsigned int Color_Key_En              : 1;

        unsigned int S_Read_En                 : 1;
        unsigned int Thread_Mode               : 1;
        unsigned int Reserved                  : 17;
    } reg;
} Reg_Dzs_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Arraysize                 : 12;
        unsigned int First_Array_Idx           : 11;
        unsigned int Reserved                  : 9;
    } reg;
} Reg_Zs_View_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Zs_Mip_En                 : 1;
        unsigned int Zs_Lod                    : 4;
        unsigned int Z_Bl_Slot_Idx             : 18;
        unsigned int Resource_Type             : 4;
        unsigned int Reserved                  : 5;
    } reg;
} Reg_Zs_Mipmap;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Width                     : 15;
        unsigned int Height                    : 15;
        unsigned int Reserved                  : 2;
    } reg;
} Reg_Zv_Size;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Depth_Or_Arraysize        : 12;
        unsigned int Z_Range_Type              : 6;
        unsigned int Format                    : 2;
        unsigned int L1cachable                : 1;
        unsigned int L2cachable                : 1;
        unsigned int S_Range_Type              : 6;
        unsigned int Z_Discard_En              : 1;


        unsigned int S_Discard_En              : 1;


        unsigned int Z_Msaa_Opt_En             : 1;


        unsigned int S_Msaa_Opt_En             : 1;


    } reg;
} Reg_Zv_Depth;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Addr                 : 32;

    } reg;
} Reg_Zv_Desc_Base0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Addr                 : 32;

    } reg;
} Reg_Sv_Desc_Base0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Bl_Slot_Idx               : 18;
        unsigned int Reserved                  : 14;
    } reg;
} Reg_Sv_Bl_Slot_Idx;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Tbr_Dsig_En               : 1;



        unsigned int Ffc_Ack_Delay             : 2;
        unsigned int En_4kb_Mem_Swizzle        : 1;
        unsigned int En_Decompressed_Data_Reorder : 1;

        unsigned int Slot_Swz_Enable           : 1;

        unsigned int Reserved                  : 26;
    } reg;
} Reg_Ffc_Config;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Global_Offset             : 8;
        unsigned int Global_Size               : 8;

        unsigned int Reserved                  : 16;
    } reg;
} Reg_Ffc_Global_Range_Config;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Reserved_4;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Addr                      : 32;
    } reg;
} Reg_Addr;

typedef struct _Group_Ffc_Dsig_Base_Addr
{
    Reg_Addr                         reg_Addr;
} Reg_Ffc_Dsig_Base_Addr_Group;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Addr                 : 32;
    } reg;
} Reg_Rt_Addr;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Depth_Or_Arraysize        : 12;
        unsigned int Lod                       : 4;
        unsigned int Range_Type                : 6;
        unsigned int Discard_En                : 1;


        unsigned int Crf_Addr                  : 6;
        unsigned int Crf_Mode                  : 1;
        unsigned int Msaa_Opt_En               : 1;


        unsigned int Reserved                  : 1;
    } reg;
} Reg_Rt_Depth;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Arraysize                 : 12;
        unsigned int First_Array_Idx           : 11;
        unsigned int Reserved                  : 9;
    } reg;
} Reg_Rt_View_Ctrl;

typedef struct _Group_Rt_Addr_Ctrl
{
    Reg_Rt_Addr                      reg_Rt_Addr;
    Reg_Rt_Depth                     reg_Rt_Depth;
    Reg_Rt_View_Ctrl                 reg_Rt_View_Ctrl;
} Reg_Rt_Addr_Ctrl_Group;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Format                    : 9;
        unsigned int Bl_Slot_Idx               : 18;
        unsigned int Reserved                  : 5;
    } reg;
} Reg_Rt_Fmt;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Width                     : 15;
        unsigned int Height                    : 15;
        unsigned int Reserved                  : 2;
    } reg;
} Reg_Rt_Size;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Rt_Enable                 : 1;
        unsigned int Is_Tiling                 : 1;
        unsigned int Rt_Write_Mask             : 4;
        unsigned int Dither_En                 : 1;
        unsigned int D_Read_En                 : 1;



        unsigned int Resolve_Write_En          : 1;
        unsigned int Blend_Enable              : 1;
        unsigned int Eu_Blend_Enable           : 1;
        unsigned int Eu_Emit_Color             : 1;

        unsigned int Eub_Format                : 5;
        unsigned int Dual_Source_En            : 1;
        unsigned int Swizzle_3d_En             : 1;
        unsigned int Mipmap_En                 : 1;
        unsigned int Resource_Type             : 4;
        unsigned int Rop                       : 5;



















        unsigned int L1cachable                : 1;
        unsigned int L2cachable                : 1;
        unsigned int Reserved                  : 1;
    } reg;
} Reg_Rt_Misc;

typedef struct _Group_Rt_Ctrl
{
    Reg_Rt_Fmt                       reg_Rt_Fmt;
    Reg_Rt_Size                      reg_Rt_Size;
    Reg_Rt_Misc                      reg_Rt_Misc;
} Reg_Rt_Ctrl_Group;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Raster_Mode               : 1;
        unsigned int Check_Board               : 4;

        unsigned int Api_Version               : 3;


        unsigned int Rov_En                    : 1;
        unsigned int Eub_En                    : 1;
        unsigned int Msaa_En                   : 2;
        unsigned int Aa_En                     : 1;
        unsigned int Msaa_Mask                 : 16;
        unsigned int Reserved                  : 3;
    } reg;
} Reg_Rast_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Rule_For_Triangle         : 2;
        unsigned int Rule_For_A_Line           : 2;
        unsigned int Rule_For_Msaa_Line        : 2;
        unsigned int Stipple_En                : 1;
        unsigned int Compress_En               : 1;
        unsigned int Evaluate_Mode_En          : 1;
        unsigned int Disable_Gpc_Split         : 1;
        unsigned int Disable_Slice_Split       : 1;
        unsigned int Reserved                  : 21;
    } reg;
} Reg_Sg_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Z_Read_En                 : 1;
        unsigned int Zl1_Zrange_Update_En      : 1;
        unsigned int Z_Alloc_En                : 1;
        unsigned int S_Alloc_En                : 1;
        unsigned int Zl2_End_Pipe_En           : 1;
        unsigned int Zl3_End_Pipe_En           : 1;
        unsigned int Zl1_Depth_Bound_Test_En   : 1;
        unsigned int Zl2_Depth_Bound_Test_En   : 1;
        unsigned int Zl3_Depth_Bound_Test_En   : 1;
        unsigned int Zl1_Range_Test_En         : 1;
        unsigned int Zl2_Z_Test_En             : 1;
        unsigned int Zl3_Z_Test_En             : 1;
        unsigned int Zl2_S_Test_En             : 1;
        unsigned int Zl3_S_Test_En             : 1;
        unsigned int Zl2_Z_Update_En           : 1;
        unsigned int Zl3_Z_Update_En           : 1;
        unsigned int Zl2_S_Update_En           : 1;
        unsigned int Zl3_S_Update_En           : 1;
        unsigned int Zl1_Clip_En               : 1;
        unsigned int Zl2_Clip_En               : 1;
        unsigned int Zl3_Clip_En               : 1;
        unsigned int Ps_Output_Stencil_Ref     : 1;
        unsigned int Ps_Output_Alpha_En        : 1;
        unsigned int Ps_Output_Depth           : 1;
        unsigned int Ps_Output_Omask           : 1;
        unsigned int Eu_Kill_Pixel             : 1;
        unsigned int D_Allocation_Module       : 1;
        unsigned int A2c_En                    : 1;
        unsigned int D_Alloc_En                : 1;
        unsigned int Oromask                   : 1;
        unsigned int Z_Resolved_Read_En        : 1;



        unsigned int S_Resolved_Read_En        : 1;


    } reg;
} Reg_Zs_Req_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Z_Cmp_Mode                : 3;
        unsigned int Z_16_Optimized_En         : 1;





        unsigned int Z_Occlusion_Counting_Enable : 1;
        unsigned int St_Ref_Front              : 8;
        unsigned int St_Ref_Back               : 8;
        unsigned int Z_Is_Read_Only            : 1;
        unsigned int S_Is_Read_Only            : 1;
        unsigned int Zl1_Pass_Optimized        : 1;
        unsigned int Is_Alpha_Reorder          : 1;

        unsigned int Reserved                  : 7;
    } reg;
} Reg_Zs_Ctl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int St_Cmp_Mode               : 3;
        unsigned int Reserved1                 : 1;
        unsigned int St_Op_Sfail               : 3;

        unsigned int Reserved2                 : 1;
        unsigned int St_Op_Zfail               : 3;

        unsigned int Reserved3                 : 1;
        unsigned int St_Op_Pass                : 3;

        unsigned int Reserved4                 : 1;
        unsigned int St_Cmp_Mask               : 8;
        unsigned int St_Wr_Mask                : 8;
    } reg;
} Reg_Stencil_State;

typedef struct _Group_Ff_Stencil_Group
{
    Reg_Stencil_State                reg_Stencil_State;
} Reg_Ff_Stencil_Group_Group;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Depth_Bound_Start;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Depth_Bound_End;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Viewport_Zmin;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Viewport_Zmax;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int S0_Xy                     : 8;
        unsigned int S1_Xy                     : 8;
        unsigned int S2_Xy                     : 8;
        unsigned int S3_Xy                     : 8;
    } reg;
} Reg_User_Msaa_Pattern_Xy;

typedef struct _Group_User_Msaa_Pattern_Group
{
    Reg_User_Msaa_Pattern_Xy         reg_User_Msaa_Pattern_Xy;
} Reg_User_Msaa_Pattern_Group_Group;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Top_Left_X                : 15;
        unsigned int Top_Left_Y                : 15;
        unsigned int Reserved                  : 2;
    } reg;
} Reg_Res_Blk_Topleft_Coord;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Width                     : 15;
        unsigned int Height                    : 15;
        unsigned int Reserved                  : 2;
    } reg;
} Reg_Res_Blk_Size;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Ps_Ref_Level              : 4;
        unsigned int Reserved                  : 28;
    } reg;
} Reg_Res_Blk_Ref_Resolution;

typedef struct _Group_Multi_Res_Blk_Group
{
    Reg_Res_Blk_Topleft_Coord        reg_Res_Blk_Topleft_Coord;
    Reg_Res_Blk_Size                 reg_Res_Blk_Size;
    Reg_Res_Blk_Ref_Resolution       reg_Res_Blk_Ref_Resolution;
} Reg_Multi_Res_Blk_Group_Group;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Vr_Znear                  : 32;
    } reg;
} Reg_Vr_Znear;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Vr_Zfar                   : 32;
    } reg;
} Reg_Vr_Zfar;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Vr_Znear_Level            : 4;
        unsigned int Vr_Zfar_Level             : 4;
        unsigned int Move_Adjust_Level         : 4;

        unsigned int Reserved                  : 20;
    } reg;
} Reg_Vr_Z_Level;

typedef struct _Group_Vr_Zrange_Group
{
    Reg_Vr_Znear                     reg_Vr_Znear;
    Reg_Vr_Zfar                      reg_Vr_Zfar;
    Reg_Vr_Z_Level                   reg_Vr_Z_Level;
} Reg_Vr_Zrange_Group_Group;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Rgb_Sblend                : 5;
        unsigned int Rgb_Dblend                : 5;

        unsigned int Rgb_Blend_Op              : 3;
        unsigned int Alpha_Blend_Op            : 3;

        unsigned int Alpha_Sblend              : 5;

        unsigned int Alpha_Dblend              : 5;

        unsigned int Reserved                  : 6;
    } reg;
} Reg_Bs_Blend_Ctl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int C_G                       : 16;
        unsigned int C_R                       : 16;
    } reg;
} Reg_Blend_Color_Rg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int C_A                       : 16;
        unsigned int C_B                       : 16;
    } reg;
} Reg_Blend_Color_Ba;

typedef struct _Group_Blend_Ctl
{
    Reg_Bs_Blend_Ctl                 reg_Bs_Blend_Ctl;
    Reg_Blend_Color_Rg               reg_Blend_Color_Rg;
    Reg_Blend_Color_Ba               reg_Blend_Color_Ba;
} Reg_Blend_Ctl_Group;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Color                     : 32;
    } reg;
} Reg_Fast_Clear_Color;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Attr_Num                  : 6;
        unsigned int Msaa_State                : 3;

        unsigned int Aa_En                     : 1;
        unsigned int Raster_Mode               : 1;
        unsigned int Reserved                  : 21;
    } reg;
} Reg_Tasbe_Ctrl;

typedef union
{
	unsigned int uint;
	struct
	{
		unsigned int Per_Draw_Sig_En	    	: 1;
		unsigned int Reserved					: 3;
		unsigned int Sig_Base_Addr				: 28;
	} reg;
} Reg_Signature_Ctrl;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Reserved_5;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Reserved_6;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Reserved_7;

typedef struct _Ff_regs
{
    Reg_Ff_Glb_Ctrl                  reg_Ff_Glb_Ctrl;
    Reg_Reserved_1                   reg_Reserved_1;
    Reg_Reserved_2                   reg_Reserved_2;
    Reg_Reserved_3                   reg_Reserved_3;
    Reg_Resolution_Ctrl              reg_Resolution_Ctrl;
    Reg_Dzs_Ctrl                     reg_Dzs_Ctrl;
    Reg_Zs_View_Ctrl                 reg_Zs_View_Ctrl;
    Reg_Zs_Mipmap                    reg_Zs_Mipmap;
    Reg_Zv_Size                      reg_Zv_Size;
    Reg_Zv_Depth                     reg_Zv_Depth;
    Reg_Zv_Desc_Base0                reg_Zv_Desc_Base0;
    Reg_Sv_Desc_Base0                reg_Sv_Desc_Base0;
    Reg_Sv_Bl_Slot_Idx               reg_Sv_Bl_Slot_Idx;
    Reg_Ffc_Config                   reg_Ffc_Config;
    Reg_Ffc_Global_Range_Config      reg_Ffc_Global_Range_Config;
    Reg_Reserved_4                   reg_Reserved_4;
    Reg_Ffc_Dsig_Base_Addr_Group     reg_Ffc_Dsig_Base_Addr[8];
    Reg_Rt_Addr_Ctrl_Group           reg_Rt_Addr_Ctrl[8];
    Reg_Rt_Ctrl_Group                reg_Rt_Ctrl[8];
    Reg_Rast_Ctrl                    reg_Rast_Ctrl;
    Reg_Sg_Ctrl                      reg_Sg_Ctrl;
    Reg_Zs_Req_Ctrl                  reg_Zs_Req_Ctrl;
    Reg_Zs_Ctl                       reg_Zs_Ctl;
    Reg_Ff_Stencil_Group_Group       reg_Ff_Stencil_Group[2];
    Reg_Depth_Bound_Start            reg_Depth_Bound_Start;
    Reg_Depth_Bound_End              reg_Depth_Bound_End;
    Reg_Viewport_Zmin                reg_Viewport_Zmin[16];
    Reg_Viewport_Zmax                reg_Viewport_Zmax[16];
    Reg_User_Msaa_Pattern_Group_Group
                                     reg_User_Msaa_Pattern_Group[4];
    Reg_Multi_Res_Blk_Group_Group    reg_Multi_Res_Blk_Group[18];
    Reg_Vr_Zrange_Group_Group        reg_Vr_Zrange_Group[2];
    Reg_Blend_Ctl_Group              reg_Blend_Ctl[8];
    Reg_Fast_Clear_Color             reg_Fast_Clear_Color[4];
    Reg_Tasbe_Ctrl                   reg_Tasbe_Ctrl;
    Reg_Reserved_5                   reg_Reserved_5;
    Reg_Reserved_6                   reg_Reserved_6;
    Reg_Reserved_7                   reg_Reserved_7;
} Ff_regs;

#endif
