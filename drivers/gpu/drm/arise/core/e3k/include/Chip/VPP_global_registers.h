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

#ifndef _VPP_GLOBAL_REGISTERS_H
#define _VPP_GLOBAL_REGISTERS_H


#ifndef        VPP_GLOBAL_BLOCKBASE_INF
    #define    VPP_GLOBAL_BLOCKBASE_INF
    #define    BLOCK_VPP_GLOBAL_VERSION 1
    #define    BLOCK_VPP_GLOBAL_TIMESTAMP "2018/7/3 10:33:13"
    #define    VPP_GLOBAL_BLOCK                                           0x10
    #define    VPP_GLOBAL_REG_START                                       0x0
    #define    VPP_GLOBAL_REG_END                                         0x10D
    #define    VPP_GLOBAL_REG_LIMIT                                       0x10D
#endif


#define        Reg_Src_Base_Offset                                        0x0
#define        Reg_Oth_Base_Offset                                        0x1
#define        Reg_Ref_Base_Offset                                        0x2
#define        Reg_Base_Map_Offset                                        0x3
#define        Reg_Dst_Base_Offset                                        0x4
#define        Reg_Scl_Dst_Base_0_Offset                                  0x5
#define        Reg_Scl_Dst_Base_1_Offset                                  0x6
#define        Reg_Scl_Dst_Height_Offset                                  0x7
#define        Reg_Bot_Base_0_Offset                                      0x8
#define        Reg_Bot_Base_1_Offset                                      0x9
#define        Reg_Bot_Base_2_Offset                                      0xA
#define        Reg_Hdr_Lut_Base_Offset                                    0xB
#define        Reg_Hdr_Ysum_Base_Offset                                   0xC
#define        Reg_Scl_Para_Error_Offset                                  0xD
#define        Reg_Src_Sf_0_Offset                                        0xE
#define        Reg_Src_Sf_1_Offset                                        0xF
#define        Reg_Scl_Dst_Sf_1_Offset                                    0x10
#define        Reg_Scl_Dst_Sf_2_Offset                                    0x11
#define        Reg_Vpp_Ref_Sf_Offset                                      0x12
#define        Reg_Mode_Set_Offset                                        0x13
#define        Reg_Drv_Mp_Set_Offset                                      0x14
#define        Reg_Multipass_Set0_Offset                                  0x15
#define        Reg_Multipass_Set1_Offset                                  0x16
#define        Reg_Clip_L_R_Offset                                        0x17
#define        Reg_Clip_T_B_Offset                                        0x18
#define        Reg_Lbuf_Set0_Offset                                       0x19
#define        Reg_Lbuf_Set0_1_Offset                                     0x1A
#define        Reg_Lbuf_Set0_2_Offset                                     0x1B
#define        Reg_Lbuf_Set0_3_Offset                                     0x1C
#define        Reg_Lbuf_Set1_Offset                                       0x1D
#define        Reg_Lbuf_Set2_Offset                                       0x1E
#define        Reg_Lbuf_Set3_Offset                                       0x1F
#define        Reg_Lbuf_Set4_Offset                                       0x20
#define        Reg_Lbuf_Set5_Offset                                       0x21
#define        Reg_Img_Related_Offset                                     0x22
#define        Reg_Hslc_Set0_Offset                                       0x23
#define        Reg_Hslc_Set1_Offset                                       0x24
#define        Reg_Hslc_Set2_Offset                                       0x25
#define        Reg_Scl_Para_Sd_Offset                                     0x26
#define        Reg_Scl_Para_Ds_Offset                                     0x27
#define        Reg_Scl_0_Offset                                           0x28
#define        Reg_Cscf_00_01_Offset                                      0x29
#define        Reg_Cscf_02_10_Offset                                      0x2A
#define        Reg_Cscf_11_12_Offset                                      0x2B
#define        Reg_Cscf_20_21_Offset                                      0x2C
#define        Reg_Cscf_22_Offset                                         0x2D
#define        Reg_Cscf_Off_00_Offset                                     0x2E
#define        Reg_Cscf_Off_01_Offset                                     0x2F
#define        Reg_Cscf_Off_02_Offset                                     0x30
#define        Reg_Cscb_00_01_Offset                                      0x31
#define        Reg_Cscb_02_10_Offset                                      0x32
#define        Reg_Cscb_11_12_Offset                                      0x33
#define        Reg_Cscb_20_21_Offset                                      0x34
#define        Reg_Cscb_22_Offset                                         0x35
#define        Reg_Cscb_Off_00_Offset                                     0x36
#define        Reg_Cscb_Off_01_Offset                                     0x37
#define        Reg_Cscb_Off_02_Offset                                     0x38
#define        Reg_Ld_Bld_Fixcolor_Offset                                 0x39
#define        Reg_Cpt_0_Offset                                           0x3A
#define        Reg_Cpt_1_Offset                                           0x3B
#define        Reg_Cpt_2_Offset                                           0x3C
#define        Reg_Mif_Arbiter_0_Offset                                   0x3D
#define        Reg_Mif_Arbiter_1_Offset                                   0x3E
#define        Reg_Mif_Arbiter_2_Offset                                   0x3F
#define        Reg_Mif_Slot_Idx_0_Offset                                  0x40
#define        Reg_Mif_Slot_Idx_1_Offset                                  0x41
#define        Reg_Mif_Slot_Idx_2_Offset                                  0x42
#define        Reg_Mif_Slot_Idx_3_Offset                                  0x43
#define        Reg_Mif_Slot_Idx_4_Offset                                  0x44
#define        Reg_Mif_Pid_0_Offset                                       0x45
#define        Reg_Mif_Pid_1_Offset                                       0x46
#define        Reg_Di_0_Offset                                            0x47
#define        Reg_Di_1_Offset                                            0x48
#define        Reg_Di_2_Offset                                            0x49
#define        Reg_Di_3_Offset                                            0x4A
#define        Reg_Di_4_Offset                                            0x4B
#define        Reg_Di_5_Offset                                            0x4C
#define        Reg_Di_6_Offset                                            0x4D
#define        Reg_Di_7_Offset                                            0x4E
#define        Reg_Di_8_Offset                                            0x4F
#define        Reg_Di_9_Offset                                            0x50
#define        Reg_Di_A_Offset                                            0x51
#define        Reg_Di_B_Offset                                            0x52
#define        Reg_Di_C_Offset                                            0x53
#define        Reg_Di_D_Offset                                            0x54
#define        Reg_Di_E_Offset                                            0x55
#define        Reg_Vpp_Control_0_Offset                                   0x56
#define        Reg_Vpp_Control_3_Offset                                   0x57
#define        Reg_Vpp_Control_4_Offset                                   0x58
#define        Reg_Vpp_Control_5_Offset                                   0x59
#define        Reg_Vpp_Control_6_Offset                                   0x5A
#define        Reg_Vpp_Control_9_Offset                                   0x5B
#define        Reg_Vpp_Lbuf_0_Offset                                      0x5C
#define        Reg_Vpp_Lbuf_1_Offset                                      0x5D
#define        Reg_Vpp_Fd_Control_0_Offset                                0x5E
#define        Reg_Vpp_Fd_Control_1_Offset                                0x5F
#define        Reg_Vpp_Fd_Control_2_Offset                                0x60
#define        Reg_Vpp_Fd_Control_3_Offset                                0x61
#define        Reg_Vpp_Fd_Control_4_Offset                                0x62
#define        Reg_Vpp_Fd_Control_5_Offset                                0x63
#define        Reg_Vpp_Fd_Control_6_Offset                                0x64
#define        Reg_Vpp_Fd_Control_7_Offset                                0x65
#define        Reg_Cg_Ctrl_Offset                                         0x66
#define        Reg_From_Opcode_Offset                                     0x67
#define        Reg_Cm_0_Offset                                            0x68
#define        Reg_Cm_1_Offset                                            0x69
#define        Reg_Ccm_0_Offset                                           0x92
#define        Reg_Ccm_1_Offset                                           0x93
#define        Reg_Ccm_3_Offset                                           0x94
#define        Reg_Ccm_4_Offset                                           0xA4
#define        Reg_Ccm_5_Offset                                           0xB4
#define        Reg_Ccm_6_Offset                                           0xB5
#define        Reg_Ccm_7_Offset                                           0xC5
#define        Reg_Ccm_8_Offset                                           0xC6
#define        Reg_Ccm_9_Offset                                           0xC7
#define        Reg_Ccm_A_Offset                                           0xC8
#define        Reg_Ccm_B_Offset                                           0xC9
#define        Reg_Ccm_C_Offset                                           0xD9
#define        Reg_Ccm_D_Offset                                           0xDA
#define        Reg_Cc_0_Offset                                            0xDB
#define        Reg_Cc_1_Offset                                            0xDC
#define        Reg_Cc_2_Offset                                            0xDD
#define        Reg_Cc_3_Offset                                            0xDE
#define        Reg_Cc_4_Offset                                            0xDF
#define        Reg_Hadj_0_Offset                                          0xE0
#define        Reg_Hadj_1_Offset                                          0xE1
#define        Reg_Hadj_2_Offset                                          0xE2
#define        Reg_Hadj_3_Offset                                          0xE3
#define        Reg_Sadj_0_Offset                                          0xE4
#define        Reg_Sadj_1_Offset                                          0xE5
#define        Reg_Sadj_2_Offset                                          0xE6
#define        Reg_Sadj_3_Offset                                          0xE7
#define        Reg_Sadj_4_Offset                                          0xE8
#define        Reg_Sadj_5_Offset                                          0xE9
#define        Reg_Sadj_6_Offset                                          0xEA
#define        Reg_Sadj_7_Offset                                          0xEB
#define        Reg_Sadj_8_Offset                                          0xEC
#define        Reg_Hdr_0_Offset                                           0xED
#define        Reg_Hdr_1_Offset                                           0xEE
#define        Reg_Hdr_2_Offset                                           0xEF
#define        Reg_Hdr_3_Offset                                           0xF0
#define        Reg_Hdr_4_Offset                                           0xF1
#define        Reg_Hdr_5_Offset                                           0xF2
#define        Reg_Hdr_6_Offset                                           0xF3
#define        Reg_Hdr_7_Offset                                           0xF4
#define        Reg_Hdr_8_Offset                                           0xF5
#define        Reg_Hdr_9_Offset                                           0xF6
#define        Reg_Hdr_Ylut_0_Offset                                      0xF7
#define        Reg_Hdr_Ylut_1_Offset                                      0xF8
#define        Reg_Hdr_Ylut_2_Offset                                      0xF9
#define        Reg_Hdr_Ylut_3_Offset                                      0xFA
#define        Reg_Hdr_Ylut_4_Offset                                      0xFB
#define        Reg_Hdr_Ylut_5_Offset                                      0xFC
#define        Reg_Hdr_Ylut_6_Offset                                      0xFD
#define        Reg_Hdr_Ylut_7_Offset                                      0xFE
#define        Reg_Hdr_Ylut_8_Offset                                      0xFF
#define        Reg_Hdr_Ylut_9_Offset                                      0x100
#define        Reg_Hdr_Ylut_A_Offset                                      0x101
#define        Reg_Hdr_Ylut_10_Offset                                     0x102
#define        Reg_Hdr_Ylut_11_Offset                                     0x103
#define        Reg_Hdr_Ylut_12_Offset                                     0x104
#define        Reg_Hdr_Ylut_13_Offset                                     0x105
#define        Reg_Hdr_Ylut_14_Offset                                     0x106
#define        Reg_Hdr_Ylut_15_Offset                                     0x107
#define        Reg_Hdr_Ylut_16_Offset                                     0x108
#define        Reg_Hdr_Ylut_17_Offset                                     0x109
#define        Reg_Hdr_Ylut_18_Offset                                     0x10A
#define        Reg_Hdr_Ylut_19_Offset                                     0x10B
#define        Reg_Hdr_Ylut_1a_Offset                                     0x10C


