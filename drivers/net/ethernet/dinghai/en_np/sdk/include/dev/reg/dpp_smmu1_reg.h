
#ifndef _DPP_SMMU1_REG_H_
#define _DPP_SMMU1_REG_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef struct dpp_se_smmu1_ddr_wdat0_t
{
    ZXIC_UINT32 ddr_wdat0;
}DPP_SE_SMMU1_DDR_WDAT0_T;

typedef struct dpp_se_smmu1_dir_arbi_ser_rpful_t
{
    ZXIC_UINT32 dir_arbi_ser_rpful;
}DPP_SE_SMMU1_DIR_ARBI_SER_RPFUL_T;

typedef struct dpp_se_smmu1_cfg_wr_arbi_pful2_t
{
    ZXIC_UINT32 hash_wr_pful;
    ZXIC_UINT32 dir_wr_pful;
}DPP_SE_SMMU1_CFG_WR_ARBI_PFUL2_T;

typedef struct dpp_se_smmu1_etm_tbl_cfg_t
{
    ZXIC_UINT32 etm_baddr;
}DPP_SE_SMMU1_ETM_TBL_CFG_T;

typedef struct dpp_se_smmu1_cfg_cash_addr_pful_t
{
    ZXIC_UINT32 cfg_cash_addr_pful;
}DPP_SE_SMMU1_CFG_CASH_ADDR_PFUL_T;

typedef struct dpp_se_smmu1_ctrl_rfifo_cfg_t
{
    ZXIC_UINT32 brst_fwft_fifo_prog_empty_assert;
    ZXIC_UINT32 brst_fwft_fifo_prog_empty_negate;
    ZXIC_UINT32 brst_fwft_fifo_prog_full_assert;
    ZXIC_UINT32 brst_fwft_fifo_prog_full_negate;
}DPP_SE_SMMU1_CTRL_RFIFO_CFG_T;

typedef struct dpp_se_smmu1_cache_req_fifo_cfg_t
{
    ZXIC_UINT32 srch_fifo_pfull_assert;
    ZXIC_UINT32 srch_fifo_pfull_negate;
}DPP_SE_SMMU1_CACHE_REQ_FIFO_CFG_T;

typedef struct dpp_se_smmu1_ddr_wdat1_t
{
    ZXIC_UINT32 ddr_wdat1;
}DPP_SE_SMMU1_DDR_WDAT1_T;

typedef struct dpp_se_smmu1_ddr_wdat2_t
{
    ZXIC_UINT32 ddr_wdat2;
}DPP_SE_SMMU1_DDR_WDAT2_T;

typedef struct dpp_se_smmu1_ddr_wdat3_t
{
    ZXIC_UINT32 ddr_wdat3;
}DPP_SE_SMMU1_DDR_WDAT3_T;

typedef struct dpp_se_smmu1_ddr_wdat4_t
{
    ZXIC_UINT32 ddr_wdat4;
}DPP_SE_SMMU1_DDR_WDAT4_T;

typedef struct dpp_se_smmu1_ddr_wdat5_t
{
    ZXIC_UINT32 ddr_wdat5;
}DPP_SE_SMMU1_DDR_WDAT5_T;

typedef struct dpp_se_smmu1_ddr_wdat6_t
{
    ZXIC_UINT32 ddr_wdat6;
}DPP_SE_SMMU1_DDR_WDAT6_T;

typedef struct dpp_se_smmu1_ddr_wdat7_t
{
    ZXIC_UINT32 ddr_wdat7;
}DPP_SE_SMMU1_DDR_WDAT7_T;

typedef struct dpp_se_smmu1_ddr_wdat8_t
{
    ZXIC_UINT32 ddr_wdat8;
}DPP_SE_SMMU1_DDR_WDAT8_T;

typedef struct dpp_se_smmu1_ddr_wdat9_t
{
    ZXIC_UINT32 ddr_wdat9;
}DPP_SE_SMMU1_DDR_WDAT9_T;

typedef struct dpp_se_smmu1_ddr_wdat10_t
{
    ZXIC_UINT32 ddr_wdat10;
}DPP_SE_SMMU1_DDR_WDAT10_T;

typedef struct dpp_se_smmu1_ddr_wdat11_t
{
    ZXIC_UINT32 ddr_wdat11;
}DPP_SE_SMMU1_DDR_WDAT11_T;

typedef struct dpp_se_smmu1_ddr_wdat12_t
{
    ZXIC_UINT32 ddr_wdat12;
}DPP_SE_SMMU1_DDR_WDAT12_T;

typedef struct dpp_se_smmu1_ddr_wdat13_t
{
    ZXIC_UINT32 ddr_wdat13;
}DPP_SE_SMMU1_DDR_WDAT13_T;

typedef struct dpp_se_smmu1_ddr_wdat14_t
{
    ZXIC_UINT32 ddr_wdat14;
}DPP_SE_SMMU1_DDR_WDAT14_T;

typedef struct dpp_se_smmu1_ddr_wdat15_t
{
    ZXIC_UINT32 ddr_wdat15;
}DPP_SE_SMMU1_DDR_WDAT15_T;

typedef struct dpp_se_smmu1_cnt_stat_cache_en_t
{
    ZXIC_UINT32 cnt_stat_cache_en;
}DPP_SE_SMMU1_CNT_STAT_CACHE_EN_T;

typedef struct dpp_se_smmu1_cnt_stat_cache_clr_t
{
    ZXIC_UINT32 cnt_stat_cache_clr;
}DPP_SE_SMMU1_CNT_STAT_CACHE_CLR_T;

typedef struct dpp_se_smmu1_cnt_stat_cache_req_63_32_t
{
    ZXIC_UINT32 cnt_stat_cache_req_63_32;
}DPP_SE_SMMU1_CNT_STAT_CACHE_REQ_63_32_T;

typedef struct dpp_se_smmu1_cnt_stat_cache_req_31_0_t
{
    ZXIC_UINT32 cnt_stat_cache_req_31_0;
}DPP_SE_SMMU1_CNT_STAT_CACHE_REQ_31_0_T;

typedef struct dpp_se_smmu1_cnt_stat_cache_hit_63_32_t
{
    ZXIC_UINT32 cnt_stat_cache_hit_63_32;
}DPP_SE_SMMU1_CNT_STAT_CACHE_HIT_63_32_T;

typedef struct dpp_se_smmu1_cnt_stat_cache_hit_31_0_t
{
    ZXIC_UINT32 cnt_stat_cache_hit_31_0;
}DPP_SE_SMMU1_CNT_STAT_CACHE_HIT_31_0_T;

typedef struct dpp_se_smmu1_ddr_cmd0_t
{
    ZXIC_UINT32 ecc_en;
    ZXIC_UINT32 rw_len;
    ZXIC_UINT32 baddr;
}DPP_SE_SMMU1_DDR_CMD0_T;

typedef struct dpp_se_smmu1_info_addr_t
{
    ZXIC_UINT32 info_addr;
}DPP_SE_SMMU1_INFO_ADDR_T;

typedef struct dpp_se_smmu1_ddr_cmd1_t
{
    ZXIC_UINT32 rw_flag;
    ZXIC_UINT32 rw_addr;
}DPP_SE_SMMU1_DDR_CMD1_T;

typedef struct dpp_se_smmu1_clr_start_addr_t
{
    ZXIC_UINT32 clr_start_addr;
}DPP_SE_SMMU1_CLR_START_ADDR_T;

typedef struct dpp_se_smmu1_clr_end_addr_t
{
    ZXIC_UINT32 clr_end_addr;
}DPP_SE_SMMU1_CLR_END_ADDR_T;

typedef struct dpp_se_smmu1_clr_tbl_en_t
{
    ZXIC_UINT32 cfg_init_en;
    ZXIC_UINT32 clr_tbl_en;
}DPP_SE_SMMU1_CLR_TBL_EN_T;

