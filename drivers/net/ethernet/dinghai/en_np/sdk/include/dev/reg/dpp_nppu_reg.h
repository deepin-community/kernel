
#ifndef _DPP_NPPU_REG_H_
#define _DPP_NPPU_REG_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef struct dpp_nppu_mr_cfg_cfg_shap_param_t
{
    ZXIC_UINT32 shap_en;
    ZXIC_UINT32 shap_rate;
}DPP_NPPU_MR_CFG_CFG_SHAP_PARAM_T;

typedef struct dpp_nppu_mr_cfg_cfg_shap_token_t
{
    ZXIC_UINT32 cfg_shap_plen_offset;
    ZXIC_UINT32 cfg_shap_token;
}DPP_NPPU_MR_CFG_CFG_SHAP_TOKEN_T;

typedef struct dpp_nppu_mr_cfg_idle_ptr_fifo_aful_th_t
{
    ZXIC_UINT32 idle_ptr3_fifo_aful_th;
    ZXIC_UINT32 idle_ptr2_fifo_aful_th;
    ZXIC_UINT32 idle_ptr1_fifo_aful_th;
    ZXIC_UINT32 idle_ptr0_fifo_aful_th;
}DPP_NPPU_MR_CFG_IDLE_PTR_FIFO_AFUL_TH_T;

typedef struct dpp_nppu_mr_cfg_mr_cos_port_cfg_t
{
    ZXIC_UINT32 cos3_port_cfg;
    ZXIC_UINT32 cos2_port_cfg;
    ZXIC_UINT32 cos1_port_cfg;
    ZXIC_UINT32 cos0_port_cfg;
}DPP_NPPU_MR_CFG_MR_COS_PORT_CFG_T;

typedef struct dpp_nppu_pktrx_cfg_ind_status_t
{
    ZXIC_UINT32 ind_access_done;
}DPP_NPPU_PKTRX_CFG_IND_STATUS_T;

typedef struct dpp_nppu_pktrx_cfg_ind_cmd_t
{
    ZXIC_UINT32 ind_rd_or_wr;
    ZXIC_UINT32 ind_mem_id;
    ZXIC_UINT32 ind_mem_addr;
}DPP_NPPU_PKTRX_CFG_IND_CMD_T;

typedef struct dpp_nppu_pktrx_cfg_ind_data0_t
{
    ZXIC_UINT32 ind_dat0;
}DPP_NPPU_PKTRX_CFG_IND_DATA0_T;

typedef struct dpp_nppu_pktrx_cfg_ind_data1_t
{
    ZXIC_UINT32 ind_dat1;
}DPP_NPPU_PKTRX_CFG_IND_DATA1_T;

typedef struct dpp_nppu_pktrx_cfg_ind_data2_t
{
    ZXIC_UINT32 ind_dat2;
}DPP_NPPU_PKTRX_CFG_IND_DATA2_T;

typedef struct dpp_nppu_pktrx_cfg_ind_data3_t
{
    ZXIC_UINT32 ind_dat3;
}DPP_NPPU_PKTRX_CFG_IND_DATA3_T;

typedef struct dpp_nppu_pktrx_cfg_ind_data4_t
{
    ZXIC_UINT32 ind_dat4;
}DPP_NPPU_PKTRX_CFG_IND_DATA4_T;

typedef struct dpp_nppu_pktrx_cfg_ind_data5_t
{
    ZXIC_UINT32 ind_dat5;
}DPP_NPPU_PKTRX_CFG_IND_DATA5_T;

typedef struct dpp_nppu_pktrx_cfg_ind_data6_t
{
    ZXIC_UINT32 ind_dat6;
}DPP_NPPU_PKTRX_CFG_IND_DATA6_T;

typedef struct dpp_nppu_pktrx_cfg_ind_data7_t
{
    ZXIC_UINT32 ind_dat7;
}DPP_NPPU_PKTRX_CFG_IND_DATA7_T;

typedef struct dpp_nppu_pktrx_cfg_tcam_0_cmd_t
{
    ZXIC_UINT32 cfg_vben;
    ZXIC_UINT32 cfg_vbi;
    ZXIC_UINT32 cfg_t_strwc;
    ZXIC_UINT32 tcam0_sm;
    ZXIC_UINT32 tcam0_smen;
    ZXIC_UINT32 tcam0_rm;
    ZXIC_UINT32 tcam0_rmen;
    ZXIC_UINT32 tcam0_enable;
    ZXIC_UINT32 tcam0_flush;
    ZXIC_UINT32 tcam0_unload;
    ZXIC_UINT32 tcam0_unload_addr;
}DPP_NPPU_PKTRX_CFG_TCAM_0_CMD_T;

typedef struct dpp_nppu_pktrx_cfg_tcam_1_cmd_t
{
    ZXIC_UINT32 tcam1_sm;
    ZXIC_UINT32 tcam1_smen;
    ZXIC_UINT32 tcam1_rm;
    ZXIC_UINT32 tcam1_rmen;
    ZXIC_UINT32 tcam1_enable;
    ZXIC_UINT32 tcam1_flush;
    ZXIC_UINT32 tcam1_unload;
    ZXIC_UINT32 tcam1_unload_addr;
}DPP_NPPU_PKTRX_CFG_TCAM_1_CMD_T;

typedef struct dpp_nppu_pktrx_cfg_port_en_0_t
{
    ZXIC_UINT32 cfg_isch_port_en_0;
}DPP_NPPU_PKTRX_CFG_PORT_EN_0_T;

typedef struct dpp_nppu_pktrx_cfg_port_en_1_t
{
    ZXIC_UINT32 cfg_isch_port_en_1;
}DPP_NPPU_PKTRX_CFG_PORT_EN_1_T;

typedef struct dpp_nppu_pktrx_cfg_port_en_2_t
{
    ZXIC_UINT32 cfg_isch_port_en_2;
}DPP_NPPU_PKTRX_CFG_PORT_EN_2_T;

typedef struct dpp_nppu_pktrx_cfg_port_en_3_t
{
    ZXIC_UINT32 cfg_port_change_en_0;
    ZXIC_UINT32 cfg_port_change_en_1;
    ZXIC_UINT32 cfg_isch_port_en_3;
}DPP_NPPU_PKTRX_CFG_PORT_EN_3_T;

typedef struct dpp_nppu_pktrx_cfg_cfg_port_l2_offset_mode_0_t
{
    ZXIC_UINT32 cfg_port_l2_offset_mode_0;
}DPP_NPPU_PKTRX_CFG_CFG_PORT_L2_OFFSET_MODE_0_T;

typedef struct dpp_nppu_pktrx_cfg_cfg_port_l2_offset_mode_1_t
{
    ZXIC_UINT32 cfg_port_l2_offset_mode_1;
}DPP_NPPU_PKTRX_CFG_CFG_PORT_L2_OFFSET_MODE_1_T;

typedef struct dpp_nppu_pktrx_cfg_cfg_port_l2_offset_mode_2_t
{
    ZXIC_UINT32 cfg_port_l2_offset_mode_2;
}DPP_NPPU_PKTRX_CFG_CFG_PORT_L2_OFFSET_MODE_2_T;

typedef struct dpp_nppu_pktrx_cfg_cfg_port_l2_offset_mode_3_t
{
    ZXIC_UINT32 cfg_port_l2_offset_mode_3;
}DPP_NPPU_PKTRX_CFG_CFG_PORT_L2_OFFSET_MODE_3_T;

typedef struct dpp_nppu_pktrx_cfg_port_fc_mode_0_t
{
    ZXIC_UINT32 cfg_isch_fc_mode_0;
}DPP_NPPU_PKTRX_CFG_PORT_FC_MODE_0_T;

typedef struct dpp_nppu_pktrx_cfg_port_fc_mode_1_t
{
    ZXIC_UINT32 cfg_isch_fc_mode_1;
}DPP_NPPU_PKTRX_CFG_PORT_FC_MODE_1_T;

typedef struct dpp_nppu_pktrx_cfg_port_fc_mode_2_t
{
    ZXIC_UINT32 cfg_isch_fc_mode_2;
}DPP_NPPU_PKTRX_CFG_PORT_FC_MODE_2_T;

typedef struct dpp_nppu_pktrx_cfg_port_fc_mode_3_t
{
    ZXIC_UINT32 cfg_isch_fc_mode_3;
}DPP_NPPU_PKTRX_CFG_PORT_FC_MODE_3_T;

typedef struct dpp_nppu_pktrx_cfg_port_fc_mode_4_t
{
    ZXIC_UINT32 cfg_isch_fc_mode_4;
}DPP_NPPU_PKTRX_CFG_PORT_FC_MODE_4_T;

typedef struct dpp_nppu_pktrx_cfg_port_fc_mode_5_t
{
    ZXIC_UINT32 cfg_isch_fc_mode_5;
}DPP_NPPU_PKTRX_CFG_PORT_FC_MODE_5_T;

typedef struct dpp_nppu_pktrx_cfg_port_fc_mode_6_t
{
    ZXIC_UINT32 cfg_isch_fc_mode_6;
}DPP_NPPU_PKTRX_CFG_PORT_FC_MODE_6_T;

typedef struct dpp_nppu_pktrx_cfg_port_fc_mode_7_t
{
    ZXIC_UINT32 cfg_pfu_aging_en;
    ZXIC_UINT32 cfg_isch_aging_en;
    ZXIC_UINT32 cfg_isch_fc_mode_7;
}DPP_NPPU_PKTRX_CFG_PORT_FC_MODE_7_T;

typedef struct dpp_nppu_pktrx_cfg_cfg_isch_aging_th_t
{
    ZXIC_UINT32 cfg_pfu_delay_cycle;
    ZXIC_UINT32 cfg_isch_aging_th;
}DPP_NPPU_PKTRX_CFG_CFG_ISCH_AGING_TH_T;

typedef struct dpp_nppu_pktrx_cfg_isch_fifo_th_0_t
{
    ZXIC_UINT32 cfg_sch_fifo3_fc_th;
    ZXIC_UINT32 cfg_sch_fifo2_fc_th;
    ZXIC_UINT32 cfg_sch_fifo1_fc_th;
    ZXIC_UINT32 cfg_sch_fifo0_fc_th;
}DPP_NPPU_PKTRX_CFG_ISCH_FIFO_TH_0_T;

typedef struct dpp_nppu_pktrx_cfg_isch_cfg_1_t
{
    ZXIC_UINT32 cfg_parser_max_len_en;
    ZXIC_UINT32 cfg_parser_max_len;
    ZXIC_UINT32 cfg_parser_min_len_en;
    ZXIC_UINT32 cfg_parser_min_len;
    ZXIC_UINT32 sp_sch_sel;
}DPP_NPPU_PKTRX_CFG_ISCH_CFG_1_T;

typedef struct dpp_nppu_pktrx_cfg_tcam_0_vld_t
{
    ZXIC_UINT32 cfg_tcam0_vld;
}DPP_NPPU_PKTRX_CFG_TCAM_0_VLD_T;

typedef struct dpp_nppu_pktrx_cfg_tcam_1_vld_t
{
    ZXIC_UINT32 cfg_tcam1_vld;
}DPP_NPPU_PKTRX_CFG_TCAM_1_VLD_T;

typedef struct dpp_nppu_pktrx_cfg_cpu_port_en_mask_t
{
    ZXIC_UINT32 cpu_port_en_mask;
}DPP_NPPU_PKTRX_CFG_CPU_PORT_EN_MASK_T;

typedef struct dpp_nppu_pktrx_cfg_pktrx_glbal_cfg_0_t
{
    ZXIC_UINT32 pktrx_glbal_cfg_0;
}DPP_NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_0_T;

typedef struct dpp_nppu_pktrx_cfg_pktrx_glbal_cfg_1_t
{
    ZXIC_UINT32 pktrx_glbal_cfg_1;
}DPP_NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_1_T;

typedef struct dpp_nppu_pktrx_cfg_pktrx_glbal_cfg_2_t
{
    ZXIC_UINT32 pktrx_glbal_cfg_2;
}DPP_NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_2_T;

typedef struct dpp_nppu_pktrx_cfg_pktrx_glbal_cfg_3_t
{
    ZXIC_UINT32 pktrx_glbal_cfg_3;
}DPP_NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_3_T;

typedef struct dpp_nppu_pktrx_cfg_nppu_start_t
{
    ZXIC_UINT32 nppu_start;
}DPP_NPPU_PKTRX_CFG_NPPU_START_T;

typedef struct dpp_nppu_pktrx_stat_ind_status_t
{
    ZXIC_UINT32 ind_access_done;
}DPP_NPPU_PKTRX_STAT_IND_STATUS_T;

typedef struct dpp_nppu_pktrx_stat_ind_cmd_t
{
    ZXIC_UINT32 ind_rd_or_wr;
    ZXIC_UINT32 ind_mem_id;
    ZXIC_UINT32 ind_mem_addr;
}DPP_NPPU_PKTRX_STAT_IND_CMD_T;

typedef struct dpp_nppu_pktrx_stat_ind_data0_t
{
    ZXIC_UINT32 ind_dat0;
}DPP_NPPU_PKTRX_STAT_IND_DATA0_T;

typedef struct dpp_nppu_idma_cfg_debug_cnt_ovfl_mode_t
{
    ZXIC_UINT32 debug_cnt_ovfl_mode;
}DPP_NPPU_IDMA_CFG_DEBUG_CNT_OVFL_MODE_T;

typedef struct dpp_nppu_idma_stat_ind_status_t
{
    ZXIC_UINT32 ind_access_done;
}DPP_NPPU_IDMA_STAT_IND_STATUS_T;

typedef struct dpp_nppu_idma_stat_ind_cmd_t
{
    ZXIC_UINT32 ind_rd_or_wr;
    ZXIC_UINT32 ind_mem_id;
    ZXIC_UINT32 ind_mem_addr;
}DPP_NPPU_IDMA_STAT_IND_CMD_T;

typedef struct dpp_nppu_idma_stat_ind_data0_t
{
    ZXIC_UINT32 ind_data0;
}DPP_NPPU_IDMA_STAT_IND_DATA0_T;

typedef struct dpp_nppu_pbu_cfg_ind_status_t
{
    ZXIC_UINT32 ind_access_done;
}DPP_NPPU_PBU_CFG_IND_STATUS_T;

typedef struct dpp_nppu_pbu_cfg_ind_cmd_t
{
    ZXIC_UINT32 ind_rd_or_wr;
    ZXIC_UINT32 ind_mem_id;
    ZXIC_UINT32 ind_mem_addr;
}DPP_NPPU_PBU_CFG_IND_CMD_T;

typedef struct dpp_nppu_pbu_cfg_ind_data0_t
{
    ZXIC_UINT32 ind_data0;
}DPP_NPPU_PBU_CFG_IND_DATA0_T;

typedef struct dpp_nppu_pbu_cfg_ind_data1_t
{
    ZXIC_UINT32 ind_data1;
}DPP_NPPU_PBU_CFG_IND_DATA1_T;

typedef struct dpp_nppu_pbu_cfg_ind_data2_t
{
    ZXIC_UINT32 ind_data2;
}DPP_NPPU_PBU_CFG_IND_DATA2_T;

typedef struct dpp_nppu_pbu_cfg_ind_data3_t
{
    ZXIC_UINT32 ind_data3;
}DPP_NPPU_PBU_CFG_IND_DATA3_T;

typedef struct dpp_nppu_pbu_cfg_ind_data4_t
{
    ZXIC_UINT32 ind_data4;
}DPP_NPPU_PBU_CFG_IND_DATA4_T;

typedef struct dpp_nppu_pbu_cfg_ind_data5_t
{
    ZXIC_UINT32 ind_data5;
}DPP_NPPU_PBU_CFG_IND_DATA5_T;

typedef struct dpp_nppu_pbu_cfg_ind_data6_t
{
    ZXIC_UINT32 ind_data6;
}DPP_NPPU_PBU_CFG_IND_DATA6_T;

typedef struct dpp_nppu_pbu_cfg_ind_data7_t
{
    ZXIC_UINT32 ind_data7;
}DPP_NPPU_PBU_CFG_IND_DATA7_T;

typedef struct dpp_nppu_pbu_cfg_idma_public_th_t
{
    ZXIC_UINT32 idma_public_th;
}DPP_NPPU_PBU_CFG_IDMA_PUBLIC_TH_T;

typedef struct dpp_nppu_pbu_cfg_lif_public_th_t
{
    ZXIC_UINT32 lif_public_th;
}DPP_NPPU_PBU_CFG_LIF_PUBLIC_TH_T;

typedef struct dpp_nppu_pbu_cfg_idma_total_th_t
{
    ZXIC_UINT32 idma_total_th;
}DPP_NPPU_PBU_CFG_IDMA_TOTAL_TH_T;

typedef struct dpp_nppu_pbu_cfg_lif_total_th_t
{
    ZXIC_UINT32 lif_total_th;
}DPP_NPPU_PBU_CFG_LIF_TOTAL_TH_T;

typedef struct dpp_nppu_pbu_cfg_mc_total_th_t
{
    ZXIC_UINT32 mc_total_th;
}DPP_NPPU_PBU_CFG_MC_TOTAL_TH_T;

typedef struct dpp_nppu_pbu_cfg_mc_cos10_th_t
{
    ZXIC_UINT32 mc_cos1_mode;
    ZXIC_UINT32 mc_cos0_mode;
    ZXIC_UINT32 mc_cos1_th;
    ZXIC_UINT32 mc_cos0_th;
}DPP_NPPU_PBU_CFG_MC_COS10_TH_T;

typedef struct dpp_nppu_pbu_cfg_mc_cos32_th_t
{
    ZXIC_UINT32 mc_cos3_mode;
    ZXIC_UINT32 mc_cos2_mode;
    ZXIC_UINT32 mc_cos3_th;
    ZXIC_UINT32 mc_cos2_th;
}DPP_NPPU_PBU_CFG_MC_COS32_TH_T;

typedef struct dpp_nppu_pbu_cfg_mc_cos54_th_t
{
    ZXIC_UINT32 mc_cos5_mode;
    ZXIC_UINT32 mc_cos4_mode;
    ZXIC_UINT32 mc_cos5_th;
    ZXIC_UINT32 mc_cos4_th;
}DPP_NPPU_PBU_CFG_MC_COS54_TH_T;

typedef struct dpp_nppu_pbu_cfg_mc_cos76_th_t
{
    ZXIC_UINT32 mc_cos7_mode;
    ZXIC_UINT32 mc_cos6_mode;
    ZXIC_UINT32 mc_cos7_th;
    ZXIC_UINT32 mc_cos6_th;
}DPP_NPPU_PBU_CFG_MC_COS76_TH_T;

typedef struct dpp_nppu_pbu_cfg_debug_cnt_ovfl_mode_t
{
    ZXIC_UINT32 debug_cnt_ovfl_mode;
}DPP_NPPU_PBU_CFG_DEBUG_CNT_OVFL_MODE_T;

typedef struct dpp_nppu_pbu_cfg_se_key_aful_negate_cfg_t
{
    ZXIC_UINT32 se_key_aful_negate_cfg;
}DPP_NPPU_PBU_CFG_SE_KEY_AFUL_NEGATE_CFG_T;

typedef struct dpp_nppu_pbu_cfg_sa_flag_t
{
    ZXIC_UINT32 sa_flag;
}DPP_NPPU_PBU_CFG_SA_FLAG_T;

typedef struct dpp_nppu_pbu_stat_ind_data_t
{
    ZXIC_UINT32 ind_data;
}DPP_NPPU_PBU_STAT_IND_DATA_T;

typedef struct dpp_nppu_pbu_stat_ind_status_t
{
    ZXIC_UINT32 ind_access_done;
}DPP_NPPU_PBU_STAT_IND_STATUS_T;

typedef struct dpp_nppu_pbu_stat_ind_cmd_t
{
    ZXIC_UINT32 ind_rd_or_wr;
    ZXIC_UINT32 ind_mem_id;
    ZXIC_UINT32 ind_mem_addr;
}DPP_NPPU_PBU_STAT_IND_CMD_T;

typedef struct dpp_nppu_pbu_stat_total_cnt_t
{
    ZXIC_UINT32 total_cnt;
}DPP_NPPU_PBU_STAT_TOTAL_CNT_T;

typedef struct dpp_nppu_pbu_stat_idma_pub_cnt_t
{
    ZXIC_UINT32 idma_pub_cnt;
}DPP_NPPU_PBU_STAT_IDMA_PUB_CNT_T;

typedef struct dpp_nppu_pbu_stat_lif_pub_cnt_t
{
    ZXIC_UINT32 lif_pub_cnt;
}DPP_NPPU_PBU_STAT_LIF_PUB_CNT_T;

typedef struct dpp_nppu_pbu_stat_mc_total_cnt_t
{
    ZXIC_UINT32 mc_total_cnt;
}DPP_NPPU_PBU_STAT_MC_TOTAL_CNT_T;

typedef struct dpp_nppu_pbu_stat_pbu_thram_init_done_t
{
    ZXIC_UINT32 pbu_thram_init_done;
}DPP_NPPU_PBU_STAT_PBU_THRAM_INIT_DONE_T;

typedef struct dpp_nppu_pbu_stat_ifb_fptr_init_done_t
{
    ZXIC_UINT32 ifb_fptr_init_done;
}DPP_NPPU_PBU_STAT_IFB_FPTR_INIT_DONE_T;

typedef struct dpp_nppu_isu_cfg_weight_normal_uc_t
{
    ZXIC_UINT32 weight_normal_uc;
}DPP_NPPU_ISU_CFG_WEIGHT_NORMAL_UC_T;

typedef struct dpp_nppu_isu_cfg_fabric_or_saip_t
{
    ZXIC_UINT32 fabric_or_saip;
}DPP_NPPU_ISU_CFG_FABRIC_OR_SAIP_T;

typedef struct dpp_nppu_isu_stat_ind_status_t
{
    ZXIC_UINT32 ind_access_done;
}DPP_NPPU_ISU_STAT_IND_STATUS_T;

typedef struct dpp_nppu_isu_stat_ind_cmd_t
{
    ZXIC_UINT32 ind_rd_or_wr;
    ZXIC_UINT32 ind_mem_id;
    ZXIC_UINT32 ind_mem_addr;
}DPP_NPPU_ISU_STAT_IND_CMD_T;

typedef struct dpp_nppu_isu_stat_ind_dat0_t
{
    ZXIC_UINT32 ind_dat0;
}DPP_NPPU_ISU_STAT_IND_DAT0_T;

typedef struct dpp_nppu_odma_cfg_ind_access_done_t
{
    ZXIC_UINT32 ind_access_done;
}DPP_NPPU_ODMA_CFG_IND_ACCESS_DONE_T;

typedef struct dpp_nppu_odma_cfg_ind_command_t
{
    ZXIC_UINT32 ind_rd_or_wr;
    ZXIC_UINT32 ind_mem_id;
    ZXIC_UINT32 ind_mem_addr;
}DPP_NPPU_ODMA_CFG_IND_COMMAND_T;

typedef struct dpp_nppu_odma_cfg_ind_dat0_t
{
    ZXIC_UINT32 ind_dat0;
}DPP_NPPU_ODMA_CFG_IND_DAT0_T;

