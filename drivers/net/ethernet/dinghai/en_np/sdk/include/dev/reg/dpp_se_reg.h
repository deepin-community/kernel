
#ifndef _DPP_SE_REG_H_
#define _DPP_SE_REG_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef struct dpp_se_alg_init_ok_t
{
    ZXIC_UINT32 init_ok;
}DPP_SE_ALG_INIT_OK_T;

typedef struct dpp_se_alg_cpu_rd_rdy_t
{
    ZXIC_UINT32 cpu_rd_rdy;
}DPP_SE_ALG_CPU_RD_RDY_T;

typedef struct dpp_se_alg_cpu_rd_data_tmp0_t
{
    ZXIC_UINT32 cpu_rd_data_tmp0;
}DPP_SE_ALG_CPU_RD_DATA_TMP0_T;

typedef struct dpp_se_alg_cpu_rd_data_tmp1_t
{
    ZXIC_UINT32 cpu_rd_data_tmp1;
}DPP_SE_ALG_CPU_RD_DATA_TMP1_T;

typedef struct dpp_se_alg_cpu_rd_data_tmp2_t
{
    ZXIC_UINT32 cpu_rd_data_tmp2;
}DPP_SE_ALG_CPU_RD_DATA_TMP2_T;

typedef struct dpp_se_alg_cpu_rd_data_tmp3_t
{
    ZXIC_UINT32 cpu_rd_data_tmp3;
}DPP_SE_ALG_CPU_RD_DATA_TMP3_T;

typedef struct dpp_se_alg_cpu_rd_data_tmp4_t
{
    ZXIC_UINT32 cpu_rd_data_tmp4;
}DPP_SE_ALG_CPU_RD_DATA_TMP4_T;

typedef struct dpp_se_alg_cpu_rd_data_tmp5_t
{
    ZXIC_UINT32 cpu_rd_data_tmp5;
}DPP_SE_ALG_CPU_RD_DATA_TMP5_T;

typedef struct dpp_se_alg_cpu_rd_data_tmp6_t
{
    ZXIC_UINT32 cpu_rd_data_tmp6;
}DPP_SE_ALG_CPU_RD_DATA_TMP6_T;

typedef struct dpp_se_alg_cpu_rd_data_tmp7_t
{
    ZXIC_UINT32 cpu_rd_data_tmp7;
}DPP_SE_ALG_CPU_RD_DATA_TMP7_T;

typedef struct dpp_se_alg_cpu_rd_data_tmp8_t
{
    ZXIC_UINT32 cpu_rd_data_tmp8;
}DPP_SE_ALG_CPU_RD_DATA_TMP8_T;

typedef struct dpp_se_alg_cpu_rd_data_tmp9_t
{
    ZXIC_UINT32 cpu_rd_data_tmp9;
}DPP_SE_ALG_CPU_RD_DATA_TMP9_T;

typedef struct dpp_se_alg_cpu_rd_data_tmp10_t
{
    ZXIC_UINT32 cpu_rd_data_tmp10;
}DPP_SE_ALG_CPU_RD_DATA_TMP10_T;

typedef struct dpp_se_alg_cpu_rd_data_tmp11_t
{
    ZXIC_UINT32 cpu_rd_data_tmp11;
}DPP_SE_ALG_CPU_RD_DATA_TMP11_T;

typedef struct dpp_se_alg_cpu_rd_data_tmp12_t
{
    ZXIC_UINT32 cpu_rd_data_tmp12;
}DPP_SE_ALG_CPU_RD_DATA_TMP12_T;

typedef struct dpp_se_alg_cpu_rd_data_tmp13_t
{
    ZXIC_UINT32 cpu_rd_data_tmp13;
}DPP_SE_ALG_CPU_RD_DATA_TMP13_T;

typedef struct dpp_se_alg_cpu_rd_data_tmp14_t
{
    ZXIC_UINT32 cpu_rd_data_tmp14;
}DPP_SE_ALG_CPU_RD_DATA_TMP14_T;

typedef struct dpp_se_alg_cpu_rd_data_tmp15_t
{
    ZXIC_UINT32 cpu_rd_data_tmp15;
}DPP_SE_ALG_CPU_RD_DATA_TMP15_T;

typedef struct dpp_se_alg_lpm_v4_config_rgt_t
{
    ZXIC_UINT32 lpm_v4_shift_sel;
    ZXIC_UINT32 lpm_v4_sram_cmp_flag;
    ZXIC_UINT32 lpm_v4_ddr3_addr_sel;
}DPP_SE_ALG_LPM_V4_CONFIG_RGT_T;

typedef struct dpp_se_alg_lpm_v6_config_rgt_t
{
    ZXIC_UINT32 lpm_v6_shift_sel;
    ZXIC_UINT32 lpm_v6_sram_cmp_flag;
    ZXIC_UINT32 lpm_v6_ddr3_addr_sel;
}DPP_SE_ALG_LPM_V6_CONFIG_RGT_T;

typedef struct dpp_se_alg_lpm_ext_rsp_fifo_u0_pfull_ast_t
{
    ZXIC_UINT32 lpm_ext_rsp_fifo_u0_pfull_ast;
}DPP_SE_ALG_LPM_EXT_RSP_FIFO_U0_PFULL_AST_T;

typedef struct dpp_se_as_hash_age_pat_cfg_t
{
    ZXIC_UINT32 hash_age_pat_cfg;
}DPP_SE_AS_HASH_AGE_PAT_CFG_T;

typedef struct dpp_se_as_learn_rdy_cfg_t
{
    ZXIC_UINT32 learn_rdy_cfg;
}DPP_SE_AS_LEARN_RDY_CFG_T;

typedef struct dpp_se_kschd_kschd_as_pful_cfg_t
{
    ZXIC_UINT32 kschd_as_pful_cfg;
}DPP_SE_KSCHD_KSCHD_AS_PFUL_CFG_T;

typedef struct dpp_se_kschd_kschd_dir_pful_cfg_t
{
    ZXIC_UINT32 kschd_dir_pful_cfg;
}DPP_SE_KSCHD_KSCHD_DIR_PFUL_CFG_T;

typedef struct dpp_se_kschd_kschd_as_ept_cfg_t
{
    ZXIC_UINT32 kschd_as_ept_cfg;
}DPP_SE_KSCHD_KSCHD_AS_EPT_CFG_T;

typedef struct dpp_se_kschd_cpu_arbi_pful_cfg_t
{
    ZXIC_UINT32 cpu_arbi_pful_cfg;
}DPP_SE_KSCHD_CPU_ARBI_PFUL_CFG_T;

typedef struct dpp_se_kschd_kschd_pbu_pful_cfg_t
{
    ZXIC_UINT32 kschd_pbu_pful_cfg;
}DPP_SE_KSCHD_KSCHD_PBU_PFUL_CFG_T;

typedef struct dpp_se_rschd_rschd_dir_pful_cfg_t
{
    ZXIC_UINT32 rschd_dir_pful_cfg;
}DPP_SE_RSCHD_RSCHD_DIR_PFUL_CFG_T;

typedef struct dpp_se_rschd_rschd_dir_ept_cfg_t
{
    ZXIC_UINT32 rschd_dir_ept_cfg;
}DPP_SE_RSCHD_RSCHD_DIR_EPT_CFG_T;

typedef struct dpp_se_cfg_ppu_soft_rst_t
{
    ZXIC_UINT32 ppu_soft_rst;
}DPP_SE_CFG_PPU_SOFT_RST_T;

typedef struct dpp_se_cfg_ept_flag_t
{
    ZXIC_UINT32 ept_flag;
}DPP_SE_CFG_EPT_FLAG_T;

typedef struct dpp_se_cfg_ddr_key_lk0_3_t
{
    ZXIC_UINT32 ddr_key_lk0_3;
}DPP_SE_CFG_DDR_KEY_LK0_3_T;

typedef struct dpp_se_cfg_ddr_key_lk0_2_t
{
    ZXIC_UINT32 ddr_key_lk0_2;
}DPP_SE_CFG_DDR_KEY_LK0_2_T;

typedef struct dpp_se_cfg_ddr_key_lk0_1_t
{
    ZXIC_UINT32 ddr_key_lk0_1;
}DPP_SE_CFG_DDR_KEY_LK0_1_T;

typedef struct dpp_se_cfg_ddr_key_lk0_0_t
{
    ZXIC_UINT32 ddr_key_lk0_0;
}DPP_SE_CFG_DDR_KEY_LK0_0_T;

typedef struct dpp_se_cfg_ddr_key_lk1_3_t
{
    ZXIC_UINT32 ddr_key_lk1_3;
}DPP_SE_CFG_DDR_KEY_LK1_3_T;

typedef struct dpp_se_cfg_ddr_key_lk1_2_t
{
    ZXIC_UINT32 ddr_key_lk1_2;
}DPP_SE_CFG_DDR_KEY_LK1_2_T;

typedef struct dpp_se_cfg_ddr_key_lk1_1_t
{
    ZXIC_UINT32 ddr_key_lk1_1;
}DPP_SE_CFG_DDR_KEY_LK1_1_T;

typedef struct dpp_se_cfg_ddr_key_lk1_0_t
{
    ZXIC_UINT32 ddr_key_lk1_0;
}DPP_SE_CFG_DDR_KEY_LK1_0_T;

typedef struct dpp_se_cfg_hash_key_lk0_18_t
{
    ZXIC_UINT32 hash_key_lk0_18;
}DPP_SE_CFG_HASH_KEY_LK0_18_T;

typedef struct dpp_se_cfg_hash_key_lk0_17_t
{
    ZXIC_UINT32 hash_key_lk0_17;
}DPP_SE_CFG_HASH_KEY_LK0_17_T;

typedef struct dpp_se_cfg_hash_key_lk0_16_t
{
    ZXIC_UINT32 hash_key_lk0_16;
}DPP_SE_CFG_HASH_KEY_LK0_16_T;

typedef struct dpp_se_cfg_hash_key_lk0_15_t
{
    ZXIC_UINT32 hash_key_lk0_15;
}DPP_SE_CFG_HASH_KEY_LK0_15_T;

typedef struct dpp_se_cfg_hash_key_lk0_14_t
{
    ZXIC_UINT32 hash_key_lk0_14;
}DPP_SE_CFG_HASH_KEY_LK0_14_T;

typedef struct dpp_se_cfg_hash_key_lk0_13_t
{
    ZXIC_UINT32 hash_key_lk0_13;
}DPP_SE_CFG_HASH_KEY_LK0_13_T;

typedef struct dpp_se_cfg_hash_key_lk0_12_t
{
    ZXIC_UINT32 hash_key_lk0_12;
}DPP_SE_CFG_HASH_KEY_LK0_12_T;

typedef struct dpp_se_cfg_hash_key_lk0_11_t
{
    ZXIC_UINT32 hash_key_lk0_11;
}DPP_SE_CFG_HASH_KEY_LK0_11_T;

typedef struct dpp_se_cfg_hash_key_lk0_10_t
{
    ZXIC_UINT32 hash_key_lk0_10;
}DPP_SE_CFG_HASH_KEY_LK0_10_T;

typedef struct dpp_se_cfg_hash_key_lk0_9_t
{
    ZXIC_UINT32 hash_key_lk0_9;
}DPP_SE_CFG_HASH_KEY_LK0_9_T;

typedef struct dpp_se_cfg_hash_key_lk0_8_t
{
    ZXIC_UINT32 hash_key_lk0_8;
}DPP_SE_CFG_HASH_KEY_LK0_8_T;

typedef struct dpp_se_cfg_hash_key_lk0_7_t
{
    ZXIC_UINT32 hash_key_lk0_7;
}DPP_SE_CFG_HASH_KEY_LK0_7_T;

typedef struct dpp_se_cfg_hash_key_lk0_6_t
{
    ZXIC_UINT32 hash_key_lk0_6;
}DPP_SE_CFG_HASH_KEY_LK0_6_T;

typedef struct dpp_se_cfg_hash_key_lk0_5_t
{
    ZXIC_UINT32 hash_key_lk0_5;
}DPP_SE_CFG_HASH_KEY_LK0_5_T;

typedef struct dpp_se_cfg_hash_key_lk0_4_t
{
    ZXIC_UINT32 hash_key_lk0_4;
}DPP_SE_CFG_HASH_KEY_LK0_4_T;

typedef struct dpp_se_cfg_hash_key_lk0_3_t
{
    ZXIC_UINT32 hash_key_lk0_3;
}DPP_SE_CFG_HASH_KEY_LK0_3_T;

typedef struct dpp_se_cfg_hash_key_lk0_2_t
{
    ZXIC_UINT32 hash_key_lk0_2;
}DPP_SE_CFG_HASH_KEY_LK0_2_T;

typedef struct dpp_se_cfg_hash_key_lk0_1_t
{
    ZXIC_UINT32 hash_key_lk0_1;
}DPP_SE_CFG_HASH_KEY_LK0_1_T;

typedef struct dpp_se_cfg_hash_key_lk0_0_t
{
    ZXIC_UINT32 hash_key_lk0_0;
}DPP_SE_CFG_HASH_KEY_LK0_0_T;

typedef struct dpp_se_cfg_hash_key_lk1_18_t
{
    ZXIC_UINT32 hash_key_lk1_18;
}DPP_SE_CFG_HASH_KEY_LK1_18_T;

typedef struct dpp_se_cfg_hash_key_lk1_17_t
{
    ZXIC_UINT32 hash_key_lk1_17;
}DPP_SE_CFG_HASH_KEY_LK1_17_T;

typedef struct dpp_se_cfg_hash_key_lk1_16_t
{
    ZXIC_UINT32 hash_key_lk1_16;
}DPP_SE_CFG_HASH_KEY_LK1_16_T;

typedef struct dpp_se_cfg_hash_key_lk1_15_t
{
    ZXIC_UINT32 hash_key_lk1_15;
}DPP_SE_CFG_HASH_KEY_LK1_15_T;

typedef struct dpp_se_cfg_hash_key_lk1_14_t
{
    ZXIC_UINT32 hash_key_lk1_14;
}DPP_SE_CFG_HASH_KEY_LK1_14_T;

typedef struct dpp_se_cfg_hash_key_lk1_13_t
{
    ZXIC_UINT32 hash_key_lk1_13;
}DPP_SE_CFG_HASH_KEY_LK1_13_T;

typedef struct dpp_se_cfg_hash_key_lk1_12_t
{
    ZXIC_UINT32 hash_key_lk1_12;
}DPP_SE_CFG_HASH_KEY_LK1_12_T;

typedef struct dpp_se_cfg_hash_key_lk1_11_t
{
    ZXIC_UINT32 hash_key_lk1_11;
}DPP_SE_CFG_HASH_KEY_LK1_11_T;

typedef struct dpp_se_cfg_hash_key_lk1_10_t
{
    ZXIC_UINT32 hash_key_lk1_10;
}DPP_SE_CFG_HASH_KEY_LK1_10_T;

typedef struct dpp_se_cfg_hash_key_lk1_9_t
{
    ZXIC_UINT32 hash_key_lk1_9;
}DPP_SE_CFG_HASH_KEY_LK1_9_T;

typedef struct dpp_se_cfg_hash_key_lk1_8_t
{
    ZXIC_UINT32 hash_key_lk1_8;
}DPP_SE_CFG_HASH_KEY_LK1_8_T;

typedef struct dpp_se_cfg_hash_key_lk1_7_t
{
    ZXIC_UINT32 hash_key_lk1_7;
}DPP_SE_CFG_HASH_KEY_LK1_7_T;

typedef struct dpp_se_cfg_hash_key_lk1_6_t
{
    ZXIC_UINT32 hash_key_lk1_6;
}DPP_SE_CFG_HASH_KEY_LK1_6_T;

typedef struct dpp_se_cfg_hash_key_lk1_5_t
{
    ZXIC_UINT32 hash_key_lk1_5;
}DPP_SE_CFG_HASH_KEY_LK1_5_T;

typedef struct dpp_se_cfg_hash_key_lk1_4_t
{
    ZXIC_UINT32 hash_key_lk1_4;
}DPP_SE_CFG_HASH_KEY_LK1_4_T;

typedef struct dpp_se_cfg_hash_key_lk1_3_t
{
    ZXIC_UINT32 hash_key_lk1_3;
}DPP_SE_CFG_HASH_KEY_LK1_3_T;

typedef struct dpp_se_cfg_hash_key_lk1_2_t
{
    ZXIC_UINT32 hash_key_lk1_2;
}DPP_SE_CFG_HASH_KEY_LK1_2_T;

typedef struct dpp_se_cfg_hash_key_lk1_1_t
{
    ZXIC_UINT32 hash_key_lk1_1;
}DPP_SE_CFG_HASH_KEY_LK1_1_T;

typedef struct dpp_se_cfg_hash_key_lk1_0_t
{
    ZXIC_UINT32 hash_key_lk1_0;
}DPP_SE_CFG_HASH_KEY_LK1_0_T;

typedef struct dpp_se_cfg_hash_key_lk2_18_t
{
    ZXIC_UINT32 hash_key_lk2_18;
}DPP_SE_CFG_HASH_KEY_LK2_18_T;

typedef struct dpp_se_cfg_hash_key_lk2_17_t
{
    ZXIC_UINT32 hash_key_lk2_17;
}DPP_SE_CFG_HASH_KEY_LK2_17_T;

typedef struct dpp_se_cfg_hash_key_lk2_16_t
{
    ZXIC_UINT32 hash_key_lk2_16;
}DPP_SE_CFG_HASH_KEY_LK2_16_T;

typedef struct dpp_se_cfg_hash_key_lk2_15_t
{
    ZXIC_UINT32 hash_key_lk2_15;
}DPP_SE_CFG_HASH_KEY_LK2_15_T;

typedef struct dpp_se_cfg_hash_key_lk2_14_t
{
    ZXIC_UINT32 hash_key_lk2_14;
}DPP_SE_CFG_HASH_KEY_LK2_14_T;

typedef struct dpp_se_cfg_hash_key_lk2_13_t
{
    ZXIC_UINT32 hash_key_lk2_13;
}DPP_SE_CFG_HASH_KEY_LK2_13_T;

typedef struct dpp_se_cfg_hash_key_lk2_12_t
{
    ZXIC_UINT32 hash_key_lk2_12;
}DPP_SE_CFG_HASH_KEY_LK2_12_T;