typedef struct dpp_se_smmu1_debug_cnt_mode_t
{
    ZXIC_UINT32 cnt_rd_mode;
    ZXIC_UINT32 cnt_overflow_mode;
}DPP_SE_SMMU1_DEBUG_CNT_MODE_T;

typedef struct dpp_se_smmu1_init_done_t
{
    ZXIC_UINT32 cache_init_done;
    ZXIC_UINT32 clr_done;
    ZXIC_UINT32 init_ok;
}DPP_SE_SMMU1_INIT_DONE_T;

typedef struct dpp_se_smmu1_cpu_rsp_rd_done_t
{
    ZXIC_UINT32 cpu_rsp_rd_done;
}DPP_SE_SMMU1_CPU_RSP_RD_DONE_T;

typedef struct dpp_se_smmu1_ksch_oam_sp_en_t
{
    ZXIC_UINT32 ksch_oam_sp_en;
}DPP_SE_SMMU1_KSCH_OAM_SP_EN_T;

typedef struct dpp_se_smmu1_cfg_cache_en_t
{
    ZXIC_UINT32 cfg_cache_en;
}DPP_SE_SMMU1_CFG_CACHE_EN_T;

typedef struct dpp_se_smmu1_cache_age_en_t
{
    ZXIC_UINT32 cache_age_en;
}DPP_SE_SMMU1_CACHE_AGE_EN_T;

typedef struct dpp_se_smmu1_cpu_rdat0_t
{
    ZXIC_UINT32 cpu_rdat0;
}DPP_SE_SMMU1_CPU_RDAT0_T;

typedef struct dpp_se_smmu1_cpu_rdat1_t
{
    ZXIC_UINT32 cpu_rdat1;
}DPP_SE_SMMU1_CPU_RDAT1_T;

typedef struct dpp_se_smmu1_cpu_rdat2_t
{
    ZXIC_UINT32 cpu_rdat2;
}DPP_SE_SMMU1_CPU_RDAT2_T;

typedef struct dpp_se_smmu1_cpu_rdat3_t
{
    ZXIC_UINT32 cpu_rdat3;
}DPP_SE_SMMU1_CPU_RDAT3_T;

typedef struct dpp_se_smmu1_cpu_rdat4_t
{
    ZXIC_UINT32 cpu_rdat4;
}DPP_SE_SMMU1_CPU_RDAT4_T;

typedef struct dpp_se_smmu1_cpu_rdat5_t
{
    ZXIC_UINT32 cpu_rdat5;
}DPP_SE_SMMU1_CPU_RDAT5_T;

typedef struct dpp_se_smmu1_cpu_rdat6_t
{
    ZXIC_UINT32 cpu_rdat6;
}DPP_SE_SMMU1_CPU_RDAT6_T;

typedef struct dpp_se_smmu1_cpu_rdat7_t
{
    ZXIC_UINT32 cpu_rdat7;
}DPP_SE_SMMU1_CPU_RDAT7_T;

typedef struct dpp_se_smmu1_cpu_rdat8_t
{
    ZXIC_UINT32 cpu_rdat8;
}DPP_SE_SMMU1_CPU_RDAT8_T;

typedef struct dpp_se_smmu1_cpu_rdat9_t
{
    ZXIC_UINT32 cpu_rdat9;
}DPP_SE_SMMU1_CPU_RDAT9_T;

typedef struct dpp_se_smmu1_cpu_rdat10_t
{
    ZXIC_UINT32 cpu_rdat10;
}DPP_SE_SMMU1_CPU_RDAT10_T;

typedef struct dpp_se_smmu1_cpu_rdat11_t
{
    ZXIC_UINT32 cpu_rdat11;
}DPP_SE_SMMU1_CPU_RDAT11_T;

typedef struct dpp_se_smmu1_cpu_rdat12_t
{
    ZXIC_UINT32 cpu_rdat12;
}DPP_SE_SMMU1_CPU_RDAT12_T;

typedef struct dpp_se_smmu1_cpu_rdat13_t
{
    ZXIC_UINT32 cpu_rdat13;
}DPP_SE_SMMU1_CPU_RDAT13_T;

typedef struct dpp_se_smmu1_cpu_rdat14_t
{
    ZXIC_UINT32 cpu_rdat14;
}DPP_SE_SMMU1_CPU_RDAT14_T;

typedef struct dpp_se_smmu1_cpu_rdat15_t
{
    ZXIC_UINT32 cpu_rdat15;
}DPP_SE_SMMU1_CPU_RDAT15_T;

typedef struct dpp_se_smmu1_ctrl_cpu_rd_rdy_t
{
    ZXIC_UINT32 ctrl_cpu_rd_rdy;
}DPP_SE_SMMU1_CTRL_CPU_RD_RDY_T;

typedef struct dpp_se_smmu1_cpu_warbi_rdy_cfg_t
{
    ZXIC_UINT32 cpu_warbi_rdy_cfg;
}DPP_SE_SMMU1_CPU_WARBI_RDY_CFG_T;

typedef struct dpp_se_smmu1_dir_arbi_cpu_rpful_t
{
    ZXIC_UINT32 smmu1_cfg_rpful;
    ZXIC_UINT32 smmu1_cfg_wpful;
}DPP_SE_SMMU1_DIR_ARBI_CPU_RPFUL_T;

typedef struct dpp_se_smmu1_dir_arbi_wpful_t
{
    ZXIC_UINT32 smmu1_ser_wdir_pful;
    ZXIC_UINT32 smmu1_cfg_wdir_pful;
}DPP_SE_SMMU1_DIR_ARBI_WPFUL_T;

typedef struct dpp_se_smmu1_cfg_wr_arbi_pful0_t
{
    ZXIC_UINT32 arbi_out_pful;
    ZXIC_UINT32 cpu_wr_pful;
}DPP_SE_SMMU1_CFG_WR_ARBI_PFUL0_T;

typedef struct dpp_se_smmu1_cfg_wr_arbi_pful1_t
{
    ZXIC_UINT32 tm_wr_pful;
    ZXIC_UINT32 stat_wr_pful;
}DPP_SE_SMMU1_CFG_WR_ARBI_PFUL1_T;

typedef struct dpp_se_smmu1_smmu1_wdone_pful_cfg_t
{
    ZXIC_UINT32 smmu1_wdone_pful_cfg;
}DPP_SE_SMMU1_SMMU1_WDONE_PFUL_CFG_T;

typedef struct dpp_se_smmu1_stat_rate_cfg_cnt_t
{
    ZXIC_UINT32 stat_rate_cfg_cnt;
}DPP_SE_SMMU1_STAT_RATE_CFG_CNT_T;

typedef struct dpp_se_smmu1_ftm_rate_cfg_cnt_t
{
    ZXIC_UINT32 ftm_rate_cfg_cnt;
}DPP_SE_SMMU1_FTM_RATE_CFG_CNT_T;

typedef struct dpp_se_smmu1_etm_rate_cfg_cnt_t
{
    ZXIC_UINT32 etm_rate_cfg_cnt;
}DPP_SE_SMMU1_ETM_RATE_CFG_CNT_T;

typedef struct dpp_se_smmu1_dir_rate_cfg_cnt_t
{
    ZXIC_UINT32 dir_rate_cfg_cnt;
}DPP_SE_SMMU1_DIR_RATE_CFG_CNT_T;

typedef struct dpp_se_smmu1_hash_rate_cfg_cnt_t
{
    ZXIC_UINT32 hash_rate_cfg_cnt;
}DPP_SE_SMMU1_HASH_RATE_CFG_CNT_T;

typedef struct dpp_se_smmu1_ftm_tbl_cfg_t
{
    ZXIC_UINT32 ftm_baddr;
}DPP_SE_SMMU1_FTM_TBL_CFG_T;