typedef struct dpp_nppu_odma_cfg_ind_dat1_t
{
    ZXIC_UINT32 ind_dat1;
}DPP_NPPU_ODMA_CFG_IND_DAT1_T;

typedef struct dpp_nppu_odma_cfg_fabric_or_saip_t
{
    ZXIC_UINT32 fabric_or_saip;
}DPP_NPPU_ODMA_CFG_FABRIC_OR_SAIP_T;

typedef struct dpp_nppu_odma_cfg_max_pkt_len_t
{
    ZXIC_UINT32 max_pkt_len;
}DPP_NPPU_ODMA_CFG_MAX_PKT_LEN_T;

typedef struct dpp_nppu_odma_cfg_age_en_t
{
    ZXIC_UINT32 age_en;
}DPP_NPPU_ODMA_CFG_AGE_EN_T;

typedef struct dpp_nppu_odma_cfg_age_mode_t
{
    ZXIC_UINT32 age_mode;
}DPP_NPPU_ODMA_CFG_AGE_MODE_T;

typedef struct dpp_nppu_odma_cfg_age_value_time_t
{
    ZXIC_UINT32 age_value_time;
}DPP_NPPU_ODMA_CFG_AGE_VALUE_TIME_T;

typedef struct dpp_nppu_odma_cfg_age_value_room_t
{
    ZXIC_UINT32 age_value_room;
}DPP_NPPU_ODMA_CFG_AGE_VALUE_ROOM_T;

typedef struct dpp_nppu_odma_cfg_age_out_cnt_t
{
    ZXIC_UINT32 age_out_cnt;
}DPP_NPPU_ODMA_CFG_AGE_OUT_CNT_T;

typedef struct dpp_nppu_odma_cfg_token_value_a_t
{
    ZXIC_UINT32 token_value_a;
}DPP_NPPU_ODMA_CFG_TOKEN_VALUE_A_T;

typedef struct dpp_nppu_odma_cfg_token_value_b_t
{
    ZXIC_UINT32 token_value_b;
}DPP_NPPU_ODMA_CFG_TOKEN_VALUE_B_T;

typedef struct dpp_nppu_odma_cfg_cfg_shap_en_p0_t
{
    ZXIC_UINT32 cfg_shap_en_p0;
}DPP_NPPU_ODMA_CFG_CFG_SHAP_EN_P0_T;

typedef struct dpp_nppu_odma_cfg_cfg_shap_en_p1_t
{
    ZXIC_UINT32 cfg_shap_en_p1;
}DPP_NPPU_ODMA_CFG_CFG_SHAP_EN_P1_T;

typedef struct dpp_nppu_odma_cfg_cfg_shap_en_tm_t
{
    ZXIC_UINT32 cfg_shap_en_tm;
}DPP_NPPU_ODMA_CFG_CFG_SHAP_EN_TM_T;

typedef struct dpp_nppu_odma_stat_ind_status_t
{
    ZXIC_UINT32 ind_access_done;
}DPP_NPPU_ODMA_STAT_IND_STATUS_T;

typedef struct dpp_nppu_odma_stat_ind_cmd_t
{
    ZXIC_UINT32 ind_rd_or_wr;
    ZXIC_UINT32 ind_mem_id;
    ZXIC_UINT32 ind_mem_addr;
}DPP_NPPU_ODMA_STAT_IND_CMD_T;

typedef struct dpp_nppu_odma_stat_ind_data0_t
{
    ZXIC_UINT32 ind_dat0;
}DPP_NPPU_ODMA_STAT_IND_DATA0_T;

typedef struct dpp_nppu_odma_stat_debug_cnt_cfg_t
{
    ZXIC_UINT32 debug_cnt_ovf_mode;
    ZXIC_UINT32 debug_cnt_rdclr_mode;
    ZXIC_UINT32 user_cnt_value;
}DPP_NPPU_ODMA_STAT_DEBUG_CNT_CFG_T;

typedef struct dpp_nppu_oam_cfg_bfd_firstchk_th_t
{
    ZXIC_UINT32 bfd_firstchk_th;
}DPP_NPPU_OAM_CFG_BFD_FIRSTCHK_TH_T;

typedef struct dpp_nppu_pbu_cfg_memid_0_pbu_fc_idmath_ram_t
{
    ZXIC_UINT32 lif_th_15;
    ZXIC_UINT32 lif_prv_15;
    ZXIC_UINT32 idma_prv_15;
    ZXIC_UINT32 idma_th_cos0_15;
    ZXIC_UINT32 idma_th_cos1_15;
    ZXIC_UINT32 idma_th_cos2_15;
    ZXIC_UINT32 idma_th_cos3_15;
    ZXIC_UINT32 idma_th_cos4_15;
    ZXIC_UINT32 idma_th_cos5_15;
    ZXIC_UINT32 idma_th_cos6_15;
    ZXIC_UINT32 idma_th_cos7_15;
}DPP_NPPU_PBU_CFG_MEMID_0_PBU_FC_IDMATH_RAM_T;

typedef struct dpp_nppu_pbu_cfg_memid_1_pbu_fc_macth_ram_t
{
    ZXIC_UINT32 cos7_th;
    ZXIC_UINT32 cos6_th;
    ZXIC_UINT32 cos5_th;
    ZXIC_UINT32 cos4_th;
    ZXIC_UINT32 cos3_th;
    ZXIC_UINT32 cos2_th;
    ZXIC_UINT32 cos1_th;
    ZXIC_UINT32 cos0_th;
}DPP_NPPU_PBU_CFG_MEMID_1_PBU_FC_MACTH_RAM_T;

typedef struct dpp_nppu_pbu_stat_memid_1_all_kind_port_cnt_t
{
    ZXIC_UINT32 peak_port_cnt;
    ZXIC_UINT32 current_port_cnt;
}DPP_NPPU_PBU_STAT_MEMID_1_ALL_KIND_PORT_CNT_T;

typedef struct dpp_nppu_pbu_stat_memid_2_ppu_pbu_ifb_req_vld_cnt_t
{
    ZXIC_UINT32 ppu_pbu_ifb_req_vld_cnt;
}DPP_NPPU_PBU_STAT_MEMID_2_PPU_PBU_IFB_REQ_VLD_CNT_T;

typedef struct dpp_nppu_pbu_stat_memid_2_pbu_ppu_ifb_rsp_vld_cnt_t
{
    ZXIC_UINT32 pbu_ppu_ifb_rsp_vld_cnt;
}DPP_NPPU_PBU_STAT_MEMID_2_PBU_PPU_IFB_RSP_VLD_CNT_T;

typedef struct dpp_nppu_pbu_stat_memid_2_odma_pbu_recy_ptr_vld_cnt_t
{
    ZXIC_UINT32 odma_pbu_recy_ptr_vld_cnt;
}DPP_NPPU_PBU_STAT_MEMID_2_ODMA_PBU_RECY_PTR_VLD_CNT_T;

typedef struct dpp_nppu_pbu_stat_memid_2_ppu_pbu_mcode_pf_req_cnt_t
{
    ZXIC_UINT32 ppu_pbu_mcode_pf_req_cnt;
}DPP_NPPU_PBU_STAT_MEMID_2_PPU_PBU_MCODE_PF_REQ_CNT_T;

typedef struct dpp_nppu_pbu_stat_memid_2_pbu_ppu_mcode_pf_rsp_cnt_t
{
    ZXIC_UINT32 pbu_ppu_mcode_pf_rsp_cnt;
}DPP_NPPU_PBU_STAT_MEMID_2_PBU_PPU_MCODE_PF_RSP_CNT_T;

typedef struct dpp_nppu_pbu_stat_memid_2_ppu_pbu_logic_pf_req_cnt_t
{
    ZXIC_UINT32 ppu_pbu_logic_pf_req_cnt;
}DPP_NPPU_PBU_STAT_MEMID_2_PPU_PBU_LOGIC_PF_REQ_CNT_T;

typedef struct dpp_nppu_pbu_stat_memid_2_pbu_ppu_logic_pf_rsp_cnt_t
{
    ZXIC_UINT32 pbu_ppu_logic_pf_rsp_cnt;
}DPP_NPPU_PBU_STAT_MEMID_2_PBU_PPU_LOGIC_PF_RSP_CNT_T;

typedef struct dpp_nppu_pbu_stat_memid_2_ppu_use_ptr_pulse_cnt_t
{
    ZXIC_UINT32 ppu_use_ptr_pulse_cnt;
}DPP_NPPU_PBU_STAT_MEMID_2_PPU_USE_PTR_PULSE_CNT_T;

typedef struct dpp_nppu_pbu_stat_memid_2_ppu_pbu_wb_vld_cnt_t
{
    ZXIC_UINT32 ppu_pbu_wb_vld_cnt;
}DPP_NPPU_PBU_STAT_MEMID_2_PPU_PBU_WB_VLD_CNT_T;

typedef struct dpp_nppu_pbu_stat_memid_2_pbu_ppu_reorder_para_vld_cnt_t
{
    ZXIC_UINT32 pbu_ppu_reorder_para_vld_cnt;
}DPP_NPPU_PBU_STAT_MEMID_2_PBU_PPU_REORDER_PARA_VLD_CNT_T;

typedef struct dpp_nppu_pbu_stat_memid_2_se_pbu_dpi_key_vld_cnt_t
{
    ZXIC_UINT32 se_pbu_dpi_key_vld_cnt;
}DPP_NPPU_PBU_STAT_MEMID_2_SE_PBU_DPI_KEY_VLD_CNT_T;

typedef struct dpp_nppu_pbu_stat_memid_2_pbu_se_dpi_rsp_datvld_cnt_t
{
    ZXIC_UINT32 pbu_se_dpi_rsp_datvld_cnt;
}DPP_NPPU_PBU_STAT_MEMID_2_PBU_SE_DPI_RSP_DATVLD_CNT_T;

typedef struct dpp_nppu_pbu_stat_memid_2_odma_pbu_ifb_rd1_cnt_t
{
    ZXIC_UINT32 odma_pbu_ifb_rd1_cnt;
}DPP_NPPU_PBU_STAT_MEMID_2_ODMA_PBU_IFB_RD1_CNT_T;

typedef struct dpp_nppu_pbu_stat_memid_2_odma_pbu_ifb_rd2_cnt_t
{
    ZXIC_UINT32 odma_pbu_ifb_rd2_cnt;
}DPP_NPPU_PBU_STAT_MEMID_2_ODMA_PBU_IFB_RD2_CNT_T;

typedef struct dpp_nppu_pbu_stat_memid_2_pbu_ppu_mcode_pf_no_rsp_cnt_t
{
    ZXIC_UINT32 pbu_ppu_mcode_pf_no_rsp_cnt;
}DPP_NPPU_PBU_STAT_MEMID_2_PBU_PPU_MCODE_PF_NO_RSP_CNT_T;

typedef struct dpp_nppu_pbu_stat_memid_2_pbu_ppu_logic_pf_no_rsp_cnt_t
{
    ZXIC_UINT32 pbu_ppu_logic_pf_no_rsp_cnt;
}DPP_NPPU_PBU_STAT_MEMID_2_PBU_PPU_LOGIC_PF_NO_RSP_CNT_T;

typedef struct dpp_nppu_pbu_stat_memid_3_cpu_rd_ifb_data_t
{
    ZXIC_UINT32 cpu_rd_ifb_data;
}DPP_NPPU_PBU_STAT_MEMID_3_CPU_RD_IFB_DATA_T;

typedef struct dpp_nppu_pbu_stat_memid_4_mux_sel_rgt_t
{
    ZXIC_UINT32 current_port_cnt;
}DPP_NPPU_PBU_STAT_MEMID_4_MUX_SEL_RGT_T;

typedef struct dpp_nppu_pbu_stat_memid_5_port_pub_cnt_t
{
    ZXIC_UINT32 port_pub_cnt;
}DPP_NPPU_PBU_STAT_MEMID_5_PORT_PUB_CNT_T;

typedef struct dpp_nppu_idma_stat_memid_1_idma_o_isu_pkt_pulse_total_cnt_t
{
    ZXIC_UINT32 idma_o_isu_pkt_pulse_total_cnt;
}DPP_NPPU_IDMA_STAT_MEMID_1_IDMA_O_ISU_PKT_PULSE_TOTAL_CNT_T;

typedef struct dpp_nppu_idma_stat_memid_1_idma_o_isu_epkt_pulse_total_cnt_t
{
    ZXIC_UINT32 idma_o_isu_epkt_pulse_total_cnt;
}DPP_NPPU_IDMA_STAT_MEMID_1_IDMA_O_ISU_EPKT_PULSE_TOTAL_CNT_T;

typedef struct dpp_nppu_idma_stat_memid_1_idma_dispkt_pulse_total_cnt_t
{
    ZXIC_UINT32 idma_dispkt_pulse_total_cnt;
}DPP_NPPU_IDMA_STAT_MEMID_1_IDMA_DISPKT_PULSE_TOTAL_CNT_T;

typedef struct dpp_nppu_idma_stat_memid_0_idma_o_isu_pkt_pulse_cnt_t
{
    ZXIC_UINT32 idma_o_isu_pkt_pulse_cnt;
}DPP_NPPU_IDMA_STAT_MEMID_0_IDMA_O_ISU_PKT_PULSE_CNT_T;

typedef struct dpp_nppu_idma_stat_memid_0_idma_o_isu_epkt_pulse_cnt_t
{
    ZXIC_UINT32 idma_o_isu_epkt_pulse_cnt;
}DPP_NPPU_IDMA_STAT_MEMID_0_IDMA_O_ISU_EPKT_PULSE_CNT_T;

typedef struct dpp_nppu_idma_stat_memid_0_idma_dispkt_pulse_cnt_t
{
    ZXIC_UINT32 idma_dispkt_pulse_cnt;
}DPP_NPPU_IDMA_STAT_MEMID_0_IDMA_DISPKT_PULSE_CNT_T;

typedef struct dpp_nppu_mr_cfg_ind_access_states_t
{
    ZXIC_UINT32 ind_access_done;
}DPP_NPPU_MR_CFG_IND_ACCESS_STATES_T;

typedef struct dpp_nppu_mr_cfg_ind_access_cmd0_t
{
    ZXIC_UINT32 wr_mode;
    ZXIC_UINT32 rd_or_wr;
    ZXIC_UINT32 ind_access_addr0;
}DPP_NPPU_MR_CFG_IND_ACCESS_CMD0_T;

typedef struct dpp_nppu_mr_cfg_ind_access_data0_t
{
    ZXIC_UINT32 ind_access_data0;
}DPP_NPPU_MR_CFG_IND_ACCESS_DATA0_T;

typedef struct dpp_nppu_mr_cfg_ind_access_data1_t
{
    ZXIC_UINT32 ind_access_data1;
}DPP_NPPU_MR_CFG_IND_ACCESS_DATA1_T;

typedef struct dpp_nppu_mr_cfg_ind_access_cmd1_t
{
    ZXIC_UINT32 ind_access_addr1;
}DPP_NPPU_MR_CFG_IND_ACCESS_CMD1_T;

typedef struct dpp_nppu_mr_cfg_mr_init_done_t
{
    ZXIC_UINT32 mr_init_done;
}DPP_NPPU_MR_CFG_MR_INIT_DONE_T;

typedef struct dpp_nppu_mr_cfg_cnt_mode_reg_t
{
    ZXIC_UINT32 cfgmt_count_rd_mode;
    ZXIC_UINT32 cfgmt_count_overflow_mode;
}DPP_NPPU_MR_CFG_CNT_MODE_REG_T;

typedef struct dpp_nppu_mr_cfg_cfg_ecc_bypass_read_t
{
    ZXIC_UINT32 cfg_ecc_bypass_read;
}DPP_NPPU_MR_CFG_CFG_ECC_BYPASS_READ_T;

typedef struct dpp_nppu_mr_cfg_cfg_rep_mod_t
{
    ZXIC_UINT32 cfg_rep_mod;
}DPP_NPPU_MR_CFG_CFG_REP_MOD_T;

typedef struct dpp_nppu_mr_cfg_block_ptr_fifo_aful_th_t
{
    ZXIC_UINT32 block_ptr3_fifo_aful_th;
    ZXIC_UINT32 block_ptr2_fifo_aful_th;
    ZXIC_UINT32 block_ptr1_fifo_aful_th;
    ZXIC_UINT32 block_ptr0_fifo_aful_th;
}DPP_NPPU_MR_CFG_BLOCK_PTR_FIFO_AFUL_TH_T;

typedef struct dpp_nppu_mr_cfg_pre_rcv_ptr_fifo_aful_th_t
{
    ZXIC_UINT32 pre_rcv_ptr3_fifo_aful_th;
    ZXIC_UINT32 pre_rcv_ptr2_fifo_aful_th;
    ZXIC_UINT32 pre_rcv_ptr1_fifo_aful_th;
    ZXIC_UINT32 pre_rcv_ptr0_fifo_aful_th;
}DPP_NPPU_MR_CFG_PRE_RCV_PTR_FIFO_AFUL_TH_T;

typedef struct dpp_nppu_mr_cfg_mgid_fifo_aful_th_t
{
    ZXIC_UINT32 mgid3_fifo_aful_th;
    ZXIC_UINT32 mgid2_fifo_aful_th;
    ZXIC_UINT32 mgid1_fifo_aful_th;
    ZXIC_UINT32 mgid0_fifo_aful_th;
}DPP_NPPU_MR_CFG_MGID_FIFO_AFUL_TH_T;

typedef struct dpp_nppu_mr_cfg_rep_cmd_fifo_aful_th_t
{
    ZXIC_UINT32 rep_cmd3_fifo_aful_th;
    ZXIC_UINT32 rep_cmd2_fifo_aful_th;
    ZXIC_UINT32 rep_cmd1_fifo_aful_th;
    ZXIC_UINT32 rep_cmd0_fifo_aful_th;
}DPP_NPPU_MR_CFG_REP_CMD_FIFO_AFUL_TH_T;

typedef struct dpp_nppu_mr_cfg_mr_int_mask_1_t
{
    ZXIC_UINT32 free_ptr0_fifo_full_mask;
    ZXIC_UINT32 free_ptr1_fifo_full_mask;
    ZXIC_UINT32 free_ptr2_fifo_full_mask;
    ZXIC_UINT32 free_ptr3_fifo_full_mask;
    ZXIC_UINT32 block_ptr0_fifo_full_mask;
    ZXIC_UINT32 block_ptr1_fifo_full_mask;
    ZXIC_UINT32 block_ptr2_fifo_full_mask;
    ZXIC_UINT32 block_ptr3_fifo_full_mask;
    ZXIC_UINT32 mgid0_fifo_full_mask;
    ZXIC_UINT32 mgid1_fifo_full_mask;
    ZXIC_UINT32 mgid2_fifo_full_mask;
    ZXIC_UINT32 mgid3_fifo_full_mask;
    ZXIC_UINT32 pre_rcv_ptr0_fifo_full_mask;
    ZXIC_UINT32 pre_rcv_ptr1_fifo_full_mask;
    ZXIC_UINT32 pre_rcv_ptr2_fifo_full_mask;
    ZXIC_UINT32 pre_rcv_ptr3_fifo_full_mask;
    ZXIC_UINT32 rep_cmd0_fifo_full_mask;
    ZXIC_UINT32 rep_cmd1_fifo_full_mask;
    ZXIC_UINT32 rep_cmd2_fifo_full_mask;
    ZXIC_UINT32 rep_cmd3_fifo_full_mask;
}DPP_NPPU_MR_CFG_MR_INT_MASK_1_T;

typedef struct dpp_nppu_mr_cfg_mr_int_mask_2_t
{
    ZXIC_UINT32 free_ptr0_fifo_udf_mask;
    ZXIC_UINT32 free_ptr1_fifo_udf_mask;
    ZXIC_UINT32 free_ptr2_fifo_udf_mask;
    ZXIC_UINT32 free_ptr3_fifo_udf_mask;
    ZXIC_UINT32 block_ptr0_fifo_udf_mask;
    ZXIC_UINT32 block_ptr1_fifo_udf_mask;
    ZXIC_UINT32 block_ptr2_fifo_udf_mask;
    ZXIC_UINT32 block_ptr3_fifo_udf_mask;
    ZXIC_UINT32 mgid0_fifo_udf_mask;
    ZXIC_UINT32 mgid1_fifo_udf_mask;
    ZXIC_UINT32 mgid2_fifo_udf_mask;
    ZXIC_UINT32 mgid3_fifo_udf_mask;
    ZXIC_UINT32 pre_rcv_ptr0_fifo_udf_mask;
    ZXIC_UINT32 pre_rcv_ptr1_fifo_udf_mask;
    ZXIC_UINT32 pre_rcv_ptr2_fifo_udf_mask;
    ZXIC_UINT32 pre_rcv_ptr3_fifo_udf_mask;
    ZXIC_UINT32 rep_cmd0_fifo_udf_mask;
    ZXIC_UINT32 rep_cmd1_fifo_udf_mask;
    ZXIC_UINT32 rep_cmd2_fifo_udf_mask;
    ZXIC_UINT32 rep_cmd3_fifo_udf_mask;
}DPP_NPPU_MR_CFG_MR_INT_MASK_2_T;

typedef struct dpp_nppu_mr_cfg_mr_int_mask_3_t
{
    ZXIC_UINT32 free_ptr0_fifo_ovf_mask;
    ZXIC_UINT32 free_ptr1_fifo_ovf_mask;
    ZXIC_UINT32 free_ptr2_fifo_ovf_mask;
    ZXIC_UINT32 free_ptr3_fifo_ovf_mask;
    ZXIC_UINT32 block_ptr0_fifo_ovf_mask;
    ZXIC_UINT32 block_ptr1_fifo_ovf_mask;
    ZXIC_UINT32 block_ptr2_fifo_ovf_mask;
    ZXIC_UINT32 block_ptr3_fifo_ovf_mask;
    ZXIC_UINT32 mgid0_fifo_ovf_mask;
    ZXIC_UINT32 mgid1_fifo_ovf_mask;
    ZXIC_UINT32 mgid2_fifo_ovf_mask;
    ZXIC_UINT32 mgid3_fifo_ovf_mask;
    ZXIC_UINT32 pre_rcv_ptr0_fifo_ovf_mask;
    ZXIC_UINT32 pre_rcv_ptr1_fifo_ovf_mask;
    ZXIC_UINT32 pre_rcv_ptr2_fifo_ovf_mask;
    ZXIC_UINT32 pre_rcv_ptr3_fifo_ovf_mask;
    ZXIC_UINT32 rep_cmd0_fifo_ovf_mask;
    ZXIC_UINT32 rep_cmd1_fifo_ovf_mask;
    ZXIC_UINT32 rep_cmd2_fifo_ovf_mask;
    ZXIC_UINT32 rep_cmd3_fifo_ovf_mask;
}DPP_NPPU_MR_CFG_MR_INT_MASK_3_T;

