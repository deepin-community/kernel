
#ifndef _DPP_ETM_REG_H_
#define _DPP_ETM_REG_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef struct dpp_etm_cfgmt_cpu_check_reg_t
{
    ZXIC_UINT32 cpu_check_reg;
}DPP_ETM_CFGMT_CPU_CHECK_REG_T;

typedef struct dpp_etm_cfgmt_cfgmt_blksize_t
{
    ZXIC_UINT32 cfgmt_blksize;
}DPP_ETM_CFGMT_CFGMT_BLKSIZE_T;

typedef struct dpp_etm_cfgmt_reg_int_state_reg_t
{
    ZXIC_UINT32 shap_int;
    ZXIC_UINT32 crdt_int;
    ZXIC_UINT32 mmu_int;
    ZXIC_UINT32 qmu_int;
    ZXIC_UINT32 cgavd_int;
    ZXIC_UINT32 olif_int;
    ZXIC_UINT32 cfgmt_int_buf;
}DPP_ETM_CFGMT_REG_INT_STATE_REG_T;

typedef struct dpp_etm_cfgmt_reg_int_mask_reg_t
{
    ZXIC_UINT32 shap_int_mask;
    ZXIC_UINT32 crdt_int_mask;
    ZXIC_UINT32 tmmu_int_mask;
    ZXIC_UINT32 qmu_int_mask;
    ZXIC_UINT32 cgavd_int_mask;
    ZXIC_UINT32 olif_int_mask;
    ZXIC_UINT32 cfgmt_int_buf_mask;
}DPP_ETM_CFGMT_REG_INT_MASK_REG_T;

typedef struct dpp_etm_cfgmt_timeout_limit_t
{
    ZXIC_UINT32 timeout_limit;
}DPP_ETM_CFGMT_TIMEOUT_LIMIT_T;

typedef struct dpp_etm_cfgmt_subsystem_rdy_reg_t
{
    ZXIC_UINT32 olif_rdy;
    ZXIC_UINT32 qmu_rdy;
    ZXIC_UINT32 cgavd_rdy;
    ZXIC_UINT32 tmmu_rdy;
    ZXIC_UINT32 shap_rdy;
    ZXIC_UINT32 crdt_rdy;
}DPP_ETM_CFGMT_SUBSYSTEM_RDY_REG_T;

typedef struct dpp_etm_cfgmt_subsystem_en_reg_t
{
    ZXIC_UINT32 subsystem_en_buf_31_28;
    ZXIC_UINT32 subsystem_en_buf_25_0;
}DPP_ETM_CFGMT_SUBSYSTEM_EN_REG_T;

typedef struct dpp_etm_cfgmt_cfgmt_int_reg_t
{
    ZXIC_UINT32 cfgmt_int_buf;
}DPP_ETM_CFGMT_CFGMT_INT_REG_T;

typedef struct dpp_etm_cfgmt_qmu_work_mode_t
{
    ZXIC_UINT32 qmu_work_mode;
}DPP_ETM_CFGMT_QMU_WORK_MODE_T;

typedef struct dpp_etm_cfgmt_cfgmt_ddr_attach_t
{
    ZXIC_UINT32 cfgmt_ddr_attach;
}DPP_ETM_CFGMT_CFGMT_DDR_ATTACH_T;

typedef struct dpp_etm_cfgmt_cnt_mode_reg_t
{
    ZXIC_UINT32 cfgmt_fc_count_mode;
    ZXIC_UINT32 cfgmt_count_rd_mode;
    ZXIC_UINT32 cfgmt_count_overflow_mode;
}DPP_ETM_CFGMT_CNT_MODE_REG_T;

typedef struct dpp_etm_cfgmt_clkgate_en_t
{
    ZXIC_UINT32 clkgate_en;
}DPP_ETM_CFGMT_CLKGATE_EN_T;

typedef struct dpp_etm_cfgmt_softrst_en_t
{
    ZXIC_UINT32 softrst_en;
}DPP_ETM_CFGMT_SOFTRST_EN_T;

typedef struct dpp_etm_olif_imem_prog_full_t
{
    ZXIC_UINT32 imem_prog_full_assert;
    ZXIC_UINT32 imem_prog_full_negate;
}DPP_ETM_OLIF_IMEM_PROG_FULL_T;

typedef struct dpp_etm_olif_qmu_para_prog_full_t
{
    ZXIC_UINT32 qmu_para_prog_full_assert;
    ZXIC_UINT32 qmu_para_prog_full_negate;
}DPP_ETM_OLIF_QMU_PARA_PROG_FULL_T;

typedef struct dpp_etm_olif_olif_int_mask_t
{
    ZXIC_UINT32 emem_dat_sop_err_mask;
    ZXIC_UINT32 emem_dat_eop_err_mask;
    ZXIC_UINT32 imem_dat_sop_err_mask;
    ZXIC_UINT32 imem_dat_eop_err_mask;
    ZXIC_UINT32 crcram_parity_err_mask;
    ZXIC_UINT32 emem_fifo_ecc_mask;
    ZXIC_UINT32 imem_fifo_ecc_mask;
    ZXIC_UINT32 emem_fifo_ovf_mask;
    ZXIC_UINT32 emem_fifo_udf_mask;
    ZXIC_UINT32 imem_fifo_ovf_mask;
    ZXIC_UINT32 imem_fifo_udf_mask;
    ZXIC_UINT32 para_fifo_ecc_mask;
    ZXIC_UINT32 para_fifo_ovf_mask;
    ZXIC_UINT32 para_fifo_udf_mask;
    ZXIC_UINT32 itmh_ecc_single_err_mask;
    ZXIC_UINT32 itmh_ecc_double_err_mask;
    ZXIC_UINT32 order_fifo_parity_err_mask;
    ZXIC_UINT32 order_fifo_ovf_mask;
    ZXIC_UINT32 order_fifo_udf_mask;
}DPP_ETM_OLIF_OLIF_INT_MASK_T;

typedef struct dpp_etm_olif_itmhram_parity_err_2_int_t
{
    ZXIC_UINT32 emem_dat_sop_err;
    ZXIC_UINT32 emem_dat_eop_err;
    ZXIC_UINT32 imem_dat_sop_err;
    ZXIC_UINT32 imem_dat_eop_err;
    ZXIC_UINT32 crcram_parity_err_1_int;
    ZXIC_UINT32 emem_fifo_ecc_single_err_int;
    ZXIC_UINT32 emem_fifo_ecc_double_err_int;
    ZXIC_UINT32 imem_fifo_ecc_single_err_int;
    ZXIC_UINT32 imem_fifo_ecc_double_err_int;
    ZXIC_UINT32 emem_fifo_ovf_int;
    ZXIC_UINT32 emem_fifo_udf_int;
    ZXIC_UINT32 imem_fifo_ovf_int;
    ZXIC_UINT32 imem_fifo_udf_int;
    ZXIC_UINT32 para_fifo_ecc_single_err_int;
    ZXIC_UINT32 para_fifo_ecc_double_err_int;
    ZXIC_UINT32 para_fifo_ovf_int;
    ZXIC_UINT32 para_fifo_udf_int;
    ZXIC_UINT32 itmh_ecc_single_err_int;
    ZXIC_UINT32 itmh_ecc_double_err_int;
    ZXIC_UINT32 order_fifo_parity_err_int;
    ZXIC_UINT32 order_fifo_ovf_int;
    ZXIC_UINT32 order_fifo_udf_int;
}DPP_ETM_OLIF_ITMHRAM_PARITY_ERR_2_INT_T;

typedef struct dpp_etm_olif_lif0_port_rdy_mask_h_t
{
    ZXIC_UINT32 lif0_port_rdy_mask_h;
}DPP_ETM_OLIF_LIF0_PORT_RDY_MASK_H_T;

typedef struct dpp_etm_olif_lif0_port_rdy_mask_l_t
{
    ZXIC_UINT32 lif0_port_rdy_mask_l;
}DPP_ETM_OLIF_LIF0_PORT_RDY_MASK_L_T;

typedef struct dpp_etm_olif_lif0_port_rdy_cfg_h_t
{
    ZXIC_UINT32 lif0_port_rdy_cfg_h;
}DPP_ETM_OLIF_LIF0_PORT_RDY_CFG_H_T;

typedef struct dpp_etm_olif_lif0_port_rdy_cfg_l_t
{
    ZXIC_UINT32 lif0_port_rdy_cfg_l;
}DPP_ETM_OLIF_LIF0_PORT_RDY_CFG_L_T;

typedef struct dpp_etm_olif_lif0_link_rdy_mask_cfg_t
{
    ZXIC_UINT32 lif0_link_rdy_mask;
    ZXIC_UINT32 lif0_link_rdy_cfg;
}DPP_ETM_OLIF_LIF0_LINK_RDY_MASK_CFG_T;

typedef struct dpp_etm_olif_tm_lif_stat_cfg_t
{
    ZXIC_UINT32 all_or_by_port;
    ZXIC_UINT32 i_or_e_sel;
    ZXIC_UINT32 port_or_dest_id_sel;
    ZXIC_UINT32 port_dest_id;
}DPP_ETM_OLIF_TM_LIF_STAT_CFG_T;

typedef struct dpp_etm_olif_tm_lif_sop_stat_t
{
    ZXIC_UINT32 tm_lif_sop_stat;
}DPP_ETM_OLIF_TM_LIF_SOP_STAT_T;

typedef struct dpp_etm_olif_tm_lif_eop_stat_t
{
    ZXIC_UINT32 tm_lif_eop_stat;
}DPP_ETM_OLIF_TM_LIF_EOP_STAT_T;

typedef struct dpp_etm_olif_tm_lif_vld_stat_t
{
    ZXIC_UINT32 tm_lif_vld_stat;
}DPP_ETM_OLIF_TM_LIF_VLD_STAT_T;

typedef struct dpp_etm_cgavd_prog_full_assert_cfg_t
{
    ZXIC_UINT32 prog_full_assert_cfg;
    ZXIC_UINT32 prog_full_negate_cfg;
}DPP_ETM_CGAVD_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_etm_cgavd_cgavd_int_t
{
    ZXIC_UINT32 cgavd_int;
}DPP_ETM_CGAVD_CGAVD_INT_T;

typedef struct dpp_etm_cgavd_cgavd_ram_err_t
{
    ZXIC_UINT32 flow_qnum_intb;
    ZXIC_UINT32 flow_qnum_inta;
    ZXIC_UINT32 pp_qlen_inta;
    ZXIC_UINT32 pp_qlen_intb;
    ZXIC_UINT32 pp_tdth_int;
    ZXIC_UINT32 flow_tdth_inta;
    ZXIC_UINT32 flow_tdth_intb;
    ZXIC_UINT32 flow_qlen_inta;
    ZXIC_UINT32 flow_qlen_intb;
    ZXIC_UINT32 qmu_cgavd_fifo_uv_int;
    ZXIC_UINT32 qmu_cgavd_fifo_ov_int;
    ZXIC_UINT32 pds_deal_fifo_ov_int;
    ZXIC_UINT32 pds_deal_fifo_uv_int;
}DPP_ETM_CGAVD_CGAVD_RAM_ERR_T;

typedef struct dpp_etm_cgavd_cgavd_int_mask_t
{
    ZXIC_UINT32 cgavd_int_mask;
}DPP_ETM_CGAVD_CGAVD_INT_MASK_T;

typedef struct dpp_etm_cgavd_cgavd_ram_err_int_mask_t
{
    ZXIC_UINT32 flow_qnum_inta_mask;
    ZXIC_UINT32 flow_qnum_intb_mask;
    ZXIC_UINT32 pp_qlen_inta_mask;
    ZXIC_UINT32 pp_qlen_intb_mask;
    ZXIC_UINT32 pp_tdth_int_mask;
    ZXIC_UINT32 flow_tdth_inta_mask;
    ZXIC_UINT32 flow_tdth_intb_mask;
    ZXIC_UINT32 flow_qlen_inta_mask;
    ZXIC_UINT32 flow_qlen_intb_mask;
    ZXIC_UINT32 qmu_cgavd_fifo_uv_int_mask;
    ZXIC_UINT32 qmu_cgavd_fifo_ov_int_mask;
    ZXIC_UINT32 pds_deal_fifo_ov_int_mask;
    ZXIC_UINT32 pds_deal_fifo_uv_int_mask;
}DPP_ETM_CGAVD_CGAVD_RAM_ERR_INT_MASK_T;

typedef struct dpp_etm_cgavd_cfgmt_byte_mode_t
{
    ZXIC_UINT32 cfgmt_byte_mode;
}DPP_ETM_CGAVD_CFGMT_BYTE_MODE_T;

typedef struct dpp_etm_cgavd_avg_qlen_return_zero_en_t
{
    ZXIC_UINT32 avg_qlen_return_zero_en;
}DPP_ETM_CGAVD_AVG_QLEN_RETURN_ZERO_EN_T;

typedef struct dpp_etm_cgavd_flow_wred_q_len_th_t
{
    ZXIC_UINT32 flow_wred_q_len_th;
}DPP_ETM_CGAVD_FLOW_WRED_Q_LEN_TH_T;

typedef struct dpp_etm_cgavd_flow_wq_t
{
    ZXIC_UINT32 wq_flow;
}DPP_ETM_CGAVD_FLOW_WQ_T;

typedef struct dpp_etm_cgavd_flow_wred_max_th_t
{
    ZXIC_UINT32 flow_wred_max_th;
}DPP_ETM_CGAVD_FLOW_WRED_MAX_TH_T;

typedef struct dpp_etm_cgavd_flow_wred_min_th_t
{
    ZXIC_UINT32 flow_wred_min_th;
}DPP_ETM_CGAVD_FLOW_WRED_MIN_TH_T;

typedef struct dpp_etm_cgavd_flow_wred_cfg_para_t
{
    ZXIC_UINT32 flow_wred_cfg_para;
}DPP_ETM_CGAVD_FLOW_WRED_CFG_PARA_T;

typedef struct dpp_etm_cgavd_pp_avg_q_len_t
{
    ZXIC_UINT32 pp_avg_q_len;
}DPP_ETM_CGAVD_PP_AVG_Q_LEN_T;

typedef struct dpp_etm_cgavd_pp_td_th_t
{
    ZXIC_UINT32 pp_td_th;
}DPP_ETM_CGAVD_PP_TD_TH_T;

typedef struct dpp_etm_cgavd_pp_ca_mtd_t
{
    ZXIC_UINT32 pp_ca_mtd;
}DPP_ETM_CGAVD_PP_CA_MTD_T;

typedef struct dpp_etm_cgavd_pp_wred_grp_th_en_t
{
    ZXIC_UINT32 pp_wred_grp;
    ZXIC_UINT32 pp_wred_grp_th_en;
}DPP_ETM_CGAVD_PP_WRED_GRP_TH_EN_T;

typedef struct dpp_etm_cgavd_pp_wred_q_len_th_t
{
    ZXIC_UINT32 pp_wred_q_len_th;
}DPP_ETM_CGAVD_PP_WRED_Q_LEN_TH_T;

typedef struct dpp_etm_cgavd_pp_wq_t
{
    ZXIC_UINT32 wq_pp;
}DPP_ETM_CGAVD_PP_WQ_T;

typedef struct dpp_etm_cgavd_pp_wred_max_th_t
{
    ZXIC_UINT32 pp_wred_max_th;
}DPP_ETM_CGAVD_PP_WRED_MAX_TH_T;

typedef struct dpp_etm_cgavd_pp_wred_min_th_t
{
    ZXIC_UINT32 pp_wred_min_th;
}DPP_ETM_CGAVD_PP_WRED_MIN_TH_T;

typedef struct dpp_etm_cgavd_pp_cfg_para_t
{
    ZXIC_UINT32 pp_cfg_para;
}DPP_ETM_CGAVD_PP_CFG_PARA_T;

typedef struct dpp_etm_cgavd_sys_avg_q_len_t
{
    ZXIC_UINT32 sys_avg_q_len;
}DPP_ETM_CGAVD_SYS_AVG_Q_LEN_T;

typedef struct dpp_etm_cgavd_sys_td_th_t
{
    ZXIC_UINT32 sys_td_th;
}DPP_ETM_CGAVD_SYS_TD_TH_T;

typedef struct dpp_etm_cgavd_sys_cgavd_metd_t
{
    ZXIC_UINT32 sys_cgavd_metd;
}DPP_ETM_CGAVD_SYS_CGAVD_METD_T;

typedef struct dpp_etm_cgavd_sys_cfg_q_grp_para_t
{
    ZXIC_UINT32 gred_q_len_th_sys;
}DPP_ETM_CGAVD_SYS_CFG_Q_GRP_PARA_T;

typedef struct dpp_etm_cgavd_sys_wq_t
{
    ZXIC_UINT32 wq_sys;
}DPP_ETM_CGAVD_SYS_WQ_T;

typedef struct dpp_etm_cgavd_gred_max_th_t
{
    ZXIC_UINT32 gred_max_th;
}DPP_ETM_CGAVD_GRED_MAX_TH_T;

typedef struct dpp_etm_cgavd_gred_mid_th_t
{
    ZXIC_UINT32 gred_mid_th;
}DPP_ETM_CGAVD_GRED_MID_TH_T;

typedef struct dpp_etm_cgavd_gred_min_th_t
{
    ZXIC_UINT32 gred_min_th;
}DPP_ETM_CGAVD_GRED_MIN_TH_T;

typedef struct dpp_etm_cgavd_gred_cfg_para0_t
{
    ZXIC_UINT32 gred_cfg_para0;
}DPP_ETM_CGAVD_GRED_CFG_PARA0_T;

typedef struct dpp_etm_cgavd_gred_cfg_para1_t
{
    ZXIC_UINT32 gred_cfg_para1;
}DPP_ETM_CGAVD_GRED_CFG_PARA1_T;

typedef struct dpp_etm_cgavd_gred_cfg_para2_t
{
    ZXIC_UINT32 gred_cfg_para2;
}DPP_ETM_CGAVD_GRED_CFG_PARA2_T;

typedef struct dpp_etm_cgavd_sys_window_th_h_t
{
    ZXIC_UINT32 sys_window_th_h;
}DPP_ETM_CGAVD_SYS_WINDOW_TH_H_T;

typedef struct dpp_etm_cgavd_sys_window_th_l_t
{
    ZXIC_UINT32 sys_window_th_l;
}DPP_ETM_CGAVD_SYS_WINDOW_TH_L_T;

typedef struct dpp_etm_cgavd_amplify_gene0_t
{
    ZXIC_UINT32 amplify_gene0;
}DPP_ETM_CGAVD_AMPLIFY_GENE0_T;

typedef struct dpp_etm_cgavd_amplify_gene1_t
{
    ZXIC_UINT32 amplify_gene1;
}DPP_ETM_CGAVD_AMPLIFY_GENE1_T;

typedef struct dpp_etm_cgavd_amplify_gene2_t
{
    ZXIC_UINT32 amplify_gene2;
}DPP_ETM_CGAVD_AMPLIFY_GENE2_T;

typedef struct dpp_etm_cgavd_amplify_gene3_t
{
    ZXIC_UINT32 amplify_gene3;
}DPP_ETM_CGAVD_AMPLIFY_GENE3_T;

typedef struct dpp_etm_cgavd_amplify_gene4_t
{
    ZXIC_UINT32 amplify_gene4;
}DPP_ETM_CGAVD_AMPLIFY_GENE4_T;

typedef struct dpp_etm_cgavd_amplify_gene5_t
{
    ZXIC_UINT32 amplify_gene5;
}DPP_ETM_CGAVD_AMPLIFY_GENE5_T;

typedef struct dpp_etm_cgavd_amplify_gene6_t
{
    ZXIC_UINT32 amplify_gene6;
}DPP_ETM_CGAVD_AMPLIFY_GENE6_T;

typedef struct dpp_etm_cgavd_amplify_gene7_t
{
    ZXIC_UINT32 amplify_gene7;
}DPP_ETM_CGAVD_AMPLIFY_GENE7_T;

typedef struct dpp_etm_cgavd_amplify_gene8_t
{
    ZXIC_UINT32 amplify_gene8;
}DPP_ETM_CGAVD_AMPLIFY_GENE8_T;

typedef struct dpp_etm_cgavd_amplify_gene9_t
{
    ZXIC_UINT32 amplify_gene9;
}DPP_ETM_CGAVD_AMPLIFY_GENE9_T;

typedef struct dpp_etm_cgavd_amplify_gene10_t
{
    ZXIC_UINT32 amplify_gene10;
}DPP_ETM_CGAVD_AMPLIFY_GENE10_T;

typedef struct dpp_etm_cgavd_amplify_gene11_t
{
    ZXIC_UINT32 amplify_gene11;
}DPP_ETM_CGAVD_AMPLIFY_GENE11_T;

typedef struct dpp_etm_cgavd_amplify_gene12_t
{
    ZXIC_UINT32 amplify_gene12;
}DPP_ETM_CGAVD_AMPLIFY_GENE12_T;

typedef struct dpp_etm_cgavd_amplify_gene13_t
{
    ZXIC_UINT32 amplify_gene13;
}DPP_ETM_CGAVD_AMPLIFY_GENE13_T;

typedef struct dpp_etm_cgavd_amplify_gene14_t
{
    ZXIC_UINT32 amplify_gene14;
}DPP_ETM_CGAVD_AMPLIFY_GENE14_T;

typedef struct dpp_etm_cgavd_amplify_gene15_t
{
    ZXIC_UINT32 amplify_gene15;
}DPP_ETM_CGAVD_AMPLIFY_GENE15_T;

typedef struct dpp_etm_cgavd_equal_pkt_len_en_t
{
    ZXIC_UINT32 equal_pkt_len_en;
}DPP_ETM_CGAVD_EQUAL_PKT_LEN_EN_T;

typedef struct dpp_etm_cgavd_equal_pkt_len_th0_t
{
    ZXIC_UINT32 equal_pkt_len_th0;
}DPP_ETM_CGAVD_EQUAL_PKT_LEN_TH0_T;

typedef struct dpp_etm_cgavd_equal_pkt_len_th1_t
{
    ZXIC_UINT32 equal_pkt_len_th1;
}DPP_ETM_CGAVD_EQUAL_PKT_LEN_TH1_T;

typedef struct dpp_etm_cgavd_equal_pkt_len_th2_t
{
    ZXIC_UINT32 equal_pkt_len_th2;
}DPP_ETM_CGAVD_EQUAL_PKT_LEN_TH2_T;

typedef struct dpp_etm_cgavd_equal_pkt_len_th3_t
{
    ZXIC_UINT32 equal_pkt_len_th3;
}DPP_ETM_CGAVD_EQUAL_PKT_LEN_TH3_T;

typedef struct dpp_etm_cgavd_equal_pkt_len_th4_t
{
    ZXIC_UINT32 equal_pkt_len_th4;
}DPP_ETM_CGAVD_EQUAL_PKT_LEN_TH4_T;

typedef struct dpp_etm_cgavd_equal_pkt_len_th5_t
{
    ZXIC_UINT32 equal_pkt_len_th5;
}DPP_ETM_CGAVD_EQUAL_PKT_LEN_TH5_T;

typedef struct dpp_etm_cgavd_equal_pkt_len_th6_t
{
    ZXIC_UINT32 equal_pkt_len_th6;
}DPP_ETM_CGAVD_EQUAL_PKT_LEN_TH6_T;

typedef struct dpp_etm_cgavd_equal_pkt_len0_t
{
    ZXIC_UINT32 equal_pkt_len0;
}DPP_ETM_CGAVD_EQUAL_PKT_LEN0_T;

typedef struct dpp_etm_cgavd_equal_pkt_len1_t
{
    ZXIC_UINT32 equal_pkt_len1;
}DPP_ETM_CGAVD_EQUAL_PKT_LEN1_T;

typedef struct dpp_etm_cgavd_equal_pkt_len2_t
{
    ZXIC_UINT32 equal_pkt_len2;
}DPP_ETM_CGAVD_EQUAL_PKT_LEN2_T;

typedef struct dpp_etm_cgavd_equal_pkt_len3_t
{
    ZXIC_UINT32 equal_pkt_len3;
}DPP_ETM_CGAVD_EQUAL_PKT_LEN3_T;

typedef struct dpp_etm_cgavd_equal_pkt_len4_t
{
    ZXIC_UINT32 equal_pkt_len4;
}DPP_ETM_CGAVD_EQUAL_PKT_LEN4_T;

typedef struct dpp_etm_cgavd_equal_pkt_len5_t
{
    ZXIC_UINT32 equal_pkt_len5;
}DPP_ETM_CGAVD_EQUAL_PKT_LEN5_T;

typedef struct dpp_etm_cgavd_equal_pkt_len6_t
{
    ZXIC_UINT32 equal_pkt_len6;
}DPP_ETM_CGAVD_EQUAL_PKT_LEN6_T;

typedef struct dpp_etm_cgavd_equal_pkt_len7_t
{
    ZXIC_UINT32 equal_pkt_len7;
}DPP_ETM_CGAVD_EQUAL_PKT_LEN7_T;

typedef struct dpp_etm_cgavd_flow_cpu_set_avg_len_t
{
    ZXIC_UINT32 flow_cpu_set_avg_len;
}DPP_ETM_CGAVD_FLOW_CPU_SET_AVG_LEN_T;

typedef struct dpp_etm_cgavd_flow_cpu_set_q_len_t
{
    ZXIC_UINT32 flow_cpu_set_q_len;
}DPP_ETM_CGAVD_FLOW_CPU_SET_Q_LEN_T;

typedef struct dpp_etm_cgavd_pp_cpu_set_avg_q_len_t
{
    ZXIC_UINT32 pp_cpu_set_avg_q_len;
}DPP_ETM_CGAVD_PP_CPU_SET_AVG_Q_LEN_T;

typedef struct dpp_etm_cgavd_pp_cpu_set_q_len_t
{
    ZXIC_UINT32 pp_cpu_set_q_len;
}DPP_ETM_CGAVD_PP_CPU_SET_Q_LEN_T;

typedef struct dpp_etm_cgavd_sys_cpu_set_avg_len_t
{
    ZXIC_UINT32 sys_cpu_set_avg_len;
}DPP_ETM_CGAVD_SYS_CPU_SET_AVG_LEN_T;

typedef struct dpp_etm_cgavd_sys_cpu_set_q_len_t
{
    ZXIC_UINT32 sys_cpu_set_q_len;
}DPP_ETM_CGAVD_SYS_CPU_SET_Q_LEN_T;

typedef struct dpp_etm_cgavd_pke_len_calc_sign_t
{
    ZXIC_UINT32 pke_len_calc_sign;
}DPP_ETM_CGAVD_PKE_LEN_CALC_SIGN_T;

typedef struct dpp_etm_cgavd_rd_cpu_or_ram_t
{
    ZXIC_UINT32 cpu_sel_sys_q_len_en;
    ZXIC_UINT32 cpu_sel_sys_avg_q_len_en;
    ZXIC_UINT32 cpu_sel_pp_q_len_en;
    ZXIC_UINT32 cpu_sel_pp_avg_q_len_en;
    ZXIC_UINT32 cpu_sel_flow_q_len_en;
    ZXIC_UINT32 cpu_sel_flow_avg_q_len_en;
}DPP_ETM_CGAVD_RD_CPU_OR_RAM_T;

typedef struct dpp_etm_cgavd_q_len_update_disable_t
{
    ZXIC_UINT32 q_len_sys_update_en;
    ZXIC_UINT32 q_len_pp_update_en;
    ZXIC_UINT32 q_len_flow_update_en;
}DPP_ETM_CGAVD_Q_LEN_UPDATE_DISABLE_T;

typedef struct dpp_etm_cgavd_cgavd_dp_sel_t
{
    ZXIC_UINT32 flow_dp_sel_high;
    ZXIC_UINT32 flow_dp_sel_mid;
    ZXIC_UINT32 flow_dp_sel_low;
    ZXIC_UINT32 pp_dp_sel_high;
    ZXIC_UINT32 pp_dp_sel_mid;
    ZXIC_UINT32 pp_dp_sel_low;
    ZXIC_UINT32 sys_dp_sel_high;
    ZXIC_UINT32 sys_dp_sel_mid;
    ZXIC_UINT32 sys_dp_sel_low;
}DPP_ETM_CGAVD_CGAVD_DP_SEL_T;

typedef struct dpp_etm_cgavd_cgavd_sub_en_t
{
    ZXIC_UINT32 cgavd_sa_sub_en;
    ZXIC_UINT32 cgavd_sys_sub_en;
    ZXIC_UINT32 cgavd_pp_sub_en;
    ZXIC_UINT32 cgavd_flow_sub_en;
}DPP_ETM_CGAVD_CGAVD_SUB_EN_T;

typedef struct dpp_etm_cgavd_default_start_queue_t
{
    ZXIC_UINT32 default_start_queue;
}DPP_ETM_CGAVD_DEFAULT_START_QUEUE_T;

