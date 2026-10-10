
#ifndef _DPP_CFG_REG_H_
#define _DPP_CFG_REG_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef struct dpp_cfg_pcie_int_repeat_t
{
    ZXIC_UINT32 int_repeat;
}DPP_CFG_PCIE_INT_REPEAT_T;

typedef struct dpp_cfg_dma_dma_up_size_t
{
    ZXIC_UINT32 dma_up_size;
}DPP_CFG_DMA_DMA_UP_SIZE_T;

typedef struct dpp_cfg_csr_soc_wr_time_out_thresh_t
{
    ZXIC_UINT32 soc_wr_time_out_thresh;
}DPP_CFG_CSR_SOC_WR_TIME_OUT_THRESH_T;

typedef struct dpp_cfg_pcie_pcie_ddr_switch_t
{
    ZXIC_UINT32 pcie_ddr_switch;
}DPP_CFG_PCIE_PCIE_DDR_SWITCH_T;

typedef struct dpp_cfg_pcie_user0_int_en_t
{
    ZXIC_UINT32 user_int_en;
}DPP_CFG_PCIE_USER0_INT_EN_T;

typedef struct dpp_cfg_pcie_user0_int_mask_t
{
    ZXIC_UINT32 user_int_mask;
}DPP_CFG_PCIE_USER0_INT_MASK_T;

typedef struct dpp_cfg_pcie_user0_int_status_t
{
    ZXIC_UINT32 user_int_status;
}DPP_CFG_PCIE_USER0_INT_STATUS_T;

typedef struct dpp_cfg_pcie_user1_int_en_t
{
    ZXIC_UINT32 user_int_en;
}DPP_CFG_PCIE_USER1_INT_EN_T;

typedef struct dpp_cfg_pcie_user1_int_mask_t
{
    ZXIC_UINT32 user_int_mask;
}DPP_CFG_PCIE_USER1_INT_MASK_T;

typedef struct dpp_cfg_pcie_user1_int_status_t
{
    ZXIC_UINT32 user_int_status;
}DPP_CFG_PCIE_USER1_INT_STATUS_T;

typedef struct dpp_cfg_pcie_user2_int_en_t
{
    ZXIC_UINT32 user_int_en;
}DPP_CFG_PCIE_USER2_INT_EN_T;

typedef struct dpp_cfg_pcie_user2_int_mask_t
{
    ZXIC_UINT32 user_int_mask;
}DPP_CFG_PCIE_USER2_INT_MASK_T;

typedef struct dpp_cfg_pcie_user2_int_status_t
{
    ZXIC_UINT32 user_int_status;
}DPP_CFG_PCIE_USER2_INT_STATUS_T;

typedef struct dpp_cfg_pcie_ecc_1b_int_en_t
{
    ZXIC_UINT32 ecc_1b_int_en;
}DPP_CFG_PCIE_ECC_1B_INT_EN_T;

typedef struct dpp_cfg_pcie_ecc_1b_int_mask_t
{
    ZXIC_UINT32 ecc_1b_int_mask;
}DPP_CFG_PCIE_ECC_1B_INT_MASK_T;

typedef struct dpp_cfg_pcie_ecc_1b_int_status_t
{
    ZXIC_UINT32 ecc_1b_int_status;
}DPP_CFG_PCIE_ECC_1B_INT_STATUS_T;

typedef struct dpp_cfg_pcie_ecc_2b_int_en_t
{
    ZXIC_UINT32 ecc_2b_int_en;
}DPP_CFG_PCIE_ECC_2B_INT_EN_T;

typedef struct dpp_cfg_pcie_ecc_2b_int_mask_t
{
    ZXIC_UINT32 ecc_2b_int_mask;
}DPP_CFG_PCIE_ECC_2B_INT_MASK_T;

typedef struct dpp_cfg_pcie_ecc_2b_int_status_t
{
    ZXIC_UINT32 ecc_2b_int_status;
}DPP_CFG_PCIE_ECC_2B_INT_STATUS_T;

typedef struct dpp_cfg_pcie_cfg_int_status_t
{
    ZXIC_UINT32 cfg_int_status;
}DPP_CFG_PCIE_CFG_INT_STATUS_T;

typedef struct dpp_cfg_pcie_i_core_to_cntl_t
{
    ZXIC_UINT32 i_core_to_cntl;
}DPP_CFG_PCIE_I_CORE_TO_CNTL_T;

typedef struct dpp_cfg_pcie_test_in_low_t
{
    ZXIC_UINT32 test_in_low;
}DPP_CFG_PCIE_TEST_IN_LOW_T;

typedef struct dpp_cfg_pcie_test_in_high_t
{
    ZXIC_UINT32 test_in_high;
}DPP_CFG_PCIE_TEST_IN_HIGH_T;

typedef struct dpp_cfg_pcie_local_interrupt_out_t
{
    ZXIC_UINT32 local_interrupt_out;
}DPP_CFG_PCIE_LOCAL_INTERRUPT_OUT_T;

typedef struct dpp_cfg_pcie_pl_ltssm_t
{
    ZXIC_UINT32 pl_ltssm;
}DPP_CFG_PCIE_PL_LTSSM_T;

typedef struct dpp_cfg_pcie_test_out0_t
{
    ZXIC_UINT32 test_out0;
}DPP_CFG_PCIE_TEST_OUT0_T;

typedef struct dpp_cfg_pcie_test_out1_t
{
    ZXIC_UINT32 test_out1;
}DPP_CFG_PCIE_TEST_OUT1_T;

typedef struct dpp_cfg_pcie_test_out2_t
{
    ZXIC_UINT32 test_out2;
}DPP_CFG_PCIE_TEST_OUT2_T;

typedef struct dpp_cfg_pcie_test_out3_t
{
    ZXIC_UINT32 test_out3;
}DPP_CFG_PCIE_TEST_OUT3_T;

typedef struct dpp_cfg_pcie_test_out4_t
{
    ZXIC_UINT32 test_out4;
}DPP_CFG_PCIE_TEST_OUT4_T;

typedef struct dpp_cfg_pcie_test_out5_t
{
    ZXIC_UINT32 test_out5;
}DPP_CFG_PCIE_TEST_OUT5_T;

typedef struct dpp_cfg_pcie_test_out6_t
{
    ZXIC_UINT32 test_out6;
}DPP_CFG_PCIE_TEST_OUT6_T;

typedef struct dpp_cfg_pcie_test_out7_t
{
    ZXIC_UINT32 test_out7;
}DPP_CFG_PCIE_TEST_OUT7_T;

typedef struct dpp_cfg_pcie_sync_o_core_status_t
{
    ZXIC_UINT32 sync_o_core_status;
}DPP_CFG_PCIE_SYNC_O_CORE_STATUS_T;

typedef struct dpp_cfg_pcie_sync_o_alert_dbe_t
{
    ZXIC_UINT32 sync_o_alert_dbe;
}DPP_CFG_PCIE_SYNC_O_ALERT_DBE_T;