typedef struct dpp_se_cfg_hash_key_lk2_11_t
{
    ZXIC_UINT32 hash_key_lk2_11;
}DPP_SE_CFG_HASH_KEY_LK2_11_T;

typedef struct dpp_se_cfg_hash_key_lk2_10_t
{
    ZXIC_UINT32 hash_key_lk2_10;
}DPP_SE_CFG_HASH_KEY_LK2_10_T;

typedef struct dpp_se_cfg_hash_key_lk2_9_t
{
    ZXIC_UINT32 hash_key_lk2_9;
}DPP_SE_CFG_HASH_KEY_LK2_9_T;

typedef struct dpp_se_cfg_hash_key_lk2_8_t
{
    ZXIC_UINT32 hash_key_lk2_8;
}DPP_SE_CFG_HASH_KEY_LK2_8_T;

typedef struct dpp_se_cfg_hash_key_lk2_7_t
{
    ZXIC_UINT32 hash_key_lk2_7;
}DPP_SE_CFG_HASH_KEY_LK2_7_T;

typedef struct dpp_se_cfg_hash_key_lk2_6_t
{
    ZXIC_UINT32 hash_key_lk2_6;
}DPP_SE_CFG_HASH_KEY_LK2_6_T;

typedef struct dpp_se_cfg_hash_key_lk2_5_t
{
    ZXIC_UINT32 hash_key_lk2_5;
}DPP_SE_CFG_HASH_KEY_LK2_5_T;

typedef struct dpp_se_cfg_hash_key_lk2_4_t
{
    ZXIC_UINT32 hash_key_lk2_4;
}DPP_SE_CFG_HASH_KEY_LK2_4_T;

typedef struct dpp_se_cfg_hash_key_lk2_3_t
{
    ZXIC_UINT32 hash_key_lk2_3;
}DPP_SE_CFG_HASH_KEY_LK2_3_T;

typedef struct dpp_se_cfg_hash_key_lk2_2_t
{
    ZXIC_UINT32 hash_key_lk2_2;
}DPP_SE_CFG_HASH_KEY_LK2_2_T;

typedef struct dpp_se_cfg_hash_key_lk2_1_t
{
    ZXIC_UINT32 hash_key_lk2_1;
}DPP_SE_CFG_HASH_KEY_LK2_1_T;

typedef struct dpp_se_cfg_hash_key_lk2_0_t
{
    ZXIC_UINT32 hash_key_lk2_0;
}DPP_SE_CFG_HASH_KEY_LK2_0_T;

typedef struct dpp_se_cfg_hash_key_lk3_18_t
{
    ZXIC_UINT32 hash_key_lk3_18;
}DPP_SE_CFG_HASH_KEY_LK3_18_T;

typedef struct dpp_se_cfg_hash_key_lk3_17_t
{
    ZXIC_UINT32 hash_key_lk3_17;
}DPP_SE_CFG_HASH_KEY_LK3_17_T;

typedef struct dpp_se_cfg_hash_key_lk3_16_t
{
    ZXIC_UINT32 hash_key_lk3_16;
}DPP_SE_CFG_HASH_KEY_LK3_16_T;

typedef struct dpp_se_cfg_hash_key_lk3_15_t
{
    ZXIC_UINT32 hash_key_lk3_15;
}DPP_SE_CFG_HASH_KEY_LK3_15_T;

typedef struct dpp_se_cfg_hash_key_lk3_14_t
{
    ZXIC_UINT32 hash_key_lk3_14;
}DPP_SE_CFG_HASH_KEY_LK3_14_T;

typedef struct dpp_se_cfg_hash_key_lk3_13_t
{
    ZXIC_UINT32 hash_key_lk3_13;
}DPP_SE_CFG_HASH_KEY_LK3_13_T;

typedef struct dpp_se_cfg_hash_key_lk3_12_t
{
    ZXIC_UINT32 hash_key_lk3_12;
}DPP_SE_CFG_HASH_KEY_LK3_12_T;

typedef struct dpp_se_cfg_hash_key_lk3_11_t
{
    ZXIC_UINT32 hash_key_lk3_11;
}DPP_SE_CFG_HASH_KEY_LK3_11_T;

typedef struct dpp_se_cfg_hash_key_lk3_10_t
{
    ZXIC_UINT32 hash_key_lk3_10;
}DPP_SE_CFG_HASH_KEY_LK3_10_T;

typedef struct dpp_se_cfg_hash_key_lk3_9_t
{
    ZXIC_UINT32 hash_key_lk3_9;
}DPP_SE_CFG_HASH_KEY_LK3_9_T;

typedef struct dpp_se_cfg_hash_key_lk3_8_t
{
    ZXIC_UINT32 hash_key_lk3_8;
}DPP_SE_CFG_HASH_KEY_LK3_8_T;

typedef struct dpp_se_cfg_hash_key_lk3_7_t
{
    ZXIC_UINT32 hash_key_lk3_7;
}DPP_SE_CFG_HASH_KEY_LK3_7_T;

typedef struct dpp_se_cfg_hash_key_lk3_6_t
{
    ZXIC_UINT32 hash_key_lk3_6;
}DPP_SE_CFG_HASH_KEY_LK3_6_T;

typedef struct dpp_se_cfg_hash_key_lk3_5_t
{
    ZXIC_UINT32 hash_key_lk3_5;
}DPP_SE_CFG_HASH_KEY_LK3_5_T;

typedef struct dpp_se_cfg_hash_key_lk3_4_t
{
    ZXIC_UINT32 hash_key_lk3_4;
}DPP_SE_CFG_HASH_KEY_LK3_4_T;

typedef struct dpp_se_cfg_hash_key_lk3_3_t
{
    ZXIC_UINT32 hash_key_lk3_3;
}DPP_SE_CFG_HASH_KEY_LK3_3_T;

typedef struct dpp_se_cfg_hash_key_lk3_2_t
{
    ZXIC_UINT32 hash_key_lk3_2;
}DPP_SE_CFG_HASH_KEY_LK3_2_T;

typedef struct dpp_se_cfg_hash_key_lk3_1_t
{
    ZXIC_UINT32 hash_key_lk3_1;
}DPP_SE_CFG_HASH_KEY_LK3_1_T;

typedef struct dpp_se_cfg_hash_key_lk3_0_t
{
    ZXIC_UINT32 hash_key_lk3_0;
}DPP_SE_CFG_HASH_KEY_LK3_0_T;

typedef struct dpp_se_cfg_lpm_key_lk0_6_t
{
    ZXIC_UINT32 lpm_key_lk0_6;
}DPP_SE_CFG_LPM_KEY_LK0_6_T;

typedef struct dpp_se_cfg_lpm_key_lk0_5_t
{
    ZXIC_UINT32 lpm_key_lk0_5;
}DPP_SE_CFG_LPM_KEY_LK0_5_T;

typedef struct dpp_se_cfg_lpm_key_lk0_4_t
{
    ZXIC_UINT32 lpm_key_lk0_4;
}DPP_SE_CFG_LPM_KEY_LK0_4_T;

typedef struct dpp_se_cfg_lpm_key_lk0_3_t
{
    ZXIC_UINT32 lpm_key_lk0_3;
}DPP_SE_CFG_LPM_KEY_LK0_3_T;

typedef struct dpp_se_cfg_lpm_key_lk0_2_t
{
    ZXIC_UINT32 lpm_key_lk0_2;
}DPP_SE_CFG_LPM_KEY_LK0_2_T;

typedef struct dpp_se_cfg_lpm_key_lk0_1_t
{
    ZXIC_UINT32 lpm_key_lk0_1;
}DPP_SE_CFG_LPM_KEY_LK0_1_T;

typedef struct dpp_se_cfg_lpm_key_lk0_0_t
{
    ZXIC_UINT32 lpm_key_lk0_0;
}DPP_SE_CFG_LPM_KEY_LK0_0_T;

typedef struct dpp_se_cfg_lpm_key_lk1_6_t
{
    ZXIC_UINT32 lpm_key_lk1_6;
}DPP_SE_CFG_LPM_KEY_LK1_6_T;

typedef struct dpp_se_cfg_lpm_key_lk1_5_t
{
    ZXIC_UINT32 lpm_key_lk1_5;
}DPP_SE_CFG_LPM_KEY_LK1_5_T;

typedef struct dpp_se_cfg_lpm_key_lk1_4_t
{
    ZXIC_UINT32 lpm_key_lk1_4;
}DPP_SE_CFG_LPM_KEY_LK1_4_T;

typedef struct dpp_se_cfg_lpm_key_lk1_3_t
{
    ZXIC_UINT32 lpm_key_lk1_3;
}DPP_SE_CFG_LPM_KEY_LK1_3_T;

typedef struct dpp_se_cfg_lpm_key_lk1_2_t
{
    ZXIC_UINT32 lpm_key_lk1_2;
}DPP_SE_CFG_LPM_KEY_LK1_2_T;

typedef struct dpp_se_cfg_lpm_key_lk1_1_t
{
    ZXIC_UINT32 lpm_key_lk1_1;
}DPP_SE_CFG_LPM_KEY_LK1_1_T;

typedef struct dpp_se_cfg_lpm_key_lk1_0_t
{
    ZXIC_UINT32 lpm_key_lk1_0;
}DPP_SE_CFG_LPM_KEY_LK1_0_T;

typedef struct dpp_se_cfg_lpm_key_lk2_6_t
{
    ZXIC_UINT32 lpm_key_lk2_6;
}DPP_SE_CFG_LPM_KEY_LK2_6_T;

typedef struct dpp_se_cfg_lpm_key_lk2_5_t
{
    ZXIC_UINT32 lpm_key_lk2_5;
}DPP_SE_CFG_LPM_KEY_LK2_5_T;

typedef struct dpp_se_cfg_lpm_key_lk2_4_t
{
    ZXIC_UINT32 lpm_key_lk2_4;
}DPP_SE_CFG_LPM_KEY_LK2_4_T;

typedef struct dpp_se_cfg_lpm_key_lk2_3_t
{
    ZXIC_UINT32 lpm_key_lk2_3;
}DPP_SE_CFG_LPM_KEY_LK2_3_T;

typedef struct dpp_se_cfg_lpm_key_lk2_2_t
{
    ZXIC_UINT32 lpm_key_lk2_2;
}DPP_SE_CFG_LPM_KEY_LK2_2_T;

typedef struct dpp_se_cfg_lpm_key_lk2_1_t
{
    ZXIC_UINT32 lpm_key_lk2_1;
}DPP_SE_CFG_LPM_KEY_LK2_1_T;

typedef struct dpp_se_cfg_lpm_key_lk2_0_t
{
    ZXIC_UINT32 lpm_key_lk2_0;
}DPP_SE_CFG_LPM_KEY_LK2_0_T;

typedef struct dpp_se_cfg_lpm_key_lk3_6_t
{
    ZXIC_UINT32 lpm_key_lk3_6;
}DPP_SE_CFG_LPM_KEY_LK3_6_T;

typedef struct dpp_se_cfg_lpm_key_lk3_5_t
{
    ZXIC_UINT32 lpm_key_lk3_5;
}DPP_SE_CFG_LPM_KEY_LK3_5_T;

typedef struct dpp_se_cfg_lpm_key_lk3_4_t
{
    ZXIC_UINT32 lpm_key_lk3_4;
}DPP_SE_CFG_LPM_KEY_LK3_4_T;

typedef struct dpp_se_cfg_lpm_key_lk3_3_t
{
    ZXIC_UINT32 lpm_key_lk3_3;
}DPP_SE_CFG_LPM_KEY_LK3_3_T;

typedef struct dpp_se_cfg_lpm_key_lk3_2_t
{
    ZXIC_UINT32 lpm_key_lk3_2;
}DPP_SE_CFG_LPM_KEY_LK3_2_T;

typedef struct dpp_se_cfg_lpm_key_lk3_1_t
{
    ZXIC_UINT32 lpm_key_lk3_1;
}DPP_SE_CFG_LPM_KEY_LK3_1_T;

typedef struct dpp_se_cfg_lpm_key_lk3_0_t
{
    ZXIC_UINT32 lpm_key_lk3_0;
}DPP_SE_CFG_LPM_KEY_LK3_0_T;

typedef struct dpp_se_cfg_etcam_key_lk0_22_t
{
    ZXIC_UINT32 etcam_key_lk0_22;
}DPP_SE_CFG_ETCAM_KEY_LK0_22_T;

typedef struct dpp_se_cfg_etcam_key_lk0_21_t
{
    ZXIC_UINT32 etcam_key_lk0_21;
}DPP_SE_CFG_ETCAM_KEY_LK0_21_T;

typedef struct dpp_se_cfg_etcam_key_lk0_20_t
{
    ZXIC_UINT32 etcam_key_lk0_20;
}DPP_SE_CFG_ETCAM_KEY_LK0_20_T;

typedef struct dpp_se_cfg_etcam_key_lk0_19_t
{
    ZXIC_UINT32 etcam_key_lk0_19;
}DPP_SE_CFG_ETCAM_KEY_LK0_19_T;

typedef struct dpp_se_cfg_etcam_key_lk0_18_t
{
    ZXIC_UINT32 etcam_key_lk0_18;
}DPP_SE_CFG_ETCAM_KEY_LK0_18_T;

typedef struct dpp_se_cfg_etcam_key_lk0_17_t
{
    ZXIC_UINT32 etcam_key_lk0_17;
}DPP_SE_CFG_ETCAM_KEY_LK0_17_T;

typedef struct dpp_se_cfg_etcam_key_lk0_16_t
{
    ZXIC_UINT32 etcam_key_lk0_16;
}DPP_SE_CFG_ETCAM_KEY_LK0_16_T;

typedef struct dpp_se_cfg_etcam_key_lk0_15_t
{
    ZXIC_UINT32 etcam_key_lk0_15;
}DPP_SE_CFG_ETCAM_KEY_LK0_15_T;

typedef struct dpp_se_cfg_etcam_key_lk0_14_t
{
    ZXIC_UINT32 etcam_key_lk0_14;
}DPP_SE_CFG_ETCAM_KEY_LK0_14_T;

typedef struct dpp_se_cfg_etcam_key_lk0_13_t
{
    ZXIC_UINT32 etcam_key_lk0_13;
}DPP_SE_CFG_ETCAM_KEY_LK0_13_T;

typedef struct dpp_se_cfg_etcam_key_lk0_12_t
{
    ZXIC_UINT32 etcam_key_lk0_12;
}DPP_SE_CFG_ETCAM_KEY_LK0_12_T;

typedef struct dpp_se_cfg_etcam_key_lk0_11_t
{
    ZXIC_UINT32 etcam_key_lk0_11;
}DPP_SE_CFG_ETCAM_KEY_LK0_11_T;

typedef struct dpp_se_cfg_etcam_key_lk0_10_t
{
    ZXIC_UINT32 etcam_key_lk0_10;
}DPP_SE_CFG_ETCAM_KEY_LK0_10_T;

typedef struct dpp_se_cfg_etcam_key_lk0_9_t
{
    ZXIC_UINT32 etcam_key_lk0_9;
}DPP_SE_CFG_ETCAM_KEY_LK0_9_T;

typedef struct dpp_se_cfg_etcam_key_lk0_8_t
{
    ZXIC_UINT32 etcam_key_lk0_8;
}DPP_SE_CFG_ETCAM_KEY_LK0_8_T;

typedef struct dpp_se_cfg_etcam_key_lk0_7_t
{
    ZXIC_UINT32 etcam_key_lk0_7;
}DPP_SE_CFG_ETCAM_KEY_LK0_7_T;

typedef struct dpp_se_cfg_etcam_key_lk0_6_t
{
    ZXIC_UINT32 etcam_key_lk0_6;
}DPP_SE_CFG_ETCAM_KEY_LK0_6_T;

typedef struct dpp_se_cfg_etcam_key_lk0_5_t
{
    ZXIC_UINT32 etcam_key_lk0_5;
}DPP_SE_CFG_ETCAM_KEY_LK0_5_T;

typedef struct dpp_se_cfg_etcam_key_lk0_4_t
{
    ZXIC_UINT32 etcam_key_lk0_4;
}DPP_SE_CFG_ETCAM_KEY_LK0_4_T;

typedef struct dpp_se_cfg_etcam_key_lk0_3_t
{
    ZXIC_UINT32 etcam_key_lk0_3;
}DPP_SE_CFG_ETCAM_KEY_LK0_3_T;

typedef struct dpp_se_cfg_etcam_key_lk0_2_t
{
    ZXIC_UINT32 etcam_key_lk0_2;
}DPP_SE_CFG_ETCAM_KEY_LK0_2_T;

typedef struct dpp_se_cfg_etcam_key_lk0_1_t
{
    ZXIC_UINT32 etcam_key_lk0_1;
}DPP_SE_CFG_ETCAM_KEY_LK0_1_T;

typedef struct dpp_se_cfg_etcam_key_lk0_0_t
{
    ZXIC_UINT32 etcam_key_lk0_0;
}DPP_SE_CFG_ETCAM_KEY_LK0_0_T;

typedef struct dpp_se_cfg_etcam_key_lk1_22_t
{
    ZXIC_UINT32 etcam_key_lk1_22;
}DPP_SE_CFG_ETCAM_KEY_LK1_22_T;

typedef struct dpp_se_cfg_etcam_key_lk1_21_t
{
    ZXIC_UINT32 etcam_key_lk1_21;
}DPP_SE_CFG_ETCAM_KEY_LK1_21_T;

typedef struct dpp_se_cfg_etcam_key_lk1_20_t
{
    ZXIC_UINT32 etcam_key_lk1_20;
}DPP_SE_CFG_ETCAM_KEY_LK1_20_T;

typedef struct dpp_se_cfg_etcam_key_lk1_19_t
{
    ZXIC_UINT32 etcam_key_lk1_19;
}DPP_SE_CFG_ETCAM_KEY_LK1_19_T;

typedef struct dpp_se_cfg_etcam_key_lk1_18_t
{
    ZXIC_UINT32 etcam_key_lk1_18;
}DPP_SE_CFG_ETCAM_KEY_LK1_18_T;