typedef struct dpp_etm_cgavd_default_finish_queue_t
{
    ZXIC_UINT32 default_finish_queue;
}DPP_ETM_CGAVD_DEFAULT_FINISH_QUEUE_T;

typedef struct dpp_etm_cgavd_protocol_start_queue_t
{
    ZXIC_UINT32 protocol_start_queue;
}DPP_ETM_CGAVD_PROTOCOL_START_QUEUE_T;

typedef struct dpp_etm_cgavd_protocol_finish_queue_t
{
    ZXIC_UINT32 protocol_finish_queue;
}DPP_ETM_CGAVD_PROTOCOL_FINISH_QUEUE_T;

typedef struct dpp_etm_cgavd_uniform_td_th_t
{
    ZXIC_UINT32 uniform_td_th;
}DPP_ETM_CGAVD_UNIFORM_TD_TH_T;

typedef struct dpp_etm_cgavd_uniform_td_th_en_t
{
    ZXIC_UINT32 uniform_td_th_en;
}DPP_ETM_CGAVD_UNIFORM_TD_TH_EN_T;

typedef struct dpp_etm_cgavd_cgavd_cfg_fc_t
{
    ZXIC_UINT32 cgavd_cfg_fc;
}DPP_ETM_CGAVD_CGAVD_CFG_FC_T;

typedef struct dpp_etm_cgavd_cgavd_cfg_no_fc_t
{
    ZXIC_UINT32 cgavd_cfg_no_fc;
}DPP_ETM_CGAVD_CGAVD_CFG_NO_FC_T;

typedef struct dpp_etm_cgavd_cgavd_force_imem_omem_t
{
    ZXIC_UINT32 imem_omem_force_en;
    ZXIC_UINT32 choose_imem_omem;
}DPP_ETM_CGAVD_CGAVD_FORCE_IMEM_OMEM_T;

typedef struct dpp_etm_cgavd_cgavd_sys_q_len_l_t
{
    ZXIC_UINT32 cgavd_sys_q_len_l;
}DPP_ETM_CGAVD_CGAVD_SYS_Q_LEN_L_T;

typedef struct dpp_etm_cgavd_default_queue_en_t
{
    ZXIC_UINT32 default_queue_en;
}DPP_ETM_CGAVD_DEFAULT_QUEUE_EN_T;

typedef struct dpp_etm_cgavd_protocol_queue_en_t
{
    ZXIC_UINT32 protocol_queue_en;
}DPP_ETM_CGAVD_PROTOCOL_QUEUE_EN_T;

typedef struct dpp_etm_cgavd_cfg_tc_flowid_dat_t
{
    ZXIC_UINT32 cfg_tc_flowid_dat;
}DPP_ETM_CGAVD_CFG_TC_FLOWID_DAT_T;

typedef struct dpp_etm_cgavd_flow_td_th_t
{
    ZXIC_UINT32 flow_td_th;
}DPP_ETM_CGAVD_FLOW_TD_TH_T;

typedef struct dpp_etm_cgavd_flow_ca_mtd_t
{
    ZXIC_UINT32 flow_ca_mtd;
}DPP_ETM_CGAVD_FLOW_CA_MTD_T;

typedef struct dpp_etm_cgavd_flow_dynamic_th_en_t
{
    ZXIC_UINT32 flow_dynamic_th_en;
}DPP_ETM_CGAVD_FLOW_DYNAMIC_TH_EN_T;

typedef struct dpp_etm_cgavd_pp_num_t
{
    ZXIC_UINT32 pp_num;
}DPP_ETM_CGAVD_PP_NUM_T;

typedef struct dpp_etm_cgavd_flow_q_len_t
{
    ZXIC_UINT32 flow_q_len;
}DPP_ETM_CGAVD_FLOW_Q_LEN_T;

typedef struct dpp_etm_cgavd_flow_wred_grp_t
{
    ZXIC_UINT32 flow_wred_grp;
}DPP_ETM_CGAVD_FLOW_WRED_GRP_T;

typedef struct dpp_etm_cgavd_flow_avg_q_len_t
{
    ZXIC_UINT32 flow_avg_q_len;
}DPP_ETM_CGAVD_FLOW_AVG_Q_LEN_T;

typedef struct dpp_etm_cgavd_qos_sign_t
{
    ZXIC_UINT32 qos_sign_flow_cfg_din;
}DPP_ETM_CGAVD_QOS_SIGN_T;

typedef struct dpp_etm_cgavd_q_pri_t
{
    ZXIC_UINT32 qpri_flow_cfg_din;
}DPP_ETM_CGAVD_Q_PRI_T;

typedef struct dpp_etm_cgavd_odma_tm_itmd_rd_low_t
{
    ZXIC_UINT32 odma_tm_itmd_low;
}DPP_ETM_CGAVD_ODMA_TM_ITMD_RD_LOW_T;

typedef struct dpp_etm_cgavd_odma_tm_itmd_rd_mid_t
{
    ZXIC_UINT32 odma_tm_itmd_mid;
}DPP_ETM_CGAVD_ODMA_TM_ITMD_RD_MID_T;

typedef struct dpp_etm_cgavd_odma_tm_itmd_rd_high_t
{
    ZXIC_UINT32 odma_tm_itmd_high;
}DPP_ETM_CGAVD_ODMA_TM_ITMD_RD_HIGH_T;

typedef struct dpp_etm_cgavd_cgavd_stat_pkt_len_t
{
    ZXIC_UINT32 expect_deq_pkt_len;
    ZXIC_UINT32 expect_enq_pkt_len;
}DPP_ETM_CGAVD_CGAVD_STAT_PKT_LEN_T;

typedef struct dpp_etm_cgavd_cgavd_stat_qnum_t
{
    ZXIC_UINT32 cgavd_unexcept_qnum;
    ZXIC_UINT32 cgavd_except_qnum;
}DPP_ETM_CGAVD_CGAVD_STAT_QNUM_T;

typedef struct dpp_etm_cgavd_cgavd_stat_dp_t
{
    ZXIC_UINT32 cgavd_stat_dp;
}DPP_ETM_CGAVD_CGAVD_STAT_DP_T;

typedef struct dpp_etm_cgavd_flow_num0_t
{
    ZXIC_UINT32 flow_num0;
}DPP_ETM_CGAVD_FLOW_NUM0_T;

typedef struct dpp_etm_cgavd_flow_num1_t
{
    ZXIC_UINT32 flow_num1;
}DPP_ETM_CGAVD_FLOW_NUM1_T;

typedef struct dpp_etm_cgavd_flow_num2_t
{
    ZXIC_UINT32 flow_num2;
}DPP_ETM_CGAVD_FLOW_NUM2_T;

typedef struct dpp_etm_cgavd_flow_num3_t
{
    ZXIC_UINT32 flow_num3;
}DPP_ETM_CGAVD_FLOW_NUM3_T;

typedef struct dpp_etm_cgavd_flow_num4_t
{
    ZXIC_UINT32 flow_num4;
}DPP_ETM_CGAVD_FLOW_NUM4_T;

typedef struct dpp_etm_cgavd_flow0_imem_cnt_t
{
    ZXIC_UINT32 flow0_imem_cnt;
}DPP_ETM_CGAVD_FLOW0_IMEM_CNT_T;

typedef struct dpp_etm_cgavd_flow1_imem_cnt_t
{
    ZXIC_UINT32 flow1_imem_cnt;
}DPP_ETM_CGAVD_FLOW1_IMEM_CNT_T;

typedef struct dpp_etm_cgavd_flow2_imem_cnt_t
{
    ZXIC_UINT32 flow2_imem_cnt;
}DPP_ETM_CGAVD_FLOW2_IMEM_CNT_T;

typedef struct dpp_etm_cgavd_flow3_imem_cnt_t
{
    ZXIC_UINT32 flow3_imem_cnt;
}DPP_ETM_CGAVD_FLOW3_IMEM_CNT_T;

typedef struct dpp_etm_cgavd_flow4_imem_cnt_t
{
    ZXIC_UINT32 flow4_imem_cnt;
}DPP_ETM_CGAVD_FLOW4_IMEM_CNT_T;

typedef struct dpp_etm_cgavd_flow0_drop_cnt_t
{
    ZXIC_UINT32 flow0_drop_cnt;
}DPP_ETM_CGAVD_FLOW0_DROP_CNT_T;

typedef struct dpp_etm_cgavd_flow1_drop_cnt_t
{
    ZXIC_UINT32 flow1_drop_cnt;
}DPP_ETM_CGAVD_FLOW1_DROP_CNT_T;

typedef struct dpp_etm_cgavd_flow2_drop_cnt_t
{
    ZXIC_UINT32 flow2_drop_cnt;
}DPP_ETM_CGAVD_FLOW2_DROP_CNT_T;

typedef struct dpp_etm_cgavd_flow3_drop_cnt_t
{
    ZXIC_UINT32 flow3_drop_cnt;
}DPP_ETM_CGAVD_FLOW3_DROP_CNT_T;

typedef struct dpp_etm_cgavd_flow4_drop_cnt_t
{
    ZXIC_UINT32 flow4_drop_cnt;
}DPP_ETM_CGAVD_FLOW4_DROP_CNT_T;

typedef struct dpp_etm_cgavd_fc_count_mode_t
{
    ZXIC_UINT32 fc_count_mode;
}DPP_ETM_CGAVD_FC_COUNT_MODE_T;

typedef struct dpp_etm_cgavd_qmu_cgavd_fc_num_t
{
    ZXIC_UINT32 qmu_cgavd_fc_state;
    ZXIC_UINT32 qmu_cgavd_fc_num;
}DPP_ETM_CGAVD_QMU_CGAVD_FC_NUM_T;

typedef struct dpp_etm_cgavd_cgavd_odma_fc_num_t
{
    ZXIC_UINT32 cgavd_lif_fc_state;
    ZXIC_UINT32 cgavd_lif_fc_num;
}DPP_ETM_CGAVD_CGAVD_ODMA_FC_NUM_T;

typedef struct dpp_etm_cgavd_cfg_offset_t
{
    ZXIC_UINT32 cfg_offset;
}DPP_ETM_CGAVD_CFG_OFFSET_T;

typedef struct dpp_etm_tmmu_tmmu_init_done_t
{
    ZXIC_UINT32 tmmu_init_done;
}DPP_ETM_TMMU_TMMU_INIT_DONE_T;

typedef struct dpp_etm_tmmu_tmmu_int_mask_1_t
{
    ZXIC_UINT32 imem_enq_rd_fifo_full_mask;
    ZXIC_UINT32 imem_enq_rd_fifo_overflow_mask;
    ZXIC_UINT32 imem_enq_rd_fifo_underflow_mask;
    ZXIC_UINT32 imem_enq_drop_fifo_full_mask;
    ZXIC_UINT32 imem_enq_drop_fifo_overflow_mask;
    ZXIC_UINT32 imem_enq_drop_fifo_underflow_mask;
    ZXIC_UINT32 imem_deq_rd_fifo_full_mask;
    ZXIC_UINT32 imem_deq_rd_fifo_overflow_mask;
    ZXIC_UINT32 imem_deq_rd_fifo_underflow_mask;
    ZXIC_UINT32 imem_deq_drop_fifo_full_mask;
    ZXIC_UINT32 imem_deq_drop_fifo_overflow_mask;
    ZXIC_UINT32 imem_deq_drop_fifo_underflow_mask;
    ZXIC_UINT32 dma_data_fifo_full_mask;
    ZXIC_UINT32 dma_data_fifo_overflow_mask;
    ZXIC_UINT32 dma_data_fifo_underflow_mask;
    ZXIC_UINT32 wr_cmd_fifo_full_mask;
    ZXIC_UINT32 wr_cmd_fifo_overflow_mask;
    ZXIC_UINT32 wr_cmd_fifo_underflow_mask;
    ZXIC_UINT32 cached_pd_fifo_full_mask;
    ZXIC_UINT32 cached_pd_fifo_overflow_mask;
    ZXIC_UINT32 cached_pd_fifo_underflow_mask;
    ZXIC_UINT32 emem_pd_fifo_full_mask;
    ZXIC_UINT32 emem_pd_fifo_overflow_mask;
    ZXIC_UINT32 emem_pd_fifo_underflow_mask;
    ZXIC_UINT32 pd_order_fifo_full_mask;
    ZXIC_UINT32 pd_order_fifo_overflow_mask;
    ZXIC_UINT32 pd_order_fifo_underflow_mask;
}DPP_ETM_TMMU_TMMU_INT_MASK_1_T;

typedef struct dpp_etm_tmmu_tmmu_int_mask_2_t
{
    ZXIC_UINT32 dma_data_fifo_parity_err_mask;
    ZXIC_UINT32 imem_enq_rd_fifo_ecc_single_err_mask;
    ZXIC_UINT32 imem_enq_rd_fifo_ecc_double_err_mask;
    ZXIC_UINT32 imem_enq_drop_fifo_ecc_single_err_mask;
    ZXIC_UINT32 imem_enq_drop_fifo_ecc_double_err_mask;
    ZXIC_UINT32 imem_deq_rd_fifo_ecc_single_err_mask;
    ZXIC_UINT32 imem_deq_rd_fifo_ecc_double_err_mask;
    ZXIC_UINT32 imem_deq_drop_fifo_ecc_single_err_mask;
    ZXIC_UINT32 imem_deq_drop_fifo_ecc_double_err_mask;
    ZXIC_UINT32 wr_cmd_fifo_ecc_single_err_mask;
    ZXIC_UINT32 wr_cmd_fifo_ecc_double_err_mask;
    ZXIC_UINT32 pd_cache_ram_ecc_single_err_mask;
    ZXIC_UINT32 pd_cache_ram_ecc_double_err_mask;
    ZXIC_UINT32 cached_pd_fifo_ecc_single_err_mask;
    ZXIC_UINT32 cached_pd_fifo_ecc_double_err_mask;
    ZXIC_UINT32 emem_pd_fifo_ecc_single_err_mask;
    ZXIC_UINT32 emem_pd_fifo_ecc_double_err_mask;
}DPP_ETM_TMMU_TMMU_INT_MASK_2_T;

typedef struct dpp_etm_tmmu_cfgmt_tm_pure_imem_en_t
{
    ZXIC_UINT32 cfgmt_tm_pure_imem_en;
}DPP_ETM_TMMU_CFGMT_TM_PURE_IMEM_EN_T;

typedef struct dpp_etm_tmmu_cfgmt_force_ddr_rdy_cfg_t
{
    ZXIC_UINT32 cfgmt_force_ddr_rdy_cfg;
}DPP_ETM_TMMU_CFGMT_FORCE_DDR_RDY_CFG_T;

typedef struct dpp_etm_tmmu_pd_order_fifo_aful_th_t
{
    ZXIC_UINT32 pd_order_fifo_aful_th;
}DPP_ETM_TMMU_PD_ORDER_FIFO_AFUL_TH_T;

typedef struct dpp_etm_tmmu_cached_pd_fifo_aful_th_t
{
    ZXIC_UINT32 cached_pd_fifo_aful_th;
}DPP_ETM_TMMU_CACHED_PD_FIFO_AFUL_TH_T;

typedef struct dpp_etm_tmmu_wr_cmd_fifo_aful_th_t
{
    ZXIC_UINT32 wr_cmd_fifo_aful_th;
}DPP_ETM_TMMU_WR_CMD_FIFO_AFUL_TH_T;

typedef struct dpp_etm_tmmu_imem_enq_rd_fifo_aful_th_t
{
    ZXIC_UINT32 imem_enq_rd_fifo_aful_th;
}DPP_ETM_TMMU_IMEM_ENQ_RD_FIFO_AFUL_TH_T;

typedef struct dpp_etm_tmmu_imem_enq_drop_fifo_aful_th_t
{
    ZXIC_UINT32 imem_enq_drop_fifo_aful_th;
}DPP_ETM_TMMU_IMEM_ENQ_DROP_FIFO_AFUL_TH_T;

typedef struct dpp_etm_tmmu_imem_deq_drop_fifo_aful_th_t
{
    ZXIC_UINT32 imem_deq_drop_fifo_aful_th;
}DPP_ETM_TMMU_IMEM_DEQ_DROP_FIFO_AFUL_TH_T;

typedef struct dpp_etm_tmmu_imem_deq_rd_fifo_aful_th_t
{
    ZXIC_UINT32 imem_deq_rd_fifo_aful_th;
}DPP_ETM_TMMU_IMEM_DEQ_RD_FIFO_AFUL_TH_T;

typedef struct dpp_etm_tmmu_tmmu_states_1_t
{
    ZXIC_UINT32 imem_enq_rd_fifo_full;
    ZXIC_UINT32 imem_enq_rd_fifo_overflow;
    ZXIC_UINT32 imem_enq_rd_fifo_underflow;
    ZXIC_UINT32 imem_enq_drop_fifo_full;
    ZXIC_UINT32 imem_enq_drop_fifo_overflow;
    ZXIC_UINT32 imem_enq_drop_fifo_underflow;
    ZXIC_UINT32 imem_deq_rd_fifo_full;
    ZXIC_UINT32 imem_deq_rd_fifo_overflow;
    ZXIC_UINT32 imem_deq_rd_fifo_underflow;
    ZXIC_UINT32 imem_deq_drop_fifo_full;
    ZXIC_UINT32 imem_deq_drop_fifo_overflow;
    ZXIC_UINT32 imem_deq_drop_fifo_underflow;
    ZXIC_UINT32 dma_data_fifo_full;
    ZXIC_UINT32 dma_data_fifo_overflow;
    ZXIC_UINT32 dma_data_fifo_underflow;
    ZXIC_UINT32 wr_cmd_fifo_full;
    ZXIC_UINT32 wr_cmd_fifo_overflow;
    ZXIC_UINT32 wr_cmd_fifo_underflow;
    ZXIC_UINT32 cached_pd_fifo_full;
    ZXIC_UINT32 cached_pd_fifo_overflow;
    ZXIC_UINT32 cached_pd_fifo_underflow;
    ZXIC_UINT32 emem_pd_fifo_full;
    ZXIC_UINT32 emem_pd_fifo_overflow;
    ZXIC_UINT32 emem_pd_fifo_underflow;
    ZXIC_UINT32 pd_order_fifo_full;
    ZXIC_UINT32 pd_order_fifo_overflow;
    ZXIC_UINT32 pd_order_fifo_underflow;
}DPP_ETM_TMMU_TMMU_STATES_1_T;

typedef struct dpp_etm_tmmu_tmmu_states_2_t
{
    ZXIC_UINT32 dma_data_fifo_parity_err;
    ZXIC_UINT32 imem_enq_rd_fifo_ecc_single_err;
    ZXIC_UINT32 imem_enq_rd_fifo_ecc_double_err;
    ZXIC_UINT32 imem_enq_drop_fifo_ecc_single_err;
    ZXIC_UINT32 imem_enq_drop_fifo_ecc_double_err;
    ZXIC_UINT32 imem_deq_rd_fifo_ecc_single_err;
    ZXIC_UINT32 imem_deq_rd_fifo_ecc_double_err;
    ZXIC_UINT32 imem_deq_drop_fifo_ecc_single_err;
    ZXIC_UINT32 imem_deq_drop_fifo_ecc_double_err;
    ZXIC_UINT32 wr_cmd_fifo_ecc_single_err;
    ZXIC_UINT32 wr_cmd_fifo_ecc_double_err;
    ZXIC_UINT32 pd_cache_ram_ecc_single_err;
    ZXIC_UINT32 pd_cache_ram_ecc_double_err;
    ZXIC_UINT32 cached_pd_fifo_ecc_single_err;
    ZXIC_UINT32 cached_pd_fifo_ecc_double_err;
    ZXIC_UINT32 emem_pd_fifo_ecc_single_err;
    ZXIC_UINT32 emem_pd_fifo_ecc_double_err;
}DPP_ETM_TMMU_TMMU_STATES_2_T;

typedef struct dpp_etm_shap_shap_ind_cmd_t
{
    ZXIC_UINT32 rd;
    ZXIC_UINT32 mem_id;
    ZXIC_UINT32 addr;
}DPP_ETM_SHAP_SHAP_IND_CMD_T;

typedef struct dpp_etm_shap_shap_ind_sta_t
{
    ZXIC_UINT32 indirectaccessdone;
}DPP_ETM_SHAP_SHAP_IND_STA_T;

typedef struct dpp_etm_shap_shap_ind_data0_t
{
    ZXIC_UINT32 indirectdata0;
}DPP_ETM_SHAP_SHAP_IND_DATA0_T;

typedef struct dpp_etm_shap_shap_ind_data1_t
{
    ZXIC_UINT32 indirectdata1;
}DPP_ETM_SHAP_SHAP_IND_DATA1_T;

typedef struct dpp_etm_shap_full_threshold_t
{
    ZXIC_UINT32 full_threshold;
}DPP_ETM_SHAP_FULL_THRESHOLD_T;

typedef struct dpp_etm_shap_empty_threshold_t
{
    ZXIC_UINT32 empty_threshold;
}DPP_ETM_SHAP_EMPTY_THRESHOLD_T;

typedef struct dpp_etm_shap_shap_sta_init_cfg_t
{
    ZXIC_UINT32 sta_ram_init_done;
    ZXIC_UINT32 sta_ram_init_en;
}DPP_ETM_SHAP_SHAP_STA_INIT_CFG_T;

typedef struct dpp_etm_shap_shap_cfg_init_cfg_t
{
    ZXIC_UINT32 cfg_ram_init_done;
    ZXIC_UINT32 cfg_ram_init_en;
}DPP_ETM_SHAP_SHAP_CFG_INIT_CFG_T;

typedef struct dpp_etm_shap_token_mode_switch_t
{
    ZXIC_UINT32 token_mode_switch;
}DPP_ETM_SHAP_TOKEN_MODE_SWITCH_T;

typedef struct dpp_etm_shap_token_grain_t
{
    ZXIC_UINT32 token_grain;
}DPP_ETM_SHAP_TOKEN_GRAIN_T;

typedef struct dpp_etm_shap_crd_grain_t
{
    ZXIC_UINT32 crd_grain;
}DPP_ETM_SHAP_CRD_GRAIN_T;

typedef struct dpp_etm_shap_shap_stat_ctrl_t
{
    ZXIC_UINT32 shap_stat_ctrl;
}DPP_ETM_SHAP_SHAP_STAT_CTRL_T;

typedef struct dpp_etm_shap_token_stat_id_t
{
    ZXIC_UINT32 token_stat_id;
}DPP_ETM_SHAP_TOKEN_STAT_ID_T;

typedef struct dpp_etm_shap_token_stat_t
{
    ZXIC_UINT32 token_stat;
}DPP_ETM_SHAP_TOKEN_STAT_T;

typedef struct dpp_etm_shap_shap_stat_clk_cnt_t
{
    ZXIC_UINT32 shap_stat_clk_cnt;
}DPP_ETM_SHAP_SHAP_STAT_CLK_CNT_T;

typedef struct dpp_etm_shap_shap_bucket_map_tbl_t
{
    ZXIC_UINT32 shap_map;
}DPP_ETM_SHAP_SHAP_BUCKET_MAP_TBL_T;

typedef struct dpp_etm_shap_bkt_para_tbl_t
{
    ZXIC_UINT32 bucket_depth;
    ZXIC_UINT32 bucket_rate;
}DPP_ETM_SHAP_BKT_PARA_TBL_T;

typedef struct dpp_etm_crdt_credit_en_t
{
    ZXIC_UINT32 credit_en;
}DPP_ETM_CRDT_CREDIT_EN_T;

typedef struct dpp_etm_crdt_crt_inter1_t
{
    ZXIC_UINT32 crd_inter1;
}DPP_ETM_CRDT_CRT_INTER1_T;

typedef struct dpp_etm_crdt_db_token_t
{
    ZXIC_UINT32 db_token;
}DPP_ETM_CRDT_DB_TOKEN_T;

typedef struct dpp_etm_crdt_crs_flt_cfg_t
{
    ZXIC_UINT32 crs_flt_cfg;
}DPP_ETM_CRDT_CRS_FLT_CFG_T;

typedef struct dpp_etm_crdt_th_sp_t
{
    ZXIC_UINT32 th_sp;
}DPP_ETM_CRDT_TH_SP_T;

typedef struct dpp_etm_crdt_th_wfq_fq_t
{
    ZXIC_UINT32 th_fq;
    ZXIC_UINT32 th_wfq;
}DPP_ETM_CRDT_TH_WFQ_FQ_T;

typedef struct dpp_etm_crdt_th_wfq2_fq2_t
{
    ZXIC_UINT32 th_fq2;
    ZXIC_UINT32 th_wfq2;
}DPP_ETM_CRDT_TH_WFQ2_FQ2_T;

typedef struct dpp_etm_crdt_th_wfq4_fq4_t
{
    ZXIC_UINT32 th_fq4;
    ZXIC_UINT32 th_wfq4;
}DPP_ETM_CRDT_TH_WFQ4_FQ4_T;

typedef struct dpp_etm_crdt_cfg_state_t
{
    ZXIC_UINT32 cfg_state;
}DPP_ETM_CRDT_CFG_STATE_T;

typedef struct dpp_etm_crdt_crdt_ind_cmd_t
{
    ZXIC_UINT32 rd;
    ZXIC_UINT32 mem_id;
    ZXIC_UINT32 addr;
}DPP_ETM_CRDT_CRDT_IND_CMD_T;

typedef struct dpp_etm_crdt_crdt_ind_sta_t
{
    ZXIC_UINT32 indirectaccessdone;
}DPP_ETM_CRDT_CRDT_IND_STA_T;

typedef struct dpp_etm_crdt_crdt_ind_data0_t
{
    ZXIC_UINT32 indirectdata0;
}DPP_ETM_CRDT_CRDT_IND_DATA0_T;

typedef struct dpp_etm_crdt_crdt_ind_data1_t
{
    ZXIC_UINT32 indirectdata1;
}DPP_ETM_CRDT_CRDT_IND_DATA1_T;

typedef struct dpp_etm_crdt_crdt_state_t
{
    ZXIC_UINT32 crdt_int;
    ZXIC_UINT32 crdt_rdy;
}DPP_ETM_CRDT_CRDT_STATE_T;

typedef struct dpp_etm_crdt_stat_que_id_0_t
{
    ZXIC_UINT32 stat_que_id_0;
}DPP_ETM_CRDT_STAT_QUE_ID_0_T;

typedef struct dpp_etm_crdt_stat_que_id_1_t
{
    ZXIC_UINT32 stat_que_id_1;
}DPP_ETM_CRDT_STAT_QUE_ID_1_T;

typedef struct dpp_etm_crdt_stat_que_id_2_t
{
    ZXIC_UINT32 stat_que_id_2;
}DPP_ETM_CRDT_STAT_QUE_ID_2_T;

typedef struct dpp_etm_crdt_stat_que_id_3_t
{
    ZXIC_UINT32 stat_que_id_3;
}DPP_ETM_CRDT_STAT_QUE_ID_3_T;

typedef struct dpp_etm_crdt_stat_que_id_4_t
{
    ZXIC_UINT32 stat_que_id_4;
}DPP_ETM_CRDT_STAT_QUE_ID_4_T;

typedef struct dpp_etm_crdt_stat_que_id_5_t
{
    ZXIC_UINT32 stat_que_id_5;
}DPP_ETM_CRDT_STAT_QUE_ID_5_T;

typedef struct dpp_etm_crdt_stat_que_id_6_t
{
    ZXIC_UINT32 stat_que_id_6;
}DPP_ETM_CRDT_STAT_QUE_ID_6_T;

typedef struct dpp_etm_crdt_stat_que_id_7_t
{
    ZXIC_UINT32 stat_que_id_7;
}DPP_ETM_CRDT_STAT_QUE_ID_7_T;

typedef struct dpp_etm_crdt_stat_que_id_8_t
{
    ZXIC_UINT32 stat_que_id_8;
}DPP_ETM_CRDT_STAT_QUE_ID_8_T;

typedef struct dpp_etm_crdt_stat_que_id_9_t
{
    ZXIC_UINT32 stat_que_id_9;
}DPP_ETM_CRDT_STAT_QUE_ID_9_T;

typedef struct dpp_etm_crdt_stat_que_id_10_t
{
    ZXIC_UINT32 stat_que_id_10;
}DPP_ETM_CRDT_STAT_QUE_ID_10_T;

typedef struct dpp_etm_crdt_stat_que_id_11_t
{
    ZXIC_UINT32 stat_que_id_11;
}DPP_ETM_CRDT_STAT_QUE_ID_11_T;

typedef struct dpp_etm_crdt_stat_que_id_12_t
{
    ZXIC_UINT32 stat_que_id_12;
}DPP_ETM_CRDT_STAT_QUE_ID_12_T;