typedef struct dpp_se_smmu1_lpm_v4_as_tbl_cfg_t
{
    ZXIC_UINT32 lpm_v4_as_rsp_len;
    ZXIC_UINT32 lpm_v4_as_ecc_en;
    ZXIC_UINT32 lpm_v4_as_baddr;
}DPP_SE_SMMU1_LPM_V4_AS_TBL_CFG_T;

typedef struct dpp_se_smmu1_lpm_v4_tbl_cfg_t
{
    ZXIC_UINT32 lpm_v4_len;
    ZXIC_UINT32 lpm_v4_ecc_en;
    ZXIC_UINT32 lpm_v4_baddr;
}DPP_SE_SMMU1_LPM_V4_TBL_CFG_T;

typedef struct dpp_se_smmu1_lpm_v6_tbl_cfg_t
{
    ZXIC_UINT32 lpm_v6_len;
    ZXIC_UINT32 lpm_v6_ecc_en;
    ZXIC_UINT32 lpm_v6_baddr;
}DPP_SE_SMMU1_LPM_V6_TBL_CFG_T;

typedef struct dpp_se_smmu1_lpm_v6_as_tbl_cfg_t
{
    ZXIC_UINT32 lpm_v6_as_rsp_len;
    ZXIC_UINT32 lpm_v6_as_ecc_en;
    ZXIC_UINT32 lpm_v6_as_baddr;
}DPP_SE_SMMU1_LPM_V6_AS_TBL_CFG_T;

typedef struct dpp_se_smmu1_dma_tbl_cfg_t
{
    ZXIC_UINT32 dma_baddr;
}DPP_SE_SMMU1_DMA_TBL_CFG_T;

typedef struct dpp_se_smmu1_stat_mode_cfg_t
{
    ZXIC_UINT32 stat_mode;
}DPP_SE_SMMU1_STAT_MODE_CFG_T;

typedef struct dpp_se_smmu1_ctrl_rpar_cpu_pful_t
{
    ZXIC_UINT32 ctrl_rpar_cpu_pful;
}DPP_SE_SMMU1_CTRL_RPAR_CPU_PFUL_T;

typedef struct dpp_se_smmu1_cfg_ksch_dir_pful_t
{
    ZXIC_UINT32 cfg_ksch_dir_pful;
}DPP_SE_SMMU1_CFG_KSCH_DIR_PFUL_T;

typedef struct dpp_se_smmu1_cfg_ksch_hash_pful_t
{
    ZXIC_UINT32 cfg_ksch_hash_pful;
}DPP_SE_SMMU1_CFG_KSCH_HASH_PFUL_T;

typedef struct dpp_se_smmu1_cfg_ksch_lpm_pful_t
{
    ZXIC_UINT32 cfg_ksch_lpm_pful;
}DPP_SE_SMMU1_CFG_KSCH_LPM_PFUL_T;

typedef struct dpp_se_smmu1_cfg_ksch_lpm_as_pful_t
{
    ZXIC_UINT32 cfg_ksch_lpm_as_pful;
}DPP_SE_SMMU1_CFG_KSCH_LPM_AS_PFUL_T;

typedef struct dpp_se_smmu1_cfg_ksch_stat_pful_t
{
    ZXIC_UINT32 cfg_ksch_stat_pful;
}DPP_SE_SMMU1_CFG_KSCH_STAT_PFUL_T;

typedef struct dpp_se_smmu1_cfg_ksch_tm_pful_t
{
    ZXIC_UINT32 cfg_ksch_tm_pful;
}DPP_SE_SMMU1_CFG_KSCH_TM_PFUL_T;

typedef struct dpp_se_smmu1_cfg_ksch_oam_pful_t
{
    ZXIC_UINT32 cfg_ksch_oam_pful;
}DPP_SE_SMMU1_CFG_KSCH_OAM_PFUL_T;

typedef struct dpp_se_smmu1_cfg_ksch_dma_pful_t
{
    ZXIC_UINT32 cfg_ksch_dma_pful;
}DPP_SE_SMMU1_CFG_KSCH_DMA_PFUL_T;

typedef struct dpp_se_smmu1_ctrl_wfifo_cfg_t
{
    ZXIC_UINT32 ctrl_wfifo_cfg;
}DPP_SE_SMMU1_CTRL_WFIFO_CFG_T;

typedef struct dpp_se_smmu1_rsch_hash_ptr_cfg_t
{
    ZXIC_UINT32 rsch_hash_ptr_cfg;
}DPP_SE_SMMU1_RSCH_HASH_PTR_CFG_T;

typedef struct dpp_se_smmu1_rsch_lpm_ptr_cfg_t
{
    ZXIC_UINT32 rsch_lpm_ptr_cfg;
}DPP_SE_SMMU1_RSCH_LPM_PTR_CFG_T;

typedef struct dpp_se_smmu1_rsch_lpm_as_ptr_cfg_t
{
    ZXIC_UINT32 rsch_lpm_as_ptr_cfg;
}DPP_SE_SMMU1_RSCH_LPM_AS_PTR_CFG_T;

typedef struct dpp_se_smmu1_rsch_stat_ptr_cfg_t
{
    ZXIC_UINT32 rsch_stat_ptr_cfg;
}DPP_SE_SMMU1_RSCH_STAT_PTR_CFG_T;

typedef struct dpp_se_smmu1_rsch_oam_ptr_cfg_t
{
    ZXIC_UINT32 rsch_oam_ptr_cfg;
}DPP_SE_SMMU1_RSCH_OAM_PTR_CFG_T;

typedef struct dpp_se_smmu1_rschd_fifo_pept_cfg_t
{
    ZXIC_UINT32 rschd_fifo_pept_cfg;
}DPP_SE_SMMU1_RSCHD_FIFO_PEPT_CFG_T;

typedef struct dpp_se_smmu1_dir_fifo_pful_cfg_t
{
    ZXIC_UINT32 dir_fifo_pful_cfg;
}DPP_SE_SMMU1_DIR_FIFO_PFUL_CFG_T;

typedef struct dpp_se_smmu1_hash_fifo_pful_cfg_t
{
    ZXIC_UINT32 hash_fifo_pful_cfg;
}DPP_SE_SMMU1_HASH_FIFO_PFUL_CFG_T;

typedef struct dpp_se_smmu1_lpm_fifo_pful_cfg_t
{
    ZXIC_UINT32 lpm_fifo_pful_cfg;
}DPP_SE_SMMU1_LPM_FIFO_PFUL_CFG_T;

typedef struct dpp_se_smmu1_lpm_as_fifo_pful_cfg_t
{
    ZXIC_UINT32 lpm_as_fifo_pful_cfg;
}DPP_SE_SMMU1_LPM_AS_FIFO_PFUL_CFG_T;

typedef struct dpp_se_smmu1_stat_fifo_pful_cfg_t
{
    ZXIC_UINT32 stat_fifo_pful_cfg;
}DPP_SE_SMMU1_STAT_FIFO_PFUL_CFG_T;

typedef struct dpp_se_smmu1_ftm_fifo_pful_cfg_t
{
    ZXIC_UINT32 ftm_fifo_pful_cfg;
}DPP_SE_SMMU1_FTM_FIFO_PFUL_CFG_T;

typedef struct dpp_se_smmu1_etm_fifo_pful_cfg_t
{
    ZXIC_UINT32 etm_fifo_pful_cfg;
}DPP_SE_SMMU1_ETM_FIFO_PFUL_CFG_T;

typedef struct dpp_se_smmu1_oam_fifo_pful_cfg_t
{
    ZXIC_UINT32 oam_fifo_pful_cfg;
}DPP_SE_SMMU1_OAM_FIFO_PFUL_CFG_T;

