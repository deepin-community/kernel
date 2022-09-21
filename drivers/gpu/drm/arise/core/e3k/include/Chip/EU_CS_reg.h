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

#ifndef _EU_CS_REG_H
#define _EU_CS_REG_H


#ifndef        EU_CS_BLOCKBASE_INF
    #define    EU_CS_BLOCKBASE_INF
    #define    BLOCK_EU_CS_VERSION 1
    #define    BLOCK_EU_CS_TIMESTAMP "2018/8/16 16:25:12"
    #define    EU_CS_BLOCK                                                0xE
    #define    EU_CS_REG_START                                            0x0
    #define    EU_CS_REG_END                                              0x900
    #define    EU_CS_REG_LIMIT                                            0x900
#endif



#define        Reg_Eu_Cs_Glb_Offset                                       0x1
#define        Reg_Cs_Cfg_Offset                                          0x2
#define        Reg_Cs_Sm_Cfg_Offset                                       0x3
#define        Reg_Cs_Pm_Cfg_Offset                                       0x4
#define        Reg_Cs_Pm_Id_Offset                                        0x5
#define        Reg_Cs_Pm_Range_Offset                                     0x6
#define        Reg_Cs_U_Enable_Offset                                     0x7
#define        Reg_Cs_U_Fmt_Offset                                        0x9
#define        Reg_Cs_U_Layout_Offset                                     0x11
#define        Reg_Cs_U_Cfg_Offset                                        0x13
#define        Reg_Cs_Instr0_Offset                                       0x14
#define        Reg_Cs_Instr1_Offset                                       0x15
#define        Reg_Cs_Instr_Range_Offset                                  0x16
#define        Reg_Cs_Cb_Cfg_Offset                                       0x17
#define        Reg_Cs_Rev_8aligned_Offset                                 0x18
#define        Reg_Cs_Rev_Cb_Offset                                       0x20
#define        Reg_Cs_Cb_Data_Offset                                      0x100




































typedef enum
{
    CS_CFG_AND_V_MASK_DISABLED               = 0,
    CS_CFG_AND_V_MASK_ENABLED                = 1,

} CS_CFG_AND_V_MASK;
typedef enum
{
    CS_CFG_RD_MODE_NEAR                      = 0,
    CS_CFG_RD_MODE_ZERO                      = 1,
    CS_CFG_RD_MODE_POS                       = 2,
    CS_CFG_RD_MODE_NEG                       = 3,
} CS_CFG_RD_MODE;
typedef enum
{
    CS_CFG_FP_MODE_STD_IEEE                  = 0,
    CS_CFG_FP_MODE_NSTD_IEEE                 = 1,
    CS_CFG_FP_MODE_MS_ALT                    = 2,
    CS_CFG_FP_MODE_RESERVED                  = 3,
} CS_CFG_FP_MODE;
typedef enum
{
    CS_CFG_DENORM_EN_DISABLED                = 0,
    CS_CFG_DENORM_EN_ENABLED                 = 1,
} CS_CFG_DENORM_EN;
typedef enum
{
    CS_CFG_THREAD_MODE_SIMD32                = 0,
    CS_CFG_THREAD_MODE_SIMD64                = 1,
} CS_CFG_THREAD_MODE;
typedef enum
{
    CS_CFG_PATTERN_MODE_FLAT                 = 0,
    CS_CFG_PATTERN_MODE_LINEAR               = 1,
    CS_CFG_PATTERN_MODE_TILE                 = 2,
    CS_CFG_PATTERN_MODE_RESERVED             = 3,
} CS_CFG_PATTERN_MODE;
typedef enum
{
    CS_CFG_U_SHARP_CACHED_DISABLED           = 0,

    CS_CFG_U_SHARP_CACHED_ENABLED            = 1,

} CS_CFG_U_SHARP_CACHED;
typedef enum
{
    CS_SM_CFG_SM_EN_OFF                      = 0,
    CS_SM_CFG_SM_EN_ON                       = 1,

} CS_SM_CFG_SM_EN;
typedef enum
{
    CS_PM_CFG_PM_EN_DISABLED                 = 0,
    CS_PM_CFG_PM_EN_ENABLED                  = 1,
} CS_PM_CFG_PM_EN;
typedef enum
{
    CS_PM_CFG_TH_PM_SIZE_8KDW                = 0,

    CS_PM_CFG_TH_PM_SIZE_16KDW               = 1,
    CS_PM_CFG_TH_PM_SIZE_32KDW               = 2,
    CS_PM_CFG_TH_PM_SIZE_64KDW               = 3,
    CS_PM_CFG_TH_PM_SIZE_128KDW              = 4,
    CS_PM_CFG_TH_PM_SIZE_256KDW              = 5,
    CS_PM_CFG_TH_PM_SIZE_512KDW              = 6,
    CS_PM_CFG_TH_PM_SIZE_1024KDW             = 7,
    CS_PM_CFG_TH_PM_SIZE_1040KDW             = 8,
} CS_PM_CFG_TH_PM_SIZE;
typedef enum
{
    CS_U_ENABLE_U0_EN_DISABLE                = 0,
    CS_U_ENABLE_U0_EN_ENABLE                 = 1,
} CS_U_ENABLE_U0_EN;
typedef enum
{
    CS_U_FMT_U0_DATA_FMT_FP32                = 0,
    CS_U_FMT_U0_DATA_FMT_FP16                = 1,
    CS_U_FMT_U0_DATA_FMT_SINT32              = 2,
    CS_U_FMT_U0_DATA_FMT_SINT16              = 3,
    CS_U_FMT_U0_DATA_FMT_UINT32              = 4,
    CS_U_FMT_U0_DATA_FMT_UINT16              = 5,
    CS_U_FMT_U0_DATA_FMT_UNORM24             = 6,
    CS_U_FMT_U0_DATA_FMT_UNORM16             = 7,
    CS_U_FMT_U0_DATA_FMT_UNORM10             = 8,
    CS_U_FMT_U0_DATA_FMT_UNORM8              = 9,
    CS_U_FMT_U0_DATA_FMT_SINT8               = 10,
    CS_U_FMT_U0_DATA_FMT_UINT8               = 11,
    CS_U_FMT_U0_DATA_FMT_SNORM8              = 12,
    CS_U_FMT_U0_DATA_FMT_SNORM16             = 13,
    CS_U_FMT_U0_DATA_FMT_UINT64              = 14,
    CS_U_FMT_U0_DATA_FMT_RESERVED            = 15,
} CS_U_FMT_U0_DATA_FMT;
typedef enum
{
    CS_U_LAYOUT_U0_LAYOUT_VERTICAL           = 0,
    CS_U_LAYOUT_U0_LAYOUT_HORIZONTAL         = 1,
} CS_U_LAYOUT_U0_LAYOUT;


































typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int U_Csh_Base                : 5;

        unsigned int Cb_Csh_Base               : 6;


        unsigned int Max_Threads               : 8;

        unsigned int Reserved                  : 13;
    } reg;
} Reg_Eu_Cs_Glb;

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

        unsigned int Pattern_Mode              : 3;
        unsigned int Th_Num_In_Group           : 6;


        unsigned int U_Sharp_Cached            : 1;
        unsigned int Reserved                  : 9;
    } reg;
} Reg_Cs_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Sm_En                     : 1;

        unsigned int Group_Sm_Size             : 16;

        unsigned int Reserved                  : 15;
    } reg;
} Reg_Cs_Sm_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Pm_En                     : 1;
        unsigned int Th_Pm_Size                : 4;


        unsigned int Reserved                  : 27;
    } reg;
} Reg_Cs_Pm_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Pm_U_Id                   : 32;



    } reg;
} Reg_Cs_Pm_Id;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Cs_Idx_Ele_Range          : 14;



        unsigned int Reserved                  : 18;
    } reg;
} Reg_Cs_Pm_Range;

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
} Reg_Cs_U_Enable;

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
} Reg_Cs_U_Fmt;

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
} Reg_Cs_U_Layout;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Cs_U_Range                : 8;

        unsigned int Reserved                  : 24;
    } reg;
} Reg_Cs_U_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Address_Low               : 32;

    } reg;
} Reg_Cs_Instr0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Address_High              : 8;

        unsigned int Reserved                  : 24;
    } reg;
} Reg_Cs_Instr1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Ins_Range                 : 16;

        unsigned int Reserved                  : 16;
    } reg;
} Reg_Cs_Instr_Range;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Cs_Cb_Range               : 9;

        unsigned int Reserved                  : 23;
    } reg;
} Reg_Cs_Cb_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Cs_Rev_8aligned;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Cs_Rev_Cb;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Const                     : 32;
    } reg;
} Reg_Cs_Cb_Data;

typedef struct _Eu_Cs_regs
{
    Reg_Eu_Full_Glb                  reg_Eu_Full_Glb;
    Reg_Eu_Cs_Glb                    reg_Eu_Cs_Glb;
    Reg_Cs_Cfg                       reg_Cs_Cfg;
    Reg_Cs_Sm_Cfg                    reg_Cs_Sm_Cfg;
    Reg_Cs_Pm_Cfg                    reg_Cs_Pm_Cfg;
    Reg_Cs_Pm_Id                     reg_Cs_Pm_Id;
    Reg_Cs_Pm_Range                  reg_Cs_Pm_Range;
    Reg_Cs_U_Enable                  reg_Cs_U_Enable[2];
    Reg_Cs_U_Fmt                     reg_Cs_U_Fmt[8];
    Reg_Cs_U_Layout                  reg_Cs_U_Layout[2];
    Reg_Cs_U_Cfg                     reg_Cs_U_Cfg;
    Reg_Cs_Instr0                    reg_Cs_Instr0;
    Reg_Cs_Instr1                    reg_Cs_Instr1;
    Reg_Cs_Instr_Range               reg_Cs_Instr_Range;
    Reg_Cs_Cb_Cfg                    reg_Cs_Cb_Cfg;
    Reg_Cs_Rev_8aligned              reg_Cs_Rev_8aligned[8];
    Reg_Cs_Rev_Cb                    reg_Cs_Rev_Cb[224];
    Reg_Cs_Cb_Data                   reg_Cs_Cb_Data[2048];
} Eu_Cs_regs;

#endif
