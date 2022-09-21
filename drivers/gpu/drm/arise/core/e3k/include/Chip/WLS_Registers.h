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

#ifndef _WLS_REGISTERS_H
#define _WLS_REGISTERS_H


#ifndef        WLS_FE_BLOCKBASE_INF
    #define    WLS_FE_BLOCKBASE_INF
    #define    BLOCK_WLS_FE_VERSION 1
    #define    BLOCK_WLS_FE_TIMESTAMP "2018-10-12 10:15:13"
    #define    WLS_FE_BLOCK                                               0x8
    #define    WLS_FE_REG_START                                           0x0
    #define    WLS_FE_REG_END                                             0x418
    #define    WLS_FE_REG_LIMIT                                           0x418
#endif

#ifndef        WLS_BE_BLOCKBASE_INF
    #define    WLS_BE_BLOCKBASE_INF
    #define    BLOCK_WLS_BE_VERSION 1
    #define    BLOCK_WLS_BE_TIMESTAMP "2017-08-31 12:06:11"
    #define    WLS_BE_BLOCK                                               0x13
    #define    WLS_BE_REG_START                                           0x0
    #define    WLS_BE_REG_END                                             0x418
    #define    WLS_BE_REG_LIMIT                                           0x418
#endif


#define        Reg_Ffc_Ubuf_Golbalconfig_Offset                           0x0
#define        Reg_Ffc_Ubuf_3dconfig_Offset                               0x1
#define        Reg_Descriptor_Heap_Base_Offset                            0x2
#define        Reg_Wls_Reserved0_Offset                                   0xA
#define        Reg_Wls_Reserved1_Offset                                   0xB
#define        Reg_Wls_Reserved2_Offset                                   0xC
#define        Reg_Wls_Reserved3_Offset                                   0xD
#define        Reg_Wls_Reserved4_Offset                                   0xE
#define        Reg_Wls_Reserved5_Offset                                   0xF
#define        Reg_Ffc_Ubuf_Csconfig_Offset                               0x10
#define        Reg_Wls_Reserved6_Offset                                   0x11
#define        Reg_Uav_Group_Offset                                       0x18


typedef enum
{
    FFC_UBUF_3DCONFIG_D3D_U_SHARP_CACHED_DISABLED= 0,


    FFC_UBUF_3DCONFIG_D3D_U_SHARP_CACHED_ENABLED= 1,

} FFC_UBUF_3DCONFIG_D3D_U_SHARP_CACHED;
typedef enum
{
    FFC_UBUF_CSCONFIG_CSL_U_SHARP_CACHED_DISABLED= 0,


    FFC_UBUF_CSCONFIG_CSL_U_SHARP_CACHED_ENABLED= 1,

} FFC_UBUF_CSCONFIG_CSL_U_SHARP_CACHED;
typedef enum
{
    FFC_UBUF_CSCONFIG_CSH_U_SHARP_CACHED_DISABLED= 0,


    FFC_UBUF_CSCONFIG_CSH_U_SHARP_CACHED_ENABLED= 1,

} FFC_UBUF_CSCONFIG_CSH_U_SHARP_CACHED;
typedef enum
{
    UAV_CTL_U_SHARP_EN_DISABLE               = 0,
    UAV_CTL_U_SHARP_EN_ENABLE                = 1,
} UAV_CTL_U_SHARP_EN;
typedef enum
{
    UAV_CTL_RESOURCE_TYPE_1D_BUFFER          = 0,
    UAV_CTL_RESOURCE_TYPE_1D_TEXTURE         = 1,
    UAV_CTL_RESOURCE_TYPE_2D_TEXTURE         = 2,
    UAV_CTL_RESOURCE_TYPE_3D_TEXTURE         = 3,
    UAV_CTL_RESOURCE_TYPE_1D_TEXTURE_ARRAY   = 5,
    UAV_CTL_RESOURCE_TYPE_2D_TEXTURE_ARRAY   = 6,
    UAV_CTL_RESOURCE_TYPE_STRUCTURED_BUFFER  = 9,
    UAV_CTL_RESOURCE_TYPE_UNTYPED            = 11,
} UAV_CTL_RESOURCE_TYPE;
typedef enum
{
    UAV_CTL_UAV_LAYOUT_HORZ_LINEAR                = 0,

    UAV_CTL_UAV_LAYOUT_VERT_TILED                 = 1,

} UAV_CTL_UAV_LAYOUT;
typedef enum
{
    UAV_CTL_OUT_OF_BOUND_MODE_DEFAULT        = 0,
    UAV_CTL_OUT_OF_BOUND_MODE_CLAMP          = 1,

    UAV_CTL_OUT_OF_BOUND_MODE_WRAP           = 2,
} UAV_CTL_OUT_OF_BOUND_MODE;
typedef enum
{
    UAV_CTL_MIPMAP_EN_DISABLE                = 0,
    UAV_CTL_MIPMAP_EN_ENABLE                 = 1,
} UAV_CTL_MIPMAP_EN;
typedef enum
{
    UAV_CTL_L1CACHABLE_DISABLE               = 0,
    UAV_CTL_L1CACHABLE_ENABLE                = 1,
} UAV_CTL_L1CACHABLE;
typedef enum
{
    UAV_CTL_L2CACHABLE_DISABLE               = 0,
    UAV_CTL_L2CACHABLE_ENABLE                = 1,
} UAV_CTL_L2CACHABLE;
typedef enum
{
    UAV_CTL_KKK_INFO_NORMAL_UAV              = 0,
    UAV_CTL_KKK_INFO_KKK_UAV                 = 1,
} UAV_CTL_KKK_INFO;





typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int U_3d_Base                 : 5;



        unsigned int Reserved                  : 27;
    } reg;
} Reg_Ffc_Ubuf_Golbalconfig;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int U_Ps_Base                 : 5;



        unsigned int D3d_U_Sharp_Cached        : 1;
        unsigned int Reserved                  : 26;
    } reg;
} Reg_Ffc_Ubuf_3dconfig;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Heap_Addr                 : 32;
    } reg;
} Reg_Descriptor_Heap_Base;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Wls_Reserved0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Wls_Reserved1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Wls_Reserved2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Wls_Reserved3;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Wls_Reserved4;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Wls_Reserved5;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int U_Csh_Base                : 5;



        unsigned int Csl_U_Sharp_Cached        : 1;
        unsigned int Csh_U_Sharp_Cached        : 1;
        unsigned int Reserved                  : 25;
    } reg;
} Reg_Ffc_Ubuf_Csconfig;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Wls_Reserved6;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Uav_Base                  : 32;
    } reg;
} Reg_Uav_Base;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Width                     : 15;
        unsigned int Height                    : 15;
        unsigned int Reserved                  : 2;
    } reg;
} Reg_Resource_Size;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Bytestride_Or_Depth       : 32;
    } reg;
} Reg_Resource_Stride;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int First_Element             : 32;
    } reg;
} Reg_View_Offset;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Num_Element               : 32;
    } reg;
} Reg_View_Size;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int U_Sharp_En                : 1;
        unsigned int Resource_Type             : 4;
        unsigned int Uav_Layout                : 1;
        unsigned int Out_Of_Bound_Mode         : 2;
        unsigned int Format                    : 9;
        unsigned int Mipmap_En                 : 1;
        unsigned int Lod                       : 4;
        unsigned int L1cachable                : 1;
        unsigned int L2cachable                : 1;
        unsigned int Kkk_Info                  : 2;

        unsigned int Range_Type                : 6;
    } reg;
} Reg_Uav_Ctl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Bl_Slot_Idx               : 18;
        unsigned int Reserved                  : 14;
    } reg;
} Reg_Bl_Slot_Idx;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Continuous_Optimize       : 1;

        unsigned int Reserved                  : 31;
    } reg;
} Reg_Wls_Config;

typedef struct _Group_Uav_Group
{
    Reg_Uav_Base                     reg_Uav_Base;
    Reg_Resource_Size                reg_Resource_Size;
    Reg_Resource_Stride              reg_Resource_Stride;
    Reg_View_Offset                  reg_View_Offset;
    Reg_View_Size                    reg_View_Size;
    Reg_Uav_Ctl                      reg_Uav_Ctl;
    Reg_Bl_Slot_Idx                  reg_Bl_Slot_Idx;
    Reg_Wls_Config                   reg_Wls_Config;
} Reg_Uav_Group_Group;

typedef struct _Wls_Fe_regs
{
    Reg_Ffc_Ubuf_Golbalconfig        reg_Ffc_Ubuf_Golbalconfig;
    Reg_Ffc_Ubuf_3dconfig            reg_Ffc_Ubuf_3dconfig;
    Reg_Descriptor_Heap_Base         reg_Descriptor_Heap_Base[8];
    Reg_Wls_Reserved0                reg_Wls_Reserved0;
    Reg_Wls_Reserved1                reg_Wls_Reserved1;
    Reg_Wls_Reserved2                reg_Wls_Reserved2;
    Reg_Wls_Reserved3                reg_Wls_Reserved3;
    Reg_Wls_Reserved4                reg_Wls_Reserved4;
    Reg_Wls_Reserved5                reg_Wls_Reserved5;
    Reg_Ffc_Ubuf_Csconfig            reg_Ffc_Ubuf_Csconfig;
    Reg_Wls_Reserved6                reg_Wls_Reserved6[7];
    Reg_Uav_Group_Group              reg_Uav_Group[128];
} Wls_Fe_regs;

#endif
