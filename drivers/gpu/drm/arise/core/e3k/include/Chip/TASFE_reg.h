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

#ifndef _TASFE_REG_H
#define _TASFE_REG_H


#ifndef        TASFE_BLOCKBASE_INF
    #define    TASFE_BLOCKBASE_INF
    #define    BLOCK_TASFE_VERSION 1
    #define    BLOCK_TASFE_TIMESTAMP "7/18/2018 12:13:43 PM"
    #define    TASFE_BLOCK                                                0x4
    #define    TASFE_REG_START                                            0x0
    #define    TASFE_REG_END                                              0xC0
    #define    TASFE_REG_LIMIT                                            0xC0
#endif


#define        Reg_Tasfe_Ctrl_Offset                                      0x0
#define        Reg_Tasfe_Ctrl_Misc_Offset                                 0x1
#define        Reg_Pre_Setup_Ctrl_Offset                                  0x2
#define        Reg_Gb_Exp_Offset                                          0x3
#define        Reg_Scissor_X_Offset                                       0xB
#define        Reg_Scissor_Y_Offset                                       0x1B
#define        Reg_Viewport_Xscale_Offset                                 0x2B
#define        Reg_Viewport_Xoffset_Offset                                0x3B
#define        Reg_Viewport_Yscale_Offset                                 0x4B
#define        Reg_Viewport_Yoffset_Offset                                0x5B
#define        Reg_Viewport_Zscale_Offset                                 0x6B
#define        Reg_Viewport_Zoffset_Offset                                0x7B
#define        Reg_Wscaling_Xcoeff_Offset                                 0x8B
#define        Reg_Wscaling_Ycoeff_Offset                                 0x9B
#define        Reg_Viewport_Swizzle_Offset                                0xAB
#define        Reg_Point_Size_Offset                                      0xB3
#define        Reg_Line_Width_Offset                                      0xB4
#define        Reg_Z_Bias_Offset                                          0xB5
#define        Reg_Z_Scale_Factor_Offset                                  0xB6
#define        Reg_Z_Bias_Clamp_Offset                                    0xB7
#define        Reg_Attr_Const_Persp_Offset                                0xB8
#define        Reg_Tasfe_Reserved_Offset                                  0xBB