typedef struct dpp_etm_crdt_stat_que_id_13_t
{
    ZXIC_UINT32 stat_que_id_13;
}DPP_ETM_CRDT_STAT_QUE_ID_13_T;

typedef struct dpp_etm_crdt_stat_que_id_14_t
{
    ZXIC_UINT32 stat_que_id_14;
}DPP_ETM_CRDT_STAT_QUE_ID_14_T;

typedef struct dpp_etm_crdt_stat_que_id_15_t
{
    ZXIC_UINT32 stat_que_id_15;
}DPP_ETM_CRDT_STAT_QUE_ID_15_T;

typedef struct dpp_etm_crdt_stat_que_credit_t
{
    ZXIC_UINT32 stat_que_credit_cnt;
}DPP_ETM_CRDT_STAT_QUE_CREDIT_T;

typedef struct dpp_etm_crdt_crdt_cfg_ram_init_t
{
    ZXIC_UINT32 cfg_ram_init_done;
    ZXIC_UINT32 cfg_ram_init_en;
}DPP_ETM_CRDT_CRDT_CFG_RAM_INIT_T;

typedef struct dpp_etm_crdt_crdt_sta_ram_init_t
{
    ZXIC_UINT32 sta_ram_init_done;
    ZXIC_UINT32 sta_ram_init_en;
}DPP_ETM_CRDT_CRDT_STA_RAM_INIT_T;

typedef struct dpp_etm_crdt_crs_que_id_t
{
    ZXIC_UINT32 crs_que_id;
}DPP_ETM_CRDT_CRS_QUE_ID_T;

typedef struct dpp_etm_crdt_qmu_crs_end_state_t
{
    ZXIC_UINT32 qmu_crs_end_state;
}DPP_ETM_CRDT_QMU_CRS_END_STATE_T;

typedef struct dpp_etm_crdt_shap_rdy_t
{
    ZXIC_UINT32 shap_rdy;
}DPP_ETM_CRDT_SHAP_RDY_T;

typedef struct dpp_etm_crdt_shap_int_reg_t
{
    ZXIC_UINT32 pp_c_token_min_int;
}DPP_ETM_CRDT_SHAP_INT_REG_T;

typedef struct dpp_etm_crdt_shap_int_mask_reg_t
{
    ZXIC_UINT32 pp_c_token_min_int_mask;
}DPP_ETM_CRDT_SHAP_INT_MASK_REG_T;

typedef struct dpp_etm_crdt_token_state_almost_empty_th_t
{
    ZXIC_UINT32 token_state_almost_empty_th;
}DPP_ETM_CRDT_TOKEN_STATE_ALMOST_EMPTY_TH_T;

typedef struct dpp_etm_crdt_token_state_empty_th_t
{
    ZXIC_UINT32 token_state_empty_th;
}DPP_ETM_CRDT_TOKEN_STATE_EMPTY_TH_T;

typedef struct dpp_etm_crdt_full_th_t
{
    ZXIC_UINT32 token_state_full_th;
}DPP_ETM_CRDT_FULL_TH_T;

typedef struct dpp_etm_crdt_pp_c_level_shap_en_t
{
    ZXIC_UINT32 pp_c_level_shap_en;
}DPP_ETM_CRDT_PP_C_LEVEL_SHAP_EN_T;

typedef struct dpp_etm_crdt_enq_token_th_t
{
    ZXIC_UINT32 enq_token_th;
}DPP_ETM_CRDT_ENQ_TOKEN_TH_T;

typedef struct dpp_etm_crdt_pp_tokenq_level1_qstate_weight_cir_t
{
    ZXIC_UINT32 pp_pp_q_state_cir;
    ZXIC_UINT32 pp_pp_q_weight_wfq_l1_cir;
}DPP_ETM_CRDT_PP_TOKENQ_LEVEL1_QSTATE_WEIGHT_CIR_T;

typedef struct dpp_etm_crdt_pp_idle_weight_level1_cir_t
{
    ZXIC_UINT32 pp_idle_q_weight_wfq_l1_cir;
}DPP_ETM_CRDT_PP_IDLE_WEIGHT_LEVEL1_CIR_T;

typedef struct dpp_etm_crdt_rci_grade_th_0_cfg_t
{
    ZXIC_UINT32 rci_grade_th_0_cfg;
}DPP_ETM_CRDT_RCI_GRADE_TH_0_CFG_T;

typedef struct dpp_etm_crdt_rci_grade_th_1_cfg_t
{
    ZXIC_UINT32 rci_grade_th_1_cfg;
}DPP_ETM_CRDT_RCI_GRADE_TH_1_CFG_T;

typedef struct dpp_etm_crdt_rci_grade_th_2_cfg_t
{
    ZXIC_UINT32 rci_grade_th_2_cfg;
}DPP_ETM_CRDT_RCI_GRADE_TH_2_CFG_T;

typedef struct dpp_etm_crdt_rci_grade_th_3_cfg_t
{
    ZXIC_UINT32 rci_grade_th_3_cfg;
}DPP_ETM_CRDT_RCI_GRADE_TH_3_CFG_T;

typedef struct dpp_etm_crdt_rci_grade_th_4_cfg_t
{
    ZXIC_UINT32 rci_grade_th_4_cfg;
}DPP_ETM_CRDT_RCI_GRADE_TH_4_CFG_T;

typedef struct dpp_etm_crdt_rci_grade_th_5_cfg_t
{
    ZXIC_UINT32 rci_grade_th_5_cfg;
}DPP_ETM_CRDT_RCI_GRADE_TH_5_CFG_T;

typedef struct dpp_etm_crdt_rci_grade_th_6_cfg_t
{
    ZXIC_UINT32 rci_grade_th_6_cfg;
}DPP_ETM_CRDT_RCI_GRADE_TH_6_CFG_T;

typedef struct dpp_etm_crdt_flow_del_cmd_t
{
    ZXIC_UINT32 flow_del_busy;
    ZXIC_UINT32 flow_alt_cmd;
    ZXIC_UINT32 flow_alt_ind;
}DPP_ETM_CRDT_FLOW_DEL_CMD_T;

typedef struct dpp_etm_crdt_cnt_clr_t
{
    ZXIC_UINT32 cnt_clr;
}DPP_ETM_CRDT_CNT_CLR_T;

typedef struct dpp_etm_crdt_crdt_int_bus_t
{
    ZXIC_UINT32 ldstr_fifo15_ovf_int;
    ZXIC_UINT32 ldstr_fifo14_ovf_int;
    ZXIC_UINT32 ldstr_fifo13_ovf_int;
    ZXIC_UINT32 ldstr_fifo12_ovf_int;
    ZXIC_UINT32 ldstr_fifo11_ovf_int;
    ZXIC_UINT32 ldstr_fifo10_ovf_int;
    ZXIC_UINT32 ldstr_fifo9_ovf_int;
    ZXIC_UINT32 ldstr_fifo8_ovf_int;
    ZXIC_UINT32 ldstr_fifo7_ovf_int;
    ZXIC_UINT32 ldstr_fifo6_ovf_int;
    ZXIC_UINT32 ldstr_fifo5_ovf_int;
    ZXIC_UINT32 ldstr_fifo4_ovf_int;
    ZXIC_UINT32 ldstr_fifo3_ovf_int;
    ZXIC_UINT32 ldstr_fifo2_ovf_int;
    ZXIC_UINT32 ldstr_fifo1_ovf_int;
    ZXIC_UINT32 ldstr_fifo0_ovf_int;
    ZXIC_UINT32 cfg_del_err_int;
    ZXIC_UINT32 flwin_secrs_fifo_ovf_int;
    ZXIC_UINT32 flwin_voqcrs_fifo_ovf_int;
}DPP_ETM_CRDT_CRDT_INT_BUS_T;

typedef struct dpp_etm_crdt_crdt_int_mask_t
{
    ZXIC_UINT32 crdt_int_mask;
}DPP_ETM_CRDT_CRDT_INT_MASK_T;

typedef struct dpp_etm_crdt_cfg_weight_together_t
{
    ZXIC_UINT32 cfg_weight_together;
}DPP_ETM_CRDT_CFG_WEIGHT_TOGETHER_T;

typedef struct dpp_etm_crdt_weight_t
{
    ZXIC_UINT32 c_weight;
    ZXIC_UINT32 e_weight;
}DPP_ETM_CRDT_WEIGHT_T;

typedef struct dpp_etm_crdt_dev_sp_state_t
{
    ZXIC_UINT32 dev_sp_state;
}DPP_ETM_CRDT_DEV_SP_STATE_T;

typedef struct dpp_etm_crdt_dev_crs_t
{
    ZXIC_UINT32 dev_crs;
}DPP_ETM_CRDT_DEV_CRS_T;

typedef struct dpp_etm_crdt_congest_token_disable_31_0_t
{
    ZXIC_UINT32 congest_token_disable_31_0;
}DPP_ETM_CRDT_CONGEST_TOKEN_DISABLE_31_0_T;

typedef struct dpp_etm_crdt_congest_token_disable_63_32_t
{
    ZXIC_UINT32 congest_token_disable_63_32;
}DPP_ETM_CRDT_CONGEST_TOKEN_DISABLE_63_32_T;

typedef struct dpp_etm_crdt_crdt_interval_en_cfg_t
{
    ZXIC_UINT32 crdt_interval_en;
}DPP_ETM_CRDT_CRDT_INTERVAL_EN_CFG_T;

typedef struct dpp_etm_crdt_q_token_staue_cfg_t
{
    ZXIC_UINT32 test_token_q_id;
}DPP_ETM_CRDT_Q_TOKEN_STAUE_CFG_T;

typedef struct dpp_etm_crdt_q_token_dist_cnt_t
{
    ZXIC_UINT32 q_token_dist_counter;
}DPP_ETM_CRDT_Q_TOKEN_DIST_CNT_T;

typedef struct dpp_etm_crdt_q_token_dec_cnt_t
{
    ZXIC_UINT32 q_token_dec_counter;
}DPP_ETM_CRDT_Q_TOKEN_DEC_CNT_T;

typedef struct dpp_etm_crdt_pp_weight_ram_t
{
    ZXIC_UINT32 pp_c_weight;
}DPP_ETM_CRDT_PP_WEIGHT_RAM_T;

typedef struct dpp_etm_crdt_pp_cbs_shape_en_ram_t
{
    ZXIC_UINT32 pp_cbs;
    ZXIC_UINT32 pp_c_shap_en;
}DPP_ETM_CRDT_PP_CBS_SHAPE_EN_RAM_T;

typedef struct dpp_etm_crdt_pp_next_pc_q_state_ram_t
{
    ZXIC_UINT32 pp_next_pc;
    ZXIC_UINT32 pp_token_num;
    ZXIC_UINT32 pp_q_state;
}DPP_ETM_CRDT_PP_NEXT_PC_Q_STATE_RAM_T;

typedef struct dpp_etm_crdt_dev_interval_t
{
    ZXIC_UINT32 dev_interval;
}DPP_ETM_CRDT_DEV_INTERVAL_T;

typedef struct dpp_etm_crdt_dev_wfq_cnt_t
{
    ZXIC_UINT32 dev_wfq_cnt;
}DPP_ETM_CRDT_DEV_WFQ_CNT_T;

typedef struct dpp_etm_crdt_dev_wfq_state_t
{
    ZXIC_UINT32 dev_wfq_state;
}DPP_ETM_CRDT_DEV_WFQ_STATE_T;

typedef struct dpp_etm_crdt_dev_active_head_ptr_t
{
    ZXIC_UINT32 dev_active_head_ptr;
}DPP_ETM_CRDT_DEV_ACTIVE_HEAD_PTR_T;

typedef struct dpp_etm_crdt_dev_active_tail_ptr_t
{
    ZXIC_UINT32 dev_active_tail_ptr;
}DPP_ETM_CRDT_DEV_ACTIVE_TAIL_PTR_T;

typedef struct dpp_etm_crdt_dev_unactive_head_ptr_t
{
    ZXIC_UINT32 dev_unactive_head_ptr;
}DPP_ETM_CRDT_DEV_UNACTIVE_HEAD_PTR_T;

typedef struct dpp_etm_crdt_dev_unactive_tail_ptr_t
{
    ZXIC_UINT32 dev_unactive_tail_ptr;
}DPP_ETM_CRDT_DEV_UNACTIVE_TAIL_PTR_T;

typedef struct dpp_etm_crdt_pp_weight_t
{
    ZXIC_UINT32 pp_weight;
}DPP_ETM_CRDT_PP_WEIGHT_T;

typedef struct dpp_etm_crdt_pp_que_state_t
{
    ZXIC_UINT32 pp_enque_flag;
    ZXIC_UINT32 pp_cir;
    ZXIC_UINT32 pp_congest_cir;
    ZXIC_UINT32 pp_crs;
    ZXIC_UINT32 dev_sp;
}DPP_ETM_CRDT_PP_QUE_STATE_T;

typedef struct dpp_etm_crdt_pp_next_ptr_t
{
    ZXIC_UINT32 pp_next_ptr;
}DPP_ETM_CRDT_PP_NEXT_PTR_T;

typedef struct dpp_etm_crdt_pp_cfg_t
{
    ZXIC_UINT32 pp_cfg;
}DPP_ETM_CRDT_PP_CFG_T;

typedef struct dpp_etm_crdt_pp_up_ptr_t
{
    ZXIC_UINT32 pp_up_ptr;
}DPP_ETM_CRDT_PP_UP_PTR_T;

typedef struct dpp_etm_crdt_credit_drop_num_t
{
    ZXIC_UINT32 credit_drop_num;
}DPP_ETM_CRDT_CREDIT_DROP_NUM_T;

typedef struct dpp_etm_crdt_se_id_lv0_t
{
    ZXIC_UINT32 se_id_out_lv0;
}DPP_ETM_CRDT_SE_ID_LV0_T;

typedef struct dpp_etm_crdt_se_id_lv1_t
{
    ZXIC_UINT32 se_id_out_lv1;
}DPP_ETM_CRDT_SE_ID_LV1_T;

typedef struct dpp_etm_crdt_se_id_lv2_t
{
    ZXIC_UINT32 se_id_out_lv2;
}DPP_ETM_CRDT_SE_ID_LV2_T;

typedef struct dpp_etm_crdt_se_id_lv3_t
{
    ZXIC_UINT32 se_id_out_lv3;
}DPP_ETM_CRDT_SE_ID_LV3_T;

typedef struct dpp_etm_crdt_se_id_lv4_t
{
    ZXIC_UINT32 se_id_out_lv4;
}DPP_ETM_CRDT_SE_ID_LV4_T;

typedef struct dpp_etm_crdt_que_id_t
{
    ZXIC_UINT32 que_id_out;
}DPP_ETM_CRDT_QUE_ID_T;

typedef struct dpp_etm_crdt_se_info_lv0_t
{
    ZXIC_UINT32 se_shape_lv0;
    ZXIC_UINT32 se_ins_out_lv0;
    ZXIC_UINT32 se_state_out_lv0;
    ZXIC_UINT32 se_new_state_out_lv0;
}DPP_ETM_CRDT_SE_INFO_LV0_T;

typedef struct dpp_etm_crdt_se_info_lv1_t
{
    ZXIC_UINT32 se_shape_lv1;
    ZXIC_UINT32 se_ins_out_lv1;
    ZXIC_UINT32 se_state_out_lv1;
    ZXIC_UINT32 se_new_state_out_lv1;
}DPP_ETM_CRDT_SE_INFO_LV1_T;

typedef struct dpp_etm_crdt_se_info_lv2_t
{
    ZXIC_UINT32 se_shape_lv2;
    ZXIC_UINT32 se_ins_out_lv2;
    ZXIC_UINT32 se_state_out_lv2;
    ZXIC_UINT32 se_new_state_out_lv2;
}DPP_ETM_CRDT_SE_INFO_LV2_T;

typedef struct dpp_etm_crdt_se_info_lv3_t
{
    ZXIC_UINT32 se_shape_lv3;
    ZXIC_UINT32 se_ins_out_lv3;
    ZXIC_UINT32 se_state_out_lv3;
    ZXIC_UINT32 se_new_state_out_lv3;
}DPP_ETM_CRDT_SE_INFO_LV3_T;

typedef struct dpp_etm_crdt_se_info_lv4_t
{
    ZXIC_UINT32 se_shape_lv4;
    ZXIC_UINT32 se_ins_out_lv4;
    ZXIC_UINT32 se_state_out_lv4;
    ZXIC_UINT32 se_new_state_out_lv4;
}DPP_ETM_CRDT_SE_INFO_LV4_T;

typedef struct dpp_etm_crdt_que_state_t
{
    ZXIC_UINT32 que_state_out;
}DPP_ETM_CRDT_QUE_STATE_T;

typedef struct dpp_etm_crdt_eir_off_in_advance_t
{
    ZXIC_UINT32 eir_crs_filter;
}DPP_ETM_CRDT_EIR_OFF_IN_ADVANCE_T;

typedef struct dpp_etm_crdt_double_level_shap_prevent_t
{
    ZXIC_UINT32 double_level_shap_prevent;
}DPP_ETM_CRDT_DOUBLE_LEVEL_SHAP_PREVENT_T;

typedef struct dpp_etm_crdt_add_store_cycle_t
{
    ZXIC_UINT32 add_store_cycle;
}DPP_ETM_CRDT_ADD_STORE_CYCLE_T;

typedef struct dpp_etm_crdt_tflag2_wr_flag_sum_t
{
    ZXIC_UINT32 tflag2_wr_flag_sum;
}DPP_ETM_CRDT_TFLAG2_WR_FLAG_SUM_T;

typedef struct dpp_etm_crdt_flowque_para_tbl_t
{
    ZXIC_UINT32 flowque_link;
    ZXIC_UINT32 flowque_w;
    ZXIC_UINT32 flowque_pri;
}DPP_ETM_CRDT_FLOWQUE_PARA_TBL_T;

typedef struct dpp_etm_crdt_se_para_tbl_t
{
    ZXIC_UINT32 se_insw;
    ZXIC_UINT32 se_link;
    ZXIC_UINT32 cp_token_en;
    ZXIC_UINT32 se_w;
    ZXIC_UINT32 se_pri;
}DPP_ETM_CRDT_SE_PARA_TBL_T;

typedef struct dpp_etm_crdt_flowque_ins_tbl_t
{
    ZXIC_UINT32 flowque_ins;
}DPP_ETM_CRDT_FLOWQUE_INS_TBL_T;

typedef struct dpp_etm_crdt_se_ins_tbl_t
{
    ZXIC_UINT32 se_ins_flag;
    ZXIC_UINT32 se_ins_priority;
}DPP_ETM_CRDT_SE_INS_TBL_T;

typedef struct dpp_etm_crdt_eir_crs_filter_tbl_t
{
    ZXIC_UINT32 eir_crs_filter;
}DPP_ETM_CRDT_EIR_CRS_FILTER_TBL_T;

typedef struct dpp_etm_qmu_qcfg_qlist_cfg_done_t
{
    ZXIC_UINT32 qcfg_qlist_cfg_done;
}DPP_ETM_QMU_QCFG_QLIST_CFG_DONE_T;

typedef struct dpp_etm_qmu_qcfg_qsch_credit_value_t
{
    ZXIC_UINT32 qcfg_qsch_credit_value;
}DPP_ETM_QMU_QCFG_QSCH_CREDIT_VALUE_T;

typedef struct dpp_etm_qmu_qcfg_qsch_crbal_init_value_t
{
    ZXIC_UINT32 qcfg_qsch_crbal_init_value;
}DPP_ETM_QMU_QCFG_QSCH_CRBAL_INIT_VALUE_T;

typedef struct dpp_etm_qmu_qcfg_qsch_crbal_init_mask_t
{
    ZXIC_UINT32 qcfg_qsch_crbal_init_mask;
}DPP_ETM_QMU_QCFG_QSCH_CRBAL_INIT_MASK_T;

typedef struct dpp_etm_qmu_cmdsch_rd_cmd_aful_th_t
{
    ZXIC_UINT32 cmdsch_rd_cmd_aful_th;
}DPP_ETM_QMU_CMDSCH_RD_CMD_AFUL_TH_T;

typedef struct dpp_etm_qmu_cfg_port_fc_interval_t
{
    ZXIC_UINT32 cfg_port_fc_interval;
}DPP_ETM_QMU_CFG_PORT_FC_INTERVAL_T;

typedef struct dpp_etm_qmu_qcfg_csch_aged_cfg_t
{
    ZXIC_UINT32 qcfg_csch_aged_cfg;
}DPP_ETM_QMU_QCFG_CSCH_AGED_CFG_T;

typedef struct dpp_etm_qmu_qcfg_csch_aged_scan_time_t
{
    ZXIC_UINT32 qcfg_csch_aged_scan_time;
}DPP_ETM_QMU_QCFG_CSCH_AGED_SCAN_TIME_T;

typedef struct dpp_etm_qmu_qcfg_qmu_qlist_state_query_t
{
    ZXIC_UINT32 pkt_age_req_fifo_afull;
    ZXIC_UINT32 rd_release_fwft_afull;
    ZXIC_UINT32 drop_imem_fwft_afull;
    ZXIC_UINT32 pkt_age_req_fifo_empty;
    ZXIC_UINT32 rd_release_fwft_empty;
    ZXIC_UINT32 drop_imem_fwft_empty;
    ZXIC_UINT32 mmu_qmu_sop_rd_rdy;
    ZXIC_UINT32 big_fifo_empty;
    ZXIC_UINT32 qmu_mmu_rd_release_rdy;
    ZXIC_UINT32 xsw_qmu_crs_rdy;
    ZXIC_UINT32 mmu_qmu_rdy;
    ZXIC_UINT32 mmu_ql_wr_rdy;
    ZXIC_UINT32 mmu_ql_rd_rdy;
    ZXIC_UINT32 csw_ql_rdy;
    ZXIC_UINT32 ql_init_done;
    ZXIC_UINT32 free_addr_ready;
    ZXIC_UINT32 bank_group_afull;
    ZXIC_UINT32 pds_fwft_empty;
    ZXIC_UINT32 enq_rpt_fwft_afull;
}DPP_ETM_QMU_QCFG_QMU_QLIST_STATE_QUERY_T;

typedef struct dpp_etm_qmu_cfgmt_qsch_crbal_drop_en_t
{
    ZXIC_UINT32 cfgmt_qsch_all_crbal_drop_en;
    ZXIC_UINT32 cfgmt_qsch_crbal_drop_en;
}DPP_ETM_QMU_CFGMT_QSCH_CRBAL_DROP_EN_T;

typedef struct dpp_etm_qmu_cfgmt_wlist_qnum_fifo_aful_th_t
{
    ZXIC_UINT32 cfgmt_wlist_qnum_fifo_aful_th;
}DPP_ETM_QMU_CFGMT_WLIST_QNUM_FIFO_AFUL_TH_T;

typedef struct dpp_etm_qmu_qcfg_csw_pkt_blk_mode_t
{
    ZXIC_UINT32 qcfg_csw_pkt_blk_mode;
}DPP_ETM_QMU_QCFG_CSW_PKT_BLK_MODE_T;

typedef struct dpp_etm_qmu_qcfg_qlist_ram_init_cancel_t
{
    ZXIC_UINT32 qcfg_qlist_ram_init_cancel;
}DPP_ETM_QMU_QCFG_QLIST_RAM_INIT_CANCEL_T;

typedef struct dpp_etm_qmu_qcfg_qsch_crbal_transfer_mode_t
{
    ZXIC_UINT32 qcfg_qsch_crbal_transfer_mode;
    ZXIC_UINT32 qcfg_qsch_crbal_transfer_value;
}DPP_ETM_QMU_QCFG_QSCH_CRBAL_TRANSFER_MODE_T;

typedef struct dpp_etm_qmu_qcfg_qlist_qclr_interval_t
{
    ZXIC_UINT32 qcfg_qlist_qclr_interval;
}DPP_ETM_QMU_QCFG_QLIST_QCLR_INTERVAL_T;

typedef struct dpp_etm_qmu_qcfg_qsch_qclr_rate_t
{
    ZXIC_UINT32 qcfg_qsch_qclr_rate;
}DPP_ETM_QMU_QCFG_QSCH_QCLR_RATE_T;

typedef struct dpp_etm_qmu_qcfg_qlist_ddr_random_t
{
    ZXIC_UINT32 qcfg_qlist_ddr_random;
}DPP_ETM_QMU_QCFG_QLIST_DDR_RANDOM_T;

typedef struct dpp_etm_qmu_cfgmt_qlist_pds_fifo_afull_th_t
{
    ZXIC_UINT32 cfgmt_qlist_pds_fifo_afull_th;
}DPP_ETM_QMU_CFGMT_QLIST_PDS_FIFO_AFULL_TH_T;

typedef struct dpp_etm_qmu_cfgmt_sop_cmd_fifo_afull_th_t
{
    ZXIC_UINT32 cfgmt_sop_cmd_fifo_afull_th;
}DPP_ETM_QMU_CFGMT_SOP_CMD_FIFO_AFULL_TH_T;

typedef struct dpp_etm_qmu_cfgmt_non_sop_cmd_fifo_afull_th_t
{
    ZXIC_UINT32 cfgmt_non_sop_cmd_fifo_afull_th;
}DPP_ETM_QMU_CFGMT_NON_SOP_CMD_FIFO_AFULL_TH_T;

typedef struct dpp_etm_qmu_cfgmt_mmu_data_fifo_afull_th_t
{
    ZXIC_UINT32 cfgmt_mmu_data_fifo_afull_th;
}DPP_ETM_QMU_CFGMT_MMU_DATA_FIFO_AFULL_TH_T;

typedef struct dpp_etm_qmu_qcfg_qlist_bank_ept_th_t
{
    ZXIC_UINT32 qcfg_qlist_bank_ept_th;
}DPP_ETM_QMU_QCFG_QLIST_BANK_EPT_TH_T;

typedef struct dpp_etm_qmu_random_bypass_en_t
{
    ZXIC_UINT32 random_bypass_en;
}DPP_ETM_QMU_RANDOM_BYPASS_EN_T;

typedef struct dpp_etm_qmu_cfgmt_crs_spd_bypass_t
{
    ZXIC_UINT32 cfgmt_crs_spd_bypass;
}DPP_ETM_QMU_CFGMT_CRS_SPD_BYPASS_T;

typedef struct dpp_etm_qmu_cfgmt_crs_interval_t
{
    ZXIC_UINT32 cfgmt_crs_interval;
}DPP_ETM_QMU_CFGMT_CRS_INTERVAL_T;

typedef struct dpp_etm_qmu_cfg_qsch_auto_credit_control_en_t
{
    ZXIC_UINT32 cfg_qsch_auto_credit_control_en;
}DPP_ETM_QMU_CFG_QSCH_AUTO_CREDIT_CONTROL_EN_T;

typedef struct dpp_etm_qmu_cfg_qsch_autocrfrstque_t
{
    ZXIC_UINT32 cfg_qsch_autocrfrstque;
}DPP_ETM_QMU_CFG_QSCH_AUTOCRFRSTQUE_T;

typedef struct dpp_etm_qmu_cfg_qsch_autocrlastque_t
{
    ZXIC_UINT32 cfg_qsch_autocrlastque;
}DPP_ETM_QMU_CFG_QSCH_AUTOCRLASTQUE_T;

typedef struct dpp_etm_qmu_cfg_qsch_autocreditrate_t
{
    ZXIC_UINT32 cfg_qsch_autocreditrate;
}DPP_ETM_QMU_CFG_QSCH_AUTOCREDITRATE_T;

typedef struct dpp_etm_qmu_cfg_qsch_scanfrstque_t
{
    ZXIC_UINT32 cfg_qsch_scanfrstque;
}DPP_ETM_QMU_CFG_QSCH_SCANFRSTQUE_T;

typedef struct dpp_etm_qmu_cfg_qsch_scanlastque_t
{
    ZXIC_UINT32 cfg_qsch_scanlastque;
}DPP_ETM_QMU_CFG_QSCH_SCANLASTQUE_T;

typedef struct dpp_etm_qmu_cfg_qsch_scanrate_t
{
    ZXIC_UINT32 cfg_qsch_scanrate;
}DPP_ETM_QMU_CFG_QSCH_SCANRATE_T;

typedef struct dpp_etm_qmu_cfg_qsch_scan_en_t
{
    ZXIC_UINT32 cfg_qsch_scan_en;
}DPP_ETM_QMU_CFG_QSCH_SCAN_EN_T;

typedef struct dpp_etm_qmu_cfgmt_qsch_rd_credit_fifo_rate_t
{
    ZXIC_UINT32 cfgmt_qsch_rd_credit_fifo_rate;
}DPP_ETM_QMU_CFGMT_QSCH_RD_CREDIT_FIFO_RATE_T;

typedef struct dpp_etm_qmu_qcfg_qlist_bdep_t
{
    ZXIC_UINT32 qcfg_qlist_bdep;
}DPP_ETM_QMU_QCFG_QLIST_BDEP_T;