typedef struct dpp_se_cfg_etcam_key_lk1_17_t
{
    ZXIC_UINT32 etcam_key_lk1_17;
}DPP_SE_CFG_ETCAM_KEY_LK1_17_T;

typedef struct dpp_se_cfg_etcam_key_lk1_16_t
{
    ZXIC_UINT32 etcam_key_lk1_16;
}DPP_SE_CFG_ETCAM_KEY_LK1_16_T;

typedef struct dpp_se_cfg_etcam_key_lk1_15_t
{
    ZXIC_UINT32 etcam_key_lk1_15;
}DPP_SE_CFG_ETCAM_KEY_LK1_15_T;

typedef struct dpp_se_cfg_etcam_key_lk1_14_t
{
    ZXIC_UINT32 etcam_key_lk1_14;
}DPP_SE_CFG_ETCAM_KEY_LK1_14_T;

typedef struct dpp_se_cfg_etcam_key_lk1_13_t
{
    ZXIC_UINT32 etcam_key_lk1_13;
}DPP_SE_CFG_ETCAM_KEY_LK1_13_T;

typedef struct dpp_se_cfg_etcam_key_lk1_12_t
{
    ZXIC_UINT32 etcam_key_lk1_12;
}DPP_SE_CFG_ETCAM_KEY_LK1_12_T;

typedef struct dpp_se_cfg_etcam_key_lk1_11_t
{
    ZXIC_UINT32 etcam_key_lk1_11;
}DPP_SE_CFG_ETCAM_KEY_LK1_11_T;

typedef struct dpp_se_cfg_etcam_key_lk1_10_t
{
    ZXIC_UINT32 etcam_key_lk1_10;
}DPP_SE_CFG_ETCAM_KEY_LK1_10_T;

typedef struct dpp_se_cfg_etcam_key_lk1_9_t
{
    ZXIC_UINT32 etcam_key_lk1_9;
}DPP_SE_CFG_ETCAM_KEY_LK1_9_T;

typedef struct dpp_se_cfg_etcam_key_lk1_8_t
{
    ZXIC_UINT32 etcam_key_lk1_8;
}DPP_SE_CFG_ETCAM_KEY_LK1_8_T;

typedef struct dpp_se_cfg_etcam_key_lk1_7_t
{
    ZXIC_UINT32 etcam_key_lk1_7;
}DPP_SE_CFG_ETCAM_KEY_LK1_7_T;

typedef struct dpp_se_cfg_etcam_key_lk1_6_t
{
    ZXIC_UINT32 etcam_key_lk1_6;
}DPP_SE_CFG_ETCAM_KEY_LK1_6_T;

typedef struct dpp_se_cfg_etcam_key_lk1_5_t
{
    ZXIC_UINT32 etcam_key_lk1_5;
}DPP_SE_CFG_ETCAM_KEY_LK1_5_T;

typedef struct dpp_se_cfg_etcam_key_lk1_4_t
{
    ZXIC_UINT32 etcam_key_lk1_4;
}DPP_SE_CFG_ETCAM_KEY_LK1_4_T;

typedef struct dpp_se_cfg_etcam_key_lk1_3_t
{
    ZXIC_UINT32 etcam_key_lk1_3;
}DPP_SE_CFG_ETCAM_KEY_LK1_3_T;

typedef struct dpp_se_cfg_etcam_key_lk1_2_t
{
    ZXIC_UINT32 etcam_key_lk1_2;
}DPP_SE_CFG_ETCAM_KEY_LK1_2_T;

typedef struct dpp_se_cfg_etcam_key_lk1_1_t
{
    ZXIC_UINT32 etcam_key_lk1_1;
}DPP_SE_CFG_ETCAM_KEY_LK1_1_T;

typedef struct dpp_se_cfg_etcam_key_lk1_0_t
{
    ZXIC_UINT32 etcam_key_lk1_0;
}DPP_SE_CFG_ETCAM_KEY_LK1_0_T;

typedef struct dpp_se_cfg_etcam_key_lk2_22_t
{
    ZXIC_UINT32 etcam_key_lk2_22;
}DPP_SE_CFG_ETCAM_KEY_LK2_22_T;

typedef struct dpp_se_cfg_etcam_key_lk2_21_t
{
    ZXIC_UINT32 etcam_key_lk2_21;
}DPP_SE_CFG_ETCAM_KEY_LK2_21_T;

typedef struct dpp_se_cfg_etcam_key_lk2_20_t
{
    ZXIC_UINT32 etcam_key_lk2_20;
}DPP_SE_CFG_ETCAM_KEY_LK2_20_T;

typedef struct dpp_se_cfg_etcam_key_lk2_19_t
{
    ZXIC_UINT32 etcam_key_lk2_19;
}DPP_SE_CFG_ETCAM_KEY_LK2_19_T;

typedef struct dpp_se_cfg_etcam_key_lk2_18_t
{
    ZXIC_UINT32 etcam_key_lk2_18;
}DPP_SE_CFG_ETCAM_KEY_LK2_18_T;

typedef struct dpp_se_cfg_etcam_key_lk2_17_t
{
    ZXIC_UINT32 etcam_key_lk2_17;
}DPP_SE_CFG_ETCAM_KEY_LK2_17_T;

typedef struct dpp_se_cfg_etcam_key_lk2_16_t
{
    ZXIC_UINT32 etcam_key_lk2_16;
}DPP_SE_CFG_ETCAM_KEY_LK2_16_T;

typedef struct dpp_se_cfg_etcam_key_lk2_15_t
{
    ZXIC_UINT32 etcam_key_lk2_15;
}DPP_SE_CFG_ETCAM_KEY_LK2_15_T;

typedef struct dpp_se_cfg_etcam_key_lk2_14_t
{
    ZXIC_UINT32 etcam_key_lk2_14;
}DPP_SE_CFG_ETCAM_KEY_LK2_14_T;

typedef struct dpp_se_cfg_etcam_key_lk2_13_t
{
    ZXIC_UINT32 etcam_key_lk2_13;
}DPP_SE_CFG_ETCAM_KEY_LK2_13_T;

typedef struct dpp_se_cfg_etcam_key_lk2_12_t
{
    ZXIC_UINT32 etcam_key_lk2_12;
}DPP_SE_CFG_ETCAM_KEY_LK2_12_T;

typedef struct dpp_se_cfg_etcam_key_lk2_11_t
{
    ZXIC_UINT32 etcam_key_lk2_11;
}DPP_SE_CFG_ETCAM_KEY_LK2_11_T;

typedef struct dpp_se_cfg_etcam_key_lk2_10_t
{
    ZXIC_UINT32 etcam_key_lk2_10;
}DPP_SE_CFG_ETCAM_KEY_LK2_10_T;

typedef struct dpp_se_cfg_etcam_key_lk2_9_t
{
    ZXIC_UINT32 etcam_key_lk2_9;
}DPP_SE_CFG_ETCAM_KEY_LK2_9_T;

typedef struct dpp_se_cfg_etcam_key_lk2_8_t
{
    ZXIC_UINT32 etcam_key_lk2_8;
}DPP_SE_CFG_ETCAM_KEY_LK2_8_T;

typedef struct dpp_se_cfg_etcam_key_lk2_7_t
{
    ZXIC_UINT32 etcam_key_lk2_7;
}DPP_SE_CFG_ETCAM_KEY_LK2_7_T;

typedef struct dpp_se_cfg_etcam_key_lk2_6_t
{
    ZXIC_UINT32 etcam_key_lk2_6;
}DPP_SE_CFG_ETCAM_KEY_LK2_6_T;

typedef struct dpp_se_cfg_etcam_key_lk2_5_t
{
    ZXIC_UINT32 etcam_key_lk2_5;
}DPP_SE_CFG_ETCAM_KEY_LK2_5_T;

typedef struct dpp_se_cfg_etcam_key_lk2_4_t
{
    ZXIC_UINT32 etcam_key_lk2_4;
}DPP_SE_CFG_ETCAM_KEY_LK2_4_T;

typedef struct dpp_se_cfg_etcam_key_lk2_3_t
{
    ZXIC_UINT32 etcam_key_lk2_3;
}DPP_SE_CFG_ETCAM_KEY_LK2_3_T;

typedef struct dpp_se_cfg_etcam_key_lk2_2_t
{
    ZXIC_UINT32 etcam_key_lk2_2;
}DPP_SE_CFG_ETCAM_KEY_LK2_2_T;

typedef struct dpp_se_cfg_etcam_key_lk2_1_t
{
    ZXIC_UINT32 etcam_key_lk2_1;
}DPP_SE_CFG_ETCAM_KEY_LK2_1_T;

typedef struct dpp_se_cfg_etcam_key_lk2_0_t
{
    ZXIC_UINT32 etcam_key_lk2_0;
}DPP_SE_CFG_ETCAM_KEY_LK2_0_T;

typedef struct dpp_se_cfg_etcam_key_lk3_22_t
{
    ZXIC_UINT32 etcam_key_lk3_22;
}DPP_SE_CFG_ETCAM_KEY_LK3_22_T;

typedef struct dpp_se_cfg_etcam_key_lk3_21_t
{
    ZXIC_UINT32 etcam_key_lk3_21;
}DPP_SE_CFG_ETCAM_KEY_LK3_21_T;

typedef struct dpp_se_cfg_etcam_key_lk3_20_t
{
    ZXIC_UINT32 etcam_key_lk3_20;
}DPP_SE_CFG_ETCAM_KEY_LK3_20_T;

typedef struct dpp_se_cfg_etcam_key_lk3_19_t
{
    ZXIC_UINT32 etcam_key_lk3_19;
}DPP_SE_CFG_ETCAM_KEY_LK3_19_T;

typedef struct dpp_se_cfg_etcam_key_lk3_18_t
{
    ZXIC_UINT32 etcam_key_lk3_18;
}DPP_SE_CFG_ETCAM_KEY_LK3_18_T;

typedef struct dpp_se_cfg_etcam_key_lk3_17_t
{
    ZXIC_UINT32 etcam_key_lk3_17;
}DPP_SE_CFG_ETCAM_KEY_LK3_17_T;

typedef struct dpp_se_cfg_etcam_key_lk3_16_t
{
    ZXIC_UINT32 etcam_key_lk3_16;
}DPP_SE_CFG_ETCAM_KEY_LK3_16_T;

typedef struct dpp_se_cfg_etcam_key_lk3_15_t
{
    ZXIC_UINT32 etcam_key_lk3_15;
}DPP_SE_CFG_ETCAM_KEY_LK3_15_T;

typedef struct dpp_se_cfg_etcam_key_lk3_14_t
{
    ZXIC_UINT32 etcam_key_lk3_14;
}DPP_SE_CFG_ETCAM_KEY_LK3_14_T;

typedef struct dpp_se_cfg_etcam_key_lk3_13_t
{
    ZXIC_UINT32 etcam_key_lk3_13;
}DPP_SE_CFG_ETCAM_KEY_LK3_13_T;

typedef struct dpp_se_cfg_etcam_key_lk3_12_t
{
    ZXIC_UINT32 etcam_key_lk3_12;
}DPP_SE_CFG_ETCAM_KEY_LK3_12_T;

typedef struct dpp_se_cfg_etcam_key_lk3_11_t
{
    ZXIC_UINT32 etcam_key_lk3_11;
}DPP_SE_CFG_ETCAM_KEY_LK3_11_T;

typedef struct dpp_se_cfg_etcam_key_lk3_10_t
{
    ZXIC_UINT32 etcam_key_lk3_10;
}DPP_SE_CFG_ETCAM_KEY_LK3_10_T;

typedef struct dpp_se_cfg_etcam_key_lk3_9_t
{
    ZXIC_UINT32 etcam_key_lk3_9;
}DPP_SE_CFG_ETCAM_KEY_LK3_9_T;

typedef struct dpp_se_cfg_etcam_key_lk3_8_t
{
    ZXIC_UINT32 etcam_key_lk3_8;
}DPP_SE_CFG_ETCAM_KEY_LK3_8_T;

typedef struct dpp_se_cfg_etcam_key_lk3_7_t
{
    ZXIC_UINT32 etcam_key_lk3_7;
}DPP_SE_CFG_ETCAM_KEY_LK3_7_T;

typedef struct dpp_se_cfg_etcam_key_lk3_6_t
{
    ZXIC_UINT32 etcam_key_lk3_6;
}DPP_SE_CFG_ETCAM_KEY_LK3_6_T;

typedef struct dpp_se_cfg_etcam_key_lk3_5_t
{
    ZXIC_UINT32 etcam_key_lk3_5;
}DPP_SE_CFG_ETCAM_KEY_LK3_5_T;

typedef struct dpp_se_cfg_etcam_key_lk3_4_t
{
    ZXIC_UINT32 etcam_key_lk3_4;
}DPP_SE_CFG_ETCAM_KEY_LK3_4_T;

typedef struct dpp_se_cfg_etcam_key_lk3_3_t
{
    ZXIC_UINT32 etcam_key_lk3_3;
}DPP_SE_CFG_ETCAM_KEY_LK3_3_T;

typedef struct dpp_se_cfg_etcam_key_lk3_2_t
{
    ZXIC_UINT32 etcam_key_lk3_2;
}DPP_SE_CFG_ETCAM_KEY_LK3_2_T;

typedef struct dpp_se_cfg_etcam_key_lk3_1_t
{
    ZXIC_UINT32 etcam_key_lk3_1;
}DPP_SE_CFG_ETCAM_KEY_LK3_1_T;

typedef struct dpp_se_cfg_etcam_key_lk3_0_t
{
    ZXIC_UINT32 etcam_key_lk3_0;
}DPP_SE_CFG_ETCAM_KEY_LK3_0_T;

typedef struct dpp_se_cfg_pbu_key_lk0_3_t
{
    ZXIC_UINT32 pbu_key_lk0_3;
}DPP_SE_CFG_PBU_KEY_LK0_3_T;

typedef struct dpp_se_cfg_pbu_key_lk0_2_t
{
    ZXIC_UINT32 pbu_key_lk0_2;
}DPP_SE_CFG_PBU_KEY_LK0_2_T;

typedef struct dpp_se_cfg_pbu_key_lk0_1_t
{
    ZXIC_UINT32 pbu_key_lk0_1;
}DPP_SE_CFG_PBU_KEY_LK0_1_T;

typedef struct dpp_se_cfg_pbu_key_lk0_0_t
{
    ZXIC_UINT32 pbu_key_lk0_0;
}DPP_SE_CFG_PBU_KEY_LK0_0_T;

typedef struct dpp_se_cfg_pbu_key_lk1_3_t
{
    ZXIC_UINT32 pbu_key_lk1_3;
}DPP_SE_CFG_PBU_KEY_LK1_3_T;

typedef struct dpp_se_cfg_pbu_key_lk1_2_t
{
    ZXIC_UINT32 pbu_key_lk1_2;
}DPP_SE_CFG_PBU_KEY_LK1_2_T;

typedef struct dpp_se_cfg_pbu_key_lk1_1_t
{
    ZXIC_UINT32 pbu_key_lk1_1;
}DPP_SE_CFG_PBU_KEY_LK1_1_T;

typedef struct dpp_se_cfg_pbu_key_lk1_0_t
{
    ZXIC_UINT32 pbu_key_lk1_0;
}DPP_SE_CFG_PBU_KEY_LK1_0_T;

typedef struct dpp_se_cfg_pbu_key_lk2_3_t
{
    ZXIC_UINT32 pbu_key_lk2_3;
}DPP_SE_CFG_PBU_KEY_LK2_3_T;

typedef struct dpp_se_cfg_pbu_key_lk2_2_t
{
    ZXIC_UINT32 pbu_key_lk2_2;
}DPP_SE_CFG_PBU_KEY_LK2_2_T;

typedef struct dpp_se_cfg_pbu_key_lk2_1_t
{
    ZXIC_UINT32 pbu_key_lk2_1;
}DPP_SE_CFG_PBU_KEY_LK2_1_T;

typedef struct dpp_se_cfg_pbu_key_lk2_0_t
{
    ZXIC_UINT32 pbu_key_lk2_0;
}DPP_SE_CFG_PBU_KEY_LK2_0_T;

typedef struct dpp_se_cfg_pbu_key_lk3_3_t
{
    ZXIC_UINT32 pbu_key_lk3_3;
}DPP_SE_CFG_PBU_KEY_LK3_3_T;

typedef struct dpp_se_cfg_pbu_key_lk3_2_t
{
    ZXIC_UINT32 pbu_key_lk3_2;
}DPP_SE_CFG_PBU_KEY_LK3_2_T;

typedef struct dpp_se_cfg_pbu_key_lk3_1_t
{
    ZXIC_UINT32 pbu_key_lk3_1;
}DPP_SE_CFG_PBU_KEY_LK3_1_T;

typedef struct dpp_se_cfg_pbu_key_lk3_0_t
{
    ZXIC_UINT32 pbu_key_lk3_0;
}DPP_SE_CFG_PBU_KEY_LK3_0_T;

