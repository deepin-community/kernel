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

#ifndef _TUFE_REGISTER_H
#define _TUFE_REGISTER_H


#ifndef    TU_BLOCKBASE_INF
#define    TU_BLOCKBASE_INF
#define    BLOCK_TU_VERSION 1
#define    BLOCK_TU_TIMESTAMP "7/10/2018 2:50:05 PM"
#define    TUFE_BLOCK                                               0xA
#define    TUBE_BLOCK                                               0x14
#define    TU_REG_START                                             0x0
#define    TU_REG_END                                               0x580
#define    TU_REG_LIMIT                                             0x580
#endif


#define        Reg_Tu_Tssharp_Ctrl_Offset                                 0x0
#define        Reg_Tu_Tssharp_3d_Ctrl_Offset                              0x1
#define        Reg_Tu_Tssharp_Heap_Base0_Offset                           0x2
#define        Reg_Tu_Tssharp_Heap_Base1_Offset                           0x3
#define        Reg_Tu_Tssharp_Heap_Base2_Offset                           0x4
#define        Reg_Tu_Tssharp_Heap_Base3_Offset                           0x5
#define        Reg_Tu_Tssharp_Heap_Base4_Offset                           0x6
#define        Reg_Tu_Tssharp_Heap_Base5_Offset                           0x7
#define        Reg_Tu_Tssharp_Heap_Base6_Offset                           0x8
#define        Reg_Tu_Tssharp_Heap_Base7_Offset                           0x9
#define        Reg_Tu_Reserved1_Offset                                    0xA
#define        Reg_Tu_Control_Fe_Offset                                   0x10
#define        Reg_Tu_Si_Ctl_Offset                                       0x11
#define        Reg_Tu_Vs_Tsharp_Ctrl_Offset                               0x12
#define        Reg_Tu_Vs_Ssharp_Ctrl_Offset                               0x13
#define        Reg_Tu_Hs_Tsharp_Ctrl_Offset                               0x14
#define        Reg_Tu_Hs_Ssharp_Ctrl_Offset                               0x15
#define        Reg_Tu_Ds_Tsharp_Ctrl_Offset                               0x16
#define        Reg_Tu_Ds_Ssharp_Ctrl_Offset                               0x17
#define        Reg_Tu_Gs_Tsharp_Ctrl_Offset                               0x18
#define        Reg_Tu_Gs_Ssharp_Ctrl_Offset                               0x19
#define        Reg_Tu_Reserved2_Offset                                    0x1A
#define        Reg_Tu_Vb_Offset                                           0x20
#define        Reg_Tu_Ve_Offset                                           0xA0
#define        Reg_Tu_Instance_Offset                                     0xC0
#define        Reg_Tu_Control_Be_Offset                                   0xE0
#define        Reg_Tu_Ps_Tsharp_Ctrl_Offset                               0xE1
#define        Reg_Tu_Ps_Ssharp_Ctrl_Offset                               0xE2
#define        Reg_Tu_Reserved3_Offset                                    0xE3
#define        Reg_Tu_Tssharp_Cs_Ctrl_Offset                              0xE8
#define        Reg_Tu_Csl_Tsharp_Ctrl_Offset                              0xE9
#define        Reg_Tu_Csl_Ssharp_Ctrl_Offset                              0xEA
#define        Reg_Tu_Csh_Tsharp_Ctrl_Offset                              0xEB
#define        Reg_Tu_Csh_Ssharp_Ctrl_Offset                              0xEC
#define        Reg_Tu_Reserved4_Offset                                    0xED
#define		   Reg_Tu_Ts_Sharp_Offset									  0x100