typedef struct dpp_etm_qmu_qcfg_qlist_bhead_t
{
    ZXIC_UINT32 bank_vld;
    ZXIC_UINT32 qcfg_qlist_bhead;
}DPP_ETM_QMU_QCFG_QLIST_BHEAD_T;

typedef struct dpp_etm_qmu_qcfg_qlist_btail_t
{
    ZXIC_UINT32 qcfg_qlist_btail;
}DPP_ETM_QMU_QCFG_QLIST_BTAIL_T;

typedef struct dpp_etm_qmu_qcfg_qsch_shap_param_t
{
    ZXIC_UINT32 qcfg_qsch_shap_en;
    ZXIC_UINT32 qcfg_qsch_shap_param1;
    ZXIC_UINT32 qcfg_qsch_shap_param2;
}DPP_ETM_QMU_QCFG_QSCH_SHAP_PARAM_T;

typedef struct dpp_etm_qmu_qcfg_qsch_shap_token_t
{
    ZXIC_UINT32 qcfg_qsch_shap_token;
}DPP_ETM_QMU_QCFG_QSCH_SHAP_TOKEN_T;

typedef struct dpp_etm_qmu_qcfg_qsch_shap_offset_t
{
    ZXIC_UINT32 qcfg_qsch_shap_offset;
}DPP_ETM_QMU_QCFG_QSCH_SHAP_OFFSET_T;

typedef struct dpp_etm_qmu_qcfg_qsch_crs_eir_th_t
{
    ZXIC_UINT32 qcfg_qsch_crs_eir_th;
}DPP_ETM_QMU_QCFG_QSCH_CRS_EIR_TH_T;

typedef struct dpp_etm_qmu_qcfg_qsch_crs_th1_t
{
    ZXIC_UINT32 qcfg_qsch_crs_th1;
}DPP_ETM_QMU_QCFG_QSCH_CRS_TH1_T;

typedef struct dpp_etm_qmu_qcfg_qsch_crs_th2_t
{
    ZXIC_UINT32 qcfg_qsch_crs_th2;
}DPP_ETM_QMU_QCFG_QSCH_CRS_TH2_T;

typedef struct dpp_etm_qmu_qcfg_csch_congest_th_t
{
    ZXIC_UINT32 qcfg_csch_congest_th;
}DPP_ETM_QMU_QCFG_CSCH_CONGEST_TH_T;

typedef struct dpp_etm_qmu_qcfg_csch_sp_fc_th_t
{
    ZXIC_UINT32 qcfg_csch_sp_fc_th;
}DPP_ETM_QMU_QCFG_CSCH_SP_FC_TH_T;

typedef struct dpp_etm_qmu_qcfg_csw_shap_parameter_t
{
    ZXIC_UINT32 qcfg_csw_shap_en;
    ZXIC_UINT32 qcfg_csw_shap_parameter;
}DPP_ETM_QMU_QCFG_CSW_SHAP_PARAMETER_T;

typedef struct dpp_etm_qmu_cfgmt_rd_release_aful_th_t
{
    ZXIC_UINT32 cfgmt_rd_release_aful_th;
}DPP_ETM_QMU_CFGMT_RD_RELEASE_AFUL_TH_T;

typedef struct dpp_etm_qmu_cfgmt_drop_imem_release_fifo_aful_th_t
{
    ZXIC_UINT32 cfgmt_drop_imem_release_fifo_aful_th;
}DPP_ETM_QMU_CFGMT_DROP_IMEM_RELEASE_FIFO_AFUL_TH_T;

typedef struct dpp_etm_qmu_cfgmt_nnh_rd_buf_aful_th_t
{
    ZXIC_UINT32 cfgmt_nnh_rd_buf_aful_th;
}DPP_ETM_QMU_CFGMT_NNH_RD_BUF_AFUL_TH_T;

typedef struct dpp_etm_qmu_cfg_pid_use_inall_t
{
    ZXIC_UINT32 cfgmt_nod_rd_buf_0_aful_th;
}DPP_ETM_QMU_CFG_PID_USE_INALL_T;

typedef struct dpp_etm_qmu_cfg_pid_round_th_t
{
    ZXIC_UINT32 cfgmt_nod_rd_buf_1_aful_th;
}DPP_ETM_QMU_CFG_PID_ROUND_TH_T;

typedef struct dpp_etm_qmu_cfgmt_credit_fifo_afull_th_t
{
    ZXIC_UINT32 cfgmt_credit_fifo_afull_th;
}DPP_ETM_QMU_CFGMT_CREDIT_FIFO_AFULL_TH_T;

typedef struct dpp_etm_qmu_cfgmt_scan_fifo_afull_th_t
{
    ZXIC_UINT32 cfgmt_scan_fifo_afull_th;
}DPP_ETM_QMU_CFGMT_SCAN_FIFO_AFULL_TH_T;

typedef struct dpp_etm_qmu_cfgmt_small_fifo_aful_th_t
{
    ZXIC_UINT32 cfgmt_small_fifo_aful_th;
}DPP_ETM_QMU_CFGMT_SMALL_FIFO_AFUL_TH_T;

typedef struct dpp_etm_qmu_cfgmt_free_addr_fifo_aful_th_t
{
    ZXIC_UINT32 cfgmt_free_addr_fifo_aful_th;
}DPP_ETM_QMU_CFGMT_FREE_ADDR_FIFO_AFUL_TH_T;

typedef struct dpp_etm_qmu_cfgmt_enq_rpt_fifo_aful_th_t
{
    ZXIC_UINT32 cfgmt_enq_rpt_fifo_aful_th;
}DPP_ETM_QMU_CFGMT_ENQ_RPT_FIFO_AFUL_TH_T;

typedef struct dpp_etm_qmu_qcfg_csw_shap_token_depth_t
{
    ZXIC_UINT32 qcfg_csw_shap_token_depth;
}DPP_ETM_QMU_QCFG_CSW_SHAP_TOKEN_DEPTH_T;

typedef struct dpp_etm_qmu_qcfg_csw_shap_offset_value_t
{
    ZXIC_UINT32 qcfg_csw_shap_offset_value;
}DPP_ETM_QMU_QCFG_CSW_SHAP_OFFSET_VALUE_T;

typedef struct dpp_etm_qmu_qcfg_csw_fc_offset_value_t
{
    ZXIC_UINT32 qcfg_csw_fc_offset_value;
}DPP_ETM_QMU_QCFG_CSW_FC_OFFSET_VALUE_T;

typedef struct dpp_etm_qmu_qmu_init_done_state_t
{
    ZXIC_UINT32 csch_qcfg_init_done;
    ZXIC_UINT32 qsch_qcfg_init_done;
    ZXIC_UINT32 qlist_qcfg_init_done;
    ZXIC_UINT32 qcsr_ram_init_done;
}DPP_ETM_QMU_QMU_INIT_DONE_STATE_T;

typedef struct dpp_etm_qmu_csw_qcfg_port_shap_rdy_0_t
{
    ZXIC_UINT32 csw_qcfg_port_shap_rdy_0;
}DPP_ETM_QMU_CSW_QCFG_PORT_SHAP_RDY_0_T;

typedef struct dpp_etm_qmu_csw_qcfg_port_shap_rdy_1_t
{
    ZXIC_UINT32 csw_qcfg_port_shap_rdy_1;
}DPP_ETM_QMU_CSW_QCFG_PORT_SHAP_RDY_1_T;

typedef struct dpp_etm_qmu_qlist_cfgmt_ram_init_done_t
{
    ZXIC_UINT32 qlist_qcfg_qds_ram_init_done;
    ZXIC_UINT32 qlist_qcfg_chk_ram_init_done;
    ZXIC_UINT32 qlist_qcfg_ept_ram_init_done;
    ZXIC_UINT32 qlist_qcfg_cti_ram_init_done;
    ZXIC_UINT32 qlist_qcfg_cto_ram_init_done;
    ZXIC_UINT32 qlist_qcfg_bcnt_ram_init_done;
    ZXIC_UINT32 qlist_qcfg_biu_ram_init_done;
    ZXIC_UINT32 qlist_qcfg_baram_init_done;
}DPP_ETM_QMU_QLIST_CFGMT_RAM_INIT_DONE_T;

typedef struct dpp_etm_qmu_qlist_cfgmt_ram_ecc_err_t
{
    ZXIC_UINT32 qds_ram_parity_err;
    ZXIC_UINT32 qcsr_qnum_fifo_parity_err;
    ZXIC_UINT32 sa_id_ram_parity_err;
    ZXIC_UINT32 enq_rpt_fifo_parity_err;
    ZXIC_UINT32 bcnts_parity_err;
    ZXIC_UINT32 baram_parity_err_a;
    ZXIC_UINT32 baram_parity_err_b;
    ZXIC_UINT32 bcntm_ram_parity_err;
    ZXIC_UINT32 biu_ram_single_ecc_err;
    ZXIC_UINT32 chk_ram_single_ecc_err;
    ZXIC_UINT32 cmd_sch_cmd_ram_single_ecc_err;
    ZXIC_UINT32 cmd_sch_list_ram_single_ecc_err;
    ZXIC_UINT32 cmd_sch_hp_ram_single_ecc_err;
    ZXIC_UINT32 cmd_sch_tp_ram_single_ecc_err;
    ZXIC_UINT32 cmd_sch_enq_active_ram_single_ecc_err;
    ZXIC_UINT32 cmd_sch_deq_active_ram_single_ecc_err;
    ZXIC_UINT32 cmd_sch_empty_ram_single_ecc_err;
    ZXIC_UINT32 cmd_sch_eop_ram_single_ecc_err;
    ZXIC_UINT32 cmd_sch_blkcnt_ram_single_ecc_err;
    ZXIC_UINT32 biu_ram_double_ecc_err;
    ZXIC_UINT32 chk_ram_double_ecc_err;
    ZXIC_UINT32 cmd_sch_cmd_ram_double_ecc_err;
    ZXIC_UINT32 cmd_sch_list_ram_double_ecc_err;
    ZXIC_UINT32 cmd_sch_hp_ram_double_ecc_err;
    ZXIC_UINT32 cmd_sch_tp_ram_double_ecc_err;
    ZXIC_UINT32 cmd_sch_enq_active_ram_double_ecc_err;
    ZXIC_UINT32 cmd_sch_deq_active_ram_double_ecc_err;
    ZXIC_UINT32 cmd_sch_empty_ram_double_ecc_err;
    ZXIC_UINT32 cmd_sch_eop_ram_double_ecc_err;
    ZXIC_UINT32 cmd_sch_blkcnt_ram_double_ecc_err;
}DPP_ETM_QMU_QLIST_CFGMT_RAM_ECC_ERR_T;

typedef struct dpp_etm_qmu_qlist_cfgmt_ram_slot_err_t
{
    ZXIC_UINT32 qds_ram_enq_rd_slot_err;
    ZXIC_UINT32 qds_ram_deq_rd_slot_err;
    ZXIC_UINT32 qds_ram_enq_wr_slot_err;
    ZXIC_UINT32 qds_ram_deq_wr_slot_err;
    ZXIC_UINT32 chk_ram_enq_rd_slot_err;
    ZXIC_UINT32 chk_ram_deq_rd_slot_err;
    ZXIC_UINT32 chk_ram_enq_wr_slot_err;
    ZXIC_UINT32 chk_ram_deq_wr_slot_err;
    ZXIC_UINT32 ept_ram_enq_rd_slot_err;
    ZXIC_UINT32 ept_ram_deq_rd_slot_err;
    ZXIC_UINT32 ept_ram_enq_wr_slot_err;
    ZXIC_UINT32 ept_ram_deq_wr_slot_err;
    ZXIC_UINT32 cti_ram_enq_rd_slot_err;
    ZXIC_UINT32 cti_ram_deq_rd_slot_err;
    ZXIC_UINT32 cti_ram_enq_wr_slot_err;
    ZXIC_UINT32 cti_ram_deq_wr_slot_err;
    ZXIC_UINT32 cto_ram_enq_rd_slot_err;
    ZXIC_UINT32 cto_ram_deq_rd_slot_err;
    ZXIC_UINT32 cto_ram_enq_wr_slot_err;
    ZXIC_UINT32 cto_ram_deq_wr_slot_err;
}DPP_ETM_QMU_QLIST_CFGMT_RAM_SLOT_ERR_T;

typedef struct dpp_etm_qmu_qsch_cfgmt_ram_ecc_t
{
    ZXIC_UINT32 crbal_rama_parity_error;
    ZXIC_UINT32 crbal_ramb_parity_error;
    ZXIC_UINT32 crs_ram_parity_error;
    ZXIC_UINT32 wlist_flag_ram_single_ecc_err;
    ZXIC_UINT32 wlist_next_single_ecc_err;
    ZXIC_UINT32 wlist_wactive_ram_single_ecc_err;
    ZXIC_UINT32 wlist_ractive_ram_single_ecc_err;
    ZXIC_UINT32 wlist_tp1_ram_single_ecc_err;
    ZXIC_UINT32 wlist_tp2_ram_single_ecc_err;
    ZXIC_UINT32 wlist_empty1_ram_single_ecc_err_a;
    ZXIC_UINT32 wlist_empty1_ram_single_ecc_err_b;
    ZXIC_UINT32 wlist_empty2_ram_single_ecc_err_a;
    ZXIC_UINT32 wlist_empty2_ram_single_ecc_err_b;
    ZXIC_UINT32 wlist_hp_ram_single_ecc_err_a;
    ZXIC_UINT32 wlist_hp_ram_single_ecc_err_b;
    ZXIC_UINT32 wlist_flag_ram_double_ecc_err;
    ZXIC_UINT32 wlist_next_double_ecc_err;
    ZXIC_UINT32 wlist_wactive_ram_double_ecc_err;
    ZXIC_UINT32 wlist_ractive_ram_double_ecc_err;
    ZXIC_UINT32 wlist_tp1_ram_double_ecc_err;
    ZXIC_UINT32 wlist_tp2_ram_double_ecc_err;
    ZXIC_UINT32 wlist_empty1_ram_double_ecc_err_a;
    ZXIC_UINT32 wlist_empty1_ram_double_ecc_err_b;
    ZXIC_UINT32 wlist_empty2_ram_double_ecc_err_a;
    ZXIC_UINT32 wlist_empty2_ram_double_ecc_err_b;
    ZXIC_UINT32 wlist_hp_ram_double_ecc_err_a;
    ZXIC_UINT32 wlist_hp_ram_double_ecc_err_b;
}DPP_ETM_QMU_QSCH_CFGMT_RAM_ECC_T;

typedef struct dpp_etm_qmu_qlist_cfgmt_fifo_state_t
{
    ZXIC_UINT32 pkt_age_req_fifo_overflow;
    ZXIC_UINT32 pkt_age_req_fifo_underflow;
    ZXIC_UINT32 qcsr_big_fifo_ovfl;
    ZXIC_UINT32 qcsr_small_fifo_overflow;
    ZXIC_UINT32 enq_rpt_fifo_overflow;
    ZXIC_UINT32 enq_rpt_fifo_underflow;
    ZXIC_UINT32 pds_fwft_overflow;
    ZXIC_UINT32 pds_fwft_underflow;
    ZXIC_UINT32 free_addr_fifo_overflow;
    ZXIC_UINT32 free_addr_fifo_underflow;
    ZXIC_UINT32 rd_release_fwft_overflow;
    ZXIC_UINT32 rd_release_fwft_underflow;
    ZXIC_UINT32 pid_free_list_overflow;
    ZXIC_UINT32 pid_free_list_underflow;
    ZXIC_UINT32 pid_prp_list_overflow;
    ZXIC_UINT32 pid_prp_list_underflow;
    ZXIC_UINT32 pid_rdy_list_overflow;
    ZXIC_UINT32 pid_rdy_list_underflow;
    ZXIC_UINT32 drop_imem_release_fwft_overflow;
    ZXIC_UINT32 drop_imem_release_fwft_underflow;
    ZXIC_UINT32 nnh_rd_buf_fifo_overflow;
    ZXIC_UINT32 nnh_rd_buf_fifo_underflow;
    ZXIC_UINT32 nod_rd_buf_0_fifo_overflow;
    ZXIC_UINT32 nod_rd_buf_0_fifo_underflow;
    ZXIC_UINT32 nod_rd_buf_1_fifo_overflow;
    ZXIC_UINT32 nod_rd_buf_1_fifo_underflow;
}DPP_ETM_QMU_QLIST_CFGMT_FIFO_STATE_T;

typedef struct dpp_etm_qmu_qlist_qcfg_clr_done_t
{
    ZXIC_UINT32 qlist_qcfg_clr_done;
}DPP_ETM_QMU_QLIST_QCFG_CLR_DONE_T;

typedef struct dpp_etm_qmu_qmu_int_mask1_t
{
    ZXIC_UINT32 qmu_int_mask1;
}DPP_ETM_QMU_QMU_INT_MASK1_T;

typedef struct dpp_etm_qmu_qmu_int_mask2_t
{
    ZXIC_UINT32 qmu_int_mask2;
}DPP_ETM_QMU_QMU_INT_MASK2_T;

typedef struct dpp_etm_qmu_qmu_int_mask3_t
{
    ZXIC_UINT32 qmu_int_mask3;
}DPP_ETM_QMU_QMU_INT_MASK3_T;

typedef struct dpp_etm_qmu_qmu_int_mask4_t
{
    ZXIC_UINT32 qmu_int_mask4;
}DPP_ETM_QMU_QMU_INT_MASK4_T;

typedef struct dpp_etm_qmu_qmu_int_mask5_t
{
    ZXIC_UINT32 qmu_int_mask5;
}DPP_ETM_QMU_QMU_INT_MASK5_T;

typedef struct dpp_etm_qmu_qmu_int_mask6_t
{
    ZXIC_UINT32 qmu_int_mask6;
}DPP_ETM_QMU_QMU_INT_MASK6_T;

typedef struct dpp_etm_qmu_cmd_sch_cfgmt_fifo_state_t
{
    ZXIC_UINT32 nsop_fifo_parity_err;
    ZXIC_UINT32 cmdsch_rd_cmd_fifo_parity_err;
    ZXIC_UINT32 sop_fifo_afull;
    ZXIC_UINT32 sop_fifo_empty;
    ZXIC_UINT32 sop_fifo_overflow;
    ZXIC_UINT32 sop_fifo_underflow;
    ZXIC_UINT32 mmu_data_fifo_afull;
    ZXIC_UINT32 mmu_data_fifo_empty;
    ZXIC_UINT32 mmudat_fifo_overflow;
    ZXIC_UINT32 mmudat_fifo_underflow;
    ZXIC_UINT32 non_sop_fifo_afull;
    ZXIC_UINT32 non_sop_fifo_empty;
    ZXIC_UINT32 nsop_fifo_overflow;
    ZXIC_UINT32 nsop_fifo_underflow;
    ZXIC_UINT32 cmdsch_rd_cmd_fifo_afull;
    ZXIC_UINT32 cmdsch_rd_cmd_fifo_empty;
    ZXIC_UINT32 cmdsch_rd_cmd_fifo_overflow;
    ZXIC_UINT32 cmdsch_rd_cmd_fifo_underflow;
    ZXIC_UINT32 wlist_qnum_fifo_overflow;
    ZXIC_UINT32 wlist_qnum_fifo_underflow;
    ZXIC_UINT32 qsch_scan_fifo_overflow;
    ZXIC_UINT32 qsch_scan_fifo_underflow;
    ZXIC_UINT32 qsch_credit_fifo_overflow;
    ZXIC_UINT32 qsch_credit_fifo_underflow;
    ZXIC_UINT32 qsch_credit_fifo2_overflow;
    ZXIC_UINT32 qsch_credit_fifo2_underflow;
}DPP_ETM_QMU_CMD_SCH_CFGMT_FIFO_STATE_T;

typedef struct dpp_etm_qmu_qlist_r_bcnt_t
{
    ZXIC_UINT32 qlist_r_bcnt;
}DPP_ETM_QMU_QLIST_R_BCNT_T;

typedef struct dpp_etm_qmu_qsch_rw_crbal_t
{
    ZXIC_UINT32 qsch_rw_crbal;
}DPP_ETM_QMU_QSCH_RW_CRBAL_T;

typedef struct dpp_etm_qmu_qsch_rw_crs_t
{
    ZXIC_UINT32 qsch_rw_crs;
}DPP_ETM_QMU_QSCH_RW_CRS_T;

typedef struct dpp_etm_qmu_qsch_r_wlist_empty_t
{
    ZXIC_UINT32 qsch_r_wlist_empty;
}DPP_ETM_QMU_QSCH_R_WLIST_EMPTY_T;

typedef struct dpp_etm_qmu_qcfg_qlist_baram_rd_t
{
    ZXIC_UINT32 qcfg_qlist_baram_rd;
}DPP_ETM_QMU_QCFG_QLIST_BARAM_RD_T;

typedef struct dpp_etm_qmu_qcfg_qsch_crbal_fb_rw_t
{
    ZXIC_UINT32 qcfg_qlist_crbal_fb_rw;
}DPP_ETM_QMU_QCFG_QSCH_CRBAL_FB_RW_T;

typedef struct dpp_etm_qmu_qcfg_qlist_grp0_bank_t
{
    ZXIC_UINT32 qcfg_qlist_grp0_bank_wr;
}DPP_ETM_QMU_QCFG_QLIST_GRP0_BANK_T;

typedef struct dpp_etm_qmu_qcfg_qlist_grp1_bank_t
{
    ZXIC_UINT32 qcfg_qlist_grp1_bank_wr;
}DPP_ETM_QMU_QCFG_QLIST_GRP1_BANK_T;

typedef struct dpp_etm_qmu_qcfg_qlist_grp2_bank_t
{
    ZXIC_UINT32 qcfg_qlist_grp2_bank_wr;
}DPP_ETM_QMU_QCFG_QLIST_GRP2_BANK_T;

typedef struct dpp_etm_qmu_qcfg_qlist_grp3_bank_t
{
    ZXIC_UINT32 qcfg_qlist_grp3_bank_wr;
}DPP_ETM_QMU_QCFG_QLIST_GRP3_BANK_T;

typedef struct dpp_etm_qmu_qcfg_qlist_grp4_bank_t
{
    ZXIC_UINT32 qcfg_qlist_grp4_bank_wr;
}DPP_ETM_QMU_QCFG_QLIST_GRP4_BANK_T;

typedef struct dpp_etm_qmu_qcfg_qlist_grp5_bank_t
{
    ZXIC_UINT32 qcfg_qlist_grp5_bank_wr;
}DPP_ETM_QMU_QCFG_QLIST_GRP5_BANK_T;

typedef struct dpp_etm_qmu_qcfg_qlist_grp6_bank_t
{
    ZXIC_UINT32 qcfg_qlist_grp6_bank_wr;
}DPP_ETM_QMU_QCFG_QLIST_GRP6_BANK_T;

typedef struct dpp_etm_qmu_qcfg_qlist_grp7_bank_t
{
    ZXIC_UINT32 qcfg_qlist_grp7_bank_wr;
}DPP_ETM_QMU_QCFG_QLIST_GRP7_BANK_T;

typedef struct dpp_etm_qmu_qcfg_qlist_grp_t
{
    ZXIC_UINT32 qcfg_qlist_grp_wr;
}DPP_ETM_QMU_QCFG_QLIST_GRP_T;

typedef struct dpp_etm_qmu_cfgmt_active_to_bank_cfg_t
{
    ZXIC_UINT32 cfgmt_active_to_bank_cfg;
}DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T;

typedef struct dpp_etm_qmu_cfgmt_ddr_in_mmu_cfg_t
{
    ZXIC_UINT32 cfgmt_ddr_in_mmu_cfg;
}DPP_ETM_QMU_CFGMT_DDR_IN_MMU_CFG_T;

typedef struct dpp_etm_qmu_cfgmt_ddr_in_qmu_cfg_t
{
    ZXIC_UINT32 cfgmt_ddr_in_qmu_cfg;
}DPP_ETM_QMU_CFGMT_DDR_IN_QMU_CFG_T;

typedef struct dpp_etm_qmu_cfgmt_bank_to_mmu_cfg_t
{
    ZXIC_UINT32 cfgmt_bank_in_mmu_cfg;
}DPP_ETM_QMU_CFGMT_BANK_TO_MMU_CFG_T;

typedef struct dpp_etm_qmu_cfgmt_bank_to_qmu_cfg_t
{
    ZXIC_UINT32 cfgmt_bank_in_qmu_cfg;
}DPP_ETM_QMU_CFGMT_BANK_TO_QMU_CFG_T;

typedef struct dpp_etm_qmu_cfgmt_grp_ram_n_clr_thd_t
{
    ZXIC_UINT32 cfgmt_grp_ram_n_clr_thd;
}DPP_ETM_QMU_CFGMT_GRP_RAM_N_CLR_THD_T;

typedef struct dpp_etm_qmu_cfgmt_age_pkt_num_t
{
    ZXIC_UINT32 cfgmt_age_pkt_num;
}DPP_ETM_QMU_CFGMT_AGE_PKT_NUM_T;

typedef struct dpp_etm_qmu_cfgmt_age_multi_interval_t
{
    ZXIC_UINT32 cfgmt_age_multi_interval;
}DPP_ETM_QMU_CFGMT_AGE_MULTI_INTERVAL_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_pkt_age_en_t
{
    ZXIC_UINT32 cfgmt_qmu_pkt_age_en;
}DPP_ETM_QMU_CFGMT_QMU_PKT_AGE_EN_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_pkt_age_interval_t
{
    ZXIC_UINT32 cfgmt_qmu_pkt_age_interval;
}DPP_ETM_QMU_CFGMT_QMU_PKT_AGE_INTERVAL_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_pkt_age_start_end_t
{
    ZXIC_UINT32 cfgmt_qmu_pkt_age_end;
    ZXIC_UINT32 cfgmt_qmu_pkt_age_start;
}DPP_ETM_QMU_CFGMT_QMU_PKT_AGE_START_END_T;

typedef struct dpp_etm_qmu_cfgmt_pkt_age_req_aful_th_t
{
    ZXIC_UINT32 cfgmt_pkt_age_req_aful_th;
}DPP_ETM_QMU_CFGMT_PKT_AGE_REQ_AFUL_TH_T;

typedef struct dpp_etm_qmu_cfgmt_pkt_age_step_interval_t
{
    ZXIC_UINT32 cfgmt_pkt_age_step_interval;
}DPP_ETM_QMU_CFGMT_PKT_AGE_STEP_INTERVAL_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_imem_age_mode_t
{
    ZXIC_UINT32 cfgmt_qmu_imem_age_en;
    ZXIC_UINT32 cfgmt_qmu_imem_age_qlen_en;
    ZXIC_UINT32 cfgmt_qmu_imem_age_time_en;
}DPP_ETM_QMU_CFGMT_QMU_IMEM_AGE_MODE_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_imem_qlen_age_interval_t
{
    ZXIC_UINT32 cfgmt_qmu_imem_qlen_age_interval;
}DPP_ETM_QMU_CFGMT_QMU_IMEM_QLEN_AGE_INTERVAL_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_imem_time_age_interval_t
{
    ZXIC_UINT32 cfgmt_qmu_imem_time_age_interval;
}DPP_ETM_QMU_CFGMT_QMU_IMEM_TIME_AGE_INTERVAL_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_imem_qlen_age_thd_t
{
    ZXIC_UINT32 cfgmt_qmu_imem_qlen_age_thd;
}DPP_ETM_QMU_CFGMT_QMU_IMEM_QLEN_AGE_THD_T;

typedef struct dpp_etm_qmu_cfgmt_imem_age_step_interval_t
{
    ZXIC_UINT32 cfgmt_imem_age_step_interval;
}DPP_ETM_QMU_CFGMT_IMEM_AGE_STEP_INTERVAL_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_ecc_bypass_read_t
{
    ZXIC_UINT32 cfgmt_qmu_ecc_bypass_read;
}DPP_ETM_QMU_CFGMT_QMU_ECC_BYPASS_READ_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_resp_stat_fc_en_t
{
    ZXIC_UINT32 cfgmt_qmu_resp_stat_fc_en;
}DPP_ETM_QMU_CFGMT_QMU_RESP_STAT_FC_EN_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_bank_xoff_pds_mode_t
{
    ZXIC_UINT32 cfgmt_qmu_bank_xoff_pds_mode;
}DPP_ETM_QMU_CFGMT_QMU_BANK_XOFF_PDS_MODE_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_stat_offset_t
{
    ZXIC_UINT32 cfgmt_qmu_stat_offset;
}DPP_ETM_QMU_CFGMT_QMU_STAT_OFFSET_T;

typedef struct dpp_etm_qmu_fc_cnt_mode_t
{
    ZXIC_UINT32 fc_cnt_mode;
}DPP_ETM_QMU_FC_CNT_MODE_T;