typedef struct dpp_nppu_mr_cfg_mr_int_mask_4_t
{
    ZXIC_UINT32 data_buf0_ram_parity_err_mask;
    ZXIC_UINT32 data_buf1_ram_parity_err_mask;
    ZXIC_UINT32 data_buf2_ram_parity_err_mask;
    ZXIC_UINT32 data_buf3_ram_parity_err_mask;
    ZXIC_UINT32 mlt_ecc_single_err_mask;
    ZXIC_UINT32 free_ptr0_fifo_ecc_single_err_mask;
    ZXIC_UINT32 free_ptr1_fifo_ecc_single_err_mask;
    ZXIC_UINT32 free_ptr2_fifo_ecc_single_err_mask;
    ZXIC_UINT32 free_ptr3_fifo_ecc_single_err_mask;
    ZXIC_UINT32 block_ptr0_fifo_ecc_single_err_mask;
    ZXIC_UINT32 block_ptr1_fifo_ecc_single_err_mask;
    ZXIC_UINT32 block_ptr2_fifo_ecc_single_err_mask;
    ZXIC_UINT32 block_ptr3_fifo_ecc_single_err_mask;
    ZXIC_UINT32 mgid0_fifo_ecc_single_err_mask;
    ZXIC_UINT32 mgid1_fifo_ecc_single_err_mask;
    ZXIC_UINT32 mgid2_fifo_ecc_single_err_mask;
    ZXIC_UINT32 mgid3_fifo_ecc_single_err_mask;
    ZXIC_UINT32 pre_rcv_ptr0_fifo_ecc_single_err_mask;
    ZXIC_UINT32 pre_rcv_ptr1_fifo_ecc_single_err_mask;
    ZXIC_UINT32 pre_rcv_ptr2_fifo_ecc_single_err_mask;
    ZXIC_UINT32 pre_rcv_ptr3_fifo_ecc_single_err_mask;
    ZXIC_UINT32 rep_cmd0_fifo_ecc_single_err_mask;
    ZXIC_UINT32 rep_cmd1_fifo_ecc_single_err_mask;
    ZXIC_UINT32 rep_cmd2_fifo_ecc_single_err_mask;
    ZXIC_UINT32 rep_cmd3_fifo_ecc_single_err_mask;
}DPP_NPPU_MR_CFG_MR_INT_MASK_4_T;

typedef struct dpp_nppu_mr_cfg_mr_states_1_t
{
    ZXIC_UINT32 free_ptr0_fifo_full;
    ZXIC_UINT32 free_ptr1_fifo_full;
    ZXIC_UINT32 free_ptr2_fifo_full;
    ZXIC_UINT32 free_ptr3_fifo_full;
    ZXIC_UINT32 block_ptr0_fifo_full;
    ZXIC_UINT32 block_ptr1_fifo_full;
    ZXIC_UINT32 block_ptr2_fifo_full;
    ZXIC_UINT32 block_ptr3_fifo_full;
    ZXIC_UINT32 mgid0_fifo_full;
    ZXIC_UINT32 mgid1_fifo_full;
    ZXIC_UINT32 mgid2_fifo_full;
    ZXIC_UINT32 mgid3_fifo_full;
    ZXIC_UINT32 pre_rcv_ptr0_fifo_full;
    ZXIC_UINT32 pre_rcv_ptr1_fifo_full;
    ZXIC_UINT32 pre_rcv_ptr2_fifo_full;
    ZXIC_UINT32 pre_rcv_ptr3_fifo_full;
    ZXIC_UINT32 rep_cmd0_fifo_full;
    ZXIC_UINT32 rep_cmd1_fifo_full;
    ZXIC_UINT32 rep_cmd2_fifo_full;
    ZXIC_UINT32 rep_cmd3_fifo_full;
}DPP_NPPU_MR_CFG_MR_STATES_1_T;

typedef struct dpp_nppu_mr_cfg_mr_states_2_t
{
    ZXIC_UINT32 free_ptr0_fifo_udf;
    ZXIC_UINT32 free_ptr1_fifo_udf;
    ZXIC_UINT32 free_ptr2_fifo_udf;
    ZXIC_UINT32 free_ptr3_fifo_udf;
    ZXIC_UINT32 block_ptr0_fifo_udf;
    ZXIC_UINT32 block_ptr1_fifo_udf;
    ZXIC_UINT32 block_ptr2_fifo_udf;
    ZXIC_UINT32 block_ptr3_fifo_udf;
    ZXIC_UINT32 mgid0_fifo_udf;
    ZXIC_UINT32 mgid1_fifo_udf;
    ZXIC_UINT32 mgid2_fifo_udf;
    ZXIC_UINT32 mgid3_fifo_udf;
    ZXIC_UINT32 pre_rcv_ptr0_fifo_udf;
    ZXIC_UINT32 pre_rcv_ptr1_fifo_udf;
    ZXIC_UINT32 pre_rcv_ptr2_fifo_udf;
    ZXIC_UINT32 pre_rcv_ptr3_fifo_udf;
    ZXIC_UINT32 rep_cmd0_fifo_udf;
    ZXIC_UINT32 rep_cmd1_fifo_udf;
    ZXIC_UINT32 rep_cmd2_fifo_udf;
    ZXIC_UINT32 rep_cmd3_fifo_udf;
}DPP_NPPU_MR_CFG_MR_STATES_2_T;

typedef struct dpp_nppu_mr_cfg_mr_states_3_t
{
    ZXIC_UINT32 free_ptr0_fifo_ovf;
    ZXIC_UINT32 free_ptr1_fifo_ovf;
    ZXIC_UINT32 free_ptr2_fifo_ovf;
    ZXIC_UINT32 free_ptr3_fifo_ovf;
    ZXIC_UINT32 block_ptr0_fifo_ovf;
    ZXIC_UINT32 block_ptr1_fifo_ovf;
    ZXIC_UINT32 block_ptr2_fifo_ovf;
    ZXIC_UINT32 block_ptr3_fifo_ovf;
    ZXIC_UINT32 mgid0_fifo_ovf;
    ZXIC_UINT32 mgid1_fifo_ovf;
    ZXIC_UINT32 mgid2_fifo_ovf;
    ZXIC_UINT32 mgid3_fifo_ovf;
    ZXIC_UINT32 pre_rcv_ptr0_fifo_ovf;
    ZXIC_UINT32 pre_rcv_ptr1_fifo_ovf;
    ZXIC_UINT32 pre_rcv_ptr2_fifo_ovf;
    ZXIC_UINT32 pre_rcv_ptr3_fifo_ovf;
    ZXIC_UINT32 rep_cmd0_fifo_ovf;
    ZXIC_UINT32 rep_cmd1_fifo_ovf;
    ZXIC_UINT32 rep_cmd2_fifo_ovf;
    ZXIC_UINT32 rep_cmd3_fifo_ovf;
}DPP_NPPU_MR_CFG_MR_STATES_3_T;

typedef struct dpp_nppu_mr_cfg_mr_states_4_t
{
    ZXIC_UINT32 data_buf0_ram_parity_err;
    ZXIC_UINT32 data_buf1_ram_parity_err;
    ZXIC_UINT32 data_buf2_ram_parity_err;
    ZXIC_UINT32 data_buf3_ram_parity_err;
    ZXIC_UINT32 mlt_ecc_single_err;
    ZXIC_UINT32 free_ptr0_fifo_ecc_single_err;
    ZXIC_UINT32 free_ptr1_fifo_ecc_single_err;
    ZXIC_UINT32 free_ptr2_fifo_ecc_single_err;
    ZXIC_UINT32 free_ptr3_fifo_ecc_single_err;
    ZXIC_UINT32 block_ptr0_fifo_ecc_single_err;
    ZXIC_UINT32 block_ptr1_fifo_ecc_single_err;
    ZXIC_UINT32 block_ptr2_fifo_ecc_single_err;
    ZXIC_UINT32 block_ptr3_fifo_ecc_single_err;
    ZXIC_UINT32 mgid0_fifo_ecc_single_err;
    ZXIC_UINT32 mgid1_fifo_ecc_single_err;
    ZXIC_UINT32 mgid2_fifo_ecc_single_err;
    ZXIC_UINT32 mgid3_fifo_ecc_single_err;
    ZXIC_UINT32 pre_rcv_ptr0_fifo_ecc_single_err;
    ZXIC_UINT32 pre_rcv_ptr1_fifo_ecc_single_err;
    ZXIC_UINT32 pre_rcv_ptr2_fifo_ecc_single_err;
    ZXIC_UINT32 pre_rcv_ptr3_fifo_ecc_single_err;
    ZXIC_UINT32 rep_cmd0_fifo_ecc_single_err;
    ZXIC_UINT32 rep_cmd1_fifo_ecc_single_err;
    ZXIC_UINT32 rep_cmd2_fifo_ecc_single_err;
    ZXIC_UINT32 rep_cmd3_fifo_ecc_single_err;
}DPP_NPPU_MR_CFG_MR_STATES_4_T;

typedef struct dpp_nppu_mr_cfg_mr_states_5_t
{
    ZXIC_UINT32 mlt_ecc_double_err;
    ZXIC_UINT32 free_ptr0_fifo_ecc_double_err;
    ZXIC_UINT32 free_ptr1_fifo_ecc_double_err;
    ZXIC_UINT32 free_ptr2_fifo_ecc_double_err;
    ZXIC_UINT32 free_ptr3_fifo_ecc_double_err;
    ZXIC_UINT32 block_ptr0_fifo_ecc_double_err;
    ZXIC_UINT32 block_ptr1_fifo_ecc_double_err;
    ZXIC_UINT32 block_ptr2_fifo_ecc_double_err;
    ZXIC_UINT32 block_ptr3_fifo_ecc_double_err;
    ZXIC_UINT32 mgid0_fifo_ecc_double_err;
    ZXIC_UINT32 mgid1_fifo_ecc_double_err;
    ZXIC_UINT32 mgid2_fifo_ecc_double_err;
    ZXIC_UINT32 mgid3_fifo_ecc_double_err;
    ZXIC_UINT32 pre_rcv_ptr0_fifo_ecc_double_err;
    ZXIC_UINT32 pre_rcv_ptr1_fifo_ecc_double_err;
    ZXIC_UINT32 pre_rcv_ptr2_fifo_ecc_double_err;
    ZXIC_UINT32 pre_rcv_ptr3_fifo_ecc_double_err;
    ZXIC_UINT32 rep_cmd0_fifo_ecc_double_err;
    ZXIC_UINT32 rep_cmd1_fifo_ecc_double_err;
    ZXIC_UINT32 rep_cmd2_fifo_ecc_double_err;
    ZXIC_UINT32 rep_cmd3_fifo_ecc_double_err;
}DPP_NPPU_MR_CFG_MR_STATES_5_T;

typedef struct dpp_nppu_mr_cfg_mr_states_6_t
{
    ZXIC_UINT32 free_ptr0_fifo_empty;
    ZXIC_UINT32 free_ptr1_fifo_empty;
    ZXIC_UINT32 free_ptr2_fifo_empty;
    ZXIC_UINT32 free_ptr3_fifo_empty;
    ZXIC_UINT32 block_ptr0_fifo_empty;
    ZXIC_UINT32 block_ptr1_fifo_empty;
    ZXIC_UINT32 block_ptr2_fifo_empty;
    ZXIC_UINT32 block_ptr3_fifo_empty;
    ZXIC_UINT32 mgid0_fifo_empty;
    ZXIC_UINT32 mgid1_fifo_empty;
    ZXIC_UINT32 mgid2_fifo_empty;
    ZXIC_UINT32 mgid3_fifo_empty;
    ZXIC_UINT32 pre_rcv_ptr0_fifo_empty;
    ZXIC_UINT32 pre_rcv_ptr1_fifo_empty;
    ZXIC_UINT32 pre_rcv_ptr2_fifo_empty;
    ZXIC_UINT32 pre_rcv_ptr3_fifo_empty;
    ZXIC_UINT32 rep_cmd0_fifo_empty;
    ZXIC_UINT32 rep_cmd1_fifo_empty;
    ZXIC_UINT32 rep_cmd2_fifo_empty;
    ZXIC_UINT32 rep_cmd3_fifo_empty;
}DPP_NPPU_MR_CFG_MR_STATES_6_T;

typedef struct dpp_nppu_mr_cfg_mr_states_7_t
{
    ZXIC_UINT32 cos0_is_rep_busy;
    ZXIC_UINT32 cos1_is_rep_busy;
    ZXIC_UINT32 cos2_is_rep_busy;
    ZXIC_UINT32 cos3_is_rep_busy;
    ZXIC_UINT32 block_ptr0_fifo_non_sop_ren_rdy;
    ZXIC_UINT32 block_ptr1_fifo_non_sop_ren_rdy;
    ZXIC_UINT32 block_ptr2_fifo_non_sop_ren_rdy;
    ZXIC_UINT32 block_ptr3_fifo_non_sop_ren_rdy;
    ZXIC_UINT32 pre_rcv_ptr0_fifo_non_sop_ren_rdy;
    ZXIC_UINT32 pre_rcv_ptr1_fifo_non_sop_ren_rdy;
    ZXIC_UINT32 pre_rcv_ptr2_fifo_non_sop_ren_rdy;
    ZXIC_UINT32 pre_rcv_ptr3_fifo_non_sop_ren_rdy;
    ZXIC_UINT32 port_shap_rdy;
    ZXIC_UINT32 mr_lif_group0_rdy_3;
    ZXIC_UINT32 mr_lif_group0_rdy_2;
    ZXIC_UINT32 mr_lif_group0_rdy_1;
    ZXIC_UINT32 mr_lif_group0_rdy_0;
    ZXIC_UINT32 pktrx_pfc_rdy_3;
    ZXIC_UINT32 pktrx_pfc_rdy_2;
    ZXIC_UINT32 pktrx_pfc_rdy_1;
    ZXIC_UINT32 pktrx_pfc_rdy_0;
    ZXIC_UINT32 pktrx_link_rdy;
}DPP_NPPU_MR_CFG_MR_STATES_7_T;

typedef struct dpp_nppu_mr_cfg_mr_states_8_t
{
    ZXIC_UINT32 mr_head;
}DPP_NPPU_MR_CFG_MR_STATES_8_T;

typedef struct dpp_nppu_mr_cfg_mr_sop_in_cnt_t
{
    ZXIC_UINT32 mr_sop_in_cnt;
}DPP_NPPU_MR_CFG_MR_SOP_IN_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_eop_in_cnt_t
{
    ZXIC_UINT32 mr_eop_in_cnt;
}DPP_NPPU_MR_CFG_MR_EOP_IN_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_sop_out_cnt_t
{
    ZXIC_UINT32 mr_sop_out_cnt;
}DPP_NPPU_MR_CFG_MR_SOP_OUT_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_eop_out_cnt_t
{
    ZXIC_UINT32 mr_eop_out_cnt;
}DPP_NPPU_MR_CFG_MR_EOP_OUT_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos0_in_cnt_t
{
    ZXIC_UINT32 mr_cos0_in_cnt;
}DPP_NPPU_MR_CFG_MR_COS0_IN_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos1_in_cnt_t
{
    ZXIC_UINT32 mr_cos1_in_cnt;
}DPP_NPPU_MR_CFG_MR_COS1_IN_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos2_in_cnt_t
{
    ZXIC_UINT32 mr_cos2_in_cnt;
}DPP_NPPU_MR_CFG_MR_COS2_IN_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos3_in_cnt_t
{
    ZXIC_UINT32 mr_cos3_in_cnt;
}DPP_NPPU_MR_CFG_MR_COS3_IN_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos0_out_cnt_t
{
    ZXIC_UINT32 mr_cos0_out_cnt;
}DPP_NPPU_MR_CFG_MR_COS0_OUT_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos1_out_cnt_t
{
    ZXIC_UINT32 mr_cos1_out_cnt;
}DPP_NPPU_MR_CFG_MR_COS1_OUT_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos2_out_cnt_t
{
    ZXIC_UINT32 mr_cos2_out_cnt;
}DPP_NPPU_MR_CFG_MR_COS2_OUT_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos3_out_cnt_t
{
    ZXIC_UINT32 mr_cos3_out_cnt;
}DPP_NPPU_MR_CFG_MR_COS3_OUT_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_err_in_cnt_t
{
    ZXIC_UINT32 mr_err_in_cnt;
}DPP_NPPU_MR_CFG_MR_ERR_IN_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos0_sop_in_cnt_t
{
    ZXIC_UINT32 mr_cos0_sop_in_cnt;
}DPP_NPPU_MR_CFG_MR_COS0_SOP_IN_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos0_eop_in_cnt_t
{
    ZXIC_UINT32 mr_cos0_eop_in_cnt;
}DPP_NPPU_MR_CFG_MR_COS0_EOP_IN_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos1_sop_in_cnt_t
{
    ZXIC_UINT32 mr_cos1_sop_in_cnt;
}DPP_NPPU_MR_CFG_MR_COS1_SOP_IN_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos1_eop_in_cnt_t
{
    ZXIC_UINT32 mr_cos1_eop_in_cnt;
}DPP_NPPU_MR_CFG_MR_COS1_EOP_IN_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos2_sop_in_cnt_t
{
    ZXIC_UINT32 mr_cos2_sop_in_cnt;
}DPP_NPPU_MR_CFG_MR_COS2_SOP_IN_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos2_eop_in_cnt_t
{
    ZXIC_UINT32 mr_cos2_eop_in_cnt;
}DPP_NPPU_MR_CFG_MR_COS2_EOP_IN_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos3_sop_in_cnt_t
{
    ZXIC_UINT32 mr_cos3_sop_in_cnt;
}DPP_NPPU_MR_CFG_MR_COS3_SOP_IN_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos3_eop_in_cnt_t
{
    ZXIC_UINT32 mr_cos3_eop_in_cnt;
}DPP_NPPU_MR_CFG_MR_COS3_EOP_IN_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos0_in_err_cnt_t
{
    ZXIC_UINT32 mr_cos0_in_err_cnt;
}DPP_NPPU_MR_CFG_MR_COS0_IN_ERR_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos1_in_err_cnt_t
{
    ZXIC_UINT32 mr_cos1_in_err_cnt;
}DPP_NPPU_MR_CFG_MR_COS1_IN_ERR_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos2_in_err_cnt_t
{
    ZXIC_UINT32 mr_cos2_in_err_cnt;
}DPP_NPPU_MR_CFG_MR_COS2_IN_ERR_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos3_in_err_cnt_t
{
    ZXIC_UINT32 mr_cos3_in_err_cnt;
}DPP_NPPU_MR_CFG_MR_COS3_IN_ERR_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos0_sop_out_cnt_t
{
    ZXIC_UINT32 mr_cos0_sop_out_cnt;
}DPP_NPPU_MR_CFG_MR_COS0_SOP_OUT_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos0_eop_out_cnt_t
{
    ZXIC_UINT32 mr_cos0_eop_out_cnt;
}DPP_NPPU_MR_CFG_MR_COS0_EOP_OUT_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos1_sop_out_cnt_t
{
    ZXIC_UINT32 mr_cos1_sop_out_cnt;
}DPP_NPPU_MR_CFG_MR_COS1_SOP_OUT_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos1_eop_out_cnt_t
{
    ZXIC_UINT32 mr_cos1_eop_out_cnt;
}DPP_NPPU_MR_CFG_MR_COS1_EOP_OUT_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos2_sop_out_cnt_t
{
    ZXIC_UINT32 mr_cos2_sop_out_cnt;
}DPP_NPPU_MR_CFG_MR_COS2_SOP_OUT_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos2_eop_out_cnt_t
{
    ZXIC_UINT32 mr_cos2_eop_out_cnt;
}DPP_NPPU_MR_CFG_MR_COS2_EOP_OUT_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos3_sop_out_cnt_t
{
    ZXIC_UINT32 mr_cos3_sop_out_cnt;
}DPP_NPPU_MR_CFG_MR_COS3_SOP_OUT_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_cos3_eop_out_cnt_t
{
    ZXIC_UINT32 mr_cos3_eop_out_cnt;
}DPP_NPPU_MR_CFG_MR_COS3_EOP_OUT_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_mlt_unvld_cnt_t
{
    ZXIC_UINT32 mr_mlt_unvld_cnt;
}DPP_NPPU_MR_CFG_MR_MLT_UNVLD_CNT_T;

typedef struct dpp_nppu_mr_cfg_mr_sop_eop_match_cfg_t
{
    ZXIC_UINT32 mr_sop_eop_macth_en;
    ZXIC_UINT32 mr_sop_eop_macth_dicard_th;
}DPP_NPPU_MR_CFG_MR_SOP_EOP_MATCH_CFG_T;

typedef struct dpp_nppu_mr_cfg_mr_mlt_unvld_mgid_t
{
    ZXIC_UINT32 mr_mlt_unvld_mgid;
}DPP_NPPU_MR_CFG_MR_MLT_UNVLD_MGID_T;

typedef struct dpp_nppu_pktrx_cfg_isch_fifo_th_1_t
{
    ZXIC_UINT32 cfg_sch_fifo7_fc_th;
    ZXIC_UINT32 cfg_sch_fifo6_fc_th;
    ZXIC_UINT32 cfg_sch_fifo5_fc_th;
    ZXIC_UINT32 cfg_sch_fifo4_fc_th;
}DPP_NPPU_PKTRX_CFG_ISCH_FIFO_TH_1_T;

typedef struct dpp_nppu_pktrx_cfg_isch_fifo_th_2_t
{
    ZXIC_UINT32 cfg_sch_fifo3_drop_th;
    ZXIC_UINT32 cfg_sch_fifo1_drop_th;
    ZXIC_UINT32 cfg_sch_fifo0_drop_th;
    ZXIC_UINT32 cfg_sch_fifo8_fc_th;
}DPP_NPPU_PKTRX_CFG_ISCH_FIFO_TH_2_T;

typedef struct dpp_nppu_pktrx_cfg_isch_fifo_th_3_t
{
    ZXIC_UINT32 cfg_sch_fifo6_drop_th;
    ZXIC_UINT32 cfg_sch_fifo5_drop_th;
    ZXIC_UINT32 cfg_sch_fifo4_drop_th;
    ZXIC_UINT32 cfg_sch_fifo2_drop_th;
}DPP_NPPU_PKTRX_CFG_ISCH_FIFO_TH_3_T;

typedef struct dpp_nppu_pktrx_cfg_isch_fifo_th_4_t
{
    ZXIC_UINT32 cfg_sch_fifo9_fc_th;
    ZXIC_UINT32 cfg_sch_fifo9_drop_th;
    ZXIC_UINT32 cfg_sch_fifo8_drop_th;
    ZXIC_UINT32 cfg_sch_fifo7_drop_th;
}DPP_NPPU_PKTRX_CFG_ISCH_FIFO_TH_4_T;

typedef struct dpp_nppu_pktrx_cfg_isch_cfg_0_t
{
    ZXIC_UINT32 cfg_sch_wrr1_weight1;
}DPP_NPPU_PKTRX_CFG_ISCH_CFG_0_T;

typedef struct dpp_nppu_pktrx_cfg_hdu_ex_tpid_0_t
{
    ZXIC_UINT32 cfg_type0;
    ZXIC_UINT32 cfg_type1;
}DPP_NPPU_PKTRX_CFG_HDU_EX_TPID_0_T;

typedef struct dpp_nppu_pktrx_cfg_hdu_ex_tpid_1_t
{
    ZXIC_UINT32 cfg_type2;
    ZXIC_UINT32 cfg_type3;
}DPP_NPPU_PKTRX_CFG_HDU_EX_TPID_1_T;

typedef struct dpp_nppu_pktrx_cfg_hdu_int_tpid_0_t
{
    ZXIC_UINT32 cfg_inner_type0;
    ZXIC_UINT32 cfg_inner_type1;
}DPP_NPPU_PKTRX_CFG_HDU_INT_TPID_0_T;

