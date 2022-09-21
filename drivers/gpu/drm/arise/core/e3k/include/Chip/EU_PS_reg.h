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

#ifndef _EU_PS_REG_H
#define _EU_PS_REG_H


#ifndef        EU_PS_BLOCKBASE_INF
    #define    EU_PS_BLOCKBASE_INF
    #define    BLOCK_EU_PS_VERSION 1
    #define    BLOCK_EU_PS_TIMESTAMP "2018/8/16 16:25:12"
    #define    EU_PS_BLOCK                                                0x9
    #define    EU_PS_REG_START                                            0x0
    #define    EU_PS_REG_END                                              0x900
    #define    EU_PS_REG_LIMIT                                            0x900
#endif


#define        Reg_Ps_Cfg_Offset                                          0x0
#define        Reg_Ps_Ctrl_Offset                                         0x1
#define        Reg_Ps_Out_Cfg_Offset                                      0x2
#define        Reg_Ps_Doutbuf_Cfg_Offset                                  0x3
#define        Reg_Ps_Pm_Cfg_Offset                                       0x4
#define        Reg_Ps_Pm_Id_Offset                                        0x5
#define        Reg_Ps_Pm_Range_Offset                                     0x6
#define        Reg_Ps_Out_Mapping_Offset                                  0x7
#define        Reg_Ps_Out_Fmt_Offset                                      0x8
#define        Reg_Ps_U_Enable_Offset                                     0x9
#define        Reg_Ps_U_Fmt_Offset                                        0xB
#define        Reg_Ps_U_Layout_Offset                                     0x13
#define        Reg_Ps_U_Cfg_Offset                                        0x15
#define        Reg_Ps_Instr0_Offset                                       0x16
#define        Reg_Ps_Instr1_Offset                                       0x17
#define        Reg_Ps_Instr_Range_Offset                                  0x18
#define        Reg_Ps_Cb_Cfg_Offset                                       0x19
#define        Reg_Ps_Rev_8aligned_Offset                                 0x1A
#define        Reg_Ps_Rev_Cb_Offset                                       0x20
#define        Reg_Ps_Cb_Data_Offset                                      0x100