typedef struct dpp_se_smmu1_dma_fifo_pful_cfg_t
{
    ZXIC_UINT32 dma_fifo_pful_cfg;
}DPP_SE_SMMU1_DMA_FIFO_PFUL_CFG_T;

typedef struct dpp_se_smmu1_cache_rsp_rr_fifo_cfg_t
{
    ZXIC_UINT32 rr_pfull_assert0;
    ZXIC_UINT32 rr_pfull_negate0;
}DPP_SE_SMMU1_CACHE_RSP_RR_FIFO_CFG_T;

typedef struct dpp_se_smmu1_ddr_rsp_rr_fifo_cfg_t
{
    ZXIC_UINT32 rr_pfull_assert1;
    ZXIC_UINT32 rr_pfull_negate1;
}DPP_SE_SMMU1_DDR_RSP_RR_FIFO_CFG_T;

typedef struct dpp_se_smmu1_cpu_cahce_fifo_cfg_t
{
    ZXIC_UINT32 smmu1_cahce_fwft_fifo_pfull_assert;
    ZXIC_UINT32 smmu1_cahce_fwft_fifo_pfull_negate;
}DPP_SE_SMMU1_CPU_CAHCE_FIFO_CFG_T;

typedef struct dpp_se_smmu1_cache_rsp_fifo_cfg_t
{
    ZXIC_UINT32 rschd_fifo_pfull_assert;
    ZXIC_UINT32 rschd_fifo_pfull_negate;
}DPP_SE_SMMU1_CACHE_RSP_FIFO_CFG_T;

typedef struct dpp_se_smmu1_test_state_t
{
    ZXIC_UINT32 test_state;
}DPP_SE_SMMU1_TEST_STATE_T;

typedef struct dpp_se_smmu1_cache_fifo_ept_t
{
    ZXIC_UINT32 cache_fifo_ept;
}DPP_SE_SMMU1_CACHE_FIFO_EPT_T;

typedef struct dpp_se_smmu1_rr_fifo_ept_t
{
    ZXIC_UINT32 rr_fifo_ept;
}DPP_SE_SMMU1_RR_FIFO_EPT_T;

typedef struct dpp_se_smmu1_wr_fifo_ept_t
{
    ZXIC_UINT32 dir_arbi_ept;
}DPP_SE_SMMU1_WR_FIFO_EPT_T;

typedef struct dpp_se_smmu1_wdone_fifo_ept_t
{
    ZXIC_UINT32 wdone_fifo_ept;
}DPP_SE_SMMU1_WDONE_FIFO_EPT_T;

typedef struct dpp_se_smmu1_kschd_fifo_ept0_t
{
    ZXIC_UINT32 kschd_fifo_ept0;
}DPP_SE_SMMU1_KSCHD_FIFO_EPT0_T;

typedef struct dpp_se_smmu1_cash_fifo_ept_t
{
    ZXIC_UINT32 cash_fifo_ept;
}DPP_SE_SMMU1_CASH_FIFO_EPT_T;

typedef struct dpp_se_smmu1_ctrl_fifo_ept_t
{
    ZXIC_UINT32 ctrl_fifo_ept;
}DPP_SE_SMMU1_CTRL_FIFO_EPT_T;

typedef struct dpp_se_smmu1_smmu1_rschd_ept3_t
{
    ZXIC_UINT32 rschd_fifo_ept3;
}DPP_SE_SMMU1_SMMU1_RSCHD_EPT3_T;

typedef struct dpp_se_smmu1_smmu1_rschd_ept2_t
{
    ZXIC_UINT32 rschd_fifo_ept2;
}DPP_SE_SMMU1_SMMU1_RSCHD_EPT2_T;

typedef struct dpp_se_smmu1_smmu1_rschd_ept1_t
{
    ZXIC_UINT32 rschd_fifo_ept1;
}DPP_SE_SMMU1_SMMU1_RSCHD_EPT1_T;

typedef struct dpp_se_smmu1_smmu1_rschd_ept0_t
{
    ZXIC_UINT32 rschd_fifo_ept0;
}DPP_SE_SMMU1_SMMU1_RSCHD_EPT0_T;

typedef struct dpp_se_smmu1_cash0_ecc_err_addr_t
{
    ZXIC_UINT32 cash0_ecc_err_addr;
}DPP_SE_SMMU1_CASH0_ECC_ERR_ADDR_T;

typedef struct dpp_se_smmu1_arbi_cpu_wr_rdy_t
{
    ZXIC_UINT32 arbi_cpu_wr_rdy;
}DPP_SE_SMMU1_ARBI_CPU_WR_RDY_T;

typedef struct dpp_se_smmu1_smmu1_int_0_en_t
{
    ZXIC_UINT32 smmu1_int_0_en;
}DPP_SE_SMMU1_SMMU1_INT_0_EN_T;

typedef struct dpp_se_smmu1_smmu1_int_0_mask_t
{
    ZXIC_UINT32 smmu1_int_0_mask;
}DPP_SE_SMMU1_SMMU1_INT_0_MASK_T;

typedef struct dpp_se_smmu1_smmu1_int_1_en_t
{
    ZXIC_UINT32 smmu1_int_1_en;
}DPP_SE_SMMU1_SMMU1_INT_1_EN_T;

typedef struct dpp_se_smmu1_smmu1_int_1_mask_t
{
    ZXIC_UINT32 smmu1_int_1_mask;
}DPP_SE_SMMU1_SMMU1_INT_1_MASK_T;

typedef struct dpp_se_smmu1_smmu1_int_2_en_t
{
    ZXIC_UINT32 smmu1_int_2_en;
}DPP_SE_SMMU1_SMMU1_INT_2_EN_T;

typedef struct dpp_se_smmu1_smmu1_int_2_mask_t
{
    ZXIC_UINT32 smmu1_int_2_mask;
}DPP_SE_SMMU1_SMMU1_INT_2_MASK_T;

typedef struct dpp_se_smmu1_smmu1_int_3_en_t
{
    ZXIC_UINT32 smmu1_int_3_en;
}DPP_SE_SMMU1_SMMU1_INT_3_EN_T;

typedef struct dpp_se_smmu1_smmu1_int_3_mask_t
{
    ZXIC_UINT32 smmu1_int_3_mask;
}DPP_SE_SMMU1_SMMU1_INT_3_MASK_T;

typedef struct dpp_se_smmu1_smmu1_int_0_status_t
{
    ZXIC_UINT32 smmu1_int_0_status;
}DPP_SE_SMMU1_SMMU1_INT_0_STATUS_T;

typedef struct dpp_se_smmu1_smmu1_int_1_status_t
{
    ZXIC_UINT32 smmu1_int_1_status;
}DPP_SE_SMMU1_SMMU1_INT_1_STATUS_T;

typedef struct dpp_se_smmu1_smmu1_int_2_status_t
{
    ZXIC_UINT32 smmu1_int_2_status;
}DPP_SE_SMMU1_SMMU1_INT_2_STATUS_T;

typedef struct dpp_se_smmu1_smmu1_int_3_status_t
{
    ZXIC_UINT32 smmu1_int_3_status;
}DPP_SE_SMMU1_SMMU1_INT_3_STATUS_T;

typedef struct dpp_se_smmu1_smmu1_int_status_t
{
    ZXIC_UINT32 smmu1_int_status;
}DPP_SE_SMMU1_SMMU1_INT_STATUS_T;

typedef struct dpp_se_smmu1_ctrl_to_cash7_0_fc_cnt_t
{
    ZXIC_UINT32 ctrl_to_cash7_0_fc_cnt;
}DPP_SE_SMMU1_CTRL_TO_CASH7_0_FC_CNT_T;