typedef struct dpp_nppu_pktrx_cfg_hdu_int_tpid_1_t
{
    ZXIC_UINT32 cfg_inner_type2;
    ZXIC_UINT32 cfg_inner_type3;
}DPP_NPPU_PKTRX_CFG_HDU_INT_TPID_1_T;

typedef struct dpp_nppu_pktrx_cfg_hdu_hdlc_0_t
{
    ZXIC_UINT32 hdlc_cfg0_type;
    ZXIC_UINT32 hdlc_cfg1_type;
}DPP_NPPU_PKTRX_CFG_HDU_HDLC_0_T;

typedef struct dpp_nppu_pktrx_cfg_hdu_hdlc_1_t
{
    ZXIC_UINT32 hdlc_cfg2_type;
    ZXIC_UINT32 hdlc_cfg3_type;
}DPP_NPPU_PKTRX_CFG_HDU_HDLC_1_T;

typedef struct dpp_nppu_pktrx_cfg_hdu_udf_l3type_0_t
{
    ZXIC_UINT32 cfg_l3_type0;
    ZXIC_UINT32 cfg_l3_type1;
}DPP_NPPU_PKTRX_CFG_HDU_UDF_L3TYPE_0_T;

typedef struct dpp_nppu_pktrx_cfg_hdu_udf_l3type_1_t
{
    ZXIC_UINT32 cfg_l3_type2;
    ZXIC_UINT32 cfg_l3_type3;
}DPP_NPPU_PKTRX_CFG_HDU_UDF_L3TYPE_1_T;

typedef struct dpp_nppu_pktrx_cfg_hdu_udf_l3type_2_t
{
    ZXIC_UINT32 cfg_l3_type4;
    ZXIC_UINT32 cfg_l3_type5;
}DPP_NPPU_PKTRX_CFG_HDU_UDF_L3TYPE_2_T;

typedef struct dpp_nppu_pktrx_cfg_hdu_udf_l3type_3_t
{
    ZXIC_UINT32 cfg_l3_type6;
    ZXIC_UINT32 cfg_l3_type7;
}DPP_NPPU_PKTRX_CFG_HDU_UDF_L3TYPE_3_T;

typedef struct dpp_nppu_pktrx_cfg_hdu_udf_l4type_0_t
{
    ZXIC_UINT32 cfg_l4_type0;
    ZXIC_UINT32 cfg_l4_type1;
    ZXIC_UINT32 cfg_l4_type2;
    ZXIC_UINT32 cfg_l4_type3;
}DPP_NPPU_PKTRX_CFG_HDU_UDF_L4TYPE_0_T;

typedef struct dpp_nppu_pktrx_cfg_hdu_udf_l4type_1_t
{
    ZXIC_UINT32 cfg_l4_type4;
    ZXIC_UINT32 cfg_l4_type5;
    ZXIC_UINT32 cfg_l4_type6;
    ZXIC_UINT32 cfg_l4_type7;
}DPP_NPPU_PKTRX_CFG_HDU_UDF_L4TYPE_1_T;

typedef struct dpp_nppu_pktrx_cfg_hdu_udf_l4type_2_t
{
    ZXIC_UINT32 cfg_l4_type8;
    ZXIC_UINT32 cfg_l4_type9;
    ZXIC_UINT32 cfg_l4_type10;
}DPP_NPPU_PKTRX_CFG_HDU_UDF_L4TYPE_2_T;

typedef struct dpp_nppu_pktrx_cfg_slot_no_cfg_t
{
    ZXIC_UINT32 cfg_parser_slot_no;
}DPP_NPPU_PKTRX_CFG_SLOT_NO_CFG_T;

typedef struct dpp_nppu_pktrx_cfg_pktrx_int_en_0_t
{
    ZXIC_UINT32 pktrx_int_en_31;
    ZXIC_UINT32 pktrx_int_en_30;
    ZXIC_UINT32 pktrx_int_en_29;
    ZXIC_UINT32 pktrx_int_en_28;
    ZXIC_UINT32 pktrx_int_en_27;
    ZXIC_UINT32 pktrx_int_en_26;
    ZXIC_UINT32 pktrx_int_en_25;
    ZXIC_UINT32 pktrx_int_en_24;
    ZXIC_UINT32 pktrx_int_en_23;
    ZXIC_UINT32 pktrx_int_en_22;
    ZXIC_UINT32 pktrx_int_en_21;
    ZXIC_UINT32 pktrx_int_en_20;
    ZXIC_UINT32 pktrx_int_en_19;
    ZXIC_UINT32 pktrx_int_en_18;
    ZXIC_UINT32 pktrx_int_en_17;
    ZXIC_UINT32 pktrx_int_en_16;
    ZXIC_UINT32 pktrx_int_en_15;
    ZXIC_UINT32 pktrx_int_en_14;
    ZXIC_UINT32 pktrx_int_en_13;
    ZXIC_UINT32 pktrx_int_en_12;
    ZXIC_UINT32 pktrx_int_en_11;
    ZXIC_UINT32 pktrx_int_en_10;
    ZXIC_UINT32 pktrx_int_en_9;
    ZXIC_UINT32 pktrx_int_en_8;
    ZXIC_UINT32 pktrx_int_en_7;
    ZXIC_UINT32 pktrx_int_en_6;
    ZXIC_UINT32 pktrx_int_en_5;
    ZXIC_UINT32 pktrx_int_en_4;
    ZXIC_UINT32 pktrx_int_en_3;
    ZXIC_UINT32 pktrx_int_en_2;
    ZXIC_UINT32 pktrx_int_en_1;
    ZXIC_UINT32 pktrx_int_en_0;
}DPP_NPPU_PKTRX_CFG_PKTRX_INT_EN_0_T;

typedef struct dpp_nppu_pktrx_cfg_pktrx_int_en_1_t
{
    ZXIC_UINT32 pktrx_int_en_35;
    ZXIC_UINT32 pktrx_int_en_34;
    ZXIC_UINT32 pktrx_int_en_33;
    ZXIC_UINT32 pktrx_int_en_32;
}DPP_NPPU_PKTRX_CFG_PKTRX_INT_EN_1_T;

typedef struct dpp_nppu_pktrx_cfg_pktrx_int_mask_0_t
{
    ZXIC_UINT32 pktrx_int_mask_31;
    ZXIC_UINT32 pktrx_int_mask_30;
    ZXIC_UINT32 pktrx_int_mask_29;
    ZXIC_UINT32 pktrx_int_mask_28;
    ZXIC_UINT32 pktrx_int_mask_27;
    ZXIC_UINT32 pktrx_int_mask_26;
    ZXIC_UINT32 pktrx_int_mask_25;
    ZXIC_UINT32 pktrx_int_mask_24;
    ZXIC_UINT32 pktrx_int_mask_23;
    ZXIC_UINT32 pktrx_int_mask_22;
    ZXIC_UINT32 pktrx_int_mask_21;
    ZXIC_UINT32 pktrx_int_mask_20;
    ZXIC_UINT32 pktrx_int_mask_19;
    ZXIC_UINT32 pktrx_int_mask_18;
    ZXIC_UINT32 pktrx_int_mask_17;
    ZXIC_UINT32 pktrx_int_mask_16;
    ZXIC_UINT32 pktrx_int_mask_15;
    ZXIC_UINT32 pktrx_int_mask_14;
    ZXIC_UINT32 pktrx_int_mask_13;
    ZXIC_UINT32 pktrx_int_mask_12;
    ZXIC_UINT32 pktrx_int_mask_11;
    ZXIC_UINT32 pktrx_int_mask_10;
    ZXIC_UINT32 pktrx_int_mask_9;
    ZXIC_UINT32 pktrx_int_mask_8;
    ZXIC_UINT32 pktrx_int_mask_7;
    ZXIC_UINT32 pktrx_int_mask_6;
    ZXIC_UINT32 pktrx_int_mask_5;
    ZXIC_UINT32 pktrx_int_mask_4;
    ZXIC_UINT32 pktrx_int_mask_3;
    ZXIC_UINT32 pktrx_int_mask_2;
    ZXIC_UINT32 pktrx_int_mask_1;
    ZXIC_UINT32 pktrx_int_mask_0;
}DPP_NPPU_PKTRX_CFG_PKTRX_INT_MASK_0_T;

typedef struct dpp_nppu_pktrx_cfg_pktrx_int_mask_1_t
{
    ZXIC_UINT32 pktrx_int_mask_35;
    ZXIC_UINT32 pktrx_int_mask_34;
    ZXIC_UINT32 pktrx_int_mask_33;
    ZXIC_UINT32 pktrx_int_mask_32;
}DPP_NPPU_PKTRX_CFG_PKTRX_INT_MASK_1_T;

typedef struct dpp_nppu_pktrx_cfg_pktrx_int_status_t
{
    ZXIC_UINT32 int_status;
}DPP_NPPU_PKTRX_CFG_PKTRX_INT_STATUS_T;

typedef struct dpp_nppu_pktrx_cfg_pktrx_port_rdy0_t
{
    ZXIC_UINT32 pktrx_trpgrx_r1_rdy;
    ZXIC_UINT32 pktrx_trpgrx_r2_rdy;
}DPP_NPPU_PKTRX_CFG_PKTRX_PORT_RDY0_T;

typedef struct dpp_nppu_pktrx_cfg_pktrx_lif0_pfc_rdy0_t
{
    ZXIC_UINT32 pktrx_trpgrx_r1_pfc_rdy_0;
}DPP_NPPU_PKTRX_CFG_PKTRX_LIF0_PFC_RDY0_T;

typedef struct dpp_nppu_pktrx_cfg_pktrx_lif0_pfc_rdy1_t
{
    ZXIC_UINT32 pktrx_trpgrx_r1_pfc_rdy_1;
}DPP_NPPU_PKTRX_CFG_PKTRX_LIF0_PFC_RDY1_T;

typedef struct dpp_nppu_pktrx_cfg_pktrx_lif0_pfc_rdy2_t
{
    ZXIC_UINT32 pktrx_trpgrx_r1_pfc_rdy_2;
}DPP_NPPU_PKTRX_CFG_PKTRX_LIF0_PFC_RDY2_T;

typedef struct dpp_nppu_pktrx_cfg_pktrx_lif0_pfc_rdy3_t
{
    ZXIC_UINT32 pktrx_trpgrx_r2_pfc_rdy_3;
}DPP_NPPU_PKTRX_CFG_PKTRX_LIF0_PFC_RDY3_T;

typedef struct dpp_nppu_pktrx_cfg_pktrx_lif0_pfc_rdy4_t
{
    ZXIC_UINT32 pktrx_trpgrx_r2_pfc_rdy_4;
}DPP_NPPU_PKTRX_CFG_PKTRX_LIF0_PFC_RDY4_T;

typedef struct dpp_nppu_pktrx_cfg_pktrx_lif0_pfc_rdy5_t
{
    ZXIC_UINT32 pktrx_trpgrx_r2_pfc_rdy_5;
}DPP_NPPU_PKTRX_CFG_PKTRX_LIF0_PFC_RDY5_T;

typedef struct dpp_nppu_pktrx_cfg_pktrx_lif0_pfc_rdy6_t
{
    ZXIC_UINT32 pktrx_trpgrx_r2_pfc_rdy_6;
}DPP_NPPU_PKTRX_CFG_PKTRX_LIF0_PFC_RDY6_T;

typedef struct dpp_nppu_pktrx_cfg_cfg_port_l2_offset_mode_t
{
    ZXIC_UINT32 cfg_port_l2_offset_mode;
}DPP_NPPU_PKTRX_CFG_CFG_PORT_L2_OFFSET_MODE_T;

typedef struct dpp_nppu_idma_cfg_int_ram_en_t
{
    ZXIC_UINT32 phy_sts_parity_err;
    ZXIC_UINT32 ptr_buf_parity_err;
}DPP_NPPU_IDMA_CFG_INT_RAM_EN_T;

typedef struct dpp_nppu_idma_cfg_int_ram_mask_t
{
    ZXIC_UINT32 phy_sts_parity_err;
    ZXIC_UINT32 ptr_buf_parity_err;
}DPP_NPPU_IDMA_CFG_INT_RAM_MASK_T;

typedef struct dpp_nppu_idma_cfg_int_ram_status_t
{
    ZXIC_UINT32 phy_sts_parity_err;
    ZXIC_UINT32 ptr_buf_parity_err;
}DPP_NPPU_IDMA_CFG_INT_RAM_STATUS_T;

typedef struct dpp_nppu_idma_cfg_subsys_int_mask_flag_t
{
    ZXIC_UINT32 subsys_int_mask_flag;
}DPP_NPPU_IDMA_CFG_SUBSYS_INT_MASK_FLAG_T;

typedef struct dpp_nppu_idma_cfg_subsys_int_unmask_flag_t
{
    ZXIC_UINT32 subsys_int_unmask_flag;
}DPP_NPPU_IDMA_CFG_SUBSYS_INT_UNMASK_FLAG_T;

typedef struct dpp_nppu_idma_cfg_debug_cnt_rdclr_mode_t
{
    ZXIC_UINT32 debug_cnt_rdclr_mode;
}DPP_NPPU_IDMA_CFG_DEBUG_CNT_RDCLR_MODE_T;

typedef struct dpp_nppu_pbu_cfg_int_ram_en0_t
{
    ZXIC_UINT32 int_ram_en_31;
    ZXIC_UINT32 int_ram_en_30;
    ZXIC_UINT32 int_ram_en_29;
    ZXIC_UINT32 int_ram_en_28;
    ZXIC_UINT32 int_ram_en_27;
    ZXIC_UINT32 int_ram_en_26;
    ZXIC_UINT32 int_ram_en_25;
    ZXIC_UINT32 int_ram_en_24;
    ZXIC_UINT32 int_ram_en_23;
    ZXIC_UINT32 int_ram_en_22;
    ZXIC_UINT32 int_ram_en_21;
    ZXIC_UINT32 int_ram_en_20;
    ZXIC_UINT32 int_ram_en_19;
    ZXIC_UINT32 int_ram_en_18;
    ZXIC_UINT32 int_ram_en_17;
    ZXIC_UINT32 int_ram_en_16;
    ZXIC_UINT32 int_ram_en_15;
    ZXIC_UINT32 int_ram_en_14;
    ZXIC_UINT32 int_ram_en_13;
    ZXIC_UINT32 int_ram_en_12;
    ZXIC_UINT32 int_ram_en_11;
    ZXIC_UINT32 int_ram_en_10;
    ZXIC_UINT32 int_ram_en_9;
    ZXIC_UINT32 int_ram_en_8;
    ZXIC_UINT32 int_ram_en_7;
    ZXIC_UINT32 int_ram_en_6;
    ZXIC_UINT32 int_ram_en_5;
    ZXIC_UINT32 int_ram_en_4;
    ZXIC_UINT32 int_ram_en_3;
    ZXIC_UINT32 int_ram_en_2;
    ZXIC_UINT32 int_ram_en_1;
    ZXIC_UINT32 int_ram_en_0;
}DPP_NPPU_PBU_CFG_INT_RAM_EN0_T;

typedef struct dpp_nppu_pbu_cfg_int_ram_mask0_t
{
    ZXIC_UINT32 int_ram_mask_31;
    ZXIC_UINT32 int_ram_mask_30;
    ZXIC_UINT32 int_ram_mask_29;
    ZXIC_UINT32 int_ram_mask_28;
    ZXIC_UINT32 int_ram_mask_27;
    ZXIC_UINT32 int_ram_mask_26;
    ZXIC_UINT32 int_ram_mask_25;
    ZXIC_UINT32 int_ram_mask_24;
    ZXIC_UINT32 int_ram_mask_23;
    ZXIC_UINT32 int_ram_mask_22;
    ZXIC_UINT32 int_ram_mask_21;
    ZXIC_UINT32 int_ram_mask_20;
    ZXIC_UINT32 int_ram_mask_19;
    ZXIC_UINT32 int_ram_mask_18;
    ZXIC_UINT32 int_ram_mask_17;
    ZXIC_UINT32 int_ram_mask_16;
    ZXIC_UINT32 int_ram_mask_15;
    ZXIC_UINT32 int_ram_mask_14;
    ZXIC_UINT32 int_ram_mask_13;
    ZXIC_UINT32 int_ram_mask_12;
    ZXIC_UINT32 int_ram_mask_11;
    ZXIC_UINT32 int_ram_mask_10;
    ZXIC_UINT32 int_ram_mask_9;
    ZXIC_UINT32 int_ram_mask_8;
    ZXIC_UINT32 int_ram_mask_7;
    ZXIC_UINT32 int_ram_mask_6;
    ZXIC_UINT32 int_ram_mask_5;
    ZXIC_UINT32 int_ram_mask_4;
    ZXIC_UINT32 int_ram_mask_3;
    ZXIC_UINT32 int_ram_mask_2;
    ZXIC_UINT32 int_ram_mask_1;
    ZXIC_UINT32 int_ram_mask_0;
}DPP_NPPU_PBU_CFG_INT_RAM_MASK0_T;

typedef struct dpp_nppu_pbu_cfg_int_ram_status0_t
{
    ZXIC_UINT32 int_ram_status_31;
    ZXIC_UINT32 int_ram_status_30;
    ZXIC_UINT32 int_ram_status_29;
    ZXIC_UINT32 int_ram_status_28;
    ZXIC_UINT32 int_ram_status_27;
    ZXIC_UINT32 int_ram_status_26;
    ZXIC_UINT32 int_ram_status_25;
    ZXIC_UINT32 int_ram_status_24;
    ZXIC_UINT32 int_ram_status_23;
    ZXIC_UINT32 int_ram_status_22;
    ZXIC_UINT32 int_ram_status_21;
    ZXIC_UINT32 int_ram_status_20;
    ZXIC_UINT32 int_ram_status_19;
    ZXIC_UINT32 int_ram_status_18;
    ZXIC_UINT32 int_ram_status_17;
    ZXIC_UINT32 int_ram_status_16;
    ZXIC_UINT32 int_ram_status_15;
    ZXIC_UINT32 int_ram_status_14;
    ZXIC_UINT32 int_ram_status_13;
    ZXIC_UINT32 int_ram_status_12;
    ZXIC_UINT32 int_ram_status_11;
    ZXIC_UINT32 int_ram_status_10;
    ZXIC_UINT32 int_ram_status_9;
    ZXIC_UINT32 int_ram_status_8;
    ZXIC_UINT32 int_ram_status_7;
    ZXIC_UINT32 int_ram_status_6;
    ZXIC_UINT32 int_ram_status_5;
    ZXIC_UINT32 int_ram_status_4;
    ZXIC_UINT32 int_ram_status_3;
    ZXIC_UINT32 int_ram_status_2;
    ZXIC_UINT32 int_ram_status_1;
    ZXIC_UINT32 int_ram_status_0;
}DPP_NPPU_PBU_CFG_INT_RAM_STATUS0_T;

typedef struct dpp_nppu_pbu_cfg_int_fifo_en0_t
{
    ZXIC_UINT32 int_fifo_en_31;
    ZXIC_UINT32 int_fifo_en_30;
    ZXIC_UINT32 int_fifo_en_29;
    ZXIC_UINT32 int_fifo_en_28;
    ZXIC_UINT32 int_fifo_en_27;
    ZXIC_UINT32 int_fifo_en_26;
    ZXIC_UINT32 int_fifo_en_25;
    ZXIC_UINT32 int_fifo_en_24;
    ZXIC_UINT32 int_fifo_en_23;
    ZXIC_UINT32 int_fifo_en_22;
    ZXIC_UINT32 int_fifo_en_21;
    ZXIC_UINT32 int_fifo_en_20;
    ZXIC_UINT32 int_fifo_en_19;
    ZXIC_UINT32 int_fifo_en_18;
    ZXIC_UINT32 int_fifo_en_17;
    ZXIC_UINT32 int_fifo_en_16;
    ZXIC_UINT32 int_fifo_en_15;
    ZXIC_UINT32 int_fifo_en_14;
    ZXIC_UINT32 int_fifo_en_13;
    ZXIC_UINT32 int_fifo_en_12;
    ZXIC_UINT32 int_fifo_en_11;
    ZXIC_UINT32 int_fifo_en_10;
    ZXIC_UINT32 int_fifo_en_9;
    ZXIC_UINT32 int_fifo_en_8;
    ZXIC_UINT32 int_fifo_en_7;
    ZXIC_UINT32 int_fifo_en_6;
    ZXIC_UINT32 int_fifo_en_5;
    ZXIC_UINT32 int_fifo_en_4;
    ZXIC_UINT32 int_fifo_en_3;
    ZXIC_UINT32 int_fifo_en_2;
    ZXIC_UINT32 int_fifo_en_1;
    ZXIC_UINT32 int_fifo_en_0;
}DPP_NPPU_PBU_CFG_INT_FIFO_EN0_T;

typedef struct dpp_nppu_pbu_cfg_int_fifo_en1_t
{
    ZXIC_UINT32 int_fifo_en_35;
    ZXIC_UINT32 int_fifo_en_34;
    ZXIC_UINT32 int_fifo_en_33;
    ZXIC_UINT32 int_fifo_en_32;
}DPP_NPPU_PBU_CFG_INT_FIFO_EN1_T;

typedef struct dpp_nppu_pbu_cfg_int_fifo_mask0_t
{
    ZXIC_UINT32 int_fifo_mask_31;
    ZXIC_UINT32 int_fifo_mask_30;
    ZXIC_UINT32 int_fifo_mask_29;
    ZXIC_UINT32 int_fifo_mask_28;
    ZXIC_UINT32 int_fifo_mask_27;
    ZXIC_UINT32 int_fifo_mask_26;
    ZXIC_UINT32 int_fifo_mask_25;
    ZXIC_UINT32 int_fifo_mask_24;
    ZXIC_UINT32 int_fifo_mask_23;
    ZXIC_UINT32 int_fifo_mask_22;
    ZXIC_UINT32 int_fifo_mask_21;
    ZXIC_UINT32 int_fifo_mask_20;
    ZXIC_UINT32 int_fifo_mask_19;
    ZXIC_UINT32 int_fifo_mask_18;
    ZXIC_UINT32 int_fifo_mask_17;
    ZXIC_UINT32 int_fifo_mask_16;
    ZXIC_UINT32 int_fifo_mask_15;
    ZXIC_UINT32 int_fifo_mask_14;
    ZXIC_UINT32 int_fifo_mask_13;
    ZXIC_UINT32 int_fifo_mask_12;
    ZXIC_UINT32 int_fifo_mask_11;
    ZXIC_UINT32 int_fifo_mask_10;
    ZXIC_UINT32 int_fifo_mask_9;
    ZXIC_UINT32 int_fifo_mask_8;
    ZXIC_UINT32 int_fifo_mask_7;
    ZXIC_UINT32 int_fifo_mask_6;
    ZXIC_UINT32 int_fifo_mask_5;
    ZXIC_UINT32 int_fifo_mask_4;
    ZXIC_UINT32 int_fifo_mask_3;
    ZXIC_UINT32 int_fifo_mask_2;
    ZXIC_UINT32 int_fifo_mask_1;
    ZXIC_UINT32 int_fifo_mask_0;
}DPP_NPPU_PBU_CFG_INT_FIFO_MASK0_T;