typedef enum
{
    PS_CFG_PS_ON_OFF                         = 0,
    PS_CFG_PS_ON_ON                          = 1,
} PS_CFG_PS_ON;
typedef enum
{
    PS_CTRL_AND_V_MASK_DISABLED              = 0,
    PS_CTRL_AND_V_MASK_ENABLED               = 1,

} PS_CTRL_AND_V_MASK;
typedef enum
{
    PS_CTRL_RD_MODE_NEAR                     = 0,
    PS_CTRL_RD_MODE_ZERO                     = 1,
    PS_CTRL_RD_MODE_POS                      = 2,
    PS_CTRL_RD_MODE_NEG                      = 3,
} PS_CTRL_RD_MODE;
typedef enum
{
    PS_CTRL_FP_MODE_STD_IEEE                 = 0,
    PS_CTRL_FP_MODE_NSTD_IEEE                = 1,
    PS_CTRL_FP_MODE_MS_ALT                   = 2,
    PS_CTRL_FP_MODE_RESERVED                 = 3,
} PS_CTRL_FP_MODE;
typedef enum
{
    PS_CTRL_DENORM_EN_DISABLED               = 0,
    PS_CTRL_DENORM_EN_ENABLED                = 1,
} PS_CTRL_DENORM_EN;
typedef enum
{
    PS_CTRL_THREAD_MODE_SIMD32               = 0,
    PS_CTRL_THREAD_MODE_SIMD64               = 1,
} PS_CTRL_THREAD_MODE;
typedef enum
{
    PS_CTRL_CUST_BLEND_EN_DISABLED           = 0,

    PS_CTRL_CUST_BLEND_EN_ENABLED            = 1,

} PS_CTRL_CUST_BLEND_EN;
typedef enum
{
    PS_CTRL_ROV_EN_DISABLED                  = 0,
    PS_CTRL_ROV_EN_ENABLED                   = 1,
} PS_CTRL_ROV_EN;
typedef enum
{
    PS_OUT_CFG_CLR_OUT_EN_OFF                = 0,
    PS_OUT_CFG_CLR_OUT_EN_ON                 = 1,
} PS_OUT_CFG_CLR_OUT_EN;
typedef enum
{
    PS_OUT_CFG_Z_OUT_EN_OFF                  = 0,
    PS_OUT_CFG_Z_OUT_EN_ON                   = 1,
} PS_OUT_CFG_Z_OUT_EN;
typedef enum
{
    PS_OUT_CFG_ALPHA_OUT_EN_OFF              = 0,
    PS_OUT_CFG_ALPHA_OUT_EN_ON               = 1,
} PS_OUT_CFG_ALPHA_OUT_EN;
typedef enum
{
    PS_OUT_CFG_STENCIL_OUT_EN_OFF            = 0,
    PS_OUT_CFG_STENCIL_OUT_EN_ON             = 1,
} PS_OUT_CFG_STENCIL_OUT_EN;
typedef enum
{
    PS_OUT_CFG_MASK_OUT_EN_OFF               = 0,
    PS_OUT_CFG_MASK_OUT_EN_ON                = 1,
} PS_OUT_CFG_MASK_OUT_EN;
typedef enum
{
    PS_OUT_CFG_DUAL_SRC_EN_OFF               = 0,
    PS_OUT_CFG_DUAL_SRC_EN_ON                = 1,
} PS_OUT_CFG_DUAL_SRC_EN;
typedef enum
{
    PS_OUT_CFG_Z_DATA_FMT_FP32               = 0,
    PS_OUT_CFG_Z_DATA_FMT_UNORM24            = 6,
    PS_OUT_CFG_Z_DATA_FMT_UNORM16            = 7,
} PS_OUT_CFG_Z_DATA_FMT;
typedef enum
{
    PS_OUT_CFG_DUAL_SRC_FMT_FP16             = 1,
    PS_OUT_CFG_DUAL_SRC_FMT_UNORM10          = 8,
    PS_OUT_CFG_DUAL_SRC_FMT_UNORM8           = 9,
    PS_OUT_CFG_DUAL_SRC_FMT_SNORM8           = 12,
} PS_OUT_CFG_DUAL_SRC_FMT;
typedef enum
{
    PS_OUT_CFG_MASK_OUT_MODE_BIT1            = 0,
    PS_OUT_CFG_MASK_OUT_MODE_BIT2            = 1,

    PS_OUT_CFG_MASK_OUT_MODE_BIT4            = 2,

    PS_OUT_CFG_MASK_OUT_MODE_BIT8            = 3,

    PS_OUT_CFG_MASK_OUT_MODE_BIT16           = 4,


} PS_OUT_CFG_MASK_OUT_MODE;
typedef enum
{
    PS_DOUTBUF_CFG_ZL3_SIZE_0LINES           = 0,
    PS_DOUTBUF_CFG_ZL3_SIZE_16LINES          = 4,

    PS_DOUTBUF_CFG_ZL3_SIZE_32LINES          = 5,
    PS_DOUTBUF_CFG_ZL3_SIZE_64LINES          = 6,
    PS_DOUTBUF_CFG_ZL3_SIZE_128LINES         = 7,
} PS_DOUTBUF_CFG_ZL3_SIZE;
typedef enum
{
    PS_DOUTBUF_CFG_CLR_SIZE_0LINES           = 0,
    PS_DOUTBUF_CFG_CLR_SIZE_32LINES          = 5,

    PS_DOUTBUF_CFG_CLR_SIZE_64LINES          = 6,
    PS_DOUTBUF_CFG_CLR_SIZE_128LINES         = 7,
} PS_DOUTBUF_CFG_CLR_SIZE;
typedef enum
{
    PS_PM_CFG_PM_EN_DISABLED                 = 0,
    PS_PM_CFG_PM_EN_ENABLED                  = 1,
} PS_PM_CFG_PM_EN;
typedef enum
{
    PS_PM_CFG_TH_PM_SIZE_8KDW                = 0,

    PS_PM_CFG_TH_PM_SIZE_16KDW               = 1,
    PS_PM_CFG_TH_PM_SIZE_32KDW               = 2,
    PS_PM_CFG_TH_PM_SIZE_64KDW               = 3,
    PS_PM_CFG_TH_PM_SIZE_128KDW              = 4,
    PS_PM_CFG_TH_PM_SIZE_256KDW              = 5,
    PS_PM_CFG_TH_PM_SIZE_512KDW              = 6,
    PS_PM_CFG_TH_PM_SIZE_1024KDW             = 7,
    PS_PM_CFG_TH_PM_SIZE_1040KDW             = 8,
} PS_PM_CFG_TH_PM_SIZE;
typedef enum
{
    PS_OUT_MAPPING_O0_EN_DISABLED            = 0,
    PS_OUT_MAPPING_O0_EN_ENABLED             = 1,
} PS_OUT_MAPPING_O0_EN;
typedef enum
{
    PS_OUT_MAPPING_O1_EN_DISABLED            = 0,
    PS_OUT_MAPPING_O1_EN_ENABLED             = 1,
} PS_OUT_MAPPING_O1_EN;
typedef enum
{
    PS_OUT_MAPPING_O2_EN_DISABLED            = 0,
    PS_OUT_MAPPING_O2_EN_ENABLED             = 1,
} PS_OUT_MAPPING_O2_EN;
typedef enum
{
    PS_OUT_MAPPING_O3_EN_DISABLED            = 0,
    PS_OUT_MAPPING_O3_EN_ENABLED             = 1,
} PS_OUT_MAPPING_O3_EN;
typedef enum
{
    PS_OUT_MAPPING_O4_EN_DISABLED            = 0,
    PS_OUT_MAPPING_O4_EN_ENABLED             = 1,
} PS_OUT_MAPPING_O4_EN;
typedef enum
{
    PS_OUT_MAPPING_O5_EN_DISABLED            = 0,
    PS_OUT_MAPPING_O5_EN_ENABLED             = 1,
} PS_OUT_MAPPING_O5_EN;
typedef enum
{
    PS_OUT_MAPPING_O6_EN_DISABLED            = 0,
    PS_OUT_MAPPING_O6_EN_ENABLED             = 1,
} PS_OUT_MAPPING_O6_EN;
typedef enum
{
    PS_OUT_MAPPING_O7_EN_DISABLED            = 0,
    PS_OUT_MAPPING_O7_EN_ENABLED             = 1,
} PS_OUT_MAPPING_O7_EN;
typedef enum
{
    PS_OUT_FMT_O0_DATA_FMT_FP32              = 0,
    PS_OUT_FMT_O0_DATA_FMT_FP16              = 1,
    PS_OUT_FMT_O0_DATA_FMT_SINT32            = 2,
    PS_OUT_FMT_O0_DATA_FMT_SINT16            = 3,
    PS_OUT_FMT_O0_DATA_FMT_UINT32            = 4,
    PS_OUT_FMT_O0_DATA_FMT_UINT16            = 5,
    PS_OUT_FMT_O0_DATA_FMT_UNORM24           = 6,
    PS_OUT_FMT_O0_DATA_FMT_UNORM16           = 7,
    PS_OUT_FMT_O0_DATA_FMT_UNORM10           = 8,
    PS_OUT_FMT_O0_DATA_FMT_UNORM8            = 9,
    PS_OUT_FMT_O0_DATA_FMT_SINT8             = 10,
    PS_OUT_FMT_O0_DATA_FMT_UINT8             = 11,
    PS_OUT_FMT_O0_DATA_FMT_SNORM8            = 12,
    PS_OUT_FMT_O0_DATA_FMT_SNORM16           = 13,
    PS_OUT_FMT_O0_DATA_FMT_UINT64            = 14,
    PS_OUT_FMT_O0_DATA_FMT_RESERVED          = 15,
} PS_OUT_FMT_O0_DATA_FMT;
typedef enum
{
    PS_U_ENABLE_U0_EN_DISABLE                = 0,
    PS_U_ENABLE_U0_EN_ENABLE                 = 1,
} PS_U_ENABLE_U0_EN;
typedef enum
{
    PS_U_FMT_U0_DATA_FMT_FP32                = 0,
    PS_U_FMT_U0_DATA_FMT_FP16                = 1,
    PS_U_FMT_U0_DATA_FMT_SINT32              = 2,
    PS_U_FMT_U0_DATA_FMT_SINT16              = 3,
    PS_U_FMT_U0_DATA_FMT_UINT32              = 4,
    PS_U_FMT_U0_DATA_FMT_UINT16              = 5,
    PS_U_FMT_U0_DATA_FMT_UNORM24             = 6,
    PS_U_FMT_U0_DATA_FMT_UNORM16             = 7,
    PS_U_FMT_U0_DATA_FMT_UNORM10             = 8,
    PS_U_FMT_U0_DATA_FMT_UNORM8              = 9,
    PS_U_FMT_U0_DATA_FMT_SINT8               = 10,
    PS_U_FMT_U0_DATA_FMT_UINT8               = 11,
    PS_U_FMT_U0_DATA_FMT_SNORM8              = 12,
    PS_U_FMT_U0_DATA_FMT_SNORM16             = 13,
    PS_U_FMT_U0_DATA_FMT_UINT64              = 14,
    PS_U_FMT_U0_DATA_FMT_RESERVED            = 15,
} PS_U_FMT_U0_DATA_FMT;
typedef enum
{
    PS_U_LAYOUT_U0_LAYOUT_VERTICAL           = 0,
    PS_U_LAYOUT_U0_LAYOUT_HORIZONTAL         = 1,
} PS_U_LAYOUT_U0_LAYOUT;





typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Ps_On                     : 1;
        unsigned int Max_Threads               : 8;


        unsigned int In_Size                   : 6;

        unsigned int Reserved1                 : 17;
    } reg;
} Reg_Ps_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Crf_Size                  : 6;


        unsigned int And_V_Mask                : 1;
        unsigned int Rd_Mode                   : 2;
        unsigned int Fp_Mode                   : 2;
        unsigned int Denorm_En                 : 1;
        unsigned int Thread_Mode               : 1;

        unsigned int Cust_Blend_En             : 1;

        unsigned int Rov_En                    : 1;

        unsigned int Cust_Crf_Addr             : 6;


        unsigned int Reserved                  : 11;
    } reg;
} Reg_Ps_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Clr_Out_En                : 1;
        unsigned int Z_Out_En                  : 1;

        unsigned int Alpha_Out_En              : 1;

        unsigned int Stencil_Out_En            : 1;

        unsigned int Mask_Out_En               : 1;
        unsigned int Dual_Src_En               : 1;



        unsigned int Clr_Out_Num               : 4;

        unsigned int Z_Data_Fmt                : 4;
        unsigned int Dual_Src_Fmt              : 4;

        unsigned int Mask_Out_Mode             : 3;

        unsigned int Reserved                  : 11;
    } reg;
} Reg_Ps_Out_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Zl3_Size                  : 4;

        unsigned int Clr_Size                  : 4;
        unsigned int Reserved                  : 24;
    } reg;
} Reg_Ps_Doutbuf_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Pm_En                     : 1;
        unsigned int Th_Pm_Size                : 4;


        unsigned int Reserved                  : 27;
    } reg;
} Reg_Ps_Pm_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Pm_U_Id                   : 32;



    } reg;
} Reg_Ps_Pm_Id;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Ps_Idx_Ele_Range          : 14;

        unsigned int Reserved                  : 18;
    } reg;
} Reg_Ps_Pm_Range;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int O0_En                     : 1;
        unsigned int O0_Addr                   : 3;
        unsigned int O1_En                     : 1;
        unsigned int O1_Addr                   : 3;
        unsigned int O2_En                     : 1;
        unsigned int O2_Addr                   : 3;
        unsigned int O3_En                     : 1;
        unsigned int O3_Addr                   : 3;
        unsigned int O4_En                     : 1;
        unsigned int O4_Addr                   : 3;
        unsigned int O5_En                     : 1;
        unsigned int O5_Addr                   : 3;
        unsigned int O6_En                     : 1;
        unsigned int O6_Addr                   : 3;
        unsigned int O7_En                     : 1;
        unsigned int O7_Addr                   : 3;
    } reg;
} Reg_Ps_Out_Mapping;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int O0_Data_Fmt               : 4;
        unsigned int O1_Data_Fmt               : 4;
        unsigned int O2_Data_Fmt               : 4;
        unsigned int O3_Data_Fmt               : 4;
        unsigned int O4_Data_Fmt               : 4;
        unsigned int O5_Data_Fmt               : 4;
        unsigned int O6_Data_Fmt               : 4;
        unsigned int O7_Data_Fmt               : 4;
    } reg;
} Reg_Ps_Out_Fmt;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int U0_En                     : 1;
        unsigned int U1_En                     : 1;
        unsigned int U2_En                     : 1;
        unsigned int U3_En                     : 1;
        unsigned int U4_En                     : 1;
        unsigned int U5_En                     : 1;
        unsigned int U6_En                     : 1;
        unsigned int U7_En                     : 1;
        unsigned int U8_En                     : 1;
        unsigned int U9_En                     : 1;
        unsigned int U10_En                    : 1;
        unsigned int U11_En                    : 1;
        unsigned int U12_En                    : 1;
        unsigned int U13_En                    : 1;
        unsigned int U14_En                    : 1;
        unsigned int U15_En                    : 1;
        unsigned int U16_En                    : 1;
        unsigned int U17_En                    : 1;
        unsigned int U18_En                    : 1;
        unsigned int U19_En                    : 1;
        unsigned int U20_En                    : 1;
        unsigned int U21_En                    : 1;
        unsigned int U22_En                    : 1;
        unsigned int U23_En                    : 1;
        unsigned int U24_En                    : 1;
        unsigned int U25_En                    : 1;
        unsigned int U26_En                    : 1;
        unsigned int U27_En                    : 1;
        unsigned int U28_En                    : 1;
        unsigned int U29_En                    : 1;
        unsigned int U30_En                    : 1;
        unsigned int U31_En                    : 1;
    } reg;
} Reg_Ps_U_Enable;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int U0_Data_Fmt               : 4;

        unsigned int U1_Data_Fmt               : 4;
        unsigned int U2_Data_Fmt               : 4;
        unsigned int U3_Data_Fmt               : 4;
        unsigned int U4_Data_Fmt               : 4;
        unsigned int U5_Data_Fmt               : 4;
        unsigned int U6_Data_Fmt               : 4;
        unsigned int U7_Data_Fmt               : 4;
    } reg;
} Reg_Ps_U_Fmt;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int U0_Layout                 : 1;
        unsigned int U1_Layout                 : 1;
        unsigned int U2_Layout                 : 1;
        unsigned int U3_Layout                 : 1;
        unsigned int U4_Layout                 : 1;
        unsigned int U5_Layout                 : 1;
        unsigned int U6_Layout                 : 1;
        unsigned int U7_Layout                 : 1;
        unsigned int U8_Layout                 : 1;
        unsigned int U9_Layout                 : 1;
        unsigned int U10_Layout                : 1;
        unsigned int U11_Layout                : 1;
        unsigned int U12_Layout                : 1;
        unsigned int U13_Layout                : 1;
        unsigned int U14_Layout                : 1;
        unsigned int U15_Layout                : 1;
        unsigned int U16_Layout                : 1;
        unsigned int U17_Layout                : 1;
        unsigned int U18_Layout                : 1;
        unsigned int U19_Layout                : 1;
        unsigned int U20_Layout                : 1;
        unsigned int U21_Layout                : 1;
        unsigned int U22_Layout                : 1;
        unsigned int U23_Layout                : 1;
        unsigned int U24_Layout                : 1;
        unsigned int U25_Layout                : 1;
        unsigned int U26_Layout                : 1;
        unsigned int U27_Layout                : 1;
        unsigned int U28_Layout                : 1;
        unsigned int U29_Layout                : 1;
        unsigned int U30_Layout                : 1;
        unsigned int U31_Layout                : 1;
    } reg;
} Reg_Ps_U_Layout;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Ps_U_Range                : 8;

        unsigned int Reserved                  : 24;
    } reg;
} Reg_Ps_U_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Address_Low               : 32;

    } reg;
} Reg_Ps_Instr0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Address_High              : 8;

        unsigned int Reserved                  : 24;
    } reg;
} Reg_Ps_Instr1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Ins_Range                 : 16;

        unsigned int Reserved                  : 16;
    } reg;
} Reg_Ps_Instr_Range;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Ps_Cb_Range               : 9;

        unsigned int Reserved                  : 23;
    } reg;
} Reg_Ps_Cb_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Ps_Rev_8aligned;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Ps_Rev_Cb;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Const                     : 32;
    } reg;
} Reg_Ps_Cb_Data;

