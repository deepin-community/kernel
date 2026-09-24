
#ifndef _DPP_STAT_REG_H_
#define _DPP_STAT_REG_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef struct dpp_stat_stat_cfg_cpu_ind_eram_wdat0_t
{
    ZXIC_UINT32 cpu_ind_eram_wdat0;
}DPP_STAT_STAT_CFG_CPU_IND_ERAM_WDAT0_T;

typedef struct dpp_stat_stat_cfg_etm_port_sel_cfg_t
{
    ZXIC_UINT32 etm_port0_sel_cfg;
    ZXIC_UINT32 etm_port1_sel_cfg;
    ZXIC_UINT32 etm_port2_sel_cfg;
    ZXIC_UINT32 etm_port3_sel_cfg;
}DPP_STAT_STAT_CFG_ETM_PORT_SEL_CFG_T;

typedef struct dpp_stat_stat_cfg_tm_stat_cfg_t
{
    ZXIC_UINT32 stat_overflow_mode;
    ZXIC_UINT32 tm_stat_mode_cfg;
    ZXIC_UINT32 tm_flow_control_cfg;
}DPP_STAT_STAT_CFG_TM_STAT_CFG_T;

typedef struct dpp_stat_stat_cfg_ppu_eram_depth_t
{
    ZXIC_UINT32 ppu_eram_depth;
}DPP_STAT_STAT_CFG_PPU_ERAM_DEPTH_T;

typedef struct dpp_stat_stat_cfg_ppu_eram_base_addr_t
{
    ZXIC_UINT32 ppu_eram_base_addr;
}DPP_STAT_STAT_CFG_PPU_ERAM_BASE_ADDR_T;

typedef struct dpp_stat_stat_cfg_ppu_ddr_base_addr_t
{
    ZXIC_UINT32 ppu_ddr_base_addr;
}DPP_STAT_STAT_CFG_PPU_DDR_BASE_ADDR_T;

typedef struct dpp_stat_stat_cfg_plcr0_base_addr_t
{
    ZXIC_UINT32 plcr0_base_addr;
}DPP_STAT_STAT_CFG_PLCR0_BASE_ADDR_T;

typedef struct dpp_stat_stat_cfg_etm_stat_start_addr_cfg_t
{
    ZXIC_UINT32 etm_stat_start_addr_cfg;
}DPP_STAT_STAT_CFG_ETM_STAT_START_ADDR_CFG_T;

typedef struct dpp_stat_stat_cfg_etm_stat_depth_cfg_t
{
    ZXIC_UINT32 etm_stat_depth_cfg;
}DPP_STAT_STAT_CFG_ETM_STAT_DEPTH_CFG_T;

typedef struct dpp_stat_stat_cfg_cycle_mov_en_cfg_t
{
    ZXIC_UINT32 cycle_mov_en_cfg;
}DPP_STAT_STAT_CFG_CYCLE_MOV_EN_CFG_T;

typedef struct dpp_stat_etcam_cpu_ind_wdat0_t
{
    ZXIC_UINT32 wdat0;
}DPP_STAT_ETCAM_CPU_IND_WDAT0_T;

typedef struct dpp_stat_etcam_cpu_ind_ctrl_tmp0_t
{
    ZXIC_UINT32 reg_tcam_flag;
    ZXIC_UINT32 flush;
    ZXIC_UINT32 rd_wr;
    ZXIC_UINT32 wr_mode;
    ZXIC_UINT32 dat_or_mask;
    ZXIC_UINT32 ram_sel;
    ZXIC_UINT32 addr;
}DPP_STAT_ETCAM_CPU_IND_CTRL_TMP0_T;

typedef struct dpp_stat_etcam_cpu_ind_ctrl_tmp1_t
{
    ZXIC_UINT32 row_or_col_msk;
    ZXIC_UINT32 vben;
    ZXIC_UINT32 vbit;
}DPP_STAT_ETCAM_CPU_IND_CTRL_TMP1_T;

typedef struct dpp_stat_etcam_cpu_ind_rd_done_t
{
    ZXIC_UINT32 cpu_ind_rd_done;
}DPP_STAT_ETCAM_CPU_IND_RD_DONE_T;

typedef struct dpp_stat_etcam_cpu_rdat0_t
{
    ZXIC_UINT32 cpu_rdat0;
}DPP_STAT_ETCAM_CPU_RDAT0_T;

typedef struct dpp_stat_etcam_cpu_rdat1_t
{
    ZXIC_UINT32 cpu_rdat1;
}DPP_STAT_ETCAM_CPU_RDAT1_T;

typedef struct dpp_stat_etcam_cpu_rdat2_t
{
    ZXIC_UINT32 cpu_rdat2;
}DPP_STAT_ETCAM_CPU_RDAT2_T;

typedef struct dpp_stat_etcam_cpu_rdat3_t
{
    ZXIC_UINT32 cpu_rdat3;
}DPP_STAT_ETCAM_CPU_RDAT3_T;

typedef struct dpp_stat_etcam_cpu_rdat4_t
{
    ZXIC_UINT32 cpu_rdat4;
}DPP_STAT_ETCAM_CPU_RDAT4_T;

typedef struct dpp_stat_etcam_cpu_rdat5_t
{
    ZXIC_UINT32 cpu_rdat5;
}DPP_STAT_ETCAM_CPU_RDAT5_T;

typedef struct dpp_stat_etcam_cpu_rdat6_t
{
    ZXIC_UINT32 cpu_rdat6;
}DPP_STAT_ETCAM_CPU_RDAT6_T;

typedef struct dpp_stat_etcam_cpu_rdat7_t
{
    ZXIC_UINT32 cpu_rdat7;
}DPP_STAT_ETCAM_CPU_RDAT7_T;

typedef struct dpp_stat_etcam_cpu_rdat8_t
{
    ZXIC_UINT32 cpu_rdat8;
}DPP_STAT_ETCAM_CPU_RDAT8_T;

typedef struct dpp_stat_etcam_cpu_rdat9_t
{
    ZXIC_UINT32 cpu_rdat9;
}DPP_STAT_ETCAM_CPU_RDAT9_T;

typedef struct dpp_stat_etcam_cpu_rdat10_t
{
    ZXIC_UINT32 cpu_rdat10;
}DPP_STAT_ETCAM_CPU_RDAT10_T;

typedef struct dpp_stat_etcam_cpu_rdat11_t
{
    ZXIC_UINT32 cpu_rdat11;
}DPP_STAT_ETCAM_CPU_RDAT11_T;

typedef struct dpp_stat_etcam_cpu_rdat12_t
{
    ZXIC_UINT32 cpu_rdat12;
}DPP_STAT_ETCAM_CPU_RDAT12_T;

typedef struct dpp_stat_etcam_cpu_rdat13_t
{
    ZXIC_UINT32 cpu_rdat13;
}DPP_STAT_ETCAM_CPU_RDAT13_T;

typedef struct dpp_stat_etcam_cpu_rdat14_t
{
    ZXIC_UINT32 cpu_rdat14;
}DPP_STAT_ETCAM_CPU_RDAT14_T;

typedef struct dpp_stat_etcam_cpu_rdat15_t
{
    ZXIC_UINT32 cpu_rdat15;
}DPP_STAT_ETCAM_CPU_RDAT15_T;

typedef struct dpp_stat_etcam_cpu_rdat16_t
{
    ZXIC_UINT32 cpu_rdat16;
}DPP_STAT_ETCAM_CPU_RDAT16_T;

typedef struct dpp_stat_etcam_cpu_rdat17_t
{
    ZXIC_UINT32 cpu_rdat17;
}DPP_STAT_ETCAM_CPU_RDAT17_T;

typedef struct dpp_stat_etcam_cpu_rdat18_t
{
    ZXIC_UINT32 cpu_rdat18;
}DPP_STAT_ETCAM_CPU_RDAT18_T;

typedef struct dpp_stat_etcam_cpu_rdat19_t
{
    ZXIC_UINT32 cpu_rdat19;
}DPP_STAT_ETCAM_CPU_RDAT19_T;

typedef struct dpp_stat_etcam_qvbo_t
{
    ZXIC_UINT32 qvbo;
}DPP_STAT_ETCAM_QVBO_T;

typedef struct dpp_stat_etcam_cnt_overflow_mode_t
{
    ZXIC_UINT32 cnt_rd_mode;
    ZXIC_UINT32 cnt_overflow_mode;
}DPP_STAT_ETCAM_CNT_OVERFLOW_MODE_T;

typedef struct dpp_stat_car0_cara_queue_ram0_159_0_t
{
    ZXIC_UINT32 cara_drop;
    ZXIC_UINT32 cara_plcr_en;
    ZXIC_UINT32 cara_profile_id;
    ZXIC_UINT32 cara_tq_h;
    ZXIC_UINT32 cara_tq_l;
    ZXIC_UINT32 cara_ted;
    ZXIC_UINT32 cara_tcd;
    ZXIC_UINT32 cara_tei;
    ZXIC_UINT32 cara_tci;
}DPP_STAT_CAR0_CARA_QUEUE_RAM0_159_0_T;

typedef struct dpp_stat_car0_cara_profile_ram1_255_0_t
{
    ZXIC_UINT32 cara_profile_wr;
    ZXIC_UINT32 cara_pkt_sign;
    ZXIC_UINT32 cara_cd;
    ZXIC_UINT32 cara_cf;
    ZXIC_UINT32 cara_cm;
    ZXIC_UINT32 cara_eir;
    ZXIC_UINT32 cara_cir;
    ZXIC_UINT32 cara_ebs_pbs;
    ZXIC_UINT32 cara_cbs;
    ZXIC_UINT32 cara_c_pri1;
    ZXIC_UINT32 cara_c_pri2;
    ZXIC_UINT32 cara_c_pri3;
    ZXIC_UINT32 cara_c_pri4;
    ZXIC_UINT32 cara_c_pri5;
    ZXIC_UINT32 cara_c_pri6;
    ZXIC_UINT32 cara_c_pri7;
    ZXIC_UINT32 cara_e_g_pri1;
    ZXIC_UINT32 cara_e_g_pri2;
    ZXIC_UINT32 cara_e_g_pri3;
    ZXIC_UINT32 cara_e_g_pri4;
    ZXIC_UINT32 cara_e_g_pri5;
    ZXIC_UINT32 cara_e_g_pri6;
    ZXIC_UINT32 cara_e_g_pri7;
    ZXIC_UINT32 cara_e_y_pri0;
    ZXIC_UINT32 cara_e_y_pri1;
    ZXIC_UINT32 cara_e_y_pri2;
    ZXIC_UINT32 cara_e_y_pri3;
    ZXIC_UINT32 cara_e_y_pri4;
    ZXIC_UINT32 cara_e_y_pri5;
    ZXIC_UINT32 cara_e_y_pri6;
    ZXIC_UINT32 cara_e_y_pri7;
}DPP_STAT_CAR0_CARA_PROFILE_RAM1_255_0_T;

typedef struct dpp_stat_car0_cara_qovs_ram_ram2_t
{
    ZXIC_UINT32 cara_qovs;
}DPP_STAT_CAR0_CARA_QOVS_RAM_RAM2_T;

typedef struct dpp_stat_car0_look_up_table1_t
{
    ZXIC_UINT32 cara_flow_id;
    ZXIC_UINT32 cara_sp;
}DPP_STAT_CAR0_LOOK_UP_TABLE1_T;

typedef struct dpp_stat_car0_cara_pkt_des_i_cnt_t
{
    ZXIC_UINT32 cara_pkt_des_i_cnt;
}DPP_STAT_CAR0_CARA_PKT_DES_I_CNT_T;

typedef struct dpp_stat_car0_cara_green_pkt_i_cnt_t
{
    ZXIC_UINT32 cara_green_pkt_i_cnt;
}DPP_STAT_CAR0_CARA_GREEN_PKT_I_CNT_T;

typedef struct dpp_stat_car0_cara_yellow_pkt_i_cnt_t
{
    ZXIC_UINT32 cara_yellow_pkt_i_cnt;
}DPP_STAT_CAR0_CARA_YELLOW_PKT_I_CNT_T;

typedef struct dpp_stat_car0_cara_red_pkt_i_cnt_t
{
    ZXIC_UINT32 cara_red_pkt_i_cnt;
}DPP_STAT_CAR0_CARA_RED_PKT_I_CNT_T;

typedef struct dpp_stat_car0_cara_pkt_des_o_cnt_t
{
    ZXIC_UINT32 cara_pkt_des_o_cnt;
}DPP_STAT_CAR0_CARA_PKT_DES_O_CNT_T;

typedef struct dpp_stat_car0_cara_green_pkt_o_cnt_t
{
    ZXIC_UINT32 cara_green_pkt_o_cnt;
}DPP_STAT_CAR0_CARA_GREEN_PKT_O_CNT_T;

typedef struct dpp_stat_car0_cara_yellow_pkt_o_cnt_t
{
    ZXIC_UINT32 cara_yellow_pkt_o_cnt;
}DPP_STAT_CAR0_CARA_YELLOW_PKT_O_CNT_T;

typedef struct dpp_stat_car0_cara_red_pkt_o_cnt_t
{
    ZXIC_UINT32 cara_red_pkt_o_cnt;
}DPP_STAT_CAR0_CARA_RED_PKT_O_CNT_T;

typedef struct dpp_stat_car0_cara_pkt_des_fc_for_cfg_cnt_t
{
    ZXIC_UINT32 cara_pkt_des_fc_for_cfg_cnt;
}DPP_STAT_CAR0_CARA_PKT_DES_FC_FOR_CFG_CNT_T;

typedef struct dpp_stat_car0_cara_appoint_qnum_or_sp_t
{
    ZXIC_UINT32 cara_appoint_qnum_or_not;
    ZXIC_UINT32 cara_appoint_sp_or_not;
    ZXIC_UINT32 cara_plcr_stat_sp;
    ZXIC_UINT32 cara_plcr_stat_qnum;
}DPP_STAT_CAR0_CARA_APPOINT_QNUM_OR_SP_T;

typedef struct dpp_stat_car0_cara_cfgmt_count_mode_t
{
    ZXIC_UINT32 cara_cfgmt_count_overflow_mode;
    ZXIC_UINT32 cara_cfgmt_count_rd_mode;
}DPP_STAT_CAR0_CARA_CFGMT_COUNT_MODE_T;