typedef struct dpp_etm_qmu_mmu_qmu_wr_fc_cnt_t
{
    ZXIC_UINT32 mmu_qmu_wr_fc_cnt;
}DPP_ETM_QMU_MMU_QMU_WR_FC_CNT_T;

typedef struct dpp_etm_qmu_mmu_qmu_rd_fc_cnt_t
{
    ZXIC_UINT32 mmu_qmu_rd_fc_cnt;
}DPP_ETM_QMU_MMU_QMU_RD_FC_CNT_T;

typedef struct dpp_etm_qmu_qmu_cgavd_fc_cnt_t
{
    ZXIC_UINT32 qmu_cgavd_fc_cnt;
}DPP_ETM_QMU_QMU_CGAVD_FC_CNT_T;

typedef struct dpp_etm_qmu_cgavd_qmu_pkt_cnt_t
{
    ZXIC_UINT32 cgavd_qmu_pkt_cnt;
}DPP_ETM_QMU_CGAVD_QMU_PKT_CNT_T;

typedef struct dpp_etm_qmu_cgavd_qmu_pktlen_all_t
{
    ZXIC_UINT32 cgavd_qmu_pktlen_all;
}DPP_ETM_QMU_CGAVD_QMU_PKTLEN_ALL_T;

typedef struct dpp_etm_qmu_observe_portfc_spec_t
{
    ZXIC_UINT32 observe_portfc_spec;
}DPP_ETM_QMU_OBSERVE_PORTFC_SPEC_T;

typedef struct dpp_etm_qmu_spec_lif_portfc_count_t
{
    ZXIC_UINT32 spec_lif_portfc_count;
}DPP_ETM_QMU_SPEC_LIF_PORTFC_COUNT_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_pfc_en_t
{
    ZXIC_UINT32 cfgmt_qmu_pfc_en;
}DPP_ETM_QMU_CFGMT_QMU_PFC_EN_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_pfc_mask_1_t
{
    ZXIC_UINT32 cfgmt_qmu_pfc_mask_1;
}DPP_ETM_QMU_CFGMT_QMU_PFC_MASK_1_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_pfc_mask_2_t
{
    ZXIC_UINT32 cfgmt_qmu_pfc_mask_2;
}DPP_ETM_QMU_CFGMT_QMU_PFC_MASK_2_T;

typedef struct dpp_etm_cfgmt_chip_version_reg_t
{
    ZXIC_UINT32 chip_version_reg;
    ZXIC_UINT32 chip_sub_reg;
    ZXIC_UINT32 chip_type_reg;
}DPP_ETM_CFGMT_CHIP_VERSION_REG_T;

typedef struct dpp_etm_cfgmt_chip_date_reg_t
{
    ZXIC_UINT32 chip_date_reg;
}DPP_ETM_CFGMT_CHIP_DATE_REG_T;

typedef struct dpp_etm_cfgmt_cfgmt_crc_en_t
{
    ZXIC_UINT32 cfgmt_crc_en;
}DPP_ETM_CFGMT_CFGMT_CRC_EN_T;

typedef struct dpp_etm_cfgmt_cfg_port_transfer_en_t
{
    ZXIC_UINT32 cfg_port_transfer_en;
}DPP_ETM_CFGMT_CFG_PORT_TRANSFER_EN_T;

typedef struct dpp_etm_cfgmt_tm_sa_work_mode_t
{
    ZXIC_UINT32 tm_sa_work_mode;
}DPP_ETM_CFGMT_TM_SA_WORK_MODE_T;

typedef struct dpp_etm_cfgmt_local_sa_id_t
{
    ZXIC_UINT32 local_sa_id;
}DPP_ETM_CFGMT_LOCAL_SA_ID_T;

typedef struct dpp_etm_olif_olif_rdy_t
{
    ZXIC_UINT32 cfgmt_block_mode;
    ZXIC_UINT32 cfgmt_count_overflow_mode;
    ZXIC_UINT32 cfgmt_count_rd_mode;
    ZXIC_UINT32 olif_rdy;
}DPP_ETM_OLIF_OLIF_RDY_T;

typedef struct dpp_etm_olif_emem_prog_full_t
{
    ZXIC_UINT32 emem_prog_full_assert;
    ZXIC_UINT32 emem_prog_full_negate;
}DPP_ETM_OLIF_EMEM_PROG_FULL_T;

typedef struct dpp_etm_olif_port_order_fifo_full_t
{
    ZXIC_UINT32 port_order_fifo_full_assert;
    ZXIC_UINT32 port_order_fifo_full_negate;
}DPP_ETM_OLIF_PORT_ORDER_FIFO_FULL_T;

typedef struct dpp_etm_olif_olif_release_last_t
{
    ZXIC_UINT32 olif_release_last_addr;
    ZXIC_UINT32 olif_release_last_bank;
}DPP_ETM_OLIF_OLIF_RELEASE_LAST_T;

typedef struct dpp_etm_olif_olif_fifo_empty_state_t
{
    ZXIC_UINT32 qmu_para_fifo_empty;
    ZXIC_UINT32 emem_empty;
    ZXIC_UINT32 imem_empty;
}DPP_ETM_OLIF_OLIF_FIFO_EMPTY_STATE_T;

typedef struct dpp_etm_olif_qmu_olif_release_fc_cnt_t
{
    ZXIC_UINT32 qmu_olif_release_fc_cnt;
}DPP_ETM_OLIF_QMU_OLIF_RELEASE_FC_CNT_T;

typedef struct dpp_etm_olif_olif_qmu_link_fc_cnt_t
{
    ZXIC_UINT32 olif_qmu_link_fc_cnt;
}DPP_ETM_OLIF_OLIF_QMU_LINK_FC_CNT_T;

typedef struct dpp_etm_olif_lif0_link_fc_cnt_t
{
    ZXIC_UINT32 lif0_link_fc_cnt;
}DPP_ETM_OLIF_LIF0_LINK_FC_CNT_T;

typedef struct dpp_etm_olif_olif_tmmu_fc_cnt_t
{
    ZXIC_UINT32 olif_tmmu_fc_cnt;
}DPP_ETM_OLIF_OLIF_TMMU_FC_CNT_T;

typedef struct dpp_etm_olif_olif_mmu_fc_cnt_t
{
    ZXIC_UINT32 olif_mmu_fc_cnt;
}DPP_ETM_OLIF_OLIF_MMU_FC_CNT_T;

typedef struct dpp_etm_olif_olif_qmu_port_rdy_h_t
{
    ZXIC_UINT32 olif_qmu_port_rdy_h;
}DPP_ETM_OLIF_OLIF_QMU_PORT_RDY_H_T;

typedef struct dpp_etm_olif_olif_qmu_port_rdy_l_t
{
    ZXIC_UINT32 olif_qmu_port_rdy_l;
}DPP_ETM_OLIF_OLIF_QMU_PORT_RDY_L_T;

typedef struct dpp_etm_olif_lif0_port_rdy_h_t
{
    ZXIC_UINT32 lif0_port_rdy_h;
}DPP_ETM_OLIF_LIF0_PORT_RDY_H_T;

typedef struct dpp_etm_olif_lif0_port_rdy_l_t
{
    ZXIC_UINT32 lif0_port_rdy_l;
}DPP_ETM_OLIF_LIF0_PORT_RDY_L_T;

typedef struct dpp_etm_olif_qmu_olif_rd_sop_cnt_t
{
    ZXIC_UINT32 qmu_olif_rd_sop_cnt;
}DPP_ETM_OLIF_QMU_OLIF_RD_SOP_CNT_T;

typedef struct dpp_etm_olif_qmu_olif_rd_eop_cnt_t
{
    ZXIC_UINT32 qmu_olif_rd_eop_cnt;
}DPP_ETM_OLIF_QMU_OLIF_RD_EOP_CNT_T;

typedef struct dpp_etm_olif_qmu_olif_rd_vld_cnt_t
{
    ZXIC_UINT32 qmu_olif_rd_vld_cnt;
}DPP_ETM_OLIF_QMU_OLIF_RD_VLD_CNT_T;

typedef struct dpp_etm_olif_qmu_olif_rd_blk_cnt_t
{
    ZXIC_UINT32 qmu_olif_rd_blk_cnt;
}DPP_ETM_OLIF_QMU_OLIF_RD_BLK_CNT_T;

typedef struct dpp_etm_olif_mmu_tm_data_sop_cnt_t
{
    ZXIC_UINT32 mmu_tm_data_sop_cnt;
}DPP_ETM_OLIF_MMU_TM_DATA_SOP_CNT_T;

typedef struct dpp_etm_olif_mmu_tm_data_eop_cnt_t
{
    ZXIC_UINT32 mmu_tm_data_eop_cnt;
}DPP_ETM_OLIF_MMU_TM_DATA_EOP_CNT_T;

typedef struct dpp_etm_olif_mmu_tm_data_vld_cnt_t
{
    ZXIC_UINT32 mmu_tm_data_vld_cnt;
}DPP_ETM_OLIF_MMU_TM_DATA_VLD_CNT_T;

typedef struct dpp_etm_olif_odma_tm_data_sop_cnt_t
{
    ZXIC_UINT32 odma_tm_data_sop_cnt;
}DPP_ETM_OLIF_ODMA_TM_DATA_SOP_CNT_T;

typedef struct dpp_etm_olif_odma_tm_data_eop_cnt_t
{
    ZXIC_UINT32 odma_tm_data_eop_cnt;
}DPP_ETM_OLIF_ODMA_TM_DATA_EOP_CNT_T;

typedef struct dpp_etm_olif_odma_tm_deq_vld_cnt_t
{
    ZXIC_UINT32 odma_tm_deq_vld_cnt;
}DPP_ETM_OLIF_ODMA_TM_DEQ_VLD_CNT_T;

typedef struct dpp_etm_olif_olif_qmu_release_vld_cnt_t
{
    ZXIC_UINT32 olif_qmu_release_vld_cnt;
}DPP_ETM_OLIF_OLIF_QMU_RELEASE_VLD_CNT_T;

typedef struct dpp_etm_olif_emem_dat_vld_cnt_t
{
    ZXIC_UINT32 emem_dat_vld_cnt;
}DPP_ETM_OLIF_EMEM_DAT_VLD_CNT_T;

typedef struct dpp_etm_olif_imem_dat_vld_cnt_t
{
    ZXIC_UINT32 imem_dat_vld_cnt;
}DPP_ETM_OLIF_IMEM_DAT_VLD_CNT_T;

typedef struct dpp_etm_olif_emem_dat_rd_cnt_t
{
    ZXIC_UINT32 emem_dat_rd_cnt;
}DPP_ETM_OLIF_EMEM_DAT_RD_CNT_T;

typedef struct dpp_etm_olif_imem_dat_rd_cnt_t
{
    ZXIC_UINT32 imem_dat_rd_cnt;
}DPP_ETM_OLIF_IMEM_DAT_RD_CNT_T;

typedef struct dpp_etm_olif_qmu_olif_rd_sop_emem_cnt_t
{
    ZXIC_UINT32 qmu_olif_rd_sop_emem_cnt;
}DPP_ETM_OLIF_QMU_OLIF_RD_SOP_EMEM_CNT_T;

typedef struct dpp_etm_olif_qmu_olif_rd_vld_emem_cnt_t
{
    ZXIC_UINT32 qmu_olif_rd_vld_emem_cnt;
}DPP_ETM_OLIF_QMU_OLIF_RD_VLD_EMEM_CNT_T;

typedef struct dpp_etm_olif_cpu_last_wr_addr_t
{
    ZXIC_UINT32 cpu_last_wr_addr;
}DPP_ETM_OLIF_CPU_LAST_WR_ADDR_T;

typedef struct dpp_etm_olif_cpu_last_wr_data_t
{
    ZXIC_UINT32 cpu_last_wr_data;
}DPP_ETM_OLIF_CPU_LAST_WR_DATA_T;

typedef struct dpp_etm_olif_cpu_last_rd_addr_t
{
    ZXIC_UINT32 cpu_last_rd_addr;
}DPP_ETM_OLIF_CPU_LAST_RD_ADDR_T;

typedef struct dpp_etm_olif_qmu_olif_last_port_t
{
    ZXIC_UINT32 qmu_olif_last_port;
}DPP_ETM_OLIF_QMU_OLIF_LAST_PORT_T;

typedef struct dpp_etm_olif_qmu_olif_last_addr_t
{
    ZXIC_UINT32 qmu_olif_last_addr;
}DPP_ETM_OLIF_QMU_OLIF_LAST_ADDR_T;

typedef struct dpp_etm_olif_qmu_olif_last_bank_t
{
    ZXIC_UINT32 qmu_olif_last_bank;
}DPP_ETM_OLIF_QMU_OLIF_LAST_BANK_T;

typedef struct dpp_etm_olif_tm_lif_byte_stat_t
{
    ZXIC_UINT32 tm_lif_byte_stat;
}DPP_ETM_OLIF_TM_LIF_BYTE_STAT_T;

typedef struct dpp_etm_olif_tm_lif_err_stat_t
{
    ZXIC_UINT32 tm_lif_err_stat;
}DPP_ETM_OLIF_TM_LIF_ERR_STAT_T;

typedef struct dpp_etm_cgavd_port_share_cnt_t
{
    ZXIC_UINT32 port_share_cnt;
}DPP_ETM_CGAVD_PORT_SHARE_CNT_T;

typedef struct dpp_etm_cgavd_total_imem_cnt_t
{
    ZXIC_UINT32 total_imem_cnt;
}DPP_ETM_CGAVD_TOTAL_IMEM_CNT_T;

typedef struct dpp_etm_cgavd_pp_q_len_t
{
    ZXIC_UINT32 pp_q_len;
}DPP_ETM_CGAVD_PP_Q_LEN_T;

typedef struct dpp_etm_cgavd_sys_q_len_t
{
    ZXIC_UINT32 sys_q_len;
}DPP_ETM_CGAVD_SYS_Q_LEN_T;

typedef struct dpp_etm_cgavd_cgavd_cfg_error_warning_t
{
    ZXIC_UINT32 error_correction_11;
    ZXIC_UINT32 error_correction_10;
    ZXIC_UINT32 error_correction_9;
    ZXIC_UINT32 error_correction_8;
    ZXIC_UINT32 error_correction_7;
    ZXIC_UINT32 error_correction_6;
    ZXIC_UINT32 error_correction5;
    ZXIC_UINT32 error_correction_4;
    ZXIC_UINT32 error_correction_3;
    ZXIC_UINT32 error_correction_2;
    ZXIC_UINT32 error_correction_1;
    ZXIC_UINT32 error_correction_0;
}DPP_ETM_CGAVD_CGAVD_CFG_ERROR_WARNING_T;

typedef struct dpp_etm_cgavd_mult_qlen_th_en_t
{
    ZXIC_UINT32 mult_qlen_th;
}DPP_ETM_CGAVD_MULT_QLEN_TH_EN_T;

typedef struct dpp_etm_cgavd_mult_qlen_th_t
{
    ZXIC_UINT32 mult_qlen_th;
}DPP_ETM_CGAVD_MULT_QLEN_TH_T;

typedef struct dpp_etm_cgavd_cgavd_cfg_move_t
{
    ZXIC_UINT32 cfgmt_sys_move_en;
    ZXIC_UINT32 cfgmt_port_move_en;
    ZXIC_UINT32 cfgmt_flow_move_en;
}DPP_ETM_CGAVD_CGAVD_CFG_MOVE_T;

typedef struct dpp_etm_cgavd_cfgmt_total_th_t
{
    ZXIC_UINT32 cfgmt_total_th;
}DPP_ETM_CGAVD_CFGMT_TOTAL_TH_T;

typedef struct dpp_etm_cgavd_cfgmt_port_share_th_t
{
    ZXIC_UINT32 cfgmt_port_share_th;
}DPP_ETM_CGAVD_CFGMT_PORT_SHARE_TH_T;

typedef struct dpp_etm_cgavd_sa_unreach_state_t
{
    ZXIC_UINT32 sa_unreach_state;
}DPP_ETM_CGAVD_SA_UNREACH_STATE_T;

typedef struct dpp_etm_cgavd_mv_port_th_t
{
    ZXIC_UINT32 port_th;
}DPP_ETM_CGAVD_MV_PORT_TH_T;

typedef struct dpp_etm_cgavd_mv_drop_sp_th_t
{
    ZXIC_UINT32 mvdrop_sp_th;
}DPP_ETM_CGAVD_MV_DROP_SP_TH_T;

typedef struct dpp_etm_cgavd_cgavd_state_warning_t
{
    ZXIC_UINT32 deq_q_num_warning;
    ZXIC_UINT32 deq_pkt_len_warning;
    ZXIC_UINT32 enq_pkt_dp_warning;
    ZXIC_UINT32 unenq_q_num_warning;
    ZXIC_UINT32 enq_q_num_warning;
    ZXIC_UINT32 enq_pkt_len_warning;
}DPP_ETM_CGAVD_CGAVD_STATE_WARNING_T;

typedef struct dpp_etm_cgavd_tmmu_cgavd_dma_fifo_cnt_t
{
    ZXIC_UINT32 tmmu_cgavd_dma_fifo_cnt;
}DPP_ETM_CGAVD_TMMU_CGAVD_DMA_FIFO_CNT_T;

typedef struct dpp_etm_cgavd_tmmu_cgavd_dma_fifo_cnt_max_t
{
    ZXIC_UINT32 tmmu_cgavd_dma_fifo_cnt_max;
}DPP_ETM_CGAVD_TMMU_CGAVD_DMA_FIFO_CNT_MAX_T;

typedef struct dpp_etm_cgavd_imem_total_cnt_t
{
    ZXIC_UINT32 imem_total_cnt;
}DPP_ETM_CGAVD_IMEM_TOTAL_CNT_T;

typedef struct dpp_etm_cgavd_imem_total_cnt_max_t
{
    ZXIC_UINT32 imem_total_cnt_max;
}DPP_ETM_CGAVD_IMEM_TOTAL_CNT_MAX_T;

typedef struct dpp_etm_cgavd_flow0_omem_cnt_t
{
    ZXIC_UINT32 flow0_omem_cnt;
}DPP_ETM_CGAVD_FLOW0_OMEM_CNT_T;

typedef struct dpp_etm_cgavd_flow1_omem_cnt_t
{
    ZXIC_UINT32 flow1_omem_cnt;
}DPP_ETM_CGAVD_FLOW1_OMEM_CNT_T;

typedef struct dpp_etm_cgavd_flow2_omem_cnt_t
{
    ZXIC_UINT32 flow2_omem_cnt;
}DPP_ETM_CGAVD_FLOW2_OMEM_CNT_T;

typedef struct dpp_etm_cgavd_flow3_omem_cnt_t
{
    ZXIC_UINT32 flow3_omem_cnt;
}DPP_ETM_CGAVD_FLOW3_OMEM_CNT_T;

typedef struct dpp_etm_cgavd_flow4_omem_cnt_t
{
    ZXIC_UINT32 flow4_omem_cnt;
}DPP_ETM_CGAVD_FLOW4_OMEM_CNT_T;

typedef struct dpp_etm_cgavd_appoint_flow_num_message_1_t
{
    ZXIC_UINT32 appoint_flow_num_en_1;
    ZXIC_UINT32 appoint_flow_num_1;
}DPP_ETM_CGAVD_APPOINT_FLOW_NUM_MESSAGE_1_T;

typedef struct dpp_etm_cgavd_appoint_flow_num_message_2_t
{
    ZXIC_UINT32 appoint_flow_num_en_2;
    ZXIC_UINT32 appoint_flow_num_2;
}DPP_ETM_CGAVD_APPOINT_FLOW_NUM_MESSAGE_2_T;

typedef struct dpp_etm_cgavd_odma_cgavd_pkt_num_1_t
{
    ZXIC_UINT32 odma_cgavd_pkt_num_1;
}DPP_ETM_CGAVD_ODMA_CGAVD_PKT_NUM_1_T;

typedef struct dpp_etm_cgavd_odma_cgavd_byte_num_1_t
{
    ZXIC_UINT32 odma_cgavd_byte_num_1;
}DPP_ETM_CGAVD_ODMA_CGAVD_BYTE_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_enqueue_pkt_num_1_t
{
    ZXIC_UINT32 cgavd_enqueue_pkt_num_1;
}DPP_ETM_CGAVD_CGAVD_ENQUEUE_PKT_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_dequeue_pkt_num_1_t
{
    ZXIC_UINT32 cgavd_dequeue_pkt_num_1;
}DPP_ETM_CGAVD_CGAVD_DEQUEUE_PKT_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_pkt_imem_num_1_t
{
    ZXIC_UINT32 cgavd_qmu_pkt_imem_num_1;
}DPP_ETM_CGAVD_CGAVD_QMU_PKT_IMEM_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_pkt_omem_num_1_t
{
    ZXIC_UINT32 cgavd_qmu_pkt_omem_num_1;
}DPP_ETM_CGAVD_CGAVD_QMU_PKT_OMEM_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_byte_imem_num_1_t
{
    ZXIC_UINT32 cgavd_qmu_byte_imem_1;
}DPP_ETM_CGAVD_CGAVD_QMU_BYTE_IMEM_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_byte_omem_num_1_t
{
    ZXIC_UINT32 cgavd_qmu_byte_omem_1;
}DPP_ETM_CGAVD_CGAVD_QMU_BYTE_OMEM_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_pkt_drop_num_1_t
{
    ZXIC_UINT32 cgavd_qmu_pkt_drop_num_1;
}DPP_ETM_CGAVD_CGAVD_QMU_PKT_DROP_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_byte_drop_num_1_t
{
    ZXIC_UINT32 cgavd_qmu_byte_drop_num_1;
}DPP_ETM_CGAVD_CGAVD_QMU_BYTE_DROP_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_forbid_drop_num_1_t
{
    ZXIC_UINT32 cgavd_qmu_forbid_drop_num_1;
}DPP_ETM_CGAVD_CGAVD_QMU_FORBID_DROP_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_flow_td_drop_num_1_t
{
    ZXIC_UINT32 cgavd_qmu_flow_td_drop_num_1;
}DPP_ETM_CGAVD_CGAVD_QMU_FLOW_TD_DROP_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_flow_wred_drop_num_1_t
{
    ZXIC_UINT32 cgavd_qmu_flow_wred_drop_num_1;
}DPP_ETM_CGAVD_CGAVD_QMU_FLOW_WRED_DROP_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_flow_wred_dp_drop_num_1_t
{
    ZXIC_UINT32 cgavd_qmu_flow_wred_dp_drop_num1;
}DPP_ETM_CGAVD_CGAVD_QMU_FLOW_WRED_DP_DROP_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_pp_td_num_1_t
{
    ZXIC_UINT32 cgavd_qmu_pp_td_num_1;
}DPP_ETM_CGAVD_CGAVD_QMU_PP_TD_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_pp_wred_drop_num_1_t
{
    ZXIC_UINT32 cgavd_qmu_pp_wred_drop_num_1;
}DPP_ETM_CGAVD_CGAVD_QMU_PP_WRED_DROP_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_pp_wred_dp_drop_num_1_t
{
    ZXIC_UINT32 cgavd_qmu_pp_wred_dp_drop_num1;
}DPP_ETM_CGAVD_CGAVD_QMU_PP_WRED_DP_DROP_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_sys_td_drop_num_1_t
{
    ZXIC_UINT32 cgavd_qmu_sys_td_drop_num_1;
}DPP_ETM_CGAVD_CGAVD_QMU_SYS_TD_DROP_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_sys_gred_drop_num_1_t
{
    ZXIC_UINT32 cgavd_qmu_sys_gred_drop_num_1;
}DPP_ETM_CGAVD_CGAVD_QMU_SYS_GRED_DROP_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_sys_gred_dp_drop_num1_t
{
    ZXIC_UINT32 cgavd_qmu_sys_gred_dp_drop_num1;
}DPP_ETM_CGAVD_CGAVD_QMU_SYS_GRED_DP_DROP_NUM1_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_sa_drop_num_1_t
{
    ZXIC_UINT32 cgavd_qmu_sa_drop_num_1;
}DPP_ETM_CGAVD_CGAVD_QMU_SA_DROP_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_move_drop_num_1_t
{
    ZXIC_UINT32 cgavd_qmu_move_drop_num_1;
}DPP_ETM_CGAVD_CGAVD_QMU_MOVE_DROP_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_tm_mult_drop_num_1_t
{
    ZXIC_UINT32 cgavd_qmu_tm_mult_drop_num_1;
}DPP_ETM_CGAVD_CGAVD_QMU_TM_MULT_DROP_NUM_1_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_tm_error_drop_num_1_t
{
    ZXIC_UINT32 cgavd_qmu_tm_error_drop_num_1;
}DPP_ETM_CGAVD_CGAVD_QMU_TM_ERROR_DROP_NUM_1_T;

typedef struct dpp_etm_cgavd_odma_cgavd_pkt_num_2_t
{
    ZXIC_UINT32 odma_cgavd_pkt_num_2;
}DPP_ETM_CGAVD_ODMA_CGAVD_PKT_NUM_2_T;

typedef struct dpp_etm_cgavd_odma_cgavd_byte_num_2_t
{
    ZXIC_UINT32 odma_cgavd_byte_num_2;
}DPP_ETM_CGAVD_ODMA_CGAVD_BYTE_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_enqueue_pkt_num_2_t
{
    ZXIC_UINT32 cgavd_enqueue_pkt_num_2;
}DPP_ETM_CGAVD_CGAVD_ENQUEUE_PKT_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_dequeue_pkt_num_2_t
{
    ZXIC_UINT32 cgavd_dequeue_pkt_num_2;
}DPP_ETM_CGAVD_CGAVD_DEQUEUE_PKT_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_pkt_imem_num_2_t
{
    ZXIC_UINT32 cgavd_qmu_pkt_imem_num_2;
}DPP_ETM_CGAVD_CGAVD_QMU_PKT_IMEM_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_pkt_omem_num_2_t
{
    ZXIC_UINT32 cgavd_qmu_pkt_omem_num_2;
}DPP_ETM_CGAVD_CGAVD_QMU_PKT_OMEM_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_byte_imem_num_2_t
{
    ZXIC_UINT32 cgavd_qmu_byte_imem_num_2;
}DPP_ETM_CGAVD_CGAVD_QMU_BYTE_IMEM_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_byte_omem_num_2_t
{
    ZXIC_UINT32 cgavd_qmu_byte_omem_num_2;
}DPP_ETM_CGAVD_CGAVD_QMU_BYTE_OMEM_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_pkt_drop_num_2_t
{
    ZXIC_UINT32 cgavd_qmu_pkt_drop_num_2;
}DPP_ETM_CGAVD_CGAVD_QMU_PKT_DROP_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_byte_drop_num_2_t
{
    ZXIC_UINT32 cgavd_qmu_byte_drop_num_2;
}DPP_ETM_CGAVD_CGAVD_QMU_BYTE_DROP_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_forbid_drop_num_2_t
{
    ZXIC_UINT32 cgavd_qmu_forbid_drop_num_2;
}DPP_ETM_CGAVD_CGAVD_QMU_FORBID_DROP_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_flow_td_drop_num_2_t
{
    ZXIC_UINT32 cgavd_qmu_flow_td_drop_num_2;
}DPP_ETM_CGAVD_CGAVD_QMU_FLOW_TD_DROP_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_flow_wred_drop_num_2_t
{
    ZXIC_UINT32 cgavd_qmu_flow_wred_drop_num_2;
}DPP_ETM_CGAVD_CGAVD_QMU_FLOW_WRED_DROP_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_flow_wred_dp_drop_num_2_t
{
    ZXIC_UINT32 cgavd_qmu_flow_wred_dp_drop_num_2;
}DPP_ETM_CGAVD_CGAVD_QMU_FLOW_WRED_DP_DROP_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_pp_td_num_2_t
{
    ZXIC_UINT32 cgavd_qmu_pp_td_num_2;
}DPP_ETM_CGAVD_CGAVD_QMU_PP_TD_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_pp_wred_drop_num_2_t
{
    ZXIC_UINT32 cgavd_qmu_pp_wred_drop_num_2;
}DPP_ETM_CGAVD_CGAVD_QMU_PP_WRED_DROP_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_pp_wred_dp_drop_num_2_t
{
    ZXIC_UINT32 cgavd_qmu_pp_wred_dp_drop_num_2;
}DPP_ETM_CGAVD_CGAVD_QMU_PP_WRED_DP_DROP_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_sys_td_drop_num_2_t
{
    ZXIC_UINT32 cgavd_qmu_sys_td_drop_num_2;
}DPP_ETM_CGAVD_CGAVD_QMU_SYS_TD_DROP_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_sys_gred_drop_num_2_t
{
    ZXIC_UINT32 cgavd_qmu_sys_gred_drop_num_2;
}DPP_ETM_CGAVD_CGAVD_QMU_SYS_GRED_DROP_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_sys_gred_dp_drop_num_2_t
{
    ZXIC_UINT32 cgavd_qmu_sys_gred_dp_drop_num_2;
}DPP_ETM_CGAVD_CGAVD_QMU_SYS_GRED_DP_DROP_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_sa_drop_num_2_t
{
    ZXIC_UINT32 cgavd_qmu_sa_drop_num_2;
}DPP_ETM_CGAVD_CGAVD_QMU_SA_DROP_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_move_drop_num_2_t
{
    ZXIC_UINT32 cgavd_qmu_move_drop_num_2;
}DPP_ETM_CGAVD_CGAVD_QMU_MOVE_DROP_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_tm_mult_drop_num_2_t
{
    ZXIC_UINT32 cgavd_qmu_tm_mult_drop_num_2;
}DPP_ETM_CGAVD_CGAVD_QMU_TM_MULT_DROP_NUM_2_T;

