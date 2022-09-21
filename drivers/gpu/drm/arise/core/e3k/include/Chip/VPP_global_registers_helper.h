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

#ifndef _VPP_GLOBAL_REGISTERSHELPER_H
#define _VPP_GLOBAL_REGISTERSHELPER_H
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "VPP_global_registers.h"


enum
{
    GB_SRC_SF_BASE,
    GB_OTH_SF_BASE,
    GB_REF_SF_BASE,
    GB_SRC_RANGE_MAP_Y_EN,
    GB_SRC_RANGE_MAP_Y,
    GB_SRC_RANGE_MAP_UV_EN,
    GB_SRC_RANGE_MAP_UV,
    GB_OTH_RANGE_MAP_Y_EN,
    GB_OTH_RANGE_MAP_Y,
    GB_OTH_RANGE_MAP_UV_EN,
    GB_OTH_RANGE_MAP_UV,
    GB_REF_RANGE_MAP_Y_EN,
    GB_REF_RANGE_MAP_Y,
    GB_REF_RANGE_MAP_UV_EN,
    GB_REF_RANGE_MAP_UV,
    RESERVED_BM,
    GB_DST_SF_BASE,
    GB_SCL_DST_SF_BASE,
    GB_SCL_DST_CLIPX,
    GB_SCL_DST_CLIPY,
    RESERVED_MOB,
    GB_SCL_DST_SF_HEIGHT,
    RESERVED_SDB2,
    GB_SRC_SF_BOT_BASE,
    GB_OTH_SF_BOT_BASE,
    GB_REF_SF_BOT_BASE,
    GB_HDR_INT_BASE,
    GB_YSUM_MEM_BASE,
    GB_HS_ERROR,
    GB_VS_ERROR,
    GB_SRC_WIDTH,
    GB_SEQ_ID_THRD,
    RESERVED_SRC_SF_0,
    RESERVED_SRC_SF_1,
    GB_SRC_FMT,
    GB_SRC_TILE,
    GB_SRC_HEIGHT,
    GB_SRC_PITCH,
    GB_SCL_DST_FMT,
    GB_SCL_DST_HEIGHT,
    GB_SCL_DST_PITCH,
    GB_SCL_DST_COMPRESS_EN,
    RESERVED_SCL_DST_SF_1,
    GB_SCL_DST_WIDTH,
    RESERVE_DST2,
    RESERVED_RS,
    GB_VPP_DNREF_SF_PITCH,
    GB_VPP_DNREF_SF_HEIGHT,
    GB_BLT_MODE,
    GB_SCL_MODE,
    GB_EN_HSCL,
    GB_EN_VSCL,
    RESERVED_GB_FIRST_SLICE,
    GB_SE_VPP,
    GB_ROTATION_MODE,
    GB_SRC_COMPRESS,
    GB_FOR_ECO,
    GB_X_UPSAMPLE_MODE,
    GB_DS_24_X,
    GB_DS_24_Y,
    GB_BLT_SIMPLE,
    GB_EN_4K_MEM_SWZL,
    GB_DS4_LSB2FORCE,
    GB_FIXCOLOR_MODE,
    GB_Y_UPSAMPLE_MODE,
    RESERVED_MS,
    RESERVED_GB_SCL_LEFT_EDGE_WIDTH,
    RESERVED_GB_SCL_RIGHT_EDGE_WIDTH,
    GB_LDRD_COMPRESS,
    GB_SEQ_ID_PEAKLEVEL,
    RESERVED_DMS,
    GB_SLICE_WIDTH,
    GB_SLICE_PIXEL_NUM,
    GB_SLICE_WIDTH_1ST,
    GB_SLICE_PIXEL_NUM_1ST,
    GB_SIGNAT_SEL_IO,
    GB_SIGNAT_SEL_OO,
    GB_SIG_ENADDR_IO,
    RESERVED_MP1,
    GB_CL_LEFT,
    GB_CL_RIGHT,
    RESERVED17,
    GB_CL_TOP,
    GB_CL_BOTTOM,
    RESERVED16,
    GB_VPPCNTOUT,
    GB_VPPARBMODE,
    GB_LB_SCL_YUV_OFFSET,
    GB_LB_SCL_A_OFFSET,
    RESERVED_LS0,
    GB_LB_SCL_YUV_PITCH,
    GB_LB_SCL_A_PITCH,
    GB_LB_NM_OPER_OFFSET,
    RESERVED_LS01,
    GB_LB_SR_Y_OFFSET,
    GB_LB_SR_UV_OFFSET,
    GB_LB_SR_Y_PITCH,
    GB_LB_SR_A_OFFSET,
    GB_LB_SR_UV_PITCH,
    GB_LB_SR_A_PITCH,
    RESERVED_LS03,
    GB_LBADIW,
    GB_LBASCLYUVW,
    GB_LBASCLAW,
    GB_LBASRUVW,
    GB_LBA_BANK_MODE,
    RESERVED_LS1,
    GB_LBASRYW,
    GB_LBASRAW,
    RESERVED_LS2,
    GB_LBADIR,
    GB_LBADIREFR,
    GB_LBASCRR,
    RESERVED_LS3,
    GB_LBASCLYUVR,
    GB_LBASCLAR,
    GB_LBASRUVR,
    RESERVED_LS4,
    GB_LBASRYR,
    GB_LBASRAR,
    RESERVED_LS5,
    GB_MOB_FIXALPHA,
    GB_REDUNDANT_EDGE_WIDTH,
    GB_MOB_RANGE_MIN,
    GB_MOB_RANGE_MAXRY,
    GB_MOB_RANGE_MAXGBUV,
    GB_SRC_LOSSY_COM_EN,
    GB_DST_LOSSY_COM_EN,
    GB_MOB_BLENDING_EN,
    GB_MOB_BLENDING_DIU_MODE,
    GB_MOB_BLENDING_STREAM_MODE,
    RESERVED_IR,
    GB_HSLC_Y_A0,
    GB_HSLC_Y_B0,
    RESERVED_HSLC0,
    GB_HSLC_UV_A0,
    GB_HSLC_UV_A1,
    RESERVED_HSLC1,
    GB_HSLC_U_B0,
    GB_HSLC_V_B0,
    RESERVED_HSLC2,
    GB_SD_WIDTH_RATIO,
    GB_SD_HEIGHT_RATIO,
    GB_DS_WIDTH_RATIO,
    GB_DS_HEIGHT_RATIO,
    RESERVED_DS,
    GB_SRC_EN_ALPHA,
    GB_SCL_BC_DELTA,
    GB_EN_SCL_MAXMIN,
    GB_EN_SR,
    GB_SR_CTDIFFTH,
    GB_EN_PIXELTYPE,
    GB_SR_BLENDSTRENGTH,
    GB_SR_ENCTBLEND,
    GB_SR_BLENDWEIGHT,
    GB_SR_COEFWEIGHT,
    GB_SR_COEFWEIGHTOTHER,
    RESERVED_SCL0,
    GB_CSCF_M_00,
    GB_CSCF_M_01,
    GB_CSCF_EN,
    GB_CSCF_ROUND,
    RESERVED_CSCF_00_01,
    GB_CSCF_M_02,
    GB_CSCF_M_10,
    RESERVED_CSCF_02_10,
    GB_CSCF_M_11,
    GB_CSCF_M_12,
    RESERVED_CSCF_11_12,
    GB_CSCF_M_20,
    GB_CSCF_M_21,
    RESERVED_CSCF_20_21,
    GB_CSCF_M_22,
    RESERVED_CSCF_22,
    GB_CSCF_OFF_00,
    RESERVED_CSCF_OFF_00,
    GB_CSCF_OFF_01,
    RESERVED_CSCF_OFF_01,
    GB_CSCF_OFF_02,
    RESERVED_CSCF_OFF_02,
    GB_CSCB_M_00,
    GB_CSCB_M_01,
    GB_CSCB_EN,
    GB_CSCB_ROUND,
    RESERVED_CSCB_00_01,
    GB_CSCB_M_02,
    GB_CSCB_M_10,
    RESERVED_CSCB_02_10,
    GB_CSCB_M_11,
    GB_CSCB_M_12,
    RESERVED_CSCB_11_12,
    GB_CSCB_M_20,
    GB_CSCB_M_21,
    RESERVED_CSCB_20_21,
    GB_CSCB_M_22,
    RESERVED_CSCB_22,
    GB_CSCB_OFF_00,
    RESERVED_CSCB_OFF_00,
    GB_CSCB_OFF_01,
    RESERVED_CSCB_OFF_01,
    GB_CSCB_OFF_02,
    RESERVED_CSCB_OFF_02,
    GB_FIXCOLOR_VALUE,
    GB_LB_CPT_OFFSET,
    GB_LB_CPT_SIZE,
    GB_LB_MOB_L2T_PITCH,
    GB_DST_END,
    GB_DST_TILE,
    GB_LB_MOB_L2T_OFFSET,
    GB_MOB_DST_ALPHAMODE,
    GB_MOB_BLENDING_MODE,
    GB_MOB_BLENDING_METHOD,
    GB_MOB_BLENDING_ROUNDING,
    GB_CPT_DNSAMPLE_MODE,
    GB_MOB_BLDALPHA,
    GB_CPT_EN,
    GB_DST_SF_WIDTH,
    RESERVED_CPT2,
    GB_MIFARB_REG,
    GB_MIFARB_YDIFF,
    GB_MIFARB_LD,
    RESERVED_MA0,
    GB_MIFARB_CPTR,
    GB_MIFARB_MTDR,
    GB_MIFARB_MOB,
    GB_MIFARB_PASSIVE,
    RESERVED_MA1,
    GB_MIFARB_MTDW,
    GB_MIFARB_HDRR,
    GB_MIFARB_HDRW,
    RESERVED_MA2,
    GB_SRC_BL_SLOT_IDX,
    GB_OTH_BL_SLOT_IDX,
    GB_REF_BL_SLOT_IDX,
    GB_DST_BL_SLOT_IDX,
    GB_SCL_DST_BL_SLOT_IDX,
    RESERVED_MSI2,
    GB_SRC_BOT_BL_SLOT_IDX,
    GB_OTH_BOT_BL_SLOT_IDX,
    GB_REF_BOT_BL_SLOT_IDX,
    RESERVED_MSI4,
    GB_SRC_PROC_ID,
    GB_OTH_PROC_ID,
    GB_REF_PROC_ID,
    GB_SRC_BOT_PROC_ID,
    GB_OTH_BOT_PROC_ID,
    GB_REF_BOT_PROC_ID,
    GB_DST_PROC_ID,
    GB_SCL_DST_PROC_ID,
    GB_MTD_HIST_PROC_ID,
    GB_HDR_YSUM_PROC_ID,
    GB_HDR_INT_PROC_ID,
    RESERVED_MPID1,
    GB_DI_INTRA_TH,
    GB_DI_ISFEATHER_TH,
    GB_DI_FORCE_EELA_TH,
    RESERVED_DI0,
    GB_DI_SCROLL_TH,
    GB_POINT_DEGREE_VAR_K,
    RESERVED_DI1,
    GB_SOBEL_TH,
    GB_SOBEL_FORCE_VALUE,
    GB_DI_FEATHER_EDGE_CNT_X1,
    GB_DI_FEATHER_EDGE_CNT_Y1,
    RESERVED_DI2,
    GB_DI_IAD_MD_X1,
    GB_DI_IAD_MD_X2,
    GB_DI_IAD_MD_Y1,
    GB_DI_IAD_MD_K,
    RESERVED_DI4,
    GB_POINT_DEGREE_VAR_X1,
    GB_POINT_DEGREE_VAR_X2,
    GB_POINT_DEGREE_VAR_Y1,
    GB_TOP_BOT_FIELD_DIV,
    GB_DI_ENABLE_STATICFLAG,
    GB_DI_FEATHER_EDGE_CNT_K,
    GB_DI_FEATHER_EDGE_CNT_X2,
    GB_FD_WAIT_FRAME,
    RESERVED_DI6,
    GB_EDGE_FLAG_TH,
    GB_DI_INTRA_DIFF_TH,
    RESERVED_DI7,
    GB_USE_MV_JUDGE,
    GB_USE_ALLMV,
    GB_MV_DIFF_TH,
    GB_DIRECTION_JUDGE_TH,
    GB_FD_BAD_MERGE_TH_1,
    GB_FD_BAD_MERGE_TH_2,
    RESERVED_DI8,
    GB_DI_MD_AREA_TH0,
    GB_DI_MD_AREA_TH1,
    GB_DI_MD_AREA_COEF0,
    GB_DI_MD_AREA_COEF1,
    GB_DI_MD_ADJ_COEF0,
    GB_DI_MD_ADJ_COEF1,
    RESERVED_DIA,
    GB_DI_IAD_AVE_ADJ_TH0,
    GB_DI_IAD_AVE_ADJ_TH1,
    RESERVED_DIB,
    GB_DI_IAD_AVE_ADJ_VAL,
    GB_DI_IAD_AVE_ADJ_COEF,
    GB_DI_MTD_FEATHERDETECT_EN,
    RESERVED_DIC,
    GB_DI_IAD_LINE_MD_X1,
    GB_DI_IAD_LINE_MD_X2,
    GB_DI_MTD_FEATHERPARA,
    GB_DI_IAD_LINE_MD_Y1,
    GB_DI_IAD_LINE_MD_K,
    GB_DI_TH,
    RESERVED_VC01,
    GB_MD_TH2,
    GB_MD_TH2_EN,
    RESERVED_VC0,
    GB_MD_TH,
    GB_DI_SR_REV_DIFF_TH,
    RESERVED_VC3,
    GB_DI_MTD_EDGEVALTHR,
    GB_DI_MTD_STATICPARA,
    GB_DI_MTD_MAXMINTHR,
    GB_DI_SR_GRAD_TH,
    GB_DI_MTD_MD_TH,
    RESERVED_VC4,
    GB_DI_SR_LINE_DIFF_TH,
    GB_DI_IAD_MD_AVE_TH,
    RESERVED_VC12,
    GB_MTD_COL_HIST_BASE,
    GB_MD_LOW_TH,
    RESERVED_VC9,
    GB_LBUF_SIZE,
    GB_VPP_IN_SIZE,
    GB_DISABLE_LCID_BLEND,
    GB_FD_FORCE_DI,
    GB_DI_BLEND_COEF,
    RESERVED_VL0,
    GB_VPP_DI_OFFSET,
    GB_VPP_DI_SIZE,
    GB_VPP_DIREF_IN_OFFSET,
    RESERVED_VL1,
    GB_FD_MD_TH,
    GB_SCENE_CHANGE_TOP,
    GB_SCENE_CHANGE_BOTTOM,
    GB_2V2_TH_DELTA,
    GB_2V2_TH_RATIO,
    GB_2V2_SAD_COUNTER_TH,
    GB_2V2_MD_FAILED_COUNTER_TH,
    GB_LCID_DIFF_TH,
    GB_2V2_COUNT_MODE,
    GB_SCENE_CHANGE_TH,
    GB_SCENECHANGE_MD_MODE,
    GB_FD_MD_TH2,
    GB_FD_COUNTER_TH,
    GB_FD_DETECT_COUNTER_INITIAL,
    GB_FD_STRONG_COUNTER_INITIAL,
    GB_2V2_FAILED_COUNTER_TH,
    GB_2V2_MD_SUCCEED_COUNTER_TH,
    GB_2V2_SAD_SUCCEED_COUNTER_TH,
    GB_2V2_SAD_FAILED_RANGE,
    GB_2V2_FRAME_CNT_TH,
    GB_2V2_FAILED_COUNTER_TH2,
    GB_FD_2V2_SAD_CLR_TH_1,
    GB_FD_2V2_SAD_CLR_TH_2,
    GB_FD_2V2_SAD_CLR_TH_3,
    RESERVED_FD4,
    GB_FD_BAD_COPY_TH_1,
    GB_FD_BAD_COPY_TH_2,
    GB_FD_BAD_COPY_TH_3,
    GB_FD_BAD_COPY_TH_4,
    GB_FD_QUIT_2V2_TH_1,
    RESERVED_FD5,
    GB_FD_QUIT_2V2_TH_2,
    GB_FD_QUIT_2V2_TH_3,
    GB_FD_QUIT_2V2_TH_4,
    GB_FD_QUIT_2V2_TH_5,
    GB_FD_2V2_MD_TH_3,
    GB_FD_2V2_MD_TH_5,
    GB_FD_2V2_MD_TH_1,
    GB_FD_2V2_MD_TH_2,
    GB_FD_2V2_MD_TH_4,
    GB_FD_SC_QUIT_2V2_TH,
    GB_DI_CG_OFF,
    GB_SCL_CG_OFF,
    GB_SCL_BP_CG_OFF,
    GB_CSCB_CG_OFF,
    GB_CSCF_CG_OFF,
    GB_CG_BUSY_TH,
    GB_FIRE_AFTER_SAVE,
    GB_RESTORE_KEEP_ON,
    GB_FOR_ECO2,
    GB_CPT_CG_OFF,
    GB_SR_CG_OFF,
    GB_CM_CG_OFF,
    GB_CCM_CG_OFF,
    GB_HDR_CG_OFF,
    RESERVED_CG,
    GB_DI_EN,
    GB_FD_EN,
    GB_MTD_EN,
    GB_TWO_FRAME_OUT_EN,
    GB_DI_FORCE_LCID,
    GB_SINGLE_SEL_OTH,
    GB_TOP_FIELD_FIRST,
    GB_RESET_HQVPP,
    GB_HQVPP_MODE,
    GB_VPP_FD_INDEX,
    RESERVED_OPCODE,
    GB_CMLUT_EN,
    GB_TINT_EN,
    GB_TINT_HOFFSET,
    RESERVED_CM0,
    GB_CMLUT0,
    GB_CMLUT1,
    RESERVED_CM1,
    GB_CCM_EN,
    GB_METADATA_EN,
    GB_DG_HCOE,
    RESERVED_CCM0,
    GB_DG_LCOE,
    GB_YUV2RGB_EN,
    GB_YUV2RGB_MODE,
    GB_CCM_HDR_INV,
    RESERVED_CCM1,
    GB_DG_TBL_0,
    GB_DG_TBL_1,
    GB_G_TBL_0,
    GB_G_TBL_1,
    GB_G_TBL_2,
    RESERVED_CCM5,
    GB_HLG_EN,
    GB_HLG_ACOEF,
    GB_HLG_BCOEF,
    RESERVED_CCM6,
    GB_HLG_TBL_0,
    GB_HLG_TBL_1,
    GB_HLG_TBL_20,
    GB_HLG_TBL_21,
    GB_HLG_TBL_22,
    GB_HLG_INVTBL_22,
    GB_1886_ACOEF,
    GB_1886_BCOEF,
    GB_HLG_INVACOEF,
    GB_HLG_INVBCOEF,
    GB_1886_EN,
    RESERVED_CCM9,
    GB_HLG_INVTBL_0,
    GB_HLG_INVTBL_1,
    GB_HLG_INVTBL_20,
    GB_HLG_INVTBL_21,
    GB_1886_INVACOEF,
    GB_1886_INVBCOEF,
    GB_CONVERT_M_00,
    GB_CONVERT_M_01,
    RESERVED_CC0,
    GB_CONVERT_M_10,
    GB_CONVERT_M_11,
    RESERVED_CC1,
    GB_CONVERT_M_20,
    GB_CONVERT_M_21,
    RESERVED_CC2,
    GB_CONVERT_M_02,
    GB_CONVERT_M_12,
    RESERVED_CC3,
    GB_CONVERT_M_22,
    RESERVED_CC4,
    GB_HADJ_EN,
    GB_HRANGE_GL,
    GB_HRANGE_GH,
    RESERVED_HADJ0,
    GB_HRANGE_RL,
    GB_HRANGE_RH,
    RESERVED_HADJ1,
    GB_G_RANGE,
    GB_G_AXIS,
    GB_G_COEF,
    GB_R_RANGE,
    GB_R_AXIS,
    GB_R_COEF,
    GB_SADJ_EN,
    GB_SADJ_INC00,
    GB_SADJ_INC01,
    RESERVED_SADJ0,
    GB_SADJ_INC02,
    GB_SADJ_INC03,
    RESERVED_SADJ1,
    GB_SADJ_INC04,
    GB_SADJ_INC05,
    RESERVED_SADJ2,
    GB_SADJ_INC06,
    GB_SADJ_INC07,
    GB_SADJ_INC14,
    RESERVED_SADJ3,
    GB_SADJ_INC08,
    GB_SADJ_INC09,
    RESERVED_SADJ4,
    GB_SADJ_INC0A,
    GB_SADJ_INC0B,
    GB_SADJ_INC13,
    RESERVED_SADJ5,
    GB_SADJ_INC0C,
    GB_SADJ_INC0D,
    RESERVED_SADJ6,
    GB_SADJ_INC0E,
    GB_SADJ_INC0F,
    GB_SADJ_INC12,
    RESERVED_SADJ7,
    GB_SADJ_INC10,
    GB_SADJ_INC11,
    RESERVED_SADJ8,
    GB_HDR_EN,
    GB_HDR_CURVE_EN,
    GB_HDR_CLIPLMT,
    GB_HDR_MAXFMEAN,
    GB_HDR_MAXMEAN,
    GB_HDR_SC_EN,
    GB_HDR_SMALLSIZE,
    GB_HDR_MAXPEAKCNT,
    GB_HDR_MINPEAKWG,
    GB_HDR_MAXPEAKWG,
    GB_HDR_MANUALCLIPMODE,
    GB_HDR_FRMWG,
    GB_RESET_HDR,
    RESERVED_HDR1,
    GB_HDR_PIXCNTFORSTAT,
    GB_HDR_SHFT,
    GB_HDR_MINSTRETCHRANGE,
    RESERVED_HDR3,
    GB_HDR_MINFILTSPD,
    GB_HDR_BINMEDSDT,
    GB_HDR_DELTAMEANTH,
    GB_HDR_BINMEDHIMAX,
    GB_HDR_YLUTHIWGINIT,
    RESERVED_HDR4,
    GB_HDR_MEANCHANGETH,
    RESERVED_HDR5,
    GB_HDR_PIXLOWRATIO,
    RESERVED_HDR6,
    GB_HDR_PIXHIRATIO,
    RESERVED_HDR7,
    GB_HDR_BLACKRATIO,
    RESERVED_HDR8,
    GB_HDR_NOISERATIO,
    RESERVED_HDR9,
    GB_HDR_YLUTHI_00,
    GB_HDR_YLUTHI_01,
    GB_HDR_YLUTHI_02,
    RESERVED_HDR_YLUT0,
    GB_HDR_YLUTHI_03,
    GB_HDR_YLUTHI_04,
    GB_HDR_YLUTHI_05,
    RESERVED_HDR_YLUT1,
    GB_HDR_YLUTHI_06,
    GB_HDR_YLUTHI_07,
    GB_HDR_YLUTHI_08,
    RESERVED_HDR_YLUT2,
    GB_HDR_YLUTHI_09,
    GB_HDR_YLUTHI_0A,
    GB_HDR_YLUTHI_0B,
    RESERVED_HDR_YLUT3,
    GB_HDR_YLUTHI_0C,
    GB_HDR_YLUTHI_0D,
    GB_HDR_YLUTHI_0E,
    RESERVED_HDR_YLUT4,
    GB_HDR_YLUTHI_0F,
    GB_HDR_YLUTHI_10,
    GB_HDR_YLUTHI_11,
    RESERVED_HDR_YLUT5,
    GB_HDR_YLUTHI_12,
    GB_HDR_YLUTHI_13,
    GB_HDR_YLUTHI_14,
    RESERVED_HDR_YLUT6,
    GB_HDR_YLUTHI_15,
    GB_HDR_YLUTHI_16,
    GB_HDR_YLUTHI_17,
    RESERVED_HDR_YLUT7,
    GB_HDR_YLUTHI_18,
    GB_HDR_YLUTHI_19,
    GB_HDR_YLUTHI_1A,
    RESERVED_HDR_YLUT8,
    GB_HDR_YLUTHI_1B,
    GB_HDR_YLUTHI_1C,
    GB_HDR_YLUTHI_1D,
    RESERVED_HDR_YLUT9,
    GB_HDR_YLUTHI_1E,
    GB_HDR_YLUTHI_1F,
    RESERVED_HDR_YLUTA,
    GB_HDR_YLUTLOW_00,
    GB_HDR_YLUTLOW_01,
    GB_HDR_YLUTLOW_02,
    RESERVED_HDR_YLUT10,
    GB_HDR_YLUTLOW_03,
    GB_HDR_YLUTLOW_04,
    GB_HDR_YLUTLOW_05,
    RESERVED_HDR_YLUT11,
    GB_HDR_YLUTLOW_06,
    GB_HDR_YLUTLOW_07,
    GB_HDR_YLUTLOW_08,
    RESERVED_HDR_YLUT12,
    GB_HDR_YLUTLOW_09,
    GB_HDR_YLUTLOW_0A,
    GB_HDR_YLUTLOW_0B,
    RESERVED_HDR_YLUT13,
    GB_HDR_YLUTLOW_0C,
    GB_HDR_YLUTLOW_0D,
    GB_HDR_YLUTLOW_0E,
    RESERVED_HDR_YLUT14,
    GB_HDR_YLUTLOW_0F,
    GB_HDR_YLUTLOW_10,
    GB_HDR_YLUTLOW_11,
    RESERVED_HDR_YLUT15,
    GB_HDR_YLUTLOW_12,
    GB_HDR_YLUTLOW_13,
    GB_HDR_YLUTLOW_14,
    RESERVED_HDR_YLUT16,
    GB_HDR_YLUTLOW_15,
    GB_HDR_YLUTLOW_16,
    GB_HDR_YLUTLOW_17,
    RESERVED_HDR_YLUT17,
    GB_HDR_YLUTLOW_18,
    GB_HDR_YLUTLOW_19,
    GB_HDR_YLUTLOW_1A,
    RESERVED_HDR_YLUT18,
    GB_HDR_YLUTLOW_1B,
    GB_HDR_YLUTLOW_1C,
    GB_HDR_YLUTLOW_1D,
    RESERVED_HDR_YLUT19,
    GB_HDR_YLUTLOW_1E,
    GB_HDR_YLUTLOW_1F,
    RESERVED_HDR_YLUT1A,
    VPP_GLOBAL_REG_ENUM_END
};
static inline int Vpp_Global_write_reg(Vpp_Global_regs * G_regs, int reg, int index , unsigned long long value)
{
    switch(reg)
    {
        case GB_SRC_SF_BASE:
            G_regs->reg_Src_Base.reg.Gb_Src_Sf_Base = (unsigned int)value;
            return 1;
        case GB_OTH_SF_BASE:
            G_regs->reg_Oth_Base.reg.Gb_Oth_Sf_Base = (unsigned int)value;
            return 1;
        case GB_REF_SF_BASE:
            G_regs->reg_Ref_Base.reg.Gb_Ref_Sf_Base = (unsigned int)value;
            return 1;
        case GB_SRC_RANGE_MAP_Y_EN:
            G_regs->reg_Base_Map.reg.Gb_Src_Range_Map_Y_En = (unsigned int)value;
            return 1;
        case GB_SRC_RANGE_MAP_Y:
            G_regs->reg_Base_Map.reg.Gb_Src_Range_Map_Y = (unsigned int)value;
            return 1;
        case GB_SRC_RANGE_MAP_UV_EN:
            G_regs->reg_Base_Map.reg.Gb_Src_Range_Map_Uv_En = (unsigned int)value;
            return 1;
        case GB_SRC_RANGE_MAP_UV:
            G_regs->reg_Base_Map.reg.Gb_Src_Range_Map_Uv = (unsigned int)value;
            return 1;
        case GB_OTH_RANGE_MAP_Y_EN:
            G_regs->reg_Base_Map.reg.Gb_Oth_Range_Map_Y_En = (unsigned int)value;
            return 1;
        case GB_OTH_RANGE_MAP_Y:
            G_regs->reg_Base_Map.reg.Gb_Oth_Range_Map_Y = (unsigned int)value;
            return 1;
        case GB_OTH_RANGE_MAP_UV_EN:
            G_regs->reg_Base_Map.reg.Gb_Oth_Range_Map_Uv_En = (unsigned int)value;
            return 1;
        case GB_OTH_RANGE_MAP_UV:
            G_regs->reg_Base_Map.reg.Gb_Oth_Range_Map_Uv = (unsigned int)value;
            return 1;
        case GB_REF_RANGE_MAP_Y_EN:
            G_regs->reg_Base_Map.reg.Gb_Ref_Range_Map_Y_En = (unsigned int)value;
            return 1;
        case GB_REF_RANGE_MAP_Y:
            G_regs->reg_Base_Map.reg.Gb_Ref_Range_Map_Y = (unsigned int)value;
            return 1;
        case GB_REF_RANGE_MAP_UV_EN:
            G_regs->reg_Base_Map.reg.Gb_Ref_Range_Map_Uv_En = (unsigned int)value;
            return 1;
        case GB_REF_RANGE_MAP_UV:
            G_regs->reg_Base_Map.reg.Gb_Ref_Range_Map_Uv = (unsigned int)value;
            return 1;
        case RESERVED_BM:
            G_regs->reg_Base_Map.reg.Reserved_Bm = (unsigned int)value;
            return 1;
        case GB_DST_SF_BASE:
            G_regs->reg_Dst_Base.reg.Gb_Dst_Sf_Base = (unsigned int)value;
            return 1;
        case GB_SCL_DST_SF_BASE:
            G_regs->reg_Scl_Dst_Base_0.reg.Gb_Scl_Dst_Sf_Base = (unsigned int)value;
            return 1;
        case GB_SCL_DST_CLIPX:
            G_regs->reg_Scl_Dst_Base_1.reg.Gb_Scl_Dst_Clipx = (unsigned int)value;
            return 1;
        case GB_SCL_DST_CLIPY:
            G_regs->reg_Scl_Dst_Base_1.reg.Gb_Scl_Dst_Clipy = (unsigned int)value;
            return 1;
        case RESERVED_MOB:
            G_regs->reg_Scl_Dst_Base_1.reg.Reserved_Mob = (unsigned int)value;
            return 1;
        case GB_SCL_DST_SF_HEIGHT:
            G_regs->reg_Scl_Dst_Height.reg.Gb_Scl_Dst_Sf_Height = (unsigned int)value;
            return 1;
        case RESERVED_SDB2:
            G_regs->reg_Scl_Dst_Height.reg.Reserved_Sdb2 = (unsigned int)value;
            return 1;
        case GB_SRC_SF_BOT_BASE:
            G_regs->reg_Bot_Base_0.reg.Gb_Src_Sf_Bot_Base = (unsigned int)value;
            return 1;
        case GB_OTH_SF_BOT_BASE:
            G_regs->reg_Bot_Base_1.reg.Gb_Oth_Sf_Bot_Base = (unsigned int)value;
            return 1;
        case GB_REF_SF_BOT_BASE:
            G_regs->reg_Bot_Base_2.reg.Gb_Ref_Sf_Bot_Base = (unsigned int)value;
            return 1;
        case GB_HDR_INT_BASE:
            G_regs->reg_Hdr_Lut_Base.reg.Gb_Hdr_Int_Base = (unsigned int)value;
            return 1;
        case GB_YSUM_MEM_BASE:
            G_regs->reg_Hdr_Ysum_Base.reg.Gb_Ysum_Mem_Base= (value>>0)&(((unsigned long long)1<<32)-1);
            G_regs->reg_Scl_Para_Error.reg.Gb_Ysum_Mem_Base= (value>>32)&((1<<2)-1);
            return 1;
        case GB_HS_ERROR:
            G_regs->reg_Scl_Para_Error.reg.Gb_Hs_Error = (unsigned int)value;
            return 1;
        case GB_VS_ERROR:
            G_regs->reg_Scl_Para_Error.reg.Gb_Vs_Error = (unsigned int)value;
            return 1;
        case GB_SRC_WIDTH:
            G_regs->reg_Src_Sf_0.reg.Gb_Src_Width = (unsigned int)value;
            return 1;
        case GB_SEQ_ID_THRD:
            G_regs->reg_Src_Sf_0.reg.Gb_Seq_Id_Thrd = (unsigned int)value;
            return 1;
        case RESERVED_SRC_SF_0:
            G_regs->reg_Src_Sf_0.reg.Reserved_Src_Sf_0 = (unsigned int)value;
            return 1;
        case RESERVED_SRC_SF_1:
            G_regs->reg_Src_Sf_1.reg.Reserved_Src_Sf_1 = (unsigned int)value;
            return 1;
        case GB_SRC_FMT:
            G_regs->reg_Src_Sf_1.reg.Gb_Src_Fmt = (unsigned int)value;
            return 1;
        case GB_SRC_TILE:
            G_regs->reg_Src_Sf_1.reg.Gb_Src_Tile = (unsigned int)value;
            return 1;
        case GB_SRC_HEIGHT:
            G_regs->reg_Src_Sf_1.reg.Gb_Src_Height = (unsigned int)value;
            return 1;
        case GB_SRC_PITCH:
            G_regs->reg_Src_Sf_1.reg.Gb_Src_Pitch = (unsigned int)value;
            return 1;
        case GB_SCL_DST_FMT:
            G_regs->reg_Scl_Dst_Sf_1.reg.Gb_Scl_Dst_Fmt = (unsigned int)value;
            return 1;
        case GB_SCL_DST_HEIGHT:
            G_regs->reg_Scl_Dst_Sf_1.reg.Gb_Scl_Dst_Height = (unsigned int)value;
            return 1;
        case GB_SCL_DST_PITCH:
            G_regs->reg_Scl_Dst_Sf_1.reg.Gb_Scl_Dst_Pitch = (unsigned int)value;
            return 1;
        case GB_SCL_DST_COMPRESS_EN:
            G_regs->reg_Scl_Dst_Sf_1.reg.Gb_Scl_Dst_Compress_En = (unsigned int)value;
            return 1;
        case RESERVED_SCL_DST_SF_1:
            G_regs->reg_Scl_Dst_Sf_1.reg.Reserved_Scl_Dst_Sf_1 = (unsigned int)value;
            return 1;
        case GB_SCL_DST_WIDTH:
            G_regs->reg_Scl_Dst_Sf_2.reg.Gb_Scl_Dst_Width = (unsigned int)value;
            return 1;
        case RESERVE_DST2:
            G_regs->reg_Scl_Dst_Sf_2.reg.Reserve_Dst2 = (unsigned int)value;
            return 1;
        case RESERVED_RS:
            G_regs->reg_Vpp_Ref_Sf.reg.Reserved_Rs = (unsigned int)value;
            return 1;
        case GB_VPP_DNREF_SF_PITCH:
            G_regs->reg_Vpp_Ref_Sf.reg.Gb_Vpp_Dnref_Sf_Pitch = (unsigned int)value;
            return 1;
        case GB_VPP_DNREF_SF_HEIGHT:
            G_regs->reg_Vpp_Ref_Sf.reg.Gb_Vpp_Dnref_Sf_Height = (unsigned int)value;
            return 1;
        case GB_BLT_MODE:
            G_regs->reg_Mode_Set.reg.Gb_Blt_Mode = (unsigned int)value;
            return 1;
        case GB_SCL_MODE:
            G_regs->reg_Mode_Set.reg.Gb_Scl_Mode = (unsigned int)value;
            return 1;
        case GB_EN_HSCL:
            G_regs->reg_Mode_Set.reg.Gb_En_Hscl = (unsigned int)value;
            return 1;
        case GB_EN_VSCL:
            G_regs->reg_Mode_Set.reg.Gb_En_Vscl = (unsigned int)value;
            return 1;
        case RESERVED_GB_FIRST_SLICE:
            G_regs->reg_Mode_Set.reg.Reserved_Gb_First_Slice = (unsigned int)value;
            return 1;
        case GB_SE_VPP:
            G_regs->reg_Mode_Set.reg.Gb_Se_Vpp = (unsigned int)value;
            return 1;
        case GB_ROTATION_MODE:
            G_regs->reg_Mode_Set.reg.Gb_Rotation_Mode = (unsigned int)value;
            return 1;
        case GB_SRC_COMPRESS:
            G_regs->reg_Mode_Set.reg.Gb_Src_Compress = (unsigned int)value;
            return 1;
        case GB_FOR_ECO:
            G_regs->reg_Mode_Set.reg.Gb_For_Eco = (unsigned int)value;
            return 1;
        case GB_X_UPSAMPLE_MODE:
            G_regs->reg_Mode_Set.reg.Gb_X_Upsample_Mode = (unsigned int)value;
            return 1;
        case GB_DS_24_X:
            G_regs->reg_Mode_Set.reg.Gb_Ds_24_X = (unsigned int)value;
            return 1;
        case GB_DS_24_Y:
            G_regs->reg_Mode_Set.reg.Gb_Ds_24_Y = (unsigned int)value;
            return 1;
        case GB_BLT_SIMPLE:
            G_regs->reg_Mode_Set.reg.Gb_Blt_Simple = (unsigned int)value;
            return 1;
        case GB_EN_4K_MEM_SWZL:
            G_regs->reg_Mode_Set.reg.Gb_En_4k_Mem_Swzl = (unsigned int)value;
            return 1;
        case GB_DS4_LSB2FORCE:
            G_regs->reg_Mode_Set.reg.Gb_Ds4_Lsb2force = (unsigned int)value;
            return 1;
        case GB_FIXCOLOR_MODE:
            G_regs->reg_Mode_Set.reg.Gb_Fixcolor_Mode = (unsigned int)value;
            return 1;
        case GB_Y_UPSAMPLE_MODE:
            G_regs->reg_Mode_Set.reg.Gb_Y_Upsample_Mode = (unsigned int)value;
            return 1;
        case RESERVED_MS:
            G_regs->reg_Mode_Set.reg.Reserved_Ms = (unsigned int)value;
            return 1;
        case RESERVED_GB_SCL_LEFT_EDGE_WIDTH:
            G_regs->reg_Drv_Mp_Set.reg.Reserved_Gb_Scl_Left_Edge_Width = (unsigned int)value;
            return 1;
        case RESERVED_GB_SCL_RIGHT_EDGE_WIDTH:
            G_regs->reg_Drv_Mp_Set.reg.Reserved_Gb_Scl_Right_Edge_Width = (unsigned int)value;
            return 1;
        case GB_LDRD_COMPRESS:
            G_regs->reg_Drv_Mp_Set.reg.Gb_Ldrd_Compress = (unsigned int)value;
            return 1;
        case GB_SEQ_ID_PEAKLEVEL:
            G_regs->reg_Drv_Mp_Set.reg.Gb_Seq_Id_Peaklevel = (unsigned int)value;
            return 1;
        case RESERVED_DMS:
            G_regs->reg_Drv_Mp_Set.reg.Reserved_Dms = (unsigned int)value;
            return 1;
        case GB_SLICE_WIDTH:
            G_regs->reg_Multipass_Set0.reg.Gb_Slice_Width = (unsigned int)value;
            return 1;
        case GB_SLICE_PIXEL_NUM:
            G_regs->reg_Multipass_Set0.reg.Gb_Slice_Pixel_Num = (unsigned int)value;
            return 1;
        case GB_SLICE_WIDTH_1ST:
            G_regs->reg_Multipass_Set0.reg.Gb_Slice_Width_1st= (value>>0)&((1<<4)-1);
            G_regs->reg_Multipass_Set1.reg.Gb_Slice_Width_1st= (value>>4)&((1<<10)-1);
            return 1;
        case GB_SLICE_PIXEL_NUM_1ST:
            G_regs->reg_Multipass_Set1.reg.Gb_Slice_Pixel_Num_1st = (unsigned int)value;
            return 1;
        case GB_SIGNAT_SEL_IO:
            G_regs->reg_Multipass_Set1.reg.Gb_Signat_Sel_Io = (unsigned int)value;
            return 1;
        case GB_SIGNAT_SEL_OO:
            G_regs->reg_Multipass_Set1.reg.Gb_Signat_Sel_Oo = (unsigned int)value;
            return 1;
        case GB_SIG_ENADDR_IO:
            G_regs->reg_Multipass_Set1.reg.Gb_Sig_Enaddr_Io = (unsigned int)value;
            return 1;
        case RESERVED_MP1:
            G_regs->reg_Multipass_Set1.reg.Reserved_Mp1 = (unsigned int)value;
            return 1;
        case GB_CL_LEFT:
            G_regs->reg_Clip_L_R.reg.Gb_Cl_Left = (unsigned int)value;
            return 1;
        case GB_CL_RIGHT:
            G_regs->reg_Clip_L_R.reg.Gb_Cl_Right = (unsigned int)value;
            return 1;
        case RESERVED17:
            G_regs->reg_Clip_L_R.reg.Reserved17 = (unsigned int)value;
            return 1;
        case GB_CL_TOP:
            G_regs->reg_Clip_T_B.reg.Gb_Cl_Top = (unsigned int)value;
            return 1;
        case GB_CL_BOTTOM:
            G_regs->reg_Clip_T_B.reg.Gb_Cl_Bottom = (unsigned int)value;
            return 1;
        case RESERVED16:
            G_regs->reg_Clip_T_B.reg.Reserved16 = (unsigned int)value;
            return 1;
        case GB_VPPCNTOUT:
            G_regs->reg_Lbuf_Set0.reg.Gb_Vppcntout = (unsigned int)value;
            return 1;
        case GB_VPPARBMODE:
            G_regs->reg_Lbuf_Set0.reg.Gb_Vpparbmode = (unsigned int)value;
            return 1;
        case GB_LB_SCL_YUV_OFFSET:
            G_regs->reg_Lbuf_Set0.reg.Gb_Lb_Scl_Yuv_Offset = (unsigned int)value;
            return 1;
        case GB_LB_SCL_A_OFFSET:
            G_regs->reg_Lbuf_Set0.reg.Gb_Lb_Scl_A_Offset = (unsigned int)value;
            return 1;
        case RESERVED_LS0:
            G_regs->reg_Lbuf_Set0.reg.Reserved_Ls0 = (unsigned int)value;
            return 1;
        case GB_LB_SCL_YUV_PITCH:
            G_regs->reg_Lbuf_Set0_1.reg.Gb_Lb_Scl_Yuv_Pitch = (unsigned int)value;
            return 1;
        case GB_LB_SCL_A_PITCH:
            G_regs->reg_Lbuf_Set0_1.reg.Gb_Lb_Scl_A_Pitch = (unsigned int)value;
            return 1;
        case GB_LB_NM_OPER_OFFSET:
            G_regs->reg_Lbuf_Set0_1.reg.Gb_Lb_Nm_Oper_Offset = (unsigned int)value;
            return 1;
        case RESERVED_LS01:
            G_regs->reg_Lbuf_Set0_1.reg.Reserved_Ls01 = (unsigned int)value;
            return 1;
        case GB_LB_SR_Y_OFFSET:
            G_regs->reg_Lbuf_Set0_2.reg.Gb_Lb_Sr_Y_Offset = (unsigned int)value;
            return 1;
        case GB_LB_SR_UV_OFFSET:
            G_regs->reg_Lbuf_Set0_2.reg.Gb_Lb_Sr_Uv_Offset = (unsigned int)value;
            return 1;
        case GB_LB_SR_Y_PITCH:
            G_regs->reg_Lbuf_Set0_2.reg.Gb_Lb_Sr_Y_Pitch = (unsigned int)value;
            return 1;
        case GB_LB_SR_A_OFFSET:
            G_regs->reg_Lbuf_Set0_3.reg.Gb_Lb_Sr_A_Offset = (unsigned int)value;
            return 1;
        case GB_LB_SR_UV_PITCH:
            G_regs->reg_Lbuf_Set0_3.reg.Gb_Lb_Sr_Uv_Pitch = (unsigned int)value;
            return 1;
        case GB_LB_SR_A_PITCH:
            G_regs->reg_Lbuf_Set0_3.reg.Gb_Lb_Sr_A_Pitch = (unsigned int)value;
            return 1;
        case RESERVED_LS03:
            G_regs->reg_Lbuf_Set0_3.reg.Reserved_Ls03 = (unsigned int)value;
            return 1;
        case GB_LBADIW:
            G_regs->reg_Lbuf_Set1.reg.Gb_Lbadiw = (unsigned int)value;
            return 1;
        case GB_LBASCLYUVW:
            G_regs->reg_Lbuf_Set1.reg.Gb_Lbasclyuvw = (unsigned int)value;
            return 1;
        case GB_LBASCLAW:
            G_regs->reg_Lbuf_Set1.reg.Gb_Lbasclaw = (unsigned int)value;
            return 1;
        case GB_LBASRUVW:
            G_regs->reg_Lbuf_Set1.reg.Gb_Lbasruvw = (unsigned int)value;
            return 1;
        case GB_LBA_BANK_MODE:
            G_regs->reg_Lbuf_Set1.reg.Gb_Lba_Bank_Mode = (unsigned int)value;
            return 1;
        case RESERVED_LS1:
            G_regs->reg_Lbuf_Set1.reg.Reserved_Ls1 = (unsigned int)value;
            return 1;
        case GB_LBASRYW:
            G_regs->reg_Lbuf_Set2.reg.Gb_Lbasryw = (unsigned int)value;
            return 1;
        case GB_LBASRAW:
            G_regs->reg_Lbuf_Set2.reg.Gb_Lbasraw = (unsigned int)value;
            return 1;
        case RESERVED_LS2:
            G_regs->reg_Lbuf_Set2.reg.Reserved_Ls2 = (unsigned int)value;
            return 1;
        case GB_LBADIR:
            G_regs->reg_Lbuf_Set3.reg.Gb_Lbadir = (unsigned int)value;
            return 1;
        case GB_LBADIREFR:
            G_regs->reg_Lbuf_Set3.reg.Gb_Lbadirefr = (unsigned int)value;
            return 1;
        case GB_LBASCRR:
            G_regs->reg_Lbuf_Set3.reg.Gb_Lbascrr = (unsigned int)value;
            return 1;
        case RESERVED_LS3:
            G_regs->reg_Lbuf_Set3.reg.Reserved_Ls3 = (unsigned int)value;
            return 1;
        case GB_LBASCLYUVR:
            G_regs->reg_Lbuf_Set4.reg.Gb_Lbasclyuvr = (unsigned int)value;
            return 1;
        case GB_LBASCLAR:
            G_regs->reg_Lbuf_Set4.reg.Gb_Lbasclar = (unsigned int)value;
            return 1;
        case GB_LBASRUVR:
            G_regs->reg_Lbuf_Set4.reg.Gb_Lbasruvr = (unsigned int)value;
            return 1;
        case RESERVED_LS4:
            G_regs->reg_Lbuf_Set4.reg.Reserved_Ls4 = (unsigned int)value;
            return 1;
        case GB_LBASRYR:
            G_regs->reg_Lbuf_Set5.reg.Gb_Lbasryr = (unsigned int)value;
            return 1;
        case GB_LBASRAR:
            G_regs->reg_Lbuf_Set5.reg.Gb_Lbasrar = (unsigned int)value;
            return 1;
        case RESERVED_LS5:
            G_regs->reg_Lbuf_Set5.reg.Reserved_Ls5 = (unsigned int)value;
            return 1;
        case GB_MOB_FIXALPHA:
            G_regs->reg_Img_Related.reg.Gb_Mob_Fixalpha = (unsigned int)value;
            return 1;
        case GB_REDUNDANT_EDGE_WIDTH:
            G_regs->reg_Img_Related.reg.Gb_Redundant_Edge_Width = (unsigned int)value;
            return 1;
        case GB_MOB_RANGE_MIN:
            G_regs->reg_Img_Related.reg.Gb_Mob_Range_Min = (unsigned int)value;
            return 1;
        case GB_MOB_RANGE_MAXRY:
            G_regs->reg_Img_Related.reg.Gb_Mob_Range_Maxry = (unsigned int)value;
            return 1;
        case GB_MOB_RANGE_MAXGBUV:
            G_regs->reg_Img_Related.reg.Gb_Mob_Range_Maxgbuv = (unsigned int)value;
            return 1;
        case GB_SRC_LOSSY_COM_EN:
            G_regs->reg_Img_Related.reg.Gb_Src_Lossy_Com_En = (unsigned int)value;
            return 1;
        case GB_DST_LOSSY_COM_EN:
            G_regs->reg_Img_Related.reg.Gb_Dst_Lossy_Com_En = (unsigned int)value;
            return 1;
        case GB_MOB_BLENDING_EN:
            G_regs->reg_Img_Related.reg.Gb_Mob_Blending_En = (unsigned int)value;
            return 1;
        case GB_MOB_BLENDING_DIU_MODE:
            G_regs->reg_Img_Related.reg.Gb_Mob_Blending_Diu_Mode = (unsigned int)value;
            return 1;
        case GB_MOB_BLENDING_STREAM_MODE:
            G_regs->reg_Img_Related.reg.Gb_Mob_Blending_Stream_Mode = (unsigned int)value;
            return 1;
        case RESERVED_IR:
            G_regs->reg_Img_Related.reg.Reserved_Ir = (unsigned int)value;
            return 1;
        case GB_HSLC_Y_A0:
            G_regs->reg_Hslc_Set0.reg.Gb_Hslc_Y_A0 = (unsigned int)value;
            return 1;
        case GB_HSLC_Y_B0:
            G_regs->reg_Hslc_Set0.reg.Gb_Hslc_Y_B0 = (unsigned int)value;
            return 1;
        case RESERVED_HSLC0:
            G_regs->reg_Hslc_Set0.reg.Reserved_Hslc0 = (unsigned int)value;
            return 1;
        case GB_HSLC_UV_A0:
            G_regs->reg_Hslc_Set1.reg.Gb_Hslc_Uv_A0 = (unsigned int)value;
            return 1;
        case GB_HSLC_UV_A1:
            G_regs->reg_Hslc_Set1.reg.Gb_Hslc_Uv_A1 = (unsigned int)value;
            return 1;
        case RESERVED_HSLC1:
            G_regs->reg_Hslc_Set1.reg.Reserved_Hslc1 = (unsigned int)value;
            return 1;
        case GB_HSLC_U_B0:
            G_regs->reg_Hslc_Set2.reg.Gb_Hslc_U_B0 = (unsigned int)value;
            return 1;
        case GB_HSLC_V_B0:
            G_regs->reg_Hslc_Set2.reg.Gb_Hslc_V_B0 = (unsigned int)value;
            return 1;
        case RESERVED_HSLC2:
            G_regs->reg_Hslc_Set2.reg.Reserved_Hslc2 = (unsigned int)value;
            return 1;
        case GB_SD_WIDTH_RATIO:
            G_regs->reg_Scl_Para_Sd.reg.Gb_Sd_Width_Ratio = (unsigned int)value;
            return 1;
        case GB_SD_HEIGHT_RATIO:
            G_regs->reg_Scl_Para_Sd.reg.Gb_Sd_Height_Ratio = (unsigned int)value;
            return 1;
        case GB_DS_WIDTH_RATIO:
            G_regs->reg_Scl_Para_Ds.reg.Gb_Ds_Width_Ratio = (unsigned int)value;
            return 1;
        case GB_DS_HEIGHT_RATIO:
            G_regs->reg_Scl_Para_Ds.reg.Gb_Ds_Height_Ratio = (unsigned int)value;
            return 1;
        case RESERVED_DS:
            G_regs->reg_Scl_Para_Ds.reg.Reserved_Ds = (unsigned int)value;
            return 1;
        case GB_SRC_EN_ALPHA:
            G_regs->reg_Scl_0.reg.Gb_Src_En_Alpha = (unsigned int)value;
            return 1;
        case GB_SCL_BC_DELTA:
            G_regs->reg_Scl_0.reg.Gb_Scl_Bc_Delta = (unsigned int)value;
            return 1;
        case GB_EN_SCL_MAXMIN:
            G_regs->reg_Scl_0.reg.Gb_En_Scl_Maxmin = (unsigned int)value;
            return 1;
        case GB_EN_SR:
            G_regs->reg_Scl_0.reg.Gb_En_Sr = (unsigned int)value;
            return 1;
        case GB_SR_CTDIFFTH:
            G_regs->reg_Scl_0.reg.Gb_Sr_Ctdiffth = (unsigned int)value;
            return 1;
        case GB_EN_PIXELTYPE:
            G_regs->reg_Scl_0.reg.Gb_En_Pixeltype = (unsigned int)value;
            return 1;
        case GB_SR_BLENDSTRENGTH:
            G_regs->reg_Scl_0.reg.Gb_Sr_Blendstrength = (unsigned int)value;
            return 1;
        case GB_SR_ENCTBLEND:
            G_regs->reg_Scl_0.reg.Gb_Sr_Enctblend = (unsigned int)value;
            return 1;
        case GB_SR_BLENDWEIGHT:
            G_regs->reg_Scl_0.reg.Gb_Sr_Blendweight = (unsigned int)value;
            return 1;
        case GB_SR_COEFWEIGHT:
            G_regs->reg_Scl_0.reg.Gb_Sr_Coefweight = (unsigned int)value;
            return 1;
        case GB_SR_COEFWEIGHTOTHER:
            G_regs->reg_Scl_0.reg.Gb_Sr_Coefweightother = (unsigned int)value;
            return 1;
        case RESERVED_SCL0:
            G_regs->reg_Scl_0.reg.Reserved_Scl0 = (unsigned int)value;
            return 1;
        case GB_CSCF_M_00:
            G_regs->reg_Cscf_00_01.reg.Gb_Cscf_M_00 = (unsigned int)value;
            return 1;
        case GB_CSCF_M_01:
            G_regs->reg_Cscf_00_01.reg.Gb_Cscf_M_01 = (unsigned int)value;
            return 1;
        case GB_CSCF_EN:
            G_regs->reg_Cscf_00_01.reg.Gb_Cscf_En = (unsigned int)value;
            return 1;
        case GB_CSCF_ROUND:
            G_regs->reg_Cscf_00_01.reg.Gb_Cscf_Round = (unsigned int)value;
            return 1;
        case RESERVED_CSCF_00_01:
            G_regs->reg_Cscf_00_01.reg.Reserved_Cscf_00_01 = (unsigned int)value;
            return 1;
        case GB_CSCF_M_02:
            G_regs->reg_Cscf_02_10.reg.Gb_Cscf_M_02 = (unsigned int)value;
            return 1;
        case GB_CSCF_M_10:
            G_regs->reg_Cscf_02_10.reg.Gb_Cscf_M_10 = (unsigned int)value;
            return 1;
        case RESERVED_CSCF_02_10:
            G_regs->reg_Cscf_02_10.reg.Reserved_Cscf_02_10 = (unsigned int)value;
            return 1;
        case GB_CSCF_M_11:
            G_regs->reg_Cscf_11_12.reg.Gb_Cscf_M_11 = (unsigned int)value;
            return 1;
        case GB_CSCF_M_12:
            G_regs->reg_Cscf_11_12.reg.Gb_Cscf_M_12 = (unsigned int)value;
            return 1;
        case RESERVED_CSCF_11_12:
            G_regs->reg_Cscf_11_12.reg.Reserved_Cscf_11_12 = (unsigned int)value;
            return 1;
        case GB_CSCF_M_20:
            G_regs->reg_Cscf_20_21.reg.Gb_Cscf_M_20 = (unsigned int)value;
            return 1;
        case GB_CSCF_M_21:
            G_regs->reg_Cscf_20_21.reg.Gb_Cscf_M_21 = (unsigned int)value;
            return 1;
        case RESERVED_CSCF_20_21:
            G_regs->reg_Cscf_20_21.reg.Reserved_Cscf_20_21 = (unsigned int)value;
            return 1;
        case GB_CSCF_M_22:
            G_regs->reg_Cscf_22.reg.Gb_Cscf_M_22 = (unsigned int)value;
            return 1;
        case RESERVED_CSCF_22:
            G_regs->reg_Cscf_22.reg.Reserved_Cscf_22 = (unsigned int)value;
            return 1;
        case GB_CSCF_OFF_00:
            G_regs->reg_Cscf_Off_00.reg.Gb_Cscf_Off_00 = (unsigned int)value;
            return 1;
        case RESERVED_CSCF_OFF_00:
            G_regs->reg_Cscf_Off_00.reg.Reserved_Cscf_Off_00 = (unsigned int)value;
            return 1;
        case GB_CSCF_OFF_01:
            G_regs->reg_Cscf_Off_01.reg.Gb_Cscf_Off_01 = (unsigned int)value;
            return 1;
        case RESERVED_CSCF_OFF_01:
            G_regs->reg_Cscf_Off_01.reg.Reserved_Cscf_Off_01 = (unsigned int)value;
            return 1;
        case GB_CSCF_OFF_02:
            G_regs->reg_Cscf_Off_02.reg.Gb_Cscf_Off_02 = (unsigned int)value;
            return 1;
        case RESERVED_CSCF_OFF_02:
            G_regs->reg_Cscf_Off_02.reg.Reserved_Cscf_Off_02 = (unsigned int)value;
            return 1;
        case GB_CSCB_M_00:
            G_regs->reg_Cscb_00_01.reg.Gb_Cscb_M_00 = (unsigned int)value;
            return 1;
        case GB_CSCB_M_01:
            G_regs->reg_Cscb_00_01.reg.Gb_Cscb_M_01 = (unsigned int)value;
            return 1;
        case GB_CSCB_EN:
            G_regs->reg_Cscb_00_01.reg.Gb_Cscb_En = (unsigned int)value;
            return 1;
        case GB_CSCB_ROUND:
            G_regs->reg_Cscb_00_01.reg.Gb_Cscb_Round = (unsigned int)value;
            return 1;
        case RESERVED_CSCB_00_01:
            G_regs->reg_Cscb_00_01.reg.Reserved_Cscb_00_01 = (unsigned int)value;
            return 1;
        case GB_CSCB_M_02:
            G_regs->reg_Cscb_02_10.reg.Gb_Cscb_M_02 = (unsigned int)value;
            return 1;
        case GB_CSCB_M_10:
            G_regs->reg_Cscb_02_10.reg.Gb_Cscb_M_10 = (unsigned int)value;
            return 1;
        case RESERVED_CSCB_02_10:
            G_regs->reg_Cscb_02_10.reg.Reserved_Cscb_02_10 = (unsigned int)value;
            return 1;
        case GB_CSCB_M_11:
            G_regs->reg_Cscb_11_12.reg.Gb_Cscb_M_11 = (unsigned int)value;
            return 1;
        case GB_CSCB_M_12:
            G_regs->reg_Cscb_11_12.reg.Gb_Cscb_M_12 = (unsigned int)value;
            return 1;
        case RESERVED_CSCB_11_12:
            G_regs->reg_Cscb_11_12.reg.Reserved_Cscb_11_12 = (unsigned int)value;
            return 1;
        case GB_CSCB_M_20:
            G_regs->reg_Cscb_20_21.reg.Gb_Cscb_M_20 = (unsigned int)value;
            return 1;
        case GB_CSCB_M_21:
            G_regs->reg_Cscb_20_21.reg.Gb_Cscb_M_21 = (unsigned int)value;
            return 1;
        case RESERVED_CSCB_20_21:
            G_regs->reg_Cscb_20_21.reg.Reserved_Cscb_20_21 = (unsigned int)value;
            return 1;
        case GB_CSCB_M_22:
            G_regs->reg_Cscb_22.reg.Gb_Cscb_M_22 = (unsigned int)value;
            return 1;
        case RESERVED_CSCB_22:
            G_regs->reg_Cscb_22.reg.Reserved_Cscb_22 = (unsigned int)value;
            return 1;
        case GB_CSCB_OFF_00:
            G_regs->reg_Cscb_Off_00.reg.Gb_Cscb_Off_00 = (unsigned int)value;
            return 1;
        case RESERVED_CSCB_OFF_00:
            G_regs->reg_Cscb_Off_00.reg.Reserved_Cscb_Off_00 = (unsigned int)value;
            return 1;
        case GB_CSCB_OFF_01:
            G_regs->reg_Cscb_Off_01.reg.Gb_Cscb_Off_01 = (unsigned int)value;
            return 1;
        case RESERVED_CSCB_OFF_01:
            G_regs->reg_Cscb_Off_01.reg.Reserved_Cscb_Off_01 = (unsigned int)value;
            return 1;
        case GB_CSCB_OFF_02:
            G_regs->reg_Cscb_Off_02.reg.Gb_Cscb_Off_02 = (unsigned int)value;
            return 1;
        case RESERVED_CSCB_OFF_02:
            G_regs->reg_Cscb_Off_02.reg.Reserved_Cscb_Off_02 = (unsigned int)value;
            return 1;
        case GB_FIXCOLOR_VALUE:
            G_regs->reg_Ld_Bld_Fixcolor.reg.Gb_Fixcolor_Value = (unsigned int)value;
            return 1;
        case GB_LB_CPT_OFFSET:
            G_regs->reg_Cpt_0.reg.Gb_Lb_Cpt_Offset = (unsigned int)value;
            return 1;
        case GB_LB_CPT_SIZE:
            G_regs->reg_Cpt_0.reg.Gb_Lb_Cpt_Size = (unsigned int)value;
            return 1;
        case GB_LB_MOB_L2T_PITCH:
            G_regs->reg_Cpt_0.reg.Gb_Lb_Mob_L2t_Pitch = (unsigned int)value;
            return 1;
        case GB_DST_END:
            G_regs->reg_Cpt_1.reg.Gb_Dst_End = (unsigned int)value;
            return 1;
        case GB_DST_TILE:
            G_regs->reg_Cpt_1.reg.Gb_Dst_Tile = (unsigned int)value;
            return 1;
        case GB_LB_MOB_L2T_OFFSET:
            G_regs->reg_Cpt_1.reg.Gb_Lb_Mob_L2t_Offset = (unsigned int)value;
            return 1;
        case GB_MOB_DST_ALPHAMODE:
            G_regs->reg_Cpt_1.reg.Gb_Mob_Dst_Alphamode = (unsigned int)value;
            return 1;
        case GB_MOB_BLENDING_MODE:
            G_regs->reg_Cpt_1.reg.Gb_Mob_Blending_Mode = (unsigned int)value;
            return 1;
        case GB_MOB_BLENDING_METHOD:
            G_regs->reg_Cpt_1.reg.Gb_Mob_Blending_Method = (unsigned int)value;
            return 1;
        case GB_MOB_BLENDING_ROUNDING:
            G_regs->reg_Cpt_1.reg.Gb_Mob_Blending_Rounding = (unsigned int)value;
            return 1;
        case GB_CPT_DNSAMPLE_MODE:
            G_regs->reg_Cpt_2.reg.Gb_Cpt_Dnsample_Mode = (unsigned int)value;
            return 1;
        case GB_MOB_BLDALPHA:
            G_regs->reg_Cpt_2.reg.Gb_Mob_Bldalpha = (unsigned int)value;
            return 1;
        case GB_CPT_EN:
            G_regs->reg_Cpt_2.reg.Gb_Cpt_En = (unsigned int)value;
            return 1;
        case GB_DST_SF_WIDTH:
            G_regs->reg_Cpt_2.reg.Gb_Dst_Sf_Width = (unsigned int)value;
            return 1;
        case RESERVED_CPT2:
            G_regs->reg_Cpt_2.reg.Reserved_Cpt2 = (unsigned int)value;
            return 1;
        case GB_MIFARB_REG:
            G_regs->reg_Mif_Arbiter_0.reg.Gb_Mifarb_Reg = (unsigned int)value;
            return 1;
        case GB_MIFARB_YDIFF:
            G_regs->reg_Mif_Arbiter_0.reg.Gb_Mifarb_Ydiff = (unsigned int)value;
            return 1;
        case GB_MIFARB_LD:
            G_regs->reg_Mif_Arbiter_0.reg.Gb_Mifarb_Ld = (unsigned int)value;
            return 1;
        case RESERVED_MA0:
            G_regs->reg_Mif_Arbiter_0.reg.Reserved_Ma0 = (unsigned int)value;
            return 1;
        case GB_MIFARB_CPTR:
            G_regs->reg_Mif_Arbiter_1.reg.Gb_Mifarb_Cptr = (unsigned int)value;
            return 1;
        case GB_MIFARB_MTDR:
            G_regs->reg_Mif_Arbiter_1.reg.Gb_Mifarb_Mtdr = (unsigned int)value;
            return 1;
        case GB_MIFARB_MOB:
            G_regs->reg_Mif_Arbiter_1.reg.Gb_Mifarb_Mob = (unsigned int)value;
            return 1;
        case GB_MIFARB_PASSIVE:
            G_regs->reg_Mif_Arbiter_1.reg.Gb_Mifarb_Passive = (unsigned int)value;
            return 1;
        case RESERVED_MA1:
            G_regs->reg_Mif_Arbiter_1.reg.Reserved_Ma1 = (unsigned int)value;
            return 1;
        case GB_MIFARB_MTDW:
            G_regs->reg_Mif_Arbiter_2.reg.Gb_Mifarb_Mtdw = (unsigned int)value;
            return 1;
        case GB_MIFARB_HDRR:
            G_regs->reg_Mif_Arbiter_2.reg.Gb_Mifarb_Hdrr = (unsigned int)value;
            return 1;
        case GB_MIFARB_HDRW:
            G_regs->reg_Mif_Arbiter_2.reg.Gb_Mifarb_Hdrw = (unsigned int)value;
            return 1;
        case RESERVED_MA2:
            G_regs->reg_Mif_Arbiter_2.reg.Reserved_Ma2 = (unsigned int)value;
            return 1;
        case GB_SRC_BL_SLOT_IDX:
            G_regs->reg_Mif_Slot_Idx_0.reg.Gb_Src_Bl_Slot_Idx = (unsigned int)value;
            return 1;
        case GB_OTH_BL_SLOT_IDX:
            G_regs->reg_Mif_Slot_Idx_0.reg.Gb_Oth_Bl_Slot_Idx= (value>>0)&((1<<14)-1);
            G_regs->reg_Mif_Slot_Idx_1.reg.Gb_Oth_Bl_Slot_Idx= (value>>14)&((1<<4)-1);
            return 1;
        case GB_REF_BL_SLOT_IDX:
            G_regs->reg_Mif_Slot_Idx_1.reg.Gb_Ref_Bl_Slot_Idx = (unsigned int)value;
            return 1;
        case GB_DST_BL_SLOT_IDX:
            G_regs->reg_Mif_Slot_Idx_1.reg.Gb_Dst_Bl_Slot_Idx= (value>>0)&((1<<10)-1);
            G_regs->reg_Mif_Slot_Idx_2.reg.Gb_Dst_Bl_Slot_Idx= (value>>10)&((1<<8)-1);
            return 1;
        case GB_SCL_DST_BL_SLOT_IDX:
            G_regs->reg_Mif_Slot_Idx_2.reg.Gb_Scl_Dst_Bl_Slot_Idx = (unsigned int)value;
            return 1;
        case RESERVED_MSI2:
            G_regs->reg_Mif_Slot_Idx_2.reg.Reserved_Msi2 = (unsigned int)value;
            return 1;
        case GB_SRC_BOT_BL_SLOT_IDX:
            G_regs->reg_Mif_Slot_Idx_3.reg.Gb_Src_Bot_Bl_Slot_Idx = (unsigned int)value;
            return 1;
        case GB_OTH_BOT_BL_SLOT_IDX:
            G_regs->reg_Mif_Slot_Idx_3.reg.Gb_Oth_Bot_Bl_Slot_Idx= (value>>0)&((1<<14)-1);
            G_regs->reg_Mif_Slot_Idx_4.reg.Gb_Oth_Bot_Bl_Slot_Idx= (value>>14)&((1<<4)-1);
            return 1;
        case GB_REF_BOT_BL_SLOT_IDX:
            G_regs->reg_Mif_Slot_Idx_4.reg.Gb_Ref_Bot_Bl_Slot_Idx = (unsigned int)value;
            return 1;
        case RESERVED_MSI4:
            G_regs->reg_Mif_Slot_Idx_4.reg.Reserved_Msi4 = (unsigned int)value;
            return 1;
        case GB_SRC_PROC_ID:
            G_regs->reg_Mif_Pid_0.reg.Gb_Src_Proc_Id = (unsigned int)value;
            return 1;
        case GB_OTH_PROC_ID:
            G_regs->reg_Mif_Pid_0.reg.Gb_Oth_Proc_Id = (unsigned int)value;
            return 1;
        case GB_REF_PROC_ID:
            G_regs->reg_Mif_Pid_0.reg.Gb_Ref_Proc_Id = (unsigned int)value;
            return 1;
        case GB_SRC_BOT_PROC_ID:
            G_regs->reg_Mif_Pid_0.reg.Gb_Src_Bot_Proc_Id = (unsigned int)value;
            return 1;
        case GB_OTH_BOT_PROC_ID:
            G_regs->reg_Mif_Pid_0.reg.Gb_Oth_Bot_Proc_Id = (unsigned int)value;
            return 1;
        case GB_REF_BOT_PROC_ID:
            G_regs->reg_Mif_Pid_0.reg.Gb_Ref_Bot_Proc_Id = (unsigned int)value;
            return 1;
        case GB_DST_PROC_ID:
            G_regs->reg_Mif_Pid_0.reg.Gb_Dst_Proc_Id = (unsigned int)value;
            return 1;
        case GB_SCL_DST_PROC_ID:
            G_regs->reg_Mif_Pid_0.reg.Gb_Scl_Dst_Proc_Id = (unsigned int)value;
            return 1;
        case GB_MTD_HIST_PROC_ID:
            G_regs->reg_Mif_Pid_1.reg.Gb_Mtd_Hist_Proc_Id = (unsigned int)value;
            return 1;
        case GB_HDR_YSUM_PROC_ID:
            G_regs->reg_Mif_Pid_1.reg.Gb_Hdr_Ysum_Proc_Id = (unsigned int)value;
            return 1;
        case GB_HDR_INT_PROC_ID:
            G_regs->reg_Mif_Pid_1.reg.Gb_Hdr_Int_Proc_Id = (unsigned int)value;
            return 1;
        case RESERVED_MPID1:
            G_regs->reg_Mif_Pid_1.reg.Reserved_Mpid1 = (unsigned int)value;
            return 1;
        case GB_DI_INTRA_TH:
            G_regs->reg_Di_0.reg.Gb_Di_Intra_Th = (unsigned int)value;
            return 1;
        case GB_DI_ISFEATHER_TH:
            G_regs->reg_Di_0.reg.Gb_Di_Isfeather_Th = (unsigned int)value;
            return 1;
        case GB_DI_FORCE_EELA_TH:
            G_regs->reg_Di_0.reg.Gb_Di_Force_Eela_Th = (unsigned int)value;
            return 1;
        case RESERVED_DI0:
            G_regs->reg_Di_0.reg.Reserved_Di0 = (unsigned int)value;
            return 1;
        case GB_DI_SCROLL_TH:
            G_regs->reg_Di_1.reg.Gb_Di_Scroll_Th = (unsigned int)value;
            return 1;
        case GB_POINT_DEGREE_VAR_K:
            G_regs->reg_Di_1.reg.Gb_Point_Degree_Var_K = (unsigned int)value;
            return 1;
        case RESERVED_DI1:
            G_regs->reg_Di_1.reg.Reserved_Di1 = (unsigned int)value;
            return 1;
        case GB_SOBEL_TH:
            G_regs->reg_Di_2.reg.Gb_Sobel_Th = (unsigned int)value;
            return 1;
        case GB_SOBEL_FORCE_VALUE:
            G_regs->reg_Di_2.reg.Gb_Sobel_Force_Value = (unsigned int)value;
            return 1;
        case GB_DI_FEATHER_EDGE_CNT_X1:
            G_regs->reg_Di_2.reg.Gb_Di_Feather_Edge_Cnt_X1 = (unsigned int)value;
            return 1;
        case GB_DI_FEATHER_EDGE_CNT_Y1:
            G_regs->reg_Di_2.reg.Gb_Di_Feather_Edge_Cnt_Y1 = (unsigned int)value;
            return 1;
        case RESERVED_DI2:
            G_regs->reg_Di_2.reg.Reserved_Di2 = (unsigned int)value;
            return 1;
        case GB_DI_IAD_MD_X1:
            G_regs->reg_Di_3.reg.Gb_Di_Iad_Md_X1 = (unsigned int)value;
            return 1;
        case GB_DI_IAD_MD_X2:
            G_regs->reg_Di_3.reg.Gb_Di_Iad_Md_X2 = (unsigned int)value;
            return 1;
        case GB_DI_IAD_MD_Y1:
            G_regs->reg_Di_4.reg.Gb_Di_Iad_Md_Y1 = (unsigned int)value;
            return 1;
        case GB_DI_IAD_MD_K:
            G_regs->reg_Di_4.reg.Gb_Di_Iad_Md_K = (unsigned int)value;
            return 1;
        case RESERVED_DI4:
            G_regs->reg_Di_4.reg.Reserved_Di4 = (unsigned int)value;
            return 1;
        case GB_POINT_DEGREE_VAR_X1:
            G_regs->reg_Di_5.reg.Gb_Point_Degree_Var_X1 = (unsigned int)value;
            return 1;
        case GB_POINT_DEGREE_VAR_X2:
            G_regs->reg_Di_5.reg.Gb_Point_Degree_Var_X2 = (unsigned int)value;
            return 1;
        case GB_POINT_DEGREE_VAR_Y1:
            G_regs->reg_Di_5.reg.Gb_Point_Degree_Var_Y1 = (unsigned int)value;
            return 1;
        case GB_TOP_BOT_FIELD_DIV:
            G_regs->reg_Di_6.reg.Gb_Top_Bot_Field_Div = (unsigned int)value;
            return 1;
        case GB_DI_ENABLE_STATICFLAG:
            G_regs->reg_Di_6.reg.Gb_Di_Enable_Staticflag = (unsigned int)value;
            return 1;
        case GB_DI_FEATHER_EDGE_CNT_K:
            G_regs->reg_Di_6.reg.Gb_Di_Feather_Edge_Cnt_K = (unsigned int)value;
            return 1;
        case GB_DI_FEATHER_EDGE_CNT_X2:
            G_regs->reg_Di_6.reg.Gb_Di_Feather_Edge_Cnt_X2 = (unsigned int)value;
            return 1;
        case GB_FD_WAIT_FRAME:
            G_regs->reg_Di_6.reg.Gb_Fd_Wait_Frame = (unsigned int)value;
            return 1;
        case RESERVED_DI6:
            G_regs->reg_Di_6.reg.Reserved_Di6 = (unsigned int)value;
            return 1;
        case GB_EDGE_FLAG_TH:
            G_regs->reg_Di_7.reg.Gb_Edge_Flag_Th = (unsigned int)value;
            return 1;
        case GB_DI_INTRA_DIFF_TH:
            G_regs->reg_Di_7.reg.Gb_Di_Intra_Diff_Th = (unsigned int)value;
            return 1;
        case RESERVED_DI7:
            G_regs->reg_Di_7.reg.Reserved_Di7 = (unsigned int)value;
            return 1;
        case GB_USE_MV_JUDGE:
            G_regs->reg_Di_8.reg.Gb_Use_Mv_Judge = (unsigned int)value;
            return 1;
        case GB_USE_ALLMV:
            G_regs->reg_Di_8.reg.Gb_Use_Allmv = (unsigned int)value;
            return 1;
        case GB_MV_DIFF_TH:
            G_regs->reg_Di_8.reg.Gb_Mv_Diff_Th = (unsigned int)value;
            return 1;
        case GB_DIRECTION_JUDGE_TH:
            G_regs->reg_Di_8.reg.Gb_Direction_Judge_Th = (unsigned int)value;
            return 1;
        case GB_FD_BAD_MERGE_TH_1:
            G_regs->reg_Di_8.reg.Gb_Fd_Bad_Merge_Th_1 = (unsigned int)value;
            return 1;
        case GB_FD_BAD_MERGE_TH_2:
            G_regs->reg_Di_8.reg.Gb_Fd_Bad_Merge_Th_2 = (unsigned int)value;
            return 1;
        case RESERVED_DI8:
            G_regs->reg_Di_8.reg.Reserved_Di8 = (unsigned int)value;
            return 1;
        case GB_DI_MD_AREA_TH0:
            G_regs->reg_Di_9.reg.Gb_Di_Md_Area_Th0 = (unsigned int)value;
            return 1;
        case GB_DI_MD_AREA_TH1:
            G_regs->reg_Di_9.reg.Gb_Di_Md_Area_Th1 = (unsigned int)value;
            return 1;
        case GB_DI_MD_AREA_COEF0:
            G_regs->reg_Di_A.reg.Gb_Di_Md_Area_Coef0 = (unsigned int)value;
            return 1;
        case GB_DI_MD_AREA_COEF1:
            G_regs->reg_Di_A.reg.Gb_Di_Md_Area_Coef1 = (unsigned int)value;
            return 1;
        case GB_DI_MD_ADJ_COEF0:
            G_regs->reg_Di_A.reg.Gb_Di_Md_Adj_Coef0 = (unsigned int)value;
            return 1;
        case GB_DI_MD_ADJ_COEF1:
            G_regs->reg_Di_A.reg.Gb_Di_Md_Adj_Coef1 = (unsigned int)value;
            return 1;
        case RESERVED_DIA:
            G_regs->reg_Di_A.reg.Reserved_Dia = (unsigned int)value;
            return 1;
        case GB_DI_IAD_AVE_ADJ_TH0:
            G_regs->reg_Di_B.reg.Gb_Di_Iad_Ave_Adj_Th0 = (unsigned int)value;
            return 1;
        case GB_DI_IAD_AVE_ADJ_TH1:
            G_regs->reg_Di_B.reg.Gb_Di_Iad_Ave_Adj_Th1 = (unsigned int)value;
            return 1;
        case RESERVED_DIB:
            G_regs->reg_Di_B.reg.Reserved_Dib = (unsigned int)value;
            return 1;
        case GB_DI_IAD_AVE_ADJ_VAL:
            G_regs->reg_Di_C.reg.Gb_Di_Iad_Ave_Adj_Val = (unsigned int)value;
            return 1;
        case GB_DI_IAD_AVE_ADJ_COEF:
            G_regs->reg_Di_C.reg.Gb_Di_Iad_Ave_Adj_Coef = (unsigned int)value;
            return 1;
        case GB_DI_MTD_FEATHERDETECT_EN:
            G_regs->reg_Di_C.reg.Gb_Di_Mtd_Featherdetect_En = (unsigned int)value;
            return 1;
        case RESERVED_DIC:
            G_regs->reg_Di_C.reg.Reserved_Dic = (unsigned int)value;
            return 1;
        case GB_DI_IAD_LINE_MD_X1:
            G_regs->reg_Di_D.reg.Gb_Di_Iad_Line_Md_X1 = (unsigned int)value;
            return 1;
        case GB_DI_IAD_LINE_MD_X2:
            G_regs->reg_Di_D.reg.Gb_Di_Iad_Line_Md_X2 = (unsigned int)value;
            return 1;
        case GB_DI_MTD_FEATHERPARA:
            G_regs->reg_Di_E.reg.Gb_Di_Mtd_Featherpara = (unsigned int)value;
            return 1;
        case GB_DI_IAD_LINE_MD_Y1:
            G_regs->reg_Di_E.reg.Gb_Di_Iad_Line_Md_Y1 = (unsigned int)value;
            return 1;
        case GB_DI_IAD_LINE_MD_K:
            G_regs->reg_Di_E.reg.Gb_Di_Iad_Line_Md_K = (unsigned int)value;
            return 1;
        case GB_DI_TH:
            G_regs->reg_Vpp_Control_0.reg.Gb_Di_Th = (unsigned int)value;
            return 1;
        case RESERVED_VC01:
            G_regs->reg_Vpp_Control_0.reg.Reserved_Vc01 = (unsigned int)value;
            return 1;
        case GB_MD_TH2:
            G_regs->reg_Vpp_Control_0.reg.Gb_Md_Th2 = (unsigned int)value;
            return 1;
        case GB_MD_TH2_EN:
            G_regs->reg_Vpp_Control_0.reg.Gb_Md_Th2_En = (unsigned int)value;
            return 1;
        case RESERVED_VC0:
            G_regs->reg_Vpp_Control_0.reg.Reserved_Vc0 = (unsigned int)value;
            return 1;
        case GB_MD_TH:
            G_regs->reg_Vpp_Control_3.reg.Gb_Md_Th = (unsigned int)value;
            return 1;
        case GB_DI_SR_REV_DIFF_TH:
            G_regs->reg_Vpp_Control_3.reg.Gb_Di_Sr_Rev_Diff_Th = (unsigned int)value;
            return 1;
        case RESERVED_VC3:
            G_regs->reg_Vpp_Control_3.reg.Reserved_Vc3 = (unsigned int)value;
            return 1;
        case GB_DI_MTD_EDGEVALTHR:
            G_regs->reg_Vpp_Control_4.reg.Gb_Di_Mtd_Edgevalthr = (unsigned int)value;
            return 1;
        case GB_DI_MTD_STATICPARA:
            G_regs->reg_Vpp_Control_4.reg.Gb_Di_Mtd_Staticpara = (unsigned int)value;
            return 1;
        case GB_DI_MTD_MAXMINTHR:
            G_regs->reg_Vpp_Control_4.reg.Gb_Di_Mtd_Maxminthr = (unsigned int)value;
            return 1;
        case GB_DI_SR_GRAD_TH:
            G_regs->reg_Vpp_Control_4.reg.Gb_Di_Sr_Grad_Th = (unsigned int)value;
            return 1;
        case GB_DI_MTD_MD_TH:
            G_regs->reg_Vpp_Control_4.reg.Gb_Di_Mtd_Md_Th = (unsigned int)value;
            return 1;
        case RESERVED_VC4:
            G_regs->reg_Vpp_Control_4.reg.Reserved_Vc4 = (unsigned int)value;
            return 1;
        case GB_DI_SR_LINE_DIFF_TH:
            G_regs->reg_Vpp_Control_5.reg.Gb_Di_Sr_Line_Diff_Th = (unsigned int)value;
            return 1;
        case GB_DI_IAD_MD_AVE_TH:
            G_regs->reg_Vpp_Control_5.reg.Gb_Di_Iad_Md_Ave_Th = (unsigned int)value;
            return 1;
        case RESERVED_VC12:
            G_regs->reg_Vpp_Control_5.reg.Reserved_Vc12 = (unsigned int)value;
            return 1;
        case GB_MTD_COL_HIST_BASE:
            G_regs->reg_Vpp_Control_6.reg.Gb_Mtd_Col_Hist_Base = (unsigned int)value;
            return 1;
        case GB_MD_LOW_TH:
            G_regs->reg_Vpp_Control_9.reg.Gb_Md_Low_Th = (unsigned int)value;
            return 1;
        case RESERVED_VC9:
            G_regs->reg_Vpp_Control_9.reg.Reserved_Vc9 = (unsigned int)value;
            return 1;
        case GB_LBUF_SIZE:
            G_regs->reg_Vpp_Lbuf_0.reg.Gb_Lbuf_Size = (unsigned int)value;
            return 1;
        case GB_VPP_IN_SIZE:
            G_regs->reg_Vpp_Lbuf_0.reg.Gb_Vpp_In_Size = (unsigned int)value;
            return 1;
        case GB_DISABLE_LCID_BLEND:
            G_regs->reg_Vpp_Lbuf_0.reg.Gb_Disable_Lcid_Blend = (unsigned int)value;
            return 1;
        case GB_FD_FORCE_DI:
            G_regs->reg_Vpp_Lbuf_0.reg.Gb_Fd_Force_Di = (unsigned int)value;
            return 1;
        case GB_DI_BLEND_COEF:
            G_regs->reg_Vpp_Lbuf_0.reg.Gb_Di_Blend_Coef = (unsigned int)value;
            return 1;
        case RESERVED_VL0:
            G_regs->reg_Vpp_Lbuf_0.reg.Reserved_Vl0 = (unsigned int)value;
            return 1;
        case GB_VPP_DI_OFFSET:
            G_regs->reg_Vpp_Lbuf_1.reg.Gb_Vpp_Di_Offset = (unsigned int)value;
            return 1;
        case GB_VPP_DI_SIZE:
            G_regs->reg_Vpp_Lbuf_1.reg.Gb_Vpp_Di_Size = (unsigned int)value;
            return 1;
        case GB_VPP_DIREF_IN_OFFSET:
            G_regs->reg_Vpp_Lbuf_1.reg.Gb_Vpp_Diref_In_Offset = (unsigned int)value;
            return 1;
        case RESERVED_VL1:
            G_regs->reg_Vpp_Lbuf_1.reg.Reserved_Vl1 = (unsigned int)value;
            return 1;
        case GB_FD_MD_TH:
            G_regs->reg_Vpp_Fd_Control_0.reg.Gb_Fd_Md_Th = (unsigned int)value;
            return 1;
        case GB_SCENE_CHANGE_TOP:
            G_regs->reg_Vpp_Fd_Control_0.reg.Gb_Scene_Change_Top = (unsigned int)value;
            return 1;
        case GB_SCENE_CHANGE_BOTTOM:
            G_regs->reg_Vpp_Fd_Control_0.reg.Gb_Scene_Change_Bottom = (unsigned int)value;
            return 1;
        case GB_2V2_TH_DELTA:
            G_regs->reg_Vpp_Fd_Control_1.reg.Gb_2v2_Th_Delta = (unsigned int)value;
            return 1;
        case GB_2V2_TH_RATIO:
            G_regs->reg_Vpp_Fd_Control_1.reg.Gb_2v2_Th_Ratio = (unsigned int)value;
            return 1;
        case GB_2V2_SAD_COUNTER_TH:
            G_regs->reg_Vpp_Fd_Control_1.reg.Gb_2v2_Sad_Counter_Th = (unsigned int)value;
            return 1;
        case GB_2V2_MD_FAILED_COUNTER_TH:
            G_regs->reg_Vpp_Fd_Control_1.reg.Gb_2v2_Md_Failed_Counter_Th = (unsigned int)value;
            return 1;
        case GB_LCID_DIFF_TH:
            G_regs->reg_Vpp_Fd_Control_1.reg.Gb_Lcid_Diff_Th = (unsigned int)value;
            return 1;
        case GB_2V2_COUNT_MODE:
            G_regs->reg_Vpp_Fd_Control_1.reg.Gb_2v2_Count_Mode = (unsigned int)value;
            return 1;
        case GB_SCENE_CHANGE_TH:
            G_regs->reg_Vpp_Fd_Control_2.reg.Gb_Scene_Change_Th = (unsigned int)value;
            return 1;
        case GB_SCENECHANGE_MD_MODE:
            G_regs->reg_Vpp_Fd_Control_2.reg.Gb_Scenechange_Md_Mode = (unsigned int)value;
            return 1;
        case GB_FD_MD_TH2:
            G_regs->reg_Vpp_Fd_Control_2.reg.Gb_Fd_Md_Th2 = (unsigned int)value;
            return 1;
        case GB_FD_COUNTER_TH:
            G_regs->reg_Vpp_Fd_Control_3.reg.Gb_Fd_Counter_Th = (unsigned int)value;
            return 1;
        case GB_FD_DETECT_COUNTER_INITIAL:
            G_regs->reg_Vpp_Fd_Control_3.reg.Gb_Fd_Detect_Counter_Initial = (unsigned int)value;
            return 1;
        case GB_FD_STRONG_COUNTER_INITIAL:
            G_regs->reg_Vpp_Fd_Control_3.reg.Gb_Fd_Strong_Counter_Initial = (unsigned int)value;
            return 1;
        case GB_2V2_FAILED_COUNTER_TH:
            G_regs->reg_Vpp_Fd_Control_3.reg.Gb_2v2_Failed_Counter_Th = (unsigned int)value;
            return 1;
        case GB_2V2_MD_SUCCEED_COUNTER_TH:
            G_regs->reg_Vpp_Fd_Control_3.reg.Gb_2v2_Md_Succeed_Counter_Th = (unsigned int)value;
            return 1;
        case GB_2V2_SAD_SUCCEED_COUNTER_TH:
            G_regs->reg_Vpp_Fd_Control_3.reg.Gb_2v2_Sad_Succeed_Counter_Th = (unsigned int)value;
            return 1;
        case GB_2V2_SAD_FAILED_RANGE:
            G_regs->reg_Vpp_Fd_Control_3.reg.Gb_2v2_Sad_Failed_Range = (unsigned int)value;
            return 1;
        case GB_2V2_FRAME_CNT_TH:
            G_regs->reg_Vpp_Fd_Control_4.reg.Gb_2v2_Frame_Cnt_Th = (unsigned int)value;
            return 1;
        case GB_2V2_FAILED_COUNTER_TH2:
            G_regs->reg_Vpp_Fd_Control_4.reg.Gb_2v2_Failed_Counter_Th2 = (unsigned int)value;
            return 1;
        case GB_FD_2V2_SAD_CLR_TH_1:
            G_regs->reg_Vpp_Fd_Control_4.reg.Gb_Fd_2v2_Sad_Clr_Th_1 = (unsigned int)value;
            return 1;
        case GB_FD_2V2_SAD_CLR_TH_2:
            G_regs->reg_Vpp_Fd_Control_4.reg.Gb_Fd_2v2_Sad_Clr_Th_2 = (unsigned int)value;
            return 1;
        case GB_FD_2V2_SAD_CLR_TH_3:
            G_regs->reg_Vpp_Fd_Control_4.reg.Gb_Fd_2v2_Sad_Clr_Th_3 = (unsigned int)value;
            return 1;
        case RESERVED_FD4:
            G_regs->reg_Vpp_Fd_Control_4.reg.Reserved_Fd4 = (unsigned int)value;
            return 1;
        case GB_FD_BAD_COPY_TH_1:
            G_regs->reg_Vpp_Fd_Control_5.reg.Gb_Fd_Bad_Copy_Th_1 = (unsigned int)value;
            return 1;
        case GB_FD_BAD_COPY_TH_2:
            G_regs->reg_Vpp_Fd_Control_5.reg.Gb_Fd_Bad_Copy_Th_2 = (unsigned int)value;
            return 1;
        case GB_FD_BAD_COPY_TH_3:
            G_regs->reg_Vpp_Fd_Control_5.reg.Gb_Fd_Bad_Copy_Th_3 = (unsigned int)value;
            return 1;
        case GB_FD_BAD_COPY_TH_4:
            G_regs->reg_Vpp_Fd_Control_5.reg.Gb_Fd_Bad_Copy_Th_4 = (unsigned int)value;
            return 1;
        case GB_FD_QUIT_2V2_TH_1:
            G_regs->reg_Vpp_Fd_Control_5.reg.Gb_Fd_Quit_2v2_Th_1 = (unsigned int)value;
            return 1;
        case RESERVED_FD5:
            G_regs->reg_Vpp_Fd_Control_5.reg.Reserved_Fd5 = (unsigned int)value;
            return 1;
        case GB_FD_QUIT_2V2_TH_2:
            G_regs->reg_Vpp_Fd_Control_6.reg.Gb_Fd_Quit_2v2_Th_2 = (unsigned int)value;
            return 1;
        case GB_FD_QUIT_2V2_TH_3:
            G_regs->reg_Vpp_Fd_Control_6.reg.Gb_Fd_Quit_2v2_Th_3 = (unsigned int)value;
            return 1;
        case GB_FD_QUIT_2V2_TH_4:
            G_regs->reg_Vpp_Fd_Control_6.reg.Gb_Fd_Quit_2v2_Th_4 = (unsigned int)value;
            return 1;
        case GB_FD_QUIT_2V2_TH_5:
            G_regs->reg_Vpp_Fd_Control_6.reg.Gb_Fd_Quit_2v2_Th_5 = (unsigned int)value;
            return 1;
        case GB_FD_2V2_MD_TH_3:
            G_regs->reg_Vpp_Fd_Control_6.reg.Gb_Fd_2v2_Md_Th_3 = (unsigned int)value;
            return 1;
        case GB_FD_2V2_MD_TH_5:
            G_regs->reg_Vpp_Fd_Control_6.reg.Gb_Fd_2v2_Md_Th_5 = (unsigned int)value;
            return 1;
        case GB_FD_2V2_MD_TH_1:
            G_regs->reg_Vpp_Fd_Control_7.reg.Gb_Fd_2v2_Md_Th_1 = (unsigned int)value;
            return 1;
        case GB_FD_2V2_MD_TH_2:
            G_regs->reg_Vpp_Fd_Control_7.reg.Gb_Fd_2v2_Md_Th_2 = (unsigned int)value;
            return 1;
        case GB_FD_2V2_MD_TH_4:
            G_regs->reg_Vpp_Fd_Control_7.reg.Gb_Fd_2v2_Md_Th_4 = (unsigned int)value;
            return 1;
        case GB_FD_SC_QUIT_2V2_TH:
            G_regs->reg_Vpp_Fd_Control_7.reg.Gb_Fd_Sc_Quit_2v2_Th = (unsigned int)value;
            return 1;
        case GB_DI_CG_OFF:
            G_regs->reg_Cg_Ctrl.reg.Gb_Di_Cg_Off = (unsigned int)value;
            return 1;
        case GB_SCL_CG_OFF:
            G_regs->reg_Cg_Ctrl.reg.Gb_Scl_Cg_Off = (unsigned int)value;
            return 1;
        case GB_SCL_BP_CG_OFF:
            G_regs->reg_Cg_Ctrl.reg.Gb_Scl_Bp_Cg_Off = (unsigned int)value;
            return 1;
        case GB_CSCB_CG_OFF:
            G_regs->reg_Cg_Ctrl.reg.Gb_Cscb_Cg_Off = (unsigned int)value;
            return 1;
        case GB_CSCF_CG_OFF:
            G_regs->reg_Cg_Ctrl.reg.Gb_Cscf_Cg_Off = (unsigned int)value;
            return 1;
        case GB_CG_BUSY_TH:
            G_regs->reg_Cg_Ctrl.reg.Gb_Cg_Busy_Th = (unsigned int)value;
            return 1;
        case GB_FIRE_AFTER_SAVE:
            G_regs->reg_Cg_Ctrl.reg.Gb_Fire_After_Save = (unsigned int)value;
            return 1;
        case GB_RESTORE_KEEP_ON:
            G_regs->reg_Cg_Ctrl.reg.Gb_Restore_Keep_On = (unsigned int)value;
            return 1;
        case GB_FOR_ECO2:
            G_regs->reg_Cg_Ctrl.reg.Gb_For_Eco2 = (unsigned int)value;
            return 1;
        case GB_CPT_CG_OFF:
            G_regs->reg_Cg_Ctrl.reg.Gb_Cpt_Cg_Off = (unsigned int)value;
            return 1;
        case GB_SR_CG_OFF:
            G_regs->reg_Cg_Ctrl.reg.Gb_Sr_Cg_Off = (unsigned int)value;
            return 1;
        case GB_CM_CG_OFF:
            G_regs->reg_Cg_Ctrl.reg.Gb_Cm_Cg_Off = (unsigned int)value;
            return 1;
        case GB_CCM_CG_OFF:
            G_regs->reg_Cg_Ctrl.reg.Gb_Ccm_Cg_Off = (unsigned int)value;
            return 1;
        case GB_HDR_CG_OFF:
            G_regs->reg_Cg_Ctrl.reg.Gb_Hdr_Cg_Off = (unsigned int)value;
            return 1;
        case RESERVED_CG:
            G_regs->reg_Cg_Ctrl.reg.Reserved_Cg = (unsigned int)value;
            return 1;
        case GB_DI_EN:
            G_regs->reg_From_Opcode.reg.Gb_Di_En = (unsigned int)value;
            return 1;
        case GB_FD_EN:
            G_regs->reg_From_Opcode.reg.Gb_Fd_En = (unsigned int)value;
            return 1;
        case GB_MTD_EN:
            G_regs->reg_From_Opcode.reg.Gb_Mtd_En = (unsigned int)value;
            return 1;
        case GB_TWO_FRAME_OUT_EN:
            G_regs->reg_From_Opcode.reg.Gb_Two_Frame_Out_En = (unsigned int)value;
            return 1;
        case GB_DI_FORCE_LCID:
            G_regs->reg_From_Opcode.reg.Gb_Di_Force_Lcid = (unsigned int)value;
            return 1;
        case GB_SINGLE_SEL_OTH:
            G_regs->reg_From_Opcode.reg.Gb_Single_Sel_Oth = (unsigned int)value;
            return 1;
        case GB_TOP_FIELD_FIRST:
            G_regs->reg_From_Opcode.reg.Gb_Top_Field_First = (unsigned int)value;
            return 1;
        case GB_RESET_HQVPP:
            G_regs->reg_From_Opcode.reg.Gb_Reset_Hqvpp = (unsigned int)value;
            return 1;
        case GB_HQVPP_MODE:
            G_regs->reg_From_Opcode.reg.Gb_Hqvpp_Mode = (unsigned int)value;
            return 1;
        case GB_VPP_FD_INDEX:
            G_regs->reg_From_Opcode.reg.Gb_Vpp_Fd_Index = (unsigned int)value;
            return 1;
        case RESERVED_OPCODE:
            G_regs->reg_From_Opcode.reg.Reserved_Opcode = (unsigned int)value;
            return 1;
        case GB_CMLUT_EN:
            G_regs->reg_Cm_0.reg.Gb_Cmlut_En = (unsigned int)value;
            return 1;
        case GB_TINT_EN:
            G_regs->reg_Cm_0.reg.Gb_Tint_En = (unsigned int)value;
            return 1;
        case GB_TINT_HOFFSET:
            G_regs->reg_Cm_0.reg.Gb_Tint_Hoffset = (unsigned int)value;
            return 1;
        case RESERVED_CM0:
            G_regs->reg_Cm_0.reg.Reserved_Cm0 = (unsigned int)value;
            return 1;
        case GB_CMLUT0:
            assert(index < 41);
            G_regs->reg_Cm_1[index].reg.Gb_Cmlut0= (unsigned int)value;
            return 1;
        case GB_CMLUT1:
            assert(index < 41);
            G_regs->reg_Cm_1[index].reg.Gb_Cmlut1= (unsigned int)value;
            return 1;
        case RESERVED_CM1:
            assert(index < 41);
            G_regs->reg_Cm_1[index].reg.Reserved_Cm1= (unsigned int)value;
            return 1;
        case GB_CCM_EN:
            G_regs->reg_Ccm_0.reg.Gb_Ccm_En = (unsigned int)value;
            return 1;
        case GB_METADATA_EN:
            G_regs->reg_Ccm_0.reg.Gb_Metadata_En = (unsigned int)value;
            return 1;
        case GB_DG_HCOE:
            G_regs->reg_Ccm_0.reg.Gb_Dg_Hcoe = (unsigned int)value;
            return 1;
        case RESERVED_CCM0:
            G_regs->reg_Ccm_0.reg.Reserved_Ccm0 = (unsigned int)value;
            return 1;
        case GB_DG_LCOE:
            G_regs->reg_Ccm_1.reg.Gb_Dg_Lcoe = (unsigned int)value;
            return 1;
        case GB_YUV2RGB_EN:
            G_regs->reg_Ccm_1.reg.Gb_Yuv2rgb_En = (unsigned int)value;
            return 1;
        case GB_YUV2RGB_MODE:
            G_regs->reg_Ccm_1.reg.Gb_Yuv2rgb_Mode = (unsigned int)value;
            return 1;
        case GB_CCM_HDR_INV:
            G_regs->reg_Ccm_1.reg.Gb_Ccm_Hdr_Inv = (unsigned int)value;
            return 1;
        case RESERVED_CCM1:
            G_regs->reg_Ccm_1.reg.Reserved_Ccm1 = (unsigned int)value;
            return 1;
        case GB_DG_TBL_0:
            assert(index < 16);
            G_regs->reg_Ccm_3[index].reg.Gb_Dg_Tbl_0= (unsigned int)value;
            return 1;
        case GB_DG_TBL_1:
            assert(index < 16);
            G_regs->reg_Ccm_3[index].reg.Gb_Dg_Tbl_1= (unsigned int)value;
            return 1;
        case GB_G_TBL_0:
            assert(index < 16);
            G_regs->reg_Ccm_4[index].reg.Gb_G_Tbl_0= (unsigned int)value;
            return 1;
        case GB_G_TBL_1:
            assert(index < 16);
            G_regs->reg_Ccm_4[index].reg.Gb_G_Tbl_1= (unsigned int)value;
            return 1;
        case GB_G_TBL_2:
            assert(index < 16);
            G_regs->reg_Ccm_4[index].reg.Gb_G_Tbl_2= (unsigned int)value;
            return 1;
        case RESERVED_CCM5:
            assert(index < 16);
            G_regs->reg_Ccm_4[index].reg.Reserved_Ccm5= (unsigned int)value;
            return 1;
        case GB_HLG_EN:
            G_regs->reg_Ccm_5.reg.Gb_Hlg_En = (unsigned int)value;
            return 1;
        case GB_HLG_ACOEF:
            G_regs->reg_Ccm_5.reg.Gb_Hlg_Acoef = (unsigned int)value;
            return 1;
        case GB_HLG_BCOEF:
            G_regs->reg_Ccm_5.reg.Gb_Hlg_Bcoef = (unsigned int)value;
            return 1;
        case RESERVED_CCM6:
            G_regs->reg_Ccm_5.reg.Reserved_Ccm6 = (unsigned int)value;
            return 1;
        case GB_HLG_TBL_0:
            assert(index < 16);
            G_regs->reg_Ccm_6[index].reg.Gb_Hlg_Tbl_0= (unsigned int)value;
            return 1;
        case GB_HLG_TBL_1:
            assert(index < 16);
            G_regs->reg_Ccm_6[index].reg.Gb_Hlg_Tbl_1= (unsigned int)value;
            return 1;
        case GB_HLG_TBL_20:
            G_regs->reg_Ccm_7.reg.Gb_Hlg_Tbl_20 = (unsigned int)value;
            return 1;
        case GB_HLG_TBL_21:
            G_regs->reg_Ccm_7.reg.Gb_Hlg_Tbl_21 = (unsigned int)value;
            return 1;
        case GB_HLG_TBL_22:
            G_regs->reg_Ccm_8.reg.Gb_Hlg_Tbl_22 = (unsigned int)value;
            return 1;
        case GB_HLG_INVTBL_22:
            G_regs->reg_Ccm_8.reg.Gb_Hlg_Invtbl_22 = (unsigned int)value;
            return 1;
        case GB_1886_ACOEF:
            G_regs->reg_Ccm_9.reg.Gb_1886_Acoef = (unsigned int)value;
            return 1;
        case GB_1886_BCOEF:
            G_regs->reg_Ccm_9.reg.Gb_1886_Bcoef = (unsigned int)value;
            return 1;
        case GB_HLG_INVACOEF:
            G_regs->reg_Ccm_A.reg.Gb_Hlg_Invacoef = (unsigned int)value;
            return 1;
        case GB_HLG_INVBCOEF:
            G_regs->reg_Ccm_A.reg.Gb_Hlg_Invbcoef = (unsigned int)value;
            return 1;
        case GB_1886_EN:
            G_regs->reg_Ccm_A.reg.Gb_1886_En = (unsigned int)value;
            return 1;
        case RESERVED_CCM9:
            G_regs->reg_Ccm_A.reg.Reserved_Ccm9 = (unsigned int)value;
            return 1;
        case GB_HLG_INVTBL_0:
            assert(index < 16);
            G_regs->reg_Ccm_B[index].reg.Gb_Hlg_Invtbl_0= (unsigned int)value;
            return 1;
        case GB_HLG_INVTBL_1:
            assert(index < 16);
            G_regs->reg_Ccm_B[index].reg.Gb_Hlg_Invtbl_1= (unsigned int)value;
            return 1;
        case GB_HLG_INVTBL_20:
            G_regs->reg_Ccm_C.reg.Gb_Hlg_Invtbl_20 = (unsigned int)value;
            return 1;
        case GB_HLG_INVTBL_21:
            G_regs->reg_Ccm_C.reg.Gb_Hlg_Invtbl_21 = (unsigned int)value;
            return 1;
        case GB_1886_INVACOEF:
            G_regs->reg_Ccm_D.reg.Gb_1886_Invacoef = (unsigned int)value;
            return 1;
        case GB_1886_INVBCOEF:
            G_regs->reg_Ccm_D.reg.Gb_1886_Invbcoef = (unsigned int)value;
            return 1;
        case GB_CONVERT_M_00:
            G_regs->reg_Cc_0.reg.Gb_Convert_M_00 = (unsigned int)value;
            return 1;
        case GB_CONVERT_M_01:
            G_regs->reg_Cc_0.reg.Gb_Convert_M_01 = (unsigned int)value;
            return 1;
        case RESERVED_CC0:
            G_regs->reg_Cc_0.reg.Reserved_Cc0 = (unsigned int)value;
            return 1;
        case GB_CONVERT_M_10:
            G_regs->reg_Cc_1.reg.Gb_Convert_M_10 = (unsigned int)value;
            return 1;
        case GB_CONVERT_M_11:
            G_regs->reg_Cc_1.reg.Gb_Convert_M_11 = (unsigned int)value;
            return 1;
        case RESERVED_CC1:
            G_regs->reg_Cc_1.reg.Reserved_Cc1 = (unsigned int)value;
            return 1;
        case GB_CONVERT_M_20:
            G_regs->reg_Cc_2.reg.Gb_Convert_M_20 = (unsigned int)value;
            return 1;
        case GB_CONVERT_M_21:
            G_regs->reg_Cc_2.reg.Gb_Convert_M_21 = (unsigned int)value;
            return 1;
        case RESERVED_CC2:
            G_regs->reg_Cc_2.reg.Reserved_Cc2 = (unsigned int)value;
            return 1;
        case GB_CONVERT_M_02:
            G_regs->reg_Cc_3.reg.Gb_Convert_M_02 = (unsigned int)value;
            return 1;
        case GB_CONVERT_M_12:
            G_regs->reg_Cc_3.reg.Gb_Convert_M_12 = (unsigned int)value;
            return 1;
        case RESERVED_CC3:
            G_regs->reg_Cc_3.reg.Reserved_Cc3 = (unsigned int)value;
            return 1;
        case GB_CONVERT_M_22:
            G_regs->reg_Cc_4.reg.Gb_Convert_M_22 = (unsigned int)value;
            return 1;
        case RESERVED_CC4:
            G_regs->reg_Cc_4.reg.Reserved_Cc4 = (unsigned int)value;
            return 1;
        case GB_HADJ_EN:
            G_regs->reg_Hadj_0.reg.Gb_Hadj_En = (unsigned int)value;
            return 1;
        case GB_HRANGE_GL:
            G_regs->reg_Hadj_0.reg.Gb_Hrange_Gl = (unsigned int)value;
            return 1;
        case GB_HRANGE_GH:
            G_regs->reg_Hadj_0.reg.Gb_Hrange_Gh = (unsigned int)value;
            return 1;
        case RESERVED_HADJ0:
            G_regs->reg_Hadj_0.reg.Reserved_Hadj0 = (unsigned int)value;
            return 1;
        case GB_HRANGE_RL:
            G_regs->reg_Hadj_1.reg.Gb_Hrange_Rl = (unsigned int)value;
            return 1;
        case GB_HRANGE_RH:
            G_regs->reg_Hadj_1.reg.Gb_Hrange_Rh = (unsigned int)value;
            return 1;
        case RESERVED_HADJ1:
            G_regs->reg_Hadj_1.reg.Reserved_Hadj1 = (unsigned int)value;
            return 1;
        case GB_G_RANGE:
            G_regs->reg_Hadj_2.reg.Gb_G_Range = (unsigned int)value;
            return 1;
        case GB_G_AXIS:
            G_regs->reg_Hadj_2.reg.Gb_G_Axis = (unsigned int)value;
            return 1;
        case GB_G_COEF:
            G_regs->reg_Hadj_2.reg.Gb_G_Coef = (unsigned int)value;
            return 1;
        case GB_R_RANGE:
            G_regs->reg_Hadj_3.reg.Gb_R_Range = (unsigned int)value;
            return 1;
        case GB_R_AXIS:
            G_regs->reg_Hadj_3.reg.Gb_R_Axis = (unsigned int)value;
            return 1;
        case GB_R_COEF:
            G_regs->reg_Hadj_3.reg.Gb_R_Coef = (unsigned int)value;
            return 1;
        case GB_SADJ_EN:
            G_regs->reg_Sadj_0.reg.Gb_Sadj_En = (unsigned int)value;
            return 1;
        case GB_SADJ_INC00:
            G_regs->reg_Sadj_0.reg.Gb_Sadj_Inc00 = (unsigned int)value;
            return 1;
        case GB_SADJ_INC01:
            G_regs->reg_Sadj_0.reg.Gb_Sadj_Inc01 = (unsigned int)value;
            return 1;
        case RESERVED_SADJ0:
            G_regs->reg_Sadj_0.reg.Reserved_Sadj0 = (unsigned int)value;
            return 1;
        case GB_SADJ_INC02:
            G_regs->reg_Sadj_1.reg.Gb_Sadj_Inc02 = (unsigned int)value;
            return 1;
        case GB_SADJ_INC03:
            G_regs->reg_Sadj_1.reg.Gb_Sadj_Inc03 = (unsigned int)value;
            return 1;
        case RESERVED_SADJ1:
            G_regs->reg_Sadj_1.reg.Reserved_Sadj1 = (unsigned int)value;
            return 1;
        case GB_SADJ_INC04:
            G_regs->reg_Sadj_2.reg.Gb_Sadj_Inc04 = (unsigned int)value;
            return 1;
        case GB_SADJ_INC05:
            G_regs->reg_Sadj_2.reg.Gb_Sadj_Inc05 = (unsigned int)value;
            return 1;
        case RESERVED_SADJ2:
            G_regs->reg_Sadj_2.reg.Reserved_Sadj2 = (unsigned int)value;
            return 1;
        case GB_SADJ_INC06:
            G_regs->reg_Sadj_3.reg.Gb_Sadj_Inc06 = (unsigned int)value;
            return 1;
        case GB_SADJ_INC07:
            G_regs->reg_Sadj_3.reg.Gb_Sadj_Inc07 = (unsigned int)value;
            return 1;
        case GB_SADJ_INC14:
            G_regs->reg_Sadj_3.reg.Gb_Sadj_Inc14= (value>>0)&((1<<5)-1);
            G_regs->reg_Sadj_4.reg.Gb_Sadj_Inc14= (value>>5)&((1<<6)-1);
            return 1;
        case RESERVED_SADJ3:
            G_regs->reg_Sadj_3.reg.Reserved_Sadj3 = (unsigned int)value;
            return 1;
        case GB_SADJ_INC08:
            G_regs->reg_Sadj_4.reg.Gb_Sadj_Inc08 = (unsigned int)value;
            return 1;
        case GB_SADJ_INC09:
            G_regs->reg_Sadj_4.reg.Gb_Sadj_Inc09 = (unsigned int)value;
            return 1;
        case RESERVED_SADJ4:
            G_regs->reg_Sadj_4.reg.Reserved_Sadj4 = (unsigned int)value;
            return 1;
        case GB_SADJ_INC0A:
            G_regs->reg_Sadj_5.reg.Gb_Sadj_Inc0a = (unsigned int)value;
            return 1;
        case GB_SADJ_INC0B:
            G_regs->reg_Sadj_5.reg.Gb_Sadj_Inc0b = (unsigned int)value;
            return 1;
        case GB_SADJ_INC13:
            G_regs->reg_Sadj_5.reg.Gb_Sadj_Inc13= (value>>0)&((1<<5)-1);
            G_regs->reg_Sadj_6.reg.Gb_Sadj_Inc13= (value>>5)&((1<<6)-1);
            return 1;
        case RESERVED_SADJ5:
            G_regs->reg_Sadj_5.reg.Reserved_Sadj5 = (unsigned int)value;
            return 1;
        case GB_SADJ_INC0C:
            G_regs->reg_Sadj_6.reg.Gb_Sadj_Inc0c = (unsigned int)value;
            return 1;
        case GB_SADJ_INC0D:
            G_regs->reg_Sadj_6.reg.Gb_Sadj_Inc0d = (unsigned int)value;
            return 1;
        case RESERVED_SADJ6:
            G_regs->reg_Sadj_6.reg.Reserved_Sadj6 = (unsigned int)value;
            return 1;
        case GB_SADJ_INC0E:
            G_regs->reg_Sadj_7.reg.Gb_Sadj_Inc0e = (unsigned int)value;
            return 1;
        case GB_SADJ_INC0F:
            G_regs->reg_Sadj_7.reg.Gb_Sadj_Inc0f = (unsigned int)value;
            return 1;
        case GB_SADJ_INC12:
            G_regs->reg_Sadj_7.reg.Gb_Sadj_Inc12= (value>>0)&((1<<5)-1);
            G_regs->reg_Sadj_8.reg.Gb_Sadj_Inc12= (value>>5)&((1<<6)-1);
            return 1;
        case RESERVED_SADJ7:
            G_regs->reg_Sadj_7.reg.Reserved_Sadj7 = (unsigned int)value;
            return 1;
        case GB_SADJ_INC10:
            G_regs->reg_Sadj_8.reg.Gb_Sadj_Inc10 = (unsigned int)value;
            return 1;
        case GB_SADJ_INC11:
            G_regs->reg_Sadj_8.reg.Gb_Sadj_Inc11 = (unsigned int)value;
            return 1;
        case RESERVED_SADJ8:
            G_regs->reg_Sadj_8.reg.Reserved_Sadj8 = (unsigned int)value;
            return 1;
        case GB_HDR_EN:
            G_regs->reg_Hdr_0.reg.Gb_Hdr_En = (unsigned int)value;
            return 1;
        case GB_HDR_CURVE_EN:
            G_regs->reg_Hdr_0.reg.Gb_Hdr_Curve_En = (unsigned int)value;
            return 1;
        case GB_HDR_CLIPLMT:
            G_regs->reg_Hdr_0.reg.Gb_Hdr_Cliplmt = (unsigned int)value;
            return 1;
        case GB_HDR_MAXFMEAN:
            G_regs->reg_Hdr_0.reg.Gb_Hdr_Maxfmean = (unsigned int)value;
            return 1;
        case GB_HDR_MAXMEAN:
            G_regs->reg_Hdr_0.reg.Gb_Hdr_Maxmean = (unsigned int)value;
            return 1;
        case GB_HDR_SC_EN:
            G_regs->reg_Hdr_0.reg.Gb_Hdr_Sc_En = (unsigned int)value;
            return 1;
        case GB_HDR_SMALLSIZE:
            G_regs->reg_Hdr_0.reg.Gb_Hdr_Smallsize = (unsigned int)value;
            return 1;
        case GB_HDR_MAXPEAKCNT:
            G_regs->reg_Hdr_1.reg.Gb_Hdr_Maxpeakcnt = (unsigned int)value;
            return 1;
        case GB_HDR_MINPEAKWG:
            G_regs->reg_Hdr_1.reg.Gb_Hdr_Minpeakwg = (unsigned int)value;
            return 1;
        case GB_HDR_MAXPEAKWG:
            G_regs->reg_Hdr_1.reg.Gb_Hdr_Maxpeakwg = (unsigned int)value;
            return 1;
        case GB_HDR_MANUALCLIPMODE:
            G_regs->reg_Hdr_1.reg.Gb_Hdr_Manualclipmode = (unsigned int)value;
            return 1;
        case GB_HDR_FRMWG:
            G_regs->reg_Hdr_1.reg.Gb_Hdr_Frmwg = (unsigned int)value;
            return 1;
        case GB_RESET_HDR:
            G_regs->reg_Hdr_1.reg.Gb_Reset_Hdr = (unsigned int)value;
            return 1;
        case RESERVED_HDR1:
            G_regs->reg_Hdr_1.reg.Reserved_Hdr1 = (unsigned int)value;
            return 1;
        case GB_HDR_PIXCNTFORSTAT:
            G_regs->reg_Hdr_2.reg.Gb_Hdr_Pixcntforstat = (unsigned int)value;
            return 1;
        case GB_HDR_SHFT:
            G_regs->reg_Hdr_2.reg.Gb_Hdr_Shft = (unsigned int)value;
            return 1;
        case GB_HDR_MINSTRETCHRANGE:
            G_regs->reg_Hdr_3.reg.Gb_Hdr_Minstretchrange = (unsigned int)value;
            return 1;
        case RESERVED_HDR3:
            G_regs->reg_Hdr_3.reg.Reserved_Hdr3 = (unsigned int)value;
            return 1;
        case GB_HDR_MINFILTSPD:
            G_regs->reg_Hdr_3.reg.Gb_Hdr_Minfiltspd = (unsigned int)value;
            return 1;
        case GB_HDR_BINMEDSDT:
            G_regs->reg_Hdr_3.reg.Gb_Hdr_Binmedsdt = (unsigned int)value;
            return 1;
        case GB_HDR_DELTAMEANTH:
            G_regs->reg_Hdr_4.reg.Gb_Hdr_Deltameanth = (unsigned int)value;
            return 1;
        case GB_HDR_BINMEDHIMAX:
            G_regs->reg_Hdr_4.reg.Gb_Hdr_Binmedhimax = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHIWGINIT:
            G_regs->reg_Hdr_4.reg.Gb_Hdr_Yluthiwginit = (unsigned int)value;
            return 1;
        case RESERVED_HDR4:
            G_regs->reg_Hdr_4.reg.Reserved_Hdr4 = (unsigned int)value;
            return 1;
        case GB_HDR_MEANCHANGETH:
            G_regs->reg_Hdr_5.reg.Gb_Hdr_Meanchangeth = (unsigned int)value;
            return 1;
        case RESERVED_HDR5:
            G_regs->reg_Hdr_5.reg.Reserved_Hdr5 = (unsigned int)value;
            return 1;
        case GB_HDR_PIXLOWRATIO:
            G_regs->reg_Hdr_6.reg.Gb_Hdr_Pixlowratio = (unsigned int)value;
            return 1;
        case RESERVED_HDR6:
            G_regs->reg_Hdr_6.reg.Reserved_Hdr6 = (unsigned int)value;
            return 1;
        case GB_HDR_PIXHIRATIO:
            G_regs->reg_Hdr_7.reg.Gb_Hdr_Pixhiratio = (unsigned int)value;
            return 1;
        case RESERVED_HDR7:
            G_regs->reg_Hdr_7.reg.Reserved_Hdr7 = (unsigned int)value;
            return 1;
        case GB_HDR_BLACKRATIO:
            G_regs->reg_Hdr_8.reg.Gb_Hdr_Blackratio = (unsigned int)value;
            return 1;
        case RESERVED_HDR8:
            G_regs->reg_Hdr_8.reg.Reserved_Hdr8 = (unsigned int)value;
            return 1;
        case GB_HDR_NOISERATIO:
            G_regs->reg_Hdr_9.reg.Gb_Hdr_Noiseratio = (unsigned int)value;
            return 1;
        case RESERVED_HDR9:
            G_regs->reg_Hdr_9.reg.Reserved_Hdr9 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_00:
            G_regs->reg_Hdr_Ylut_0.reg.Gb_Hdr_Yluthi_00 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_01:
            G_regs->reg_Hdr_Ylut_0.reg.Gb_Hdr_Yluthi_01 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_02:
            G_regs->reg_Hdr_Ylut_0.reg.Gb_Hdr_Yluthi_02 = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT0:
            G_regs->reg_Hdr_Ylut_0.reg.Reserved_Hdr_Ylut0 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_03:
            G_regs->reg_Hdr_Ylut_1.reg.Gb_Hdr_Yluthi_03 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_04:
            G_regs->reg_Hdr_Ylut_1.reg.Gb_Hdr_Yluthi_04 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_05:
            G_regs->reg_Hdr_Ylut_1.reg.Gb_Hdr_Yluthi_05 = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT1:
            G_regs->reg_Hdr_Ylut_1.reg.Reserved_Hdr_Ylut1 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_06:
            G_regs->reg_Hdr_Ylut_2.reg.Gb_Hdr_Yluthi_06 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_07:
            G_regs->reg_Hdr_Ylut_2.reg.Gb_Hdr_Yluthi_07 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_08:
            G_regs->reg_Hdr_Ylut_2.reg.Gb_Hdr_Yluthi_08 = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT2:
            G_regs->reg_Hdr_Ylut_2.reg.Reserved_Hdr_Ylut2 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_09:
            G_regs->reg_Hdr_Ylut_3.reg.Gb_Hdr_Yluthi_09 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_0A:
            G_regs->reg_Hdr_Ylut_3.reg.Gb_Hdr_Yluthi_0a = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_0B:
            G_regs->reg_Hdr_Ylut_3.reg.Gb_Hdr_Yluthi_0b = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT3:
            G_regs->reg_Hdr_Ylut_3.reg.Reserved_Hdr_Ylut3 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_0C:
            G_regs->reg_Hdr_Ylut_4.reg.Gb_Hdr_Yluthi_0c = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_0D:
            G_regs->reg_Hdr_Ylut_4.reg.Gb_Hdr_Yluthi_0d = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_0E:
            G_regs->reg_Hdr_Ylut_4.reg.Gb_Hdr_Yluthi_0e = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT4:
            G_regs->reg_Hdr_Ylut_4.reg.Reserved_Hdr_Ylut4 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_0F:
            G_regs->reg_Hdr_Ylut_5.reg.Gb_Hdr_Yluthi_0f = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_10:
            G_regs->reg_Hdr_Ylut_5.reg.Gb_Hdr_Yluthi_10 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_11:
            G_regs->reg_Hdr_Ylut_5.reg.Gb_Hdr_Yluthi_11 = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT5:
            G_regs->reg_Hdr_Ylut_5.reg.Reserved_Hdr_Ylut5 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_12:
            G_regs->reg_Hdr_Ylut_6.reg.Gb_Hdr_Yluthi_12 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_13:
            G_regs->reg_Hdr_Ylut_6.reg.Gb_Hdr_Yluthi_13 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_14:
            G_regs->reg_Hdr_Ylut_6.reg.Gb_Hdr_Yluthi_14 = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT6:
            G_regs->reg_Hdr_Ylut_6.reg.Reserved_Hdr_Ylut6 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_15:
            G_regs->reg_Hdr_Ylut_7.reg.Gb_Hdr_Yluthi_15 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_16:
            G_regs->reg_Hdr_Ylut_7.reg.Gb_Hdr_Yluthi_16 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_17:
            G_regs->reg_Hdr_Ylut_7.reg.Gb_Hdr_Yluthi_17 = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT7:
            G_regs->reg_Hdr_Ylut_7.reg.Reserved_Hdr_Ylut7 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_18:
            G_regs->reg_Hdr_Ylut_8.reg.Gb_Hdr_Yluthi_18 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_19:
            G_regs->reg_Hdr_Ylut_8.reg.Gb_Hdr_Yluthi_19 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_1A:
            G_regs->reg_Hdr_Ylut_8.reg.Gb_Hdr_Yluthi_1a = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT8:
            G_regs->reg_Hdr_Ylut_8.reg.Reserved_Hdr_Ylut8 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_1B:
            G_regs->reg_Hdr_Ylut_9.reg.Gb_Hdr_Yluthi_1b = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_1C:
            G_regs->reg_Hdr_Ylut_9.reg.Gb_Hdr_Yluthi_1c = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_1D:
            G_regs->reg_Hdr_Ylut_9.reg.Gb_Hdr_Yluthi_1d = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT9:
            G_regs->reg_Hdr_Ylut_9.reg.Reserved_Hdr_Ylut9 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_1E:
            G_regs->reg_Hdr_Ylut_A.reg.Gb_Hdr_Yluthi_1e = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTHI_1F:
            G_regs->reg_Hdr_Ylut_A.reg.Gb_Hdr_Yluthi_1f = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUTA:
            G_regs->reg_Hdr_Ylut_A.reg.Reserved_Hdr_Yluta = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_00:
            G_regs->reg_Hdr_Ylut_10.reg.Gb_Hdr_Ylutlow_00 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_01:
            G_regs->reg_Hdr_Ylut_10.reg.Gb_Hdr_Ylutlow_01 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_02:
            G_regs->reg_Hdr_Ylut_10.reg.Gb_Hdr_Ylutlow_02 = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT10:
            G_regs->reg_Hdr_Ylut_10.reg.Reserved_Hdr_Ylut10 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_03:
            G_regs->reg_Hdr_Ylut_11.reg.Gb_Hdr_Ylutlow_03 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_04:
            G_regs->reg_Hdr_Ylut_11.reg.Gb_Hdr_Ylutlow_04 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_05:
            G_regs->reg_Hdr_Ylut_11.reg.Gb_Hdr_Ylutlow_05 = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT11:
            G_regs->reg_Hdr_Ylut_11.reg.Reserved_Hdr_Ylut11 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_06:
            G_regs->reg_Hdr_Ylut_12.reg.Gb_Hdr_Ylutlow_06 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_07:
            G_regs->reg_Hdr_Ylut_12.reg.Gb_Hdr_Ylutlow_07 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_08:
            G_regs->reg_Hdr_Ylut_12.reg.Gb_Hdr_Ylutlow_08 = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT12:
            G_regs->reg_Hdr_Ylut_12.reg.Reserved_Hdr_Ylut12 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_09:
            G_regs->reg_Hdr_Ylut_13.reg.Gb_Hdr_Ylutlow_09 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_0A:
            G_regs->reg_Hdr_Ylut_13.reg.Gb_Hdr_Ylutlow_0a = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_0B:
            G_regs->reg_Hdr_Ylut_13.reg.Gb_Hdr_Ylutlow_0b = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT13:
            G_regs->reg_Hdr_Ylut_13.reg.Reserved_Hdr_Ylut13 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_0C:
            G_regs->reg_Hdr_Ylut_14.reg.Gb_Hdr_Ylutlow_0c = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_0D:
            G_regs->reg_Hdr_Ylut_14.reg.Gb_Hdr_Ylutlow_0d = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_0E:
            G_regs->reg_Hdr_Ylut_14.reg.Gb_Hdr_Ylutlow_0e = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT14:
            G_regs->reg_Hdr_Ylut_14.reg.Reserved_Hdr_Ylut14 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_0F:
            G_regs->reg_Hdr_Ylut_15.reg.Gb_Hdr_Ylutlow_0f = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_10:
            G_regs->reg_Hdr_Ylut_15.reg.Gb_Hdr_Ylutlow_10 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_11:
            G_regs->reg_Hdr_Ylut_15.reg.Gb_Hdr_Ylutlow_11 = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT15:
            G_regs->reg_Hdr_Ylut_15.reg.Reserved_Hdr_Ylut15 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_12:
            G_regs->reg_Hdr_Ylut_16.reg.Gb_Hdr_Ylutlow_12 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_13:
            G_regs->reg_Hdr_Ylut_16.reg.Gb_Hdr_Ylutlow_13 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_14:
            G_regs->reg_Hdr_Ylut_16.reg.Gb_Hdr_Ylutlow_14 = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT16:
            G_regs->reg_Hdr_Ylut_16.reg.Reserved_Hdr_Ylut16 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_15:
            G_regs->reg_Hdr_Ylut_17.reg.Gb_Hdr_Ylutlow_15 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_16:
            G_regs->reg_Hdr_Ylut_17.reg.Gb_Hdr_Ylutlow_16 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_17:
            G_regs->reg_Hdr_Ylut_17.reg.Gb_Hdr_Ylutlow_17 = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT17:
            G_regs->reg_Hdr_Ylut_17.reg.Reserved_Hdr_Ylut17 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_18:
            G_regs->reg_Hdr_Ylut_18.reg.Gb_Hdr_Ylutlow_18 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_19:
            G_regs->reg_Hdr_Ylut_18.reg.Gb_Hdr_Ylutlow_19 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_1A:
            G_regs->reg_Hdr_Ylut_18.reg.Gb_Hdr_Ylutlow_1a = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT18:
            G_regs->reg_Hdr_Ylut_18.reg.Reserved_Hdr_Ylut18 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_1B:
            G_regs->reg_Hdr_Ylut_19.reg.Gb_Hdr_Ylutlow_1b = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_1C:
            G_regs->reg_Hdr_Ylut_19.reg.Gb_Hdr_Ylutlow_1c = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_1D:
            G_regs->reg_Hdr_Ylut_19.reg.Gb_Hdr_Ylutlow_1d = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT19:
            G_regs->reg_Hdr_Ylut_19.reg.Reserved_Hdr_Ylut19 = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_1E:
            G_regs->reg_Hdr_Ylut_1a.reg.Gb_Hdr_Ylutlow_1e = (unsigned int)value;
            return 1;
        case GB_HDR_YLUTLOW_1F:
            G_regs->reg_Hdr_Ylut_1a.reg.Gb_Hdr_Ylutlow_1f = (unsigned int)value;
            return 1;
        case RESERVED_HDR_YLUT1A:
            G_regs->reg_Hdr_Ylut_1a.reg.Reserved_Hdr_Ylut1a = (unsigned int)value;
            return 1;
    }
    return 0;
}


static inline unsigned long long Vpp_Global_read_reg(Vpp_Global_regs * G_regs, int reg, int index)
{
    unsigned long long value = 0;
    switch(reg)
    {
        case GB_SRC_SF_BASE:
            return G_regs->reg_Src_Base.reg.Gb_Src_Sf_Base;
        case GB_OTH_SF_BASE:
            return G_regs->reg_Oth_Base.reg.Gb_Oth_Sf_Base;
        case GB_REF_SF_BASE:
            return G_regs->reg_Ref_Base.reg.Gb_Ref_Sf_Base;
        case GB_SRC_RANGE_MAP_Y_EN:
            return G_regs->reg_Base_Map.reg.Gb_Src_Range_Map_Y_En;
        case GB_SRC_RANGE_MAP_Y:
            return G_regs->reg_Base_Map.reg.Gb_Src_Range_Map_Y;
        case GB_SRC_RANGE_MAP_UV_EN:
            return G_regs->reg_Base_Map.reg.Gb_Src_Range_Map_Uv_En;
        case GB_SRC_RANGE_MAP_UV:
            return G_regs->reg_Base_Map.reg.Gb_Src_Range_Map_Uv;
        case GB_OTH_RANGE_MAP_Y_EN:
            return G_regs->reg_Base_Map.reg.Gb_Oth_Range_Map_Y_En;
        case GB_OTH_RANGE_MAP_Y:
            return G_regs->reg_Base_Map.reg.Gb_Oth_Range_Map_Y;
        case GB_OTH_RANGE_MAP_UV_EN:
            return G_regs->reg_Base_Map.reg.Gb_Oth_Range_Map_Uv_En;
        case GB_OTH_RANGE_MAP_UV:
            return G_regs->reg_Base_Map.reg.Gb_Oth_Range_Map_Uv;
        case GB_REF_RANGE_MAP_Y_EN:
            return G_regs->reg_Base_Map.reg.Gb_Ref_Range_Map_Y_En;
        case GB_REF_RANGE_MAP_Y:
            return G_regs->reg_Base_Map.reg.Gb_Ref_Range_Map_Y;
        case GB_REF_RANGE_MAP_UV_EN:
            return G_regs->reg_Base_Map.reg.Gb_Ref_Range_Map_Uv_En;
        case GB_REF_RANGE_MAP_UV:
            return G_regs->reg_Base_Map.reg.Gb_Ref_Range_Map_Uv;
        case RESERVED_BM:
            return G_regs->reg_Base_Map.reg.Reserved_Bm;
        case GB_DST_SF_BASE:
            return G_regs->reg_Dst_Base.reg.Gb_Dst_Sf_Base;
        case GB_SCL_DST_SF_BASE:
            return G_regs->reg_Scl_Dst_Base_0.reg.Gb_Scl_Dst_Sf_Base;
        case GB_SCL_DST_CLIPX:
            return G_regs->reg_Scl_Dst_Base_1.reg.Gb_Scl_Dst_Clipx;
        case GB_SCL_DST_CLIPY:
            return G_regs->reg_Scl_Dst_Base_1.reg.Gb_Scl_Dst_Clipy;
        case RESERVED_MOB:
            return G_regs->reg_Scl_Dst_Base_1.reg.Reserved_Mob;
        case GB_SCL_DST_SF_HEIGHT:
            return G_regs->reg_Scl_Dst_Height.reg.Gb_Scl_Dst_Sf_Height;
        case RESERVED_SDB2:
            return G_regs->reg_Scl_Dst_Height.reg.Reserved_Sdb2;
        case GB_SRC_SF_BOT_BASE:
            return G_regs->reg_Bot_Base_0.reg.Gb_Src_Sf_Bot_Base;
        case GB_OTH_SF_BOT_BASE:
            return G_regs->reg_Bot_Base_1.reg.Gb_Oth_Sf_Bot_Base;
        case GB_REF_SF_BOT_BASE:
            return G_regs->reg_Bot_Base_2.reg.Gb_Ref_Sf_Bot_Base;
        case GB_HDR_INT_BASE:
            return G_regs->reg_Hdr_Lut_Base.reg.Gb_Hdr_Int_Base;
        case GB_YSUM_MEM_BASE:
            value |= G_regs->reg_Hdr_Ysum_Base.reg.Gb_Ysum_Mem_Base<<0;
            value |= (unsigned long long)G_regs->reg_Scl_Para_Error.reg.Gb_Ysum_Mem_Base<<32;
            return value;
        case GB_HS_ERROR:
            return G_regs->reg_Scl_Para_Error.reg.Gb_Hs_Error;
        case GB_VS_ERROR:
            return G_regs->reg_Scl_Para_Error.reg.Gb_Vs_Error;
        case GB_SRC_WIDTH:
            return G_regs->reg_Src_Sf_0.reg.Gb_Src_Width;
        case GB_SEQ_ID_THRD:
            return G_regs->reg_Src_Sf_0.reg.Gb_Seq_Id_Thrd;
        case RESERVED_SRC_SF_0:
            return G_regs->reg_Src_Sf_0.reg.Reserved_Src_Sf_0;
        case RESERVED_SRC_SF_1:
            return G_regs->reg_Src_Sf_1.reg.Reserved_Src_Sf_1;
        case GB_SRC_FMT:
            return G_regs->reg_Src_Sf_1.reg.Gb_Src_Fmt;
        case GB_SRC_TILE:
            return G_regs->reg_Src_Sf_1.reg.Gb_Src_Tile;
        case GB_SRC_HEIGHT:
            return G_regs->reg_Src_Sf_1.reg.Gb_Src_Height;
        case GB_SRC_PITCH:
            return G_regs->reg_Src_Sf_1.reg.Gb_Src_Pitch;
        case GB_SCL_DST_FMT:
            return G_regs->reg_Scl_Dst_Sf_1.reg.Gb_Scl_Dst_Fmt;
        case GB_SCL_DST_HEIGHT:
            return G_regs->reg_Scl_Dst_Sf_1.reg.Gb_Scl_Dst_Height;
        case GB_SCL_DST_PITCH:
            return G_regs->reg_Scl_Dst_Sf_1.reg.Gb_Scl_Dst_Pitch;
        case GB_SCL_DST_COMPRESS_EN:
            return G_regs->reg_Scl_Dst_Sf_1.reg.Gb_Scl_Dst_Compress_En;
        case RESERVED_SCL_DST_SF_1:
            return G_regs->reg_Scl_Dst_Sf_1.reg.Reserved_Scl_Dst_Sf_1;
        case GB_SCL_DST_WIDTH:
            return G_regs->reg_Scl_Dst_Sf_2.reg.Gb_Scl_Dst_Width;
        case RESERVE_DST2:
            return G_regs->reg_Scl_Dst_Sf_2.reg.Reserve_Dst2;
        case RESERVED_RS:
            return G_regs->reg_Vpp_Ref_Sf.reg.Reserved_Rs;
        case GB_VPP_DNREF_SF_PITCH:
            return G_regs->reg_Vpp_Ref_Sf.reg.Gb_Vpp_Dnref_Sf_Pitch;
        case GB_VPP_DNREF_SF_HEIGHT:
            return G_regs->reg_Vpp_Ref_Sf.reg.Gb_Vpp_Dnref_Sf_Height;
        case GB_BLT_MODE:
            return G_regs->reg_Mode_Set.reg.Gb_Blt_Mode;
        case GB_SCL_MODE:
            return G_regs->reg_Mode_Set.reg.Gb_Scl_Mode;
        case GB_EN_HSCL:
            return G_regs->reg_Mode_Set.reg.Gb_En_Hscl;
        case GB_EN_VSCL:
            return G_regs->reg_Mode_Set.reg.Gb_En_Vscl;
        case RESERVED_GB_FIRST_SLICE:
            return G_regs->reg_Mode_Set.reg.Reserved_Gb_First_Slice;
        case GB_SE_VPP:
            return G_regs->reg_Mode_Set.reg.Gb_Se_Vpp;
        case GB_ROTATION_MODE:
            return G_regs->reg_Mode_Set.reg.Gb_Rotation_Mode;
        case GB_SRC_COMPRESS:
            return G_regs->reg_Mode_Set.reg.Gb_Src_Compress;
        case GB_FOR_ECO:
            return G_regs->reg_Mode_Set.reg.Gb_For_Eco;
        case GB_X_UPSAMPLE_MODE:
            return G_regs->reg_Mode_Set.reg.Gb_X_Upsample_Mode;
        case GB_DS_24_X:
            return G_regs->reg_Mode_Set.reg.Gb_Ds_24_X;
        case GB_DS_24_Y:
            return G_regs->reg_Mode_Set.reg.Gb_Ds_24_Y;
        case GB_BLT_SIMPLE:
            return G_regs->reg_Mode_Set.reg.Gb_Blt_Simple;
        case GB_EN_4K_MEM_SWZL:
            return G_regs->reg_Mode_Set.reg.Gb_En_4k_Mem_Swzl;
        case GB_DS4_LSB2FORCE:
            return G_regs->reg_Mode_Set.reg.Gb_Ds4_Lsb2force;
        case GB_FIXCOLOR_MODE:
            return G_regs->reg_Mode_Set.reg.Gb_Fixcolor_Mode;
        case GB_Y_UPSAMPLE_MODE:
            return G_regs->reg_Mode_Set.reg.Gb_Y_Upsample_Mode;
        case RESERVED_MS:
            return G_regs->reg_Mode_Set.reg.Reserved_Ms;
        case RESERVED_GB_SCL_LEFT_EDGE_WIDTH:
            return G_regs->reg_Drv_Mp_Set.reg.Reserved_Gb_Scl_Left_Edge_Width;
        case RESERVED_GB_SCL_RIGHT_EDGE_WIDTH:
            return G_regs->reg_Drv_Mp_Set.reg.Reserved_Gb_Scl_Right_Edge_Width;
        case GB_LDRD_COMPRESS:
            return G_regs->reg_Drv_Mp_Set.reg.Gb_Ldrd_Compress;
        case GB_SEQ_ID_PEAKLEVEL:
            return G_regs->reg_Drv_Mp_Set.reg.Gb_Seq_Id_Peaklevel;
        case RESERVED_DMS:
            return G_regs->reg_Drv_Mp_Set.reg.Reserved_Dms;
        case GB_SLICE_WIDTH:
            return G_regs->reg_Multipass_Set0.reg.Gb_Slice_Width;
        case GB_SLICE_PIXEL_NUM:
            return G_regs->reg_Multipass_Set0.reg.Gb_Slice_Pixel_Num;
        case GB_SLICE_WIDTH_1ST:
            value |= G_regs->reg_Multipass_Set0.reg.Gb_Slice_Width_1st<<0;
            value |= G_regs->reg_Multipass_Set1.reg.Gb_Slice_Width_1st<<4;
            return value;
        case GB_SLICE_PIXEL_NUM_1ST:
            return G_regs->reg_Multipass_Set1.reg.Gb_Slice_Pixel_Num_1st;
        case GB_SIGNAT_SEL_IO:
            return G_regs->reg_Multipass_Set1.reg.Gb_Signat_Sel_Io;
        case GB_SIGNAT_SEL_OO:
            return G_regs->reg_Multipass_Set1.reg.Gb_Signat_Sel_Oo;
        case GB_SIG_ENADDR_IO:
            return G_regs->reg_Multipass_Set1.reg.Gb_Sig_Enaddr_Io;
        case RESERVED_MP1:
            return G_regs->reg_Multipass_Set1.reg.Reserved_Mp1;
        case GB_CL_LEFT:
            return G_regs->reg_Clip_L_R.reg.Gb_Cl_Left;
        case GB_CL_RIGHT:
            return G_regs->reg_Clip_L_R.reg.Gb_Cl_Right;
        case RESERVED17:
            return G_regs->reg_Clip_L_R.reg.Reserved17;
        case GB_CL_TOP:
            return G_regs->reg_Clip_T_B.reg.Gb_Cl_Top;
        case GB_CL_BOTTOM:
            return G_regs->reg_Clip_T_B.reg.Gb_Cl_Bottom;
        case RESERVED16:
            return G_regs->reg_Clip_T_B.reg.Reserved16;
        case GB_VPPCNTOUT:
            return G_regs->reg_Lbuf_Set0.reg.Gb_Vppcntout;
        case GB_VPPARBMODE:
            return G_regs->reg_Lbuf_Set0.reg.Gb_Vpparbmode;
        case GB_LB_SCL_YUV_OFFSET:
            return G_regs->reg_Lbuf_Set0.reg.Gb_Lb_Scl_Yuv_Offset;
        case GB_LB_SCL_A_OFFSET:
            return G_regs->reg_Lbuf_Set0.reg.Gb_Lb_Scl_A_Offset;
        case RESERVED_LS0:
            return G_regs->reg_Lbuf_Set0.reg.Reserved_Ls0;
        case GB_LB_SCL_YUV_PITCH:
            return G_regs->reg_Lbuf_Set0_1.reg.Gb_Lb_Scl_Yuv_Pitch;
        case GB_LB_SCL_A_PITCH:
            return G_regs->reg_Lbuf_Set0_1.reg.Gb_Lb_Scl_A_Pitch;
        case GB_LB_NM_OPER_OFFSET:
            return G_regs->reg_Lbuf_Set0_1.reg.Gb_Lb_Nm_Oper_Offset;
        case RESERVED_LS01:
            return G_regs->reg_Lbuf_Set0_1.reg.Reserved_Ls01;
        case GB_LB_SR_Y_OFFSET:
            return G_regs->reg_Lbuf_Set0_2.reg.Gb_Lb_Sr_Y_Offset;
        case GB_LB_SR_UV_OFFSET:
            return G_regs->reg_Lbuf_Set0_2.reg.Gb_Lb_Sr_Uv_Offset;
        case GB_LB_SR_Y_PITCH:
            return G_regs->reg_Lbuf_Set0_2.reg.Gb_Lb_Sr_Y_Pitch;
        case GB_LB_SR_A_OFFSET:
            return G_regs->reg_Lbuf_Set0_3.reg.Gb_Lb_Sr_A_Offset;
        case GB_LB_SR_UV_PITCH:
            return G_regs->reg_Lbuf_Set0_3.reg.Gb_Lb_Sr_Uv_Pitch;
        case GB_LB_SR_A_PITCH:
            return G_regs->reg_Lbuf_Set0_3.reg.Gb_Lb_Sr_A_Pitch;
        case RESERVED_LS03:
            return G_regs->reg_Lbuf_Set0_3.reg.Reserved_Ls03;
        case GB_LBADIW:
            return G_regs->reg_Lbuf_Set1.reg.Gb_Lbadiw;
        case GB_LBASCLYUVW:
            return G_regs->reg_Lbuf_Set1.reg.Gb_Lbasclyuvw;
        case GB_LBASCLAW:
            return G_regs->reg_Lbuf_Set1.reg.Gb_Lbasclaw;
        case GB_LBASRUVW:
            return G_regs->reg_Lbuf_Set1.reg.Gb_Lbasruvw;
        case GB_LBA_BANK_MODE:
            return G_regs->reg_Lbuf_Set1.reg.Gb_Lba_Bank_Mode;
        case RESERVED_LS1:
            return G_regs->reg_Lbuf_Set1.reg.Reserved_Ls1;
        case GB_LBASRYW:
            return G_regs->reg_Lbuf_Set2.reg.Gb_Lbasryw;
        case GB_LBASRAW:
            return G_regs->reg_Lbuf_Set2.reg.Gb_Lbasraw;
        case RESERVED_LS2:
            return G_regs->reg_Lbuf_Set2.reg.Reserved_Ls2;
        case GB_LBADIR:
            return G_regs->reg_Lbuf_Set3.reg.Gb_Lbadir;
        case GB_LBADIREFR:
            return G_regs->reg_Lbuf_Set3.reg.Gb_Lbadirefr;
        case GB_LBASCRR:
            return G_regs->reg_Lbuf_Set3.reg.Gb_Lbascrr;
        case RESERVED_LS3:
            return G_regs->reg_Lbuf_Set3.reg.Reserved_Ls3;
        case GB_LBASCLYUVR:
            return G_regs->reg_Lbuf_Set4.reg.Gb_Lbasclyuvr;
        case GB_LBASCLAR:
            return G_regs->reg_Lbuf_Set4.reg.Gb_Lbasclar;
        case GB_LBASRUVR:
            return G_regs->reg_Lbuf_Set4.reg.Gb_Lbasruvr;
        case RESERVED_LS4:
            return G_regs->reg_Lbuf_Set4.reg.Reserved_Ls4;
        case GB_LBASRYR:
            return G_regs->reg_Lbuf_Set5.reg.Gb_Lbasryr;
        case GB_LBASRAR:
            return G_regs->reg_Lbuf_Set5.reg.Gb_Lbasrar;
        case RESERVED_LS5:
            return G_regs->reg_Lbuf_Set5.reg.Reserved_Ls5;
        case GB_MOB_FIXALPHA:
            return G_regs->reg_Img_Related.reg.Gb_Mob_Fixalpha;
        case GB_REDUNDANT_EDGE_WIDTH:
            return G_regs->reg_Img_Related.reg.Gb_Redundant_Edge_Width;
        case GB_MOB_RANGE_MIN:
            return G_regs->reg_Img_Related.reg.Gb_Mob_Range_Min;
        case GB_MOB_RANGE_MAXRY:
            return G_regs->reg_Img_Related.reg.Gb_Mob_Range_Maxry;
        case GB_MOB_RANGE_MAXGBUV:
            return G_regs->reg_Img_Related.reg.Gb_Mob_Range_Maxgbuv;
        case GB_SRC_LOSSY_COM_EN:
            return G_regs->reg_Img_Related.reg.Gb_Src_Lossy_Com_En;
        case GB_DST_LOSSY_COM_EN:
            return G_regs->reg_Img_Related.reg.Gb_Dst_Lossy_Com_En;
        case GB_MOB_BLENDING_EN:
            return G_regs->reg_Img_Related.reg.Gb_Mob_Blending_En;
        case GB_MOB_BLENDING_DIU_MODE:
            return G_regs->reg_Img_Related.reg.Gb_Mob_Blending_Diu_Mode;
        case GB_MOB_BLENDING_STREAM_MODE:
            return G_regs->reg_Img_Related.reg.Gb_Mob_Blending_Stream_Mode;
        case RESERVED_IR:
            return G_regs->reg_Img_Related.reg.Reserved_Ir;
        case GB_HSLC_Y_A0:
            return G_regs->reg_Hslc_Set0.reg.Gb_Hslc_Y_A0;
        case GB_HSLC_Y_B0:
            return G_regs->reg_Hslc_Set0.reg.Gb_Hslc_Y_B0;
        case RESERVED_HSLC0:
            return G_regs->reg_Hslc_Set0.reg.Reserved_Hslc0;
        case GB_HSLC_UV_A0:
            return G_regs->reg_Hslc_Set1.reg.Gb_Hslc_Uv_A0;
        case GB_HSLC_UV_A1:
            return G_regs->reg_Hslc_Set1.reg.Gb_Hslc_Uv_A1;
        case RESERVED_HSLC1:
            return G_regs->reg_Hslc_Set1.reg.Reserved_Hslc1;
        case GB_HSLC_U_B0:
            return G_regs->reg_Hslc_Set2.reg.Gb_Hslc_U_B0;
        case GB_HSLC_V_B0:
            return G_regs->reg_Hslc_Set2.reg.Gb_Hslc_V_B0;
        case RESERVED_HSLC2:
            return G_regs->reg_Hslc_Set2.reg.Reserved_Hslc2;
        case GB_SD_WIDTH_RATIO:
            return G_regs->reg_Scl_Para_Sd.reg.Gb_Sd_Width_Ratio;
        case GB_SD_HEIGHT_RATIO:
            return G_regs->reg_Scl_Para_Sd.reg.Gb_Sd_Height_Ratio;
        case GB_DS_WIDTH_RATIO:
            return G_regs->reg_Scl_Para_Ds.reg.Gb_Ds_Width_Ratio;
        case GB_DS_HEIGHT_RATIO:
            return G_regs->reg_Scl_Para_Ds.reg.Gb_Ds_Height_Ratio;
        case RESERVED_DS:
            return G_regs->reg_Scl_Para_Ds.reg.Reserved_Ds;
        case GB_SRC_EN_ALPHA:
            return G_regs->reg_Scl_0.reg.Gb_Src_En_Alpha;
        case GB_SCL_BC_DELTA:
            return G_regs->reg_Scl_0.reg.Gb_Scl_Bc_Delta;
        case GB_EN_SCL_MAXMIN:
            return G_regs->reg_Scl_0.reg.Gb_En_Scl_Maxmin;
        case GB_EN_SR:
            return G_regs->reg_Scl_0.reg.Gb_En_Sr;
        case GB_SR_CTDIFFTH:
            return G_regs->reg_Scl_0.reg.Gb_Sr_Ctdiffth;
        case GB_EN_PIXELTYPE:
            return G_regs->reg_Scl_0.reg.Gb_En_Pixeltype;
        case GB_SR_BLENDSTRENGTH:
            return G_regs->reg_Scl_0.reg.Gb_Sr_Blendstrength;
        case GB_SR_ENCTBLEND:
            return G_regs->reg_Scl_0.reg.Gb_Sr_Enctblend;
        case GB_SR_BLENDWEIGHT:
            return G_regs->reg_Scl_0.reg.Gb_Sr_Blendweight;
        case GB_SR_COEFWEIGHT:
            return G_regs->reg_Scl_0.reg.Gb_Sr_Coefweight;
        case GB_SR_COEFWEIGHTOTHER:
            return G_regs->reg_Scl_0.reg.Gb_Sr_Coefweightother;
        case RESERVED_SCL0:
            return G_regs->reg_Scl_0.reg.Reserved_Scl0;
        case GB_CSCF_M_00:
            return G_regs->reg_Cscf_00_01.reg.Gb_Cscf_M_00;
        case GB_CSCF_M_01:
            return G_regs->reg_Cscf_00_01.reg.Gb_Cscf_M_01;
        case GB_CSCF_EN:
            return G_regs->reg_Cscf_00_01.reg.Gb_Cscf_En;
        case GB_CSCF_ROUND:
            return G_regs->reg_Cscf_00_01.reg.Gb_Cscf_Round;
        case RESERVED_CSCF_00_01:
            return G_regs->reg_Cscf_00_01.reg.Reserved_Cscf_00_01;
        case GB_CSCF_M_02:
            return G_regs->reg_Cscf_02_10.reg.Gb_Cscf_M_02;
        case GB_CSCF_M_10:
            return G_regs->reg_Cscf_02_10.reg.Gb_Cscf_M_10;
        case RESERVED_CSCF_02_10:
            return G_regs->reg_Cscf_02_10.reg.Reserved_Cscf_02_10;
        case GB_CSCF_M_11:
            return G_regs->reg_Cscf_11_12.reg.Gb_Cscf_M_11;
        case GB_CSCF_M_12:
            return G_regs->reg_Cscf_11_12.reg.Gb_Cscf_M_12;
        case RESERVED_CSCF_11_12:
            return G_regs->reg_Cscf_11_12.reg.Reserved_Cscf_11_12;
        case GB_CSCF_M_20:
            return G_regs->reg_Cscf_20_21.reg.Gb_Cscf_M_20;
        case GB_CSCF_M_21:
            return G_regs->reg_Cscf_20_21.reg.Gb_Cscf_M_21;
        case RESERVED_CSCF_20_21:
            return G_regs->reg_Cscf_20_21.reg.Reserved_Cscf_20_21;
        case GB_CSCF_M_22:
            return G_regs->reg_Cscf_22.reg.Gb_Cscf_M_22;
        case RESERVED_CSCF_22:
            return G_regs->reg_Cscf_22.reg.Reserved_Cscf_22;
        case GB_CSCF_OFF_00:
            return G_regs->reg_Cscf_Off_00.reg.Gb_Cscf_Off_00;
        case RESERVED_CSCF_OFF_00:
            return G_regs->reg_Cscf_Off_00.reg.Reserved_Cscf_Off_00;
        case GB_CSCF_OFF_01:
            return G_regs->reg_Cscf_Off_01.reg.Gb_Cscf_Off_01;
        case RESERVED_CSCF_OFF_01:
            return G_regs->reg_Cscf_Off_01.reg.Reserved_Cscf_Off_01;
        case GB_CSCF_OFF_02:
            return G_regs->reg_Cscf_Off_02.reg.Gb_Cscf_Off_02;
        case RESERVED_CSCF_OFF_02:
            return G_regs->reg_Cscf_Off_02.reg.Reserved_Cscf_Off_02;
        case GB_CSCB_M_00:
            return G_regs->reg_Cscb_00_01.reg.Gb_Cscb_M_00;
        case GB_CSCB_M_01:
            return G_regs->reg_Cscb_00_01.reg.Gb_Cscb_M_01;
        case GB_CSCB_EN:
            return G_regs->reg_Cscb_00_01.reg.Gb_Cscb_En;
        case GB_CSCB_ROUND:
            return G_regs->reg_Cscb_00_01.reg.Gb_Cscb_Round;
        case RESERVED_CSCB_00_01:
            return G_regs->reg_Cscb_00_01.reg.Reserved_Cscb_00_01;
        case GB_CSCB_M_02:
            return G_regs->reg_Cscb_02_10.reg.Gb_Cscb_M_02;
        case GB_CSCB_M_10:
            return G_regs->reg_Cscb_02_10.reg.Gb_Cscb_M_10;
        case RESERVED_CSCB_02_10:
            return G_regs->reg_Cscb_02_10.reg.Reserved_Cscb_02_10;
        case GB_CSCB_M_11:
            return G_regs->reg_Cscb_11_12.reg.Gb_Cscb_M_11;
        case GB_CSCB_M_12:
            return G_regs->reg_Cscb_11_12.reg.Gb_Cscb_M_12;
        case RESERVED_CSCB_11_12:
            return G_regs->reg_Cscb_11_12.reg.Reserved_Cscb_11_12;
        case GB_CSCB_M_20:
            return G_regs->reg_Cscb_20_21.reg.Gb_Cscb_M_20;
        case GB_CSCB_M_21:
            return G_regs->reg_Cscb_20_21.reg.Gb_Cscb_M_21;
        case RESERVED_CSCB_20_21:
            return G_regs->reg_Cscb_20_21.reg.Reserved_Cscb_20_21;
        case GB_CSCB_M_22:
            return G_regs->reg_Cscb_22.reg.Gb_Cscb_M_22;
        case RESERVED_CSCB_22:
            return G_regs->reg_Cscb_22.reg.Reserved_Cscb_22;
        case GB_CSCB_OFF_00:
            return G_regs->reg_Cscb_Off_00.reg.Gb_Cscb_Off_00;
        case RESERVED_CSCB_OFF_00:
            return G_regs->reg_Cscb_Off_00.reg.Reserved_Cscb_Off_00;
        case GB_CSCB_OFF_01:
            return G_regs->reg_Cscb_Off_01.reg.Gb_Cscb_Off_01;
        case RESERVED_CSCB_OFF_01:
            return G_regs->reg_Cscb_Off_01.reg.Reserved_Cscb_Off_01;
        case GB_CSCB_OFF_02:
            return G_regs->reg_Cscb_Off_02.reg.Gb_Cscb_Off_02;
        case RESERVED_CSCB_OFF_02:
            return G_regs->reg_Cscb_Off_02.reg.Reserved_Cscb_Off_02;
        case GB_FIXCOLOR_VALUE:
            return G_regs->reg_Ld_Bld_Fixcolor.reg.Gb_Fixcolor_Value;
        case GB_LB_CPT_OFFSET:
            return G_regs->reg_Cpt_0.reg.Gb_Lb_Cpt_Offset;
        case GB_LB_CPT_SIZE:
            return G_regs->reg_Cpt_0.reg.Gb_Lb_Cpt_Size;
        case GB_LB_MOB_L2T_PITCH:
            return G_regs->reg_Cpt_0.reg.Gb_Lb_Mob_L2t_Pitch;
        case GB_DST_END:
            return G_regs->reg_Cpt_1.reg.Gb_Dst_End;
        case GB_DST_TILE:
            return G_regs->reg_Cpt_1.reg.Gb_Dst_Tile;
        case GB_LB_MOB_L2T_OFFSET:
            return G_regs->reg_Cpt_1.reg.Gb_Lb_Mob_L2t_Offset;
        case GB_MOB_DST_ALPHAMODE:
            return G_regs->reg_Cpt_1.reg.Gb_Mob_Dst_Alphamode;
        case GB_MOB_BLENDING_MODE:
            return G_regs->reg_Cpt_1.reg.Gb_Mob_Blending_Mode;
        case GB_MOB_BLENDING_METHOD:
            return G_regs->reg_Cpt_1.reg.Gb_Mob_Blending_Method;
        case GB_MOB_BLENDING_ROUNDING:
            return G_regs->reg_Cpt_1.reg.Gb_Mob_Blending_Rounding;
        case GB_CPT_DNSAMPLE_MODE:
            return G_regs->reg_Cpt_2.reg.Gb_Cpt_Dnsample_Mode;
        case GB_MOB_BLDALPHA:
            return G_regs->reg_Cpt_2.reg.Gb_Mob_Bldalpha;
        case GB_CPT_EN:
            return G_regs->reg_Cpt_2.reg.Gb_Cpt_En;
        case GB_DST_SF_WIDTH:
            return G_regs->reg_Cpt_2.reg.Gb_Dst_Sf_Width;
        case RESERVED_CPT2:
            return G_regs->reg_Cpt_2.reg.Reserved_Cpt2;
        case GB_MIFARB_REG:
            return G_regs->reg_Mif_Arbiter_0.reg.Gb_Mifarb_Reg;
        case GB_MIFARB_YDIFF:
            return G_regs->reg_Mif_Arbiter_0.reg.Gb_Mifarb_Ydiff;
        case GB_MIFARB_LD:
            return G_regs->reg_Mif_Arbiter_0.reg.Gb_Mifarb_Ld;
        case RESERVED_MA0:
            return G_regs->reg_Mif_Arbiter_0.reg.Reserved_Ma0;
        case GB_MIFARB_CPTR:
            return G_regs->reg_Mif_Arbiter_1.reg.Gb_Mifarb_Cptr;
        case GB_MIFARB_MTDR:
            return G_regs->reg_Mif_Arbiter_1.reg.Gb_Mifarb_Mtdr;
        case GB_MIFARB_MOB:
            return G_regs->reg_Mif_Arbiter_1.reg.Gb_Mifarb_Mob;
        case GB_MIFARB_PASSIVE:
            return G_regs->reg_Mif_Arbiter_1.reg.Gb_Mifarb_Passive;
        case RESERVED_MA1:
            return G_regs->reg_Mif_Arbiter_1.reg.Reserved_Ma1;
        case GB_MIFARB_MTDW:
            return G_regs->reg_Mif_Arbiter_2.reg.Gb_Mifarb_Mtdw;
        case GB_MIFARB_HDRR:
            return G_regs->reg_Mif_Arbiter_2.reg.Gb_Mifarb_Hdrr;
        case GB_MIFARB_HDRW:
            return G_regs->reg_Mif_Arbiter_2.reg.Gb_Mifarb_Hdrw;
        case RESERVED_MA2:
            return G_regs->reg_Mif_Arbiter_2.reg.Reserved_Ma2;
        case GB_SRC_BL_SLOT_IDX:
            return G_regs->reg_Mif_Slot_Idx_0.reg.Gb_Src_Bl_Slot_Idx;
        case GB_OTH_BL_SLOT_IDX:
            value |= G_regs->reg_Mif_Slot_Idx_0.reg.Gb_Oth_Bl_Slot_Idx<<0;
            value |= G_regs->reg_Mif_Slot_Idx_1.reg.Gb_Oth_Bl_Slot_Idx<<14;
            return value;
        case GB_REF_BL_SLOT_IDX:
            return G_regs->reg_Mif_Slot_Idx_1.reg.Gb_Ref_Bl_Slot_Idx;
        case GB_DST_BL_SLOT_IDX:
            value |= G_regs->reg_Mif_Slot_Idx_1.reg.Gb_Dst_Bl_Slot_Idx<<0;
            value |= G_regs->reg_Mif_Slot_Idx_2.reg.Gb_Dst_Bl_Slot_Idx<<10;
            return value;
        case GB_SCL_DST_BL_SLOT_IDX:
            return G_regs->reg_Mif_Slot_Idx_2.reg.Gb_Scl_Dst_Bl_Slot_Idx;
        case RESERVED_MSI2:
            return G_regs->reg_Mif_Slot_Idx_2.reg.Reserved_Msi2;
        case GB_SRC_BOT_BL_SLOT_IDX:
            return G_regs->reg_Mif_Slot_Idx_3.reg.Gb_Src_Bot_Bl_Slot_Idx;
        case GB_OTH_BOT_BL_SLOT_IDX:
            value |= G_regs->reg_Mif_Slot_Idx_3.reg.Gb_Oth_Bot_Bl_Slot_Idx<<0;
            value |= G_regs->reg_Mif_Slot_Idx_4.reg.Gb_Oth_Bot_Bl_Slot_Idx<<14;
            return value;
        case GB_REF_BOT_BL_SLOT_IDX:
            return G_regs->reg_Mif_Slot_Idx_4.reg.Gb_Ref_Bot_Bl_Slot_Idx;
        case RESERVED_MSI4:
            return G_regs->reg_Mif_Slot_Idx_4.reg.Reserved_Msi4;
        case GB_SRC_PROC_ID:
            return G_regs->reg_Mif_Pid_0.reg.Gb_Src_Proc_Id;
        case GB_OTH_PROC_ID:
            return G_regs->reg_Mif_Pid_0.reg.Gb_Oth_Proc_Id;
        case GB_REF_PROC_ID:
            return G_regs->reg_Mif_Pid_0.reg.Gb_Ref_Proc_Id;
        case GB_SRC_BOT_PROC_ID:
            return G_regs->reg_Mif_Pid_0.reg.Gb_Src_Bot_Proc_Id;
        case GB_OTH_BOT_PROC_ID:
            return G_regs->reg_Mif_Pid_0.reg.Gb_Oth_Bot_Proc_Id;
        case GB_REF_BOT_PROC_ID:
            return G_regs->reg_Mif_Pid_0.reg.Gb_Ref_Bot_Proc_Id;
        case GB_DST_PROC_ID:
            return G_regs->reg_Mif_Pid_0.reg.Gb_Dst_Proc_Id;
        case GB_SCL_DST_PROC_ID:
            return G_regs->reg_Mif_Pid_0.reg.Gb_Scl_Dst_Proc_Id;
        case GB_MTD_HIST_PROC_ID:
            return G_regs->reg_Mif_Pid_1.reg.Gb_Mtd_Hist_Proc_Id;
        case GB_HDR_YSUM_PROC_ID:
            return G_regs->reg_Mif_Pid_1.reg.Gb_Hdr_Ysum_Proc_Id;
        case GB_HDR_INT_PROC_ID:
            return G_regs->reg_Mif_Pid_1.reg.Gb_Hdr_Int_Proc_Id;
        case RESERVED_MPID1:
            return G_regs->reg_Mif_Pid_1.reg.Reserved_Mpid1;
        case GB_DI_INTRA_TH:
            return G_regs->reg_Di_0.reg.Gb_Di_Intra_Th;
        case GB_DI_ISFEATHER_TH:
            return G_regs->reg_Di_0.reg.Gb_Di_Isfeather_Th;
        case GB_DI_FORCE_EELA_TH:
            return G_regs->reg_Di_0.reg.Gb_Di_Force_Eela_Th;
        case RESERVED_DI0:
            return G_regs->reg_Di_0.reg.Reserved_Di0;
        case GB_DI_SCROLL_TH:
            return G_regs->reg_Di_1.reg.Gb_Di_Scroll_Th;
        case GB_POINT_DEGREE_VAR_K:
            return G_regs->reg_Di_1.reg.Gb_Point_Degree_Var_K;
        case RESERVED_DI1:
            return G_regs->reg_Di_1.reg.Reserved_Di1;
        case GB_SOBEL_TH:
            return G_regs->reg_Di_2.reg.Gb_Sobel_Th;
        case GB_SOBEL_FORCE_VALUE:
            return G_regs->reg_Di_2.reg.Gb_Sobel_Force_Value;
        case GB_DI_FEATHER_EDGE_CNT_X1:
            return G_regs->reg_Di_2.reg.Gb_Di_Feather_Edge_Cnt_X1;
        case GB_DI_FEATHER_EDGE_CNT_Y1:
            return G_regs->reg_Di_2.reg.Gb_Di_Feather_Edge_Cnt_Y1;
        case RESERVED_DI2:
            return G_regs->reg_Di_2.reg.Reserved_Di2;
        case GB_DI_IAD_MD_X1:
            return G_regs->reg_Di_3.reg.Gb_Di_Iad_Md_X1;
        case GB_DI_IAD_MD_X2:
            return G_regs->reg_Di_3.reg.Gb_Di_Iad_Md_X2;
        case GB_DI_IAD_MD_Y1:
            return G_regs->reg_Di_4.reg.Gb_Di_Iad_Md_Y1;
        case GB_DI_IAD_MD_K:
            return G_regs->reg_Di_4.reg.Gb_Di_Iad_Md_K;
        case RESERVED_DI4:
            return G_regs->reg_Di_4.reg.Reserved_Di4;
        case GB_POINT_DEGREE_VAR_X1:
            return G_regs->reg_Di_5.reg.Gb_Point_Degree_Var_X1;
        case GB_POINT_DEGREE_VAR_X2:
            return G_regs->reg_Di_5.reg.Gb_Point_Degree_Var_X2;
        case GB_POINT_DEGREE_VAR_Y1:
            return G_regs->reg_Di_5.reg.Gb_Point_Degree_Var_Y1;
        case GB_TOP_BOT_FIELD_DIV:
            return G_regs->reg_Di_6.reg.Gb_Top_Bot_Field_Div;
        case GB_DI_ENABLE_STATICFLAG:
            return G_regs->reg_Di_6.reg.Gb_Di_Enable_Staticflag;
        case GB_DI_FEATHER_EDGE_CNT_K:
            return G_regs->reg_Di_6.reg.Gb_Di_Feather_Edge_Cnt_K;
        case GB_DI_FEATHER_EDGE_CNT_X2:
            return G_regs->reg_Di_6.reg.Gb_Di_Feather_Edge_Cnt_X2;
        case GB_FD_WAIT_FRAME:
            return G_regs->reg_Di_6.reg.Gb_Fd_Wait_Frame;
        case RESERVED_DI6:
            return G_regs->reg_Di_6.reg.Reserved_Di6;
        case GB_EDGE_FLAG_TH:
            return G_regs->reg_Di_7.reg.Gb_Edge_Flag_Th;
        case GB_DI_INTRA_DIFF_TH:
            return G_regs->reg_Di_7.reg.Gb_Di_Intra_Diff_Th;
        case RESERVED_DI7:
            return G_regs->reg_Di_7.reg.Reserved_Di7;
        case GB_USE_MV_JUDGE:
            return G_regs->reg_Di_8.reg.Gb_Use_Mv_Judge;
        case GB_USE_ALLMV:
            return G_regs->reg_Di_8.reg.Gb_Use_Allmv;
        case GB_MV_DIFF_TH:
            return G_regs->reg_Di_8.reg.Gb_Mv_Diff_Th;
        case GB_DIRECTION_JUDGE_TH:
            return G_regs->reg_Di_8.reg.Gb_Direction_Judge_Th;
        case GB_FD_BAD_MERGE_TH_1:
            return G_regs->reg_Di_8.reg.Gb_Fd_Bad_Merge_Th_1;
        case GB_FD_BAD_MERGE_TH_2:
            return G_regs->reg_Di_8.reg.Gb_Fd_Bad_Merge_Th_2;
        case RESERVED_DI8:
            return G_regs->reg_Di_8.reg.Reserved_Di8;
        case GB_DI_MD_AREA_TH0:
            return G_regs->reg_Di_9.reg.Gb_Di_Md_Area_Th0;
        case GB_DI_MD_AREA_TH1:
            return G_regs->reg_Di_9.reg.Gb_Di_Md_Area_Th1;
        case GB_DI_MD_AREA_COEF0:
            return G_regs->reg_Di_A.reg.Gb_Di_Md_Area_Coef0;
        case GB_DI_MD_AREA_COEF1:
            return G_regs->reg_Di_A.reg.Gb_Di_Md_Area_Coef1;
        case GB_DI_MD_ADJ_COEF0:
            return G_regs->reg_Di_A.reg.Gb_Di_Md_Adj_Coef0;
        case GB_DI_MD_ADJ_COEF1:
            return G_regs->reg_Di_A.reg.Gb_Di_Md_Adj_Coef1;
        case RESERVED_DIA:
            return G_regs->reg_Di_A.reg.Reserved_Dia;
        case GB_DI_IAD_AVE_ADJ_TH0:
            return G_regs->reg_Di_B.reg.Gb_Di_Iad_Ave_Adj_Th0;
        case GB_DI_IAD_AVE_ADJ_TH1:
            return G_regs->reg_Di_B.reg.Gb_Di_Iad_Ave_Adj_Th1;
        case RESERVED_DIB:
            return G_regs->reg_Di_B.reg.Reserved_Dib;
        case GB_DI_IAD_AVE_ADJ_VAL:
            return G_regs->reg_Di_C.reg.Gb_Di_Iad_Ave_Adj_Val;
        case GB_DI_IAD_AVE_ADJ_COEF:
            return G_regs->reg_Di_C.reg.Gb_Di_Iad_Ave_Adj_Coef;
        case GB_DI_MTD_FEATHERDETECT_EN:
            return G_regs->reg_Di_C.reg.Gb_Di_Mtd_Featherdetect_En;
        case RESERVED_DIC:
            return G_regs->reg_Di_C.reg.Reserved_Dic;
        case GB_DI_IAD_LINE_MD_X1:
            return G_regs->reg_Di_D.reg.Gb_Di_Iad_Line_Md_X1;
        case GB_DI_IAD_LINE_MD_X2:
            return G_regs->reg_Di_D.reg.Gb_Di_Iad_Line_Md_X2;
        case GB_DI_MTD_FEATHERPARA:
            return G_regs->reg_Di_E.reg.Gb_Di_Mtd_Featherpara;
        case GB_DI_IAD_LINE_MD_Y1:
            return G_regs->reg_Di_E.reg.Gb_Di_Iad_Line_Md_Y1;
        case GB_DI_IAD_LINE_MD_K:
            return G_regs->reg_Di_E.reg.Gb_Di_Iad_Line_Md_K;
        case GB_DI_TH:
            return G_regs->reg_Vpp_Control_0.reg.Gb_Di_Th;
        case RESERVED_VC01:
            return G_regs->reg_Vpp_Control_0.reg.Reserved_Vc01;
        case GB_MD_TH2:
            return G_regs->reg_Vpp_Control_0.reg.Gb_Md_Th2;
        case GB_MD_TH2_EN:
            return G_regs->reg_Vpp_Control_0.reg.Gb_Md_Th2_En;
        case RESERVED_VC0:
            return G_regs->reg_Vpp_Control_0.reg.Reserved_Vc0;
        case GB_MD_TH:
            return G_regs->reg_Vpp_Control_3.reg.Gb_Md_Th;
        case GB_DI_SR_REV_DIFF_TH:
            return G_regs->reg_Vpp_Control_3.reg.Gb_Di_Sr_Rev_Diff_Th;
        case RESERVED_VC3:
            return G_regs->reg_Vpp_Control_3.reg.Reserved_Vc3;
        case GB_DI_MTD_EDGEVALTHR:
            return G_regs->reg_Vpp_Control_4.reg.Gb_Di_Mtd_Edgevalthr;
        case GB_DI_MTD_STATICPARA:
            return G_regs->reg_Vpp_Control_4.reg.Gb_Di_Mtd_Staticpara;
        case GB_DI_MTD_MAXMINTHR:
            return G_regs->reg_Vpp_Control_4.reg.Gb_Di_Mtd_Maxminthr;
        case GB_DI_SR_GRAD_TH:
            return G_regs->reg_Vpp_Control_4.reg.Gb_Di_Sr_Grad_Th;
        case GB_DI_MTD_MD_TH:
            return G_regs->reg_Vpp_Control_4.reg.Gb_Di_Mtd_Md_Th;
        case RESERVED_VC4:
            return G_regs->reg_Vpp_Control_4.reg.Reserved_Vc4;
        case GB_DI_SR_LINE_DIFF_TH:
            return G_regs->reg_Vpp_Control_5.reg.Gb_Di_Sr_Line_Diff_Th;
        case GB_DI_IAD_MD_AVE_TH:
            return G_regs->reg_Vpp_Control_5.reg.Gb_Di_Iad_Md_Ave_Th;
        case RESERVED_VC12:
            return G_regs->reg_Vpp_Control_5.reg.Reserved_Vc12;
        case GB_MTD_COL_HIST_BASE:
            return G_regs->reg_Vpp_Control_6.reg.Gb_Mtd_Col_Hist_Base;
        case GB_MD_LOW_TH:
            return G_regs->reg_Vpp_Control_9.reg.Gb_Md_Low_Th;
        case RESERVED_VC9:
            return G_regs->reg_Vpp_Control_9.reg.Reserved_Vc9;
        case GB_LBUF_SIZE:
            return G_regs->reg_Vpp_Lbuf_0.reg.Gb_Lbuf_Size;
        case GB_VPP_IN_SIZE:
            return G_regs->reg_Vpp_Lbuf_0.reg.Gb_Vpp_In_Size;
        case GB_DISABLE_LCID_BLEND:
            return G_regs->reg_Vpp_Lbuf_0.reg.Gb_Disable_Lcid_Blend;
        case GB_FD_FORCE_DI:
            return G_regs->reg_Vpp_Lbuf_0.reg.Gb_Fd_Force_Di;
        case GB_DI_BLEND_COEF:
            return G_regs->reg_Vpp_Lbuf_0.reg.Gb_Di_Blend_Coef;
        case RESERVED_VL0:
            return G_regs->reg_Vpp_Lbuf_0.reg.Reserved_Vl0;
        case GB_VPP_DI_OFFSET:
            return G_regs->reg_Vpp_Lbuf_1.reg.Gb_Vpp_Di_Offset;
        case GB_VPP_DI_SIZE:
            return G_regs->reg_Vpp_Lbuf_1.reg.Gb_Vpp_Di_Size;
        case GB_VPP_DIREF_IN_OFFSET:
            return G_regs->reg_Vpp_Lbuf_1.reg.Gb_Vpp_Diref_In_Offset;
        case RESERVED_VL1:
            return G_regs->reg_Vpp_Lbuf_1.reg.Reserved_Vl1;
        case GB_FD_MD_TH:
            return G_regs->reg_Vpp_Fd_Control_0.reg.Gb_Fd_Md_Th;
        case GB_SCENE_CHANGE_TOP:
            return G_regs->reg_Vpp_Fd_Control_0.reg.Gb_Scene_Change_Top;
        case GB_SCENE_CHANGE_BOTTOM:
            return G_regs->reg_Vpp_Fd_Control_0.reg.Gb_Scene_Change_Bottom;
        case GB_2V2_TH_DELTA:
            return G_regs->reg_Vpp_Fd_Control_1.reg.Gb_2v2_Th_Delta;
        case GB_2V2_TH_RATIO:
            return G_regs->reg_Vpp_Fd_Control_1.reg.Gb_2v2_Th_Ratio;
        case GB_2V2_SAD_COUNTER_TH:
            return G_regs->reg_Vpp_Fd_Control_1.reg.Gb_2v2_Sad_Counter_Th;
        case GB_2V2_MD_FAILED_COUNTER_TH:
            return G_regs->reg_Vpp_Fd_Control_1.reg.Gb_2v2_Md_Failed_Counter_Th;
        case GB_LCID_DIFF_TH:
            return G_regs->reg_Vpp_Fd_Control_1.reg.Gb_Lcid_Diff_Th;
        case GB_2V2_COUNT_MODE:
            return G_regs->reg_Vpp_Fd_Control_1.reg.Gb_2v2_Count_Mode;
        case GB_SCENE_CHANGE_TH:
            return G_regs->reg_Vpp_Fd_Control_2.reg.Gb_Scene_Change_Th;
        case GB_SCENECHANGE_MD_MODE:
            return G_regs->reg_Vpp_Fd_Control_2.reg.Gb_Scenechange_Md_Mode;
        case GB_FD_MD_TH2:
            return G_regs->reg_Vpp_Fd_Control_2.reg.Gb_Fd_Md_Th2;
        case GB_FD_COUNTER_TH:
            return G_regs->reg_Vpp_Fd_Control_3.reg.Gb_Fd_Counter_Th;
        case GB_FD_DETECT_COUNTER_INITIAL:
            return G_regs->reg_Vpp_Fd_Control_3.reg.Gb_Fd_Detect_Counter_Initial;
        case GB_FD_STRONG_COUNTER_INITIAL:
            return G_regs->reg_Vpp_Fd_Control_3.reg.Gb_Fd_Strong_Counter_Initial;
        case GB_2V2_FAILED_COUNTER_TH:
            return G_regs->reg_Vpp_Fd_Control_3.reg.Gb_2v2_Failed_Counter_Th;
        case GB_2V2_MD_SUCCEED_COUNTER_TH:
            return G_regs->reg_Vpp_Fd_Control_3.reg.Gb_2v2_Md_Succeed_Counter_Th;
        case GB_2V2_SAD_SUCCEED_COUNTER_TH:
            return G_regs->reg_Vpp_Fd_Control_3.reg.Gb_2v2_Sad_Succeed_Counter_Th;
        case GB_2V2_SAD_FAILED_RANGE:
            return G_regs->reg_Vpp_Fd_Control_3.reg.Gb_2v2_Sad_Failed_Range;
        case GB_2V2_FRAME_CNT_TH:
            return G_regs->reg_Vpp_Fd_Control_4.reg.Gb_2v2_Frame_Cnt_Th;
        case GB_2V2_FAILED_COUNTER_TH2:
            return G_regs->reg_Vpp_Fd_Control_4.reg.Gb_2v2_Failed_Counter_Th2;
        case GB_FD_2V2_SAD_CLR_TH_1:
            return G_regs->reg_Vpp_Fd_Control_4.reg.Gb_Fd_2v2_Sad_Clr_Th_1;
        case GB_FD_2V2_SAD_CLR_TH_2:
            return G_regs->reg_Vpp_Fd_Control_4.reg.Gb_Fd_2v2_Sad_Clr_Th_2;
        case GB_FD_2V2_SAD_CLR_TH_3:
            return G_regs->reg_Vpp_Fd_Control_4.reg.Gb_Fd_2v2_Sad_Clr_Th_3;
        case RESERVED_FD4:
            return G_regs->reg_Vpp_Fd_Control_4.reg.Reserved_Fd4;
        case GB_FD_BAD_COPY_TH_1:
            return G_regs->reg_Vpp_Fd_Control_5.reg.Gb_Fd_Bad_Copy_Th_1;
        case GB_FD_BAD_COPY_TH_2:
            return G_regs->reg_Vpp_Fd_Control_5.reg.Gb_Fd_Bad_Copy_Th_2;
        case GB_FD_BAD_COPY_TH_3:
            return G_regs->reg_Vpp_Fd_Control_5.reg.Gb_Fd_Bad_Copy_Th_3;
        case GB_FD_BAD_COPY_TH_4:
            return G_regs->reg_Vpp_Fd_Control_5.reg.Gb_Fd_Bad_Copy_Th_4;
        case GB_FD_QUIT_2V2_TH_1:
            return G_regs->reg_Vpp_Fd_Control_5.reg.Gb_Fd_Quit_2v2_Th_1;
        case RESERVED_FD5:
            return G_regs->reg_Vpp_Fd_Control_5.reg.Reserved_Fd5;
        case GB_FD_QUIT_2V2_TH_2:
            return G_regs->reg_Vpp_Fd_Control_6.reg.Gb_Fd_Quit_2v2_Th_2;
        case GB_FD_QUIT_2V2_TH_3:
            return G_regs->reg_Vpp_Fd_Control_6.reg.Gb_Fd_Quit_2v2_Th_3;
        case GB_FD_QUIT_2V2_TH_4:
            return G_regs->reg_Vpp_Fd_Control_6.reg.Gb_Fd_Quit_2v2_Th_4;
        case GB_FD_QUIT_2V2_TH_5:
            return G_regs->reg_Vpp_Fd_Control_6.reg.Gb_Fd_Quit_2v2_Th_5;
        case GB_FD_2V2_MD_TH_3:
            return G_regs->reg_Vpp_Fd_Control_6.reg.Gb_Fd_2v2_Md_Th_3;
        case GB_FD_2V2_MD_TH_5:
            return G_regs->reg_Vpp_Fd_Control_6.reg.Gb_Fd_2v2_Md_Th_5;
        case GB_FD_2V2_MD_TH_1:
            return G_regs->reg_Vpp_Fd_Control_7.reg.Gb_Fd_2v2_Md_Th_1;
        case GB_FD_2V2_MD_TH_2:
            return G_regs->reg_Vpp_Fd_Control_7.reg.Gb_Fd_2v2_Md_Th_2;
        case GB_FD_2V2_MD_TH_4:
            return G_regs->reg_Vpp_Fd_Control_7.reg.Gb_Fd_2v2_Md_Th_4;
        case GB_FD_SC_QUIT_2V2_TH:
            return G_regs->reg_Vpp_Fd_Control_7.reg.Gb_Fd_Sc_Quit_2v2_Th;
        case GB_DI_CG_OFF:
            return G_regs->reg_Cg_Ctrl.reg.Gb_Di_Cg_Off;
        case GB_SCL_CG_OFF:
            return G_regs->reg_Cg_Ctrl.reg.Gb_Scl_Cg_Off;
        case GB_SCL_BP_CG_OFF:
            return G_regs->reg_Cg_Ctrl.reg.Gb_Scl_Bp_Cg_Off;
        case GB_CSCB_CG_OFF:
            return G_regs->reg_Cg_Ctrl.reg.Gb_Cscb_Cg_Off;
        case GB_CSCF_CG_OFF:
            return G_regs->reg_Cg_Ctrl.reg.Gb_Cscf_Cg_Off;
        case GB_CG_BUSY_TH:
            return G_regs->reg_Cg_Ctrl.reg.Gb_Cg_Busy_Th;
        case GB_FIRE_AFTER_SAVE:
            return G_regs->reg_Cg_Ctrl.reg.Gb_Fire_After_Save;
        case GB_RESTORE_KEEP_ON:
            return G_regs->reg_Cg_Ctrl.reg.Gb_Restore_Keep_On;
        case GB_FOR_ECO2:
            return G_regs->reg_Cg_Ctrl.reg.Gb_For_Eco2;
        case GB_CPT_CG_OFF:
            return G_regs->reg_Cg_Ctrl.reg.Gb_Cpt_Cg_Off;
        case GB_SR_CG_OFF:
            return G_regs->reg_Cg_Ctrl.reg.Gb_Sr_Cg_Off;
        case GB_CM_CG_OFF:
            return G_regs->reg_Cg_Ctrl.reg.Gb_Cm_Cg_Off;
        case GB_CCM_CG_OFF:
            return G_regs->reg_Cg_Ctrl.reg.Gb_Ccm_Cg_Off;
        case GB_HDR_CG_OFF:
            return G_regs->reg_Cg_Ctrl.reg.Gb_Hdr_Cg_Off;
        case RESERVED_CG:
            return G_regs->reg_Cg_Ctrl.reg.Reserved_Cg;
        case GB_DI_EN:
            return G_regs->reg_From_Opcode.reg.Gb_Di_En;
        case GB_FD_EN:
            return G_regs->reg_From_Opcode.reg.Gb_Fd_En;
        case GB_MTD_EN:
            return G_regs->reg_From_Opcode.reg.Gb_Mtd_En;
        case GB_TWO_FRAME_OUT_EN:
            return G_regs->reg_From_Opcode.reg.Gb_Two_Frame_Out_En;
        case GB_DI_FORCE_LCID:
            return G_regs->reg_From_Opcode.reg.Gb_Di_Force_Lcid;
        case GB_SINGLE_SEL_OTH:
            return G_regs->reg_From_Opcode.reg.Gb_Single_Sel_Oth;
        case GB_TOP_FIELD_FIRST:
            return G_regs->reg_From_Opcode.reg.Gb_Top_Field_First;
        case GB_RESET_HQVPP:
            return G_regs->reg_From_Opcode.reg.Gb_Reset_Hqvpp;
        case GB_HQVPP_MODE:
            return G_regs->reg_From_Opcode.reg.Gb_Hqvpp_Mode;
        case GB_VPP_FD_INDEX:
            return G_regs->reg_From_Opcode.reg.Gb_Vpp_Fd_Index;
        case RESERVED_OPCODE:
            return G_regs->reg_From_Opcode.reg.Reserved_Opcode;
        case GB_CMLUT_EN:
            return G_regs->reg_Cm_0.reg.Gb_Cmlut_En;
        case GB_TINT_EN:
            return G_regs->reg_Cm_0.reg.Gb_Tint_En;
        case GB_TINT_HOFFSET:
            return G_regs->reg_Cm_0.reg.Gb_Tint_Hoffset;
        case RESERVED_CM0:
            return G_regs->reg_Cm_0.reg.Reserved_Cm0;
        case GB_CMLUT0:
            assert(index < 41);
            return G_regs->reg_Cm_1[index].reg.Gb_Cmlut0;
        case GB_CMLUT1:
            assert(index < 41);
            return G_regs->reg_Cm_1[index].reg.Gb_Cmlut1;
        case RESERVED_CM1:
            assert(index < 41);
            return G_regs->reg_Cm_1[index].reg.Reserved_Cm1;
        case GB_CCM_EN:
            return G_regs->reg_Ccm_0.reg.Gb_Ccm_En;
        case GB_METADATA_EN:
            return G_regs->reg_Ccm_0.reg.Gb_Metadata_En;
        case GB_DG_HCOE:
            return G_regs->reg_Ccm_0.reg.Gb_Dg_Hcoe;
        case RESERVED_CCM0:
            return G_regs->reg_Ccm_0.reg.Reserved_Ccm0;
        case GB_DG_LCOE:
            return G_regs->reg_Ccm_1.reg.Gb_Dg_Lcoe;
        case GB_YUV2RGB_EN:
            return G_regs->reg_Ccm_1.reg.Gb_Yuv2rgb_En;
        case GB_YUV2RGB_MODE:
            return G_regs->reg_Ccm_1.reg.Gb_Yuv2rgb_Mode;
        case GB_CCM_HDR_INV:
            return G_regs->reg_Ccm_1.reg.Gb_Ccm_Hdr_Inv;
        case RESERVED_CCM1:
            return G_regs->reg_Ccm_1.reg.Reserved_Ccm1;
        case GB_DG_TBL_0:
            assert(index < 16);
            return G_regs->reg_Ccm_3[index].reg.Gb_Dg_Tbl_0;
        case GB_DG_TBL_1:
            assert(index < 16);
            return G_regs->reg_Ccm_3[index].reg.Gb_Dg_Tbl_1;
        case GB_G_TBL_0:
            assert(index < 16);
            return G_regs->reg_Ccm_4[index].reg.Gb_G_Tbl_0;
        case GB_G_TBL_1:
            assert(index < 16);
            return G_regs->reg_Ccm_4[index].reg.Gb_G_Tbl_1;
        case GB_G_TBL_2:
            assert(index < 16);
            return G_regs->reg_Ccm_4[index].reg.Gb_G_Tbl_2;
        case RESERVED_CCM5:
            assert(index < 16);
            return G_regs->reg_Ccm_4[index].reg.Reserved_Ccm5;
        case GB_HLG_EN:
            return G_regs->reg_Ccm_5.reg.Gb_Hlg_En;
        case GB_HLG_ACOEF:
            return G_regs->reg_Ccm_5.reg.Gb_Hlg_Acoef;
        case GB_HLG_BCOEF:
            return G_regs->reg_Ccm_5.reg.Gb_Hlg_Bcoef;
        case RESERVED_CCM6:
            return G_regs->reg_Ccm_5.reg.Reserved_Ccm6;
        case GB_HLG_TBL_0:
            assert(index < 16);
            return G_regs->reg_Ccm_6[index].reg.Gb_Hlg_Tbl_0;
        case GB_HLG_TBL_1:
            assert(index < 16);
            return G_regs->reg_Ccm_6[index].reg.Gb_Hlg_Tbl_1;
        case GB_HLG_TBL_20:
            return G_regs->reg_Ccm_7.reg.Gb_Hlg_Tbl_20;
        case GB_HLG_TBL_21:
            return G_regs->reg_Ccm_7.reg.Gb_Hlg_Tbl_21;
        case GB_HLG_TBL_22:
            return G_regs->reg_Ccm_8.reg.Gb_Hlg_Tbl_22;
        case GB_HLG_INVTBL_22:
            return G_regs->reg_Ccm_8.reg.Gb_Hlg_Invtbl_22;
        case GB_1886_ACOEF:
            return G_regs->reg_Ccm_9.reg.Gb_1886_Acoef;
        case GB_1886_BCOEF:
            return G_regs->reg_Ccm_9.reg.Gb_1886_Bcoef;
        case GB_HLG_INVACOEF:
            return G_regs->reg_Ccm_A.reg.Gb_Hlg_Invacoef;
        case GB_HLG_INVBCOEF:
            return G_regs->reg_Ccm_A.reg.Gb_Hlg_Invbcoef;
        case GB_1886_EN:
            return G_regs->reg_Ccm_A.reg.Gb_1886_En;
        case RESERVED_CCM9:
            return G_regs->reg_Ccm_A.reg.Reserved_Ccm9;
        case GB_HLG_INVTBL_0:
            assert(index < 16);
            return G_regs->reg_Ccm_B[index].reg.Gb_Hlg_Invtbl_0;
        case GB_HLG_INVTBL_1:
            assert(index < 16);
            return G_regs->reg_Ccm_B[index].reg.Gb_Hlg_Invtbl_1;
        case GB_HLG_INVTBL_20:
            return G_regs->reg_Ccm_C.reg.Gb_Hlg_Invtbl_20;
        case GB_HLG_INVTBL_21:
            return G_regs->reg_Ccm_C.reg.Gb_Hlg_Invtbl_21;
        case GB_1886_INVACOEF:
            return G_regs->reg_Ccm_D.reg.Gb_1886_Invacoef;
        case GB_1886_INVBCOEF:
            return G_regs->reg_Ccm_D.reg.Gb_1886_Invbcoef;
        case GB_CONVERT_M_00:
            return G_regs->reg_Cc_0.reg.Gb_Convert_M_00;
        case GB_CONVERT_M_01:
            return G_regs->reg_Cc_0.reg.Gb_Convert_M_01;
        case RESERVED_CC0:
            return G_regs->reg_Cc_0.reg.Reserved_Cc0;
        case GB_CONVERT_M_10:
            return G_regs->reg_Cc_1.reg.Gb_Convert_M_10;
        case GB_CONVERT_M_11:
            return G_regs->reg_Cc_1.reg.Gb_Convert_M_11;
        case RESERVED_CC1:
            return G_regs->reg_Cc_1.reg.Reserved_Cc1;
        case GB_CONVERT_M_20:
            return G_regs->reg_Cc_2.reg.Gb_Convert_M_20;
        case GB_CONVERT_M_21:
            return G_regs->reg_Cc_2.reg.Gb_Convert_M_21;
        case RESERVED_CC2:
            return G_regs->reg_Cc_2.reg.Reserved_Cc2;
        case GB_CONVERT_M_02:
            return G_regs->reg_Cc_3.reg.Gb_Convert_M_02;
        case GB_CONVERT_M_12:
            return G_regs->reg_Cc_3.reg.Gb_Convert_M_12;
        case RESERVED_CC3:
            return G_regs->reg_Cc_3.reg.Reserved_Cc3;
        case GB_CONVERT_M_22:
            return G_regs->reg_Cc_4.reg.Gb_Convert_M_22;
        case RESERVED_CC4:
            return G_regs->reg_Cc_4.reg.Reserved_Cc4;
        case GB_HADJ_EN:
            return G_regs->reg_Hadj_0.reg.Gb_Hadj_En;
        case GB_HRANGE_GL:
            return G_regs->reg_Hadj_0.reg.Gb_Hrange_Gl;
        case GB_HRANGE_GH:
            return G_regs->reg_Hadj_0.reg.Gb_Hrange_Gh;
        case RESERVED_HADJ0:
            return G_regs->reg_Hadj_0.reg.Reserved_Hadj0;
        case GB_HRANGE_RL:
            return G_regs->reg_Hadj_1.reg.Gb_Hrange_Rl;
        case GB_HRANGE_RH:
            return G_regs->reg_Hadj_1.reg.Gb_Hrange_Rh;
        case RESERVED_HADJ1:
            return G_regs->reg_Hadj_1.reg.Reserved_Hadj1;
        case GB_G_RANGE:
            return G_regs->reg_Hadj_2.reg.Gb_G_Range;
        case GB_G_AXIS:
            return G_regs->reg_Hadj_2.reg.Gb_G_Axis;
        case GB_G_COEF:
            return G_regs->reg_Hadj_2.reg.Gb_G_Coef;
        case GB_R_RANGE:
            return G_regs->reg_Hadj_3.reg.Gb_R_Range;
        case GB_R_AXIS:
            return G_regs->reg_Hadj_3.reg.Gb_R_Axis;
        case GB_R_COEF:
            return G_regs->reg_Hadj_3.reg.Gb_R_Coef;
        case GB_SADJ_EN:
            return G_regs->reg_Sadj_0.reg.Gb_Sadj_En;
        case GB_SADJ_INC00:
            return G_regs->reg_Sadj_0.reg.Gb_Sadj_Inc00;
        case GB_SADJ_INC01:
            return G_regs->reg_Sadj_0.reg.Gb_Sadj_Inc01;
        case RESERVED_SADJ0:
            return G_regs->reg_Sadj_0.reg.Reserved_Sadj0;
        case GB_SADJ_INC02:
            return G_regs->reg_Sadj_1.reg.Gb_Sadj_Inc02;
        case GB_SADJ_INC03:
            return G_regs->reg_Sadj_1.reg.Gb_Sadj_Inc03;
        case RESERVED_SADJ1:
            return G_regs->reg_Sadj_1.reg.Reserved_Sadj1;
        case GB_SADJ_INC04:
            return G_regs->reg_Sadj_2.reg.Gb_Sadj_Inc04;
        case GB_SADJ_INC05:
            return G_regs->reg_Sadj_2.reg.Gb_Sadj_Inc05;
        case RESERVED_SADJ2:
            return G_regs->reg_Sadj_2.reg.Reserved_Sadj2;
        case GB_SADJ_INC06:
            return G_regs->reg_Sadj_3.reg.Gb_Sadj_Inc06;
        case GB_SADJ_INC07:
            return G_regs->reg_Sadj_3.reg.Gb_Sadj_Inc07;
        case GB_SADJ_INC14:
            value |= G_regs->reg_Sadj_3.reg.Gb_Sadj_Inc14<<0;
            value |= G_regs->reg_Sadj_4.reg.Gb_Sadj_Inc14<<5;
            return value;
        case RESERVED_SADJ3:
            return G_regs->reg_Sadj_3.reg.Reserved_Sadj3;
        case GB_SADJ_INC08:
            return G_regs->reg_Sadj_4.reg.Gb_Sadj_Inc08;
        case GB_SADJ_INC09:
            return G_regs->reg_Sadj_4.reg.Gb_Sadj_Inc09;
        case RESERVED_SADJ4:
            return G_regs->reg_Sadj_4.reg.Reserved_Sadj4;
        case GB_SADJ_INC0A:
            return G_regs->reg_Sadj_5.reg.Gb_Sadj_Inc0a;
        case GB_SADJ_INC0B:
            return G_regs->reg_Sadj_5.reg.Gb_Sadj_Inc0b;
        case GB_SADJ_INC13:
            value |= G_regs->reg_Sadj_5.reg.Gb_Sadj_Inc13<<0;
            value |= G_regs->reg_Sadj_6.reg.Gb_Sadj_Inc13<<5;
            return value;
        case RESERVED_SADJ5:
            return G_regs->reg_Sadj_5.reg.Reserved_Sadj5;
        case GB_SADJ_INC0C:
            return G_regs->reg_Sadj_6.reg.Gb_Sadj_Inc0c;
        case GB_SADJ_INC0D:
            return G_regs->reg_Sadj_6.reg.Gb_Sadj_Inc0d;
        case RESERVED_SADJ6:
            return G_regs->reg_Sadj_6.reg.Reserved_Sadj6;
        case GB_SADJ_INC0E:
            return G_regs->reg_Sadj_7.reg.Gb_Sadj_Inc0e;
        case GB_SADJ_INC0F:
            return G_regs->reg_Sadj_7.reg.Gb_Sadj_Inc0f;
        case GB_SADJ_INC12:
            value |= G_regs->reg_Sadj_7.reg.Gb_Sadj_Inc12<<0;
            value |= G_regs->reg_Sadj_8.reg.Gb_Sadj_Inc12<<5;
            return value;
        case RESERVED_SADJ7:
            return G_regs->reg_Sadj_7.reg.Reserved_Sadj7;
        case GB_SADJ_INC10:
            return G_regs->reg_Sadj_8.reg.Gb_Sadj_Inc10;
        case GB_SADJ_INC11:
            return G_regs->reg_Sadj_8.reg.Gb_Sadj_Inc11;
        case RESERVED_SADJ8:
            return G_regs->reg_Sadj_8.reg.Reserved_Sadj8;
        case GB_HDR_EN:
            return G_regs->reg_Hdr_0.reg.Gb_Hdr_En;
        case GB_HDR_CURVE_EN:
            return G_regs->reg_Hdr_0.reg.Gb_Hdr_Curve_En;
        case GB_HDR_CLIPLMT:
            return G_regs->reg_Hdr_0.reg.Gb_Hdr_Cliplmt;
        case GB_HDR_MAXFMEAN:
            return G_regs->reg_Hdr_0.reg.Gb_Hdr_Maxfmean;
        case GB_HDR_MAXMEAN:
            return G_regs->reg_Hdr_0.reg.Gb_Hdr_Maxmean;
        case GB_HDR_SC_EN:
            return G_regs->reg_Hdr_0.reg.Gb_Hdr_Sc_En;
        case GB_HDR_SMALLSIZE:
            return G_regs->reg_Hdr_0.reg.Gb_Hdr_Smallsize;
        case GB_HDR_MAXPEAKCNT:
            return G_regs->reg_Hdr_1.reg.Gb_Hdr_Maxpeakcnt;
        case GB_HDR_MINPEAKWG:
            return G_regs->reg_Hdr_1.reg.Gb_Hdr_Minpeakwg;
        case GB_HDR_MAXPEAKWG:
            return G_regs->reg_Hdr_1.reg.Gb_Hdr_Maxpeakwg;
        case GB_HDR_MANUALCLIPMODE:
            return G_regs->reg_Hdr_1.reg.Gb_Hdr_Manualclipmode;
        case GB_HDR_FRMWG:
            return G_regs->reg_Hdr_1.reg.Gb_Hdr_Frmwg;
        case GB_RESET_HDR:
            return G_regs->reg_Hdr_1.reg.Gb_Reset_Hdr;
        case RESERVED_HDR1:
            return G_regs->reg_Hdr_1.reg.Reserved_Hdr1;
        case GB_HDR_PIXCNTFORSTAT:
            return G_regs->reg_Hdr_2.reg.Gb_Hdr_Pixcntforstat;
        case GB_HDR_SHFT:
            return G_regs->reg_Hdr_2.reg.Gb_Hdr_Shft;
        case GB_HDR_MINSTRETCHRANGE:
            return G_regs->reg_Hdr_3.reg.Gb_Hdr_Minstretchrange;
        case RESERVED_HDR3:
            return G_regs->reg_Hdr_3.reg.Reserved_Hdr3;
        case GB_HDR_MINFILTSPD:
            return G_regs->reg_Hdr_3.reg.Gb_Hdr_Minfiltspd;
        case GB_HDR_BINMEDSDT:
            return G_regs->reg_Hdr_3.reg.Gb_Hdr_Binmedsdt;
        case GB_HDR_DELTAMEANTH:
            return G_regs->reg_Hdr_4.reg.Gb_Hdr_Deltameanth;
        case GB_HDR_BINMEDHIMAX:
            return G_regs->reg_Hdr_4.reg.Gb_Hdr_Binmedhimax;
        case GB_HDR_YLUTHIWGINIT:
            return G_regs->reg_Hdr_4.reg.Gb_Hdr_Yluthiwginit;
        case RESERVED_HDR4:
            return G_regs->reg_Hdr_4.reg.Reserved_Hdr4;
        case GB_HDR_MEANCHANGETH:
            return G_regs->reg_Hdr_5.reg.Gb_Hdr_Meanchangeth;
        case RESERVED_HDR5:
            return G_regs->reg_Hdr_5.reg.Reserved_Hdr5;
        case GB_HDR_PIXLOWRATIO:
            return G_regs->reg_Hdr_6.reg.Gb_Hdr_Pixlowratio;
        case RESERVED_HDR6:
            return G_regs->reg_Hdr_6.reg.Reserved_Hdr6;
        case GB_HDR_PIXHIRATIO:
            return G_regs->reg_Hdr_7.reg.Gb_Hdr_Pixhiratio;
        case RESERVED_HDR7:
            return G_regs->reg_Hdr_7.reg.Reserved_Hdr7;
        case GB_HDR_BLACKRATIO:
            return G_regs->reg_Hdr_8.reg.Gb_Hdr_Blackratio;
        case RESERVED_HDR8:
            return G_regs->reg_Hdr_8.reg.Reserved_Hdr8;
        case GB_HDR_NOISERATIO:
            return G_regs->reg_Hdr_9.reg.Gb_Hdr_Noiseratio;
        case RESERVED_HDR9:
            return G_regs->reg_Hdr_9.reg.Reserved_Hdr9;
        case GB_HDR_YLUTHI_00:
            return G_regs->reg_Hdr_Ylut_0.reg.Gb_Hdr_Yluthi_00;
        case GB_HDR_YLUTHI_01:
            return G_regs->reg_Hdr_Ylut_0.reg.Gb_Hdr_Yluthi_01;
        case GB_HDR_YLUTHI_02:
            return G_regs->reg_Hdr_Ylut_0.reg.Gb_Hdr_Yluthi_02;
        case RESERVED_HDR_YLUT0:
            return G_regs->reg_Hdr_Ylut_0.reg.Reserved_Hdr_Ylut0;
        case GB_HDR_YLUTHI_03:
            return G_regs->reg_Hdr_Ylut_1.reg.Gb_Hdr_Yluthi_03;
        case GB_HDR_YLUTHI_04:
            return G_regs->reg_Hdr_Ylut_1.reg.Gb_Hdr_Yluthi_04;
        case GB_HDR_YLUTHI_05:
            return G_regs->reg_Hdr_Ylut_1.reg.Gb_Hdr_Yluthi_05;
        case RESERVED_HDR_YLUT1:
            return G_regs->reg_Hdr_Ylut_1.reg.Reserved_Hdr_Ylut1;
        case GB_HDR_YLUTHI_06:
            return G_regs->reg_Hdr_Ylut_2.reg.Gb_Hdr_Yluthi_06;
        case GB_HDR_YLUTHI_07:
            return G_regs->reg_Hdr_Ylut_2.reg.Gb_Hdr_Yluthi_07;
        case GB_HDR_YLUTHI_08:
            return G_regs->reg_Hdr_Ylut_2.reg.Gb_Hdr_Yluthi_08;
        case RESERVED_HDR_YLUT2:
            return G_regs->reg_Hdr_Ylut_2.reg.Reserved_Hdr_Ylut2;
        case GB_HDR_YLUTHI_09:
            return G_regs->reg_Hdr_Ylut_3.reg.Gb_Hdr_Yluthi_09;
        case GB_HDR_YLUTHI_0A:
            return G_regs->reg_Hdr_Ylut_3.reg.Gb_Hdr_Yluthi_0a;
        case GB_HDR_YLUTHI_0B:
            return G_regs->reg_Hdr_Ylut_3.reg.Gb_Hdr_Yluthi_0b;
        case RESERVED_HDR_YLUT3:
            return G_regs->reg_Hdr_Ylut_3.reg.Reserved_Hdr_Ylut3;
        case GB_HDR_YLUTHI_0C:
            return G_regs->reg_Hdr_Ylut_4.reg.Gb_Hdr_Yluthi_0c;
        case GB_HDR_YLUTHI_0D:
            return G_regs->reg_Hdr_Ylut_4.reg.Gb_Hdr_Yluthi_0d;
        case GB_HDR_YLUTHI_0E:
            return G_regs->reg_Hdr_Ylut_4.reg.Gb_Hdr_Yluthi_0e;
        case RESERVED_HDR_YLUT4:
            return G_regs->reg_Hdr_Ylut_4.reg.Reserved_Hdr_Ylut4;
        case GB_HDR_YLUTHI_0F:
            return G_regs->reg_Hdr_Ylut_5.reg.Gb_Hdr_Yluthi_0f;
        case GB_HDR_YLUTHI_10:
            return G_regs->reg_Hdr_Ylut_5.reg.Gb_Hdr_Yluthi_10;
        case GB_HDR_YLUTHI_11:
            return G_regs->reg_Hdr_Ylut_5.reg.Gb_Hdr_Yluthi_11;
        case RESERVED_HDR_YLUT5:
            return G_regs->reg_Hdr_Ylut_5.reg.Reserved_Hdr_Ylut5;
        case GB_HDR_YLUTHI_12:
            return G_regs->reg_Hdr_Ylut_6.reg.Gb_Hdr_Yluthi_12;
        case GB_HDR_YLUTHI_13:
            return G_regs->reg_Hdr_Ylut_6.reg.Gb_Hdr_Yluthi_13;
        case GB_HDR_YLUTHI_14:
            return G_regs->reg_Hdr_Ylut_6.reg.Gb_Hdr_Yluthi_14;
        case RESERVED_HDR_YLUT6:
            return G_regs->reg_Hdr_Ylut_6.reg.Reserved_Hdr_Ylut6;
        case GB_HDR_YLUTHI_15:
            return G_regs->reg_Hdr_Ylut_7.reg.Gb_Hdr_Yluthi_15;
        case GB_HDR_YLUTHI_16:
            return G_regs->reg_Hdr_Ylut_7.reg.Gb_Hdr_Yluthi_16;
        case GB_HDR_YLUTHI_17:
            return G_regs->reg_Hdr_Ylut_7.reg.Gb_Hdr_Yluthi_17;
        case RESERVED_HDR_YLUT7:
            return G_regs->reg_Hdr_Ylut_7.reg.Reserved_Hdr_Ylut7;
        case GB_HDR_YLUTHI_18:
            return G_regs->reg_Hdr_Ylut_8.reg.Gb_Hdr_Yluthi_18;
        case GB_HDR_YLUTHI_19:
            return G_regs->reg_Hdr_Ylut_8.reg.Gb_Hdr_Yluthi_19;
        case GB_HDR_YLUTHI_1A:
            return G_regs->reg_Hdr_Ylut_8.reg.Gb_Hdr_Yluthi_1a;
        case RESERVED_HDR_YLUT8:
            return G_regs->reg_Hdr_Ylut_8.reg.Reserved_Hdr_Ylut8;
        case GB_HDR_YLUTHI_1B:
            return G_regs->reg_Hdr_Ylut_9.reg.Gb_Hdr_Yluthi_1b;
        case GB_HDR_YLUTHI_1C:
            return G_regs->reg_Hdr_Ylut_9.reg.Gb_Hdr_Yluthi_1c;
        case GB_HDR_YLUTHI_1D:
            return G_regs->reg_Hdr_Ylut_9.reg.Gb_Hdr_Yluthi_1d;
        case RESERVED_HDR_YLUT9:
            return G_regs->reg_Hdr_Ylut_9.reg.Reserved_Hdr_Ylut9;
        case GB_HDR_YLUTHI_1E:
            return G_regs->reg_Hdr_Ylut_A.reg.Gb_Hdr_Yluthi_1e;
        case GB_HDR_YLUTHI_1F:
            return G_regs->reg_Hdr_Ylut_A.reg.Gb_Hdr_Yluthi_1f;
        case RESERVED_HDR_YLUTA:
            return G_regs->reg_Hdr_Ylut_A.reg.Reserved_Hdr_Yluta;
        case GB_HDR_YLUTLOW_00:
            return G_regs->reg_Hdr_Ylut_10.reg.Gb_Hdr_Ylutlow_00;
        case GB_HDR_YLUTLOW_01:
            return G_regs->reg_Hdr_Ylut_10.reg.Gb_Hdr_Ylutlow_01;
        case GB_HDR_YLUTLOW_02:
            return G_regs->reg_Hdr_Ylut_10.reg.Gb_Hdr_Ylutlow_02;
        case RESERVED_HDR_YLUT10:
            return G_regs->reg_Hdr_Ylut_10.reg.Reserved_Hdr_Ylut10;
        case GB_HDR_YLUTLOW_03:
            return G_regs->reg_Hdr_Ylut_11.reg.Gb_Hdr_Ylutlow_03;
        case GB_HDR_YLUTLOW_04:
            return G_regs->reg_Hdr_Ylut_11.reg.Gb_Hdr_Ylutlow_04;
        case GB_HDR_YLUTLOW_05:
            return G_regs->reg_Hdr_Ylut_11.reg.Gb_Hdr_Ylutlow_05;
        case RESERVED_HDR_YLUT11:
            return G_regs->reg_Hdr_Ylut_11.reg.Reserved_Hdr_Ylut11;
        case GB_HDR_YLUTLOW_06:
            return G_regs->reg_Hdr_Ylut_12.reg.Gb_Hdr_Ylutlow_06;
        case GB_HDR_YLUTLOW_07:
            return G_regs->reg_Hdr_Ylut_12.reg.Gb_Hdr_Ylutlow_07;
        case GB_HDR_YLUTLOW_08:
            return G_regs->reg_Hdr_Ylut_12.reg.Gb_Hdr_Ylutlow_08;
        case RESERVED_HDR_YLUT12:
            return G_regs->reg_Hdr_Ylut_12.reg.Reserved_Hdr_Ylut12;
        case GB_HDR_YLUTLOW_09:
            return G_regs->reg_Hdr_Ylut_13.reg.Gb_Hdr_Ylutlow_09;
        case GB_HDR_YLUTLOW_0A:
            return G_regs->reg_Hdr_Ylut_13.reg.Gb_Hdr_Ylutlow_0a;
        case GB_HDR_YLUTLOW_0B:
            return G_regs->reg_Hdr_Ylut_13.reg.Gb_Hdr_Ylutlow_0b;
        case RESERVED_HDR_YLUT13:
            return G_regs->reg_Hdr_Ylut_13.reg.Reserved_Hdr_Ylut13;
        case GB_HDR_YLUTLOW_0C:
            return G_regs->reg_Hdr_Ylut_14.reg.Gb_Hdr_Ylutlow_0c;
        case GB_HDR_YLUTLOW_0D:
            return G_regs->reg_Hdr_Ylut_14.reg.Gb_Hdr_Ylutlow_0d;
        case GB_HDR_YLUTLOW_0E:
            return G_regs->reg_Hdr_Ylut_14.reg.Gb_Hdr_Ylutlow_0e;
        case RESERVED_HDR_YLUT14:
            return G_regs->reg_Hdr_Ylut_14.reg.Reserved_Hdr_Ylut14;
        case GB_HDR_YLUTLOW_0F:
            return G_regs->reg_Hdr_Ylut_15.reg.Gb_Hdr_Ylutlow_0f;
        case GB_HDR_YLUTLOW_10:
            return G_regs->reg_Hdr_Ylut_15.reg.Gb_Hdr_Ylutlow_10;
        case GB_HDR_YLUTLOW_11:
            return G_regs->reg_Hdr_Ylut_15.reg.Gb_Hdr_Ylutlow_11;
        case RESERVED_HDR_YLUT15:
            return G_regs->reg_Hdr_Ylut_15.reg.Reserved_Hdr_Ylut15;
        case GB_HDR_YLUTLOW_12:
            return G_regs->reg_Hdr_Ylut_16.reg.Gb_Hdr_Ylutlow_12;
        case GB_HDR_YLUTLOW_13:
            return G_regs->reg_Hdr_Ylut_16.reg.Gb_Hdr_Ylutlow_13;
        case GB_HDR_YLUTLOW_14:
            return G_regs->reg_Hdr_Ylut_16.reg.Gb_Hdr_Ylutlow_14;
        case RESERVED_HDR_YLUT16:
            return G_regs->reg_Hdr_Ylut_16.reg.Reserved_Hdr_Ylut16;
        case GB_HDR_YLUTLOW_15:
            return G_regs->reg_Hdr_Ylut_17.reg.Gb_Hdr_Ylutlow_15;
        case GB_HDR_YLUTLOW_16:
            return G_regs->reg_Hdr_Ylut_17.reg.Gb_Hdr_Ylutlow_16;
        case GB_HDR_YLUTLOW_17:
            return G_regs->reg_Hdr_Ylut_17.reg.Gb_Hdr_Ylutlow_17;
        case RESERVED_HDR_YLUT17:
            return G_regs->reg_Hdr_Ylut_17.reg.Reserved_Hdr_Ylut17;
        case GB_HDR_YLUTLOW_18:
            return G_regs->reg_Hdr_Ylut_18.reg.Gb_Hdr_Ylutlow_18;
        case GB_HDR_YLUTLOW_19:
            return G_regs->reg_Hdr_Ylut_18.reg.Gb_Hdr_Ylutlow_19;
        case GB_HDR_YLUTLOW_1A:
            return G_regs->reg_Hdr_Ylut_18.reg.Gb_Hdr_Ylutlow_1a;
        case RESERVED_HDR_YLUT18:
            return G_regs->reg_Hdr_Ylut_18.reg.Reserved_Hdr_Ylut18;
        case GB_HDR_YLUTLOW_1B:
            return G_regs->reg_Hdr_Ylut_19.reg.Gb_Hdr_Ylutlow_1b;
        case GB_HDR_YLUTLOW_1C:
            return G_regs->reg_Hdr_Ylut_19.reg.Gb_Hdr_Ylutlow_1c;
        case GB_HDR_YLUTLOW_1D:
            return G_regs->reg_Hdr_Ylut_19.reg.Gb_Hdr_Ylutlow_1d;
        case RESERVED_HDR_YLUT19:
            return G_regs->reg_Hdr_Ylut_19.reg.Reserved_Hdr_Ylut19;
        case GB_HDR_YLUTLOW_1E:
            return G_regs->reg_Hdr_Ylut_1a.reg.Gb_Hdr_Ylutlow_1e;
        case GB_HDR_YLUTLOW_1F:
            return G_regs->reg_Hdr_Ylut_1a.reg.Gb_Hdr_Ylutlow_1f;
        case RESERVED_HDR_YLUT1A:
            return G_regs->reg_Hdr_Ylut_1a.reg.Reserved_Hdr_Ylut1a;
    }
    printf("Error! There is no register named as %d, please check vector setting!\n", reg);
    assert(0);
    return 0;
}


static inline int Vpp_Global_get_reg_index(char * name)
{
    if(!strcmp("gb_src_sf_base", name))
        return GB_SRC_SF_BASE;
    if(!strcmp("gb_oth_sf_base", name))
        return GB_OTH_SF_BASE;
    if(!strcmp("gb_ref_sf_base", name))
        return GB_REF_SF_BASE;
    if(!strcmp("gb_src_range_map_y_en", name))
        return GB_SRC_RANGE_MAP_Y_EN;
    if(!strcmp("gb_src_range_map_y", name))
        return GB_SRC_RANGE_MAP_Y;
    if(!strcmp("gb_src_range_map_uv_en", name))
        return GB_SRC_RANGE_MAP_UV_EN;
    if(!strcmp("gb_src_range_map_uv", name))
        return GB_SRC_RANGE_MAP_UV;
    if(!strcmp("gb_oth_range_map_y_en", name))
        return GB_OTH_RANGE_MAP_Y_EN;
    if(!strcmp("gb_oth_range_map_y", name))
        return GB_OTH_RANGE_MAP_Y;
    if(!strcmp("gb_oth_range_map_uv_en", name))
        return GB_OTH_RANGE_MAP_UV_EN;
    if(!strcmp("gb_oth_range_map_uv", name))
        return GB_OTH_RANGE_MAP_UV;
    if(!strcmp("gb_ref_range_map_y_en", name))
        return GB_REF_RANGE_MAP_Y_EN;
    if(!strcmp("gb_ref_range_map_y", name))
        return GB_REF_RANGE_MAP_Y;
    if(!strcmp("gb_ref_range_map_uv_en", name))
        return GB_REF_RANGE_MAP_UV_EN;
    if(!strcmp("gb_ref_range_map_uv", name))
        return GB_REF_RANGE_MAP_UV;
    if(!strcmp("reserved_bm", name))
        return RESERVED_BM;
    if(!strcmp("gb_dst_sf_base", name))
        return GB_DST_SF_BASE;
    if(!strcmp("gb_scl_dst_sf_base", name))
        return GB_SCL_DST_SF_BASE;
    if(!strcmp("gb_scl_dst_clipx", name))
        return GB_SCL_DST_CLIPX;
    if(!strcmp("gb_scl_dst_clipy", name))
        return GB_SCL_DST_CLIPY;
    if(!strcmp("reserved_mob", name))
        return RESERVED_MOB;
    if(!strcmp("gb_scl_dst_sf_height", name))
        return GB_SCL_DST_SF_HEIGHT;
    if(!strcmp("reserved_sdb2", name))
        return RESERVED_SDB2;
    if(!strcmp("gb_src_sf_bot_base", name))
        return GB_SRC_SF_BOT_BASE;
    if(!strcmp("gb_oth_sf_bot_base", name))
        return GB_OTH_SF_BOT_BASE;
    if(!strcmp("gb_ref_sf_bot_base", name))
        return GB_REF_SF_BOT_BASE;
    if(!strcmp("gb_hdr_int_base", name))
        return GB_HDR_INT_BASE;
    if(!strcmp("gb_ysum_mem_base", name))
        return GB_YSUM_MEM_BASE;
    if(!strcmp("gb_hs_error", name))
        return GB_HS_ERROR;
    if(!strcmp("gb_vs_error", name))
        return GB_VS_ERROR;
    if(!strcmp("gb_src_width", name))
        return GB_SRC_WIDTH;
    if(!strcmp("gb_seq_id_thrd", name))
        return GB_SEQ_ID_THRD;
    if(!strcmp("reserved_src_sf_0", name))
        return RESERVED_SRC_SF_0;
    if(!strcmp("reserved_src_sf_1", name))
        return RESERVED_SRC_SF_1;
    if(!strcmp("gb_src_fmt", name))
        return GB_SRC_FMT;
    if(!strcmp("gb_src_tile", name))
        return GB_SRC_TILE;
    if(!strcmp("gb_src_height", name))
        return GB_SRC_HEIGHT;
    if(!strcmp("gb_src_pitch", name))
        return GB_SRC_PITCH;
    if(!strcmp("gb_scl_dst_fmt", name))
        return GB_SCL_DST_FMT;
    if(!strcmp("gb_scl_dst_height", name))
        return GB_SCL_DST_HEIGHT;
    if(!strcmp("gb_scl_dst_pitch", name))
        return GB_SCL_DST_PITCH;
    if(!strcmp("gb_scl_dst_compress_en", name))
        return GB_SCL_DST_COMPRESS_EN;
    if(!strcmp("reserved_scl_dst_sf_1", name))
        return RESERVED_SCL_DST_SF_1;
    if(!strcmp("gb_scl_dst_width", name))
        return GB_SCL_DST_WIDTH;
    if(!strcmp("reserve_dst2", name))
        return RESERVE_DST2;
    if(!strcmp("reserved_rs", name))
        return RESERVED_RS;
    if(!strcmp("gb_vpp_dnref_sf_pitch", name))
        return GB_VPP_DNREF_SF_PITCH;
    if(!strcmp("gb_vpp_dnref_sf_height", name))
        return GB_VPP_DNREF_SF_HEIGHT;
    if(!strcmp("gb_blt_mode", name))
        return GB_BLT_MODE;
    if(!strcmp("gb_scl_mode", name))
        return GB_SCL_MODE;
    if(!strcmp("gb_en_hscl", name))
        return GB_EN_HSCL;
    if(!strcmp("gb_en_vscl", name))
        return GB_EN_VSCL;
    if(!strcmp("reserved_gb_first_slice", name))
        return RESERVED_GB_FIRST_SLICE;
    if(!strcmp("gb_se_vpp", name))
        return GB_SE_VPP;
    if(!strcmp("gb_rotation_mode", name))
        return GB_ROTATION_MODE;
    if(!strcmp("gb_src_compress", name))
        return GB_SRC_COMPRESS;
    if(!strcmp("gb_for_eco", name))
        return GB_FOR_ECO;
    if(!strcmp("gb_x_upsample_mode", name))
        return GB_X_UPSAMPLE_MODE;
    if(!strcmp("gb_ds_24_x", name))
        return GB_DS_24_X;
    if(!strcmp("gb_ds_24_y", name))
        return GB_DS_24_Y;
    if(!strcmp("gb_blt_simple", name))
        return GB_BLT_SIMPLE;
    if(!strcmp("gb_en_4k_mem_swzl", name))
        return GB_EN_4K_MEM_SWZL;
    if(!strcmp("gb_ds4_lsb2force", name))
        return GB_DS4_LSB2FORCE;
    if(!strcmp("gb_fixcolor_mode", name))
        return GB_FIXCOLOR_MODE;
    if(!strcmp("gb_y_upsample_mode", name))
        return GB_Y_UPSAMPLE_MODE;
    if(!strcmp("reserved_ms", name))
        return RESERVED_MS;
    if(!strcmp("reserved_gb_scl_left_edge_width", name))
        return RESERVED_GB_SCL_LEFT_EDGE_WIDTH;
    if(!strcmp("reserved_gb_scl_right_edge_width", name))
        return RESERVED_GB_SCL_RIGHT_EDGE_WIDTH;
    if(!strcmp("gb_ldrd_compress", name))
        return GB_LDRD_COMPRESS;
    if(!strcmp("gb_seq_id_peaklevel", name))
        return GB_SEQ_ID_PEAKLEVEL;
    if(!strcmp("reserved_dms", name))
        return RESERVED_DMS;
    if(!strcmp("gb_slice_width", name))
        return GB_SLICE_WIDTH;
    if(!strcmp("gb_slice_pixel_num", name))
        return GB_SLICE_PIXEL_NUM;
    if(!strcmp("gb_slice_width_1st", name))
        return GB_SLICE_WIDTH_1ST;
    if(!strcmp("gb_slice_pixel_num_1st", name))
        return GB_SLICE_PIXEL_NUM_1ST;
    if(!strcmp("gb_signat_sel_io", name))
        return GB_SIGNAT_SEL_IO;
    if(!strcmp("gb_signat_sel_oo", name))
        return GB_SIGNAT_SEL_OO;
    if(!strcmp("gb_sig_enaddr_io", name))
        return GB_SIG_ENADDR_IO;
    if(!strcmp("reserved_mp1", name))
        return RESERVED_MP1;
    if(!strcmp("gb_cl_left", name))
        return GB_CL_LEFT;
    if(!strcmp("gb_cl_right", name))
        return GB_CL_RIGHT;
    if(!strcmp("reserved17", name))
        return RESERVED17;
    if(!strcmp("gb_cl_top", name))
        return GB_CL_TOP;
    if(!strcmp("gb_cl_bottom", name))
        return GB_CL_BOTTOM;
    if(!strcmp("reserved16", name))
        return RESERVED16;
    if(!strcmp("gb_vppcntout", name))
        return GB_VPPCNTOUT;
    if(!strcmp("gb_vpparbmode", name))
        return GB_VPPARBMODE;
    if(!strcmp("gb_lb_scl_yuv_offset", name))
        return GB_LB_SCL_YUV_OFFSET;
    if(!strcmp("gb_lb_scl_a_offset", name))
        return GB_LB_SCL_A_OFFSET;
    if(!strcmp("reserved_ls0", name))
        return RESERVED_LS0;
    if(!strcmp("gb_lb_scl_yuv_pitch", name))
        return GB_LB_SCL_YUV_PITCH;
    if(!strcmp("gb_lb_scl_a_pitch", name))
        return GB_LB_SCL_A_PITCH;
    if(!strcmp("gb_lb_nm_oper_offset", name))
        return GB_LB_NM_OPER_OFFSET;
    if(!strcmp("reserved_ls01", name))
        return RESERVED_LS01;
    if(!strcmp("gb_lb_sr_y_offset", name))
        return GB_LB_SR_Y_OFFSET;
    if(!strcmp("gb_lb_sr_uv_offset", name))
        return GB_LB_SR_UV_OFFSET;
    if(!strcmp("gb_lb_sr_y_pitch", name))
        return GB_LB_SR_Y_PITCH;
    if(!strcmp("gb_lb_sr_a_offset", name))
        return GB_LB_SR_A_OFFSET;
    if(!strcmp("gb_lb_sr_uv_pitch", name))
        return GB_LB_SR_UV_PITCH;
    if(!strcmp("gb_lb_sr_a_pitch", name))
        return GB_LB_SR_A_PITCH;
    if(!strcmp("reserved_ls03", name))
        return RESERVED_LS03;
    if(!strcmp("gb_lbadiw", name))
        return GB_LBADIW;
    if(!strcmp("gb_lbasclyuvw", name))
        return GB_LBASCLYUVW;
    if(!strcmp("gb_lbasclaw", name))
        return GB_LBASCLAW;
    if(!strcmp("gb_lbasruvw", name))
        return GB_LBASRUVW;
    if(!strcmp("gb_lba_bank_mode", name))
        return GB_LBA_BANK_MODE;
    if(!strcmp("reserved_ls1", name))
        return RESERVED_LS1;
    if(!strcmp("gb_lbasryw", name))
        return GB_LBASRYW;
    if(!strcmp("gb_lbasraw", name))
        return GB_LBASRAW;
    if(!strcmp("reserved_ls2", name))
        return RESERVED_LS2;
    if(!strcmp("gb_lbadir", name))
        return GB_LBADIR;
    if(!strcmp("gb_lbadirefr", name))
        return GB_LBADIREFR;
    if(!strcmp("gb_lbascrr", name))
        return GB_LBASCRR;
    if(!strcmp("reserved_ls3", name))
        return RESERVED_LS3;
    if(!strcmp("gb_lbasclyuvr", name))
        return GB_LBASCLYUVR;
    if(!strcmp("gb_lbasclar", name))
        return GB_LBASCLAR;
    if(!strcmp("gb_lbasruvr", name))
        return GB_LBASRUVR;
    if(!strcmp("reserved_ls4", name))
        return RESERVED_LS4;
    if(!strcmp("gb_lbasryr", name))
        return GB_LBASRYR;
    if(!strcmp("gb_lbasrar", name))
        return GB_LBASRAR;
    if(!strcmp("reserved_ls5", name))
        return RESERVED_LS5;
    if(!strcmp("gb_mob_fixalpha", name))
        return GB_MOB_FIXALPHA;
    if(!strcmp("gb_redundant_edge_width", name))
        return GB_REDUNDANT_EDGE_WIDTH;
    if(!strcmp("gb_mob_range_min", name))
        return GB_MOB_RANGE_MIN;
    if(!strcmp("gb_mob_range_maxry", name))
        return GB_MOB_RANGE_MAXRY;
    if(!strcmp("gb_mob_range_maxgbuv", name))
        return GB_MOB_RANGE_MAXGBUV;
    if(!strcmp("gb_src_lossy_com_en", name))
        return GB_SRC_LOSSY_COM_EN;
    if(!strcmp("gb_dst_lossy_com_en", name))
        return GB_DST_LOSSY_COM_EN;
    if(!strcmp("gb_mob_blending_en", name))
        return GB_MOB_BLENDING_EN;
    if(!strcmp("gb_mob_blending_diu_mode", name))
        return GB_MOB_BLENDING_DIU_MODE;
    if(!strcmp("gb_mob_blending_stream_mode", name))
        return GB_MOB_BLENDING_STREAM_MODE;
    if(!strcmp("reserved_ir", name))
        return RESERVED_IR;
    if(!strcmp("gb_hslc_y_a0", name))
        return GB_HSLC_Y_A0;
    if(!strcmp("gb_hslc_y_b0", name))
        return GB_HSLC_Y_B0;
    if(!strcmp("reserved_hslc0", name))
        return RESERVED_HSLC0;
    if(!strcmp("gb_hslc_uv_a0", name))
        return GB_HSLC_UV_A0;
    if(!strcmp("gb_hslc_uv_a1", name))
        return GB_HSLC_UV_A1;
    if(!strcmp("reserved_hslc1", name))
        return RESERVED_HSLC1;
    if(!strcmp("gb_hslc_u_b0", name))
        return GB_HSLC_U_B0;
    if(!strcmp("gb_hslc_v_b0", name))
        return GB_HSLC_V_B0;
    if(!strcmp("reserved_hslc2", name))
        return RESERVED_HSLC2;
    if(!strcmp("gb_sd_width_ratio", name))
        return GB_SD_WIDTH_RATIO;
    if(!strcmp("gb_sd_height_ratio", name))
        return GB_SD_HEIGHT_RATIO;
    if(!strcmp("gb_ds_width_ratio", name))
        return GB_DS_WIDTH_RATIO;
    if(!strcmp("gb_ds_height_ratio", name))
        return GB_DS_HEIGHT_RATIO;
    if(!strcmp("reserved_ds", name))
        return RESERVED_DS;
    if(!strcmp("gb_src_en_alpha", name))
        return GB_SRC_EN_ALPHA;
    if(!strcmp("gb_scl_bc_delta", name))
        return GB_SCL_BC_DELTA;
    if(!strcmp("gb_en_scl_maxmin", name))
        return GB_EN_SCL_MAXMIN;
    if(!strcmp("gb_en_sr", name))
        return GB_EN_SR;
    if(!strcmp("gb_sr_ctdiffth", name))
        return GB_SR_CTDIFFTH;
    if(!strcmp("gb_en_pixeltype", name))
        return GB_EN_PIXELTYPE;
    if(!strcmp("gb_sr_blendstrength", name))
        return GB_SR_BLENDSTRENGTH;
    if(!strcmp("gb_sr_enctblend", name))
        return GB_SR_ENCTBLEND;
    if(!strcmp("gb_sr_blendweight", name))
        return GB_SR_BLENDWEIGHT;
    if(!strcmp("gb_sr_coefweight", name))
        return GB_SR_COEFWEIGHT;
    if(!strcmp("gb_sr_coefweightother", name))
        return GB_SR_COEFWEIGHTOTHER;
    if(!strcmp("reserved_scl0", name))
        return RESERVED_SCL0;
    if(!strcmp("gb_cscf_m_00", name))
        return GB_CSCF_M_00;
    if(!strcmp("gb_cscf_m_01", name))
        return GB_CSCF_M_01;
    if(!strcmp("gb_cscf_en", name))
        return GB_CSCF_EN;
    if(!strcmp("gb_cscf_round", name))
        return GB_CSCF_ROUND;
    if(!strcmp("reserved_cscf_00_01", name))
        return RESERVED_CSCF_00_01;
    if(!strcmp("gb_cscf_m_02", name))
        return GB_CSCF_M_02;
    if(!strcmp("gb_cscf_m_10", name))
        return GB_CSCF_M_10;
    if(!strcmp("reserved_cscf_02_10", name))
        return RESERVED_CSCF_02_10;
    if(!strcmp("gb_cscf_m_11", name))
        return GB_CSCF_M_11;
    if(!strcmp("gb_cscf_m_12", name))
        return GB_CSCF_M_12;
    if(!strcmp("reserved_cscf_11_12", name))
        return RESERVED_CSCF_11_12;
    if(!strcmp("gb_cscf_m_20", name))
        return GB_CSCF_M_20;
    if(!strcmp("gb_cscf_m_21", name))
        return GB_CSCF_M_21;
    if(!strcmp("reserved_cscf_20_21", name))
        return RESERVED_CSCF_20_21;
    if(!strcmp("gb_cscf_m_22", name))
        return GB_CSCF_M_22;
    if(!strcmp("reserved_cscf_22", name))
        return RESERVED_CSCF_22;
    if(!strcmp("gb_cscf_off_00", name))
        return GB_CSCF_OFF_00;
    if(!strcmp("reserved_cscf_off_00", name))
        return RESERVED_CSCF_OFF_00;
    if(!strcmp("gb_cscf_off_01", name))
        return GB_CSCF_OFF_01;
    if(!strcmp("reserved_cscf_off_01", name))
        return RESERVED_CSCF_OFF_01;
    if(!strcmp("gb_cscf_off_02", name))
        return GB_CSCF_OFF_02;
    if(!strcmp("reserved_cscf_off_02", name))
        return RESERVED_CSCF_OFF_02;
    if(!strcmp("gb_cscb_m_00", name))
        return GB_CSCB_M_00;
    if(!strcmp("gb_cscb_m_01", name))
        return GB_CSCB_M_01;
    if(!strcmp("gb_cscb_en", name))
        return GB_CSCB_EN;
    if(!strcmp("gb_cscb_round", name))
        return GB_CSCB_ROUND;
    if(!strcmp("reserved_cscb_00_01", name))
        return RESERVED_CSCB_00_01;
    if(!strcmp("gb_cscb_m_02", name))
        return GB_CSCB_M_02;
    if(!strcmp("gb_cscb_m_10", name))
        return GB_CSCB_M_10;
    if(!strcmp("reserved_cscb_02_10", name))
        return RESERVED_CSCB_02_10;
    if(!strcmp("gb_cscb_m_11", name))
        return GB_CSCB_M_11;
    if(!strcmp("gb_cscb_m_12", name))
        return GB_CSCB_M_12;
    if(!strcmp("reserved_cscb_11_12", name))
        return RESERVED_CSCB_11_12;
    if(!strcmp("gb_cscb_m_20", name))
        return GB_CSCB_M_20;
    if(!strcmp("gb_cscb_m_21", name))
        return GB_CSCB_M_21;
    if(!strcmp("reserved_cscb_20_21", name))
        return RESERVED_CSCB_20_21;
    if(!strcmp("gb_cscb_m_22", name))
        return GB_CSCB_M_22;
    if(!strcmp("reserved_cscb_22", name))
        return RESERVED_CSCB_22;
    if(!strcmp("gb_cscb_off_00", name))
        return GB_CSCB_OFF_00;
    if(!strcmp("reserved_cscb_off_00", name))
        return RESERVED_CSCB_OFF_00;
    if(!strcmp("gb_cscb_off_01", name))
        return GB_CSCB_OFF_01;
    if(!strcmp("reserved_cscb_off_01", name))
        return RESERVED_CSCB_OFF_01;
    if(!strcmp("gb_cscb_off_02", name))
        return GB_CSCB_OFF_02;
    if(!strcmp("reserved_cscb_off_02", name))
        return RESERVED_CSCB_OFF_02;
    if(!strcmp("gb_fixcolor_value", name))
        return GB_FIXCOLOR_VALUE;
    if(!strcmp("gb_lb_cpt_offset", name))
        return GB_LB_CPT_OFFSET;
    if(!strcmp("gb_lb_cpt_size", name))
        return GB_LB_CPT_SIZE;
    if(!strcmp("gb_lb_mob_l2t_pitch", name))
        return GB_LB_MOB_L2T_PITCH;
    if(!strcmp("gb_dst_end", name))
        return GB_DST_END;
    if(!strcmp("gb_dst_tile", name))
        return GB_DST_TILE;
    if(!strcmp("gb_lb_mob_l2t_offset", name))
        return GB_LB_MOB_L2T_OFFSET;
    if(!strcmp("gb_mob_dst_alphamode", name))
        return GB_MOB_DST_ALPHAMODE;
    if(!strcmp("gb_mob_blending_mode", name))
        return GB_MOB_BLENDING_MODE;
    if(!strcmp("gb_mob_blending_method", name))
        return GB_MOB_BLENDING_METHOD;
    if(!strcmp("gb_mob_blending_rounding", name))
        return GB_MOB_BLENDING_ROUNDING;
    if(!strcmp("gb_cpt_dnsample_mode", name))
        return GB_CPT_DNSAMPLE_MODE;
    if(!strcmp("gb_mob_bldalpha", name))
        return GB_MOB_BLDALPHA;
    if(!strcmp("gb_cpt_en", name))
        return GB_CPT_EN;
    if(!strcmp("gb_dst_sf_width", name))
        return GB_DST_SF_WIDTH;
    if(!strcmp("reserved_cpt2", name))
        return RESERVED_CPT2;
    if(!strcmp("gb_mifarb_reg", name))
        return GB_MIFARB_REG;
    if(!strcmp("gb_mifarb_ydiff", name))
        return GB_MIFARB_YDIFF;
    if(!strcmp("gb_mifarb_ld", name))
        return GB_MIFARB_LD;
    if(!strcmp("reserved_ma0", name))
        return RESERVED_MA0;
    if(!strcmp("gb_mifarb_cptr", name))
        return GB_MIFARB_CPTR;
    if(!strcmp("gb_mifarb_mtdr", name))
        return GB_MIFARB_MTDR;
    if(!strcmp("gb_mifarb_mob", name))
        return GB_MIFARB_MOB;
    if(!strcmp("gb_mifarb_passive", name))
        return GB_MIFARB_PASSIVE;
    if(!strcmp("reserved_ma1", name))
        return RESERVED_MA1;
    if(!strcmp("gb_mifarb_mtdw", name))
        return GB_MIFARB_MTDW;
    if(!strcmp("gb_mifarb_hdrr", name))
        return GB_MIFARB_HDRR;
    if(!strcmp("gb_mifarb_hdrw", name))
        return GB_MIFARB_HDRW;
    if(!strcmp("reserved_ma2", name))
        return RESERVED_MA2;
    if(!strcmp("gb_src_bl_slot_idx", name))
        return GB_SRC_BL_SLOT_IDX;
    if(!strcmp("gb_oth_bl_slot_idx", name))
        return GB_OTH_BL_SLOT_IDX;
    if(!strcmp("gb_ref_bl_slot_idx", name))
        return GB_REF_BL_SLOT_IDX;
    if(!strcmp("gb_dst_bl_slot_idx", name))
        return GB_DST_BL_SLOT_IDX;
    if(!strcmp("gb_scl_dst_bl_slot_idx", name))
        return GB_SCL_DST_BL_SLOT_IDX;
    if(!strcmp("reserved_msi2", name))
        return RESERVED_MSI2;
    if(!strcmp("gb_src_bot_bl_slot_idx", name))
        return GB_SRC_BOT_BL_SLOT_IDX;
    if(!strcmp("gb_oth_bot_bl_slot_idx", name))
        return GB_OTH_BOT_BL_SLOT_IDX;
    if(!strcmp("gb_ref_bot_bl_slot_idx", name))
        return GB_REF_BOT_BL_SLOT_IDX;
    if(!strcmp("reserved_msi4", name))
        return RESERVED_MSI4;
    if(!strcmp("gb_src_proc_id", name))
        return GB_SRC_PROC_ID;
    if(!strcmp("gb_oth_proc_id", name))
        return GB_OTH_PROC_ID;
    if(!strcmp("gb_ref_proc_id", name))
        return GB_REF_PROC_ID;
    if(!strcmp("gb_src_bot_proc_id", name))
        return GB_SRC_BOT_PROC_ID;
    if(!strcmp("gb_oth_bot_proc_id", name))
        return GB_OTH_BOT_PROC_ID;
    if(!strcmp("gb_ref_bot_proc_id", name))
        return GB_REF_BOT_PROC_ID;
    if(!strcmp("gb_dst_proc_id", name))
        return GB_DST_PROC_ID;
    if(!strcmp("gb_scl_dst_proc_id", name))
        return GB_SCL_DST_PROC_ID;
    if(!strcmp("gb_mtd_hist_proc_id", name))
        return GB_MTD_HIST_PROC_ID;
    if(!strcmp("gb_hdr_ysum_proc_id", name))
        return GB_HDR_YSUM_PROC_ID;
    if(!strcmp("gb_hdr_int_proc_id", name))
        return GB_HDR_INT_PROC_ID;
    if(!strcmp("reserved_mpid1", name))
        return RESERVED_MPID1;
    if(!strcmp("gb_di_intra_th", name))
        return GB_DI_INTRA_TH;
    if(!strcmp("gb_di_isfeather_th", name))
        return GB_DI_ISFEATHER_TH;
    if(!strcmp("gb_di_force_eela_th", name))
        return GB_DI_FORCE_EELA_TH;
    if(!strcmp("reserved_di0", name))
        return RESERVED_DI0;
    if(!strcmp("gb_di_scroll_th", name))
        return GB_DI_SCROLL_TH;
    if(!strcmp("gb_point_degree_var_k", name))
        return GB_POINT_DEGREE_VAR_K;
    if(!strcmp("reserved_di1", name))
        return RESERVED_DI1;
    if(!strcmp("gb_sobel_th", name))
        return GB_SOBEL_TH;
    if(!strcmp("gb_sobel_force_value", name))
        return GB_SOBEL_FORCE_VALUE;
    if(!strcmp("gb_di_feather_edge_cnt_x1", name))
        return GB_DI_FEATHER_EDGE_CNT_X1;
    if(!strcmp("gb_di_feather_edge_cnt_y1", name))
        return GB_DI_FEATHER_EDGE_CNT_Y1;
    if(!strcmp("reserved_di2", name))
        return RESERVED_DI2;
    if(!strcmp("gb_di_iad_md_x1", name))
        return GB_DI_IAD_MD_X1;
    if(!strcmp("gb_di_iad_md_x2", name))
        return GB_DI_IAD_MD_X2;
    if(!strcmp("gb_di_iad_md_y1", name))
        return GB_DI_IAD_MD_Y1;
    if(!strcmp("gb_di_iad_md_k", name))
        return GB_DI_IAD_MD_K;
    if(!strcmp("reserved_di4", name))
        return RESERVED_DI4;
    if(!strcmp("gb_point_degree_var_x1", name))
        return GB_POINT_DEGREE_VAR_X1;
    if(!strcmp("gb_point_degree_var_x2", name))
        return GB_POINT_DEGREE_VAR_X2;
    if(!strcmp("gb_point_degree_var_y1", name))
        return GB_POINT_DEGREE_VAR_Y1;
    if(!strcmp("gb_top_bot_field_div", name))
        return GB_TOP_BOT_FIELD_DIV;
    if(!strcmp("gb_di_enable_staticflag", name))
        return GB_DI_ENABLE_STATICFLAG;
    if(!strcmp("gb_di_feather_edge_cnt_k", name))
        return GB_DI_FEATHER_EDGE_CNT_K;
    if(!strcmp("gb_di_feather_edge_cnt_x2", name))
        return GB_DI_FEATHER_EDGE_CNT_X2;
    if(!strcmp("gb_fd_wait_frame", name))
        return GB_FD_WAIT_FRAME;
    if(!strcmp("reserved_di6", name))
        return RESERVED_DI6;
    if(!strcmp("gb_edge_flag_th", name))
        return GB_EDGE_FLAG_TH;
    if(!strcmp("gb_di_intra_diff_th", name))
        return GB_DI_INTRA_DIFF_TH;
    if(!strcmp("reserved_di7", name))
        return RESERVED_DI7;
    if(!strcmp("gb_use_mv_judge", name))
        return GB_USE_MV_JUDGE;
    if(!strcmp("gb_use_allmv", name))
        return GB_USE_ALLMV;
    if(!strcmp("gb_mv_diff_th", name))
        return GB_MV_DIFF_TH;
    if(!strcmp("gb_direction_judge_th", name))
        return GB_DIRECTION_JUDGE_TH;
    if(!strcmp("gb_fd_bad_merge_th_1", name))
        return GB_FD_BAD_MERGE_TH_1;
    if(!strcmp("gb_fd_bad_merge_th_2", name))
        return GB_FD_BAD_MERGE_TH_2;
    if(!strcmp("reserved_di8", name))
        return RESERVED_DI8;
    if(!strcmp("gb_di_md_area_th0", name))
        return GB_DI_MD_AREA_TH0;
    if(!strcmp("gb_di_md_area_th1", name))
        return GB_DI_MD_AREA_TH1;
    if(!strcmp("gb_di_md_area_coef0", name))
        return GB_DI_MD_AREA_COEF0;
    if(!strcmp("gb_di_md_area_coef1", name))
        return GB_DI_MD_AREA_COEF1;
    if(!strcmp("gb_di_md_adj_coef0", name))
        return GB_DI_MD_ADJ_COEF0;
    if(!strcmp("gb_di_md_adj_coef1", name))
        return GB_DI_MD_ADJ_COEF1;
    if(!strcmp("reserved_dia", name))
        return RESERVED_DIA;
    if(!strcmp("gb_di_iad_ave_adj_th0", name))
        return GB_DI_IAD_AVE_ADJ_TH0;
    if(!strcmp("gb_di_iad_ave_adj_th1", name))
        return GB_DI_IAD_AVE_ADJ_TH1;
    if(!strcmp("reserved_dib", name))
        return RESERVED_DIB;
    if(!strcmp("gb_di_iad_ave_adj_val", name))
        return GB_DI_IAD_AVE_ADJ_VAL;
    if(!strcmp("gb_di_iad_ave_adj_coef", name))
        return GB_DI_IAD_AVE_ADJ_COEF;
    if(!strcmp("gb_di_mtd_featherdetect_en", name))
        return GB_DI_MTD_FEATHERDETECT_EN;
    if(!strcmp("reserved_dic", name))
        return RESERVED_DIC;
    if(!strcmp("gb_di_iad_line_md_x1", name))
        return GB_DI_IAD_LINE_MD_X1;
    if(!strcmp("gb_di_iad_line_md_x2", name))
        return GB_DI_IAD_LINE_MD_X2;
    if(!strcmp("gb_di_mtd_featherpara", name))
        return GB_DI_MTD_FEATHERPARA;
    if(!strcmp("gb_di_iad_line_md_y1", name))
        return GB_DI_IAD_LINE_MD_Y1;
    if(!strcmp("gb_di_iad_line_md_k", name))
        return GB_DI_IAD_LINE_MD_K;
    if(!strcmp("gb_di_th", name))
        return GB_DI_TH;
    if(!strcmp("reserved_vc01", name))
        return RESERVED_VC01;
    if(!strcmp("gb_md_th2", name))
        return GB_MD_TH2;
    if(!strcmp("gb_md_th2_en", name))
        return GB_MD_TH2_EN;
    if(!strcmp("reserved_vc0", name))
        return RESERVED_VC0;
    if(!strcmp("gb_md_th", name))
        return GB_MD_TH;
    if(!strcmp("gb_di_sr_rev_diff_th", name))
        return GB_DI_SR_REV_DIFF_TH;
    if(!strcmp("reserved_vc3", name))
        return RESERVED_VC3;
    if(!strcmp("gb_di_mtd_edgevalthr", name))
        return GB_DI_MTD_EDGEVALTHR;
    if(!strcmp("gb_di_mtd_staticpara", name))
        return GB_DI_MTD_STATICPARA;
    if(!strcmp("gb_di_mtd_maxminthr", name))
        return GB_DI_MTD_MAXMINTHR;
    if(!strcmp("gb_di_sr_grad_th", name))
        return GB_DI_SR_GRAD_TH;
    if(!strcmp("gb_di_mtd_md_th", name))
        return GB_DI_MTD_MD_TH;
    if(!strcmp("reserved_vc4", name))
        return RESERVED_VC4;
    if(!strcmp("gb_di_sr_line_diff_th", name))
        return GB_DI_SR_LINE_DIFF_TH;
    if(!strcmp("gb_di_iad_md_ave_th", name))
        return GB_DI_IAD_MD_AVE_TH;
    if(!strcmp("reserved_vc12", name))
        return RESERVED_VC12;
    if(!strcmp("gb_mtd_col_hist_base", name))
        return GB_MTD_COL_HIST_BASE;
    if(!strcmp("gb_md_low_th", name))
        return GB_MD_LOW_TH;
    if(!strcmp("reserved_vc9", name))
        return RESERVED_VC9;
    if(!strcmp("gb_lbuf_size", name))
        return GB_LBUF_SIZE;
    if(!strcmp("gb_vpp_in_size", name))
        return GB_VPP_IN_SIZE;
    if(!strcmp("gb_disable_lcid_blend", name))
        return GB_DISABLE_LCID_BLEND;
    if(!strcmp("gb_fd_force_di", name))
        return GB_FD_FORCE_DI;
    if(!strcmp("gb_di_blend_coef", name))
        return GB_DI_BLEND_COEF;
    if(!strcmp("reserved_vl0", name))
        return RESERVED_VL0;
    if(!strcmp("gb_vpp_di_offset", name))
        return GB_VPP_DI_OFFSET;
    if(!strcmp("gb_vpp_di_size", name))
        return GB_VPP_DI_SIZE;
    if(!strcmp("gb_vpp_diref_in_offset", name))
        return GB_VPP_DIREF_IN_OFFSET;
    if(!strcmp("reserved_vl1", name))
        return RESERVED_VL1;
    if(!strcmp("gb_fd_md_th", name))
        return GB_FD_MD_TH;
    if(!strcmp("gb_scene_change_top", name))
        return GB_SCENE_CHANGE_TOP;
    if(!strcmp("gb_scene_change_bottom", name))
        return GB_SCENE_CHANGE_BOTTOM;
    if(!strcmp("gb_2v2_th_delta", name))
        return GB_2V2_TH_DELTA;
    if(!strcmp("gb_2v2_th_ratio", name))
        return GB_2V2_TH_RATIO;
    if(!strcmp("gb_2v2_sad_counter_th", name))
        return GB_2V2_SAD_COUNTER_TH;
    if(!strcmp("gb_2v2_md_failed_counter_th", name))
        return GB_2V2_MD_FAILED_COUNTER_TH;
    if(!strcmp("gb_lcid_diff_th", name))
        return GB_LCID_DIFF_TH;
    if(!strcmp("gb_2v2_count_mode", name))
        return GB_2V2_COUNT_MODE;
    if(!strcmp("gb_scene_change_th", name))
        return GB_SCENE_CHANGE_TH;
    if(!strcmp("gb_scenechange_md_mode", name))
        return GB_SCENECHANGE_MD_MODE;
    if(!strcmp("gb_fd_md_th2", name))
        return GB_FD_MD_TH2;
    if(!strcmp("gb_fd_counter_th", name))
        return GB_FD_COUNTER_TH;
    if(!strcmp("gb_fd_detect_counter_initial", name))
        return GB_FD_DETECT_COUNTER_INITIAL;
    if(!strcmp("gb_fd_strong_counter_initial", name))
        return GB_FD_STRONG_COUNTER_INITIAL;
    if(!strcmp("gb_2v2_failed_counter_th", name))
        return GB_2V2_FAILED_COUNTER_TH;
    if(!strcmp("gb_2v2_md_succeed_counter_th", name))
        return GB_2V2_MD_SUCCEED_COUNTER_TH;
    if(!strcmp("gb_2v2_sad_succeed_counter_th", name))
        return GB_2V2_SAD_SUCCEED_COUNTER_TH;
    if(!strcmp("gb_2v2_sad_failed_range", name))
        return GB_2V2_SAD_FAILED_RANGE;
    if(!strcmp("gb_2v2_frame_cnt_th", name))
        return GB_2V2_FRAME_CNT_TH;
    if(!strcmp("gb_2v2_failed_counter_th2", name))
        return GB_2V2_FAILED_COUNTER_TH2;
    if(!strcmp("gb_fd_2v2_sad_clr_th_1", name))
        return GB_FD_2V2_SAD_CLR_TH_1;
    if(!strcmp("gb_fd_2v2_sad_clr_th_2", name))
        return GB_FD_2V2_SAD_CLR_TH_2;
    if(!strcmp("gb_fd_2v2_sad_clr_th_3", name))
        return GB_FD_2V2_SAD_CLR_TH_3;
    if(!strcmp("reserved_fd4", name))
        return RESERVED_FD4;
    if(!strcmp("gb_fd_bad_copy_th_1", name))
        return GB_FD_BAD_COPY_TH_1;
    if(!strcmp("gb_fd_bad_copy_th_2", name))
        return GB_FD_BAD_COPY_TH_2;
    if(!strcmp("gb_fd_bad_copy_th_3", name))
        return GB_FD_BAD_COPY_TH_3;
    if(!strcmp("gb_fd_bad_copy_th_4", name))
        return GB_FD_BAD_COPY_TH_4;
    if(!strcmp("gb_fd_quit_2v2_th_1", name))
        return GB_FD_QUIT_2V2_TH_1;
    if(!strcmp("reserved_fd5", name))
        return RESERVED_FD5;
    if(!strcmp("gb_fd_quit_2v2_th_2", name))
        return GB_FD_QUIT_2V2_TH_2;
    if(!strcmp("gb_fd_quit_2v2_th_3", name))
        return GB_FD_QUIT_2V2_TH_3;
    if(!strcmp("gb_fd_quit_2v2_th_4", name))
        return GB_FD_QUIT_2V2_TH_4;
    if(!strcmp("gb_fd_quit_2v2_th_5", name))
        return GB_FD_QUIT_2V2_TH_5;
    if(!strcmp("gb_fd_2v2_md_th_3", name))
        return GB_FD_2V2_MD_TH_3;
    if(!strcmp("gb_fd_2v2_md_th_5", name))
        return GB_FD_2V2_MD_TH_5;
    if(!strcmp("gb_fd_2v2_md_th_1", name))
        return GB_FD_2V2_MD_TH_1;
    if(!strcmp("gb_fd_2v2_md_th_2", name))
        return GB_FD_2V2_MD_TH_2;
    if(!strcmp("gb_fd_2v2_md_th_4", name))
        return GB_FD_2V2_MD_TH_4;
    if(!strcmp("gb_fd_sc_quit_2v2_th", name))
        return GB_FD_SC_QUIT_2V2_TH;
    if(!strcmp("gb_di_cg_off", name))
        return GB_DI_CG_OFF;
    if(!strcmp("gb_scl_cg_off", name))
        return GB_SCL_CG_OFF;
    if(!strcmp("gb_scl_bp_cg_off", name))
        return GB_SCL_BP_CG_OFF;
    if(!strcmp("gb_cscb_cg_off", name))
        return GB_CSCB_CG_OFF;
    if(!strcmp("gb_cscf_cg_off", name))
        return GB_CSCF_CG_OFF;
    if(!strcmp("gb_cg_busy_th", name))
        return GB_CG_BUSY_TH;
    if(!strcmp("gb_fire_after_save", name))
        return GB_FIRE_AFTER_SAVE;
    if(!strcmp("gb_restore_keep_on", name))
        return GB_RESTORE_KEEP_ON;
    if(!strcmp("gb_for_eco2", name))
        return GB_FOR_ECO2;
    if(!strcmp("gb_cpt_cg_off", name))
        return GB_CPT_CG_OFF;
    if(!strcmp("gb_sr_cg_off", name))
        return GB_SR_CG_OFF;
    if(!strcmp("gb_cm_cg_off", name))
        return GB_CM_CG_OFF;
    if(!strcmp("gb_ccm_cg_off", name))
        return GB_CCM_CG_OFF;
    if(!strcmp("gb_hdr_cg_off", name))
        return GB_HDR_CG_OFF;
    if(!strcmp("reserved_cg", name))
        return RESERVED_CG;
    if(!strcmp("gb_di_en", name))
        return GB_DI_EN;
    if(!strcmp("gb_fd_en", name))
        return GB_FD_EN;
    if(!strcmp("gb_mtd_en", name))
        return GB_MTD_EN;
    if(!strcmp("gb_two_frame_out_en", name))
        return GB_TWO_FRAME_OUT_EN;
    if(!strcmp("gb_di_force_lcid", name))
        return GB_DI_FORCE_LCID;
    if(!strcmp("gb_single_sel_oth", name))
        return GB_SINGLE_SEL_OTH;
    if(!strcmp("gb_top_field_first", name))
        return GB_TOP_FIELD_FIRST;
    if(!strcmp("gb_reset_hqvpp", name))
        return GB_RESET_HQVPP;
    if(!strcmp("gb_hqvpp_mode", name))
        return GB_HQVPP_MODE;
    if(!strcmp("gb_vpp_fd_index", name))
        return GB_VPP_FD_INDEX;
    if(!strcmp("reserved_opcode", name))
        return RESERVED_OPCODE;
    if(!strcmp("gb_cmlut_en", name))
        return GB_CMLUT_EN;
    if(!strcmp("gb_tint_en", name))
        return GB_TINT_EN;
    if(!strcmp("gb_tint_hoffset", name))
        return GB_TINT_HOFFSET;
    if(!strcmp("reserved_cm0", name))
        return RESERVED_CM0;
    if(!strcmp("gb_cmlut0", name))
        return GB_CMLUT0;
    if(!strcmp("gb_cmlut1", name))
        return GB_CMLUT1;
    if(!strcmp("reserved_cm1", name))
        return RESERVED_CM1;
    if(!strcmp("gb_ccm_en", name))
        return GB_CCM_EN;
    if(!strcmp("gb_metadata_en", name))
        return GB_METADATA_EN;
    if(!strcmp("gb_dg_hcoe", name))
        return GB_DG_HCOE;
    if(!strcmp("reserved_ccm0", name))
        return RESERVED_CCM0;
    if(!strcmp("gb_dg_lcoe", name))
        return GB_DG_LCOE;
    if(!strcmp("gb_yuv2rgb_en", name))
        return GB_YUV2RGB_EN;
    if(!strcmp("gb_yuv2rgb_mode", name))
        return GB_YUV2RGB_MODE;
    if(!strcmp("gb_ccm_hdr_inv", name))
        return GB_CCM_HDR_INV;
    if(!strcmp("reserved_ccm1", name))
        return RESERVED_CCM1;
    if(!strcmp("gb_dg_tbl_0", name))
        return GB_DG_TBL_0;
    if(!strcmp("gb_dg_tbl_1", name))
        return GB_DG_TBL_1;
    if(!strcmp("gb_g_tbl_0", name))
        return GB_G_TBL_0;
    if(!strcmp("gb_g_tbl_1", name))
        return GB_G_TBL_1;
    if(!strcmp("gb_g_tbl_2", name))
        return GB_G_TBL_2;
    if(!strcmp("reserved_ccm5", name))
        return RESERVED_CCM5;
    if(!strcmp("gb_hlg_en", name))
        return GB_HLG_EN;
    if(!strcmp("gb_hlg_acoef", name))
        return GB_HLG_ACOEF;
    if(!strcmp("gb_hlg_bcoef", name))
        return GB_HLG_BCOEF;
    if(!strcmp("reserved_ccm6", name))
        return RESERVED_CCM6;
    if(!strcmp("gb_hlg_tbl_0", name))
        return GB_HLG_TBL_0;
    if(!strcmp("gb_hlg_tbl_1", name))
        return GB_HLG_TBL_1;
    if(!strcmp("gb_hlg_tbl_20", name))
        return GB_HLG_TBL_20;
    if(!strcmp("gb_hlg_tbl_21", name))
        return GB_HLG_TBL_21;
    if(!strcmp("gb_hlg_tbl_22", name))
        return GB_HLG_TBL_22;
    if(!strcmp("gb_hlg_invtbl_22", name))
        return GB_HLG_INVTBL_22;
    if(!strcmp("gb_1886_acoef", name))
        return GB_1886_ACOEF;
    if(!strcmp("gb_1886_bcoef", name))
        return GB_1886_BCOEF;
    if(!strcmp("gb_hlg_invacoef", name))
        return GB_HLG_INVACOEF;
    if(!strcmp("gb_hlg_invbcoef", name))
        return GB_HLG_INVBCOEF;
    if(!strcmp("gb_1886_en", name))
        return GB_1886_EN;
    if(!strcmp("reserved_ccm9", name))
        return RESERVED_CCM9;
    if(!strcmp("gb_hlg_invtbl_0", name))
        return GB_HLG_INVTBL_0;
    if(!strcmp("gb_hlg_invtbl_1", name))
        return GB_HLG_INVTBL_1;
    if(!strcmp("gb_hlg_invtbl_20", name))
        return GB_HLG_INVTBL_20;
    if(!strcmp("gb_hlg_invtbl_21", name))
        return GB_HLG_INVTBL_21;
    if(!strcmp("gb_1886_invacoef", name))
        return GB_1886_INVACOEF;
    if(!strcmp("gb_1886_invbcoef", name))
        return GB_1886_INVBCOEF;
    if(!strcmp("gb_convert_m_00", name))
        return GB_CONVERT_M_00;
    if(!strcmp("gb_convert_m_01", name))
        return GB_CONVERT_M_01;
    if(!strcmp("reserved_cc0", name))
        return RESERVED_CC0;
    if(!strcmp("gb_convert_m_10", name))
        return GB_CONVERT_M_10;
    if(!strcmp("gb_convert_m_11", name))
        return GB_CONVERT_M_11;
    if(!strcmp("reserved_cc1", name))
        return RESERVED_CC1;
    if(!strcmp("gb_convert_m_20", name))
        return GB_CONVERT_M_20;
    if(!strcmp("gb_convert_m_21", name))
        return GB_CONVERT_M_21;
    if(!strcmp("reserved_cc2", name))
        return RESERVED_CC2;
    if(!strcmp("gb_convert_m_02", name))
        return GB_CONVERT_M_02;
    if(!strcmp("gb_convert_m_12", name))
        return GB_CONVERT_M_12;
    if(!strcmp("reserved_cc3", name))
        return RESERVED_CC3;
    if(!strcmp("gb_convert_m_22", name))
        return GB_CONVERT_M_22;
    if(!strcmp("reserved_cc4", name))
        return RESERVED_CC4;
    if(!strcmp("gb_hadj_en", name))
        return GB_HADJ_EN;
    if(!strcmp("gb_hrange_gl", name))
        return GB_HRANGE_GL;
    if(!strcmp("gb_hrange_gh", name))
        return GB_HRANGE_GH;
    if(!strcmp("reserved_hadj0", name))
        return RESERVED_HADJ0;
    if(!strcmp("gb_hrange_rl", name))
        return GB_HRANGE_RL;
    if(!strcmp("gb_hrange_rh", name))
        return GB_HRANGE_RH;
    if(!strcmp("reserved_hadj1", name))
        return RESERVED_HADJ1;
    if(!strcmp("gb_g_range", name))
        return GB_G_RANGE;
    if(!strcmp("gb_g_axis", name))
        return GB_G_AXIS;
    if(!strcmp("gb_g_coef", name))
        return GB_G_COEF;
    if(!strcmp("gb_r_range", name))
        return GB_R_RANGE;
    if(!strcmp("gb_r_axis", name))
        return GB_R_AXIS;
    if(!strcmp("gb_r_coef", name))
        return GB_R_COEF;
    if(!strcmp("gb_sadj_en", name))
        return GB_SADJ_EN;
    if(!strcmp("gb_sadj_inc00", name))
        return GB_SADJ_INC00;
    if(!strcmp("gb_sadj_inc01", name))
        return GB_SADJ_INC01;
    if(!strcmp("reserved_sadj0", name))
        return RESERVED_SADJ0;
    if(!strcmp("gb_sadj_inc02", name))
        return GB_SADJ_INC02;
    if(!strcmp("gb_sadj_inc03", name))
        return GB_SADJ_INC03;
    if(!strcmp("reserved_sadj1", name))
        return RESERVED_SADJ1;
    if(!strcmp("gb_sadj_inc04", name))
        return GB_SADJ_INC04;
    if(!strcmp("gb_sadj_inc05", name))
        return GB_SADJ_INC05;
    if(!strcmp("reserved_sadj2", name))
        return RESERVED_SADJ2;
    if(!strcmp("gb_sadj_inc06", name))
        return GB_SADJ_INC06;
    if(!strcmp("gb_sadj_inc07", name))
        return GB_SADJ_INC07;
    if(!strcmp("gb_sadj_inc14", name))
        return GB_SADJ_INC14;
    if(!strcmp("reserved_sadj3", name))
        return RESERVED_SADJ3;
    if(!strcmp("gb_sadj_inc08", name))
        return GB_SADJ_INC08;
    if(!strcmp("gb_sadj_inc09", name))
        return GB_SADJ_INC09;
    if(!strcmp("reserved_sadj4", name))
        return RESERVED_SADJ4;
    if(!strcmp("gb_sadj_inc0a", name))
        return GB_SADJ_INC0A;
    if(!strcmp("gb_sadj_inc0b", name))
        return GB_SADJ_INC0B;
    if(!strcmp("gb_sadj_inc13", name))
        return GB_SADJ_INC13;
    if(!strcmp("reserved_sadj5", name))
        return RESERVED_SADJ5;
    if(!strcmp("gb_sadj_inc0c", name))
        return GB_SADJ_INC0C;
    if(!strcmp("gb_sadj_inc0d", name))
        return GB_SADJ_INC0D;
    if(!strcmp("reserved_sadj6", name))
        return RESERVED_SADJ6;
    if(!strcmp("gb_sadj_inc0e", name))
        return GB_SADJ_INC0E;
    if(!strcmp("gb_sadj_inc0f", name))
        return GB_SADJ_INC0F;
    if(!strcmp("gb_sadj_inc12", name))
        return GB_SADJ_INC12;
    if(!strcmp("reserved_sadj7", name))
        return RESERVED_SADJ7;
    if(!strcmp("gb_sadj_inc10", name))
        return GB_SADJ_INC10;
    if(!strcmp("gb_sadj_inc11", name))
        return GB_SADJ_INC11;
    if(!strcmp("reserved_sadj8", name))
        return RESERVED_SADJ8;
    if(!strcmp("gb_hdr_en", name))
        return GB_HDR_EN;
    if(!strcmp("gb_hdr_curve_en", name))
        return GB_HDR_CURVE_EN;
    if(!strcmp("gb_hdr_cliplmt", name))
        return GB_HDR_CLIPLMT;
    if(!strcmp("gb_hdr_maxfmean", name))
        return GB_HDR_MAXFMEAN;
    if(!strcmp("gb_hdr_maxmean", name))
        return GB_HDR_MAXMEAN;
    if(!strcmp("gb_hdr_sc_en", name))
        return GB_HDR_SC_EN;
    if(!strcmp("gb_hdr_smallsize", name))
        return GB_HDR_SMALLSIZE;
    if(!strcmp("gb_hdr_maxpeakcnt", name))
        return GB_HDR_MAXPEAKCNT;
    if(!strcmp("gb_hdr_minpeakwg", name))
        return GB_HDR_MINPEAKWG;
    if(!strcmp("gb_hdr_maxpeakwg", name))
        return GB_HDR_MAXPEAKWG;
    if(!strcmp("gb_hdr_manualclipmode", name))
        return GB_HDR_MANUALCLIPMODE;
    if(!strcmp("gb_hdr_frmwg", name))
        return GB_HDR_FRMWG;
    if(!strcmp("gb_reset_hdr", name))
        return GB_RESET_HDR;
    if(!strcmp("reserved_hdr1", name))
        return RESERVED_HDR1;
    if(!strcmp("gb_hdr_pixcntforstat", name))
        return GB_HDR_PIXCNTFORSTAT;
    if(!strcmp("gb_hdr_shft", name))
        return GB_HDR_SHFT;
    if(!strcmp("gb_hdr_minstretchrange", name))
        return GB_HDR_MINSTRETCHRANGE;
    if(!strcmp("reserved_hdr3", name))
        return RESERVED_HDR3;
    if(!strcmp("gb_hdr_minfiltspd", name))
        return GB_HDR_MINFILTSPD;
    if(!strcmp("gb_hdr_binmedsdt", name))
        return GB_HDR_BINMEDSDT;
    if(!strcmp("gb_hdr_deltameanth", name))
        return GB_HDR_DELTAMEANTH;
    if(!strcmp("gb_hdr_binmedhimax", name))
        return GB_HDR_BINMEDHIMAX;
    if(!strcmp("gb_hdr_yluthiwginit", name))
        return GB_HDR_YLUTHIWGINIT;
    if(!strcmp("reserved_hdr4", name))
        return RESERVED_HDR4;
    if(!strcmp("gb_hdr_meanchangeth", name))
        return GB_HDR_MEANCHANGETH;
    if(!strcmp("reserved_hdr5", name))
        return RESERVED_HDR5;
    if(!strcmp("gb_hdr_pixlowratio", name))
        return GB_HDR_PIXLOWRATIO;
    if(!strcmp("reserved_hdr6", name))
        return RESERVED_HDR6;
    if(!strcmp("gb_hdr_pixhiratio", name))
        return GB_HDR_PIXHIRATIO;
    if(!strcmp("reserved_hdr7", name))
        return RESERVED_HDR7;
    if(!strcmp("gb_hdr_blackratio", name))
        return GB_HDR_BLACKRATIO;
    if(!strcmp("reserved_hdr8", name))
        return RESERVED_HDR8;
    if(!strcmp("gb_hdr_noiseratio", name))
        return GB_HDR_NOISERATIO;
    if(!strcmp("reserved_hdr9", name))
        return RESERVED_HDR9;
    if(!strcmp("gb_hdr_yluthi_00", name))
        return GB_HDR_YLUTHI_00;
    if(!strcmp("gb_hdr_yluthi_01", name))
        return GB_HDR_YLUTHI_01;
    if(!strcmp("gb_hdr_yluthi_02", name))
        return GB_HDR_YLUTHI_02;
    if(!strcmp("reserved_hdr_ylut0", name))
        return RESERVED_HDR_YLUT0;
    if(!strcmp("gb_hdr_yluthi_03", name))
        return GB_HDR_YLUTHI_03;
    if(!strcmp("gb_hdr_yluthi_04", name))
        return GB_HDR_YLUTHI_04;
    if(!strcmp("gb_hdr_yluthi_05", name))
        return GB_HDR_YLUTHI_05;
    if(!strcmp("reserved_hdr_ylut1", name))
        return RESERVED_HDR_YLUT1;
    if(!strcmp("gb_hdr_yluthi_06", name))
        return GB_HDR_YLUTHI_06;
    if(!strcmp("gb_hdr_yluthi_07", name))
        return GB_HDR_YLUTHI_07;
    if(!strcmp("gb_hdr_yluthi_08", name))
        return GB_HDR_YLUTHI_08;
    if(!strcmp("reserved_hdr_ylut2", name))
        return RESERVED_HDR_YLUT2;
    if(!strcmp("gb_hdr_yluthi_09", name))
        return GB_HDR_YLUTHI_09;
    if(!strcmp("gb_hdr_yluthi_0a", name))
        return GB_HDR_YLUTHI_0A;
    if(!strcmp("gb_hdr_yluthi_0b", name))
        return GB_HDR_YLUTHI_0B;
    if(!strcmp("reserved_hdr_ylut3", name))
        return RESERVED_HDR_YLUT3;
    if(!strcmp("gb_hdr_yluthi_0c", name))
        return GB_HDR_YLUTHI_0C;
    if(!strcmp("gb_hdr_yluthi_0d", name))
        return GB_HDR_YLUTHI_0D;
    if(!strcmp("gb_hdr_yluthi_0e", name))
        return GB_HDR_YLUTHI_0E;
    if(!strcmp("reserved_hdr_ylut4", name))
        return RESERVED_HDR_YLUT4;
    if(!strcmp("gb_hdr_yluthi_0f", name))
        return GB_HDR_YLUTHI_0F;
    if(!strcmp("gb_hdr_yluthi_10", name))
        return GB_HDR_YLUTHI_10;
    if(!strcmp("gb_hdr_yluthi_11", name))
        return GB_HDR_YLUTHI_11;
    if(!strcmp("reserved_hdr_ylut5", name))
        return RESERVED_HDR_YLUT5;
    if(!strcmp("gb_hdr_yluthi_12", name))
        return GB_HDR_YLUTHI_12;
    if(!strcmp("gb_hdr_yluthi_13", name))
        return GB_HDR_YLUTHI_13;
    if(!strcmp("gb_hdr_yluthi_14", name))
        return GB_HDR_YLUTHI_14;
    if(!strcmp("reserved_hdr_ylut6", name))
        return RESERVED_HDR_YLUT6;
    if(!strcmp("gb_hdr_yluthi_15", name))
        return GB_HDR_YLUTHI_15;
    if(!strcmp("gb_hdr_yluthi_16", name))
        return GB_HDR_YLUTHI_16;
    if(!strcmp("gb_hdr_yluthi_17", name))
        return GB_HDR_YLUTHI_17;
    if(!strcmp("reserved_hdr_ylut7", name))
        return RESERVED_HDR_YLUT7;
    if(!strcmp("gb_hdr_yluthi_18", name))
        return GB_HDR_YLUTHI_18;
    if(!strcmp("gb_hdr_yluthi_19", name))
        return GB_HDR_YLUTHI_19;
    if(!strcmp("gb_hdr_yluthi_1a", name))
        return GB_HDR_YLUTHI_1A;
    if(!strcmp("reserved_hdr_ylut8", name))
        return RESERVED_HDR_YLUT8;
    if(!strcmp("gb_hdr_yluthi_1b", name))
        return GB_HDR_YLUTHI_1B;
    if(!strcmp("gb_hdr_yluthi_1c", name))
        return GB_HDR_YLUTHI_1C;
    if(!strcmp("gb_hdr_yluthi_1d", name))
        return GB_HDR_YLUTHI_1D;
    if(!strcmp("reserved_hdr_ylut9", name))
        return RESERVED_HDR_YLUT9;
    if(!strcmp("gb_hdr_yluthi_1e", name))
        return GB_HDR_YLUTHI_1E;
    if(!strcmp("gb_hdr_yluthi_1f", name))
        return GB_HDR_YLUTHI_1F;
    if(!strcmp("reserved_hdr_yluta", name))
        return RESERVED_HDR_YLUTA;
    if(!strcmp("gb_hdr_ylutlow_00", name))
        return GB_HDR_YLUTLOW_00;
    if(!strcmp("gb_hdr_ylutlow_01", name))
        return GB_HDR_YLUTLOW_01;
    if(!strcmp("gb_hdr_ylutlow_02", name))
        return GB_HDR_YLUTLOW_02;
    if(!strcmp("reserved_hdr_ylut10", name))
        return RESERVED_HDR_YLUT10;
    if(!strcmp("gb_hdr_ylutlow_03", name))
        return GB_HDR_YLUTLOW_03;
    if(!strcmp("gb_hdr_ylutlow_04", name))
        return GB_HDR_YLUTLOW_04;
    if(!strcmp("gb_hdr_ylutlow_05", name))
        return GB_HDR_YLUTLOW_05;
    if(!strcmp("reserved_hdr_ylut11", name))
        return RESERVED_HDR_YLUT11;
    if(!strcmp("gb_hdr_ylutlow_06", name))
        return GB_HDR_YLUTLOW_06;
    if(!strcmp("gb_hdr_ylutlow_07", name))
        return GB_HDR_YLUTLOW_07;
    if(!strcmp("gb_hdr_ylutlow_08", name))
        return GB_HDR_YLUTLOW_08;
    if(!strcmp("reserved_hdr_ylut12", name))
        return RESERVED_HDR_YLUT12;
    if(!strcmp("gb_hdr_ylutlow_09", name))
        return GB_HDR_YLUTLOW_09;
    if(!strcmp("gb_hdr_ylutlow_0a", name))
        return GB_HDR_YLUTLOW_0A;
    if(!strcmp("gb_hdr_ylutlow_0b", name))
        return GB_HDR_YLUTLOW_0B;
    if(!strcmp("reserved_hdr_ylut13", name))
        return RESERVED_HDR_YLUT13;
    if(!strcmp("gb_hdr_ylutlow_0c", name))
        return GB_HDR_YLUTLOW_0C;
    if(!strcmp("gb_hdr_ylutlow_0d", name))
        return GB_HDR_YLUTLOW_0D;
    if(!strcmp("gb_hdr_ylutlow_0e", name))
        return GB_HDR_YLUTLOW_0E;
    if(!strcmp("reserved_hdr_ylut14", name))
        return RESERVED_HDR_YLUT14;
    if(!strcmp("gb_hdr_ylutlow_0f", name))
        return GB_HDR_YLUTLOW_0F;
    if(!strcmp("gb_hdr_ylutlow_10", name))
        return GB_HDR_YLUTLOW_10;
    if(!strcmp("gb_hdr_ylutlow_11", name))
        return GB_HDR_YLUTLOW_11;
    if(!strcmp("reserved_hdr_ylut15", name))
        return RESERVED_HDR_YLUT15;
    if(!strcmp("gb_hdr_ylutlow_12", name))
        return GB_HDR_YLUTLOW_12;
    if(!strcmp("gb_hdr_ylutlow_13", name))
        return GB_HDR_YLUTLOW_13;
    if(!strcmp("gb_hdr_ylutlow_14", name))
        return GB_HDR_YLUTLOW_14;
    if(!strcmp("reserved_hdr_ylut16", name))
        return RESERVED_HDR_YLUT16;
    if(!strcmp("gb_hdr_ylutlow_15", name))
        return GB_HDR_YLUTLOW_15;
    if(!strcmp("gb_hdr_ylutlow_16", name))
        return GB_HDR_YLUTLOW_16;
    if(!strcmp("gb_hdr_ylutlow_17", name))
        return GB_HDR_YLUTLOW_17;
    if(!strcmp("reserved_hdr_ylut17", name))
        return RESERVED_HDR_YLUT17;
    if(!strcmp("gb_hdr_ylutlow_18", name))
        return GB_HDR_YLUTLOW_18;
    if(!strcmp("gb_hdr_ylutlow_19", name))
        return GB_HDR_YLUTLOW_19;
    if(!strcmp("gb_hdr_ylutlow_1a", name))
        return GB_HDR_YLUTLOW_1A;
    if(!strcmp("reserved_hdr_ylut18", name))
        return RESERVED_HDR_YLUT18;
    if(!strcmp("gb_hdr_ylutlow_1b", name))
        return GB_HDR_YLUTLOW_1B;
    if(!strcmp("gb_hdr_ylutlow_1c", name))
        return GB_HDR_YLUTLOW_1C;
    if(!strcmp("gb_hdr_ylutlow_1d", name))
        return GB_HDR_YLUTLOW_1D;
    if(!strcmp("reserved_hdr_ylut19", name))
        return RESERVED_HDR_YLUT19;
    if(!strcmp("gb_hdr_ylutlow_1e", name))
        return GB_HDR_YLUTLOW_1E;
    if(!strcmp("gb_hdr_ylutlow_1f", name))
        return GB_HDR_YLUTLOW_1F;
    if(!strcmp("reserved_hdr_ylut1a", name))
        return RESERVED_HDR_YLUT1A;
    printf("Error! There is no register named as %s, please check vector setting!\n", name);
    assert(0);
    return 0;
}


static inline unsigned int Vpp_Global_get_reg_name(int reg, char * name)
{
    switch(reg)
    {
        case GB_SRC_SF_BASE:
            strcpy(name,"gb_src_sf_base");
            return 1;
        case GB_OTH_SF_BASE:
            strcpy(name,"gb_oth_sf_base");
            return 1;
        case GB_REF_SF_BASE:
            strcpy(name,"gb_ref_sf_base");
            return 1;
        case GB_SRC_RANGE_MAP_Y_EN:
            strcpy(name,"gb_src_range_map_y_en");
            return 1;
        case GB_SRC_RANGE_MAP_Y:
            strcpy(name,"gb_src_range_map_y");
            return 1;
        case GB_SRC_RANGE_MAP_UV_EN:
            strcpy(name,"gb_src_range_map_uv_en");
            return 1;
        case GB_SRC_RANGE_MAP_UV:
            strcpy(name,"gb_src_range_map_uv");
            return 1;
        case GB_OTH_RANGE_MAP_Y_EN:
            strcpy(name,"gb_oth_range_map_y_en");
            return 1;
        case GB_OTH_RANGE_MAP_Y:
            strcpy(name,"gb_oth_range_map_y");
            return 1;
        case GB_OTH_RANGE_MAP_UV_EN:
            strcpy(name,"gb_oth_range_map_uv_en");
            return 1;
        case GB_OTH_RANGE_MAP_UV:
            strcpy(name,"gb_oth_range_map_uv");
            return 1;
        case GB_REF_RANGE_MAP_Y_EN:
            strcpy(name,"gb_ref_range_map_y_en");
            return 1;
        case GB_REF_RANGE_MAP_Y:
            strcpy(name,"gb_ref_range_map_y");
            return 1;
        case GB_REF_RANGE_MAP_UV_EN:
            strcpy(name,"gb_ref_range_map_uv_en");
            return 1;
        case GB_REF_RANGE_MAP_UV:
            strcpy(name,"gb_ref_range_map_uv");
            return 1;
        case RESERVED_BM:
            strcpy(name,"reserved_bm");
            return 1;
        case GB_DST_SF_BASE:
            strcpy(name,"gb_dst_sf_base");
            return 1;
        case GB_SCL_DST_SF_BASE:
            strcpy(name,"gb_scl_dst_sf_base");
            return 1;
        case GB_SCL_DST_CLIPX:
            strcpy(name,"gb_scl_dst_clipx");
            return 1;
        case GB_SCL_DST_CLIPY:
            strcpy(name,"gb_scl_dst_clipy");
            return 1;
        case RESERVED_MOB:
            strcpy(name,"reserved_mob");
            return 1;
        case GB_SCL_DST_SF_HEIGHT:
            strcpy(name,"gb_scl_dst_sf_height");
            return 1;
        case RESERVED_SDB2:
            strcpy(name,"reserved_sdb2");
            return 1;
        case GB_SRC_SF_BOT_BASE:
            strcpy(name,"gb_src_sf_bot_base");
            return 1;
        case GB_OTH_SF_BOT_BASE:
            strcpy(name,"gb_oth_sf_bot_base");
            return 1;
        case GB_REF_SF_BOT_BASE:
            strcpy(name,"gb_ref_sf_bot_base");
            return 1;
        case GB_HDR_INT_BASE:
            strcpy(name,"gb_hdr_int_base");
            return 1;
        case GB_YSUM_MEM_BASE:
            strcpy(name,"gb_ysum_mem_base");
            return 1;
        case GB_HS_ERROR:
            strcpy(name,"gb_hs_error");
            return 1;
        case GB_VS_ERROR:
            strcpy(name,"gb_vs_error");
            return 1;
        case GB_SRC_WIDTH:
            strcpy(name,"gb_src_width");
            return 1;
        case GB_SEQ_ID_THRD:
            strcpy(name,"gb_seq_id_thrd");
            return 1;
        case RESERVED_SRC_SF_0:
            strcpy(name,"reserved_src_sf_0");
            return 1;
        case RESERVED_SRC_SF_1:
            strcpy(name,"reserved_src_sf_1");
            return 1;
        case GB_SRC_FMT:
            strcpy(name,"gb_src_fmt");
            return 1;
        case GB_SRC_TILE:
            strcpy(name,"gb_src_tile");
            return 1;
        case GB_SRC_HEIGHT:
            strcpy(name,"gb_src_height");
            return 1;
        case GB_SRC_PITCH:
            strcpy(name,"gb_src_pitch");
            return 1;
        case GB_SCL_DST_FMT:
            strcpy(name,"gb_scl_dst_fmt");
            return 1;
        case GB_SCL_DST_HEIGHT:
            strcpy(name,"gb_scl_dst_height");
            return 1;
        case GB_SCL_DST_PITCH:
            strcpy(name,"gb_scl_dst_pitch");
            return 1;
        case GB_SCL_DST_COMPRESS_EN:
            strcpy(name,"gb_scl_dst_compress_en");
            return 1;
        case RESERVED_SCL_DST_SF_1:
            strcpy(name,"reserved_scl_dst_sf_1");
            return 1;
        case GB_SCL_DST_WIDTH:
            strcpy(name,"gb_scl_dst_width");
            return 1;
        case RESERVE_DST2:
            strcpy(name,"reserve_dst2");
            return 1;
        case RESERVED_RS:
            strcpy(name,"reserved_rs");
            return 1;
        case GB_VPP_DNREF_SF_PITCH:
            strcpy(name,"gb_vpp_dnref_sf_pitch");
            return 1;
        case GB_VPP_DNREF_SF_HEIGHT:
            strcpy(name,"gb_vpp_dnref_sf_height");
            return 1;
        case GB_BLT_MODE:
            strcpy(name,"gb_blt_mode");
            return 1;
        case GB_SCL_MODE:
            strcpy(name,"gb_scl_mode");
            return 1;
        case GB_EN_HSCL:
            strcpy(name,"gb_en_hscl");
            return 1;
        case GB_EN_VSCL:
            strcpy(name,"gb_en_vscl");
            return 1;
        case RESERVED_GB_FIRST_SLICE:
            strcpy(name,"reserved_gb_first_slice");
            return 1;
        case GB_SE_VPP:
            strcpy(name,"gb_se_vpp");
            return 1;
        case GB_ROTATION_MODE:
            strcpy(name,"gb_rotation_mode");
            return 1;
        case GB_SRC_COMPRESS:
            strcpy(name,"gb_src_compress");
            return 1;
        case GB_FOR_ECO:
            strcpy(name,"gb_for_eco");
            return 1;
        case GB_X_UPSAMPLE_MODE:
            strcpy(name,"gb_x_upsample_mode");
            return 1;
        case GB_DS_24_X:
            strcpy(name,"gb_ds_24_x");
            return 1;
        case GB_DS_24_Y:
            strcpy(name,"gb_ds_24_y");
            return 1;
        case GB_BLT_SIMPLE:
            strcpy(name,"gb_blt_simple");
            return 1;
        case GB_EN_4K_MEM_SWZL:
            strcpy(name,"gb_en_4k_mem_swzl");
            return 1;
        case GB_DS4_LSB2FORCE:
            strcpy(name,"gb_ds4_lsb2force");
            return 1;
        case GB_FIXCOLOR_MODE:
            strcpy(name,"gb_fixcolor_mode");
            return 1;
        case GB_Y_UPSAMPLE_MODE:
            strcpy(name,"gb_y_upsample_mode");
            return 1;
        case RESERVED_MS:
            strcpy(name,"reserved_ms");
            return 1;
        case RESERVED_GB_SCL_LEFT_EDGE_WIDTH:
            strcpy(name,"reserved_gb_scl_left_edge_width");
            return 1;
        case RESERVED_GB_SCL_RIGHT_EDGE_WIDTH:
            strcpy(name,"reserved_gb_scl_right_edge_width");
            return 1;
        case GB_LDRD_COMPRESS:
            strcpy(name,"gb_ldrd_compress");
            return 1;
        case GB_SEQ_ID_PEAKLEVEL:
            strcpy(name,"gb_seq_id_peaklevel");
            return 1;
        case RESERVED_DMS:
            strcpy(name,"reserved_dms");
            return 1;
        case GB_SLICE_WIDTH:
            strcpy(name,"gb_slice_width");
            return 1;
        case GB_SLICE_PIXEL_NUM:
            strcpy(name,"gb_slice_pixel_num");
            return 1;
        case GB_SLICE_WIDTH_1ST:
            strcpy(name,"gb_slice_width_1st");
            return 1;
        case GB_SLICE_PIXEL_NUM_1ST:
            strcpy(name,"gb_slice_pixel_num_1st");
            return 1;
        case GB_SIGNAT_SEL_IO:
            strcpy(name,"gb_signat_sel_io");
            return 1;
        case GB_SIGNAT_SEL_OO:
            strcpy(name,"gb_signat_sel_oo");
            return 1;
        case GB_SIG_ENADDR_IO:
            strcpy(name,"gb_sig_enaddr_io");
            return 1;
        case RESERVED_MP1:
            strcpy(name,"reserved_mp1");
            return 1;
        case GB_CL_LEFT:
            strcpy(name,"gb_cl_left");
            return 1;
        case GB_CL_RIGHT:
            strcpy(name,"gb_cl_right");
            return 1;
        case RESERVED17:
            strcpy(name,"reserved17");
            return 1;
        case GB_CL_TOP:
            strcpy(name,"gb_cl_top");
            return 1;
        case GB_CL_BOTTOM:
            strcpy(name,"gb_cl_bottom");
            return 1;
        case RESERVED16:
            strcpy(name,"reserved16");
            return 1;
        case GB_VPPCNTOUT:
            strcpy(name,"gb_vppcntout");
            return 1;
        case GB_VPPARBMODE:
            strcpy(name,"gb_vpparbmode");
            return 1;
        case GB_LB_SCL_YUV_OFFSET:
            strcpy(name,"gb_lb_scl_yuv_offset");
            return 1;
        case GB_LB_SCL_A_OFFSET:
            strcpy(name,"gb_lb_scl_a_offset");
            return 1;
        case RESERVED_LS0:
            strcpy(name,"reserved_ls0");
            return 1;
        case GB_LB_SCL_YUV_PITCH:
            strcpy(name,"gb_lb_scl_yuv_pitch");
            return 1;
        case GB_LB_SCL_A_PITCH:
            strcpy(name,"gb_lb_scl_a_pitch");
            return 1;
        case GB_LB_NM_OPER_OFFSET:
            strcpy(name,"gb_lb_nm_oper_offset");
            return 1;
        case RESERVED_LS01:
            strcpy(name,"reserved_ls01");
            return 1;
        case GB_LB_SR_Y_OFFSET:
            strcpy(name,"gb_lb_sr_y_offset");
            return 1;
        case GB_LB_SR_UV_OFFSET:
            strcpy(name,"gb_lb_sr_uv_offset");
            return 1;
        case GB_LB_SR_Y_PITCH:
            strcpy(name,"gb_lb_sr_y_pitch");
            return 1;
        case GB_LB_SR_A_OFFSET:
            strcpy(name,"gb_lb_sr_a_offset");
            return 1;
        case GB_LB_SR_UV_PITCH:
            strcpy(name,"gb_lb_sr_uv_pitch");
            return 1;
        case GB_LB_SR_A_PITCH:
            strcpy(name,"gb_lb_sr_a_pitch");
            return 1;
        case RESERVED_LS03:
            strcpy(name,"reserved_ls03");
            return 1;
        case GB_LBADIW:
            strcpy(name,"gb_lbadiw");
            return 1;
        case GB_LBASCLYUVW:
            strcpy(name,"gb_lbasclyuvw");
            return 1;
        case GB_LBASCLAW:
            strcpy(name,"gb_lbasclaw");
            return 1;
        case GB_LBASRUVW:
            strcpy(name,"gb_lbasruvw");
            return 1;
        case GB_LBA_BANK_MODE:
            strcpy(name,"gb_lba_bank_mode");
            return 1;
        case RESERVED_LS1:
            strcpy(name,"reserved_ls1");
            return 1;
        case GB_LBASRYW:
            strcpy(name,"gb_lbasryw");
            return 1;
        case GB_LBASRAW:
            strcpy(name,"gb_lbasraw");
            return 1;
        case RESERVED_LS2:
            strcpy(name,"reserved_ls2");
            return 1;
        case GB_LBADIR:
            strcpy(name,"gb_lbadir");
            return 1;
        case GB_LBADIREFR:
            strcpy(name,"gb_lbadirefr");
            return 1;
        case GB_LBASCRR:
            strcpy(name,"gb_lbascrr");
            return 1;
        case RESERVED_LS3:
            strcpy(name,"reserved_ls3");
            return 1;
        case GB_LBASCLYUVR:
            strcpy(name,"gb_lbasclyuvr");
            return 1;
        case GB_LBASCLAR:
            strcpy(name,"gb_lbasclar");
            return 1;
        case GB_LBASRUVR:
            strcpy(name,"gb_lbasruvr");
            return 1;
        case RESERVED_LS4:
            strcpy(name,"reserved_ls4");
            return 1;
        case GB_LBASRYR:
            strcpy(name,"gb_lbasryr");
            return 1;
        case GB_LBASRAR:
            strcpy(name,"gb_lbasrar");
            return 1;
        case RESERVED_LS5:
            strcpy(name,"reserved_ls5");
            return 1;
        case GB_MOB_FIXALPHA:
            strcpy(name,"gb_mob_fixalpha");
            return 1;
        case GB_REDUNDANT_EDGE_WIDTH:
            strcpy(name,"gb_redundant_edge_width");
            return 1;
        case GB_MOB_RANGE_MIN:
            strcpy(name,"gb_mob_range_min");
            return 1;
        case GB_MOB_RANGE_MAXRY:
            strcpy(name,"gb_mob_range_maxry");
            return 1;
        case GB_MOB_RANGE_MAXGBUV:
            strcpy(name,"gb_mob_range_maxgbuv");
            return 1;
        case GB_SRC_LOSSY_COM_EN:
            strcpy(name,"gb_src_lossy_com_en");
            return 1;
        case GB_DST_LOSSY_COM_EN:
            strcpy(name,"gb_dst_lossy_com_en");
            return 1;
        case GB_MOB_BLENDING_EN:
            strcpy(name,"gb_mob_blending_en");
            return 1;
        case GB_MOB_BLENDING_DIU_MODE:
            strcpy(name,"gb_mob_blending_diu_mode");
            return 1;
        case GB_MOB_BLENDING_STREAM_MODE:
            strcpy(name,"gb_mob_blending_stream_mode");
            return 1;
        case RESERVED_IR:
            strcpy(name,"reserved_ir");
            return 1;
        case GB_HSLC_Y_A0:
            strcpy(name,"gb_hslc_y_a0");
            return 1;
        case GB_HSLC_Y_B0:
            strcpy(name,"gb_hslc_y_b0");
            return 1;
        case RESERVED_HSLC0:
            strcpy(name,"reserved_hslc0");
            return 1;
        case GB_HSLC_UV_A0:
            strcpy(name,"gb_hslc_uv_a0");
            return 1;
        case GB_HSLC_UV_A1:
            strcpy(name,"gb_hslc_uv_a1");
            return 1;
        case RESERVED_HSLC1:
            strcpy(name,"reserved_hslc1");
            return 1;
        case GB_HSLC_U_B0:
            strcpy(name,"gb_hslc_u_b0");
            return 1;
        case GB_HSLC_V_B0:
            strcpy(name,"gb_hslc_v_b0");
            return 1;
        case RESERVED_HSLC2:
            strcpy(name,"reserved_hslc2");
            return 1;
        case GB_SD_WIDTH_RATIO:
            strcpy(name,"gb_sd_width_ratio");
            return 1;
        case GB_SD_HEIGHT_RATIO:
            strcpy(name,"gb_sd_height_ratio");
            return 1;
        case GB_DS_WIDTH_RATIO:
            strcpy(name,"gb_ds_width_ratio");
            return 1;
        case GB_DS_HEIGHT_RATIO:
            strcpy(name,"gb_ds_height_ratio");
            return 1;
        case RESERVED_DS:
            strcpy(name,"reserved_ds");
            return 1;
        case GB_SRC_EN_ALPHA:
            strcpy(name,"gb_src_en_alpha");
            return 1;
        case GB_SCL_BC_DELTA:
            strcpy(name,"gb_scl_bc_delta");
            return 1;
        case GB_EN_SCL_MAXMIN:
            strcpy(name,"gb_en_scl_maxmin");
            return 1;
        case GB_EN_SR:
            strcpy(name,"gb_en_sr");
            return 1;
        case GB_SR_CTDIFFTH:
            strcpy(name,"gb_sr_ctdiffth");
            return 1;
        case GB_EN_PIXELTYPE:
            strcpy(name,"gb_en_pixeltype");
            return 1;
        case GB_SR_BLENDSTRENGTH:
            strcpy(name,"gb_sr_blendstrength");
            return 1;
        case GB_SR_ENCTBLEND:
            strcpy(name,"gb_sr_enctblend");
            return 1;
        case GB_SR_BLENDWEIGHT:
            strcpy(name,"gb_sr_blendweight");
            return 1;
        case GB_SR_COEFWEIGHT:
            strcpy(name,"gb_sr_coefweight");
            return 1;
        case GB_SR_COEFWEIGHTOTHER:
            strcpy(name,"gb_sr_coefweightother");
            return 1;
        case RESERVED_SCL0:
            strcpy(name,"reserved_scl0");
            return 1;
        case GB_CSCF_M_00:
            strcpy(name,"gb_cscf_m_00");
            return 1;
        case GB_CSCF_M_01:
            strcpy(name,"gb_cscf_m_01");
            return 1;
        case GB_CSCF_EN:
            strcpy(name,"gb_cscf_en");
            return 1;
        case GB_CSCF_ROUND:
            strcpy(name,"gb_cscf_round");
            return 1;
        case RESERVED_CSCF_00_01:
            strcpy(name,"reserved_cscf_00_01");
            return 1;
        case GB_CSCF_M_02:
            strcpy(name,"gb_cscf_m_02");
            return 1;
        case GB_CSCF_M_10:
            strcpy(name,"gb_cscf_m_10");
            return 1;
        case RESERVED_CSCF_02_10:
            strcpy(name,"reserved_cscf_02_10");
            return 1;
        case GB_CSCF_M_11:
            strcpy(name,"gb_cscf_m_11");
            return 1;
        case GB_CSCF_M_12:
            strcpy(name,"gb_cscf_m_12");
            return 1;
        case RESERVED_CSCF_11_12:
            strcpy(name,"reserved_cscf_11_12");
            return 1;
        case GB_CSCF_M_20:
            strcpy(name,"gb_cscf_m_20");
            return 1;
        case GB_CSCF_M_21:
            strcpy(name,"gb_cscf_m_21");
            return 1;
        case RESERVED_CSCF_20_21:
            strcpy(name,"reserved_cscf_20_21");
            return 1;
        case GB_CSCF_M_22:
            strcpy(name,"gb_cscf_m_22");
            return 1;
        case RESERVED_CSCF_22:
            strcpy(name,"reserved_cscf_22");
            return 1;
        case GB_CSCF_OFF_00:
            strcpy(name,"gb_cscf_off_00");
            return 1;
        case RESERVED_CSCF_OFF_00:
            strcpy(name,"reserved_cscf_off_00");
            return 1;
        case GB_CSCF_OFF_01:
            strcpy(name,"gb_cscf_off_01");
            return 1;
        case RESERVED_CSCF_OFF_01:
            strcpy(name,"reserved_cscf_off_01");
            return 1;
        case GB_CSCF_OFF_02:
            strcpy(name,"gb_cscf_off_02");
            return 1;
        case RESERVED_CSCF_OFF_02:
            strcpy(name,"reserved_cscf_off_02");
            return 1;
        case GB_CSCB_M_00:
            strcpy(name,"gb_cscb_m_00");
            return 1;
        case GB_CSCB_M_01:
            strcpy(name,"gb_cscb_m_01");
            return 1;
        case GB_CSCB_EN:
            strcpy(name,"gb_cscb_en");
            return 1;
        case GB_CSCB_ROUND:
            strcpy(name,"gb_cscb_round");
            return 1;
        case RESERVED_CSCB_00_01:
            strcpy(name,"reserved_cscb_00_01");
            return 1;
        case GB_CSCB_M_02:
            strcpy(name,"gb_cscb_m_02");
            return 1;
        case GB_CSCB_M_10:
            strcpy(name,"gb_cscb_m_10");
            return 1;
        case RESERVED_CSCB_02_10:
            strcpy(name,"reserved_cscb_02_10");
            return 1;
        case GB_CSCB_M_11:
            strcpy(name,"gb_cscb_m_11");
            return 1;
        case GB_CSCB_M_12:
            strcpy(name,"gb_cscb_m_12");
            return 1;
        case RESERVED_CSCB_11_12:
            strcpy(name,"reserved_cscb_11_12");
            return 1;
        case GB_CSCB_M_20:
            strcpy(name,"gb_cscb_m_20");
            return 1;
        case GB_CSCB_M_21:
            strcpy(name,"gb_cscb_m_21");
            return 1;
        case RESERVED_CSCB_20_21:
            strcpy(name,"reserved_cscb_20_21");
            return 1;
        case GB_CSCB_M_22:
            strcpy(name,"gb_cscb_m_22");
            return 1;
        case RESERVED_CSCB_22:
            strcpy(name,"reserved_cscb_22");
            return 1;
        case GB_CSCB_OFF_00:
            strcpy(name,"gb_cscb_off_00");
            return 1;
        case RESERVED_CSCB_OFF_00:
            strcpy(name,"reserved_cscb_off_00");
            return 1;
        case GB_CSCB_OFF_01:
            strcpy(name,"gb_cscb_off_01");
            return 1;
        case RESERVED_CSCB_OFF_01:
            strcpy(name,"reserved_cscb_off_01");
            return 1;
        case GB_CSCB_OFF_02:
            strcpy(name,"gb_cscb_off_02");
            return 1;
        case RESERVED_CSCB_OFF_02:
            strcpy(name,"reserved_cscb_off_02");
            return 1;
        case GB_FIXCOLOR_VALUE:
            strcpy(name,"gb_fixcolor_value");
            return 1;
        case GB_LB_CPT_OFFSET:
            strcpy(name,"gb_lb_cpt_offset");
            return 1;
        case GB_LB_CPT_SIZE:
            strcpy(name,"gb_lb_cpt_size");
            return 1;
        case GB_LB_MOB_L2T_PITCH:
            strcpy(name,"gb_lb_mob_l2t_pitch");
            return 1;
        case GB_DST_END:
            strcpy(name,"gb_dst_end");
            return 1;
        case GB_DST_TILE:
            strcpy(name,"gb_dst_tile");
            return 1;
        case GB_LB_MOB_L2T_OFFSET:
            strcpy(name,"gb_lb_mob_l2t_offset");
            return 1;
        case GB_MOB_DST_ALPHAMODE:
            strcpy(name,"gb_mob_dst_alphamode");
            return 1;
        case GB_MOB_BLENDING_MODE:
            strcpy(name,"gb_mob_blending_mode");
            return 1;
        case GB_MOB_BLENDING_METHOD:
            strcpy(name,"gb_mob_blending_method");
            return 1;
        case GB_MOB_BLENDING_ROUNDING:
            strcpy(name,"gb_mob_blending_rounding");
            return 1;
        case GB_CPT_DNSAMPLE_MODE:
            strcpy(name,"gb_cpt_dnsample_mode");
            return 1;
        case GB_MOB_BLDALPHA:
            strcpy(name,"gb_mob_bldalpha");
            return 1;
        case GB_CPT_EN:
            strcpy(name,"gb_cpt_en");
            return 1;
        case GB_DST_SF_WIDTH:
            strcpy(name,"gb_dst_sf_width");
            return 1;
        case RESERVED_CPT2:
            strcpy(name,"reserved_cpt2");
            return 1;
        case GB_MIFARB_REG:
            strcpy(name,"gb_mifarb_reg");
            return 1;
        case GB_MIFARB_YDIFF:
            strcpy(name,"gb_mifarb_ydiff");
            return 1;
        case GB_MIFARB_LD:
            strcpy(name,"gb_mifarb_ld");
            return 1;
        case RESERVED_MA0:
            strcpy(name,"reserved_ma0");
            return 1;
        case GB_MIFARB_CPTR:
            strcpy(name,"gb_mifarb_cptr");
            return 1;
        case GB_MIFARB_MTDR:
            strcpy(name,"gb_mifarb_mtdr");
            return 1;
        case GB_MIFARB_MOB:
            strcpy(name,"gb_mifarb_mob");
            return 1;
        case GB_MIFARB_PASSIVE:
            strcpy(name,"gb_mifarb_passive");
            return 1;
        case RESERVED_MA1:
            strcpy(name,"reserved_ma1");
            return 1;
        case GB_MIFARB_MTDW:
            strcpy(name,"gb_mifarb_mtdw");
            return 1;
        case GB_MIFARB_HDRR:
            strcpy(name,"gb_mifarb_hdrr");
            return 1;
        case GB_MIFARB_HDRW:
            strcpy(name,"gb_mifarb_hdrw");
            return 1;
        case RESERVED_MA2:
            strcpy(name,"reserved_ma2");
            return 1;
        case GB_SRC_BL_SLOT_IDX:
            strcpy(name,"gb_src_bl_slot_idx");
            return 1;
        case GB_OTH_BL_SLOT_IDX:
            strcpy(name,"gb_oth_bl_slot_idx");
            return 1;
        case GB_REF_BL_SLOT_IDX:
            strcpy(name,"gb_ref_bl_slot_idx");
            return 1;
        case GB_DST_BL_SLOT_IDX:
            strcpy(name,"gb_dst_bl_slot_idx");
            return 1;
        case GB_SCL_DST_BL_SLOT_IDX:
            strcpy(name,"gb_scl_dst_bl_slot_idx");
            return 1;
        case RESERVED_MSI2:
            strcpy(name,"reserved_msi2");
            return 1;
        case GB_SRC_BOT_BL_SLOT_IDX:
            strcpy(name,"gb_src_bot_bl_slot_idx");
            return 1;
        case GB_OTH_BOT_BL_SLOT_IDX:
            strcpy(name,"gb_oth_bot_bl_slot_idx");
            return 1;
        case GB_REF_BOT_BL_SLOT_IDX:
            strcpy(name,"gb_ref_bot_bl_slot_idx");
            return 1;
        case RESERVED_MSI4:
            strcpy(name,"reserved_msi4");
            return 1;
        case GB_SRC_PROC_ID:
            strcpy(name,"gb_src_proc_id");
            return 1;
        case GB_OTH_PROC_ID:
            strcpy(name,"gb_oth_proc_id");
            return 1;
        case GB_REF_PROC_ID:
            strcpy(name,"gb_ref_proc_id");
            return 1;
        case GB_SRC_BOT_PROC_ID:
            strcpy(name,"gb_src_bot_proc_id");
            return 1;
        case GB_OTH_BOT_PROC_ID:
            strcpy(name,"gb_oth_bot_proc_id");
            return 1;
        case GB_REF_BOT_PROC_ID:
            strcpy(name,"gb_ref_bot_proc_id");
            return 1;
        case GB_DST_PROC_ID:
            strcpy(name,"gb_dst_proc_id");
            return 1;
        case GB_SCL_DST_PROC_ID:
            strcpy(name,"gb_scl_dst_proc_id");
            return 1;
        case GB_MTD_HIST_PROC_ID:
            strcpy(name,"gb_mtd_hist_proc_id");
            return 1;
        case GB_HDR_YSUM_PROC_ID:
            strcpy(name,"gb_hdr_ysum_proc_id");
            return 1;
        case GB_HDR_INT_PROC_ID:
            strcpy(name,"gb_hdr_int_proc_id");
            return 1;
        case RESERVED_MPID1:
            strcpy(name,"reserved_mpid1");
            return 1;
        case GB_DI_INTRA_TH:
            strcpy(name,"gb_di_intra_th");
            return 1;
        case GB_DI_ISFEATHER_TH:
            strcpy(name,"gb_di_isfeather_th");
            return 1;
        case GB_DI_FORCE_EELA_TH:
            strcpy(name,"gb_di_force_eela_th");
            return 1;
        case RESERVED_DI0:
            strcpy(name,"reserved_di0");
            return 1;
        case GB_DI_SCROLL_TH:
            strcpy(name,"gb_di_scroll_th");
            return 1;
        case GB_POINT_DEGREE_VAR_K:
            strcpy(name,"gb_point_degree_var_k");
            return 1;
        case RESERVED_DI1:
            strcpy(name,"reserved_di1");
            return 1;
        case GB_SOBEL_TH:
            strcpy(name,"gb_sobel_th");
            return 1;
        case GB_SOBEL_FORCE_VALUE:
            strcpy(name,"gb_sobel_force_value");
            return 1;
        case GB_DI_FEATHER_EDGE_CNT_X1:
            strcpy(name,"gb_di_feather_edge_cnt_x1");
            return 1;
        case GB_DI_FEATHER_EDGE_CNT_Y1:
            strcpy(name,"gb_di_feather_edge_cnt_y1");
            return 1;
        case RESERVED_DI2:
            strcpy(name,"reserved_di2");
            return 1;
        case GB_DI_IAD_MD_X1:
            strcpy(name,"gb_di_iad_md_x1");
            return 1;
        case GB_DI_IAD_MD_X2:
            strcpy(name,"gb_di_iad_md_x2");
            return 1;
        case GB_DI_IAD_MD_Y1:
            strcpy(name,"gb_di_iad_md_y1");
            return 1;
        case GB_DI_IAD_MD_K:
            strcpy(name,"gb_di_iad_md_k");
            return 1;
        case RESERVED_DI4:
            strcpy(name,"reserved_di4");
            return 1;
        case GB_POINT_DEGREE_VAR_X1:
            strcpy(name,"gb_point_degree_var_x1");
            return 1;
        case GB_POINT_DEGREE_VAR_X2:
            strcpy(name,"gb_point_degree_var_x2");
            return 1;
        case GB_POINT_DEGREE_VAR_Y1:
            strcpy(name,"gb_point_degree_var_y1");
            return 1;
        case GB_TOP_BOT_FIELD_DIV:
            strcpy(name,"gb_top_bot_field_div");
            return 1;
        case GB_DI_ENABLE_STATICFLAG:
            strcpy(name,"gb_di_enable_staticflag");
            return 1;
        case GB_DI_FEATHER_EDGE_CNT_K:
            strcpy(name,"gb_di_feather_edge_cnt_k");
            return 1;
        case GB_DI_FEATHER_EDGE_CNT_X2:
            strcpy(name,"gb_di_feather_edge_cnt_x2");
            return 1;
        case GB_FD_WAIT_FRAME:
            strcpy(name,"gb_fd_wait_frame");
            return 1;
        case RESERVED_DI6:
            strcpy(name,"reserved_di6");
            return 1;
        case GB_EDGE_FLAG_TH:
            strcpy(name,"gb_edge_flag_th");
            return 1;
        case GB_DI_INTRA_DIFF_TH:
            strcpy(name,"gb_di_intra_diff_th");
            return 1;
        case RESERVED_DI7:
            strcpy(name,"reserved_di7");
            return 1;
        case GB_USE_MV_JUDGE:
            strcpy(name,"gb_use_mv_judge");
            return 1;
        case GB_USE_ALLMV:
            strcpy(name,"gb_use_allmv");
            return 1;
        case GB_MV_DIFF_TH:
            strcpy(name,"gb_mv_diff_th");
            return 1;
        case GB_DIRECTION_JUDGE_TH:
            strcpy(name,"gb_direction_judge_th");
            return 1;
        case GB_FD_BAD_MERGE_TH_1:
            strcpy(name,"gb_fd_bad_merge_th_1");
            return 1;
        case GB_FD_BAD_MERGE_TH_2:
            strcpy(name,"gb_fd_bad_merge_th_2");
            return 1;
        case RESERVED_DI8:
            strcpy(name,"reserved_di8");
            return 1;
        case GB_DI_MD_AREA_TH0:
            strcpy(name,"gb_di_md_area_th0");
            return 1;
        case GB_DI_MD_AREA_TH1:
            strcpy(name,"gb_di_md_area_th1");
            return 1;
        case GB_DI_MD_AREA_COEF0:
            strcpy(name,"gb_di_md_area_coef0");
            return 1;
        case GB_DI_MD_AREA_COEF1:
            strcpy(name,"gb_di_md_area_coef1");
            return 1;
        case GB_DI_MD_ADJ_COEF0:
            strcpy(name,"gb_di_md_adj_coef0");
            return 1;
        case GB_DI_MD_ADJ_COEF1:
            strcpy(name,"gb_di_md_adj_coef1");
            return 1;
        case RESERVED_DIA:
            strcpy(name,"reserved_dia");
            return 1;
        case GB_DI_IAD_AVE_ADJ_TH0:
            strcpy(name,"gb_di_iad_ave_adj_th0");
            return 1;
        case GB_DI_IAD_AVE_ADJ_TH1:
            strcpy(name,"gb_di_iad_ave_adj_th1");
            return 1;
        case RESERVED_DIB:
            strcpy(name,"reserved_dib");
            return 1;
        case GB_DI_IAD_AVE_ADJ_VAL:
            strcpy(name,"gb_di_iad_ave_adj_val");
            return 1;
        case GB_DI_IAD_AVE_ADJ_COEF:
            strcpy(name,"gb_di_iad_ave_adj_coef");
            return 1;
        case GB_DI_MTD_FEATHERDETECT_EN:
            strcpy(name,"gb_di_mtd_featherdetect_en");
            return 1;
        case RESERVED_DIC:
            strcpy(name,"reserved_dic");
            return 1;
        case GB_DI_IAD_LINE_MD_X1:
            strcpy(name,"gb_di_iad_line_md_x1");
            return 1;
        case GB_DI_IAD_LINE_MD_X2:
            strcpy(name,"gb_di_iad_line_md_x2");
            return 1;
        case GB_DI_MTD_FEATHERPARA:
            strcpy(name,"gb_di_mtd_featherpara");
            return 1;
        case GB_DI_IAD_LINE_MD_Y1:
            strcpy(name,"gb_di_iad_line_md_y1");
            return 1;
        case GB_DI_IAD_LINE_MD_K:
            strcpy(name,"gb_di_iad_line_md_k");
            return 1;
        case GB_DI_TH:
            strcpy(name,"gb_di_th");
            return 1;
        case RESERVED_VC01:
            strcpy(name,"reserved_vc01");
            return 1;
        case GB_MD_TH2:
            strcpy(name,"gb_md_th2");
            return 1;
        case GB_MD_TH2_EN:
            strcpy(name,"gb_md_th2_en");
            return 1;
        case RESERVED_VC0:
            strcpy(name,"reserved_vc0");
            return 1;
        case GB_MD_TH:
            strcpy(name,"gb_md_th");
            return 1;
        case GB_DI_SR_REV_DIFF_TH:
            strcpy(name,"gb_di_sr_rev_diff_th");
            return 1;
        case RESERVED_VC3:
            strcpy(name,"reserved_vc3");
            return 1;
        case GB_DI_MTD_EDGEVALTHR:
            strcpy(name,"gb_di_mtd_edgevalthr");
            return 1;
        case GB_DI_MTD_STATICPARA:
            strcpy(name,"gb_di_mtd_staticpara");
            return 1;
        case GB_DI_MTD_MAXMINTHR:
            strcpy(name,"gb_di_mtd_maxminthr");
            return 1;
        case GB_DI_SR_GRAD_TH:
            strcpy(name,"gb_di_sr_grad_th");
            return 1;
        case GB_DI_MTD_MD_TH:
            strcpy(name,"gb_di_mtd_md_th");
            return 1;
        case RESERVED_VC4:
            strcpy(name,"reserved_vc4");
            return 1;
        case GB_DI_SR_LINE_DIFF_TH:
            strcpy(name,"gb_di_sr_line_diff_th");
            return 1;
        case GB_DI_IAD_MD_AVE_TH:
            strcpy(name,"gb_di_iad_md_ave_th");
            return 1;
        case RESERVED_VC12:
            strcpy(name,"reserved_vc12");
            return 1;
        case GB_MTD_COL_HIST_BASE:
            strcpy(name,"gb_mtd_col_hist_base");
            return 1;
        case GB_MD_LOW_TH:
            strcpy(name,"gb_md_low_th");
            return 1;
        case RESERVED_VC9:
            strcpy(name,"reserved_vc9");
            return 1;
        case GB_LBUF_SIZE:
            strcpy(name,"gb_lbuf_size");
            return 1;
        case GB_VPP_IN_SIZE:
            strcpy(name,"gb_vpp_in_size");
            return 1;
        case GB_DISABLE_LCID_BLEND:
            strcpy(name,"gb_disable_lcid_blend");
            return 1;
        case GB_FD_FORCE_DI:
            strcpy(name,"gb_fd_force_di");
            return 1;
        case GB_DI_BLEND_COEF:
            strcpy(name,"gb_di_blend_coef");
            return 1;
        case RESERVED_VL0:
            strcpy(name,"reserved_vl0");
            return 1;
        case GB_VPP_DI_OFFSET:
            strcpy(name,"gb_vpp_di_offset");
            return 1;
        case GB_VPP_DI_SIZE:
            strcpy(name,"gb_vpp_di_size");
            return 1;
        case GB_VPP_DIREF_IN_OFFSET:
            strcpy(name,"gb_vpp_diref_in_offset");
            return 1;
        case RESERVED_VL1:
            strcpy(name,"reserved_vl1");
            return 1;
        case GB_FD_MD_TH:
            strcpy(name,"gb_fd_md_th");
            return 1;
        case GB_SCENE_CHANGE_TOP:
            strcpy(name,"gb_scene_change_top");
            return 1;
        case GB_SCENE_CHANGE_BOTTOM:
            strcpy(name,"gb_scene_change_bottom");
            return 1;
        case GB_2V2_TH_DELTA:
            strcpy(name,"gb_2v2_th_delta");
            return 1;
        case GB_2V2_TH_RATIO:
            strcpy(name,"gb_2v2_th_ratio");
            return 1;
        case GB_2V2_SAD_COUNTER_TH:
            strcpy(name,"gb_2v2_sad_counter_th");
            return 1;
        case GB_2V2_MD_FAILED_COUNTER_TH:
            strcpy(name,"gb_2v2_md_failed_counter_th");
            return 1;
        case GB_LCID_DIFF_TH:
            strcpy(name,"gb_lcid_diff_th");
            return 1;
        case GB_2V2_COUNT_MODE:
            strcpy(name,"gb_2v2_count_mode");
            return 1;
        case GB_SCENE_CHANGE_TH:
            strcpy(name,"gb_scene_change_th");
            return 1;
        case GB_SCENECHANGE_MD_MODE:
            strcpy(name,"gb_scenechange_md_mode");
            return 1;
        case GB_FD_MD_TH2:
            strcpy(name,"gb_fd_md_th2");
            return 1;
        case GB_FD_COUNTER_TH:
            strcpy(name,"gb_fd_counter_th");
            return 1;
        case GB_FD_DETECT_COUNTER_INITIAL:
            strcpy(name,"gb_fd_detect_counter_initial");
            return 1;
        case GB_FD_STRONG_COUNTER_INITIAL:
            strcpy(name,"gb_fd_strong_counter_initial");
            return 1;
        case GB_2V2_FAILED_COUNTER_TH:
            strcpy(name,"gb_2v2_failed_counter_th");
            return 1;
        case GB_2V2_MD_SUCCEED_COUNTER_TH:
            strcpy(name,"gb_2v2_md_succeed_counter_th");
            return 1;
        case GB_2V2_SAD_SUCCEED_COUNTER_TH:
            strcpy(name,"gb_2v2_sad_succeed_counter_th");
            return 1;
        case GB_2V2_SAD_FAILED_RANGE:
            strcpy(name,"gb_2v2_sad_failed_range");
            return 1;
        case GB_2V2_FRAME_CNT_TH:
            strcpy(name,"gb_2v2_frame_cnt_th");
            return 1;
        case GB_2V2_FAILED_COUNTER_TH2:
            strcpy(name,"gb_2v2_failed_counter_th2");
            return 1;
        case GB_FD_2V2_SAD_CLR_TH_1:
            strcpy(name,"gb_fd_2v2_sad_clr_th_1");
            return 1;
        case GB_FD_2V2_SAD_CLR_TH_2:
            strcpy(name,"gb_fd_2v2_sad_clr_th_2");
            return 1;
        case GB_FD_2V2_SAD_CLR_TH_3:
            strcpy(name,"gb_fd_2v2_sad_clr_th_3");
            return 1;
        case RESERVED_FD4:
            strcpy(name,"reserved_fd4");
            return 1;
        case GB_FD_BAD_COPY_TH_1:
            strcpy(name,"gb_fd_bad_copy_th_1");
            return 1;
        case GB_FD_BAD_COPY_TH_2:
            strcpy(name,"gb_fd_bad_copy_th_2");
            return 1;
        case GB_FD_BAD_COPY_TH_3:
            strcpy(name,"gb_fd_bad_copy_th_3");
            return 1;
        case GB_FD_BAD_COPY_TH_4:
            strcpy(name,"gb_fd_bad_copy_th_4");
            return 1;
        case GB_FD_QUIT_2V2_TH_1:
            strcpy(name,"gb_fd_quit_2v2_th_1");
            return 1;
        case RESERVED_FD5:
            strcpy(name,"reserved_fd5");
            return 1;
        case GB_FD_QUIT_2V2_TH_2:
            strcpy(name,"gb_fd_quit_2v2_th_2");
            return 1;
        case GB_FD_QUIT_2V2_TH_3:
            strcpy(name,"gb_fd_quit_2v2_th_3");
            return 1;
        case GB_FD_QUIT_2V2_TH_4:
            strcpy(name,"gb_fd_quit_2v2_th_4");
            return 1;
        case GB_FD_QUIT_2V2_TH_5:
            strcpy(name,"gb_fd_quit_2v2_th_5");
            return 1;
        case GB_FD_2V2_MD_TH_3:
            strcpy(name,"gb_fd_2v2_md_th_3");
            return 1;
        case GB_FD_2V2_MD_TH_5:
            strcpy(name,"gb_fd_2v2_md_th_5");
            return 1;
        case GB_FD_2V2_MD_TH_1:
            strcpy(name,"gb_fd_2v2_md_th_1");
            return 1;
        case GB_FD_2V2_MD_TH_2:
            strcpy(name,"gb_fd_2v2_md_th_2");
            return 1;
        case GB_FD_2V2_MD_TH_4:
            strcpy(name,"gb_fd_2v2_md_th_4");
            return 1;
        case GB_FD_SC_QUIT_2V2_TH:
            strcpy(name,"gb_fd_sc_quit_2v2_th");
            return 1;
        case GB_DI_CG_OFF:
            strcpy(name,"gb_di_cg_off");
            return 1;
        case GB_SCL_CG_OFF:
            strcpy(name,"gb_scl_cg_off");
            return 1;
        case GB_SCL_BP_CG_OFF:
            strcpy(name,"gb_scl_bp_cg_off");
            return 1;
        case GB_CSCB_CG_OFF:
            strcpy(name,"gb_cscb_cg_off");
            return 1;
        case GB_CSCF_CG_OFF:
            strcpy(name,"gb_cscf_cg_off");
            return 1;
        case GB_CG_BUSY_TH:
            strcpy(name,"gb_cg_busy_th");
            return 1;
        case GB_FIRE_AFTER_SAVE:
            strcpy(name,"gb_fire_after_save");
            return 1;
        case GB_RESTORE_KEEP_ON:
            strcpy(name,"gb_restore_keep_on");
            return 1;
        case GB_FOR_ECO2:
            strcpy(name,"gb_for_eco2");
            return 1;
        case GB_CPT_CG_OFF:
            strcpy(name,"gb_cpt_cg_off");
            return 1;
        case GB_SR_CG_OFF:
            strcpy(name,"gb_sr_cg_off");
            return 1;
        case GB_CM_CG_OFF:
            strcpy(name,"gb_cm_cg_off");
            return 1;
        case GB_CCM_CG_OFF:
            strcpy(name,"gb_ccm_cg_off");
            return 1;
        case GB_HDR_CG_OFF:
            strcpy(name,"gb_hdr_cg_off");
            return 1;
        case RESERVED_CG:
            strcpy(name,"reserved_cg");
            return 1;
        case GB_DI_EN:
            strcpy(name,"gb_di_en");
            return 1;
        case GB_FD_EN:
            strcpy(name,"gb_fd_en");
            return 1;
        case GB_MTD_EN:
            strcpy(name,"gb_mtd_en");
            return 1;
        case GB_TWO_FRAME_OUT_EN:
            strcpy(name,"gb_two_frame_out_en");
            return 1;
        case GB_DI_FORCE_LCID:
            strcpy(name,"gb_di_force_lcid");
            return 1;
        case GB_SINGLE_SEL_OTH:
            strcpy(name,"gb_single_sel_oth");
            return 1;
        case GB_TOP_FIELD_FIRST:
            strcpy(name,"gb_top_field_first");
            return 1;
        case GB_RESET_HQVPP:
            strcpy(name,"gb_reset_hqvpp");
            return 1;
        case GB_HQVPP_MODE:
            strcpy(name,"gb_hqvpp_mode");
            return 1;
        case GB_VPP_FD_INDEX:
            strcpy(name,"gb_vpp_fd_index");
            return 1;
        case RESERVED_OPCODE:
            strcpy(name,"reserved_opcode");
            return 1;
        case GB_CMLUT_EN:
            strcpy(name,"gb_cmlut_en");
            return 1;
        case GB_TINT_EN:
            strcpy(name,"gb_tint_en");
            return 1;
        case GB_TINT_HOFFSET:
            strcpy(name,"gb_tint_hoffset");
            return 1;
        case RESERVED_CM0:
            strcpy(name,"reserved_cm0");
            return 1;
        case GB_CMLUT0:
            strcpy(name,"gb_cmlut0");
            return 1;
        case GB_CMLUT1:
            strcpy(name,"gb_cmlut1");
            return 1;
        case RESERVED_CM1:
            strcpy(name,"reserved_cm1");
            return 1;
        case GB_CCM_EN:
            strcpy(name,"gb_ccm_en");
            return 1;
        case GB_METADATA_EN:
            strcpy(name,"gb_metadata_en");
            return 1;
        case GB_DG_HCOE:
            strcpy(name,"gb_dg_hcoe");
            return 1;
        case RESERVED_CCM0:
            strcpy(name,"reserved_ccm0");
            return 1;
        case GB_DG_LCOE:
            strcpy(name,"gb_dg_lcoe");
            return 1;
        case GB_YUV2RGB_EN:
            strcpy(name,"gb_yuv2rgb_en");
            return 1;
        case GB_YUV2RGB_MODE:
            strcpy(name,"gb_yuv2rgb_mode");
            return 1;
        case GB_CCM_HDR_INV:
            strcpy(name,"gb_ccm_hdr_inv");
            return 1;
        case RESERVED_CCM1:
            strcpy(name,"reserved_ccm1");
            return 1;
        case GB_DG_TBL_0:
            strcpy(name,"gb_dg_tbl_0");
            return 1;
        case GB_DG_TBL_1:
            strcpy(name,"gb_dg_tbl_1");
            return 1;
        case GB_G_TBL_0:
            strcpy(name,"gb_g_tbl_0");
            return 1;
        case GB_G_TBL_1:
            strcpy(name,"gb_g_tbl_1");
            return 1;
        case GB_G_TBL_2:
            strcpy(name,"gb_g_tbl_2");
            return 1;
        case RESERVED_CCM5:
            strcpy(name,"reserved_ccm5");
            return 1;
        case GB_HLG_EN:
            strcpy(name,"gb_hlg_en");
            return 1;
        case GB_HLG_ACOEF:
            strcpy(name,"gb_hlg_acoef");
            return 1;
        case GB_HLG_BCOEF:
            strcpy(name,"gb_hlg_bcoef");
            return 1;
        case RESERVED_CCM6:
            strcpy(name,"reserved_ccm6");
            return 1;
        case GB_HLG_TBL_0:
            strcpy(name,"gb_hlg_tbl_0");
            return 1;
        case GB_HLG_TBL_1:
            strcpy(name,"gb_hlg_tbl_1");
            return 1;
        case GB_HLG_TBL_20:
            strcpy(name,"gb_hlg_tbl_20");
            return 1;
        case GB_HLG_TBL_21:
            strcpy(name,"gb_hlg_tbl_21");
            return 1;
        case GB_HLG_TBL_22:
            strcpy(name,"gb_hlg_tbl_22");
            return 1;
        case GB_HLG_INVTBL_22:
            strcpy(name,"gb_hlg_invtbl_22");
            return 1;
        case GB_1886_ACOEF:
            strcpy(name,"gb_1886_acoef");
            return 1;
        case GB_1886_BCOEF:
            strcpy(name,"gb_1886_bcoef");
            return 1;
        case GB_HLG_INVACOEF:
            strcpy(name,"gb_hlg_invacoef");
            return 1;
        case GB_HLG_INVBCOEF:
            strcpy(name,"gb_hlg_invbcoef");
            return 1;
        case GB_1886_EN:
            strcpy(name,"gb_1886_en");
            return 1;
        case RESERVED_CCM9:
            strcpy(name,"reserved_ccm9");
            return 1;
        case GB_HLG_INVTBL_0:
            strcpy(name,"gb_hlg_invtbl_0");
            return 1;
        case GB_HLG_INVTBL_1:
            strcpy(name,"gb_hlg_invtbl_1");
            return 1;
        case GB_HLG_INVTBL_20:
            strcpy(name,"gb_hlg_invtbl_20");
            return 1;
        case GB_HLG_INVTBL_21:
            strcpy(name,"gb_hlg_invtbl_21");
            return 1;
        case GB_1886_INVACOEF:
            strcpy(name,"gb_1886_invacoef");
            return 1;
        case GB_1886_INVBCOEF:
            strcpy(name,"gb_1886_invbcoef");
            return 1;
        case GB_CONVERT_M_00:
            strcpy(name,"gb_convert_m_00");
            return 1;
        case GB_CONVERT_M_01:
            strcpy(name,"gb_convert_m_01");
            return 1;
        case RESERVED_CC0:
            strcpy(name,"reserved_cc0");
            return 1;
        case GB_CONVERT_M_10:
            strcpy(name,"gb_convert_m_10");
            return 1;
        case GB_CONVERT_M_11:
            strcpy(name,"gb_convert_m_11");
            return 1;
        case RESERVED_CC1:
            strcpy(name,"reserved_cc1");
            return 1;
        case GB_CONVERT_M_20:
            strcpy(name,"gb_convert_m_20");
            return 1;
        case GB_CONVERT_M_21:
            strcpy(name,"gb_convert_m_21");
            return 1;
        case RESERVED_CC2:
            strcpy(name,"reserved_cc2");
            return 1;
        case GB_CONVERT_M_02:
            strcpy(name,"gb_convert_m_02");
            return 1;
        case GB_CONVERT_M_12:
            strcpy(name,"gb_convert_m_12");
            return 1;
        case RESERVED_CC3:
            strcpy(name,"reserved_cc3");
            return 1;
        case GB_CONVERT_M_22:
            strcpy(name,"gb_convert_m_22");
            return 1;
        case RESERVED_CC4:
            strcpy(name,"reserved_cc4");
            return 1;
        case GB_HADJ_EN:
            strcpy(name,"gb_hadj_en");
            return 1;
        case GB_HRANGE_GL:
            strcpy(name,"gb_hrange_gl");
            return 1;
        case GB_HRANGE_GH:
            strcpy(name,"gb_hrange_gh");
            return 1;
        case RESERVED_HADJ0:
            strcpy(name,"reserved_hadj0");
            return 1;
        case GB_HRANGE_RL:
            strcpy(name,"gb_hrange_rl");
            return 1;
        case GB_HRANGE_RH:
            strcpy(name,"gb_hrange_rh");
            return 1;
        case RESERVED_HADJ1:
            strcpy(name,"reserved_hadj1");
            return 1;
        case GB_G_RANGE:
            strcpy(name,"gb_g_range");
            return 1;
        case GB_G_AXIS:
            strcpy(name,"gb_g_axis");
            return 1;
        case GB_G_COEF:
            strcpy(name,"gb_g_coef");
            return 1;
        case GB_R_RANGE:
            strcpy(name,"gb_r_range");
            return 1;
        case GB_R_AXIS:
            strcpy(name,"gb_r_axis");
            return 1;
        case GB_R_COEF:
            strcpy(name,"gb_r_coef");
            return 1;
        case GB_SADJ_EN:
            strcpy(name,"gb_sadj_en");
            return 1;
        case GB_SADJ_INC00:
            strcpy(name,"gb_sadj_inc00");
            return 1;
        case GB_SADJ_INC01:
            strcpy(name,"gb_sadj_inc01");
            return 1;
        case RESERVED_SADJ0:
            strcpy(name,"reserved_sadj0");
            return 1;
        case GB_SADJ_INC02:
            strcpy(name,"gb_sadj_inc02");
            return 1;
        case GB_SADJ_INC03:
            strcpy(name,"gb_sadj_inc03");
            return 1;
        case RESERVED_SADJ1:
            strcpy(name,"reserved_sadj1");
            return 1;
        case GB_SADJ_INC04:
            strcpy(name,"gb_sadj_inc04");
            return 1;
        case GB_SADJ_INC05:
            strcpy(name,"gb_sadj_inc05");
            return 1;
        case RESERVED_SADJ2:
            strcpy(name,"reserved_sadj2");
            return 1;
        case GB_SADJ_INC06:
            strcpy(name,"gb_sadj_inc06");
            return 1;
        case GB_SADJ_INC07:
            strcpy(name,"gb_sadj_inc07");
            return 1;
        case GB_SADJ_INC14:
            strcpy(name,"gb_sadj_inc14");
            return 1;
        case RESERVED_SADJ3:
            strcpy(name,"reserved_sadj3");
            return 1;
        case GB_SADJ_INC08:
            strcpy(name,"gb_sadj_inc08");
            return 1;
        case GB_SADJ_INC09:
            strcpy(name,"gb_sadj_inc09");
            return 1;
        case RESERVED_SADJ4:
            strcpy(name,"reserved_sadj4");
            return 1;
        case GB_SADJ_INC0A:
            strcpy(name,"gb_sadj_inc0a");
            return 1;
        case GB_SADJ_INC0B:
            strcpy(name,"gb_sadj_inc0b");
            return 1;
        case GB_SADJ_INC13:
            strcpy(name,"gb_sadj_inc13");
            return 1;
        case RESERVED_SADJ5:
            strcpy(name,"reserved_sadj5");
            return 1;
        case GB_SADJ_INC0C:
            strcpy(name,"gb_sadj_inc0c");
            return 1;
        case GB_SADJ_INC0D:
            strcpy(name,"gb_sadj_inc0d");
            return 1;
        case RESERVED_SADJ6:
            strcpy(name,"reserved_sadj6");
            return 1;
        case GB_SADJ_INC0E:
            strcpy(name,"gb_sadj_inc0e");
            return 1;
        case GB_SADJ_INC0F:
            strcpy(name,"gb_sadj_inc0f");
            return 1;
        case GB_SADJ_INC12:
            strcpy(name,"gb_sadj_inc12");
            return 1;
        case RESERVED_SADJ7:
            strcpy(name,"reserved_sadj7");
            return 1;
        case GB_SADJ_INC10:
            strcpy(name,"gb_sadj_inc10");
            return 1;
        case GB_SADJ_INC11:
            strcpy(name,"gb_sadj_inc11");
            return 1;
        case RESERVED_SADJ8:
            strcpy(name,"reserved_sadj8");
            return 1;
        case GB_HDR_EN:
            strcpy(name,"gb_hdr_en");
            return 1;
        case GB_HDR_CURVE_EN:
            strcpy(name,"gb_hdr_curve_en");
            return 1;
        case GB_HDR_CLIPLMT:
            strcpy(name,"gb_hdr_cliplmt");
            return 1;
        case GB_HDR_MAXFMEAN:
            strcpy(name,"gb_hdr_maxfmean");
            return 1;
        case GB_HDR_MAXMEAN:
            strcpy(name,"gb_hdr_maxmean");
            return 1;
        case GB_HDR_SC_EN:
            strcpy(name,"gb_hdr_sc_en");
            return 1;
        case GB_HDR_SMALLSIZE:
            strcpy(name,"gb_hdr_smallsize");
            return 1;
        case GB_HDR_MAXPEAKCNT:
            strcpy(name,"gb_hdr_maxpeakcnt");
            return 1;
        case GB_HDR_MINPEAKWG:
            strcpy(name,"gb_hdr_minpeakwg");
            return 1;
        case GB_HDR_MAXPEAKWG:
            strcpy(name,"gb_hdr_maxpeakwg");
            return 1;
        case GB_HDR_MANUALCLIPMODE:
            strcpy(name,"gb_hdr_manualclipmode");
            return 1;
        case GB_HDR_FRMWG:
            strcpy(name,"gb_hdr_frmwg");
            return 1;
        case GB_RESET_HDR:
            strcpy(name,"gb_reset_hdr");
            return 1;
        case RESERVED_HDR1:
            strcpy(name,"reserved_hdr1");
            return 1;
        case GB_HDR_PIXCNTFORSTAT:
            strcpy(name,"gb_hdr_pixcntforstat");
            return 1;
        case GB_HDR_SHFT:
            strcpy(name,"gb_hdr_shft");
            return 1;
        case GB_HDR_MINSTRETCHRANGE:
            strcpy(name,"gb_hdr_minstretchrange");
            return 1;
        case RESERVED_HDR3:
            strcpy(name,"reserved_hdr3");
            return 1;
        case GB_HDR_MINFILTSPD:
            strcpy(name,"gb_hdr_minfiltspd");
            return 1;
        case GB_HDR_BINMEDSDT:
            strcpy(name,"gb_hdr_binmedsdt");
            return 1;
        case GB_HDR_DELTAMEANTH:
            strcpy(name,"gb_hdr_deltameanth");
            return 1;
        case GB_HDR_BINMEDHIMAX:
            strcpy(name,"gb_hdr_binmedhimax");
            return 1;
        case GB_HDR_YLUTHIWGINIT:
            strcpy(name,"gb_hdr_yluthiwginit");
            return 1;
        case RESERVED_HDR4:
            strcpy(name,"reserved_hdr4");
            return 1;
        case GB_HDR_MEANCHANGETH:
            strcpy(name,"gb_hdr_meanchangeth");
            return 1;
        case RESERVED_HDR5:
            strcpy(name,"reserved_hdr5");
            return 1;
        case GB_HDR_PIXLOWRATIO:
            strcpy(name,"gb_hdr_pixlowratio");
            return 1;
        case RESERVED_HDR6:
            strcpy(name,"reserved_hdr6");
            return 1;
        case GB_HDR_PIXHIRATIO:
            strcpy(name,"gb_hdr_pixhiratio");
            return 1;
        case RESERVED_HDR7:
            strcpy(name,"reserved_hdr7");
            return 1;
        case GB_HDR_BLACKRATIO:
            strcpy(name,"gb_hdr_blackratio");
            return 1;
        case RESERVED_HDR8:
            strcpy(name,"reserved_hdr8");
            return 1;
        case GB_HDR_NOISERATIO:
            strcpy(name,"gb_hdr_noiseratio");
            return 1;
        case RESERVED_HDR9:
            strcpy(name,"reserved_hdr9");
            return 1;
        case GB_HDR_YLUTHI_00:
            strcpy(name,"gb_hdr_yluthi_00");
            return 1;
        case GB_HDR_YLUTHI_01:
            strcpy(name,"gb_hdr_yluthi_01");
            return 1;
        case GB_HDR_YLUTHI_02:
            strcpy(name,"gb_hdr_yluthi_02");
            return 1;
        case RESERVED_HDR_YLUT0:
            strcpy(name,"reserved_hdr_ylut0");
            return 1;
        case GB_HDR_YLUTHI_03:
            strcpy(name,"gb_hdr_yluthi_03");
            return 1;
        case GB_HDR_YLUTHI_04:
            strcpy(name,"gb_hdr_yluthi_04");
            return 1;
        case GB_HDR_YLUTHI_05:
            strcpy(name,"gb_hdr_yluthi_05");
            return 1;
        case RESERVED_HDR_YLUT1:
            strcpy(name,"reserved_hdr_ylut1");
            return 1;
        case GB_HDR_YLUTHI_06:
            strcpy(name,"gb_hdr_yluthi_06");
            return 1;
        case GB_HDR_YLUTHI_07:
            strcpy(name,"gb_hdr_yluthi_07");
            return 1;
        case GB_HDR_YLUTHI_08:
            strcpy(name,"gb_hdr_yluthi_08");
            return 1;
        case RESERVED_HDR_YLUT2:
            strcpy(name,"reserved_hdr_ylut2");
            return 1;
        case GB_HDR_YLUTHI_09:
            strcpy(name,"gb_hdr_yluthi_09");
            return 1;
        case GB_HDR_YLUTHI_0A:
            strcpy(name,"gb_hdr_yluthi_0a");
            return 1;
        case GB_HDR_YLUTHI_0B:
            strcpy(name,"gb_hdr_yluthi_0b");
            return 1;
        case RESERVED_HDR_YLUT3:
            strcpy(name,"reserved_hdr_ylut3");
            return 1;
        case GB_HDR_YLUTHI_0C:
            strcpy(name,"gb_hdr_yluthi_0c");
            return 1;
        case GB_HDR_YLUTHI_0D:
            strcpy(name,"gb_hdr_yluthi_0d");
            return 1;
        case GB_HDR_YLUTHI_0E:
            strcpy(name,"gb_hdr_yluthi_0e");
            return 1;
        case RESERVED_HDR_YLUT4:
            strcpy(name,"reserved_hdr_ylut4");
            return 1;
        case GB_HDR_YLUTHI_0F:
            strcpy(name,"gb_hdr_yluthi_0f");
            return 1;
        case GB_HDR_YLUTHI_10:
            strcpy(name,"gb_hdr_yluthi_10");
            return 1;
        case GB_HDR_YLUTHI_11:
            strcpy(name,"gb_hdr_yluthi_11");
            return 1;
        case RESERVED_HDR_YLUT5:
            strcpy(name,"reserved_hdr_ylut5");
            return 1;
        case GB_HDR_YLUTHI_12:
            strcpy(name,"gb_hdr_yluthi_12");
            return 1;
        case GB_HDR_YLUTHI_13:
            strcpy(name,"gb_hdr_yluthi_13");
            return 1;
        case GB_HDR_YLUTHI_14:
            strcpy(name,"gb_hdr_yluthi_14");
            return 1;
        case RESERVED_HDR_YLUT6:
            strcpy(name,"reserved_hdr_ylut6");
            return 1;
        case GB_HDR_YLUTHI_15:
            strcpy(name,"gb_hdr_yluthi_15");
            return 1;
        case GB_HDR_YLUTHI_16:
            strcpy(name,"gb_hdr_yluthi_16");
            return 1;
        case GB_HDR_YLUTHI_17:
            strcpy(name,"gb_hdr_yluthi_17");
            return 1;
        case RESERVED_HDR_YLUT7:
            strcpy(name,"reserved_hdr_ylut7");
            return 1;
        case GB_HDR_YLUTHI_18:
            strcpy(name,"gb_hdr_yluthi_18");
            return 1;
        case GB_HDR_YLUTHI_19:
            strcpy(name,"gb_hdr_yluthi_19");
            return 1;
        case GB_HDR_YLUTHI_1A:
            strcpy(name,"gb_hdr_yluthi_1a");
            return 1;
        case RESERVED_HDR_YLUT8:
            strcpy(name,"reserved_hdr_ylut8");
            return 1;
        case GB_HDR_YLUTHI_1B:
            strcpy(name,"gb_hdr_yluthi_1b");
            return 1;
        case GB_HDR_YLUTHI_1C:
            strcpy(name,"gb_hdr_yluthi_1c");
            return 1;
        case GB_HDR_YLUTHI_1D:
            strcpy(name,"gb_hdr_yluthi_1d");
            return 1;
        case RESERVED_HDR_YLUT9:
            strcpy(name,"reserved_hdr_ylut9");
            return 1;
        case GB_HDR_YLUTHI_1E:
            strcpy(name,"gb_hdr_yluthi_1e");
            return 1;
        case GB_HDR_YLUTHI_1F:
            strcpy(name,"gb_hdr_yluthi_1f");
            return 1;
        case RESERVED_HDR_YLUTA:
            strcpy(name,"reserved_hdr_yluta");
            return 1;
        case GB_HDR_YLUTLOW_00:
            strcpy(name,"gb_hdr_ylutlow_00");
            return 1;
        case GB_HDR_YLUTLOW_01:
            strcpy(name,"gb_hdr_ylutlow_01");
            return 1;
        case GB_HDR_YLUTLOW_02:
            strcpy(name,"gb_hdr_ylutlow_02");
            return 1;
        case RESERVED_HDR_YLUT10:
            strcpy(name,"reserved_hdr_ylut10");
            return 1;
        case GB_HDR_YLUTLOW_03:
            strcpy(name,"gb_hdr_ylutlow_03");
            return 1;
        case GB_HDR_YLUTLOW_04:
            strcpy(name,"gb_hdr_ylutlow_04");
            return 1;
        case GB_HDR_YLUTLOW_05:
            strcpy(name,"gb_hdr_ylutlow_05");
            return 1;
        case RESERVED_HDR_YLUT11:
            strcpy(name,"reserved_hdr_ylut11");
            return 1;
        case GB_HDR_YLUTLOW_06:
            strcpy(name,"gb_hdr_ylutlow_06");
            return 1;
        case GB_HDR_YLUTLOW_07:
            strcpy(name,"gb_hdr_ylutlow_07");
            return 1;
        case GB_HDR_YLUTLOW_08:
            strcpy(name,"gb_hdr_ylutlow_08");
            return 1;
        case RESERVED_HDR_YLUT12:
            strcpy(name,"reserved_hdr_ylut12");
            return 1;
        case GB_HDR_YLUTLOW_09:
            strcpy(name,"gb_hdr_ylutlow_09");
            return 1;
        case GB_HDR_YLUTLOW_0A:
            strcpy(name,"gb_hdr_ylutlow_0a");
            return 1;
        case GB_HDR_YLUTLOW_0B:
            strcpy(name,"gb_hdr_ylutlow_0b");
            return 1;
        case RESERVED_HDR_YLUT13:
            strcpy(name,"reserved_hdr_ylut13");
            return 1;
        case GB_HDR_YLUTLOW_0C:
            strcpy(name,"gb_hdr_ylutlow_0c");
            return 1;
        case GB_HDR_YLUTLOW_0D:
            strcpy(name,"gb_hdr_ylutlow_0d");
            return 1;
        case GB_HDR_YLUTLOW_0E:
            strcpy(name,"gb_hdr_ylutlow_0e");
            return 1;
        case RESERVED_HDR_YLUT14:
            strcpy(name,"reserved_hdr_ylut14");
            return 1;
        case GB_HDR_YLUTLOW_0F:
            strcpy(name,"gb_hdr_ylutlow_0f");
            return 1;
        case GB_HDR_YLUTLOW_10:
            strcpy(name,"gb_hdr_ylutlow_10");
            return 1;
        case GB_HDR_YLUTLOW_11:
            strcpy(name,"gb_hdr_ylutlow_11");
            return 1;
        case RESERVED_HDR_YLUT15:
            strcpy(name,"reserved_hdr_ylut15");
            return 1;
        case GB_HDR_YLUTLOW_12:
            strcpy(name,"gb_hdr_ylutlow_12");
            return 1;
        case GB_HDR_YLUTLOW_13:
            strcpy(name,"gb_hdr_ylutlow_13");
            return 1;
        case GB_HDR_YLUTLOW_14:
            strcpy(name,"gb_hdr_ylutlow_14");
            return 1;
        case RESERVED_HDR_YLUT16:
            strcpy(name,"reserved_hdr_ylut16");
            return 1;
        case GB_HDR_YLUTLOW_15:
            strcpy(name,"gb_hdr_ylutlow_15");
            return 1;
        case GB_HDR_YLUTLOW_16:
            strcpy(name,"gb_hdr_ylutlow_16");
            return 1;
        case GB_HDR_YLUTLOW_17:
            strcpy(name,"gb_hdr_ylutlow_17");
            return 1;
        case RESERVED_HDR_YLUT17:
            strcpy(name,"reserved_hdr_ylut17");
            return 1;
        case GB_HDR_YLUTLOW_18:
            strcpy(name,"gb_hdr_ylutlow_18");
            return 1;
        case GB_HDR_YLUTLOW_19:
            strcpy(name,"gb_hdr_ylutlow_19");
            return 1;
        case GB_HDR_YLUTLOW_1A:
            strcpy(name,"gb_hdr_ylutlow_1a");
            return 1;
        case RESERVED_HDR_YLUT18:
            strcpy(name,"reserved_hdr_ylut18");
            return 1;
        case GB_HDR_YLUTLOW_1B:
            strcpy(name,"gb_hdr_ylutlow_1b");
            return 1;
        case GB_HDR_YLUTLOW_1C:
            strcpy(name,"gb_hdr_ylutlow_1c");
            return 1;
        case GB_HDR_YLUTLOW_1D:
            strcpy(name,"gb_hdr_ylutlow_1d");
            return 1;
        case RESERVED_HDR_YLUT19:
            strcpy(name,"reserved_hdr_ylut19");
            return 1;
        case GB_HDR_YLUTLOW_1E:
            strcpy(name,"gb_hdr_ylutlow_1e");
            return 1;
        case GB_HDR_YLUTLOW_1F:
            strcpy(name,"gb_hdr_ylutlow_1f");
            return 1;
        case RESERVED_HDR_YLUT1A:
            strcpy(name,"reserved_hdr_ylut1a");
            return 1;
    }
    printf("Error! There is no register named as %d, please check vector setting!\n", reg);
    assert(0);
    return 0;
}

#endif
