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

#ifndef _EU_FS_REG_H
#define _EU_FS_REG_H


#ifndef        EU_FS_BLOCKBASE_INF
    #define    EU_FS_BLOCKBASE_INF
    #define    BLOCK_EU_FS_VERSION 1
    #define    BLOCK_EU_FS_TIMESTAMP "2018/8/27 15:59:05"
    #define    EU_FS_BLOCK                                                0x3
    #define    EU_FS_REG_START                                            0x0
    #define    EU_FS_REG_END                                              0x900
    #define    EU_FS_REG_LIMIT                                            0xAC0
#endif


#define        Reg_Eu_Full_Glb_Offset                                     0x0
#define        Reg_Eu_3d_Glb_Offset                                       0x1
#define        Reg_Eu_Fe_Glb_Offset                                       0x2
#define        Reg_Mv_Cfg_Offset                                          0x3
#define        Reg_Vc_Size_Cfg0_Offset                                    0x4
#define        Reg_Vc_Size_Cfg1_Offset                                    0x5
#define        Reg_Vc_Base_Cfg0_Offset                                    0x6
#define        Reg_Vc_Base_Cfg1_Offset                                    0x7
#define        Reg_Fs_Cfg_Offset                                          0x8
#define        Reg_Max_Threads_Cfg_Offset                                 0x9
#define        Reg_Antilock_Cfg_Offset                                    0xA
#define        Reg_Vs_Ctrl_Offset                                         0xB
#define        Reg_Hs_Ctrl_Offset                                         0xC
#define        Reg_Hs_In_Cfg_Offset                                       0xD
#define        Reg_Hs_Out_Cfg_Offset                                      0xE
#define        Reg_Ds_Ctrl_Offset                                         0xF
#define        Reg_Ds_In_Cfg_Offset                                       0x10
#define        Reg_Gs_Ctrl_Offset                                         0x11
#define        Reg_Gs_In_Cfg_Offset                                       0x12
#define        Reg_Gs_Out_Cfg_Offset                                      0x13
#define        Reg_Ts_Ctrl_Offset                                         0x14
#define        Reg_Ts_Input_Mapping0_Offset                               0x15
#define        Reg_Ts_Input_Mapping1_Offset                               0x16
#define        Reg_Ts_Factors_Const_Offset                                0x17
#define        Reg_Fs_Cs_Ctrl_Offset                                      0x1D
#define        Reg_Fs_Cs_Sm_Cfg_Offset                                    0x1E
#define        Reg_Fs_Out_Loc0_Offset                                     0x1F
#define        Reg_Fs_Out_Loc1_Offset                                     0x20
#define        Reg_Fs_Out_Mapping_Offset                                  0x21
#define        Reg_Fs_Out_Comp_Offset                                     0x2D
#define        Reg_Fs_Out_Mask_Offset                                     0x30
#define        Reg_Fs_Pm_Cfg_Offset                                       0x36
#define        Reg_Fs_Pm_Id_Offset                                        0x37
#define        Reg_Vgs_Pm_Range_Offset                                    0x38
#define        Reg_Hds_Pm_Range_Offset                                    0x39
#define        Reg_Fs_U_Enable_Offset                                     0x3A
#define        Reg_Fs_U_Fmt_Offset                                        0x3E
#define        Reg_Fs_U_Layout_Offset                                     0x4E
#define        Reg_Vcs_U_Cfg_Offset                                       0x52
#define        Reg_Hs_U_Cfg_Offset                                        0x53
#define        Reg_Ds_U_Cfg_Offset                                        0x54
#define        Reg_Gs_U_Cfg_Offset                                        0x55
#define        Reg_Vcs_Instr0_Offset                                      0x56
#define        Reg_Vcs_Instr1_Offset                                      0x57
#define        Reg_Vcs_Instr_Range_Offset                                 0x58
#define        Reg_Hs_Instr0_Offset                                       0x59
#define        Reg_Hs_Instr1_Offset                                       0x5A
#define        Reg_Hs_Instr_Range_Offset                                  0x5B
#define        Reg_Ds_Instr0_Offset                                       0x5C
#define        Reg_Ds_Instr1_Offset                                       0x5D
#define        Reg_Ds_Instr_Range_Offset                                  0x5E
#define        Reg_Gs_Instr0_Offset                                       0x5F
#define        Reg_Gs_Instr1_Offset                                       0x60
#define        Reg_Gs_Instr_Range_Offset                                  0x61
#define        Reg_Vcs_Cb_Cfg_Offset                                      0x62
#define        Reg_Hs_Cb_Cfg_Offset                                       0x63
#define        Reg_Ds_Cb_Cfg_Offset                                       0x64
#define        Reg_Gs_Cb_Cfg_Offset                                       0x65
#define        Reg_Fs_Cb_Cfg_Offset                                       0x66
#define        Reg_Fs_Rev_8aligned_Offset                                 0x67
#define        Reg_Fs_Rev_Cb_Offset                                       0x68
#define        Reg_Fs_Cb_Data_Offset                                      0x100
#define        Reg_Dbg_Cfg_Offset                                         0x900
#define        Reg_Dbg_Bp_Stat_Offset                                     0x920
#define        Reg_Dbg_Bp_Pc_Offset                                       0x940
#define        Reg_Dbg_Int_Instr_Offset                                   0xA40
#define        Reg_Dbg_Res_Instr_Offset                                   0xA60
#define        Reg_Dbg_Time_Stamp_Offset                                  0xA80


