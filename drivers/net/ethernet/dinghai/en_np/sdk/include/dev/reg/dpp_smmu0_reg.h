
#ifndef _DPP_SMMU0_REG_H_
#define _DPP_SMMU0_REG_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef struct dpp_smmu0_smmu0_init_done_t
{
    ZXIC_UINT32 init_done;
}DPP_SMMU0_SMMU0_INIT_DONE_T;

typedef struct dpp_smmu0_smmu0_cpu_ind_wdat0_t
{
    ZXIC_UINT32 cpu_ind_wdat0;
}DPP_SMMU0_SMMU0_CPU_IND_WDAT0_T;

typedef struct dpp_smmu0_smmu0_cpu_ind_wdat1_t
{
    ZXIC_UINT32 cpu_ind_wdat1;
}DPP_SMMU0_SMMU0_CPU_IND_WDAT1_T;

typedef struct dpp_smmu0_smmu0_cpu_ind_wdat2_t
{
    ZXIC_UINT32 cpu_ind_wdat2;
}DPP_SMMU0_SMMU0_CPU_IND_WDAT2_T;

typedef struct dpp_smmu0_smmu0_cpu_ind_wdat3_t
{
    ZXIC_UINT32 cpu_ind_wdat3;
}DPP_SMMU0_SMMU0_CPU_IND_WDAT3_T;

typedef struct dpp_smmu0_smmu0_cpu_ind_cmd_t
{
    ZXIC_UINT32 cpu_ind_rw;
    ZXIC_UINT32 cpu_ind_rd_mode;
    ZXIC_UINT32 cpu_req_mode;
    ZXIC_UINT32 cpu_ind_addr;
}DPP_SMMU0_SMMU0_CPU_IND_CMD_T;

typedef struct dpp_smmu0_smmu0_cpu_ind_rd_done_t
{
    ZXIC_UINT32 cpu_ind_rd_done;
}DPP_SMMU0_SMMU0_CPU_IND_RD_DONE_T;

typedef struct dpp_smmu0_smmu0_cpu_ind_rdat0_t
{
    ZXIC_UINT32 cpu_ind_rdat0;
}DPP_SMMU0_SMMU0_CPU_IND_RDAT0_T;

typedef struct dpp_smmu0_smmu0_cpu_ind_rdat1_t
{
    ZXIC_UINT32 cpu_ind_rdat1;
}DPP_SMMU0_SMMU0_CPU_IND_RDAT1_T;

typedef struct dpp_smmu0_smmu0_cpu_ind_rdat2_t
{
    ZXIC_UINT32 cpu_ind_rdat2;
}DPP_SMMU0_SMMU0_CPU_IND_RDAT2_T;

typedef struct dpp_smmu0_smmu0_cpu_ind_rdat3_t
{
    ZXIC_UINT32 cpu_ind_rdat3;
}DPP_SMMU0_SMMU0_CPU_IND_RDAT3_T;

typedef struct dpp_smmu0_smmu0_cfg_plcr_mono_t
{
    ZXIC_UINT32 cfg_plcr_mono;
}DPP_SMMU0_SMMU0_CFG_PLCR_MONO_T;

typedef struct dpp_smmu0_smmu0_wr_arb_cpu_rdy_t
{
    ZXIC_UINT32 wr_arb_cpu_rdy;
}DPP_SMMU0_SMMU0_WR_ARB_CPU_RDY_T;

typedef struct dpp_smmu0_smmu0_tm_stat_en_cfg_t
{
    ZXIC_UINT32 tm_stat_en_cfg;
}DPP_SMMU0_SMMU0_TM_STAT_EN_CFG_T;

typedef struct dpp_smmu0_smmu0_kschd_pful_cfg0_t
{
    ZXIC_UINT32 kschd_pful_assert0_1;
    ZXIC_UINT32 kschd_pful_negate0_1;
    ZXIC_UINT32 kschd_pful_assert0_0;
    ZXIC_UINT32 kschd_pful_negate0_0;
}DPP_SMMU0_SMMU0_KSCHD_PFUL_CFG0_T;

typedef struct dpp_smmu0_smmu0_kschd_pful_cfg1_t
{
    ZXIC_UINT32 kschd_pful_assert1_1;
    ZXIC_UINT32 kschd_pful_negate1_1;
    ZXIC_UINT32 kschd_pful_assert1_0;
    ZXIC_UINT32 kschd_pful_negate1_0;
}DPP_SMMU0_SMMU0_KSCHD_PFUL_CFG1_T;

typedef struct dpp_smmu0_smmu0_ctrl_pful1_cfg_t
{
    ZXIC_UINT32 ctrl_pful1_assert;
    ZXIC_UINT32 ctrl_pful1_negate;
}DPP_SMMU0_SMMU0_CTRL_PFUL1_CFG_T;

typedef struct dpp_smmu0_smmu0_ctrl_pful2_cfg_t
{
    ZXIC_UINT32 ctrl_pful2_assert;
    ZXIC_UINT32 ctrl_pful2_negate;
}DPP_SMMU0_SMMU0_CTRL_PFUL2_CFG_T;

typedef struct dpp_smmu0_smmu0_ctrl_pful3_cfg_t
{
    ZXIC_UINT32 ctrl_pful3_assert;
    ZXIC_UINT32 ctrl_pful3_negate;
}DPP_SMMU0_SMMU0_CTRL_PFUL3_CFG_T;

typedef struct dpp_smmu0_smmu0_rschd_pful_cfg_t
{
    ZXIC_UINT32 rschd_pful_assert;
    ZXIC_UINT32 rschd_pful_negate;
}DPP_SMMU0_SMMU0_RSCHD_PFUL_CFG_T;

typedef struct dpp_smmu0_smmu0_rschd_ept_cfg_t
{
    ZXIC_UINT32 rschd_ept_assert;
    ZXIC_UINT32 rschd_ept_negate;
}DPP_SMMU0_SMMU0_RSCHD_EPT_CFG_T;

typedef struct dpp_smmu0_smmu0_alucmd_pful_cfg_t
{
    ZXIC_UINT32 alucmd_pful_assert;
    ZXIC_UINT32 alucmd_pful_negate;
}DPP_SMMU0_SMMU0_ALUCMD_PFUL_CFG_T;

typedef struct dpp_smmu0_smmu0_aluwr_pful_cfg_t
{
    ZXIC_UINT32 aluwr_pful_assert;
    ZXIC_UINT32 aluwr_pful_negate;
}DPP_SMMU0_SMMU0_ALUWR_PFUL_CFG_T;

typedef struct dpp_smmu0_smmu0_wr_arb_pful_cfg0_t
{
    ZXIC_UINT32 wr_arb_pful0_assert;
    ZXIC_UINT32 wr_arb_pful0_negate;
}DPP_SMMU0_SMMU0_WR_ARB_PFUL_CFG0_T;

typedef struct dpp_smmu0_smmu0_wr_arb_pful_cfg1_t
{
    ZXIC_UINT32 wr_arb_pful1_assert;
    ZXIC_UINT32 wr_arb_pful1_negate;
}DPP_SMMU0_SMMU0_WR_ARB_PFUL_CFG1_T;

typedef struct dpp_smmu0_smmu0_ord_pful_cfg_t
{
    ZXIC_UINT32 ord_pful_assert;
    ZXIC_UINT32 ord_pful_negate;
}DPP_SMMU0_SMMU0_ORD_PFUL_CFG_T;

typedef struct dpp_smmu0_smmu0_cfg_dma_baddr_t
{
    ZXIC_UINT32 cfg_dma_baddr;
}DPP_SMMU0_SMMU0_CFG_DMA_BADDR_T;

typedef struct dpp_smmu0_smmu0_cfg_odma0_baddr_t
{
    ZXIC_UINT32 cfg_odma0_baddr;
}DPP_SMMU0_SMMU0_CFG_ODMA0_BADDR_T;

typedef struct dpp_smmu0_smmu0_cfg_odma1_baddr_t
{
    ZXIC_UINT32 cfg_odma1_baddr;
}DPP_SMMU0_SMMU0_CFG_ODMA1_BADDR_T;

typedef struct dpp_smmu0_smmu0_cfg_odma2_baddr_t
{
    ZXIC_UINT32 cfg_odma2_baddr;
}DPP_SMMU0_SMMU0_CFG_ODMA2_BADDR_T;

typedef struct dpp_smmu0_smmu0_cfg_odma_tdm_baddr_t
{
    ZXIC_UINT32 cfg_odma_tdm_baddr;
}DPP_SMMU0_SMMU0_CFG_ODMA_TDM_BADDR_T;

typedef struct dpp_smmu0_smmu0_cfg_mcast_baddr_t
{
    ZXIC_UINT32 cfg_mcast_baddr;
}DPP_SMMU0_SMMU0_CFG_MCAST_BADDR_T;

typedef struct dpp_smmu0_smmu0_cfg_lpm0_t
{
    ZXIC_UINT32 lpm0_rsp_mode;
    ZXIC_UINT32 lpm0_baddr;
}DPP_SMMU0_SMMU0_CFG_LPM0_T;

typedef struct dpp_smmu0_smmu0_cfg_lpm1_t
{
    ZXIC_UINT32 lpm1_rsp_mode;
    ZXIC_UINT32 lpm1_baddr;
}DPP_SMMU0_SMMU0_CFG_LPM1_T;

typedef struct dpp_smmu0_smmu0_cfg_lpm2_t
{
    ZXIC_UINT32 lpm2_rsp_mode;
    ZXIC_UINT32 lpm2_baddr;
}DPP_SMMU0_SMMU0_CFG_LPM2_T;

typedef struct dpp_smmu0_smmu0_cfg_lpm3_t
{
    ZXIC_UINT32 lpm3_rsp_mode;
    ZXIC_UINT32 lpm3_baddr;
}DPP_SMMU0_SMMU0_CFG_LPM3_T;

typedef struct dpp_smmu0_smmu0_cfg_lpm4_t
{
    ZXIC_UINT32 lpm4_rsp_mode;
    ZXIC_UINT32 lpm4_baddr;
}DPP_SMMU0_SMMU0_CFG_LPM4_T;

typedef struct dpp_smmu0_smmu0_cfg_lpm5_t
{
    ZXIC_UINT32 lpm5_rsp_mode;
    ZXIC_UINT32 lpm5_baddr;
}DPP_SMMU0_SMMU0_CFG_LPM5_T;

typedef struct dpp_smmu0_smmu0_cfg_lpm6_t
{
    ZXIC_UINT32 lpm6_rsp_mode;
    ZXIC_UINT32 lpm6_baddr;
}DPP_SMMU0_SMMU0_CFG_LPM6_T;

typedef struct dpp_smmu0_smmu0_cfg_lpm7_t
{
    ZXIC_UINT32 lpm7_rsp_mode;
    ZXIC_UINT32 lpm7_baddr;
}DPP_SMMU0_SMMU0_CFG_LPM7_T;

typedef struct dpp_smmu0_smmu0_debug_cnt_mode_t
{
    ZXIC_UINT32 cnt_rd_mode;
    ZXIC_UINT32 cnt_overflow_mode;
}DPP_SMMU0_SMMU0_DEBUG_CNT_MODE_T;

typedef struct dpp_smmu0_smmu0_stat_overflow_mode_t
{
    ZXIC_UINT32 stat_overflow_mode;
}DPP_SMMU0_SMMU0_STAT_OVERFLOW_MODE_T;

typedef struct dpp_smmu0_smmu0_init_en_cfg_tmp_t
{
    ZXIC_UINT32 init_en_cfg_tmp31;
    ZXIC_UINT32 init_en_cfg_tmp30;
    ZXIC_UINT32 init_en_cfg_tmp29;
    ZXIC_UINT32 init_en_cfg_tmp28;
    ZXIC_UINT32 init_en_cfg_tmp27;
    ZXIC_UINT32 init_en_cfg_tmp26;
    ZXIC_UINT32 init_en_cfg_tmp25;
    ZXIC_UINT32 init_en_cfg_tmp24;
    ZXIC_UINT32 init_en_cfg_tmp23;
    ZXIC_UINT32 init_en_cfg_tmp22;
    ZXIC_UINT32 init_en_cfg_tmp21;
    ZXIC_UINT32 init_en_cfg_tmp20;
    ZXIC_UINT32 init_en_cfg_tmp19;
    ZXIC_UINT32 init_en_cfg_tmp18;
    ZXIC_UINT32 init_en_cfg_tmp17;
    ZXIC_UINT32 init_en_cfg_tmp16;
    ZXIC_UINT32 init_en_cfg_tmp15;
    ZXIC_UINT32 init_en_cfg_tmp14;
    ZXIC_UINT32 init_en_cfg_tmp13;
    ZXIC_UINT32 init_en_cfg_tmp12;
    ZXIC_UINT32 init_en_cfg_tmp11;
    ZXIC_UINT32 init_en_cfg_tmp10;
    ZXIC_UINT32 init_en_cfg_tmp9;
    ZXIC_UINT32 init_en_cfg_tmp8;
    ZXIC_UINT32 init_en_cfg_tmp7;
    ZXIC_UINT32 init_en_cfg_tmp6;
    ZXIC_UINT32 init_en_cfg_tmp5;
    ZXIC_UINT32 init_en_cfg_tmp4;
    ZXIC_UINT32 init_en_cfg_tmp3;
    ZXIC_UINT32 init_en_cfg_tmp2;
    ZXIC_UINT32 init_en_cfg_tmp1;
    ZXIC_UINT32 init_en_cfg_tmp0;
}DPP_SMMU0_SMMU0_INIT_EN_CFG_TMP_T;

typedef struct dpp_smmu0_smmu0_smmu0_int_unmask_flag_t
{
    ZXIC_UINT32 smmu0_int0_31_unmask_flag;
}DPP_SMMU0_SMMU0_SMMU0_INT_UNMASK_FLAG_T;