typedef struct dpp_stat_car0_cara_pkt_size_cnt_t
{
    ZXIC_UINT32 cara_pkt_size_cnt;
}DPP_STAT_CAR0_CARA_PKT_SIZE_CNT_T;

typedef struct dpp_stat_car0_cara_plcr_init_dont_t
{
    ZXIC_UINT32 cara_plcr_init_done;
}DPP_STAT_CAR0_CARA_PLCR_INIT_DONT_T;

typedef struct dpp_stat_car0_carb_queue_ram0_159_0_t
{
    ZXIC_UINT32 carb_drop;
    ZXIC_UINT32 carb_plcr_en;
    ZXIC_UINT32 carb_profile_id;
    ZXIC_UINT32 carb_tq_h;
    ZXIC_UINT32 carb_tq_l;
    ZXIC_UINT32 carb_ted;
    ZXIC_UINT32 carb_tcd;
    ZXIC_UINT32 carb_tei;
    ZXIC_UINT32 carb_tci;
}DPP_STAT_CAR0_CARB_QUEUE_RAM0_159_0_T;

typedef struct dpp_stat_car0_carb_profile_ram1_255_0_t
{
    ZXIC_UINT32 carb_profile_wr;
    ZXIC_UINT32 carb_random_discard_en_e;
    ZXIC_UINT32 carb_random_discard_en_c;
    ZXIC_UINT32 carb_pkt_sign;
    ZXIC_UINT32 carb_cd;
    ZXIC_UINT32 carb_cf;
    ZXIC_UINT32 carb_cm;
    ZXIC_UINT32 carb_eir;
    ZXIC_UINT32 carb_cir;
    ZXIC_UINT32 carb_ebs_pbs;
    ZXIC_UINT32 carb_cbs;
    ZXIC_UINT32 carb_c_pri1;
    ZXIC_UINT32 carb_c_pri2;
    ZXIC_UINT32 carb_c_pri3;
    ZXIC_UINT32 carb_c_pri4;
    ZXIC_UINT32 carb_c_pri5;
    ZXIC_UINT32 carb_c_pri6;
    ZXIC_UINT32 carb_c_pri7;
    ZXIC_UINT32 carb_e_g_pri1;
    ZXIC_UINT32 carb_e_g_pri2;
    ZXIC_UINT32 carb_e_g_pri3;
    ZXIC_UINT32 carb_e_g_pri4;
    ZXIC_UINT32 carb_e_g_pri5;
    ZXIC_UINT32 carb_e_g_pri6;
    ZXIC_UINT32 carb_e_g_pri7;
    ZXIC_UINT32 carb_e_y_pri0;
    ZXIC_UINT32 carb_e_y_pri1;
    ZXIC_UINT32 carb_e_y_pri2;
    ZXIC_UINT32 carb_e_y_pri3;
    ZXIC_UINT32 carb_e_y_pri4;
    ZXIC_UINT32 carb_e_y_pri5;
    ZXIC_UINT32 carb_e_y_pri6;
    ZXIC_UINT32 carb_e_y_pri7;
}DPP_STAT_CAR0_CARB_PROFILE_RAM1_255_0_T;

typedef struct dpp_stat_car0_carb_qovs_ram_ram2_t
{
    ZXIC_UINT32 carb_qovs;
}DPP_STAT_CAR0_CARB_QOVS_RAM_RAM2_T;

typedef struct dpp_stat_car0_look_up_table2_t
{
    ZXIC_UINT32 carb_flow_id;
    ZXIC_UINT32 carb_sp;
}DPP_STAT_CAR0_LOOK_UP_TABLE2_T;

typedef struct dpp_stat_car0_carb_pkt_des_i_cnt_t
{
    ZXIC_UINT32 carb_pkt_des_i_cnt;
}DPP_STAT_CAR0_CARB_PKT_DES_I_CNT_T;

typedef struct dpp_stat_car0_carb_green_pkt_i_cnt_t
{
    ZXIC_UINT32 carb_green_pkt_i_cnt;
}DPP_STAT_CAR0_CARB_GREEN_PKT_I_CNT_T;

typedef struct dpp_stat_car0_carb_yellow_pkt_i_cnt_t
{
    ZXIC_UINT32 carb_yellow_pkt_i_cnt;
}DPP_STAT_CAR0_CARB_YELLOW_PKT_I_CNT_T;

typedef struct dpp_stat_car0_carb_red_pkt_i_cnt_t
{
    ZXIC_UINT32 carb_red_pkt_i_cnt;
}DPP_STAT_CAR0_CARB_RED_PKT_I_CNT_T;

typedef struct dpp_stat_car0_carb_pkt_des_o_cnt_t
{
    ZXIC_UINT32 carb_pkt_des_o_cnt;
}DPP_STAT_CAR0_CARB_PKT_DES_O_CNT_T;

typedef struct dpp_stat_car0_carb_green_pkt_o_cnt_t
{
    ZXIC_UINT32 carb_green_pkt_o_cnt;
}DPP_STAT_CAR0_CARB_GREEN_PKT_O_CNT_T;

typedef struct dpp_stat_car0_carb_yellow_pkt_o_cnt_t
{
    ZXIC_UINT32 carb_yellow_pkt_o_cnt;
}DPP_STAT_CAR0_CARB_YELLOW_PKT_O_CNT_T;

typedef struct dpp_stat_car0_carb_red_pkt_o_cnt_t
{
    ZXIC_UINT32 carb_red_pkt_o_cnt;
}DPP_STAT_CAR0_CARB_RED_PKT_O_CNT_T;

typedef struct dpp_stat_car0_carb_pkt_des_fc_for_cfg_cnt_t
{
    ZXIC_UINT32 carb_pkt_des_fc_for_cfg_cnt;
}DPP_STAT_CAR0_CARB_PKT_DES_FC_FOR_CFG_CNT_T;

typedef struct dpp_stat_car0_carb_appoint_qnum_or_sp_t
{
    ZXIC_UINT32 carb_appoint_qnum_or_not;
    ZXIC_UINT32 carb_appoint_sp_or_not;
    ZXIC_UINT32 carb_plcr_stat_sp;
    ZXIC_UINT32 carb_plcr_stat_qnum;
}DPP_STAT_CAR0_CARB_APPOINT_QNUM_OR_SP_T;

typedef struct dpp_stat_car0_carb_cfgmt_count_mode_t
{
    ZXIC_UINT32 carb_cfgmt_count_overflow_mode;
    ZXIC_UINT32 carb_cfgmt_count_rd_mode;
}DPP_STAT_CAR0_CARB_CFGMT_COUNT_MODE_T;

typedef struct dpp_stat_car0_carb_pkt_size_cnt_t
{
    ZXIC_UINT32 carb_pkt_size_cnt;
}DPP_STAT_CAR0_CARB_PKT_SIZE_CNT_T;

typedef struct dpp_stat_car0_carb_plcr_init_dont_t
{
    ZXIC_UINT32 carb_plcr_init_done;
}DPP_STAT_CAR0_CARB_PLCR_INIT_DONT_T;

typedef struct dpp_stat_car0_carc_queue_ram0_159_0_t
{
    ZXIC_UINT32 carc_drop;
    ZXIC_UINT32 carc_plcr_en;
    ZXIC_UINT32 carc_profile_id;
    ZXIC_UINT32 carc_tq_h;
    ZXIC_UINT32 carc_tq_l;
    ZXIC_UINT32 carc_ted;
    ZXIC_UINT32 carc_tcd;
    ZXIC_UINT32 carc_tei;
    ZXIC_UINT32 carc_tci;
}DPP_STAT_CAR0_CARC_QUEUE_RAM0_159_0_T;

typedef struct dpp_stat_car0_carc_profile_ram1_255_0_t
{
    ZXIC_UINT32 carc_profile_wr;
    ZXIC_UINT32 carc_random_discard_en_e;
    ZXIC_UINT32 carc_random_discard_en_c;
    ZXIC_UINT32 carc_pkt_sign;
    ZXIC_UINT32 carc_cd;
    ZXIC_UINT32 carc_cf;
    ZXIC_UINT32 carc_cm;
    ZXIC_UINT32 carc_eir;
    ZXIC_UINT32 carc_cir;
    ZXIC_UINT32 carc_ebs_pbs;
    ZXIC_UINT32 carc_cbs;
    ZXIC_UINT32 carc_c_pri1;
    ZXIC_UINT32 carc_c_pri2;
    ZXIC_UINT32 carc_c_pri3;
    ZXIC_UINT32 carc_c_pri4;
    ZXIC_UINT32 carc_c_pri5;
    ZXIC_UINT32 carc_c_pri6;
    ZXIC_UINT32 carc_c_pri7;
    ZXIC_UINT32 carc_e_g_pri1;
    ZXIC_UINT32 carc_e_g_pri2;
    ZXIC_UINT32 carc_e_g_pri3;
    ZXIC_UINT32 carc_e_g_pri4;
    ZXIC_UINT32 carc_e_g_pri5;
    ZXIC_UINT32 carc_e_g_pri6;
    ZXIC_UINT32 carc_e_g_pri7;
    ZXIC_UINT32 carc_e_y_pri0;
    ZXIC_UINT32 carc_e_y_pri1;
    ZXIC_UINT32 carc_e_y_pri2;
    ZXIC_UINT32 carc_e_y_pri3;
    ZXIC_UINT32 carc_e_y_pri4;
    ZXIC_UINT32 carc_e_y_pri5;
    ZXIC_UINT32 carc_e_y_pri6;
    ZXIC_UINT32 carc_e_y_pri7;
}DPP_STAT_CAR0_CARC_PROFILE_RAM1_255_0_T;

typedef struct dpp_stat_car0_carc_qovs_ram_ram2_t
{
    ZXIC_UINT32 carc_qovs;
}DPP_STAT_CAR0_CARC_QOVS_RAM_RAM2_T;

typedef struct dpp_stat_car0_carc_pkt_des_i_cnt_t
{
    ZXIC_UINT32 carc_pkt_des_i_cnt;
}DPP_STAT_CAR0_CARC_PKT_DES_I_CNT_T;

typedef struct dpp_stat_car0_carc_green_pkt_i_cnt_t
{
    ZXIC_UINT32 carc_green_pkt_i_cnt;
}DPP_STAT_CAR0_CARC_GREEN_PKT_I_CNT_T;

typedef struct dpp_stat_car0_carc_yellow_pkt_i_cnt_t
{
    ZXIC_UINT32 carc_yellow_pkt_i_cnt;
}DPP_STAT_CAR0_CARC_YELLOW_PKT_I_CNT_T;

typedef struct dpp_stat_car0_carc_red_pkt_i_cnt_t
{
    ZXIC_UINT32 carc_red_pkt_i_cnt;
}DPP_STAT_CAR0_CARC_RED_PKT_I_CNT_T;

typedef struct dpp_stat_car0_carc_pkt_des_o_cnt_t
{
    ZXIC_UINT32 carc_pkt_des_o_cnt;
}DPP_STAT_CAR0_CARC_PKT_DES_O_CNT_T;

typedef struct dpp_stat_car0_carc_green_pkt_o_cnt_t
{
    ZXIC_UINT32 carc_green_pkt_o_cnt;
}DPP_STAT_CAR0_CARC_GREEN_PKT_O_CNT_T;

typedef struct dpp_stat_car0_carc_yellow_pkt_o_cnt_t
{
    ZXIC_UINT32 carc_yellow_pkt_o_cnt;
}DPP_STAT_CAR0_CARC_YELLOW_PKT_O_CNT_T;

typedef struct dpp_stat_car0_carc_red_pkt_o_cnt_t
{
    ZXIC_UINT32 carc_red_pkt_o_cnt;
}DPP_STAT_CAR0_CARC_RED_PKT_O_CNT_T;

typedef struct dpp_stat_car0_carc_pkt_des_fc_for_cfg_cnt_t
{
    ZXIC_UINT32 carc_pkt_des_fc_for_cfg_cnt;
}DPP_STAT_CAR0_CARC_PKT_DES_FC_FOR_CFG_CNT_T;

typedef struct dpp_stat_car0_carc_appoint_qnum_or_sp_t
{
    ZXIC_UINT32 carc_appoint_qnum_or_not;
    ZXIC_UINT32 carc_appoint_sp_or_not;
    ZXIC_UINT32 carc_plcr_stat_sp;
    ZXIC_UINT32 carc_plcr_stat_qnum;
}DPP_STAT_CAR0_CARC_APPOINT_QNUM_OR_SP_T;

typedef struct dpp_stat_car0_carc_cfgmt_count_mode_t
{
    ZXIC_UINT32 carc_cfgmt_count_overflow_mode;
    ZXIC_UINT32 carc_cfgmt_count_rd_mode;
}DPP_STAT_CAR0_CARC_CFGMT_COUNT_MODE_T;

typedef struct dpp_stat_car0_carc_pkt_size_cnt_t
{
    ZXIC_UINT32 carc_pkt_size_cnt;
}DPP_STAT_CAR0_CARC_PKT_SIZE_CNT_T;

typedef struct dpp_stat_car0_carc_plcr_init_dont_t
{
    ZXIC_UINT32 carc_plcr_init_done;
}DPP_STAT_CAR0_CARC_PLCR_INIT_DONT_T;

typedef struct dpp_stat_car0_carb_random_ram_t
{
    ZXIC_UINT32 para8_e;
    ZXIC_UINT32 para7_e;
    ZXIC_UINT32 para6_e;
    ZXIC_UINT32 para5_e;
    ZXIC_UINT32 para4_h_e;
    ZXIC_UINT32 para4_l_e;
    ZXIC_UINT32 para3_e;
    ZXIC_UINT32 para2_h_e;
    ZXIC_UINT32 para2_l_e;
    ZXIC_UINT32 para1_e;
    ZXIC_UINT32 para0_h_e;
    ZXIC_UINT32 para0_l_e;
    ZXIC_UINT32 para8_c;
    ZXIC_UINT32 para7_c;
    ZXIC_UINT32 para6_c;
    ZXIC_UINT32 para5_c;
    ZXIC_UINT32 para4_h_c;
    ZXIC_UINT32 para4_l_c;
    ZXIC_UINT32 para3_c;
    ZXIC_UINT32 para2_h_c;
    ZXIC_UINT32 para2_l_c;
    ZXIC_UINT32 para1_c;
    ZXIC_UINT32 para0_h_c;
    ZXIC_UINT32 para0_l_c;
}DPP_STAT_CAR0_CARB_RANDOM_RAM_T;