typedef struct dpp_se_alg_schd_learn_fifo_pfull_ast_t
{
    ZXIC_UINT32 schd_learn_fifo_pfull_ast;
}DPP_SE_ALG_SCHD_LEARN_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_schd_learn_fifo_pfull_neg_t
{
    ZXIC_UINT32 schd_learn_fifo_pfull_neg;
}DPP_SE_ALG_SCHD_LEARN_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_schd_hash0_fifo_pfull_ast_t
{
    ZXIC_UINT32 schd_hash0_fifo_pfull_ast;
}DPP_SE_ALG_SCHD_HASH0_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_schd_hash0_fifo_pfull_neg_t
{
    ZXIC_UINT32 schd_hash0_fifo_pfull_neg;
}DPP_SE_ALG_SCHD_HASH0_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_schd_hash1_fifo_pfull_ast_t
{
    ZXIC_UINT32 schd_hash1_fifo_pfull_ast;
}DPP_SE_ALG_SCHD_HASH1_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_schd_hash1_fifo_pfull_neg_t
{
    ZXIC_UINT32 schd_hash1_fifo_pfull_neg;
}DPP_SE_ALG_SCHD_HASH1_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_schd_hash2_fifo_pfull_ast_t
{
    ZXIC_UINT32 schd_hash2_fifo_pfull_ast;
}DPP_SE_ALG_SCHD_HASH2_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_schd_hash2_fifo_pfull_neg_t
{
    ZXIC_UINT32 schd_hash2_fifo_pfull_neg;
}DPP_SE_ALG_SCHD_HASH2_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_schd_hash3_fifo_pfull_ast_t
{
    ZXIC_UINT32 schd_hash3_fifo_pfull_ast;
}DPP_SE_ALG_SCHD_HASH3_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_schd_hash3_fifo_pfull_neg_t
{
    ZXIC_UINT32 schd_hash3_fifo_pfull_neg;
}DPP_SE_ALG_SCHD_HASH3_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_schd_lpm_fifo_pfull_ast_t
{
    ZXIC_UINT32 schd_lpm_fifo_pfull_ast;
}DPP_SE_ALG_SCHD_LPM_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_schd_lpm_fifo_pfull_neg_t
{
    ZXIC_UINT32 schd_lpm_fifo_pfull_neg;
}DPP_SE_ALG_SCHD_LPM_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_hash0_key_fifo_pfull_ast_t
{
    ZXIC_UINT32 hash0_key_fifo_pfull_ast;
}DPP_SE_ALG_HASH0_KEY_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_hash0_key_fifo_pfull_neg_t
{
    ZXIC_UINT32 hash0_key_fifo_pfull_ast;
}DPP_SE_ALG_HASH0_KEY_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_hash0_sreq_fifo_pfull_ast_t
{
    ZXIC_UINT32 hash0_sreq_fifo_pfull_ast;
}DPP_SE_ALG_HASH0_SREQ_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_hash0_sreq_fifo_pfull_neg_t
{
    ZXIC_UINT32 hash0_sreq_fifo_pfull_neg;
}DPP_SE_ALG_HASH0_SREQ_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_hash0_int_rsp_fifo_pfull_ast_t
{
    ZXIC_UINT32 hash0_int_rsp_fifo_pfull_ast;
}DPP_SE_ALG_HASH0_INT_RSP_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_hash0_int_rsp_fifo_pfull_neg_t
{
    ZXIC_UINT32 hash0_int_rsp_fifo_pfull_neg;
}DPP_SE_ALG_HASH0_INT_RSP_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_hash0_ext_rsp_fifo_pfull_ast_t
{
    ZXIC_UINT32 hash0_ext_rsp_fifo_pfull_ast;
}DPP_SE_ALG_HASH0_EXT_RSP_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_hash0_ext_rsp_fifo_pfull_neg_t
{
    ZXIC_UINT32 hash0_ext_rsp_fifo_pfull_neg;
}DPP_SE_ALG_HASH0_EXT_RSP_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_hash1_key_fifo_pfull_ast_t
{
    ZXIC_UINT32 hash1_key_fifo_pfull_ast;
}DPP_SE_ALG_HASH1_KEY_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_hash1_key_fifo_pfull_neg_t
{
    ZXIC_UINT32 hash1_key_fifo_pfull_ast;
}DPP_SE_ALG_HASH1_KEY_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_hash1_sreq_fifo_pfull_ast_t
{
    ZXIC_UINT32 hash1_sreq_fifo_pfull_ast;
}DPP_SE_ALG_HASH1_SREQ_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_hash1_sreq_fifo_pfull_neg_t
{
    ZXIC_UINT32 hash1_sreq_fifo_pfull_neg;
}DPP_SE_ALG_HASH1_SREQ_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_hash1_int_rsp_fifo_pfull_ast_t
{
    ZXIC_UINT32 hash1_int_rsp_fifo_pfull_ast;
}DPP_SE_ALG_HASH1_INT_RSP_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_hash1_int_rsp_fifo_pfull_neg_t
{
    ZXIC_UINT32 hash1_int_rsp_fifo_pfull_neg;
}DPP_SE_ALG_HASH1_INT_RSP_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_hash1_ext_rsp_fifo_pfull_ast_t
{
    ZXIC_UINT32 hash1_ext_rsp_fifo_pfull_ast;
}DPP_SE_ALG_HASH1_EXT_RSP_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_hash1_ext_rsp_fifo_pfull_neg_t
{
    ZXIC_UINT32 hash1_ext_rsp_fifo_pfull_neg;
}DPP_SE_ALG_HASH1_EXT_RSP_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_hash2_key_fifo_pfull_ast_t
{
    ZXIC_UINT32 hash2_key_fifo_pfull_ast;
}DPP_SE_ALG_HASH2_KEY_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_hash2_key_fifo_pfull_neg_t
{
    ZXIC_UINT32 hash2_key_fifo_pfull_ast;
}DPP_SE_ALG_HASH2_KEY_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_hash2_sreq_fifo_pfull_ast_t
{
    ZXIC_UINT32 hash2_sreq_fifo_pfull_ast;
}DPP_SE_ALG_HASH2_SREQ_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_hash2_sreq_fifo_pfull_neg_t
{
    ZXIC_UINT32 hash2_sreq_fifo_pfull_neg;
}DPP_SE_ALG_HASH2_SREQ_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_hash2_int_rsp_fifo_pfull_ast_t
{
    ZXIC_UINT32 hash2_int_rsp_fifo_pfull_ast;
}DPP_SE_ALG_HASH2_INT_RSP_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_hash2_int_rsp_fifo_pfull_neg_t
{
    ZXIC_UINT32 hash2_int_rsp_fifo_pfull_neg;
}DPP_SE_ALG_HASH2_INT_RSP_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_hash2_ext_rsp_fifo_pfull_ast_t
{
    ZXIC_UINT32 hash2_ext_rsp_fifo_pfull_ast;
}DPP_SE_ALG_HASH2_EXT_RSP_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_hash2_ext_rsp_fifo_pfull_neg_t
{
    ZXIC_UINT32 hash2_ext_rsp_fifo_pfull_neg;
}DPP_SE_ALG_HASH2_EXT_RSP_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_hash3_key_fifo_pfull_ast_t
{
    ZXIC_UINT32 hash3_key_fifo_pfull_ast;
}DPP_SE_ALG_HASH3_KEY_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_hash3_key_fifo_pfull_neg_t
{
    ZXIC_UINT32 hash3_key_fifo_pfull_ast;
}DPP_SE_ALG_HASH3_KEY_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_hash3_sreq_fifo_pfull_ast_t
{
    ZXIC_UINT32 hash3_sreq_fifo_pfull_ast;
}DPP_SE_ALG_HASH3_SREQ_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_hash3_sreq_fifo_pfull_neg_t
{
    ZXIC_UINT32 hash3_sreq_fifo_pfull_neg;
}DPP_SE_ALG_HASH3_SREQ_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_hash3_int_rsp_fifo_pfull_ast_t
{
    ZXIC_UINT32 hash3_int_rsp_fifo_pfull_ast;
}DPP_SE_ALG_HASH3_INT_RSP_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_hash3_int_rsp_fifo_pfull_neg_t
{
    ZXIC_UINT32 hash3_int_rsp_fifo_pfull_neg;
}DPP_SE_ALG_HASH3_INT_RSP_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_hash3_ext_rsp_fifo_pfull_ast_t
{
    ZXIC_UINT32 hash3_ext_rsp_fifo_pfull_ast;
}DPP_SE_ALG_HASH3_EXT_RSP_FIFO_PFULL_AST_T;

typedef struct dpp_se_alg_hash3_ext_rsp_fifo_pfull_neg_t
{
    ZXIC_UINT32 hash3_ext_rsp_fifo_pfull_neg;
}DPP_SE_ALG_HASH3_EXT_RSP_FIFO_PFULL_NEG_T;

typedef struct dpp_se_alg_lpm_as_info_t
{
    ZXIC_UINT32 lpm_as_type;
    ZXIC_UINT32 lpm_as_en;
}DPP_SE_ALG_LPM_AS_INFO_T;

typedef struct dpp_se_alg_lpm_ext_rsp_fifo_u0_pfull_neg_t
{
    ZXIC_UINT32 lpm_ext_rsp_fifo_u0_pfull_neg;
}DPP_SE_ALG_LPM_EXT_RSP_FIFO_U0_PFULL_NEG_T;

typedef struct dpp_se_alg_lpm_ext_rsp_fifo_u2_pfull_ast_t
{
    ZXIC_UINT32 lpm_ext_rsp_fifo_u2_pfull_ast;
}DPP_SE_ALG_LPM_EXT_RSP_FIFO_U2_PFULL_AST_T;

typedef struct dpp_se_alg_lpm_ext_rsp_fifo_u2_pfull_neg_t
{
    ZXIC_UINT32 lpm_ext_rsp_fifo_u2_pfull_neg;
}DPP_SE_ALG_LPM_EXT_RSP_FIFO_U2_PFULL_NEG_T;

typedef struct dpp_se_alg_lpm_ext_rsp_fifo_u3_pfull_ast_t
{
    ZXIC_UINT32 lpm_ext_rsp_fifo_u3_pfull_ast;
}DPP_SE_ALG_LPM_EXT_RSP_FIFO_U3_PFULL_AST_T;

typedef struct dpp_se_alg_lpm_ext_rsp_fifo_u3_pfull_neg_t
{
    ZXIC_UINT32 lpm_ext_rsp_fifo_u3_pfull_neg;
}DPP_SE_ALG_LPM_EXT_RSP_FIFO_U3_PFULL_NEG_T;

typedef struct dpp_se_alg_lpm_ext_rsp_fifo_u4_pfull_ast_t
{
    ZXIC_UINT32 lpm_ext_rsp_fifo_u4_pfull_ast;
}DPP_SE_ALG_LPM_EXT_RSP_FIFO_U4_PFULL_AST_T;

typedef struct dpp_se_alg_lpm_ext_rsp_fifo_u4_pfull_neg_t
{
    ZXIC_UINT32 lpm_ext_rsp_fifo_u4_pfull_neg;
}DPP_SE_ALG_LPM_EXT_RSP_FIFO_U4_PFULL_NEG_T;

typedef struct dpp_se_alg_lpm_as_rsp_fifo_u0_pfull_ast_t
{
    ZXIC_UINT32 lpm_as_rsp_fifo_u0_pfull_ast;
}DPP_SE_ALG_LPM_AS_RSP_FIFO_U0_PFULL_AST_T;

typedef struct dpp_se_alg_lpm_as_rsp_fifo_u0_pfull_neg_t
{
    ZXIC_UINT32 lpm_as_rsp_fifo_u0_pfull_neg;
}DPP_SE_ALG_LPM_AS_RSP_FIFO_U0_PFULL_NEG_T;

typedef struct dpp_se_alg_lpm_as_rsp_fifo_u1_pfull_ast_t
{
    ZXIC_UINT32 lpm_as_rsp_fifo_u1_pfull_ast;
}DPP_SE_ALG_LPM_AS_RSP_FIFO_U1_PFULL_AST_T;

typedef struct dpp_se_alg_lpm_as_rsp_fifo_u1_pfull_neg_t
{
    ZXIC_UINT32 lpm_as_rsp_fifo_u1_pfull_neg;
}DPP_SE_ALG_LPM_AS_RSP_FIFO_U1_PFULL_NEG_T;

typedef struct dpp_se_alg_lpm_v4_ddr3_base_addr_t
{
    ZXIC_UINT32 lpm_v4_ddr3_base_addr;
}DPP_SE_ALG_LPM_V4_DDR3_BASE_ADDR_T;

typedef struct dpp_se_alg_lpm_v6_ddr3_base_addr_t
{
    ZXIC_UINT32 lpm_v6_ddr3_base_addr;
}DPP_SE_ALG_LPM_V6_DDR3_BASE_ADDR_T;

typedef struct dpp_se_alg_debug_cnt_mode_t
{
    ZXIC_UINT32 cnt_rd_mode;
    ZXIC_UINT32 cnt_overflow_mode;
}DPP_SE_ALG_DEBUG_CNT_MODE_T;

typedef struct dpp_se_alg_hash_p0_key_vld_cnt_t
{
    ZXIC_UINT32 hash_p0_key_vld_cnt;
}DPP_SE_ALG_HASH_P0_KEY_VLD_CNT_T;

typedef struct dpp_se_alg_hash_p1_key_vld_cnt_t
{
    ZXIC_UINT32 hash_p1_key_vld_cnt;
}DPP_SE_ALG_HASH_P1_KEY_VLD_CNT_T;

typedef struct dpp_se_alg_hash_p2_key_vld_cnt_t
{
    ZXIC_UINT32 hash_p2_key_vld_cnt;
}DPP_SE_ALG_HASH_P2_KEY_VLD_CNT_T;

typedef struct dpp_se_alg_hash_p3_key_vld_cnt_t
{
    ZXIC_UINT32 hash_p3_key_vld_cnt;
}DPP_SE_ALG_HASH_P3_KEY_VLD_CNT_T;

typedef struct dpp_se_alg_lpm_p0_key_vld_cnt_t
{
    ZXIC_UINT32 lpm_p0_key_vld_cnt;
}DPP_SE_ALG_LPM_P0_KEY_VLD_CNT_T;

typedef struct dpp_se_alg_hash_p0_rsp_vld_cnt_t
{
    ZXIC_UINT32 hash_p0_rsp_vld_cnt;
}DPP_SE_ALG_HASH_P0_RSP_VLD_CNT_T;

typedef struct dpp_se_alg_hash_p1_rsp_vld_cnt_t
{
    ZXIC_UINT32 hash_p1_rsp_vld_cnt;
}DPP_SE_ALG_HASH_P1_RSP_VLD_CNT_T;

typedef struct dpp_se_alg_hash_p2_rsp_vld_cnt_t
{
    ZXIC_UINT32 hash_p2_rsp_vld_cnt;
}DPP_SE_ALG_HASH_P2_RSP_VLD_CNT_T;

typedef struct dpp_se_alg_hash_p3_rsp_vld_cnt_t
{
    ZXIC_UINT32 hash_p3_rsp_vld_cnt;
}DPP_SE_ALG_HASH_P3_RSP_VLD_CNT_T;

typedef struct dpp_se_alg_lpm_p0_rsp_vld_cnt_t
{
    ZXIC_UINT32 lpm_p0_rsp_vld_cnt;
}DPP_SE_ALG_LPM_P0_RSP_VLD_CNT_T;

typedef struct dpp_se_alg_hash_p0_smf_cnt_t
{
    ZXIC_UINT32 hash_p0_smf_cnt;
}DPP_SE_ALG_HASH_P0_SMF_CNT_T;

typedef struct dpp_se_alg_hash_p1_smf_cnt_t
{
    ZXIC_UINT32 hash_p1_smf_cnt;
}DPP_SE_ALG_HASH_P1_SMF_CNT_T;

typedef struct dpp_se_alg_hash_p2_smf_cnt_t
{
    ZXIC_UINT32 hash_p2_smf_cnt;
}DPP_SE_ALG_HASH_P2_SMF_CNT_T;

typedef struct dpp_se_alg_hash_p3_smf_cnt_t
{
    ZXIC_UINT32 hash_p3_smf_cnt;
}DPP_SE_ALG_HASH_P3_SMF_CNT_T;

typedef struct dpp_se_alg_lpm_p0_smf_cnt_t
{
    ZXIC_UINT32 lpm_p0_smf_cnt;
}DPP_SE_ALG_LPM_P0_SMF_CNT_T;

typedef struct dpp_se_alg_hash_p0_spacevld_cnt_t
{
    ZXIC_UINT32 hash_p0_spacevld_cnt;
}DPP_SE_ALG_HASH_P0_SPACEVLD_CNT_T;

typedef struct dpp_se_alg_hash_p1_spacevld_cnt_t
{
    ZXIC_UINT32 hash_p1_spacevld_cnt;
}DPP_SE_ALG_HASH_P1_SPACEVLD_CNT_T;

typedef struct dpp_se_alg_hash_p2_spacevld_cnt_t
{
    ZXIC_UINT32 hash_p2_spacevld_cnt;
}DPP_SE_ALG_HASH_P2_SPACEVLD_CNT_T;

typedef struct dpp_se_alg_hash_p3_spacevld_cnt_t
{
    ZXIC_UINT32 hash_p3_spacevld_cnt;
}DPP_SE_ALG_HASH_P3_SPACEVLD_CNT_T;

typedef struct dpp_se_alg_smmu1_p0_req_vld_cnt_t
{
    ZXIC_UINT32 smmu1_p0_req_vld_cnt;
}DPP_SE_ALG_SMMU1_P0_REQ_VLD_CNT_T;

typedef struct dpp_se_alg_smmu1_p1_req_vld_cnt_t
{
    ZXIC_UINT32 smmu1_p1_req_vld_cnt;
}DPP_SE_ALG_SMMU1_P1_REQ_VLD_CNT_T;

typedef struct dpp_se_alg_smmu1_p2_req_vld_cnt_t
{
    ZXIC_UINT32 smmu1_p2_req_vld_cnt;
}DPP_SE_ALG_SMMU1_P2_REQ_VLD_CNT_T;

typedef struct dpp_se_alg_smmu1_p3_req_vld_cnt_t
{
    ZXIC_UINT32 smmu1_p3_req_vld_cnt;
}DPP_SE_ALG_SMMU1_P3_REQ_VLD_CNT_T;

typedef struct dpp_se_alg_smmu1_p4_req_vld_cnt_t
{
    ZXIC_UINT32 smmu1_p4_req_vld_cnt;
}DPP_SE_ALG_SMMU1_P4_REQ_VLD_CNT_T;

typedef struct dpp_se_alg_smmu1_p5_req_vld_cnt_t
{
    ZXIC_UINT32 smmu1_p5_req_vld_cnt;
}DPP_SE_ALG_SMMU1_P5_REQ_VLD_CNT_T;

typedef struct dpp_se_alg_smmu1_p0_rsp_vld_cnt_t
{
    ZXIC_UINT32 smmu1_p0_rsp_vld_cnt;
}DPP_SE_ALG_SMMU1_P0_RSP_VLD_CNT_T;

typedef struct dpp_se_alg_smmu1_p1_rsp_vld_cnt_t
{
    ZXIC_UINT32 smmu1_p1_rsp_vld_cnt;
}DPP_SE_ALG_SMMU1_P1_RSP_VLD_CNT_T;

typedef struct dpp_se_alg_smmu1_p2_rsp_vld_cnt_t
{
    ZXIC_UINT32 smmu1_p2_rsp_vld_cnt;
}DPP_SE_ALG_SMMU1_P2_RSP_VLD_CNT_T;