typedef struct dpp_cfg_pcie_sync_o_alert_sbe_t
{
    ZXIC_UINT32 sync_o_alert_sbe;
}DPP_CFG_PCIE_SYNC_O_ALERT_SBE_T;

typedef struct dpp_cfg_pcie_sync_o_link_loopback_en_t
{
    ZXIC_UINT32 sync_o_link_loopback_en;
}DPP_CFG_PCIE_SYNC_O_LINK_LOOPBACK_EN_T;

typedef struct dpp_cfg_pcie_sync_o_local_fs_lf_valid_t
{
    ZXIC_UINT32 sync_o_local_fs_lf_valid;
}DPP_CFG_PCIE_SYNC_O_LOCAL_FS_LF_VALID_T;

typedef struct dpp_cfg_pcie_sync_o_rx_idle_detect_t
{
    ZXIC_UINT32 sync_o_rx_idle_detect;
}DPP_CFG_PCIE_SYNC_O_RX_IDLE_DETECT_T;

typedef struct dpp_cfg_pcie_sync_o_rx_rdy_t
{
    ZXIC_UINT32 sync_o_rx_rdy;
}DPP_CFG_PCIE_SYNC_O_RX_RDY_T;

typedef struct dpp_cfg_pcie_sync_o_tx_rdy_t
{
    ZXIC_UINT32 sync_o_tx_rdy;
}DPP_CFG_PCIE_SYNC_O_TX_RDY_T;

typedef struct dpp_cfg_pcie_pcie_link_up_cnt_t
{
    ZXIC_UINT32 pcie_link_up_cnt;
}DPP_CFG_PCIE_PCIE_LINK_UP_CNT_T;

typedef struct dpp_cfg_pcie_test_out_pcie0_t
{
    ZXIC_UINT32 test_out_pcie0;
}DPP_CFG_PCIE_TEST_OUT_PCIE0_T;

typedef struct dpp_cfg_pcie_test_out_pcie1_t
{
    ZXIC_UINT32 test_out_pcie1;
}DPP_CFG_PCIE_TEST_OUT_PCIE1_T;

typedef struct dpp_cfg_pcie_test_out_pcie2_t
{
    ZXIC_UINT32 test_out_pcie2;
}DPP_CFG_PCIE_TEST_OUT_PCIE2_T;

typedef struct dpp_cfg_pcie_test_out_pcie3_t
{
    ZXIC_UINT32 test_out_pcie3;
}DPP_CFG_PCIE_TEST_OUT_PCIE3_T;

typedef struct dpp_cfg_pcie_test_out_pcie4_t
{
    ZXIC_UINT32 test_out_pcie4;
}DPP_CFG_PCIE_TEST_OUT_PCIE4_T;

typedef struct dpp_cfg_pcie_test_out_pcie5_t
{
    ZXIC_UINT32 test_out_pcie5;
}DPP_CFG_PCIE_TEST_OUT_PCIE5_T;

typedef struct dpp_cfg_pcie_test_out_pcie6_t
{
    ZXIC_UINT32 test_out_pcie6;
}DPP_CFG_PCIE_TEST_OUT_PCIE6_T;

typedef struct dpp_cfg_pcie_test_out_pcie7_t
{
    ZXIC_UINT32 test_out_pcie7;
}DPP_CFG_PCIE_TEST_OUT_PCIE7_T;

typedef struct dpp_cfg_pcie_test_out_pcie8_t
{
    ZXIC_UINT32 test_out_pcie8;
}DPP_CFG_PCIE_TEST_OUT_PCIE8_T;

typedef struct dpp_cfg_pcie_test_out_pcie9_t
{
    ZXIC_UINT32 test_out_pcie9;
}DPP_CFG_PCIE_TEST_OUT_PCIE9_T;

typedef struct dpp_cfg_pcie_test_out_pcie10_t
{
    ZXIC_UINT32 test_out_pcie10;
}DPP_CFG_PCIE_TEST_OUT_PCIE10_T;

typedef struct dpp_cfg_pcie_test_out_pcie11_t
{
    ZXIC_UINT32 test_out_pcie11;
}DPP_CFG_PCIE_TEST_OUT_PCIE11_T;

typedef struct dpp_cfg_pcie_test_out_pcie12_t
{
    ZXIC_UINT32 test_out_pcie12;
}DPP_CFG_PCIE_TEST_OUT_PCIE12_T;

typedef struct dpp_cfg_pcie_test_out_pcie13_t
{
    ZXIC_UINT32 test_out_pcie13;
}DPP_CFG_PCIE_TEST_OUT_PCIE13_T;

typedef struct dpp_cfg_pcie_test_out_pcie14_t
{
    ZXIC_UINT32 test_out_pcie14;
}DPP_CFG_PCIE_TEST_OUT_PCIE14_T;

typedef struct dpp_cfg_pcie_test_out_pcie15_t
{
    ZXIC_UINT32 test_out_pcie15;
}DPP_CFG_PCIE_TEST_OUT_PCIE15_T;

typedef struct dpp_cfg_pcie_int_repeat_en_t
{
    ZXIC_UINT32 int_repeat_en;
}DPP_CFG_PCIE_INT_REPEAT_EN_T;

typedef struct dpp_cfg_pcie_dbg_awid_axi_mst_t
{
    ZXIC_UINT32 dbg_awid_axi_mst;
}DPP_CFG_PCIE_DBG_AWID_AXI_MST_T;

typedef struct dpp_cfg_pcie_dbg_awaddr_axi_mst0_t
{
    ZXIC_UINT32 dbg_awaddr_axi_mst0;
}DPP_CFG_PCIE_DBG_AWADDR_AXI_MST0_T;

typedef struct dpp_cfg_pcie_dbg_awaddr_axi_mst1_t
{
    ZXIC_UINT32 dbg_awaddr_axi_mst1;
}DPP_CFG_PCIE_DBG_AWADDR_AXI_MST1_T;

typedef struct dpp_cfg_pcie_dbg_awlen_axi_mst_t
{
    ZXIC_UINT32 dbg_awlen_axi_mst;
}DPP_CFG_PCIE_DBG_AWLEN_AXI_MST_T;

typedef struct dpp_cfg_pcie_dbg_awsize_axi_mst_t
{
    ZXIC_UINT32 dbg_awid_axi_mst;
}DPP_CFG_PCIE_DBG_AWSIZE_AXI_MST_T;

typedef struct dpp_cfg_pcie_dbg_awburst_axi_mst_t
{
    ZXIC_UINT32 dbg_awburst_axi_mst;
}DPP_CFG_PCIE_DBG_AWBURST_AXI_MST_T;

typedef struct dpp_cfg_pcie_dbg_awlock_axi_mst_t
{
    ZXIC_UINT32 dbg_awlock_axi_mst;
}DPP_CFG_PCIE_DBG_AWLOCK_AXI_MST_T;

typedef struct dpp_cfg_pcie_dbg_awcache_axi_mst_t
{
    ZXIC_UINT32 dbg_awcache_axi_mst;
}DPP_CFG_PCIE_DBG_AWCACHE_AXI_MST_T;