typedef struct dpp_stat_car0_carc_random_ram_t
{
    ZXIC_UINT32 para8_e;
    ZXIC_UINT32 para7_e;
    ZXIC_UINT32 para6_e;
    ZXIC_UINT32 para5_e;
    ZXIC_UINT32 para4_h_e;
    ZXIC_UINT32 para4_l_e;
    ZXIC_UINT32 para3_e;
    ZXIC_UINT32 para2_h_e;
    ZXIC_UINT32 para2_l_e;
    ZXIC_UINT32 para1_e;
    ZXIC_UINT32 para0_h_e;
    ZXIC_UINT32 para0_l_e;
    ZXIC_UINT32 para8_c;
    ZXIC_UINT32 para7_c;
    ZXIC_UINT32 para6_c;
    ZXIC_UINT32 para5_c;
    ZXIC_UINT32 para4_h_c;
    ZXIC_UINT32 para4_l_c;
    ZXIC_UINT32 para3_c;
    ZXIC_UINT32 para2_h_c;
    ZXIC_UINT32 para2_l_c;
    ZXIC_UINT32 para1_c;
    ZXIC_UINT32 para0_h_c;
    ZXIC_UINT32 para0_l_c;
}DPP_STAT_CAR0_CARC_RANDOM_RAM_T;

typedef struct dpp_stat_car0_cara_begin_flow_id_t
{
    ZXIC_UINT32 cara_begin_flow_id;
}DPP_STAT_CAR0_CARA_BEGIN_FLOW_ID_T;

typedef struct dpp_stat_car0_carb_begin_flow_id_t
{
    ZXIC_UINT32 carb_begin_flow_id;
}DPP_STAT_CAR0_CARB_BEGIN_FLOW_ID_T;

typedef struct dpp_stat_car0_carc_begin_flow_id_t
{
    ZXIC_UINT32 carc_begin_flow_id;
}DPP_STAT_CAR0_CARC_BEGIN_FLOW_ID_T;

typedef struct dpp_stat_car0_prog_full_assert_cfg_w_t
{
    ZXIC_UINT32 prog_full_assert_cfg_w;
}DPP_STAT_CAR0_PROG_FULL_ASSERT_CFG_W_T;

typedef struct dpp_stat_car0_prog_full_negate_cfg_w_t
{
    ZXIC_UINT32 prog_full_negate_cfg_w;
}DPP_STAT_CAR0_PROG_FULL_NEGATE_CFG_W_T;

typedef struct dpp_stat_car0_timeout_limit_t
{
    ZXIC_UINT32 timeout_limit;
}DPP_STAT_CAR0_TIMEOUT_LIMIT_T;

typedef struct dpp_stat_car0_pkt_des_fifo_overflow_t
{
    ZXIC_UINT32 pkt_des_fifo_overflow;
}DPP_STAT_CAR0_PKT_DES_FIFO_OVERFLOW_T;

typedef struct dpp_stat_car0_pkt_des_fifo_underflow_t
{
    ZXIC_UINT32 pkt_des_fifo_underflow;
}DPP_STAT_CAR0_PKT_DES_FIFO_UNDERFLOW_T;

typedef struct dpp_stat_car0_pkt_des_fifo_prog_full_t
{
    ZXIC_UINT32 pkt_des_fifo_prog_full;
}DPP_STAT_CAR0_PKT_DES_FIFO_PROG_FULL_T;

typedef struct dpp_stat_car0_pkt_des_fifo_prog_empty_t
{
    ZXIC_UINT32 pkt_des_fifo_prog_empty;
}DPP_STAT_CAR0_PKT_DES_FIFO_PROG_EMPTY_T;

typedef struct dpp_stat_car0_pkt_des_fifo_full_t
{
    ZXIC_UINT32 pkt_des_fifo_full;
}DPP_STAT_CAR0_PKT_DES_FIFO_FULL_T;

typedef struct dpp_stat_car0_pkt_des_fifo_empty_t
{
    ZXIC_UINT32 pkt_des_fifo_empty;
}DPP_STAT_CAR0_PKT_DES_FIFO_EMPTY_T;

typedef struct dpp_stat_car0_pkt_size_offset_t
{
    ZXIC_UINT32 pkt_size_offset;
}DPP_STAT_CAR0_PKT_SIZE_OFFSET_T;

typedef struct dpp_stat_car0_car_plcr_init_dont_t
{
    ZXIC_UINT32 plcr_init_done;
}DPP_STAT_CAR0_CAR_PLCR_INIT_DONT_T;

typedef struct dpp_stat_car0_max_pkt_size_a_t
{
    ZXIC_UINT32 max_pkt_size_a;
}DPP_STAT_CAR0_MAX_PKT_SIZE_A_T;

typedef struct dpp_stat_car0_max_pkt_size_b_t
{
    ZXIC_UINT32 max_pkt_size_b;
}DPP_STAT_CAR0_MAX_PKT_SIZE_B_T;

typedef struct dpp_stat_car0_max_pkt_size_c_t
{
    ZXIC_UINT32 max_pkt_size_c;
}DPP_STAT_CAR0_MAX_PKT_SIZE_C_T;

typedef struct dpp_stat_car0_car_hierarchy_mode_t
{
    ZXIC_UINT32 car_hierarchy_mode;
}DPP_STAT_CAR0_CAR_HIERARCHY_MODE_T;

typedef struct dpp_stat_car0_prog_empty_assert_cfg_w_t
{
    ZXIC_UINT32 prog_empty_assert_cfg_w;
}DPP_STAT_CAR0_PROG_EMPTY_ASSERT_CFG_W_T;

typedef struct dpp_stat_car0_prog_empty_negate_cfg_w_t
{
    ZXIC_UINT32 prog_empty_negate_cfg_w;
}DPP_STAT_CAR0_PROG_EMPTY_NEGATE_CFG_W_T;

typedef struct dpp_stat_car0_pkt_des_fifo_ovf_int_t
{
    ZXIC_UINT32 pkt_des_fifo_ovf_int;
}DPP_STAT_CAR0_PKT_DES_FIFO_OVF_INT_T;

typedef struct dpp_stat_car0_pkt_des_fifo_data_count_t
{
    ZXIC_UINT32 pkt_des_fifo_data_count;
}DPP_STAT_CAR0_PKT_DES_FIFO_DATA_COUNT_T;

typedef struct dpp_stat_car0_pkt_des_fifo_udf_int_t
{
    ZXIC_UINT32 pkt_des_fifo_udf_int;
}DPP_STAT_CAR0_PKT_DES_FIFO_UDF_INT_T;

typedef struct dpp_stat_car0_cara_queue_ram0_159_0_pkt_t
{
    ZXIC_UINT32 cara_drop;
    ZXIC_UINT32 cara_plcr_en;
    ZXIC_UINT32 cara_profile_id;
    ZXIC_UINT32 cara_tq_h;
    ZXIC_UINT32 cara_tq_l;
    ZXIC_UINT32 cara_dc_high;
    ZXIC_UINT32 cara_dc_low;
    ZXIC_UINT32 cara_tc;
}DPP_STAT_CAR0_CARA_QUEUE_RAM0_159_0_PKT_T;

typedef struct dpp_stat_car0_cara_profile_ram1_255_0_pkt_t
{
    ZXIC_UINT32 cara_profile_wr;
    ZXIC_UINT32 cara_pkt_sign;
    ZXIC_UINT32 cara_pkt_cir;
    ZXIC_UINT32 cara_pkt_cbs;
    ZXIC_UINT32 cara_pri0;
    ZXIC_UINT32 cara_pri1;
    ZXIC_UINT32 cara_pri2;
    ZXIC_UINT32 cara_pri3;
    ZXIC_UINT32 cara_pri4;
    ZXIC_UINT32 cara_pri5;
    ZXIC_UINT32 cara_pri6;
    ZXIC_UINT32 cara_pri7;
}DPP_STAT_CAR0_CARA_PROFILE_RAM1_255_0_PKT_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_eram_wdat1_t
{
    ZXIC_UINT32 cpu_ind_eram_wdat1;
}DPP_STAT_STAT_CFG_CPU_IND_ERAM_WDAT1_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_eram_wdat2_t
{
    ZXIC_UINT32 cpu_ind_eram_wdat2;
}DPP_STAT_STAT_CFG_CPU_IND_ERAM_WDAT2_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_eram_wdat3_t
{
    ZXIC_UINT32 cpu_ind_eram_wdat3;
}DPP_STAT_STAT_CFG_CPU_IND_ERAM_WDAT3_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_eram_req_info_t
{
    ZXIC_UINT32 rw_mode;
    ZXIC_UINT32 read_mode;
    ZXIC_UINT32 tm_cs;
    ZXIC_UINT32 queue_cs;
    ZXIC_UINT32 rw_addr;
}DPP_STAT_STAT_CFG_CPU_IND_ERAM_REQ_INFO_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_eram_rd_done_t
{
    ZXIC_UINT32 cpu_ind_eram_rd_done;
}DPP_STAT_STAT_CFG_CPU_IND_ERAM_RD_DONE_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_eram_rdat0_t
{
    ZXIC_UINT32 cpu_ind_eram_rdat0;
}DPP_STAT_STAT_CFG_CPU_IND_ERAM_RDAT0_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_eram_rdat1_t
{
    ZXIC_UINT32 cpu_ind_eram_rdat1;
}DPP_STAT_STAT_CFG_CPU_IND_ERAM_RDAT1_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_eram_rdat2_t
{
    ZXIC_UINT32 cpu_ind_eram_rdat2;
}DPP_STAT_STAT_CFG_CPU_IND_ERAM_RDAT2_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_eram_rdat3_t
{
    ZXIC_UINT32 cpu_ind_eram_rdat3;
}DPP_STAT_STAT_CFG_CPU_IND_ERAM_RDAT3_T;

typedef struct dpp_stat_stat_cfg_tm_alu_eram_cpu_rdy_t
{
    ZXIC_UINT32 tm_alu_eram_cpu_rdy;
}DPP_STAT_STAT_CFG_TM_ALU_ERAM_CPU_RDY_T;

typedef struct dpp_stat_stat_cfg_oam_stat_cfg_t
{
    ZXIC_UINT32 oam_flow_control_cfg;
    ZXIC_UINT32 oam_lm_flow_control_cfg;
    ZXIC_UINT32 oam_in_eram_cfg;
}DPP_STAT_STAT_CFG_OAM_STAT_CFG_T;

typedef struct dpp_stat_stat_cfg_ftm_port_sel_cfg_t
{
    ZXIC_UINT32 ftm_port0_sel_cfg;
    ZXIC_UINT32 ftm_port1_sel_cfg;
    ZXIC_UINT32 ftm_port2_sel_cfg;
    ZXIC_UINT32 ftm_port3_sel_cfg;
}DPP_STAT_STAT_CFG_FTM_PORT_SEL_CFG_T;

typedef struct dpp_stat_stat_cfg_oam_eram_base_addr_t
{
    ZXIC_UINT32 oam_eram_base_addr;
}DPP_STAT_STAT_CFG_OAM_ERAM_BASE_ADDR_T;

typedef struct dpp_stat_stat_cfg_oam_lm_eram_base_addr_t
{
    ZXIC_UINT32 oam_lm_eram_base_addr;
}DPP_STAT_STAT_CFG_OAM_LM_ERAM_BASE_ADDR_T;

typedef struct dpp_stat_stat_cfg_oam_ddr_base_addr_t
{
    ZXIC_UINT32 oam_ddr_base_addr;
}DPP_STAT_STAT_CFG_OAM_DDR_BASE_ADDR_T;

typedef struct dpp_stat_stat_cfg_plcr0_schd_pful_cfg_t
{
    ZXIC_UINT32 plcr0_schd_pful_assert;
    ZXIC_UINT32 plcr0_schd_pful_negate;
}DPP_STAT_STAT_CFG_PLCR0_SCHD_PFUL_CFG_T;

typedef struct dpp_stat_stat_cfg_oam_lm_ord_pful_cfg_t
{
    ZXIC_UINT32 oam_lm_ord_pful_assert;
    ZXIC_UINT32 oam_lm_ord_pful_negate;
}DPP_STAT_STAT_CFG_OAM_LM_ORD_PFUL_CFG_T;

typedef struct dpp_stat_stat_cfg_ddr_schd_pful_cfg_t
{
    ZXIC_UINT32 ddr_schd_pful_assert;
    ZXIC_UINT32 ddr_schd_pful_negate;
}DPP_STAT_STAT_CFG_DDR_SCHD_PFUL_CFG_T;

typedef struct dpp_stat_stat_cfg_eram_schd_pful_cfg_t
{
    ZXIC_UINT32 eram_schd_pful_assert;
    ZXIC_UINT32 eram_schd_pful_negate;
}DPP_STAT_STAT_CFG_ERAM_SCHD_PFUL_CFG_T;

typedef struct dpp_stat_stat_cfg_eram_schd_pept_cfg_t
{
    ZXIC_UINT32 eram_schd_pept_assert;
    ZXIC_UINT32 eram_schd_pept_negate;
}DPP_STAT_STAT_CFG_ERAM_SCHD_PEPT_CFG_T;

typedef struct dpp_stat_stat_cfg_eram_schd_oam_pful_cfg_t
{
    ZXIC_UINT32 eram_schd_oam_pful_assert;
    ZXIC_UINT32 eram_schd_oam_pful_negate;
}DPP_STAT_STAT_CFG_ERAM_SCHD_OAM_PFUL_CFG_T;

typedef struct dpp_stat_stat_cfg_eram_schd_oam_pept_cfg_t
{
    ZXIC_UINT32 eram_schd_oam_pept_assert;
    ZXIC_UINT32 eram_schd_oam_pept_negate;
}DPP_STAT_STAT_CFG_ERAM_SCHD_OAM_PEPT_CFG_T;

typedef struct dpp_stat_stat_cfg_eram_schd_oam_lm_pful_cfg_t
{
    ZXIC_UINT32 eram_schd_oam_lm_pful_assert;
    ZXIC_UINT32 eram_schd_oam_lm_pful_negate;
}DPP_STAT_STAT_CFG_ERAM_SCHD_OAM_LM_PFUL_CFG_T;