typedef struct dpp_nppu_pbu_cfg_int_fifo_mask1_t
{
    ZXIC_UINT32 int_fifo_mask_35;
    ZXIC_UINT32 int_fifo_mask_34;
    ZXIC_UINT32 int_fifo_mask_33;
    ZXIC_UINT32 int_fifo_mask_32;
}DPP_NPPU_PBU_CFG_INT_FIFO_MASK1_T;

typedef struct dpp_nppu_pbu_cfg_int_fifo_status0_t
{
    ZXIC_UINT32 int_fifo_status_31;
    ZXIC_UINT32 int_fifo_status_30;
    ZXIC_UINT32 int_fifo_status_29;
    ZXIC_UINT32 int_fifo_status_28;
    ZXIC_UINT32 int_fifo_status_27;
    ZXIC_UINT32 int_fifo_status_26;
    ZXIC_UINT32 int_fifo_status_25;
    ZXIC_UINT32 int_fifo_status_24;
    ZXIC_UINT32 int_fifo_status_23;
    ZXIC_UINT32 int_fifo_status_22;
    ZXIC_UINT32 int_fifo_status_21;
    ZXIC_UINT32 int_fifo_status_20;
    ZXIC_UINT32 int_fifo_status_19;
    ZXIC_UINT32 int_fifo_status_18;
    ZXIC_UINT32 int_fifo_status_17;
    ZXIC_UINT32 int_fifo_status_16;
    ZXIC_UINT32 int_fifo_status_15;
    ZXIC_UINT32 int_fifo_status_14;
    ZXIC_UINT32 int_fifo_status_13;
    ZXIC_UINT32 int_fifo_status_12;
    ZXIC_UINT32 int_fifo_status_11;
    ZXIC_UINT32 int_fifo_status_10;
    ZXIC_UINT32 int_fifo_status_9;
    ZXIC_UINT32 int_fifo_status_8;
    ZXIC_UINT32 int_fifo_status_7;
    ZXIC_UINT32 int_fifo_status_6;
    ZXIC_UINT32 int_fifo_status_5;
    ZXIC_UINT32 int_fifo_status_4;
    ZXIC_UINT32 int_fifo_status_3;
    ZXIC_UINT32 int_fifo_status_2;
    ZXIC_UINT32 int_fifo_status_1;
    ZXIC_UINT32 int_fifo_status_0;
}DPP_NPPU_PBU_CFG_INT_FIFO_STATUS0_T;

typedef struct dpp_nppu_pbu_cfg_int_fifo_status1_t
{
    ZXIC_UINT32 int_fifo_status_35;
    ZXIC_UINT32 int_fifo_status_34;
    ZXIC_UINT32 int_fifo_status_33;
    ZXIC_UINT32 int_fifo_status_32;
}DPP_NPPU_PBU_CFG_INT_FIFO_STATUS1_T;

typedef struct dpp_nppu_pbu_cfg_subsys_int_mask_flag_t
{
    ZXIC_UINT32 subsys_int_mask_flag;
}DPP_NPPU_PBU_CFG_SUBSYS_INT_MASK_FLAG_T;

typedef struct dpp_nppu_pbu_cfg_subsys_int_unmask_flag_t
{
    ZXIC_UINT32 subsys_int_unmask_flag;
}DPP_NPPU_PBU_CFG_SUBSYS_INT_UNMASK_FLAG_T;

typedef struct dpp_nppu_pbu_cfg_sa_ip_en_t
{
    ZXIC_UINT32 sa_ip_en;
}DPP_NPPU_PBU_CFG_SA_IP_EN_T;

typedef struct dpp_nppu_pbu_cfg_debug_cnt_rdclr_mode_t
{
    ZXIC_UINT32 debug_cnt_rdclr_mode;
}DPP_NPPU_PBU_CFG_DEBUG_CNT_RDCLR_MODE_T;

typedef struct dpp_nppu_pbu_cfg_fptr_fifo_aful_assert_cfg_t
{
    ZXIC_UINT32 fptr_fifo_aful_assert_cfg;
}DPP_NPPU_PBU_CFG_FPTR_FIFO_AFUL_ASSERT_CFG_T;

typedef struct dpp_nppu_pbu_cfg_fptr_fifo_aful_negate_cfg_t
{
    ZXIC_UINT32 fptr_fifo_aful_negate_cfg;
}DPP_NPPU_PBU_CFG_FPTR_FIFO_AFUL_NEGATE_CFG_T;

typedef struct dpp_nppu_pbu_cfg_pf_fifo_aful_assert_cfg_t
{
    ZXIC_UINT32 pf_fifo_aful_assert_cfg;
}DPP_NPPU_PBU_CFG_PF_FIFO_AFUL_ASSERT_CFG_T;

typedef struct dpp_nppu_pbu_cfg_pf_fifo_aful_negate_cfg_t
{
    ZXIC_UINT32 pf_fifo_aful_negate_cfg;
}DPP_NPPU_PBU_CFG_PF_FIFO_AFUL_NEGATE_CFG_T;

typedef struct dpp_nppu_pbu_cfg_pf_fifo_aept_assert_cfg_t
{
    ZXIC_UINT32 pf_fifo_aept_assert_cfg;
}DPP_NPPU_PBU_CFG_PF_FIFO_AEPT_ASSERT_CFG_T;

typedef struct dpp_nppu_pbu_cfg_pf_fifo_aept_negate_cfg_t
{
    ZXIC_UINT32 pf_fifo_aept_negate_cfg;
}DPP_NPPU_PBU_CFG_PF_FIFO_AEPT_NEGATE_CFG_T;

typedef struct dpp_nppu_pbu_cfg_wb_aful_assert_cfg_t
{
    ZXIC_UINT32 wb_aful_assert_cfg;
}DPP_NPPU_PBU_CFG_WB_AFUL_ASSERT_CFG_T;

typedef struct dpp_nppu_pbu_cfg_wb_aful_negate_cfg_t
{
    ZXIC_UINT32 wb_aful_negate_cfg;
}DPP_NPPU_PBU_CFG_WB_AFUL_NEGATE_CFG_T;

typedef struct dpp_nppu_pbu_cfg_se_key_aful_assert_cfg_t
{
    ZXIC_UINT32 se_key_aful_assert_cfg;
}DPP_NPPU_PBU_CFG_SE_KEY_AFUL_ASSERT_CFG_T;

typedef struct dpp_nppu_pbu_cfg_ifbrd_se_aful_assert_cfg_t
{
    ZXIC_UINT32 ifbrd_se_aful_assert_cfg;
}DPP_NPPU_PBU_CFG_IFBRD_SE_AFUL_ASSERT_CFG_T;

typedef struct dpp_nppu_pbu_cfg_ifbrd_se_aful_negate_cfg_t
{
    ZXIC_UINT32 ifbrd_se_aful_negate_cfg;
}DPP_NPPU_PBU_CFG_IFBRD_SE_AFUL_NEGATE_CFG_T;

typedef struct dpp_nppu_pbu_cfg_ifbrd_odma_aful_assert_cfg_t
{
    ZXIC_UINT32 ifbrd_odma_aful_assert_cfg;
}DPP_NPPU_PBU_CFG_IFBRD_ODMA_AFUL_ASSERT_CFG_T;

typedef struct dpp_nppu_pbu_cfg_ifbrd_odma_aful_negate_cfg_t
{
    ZXIC_UINT32 ifbrd_odma_aful_negate_cfg;
}DPP_NPPU_PBU_CFG_IFBRD_ODMA_AFUL_NEGATE_CFG_T;

typedef struct dpp_nppu_pbu_cfg_ifbrd_ppu_aful_assert_cfg_t
{
    ZXIC_UINT32 ifbrd_ppu_aful_assert_cfg;
}DPP_NPPU_PBU_CFG_IFBRD_PPU_AFUL_ASSERT_CFG_T;

typedef struct dpp_nppu_pbu_cfg_ifbrd_ppu_aful_negate_cfg_t
{
    ZXIC_UINT32 ifbrd_ppu_aful_negate_cfg;
}DPP_NPPU_PBU_CFG_IFBRD_PPU_AFUL_NEGATE_CFG_T;

typedef struct dpp_nppu_pbu_cfg_mc_logic_aful_assert_cfg_t
{
    ZXIC_UINT32 mc_logic_aful_assert_cfg;
}DPP_NPPU_PBU_CFG_MC_LOGIC_AFUL_ASSERT_CFG_T;

typedef struct dpp_nppu_pbu_cfg_mc_logic_aful_negate_cfg_t
{
    ZXIC_UINT32 mc_logic_aful_negate_cfg;
}DPP_NPPU_PBU_CFG_MC_LOGIC_AFUL_NEGATE_CFG_T;

typedef struct dpp_nppu_pbu_cfg_mc_logic_diff_t
{
    ZXIC_UINT32 mc_logic_diff;
}DPP_NPPU_PBU_CFG_MC_LOGIC_DIFF_T;

typedef struct dpp_nppu_pbu_cfg_cfg_peak_port_cnt_clr_t
{
    ZXIC_UINT32 cfg_peak_port_cnt_clr;
}DPP_NPPU_PBU_CFG_CFG_PEAK_PORT_CNT_CLR_T;

typedef struct dpp_nppu_pbu_cfg_all_ftm_crdt_th_t
{
    ZXIC_UINT32 ftm_crdt_port_cng_th;
    ZXIC_UINT32 ftm_crdt_port_th;
}DPP_NPPU_PBU_CFG_ALL_FTM_CRDT_TH_T;

typedef struct dpp_nppu_pbu_cfg_all_ftm_link_th_01_t
{
    ZXIC_UINT32 total_congest_th1;
    ZXIC_UINT32 total_congest_th0;
}DPP_NPPU_PBU_CFG_ALL_FTM_LINK_TH_01_T;

typedef struct dpp_nppu_pbu_cfg_all_ftm_link_th_23_t
{
    ZXIC_UINT32 total_congest_th3;
    ZXIC_UINT32 total_congest_th2;
}DPP_NPPU_PBU_CFG_ALL_FTM_LINK_TH_23_T;

typedef struct dpp_nppu_pbu_cfg_all_ftm_link_th_45_t
{
    ZXIC_UINT32 total_congest_th5;
    ZXIC_UINT32 total_congest_th4;
}DPP_NPPU_PBU_CFG_ALL_FTM_LINK_TH_45_T;

typedef struct dpp_nppu_pbu_cfg_all_ftm_link_th_6_t
{
    ZXIC_UINT32 total_congest_th6;
}DPP_NPPU_PBU_CFG_ALL_FTM_LINK_TH_6_T;

typedef struct dpp_nppu_pbu_cfg_all_ftm_total_congest_th_t
{
    ZXIC_UINT32 all_ftm_total_congest_th;
}DPP_NPPU_PBU_CFG_ALL_FTM_TOTAL_CONGEST_TH_T;

typedef struct dpp_nppu_pbu_cfg_cfg_crdt_mode_t
{
    ZXIC_UINT32 cfg_crdt_mode;
}DPP_NPPU_PBU_CFG_CFG_CRDT_MODE_T;

typedef struct dpp_nppu_pbu_cfg_cfg_pfc_rdy_high_time_t
{
    ZXIC_UINT32 cfg_pfc_rdy_high_time;
}DPP_NPPU_PBU_CFG_CFG_PFC_RDY_HIGH_TIME_T;

typedef struct dpp_nppu_pbu_cfg_cfg_pfc_rdy_low_time_t
{
    ZXIC_UINT32 cfg_pfc_rdy_low_time;
}DPP_NPPU_PBU_CFG_CFG_PFC_RDY_LOW_TIME_T;

typedef struct dpp_nppu_pbu_stat_pbu_fc_rdy_t
{
    ZXIC_UINT32 pbu_oam_send_fc_rdy;
    ZXIC_UINT32 pbu_odma_fc_rdy;
    ZXIC_UINT32 pbu_tm_fc_rdy;
    ZXIC_UINT32 pbu_idma_cos_rdy;
}DPP_NPPU_PBU_STAT_PBU_FC_RDY_T;

typedef struct dpp_nppu_pbu_stat_pbu_lif_group0_rdy0_t
{
    ZXIC_UINT32 pbu_ipg1_rdy;
    ZXIC_UINT32 pbu_ipg0_rdy;
    ZXIC_UINT32 pbu_trpgrx_xge_rdy;
    ZXIC_UINT32 pbu_trpgrx_cge1_rdy;
    ZXIC_UINT32 pbu_trpgrx_cge0_rdy;
}DPP_NPPU_PBU_STAT_PBU_LIF_GROUP0_RDY0_T;

typedef struct dpp_nppu_pbu_stat_pbu_lif_group0_rdy1_t
{
    ZXIC_UINT32 pbu_lif_group0_rdy1;
}DPP_NPPU_PBU_STAT_PBU_LIF_GROUP0_RDY1_T;

typedef struct dpp_nppu_pbu_stat_pbu_lif_group1_rdy_t
{
    ZXIC_UINT32 pbu_lif_group1_rdy1;
}DPP_NPPU_PBU_STAT_PBU_LIF_GROUP1_RDY_T;

typedef struct dpp_nppu_pbu_stat_pbu_lif_group0_pfc_rdy_t
{
    ZXIC_UINT32 pbu_lif_group0_pfc_rdy;
}DPP_NPPU_PBU_STAT_PBU_LIF_GROUP0_PFC_RDY_T;

typedef struct dpp_nppu_pbu_stat_pbu_lif_group1_pfc_rdy_t
{
    ZXIC_UINT32 pbu_lif_group1_pfc_rdy;
}DPP_NPPU_PBU_STAT_PBU_LIF_GROUP1_PFC_RDY_T;

typedef struct dpp_nppu_pbu_stat_pbu_sa_port_rdy_0_31_t
{
    ZXIC_UINT32 pbu_sa_port_rdy_0_31;
}DPP_NPPU_PBU_STAT_PBU_SA_PORT_RDY_0_31_T;

typedef struct dpp_nppu_pbu_stat_pbu_sa_port_rdy_32_50_t
{
    ZXIC_UINT32 pbu_sa_port_rdy_32_50;
}DPP_NPPU_PBU_STAT_PBU_SA_PORT_RDY_32_50_T;

typedef struct dpp_nppu_pbu_stat_pbu_pktrx_mr_pfc_rdy_t
{
    ZXIC_UINT32 pbu_pktrx_mr_pfc_rdy;
}DPP_NPPU_PBU_STAT_PBU_PKTRX_MR_PFC_RDY_T;

typedef struct dpp_nppu_pbu_stat_pbu_ftm_crdt_port_rdy_0_31_t
{
    ZXIC_UINT32 pbu_ftm_crdt_port_rdy_0_31;
}DPP_NPPU_PBU_STAT_PBU_FTM_CRDT_PORT_RDY_0_31_T;

typedef struct dpp_nppu_pbu_stat_pbu_ftm_crdt_port_rdy_32_47_t
{
    ZXIC_UINT32 pbu_ftm_crdt_port_rdy_32_47;
}DPP_NPPU_PBU_STAT_PBU_FTM_CRDT_PORT_RDY_32_47_T;

typedef struct dpp_nppu_pbu_stat_pbu_ftm_crdt_port_cng_rdy_0_31_t
{
    ZXIC_UINT32 pbu_ftm_crdt_port_cng_rdy_0_31;
}DPP_NPPU_PBU_STAT_PBU_FTM_CRDT_PORT_CNG_RDY_0_31_T;

typedef struct dpp_nppu_pbu_stat_pbu_ftm_crdt_port_cng_rdy_32_47_t
{
    ZXIC_UINT32 pbu_ftm_crdt_port_cng_rdy_32_47;
}DPP_NPPU_PBU_STAT_PBU_FTM_CRDT_PORT_CNG_RDY_32_47_T;

typedef struct dpp_nppu_pbu_stat_pbu_ftm_crdt_sys_info_t
{
    ZXIC_UINT32 pbu_ftm_crdt_sys_info;
}DPP_NPPU_PBU_STAT_PBU_FTM_CRDT_SYS_INFO_T;

typedef struct dpp_nppu_isu_cfg_weight_normal_mc_t
{
    ZXIC_UINT32 weight_normal_mc;
}DPP_NPPU_ISU_CFG_WEIGHT_NORMAL_MC_T;

typedef struct dpp_nppu_isu_cfg_weight_sa_mc_t
{
    ZXIC_UINT32 weight_sa_mc;
}DPP_NPPU_ISU_CFG_WEIGHT_SA_MC_T;

typedef struct dpp_nppu_isu_cfg_weight_etm_t
{
    ZXIC_UINT32 weight_etm;
}DPP_NPPU_ISU_CFG_WEIGHT_ETM_T;

typedef struct dpp_nppu_isu_cfg_weight_lp_mc_t
{
    ZXIC_UINT32 weight_lp_mc;
}DPP_NPPU_ISU_CFG_WEIGHT_LP_MC_T;

typedef struct dpp_nppu_isu_cfg_weight_oam_t
{
    ZXIC_UINT32 weight_oam;
}DPP_NPPU_ISU_CFG_WEIGHT_OAM_T;

typedef struct dpp_nppu_isu_cfg_weight_lif_ctrl1_t
{
    ZXIC_UINT32 weight_lif_ctrl1;
}DPP_NPPU_ISU_CFG_WEIGHT_LIF_CTRL1_T;

typedef struct dpp_nppu_isu_cfg_weight_lif_ctrl2_t
{
    ZXIC_UINT32 weight_lif_ctrl2;
}DPP_NPPU_ISU_CFG_WEIGHT_LIF_CTRL2_T;

typedef struct dpp_nppu_isu_cfg_ecc_bypass_read_t
{
    ZXIC_UINT32 eccbypass;
}DPP_NPPU_ISU_CFG_ECC_BYPASS_READ_T;

typedef struct dpp_nppu_isu_cfg_isu_int_mask_t
{
    ZXIC_UINT32 isu_int_mask;
}DPP_NPPU_ISU_CFG_ISU_INT_MASK_T;

typedef struct dpp_nppu_isu_cfg_cfg_crdt_cycle_t
{
    ZXIC_UINT32 cfg_cycle;
}DPP_NPPU_ISU_CFG_CFG_CRDT_CYCLE_T;

typedef struct dpp_nppu_isu_cfg_cfg_crdt_value_t
{
    ZXIC_UINT32 cfg_value;
}DPP_NPPU_ISU_CFG_CFG_CRDT_VALUE_T;

typedef struct dpp_nppu_isu_cfg_isu_int_en_t
{
    ZXIC_UINT32 isu_int_en;
}DPP_NPPU_ISU_CFG_ISU_INT_EN_T;

typedef struct dpp_nppu_isu_cfg_isu_ppu_fifo_fc_t
{
    ZXIC_UINT32 isu_ppu_fifo_fc;
}DPP_NPPU_ISU_CFG_ISU_PPU_FIFO_FC_T;

typedef struct dpp_nppu_isu_cfg_isu_int_status_t
{
    ZXIC_UINT32 isu_int_status_26;
    ZXIC_UINT32 isu_int_status_25;
    ZXIC_UINT32 isu_int_status_24;
    ZXIC_UINT32 isu_int_status_23;
    ZXIC_UINT32 isu_int_status_22;
    ZXIC_UINT32 isu_int_status_21;
    ZXIC_UINT32 isu_int_status_20;
    ZXIC_UINT32 isu_int_status_19;
    ZXIC_UINT32 isu_int_status_18;
    ZXIC_UINT32 isu_int_status_17;
    ZXIC_UINT32 isu_int_status_16;
    ZXIC_UINT32 isu_int_status_15;
    ZXIC_UINT32 isu_int_status_14;
    ZXIC_UINT32 isu_int_status_13;
    ZXIC_UINT32 isu_int_status_12;
    ZXIC_UINT32 isu_int_status_11;
    ZXIC_UINT32 isu_int_status_10;
    ZXIC_UINT32 isu_int_status_9;
    ZXIC_UINT32 isu_int_status_8;
    ZXIC_UINT32 isu_int_status_7;
    ZXIC_UINT32 isu_int_status_6;
    ZXIC_UINT32 isu_int_status_5;
    ZXIC_UINT32 isu_int_status_4;
    ZXIC_UINT32 isu_int_status_3;
    ZXIC_UINT32 isu_int_status_2;
    ZXIC_UINT32 isu_int_status_1;
    ZXIC_UINT32 isu_int_status_0;
}DPP_NPPU_ISU_CFG_ISU_INT_STATUS_T;