typedef enum
{
    TU_CONTROL_FE_SECTOR_MODE_512B           = 0,
    TU_CONTROL_FE_SECTOR_MODE_1K             = 1,
    TU_CONTROL_FE_SECTOR_MODE_2K             = 2,
    TU_CONTROL_FE_SECTOR_MODE_RESERVED       = 3,
} TU_CONTROL_FE_SECTOR_MODE;
typedef enum
{
    TU_CONTROL_FE_API_MODE_FE_DX             = 0,
    TU_CONTROL_FE_API_MODE_FE_DX9            = 1,
    TU_CONTROL_FE_API_MODE_FE_VULKAN         = 2,
    TU_CONTROL_FE_API_MODE_FE_OGL            = 3,
    TU_CONTROL_FE_API_MODE_FE_OES            = 4,
    TU_CONTROL_FE_API_MODE_FE_OCL            = 5,
} TU_CONTROL_FE_API_MODE_FE;
typedef enum
{
    TU_SI_CTL_API_MODE_DX                    = 0,
    TU_SI_CTL_API_MODE_DX9                   = 1,
    TU_SI_CTL_API_MODE_VULKAN                = 2,
    TU_SI_CTL_API_MODE_OGL                   = 3,
    TU_SI_CTL_API_MODE_OES                   = 4,
    TU_SI_CTL_API_MODE_OCL                   = 5,
} TU_SI_CTL_API_MODE;
typedef enum
{
    TU_CONTROL_BE_SECTOR_MODE_512B           = 0,
    TU_CONTROL_BE_SECTOR_MODE_1K             = 1,
    TU_CONTROL_BE_SECTOR_MODE_2K             = 2,
    TU_CONTROL_BE_SECTOR_MODE_RESERVED       = 3,
} TU_CONTROL_BE_SECTOR_MODE;
typedef enum
{
    TU_CONTROL_BE_API_MODE_BE_DX             = 0,
    TU_CONTROL_BE_API_MODE_BE_DX9            = 1,
    TU_CONTROL_BE_API_MODE_BE_VULKAN         = 2,
    TU_CONTROL_BE_API_MODE_BE_OGL            = 3,
    TU_CONTROL_BE_API_MODE_BE_OES            = 4,
    TU_CONTROL_BE_API_MODE_BE_OCL            = 5,
} TU_CONTROL_BE_API_MODE_BE;
typedef enum
{
    TU_T_SHARP_REG2_MSAA_MODE_MSAA_1X        = 0,
    TU_T_SHARP_REG2_MSAA_MODE_MSAA_2X        = 1,
    TU_T_SHARP_REG2_MSAA_MODE_MSAA_4X        = 2,
    TU_T_SHARP_REG2_MSAA_MODE_MSAA_8X        = 3,
    TU_T_SHARP_REG2_MSAA_MODE_MSAA_16X       = 4,
} TU_T_SHARP_REG2_MSAA_MODE;
typedef enum
{
    TU_T_SHARP_REG2_RES_TYPE_1D_BUFFER       = 0,
    TU_T_SHARP_REG2_RES_TYPE_1D_TEXTURE      = 1,
    TU_T_SHARP_REG2_RES_TYPE_2D_TEXTURE      = 2,
    TU_T_SHARP_REG2_RES_TYPE_3D_TEXTURE      = 3,
    TU_T_SHARP_REG2_RES_TYPE_CUBE_TEXTURE    = 4,
    TU_T_SHARP_REG2_RES_TYPE_1D_TEXTURE_ARRAY= 5,
    TU_T_SHARP_REG2_RES_TYPE_2D_TEXTURE_ARRAY= 6,
    TU_T_SHARP_REG2_RES_TYPE_CUBE_TEXTURE_ARRAY= 7,
    TU_T_SHARP_REG2_RES_TYPE_RAW_BUFFER      = 8,
    TU_T_SHARP_REG2_RES_TYPE_STRUCTURED_BUFFER= 9,
    TU_T_SHARP_REG2_RES_TYPE_VERTEX_BUFFER   = 10,
} TU_T_SHARP_REG2_RES_TYPE;
typedef enum
{
    TU_T_SHARP_REG5_COMPONENT_MAPPING_R_COMPONENT_0= 0,

    TU_T_SHARP_REG5_COMPONENT_MAPPING_R_COMPONENT_1= 1,

    TU_T_SHARP_REG5_COMPONENT_MAPPING_R_COMPONENT_2= 2,

    TU_T_SHARP_REG5_COMPONENT_MAPPING_R_COMPONENT_3= 3,

    TU_T_SHARP_REG5_COMPONENT_MAPPING_R_VALUE_0= 4,
    TU_T_SHARP_REG5_COMPONENT_MAPPING_R_VALUE_1= 5,
} TU_T_SHARP_REG5_COMPONENT_MAPPING_R;
typedef enum
{
    TU_T_SHARP_REG5_COMPONENT_MAPPING_G_COMPONENT_0= 0,

    TU_T_SHARP_REG5_COMPONENT_MAPPING_G_COMPONENT_1= 1,

    TU_T_SHARP_REG5_COMPONENT_MAPPING_G_COMPONENT_2= 2,

    TU_T_SHARP_REG5_COMPONENT_MAPPING_G_COMPONENT_3= 3,

    TU_T_SHARP_REG5_COMPONENT_MAPPING_G_VALUE_0= 4,
    TU_T_SHARP_REG5_COMPONENT_MAPPING_G_VALUE_1= 5,
} TU_T_SHARP_REG5_COMPONENT_MAPPING_G;
typedef enum
{
    TU_T_SHARP_REG5_COMPONENT_MAPPING_B_COMPONENT_0= 0,

    TU_T_SHARP_REG5_COMPONENT_MAPPING_B_COMPONENT_1= 1,

    TU_T_SHARP_REG5_COMPONENT_MAPPING_B_COMPONENT_2= 2,

    TU_T_SHARP_REG5_COMPONENT_MAPPING_B_COMPONENT_3= 3,

    TU_T_SHARP_REG5_COMPONENT_MAPPING_B_VALUE_0= 4,
    TU_T_SHARP_REG5_COMPONENT_MAPPING_B_VALUE_1= 5,
} TU_T_SHARP_REG5_COMPONENT_MAPPING_B;
typedef enum
{
    TU_T_SHARP_REG5_COMPONENT_MAPPING_A_COMPONENT_0= 0,

    TU_T_SHARP_REG5_COMPONENT_MAPPING_A_COMPONENT_1= 1,

    TU_T_SHARP_REG5_COMPONENT_MAPPING_A_COMPONENT_2= 2,

    TU_T_SHARP_REG5_COMPONENT_MAPPING_A_COMPONENT_3= 3,

    TU_T_SHARP_REG5_COMPONENT_MAPPING_A_VALUE_0= 4,
    TU_T_SHARP_REG5_COMPONENT_MAPPING_A_VALUE_1= 5,
} TU_T_SHARP_REG5_COMPONENT_MAPPING_A;
typedef enum
{
    TU_S_SHARP_REG0_COMPARE_FUNC_NEVER       = 0,
    TU_S_SHARP_REG0_COMPARE_FUNC_LESS        = 1,
    TU_S_SHARP_REG0_COMPARE_FUNC_EQUAL       = 2,
    TU_S_SHARP_REG0_COMPARE_FUNC_LESSEQUAL   = 3,
    TU_S_SHARP_REG0_COMPARE_FUNC_GREATER     = 4,
    TU_S_SHARP_REG0_COMPARE_FUNC_NOTEQUAL    = 5,
    TU_S_SHARP_REG0_COMPARE_FUNC_GREATEREQUAL= 6,
    TU_S_SHARP_REG0_COMPARE_FUNC_ALWAYS      = 7,
} TU_S_SHARP_REG0_COMPARE_FUNC;
typedef enum
{
    TU_S_SHARP_REG0_LOD_BRILINEAR_THRESHOLD_BRI_0= 0,

    TU_S_SHARP_REG0_LOD_BRILINEAR_THRESHOLD_BRI_DOT_0625= 1,

    TU_S_SHARP_REG0_LOD_BRILINEAR_THRESHOLD_BRI_DOT_125= 2,

    TU_S_SHARP_REG0_LOD_BRILINEAR_THRESHOLD_BRI_DOT_1875= 3,

} TU_S_SHARP_REG0_LOD_BRILINEAR_THRESHOLD;
typedef enum
{
    TU_S_SHARP_REG0_DEPTH_MODE_NORMAL        = 0,
    TU_S_SHARP_REG0_DEPTH_MODE_AMODE         = 1,
    TU_S_SHARP_REG0_DEPTH_MODE_IMODE         = 2,
    TU_S_SHARP_REG0_DEPTH_MODE_LMODE         = 3,
    TU_S_SHARP_REG0_DEPTH_MODE_LAMODE        = 4,
    TU_S_SHARP_REG0_DEPTH_MODE_RAMODE        = 5,
} TU_S_SHARP_REG0_DEPTH_MODE;
typedef enum
{
    TU_S_SHARP_REG0_FILTER_REDU_TYPE_STANDARD= 0,
    TU_S_SHARP_REG0_FILTER_REDU_TYPE_COMPARISON= 1,
    TU_S_SHARP_REG0_FILTER_REDU_TYPE_MIN     = 2,

    TU_S_SHARP_REG0_FILTER_REDU_TYPE_MAX     = 3,

} TU_S_SHARP_REG0_FILTER_REDU_TYPE;
typedef enum
{
    TU_S_SHARP_REG1_ADDR_U_WRAP              = 0,

    TU_S_SHARP_REG1_ADDR_U_MIRROR            = 1,

    TU_S_SHARP_REG1_ADDR_U_CLAMP             = 2,


    TU_S_SHARP_REG1_ADDR_U_BORDER            = 3,
    TU_S_SHARP_REG1_ADDR_U_MIRROR_ONCE       = 4,
    TU_S_SHARP_REG1_ADDR_U_HALF_BORDER       = 5,
    TU_S_SHARP_REG1_ADDR_U_CUBE_WRAP         = 6,
} TU_S_SHARP_REG1_ADDR_U;
typedef enum
{
    TU_S_SHARP_REG3_YUV_SELECT_CHANNEL_Y     = 0,
    TU_S_SHARP_REG3_YUV_SELECT_CHANNEL_U     = 1,

    TU_S_SHARP_REG3_YUV_SELECT_CHANNEL_V     = 2,
    TU_S_SHARP_REG3_YUV_SELECT_CHANNEL_ALL   = 3,
} TU_S_SHARP_REG3_YUV_SELECT;
typedef enum
{
    TU_S_SHARP_REG3_MAG_FILTER_POINT         = 0,
    TU_S_SHARP_REG3_MAG_FILTER_LINEAR        = 1,
    TU_S_SHARP_REG3_MAG_FILTER_ANISOPOINT    = 2,
    TU_S_SHARP_REG3_MAG_FILTER_ANISO         = 3,
} TU_S_SHARP_REG3_MAG_FILTER;
typedef enum
{
    TU_S_SHARP_REG3_MIN_FILTER_POINT         = 0,
    TU_S_SHARP_REG3_MIN_FILTER_LINEAR        = 1,
    TU_S_SHARP_REG3_MIN_FILTER_ANISOPOINT    = 2,
    TU_S_SHARP_REG3_MIN_FILTER_ANISO         = 3,
} TU_S_SHARP_REG3_MIN_FILTER;
typedef enum
{
    TU_S_SHARP_REG3_MIP_FILTER_NONE          = 0,
    TU_S_SHARP_REG3_MIP_FILTER_POINT         = 1,
    TU_S_SHARP_REG3_MIP_FILTER_LINEAR        = 2,
    TU_S_SHARP_REG3_MIP_FILTER_RESERVED      = 3,
} TU_S_SHARP_REG3_MIP_FILTER;





typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address_3d           : 8;

        unsigned int Reserved                  : 24;
    } reg;
} Reg_Tu_Tssharp_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address_Be           : 8;

        unsigned int Is_3d_Cachemode           : 1;
        unsigned int Reserved                  : 23;
    } reg;
} Reg_Tu_Tssharp_3d_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 32;
    } reg;
} Reg_Tu_Tssharp_Heap_Base0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 32;
    } reg;
} Reg_Tu_Tssharp_Heap_Base1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 32;
    } reg;
} Reg_Tu_Tssharp_Heap_Base2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 32;
    } reg;
} Reg_Tu_Tssharp_Heap_Base3;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 32;
    } reg;
} Reg_Tu_Tssharp_Heap_Base4;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 32;
    } reg;
} Reg_Tu_Tssharp_Heap_Base5;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 32;
    } reg;
} Reg_Tu_Tssharp_Heap_Base6;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 32;
    } reg;
} Reg_Tu_Tssharp_Heap_Base7;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Tu_Reserved1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Sector_Mode               : 2;
        unsigned int Vb_Sector_Mode            : 2;
        unsigned int Reserved_0                : 1;
        unsigned int Enable_Vb_Bank_Swizzle    : 1;
        unsigned int Enable_3dtex_Qswizzle     : 1;

        unsigned int Api_Mode_Fe               : 3;
        unsigned int Reserved_1                : 22;
    } reg;
} Reg_Tu_Control_Fe;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Element_Num               : 7;
        unsigned int Vid_En                    : 1;
        unsigned int Iid_En                    : 1;
        unsigned int Api_Mode                  : 3;
        unsigned int Did_En                    : 1;
        unsigned int Base_Vertex_En            : 1;

        unsigned int Base_Instance_En          : 1;

        unsigned int Reserved                  : 17;
    } reg;
} Reg_Tu_Si_Ctl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 8;
        unsigned int Count                     : 8;
        unsigned int Reserved                  : 16;
    } reg;
} Reg_Tu_Vs_Tsharp_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 8;
        unsigned int Count                     : 8;
        unsigned int Reserved                  : 16;
    } reg;
} Reg_Tu_Vs_Ssharp_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 8;
        unsigned int Count                     : 8;
        unsigned int Reserved                  : 16;
    } reg;
} Reg_Tu_Hs_Tsharp_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 8;
        unsigned int Count                     : 8;
        unsigned int Reserved                  : 16;
    } reg;
} Reg_Tu_Hs_Ssharp_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 8;
        unsigned int Count                     : 8;
        unsigned int Reserved                  : 16;
    } reg;
} Reg_Tu_Ds_Tsharp_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 8;
        unsigned int Count                     : 8;
        unsigned int Reserved                  : 16;
    } reg;
} Reg_Tu_Ds_Ssharp_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 8;
        unsigned int Count                     : 8;
        unsigned int Reserved                  : 16;
    } reg;
} Reg_Tu_Gs_Tsharp_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 8;
        unsigned int Count                     : 8;
        unsigned int Reserved                  : 16;
    } reg;
} Reg_Tu_Gs_Ssharp_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Tu_Reserved2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Resource_Address          : 32;
    } reg;
} Reg_Tu_Vb0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Vb_Size_Inbyte            : 28;
        unsigned int Reserved                  : 4;
    } reg;
} Reg_Tu_Vb1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Vb_Start_Offset           : 32;
    } reg;
} Reg_Tu_Vb2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Vb_Stride_Inbyte          : 12;
    } reg;
} Reg_Tu_Vb3;