typedef struct dpp_stat_stat_cfg_eram_schd_oam_lm_pept_cfg_t
{
    ZXIC_UINT32 eram_schd_oam_lm_pept_assert;
    ZXIC_UINT32 eram_schd_oam_lm_pept_negate;
}DPP_STAT_STAT_CFG_ERAM_SCHD_OAM_LM_PEPT_CFG_T;

typedef struct dpp_stat_stat_cfg_rschd_pful_cfg_t
{
    ZXIC_UINT32 rschd_pful_assert;
    ZXIC_UINT32 rschd_pful_negate;
}DPP_STAT_STAT_CFG_RSCHD_PFUL_CFG_T;

typedef struct dpp_stat_stat_cfg_rschd_pept_cfg_t
{
    ZXIC_UINT32 rschd_pept_assert;
    ZXIC_UINT32 rschd_pept_negate;
}DPP_STAT_STAT_CFG_RSCHD_PEPT_CFG_T;

typedef struct dpp_stat_stat_cfg_rschd_plcr_pful_cfg_t
{
    ZXIC_UINT32 rschd_plcr_pful_assert;
    ZXIC_UINT32 rschd_plcr_pful_negate;
}DPP_STAT_STAT_CFG_RSCHD_PLCR_PFUL_CFG_T;

typedef struct dpp_stat_stat_cfg_rschd_plcr_pept_cfg_t
{
    ZXIC_UINT32 rschd_plcr_pept_assert;
    ZXIC_UINT32 rschd_plcr_pept_negate;
}DPP_STAT_STAT_CFG_RSCHD_PLCR_PEPT_CFG_T;

typedef struct dpp_stat_stat_cfg_rschd_plcr_info_pful_cfg_t
{
    ZXIC_UINT32 rschd_plcr_info_pful_assert;
    ZXIC_UINT32 rschd_plcr_info_pful_negate;
}DPP_STAT_STAT_CFG_RSCHD_PLCR_INFO_PFUL_CFG_T;

typedef struct dpp_stat_stat_cfg_alu_arb_cpu_pful_cfg_t
{
    ZXIC_UINT32 alu_arb_cpu_pful_assert;
    ZXIC_UINT32 alu_arb_cpu_pful_negate;
}DPP_STAT_STAT_CFG_ALU_ARB_CPU_PFUL_CFG_T;

typedef struct dpp_stat_stat_cfg_alu_arb_user_pful_cfg_t
{
    ZXIC_UINT32 alu_arb_user_pful_assert;
    ZXIC_UINT32 alu_arb_user_pful_negate;
}DPP_STAT_STAT_CFG_ALU_ARB_USER_PFUL_CFG_T;

typedef struct dpp_stat_stat_cfg_alu_arb_stat_pful_cfg_t
{
    ZXIC_UINT32 alu_arb_stat_pful_assert;
    ZXIC_UINT32 alu_arb_stat_pful_negate;
}DPP_STAT_STAT_CFG_ALU_ARB_STAT_PFUL_CFG_T;

typedef struct dpp_stat_stat_cfg_cycmov_dat_pful_cfg_t
{
    ZXIC_UINT32 cycmov_dat_pful_assert;
    ZXIC_UINT32 cycmov_dat_pful_negate;
}DPP_STAT_STAT_CFG_CYCMOV_DAT_PFUL_CFG_T;

typedef struct dpp_stat_stat_cfg_ddr_opr_pful_cfg_t
{
    ZXIC_UINT32 ddr_opr_pful_assert;
    ZXIC_UINT32 ddr_opr_pful_negate;
}DPP_STAT_STAT_CFG_DDR_OPR_PFUL_CFG_T;

typedef struct dpp_stat_stat_cfg_cycle_mov_pful_cfg_t
{
    ZXIC_UINT32 cycle_mov_pful_assert;
    ZXIC_UINT32 cycle_mov_pful_negate;
}DPP_STAT_STAT_CFG_CYCLE_MOV_PFUL_CFG_T;

typedef struct dpp_stat_stat_cfg_cntovf_pful_cfg_t
{
    ZXIC_UINT32 cntovf_pful_assert;
    ZXIC_UINT32 cntovf_pful_negate;
}DPP_STAT_STAT_CFG_CNTOVF_PFUL_CFG_T;

typedef struct dpp_stat_stat_cfg_eram_schd_plcr_pful_cfg_t
{
    ZXIC_UINT32 eram_schd_plcr_pful_assert;
    ZXIC_UINT32 eram_schd_plcr_pful_negate;
}DPP_STAT_STAT_CFG_ERAM_SCHD_PLCR_PFUL_CFG_T;

typedef struct dpp_stat_stat_cfg_eram_schd_plcr_pept_cfg_t
{
    ZXIC_UINT32 eram_schd_plcr_pept_assert;
    ZXIC_UINT32 eram_schd_plcr_pept_negate;
}DPP_STAT_STAT_CFG_ERAM_SCHD_PLCR_PEPT_CFG_T;

typedef struct dpp_stat_stat_cfg_debug_cnt_mode_t
{
    ZXIC_UINT32 cnt_rd_mode;
    ZXIC_UINT32 cnt_overflow_mode;
}DPP_STAT_STAT_CFG_DEBUG_CNT_MODE_T;

typedef struct dpp_stat_stat_cfg_tm_mov_period_cfg_t
{
    ZXIC_UINT32 etm_mov_period_cfg;
    ZXIC_UINT32 ftm_mov_period_cfg;
}DPP_STAT_STAT_CFG_TM_MOV_PERIOD_CFG_T;

typedef struct dpp_stat_stat_cfg_alu_ddr_cpu_req_pful_cfg_t
{
    ZXIC_UINT32 alu_ddr_cpu_req_pful_assert;
    ZXIC_UINT32 alu_ddr_cpu_req_pful_negate;
}DPP_STAT_STAT_CFG_ALU_DDR_CPU_REQ_PFUL_CFG_T;

typedef struct dpp_stat_stat_cfg_cycmov_addr_pful_cfg_t
{
    ZXIC_UINT32 cycmov_addr_pful_assert;
    ZXIC_UINT32 cycmov_addr_pful_negate;
}DPP_STAT_STAT_CFG_CYCMOV_ADDR_PFUL_CFG_T;

typedef struct dpp_stat_stat_cfg_ord_ddr_plcr_fifo_empty_t
{
    ZXIC_UINT32 ord_oam_lm_empty;
    ZXIC_UINT32 ddr_schd_fifo_empty;
    ZXIC_UINT32 plcr0_schd_fifo_empty;
}DPP_STAT_STAT_CFG_ORD_DDR_PLCR_FIFO_EMPTY_T;

typedef struct dpp_stat_stat_cfg_tm_stat_fifo_empty_t
{
    ZXIC_UINT32 tm_stat_fifo_empty;
}DPP_STAT_STAT_CFG_TM_STAT_FIFO_EMPTY_T;

typedef struct dpp_stat_stat_cfg_eram_schd_fifo_empty_0_1_t
{
    ZXIC_UINT32 eram_schd_fifo_empty1;
    ZXIC_UINT32 eram_schd_fifo_empty0;
}DPP_STAT_STAT_CFG_ERAM_SCHD_FIFO_EMPTY_0_1_T;

typedef struct dpp_stat_stat_cfg_eram_schd_fifo_empty_2_3_t
{
    ZXIC_UINT32 eram_schd_fifo_empty3;
    ZXIC_UINT32 eram_schd_fifo_empty2;
}DPP_STAT_STAT_CFG_ERAM_SCHD_FIFO_EMPTY_2_3_T;

typedef struct dpp_stat_stat_cfg_eram_schd_fifo_empty_4_5_t
{
    ZXIC_UINT32 eram_schd_fifo_empty5;
    ZXIC_UINT32 eram_schd_fifo_empty4;
}DPP_STAT_STAT_CFG_ERAM_SCHD_FIFO_EMPTY_4_5_T;

typedef struct dpp_stat_stat_cfg_eram_schd_fifo_empty_6_7_t
{
    ZXIC_UINT32 eram_schd_fifo_empty7;
    ZXIC_UINT32 eram_schd_fifo_empty6;
}DPP_STAT_STAT_CFG_ERAM_SCHD_FIFO_EMPTY_6_7_T;

typedef struct dpp_stat_stat_cfg_eram_schd_fifo_empty_free_8_t
{
    ZXIC_UINT32 eram_schd_free_fifo_empty8;
    ZXIC_UINT32 eram_schd_free_fifo_empty7;
    ZXIC_UINT32 eram_schd_free_fifo_empty6;
    ZXIC_UINT32 eram_schd_free_fifo_empty5;
    ZXIC_UINT32 eram_schd_free_fifo_empty4;
    ZXIC_UINT32 eram_schd_free_fifo_empty3;
    ZXIC_UINT32 eram_schd_free_fifo_empty2;
    ZXIC_UINT32 eram_schd_free_fifo_empty1;
    ZXIC_UINT32 eram_schd_free_fifo_empty0;
    ZXIC_UINT32 eram_schd_fifo_empty8;
}DPP_STAT_STAT_CFG_ERAM_SCHD_FIFO_EMPTY_FREE_8_T;

typedef struct dpp_stat_stat_cfg_rschd_fifo_empty_0_3_t
{
    ZXIC_UINT32 rschd_fifo_empty3;
    ZXIC_UINT32 rschd_fifo_empty2;
    ZXIC_UINT32 rschd_fifo_empty1;
    ZXIC_UINT32 rschd_fifo_empty0;
}DPP_STAT_STAT_CFG_RSCHD_FIFO_EMPTY_0_3_T;

typedef struct dpp_stat_stat_cfg_rschd_fifo_empty_4_7_t
{
    ZXIC_UINT32 rschd_fifo_empty7;
    ZXIC_UINT32 rschd_fifo_empty6;
    ZXIC_UINT32 rschd_fifo_empty5;
    ZXIC_UINT32 rschd_fifo_empty4;
}DPP_STAT_STAT_CFG_RSCHD_FIFO_EMPTY_4_7_T;

typedef struct dpp_stat_stat_cfg_rschd_fifo_empty_8_11_t
{
    ZXIC_UINT32 rschd_fifo_empty11;
    ZXIC_UINT32 rschd_fifo_empty10;
    ZXIC_UINT32 rschd_fifo_empty9;
    ZXIC_UINT32 rschd_fifo_empty8;
}DPP_STAT_STAT_CFG_RSCHD_FIFO_EMPTY_8_11_T;

typedef struct dpp_stat_stat_cfg_rschd_fifo_empty_12_15_t
{
    ZXIC_UINT32 rschd_fifo_empty15;
    ZXIC_UINT32 rschd_fifo_empty14;
    ZXIC_UINT32 rschd_fifo_empty13;
    ZXIC_UINT32 rschd_fifo_empty12;
}DPP_STAT_STAT_CFG_RSCHD_FIFO_EMPTY_12_15_T;

typedef struct dpp_stat_stat_cfg_rschd_fifo_empty_plcr_16_17_t
{
    ZXIC_UINT32 rschd_fifo_empty_plcr;
    ZXIC_UINT32 rschd_fifo_empty17;
    ZXIC_UINT32 rschd_fifo_empty16;
}DPP_STAT_STAT_CFG_RSCHD_FIFO_EMPTY_PLCR_16_17_T;

typedef struct dpp_stat_stat_cfg_stat_int_unmask_flag_t
{
    ZXIC_UINT32 stat_int5_unmask_flag;
    ZXIC_UINT32 stat_int4_unmask_flag;
    ZXIC_UINT32 stat_int3_unmask_flag;
    ZXIC_UINT32 stat_int2_unmask_flag;
    ZXIC_UINT32 stat_int1_unmask_flag;
    ZXIC_UINT32 stat_int0_unmask_flag;
}DPP_STAT_STAT_CFG_STAT_INT_UNMASK_FLAG_T;

typedef struct dpp_stat_stat_cfg_stat_int0_en_t
{
    ZXIC_UINT32 stat_int0_en31;
    ZXIC_UINT32 stat_int0_en30;
    ZXIC_UINT32 stat_int0_en29;
    ZXIC_UINT32 stat_int0_en28;
    ZXIC_UINT32 stat_int0_en27;
    ZXIC_UINT32 stat_int0_en26;
    ZXIC_UINT32 stat_int0_en25;
    ZXIC_UINT32 stat_int0_en24;
    ZXIC_UINT32 stat_int0_en23;
    ZXIC_UINT32 stat_int0_en22;
    ZXIC_UINT32 stat_int0_en21;
    ZXIC_UINT32 stat_int0_en20;
    ZXIC_UINT32 stat_int0_en19;
    ZXIC_UINT32 stat_int0_en18;
    ZXIC_UINT32 stat_int0_en17;
    ZXIC_UINT32 stat_int0_en16;
    ZXIC_UINT32 stat_int0_en15;
    ZXIC_UINT32 stat_int0_en14;
    ZXIC_UINT32 stat_int0_en13;
    ZXIC_UINT32 stat_int0_en12;
    ZXIC_UINT32 stat_int0_en11;
    ZXIC_UINT32 stat_int0_en10;
    ZXIC_UINT32 stat_int0_en9;
    ZXIC_UINT32 stat_int0_en8;
    ZXIC_UINT32 stat_int0_en7;
    ZXIC_UINT32 stat_int0_en6;
    ZXIC_UINT32 stat_int0_en5;
    ZXIC_UINT32 stat_int0_en4;
    ZXIC_UINT32 stat_int0_en3;
    ZXIC_UINT32 stat_int0_en2;
    ZXIC_UINT32 stat_int0_en1;
    ZXIC_UINT32 stat_int0_en0;
}DPP_STAT_STAT_CFG_STAT_INT0_EN_T;

