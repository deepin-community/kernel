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

#ifndef _MCE_REG_H
#define _MCE_REG_H


#ifndef        MCE_BLOCKBASE_INF
    #define    MCE_BLOCKBASE_INF
    #define    BLOCK_MCE_VERSION 1
    #define    BLOCK_MCE_TIMESTAMP "2017/11/20 9:05:57"
    #define    MCE_BLOCK                                                  0xD
    #define    MCE_REG_START                                              0x0
    #define    MCE_REG_END                                                0x18
    #define    MCE_REG_LIMIT                                              0x18
#endif


#define        Reg_Mce_Ctrl_Offset                                        0x0
#define        Reg_Mce_Rt_Offset                                          0x1
#define        Reg_Mce_Blend_Color_Offset                                 0xf
#define        Reg_Mce_Fast_Clear_Color_Offset                            0x13
#define        Reg_Mce_Global_Range_Config_Offset                         0x17


typedef enum
{
    MCE_CTRL_SRC_BITS_MODE_NORMAL            = 0,

    MCE_CTRL_SRC_BITS_MODE_IS_1BPP           = 1,


    MCE_CTRL_SRC_BITS_MODE_IS_4BPP           = 2,



    MCE_CTRL_SRC_BITS_MODE_UNUSED            = 3,
} MCE_CTRL_SRC_BITS_MODE;
typedef enum
{
    MCE_CTRL_MSAA_EN_MSAA_OFF                = 0,
    MCE_CTRL_MSAA_EN_MSAA_ON                 = 1,
} MCE_CTRL_MSAA_EN;
typedef enum
{
    MCE_CTRL_MSAA_MODE_MSAA_1X               = 0,
    MCE_CTRL_MSAA_MODE_MSAA_2X               = 1,
    MCE_CTRL_MSAA_MODE_MSAA_4X               = 2,
    MCE_CTRL_MSAA_MODE_MSAA_8X               = 3,
    MCE_CTRL_MSAA_MODE_MSAA_16X              = 4,
} MCE_CTRL_MSAA_MODE;
typedef enum
{
    MCE_RT_MISC_DITHER_EN_DISABLE            = 0,
    MCE_RT_MISC_DITHER_EN_ENABLE             = 1,
} MCE_RT_MISC_DITHER_EN;
typedef enum
{
    MCE_RT_MISC_SWIZZLE_3D_EN_DISABLE        = 0,
    MCE_RT_MISC_SWIZZLE_3D_EN_ENABLE         = 1,
} MCE_RT_MISC_SWIZZLE_3D_EN;
typedef enum
{
    MCE_RT_MISC_MIPMAP_EN_DISABLE            = 0,
    MCE_RT_MISC_MIPMAP_EN_ENABLE             = 1,
} MCE_RT_MISC_MIPMAP_EN;
typedef enum
{
    MCE_RT_MISC_RESOURCE_TYPE_1D_BUFFER      = 0,
    MCE_RT_MISC_RESOURCE_TYPE_1D_TEXTURE     = 1,
    MCE_RT_MISC_RESOURCE_TYPE_2D_TEXTURE     = 2,
    MCE_RT_MISC_RESOURCE_TYPE_3D_TEXTURE     = 3,
    MCE_RT_MISC_RESOURCE_TYPE_CUBIC_TEXTURE  = 4,
    MCE_RT_MISC_RESOURCE_TYPE_1D_TEXTURE_ARRAY= 5,
    MCE_RT_MISC_RESOURCE_TYPE_2D_TEXTURE_ARRAY= 6,
    MCE_RT_MISC_RESOURCE_TYPE_CUBE_TEXTURE_ARRAY= 7,
    MCE_RT_MISC_RESOURCE_TYPE_RAW_BUFFER     = 8,
    MCE_RT_MISC_RESOURCE_TYPE_STRUCTURED_BUFFER= 9,
} MCE_RT_MISC_RESOURCE_TYPE;
typedef enum
{
    MCE_BS_BLEND_CTL_RGB_SBLEND_ZERO         = 0,
    MCE_BS_BLEND_CTL_RGB_SBLEND_ONE          = 1,
    MCE_BS_BLEND_CTL_RGB_SBLEND_SRCCOLOR     = 2,
    MCE_BS_BLEND_CTL_RGB_SBLEND_INVSRCCOLOR  = 3,

    MCE_BS_BLEND_CTL_RGB_SBLEND_SRCALPHA     = 4,
    MCE_BS_BLEND_CTL_RGB_SBLEND_INVSRCALPHA  = 5,

    MCE_BS_BLEND_CTL_RGB_SBLEND_DESTALPHA    = 6,
    MCE_BS_BLEND_CTL_RGB_SBLEND_INVDESTALPHA= 7,

    MCE_BS_BLEND_CTL_RGB_SBLEND_DESTCOLOR    = 8,
    MCE_BS_BLEND_CTL_RGB_SBLEND_INVDESTCOLOR= 9,

    MCE_BS_BLEND_CTL_RGB_SBLEND_SRCALPHASAT  = 10,

    MCE_BS_BLEND_CTL_RGB_SBLEND_CONSTANTCOLOR= 11,

    MCE_BS_BLEND_CTL_RGB_SBLEND_INVCONSTANTCOLOR= 12,


    MCE_BS_BLEND_CTL_RGB_SBLEND_CONSTANTALPHA= 13,

    MCE_BS_BLEND_CTL_RGB_SBLEND_INVCONSTANTALPHA= 14,


    MCE_BS_BLEND_CTL_RGB_SBLEND_SRCCONSTANTALPHA= 15,


    MCE_BS_BLEND_CTL_RGB_SBLEND_SRC1_COLOR   = 16,
    MCE_BS_BLEND_CTL_RGB_SBLEND_INVSRC1_COLOR= 17,
    MCE_BS_BLEND_CTL_RGB_SBLEND_SRC1_ALPHA   = 18,
    MCE_BS_BLEND_CTL_RGB_SBLEND_INVSRC1_ALPHA= 19,
} MCE_BS_BLEND_CTL_RGB_SBLEND;
typedef enum
{
    MCE_BS_BLEND_CTL_RGB_BLEND_OP_ADD        = 0,
    MCE_BS_BLEND_CTL_RGB_BLEND_OP_SUBTRACT   = 1,
    MCE_BS_BLEND_CTL_RGB_BLEND_OP_REVSUBTRACT= 2,
    MCE_BS_BLEND_CTL_RGB_BLEND_OP_MAX_OP     = 3,
    MCE_BS_BLEND_CTL_RGB_BLEND_OP_MIN_OP     = 4,
} MCE_BS_BLEND_CTL_RGB_BLEND_OP;





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

        unsigned int Msaa_En                   : 1;
        unsigned int Msaa_Mode                 : 3;
        unsigned int Reserved                  : 15;
    } reg;
} Reg_Mce_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Addr                 : 32;
    } reg;
} Reg_Mce_Rt_Addr;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Format                    : 9;
        unsigned int Bl_Slot_Idx               : 18;
        unsigned int Reserved                  : 5;
    } reg;
} Reg_Mce_Rt_Fmt;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Width                     : 15;
        unsigned int Height                    : 15;
        unsigned int Reserved                  : 2;
    } reg;
} Reg_Mce_Rt_Size;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Depth_Or_Arraysize        : 12;
        unsigned int Lod                       : 4;
        unsigned int Range_Type                : 6;
        unsigned int Reserved                  : 10;
    } reg;
} Reg_Mce_Rt_Depth;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Arraysize                 : 12;
        unsigned int First_Array_Idx           : 11;
        unsigned int Reserved                  : 9;
    } reg;
} Reg_Mce_Rt_View_Ctrl;

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
        unsigned int Dual_Source_En            : 1;
        unsigned int Swizzle_3d_En             : 1;
        unsigned int Mipmap_En                 : 1;
        unsigned int Resource_Type             : 4;
        unsigned int Rop                       : 5;



















        unsigned int Reserved                  : 10;
    } reg;
} Reg_Mce_Rt_Misc;

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
} Reg_Mce_Bs_Blend_Ctl;