typedef struct _Group_Tu_Vb
{
    Reg_Tu_Vb0                       reg_Tu_Vb0;
    Reg_Tu_Vb1                       reg_Tu_Vb1;
    Reg_Tu_Vb2                       reg_Tu_Vb2;
    Reg_Tu_Vb3                       reg_Tu_Vb3;
} Reg_Tu_Vb_Group;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Res_Format                : 9;
        unsigned int Valid                     : 1;
        unsigned int Element_Offset            : 11;

        unsigned int Vb_Id                     : 5;
        unsigned int Instance_En               : 1;
        unsigned int Reserved                  : 5;
    } reg;
} Reg_Tu_Ve;

typedef struct _Group_Tu_Ve
{
    Reg_Tu_Ve                        reg_Tu_Ve;
} Reg_Tu_Ve_Group;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Step_Rate                 : 26;

        unsigned int Reserved                  : 6;
    } reg;
} Reg_Tu_Instance_Ctl;

typedef struct _Group_Tu_Instance
{
    Reg_Tu_Instance_Ctl              reg_Tu_Instance_Ctl;
} Reg_Tu_Instance_Group;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Sector_Mode               : 2;
        unsigned int Reserved_0                : 4;
        unsigned int Enable_3dtex_Qswizzle     : 1;

        unsigned int Api_Mode_Be               : 3;
        unsigned int Reserved_1                : 22;
    } reg;
} Reg_Tu_Control_Be;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 8;
        unsigned int Count                     : 8;
        unsigned int Reserved                  : 16;
    } reg;
} Reg_Tu_Ps_Tsharp_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 8;
        unsigned int Count                     : 8;
        unsigned int Reserved                  : 16;
    } reg;
} Reg_Tu_Ps_Ssharp_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Tu_Reserved3;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address_Csh          : 8;

        unsigned int Is_Csl_Cachemode          : 1;
        unsigned int Is_Csh_Cachemode          : 1;
        unsigned int Reserved                  : 22;
    } reg;
} Reg_Tu_Tssharp_Cs_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 8;
        unsigned int Count                     : 8;
        unsigned int Reserved                  : 16;
    } reg;
} Reg_Tu_Csl_Tsharp_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 8;
        unsigned int Count                     : 8;
        unsigned int Reserved                  : 16;
    } reg;
} Reg_Tu_Csl_Ssharp_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 8;
        unsigned int Count                     : 8;
        unsigned int Reserved                  : 16;
    } reg;
} Reg_Tu_Csh_Tsharp_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Address              : 8;
        unsigned int Count                     : 8;
        unsigned int Reserved                  : 16;
    } reg;
} Reg_Tu_Csh_Ssharp_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Tu_Reserved4_Ctl;