typedef struct dpp_cfg_pcie_dbg_awprot_axi_mst_t
{
    ZXIC_UINT32 dbg_awprot_axi_mst;
}DPP_CFG_PCIE_DBG_AWPROT_AXI_MST_T;

typedef struct dpp_cfg_pcie_dbg_wid_axi_mst_t
{
    ZXIC_UINT32 dbg_wid_axi_mst;
}DPP_CFG_PCIE_DBG_WID_AXI_MST_T;

typedef struct dpp_cfg_pcie_dbg_wdata_axi_mst0_t
{
    ZXIC_UINT32 dbg_wdata_axi_mst0;
}DPP_CFG_PCIE_DBG_WDATA_AXI_MST0_T;

typedef struct dpp_cfg_pcie_dbg_wdata_axi_mst1_t
{
    ZXIC_UINT32 dbg_wdata_axi_mst1;
}DPP_CFG_PCIE_DBG_WDATA_AXI_MST1_T;

typedef struct dpp_cfg_pcie_dbg_wdata_axi_mst2_t
{
    ZXIC_UINT32 dbg_wdata_axi_mst2;
}DPP_CFG_PCIE_DBG_WDATA_AXI_MST2_T;

typedef struct dpp_cfg_pcie_dbg_wdata_axi_mst3_t
{
    ZXIC_UINT32 dbg_wdata_axi_mst3;
}DPP_CFG_PCIE_DBG_WDATA_AXI_MST3_T;

typedef struct dpp_cfg_pcie_dbg_wstrb_axi_mst_t
{
    ZXIC_UINT32 dbg_wstrb_axi_mst;
}DPP_CFG_PCIE_DBG_WSTRB_AXI_MST_T;

typedef struct dpp_cfg_pcie_dbg_wlast_axi_mst_t
{
    ZXIC_UINT32 dbg_wlast_axi_mst;
}DPP_CFG_PCIE_DBG_WLAST_AXI_MST_T;

typedef struct dpp_cfg_pcie_dbg_arid_axi_mst_t
{
    ZXIC_UINT32 dbg_arid_axi_mst;
}DPP_CFG_PCIE_DBG_ARID_AXI_MST_T;

typedef struct dpp_cfg_pcie_dbg_araddr_axi_mst0_t
{
    ZXIC_UINT32 dbg_araddr_axi_mst0;
}DPP_CFG_PCIE_DBG_ARADDR_AXI_MST0_T;

typedef struct dpp_cfg_pcie_dbg_araddr_axi_mst1_t
{
    ZXIC_UINT32 dbg_araddr_axi_mst1;
}DPP_CFG_PCIE_DBG_ARADDR_AXI_MST1_T;

typedef struct dpp_cfg_pcie_dbg_arlen_axi_mst_t
{
    ZXIC_UINT32 dbg_arlen_axi_mst;
}DPP_CFG_PCIE_DBG_ARLEN_AXI_MST_T;

typedef struct dpp_cfg_pcie_dbg_arsize_axi_mst_t
{
    ZXIC_UINT32 dbg_arsize_axi_mst;
}DPP_CFG_PCIE_DBG_ARSIZE_AXI_MST_T;

typedef struct dpp_cfg_pcie_dbg_arburst_axi_mst_t
{
    ZXIC_UINT32 dbg_arburst_axi_mst;
}DPP_CFG_PCIE_DBG_ARBURST_AXI_MST_T;

typedef struct dpp_cfg_pcie_dbg_arlock_axi_mst_t
{
    ZXIC_UINT32 dbg_arlock_axi_mst;
}DPP_CFG_PCIE_DBG_ARLOCK_AXI_MST_T;

typedef struct dpp_cfg_pcie_dbg_arcache_axi_mst_t
{
    ZXIC_UINT32 dbg_arcache_axi_mst;
}DPP_CFG_PCIE_DBG_ARCACHE_AXI_MST_T;

typedef struct dpp_cfg_pcie_dbg_arprot_axi_mst_t
{
    ZXIC_UINT32 dbg_arprot_axi_mst;
}DPP_CFG_PCIE_DBG_ARPROT_AXI_MST_T;

typedef struct dpp_cfg_pcie_dbg_rdata_axi_mst0_t
{
    ZXIC_UINT32 dbg_rdata_axi_mst0;
}DPP_CFG_PCIE_DBG_RDATA_AXI_MST0_T;

typedef struct dpp_cfg_pcie_dbg_rdata_axi_mst1_t
{
    ZXIC_UINT32 dbg_rdata_axi_mst1;
}DPP_CFG_PCIE_DBG_RDATA_AXI_MST1_T;

typedef struct dpp_cfg_pcie_dbg_rdata_axi_mst2_t
{
    ZXIC_UINT32 dbg_rdata_axi_mst2;
}DPP_CFG_PCIE_DBG_RDATA_AXI_MST2_T;

typedef struct dpp_cfg_pcie_dbg_rdata_axi_mst3_t
{
    ZXIC_UINT32 dbg_rdata_axi_mst3;
}DPP_CFG_PCIE_DBG_RDATA_AXI_MST3_T;

typedef struct dpp_cfg_pcie_axi_mst_state_t
{
    ZXIC_UINT32 axi_mst_state;
}DPP_CFG_PCIE_AXI_MST_STATE_T;

typedef struct dpp_cfg_pcie_axi_cfg_state_t
{
    ZXIC_UINT32 axi_cfg_state;
}DPP_CFG_PCIE_AXI_CFG_STATE_T;

typedef struct dpp_cfg_pcie_axi_slv_rd_state_t
{
    ZXIC_UINT32 axi_slv_rd_state;
}DPP_CFG_PCIE_AXI_SLV_RD_STATE_T;

typedef struct dpp_cfg_pcie_axi_slv_wr_state_t
{
    ZXIC_UINT32 axi_slv_wr_state;
}DPP_CFG_PCIE_AXI_SLV_WR_STATE_T;

typedef struct dpp_cfg_pcie_axim_delay_en_t
{
    ZXIC_UINT32 axim_delay_en;
}DPP_CFG_PCIE_AXIM_DELAY_EN_T;

typedef struct dpp_cfg_pcie_axim_delay_t
{
    ZXIC_UINT32 axim_delay;
}DPP_CFG_PCIE_AXIM_DELAY_T;

typedef struct dpp_cfg_pcie_axim_speed_wr_t
{
    ZXIC_UINT32 axim_speed_wr;
}DPP_CFG_PCIE_AXIM_SPEED_WR_T;

typedef struct dpp_cfg_pcie_axim_speed_rd_t
{
    ZXIC_UINT32 axim_speed_rd;
}DPP_CFG_PCIE_AXIM_SPEED_RD_T;

typedef struct dpp_cfg_pcie_dbg_awaddr_axi_slv0_t
{
    ZXIC_UINT32 dbg_awaddr_axi_slv0;
}DPP_CFG_PCIE_DBG_AWADDR_AXI_SLV0_T;