typedef struct dpp_stat_stat_cfg_stat_int0_mask_t
{
    ZXIC_UINT32 stat_int0_mask31;
    ZXIC_UINT32 stat_int0_mask30;
    ZXIC_UINT32 stat_int0_mask29;
    ZXIC_UINT32 stat_int0_mask28;
    ZXIC_UINT32 stat_int0_mask27;
    ZXIC_UINT32 stat_int0_mask26;
    ZXIC_UINT32 stat_int0_mask25;
    ZXIC_UINT32 stat_int0_mask24;
    ZXIC_UINT32 stat_int0_mask23;
    ZXIC_UINT32 stat_int0_mask22;
    ZXIC_UINT32 stat_int0_mask21;
    ZXIC_UINT32 stat_int0_mask20;
    ZXIC_UINT32 stat_int0_mask19;
    ZXIC_UINT32 stat_int0_mask18;
    ZXIC_UINT32 stat_int0_mask17;
    ZXIC_UINT32 stat_int0_mask16;
    ZXIC_UINT32 stat_int0_mask15;
    ZXIC_UINT32 stat_int0_mask14;
    ZXIC_UINT32 stat_int0_mask13;
    ZXIC_UINT32 stat_int0_mask12;
    ZXIC_UINT32 stat_int0_mask11;
    ZXIC_UINT32 stat_int0_mask10;
    ZXIC_UINT32 stat_int0_mask9;
    ZXIC_UINT32 stat_int0_mask8;
    ZXIC_UINT32 stat_int0_mask7;
    ZXIC_UINT32 stat_int0_mask6;
    ZXIC_UINT32 stat_int0_mask5;
    ZXIC_UINT32 stat_int0_mask4;
    ZXIC_UINT32 stat_int0_mask3;
    ZXIC_UINT32 stat_int0_mask2;
    ZXIC_UINT32 stat_int0_mask1;
    ZXIC_UINT32 stat_int0_mask0;
}DPP_STAT_STAT_CFG_STAT_INT0_MASK_T;

typedef struct dpp_stat_stat_cfg_stat_int0_status_t
{
    ZXIC_UINT32 stat_int0_status31;
    ZXIC_UINT32 stat_int0_status30;
    ZXIC_UINT32 stat_int0_status29;
    ZXIC_UINT32 stat_int0_status28;
    ZXIC_UINT32 stat_int0_status27;
    ZXIC_UINT32 stat_int0_status26;
    ZXIC_UINT32 stat_int0_status25;
    ZXIC_UINT32 stat_int0_status24;
    ZXIC_UINT32 stat_int0_status23;
    ZXIC_UINT32 stat_int0_status22;
    ZXIC_UINT32 stat_int0_status21;
    ZXIC_UINT32 stat_int0_status20;
    ZXIC_UINT32 stat_int0_status19;
    ZXIC_UINT32 stat_int0_status18;
    ZXIC_UINT32 stat_int0_status17;
    ZXIC_UINT32 stat_int0_status16;
    ZXIC_UINT32 stat_int0_status15;
    ZXIC_UINT32 stat_int0_status14;
    ZXIC_UINT32 stat_int0_status13;
    ZXIC_UINT32 stat_int0_status12;
    ZXIC_UINT32 stat_int0_status11;
    ZXIC_UINT32 stat_int0_status10;
    ZXIC_UINT32 stat_int0_status9;
    ZXIC_UINT32 stat_int0_status8;
    ZXIC_UINT32 stat_int0_status7;
    ZXIC_UINT32 stat_int0_status6;
    ZXIC_UINT32 stat_int0_status5;
    ZXIC_UINT32 stat_int0_status4;
    ZXIC_UINT32 stat_int0_status3;
    ZXIC_UINT32 stat_int0_status2;
    ZXIC_UINT32 stat_int0_status1;
    ZXIC_UINT32 stat_int0_status0;
}DPP_STAT_STAT_CFG_STAT_INT0_STATUS_T;

typedef struct dpp_stat_stat_cfg_stat_int1_en_t
{
    ZXIC_UINT32 stat_int1_en31;
    ZXIC_UINT32 stat_int1_en30;
    ZXIC_UINT32 stat_int1_en29;
    ZXIC_UINT32 stat_int1_en28;
    ZXIC_UINT32 stat_int1_en27;
    ZXIC_UINT32 stat_int1_en26;
    ZXIC_UINT32 stat_int1_en25;
    ZXIC_UINT32 stat_int1_en24;
    ZXIC_UINT32 stat_int1_en23;
    ZXIC_UINT32 stat_int1_en22;
    ZXIC_UINT32 stat_int1_en21;
    ZXIC_UINT32 stat_int1_en20;
    ZXIC_UINT32 stat_int1_en19;
    ZXIC_UINT32 stat_int1_en18;
    ZXIC_UINT32 stat_int1_en17;
    ZXIC_UINT32 stat_int1_en16;
    ZXIC_UINT32 stat_int1_en15;
    ZXIC_UINT32 stat_int1_en14;
    ZXIC_UINT32 stat_int1_en13;
    ZXIC_UINT32 stat_int1_en12;
    ZXIC_UINT32 stat_int1_en11;
    ZXIC_UINT32 stat_int1_en10;
    ZXIC_UINT32 stat_int1_en9;
    ZXIC_UINT32 stat_int1_en8;
    ZXIC_UINT32 stat_int1_en7;
    ZXIC_UINT32 stat_int1_en6;
    ZXIC_UINT32 stat_int1_en5;
    ZXIC_UINT32 stat_int1_en4;
    ZXIC_UINT32 stat_int1_en3;
    ZXIC_UINT32 stat_int1_en2;
    ZXIC_UINT32 stat_int1_en1;
    ZXIC_UINT32 stat_int1_en0;
}DPP_STAT_STAT_CFG_STAT_INT1_EN_T;

typedef struct dpp_stat_stat_cfg_stat_int1_mask_t
{
    ZXIC_UINT32 stat_int1_mask31;
    ZXIC_UINT32 stat_int1_mask30;
    ZXIC_UINT32 stat_int1_mask29;
    ZXIC_UINT32 stat_int1_mask28;
    ZXIC_UINT32 stat_int1_mask27;
    ZXIC_UINT32 stat_int1_mask26;
    ZXIC_UINT32 stat_int1_mask25;
    ZXIC_UINT32 stat_int1_mask24;
    ZXIC_UINT32 stat_int1_mask23;
    ZXIC_UINT32 stat_int1_mask22;
    ZXIC_UINT32 stat_int1_mask21;
    ZXIC_UINT32 stat_int1_mask20;
    ZXIC_UINT32 stat_int1_mask19;
    ZXIC_UINT32 stat_int1_mask18;
    ZXIC_UINT32 stat_int1_mask17;
    ZXIC_UINT32 stat_int1_mask16;
    ZXIC_UINT32 stat_int1_mask15;
    ZXIC_UINT32 stat_int1_mask14;
    ZXIC_UINT32 stat_int1_mask13;
    ZXIC_UINT32 stat_int1_mask12;
    ZXIC_UINT32 stat_int1_mask11;
    ZXIC_UINT32 stat_int1_mask10;
    ZXIC_UINT32 stat_int1_mask9;
    ZXIC_UINT32 stat_int1_mask8;
    ZXIC_UINT32 stat_int1_mask7;
    ZXIC_UINT32 stat_int1_mask6;
    ZXIC_UINT32 stat_int1_mask5;
    ZXIC_UINT32 stat_int1_mask4;
    ZXIC_UINT32 stat_int1_mask3;
    ZXIC_UINT32 stat_int1_mask2;
    ZXIC_UINT32 stat_int1_mask1;
    ZXIC_UINT32 stat_int1_mask0;
}DPP_STAT_STAT_CFG_STAT_INT1_MASK_T;

typedef struct dpp_stat_stat_cfg_stat_int1_status_t
{
    ZXIC_UINT32 stat_int1_status31;
    ZXIC_UINT32 stat_int1_status30;
    ZXIC_UINT32 stat_int1_status29;
    ZXIC_UINT32 stat_int1_status28;
    ZXIC_UINT32 stat_int1_status27;
    ZXIC_UINT32 stat_int1_status26;
    ZXIC_UINT32 stat_int1_status25;
    ZXIC_UINT32 stat_int1_status24;
    ZXIC_UINT32 stat_int1_status23;
    ZXIC_UINT32 stat_int1_status22;
    ZXIC_UINT32 stat_int1_status21;
    ZXIC_UINT32 stat_int1_status20;
    ZXIC_UINT32 stat_int1_status19;
    ZXIC_UINT32 stat_int1_status18;
    ZXIC_UINT32 stat_int1_status17;
    ZXIC_UINT32 stat_int1_status16;
    ZXIC_UINT32 stat_int1_status15;
    ZXIC_UINT32 stat_int1_status14;
    ZXIC_UINT32 stat_int1_status13;
    ZXIC_UINT32 stat_int1_status12;
    ZXIC_UINT32 stat_int1_status11;
    ZXIC_UINT32 stat_int1_status10;
    ZXIC_UINT32 stat_int1_status9;
    ZXIC_UINT32 stat_int1_status8;
    ZXIC_UINT32 stat_int1_status7;
    ZXIC_UINT32 stat_int1_status6;
    ZXIC_UINT32 stat_int1_status5;
    ZXIC_UINT32 stat_int1_status4;
    ZXIC_UINT32 stat_int1_status3;
    ZXIC_UINT32 stat_int1_status2;
    ZXIC_UINT32 stat_int1_status1;
    ZXIC_UINT32 stat_int1_status0;
}DPP_STAT_STAT_CFG_STAT_INT1_STATUS_T;

typedef struct dpp_stat_stat_cfg_stat_int2_en_t
{
    ZXIC_UINT32 stat_int2_en31;
    ZXIC_UINT32 stat_int2_en30;
    ZXIC_UINT32 stat_int2_en29;
    ZXIC_UINT32 stat_int2_en28;
    ZXIC_UINT32 stat_int2_en27;
    ZXIC_UINT32 stat_int2_en26;
    ZXIC_UINT32 stat_int2_en25;
    ZXIC_UINT32 stat_int2_en24;
    ZXIC_UINT32 stat_int2_en23;
    ZXIC_UINT32 stat_int2_en22;
    ZXIC_UINT32 stat_int2_en21;
    ZXIC_UINT32 stat_int2_en20;
    ZXIC_UINT32 stat_int2_en19;
    ZXIC_UINT32 stat_int2_en18;
    ZXIC_UINT32 stat_int2_en17;
    ZXIC_UINT32 stat_int2_en16;
    ZXIC_UINT32 stat_int2_en15;
    ZXIC_UINT32 stat_int2_en14;
    ZXIC_UINT32 stat_int2_en13;
    ZXIC_UINT32 stat_int2_en12;
    ZXIC_UINT32 stat_int2_en11;
    ZXIC_UINT32 stat_int2_en10;
    ZXIC_UINT32 stat_int2_en9;
    ZXIC_UINT32 stat_int2_en8;
    ZXIC_UINT32 stat_int2_en7;
    ZXIC_UINT32 stat_int2_en6;
    ZXIC_UINT32 stat_int2_en5;
    ZXIC_UINT32 stat_int2_en4;
    ZXIC_UINT32 stat_int2_en3;
    ZXIC_UINT32 stat_int2_en2;
    ZXIC_UINT32 stat_int2_en1;
    ZXIC_UINT32 stat_int2_en0;
}DPP_STAT_STAT_CFG_STAT_INT2_EN_T;

typedef struct dpp_stat_stat_cfg_stat_int2_mask_t
{
    ZXIC_UINT32 stat_int2_mask31;
    ZXIC_UINT32 stat_int2_mask30;
    ZXIC_UINT32 stat_int2_mask29;
    ZXIC_UINT32 stat_int2_mask28;
    ZXIC_UINT32 stat_int2_mask27;
    ZXIC_UINT32 stat_int2_mask26;
    ZXIC_UINT32 stat_int2_mask25;
    ZXIC_UINT32 stat_int2_mask24;
    ZXIC_UINT32 stat_int2_mask23;
    ZXIC_UINT32 stat_int2_mask22;
    ZXIC_UINT32 stat_int2_mask21;
    ZXIC_UINT32 stat_int2_mask20;
    ZXIC_UINT32 stat_int2_mask19;
    ZXIC_UINT32 stat_int2_mask18;
    ZXIC_UINT32 stat_int2_mask17;
    ZXIC_UINT32 stat_int2_mask16;
    ZXIC_UINT32 stat_int2_mask15;
    ZXIC_UINT32 stat_int2_mask14;
    ZXIC_UINT32 stat_int2_mask13;
    ZXIC_UINT32 stat_int2_mask12;
    ZXIC_UINT32 stat_int2_mask11;
    ZXIC_UINT32 stat_int2_mask10;
    ZXIC_UINT32 stat_int2_mask9;
    ZXIC_UINT32 stat_int2_mask8;
    ZXIC_UINT32 stat_int2_mask7;
    ZXIC_UINT32 stat_int2_mask6;
    ZXIC_UINT32 stat_int2_mask5;
    ZXIC_UINT32 stat_int2_mask4;
    ZXIC_UINT32 stat_int2_mask3;
    ZXIC_UINT32 stat_int2_mask2;
    ZXIC_UINT32 stat_int2_mask1;
    ZXIC_UINT32 stat_int2_mask0;
}DPP_STAT_STAT_CFG_STAT_INT2_MASK_T;

typedef struct dpp_stat_stat_cfg_stat_int2_status_t
{
    ZXIC_UINT32 stat_int2_status31;
    ZXIC_UINT32 stat_int2_status30;
    ZXIC_UINT32 stat_int2_status29;
    ZXIC_UINT32 stat_int2_status28;
    ZXIC_UINT32 stat_int2_status27;
    ZXIC_UINT32 stat_int2_status26;
    ZXIC_UINT32 stat_int2_status25;
    ZXIC_UINT32 stat_int2_status24;
    ZXIC_UINT32 stat_int2_status23;
    ZXIC_UINT32 stat_int2_status22;
    ZXIC_UINT32 stat_int2_status21;
    ZXIC_UINT32 stat_int2_status20;
    ZXIC_UINT32 stat_int2_status19;
    ZXIC_UINT32 stat_int2_status18;
    ZXIC_UINT32 stat_int2_status17;
    ZXIC_UINT32 stat_int2_status16;
    ZXIC_UINT32 stat_int2_status15;
    ZXIC_UINT32 stat_int2_status14;
    ZXIC_UINT32 stat_int2_status13;
    ZXIC_UINT32 stat_int2_status12;
    ZXIC_UINT32 stat_int2_status11;
    ZXIC_UINT32 stat_int2_status10;
    ZXIC_UINT32 stat_int2_status9;
    ZXIC_UINT32 stat_int2_status8;
    ZXIC_UINT32 stat_int2_status7;
    ZXIC_UINT32 stat_int2_status6;
    ZXIC_UINT32 stat_int2_status5;
    ZXIC_UINT32 stat_int2_status4;
    ZXIC_UINT32 stat_int2_status3;
    ZXIC_UINT32 stat_int2_status2;
    ZXIC_UINT32 stat_int2_status1;
    ZXIC_UINT32 stat_int2_status0;
}DPP_STAT_STAT_CFG_STAT_INT2_STATUS_T;