typedef struct _Eu_Ps_regs
{
    Reg_Ps_Cfg                       reg_Ps_Cfg;
    Reg_Ps_Ctrl                      reg_Ps_Ctrl;
    Reg_Ps_Out_Cfg                   reg_Ps_Out_Cfg;
    Reg_Ps_Doutbuf_Cfg               reg_Ps_Doutbuf_Cfg;
    Reg_Ps_Pm_Cfg                    reg_Ps_Pm_Cfg;
    Reg_Ps_Pm_Id                     reg_Ps_Pm_Id;
    Reg_Ps_Pm_Range                  reg_Ps_Pm_Range;
    Reg_Ps_Out_Mapping               reg_Ps_Out_Mapping;
    Reg_Ps_Out_Fmt                   reg_Ps_Out_Fmt;
    Reg_Ps_U_Enable                  reg_Ps_U_Enable[2];
    Reg_Ps_U_Fmt                     reg_Ps_U_Fmt[8];
    Reg_Ps_U_Layout                  reg_Ps_U_Layout[2];
    Reg_Ps_U_Cfg                     reg_Ps_U_Cfg;
    Reg_Ps_Instr0                    reg_Ps_Instr0;
    Reg_Ps_Instr1                    reg_Ps_Instr1;
    Reg_Ps_Instr_Range               reg_Ps_Instr_Range;
    Reg_Ps_Cb_Cfg                    reg_Ps_Cb_Cfg;
    Reg_Ps_Rev_8aligned              reg_Ps_Rev_8aligned[6];
    Reg_Ps_Rev_Cb                    reg_Ps_Rev_Cb[224];
    Reg_Ps_Cb_Data                   reg_Ps_Cb_Data[2048];
} Eu_Ps_regs;

#endif