typedef struct _Group_Tu_Reserved4
{
    Reg_Tu_Reserved4_Ctl             reg_Tu_Reserved4_Ctl;
} Reg_Tu_Reserved4_Group;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Res_Address               : 32;
    } reg;
} Reg_Tu_T_Sharp_Reg0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Res_Width                 : 15;
        unsigned int Res_Height                : 15;
        unsigned int Is_Compression            : 1;
        unsigned int Is_Data_Matrix            : 1;
    } reg;
} Reg_Tu_T_Sharp_Reg1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Res_Depth                 : 12;

        unsigned int Res_Format                : 9;
        unsigned int Msaa_Mode                 : 3;
        unsigned int Res_Type                  : 4;
        unsigned int Reserved                  : 4;
    } reg;
} Reg_Tu_T_Sharp_Reg2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Is_Mipmap                 : 1;
        unsigned int Is_Linear_Texture         : 1;
        unsigned int View_Lod_Start            : 4;
        unsigned int View_Lod_End              : 4;
        unsigned int View_Array_Slice_Start    : 11;
        unsigned int View_Array_Slice_End      : 11;
    } reg;
} Reg_Tu_T_Sharp_Reg3;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Bl_Start                  : 18;

        unsigned int Lod_Clamp                 : 12;



        unsigned int Reserved                  : 1;

        unsigned int Is_Bdr_Added              : 1;
    } reg;
} Reg_Tu_T_Sharp_Reg4;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Component_Mapping_R       : 3;
        unsigned int Component_Mapping_G       : 3;
        unsigned int Component_Mapping_B       : 3;
        unsigned int Component_Mapping_A       : 3;
        unsigned int Range_Type                : 6;
        unsigned int Is_Sparse                 : 1;
        unsigned int Valid                     : 1;
        unsigned int Reserved                  : 12;
    } reg;
} Reg_Tu_T_Sharp_Reg5;

