
#ifndef _DPP_TSN_REG_H_
#define _DPP_TSN_REG_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef struct dpp_tsn_tsn_port_cfg_tsn_test_reg_t
{
    ZXIC_UINT32 cfg_tsn_test_reg;
}DPP_TSN_TSN_PORT_CFG_TSN_TEST_REG_T;

typedef struct dpp_tsn_tsn_port_cfg_tsn_port_qbv_enable_t
{
    ZXIC_UINT32 cfg_tsn_port_qbv_enable;
}DPP_TSN_TSN_PORT_CFG_TSN_PORT_QBV_ENABLE_T;

typedef struct dpp_tsn_tsn_port_cfg_tsn_phy_port_sel_t
{
    ZXIC_UINT32 cfg_tsn_phy_port_sel;
}DPP_TSN_TSN_PORT_CFG_TSN_PHY_PORT_SEL_T;

typedef struct dpp_tsn_tsn_port_cfg_tsn_port_time_sel_t
{
    ZXIC_UINT32 cfg_tsn_port_time_sel;
}DPP_TSN_TSN_PORT_CFG_TSN_PORT_TIME_SEL_T;

typedef struct dpp_tsn_tsn_port_cfg_tsn_clk_freq_t
{
    ZXIC_UINT32 en;
    ZXIC_UINT32 cfg_tsn_clk_freq;
}DPP_TSN_TSN_PORT_CFG_TSN_CLK_FREQ_T;

typedef struct dpp_tsn_tsn_port_cfg_tsn_read_ram_n_t
{
    ZXIC_UINT32 cfg_tsn_data;
    ZXIC_UINT32 cfg_tsn_read_status;
    ZXIC_UINT32 cfg_tsn_read_ram_n;
}DPP_TSN_TSN_PORT_CFG_TSN_READ_RAM_N_T;

typedef struct dpp_tsn_tsn_port_cfg_tsn_exe_time_t
{
    ZXIC_UINT32 cfg_tsn_exe_time;
}DPP_TSN_TSN_PORT_CFG_TSN_EXE_TIME_T;

typedef struct dpp_tsn_tsn_port_cfg_tsn_port_itr_shift_t
{
    ZXIC_UINT32 cfg_tsn_port_itr_shift;
}DPP_TSN_TSN_PORT_CFG_TSN_PORT_ITR_SHIFT_T;

typedef struct dpp_tsn_tsn_port_cfg_tsn_port_base_time_h_t
{
    ZXIC_UINT32 cfg_tsn_port_base_time_h;
}DPP_TSN_TSN_PORT_CFG_TSN_PORT_BASE_TIME_H_T;

typedef struct dpp_tsn_tsn_port_cfg_tsn_port_base_time_l_t
{
    ZXIC_UINT32 cfg_tsn_port_base_time_l;
}DPP_TSN_TSN_PORT_CFG_TSN_PORT_BASE_TIME_L_T;

typedef struct dpp_tsn_tsn_port_cfg_tsn_port_cycle_time_h_t
{
    ZXIC_UINT32 cfg_tsn_port_cycle_time_h;
}DPP_TSN_TSN_PORT_CFG_TSN_PORT_CYCLE_TIME_H_T;

typedef struct dpp_tsn_tsn_port_cfg_tsn_port_cycle_time_l_t
{
    ZXIC_UINT32 cfg_tsn_port_cycle_time_l;
}DPP_TSN_TSN_PORT_CFG_TSN_PORT_CYCLE_TIME_L_T;

typedef struct dpp_tsn_tsn_port_cfg_tsn_port_guard_band_time_t
{
    ZXIC_UINT32 cfg_tsn_port_guard_band_time;
}DPP_TSN_TSN_PORT_CFG_TSN_PORT_GUARD_BAND_TIME_T;

typedef struct dpp_tsn_tsn_port_cfg_tsn_port_default_gate_en_t
{
    ZXIC_UINT32 cfg_tsn_port_default_gate_en;
}DPP_TSN_TSN_PORT_CFG_TSN_PORT_DEFAULT_GATE_EN_T;

typedef struct dpp_tsn_tsn_port_cfg_tsn_port_change_gate_en_t
{
    ZXIC_UINT32 cfg_tsn_port_change_gate_en;
}DPP_TSN_TSN_PORT_CFG_TSN_PORT_CHANGE_GATE_EN_T;

typedef struct dpp_tsn_tsn_port_cfg_tsn_port_init_finish_t
{
    ZXIC_UINT32 cfg_tsn_port_init_finish;
}DPP_TSN_TSN_PORT_CFG_TSN_PORT_INIT_FINISH_T;

typedef struct dpp_tsn_tsn_port_cfg_tsn_port_change_en_t
{
    ZXIC_UINT32 cfg_tsn_port_change_en;
}DPP_TSN_TSN_PORT_CFG_TSN_PORT_CHANGE_EN_T;

typedef struct dpp_tsn_tsn_port_cfg_tsn_port_gcl_num0_t
{
    ZXIC_UINT32 cfg_tsn_port_gcl_num0;
}DPP_TSN_TSN_PORT_CFG_TSN_PORT_GCL_NUM0_T;

typedef struct dpp_tsn_tsn_port_cfg_tsn_port_gcl_num1_t
{
    ZXIC_UINT32 cfg_tsn_port_gcl_num1;
}DPP_TSN_TSN_PORT_CFG_TSN_PORT_GCL_NUM1_T;

typedef struct dpp_tsn_tsn_port_cfg_tsn_port_gcl_value0_t
{
    ZXIC_UINT32 cfg_tsn_port_gcl_gate_control0;
    ZXIC_UINT32 cfg_tsn_port_gcl_interval_time0;
}DPP_TSN_TSN_PORT_CFG_TSN_PORT_GCL_VALUE0_T;

typedef struct dpp_tsn_tsn_port_cfg_tsn_port_gcl_value1_t
{
    ZXIC_UINT32 cfg_tsn_port_gcl_gate_control1;
    ZXIC_UINT32 cfg_tsn_port_gcl_interval_time1;
}DPP_TSN_TSN_PORT_CFG_TSN_PORT_GCL_VALUE1_T;


#ifdef __cplusplus
}
#endif
#endif