typedef struct dpp_cfg_pcie_dbg_awaddr_axi_slv1_t
{
    ZXIC_UINT32 dbg_awaddr_axi_slv1;
}DPP_CFG_PCIE_DBG_AWADDR_AXI_SLV1_T;

typedef struct dpp_cfg_pcie_dbg0_wdata_axi_slv0_t
{
    ZXIC_UINT32 dbg0_wdata_axi_slv0;
}DPP_CFG_PCIE_DBG0_WDATA_AXI_SLV0_T;

typedef struct dpp_cfg_pcie_dbg0_wdata_axi_slv1_t
{
    ZXIC_UINT32 dbg0_wdata_axi_slv1;
}DPP_CFG_PCIE_DBG0_WDATA_AXI_SLV1_T;

typedef struct dpp_cfg_pcie_dbg0_wdata_axi_slv2_t
{
    ZXIC_UINT32 dbg0_wdata_axi_slv2;
}DPP_CFG_PCIE_DBG0_WDATA_AXI_SLV2_T;

typedef struct dpp_cfg_pcie_dbg0_wdata_axi_slv3_t
{
    ZXIC_UINT32 dbg0_wdata_axi_slv3;
}DPP_CFG_PCIE_DBG0_WDATA_AXI_SLV3_T;

typedef struct dpp_cfg_pcie_dbg1_wdata_axi_slv0_t
{
    ZXIC_UINT32 dbg1_wdata_axi_slv0;
}DPP_CFG_PCIE_DBG1_WDATA_AXI_SLV0_T;

typedef struct dpp_cfg_pcie_dbg1_wdata_axi_slv1_t
{
    ZXIC_UINT32 dbg1_wdata_axi_slv1;
}DPP_CFG_PCIE_DBG1_WDATA_AXI_SLV1_T;

typedef struct dpp_cfg_pcie_dbg1_wdata_axi_slv2_t
{
    ZXIC_UINT32 dbg1_wdata_axi_slv2;
}DPP_CFG_PCIE_DBG1_WDATA_AXI_SLV2_T;

typedef struct dpp_cfg_pcie_dbg1_wdata_axi_slv3_t
{
    ZXIC_UINT32 dbg1_wdata_axi_slv3;
}DPP_CFG_PCIE_DBG1_WDATA_AXI_SLV3_T;

typedef struct dpp_cfg_pcie_dbg2_wdata_axi_slv0_t
{
    ZXIC_UINT32 dbg2_wdata_axi_slv0;
}DPP_CFG_PCIE_DBG2_WDATA_AXI_SLV0_T;

typedef struct dpp_cfg_pcie_dbg2_wdata_axi_slv1_t
{
    ZXIC_UINT32 dbg2_wdata_axi_slv1;
}DPP_CFG_PCIE_DBG2_WDATA_AXI_SLV1_T;

typedef struct dpp_cfg_pcie_dbg2_wdata_axi_slv2_t
{
    ZXIC_UINT32 dbg2_wdata_axi_slv2;
}DPP_CFG_PCIE_DBG2_WDATA_AXI_SLV2_T;

typedef struct dpp_cfg_pcie_dbg2_wdata_axi_slv3_t
{
    ZXIC_UINT32 dbg2_wdata_axi_slv3;
}DPP_CFG_PCIE_DBG2_WDATA_AXI_SLV3_T;

typedef struct dpp_cfg_pcie_dbg3_wdata_axi_slv0_t
{
    ZXIC_UINT32 dbg3_wdata_axi_slv0;
}DPP_CFG_PCIE_DBG3_WDATA_AXI_SLV0_T;

typedef struct dpp_cfg_pcie_dbg3_wdata_axi_slv1_t
{
    ZXIC_UINT32 dbg3_wdata_axi_slv1;
}DPP_CFG_PCIE_DBG3_WDATA_AXI_SLV1_T;

typedef struct dpp_cfg_pcie_dbg3_wdata_axi_slv2_t
{
    ZXIC_UINT32 dbg3_wdata_axi_slv2;
}DPP_CFG_PCIE_DBG3_WDATA_AXI_SLV2_T;

typedef struct dpp_cfg_pcie_dbg3_wdata_axi_slv3_t
{
    ZXIC_UINT32 dbg3_wdata_axi_slv3;
}DPP_CFG_PCIE_DBG3_WDATA_AXI_SLV3_T;

typedef struct dpp_cfg_pcie_dbg4_wdata_axi_slv0_t
{
    ZXIC_UINT32 dbg4_wdata_axi_slv0;
}DPP_CFG_PCIE_DBG4_WDATA_AXI_SLV0_T;

typedef struct dpp_cfg_pcie_dbg4_wdata_axi_slv1_t
{
    ZXIC_UINT32 dbg4_wdata_axi_slv1;
}DPP_CFG_PCIE_DBG4_WDATA_AXI_SLV1_T;

typedef struct dpp_cfg_pcie_dbg4_wdata_axi_slv2_t
{
    ZXIC_UINT32 dbg4_wdata_axi_slv2;
}DPP_CFG_PCIE_DBG4_WDATA_AXI_SLV2_T;

typedef struct dpp_cfg_pcie_dbg4_wdata_axi_slv3_t
{
    ZXIC_UINT32 dbg4_wdata_axi_slv3;
}DPP_CFG_PCIE_DBG4_WDATA_AXI_SLV3_T;

typedef struct dpp_cfg_pcie_dbg5_wdata_axi_slv0_t
{
    ZXIC_UINT32 dbg5_wdata_axi_slv0;
}DPP_CFG_PCIE_DBG5_WDATA_AXI_SLV0_T;

typedef struct dpp_cfg_pcie_dbg5_wdata_axi_slv1_t
{
    ZXIC_UINT32 dbg5_wdata_axi_slv1;
}DPP_CFG_PCIE_DBG5_WDATA_AXI_SLV1_T;

typedef struct dpp_cfg_pcie_dbg5_wdata_axi_slv2_t
{
    ZXIC_UINT32 dbg5_wdata_axi_slv2;
}DPP_CFG_PCIE_DBG5_WDATA_AXI_SLV2_T;

typedef struct dpp_cfg_pcie_dbg5_wdata_axi_slv3_t
{
    ZXIC_UINT32 dbg5_wdata_axi_slv3;
}DPP_CFG_PCIE_DBG5_WDATA_AXI_SLV3_T;

typedef struct dpp_cfg_pcie_dbg6_wdata_axi_slv0_t
{
    ZXIC_UINT32 dbg6_wdata_axi_slv0;
}DPP_CFG_PCIE_DBG6_WDATA_AXI_SLV0_T;

typedef struct dpp_cfg_pcie_dbg6_wdata_axi_slv1_t
{
    ZXIC_UINT32 dbg6_wdata_axi_slv1;
}DPP_CFG_PCIE_DBG6_WDATA_AXI_SLV1_T;

typedef struct dpp_cfg_pcie_dbg6_wdata_axi_slv2_t
{
    ZXIC_UINT32 dbg6_wdata_axi_slv2;
}DPP_CFG_PCIE_DBG6_WDATA_AXI_SLV2_T;