typedef struct dpp_se_alg_smmu1_p3_rsp_vld_cnt_t
{
    ZXIC_UINT32 smmu1_p3_rsp_vld_cnt;
}DPP_SE_ALG_SMMU1_P3_RSP_VLD_CNT_T;

typedef struct dpp_se_alg_smmu1_p4_rsp_vld_cnt_t
{
    ZXIC_UINT32 smmu1_p4_rsp_vld_cnt;
}DPP_SE_ALG_SMMU1_P4_RSP_VLD_CNT_T;

typedef struct dpp_se_alg_smmu1_p5_rsp_vld_cnt_t
{
    ZXIC_UINT32 smmu1_p5_rsp_vld_cnt;
}DPP_SE_ALG_SMMU1_P5_RSP_VLD_CNT_T;

typedef struct dpp_se_alg_schd_learn_fifo_int_cnt_t
{
    ZXIC_UINT32 schd_learn_fifo_int_cnt;
}DPP_SE_ALG_SCHD_LEARN_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_schd_hash0_fifo_int_cnt_t
{
    ZXIC_UINT32 schd_hash0_fifo_int_cnt;
}DPP_SE_ALG_SCHD_HASH0_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_schd_hash1_fifo_int_cnt_t
{
    ZXIC_UINT32 schd_hash1_fifo_int_cnt;
}DPP_SE_ALG_SCHD_HASH1_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_schd_hash2_fifo_int_cnt_t
{
    ZXIC_UINT32 schd_hash2_fifo_int_cnt;
}DPP_SE_ALG_SCHD_HASH2_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_schd_hash3_fifo_int_cnt_t
{
    ZXIC_UINT32 schd_hash3_fifo_int_cnt;
}DPP_SE_ALG_SCHD_HASH3_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_schd_lpm_fifo_int_cnt_t
{
    ZXIC_UINT32 schd_lpm_fifo_int_cnt;
}DPP_SE_ALG_SCHD_LPM_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_schd_learn_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 schd_learn_fifo_parity_err_cnt;
}DPP_SE_ALG_SCHD_LEARN_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_schd_hash0_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 schd_hash0_fifo_parity_err_cnt;
}DPP_SE_ALG_SCHD_HASH0_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_schd_hash1_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 schd_hash1_fifo_parity_err_cnt;
}DPP_SE_ALG_SCHD_HASH1_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_schd_hash2_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 schd_hash2_fifo_parity_err_cnt;
}DPP_SE_ALG_SCHD_HASH2_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_schd_hash3_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 schd_hash3_fifo_parity_err_cnt;
}DPP_SE_ALG_SCHD_HASH3_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_schd_lpm_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 schd_lpm_fifo_parity_err_cnt;
}DPP_SE_ALG_SCHD_LPM_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_rd_init_cft_cnt_t
{
    ZXIC_UINT32 rd_init_cft_cnt;
}DPP_SE_ALG_RD_INIT_CFT_CNT_T;

typedef struct dpp_se_alg_zgp0_zblk0_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp0_zblk0_ecc_err_cnt;
}DPP_SE_ALG_ZGP0_ZBLK0_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp0_zblk1_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp0_zblk1_ecc_err_cnt;
}DPP_SE_ALG_ZGP0_ZBLK1_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp0_zblk2_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp0_zblk2_ecc_err_cnt;
}DPP_SE_ALG_ZGP0_ZBLK2_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp0_zblk3_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp0_zblk3_ecc_err_cnt;
}DPP_SE_ALG_ZGP0_ZBLK3_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp0_zblk4_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp0_zblk4_ecc_err_cnt;
}DPP_SE_ALG_ZGP0_ZBLK4_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp0_zblk5_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp0_zblk5_ecc_err_cnt;
}DPP_SE_ALG_ZGP0_ZBLK5_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp0_zblk6_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp0_zblk6_ecc_err_cnt;
}DPP_SE_ALG_ZGP0_ZBLK6_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp0_zblk7_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp0_zblk7_ecc_err_cnt;
}DPP_SE_ALG_ZGP0_ZBLK7_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp1_zblk0_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp1_zblk0_ecc_err_cnt;
}DPP_SE_ALG_ZGP1_ZBLK0_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp1_zblk1_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp1_zblk1_ecc_err_cnt;
}DPP_SE_ALG_ZGP1_ZBLK1_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp1_zblk2_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp1_zblk2_ecc_err_cnt;
}DPP_SE_ALG_ZGP1_ZBLK2_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp1_zblk3_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp1_zblk3_ecc_err_cnt;
}DPP_SE_ALG_ZGP1_ZBLK3_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp1_zblk4_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp1_zblk4_ecc_err_cnt;
}DPP_SE_ALG_ZGP1_ZBLK4_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp1_zblk5_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp1_zblk5_ecc_err_cnt;
}DPP_SE_ALG_ZGP1_ZBLK5_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp1_zblk6_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp1_zblk6_ecc_err_cnt;
}DPP_SE_ALG_ZGP1_ZBLK6_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp1_zblk7_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp1_zblk7_ecc_err_cnt;
}DPP_SE_ALG_ZGP1_ZBLK7_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp2_zblk0_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp2_zblk0_ecc_err_cnt;
}DPP_SE_ALG_ZGP2_ZBLK0_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp2_zblk1_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp2_zblk1_ecc_err_cnt;
}DPP_SE_ALG_ZGP2_ZBLK1_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp2_zblk2_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp2_zblk2_ecc_err_cnt;
}DPP_SE_ALG_ZGP2_ZBLK2_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp2_zblk3_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp2_zblk3_ecc_err_cnt;
}DPP_SE_ALG_ZGP2_ZBLK3_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp2_zblk4_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp2_zblk4_ecc_err_cnt;
}DPP_SE_ALG_ZGP2_ZBLK4_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp2_zblk5_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp2_zblk5_ecc_err_cnt;
}DPP_SE_ALG_ZGP2_ZBLK5_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp2_zblk6_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp2_zblk6_ecc_err_cnt;
}DPP_SE_ALG_ZGP2_ZBLK6_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp2_zblk7_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp2_zblk7_ecc_err_cnt;
}DPP_SE_ALG_ZGP2_ZBLK7_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp3_zblk0_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp3_zblk0_ecc_err_cnt;
}DPP_SE_ALG_ZGP3_ZBLK0_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp3_zblk1_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp3_zblk1_ecc_err_cnt;
}DPP_SE_ALG_ZGP3_ZBLK1_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp3_zblk2_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp3_zblk2_ecc_err_cnt;
}DPP_SE_ALG_ZGP3_ZBLK2_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp3_zblk3_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp3_zblk3_ecc_err_cnt;
}DPP_SE_ALG_ZGP3_ZBLK3_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp3_zblk4_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp3_zblk4_ecc_err_cnt;
}DPP_SE_ALG_ZGP3_ZBLK4_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp3_zblk5_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp3_zblk5_ecc_err_cnt;
}DPP_SE_ALG_ZGP3_ZBLK5_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp3_zblk6_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp3_zblk6_ecc_err_cnt;
}DPP_SE_ALG_ZGP3_ZBLK6_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zgp3_zblk7_ecc_err_cnt_t
{
    ZXIC_UINT32 zgp3_zblk7_ecc_err_cnt;
}DPP_SE_ALG_ZGP3_ZBLK7_ECC_ERR_CNT_T;

typedef struct dpp_se_alg_zcam_hash_p0_err_cnt_t
{
    ZXIC_UINT32 zcam_hash_p0_err_cnt;
}DPP_SE_ALG_ZCAM_HASH_P0_ERR_CNT_T;

typedef struct dpp_se_alg_zcam_hash_p1_err_cnt_t
{
    ZXIC_UINT32 zcam_hash_p1_err_cnt;
}DPP_SE_ALG_ZCAM_HASH_P1_ERR_CNT_T;

typedef struct dpp_se_alg_zcam_hash_p2_err_cnt_t
{
    ZXIC_UINT32 zcam_hash_p2_err_cnt;
}DPP_SE_ALG_ZCAM_HASH_P2_ERR_CNT_T;

typedef struct dpp_se_alg_zcam_hash_p3_err_cnt_t
{
    ZXIC_UINT32 zcam_hash_p3_err_cnt;
}DPP_SE_ALG_ZCAM_HASH_P3_ERR_CNT_T;

typedef struct dpp_se_alg_zcam_lpm_err_cnt_t
{
    ZXIC_UINT32 zcam_lpm_err_cnt;
}DPP_SE_ALG_ZCAM_LPM_ERR_CNT_T;

typedef struct dpp_se_alg_hash0_sreq_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 hash0_sreq_fifo_parity_err_cnt;
}DPP_SE_ALG_HASH0_SREQ_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_hash0_sreq_fifo_int_cnt_t
{
    ZXIC_UINT32 hash0_sreq_fifo_int_cnt;
}DPP_SE_ALG_HASH0_SREQ_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_hash0_key_fifo_int_cnt_t
{
    ZXIC_UINT32 hash0_key_fifo_int_cnt;
}DPP_SE_ALG_HASH0_KEY_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_hash0_int_rsp_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 hash0_int_rsp_fifo_parity_err_cnt;
}DPP_SE_ALG_HASH0_INT_RSP_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_hash0_ext_rsp_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 hash0_ext_rsp_fifo_parity_err_cnt;
}DPP_SE_ALG_HASH0_EXT_RSP_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_hash0_ext_rsp_fifo_int_cnt_t
{
    ZXIC_UINT32 hash0_ext_rsp_fifo_int_cnt;
}DPP_SE_ALG_HASH0_EXT_RSP_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_hash0_int_rsp_fifo_int_cnt_t
{
    ZXIC_UINT32 hash0_int_rsp_fifo_int_cnt;
}DPP_SE_ALG_HASH0_INT_RSP_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_hash1_sreq_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 hash1_sreq_fifo_parity_err_cnt;
}DPP_SE_ALG_HASH1_SREQ_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_hash1_sreq_fifo_int_cnt_t
{
    ZXIC_UINT32 hash1_sreq_fifo_int_cnt;
}DPP_SE_ALG_HASH1_SREQ_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_hash1_key_fifo_int_cnt_t
{
    ZXIC_UINT32 hash1_key_fifo_int_cnt;
}DPP_SE_ALG_HASH1_KEY_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_hash1_int_rsp_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 hash1_int_rsp_fifo_parity_err_cnt;
}DPP_SE_ALG_HASH1_INT_RSP_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_hash1_ext_rsp_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 hash1_ext_rsp_fifo_parity_err_cnt;
}DPP_SE_ALG_HASH1_EXT_RSP_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_hash1_ext_rsp_fifo_int_cnt_t
{
    ZXIC_UINT32 hash1_ext_rsp_fifo_int_cnt;
}DPP_SE_ALG_HASH1_EXT_RSP_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_hash1_int_rsp_fifo_int_cnt_t
{
    ZXIC_UINT32 hash1_int_rsp_fifo_int_cnt;
}DPP_SE_ALG_HASH1_INT_RSP_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_hash2_sreq_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 hash2_sreq_fifo_parity_err_cnt;
}DPP_SE_ALG_HASH2_SREQ_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_hash2_sreq_fifo_int_cnt_t
{
    ZXIC_UINT32 hash2_sreq_fifo_int_cnt;
}DPP_SE_ALG_HASH2_SREQ_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_hash2_key_fifo_int_cnt_t
{
    ZXIC_UINT32 hash2_key_fifo_int_cnt;
}DPP_SE_ALG_HASH2_KEY_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_hash2_int_rsp_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 hash2_int_rsp_fifo_parity_err_cnt;
}DPP_SE_ALG_HASH2_INT_RSP_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_hash2_ext_rsp_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 hash2_ext_rsp_fifo_parity_err_cnt;
}DPP_SE_ALG_HASH2_EXT_RSP_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_hash2_ext_rsp_fifo_int_cnt_t
{
    ZXIC_UINT32 hash2_ext_rsp_fifo_int_cnt;
}DPP_SE_ALG_HASH2_EXT_RSP_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_hash2_int_rsp_fifo_int_cnt_t
{
    ZXIC_UINT32 hash2_int_rsp_fifo_int_cnt;
}DPP_SE_ALG_HASH2_INT_RSP_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_hash3_sreq_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 hash3_sreq_fifo_parity_err_cnt;
}DPP_SE_ALG_HASH3_SREQ_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_hash3_sreq_fifo_int_cnt_t
{
    ZXIC_UINT32 hash3_sreq_fifo_int_cnt;
}DPP_SE_ALG_HASH3_SREQ_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_hash3_key_fifo_int_cnt_t
{
    ZXIC_UINT32 hash3_key_fifo_int_cnt;
}DPP_SE_ALG_HASH3_KEY_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_hash3_int_rsp_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 hash3_int_rsp_fifo_parity_err_cnt;
}DPP_SE_ALG_HASH3_INT_RSP_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_hash3_ext_rsp_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 hash3_ext_rsp_fifo_parity_err_cnt;
}DPP_SE_ALG_HASH3_EXT_RSP_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_hash3_ext_rsp_fifo_int_cnt_t
{
    ZXIC_UINT32 hash3_ext_rsp_fifo_int_cnt;
}DPP_SE_ALG_HASH3_EXT_RSP_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_hash3_int_rsp_fifo_int_cnt_t
{
    ZXIC_UINT32 hash3_int_rsp_fifo_int_cnt;
}DPP_SE_ALG_HASH3_INT_RSP_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_lpm_ext_rsp_fifo_int_cnt_t
{
    ZXIC_UINT32 lpm_ext_rsp_fifo_int_cnt;
}DPP_SE_ALG_LPM_EXT_RSP_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_lpm_ext_v6_fifo_int_cnt_t
{
    ZXIC_UINT32 lpm_ext_v6_fifo_int_cnt;
}DPP_SE_ALG_LPM_EXT_V6_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_lpm_ext_v4_fifo_int_cnt_t
{
    ZXIC_UINT32 lpm_ext_v4_fifo_int_cnt;
}DPP_SE_ALG_LPM_EXT_V4_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_lpm_ext_addr_fifo_int_cnt_t
{
    ZXIC_UINT32 lpm_ext_addr_fifo_int_cnt;
}DPP_SE_ALG_LPM_EXT_ADDR_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_lpm_ext_v4_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 lpm_ext_v4_fifo_parity_err_cnt;
}DPP_SE_ALG_LPM_EXT_V4_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_lpm_ext_v6_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 lpm_ext_v6_fifo_parity_err_cnt;
}DPP_SE_ALG_LPM_EXT_V6_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_lpm_ext_rsp_fifo_parity_err_cnt_t
{
    ZXIC_UINT32 lpm_ext_rsp_fifo_parity_err_cnt;
}DPP_SE_ALG_LPM_EXT_RSP_FIFO_PARITY_ERR_CNT_T;

typedef struct dpp_se_alg_lpm_as_req_fifo_int_cnt_t
{
    ZXIC_UINT32 lpm_as_req_fifo_int_cnt;
}DPP_SE_ALG_LPM_AS_REQ_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_lpm_as_int_rsp_fifo_int_cnt_t
{
    ZXIC_UINT32 lpm_as_int_rsp_fifo_int_cnt;
}DPP_SE_ALG_LPM_AS_INT_RSP_FIFO_INT_CNT_T;

typedef struct dpp_se_alg_se_alg_int_status_t
{
    ZXIC_UINT32 schd_int_unmask_flag;
    ZXIC_UINT32 zblk_ecc_int_unmask_flag;
    ZXIC_UINT32 hash0_int_unmask_flag;
    ZXIC_UINT32 hash1_int_unmask_flag;
    ZXIC_UINT32 hash2_int_unmask_flag;
    ZXIC_UINT32 hash3_int_unmask_flag;
    ZXIC_UINT32 lpm_int_unmask_flag;
}DPP_SE_ALG_SE_ALG_INT_STATUS_T;

typedef struct dpp_se_alg_schd_int_en_t
{
    ZXIC_UINT32 wr_rsp_fifo_ovfl;
    ZXIC_UINT32 init_rd_cft_en;
    ZXIC_UINT32 schd_lpm_fifo_parity_errl;
    ZXIC_UINT32 schd_hash3_fifo_parity_err;
    ZXIC_UINT32 schd_hash2_fifo_parity_err;
    ZXIC_UINT32 schd_hash1_fifo_parity_err;
    ZXIC_UINT32 schd_hash0_fifo_parity_err;
    ZXIC_UINT32 schd_learn_fifo_parity_err;
    ZXIC_UINT32 schd_lpm_fifo_ovfl;
    ZXIC_UINT32 schd_hash3_fifo_ovfl;
    ZXIC_UINT32 schd_hash2_fifo_unfl;
    ZXIC_UINT32 schd_hash1_fifo_ovfl;
    ZXIC_UINT32 schd_hash0_fifo_ovfl;
    ZXIC_UINT32 schd_learn_fifo_ovfl;
}DPP_SE_ALG_SCHD_INT_EN_T;

typedef struct dpp_se_alg_schd_int_mask_t
{
    ZXIC_UINT32 schd_int_mask;
}DPP_SE_ALG_SCHD_INT_MASK_T;

typedef struct dpp_se_alg_schd_int_status_t
{
    ZXIC_UINT32 schd_int_status;
}DPP_SE_ALG_SCHD_INT_STATUS_T;

typedef struct dpp_se_alg_zblk_ecc_int_en_t
{
    ZXIC_UINT32 zblk_ecc_int_en;
}DPP_SE_ALG_ZBLK_ECC_INT_EN_T;

typedef struct dpp_se_alg_zblk_ecc_int_mask_t
{
    ZXIC_UINT32 zblk_ecc_int_mask;
}DPP_SE_ALG_ZBLK_ECC_INT_MASK_T;

typedef struct dpp_se_alg_zblk_ecc_int_status_t
{
    ZXIC_UINT32 zblk_ecc_int_status;
}DPP_SE_ALG_ZBLK_ECC_INT_STATUS_T;

