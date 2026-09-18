
#ifndef _DPP_AXI_REG_H_
#define _DPP_AXI_REG_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef struct dpp_axi_axi_conv_cfg_epid_v_func_num_t
{
    ZXIC_UINT32 user_en;
    ZXIC_UINT32 cfg_epid;
    ZXIC_UINT32 cfg_vfunc_num;
    ZXIC_UINT32 cfg_func_num;
    ZXIC_UINT32 cfg_vfunc_active;
}DPP_AXI_AXI_CONV_CFG_EPID_V_FUNC_NUM_T;

typedef struct dpp_axi_axi_conv_info_axim_rw_hsk_cnt_t
{
    ZXIC_UINT32 axim_rd_handshake_cnt;
    ZXIC_UINT32 axim_wr_handshake_cnt;
}DPP_AXI_AXI_CONV_INFO_AXIM_RW_HSK_CNT_T;

typedef struct dpp_axi_axi_conv_info_axim_last_wr_id_t
{
    ZXIC_UINT32 axim_rd_id;
    ZXIC_UINT32 axim_wr_id;
}DPP_AXI_AXI_CONV_INFO_AXIM_LAST_WR_ID_T;

typedef struct dpp_axi_axi_conv_info_axim_last_wr_addr_h_t
{
    ZXIC_UINT32 aximlastwraddrhigh;
}DPP_AXI_AXI_CONV_INFO_AXIM_LAST_WR_ADDR_H_T;

typedef struct dpp_axi_axi_conv_info_axim_last_wr_addr_l_t
{
    ZXIC_UINT32 aximlastrdaddrlow;
}DPP_AXI_AXI_CONV_INFO_AXIM_LAST_WR_ADDR_L_T;

typedef struct dpp_axi_axi_conv_cfg_debug_info_clr_en_t
{
    ZXIC_UINT32 cfg_global_clr_en;
}DPP_AXI_AXI_CONV_CFG_DEBUG_INFO_CLR_EN_T;


#ifdef __cplusplus
}
#endif
#endif