typedef struct _Group_Mce_Rt
{
    Reg_Mce_Rt_Addr                  reg_Mce_Rt_Addr;
    Reg_Mce_Rt_Fmt                   reg_Mce_Rt_Fmt;
    Reg_Mce_Rt_Size                  reg_Mce_Rt_Size;
    Reg_Mce_Rt_Depth                 reg_Mce_Rt_Depth;
    Reg_Mce_Rt_View_Ctrl             reg_Mce_Rt_View_Ctrl;
    Reg_Mce_Rt_Misc                  reg_Mce_Rt_Misc;
    Reg_Mce_Bs_Blend_Ctl             reg_Mce_Bs_Blend_Ctl;
} Reg_Mce_Rt_Group;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int C_R                       : 16;
        unsigned int C_G                       : 16;
    } reg;
} Reg_Mce_Blend_Color_Rg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int C_B                       : 16;
        unsigned int C_A                       : 16;
    } reg;
} Reg_Mce_Blend_Color_Ba;

typedef struct _Group_Mce_Blend_Color
{
    Reg_Mce_Blend_Color_Rg           reg_Mce_Blend_Color_Rg;
    Reg_Mce_Blend_Color_Ba           reg_Mce_Blend_Color_Ba;
} Reg_Mce_Blend_Color_Group;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Color                     : 32;
    } reg;
} Reg_Mce_Fast_Clear_Color;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Global_Start              : 8;
        unsigned int Global_End                : 8;
        unsigned int Reserved                  : 16;
    } reg;
} Reg_Mce_Global_Range_Config;

typedef struct _Mce_regs
{
    Reg_Mce_Ctrl                     reg_Mce_Ctrl;
    Reg_Mce_Rt_Group                 reg_Mce_Rt[2];
    Reg_Mce_Blend_Color_Group        reg_Mce_Blend_Color[2];
    Reg_Mce_Fast_Clear_Color         reg_Mce_Fast_Clear_Color[4];
    Reg_Mce_Global_Range_Config      reg_Mce_Global_Range_Config;
} Mce_regs;

#endif