typedef struct dpp_stat_stat_cfg_stat_int3_en_t
{
    ZXIC_UINT32 stat_int3_en31;
    ZXIC_UINT32 stat_int3_en30;
    ZXIC_UINT32 stat_int3_en29;
    ZXIC_UINT32 stat_int3_en28;
    ZXIC_UINT32 stat_int3_en27;
    ZXIC_UINT32 stat_int3_en26;
    ZXIC_UINT32 stat_int3_en25;
    ZXIC_UINT32 stat_int3_en24;
    ZXIC_UINT32 stat_int3_en23;
    ZXIC_UINT32 stat_int3_en22;
    ZXIC_UINT32 stat_int3_en21;
    ZXIC_UINT32 stat_int3_en20;
    ZXIC_UINT32 stat_int3_en19;
    ZXIC_UINT32 stat_int3_en18;
    ZXIC_UINT32 stat_int3_en17;
    ZXIC_UINT32 stat_int3_en16;
    ZXIC_UINT32 stat_int3_en15;
    ZXIC_UINT32 stat_int3_en14;
    ZXIC_UINT32 stat_int3_en13;
    ZXIC_UINT32 stat_int3_en12;
    ZXIC_UINT32 stat_int3_en11;
    ZXIC_UINT32 stat_int3_en10;
    ZXIC_UINT32 stat_int3_en9;
    ZXIC_UINT32 stat_int3_en8;
    ZXIC_UINT32 stat_int3_en7;
    ZXIC_UINT32 stat_int3_en6;
    ZXIC_UINT32 stat_int3_en5;
    ZXIC_UINT32 stat_int3_en4;
    ZXIC_UINT32 stat_int3_en3;
    ZXIC_UINT32 stat_int3_en2;
    ZXIC_UINT32 stat_int3_en1;
    ZXIC_UINT32 stat_int3_en0;
}DPP_STAT_STAT_CFG_STAT_INT3_EN_T;

typedef struct dpp_stat_stat_cfg_stat_int3_mask_t
{
    ZXIC_UINT32 stat_int3_mask31;
    ZXIC_UINT32 stat_int3_mask30;
    ZXIC_UINT32 stat_int3_mask29;
    ZXIC_UINT32 stat_int3_mask28;
    ZXIC_UINT32 stat_int3_mask27;
    ZXIC_UINT32 stat_int3_mask26;
    ZXIC_UINT32 stat_int3_mask25;
    ZXIC_UINT32 stat_int3_mask24;
    ZXIC_UINT32 stat_int3_mask23;
    ZXIC_UINT32 stat_int3_mask22;
    ZXIC_UINT32 stat_int3_mask21;
    ZXIC_UINT32 stat_int3_mask20;
    ZXIC_UINT32 stat_int3_mask19;
    ZXIC_UINT32 stat_int3_mask18;
    ZXIC_UINT32 stat_int3_mask17;
    ZXIC_UINT32 stat_int3_mask16;
    ZXIC_UINT32 stat_int3_mask15;
    ZXIC_UINT32 stat_int3_mask14;
    ZXIC_UINT32 stat_int3_mask13;
    ZXIC_UINT32 stat_int3_mask12;
    ZXIC_UINT32 stat_int3_mask11;
    ZXIC_UINT32 stat_int3_mask10;
    ZXIC_UINT32 stat_int3_mask9;
    ZXIC_UINT32 stat_int3_mask8;
    ZXIC_UINT32 stat_int3_mask7;
    ZXIC_UINT32 stat_int3_mask6;
    ZXIC_UINT32 stat_int3_mask5;
    ZXIC_UINT32 stat_int3_mask4;
    ZXIC_UINT32 stat_int3_mask3;
    ZXIC_UINT32 stat_int3_mask2;
    ZXIC_UINT32 stat_int3_mask1;
    ZXIC_UINT32 stat_int3_mask0;
}DPP_STAT_STAT_CFG_STAT_INT3_MASK_T;

typedef struct dpp_stat_stat_cfg_stat_int3_status_t
{
    ZXIC_UINT32 stat_int3_status31;
    ZXIC_UINT32 stat_int3_status30;
    ZXIC_UINT32 stat_int3_status29;
    ZXIC_UINT32 stat_int3_status28;
    ZXIC_UINT32 stat_int3_status27;
    ZXIC_UINT32 stat_int3_status26;
    ZXIC_UINT32 stat_int3_status25;
    ZXIC_UINT32 stat_int3_status24;
    ZXIC_UINT32 stat_int3_status23;
    ZXIC_UINT32 stat_int3_status22;
    ZXIC_UINT32 stat_int3_status21;
    ZXIC_UINT32 stat_int3_status20;
    ZXIC_UINT32 stat_int3_status19;
    ZXIC_UINT32 stat_int3_status18;
    ZXIC_UINT32 stat_int3_status17;
    ZXIC_UINT32 stat_int3_status16;
    ZXIC_UINT32 stat_int3_status15;
    ZXIC_UINT32 stat_int3_status14;
    ZXIC_UINT32 stat_int3_status13;
    ZXIC_UINT32 stat_int3_status12;
    ZXIC_UINT32 stat_int3_status11;
    ZXIC_UINT32 stat_int3_status10;
    ZXIC_UINT32 stat_int3_status9;
    ZXIC_UINT32 stat_int3_status8;
    ZXIC_UINT32 stat_int3_status7;
    ZXIC_UINT32 stat_int3_status6;
    ZXIC_UINT32 stat_int3_status5;
    ZXIC_UINT32 stat_int3_status4;
    ZXIC_UINT32 stat_int3_status3;
    ZXIC_UINT32 stat_int3_status2;
    ZXIC_UINT32 stat_int3_status1;
    ZXIC_UINT32 stat_int3_status0;
}DPP_STAT_STAT_CFG_STAT_INT3_STATUS_T;

typedef struct dpp_stat_stat_cfg_stat_int4_en_t
{
    ZXIC_UINT32 stat_int4_en_18;
    ZXIC_UINT32 stat_int4_en_17;
    ZXIC_UINT32 stat_int4_en_16;
    ZXIC_UINT32 stat_int4_en_15;
    ZXIC_UINT32 stat_int4_en_14;
    ZXIC_UINT32 stat_int4_en_13;
    ZXIC_UINT32 stat_int4_en_12;
    ZXIC_UINT32 stat_int4_en_11;
    ZXIC_UINT32 stat_int4_en_10;
    ZXIC_UINT32 stat_int4_en_9;
    ZXIC_UINT32 stat_int4_en_8;
    ZXIC_UINT32 stat_int4_en_7;
    ZXIC_UINT32 stat_int4_en_6;
    ZXIC_UINT32 stat_int4_en_5;
    ZXIC_UINT32 stat_int4_en_4;
    ZXIC_UINT32 stat_int4_en_3;
    ZXIC_UINT32 stat_int4_en_2;
    ZXIC_UINT32 stat_int4_en_1;
    ZXIC_UINT32 stat_int4_en_0;
}DPP_STAT_STAT_CFG_STAT_INT4_EN_T;

typedef struct dpp_stat_stat_cfg_stat_int4_mask_t
{
    ZXIC_UINT32 stat_int4_mask_18;
    ZXIC_UINT32 stat_int4_mask_17;
    ZXIC_UINT32 stat_int4_mask_16;
    ZXIC_UINT32 stat_int4_mask_15;
    ZXIC_UINT32 stat_int4_mask_14;
    ZXIC_UINT32 stat_int4_mask_13;
    ZXIC_UINT32 stat_int4_mask_12;
    ZXIC_UINT32 stat_int4_mask_11;
    ZXIC_UINT32 stat_int4_mask_10;
    ZXIC_UINT32 stat_int4_mask_9;
    ZXIC_UINT32 stat_int4_mask_8;
    ZXIC_UINT32 stat_int4_mask_7;
    ZXIC_UINT32 stat_int4_mask_6;
    ZXIC_UINT32 stat_int4_mask_5;
    ZXIC_UINT32 stat_int4_mask_4;
    ZXIC_UINT32 stat_int4_mask_3;
    ZXIC_UINT32 stat_int4_mask_2;
    ZXIC_UINT32 stat_int4_mask_1;
    ZXIC_UINT32 stat_int4_mask_0;
}DPP_STAT_STAT_CFG_STAT_INT4_MASK_T;

typedef struct dpp_stat_stat_cfg_stat_int4_status_t
{
    ZXIC_UINT32 stat_int4_mask_18;
    ZXIC_UINT32 stat_int4_mask_17;
    ZXIC_UINT32 stat_int4_mask_16;
    ZXIC_UINT32 stat_int4_mask_15;
    ZXIC_UINT32 stat_int4_mask_14;
    ZXIC_UINT32 stat_int4_mask_13;
    ZXIC_UINT32 stat_int4_mask_12;
    ZXIC_UINT32 stat_int4_mask_11;
    ZXIC_UINT32 stat_int4_mask_10;
    ZXIC_UINT32 stat_int4_mask_9;
    ZXIC_UINT32 stat_int4_mask_8;
    ZXIC_UINT32 stat_int4_mask_7;
    ZXIC_UINT32 stat_int4_mask_6;
    ZXIC_UINT32 stat_int4_mask_5;
    ZXIC_UINT32 stat_int4_mask_4;
    ZXIC_UINT32 stat_int4_mask_3;
    ZXIC_UINT32 stat_int4_mask_2;
    ZXIC_UINT32 stat_int4_mask_1;
    ZXIC_UINT32 stat_int4_mask_0;
}DPP_STAT_STAT_CFG_STAT_INT4_STATUS_T;

typedef struct dpp_stat_stat_cfg_stat_int5_en_t
{
    ZXIC_UINT32 stat_int5_en_18;
    ZXIC_UINT32 stat_int5_en_17;
    ZXIC_UINT32 stat_int5_en_16;
    ZXIC_UINT32 stat_int5_en_15;
    ZXIC_UINT32 stat_int5_en_14;
    ZXIC_UINT32 stat_int5_en_13;
    ZXIC_UINT32 stat_int5_en_12;
    ZXIC_UINT32 stat_int5_en_11;
    ZXIC_UINT32 stat_int5_en_10;
    ZXIC_UINT32 stat_int5_en_9;
    ZXIC_UINT32 stat_int5_en_8;
    ZXIC_UINT32 stat_int5_en_7;
    ZXIC_UINT32 stat_int5_en_6;
    ZXIC_UINT32 stat_int5_en_5;
    ZXIC_UINT32 stat_int5_en_4;
    ZXIC_UINT32 stat_int5_en_3;
    ZXIC_UINT32 stat_int5_en_2;
    ZXIC_UINT32 stat_int5_en_1;
    ZXIC_UINT32 stat_int5_en_0;
}DPP_STAT_STAT_CFG_STAT_INT5_EN_T;

typedef struct dpp_stat_stat_cfg_stat_int5_mask_t
{
    ZXIC_UINT32 stat_int5_mask_18;
    ZXIC_UINT32 stat_int5_mask_17;
    ZXIC_UINT32 stat_int5_mask_16;
    ZXIC_UINT32 stat_int5_mask_15;
    ZXIC_UINT32 stat_int5_mask_14;
    ZXIC_UINT32 stat_int5_mask_13;
    ZXIC_UINT32 stat_int5_mask_12;
    ZXIC_UINT32 stat_int5_mask_11;
    ZXIC_UINT32 stat_int5_mask_10;
    ZXIC_UINT32 stat_int5_mask_9;
    ZXIC_UINT32 stat_int5_mask_8;
    ZXIC_UINT32 stat_int5_mask_7;
    ZXIC_UINT32 stat_int5_mask_6;
    ZXIC_UINT32 stat_int5_mask_5;
    ZXIC_UINT32 stat_int5_mask_4;
    ZXIC_UINT32 stat_int5_mask_3;
    ZXIC_UINT32 stat_int5_mask_2;
    ZXIC_UINT32 stat_int5_mask_1;
    ZXIC_UINT32 stat_int5_mask_0;
}DPP_STAT_STAT_CFG_STAT_INT5_MASK_T;

typedef struct dpp_stat_stat_cfg_stat_int5_status_t
{
    ZXIC_UINT32 stat_int5_mask_18;
    ZXIC_UINT32 stat_int5_mask_17;
    ZXIC_UINT32 stat_int5_mask_16;
    ZXIC_UINT32 stat_int5_mask_15;
    ZXIC_UINT32 stat_int5_mask_14;
    ZXIC_UINT32 stat_int5_mask_13;
    ZXIC_UINT32 stat_int5_mask_12;
    ZXIC_UINT32 stat_int5_mask_11;
    ZXIC_UINT32 stat_int5_mask_10;
    ZXIC_UINT32 stat_int5_mask_9;
    ZXIC_UINT32 stat_int5_mask_8;
    ZXIC_UINT32 stat_int5_mask_7;
    ZXIC_UINT32 stat_int5_mask_6;
    ZXIC_UINT32 stat_int5_mask_5;
    ZXIC_UINT32 stat_int5_mask_4;
    ZXIC_UINT32 stat_int5_mask_3;
    ZXIC_UINT32 stat_int5_mask_2;
    ZXIC_UINT32 stat_int5_mask_1;
    ZXIC_UINT32 stat_int5_mask_0;
}DPP_STAT_STAT_CFG_STAT_INT5_STATUS_T;