typedef struct dpp_cfg_pcie_dbg6_wdata_axi_slv3_t
{
    ZXIC_UINT32 dbg6_wdata_axi_slv3;
}DPP_CFG_PCIE_DBG6_WDATA_AXI_SLV3_T;

typedef struct dpp_cfg_pcie_dbg7_wdata_axi_slv0_t
{
    ZXIC_UINT32 dbg7_wdata_axi_slv0;
}DPP_CFG_PCIE_DBG7_WDATA_AXI_SLV0_T;

typedef struct dpp_cfg_pcie_dbg7_wdata_axi_slv1_t
{
    ZXIC_UINT32 dbg7_wdata_axi_slv1;
}DPP_CFG_PCIE_DBG7_WDATA_AXI_SLV1_T;

typedef struct dpp_cfg_pcie_dbg7_wdata_axi_slv2_t
{
    ZXIC_UINT32 dbg7_wdata_axi_slv2;
}DPP_CFG_PCIE_DBG7_WDATA_AXI_SLV2_T;

typedef struct dpp_cfg_pcie_dbg7_wdata_axi_slv3_t
{
    ZXIC_UINT32 dbg7_wdata_axi_slv3;
}DPP_CFG_PCIE_DBG7_WDATA_AXI_SLV3_T;

typedef struct dpp_cfg_pcie_dbg8_wdata_axi_slv0_t
{
    ZXIC_UINT32 dbg8_wdata_axi_slv0;
}DPP_CFG_PCIE_DBG8_WDATA_AXI_SLV0_T;

typedef struct dpp_cfg_pcie_dbg8_wdata_axi_slv1_t
{
    ZXIC_UINT32 dbg8_wdata_axi_slv1;
}DPP_CFG_PCIE_DBG8_WDATA_AXI_SLV1_T;

typedef struct dpp_cfg_pcie_dbg8_wdata_axi_slv2_t
{
    ZXIC_UINT32 dbg8_wdata_axi_slv2;
}DPP_CFG_PCIE_DBG8_WDATA_AXI_SLV2_T;

typedef struct dpp_cfg_pcie_dbg8_wdata_axi_slv3_t
{
    ZXIC_UINT32 dbg8_wdata_axi_slv3;
}DPP_CFG_PCIE_DBG8_WDATA_AXI_SLV3_T;

typedef struct dpp_cfg_pcie_dbg9_wdata_axi_slv0_t
{
    ZXIC_UINT32 dbg9_wdata_axi_slv0;
}DPP_CFG_PCIE_DBG9_WDATA_AXI_SLV0_T;

typedef struct dpp_cfg_pcie_dbg9_wdata_axi_slv1_t
{
    ZXIC_UINT32 dbg9_wdata_axi_slv1;
}DPP_CFG_PCIE_DBG9_WDATA_AXI_SLV1_T;

typedef struct dpp_cfg_pcie_dbg9_wdata_axi_slv2_t
{
    ZXIC_UINT32 dbg9_wdata_axi_slv2;
}DPP_CFG_PCIE_DBG9_WDATA_AXI_SLV2_T;

typedef struct dpp_cfg_pcie_dbg9_wdata_axi_slv3_t
{
    ZXIC_UINT32 dbg9_wdata_axi_slv3;
}DPP_CFG_PCIE_DBG9_WDATA_AXI_SLV3_T;

typedef struct dpp_cfg_pcie_dbg_awlen_axi_slv_t
{
    ZXIC_UINT32 dbg_awlen_axi_slv;
}DPP_CFG_PCIE_DBG_AWLEN_AXI_SLV_T;

typedef struct dpp_cfg_pcie_dbg_wlast_axi_slv_t
{
    ZXIC_UINT32 dbg_wlast_axi_slv;
}DPP_CFG_PCIE_DBG_WLAST_AXI_SLV_T;

typedef struct dpp_cfg_pcie_dbg_araddr_axi_slv0_t
{
    ZXIC_UINT32 dbg5_wdata_axi_slv1;
}DPP_CFG_PCIE_DBG_ARADDR_AXI_SLV0_T;

typedef struct dpp_cfg_pcie_dbg_araddr_axi_slv1_t
{
    ZXIC_UINT32 dbg5_wdata_axi_slv2;
}DPP_CFG_PCIE_DBG_ARADDR_AXI_SLV1_T;

typedef struct dpp_cfg_pcie_dbg0_rdata_axi_slv0_t
{
    ZXIC_UINT32 dbg5_wdata_axi_slv3;
}DPP_CFG_PCIE_DBG0_RDATA_AXI_SLV0_T;

typedef struct dpp_cfg_pcie_dbg0_rdata_axi_slv1_t
{
    ZXIC_UINT32 dbg6_wdata_axi_slv0;
}DPP_CFG_PCIE_DBG0_RDATA_AXI_SLV1_T;

typedef struct dpp_cfg_pcie_dbg0_rdata_axi_slv2_t
{
    ZXIC_UINT32 dbg6_wdata_axi_slv1;
}DPP_CFG_PCIE_DBG0_RDATA_AXI_SLV2_T;

typedef struct dpp_cfg_pcie_dbg0_rdata_axi_slv3_t
{
    ZXIC_UINT32 dbg6_wdata_axi_slv2;
}DPP_CFG_PCIE_DBG0_RDATA_AXI_SLV3_T;

typedef struct dpp_cfg_pcie_dbg1_rdata_axi_slv0_t
{
    ZXIC_UINT32 dbg6_wdata_axi_slv3;
}DPP_CFG_PCIE_DBG1_RDATA_AXI_SLV0_T;

typedef struct dpp_cfg_pcie_dbg1_rdata_axi_slv1_t
{
    ZXIC_UINT32 dbg7_wdata_axi_slv0;
}DPP_CFG_PCIE_DBG1_RDATA_AXI_SLV1_T;

typedef struct dpp_cfg_pcie_dbg1_rdata_axi_slv2_t
{
    ZXIC_UINT32 dbg7_wdata_axi_slv1;
}DPP_CFG_PCIE_DBG1_RDATA_AXI_SLV2_T;

typedef struct dpp_cfg_pcie_dbg1_rdata_axi_slv3_t
{
    ZXIC_UINT32 dbg7_wdata_axi_slv2;
}DPP_CFG_PCIE_DBG1_RDATA_AXI_SLV3_T;

typedef struct dpp_cfg_pcie_dbg_rlast_axi_slv_t
{
    ZXIC_UINT32 dbg_rlast_axi_slv;
}DPP_CFG_PCIE_DBG_RLAST_AXI_SLV_T;

typedef struct dpp_cfg_dma_dma_enable_t
{
    ZXIC_UINT32 dma_enable;
}DPP_CFG_DMA_DMA_ENABLE_T;

typedef struct dpp_cfg_dma_up_req_t
{
    ZXIC_UINT32 up_req;
}DPP_CFG_DMA_UP_REQ_T;