typedef struct dpp_etm_cgavd_cgavd_qmu_tm_error_drop_num_2_t
{
    ZXIC_UINT32 cgavd_qmu_tm_error_drop_num_2;
}DPP_ETM_CGAVD_CGAVD_QMU_TM_ERROR_DROP_NUM_2_T;

typedef struct dpp_etm_cgavd_move_flow_th_profile_t
{
    ZXIC_UINT32 move_drop_profile;
}DPP_ETM_CGAVD_MOVE_FLOW_TH_PROFILE_T;

typedef struct dpp_etm_cgavd_move_flow_th_t
{
    ZXIC_UINT32 move_drop_flow_th;
}DPP_ETM_CGAVD_MOVE_FLOW_TH_T;

typedef struct dpp_etm_tmmu_emem_pd_fifo_aful_th_t
{
    ZXIC_UINT32 emem_pd_fifo_aful_th;
}DPP_ETM_TMMU_EMEM_PD_FIFO_AFUL_TH_T;

typedef struct dpp_etm_tmmu_dma_data_fifo_aful_th_t
{
    ZXIC_UINT32 dma_data_fifo_aful_th;
}DPP_ETM_TMMU_DMA_DATA_FIFO_AFUL_TH_T;

typedef struct dpp_etm_tmmu_tmmu_states_0_t
{
    ZXIC_UINT32 tm_odma_pkt_rdy;
    ZXIC_UINT32 dma_data_fifo_empty;
    ZXIC_UINT32 imem_enq_rd_fifo_empty;
    ZXIC_UINT32 imem_enq_drop_fifo_empty;
    ZXIC_UINT32 imem_deq_rd_fifo_empty;
    ZXIC_UINT32 imem_deq_drop_fifo_empty;
    ZXIC_UINT32 wr_cmd_fifo_empty;
    ZXIC_UINT32 cached_pd_fifo_empty;
    ZXIC_UINT32 emem_pd_fifo_empty;
    ZXIC_UINT32 pd_order_fifo_empty;
    ZXIC_UINT32 odma_tm_data_rdy;
    ZXIC_UINT32 odma_tm_discard_rdy;
    ZXIC_UINT32 olif_tmmu_rdy;
    ZXIC_UINT32 mmu_tm_cmd_wr_rdy;
    ZXIC_UINT32 mmu_tm_data_wr_rdy;
    ZXIC_UINT32 mmu_tm_rd_rdy;
    ZXIC_UINT32 mmu_tm_sop_rd_rdy;
    ZXIC_UINT32 qmu_tmmu_sop_data_rdy;
    ZXIC_UINT32 tmmu_cmdsw_imem_release_rdy;
    ZXIC_UINT32 imem_age_release_rdy;
    ZXIC_UINT32 tmmu_qmu_wr_rdy;
    ZXIC_UINT32 tmmu_qmu_rdy_7;
    ZXIC_UINT32 tmmu_qmu_rdy_6;
    ZXIC_UINT32 tmmu_qmu_rdy_5;
    ZXIC_UINT32 tmmu_qmu_rdy_4;
    ZXIC_UINT32 tmmu_qmu_rdy_3;
    ZXIC_UINT32 tmmu_qmu_rdy_2;
    ZXIC_UINT32 tmmu_qmu_rdy_1;
    ZXIC_UINT32 tmmu_qmu_rdy_0;
    ZXIC_UINT32 tmmu_qmu_rd_rdy;
    ZXIC_UINT32 tmmu_qmu_sop_rd_rdy;
}DPP_ETM_TMMU_TMMU_STATES_0_T;

typedef struct dpp_etm_tmmu_qmu_tmmu_wr_sop_cnt_t
{
    ZXIC_UINT32 qmu_tmmu_wr_sop_cnt;
}DPP_ETM_TMMU_QMU_TMMU_WR_SOP_CNT_T;

typedef struct dpp_etm_tmmu_qmu_tmmu_wr_eop_cnt_t
{
    ZXIC_UINT32 qmu_tmmu_wr_eop_cnt;
}DPP_ETM_TMMU_QMU_TMMU_WR_EOP_CNT_T;

typedef struct dpp_etm_tmmu_qmu_tmmu_wr_drop_cnt_t
{
    ZXIC_UINT32 qmu_tmmu_wr_drop_cnt;
}DPP_ETM_TMMU_QMU_TMMU_WR_DROP_CNT_T;

typedef struct dpp_etm_tmmu_qmu_tmmu_wr_emem_cnt_t
{
    ZXIC_UINT32 qmu_tmmu_wr_emem_cnt;
}DPP_ETM_TMMU_QMU_TMMU_WR_EMEM_CNT_T;

typedef struct dpp_etm_tmmu_qmu_tmmu_wr_imem_cnt_t
{
    ZXIC_UINT32 qmu_tmmu_wr_imem_cnt;
}DPP_ETM_TMMU_QMU_TMMU_WR_IMEM_CNT_T;

typedef struct dpp_etm_tmmu_tmmu_mmu_wr_sop_cnt_t
{
    ZXIC_UINT32 tmmu_mmu_wr_sop_cnt;
}DPP_ETM_TMMU_TMMU_MMU_WR_SOP_CNT_T;

typedef struct dpp_etm_tmmu_tmmu_mmu_wr_eop_cnt_t
{
    ZXIC_UINT32 tmmu_mmu_wr_eop_cnt;
}DPP_ETM_TMMU_TMMU_MMU_WR_EOP_CNT_T;

typedef struct dpp_etm_tmmu_qmu_tmmu_rd_sop_cnt_t
{
    ZXIC_UINT32 qmu_tmmu_rd_sop_cnt;
}DPP_ETM_TMMU_QMU_TMMU_RD_SOP_CNT_T;

typedef struct dpp_etm_tmmu_qmu_tmmu_rd_eop_cnt_t
{
    ZXIC_UINT32 qmu_tmmu_rd_eop_cnt;
}DPP_ETM_TMMU_QMU_TMMU_RD_EOP_CNT_T;

typedef struct dpp_etm_tmmu_qmu_tmmu_rd_drop_cnt_t
{
    ZXIC_UINT32 qmu_tmmu_rd_drop_cnt;
}DPP_ETM_TMMU_QMU_TMMU_RD_DROP_CNT_T;

typedef struct dpp_etm_tmmu_qmu_tmmu_rd_emem_cnt_t
{
    ZXIC_UINT32 qmu_tmmu_rd_emem_cnt;
}DPP_ETM_TMMU_QMU_TMMU_RD_EMEM_CNT_T;

typedef struct dpp_etm_tmmu_qmu_tmmu_rd_imem_cnt_t
{
    ZXIC_UINT32 qmu_tmmu_rd_imem_cnt;
}DPP_ETM_TMMU_QMU_TMMU_RD_IMEM_CNT_T;

typedef struct dpp_etm_tmmu_tmmu_mmu_rd_sop_cnt_t
{
    ZXIC_UINT32 tmmu_mmu_rd_sop_cnt;
}DPP_ETM_TMMU_TMMU_MMU_RD_SOP_CNT_T;

typedef struct dpp_etm_tmmu_tmmu_mmu_rd_eop_cnt_t
{
    ZXIC_UINT32 tmmu_mmu_rd_eop_cnt;
}DPP_ETM_TMMU_TMMU_MMU_RD_EOP_CNT_T;

typedef struct dpp_etm_tmmu_tmmu_odma_in_sop_cnt_t
{
    ZXIC_UINT32 tmmu_odma_in_sop_cnt;
}DPP_ETM_TMMU_TMMU_ODMA_IN_SOP_CNT_T;

typedef struct dpp_etm_tmmu_tmmu_odma_in_eop_cnt_t
{
    ZXIC_UINT32 tmmu_odma_in_eop_cnt;
}DPP_ETM_TMMU_TMMU_ODMA_IN_EOP_CNT_T;

typedef struct dpp_etm_tmmu_tmmu_odma_vld_cnt_t
{
    ZXIC_UINT32 tmmu_odma_vld_cnt;
}DPP_ETM_TMMU_TMMU_ODMA_VLD_CNT_T;

typedef struct dpp_etm_tmmu_qmu_pd_in_cnt_t
{
    ZXIC_UINT32 qmu_pd_in_cnt;
}DPP_ETM_TMMU_QMU_PD_IN_CNT_T;

typedef struct dpp_etm_tmmu_tmmu_pd_hit_cnt_t
{
    ZXIC_UINT32 tmmu_pd_hit_cnt;
}DPP_ETM_TMMU_TMMU_PD_HIT_CNT_T;

typedef struct dpp_etm_tmmu_tmmu_pd_out_cnt_t
{
    ZXIC_UINT32 tmmu_pd_out_cnt;
}DPP_ETM_TMMU_TMMU_PD_OUT_CNT_T;

typedef struct dpp_etm_tmmu_tmmu_wr_cmd_fifo_wr_cnt_t
{
    ZXIC_UINT32 tmmu_wr_cmd_fifo_wr_cnt;
}DPP_ETM_TMMU_TMMU_WR_CMD_FIFO_WR_CNT_T;

typedef struct dpp_etm_tmmu_tmmu_imem_age_cnt_t
{
    ZXIC_UINT32 tmmu_imem_age_cnt;
}DPP_ETM_TMMU_TMMU_IMEM_AGE_CNT_T;

typedef struct dpp_etm_tmmu_tmmu_cmdsch_rd_cnt_t
{
    ZXIC_UINT32 tmmu_cmdsch_rd_cnt;
}DPP_ETM_TMMU_TMMU_CMDSCH_RD_CNT_T;

typedef struct dpp_etm_tmmu_tmmu_cmdsch_drop_cnt_t
{
    ZXIC_UINT32 tmmu_cmdsch_drop_cnt;
}DPP_ETM_TMMU_TMMU_CMDSCH_DROP_CNT_T;

typedef struct dpp_etm_tmmu_tmmu_cmdsw_drop_cnt_t
{
    ZXIC_UINT32 tmmu_cmdsw_drop_cnt;
}DPP_ETM_TMMU_TMMU_CMDSW_DROP_CNT_T;

typedef struct dpp_etm_tmmu_tmmu_odma_enq_rd_cnt_t
{
    ZXIC_UINT32 tmmu_odma_enq_rd_cnt;
}DPP_ETM_TMMU_TMMU_ODMA_ENQ_RD_CNT_T;

typedef struct dpp_etm_tmmu_tmmu_odma_enq_drop_cnt_t
{
    ZXIC_UINT32 tmmu_odma_enq_drop_cnt;
}DPP_ETM_TMMU_TMMU_ODMA_ENQ_DROP_CNT_T;

typedef struct dpp_etm_tmmu_tmmu_odma_imem_age_cnt_t
{
    ZXIC_UINT32 tmmu_odma_imem_age_cnt;
}DPP_ETM_TMMU_TMMU_ODMA_IMEM_AGE_CNT_T;

typedef struct dpp_etm_tmmu_tmmu_odma_deq_rd_cnt_t
{
    ZXIC_UINT32 tmmu_odma_deq_rd_cnt;
}DPP_ETM_TMMU_TMMU_ODMA_DEQ_RD_CNT_T;

typedef struct dpp_etm_tmmu_tmmu_odma_deq_drop_cnt_t
{
    ZXIC_UINT32 tmmu_odma_deq_drop_cnt;
}DPP_ETM_TMMU_TMMU_ODMA_DEQ_DROP_CNT_T;

typedef struct dpp_etm_tmmu_olif_tmmu_xoff_cnt_t
{
    ZXIC_UINT32 olif_tmmu_xoff_cnt;
}DPP_ETM_TMMU_OLIF_TMMU_XOFF_CNT_T;

typedef struct dpp_etm_tmmu_odma_tm_data_xoff_cnt_t
{
    ZXIC_UINT32 odma_tm_data_xoff_cnt;
}DPP_ETM_TMMU_ODMA_TM_DATA_XOFF_CNT_T;

typedef struct dpp_etm_tmmu_tm_odma_pkt_xoff_cnt_t
{
    ZXIC_UINT32 tm_odma_pkt_xoff_cnt;
}DPP_ETM_TMMU_TM_ODMA_PKT_XOFF_CNT_T;

typedef struct dpp_etm_tmmu_tm_state_3_t
{
    ZXIC_UINT32 tmmu_qmu_rdy_9;
    ZXIC_UINT32 tmmu_qmu_rdy_8;
}DPP_ETM_TMMU_TM_STATE_3_T;

typedef struct dpp_etm_tmmu_cfgmt_pd_cache_cmd_t
{
    ZXIC_UINT32 cfgmt_pd_cache_addr;
}DPP_ETM_TMMU_CFGMT_PD_CACHE_CMD_T;

typedef struct dpp_etm_tmmu_cfgmt_pd_cache_rd_done_t
{
    ZXIC_UINT32 cfgmt_pd_cache_rd_done;
}DPP_ETM_TMMU_CFGMT_PD_CACHE_RD_DONE_T;

typedef struct dpp_etm_tmmu_cfgmt_pd_cache_rd_data_0_t
{
    ZXIC_UINT32 cfgmt_pd_cache_rd_data_0;
}DPP_ETM_TMMU_CFGMT_PD_CACHE_RD_DATA_0_T;

typedef struct dpp_etm_tmmu_cfgmt_pd_cache_rd_data_1_t
{
    ZXIC_UINT32 cfgmt_pd_cache_rd_data_1;
}DPP_ETM_TMMU_CFGMT_PD_CACHE_RD_DATA_1_T;

typedef struct dpp_etm_tmmu_cfgmt_pd_cache_rd_data_2_t
{
    ZXIC_UINT32 cfgmt_pd_cache_rd_data_2;
}DPP_ETM_TMMU_CFGMT_PD_CACHE_RD_DATA_2_T;

typedef struct dpp_etm_tmmu_cfgmt_pd_cache_rd_data_3_t
{
    ZXIC_UINT32 cfgmt_pd_cache_rd_data_3;
}DPP_ETM_TMMU_CFGMT_PD_CACHE_RD_DATA_3_T;

typedef struct dpp_etm_tmmu_cfgmt_tmmu_to_odma_para_t
{
    ZXIC_UINT32 cfgmt_tmmu_to_odma_para;
}DPP_ETM_TMMU_CFGMT_TMMU_TO_ODMA_PARA_T;

typedef struct dpp_etm_tmmu_cfgmt_dma_data_fifo_cnt_t
{
    ZXIC_UINT32 cfgmt_dma_data_fifo_cnt;
}DPP_ETM_TMMU_CFGMT_DMA_DATA_FIFO_CNT_T;

typedef struct dpp_etm_tmmu_cfgmt_cache_tag_bit0_offset_t
{
    ZXIC_UINT32 cfgmt_cache_tag_bit0_offset;
}DPP_ETM_TMMU_CFGMT_CACHE_TAG_BIT0_OFFSET_T;

typedef struct dpp_etm_tmmu_cfgmt_cache_tag_bit1_offset_t
{
    ZXIC_UINT32 cfgmt_cache_tag_bit1_offset;
}DPP_ETM_TMMU_CFGMT_CACHE_TAG_BIT1_OFFSET_T;

typedef struct dpp_etm_tmmu_cfgmt_cache_tag_bit2_offset_t
{
    ZXIC_UINT32 cfgmt_cache_tag_bit2_offset;
}DPP_ETM_TMMU_CFGMT_CACHE_TAG_BIT2_OFFSET_T;

typedef struct dpp_etm_tmmu_cfgmt_cache_tag_bit3_offset_t
{
    ZXIC_UINT32 cfgmt_cache_tag_bit3_offset;
}DPP_ETM_TMMU_CFGMT_CACHE_TAG_BIT3_OFFSET_T;

typedef struct dpp_etm_tmmu_cfgmt_cache_tag_bit4_offset_t
{
    ZXIC_UINT32 cfgmt_cache_tag_bit4_offset;
}DPP_ETM_TMMU_CFGMT_CACHE_TAG_BIT4_OFFSET_T;

typedef struct dpp_etm_tmmu_cfgmt_cache_tag_bit5_offset_t
{
    ZXIC_UINT32 cfgmt_cache_tag_bit5_offset;
}DPP_ETM_TMMU_CFGMT_CACHE_TAG_BIT5_OFFSET_T;

typedef struct dpp_etm_tmmu_cfgmt_cache_index_bit0_offset_t
{
    ZXIC_UINT32 cfgmt_cache_index_bit0_offset;
}DPP_ETM_TMMU_CFGMT_CACHE_INDEX_BIT0_OFFSET_T;

typedef struct dpp_etm_tmmu_cfgmt_cache_index_bit1_offset_t
{
    ZXIC_UINT32 cfgmt_cache_index_bit1_offset;
}DPP_ETM_TMMU_CFGMT_CACHE_INDEX_BIT1_OFFSET_T;

typedef struct dpp_etm_tmmu_cfgmt_cache_index_bit2_offset_t
{
    ZXIC_UINT32 cfgmt_cache_index_bit2_offset;
}DPP_ETM_TMMU_CFGMT_CACHE_INDEX_BIT2_OFFSET_T;

typedef struct dpp_etm_tmmu_cfgmt_cache_index_bit3_offset_t
{
    ZXIC_UINT32 cfgmt_cache_index_bit3_offset;
}DPP_ETM_TMMU_CFGMT_CACHE_INDEX_BIT3_OFFSET_T;

typedef struct dpp_etm_tmmu_cfgmt_cache_index_bit4_offset_t
{
    ZXIC_UINT32 cfgmt_cache_index_bit4_offset;
}DPP_ETM_TMMU_CFGMT_CACHE_INDEX_BIT4_OFFSET_T;

typedef struct dpp_etm_tmmu_cfgmt_cache_index_bit5_offset_t
{
    ZXIC_UINT32 cfgmt_cache_index_bit5_offset;
}DPP_ETM_TMMU_CFGMT_CACHE_INDEX_BIT5_OFFSET_T;

typedef struct dpp_etm_tmmu_cfgmt_cache_index_bit6_offset_t
{
    ZXIC_UINT32 cfgmt_cache_index_bit6_offset;
}DPP_ETM_TMMU_CFGMT_CACHE_INDEX_BIT6_OFFSET_T;

typedef struct dpp_etm_tmmu_cfgmt_cache_index_bit7_offset_t
{
    ZXIC_UINT32 cfgmt_cache_index_bit7_offset;
}DPP_ETM_TMMU_CFGMT_CACHE_INDEX_BIT7_OFFSET_T;

typedef struct dpp_etm_tmmu_cfgmt_cache_index_bit8_offset_t
{
    ZXIC_UINT32 cfgmt_cache_index_bit8_offset;
}DPP_ETM_TMMU_CFGMT_CACHE_INDEX_BIT8_OFFSET_T;

typedef struct dpp_etm_tmmu_cfgmt_cache_index_bit9_offset_t
{
    ZXIC_UINT32 cfgmt_cache_index_bit9_offset;
}DPP_ETM_TMMU_CFGMT_CACHE_INDEX_BIT9_OFFSET_T;

typedef struct dpp_etm_tmmu_cfgmt_cache_index_bit10_offset_t
{
    ZXIC_UINT32 cfgmt_cache_index_bit10_offset;
}DPP_ETM_TMMU_CFGMT_CACHE_INDEX_BIT10_OFFSET_T;

typedef struct dpp_etm_tmmu_cfgmt_cache_index_bit11_offset_t
{
    ZXIC_UINT32 cfgmt_cache_index_bit11_offset;
}DPP_ETM_TMMU_CFGMT_CACHE_INDEX_BIT11_OFFSET_T;

typedef struct dpp_etm_tmmu_cfgmt_cache_index_bit12_offset_t
{
    ZXIC_UINT32 cfgmt_cache_index_bit12_offset;
}DPP_ETM_TMMU_CFGMT_CACHE_INDEX_BIT12_OFFSET_T;

typedef struct dpp_etm_shap_bktfull_fifo_full_flagregister_t
{
    ZXIC_UINT32 bktfull_fifo_full_flag_core;
}DPP_ETM_SHAP_BKTFULL_FIFO_FULL_FLAGREGISTER_T;

typedef struct dpp_etm_shap_fifo_full_regregister_t
{
    ZXIC_UINT32 fifo_full_reg;
}DPP_ETM_SHAP_FIFO_FULL_REGREGISTER_T;

typedef struct dpp_etm_shap_fifo_empty_regregister_t
{
    ZXIC_UINT32 fifo_empty_reg;
}DPP_ETM_SHAP_FIFO_EMPTY_REGREGISTER_T;

typedef struct dpp_etm_shap_fifo_almost_full_regregister_t
{
    ZXIC_UINT32 fifo_almost_full_reg;
}DPP_ETM_SHAP_FIFO_ALMOST_FULL_REGREGISTER_T;

typedef struct dpp_etm_shap_fifo_almost_empty_regregister_t
{
    ZXIC_UINT32 fifo_almost_empty_reg;
}DPP_ETM_SHAP_FIFO_ALMOST_EMPTY_REGREGISTER_T;

typedef struct dpp_etm_crdt_credit_space_select_t
{
    ZXIC_UINT32 credit_space_select;
}DPP_ETM_CRDT_CREDIT_SPACE_SELECT_T;

typedef struct dpp_etm_crdt_stat_space_max_t
{
    ZXIC_UINT32 stat_space_max;
}DPP_ETM_CRDT_STAT_SPACE_MAX_T;

typedef struct dpp_etm_crdt_stat_space_min_t
{
    ZXIC_UINT32 stat_space_min;
}DPP_ETM_CRDT_STAT_SPACE_MIN_T;

typedef struct dpp_etm_crdt_stat_space_credit_t
{
    ZXIC_UINT32 stat_space_credit;
}DPP_ETM_CRDT_STAT_SPACE_CREDIT_T;

typedef struct dpp_etm_crdt_stat_que_step8_credit_t
{
    ZXIC_UINT32 stat_que_step8_credit;
}DPP_ETM_CRDT_STAT_QUE_STEP8_CREDIT_T;

typedef struct dpp_etm_crdt_special_que_t
{
    ZXIC_UINT32 special_que_id;
}DPP_ETM_CRDT_SPECIAL_QUE_T;

typedef struct dpp_etm_crdt_special_que_credit_t
{
    ZXIC_UINT32 special_que_credit;
}DPP_ETM_CRDT_SPECIAL_QUE_CREDIT_T;

typedef struct dpp_etm_crdt_lif_congest_credit_cnt_t
{
    ZXIC_UINT32 lif_congest_credit_cnt;
}DPP_ETM_CRDT_LIF_CONGEST_CREDIT_CNT_T;

typedef struct dpp_etm_crdt_lif_port_congest_credit_cnt_t
{
    ZXIC_UINT32 lif_port_congest_credit_cnt;
}DPP_ETM_CRDT_LIF_PORT_CONGEST_CREDIT_CNT_T;

typedef struct dpp_etm_crdt_crdt_congest_credit_cnt_t
{
    ZXIC_UINT32 crdt_congest_credit_cnt;
}DPP_ETM_CRDT_CRDT_CONGEST_CREDIT_CNT_T;

typedef struct dpp_etm_crdt_crdt_port_congest_credit_cnt_t
{
    ZXIC_UINT32 crdt_port_congest_credit_cnt;
}DPP_ETM_CRDT_CRDT_PORT_CONGEST_CREDIT_CNT_T;

typedef struct dpp_etm_crdt_congest_port_id_t
{
    ZXIC_UINT32 congest_port_id;
}DPP_ETM_CRDT_CONGEST_PORT_ID_T;

typedef struct dpp_etm_crdt_dev_link_control_t
{
    ZXIC_UINT32 dev_link_control;
}DPP_ETM_CRDT_DEV_LINK_CONTROL_T;

typedef struct dpp_etm_crdt_crdt_sa_port_rdy_t
{
    ZXIC_UINT32 crdt_sa_port_rdy;
}DPP_ETM_CRDT_CRDT_SA_PORT_RDY_T;

typedef struct dpp_etm_crdt_crdt_congest_mode_select_t
{
    ZXIC_UINT32 crdt_congest_mode_selectr;
}DPP_ETM_CRDT_CRDT_CONGEST_MODE_SELECT_T;

typedef struct dpp_etm_crdt_fifo_out_all_crs_normal_cnt_t
{
    ZXIC_UINT32 fifo_out_all_crs_normal_cnt;
}DPP_ETM_CRDT_FIFO_OUT_ALL_CRS_NORMAL_CNT_T;

typedef struct dpp_etm_crdt_fifo_out_all_crs_off_cnt_t
{
    ZXIC_UINT32 fifo_out_all_crs_off_cnt;
}DPP_ETM_CRDT_FIFO_OUT_ALL_CRS_OFF_CNT_T;

typedef struct dpp_etm_crdt_fifo_out_que_crs_normal_cnt_t
{
    ZXIC_UINT32 fifo_out_que_crs_normal_cnt;
}DPP_ETM_CRDT_FIFO_OUT_QUE_CRS_NORMAL_CNT_T;

typedef struct dpp_etm_crdt_fifo_out_que_crs_off_cnt_t
{
    ZXIC_UINT32 fifo_out_que_crs_off_cnt;
}DPP_ETM_CRDT_FIFO_OUT_QUE_CRS_OFF_CNT_T;

typedef struct dpp_etm_crdt_mode_add_60g_t
{
    ZXIC_UINT32 mode_add_60g;
}DPP_ETM_CRDT_MODE_ADD_60G_T;

typedef struct dpp_etm_crdt_pp_token_add_t
{
    ZXIC_UINT32 pp_token_add_cir;
}DPP_ETM_CRDT_PP_TOKEN_ADD_T;

typedef struct dpp_etm_crdt_pp_cir_token_total_dist_cnt_t
{
    ZXIC_UINT32 pp_cir_token_total_dist_counter;
}DPP_ETM_CRDT_PP_CIR_TOKEN_TOTAL_DIST_CNT_T;

typedef struct dpp_etm_crdt_pp_cir_token_total_dec_cnt_t
{
    ZXIC_UINT32 pp_cir_token_total_dec_counter;
}DPP_ETM_CRDT_PP_CIR_TOKEN_TOTAL_DEC_CNT_T;

typedef struct dpp_etm_crdt_dev_credit_cnt_t
{
    ZXIC_UINT32 dev_credit_cnt;
}DPP_ETM_CRDT_DEV_CREDIT_CNT_T;

typedef struct dpp_etm_crdt_no_credit_cnt1_t
{
    ZXIC_UINT32 no_credit_cnt1;
}DPP_ETM_CRDT_NO_CREDIT_CNT1_T;

typedef struct dpp_etm_crdt_no_credit_cnt2_t
{
    ZXIC_UINT32 no_credit_cnt2;
}DPP_ETM_CRDT_NO_CREDIT_CNT2_T;

typedef struct dpp_etm_crdt_asm_interval_0_cfg_t
{
    ZXIC_UINT32 asm_interval_0_cfg;
}DPP_ETM_CRDT_ASM_INTERVAL_0_CFG_T;

typedef struct dpp_etm_crdt_asm_interval_1_cfg_t
{
    ZXIC_UINT32 asm_interval_1_cfg;
}DPP_ETM_CRDT_ASM_INTERVAL_1_CFG_T;

typedef struct dpp_etm_crdt_asm_interval_2_cfg_t
{
    ZXIC_UINT32 asm_interval_2_cfg;
}DPP_ETM_CRDT_ASM_INTERVAL_2_CFG_T;

typedef struct dpp_etm_crdt_asm_interval_3_cfg_t
{
    ZXIC_UINT32 asm_interval_3_cfg;
}DPP_ETM_CRDT_ASM_INTERVAL_3_CFG_T;

typedef struct dpp_etm_crdt_asm_interval_4_cfg_t
{
    ZXIC_UINT32 asm_interval_4_cfg;
}DPP_ETM_CRDT_ASM_INTERVAL_4_CFG_T;

typedef struct dpp_etm_crdt_asm_interval_5cfg_t
{
    ZXIC_UINT32 asm_interval_5_cfg;
}DPP_ETM_CRDT_ASM_INTERVAL_5CFG_T;