typedef enum
{
    EU_FULL_GLB_SHADER_DBG_EN_DISABLED       = 0,
    EU_FULL_GLB_SHADER_DBG_EN_ENABLED        = 1,

} EU_FULL_GLB_SHADER_DBG_EN;
typedef enum
{
    EU_FULL_GLB_THD_SLOT_SIZE_8THD_SLOT      = 0,
    EU_FULL_GLB_THD_SLOT_SIZE_4THD_SLOT      = 1,
    EU_FULL_GLB_THD_SLOT_SIZE_2THD_SLOT      = 2,
    EU_FULL_GLB_THD_SLOT_SIZE_RESERVED       = 3,
} EU_FULL_GLB_THD_SLOT_SIZE;
typedef enum
{
    EU_FULL_GLB_UAV_CONTINUE_CHK_DISABLED    = 0,

    EU_FULL_GLB_UAV_CONTINUE_CHK_ENABLED     = 1,

} EU_FULL_GLB_UAV_CONTINUE_CHK;
typedef enum
{
    EU_FULL_GLB_UAV_BUF_SIZE_0LINES          = 0,
    EU_FULL_GLB_UAV_BUF_SIZE_16LINES         = 4,
    EU_FULL_GLB_UAV_BUF_SIZE_32LINES         = 5,
    EU_FULL_GLB_UAV_BUF_SIZE_64LINES         = 6,
    EU_FULL_GLB_UAV_BUF_SIZE_128LINES        = 7,
} EU_FULL_GLB_UAV_BUF_SIZE;
typedef enum
{
    EU_FULL_GLB_IC_TO_L2_DISABLED            = 0,
    EU_FULL_GLB_IC_TO_L2_ENABLED             = 1,

} EU_FULL_GLB_IC_TO_L2;
typedef enum
{
    EU_3D_GLB_U_SHARP_CACHED_DISABLED        = 0,

    EU_3D_GLB_U_SHARP_CACHED_ENABLED         = 1,

} EU_3D_GLB_U_SHARP_CACHED;
typedef enum
{
    EU_FE_GLB_CS_ON_OFF                      = 0,
    EU_FE_GLB_CS_ON_ON                       = 1,
} EU_FE_GLB_CS_ON;
typedef enum
{
    EU_FE_GLB_VS_ON_OFF                      = 0,
    EU_FE_GLB_VS_ON_ON                       = 1,
} EU_FE_GLB_VS_ON;
typedef enum
{
    EU_FE_GLB_TESS_ON_OFF                    = 0,
    EU_FE_GLB_TESS_ON_ON                     = 1,
} EU_FE_GLB_TESS_ON;
typedef enum
{
    EU_FE_GLB_GS_ON_OFF                      = 0,
    EU_FE_GLB_GS_ON_ON                       = 1,
} EU_FE_GLB_GS_ON;
typedef enum
{
    EU_FE_GLB_FS_LAST_VS                     = 0,
    EU_FE_GLB_FS_LAST_HS                     = 1,
    EU_FE_GLB_FS_LAST_DS                     = 2,
    EU_FE_GLB_FS_LAST_GS                     = 3,
    EU_FE_GLB_FS_LAST_PS                     = 4,
    EU_FE_GLB_FS_LAST_CS                     = 5,
} EU_FE_GLB_FS_LAST;
typedef enum
{
    EU_FE_GLB_OUT_RAST_EN_DISABLE            = 0,
    EU_FE_GLB_OUT_RAST_EN_ENABLE             = 1,
} EU_FE_GLB_OUT_RAST_EN;
typedef enum
{
    EU_FE_GLB_STO_EN_DISABLED                = 0,
    EU_FE_GLB_STO_EN_ENABLED                 = 1,

} EU_FE_GLB_STO_EN;
typedef enum
{
    EU_FE_GLB_OGL_EN_DX                      = 0,
    EU_FE_GLB_OGL_EN_OGL                     = 1,
} EU_FE_GLB_OGL_EN;
typedef enum
{
    EU_FE_GLB_TAS_AB_SIZE_CFG_HOLD_8VTX      = 0,
    EU_FE_GLB_TAS_AB_SIZE_CFG_HOLD_16VTX     = 1,
} EU_FE_GLB_TAS_AB_SIZE_CFG;
typedef enum
{
    MV_CFG_MV_EN_DISABLE                     = 0,
    MV_CFG_MV_EN_ENABLE                      = 1,
} MV_CFG_MV_EN;
typedef enum
{
    FS_CFG_AND_V_MASK_DISABLED               = 0,
    FS_CFG_AND_V_MASK_ENABLED                = 1,

} FS_CFG_AND_V_MASK;
typedef enum
{
    FS_CFG_RD_MODE_NEAR                      = 0,
    FS_CFG_RD_MODE_ZERO                      = 1,
    FS_CFG_RD_MODE_POS                       = 2,
    FS_CFG_RD_MODE_NEG                       = 3,
} FS_CFG_RD_MODE;
typedef enum
{
    FS_CFG_FP_MODE_STD_IEEE                  = 0,
    FS_CFG_FP_MODE_NSTD_IEEE                 = 1,
    FS_CFG_FP_MODE_MS_ALT                    = 2,
    FS_CFG_FP_MODE_RESERVED                  = 3,
} FS_CFG_FP_MODE;
typedef enum
{
    FS_CFG_DENORM_EN_DISABLED                = 0,
    FS_CFG_DENORM_EN_ENABLED                 = 1,
} FS_CFG_DENORM_EN;
typedef enum
{
    ANTILOCK_CFG_ANTILOCK_TO_EN_DISABLED     = 0,
    ANTILOCK_CFG_ANTILOCK_TO_EN_ENABLED      = 1,

} ANTILOCK_CFG_ANTILOCK_TO_EN;
typedef enum
{
    ANTILOCK_CFG_ACCUR_KICKOFF_EN_DISABLED   = 0,

    ANTILOCK_CFG_ACCUR_KICKOFF_EN_ENABLED    = 1,

} ANTILOCK_CFG_ACCUR_KICKOFF_EN;
typedef enum
{
    VS_CTRL_VID_EN_VID_NONE                  = 0,
    VS_CTRL_VID_EN_VID_AV                    = 1,
} VS_CTRL_VID_EN;
typedef enum
{
    VS_CTRL_IID_EN_IID_NONE                  = 0,
    VS_CTRL_IID_EN_IID_AV                    = 1,
} VS_CTRL_IID_EN;
typedef enum
{
    VS_CTRL_HITTEST_DISABLE_ENABLED          = 0,

    VS_CTRL_HITTEST_DISABLE_DISABLED         = 1,
    VS_CTRL_HITTEST_DISABLE_RESERVED         = 2,
} VS_CTRL_HITTEST_DISABLE;
typedef enum
{
    VS_CTRL_SIMD_NUM_1_LANE                  = 0,
    VS_CTRL_SIMD_NUM_2_LANE                  = 1,
    VS_CTRL_SIMD_NUM_4_LANE                  = 2,
    VS_CTRL_SIMD_NUM_8_LANE                  = 3,
    VS_CTRL_SIMD_NUM_16_LANE                 = 4,
    VS_CTRL_SIMD_NUM_32_LANE                 = 5,
} VS_CTRL_SIMD_NUM;
typedef enum
{
    HS_CTRL_PID_EN_PID_NONE                  = 0,
    HS_CTRL_PID_EN_PID_AV                    = 1,
} HS_CTRL_PID_EN;
typedef enum
{
    HS_CTRL_SIMD_NUM_1_LANE                  = 0,
    HS_CTRL_SIMD_NUM_2_LANE                  = 1,
    HS_CTRL_SIMD_NUM_4_LANE                  = 2,
    HS_CTRL_SIMD_NUM_8_LANE                  = 3,
    HS_CTRL_SIMD_NUM_16_LANE                 = 4,
    HS_CTRL_SIMD_NUM_32_LANE                 = 5,
} HS_CTRL_SIMD_NUM;
typedef enum
{
    HS_CTRL_CPPC_OUT_MODE_BOTH_ON            = 0,
    HS_CTRL_CPPC_OUT_MODE_CP_OFF_PC_ON       = 1,

    HS_CTRL_CPPC_OUT_MODE_BOTH_OFF           = 2,

    HS_CTRL_CPPC_OUT_MODE_RESERVED           = 3,
} HS_CTRL_CPPC_OUT_MODE;
typedef enum
{
    DS_CTRL_DOM_EN_DOM_NONE                  = 0,
    DS_CTRL_DOM_EN_DOM_AV                    = 1,
} DS_CTRL_DOM_EN;
typedef enum
{
    DS_CTRL_PID_EN_PID_NONE                  = 0,
    DS_CTRL_PID_EN_PID_AV                    = 1,
} DS_CTRL_PID_EN;
typedef enum
{
    DS_CTRL_HITTEST_DIS_ENABLED              = 0,

    DS_CTRL_HITTEST_DIS_DISABLED             = 1,
    DS_CTRL_HITTEST_DIS_RESERVED             = 2,
} DS_CTRL_HITTEST_DIS;
typedef enum
{
    DS_CTRL_SIMD_NUM_1_LANE                  = 0,
    DS_CTRL_SIMD_NUM_2_LANE                  = 1,
    DS_CTRL_SIMD_NUM_4_LANE                  = 2,
    DS_CTRL_SIMD_NUM_8_LANE                  = 3,
    DS_CTRL_SIMD_NUM_16_LANE                 = 4,
    DS_CTRL_SIMD_NUM_32_LANE                 = 5,
} DS_CTRL_SIMD_NUM;
typedef enum
{
    GS_CTRL_PID_EN_PID_NONE                  = 0,
    GS_CTRL_PID_EN_PID_AV                    = 1,
} GS_CTRL_PID_EN;
typedef enum
{
    GS_CTRL_INST_EN_INST_NONE                = 0,
    GS_CTRL_INST_EN_INST_AV                  = 1,
} GS_CTRL_INST_EN;
typedef enum
{
    GS_CTRL_SIMD_NUM_1_LANE                  = 0,
    GS_CTRL_SIMD_NUM_2_LANE                  = 1,
    GS_CTRL_SIMD_NUM_4_LANE                  = 2,
    GS_CTRL_SIMD_NUM_8_LANE                  = 3,
    GS_CTRL_SIMD_NUM_16_LANE                 = 4,
    GS_CTRL_SIMD_NUM_32_LANE                 = 5,
} GS_CTRL_SIMD_NUM;
typedef enum
{
    GS_OUT_CFG_OUT_PRIM_TYPE_POINTLIST       = 0,
    GS_OUT_CFG_OUT_PRIM_TYPE_LINESTRIP       = 1,
    GS_OUT_CFG_OUT_PRIM_TYPE_TRIANGLESTRIP   = 2,
    GS_OUT_CFG_OUT_PRIM_TYPE_LINESTRIP_ADJ   = 3,
    GS_OUT_CFG_OUT_PRIM_TYPE_TRIANGLESTRIP_ADJ= 4,
    GS_OUT_CFG_OUT_PRIM_TYPE_TRIANGLELIST_ADJ= 5,
    GS_OUT_CFG_OUT_PRIM_TYPE_PATCHLIST       = 6,
    GS_OUT_CFG_OUT_PRIM_TYPE_AA_RECT         = 7,
} GS_OUT_CFG_OUT_PRIM_TYPE;
typedef enum
{
    TS_CTRL_DOMAIN_TRIANGLE                  = 0,
    TS_CTRL_DOMAIN_QUAD                      = 1,
    TS_CTRL_DOMAIN_ISOLINE                   = 2,
    TS_CTRL_DOMAIN_RESERVED                  = 3,
} TS_CTRL_DOMAIN;
typedef enum
{
    TS_CTRL_PARTITIONING_INTEGER             = 0,
    TS_CTRL_PARTITIONING_POW2                = 1,
    TS_CTRL_PARTITIONING_FRAC_ODD            = 2,
    TS_CTRL_PARTITIONING_FRAC_EVEN           = 3,
} TS_CTRL_PARTITIONING;
typedef enum
{
    TS_CTRL_TF_REDU_MODE_REV_MIN             = 0,
    TS_CTRL_TF_REDU_MODE_REV_MAX             = 1,
    TS_CTRL_TF_REDU_MODE_REV_AVG             = 2,
    TS_CTRL_TF_REDU_MODE_REV_RESERVED        = 3,
} TS_CTRL_TF_REDU_MODE_REV;
typedef enum
{
    TS_CTRL_TF_REDU_AXIS_REV_1_AXIS          = 0,
    TS_CTRL_TF_REDU_AXIS_REV_2_AXIS          = 1,
} TS_CTRL_TF_REDU_AXIS_REV;
typedef enum
{
    TS_CTRL_TOPOLOGY_POINT                   = 0,
    TS_CTRL_TOPOLOGY_LINE                    = 1,
    TS_CTRL_TOPOLOGY_TRIANGLE_CW             = 2,
    TS_CTRL_TOPOLOGY_TRIANGLE_CCW            = 3,
} TS_CTRL_TOPOLOGY;
typedef enum
{
    TS_INPUT_MAPPING0_TF0_EN_DISABLED        = 0,
    TS_INPUT_MAPPING0_TF0_EN_ENABLED         = 1,
} TS_INPUT_MAPPING0_TF0_EN;
typedef enum
{
    TS_INPUT_MAPPING0_TF1_EN_DISABLED        = 0,
    TS_INPUT_MAPPING0_TF1_EN_ENABLED         = 1,
} TS_INPUT_MAPPING0_TF1_EN;
typedef enum
{
    TS_INPUT_MAPPING0_TF2_EN_DISABLED        = 0,
    TS_INPUT_MAPPING0_TF2_EN_ENABLED         = 1,
} TS_INPUT_MAPPING0_TF2_EN;
typedef enum
{
    TS_INPUT_MAPPING1_TF3_EN_DISABLED        = 0,
    TS_INPUT_MAPPING1_TF3_EN_ENABLED         = 1,
} TS_INPUT_MAPPING1_TF3_EN;
typedef enum
{
    TS_INPUT_MAPPING1_TF4_EN_DISABLED        = 0,
    TS_INPUT_MAPPING1_TF4_EN_ENABLED         = 1,
} TS_INPUT_MAPPING1_TF4_EN;
typedef enum
{
    TS_INPUT_MAPPING1_TF5_EN_DISABLED        = 0,
    TS_INPUT_MAPPING1_TF5_EN_ENABLED         = 1,
} TS_INPUT_MAPPING1_TF5_EN;
typedef enum
{
    FS_CS_CTRL_THREAD_MODE_SIMD32            = 0,
    FS_CS_CTRL_THREAD_MODE_SIMD64            = 1,
} FS_CS_CTRL_THREAD_MODE;
typedef enum
{
    FS_CS_CTRL_PATTERN_MODE_FLAT             = 0,
    FS_CS_CTRL_PATTERN_MODE_LINEAR           = 1,
    FS_CS_CTRL_PATTERN_MODE_TILE             = 2,
    FS_CS_CTRL_PATTERN_MODE_RESERVED         = 3,
} FS_CS_CTRL_PATTERN_MODE;
typedef enum
{
    FS_CS_SM_CFG_SM_EN_OFF                   = 0,
    FS_CS_SM_CFG_SM_EN_ON                    = 1,

} FS_CS_SM_CFG_SM_EN;
typedef enum
{
    FS_OUT_LOC0_OUT_LOC_SEPARATED            = 0,

    FS_OUT_LOC0_OUT_LOC_SHARED               = 1,

} FS_OUT_LOC0_OUT_LOC;
typedef enum
{
    FS_OUT_LOC1_OUT_LOC_SEPARATED            = 0,

    FS_OUT_LOC1_OUT_LOC_SHARED               = 1,

} FS_OUT_LOC1_OUT_LOC;
typedef enum
{
    FS_PM_CFG_PM_EN_DISABLED                 = 0,

    FS_PM_CFG_PM_EN_ENABLED                  = 1,
} FS_PM_CFG_PM_EN;
typedef enum
{
    FS_PM_CFG_TH_PM_SIZE_8KDW                = 0,

    FS_PM_CFG_TH_PM_SIZE_16KDW               = 1,
    FS_PM_CFG_TH_PM_SIZE_32KDW               = 2,
    FS_PM_CFG_TH_PM_SIZE_64KDW               = 3,
    FS_PM_CFG_TH_PM_SIZE_128KDW              = 4,
    FS_PM_CFG_TH_PM_SIZE_256KDW              = 5,
    FS_PM_CFG_TH_PM_SIZE_512KDW              = 6,
    FS_PM_CFG_TH_PM_SIZE_1024KDW             = 7,
    FS_PM_CFG_TH_PM_SIZE_1040KDW             = 8,
} FS_PM_CFG_TH_PM_SIZE;
typedef enum
{
    FS_U_ENABLE_U0_EN_DISABLE                = 0,
    FS_U_ENABLE_U0_EN_ENABLE                 = 1,
} FS_U_ENABLE_U0_EN;
typedef enum
{
    FS_U_FMT_U0_DATA_FMT_FP32                = 0,
    FS_U_FMT_U0_DATA_FMT_FP16                = 1,
    FS_U_FMT_U0_DATA_FMT_SINT32              = 2,
    FS_U_FMT_U0_DATA_FMT_SINT16              = 3,
    FS_U_FMT_U0_DATA_FMT_UINT32              = 4,
    FS_U_FMT_U0_DATA_FMT_UINT16              = 5,
    FS_U_FMT_U0_DATA_FMT_UNORM24             = 6,
    FS_U_FMT_U0_DATA_FMT_UNORM16             = 7,
    FS_U_FMT_U0_DATA_FMT_UNORM10             = 8,
    FS_U_FMT_U0_DATA_FMT_UNORM8              = 9,
    FS_U_FMT_U0_DATA_FMT_SINT8               = 10,
    FS_U_FMT_U0_DATA_FMT_UINT8               = 11,
    FS_U_FMT_U0_DATA_FMT_SNORM8              = 12,
    FS_U_FMT_U0_DATA_FMT_SNORM16             = 13,
    FS_U_FMT_U0_DATA_FMT_UINT64              = 14,
    FS_U_FMT_U0_DATA_FMT_RESERVED            = 15,
} FS_U_FMT_U0_DATA_FMT;
typedef enum
{
    FS_U_LAYOUT_U0_LAYOUT_VERTICAL           = 0,
    FS_U_LAYOUT_U0_LAYOUT_HORIZONTAL         = 1,
} FS_U_LAYOUT_U0_LAYOUT;
typedef enum
{
    DBG_CFG_EXEC_EN_DISABLED                 = 0,


    DBG_CFG_EXEC_EN_ENABLED                  = 1,

} DBG_CFG_EXEC_EN;
typedef enum
{
    DBG_CFG_TH_MODE_SINGLE                   = 0,

    DBG_CFG_TH_MODE_ALL                      = 1,
} DBG_CFG_TH_MODE;
typedef enum
{
    DBG_CFG_INT_EN_DISABLED                  = 0,

    DBG_CFG_INT_EN_ENABLED                   = 1,


} DBG_CFG_INT_EN;
typedef enum
{
    DBG_CFG_RES_EN_DISABLED                  = 0,

    DBG_CFG_RES_EN_ENABLED                   = 1,


} DBG_CFG_RES_EN;
typedef enum
{
    DBG_BP_STAT_BP_HIT_DISABLED              = 0,

    DBG_BP_STAT_BP_HIT_ENABLED               = 1,


} DBG_BP_STAT_BP_HIT;
typedef enum
{
    DBG_BP_PC_BP_VALID_DISABLED              = 0,
    DBG_BP_PC_BP_VALID_ENABLED               = 1,
} DBG_BP_PC_BP_VALID;
typedef enum
{
    DBG_BP_PC_SHADER_VS                      = 0,
    DBG_BP_PC_SHADER_HS                      = 1,
    DBG_BP_PC_SHADER_DS                      = 2,
    DBG_BP_PC_SHADER_GS                      = 3,
    DBG_BP_PC_SHADER_PS                      = 4,
    DBG_BP_PC_SHADER_CS                      = 5,
} DBG_BP_PC_SHADER;





typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Shader_Dbg_En             : 1;
        unsigned int Thd_Slot_Size             : 2;

        unsigned int Uav_En                    : 1;

        unsigned int Uav_Continue_Chk          : 1;
        unsigned int Uav_Buf_Size              : 4;



        unsigned int U_3d_Base                 : 5;



        unsigned int Cb_3d_Base                : 6;



        unsigned int Ic_To_L2                  : 1;

        unsigned int Reserved                  : 11;
    } reg;
} Reg_Eu_Full_Glb;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int U_Ps_Base                 : 5;

        unsigned int Cb_Ps_Base                : 6;


        unsigned int U_Sharp_Cached            : 1;
        unsigned int Reserved                  : 20;
    } reg;
} Reg_Eu_3d_Glb;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Cs_On                     : 1;
        unsigned int Vs_On                     : 1;
        unsigned int Tess_On                   : 1;
        unsigned int Gs_On                     : 1;
        unsigned int Fs_Last                   : 3;

        unsigned int Out_Rast_En               : 1;
        unsigned int Sto_En                    : 1;
        unsigned int Rast_Sel                  : 2;

        unsigned int Ogl_En                    : 1;

        unsigned int Tas_Ab_Size_Cfg           : 1;

        unsigned int Reserved                  : 19;
    } reg;
} Reg_Eu_Fe_Glb;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Mv_En                     : 1;
        unsigned int Mv_Num                    : 4;

        unsigned int Mv_Shared_Size            : 6;

        unsigned int Reserved                  : 21;
    } reg;
} Reg_Mv_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Vs_Vc_Size                : 16;

        unsigned int Hs_Vc_Size                : 16;

    } reg;
} Reg_Vc_Size_Cfg0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Ds_Vc_Size                : 16;

        unsigned int Gs_Vc_Size                : 16;

    } reg;
} Reg_Vc_Size_Cfg1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Vs_Vc_Base                : 16;

        unsigned int Hs_Vc_Base                : 16;

    } reg;
} Reg_Vc_Base_Cfg0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Ds_Vc_Base                : 16;

        unsigned int Gs_Vc_Base                : 16;

    } reg;
} Reg_Vc_Base_Cfg1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int And_V_Mask                : 1;
        unsigned int Rd_Mode                   : 2;
        unsigned int Fp_Mode                   : 2;
        unsigned int Denorm_En                 : 1;
        unsigned int Reserved                  : 26;
    } reg;
} Reg_Fs_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Vcs_Max_Threads           : 8;


        unsigned int Hs_Max_Threads            : 8;

        unsigned int Ds_Max_Threads            : 8;

        unsigned int Gs_Max_Threads            : 8;

    } reg;
} Reg_Max_Threads_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Vs_Antilock_Timeout       : 4;





        unsigned int Hs_Antilock_Timeout       : 4;





        unsigned int Ds_Antilock_Timeout       : 4;





        unsigned int Gs_Antilock_Timeout       : 4;





        unsigned int Antilock_To_En            : 1;

        unsigned int Accur_Kickoff_En          : 1;

        unsigned int Reserved                  : 14;
    } reg;
} Reg_Antilock_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Crf_Size                  : 6;


        unsigned int In_Size                   : 6;

        unsigned int Out_Size                  : 6;

        unsigned int Vid_En                    : 1;


        unsigned int Iid_En                    : 1;


        unsigned int Hittest_Disable           : 2;

        unsigned int Simd_Num                  : 3;
        unsigned int Reserved                  : 7;
    } reg;
} Reg_Vs_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Crf_Size                  : 6;


        unsigned int Pid_En                    : 1;


        unsigned int Simd_Num                  : 3;
        unsigned int Cppc_Out_Mode             : 2;
        unsigned int Reserved                  : 20;
    } reg;
} Reg_Hs_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Input_Cp_Num              : 6;

        unsigned int Input_Cp_Size             : 6;

        unsigned int Reserved                  : 20;
    } reg;
} Reg_Hs_In_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Output_Cp_Num             : 6;

        unsigned int Output_Cp_Size            : 6;

        unsigned int Output_Pc_Size            : 6;

        unsigned int Out_Patch_Size            : 11;


        unsigned int Reserved                  : 3;
    } reg;
} Reg_Hs_Out_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Crf_Size                  : 6;


        unsigned int Out_Size                  : 6;

        unsigned int Dom_En                    : 1;


        unsigned int Pid_En                    : 1;


        unsigned int Hittest_Dis               : 2;

        unsigned int Simd_Num                  : 3;
        unsigned int Reserved                  : 13;
    } reg;
} Reg_Ds_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Input_Cp_Num              : 6;

        unsigned int Input_Cp_Size             : 6;

        unsigned int Input_Pc_Size             : 6;

        unsigned int Reserved                  : 14;
    } reg;
} Reg_Ds_In_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Crf_Size                  : 6;


        unsigned int Pid_En                    : 1;


        unsigned int Inst_En                   : 1;


        unsigned int Instance_Cnt              : 6;



        unsigned int Simd_Num                  : 3;
        unsigned int Reserved                  : 13;
    } reg;
} Reg_Gs_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int In_Size                   : 6;

        unsigned int In_Vtx_Num                : 6;

        unsigned int Reserved                  : 20;
    } reg;
} Reg_Gs_In_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Out_Size                  : 6;

        unsigned int Out_Vtx_Num               : 11;


        unsigned int Out_Prim_Size             : 9;


        unsigned int Out_Prim_Type             : 4;

        unsigned int Reserved                  : 2;
    } reg;
} Reg_Gs_Out_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Vtx_Split_Cnt             : 6;
        unsigned int Domain                    : 2;
        unsigned int Partitioning              : 2;
        unsigned int Tf_Redu_Mode_Rev          : 2;
        unsigned int Tf_Redu_Axis_Rev          : 1;

        unsigned int Topology                  : 2;
        unsigned int Reserved                  : 17;
    } reg;
} Reg_Ts_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Tf0_Addr                  : 6;

        unsigned int Tf0_Comp_Addr             : 2;

        unsigned int Tf0_En                    : 1;
        unsigned int Tf1_Addr                  : 6;

        unsigned int Tf1_Comp_Addr             : 2;

        unsigned int Tf1_En                    : 1;
        unsigned int Tf2_Addr                  : 6;

        unsigned int Tf2_Comp_Addr             : 2;

        unsigned int Tf2_En                    : 1;
        unsigned int Reserved                  : 5;
    } reg;
} Reg_Ts_Input_Mapping0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Tf3_Addr                  : 6;

        unsigned int Tf3_Comp_Addr             : 2;

        unsigned int Tf3_En                    : 1;
        unsigned int Tf4_Addr                  : 6;

        unsigned int Tf4_Comp_Addr             : 2;

        unsigned int Tf4_En                    : 1;
        unsigned int Tf5_Addr                  : 6;

        unsigned int Tf5_Comp_Addr             : 2;

        unsigned int Tf5_En                    : 1;
        unsigned int Reserved                  : 5;
    } reg;
} Reg_Ts_Input_Mapping1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Factor                    : 32;

    } reg;
} Reg_Ts_Factors_Const;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Crf_Size                  : 6;


        unsigned int Reserved0                 : 6;

        unsigned int Thread_Mode               : 1;

        unsigned int Pattern_Mode              : 3;
        unsigned int Th_Num_In_Group           : 6;


        unsigned int Reserved                  : 10;
    } reg;
} Reg_Fs_Cs_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Sm_En                     : 1;

        unsigned int Group_Sm_Size             : 16;

        unsigned int Reserved                  : 15;
    } reg;
} Reg_Fs_Cs_Sm_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Out_Loc                   : 32;


    } reg;
} Reg_Fs_Out_Loc0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Out_Loc                   : 16;


        unsigned int Reserved                  : 16;
    } reg;
} Reg_Fs_Out_Loc1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int O0_Addr                   : 6;


        unsigned int O1_Addr                   : 6;
        unsigned int O2_Addr                   : 6;
        unsigned int O3_Addr                   : 6;
        unsigned int Reserved                  : 8;
    } reg;
} Reg_Fs_Out_Mapping;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int O0_Comp_Id                : 2;


        unsigned int O1_Comp_Id                : 2;
        unsigned int O2_Comp_Id                : 2;
        unsigned int O3_Comp_Id                : 2;
        unsigned int O4_Comp_Id                : 2;
        unsigned int O5_Comp_Id                : 2;
        unsigned int O6_Comp_Id                : 2;
        unsigned int O7_Comp_Id                : 2;
        unsigned int O8_Comp_Id                : 2;
        unsigned int O9_Comp_Id                : 2;
        unsigned int O10_Comp_Id               : 2;
        unsigned int O11_Comp_Id               : 2;
        unsigned int O12_Comp_Id               : 2;
        unsigned int O13_Comp_Id               : 2;
        unsigned int O14_Comp_Id               : 2;
        unsigned int O15_Comp_Id               : 2;
    } reg;
} Reg_Fs_Out_Comp;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int O0_Mask                   : 4;

        unsigned int O1_Mask                   : 4;
        unsigned int O2_Mask                   : 4;
        unsigned int O3_Mask                   : 4;
        unsigned int O4_Mask                   : 4;
        unsigned int O5_Mask                   : 4;
        unsigned int O6_Mask                   : 4;
        unsigned int O7_Mask                   : 4;
    } reg;
} Reg_Fs_Out_Mask;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Pm_En                     : 1;
        unsigned int Th_Pm_Size                : 4;


        unsigned int Reserved                  : 27;
    } reg;
} Reg_Fs_Pm_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Pm_U_Id                   : 32;



    } reg;
} Reg_Fs_Pm_Id;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Vcs_Idx_Ele_Range         : 14;



        unsigned int Gs_Idx_Ele_Range          : 14;

        unsigned int Reserved                  : 4;
    } reg;
} Reg_Vgs_Pm_Range;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Hs_Idx_Ele_Range          : 14;

        unsigned int Ds_Idx_Ele_Range          : 14;

        unsigned int Reserved                  : 4;
    } reg;
} Reg_Hds_Pm_Range;

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
} Reg_Fs_U_Enable;

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
} Reg_Fs_U_Fmt;

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
} Reg_Fs_U_Layout;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Vcs_U_Base                : 8;

        unsigned int Vcs_U_Range               : 8;

        unsigned int Reserved                  : 16;
    } reg;
} Reg_Vcs_U_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Hs_U_Base                 : 8;

        unsigned int Hs_U_Range                : 8;

        unsigned int Reserved                  : 16;
    } reg;
} Reg_Hs_U_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Ds_U_Base                 : 8;

        unsigned int Ds_U_Range                : 8;

        unsigned int Reserved                  : 16;
    } reg;
} Reg_Ds_U_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gs_U_Base                 : 8;

        unsigned int Gs_U_Range                : 8;

        unsigned int Reserved                  : 16;
    } reg;
} Reg_Gs_U_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Address_Low               : 32;

    } reg;
} Reg_Vcs_Instr0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Address_High              : 8;

        unsigned int Reserved                  : 24;
    } reg;
} Reg_Vcs_Instr1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Ins_Range                 : 16;

        unsigned int Reserved                  : 16;
    } reg;
} Reg_Vcs_Instr_Range;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Address_Low               : 32;

    } reg;
} Reg_Hs_Instr0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Address_High              : 8;

        unsigned int Reserved                  : 24;
    } reg;
} Reg_Hs_Instr1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Ins_Range                 : 16;

        unsigned int Reserved                  : 16;
    } reg;
} Reg_Hs_Instr_Range;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Address_Low               : 32;

    } reg;
} Reg_Ds_Instr0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Address_High              : 8;

        unsigned int Reserved                  : 24;
    } reg;
} Reg_Ds_Instr1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Ins_Range                 : 16;

        unsigned int Reserved                  : 16;
    } reg;
} Reg_Ds_Instr_Range;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Address_Low               : 32;

    } reg;
} Reg_Gs_Instr0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Address_High              : 8;

        unsigned int Reserved                  : 24;
    } reg;
} Reg_Gs_Instr1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Ins_Range                 : 16;

        unsigned int Reserved                  : 16;
    } reg;
} Reg_Gs_Instr_Range;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Vcs_Cb_Base               : 9;

        unsigned int Vcs_Cb_Range              : 9;

        unsigned int Reserved                  : 14;
    } reg;
} Reg_Vcs_Cb_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Hs_Cb_Base                : 9;

        unsigned int Hs_Cb_Range               : 9;

        unsigned int Reserved                  : 14;
    } reg;
} Reg_Hs_Cb_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Ds_Cb_Base                : 9;

        unsigned int Ds_Cb_Range               : 9;

        unsigned int Reserved                  : 14;
    } reg;
} Reg_Ds_Cb_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Gs_Cb_Base                : 9;

        unsigned int Gs_Cb_Range               : 9;

        unsigned int Reserved                  : 14;
    } reg;
} Reg_Gs_Cb_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Fs_Cb_Range               : 9;

        unsigned int Reserved                  : 23;
    } reg;
} Reg_Fs_Cb_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Fs_Rev_8aligned;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Fs_Rev_Cb;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Const                     : 32;
    } reg;
} Reg_Fs_Cb_Data;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Exec_En                   : 1;
        unsigned int Th_Mode                   : 1;
        unsigned int Int_En                    : 1;
        unsigned int Res_En                    : 1;
        unsigned int Dbg_Th_Id                 : 5;


        unsigned int Reserved                  : 23;
    } reg;
} Reg_Dbg_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Bp_Hit                    : 1;

        unsigned int Bp_Id                     : 3;
        unsigned int Hit_Th_Id                 : 5;

        unsigned int Reserved                  : 23;
    } reg;
} Reg_Dbg_Bp_Stat;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Bp_Valid                  : 1;
        unsigned int Shader                    : 3;
        unsigned int Offset                    : 16;


        unsigned int Reserved                  : 12;
    } reg;
} Reg_Dbg_Bp_Pc;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Offset                    : 16;

        unsigned int Reserved                  : 16;
    } reg;
} Reg_Dbg_Int_Instr;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Offset                    : 16;

        unsigned int Reserved                  : 16;
    } reg;
} Reg_Dbg_Res_Instr;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Value                     : 32;

    } reg;
} Reg_Dbg_Time_Stamp;