typedef enum
{
    SRC_SF_1_GB_SRC_FMT_RESERVED_BAYER       = 0,

    SRC_SF_1_GB_SRC_FMT_RGB565               = 1,
    SRC_SF_1_GB_SRC_FMT_RGBA8888             = 2,

    SRC_SF_1_GB_SRC_FMT_BGRA8888             = 3,

    SRC_SF_1_GB_SRC_FMT_AYUV8888             = 4,

    SRC_SF_1_GB_SRC_FMT_YUYV                 = 5,


    SRC_SF_1_GB_SRC_FMT_UYVY                 = 6,

    SRC_SF_1_GB_SRC_FMT_NV12                 = 7,
    SRC_SF_1_GB_SRC_FMT_NV21                 = 8,
    SRC_SF_1_GB_SRC_FMT_YV12                 = 9,

    SRC_SF_1_GB_SRC_FMT_ARGB2101010          = 10,

    SRC_SF_1_GB_SRC_FMT_ABGR2101010          = 11,

    SRC_SF_1_GB_SRC_FMT_UYVA1010102          = 12,

    SRC_SF_1_GB_SRC_FMT_RESERVED_Y210        = 13,

    SRC_SF_1_GB_SRC_FMT_P010                 = 14,

} SRC_SF_1_GB_SRC_FMT;
typedef enum
{
    SRC_SF_1_GB_SRC_TILE_LINEAR              = 0,
    SRC_SF_1_GB_SRC_TILE_TILE                = 1,
} SRC_SF_1_GB_SRC_TILE;
typedef enum
{
    SCL_DST_SF_1_GB_SCL_DST_FMT_RESERVED_BAYER= 0,

    SCL_DST_SF_1_GB_SCL_DST_FMT_RGB565       = 1,
    SCL_DST_SF_1_GB_SCL_DST_FMT_RGBA8888     = 2,

    SCL_DST_SF_1_GB_SCL_DST_FMT_BGRA8888     = 3,

    SCL_DST_SF_1_GB_SCL_DST_FMT_AYUV8888     = 4,

    SCL_DST_SF_1_GB_SCL_DST_FMT_YUYV         = 5,


    SCL_DST_SF_1_GB_SCL_DST_FMT_UYVY         = 6,

    SCL_DST_SF_1_GB_SCL_DST_FMT_NV12         = 7,
    SCL_DST_SF_1_GB_SCL_DST_FMT_NV21         = 8,
    SCL_DST_SF_1_GB_SCL_DST_FMT_YV12         = 9,

    SCL_DST_SF_1_GB_SCL_DST_FMT_ARGB2101010  = 10,

    SCL_DST_SF_1_GB_SCL_DST_FMT_ABGR2101010  = 11,

    SCL_DST_SF_1_GB_SCL_DST_FMT_UYVA1010102  = 12,

    SCL_DST_SF_1_GB_SCL_DST_FMT_RESERVED_Y210= 13,

    SCL_DST_SF_1_GB_SCL_DST_FMT_P010         = 14,

} SCL_DST_SF_1_GB_SCL_DST_FMT;
typedef enum
{
    SCL_DST_SF_1_GB_SCL_DST_COMPRESS_EN_DISABLE= 0,
    SCL_DST_SF_1_GB_SCL_DST_COMPRESS_EN_ENABLE= 1,
} SCL_DST_SF_1_GB_SCL_DST_COMPRESS_EN;
typedef enum
{
    MODE_SET_GB_BLT_MODE_DISABLE             = 0,
    MODE_SET_GB_BLT_MODE_ENABLE              = 1,
} MODE_SET_GB_BLT_MODE;
typedef enum
{
    MODE_SET_GB_SCL_MODE_BILINEAR            = 0,
    MODE_SET_GB_SCL_MODE_BICUBIC             = 1,
} MODE_SET_GB_SCL_MODE;
typedef enum
{
    MODE_SET_GB_EN_HSCL_DISABLE              = 0,
    MODE_SET_GB_EN_HSCL_UPSCALING            = 2,
    MODE_SET_GB_EN_HSCL_DOWNSCALING          = 3,
} MODE_SET_GB_EN_HSCL;
typedef enum
{
    MODE_SET_GB_EN_VSCL_DISABLE              = 0,
    MODE_SET_GB_EN_VSCL_UPSCALING            = 2,
    MODE_SET_GB_EN_VSCL_DOWNSCALING          = 3,
} MODE_SET_GB_EN_VSCL;
typedef enum
{
    MODE_SET_GB_ROTATION_MODE_DEGREE0        = 0,
    MODE_SET_GB_ROTATION_MODE_DEGREE90       = 1,
    MODE_SET_GB_ROTATION_MODE_DEGREE180      = 2,
    MODE_SET_GB_ROTATION_MODE_DEGREE270      = 3,
} MODE_SET_GB_ROTATION_MODE;
typedef enum
{
    MODE_SET_GB_X_UPSAMPLE_MODE_INT          = 0,
    MODE_SET_GB_X_UPSAMPLE_MODE_COPY         = 1,
} MODE_SET_GB_X_UPSAMPLE_MODE;
typedef enum
{
    MODE_SET_GB_EN_4K_MEM_SWZL_DISABLE       = 0,
    MODE_SET_GB_EN_4K_MEM_SWZL_ENABLE        = 1,
} MODE_SET_GB_EN_4K_MEM_SWZL;
typedef enum
{
    MODE_SET_GB_Y_UPSAMPLE_MODE_INT          = 0,
    MODE_SET_GB_Y_UPSAMPLE_MODE_COPY         = 1,
} MODE_SET_GB_Y_UPSAMPLE_MODE;
typedef enum
{
    DRV_MP_SET_GB_SEQ_ID_PEAKLEVEL_LEVEL0    = 0,
    DRV_MP_SET_GB_SEQ_ID_PEAKLEVEL_LEVEL1    = 1,
    DRV_MP_SET_GB_SEQ_ID_PEAKLEVEL_LEVEL2    = 2,
    DRV_MP_SET_GB_SEQ_ID_PEAKLEVEL_LEVEL3    = 3,
} DRV_MP_SET_GB_SEQ_ID_PEAKLEVEL;
typedef enum
{
    MULTIPASS_SET1_GB_SIGNAT_SEL_IO_DISABLED= 0,
    MULTIPASS_SET1_GB_SIGNAT_SEL_IO_LD       = 1,
    MULTIPASS_SET1_GB_SIGNAT_SEL_IO_RESERVED_2= 2,
    MULTIPASS_SET1_GB_SIGNAT_SEL_IO_MOB_SCL_SF= 3,
} MULTIPASS_SET1_GB_SIGNAT_SEL_IO;
typedef enum
{
    MULTIPASS_SET1_GB_SIGNAT_SEL_OO_DISABLED= 0,
    MULTIPASS_SET1_GB_SIGNAT_SEL_OO_LD       = 1,
    MULTIPASS_SET1_GB_SIGNAT_SEL_OO_SCL_SF   = 2,
} MULTIPASS_SET1_GB_SIGNAT_SEL_OO;
typedef enum
{
    MULTIPASS_SET1_GB_SIG_ENADDR_IO_DISABLE  = 0,
    MULTIPASS_SET1_GB_SIG_ENADDR_IO_ENABLE   = 1,
} MULTIPASS_SET1_GB_SIG_ENADDR_IO;
typedef enum
{
    LBUF_SET0_GB_VPPARBMODE_FIX              = 0,
    LBUF_SET0_GB_VPPARBMODE_RESERVED         = 1,
    LBUF_SET0_GB_VPPARBMODE_ROUND_ROBIN      = 2,
    LBUF_SET0_GB_VPPARBMODE_LRU              = 3,
} LBUF_SET0_GB_VPPARBMODE;
typedef enum
{
    MIF_ARBITER_1_GB_MIFARB_PASSIVE_COMPRESSION_MODE= 0,

    MIF_ARBITER_1_GB_MIFARB_PASSIVE_PASSIVE_MODE= 1,

} MIF_ARBITER_1_GB_MIFARB_PASSIVE;
typedef enum
{
    VPP_FD_CONTROL_2_GB_SCENECHANGE_MD_MODE_TWO_IN= 0,

    VPP_FD_CONTROL_2_GB_SCENECHANGE_MD_MODE_THREE_IN= 1,

} VPP_FD_CONTROL_2_GB_SCENECHANGE_MD_MODE;





typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Src_Sf_Base            : 32;
    } reg;
} Reg_Src_Base;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Oth_Sf_Base            : 32;
    } reg;
} Reg_Oth_Base;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Ref_Sf_Base            : 32;
    } reg;
} Reg_Ref_Base;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Src_Range_Map_Y_En     : 1;
        unsigned int Gb_Src_Range_Map_Y        : 3;
        unsigned int Gb_Src_Range_Map_Uv_En    : 1;
        unsigned int Gb_Src_Range_Map_Uv       : 3;
        unsigned int Gb_Oth_Range_Map_Y_En     : 1;
        unsigned int Gb_Oth_Range_Map_Y        : 3;
        unsigned int Gb_Oth_Range_Map_Uv_En    : 1;
        unsigned int Gb_Oth_Range_Map_Uv       : 3;
        unsigned int Gb_Ref_Range_Map_Y_En     : 1;
        unsigned int Gb_Ref_Range_Map_Y        : 3;
        unsigned int Gb_Ref_Range_Map_Uv_En    : 1;
        unsigned int Gb_Ref_Range_Map_Uv       : 3;
        unsigned int Reserved_Bm               : 8;
    } reg;
} Reg_Base_Map;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Dst_Sf_Base            : 32;
    } reg;
} Reg_Dst_Base;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Scl_Dst_Sf_Base        : 32;
    } reg;
} Reg_Scl_Dst_Base_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Scl_Dst_Clipx          : 13;
        unsigned int Gb_Scl_Dst_Clipy          : 13;
        unsigned int Reserved_Mob              : 6;
    } reg;
} Reg_Scl_Dst_Base_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Scl_Dst_Sf_Height      : 14;
        unsigned int Reserved_Sdb2             : 18;
    } reg;
} Reg_Scl_Dst_Height;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Src_Sf_Bot_Base        : 32;

    } reg;
} Reg_Bot_Base_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Oth_Sf_Bot_Base        : 32;

    } reg;
} Reg_Bot_Base_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Ref_Sf_Bot_Base        : 32;

    } reg;
} Reg_Bot_Base_2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Int_Base           : 32;
    } reg;
} Reg_Hdr_Lut_Base;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Ysum_Mem_Base          : 32;

    } reg;
} Reg_Hdr_Ysum_Base;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hs_Error               : 15;


        unsigned int Gb_Vs_Error               : 15;


        unsigned int Gb_Ysum_Mem_Base          : 2;

    } reg;
} Reg_Scl_Para_Error;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Src_Width              : 14;
        unsigned int Gb_Seq_Id_Thrd            : 8;

        unsigned int Reserved_Src_Sf_0         : 10;
    } reg;
} Reg_Src_Sf_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved_Src_Sf_1         : 2;
        unsigned int Gb_Src_Fmt                : 4;
        unsigned int Gb_Src_Tile               : 1;
        unsigned int Gb_Src_Height             : 14;
        unsigned int Gb_Src_Pitch              : 11;
    } reg;
} Reg_Src_Sf_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Scl_Dst_Fmt            : 4;
        unsigned int Gb_Scl_Dst_Height         : 14;




        unsigned int Gb_Scl_Dst_Pitch          : 11;
        unsigned int Gb_Scl_Dst_Compress_En    : 1;
        unsigned int Reserved_Scl_Dst_Sf_1     : 2;
    } reg;
} Reg_Scl_Dst_Sf_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Scl_Dst_Width          : 14;




        unsigned int Reserve_Dst2              : 18;
    } reg;
} Reg_Scl_Dst_Sf_2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved_Rs               : 8;
        unsigned int Gb_Vpp_Dnref_Sf_Pitch     : 11;
        unsigned int Gb_Vpp_Dnref_Sf_Height    : 13;
    } reg;
} Reg_Vpp_Ref_Sf;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Blt_Mode               : 1;
        unsigned int Gb_Scl_Mode               : 1;
        unsigned int Gb_En_Hscl                : 2;
        unsigned int Gb_En_Vscl                : 2;
        unsigned int Reserved_Gb_First_Slice   : 2;
        unsigned int Gb_Se_Vpp                 : 1;
        unsigned int Gb_Rotation_Mode          : 2;
        unsigned int Gb_Src_Compress           : 1;


        unsigned int Gb_For_Eco                : 2;
        unsigned int Gb_X_Upsample_Mode        : 1;
        unsigned int Gb_Ds_24_X                : 2;
        unsigned int Gb_Ds_24_Y                : 2;
        unsigned int Gb_Blt_Simple             : 1;
        unsigned int Gb_En_4k_Mem_Swzl         : 1;
        unsigned int Gb_Ds4_Lsb2force          : 1;




        unsigned int Gb_Fixcolor_Mode          : 2;



        unsigned int Gb_Y_Upsample_Mode        : 1;
        unsigned int Reserved_Ms               : 7;
    } reg;
} Reg_Mode_Set;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved_Gb_Scl_Left_Edge_Width : 8;

        unsigned int Reserved_Gb_Scl_Right_Edge_Width : 8;

        unsigned int Gb_Ldrd_Compress          : 1;


        unsigned int Gb_Seq_Id_Peaklevel       : 2;
        unsigned int Reserved_Dms              : 13;
    } reg;
} Reg_Drv_Mp_Set;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Slice_Width            : 14;
        unsigned int Gb_Slice_Pixel_Num        : 14;






        unsigned int Gb_Slice_Width_1st        : 4;
    } reg;
} Reg_Multipass_Set0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Slice_Pixel_Num_1st    : 14;






        unsigned int Gb_Signat_Sel_Io          : 2;
        unsigned int Gb_Signat_Sel_Oo          : 2;
        unsigned int Gb_Slice_Width_1st        : 10;
        unsigned int Gb_Sig_Enaddr_Io          : 1;
        unsigned int Reserved_Mp1              : 3;
    } reg;
} Reg_Multipass_Set1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cl_Left                : 13;

        unsigned int Gb_Cl_Right               : 13;

        unsigned int Reserved17                : 6;
    } reg;
} Reg_Clip_L_R;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cl_Top                 : 13;
        unsigned int Gb_Cl_Bottom              : 13;
        unsigned int Reserved16                : 6;
    } reg;
} Reg_Clip_T_B;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Vppcntout              : 4;

        unsigned int Gb_Vpparbmode             : 2;
        unsigned int Gb_Lb_Scl_Yuv_Offset      : 11;
        unsigned int Gb_Lb_Scl_A_Offset        : 11;
        unsigned int Reserved_Ls0              : 4;
    } reg;
} Reg_Lbuf_Set0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Lb_Scl_Yuv_Pitch       : 10;
        unsigned int Gb_Lb_Scl_A_Pitch         : 10;
        unsigned int Gb_Lb_Nm_Oper_Offset      : 11;
        unsigned int Reserved_Ls01             : 1;
    } reg;
} Reg_Lbuf_Set0_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Lb_Sr_Y_Offset         : 11;
        unsigned int Gb_Lb_Sr_Uv_Offset        : 11;
        unsigned int Gb_Lb_Sr_Y_Pitch          : 10;
    } reg;
} Reg_Lbuf_Set0_2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Lb_Sr_A_Offset         : 11;
        unsigned int Gb_Lb_Sr_Uv_Pitch         : 10;
        unsigned int Gb_Lb_Sr_A_Pitch          : 10;
        unsigned int Reserved_Ls03             : 1;
    } reg;
} Reg_Lbuf_Set0_3;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Lbadiw                 : 6;
        unsigned int Gb_Lbasclyuvw             : 6;
        unsigned int Gb_Lbasclaw               : 6;
        unsigned int Gb_Lbasruvw               : 6;
        unsigned int Gb_Lba_Bank_Mode          : 1;

        unsigned int Reserved_Ls1              : 7;
    } reg;
} Reg_Lbuf_Set1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Lbasryw                : 6;
        unsigned int Gb_Lbasraw                : 6;
        unsigned int Reserved_Ls2              : 20;
    } reg;
} Reg_Lbuf_Set2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Lbadir                 : 8;
        unsigned int Gb_Lbadirefr              : 8;
        unsigned int Gb_Lbascrr                : 8;
        unsigned int Reserved_Ls3              : 8;
    } reg;
} Reg_Lbuf_Set3;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Lbasclyuvr             : 8;
        unsigned int Gb_Lbasclar               : 8;
        unsigned int Gb_Lbasruvr               : 8;
        unsigned int Reserved_Ls4              : 8;
    } reg;
} Reg_Lbuf_Set4;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Lbasryr                : 10;
        unsigned int Gb_Lbasrar                : 10;
        unsigned int Reserved_Ls5              : 12;
    } reg;
} Reg_Lbuf_Set5;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Mob_Fixalpha           : 8;
        unsigned int Gb_Redundant_Edge_Width   : 5;


        unsigned int Gb_Mob_Range_Min          : 1;
        unsigned int Gb_Mob_Range_Maxry        : 2;
        unsigned int Gb_Mob_Range_Maxgbuv      : 2;
        unsigned int Gb_Src_Lossy_Com_En       : 1;

        unsigned int Gb_Dst_Lossy_Com_En       : 1;

        unsigned int Gb_Mob_Blending_En        : 1;
        unsigned int Gb_Mob_Blending_Diu_Mode : 1;
        unsigned int Gb_Mob_Blending_Stream_Mode : 1;
        unsigned int Reserved_Ir               : 9;
    } reg;
} Reg_Img_Related;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hslc_Y_A0              : 12;
        unsigned int Gb_Hslc_Y_B0              : 14;
        unsigned int Reserved_Hslc0            : 6;
    } reg;
} Reg_Hslc_Set0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hslc_Uv_A0             : 15;

        unsigned int Gb_Hslc_Uv_A1             : 15;
        unsigned int Reserved_Hslc1            : 2;
    } reg;
} Reg_Hslc_Set1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hslc_U_B0              : 14;
        unsigned int Gb_Hslc_V_B0              : 14;

        unsigned int Reserved_Hslc2            : 4;
    } reg;
} Reg_Hslc_Set2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Sd_Width_Ratio         : 16;


        unsigned int Gb_Sd_Height_Ratio        : 16;


    } reg;
} Reg_Scl_Para_Sd;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Ds_Width_Ratio         : 11;


        unsigned int Gb_Ds_Height_Ratio        : 11;


        unsigned int Reserved_Ds               : 10;
    } reg;
} Reg_Scl_Para_Ds;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Src_En_Alpha           : 1;
        unsigned int Gb_Scl_Bc_Delta           : 5;
        unsigned int Gb_En_Scl_Maxmin          : 1;

        unsigned int Gb_En_Sr                  : 1;
        unsigned int Gb_Sr_Ctdiffth            : 6;










        unsigned int Gb_En_Pixeltype           : 1;

        unsigned int Gb_Sr_Blendstrength       : 3;
        unsigned int Gb_Sr_Enctblend           : 1;
        unsigned int Gb_Sr_Blendweight         : 4;
        unsigned int Gb_Sr_Coefweight          : 4;
        unsigned int Gb_Sr_Coefweightother     : 4;
        unsigned int Reserved_Scl0             : 1;
    } reg;
} Reg_Scl_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cscf_M_00              : 13;
        unsigned int Gb_Cscf_M_01              : 13;
        unsigned int Gb_Cscf_En                : 1;
        unsigned int Gb_Cscf_Round             : 1;
        unsigned int Reserved_Cscf_00_01       : 4;
    } reg;
} Reg_Cscf_00_01;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cscf_M_02              : 13;
        unsigned int Gb_Cscf_M_10              : 13;
        unsigned int Reserved_Cscf_02_10       : 6;
    } reg;
} Reg_Cscf_02_10;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cscf_M_11              : 13;
        unsigned int Gb_Cscf_M_12              : 13;
        unsigned int Reserved_Cscf_11_12       : 6;
    } reg;
} Reg_Cscf_11_12;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cscf_M_20              : 13;
        unsigned int Gb_Cscf_M_21              : 13;
        unsigned int Reserved_Cscf_20_21       : 6;
    } reg;
} Reg_Cscf_20_21;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cscf_M_22              : 13;
        unsigned int Reserved_Cscf_22          : 19;
    } reg;
} Reg_Cscf_22;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cscf_Off_00            : 20;
        unsigned int Reserved_Cscf_Off_00      : 12;
    } reg;
} Reg_Cscf_Off_00;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cscf_Off_01            : 20;
        unsigned int Reserved_Cscf_Off_01      : 12;
    } reg;
} Reg_Cscf_Off_01;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cscf_Off_02            : 20;
        unsigned int Reserved_Cscf_Off_02      : 12;
    } reg;
} Reg_Cscf_Off_02;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cscb_M_00              : 13;
        unsigned int Gb_Cscb_M_01              : 13;
        unsigned int Gb_Cscb_En                : 1;
        unsigned int Gb_Cscb_Round             : 1;
        unsigned int Reserved_Cscb_00_01       : 4;
    } reg;
} Reg_Cscb_00_01;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cscb_M_02              : 13;
        unsigned int Gb_Cscb_M_10              : 13;
        unsigned int Reserved_Cscb_02_10       : 6;
    } reg;
} Reg_Cscb_02_10;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cscb_M_11              : 13;
        unsigned int Gb_Cscb_M_12              : 13;
        unsigned int Reserved_Cscb_11_12       : 6;
    } reg;
} Reg_Cscb_11_12;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cscb_M_20              : 13;
        unsigned int Gb_Cscb_M_21              : 13;
        unsigned int Reserved_Cscb_20_21       : 6;
    } reg;
} Reg_Cscb_20_21;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cscb_M_22              : 13;
        unsigned int Reserved_Cscb_22          : 19;
    } reg;
} Reg_Cscb_22;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cscb_Off_00            : 20;
        unsigned int Reserved_Cscb_Off_00      : 12;
    } reg;
} Reg_Cscb_Off_00;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cscb_Off_01            : 20;
        unsigned int Reserved_Cscb_Off_01      : 12;
    } reg;
} Reg_Cscb_Off_01;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cscb_Off_02            : 20;
        unsigned int Reserved_Cscb_Off_02      : 12;
    } reg;
} Reg_Cscb_Off_02;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Fixcolor_Value         : 32;

    } reg;
} Reg_Ld_Bld_Fixcolor;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Lb_Cpt_Offset          : 11;
        unsigned int Gb_Lb_Cpt_Size            : 11;
        unsigned int Gb_Lb_Mob_L2t_Pitch       : 10;
    } reg;
} Reg_Cpt_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Dst_End                : 14;

        unsigned int Gb_Dst_Tile               : 1;
        unsigned int Gb_Lb_Mob_L2t_Offset      : 11;
        unsigned int Gb_Mob_Dst_Alphamode      : 2;


        unsigned int Gb_Mob_Blending_Mode      : 2;


        unsigned int Gb_Mob_Blending_Method    : 1;
        unsigned int Gb_Mob_Blending_Rounding : 1;
    } reg;
} Reg_Cpt_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cpt_Dnsample_Mode      : 1;

        unsigned int Gb_Mob_Bldalpha           : 8;
        unsigned int Gb_Cpt_En                 : 1;
        unsigned int Gb_Dst_Sf_Width           : 14;
        unsigned int Reserved_Cpt2             : 8;
    } reg;
} Reg_Cpt_2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Mifarb_Reg             : 8;
        unsigned int Gb_Mifarb_Ydiff           : 8;
        unsigned int Gb_Mifarb_Ld              : 8;
        unsigned int Reserved_Ma0              : 8;
    } reg;
} Reg_Mif_Arbiter_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Mifarb_Cptr            : 8;
        unsigned int Gb_Mifarb_Mtdr            : 8;
        unsigned int Gb_Mifarb_Mob             : 8;
        unsigned int Gb_Mifarb_Passive         : 1;
        unsigned int Reserved_Ma1              : 7;
    } reg;
} Reg_Mif_Arbiter_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Mifarb_Mtdw            : 8;
        unsigned int Gb_Mifarb_Hdrr            : 8;
        unsigned int Gb_Mifarb_Hdrw            : 8;
        unsigned int Reserved_Ma2              : 8;
    } reg;
} Reg_Mif_Arbiter_2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Src_Bl_Slot_Idx        : 18;
        unsigned int Gb_Oth_Bl_Slot_Idx        : 14;
    } reg;
} Reg_Mif_Slot_Idx_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Oth_Bl_Slot_Idx        : 4;
        unsigned int Gb_Ref_Bl_Slot_Idx        : 18;
        unsigned int Gb_Dst_Bl_Slot_Idx        : 10;
    } reg;
} Reg_Mif_Slot_Idx_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Dst_Bl_Slot_Idx        : 8;
        unsigned int Gb_Scl_Dst_Bl_Slot_Idx    : 18;
        unsigned int Reserved_Msi2             : 6;
    } reg;
} Reg_Mif_Slot_Idx_2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Src_Bot_Bl_Slot_Idx    : 18;
        unsigned int Gb_Oth_Bot_Bl_Slot_Idx    : 14;
    } reg;
} Reg_Mif_Slot_Idx_3;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Oth_Bot_Bl_Slot_Idx    : 4;
        unsigned int Gb_Ref_Bot_Bl_Slot_Idx    : 18;
        unsigned int Reserved_Msi4             : 10;
    } reg;
} Reg_Mif_Slot_Idx_4;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Src_Proc_Id            : 4;
        unsigned int Gb_Oth_Proc_Id            : 4;
        unsigned int Gb_Ref_Proc_Id            : 4;
        unsigned int Gb_Src_Bot_Proc_Id        : 4;
        unsigned int Gb_Oth_Bot_Proc_Id        : 4;
        unsigned int Gb_Ref_Bot_Proc_Id        : 4;
        unsigned int Gb_Dst_Proc_Id            : 4;
        unsigned int Gb_Scl_Dst_Proc_Id        : 4;
    } reg;
} Reg_Mif_Pid_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Mtd_Hist_Proc_Id       : 4;
        unsigned int Gb_Hdr_Ysum_Proc_Id       : 4;
        unsigned int Gb_Hdr_Int_Proc_Id        : 4;
        unsigned int Reserved_Mpid1            : 20;
    } reg;
} Reg_Mif_Pid_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Di_Intra_Th            : 6;
        unsigned int Gb_Di_Isfeather_Th        : 7;
        unsigned int Gb_Di_Force_Eela_Th       : 18;

        unsigned int Reserved_Di0              : 1;
    } reg;
} Reg_Di_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Di_Scroll_Th           : 20;

        unsigned int Gb_Point_Degree_Var_K     : 8;


        unsigned int Reserved_Di1              : 4;
    } reg;
} Reg_Di_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Sobel_Th               : 7;

        unsigned int Gb_Sobel_Force_Value      : 9;

        unsigned int Gb_Di_Feather_Edge_Cnt_X1 : 6;


        unsigned int Gb_Di_Feather_Edge_Cnt_Y1 : 4;


        unsigned int Reserved_Di2              : 6;
    } reg;
} Reg_Di_2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Di_Iad_Md_X1           : 16;

        unsigned int Gb_Di_Iad_Md_X2           : 16;

    } reg;
} Reg_Di_3;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Di_Iad_Md_Y1           : 15;

        unsigned int Gb_Di_Iad_Md_K            : 12;

        unsigned int Reserved_Di4              : 5;
    } reg;
} Reg_Di_4;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Point_Degree_Var_X1    : 12;


        unsigned int Gb_Point_Degree_Var_X2    : 12;


        unsigned int Gb_Point_Degree_Var_Y1    : 8;


    } reg;
} Reg_Di_5;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Top_Bot_Field_Div      : 1;

        unsigned int Gb_Di_Enable_Staticflag   : 1;
        unsigned int Gb_Di_Feather_Edge_Cnt_K : 7;


        unsigned int Gb_Di_Feather_Edge_Cnt_X2 : 6;


        unsigned int Gb_Fd_Wait_Frame          : 10;

        unsigned int Reserved_Di6              : 7;
    } reg;
} Reg_Di_6;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Edge_Flag_Th           : 9;

        unsigned int Gb_Di_Intra_Diff_Th       : 20;

        unsigned int Reserved_Di7              : 3;
    } reg;
} Reg_Di_7;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Use_Mv_Judge           : 1;
        unsigned int Gb_Use_Allmv              : 1;

        unsigned int Gb_Mv_Diff_Th             : 5;

        unsigned int Gb_Direction_Judge_Th     : 7;
        unsigned int Gb_Fd_Bad_Merge_Th_1      : 9;

        unsigned int Gb_Fd_Bad_Merge_Th_2      : 4;


        unsigned int Reserved_Di8              : 5;
    } reg;
} Reg_Di_8;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Di_Md_Area_Th0         : 16;

        unsigned int Gb_Di_Md_Area_Th1         : 16;

    } reg;
} Reg_Di_9;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Di_Md_Area_Coef0       : 5;

        unsigned int Gb_Di_Md_Area_Coef1       : 5;
        unsigned int Gb_Di_Md_Adj_Coef0        : 8;

        unsigned int Gb_Di_Md_Adj_Coef1        : 8;

        unsigned int Reserved_Dia              : 6;
    } reg;
} Reg_Di_A;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Di_Iad_Ave_Adj_Th0     : 10;
        unsigned int Gb_Di_Iad_Ave_Adj_Th1     : 10;
        unsigned int Reserved_Dib              : 12;
    } reg;
} Reg_Di_B;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Di_Iad_Ave_Adj_Val     : 14;
        unsigned int Gb_Di_Iad_Ave_Adj_Coef    : 5;
        unsigned int Gb_Di_Mtd_Featherdetect_En : 1;
        unsigned int Reserved_Dic              : 12;
    } reg;
} Reg_Di_C;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Di_Iad_Line_Md_X1      : 16;
        unsigned int Gb_Di_Iad_Line_Md_X2      : 16;
    } reg;
} Reg_Di_D;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Di_Mtd_Featherpara     : 5;
        unsigned int Gb_Di_Iad_Line_Md_Y1      : 15;
        unsigned int Gb_Di_Iad_Line_Md_K       : 12;
    } reg;
} Reg_Di_E;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Di_Th                  : 8;
        unsigned int Reserved_Vc01             : 2;
        unsigned int Gb_Md_Th2                 : 5;
        unsigned int Gb_Md_Th2_En              : 1;
        unsigned int Reserved_Vc0              : 16;
    } reg;
} Reg_Vpp_Control_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Md_Th                  : 20;
        unsigned int Gb_Di_Sr_Rev_Diff_Th      : 10;
        unsigned int Reserved_Vc3              : 2;
    } reg;
} Reg_Vpp_Control_3;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Di_Mtd_Edgevalthr      : 5;
        unsigned int Gb_Di_Mtd_Staticpara      : 5;
        unsigned int Gb_Di_Mtd_Maxminthr       : 3;
        unsigned int Gb_Di_Sr_Grad_Th          : 10;
        unsigned int Gb_Di_Mtd_Md_Th           : 7;
        unsigned int Reserved_Vc4              : 2;
    } reg;
} Reg_Vpp_Control_4;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Di_Sr_Line_Diff_Th     : 10;
        unsigned int Gb_Di_Iad_Md_Ave_Th       : 10;
        unsigned int Reserved_Vc12             : 12;
    } reg;
} Reg_Vpp_Control_5;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Mtd_Col_Hist_Base      : 32;

    } reg;
} Reg_Vpp_Control_6;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Md_Low_Th              : 20;
        unsigned int Reserved_Vc9              : 12;
    } reg;
} Reg_Vpp_Control_9;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Lbuf_Size              : 14;
        unsigned int Gb_Vpp_In_Size            : 11;

        unsigned int Gb_Disable_Lcid_Blend     : 1;
        unsigned int Gb_Fd_Force_Di            : 1;
        unsigned int Gb_Di_Blend_Coef          : 3;
        unsigned int Reserved_Vl0              : 2;
    } reg;
} Reg_Vpp_Lbuf_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Vpp_Di_Offset          : 10;
        unsigned int Gb_Vpp_Di_Size            : 10;


        unsigned int Gb_Vpp_Diref_In_Offset    : 10;

        unsigned int Reserved_Vl1              : 2;
    } reg;
} Reg_Vpp_Lbuf_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Fd_Md_Th               : 8;
        unsigned int Gb_Scene_Change_Top       : 12;
        unsigned int Gb_Scene_Change_Bottom    : 12;
    } reg;
} Reg_Vpp_Fd_Control_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_2v2_Th_Delta           : 8;
        unsigned int Gb_2v2_Th_Ratio           : 4;
        unsigned int Gb_2v2_Sad_Counter_Th     : 5;
        unsigned int Gb_2v2_Md_Failed_Counter_Th : 4;
        unsigned int Gb_Lcid_Diff_Th           : 10;
        unsigned int Gb_2v2_Count_Mode         : 1;


    } reg;
} Reg_Vpp_Fd_Control_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Scene_Change_Th        : 23;
        unsigned int Gb_Scenechange_Md_Mode    : 1;
        unsigned int Gb_Fd_Md_Th2              : 8;
    } reg;
} Reg_Vpp_Fd_Control_2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Fd_Counter_Th          : 4;
        unsigned int Gb_Fd_Detect_Counter_Initial : 4;
        unsigned int Gb_Fd_Strong_Counter_Initial : 4;
        unsigned int Gb_2v2_Failed_Counter_Th : 5;
        unsigned int Gb_2v2_Md_Succeed_Counter_Th : 5;
        unsigned int Gb_2v2_Sad_Succeed_Counter_Th : 5;

        unsigned int Gb_2v2_Sad_Failed_Range   : 5;
    } reg;
} Reg_Vpp_Fd_Control_3;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_2v2_Frame_Cnt_Th       : 9;
        unsigned int Gb_2v2_Failed_Counter_Th2 : 5;
        unsigned int Gb_Fd_2v2_Sad_Clr_Th_1    : 9;

        unsigned int Gb_Fd_2v2_Sad_Clr_Th_2    : 4;

        unsigned int Gb_Fd_2v2_Sad_Clr_Th_3    : 4;

        unsigned int Reserved_Fd4              : 1;
    } reg;
} Reg_Vpp_Fd_Control_4;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Fd_Bad_Copy_Th_1       : 7;
        unsigned int Gb_Fd_Bad_Copy_Th_2       : 9;
        unsigned int Gb_Fd_Bad_Copy_Th_3       : 3;

        unsigned int Gb_Fd_Bad_Copy_Th_4       : 3;

        unsigned int Gb_Fd_Quit_2v2_Th_1       : 9;
        unsigned int Reserved_Fd5              : 1;
    } reg;
} Reg_Vpp_Fd_Control_5;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Fd_Quit_2v2_Th_2       : 4;

        unsigned int Gb_Fd_Quit_2v2_Th_3       : 7;
        unsigned int Gb_Fd_Quit_2v2_Th_4       : 9;
        unsigned int Gb_Fd_Quit_2v2_Th_5       : 4;

        unsigned int Gb_Fd_2v2_Md_Th_3         : 4;
        unsigned int Gb_Fd_2v2_Md_Th_5         : 4;
    } reg;
} Reg_Vpp_Fd_Control_6;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Fd_2v2_Md_Th_1         : 9;
        unsigned int Gb_Fd_2v2_Md_Th_2         : 11;
        unsigned int Gb_Fd_2v2_Md_Th_4         : 9;
        unsigned int Gb_Fd_Sc_Quit_2v2_Th      : 3;

    } reg;
} Reg_Vpp_Fd_Control_7;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Di_Cg_Off              : 1;
        unsigned int Gb_Scl_Cg_Off             : 1;
        unsigned int Gb_Scl_Bp_Cg_Off          : 1;
        unsigned int Gb_Cscb_Cg_Off            : 1;
        unsigned int Gb_Cscf_Cg_Off            : 1;
        unsigned int Gb_Cg_Busy_Th             : 3;

        unsigned int Gb_Fire_After_Save        : 1;



        unsigned int Gb_Restore_Keep_On        : 1;

        unsigned int Gb_For_Eco2               : 8;
        unsigned int Gb_Cpt_Cg_Off             : 1;
        unsigned int Gb_Sr_Cg_Off              : 1;
        unsigned int Gb_Cm_Cg_Off              : 1;
        unsigned int Gb_Ccm_Cg_Off             : 1;
        unsigned int Gb_Hdr_Cg_Off             : 1;
        unsigned int Reserved_Cg               : 9;
    } reg;
} Reg_Cg_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Di_En                  : 1;
        unsigned int Gb_Fd_En                  : 1;
        unsigned int Gb_Mtd_En                 : 1;
        unsigned int Gb_Two_Frame_Out_En       : 1;

        unsigned int Gb_Di_Force_Lcid          : 1;

        unsigned int Gb_Single_Sel_Oth         : 1;



        unsigned int Gb_Top_Field_First        : 1;


        unsigned int Gb_Reset_Hqvpp            : 1;

        unsigned int Gb_Hqvpp_Mode             : 1;

        unsigned int Gb_Vpp_Fd_Index           : 2;
        unsigned int Reserved_Opcode           : 21;
    } reg;
} Reg_From_Opcode;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cmlut_En               : 1;
        unsigned int Gb_Tint_En                : 1;
        unsigned int Gb_Tint_Hoffset           : 10;
        unsigned int Reserved_Cm0              : 20;
    } reg;
} Reg_Cm_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Cmlut0                 : 11;
        unsigned int Gb_Cmlut1                 : 11;
        unsigned int Reserved_Cm1              : 10;
    } reg;
} Reg_Cm_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Ccm_En                 : 1;


        unsigned int Gb_Metadata_En            : 1;

        unsigned int Gb_Dg_Hcoe                : 20;
        unsigned int Reserved_Ccm0             : 10;
    } reg;
} Reg_Ccm_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Dg_Lcoe                : 20;
        unsigned int Gb_Yuv2rgb_En             : 1;
        unsigned int Gb_Yuv2rgb_Mode           : 2;



        unsigned int Gb_Ccm_Hdr_Inv            : 1;
        unsigned int Reserved_Ccm1             : 8;
    } reg;
} Reg_Ccm_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Dg_Tbl_0               : 16;
        unsigned int Gb_Dg_Tbl_1               : 16;
    } reg;
} Reg_Ccm_3;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_G_Tbl_0                : 10;
        unsigned int Gb_G_Tbl_1                : 10;
        unsigned int Gb_G_Tbl_2                : 10;
        unsigned int Reserved_Ccm5             : 2;
    } reg;
} Reg_Ccm_4;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hlg_En                 : 2;



        unsigned int Gb_Hlg_Acoef              : 16;
        unsigned int Gb_Hlg_Bcoef              : 10;
        unsigned int Reserved_Ccm6             : 4;
    } reg;
} Reg_Ccm_5;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hlg_Tbl_0              : 16;
        unsigned int Gb_Hlg_Tbl_1              : 16;
    } reg;
} Reg_Ccm_6;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hlg_Tbl_20             : 16;
        unsigned int Gb_Hlg_Tbl_21             : 16;
    } reg;
} Reg_Ccm_7;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hlg_Tbl_22             : 16;
        unsigned int Gb_Hlg_Invtbl_22          : 16;
    } reg;
} Reg_Ccm_8;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_1886_Acoef             : 16;
        unsigned int Gb_1886_Bcoef             : 16;
    } reg;
} Reg_Ccm_9;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hlg_Invacoef           : 16;
        unsigned int Gb_Hlg_Invbcoef           : 10;
        unsigned int Gb_1886_En                : 2;



        unsigned int Reserved_Ccm9             : 4;
    } reg;
} Reg_Ccm_A;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hlg_Invtbl_0           : 16;
        unsigned int Gb_Hlg_Invtbl_1           : 16;
    } reg;
} Reg_Ccm_B;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hlg_Invtbl_20          : 16;
        unsigned int Gb_Hlg_Invtbl_21          : 16;
    } reg;
} Reg_Ccm_C;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_1886_Invacoef          : 16;
        unsigned int Gb_1886_Invbcoef          : 16;
    } reg;
} Reg_Ccm_D;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Convert_M_00           : 14;
        unsigned int Gb_Convert_M_01           : 14;
        unsigned int Reserved_Cc0              : 4;
    } reg;
} Reg_Cc_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Convert_M_10           : 14;
        unsigned int Gb_Convert_M_11           : 14;
        unsigned int Reserved_Cc1              : 4;
    } reg;
} Reg_Cc_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Convert_M_20           : 14;
        unsigned int Gb_Convert_M_21           : 14;
        unsigned int Reserved_Cc2              : 4;
    } reg;
} Reg_Cc_2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Convert_M_02           : 14;
        unsigned int Gb_Convert_M_12           : 14;
        unsigned int Reserved_Cc3              : 4;
    } reg;
} Reg_Cc_3;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Convert_M_22           : 14;
        unsigned int Reserved_Cc4              : 18;
    } reg;
} Reg_Cc_4;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hadj_En                : 1;
        unsigned int Gb_Hrange_Gl              : 10;
        unsigned int Gb_Hrange_Gh              : 10;
        unsigned int Reserved_Hadj0            : 11;
    } reg;
} Reg_Hadj_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hrange_Rl              : 10;
        unsigned int Gb_Hrange_Rh              : 10;
        unsigned int Reserved_Hadj1            : 12;
    } reg;
} Reg_Hadj_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_G_Range                : 10;
        unsigned int Gb_G_Axis                 : 10;
        unsigned int Gb_G_Coef                 : 12;
    } reg;
} Reg_Hadj_2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_R_Range                : 10;
        unsigned int Gb_R_Axis                 : 10;
        unsigned int Gb_R_Coef                 : 12;
    } reg;
} Reg_Hadj_3;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Sadj_En                : 1;
        unsigned int Gb_Sadj_Inc00             : 11;
        unsigned int Gb_Sadj_Inc01             : 11;
        unsigned int Reserved_Sadj0            : 9;
    } reg;
} Reg_Sadj_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Sadj_Inc02             : 11;
        unsigned int Gb_Sadj_Inc03             : 11;
        unsigned int Reserved_Sadj1            : 10;
    } reg;
} Reg_Sadj_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Sadj_Inc04             : 11;
        unsigned int Gb_Sadj_Inc05             : 11;
        unsigned int Reserved_Sadj2            : 10;
    } reg;
} Reg_Sadj_2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Sadj_Inc06             : 11;
        unsigned int Gb_Sadj_Inc07             : 11;
        unsigned int Gb_Sadj_Inc14             : 5;
        unsigned int Reserved_Sadj3            : 5;
    } reg;
} Reg_Sadj_3;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Sadj_Inc08             : 11;
        unsigned int Gb_Sadj_Inc09             : 11;
        unsigned int Gb_Sadj_Inc14             : 6;
        unsigned int Reserved_Sadj4            : 4;
    } reg;
} Reg_Sadj_4;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Sadj_Inc0a             : 11;
        unsigned int Gb_Sadj_Inc0b             : 11;
        unsigned int Gb_Sadj_Inc13             : 5;
        unsigned int Reserved_Sadj5            : 5;
    } reg;
} Reg_Sadj_5;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Sadj_Inc0c             : 11;
        unsigned int Gb_Sadj_Inc0d             : 11;
        unsigned int Gb_Sadj_Inc13             : 6;
        unsigned int Reserved_Sadj6            : 4;
    } reg;
} Reg_Sadj_6;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Sadj_Inc0e             : 11;
        unsigned int Gb_Sadj_Inc0f             : 11;
        unsigned int Gb_Sadj_Inc12             : 5;
        unsigned int Reserved_Sadj7            : 5;
    } reg;
} Reg_Sadj_7;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Sadj_Inc10             : 11;
        unsigned int Gb_Sadj_Inc11             : 11;
        unsigned int Gb_Sadj_Inc12             : 6;
        unsigned int Reserved_Sadj8            : 4;
    } reg;
} Reg_Sadj_8;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_En                 : 1;
        unsigned int Gb_Hdr_Curve_En           : 1;
        unsigned int Gb_Hdr_Cliplmt            : 10;
        unsigned int Gb_Hdr_Maxfmean           : 8;
        unsigned int Gb_Hdr_Maxmean            : 10;
        unsigned int Gb_Hdr_Sc_En              : 1;
        unsigned int Gb_Hdr_Smallsize          : 1;
    } reg;
} Reg_Hdr_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Maxpeakcnt         : 8;
        unsigned int Gb_Hdr_Minpeakwg          : 7;
        unsigned int Gb_Hdr_Maxpeakwg          : 7;
        unsigned int Gb_Hdr_Manualclipmode     : 1;
        unsigned int Gb_Hdr_Frmwg              : 7;
        unsigned int Gb_Reset_Hdr              : 1;
        unsigned int Reserved_Hdr1             : 1;
    } reg;
} Reg_Hdr_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Pixcntforstat      : 27;
        unsigned int Gb_Hdr_Shft               : 5;
    } reg;
} Reg_Hdr_2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Minstretchrange    : 10;

        unsigned int Reserved_Hdr3             : 9;
        unsigned int Gb_Hdr_Minfiltspd         : 5;
        unsigned int Gb_Hdr_Binmedsdt          : 8;
    } reg;
} Reg_Hdr_3;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Deltameanth        : 10;
        unsigned int Gb_Hdr_Binmedhimax        : 8;
        unsigned int Gb_Hdr_Yluthiwginit       : 7;
        unsigned int Reserved_Hdr4             : 7;
    } reg;
} Reg_Hdr_4;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Meanchangeth       : 28;
        unsigned int Reserved_Hdr5             : 4;
    } reg;
} Reg_Hdr_5;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Pixlowratio        : 26;
        unsigned int Reserved_Hdr6             : 6;
    } reg;
} Reg_Hdr_6;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Pixhiratio         : 26;
        unsigned int Reserved_Hdr7             : 6;
    } reg;
} Reg_Hdr_7;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Blackratio         : 26;
        unsigned int Reserved_Hdr8             : 6;
    } reg;
} Reg_Hdr_8;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Noiseratio         : 26;
        unsigned int Reserved_Hdr9             : 6;
    } reg;
} Reg_Hdr_9;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Yluthi_00          : 10;
        unsigned int Gb_Hdr_Yluthi_01          : 10;
        unsigned int Gb_Hdr_Yluthi_02          : 10;
        unsigned int Reserved_Hdr_Ylut0        : 2;
    } reg;
} Reg_Hdr_Ylut_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Yluthi_03          : 10;
        unsigned int Gb_Hdr_Yluthi_04          : 10;
        unsigned int Gb_Hdr_Yluthi_05          : 10;
        unsigned int Reserved_Hdr_Ylut1        : 2;
    } reg;
} Reg_Hdr_Ylut_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Yluthi_06          : 10;
        unsigned int Gb_Hdr_Yluthi_07          : 10;
        unsigned int Gb_Hdr_Yluthi_08          : 10;
        unsigned int Reserved_Hdr_Ylut2        : 2;
    } reg;
} Reg_Hdr_Ylut_2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Yluthi_09          : 10;
        unsigned int Gb_Hdr_Yluthi_0a          : 10;
        unsigned int Gb_Hdr_Yluthi_0b          : 10;
        unsigned int Reserved_Hdr_Ylut3        : 2;
    } reg;
} Reg_Hdr_Ylut_3;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Yluthi_0c          : 10;
        unsigned int Gb_Hdr_Yluthi_0d          : 10;
        unsigned int Gb_Hdr_Yluthi_0e          : 10;
        unsigned int Reserved_Hdr_Ylut4        : 2;
    } reg;
} Reg_Hdr_Ylut_4;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Yluthi_0f          : 10;
        unsigned int Gb_Hdr_Yluthi_10          : 10;
        unsigned int Gb_Hdr_Yluthi_11          : 10;
        unsigned int Reserved_Hdr_Ylut5        : 2;
    } reg;
} Reg_Hdr_Ylut_5;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Yluthi_12          : 10;
        unsigned int Gb_Hdr_Yluthi_13          : 10;
        unsigned int Gb_Hdr_Yluthi_14          : 10;
        unsigned int Reserved_Hdr_Ylut6        : 2;
    } reg;
} Reg_Hdr_Ylut_6;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Yluthi_15          : 10;
        unsigned int Gb_Hdr_Yluthi_16          : 10;
        unsigned int Gb_Hdr_Yluthi_17          : 10;
        unsigned int Reserved_Hdr_Ylut7        : 2;
    } reg;
} Reg_Hdr_Ylut_7;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Yluthi_18          : 10;
        unsigned int Gb_Hdr_Yluthi_19          : 10;
        unsigned int Gb_Hdr_Yluthi_1a          : 10;
        unsigned int Reserved_Hdr_Ylut8        : 2;
    } reg;
} Reg_Hdr_Ylut_8;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Yluthi_1b          : 10;
        unsigned int Gb_Hdr_Yluthi_1c          : 10;
        unsigned int Gb_Hdr_Yluthi_1d          : 10;
        unsigned int Reserved_Hdr_Ylut9        : 2;
    } reg;
} Reg_Hdr_Ylut_9;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Yluthi_1e          : 10;
        unsigned int Gb_Hdr_Yluthi_1f          : 10;
        unsigned int Reserved_Hdr_Yluta        : 12;
    } reg;
} Reg_Hdr_Ylut_A;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Ylutlow_00         : 10;
        unsigned int Gb_Hdr_Ylutlow_01         : 10;
        unsigned int Gb_Hdr_Ylutlow_02         : 10;
        unsigned int Reserved_Hdr_Ylut10       : 2;
    } reg;
} Reg_Hdr_Ylut_10;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Ylutlow_03         : 10;
        unsigned int Gb_Hdr_Ylutlow_04         : 10;
        unsigned int Gb_Hdr_Ylutlow_05         : 10;
        unsigned int Reserved_Hdr_Ylut11       : 2;
    } reg;
} Reg_Hdr_Ylut_11;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Ylutlow_06         : 10;
        unsigned int Gb_Hdr_Ylutlow_07         : 10;
        unsigned int Gb_Hdr_Ylutlow_08         : 10;
        unsigned int Reserved_Hdr_Ylut12       : 2;
    } reg;
} Reg_Hdr_Ylut_12;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Ylutlow_09         : 10;
        unsigned int Gb_Hdr_Ylutlow_0a         : 10;
        unsigned int Gb_Hdr_Ylutlow_0b         : 10;
        unsigned int Reserved_Hdr_Ylut13       : 2;
    } reg;
} Reg_Hdr_Ylut_13;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Ylutlow_0c         : 10;
        unsigned int Gb_Hdr_Ylutlow_0d         : 10;
        unsigned int Gb_Hdr_Ylutlow_0e         : 10;
        unsigned int Reserved_Hdr_Ylut14       : 2;
    } reg;
} Reg_Hdr_Ylut_14;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Ylutlow_0f         : 10;
        unsigned int Gb_Hdr_Ylutlow_10         : 10;
        unsigned int Gb_Hdr_Ylutlow_11         : 10;
        unsigned int Reserved_Hdr_Ylut15       : 2;
    } reg;
} Reg_Hdr_Ylut_15;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Ylutlow_12         : 10;
        unsigned int Gb_Hdr_Ylutlow_13         : 10;
        unsigned int Gb_Hdr_Ylutlow_14         : 10;
        unsigned int Reserved_Hdr_Ylut16       : 2;
    } reg;
} Reg_Hdr_Ylut_16;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Ylutlow_15         : 10;
        unsigned int Gb_Hdr_Ylutlow_16         : 10;
        unsigned int Gb_Hdr_Ylutlow_17         : 10;
        unsigned int Reserved_Hdr_Ylut17       : 2;
    } reg;
} Reg_Hdr_Ylut_17;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Ylutlow_18         : 10;
        unsigned int Gb_Hdr_Ylutlow_19         : 10;
        unsigned int Gb_Hdr_Ylutlow_1a         : 10;
        unsigned int Reserved_Hdr_Ylut18       : 2;
    } reg;
} Reg_Hdr_Ylut_18;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Ylutlow_1b         : 10;
        unsigned int Gb_Hdr_Ylutlow_1c         : 10;
        unsigned int Gb_Hdr_Ylutlow_1d         : 10;
        unsigned int Reserved_Hdr_Ylut19       : 2;
    } reg;
} Reg_Hdr_Ylut_19;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gb_Hdr_Ylutlow_1e         : 10;
        unsigned int Gb_Hdr_Ylutlow_1f         : 10;
        unsigned int Reserved_Hdr_Ylut1a       : 12;
    } reg;
} Reg_Hdr_Ylut_1a;