typedef struct dpp_etm_crdt_asm_interval_6_cfg_t
{
    ZXIC_UINT32 asm_interval_6_cfg;
}DPP_ETM_CRDT_ASM_INTERVAL_6_CFG_T;

typedef struct dpp_etm_crdt_asm_interval_7_cfg_t
{
    ZXIC_UINT32 asm_interval_7_cfg;
}DPP_ETM_CRDT_ASM_INTERVAL_7_CFG_T;

typedef struct dpp_etm_crdt_crdt_total_congest_mode_cfg_t
{
    ZXIC_UINT32 crdt_total_congest_mode_cfg;
}DPP_ETM_CRDT_CRDT_TOTAL_CONGEST_MODE_CFG_T;

typedef struct dpp_etm_crdt_rci_fifo_ini_deep_cfg_t
{
    ZXIC_UINT32 rci_fifo_ini_deep_cfg;
}DPP_ETM_CRDT_RCI_FIFO_INI_DEEP_CFG_T;

typedef struct dpp_etm_crdt_crdt_ecc_t
{
    ZXIC_UINT32 seinfo_wfq_single_ecc_err;
    ZXIC_UINT32 seinfo_wfq_double_ecc_err;
    ZXIC_UINT32 seinfo_fq_single_ecc_err;
    ZXIC_UINT32 seinfo_fq_double_ecc_err;
    ZXIC_UINT32 ecc_bypass;
}DPP_ETM_CRDT_CRDT_ECC_T;

typedef struct dpp_etm_crdt_ucn_asm_rdy_shield_en_t
{
    ZXIC_UINT32 ucn_rdy_shield_en;
    ZXIC_UINT32 asm_rdy_shield_en;
}DPP_ETM_CRDT_UCN_ASM_RDY_SHIELD_EN_T;

typedef struct dpp_etm_crdt_ucn_asm_rdy_t
{
    ZXIC_UINT32 ucn_rdy;
    ZXIC_UINT32 asm_rdy;
}DPP_ETM_CRDT_UCN_ASM_RDY_T;

typedef struct dpp_etm_crdt_rci_grade_t
{
    ZXIC_UINT32 rci_grade;
}DPP_ETM_CRDT_RCI_GRADE_T;

typedef struct dpp_etm_crdt_crdt_rci_value_r_t
{
    ZXIC_UINT32 crdt_rci_value_r;
}DPP_ETM_CRDT_CRDT_RCI_VALUE_R_T;

typedef struct dpp_etm_crdt_crdt_interval_now_t
{
    ZXIC_UINT32 crdt_interval_now;
}DPP_ETM_CRDT_CRDT_INTERVAL_NOW_T;

typedef struct dpp_etm_crdt_crs_sheild_flow_id_cfg_t
{
    ZXIC_UINT32 crs_sheild_flow_id_cfg;
}DPP_ETM_CRDT_CRS_SHEILD_FLOW_ID_CFG_T;

typedef struct dpp_etm_crdt_crs_sheild_en_cfg_t
{
    ZXIC_UINT32 crs_sheild_en_cfg;
}DPP_ETM_CRDT_CRS_SHEILD_EN_CFG_T;

typedef struct dpp_etm_crdt_crs_sheild_value_cfg_t
{
    ZXIC_UINT32 crs_sheild_value_cfg;
}DPP_ETM_CRDT_CRS_SHEILD_VALUE_CFG_T;

typedef struct dpp_etm_crdt_test_token_calc_ctrl_t
{
    ZXIC_UINT32 test_token_calc_state;
    ZXIC_UINT32 test_token_calc_trigger;
}DPP_ETM_CRDT_TEST_TOKEN_CALC_CTRL_T;

typedef struct dpp_etm_crdt_test_token_sample_cycle_num_t
{
    ZXIC_UINT32 sample_cycle_num;
}DPP_ETM_CRDT_TEST_TOKEN_SAMPLE_CYCLE_NUM_T;

typedef struct dpp_etm_crdt_q_state_0_7_t
{
    ZXIC_UINT32 q_token_state_7;
    ZXIC_UINT32 q_token_state_6;
    ZXIC_UINT32 q_token_state_5;
    ZXIC_UINT32 q_token_state_4;
    ZXIC_UINT32 q_token_state_3;
    ZXIC_UINT32 q_token_state_2;
    ZXIC_UINT32 q_token_state_1;
    ZXIC_UINT32 q_token_state_0;
}DPP_ETM_CRDT_Q_STATE_0_7_T;

typedef struct dpp_etm_crdt_q_state_8_15_t
{
    ZXIC_UINT32 q_token_state_15;
    ZXIC_UINT32 q_token_state_14;
    ZXIC_UINT32 q_token_state_13;
    ZXIC_UINT32 q_token_state_12;
    ZXIC_UINT32 q_token_state_11;
    ZXIC_UINT32 q_token_state_10;
    ZXIC_UINT32 q_token_state_9;
    ZXIC_UINT32 q_token_state_8;
}DPP_ETM_CRDT_Q_STATE_8_15_T;

typedef struct dpp_etm_qmu_csw_csch_rd_cmd_cnt_t
{
    ZXIC_UINT32 csw_csch_rd_cmd_cnt;
}DPP_ETM_QMU_CSW_CSCH_RD_CMD_CNT_T;

typedef struct dpp_etm_qmu_csw_csch_rd_sop_cnt_t
{
    ZXIC_UINT32 csw_csch_rd_sop_cnt;
}DPP_ETM_QMU_CSW_CSCH_RD_SOP_CNT_T;

typedef struct dpp_etm_qmu_csw_csch_rd_eop_cnt_t
{
    ZXIC_UINT32 csw_csch_rd_eop_cnt;
}DPP_ETM_QMU_CSW_CSCH_RD_EOP_CNT_T;

typedef struct dpp_etm_qmu_csw_csch_rd_drop_cnt_t
{
    ZXIC_UINT32 csw_csch_rd_drop_cnt;
}DPP_ETM_QMU_CSW_CSCH_RD_DROP_CNT_T;

typedef struct dpp_etm_qmu_csch_mmu_rd_cmd_cnt_t
{
    ZXIC_UINT32 csch_mmu_rd_cmd_cnt;
}DPP_ETM_QMU_CSCH_MMU_RD_CMD_CNT_T;

typedef struct dpp_etm_qmu_csch_mmu_rd_sop_cnt_t
{
    ZXIC_UINT32 csch_mmu_rd_sop_cnt;
}DPP_ETM_QMU_CSCH_MMU_RD_SOP_CNT_T;

typedef struct dpp_etm_qmu_csch_mmu_rd_eop_cnt_t
{
    ZXIC_UINT32 csch_mmu_rd_eop_cnt;
}DPP_ETM_QMU_CSCH_MMU_RD_EOP_CNT_T;

typedef struct dpp_etm_qmu_csch_mmu_rd_drop_cnt_t
{
    ZXIC_UINT32 csch_mmu_rd_drop_cnt;
}DPP_ETM_QMU_CSCH_MMU_RD_DROP_CNT_T;

typedef struct dpp_etm_qmu_qcfg_qsch_crs_filter_t
{
    ZXIC_UINT32 qcfg_qsch_crs_filter;
}DPP_ETM_QMU_QCFG_QSCH_CRS_FILTER_T;

typedef struct dpp_etm_qmu_qcfg_qsch_crs_force_en_t
{
    ZXIC_UINT32 qcfg_qsch_crs_force_en;
}DPP_ETM_QMU_QCFG_QSCH_CRS_FORCE_EN_T;

typedef struct dpp_etm_qmu_qcfg_qsch_crs_force_qnum_t
{
    ZXIC_UINT32 qcfg_qsch_crs_force_qnum;
}DPP_ETM_QMU_QCFG_QSCH_CRS_FORCE_QNUM_T;

typedef struct dpp_etm_qmu_qcfg_qsch_crs_force_crs_t
{
    ZXIC_UINT32 qcfg_qsch_crs_force_crs;
}DPP_ETM_QMU_QCFG_QSCH_CRS_FORCE_CRS_T;

typedef struct dpp_etm_qmu_cfgmt_oshp_sgmii_shap_mode_t
{
    ZXIC_UINT32 cfgmt_oshp_sgmii_shap_mode;
}DPP_ETM_QMU_CFGMT_OSHP_SGMII_SHAP_MODE_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_sashap_en_t
{
    ZXIC_UINT32 cfgmt_qmu_sashap_en;
}DPP_ETM_QMU_CFGMT_QMU_SASHAP_EN_T;

typedef struct dpp_etm_qmu_cfgmt_sashap_token_max_t
{
    ZXIC_UINT32 cfgmt_sashap_token_max;
}DPP_ETM_QMU_CFGMT_SASHAP_TOKEN_MAX_T;

typedef struct dpp_etm_qmu_cfgmt_sashap_token_min_t
{
    ZXIC_UINT32 cfgmt_sashap_token_min;
}DPP_ETM_QMU_CFGMT_SASHAP_TOKEN_MIN_T;

typedef struct dpp_etm_qmu_cfg_qsch_q3lbaddrate_t
{
    ZXIC_UINT32 cfg_qsch_q3lbaddrate;
}DPP_ETM_QMU_CFG_QSCH_Q3LBADDRATE_T;

typedef struct dpp_etm_qmu_cfg_qsch_q012lbaddrate_t
{
    ZXIC_UINT32 cfg_qsch_q012lbaddrate;
}DPP_ETM_QMU_CFG_QSCH_Q012LBADDRATE_T;

typedef struct dpp_etm_qmu_cfg_qsch_q3creditlbmaxcnt_t
{
    ZXIC_UINT32 cfg_qsch_q3creditlbmaxcnt;
}DPP_ETM_QMU_CFG_QSCH_Q3CREDITLBMAXCNT_T;

typedef struct dpp_etm_qmu_cfg_qsch_q012creditlbmaxcnt_t
{
    ZXIC_UINT32 cfg_qsch_q012creditlbmaxcnt;
}DPP_ETM_QMU_CFG_QSCH_Q012CREDITLBMAXCNT_T;

typedef struct dpp_etm_qmu_cfg_qsch_mul_token_gen_num_t
{
    ZXIC_UINT32 cfg_qsch_mul_token_gen_num;
}DPP_ETM_QMU_CFG_QSCH_MUL_TOKEN_GEN_NUM_T;

typedef struct dpp_etm_qmu_cfg_qsch_q3_credit_lb_control_en_t
{
    ZXIC_UINT32 cfg_qsch_q3_credit_lb_control_en;
}DPP_ETM_QMU_CFG_QSCH_Q3_CREDIT_LB_CONTROL_EN_T;

typedef struct dpp_etm_qmu_cfg_qsch_q012_credit_lb_control_en_t
{
    ZXIC_UINT32 cfg_qsch_q012_credit_lb_control_en;
}DPP_ETM_QMU_CFG_QSCH_Q012_CREDIT_LB_CONTROL_EN_T;

typedef struct dpp_etm_qmu_cfg_qsch_sp_dwrr_en_t
{
    ZXIC_UINT32 cfg_qsch_sp_dwrr_en;
}DPP_ETM_QMU_CFG_QSCH_SP_DWRR_EN_T;

typedef struct dpp_etm_qmu_cfg_qsch_q01_attach_en_t
{
    ZXIC_UINT32 cfg_qsch_q01_attach_en;
}DPP_ETM_QMU_CFG_QSCH_Q01_ATTACH_EN_T;

typedef struct dpp_etm_qmu_cfg_qsch_w0_t
{
    ZXIC_UINT32 cfg_qsch_w0;
}DPP_ETM_QMU_CFG_QSCH_W0_T;

typedef struct dpp_etm_qmu_cfg_qsch_w1_t
{
    ZXIC_UINT32 cfg_qsch_w1;
}DPP_ETM_QMU_CFG_QSCH_W1_T;

typedef struct dpp_etm_qmu_cfg_qsch_w2_t
{
    ZXIC_UINT32 cfg_qsch_w2;
}DPP_ETM_QMU_CFG_QSCH_W2_T;

typedef struct dpp_etm_qmu_cfg_qsch_lkybktmaxcnt1_t
{
    ZXIC_UINT32 cfg_qsch_lkybktmaxcnt1;
}DPP_ETM_QMU_CFG_QSCH_LKYBKTMAXCNT1_T;

typedef struct dpp_etm_qmu_cfg_qsch_lkybktmaxcnt2_t
{
    ZXIC_UINT32 cfg_qsch_lkybktmaxcnt2;
}DPP_ETM_QMU_CFG_QSCH_LKYBKTMAXCNT2_T;

typedef struct dpp_etm_qmu_cfg_qsch_lkybktdcrrate1_t
{
    ZXIC_UINT32 cfg_qsch_lkybktdcrrate1;
}DPP_ETM_QMU_CFG_QSCH_LKYBKTDCRRATE1_T;

typedef struct dpp_etm_qmu_cfg_qsch_lkybktdcrrate2_t
{
    ZXIC_UINT32 cfg_qsch_lkybktdcrrate2;
}DPP_ETM_QMU_CFG_QSCH_LKYBKTDCRRATE2_T;

typedef struct dpp_etm_qmu_cfg_qsch_lkybktdcrrate3_t
{
    ZXIC_UINT32 cfg_qsch_lkybktdcrrate3;
}DPP_ETM_QMU_CFG_QSCH_LKYBKTDCRRATE3_T;

typedef struct dpp_etm_qmu_cfg_qsch_lkybktmaxcnt3_t
{
    ZXIC_UINT32 cfg_qsch_lkybktmaxcnt3;
}DPP_ETM_QMU_CFG_QSCH_LKYBKTMAXCNT3_T;

typedef struct dpp_etm_qmu_cfg_qsch_qmu_mul_auto_sa_version_t
{
    ZXIC_UINT32 cfg_qsch_qmu_mul_auto_sa_version;
}DPP_ETM_QMU_CFG_QSCH_QMU_MUL_AUTO_SA_VERSION_T;

typedef struct dpp_etm_qmu_cfg_qsch_sa_credit_value_0_t
{
    ZXIC_UINT32 cfg_qsch_sa_credit_value_0;
}DPP_ETM_QMU_CFG_QSCH_SA_CREDIT_VALUE_0_T;

typedef struct dpp_etm_qmu_cfg_qsch_sa_credit_value_1_t
{
    ZXIC_UINT32 cfg_qsch_sa_credit_value_1;
}DPP_ETM_QMU_CFG_QSCH_SA_CREDIT_VALUE_1_T;

typedef struct dpp_etm_qmu_cfg_qsch_sa_credit_value_2_t
{
    ZXIC_UINT32 cfg_qsch_sa_credit_value_2;
}DPP_ETM_QMU_CFG_QSCH_SA_CREDIT_VALUE_2_T;

typedef struct dpp_etm_qmu_cfg_qsch_sa_credit_value_3_t
{
    ZXIC_UINT32 cfg_qsch_sa_credit_value_3;
}DPP_ETM_QMU_CFG_QSCH_SA_CREDIT_VALUE_3_T;

typedef struct dpp_etm_qmu_cfg_qsch_sa_credit_value_4_t
{
    ZXIC_UINT32 cfg_qsch_sa_credit_value_4;
}DPP_ETM_QMU_CFG_QSCH_SA_CREDIT_VALUE_4_T;

typedef struct dpp_etm_qmu_cfg_qsch_sa_credit_value_5_t
{
    ZXIC_UINT32 cfg_qsch_sa_credit_value_5;
}DPP_ETM_QMU_CFG_QSCH_SA_CREDIT_VALUE_5_T;

typedef struct dpp_etm_qmu_cfg_qsch_sa_credit_value_6_t
{
    ZXIC_UINT32 cfg_qsch_sa_credit_value_6;
}DPP_ETM_QMU_CFG_QSCH_SA_CREDIT_VALUE_6_T;

typedef struct dpp_etm_qmu_cfg_qsch_sa_credit_value_7_t
{
    ZXIC_UINT32 cfg_qsch_sa_credit_value_7;
}DPP_ETM_QMU_CFG_QSCH_SA_CREDIT_VALUE_7_T;

typedef struct dpp_etm_qmu_cfg_qsch_remote_credit_fifo_almost_full_th_t
{
    ZXIC_UINT32 cfg_qsch_remote_credit_fifo_almost_full_th;
}DPP_ETM_QMU_CFG_QSCH_REMOTE_CREDIT_FIFO_ALMOST_FULL_TH_T;

typedef struct dpp_etm_qmu_cfg_qsch_auto_credit_fifo_almost_full_th_t
{
    ZXIC_UINT32 cfg_qsch_auto_credit_fifo_almost_full_th;
}DPP_ETM_QMU_CFG_QSCH_AUTO_CREDIT_FIFO_ALMOST_FULL_TH_T;

typedef struct dpp_etm_qmu_cfg_qsch_q3_credit_fifo_almost_full_th_t
{
    ZXIC_UINT32 cfg_qsch_q3_credit_fifo_almost_full_th;
}DPP_ETM_QMU_CFG_QSCH_Q3_CREDIT_FIFO_ALMOST_FULL_TH_T;

typedef struct dpp_etm_qmu_cfg_qsch_q012_credit_fifo_almost_full_th_t
{
    ZXIC_UINT32 cfg_qsch_q012_credit_fifo_almost_full_th;
}DPP_ETM_QMU_CFG_QSCH_Q012_CREDIT_FIFO_ALMOST_FULL_TH_T;

typedef struct dpp_etm_qmu_cfg_qsch_mul_fc_res_en_t
{
    ZXIC_UINT32 cfg_qsch_mul_fc_res_en;
}DPP_ETM_QMU_CFG_QSCH_MUL_FC_RES_EN_T;

typedef struct dpp_etm_qmu_cfgmt_mul_ovf_udf_flg_query_t
{
    ZXIC_UINT32 qsch_cfg_remote_credit_fifo_full;
    ZXIC_UINT32 qsch_cfg_remote_credit_fifo_empty;
    ZXIC_UINT32 qsch_cfg_remote_credit_fifo_overflow;
    ZXIC_UINT32 qsch_cfg_remote_credit_fifo_underflow;
    ZXIC_UINT32 qsch_cfg_auto_credit_fifo_full;
    ZXIC_UINT32 qsch_cfg_auto_credit_fifo_empty;
    ZXIC_UINT32 qsch_cfg_auto_credit_fifo_overflow;
    ZXIC_UINT32 qsch_cfg_auto_credit_fifo_underflow;
    ZXIC_UINT32 qsch_cfg_q3_credit_fifo_full;
    ZXIC_UINT32 qsch_cfg_q3_credit_fifo_empty;
    ZXIC_UINT32 qsch_cfg_q3_credit_fifo_overflow;
    ZXIC_UINT32 qsch_cfg_q3_credit_fifo_underflow;
    ZXIC_UINT32 qsch_cfg_q012_credit_fifo_full;
    ZXIC_UINT32 qsch_cfg_q012_credit_fifo_empty;
    ZXIC_UINT32 qsch_cfg_q012_credit_fifo_overflow;
    ZXIC_UINT32 qsch_cfg_q012_credit_fifo_underflow;
    ZXIC_UINT32 qsch_cfg_lkybktoverflow1;
    ZXIC_UINT32 qsch_cfg_lkybktoverflow2;
    ZXIC_UINT32 qsch_cfg_lkybktoverflow3;
}DPP_ETM_QMU_CFGMT_MUL_OVF_UDF_FLG_QUERY_T;

typedef struct dpp_etm_qmu_cfgmt_mul_cng_flg_query_t
{
    ZXIC_UINT32 qsch_cfg_q3cngflag;
    ZXIC_UINT32 qsch_cfg_q012cngflag;
    ZXIC_UINT32 qsch_cfg_cngflag1;
    ZXIC_UINT32 qsch_cfg_cngflag2;
    ZXIC_UINT32 qsch_cfg_cngflag3;
}DPP_ETM_QMU_CFGMT_MUL_CNG_FLG_QUERY_T;

typedef struct dpp_etm_qmu_qsch_cfg_lkybktval1_t
{
    ZXIC_UINT32 qsch_cfg_lkybktval1;
}DPP_ETM_QMU_QSCH_CFG_LKYBKTVAL1_T;

typedef struct dpp_etm_qmu_qsch_cfg_lkybktval2_t
{
    ZXIC_UINT32 qsch_cfg_lkybktval2;
}DPP_ETM_QMU_QSCH_CFG_LKYBKTVAL2_T;

typedef struct dpp_etm_qmu_qsch_cfg_lkybktval3_t
{
    ZXIC_UINT32 qsch_cfg_lkybktval3;
}DPP_ETM_QMU_QSCH_CFG_LKYBKTVAL3_T;

typedef struct dpp_etm_qmu_qsch_cfg_q3lbval_t
{
    ZXIC_UINT32 qsch_cfg_q3lbval;
}DPP_ETM_QMU_QSCH_CFG_Q3LBVAL_T;

typedef struct dpp_etm_qmu_qsch_cfg_q012lbval_t
{
    ZXIC_UINT32 qsch_cfg_q012lbval;
}DPP_ETM_QMU_QSCH_CFG_Q012LBVAL_T;

typedef struct dpp_etm_qmu_qlist_cfgmt_ram_ecc_err2_t
{
    ZXIC_UINT32 qlist_imem_pd_ram_single_ecc_err;
    ZXIC_UINT32 qlist_imem_pd_ram_double_ecc_err;
    ZXIC_UINT32 qlist_imem_up_ptr_ram_single_ecc_err;
    ZXIC_UINT32 qlist_imem_up_ptr_ram_double_ecc_err;
    ZXIC_UINT32 qlist_imem_down_ptr_ram_single_ecc_err;
    ZXIC_UINT32 qlist_imem_down_ptr_ram_double_ecc_err;
    ZXIC_UINT32 cmdsw_sop_fifo_single_ecc_err;
    ZXIC_UINT32 cmdsw_sop_fifo_double_ecc_err;
    ZXIC_UINT32 cmdsw_nsop_fifo_single_ecc_err;
    ZXIC_UINT32 cmdsw_nsop_fifo_double_ecc_err;
    ZXIC_UINT32 cmdsw_mmudat_fifo_single_ecc_err;
    ZXIC_UINT32 cmdsw_mmudat_fifo_double_ecc_err;
    ZXIC_UINT32 qlist_rd_release_fwft_single_ecc_err;
    ZXIC_UINT32 qlist_rd_release_fwft_double_ecc_err;
    ZXIC_UINT32 qlist_drop_imem_fwft_single_ecc_err;
    ZXIC_UINT32 qlist_drop_imem_fwft_double_ecc_err;
}DPP_ETM_QMU_QLIST_CFGMT_RAM_ECC_ERR2_T;

typedef struct dpp_etm_qmu_csch_aged_cmd_cnt_t
{
    ZXIC_UINT32 csch_aged_cmd_cnt;
}DPP_ETM_QMU_CSCH_AGED_CMD_CNT_T;

typedef struct dpp_etm_qmu_csch_qcfg_csch_congest_cnt_t
{
    ZXIC_UINT32 csch_qcfg_csch_congest_cnt;
}DPP_ETM_QMU_CSCH_QCFG_CSCH_CONGEST_CNT_T;

typedef struct dpp_etm_qmu_csch_qcfg_qlist_csch_sop_cnt_t
{
    ZXIC_UINT32 csch_qcfg_qlist_csch_sop_cnt;
}DPP_ETM_QMU_CSCH_QCFG_QLIST_CSCH_SOP_CNT_T;

typedef struct dpp_etm_qmu_csch_qcfg_qlist_csch_eop_cnt_t
{
    ZXIC_UINT32 csch_qcfg_qlist_csch_eop_cnt;
}DPP_ETM_QMU_CSCH_QCFG_QLIST_CSCH_EOP_CNT_T;

typedef struct dpp_etm_qmu_csch_qcfg_csch_csw_sop_cnt_t
{
    ZXIC_UINT32 csch_qcfg_csch_csw_sop_cnt;
}DPP_ETM_QMU_CSCH_QCFG_CSCH_CSW_SOP_CNT_T;

typedef struct dpp_etm_qmu_csch_qcfg_csch_csw_eop_cnt_t
{
    ZXIC_UINT32 csch_qcfg_csch_csw_eop_cnt;
}DPP_ETM_QMU_CSCH_QCFG_CSCH_CSW_EOP_CNT_T;

typedef struct dpp_etm_qmu_csch_qcfg_qlist_csch_drop_cnt_t
{
    ZXIC_UINT32 csch_qcfg_qlist_csch_drop_cnt;
}DPP_ETM_QMU_CSCH_QCFG_QLIST_CSCH_DROP_CNT_T;

typedef struct dpp_etm_qmu_csch_qcfg_csch_csw_drop_cnt_t
{
    ZXIC_UINT32 csch_qcfg_csch_csw_drop_cnt;
}DPP_ETM_QMU_CSCH_QCFG_CSCH_CSW_DROP_CNT_T;

typedef struct dpp_etm_qmu_csw_mmu_sop_cmd_cnt_t
{
    ZXIC_UINT32 csw_mmu_sop_cmd_cnt;
}DPP_ETM_QMU_CSW_MMU_SOP_CMD_CNT_T;

typedef struct dpp_etm_qmu_mmu_csw_sop_data_cnt_t
{
    ZXIC_UINT32 mmu_csw_sop_data_cnt;
}DPP_ETM_QMU_MMU_CSW_SOP_DATA_CNT_T;

typedef struct dpp_etm_qmu_csw_qsch_feedb_cnt_t
{
    ZXIC_UINT32 csw_qsch_feedb_cnt;
}DPP_ETM_QMU_CSW_QSCH_FEEDB_CNT_T;

typedef struct dpp_etm_qmu_qmu_crdt_port_fc_cnt_t
{
    ZXIC_UINT32 qmu_crdt_port_fc_cnt;
}DPP_ETM_QMU_QMU_CRDT_PORT_FC_CNT_T;

typedef struct dpp_etm_qmu_csch_r_block_cnt_t
{
    ZXIC_UINT32 csch_r_block_cnt;
}DPP_ETM_QMU_CSCH_R_BLOCK_CNT_T;

typedef struct dpp_etm_qmu_qcfg_qlist_qds_head_rd_t
{
    ZXIC_UINT32 qcfg_qlist_qds_head_rd;
}DPP_ETM_QMU_QCFG_QLIST_QDS_HEAD_RD_T;

typedef struct dpp_etm_qmu_qcfg_qlist_qds_tail_rd_t
{
    ZXIC_UINT32 qcfg_qlist_qds_tail_rd;
}DPP_ETM_QMU_QCFG_QLIST_QDS_TAIL_RD_T;

typedef struct dpp_etm_qmu_qcfg_qlist_ept_rd_t
{
    ZXIC_UINT32 qcfg_qlist_ept_rd;
}DPP_ETM_QMU_QCFG_QLIST_EPT_RD_T;

typedef struct dpp_etm_qmu_qcfg_qlist_age_flag_rd_t
{
    ZXIC_UINT32 qcfg_qlist_age_flag_rd;
}DPP_ETM_QMU_QCFG_QLIST_AGE_FLAG_RD_T;

typedef struct dpp_etm_qmu_qcfg_qlist_cti_rd_t
{
    ZXIC_UINT32 qcfg_qlist_cti_rd;
}DPP_ETM_QMU_QCFG_QLIST_CTI_RD_T;

typedef struct dpp_etm_qmu_qcfg_qlist_cto_rd_t
{
    ZXIC_UINT32 qcfg_qlist_cto_rd;
}DPP_ETM_QMU_QCFG_QLIST_CTO_RD_T;

typedef struct dpp_etm_qmu_qcfg_qlist_chk_rd_t
{
    ZXIC_UINT32 qcfg_qlist_chk_rd;
}DPP_ETM_QMU_QCFG_QLIST_CHK_RD_T;

typedef struct dpp_etm_qmu_qcfg_qlist_nod_rd_t
{
    ZXIC_UINT32 qcfg_qlist_nod_rd;
}DPP_ETM_QMU_QCFG_QLIST_NOD_RD_T;

typedef struct dpp_etm_qmu_qcfg_qlist_biu_rd_t
{
    ZXIC_UINT32 qcfg_qlist_biu_rd;
}DPP_ETM_QMU_QCFG_QLIST_BIU_RD_T;

typedef struct dpp_etm_qmu_qsch_r_wlist_flag_t
{
    ZXIC_UINT32 qsch_r_wlist_flag;
}DPP_ETM_QMU_QSCH_R_WLIST_FLAG_T;