typedef struct dpp_se_smmu1_cash7_0_to_ctrl_req_cnt_t
{
    ZXIC_UINT32 cash7_0_to_ctrl_req_cnt;
}DPP_SE_SMMU1_CASH7_0_TO_CTRL_REQ_CNT_T;

typedef struct dpp_se_smmu1_rschd_to_cache7_fc_cnt_t
{
    ZXIC_UINT32 rschd_to_cache7_fc_cnt;
}DPP_SE_SMMU1_RSCHD_TO_CACHE7_FC_CNT_T;

typedef struct dpp_se_smmu1_cash7_to_cache_rsp_cnt_t
{
    ZXIC_UINT32 cash7_to_cache_rsp_cnt;
}DPP_SE_SMMU1_CASH7_TO_CACHE_RSP_CNT_T;

typedef struct dpp_se_smmu1_cash7_to_ctrl_fc_cnt_t
{
    ZXIC_UINT32 cash7_to_ctrl_fc_cnt;
}DPP_SE_SMMU1_CASH7_TO_CTRL_FC_CNT_T;

typedef struct dpp_se_smmu1_ctrl_to_cash7_0_rsp_cnt_t
{
    ZXIC_UINT32 ctrl_to_cash7_0_rsp_cnt;
}DPP_SE_SMMU1_CTRL_TO_CASH7_0_RSP_CNT_T;

typedef struct dpp_se_smmu1_kschd_to_cache7_0_req_cnt_t
{
    ZXIC_UINT32 kschd_to_cache7_0_req_cnt;
}DPP_SE_SMMU1_KSCHD_TO_CACHE7_0_REQ_CNT_T;

typedef struct dpp_se_smmu1_cache7_0_to_kschd_fc_cnt_t
{
    ZXIC_UINT32 cache7_0_to_kschd_fc_cnt;
}DPP_SE_SMMU1_CACHE7_0_TO_KSCHD_FC_CNT_T;

typedef struct dpp_se_smmu1_dma_to_smmu1_rd_req_cnt_t
{
    ZXIC_UINT32 dma_to_smmu1_rd_req_cnt;
}DPP_SE_SMMU1_DMA_TO_SMMU1_RD_REQ_CNT_T;

typedef struct dpp_se_smmu1_oam_to_kschd_req_cnt_t
{
    ZXIC_UINT32 oam_to_kschd_req_cnt;
}DPP_SE_SMMU1_OAM_TO_KSCHD_REQ_CNT_T;

typedef struct dpp_se_smmu1_oam_rr_state_rsp_cnt_t
{
    ZXIC_UINT32 oam_rr_state_rsp_cnt;
}DPP_SE_SMMU1_OAM_RR_STATE_RSP_CNT_T;

typedef struct dpp_se_smmu1_oam_clash_info_cnt_t
{
    ZXIC_UINT32 oam_clash_info_cnt;
}DPP_SE_SMMU1_OAM_CLASH_INFO_CNT_T;

typedef struct dpp_se_smmu1_oam_to_rr_req_cnt_t
{
    ZXIC_UINT32 oam_to_rr_req_cnt;
}DPP_SE_SMMU1_OAM_TO_RR_REQ_CNT_T;

typedef struct dpp_se_smmu1_lpm_as_to_kschd_req_cnt_t
{
    ZXIC_UINT32 lpm_as_to_kschd_req_cnt;
}DPP_SE_SMMU1_LPM_AS_TO_KSCHD_REQ_CNT_T;

typedef struct dpp_se_smmu1_lpm_as_rr_state_rsp_cnt_t
{
    ZXIC_UINT32 lpm_as_rr_state_rsp_cnt;
}DPP_SE_SMMU1_LPM_AS_RR_STATE_RSP_CNT_T;

typedef struct dpp_se_smmu1_lpm_as_clash_info_cnt_t
{
    ZXIC_UINT32 lpm_as_clash_info_cnt;
}DPP_SE_SMMU1_LPM_AS_CLASH_INFO_CNT_T;

typedef struct dpp_se_smmu1_lpm_as_to_rr_req_cnt_t
{
    ZXIC_UINT32 lpm_as_to_rr_req_cnt;
}DPP_SE_SMMU1_LPM_AS_TO_RR_REQ_CNT_T;

typedef struct dpp_se_smmu1_lpm_to_kschd_req_cnt_t
{
    ZXIC_UINT32 lpm_to_kschd_req_cnt;
}DPP_SE_SMMU1_LPM_TO_KSCHD_REQ_CNT_T;

typedef struct dpp_se_smmu1_lpm_rr_state_rsp_cnt_t
{
    ZXIC_UINT32 lpm_rr_state_rsp_cnt;
}DPP_SE_SMMU1_LPM_RR_STATE_RSP_CNT_T;

typedef struct dpp_se_smmu1_lpm_clash_info_cnt_t
{
    ZXIC_UINT32 lpm_clash_info_cnt;
}DPP_SE_SMMU1_LPM_CLASH_INFO_CNT_T;

typedef struct dpp_se_smmu1_lpm_to_rr_req_cnt_t
{
    ZXIC_UINT32 lpm_to_rr_req_cnt;
}DPP_SE_SMMU1_LPM_TO_RR_REQ_CNT_T;

typedef struct dpp_se_smmu1_hash3_0_to_kschd_req_cnt_t
{
    ZXIC_UINT32 hash3_0_to_kschd_req_cnt;
}DPP_SE_SMMU1_HASH3_0_TO_KSCHD_REQ_CNT_T;

typedef struct dpp_se_smmu1_hash3_0_rr_state_rsp_cnt_t
{
    ZXIC_UINT32 hash3_0_rr_state_rsp_cnt;
}DPP_SE_SMMU1_HASH3_0_RR_STATE_RSP_CNT_T;

typedef struct dpp_se_smmu1_hash3_0_clash_info_cnt_t
{
    ZXIC_UINT32 hash3_0_clash_info_cnt;
}DPP_SE_SMMU1_HASH3_0_CLASH_INFO_CNT_T;

typedef struct dpp_se_smmu1_hash3_0_to_rr_req_cnt_t
{
    ZXIC_UINT32 hash3_0_to_rr_req_cnt;
}DPP_SE_SMMU1_HASH3_0_TO_RR_REQ_CNT_T;

typedef struct dpp_se_smmu1_dir3_0_to_kschd_req_cnt_t
{
    ZXIC_UINT32 dir3_0_to_kschd_req_cnt;
}DPP_SE_SMMU1_DIR3_0_TO_KSCHD_REQ_CNT_T;

typedef struct dpp_se_smmu1_dir3_0_clash_info_cnt_t
{
    ZXIC_UINT32 dir3_0_clash_info_cnt;
}DPP_SE_SMMU1_DIR3_0_CLASH_INFO_CNT_T;

typedef struct dpp_se_smmu1_dir_tbl_wr_req_cnt_t
{
    ZXIC_UINT32 dir_tbl_wr_req_cnt;
}DPP_SE_SMMU1_DIR_TBL_WR_REQ_CNT_T;

typedef struct dpp_se_smmu1_warbi_to_dir_tbl_warbi_fc_cnt_t
{
    ZXIC_UINT32 warbi_to_dir_tbl_warbi_fc_cnt;
}DPP_SE_SMMU1_WARBI_TO_DIR_TBL_WARBI_FC_CNT_T;

typedef struct dpp_se_smmu1_dir3_0_to_bank_rr_req_cnt_t
{
    ZXIC_UINT32 dir3_0_to_bank_rr_req_cnt;
}DPP_SE_SMMU1_DIR3_0_TO_BANK_RR_REQ_CNT_T;

typedef struct dpp_se_smmu1_kschd_to_dir3_0_fc_cnt_t
{
    ZXIC_UINT32 kschd_to_dir3_0_fc_cnt;
}DPP_SE_SMMU1_KSCHD_TO_DIR3_0_FC_CNT_T;