typedef struct dpp_se_alg_hash0_int_en_t
{
    ZXIC_UINT32 zcam_hash_p0_err_en;
    ZXIC_UINT32 hash0_agree_int_fifo_ovf_en;
    ZXIC_UINT32 hash0_agree_ext_fifo_ovf_en;
    ZXIC_UINT32 hash0_agree_ext_fifo_parity_err_en;
    ZXIC_UINT32 hash0_agree_int_fifo_parity_err_en;
    ZXIC_UINT32 hash0_key_fifo_ovfl_en;
    ZXIC_UINT32 hash0_sreq_fifo_ovfl_en;
    ZXIC_UINT32 hash0_key_fifo_parity_err_en;
}DPP_SE_ALG_HASH0_INT_EN_T;

typedef struct dpp_se_alg_hash0_int_mask_t
{
    ZXIC_UINT32 hash0_int_mask;
}DPP_SE_ALG_HASH0_INT_MASK_T;

typedef struct dpp_se_alg_hash0_int_status_t
{
    ZXIC_UINT32 hash0_int_status;
}DPP_SE_ALG_HASH0_INT_STATUS_T;

typedef struct dpp_se_alg_hash1_int_en_t
{
    ZXIC_UINT32 zcam_hash_p1_err_en;
    ZXIC_UINT32 hash1_agree_int_fifo_ovf_en;
    ZXIC_UINT32 hash1_agree_ext_fifo_ovf_en;
    ZXIC_UINT32 hash1_agree_ext_fifo_parity_err_en;
    ZXIC_UINT32 hash1_agree_int_fifo_parity_err_en;
    ZXIC_UINT32 hash1_key_fifo_ovfl_en;
    ZXIC_UINT32 hash1_sreq_fifo_ovfl_en;
    ZXIC_UINT32 hash1_key_fifo_parity_err_en;
}DPP_SE_ALG_HASH1_INT_EN_T;

typedef struct dpp_se_alg_hash1_int_mask_t
{
    ZXIC_UINT32 hash1_int_mask;
}DPP_SE_ALG_HASH1_INT_MASK_T;

typedef struct dpp_se_alg_hash1_int_status_t
{
    ZXIC_UINT32 hash1_int_status;
}DPP_SE_ALG_HASH1_INT_STATUS_T;

typedef struct dpp_se_alg_hash2_int_en_t
{
    ZXIC_UINT32 zcam_hash_p2_err_en;
    ZXIC_UINT32 hash2_agree_int_fifo_ovf_en;
    ZXIC_UINT32 hash2_agree_ext_fifo_ovf_en;
    ZXIC_UINT32 hash2_agree_ext_fifo_parity_err_en;
    ZXIC_UINT32 hash2_agree_int_fifo_parity_err_en;
    ZXIC_UINT32 hash2_key_fifo_ovfl_en;
    ZXIC_UINT32 hash2_sreq_fifo_ovfl_en;
    ZXIC_UINT32 hash2_key_fifo_parity_err_en;
}DPP_SE_ALG_HASH2_INT_EN_T;

typedef struct dpp_se_alg_hash2_int_mask_t
{
    ZXIC_UINT32 hash2_int_mask;
}DPP_SE_ALG_HASH2_INT_MASK_T;

typedef struct dpp_se_alg_hash2_int_status_t
{
    ZXIC_UINT32 hash2_int_status;
}DPP_SE_ALG_HASH2_INT_STATUS_T;

typedef struct dpp_se_alg_hash3_int_en_t
{
    ZXIC_UINT32 zcam_hash_p3_err_en;
    ZXIC_UINT32 hash3_agree_int_fifo_ovf_en;
    ZXIC_UINT32 hash3_agree_ext_fifo_ovf_en;
    ZXIC_UINT32 hash3_agree_ext_fifo_parity_err_en;
    ZXIC_UINT32 hash3_agree_int_fifo_parity_err_en;
    ZXIC_UINT32 hash3_key_fifo_ovfl_en;
    ZXIC_UINT32 hash3_sreq_fifo_ovfl_en;
    ZXIC_UINT32 hash3_key_fifo_parity_err_en;
}DPP_SE_ALG_HASH3_INT_EN_T;

typedef struct dpp_se_alg_hash3_int_mask_t
{
    ZXIC_UINT32 hash3_int_mask;
}DPP_SE_ALG_HASH3_INT_MASK_T;

typedef struct dpp_se_alg_hash3_int_status_t
{
    ZXIC_UINT32 hash3_int_status;
}DPP_SE_ALG_HASH3_INT_STATUS_T;

typedef struct dpp_se_alg_lpm_int_en_t
{
    ZXIC_UINT32 zcam_lpm_err_en;
    ZXIC_UINT32 lpm_as_int_rsp_fifo_ovfl_en;
    ZXIC_UINT32 lpm_as_req_fifo_ovfl_en;
    ZXIC_UINT32 lpm_ext_ddr_rsp_fifo_parity_en;
    ZXIC_UINT32 lpm_ext_v6_key_parity_en;
    ZXIC_UINT32 lpm_ext_v4_key_parity_en;
    ZXIC_UINT32 lpm_ext_addr_fifo_ovfl_en;
    ZXIC_UINT32 lpm_ext_v4_fifo_ovfl_en;
    ZXIC_UINT32 lpm_ext_v6_fifo_ovfl_en;
    ZXIC_UINT32 lpm_ext_ddr_rsp_ovf_en;
}DPP_SE_ALG_LPM_INT_EN_T;

typedef struct dpp_se_alg_lpm_int_mask_t
{
    ZXIC_UINT32 lpm_int_mask;
}DPP_SE_ALG_LPM_INT_MASK_T;

typedef struct dpp_se_alg_lpm_int_status_t
{
    ZXIC_UINT32 lpm_int_status;
}DPP_SE_ALG_LPM_INT_STATUS_T;

typedef struct dpp_se_alg_zblock_lpm_mask0_t
{
    ZXIC_UINT32 vpn_id_mask;
    ZXIC_UINT32 prefix0_mask;
    ZXIC_UINT32 prefix1_mask;
    ZXIC_UINT32 prefix2_mask;
    ZXIC_UINT32 prefix3_mask;
}DPP_SE_ALG_ZBLOCK_LPM_MASK0_T;

typedef struct dpp_se_alg_zblock_lpm_mask1_t
{
    ZXIC_UINT32 vpn_id_mask;
    ZXIC_UINT32 prefix0_mask;
    ZXIC_UINT32 prefix1_mask;
    ZXIC_UINT32 prefix2_mask;
    ZXIC_UINT32 prefix3_mask;
}DPP_SE_ALG_ZBLOCK_LPM_MASK1_T;

typedef struct dpp_se_alg_zblock_lpm_mask2_t
{
    ZXIC_UINT32 vpn_id_mask;
    ZXIC_UINT32 prefix0_mask;
    ZXIC_UINT32 prefix1_mask;
    ZXIC_UINT32 prefix2_mask;
    ZXIC_UINT32 prefix3_mask;
}DPP_SE_ALG_ZBLOCK_LPM_MASK2_T;

typedef struct dpp_se_alg_zblock_lpm_mask3_t
{
    ZXIC_UINT32 vpn_id_mask;
    ZXIC_UINT32 prefix0_mask;
    ZXIC_UINT32 prefix1_mask;
    ZXIC_UINT32 prefix2_mask;
    ZXIC_UINT32 prefix3_mask;
}DPP_SE_ALG_ZBLOCK_LPM_MASK3_T;

typedef struct dpp_se_alg_zblock_default_route0_t
{
    ZXIC_UINT32 vpn_id;
    ZXIC_UINT32 vpn_dresult;
    ZXIC_UINT32 vpn_flag;
    ZXIC_UINT32 vpn_vld;
}DPP_SE_ALG_ZBLOCK_DEFAULT_ROUTE0_T;

typedef struct dpp_se_alg_zblock_default_route1_t
{
    ZXIC_UINT32 vpn_id;
    ZXIC_UINT32 vpn_dresult;
    ZXIC_UINT32 vpn_flag;
    ZXIC_UINT32 vpn_vld;
}DPP_SE_ALG_ZBLOCK_DEFAULT_ROUTE1_T;

typedef struct dpp_se_alg_zblock_default_route2_t
{
    ZXIC_UINT32 vpn_id;
    ZXIC_UINT32 vpn_dresult;
    ZXIC_UINT32 vpn_flag;
    ZXIC_UINT32 vpn_vld;
}DPP_SE_ALG_ZBLOCK_DEFAULT_ROUTE2_T;

typedef struct dpp_se_alg_zblock_default_route3_t
{
    ZXIC_UINT32 vpn_id;
    ZXIC_UINT32 vpn_dresult;
    ZXIC_UINT32 vpn_flag;
    ZXIC_UINT32 vpn_vld;
}DPP_SE_ALG_ZBLOCK_DEFAULT_ROUTE3_T;

typedef struct dpp_se_alg_zblock_default_route4_t
{
    ZXIC_UINT32 vpn_id;
    ZXIC_UINT32 vpn_dresult;
    ZXIC_UINT32 vpn_flag;
    ZXIC_UINT32 vpn_vld;
}DPP_SE_ALG_ZBLOCK_DEFAULT_ROUTE4_T;

typedef struct dpp_se_alg_zblock_default_route5_t
{
    ZXIC_UINT32 vpn_id;
    ZXIC_UINT32 vpn_dresult;
    ZXIC_UINT32 vpn_flag;
    ZXIC_UINT32 vpn_vld;
}DPP_SE_ALG_ZBLOCK_DEFAULT_ROUTE5_T;

typedef struct dpp_se_alg_zblock_default_route6_t
{
    ZXIC_UINT32 vpn_id;
    ZXIC_UINT32 vpn_dresult;
    ZXIC_UINT32 vpn_flag;
    ZXIC_UINT32 vpn_vld;
}DPP_SE_ALG_ZBLOCK_DEFAULT_ROUTE6_T;

typedef struct dpp_se_alg_zblock_default_route7_t
{
    ZXIC_UINT32 vpn_id;
    ZXIC_UINT32 vpn_dresult;
    ZXIC_UINT32 vpn_flag;
    ZXIC_UINT32 vpn_vld;
}DPP_SE_ALG_ZBLOCK_DEFAULT_ROUTE7_T;

typedef struct dpp_se_alg_zblock_hash_listtable_item0_t
{
    ZXIC_UINT32 hash_item;
}DPP_SE_ALG_ZBLOCK_HASH_LISTTABLE_ITEM0_T;

typedef struct dpp_se_alg_zblock_hash_listtable_item1_t
{
    ZXIC_UINT32 hash_item;
}DPP_SE_ALG_ZBLOCK_HASH_LISTTABLE_ITEM1_T;

typedef struct dpp_se_alg_zblock_hash_listtable_item2_t
{
    ZXIC_UINT32 hash_item;
}DPP_SE_ALG_ZBLOCK_HASH_LISTTABLE_ITEM2_T;

typedef struct dpp_se_alg_zblock_hash_listtable_item3_t
{
    ZXIC_UINT32 hash_item;
}DPP_SE_ALG_ZBLOCK_HASH_LISTTABLE_ITEM3_T;

typedef struct dpp_se_alg_zblock_ecc_err_status_t
{
    ZXIC_UINT32 sram3_ecc_err;
    ZXIC_UINT32 sram2_ecc_err;
    ZXIC_UINT32 sram1_ecc_err;
    ZXIC_UINT32 sram0_ecc_err;
}DPP_SE_ALG_ZBLOCK_ECC_ERR_STATUS_T;

typedef struct dpp_se_alg_zblock_lpm_v6_sram_cmp_t
{
    ZXIC_UINT32 sram_cmp_flag;
}DPP_SE_ALG_ZBLOCK_LPM_V6_SRAM_CMP_T;

typedef struct dpp_se_alg_zblock_lpm_v4_sram_cmp_t
{
    ZXIC_UINT32 sram_cmp_flag;
}DPP_SE_ALG_ZBLOCK_LPM_V4_SRAM_CMP_T;

typedef struct dpp_se_parser_kschd_pful_cfg_t
{
    ZXIC_UINT32 kschd_pful_assert;
    ZXIC_UINT32 kschd_pful_negate;
}DPP_SE_PARSER_KSCHD_PFUL_CFG_T;

typedef struct dpp_se_parser_debug_cnt_mode_t
{
    ZXIC_UINT32 cnt_rd_mode;
    ZXIC_UINT32 cnt_overflow_mode;
}DPP_SE_PARSER_DEBUG_CNT_MODE_T;

typedef struct dpp_se_parser_parser_int_en_t
{
    ZXIC_UINT32 parser_int_en;
}DPP_SE_PARSER_PARSER_INT_EN_T;

typedef struct dpp_se_parser_parser_int_mask_t
{
    ZXIC_UINT32 parser_int_mask;
}DPP_SE_PARSER_PARSER_INT_MASK_T;

typedef struct dpp_se_parser_parser_int_status_t
{
    ZXIC_UINT32 parser_int_status;
}DPP_SE_PARSER_PARSER_INT_STATUS_T;

typedef struct dpp_se_parser_parser_int_unmask_flag_t
{
    ZXIC_UINT32 parser_int_unmask_flag;
}DPP_SE_PARSER_PARSER_INT_UNMASK_FLAG_T;

typedef struct dpp_se_parser_ecc_bypass_read_t
{
    ZXIC_UINT32 ecc_bypass_read;
}DPP_SE_PARSER_ECC_BYPASS_READ_T;

typedef struct dpp_se_parser_mex0_5_req_cnt_t
{
    ZXIC_UINT32 mex0_5_req_cnt;
}DPP_SE_PARSER_MEX0_5_REQ_CNT_T;

typedef struct dpp_se_parser_kschd_req0_5_cnt_t
{
    ZXIC_UINT32 kschd_req0_5_cnt;
}DPP_SE_PARSER_KSCHD_REQ0_5_CNT_T;

typedef struct dpp_se_parser_kschd_parser_fc0_5_cnt_t
{
    ZXIC_UINT32 kschd_parser_fc0_5_cnt;
}DPP_SE_PARSER_KSCHD_PARSER_FC0_5_CNT_T;

typedef struct dpp_se_parser_se_ppu_mex0_5_fc_cnt_t
{
    ZXIC_UINT32 se_ppu_mex0_5_fc_cnt;
}DPP_SE_PARSER_SE_PPU_MEX0_5_FC_CNT_T;

typedef struct dpp_se_parser_smmu0_marc_fc_cnt_t
{
    ZXIC_UINT32 smmu0_marc_fc_cnt;
}DPP_SE_PARSER_SMMU0_MARC_FC_CNT_T;

typedef struct dpp_se_parser_smmu0_marc_key_cnt_t
{
    ZXIC_UINT32 smmu0_marc_key_cnt;
}DPP_SE_PARSER_SMMU0_MARC_KEY_CNT_T;

typedef struct dpp_se_parser_cmmu_key_cnt_t
{
    ZXIC_UINT32 cmmu_key_cnt;
}DPP_SE_PARSER_CMMU_KEY_CNT_T;

typedef struct dpp_se_parser_cmmu_parser_fc_cnt_t
{
    ZXIC_UINT32 cmmu_parser_fc_cnt;
}DPP_SE_PARSER_CMMU_PARSER_FC_CNT_T;

typedef struct dpp_se_parser_marc_tab_type_err_mex0_5_cnt_t
{
    ZXIC_UINT32 marc_tab_type_err_mex0_5_cnt;
}DPP_SE_PARSER_MARC_TAB_TYPE_ERR_MEX0_5_CNT_T;

typedef struct dpp_se_parser_eram_fulladdr_drop_cnt_t
{
    ZXIC_UINT32 eram_fulladdr_drop_cnt;
}DPP_SE_PARSER_ERAM_FULLADDR_DROP_CNT_T;

typedef struct dpp_se_as_hash0_pful_cfg_t
{
    ZXIC_UINT32 hash0_pful_cfg;
}DPP_SE_AS_HASH0_PFUL_CFG_T;

typedef struct dpp_se_as_hash1_pful_cfg_t
{
    ZXIC_UINT32 hash1_pful_cfg;
}DPP_SE_AS_HASH1_PFUL_CFG_T;

typedef struct dpp_se_as_hash2_pful_cfg_t
{
    ZXIC_UINT32 hash2_pful_cfg;
}DPP_SE_AS_HASH2_PFUL_CFG_T;

typedef struct dpp_se_as_hash3_pful_cfg_t
{
    ZXIC_UINT32 hash3_pful_cfg;
}DPP_SE_AS_HASH3_PFUL_CFG_T;

typedef struct dpp_se_as_pbu_pful_cfg_t
{
    ZXIC_UINT32 pbu_pful_cfg;
}DPP_SE_AS_PBU_PFUL_CFG_T;

typedef struct dpp_se_as_lpm_pful_cfg_t
{
    ZXIC_UINT32 lpm_pful_cfg;
}DPP_SE_AS_LPM_PFUL_CFG_T;

typedef struct dpp_se_as_etcam_pful_cfg_t
{
    ZXIC_UINT32 etcam_pful_cfg;
}DPP_SE_AS_ETCAM_PFUL_CFG_T;

typedef struct dpp_se_as_as_learn0_fifo_cfg_t
{
    ZXIC_UINT32 as_learn1_pful_negate;
    ZXIC_UINT32 as_learn1_pful_asert;
    ZXIC_UINT32 as_learn0_pful_negate;
    ZXIC_UINT32 as_learn0_pful_asert;
}DPP_SE_AS_AS_LEARN0_FIFO_CFG_T;

typedef struct dpp_se_as_as_learn1_fifo_cfg_t
{
    ZXIC_UINT32 as_learn3_pful_negate;
    ZXIC_UINT32 as_learn3_pful_asert;
    ZXIC_UINT32 as_learn2_pful_negate;
    ZXIC_UINT32 as_learn2_pful_asert;
}DPP_SE_AS_AS_LEARN1_FIFO_CFG_T;

typedef struct dpp_se_as_as_dma_fifo_cfg_t
{
    ZXIC_UINT32 as_dma_fifo_cfg;
}DPP_SE_AS_AS_DMA_FIFO_CFG_T;

typedef struct dpp_se_as_age_pful_cfg_t
{
    ZXIC_UINT32 age_pful_cfg;
}DPP_SE_AS_AGE_PFUL_CFG_T;

typedef struct dpp_se_as_etcam_rsp_cfg_t
{
    ZXIC_UINT32 eram_rsp_pful_negate;
    ZXIC_UINT32 eram_rsp_pful_assert;
    ZXIC_UINT32 etcam_rsp_pful_negate;
    ZXIC_UINT32 etcam_rsp_pful_assert;
}DPP_SE_AS_ETCAM_RSP_CFG_T;