typedef struct dpp_etm_qmu_qcfg_crs_flg_rd_t
{
    ZXIC_UINT32 qcfg_crs_flg_rd;
}DPP_ETM_QMU_QCFG_CRS_FLG_RD_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_imem_age_qds_t
{
    ZXIC_UINT32 cfgmt_qmu_imem_tp;
    ZXIC_UINT32 cfgmt_qmu_imem_hp;
}DPP_ETM_QMU_CFGMT_QMU_IMEM_AGE_QDS_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_imem_age_qlen_t
{
    ZXIC_UINT32 cfgmt_qmu_imem_no_empty;
    ZXIC_UINT32 cfgmt_qmu_imem_qlen;
}DPP_ETM_QMU_CFGMT_QMU_IMEM_AGE_QLEN_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_imem_pd_ram_low_t
{
    ZXIC_UINT32 cfgmt_qmu_imem_pd_ram_low;
}DPP_ETM_QMU_CFGMT_QMU_IMEM_PD_RAM_LOW_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_imem_pd_ram_high_t
{
    ZXIC_UINT32 cfgmt_qmu_imem_pd_ram_high;
}DPP_ETM_QMU_CFGMT_QMU_IMEM_PD_RAM_HIGH_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_imem_up_ptr_t
{
    ZXIC_UINT32 cfgmt_qmu_imem_up_ptr;
}DPP_ETM_QMU_CFGMT_QMU_IMEM_UP_PTR_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_imem_down_ptr_t
{
    ZXIC_UINT32 cfgmt_qmu_imem_down_ptr;
}DPP_ETM_QMU_CFGMT_QMU_IMEM_DOWN_PTR_T;

typedef struct dpp_etm_qmu_cfgmt_qmu_imem_age_flag_t
{
    ZXIC_UINT32 cfgmt_qmu_imem_age_flag;
}DPP_ETM_QMU_CFGMT_QMU_IMEM_AGE_FLAG_T;

typedef struct dpp_etm_qmu_cfg_qsch_lkybkt2cngth_t
{
    ZXIC_UINT32 cfg_qsch_lkybkt2cngth;
}DPP_ETM_QMU_CFG_QSCH_LKYBKT2CNGTH_T;

typedef struct dpp_etm_qmu_cfg_qsch_lkybkt1cngth_t
{
    ZXIC_UINT32 cfg_qsch_lkybkt1cngth;
}DPP_ETM_QMU_CFG_QSCH_LKYBKT1CNGTH_T;

typedef struct dpp_etm_qmu_cfg_qsch_lkybkt3cngth_t
{
    ZXIC_UINT32 cfg_qsch_lkybkt3cngth;
}DPP_ETM_QMU_CFG_QSCH_LKYBKT3CNGTH_T;

typedef struct dpp_etm_qmu_cfg_qsch_rm_mul_mcn1_credit_value_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn1_credit_value;
}DPP_ETM_QMU_CFG_QSCH_RM_MUL_MCN1_CREDIT_VALUE_T;

typedef struct dpp_etm_qmu_cfg_qsch_rm_mul_mcn2_credit_value_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn2_credit_value;
}DPP_ETM_QMU_CFG_QSCH_RM_MUL_MCN2_CREDIT_VALUE_T;

typedef struct dpp_etm_qmu_cfg_qsch_rm_mul_mcn3_credit_value_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn3_credit_value;
}DPP_ETM_QMU_CFG_QSCH_RM_MUL_MCN3_CREDIT_VALUE_T;

typedef struct dpp_etm_qmu_rm_mul_mcn1_rand_ansr_seed_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn1_rand_mchsm_en;
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn1_rand_ansr_seed;
}DPP_ETM_QMU_RM_MUL_MCN1_RAND_ANSR_SEED_T;

typedef struct dpp_etm_qmu_rm_mul_mcn2_rand_ansr_seed_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn2_rand_mchsm_en;
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn2_rand_ansr_seed;
}DPP_ETM_QMU_RM_MUL_MCN2_RAND_ANSR_SEED_T;

typedef struct dpp_etm_qmu_rm_mul_mcn3_rand_ansr_seed_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn3_rand_mchsm_en;
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn3_rand_ansr_seed;
}DPP_ETM_QMU_RM_MUL_MCN3_RAND_ANSR_SEED_T;

typedef struct dpp_etm_qmu_rm_mul_mcn1_rand_ansr_th_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn1_rand_ansr_th;
}DPP_ETM_QMU_RM_MUL_MCN1_RAND_ANSR_TH_T;

typedef struct dpp_etm_qmu_rm_mul_mcn2_rand_ansr_th_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn2_rand_ansr_th;
}DPP_ETM_QMU_RM_MUL_MCN2_RAND_ANSR_TH_T;

typedef struct dpp_etm_qmu_rm_mul_mcn3_rand_ansr_th_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn3_rand_ansr_th;
}DPP_ETM_QMU_RM_MUL_MCN3_RAND_ANSR_TH_T;

typedef struct dpp_etm_qmu_rm_mul_mcn1_rand_hold_base_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn1_rand_mchsm_en;
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn1_rand_hold_base;
}DPP_ETM_QMU_RM_MUL_MCN1_RAND_HOLD_BASE_T;

typedef struct dpp_etm_qmu_rm_mul_mcn2_rand_hold_base_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn2_rand_mchsm_en;
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn2_rand_hold_base;
}DPP_ETM_QMU_RM_MUL_MCN2_RAND_HOLD_BASE_T;

typedef struct dpp_etm_qmu_rm_mul_mcn3_rand_hold_base_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn3_rand_mchsm_en;
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn3_rand_hold_base;
}DPP_ETM_QMU_RM_MUL_MCN3_RAND_HOLD_BASE_T;

typedef struct dpp_etm_qmu_rm_mul_mcn1_rand_sel_mask_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn1_rand_sel_mask;
}DPP_ETM_QMU_RM_MUL_MCN1_RAND_SEL_MASK_T;

typedef struct dpp_etm_qmu_rm_mul_mcn2_rand_sel_mask_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn2_rand_sel_mask;
}DPP_ETM_QMU_RM_MUL_MCN2_RAND_SEL_MASK_T;

typedef struct dpp_etm_qmu_rm_mul_mcn3_rand_sel_mask_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn3_rand_sel_mask;
}DPP_ETM_QMU_RM_MUL_MCN3_RAND_SEL_MASK_T;

typedef struct dpp_etm_qmu_rm_mul_mcn1_rand_sel_seed_reg0_t
{
    ZXIC_UINT32 rm_mul_mcn1_rand_sel_seed7;
    ZXIC_UINT32 rm_mul_mcn1_rand_sel_seed6;
    ZXIC_UINT32 rm_mul_mcn1_rand_sel_seed5;
    ZXIC_UINT32 rm_mul_mcn1_rand_sel_seed4;
    ZXIC_UINT32 rm_mul_mcn1_rand_sel_seed3;
    ZXIC_UINT32 rm_mul_mcn1_rand_sel_seed2;
    ZXIC_UINT32 rm_mul_mcn1_rand_sel_seed1;
    ZXIC_UINT32 rm_mul_mcn1_rand_sel_seed0;
}DPP_ETM_QMU_RM_MUL_MCN1_RAND_SEL_SEED_REG0_T;

typedef struct dpp_etm_qmu_rm_mul_mcn1_rand_sel_seed_reg1_t
{
    ZXIC_UINT32 rm_mul_mcn1_rand_sel_seed8;
}DPP_ETM_QMU_RM_MUL_MCN1_RAND_SEL_SEED_REG1_T;

typedef struct dpp_etm_qmu_rm_mul_mcn2_rand_sel_seed_reg0_t
{
    ZXIC_UINT32 rm_mul_mcn2_rand_sel_seed7;
    ZXIC_UINT32 rm_mul_mcn2_rand_sel_seed6;
    ZXIC_UINT32 rm_mul_mcn2_rand_sel_seed5;
    ZXIC_UINT32 rm_mul_mcn2_rand_sel_seed4;
    ZXIC_UINT32 rm_mul_mcn2_rand_sel_seed3;
    ZXIC_UINT32 rm_mul_mcn2_rand_sel_seed2;
    ZXIC_UINT32 rm_mul_mcn2_rand_sel_seed1;
    ZXIC_UINT32 rm_mul_mcn2_rand_sel_seed0;
}DPP_ETM_QMU_RM_MUL_MCN2_RAND_SEL_SEED_REG0_T;

typedef struct dpp_etm_qmu_rm_mul_mcn2_rand_sel_seed_reg1_t
{
    ZXIC_UINT32 rm_mul_mcn2_rand_sel_seed8;
}DPP_ETM_QMU_RM_MUL_MCN2_RAND_SEL_SEED_REG1_T;

typedef struct dpp_etm_qmu_rm_mul_mcn3_rand_sel_seed_reg0_t
{
    ZXIC_UINT32 rm_mul_mcn3_rand_sel_seed7;
    ZXIC_UINT32 rm_mul_mcn3_rand_sel_seed6;
    ZXIC_UINT32 rm_mul_mcn3_rand_sel_seed5;
    ZXIC_UINT32 rm_mul_mcn3_rand_sel_seed4;
    ZXIC_UINT32 rm_mul_mcn3_rand_sel_seed3;
    ZXIC_UINT32 rm_mul_mcn3_rand_sel_seed2;
    ZXIC_UINT32 rm_mul_mcn3_rand_sel_seed1;
    ZXIC_UINT32 rm_mul_mcn3_rand_sel_seed0;
}DPP_ETM_QMU_RM_MUL_MCN3_RAND_SEL_SEED_REG0_T;

typedef struct dpp_etm_qmu_rm_mul_mcn3_rand_sel_seed_reg1_t
{
    ZXIC_UINT32 rm_mul_mcn3_rand_sel_seed8;
}DPP_ETM_QMU_RM_MUL_MCN3_RAND_SEL_SEED_REG1_T;

typedef struct dpp_etm_qmu_rm_mul_mcn1_step_wait_th1_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn1_step_wait_th1;
}DPP_ETM_QMU_RM_MUL_MCN1_STEP_WAIT_TH1_T;

typedef struct dpp_etm_qmu_rm_mul_mcn1_step_wait_th2_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn1_step_wait_th2;
}DPP_ETM_QMU_RM_MUL_MCN1_STEP_WAIT_TH2_T;

typedef struct dpp_etm_qmu_rm_mul_mcn1_step_wait_th3_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn1_step_wait_th3;
}DPP_ETM_QMU_RM_MUL_MCN1_STEP_WAIT_TH3_T;

typedef struct dpp_etm_qmu_rm_mul_mcn1_step_wait_th4_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn1_step_wait_th4;
}DPP_ETM_QMU_RM_MUL_MCN1_STEP_WAIT_TH4_T;

typedef struct dpp_etm_qmu_rm_mul_mcn1_step_wait_th5_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn1_step_wait_th5;
}DPP_ETM_QMU_RM_MUL_MCN1_STEP_WAIT_TH5_T;

typedef struct dpp_etm_qmu_rm_mul_mcn1_step_wait_th6_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn1_step_wait_th6;
}DPP_ETM_QMU_RM_MUL_MCN1_STEP_WAIT_TH6_T;

typedef struct dpp_etm_qmu_rm_mul_mcn1_step_wait_th7_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn1_step_wait_th7;
}DPP_ETM_QMU_RM_MUL_MCN1_STEP_WAIT_TH7_T;

typedef struct dpp_etm_qmu_rm_mul_mcn2_step_wait_th1_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn2_step_wait_th1;
}DPP_ETM_QMU_RM_MUL_MCN2_STEP_WAIT_TH1_T;

typedef struct dpp_etm_qmu_rm_mul_mcn2_step_wait_th2_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn2_step_wait_th2;
}DPP_ETM_QMU_RM_MUL_MCN2_STEP_WAIT_TH2_T;

typedef struct dpp_etm_qmu_rm_mul_mcn2_step_wait_th3_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn2_step_wait_th3;
}DPP_ETM_QMU_RM_MUL_MCN2_STEP_WAIT_TH3_T;

typedef struct dpp_etm_qmu_rm_mul_mcn2_step_wait_th4_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn2_step_wait_th4;
}DPP_ETM_QMU_RM_MUL_MCN2_STEP_WAIT_TH4_T;

typedef struct dpp_etm_qmu_rm_mul_mcn2_step_wait_th5_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn2_step_wait_th5;
}DPP_ETM_QMU_RM_MUL_MCN2_STEP_WAIT_TH5_T;

typedef struct dpp_etm_qmu_rm_mul_mcn2_step_wait_th6_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn2_step_wait_th6;
}DPP_ETM_QMU_RM_MUL_MCN2_STEP_WAIT_TH6_T;

typedef struct dpp_etm_qmu_rm_mul_mcn2_step_wait_th7_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn2_step_wait_th7;
}DPP_ETM_QMU_RM_MUL_MCN2_STEP_WAIT_TH7_T;

typedef struct dpp_etm_qmu_rm_mul_mcn3_step_wait_th1_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn3_step_wait_th1;
}DPP_ETM_QMU_RM_MUL_MCN3_STEP_WAIT_TH1_T;

typedef struct dpp_etm_qmu_rm_mul_mcn3_step_wait_th2_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn3_step_wait_th2;
}DPP_ETM_QMU_RM_MUL_MCN3_STEP_WAIT_TH2_T;

typedef struct dpp_etm_qmu_rm_mul_mcn3_step_wait_th3_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn3_step_wait_th3;
}DPP_ETM_QMU_RM_MUL_MCN3_STEP_WAIT_TH3_T;

typedef struct dpp_etm_qmu_rm_mul_mcn3_step_wait_th4_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn3_step_wait_th4;
}DPP_ETM_QMU_RM_MUL_MCN3_STEP_WAIT_TH4_T;

typedef struct dpp_etm_qmu_rm_mul_mcn3_step_wait_th5_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn3_step_wait_th5;
}DPP_ETM_QMU_RM_MUL_MCN3_STEP_WAIT_TH5_T;

typedef struct dpp_etm_qmu_rm_mul_mcn3_step_wait_th6_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn3_step_wait_th6;
}DPP_ETM_QMU_RM_MUL_MCN3_STEP_WAIT_TH6_T;

typedef struct dpp_etm_qmu_rm_mul_mcn3step_wait_th7_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn3_step_wait_th7;
}DPP_ETM_QMU_RM_MUL_MCN3STEP_WAIT_TH7_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate0_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate0;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE0_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate1_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate1;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE1_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate2_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate2;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE2_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate3_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate3;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE3_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate4_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate4;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE4_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate5_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate5;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE5_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate6_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate6;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE6_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate7_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate7;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE7_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate8_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate8;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE8_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate9_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate9;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE9_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate10_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate10;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE10_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate11_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate11;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE11_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate12_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate12;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE12_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate13_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate13;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE13_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate14_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate14;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE14_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate15_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate15;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE15_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate16_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate16;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE16_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate17_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate17;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE17_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate18_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate18;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE18_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate19_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate19;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE19_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate20_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate20;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE20_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate21_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate21;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE21_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate22_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate22;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE22_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate23_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate23;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE23_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate24_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate24;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE24_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate25_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate25;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE25_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate26_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate26;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE26_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate27_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate27;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE27_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate28_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate28;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE28_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate29_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate29;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE29_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate30_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate30;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE30_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate31_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate31;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE31_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate32_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate32;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE32_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate33_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate33;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE33_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate34_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate34;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE34_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate35_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate35;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE35_T;

typedef struct dpp_etm_qmu_cfg_qsch_mulcrdcntrate36_t
{
    ZXIC_UINT32 cfg_qsch_mulcrdcntrate36;
}DPP_ETM_QMU_CFG_QSCH_MULCRDCNTRATE36_T;

typedef struct dpp_etm_qmu_cfg_qsch_rm_mul_mcn1_rand_hold_shift_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn1_rand_hold_shift;
}DPP_ETM_QMU_CFG_QSCH_RM_MUL_MCN1_RAND_HOLD_SHIFT_T;

typedef struct dpp_etm_qmu_cfg_qsch_rm_mul_mcn2_rand_hold_shift_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn2_rand_hold_shift;
}DPP_ETM_QMU_CFG_QSCH_RM_MUL_MCN2_RAND_HOLD_SHIFT_T;

typedef struct dpp_etm_qmu_cfg_qsch_rm_mul_mcn3_rand_hold_shift_t
{
    ZXIC_UINT32 cfg_qsch_rm_mul_mcn3_rand_hold_shift;
}DPP_ETM_QMU_CFG_QSCH_RM_MUL_MCN3_RAND_HOLD_SHIFT_T;

typedef struct dpp_etm_qmu_last_drop_qnum_get_t
{
    ZXIC_UINT32 cgavd_qmu_drop_tap;
    ZXIC_UINT32 last_drop_qnum;
}DPP_ETM_QMU_LAST_DROP_QNUM_GET_T;

typedef struct dpp_etm_qmu_crdt_qmu_credit_cnt_t
{
    ZXIC_UINT32 crdt_qmu_credit_cnt;
}DPP_ETM_QMU_CRDT_QMU_CREDIT_CNT_T;

typedef struct dpp_etm_qmu_qmu_to_qsch_report_cnt_t
{
    ZXIC_UINT32 qmu_to_qsch_report_cnt;
}DPP_ETM_QMU_QMU_TO_QSCH_REPORT_CNT_T;

typedef struct dpp_etm_qmu_qmu_to_cgavd_report_cnt_t
{
    ZXIC_UINT32 qmu_to_cgavd_report_cnt;
}DPP_ETM_QMU_QMU_TO_CGAVD_REPORT_CNT_T;

typedef struct dpp_etm_qmu_qmu_crdt_crs_normal_cnt_t
{
    ZXIC_UINT32 qmu_crdt_crs_normal_cnt;
}DPP_ETM_QMU_QMU_CRDT_CRS_NORMAL_CNT_T;

typedef struct dpp_etm_qmu_qmu_crdt_crs_off_cnt_t
{
    ZXIC_UINT32 qmu_crdt_crs_off_cnt;
}DPP_ETM_QMU_QMU_CRDT_CRS_OFF_CNT_T;

typedef struct dpp_etm_qmu_qsch_qlist_shedule_cnt_t
{
    ZXIC_UINT32 qsch_qlist_shedule_cnt;
}DPP_ETM_QMU_QSCH_QLIST_SHEDULE_CNT_T;

typedef struct dpp_etm_qmu_qsch_qlist_sch_ept_cnt_t
{
    ZXIC_UINT32 qsch_qlist_sch_ept_cnt;
}DPP_ETM_QMU_QSCH_QLIST_SCH_EPT_CNT_T;

typedef struct dpp_etm_qmu_qmu_to_mmu_blk_wr_cnt_t
{
    ZXIC_UINT32 qmu_to_mmu_blk_wr_cnt;
}DPP_ETM_QMU_QMU_TO_MMU_BLK_WR_CNT_T;

typedef struct dpp_etm_qmu_qmu_to_csw_blk_rd_cnt_t
{
    ZXIC_UINT32 qmu_to_csw_blk_rd_cnt;
}DPP_ETM_QMU_QMU_TO_CSW_BLK_RD_CNT_T;

typedef struct dpp_etm_qmu_qmu_to_mmu_sop_wr_cnt_t
{
    ZXIC_UINT32 qmu_to_mmu_sop_wr_cnt;
}DPP_ETM_QMU_QMU_TO_MMU_SOP_WR_CNT_T;

typedef struct dpp_etm_qmu_qmu_to_mmu_eop_wr_cnt_t
{
    ZXIC_UINT32 qmu_to_mmu_eop_wr_cnt;
}DPP_ETM_QMU_QMU_TO_MMU_EOP_WR_CNT_T;

typedef struct dpp_etm_qmu_qmu_to_mmu_drop_wr_cnt_t
{
    ZXIC_UINT32 qmu_to_mmu_drop_wr_cnt;
}DPP_ETM_QMU_QMU_TO_MMU_DROP_WR_CNT_T;

typedef struct dpp_etm_qmu_qmu_to_csw_sop_rd_cnt_t
{
    ZXIC_UINT32 qmu_to_csw_sop_rd_cnt;
}DPP_ETM_QMU_QMU_TO_CSW_SOP_RD_CNT_T;

typedef struct dpp_etm_qmu_qmu_to_csw_eop_rd_cnt_t
{
    ZXIC_UINT32 qmu_to_csw_eop_rd_cnt;
}DPP_ETM_QMU_QMU_TO_CSW_EOP_RD_CNT_T;

typedef struct dpp_etm_qmu_qmu_to_csw_drop_rd_cnt_t
{
    ZXIC_UINT32 qmu_to_csw_drop_rd_cnt;
}DPP_ETM_QMU_QMU_TO_CSW_DROP_RD_CNT_T;

typedef struct dpp_etm_qmu_mmu_to_qmu_wr_release_cnt_t
{
    ZXIC_UINT32 mmu_to_qmu_wr_release_cnt;
}DPP_ETM_QMU_MMU_TO_QMU_WR_RELEASE_CNT_T;

typedef struct dpp_etm_qmu_mmu_to_qmu_rd_release_cnt_t
{
    ZXIC_UINT32 mmu_to_qmu_rd_release_cnt;
}DPP_ETM_QMU_MMU_TO_QMU_RD_RELEASE_CNT_T;

typedef struct dpp_etm_qmu_observe_qnum_set_t
{
    ZXIC_UINT32 observe_qnum_set;
}DPP_ETM_QMU_OBSERVE_QNUM_SET_T;

typedef struct dpp_etm_qmu_spec_q_pkt_received_t
{
    ZXIC_UINT32 spec_q_pkt_received;
}DPP_ETM_QMU_SPEC_Q_PKT_RECEIVED_T;

typedef struct dpp_etm_qmu_spec_q_pkt_dropped_t
{
    ZXIC_UINT32 spec_q_pkt_dropped;
}DPP_ETM_QMU_SPEC_Q_PKT_DROPPED_T;

typedef struct dpp_etm_qmu_spec_q_pkt_scheduled_t
{
    ZXIC_UINT32 spec_q_pkt_scheduled;
}DPP_ETM_QMU_SPEC_Q_PKT_SCHEDULED_T;

typedef struct dpp_etm_qmu_spec_q_wr_cmd_sent_t
{
    ZXIC_UINT32 spec_q_wr_cmd_sent;
}DPP_ETM_QMU_SPEC_Q_WR_CMD_SENT_T;

typedef struct dpp_etm_qmu_spec_q_rd_cmd_sent_t
{
    ZXIC_UINT32 spec_q_rd_cmd_sent;
}DPP_ETM_QMU_SPEC_Q_RD_CMD_SENT_T;

typedef struct dpp_etm_qmu_spec_q_pkt_enq_t
{
    ZXIC_UINT32 spec_q_pkt_enq;
}DPP_ETM_QMU_SPEC_Q_PKT_ENQ_T;

typedef struct dpp_etm_qmu_spec_q_pkt_deq_t
{
    ZXIC_UINT32 spec_q_pkt_deq;
}DPP_ETM_QMU_SPEC_Q_PKT_DEQ_T;

typedef struct dpp_etm_qmu_spec_q_crdt_uncon_received_t
{
    ZXIC_UINT32 spec_q_crdt_uncon_received;
}DPP_ETM_QMU_SPEC_Q_CRDT_UNCON_RECEIVED_T;

typedef struct dpp_etm_qmu_spec_q_crdt_cong_received_t
{
    ZXIC_UINT32 spec_q_crdt_cong_received;
}DPP_ETM_QMU_SPEC_Q_CRDT_CONG_RECEIVED_T;

typedef struct dpp_etm_qmu_spec_q_crs_normal_cnt_t
{
    ZXIC_UINT32 spec_q_crs_normal_cnt;
}DPP_ETM_QMU_SPEC_Q_CRS_NORMAL_CNT_T;

typedef struct dpp_etm_qmu_spec_q_crs_off_cnt_t
{
    ZXIC_UINT32 spec_q_crs_off_cnt;
}DPP_ETM_QMU_SPEC_Q_CRS_OFF_CNT_T;

typedef struct dpp_etm_qmu_observe_batch_set_t
{
    ZXIC_UINT32 observe_batch_set;
}DPP_ETM_QMU_OBSERVE_BATCH_SET_T;

typedef struct dpp_etm_qmu_spec_bat_pkt_received_t
{
    ZXIC_UINT32 spec_bat_pkt_received;
}DPP_ETM_QMU_SPEC_BAT_PKT_RECEIVED_T;

typedef struct dpp_etm_qmu_spec_bat_pkt_dropped_t
{
    ZXIC_UINT32 spec_bat_pkt_dropped;
}DPP_ETM_QMU_SPEC_BAT_PKT_DROPPED_T;

typedef struct dpp_etm_qmu_spec_bat_blk_scheduled_t
{
    ZXIC_UINT32 spec_bat_blk_scheduled;
}DPP_ETM_QMU_SPEC_BAT_BLK_SCHEDULED_T;

typedef struct dpp_etm_qmu_spec_bat_wr_cmd_sent_t
{
    ZXIC_UINT32 spec_bat_wr_cmd_sent;
}DPP_ETM_QMU_SPEC_BAT_WR_CMD_SENT_T;

typedef struct dpp_etm_qmu_spec_bat_rd_cmd_sent_t
{
    ZXIC_UINT32 spec_bat_rd_cmd_sent;
}DPP_ETM_QMU_SPEC_BAT_RD_CMD_SENT_T;

typedef struct dpp_etm_qmu_spec_bat_pkt_enq_t
{
    ZXIC_UINT32 spec_bat_pkt_enq;
}DPP_ETM_QMU_SPEC_BAT_PKT_ENQ_T;

typedef struct dpp_etm_qmu_spec_bat_pkt_deq_t
{
    ZXIC_UINT32 spec_bat_pkt_deq;
}DPP_ETM_QMU_SPEC_BAT_PKT_DEQ_T;

typedef struct dpp_etm_qmu_spec_bat_crdt_uncon_received_t
{
    ZXIC_UINT32 spec_bat_crdt_uncon_received;
}DPP_ETM_QMU_SPEC_BAT_CRDT_UNCON_RECEIVED_T;

typedef struct dpp_etm_qmu_spec_bat_crdt_cong_received_t
{
    ZXIC_UINT32 spec_bat_crdt_cong_received;
}DPP_ETM_QMU_SPEC_BAT_CRDT_CONG_RECEIVED_T;

typedef struct dpp_etm_qmu_spec_bat_crs_normal_cnt_t
{
    ZXIC_UINT32 spec_bat_crs_normal_cnt;
}DPP_ETM_QMU_SPEC_BAT_CRS_NORMAL_CNT_T;

typedef struct dpp_etm_qmu_spec_bat_crs_off_cnt_t
{
    ZXIC_UINT32 spec_bat_crs_off_cnt;
}DPP_ETM_QMU_SPEC_BAT_CRS_OFF_CNT_T;

typedef struct dpp_etm_qmu_bcntm_ovfl_qnum_get_t
{
    ZXIC_UINT32 bcntm_ovfl_qnum_get;
}DPP_ETM_QMU_BCNTM_OVFL_QNUM_GET_T;

typedef struct dpp_etm_qmu_crbal_a_ovf_qnum_get_t
{
    ZXIC_UINT32 crbal_a_ovf_qnum_get;
}DPP_ETM_QMU_CRBAL_A_OVF_QNUM_GET_T;

typedef struct dpp_etm_qmu_crbal_b_ovf_qnum_get_t
{
    ZXIC_UINT32 crbal_b_ovf_qnum_get;
}DPP_ETM_QMU_CRBAL_B_OVF_QNUM_GET_T;

typedef struct dpp_etm_qmu_crbal_drop_qnum_get_t
{
    ZXIC_UINT32 crbal_drop_qnum_get;
}DPP_ETM_QMU_CRBAL_DROP_QNUM_GET_T;

typedef struct dpp_etm_qmu_deq_flg_report_cnt_t
{
    ZXIC_UINT32 deq_flg_report_cnt;
}DPP_ETM_QMU_DEQ_FLG_REPORT_CNT_T;

typedef struct dpp_etm_qmu_spec_q_crs_get_t
{
    ZXIC_UINT32 spec_q_crs_get;
}DPP_ETM_QMU_SPEC_Q_CRS_GET_T;

typedef struct dpp_etm_qmu_spec_q_crs_in_get_t
{
    ZXIC_UINT32 spec_q_crs_in_get;
}DPP_ETM_QMU_SPEC_Q_CRS_IN_GET_T;

typedef struct dpp_etm_qmu_spec_q_crs_flg_csol_get_t
{
    ZXIC_UINT32 spec_q_crs_flg_csol_get;
}DPP_ETM_QMU_SPEC_Q_CRS_FLG_CSOL_GET_T;

typedef struct dpp_etm_qmu_ept_sch_qnum_get_t
{
    ZXIC_UINT32 ept_sch_qnum_get;
}DPP_ETM_QMU_EPT_SCH_QNUM_GET_T;


#ifdef __cplusplus
}
#endif
#endif