typedef struct dpp_se_smmu1_dir3_0_rr_state_rsp_cnt_t
{
    ZXIC_UINT32 dir3_0_rr_state_rsp_cnt;
}DPP_SE_SMMU1_DIR3_0_RR_STATE_RSP_CNT_T;

typedef struct dpp_se_smmu1_wr_done_to_warbi_fc_cnt_t
{
    ZXIC_UINT32 wr_done_to_warbi_fc_cnt;
}DPP_SE_SMMU1_WR_DONE_TO_WARBI_FC_CNT_T;

typedef struct dpp_se_smmu1_wr_done_ptr_req_cnt_t
{
    ZXIC_UINT32 wr_done_ptr_req_cnt;
}DPP_SE_SMMU1_WR_DONE_PTR_REQ_CNT_T;

typedef struct dpp_se_smmu1_ctrl7_0_to_warbi_fc_cnt_t
{
    ZXIC_UINT32 ctrl7_0_to_warbi_fc_cnt;
}DPP_SE_SMMU1_CTRL7_0_TO_WARBI_FC_CNT_T;

typedef struct dpp_se_smmu1_warbi_to_ctrl7_0_wr_req_cnt_t
{
    ZXIC_UINT32 warbi_to_ctrl7_0_wr_req_cnt;
}DPP_SE_SMMU1_WARBI_TO_CTRL7_0_WR_REQ_CNT_T;

typedef struct dpp_se_smmu1_warbi_to_cash7_0_wr_req_cnt_t
{
    ZXIC_UINT32 warbi_to_cash7_0_wr_req_cnt;
}DPP_SE_SMMU1_WARBI_TO_CASH7_0_WR_REQ_CNT_T;

typedef struct dpp_se_smmu1_warbi_to_cpu_wr_fc_cnt_t
{
    ZXIC_UINT32 warbi_to_cpu_wr_fc_cnt;
}DPP_SE_SMMU1_WARBI_TO_CPU_WR_FC_CNT_T;

typedef struct dpp_se_smmu1_cpu_wr_req_cnt_t
{
    ZXIC_UINT32 cpu_wr_req_cnt;
}DPP_SE_SMMU1_CPU_WR_REQ_CNT_T;

typedef struct dpp_se_smmu1_ctrl7_0_to_cpu_rd_rsp_cnt_t
{
    ZXIC_UINT32 ctrl7_0_to_cpu_rd_rsp_cnt;
}DPP_SE_SMMU1_CTRL7_0_TO_CPU_RD_RSP_CNT_T;

typedef struct dpp_se_smmu1_cpu_to_ctrl7_0_rd_req_cnt_t
{
    ZXIC_UINT32 cpu_to_ctrl7_0_rd_req_cnt;
}DPP_SE_SMMU1_CPU_TO_CTRL7_0_RD_REQ_CNT_T;

typedef struct dpp_se_smmu1_cpu_rd_dir_tbl_rsp_cnt_t
{
    ZXIC_UINT32 cpu_rd_dir_tbl_rsp_cnt;
}DPP_SE_SMMU1_CPU_RD_DIR_TBL_RSP_CNT_T;

typedef struct dpp_se_smmu1_cpu_to_dir_tbl_rd_wr_req_cnt_t
{
    ZXIC_UINT32 cpu_to_dir_tbl_rd_wr_req_cnt;
}DPP_SE_SMMU1_CPU_TO_DIR_TBL_RD_WR_REQ_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_mmu_7_0_rsp_fc_cnt_t
{
    ZXIC_UINT32 smmu1_to_mmu_7_0_rsp_fc_cnt;
}DPP_SE_SMMU1_SMMU1_TO_MMU_7_0_RSP_FC_CNT_T;

typedef struct dpp_se_smmu1_mmu_7_0_to_smmu1_rd_rsp_cnt_t
{
    ZXIC_UINT32 mmu_7_0_to_smmu1_rd_rsp_cnt;
}DPP_SE_SMMU1_MMU_7_0_TO_SMMU1_RD_RSP_CNT_T;

typedef struct dpp_se_smmu1_mmu_7_0_to_smmu1_rd_fc_cnt_t
{
    ZXIC_UINT32 mmu_7_0_to_smmu1_rd_fc_cnt;
}DPP_SE_SMMU1_MMU_7_0_TO_SMMU1_RD_FC_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_mmu_7_rd_req_cnt_t
{
    ZXIC_UINT32 smmu1_to_mmu_7_rd_req_cnt;
}DPP_SE_SMMU1_SMMU1_TO_MMU_7_RD_REQ_CNT_T;

typedef struct dpp_se_smmu1_mmu_7_to_smmu1_wr_fc_cnt_t
{
    ZXIC_UINT32 mmu_7_to_smmu1_wr_fc_cnt;
}DPP_SE_SMMU1_MMU_7_TO_SMMU1_WR_FC_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_mmu_7_0_wr_req_cnt_t
{
    ZXIC_UINT32 smmu1_to_mmu_7_0_wr_req_cnt;
}DPP_SE_SMMU1_SMMU1_TO_MMU_7_0_WR_REQ_CNT_T;

typedef struct dpp_se_smmu1_se_to_smmu1_wr_rsp_fc_cnt_t
{
    ZXIC_UINT32 se_to_smmu1_wr_rsp_fc_cnt;
}DPP_SE_SMMU1_SE_TO_SMMU1_WR_RSP_FC_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_se_wr_rsp_cnt_t
{
    ZXIC_UINT32 smmu1_to_se_wr_rsp_cnt;
}DPP_SE_SMMU1_SMMU1_TO_SE_WR_RSP_CNT_T;

typedef struct dpp_se_smmu1_ddr7_0_wr_rsp_cnt_t
{
    ZXIC_UINT32 ddr7_0_wr_rsp_cnt;
}DPP_SE_SMMU1_DDR7_0_WR_RSP_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_as_fc_cnt_t
{
    ZXIC_UINT32 smmu1_to_as_fc_cnt;
}DPP_SE_SMMU1_SMMU1_TO_AS_FC_CNT_T;

typedef struct dpp_se_smmu1_as_to_smmu1_wr_req_cnt_t
{
    ZXIC_UINT32 as_to_smmu1_wr_req_cnt;
}DPP_SE_SMMU1_AS_TO_SMMU1_WR_REQ_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_se_parser_fc_cnt_t
{
    ZXIC_UINT32 smmu1_to_se_parser_fc_cnt;
}DPP_SE_SMMU1_SMMU1_TO_SE_PARSER_FC_CNT_T;

typedef struct dpp_se_smmu1_se_parser_to_smmu1_req_cnt_t
{
    ZXIC_UINT32 se_parser_to_smmu1_req_cnt;
}DPP_SE_SMMU1_SE_PARSER_TO_SMMU1_REQ_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_etm_wr_fc_cnt_t
{
    ZXIC_UINT32 smmu1_to_etm_wr_fc_cnt;
}DPP_SE_SMMU1_SMMU1_TO_ETM_WR_FC_CNT_T;

typedef struct dpp_se_smmu1_etm_wr_req_cnt_t
{
    ZXIC_UINT32 etm_wr_req_cnt;
}DPP_SE_SMMU1_ETM_WR_REQ_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_ftm_wr_fc_cnt_t
{
    ZXIC_UINT32 smmu1_to_ftm_wr_fc_cnt;
}DPP_SE_SMMU1_SMMU1_TO_FTM_WR_FC_CNT_T;

typedef struct dpp_se_smmu1_ftm_wr_req_cnt_t
{
    ZXIC_UINT32 ftm_wr_req_cnt;
}DPP_SE_SMMU1_FTM_WR_REQ_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_state_wr_fc_cnt_t
{
    ZXIC_UINT32 smmu1_to_state_wr_fc_cnt;
}DPP_SE_SMMU1_SMMU1_TO_STATE_WR_FC_CNT_T;