typedef enum
{
    TASFE_CTRL_AS_EN_DISABLED                = 0,
    TASFE_CTRL_AS_EN_ENABLED                 = 1,
} TASFE_CTRL_AS_EN;
typedef enum
{
    TASFE_CTRL_ZERO_DET_MODE_FORCE_REJECT    = 0,
    TASFE_CTRL_ZERO_DET_MODE_BACK_FACE       = 1,
} TASFE_CTRL_ZERO_DET_MODE;
typedef enum
{
    TASFE_CTRL_CULL_EN_DISABLED              = 0,
    TASFE_CTRL_CULL_EN_ENABLED               = 1,
} TASFE_CTRL_CULL_EN;
typedef enum
{
    TASFE_CTRL_CULL_MODE_CULL_BACK           = 0,
    TASFE_CTRL_CULL_MODE_CULL_FRONT          = 1,
} TASFE_CTRL_CULL_MODE;
typedef enum
{
    TASFE_CTRL_WINDING_FRONT_CW              = 0,
    TASFE_CTRL_WINDING_FRONT_CCW             = 1,
} TASFE_CTRL_WINDING;
typedef enum
{
    TASFE_CTRL_ZCLIP_EN_DISABLED             = 0,
    TASFE_CTRL_ZCLIP_EN_ENABLED              = 1,
} TASFE_CTRL_ZCLIP_EN;
typedef enum
{
    TASFE_CTRL_ZOFFSET_FILL_EN_DISABLED      = 0,
    TASFE_CTRL_ZOFFSET_FILL_EN_ENABLED       = 1,
} TASFE_CTRL_ZOFFSET_FILL_EN;
typedef enum
{
    TASFE_CTRL_ZOFFSET_LINE_EN_DISABLED      = 0,
    TASFE_CTRL_ZOFFSET_LINE_EN_ENABLED       = 1,
} TASFE_CTRL_ZOFFSET_LINE_EN;
typedef enum
{
    TASFE_CTRL_ZOFFSET_POINT_EN_DISABLED     = 0,
    TASFE_CTRL_ZOFFSET_POINT_EN_ENABLED      = 1,
} TASFE_CTRL_ZOFFSET_POINT_EN;
typedef enum
{
    TASFE_CTRL_STRICT_LINE_EN_DISABLED       = 0,
    TASFE_CTRL_STRICT_LINE_EN_ENABLED        = 1,
} TASFE_CTRL_STRICT_LINE_EN;
typedef enum
{
    TASFE_CTRL_NON_PERSPECTIVE_EN_DISABLED   = 0,
    TASFE_CTRL_NON_PERSPECTIVE_EN_ENABLED    = 1,
} TASFE_CTRL_NON_PERSPECTIVE_EN;
typedef enum
{
    TASFE_CTRL_PTTRI_EN_DISABLE              = 0,
    TASFE_CTRL_PTTRI_EN_ENABLE               = 1,
} TASFE_CTRL_PTTRI_EN;
typedef enum
{
    TASFE_CTRL_POINT_SIZE_EN_DISABLED        = 0,

    TASFE_CTRL_POINT_SIZE_EN_ENABLED         = 1,
} TASFE_CTRL_POINT_SIZE_EN;
typedef enum
{
    TASFE_CTRL_LAST_PIXEL_DISABLED           = 0,
    TASFE_CTRL_LAST_PIXEL_ENABLED            = 1,
} TASFE_CTRL_LAST_PIXEL;
typedef enum
{
    TASFE_CTRL_LEADING_VTX_VTX0              = 0,

    TASFE_CTRL_LEADING_VTX_VTX1              = 1,

} TASFE_CTRL_LEADING_VTX;
typedef enum
{
    TASFE_CTRL_Z32F_EN_DISABLED              = 0,
    TASFE_CTRL_Z32F_EN_ENABLED               = 1,
} TASFE_CTRL_Z32F_EN;
typedef enum
{
    TASFE_CTRL_Z_SETUP_MODE_LINE_BASED       = 0,
    TASFE_CTRL_Z_SETUP_MODE_TRIANGLE_BASED   = 1,
} TASFE_CTRL_Z_SETUP_MODE;
typedef enum
{
    TASFE_CTRL_RHW_SETUP_MODE_LINE_BASED     = 0,
    TASFE_CTRL_RHW_SETUP_MODE_TRIANGLE_BASED= 1,
} TASFE_CTRL_RHW_SETUP_MODE;
typedef enum
{
    TASFE_CTRL_FRONT_POLYGON_MODE_SOLID      = 0,
    TASFE_CTRL_FRONT_POLYGON_MODE_WIREFRAME  = 1,
    TASFE_CTRL_FRONT_POLYGON_MODE_POINT      = 2,
} TASFE_CTRL_FRONT_POLYGON_MODE;
typedef enum
{
    TASFE_CTRL_BACK_POLYGON_MODE_SOLID       = 0,
    TASFE_CTRL_BACK_POLYGON_MODE_WIREFRAME   = 1,
    TASFE_CTRL_BACK_POLYGON_MODE_POINT       = 2,
} TASFE_CTRL_BACK_POLYGON_MODE;
typedef enum
{
    TASFE_CTRL_DEPTH_MODE_ZERO_TO_ONE        = 0,
    TASFE_CTRL_DEPTH_MODE_NEGATIVE_ONE_TO_ONE= 1,
} TASFE_CTRL_DEPTH_MODE;
typedef enum
{
    TASFE_CTRL_FORCE_Z_GBC_FALSE             = 0,
    TASFE_CTRL_FORCE_Z_GBC_TRUE              = 1,
} TASFE_CTRL_FORCE_Z_GBC;
typedef enum
{
    TASFE_CTRL_API_VERSION_OGL               = 0,
    TASFE_CTRL_API_VERSION_DX9               = 1,
    TASFE_CTRL_API_VERSION_DX10              = 2,
    TASFE_CTRL_API_VERSION_DX11              = 3,
    TASFE_CTRL_API_VERSION_DX12              = 4,
} TASFE_CTRL_API_VERSION;
typedef enum
{
    TASFE_CTRL_MSAA_STATE_MSAA_1X            = 0,
    TASFE_CTRL_MSAA_STATE_MSAA_2X            = 1,
    TASFE_CTRL_MSAA_STATE_MSAA_4X            = 2,
    TASFE_CTRL_MSAA_STATE_MSAA_8X            = 3,
    TASFE_CTRL_MSAA_STATE_MSAA_16X           = 4,
} TASFE_CTRL_MSAA_STATE;
typedef enum
{
    TASFE_CTRL_AA_EN_FALSE                   = 0,
    TASFE_CTRL_AA_EN_TRUE                    = 1,
} TASFE_CTRL_AA_EN;
typedef enum
{
    TASFE_CTRL_RASTER_MODE_NORMAL            = 0,
    TASFE_CTRL_RASTER_MODE_CONSERVATIVE      = 1,
} TASFE_CTRL_RASTER_MODE;
typedef enum
{
    TASFE_CTRL_MISC_VIEWPORT_IDX_EN_DISABLED= 0,
    TASFE_CTRL_MISC_VIEWPORT_IDX_EN_ENABLED  = 1,
} TASFE_CTRL_MISC_VIEWPORT_IDX_EN;
typedef enum
{
    TASFE_CTRL_MISC_STO_EN_DISABLED          = 0,
    TASFE_CTRL_MISC_STO_EN_ENABLED           = 1,
} TASFE_CTRL_MISC_STO_EN;
typedef enum
{
    TASFE_CTRL_MISC_POINT_SPRITE_EN_DISABLED= 0,
    TASFE_CTRL_MISC_POINT_SPRITE_EN_ENABLED  = 1,

} TASFE_CTRL_MISC_POINT_SPRITE_EN;
typedef enum
{
    TASFE_CTRL_MISC_TILE_SIZE_256X128        = 0,
    TASFE_CTRL_MISC_TILE_SIZE_128X128        = 1,
    TASFE_CTRL_MISC_TILE_SIZE_128X64         = 2,
    TASFE_CTRL_MISC_TILE_SIZE_64X64          = 3,
    TASFE_CTRL_MISC_TILE_SIZE_64X32          = 4,
    TASFE_CTRL_MISC_TILE_SIZE_32X32          = 5,
    TASFE_CTRL_MISC_TILE_SIZE_32X16          = 6,
    TASFE_CTRL_MISC_TILE_SIZE_16X16          = 7,
    TASFE_CTRL_MISC_TILE_SIZE_16X8           = 8,
    TASFE_CTRL_MISC_TILE_SIZE_8X8            = 9,
} TASFE_CTRL_MISC_TILE_SIZE;
typedef enum
{
    TASFE_CTRL_MISC_SPECIAL_ROTATE_FALSE     = 0,
    TASFE_CTRL_MISC_SPECIAL_ROTATE_TRUE      = 1,
} TASFE_CTRL_MISC_SPECIAL_ROTATE;
typedef enum
{
    TASFE_CTRL_MISC_PRIMID_APPEND_EN_FALSE   = 0,
    TASFE_CTRL_MISC_PRIMID_APPEND_EN_TRUE    = 1,
} TASFE_CTRL_MISC_PRIMID_APPEND_EN;
typedef enum
{
    TASFE_CTRL_MISC_RULE_FOR_TRIANGLE_TOP_LEFT= 0,
    TASFE_CTRL_MISC_RULE_FOR_TRIANGLE_TOP_RIGHT= 1,
    TASFE_CTRL_MISC_RULE_FOR_TRIANGLE_BOTTOM_LEFT= 2,

    TASFE_CTRL_MISC_RULE_FOR_TRIANGLE_BOTTON_RIGHT= 3,

} TASFE_CTRL_MISC_RULE_FOR_TRIANGLE;
typedef enum
{
    TASFE_CTRL_MISC_QUAD_LINE_EN_FALSE       = 0,
    TASFE_CTRL_MISC_QUAD_LINE_EN_TRUE        = 1,
} TASFE_CTRL_MISC_QUAD_LINE_EN;
typedef enum
{
    PRE_SETUP_CTRL_PRIMID_PACK_EN_FALSE      = 0,
    PRE_SETUP_CTRL_PRIMID_PACK_EN_TRUE       = 1,
} PRE_SETUP_CTRL_PRIMID_PACK_EN;
typedef enum
{
    PRE_SETUP_CTRL_PTSPRITE_PERSP_FALSE      = 0,
    PRE_SETUP_CTRL_PTSPRITE_PERSP_TRUE       = 1,
} PRE_SETUP_CTRL_PTSPRITE_PERSP;
typedef enum
{
    PRE_SETUP_CTRL_EDGE_FLAG_EN_FALSE        = 0,
    PRE_SETUP_CTRL_EDGE_FLAG_EN_TRUE         = 1,
} PRE_SETUP_CTRL_EDGE_FLAG_EN;
typedef enum
{
    PRE_SETUP_CTRL_VIEWPORT_MASK_EN_FALSE    = 0,
    PRE_SETUP_CTRL_VIEWPORT_MASK_EN_TRUE     = 1,
} PRE_SETUP_CTRL_VIEWPORT_MASK_EN;
typedef enum
{
    PRE_SETUP_CTRL_VIEWPORT_SWIZZLE_EN_FALSE= 0,
    PRE_SETUP_CTRL_VIEWPORT_SWIZZLE_EN_TRUE  = 1,
} PRE_SETUP_CTRL_VIEWPORT_SWIZZLE_EN;
typedef enum
{
    PRE_SETUP_CTRL_VIEWPORT_RELATIVE_FALSE   = 0,
    PRE_SETUP_CTRL_VIEWPORT_RELATIVE_TRUE    = 1,
} PRE_SETUP_CTRL_VIEWPORT_RELATIVE;
typedef enum
{
    PRE_SETUP_CTRL_W_SCALING_EN_FALSE        = 0,
    PRE_SETUP_CTRL_W_SCALING_EN_TRUE         = 1,
} PRE_SETUP_CTRL_W_SCALING_EN;
typedef enum
{
    PRE_SETUP_CTRL_TRANSFORM_EN_FALSE        = 0,
    PRE_SETUP_CTRL_TRANSFORM_EN_TRUE         = 1,
} PRE_SETUP_CTRL_TRANSFORM_EN;
typedef enum
{
    PRE_SETUP_CTRL_DO_CLIPPING_FOR_POINT_FALSE= 0,
    PRE_SETUP_CTRL_DO_CLIPPING_FOR_POINT_TRUE= 1,
} PRE_SETUP_CTRL_DO_CLIPPING_FOR_POINT;
typedef enum
{
    PRE_SETUP_CTRL_PTSPRITE_INVERT_POSITIVE_U_POSITIVE_V= 0,

    PRE_SETUP_CTRL_PTSPRITE_INVERT_POSITIVE_U_NEGATIVE_V= 1,

    PRE_SETUP_CTRL_PTSPRITE_INVERT_NEGATIVE_U_POSITIVE_V= 2,

    PRE_SETUP_CTRL_PTSPRITE_INVERT_NEGATIVE_U_NEGATIVE_V= 3,

    PRE_SETUP_CTRL_PTSPRITE_INVERT_POSITIVE_V_POSITIVE_U= 4,

    PRE_SETUP_CTRL_PTSPRITE_INVERT_POSITIVE_V_NEGATIVE_U= 5,

    PRE_SETUP_CTRL_PTSPRITE_INVERT_NEGATIVE_V_POSITIVE_U= 6,

    PRE_SETUP_CTRL_PTSPRITE_INVERT_NEGATIVE_V_NEGATIVE_U= 7,

} PRE_SETUP_CTRL_PTSPRITE_INVERT;
typedef enum
{
    PRE_SETUP_CTRL_SKIP_FRUSTUM_REJ_FALSE    = 0,
    PRE_SETUP_CTRL_SKIP_FRUSTUM_REJ_TRUE     = 1,
} PRE_SETUP_CTRL_SKIP_FRUSTUM_REJ;
typedef enum
{
    PRE_SETUP_CTRL_SKIP_SMALLTRI_REJ_FALSE   = 0,
    PRE_SETUP_CTRL_SKIP_SMALLTRI_REJ_TRUE    = 1,
} PRE_SETUP_CTRL_SKIP_SMALLTRI_REJ;
typedef enum
{
    VIEWPORT_SWIZZLE_X_SWIZZLE0__POSITIVE_X  = 0,
    VIEWPORT_SWIZZLE_X_SWIZZLE0_NEGATIVE_X   = 1,
    VIEWPORT_SWIZZLE_X_SWIZZLE0__POSITIVE_Y  = 2,
    VIEWPORT_SWIZZLE_X_SWIZZLE0_NEGATIVE_Y   = 3,
    VIEWPORT_SWIZZLE_X_SWIZZLE0__POSITIVE_Z  = 4,
    VIEWPORT_SWIZZLE_X_SWIZZLE0_NEGATIVE_Z   = 5,
    VIEWPORT_SWIZZLE_X_SWIZZLE0__POSITIVE_W  = 6,
    VIEWPORT_SWIZZLE_X_SWIZZLE0_NEGATIVE_W   = 7,
} VIEWPORT_SWIZZLE_X_SWIZZLE0;
typedef enum
{
    VIEWPORT_SWIZZLE_Y_SWIZZLE0__POSITIVE_X  = 0,
    VIEWPORT_SWIZZLE_Y_SWIZZLE0_NEGATIVE_X   = 1,
    VIEWPORT_SWIZZLE_Y_SWIZZLE0__POSITIVE_Y  = 2,
    VIEWPORT_SWIZZLE_Y_SWIZZLE0_NEGATIVE_Y   = 3,
    VIEWPORT_SWIZZLE_Y_SWIZZLE0__POSITIVE_Z  = 4,
    VIEWPORT_SWIZZLE_Y_SWIZZLE0_NEGATIVE_Z   = 5,
    VIEWPORT_SWIZZLE_Y_SWIZZLE0__POSITIVE_W  = 6,
    VIEWPORT_SWIZZLE_Y_SWIZZLE0_NEGATIVE_W   = 7,
} VIEWPORT_SWIZZLE_Y_SWIZZLE0;
typedef enum
{
    VIEWPORT_SWIZZLE_Z_SWIZZLE0__POSITIVE_X  = 0,
    VIEWPORT_SWIZZLE_Z_SWIZZLE0_NEGATIVE_X   = 1,
    VIEWPORT_SWIZZLE_Z_SWIZZLE0__POSITIVE_Y  = 2,
    VIEWPORT_SWIZZLE_Z_SWIZZLE0_NEGATIVE_Y   = 3,
    VIEWPORT_SWIZZLE_Z_SWIZZLE0__POSITIVE_Z  = 4,
    VIEWPORT_SWIZZLE_Z_SWIZZLE0_NEGATIVE_Z   = 5,
    VIEWPORT_SWIZZLE_Z_SWIZZLE0__POSITIVE_W  = 6,
    VIEWPORT_SWIZZLE_Z_SWIZZLE0_NEGATIVE_W   = 7,
} VIEWPORT_SWIZZLE_Z_SWIZZLE0;
typedef enum
{
    VIEWPORT_SWIZZLE_W_SWIZZLE0__POSITIVE_X  = 0,
    VIEWPORT_SWIZZLE_W_SWIZZLE0_NEGATIVE_X   = 1,
    VIEWPORT_SWIZZLE_W_SWIZZLE0__POSITIVE_Y  = 2,
    VIEWPORT_SWIZZLE_W_SWIZZLE0_NEGATIVE_Y   = 3,
    VIEWPORT_SWIZZLE_W_SWIZZLE0__POSITIVE_Z  = 4,
    VIEWPORT_SWIZZLE_W_SWIZZLE0_NEGATIVE_Z   = 5,
    VIEWPORT_SWIZZLE_W_SWIZZLE0__POSITIVE_W  = 6,
    VIEWPORT_SWIZZLE_W_SWIZZLE0_NEGATIVE_W   = 7,
} VIEWPORT_SWIZZLE_W_SWIZZLE0;





typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int As_En                     : 1;
        unsigned int Zero_Det_Mode             : 1;
        unsigned int Cull_En                   : 1;
        unsigned int Cull_Mode                 : 1;
        unsigned int Winding                   : 1;
        unsigned int Zclip_En                  : 1;
        unsigned int Zoffset_Fill_En           : 1;
        unsigned int Zoffset_Line_En           : 1;
        unsigned int Zoffset_Point_En          : 1;
        unsigned int Strict_Line_En            : 1;
        unsigned int Non_Perspective_En        : 1;
        unsigned int Pttri_En                  : 1;
        unsigned int Point_Size_En             : 1;
        unsigned int Last_Pixel                : 1;
        unsigned int Leading_Vtx               : 1;
        unsigned int Z32f_En                   : 1;
        unsigned int Z_Setup_Mode              : 1;
        unsigned int Rhw_Setup_Mode            : 1;
        unsigned int Front_Polygon_Mode        : 2;
        unsigned int Back_Polygon_Mode         : 2;
        unsigned int Depth_Mode                : 1;
        unsigned int Force_Z_Gbc               : 1;
        unsigned int Api_Version               : 3;


        unsigned int Msaa_State                : 3;

        unsigned int Aa_En                     : 1;
        unsigned int Raster_Mode               : 1;
    } reg;
} Reg_Tasfe_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Attr_Num                  : 6;
        unsigned int Viewport_Idx_En           : 1;
        unsigned int Sto_En                    : 1;
        unsigned int Point_Sprite_En           : 1;
        unsigned int Tile_Size                 : 4;
        unsigned int Special_Rotate            : 1;
        unsigned int Primid_Append_En          : 1;




        unsigned int Pos_End_Ptr               : 10;
        unsigned int Rule_For_Triangle         : 2;
        unsigned int Rule_For_A_Line           : 2;
        unsigned int Rule_For_Point            : 2;
        unsigned int Quad_Line_En              : 1;
    } reg;
} Reg_Tasfe_Ctrl_Misc;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Primid_Pack_En            : 1;
        unsigned int Primid_Pack_Attr          : 6;

        unsigned int Primid_Pack_Channel       : 2;

        unsigned int Ptsprite_Persp            : 1;
        unsigned int Edge_Flag_En              : 1;
        unsigned int Reserved                  : 5;
        unsigned int Viewport_Mask_En          : 1;
        unsigned int Viewport_Swizzle_En       : 1;
        unsigned int Viewport_Relative         : 1;
        unsigned int W_Scaling_En              : 1;
        unsigned int Transform_En              : 1;
        unsigned int Do_Clipping_For_Point     : 1;
        unsigned int Ptsprite_Invert           : 3;
        unsigned int Skip_Frustum_Rej          : 1;
        unsigned int Gb_Size                   : 5;
        unsigned int Skip_Smalltri_Rej         : 1;
    } reg;
} Reg_Pre_Setup_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int X_Gb_Exp0                 : 8;

        unsigned int Y_Gb_Exp0                 : 8;

        unsigned int X_Gb_Exp1                 : 8;

        unsigned int Y_Gb_Exp1                 : 8;

    } reg;
} Reg_Gb_Exp;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int X_Min                     : 16;
        unsigned int X_Max                     : 16;
    } reg;
} Reg_Scissor_X;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Y_Min                     : 16;

        unsigned int Y_Max                     : 16;

    } reg;
} Reg_Scissor_Y;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int X_Scale                   : 32;
    } reg;
} Reg_Viewport_Xscale;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int X_Offset                  : 32;
    } reg;
} Reg_Viewport_Xoffset;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Y_Scale                   : 32;
    } reg;
} Reg_Viewport_Yscale;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Y_Offset                  : 32;
    } reg;
} Reg_Viewport_Yoffset;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Z_Scale                   : 32;
    } reg;
} Reg_Viewport_Zscale;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Z_Offset                  : 32;
    } reg;
} Reg_Viewport_Zoffset;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int X_Coeff                   : 32;
    } reg;
} Reg_Wscaling_Xcoeff;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Y_Coeff                   : 32;
    } reg;
} Reg_Wscaling_Ycoeff;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int X_Swizzle0                : 3;
        unsigned int Y_Swizzle0                : 3;
        unsigned int Z_Swizzle0                : 3;
        unsigned int W_Swizzle0                : 3;
        unsigned int X_Swizzle1                : 3;
        unsigned int Y_Swizzle1                : 3;
        unsigned int Z_Swizzle1                : 3;
        unsigned int W_Swizzle1                : 3;
        unsigned int Reserved                  : 8;
    } reg;
} Reg_Viewport_Swizzle;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Point_Size                : 27;

        unsigned int Reserved                  : 5;
    } reg;
} Reg_Point_Size;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Line_Width                : 27;
        unsigned int Reserved                  : 5;
    } reg;
} Reg_Line_Width;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Zbias                     : 32;
    } reg;
} Reg_Z_Bias;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Factor                    : 32;
    } reg;
} Reg_Z_Scale_Factor;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Zbias_Clamp               : 32;

    } reg;
} Reg_Z_Bias_Clamp;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Attr0_Const_Persp         : 2;

        unsigned int Attr1_Const_Persp         : 2;

        unsigned int Attr2_Const_Persp         : 2;

        unsigned int Attr3_Const_Persp         : 2;

        unsigned int Attr4_Const_Persp         : 2;

        unsigned int Attr5_Const_Persp         : 2;

        unsigned int Attr6_Const_Persp         : 2;

        unsigned int Attr7_Const_Persp         : 2;

        unsigned int Attr8_Const_Persp         : 2;

        unsigned int Attr9_Const_Persp         : 2;

        unsigned int Attr10_Const_Persp        : 2;

        unsigned int Attr11_Const_Persp        : 2;

        unsigned int Attr12_Const_Persp        : 2;

        unsigned int Attr13_Const_Persp        : 2;

        unsigned int Attr14_Const_Persp        : 2;

        unsigned int Attr15_Const_Persp        : 2;

    } reg;
} Reg_Attr_Const_Persp;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Tasfe_Reserved;