typedef struct dpp_se_as_pbu_ecc_bypass_read_t
{
    ZXIC_UINT32 pbu_ecc_bypass_read;
}DPP_SE_AS_PBU_ECC_BYPASS_READ_T;

typedef struct dpp_se_as_etcam0_ecc_bypass_read_t
{
    ZXIC_UINT32 etcam0_ecc_bypass_read;
}DPP_SE_AS_ETCAM0_ECC_BYPASS_READ_T;

typedef struct dpp_se_as_etcam1_ecc_bypass_read_t
{
    ZXIC_UINT32 etcam1_ecc_bypass_read;
}DPP_SE_AS_ETCAM1_ECC_BYPASS_READ_T;

typedef struct dpp_se_as_lpm_ecc_bypass_read_t
{
    ZXIC_UINT32 lpm_ecc_bypass_read;
}DPP_SE_AS_LPM_ECC_BYPASS_READ_T;

typedef struct dpp_se_as_hash_ecc_bypass_read_t
{
    ZXIC_UINT32 hash3_ecc_bypass_read;
    ZXIC_UINT32 hash2_ecc_bypass_read;
    ZXIC_UINT32 hash1_ecc_bypass_read;
    ZXIC_UINT32 hash0_ecc_bypass_read;
}DPP_SE_AS_HASH_ECC_BYPASS_READ_T;

typedef struct dpp_se_as_hash_learn_ecc_bypass_read_t
{
    ZXIC_UINT32 hash_learn_ecc_bypass_read;
}DPP_SE_AS_HASH_LEARN_ECC_BYPASS_READ_T;

typedef struct dpp_se_as_debug_cnt_mode_t
{
    ZXIC_UINT32 cnt_rd_mode;
    ZXIC_UINT32 cnt_overflow_mode;
}DPP_SE_AS_DEBUG_CNT_MODE_T;

typedef struct dpp_se_as_as_int_0_en_t
{
    ZXIC_UINT32 as_int_0_en;
}DPP_SE_AS_AS_INT_0_EN_T;

typedef struct dpp_se_as_as_int_0_mask_t
{
    ZXIC_UINT32 as_int_0_mask;
}DPP_SE_AS_AS_INT_0_MASK_T;

typedef struct dpp_se_as_as_int_1_en_t
{
    ZXIC_UINT32 as_int_1_en;
}DPP_SE_AS_AS_INT_1_EN_T;

typedef struct dpp_se_as_as_int_1_mask_t
{
    ZXIC_UINT32 as_int_1_mask;
}DPP_SE_AS_AS_INT_1_MASK_T;

typedef struct dpp_se_as_as_int_2_en_t
{
    ZXIC_UINT32 as_int_2_en;
}DPP_SE_AS_AS_INT_2_EN_T;

typedef struct dpp_se_as_as_int_2_mask_t
{
    ZXIC_UINT32 as_int_2_mask;
}DPP_SE_AS_AS_INT_2_MASK_T;

typedef struct dpp_se_as_as_int_0_status_t
{
    ZXIC_UINT32 port0_int_status;
}DPP_SE_AS_AS_INT_0_STATUS_T;

typedef struct dpp_se_as_as_int_1_status_t
{
    ZXIC_UINT32 port1_int_status;
}DPP_SE_AS_AS_INT_1_STATUS_T;

typedef struct dpp_se_as_as_int_2_status_t
{
    ZXIC_UINT32 port2_int_status;
}DPP_SE_AS_AS_INT_2_STATUS_T;

typedef struct dpp_se_as_se_as_int_status_t
{
    ZXIC_UINT32 as_int_2_unmask_flag;
    ZXIC_UINT32 as_int_1_unmask_flag;
    ZXIC_UINT32 as_int_0_unmask_flag;
}DPP_SE_AS_SE_AS_INT_STATUS_T;

typedef struct dpp_se_as_hash0_3_wr_req_cnt_t
{
    ZXIC_UINT32 hash0_3_wr_req_cnt;
}DPP_SE_AS_HASH0_3_WR_REQ_CNT_T;

typedef struct dpp_se_as_smmu0_etcam0_1_fc_cnt_t
{
    ZXIC_UINT32 smmu0_etcam0_1_fc_cnt;
}DPP_SE_AS_SMMU0_ETCAM0_1_FC_CNT_T;

typedef struct dpp_se_as_etcam0_1_smmu0_req_cnt_t
{
    ZXIC_UINT32 etcam0_1_smmu0_req_cnt;
}DPP_SE_AS_ETCAM0_1_SMMU0_REQ_CNT_T;

typedef struct dpp_se_as_smmu0_etcam0_1_rsp_cnt_t
{
    ZXIC_UINT32 smmu0_etcam0_1_rsp_cnt;
}DPP_SE_AS_SMMU0_ETCAM0_1_RSP_CNT_T;

typedef struct dpp_se_as_as_hla_hash_p0_3_key_cnt_t
{
    ZXIC_UINT32 as_hla_hash_p0_3_key_cnt;
}DPP_SE_AS_AS_HLA_HASH_P0_3_KEY_CNT_T;

typedef struct dpp_se_as_as_hla_lpm_p0_key_cnt_t
{
    ZXIC_UINT32 as_hla_lpm_p0_key_cnt;
}DPP_SE_AS_AS_HLA_LPM_P0_KEY_CNT_T;

typedef struct dpp_se_as_alg_as_hash_p0_3_rsp_cnt_t
{
    ZXIC_UINT32 alg_as_hash_p0_3_rsp_cnt;
}DPP_SE_AS_ALG_AS_HASH_P0_3_RSP_CNT_T;

typedef struct dpp_se_as_alg_as_hash_p0_3_smf_rsp_cnt_t
{
    ZXIC_UINT32 alg_as_hash_p0_3_smf_rsp_cnt;
}DPP_SE_AS_ALG_AS_HASH_P0_3_SMF_RSP_CNT_T;

typedef struct dpp_se_as_alg_as_lpm_p0_rsp_cnt_t
{
    ZXIC_UINT32 alg_as_lpm_p0_rsp_cnt;
}DPP_SE_AS_ALG_AS_LPM_P0_RSP_CNT_T;

typedef struct dpp_se_as_alg_as_lpm_p0_3_smf_rsp_cnt_t
{
    ZXIC_UINT32 alg_as_lpm_p0_3_smf_rsp_cnt;
}DPP_SE_AS_ALG_AS_LPM_P0_3_SMF_RSP_CNT_T;

typedef struct dpp_se_as_as_pbu_key_cnt_t
{
    ZXIC_UINT32 as_pbu_key_cnt;
}DPP_SE_AS_AS_PBU_KEY_CNT_T;

typedef struct dpp_se_as_pbu_se_dpi_rsp_dat_cnt_t
{
    ZXIC_UINT32 pbu_se_dpi_rsp_dat_cnt;
}DPP_SE_AS_PBU_SE_DPI_RSP_DAT_CNT_T;

typedef struct dpp_se_as_as_etcam_ctrl_req0_cnt_t
{
    ZXIC_UINT32 as_etcam_ctrl_req0_cnt;
}DPP_SE_AS_AS_ETCAM_CTRL_REQ0_CNT_T;

typedef struct dpp_se_as_etcam_ctrl_as_index0_1_cnt_t
{
    ZXIC_UINT32 etcam_ctrl_as_index0_1_cnt;
}DPP_SE_AS_ETCAM_CTRL_AS_INDEX0_1_CNT_T;

typedef struct dpp_se_as_etcam_ctrl_as_hit0_1_cnt_t
{
    ZXIC_UINT32 etcam_ctrl_as_hit0_1_cnt;
}DPP_SE_AS_ETCAM_CTRL_AS_HIT0_1_CNT_T;

typedef struct dpp_se_as_as_smmu0_req_cnt_t
{
    ZXIC_UINT32 as_smmu0_req_cnt;
}DPP_SE_AS_AS_SMMU0_REQ_CNT_T;

typedef struct dpp_se_as_learn_hla_wr_cnt_t
{
    ZXIC_UINT32 learn_hla_wr_cnt;
}DPP_SE_AS_LEARN_HLA_WR_CNT_T;

typedef struct dpp_se_as_as_smmu1_req_cnt_t
{
    ZXIC_UINT32 as_smmu1_req_cnt;
}DPP_SE_AS_AS_SMMU1_REQ_CNT_T;

typedef struct dpp_se_as_se_cfg_mac_dat_cnt_t
{
    ZXIC_UINT32 se_cfg_mac_dat_cnt;
}DPP_SE_AS_SE_CFG_MAC_DAT_CNT_T;

typedef struct dpp_se_as_alg_as_hash_p0_3_fc_cnt_t
{
    ZXIC_UINT32 alg_as_hash_p0_3_fc_cnt;
}DPP_SE_AS_ALG_AS_HASH_P0_3_FC_CNT_T;

typedef struct dpp_se_as_alg_as_lpm_p0_fc_cnt_t
{
    ZXIC_UINT32 alg_as_lpm_p0_fc_cnt;
}DPP_SE_AS_ALG_AS_LPM_P0_FC_CNT_T;

typedef struct dpp_se_as_as_alg_hash_p0_3_fc_cnt_t
{
    ZXIC_UINT32 as_alg_hash_p0_3_fc_cnt;
}DPP_SE_AS_AS_ALG_HASH_P0_3_FC_CNT_T;

typedef struct dpp_se_as_as_alg_lpm_p0_fc_cnt_t
{
    ZXIC_UINT32 as_alg_lpm_p0_fc_cnt;
}DPP_SE_AS_AS_ALG_LPM_P0_FC_CNT_T;

typedef struct dpp_se_as_as_pbu_fc_cnt_t
{
    ZXIC_UINT32 as_pbu_fc_cnt;
}DPP_SE_AS_AS_PBU_FC_CNT_T;

typedef struct dpp_se_as_pbu_se_dpi_key_fc_cnt_t
{
    ZXIC_UINT32 pbu_se_dpi_key_fc_cnt;
}DPP_SE_AS_PBU_SE_DPI_KEY_FC_CNT_T;

typedef struct dpp_se_as_as_etcam_ctrl_fc0_1_cnt_t
{
    ZXIC_UINT32 as_etcam_ctrl_fc0_1_cnt;
}DPP_SE_AS_AS_ETCAM_CTRL_FC0_1_CNT_T;

typedef struct dpp_se_as_etcam_ctrl_as_fc0_1_cnt_t
{
    ZXIC_UINT32 etcam_ctrl_as_fc0_1_cnt;
}DPP_SE_AS_ETCAM_CTRL_AS_FC0_1_CNT_T;

typedef struct dpp_se_as_smmu0_as_mac_age_fc_cnt_t
{
    ZXIC_UINT32 smmu0_as_mac_age_fc_cnt;
}DPP_SE_AS_SMMU0_AS_MAC_AGE_FC_CNT_T;

typedef struct dpp_se_as_alg_learn_fc_cnt_t
{
    ZXIC_UINT32 alg_learn_fc_cnt;
}DPP_SE_AS_ALG_LEARN_FC_CNT_T;

typedef struct dpp_se_as_smmu1_as_fc_cnt_t
{
    ZXIC_UINT32 smmu1_as_fc_cnt;
}DPP_SE_AS_SMMU1_AS_FC_CNT_T;

typedef struct dpp_se_as_cfg_se_mac_fc_cnt_t
{
    ZXIC_UINT32 cfg_se_mac_fc_cnt;
}DPP_SE_AS_CFG_SE_MAC_FC_CNT_T;

typedef struct dpp_se_kschd_kschd_cpu_rdy_t
{
    ZXIC_UINT32 kschd_cpu_rdy;
}DPP_SE_KSCHD_KSCHD_CPU_RDY_T;

typedef struct dpp_se_kschd_ppu0_ecc_bypass_read_t
{
    ZXIC_UINT32 ppu0_ecc_bypass_read;
}DPP_SE_KSCHD_PPU0_ECC_BYPASS_READ_T;

typedef struct dpp_se_kschd_pbu_ecc_bypass_read_t
{
    ZXIC_UINT32 pbu_ecc_bypass_read;
}DPP_SE_KSCHD_PBU_ECC_BYPASS_READ_T;

typedef struct dpp_se_kschd_smmu1_ecc_bypass_read_t
{
    ZXIC_UINT32 u3_smmu1_ecc_bypass_read;
    ZXIC_UINT32 u2_smmu1_ecc_bypass_read;
    ZXIC_UINT32 u1_smmu1_ecc_bypass_read;
    ZXIC_UINT32 u0_smmu1_ecc_bypass_read;
}DPP_SE_KSCHD_SMMU1_ECC_BYPASS_READ_T;

typedef struct dpp_se_kschd_ass_ecc_bypass_read_t
{
    ZXIC_UINT32 ass_ecc_bypass_read;
}DPP_SE_KSCHD_ASS_ECC_BYPASS_READ_T;

typedef struct dpp_se_kschd_sdt_h_t
{
    ZXIC_UINT32 sdt_h;
}DPP_SE_KSCHD_SDT_H_T;

typedef struct dpp_se_kschd_sdt_l_t
{
    ZXIC_UINT32 sdt_l;
}DPP_SE_KSCHD_SDT_L_T;

typedef struct dpp_se_kschd_hash_key15_t
{
    ZXIC_UINT32 dma_en;
    ZXIC_UINT32 delete_en;
    ZXIC_UINT32 hash_key15;
}DPP_SE_KSCHD_HASH_KEY15_T;

typedef struct dpp_se_kschd_hash_key14_t
{
    ZXIC_UINT32 hash_key14;
}DPP_SE_KSCHD_HASH_KEY14_T;

typedef struct dpp_se_kschd_hash_key13_t
{
    ZXIC_UINT32 hash_key13;
}DPP_SE_KSCHD_HASH_KEY13_T;

typedef struct dpp_se_kschd_hash_key12_t
{
    ZXIC_UINT32 hash_key12;
}DPP_SE_KSCHD_HASH_KEY12_T;

typedef struct dpp_se_kschd_hash_key11_t
{
    ZXIC_UINT32 hash_key11;
}DPP_SE_KSCHD_HASH_KEY11_T;

typedef struct dpp_se_kschd_hash_key10_t
{
    ZXIC_UINT32 hash_key10;
}DPP_SE_KSCHD_HASH_KEY10_T;

typedef struct dpp_se_kschd_hash_key9_t
{
    ZXIC_UINT32 hash_key9;
}DPP_SE_KSCHD_HASH_KEY9_T;

typedef struct dpp_se_kschd_hash_key8_t
{
    ZXIC_UINT32 hash_key8;
}DPP_SE_KSCHD_HASH_KEY8_T;

typedef struct dpp_se_kschd_hash_key7_t
{
    ZXIC_UINT32 hash_key7;
}DPP_SE_KSCHD_HASH_KEY7_T;

typedef struct dpp_se_kschd_hash_key6_t
{
    ZXIC_UINT32 hash_key6;
}DPP_SE_KSCHD_HASH_KEY6_T;

typedef struct dpp_se_kschd_hash_key5_t
{
    ZXIC_UINT32 hash_key5;
}DPP_SE_KSCHD_HASH_KEY5_T;

typedef struct dpp_se_kschd_hash_key4_t
{
    ZXIC_UINT32 hash_key4;
}DPP_SE_KSCHD_HASH_KEY4_T;

typedef struct dpp_se_kschd_hash_key3_t
{
    ZXIC_UINT32 hash_key3;
}DPP_SE_KSCHD_HASH_KEY3_T;

typedef struct dpp_se_kschd_hash_key2_t
{
    ZXIC_UINT32 hash_key2;
}DPP_SE_KSCHD_HASH_KEY2_T;

typedef struct dpp_se_kschd_hash_key1_t
{
    ZXIC_UINT32 hash_key1;
}DPP_SE_KSCHD_HASH_KEY1_T;

typedef struct dpp_se_kschd_hash_key0_t
{
    ZXIC_UINT32 hash_key0;
}DPP_SE_KSCHD_HASH_KEY0_T;

typedef struct dpp_se_kschd_schd_int_0_en_t
{
    ZXIC_UINT32 port0_int_en;
}DPP_SE_KSCHD_SCHD_INT_0_EN_T;

typedef struct dpp_se_kschd_schd_int_0_mask_t
{
    ZXIC_UINT32 port0_int_mask;
}DPP_SE_KSCHD_SCHD_INT_0_MASK_T;

typedef struct dpp_se_kschd_schd_int_1_en_t
{
    ZXIC_UINT32 port1_int_en;
}DPP_SE_KSCHD_SCHD_INT_1_EN_T;

typedef struct dpp_se_kschd_schd_int_1_mask_t
{
    ZXIC_UINT32 port1_int_mask;
}DPP_SE_KSCHD_SCHD_INT_1_MASK_T;

typedef struct dpp_se_kschd_schd_int_2_en_t
{
    ZXIC_UINT32 port2_int_en;
}DPP_SE_KSCHD_SCHD_INT_2_EN_T;

typedef struct dpp_se_kschd_schd_int_2_mask_t
{
    ZXIC_UINT32 port2_int_mask;
}DPP_SE_KSCHD_SCHD_INT_2_MASK_T;

typedef struct dpp_se_kschd_schd_int_3_en_t
{
    ZXIC_UINT32 port3_int_en;
}DPP_SE_KSCHD_SCHD_INT_3_EN_T;

typedef struct dpp_se_kschd_schd_int_3_mask_t
{
    ZXIC_UINT32 port3_int_mask;
}DPP_SE_KSCHD_SCHD_INT_3_MASK_T;

typedef struct dpp_se_kschd_schd_int_4_en_t
{
    ZXIC_UINT32 port4_int_en;
}DPP_SE_KSCHD_SCHD_INT_4_EN_T;

typedef struct dpp_se_kschd_schd_int_4_mask_t
{
    ZXIC_UINT32 port4_int_mask;
}DPP_SE_KSCHD_SCHD_INT_4_MASK_T;

typedef struct dpp_se_kschd_schd_int_0_status_t
{
    ZXIC_UINT32 port0_int_status;
}DPP_SE_KSCHD_SCHD_INT_0_STATUS_T;