typedef struct dpp_cfg_dma_dma_up_current_state_t
{
    ZXIC_UINT32 dma_up_current_state;
}DPP_CFG_DMA_DMA_UP_CURRENT_STATE_T;

typedef struct dpp_cfg_dma_dma_up_req_ack_t
{
    ZXIC_UINT32 dma_up_req_ack;
}DPP_CFG_DMA_DMA_UP_REQ_ACK_T;

typedef struct dpp_cfg_dma_dma_done_latch_t
{
    ZXIC_UINT32 done_latch;
}DPP_CFG_DMA_DMA_DONE_LATCH_T;

typedef struct dpp_cfg_dma_dma_up_cpu_addr_low32_t
{
    ZXIC_UINT32 dma_up_cpu_addr_low;
}DPP_CFG_DMA_DMA_UP_CPU_ADDR_LOW32_T;

typedef struct dpp_cfg_dma_dma_up_cpu_addr_high32_t
{
    ZXIC_UINT32 dma_up_cpu_addr_high;
}DPP_CFG_DMA_DMA_UP_CPU_ADDR_HIGH32_T;

typedef struct dpp_cfg_dma_dma_up_se_addr_t
{
    ZXIC_UINT32 dma_up_se_addr;
}DPP_CFG_DMA_DMA_UP_SE_ADDR_T;

typedef struct dpp_cfg_dma_dma_done_int_t
{
    ZXIC_UINT32 dma_done_int;
}DPP_CFG_DMA_DMA_DONE_INT_T;

typedef struct dpp_cfg_dma_sp_cfg_t
{
    ZXIC_UINT32 sp_cfg;
}DPP_CFG_DMA_SP_CFG_T;

typedef struct dpp_cfg_dma_dma_ing_t
{
    ZXIC_UINT32 dma_ing;
}DPP_CFG_DMA_DMA_ING_T;

typedef struct dpp_cfg_dma_rd_timeout_thresh_t
{
    ZXIC_UINT32 rd_timeout_thresh;
}DPP_CFG_DMA_RD_TIMEOUT_THRESH_T;

typedef struct dpp_cfg_dma_dma_tab_sta_up_fifo_gap_t
{
    ZXIC_UINT32 dma_tab_sta_up_fifo_gap;
}DPP_CFG_DMA_DMA_TAB_STA_UP_FIFO_GAP_T;

typedef struct dpp_cfg_dma_cfg_mac_tim_t
{
    ZXIC_UINT32 cfg_mac_tim;
}DPP_CFG_DMA_CFG_MAC_TIM_T;

typedef struct dpp_cfg_dma_cfg_mac_num_t
{
    ZXIC_UINT32 cfg_mac_num;
}DPP_CFG_DMA_CFG_MAC_NUM_T;

typedef struct dpp_cfg_dma_init_bd_addr_t
{
    ZXIC_UINT32 init_bd_addr;
}DPP_CFG_DMA_INIT_BD_ADDR_T;

typedef struct dpp_cfg_dma_mac_up_bd_addr1_low32_t
{
    ZXIC_UINT32 mac_up_bd_addr1_low32;
}DPP_CFG_DMA_MAC_UP_BD_ADDR1_LOW32_T;

typedef struct dpp_cfg_dma_mac_up_bd_addr1_high32_t
{
    ZXIC_UINT32 mac_up_bd_addr1_high32;
}DPP_CFG_DMA_MAC_UP_BD_ADDR1_HIGH32_T;

typedef struct dpp_cfg_dma_mac_up_bd_addr2_low32_t
{
    ZXIC_UINT32 mac_up_bd_addr2_low32;
}DPP_CFG_DMA_MAC_UP_BD_ADDR2_LOW32_T;

typedef struct dpp_cfg_dma_mac_up_bd_addr2_high32_t
{
    ZXIC_UINT32 mac_up_bd_addr2_high32;
}DPP_CFG_DMA_MAC_UP_BD_ADDR2_HIGH32_T;

typedef struct dpp_cfg_dma_cfg_mac_max_num_t
{
    ZXIC_UINT32 cfg_mac_max_num;
}DPP_CFG_DMA_CFG_MAC_MAX_NUM_T;

typedef struct dpp_cfg_dma_dma_wbuf_ff_empty_t
{
    ZXIC_UINT32 dma_wbuf_ff_empty;
}DPP_CFG_DMA_DMA_WBUF_FF_EMPTY_T;

typedef struct dpp_cfg_dma_dma_wbuf_state_t
{
    ZXIC_UINT32 dma_wbuf_state;
}DPP_CFG_DMA_DMA_WBUF_STATE_T;

typedef struct dpp_cfg_dma_dma_mac_bd_addr_low32_t
{
    ZXIC_UINT32 dma_mac_bd_addr_low32;
}DPP_CFG_DMA_DMA_MAC_BD_ADDR_LOW32_T;

typedef struct dpp_cfg_dma_dma_mac_bd_addr_high32_t
{
    ZXIC_UINT32 dma_mac_bd_addr_high32;
}DPP_CFG_DMA_DMA_MAC_BD_ADDR_HIGH32_T;

typedef struct dpp_cfg_dma_mac_up_enable_t
{
    ZXIC_UINT32 mac_up_enable;
}DPP_CFG_DMA_MAC_UP_ENABLE_T;

typedef struct dpp_cfg_dma_mac_endian_t
{
    ZXIC_UINT32 mac_endian;
}DPP_CFG_DMA_MAC_ENDIAN_T;

typedef struct dpp_cfg_dma_up_endian_t
{
    ZXIC_UINT32 up_endian;
}DPP_CFG_DMA_UP_ENDIAN_T;

typedef struct dpp_cfg_dma_dma_up_rd_cnt_latch_t
{
    ZXIC_UINT32 dma_up_rd_cnt_latch;
}DPP_CFG_DMA_DMA_UP_RD_CNT_LATCH_T;

typedef struct dpp_cfg_dma_dma_up_rcv_cnt_latch_t
{
    ZXIC_UINT32 dma_up_rcv_cnt_latch;
}DPP_CFG_DMA_DMA_UP_RCV_CNT_LATCH_T;

typedef struct dpp_cfg_dma_dma_up_cnt_latch_t
{
    ZXIC_UINT32 dma_up_cnt_latch;
}DPP_CFG_DMA_DMA_UP_CNT_LATCH_T;

typedef struct dpp_cfg_dma_cpu_rd_bd_pulse_t
{
    ZXIC_UINT32 cpu_rd_bd_pulse;
}DPP_CFG_DMA_CPU_RD_BD_PULSE_T;

typedef struct dpp_cfg_dma_cpu_bd_threshold_t
{
    ZXIC_UINT32 cpu_bd_threshold;
}DPP_CFG_DMA_CPU_BD_THRESHOLD_T;

typedef struct dpp_cfg_dma_cpu_bd_used_cnt_t
{
    ZXIC_UINT32 cpu_bd_used_cnt;
}DPP_CFG_DMA_CPU_BD_USED_CNT_T;