typedef struct dpp_stat_stat_cfg_rschd_ecc_bypass_t
{
    ZXIC_UINT32 rschd_ecc_bypass_18;
    ZXIC_UINT32 rschd_ecc_bypass_17;
    ZXIC_UINT32 rschd_ecc_bypass_16;
    ZXIC_UINT32 rschd_ecc_bypass_15;
    ZXIC_UINT32 rschd_ecc_bypass_14;
    ZXIC_UINT32 rschd_ecc_bypass_13;
    ZXIC_UINT32 rschd_ecc_bypass_12;
    ZXIC_UINT32 rschd_ecc_bypass_11;
    ZXIC_UINT32 rschd_ecc_bypass_10;
    ZXIC_UINT32 rschd_ecc_bypass_9;
    ZXIC_UINT32 rschd_ecc_bypass_8;
    ZXIC_UINT32 rschd_ecc_bypass_7;
    ZXIC_UINT32 rschd_ecc_bypass_6;
    ZXIC_UINT32 rschd_ecc_bypass_5;
    ZXIC_UINT32 rschd_ecc_bypass_4;
    ZXIC_UINT32 rschd_ecc_bypass_3;
    ZXIC_UINT32 rschd_ecc_bypass_2;
    ZXIC_UINT32 rschd_ecc_bypass_1;
    ZXIC_UINT32 rschd_ecc_bypass_0;
}DPP_STAT_STAT_CFG_RSCHD_ECC_BYPASS_T;

typedef struct dpp_stat_stat_cfg_rschd_ecc_single_err_t
{
    ZXIC_UINT32 rschd_ecc_single_err_18;
    ZXIC_UINT32 rschd_ecc_single_err_17;
    ZXIC_UINT32 rschd_ecc_single_err_16;
    ZXIC_UINT32 rschd_ecc_single_err_15;
    ZXIC_UINT32 rschd_ecc_single_err_14;
    ZXIC_UINT32 rschd_ecc_single_err_13;
    ZXIC_UINT32 rschd_ecc_single_err_12;
    ZXIC_UINT32 rschd_ecc_single_err_11;
    ZXIC_UINT32 rschd_ecc_single_err_10;
    ZXIC_UINT32 rschd_ecc_single_err_9;
    ZXIC_UINT32 rschd_ecc_single_err_8;
    ZXIC_UINT32 rschd_ecc_single_err_7;
    ZXIC_UINT32 rschd_ecc_single_err_6;
    ZXIC_UINT32 rschd_ecc_single_err_5;
    ZXIC_UINT32 rschd_ecc_single_err_4;
    ZXIC_UINT32 rschd_ecc_single_err_3;
    ZXIC_UINT32 rschd_ecc_single_err_2;
    ZXIC_UINT32 rschd_ecc_single_err_1;
    ZXIC_UINT32 rschd_ecc_single_err_0;
}DPP_STAT_STAT_CFG_RSCHD_ECC_SINGLE_ERR_T;