typedef struct dpp_smmu0_smmu0_smmu0_int0_en_t
{
    ZXIC_UINT32 smmu0_int0_en31;
    ZXIC_UINT32 smmu0_int0_en30;
    ZXIC_UINT32 smmu0_int0_en29;
    ZXIC_UINT32 smmu0_int0_en28;
    ZXIC_UINT32 smmu0_int0_en27;
    ZXIC_UINT32 smmu0_int0_en26;
    ZXIC_UINT32 smmu0_int0_en25;
    ZXIC_UINT32 smmu0_int0_en24;
    ZXIC_UINT32 smmu0_int0_en23;
    ZXIC_UINT32 smmu0_int0_en22;
    ZXIC_UINT32 smmu0_int0_en21;
    ZXIC_UINT32 smmu0_int0_en20;
    ZXIC_UINT32 smmu0_int0_en19;
    ZXIC_UINT32 smmu0_int0_en18;
    ZXIC_UINT32 smmu0_int0_en17;
    ZXIC_UINT32 smmu0_int0_en16;
    ZXIC_UINT32 smmu0_int0_en15;
    ZXIC_UINT32 smmu0_int0_en14;
    ZXIC_UINT32 smmu0_int0_en13;
    ZXIC_UINT32 smmu0_int0_en12;
    ZXIC_UINT32 smmu0_int0_en11;
    ZXIC_UINT32 smmu0_int0_en10;
    ZXIC_UINT32 smmu0_int0_en9;
    ZXIC_UINT32 smmu0_int0_en8;
    ZXIC_UINT32 smmu0_int0_en7;
    ZXIC_UINT32 smmu0_int0_en6;
    ZXIC_UINT32 smmu0_int0_en5;
    ZXIC_UINT32 smmu0_int0_en4;
    ZXIC_UINT32 smmu0_int0_en3;
    ZXIC_UINT32 smmu0_int0_en2;
    ZXIC_UINT32 smmu0_int0_en1;
    ZXIC_UINT32 smmu0_int0_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT0_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int0_mask_t
{
    ZXIC_UINT32 smmu0_int0_mask31;
    ZXIC_UINT32 smmu0_int0_mask30;
    ZXIC_UINT32 smmu0_int0_mask29;
    ZXIC_UINT32 smmu0_int0_mask28;
    ZXIC_UINT32 smmu0_int0_mask27;
    ZXIC_UINT32 smmu0_int0_mask26;
    ZXIC_UINT32 smmu0_int0_mask25;
    ZXIC_UINT32 smmu0_int0_mask24;
    ZXIC_UINT32 smmu0_int0_mask23;
    ZXIC_UINT32 smmu0_int0_mask22;
    ZXIC_UINT32 smmu0_int0_mask21;
    ZXIC_UINT32 smmu0_int0_mask20;
    ZXIC_UINT32 smmu0_int0_mask19;
    ZXIC_UINT32 smmu0_int0_mask18;
    ZXIC_UINT32 smmu0_int0_mask17;
    ZXIC_UINT32 smmu0_int0_mask16;
    ZXIC_UINT32 smmu0_int0_mask15;
    ZXIC_UINT32 smmu0_int0_mask14;
    ZXIC_UINT32 smmu0_int0_mask13;
    ZXIC_UINT32 smmu0_int0_mask12;
    ZXIC_UINT32 smmu0_int0_mask11;
    ZXIC_UINT32 smmu0_int0_mask10;
    ZXIC_UINT32 smmu0_int0_mask9;
    ZXIC_UINT32 smmu0_int0_mask8;
    ZXIC_UINT32 smmu0_int0_mask7;
    ZXIC_UINT32 smmu0_int0_mask6;
    ZXIC_UINT32 smmu0_int0_mask5;
    ZXIC_UINT32 smmu0_int0_mask4;
    ZXIC_UINT32 smmu0_int0_mask3;
    ZXIC_UINT32 smmu0_int0_mask2;
    ZXIC_UINT32 smmu0_int0_mask1;
    ZXIC_UINT32 smmu0_int0_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT0_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int0_status_t
{
    ZXIC_UINT32 smmu0_int0_status31;
    ZXIC_UINT32 smmu0_int0_status30;
    ZXIC_UINT32 smmu0_int0_status29;
    ZXIC_UINT32 smmu0_int0_status28;
    ZXIC_UINT32 smmu0_int0_status27;
    ZXIC_UINT32 smmu0_int0_status26;
    ZXIC_UINT32 smmu0_int0_status25;
    ZXIC_UINT32 smmu0_int0_status24;
    ZXIC_UINT32 smmu0_int0_status23;
    ZXIC_UINT32 smmu0_int0_status22;
    ZXIC_UINT32 smmu0_int0_status21;
    ZXIC_UINT32 smmu0_int0_status20;
    ZXIC_UINT32 smmu0_int0_status19;
    ZXIC_UINT32 smmu0_int0_status18;
    ZXIC_UINT32 smmu0_int0_status17;
    ZXIC_UINT32 smmu0_int0_status16;
    ZXIC_UINT32 smmu0_int0_status15;
    ZXIC_UINT32 smmu0_int0_status14;
    ZXIC_UINT32 smmu0_int0_status13;
    ZXIC_UINT32 smmu0_int0_status12;
    ZXIC_UINT32 smmu0_int0_status11;
    ZXIC_UINT32 smmu0_int0_status10;
    ZXIC_UINT32 smmu0_int0_status9;
    ZXIC_UINT32 smmu0_int0_status8;
    ZXIC_UINT32 smmu0_int0_status7;
    ZXIC_UINT32 smmu0_int0_status6;
    ZXIC_UINT32 smmu0_int0_status5;
    ZXIC_UINT32 smmu0_int0_status4;
    ZXIC_UINT32 smmu0_int0_status3;
    ZXIC_UINT32 smmu0_int0_status2;
    ZXIC_UINT32 smmu0_int0_status1;
    ZXIC_UINT32 smmu0_int0_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT0_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int1_en_t
{
    ZXIC_UINT32 smmu0_int1_en31;
    ZXIC_UINT32 smmu0_int1_en30;
    ZXIC_UINT32 smmu0_int1_en29;
    ZXIC_UINT32 smmu0_int1_en28;
    ZXIC_UINT32 smmu0_int1_en27;
    ZXIC_UINT32 smmu0_int1_en26;
    ZXIC_UINT32 smmu0_int1_en25;
    ZXIC_UINT32 smmu0_int1_en24;
    ZXIC_UINT32 smmu0_int1_en23;
    ZXIC_UINT32 smmu0_int1_en22;
    ZXIC_UINT32 smmu0_int1_en21;
    ZXIC_UINT32 smmu0_int1_en20;
    ZXIC_UINT32 smmu0_int1_en19;
    ZXIC_UINT32 smmu0_int1_en18;
    ZXIC_UINT32 smmu0_int1_en17;
    ZXIC_UINT32 smmu0_int1_en16;
    ZXIC_UINT32 smmu0_int1_en15;
    ZXIC_UINT32 smmu0_int1_en14;
    ZXIC_UINT32 smmu0_int1_en13;
    ZXIC_UINT32 smmu0_int1_en12;
    ZXIC_UINT32 smmu0_int1_en11;
    ZXIC_UINT32 smmu0_int1_en10;
    ZXIC_UINT32 smmu0_int1_en9;
    ZXIC_UINT32 smmu0_int1_en8;
    ZXIC_UINT32 smmu0_int1_en7;
    ZXIC_UINT32 smmu0_int1_en6;
    ZXIC_UINT32 smmu0_int1_en5;
    ZXIC_UINT32 smmu0_int1_en4;
    ZXIC_UINT32 smmu0_int1_en3;
    ZXIC_UINT32 smmu0_int1_en2;
    ZXIC_UINT32 smmu0_int1_en1;
    ZXIC_UINT32 smmu0_int1_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT1_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int1_mask_t
{
    ZXIC_UINT32 smmu0_int1_mask31;
    ZXIC_UINT32 smmu0_int1_mask30;
    ZXIC_UINT32 smmu0_int1_mask29;
    ZXIC_UINT32 smmu0_int1_mask28;
    ZXIC_UINT32 smmu0_int1_mask27;
    ZXIC_UINT32 smmu0_int1_mask26;
    ZXIC_UINT32 smmu0_int1_mask25;
    ZXIC_UINT32 smmu0_int1_mask24;
    ZXIC_UINT32 smmu0_int1_mask23;
    ZXIC_UINT32 smmu0_int1_mask22;
    ZXIC_UINT32 smmu0_int1_mask21;
    ZXIC_UINT32 smmu0_int1_mask20;
    ZXIC_UINT32 smmu0_int1_mask19;
    ZXIC_UINT32 smmu0_int1_mask18;
    ZXIC_UINT32 smmu0_int1_mask17;
    ZXIC_UINT32 smmu0_int1_mask16;
    ZXIC_UINT32 smmu0_int1_mask15;
    ZXIC_UINT32 smmu0_int1_mask14;
    ZXIC_UINT32 smmu0_int1_mask13;
    ZXIC_UINT32 smmu0_int1_mask12;
    ZXIC_UINT32 smmu0_int1_mask11;
    ZXIC_UINT32 smmu0_int1_mask10;
    ZXIC_UINT32 smmu0_int1_mask9;
    ZXIC_UINT32 smmu0_int1_mask8;
    ZXIC_UINT32 smmu0_int1_mask7;
    ZXIC_UINT32 smmu0_int1_mask6;
    ZXIC_UINT32 smmu0_int1_mask5;
    ZXIC_UINT32 smmu0_int1_mask4;
    ZXIC_UINT32 smmu0_int1_mask3;
    ZXIC_UINT32 smmu0_int1_mask2;
    ZXIC_UINT32 smmu0_int1_mask1;
    ZXIC_UINT32 smmu0_int1_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT1_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int1_status_t
{
    ZXIC_UINT32 smmu0_int1_status31;
    ZXIC_UINT32 smmu0_int1_status30;
    ZXIC_UINT32 smmu0_int1_status29;
    ZXIC_UINT32 smmu0_int1_status28;
    ZXIC_UINT32 smmu0_int1_status27;
    ZXIC_UINT32 smmu0_int1_status26;
    ZXIC_UINT32 smmu0_int1_status25;
    ZXIC_UINT32 smmu0_int1_status24;
    ZXIC_UINT32 smmu0_int1_status23;
    ZXIC_UINT32 smmu0_int1_status22;
    ZXIC_UINT32 smmu0_int1_status21;
    ZXIC_UINT32 smmu0_int1_status20;
    ZXIC_UINT32 smmu0_int1_status19;
    ZXIC_UINT32 smmu0_int1_status18;
    ZXIC_UINT32 smmu0_int1_status17;
    ZXIC_UINT32 smmu0_int1_status16;
    ZXIC_UINT32 smmu0_int1_status15;
    ZXIC_UINT32 smmu0_int1_status14;
    ZXIC_UINT32 smmu0_int1_status13;
    ZXIC_UINT32 smmu0_int1_status12;
    ZXIC_UINT32 smmu0_int1_status11;
    ZXIC_UINT32 smmu0_int1_status10;
    ZXIC_UINT32 smmu0_int1_status9;
    ZXIC_UINT32 smmu0_int1_status8;
    ZXIC_UINT32 smmu0_int1_status7;
    ZXIC_UINT32 smmu0_int1_status6;
    ZXIC_UINT32 smmu0_int1_status5;
    ZXIC_UINT32 smmu0_int1_status4;
    ZXIC_UINT32 smmu0_int1_status3;
    ZXIC_UINT32 smmu0_int1_status2;
    ZXIC_UINT32 smmu0_int1_status1;
    ZXIC_UINT32 smmu0_int1_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT1_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int2_en_t
{
    ZXIC_UINT32 smmu0_int2_en31;
    ZXIC_UINT32 smmu0_int2_en30;
    ZXIC_UINT32 smmu0_int2_en29;
    ZXIC_UINT32 smmu0_int2_en28;
    ZXIC_UINT32 smmu0_int2_en27;
    ZXIC_UINT32 smmu0_int2_en26;
    ZXIC_UINT32 smmu0_int2_en25;
    ZXIC_UINT32 smmu0_int2_en24;
    ZXIC_UINT32 smmu0_int2_en23;
    ZXIC_UINT32 smmu0_int2_en22;
    ZXIC_UINT32 smmu0_int2_en21;
    ZXIC_UINT32 smmu0_int2_en20;
    ZXIC_UINT32 smmu0_int2_en19;
    ZXIC_UINT32 smmu0_int2_en18;
    ZXIC_UINT32 smmu0_int2_en17;
    ZXIC_UINT32 smmu0_int2_en16;
    ZXIC_UINT32 smmu0_int2_en15;
    ZXIC_UINT32 smmu0_int2_en14;
    ZXIC_UINT32 smmu0_int2_en13;
    ZXIC_UINT32 smmu0_int2_en12;
    ZXIC_UINT32 smmu0_int2_en11;
    ZXIC_UINT32 smmu0_int2_en10;
    ZXIC_UINT32 smmu0_int2_en9;
    ZXIC_UINT32 smmu0_int2_en8;
    ZXIC_UINT32 smmu0_int2_en7;
    ZXIC_UINT32 smmu0_int2_en6;
    ZXIC_UINT32 smmu0_int2_en5;
    ZXIC_UINT32 smmu0_int2_en4;
    ZXIC_UINT32 smmu0_int2_en3;
    ZXIC_UINT32 smmu0_int2_en2;
    ZXIC_UINT32 smmu0_int2_en1;
    ZXIC_UINT32 smmu0_int2_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT2_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int2_mask_t
{
    ZXIC_UINT32 smmu0_int2_mask31;
    ZXIC_UINT32 smmu0_int2_mask30;
    ZXIC_UINT32 smmu0_int2_mask29;
    ZXIC_UINT32 smmu0_int2_mask28;
    ZXIC_UINT32 smmu0_int2_mask27;
    ZXIC_UINT32 smmu0_int2_mask26;
    ZXIC_UINT32 smmu0_int2_mask25;
    ZXIC_UINT32 smmu0_int2_mask24;
    ZXIC_UINT32 smmu0_int2_mask23;
    ZXIC_UINT32 smmu0_int2_mask22;
    ZXIC_UINT32 smmu0_int2_mask21;
    ZXIC_UINT32 smmu0_int2_mask20;
    ZXIC_UINT32 smmu0_int2_mask19;
    ZXIC_UINT32 smmu0_int2_mask18;
    ZXIC_UINT32 smmu0_int2_mask17;
    ZXIC_UINT32 smmu0_int2_mask16;
    ZXIC_UINT32 smmu0_int2_mask15;
    ZXIC_UINT32 smmu0_int2_mask14;
    ZXIC_UINT32 smmu0_int2_mask13;
    ZXIC_UINT32 smmu0_int2_mask12;
    ZXIC_UINT32 smmu0_int2_mask11;
    ZXIC_UINT32 smmu0_int2_mask10;
    ZXIC_UINT32 smmu0_int2_mask9;
    ZXIC_UINT32 smmu0_int2_mask8;
    ZXIC_UINT32 smmu0_int2_mask7;
    ZXIC_UINT32 smmu0_int2_mask6;
    ZXIC_UINT32 smmu0_int2_mask5;
    ZXIC_UINT32 smmu0_int2_mask4;
    ZXIC_UINT32 smmu0_int2_mask3;
    ZXIC_UINT32 smmu0_int2_mask2;
    ZXIC_UINT32 smmu0_int2_mask1;
    ZXIC_UINT32 smmu0_int2_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT2_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int2_status_t
{
    ZXIC_UINT32 smmu0_int2_status31;
    ZXIC_UINT32 smmu0_int2_status30;
    ZXIC_UINT32 smmu0_int2_status29;
    ZXIC_UINT32 smmu0_int2_status28;
    ZXIC_UINT32 smmu0_int2_status27;
    ZXIC_UINT32 smmu0_int2_status26;
    ZXIC_UINT32 smmu0_int2_status25;
    ZXIC_UINT32 smmu0_int2_status24;
    ZXIC_UINT32 smmu0_int2_status23;
    ZXIC_UINT32 smmu0_int2_status22;
    ZXIC_UINT32 smmu0_int2_status21;
    ZXIC_UINT32 smmu0_int2_status20;
    ZXIC_UINT32 smmu0_int2_status19;
    ZXIC_UINT32 smmu0_int2_status18;
    ZXIC_UINT32 smmu0_int2_status17;
    ZXIC_UINT32 smmu0_int2_status16;
    ZXIC_UINT32 smmu0_int2_status15;
    ZXIC_UINT32 smmu0_int2_status14;
    ZXIC_UINT32 smmu0_int2_status13;
    ZXIC_UINT32 smmu0_int2_status12;
    ZXIC_UINT32 smmu0_int2_status11;
    ZXIC_UINT32 smmu0_int2_status10;
    ZXIC_UINT32 smmu0_int2_status9;
    ZXIC_UINT32 smmu0_int2_status8;
    ZXIC_UINT32 smmu0_int2_status7;
    ZXIC_UINT32 smmu0_int2_status6;
    ZXIC_UINT32 smmu0_int2_status5;
    ZXIC_UINT32 smmu0_int2_status4;
    ZXIC_UINT32 smmu0_int2_status3;
    ZXIC_UINT32 smmu0_int2_status2;
    ZXIC_UINT32 smmu0_int2_status1;
    ZXIC_UINT32 smmu0_int2_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT2_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int3_en_t
{
    ZXIC_UINT32 smmu0_int3_en31;
    ZXIC_UINT32 smmu0_int3_en30;
    ZXIC_UINT32 smmu0_int3_en29;
    ZXIC_UINT32 smmu0_int3_en28;
    ZXIC_UINT32 smmu0_int3_en27;
    ZXIC_UINT32 smmu0_int3_en26;
    ZXIC_UINT32 smmu0_int3_en25;
    ZXIC_UINT32 smmu0_int3_en24;
    ZXIC_UINT32 smmu0_int3_en23;
    ZXIC_UINT32 smmu0_int3_en22;
    ZXIC_UINT32 smmu0_int3_en21;
    ZXIC_UINT32 smmu0_int3_en20;
    ZXIC_UINT32 smmu0_int3_en19;
    ZXIC_UINT32 smmu0_int3_en18;
    ZXIC_UINT32 smmu0_int3_en17;
    ZXIC_UINT32 smmu0_int3_en16;
    ZXIC_UINT32 smmu0_int3_en15;
    ZXIC_UINT32 smmu0_int3_en14;
    ZXIC_UINT32 smmu0_int3_en13;
    ZXIC_UINT32 smmu0_int3_en12;
    ZXIC_UINT32 smmu0_int3_en11;
    ZXIC_UINT32 smmu0_int3_en10;
    ZXIC_UINT32 smmu0_int3_en9;
    ZXIC_UINT32 smmu0_int3_en8;
    ZXIC_UINT32 smmu0_int3_en7;
    ZXIC_UINT32 smmu0_int3_en6;
    ZXIC_UINT32 smmu0_int3_en5;
    ZXIC_UINT32 smmu0_int3_en4;
    ZXIC_UINT32 smmu0_int3_en3;
    ZXIC_UINT32 smmu0_int3_en2;
    ZXIC_UINT32 smmu0_int3_en1;
    ZXIC_UINT32 smmu0_int3_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT3_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int3_mask_t
{
    ZXIC_UINT32 smmu0_int3_mask31;
    ZXIC_UINT32 smmu0_int3_mask30;
    ZXIC_UINT32 smmu0_int3_mask29;
    ZXIC_UINT32 smmu0_int3_mask28;
    ZXIC_UINT32 smmu0_int3_mask27;
    ZXIC_UINT32 smmu0_int3_mask26;
    ZXIC_UINT32 smmu0_int3_mask25;
    ZXIC_UINT32 smmu0_int3_mask24;
    ZXIC_UINT32 smmu0_int3_mask23;
    ZXIC_UINT32 smmu0_int3_mask22;
    ZXIC_UINT32 smmu0_int3_mask21;
    ZXIC_UINT32 smmu0_int3_mask20;
    ZXIC_UINT32 smmu0_int3_mask19;
    ZXIC_UINT32 smmu0_int3_mask18;
    ZXIC_UINT32 smmu0_int3_mask17;
    ZXIC_UINT32 smmu0_int3_mask16;
    ZXIC_UINT32 smmu0_int3_mask15;
    ZXIC_UINT32 smmu0_int3_mask14;
    ZXIC_UINT32 smmu0_int3_mask13;
    ZXIC_UINT32 smmu0_int3_mask12;
    ZXIC_UINT32 smmu0_int3_mask11;
    ZXIC_UINT32 smmu0_int3_mask10;
    ZXIC_UINT32 smmu0_int3_mask9;
    ZXIC_UINT32 smmu0_int3_mask8;
    ZXIC_UINT32 smmu0_int3_mask7;
    ZXIC_UINT32 smmu0_int3_mask6;
    ZXIC_UINT32 smmu0_int3_mask5;
    ZXIC_UINT32 smmu0_int3_mask4;
    ZXIC_UINT32 smmu0_int3_mask3;
    ZXIC_UINT32 smmu0_int3_mask2;
    ZXIC_UINT32 smmu0_int3_mask1;
    ZXIC_UINT32 smmu0_int3_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT3_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int3_status_t
{
    ZXIC_UINT32 smmu0_int3_status31;
    ZXIC_UINT32 smmu0_int3_status30;
    ZXIC_UINT32 smmu0_int3_status29;
    ZXIC_UINT32 smmu0_int3_status28;
    ZXIC_UINT32 smmu0_int3_status27;
    ZXIC_UINT32 smmu0_int3_status26;
    ZXIC_UINT32 smmu0_int3_status25;
    ZXIC_UINT32 smmu0_int3_status24;
    ZXIC_UINT32 smmu0_int3_status23;
    ZXIC_UINT32 smmu0_int3_status22;
    ZXIC_UINT32 smmu0_int3_status21;
    ZXIC_UINT32 smmu0_int3_status20;
    ZXIC_UINT32 smmu0_int3_status19;
    ZXIC_UINT32 smmu0_int3_status18;
    ZXIC_UINT32 smmu0_int3_status17;
    ZXIC_UINT32 smmu0_int3_status16;
    ZXIC_UINT32 smmu0_int3_status15;
    ZXIC_UINT32 smmu0_int3_status14;
    ZXIC_UINT32 smmu0_int3_status13;
    ZXIC_UINT32 smmu0_int3_status12;
    ZXIC_UINT32 smmu0_int3_status11;
    ZXIC_UINT32 smmu0_int3_status10;
    ZXIC_UINT32 smmu0_int3_status9;
    ZXIC_UINT32 smmu0_int3_status8;
    ZXIC_UINT32 smmu0_int3_status7;
    ZXIC_UINT32 smmu0_int3_status6;
    ZXIC_UINT32 smmu0_int3_status5;
    ZXIC_UINT32 smmu0_int3_status4;
    ZXIC_UINT32 smmu0_int3_status3;
    ZXIC_UINT32 smmu0_int3_status2;
    ZXIC_UINT32 smmu0_int3_status1;
    ZXIC_UINT32 smmu0_int3_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT3_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int4_en_t
{
    ZXIC_UINT32 smmu0_int4_en31;
    ZXIC_UINT32 smmu0_int4_en30;
    ZXIC_UINT32 smmu0_int4_en29;
    ZXIC_UINT32 smmu0_int4_en28;
    ZXIC_UINT32 smmu0_int4_en27;
    ZXIC_UINT32 smmu0_int4_en26;
    ZXIC_UINT32 smmu0_int4_en25;
    ZXIC_UINT32 smmu0_int4_en24;
    ZXIC_UINT32 smmu0_int4_en23;
    ZXIC_UINT32 smmu0_int4_en22;
    ZXIC_UINT32 smmu0_int4_en21;
    ZXIC_UINT32 smmu0_int4_en20;
    ZXIC_UINT32 smmu0_int4_en19;
    ZXIC_UINT32 smmu0_int4_en18;
    ZXIC_UINT32 smmu0_int4_en17;
    ZXIC_UINT32 smmu0_int4_en16;
    ZXIC_UINT32 smmu0_int4_en15;
    ZXIC_UINT32 smmu0_int4_en14;
    ZXIC_UINT32 smmu0_int4_en13;
    ZXIC_UINT32 smmu0_int4_en12;
    ZXIC_UINT32 smmu0_int4_en11;
    ZXIC_UINT32 smmu0_int4_en10;
    ZXIC_UINT32 smmu0_int4_en9;
    ZXIC_UINT32 smmu0_int4_en8;
    ZXIC_UINT32 smmu0_int4_en7;
    ZXIC_UINT32 smmu0_int4_en6;
    ZXIC_UINT32 smmu0_int4_en5;
    ZXIC_UINT32 smmu0_int4_en4;
    ZXIC_UINT32 smmu0_int4_en3;
    ZXIC_UINT32 smmu0_int4_en2;
    ZXIC_UINT32 smmu0_int4_en1;
    ZXIC_UINT32 smmu0_int4_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT4_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int4_mask_t
{
    ZXIC_UINT32 smmu0_int4_mask31;
    ZXIC_UINT32 smmu0_int4_mask30;
    ZXIC_UINT32 smmu0_int4_mask29;
    ZXIC_UINT32 smmu0_int4_mask28;
    ZXIC_UINT32 smmu0_int4_mask27;
    ZXIC_UINT32 smmu0_int4_mask26;
    ZXIC_UINT32 smmu0_int4_mask25;
    ZXIC_UINT32 smmu0_int4_mask24;
    ZXIC_UINT32 smmu0_int4_mask23;
    ZXIC_UINT32 smmu0_int4_mask22;
    ZXIC_UINT32 smmu0_int4_mask21;
    ZXIC_UINT32 smmu0_int4_mask20;
    ZXIC_UINT32 smmu0_int4_mask19;
    ZXIC_UINT32 smmu0_int4_mask18;
    ZXIC_UINT32 smmu0_int4_mask17;
    ZXIC_UINT32 smmu0_int4_mask16;
    ZXIC_UINT32 smmu0_int4_mask15;
    ZXIC_UINT32 smmu0_int4_mask14;
    ZXIC_UINT32 smmu0_int4_mask13;
    ZXIC_UINT32 smmu0_int4_mask12;
    ZXIC_UINT32 smmu0_int4_mask11;
    ZXIC_UINT32 smmu0_int4_mask10;
    ZXIC_UINT32 smmu0_int4_mask9;
    ZXIC_UINT32 smmu0_int4_mask8;
    ZXIC_UINT32 smmu0_int4_mask7;
    ZXIC_UINT32 smmu0_int4_mask6;
    ZXIC_UINT32 smmu0_int4_mask5;
    ZXIC_UINT32 smmu0_int4_mask4;
    ZXIC_UINT32 smmu0_int4_mask3;
    ZXIC_UINT32 smmu0_int4_mask2;
    ZXIC_UINT32 smmu0_int4_mask1;
    ZXIC_UINT32 smmu0_int4_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT4_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int4_status_t
{
    ZXIC_UINT32 smmu0_int4_status31;
    ZXIC_UINT32 smmu0_int4_status30;
    ZXIC_UINT32 smmu0_int4_status29;
    ZXIC_UINT32 smmu0_int4_status28;
    ZXIC_UINT32 smmu0_int4_status27;
    ZXIC_UINT32 smmu0_int4_status26;
    ZXIC_UINT32 smmu0_int4_status25;
    ZXIC_UINT32 smmu0_int4_status24;
    ZXIC_UINT32 smmu0_int4_status23;
    ZXIC_UINT32 smmu0_int4_status22;
    ZXIC_UINT32 smmu0_int4_status21;
    ZXIC_UINT32 smmu0_int4_status20;
    ZXIC_UINT32 smmu0_int4_status19;
    ZXIC_UINT32 smmu0_int4_status18;
    ZXIC_UINT32 smmu0_int4_status17;
    ZXIC_UINT32 smmu0_int4_status16;
    ZXIC_UINT32 smmu0_int4_status15;
    ZXIC_UINT32 smmu0_int4_status14;
    ZXIC_UINT32 smmu0_int4_status13;
    ZXIC_UINT32 smmu0_int4_status12;
    ZXIC_UINT32 smmu0_int4_status11;
    ZXIC_UINT32 smmu0_int4_status10;
    ZXIC_UINT32 smmu0_int4_status9;
    ZXIC_UINT32 smmu0_int4_status8;
    ZXIC_UINT32 smmu0_int4_status7;
    ZXIC_UINT32 smmu0_int4_status6;
    ZXIC_UINT32 smmu0_int4_status5;
    ZXIC_UINT32 smmu0_int4_status4;
    ZXIC_UINT32 smmu0_int4_status3;
    ZXIC_UINT32 smmu0_int4_status2;
    ZXIC_UINT32 smmu0_int4_status1;
    ZXIC_UINT32 smmu0_int4_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT4_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int5_en_t
{
    ZXIC_UINT32 smmu0_int5_en31;
    ZXIC_UINT32 smmu0_int5_en30;
    ZXIC_UINT32 smmu0_int5_en29;
    ZXIC_UINT32 smmu0_int5_en28;
    ZXIC_UINT32 smmu0_int5_en27;
    ZXIC_UINT32 smmu0_int5_en26;
    ZXIC_UINT32 smmu0_int5_en25;
    ZXIC_UINT32 smmu0_int5_en24;
    ZXIC_UINT32 smmu0_int5_en23;
    ZXIC_UINT32 smmu0_int5_en22;
    ZXIC_UINT32 smmu0_int5_en21;
    ZXIC_UINT32 smmu0_int5_en20;
    ZXIC_UINT32 smmu0_int5_en19;
    ZXIC_UINT32 smmu0_int5_en18;
    ZXIC_UINT32 smmu0_int5_en17;
    ZXIC_UINT32 smmu0_int5_en16;
    ZXIC_UINT32 smmu0_int5_en15;
    ZXIC_UINT32 smmu0_int5_en14;
    ZXIC_UINT32 smmu0_int5_en13;
    ZXIC_UINT32 smmu0_int5_en12;
    ZXIC_UINT32 smmu0_int5_en11;
    ZXIC_UINT32 smmu0_int5_en10;
    ZXIC_UINT32 smmu0_int5_en9;
    ZXIC_UINT32 smmu0_int5_en8;
    ZXIC_UINT32 smmu0_int5_en7;
    ZXIC_UINT32 smmu0_int5_en6;
    ZXIC_UINT32 smmu0_int5_en5;
    ZXIC_UINT32 smmu0_int5_en4;
    ZXIC_UINT32 smmu0_int5_en3;
    ZXIC_UINT32 smmu0_int5_en2;
    ZXIC_UINT32 smmu0_int5_en1;
    ZXIC_UINT32 smmu0_int5_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT5_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int5_mask_t
{
    ZXIC_UINT32 smmu0_int5_mask31;
    ZXIC_UINT32 smmu0_int5_mask30;
    ZXIC_UINT32 smmu0_int5_mask29;
    ZXIC_UINT32 smmu0_int5_mask28;
    ZXIC_UINT32 smmu0_int5_mask27;
    ZXIC_UINT32 smmu0_int5_mask26;
    ZXIC_UINT32 smmu0_int5_mask25;
    ZXIC_UINT32 smmu0_int5_mask24;
    ZXIC_UINT32 smmu0_int5_mask23;
    ZXIC_UINT32 smmu0_int5_mask22;
    ZXIC_UINT32 smmu0_int5_mask21;
    ZXIC_UINT32 smmu0_int5_mask20;
    ZXIC_UINT32 smmu0_int5_mask19;
    ZXIC_UINT32 smmu0_int5_mask18;
    ZXIC_UINT32 smmu0_int5_mask17;
    ZXIC_UINT32 smmu0_int5_mask16;
    ZXIC_UINT32 smmu0_int5_mask15;
    ZXIC_UINT32 smmu0_int5_mask14;
    ZXIC_UINT32 smmu0_int5_mask13;
    ZXIC_UINT32 smmu0_int5_mask12;
    ZXIC_UINT32 smmu0_int5_mask11;
    ZXIC_UINT32 smmu0_int5_mask10;
    ZXIC_UINT32 smmu0_int5_mask9;
    ZXIC_UINT32 smmu0_int5_mask8;
    ZXIC_UINT32 smmu0_int5_mask7;
    ZXIC_UINT32 smmu0_int5_mask6;
    ZXIC_UINT32 smmu0_int5_mask5;
    ZXIC_UINT32 smmu0_int5_mask4;
    ZXIC_UINT32 smmu0_int5_mask3;
    ZXIC_UINT32 smmu0_int5_mask2;
    ZXIC_UINT32 smmu0_int5_mask1;
    ZXIC_UINT32 smmu0_int5_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT5_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int5_status_t
{
    ZXIC_UINT32 smmu0_int5_status31;
    ZXIC_UINT32 smmu0_int5_status30;
    ZXIC_UINT32 smmu0_int5_status29;
    ZXIC_UINT32 smmu0_int5_status28;
    ZXIC_UINT32 smmu0_int5_status27;
    ZXIC_UINT32 smmu0_int5_status26;
    ZXIC_UINT32 smmu0_int5_status25;
    ZXIC_UINT32 smmu0_int5_status24;
    ZXIC_UINT32 smmu0_int5_status23;
    ZXIC_UINT32 smmu0_int5_status22;
    ZXIC_UINT32 smmu0_int5_status21;
    ZXIC_UINT32 smmu0_int5_status20;
    ZXIC_UINT32 smmu0_int5_status19;
    ZXIC_UINT32 smmu0_int5_status18;
    ZXIC_UINT32 smmu0_int5_status17;
    ZXIC_UINT32 smmu0_int5_status16;
    ZXIC_UINT32 smmu0_int5_status15;
    ZXIC_UINT32 smmu0_int5_status14;
    ZXIC_UINT32 smmu0_int5_status13;
    ZXIC_UINT32 smmu0_int5_status12;
    ZXIC_UINT32 smmu0_int5_status11;
    ZXIC_UINT32 smmu0_int5_status10;
    ZXIC_UINT32 smmu0_int5_status9;
    ZXIC_UINT32 smmu0_int5_status8;
    ZXIC_UINT32 smmu0_int5_status7;
    ZXIC_UINT32 smmu0_int5_status6;
    ZXIC_UINT32 smmu0_int5_status5;
    ZXIC_UINT32 smmu0_int5_status4;
    ZXIC_UINT32 smmu0_int5_status3;
    ZXIC_UINT32 smmu0_int5_status2;
    ZXIC_UINT32 smmu0_int5_status1;
    ZXIC_UINT32 smmu0_int5_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT5_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int6_en_t
{
    ZXIC_UINT32 smmu0_int6_en31;
    ZXIC_UINT32 smmu0_int6_en30;
    ZXIC_UINT32 smmu0_int6_en29;
    ZXIC_UINT32 smmu0_int6_en28;
    ZXIC_UINT32 smmu0_int6_en27;
    ZXIC_UINT32 smmu0_int6_en26;
    ZXIC_UINT32 smmu0_int6_en25;
    ZXIC_UINT32 smmu0_int6_en24;
    ZXIC_UINT32 smmu0_int6_en23;
    ZXIC_UINT32 smmu0_int6_en22;
    ZXIC_UINT32 smmu0_int6_en21;
    ZXIC_UINT32 smmu0_int6_en20;
    ZXIC_UINT32 smmu0_int6_en19;
    ZXIC_UINT32 smmu0_int6_en18;
    ZXIC_UINT32 smmu0_int6_en17;
    ZXIC_UINT32 smmu0_int6_en16;
    ZXIC_UINT32 smmu0_int6_en15;
    ZXIC_UINT32 smmu0_int6_en14;
    ZXIC_UINT32 smmu0_int6_en13;
    ZXIC_UINT32 smmu0_int6_en12;
    ZXIC_UINT32 smmu0_int6_en11;
    ZXIC_UINT32 smmu0_int6_en10;
    ZXIC_UINT32 smmu0_int6_en9;
    ZXIC_UINT32 smmu0_int6_en8;
    ZXIC_UINT32 smmu0_int6_en7;
    ZXIC_UINT32 smmu0_int6_en6;
    ZXIC_UINT32 smmu0_int6_en5;
    ZXIC_UINT32 smmu0_int6_en4;
    ZXIC_UINT32 smmu0_int6_en3;
    ZXIC_UINT32 smmu0_int6_en2;
    ZXIC_UINT32 smmu0_int6_en1;
    ZXIC_UINT32 smmu0_int6_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT6_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int6_mask_t
{
    ZXIC_UINT32 smmu0_int6_mask31;
    ZXIC_UINT32 smmu0_int6_mask30;
    ZXIC_UINT32 smmu0_int6_mask29;
    ZXIC_UINT32 smmu0_int6_mask28;
    ZXIC_UINT32 smmu0_int6_mask27;
    ZXIC_UINT32 smmu0_int6_mask26;
    ZXIC_UINT32 smmu0_int6_mask25;
    ZXIC_UINT32 smmu0_int6_mask24;
    ZXIC_UINT32 smmu0_int6_mask23;
    ZXIC_UINT32 smmu0_int6_mask22;
    ZXIC_UINT32 smmu0_int6_mask21;
    ZXIC_UINT32 smmu0_int6_mask20;
    ZXIC_UINT32 smmu0_int6_mask19;
    ZXIC_UINT32 smmu0_int6_mask18;
    ZXIC_UINT32 smmu0_int6_mask17;
    ZXIC_UINT32 smmu0_int6_mask16;
    ZXIC_UINT32 smmu0_int6_mask15;
    ZXIC_UINT32 smmu0_int6_mask14;
    ZXIC_UINT32 smmu0_int6_mask13;
    ZXIC_UINT32 smmu0_int6_mask12;
    ZXIC_UINT32 smmu0_int6_mask11;
    ZXIC_UINT32 smmu0_int6_mask10;
    ZXIC_UINT32 smmu0_int6_mask9;
    ZXIC_UINT32 smmu0_int6_mask8;
    ZXIC_UINT32 smmu0_int6_mask7;
    ZXIC_UINT32 smmu0_int6_mask6;
    ZXIC_UINT32 smmu0_int6_mask5;
    ZXIC_UINT32 smmu0_int6_mask4;
    ZXIC_UINT32 smmu0_int6_mask3;
    ZXIC_UINT32 smmu0_int6_mask2;
    ZXIC_UINT32 smmu0_int6_mask1;
    ZXIC_UINT32 smmu0_int6_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT6_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int6_status_t
{
    ZXIC_UINT32 smmu0_int6_status31;
    ZXIC_UINT32 smmu0_int6_status30;
    ZXIC_UINT32 smmu0_int6_status29;
    ZXIC_UINT32 smmu0_int6_status28;
    ZXIC_UINT32 smmu0_int6_status27;
    ZXIC_UINT32 smmu0_int6_status26;
    ZXIC_UINT32 smmu0_int6_status25;
    ZXIC_UINT32 smmu0_int6_status24;
    ZXIC_UINT32 smmu0_int6_status23;
    ZXIC_UINT32 smmu0_int6_status22;
    ZXIC_UINT32 smmu0_int6_status21;
    ZXIC_UINT32 smmu0_int6_status20;
    ZXIC_UINT32 smmu0_int6_status19;
    ZXIC_UINT32 smmu0_int6_status18;
    ZXIC_UINT32 smmu0_int6_status17;
    ZXIC_UINT32 smmu0_int6_status16;
    ZXIC_UINT32 smmu0_int6_status15;
    ZXIC_UINT32 smmu0_int6_status14;
    ZXIC_UINT32 smmu0_int6_status13;
    ZXIC_UINT32 smmu0_int6_status12;
    ZXIC_UINT32 smmu0_int6_status11;
    ZXIC_UINT32 smmu0_int6_status10;
    ZXIC_UINT32 smmu0_int6_status9;
    ZXIC_UINT32 smmu0_int6_status8;
    ZXIC_UINT32 smmu0_int6_status7;
    ZXIC_UINT32 smmu0_int6_status6;
    ZXIC_UINT32 smmu0_int6_status5;
    ZXIC_UINT32 smmu0_int6_status4;
    ZXIC_UINT32 smmu0_int6_status3;
    ZXIC_UINT32 smmu0_int6_status2;
    ZXIC_UINT32 smmu0_int6_status1;
    ZXIC_UINT32 smmu0_int6_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT6_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int7_en_t
{
    ZXIC_UINT32 smmu0_int7_en31;
    ZXIC_UINT32 smmu0_int7_en30;
    ZXIC_UINT32 smmu0_int7_en29;
    ZXIC_UINT32 smmu0_int7_en28;
    ZXIC_UINT32 smmu0_int7_en27;
    ZXIC_UINT32 smmu0_int7_en26;
    ZXIC_UINT32 smmu0_int7_en25;
    ZXIC_UINT32 smmu0_int7_en24;
    ZXIC_UINT32 smmu0_int7_en23;
    ZXIC_UINT32 smmu0_int7_en22;
    ZXIC_UINT32 smmu0_int7_en21;
    ZXIC_UINT32 smmu0_int7_en20;
    ZXIC_UINT32 smmu0_int7_en19;
    ZXIC_UINT32 smmu0_int7_en18;
    ZXIC_UINT32 smmu0_int7_en17;
    ZXIC_UINT32 smmu0_int7_en16;
    ZXIC_UINT32 smmu0_int7_en15;
    ZXIC_UINT32 smmu0_int7_en14;
    ZXIC_UINT32 smmu0_int7_en13;
    ZXIC_UINT32 smmu0_int7_en12;
    ZXIC_UINT32 smmu0_int7_en11;
    ZXIC_UINT32 smmu0_int7_en10;
    ZXIC_UINT32 smmu0_int7_en9;
    ZXIC_UINT32 smmu0_int7_en8;
    ZXIC_UINT32 smmu0_int7_en7;
    ZXIC_UINT32 smmu0_int7_en6;
    ZXIC_UINT32 smmu0_int7_en5;
    ZXIC_UINT32 smmu0_int7_en4;
    ZXIC_UINT32 smmu0_int7_en3;
    ZXIC_UINT32 smmu0_int7_en2;
    ZXIC_UINT32 smmu0_int7_en1;
    ZXIC_UINT32 smmu0_int7_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT7_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int7_mask_t
{
    ZXIC_UINT32 smmu0_int7_mask31;
    ZXIC_UINT32 smmu0_int7_mask30;
    ZXIC_UINT32 smmu0_int7_mask29;
    ZXIC_UINT32 smmu0_int7_mask28;
    ZXIC_UINT32 smmu0_int7_mask27;
    ZXIC_UINT32 smmu0_int7_mask26;
    ZXIC_UINT32 smmu0_int7_mask25;
    ZXIC_UINT32 smmu0_int7_mask24;
    ZXIC_UINT32 smmu0_int7_mask23;
    ZXIC_UINT32 smmu0_int7_mask22;
    ZXIC_UINT32 smmu0_int7_mask21;
    ZXIC_UINT32 smmu0_int7_mask20;
    ZXIC_UINT32 smmu0_int7_mask19;
    ZXIC_UINT32 smmu0_int7_mask18;
    ZXIC_UINT32 smmu0_int7_mask17;
    ZXIC_UINT32 smmu0_int7_mask16;
    ZXIC_UINT32 smmu0_int7_mask15;
    ZXIC_UINT32 smmu0_int7_mask14;
    ZXIC_UINT32 smmu0_int7_mask13;
    ZXIC_UINT32 smmu0_int7_mask12;
    ZXIC_UINT32 smmu0_int7_mask11;
    ZXIC_UINT32 smmu0_int7_mask10;
    ZXIC_UINT32 smmu0_int7_mask9;
    ZXIC_UINT32 smmu0_int7_mask8;
    ZXIC_UINT32 smmu0_int7_mask7;
    ZXIC_UINT32 smmu0_int7_mask6;
    ZXIC_UINT32 smmu0_int7_mask5;
    ZXIC_UINT32 smmu0_int7_mask4;
    ZXIC_UINT32 smmu0_int7_mask3;
    ZXIC_UINT32 smmu0_int7_mask2;
    ZXIC_UINT32 smmu0_int7_mask1;
    ZXIC_UINT32 smmu0_int7_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT7_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int7_status_t
{
    ZXIC_UINT32 smmu0_int7_status31;
    ZXIC_UINT32 smmu0_int7_status30;
    ZXIC_UINT32 smmu0_int7_status29;
    ZXIC_UINT32 smmu0_int7_status28;
    ZXIC_UINT32 smmu0_int7_status27;
    ZXIC_UINT32 smmu0_int7_status26;
    ZXIC_UINT32 smmu0_int7_status25;
    ZXIC_UINT32 smmu0_int7_status24;
    ZXIC_UINT32 smmu0_int7_status23;
    ZXIC_UINT32 smmu0_int7_status22;
    ZXIC_UINT32 smmu0_int7_status21;
    ZXIC_UINT32 smmu0_int7_status20;
    ZXIC_UINT32 smmu0_int7_status19;
    ZXIC_UINT32 smmu0_int7_status18;
    ZXIC_UINT32 smmu0_int7_status17;
    ZXIC_UINT32 smmu0_int7_status16;
    ZXIC_UINT32 smmu0_int7_status15;
    ZXIC_UINT32 smmu0_int7_status14;
    ZXIC_UINT32 smmu0_int7_status13;
    ZXIC_UINT32 smmu0_int7_status12;
    ZXIC_UINT32 smmu0_int7_status11;
    ZXIC_UINT32 smmu0_int7_status10;
    ZXIC_UINT32 smmu0_int7_status9;
    ZXIC_UINT32 smmu0_int7_status8;
    ZXIC_UINT32 smmu0_int7_status7;
    ZXIC_UINT32 smmu0_int7_status6;
    ZXIC_UINT32 smmu0_int7_status5;
    ZXIC_UINT32 smmu0_int7_status4;
    ZXIC_UINT32 smmu0_int7_status3;
    ZXIC_UINT32 smmu0_int7_status2;
    ZXIC_UINT32 smmu0_int7_status1;
    ZXIC_UINT32 smmu0_int7_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT7_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int8_en_t
{
    ZXIC_UINT32 smmu0_int8_en31;
    ZXIC_UINT32 smmu0_int8_en30;
    ZXIC_UINT32 smmu0_int8_en29;
    ZXIC_UINT32 smmu0_int8_en28;
    ZXIC_UINT32 smmu0_int8_en27;
    ZXIC_UINT32 smmu0_int8_en26;
    ZXIC_UINT32 smmu0_int8_en25;
    ZXIC_UINT32 smmu0_int8_en24;
    ZXIC_UINT32 smmu0_int8_en23;
    ZXIC_UINT32 smmu0_int8_en22;
    ZXIC_UINT32 smmu0_int8_en21;
    ZXIC_UINT32 smmu0_int8_en20;
    ZXIC_UINT32 smmu0_int8_en19;
    ZXIC_UINT32 smmu0_int8_en18;
    ZXIC_UINT32 smmu0_int8_en17;
    ZXIC_UINT32 smmu0_int8_en16;
    ZXIC_UINT32 smmu0_int8_en15;
    ZXIC_UINT32 smmu0_int8_en14;
    ZXIC_UINT32 smmu0_int8_en13;
    ZXIC_UINT32 smmu0_int8_en12;
    ZXIC_UINT32 smmu0_int8_en11;
    ZXIC_UINT32 smmu0_int8_en10;
    ZXIC_UINT32 smmu0_int8_en9;
    ZXIC_UINT32 smmu0_int8_en8;
    ZXIC_UINT32 smmu0_int8_en7;
    ZXIC_UINT32 smmu0_int8_en6;
    ZXIC_UINT32 smmu0_int8_en5;
    ZXIC_UINT32 smmu0_int8_en4;
    ZXIC_UINT32 smmu0_int8_en3;
    ZXIC_UINT32 smmu0_int8_en2;
    ZXIC_UINT32 smmu0_int8_en1;
    ZXIC_UINT32 smmu0_int8_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT8_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int8_mask_t
{
    ZXIC_UINT32 smmu0_int8_mask31;
    ZXIC_UINT32 smmu0_int8_mask30;
    ZXIC_UINT32 smmu0_int8_mask29;
    ZXIC_UINT32 smmu0_int8_mask28;
    ZXIC_UINT32 smmu0_int8_mask27;
    ZXIC_UINT32 smmu0_int8_mask26;
    ZXIC_UINT32 smmu0_int8_mask25;
    ZXIC_UINT32 smmu0_int8_mask24;
    ZXIC_UINT32 smmu0_int8_mask23;
    ZXIC_UINT32 smmu0_int8_mask22;
    ZXIC_UINT32 smmu0_int8_mask21;
    ZXIC_UINT32 smmu0_int8_mask20;
    ZXIC_UINT32 smmu0_int8_mask19;
    ZXIC_UINT32 smmu0_int8_mask18;
    ZXIC_UINT32 smmu0_int8_mask17;
    ZXIC_UINT32 smmu0_int8_mask16;
    ZXIC_UINT32 smmu0_int8_mask15;
    ZXIC_UINT32 smmu0_int8_mask14;
    ZXIC_UINT32 smmu0_int8_mask13;
    ZXIC_UINT32 smmu0_int8_mask12;
    ZXIC_UINT32 smmu0_int8_mask11;
    ZXIC_UINT32 smmu0_int8_mask10;
    ZXIC_UINT32 smmu0_int8_mask9;
    ZXIC_UINT32 smmu0_int8_mask8;
    ZXIC_UINT32 smmu0_int8_mask7;
    ZXIC_UINT32 smmu0_int8_mask6;
    ZXIC_UINT32 smmu0_int8_mask5;
    ZXIC_UINT32 smmu0_int8_mask4;
    ZXIC_UINT32 smmu0_int8_mask3;
    ZXIC_UINT32 smmu0_int8_mask2;
    ZXIC_UINT32 smmu0_int8_mask1;
    ZXIC_UINT32 smmu0_int8_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT8_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int8_status_t
{
    ZXIC_UINT32 smmu0_int8_status31;
    ZXIC_UINT32 smmu0_int8_status30;
    ZXIC_UINT32 smmu0_int8_status29;
    ZXIC_UINT32 smmu0_int8_status28;
    ZXIC_UINT32 smmu0_int8_status27;
    ZXIC_UINT32 smmu0_int8_status26;
    ZXIC_UINT32 smmu0_int8_status25;
    ZXIC_UINT32 smmu0_int8_status24;
    ZXIC_UINT32 smmu0_int8_status23;
    ZXIC_UINT32 smmu0_int8_status22;
    ZXIC_UINT32 smmu0_int8_status21;
    ZXIC_UINT32 smmu0_int8_status20;
    ZXIC_UINT32 smmu0_int8_status19;
    ZXIC_UINT32 smmu0_int8_status18;
    ZXIC_UINT32 smmu0_int8_status17;
    ZXIC_UINT32 smmu0_int8_status16;
    ZXIC_UINT32 smmu0_int8_status15;
    ZXIC_UINT32 smmu0_int8_status14;
    ZXIC_UINT32 smmu0_int8_status13;
    ZXIC_UINT32 smmu0_int8_status12;
    ZXIC_UINT32 smmu0_int8_status11;
    ZXIC_UINT32 smmu0_int8_status10;
    ZXIC_UINT32 smmu0_int8_status9;
    ZXIC_UINT32 smmu0_int8_status8;
    ZXIC_UINT32 smmu0_int8_status7;
    ZXIC_UINT32 smmu0_int8_status6;
    ZXIC_UINT32 smmu0_int8_status5;
    ZXIC_UINT32 smmu0_int8_status4;
    ZXIC_UINT32 smmu0_int8_status3;
    ZXIC_UINT32 smmu0_int8_status2;
    ZXIC_UINT32 smmu0_int8_status1;
    ZXIC_UINT32 smmu0_int8_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT8_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int9_en_t
{
    ZXIC_UINT32 smmu0_int8_en31;
    ZXIC_UINT32 smmu0_int8_en30;
    ZXIC_UINT32 smmu0_int8_en29;
    ZXIC_UINT32 smmu0_int8_en28;
    ZXIC_UINT32 smmu0_int8_en27;
    ZXIC_UINT32 smmu0_int8_en26;
    ZXIC_UINT32 smmu0_int8_en25;
    ZXIC_UINT32 smmu0_int8_en24;
    ZXIC_UINT32 smmu0_int8_en23;
    ZXIC_UINT32 smmu0_int8_en22;
    ZXIC_UINT32 smmu0_int8_en21;
    ZXIC_UINT32 smmu0_int8_en20;
    ZXIC_UINT32 smmu0_int9_en19;
    ZXIC_UINT32 smmu0_int9_en18;
    ZXIC_UINT32 smmu0_int9_en17;
    ZXIC_UINT32 smmu0_int9_en16;
    ZXIC_UINT32 smmu0_int9_en15;
    ZXIC_UINT32 smmu0_int9_en14;
    ZXIC_UINT32 smmu0_int9_en13;
    ZXIC_UINT32 smmu0_int9_en12;
    ZXIC_UINT32 smmu0_int9_en11;
    ZXIC_UINT32 smmu0_int9_en10;
    ZXIC_UINT32 smmu0_int9_en9;
    ZXIC_UINT32 smmu0_int9_en8;
    ZXIC_UINT32 smmu0_int9_en7;
    ZXIC_UINT32 smmu0_int9_en6;
    ZXIC_UINT32 smmu0_int9_en5;
    ZXIC_UINT32 smmu0_int9_en4;
    ZXIC_UINT32 smmu0_int9_en3;
    ZXIC_UINT32 smmu0_int9_en2;
    ZXIC_UINT32 smmu0_int9_en1;
    ZXIC_UINT32 smmu0_int9_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT9_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int9_mask_t
{
    ZXIC_UINT32 smmu0_int9_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT9_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int9_status_t
{
    ZXIC_UINT32 smmu0_int9_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT9_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int10_en_t
{
    ZXIC_UINT32 smmu0_int10_en31;
    ZXIC_UINT32 smmu0_int10_en30;
    ZXIC_UINT32 smmu0_int10_en29;
    ZXIC_UINT32 smmu0_int10_en28;
    ZXIC_UINT32 smmu0_int10_en27;
    ZXIC_UINT32 smmu0_int10_en26;
    ZXIC_UINT32 smmu0_int10_en25;
    ZXIC_UINT32 smmu0_int10_en24;
    ZXIC_UINT32 smmu0_int10_en23;
    ZXIC_UINT32 smmu0_int10_en22;
    ZXIC_UINT32 smmu0_int10_en21;
    ZXIC_UINT32 smmu0_int10_en20;
    ZXIC_UINT32 smmu0_int10_en19;
    ZXIC_UINT32 smmu0_int10_en18;
    ZXIC_UINT32 smmu0_int10_en17;
    ZXIC_UINT32 smmu0_int10_en16;
    ZXIC_UINT32 smmu0_int10_en15;
    ZXIC_UINT32 smmu0_int10_en14;
    ZXIC_UINT32 smmu0_int10_en13;
    ZXIC_UINT32 smmu0_int10_en12;
    ZXIC_UINT32 smmu0_int10_en11;
    ZXIC_UINT32 smmu0_int10_en10;
    ZXIC_UINT32 smmu0_int10_en9;
    ZXIC_UINT32 smmu0_int10_en8;
    ZXIC_UINT32 smmu0_int10_en7;
    ZXIC_UINT32 smmu0_int10_en6;
    ZXIC_UINT32 smmu0_int10_en5;
    ZXIC_UINT32 smmu0_int10_en4;
    ZXIC_UINT32 smmu0_int10_en3;
    ZXIC_UINT32 smmu0_int10_en2;
    ZXIC_UINT32 smmu0_int10_en1;
    ZXIC_UINT32 smmu0_int10_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT10_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int10_mask_t
{
    ZXIC_UINT32 smmu0_int10_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT10_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int10_status_t
{
    ZXIC_UINT32 smmu0_int10_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT10_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int11_en_t
{
    ZXIC_UINT32 smmu0_int11_en0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT11_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int11_mask_t
{
    ZXIC_UINT32 smmu0_int11_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT11_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int11_status_t
{
    ZXIC_UINT32 smmu0_int11_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT11_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int12_en_t
{
    ZXIC_UINT32 smmu0_int12_en0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT12_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int12_mask_t
{
    ZXIC_UINT32 smmu0_int12_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT12_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int12_status_t
{
    ZXIC_UINT32 smmu0_int12_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT12_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int13_en_t
{
    ZXIC_UINT32 smmu0_int13_en19;
    ZXIC_UINT32 smmu0_int13_en18;
    ZXIC_UINT32 smmu0_int13_en17;
    ZXIC_UINT32 smmu0_int13_en16;
    ZXIC_UINT32 smmu0_int13_en15;
    ZXIC_UINT32 smmu0_int13_en14;
    ZXIC_UINT32 smmu0_int13_en13;
    ZXIC_UINT32 smmu0_int13_en12;
    ZXIC_UINT32 smmu0_int13_en11;
    ZXIC_UINT32 smmu0_int13_en10;
    ZXIC_UINT32 smmu0_int13_en9;
    ZXIC_UINT32 smmu0_int13_en8;
    ZXIC_UINT32 smmu0_int13_en7;
    ZXIC_UINT32 smmu0_int13_en6;
    ZXIC_UINT32 smmu0_int13_en5;
    ZXIC_UINT32 smmu0_int13_en4;
    ZXIC_UINT32 smmu0_int13_en3;
    ZXIC_UINT32 smmu0_int13_en2;
    ZXIC_UINT32 smmu0_int13_en1;
    ZXIC_UINT32 smmu0_int13_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT13_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int13_mask_t
{
    ZXIC_UINT32 smmu0_int13_mask19;
    ZXIC_UINT32 smmu0_int13_mask18;
    ZXIC_UINT32 smmu0_int13_mask17;
    ZXIC_UINT32 smmu0_int13_mask16;
    ZXIC_UINT32 smmu0_int13_mask15;
    ZXIC_UINT32 smmu0_int13_mask14;
    ZXIC_UINT32 smmu0_int13_mask13;
    ZXIC_UINT32 smmu0_int13_mask12;
    ZXIC_UINT32 smmu0_int13_mask11;
    ZXIC_UINT32 smmu0_int13_mask10;
    ZXIC_UINT32 smmu0_int13_mask9;
    ZXIC_UINT32 smmu0_int13_mask8;
    ZXIC_UINT32 smmu0_int13_mask7;
    ZXIC_UINT32 smmu0_int13_mask6;
    ZXIC_UINT32 smmu0_int13_mask5;
    ZXIC_UINT32 smmu0_int13_mask4;
    ZXIC_UINT32 smmu0_int13_mask3;
    ZXIC_UINT32 smmu0_int13_mask2;
    ZXIC_UINT32 smmu0_int13_mask1;
    ZXIC_UINT32 smmu0_int13_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT13_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int13_status_t
{
    ZXIC_UINT32 smmu0_int13_status19;
    ZXIC_UINT32 smmu0_int13_status18;
    ZXIC_UINT32 smmu0_int13_status17;
    ZXIC_UINT32 smmu0_int13_status16;
    ZXIC_UINT32 smmu0_int13_status15;
    ZXIC_UINT32 smmu0_int13_status14;
    ZXIC_UINT32 smmu0_int13_status13;
    ZXIC_UINT32 smmu0_int13_status12;
    ZXIC_UINT32 smmu0_int13_status11;
    ZXIC_UINT32 smmu0_int13_status10;
    ZXIC_UINT32 smmu0_int13_status9;
    ZXIC_UINT32 smmu0_int13_status8;
    ZXIC_UINT32 smmu0_int13_status7;
    ZXIC_UINT32 smmu0_int13_status6;
    ZXIC_UINT32 smmu0_int13_status5;
    ZXIC_UINT32 smmu0_int13_status4;
    ZXIC_UINT32 smmu0_int13_status3;
    ZXIC_UINT32 smmu0_int13_status2;
    ZXIC_UINT32 smmu0_int13_status1;
    ZXIC_UINT32 smmu0_int13_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT13_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int14_en_t
{
    ZXIC_UINT32 smmu0_int14_en16;
    ZXIC_UINT32 smmu0_int14_en15;
    ZXIC_UINT32 smmu0_int14_en14;
    ZXIC_UINT32 smmu0_int14_en13;
    ZXIC_UINT32 smmu0_int14_en12;
    ZXIC_UINT32 smmu0_int14_en11;
    ZXIC_UINT32 smmu0_int14_en10;
    ZXIC_UINT32 smmu0_int14_en9;
    ZXIC_UINT32 smmu0_int14_en8;
    ZXIC_UINT32 smmu0_int14_en7;
    ZXIC_UINT32 smmu0_int14_en6;
    ZXIC_UINT32 smmu0_int14_en5;
    ZXIC_UINT32 smmu0_int14_en4;
    ZXIC_UINT32 smmu0_int14_en3;
    ZXIC_UINT32 smmu0_int14_en2;
    ZXIC_UINT32 smmu0_int14_en1;
    ZXIC_UINT32 smmu0_int14_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT14_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int14_mask_t
{
    ZXIC_UINT32 smmu0_int14_mask16;
    ZXIC_UINT32 smmu0_int14_mask15;
    ZXIC_UINT32 smmu0_int14_mask14;
    ZXIC_UINT32 smmu0_int14_mask13;
    ZXIC_UINT32 smmu0_int14_mask12;
    ZXIC_UINT32 smmu0_int14_mask11;
    ZXIC_UINT32 smmu0_int14_mask10;
    ZXIC_UINT32 smmu0_int14_mask9;
    ZXIC_UINT32 smmu0_int14_mask8;
    ZXIC_UINT32 smmu0_int14_mask7;
    ZXIC_UINT32 smmu0_int14_mask6;
    ZXIC_UINT32 smmu0_int14_mask5;
    ZXIC_UINT32 smmu0_int14_mask4;
    ZXIC_UINT32 smmu0_int14_mask3;
    ZXIC_UINT32 smmu0_int14_mask2;
    ZXIC_UINT32 smmu0_int14_mask1;
    ZXIC_UINT32 smmu0_int14_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT14_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int14_status_t
{
    ZXIC_UINT32 smmu0_int14_status16;
    ZXIC_UINT32 smmu0_int14_status15;
    ZXIC_UINT32 smmu0_int14_status14;
    ZXIC_UINT32 smmu0_int14_status13;
    ZXIC_UINT32 smmu0_int14_status12;
    ZXIC_UINT32 smmu0_int14_status11;
    ZXIC_UINT32 smmu0_int14_status10;
    ZXIC_UINT32 smmu0_int14_status9;
    ZXIC_UINT32 smmu0_int14_status8;
    ZXIC_UINT32 smmu0_int14_status7;
    ZXIC_UINT32 smmu0_int14_status6;
    ZXIC_UINT32 smmu0_int14_status5;
    ZXIC_UINT32 smmu0_int14_status4;
    ZXIC_UINT32 smmu0_int14_status3;
    ZXIC_UINT32 smmu0_int14_status2;
    ZXIC_UINT32 smmu0_int14_status1;
    ZXIC_UINT32 smmu0_int14_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT14_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_ecc_unmask_flag_t
{
    ZXIC_UINT32 smmu0_int53_unmask_flag;
    ZXIC_UINT32 smmu0_int52_unmask_flag;
    ZXIC_UINT32 smmu0_int51_unmask_flag;
    ZXIC_UINT32 smmu0_int50_unmask_flag;
    ZXIC_UINT32 smmu0_int49_unmask_flag;
    ZXIC_UINT32 smmu0_int48_unmask_flag;
    ZXIC_UINT32 smmu0_int47_unmask_flag;
    ZXIC_UINT32 smmu0_int46_unmask_flag;
    ZXIC_UINT32 smmu0_int45_unmask_flag;
    ZXIC_UINT32 smmu0_int44_unmask_flag;
    ZXIC_UINT32 smmu0_int43_unmask_flag;
    ZXIC_UINT32 smmu0_int42_unmask_flag;
    ZXIC_UINT32 smmu0_int41_unmask_flag;
    ZXIC_UINT32 smmu0_int40_unmask_flag;
    ZXIC_UINT32 smmu0_int39_unmask_flag;
    ZXIC_UINT32 smmu0_int38_unmask_flag;
    ZXIC_UINT32 smmu0_int37_unmask_flag;
    ZXIC_UINT32 smmu0_int36_unmask_flag;
    ZXIC_UINT32 smmu0_int35_unmask_flag;
    ZXIC_UINT32 smmu0_int34_unmask_flag;
    ZXIC_UINT32 smmu0_int33_unmask_flag;
    ZXIC_UINT32 smmu0_int32_unmask_flag;
}DPP_SMMU0_SMMU0_SMMU0_ECC_UNMASK_FLAG_T;

typedef struct dpp_smmu0_smmu0_smmu0_int15_en_t
{
    ZXIC_UINT32 smmu0_int15_en31;
    ZXIC_UINT32 smmu0_int15_en30;
    ZXIC_UINT32 smmu0_int15_en29;
    ZXIC_UINT32 smmu0_int15_en28;
    ZXIC_UINT32 smmu0_int15_en27;
    ZXIC_UINT32 smmu0_int15_en26;
    ZXIC_UINT32 smmu0_int15_en25;
    ZXIC_UINT32 smmu0_int15_en24;
    ZXIC_UINT32 smmu0_int15_en23;
    ZXIC_UINT32 smmu0_int15_en22;
    ZXIC_UINT32 smmu0_int15_en21;
    ZXIC_UINT32 smmu0_int15_en20;
    ZXIC_UINT32 smmu0_int15_en19;
    ZXIC_UINT32 smmu0_int15_en18;
    ZXIC_UINT32 smmu0_int15_en17;
    ZXIC_UINT32 smmu0_int15_en16;
    ZXIC_UINT32 smmu0_int15_en15;
    ZXIC_UINT32 smmu0_int15_en14;
    ZXIC_UINT32 smmu0_int15_en13;
    ZXIC_UINT32 smmu0_int15_en12;
    ZXIC_UINT32 smmu0_int15_en11;
    ZXIC_UINT32 smmu0_int15_en10;
    ZXIC_UINT32 smmu0_int15_en9;
    ZXIC_UINT32 smmu0_int15_en8;
    ZXIC_UINT32 smmu0_int15_en7;
    ZXIC_UINT32 smmu0_int15_en6;
    ZXIC_UINT32 smmu0_int15_en5;
    ZXIC_UINT32 smmu0_int15_en4;
    ZXIC_UINT32 smmu0_int15_en3;
    ZXIC_UINT32 smmu0_int15_en2;
    ZXIC_UINT32 smmu0_int15_en1;
    ZXIC_UINT32 smmu0_int15_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT15_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int15_mask_t
{
    ZXIC_UINT32 smmu0_int15_mask31;
    ZXIC_UINT32 smmu0_int15_mask30;
    ZXIC_UINT32 smmu0_int15_mask29;
    ZXIC_UINT32 smmu0_int15_mask28;
    ZXIC_UINT32 smmu0_int15_mask27;
    ZXIC_UINT32 smmu0_int15_mask26;
    ZXIC_UINT32 smmu0_int15_mask25;
    ZXIC_UINT32 smmu0_int15_mask24;
    ZXIC_UINT32 smmu0_int15_mask23;
    ZXIC_UINT32 smmu0_int15_mask22;
    ZXIC_UINT32 smmu0_int15_mask21;
    ZXIC_UINT32 smmu0_int15_mask20;
    ZXIC_UINT32 smmu0_int15_mask19;
    ZXIC_UINT32 smmu0_int15_mask18;
    ZXIC_UINT32 smmu0_int15_mask17;
    ZXIC_UINT32 smmu0_int15_mask16;
    ZXIC_UINT32 smmu0_int15_mask15;
    ZXIC_UINT32 smmu0_int15_mask14;
    ZXIC_UINT32 smmu0_int15_mask13;
    ZXIC_UINT32 smmu0_int15_mask12;
    ZXIC_UINT32 smmu0_int15_mask11;
    ZXIC_UINT32 smmu0_int15_mask10;
    ZXIC_UINT32 smmu0_int15_mask9;
    ZXIC_UINT32 smmu0_int15_mask8;
    ZXIC_UINT32 smmu0_int15_mask7;
    ZXIC_UINT32 smmu0_int15_mask6;
    ZXIC_UINT32 smmu0_int15_mask5;
    ZXIC_UINT32 smmu0_int15_mask4;
    ZXIC_UINT32 smmu0_int15_mask3;
    ZXIC_UINT32 smmu0_int15_mask2;
    ZXIC_UINT32 smmu0_int15_mask1;
    ZXIC_UINT32 smmu0_int15_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT15_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int15_status_t
{
    ZXIC_UINT32 smmu0_int15_status31;
    ZXIC_UINT32 smmu0_int15_status30;
    ZXIC_UINT32 smmu0_int15_status29;
    ZXIC_UINT32 smmu0_int15_status28;
    ZXIC_UINT32 smmu0_int15_status27;
    ZXIC_UINT32 smmu0_int15_status26;
    ZXIC_UINT32 smmu0_int15_status25;
    ZXIC_UINT32 smmu0_int15_status24;
    ZXIC_UINT32 smmu0_int15_status23;
    ZXIC_UINT32 smmu0_int15_status22;
    ZXIC_UINT32 smmu0_int15_status21;
    ZXIC_UINT32 smmu0_int15_status20;
    ZXIC_UINT32 smmu0_int15_status19;
    ZXIC_UINT32 smmu0_int15_status18;
    ZXIC_UINT32 smmu0_int15_status17;
    ZXIC_UINT32 smmu0_int15_status16;
    ZXIC_UINT32 smmu0_int15_status15;
    ZXIC_UINT32 smmu0_int15_status14;
    ZXIC_UINT32 smmu0_int15_status13;
    ZXIC_UINT32 smmu0_int15_status12;
    ZXIC_UINT32 smmu0_int15_status11;
    ZXIC_UINT32 smmu0_int15_status10;
    ZXIC_UINT32 smmu0_int15_status9;
    ZXIC_UINT32 smmu0_int15_status8;
    ZXIC_UINT32 smmu0_int15_status7;
    ZXIC_UINT32 smmu0_int15_status6;
    ZXIC_UINT32 smmu0_int15_status5;
    ZXIC_UINT32 smmu0_int15_status4;
    ZXIC_UINT32 smmu0_int15_status3;
    ZXIC_UINT32 smmu0_int15_status2;
    ZXIC_UINT32 smmu0_int15_status1;
    ZXIC_UINT32 smmu0_int15_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT15_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int16_en_t
{
    ZXIC_UINT32 smmu0_int16_en31;
    ZXIC_UINT32 smmu0_int16_en30;
    ZXIC_UINT32 smmu0_int16_en29;
    ZXIC_UINT32 smmu0_int16_en28;
    ZXIC_UINT32 smmu0_int16_en27;
    ZXIC_UINT32 smmu0_int16_en26;
    ZXIC_UINT32 smmu0_int16_en25;
    ZXIC_UINT32 smmu0_int16_en24;
    ZXIC_UINT32 smmu0_int16_en23;
    ZXIC_UINT32 smmu0_int16_en22;
    ZXIC_UINT32 smmu0_int16_en21;
    ZXIC_UINT32 smmu0_int16_en20;
    ZXIC_UINT32 smmu0_int16_en19;
    ZXIC_UINT32 smmu0_int16_en18;
    ZXIC_UINT32 smmu0_int16_en17;
    ZXIC_UINT32 smmu0_int16_en16;
    ZXIC_UINT32 smmu0_int16_en15;
    ZXIC_UINT32 smmu0_int16_en14;
    ZXIC_UINT32 smmu0_int16_en13;
    ZXIC_UINT32 smmu0_int16_en12;
    ZXIC_UINT32 smmu0_int16_en11;
    ZXIC_UINT32 smmu0_int16_en10;
    ZXIC_UINT32 smmu0_int16_en9;
    ZXIC_UINT32 smmu0_int16_en8;
    ZXIC_UINT32 smmu0_int16_en7;
    ZXIC_UINT32 smmu0_int16_en6;
    ZXIC_UINT32 smmu0_int16_en5;
    ZXIC_UINT32 smmu0_int16_en4;
    ZXIC_UINT32 smmu0_int16_en3;
    ZXIC_UINT32 smmu0_int16_en2;
    ZXIC_UINT32 smmu0_int16_en1;
    ZXIC_UINT32 smmu0_int16_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT16_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int16_mask_t
{
    ZXIC_UINT32 smmu0_int16_mask31;
    ZXIC_UINT32 smmu0_int16_mask30;
    ZXIC_UINT32 smmu0_int16_mask29;
    ZXIC_UINT32 smmu0_int16_mask28;
    ZXIC_UINT32 smmu0_int16_mask27;
    ZXIC_UINT32 smmu0_int16_mask26;
    ZXIC_UINT32 smmu0_int16_mask25;
    ZXIC_UINT32 smmu0_int16_mask24;
    ZXIC_UINT32 smmu0_int16_mask23;
    ZXIC_UINT32 smmu0_int16_mask22;
    ZXIC_UINT32 smmu0_int16_mask21;
    ZXIC_UINT32 smmu0_int16_mask20;
    ZXIC_UINT32 smmu0_int16_mask19;
    ZXIC_UINT32 smmu0_int16_mask18;
    ZXIC_UINT32 smmu0_int16_mask17;
    ZXIC_UINT32 smmu0_int16_mask16;
    ZXIC_UINT32 smmu0_int16_mask15;
    ZXIC_UINT32 smmu0_int16_mask14;
    ZXIC_UINT32 smmu0_int16_mask13;
    ZXIC_UINT32 smmu0_int16_mask12;
    ZXIC_UINT32 smmu0_int16_mask11;
    ZXIC_UINT32 smmu0_int16_mask10;
    ZXIC_UINT32 smmu0_int16_mask9;
    ZXIC_UINT32 smmu0_int16_mask8;
    ZXIC_UINT32 smmu0_int16_mask7;
    ZXIC_UINT32 smmu0_int16_mask6;
    ZXIC_UINT32 smmu0_int16_mask5;
    ZXIC_UINT32 smmu0_int16_mask4;
    ZXIC_UINT32 smmu0_int16_mask3;
    ZXIC_UINT32 smmu0_int16_mask2;
    ZXIC_UINT32 smmu0_int16_mask1;
    ZXIC_UINT32 smmu0_int16_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT16_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int16_status_t
{
    ZXIC_UINT32 smmu0_int16_status31;
    ZXIC_UINT32 smmu0_int16_status30;
    ZXIC_UINT32 smmu0_int16_status29;
    ZXIC_UINT32 smmu0_int16_status28;
    ZXIC_UINT32 smmu0_int16_status27;
    ZXIC_UINT32 smmu0_int16_status26;
    ZXIC_UINT32 smmu0_int16_status25;
    ZXIC_UINT32 smmu0_int16_status24;
    ZXIC_UINT32 smmu0_int16_status23;
    ZXIC_UINT32 smmu0_int16_status22;
    ZXIC_UINT32 smmu0_int16_status21;
    ZXIC_UINT32 smmu0_int16_status20;
    ZXIC_UINT32 smmu0_int16_status19;
    ZXIC_UINT32 smmu0_int16_status18;
    ZXIC_UINT32 smmu0_int16_status17;
    ZXIC_UINT32 smmu0_int16_status16;
    ZXIC_UINT32 smmu0_int16_status15;
    ZXIC_UINT32 smmu0_int16_status14;
    ZXIC_UINT32 smmu0_int16_status13;
    ZXIC_UINT32 smmu0_int16_status12;
    ZXIC_UINT32 smmu0_int16_status11;
    ZXIC_UINT32 smmu0_int16_status10;
    ZXIC_UINT32 smmu0_int16_status9;
    ZXIC_UINT32 smmu0_int16_status8;
    ZXIC_UINT32 smmu0_int16_status7;
    ZXIC_UINT32 smmu0_int16_status6;
    ZXIC_UINT32 smmu0_int16_status5;
    ZXIC_UINT32 smmu0_int16_status4;
    ZXIC_UINT32 smmu0_int16_status3;
    ZXIC_UINT32 smmu0_int16_status2;
    ZXIC_UINT32 smmu0_int16_status1;
    ZXIC_UINT32 smmu0_int16_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT16_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int17_en_t
{
    ZXIC_UINT32 smmu0_int17_en31;
    ZXIC_UINT32 smmu0_int17_en30;
    ZXIC_UINT32 smmu0_int17_en29;
    ZXIC_UINT32 smmu0_int17_en28;
    ZXIC_UINT32 smmu0_int17_en27;
    ZXIC_UINT32 smmu0_int17_en26;
    ZXIC_UINT32 smmu0_int17_en25;
    ZXIC_UINT32 smmu0_int17_en24;
    ZXIC_UINT32 smmu0_int17_en23;
    ZXIC_UINT32 smmu0_int17_en22;
    ZXIC_UINT32 smmu0_int17_en21;
    ZXIC_UINT32 smmu0_int17_en20;
    ZXIC_UINT32 smmu0_int17_en19;
    ZXIC_UINT32 smmu0_int17_en18;
    ZXIC_UINT32 smmu0_int17_en17;
    ZXIC_UINT32 smmu0_int17_en16;
    ZXIC_UINT32 smmu0_int17_en15;
    ZXIC_UINT32 smmu0_int17_en14;
    ZXIC_UINT32 smmu0_int17_en13;
    ZXIC_UINT32 smmu0_int17_en12;
    ZXIC_UINT32 smmu0_int17_en11;
    ZXIC_UINT32 smmu0_int17_en10;
    ZXIC_UINT32 smmu0_int17_en9;
    ZXIC_UINT32 smmu0_int17_en8;
    ZXIC_UINT32 smmu0_int17_en7;
    ZXIC_UINT32 smmu0_int17_en6;
    ZXIC_UINT32 smmu0_int17_en5;
    ZXIC_UINT32 smmu0_int17_en4;
    ZXIC_UINT32 smmu0_int17_en3;
    ZXIC_UINT32 smmu0_int17_en2;
    ZXIC_UINT32 smmu0_int17_en1;
    ZXIC_UINT32 smmu0_int17_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT17_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int17_mask_t
{
    ZXIC_UINT32 smmu0_int17_mask31;
    ZXIC_UINT32 smmu0_int17_mask30;
    ZXIC_UINT32 smmu0_int17_mask29;
    ZXIC_UINT32 smmu0_int17_mask28;
    ZXIC_UINT32 smmu0_int17_mask27;
    ZXIC_UINT32 smmu0_int17_mask26;
    ZXIC_UINT32 smmu0_int17_mask25;
    ZXIC_UINT32 smmu0_int17_mask24;
    ZXIC_UINT32 smmu0_int17_mask23;
    ZXIC_UINT32 smmu0_int17_mask22;
    ZXIC_UINT32 smmu0_int17_mask21;
    ZXIC_UINT32 smmu0_int17_mask20;
    ZXIC_UINT32 smmu0_int17_mask19;
    ZXIC_UINT32 smmu0_int17_mask18;
    ZXIC_UINT32 smmu0_int17_mask17;
    ZXIC_UINT32 smmu0_int17_mask16;
    ZXIC_UINT32 smmu0_int17_mask15;
    ZXIC_UINT32 smmu0_int17_mask14;
    ZXIC_UINT32 smmu0_int17_mask13;
    ZXIC_UINT32 smmu0_int17_mask12;
    ZXIC_UINT32 smmu0_int17_mask11;
    ZXIC_UINT32 smmu0_int17_mask10;
    ZXIC_UINT32 smmu0_int17_mask9;
    ZXIC_UINT32 smmu0_int17_mask8;
    ZXIC_UINT32 smmu0_int17_mask7;
    ZXIC_UINT32 smmu0_int17_mask6;
    ZXIC_UINT32 smmu0_int17_mask5;
    ZXIC_UINT32 smmu0_int17_mask4;
    ZXIC_UINT32 smmu0_int17_mask3;
    ZXIC_UINT32 smmu0_int17_mask2;
    ZXIC_UINT32 smmu0_int17_mask1;
    ZXIC_UINT32 smmu0_int17_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT17_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int17_status_t
{
    ZXIC_UINT32 smmu0_int17_status31;
    ZXIC_UINT32 smmu0_int17_status30;
    ZXIC_UINT32 smmu0_int17_status29;
    ZXIC_UINT32 smmu0_int17_status28;
    ZXIC_UINT32 smmu0_int17_status27;
    ZXIC_UINT32 smmu0_int17_status26;
    ZXIC_UINT32 smmu0_int17_status25;
    ZXIC_UINT32 smmu0_int17_status24;
    ZXIC_UINT32 smmu0_int17_status23;
    ZXIC_UINT32 smmu0_int17_status22;
    ZXIC_UINT32 smmu0_int17_status21;
    ZXIC_UINT32 smmu0_int17_status20;
    ZXIC_UINT32 smmu0_int17_status19;
    ZXIC_UINT32 smmu0_int17_status18;
    ZXIC_UINT32 smmu0_int17_status17;
    ZXIC_UINT32 smmu0_int17_status16;
    ZXIC_UINT32 smmu0_int17_status15;
    ZXIC_UINT32 smmu0_int17_status14;
    ZXIC_UINT32 smmu0_int17_status13;
    ZXIC_UINT32 smmu0_int17_status12;
    ZXIC_UINT32 smmu0_int17_status11;
    ZXIC_UINT32 smmu0_int17_status10;
    ZXIC_UINT32 smmu0_int17_status9;
    ZXIC_UINT32 smmu0_int17_status8;
    ZXIC_UINT32 smmu0_int17_status7;
    ZXIC_UINT32 smmu0_int17_status6;
    ZXIC_UINT32 smmu0_int17_status5;
    ZXIC_UINT32 smmu0_int17_status4;
    ZXIC_UINT32 smmu0_int17_status3;
    ZXIC_UINT32 smmu0_int17_status2;
    ZXIC_UINT32 smmu0_int17_status1;
    ZXIC_UINT32 smmu0_int17_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT17_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int18_en_t
{
    ZXIC_UINT32 smmu0_int18_en31;
    ZXIC_UINT32 smmu0_int18_en30;
    ZXIC_UINT32 smmu0_int18_en29;
    ZXIC_UINT32 smmu0_int18_en28;
    ZXIC_UINT32 smmu0_int18_en27;
    ZXIC_UINT32 smmu0_int18_en26;
    ZXIC_UINT32 smmu0_int18_en25;
    ZXIC_UINT32 smmu0_int18_en24;
    ZXIC_UINT32 smmu0_int18_en23;
    ZXIC_UINT32 smmu0_int18_en22;
    ZXIC_UINT32 smmu0_int18_en21;
    ZXIC_UINT32 smmu0_int18_en20;
    ZXIC_UINT32 smmu0_int18_en19;
    ZXIC_UINT32 smmu0_int18_en18;
    ZXIC_UINT32 smmu0_int18_en17;
    ZXIC_UINT32 smmu0_int18_en16;
    ZXIC_UINT32 smmu0_int18_en15;
    ZXIC_UINT32 smmu0_int18_en14;
    ZXIC_UINT32 smmu0_int18_en13;
    ZXIC_UINT32 smmu0_int18_en12;
    ZXIC_UINT32 smmu0_int18_en11;
    ZXIC_UINT32 smmu0_int18_en10;
    ZXIC_UINT32 smmu0_int18_en9;
    ZXIC_UINT32 smmu0_int18_en8;
    ZXIC_UINT32 smmu0_int18_en7;
    ZXIC_UINT32 smmu0_int18_en6;
    ZXIC_UINT32 smmu0_int18_en5;
    ZXIC_UINT32 smmu0_int18_en4;
    ZXIC_UINT32 smmu0_int18_en3;
    ZXIC_UINT32 smmu0_int18_en2;
    ZXIC_UINT32 smmu0_int18_en1;
    ZXIC_UINT32 smmu0_int18_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT18_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int18_mask_t
{
    ZXIC_UINT32 smmu0_int18_mask31;
    ZXIC_UINT32 smmu0_int18_mask30;
    ZXIC_UINT32 smmu0_int18_mask29;
    ZXIC_UINT32 smmu0_int18_mask28;
    ZXIC_UINT32 smmu0_int18_mask27;
    ZXIC_UINT32 smmu0_int18_mask26;
    ZXIC_UINT32 smmu0_int18_mask25;
    ZXIC_UINT32 smmu0_int18_mask24;
    ZXIC_UINT32 smmu0_int18_mask23;
    ZXIC_UINT32 smmu0_int18_mask22;
    ZXIC_UINT32 smmu0_int18_mask21;
    ZXIC_UINT32 smmu0_int18_mask20;
    ZXIC_UINT32 smmu0_int18_mask19;
    ZXIC_UINT32 smmu0_int18_mask18;
    ZXIC_UINT32 smmu0_int18_mask17;
    ZXIC_UINT32 smmu0_int18_mask16;
    ZXIC_UINT32 smmu0_int18_mask15;
    ZXIC_UINT32 smmu0_int18_mask14;
    ZXIC_UINT32 smmu0_int18_mask13;
    ZXIC_UINT32 smmu0_int18_mask12;
    ZXIC_UINT32 smmu0_int18_mask11;
    ZXIC_UINT32 smmu0_int18_mask10;
    ZXIC_UINT32 smmu0_int18_mask9;
    ZXIC_UINT32 smmu0_int18_mask8;
    ZXIC_UINT32 smmu0_int18_mask7;
    ZXIC_UINT32 smmu0_int18_mask6;
    ZXIC_UINT32 smmu0_int18_mask5;
    ZXIC_UINT32 smmu0_int18_mask4;
    ZXIC_UINT32 smmu0_int18_mask3;
    ZXIC_UINT32 smmu0_int18_mask2;
    ZXIC_UINT32 smmu0_int18_mask1;
    ZXIC_UINT32 smmu0_int18_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT18_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int18_status_t
{
    ZXIC_UINT32 smmu0_int18_status31;
    ZXIC_UINT32 smmu0_int18_status30;
    ZXIC_UINT32 smmu0_int18_status29;
    ZXIC_UINT32 smmu0_int18_status28;
    ZXIC_UINT32 smmu0_int18_status27;
    ZXIC_UINT32 smmu0_int18_status26;
    ZXIC_UINT32 smmu0_int18_status25;
    ZXIC_UINT32 smmu0_int18_status24;
    ZXIC_UINT32 smmu0_int18_status23;
    ZXIC_UINT32 smmu0_int18_status22;
    ZXIC_UINT32 smmu0_int18_status21;
    ZXIC_UINT32 smmu0_int18_status20;
    ZXIC_UINT32 smmu0_int18_status19;
    ZXIC_UINT32 smmu0_int18_status18;
    ZXIC_UINT32 smmu0_int18_status17;
    ZXIC_UINT32 smmu0_int18_status16;
    ZXIC_UINT32 smmu0_int18_status15;
    ZXIC_UINT32 smmu0_int18_status14;
    ZXIC_UINT32 smmu0_int18_status13;
    ZXIC_UINT32 smmu0_int18_status12;
    ZXIC_UINT32 smmu0_int18_status11;
    ZXIC_UINT32 smmu0_int18_status10;
    ZXIC_UINT32 smmu0_int18_status9;
    ZXIC_UINT32 smmu0_int18_status8;
    ZXIC_UINT32 smmu0_int18_status7;
    ZXIC_UINT32 smmu0_int18_status6;
    ZXIC_UINT32 smmu0_int18_status5;
    ZXIC_UINT32 smmu0_int18_status4;
    ZXIC_UINT32 smmu0_int18_status3;
    ZXIC_UINT32 smmu0_int18_status2;
    ZXIC_UINT32 smmu0_int18_status1;
    ZXIC_UINT32 smmu0_int18_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT18_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int19_en_t
{
    ZXIC_UINT32 smmu0_int19_en31;
    ZXIC_UINT32 smmu0_int19_en30;
    ZXIC_UINT32 smmu0_int19_en29;
    ZXIC_UINT32 smmu0_int19_en28;
    ZXIC_UINT32 smmu0_int19_en27;
    ZXIC_UINT32 smmu0_int19_en26;
    ZXIC_UINT32 smmu0_int19_en25;
    ZXIC_UINT32 smmu0_int19_en24;
    ZXIC_UINT32 smmu0_int19_en23;
    ZXIC_UINT32 smmu0_int19_en22;
    ZXIC_UINT32 smmu0_int19_en21;
    ZXIC_UINT32 smmu0_int19_en20;
    ZXIC_UINT32 smmu0_int19_en19;
    ZXIC_UINT32 smmu0_int19_en18;
    ZXIC_UINT32 smmu0_int19_en17;
    ZXIC_UINT32 smmu0_int19_en16;
    ZXIC_UINT32 smmu0_int19_en15;
    ZXIC_UINT32 smmu0_int19_en14;
    ZXIC_UINT32 smmu0_int19_en13;
    ZXIC_UINT32 smmu0_int19_en12;
    ZXIC_UINT32 smmu0_int19_en11;
    ZXIC_UINT32 smmu0_int19_en10;
    ZXIC_UINT32 smmu0_int19_en9;
    ZXIC_UINT32 smmu0_int19_en8;
    ZXIC_UINT32 smmu0_int19_en7;
    ZXIC_UINT32 smmu0_int19_en6;
    ZXIC_UINT32 smmu0_int19_en5;
    ZXIC_UINT32 smmu0_int19_en4;
    ZXIC_UINT32 smmu0_int19_en3;
    ZXIC_UINT32 smmu0_int19_en2;
    ZXIC_UINT32 smmu0_int19_en1;
    ZXIC_UINT32 smmu0_int19_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT19_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int19_mask_t
{
    ZXIC_UINT32 smmu0_int19_mask31;
    ZXIC_UINT32 smmu0_int19_mask30;
    ZXIC_UINT32 smmu0_int19_mask29;
    ZXIC_UINT32 smmu0_int19_mask28;
    ZXIC_UINT32 smmu0_int19_mask27;
    ZXIC_UINT32 smmu0_int19_mask26;
    ZXIC_UINT32 smmu0_int19_mask25;
    ZXIC_UINT32 smmu0_int19_mask24;
    ZXIC_UINT32 smmu0_int19_mask23;
    ZXIC_UINT32 smmu0_int19_mask22;
    ZXIC_UINT32 smmu0_int19_mask21;
    ZXIC_UINT32 smmu0_int19_mask20;
    ZXIC_UINT32 smmu0_int19_mask19;
    ZXIC_UINT32 smmu0_int19_mask18;
    ZXIC_UINT32 smmu0_int19_mask17;
    ZXIC_UINT32 smmu0_int19_mask16;
    ZXIC_UINT32 smmu0_int19_mask15;
    ZXIC_UINT32 smmu0_int19_mask14;
    ZXIC_UINT32 smmu0_int19_mask13;
    ZXIC_UINT32 smmu0_int19_mask12;
    ZXIC_UINT32 smmu0_int19_mask11;
    ZXIC_UINT32 smmu0_int19_mask10;
    ZXIC_UINT32 smmu0_int19_mask9;
    ZXIC_UINT32 smmu0_int19_mask8;
    ZXIC_UINT32 smmu0_int19_mask7;
    ZXIC_UINT32 smmu0_int19_mask6;
    ZXIC_UINT32 smmu0_int19_mask5;
    ZXIC_UINT32 smmu0_int19_mask4;
    ZXIC_UINT32 smmu0_int19_mask3;
    ZXIC_UINT32 smmu0_int19_mask2;
    ZXIC_UINT32 smmu0_int19_mask1;
    ZXIC_UINT32 smmu0_int19_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT19_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int19_status_t
{
    ZXIC_UINT32 smmu0_int19_status31;
    ZXIC_UINT32 smmu0_int19_status30;
    ZXIC_UINT32 smmu0_int19_status29;
    ZXIC_UINT32 smmu0_int19_status28;
    ZXIC_UINT32 smmu0_int19_status27;
    ZXIC_UINT32 smmu0_int19_status26;
    ZXIC_UINT32 smmu0_int19_status25;
    ZXIC_UINT32 smmu0_int19_status24;
    ZXIC_UINT32 smmu0_int19_status23;
    ZXIC_UINT32 smmu0_int19_status22;
    ZXIC_UINT32 smmu0_int19_status21;
    ZXIC_UINT32 smmu0_int19_status20;
    ZXIC_UINT32 smmu0_int19_status19;
    ZXIC_UINT32 smmu0_int19_status18;
    ZXIC_UINT32 smmu0_int19_status17;
    ZXIC_UINT32 smmu0_int19_status16;
    ZXIC_UINT32 smmu0_int19_status15;
    ZXIC_UINT32 smmu0_int19_status14;
    ZXIC_UINT32 smmu0_int19_status13;
    ZXIC_UINT32 smmu0_int19_status12;
    ZXIC_UINT32 smmu0_int19_status11;
    ZXIC_UINT32 smmu0_int19_status10;
    ZXIC_UINT32 smmu0_int19_status9;
    ZXIC_UINT32 smmu0_int19_status8;
    ZXIC_UINT32 smmu0_int19_status7;
    ZXIC_UINT32 smmu0_int19_status6;
    ZXIC_UINT32 smmu0_int19_status5;
    ZXIC_UINT32 smmu0_int19_status4;
    ZXIC_UINT32 smmu0_int19_status3;
    ZXIC_UINT32 smmu0_int19_status2;
    ZXIC_UINT32 smmu0_int19_status1;
    ZXIC_UINT32 smmu0_int19_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT19_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int20_en_t
{
    ZXIC_UINT32 smmu0_int20_en31;
    ZXIC_UINT32 smmu0_int20_en30;
    ZXIC_UINT32 smmu0_int20_en29;
    ZXIC_UINT32 smmu0_int20_en28;
    ZXIC_UINT32 smmu0_int20_en27;
    ZXIC_UINT32 smmu0_int20_en26;
    ZXIC_UINT32 smmu0_int20_en25;
    ZXIC_UINT32 smmu0_int20_en24;
    ZXIC_UINT32 smmu0_int20_en23;
    ZXIC_UINT32 smmu0_int20_en22;
    ZXIC_UINT32 smmu0_int20_en21;
    ZXIC_UINT32 smmu0_int20_en20;
    ZXIC_UINT32 smmu0_int20_en19;
    ZXIC_UINT32 smmu0_int20_en18;
    ZXIC_UINT32 smmu0_int20_en17;
    ZXIC_UINT32 smmu0_int20_en16;
    ZXIC_UINT32 smmu0_int20_en15;
    ZXIC_UINT32 smmu0_int20_en14;
    ZXIC_UINT32 smmu0_int20_en13;
    ZXIC_UINT32 smmu0_int20_en12;
    ZXIC_UINT32 smmu0_int20_en11;
    ZXIC_UINT32 smmu0_int20_en10;
    ZXIC_UINT32 smmu0_int20_en9;
    ZXIC_UINT32 smmu0_int20_en8;
    ZXIC_UINT32 smmu0_int20_en7;
    ZXIC_UINT32 smmu0_int20_en6;
    ZXIC_UINT32 smmu0_int20_en5;
    ZXIC_UINT32 smmu0_int20_en4;
    ZXIC_UINT32 smmu0_int20_en3;
    ZXIC_UINT32 smmu0_int20_en2;
    ZXIC_UINT32 smmu0_int20_en1;
    ZXIC_UINT32 smmu0_int20_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT20_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int20_mask_t
{
    ZXIC_UINT32 smmu0_int20_mask31;
    ZXIC_UINT32 smmu0_int20_mask30;
    ZXIC_UINT32 smmu0_int20_mask29;
    ZXIC_UINT32 smmu0_int20_mask28;
    ZXIC_UINT32 smmu0_int20_mask27;
    ZXIC_UINT32 smmu0_int20_mask26;
    ZXIC_UINT32 smmu0_int20_mask25;
    ZXIC_UINT32 smmu0_int20_mask24;
    ZXIC_UINT32 smmu0_int20_mask23;
    ZXIC_UINT32 smmu0_int20_mask22;
    ZXIC_UINT32 smmu0_int20_mask21;
    ZXIC_UINT32 smmu0_int20_mask20;
    ZXIC_UINT32 smmu0_int20_mask19;
    ZXIC_UINT32 smmu0_int20_mask18;
    ZXIC_UINT32 smmu0_int20_mask17;
    ZXIC_UINT32 smmu0_int20_mask16;
    ZXIC_UINT32 smmu0_int20_mask15;
    ZXIC_UINT32 smmu0_int20_mask14;
    ZXIC_UINT32 smmu0_int20_mask13;
    ZXIC_UINT32 smmu0_int20_mask12;
    ZXIC_UINT32 smmu0_int20_mask11;
    ZXIC_UINT32 smmu0_int20_mask10;
    ZXIC_UINT32 smmu0_int20_mask9;
    ZXIC_UINT32 smmu0_int20_mask8;
    ZXIC_UINT32 smmu0_int20_mask7;
    ZXIC_UINT32 smmu0_int20_mask6;
    ZXIC_UINT32 smmu0_int20_mask5;
    ZXIC_UINT32 smmu0_int20_mask4;
    ZXIC_UINT32 smmu0_int20_mask3;
    ZXIC_UINT32 smmu0_int20_mask2;
    ZXIC_UINT32 smmu0_int20_mask1;
    ZXIC_UINT32 smmu0_int20_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT20_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int20_status_t
{
    ZXIC_UINT32 smmu0_int20_status31;
    ZXIC_UINT32 smmu0_int20_status30;
    ZXIC_UINT32 smmu0_int20_status29;
    ZXIC_UINT32 smmu0_int20_status28;
    ZXIC_UINT32 smmu0_int20_status27;
    ZXIC_UINT32 smmu0_int20_status26;
    ZXIC_UINT32 smmu0_int20_status25;
    ZXIC_UINT32 smmu0_int20_status24;
    ZXIC_UINT32 smmu0_int20_status23;
    ZXIC_UINT32 smmu0_int20_status22;
    ZXIC_UINT32 smmu0_int20_status21;
    ZXIC_UINT32 smmu0_int20_status20;
    ZXIC_UINT32 smmu0_int20_status19;
    ZXIC_UINT32 smmu0_int20_status18;
    ZXIC_UINT32 smmu0_int20_status17;
    ZXIC_UINT32 smmu0_int20_status16;
    ZXIC_UINT32 smmu0_int20_status15;
    ZXIC_UINT32 smmu0_int20_status14;
    ZXIC_UINT32 smmu0_int20_status13;
    ZXIC_UINT32 smmu0_int20_status12;
    ZXIC_UINT32 smmu0_int20_status11;
    ZXIC_UINT32 smmu0_int20_status10;
    ZXIC_UINT32 smmu0_int20_status9;
    ZXIC_UINT32 smmu0_int20_status8;
    ZXIC_UINT32 smmu0_int20_status7;
    ZXIC_UINT32 smmu0_int20_status6;
    ZXIC_UINT32 smmu0_int20_status5;
    ZXIC_UINT32 smmu0_int20_status4;
    ZXIC_UINT32 smmu0_int20_status3;
    ZXIC_UINT32 smmu0_int20_status2;
    ZXIC_UINT32 smmu0_int20_status1;
    ZXIC_UINT32 smmu0_int20_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT20_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int21_en_t
{
    ZXIC_UINT32 smmu0_int21_en31;
    ZXIC_UINT32 smmu0_int21_en30;
    ZXIC_UINT32 smmu0_int21_en29;
    ZXIC_UINT32 smmu0_int21_en28;
    ZXIC_UINT32 smmu0_int21_en27;
    ZXIC_UINT32 smmu0_int21_en26;
    ZXIC_UINT32 smmu0_int21_en25;
    ZXIC_UINT32 smmu0_int21_en24;
    ZXIC_UINT32 smmu0_int21_en23;
    ZXIC_UINT32 smmu0_int21_en22;
    ZXIC_UINT32 smmu0_int21_en21;
    ZXIC_UINT32 smmu0_int21_en20;
    ZXIC_UINT32 smmu0_int21_en19;
    ZXIC_UINT32 smmu0_int21_en18;
    ZXIC_UINT32 smmu0_int21_en17;
    ZXIC_UINT32 smmu0_int21_en16;
    ZXIC_UINT32 smmu0_int21_en15;
    ZXIC_UINT32 smmu0_int21_en14;
    ZXIC_UINT32 smmu0_int21_en13;
    ZXIC_UINT32 smmu0_int21_en12;
    ZXIC_UINT32 smmu0_int21_en11;
    ZXIC_UINT32 smmu0_int21_en10;
    ZXIC_UINT32 smmu0_int21_en9;
    ZXIC_UINT32 smmu0_int21_en8;
    ZXIC_UINT32 smmu0_int21_en7;
    ZXIC_UINT32 smmu0_int21_en6;
    ZXIC_UINT32 smmu0_int21_en5;
    ZXIC_UINT32 smmu0_int21_en4;
    ZXIC_UINT32 smmu0_int21_en3;
    ZXIC_UINT32 smmu0_int21_en2;
    ZXIC_UINT32 smmu0_int21_en1;
    ZXIC_UINT32 smmu0_int21_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT21_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int21_mask_t
{
    ZXIC_UINT32 smmu0_int21_mask31;
    ZXIC_UINT32 smmu0_int21_mask30;
    ZXIC_UINT32 smmu0_int21_mask29;
    ZXIC_UINT32 smmu0_int21_mask28;
    ZXIC_UINT32 smmu0_int21_mask27;
    ZXIC_UINT32 smmu0_int21_mask26;
    ZXIC_UINT32 smmu0_int21_mask25;
    ZXIC_UINT32 smmu0_int21_mask24;
    ZXIC_UINT32 smmu0_int21_mask23;
    ZXIC_UINT32 smmu0_int21_mask22;
    ZXIC_UINT32 smmu0_int21_mask21;
    ZXIC_UINT32 smmu0_int21_mask20;
    ZXIC_UINT32 smmu0_int21_mask19;
    ZXIC_UINT32 smmu0_int21_mask18;
    ZXIC_UINT32 smmu0_int21_mask17;
    ZXIC_UINT32 smmu0_int21_mask16;
    ZXIC_UINT32 smmu0_int21_mask15;
    ZXIC_UINT32 smmu0_int21_mask14;
    ZXIC_UINT32 smmu0_int21_mask13;
    ZXIC_UINT32 smmu0_int21_mask12;
    ZXIC_UINT32 smmu0_int21_mask11;
    ZXIC_UINT32 smmu0_int21_mask10;
    ZXIC_UINT32 smmu0_int21_mask9;
    ZXIC_UINT32 smmu0_int21_mask8;
    ZXIC_UINT32 smmu0_int21_mask7;
    ZXIC_UINT32 smmu0_int21_mask6;
    ZXIC_UINT32 smmu0_int21_mask5;
    ZXIC_UINT32 smmu0_int21_mask4;
    ZXIC_UINT32 smmu0_int21_mask3;
    ZXIC_UINT32 smmu0_int21_mask2;
    ZXIC_UINT32 smmu0_int21_mask1;
    ZXIC_UINT32 smmu0_int21_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT21_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int21_status_t
{
    ZXIC_UINT32 smmu0_int21_status31;
    ZXIC_UINT32 smmu0_int21_status30;
    ZXIC_UINT32 smmu0_int21_status29;
    ZXIC_UINT32 smmu0_int21_status28;
    ZXIC_UINT32 smmu0_int21_status27;
    ZXIC_UINT32 smmu0_int21_status26;
    ZXIC_UINT32 smmu0_int21_status25;
    ZXIC_UINT32 smmu0_int21_status24;
    ZXIC_UINT32 smmu0_int21_status23;
    ZXIC_UINT32 smmu0_int21_status22;
    ZXIC_UINT32 smmu0_int21_status21;
    ZXIC_UINT32 smmu0_int21_status20;
    ZXIC_UINT32 smmu0_int21_status19;
    ZXIC_UINT32 smmu0_int21_status18;
    ZXIC_UINT32 smmu0_int21_status17;
    ZXIC_UINT32 smmu0_int21_status16;
    ZXIC_UINT32 smmu0_int21_status15;
    ZXIC_UINT32 smmu0_int21_status14;
    ZXIC_UINT32 smmu0_int21_status13;
    ZXIC_UINT32 smmu0_int21_status12;
    ZXIC_UINT32 smmu0_int21_status11;
    ZXIC_UINT32 smmu0_int21_status10;
    ZXIC_UINT32 smmu0_int21_status9;
    ZXIC_UINT32 smmu0_int21_status8;
    ZXIC_UINT32 smmu0_int21_status7;
    ZXIC_UINT32 smmu0_int21_status6;
    ZXIC_UINT32 smmu0_int21_status5;
    ZXIC_UINT32 smmu0_int21_status4;
    ZXIC_UINT32 smmu0_int21_status3;
    ZXIC_UINT32 smmu0_int21_status2;
    ZXIC_UINT32 smmu0_int21_status1;
    ZXIC_UINT32 smmu0_int21_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT21_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int22_en_t
{
    ZXIC_UINT32 smmu0_int22_en31;
    ZXIC_UINT32 smmu0_int22_en30;
    ZXIC_UINT32 smmu0_int22_en29;
    ZXIC_UINT32 smmu0_int22_en28;
    ZXIC_UINT32 smmu0_int22_en27;
    ZXIC_UINT32 smmu0_int22_en26;
    ZXIC_UINT32 smmu0_int22_en25;
    ZXIC_UINT32 smmu0_int22_en24;
    ZXIC_UINT32 smmu0_int22_en23;
    ZXIC_UINT32 smmu0_int22_en22;
    ZXIC_UINT32 smmu0_int22_en21;
    ZXIC_UINT32 smmu0_int22_en20;
    ZXIC_UINT32 smmu0_int22_en19;
    ZXIC_UINT32 smmu0_int22_en18;
    ZXIC_UINT32 smmu0_int22_en17;
    ZXIC_UINT32 smmu0_int22_en16;
    ZXIC_UINT32 smmu0_int22_en15;
    ZXIC_UINT32 smmu0_int22_en14;
    ZXIC_UINT32 smmu0_int22_en13;
    ZXIC_UINT32 smmu0_int22_en12;
    ZXIC_UINT32 smmu0_int22_en11;
    ZXIC_UINT32 smmu0_int22_en10;
    ZXIC_UINT32 smmu0_int22_en9;
    ZXIC_UINT32 smmu0_int22_en8;
    ZXIC_UINT32 smmu0_int22_en7;
    ZXIC_UINT32 smmu0_int22_en6;
    ZXIC_UINT32 smmu0_int22_en5;
    ZXIC_UINT32 smmu0_int22_en4;
    ZXIC_UINT32 smmu0_int22_en3;
    ZXIC_UINT32 smmu0_int22_en2;
    ZXIC_UINT32 smmu0_int22_en1;
    ZXIC_UINT32 smmu0_int22_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT22_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int22_mask_t
{
    ZXIC_UINT32 smmu0_int22_mask31;
    ZXIC_UINT32 smmu0_int22_mask30;
    ZXIC_UINT32 smmu0_int22_mask29;
    ZXIC_UINT32 smmu0_int22_mask28;
    ZXIC_UINT32 smmu0_int22_mask27;
    ZXIC_UINT32 smmu0_int22_mask26;
    ZXIC_UINT32 smmu0_int22_mask25;
    ZXIC_UINT32 smmu0_int22_mask24;
    ZXIC_UINT32 smmu0_int22_mask23;
    ZXIC_UINT32 smmu0_int22_mask22;
    ZXIC_UINT32 smmu0_int22_mask21;
    ZXIC_UINT32 smmu0_int22_mask20;
    ZXIC_UINT32 smmu0_int22_mask19;
    ZXIC_UINT32 smmu0_int22_mask18;
    ZXIC_UINT32 smmu0_int22_mask17;
    ZXIC_UINT32 smmu0_int22_mask16;
    ZXIC_UINT32 smmu0_int22_mask15;
    ZXIC_UINT32 smmu0_int22_mask14;
    ZXIC_UINT32 smmu0_int22_mask13;
    ZXIC_UINT32 smmu0_int22_mask12;
    ZXIC_UINT32 smmu0_int22_mask11;
    ZXIC_UINT32 smmu0_int22_mask10;
    ZXIC_UINT32 smmu0_int22_mask9;
    ZXIC_UINT32 smmu0_int22_mask8;
    ZXIC_UINT32 smmu0_int22_mask7;
    ZXIC_UINT32 smmu0_int22_mask6;
    ZXIC_UINT32 smmu0_int22_mask5;
    ZXIC_UINT32 smmu0_int22_mask4;
    ZXIC_UINT32 smmu0_int22_mask3;
    ZXIC_UINT32 smmu0_int22_mask2;
    ZXIC_UINT32 smmu0_int22_mask1;
    ZXIC_UINT32 smmu0_int22_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT22_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int22_status_t
{
    ZXIC_UINT32 smmu0_int22_status31;
    ZXIC_UINT32 smmu0_int22_status30;
    ZXIC_UINT32 smmu0_int22_status29;
    ZXIC_UINT32 smmu0_int22_status28;
    ZXIC_UINT32 smmu0_int22_status27;
    ZXIC_UINT32 smmu0_int22_status26;
    ZXIC_UINT32 smmu0_int22_status25;
    ZXIC_UINT32 smmu0_int22_status24;
    ZXIC_UINT32 smmu0_int22_status23;
    ZXIC_UINT32 smmu0_int22_status22;
    ZXIC_UINT32 smmu0_int22_status21;
    ZXIC_UINT32 smmu0_int22_status20;
    ZXIC_UINT32 smmu0_int22_status19;
    ZXIC_UINT32 smmu0_int22_status18;
    ZXIC_UINT32 smmu0_int22_status17;
    ZXIC_UINT32 smmu0_int22_status16;
    ZXIC_UINT32 smmu0_int22_status15;
    ZXIC_UINT32 smmu0_int22_status14;
    ZXIC_UINT32 smmu0_int22_status13;
    ZXIC_UINT32 smmu0_int22_status12;
    ZXIC_UINT32 smmu0_int22_status11;
    ZXIC_UINT32 smmu0_int22_status10;
    ZXIC_UINT32 smmu0_int22_status9;
    ZXIC_UINT32 smmu0_int22_status8;
    ZXIC_UINT32 smmu0_int22_status7;
    ZXIC_UINT32 smmu0_int22_status6;
    ZXIC_UINT32 smmu0_int22_status5;
    ZXIC_UINT32 smmu0_int22_status4;
    ZXIC_UINT32 smmu0_int22_status3;
    ZXIC_UINT32 smmu0_int22_status2;
    ZXIC_UINT32 smmu0_int22_status1;
    ZXIC_UINT32 smmu0_int22_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT22_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int23_en_t
{
    ZXIC_UINT32 smmu0_int23_en31;
    ZXIC_UINT32 smmu0_int23_en30;
    ZXIC_UINT32 smmu0_int23_en29;
    ZXIC_UINT32 smmu0_int23_en28;
    ZXIC_UINT32 smmu0_int23_en27;
    ZXIC_UINT32 smmu0_int23_en26;
    ZXIC_UINT32 smmu0_int23_en25;
    ZXIC_UINT32 smmu0_int23_en24;
    ZXIC_UINT32 smmu0_int23_en23;
    ZXIC_UINT32 smmu0_int23_en22;
    ZXIC_UINT32 smmu0_int23_en21;
    ZXIC_UINT32 smmu0_int23_en20;
    ZXIC_UINT32 smmu0_int23_en19;
    ZXIC_UINT32 smmu0_int23_en18;
    ZXIC_UINT32 smmu0_int23_en17;
    ZXIC_UINT32 smmu0_int23_en16;
    ZXIC_UINT32 smmu0_int23_en15;
    ZXIC_UINT32 smmu0_int23_en14;
    ZXIC_UINT32 smmu0_int23_en13;
    ZXIC_UINT32 smmu0_int23_en12;
    ZXIC_UINT32 smmu0_int23_en11;
    ZXIC_UINT32 smmu0_int23_en10;
    ZXIC_UINT32 smmu0_int23_en9;
    ZXIC_UINT32 smmu0_int23_en8;
    ZXIC_UINT32 smmu0_int23_en7;
    ZXIC_UINT32 smmu0_int23_en6;
    ZXIC_UINT32 smmu0_int23_en5;
    ZXIC_UINT32 smmu0_int23_en4;
    ZXIC_UINT32 smmu0_int23_en3;
    ZXIC_UINT32 smmu0_int23_en2;
    ZXIC_UINT32 smmu0_int23_en1;
    ZXIC_UINT32 smmu0_int23_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT23_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int23_mask_t
{
    ZXIC_UINT32 smmu0_int23_mask31;
    ZXIC_UINT32 smmu0_int23_mask30;
    ZXIC_UINT32 smmu0_int23_mask29;
    ZXIC_UINT32 smmu0_int23_mask28;
    ZXIC_UINT32 smmu0_int23_mask27;
    ZXIC_UINT32 smmu0_int23_mask26;
    ZXIC_UINT32 smmu0_int23_mask25;
    ZXIC_UINT32 smmu0_int23_mask24;
    ZXIC_UINT32 smmu0_int23_mask23;
    ZXIC_UINT32 smmu0_int23_mask22;
    ZXIC_UINT32 smmu0_int23_mask21;
    ZXIC_UINT32 smmu0_int23_mask20;
    ZXIC_UINT32 smmu0_int23_mask19;
    ZXIC_UINT32 smmu0_int23_mask18;
    ZXIC_UINT32 smmu0_int23_mask17;
    ZXIC_UINT32 smmu0_int23_mask16;
    ZXIC_UINT32 smmu0_int23_mask15;
    ZXIC_UINT32 smmu0_int23_mask14;
    ZXIC_UINT32 smmu0_int23_mask13;
    ZXIC_UINT32 smmu0_int23_mask12;
    ZXIC_UINT32 smmu0_int23_mask11;
    ZXIC_UINT32 smmu0_int23_mask10;
    ZXIC_UINT32 smmu0_int23_mask9;
    ZXIC_UINT32 smmu0_int23_mask8;
    ZXIC_UINT32 smmu0_int23_mask7;
    ZXIC_UINT32 smmu0_int23_mask6;
    ZXIC_UINT32 smmu0_int23_mask5;
    ZXIC_UINT32 smmu0_int23_mask4;
    ZXIC_UINT32 smmu0_int23_mask3;
    ZXIC_UINT32 smmu0_int23_mask2;
    ZXIC_UINT32 smmu0_int23_mask1;
    ZXIC_UINT32 smmu0_int23_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT23_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int23_status_t
{
    ZXIC_UINT32 smmu0_int23_status31;
    ZXIC_UINT32 smmu0_int23_status30;
    ZXIC_UINT32 smmu0_int23_status29;
    ZXIC_UINT32 smmu0_int23_status28;
    ZXIC_UINT32 smmu0_int23_status27;
    ZXIC_UINT32 smmu0_int23_status26;
    ZXIC_UINT32 smmu0_int23_status25;
    ZXIC_UINT32 smmu0_int23_status24;
    ZXIC_UINT32 smmu0_int23_status23;
    ZXIC_UINT32 smmu0_int23_status22;
    ZXIC_UINT32 smmu0_int23_status21;
    ZXIC_UINT32 smmu0_int23_status20;
    ZXIC_UINT32 smmu0_int23_status19;
    ZXIC_UINT32 smmu0_int23_status18;
    ZXIC_UINT32 smmu0_int23_status17;
    ZXIC_UINT32 smmu0_int23_status16;
    ZXIC_UINT32 smmu0_int23_status15;
    ZXIC_UINT32 smmu0_int23_status14;
    ZXIC_UINT32 smmu0_int23_status13;
    ZXIC_UINT32 smmu0_int23_status12;
    ZXIC_UINT32 smmu0_int23_status11;
    ZXIC_UINT32 smmu0_int23_status10;
    ZXIC_UINT32 smmu0_int23_status9;
    ZXIC_UINT32 smmu0_int23_status8;
    ZXIC_UINT32 smmu0_int23_status7;
    ZXIC_UINT32 smmu0_int23_status6;
    ZXIC_UINT32 smmu0_int23_status5;
    ZXIC_UINT32 smmu0_int23_status4;
    ZXIC_UINT32 smmu0_int23_status3;
    ZXIC_UINT32 smmu0_int23_status2;
    ZXIC_UINT32 smmu0_int23_status1;
    ZXIC_UINT32 smmu0_int23_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT23_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int24_en_t
{
    ZXIC_UINT32 smmu0_int24_en31;
    ZXIC_UINT32 smmu0_int24_en30;
    ZXIC_UINT32 smmu0_int24_en29;
    ZXIC_UINT32 smmu0_int24_en28;
    ZXIC_UINT32 smmu0_int24_en27;
    ZXIC_UINT32 smmu0_int24_en26;
    ZXIC_UINT32 smmu0_int24_en25;
    ZXIC_UINT32 smmu0_int24_en24;
    ZXIC_UINT32 smmu0_int24_en23;
    ZXIC_UINT32 smmu0_int24_en22;
    ZXIC_UINT32 smmu0_int24_en21;
    ZXIC_UINT32 smmu0_int24_en20;
    ZXIC_UINT32 smmu0_int24_en19;
    ZXIC_UINT32 smmu0_int24_en18;
    ZXIC_UINT32 smmu0_int24_en17;
    ZXIC_UINT32 smmu0_int24_en16;
    ZXIC_UINT32 smmu0_int24_en15;
    ZXIC_UINT32 smmu0_int24_en14;
    ZXIC_UINT32 smmu0_int24_en13;
    ZXIC_UINT32 smmu0_int24_en12;
    ZXIC_UINT32 smmu0_int24_en11;
    ZXIC_UINT32 smmu0_int24_en10;
    ZXIC_UINT32 smmu0_int24_en9;
    ZXIC_UINT32 smmu0_int24_en8;
    ZXIC_UINT32 smmu0_int24_en7;
    ZXIC_UINT32 smmu0_int24_en6;
    ZXIC_UINT32 smmu0_int24_en5;
    ZXIC_UINT32 smmu0_int24_en4;
    ZXIC_UINT32 smmu0_int24_en3;
    ZXIC_UINT32 smmu0_int24_en2;
    ZXIC_UINT32 smmu0_int24_en1;
    ZXIC_UINT32 smmu0_int24_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT24_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int24_mask_t
{
    ZXIC_UINT32 smmu0_int24_mask31;
    ZXIC_UINT32 smmu0_int24_mask30;
    ZXIC_UINT32 smmu0_int24_mask29;
    ZXIC_UINT32 smmu0_int24_mask28;
    ZXIC_UINT32 smmu0_int24_mask27;
    ZXIC_UINT32 smmu0_int24_mask26;
    ZXIC_UINT32 smmu0_int24_mask25;
    ZXIC_UINT32 smmu0_int24_mask24;
    ZXIC_UINT32 smmu0_int24_mask23;
    ZXIC_UINT32 smmu0_int24_mask22;
    ZXIC_UINT32 smmu0_int24_mask21;
    ZXIC_UINT32 smmu0_int24_mask20;
    ZXIC_UINT32 smmu0_int24_mask19;
    ZXIC_UINT32 smmu0_int24_mask18;
    ZXIC_UINT32 smmu0_int24_mask17;
    ZXIC_UINT32 smmu0_int24_mask16;
    ZXIC_UINT32 smmu0_int24_mask15;
    ZXIC_UINT32 smmu0_int24_mask14;
    ZXIC_UINT32 smmu0_int24_mask13;
    ZXIC_UINT32 smmu0_int24_mask12;
    ZXIC_UINT32 smmu0_int24_mask11;
    ZXIC_UINT32 smmu0_int24_mask10;
    ZXIC_UINT32 smmu0_int24_mask9;
    ZXIC_UINT32 smmu0_int24_mask8;
    ZXIC_UINT32 smmu0_int24_mask7;
    ZXIC_UINT32 smmu0_int24_mask6;
    ZXIC_UINT32 smmu0_int24_mask5;
    ZXIC_UINT32 smmu0_int24_mask4;
    ZXIC_UINT32 smmu0_int24_mask3;
    ZXIC_UINT32 smmu0_int24_mask2;
    ZXIC_UINT32 smmu0_int24_mask1;
    ZXIC_UINT32 smmu0_int24_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT24_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int24_status_t
{
    ZXIC_UINT32 smmu0_int24_status31;
    ZXIC_UINT32 smmu0_int24_status30;
    ZXIC_UINT32 smmu0_int24_status29;
    ZXIC_UINT32 smmu0_int24_status28;
    ZXIC_UINT32 smmu0_int24_status27;
    ZXIC_UINT32 smmu0_int24_status26;
    ZXIC_UINT32 smmu0_int24_status25;
    ZXIC_UINT32 smmu0_int24_status24;
    ZXIC_UINT32 smmu0_int24_status23;
    ZXIC_UINT32 smmu0_int24_status22;
    ZXIC_UINT32 smmu0_int24_status21;
    ZXIC_UINT32 smmu0_int24_status20;
    ZXIC_UINT32 smmu0_int24_status19;
    ZXIC_UINT32 smmu0_int24_status18;
    ZXIC_UINT32 smmu0_int24_status17;
    ZXIC_UINT32 smmu0_int24_status16;
    ZXIC_UINT32 smmu0_int24_status15;
    ZXIC_UINT32 smmu0_int24_status14;
    ZXIC_UINT32 smmu0_int24_status13;
    ZXIC_UINT32 smmu0_int24_status12;
    ZXIC_UINT32 smmu0_int24_status11;
    ZXIC_UINT32 smmu0_int24_status10;
    ZXIC_UINT32 smmu0_int24_status9;
    ZXIC_UINT32 smmu0_int24_status8;
    ZXIC_UINT32 smmu0_int24_status7;
    ZXIC_UINT32 smmu0_int24_status6;
    ZXIC_UINT32 smmu0_int24_status5;
    ZXIC_UINT32 smmu0_int24_status4;
    ZXIC_UINT32 smmu0_int24_status3;
    ZXIC_UINT32 smmu0_int24_status2;
    ZXIC_UINT32 smmu0_int24_status1;
    ZXIC_UINT32 smmu0_int24_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT24_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int25_en_t
{
    ZXIC_UINT32 smmu0_int25_en31;
    ZXIC_UINT32 smmu0_int25_en30;
    ZXIC_UINT32 smmu0_int25_en29;
    ZXIC_UINT32 smmu0_int25_en28;
    ZXIC_UINT32 smmu0_int25_en27;
    ZXIC_UINT32 smmu0_int25_en26;
    ZXIC_UINT32 smmu0_int25_en25;
    ZXIC_UINT32 smmu0_int25_en24;
    ZXIC_UINT32 smmu0_int25_en23;
    ZXIC_UINT32 smmu0_int25_en22;
    ZXIC_UINT32 smmu0_int25_en21;
    ZXIC_UINT32 smmu0_int25_en20;
    ZXIC_UINT32 smmu0_int25_en19;
    ZXIC_UINT32 smmu0_int25_en18;
    ZXIC_UINT32 smmu0_int25_en17;
    ZXIC_UINT32 smmu0_int25_en16;
    ZXIC_UINT32 smmu0_int25_en15;
    ZXIC_UINT32 smmu0_int25_en14;
    ZXIC_UINT32 smmu0_int25_en13;
    ZXIC_UINT32 smmu0_int25_en12;
    ZXIC_UINT32 smmu0_int25_en11;
    ZXIC_UINT32 smmu0_int25_en10;
    ZXIC_UINT32 smmu0_int25_en9;
    ZXIC_UINT32 smmu0_int25_en8;
    ZXIC_UINT32 smmu0_int25_en7;
    ZXIC_UINT32 smmu0_int25_en6;
    ZXIC_UINT32 smmu0_int25_en5;
    ZXIC_UINT32 smmu0_int25_en4;
    ZXIC_UINT32 smmu0_int25_en3;
    ZXIC_UINT32 smmu0_int25_en2;
    ZXIC_UINT32 smmu0_int25_en1;
    ZXIC_UINT32 smmu0_int25_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT25_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int25_mask_t
{
    ZXIC_UINT32 smmu0_int25_mask31;
    ZXIC_UINT32 smmu0_int25_mask30;
    ZXIC_UINT32 smmu0_int25_mask29;
    ZXIC_UINT32 smmu0_int25_mask28;
    ZXIC_UINT32 smmu0_int25_mask27;
    ZXIC_UINT32 smmu0_int25_mask26;
    ZXIC_UINT32 smmu0_int25_mask25;
    ZXIC_UINT32 smmu0_int25_mask24;
    ZXIC_UINT32 smmu0_int25_mask23;
    ZXIC_UINT32 smmu0_int25_mask22;
    ZXIC_UINT32 smmu0_int25_mask21;
    ZXIC_UINT32 smmu0_int25_mask20;
    ZXIC_UINT32 smmu0_int25_mask19;
    ZXIC_UINT32 smmu0_int25_mask18;
    ZXIC_UINT32 smmu0_int25_mask17;
    ZXIC_UINT32 smmu0_int25_mask16;
    ZXIC_UINT32 smmu0_int25_mask15;
    ZXIC_UINT32 smmu0_int25_mask14;
    ZXIC_UINT32 smmu0_int25_mask13;
    ZXIC_UINT32 smmu0_int25_mask12;
    ZXIC_UINT32 smmu0_int25_mask11;
    ZXIC_UINT32 smmu0_int25_mask10;
    ZXIC_UINT32 smmu0_int25_mask9;
    ZXIC_UINT32 smmu0_int25_mask8;
    ZXIC_UINT32 smmu0_int25_mask7;
    ZXIC_UINT32 smmu0_int25_mask6;
    ZXIC_UINT32 smmu0_int25_mask5;
    ZXIC_UINT32 smmu0_int25_mask4;
    ZXIC_UINT32 smmu0_int25_mask3;
    ZXIC_UINT32 smmu0_int25_mask2;
    ZXIC_UINT32 smmu0_int25_mask1;
    ZXIC_UINT32 smmu0_int25_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT25_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int25_status_t
{
    ZXIC_UINT32 smmu0_int25_status31;
    ZXIC_UINT32 smmu0_int25_status30;
    ZXIC_UINT32 smmu0_int25_status29;
    ZXIC_UINT32 smmu0_int25_status28;
    ZXIC_UINT32 smmu0_int25_status27;
    ZXIC_UINT32 smmu0_int25_status26;
    ZXIC_UINT32 smmu0_int25_status25;
    ZXIC_UINT32 smmu0_int25_status24;
    ZXIC_UINT32 smmu0_int25_status23;
    ZXIC_UINT32 smmu0_int25_status22;
    ZXIC_UINT32 smmu0_int25_status21;
    ZXIC_UINT32 smmu0_int25_status20;
    ZXIC_UINT32 smmu0_int25_status19;
    ZXIC_UINT32 smmu0_int25_status18;
    ZXIC_UINT32 smmu0_int25_status17;
    ZXIC_UINT32 smmu0_int25_status16;
    ZXIC_UINT32 smmu0_int25_status15;
    ZXIC_UINT32 smmu0_int25_status14;
    ZXIC_UINT32 smmu0_int25_status13;
    ZXIC_UINT32 smmu0_int25_status12;
    ZXIC_UINT32 smmu0_int25_status11;
    ZXIC_UINT32 smmu0_int25_status10;
    ZXIC_UINT32 smmu0_int25_status9;
    ZXIC_UINT32 smmu0_int25_status8;
    ZXIC_UINT32 smmu0_int25_status7;
    ZXIC_UINT32 smmu0_int25_status6;
    ZXIC_UINT32 smmu0_int25_status5;
    ZXIC_UINT32 smmu0_int25_status4;
    ZXIC_UINT32 smmu0_int25_status3;
    ZXIC_UINT32 smmu0_int25_status2;
    ZXIC_UINT32 smmu0_int25_status1;
    ZXIC_UINT32 smmu0_int25_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT25_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int26_en_t
{
    ZXIC_UINT32 smmu0_int26_en31;
    ZXIC_UINT32 smmu0_int26_en30;
    ZXIC_UINT32 smmu0_int26_en29;
    ZXIC_UINT32 smmu0_int26_en28;
    ZXIC_UINT32 smmu0_int26_en27;
    ZXIC_UINT32 smmu0_int26_en26;
    ZXIC_UINT32 smmu0_int26_en25;
    ZXIC_UINT32 smmu0_int26_en24;
    ZXIC_UINT32 smmu0_int26_en23;
    ZXIC_UINT32 smmu0_int26_en22;
    ZXIC_UINT32 smmu0_int26_en21;
    ZXIC_UINT32 smmu0_int26_en20;
    ZXIC_UINT32 smmu0_int26_en19;
    ZXIC_UINT32 smmu0_int26_en18;
    ZXIC_UINT32 smmu0_int26_en17;
    ZXIC_UINT32 smmu0_int26_en16;
    ZXIC_UINT32 smmu0_int26_en15;
    ZXIC_UINT32 smmu0_int26_en14;
    ZXIC_UINT32 smmu0_int26_en13;
    ZXIC_UINT32 smmu0_int26_en12;
    ZXIC_UINT32 smmu0_int26_en11;
    ZXIC_UINT32 smmu0_int26_en10;
    ZXIC_UINT32 smmu0_int26_en9;
    ZXIC_UINT32 smmu0_int26_en8;
    ZXIC_UINT32 smmu0_int26_en7;
    ZXIC_UINT32 smmu0_int26_en6;
    ZXIC_UINT32 smmu0_int26_en5;
    ZXIC_UINT32 smmu0_int26_en4;
    ZXIC_UINT32 smmu0_int26_en3;
    ZXIC_UINT32 smmu0_int26_en2;
    ZXIC_UINT32 smmu0_int26_en1;
    ZXIC_UINT32 smmu0_int26_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT26_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int26_mask_t
{
    ZXIC_UINT32 smmu0_int26_mask31;
    ZXIC_UINT32 smmu0_int26_mask30;
    ZXIC_UINT32 smmu0_int26_mask29;
    ZXIC_UINT32 smmu0_int26_mask28;
    ZXIC_UINT32 smmu0_int26_mask27;
    ZXIC_UINT32 smmu0_int26_mask26;
    ZXIC_UINT32 smmu0_int26_mask25;
    ZXIC_UINT32 smmu0_int26_mask24;
    ZXIC_UINT32 smmu0_int26_mask23;
    ZXIC_UINT32 smmu0_int26_mask22;
    ZXIC_UINT32 smmu0_int26_mask21;
    ZXIC_UINT32 smmu0_int26_mask20;
    ZXIC_UINT32 smmu0_int26_mask19;
    ZXIC_UINT32 smmu0_int26_mask18;
    ZXIC_UINT32 smmu0_int26_mask17;
    ZXIC_UINT32 smmu0_int26_mask16;
    ZXIC_UINT32 smmu0_int26_mask15;
    ZXIC_UINT32 smmu0_int26_mask14;
    ZXIC_UINT32 smmu0_int26_mask13;
    ZXIC_UINT32 smmu0_int26_mask12;
    ZXIC_UINT32 smmu0_int26_mask11;
    ZXIC_UINT32 smmu0_int26_mask10;
    ZXIC_UINT32 smmu0_int26_mask9;
    ZXIC_UINT32 smmu0_int26_mask8;
    ZXIC_UINT32 smmu0_int26_mask7;
    ZXIC_UINT32 smmu0_int26_mask6;
    ZXIC_UINT32 smmu0_int26_mask5;
    ZXIC_UINT32 smmu0_int26_mask4;
    ZXIC_UINT32 smmu0_int26_mask3;
    ZXIC_UINT32 smmu0_int26_mask2;
    ZXIC_UINT32 smmu0_int26_mask1;
    ZXIC_UINT32 smmu0_int26_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT26_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int26_status_t
{
    ZXIC_UINT32 smmu0_int26_status31;
    ZXIC_UINT32 smmu0_int26_status30;
    ZXIC_UINT32 smmu0_int26_status29;
    ZXIC_UINT32 smmu0_int26_status28;
    ZXIC_UINT32 smmu0_int26_status27;
    ZXIC_UINT32 smmu0_int26_status26;
    ZXIC_UINT32 smmu0_int26_status25;
    ZXIC_UINT32 smmu0_int26_status24;
    ZXIC_UINT32 smmu0_int26_status23;
    ZXIC_UINT32 smmu0_int26_status22;
    ZXIC_UINT32 smmu0_int26_status21;
    ZXIC_UINT32 smmu0_int26_status20;
    ZXIC_UINT32 smmu0_int26_status19;
    ZXIC_UINT32 smmu0_int26_status18;
    ZXIC_UINT32 smmu0_int26_status17;
    ZXIC_UINT32 smmu0_int26_status16;
    ZXIC_UINT32 smmu0_int26_status15;
    ZXIC_UINT32 smmu0_int26_status14;
    ZXIC_UINT32 smmu0_int26_status13;
    ZXIC_UINT32 smmu0_int26_status12;
    ZXIC_UINT32 smmu0_int26_status11;
    ZXIC_UINT32 smmu0_int26_status10;
    ZXIC_UINT32 smmu0_int26_status9;
    ZXIC_UINT32 smmu0_int26_status8;
    ZXIC_UINT32 smmu0_int26_status7;
    ZXIC_UINT32 smmu0_int26_status6;
    ZXIC_UINT32 smmu0_int26_status5;
    ZXIC_UINT32 smmu0_int26_status4;
    ZXIC_UINT32 smmu0_int26_status3;
    ZXIC_UINT32 smmu0_int26_status2;
    ZXIC_UINT32 smmu0_int26_status1;
    ZXIC_UINT32 smmu0_int26_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT26_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int27_en_t
{
    ZXIC_UINT32 smmu0_int27_en31;
    ZXIC_UINT32 smmu0_int27_en30;
    ZXIC_UINT32 smmu0_int27_en29;
    ZXIC_UINT32 smmu0_int27_en28;
    ZXIC_UINT32 smmu0_int27_en27;
    ZXIC_UINT32 smmu0_int27_en26;
    ZXIC_UINT32 smmu0_int27_en25;
    ZXIC_UINT32 smmu0_int27_en24;
    ZXIC_UINT32 smmu0_int27_en23;
    ZXIC_UINT32 smmu0_int27_en22;
    ZXIC_UINT32 smmu0_int27_en21;
    ZXIC_UINT32 smmu0_int27_en20;
    ZXIC_UINT32 smmu0_int27_en19;
    ZXIC_UINT32 smmu0_int27_en18;
    ZXIC_UINT32 smmu0_int27_en17;
    ZXIC_UINT32 smmu0_int27_en16;
    ZXIC_UINT32 smmu0_int27_en15;
    ZXIC_UINT32 smmu0_int27_en14;
    ZXIC_UINT32 smmu0_int27_en13;
    ZXIC_UINT32 smmu0_int27_en12;
    ZXIC_UINT32 smmu0_int27_en11;
    ZXIC_UINT32 smmu0_int27_en10;
    ZXIC_UINT32 smmu0_int27_en9;
    ZXIC_UINT32 smmu0_int27_en8;
    ZXIC_UINT32 smmu0_int27_en7;
    ZXIC_UINT32 smmu0_int27_en6;
    ZXIC_UINT32 smmu0_int27_en5;
    ZXIC_UINT32 smmu0_int27_en4;
    ZXIC_UINT32 smmu0_int27_en3;
    ZXIC_UINT32 smmu0_int27_en2;
    ZXIC_UINT32 smmu0_int27_en1;
    ZXIC_UINT32 smmu0_int27_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT27_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int27_mask_t
{
    ZXIC_UINT32 smmu0_int27_mask31;
    ZXIC_UINT32 smmu0_int27_mask30;
    ZXIC_UINT32 smmu0_int27_mask29;
    ZXIC_UINT32 smmu0_int27_mask28;
    ZXIC_UINT32 smmu0_int27_mask27;
    ZXIC_UINT32 smmu0_int27_mask26;
    ZXIC_UINT32 smmu0_int27_mask25;
    ZXIC_UINT32 smmu0_int27_mask24;
    ZXIC_UINT32 smmu0_int27_mask23;
    ZXIC_UINT32 smmu0_int27_mask22;
    ZXIC_UINT32 smmu0_int27_mask21;
    ZXIC_UINT32 smmu0_int27_mask20;
    ZXIC_UINT32 smmu0_int27_mask19;
    ZXIC_UINT32 smmu0_int27_mask18;
    ZXIC_UINT32 smmu0_int27_mask17;
    ZXIC_UINT32 smmu0_int27_mask16;
    ZXIC_UINT32 smmu0_int27_mask15;
    ZXIC_UINT32 smmu0_int27_mask14;
    ZXIC_UINT32 smmu0_int27_mask13;
    ZXIC_UINT32 smmu0_int27_mask12;
    ZXIC_UINT32 smmu0_int27_mask11;
    ZXIC_UINT32 smmu0_int27_mask10;
    ZXIC_UINT32 smmu0_int27_mask9;
    ZXIC_UINT32 smmu0_int27_mask8;
    ZXIC_UINT32 smmu0_int27_mask7;
    ZXIC_UINT32 smmu0_int27_mask6;
    ZXIC_UINT32 smmu0_int27_mask5;
    ZXIC_UINT32 smmu0_int27_mask4;
    ZXIC_UINT32 smmu0_int27_mask3;
    ZXIC_UINT32 smmu0_int27_mask2;
    ZXIC_UINT32 smmu0_int27_mask1;
    ZXIC_UINT32 smmu0_int27_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT27_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int27_status_t
{
    ZXIC_UINT32 smmu0_int27_status31;
    ZXIC_UINT32 smmu0_int27_status30;
    ZXIC_UINT32 smmu0_int27_status29;
    ZXIC_UINT32 smmu0_int27_status28;
    ZXIC_UINT32 smmu0_int27_status27;
    ZXIC_UINT32 smmu0_int27_status26;
    ZXIC_UINT32 smmu0_int27_status25;
    ZXIC_UINT32 smmu0_int27_status24;
    ZXIC_UINT32 smmu0_int27_status23;
    ZXIC_UINT32 smmu0_int27_status22;
    ZXIC_UINT32 smmu0_int27_status21;
    ZXIC_UINT32 smmu0_int27_status20;
    ZXIC_UINT32 smmu0_int27_status19;
    ZXIC_UINT32 smmu0_int27_status18;
    ZXIC_UINT32 smmu0_int27_status17;
    ZXIC_UINT32 smmu0_int27_status16;
    ZXIC_UINT32 smmu0_int27_status15;
    ZXIC_UINT32 smmu0_int27_status14;
    ZXIC_UINT32 smmu0_int27_status13;
    ZXIC_UINT32 smmu0_int27_status12;
    ZXIC_UINT32 smmu0_int27_status11;
    ZXIC_UINT32 smmu0_int27_status10;
    ZXIC_UINT32 smmu0_int27_status9;
    ZXIC_UINT32 smmu0_int27_status8;
    ZXIC_UINT32 smmu0_int27_status7;
    ZXIC_UINT32 smmu0_int27_status6;
    ZXIC_UINT32 smmu0_int27_status5;
    ZXIC_UINT32 smmu0_int27_status4;
    ZXIC_UINT32 smmu0_int27_status3;
    ZXIC_UINT32 smmu0_int27_status2;
    ZXIC_UINT32 smmu0_int27_status1;
    ZXIC_UINT32 smmu0_int27_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT27_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int28_en_t
{
    ZXIC_UINT32 smmu0_int28_en31;
    ZXIC_UINT32 smmu0_int28_en30;
    ZXIC_UINT32 smmu0_int28_en29;
    ZXIC_UINT32 smmu0_int28_en28;
    ZXIC_UINT32 smmu0_int28_en27;
    ZXIC_UINT32 smmu0_int28_en26;
    ZXIC_UINT32 smmu0_int28_en25;
    ZXIC_UINT32 smmu0_int28_en24;
    ZXIC_UINT32 smmu0_int28_en23;
    ZXIC_UINT32 smmu0_int28_en22;
    ZXIC_UINT32 smmu0_int28_en21;
    ZXIC_UINT32 smmu0_int28_en20;
    ZXIC_UINT32 smmu0_int28_en19;
    ZXIC_UINT32 smmu0_int28_en18;
    ZXIC_UINT32 smmu0_int28_en17;
    ZXIC_UINT32 smmu0_int28_en16;
    ZXIC_UINT32 smmu0_int28_en15;
    ZXIC_UINT32 smmu0_int28_en14;
    ZXIC_UINT32 smmu0_int28_en13;
    ZXIC_UINT32 smmu0_int28_en12;
    ZXIC_UINT32 smmu0_int28_en11;
    ZXIC_UINT32 smmu0_int28_en10;
    ZXIC_UINT32 smmu0_int28_en9;
    ZXIC_UINT32 smmu0_int28_en8;
    ZXIC_UINT32 smmu0_int28_en7;
    ZXIC_UINT32 smmu0_int28_en6;
    ZXIC_UINT32 smmu0_int28_en5;
    ZXIC_UINT32 smmu0_int28_en4;
    ZXIC_UINT32 smmu0_int28_en3;
    ZXIC_UINT32 smmu0_int28_en2;
    ZXIC_UINT32 smmu0_int28_en1;
    ZXIC_UINT32 smmu0_int28_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT28_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int28_mask_t
{
    ZXIC_UINT32 smmu0_int28_mask31;
    ZXIC_UINT32 smmu0_int28_mask30;
    ZXIC_UINT32 smmu0_int28_mask29;
    ZXIC_UINT32 smmu0_int28_mask28;
    ZXIC_UINT32 smmu0_int28_mask27;
    ZXIC_UINT32 smmu0_int28_mask26;
    ZXIC_UINT32 smmu0_int28_mask25;
    ZXIC_UINT32 smmu0_int28_mask24;
    ZXIC_UINT32 smmu0_int28_mask23;
    ZXIC_UINT32 smmu0_int28_mask22;
    ZXIC_UINT32 smmu0_int28_mask21;
    ZXIC_UINT32 smmu0_int28_mask20;
    ZXIC_UINT32 smmu0_int28_mask19;
    ZXIC_UINT32 smmu0_int28_mask18;
    ZXIC_UINT32 smmu0_int28_mask17;
    ZXIC_UINT32 smmu0_int28_mask16;
    ZXIC_UINT32 smmu0_int28_mask15;
    ZXIC_UINT32 smmu0_int28_mask14;
    ZXIC_UINT32 smmu0_int28_mask13;
    ZXIC_UINT32 smmu0_int28_mask12;
    ZXIC_UINT32 smmu0_int28_mask11;
    ZXIC_UINT32 smmu0_int28_mask10;
    ZXIC_UINT32 smmu0_int28_mask9;
    ZXIC_UINT32 smmu0_int28_mask8;
    ZXIC_UINT32 smmu0_int28_mask7;
    ZXIC_UINT32 smmu0_int28_mask6;
    ZXIC_UINT32 smmu0_int28_mask5;
    ZXIC_UINT32 smmu0_int28_mask4;
    ZXIC_UINT32 smmu0_int28_mask3;
    ZXIC_UINT32 smmu0_int28_mask2;
    ZXIC_UINT32 smmu0_int28_mask1;
    ZXIC_UINT32 smmu0_int28_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT28_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int28_status_t
{
    ZXIC_UINT32 smmu0_int28_status31;
    ZXIC_UINT32 smmu0_int28_status30;
    ZXIC_UINT32 smmu0_int28_status29;
    ZXIC_UINT32 smmu0_int28_status28;
    ZXIC_UINT32 smmu0_int28_status27;
    ZXIC_UINT32 smmu0_int28_status26;
    ZXIC_UINT32 smmu0_int28_status25;
    ZXIC_UINT32 smmu0_int28_status24;
    ZXIC_UINT32 smmu0_int28_status23;
    ZXIC_UINT32 smmu0_int28_status22;
    ZXIC_UINT32 smmu0_int28_status21;
    ZXIC_UINT32 smmu0_int28_status20;
    ZXIC_UINT32 smmu0_int28_status19;
    ZXIC_UINT32 smmu0_int28_status18;
    ZXIC_UINT32 smmu0_int28_status17;
    ZXIC_UINT32 smmu0_int28_status16;
    ZXIC_UINT32 smmu0_int28_status15;
    ZXIC_UINT32 smmu0_int28_status14;
    ZXIC_UINT32 smmu0_int28_status13;
    ZXIC_UINT32 smmu0_int28_status12;
    ZXIC_UINT32 smmu0_int28_status11;
    ZXIC_UINT32 smmu0_int28_status10;
    ZXIC_UINT32 smmu0_int28_status9;
    ZXIC_UINT32 smmu0_int28_status8;
    ZXIC_UINT32 smmu0_int28_status7;
    ZXIC_UINT32 smmu0_int28_status6;
    ZXIC_UINT32 smmu0_int28_status5;
    ZXIC_UINT32 smmu0_int28_status4;
    ZXIC_UINT32 smmu0_int28_status3;
    ZXIC_UINT32 smmu0_int28_status2;
    ZXIC_UINT32 smmu0_int28_status1;
    ZXIC_UINT32 smmu0_int28_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT28_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int29_en_t
{
    ZXIC_UINT32 smmu0_int29_en31;
    ZXIC_UINT32 smmu0_int29_en30;
    ZXIC_UINT32 smmu0_int29_en29;
    ZXIC_UINT32 smmu0_int29_en28;
    ZXIC_UINT32 smmu0_int29_en27;
    ZXIC_UINT32 smmu0_int29_en26;
    ZXIC_UINT32 smmu0_int29_en25;
    ZXIC_UINT32 smmu0_int29_en24;
    ZXIC_UINT32 smmu0_int29_en23;
    ZXIC_UINT32 smmu0_int29_en22;
    ZXIC_UINT32 smmu0_int29_en21;
    ZXIC_UINT32 smmu0_int29_en20;
    ZXIC_UINT32 smmu0_int29_en19;
    ZXIC_UINT32 smmu0_int29_en18;
    ZXIC_UINT32 smmu0_int29_en17;
    ZXIC_UINT32 smmu0_int29_en16;
    ZXIC_UINT32 smmu0_int29_en15;
    ZXIC_UINT32 smmu0_int29_en14;
    ZXIC_UINT32 smmu0_int29_en13;
    ZXIC_UINT32 smmu0_int29_en12;
    ZXIC_UINT32 smmu0_int29_en11;
    ZXIC_UINT32 smmu0_int29_en10;
    ZXIC_UINT32 smmu0_int29_en9;
    ZXIC_UINT32 smmu0_int29_en8;
    ZXIC_UINT32 smmu0_int29_en7;
    ZXIC_UINT32 smmu0_int29_en6;
    ZXIC_UINT32 smmu0_int29_en5;
    ZXIC_UINT32 smmu0_int29_en4;
    ZXIC_UINT32 smmu0_int29_en3;
    ZXIC_UINT32 smmu0_int29_en2;
    ZXIC_UINT32 smmu0_int29_en1;
    ZXIC_UINT32 smmu0_int29_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT29_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int29_mask_t
{
    ZXIC_UINT32 smmu0_int29_mask31;
    ZXIC_UINT32 smmu0_int29_mask30;
    ZXIC_UINT32 smmu0_int29_mask29;
    ZXIC_UINT32 smmu0_int29_mask28;
    ZXIC_UINT32 smmu0_int29_mask27;
    ZXIC_UINT32 smmu0_int29_mask26;
    ZXIC_UINT32 smmu0_int29_mask25;
    ZXIC_UINT32 smmu0_int29_mask24;
    ZXIC_UINT32 smmu0_int29_mask23;
    ZXIC_UINT32 smmu0_int29_mask22;
    ZXIC_UINT32 smmu0_int29_mask21;
    ZXIC_UINT32 smmu0_int29_mask20;
    ZXIC_UINT32 smmu0_int29_mask19;
    ZXIC_UINT32 smmu0_int29_mask18;
    ZXIC_UINT32 smmu0_int29_mask17;
    ZXIC_UINT32 smmu0_int29_mask16;
    ZXIC_UINT32 smmu0_int29_mask15;
    ZXIC_UINT32 smmu0_int29_mask14;
    ZXIC_UINT32 smmu0_int29_mask13;
    ZXIC_UINT32 smmu0_int29_mask12;
    ZXIC_UINT32 smmu0_int29_mask11;
    ZXIC_UINT32 smmu0_int29_mask10;
    ZXIC_UINT32 smmu0_int29_mask9;
    ZXIC_UINT32 smmu0_int29_mask8;
    ZXIC_UINT32 smmu0_int29_mask7;
    ZXIC_UINT32 smmu0_int29_mask6;
    ZXIC_UINT32 smmu0_int29_mask5;
    ZXIC_UINT32 smmu0_int29_mask4;
    ZXIC_UINT32 smmu0_int29_mask3;
    ZXIC_UINT32 smmu0_int29_mask2;
    ZXIC_UINT32 smmu0_int29_mask1;
    ZXIC_UINT32 smmu0_int29_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT29_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int29_status_t
{
    ZXIC_UINT32 smmu0_int29_status31;
    ZXIC_UINT32 smmu0_int29_status30;
    ZXIC_UINT32 smmu0_int29_status29;
    ZXIC_UINT32 smmu0_int29_status28;
    ZXIC_UINT32 smmu0_int29_status27;
    ZXIC_UINT32 smmu0_int29_status26;
    ZXIC_UINT32 smmu0_int29_status25;
    ZXIC_UINT32 smmu0_int29_status24;
    ZXIC_UINT32 smmu0_int29_status23;
    ZXIC_UINT32 smmu0_int29_status22;
    ZXIC_UINT32 smmu0_int29_status21;
    ZXIC_UINT32 smmu0_int29_status20;
    ZXIC_UINT32 smmu0_int29_status19;
    ZXIC_UINT32 smmu0_int29_status18;
    ZXIC_UINT32 smmu0_int29_status17;
    ZXIC_UINT32 smmu0_int29_status16;
    ZXIC_UINT32 smmu0_int29_status15;
    ZXIC_UINT32 smmu0_int29_status14;
    ZXIC_UINT32 smmu0_int29_status13;
    ZXIC_UINT32 smmu0_int29_status12;
    ZXIC_UINT32 smmu0_int29_status11;
    ZXIC_UINT32 smmu0_int29_status10;
    ZXIC_UINT32 smmu0_int29_status9;
    ZXIC_UINT32 smmu0_int29_status8;
    ZXIC_UINT32 smmu0_int29_status7;
    ZXIC_UINT32 smmu0_int29_status6;
    ZXIC_UINT32 smmu0_int29_status5;
    ZXIC_UINT32 smmu0_int29_status4;
    ZXIC_UINT32 smmu0_int29_status3;
    ZXIC_UINT32 smmu0_int29_status2;
    ZXIC_UINT32 smmu0_int29_status1;
    ZXIC_UINT32 smmu0_int29_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT29_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int30_en_t
{
    ZXIC_UINT32 smmu0_int30_en31;
    ZXIC_UINT32 smmu0_int30_en30;
    ZXIC_UINT32 smmu0_int30_en29;
    ZXIC_UINT32 smmu0_int30_en28;
    ZXIC_UINT32 smmu0_int30_en27;
    ZXIC_UINT32 smmu0_int30_en26;
    ZXIC_UINT32 smmu0_int30_en25;
    ZXIC_UINT32 smmu0_int30_en24;
    ZXIC_UINT32 smmu0_int30_en23;
    ZXIC_UINT32 smmu0_int30_en22;
    ZXIC_UINT32 smmu0_int30_en21;
    ZXIC_UINT32 smmu0_int30_en20;
    ZXIC_UINT32 smmu0_int30_en19;
    ZXIC_UINT32 smmu0_int30_en18;
    ZXIC_UINT32 smmu0_int30_en17;
    ZXIC_UINT32 smmu0_int30_en16;
    ZXIC_UINT32 smmu0_int30_en15;
    ZXIC_UINT32 smmu0_int30_en14;
    ZXIC_UINT32 smmu0_int30_en13;
    ZXIC_UINT32 smmu0_int30_en12;
    ZXIC_UINT32 smmu0_int30_en11;
    ZXIC_UINT32 smmu0_int30_en10;
    ZXIC_UINT32 smmu0_int30_en9;
    ZXIC_UINT32 smmu0_int30_en8;
    ZXIC_UINT32 smmu0_int30_en7;
    ZXIC_UINT32 smmu0_int30_en6;
    ZXIC_UINT32 smmu0_int30_en5;
    ZXIC_UINT32 smmu0_int30_en4;
    ZXIC_UINT32 smmu0_int30_en3;
    ZXIC_UINT32 smmu0_int30_en2;
    ZXIC_UINT32 smmu0_int30_en1;
    ZXIC_UINT32 smmu0_int30_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT30_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int30_mask_t
{
    ZXIC_UINT32 smmu0_int30_mask31;
    ZXIC_UINT32 smmu0_int30_mask30;
    ZXIC_UINT32 smmu0_int30_mask29;
    ZXIC_UINT32 smmu0_int30_mask28;
    ZXIC_UINT32 smmu0_int30_mask27;
    ZXIC_UINT32 smmu0_int30_mask26;
    ZXIC_UINT32 smmu0_int30_mask25;
    ZXIC_UINT32 smmu0_int30_mask24;
    ZXIC_UINT32 smmu0_int30_mask23;
    ZXIC_UINT32 smmu0_int30_mask22;
    ZXIC_UINT32 smmu0_int30_mask21;
    ZXIC_UINT32 smmu0_int30_mask20;
    ZXIC_UINT32 smmu0_int30_mask19;
    ZXIC_UINT32 smmu0_int30_mask18;
    ZXIC_UINT32 smmu0_int30_mask17;
    ZXIC_UINT32 smmu0_int30_mask16;
    ZXIC_UINT32 smmu0_int30_mask15;
    ZXIC_UINT32 smmu0_int30_mask14;
    ZXIC_UINT32 smmu0_int30_mask13;
    ZXIC_UINT32 smmu0_int30_mask12;
    ZXIC_UINT32 smmu0_int30_mask11;
    ZXIC_UINT32 smmu0_int30_mask10;
    ZXIC_UINT32 smmu0_int30_mask9;
    ZXIC_UINT32 smmu0_int30_mask8;
    ZXIC_UINT32 smmu0_int30_mask7;
    ZXIC_UINT32 smmu0_int30_mask6;
    ZXIC_UINT32 smmu0_int30_mask5;
    ZXIC_UINT32 smmu0_int30_mask4;
    ZXIC_UINT32 smmu0_int30_mask3;
    ZXIC_UINT32 smmu0_int30_mask2;
    ZXIC_UINT32 smmu0_int30_mask1;
    ZXIC_UINT32 smmu0_int30_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT30_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int30_status_t
{
    ZXIC_UINT32 smmu0_int30_status31;
    ZXIC_UINT32 smmu0_int30_status30;
    ZXIC_UINT32 smmu0_int30_status29;
    ZXIC_UINT32 smmu0_int30_status28;
    ZXIC_UINT32 smmu0_int30_status27;
    ZXIC_UINT32 smmu0_int30_status26;
    ZXIC_UINT32 smmu0_int30_status25;
    ZXIC_UINT32 smmu0_int30_status24;
    ZXIC_UINT32 smmu0_int30_status23;
    ZXIC_UINT32 smmu0_int30_status22;
    ZXIC_UINT32 smmu0_int30_status21;
    ZXIC_UINT32 smmu0_int30_status20;
    ZXIC_UINT32 smmu0_int30_status19;
    ZXIC_UINT32 smmu0_int30_status18;
    ZXIC_UINT32 smmu0_int30_status17;
    ZXIC_UINT32 smmu0_int30_status16;
    ZXIC_UINT32 smmu0_int30_status15;
    ZXIC_UINT32 smmu0_int30_status14;
    ZXIC_UINT32 smmu0_int30_status13;
    ZXIC_UINT32 smmu0_int30_status12;
    ZXIC_UINT32 smmu0_int30_status11;
    ZXIC_UINT32 smmu0_int30_status10;
    ZXIC_UINT32 smmu0_int30_status9;
    ZXIC_UINT32 smmu0_int30_status8;
    ZXIC_UINT32 smmu0_int30_status7;
    ZXIC_UINT32 smmu0_int30_status6;
    ZXIC_UINT32 smmu0_int30_status5;
    ZXIC_UINT32 smmu0_int30_status4;
    ZXIC_UINT32 smmu0_int30_status3;
    ZXIC_UINT32 smmu0_int30_status2;
    ZXIC_UINT32 smmu0_int30_status1;
    ZXIC_UINT32 smmu0_int30_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT30_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int31_en_t
{
    ZXIC_UINT32 smmu0_int31_en31;
    ZXIC_UINT32 smmu0_int31_en30;
    ZXIC_UINT32 smmu0_int31_en29;
    ZXIC_UINT32 smmu0_int31_en28;
    ZXIC_UINT32 smmu0_int31_en27;
    ZXIC_UINT32 smmu0_int31_en26;
    ZXIC_UINT32 smmu0_int31_en25;
    ZXIC_UINT32 smmu0_int31_en24;
    ZXIC_UINT32 smmu0_int31_en23;
    ZXIC_UINT32 smmu0_int31_en22;
    ZXIC_UINT32 smmu0_int31_en21;
    ZXIC_UINT32 smmu0_int31_en20;
    ZXIC_UINT32 smmu0_int31_en19;
    ZXIC_UINT32 smmu0_int31_en18;
    ZXIC_UINT32 smmu0_int31_en17;
    ZXIC_UINT32 smmu0_int31_en16;
    ZXIC_UINT32 smmu0_int31_en15;
    ZXIC_UINT32 smmu0_int31_en14;
    ZXIC_UINT32 smmu0_int31_en13;
    ZXIC_UINT32 smmu0_int31_en12;
    ZXIC_UINT32 smmu0_int31_en11;
    ZXIC_UINT32 smmu0_int31_en10;
    ZXIC_UINT32 smmu0_int31_en9;
    ZXIC_UINT32 smmu0_int31_en8;
    ZXIC_UINT32 smmu0_int31_en7;
    ZXIC_UINT32 smmu0_int31_en6;
    ZXIC_UINT32 smmu0_int31_en5;
    ZXIC_UINT32 smmu0_int31_en4;
    ZXIC_UINT32 smmu0_int31_en3;
    ZXIC_UINT32 smmu0_int31_en2;
    ZXIC_UINT32 smmu0_int31_en1;
    ZXIC_UINT32 smmu0_int31_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT31_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int31_mask_t
{
    ZXIC_UINT32 smmu0_int31_mask31;
    ZXIC_UINT32 smmu0_int31_mask30;
    ZXIC_UINT32 smmu0_int31_mask29;
    ZXIC_UINT32 smmu0_int31_mask28;
    ZXIC_UINT32 smmu0_int31_mask27;
    ZXIC_UINT32 smmu0_int31_mask26;
    ZXIC_UINT32 smmu0_int31_mask25;
    ZXIC_UINT32 smmu0_int31_mask24;
    ZXIC_UINT32 smmu0_int31_mask23;
    ZXIC_UINT32 smmu0_int31_mask22;
    ZXIC_UINT32 smmu0_int31_mask21;
    ZXIC_UINT32 smmu0_int31_mask20;
    ZXIC_UINT32 smmu0_int31_mask19;
    ZXIC_UINT32 smmu0_int31_mask18;
    ZXIC_UINT32 smmu0_int31_mask17;
    ZXIC_UINT32 smmu0_int31_mask16;
    ZXIC_UINT32 smmu0_int31_mask15;
    ZXIC_UINT32 smmu0_int31_mask14;
    ZXIC_UINT32 smmu0_int31_mask13;
    ZXIC_UINT32 smmu0_int31_mask12;
    ZXIC_UINT32 smmu0_int31_mask11;
    ZXIC_UINT32 smmu0_int31_mask10;
    ZXIC_UINT32 smmu0_int31_mask9;
    ZXIC_UINT32 smmu0_int31_mask8;
    ZXIC_UINT32 smmu0_int31_mask7;
    ZXIC_UINT32 smmu0_int31_mask6;
    ZXIC_UINT32 smmu0_int31_mask5;
    ZXIC_UINT32 smmu0_int31_mask4;
    ZXIC_UINT32 smmu0_int31_mask3;
    ZXIC_UINT32 smmu0_int31_mask2;
    ZXIC_UINT32 smmu0_int31_mask1;
    ZXIC_UINT32 smmu0_int31_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT31_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int31_status_t
{
    ZXIC_UINT32 smmu0_int31_status31;
    ZXIC_UINT32 smmu0_int31_status30;
    ZXIC_UINT32 smmu0_int31_status29;
    ZXIC_UINT32 smmu0_int31_status28;
    ZXIC_UINT32 smmu0_int31_status27;
    ZXIC_UINT32 smmu0_int31_status26;
    ZXIC_UINT32 smmu0_int31_status25;
    ZXIC_UINT32 smmu0_int31_status24;
    ZXIC_UINT32 smmu0_int31_status23;
    ZXIC_UINT32 smmu0_int31_status22;
    ZXIC_UINT32 smmu0_int31_status21;
    ZXIC_UINT32 smmu0_int31_status20;
    ZXIC_UINT32 smmu0_int31_status19;
    ZXIC_UINT32 smmu0_int31_status18;
    ZXIC_UINT32 smmu0_int31_status17;
    ZXIC_UINT32 smmu0_int31_status16;
    ZXIC_UINT32 smmu0_int31_status15;
    ZXIC_UINT32 smmu0_int31_status14;
    ZXIC_UINT32 smmu0_int31_status13;
    ZXIC_UINT32 smmu0_int31_status12;
    ZXIC_UINT32 smmu0_int31_status11;
    ZXIC_UINT32 smmu0_int31_status10;
    ZXIC_UINT32 smmu0_int31_status9;
    ZXIC_UINT32 smmu0_int31_status8;
    ZXIC_UINT32 smmu0_int31_status7;
    ZXIC_UINT32 smmu0_int31_status6;
    ZXIC_UINT32 smmu0_int31_status5;
    ZXIC_UINT32 smmu0_int31_status4;
    ZXIC_UINT32 smmu0_int31_status3;
    ZXIC_UINT32 smmu0_int31_status2;
    ZXIC_UINT32 smmu0_int31_status1;
    ZXIC_UINT32 smmu0_int31_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT31_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int32_en_t
{
    ZXIC_UINT32 smmu0_int32_en31;
    ZXIC_UINT32 smmu0_int32_en30;
    ZXIC_UINT32 smmu0_int32_en29;
    ZXIC_UINT32 smmu0_int32_en28;
    ZXIC_UINT32 smmu0_int32_en27;
    ZXIC_UINT32 smmu0_int32_en26;
    ZXIC_UINT32 smmu0_int32_en25;
    ZXIC_UINT32 smmu0_int32_en24;
    ZXIC_UINT32 smmu0_int32_en23;
    ZXIC_UINT32 smmu0_int32_en22;
    ZXIC_UINT32 smmu0_int32_en21;
    ZXIC_UINT32 smmu0_int32_en20;
    ZXIC_UINT32 smmu0_int32_en19;
    ZXIC_UINT32 smmu0_int32_en18;
    ZXIC_UINT32 smmu0_int32_en17;
    ZXIC_UINT32 smmu0_int32_en16;
    ZXIC_UINT32 smmu0_int32_en15;
    ZXIC_UINT32 smmu0_int32_en14;
    ZXIC_UINT32 smmu0_int32_en13;
    ZXIC_UINT32 smmu0_int32_en12;
    ZXIC_UINT32 smmu0_int32_en11;
    ZXIC_UINT32 smmu0_int32_en10;
    ZXIC_UINT32 smmu0_int32_en9;
    ZXIC_UINT32 smmu0_int32_en8;
    ZXIC_UINT32 smmu0_int32_en7;
    ZXIC_UINT32 smmu0_int32_en6;
    ZXIC_UINT32 smmu0_int32_en5;
    ZXIC_UINT32 smmu0_int32_en4;
    ZXIC_UINT32 smmu0_int32_en3;
    ZXIC_UINT32 smmu0_int32_en2;
    ZXIC_UINT32 smmu0_int32_en1;
    ZXIC_UINT32 smmu0_int32_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT32_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int32_mask_t
{
    ZXIC_UINT32 smmu0_int32_mask31;
    ZXIC_UINT32 smmu0_int32_mask30;
    ZXIC_UINT32 smmu0_int32_mask29;
    ZXIC_UINT32 smmu0_int32_mask28;
    ZXIC_UINT32 smmu0_int32_mask27;
    ZXIC_UINT32 smmu0_int32_mask26;
    ZXIC_UINT32 smmu0_int32_mask25;
    ZXIC_UINT32 smmu0_int32_mask24;
    ZXIC_UINT32 smmu0_int32_mask23;
    ZXIC_UINT32 smmu0_int32_mask22;
    ZXIC_UINT32 smmu0_int32_mask21;
    ZXIC_UINT32 smmu0_int32_mask20;
    ZXIC_UINT32 smmu0_int32_mask19;
    ZXIC_UINT32 smmu0_int32_mask18;
    ZXIC_UINT32 smmu0_int32_mask17;
    ZXIC_UINT32 smmu0_int32_mask16;
    ZXIC_UINT32 smmu0_int32_mask15;
    ZXIC_UINT32 smmu0_int32_mask14;
    ZXIC_UINT32 smmu0_int32_mask13;
    ZXIC_UINT32 smmu0_int32_mask12;
    ZXIC_UINT32 smmu0_int32_mask11;
    ZXIC_UINT32 smmu0_int32_mask10;
    ZXIC_UINT32 smmu0_int32_mask9;
    ZXIC_UINT32 smmu0_int32_mask8;
    ZXIC_UINT32 smmu0_int32_mask7;
    ZXIC_UINT32 smmu0_int32_mask6;
    ZXIC_UINT32 smmu0_int32_mask5;
    ZXIC_UINT32 smmu0_int32_mask4;
    ZXIC_UINT32 smmu0_int32_mask3;
    ZXIC_UINT32 smmu0_int32_mask2;
    ZXIC_UINT32 smmu0_int32_mask1;
    ZXIC_UINT32 smmu0_int32_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT32_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int32_status_t
{
    ZXIC_UINT32 smmu0_int32_status31;
    ZXIC_UINT32 smmu0_int32_status30;
    ZXIC_UINT32 smmu0_int32_status29;
    ZXIC_UINT32 smmu0_int32_status28;
    ZXIC_UINT32 smmu0_int32_status27;
    ZXIC_UINT32 smmu0_int32_status26;
    ZXIC_UINT32 smmu0_int32_status25;
    ZXIC_UINT32 smmu0_int32_status24;
    ZXIC_UINT32 smmu0_int32_status23;
    ZXIC_UINT32 smmu0_int32_status22;
    ZXIC_UINT32 smmu0_int32_status21;
    ZXIC_UINT32 smmu0_int32_status20;
    ZXIC_UINT32 smmu0_int32_status19;
    ZXIC_UINT32 smmu0_int32_status18;
    ZXIC_UINT32 smmu0_int32_status17;
    ZXIC_UINT32 smmu0_int32_status16;
    ZXIC_UINT32 smmu0_int32_status15;
    ZXIC_UINT32 smmu0_int32_status14;
    ZXIC_UINT32 smmu0_int32_status13;
    ZXIC_UINT32 smmu0_int32_status12;
    ZXIC_UINT32 smmu0_int32_status11;
    ZXIC_UINT32 smmu0_int32_status10;
    ZXIC_UINT32 smmu0_int32_status9;
    ZXIC_UINT32 smmu0_int32_status8;
    ZXIC_UINT32 smmu0_int32_status7;
    ZXIC_UINT32 smmu0_int32_status6;
    ZXIC_UINT32 smmu0_int32_status5;
    ZXIC_UINT32 smmu0_int32_status4;
    ZXIC_UINT32 smmu0_int32_status3;
    ZXIC_UINT32 smmu0_int32_status2;
    ZXIC_UINT32 smmu0_int32_status1;
    ZXIC_UINT32 smmu0_int32_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT32_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int33_en_t
{
    ZXIC_UINT32 smmu0_int33_en31;
    ZXIC_UINT32 smmu0_int33_en30;
    ZXIC_UINT32 smmu0_int33_en29;
    ZXIC_UINT32 smmu0_int33_en28;
    ZXIC_UINT32 smmu0_int33_en27;
    ZXIC_UINT32 smmu0_int33_en26;
    ZXIC_UINT32 smmu0_int33_en25;
    ZXIC_UINT32 smmu0_int33_en24;
    ZXIC_UINT32 smmu0_int33_en23;
    ZXIC_UINT32 smmu0_int33_en22;
    ZXIC_UINT32 smmu0_int33_en21;
    ZXIC_UINT32 smmu0_int33_en20;
    ZXIC_UINT32 smmu0_int33_en19;
    ZXIC_UINT32 smmu0_int33_en18;
    ZXIC_UINT32 smmu0_int33_en17;
    ZXIC_UINT32 smmu0_int33_en16;
    ZXIC_UINT32 smmu0_int33_en15;
    ZXIC_UINT32 smmu0_int33_en14;
    ZXIC_UINT32 smmu0_int33_en13;
    ZXIC_UINT32 smmu0_int33_en12;
    ZXIC_UINT32 smmu0_int33_en11;
    ZXIC_UINT32 smmu0_int33_en10;
    ZXIC_UINT32 smmu0_int33_en9;
    ZXIC_UINT32 smmu0_int33_en8;
    ZXIC_UINT32 smmu0_int33_en7;
    ZXIC_UINT32 smmu0_int33_en6;
    ZXIC_UINT32 smmu0_int33_en5;
    ZXIC_UINT32 smmu0_int33_en4;
    ZXIC_UINT32 smmu0_int33_en3;
    ZXIC_UINT32 smmu0_int33_en2;
    ZXIC_UINT32 smmu0_int33_en1;
    ZXIC_UINT32 smmu0_int33_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT33_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int33_mask_t
{
    ZXIC_UINT32 smmu0_int33_mask31;
    ZXIC_UINT32 smmu0_int33_mask30;
    ZXIC_UINT32 smmu0_int33_mask29;
    ZXIC_UINT32 smmu0_int33_mask28;
    ZXIC_UINT32 smmu0_int33_mask27;
    ZXIC_UINT32 smmu0_int33_mask26;
    ZXIC_UINT32 smmu0_int33_mask25;
    ZXIC_UINT32 smmu0_int33_mask24;
    ZXIC_UINT32 smmu0_int33_mask23;
    ZXIC_UINT32 smmu0_int33_mask22;
    ZXIC_UINT32 smmu0_int33_mask21;
    ZXIC_UINT32 smmu0_int33_mask20;
    ZXIC_UINT32 smmu0_int33_mask19;
    ZXIC_UINT32 smmu0_int33_mask18;
    ZXIC_UINT32 smmu0_int33_mask17;
    ZXIC_UINT32 smmu0_int33_mask16;
    ZXIC_UINT32 smmu0_int33_mask15;
    ZXIC_UINT32 smmu0_int33_mask14;
    ZXIC_UINT32 smmu0_int33_mask13;
    ZXIC_UINT32 smmu0_int33_mask12;
    ZXIC_UINT32 smmu0_int33_mask11;
    ZXIC_UINT32 smmu0_int33_mask10;
    ZXIC_UINT32 smmu0_int33_mask9;
    ZXIC_UINT32 smmu0_int33_mask8;
    ZXIC_UINT32 smmu0_int33_mask7;
    ZXIC_UINT32 smmu0_int33_mask6;
    ZXIC_UINT32 smmu0_int33_mask5;
    ZXIC_UINT32 smmu0_int33_mask4;
    ZXIC_UINT32 smmu0_int33_mask3;
    ZXIC_UINT32 smmu0_int33_mask2;
    ZXIC_UINT32 smmu0_int33_mask1;
    ZXIC_UINT32 smmu0_int33_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT33_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int33_status_t
{
    ZXIC_UINT32 smmu0_int33_status31;
    ZXIC_UINT32 smmu0_int33_status30;
    ZXIC_UINT32 smmu0_int33_status29;
    ZXIC_UINT32 smmu0_int33_status28;
    ZXIC_UINT32 smmu0_int33_status27;
    ZXIC_UINT32 smmu0_int33_status26;
    ZXIC_UINT32 smmu0_int33_status25;
    ZXIC_UINT32 smmu0_int33_status24;
    ZXIC_UINT32 smmu0_int33_status23;
    ZXIC_UINT32 smmu0_int33_status22;
    ZXIC_UINT32 smmu0_int33_status21;
    ZXIC_UINT32 smmu0_int33_status20;
    ZXIC_UINT32 smmu0_int33_status19;
    ZXIC_UINT32 smmu0_int33_status18;
    ZXIC_UINT32 smmu0_int33_status17;
    ZXIC_UINT32 smmu0_int33_status16;
    ZXIC_UINT32 smmu0_int33_status15;
    ZXIC_UINT32 smmu0_int33_status14;
    ZXIC_UINT32 smmu0_int33_status13;
    ZXIC_UINT32 smmu0_int33_status12;
    ZXIC_UINT32 smmu0_int33_status11;
    ZXIC_UINT32 smmu0_int33_status10;
    ZXIC_UINT32 smmu0_int33_status9;
    ZXIC_UINT32 smmu0_int33_status8;
    ZXIC_UINT32 smmu0_int33_status7;
    ZXIC_UINT32 smmu0_int33_status6;
    ZXIC_UINT32 smmu0_int33_status5;
    ZXIC_UINT32 smmu0_int33_status4;
    ZXIC_UINT32 smmu0_int33_status3;
    ZXIC_UINT32 smmu0_int33_status2;
    ZXIC_UINT32 smmu0_int33_status1;
    ZXIC_UINT32 smmu0_int33_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT33_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int34_en_t
{
    ZXIC_UINT32 smmu0_int34_en31;
    ZXIC_UINT32 smmu0_int34_en30;
    ZXIC_UINT32 smmu0_int34_en29;
    ZXIC_UINT32 smmu0_int34_en28;
    ZXIC_UINT32 smmu0_int34_en27;
    ZXIC_UINT32 smmu0_int34_en26;
    ZXIC_UINT32 smmu0_int34_en25;
    ZXIC_UINT32 smmu0_int34_en24;
    ZXIC_UINT32 smmu0_int34_en23;
    ZXIC_UINT32 smmu0_int34_en22;
    ZXIC_UINT32 smmu0_int34_en21;
    ZXIC_UINT32 smmu0_int34_en20;
    ZXIC_UINT32 smmu0_int34_en19;
    ZXIC_UINT32 smmu0_int34_en18;
    ZXIC_UINT32 smmu0_int34_en17;
    ZXIC_UINT32 smmu0_int34_en16;
    ZXIC_UINT32 smmu0_int34_en15;
    ZXIC_UINT32 smmu0_int34_en14;
    ZXIC_UINT32 smmu0_int34_en13;
    ZXIC_UINT32 smmu0_int34_en12;
    ZXIC_UINT32 smmu0_int34_en11;
    ZXIC_UINT32 smmu0_int34_en10;
    ZXIC_UINT32 smmu0_int34_en9;
    ZXIC_UINT32 smmu0_int34_en8;
    ZXIC_UINT32 smmu0_int34_en7;
    ZXIC_UINT32 smmu0_int34_en6;
    ZXIC_UINT32 smmu0_int34_en5;
    ZXIC_UINT32 smmu0_int34_en4;
    ZXIC_UINT32 smmu0_int34_en3;
    ZXIC_UINT32 smmu0_int34_en2;
    ZXIC_UINT32 smmu0_int34_en1;
    ZXIC_UINT32 smmu0_int34_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT34_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int34_mask_t
{
    ZXIC_UINT32 smmu0_int34_mask31;
    ZXIC_UINT32 smmu0_int34_mask30;
    ZXIC_UINT32 smmu0_int34_mask29;
    ZXIC_UINT32 smmu0_int34_mask28;
    ZXIC_UINT32 smmu0_int34_mask27;
    ZXIC_UINT32 smmu0_int34_mask26;
    ZXIC_UINT32 smmu0_int34_mask25;
    ZXIC_UINT32 smmu0_int34_mask24;
    ZXIC_UINT32 smmu0_int34_mask23;
    ZXIC_UINT32 smmu0_int34_mask22;
    ZXIC_UINT32 smmu0_int34_mask21;
    ZXIC_UINT32 smmu0_int34_mask20;
    ZXIC_UINT32 smmu0_int34_mask19;
    ZXIC_UINT32 smmu0_int34_mask18;
    ZXIC_UINT32 smmu0_int34_mask17;
    ZXIC_UINT32 smmu0_int34_mask16;
    ZXIC_UINT32 smmu0_int34_mask15;
    ZXIC_UINT32 smmu0_int34_mask14;
    ZXIC_UINT32 smmu0_int34_mask13;
    ZXIC_UINT32 smmu0_int34_mask12;
    ZXIC_UINT32 smmu0_int34_mask11;
    ZXIC_UINT32 smmu0_int34_mask10;
    ZXIC_UINT32 smmu0_int34_mask9;
    ZXIC_UINT32 smmu0_int34_mask8;
    ZXIC_UINT32 smmu0_int34_mask7;
    ZXIC_UINT32 smmu0_int34_mask6;
    ZXIC_UINT32 smmu0_int34_mask5;
    ZXIC_UINT32 smmu0_int34_mask4;
    ZXIC_UINT32 smmu0_int34_mask3;
    ZXIC_UINT32 smmu0_int34_mask2;
    ZXIC_UINT32 smmu0_int34_mask1;
    ZXIC_UINT32 smmu0_int34_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT34_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int34_status_t
{
    ZXIC_UINT32 smmu0_int34_status31;
    ZXIC_UINT32 smmu0_int34_status30;
    ZXIC_UINT32 smmu0_int34_status29;
    ZXIC_UINT32 smmu0_int34_status28;
    ZXIC_UINT32 smmu0_int34_status27;
    ZXIC_UINT32 smmu0_int34_status26;
    ZXIC_UINT32 smmu0_int34_status25;
    ZXIC_UINT32 smmu0_int34_status24;
    ZXIC_UINT32 smmu0_int34_status23;
    ZXIC_UINT32 smmu0_int34_status22;
    ZXIC_UINT32 smmu0_int34_status21;
    ZXIC_UINT32 smmu0_int34_status20;
    ZXIC_UINT32 smmu0_int34_status19;
    ZXIC_UINT32 smmu0_int34_status18;
    ZXIC_UINT32 smmu0_int34_status17;
    ZXIC_UINT32 smmu0_int34_status16;
    ZXIC_UINT32 smmu0_int34_status15;
    ZXIC_UINT32 smmu0_int34_status14;
    ZXIC_UINT32 smmu0_int34_status13;
    ZXIC_UINT32 smmu0_int34_status12;
    ZXIC_UINT32 smmu0_int34_status11;
    ZXIC_UINT32 smmu0_int34_status10;
    ZXIC_UINT32 smmu0_int34_status9;
    ZXIC_UINT32 smmu0_int34_status8;
    ZXIC_UINT32 smmu0_int34_status7;
    ZXIC_UINT32 smmu0_int34_status6;
    ZXIC_UINT32 smmu0_int34_status5;
    ZXIC_UINT32 smmu0_int34_status4;
    ZXIC_UINT32 smmu0_int34_status3;
    ZXIC_UINT32 smmu0_int34_status2;
    ZXIC_UINT32 smmu0_int34_status1;
    ZXIC_UINT32 smmu0_int34_status0;
}DPP_SMMU0_SMMU0_SMMU0_INT34_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int35_en_t
{
    ZXIC_UINT32 smmu0_int35_en0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT35_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int35_mask_t
{
    ZXIC_UINT32 smmu0_int35_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT35_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int35_status_t
{
    ZXIC_UINT32 smmu0_int35_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT35_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int36_en_t
{
    ZXIC_UINT32 smmu0_int36_en0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT36_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int36_mask_t
{
    ZXIC_UINT32 smmu0_int36_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT36_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int36_status_t
{
    ZXIC_UINT32 smmu0_int36_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT36_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int37_en_t
{
    ZXIC_UINT32 smmu0_int37_en0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT37_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int37_mask_t
{
    ZXIC_UINT32 smmu0_int37_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT37_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int37_status_t
{
    ZXIC_UINT32 smmu0_int37_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT37_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int38_en_t
{
    ZXIC_UINT32 smmu0_int38_en0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT38_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int38_mask_t
{
    ZXIC_UINT32 smmu0_int38_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT38_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int38_status_t
{
    ZXIC_UINT32 smmu0_int38_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT38_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int39_en_t
{
    ZXIC_UINT32 smmu0_int39_en0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT39_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int39_mask_t
{
    ZXIC_UINT32 smmu0_int39_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT39_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int39_status_t
{
    ZXIC_UINT32 smmu0_int39_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT39_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int40_en_t
{
    ZXIC_UINT32 smmu0_int40_en0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT40_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int40_mask_t
{
    ZXIC_UINT32 smmu0_int40_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT40_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int40_status_t
{
    ZXIC_UINT32 smmu0_int40_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT40_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int41_en_t
{
    ZXIC_UINT32 smmu0_int41_en0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT41_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int41_mask_t
{
    ZXIC_UINT32 smmu0_int41_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT41_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int41_status_t
{
    ZXIC_UINT32 smmu0_int41_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT41_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int42_en_t
{
    ZXIC_UINT32 smmu0_int42_en0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT42_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int42_mask_t
{
    ZXIC_UINT32 smmu0_int42_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT42_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int42_status_t
{
    ZXIC_UINT32 smmu0_int42_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT42_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int43_en_t
{
    ZXIC_UINT32 smmu0_int43_en0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT43_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int43_mask_t
{
    ZXIC_UINT32 smmu0_int43_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT43_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int43_status_t
{
    ZXIC_UINT32 smmu0_int43_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT43_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int44_en_t
{
    ZXIC_UINT32 smmu0_int44_en0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT44_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int44_mask_t
{
    ZXIC_UINT32 smmu0_int44_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT44_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int44_status_t
{
    ZXIC_UINT32 smmu0_int44_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT44_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int45_en_t
{
    ZXIC_UINT32 smmu0_int45_en0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT45_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int45_mask_t
{
    ZXIC_UINT32 smmu0_int45_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT45_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int45_status_t
{
    ZXIC_UINT32 smmu0_int45_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT45_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int46_en_t
{
    ZXIC_UINT32 smmu0_int46_en0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT46_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int46_mask_t
{
    ZXIC_UINT32 smmu0_int46_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT46_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int46_status_t
{
    ZXIC_UINT32 smmu0_int46_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT46_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int47_en_t
{
    ZXIC_UINT32 smmu0_int47_en0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT47_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int47_mask_t
{
    ZXIC_UINT32 smmu0_int47_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT47_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int47_status_t
{
    ZXIC_UINT32 smmu0_int47_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT47_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int48_en_t
{
    ZXIC_UINT32 smmu0_int48_en0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT48_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int48_mask_t
{
    ZXIC_UINT32 smmu0_int48_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT48_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int48_status_t
{
    ZXIC_UINT32 smmu0_int48_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT48_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int49_en_t
{
    ZXIC_UINT32 smmu0_int49_en0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT49_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int49_mask_t
{
    ZXIC_UINT32 smmu0_int49_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT49_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int49_status_t
{
    ZXIC_UINT32 smmu0_int49_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT49_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int50_en_t
{
    ZXIC_UINT32 smmu0_int50_en0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT50_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int50_mask_t
{
    ZXIC_UINT32 smmu0_int50_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT50_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int50_status_t
{
    ZXIC_UINT32 smmu0_int50_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT50_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int51_en_t
{
    ZXIC_UINT32 smmu0_int51_en0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT51_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int51_mask_t
{
    ZXIC_UINT32 smmu0_int51_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT51_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int51_status_t
{
    ZXIC_UINT32 smmu0_int51_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT51_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int52_en_t
{
    ZXIC_UINT32 smmu0_int52_en0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT52_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int52_mask_t
{
    ZXIC_UINT32 smmu0_int52_mask0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT52_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int52_status_t
{
    ZXIC_UINT32 smmu0_int52_status0_31;
}DPP_SMMU0_SMMU0_SMMU0_INT52_STATUS_T;

typedef struct dpp_smmu0_smmu0_smmu0_int53_en_t
{
    ZXIC_UINT32 smmu0_int53_en3;
    ZXIC_UINT32 smmu0_int53_en2;
    ZXIC_UINT32 smmu0_int53_en1;
    ZXIC_UINT32 smmu0_int53_en0;
}DPP_SMMU0_SMMU0_SMMU0_INT53_EN_T;

typedef struct dpp_smmu0_smmu0_smmu0_int53_mask_t
{
    ZXIC_UINT32 smmu0_int53_mask3;
    ZXIC_UINT32 smmu0_int53_mask2;
    ZXIC_UINT32 smmu0_int53_mask1;
    ZXIC_UINT32 smmu0_int53_mask0;
}DPP_SMMU0_SMMU0_SMMU0_INT53_MASK_T;

typedef struct dpp_smmu0_smmu0_smmu0_int53_status_t
{
    ZXIC_UINT32 smmu0_int53_status15;
    ZXIC_UINT32 smmu0_int53_status14;
    ZXIC_UINT32 smmu0_int53_status13;
    ZXIC_UINT32 smmu0_int53_status12;
}DPP_SMMU0_SMMU0_SMMU0_INT53_STATUS_T;

typedef struct dpp_smmu0_smmu0_ctrl0_arbiter_ecc_bypass_t
{
    ZXIC_UINT32 ctrl1_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl1_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl1_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl1_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl1_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl1_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl1_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl1_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl1_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl1_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl1_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl1_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl1_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl1_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl1_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl1_arbiter_ecc_bypass_0;
    ZXIC_UINT32 ctrl0_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl0_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl0_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl0_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl0_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl0_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl0_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl0_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl0_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl0_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl0_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl0_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl0_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl0_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl0_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl0_arbiter_ecc_bypass_0;
}DPP_SMMU0_SMMU0_CTRL0_ARBITER_ECC_BYPASS_T;

typedef struct dpp_smmu0_smmu0_ctrl2_arbiter_ecc_bypass_t
{
    ZXIC_UINT32 ctrl3_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl3_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl3_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl3_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl3_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl3_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl3_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl3_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl3_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl3_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl3_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl3_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl3_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl3_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl3_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl3_arbiter_ecc_bypass_0;
    ZXIC_UINT32 ctrl2_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl2_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl2_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl2_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl2_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl2_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl2_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl2_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl2_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl2_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl2_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl2_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl2_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl2_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl2_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl2_arbiter_ecc_bypass_0;
}DPP_SMMU0_SMMU0_CTRL2_ARBITER_ECC_BYPASS_T;

typedef struct dpp_smmu0_smmu0_ctrl4_arbiter_ecc_bypass_t
{
    ZXIC_UINT32 ctrl5_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl5_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl5_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl5_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl5_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl5_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl5_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl5_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl5_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl5_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl5_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl5_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl5_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl5_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl5_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl5_arbiter_ecc_bypass_0;
    ZXIC_UINT32 ctrl4_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl4_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl4_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl4_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl4_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl4_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl4_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl4_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl4_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl4_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl4_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl4_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl4_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl4_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl4_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl4_arbiter_ecc_bypass_0;
}DPP_SMMU0_SMMU0_CTRL4_ARBITER_ECC_BYPASS_T;

typedef struct dpp_smmu0_smmu0_ctrl6_arbiter_ecc_bypass_t
{
    ZXIC_UINT32 ctrl7_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl7_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl7_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl7_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl7_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl7_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl7_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl7_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl7_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl7_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl7_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl7_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl7_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl7_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl7_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl7_arbiter_ecc_bypass_0;
    ZXIC_UINT32 ctrl6_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl6_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl6_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl6_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl6_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl6_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl6_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl6_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl6_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl6_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl6_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl6_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl6_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl6_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl6_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl6_arbiter_ecc_bypass_0;
}DPP_SMMU0_SMMU0_CTRL6_ARBITER_ECC_BYPASS_T;

typedef struct dpp_smmu0_smmu0_ctrl8_arbiter_ecc_bypass_t
{
    ZXIC_UINT32 ctrl9_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl9_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl9_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl9_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl9_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl9_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl9_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl9_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl9_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl9_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl9_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl9_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl9_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl9_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl9_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl9_arbiter_ecc_bypass_0;
    ZXIC_UINT32 ctrl8_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl8_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl8_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl8_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl8_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl8_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl8_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl8_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl8_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl8_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl8_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl8_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl8_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl8_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl8_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl8_arbiter_ecc_bypass_0;
}DPP_SMMU0_SMMU0_CTRL8_ARBITER_ECC_BYPASS_T;

typedef struct dpp_smmu0_smmu0_ctrl10_arbiter_ecc_bypass_t
{
    ZXIC_UINT32 ctrl11_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl11_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl11_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl11_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl11_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl11_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl11_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl11_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl11_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl11_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl11_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl11_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl11_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl11_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl11_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl11_arbiter_ecc_bypass_0;
    ZXIC_UINT32 ctrl10_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl10_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl10_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl10_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl10_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl10_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl10_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl10_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl10_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl10_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl10_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl10_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl10_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl10_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl10_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl10_arbiter_ecc_bypass_0;
}DPP_SMMU0_SMMU0_CTRL10_ARBITER_ECC_BYPASS_T;

typedef struct dpp_smmu0_smmu0_ctrl12_arbiter_ecc_bypass_t
{
    ZXIC_UINT32 ctrl13_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl13_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl13_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl13_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl13_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl13_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl13_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl13_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl13_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl13_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl13_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl13_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl13_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl13_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl13_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl13_arbiter_ecc_bypass_0;
    ZXIC_UINT32 ctrl12_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl12_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl12_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl12_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl12_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl12_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl12_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl12_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl12_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl12_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl12_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl12_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl12_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl12_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl12_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl12_arbiter_ecc_bypass_0;
}DPP_SMMU0_SMMU0_CTRL12_ARBITER_ECC_BYPASS_T;

typedef struct dpp_smmu0_smmu0_ctrl14_arbiter_ecc_bypass_t
{
    ZXIC_UINT32 ctrl15_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl15_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl15_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl15_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl15_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl15_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl15_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl15_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl15_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl15_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl15_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl15_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl15_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl15_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl15_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl15_arbiter_ecc_bypass_0;
    ZXIC_UINT32 ctrl14_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl14_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl14_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl14_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl14_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl14_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl14_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl14_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl14_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl14_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl14_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl14_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl14_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl14_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl14_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl14_arbiter_ecc_bypass_0;
}DPP_SMMU0_SMMU0_CTRL14_ARBITER_ECC_BYPASS_T;

typedef struct dpp_smmu0_smmu0_ctrl16_arbiter_ecc_bypass_t
{
    ZXIC_UINT32 ctrl17_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl17_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl17_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl17_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl17_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl17_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl17_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl17_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl17_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl17_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl17_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl17_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl17_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl17_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl17_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl17_arbiter_ecc_bypass_0;
    ZXIC_UINT32 ctrl16_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl16_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl16_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl16_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl16_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl16_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl16_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl16_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl16_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl16_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl16_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl16_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl16_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl16_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl16_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl16_arbiter_ecc_bypass_0;
}DPP_SMMU0_SMMU0_CTRL16_ARBITER_ECC_BYPASS_T;

typedef struct dpp_smmu0_smmu0_ctrl18_arbiter_ecc_bypass_t
{
    ZXIC_UINT32 ctrl19_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl19_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl19_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl19_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl19_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl19_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl19_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl19_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl19_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl19_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl19_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl19_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl19_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl19_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl19_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl19_arbiter_ecc_bypass_0;
    ZXIC_UINT32 ctrl18_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl18_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl18_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl18_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl18_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl18_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl18_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl18_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl18_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl18_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl18_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl18_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl18_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl18_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl18_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl18_arbiter_ecc_bypass_0;
}DPP_SMMU0_SMMU0_CTRL18_ARBITER_ECC_BYPASS_T;

typedef struct dpp_smmu0_smmu0_ctrl20_arbiter_ecc_bypass_t
{
    ZXIC_UINT32 ctrl21_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl21_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl21_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl21_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl21_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl21_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl21_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl21_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl21_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl21_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl21_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl21_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl21_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl21_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl21_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl21_arbiter_ecc_bypass_0;
    ZXIC_UINT32 ctrl20_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl20_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl20_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl20_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl20_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl20_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl20_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl20_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl20_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl20_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl20_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl20_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl20_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl20_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl20_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl20_arbiter_ecc_bypass_0;
}DPP_SMMU0_SMMU0_CTRL20_ARBITER_ECC_BYPASS_T;

typedef struct dpp_smmu0_smmu0_ctrl22_arbiter_ecc_bypass_t
{
    ZXIC_UINT32 ctrl23_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl23_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl23_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl23_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl23_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl23_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl23_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl23_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl23_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl23_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl23_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl23_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl23_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl23_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl23_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl23_arbiter_ecc_bypass_0;
    ZXIC_UINT32 ctrl22_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl22_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl22_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl22_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl22_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl22_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl22_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl22_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl22_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl22_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl22_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl22_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl22_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl22_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl22_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl22_arbiter_ecc_bypass_0;
}DPP_SMMU0_SMMU0_CTRL22_ARBITER_ECC_BYPASS_T;

typedef struct dpp_smmu0_smmu0_ctrl24_arbiter_ecc_bypass_t
{
    ZXIC_UINT32 ctrl25_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl25_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl25_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl25_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl25_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl25_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl25_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl25_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl25_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl25_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl25_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl25_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl25_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl25_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl25_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl25_arbiter_ecc_bypass_0;
    ZXIC_UINT32 ctrl24_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl24_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl24_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl24_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl24_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl24_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl24_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl24_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl24_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl24_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl24_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl24_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl24_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl24_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl24_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl24_arbiter_ecc_bypass_0;
}DPP_SMMU0_SMMU0_CTRL24_ARBITER_ECC_BYPASS_T;

typedef struct dpp_smmu0_smmu0_ctrl26_arbiter_ecc_bypass_t
{
    ZXIC_UINT32 ctrl27_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl27_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl27_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl27_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl27_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl27_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl27_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl27_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl27_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl27_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl27_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl27_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl27_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl27_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl27_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl27_arbiter_ecc_bypass_0;
    ZXIC_UINT32 ctrl26_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl26_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl26_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl26_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl26_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl26_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl26_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl26_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl26_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl26_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl26_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl26_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl26_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl26_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl26_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl26_arbiter_ecc_bypass_0;
}DPP_SMMU0_SMMU0_CTRL26_ARBITER_ECC_BYPASS_T;

typedef struct dpp_smmu0_smmu0_ctrl28_arbiter_ecc_bypass_t
{
    ZXIC_UINT32 ctrl29_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl29_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl29_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl29_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl29_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl29_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl29_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl29_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl29_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl29_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl29_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl29_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl29_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl29_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl29_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl29_arbiter_ecc_bypass_0;
    ZXIC_UINT32 ctrl28_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl28_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl28_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl28_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl28_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl28_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl28_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl28_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl28_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl28_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl28_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl28_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl28_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl28_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl28_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl28_arbiter_ecc_bypass_0;
}DPP_SMMU0_SMMU0_CTRL28_ARBITER_ECC_BYPASS_T;

typedef struct dpp_smmu0_smmu0_ctrl30_arbiter_ecc_bypass_t
{
    ZXIC_UINT32 ctrl31_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl31_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl31_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl31_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl31_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl31_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl31_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl31_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl31_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl31_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl31_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl31_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl31_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl31_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl31_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl31_arbiter_ecc_bypass_0;
    ZXIC_UINT32 ctrl30_arbiter_ecc_bypass_15;
    ZXIC_UINT32 ctrl30_arbiter_ecc_bypass_14;
    ZXIC_UINT32 ctrl30_arbiter_ecc_bypass_13;
    ZXIC_UINT32 ctrl30_arbiter_ecc_bypass_12;
    ZXIC_UINT32 ctrl30_arbiter_ecc_bypass_11;
    ZXIC_UINT32 ctrl30_arbiter_ecc_bypass_10;
    ZXIC_UINT32 ctrl30_arbiter_ecc_bypass_9;
    ZXIC_UINT32 ctrl30_arbiter_ecc_bypass_8;
    ZXIC_UINT32 ctrl30_arbiter_ecc_bypass_7;
    ZXIC_UINT32 ctrl30_arbiter_ecc_bypass_6;
    ZXIC_UINT32 ctrl30_arbiter_ecc_bypass_5;
    ZXIC_UINT32 ctrl30_arbiter_ecc_bypass_4;
    ZXIC_UINT32 ctrl30_arbiter_ecc_bypass_3;
    ZXIC_UINT32 ctrl30_arbiter_ecc_bypass_2;
    ZXIC_UINT32 ctrl30_arbiter_ecc_bypass_1;
    ZXIC_UINT32 ctrl30_arbiter_ecc_bypass_0;
}DPP_SMMU0_SMMU0_CTRL30_ARBITER_ECC_BYPASS_T;

typedef struct dpp_smmu0_smmu0_ctrl_req_ecc_bypass_t
{
    ZXIC_UINT32 ctrl_req_ecc_bypass_0_31;
}DPP_SMMU0_SMMU0_CTRL_REQ_ECC_BYPASS_T;

typedef struct dpp_smmu0_smmu0_ctrl_info_ecc_bypass_t
{
    ZXIC_UINT32 ctrl_info_ecc_bypass_0_31;
}DPP_SMMU0_SMMU0_CTRL_INFO_ECC_BYPASS_T;

typedef struct dpp_smmu0_smmu0_smmu0_rschd_ecc_bypass_t
{
    ZXIC_UINT32 smmu0_rschd_ecc_bypass_0_31;
}DPP_SMMU0_SMMU0_SMMU0_RSCHD_ECC_BYPASS_T;

typedef struct dpp_smmu0_smmu0_smmu0_wr_ecc_bypass_t
{
    ZXIC_UINT32 smmu0_wr_ecc_bypass1;
    ZXIC_UINT32 smmu0_wr_ecc_bypass0;
}DPP_SMMU0_SMMU0_SMMU0_WR_ECC_BYPASS_T;

typedef struct dpp_smmu0_smmu0_ctrl0_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl0_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL0_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl1_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl1_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL1_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl2_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl2_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL2_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl3_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl3_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL3_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl4_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl4_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL4_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl5_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl5_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL5_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl6_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl6_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL6_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl7_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl7_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL7_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl8_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl8_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL8_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl9_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl9_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL9_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl10_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl10_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL10_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl11_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl11_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL11_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl12_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl12_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL12_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl13_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl13_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL13_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl14_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl14_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL14_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl15_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl15_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL15_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl16_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl16_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL16_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl17_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl17_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL17_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl18_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl18_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL18_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl19_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl19_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL19_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl20_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl20_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL20_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl21_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl21_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL21_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl22_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl22_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL22_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl23_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl23_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL23_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl24_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl24_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL24_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl25_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl25_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL25_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl26_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl26_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL26_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl27_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl27_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL27_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl28_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl28_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL28_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl29_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl29_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL29_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl30_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl30_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL30_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl31_arbiter_ecc_err_t
{
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_31;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_30;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_29;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_28;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_27;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_26;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_25;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_24;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_23;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_22;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_21;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_20;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_19;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_18;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_17;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_16;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_15;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_14;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_13;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_12;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_11;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_10;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_9;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_8;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_7;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_6;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_5;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_4;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_3;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_2;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_1;
    ZXIC_UINT32 ctrl31_arbiter_ecc_err_0;
}DPP_SMMU0_SMMU0_CTRL31_ARBITER_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl_req_ecc_single_err_t
{
    ZXIC_UINT32 ctrl_req_ecc_single_err_0_31;
}DPP_SMMU0_SMMU0_CTRL_REQ_ECC_SINGLE_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl_req_ecc_double_err_t
{
    ZXIC_UINT32 ctrl_req_ecc_double_err_0_31;
}DPP_SMMU0_SMMU0_CTRL_REQ_ECC_DOUBLE_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl_info_ecc_single_err_t
{
    ZXIC_UINT32 ctrl_info_ecc_single_err_0_31;
}DPP_SMMU0_SMMU0_CTRL_INFO_ECC_SINGLE_ERR_T;

typedef struct dpp_smmu0_smmu0_ctrl_info_ecc_double_err_t
{
    ZXIC_UINT32 ctrl_info_ecc_double_err_0_31;
}DPP_SMMU0_SMMU0_CTRL_INFO_ECC_DOUBLE_ERR_T;

typedef struct dpp_smmu0_smmu0_smmu0_wr_ecc_err_t
{
    ZXIC_UINT32 smmu0_wr_ecc_err_3;
    ZXIC_UINT32 smmu0_wr_ecc_err_2;
    ZXIC_UINT32 smmu0_wr_ecc_err_1;
    ZXIC_UINT32 smmu0_wr_ecc_err_0;
}DPP_SMMU0_SMMU0_SMMU0_WR_ECC_ERR_T;

typedef struct dpp_smmu0_smmu0_smmu0_rschd_ecc_single_err_t
{
    ZXIC_UINT32 smmu0_rschd_ecc_single_err_0_31;
}DPP_SMMU0_SMMU0_SMMU0_RSCHD_ECC_SINGLE_ERR_T;

typedef struct dpp_smmu0_smmu0_smmu0_rschd_ecc_double_err_t
{
    ZXIC_UINT32 smmu0_rschd_ecc_double_err_0_31;
}DPP_SMMU0_SMMU0_SMMU0_RSCHD_ECC_DOUBLE_ERR_T;

typedef struct dpp_smmu0_smmu0_ord_fifo_empty_t
{
    ZXIC_UINT32 ord_fifo_empty;
}DPP_SMMU0_SMMU0_ORD_FIFO_EMPTY_T;

typedef struct dpp_smmu0_smmu0_wr_arb_fifo_empty_t
{
    ZXIC_UINT32 wr_arb_fifo_empty;
}DPP_SMMU0_SMMU0_WR_ARB_FIFO_EMPTY_T;

typedef struct dpp_smmu0_smmu0_ctrl_fifo_empty0_t
{
    ZXIC_UINT32 ctrl_fifo_empty0_5;
    ZXIC_UINT32 ctrl_fifo_empty0_4;
    ZXIC_UINT32 ctrl_fifo_empty0_3;
    ZXIC_UINT32 ctrl_fifo_empty0_2;
    ZXIC_UINT32 ctrl_fifo_empty0_1;
    ZXIC_UINT32 ctrl_fifo_empty0_0;
}DPP_SMMU0_SMMU0_CTRL_FIFO_EMPTY0_T;

typedef struct dpp_smmu0_smmu0_ctrl_fifo_empty1_t
{
    ZXIC_UINT32 ctrl_fifo_empty1_5;
    ZXIC_UINT32 ctrl_fifo_empty1_4;
    ZXIC_UINT32 ctrl_fifo_empty1_3;
    ZXIC_UINT32 ctrl_fifo_empty1_2;
    ZXIC_UINT32 ctrl_fifo_empty1_1;
    ZXIC_UINT32 ctrl_fifo_empty1_0;
}DPP_SMMU0_SMMU0_CTRL_FIFO_EMPTY1_T;

typedef struct dpp_smmu0_smmu0_ctrl_fifo_empty2_t
{
    ZXIC_UINT32 ctrl_fifo_empty2_5;
    ZXIC_UINT32 ctrl_fifo_empty2_4;
    ZXIC_UINT32 ctrl_fifo_empty2_3;
    ZXIC_UINT32 ctrl_fifo_empty2_2;
    ZXIC_UINT32 ctrl_fifo_empty2_1;
    ZXIC_UINT32 ctrl_fifo_empty2_0;
}DPP_SMMU0_SMMU0_CTRL_FIFO_EMPTY2_T;

typedef struct dpp_smmu0_smmu0_ctrl_fifo_empty3_t
{
    ZXIC_UINT32 ctrl_fifo_empty3_5;
    ZXIC_UINT32 ctrl_fifo_empty3_4;
    ZXIC_UINT32 ctrl_fifo_empty3_3;
    ZXIC_UINT32 ctrl_fifo_empty3_2;
    ZXIC_UINT32 ctrl_fifo_empty3_1;
    ZXIC_UINT32 ctrl_fifo_empty3_0;
}DPP_SMMU0_SMMU0_CTRL_FIFO_EMPTY3_T;

typedef struct dpp_smmu0_smmu0_ctrl_fifo_empty4_t
{
    ZXIC_UINT32 ctrl_fifo_empty4_5;
    ZXIC_UINT32 ctrl_fifo_empty4_4;
    ZXIC_UINT32 ctrl_fifo_empty4_3;
    ZXIC_UINT32 ctrl_fifo_empty4_2;
    ZXIC_UINT32 ctrl_fifo_empty4_1;
    ZXIC_UINT32 ctrl_fifo_empty4_0;
}DPP_SMMU0_SMMU0_CTRL_FIFO_EMPTY4_T;

typedef struct dpp_smmu0_smmu0_ctrl_fifo_empty5_t
{
    ZXIC_UINT32 ctrl_fifo_empty5_1;
    ZXIC_UINT32 ctrl_fifo_empty5_0;
}DPP_SMMU0_SMMU0_CTRL_FIFO_EMPTY5_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty0_t
{
    ZXIC_UINT32 kschd_fifo_empty0;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY0_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty1_t
{
    ZXIC_UINT32 kschd_fifo_empty1;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY1_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty2_t
{
    ZXIC_UINT32 kschd_fifo_empty2;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY2_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty3_t
{
    ZXIC_UINT32 kschd_fifo_empty3;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY3_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty4_t
{
    ZXIC_UINT32 kschd_fifo_empty4;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY4_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty5_t
{
    ZXIC_UINT32 kschd_fifo_empty5;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY5_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty6_t
{
    ZXIC_UINT32 kschd_fifo_empty6;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY6_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty7_t
{
    ZXIC_UINT32 kschd_fifo_empty7;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY7_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty8_t
{
    ZXIC_UINT32 kschd_fifo_empty8;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY8_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty9_t
{
    ZXIC_UINT32 kschd_fifo_empty9;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY9_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty10_t
{
    ZXIC_UINT32 kschd_fifo_empty10;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY10_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty11_t
{
    ZXIC_UINT32 kschd_fifo_empty11;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY11_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty12_t
{
    ZXIC_UINT32 kschd_fifo_empty12;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY12_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty13_t
{
    ZXIC_UINT32 kschd_fifo_empty13;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY13_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty14_t
{
    ZXIC_UINT32 kschd_fifo_empty14;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY14_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty15_t
{
    ZXIC_UINT32 kschd_fifo_empty15;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY15_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty16_t
{
    ZXIC_UINT32 kschd_fifo_empty16;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY16_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty17_t
{
    ZXIC_UINT32 kschd_fifo_empty17;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY17_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty18_t
{
    ZXIC_UINT32 kschd_fifo_empty18;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY18_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty19_t
{
    ZXIC_UINT32 kschd_fifo_empty19;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY19_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty20_t
{
    ZXIC_UINT32 kschd_fifo_empty20;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY20_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty21_t
{
    ZXIC_UINT32 kschd_fifo_empty21;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY21_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty22_t
{
    ZXIC_UINT32 kschd_fifo_empty22;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY22_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty23_t
{
    ZXIC_UINT32 kschd_fifo_empty23;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY23_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty24_t
{
    ZXIC_UINT32 kschd_fifo_empty24;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY24_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty25_t
{
    ZXIC_UINT32 kschd_fifo_empty25;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY25_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty26_t
{
    ZXIC_UINT32 kschd_fifo_empty26;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY26_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty27_t
{
    ZXIC_UINT32 kschd_fifo_empty27;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY27_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty28_t
{
    ZXIC_UINT32 kschd_fifo_empty28;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY28_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty29_t
{
    ZXIC_UINT32 kschd_fifo_empty29;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY29_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty30_t
{
    ZXIC_UINT32 kschd_fifo_empty30;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY30_T;

typedef struct dpp_smmu0_smmu0_kschd_fifo_empty31_t
{
    ZXIC_UINT32 kschd_fifo_empty31;
}DPP_SMMU0_SMMU0_KSCHD_FIFO_EMPTY31_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty0_t
{
    ZXIC_UINT32 rschd_fifo_empty0;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY0_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty1_t
{
    ZXIC_UINT32 rschd_fifo_empty1;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY1_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty2_t
{
    ZXIC_UINT32 rschd_fifo_empty2;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY2_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty3_t
{
    ZXIC_UINT32 rschd_fifo_empty3;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY3_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty4_t
{
    ZXIC_UINT32 rschd_fifo_empty4;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY4_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty5_t
{
    ZXIC_UINT32 rschd_fifo_empty5;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY5_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty6_t
{
    ZXIC_UINT32 rschd_fifo_empty6;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY6_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty7_t
{
    ZXIC_UINT32 rschd_fifo_empty7;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY7_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty8_t
{
    ZXIC_UINT32 rschd_fifo_empty8;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY8_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty9_t
{
    ZXIC_UINT32 rschd_fifo_empty9;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY9_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty10_t
{
    ZXIC_UINT32 rschd_fifo_empty10;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY10_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty11_t
{
    ZXIC_UINT32 rschd_fifo_empty11;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY11_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty12_t
{
    ZXIC_UINT32 rschd_fifo_empty12;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY12_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty13_t
{
    ZXIC_UINT32 rschd_fifo_empty13;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY13_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty14_t
{
    ZXIC_UINT32 rschd_fifo_empty14;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY14_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty15_t
{
    ZXIC_UINT32 rschd_fifo_empty15;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY15_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty16_t
{
    ZXIC_UINT32 rschd_fifo_empty16;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY16_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty17_t
{
    ZXIC_UINT32 rschd_fifo_empty17;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY17_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty18_t
{
    ZXIC_UINT32 rschd_fifo_empty18;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY18_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty19_t
{
    ZXIC_UINT32 rschd_fifo_empty19;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY19_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty20_t
{
    ZXIC_UINT32 rschd_fifo_empty20;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY20_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty21_t
{
    ZXIC_UINT32 rschd_fifo_empty21;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY21_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty22_t
{
    ZXIC_UINT32 rschd_fifo_empty22;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY22_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty23_t
{
    ZXIC_UINT32 rschd_fifo_empty23;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY23_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty24_t
{
    ZXIC_UINT32 rschd_fifo_empty24;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY24_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty25_t
{
    ZXIC_UINT32 rschd_fifo_empty25;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY25_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty26_t
{
    ZXIC_UINT32 rschd_fifo_empty26;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY26_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty27_t
{
    ZXIC_UINT32 rschd_fifo_empty27;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY27_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty28_t
{
    ZXIC_UINT32 rschd_fifo_empty28;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY28_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty29_t
{
    ZXIC_UINT32 rschd_fifo_empty29;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY29_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty30_t
{
    ZXIC_UINT32 rschd_fifo_empty30;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY30_T;

typedef struct dpp_smmu0_smmu0_rschd_fifo_empty31_t
{
    ZXIC_UINT32 rschd_fifo_empty31;
}DPP_SMMU0_SMMU0_RSCHD_FIFO_EMPTY31_T;

typedef struct dpp_smmu0_smmu0_ept_flag_t
{
    ZXIC_UINT32 ept_flag8;
    ZXIC_UINT32 ept_flag7;
    ZXIC_UINT32 ept_flag6;
    ZXIC_UINT32 ept_flag5;
    ZXIC_UINT32 ept_flag4;
    ZXIC_UINT32 ept_flag3;
    ZXIC_UINT32 ept_flag2;
    ZXIC_UINT32 ept_flag1;
    ZXIC_UINT32 ept_flag0;
}DPP_SMMU0_SMMU0_EPT_FLAG_T;

typedef struct dpp_smmu0_smmu0_ppu_soft_rst_t
{
    ZXIC_UINT32 ppu_soft_rst;
}DPP_SMMU0_SMMU0_PPU_SOFT_RST_T;

typedef struct dpp_smmu0_smmu0_smmu0_as_mac_age_fc_cnt_t
{
    ZXIC_UINT32 smmu0_as_mac_age_fc_cnt;
}DPP_SMMU0_SMMU0_SMMU0_AS_MAC_AGE_FC_CNT_T;

typedef struct dpp_smmu0_smmu0_smmu0_marc_se_parser_fc_cnt_t
{
    ZXIC_UINT32 smmu0_marc_se_parser_fc_cnt;
}DPP_SMMU0_SMMU0_SMMU0_MARC_SE_PARSER_FC_CNT_T;

typedef struct dpp_smmu0_smmu0_wr_arb_cpu_fc_cnt_t
{
    ZXIC_UINT32 wr_arb_cpu_fc_cnt;
}DPP_SMMU0_SMMU0_WR_ARB_CPU_FC_CNT_T;

typedef struct dpp_smmu0_smmu0_smmu0_lpm_as_fc_cnt_t
{
    ZXIC_UINT32 smmu0_lpm_as_fc_cnt;
}DPP_SMMU0_SMMU0_SMMU0_LPM_AS_FC_CNT_T;

typedef struct dpp_smmu0_smmu0_lpm_as_smmu0_fc_cnt_t
{
    ZXIC_UINT32 lpm_as_smmu0_fc_cnt;
}DPP_SMMU0_SMMU0_LPM_AS_SMMU0_FC_CNT_T;

typedef struct dpp_smmu0_smmu0_smmu0_etcam1_0_as_fc_cnt_t
{
    ZXIC_UINT32 smmu0_etcam1_0_as_fc_cnt;
}DPP_SMMU0_SMMU0_SMMU0_ETCAM1_0_AS_FC_CNT_T;

typedef struct dpp_smmu0_smmu0_as_etcam1_0_smmu0_fc_cnt_t
{
    ZXIC_UINT32 as_etcam1_0_smmu0_fc_cnt;
}DPP_SMMU0_SMMU0_AS_ETCAM1_0_SMMU0_FC_CNT_T;

typedef struct dpp_smmu0_smmu0_smmu0_ppu_mcast_fc_cnt_t
{
    ZXIC_UINT32 smmu0_ppu_mcast_fc_cnt;
}DPP_SMMU0_SMMU0_SMMU0_PPU_MCAST_FC_CNT_T;

typedef struct dpp_smmu0_smmu0_ppu_smmu0_mcast_fc_cnt_t
{
    ZXIC_UINT32 ppu_smmu0_mcast_fc_cnt;
}DPP_SMMU0_SMMU0_PPU_SMMU0_MCAST_FC_CNT_T;

typedef struct dpp_smmu0_smmu0_odma_smmu0_tdm_fc_rsp_fc_cnt_t
{
    ZXIC_UINT32 odma_smmu0_tdm_fc_rsp_fc_cnt;
}DPP_SMMU0_SMMU0_ODMA_SMMU0_TDM_FC_RSP_FC_CNT_T;

typedef struct dpp_smmu0_smmu0_smmu0_odma_tdm_fc_key_fc_cnt_t
{
    ZXIC_UINT32 smmu0_odma_tdm_fc_key_fc_cnt;
}DPP_SMMU0_SMMU0_SMMU0_ODMA_TDM_FC_KEY_FC_CNT_T;

typedef struct dpp_smmu0_smmu0_smmu0_odma_fc_cnt_t
{
    ZXIC_UINT32 smmu0_odma_fc_cnt;
}DPP_SMMU0_SMMU0_SMMU0_ODMA_FC_CNT_T;

typedef struct dpp_smmu0_smmu0_smmu0_cfg_tab_rd_fc_cnt_t
{
    ZXIC_UINT32 smmu0_cfg_tab_rd_fc_cnt;
}DPP_SMMU0_SMMU0_SMMU0_CFG_TAB_RD_FC_CNT_T;

typedef struct dpp_smmu0_smmu0_smmu0_stat_fc15_0_cnt_t
{
    ZXIC_UINT32 smmu0_stat_fc15_0_cnt;
}DPP_SMMU0_SMMU0_SMMU0_STAT_FC15_0_CNT_T;

typedef struct dpp_smmu0_smmu0_stat_smmu0_fc15_0_cnt_t
{
    ZXIC_UINT32 stat_smmu0_fc15_0_cnt;
}DPP_SMMU0_SMMU0_STAT_SMMU0_FC15_0_CNT_T;

typedef struct dpp_smmu0_smmu0_smmu0_ppu_mex5_0_fc_cnt_t
{
    ZXIC_UINT32 smmu0_ppu_mex5_0_fc_cnt;
}DPP_SMMU0_SMMU0_SMMU0_PPU_MEX5_0_FC_CNT_T;

typedef struct dpp_smmu0_smmu0_ppu_smmu0_mex5_0_fc_cnt_t
{
    ZXIC_UINT32 ppu_smmu0_mex5_0_fc_cnt;
}DPP_SMMU0_SMMU0_PPU_SMMU0_MEX5_0_FC_CNT_T;

typedef struct dpp_smmu0_smmu0_as_smmu0_mac_age_req_cnt_t
{
    ZXIC_UINT32 as_smmu0_mac_age_req_cnt;
}DPP_SMMU0_SMMU0_AS_SMMU0_MAC_AGE_REQ_CNT_T;

typedef struct dpp_smmu0_smmu0_se_parser_smmu0_marc_key_cnt_t
{
    ZXIC_UINT32 se_parser_smmu0_marc_key_cnt;
}DPP_SMMU0_SMMU0_SE_PARSER_SMMU0_MARC_KEY_CNT_T;

typedef struct dpp_smmu0_smmu0_cpu_ind_rdat_cnt_t
{
    ZXIC_UINT32 cpu_ind_rdat_cnt;
}DPP_SMMU0_SMMU0_CPU_IND_RDAT_CNT_T;

typedef struct dpp_smmu0_smmu0_cpu_ind_rd_req_cnt_t
{
    ZXIC_UINT32 cpu_ind_rd_req_cnt;
}DPP_SMMU0_SMMU0_CPU_IND_RD_REQ_CNT_T;

typedef struct dpp_smmu0_smmu0_cpu_ind_wr_req_cnt_t
{
    ZXIC_UINT32 cpu_ind_wr_req_cnt;
}DPP_SMMU0_SMMU0_CPU_IND_WR_REQ_CNT_T;

typedef struct dpp_smmu0_smmu0_smmu0_plcr_rsp0_cnt_t
{
    ZXIC_UINT32 smmu0_plcr_rsp0_cnt;
}DPP_SMMU0_SMMU0_SMMU0_PLCR_RSP0_CNT_T;

typedef struct dpp_smmu0_smmu0_plcr_smmu0_req0_cnt_t
{
    ZXIC_UINT32 plcr_smmu0_req0_cnt;
}DPP_SMMU0_SMMU0_PLCR_SMMU0_REQ0_CNT_T;

typedef struct dpp_smmu0_smmu0_smmu0_lpm_as_rsp_cnt_t
{
    ZXIC_UINT32 smmu0_lpm_as_rsp_cnt;
}DPP_SMMU0_SMMU0_SMMU0_LPM_AS_RSP_CNT_T;

typedef struct dpp_smmu0_smmu0_lpm_as_smmu0_req_cnt_t
{
    ZXIC_UINT32 lpm_as_smmu0_req_cnt;
}DPP_SMMU0_SMMU0_LPM_AS_SMMU0_REQ_CNT_T;

typedef struct dpp_smmu0_smmu0_smmu0_etcam1_0_as_rsp_cnt_t
{
    ZXIC_UINT32 smmu0_etcam1_0_as_rsp_cnt;
}DPP_SMMU0_SMMU0_SMMU0_ETCAM1_0_AS_RSP_CNT_T;

typedef struct dpp_smmu0_smmu0_etcam1_0_as_smmu0_req_cnt_t
{
    ZXIC_UINT32 etcam1_0_as_smmu0_req_cnt;
}DPP_SMMU0_SMMU0_ETCAM1_0_AS_SMMU0_REQ_CNT_T;

typedef struct dpp_smmu0_smmu0_smmu0_ppu_mcast_rsp_cnt_t
{
    ZXIC_UINT32 smmu0_ppu_mcast_rsp_cnt;
}DPP_SMMU0_SMMU0_SMMU0_PPU_MCAST_RSP_CNT_T;

typedef struct dpp_smmu0_smmu0_ppu_smmu0_mcast_key_cnt_t
{
    ZXIC_UINT32 ppu_smmu0_mcast_key_cnt;
}DPP_SMMU0_SMMU0_PPU_SMMU0_MCAST_KEY_CNT_T;

typedef struct dpp_smmu0_smmu0_smmu0_odma_tdm_mc_rsp_cnt_t
{
    ZXIC_UINT32 smmu0_odma_tdm_mc_rsp_cnt;
}DPP_SMMU0_SMMU0_SMMU0_ODMA_TDM_MC_RSP_CNT_T;

typedef struct dpp_smmu0_smmu0_odma_smmu0_tdm_mc_key_cnt_t
{
    ZXIC_UINT32 odma_smmu0_tdm_mc_key_cnt;
}DPP_SMMU0_SMMU0_ODMA_SMMU0_TDM_MC_KEY_CNT_T;

typedef struct dpp_smmu0_smmu0_smmu0_odma_rsp_cnt_t
{
    ZXIC_UINT32 smmu0_odma_rsp_cnt;
}DPP_SMMU0_SMMU0_SMMU0_ODMA_RSP_CNT_T;

typedef struct dpp_smmu0_smmu0_odma_smmu0_cmd_cnt_t
{
    ZXIC_UINT32 odma_smmu0_cmd_cnt;
}DPP_SMMU0_SMMU0_ODMA_SMMU0_CMD_CNT_T;

typedef struct dpp_smmu0_smmu0_smmu0_cfg_tab_rdat_cnt_t
{
    ZXIC_UINT32 smmu0_cfg_tab_rdat_cnt;
}DPP_SMMU0_SMMU0_SMMU0_CFG_TAB_RDAT_CNT_T;

typedef struct dpp_smmu0_smmu0_cfg_smmu0_tab_rd_cnt_t
{
    ZXIC_UINT32 cfg_smmu0_tab_rd_cnt;
}DPP_SMMU0_SMMU0_CFG_SMMU0_TAB_RD_CNT_T;

typedef struct dpp_smmu0_smmu0_smmu0_stat_rsp15_0_cnt_t
{
    ZXIC_UINT32 smmu0_stat_rsp15_0_cnt;
}DPP_SMMU0_SMMU0_SMMU0_STAT_RSP15_0_CNT_T;

typedef struct dpp_smmu0_smmu0_stat_smmu0_req15_0_cnt_t
{
    ZXIC_UINT32 stat_smmu0_req15_0_cnt;
}DPP_SMMU0_SMMU0_STAT_SMMU0_REQ15_0_CNT_T;

typedef struct dpp_smmu0_smmu0_smmu0_ppu_mex5_0_rsp_cnt_t
{
    ZXIC_UINT32 smmu0_ppu_mex5_0_rsp_cnt;
}DPP_SMMU0_SMMU0_SMMU0_PPU_MEX5_0_RSP_CNT_T;

typedef struct dpp_smmu0_smmu0_ppu_smmu0_mex5_0_key_cnt_t
{
    ZXIC_UINT32 ppu_smmu0_mex5_0_key_cnt;
}DPP_SMMU0_SMMU0_PPU_SMMU0_MEX5_0_KEY_CNT_T;

typedef struct dpp_smmu0_smmu0_ftm_stat_smmu0_req0_cnt_t
{
    ZXIC_UINT32 ftm_stat_smmu0_req0_cnt;
}DPP_SMMU0_SMMU0_FTM_STAT_SMMU0_REQ0_CNT_T;

typedef struct dpp_smmu0_smmu0_ftm_stat_smmu0_req1_cnt_t
{
    ZXIC_UINT32 ftm_stat_smmu0_req1_cnt;
}DPP_SMMU0_SMMU0_FTM_STAT_SMMU0_REQ1_CNT_T;

typedef struct dpp_smmu0_smmu0_etm_stat_smmu0_req0_cnt_t
{
    ZXIC_UINT32 etm_stat_smmu0_req0_cnt;
}DPP_SMMU0_SMMU0_ETM_STAT_SMMU0_REQ0_CNT_T;

typedef struct dpp_smmu0_smmu0_etm_stat_smmu0_req1_cnt_t
{
    ZXIC_UINT32 etm_stat_smmu0_req1_cnt;
}DPP_SMMU0_SMMU0_ETM_STAT_SMMU0_REQ1_CNT_T;

typedef struct dpp_smmu0_smmu0_req_eram0_31_rd_cnt_t
{
    ZXIC_UINT32 req_eram0_31_rd_cnt;
}DPP_SMMU0_SMMU0_REQ_ERAM0_31_RD_CNT_T;

typedef struct dpp_smmu0_smmu0_req_eram0_31_wr_cnt_t
{
    ZXIC_UINT32 req_eram0_31_wr_cnt;
}DPP_SMMU0_SMMU0_REQ_ERAM0_31_WR_CNT_T;


#ifdef __cplusplus
}
#endif
#endif