typedef struct _Eu_Fs_regs
{
    Reg_Eu_Full_Glb                  reg_Eu_Full_Glb;
    Reg_Eu_3d_Glb                    reg_Eu_3d_Glb;
    Reg_Eu_Fe_Glb                    reg_Eu_Fe_Glb;
    Reg_Mv_Cfg                       reg_Mv_Cfg;
    Reg_Vc_Size_Cfg0                 reg_Vc_Size_Cfg0;
    Reg_Vc_Size_Cfg1                 reg_Vc_Size_Cfg1;
    Reg_Vc_Base_Cfg0                 reg_Vc_Base_Cfg0;
    Reg_Vc_Base_Cfg1                 reg_Vc_Base_Cfg1;
    Reg_Fs_Cfg                       reg_Fs_Cfg;
    Reg_Max_Threads_Cfg              reg_Max_Threads_Cfg;
    Reg_Antilock_Cfg                 reg_Antilock_Cfg;
    Reg_Vs_Ctrl                      reg_Vs_Ctrl;
    Reg_Hs_Ctrl                      reg_Hs_Ctrl;
    Reg_Hs_In_Cfg                    reg_Hs_In_Cfg;
    Reg_Hs_Out_Cfg                   reg_Hs_Out_Cfg;
    Reg_Ds_Ctrl                      reg_Ds_Ctrl;
    Reg_Ds_In_Cfg                    reg_Ds_In_Cfg;
    Reg_Gs_Ctrl                      reg_Gs_Ctrl;
    Reg_Gs_In_Cfg                    reg_Gs_In_Cfg;
    Reg_Gs_Out_Cfg                   reg_Gs_Out_Cfg;
    Reg_Ts_Ctrl                      reg_Ts_Ctrl;
    Reg_Ts_Input_Mapping0            reg_Ts_Input_Mapping0;
    Reg_Ts_Input_Mapping1            reg_Ts_Input_Mapping1;
    Reg_Ts_Factors_Const             reg_Ts_Factors_Const[6];
    Reg_Fs_Cs_Ctrl                   reg_Fs_Cs_Ctrl;
    Reg_Fs_Cs_Sm_Cfg                 reg_Fs_Cs_Sm_Cfg;
    Reg_Fs_Out_Loc0                  reg_Fs_Out_Loc0;
    Reg_Fs_Out_Loc1                  reg_Fs_Out_Loc1;
    Reg_Fs_Out_Mapping               reg_Fs_Out_Mapping[12];
    Reg_Fs_Out_Comp                  reg_Fs_Out_Comp[3];
    Reg_Fs_Out_Mask                  reg_Fs_Out_Mask[6];
    Reg_Fs_Pm_Cfg                    reg_Fs_Pm_Cfg;
    Reg_Fs_Pm_Id                     reg_Fs_Pm_Id;
    Reg_Vgs_Pm_Range                 reg_Vgs_Pm_Range;
    Reg_Hds_Pm_Range                 reg_Hds_Pm_Range;
    Reg_Fs_U_Enable                  reg_Fs_U_Enable[4];
    Reg_Fs_U_Fmt                     reg_Fs_U_Fmt[16];
    Reg_Fs_U_Layout                  reg_Fs_U_Layout[4];
    Reg_Vcs_U_Cfg                    reg_Vcs_U_Cfg;
    Reg_Hs_U_Cfg                     reg_Hs_U_Cfg;
    Reg_Ds_U_Cfg                     reg_Ds_U_Cfg;
    Reg_Gs_U_Cfg                     reg_Gs_U_Cfg;
    Reg_Vcs_Instr0                   reg_Vcs_Instr0;
    Reg_Vcs_Instr1                   reg_Vcs_Instr1;
    Reg_Vcs_Instr_Range              reg_Vcs_Instr_Range;
    Reg_Hs_Instr0                    reg_Hs_Instr0;
    Reg_Hs_Instr1                    reg_Hs_Instr1;
    Reg_Hs_Instr_Range               reg_Hs_Instr_Range;
    Reg_Ds_Instr0                    reg_Ds_Instr0;
    Reg_Ds_Instr1                    reg_Ds_Instr1;
    Reg_Ds_Instr_Range               reg_Ds_Instr_Range;
    Reg_Gs_Instr0                    reg_Gs_Instr0;
    Reg_Gs_Instr1                    reg_Gs_Instr1;
    Reg_Gs_Instr_Range               reg_Gs_Instr_Range;
    Reg_Vcs_Cb_Cfg                   reg_Vcs_Cb_Cfg;
    Reg_Hs_Cb_Cfg                    reg_Hs_Cb_Cfg;
    Reg_Ds_Cb_Cfg                    reg_Ds_Cb_Cfg;
    Reg_Gs_Cb_Cfg                    reg_Gs_Cb_Cfg;
    Reg_Fs_Cb_Cfg                    reg_Fs_Cb_Cfg;
    Reg_Fs_Rev_8aligned              reg_Fs_Rev_8aligned;
    Reg_Fs_Rev_Cb                    reg_Fs_Rev_Cb[152];
    Reg_Fs_Cb_Data                   reg_Fs_Cb_Data[2048];
    Reg_Dbg_Cfg                      reg_Dbg_Cfg[32];
    Reg_Dbg_Bp_Stat                  reg_Dbg_Bp_Stat[32];
    Reg_Dbg_Bp_Pc                    reg_Dbg_Bp_Pc[256];
    Reg_Dbg_Int_Instr                reg_Dbg_Int_Instr[32];
    Reg_Dbg_Res_Instr                reg_Dbg_Res_Instr[32];
    Reg_Dbg_Time_Stamp               reg_Dbg_Time_Stamp[64];
} Eu_Fs_regs;

#endif