typedef struct dpp_cfg_dma_dma_up_rcv_status_t
{
    ZXIC_UINT32 dma_up_rcv_status;
}DPP_CFG_DMA_DMA_UP_RCV_STATUS_T;

typedef struct dpp_cfg_dma_slv_rid_err_en_t
{
    ZXIC_UINT32 slv_rid_err_en;
}DPP_CFG_DMA_SLV_RID_ERR_EN_T;

typedef struct dpp_cfg_dma_slv_rresp_err_en_t
{
    ZXIC_UINT32 slv_rresp_err_en;
}DPP_CFG_DMA_SLV_RRESP_ERR_EN_T;

typedef struct dpp_cfg_dma_se_rdbk_ff_full_t
{
    ZXIC_UINT32 se_rdbk_ff_full;
}DPP_CFG_DMA_SE_RDBK_FF_FULL_T;

typedef struct dpp_cfg_dma_dma_up_data_count_t
{
    ZXIC_UINT32 dma_up_data_count;
}DPP_CFG_DMA_DMA_UP_DATA_COUNT_T;

typedef struct dpp_cfg_dma_dma_mwr_fifo_afull_gap_t
{
    ZXIC_UINT32 dma_mwr_fifo_afull_gap;
}DPP_CFG_DMA_DMA_MWR_FIFO_AFULL_GAP_T;

typedef struct dpp_cfg_dma_dma_info_fifo_afull_gap_t
{
    ZXIC_UINT32 dma_mwr_fifo_afull_gap;
}DPP_CFG_DMA_DMA_INFO_FIFO_AFULL_GAP_T;

typedef struct dpp_cfg_dma_dma_rd_timeout_set_t
{
    ZXIC_UINT32 dma_rd_timeout_set;
}DPP_CFG_DMA_DMA_RD_TIMEOUT_SET_T;

typedef struct dpp_cfg_dma_dma_bd_dat_err_en_t
{
    ZXIC_UINT32 dma_bd_dat_err_en;
}DPP_CFG_DMA_DMA_BD_DAT_ERR_EN_T;

typedef struct dpp_cfg_dma_dma_repeat_cnt_t
{
    ZXIC_UINT32 dma_repeat_cnt;
}DPP_CFG_DMA_DMA_REPEAT_CNT_T;

typedef struct dpp_cfg_dma_dma_rd_timeout_en_t
{
    ZXIC_UINT32 dma_rd_timeout_en;
}DPP_CFG_DMA_DMA_RD_TIMEOUT_EN_T;

typedef struct dpp_cfg_dma_dma_repeat_read_t
{
    ZXIC_UINT32 dma_repeat_read;
}DPP_CFG_DMA_DMA_REPEAT_READ_T;

typedef struct dpp_cfg_dma_dma_repeat_read_en_t
{
    ZXIC_UINT32 dma_repeat_read_en;
}DPP_CFG_DMA_DMA_REPEAT_READ_EN_T;

typedef struct dpp_cfg_dma_bd_ctl_state_t
{
    ZXIC_UINT32 bd_ctl_state;
}DPP_CFG_DMA_BD_CTL_STATE_T;

typedef struct dpp_cfg_dma_dma_done_int_cnt_wr_t
{
    ZXIC_UINT32 dma_done_int_cnt_wr;
}DPP_CFG_DMA_DMA_DONE_INT_CNT_WR_T;

typedef struct dpp_cfg_dma_dma_done_int_cnt_mac_t
{
    ZXIC_UINT32 dma_done_int_cnt_mac;
}DPP_CFG_DMA_DMA_DONE_INT_CNT_MAC_T;

typedef struct dpp_cfg_dma_current_mac_num_t
{
    ZXIC_UINT32 current_mac_num;
}DPP_CFG_DMA_CURRENT_MAC_NUM_T;

typedef struct dpp_cfg_dma_cfg_mac_afifo_afull_t
{
    ZXIC_UINT32 cfg_mac_afifo_afull;
}DPP_CFG_DMA_CFG_MAC_AFIFO_AFULL_T;

typedef struct dpp_cfg_dma_dma_mac_ff_full_t
{
    ZXIC_UINT32 dma_mac_ff_full;
}DPP_CFG_DMA_DMA_MAC_FF_FULL_T;

typedef struct dpp_cfg_dma_user_axi_mst_t
{
    ZXIC_UINT32 user_en;
    ZXIC_UINT32 cfg_epid;
    ZXIC_UINT32 cfg_vfunc_num;
    ZXIC_UINT32 cfg_func_num;
    ZXIC_UINT32 cfg_vfunc_active;
}DPP_CFG_DMA_USER_AXI_MST_T;

typedef struct dpp_cfg_csr_sbus_state_t
{
    ZXIC_UINT32 sbus_state;
}DPP_CFG_CSR_SBUS_STATE_T;

typedef struct dpp_cfg_csr_mst_debug_en_t
{
    ZXIC_UINT32 mst_debug_en;
}DPP_CFG_CSR_MST_DEBUG_EN_T;

typedef struct dpp_cfg_csr_sbus_command_sel_t
{
    ZXIC_UINT32 sbus_command_sel;
}DPP_CFG_CSR_SBUS_COMMAND_SEL_T;

typedef struct dpp_cfg_csr_soc_rd_time_out_thresh_t
{
    ZXIC_UINT32 soc_rd_time_out_thresh;
}DPP_CFG_CSR_SOC_RD_TIME_OUT_THRESH_T;

typedef struct dpp_cfg_csr_big_little_byte_order_t
{
    ZXIC_UINT32 big_little_byte_order;
}DPP_CFG_CSR_BIG_LITTLE_BYTE_ORDER_T;

typedef struct dpp_cfg_csr_ecc_bypass_read_t
{
    ZXIC_UINT32 ecc_bypass_read;
}DPP_CFG_CSR_ECC_BYPASS_READ_T;

typedef struct dpp_cfg_csr_ahb_async_wr_fifo_afull_gap_t
{
    ZXIC_UINT32 ahb_async_wr_fifo_afull_gap;
}DPP_CFG_CSR_AHB_ASYNC_WR_FIFO_AFULL_GAP_T;

typedef struct dpp_cfg_csr_ahb_async_rd_fifo_afull_gap_t
{
    ZXIC_UINT32 ahb_async_rd_fifo_afull_gap;
}DPP_CFG_CSR_AHB_ASYNC_RD_FIFO_AFULL_GAP_T;

typedef struct dpp_cfg_csr_ahb_async_cpl_fifo_afull_gap_t
{
    ZXIC_UINT32 ahb_async_cpl_fifo_afull_gap;
}DPP_CFG_CSR_AHB_ASYNC_CPL_FIFO_AFULL_GAP_T;

typedef struct dpp_cfg_csr_mst_debug_data0_high26_t
{
    ZXIC_UINT32 mst_debug_data0_high26;
}DPP_CFG_CSR_MST_DEBUG_DATA0_HIGH26_T;

typedef struct dpp_cfg_csr_mst_debug_data0_low32_t
{
    ZXIC_UINT32 mst_debug_data0_low32;
}DPP_CFG_CSR_MST_DEBUG_DATA0_LOW32_T;