typedef struct dpp_se_smmu1_state_wr_req_cnt_t
{
    ZXIC_UINT32 state_wr_req_cnt;
}DPP_SE_SMMU1_STATE_WR_REQ_CNT_T;

typedef struct dpp_se_smmu1_se_to_dma_rsp_cnt_t
{
    ZXIC_UINT32 se_to_dma_rsp_cnt;
}DPP_SE_SMMU1_SE_TO_DMA_RSP_CNT_T;

typedef struct dpp_se_smmu1_se_to_dma_fc_cnt_t
{
    ZXIC_UINT32 se_to_dma_fc_cnt;
}DPP_SE_SMMU1_SE_TO_DMA_FC_CNT_T;

typedef struct dpp_se_smmu1_oam_to_smmu1_fc_cnt_t
{
    ZXIC_UINT32 oam_to_smmu1_fc_cnt;
}DPP_SE_SMMU1_OAM_TO_SMMU1_FC_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_oam_rsp_cnt_t
{
    ZXIC_UINT32 smmu1_to_oam_rsp_cnt;
}DPP_SE_SMMU1_SMMU1_TO_OAM_RSP_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_oam_fc_cnt_t
{
    ZXIC_UINT32 smmu1_to_oam_fc_cnt;
}DPP_SE_SMMU1_SMMU1_TO_OAM_FC_CNT_T;

typedef struct dpp_se_smmu1_oam_to_smmu1_req_cnt_t
{
    ZXIC_UINT32 oam_to_smmu1_req_cnt;
}DPP_SE_SMMU1_OAM_TO_SMMU1_REQ_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_etm_rsp_cnt_t
{
    ZXIC_UINT32 smmu1_to_etm_rsp_cnt;
}DPP_SE_SMMU1_SMMU1_TO_ETM_RSP_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_ftm_rsp_cnt_t
{
    ZXIC_UINT32 smmu1_to_ftm_rsp_cnt;
}DPP_SE_SMMU1_SMMU1_TO_FTM_RSP_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_etm_fc_cnt_t
{
    ZXIC_UINT32 smmu1_to_etm_fc_cnt;
}DPP_SE_SMMU1_SMMU1_TO_ETM_FC_CNT_T;

typedef struct dpp_se_smmu1_etm_to_smmu1_req_cnt_t
{
    ZXIC_UINT32 etm_to_smmu1_req_cnt;
}DPP_SE_SMMU1_ETM_TO_SMMU1_REQ_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_ftm_fc_cnt_t
{
    ZXIC_UINT32 smmu1_to_ftm_fc_cnt;
}DPP_SE_SMMU1_SMMU1_TO_FTM_FC_CNT_T;

typedef struct dpp_se_smmu1_ftm_to_smmu1_req_cnt_t
{
    ZXIC_UINT32 ftm_to_smmu1_req_cnt;
}DPP_SE_SMMU1_FTM_TO_SMMU1_REQ_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_stat_rsp_cnt_t
{
    ZXIC_UINT32 smmu1_to_stat_rsp_cnt;
}DPP_SE_SMMU1_SMMU1_TO_STAT_RSP_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_stat_fc_cnt_t
{
    ZXIC_UINT32 smmu1_to_stat_fc_cnt;
}DPP_SE_SMMU1_SMMU1_TO_STAT_FC_CNT_T;

typedef struct dpp_se_smmu1_stat_to_smmu1_req_cnt_t
{
    ZXIC_UINT32 stat_to_smmu1_req_cnt;
}DPP_SE_SMMU1_STAT_TO_SMMU1_REQ_CNT_T;

typedef struct dpp_se_smmu1_lpm_as_to_smmu1_fc_cnt_t
{
    ZXIC_UINT32 lpm_as_to_smmu1_fc_cnt;
}DPP_SE_SMMU1_LPM_AS_TO_SMMU1_FC_CNT_T;

typedef struct dpp_se_smmu1_lpm_to_smmu1_fc_cnt_t
{
    ZXIC_UINT32 lpm_to_smmu1_fc_cnt;
}DPP_SE_SMMU1_LPM_TO_SMMU1_FC_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_lpm_as_rsp_cnt_t
{
    ZXIC_UINT32 smmu1_to_lpm_as_rsp_cnt;
}DPP_SE_SMMU1_SMMU1_TO_LPM_AS_RSP_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_lpm_rsp_cnt_t
{
    ZXIC_UINT32 smmu1_to_lpm_rsp_cnt;
}DPP_SE_SMMU1_SMMU1_TO_LPM_RSP_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_lpm_as_fc_cnt_t
{
    ZXIC_UINT32 smmu1_to_lpm_as_fc_cnt;
}DPP_SE_SMMU1_SMMU1_TO_LPM_AS_FC_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_lpm_fc_cnt_t
{
    ZXIC_UINT32 smmu1_to_lpm_fc_cnt;
}DPP_SE_SMMU1_SMMU1_TO_LPM_FC_CNT_T;

typedef struct dpp_se_smmu1_lpm_as_to_smmu1_req_cnt_t
{
    ZXIC_UINT32 lpm_as_to_smmu1_req_cnt;
}DPP_SE_SMMU1_LPM_AS_TO_SMMU1_REQ_CNT_T;

typedef struct dpp_se_smmu1_lpm_to_smmu1_req_cnt_t
{
    ZXIC_UINT32 lpm_to_smmu1_req_cnt;
}DPP_SE_SMMU1_LPM_TO_SMMU1_REQ_CNT_T;

typedef struct dpp_se_smmu1_hash3_0_to_smmu1_fc_cnt_t
{
    ZXIC_UINT32 hash3_0_to_smmu1_fc_cnt;
}DPP_SE_SMMU1_HASH3_0_TO_SMMU1_FC_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_hash3_0_rsp_cnt_t
{
    ZXIC_UINT32 smmu1_to_hash3_0_rsp_cnt;
}DPP_SE_SMMU1_SMMU1_TO_HASH3_0_RSP_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_hash3_0_fc_cnt_t
{
    ZXIC_UINT32 smmu1_to_hash3_0_fc_cnt;
}DPP_SE_SMMU1_SMMU1_TO_HASH3_0_FC_CNT_T;

typedef struct dpp_se_smmu1_hash3_0_to_smmu1_cnt_t
{
    ZXIC_UINT32 hash3_0_to_smmu1_cnt;
}DPP_SE_SMMU1_HASH3_0_TO_SMMU1_CNT_T;

typedef struct dpp_se_smmu1_se_to_smmu1_dir3_0_rsp_fc_cnt_t
{
    ZXIC_UINT32 se_to_smmu1_dir3_0_rsp_fc_cnt;
}DPP_SE_SMMU1_SE_TO_SMMU1_DIR3_0_RSP_FC_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_se_dir3_0_rsp_cnt_t
{
    ZXIC_UINT32 smmu1_to_se_dir3_0_rsp_cnt;
}DPP_SE_SMMU1_SMMU1_TO_SE_DIR3_0_RSP_CNT_T;

typedef struct dpp_se_smmu1_smmu1_to_se_dir3_0_fc_cnt_t
{
    ZXIC_UINT32 smmu1_to_se_dir3_0_fc_cnt;
}DPP_SE_SMMU1_SMMU1_TO_SE_DIR3_0_FC_CNT_T;

typedef struct dpp_se_smmu1_se_to_smmu1_dir3_0_cnt_t
{
    ZXIC_UINT32 se_to_smmu1_dir3_0_cnt;
}DPP_SE_SMMU1_SE_TO_SMMU1_DIR3_0_CNT_T;