typedef struct _Vpp_Global_regs
{
    Reg_Src_Base                     reg_Src_Base;
    Reg_Oth_Base                     reg_Oth_Base;
    Reg_Ref_Base                     reg_Ref_Base;
    Reg_Base_Map                     reg_Base_Map;
    Reg_Dst_Base                     reg_Dst_Base;
    Reg_Scl_Dst_Base_0               reg_Scl_Dst_Base_0;
    Reg_Scl_Dst_Base_1               reg_Scl_Dst_Base_1;
    Reg_Scl_Dst_Height               reg_Scl_Dst_Height;
    Reg_Bot_Base_0                   reg_Bot_Base_0;
    Reg_Bot_Base_1                   reg_Bot_Base_1;
    Reg_Bot_Base_2                   reg_Bot_Base_2;
    Reg_Hdr_Lut_Base                 reg_Hdr_Lut_Base;
    Reg_Hdr_Ysum_Base                reg_Hdr_Ysum_Base;
    Reg_Scl_Para_Error               reg_Scl_Para_Error;
    Reg_Src_Sf_0                     reg_Src_Sf_0;
    Reg_Src_Sf_1                     reg_Src_Sf_1;
    Reg_Scl_Dst_Sf_1                 reg_Scl_Dst_Sf_1;
    Reg_Scl_Dst_Sf_2                 reg_Scl_Dst_Sf_2;
    Reg_Vpp_Ref_Sf                   reg_Vpp_Ref_Sf;
    Reg_Mode_Set                     reg_Mode_Set;
    Reg_Drv_Mp_Set                   reg_Drv_Mp_Set;
    Reg_Multipass_Set0               reg_Multipass_Set0;
    Reg_Multipass_Set1               reg_Multipass_Set1;
    Reg_Clip_L_R                     reg_Clip_L_R;
    Reg_Clip_T_B                     reg_Clip_T_B;
    Reg_Lbuf_Set0                    reg_Lbuf_Set0;
    Reg_Lbuf_Set0_1                  reg_Lbuf_Set0_1;
    Reg_Lbuf_Set0_2                  reg_Lbuf_Set0_2;
    Reg_Lbuf_Set0_3                  reg_Lbuf_Set0_3;
    Reg_Lbuf_Set1                    reg_Lbuf_Set1;
    Reg_Lbuf_Set2                    reg_Lbuf_Set2;
    Reg_Lbuf_Set3                    reg_Lbuf_Set3;
    Reg_Lbuf_Set4                    reg_Lbuf_Set4;
    Reg_Lbuf_Set5                    reg_Lbuf_Set5;
    Reg_Img_Related                  reg_Img_Related;
    Reg_Hslc_Set0                    reg_Hslc_Set0;
    Reg_Hslc_Set1                    reg_Hslc_Set1;
    Reg_Hslc_Set2                    reg_Hslc_Set2;
    Reg_Scl_Para_Sd                  reg_Scl_Para_Sd;
    Reg_Scl_Para_Ds                  reg_Scl_Para_Ds;
    Reg_Scl_0                        reg_Scl_0;
    Reg_Cscf_00_01                   reg_Cscf_00_01;
    Reg_Cscf_02_10                   reg_Cscf_02_10;
    Reg_Cscf_11_12                   reg_Cscf_11_12;
    Reg_Cscf_20_21                   reg_Cscf_20_21;
    Reg_Cscf_22                      reg_Cscf_22;
    Reg_Cscf_Off_00                  reg_Cscf_Off_00;
    Reg_Cscf_Off_01                  reg_Cscf_Off_01;
    Reg_Cscf_Off_02                  reg_Cscf_Off_02;
    Reg_Cscb_00_01                   reg_Cscb_00_01;
    Reg_Cscb_02_10                   reg_Cscb_02_10;
    Reg_Cscb_11_12                   reg_Cscb_11_12;
    Reg_Cscb_20_21                   reg_Cscb_20_21;
    Reg_Cscb_22                      reg_Cscb_22;
    Reg_Cscb_Off_00                  reg_Cscb_Off_00;
    Reg_Cscb_Off_01                  reg_Cscb_Off_01;
    Reg_Cscb_Off_02                  reg_Cscb_Off_02;
    Reg_Ld_Bld_Fixcolor              reg_Ld_Bld_Fixcolor;
    Reg_Cpt_0                        reg_Cpt_0;
    Reg_Cpt_1                        reg_Cpt_1;
    Reg_Cpt_2                        reg_Cpt_2;
    Reg_Mif_Arbiter_0                reg_Mif_Arbiter_0;
    Reg_Mif_Arbiter_1                reg_Mif_Arbiter_1;
    Reg_Mif_Arbiter_2                reg_Mif_Arbiter_2;
    Reg_Mif_Slot_Idx_0               reg_Mif_Slot_Idx_0;
    Reg_Mif_Slot_Idx_1               reg_Mif_Slot_Idx_1;
    Reg_Mif_Slot_Idx_2               reg_Mif_Slot_Idx_2;
    Reg_Mif_Slot_Idx_3               reg_Mif_Slot_Idx_3;
    Reg_Mif_Slot_Idx_4               reg_Mif_Slot_Idx_4;
    Reg_Mif_Pid_0                    reg_Mif_Pid_0;
    Reg_Mif_Pid_1                    reg_Mif_Pid_1;
    Reg_Di_0                         reg_Di_0;
    Reg_Di_1                         reg_Di_1;
    Reg_Di_2                         reg_Di_2;
    Reg_Di_3                         reg_Di_3;
    Reg_Di_4                         reg_Di_4;
    Reg_Di_5                         reg_Di_5;
    Reg_Di_6                         reg_Di_6;
    Reg_Di_7                         reg_Di_7;
    Reg_Di_8                         reg_Di_8;
    Reg_Di_9                         reg_Di_9;
    Reg_Di_A                         reg_Di_A;
    Reg_Di_B                         reg_Di_B;
    Reg_Di_C                         reg_Di_C;
    Reg_Di_D                         reg_Di_D;
    Reg_Di_E                         reg_Di_E;
    Reg_Vpp_Control_0                reg_Vpp_Control_0;
    Reg_Vpp_Control_3                reg_Vpp_Control_3;
    Reg_Vpp_Control_4                reg_Vpp_Control_4;
    Reg_Vpp_Control_5                reg_Vpp_Control_5;
    Reg_Vpp_Control_6                reg_Vpp_Control_6;
    Reg_Vpp_Control_9                reg_Vpp_Control_9;
    Reg_Vpp_Lbuf_0                   reg_Vpp_Lbuf_0;
    Reg_Vpp_Lbuf_1                   reg_Vpp_Lbuf_1;
    Reg_Vpp_Fd_Control_0             reg_Vpp_Fd_Control_0;
    Reg_Vpp_Fd_Control_1             reg_Vpp_Fd_Control_1;
    Reg_Vpp_Fd_Control_2             reg_Vpp_Fd_Control_2;
    Reg_Vpp_Fd_Control_3             reg_Vpp_Fd_Control_3;
    Reg_Vpp_Fd_Control_4             reg_Vpp_Fd_Control_4;
    Reg_Vpp_Fd_Control_5             reg_Vpp_Fd_Control_5;
    Reg_Vpp_Fd_Control_6             reg_Vpp_Fd_Control_6;
    Reg_Vpp_Fd_Control_7             reg_Vpp_Fd_Control_7;
    Reg_Cg_Ctrl                      reg_Cg_Ctrl;
    Reg_From_Opcode                  reg_From_Opcode;
    Reg_Cm_0                         reg_Cm_0;
    Reg_Cm_1                         reg_Cm_1[41];
    Reg_Ccm_0                        reg_Ccm_0;
    Reg_Ccm_1                        reg_Ccm_1;
    Reg_Ccm_3                        reg_Ccm_3[16];
    Reg_Ccm_4                        reg_Ccm_4[16];
    Reg_Ccm_5                        reg_Ccm_5;
    Reg_Ccm_6                        reg_Ccm_6[16];
    Reg_Ccm_7                        reg_Ccm_7;
    Reg_Ccm_8                        reg_Ccm_8;
    Reg_Ccm_9                        reg_Ccm_9;
    Reg_Ccm_A                        reg_Ccm_A;
    Reg_Ccm_B                        reg_Ccm_B[16];
    Reg_Ccm_C                        reg_Ccm_C;
    Reg_Ccm_D                        reg_Ccm_D;
    Reg_Cc_0                         reg_Cc_0;
    Reg_Cc_1                         reg_Cc_1;
    Reg_Cc_2                         reg_Cc_2;
    Reg_Cc_3                         reg_Cc_3;
    Reg_Cc_4                         reg_Cc_4;
    Reg_Hadj_0                       reg_Hadj_0;
    Reg_Hadj_1                       reg_Hadj_1;
    Reg_Hadj_2                       reg_Hadj_2;
    Reg_Hadj_3                       reg_Hadj_3;
    Reg_Sadj_0                       reg_Sadj_0;
    Reg_Sadj_1                       reg_Sadj_1;
    Reg_Sadj_2                       reg_Sadj_2;
    Reg_Sadj_3                       reg_Sadj_3;
    Reg_Sadj_4                       reg_Sadj_4;
    Reg_Sadj_5                       reg_Sadj_5;
    Reg_Sadj_6                       reg_Sadj_6;
    Reg_Sadj_7                       reg_Sadj_7;
    Reg_Sadj_8                       reg_Sadj_8;
    Reg_Hdr_0                        reg_Hdr_0;
    Reg_Hdr_1                        reg_Hdr_1;
    Reg_Hdr_2                        reg_Hdr_2;
    Reg_Hdr_3                        reg_Hdr_3;
    Reg_Hdr_4                        reg_Hdr_4;
    Reg_Hdr_5                        reg_Hdr_5;
    Reg_Hdr_6                        reg_Hdr_6;
    Reg_Hdr_7                        reg_Hdr_7;
    Reg_Hdr_8                        reg_Hdr_8;
    Reg_Hdr_9                        reg_Hdr_9;
    Reg_Hdr_Ylut_0                   reg_Hdr_Ylut_0;
    Reg_Hdr_Ylut_1                   reg_Hdr_Ylut_1;
    Reg_Hdr_Ylut_2                   reg_Hdr_Ylut_2;
    Reg_Hdr_Ylut_3                   reg_Hdr_Ylut_3;
    Reg_Hdr_Ylut_4                   reg_Hdr_Ylut_4;
    Reg_Hdr_Ylut_5                   reg_Hdr_Ylut_5;
    Reg_Hdr_Ylut_6                   reg_Hdr_Ylut_6;
    Reg_Hdr_Ylut_7                   reg_Hdr_Ylut_7;
    Reg_Hdr_Ylut_8                   reg_Hdr_Ylut_8;
    Reg_Hdr_Ylut_9                   reg_Hdr_Ylut_9;
    Reg_Hdr_Ylut_A                   reg_Hdr_Ylut_A;
    Reg_Hdr_Ylut_10                  reg_Hdr_Ylut_10;
    Reg_Hdr_Ylut_11                  reg_Hdr_Ylut_11;
    Reg_Hdr_Ylut_12                  reg_Hdr_Ylut_12;
    Reg_Hdr_Ylut_13                  reg_Hdr_Ylut_13;
    Reg_Hdr_Ylut_14                  reg_Hdr_Ylut_14;
    Reg_Hdr_Ylut_15                  reg_Hdr_Ylut_15;
    Reg_Hdr_Ylut_16                  reg_Hdr_Ylut_16;
    Reg_Hdr_Ylut_17                  reg_Hdr_Ylut_17;
    Reg_Hdr_Ylut_18                  reg_Hdr_Ylut_18;
    Reg_Hdr_Ylut_19                  reg_Hdr_Ylut_19;
    Reg_Hdr_Ylut_1a                  reg_Hdr_Ylut_1a;
} Vpp_Global_regs;

#endif