typedef struct dpp_se_kschd_schd_int_1_status_t
{
    ZXIC_UINT32 port1_int_status;
}DPP_SE_KSCHD_SCHD_INT_1_STATUS_T;

typedef struct dpp_se_kschd_schd_int_2_status_t
{
    ZXIC_UINT32 port2_int_status;
}DPP_SE_KSCHD_SCHD_INT_2_STATUS_T;

typedef struct dpp_se_kschd_schd_int_3_status_t
{
    ZXIC_UINT32 port3_int_status;
}DPP_SE_KSCHD_SCHD_INT_3_STATUS_T;

typedef struct dpp_se_kschd_schd_int_4_status_t
{
    ZXIC_UINT32 port4_int_status;
}DPP_SE_KSCHD_SCHD_INT_4_STATUS_T;

typedef struct dpp_se_kschd_se_kschd_int_status_t
{
    ZXIC_UINT32 schd_int4_unmask_flag;
    ZXIC_UINT32 schd_int3_unmask_flag;
    ZXIC_UINT32 schd_int2_unmask_flag;
    ZXIC_UINT32 schd_int1_unmask_flag;
    ZXIC_UINT32 schd_int0_unmask_flag;
}DPP_SE_KSCHD_SE_KSCHD_INT_STATUS_T;

typedef struct dpp_se_kschd_debug_cnt_mode_t
{
    ZXIC_UINT32 cnt_rd_mode;
    ZXIC_UINT32 cnt_overflow_mode;
}DPP_SE_KSCHD_DEBUG_CNT_MODE_T;

typedef struct dpp_se_kschd_se_parser_kschd_key0_3_cnt_t
{
    ZXIC_UINT32 se_parser_kschd_key0_3_cnt;
}DPP_SE_KSCHD_SE_PARSER_KSCHD_KEY0_3_CNT_T;

typedef struct dpp_se_kschd_se_smmu1_key0_3_cnt_t
{
    ZXIC_UINT32 se_smmu1_key0_3_cnt;
}DPP_SE_KSCHD_SE_SMMU1_KEY0_3_CNT_T;

typedef struct dpp_se_kschd_kschd_as_key0_cnt_t
{
    ZXIC_UINT32 kschd_as_key0_cnt;
}DPP_SE_KSCHD_KSCHD_AS_KEY0_CNT_T;

typedef struct dpp_se_kschd_kschd_as_key1_cnt_t
{
    ZXIC_UINT32 kschd_as_key1_cnt;
}DPP_SE_KSCHD_KSCHD_AS_KEY1_CNT_T;

typedef struct dpp_se_kschd_kschd_as_key2_cnt_t
{
    ZXIC_UINT32 kschd_as_key2_cnt;
}DPP_SE_KSCHD_KSCHD_AS_KEY2_CNT_T;

typedef struct dpp_se_kschd_kschd_as_key3_cnt_t
{
    ZXIC_UINT32 kschd_as_key3_cnt;
}DPP_SE_KSCHD_KSCHD_AS_KEY3_CNT_T;

typedef struct dpp_se_kschd_kschd_as_key4_cnt_t
{
    ZXIC_UINT32 kschd_as_key4_cnt;
}DPP_SE_KSCHD_KSCHD_AS_KEY4_CNT_T;

typedef struct dpp_se_kschd_kschd_as_key5_cnt_t
{
    ZXIC_UINT32 kschd_as_key5_cnt;
}DPP_SE_KSCHD_KSCHD_AS_KEY5_CNT_T;

typedef struct dpp_se_kschd_kschd_as_key6_cnt_t
{
    ZXIC_UINT32 kschd_as_key6_cnt;
}DPP_SE_KSCHD_KSCHD_AS_KEY6_CNT_T;

typedef struct dpp_se_kschd_kschd_as_key9_cnt_t
{
    ZXIC_UINT32 kschd_as_key9_cnt;
}DPP_SE_KSCHD_KSCHD_AS_KEY9_CNT_T;

typedef struct dpp_se_kschd_kschd_se_parser_fc0_3_cnt_t
{
    ZXIC_UINT32 kschd_se_parser_fc0_3_cnt;
}DPP_SE_KSCHD_KSCHD_SE_PARSER_FC0_3_CNT_T;

typedef struct dpp_se_kschd_smmu1_se_fc0_3_cnt_t
{
    ZXIC_UINT32 smmu1_se_fc0_3_cnt;
}DPP_SE_KSCHD_SMMU1_SE_FC0_3_CNT_T;

typedef struct dpp_se_kschd_as_kschd_fc_cnt0_t
{
    ZXIC_UINT32 as_kschd_fc_cnt0;
}DPP_SE_KSCHD_AS_KSCHD_FC_CNT0_T;

typedef struct dpp_se_kschd_as_kschd_fc_cnt1_t
{
    ZXIC_UINT32 as_kschd_fc_cnt1;
}DPP_SE_KSCHD_AS_KSCHD_FC_CNT1_T;

typedef struct dpp_se_kschd_as_kschd_fc_cnt2_t
{
    ZXIC_UINT32 as_kschd_fc_cnt2;
}DPP_SE_KSCHD_AS_KSCHD_FC_CNT2_T;

typedef struct dpp_se_kschd_as_kschd_fc_cnt3_t
{
    ZXIC_UINT32 as_kschd_fc_cnt3;
}DPP_SE_KSCHD_AS_KSCHD_FC_CNT3_T;

typedef struct dpp_se_kschd_as_kschd_fc_cnt4_t
{
    ZXIC_UINT32 as_kschd_fc_cnt4;
}DPP_SE_KSCHD_AS_KSCHD_FC_CNT4_T;

typedef struct dpp_se_kschd_as_kschd_fc_cnt5_t
{
    ZXIC_UINT32 as_kschd_fc_cnt5;
}DPP_SE_KSCHD_AS_KSCHD_FC_CNT5_T;

typedef struct dpp_se_kschd_as_kschd_fc_cnt6_t
{
    ZXIC_UINT32 as_kschd_fc_cnt6;
}DPP_SE_KSCHD_AS_KSCHD_FC_CNT6_T;

typedef struct dpp_se_kschd_as_kschd_fc_cnt9_t
{
    ZXIC_UINT32 as_kschd_fc_cnt9;
}DPP_SE_KSCHD_AS_KSCHD_FC_CNT9_T;

typedef struct dpp_se_rschd_rschd_hash_pful_cfg_t
{
    ZXIC_UINT32 rschd_hash_pful_cfg;
}DPP_SE_RSCHD_RSCHD_HASH_PFUL_CFG_T;

typedef struct dpp_se_rschd_rschd_hash_ept_cfg_t
{
    ZXIC_UINT32 rschd_hash_ept_cfg;
}DPP_SE_RSCHD_RSCHD_HASH_EPT_CFG_T;

typedef struct dpp_se_rschd_rschd_pbu_pful_cfg_t
{
    ZXIC_UINT32 rschd_pbu_pful_cfg;
}DPP_SE_RSCHD_RSCHD_PBU_PFUL_CFG_T;

typedef struct dpp_se_rschd_rschd_pbu_ept_cfg_t
{
    ZXIC_UINT32 rschd_pbu_ept_cfg;
}DPP_SE_RSCHD_RSCHD_PBU_EPT_CFG_T;

typedef struct dpp_se_rschd_rschd_lpm_pful_cfg_t
{
    ZXIC_UINT32 rschd_lpm_pful_cfg;
}DPP_SE_RSCHD_RSCHD_LPM_PFUL_CFG_T;

typedef struct dpp_se_rschd_rschd_lpm_ept_cfg_t
{
    ZXIC_UINT32 rschd_lpm_ept_cfg;
}DPP_SE_RSCHD_RSCHD_LPM_EPT_CFG_T;

typedef struct dpp_se_rschd_rschd_etcam_pful_cfg_t
{
    ZXIC_UINT32 rschd_etcam_pful_cfg;
}DPP_SE_RSCHD_RSCHD_ETCAM_PFUL_CFG_T;

typedef struct dpp_se_rschd_rschd_etcam_ept_cfg_t
{
    ZXIC_UINT32 rschd_etcam_ept_cfg;
}DPP_SE_RSCHD_RSCHD_ETCAM_EPT_CFG_T;

typedef struct dpp_se_rschd_smmu0_wb_pful_cfg_t
{
    ZXIC_UINT32 smmu0_wb_pful_cfg;
}DPP_SE_RSCHD_SMMU0_WB_PFUL_CFG_T;

typedef struct dpp_se_rschd_smmu0_wb_ept_cfg_t
{
    ZXIC_UINT32 smmu0_wb_ept_cfg;
}DPP_SE_RSCHD_SMMU0_WB_EPT_CFG_T;

typedef struct dpp_se_rschd_smmu1_wb_pful_cfg_t
{
    ZXIC_UINT32 smmu1_wb_pful_cfg;
}DPP_SE_RSCHD_SMMU1_WB_PFUL_CFG_T;

typedef struct dpp_se_rschd_smmu1_wb_ept_cfg_t
{
    ZXIC_UINT32 smmu1_wb_ept_cfg;
}DPP_SE_RSCHD_SMMU1_WB_EPT_CFG_T;

typedef struct dpp_se_rschd_alg_wb_pful_cfg_t
{
    ZXIC_UINT32 alg_wb_pful_cfg;
}DPP_SE_RSCHD_ALG_WB_PFUL_CFG_T;

typedef struct dpp_se_rschd_alg_wb_ept_cfg_t
{
    ZXIC_UINT32 alg_wb_ept_cfg;
}DPP_SE_RSCHD_ALG_WB_EPT_CFG_T;

typedef struct dpp_se_rschd_wr_rsp_vld_en_t
{
    ZXIC_UINT32 wr_rsp_vld_en;
}DPP_SE_RSCHD_WR_RSP_VLD_EN_T;

typedef struct dpp_se_rschd_nppu_wb_pful_cfg_t
{
    ZXIC_UINT32 nppu_wb_pful_cfg;
}DPP_SE_RSCHD_NPPU_WB_PFUL_CFG_T;

typedef struct dpp_se_rschd_nppu_wb_ept_cfg_t
{
    ZXIC_UINT32 nppu_wb_ept_cfg;
}DPP_SE_RSCHD_NPPU_WB_EPT_CFG_T;

typedef struct dpp_se_rschd_port0_int_en_t
{
    ZXIC_UINT32 port0_int_en;
}DPP_SE_RSCHD_PORT0_INT_EN_T;

typedef struct dpp_se_rschd_port0_int_mask_t
{
    ZXIC_UINT32 port0_int_mask;
}DPP_SE_RSCHD_PORT0_INT_MASK_T;

typedef struct dpp_se_rschd_port1_int_en_t
{
    ZXIC_UINT32 port1_int_en;
}DPP_SE_RSCHD_PORT1_INT_EN_T;

typedef struct dpp_se_rschd_port1_int_mask_t
{
    ZXIC_UINT32 port1_int_mask;
}DPP_SE_RSCHD_PORT1_INT_MASK_T;

typedef struct dpp_se_rschd_port0_int_status_t
{
    ZXIC_UINT32 port0_int_status;
}DPP_SE_RSCHD_PORT0_INT_STATUS_T;

typedef struct dpp_se_rschd_port1_int_status_t
{
    ZXIC_UINT32 port1_int_status;
}DPP_SE_RSCHD_PORT1_INT_STATUS_T;

typedef struct dpp_se_rschd_se_rschd_int_status_t
{
    ZXIC_UINT32 port1_int_unmask_flag;
    ZXIC_UINT32 port0_int_unmask_flag;
}DPP_SE_RSCHD_SE_RSCHD_INT_STATUS_T;

typedef struct dpp_se_rschd_debug_cnt_mode_t
{
    ZXIC_UINT32 cnt_rd_mode;
    ZXIC_UINT32 cnt_overflow_mode;
}DPP_SE_RSCHD_DEBUG_CNT_MODE_T;

typedef struct dpp_se_rschd_se_ppu_mex0_5_rsp1_cnt_t
{
    ZXIC_UINT32 se_ppu_mex0_5_rsp1_cnt;
}DPP_SE_RSCHD_SE_PPU_MEX0_5_RSP1_CNT_T;

typedef struct dpp_se_rschd_as_rschd_rsp0_cnt_t
{
    ZXIC_UINT32 as_rschd_rsp0_cnt;
}DPP_SE_RSCHD_AS_RSCHD_RSP0_CNT_T;

typedef struct dpp_se_rschd_as_rschd_rsp1_cnt_t
{
    ZXIC_UINT32 as_rschd_rsp1_cnt;
}DPP_SE_RSCHD_AS_RSCHD_RSP1_CNT_T;

typedef struct dpp_se_rschd_as_rschd_rsp2_cnt_t
{
    ZXIC_UINT32 as_rschd_rsp2_cnt;
}DPP_SE_RSCHD_AS_RSCHD_RSP2_CNT_T;

typedef struct dpp_se_rschd_as_rschd_rsp3_cnt_t
{
    ZXIC_UINT32 as_rschd_rsp3_cnt;
}DPP_SE_RSCHD_AS_RSCHD_RSP3_CNT_T;

typedef struct dpp_se_rschd_as_rschd_rsp4_cnt_t
{
    ZXIC_UINT32 as_rschd_rsp4_cnt;
}DPP_SE_RSCHD_AS_RSCHD_RSP4_CNT_T;

typedef struct dpp_se_rschd_as_rschd_rsp5_cnt_t
{
    ZXIC_UINT32 as_rschd_rsp5_cnt;
}DPP_SE_RSCHD_AS_RSCHD_RSP5_CNT_T;

typedef struct dpp_se_rschd_as_rschd_rsp6_cnt_t
{
    ZXIC_UINT32 as_rschd_rsp6_cnt;
}DPP_SE_RSCHD_AS_RSCHD_RSP6_CNT_T;

typedef struct dpp_se_rschd_as_rschd_rsp9_cnt_t
{
    ZXIC_UINT32 as_rschd_rsp9_cnt;
}DPP_SE_RSCHD_AS_RSCHD_RSP9_CNT_T;

typedef struct dpp_se_rschd_smmu1_se_rsp0_3_cnt_t
{
    ZXIC_UINT32 smmu1_se_rsp0_3_cnt;
}DPP_SE_RSCHD_SMMU1_SE_RSP0_3_CNT_T;

typedef struct dpp_se_rschd_ppu_se_mex0_3_fc_cnt_t
{
    ZXIC_UINT32 ppu_se_mex0_3_fc_cnt;
}DPP_SE_RSCHD_PPU_SE_MEX0_3_FC_CNT_T;

typedef struct dpp_se_rschd_rschd_as_fc_cnt0_t
{
    ZXIC_UINT32 rschd_as_fc_cnt0;
}DPP_SE_RSCHD_RSCHD_AS_FC_CNT0_T;

typedef struct dpp_se_rschd_rschd_as_fc_cnt1_t
{
    ZXIC_UINT32 rschd_as_fc_cnt1;
}DPP_SE_RSCHD_RSCHD_AS_FC_CNT1_T;

typedef struct dpp_se_rschd_rschd_as_fc_cnt2_t
{
    ZXIC_UINT32 rschd_as_fc_cnt2;
}DPP_SE_RSCHD_RSCHD_AS_FC_CNT2_T;

typedef struct dpp_se_rschd_rschd_as_fc_cnt3_t
{
    ZXIC_UINT32 rschd_as_fc_cnt3;
}DPP_SE_RSCHD_RSCHD_AS_FC_CNT3_T;

typedef struct dpp_se_rschd_rschd_as_fc_cnt4_t
{
    ZXIC_UINT32 rschd_as_fc_cnt4;
}DPP_SE_RSCHD_RSCHD_AS_FC_CNT4_T;

typedef struct dpp_se_rschd_rschd_as_fc_cnt5_t
{
    ZXIC_UINT32 rschd_as_fc_cnt5;
}DPP_SE_RSCHD_RSCHD_AS_FC_CNT5_T;

typedef struct dpp_se_rschd_rschd_as_fc_cnt6_t
{
    ZXIC_UINT32 rschd_as_fc_cnt6;
}DPP_SE_RSCHD_RSCHD_AS_FC_CNT6_T;

typedef struct dpp_se_rschd_rschd_as_fc_cnt9_t
{
    ZXIC_UINT32 rschd_as_fc_cnt9;
}DPP_SE_RSCHD_RSCHD_AS_FC_CNT9_T;

typedef struct dpp_se_rschd_se_smmu1_fc0_3_cnt_t
{
    ZXIC_UINT32 se_smmu1_fc0_3_cnt;
}DPP_SE_RSCHD_SE_SMMU1_FC0_3_CNT_T;

typedef struct dpp_se_rschd_smmu0_se_wr_done_cnt_t
{
    ZXIC_UINT32 smmu0_se_wr_done_cnt;
}DPP_SE_RSCHD_SMMU0_SE_WR_DONE_CNT_T;

typedef struct dpp_se_rschd_se_smmu0_wr_done_fc_cnt_t
{
    ZXIC_UINT32 se_smmu0_wr_done_fc_cnt;
}DPP_SE_RSCHD_SE_SMMU0_WR_DONE_FC_CNT_T;

typedef struct dpp_se_rschd_smmu1_se_wr_rsp_cnt_t
{
    ZXIC_UINT32 smmu1_se_wr_rsp_cnt;
}DPP_SE_RSCHD_SMMU1_SE_WR_RSP_CNT_T;

typedef struct dpp_se_rschd_se_smmu1_wr_rsp_fc_cnt_t
{
    ZXIC_UINT32 se_smmu1_wr_rsp_fc_cnt;
}DPP_SE_RSCHD_SE_SMMU1_WR_RSP_FC_CNT_T;

typedef struct dpp_se_rschd_alg_se_wr_rsp_cnt_t
{
    ZXIC_UINT32 alg_se_wr_rsp_cnt;
}DPP_SE_RSCHD_ALG_SE_WR_RSP_CNT_T;

typedef struct dpp_se_rschd_se_alg_wr_rsp_fc_cnt_t
{
    ZXIC_UINT32 se_alg_wr_rsp_fc_cnt;
}DPP_SE_RSCHD_SE_ALG_WR_RSP_FC_CNT_T;


#ifdef __cplusplus
}
#endif
#endif