typedef struct dpp_stat_stat_cfg_rschd_ecc_double_err_t
{
    ZXIC_UINT32 rschd_ecc_double_err_18;
    ZXIC_UINT32 rschd_ecc_double_err_17;
    ZXIC_UINT32 rschd_ecc_double_err_16;
    ZXIC_UINT32 rschd_ecc_double_err_15;
    ZXIC_UINT32 rschd_ecc_double_err_14;
    ZXIC_UINT32 rschd_ecc_double_err_13;
    ZXIC_UINT32 rschd_ecc_double_err_12;
    ZXIC_UINT32 rschd_ecc_double_err_11;
    ZXIC_UINT32 rschd_ecc_double_err_10;
    ZXIC_UINT32 rschd_ecc_double_err_9;
    ZXIC_UINT32 rschd_ecc_double_err_8;
    ZXIC_UINT32 rschd_ecc_double_err_7;
    ZXIC_UINT32 rschd_ecc_double_err_6;
    ZXIC_UINT32 rschd_ecc_double_err_5;
    ZXIC_UINT32 rschd_ecc_double_err_4;
    ZXIC_UINT32 rschd_ecc_double_err_3;
    ZXIC_UINT32 rschd_ecc_double_err_2;
    ZXIC_UINT32 rschd_ecc_double_err_1;
    ZXIC_UINT32 rschd_ecc_double_err_0;
}DPP_STAT_STAT_CFG_RSCHD_ECC_DOUBLE_ERR_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_wdat0_t
{
    ZXIC_UINT32 cpu_ind_ddr_wdat0;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_WDAT0_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_wdat1_t
{
    ZXIC_UINT32 cpu_ind_ddr_wdat1;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_WDAT1_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_wdat2_t
{
    ZXIC_UINT32 cpu_ind_ddr_wdat2;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_WDAT2_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_wdat3_t
{
    ZXIC_UINT32 cpu_ind_ddr_wdat3;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_WDAT3_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_wdat4_t
{
    ZXIC_UINT32 cpu_ind_ddr_wdat4;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_WDAT4_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_wdat5_t
{
    ZXIC_UINT32 cpu_ind_ddr_wdat5;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_WDAT5_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_wdat6_t
{
    ZXIC_UINT32 cpu_ind_ddr_wdat6;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_WDAT6_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_wdat7_t
{
    ZXIC_UINT32 cpu_ind_ddr_wdat7;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_WDAT7_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_wdat8_t
{
    ZXIC_UINT32 cpu_ind_ddr_wdat8;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_WDAT8_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_wdat9_t
{
    ZXIC_UINT32 cpu_ind_ddr_wdat9;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_WDAT9_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_wdat10_t
{
    ZXIC_UINT32 cpu_ind_ddr_wdat10;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_WDAT10_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_wdat11_t
{
    ZXIC_UINT32 cpu_ind_ddr_wdat11;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_WDAT11_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_wdat12_t
{
    ZXIC_UINT32 cpu_ind_ddr_wdat12;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_WDAT12_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_wdat13_t
{
    ZXIC_UINT32 cpu_ind_ddr_wdat13;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_WDAT13_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_wdat14_t
{
    ZXIC_UINT32 cpu_ind_ddr_wdat14;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_WDAT14_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_wdat15_t
{
    ZXIC_UINT32 cpu_ind_ddr_wdat15;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_WDAT15_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_req_info_t
{
    ZXIC_UINT32 rw_mode;
    ZXIC_UINT32 read_mode;
    ZXIC_UINT32 tm_cs;
    ZXIC_UINT32 rw_addr;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_REQ_INFO_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_rd_done_t
{
    ZXIC_UINT32 cpu_ind_ddr_rd_done;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_RD_DONE_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_rdat0_t
{
    ZXIC_UINT32 cpu_ind_ddr_rdat0;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_RDAT0_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_rdat1_t
{
    ZXIC_UINT32 cpu_ind_ddr_rdat1;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_RDAT1_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_rdat2_t
{
    ZXIC_UINT32 cpu_ind_ddr_rdat2;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_RDAT2_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_rdat3_t
{
    ZXIC_UINT32 cpu_ind_ddr_rdat3;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_RDAT3_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_rdat4_t
{
    ZXIC_UINT32 cpu_ind_ddr_rdat4;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_RDAT4_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_rdat5_t
{
    ZXIC_UINT32 cpu_ind_ddr_rdat5;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_RDAT5_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_rdat6_t
{
    ZXIC_UINT32 cpu_ind_ddr_rdat6;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_RDAT6_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_rdat7_t
{
    ZXIC_UINT32 cpu_ind_ddr_rdat7;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_RDAT7_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_rdat8_t
{
    ZXIC_UINT32 cpu_ind_ddr_rdat8;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_RDAT8_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_rdat9_t
{
    ZXIC_UINT32 cpu_ind_ddr_rdat9;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_RDAT9_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_rdat10_t
{
    ZXIC_UINT32 cpu_ind_ddr_rdat10;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_RDAT10_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_rdat11_t
{
    ZXIC_UINT32 cpu_ind_ddr_rdat11;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_RDAT11_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_rdat12_t
{
    ZXIC_UINT32 cpu_ind_ddr_rdat12;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_RDAT12_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_rdat13_t
{
    ZXIC_UINT32 cpu_ind_ddr_rdat13;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_RDAT13_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_rdat14_t
{
    ZXIC_UINT32 cpu_ind_ddr_rdat14;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_RDAT14_T;

typedef struct dpp_stat_stat_cfg_cpu_ind_ddr_rdat15_t
{
    ZXIC_UINT32 cpu_ind_ddr_rdat15;
}DPP_STAT_STAT_CFG_CPU_IND_DDR_RDAT15_T;

typedef struct dpp_stat_stat_cfg_tm_alu_ddr_cpu_rdy_t
{
    ZXIC_UINT32 tm_alu_ddr_cpu_rdy;
}DPP_STAT_STAT_CFG_TM_ALU_DDR_CPU_RDY_T;

typedef struct dpp_stat_stat_cfg_ept_flag_t
{
    ZXIC_UINT32 ept_flag;
}DPP_STAT_STAT_CFG_EPT_FLAG_T;

typedef struct dpp_stat_stat_cfg_ppu_soft_rst_t
{
    ZXIC_UINT32 ppu_soft_rst;
}DPP_STAT_STAT_CFG_PPU_SOFT_RST_T;

typedef struct dpp_stat_stat_cfg_stat_smmu0_fc15_0_cnt_t
{
    ZXIC_UINT32 stat_smmu0_fc15_0_cnt;
}DPP_STAT_STAT_CFG_STAT_SMMU0_FC15_0_CNT_T;

typedef struct dpp_stat_stat_cfg_smmu0_stat_fc15_0_cnt_t
{
    ZXIC_UINT32 smmu0_stat_fc15_0_cnt;
}DPP_STAT_STAT_CFG_SMMU0_STAT_FC15_0_CNT_T;

typedef struct dpp_stat_stat_cfg_smmu0_stat_rsp15_0_cnt_t
{
    ZXIC_UINT32 smmu0_stat_rsp15_0_cnt;
}DPP_STAT_STAT_CFG_SMMU0_STAT_RSP15_0_CNT_T;

typedef struct dpp_stat_stat_cfg_stat_smmu0_req15_0_cnt_t
{
    ZXIC_UINT32 stat_smmu0_req15_0_cnt;
}DPP_STAT_STAT_CFG_STAT_SMMU0_REQ15_0_CNT_T;

typedef struct dpp_stat_stat_cfg_ppu_stat_mec5_0_rsp_fc_cnt_t
{
    ZXIC_UINT32 ppu_stat_mec5_0_rsp_fc_cnt;
}DPP_STAT_STAT_CFG_PPU_STAT_MEC5_0_RSP_FC_CNT_T;

typedef struct dpp_stat_stat_cfg_stat_ppu_mec5_0_key_fc_cnt_t
{
    ZXIC_UINT32 stat_ppu_mec5_0_key_fc_cnt;
}DPP_STAT_STAT_CFG_STAT_PPU_MEC5_0_KEY_FC_CNT_T;

typedef struct dpp_stat_stat_cfg_stat_ppu_mec5_0_rsp_cnt_t
{
    ZXIC_UINT32 stat_ppu_mec5_0_rsp_cnt;
}DPP_STAT_STAT_CFG_STAT_PPU_MEC5_0_RSP_CNT_T;

typedef struct dpp_stat_stat_cfg_ppu_stat_mec5_0_key_cnt_t
{
    ZXIC_UINT32 ppu_stat_mec5_0_key_cnt;
}DPP_STAT_STAT_CFG_PPU_STAT_MEC5_0_KEY_CNT_T;

typedef struct dpp_stat_stat_cfg_ppu5_0_no_exist_opcd_ex_cnt_t
{
    ZXIC_UINT32 ppu5_0_no_exist_opcd_ex_cnt;
}DPP_STAT_STAT_CFG_PPU5_0_NO_EXIST_OPCD_EX_CNT_T;

typedef struct dpp_stat_stat_cfg_se_etm_stat_wr_fc_cnt_t
{
    ZXIC_UINT32 se_etm_stat_wr_fc_cnt;
}DPP_STAT_STAT_CFG_SE_ETM_STAT_WR_FC_CNT_T;

typedef struct dpp_stat_stat_cfg_se_etm_stat_rd_fc_cnt_t
{
    ZXIC_UINT32 se_etm_stat_rd_fc_cnt;
}DPP_STAT_STAT_CFG_SE_ETM_STAT_RD_FC_CNT_T;

typedef struct dpp_stat_stat_cfg_stat_etm_deq_fc_cnt_t
{
    ZXIC_UINT32 stat_etm_deq_fc_cnt;
}DPP_STAT_STAT_CFG_STAT_ETM_DEQ_FC_CNT_T;

typedef struct dpp_stat_stat_cfg_stat_etm_enq_fc_cnt_t
{
    ZXIC_UINT32 stat_etm_enq_fc_cnt;
}DPP_STAT_STAT_CFG_STAT_ETM_ENQ_FC_CNT_T;

typedef struct dpp_stat_stat_cfg_stat_oam_lm_fc_cnt_t
{
    ZXIC_UINT32 stat_oam_lm_fc_cnt;
}DPP_STAT_STAT_CFG_STAT_OAM_LM_FC_CNT_T;

typedef struct dpp_stat_stat_cfg_oam_stat_lm_fc_cnt_t
{
    ZXIC_UINT32 oam_stat_lm_fc_cnt;
}DPP_STAT_STAT_CFG_OAM_STAT_LM_FC_CNT_T;

typedef struct dpp_stat_stat_cfg_stat_oam_fc_cnt_t
{
    ZXIC_UINT32 stat_oam_fc_cnt;
}DPP_STAT_STAT_CFG_STAT_OAM_FC_CNT_T;

typedef struct dpp_stat_stat_cfg_cmmu_stat_fc_cnt_t
{
    ZXIC_UINT32 cmmu_stat_fc_cnt;
}DPP_STAT_STAT_CFG_CMMU_STAT_FC_CNT_T;

typedef struct dpp_stat_stat_cfg_stat_cmmu_req_cnt_t
{
    ZXIC_UINT32 stat_cmmu_req_cnt;
}DPP_STAT_STAT_CFG_STAT_CMMU_REQ_CNT_T;

typedef struct dpp_stat_stat_cfg_smmu0_plcr_rsp0_cnt_t
{
    ZXIC_UINT32 smmu0_plcr_rsp0_cnt;
}DPP_STAT_STAT_CFG_SMMU0_PLCR_RSP0_CNT_T;

typedef struct dpp_stat_stat_cfg_plcr_smmu0_req0_cnt_t
{
    ZXIC_UINT32 plcr_smmu0_req0_cnt;
}DPP_STAT_STAT_CFG_PLCR_SMMU0_REQ0_CNT_T;

typedef struct dpp_stat_stat_cfg_stat_oam_lm_rsp_cnt_t
{
    ZXIC_UINT32 stat_oam_lm_rsp_cnt;
}DPP_STAT_STAT_CFG_STAT_OAM_LM_RSP_CNT_T;

typedef struct dpp_stat_stat_cfg_oam_stat_lm_req_cnt_t
{
    ZXIC_UINT32 oam_stat_lm_req_cnt;
}DPP_STAT_STAT_CFG_OAM_STAT_LM_REQ_CNT_T;

typedef struct dpp_stat_stat_cfg_oam_stat_req_cnt_t
{
    ZXIC_UINT32 oam_stat_req_cnt;
}DPP_STAT_STAT_CFG_OAM_STAT_REQ_CNT_T;

typedef struct dpp_stat_stat_cfg_se_etm_stat_rsp_cnt_t
{
    ZXIC_UINT32 se_etm_stat_rsp_cnt;
}DPP_STAT_STAT_CFG_SE_ETM_STAT_RSP_CNT_T;

typedef struct dpp_stat_stat_cfg_etm_stat_se_wr_req_cnt_t
{
    ZXIC_UINT32 etm_stat_se_wr_req_cnt;
}DPP_STAT_STAT_CFG_ETM_STAT_SE_WR_REQ_CNT_T;

typedef struct dpp_stat_stat_cfg_etm_stat_se_rd_req_cnt_t
{
    ZXIC_UINT32 etm_stat_se_rd_req_cnt;
}DPP_STAT_STAT_CFG_ETM_STAT_SE_RD_REQ_CNT_T;

typedef struct dpp_stat_stat_cfg_etm_stat_smmu0_req_cnt0_t
{
    ZXIC_UINT32 etm_stat_smmu0_req_cnt0;
}DPP_STAT_STAT_CFG_ETM_STAT_SMMU0_REQ_CNT0_T;

typedef struct dpp_stat_stat_cfg_etm_stat_smmu0_req_cnt1_t
{
    ZXIC_UINT32 etm_stat_smmu0_req_cnt1;
}DPP_STAT_STAT_CFG_ETM_STAT_SMMU0_REQ_CNT1_T;

typedef struct dpp_stat_stat_cfg_tm_stat_eram_cpu_rsp_cnt_t
{
    ZXIC_UINT32 tm_stat_eram_cpu_rsp_cnt;
}DPP_STAT_STAT_CFG_TM_STAT_ERAM_CPU_RSP_CNT_T;

typedef struct dpp_stat_stat_cfg_cpu_rd_eram_req_cnt_t
{
    ZXIC_UINT32 cpu_rd_eram_req_cnt;
}DPP_STAT_STAT_CFG_CPU_RD_ERAM_REQ_CNT_T;

typedef struct dpp_stat_stat_cfg_cpu_wr_eram_req_cnt_t
{
    ZXIC_UINT32 cpu_wr_eram_req_cnt;
}DPP_STAT_STAT_CFG_CPU_WR_ERAM_REQ_CNT_T;

typedef struct dpp_stat_stat_cfg_tm_stat_ddr_cpu_rsp_cnt_t
{
    ZXIC_UINT32 tm_stat_ddr_cpu_rsp_cnt;
}DPP_STAT_STAT_CFG_TM_STAT_DDR_CPU_RSP_CNT_T;

typedef struct dpp_stat_stat_cfg_cpu_rd_ddr_req_cnt_t
{
    ZXIC_UINT32 cpu_rd_ddr_req_cnt;
}DPP_STAT_STAT_CFG_CPU_RD_DDR_REQ_CNT_T;

typedef struct dpp_stat_stat_cfg_cpu_wr_ddr_req_cnt_t
{
    ZXIC_UINT32 cpu_wr_ddr_req_cnt;
}DPP_STAT_STAT_CFG_CPU_WR_DDR_REQ_CNT_T;

typedef struct dpp_stat_etcam_cpu_ind_wdat1_t
{
    ZXIC_UINT32 wdat1;
}DPP_STAT_ETCAM_CPU_IND_WDAT1_T;

typedef struct dpp_stat_etcam_cpu_ind_wdat2_t
{
    ZXIC_UINT32 wdat2;
}DPP_STAT_ETCAM_CPU_IND_WDAT2_T;

typedef struct dpp_stat_etcam_cpu_ind_wdat3_t
{
    ZXIC_UINT32 wdat3;
}DPP_STAT_ETCAM_CPU_IND_WDAT3_T;

typedef struct dpp_stat_etcam_cpu_ind_wdat4_t
{
    ZXIC_UINT32 wdat4;
}DPP_STAT_ETCAM_CPU_IND_WDAT4_T;

typedef struct dpp_stat_etcam_cpu_ind_wdat5_t
{
    ZXIC_UINT32 wdat5;
}DPP_STAT_ETCAM_CPU_IND_WDAT5_T;

typedef struct dpp_stat_etcam_cpu_ind_wdat6_t
{
    ZXIC_UINT32 wdat6;
}DPP_STAT_ETCAM_CPU_IND_WDAT6_T;

typedef struct dpp_stat_etcam_cpu_ind_wdat7_t
{
    ZXIC_UINT32 wdat7;
}DPP_STAT_ETCAM_CPU_IND_WDAT7_T;

typedef struct dpp_stat_etcam_cpu_ind_wdat8_t
{
    ZXIC_UINT32 wdat8;
}DPP_STAT_ETCAM_CPU_IND_WDAT8_T;

typedef struct dpp_stat_etcam_cpu_ind_wdat9_t
{
    ZXIC_UINT32 wdat9;
}DPP_STAT_ETCAM_CPU_IND_WDAT9_T;

typedef struct dpp_stat_etcam_cpu_ind_wdat10_t
{
    ZXIC_UINT32 wdat10;
}DPP_STAT_ETCAM_CPU_IND_WDAT10_T;

typedef struct dpp_stat_etcam_cpu_ind_wdat11_t
{
    ZXIC_UINT32 wdat11;
}DPP_STAT_ETCAM_CPU_IND_WDAT11_T;

typedef struct dpp_stat_etcam_cpu_ind_wdat12_t
{
    ZXIC_UINT32 wdat12;
}DPP_STAT_ETCAM_CPU_IND_WDAT12_T;

typedef struct dpp_stat_etcam_cpu_ind_wdat13_t
{
    ZXIC_UINT32 wdat13;
}DPP_STAT_ETCAM_CPU_IND_WDAT13_T;

typedef struct dpp_stat_etcam_cpu_ind_wdat14_t
{
    ZXIC_UINT32 wdat14;
}DPP_STAT_ETCAM_CPU_IND_WDAT14_T;

typedef struct dpp_stat_etcam_cpu_ind_wdat15_t
{
    ZXIC_UINT32 wdat15;
}DPP_STAT_ETCAM_CPU_IND_WDAT15_T;

typedef struct dpp_stat_etcam_cpu_ind_wdat16_t
{
    ZXIC_UINT32 wdat16;
}DPP_STAT_ETCAM_CPU_IND_WDAT16_T;

typedef struct dpp_stat_etcam_cpu_ind_wdat17_t
{
    ZXIC_UINT32 wdat17;
}DPP_STAT_ETCAM_CPU_IND_WDAT17_T;

typedef struct dpp_stat_etcam_cpu_ind_wdat18_t
{
    ZXIC_UINT32 wdat18;
}DPP_STAT_ETCAM_CPU_IND_WDAT18_T;

typedef struct dpp_stat_etcam_cpu_ind_wdat19_t
{
    ZXIC_UINT32 wdat19;
}DPP_STAT_ETCAM_CPU_IND_WDAT19_T;

typedef struct dpp_stat_etcam_t_strwc_cfg_t
{
    ZXIC_UINT32 t_strwc_cfg;
}DPP_STAT_ETCAM_T_STRWC_CFG_T;

typedef struct dpp_stat_etcam_etcam_int_unmask_flag_t
{
    ZXIC_UINT32 etcam_int_unmask_flag;
}DPP_STAT_ETCAM_ETCAM_INT_UNMASK_FLAG_T;

typedef struct dpp_stat_etcam_etcam_int_en0_t
{
    ZXIC_UINT32 etcam_int_en17;
    ZXIC_UINT32 etcam_int_en16;
    ZXIC_UINT32 etcam_int_en15;
    ZXIC_UINT32 etcam_int_en14;
    ZXIC_UINT32 etcam_int_en13;
    ZXIC_UINT32 etcam_int_en12;
    ZXIC_UINT32 etcam_int_en11;
    ZXIC_UINT32 etcam_int_en10;
    ZXIC_UINT32 etcam_int_en9;
    ZXIC_UINT32 etcam_int_en8;
    ZXIC_UINT32 etcam_int_en7;
    ZXIC_UINT32 etcam_int_en6;
    ZXIC_UINT32 etcam_int_en5;
    ZXIC_UINT32 etcam_int_en4;
    ZXIC_UINT32 etcam_int_en3;
    ZXIC_UINT32 etcam_int_en2;
    ZXIC_UINT32 etcam_int_en1;
    ZXIC_UINT32 etcam_int_en0;
}DPP_STAT_ETCAM_ETCAM_INT_EN0_T;

typedef struct dpp_stat_etcam_etcam_int_mask0_t
{
    ZXIC_UINT32 etcam_int_mask17;
    ZXIC_UINT32 etcam_int_mask16;
    ZXIC_UINT32 etcam_int_mask15;
    ZXIC_UINT32 etcam_int_mask14;
    ZXIC_UINT32 etcam_int_mask13;
    ZXIC_UINT32 etcam_int_mask12;
    ZXIC_UINT32 etcam_int_mask11;
    ZXIC_UINT32 etcam_int_mask10;
    ZXIC_UINT32 etcam_int_mask9;
    ZXIC_UINT32 etcam_int_mask8;
    ZXIC_UINT32 etcam_int_mask7;
    ZXIC_UINT32 etcam_int_mask6;
    ZXIC_UINT32 etcam_int_mask5;
    ZXIC_UINT32 etcam_int_mask4;
    ZXIC_UINT32 etcam_int_mask3;
    ZXIC_UINT32 etcam_int_mask2;
    ZXIC_UINT32 etcam_int_mask1;
    ZXIC_UINT32 etcam_int_mask0;
}DPP_STAT_ETCAM_ETCAM_INT_MASK0_T;

typedef struct dpp_stat_etcam_etcam_int_status_t
{
    ZXIC_UINT32 etcam_int_status17;
    ZXIC_UINT32 etcam_int_status16;
    ZXIC_UINT32 etcam_int_status15;
    ZXIC_UINT32 etcam_int_status14;
    ZXIC_UINT32 etcam_int_status13;
    ZXIC_UINT32 etcam_int_status12;
    ZXIC_UINT32 etcam_int_status11;
    ZXIC_UINT32 etcam_int_status10;
    ZXIC_UINT32 etcam_int_status9;
    ZXIC_UINT32 etcam_int_status8;
    ZXIC_UINT32 etcam_int_status7;
    ZXIC_UINT32 etcam_int_status6;
    ZXIC_UINT32 etcam_int_status5;
    ZXIC_UINT32 etcam_int_status4;
    ZXIC_UINT32 etcam_int_status3;
    ZXIC_UINT32 etcam_int_status2;
    ZXIC_UINT32 etcam_int_status1;
    ZXIC_UINT32 etcam_int_status0;
}DPP_STAT_ETCAM_ETCAM_INT_STATUS_T;

typedef struct dpp_stat_etcam_int_tb_ini_ok_t
{
    ZXIC_UINT32 int_tb_ini_ok;
}DPP_STAT_ETCAM_INT_TB_INI_OK_T;

typedef struct dpp_stat_etcam_etcam_clk_en_t
{
    ZXIC_UINT32 etcam_clk_en;
}DPP_STAT_ETCAM_ETCAM_CLK_EN_T;

typedef struct dpp_stat_etcam_as_etcam_req0_cnt_t
{
    ZXIC_UINT32 as_etcam_req0_cnt;
}DPP_STAT_ETCAM_AS_ETCAM_REQ0_CNT_T;

typedef struct dpp_stat_etcam_as_etcam_req1_cnt_t
{
    ZXIC_UINT32 as_etcam_req1_cnt;
}DPP_STAT_ETCAM_AS_ETCAM_REQ1_CNT_T;

typedef struct dpp_stat_etcam_etcam_as_index0_cnt_t
{
    ZXIC_UINT32 etcam_as_index0_cnt;
}DPP_STAT_ETCAM_ETCAM_AS_INDEX0_CNT_T;

typedef struct dpp_stat_etcam_etcam_as_index1_cnt_t
{
    ZXIC_UINT32 etcam_as_index1_cnt;
}DPP_STAT_ETCAM_ETCAM_AS_INDEX1_CNT_T;

typedef struct dpp_stat_etcam_etcam_not_hit0_cnt_t
{
    ZXIC_UINT32 etcam_not_hit0_cnt;
}DPP_STAT_ETCAM_ETCAM_NOT_HIT0_CNT_T;

typedef struct dpp_stat_etcam_etcam_not_hit1_cnt_t
{
    ZXIC_UINT32 etcam_not_hit1_cnt;
}DPP_STAT_ETCAM_ETCAM_NOT_HIT1_CNT_T;

typedef struct dpp_stat_etcam_table_id_not_match_cnt_t
{
    ZXIC_UINT32 table_id_not_match_cnt;
}DPP_STAT_ETCAM_TABLE_ID_NOT_MATCH_CNT_T;

typedef struct dpp_stat_etcam_table_id_clash01_cnt_t
{
    ZXIC_UINT32 table_id_clash01_cnt;
}DPP_STAT_ETCAM_TABLE_ID_CLASH01_CNT_T;

typedef struct dpp_stat_etcam_etcam_cpu_fl_t
{
    ZXIC_UINT32 etcam_cpu_fl;
}DPP_STAT_ETCAM_ETCAM_CPU_FL_T;

typedef struct dpp_stat_etcam_etcam_arb_empty_t
{
    ZXIC_UINT32 etcam_arb_empty;
}DPP_STAT_ETCAM_ETCAM_ARB_EMPTY_T;


#ifdef __cplusplus
}
#endif
#endif