typedef struct dpp_cfg_csr_mst_debug_data1_high26_t
{
    ZXIC_UINT32 mst_debug_data1_high26;
}DPP_CFG_CSR_MST_DEBUG_DATA1_HIGH26_T;

typedef struct dpp_cfg_csr_mst_debug_data1_low32_t
{
    ZXIC_UINT32 mst_debug_data1_low32;
}DPP_CFG_CSR_MST_DEBUG_DATA1_LOW32_T;

typedef struct dpp_cfg_csr_mst_debug_data2_high26_t
{
    ZXIC_UINT32 mst_debug_data2_high26;
}DPP_CFG_CSR_MST_DEBUG_DATA2_HIGH26_T;

typedef struct dpp_cfg_csr_mst_debug_data2_low32_t
{
    ZXIC_UINT32 mst_debug_data2_low32;
}DPP_CFG_CSR_MST_DEBUG_DATA2_LOW32_T;

typedef struct dpp_cfg_csr_mst_debug_data3_high26_t
{
    ZXIC_UINT32 mst_debug_data3_high26;
}DPP_CFG_CSR_MST_DEBUG_DATA3_HIGH26_T;

typedef struct dpp_cfg_csr_mst_debug_data3_low32_t
{
    ZXIC_UINT32 mst_debug_data3_low32;
}DPP_CFG_CSR_MST_DEBUG_DATA3_LOW32_T;

typedef struct dpp_cfg_csr_mst_debug_data4_high26_t
{
    ZXIC_UINT32 mst_debug_data4_high26;
}DPP_CFG_CSR_MST_DEBUG_DATA4_HIGH26_T;

typedef struct dpp_cfg_csr_mst_debug_data4_low32_t
{
    ZXIC_UINT32 mst_debug_data4_low32;
}DPP_CFG_CSR_MST_DEBUG_DATA4_LOW32_T;

typedef struct dpp_cfg_csr_mst_debug_data5_high26_t
{
    ZXIC_UINT32 mst_debug_data5_high26;
}DPP_CFG_CSR_MST_DEBUG_DATA5_HIGH26_T;

typedef struct dpp_cfg_csr_mst_debug_data5_low32_t
{
    ZXIC_UINT32 mst_debug_data5_low32;
}DPP_CFG_CSR_MST_DEBUG_DATA5_LOW32_T;

typedef struct dpp_cfg_csr_mst_debug_data6_high26_t
{
    ZXIC_UINT32 mst_debug_data6_high26;
}DPP_CFG_CSR_MST_DEBUG_DATA6_HIGH26_T;

typedef struct dpp_cfg_csr_mst_debug_data6_low32_t
{
    ZXIC_UINT32 mst_debug_data6_low32;
}DPP_CFG_CSR_MST_DEBUG_DATA6_LOW32_T;

typedef struct dpp_cfg_csr_mst_debug_data7_high26_t
{
    ZXIC_UINT32 mst_debug_data7_high26;
}DPP_CFG_CSR_MST_DEBUG_DATA7_HIGH26_T;

typedef struct dpp_cfg_csr_mst_debug_data7_low32_t
{
    ZXIC_UINT32 mst_debug_data7_low32;
}DPP_CFG_CSR_MST_DEBUG_DATA7_LOW32_T;

typedef struct dpp_cfg_csr_mst_debug_data8_high26_t
{
    ZXIC_UINT32 mst_debug_data8_high26;
}DPP_CFG_CSR_MST_DEBUG_DATA8_HIGH26_T;

typedef struct dpp_cfg_csr_mst_debug_data8_low32_t
{
    ZXIC_UINT32 mst_debug_data8_low32;
}DPP_CFG_CSR_MST_DEBUG_DATA8_LOW32_T;

typedef struct dpp_cfg_csr_mst_debug_data9_high26_t
{
    ZXIC_UINT32 mst_debug_data9_high26;
}DPP_CFG_CSR_MST_DEBUG_DATA9_HIGH26_T;

typedef struct dpp_cfg_csr_mst_debug_data9_low32_t
{
    ZXIC_UINT32 mst_debug_data9_low32;
}DPP_CFG_CSR_MST_DEBUG_DATA9_LOW32_T;

typedef struct dpp_cfg_csr_mst_debug_data10_high26_t
{
    ZXIC_UINT32 mst_debug_data10_high26;
}DPP_CFG_CSR_MST_DEBUG_DATA10_HIGH26_T;

typedef struct dpp_cfg_csr_mst_debug_data10_low32_t
{
    ZXIC_UINT32 mst_debug_data10_low32;
}DPP_CFG_CSR_MST_DEBUG_DATA10_LOW32_T;

typedef struct dpp_cfg_csr_mst_debug_data11_high26_t
{
    ZXIC_UINT32 mst_debug_data11_high26;
}DPP_CFG_CSR_MST_DEBUG_DATA11_HIGH26_T;

typedef struct dpp_cfg_csr_mst_debug_data11_low32_t
{
    ZXIC_UINT32 mst_debug_data11_low32;
}DPP_CFG_CSR_MST_DEBUG_DATA11_LOW32_T;

typedef struct dpp_cfg_csr_mst_debug_data12_high26_t
{
    ZXIC_UINT32 mst_debug_data12_high26;
}DPP_CFG_CSR_MST_DEBUG_DATA12_HIGH26_T;

typedef struct dpp_cfg_csr_mst_debug_data12_low32_t
{
    ZXIC_UINT32 mst_debug_data12_low32;
}DPP_CFG_CSR_MST_DEBUG_DATA12_LOW32_T;

typedef struct dpp_cfg_csr_mst_debug_data13_high26_t
{
    ZXIC_UINT32 mst_debug_data13_high26;
}DPP_CFG_CSR_MST_DEBUG_DATA13_HIGH26_T;

typedef struct dpp_cfg_csr_mst_debug_data13_low32_t
{
    ZXIC_UINT32 mst_debug_data13_low32;
}DPP_CFG_CSR_MST_DEBUG_DATA13_LOW32_T;

typedef struct dpp_cfg_csr_mst_debug_data14_high26_t
{
    ZXIC_UINT32 mst_debug_data14_high26;
}DPP_CFG_CSR_MST_DEBUG_DATA14_HIGH26_T;

typedef struct dpp_cfg_csr_mst_debug_data14_low32_t
{
    ZXIC_UINT32 mst_debug_data14_low32;
}DPP_CFG_CSR_MST_DEBUG_DATA14_LOW32_T;

typedef struct dpp_cfg_csr_mst_debug_data15_high26_t
{
    ZXIC_UINT32 mst_debug_data15_high26;
}DPP_CFG_CSR_MST_DEBUG_DATA15_HIGH26_T;

typedef struct dpp_cfg_csr_mst_debug_data15_low32_t
{
    ZXIC_UINT32 mst_debug_data15_low32;
}DPP_CFG_CSR_MST_DEBUG_DATA15_LOW32_T;


#ifdef __cplusplus
}
#endif
#endif