typedef struct _Group_Tu_T_Sharp
{
    Reg_Tu_T_Sharp_Reg0              reg_Tu_T_Sharp_Reg0;
    Reg_Tu_T_Sharp_Reg1              reg_Tu_T_Sharp_Reg1;
    Reg_Tu_T_Sharp_Reg2              reg_Tu_T_Sharp_Reg2;
    Reg_Tu_T_Sharp_Reg3              reg_Tu_T_Sharp_Reg3;
    Reg_Tu_T_Sharp_Reg4              reg_Tu_T_Sharp_Reg4;
    Reg_Tu_T_Sharp_Reg5              reg_Tu_T_Sharp_Reg5;
} Reg_Tu_T_Sharp_Group;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Sample_C_Fail_Value       : 8;

        unsigned int Compare_Func              : 3;

        unsigned int Reserved_0                : 3;
        unsigned int Lod_Brilinear_Threshold   : 2;
        unsigned int Uv_Brilinear_Threshold    : 2;
        unsigned int Q_Brilinear_Threshold     : 2;
        unsigned int Depth_Mode                : 3;
        unsigned int Max_Aniso_Ratio           : 4;
        unsigned int Is_Unnormalized_Coord     : 1;
        unsigned int Filter_Redu_Type          : 2;
        unsigned int Degamma_En                : 1;
        unsigned int Reserved_1                : 1;
    } reg;
} Reg_Tu_S_Sharp_Reg0;

typedef union
{
	unsigned int uint;
	struct
	{
		unsigned int Mip_Bias                  : 16;

		unsigned int Addr_U                    : 3;
		unsigned int Addr_V                    : 3;
		unsigned int Addr_R                    : 3;
		unsigned int Is_Non_Seamless_Cube      : 1;
		unsigned int Reserved                  : 6;
	} reg;
} Reg_Tu_S_Sharp_Reg1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Bc_Address                : 32;
    } reg;
} Reg_Tu_S_Sharp_Reg2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Min_Lod                   : 12;
        unsigned int Max_Lod                   : 12;
        unsigned int Yuv_Select                : 2;

        unsigned int Mag_Filter                : 2;
        unsigned int Min_Filter                : 2;
        unsigned int Mip_Filter                : 2;
    } reg;
} Reg_Tu_S_Sharp_Reg3;

