
#ifndef _DPP_TRPG_REG_H_
#define _DPP_TRPG_REG_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef struct dpp_trpg_trpg_rx_port_cpu_trpg_ms_en_t
{
    ZXIC_UINT32 cpu_trpgrx_ms_en;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPG_MS_EN_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpg_port_en_t
{
    ZXIC_UINT32 cpu_trpgrx_port_en;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPG_PORT_EN_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpg_look_en_t
{
    ZXIC_UINT32 cpu_trpgrx_look_en;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPG_LOOK_EN_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_ram_almost_full_t
{
    ZXIC_UINT32 cpu_trpgrx_ram_almost_full;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_RAM_ALMOST_FULL_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_ram_test_en_t
{
    ZXIC_UINT32 cpu_trpgrx_ram_test_en;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_RAM_TEST_EN_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_inmod_pfc_rdy_en_t
{
    ZXIC_UINT32 cpu_trpgrx_inmod_pfc_rdy_en;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_INMOD_PFC_RDY_EN_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_pkt_num_h_t
{
    ZXIC_UINT32 cpu_trpgrx_pkt_num_h;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_PKT_NUM_H_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_pkt_num_l_t
{
    ZXIC_UINT32 cpu_trpgrx_pkt_num_l;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_PKT_NUM_L_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_pkt_byte_num_h_t
{
    ZXIC_UINT32 cpu_trpgrx_pkt_byte_num_h;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_PKT_BYTE_NUM_H_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_pkt_byte_num_l_t
{
    ZXIC_UINT32 cpu_trpgrx_pkt_byte_num_l;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_PKT_BYTE_NUM_L_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_pkt_cnt_clr_t
{
    ZXIC_UINT32 cpu_trpgrx_pkt_cnt_clr;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_PKT_CNT_CLR_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_fc_clk_freq_t
{
    ZXIC_UINT32 cpu_trpgrx_fc_clk_freq;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_FC_CLK_FREQ_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_fc_en_t
{
    ZXIC_UINT32 cpu_trpgrx_fc_en;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_FC_EN_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_fc_token_add_num_t
{
    ZXIC_UINT32 cpu_trpgrx_fc_token_add_num;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_FC_TOKEN_ADD_NUM_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_fc_token_max_num_t
{
    ZXIC_UINT32 cpu_trpgrx_fc_token_max_num;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_FC_TOKEN_MAX_NUM_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_port_state_info_t
{
    ZXIC_UINT32 cpu_trpgrx_port_state_info;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_PORT_STATE_INFO_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_ram_past_max_dep_t
{
    ZXIC_UINT32 cpu_trpgrx_ram_past_max_dep;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_RAM_PAST_MAX_DEP_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_ram_past_max_dep_clr_t
{
    ZXIC_UINT32 cpu_trpgrx_ram_past_max_dep_clr;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_RAM_PAST_MAX_DEP_CLR_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_pkt_past_max_len_t
{
    ZXIC_UINT32 cpu_trpgrx_pkt_past_max_len;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_PKT_PAST_MAX_LEN_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_pkt_past_max_len_clr_t
{
    ZXIC_UINT32 cpu_trpgrx_pkt_past_max_len_clr;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_PKT_PAST_MAX_LEN_CLR_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_pkt_past_min_len_t
{
    ZXIC_UINT32 cpu_trpgrx_pkt_past_min_len;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_PKT_PAST_MIN_LEN_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_pkt_past_min_len_clr_t
{
    ZXIC_UINT32 cpu_trpgrx_pkt_past_min_len_clr;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_PKT_PAST_MIN_LEN_CLR_T;

typedef struct dpp_trpg_trpg_rx_ram_trpg_rx_data_ram_t
{
    ZXIC_UINT32 trpg_rx_data_ram;
}DPP_TRPG_TRPG_RX_RAM_TRPG_RX_DATA_RAM_T;

typedef struct dpp_trpg_trpg_rx_ram_trpg_rx_info_ram_t
{
    ZXIC_UINT32 trpg_rx_info_ram;
}DPP_TRPG_TRPG_RX_RAM_TRPG_RX_INFO_RAM_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpg_ms_en_t
{
    ZXIC_UINT32 cpu_trpgtx_ms_en;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPG_MS_EN_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpg_port_en_t
{
    ZXIC_UINT32 cpu_trpgtx_port_en;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPG_PORT_EN_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpg_look_en_t
{
    ZXIC_UINT32 cpu_trpgtx_look_en;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPG_LOOK_EN_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpgtx_ram_almost_full_t
{
    ZXIC_UINT32 cpu_trpgtx_ram_almost_full;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPGTX_RAM_ALMOST_FULL_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpgtx_ram_test_en_t
{
    ZXIC_UINT32 cpu_trpgtx_ram_test_en;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPGTX_RAM_TEST_EN_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpgtx_pkt_num_h_t
{
    ZXIC_UINT32 cpu_trpgtx_pkt_num_h;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPGTX_PKT_NUM_H_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpgtx_pkt_num_l_t
{
    ZXIC_UINT32 cpu_trpgtx_pkt_num_l;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPGTX_PKT_NUM_L_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpgtx_pkt_byte_num_h_t
{
    ZXIC_UINT32 cpu_trpgtx_pkt_byte_num_h;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPGTX_PKT_BYTE_NUM_H_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpgtx_pkt_byte_num_l_t
{
    ZXIC_UINT32 cpu_trpgtx_pkt_byte_num_l;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPGTX_PKT_BYTE_NUM_L_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpgtx_pkt_cnt_clr_t
{
    ZXIC_UINT32 cpu_trpgtx_pkt_cnt_clr;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPGTX_PKT_CNT_CLR_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpgtx_fc_clk_freq_t
{
    ZXIC_UINT32 cpu_trpgtx_fc_clk_freq;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPGTX_FC_CLK_FREQ_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpgtx_fc_en_t
{
    ZXIC_UINT32 cpu_trpgtx_fc_en;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPGTX_FC_EN_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpgtx_fc_token_add_num_t
{
    ZXIC_UINT32 cpu_trpgtx_fc_token_add_num;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPGTX_FC_TOKEN_ADD_NUM_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpgtx_fc_token_max_num_t
{
    ZXIC_UINT32 cpu_trpgtx_fc_token_max_num;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPGTX_FC_TOKEN_MAX_NUM_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpgtx_port_state_info_t
{
    ZXIC_UINT32 cpu_trpgtx_port_state_info;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPGTX_PORT_STATE_INFO_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpgtx_ram_past_max_dep_t
{
    ZXIC_UINT32 cpu_trpgtx_ram_past_max_dep;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPGTX_RAM_PAST_MAX_DEP_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpgtx_ram_past_max_dep_clr_t
{
    ZXIC_UINT32 cpu_trpgtx_ram_past_max_dep_clr;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPGTX_RAM_PAST_MAX_DEP_CLR_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpgtx_pkt_past_max_len_t
{
    ZXIC_UINT32 cpu_trpgtx_pkt_past_max_len;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPGTX_PKT_PAST_MAX_LEN_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpgtx_pkt_past_max_len_clr_t
{
    ZXIC_UINT32 cpu_trpgtx_pkt_past_max_len_clr;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPGTX_PKT_PAST_MAX_LEN_CLR_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpgtx_pkt_past_min_len_t
{
    ZXIC_UINT32 cpu_trpgtx_pkt_past_min_len;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPGTX_PKT_PAST_MIN_LEN_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpgtx_pkt_past_min_len_clr_t
{
    ZXIC_UINT32 cpu_trpgtx_pkt_past_min_len_clr;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPGTX_PKT_PAST_MIN_LEN_CLR_T;

typedef struct dpp_trpg_trpg_tx_etm_port_cpu_trpgtx_etm_ram_almost_full_t
{
    ZXIC_UINT32 cpu_trpgtx_etm_ram_almost_full;
}DPP_TRPG_TRPG_TX_ETM_PORT_CPU_TRPGTX_ETM_RAM_ALMOST_FULL_T;

typedef struct dpp_trpg_trpg_tx_etm_port_cpu_trpgtx_etm_ram_test_en_t
{
    ZXIC_UINT32 cpu_trpgtx_etm_ram_test_en;
}DPP_TRPG_TRPG_TX_ETM_PORT_CPU_TRPGTX_ETM_RAM_TEST_EN_T;

typedef struct dpp_trpg_trpg_tx_glb_cpu_todtime_update_int_mask_t
{
    ZXIC_UINT32 cpu_todtime_update_int_mask;
}DPP_TRPG_TRPG_TX_GLB_CPU_TODTIME_UPDATE_INT_MASK_T;

typedef struct dpp_trpg_trpg_tx_glb_cpu_todtime_update_int_clr_t
{
    ZXIC_UINT32 cpu_todtime_update_int_clr;
}DPP_TRPG_TRPG_TX_GLB_CPU_TODTIME_UPDATE_INT_CLR_T;

typedef struct dpp_trpg_trpg_tx_glb_cpu_todtime_ram_test_en_t
{
    ZXIC_UINT32 cpu_todtime_ram_test_en;
}DPP_TRPG_TRPG_TX_GLB_CPU_TODTIME_RAM_TEST_EN_T;

typedef struct dpp_trpg_trpg_tx_ram_trpg_tx_data_ram_t
{
    ZXIC_UINT32 trpg_tx_data_ram;
}DPP_TRPG_TRPG_TX_RAM_TRPG_TX_DATA_RAM_T;

typedef struct dpp_trpg_trpg_tx_ram_trpg_tx_info_ram_t
{
    ZXIC_UINT32 trpg_tx_info_ram;
}DPP_TRPG_TRPG_TX_RAM_TRPG_TX_INFO_RAM_T;

typedef struct dpp_trpg_trpg_tx_etm_ram_trpg_tx_etm_data_ram_t
{
    ZXIC_UINT32 trpg_tx_etm_data_ram;
}DPP_TRPG_TRPG_TX_ETM_RAM_TRPG_TX_ETM_DATA_RAM_T;

typedef struct dpp_trpg_trpg_tx_etm_ram_trpg_tx_etm_info_ram_t
{
    ZXIC_UINT32 trpg_tx_etm_info_ram;
}DPP_TRPG_TRPG_TX_ETM_RAM_TRPG_TX_ETM_INFO_RAM_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpg_ms_st_t
{
    ZXIC_UINT32 cpu_trpgrx_ms_st;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPG_MS_ST_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpg_ms_ind_t
{
    ZXIC_UINT32 cpu_trpgrx_ms_ind;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPG_MS_IND_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpg_ms_slave_ind_t
{
    ZXIC_UINT32 cpu_trpgrx_ms_slave_ind;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPG_MS_SLAVE_IND_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_up_water_level_t
{
    ZXIC_UINT32 cpu_trpgrx_up_water_level;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_UP_WATER_LEVEL_T;

typedef struct dpp_trpg_trpg_rx_port_cpu_trpgrx_low_water_level_t
{
    ZXIC_UINT32 cpu_trpgrx_low_water_level;
}DPP_TRPG_TRPG_RX_PORT_CPU_TRPGRX_LOW_WATER_LEVEL_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpg_ms_st_t
{
    ZXIC_UINT32 cpu_trpgtx_ms_st;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPG_MS_ST_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpg_ms_ind_t
{
    ZXIC_UINT32 cpu_trpgtx_ms_ind;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPG_MS_IND_T;

typedef struct dpp_trpg_trpg_tx_port_cpu_trpg_ms_slave_ind_t
{
    ZXIC_UINT32 cpu_trpgtx_ms_slave_ind;
}DPP_TRPG_TRPG_TX_PORT_CPU_TRPG_MS_SLAVE_IND_T;

typedef struct dpp_trpg_trpg_tx_glb_cpu_todtime_update_int_event_t
{
    ZXIC_UINT32 cpu_todtime_update_int_event;
}DPP_TRPG_TRPG_TX_GLB_CPU_TODTIME_UPDATE_INT_EVENT_T;

typedef struct dpp_trpg_trpg_tx_glb_cpu_todtime_update_int_test_t
{
    ZXIC_UINT32 cpu_todtime_update_int_test;
}DPP_TRPG_TRPG_TX_GLB_CPU_TODTIME_UPDATE_INT_TEST_T;

typedef struct dpp_trpg_trpg_tx_glb_cpu_todtime_update_int_addr_t
{
    ZXIC_UINT32 cpu_todtime_update_int_addr;
}DPP_TRPG_TRPG_TX_GLB_CPU_TODTIME_UPDATE_INT_ADDR_T;

typedef struct dpp_trpg_trpg_tx_todtime_ram_trpg_tx_todtime_ram_t
{
    ZXIC_UINT32 trpg_tx_todtime_ram;
}DPP_TRPG_TRPG_TX_TODTIME_RAM_TRPG_TX_TODTIME_RAM_T;


#ifdef __cplusplus
}
#endif
#endif