typedef struct _Tasfe_regs
{
    Reg_Tasfe_Ctrl                   reg_Tasfe_Ctrl;
    Reg_Tasfe_Ctrl_Misc              reg_Tasfe_Ctrl_Misc;
    Reg_Pre_Setup_Ctrl               reg_Pre_Setup_Ctrl;
    Reg_Gb_Exp                       reg_Gb_Exp[8];
    Reg_Scissor_X                    reg_Scissor_X[16];
    Reg_Scissor_Y                    reg_Scissor_Y[16];
    Reg_Viewport_Xscale              reg_Viewport_Xscale[16];
    Reg_Viewport_Xoffset             reg_Viewport_Xoffset[16];
    Reg_Viewport_Yscale              reg_Viewport_Yscale[16];
    Reg_Viewport_Yoffset             reg_Viewport_Yoffset[16];
    Reg_Viewport_Zscale              reg_Viewport_Zscale[16];
    Reg_Viewport_Zoffset             reg_Viewport_Zoffset[16];
    Reg_Wscaling_Xcoeff              reg_Wscaling_Xcoeff[16];
    Reg_Wscaling_Ycoeff              reg_Wscaling_Ycoeff[16];
    Reg_Viewport_Swizzle             reg_Viewport_Swizzle[8];
    Reg_Point_Size                   reg_Point_Size;
    Reg_Line_Width                   reg_Line_Width;
    Reg_Z_Bias                       reg_Z_Bias;
    Reg_Z_Scale_Factor               reg_Z_Scale_Factor;
    Reg_Z_Bias_Clamp                 reg_Z_Bias_Clamp;
    Reg_Attr_Const_Persp             reg_Attr_Const_Persp[3];
    Reg_Tasfe_Reserved               reg_Tasfe_Reserved[5];
} Tasfe_regs;

#endif