typedef struct _Group_Tu_S_Sharp
{
    Reg_Tu_S_Sharp_Reg0              reg_Tu_S_Sharp_Reg0;
    Reg_Tu_S_Sharp_Reg1              reg_Tu_S_Sharp_Reg1;
    Reg_Tu_S_Sharp_Reg2              reg_Tu_S_Sharp_Reg2;
    Reg_Tu_S_Sharp_Reg3              reg_Tu_S_Sharp_Reg3;
} Reg_Tu_S_Sharp_Group;

 typedef union
{
    Reg_Tu_T_Sharp_Group             reg_tsharp;
    Reg_Tu_S_Sharp_Group             reg_ssharp;
} Reg_Tu_Ts_Sharp_Group;

typedef struct _Tu_regs
{
    Reg_Tu_Tssharp_Ctrl              reg_Tu_Tssharp_Ctrl;
    Reg_Tu_Tssharp_3d_Ctrl           reg_Tu_Tssharp_3d_Ctrl;
    Reg_Tu_Tssharp_Heap_Base0        reg_Tu_Tssharp_Heap_Base0;
    Reg_Tu_Tssharp_Heap_Base1        reg_Tu_Tssharp_Heap_Base1;
    Reg_Tu_Tssharp_Heap_Base2        reg_Tu_Tssharp_Heap_Base2;
    Reg_Tu_Tssharp_Heap_Base3        reg_Tu_Tssharp_Heap_Base3;
    Reg_Tu_Tssharp_Heap_Base4        reg_Tu_Tssharp_Heap_Base4;
    Reg_Tu_Tssharp_Heap_Base5        reg_Tu_Tssharp_Heap_Base5;
    Reg_Tu_Tssharp_Heap_Base6        reg_Tu_Tssharp_Heap_Base6;
    Reg_Tu_Tssharp_Heap_Base7        reg_Tu_Tssharp_Heap_Base7;
    Reg_Tu_Reserved1                 reg_Tu_Reserved1[6];
    Reg_Tu_Control_Fe                reg_Tu_Control_Fe;
    Reg_Tu_Si_Ctl                    reg_Tu_Si_Ctl;
    Reg_Tu_Vs_Tsharp_Ctrl            reg_Tu_Vs_Tsharp_Ctrl;
    Reg_Tu_Vs_Ssharp_Ctrl            reg_Tu_Vs_Ssharp_Ctrl;
    Reg_Tu_Hs_Tsharp_Ctrl            reg_Tu_Hs_Tsharp_Ctrl;
    Reg_Tu_Hs_Ssharp_Ctrl            reg_Tu_Hs_Ssharp_Ctrl;
    Reg_Tu_Ds_Tsharp_Ctrl            reg_Tu_Ds_Tsharp_Ctrl;
    Reg_Tu_Ds_Ssharp_Ctrl            reg_Tu_Ds_Ssharp_Ctrl;
    Reg_Tu_Gs_Tsharp_Ctrl            reg_Tu_Gs_Tsharp_Ctrl;
    Reg_Tu_Gs_Ssharp_Ctrl            reg_Tu_Gs_Ssharp_Ctrl;
    Reg_Tu_Reserved2                 reg_Tu_Reserved2[6];
    Reg_Tu_Vb_Group                  reg_Tu_Vb[32];
    Reg_Tu_Ve_Group                  reg_Tu_Ve[32];
    Reg_Tu_Instance_Group            reg_Tu_Instance[32];
    Reg_Tu_Control_Be                reg_Tu_Control_Be;
    Reg_Tu_Ps_Tsharp_Ctrl            reg_Tu_Ps_Tsharp_Ctrl;
    Reg_Tu_Ps_Ssharp_Ctrl            reg_Tu_Ps_Ssharp_Ctrl;
    Reg_Tu_Reserved3                 reg_Tu_Reserved3[5];
    Reg_Tu_Tssharp_Cs_Ctrl           reg_Tu_Tssharp_Cs_Ctrl;
    Reg_Tu_Csl_Tsharp_Ctrl           reg_Tu_Csl_Tsharp_Ctrl;
    Reg_Tu_Csl_Ssharp_Ctrl           reg_Tu_Csl_Ssharp_Ctrl;
    Reg_Tu_Csh_Tsharp_Ctrl           reg_Tu_Csh_Tsharp_Ctrl;
    Reg_Tu_Csh_Ssharp_Ctrl           reg_Tu_Csh_Ssharp_Ctrl;
    Reg_Tu_Reserved4_Group           reg_Tu_Reserved4[19];
    Reg_Tu_Ts_Sharp_Group            reg_Tu_Ts_Sharp[192];
} Tu_regs;

#endif