typedef struct dpp_se_smmu1_cache7_0_to_rschd_rsp_cnt_t
{
    ZXIC_UINT32 cache7_0_to_rschd_rsp_cnt;
}DPP_SE_SMMU1_CACHE7_0_TO_RSCHD_RSP_CNT_T;

typedef struct dpp_se_cmmu_ddr_rw_addr_t
{
    ZXIC_UINT32 ddr_wr;
}DPP_SE_CMMU_DDR_RW_ADDR_T;

typedef struct dpp_se_cmmu_ddr_rw_mode_t
{
    ZXIC_UINT32 ddr_rw_flag;
    ZXIC_UINT32 ddr_rw_mode;
}DPP_SE_CMMU_DDR_RW_MODE_T;

typedef struct dpp_se_cmmu_cp_cmd_t
{
    ZXIC_UINT32 stat_tbl_baddr;
}DPP_SE_CMMU_CP_CMD_T;

typedef struct dpp_se_cmmu_cpu_ind_rd_done_t
{
    ZXIC_UINT32 cpu_ind_rd_done;
}DPP_SE_CMMU_CPU_IND_RD_DONE_T;

typedef struct dpp_se_cmmu_cpu_ind_rdat0_t
{
    ZXIC_UINT32 cpu_ind_rdat0;
}DPP_SE_CMMU_CPU_IND_RDAT0_T;

typedef struct dpp_se_cmmu_cpu_ind_rdat1_t
{
    ZXIC_UINT32 cpu_ind_rdat1;
}DPP_SE_CMMU_CPU_IND_RDAT1_T;

typedef struct dpp_se_cmmu_cpu_ind_rdat2_t
{
    ZXIC_UINT32 cpu_ind_rdat2;
}DPP_SE_CMMU_CPU_IND_RDAT2_T;

typedef struct dpp_se_cmmu_cpu_ind_rdat3_t
{
    ZXIC_UINT32 cpu_ind_rdat3;
}DPP_SE_CMMU_CPU_IND_RDAT3_T;

typedef struct dpp_se_cmmu_cpu_ddr_fifo_almful_t
{
    ZXIC_UINT32 cpu_ddr_fifo_almful;
}DPP_SE_CMMU_CPU_DDR_FIFO_ALMFUL_T;

typedef struct dpp_se_cmmu_debug_cnt_mode_t
{
    ZXIC_UINT32 cnt_rd_mode;
    ZXIC_UINT32 cnt_overflow_mode;
}DPP_SE_CMMU_DEBUG_CNT_MODE_T;

typedef struct dpp_se_cmmu_cmmu_pful_cfg_t
{
    ZXIC_UINT32 alu_cmd_pful_negate;
    ZXIC_UINT32 alu_cmd_pful_assert;
}DPP_SE_CMMU_CMMU_PFUL_CFG_T;

typedef struct dpp_se_cmmu_cmmu_stat_pful_cfg_t
{
    ZXIC_UINT32 cmmu_stat_pful_negate;
    ZXIC_UINT32 cmmu_stat_pful_assert;
}DPP_SE_CMMU_CMMU_STAT_PFUL_CFG_T;

typedef struct dpp_se_cmmu_stat_overflow_mode_t
{
    ZXIC_UINT32 stat_overflow_mode;
}DPP_SE_CMMU_STAT_OVERFLOW_MODE_T;

typedef struct dpp_se_cmmu_cmmu_cp_fifo_pful_t
{
    ZXIC_UINT32 cmmu_cp_fifo_pful;
}DPP_SE_CMMU_CMMU_CP_FIFO_PFUL_T;

typedef struct dpp_se_cmmu_ddr_wr_dat0_t
{
    ZXIC_UINT32 ddr_wr_dat0;
}DPP_SE_CMMU_DDR_WR_DAT0_T;

typedef struct dpp_se_cmmu_ddr_wr_dat1_t
{
    ZXIC_UINT32 ddr_wr_dat1;
}DPP_SE_CMMU_DDR_WR_DAT1_T;

typedef struct dpp_se_cmmu_cmmu_int_unmask_flag_t
{
    ZXIC_UINT32 cmmu_int_unmask_flag;
}DPP_SE_CMMU_CMMU_INT_UNMASK_FLAG_T;

typedef struct dpp_se_cmmu_cmmu_int_en_t
{
    ZXIC_UINT32 cmmu_int_en12;
    ZXIC_UINT32 cmmu_int_en11;
    ZXIC_UINT32 cmmu_int_en10;
    ZXIC_UINT32 cmmu_int_en9;
    ZXIC_UINT32 cmmu_int_en8;
    ZXIC_UINT32 cmmu_int_en7;
    ZXIC_UINT32 cmmu_int_en6;
    ZXIC_UINT32 cmmu_int_en5;
    ZXIC_UINT32 cmmu_int_en4;
    ZXIC_UINT32 cmmu_int_en3;
    ZXIC_UINT32 cmmu_int_en2;
    ZXIC_UINT32 cmmu_int_en1;
    ZXIC_UINT32 cmmu_int_en0;
}DPP_SE_CMMU_CMMU_INT_EN_T;

typedef struct dpp_se_cmmu_cmmu_int_mask_t
{
    ZXIC_UINT32 cmmu_int_mask12;
    ZXIC_UINT32 cmmu_int_mask11;
    ZXIC_UINT32 cmmu_int_mask10;
    ZXIC_UINT32 cmmu_int_mask9;
    ZXIC_UINT32 cmmu_int_mask8;
    ZXIC_UINT32 cmmu_int_mask7;
    ZXIC_UINT32 cmmu_int_mask6;
    ZXIC_UINT32 cmmu_int_mask5;
    ZXIC_UINT32 cmmu_int_mask4;
    ZXIC_UINT32 cmmu_int_mask3;
    ZXIC_UINT32 cmmu_int_mask2;
    ZXIC_UINT32 cmmu_int_mask1;
    ZXIC_UINT32 cmmu_int_mask0;
}DPP_SE_CMMU_CMMU_INT_MASK_T;

typedef struct dpp_se_cmmu_cmmu_int_status_t
{
    ZXIC_UINT32 cmmu_int_status12;
    ZXIC_UINT32 cmmu_int_status11;
    ZXIC_UINT32 cmmu_int_status10;
    ZXIC_UINT32 cmmu_int_status9;
    ZXIC_UINT32 cmmu_int_status8;
    ZXIC_UINT32 cmmu_int_status7;
    ZXIC_UINT32 cmmu_int_status6;
    ZXIC_UINT32 cmmu_int_status5;
    ZXIC_UINT32 cmmu_int_status4;
    ZXIC_UINT32 cmmu_int_status3;
    ZXIC_UINT32 cmmu_int_status2;
    ZXIC_UINT32 cmmu_int_status1;
    ZXIC_UINT32 cmmu_int_status0;
}DPP_SE_CMMU_CMMU_INT_STATUS_T;

typedef struct dpp_se_cmmu_stat_cmmu_req_cnt_t
{
    ZXIC_UINT32 stat_cmmu_req_cnt;
}DPP_SE_CMMU_STAT_CMMU_REQ_CNT_T;

typedef struct dpp_se_cmmu_cmmu_fc0_cnt_t
{
    ZXIC_UINT32 cmmu_stat_rdy;
}DPP_SE_CMMU_CMMU_FC0_CNT_T;

typedef struct dpp_se_cmmu_cmmu_fc1_cnt_t
{
    ZXIC_UINT32 smmu1_cmmu_wr_rdy;
}DPP_SE_CMMU_CMMU_FC1_CNT_T;

typedef struct dpp_se_cmmu_cmmu_fc2_cnt_t
{
    ZXIC_UINT32 smmu1_cmmu_rd_rdy;
}DPP_SE_CMMU_CMMU_FC2_CNT_T;


#ifdef __cplusplus
}
#endif
#endif