typedef struct dpp_nppu_isu_cfg_fd_prog_full_assert_cfg_t
{
    ZXIC_UINT32 fd_prog_full_assert_cfg;
}DPP_NPPU_ISU_CFG_FD_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_nppu_isu_cfg_fd_prog_full_negate_cfg_t
{
    ZXIC_UINT32 fd_prog_full_negate_cfg;
}DPP_NPPU_ISU_CFG_FD_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_nppu_isu_cfg_lp_prog_full_assert_cfg_t
{
    ZXIC_UINT32 lp_prog_ept_assert_cfg;
}DPP_NPPU_ISU_CFG_LP_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_nppu_isu_cfg_lp_prog_full_negate_cfg_t
{
    ZXIC_UINT32 lp_prog_ept_negate_cfg;
}DPP_NPPU_ISU_CFG_LP_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_dat0_t
{
    ZXIC_UINT32 debug_cnt_dat0;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_DAT0_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_dat1_t
{
    ZXIC_UINT32 debug_cnt_dat1;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_DAT1_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_dat2_t
{
    ZXIC_UINT32 debug_cnt_dat2;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_DAT2_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_dat3_t
{
    ZXIC_UINT32 debug_cnt_dat3;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_DAT3_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_dat4_t
{
    ZXIC_UINT32 debug_cnt_dat4;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_DAT4_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_dat5_t
{
    ZXIC_UINT32 debug_cnt_dat5;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_DAT5_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_dat6_t
{
    ZXIC_UINT32 debug_cnt_dat6;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_DAT6_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_dat7_t
{
    ZXIC_UINT32 debug_cnt_dat7;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_DAT7_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_dat8_t
{
    ZXIC_UINT32 debug_cnt_dat8;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_DAT8_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_dat9_t
{
    ZXIC_UINT32 debug_cnt_dat9;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_DAT9_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_dat10_t
{
    ZXIC_UINT32 debug_cnt_dat10;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_DAT10_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_dat11_t
{
    ZXIC_UINT32 debug_cnt_dat11;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_DAT11_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_dat12_t
{
    ZXIC_UINT32 debug_cnt_dat12;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_DAT12_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_dat13_t
{
    ZXIC_UINT32 debug_cnt_dat13;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_DAT13_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_dat14_t
{
    ZXIC_UINT32 debug_cnt_dat14;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_DAT14_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_dat15_t
{
    ZXIC_UINT32 debug_cnt_dat15;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_DAT15_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_dat16_t
{
    ZXIC_UINT32 debug_cnt_dat16;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_DAT16_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_dat17_t
{
    ZXIC_UINT32 debug_cnt_dat17;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_DAT17_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_dat18_t
{
    ZXIC_UINT32 debug_cnt_dat18;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_DAT18_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_dat19_t
{
    ZXIC_UINT32 debug_cnt_dat18;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_DAT19_T;

typedef struct dpp_nppu_isu_stat_debug_cnt_cfg_t
{
    ZXIC_UINT32 debug_cnt_ovf_mode;
    ZXIC_UINT32 debug_cnt_rdclr_mode;
    ZXIC_UINT32 user_cnt_value;
}DPP_NPPU_ISU_STAT_DEBUG_CNT_CFG_T;

typedef struct dpp_nppu_odma_cfg_exsa_tdm_offset_t
{
    ZXIC_UINT32 exsa_tdm_offset;
}DPP_NPPU_ODMA_CFG_EXSA_TDM_OFFSET_T;

typedef struct dpp_nppu_odma_cfg_ecc_bypass_readt_t
{
    ZXIC_UINT32 ecc_bypass_read;
}DPP_NPPU_ODMA_CFG_ECC_BYPASS_READT_T;

typedef struct dpp_nppu_odma_cfg_odma_int_en_0_t
{
    ZXIC_UINT32 odma_int_en_31;
    ZXIC_UINT32 odma_int_en_30;
    ZXIC_UINT32 odma_int_en_29;
    ZXIC_UINT32 odma_int_en_28;
    ZXIC_UINT32 odma_int_en_27;
    ZXIC_UINT32 odma_int_en_26;
    ZXIC_UINT32 odma_int_en_25;
    ZXIC_UINT32 odma_int_en_24;
    ZXIC_UINT32 odma_int_en_22;
    ZXIC_UINT32 odma_int_en_21;
    ZXIC_UINT32 odma_int_en_18;
}DPP_NPPU_ODMA_CFG_ODMA_INT_EN_0_T;

typedef struct dpp_nppu_odma_cfg_odma_int_en_1_t
{
    ZXIC_UINT32 odma_int_en_63;
    ZXIC_UINT32 odma_int_en_62;
    ZXIC_UINT32 odma_int_en_61;
    ZXIC_UINT32 odma_int_en_59;
    ZXIC_UINT32 odma_int_en_58;
    ZXIC_UINT32 odma_int_en_57;
    ZXIC_UINT32 odma_int_en_56;
    ZXIC_UINT32 odma_int_en_55;
    ZXIC_UINT32 odma_int_en_54;
    ZXIC_UINT32 odma_int_en_53;
    ZXIC_UINT32 odma_int_en_52;
    ZXIC_UINT32 odma_int_en_51;
    ZXIC_UINT32 odma_int_en_49;
    ZXIC_UINT32 odma_int_en_47;
    ZXIC_UINT32 odma_int_en_45;
    ZXIC_UINT32 odma_int_en_39;
    ZXIC_UINT32 odma_int_en_38;
    ZXIC_UINT32 odma_int_en_37;
    ZXIC_UINT32 odma_int_en_36;
    ZXIC_UINT32 odma_int_en_35;
    ZXIC_UINT32 odma_int_en_34;
    ZXIC_UINT32 odma_int_en_33;
    ZXIC_UINT32 odma_int_en_32;
}DPP_NPPU_ODMA_CFG_ODMA_INT_EN_1_T;

typedef struct dpp_nppu_odma_cfg_odma_int_en_2_t
{
    ZXIC_UINT32 odma_int_en_91;
    ZXIC_UINT32 odma_int_en_88;
    ZXIC_UINT32 odma_int_en_85;
    ZXIC_UINT32 odma_int_en_82;
    ZXIC_UINT32 odma_int_en_79;
    ZXIC_UINT32 odma_int_en_75;
    ZXIC_UINT32 odma_int_en_74;
    ZXIC_UINT32 odma_int_en_71;
    ZXIC_UINT32 odma_int_en_65;
    ZXIC_UINT32 odma_int_en_64;
}DPP_NPPU_ODMA_CFG_ODMA_INT_EN_2_T;

typedef struct dpp_nppu_odma_cfg_odma_int_en_3_t
{
    ZXIC_UINT32 odma_int_en_115;
    ZXIC_UINT32 odma_int_en_114;
    ZXIC_UINT32 odma_int_en_112;
    ZXIC_UINT32 odma_int_en_110;
    ZXIC_UINT32 odma_int_en_109;
    ZXIC_UINT32 odma_int_en_108;
    ZXIC_UINT32 odma_int_en_107;
    ZXIC_UINT32 odma_int_en_106;
    ZXIC_UINT32 odma_int_en_102;
    ZXIC_UINT32 odma_int_en_101;
    ZXIC_UINT32 odma_int_en_100;
    ZXIC_UINT32 odma_int_en_98;
    ZXIC_UINT32 odma_int_en_96;
}DPP_NPPU_ODMA_CFG_ODMA_INT_EN_3_T;

typedef struct dpp_nppu_odma_cfg_odma_int_mask_0_t
{
    ZXIC_UINT32 odma_int_mask_31;
    ZXIC_UINT32 odma_int_mask_30;
    ZXIC_UINT32 odma_int_mask_29;
    ZXIC_UINT32 odma_int_mask_28;
    ZXIC_UINT32 odma_int_mask_27;
    ZXIC_UINT32 odma_int_mask_26;
    ZXIC_UINT32 odma_int_mask_25;
    ZXIC_UINT32 odma_int_mask_24;
    ZXIC_UINT32 odma_int_mask_22;
    ZXIC_UINT32 odma_int_mask_21;
    ZXIC_UINT32 odma_int_mask_18;
}DPP_NPPU_ODMA_CFG_ODMA_INT_MASK_0_T;

typedef struct dpp_nppu_odma_cfg_odma_int_mask_1_t
{
    ZXIC_UINT32 odma_int_mask_63;
    ZXIC_UINT32 odma_int_mask_62;
    ZXIC_UINT32 odma_int_mask_61;
    ZXIC_UINT32 odma_int_mask_59;
    ZXIC_UINT32 odma_int_mask_58;
    ZXIC_UINT32 odma_int_mask_57;
    ZXIC_UINT32 odma_int_mask_56;
    ZXIC_UINT32 odma_int_mask_55;
    ZXIC_UINT32 odma_int_mask_54;
    ZXIC_UINT32 odma_int_mask_53;
    ZXIC_UINT32 odma_int_mask_52;
    ZXIC_UINT32 odma_int_mask_51;
    ZXIC_UINT32 odma_int_mask_50;
    ZXIC_UINT32 odma_int_mask_49;
    ZXIC_UINT32 odma_int_mask_47;
    ZXIC_UINT32 odma_int_mask_45;
    ZXIC_UINT32 odma_int_mask_39;
    ZXIC_UINT32 odma_int_mask_38;
    ZXIC_UINT32 odma_int_mask_37;
    ZXIC_UINT32 odma_int_mask_36;
    ZXIC_UINT32 odma_int_mask_35;
    ZXIC_UINT32 odma_int_mask_34;
    ZXIC_UINT32 odma_int_mask_33;
    ZXIC_UINT32 odma_int_mask_32;
}DPP_NPPU_ODMA_CFG_ODMA_INT_MASK_1_T;

typedef struct dpp_nppu_odma_cfg_odma_int_mask_2_t
{
    ZXIC_UINT32 odma_int_mask_91;
    ZXIC_UINT32 odma_int_mask_88;
    ZXIC_UINT32 odma_int_mask_85;
    ZXIC_UINT32 odma_int_mask_82;
    ZXIC_UINT32 odma_int_mask_79;
    ZXIC_UINT32 odma_int_mask_75;
    ZXIC_UINT32 odma_int_mask_74;
    ZXIC_UINT32 odma_int_mask_71;
    ZXIC_UINT32 odma_int_mask_65;
    ZXIC_UINT32 odma_int_mask_64;
}DPP_NPPU_ODMA_CFG_ODMA_INT_MASK_2_T;

typedef struct dpp_nppu_odma_cfg_odma_int_mask_3_t
{
    ZXIC_UINT32 odma_int_mask_115;
    ZXIC_UINT32 odma_int_mask_114;
    ZXIC_UINT32 odma_int_mask_112;
    ZXIC_UINT32 odma_int_mask_110;
    ZXIC_UINT32 odma_int_mask_109;
    ZXIC_UINT32 odma_int_mask_108;
    ZXIC_UINT32 odma_int_mask_107;
    ZXIC_UINT32 odma_int_mask_106;
    ZXIC_UINT32 odma_int_mask_102;
    ZXIC_UINT32 odma_int_mask_101;
    ZXIC_UINT32 odma_int_mask_100;
    ZXIC_UINT32 odma_int_mask_98;
    ZXIC_UINT32 odma_int_mask_96;
}DPP_NPPU_ODMA_CFG_ODMA_INT_MASK_3_T;

typedef struct dpp_nppu_odma_cfg_odma_int_status_0_t
{
    ZXIC_UINT32 odma_int_status_31;
    ZXIC_UINT32 odma_int_status_30;
    ZXIC_UINT32 odma_int_status_29;
    ZXIC_UINT32 odma_int_status_28;
    ZXIC_UINT32 odma_int_status_27;
    ZXIC_UINT32 odma_int_status_26;
    ZXIC_UINT32 odma_int_status_25;
    ZXIC_UINT32 odma_int_status_24;
    ZXIC_UINT32 odma_int_status_22;
    ZXIC_UINT32 odma_int_status_21;
    ZXIC_UINT32 odma_int_status_18;
}DPP_NPPU_ODMA_CFG_ODMA_INT_STATUS_0_T;

typedef struct dpp_nppu_odma_cfg_odma_int_status_1_t
{
    ZXIC_UINT32 odma_int_status_63;
    ZXIC_UINT32 odma_int_status_62;
    ZXIC_UINT32 odma_int_status_61;
    ZXIC_UINT32 odma_int_status_59;
    ZXIC_UINT32 odma_int_status_58;
    ZXIC_UINT32 odma_int_status_57;
    ZXIC_UINT32 odma_int_status_56;
    ZXIC_UINT32 odma_int_status_55;
    ZXIC_UINT32 odma_int_status_54;
    ZXIC_UINT32 odma_int_status_53;
    ZXIC_UINT32 odma_int_status_52;
    ZXIC_UINT32 odma_int_status_51;
    ZXIC_UINT32 odma_int_status_49;
    ZXIC_UINT32 odma_int_status_47;
    ZXIC_UINT32 odma_int_status_45;
    ZXIC_UINT32 odma_int_status_39;
    ZXIC_UINT32 odma_int_status_38;
    ZXIC_UINT32 odma_int_status_37;
    ZXIC_UINT32 odma_int_status_36;
    ZXIC_UINT32 odma_int_status_35;
    ZXIC_UINT32 odma_int_status_34;
    ZXIC_UINT32 odma_int_status_33;
    ZXIC_UINT32 odma_int_status_32;
}DPP_NPPU_ODMA_CFG_ODMA_INT_STATUS_1_T;

typedef struct dpp_nppu_odma_cfg_odma_int_status_2_t
{
    ZXIC_UINT32 odma_int_status_91;
    ZXIC_UINT32 odma_int_status_88;
    ZXIC_UINT32 odma_int_status_85;
    ZXIC_UINT32 odma_int_status_82;
    ZXIC_UINT32 odma_int_status_79;
    ZXIC_UINT32 odma_int_status_75;
    ZXIC_UINT32 odma_int_status_74;
    ZXIC_UINT32 odma_int_status_71;
    ZXIC_UINT32 odma_int_status_65;
    ZXIC_UINT32 odma_int_status_64;
}DPP_NPPU_ODMA_CFG_ODMA_INT_STATUS_2_T;

typedef struct dpp_nppu_odma_cfg_odma_int_status_3_t
{
    ZXIC_UINT32 odma_int_status_117;
    ZXIC_UINT32 odma_int_status_116;
    ZXIC_UINT32 odma_int_status_115;
    ZXIC_UINT32 odma_int_status_114;
    ZXIC_UINT32 odma_int_status_112;
    ZXIC_UINT32 odma_int_status_110;
    ZXIC_UINT32 odma_int_status_109;
    ZXIC_UINT32 odma_int_status_108;
    ZXIC_UINT32 odma_int_status_107;
    ZXIC_UINT32 odma_int_status_106;
    ZXIC_UINT32 odma_int_status_102;
    ZXIC_UINT32 odma_int_status_101;
    ZXIC_UINT32 odma_int_status_100;
    ZXIC_UINT32 odma_int_status_98;
    ZXIC_UINT32 odma_int_status_96;
}DPP_NPPU_ODMA_CFG_ODMA_INT_STATUS_3_T;

typedef struct dpp_nppu_odma_cfg_sp_tdm_err_nor_cfg_t
{
    ZXIC_UINT32 sp_tdm_err_nor_cfg;
}DPP_NPPU_ODMA_CFG_SP_TDM_ERR_NOR_CFG_T;

typedef struct dpp_nppu_odma_cfg_etm_dis_ptr_prog_full_cfg_a_t
{
    ZXIC_UINT32 etm_dis_ptr_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_ETM_DIS_PTR_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_etm_dis_ptr_prog_full_cfg_n_t
{
    ZXIC_UINT32 etm_dis_ptr_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_ETM_DIS_PTR_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_ftm_dis_ptr_prog_full_cfg_a_t
{
    ZXIC_UINT32 ftm_dis_ptr_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_FTM_DIS_PTR_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_ftm_dis_ptr_prog_full_cfg_n_t
{
    ZXIC_UINT32 ftm_dis_ptr_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_FTM_DIS_PTR_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_tm_dis_fifo_prog_full_cfg_a_t
{
    ZXIC_UINT32 tm_dis_fifo_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_TM_DIS_FIFO_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_tm_dis_fifo_prog_full_cfg_n_t
{
    ZXIC_UINT32 tm_dis_fifo_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_TM_DIS_FIFO_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_err_prog_full_cfg_a_t
{
    ZXIC_UINT32 err_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_ERR_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_err_prog_full_cfg_n_t
{
    ZXIC_UINT32 err_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_ERR_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_tdmuc_prog_full_cfg_a_t
{
    ZXIC_UINT32 tdmuc_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_TDMUC_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_tdmuc_prog_full_cfg_n_t
{
    ZXIC_UINT32 tdmuc_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_TDMUC_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_tdmmc_groupid_prog_full_cfg_a_t
{
    ZXIC_UINT32 tdmmc_groupid_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_TDMMC_GROUPID_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_tdmmc_groupid_prog_full_cfg_n_t
{
    ZXIC_UINT32 tdmmc_groupid_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_TDMMC_GROUPID_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_tdmmc_no_bitmap_prog_full_cfg_a_t
{
    ZXIC_UINT32 tdmmc_no_bitmap_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_TDMMC_NO_BITMAP_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_tdmmc_no_bitmap_prog_full_cfg_n_t
{
    ZXIC_UINT32 tdmmc_no_bitmap_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_TDMMC_NO_BITMAP_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_tdmmc_prog_full_cfg_a_t
{
    ZXIC_UINT32 tdmmc_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_TDMMC_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_tdmmc_prog_full_cfg_n_t
{
    ZXIC_UINT32 tdmmc_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_TDMMC_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_desc_prog_full_cfg_a_t
{
    ZXIC_UINT32 desc_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_DESC_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_desc_prog_full_cfg_n_t
{
    ZXIC_UINT32 desc_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_DESC_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_dly_prog_full_cfg_a_t
{
    ZXIC_UINT32 dly_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_DLY_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_dly_prog_full_cfg_n_t
{
    ZXIC_UINT32 dly_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_DLY_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_rsp_prog_full_cfg_a_t
{
    ZXIC_UINT32 rsp_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_RSP_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_rsp_prog_full_cfg_n_t
{
    ZXIC_UINT32 rsp_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_RSP_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_nor_prog_full_cfg_a_t
{
    ZXIC_UINT32 nor_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_NOR_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_nor_prog_full_cfg_n_t
{
    ZXIC_UINT32 nor_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_NOR_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_etm_nor_prog_full_cfg_a_t
{
    ZXIC_UINT32 etm_nor_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_ETM_NOR_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_etm_nor_prog_full_cfg_n_t
{
    ZXIC_UINT32 etm_nor_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_ETM_NOR_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_ftm_nor_prog_full_cfg_a_t
{
    ZXIC_UINT32 ftm_nor_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_FTM_NOR_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_ftm_nor_prog_full_cfg_n_t
{
    ZXIC_UINT32 ftm_nor_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_FTM_NOR_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_etm_prog_full_cfg_a_t
{
    ZXIC_UINT32 etm_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_ETM_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_etm_prog_full_cfg_n_t
{
    ZXIC_UINT32 etm_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_ETM_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_ftm_prog_full_cfg_a_t
{
    ZXIC_UINT32 ftm_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_FTM_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_ftm_prog_full_cfg_n_t
{
    ZXIC_UINT32 ftm_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_FTM_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_etm_nrdcnt_prog_full_cfg_a_t
{
    ZXIC_UINT32 etm_nrdcnt_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_ETM_NRDCNT_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_etm_nrdcnt_prog_full_cfg_n_t
{
    ZXIC_UINT32 etm_nrdcnt_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_ETM_NRDCNT_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_ftm_nrdcnt_prog_full_cfg_a_t
{
    ZXIC_UINT32 ftm_nrdcnt_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_FTM_NRDCNT_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_ftm_nrdcnt_prog_full_cfg_n_t
{
    ZXIC_UINT32 ftm_nrdcnt_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_FTM_NRDCNT_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_pp_prog_full_cfg_a_t
{
    ZXIC_UINT32 pp_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_PP_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_pp_prog_full_cfg_n_t
{
    ZXIC_UINT32 pp_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_PP_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_tm_weight_t
{
    ZXIC_UINT32 tm_weight;
}DPP_NPPU_ODMA_CFG_TM_WEIGHT_T;

typedef struct dpp_nppu_odma_cfg_pp_weight_t
{
    ZXIC_UINT32 pp_weight;
}DPP_NPPU_ODMA_CFG_PP_WEIGHT_T;

typedef struct dpp_nppu_odma_cfg_ifbcmd_prog_full_cfg_a_t
{
    ZXIC_UINT32 ifbcmd_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_IFBCMD_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_ifbcmd_prog_full_cfg_n_t
{
    ZXIC_UINT32 ifbcmd_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_IFBCMD_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_mccnt_prog_full_cfg_a_t
{
    ZXIC_UINT32 mccnt_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_MCCNT_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_mccnt_prog_full_cfg_n_t
{
    ZXIC_UINT32 mccnt_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_MCCNT_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_int_or_pon_t
{
    ZXIC_UINT32 int_or_pon;
}DPP_NPPU_ODMA_CFG_INT_OR_PON_T;

typedef struct dpp_nppu_odma_cfg_quemng_cnt_in_err_cnt_t
{
    ZXIC_UINT32 quemng_cnt_in_err_cnt;
}DPP_NPPU_ODMA_CFG_QUEMNG_CNT_IN_ERR_CNT_T;

typedef struct dpp_nppu_odma_cfg_lif0_port_eop_cnt_t
{
    ZXIC_UINT32 lif0_port_eop_cnt;
}DPP_NPPU_ODMA_CFG_LIF0_PORT_EOP_CNT_T;

typedef struct dpp_nppu_odma_cfg_lif1_port_eop_cnt_t
{
    ZXIC_UINT32 lif1_port_eop_cnt;
}DPP_NPPU_ODMA_CFG_LIF1_PORT_EOP_CNT_T;

typedef struct dpp_nppu_odma_cfg_lifc_port0_eop_cnt_t
{
    ZXIC_UINT32 lifc_port0_eop_cnt;
}DPP_NPPU_ODMA_CFG_LIFC_PORT0_EOP_CNT_T;

typedef struct dpp_nppu_odma_cfg_lifc_port1_eop_cnt_t
{
    ZXIC_UINT32 lifc_port1_eop_cnt;
}DPP_NPPU_ODMA_CFG_LIFC_PORT1_EOP_CNT_T;

typedef struct dpp_nppu_odma_cfg_fptr_fifo_prog_ept_cfg_n_t
{
    ZXIC_UINT32 fptr_fifo_prog_ept_cfg_n;
}DPP_NPPU_ODMA_CFG_FPTR_FIFO_PROG_EPT_CFG_N_T;

typedef struct dpp_nppu_odma_cfg_isu_fifo_prog_full_cfg_a_t
{
    ZXIC_UINT32 isu_fifo_prog_full_cfg_a;
}DPP_NPPU_ODMA_CFG_ISU_FIFO_PROG_FULL_CFG_A_T;

typedef struct dpp_nppu_odma_cfg_isu_fifo_prog_full_cfg_n_t
{
    ZXIC_UINT32 isu_fifo_prog_full_cfg_n;
}DPP_NPPU_ODMA_CFG_ISU_FIFO_PROG_FULL_CFG_N_T;

typedef struct dpp_nppu_oam_cfg_ind_access_done_t
{
    ZXIC_UINT32 ind_access_done;
}DPP_NPPU_OAM_CFG_IND_ACCESS_DONE_T;

typedef struct dpp_nppu_oam_cfg_ind_access_command_t
{
    ZXIC_UINT32 ind_rd_or_wr;
    ZXIC_UINT32 ind_mem_mask;
    ZXIC_UINT32 ind_mem_id;
    ZXIC_UINT32 ind_mem_addr;
}DPP_NPPU_OAM_CFG_IND_ACCESS_COMMAND_T;

typedef struct dpp_nppu_oam_cfg_ind_dat0_t
{
    ZXIC_UINT32 ind_dat0;
}DPP_NPPU_OAM_CFG_IND_DAT0_T;

typedef struct dpp_nppu_oam_cfg_ind_dat1_t
{
    ZXIC_UINT32 ind_dat1;
}DPP_NPPU_OAM_CFG_IND_DAT1_T;

typedef struct dpp_nppu_oam_cfg_ind_dat2_t
{
    ZXIC_UINT32 ind_dat2;
}DPP_NPPU_OAM_CFG_IND_DAT2_T;

typedef struct dpp_nppu_oam_cfg_ind_dat3_t
{
    ZXIC_UINT32 ind_dat3;
}DPP_NPPU_OAM_CFG_IND_DAT3_T;

typedef struct dpp_nppu_oam_cfg_oam_tx_main_en_t
{
    ZXIC_UINT32 oam_tx_main_en;
}DPP_NPPU_OAM_CFG_OAM_TX_MAIN_EN_T;

typedef struct dpp_nppu_oam_cfg_tx_total_num_t
{
    ZXIC_UINT32 tx_total_num;
}DPP_NPPU_OAM_CFG_TX_TOTAL_NUM_T;

typedef struct dpp_nppu_oam_cfg_oam_chk_main_en_t
{
    ZXIC_UINT32 oam_chk_main_en;
}DPP_NPPU_OAM_CFG_OAM_CHK_MAIN_EN_T;

typedef struct dpp_nppu_oam_cfg_chk_total_num0_t
{
    ZXIC_UINT32 chk_total_num0;
}DPP_NPPU_OAM_CFG_CHK_TOTAL_NUM0_T;

typedef struct dpp_nppu_oam_cfg_ma_chk_main_en_t
{
    ZXIC_UINT32 oam_chk_main_en;
}DPP_NPPU_OAM_CFG_MA_CHK_MAIN_EN_T;

typedef struct dpp_nppu_oam_cfg_chk_total_num1_t
{
    ZXIC_UINT32 chk_total_num0;
}DPP_NPPU_OAM_CFG_CHK_TOTAL_NUM1_T;

typedef struct dpp_nppu_oam_cfg_tx_stat_en_t
{
    ZXIC_UINT32 tx_stat_en;
}DPP_NPPU_OAM_CFG_TX_STAT_EN_T;

typedef struct dpp_nppu_oam_cfg_rec_stat_en_t
{
    ZXIC_UINT32 rec_stat_en;
}DPP_NPPU_OAM_CFG_REC_STAT_EN_T;

typedef struct dpp_nppu_oam_cfg_stat_oam_rdy_mask_t
{
    ZXIC_UINT32 stat_oam_rdy_mask;
}DPP_NPPU_OAM_CFG_STAT_OAM_RDY_MASK_T;

typedef struct dpp_nppu_oam_cfg_session_grading0_t
{
    ZXIC_UINT32 session_grading0;
}DPP_NPPU_OAM_CFG_SESSION_GRADING0_T;

typedef struct dpp_nppu_oam_cfg_session_grading1_t
{
    ZXIC_UINT32 session_grading1;
}DPP_NPPU_OAM_CFG_SESSION_GRADING1_T;

typedef struct dpp_nppu_oam_cfg_session_grading2_t
{
    ZXIC_UINT32 session_grading2;
}DPP_NPPU_OAM_CFG_SESSION_GRADING2_T;

typedef struct dpp_nppu_oam_cfg_session_grading3_t
{
    ZXIC_UINT32 session_grading3;
}DPP_NPPU_OAM_CFG_SESSION_GRADING3_T;

typedef struct dpp_nppu_oam_cfg_bfd_chk_haddr_t
{
    ZXIC_UINT32 bfd_chk_haddr;
}DPP_NPPU_OAM_CFG_BFD_CHK_HADDR_T;

typedef struct dpp_nppu_oam_cfg_ethccm_chk_haddr_t
{
    ZXIC_UINT32 ethccm_chk_haddr;
}DPP_NPPU_OAM_CFG_ETHCCM_CHK_HADDR_T;

typedef struct dpp_nppu_oam_cfg_tpbfd_chk_haddr_t
{
    ZXIC_UINT32 tpbfd_chk_haddr;
}DPP_NPPU_OAM_CFG_TPBFD_CHK_HADDR_T;

typedef struct dpp_nppu_oam_cfg_tpoam_ccm_chk_haddr_t
{
    ZXIC_UINT32 tpoam_ccm_chk_haddr;
}DPP_NPPU_OAM_CFG_TPOAM_CCM_CHK_HADDR_T;

typedef struct dpp_nppu_oam_cfg_bfd_tx_haddr_t
{
    ZXIC_UINT32 bfd_tx_haddr;
}DPP_NPPU_OAM_CFG_BFD_TX_HADDR_T;

typedef struct dpp_nppu_oam_cfg_ethccm_tx_haddr_t
{
    ZXIC_UINT32 ethccm_tx_haddr;
}DPP_NPPU_OAM_CFG_ETHCCM_TX_HADDR_T;

typedef struct dpp_nppu_oam_cfg_tpbfd_tx_haddr_t
{
    ZXIC_UINT32 tpbfd_tx_haddr;
}DPP_NPPU_OAM_CFG_TPBFD_TX_HADDR_T;

typedef struct dpp_nppu_oam_cfg_tpoam_ccm_tx_haddr_t
{
    ZXIC_UINT32 tpoam_ccm_tx_haddr;
}DPP_NPPU_OAM_CFG_TPOAM_CCM_TX_HADDR_T;

typedef struct dpp_nppu_oam_cfg_ethccm_ma_chk_haddr_t
{
    ZXIC_UINT32 ethccm_ma_chk_haddr;
}DPP_NPPU_OAM_CFG_ETHCCM_MA_CHK_HADDR_T;

typedef struct dpp_nppu_oam_cfg_tpccm_ma_chk_haddr_t
{
    ZXIC_UINT32 tpccm_ma_chk_haddr;
}DPP_NPPU_OAM_CFG_TPCCM_MA_CHK_HADDR_T;

typedef struct dpp_nppu_oam_cfg_groupnum_ram_clr_t
{
    ZXIC_UINT32 groupnum_ram_clr;
}DPP_NPPU_OAM_CFG_GROUPNUM_RAM_CLR_T;

typedef struct dpp_nppu_oam_cfg_index_ram0_clr_t
{
    ZXIC_UINT32 index_ram0_clr;
}DPP_NPPU_OAM_CFG_INDEX_RAM0_CLR_T;

typedef struct dpp_nppu_oam_cfg_index_ram1_clr_t
{
    ZXIC_UINT32 index_ram1_clr;
}DPP_NPPU_OAM_CFG_INDEX_RAM1_CLR_T;

typedef struct dpp_nppu_oam_cfg_rmep_ram_clr_t
{
    ZXIC_UINT32 rmep_ram_clr;
}DPP_NPPU_OAM_CFG_RMEP_RAM_CLR_T;

typedef struct dpp_nppu_oam_cfg_ma_ram_clr_t
{
    ZXIC_UINT32 ma_ram_clr;
}DPP_NPPU_OAM_CFG_MA_RAM_CLR_T;

typedef struct dpp_nppu_oam_cfg_ram_init_done_t
{
    ZXIC_UINT32 ram_init_done;
}DPP_NPPU_OAM_CFG_RAM_INIT_DONE_T;

typedef struct dpp_nppu_oam_cfg_rec_bfd_debug_en_t
{
    ZXIC_UINT32 rec_bfd_debug_en;
}DPP_NPPU_OAM_CFG_REC_BFD_DEBUG_EN_T;

typedef struct dpp_nppu_oam_cfg_oam_session_int_t
{
    ZXIC_UINT32 tpma_int;
    ZXIC_UINT32 ethma_int;
    ZXIC_UINT32 bfd_int;
    ZXIC_UINT32 ethoam_int;
    ZXIC_UINT32 tpbfd_int;
    ZXIC_UINT32 tpoam_int;
}DPP_NPPU_OAM_CFG_OAM_SESSION_INT_T;

typedef struct dpp_nppu_oam_cfg_pon_int_t
{
    ZXIC_UINT32 fifo_int;
    ZXIC_UINT32 pon_protect_int;
}DPP_NPPU_OAM_CFG_PON_INT_T;

typedef struct dpp_nppu_oam_cfg_oam_int_clr_t
{
    ZXIC_UINT32 oam_int_clr;
}DPP_NPPU_OAM_CFG_OAM_INT_CLR_T;

typedef struct dpp_nppu_oam_cfg_type_int_clr0_t
{
    ZXIC_UINT32 tpma_int_clr;
    ZXIC_UINT32 ethma_int_clr;
    ZXIC_UINT32 bfd_int_clr;
    ZXIC_UINT32 ethoam_int_clr;
    ZXIC_UINT32 tpbfd_int_clr;
    ZXIC_UINT32 tpoam_int_clr;
}DPP_NPPU_OAM_CFG_TYPE_INT_CLR0_T;

typedef struct dpp_nppu_oam_cfg_type_int_clr1_t
{
    ZXIC_UINT32 fifo_int_clr;
    ZXIC_UINT32 pon_protect_int_clr;
}DPP_NPPU_OAM_CFG_TYPE_INT_CLR1_T;

typedef struct dpp_nppu_oam_cfg_interrupt_mask_t
{
    ZXIC_UINT32 fifo_interrupt_mask;
    ZXIC_UINT32 pon_protect_interruptmask;
    ZXIC_UINT32 tpma_interrupt_mask;
    ZXIC_UINT32 ethma_interrupt_mask;
    ZXIC_UINT32 bfd_interrupt_mask;
    ZXIC_UINT32 ethoam_interrupt_mask;
    ZXIC_UINT32 tpbfd_interrupt_mask;
    ZXIC_UINT32 tpoam_interrupt_mask;
}DPP_NPPU_OAM_CFG_INTERRUPT_MASK_T;

typedef struct dpp_nppu_oam_cfg_int0_index_t
{
    ZXIC_UINT32 int0_index0;
}DPP_NPPU_OAM_CFG_INT0_INDEX_T;

typedef struct dpp_nppu_oam_cfg_int1_index_t
{
    ZXIC_UINT32 int1_index0;
}DPP_NPPU_OAM_CFG_INT1_INDEX_T;

typedef struct dpp_nppu_oam_cfg_int0_index_region_t
{
    ZXIC_UINT32 int0_index_region;
}DPP_NPPU_OAM_CFG_INT0_INDEX_REGION_T;

typedef struct dpp_nppu_oam_cfg_int1_index_region_t
{
    ZXIC_UINT32 int1_index_region;
}DPP_NPPU_OAM_CFG_INT1_INDEX_REGION_T;

typedef struct dpp_nppu_oam_cfg_bdiinfo_fwft_fifo_th_t
{
    ZXIC_UINT32 bdiinfo_fwft_fifo_th;
}DPP_NPPU_OAM_CFG_BDIINFO_FWFT_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_recsec_fwft_fifo_th_t
{
    ZXIC_UINT32 recsec_fwft_fifo_th;
}DPP_NPPU_OAM_CFG_RECSEC_FWFT_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_timing_chk_info0_fwft_fifo_th_t
{
    ZXIC_UINT32 timing_chk_info0_fwft_fifo_th;
}DPP_NPPU_OAM_CFG_TIMING_CHK_INFO0_FWFT_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_recma_fwft_fifo_th_t
{
    ZXIC_UINT32 recma_fwft_fifo_th;
}DPP_NPPU_OAM_CFG_RECMA_FWFT_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_timing_chk_info1_fwft_fifo_th_t
{
    ZXIC_UINT32 timing_chk_info1_fwft_fifo_th;
}DPP_NPPU_OAM_CFG_TIMING_CHK_INFO1_FWFT_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_oam_txinst_fifo_th_t
{
    ZXIC_UINT32 oam_txinst_fifo_th;
}DPP_NPPU_OAM_CFG_OAM_TXINST_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_oam_rdinfo_fwft_fifo_th_t
{
    ZXIC_UINT32 oam_rdinfo_fwft_fifo_th;
}DPP_NPPU_OAM_CFG_OAM_RDINFO_FWFT_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_lm_cnt_fwft_fifo_th_t
{
    ZXIC_UINT32 lm_cnt_fwft_fifo_th;
}DPP_NPPU_OAM_CFG_LM_CNT_FWFT_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_oam_pkt_fifo_th_t
{
    ZXIC_UINT32 oam_pkt_fifo_th;
}DPP_NPPU_OAM_CFG_OAM_PKT_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_reclm_stat_fifo_th_t
{
    ZXIC_UINT32 reclm_stat_fifo_th;
}DPP_NPPU_OAM_CFG_RECLM_STAT_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_txlm_stat_fifo_th_t
{
    ZXIC_UINT32 txlm_stat_fifo_th;
}DPP_NPPU_OAM_CFG_TXLM_STAT_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_oam_chk_fwft_fifo_th_t
{
    ZXIC_UINT32 oam_chk_fwft_fifo_th;
}DPP_NPPU_OAM_CFG_OAM_CHK_FWFT_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_txoam_stat_fifo_th_t
{
    ZXIC_UINT32 txoam_stat_fifo_th;
}DPP_NPPU_OAM_CFG_TXOAM_STAT_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_recoam_stat_fifo_th_t
{
    ZXIC_UINT32 recoam_stat_fifo_th;
}DPP_NPPU_OAM_CFG_RECOAM_STAT_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_txpkt_data_fwft_fifo_th_t
{
    ZXIC_UINT32 txpkt_data_fwft_fifo_th;
}DPP_NPPU_OAM_CFG_TXPKT_DATA_FWFT_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_tstpkt_fwft_fifo_th_t
{
    ZXIC_UINT32 tstpkt_fwft_fifo_th;
}DPP_NPPU_OAM_CFG_TSTPKT_FWFT_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_tst_txinst_fwft_fifo_th_t
{
    ZXIC_UINT32 tst_txinst_fwft_fifo_th;
}DPP_NPPU_OAM_CFG_TST_TXINST_FWFT_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_tstrx_main_en_t
{
    ZXIC_UINT32 tstrx_main_en;
}DPP_NPPU_OAM_CFG_TSTRX_MAIN_EN_T;

typedef struct dpp_nppu_oam_cfg_tsttx_cfg_para_tbl2_t
{
    ZXIC_UINT32 ddr_self_test_tx_en;
    ZXIC_UINT32 tm_self_test_tx_en;
    ZXIC_UINT32 fast_aging_tx_en;
    ZXIC_UINT32 timing_aging_tx_en;
    ZXIC_UINT32 backgroud_flow_tx_en;
    ZXIC_UINT32 tsttx_tx_en;
    ZXIC_UINT32 tx_freq;
    ZXIC_UINT32 tx_offset;
}DPP_NPPU_OAM_CFG_TSTTX_CFG_PARA_TBL2_T;

typedef struct dpp_nppu_oam_cfg_tsttx_cfg_para_tbl1_t
{
    ZXIC_UINT32 tx_count;
}DPP_NPPU_OAM_CFG_TSTTX_CFG_PARA_TBL1_T;

typedef struct dpp_nppu_oam_cfg_tsttx_cfg_para_tbl0_t
{
    ZXIC_UINT32 fast_tx_mode_en;
    ZXIC_UINT32 tsttx_tx_head_len;
    ZXIC_UINT32 tsttx_tx_interval;
}DPP_NPPU_OAM_CFG_TSTTX_CFG_PARA_TBL0_T;

typedef struct dpp_nppu_oam_cfg_tstrx_cfg_para_t
{
    ZXIC_UINT32 tstrx_session_num;
    ZXIC_UINT32 tstrx_session_en;
}DPP_NPPU_OAM_CFG_TSTRX_CFG_PARA_T;

typedef struct dpp_nppu_oam_cfg_fifo_status_int_en_0_t
{
    ZXIC_UINT32 fifo_status_int_en_31;
    ZXIC_UINT32 fifo_status_int_en_30;
    ZXIC_UINT32 fifo_status_int_en_29;
    ZXIC_UINT32 fifo_status_int_en_28;
    ZXIC_UINT32 fifo_status_int_en_27;
    ZXIC_UINT32 fifo_status_int_en_26;
    ZXIC_UINT32 fifo_status_int_en_25;
    ZXIC_UINT32 fifo_status_int_en_24;
    ZXIC_UINT32 fifo_status_int_en_23;
    ZXIC_UINT32 fifo_status_int_en_22;
    ZXIC_UINT32 fifo_status_int_en_21;
    ZXIC_UINT32 fifo_status_int_en_20;
    ZXIC_UINT32 fifo_status_int_en_19;
    ZXIC_UINT32 fifo_status_int_en_18;
    ZXIC_UINT32 fifo_status_int_en_17;
    ZXIC_UINT32 fifo_status_int_en_16;
    ZXIC_UINT32 fifo_status_int_en_15;
    ZXIC_UINT32 fifo_status_int_en_14;
    ZXIC_UINT32 fifo_status_int_en_13;
    ZXIC_UINT32 fifo_status_int_en_12;
    ZXIC_UINT32 fifo_status_int_en_11;
    ZXIC_UINT32 fifo_status_int_en_10;
    ZXIC_UINT32 fifo_status_int_en_9;
    ZXIC_UINT32 fifo_status_int_en_8;
    ZXIC_UINT32 fifo_status_int_en_7;
    ZXIC_UINT32 fifo_status_int_en_6;
    ZXIC_UINT32 fifo_status_int_en_5;
    ZXIC_UINT32 fifo_status_int_en_4;
    ZXIC_UINT32 fifo_status_int_en_3;
    ZXIC_UINT32 fifo_status_int_en_2;
    ZXIC_UINT32 fifo_status_int_en_1;
    ZXIC_UINT32 fifo_status_int_en_0;
}DPP_NPPU_OAM_CFG_FIFO_STATUS_INT_EN_0_T;

typedef struct dpp_nppu_oam_cfg_fifo_status_int_en_1_t
{
    ZXIC_UINT32 fifo_status_int_en_41;
    ZXIC_UINT32 fifo_status_int_en_40;
    ZXIC_UINT32 fifo_status_int_en_39;
    ZXIC_UINT32 fifo_status_int_en_38;
    ZXIC_UINT32 fifo_status_int_en_37;
    ZXIC_UINT32 fifo_status_int_en_36;
    ZXIC_UINT32 fifo_status_int_en_35;
    ZXIC_UINT32 fifo_status_int_en_34;
    ZXIC_UINT32 fifo_status_int_en_33;
    ZXIC_UINT32 fifo_status_int_en_32;
}DPP_NPPU_OAM_CFG_FIFO_STATUS_INT_EN_1_T;

typedef struct dpp_nppu_oam_cfg_fifo_status_int_mask_0_t
{
    ZXIC_UINT32 fifo_status_int_mask_31;
    ZXIC_UINT32 fifo_status_int_mask_30;
    ZXIC_UINT32 fifo_status_int_mask_29;
    ZXIC_UINT32 fifo_status_int_mask_28;
    ZXIC_UINT32 fifo_status_int_mask_27;
    ZXIC_UINT32 fifo_status_int_mask_26;
    ZXIC_UINT32 fifo_status_int_mask_25;
    ZXIC_UINT32 fifo_status_int_mask_24;
    ZXIC_UINT32 fifo_status_int_mask_23;
    ZXIC_UINT32 fifo_status_int_mask_22;
    ZXIC_UINT32 fifo_status_int_mask_21;
    ZXIC_UINT32 fifo_status_int_mask_20;
    ZXIC_UINT32 fifo_status_int_mask_19;
    ZXIC_UINT32 fifo_status_int_mask_18;
    ZXIC_UINT32 fifo_status_int_mask_17;
    ZXIC_UINT32 fifo_status_int_mask_16;
    ZXIC_UINT32 fifo_status_int_mask_15;
    ZXIC_UINT32 fifo_status_int_mask_14;
    ZXIC_UINT32 fifo_status_int_mask_13;
    ZXIC_UINT32 fifo_status_int_mask_12;
    ZXIC_UINT32 fifo_status_int_mask_11;
    ZXIC_UINT32 fifo_status_int_mask_10;
    ZXIC_UINT32 fifo_status_int_mask_9;
    ZXIC_UINT32 fifo_status_int_mask_8;
    ZXIC_UINT32 fifo_status_int_mask_7;
    ZXIC_UINT32 fifo_status_int_mask_6;
    ZXIC_UINT32 fifo_status_int_mask_5;
    ZXIC_UINT32 fifo_status_int_mask_4;
    ZXIC_UINT32 fifo_status_int_mask_3;
    ZXIC_UINT32 fifo_status_int_mask_2;
    ZXIC_UINT32 fifo_status_int_mask_1;
    ZXIC_UINT32 fifo_status_int_mask_0;
}DPP_NPPU_OAM_CFG_FIFO_STATUS_INT_MASK_0_T;

typedef struct dpp_nppu_oam_cfg_fifo_status_int_mask_1_t
{
    ZXIC_UINT32 fifo_status_int_mask_41;
    ZXIC_UINT32 fifo_status_int_mask_40;
    ZXIC_UINT32 fifo_status_int_mask_39;
    ZXIC_UINT32 fifo_status_int_mask_38;
    ZXIC_UINT32 fifo_status_int_mask_37;
    ZXIC_UINT32 fifo_status_int_mask_36;
    ZXIC_UINT32 fifo_status_int_mask_35;
    ZXIC_UINT32 fifo_status_int_mask_34;
    ZXIC_UINT32 fifo_status_int_mask_33;
    ZXIC_UINT32 fifo_status_int_mask_32;
}DPP_NPPU_OAM_CFG_FIFO_STATUS_INT_MASK_1_T;

typedef struct dpp_nppu_oam_cfg_fifo_status_int_status_t
{
    ZXIC_UINT32 fifo_status_int_status;
}DPP_NPPU_OAM_CFG_FIFO_STATUS_INT_STATUS_T;

typedef struct dpp_nppu_oam_cfg_main_frequency_t
{
    ZXIC_UINT32 main_frequency;
}DPP_NPPU_OAM_CFG_MAIN_FREQUENCY_T;

typedef struct dpp_nppu_oam_cfg_oam_cfg_type_t
{
    ZXIC_UINT32 oam_cfg_type;
}DPP_NPPU_OAM_CFG_OAM_CFG_TYPE_T;

typedef struct dpp_nppu_oam_cfg_fst_swch_eth_head0_t
{
    ZXIC_UINT32 fst_swch_eth_head;
}DPP_NPPU_OAM_CFG_FST_SWCH_ETH_HEAD0_T;

typedef struct dpp_nppu_oam_cfg_fst_swch_eth_head1_t
{
    ZXIC_UINT32 fst_swch_eth_head1;
}DPP_NPPU_OAM_CFG_FST_SWCH_ETH_HEAD1_T;

typedef struct dpp_nppu_oam_cfg_fst_swch_eth_head2_t
{
    ZXIC_UINT32 fst_swch_eth_head2;
}DPP_NPPU_OAM_CFG_FST_SWCH_ETH_HEAD2_T;

typedef struct dpp_nppu_oam_cfg_fst_swch_eth_head3_t
{
    ZXIC_UINT32 fst_swch_eth_head3;
}DPP_NPPU_OAM_CFG_FST_SWCH_ETH_HEAD3_T;

typedef struct dpp_nppu_oam_cfg_oam_fs_txinst_fifo_th_t
{
    ZXIC_UINT32 oam_fs_txinst_fifo_th;
}DPP_NPPU_OAM_CFG_OAM_FS_TXINST_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_oam_ma_fs_txinst_fifo_th_t
{
    ZXIC_UINT32 oam_ma_fs_txinst_fifo_th;
}DPP_NPPU_OAM_CFG_OAM_MA_FS_TXINST_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_pon_int_ram_clr_t
{
    ZXIC_UINT32 pon_int_ram_clr;
}DPP_NPPU_OAM_CFG_PON_INT_RAM_CLR_T;

typedef struct dpp_nppu_oam_cfg_pon_p_int_index_t
{
    ZXIC_UINT32 pon_p_int_index;
}DPP_NPPU_OAM_CFG_PON_P_INT_INDEX_T;

typedef struct dpp_nppu_oam_cfg_pon_protect_pkt_fifo_th_t
{
    ZXIC_UINT32 pon_protect_pkt_fifo_th;
}DPP_NPPU_OAM_CFG_PON_PROTECT_PKT_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_pon_laser_off_en_t
{
    ZXIC_UINT32 pon_laser_off_en;
}DPP_NPPU_OAM_CFG_PON_LASER_OFF_EN_T;

typedef struct dpp_nppu_oam_cfg_pon_prtct_pkt_tx_en_t
{
    ZXIC_UINT32 pon_prtct_pkt_tx_en;
}DPP_NPPU_OAM_CFG_PON_PRTCT_PKT_TX_EN_T;

typedef struct dpp_nppu_oam_cfg_cfg_pon_master_t
{
    ZXIC_UINT32 cfg_pon_master;
}DPP_NPPU_OAM_CFG_CFG_PON_MASTER_T;

typedef struct dpp_nppu_oam_cfg_level_mode_t
{
    ZXIC_UINT32 level_mode;
}DPP_NPPU_OAM_CFG_LEVEL_MODE_T;

typedef struct dpp_nppu_oam_cfg_interrupt_en_t
{
    ZXIC_UINT32 interrupt_en;
}DPP_NPPU_OAM_CFG_INTERRUPT_EN_T;

typedef struct dpp_nppu_oam_cfg_pon_laser_on_en_t
{
    ZXIC_UINT32 pon_laser_on_en;
}DPP_NPPU_OAM_CFG_PON_LASER_ON_EN_T;

typedef struct dpp_nppu_oam_cfg_ti_pon_sd_t
{
    ZXIC_UINT32 ti_pon_sd;
}DPP_NPPU_OAM_CFG_TI_PON_SD_T;

typedef struct dpp_nppu_oam_cfg_ti_pon_los_t
{
    ZXIC_UINT32 ti_pon_los;
}DPP_NPPU_OAM_CFG_TI_PON_LOS_T;

typedef struct dpp_nppu_oam_cfg_ind_dat4_t
{
    ZXIC_UINT32 ind_dat4;
}DPP_NPPU_OAM_CFG_IND_DAT4_T;

typedef struct dpp_nppu_oam_cfg_ind_dat5_t
{
    ZXIC_UINT32 ind_dat5;
}DPP_NPPU_OAM_CFG_IND_DAT5_T;

typedef struct dpp_nppu_oam_cfg_ind_dat6_t
{
    ZXIC_UINT32 ind_dat6;
}DPP_NPPU_OAM_CFG_IND_DAT6_T;

typedef struct dpp_nppu_oam_cfg_ind_dat7_t
{
    ZXIC_UINT32 ind_dat7;
}DPP_NPPU_OAM_CFG_IND_DAT7_T;

typedef struct dpp_nppu_oam_cfg_ind_dat8_t
{
    ZXIC_UINT32 ind_dat8;
}DPP_NPPU_OAM_CFG_IND_DAT8_T;

typedef struct dpp_nppu_oam_cfg_ind_dat9_t
{
    ZXIC_UINT32 ind_dat9;
}DPP_NPPU_OAM_CFG_IND_DAT9_T;

typedef struct dpp_nppu_oam_cfg_ind_dat10_t
{
    ZXIC_UINT32 ind_dat10;
}DPP_NPPU_OAM_CFG_IND_DAT10_T;

typedef struct dpp_nppu_oam_cfg_ind_dat11_t
{
    ZXIC_UINT32 ind_dat11;
}DPP_NPPU_OAM_CFG_IND_DAT11_T;

typedef struct dpp_nppu_oam_cfg_ind_dat12_t
{
    ZXIC_UINT32 ind_dat12;
}DPP_NPPU_OAM_CFG_IND_DAT12_T;

typedef struct dpp_nppu_oam_cfg_ind_dat13_t
{
    ZXIC_UINT32 ind_dat13;
}DPP_NPPU_OAM_CFG_IND_DAT13_T;

typedef struct dpp_nppu_oam_cfg_ind_dat14_t
{
    ZXIC_UINT32 ind_dat14;
}DPP_NPPU_OAM_CFG_IND_DAT14_T;

typedef struct dpp_nppu_oam_cfg_ind_dat15_t
{
    ZXIC_UINT32 ind_dat15;
}DPP_NPPU_OAM_CFG_IND_DAT15_T;

typedef struct dpp_nppu_oam_cfg_oam_2544_pkt_fifo_th_t
{
    ZXIC_UINT32 oam_2544_pkt_fifo_th;
}DPP_NPPU_OAM_CFG_OAM_2544_PKT_FIFO_TH_T;

typedef struct dpp_nppu_oam_cfg_txinfo_ram_clr_t
{
    ZXIC_UINT32 txinfo_ram_clr;
}DPP_NPPU_OAM_CFG_TXINFO_RAM_CLR_T;

typedef struct dpp_nppu_oam_cfg_txinfo_ram_init_done_t
{
    ZXIC_UINT32 txinfo_ram_init_done;
}DPP_NPPU_OAM_CFG_TXINFO_RAM_INIT_DONE_T;

typedef struct dpp_nppu_oam_cfg_fifo_status_int_status40_t
{
    ZXIC_UINT32 fifo_status_int_status40;
}DPP_NPPU_OAM_CFG_FIFO_STATUS_INT_STATUS40_T;

typedef struct dpp_nppu_oam_cfg_fifo_status_int_status41_t
{
    ZXIC_UINT32 fifo_status_int_status41;
}DPP_NPPU_OAM_CFG_FIFO_STATUS_INT_STATUS41_T;

typedef struct dpp_nppu_oam_cfg_oam_2544_fun_en_t
{
    ZXIC_UINT32 oam_2544_fun_en;
}DPP_NPPU_OAM_CFG_OAM_2544_FUN_EN_T;

typedef struct dpp_nppu_oam_cfg_oam_2544_stat_clr_t
{
    ZXIC_UINT32 oam_2544_stat_clr;
}DPP_NPPU_OAM_CFG_OAM_2544_STAT_CLR_T;

typedef struct dpp_nppu_oam_cfg_txdis_default_t
{
    ZXIC_UINT32 txdis_default;
}DPP_NPPU_OAM_CFG_TXDIS_DEFAULT_T;

typedef struct dpp_nppu_oam_cfg_txdis_default_en_t
{
    ZXIC_UINT32 txdis_default_en;
}DPP_NPPU_OAM_CFG_TXDIS_DEFAULT_EN_T;

typedef struct dpp_nppu_oam_cfg_tpbfd_firstchk_th_t
{
    ZXIC_UINT32 tpbfd_firstchk_th;
}DPP_NPPU_OAM_CFG_TPBFD_FIRSTCHK_TH_T;

typedef struct dpp_nppu_oam_cfg_ethccm_firstchk_th_t
{
    ZXIC_UINT32 ethccm_firstchk_th;
}DPP_NPPU_OAM_CFG_ETHCCM_FIRSTCHK_TH_T;

typedef struct dpp_nppu_oam_cfg_tpccm_firstchk_th_t
{
    ZXIC_UINT32 tpccm_firstchk_th;
}DPP_NPPU_OAM_CFG_TPCCM_FIRSTCHK_TH_T;

typedef struct dpp_nppu_oam_stat_txstat_req_cnt_t
{
    ZXIC_UINT32 txstat_req_cnt;
}DPP_NPPU_OAM_STAT_TXSTAT_REQ_CNT_T;

typedef struct dpp_nppu_oam_stat_chkstat_req_cnt_t
{
    ZXIC_UINT32 chkstat_req_cnt;
}DPP_NPPU_OAM_STAT_CHKSTAT_REQ_CNT_T;

typedef struct dpp_nppu_oam_stat_stat_oam_fc_cnt_t
{
    ZXIC_UINT32 stat1_oam_fc_cnt;
}DPP_NPPU_OAM_STAT_STAT_OAM_FC_CNT_T;

typedef struct dpp_nppu_oam_stat_bfdseq_req_cnt_t
{
    ZXIC_UINT32 bfdseq_req_cnt;
}DPP_NPPU_OAM_STAT_BFDSEQ_REQ_CNT_T;

typedef struct dpp_nppu_oam_stat_lmcnt_req_cnt_t
{
    ZXIC_UINT32 lmcnt_req_cnt;
}DPP_NPPU_OAM_STAT_LMCNT_REQ_CNT_T;

typedef struct dpp_nppu_oam_stat_stat_oam_lm_rsp_cnt_t
{
    ZXIC_UINT32 stat2_rsp_cnt;
}DPP_NPPU_OAM_STAT_STAT_OAM_LM_RSP_CNT_T;

typedef struct dpp_nppu_oam_stat_stat_oam_lm_fc_cnt_t
{
    ZXIC_UINT32 stat2_oam_fc_cnt;
}DPP_NPPU_OAM_STAT_STAT_OAM_LM_FC_CNT_T;

typedef struct dpp_nppu_oam_stat_se_req_cnt_t
{
    ZXIC_UINT32 se_req_cnt;
}DPP_NPPU_OAM_STAT_SE_REQ_CNT_T;

typedef struct dpp_nppu_oam_stat_se_rsp_cnt_t
{
    ZXIC_UINT32 se_rsp_cnt;
}DPP_NPPU_OAM_STAT_SE_RSP_CNT_T;

typedef struct dpp_nppu_oam_stat_se_oam_fc_cnt_t
{
    ZXIC_UINT32 se_oam_fc_cnt;
}DPP_NPPU_OAM_STAT_SE_OAM_FC_CNT_T;

typedef struct dpp_nppu_oam_stat_oam_se_fc_cnt_t
{
    ZXIC_UINT32 oam_se_fc_cnt;
}DPP_NPPU_OAM_STAT_OAM_SE_FC_CNT_T;

typedef struct dpp_nppu_oam_stat_oam_pktrx_sop_cnt_t
{
    ZXIC_UINT32 oam_pktrx_sop_cnt;
}DPP_NPPU_OAM_STAT_OAM_PKTRX_SOP_CNT_T;

typedef struct dpp_nppu_oam_stat_oam_pktrx_eop_cnt_t
{
    ZXIC_UINT32 oam_pktrx_eop_cnt;
}DPP_NPPU_OAM_STAT_OAM_PKTRX_EOP_CNT_T;

typedef struct dpp_nppu_oam_stat_pktrx_oam_fc_cnt_t
{
    ZXIC_UINT32 pktrx_oam_fc_cnt;
}DPP_NPPU_OAM_STAT_PKTRX_OAM_FC_CNT_T;

typedef struct dpp_nppu_oam_stat_pktrx_oam_tst_fc_cnt_t
{
    ZXIC_UINT32 pktrx_oam_tst_fc_cnt;
}DPP_NPPU_OAM_STAT_PKTRX_OAM_TST_FC_CNT_T;

typedef struct dpp_nppu_oam_stat_odma_oam_sop_cnt_t
{
    ZXIC_UINT32 odma_oam_sop_cnt;
}DPP_NPPU_OAM_STAT_ODMA_OAM_SOP_CNT_T;

typedef struct dpp_nppu_oam_stat_odma_oam_eop_cnt_t
{
    ZXIC_UINT32 odma_oam_eop_cnt;
}DPP_NPPU_OAM_STAT_ODMA_OAM_EOP_CNT_T;

typedef struct dpp_nppu_oam_stat_oam_odma_fc_cnt_t
{
    ZXIC_UINT32 oam_odma_fc_cnt;
}DPP_NPPU_OAM_STAT_OAM_ODMA_FC_CNT_T;

typedef struct dpp_nppu_oam_stat_rec_ma_pkt_illegal_cnt_t
{
    ZXIC_UINT32 rec_ma_pkt_illegal_cnt;
}DPP_NPPU_OAM_STAT_REC_MA_PKT_ILLEGAL_CNT_T;

typedef struct dpp_nppu_oam_stat_rec_rmep_pkt_illegal_cnt_t
{
    ZXIC_UINT32 rec_rmep_pkt_illegal_cnt;
}DPP_NPPU_OAM_STAT_REC_RMEP_PKT_ILLEGAL_CNT_T;

typedef struct dpp_nppu_oam_stat_rec_eth_ais_pkt_cnt_t
{
    ZXIC_UINT32 rec_eth_ais_pkt_cnt;
}DPP_NPPU_OAM_STAT_REC_ETH_AIS_PKT_CNT_T;

typedef struct dpp_nppu_oam_stat_rec_tp_ais_pkt_cnt_t
{
    ZXIC_UINT32 rec_tp_ais_pkt_cnt;
}DPP_NPPU_OAM_STAT_REC_TP_AIS_PKT_CNT_T;

typedef struct dpp_nppu_oam_stat_rec_tp_csf_pkt_cnt_t
{
    ZXIC_UINT32 rec_tp_csf_pkt_cnt;
}DPP_NPPU_OAM_STAT_REC_TP_CSF_PKT_CNT_T;

typedef struct dpp_nppu_oam_stat_rec_eth_level_defect_cnt_t
{
    ZXIC_UINT32 rec_eth_level_defect_cnt;
}DPP_NPPU_OAM_STAT_REC_ETH_LEVEL_DEFECT_CNT_T;

typedef struct dpp_nppu_oam_stat_rec_eth_megid_defect_cnt_t
{
    ZXIC_UINT32 rec_eth_megid_defect_cnt;
}DPP_NPPU_OAM_STAT_REC_ETH_MEGID_DEFECT_CNT_T;

typedef struct dpp_nppu_oam_stat_rec_eth_mepid_defect_cnt_t
{
    ZXIC_UINT32 rec_eth_mepid_defect_cnt;
}DPP_NPPU_OAM_STAT_REC_ETH_MEPID_DEFECT_CNT_T;

typedef struct dpp_nppu_oam_stat_rec_eth_interval_defect_cnt_t
{
    ZXIC_UINT32 rec_eth_interval_defect_cnt;
}DPP_NPPU_OAM_STAT_REC_ETH_INTERVAL_DEFECT_CNT_T;

typedef struct dpp_nppu_oam_stat_rec_sess_unenable_cnt_t
{
    ZXIC_UINT32 rec_sess_unenable_cnt;
}DPP_NPPU_OAM_STAT_REC_SESS_UNENABLE_CNT_T;

typedef struct dpp_nppu_oam_stat_oam_2544_rd_pkt_cnt_t
{
    ZXIC_UINT32 oam_2544_rd_pkt_cnt;
}DPP_NPPU_OAM_STAT_OAM_2544_RD_PKT_CNT_T;

typedef struct dpp_nppu_oam_stat_debug_cnt_clr_t
{
    ZXIC_UINT32 debug_cnt_clr;
}DPP_NPPU_OAM_STAT_DEBUG_CNT_CLR_T;

typedef struct dpp_nppu_oam_stat_oam_pktrx_catch_data_t
{
    ZXIC_UINT32 oam_pktrx_catch_data;
}DPP_NPPU_OAM_STAT_OAM_PKTRX_CATCH_DATA_T;

typedef struct dpp_nppu_oam_stat_odma_oam_catch_data_t
{
    ZXIC_UINT32 odma_oam_catch_data;
}DPP_NPPU_OAM_STAT_ODMA_OAM_CATCH_DATA_T;

typedef struct dpp_nppu_oam_stat_tst_session_tx_cnt_t
{
    ZXIC_UINT32 tst_session_tx_cnt;
}DPP_NPPU_OAM_STAT_TST_SESSION_TX_CNT_T;

typedef struct dpp_nppu_oam_stat_tst_session_rx_cnt_t
{
    ZXIC_UINT32 tst_session_rx_cnt;
}DPP_NPPU_OAM_STAT_TST_SESSION_RX_CNT_T;

typedef struct dpp_nppu_oam_stat_tstrx_lost_cnt_t
{
    ZXIC_UINT32 tstrx_lost_cnt;
}DPP_NPPU_OAM_STAT_TSTRX_LOST_CNT_T;

typedef struct dpp_nppu_oam_stat_bfdseq_wr_cnt_t
{
    ZXIC_UINT32 bfdseq_wr_cnt;
}DPP_NPPU_OAM_STAT_BFDSEQ_WR_CNT_T;

typedef struct dpp_nppu_oam_stat_bfdtime_wr_cnt_t
{
    ZXIC_UINT32 bfdtime_wr_cnt;
}DPP_NPPU_OAM_STAT_BFDTIME_WR_CNT_T;

typedef struct dpp_nppu_oam_stat_lmcnt_wr_cnt_t
{
    ZXIC_UINT32 lmcnt_wr_cnt;
}DPP_NPPU_OAM_STAT_LMCNT_WR_CNT_T;

typedef struct dpp_nppu_oam_stat_oam_fs_pkt_cnt_t
{
    ZXIC_UINT32 oam_fs_pkt_cnt;
}DPP_NPPU_OAM_STAT_OAM_FS_PKT_CNT_T;

typedef struct dpp_nppu_oam_stat_oam_ma_fs_pkt_cnt_t
{
    ZXIC_UINT32 lmcnt_wr_cnt;
}DPP_NPPU_OAM_STAT_OAM_MA_FS_PKT_CNT_T;

typedef struct dpp_nppu_oam_stat_rec_tp_level_defect_cnt_t
{
    ZXIC_UINT32 rec_tp_level_defect_cnt;
}DPP_NPPU_OAM_STAT_REC_TP_LEVEL_DEFECT_CNT_T;

typedef struct dpp_nppu_oam_stat_rec_tp_megid_defect_cnt_t
{
    ZXIC_UINT32 rec_tp_megid_defect_cnt;
}DPP_NPPU_OAM_STAT_REC_TP_MEGID_DEFECT_CNT_T;

typedef struct dpp_nppu_oam_stat_rec_tp_mepid_defect_cnt_t
{
    ZXIC_UINT32 rec_tp_mepid_defect_cnt;
}DPP_NPPU_OAM_STAT_REC_TP_MEPID_DEFECT_CNT_T;

typedef struct dpp_nppu_oam_stat_rec_tp_interval_defect_cnt_t
{
    ZXIC_UINT32 rec_tp_interval_defect_cnt;
}DPP_NPPU_OAM_STAT_REC_TP_INTERVAL_DEFECT_CNT_T;

typedef struct dpp_nppu_oam_stat_rd_reg_clear_mode_t
{
    ZXIC_UINT32 rd_clear_mode_cfg;
}DPP_NPPU_OAM_STAT_RD_REG_CLEAR_MODE_T;

typedef struct dpp_nppu_oam_stat_rd_data_reg_clear_mode_t
{
    ZXIC_UINT32 rd_data_reg_clear_mode_cfg;
}DPP_NPPU_OAM_STAT_RD_DATA_REG_CLEAR_MODE_T;

typedef struct dpp_nppu_oam_cfg_indir_oam_int_status_ram_0_t
{
    ZXIC_UINT32 bfd_diag_value_bit4;
    ZXIC_UINT32 bfd_diag_value_bit3;
    ZXIC_UINT32 bfd_diag_value_bit2;
    ZXIC_UINT32 bfd_diag_value_bit1;
    ZXIC_UINT32 bfd_diag_value_bit0;
    ZXIC_UINT32 dloc_int;
    ZXIC_UINT32 drdi_int;
}DPP_NPPU_OAM_CFG_INDIR_OAM_INT_STATUS_RAM_0_T;

typedef struct dpp_nppu_oam_cfg_indir_oam_int_status_ram1_t
{
    ZXIC_UINT32 sticky_error_level_defect;
    ZXIC_UINT32 sticky_error_megid_defect;
    ZXIC_UINT32 sticky_error_mepid_defect;
    ZXIC_UINT32 sticky_error_inter_defect;
    ZXIC_UINT32 sticky_ais_defect;
    ZXIC_UINT32 sticky_csf_defect;
    ZXIC_UINT32 current_error_level_defect;
    ZXIC_UINT32 current_error_megid_defect;
    ZXIC_UINT32 current_error_mepid_defect;
    ZXIC_UINT32 current_error_inter_defect;
    ZXIC_UINT32 current_ais_defect;
    ZXIC_UINT32 current_csf_defect;
}DPP_NPPU_OAM_CFG_INDIR_OAM_INT_STATUS_RAM1_T;

typedef struct dpp_nppu_oam_cfg_indir_tst_pkt_tx_para_ram_t
{
    ZXIC_UINT32 ddr_self_test_tx_en;
    ZXIC_UINT32 tm_self_test_tx_en;
    ZXIC_UINT32 fast_aging_tx_en;
    ZXIC_UINT32 timing_aging_tx_en;
    ZXIC_UINT32 backgroud_flow_tx_en;
    ZXIC_UINT32 tsttx_session_en;
    ZXIC_UINT32 tx_freq;
    ZXIC_UINT32 tx_offset;
    ZXIC_UINT32 tx_count;
    ZXIC_UINT32 fast_tx_mode_en;
    ZXIC_UINT32 tsttx_pkthead_len;
    ZXIC_UINT32 tsttx_interval;
}DPP_NPPU_OAM_CFG_INDIR_TST_PKT_TX_PARA_RAM_T;

typedef struct dpp_nppu_oam_cfg_indir_groupnumram_t
{
    ZXIC_UINT32 mep_down_num;
}DPP_NPPU_OAM_CFG_INDIR_GROUPNUMRAM_T;

typedef struct dpp_nppu_oam_cfg_indir_oam_tx_tbl_ram_t
{
    ZXIC_UINT32 oam_tx_en;
    ZXIC_UINT32 oam_tx_type;
    ZXIC_UINT32 oam_fetch_len;
    ZXIC_UINT32 bfd_seq_tx_en;
    ZXIC_UINT32 tx_para;
    ZXIC_UINT32 oam_tx_interval;
    ZXIC_UINT32 hd_ena_flag;
    ZXIC_UINT32 last_tx_time;
}DPP_NPPU_OAM_CFG_INDIR_OAM_TX_TBL_RAM_T;

typedef struct dpp_nppu_oam_cfg_indir_oam_chk_tbl_ram_t
{
    ZXIC_UINT32 fast_switch_en;
    ZXIC_UINT32 oam_chk_en;
    ZXIC_UINT32 oam_chk_type;
    ZXIC_UINT32 ccm_predel_flag;
    ZXIC_UINT32 lm_chk_en;
    ZXIC_UINT32 ccm_group_id;
    ZXIC_UINT32 oam_chk_internal;
    ZXIC_UINT32 fist_chk_flag;
    ZXIC_UINT32 last_chk_time;
}DPP_NPPU_OAM_CFG_INDIR_OAM_CHK_TBL_RAM_T;

typedef struct dpp_nppu_oam_cfg_indir_oam_ma_chk_tbl_ram_t
{
    ZXIC_UINT32 ma_fast_switch_en;
    ZXIC_UINT32 ma_chk_en;
    ZXIC_UINT32 ma_type;
    ZXIC_UINT32 error_level_defect_en;
    ZXIC_UINT32 error_megid_defect_en;
    ZXIC_UINT32 error_mepid_defect_en;
    ZXIC_UINT32 error_inter_defect_en;
    ZXIC_UINT32 ais_defect_en;
    ZXIC_UINT32 csf_defect_en;
    ZXIC_UINT32 error_level_defect_ccm;
    ZXIC_UINT32 error_megid_defect_ccm;
    ZXIC_UINT32 error_mepid_defect_ccm;
    ZXIC_UINT32 error_inter_defect_ccm;
    ZXIC_UINT32 ais_defect_ccm;
    ZXIC_UINT32 csf_defect_ccm;
    ZXIC_UINT32 ma_predel_en;
    ZXIC_UINT32 error_level_defect_ts;
    ZXIC_UINT32 error_megid_defect_ts;
    ZXIC_UINT32 error_mepid_defect_ts;
    ZXIC_UINT32 error_inter_defect_ts;
    ZXIC_UINT32 ais_defect_ts;
    ZXIC_UINT32 csf_defect_ts;
}DPP_NPPU_OAM_CFG_INDIR_OAM_MA_CHK_TBL_RAM_T;

typedef struct dpp_nppu_oam_cfg_indir_oam_2544_tx_ram_t
{
    ZXIC_UINT32 tx_en_2544;
    ZXIC_UINT32 tx_cfg_times_2544;
    ZXIC_UINT32 current_times;
    ZXIC_UINT32 slice_num;
    ZXIC_UINT32 pkt_mty;
}DPP_NPPU_OAM_CFG_INDIR_OAM_2544_TX_RAM_T;


#ifdef __cplusplus
}
#endif
#endif

