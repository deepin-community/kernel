
#ifndef _DPP_PPU_REG_H_
#define _DPP_PPU_REG_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef struct dpp_ppu_ppu_test_r_t
{
    ZXIC_UINT32 test_r;
}DPP_PPU_PPU_TEST_R_T;

typedef struct dpp_ppu_ppu_ppu_debug_en_r_t
{
    ZXIC_UINT32 debug_en_r;
}DPP_PPU_PPU_PPU_DEBUG_EN_R_T;

typedef struct dpp_ppu_ppu_csr_dup_table_wr_data_t
{
    ZXIC_UINT32 item_vld;
    ZXIC_UINT32 flownum_vld;
    ZXIC_UINT32 start_pc;
    ZXIC_UINT32 flownum;
}DPP_PPU_PPU_CSR_DUP_TABLE_WR_DATA_T;

typedef struct dpp_ppu_ppu_csr_dup_table_rd_data_t
{
    ZXIC_UINT32 item_vld;
    ZXIC_UINT32 flownum_vld;
    ZXIC_UINT32 start_pc;
    ZXIC_UINT32 flownum;
}DPP_PPU_PPU_CSR_DUP_TABLE_RD_DATA_T;

typedef struct dpp_ppu_ppu_csr_dup_table_addr_t
{
    ZXIC_UINT32 csr_dup_table_operation;
    ZXIC_UINT32 csr_dup_table_addr;
}DPP_PPU_PPU_CSR_DUP_TABLE_ADDR_T;

typedef struct dpp_ppu_ppu_ppu_debug_vld_t
{
    ZXIC_UINT32 ppu_debug_vld;
}DPP_PPU_PPU_PPU_DEBUG_VLD_T;

typedef struct dpp_ppu_ppu_cop_thash_rsk_319_288_t
{
    ZXIC_UINT32 rsk_319_288;
}DPP_PPU_PPU_COP_THASH_RSK_319_288_T;

typedef struct dpp_ppu_ppu_cop_thash_rsk_287_256_t
{
    ZXIC_UINT32 rsk_287_256;
}DPP_PPU_PPU_COP_THASH_RSK_287_256_T;

typedef struct dpp_ppu_ppu_cop_thash_rsk_255_224_t
{
    ZXIC_UINT32 rsk_255_224;
}DPP_PPU_PPU_COP_THASH_RSK_255_224_T;

typedef struct dpp_ppu_ppu_cop_thash_rsk_223_192_t
{
    ZXIC_UINT32 rsk_223_192;
}DPP_PPU_PPU_COP_THASH_RSK_223_192_T;

typedef struct dpp_ppu_ppu_cop_thash_rsk_191_160_t
{
    ZXIC_UINT32 rsk_191_160;
}DPP_PPU_PPU_COP_THASH_RSK_191_160_T;

typedef struct dpp_ppu_ppu_cop_thash_rsk_159_128_t
{
    ZXIC_UINT32 rsk_159_128;
}DPP_PPU_PPU_COP_THASH_RSK_159_128_T;

typedef struct dpp_ppu_ppu_cop_thash_rsk_127_096_t
{
    ZXIC_UINT32 rsk_127_096;
}DPP_PPU_PPU_COP_THASH_RSK_127_096_T;

typedef struct dpp_ppu_ppu_cop_thash_rsk_095_064_t
{
    ZXIC_UINT32 rsk_095_064;
}DPP_PPU_PPU_COP_THASH_RSK_095_064_T;

typedef struct dpp_ppu_ppu_cop_thash_rsk_063_032_t
{
    ZXIC_UINT32 rsk_063_032;
}DPP_PPU_PPU_COP_THASH_RSK_063_032_T;

typedef struct dpp_ppu_ppu_cop_thash_rsk_031_000_t
{
    ZXIC_UINT32 rsk_031_000;
}DPP_PPU_PPU_COP_THASH_RSK_031_000_T;

typedef struct dpp_ppu_ppu_cfg_ipv4_ipid_start_value_t
{
    ZXIC_UINT32 cfg_ipv4_ipid_start_value;
}DPP_PPU_PPU_CFG_IPV4_IPID_START_VALUE_T;

typedef struct dpp_ppu_ppu_cfg_ipv4_ipid_end_value_t
{
    ZXIC_UINT32 cfg_ipv4_ipid_end_value;
}DPP_PPU_PPU_CFG_IPV4_IPID_END_VALUE_T;

typedef struct dpp_ppu_ppu_cluster_mf_in_en_t
{
    ZXIC_UINT32 cluster_mf_in_en;
}DPP_PPU_PPU_CLUSTER_MF_IN_EN_T;

typedef struct dpp_ppu_ppu_ppu_empty_t
{
    ZXIC_UINT32 ppu_empty;
}DPP_PPU_PPU_PPU_EMPTY_T;

typedef struct dpp_ppu_ppu_instrmem_w_addr_t
{
    ZXIC_UINT32 instrmem_w_addr;
}DPP_PPU_PPU_INSTRMEM_W_ADDR_T;

typedef struct dpp_ppu_ppu_instrmem_w_data_191_160_t
{
    ZXIC_UINT32 instrmem_w_data_191_160;
}DPP_PPU_PPU_INSTRMEM_W_DATA_191_160_T;

typedef struct dpp_ppu_ppu_instrmem_w_data_159_128_t
{
    ZXIC_UINT32 instrmem_w_data_159_128;
}DPP_PPU_PPU_INSTRMEM_W_DATA_159_128_T;

typedef struct dpp_ppu_ppu_instrmem_w_data_127_96_t
{
    ZXIC_UINT32 instrmem_w_data_127_96;
}DPP_PPU_PPU_INSTRMEM_W_DATA_127_96_T;

typedef struct dpp_ppu_ppu_instrmem_w_data_95_64_t
{
    ZXIC_UINT32 instrmem_w_data_95_64;
}DPP_PPU_PPU_INSTRMEM_W_DATA_95_64_T;

typedef struct dpp_ppu_ppu_instrmem_w_data_63_32_t
{
    ZXIC_UINT32 instrmem_w_data_63_32;
}DPP_PPU_PPU_INSTRMEM_W_DATA_63_32_T;

typedef struct dpp_ppu_ppu_instrmem_w_data_31_0_t
{
    ZXIC_UINT32 instrmem_w_data_31_0;
}DPP_PPU_PPU_INSTRMEM_W_DATA_31_0_T;

typedef struct dpp_ppu_ppu_isu_fwft_mf_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 isu_fwft_mf_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_ISU_FWFT_MF_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_isu_fwft_mf_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 isu_fwft_mf_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_ISU_FWFT_MF_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_int_1200m_mask_t
{
    ZXIC_UINT32 me7_interrupt_mask;
    ZXIC_UINT32 me6_interrupt_mask;
    ZXIC_UINT32 me5_interrupt_mask;
    ZXIC_UINT32 me4_interrupt_mask;
    ZXIC_UINT32 me3_interrupt_mask;
    ZXIC_UINT32 me2_interrupt_mask;
    ZXIC_UINT32 me1_interrupt_mask;
    ZXIC_UINT32 me0_interrupt_mask;
}DPP_PPU_CLUSTER_INT_1200M_MASK_T;

typedef struct dpp_ppu_ppu_interrupt_en_r_t
{
    ZXIC_UINT32 interrupt_en_r;
}DPP_PPU_PPU_INTERRUPT_EN_R_T;

typedef struct dpp_ppu_ppu_mec_host_interrupt_t
{
    ZXIC_UINT32 mec_host_interrupt;
}DPP_PPU_PPU_MEC_HOST_INTERRUPT_T;

typedef struct dpp_ppu_ppu_dbg_rtl_date_t
{
    ZXIC_UINT32 dbg_rtl_date;
}DPP_PPU_PPU_DBG_RTL_DATE_T;

typedef struct dpp_ppu_ppu_dup_start_num_cfg_t
{
    ZXIC_UINT32 dup_start_num_cfg;
}DPP_PPU_PPU_DUP_START_NUM_CFG_T;

typedef struct dpp_ppu_ppu_debug_data_write_complete_t
{
    ZXIC_UINT32 debug_data_write_complete;
}DPP_PPU_PPU_DEBUG_DATA_WRITE_COMPLETE_T;

typedef struct dpp_ppu_ppu_uc_mc_wrr_cfg_t
{
    ZXIC_UINT32 uc_mc_wrr_cfg;
}DPP_PPU_PPU_UC_MC_WRR_CFG_T;

typedef struct dpp_ppu_ppu_debug_pkt_send_en_t
{
    ZXIC_UINT32 debug_pkt_send_en;
}DPP_PPU_PPU_DEBUG_PKT_SEND_EN_T;

typedef struct dpp_ppu_ppu_dup_tbl_ind_access_done_t
{
    ZXIC_UINT32 dup_tbl_ind_access_done;
}DPP_PPU_PPU_DUP_TBL_IND_ACCESS_DONE_T;

typedef struct dpp_ppu_ppu_isu_ppu_demux_fifo_interrupt_mask_t
{
    ZXIC_UINT32 isu_in_para_fwft_fifo_32x81_wrapper_u0_overflow_mask;
    ZXIC_UINT32 isu_in_para_fwft_fifo_32x81_wrapper_u0_underflow_mask;
    ZXIC_UINT32 isu_in_fifo_64x81_wrapper_u0_overflow_mask;
    ZXIC_UINT32 isu_in_fifo_64x81_wrapper_u0_underflow_mask;
}DPP_PPU_PPU_ISU_PPU_DEMUX_FIFO_INTERRUPT_MASK_T;

typedef struct dpp_ppu_ppu_ppu_multicast_fifo_interrupt_mask_t
{
    ZXIC_UINT32 ppu_pktrx_mc_ptr_fifo_16384x17_wrapper_u0_underflow_mask;
    ZXIC_UINT32 ppu_pktrx_mc_ptr_fifo_16384x17_wrapper_u0_overflow_mask;
    ZXIC_UINT32 pf_req_fwft_fifo_16x36_wrapper_u0_overflow_mask;
    ZXIC_UINT32 pf_req_fwft_fifo_16x36_wrapper_u0_underflow_mask;
    ZXIC_UINT32 pf_rsp_fwft_fifo_32x34_wrapper_u0_overflow_mask;
    ZXIC_UINT32 pf_rsp_fwft_fifo_32x34_wrapper_u0_underflow_mask;
    ZXIC_UINT32 dup_para_fwft_fifo_16x35_wrapper_u0_overflow_mask;
    ZXIC_UINT32 dup_para_fwft_fifo_16x35_wrapper_u0_underflow_mask;
    ZXIC_UINT32 se_mc_rsp_fwft_fifo_32x17_wrapper_u0_overflow_mask;
    ZXIC_UINT32 se_mc_rsp_fwft_fifo_32x17_wrapper_u0_underflow_mask;
    ZXIC_UINT32 sa_para_fwft_fifo_64x17_wrapper_u0_overflow_mask;
    ZXIC_UINT32 sa_para_fwft_fifo_64x17_wrapper_u0_underflow_mask;
    ZXIC_UINT32 group_id_fifo_64x16_wrapper_u0_overflow_mask;
    ZXIC_UINT32 group_id_fifo_64x16_wrapper_u0_underflow_mask;
    ZXIC_UINT32 isu_mc_para_fwft_fifo_128x34_wrapper_u0_overflow_mask;
    ZXIC_UINT32 isu_mc_para_fwft_fifo_128x34_wrapper_u0_underflow_mask;
    ZXIC_UINT32 dup_freeptr_fwft_fifo_128x7_wrapper_u0_overflow_mask;
    ZXIC_UINT32 dup_freeptr_fwft_fifo_128x7_wrapper_u0_underflow_mask;
    ZXIC_UINT32 car_flag_fifo_32x1_wrapper_overflow_mask;
    ZXIC_UINT32 car_flag_fifo_32x1_wrapper_underflow_mask;
}DPP_PPU_PPU_PPU_MULTICAST_FIFO_INTERRUPT_MASK_T;

typedef struct dpp_ppu_ppu_ppu_in_schedule_fifo_interrupt_mask_t
{
    ZXIC_UINT32 free_global_num_fwft_fifo_8192x13_wrapper_u0_overflow_mask;
    ZXIC_UINT32 free_global_num_fwft_fifo_8192x13_wrapper_u0_underflow_mask;
    ZXIC_UINT32 mc_mf_fifo_16x2048_wrapper_u0_overflow_mask;
    ZXIC_UINT32 mc_mf_fifo_16x2048_wrapper_u0_underflow_mask;
    ZXIC_UINT32 uc_mf_fifo_96x2048_wrapper_u0_overflow_mask;
    ZXIC_UINT32 uc_mf_fifo_96x2048_wrapper_u0_underflow_mask;
}DPP_PPU_PPU_PPU_IN_SCHEDULE_FIFO_INTERRUPT_MASK_T;

typedef struct dpp_ppu_ppu_ppu_mf_out_fifo_interrupt_mask_t
{
    ZXIC_UINT32 ppu_cluster5_mf_out_afifo_32x2048_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cluster5_mf_out_afifo_32x2048_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_cluster4_mf_out_afifo_32x2048_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cluster4_mf_out_afifo_32x2048_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_cluster3_mf_out_afifo_32x2048_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cluster3_mf_out_afifo_32x2048_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_cluster2_mf_out_afifo_32x2048_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cluster2_mf_out_afifo_32x2048_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_cluster1_mf_out_afifo_32x2048_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cluster1_mf_out_afifo_32x2048_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_cluster0_mf_out_afifo_32x2048_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cluster0_mf_out_afifo_32x2048_wrapper_underflow_mask;
}DPP_PPU_PPU_PPU_MF_OUT_FIFO_INTERRUPT_MASK_T;

typedef struct dpp_ppu_ppu_pbu_mcode_pf_req_schedule_fifo_interrupt_mask_t
{
    ZXIC_UINT32 ppu_cluster5_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cluster4_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cluster3_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cluster2_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cluster1_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cluster0_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cluster5_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_cluster4_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_cluster3_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_cluster2_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_cluster1_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_cluster0_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_mask;
}DPP_PPU_PPU_PBU_MCODE_PF_REQ_SCHEDULE_FIFO_INTERRUPT_MASK_T;

typedef struct dpp_ppu_ppu_pbu_mcode_pf_rsp_schedule_fifo_interrupt_mask_t
{
    ZXIC_UINT32 ppu_pbu_mcode_pf_rsp_afifo_64x16_wrapper_u0r_underflow_mask;
    ZXIC_UINT32 ppu_pbu_mcode_pf_rsp_afifo_64x16_wrapper_u0_overflow_mask;
}DPP_PPU_PPU_PBU_MCODE_PF_RSP_SCHEDULE_FIFO_INTERRUPT_MASK_T;

typedef struct dpp_ppu_ppu_ppu_mccnt_fifo_interrupt_mask_t
{
    ZXIC_UINT32 ppu_mccnt_fifo_32x15_wrapper_u0_overflow_mask;
    ZXIC_UINT32 ppu_mccnt_fifo_32x15_wrapper_u0_underflow_mask;
    ZXIC_UINT32 ppu_wb_data_fifo_32x2048_wrapper_u0_overflow_mask;
    ZXIC_UINT32 ppu_wb_data_fifo_32x2048_wrapper_u0_underflow_mask;
    ZXIC_UINT32 mccnt_rsp_fifo_32x1_wrapper_u0_overflow_mask;
    ZXIC_UINT32 mccnt_rsp_fifo_32x1_wrapper_u0_underflow_mask;
}DPP_PPU_PPU_PPU_MCCNT_FIFO_INTERRUPT_MASK_T;

typedef struct dpp_ppu_ppu_coprocessor_fifo_interrupt_mask_l_t
{
    ZXIC_UINT32 mec3_cop_key_crc_fifo_32x625_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec3_cop_key_crc_fifo_32x625_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec3_cop_key_checksum_fifo_32x180_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec3_cop_key_checksum_fifo_32x180_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec3_cop_key_mul_fifo_32x52_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec3_cop_key_mul_fifo_32x52_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec3_cop_key_random_mod_fifo_32x44_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec3_cop_key_random_mod_fifo_32x44_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec2_cop_key_crc_fifo_32x625_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec2_cop_key_crc_fifo_32x625_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec2_cop_key_checksum_fifo_32x180_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec2_cop_key_checksum_fifo_32x180_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec2_cop_key_mul_fifo_32x52_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec2_cop_key_mul_fifo_32x52_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec2_cop_key_random_mod_fifo_32x44_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec2_cop_key_random_mod_fifo_32x44_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec1_cop_key_crc_fifo_32x625_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec1_cop_key_crc_fifo_32x625_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec1_cop_key_checksum_fifo_32x180_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec1_cop_key_checksum_fifo_32x180_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec1_cop_key_mul_fifo_32x52_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec1_cop_key_mul_fifo_32x52_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec1_cop_key_random_mod_fifo_32x44_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec1_cop_key_random_mod_fifo_32x44_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec0_cop_key_crc_fifo_32x625_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec0_cop_key_crc_fifo_32x625_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec0_cop_key_checksum_fifo_32x180_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec0_cop_key_checksum_fifo_32x180_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec0_cop_key_mul_fifo_32x52_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec0_cop_key_mul_fifo_32x52_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec0_cop_key_random_mod_fifo_32x44_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec0_cop_key_random_mod_fifo_32x44_wrapper_underflow_flg_mask;
}DPP_PPU_PPU_COPROCESSOR_FIFO_INTERRUPT_MASK_L_T;

typedef struct dpp_ppu_ppu_coprocessor_fifo_interrupt_mask_m_t
{
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_80x80_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_80x80_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_cop_delay_fifo_48x16_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cop_delay_fifo_48x16_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_cop_delay_fifo_16x48_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cop_delay_fifo_16x48_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_cop_delay_fifo_16x32_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cop_delay_fifo_16x32_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_96x80_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_96x80_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_cop_delay_fifo_16x16_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cop_delay_fifo_16x16_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_32x80_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_32x80_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_16x80_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_16x80_wrapper_underflow_mask;
    ZXIC_UINT32 mec5_cop_key_crc_fifo_32x625_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec5_cop_key_crc_fifo_32x625_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec5_cop_key_checksum_fifo_32x180_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec5_cop_key_checksum_fifo_32x180_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec5_cop_key_mul_fifo_32x52_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec5_cop_key_mul_fifo_32x52_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec5_cop_key_random_mod_fifo_32x44_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec5_cop_key_random_mod_fifo_32x44_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec4_cop_key_crc_fifo_32x625_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec4_cop_key_crc_fifo_32x625_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec4_cop_key_checksum_fifo_32x180_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec4_cop_key_checksum_fifo_32x180_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec4_cop_key_mul_fifo_32x52_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec4_cop_key_mul_fifo_32x52_wrapper_underflow_flg_mask;
    ZXIC_UINT32 mec4_cop_key_random_mod_fifo_32x44_wrapper_overflow_flg_mask;
    ZXIC_UINT32 mec4_cop_key_random_mod_fifo_32x44_wrapper_underflow_flg_mask;
}DPP_PPU_PPU_COPROCESSOR_FIFO_INTERRUPT_MASK_M_T;

typedef struct dpp_ppu_ppu_coprocessor_fifo_interrupt_mask_h_t
{
    ZXIC_UINT32 coprocessor_fwft_fifo_16x80_wrapper_overflow_mask;
    ZXIC_UINT32 coprocessor_fwft_fifo_16x80_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_cop_random_mod_para_delay_fifo_48x16_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cop_random_mod_para_delay_fifo_48x16_wrapper_underflow_mask;
}DPP_PPU_PPU_COPROCESSOR_FIFO_INTERRUPT_MASK_H_T;

typedef struct dpp_ppu_ppu_ppu_ram_check_err_mask_t
{
    ZXIC_UINT32 parity_err_mask;
}DPP_PPU_PPU_PPU_RAM_CHECK_ERR_MASK_T;

typedef struct dpp_ppu_ppu_instrmem_fifo_interrupt_mask_t
{
    ZXIC_UINT32 instrmem2_wr_fifo_ovf_mask;
    ZXIC_UINT32 instrmem2_wr_fifo_udf_mask;
    ZXIC_UINT32 instrmem2_rd_fifo_ovf_mask;
    ZXIC_UINT32 instrmem2_rd_fifo_udf_mask;
    ZXIC_UINT32 instrmem1_wr_fifo_ovf_mask;
    ZXIC_UINT32 instrmem1_wr_fifo_udf_mask;
    ZXIC_UINT32 instrmem1_rd_fifo_ovf_mask;
    ZXIC_UINT32 instrmem1_rd_fifo_udf_mask;
    ZXIC_UINT32 instrmem0_wr_fifo_ovf_mask;
    ZXIC_UINT32 instrmem0_wr_fifo_udf_mask;
    ZXIC_UINT32 instrmem0_rd_fifo_ovf_mask;
    ZXIC_UINT32 instrmem0_rd_fifo_udf_mask;
}DPP_PPU_PPU_INSTRMEM_FIFO_INTERRUPT_MASK_T;

typedef struct dpp_ppu_ppu_isu_ppu_demux_fifo_interrupt_sta_t
{
    ZXIC_UINT32 isu_in_para_fwft_fifo_32x81_wrapper_u0_overflow_sta;
    ZXIC_UINT32 isu_in_para_fwft_fifo_32x81_wrapper_u0_underflow_sta;
    ZXIC_UINT32 isu_in_fifo_64x81_wrapper_u0_overflow_sta;
    ZXIC_UINT32 isu_in_fifo_64x81_wrapper_u0_underflow_sta;
}DPP_PPU_PPU_ISU_PPU_DEMUX_FIFO_INTERRUPT_STA_T;

typedef struct dpp_ppu_ppu_ppu_multicast_fifo_interrupt_sta_t
{
    ZXIC_UINT32 ppu_pktrx_mc_ptr_fifo_16384x17_wrapper_u0_overflow_sta;
    ZXIC_UINT32 ppu_pktrx_mc_ptr_fifo_16384x17_wrapper_u0_underflow_sta;
    ZXIC_UINT32 pf_req_fwft_fifo_16x36_wrapper_u0_overflow_sta;
    ZXIC_UINT32 pf_req_fwft_fifo_16x36_wrapper_u0_underflow_sta;
    ZXIC_UINT32 pf_rsp_fwft_fifo_32x34_wrapper_u0_overflow_sta;
    ZXIC_UINT32 pf_rsp_fwft_fifo_32x34_wrapper_u0_underflow_sta;
    ZXIC_UINT32 dup_para_fwft_fifo_16x35_wrapper_u0_overflow_sta;
    ZXIC_UINT32 dup_para_fwft_fifo_16x35_wrapper_u0_underflow_sta;
    ZXIC_UINT32 se_mc_rsp_fwft_fifo_32x17_wrapper_u0_overflow_sta;
    ZXIC_UINT32 se_mc_rsp_fwft_fifo_32x17_wrapper_u0_underflow_sta;
    ZXIC_UINT32 sa_para_fwft_fifo_64x17_wrapper_u0_overflow_sta;
    ZXIC_UINT32 sa_para_fwft_fifo_64x17_wrapper_u0_underflow_sta;
    ZXIC_UINT32 group_id_fifo_64x16_wrapper_u0_overflow_sta;
    ZXIC_UINT32 group_id_fifo_64x16_wrapper_u0_underflow_sta;
    ZXIC_UINT32 isu_mc_para_fwft_fifo_128x34_wrapper_u0_overflow_sta;
    ZXIC_UINT32 isu_mc_para_fwft_fifo_128x34_wrapper_u0_underflow_sta;
    ZXIC_UINT32 dup_freeptr_fwft_fifo_128x7_wrapper_u0_overflow_sta;
    ZXIC_UINT32 dup_freeptr_fwft_fifo_128x7_wrapper_u0_underflow_sta;
    ZXIC_UINT32 car_flag_fifo_32x1_wrapper_overflow_sta;
    ZXIC_UINT32 car_flag_fifo_32x1_wrapper_underflow_sta;
}DPP_PPU_PPU_PPU_MULTICAST_FIFO_INTERRUPT_STA_T;

typedef struct dpp_ppu_ppu_ppu_in_schedule_fifo_interrupt_sta_t
{
    ZXIC_UINT32 free_global_num_fwft_fifo_8192x13_wrapper_u0_overflow_sta;
    ZXIC_UINT32 free_global_num_fwft_fifo_8192x13_wrapper_u0_underflow_sta;
    ZXIC_UINT32 mc_mf_fifo_16x2048_wrapper_u0_overflow_sta;
    ZXIC_UINT32 mc_mf_fifo_16x2048_wrapper_u0_underflow_sta;
    ZXIC_UINT32 uc_mf_fifo_96x2048_wrapper_u0_overflow_sta;
    ZXIC_UINT32 uc_mf_fifo_96x2048_wrapper_u0_underflow_sta;
}DPP_PPU_PPU_PPU_IN_SCHEDULE_FIFO_INTERRUPT_STA_T;

typedef struct dpp_ppu_ppu_ppu_mf_out_fifo_interrupt_sta_t
{
    ZXIC_UINT32 ppu_cluster5_mf_out_afifo_32x2048_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cluster5_mf_out_afifo_32x2048_wrapper_underflow_sta;
    ZXIC_UINT32 ppu_cluster4_mf_out_afifo_32x2048_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cluster4_mf_out_afifo_32x2048_wrapper_underflow_sta;
    ZXIC_UINT32 ppu_cluster3_mf_out_afifo_32x2048_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cluster3_mf_out_afifo_32x2048_wrapper_underflow_sta;
    ZXIC_UINT32 ppu_cluster2_mf_out_afifo_32x2048_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cluster2_mf_out_afifo_32x2048_wrapper_underflow_sta;
    ZXIC_UINT32 ppu_cluster1_mf_out_afifo_32x2048_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cluster1_mf_out_afifo_32x2048_wrapper_underflow_sta;
    ZXIC_UINT32 ppu_cluster0_mf_out_afifo_32x2048_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cluster0_mf_out_afifo_32x2048_wrapper_underflow_sta;
}DPP_PPU_PPU_PPU_MF_OUT_FIFO_INTERRUPT_STA_T;

typedef struct dpp_ppu_ppu_pbu_mcode_pf_req_schedule_fifo_interrupt_sta_t
{
    ZXIC_UINT32 ppu_cluster5_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cluster4_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cluster3_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cluster2_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cluster1_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cluster0_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cluster5_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_sta;
    ZXIC_UINT32 ppu_cluster4_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_sta;
    ZXIC_UINT32 ppu_cluster3_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_sta;
    ZXIC_UINT32 ppu_cluster2_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_sta;
    ZXIC_UINT32 ppu_cluster1_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_sta;
    ZXIC_UINT32 ppu_cluster0_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_sta;
}DPP_PPU_PPU_PBU_MCODE_PF_REQ_SCHEDULE_FIFO_INTERRUPT_STA_T;

typedef struct dpp_ppu_ppu_pbu_mcode_pf_rsp_schedule_fifo_interrupt_sta_t
{
    ZXIC_UINT32 ppu_pbu_mcode_pf_rsp_afifo_64x16_wrapper_u0r_underflow_sta;
    ZXIC_UINT32 ppu_pbu_mcode_pf_rsp_afifo_64x16_wrapper_u0_overflow_sta;
}DPP_PPU_PPU_PBU_MCODE_PF_RSP_SCHEDULE_FIFO_INTERRUPT_STA_T;

typedef struct dpp_ppu_ppu_ppu_mccnt_fifo_interrupt_sta_t
{
    ZXIC_UINT32 ppu_mccnt_fifo_32x15_wrapper_u0_overflow_sta;
    ZXIC_UINT32 ppu_mccnt_fifo_32x15_wrapper_u0_underflow_sta;
    ZXIC_UINT32 ppu_wb_data_fifo_32x2048_wrapper_u0_overflow_sta;
    ZXIC_UINT32 ppu_wb_data_fifo_32x2048_wrapper_u0_underflow_sta;
    ZXIC_UINT32 mccnt_rsp_fifo_32x1_wrapper_u0_overflow_sta;
    ZXIC_UINT32 mccnt_rsp_fifo_32x1_wrapper_u0_underflow_sta;
}DPP_PPU_PPU_PPU_MCCNT_FIFO_INTERRUPT_STA_T;

typedef struct dpp_ppu_ppu_coprocessor_fifo_interrupt_sta_l_t
{
    ZXIC_UINT32 mec3_cop_key_crc_fifo_32x625_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec3_cop_key_crc_fifo_32x625_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec3_cop_key_checksum_fifo_32x180_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec3_cop_key_checksum_fifo_32x180_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec3_cop_key_mul_fifo_32x52_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec3_cop_key_mul_fifo_32x52_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec3_cop_key_random_mod_fifo_32x44_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec3_cop_key_random_mod_fifo_32x44_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec2_cop_key_crc_fifo_32x625_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec2_cop_key_crc_fifo_32x625_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec2_cop_key_checksum_fifo_32x180_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec2_cop_key_checksum_fifo_32x180_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec2_cop_key_mul_fifo_32x52_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec2_cop_key_mul_fifo_32x52_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec2_cop_key_random_mod_fifo_32x44_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec2_cop_key_random_mod_fifo_32x44_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec1_cop_key_crc_fifo_32x625_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec1_cop_key_crc_fifo_32x625_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec1_cop_key_checksum_fifo_32x180_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec1_cop_key_checksum_fifo_32x180_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec1_cop_key_mul_fifo_32x52_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec1_cop_key_mul_fifo_32x52_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec1_cop_key_random_mod_fifo_32x44_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec1_cop_key_random_mod_fifo_32x44_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec0_cop_key_crc_fifo_32x625_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec0_cop_key_crc_fifo_32x625_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec0_cop_key_checksum_fifo_32x180_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec0_cop_key_checksum_fifo_32x180_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec0_cop_key_mul_fifo_32x52_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec0_cop_key_mul_fifo_32x52_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec0_cop_key_random_mod_fifo_32x44_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec0_cop_key_random_mod_fifo_32x44_wrapper_underflow_flg_sta;
}DPP_PPU_PPU_COPROCESSOR_FIFO_INTERRUPT_STA_L_T;

typedef struct dpp_ppu_ppu_coprocessor_fifo_interrupt_sta_m_t
{
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_80x80_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_80x80_wrapper_underflow_sta;
    ZXIC_UINT32 ppu_cop_delay_fifo_48x16_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cop_delay_fifo_48x16_wrapper_underflow_sta;
    ZXIC_UINT32 ppu_cop_delay_fifo_16x48_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cop_delay_fifo_16x48_wrapper_underflow_sta;
    ZXIC_UINT32 ppu_cop_delay_fifo_16x32_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cop_delay_fifo_16x32_wrapper_underflow_sta;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_96x80_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_96x80_wrapper_underflow_sta;
    ZXIC_UINT32 ppu_cop_delay_fifo_16x16_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cop_delay_fifo_16x16_wrapper_underflow_sta;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_32x80_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_32x80_wrapper_underflow_sta;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_16x80_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_16x80_wrapper_underflow_sta;
    ZXIC_UINT32 mec5_cop_key_crc_fifo_32x625_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec5_cop_key_crc_fifo_32x625_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec5_cop_key_checksum_fifo_32x180_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec5_cop_key_checksum_fifo_32x180_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec5_cop_key_mul_fifo_32x52_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec5_cop_key_mul_fifo_32x52_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec5_cop_key_random_mod_fifo_32x44_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec5_cop_key_random_mod_fifo_32x44_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec4_cop_key_crc_fifo_32x625_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec4_cop_key_crc_fifo_32x625_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec4_cop_key_checksum_fifo_32x180_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec4_cop_key_checksum_fifo_32x180_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec4_cop_key_mul_fifo_32x52_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec4_cop_key_mul_fifo_32x52_wrapper_underflow_flg_sta;
    ZXIC_UINT32 mec4_cop_key_random_mod_fifo_32x44_wrapper_overflow_flg_sta;
    ZXIC_UINT32 mec4_cop_key_random_mod_fifo_32x44_wrapper_underflow_flg_sta;
}DPP_PPU_PPU_COPROCESSOR_FIFO_INTERRUPT_STA_M_T;

typedef struct dpp_ppu_ppu_coprocessor_fifo_interrupt_sta_h_t
{
    ZXIC_UINT32 ppu_cop_random_mod_para_delay_fifo_48x16_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_cop_random_mod_para_delay_fifo_48x16_wrapper_underflow_sta;
}DPP_PPU_PPU_COPROCESSOR_FIFO_INTERRUPT_STA_H_T;

typedef struct dpp_ppu_ppu_instrmem_fifo_interrupt_sta_t
{
    ZXIC_UINT32 instrmem1_wr_fifo_ovf_sta;
    ZXIC_UINT32 instrmem1_wr_fifo_udf_sta;
    ZXIC_UINT32 instrmem1_rd_fifo_ovf_sta;
    ZXIC_UINT32 instrmem1_rd_fifo_udf_sta;
    ZXIC_UINT32 instrmem0_wr_fifo_ovf_sta;
    ZXIC_UINT32 instrmem0_wr_fifo_udf_sta;
    ZXIC_UINT32 instrmem0_rd_fifo_ovf_sta;
    ZXIC_UINT32 instrmem0_rd_fifo_udf_sta;
}DPP_PPU_PPU_INSTRMEM_FIFO_INTERRUPT_STA_T;

typedef struct dpp_ppu_ppu_ppu_ram_check_ecc_err_flag_1_t
{
    ZXIC_UINT32 ecc_single_err_sa_para_fifo_int_flag;
    ZXIC_UINT32 ecc_double_err_sa_para_fifo_int_flag;
    ZXIC_UINT32 ecc_single_err_dup_para_fifo_int_flag;
    ZXIC_UINT32 ecc_double_err_dup_para_fifo_int_flag;
    ZXIC_UINT32 ecc_single_err_pf_rsp_fifo_int_flag;
    ZXIC_UINT32 ecc_double_err_pf_rsp_fifo_int_flag;
    ZXIC_UINT32 ecc_single_err_pf_req_fifo_int_flag;
    ZXIC_UINT32 ecc_double_err_pf_req_fifo_int_flag;
    ZXIC_UINT32 ecc_single_err_ppu_reorder_link_ram0_int_flag;
    ZXIC_UINT32 ecc_double_err_ppu_reorder_link_ram0_int_flag;
    ZXIC_UINT32 ecc_single_err_ppu_reorder_link_ram1_int_flag;
    ZXIC_UINT32 ecc_double_err_ppu_reorder_link_ram1_int_flag;
    ZXIC_UINT32 ecc_single_err_ppu_reorder_link_flag_array_ram0_int_flag;
    ZXIC_UINT32 ecc_single_err_ppu_reorder_link_flag_array_ram1_int_flag;
    ZXIC_UINT32 ecc_single_err_ppu_reorder_ifb_ram_int_flag;
    ZXIC_UINT32 ecc_double_err_ppu_reorder_ifb_ram_int_flag;
    ZXIC_UINT32 ecc_single_err_ppu_reorder_flag_array_ram0_int_flag;
    ZXIC_UINT32 ecc_single_err_ppu_reorder_flag_array_ram1_int_flag;
    ZXIC_UINT32 ecc_single_err_ppu_reorder_flag_ram0_int_flag;
    ZXIC_UINT32 ecc_single_err_ppu_reorder_flag_ram1_int_flag;
    ZXIC_UINT32 ecc_single_err_uc_mf_fifo_int_flag;
    ZXIC_UINT32 ecc_double_err_uc_mf_fifo_int_flag;
    ZXIC_UINT32 ecc_single_err_mc_mf_fifo_int_flag;
    ZXIC_UINT32 ecc_double_err_mc_mf_fifo_int_flag;
    ZXIC_UINT32 ecc_single_err_free_global_num_fifo_int_flag;
    ZXIC_UINT32 ecc_double_err_free_global_num_fifo_int_flag;
}DPP_PPU_PPU_PPU_RAM_CHECK_ECC_ERR_FLAG_1_T;

typedef struct dpp_ppu_ppu_isu_ppu_demux_fifo_interrupt_flag_t
{
    ZXIC_UINT32 isu_in_para_fwft_fifo_32x81_wrapper_u0_overflow_flag;
    ZXIC_UINT32 isu_in_para_fwft_fifo_32x81_wrapper_u0_underflow_flag;
    ZXIC_UINT32 isu_in_fifo_64x81_wrapper_u0_overflow_flag;
    ZXIC_UINT32 isu_in_fifo_64x81_wrapper_u0_underflow_flag;
}DPP_PPU_PPU_ISU_PPU_DEMUX_FIFO_INTERRUPT_FLAG_T;

typedef struct dpp_ppu_ppu_ppu_multicast_fifo_interrupt_flag_t
{
    ZXIC_UINT32 ppu_pktrx_mc_ptr_fifo_16384x17_wrapper_u0_overflow_flag;
    ZXIC_UINT32 ppu_pktrx_mc_ptr_fifo_16384x17_wrapper_u0_underflow_flag;
    ZXIC_UINT32 pf_req_fwft_fifo_16x36_wrapper_u0_overflow_flag;
    ZXIC_UINT32 pf_req_fwft_fifo_16x36_wrapper_u0_underflow_flag;
    ZXIC_UINT32 pf_rsp_fwft_fifo_32x34_wrapper_u0_overflow_flag;
    ZXIC_UINT32 pf_rsp_fwft_fifo_32x34_wrapper_u0_underflow_flag;
    ZXIC_UINT32 dup_para_fwft_fifo_16x35_wrapper_u0_overflow_flag;
    ZXIC_UINT32 dup_para_fwft_fifo_16x35_wrapper_u0_underflow_flag;
    ZXIC_UINT32 se_mc_rsp_fwft_fifo_32x17_wrapper_u0_overflow_flag;
    ZXIC_UINT32 se_mc_rsp_fwft_fifo_32x17_wrapper_u0_underflow_flag;
    ZXIC_UINT32 sa_para_fwft_fifo_64x17_wrapper_u0_overflow_flag;
    ZXIC_UINT32 sa_para_fwft_fifo_64x17_wrapper_u0_underflow_flag;
    ZXIC_UINT32 group_id_fifo_64x16_wrapper_u0_overflow_flag;
    ZXIC_UINT32 group_id_fifo_64x16_wrapper_u0_underflow_flag;
    ZXIC_UINT32 isu_mc_para_fwft_fifo_128x34_wrapper_u0_overflow_flag;
    ZXIC_UINT32 isu_mc_para_fwft_fifo_128x34_wrapper_u0_underflow_flag;
    ZXIC_UINT32 dup_freeptr_fwft_fifo_128x7_wrapper_u0_overflow_flag;
    ZXIC_UINT32 dup_freeptr_fwft_fifo_128x7_wrapper_u0_underflow_flag;
    ZXIC_UINT32 car_flag_fifo_32x1_wrapper_overflow_flag;
    ZXIC_UINT32 car_flag_fifo_32x1_wrapper_underflow_flag;
}DPP_PPU_PPU_PPU_MULTICAST_FIFO_INTERRUPT_FLAG_T;

typedef struct dpp_ppu_ppu_ppu_in_schedule_fifo_interrupt_flag_t
{
    ZXIC_UINT32 free_global_num_fwft_fifo_8192x13_wrapper_u0_overflow_flag;
    ZXIC_UINT32 free_global_num_fwft_fifo_8192x13_wrapper_u0_underflow_flag;
    ZXIC_UINT32 mc_mf_fifo_16x2048_wrapper_u0_overflow_flag;
    ZXIC_UINT32 mc_mf_fifo_16x2048_wrapper_u0_underflow_flag;
    ZXIC_UINT32 uc_mf_fifo_96x2048_wrapper_u0_overflow_flag;
    ZXIC_UINT32 uc_mf_fifo_96x2048_wrapper_u0_underflow_flag;
}DPP_PPU_PPU_PPU_IN_SCHEDULE_FIFO_INTERRUPT_FLAG_T;

typedef struct dpp_ppu_ppu_ppu_mf_out_fifo_interrupt_flag_t
{
    ZXIC_UINT32 ppu_cluster5_mf_out_afifo_32x2048_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cluster5_mf_out_afifo_32x2048_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_cluster4_mf_out_afifo_32x2048_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cluster4_mf_out_afifo_32x2048_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_cluster3_mf_out_afifo_32x2048_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cluster3_mf_out_afifo_32x2048_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_cluster2_mf_out_afifo_32x2048_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cluster2_mf_out_afifo_32x2048_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_cluster1_mf_out_afifo_32x2048_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cluster1_mf_out_afifo_32x2048_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_cluster0_mf_out_afifo_32x2048_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cluster0_mf_out_afifo_32x2048_wrapper_underflow_flag;
}DPP_PPU_PPU_PPU_MF_OUT_FIFO_INTERRUPT_FLAG_T;

typedef struct dpp_ppu_ppu_pbu_mcode_pf_req_schedule_fifo_interrupt_flag_t
{
    ZXIC_UINT32 ppu_cluster5_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cluster4_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cluster3_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cluster2_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cluster1_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cluster0_pbu_mcode_pf_req_afifo_32x15_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cluster5_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_cluster4_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_cluster3_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_cluster2_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_cluster1_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_cluster0_pbu_mcode_pf_req_afifo_32x15_wrapper_underflow_flag;
}DPP_PPU_PPU_PBU_MCODE_PF_REQ_SCHEDULE_FIFO_INTERRUPT_FLAG_T;

typedef struct dpp_ppu_ppu_pbu_mcode_pf_rsp_schedule_fifo_interrupt_flag_t
{
    ZXIC_UINT32 ppu_pbu_mcode_pf_rsp_afifo_64x16_wrapper_u0r_underflow_flag;
    ZXIC_UINT32 ppu_pbu_mcode_pf_rsp_afifo_64x16_wrapper_u0_overflow_flag;
}DPP_PPU_PPU_PBU_MCODE_PF_RSP_SCHEDULE_FIFO_INTERRUPT_FLAG_T;

typedef struct dpp_ppu_ppu_ppu_mccnt_fifo_interrupt_flag_t
{
    ZXIC_UINT32 ppu_mccnt_fifo_32x15_wrapper_u0_overflow_flag;
    ZXIC_UINT32 ppu_mccnt_fifo_32x15_wrapper_u0_underflow_flag;
    ZXIC_UINT32 ppu_wb_data_fifo_32x2048_wrapper_u0_overflow_flag;
    ZXIC_UINT32 ppu_wb_data_fifo_32x2048_wrapper_u0_underflow_flag;
    ZXIC_UINT32 mccnt_rsp_fifo_32x1_wrapper_u0_overflow_flag;
    ZXIC_UINT32 mccnt_rsp_fifo_32x1_wrapper_u0_underflow_flag;
}DPP_PPU_PPU_PPU_MCCNT_FIFO_INTERRUPT_FLAG_T;

typedef struct dpp_ppu_ppu_coprocessor_fifo_interrupt_flag_l_t
{
    ZXIC_UINT32 mec3_cop_key_crc_fifo_32x625_wrapper_overflow_flag;
    ZXIC_UINT32 mec3_cop_key_crc_fifo_32x625_wrapper_underflow_flag;
    ZXIC_UINT32 mec3_cop_key_checksum_fifo_32x180_wrapper_overflow_flag;
    ZXIC_UINT32 mec3_cop_key_checksum_fifo_32x180_wrapper_underflow_flag;
    ZXIC_UINT32 mec3_cop_key_mul_fifo_32x52_wrapper_overflow_flag;
    ZXIC_UINT32 mec3_cop_key_mul_fifo_32x52_wrapper_underflow_flag;
    ZXIC_UINT32 mec3_cop_key_random_mod_fifo_32x44_wrapper_overflow_flag;
    ZXIC_UINT32 mec3_cop_key_random_mod_fifo_32x44_wrapper_underflow_flag;
    ZXIC_UINT32 mec2_cop_key_crc_fifo_32x625_wrapper_overflow_flag;
    ZXIC_UINT32 mec2_cop_key_crc_fifo_32x625_wrapper_underflow_flag;
    ZXIC_UINT32 mec2_cop_key_checksum_fifo_32x180_wrapper_overflow_flag;
    ZXIC_UINT32 mec2_cop_key_checksum_fifo_32x180_wrapper_underflow_flag;
    ZXIC_UINT32 mec2_cop_key_mul_fifo_32x52_wrapper_overflow_flag;
    ZXIC_UINT32 mec2_cop_key_mul_fifo_32x52_wrapper_underflow_flag;
    ZXIC_UINT32 mec2_cop_key_random_mod_fifo_32x44_wrapper_overflow_flag;
    ZXIC_UINT32 mec2_cop_key_random_mod_fifo_32x44_wrapper_underflow_flag;
    ZXIC_UINT32 mec1_cop_key_crc_fifo_32x625_wrapper_overflow_flag;
    ZXIC_UINT32 mec1_cop_key_crc_fifo_32x625_wrapper_underflow_flag;
    ZXIC_UINT32 mec1_cop_key_checksum_fifo_32x180_wrapper_overflow_flag;
    ZXIC_UINT32 mec1_cop_key_checksum_fifo_32x180_wrapper_underflow_flag;
    ZXIC_UINT32 mec1_cop_key_mul_fifo_32x52_wrapper_overflow_flag;
    ZXIC_UINT32 mec1_cop_key_mul_fifo_32x52_wrapper_underflow_flag;
    ZXIC_UINT32 mec1_cop_key_random_mod_fifo_32x44_wrapper_overflow_flag;
    ZXIC_UINT32 mec1_cop_key_random_mod_fifo_32x44_wrapper_underflow_flag;
    ZXIC_UINT32 mec0_cop_key_crc_fifo_32x625_wrapper_overflow_flag;
    ZXIC_UINT32 mec0_cop_key_crc_fifo_32x625_wrapper_underflow_flag;
    ZXIC_UINT32 mec0_cop_key_checksum_fifo_32x180_wrapper_overflow_flag;
    ZXIC_UINT32 mec0_cop_key_checksum_fifo_32x180_wrapper_underflow_flag;
    ZXIC_UINT32 mec0_cop_key_mul_fifo_32x52_wrapper_overflow_flag;
    ZXIC_UINT32 mec0_cop_key_mul_fifo_32x52_wrapper_underflow_flag;
    ZXIC_UINT32 mec0_cop_key_random_mod_fifo_32x44_wrapper_overflow_flag;
    ZXIC_UINT32 mec0_cop_key_random_mod_fifo_32x44_wrapper_underflow_flag;
}DPP_PPU_PPU_COPROCESSOR_FIFO_INTERRUPT_FLAG_L_T;

typedef struct dpp_ppu_ppu_coprocessor_fifo_interrupt_flag_m_t
{
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_80x80_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_80x80_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_cop_delay_fifo_48x16_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cop_delay_fifo_48x16_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_cop_delay_fifo_16x48_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cop_delay_fifo_16x48_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_cop_delay_fifo_16x32_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cop_delay_fifo_16x32_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_96x80_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_96x80_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_cop_delay_fifo_16x16_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cop_delay_fifo_16x16_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_32x80_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_32x80_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_16x80_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cop_result_fwft_fifo_16x80_wrapper_underflow_flag;
    ZXIC_UINT32 mec5_cop_key_crc_fifo_32x625_wrapper_overflow_flag;
    ZXIC_UINT32 mec5_cop_key_crc_fifo_32x625_wrapper_underflow_flag;
    ZXIC_UINT32 mec5_cop_key_checksum_fifo_32x180_wrapper_overflow_flag;
    ZXIC_UINT32 mec5_cop_key_checksum_fifo_32x180_wrapper_underflow_flag;
    ZXIC_UINT32 mec5_cop_key_mul_fifo_32x52_wrapper_overflow_flag;
    ZXIC_UINT32 mec5_cop_key_mul_fifo_32x52_wrapper_underflow_flag;
    ZXIC_UINT32 mec5_cop_key_random_mod_fifo_32x44_wrapper_overflow_flag;
    ZXIC_UINT32 mec5_cop_key_random_mod_fifo_32x44_wrapper_underflow_flag;
    ZXIC_UINT32 mec4_cop_key_crc_fifo_32x625_wrapper_overflow_flag;
    ZXIC_UINT32 mec4_cop_key_crc_fifo_32x625_wrapper_underflow_flag;
    ZXIC_UINT32 mec4_cop_key_checksum_fifo_32x180_wrapper_overflow_flag;
    ZXIC_UINT32 mec4_cop_key_checksum_fifo_32x180_wrapper_underflow_flag;
    ZXIC_UINT32 mec4_cop_key_mul_fifo_32x52_wrapper_overflow_flag;
    ZXIC_UINT32 mec4_cop_key_mul_fifo_32x52_wrapper_underflow_flag;
    ZXIC_UINT32 mec4_cop_key_random_mod_fifo_32x44_wrapper_overflow_flag;
    ZXIC_UINT32 mec4_cop_key_random_mod_fifo_32x44_wrapper_underflow_flag;
}DPP_PPU_PPU_COPROCESSOR_FIFO_INTERRUPT_FLAG_M_T;

typedef struct dpp_ppu_ppu_coprocessor_fifo_interrupt_flag_h_t
{
    ZXIC_UINT32 ppu_cop_random_mod_para_delay_fifo_48x16_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cop_random_mod_para_delay_fifo_48x16_wrapper_underflow_flag;
}DPP_PPU_PPU_COPROCESSOR_FIFO_INTERRUPT_FLAG_H_T;

typedef struct dpp_ppu_ppu_instrmem_fifo_interrupt_flag_t
{
    ZXIC_UINT32 instrmem2_wr_fifo_ovf_flag;
    ZXIC_UINT32 instrmem2_wr_fifo_udf_flag;
    ZXIC_UINT32 instrmem2_rd_fifo_ovf_flag;
    ZXIC_UINT32 instrmem2_rd_fifo_udf_flag;
    ZXIC_UINT32 instrmem1_wr_fifo_ovf_flag;
    ZXIC_UINT32 instrmem1_wr_fifo_udf_flag;
    ZXIC_UINT32 instrmem1_rd_fifo_ovf_flag;
    ZXIC_UINT32 instrmem1_rd_fifo_udf_flag;
    ZXIC_UINT32 instrmem0_wr_fifo_ovf_flag;
    ZXIC_UINT32 instrmem0_wr_fifo_udf_flag;
    ZXIC_UINT32 instrmem0_rd_fifo_ovf_flag;
    ZXIC_UINT32 instrmem0_rd_fifo_udf_flag;
}DPP_PPU_PPU_INSTRMEM_FIFO_INTERRUPT_FLAG_T;

typedef struct dpp_ppu_ppu_instrmem_ram_int_out_t
{
    ZXIC_UINT32 instrmem2_bank3_ram_parity_err_int_out;
    ZXIC_UINT32 instrmem2_bank2_ram_parity_err_int_out;
    ZXIC_UINT32 instrmem2_bank1_ram_parity_err_int_out;
    ZXIC_UINT32 instrmem2_bank0_ram_parity_err_int_out;
    ZXIC_UINT32 instrmem1_bank3_ram_parity_err_int_out;
    ZXIC_UINT32 instrmem1_bank2_ram_parity_err_int_out;
    ZXIC_UINT32 instrmem1_bank1_ram_parity_err_int_out;
    ZXIC_UINT32 instrmem1_bank0_ram_parity_err_int_out;
    ZXIC_UINT32 instrmem0_bank3_ram_parity_err_int_out;
    ZXIC_UINT32 instrmem0_bank2_ram_parity_err_int_out;
    ZXIC_UINT32 instrmem0_bank1_ram_parity_err_int_out;
    ZXIC_UINT32 instrmem0_bank0_ram_parity_err_int_out;
}DPP_PPU_PPU_INSTRMEM_RAM_INT_OUT_T;

typedef struct dpp_ppu_ppu_instrmem_ram_int_mask_t
{
    ZXIC_UINT32 instrmem2_bank3_ram_parity_err_mask;
    ZXIC_UINT32 instrmem2_bank2_ram_parity_err_mask;
    ZXIC_UINT32 instrmem2_bank1_ram_parity_err_mask;
    ZXIC_UINT32 instrmem2_bank0_ram_parity_err_mask;
    ZXIC_UINT32 instrmem1_bank3_ram_parity_err_mask;
    ZXIC_UINT32 instrmem1_bank2_ram_parity_err_mask;
    ZXIC_UINT32 instrmem1_bank1_ram_parity_err_mask;
    ZXIC_UINT32 instrmem1_bank0_ram_parity_err_mask;
    ZXIC_UINT32 instrmem0_bank3_ram_parity_err_mask;
    ZXIC_UINT32 instrmem0_bank2_ram_parity_err_mask;
    ZXIC_UINT32 instrmem0_bank1_ram_parity_err_mask;
    ZXIC_UINT32 instrmem0_bank0_ram_parity_err_mask;
}DPP_PPU_PPU_INSTRMEM_RAM_INT_MASK_T;

typedef struct dpp_ppu_ppu_instrmem_ram_int_stat_t
{
    ZXIC_UINT32 instrmem2_bank3_ram_parity_errstat;
    ZXIC_UINT32 instrmem2_bank2_ram_parity_errstat;
    ZXIC_UINT32 instrmem2_bank1_ram_parity_errstat;
    ZXIC_UINT32 instrmem2_bank0_ram_parity_errstat;
    ZXIC_UINT32 instrmem1_bank3_ram_parity_errstat;
    ZXIC_UINT32 instrmem1_bank2_ram_parity_errstat;
    ZXIC_UINT32 instrmem1_bank1_ram_parity_errstat;
    ZXIC_UINT32 instrmem1_bank0_ram_parity_errstat;
    ZXIC_UINT32 instrmem0_bank3_ram_parity_errstat;
    ZXIC_UINT32 instrmem0_bank2_ram_parity_errstat;
    ZXIC_UINT32 instrmem0_bank1_ram_parity_errstat;
    ZXIC_UINT32 instrmem0_bank0_ram_parity_errstat;
}DPP_PPU_PPU_INSTRMEM_RAM_INT_STAT_T;

typedef struct dpp_ppu_ppu_instrmem_ram_int_flag_t
{
    ZXIC_UINT32 instrmem2_bank3_ram_parity_err_flag;
    ZXIC_UINT32 instrmem2_bank2_ram_parity_err_flag;
    ZXIC_UINT32 instrmem2_bank1_ram_parity_err_flag;
    ZXIC_UINT32 instrmem2_bank0_ram_parity_err_flag;
    ZXIC_UINT32 instrmem1_bank3_ram_parity_err_flag;
    ZXIC_UINT32 instrmem1_bank2_ram_parity_err_flag;
    ZXIC_UINT32 instrmem1_bank1_ram_parity_err_flag;
    ZXIC_UINT32 instrmem1_bank0_ram_parity_err_flag;
    ZXIC_UINT32 instrmem0_bank3_ram_parity_err_flag;
    ZXIC_UINT32 instrmem0_bank2_ram_parity_err_flag;
    ZXIC_UINT32 instrmem0_bank1_ram_parity_err_flag;
    ZXIC_UINT32 instrmem0_bank0_ram_parity_err_flag;
}DPP_PPU_PPU_INSTRMEM_RAM_INT_FLAG_T;

typedef struct dpp_ppu_ppu_ppu_count_cfg_t
{
    ZXIC_UINT32 ppu_count_overflow_mode;
    ZXIC_UINT32 ppu_count_rd_mode;
}DPP_PPU_PPU_PPU_COUNT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_statics_cfg_t
{
    ZXIC_UINT32 csr_statics_mc_type;
    ZXIC_UINT32 csr_statics_bufnum;
    ZXIC_UINT32 csr_statics_portnum1;
    ZXIC_UINT32 csr_statics_portnum0;
}DPP_PPU_PPU_PPU_STATICS_CFG_T;

typedef struct dpp_ppu_ppu_ppu_statics_wb_cfg_t
{
    ZXIC_UINT32 csr_statics_wb_halt_send_type;
    ZXIC_UINT32 csr_statics_wb_mf_type;
    ZXIC_UINT32 csr_statics_wb_halt_continue_end;
    ZXIC_UINT32 csr_statics_wb_dup_flag;
    ZXIC_UINT32 csr_statics_wb_last_flag;
    ZXIC_UINT32 csr_statics_wb_dis_flag;
}DPP_PPU_PPU_PPU_STATICS_WB_CFG_T;

typedef struct dpp_ppu_ppu_wr_table_self_rsp_en_cfg_t
{
    ZXIC_UINT32 wr_table_self_rsp_en_cfg;
}DPP_PPU_PPU_WR_TABLE_SELF_RSP_EN_CFG_T;

typedef struct dpp_ppu_ppu_ppu_random_arbiter_8to1_cfg_t
{
    ZXIC_UINT32 ppu_random_arbiter_8to1_cfg;
}DPP_PPU_PPU_PPU_RANDOM_ARBITER_8TO1_CFG_T;

typedef struct dpp_ppu_ppu_ppu_reorder_bypass_flow_num_cfg_t
{
    ZXIC_UINT32 ppu_reorder_bypass_flow_num_cfg;
}DPP_PPU_PPU_PPU_REORDER_BYPASS_FLOW_NUM_CFG_T;

typedef struct dpp_ppu_ppu_cos_meter_cfg_h_t
{
    ZXIC_UINT32 cbs;
    ZXIC_UINT32 pbs;
    ZXIC_UINT32 green_action;
    ZXIC_UINT32 yellow_action;
    ZXIC_UINT32 red_action;
}DPP_PPU_PPU_COS_METER_CFG_H_T;

typedef struct dpp_ppu_ppu_cos_meter_cfg_l_t
{
    ZXIC_UINT32 cir;
    ZXIC_UINT32 pir;
    ZXIC_UINT32 car_en;
}DPP_PPU_PPU_COS_METER_CFG_L_T;

typedef struct dpp_ppu_ppu_instrmem_rdy_t
{
    ZXIC_UINT32 instrmem_rdy;
}DPP_PPU_PPU_INSTRMEM_RDY_T;

typedef struct dpp_ppu_ppu_instrmem_addr_t
{
    ZXIC_UINT32 instrmem_operate;
    ZXIC_UINT32 instrmem_addr;
}DPP_PPU_PPU_INSTRMEM_ADDR_T;

typedef struct dpp_ppu_ppu_instrmem_ind_access_done_t
{
    ZXIC_UINT32 instrmem_ind_access_done;
}DPP_PPU_PPU_INSTRMEM_IND_ACCESS_DONE_T;

typedef struct dpp_ppu_ppu_instrmem_instr0_data_l_t
{
    ZXIC_UINT32 instrmem_instr0_data_l;
}DPP_PPU_PPU_INSTRMEM_INSTR0_DATA_L_T;

typedef struct dpp_ppu_ppu_instrmem_instr0_data_h_t
{
    ZXIC_UINT32 instrmem_instr0_data_h;
}DPP_PPU_PPU_INSTRMEM_INSTR0_DATA_H_T;

typedef struct dpp_ppu_ppu_instrmem_instr1_data_l_t
{
    ZXIC_UINT32 instrmem_instr1_data_l;
}DPP_PPU_PPU_INSTRMEM_INSTR1_DATA_L_T;

typedef struct dpp_ppu_ppu_instrmem_instr1_data_h_t
{
    ZXIC_UINT32 instrmem_instr1_data_h;
}DPP_PPU_PPU_INSTRMEM_INSTR1_DATA_H_T;

typedef struct dpp_ppu_ppu_instrmem_instr2_data_l_t
{
    ZXIC_UINT32 instrmem_instr2_data_l;
}DPP_PPU_PPU_INSTRMEM_INSTR2_DATA_L_T;

typedef struct dpp_ppu_ppu_instrmem_instr2_data_h_t
{
    ZXIC_UINT32 instrmem_instr2_data_h;
}DPP_PPU_PPU_INSTRMEM_INSTR2_DATA_H_T;

typedef struct dpp_ppu_ppu_instrmem_instr3_data_l_t
{
    ZXIC_UINT32 instrmem_instr3_data_l;
}DPP_PPU_PPU_INSTRMEM_INSTR3_DATA_L_T;

typedef struct dpp_ppu_ppu_instrmem_instr3_data_h_t
{
    ZXIC_UINT32 instrmem_instr3_data_h;
}DPP_PPU_PPU_INSTRMEM_INSTR3_DATA_H_T;

typedef struct dpp_ppu_ppu_instrmem_read_instr0_data_l_t
{
    ZXIC_UINT32 instrmem_read_instr0_data_l;
}DPP_PPU_PPU_INSTRMEM_READ_INSTR0_DATA_L_T;

typedef struct dpp_ppu_ppu_instrmem_read_instr0_data_h_t
{
    ZXIC_UINT32 instrmem_read_instr0_data_h;
}DPP_PPU_PPU_INSTRMEM_READ_INSTR0_DATA_H_T;

typedef struct dpp_ppu_ppu_instrmem_read_instr1_data_l_t
{
    ZXIC_UINT32 instrmem_read_instr1_data_l;
}DPP_PPU_PPU_INSTRMEM_READ_INSTR1_DATA_L_T;

typedef struct dpp_ppu_ppu_instrmem_read_instr1_data_h_t
{
    ZXIC_UINT32 instrmem_read_instr1_data_h;
}DPP_PPU_PPU_INSTRMEM_READ_INSTR1_DATA_H_T;

typedef struct dpp_ppu_ppu_instrmem_read_instr2_data_l_t
{
    ZXIC_UINT32 instrmem_read_instr2_data_l;
}DPP_PPU_PPU_INSTRMEM_READ_INSTR2_DATA_L_T;

typedef struct dpp_ppu_ppu_instrmem_read_instr2_data_h_t
{
    ZXIC_UINT32 instrmem_read_instr2_data_h;
}DPP_PPU_PPU_INSTRMEM_READ_INSTR2_DATA_H_T;

typedef struct dpp_ppu_ppu_instrmem_read_instr3_data_l_t
{
    ZXIC_UINT32 instrmem_read_instr3_data_l;
}DPP_PPU_PPU_INSTRMEM_READ_INSTR3_DATA_L_T;

typedef struct dpp_ppu_ppu_instrmem_read_instr3_data_h_t
{
    ZXIC_UINT32 instrmem_read_instr3_data_h;
}DPP_PPU_PPU_INSTRMEM_READ_INSTR3_DATA_H_T;

typedef struct dpp_ppu_ppu_se_ppu_mc_srh_fc_cnt_h_t
{
    ZXIC_UINT32 se_ppu_mc_srh_fc_cnt_h;
}DPP_PPU_PPU_SE_PPU_MC_SRH_FC_CNT_H_T;

typedef struct dpp_ppu_ppu_se_ppu_mc_srh_fc_cnt_l_t
{
    ZXIC_UINT32 se_ppu_mc_srh_fc_cnt_l;
}DPP_PPU_PPU_SE_PPU_MC_SRH_FC_CNT_L_T;

typedef struct dpp_ppu_ppu_ppu_se_mc_srh_fc_cnt_h_t
{
    ZXIC_UINT32 ppu_se_mc_srh_fc_cnt_h;
}DPP_PPU_PPU_PPU_SE_MC_SRH_FC_CNT_H_T;

typedef struct dpp_ppu_ppu_ppu_se_mc_srh_fc_cnt_l_t
{
    ZXIC_UINT32 ppu_se_mc_srh_fc_cnt_l;
}DPP_PPU_PPU_PPU_SE_MC_SRH_FC_CNT_L_T;

typedef struct dpp_ppu_ppu_ppu_se_mc_srh_vld_cnt_h_t
{
    ZXIC_UINT32 ppu_se_mc_srh_vld_cnt_h;
}DPP_PPU_PPU_PPU_SE_MC_SRH_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_ppu_se_mc_srh_vld_cnt_l_t
{
    ZXIC_UINT32 ppu_se_mc_srh_vld_cnt_l;
}DPP_PPU_PPU_PPU_SE_MC_SRH_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_se_ppu_mc_srh_vld_cnt_h_t
{
    ZXIC_UINT32 se_ppu_mc_srh_vld_cnt_h;
}DPP_PPU_PPU_SE_PPU_MC_SRH_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_se_ppu_mc_srh_vld_cnt_l_t
{
    ZXIC_UINT32 se_ppu_mc_srh_vld_cnt_l;
}DPP_PPU_PPU_SE_PPU_MC_SRH_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_pbu_ppu_logic_pf_fc_cnt_h_t
{
    ZXIC_UINT32 pbu_ppu_logic_pf_fc_cnt_h;
}DPP_PPU_PPU_PBU_PPU_LOGIC_PF_FC_CNT_H_T;

typedef struct dpp_ppu_ppu_pbu_ppu_logic_pf_fc_cnt_l_t
{
    ZXIC_UINT32 pbu_ppu_logic_pf_fc_cnt_l;
}DPP_PPU_PPU_PBU_PPU_LOGIC_PF_FC_CNT_L_T;

typedef struct dpp_ppu_ppu_ppu_pbu_logic_rsp_fc_cnt_h_t
{
    ZXIC_UINT32 ppu_pbu_logic_rsp_fc_cnt_h;
}DPP_PPU_PPU_PPU_PBU_LOGIC_RSP_FC_CNT_H_T;

typedef struct dpp_ppu_ppu_ppu_pbu_logic_rsp_fc_cnt_l_t
{
    ZXIC_UINT32 ppu_pbu_logic_rsp_fc_cnt_l;
}DPP_PPU_PPU_PPU_PBU_LOGIC_RSP_FC_CNT_L_T;

typedef struct dpp_ppu_ppu_ppu_pbu_logic_pf_req_vld_cnt_h_t
{
    ZXIC_UINT32 ppu_pbu_logic_pf_req_vld_cnt_h;
}DPP_PPU_PPU_PPU_PBU_LOGIC_PF_REQ_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_ppu_pbu_logic_pf_req_vld_cnt_l_t
{
    ZXIC_UINT32 ppu_pbu_logic_pf_req_vld_cnt_l;
}DPP_PPU_PPU_PPU_PBU_LOGIC_PF_REQ_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_pbu_ppu_logic_pf_rsp_vld_cnt_h_t
{
    ZXIC_UINT32 pbu_ppu_logic_pf_rsp_vld_cnt_h;
}DPP_PPU_PPU_PBU_PPU_LOGIC_PF_RSP_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_pbu_ppu_logic_pf_rsp_vld_cnt_l_t
{
    ZXIC_UINT32 pbu_ppu_logic_pf_rsp_vld_cnt_l;
}DPP_PPU_PPU_PBU_PPU_LOGIC_PF_RSP_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_pbu_ppu_ifb_rd_fc_cnt_h_t
{
    ZXIC_UINT32 pbu_ppu_ifb_rd_fc_cnt_h;
}DPP_PPU_PPU_PBU_PPU_IFB_RD_FC_CNT_H_T;

typedef struct dpp_ppu_ppu_pbu_ppu_ifb_rd_fc_cnt_l_t
{
    ZXIC_UINT32 pbu_ppu_ifb_rd_fc_cnt_l;
}DPP_PPU_PPU_PBU_PPU_IFB_RD_FC_CNT_L_T;

typedef struct dpp_ppu_ppu_pbu_ppu_wb_fc_cnt_h_t
{
    ZXIC_UINT32 pbu_ppu_wb_fc_cnt_h;
}DPP_PPU_PPU_PBU_PPU_WB_FC_CNT_H_T;

typedef struct dpp_ppu_ppu_pbu_ppu_wb_fc_cnt_l_t
{
    ZXIC_UINT32 pbu_ppu_wb_fc_cnt_l;
}DPP_PPU_PPU_PBU_PPU_WB_FC_CNT_L_T;

typedef struct dpp_ppu_ppu_ppu_pbu_mcode_pf_req_vld_cnt_h_t
{
    ZXIC_UINT32 ppu_pbu_mcode_pf_req_vld_cnt_h;
}DPP_PPU_PPU_PPU_PBU_MCODE_PF_REQ_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_ppu_pbu_mcode_pf_req_vld_cnt_l_t
{
    ZXIC_UINT32 ppu_pbu_mcode_pf_req_vld_cnt_l;
}DPP_PPU_PPU_PPU_PBU_MCODE_PF_REQ_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_pbu_ppu_mcode_pf_rsp_vld_cnt_h_t
{
    ZXIC_UINT32 pbu_ppu_mcode_pf_rsp_vld_cnt_h;
}DPP_PPU_PPU_PBU_PPU_MCODE_PF_RSP_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_pbu_ppu_mcode_pf_rsp_vld_cnt_l_t
{
    ZXIC_UINT32 pbu_ppu_mcode_pf_rsp_vld_cnt_l;
}DPP_PPU_PPU_PBU_PPU_MCODE_PF_RSP_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_odma_ppu_para_fc_cnt_h_t
{
    ZXIC_UINT32 odma_ppu_para_fc_cnt_h;
}DPP_PPU_PPU_ODMA_PPU_PARA_FC_CNT_H_T;

typedef struct dpp_ppu_ppu_odma_ppu_para_fc_cnt_l_t
{
    ZXIC_UINT32 odma_ppu_para_fc_cnt_l;
}DPP_PPU_PPU_ODMA_PPU_PARA_FC_CNT_L_T;

typedef struct dpp_ppu_ppu_odma_ppu_mccnt_wr_fc_cnt_h_t
{
    ZXIC_UINT32 odma_ppu_mccnt_wr_fc_cnt_h;
}DPP_PPU_PPU_ODMA_PPU_MCCNT_WR_FC_CNT_H_T;

typedef struct dpp_ppu_ppu_odma_ppu_mccnt_wr_fc_cnt_l_t
{
    ZXIC_UINT32 odma_ppu_mccnt_wr_fc_cnt_l;
}DPP_PPU_PPU_ODMA_PPU_MCCNT_WR_FC_CNT_L_T;

typedef struct dpp_ppu_ppu_ppu_odma_mccnt_wr_vld_cnt_h_t
{
    ZXIC_UINT32 ppu_odma_mccnt_wr_vld_cnt_h;
}DPP_PPU_PPU_PPU_ODMA_MCCNT_WR_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_ppu_odma_mccnt_wr_vld_cnt_l_t
{
    ZXIC_UINT32 ppu_odma_mccnt_wr_vld_cnt_l;
}DPP_PPU_PPU_PPU_ODMA_MCCNT_WR_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_odma_ppu_mccnt_rsp_vld_cnt_h_t
{
    ZXIC_UINT32 odma_ppu_mccnt_rsp_vld_cnt_h;
}DPP_PPU_PPU_ODMA_PPU_MCCNT_RSP_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_odma_ppu_mccnt_rsp_vld_cnt_l_t
{
    ZXIC_UINT32 odma_ppu_mccnt_rsp_vld_cnt_l;
}DPP_PPU_PPU_ODMA_PPU_MCCNT_RSP_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_ppu_pktrx_uc_fc_cnt_h_t
{
    ZXIC_UINT32 ppu_pktrx_uc_fc_cnt_h;
}DPP_PPU_PPU_PPU_PKTRX_UC_FC_CNT_H_T;

typedef struct dpp_ppu_ppu_ppu_pktrx_uc_fc_cnt_l_t
{
    ZXIC_UINT32 ppu_pktrx_uc_fc_cnt_l;
}DPP_PPU_PPU_PPU_PKTRX_UC_FC_CNT_L_T;

typedef struct dpp_ppu_ppu_ppu_pktrx_mc_fc_cnt_h_t
{
    ZXIC_UINT32 ppu_pktrx_mc_fc_cnt_h;
}DPP_PPU_PPU_PPU_PKTRX_MC_FC_CNT_H_T;

typedef struct dpp_ppu_ppu_ppu_pktrx_mc_fc_cnt_l_t
{
    ZXIC_UINT32 ppu_pktrx_mc_fc_cnt_l;
}DPP_PPU_PPU_PPU_PKTRX_MC_FC_CNT_L_T;

typedef struct dpp_ppu_ppu_pktrx_ppu_desc_vld_cnt_h_t
{
    ZXIC_UINT32 pktrx_ppu_desc_vld_cnt_h;
}DPP_PPU_PPU_PKTRX_PPU_DESC_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_pktrx_ppu_desc_vld_cnt_l_t
{
    ZXIC_UINT32 pktrx_ppu_desc_vld_cnt_l;
}DPP_PPU_PPU_PKTRX_PPU_DESC_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_ppu_pbu_ifb_req_vld_cnt_h_t
{
    ZXIC_UINT32 ppu_pbu_ifb_req_vld_cnt_h;
}DPP_PPU_PPU_PPU_PBU_IFB_REQ_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_ppu_pbu_ifb_req_vld_cnt_l_t
{
    ZXIC_UINT32 ppu_pbu_ifb_req_vld_cnt_l;
}DPP_PPU_PPU_PPU_PBU_IFB_REQ_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_pbu_ppu_ifb_rsp_vld_cnt_h_t
{
    ZXIC_UINT32 pbu_ppu_ifb_rsp_vld_cnt_h;
}DPP_PPU_PPU_PBU_PPU_IFB_RSP_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_pbu_ppu_ifb_rsp_vld_cnt_l_t
{
    ZXIC_UINT32 pbu_ppu_ifb_rsp_vld_cnt_l;
}DPP_PPU_PPU_PBU_PPU_IFB_RSP_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_ppu_pbu_wb_vld_cnt_h_t
{
    ZXIC_UINT32 ppu_pbu_wb_vld_cnt_h;
}DPP_PPU_PPU_PPU_PBU_WB_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_ppu_pbu_wb_vld_cnt_l_t
{
    ZXIC_UINT32 ppu_pbu_wb_vld_cnt_l;
}DPP_PPU_PPU_PPU_PBU_WB_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_pbu_ppu_reorder_para_vld_cnt_h_t
{
    ZXIC_UINT32 pbu_ppu_reorder_para_vld_cnt_h;
}DPP_PPU_PPU_PBU_PPU_REORDER_PARA_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_pbu_ppu_reorder_para_vld_cnt_l_t
{
    ZXIC_UINT32 pbu_ppu_reorder_para_vld_cnt_l;
}DPP_PPU_PPU_PBU_PPU_REORDER_PARA_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_ppu_odma_para_vld_cnt_h_t
{
    ZXIC_UINT32 ppu_odma_para_vld_cnt_h;
}DPP_PPU_PPU_PPU_ODMA_PARA_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_ppu_odma_para_vld_cnt_l_t
{
    ZXIC_UINT32 ppu_odma_para_vld_cnt_l;
}DPP_PPU_PPU_PPU_ODMA_PARA_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_isu_ppu_mc_vld_cnt_h_t
{
    ZXIC_UINT32 statics_isu_ppu_mc_vld_cnt_h;
}DPP_PPU_PPU_STATICS_ISU_PPU_MC_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_isu_ppu_mc_vld_cnt_l_t
{
    ZXIC_UINT32 statics_isu_ppu_mc_vld_cnt_l;
}DPP_PPU_PPU_STATICS_ISU_PPU_MC_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_isu_ppu_mc_loop_vld_cnt_h_t
{
    ZXIC_UINT32 statics_isu_ppu_mc_loop_vld_cnt_h;
}DPP_PPU_PPU_STATICS_ISU_PPU_MC_LOOP_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_isu_ppu_mc_loop_vld_cnt_l_t
{
    ZXIC_UINT32 statics_isu_ppu_mc_loop_vld_cnt_l;
}DPP_PPU_PPU_STATICS_ISU_PPU_MC_LOOP_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_isu_ppu_uc_vld_cnt_h_t
{
    ZXIC_UINT32 statics_isu_ppu_uc_vld_cnt_h;
}DPP_PPU_PPU_STATICS_ISU_PPU_UC_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_isu_ppu_uc_vld_cnt_l_t
{
    ZXIC_UINT32 statics_isu_ppu_uc_vld_cnt_l;
}DPP_PPU_PPU_STATICS_ISU_PPU_UC_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_isu_ppu_uc_bufnumis0_vld_cnt_h_t
{
    ZXIC_UINT32 statics_isu_ppu_uc_bufnumis0_vld_cnt_h;
}DPP_PPU_PPU_STATICS_ISU_PPU_UC_BUFNUMIS0_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_isu_ppu_uc_bufnumis0_vld_cnt_l_t
{
    ZXIC_UINT32 statics_isu_ppu_uc_bufnumis0_vld_cnt_l;
}DPP_PPU_PPU_STATICS_ISU_PPU_UC_BUFNUMIS0_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_demux_schedule_mc_vld_cnt_h_t
{
    ZXIC_UINT32 statics_demux_schedule_mc_vld_cnt_h;
}DPP_PPU_PPU_STATICS_DEMUX_SCHEDULE_MC_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_demux_schedule_mc_vld_cnt_l_t
{
    ZXIC_UINT32 statics_demux_schedule_mc_vld_cnt_l;
}DPP_PPU_PPU_STATICS_DEMUX_SCHEDULE_MC_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_demux_schedule_mc_bufnumis0_vld_cnt_h_t
{
    ZXIC_UINT32 statics_demux_schedule_mc_bufnumis0_vld_cnt_h;
}DPP_PPU_PPU_STATICS_DEMUX_SCHEDULE_MC_BUFNUMIS0_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_demux_schedule_mc_bufnumis0_vld_cnt_l_t
{
    ZXIC_UINT32 statics_demux_schedule_mc_bufnumis0_vld_cnt_l;
}DPP_PPU_PPU_STATICS_DEMUX_SCHEDULE_MC_BUFNUMIS0_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_demux_schedule_mc_srcportis0_vld_cnt_h_t
{
    ZXIC_UINT32 statics_demux_schedule_mc_srcportis0_vld_cnt_h;
}DPP_PPU_PPU_STATICS_DEMUX_SCHEDULE_MC_SRCPORTIS0_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_demux_schedule_mc_srcportis0_vld_cnt_l_t
{
    ZXIC_UINT32 statics_demux_schedule_mc_srcportis0_vld_cnt_l;
}DPP_PPU_PPU_STATICS_DEMUX_SCHEDULE_MC_SRCPORTIS0_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_demux_schedule_mc_srcportis1_vld_cnt_h_t
{
    ZXIC_UINT32 statics_demux_schedule_mc_srcportis1_vld_cnt_h;
}DPP_PPU_PPU_STATICS_DEMUX_SCHEDULE_MC_SRCPORTIS1_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_demux_schedule_mc_srcportis1_vld_cnt_l_t
{
    ZXIC_UINT32 statics_demux_schedule_mc_srcportis1_vld_cnt_l;
}DPP_PPU_PPU_STATICS_DEMUX_SCHEDULE_MC_SRCPORTIS1_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_demux_schedule_uc_vld_cnt_h_t
{
    ZXIC_UINT32 statics_demux_schedule_uc_vld_cnt_h;
}DPP_PPU_PPU_STATICS_DEMUX_SCHEDULE_UC_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_demux_schedule_uc_vld_cnt_l_t
{
    ZXIC_UINT32 statics_demux_schedule_uc_vld_cnt_l;
}DPP_PPU_PPU_STATICS_DEMUX_SCHEDULE_UC_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_demux_schedule_uc_bufnumis0_vld_cnt_h_t
{
    ZXIC_UINT32 statics_demux_schedule_uc_bufnumis0_vld_cnt_h;
}DPP_PPU_PPU_STATICS_DEMUX_SCHEDULE_UC_BUFNUMIS0_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_demux_schedule_uc_bufnumis0_vld_cnt_l_t
{
    ZXIC_UINT32 statics_demux_schedule_uc_bufnumis0_vld_cnt_l;
}DPP_PPU_PPU_STATICS_DEMUX_SCHEDULE_UC_BUFNUMIS0_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_demux_schedule_uc_srcportis0_vld_cnt_h_t
{
    ZXIC_UINT32 statics_demux_schedule_uc_srcportis0_vld_cnt_h;
}DPP_PPU_PPU_STATICS_DEMUX_SCHEDULE_UC_SRCPORTIS0_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_demux_schedule_uc_srcportis0_vld_cnt_l_t
{
    ZXIC_UINT32 statics_demux_schedule_uc_srcportis0_vld_cnt_l;
}DPP_PPU_PPU_STATICS_DEMUX_SCHEDULE_UC_SRCPORTIS0_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_demux_schedule_uc_srcportis1_vld_cnt_h_t
{
    ZXIC_UINT32 statics_demux_schedule_uc_srcportis1_vld_cnt_h;
}DPP_PPU_PPU_STATICS_DEMUX_SCHEDULE_UC_SRCPORTIS1_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_demux_schedule_uc_srcportis1_vld_cnt_l_t
{
    ZXIC_UINT32 statics_demux_schedule_uc_srcportis1_vld_cnt_l;
}DPP_PPU_PPU_STATICS_DEMUX_SCHEDULE_UC_SRCPORTIS1_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_ppu_wb_vld_cnt_h_t
{
    ZXIC_UINT32 statics_ppu_wb_vld_cnt_h;
}DPP_PPU_PPU_STATICS_PPU_WB_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_ppu_wb_vld_cnt_l_t
{
    ZXIC_UINT32 statics_ppu_wb_vld_cnt_l;
}DPP_PPU_PPU_STATICS_PPU_WB_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_ppu_wb_bufnumis0_vld_cnt_h_t
{
    ZXIC_UINT32 statics_ppu_wb_bufnumis0_vld_cnt_h;
}DPP_PPU_PPU_STATICS_PPU_WB_BUFNUMIS0_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_ppu_wb_bufnumis0_vld_cnt_l_t
{
    ZXIC_UINT32 statics_ppu_wb_bufnumis0_vld_cnt_l;
}DPP_PPU_PPU_STATICS_PPU_WB_BUFNUMIS0_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_ppu_wb_srcportis0_vld_cnt_h_t
{
    ZXIC_UINT32 statics_ppu_wb_srcportis0_vld_cnt_h;
}DPP_PPU_PPU_STATICS_PPU_WB_SRCPORTIS0_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_ppu_wb_srcportis0_vld_cnt_l_t
{
    ZXIC_UINT32 statics_ppu_wb_srcportis0_vld_cnt_l;
}DPP_PPU_PPU_STATICS_PPU_WB_SRCPORTIS0_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_ppu_wb_srcportis1_vld_cnt_h_t
{
    ZXIC_UINT32 statics_ppu_wb_srcportis1_vld_cnt_h;
}DPP_PPU_PPU_STATICS_PPU_WB_SRCPORTIS1_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_ppu_wb_srcportis1_vld_cnt_l_t
{
    ZXIC_UINT32 statics_ppu_wb_srcportis1_vld_cnt_l;
}DPP_PPU_PPU_STATICS_PPU_WB_SRCPORTIS1_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_ppu_wb_halt_send_type_vld_cnt_h_t
{
    ZXIC_UINT32 statics_ppu_wb_halt_send_type_vld_cnt_h;
}DPP_PPU_PPU_STATICS_PPU_WB_HALT_SEND_TYPE_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_ppu_wb_halt_send_type_vld_cnt_l_t
{
    ZXIC_UINT32 statics_ppu_wb_halt_send_type_vld_cnt_l;
}DPP_PPU_PPU_STATICS_PPU_WB_HALT_SEND_TYPE_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_ppu_wb_mf_type_vld_cnt_h_t
{
    ZXIC_UINT32 statics_ppu_wb_mf_type_vld_cnt_h;
}DPP_PPU_PPU_STATICS_PPU_WB_MF_TYPE_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_ppu_wb_mf_type_vld_cnt_l_t
{
    ZXIC_UINT32 statics_ppu_wb_mf_type_vld_cnt_l;
}DPP_PPU_PPU_STATICS_PPU_WB_MF_TYPE_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_ppu_wb_halt_continue_end_vld_cnt_h_t
{
    ZXIC_UINT32 statics_ppu_wb_halt_continue_end_vld_cnt_h;
}DPP_PPU_PPU_STATICS_PPU_WB_HALT_CONTINUE_END_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_ppu_wb_halt_continue_end_vld_cnt_l_t
{
    ZXIC_UINT32 statics_ppu_wb_halt_continue_end_vld_cnt_l;
}DPP_PPU_PPU_STATICS_PPU_WB_HALT_CONTINUE_END_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_ppu_wb_dup_flag_vld_cnt_h_t
{
    ZXIC_UINT32 statics_ppu_wb_dup_flag_vld_cnt_h;
}DPP_PPU_PPU_STATICS_PPU_WB_DUP_FLAG_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_ppu_wb_dup_flag_vld_cnt_l_t
{
    ZXIC_UINT32 statics_ppu_wb_dup_flag_vld_cnt_l;
}DPP_PPU_PPU_STATICS_PPU_WB_DUP_FLAG_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_ppu_wb_last_flag_vld_cnt_h_t
{
    ZXIC_UINT32 statics_ppu_wb_last_flag_vld_cnt_h;
}DPP_PPU_PPU_STATICS_PPU_WB_LAST_FLAG_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_ppu_wb_last_flag_vld_cnt_l_t
{
    ZXIC_UINT32 statics_ppu_wb_last_flag_vld_cnt_l;
}DPP_PPU_PPU_STATICS_PPU_WB_LAST_FLAG_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_ppu_wb_dis_flag_vld_cnt_h_t
{
    ZXIC_UINT32 statics_ppu_wb_dis_flag_vld_cnt_h;
}DPP_PPU_PPU_STATICS_PPU_WB_DIS_FLAG_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_ppu_wb_dis_flag_vld_cnt_l_t
{
    ZXIC_UINT32 statics_ppu_wb_dis_flag_vld_cnt_l;
}DPP_PPU_PPU_STATICS_PPU_WB_DIS_FLAG_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_pbu_ppu_reorder_halt_send_type_vld_cnt_h_t
{
    ZXIC_UINT32 statics_pbu_ppu_reorder_halt_send_type_vld_cnt_h;
}DPP_PPU_PPU_STATICS_PBU_PPU_REORDER_HALT_SEND_TYPE_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_pbu_ppu_reorder_halt_send_type_vld_cnt_l_t
{
    ZXIC_UINT32 statics_pbu_ppu_reorder_halt_send_type_vld_cnt_l;
}DPP_PPU_PPU_STATICS_PBU_PPU_REORDER_HALT_SEND_TYPE_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_pbu_ppu_reorder_mf_type_vld_cnt_h_t
{
    ZXIC_UINT32 statics_pbu_ppu_reorder_mf_type_vld_cnt_h;
}DPP_PPU_PPU_STATICS_PBU_PPU_REORDER_MF_TYPE_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_pbu_ppu_reorder_mf_type_vld_cnt_l_t
{
    ZXIC_UINT32 statics_pbu_ppu_reorder_mf_type_vld_cnt_l;
}DPP_PPU_PPU_STATICS_PBU_PPU_REORDER_MF_TYPE_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_statics_pbu_ppu_reorder_halt_continue_end_vld_cnt_h_t
{
    ZXIC_UINT32 statics_pbu_ppu_reorder_halt_continue_end_vld_cnt_h;
}DPP_PPU_PPU_STATICS_PBU_PPU_REORDER_HALT_CONTINUE_END_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_statics_pbu_ppu_reorder_halt_continue_end_vld_cnt_l_t
{
    ZXIC_UINT32 statics_pbu_ppu_reorder_halt_continue_end_vld_cnt_l;
}DPP_PPU_PPU_STATICS_PBU_PPU_REORDER_HALT_CONTINUE_END_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_car_green_pkt_vld_cnt_h_t
{
    ZXIC_UINT32 car_green_pkt_vld_cnt_h;
}DPP_PPU_PPU_CAR_GREEN_PKT_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_car_green_pkt_vld_cnt_l_t
{
    ZXIC_UINT32 car_green_pkt_vld_cnt_l;
}DPP_PPU_PPU_CAR_GREEN_PKT_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_car_yellow_pkt_vld_cnt_h_t
{
    ZXIC_UINT32 car_yellow_pkt_vld_cnt_h;
}DPP_PPU_PPU_CAR_YELLOW_PKT_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_car_yellow_pkt_vld_cnt_l_t
{
    ZXIC_UINT32 car_yellow_pkt_vld_cnt_l;
}DPP_PPU_PPU_CAR_YELLOW_PKT_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_car_red_pkt_vld_cnt_h_t
{
    ZXIC_UINT32 car_red_pkt_vld_cnt_h;
}DPP_PPU_PPU_CAR_RED_PKT_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_car_red_pkt_vld_cnt_l_t
{
    ZXIC_UINT32 car_red_pkt_vld_cnt_l;
}DPP_PPU_PPU_CAR_RED_PKT_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_car_drop_pkt_vld_cnt_h_t
{
    ZXIC_UINT32 car_drop_pkt_vld_cnt_h;
}DPP_PPU_PPU_CAR_DROP_PKT_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_car_drop_pkt_vld_cnt_l_t
{
    ZXIC_UINT32 car_drop_pkt_vld_cnt_l;
}DPP_PPU_PPU_CAR_DROP_PKT_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_ppu_pktrx_mc_ptr_vld_cnt_h_t
{
    ZXIC_UINT32 ppu_pktrx_mc_ptr_vld_cnt_h;
}DPP_PPU_PPU_PPU_PKTRX_MC_PTR_VLD_CNT_H_T;

typedef struct dpp_ppu_ppu_ppu_pktrx_mc_ptr_vld_cnt_l_t
{
    ZXIC_UINT32 ppu_pktrx_mc_ptr_vld_cnt_l;
}DPP_PPU_PPU_PPU_PKTRX_MC_PTR_VLD_CNT_L_T;

typedef struct dpp_ppu_ppu_isu_ppu_loopback_fc_cnt_h_t
{
    ZXIC_UINT32 ppu_pktrx_mc_ptr_vld_cnt_h;
}DPP_PPU_PPU_ISU_PPU_LOOPBACK_FC_CNT_H_T;

typedef struct dpp_ppu_ppu_isu_ppu_loopback_fc_cnt_l_t
{
    ZXIC_UINT32 ppu_pktrx_mc_ptr_vld_cnt_l;
}DPP_PPU_PPU_ISU_PPU_LOOPBACK_FC_CNT_L_T;

typedef struct dpp_ppu_ppu_ppu_culster_pbu_mcode_pf_req_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_culster_pbu_mcode_pf_req_prog_full_assert_cfg;
}DPP_PPU_PPU_PPU_CULSTER_PBU_MCODE_PF_REQ_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_culster_pbu_mcode_pf_req_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_culster_pbu_mcode_pf_req_prog_full_negate_cfg;
}DPP_PPU_PPU_PPU_CULSTER_PBU_MCODE_PF_REQ_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_culster_pbu_mcode_pf_req_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_culster_pbu_mcode_pf_req_prog_empty_assert_cfg;
}DPP_PPU_PPU_PPU_CULSTER_PBU_MCODE_PF_REQ_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_culster_pbu_mcode_pf_req_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_culster_pbu_mcode_pf_req_prog_empty_negate_cfg;
}DPP_PPU_PPU_PPU_CULSTER_PBU_MCODE_PF_REQ_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_pbu_mcode_pf_rsp_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_pbu_mcode_pf_rsp_prog_full_assert_cfg;
}DPP_PPU_PPU_PPU_PBU_MCODE_PF_RSP_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_pbu_mcode_pf_rsp_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_pbu_mcode_pf_rsp_prog_full_negate_cfg;
}DPP_PPU_PPU_PPU_PBU_MCODE_PF_RSP_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_pbu_mcode_pf_rsp_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_pbu_mcode_pf_rsp_prog_empty_assert_cfg;
}DPP_PPU_PPU_PPU_PBU_MCODE_PF_RSP_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_pbu_mcode_pf_rsp_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_pbu_mcode_pf_rsp_prog_empty_negate_cfg;
}DPP_PPU_PPU_PPU_PBU_MCODE_PF_RSP_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_mccnt_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 mccnt_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_MCCNT_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_mccnt_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 mccnt_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_MCCNT_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_mccnt_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 mccnt_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_MCCNT_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_mccnt_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 mccnt_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_MCCNT_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_uc_mf_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 uc_mf_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_UC_MF_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_uc_mf_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 uc_mf_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_UC_MF_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_uc_mf_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 uc_mf_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_UC_MF_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_uc_mf_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 uc_mf_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_UC_MF_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_mc_mf_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 mc_mf_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_MC_MF_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_mc_mf_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 mc_mf_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_MC_MF_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_mc_mf_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 mc_mf_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_MC_MF_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_mc_mf_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 mc_mf_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_MC_MF_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_isu_mf_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 isu_mf_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_ISU_MF_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_isu_mf_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 isu_mf_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_ISU_MF_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_isu_mf_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 isu_mf_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_ISU_MF_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_isu_mf_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 isu_mf_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_ISU_MF_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_isu_fwft_mf_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 isu_fwft_mf_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_ISU_FWFT_MF_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_isu_fwft_mf_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 isu_fwft_mf_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_ISU_FWFT_MF_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_isu_mc_para_mf_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 isu_mc_para_mf_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_ISU_MC_PARA_MF_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_isu_mc_para_mf_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 isu_mc_para_mf_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_ISU_MC_PARA_MF_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_isu_mc_para_mf_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 isu_mc_para_mf_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_ISU_MC_PARA_MF_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_isu_mc_para_mf_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 isu_mc_para_mf_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_ISU_MC_PARA_MF_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_group_id_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 group_id_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_GROUP_ID_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_group_id_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 group_id_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_GROUP_ID_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_group_id_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 group_id_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_GROUP_ID_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_group_id_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 group_id_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_GROUP_ID_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_sa_para_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 sa_para_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_SA_PARA_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_sa_para_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 sa_para_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_SA_PARA_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_sa_para_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 sa_para_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_SA_PARA_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_sa_para_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 sa_para_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_SA_PARA_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_se_mc_rsp_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 se_mc_rsp_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_SE_MC_RSP_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_se_mc_rsp_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 se_mc_rsp_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_SE_MC_RSP_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_se_mc_rsp_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 se_mc_rsp_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_SE_MC_RSP_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_se_mc_rsp_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 se_mc_rsp_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_SE_MC_RSP_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_dup_para_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 dup_para_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_DUP_PARA_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_dup_para_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 dup_para_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_DUP_PARA_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_dup_para_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 dup_para_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_DUP_PARA_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_dup_para_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 dup_para_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_DUP_PARA_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_pf_rsp_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 pf_rsp_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_PF_RSP_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_pf_rsp_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 pf_rsp_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_PF_RSP_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_pf_rsp_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 pf_rsp_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_PF_RSP_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_pf_rsp_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 pf_rsp_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_PF_RSP_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_dup_freeptr_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 dup_freeptr_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_DUP_FREEPTR_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_dup_freeptr_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 dup_freeptr_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_DUP_FREEPTR_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_dup_freeptr_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 dup_freeptr_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_DUP_FREEPTR_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_dup_freeptr_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 dup_freeptr_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_DUP_FREEPTR_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_pf_req_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 pf_req_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_PF_REQ_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_pf_req_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 pf_req_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_PF_REQ_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_pf_req_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 pf_req_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_PF_REQ_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_pf_req_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 pf_req_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_PF_REQ_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_car_flag_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 car_flag_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_CAR_FLAG_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_car_flag_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 car_flag_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_CAR_FLAG_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_car_flag_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 car_flag_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_CAR_FLAG_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_car_flag_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 car_flag_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_CAR_FLAG_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cluster_mf_out_afifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_cluster_mf_out_afifo_prog_full_assert_cfg;
}DPP_PPU_PPU_PPU_CLUSTER_MF_OUT_AFIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cluster_mf_out_afifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_cluster_mf_out_afifo_prog_full_negate_cfg;
}DPP_PPU_PPU_PPU_CLUSTER_MF_OUT_AFIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cluster_mf_out_afifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_cluster_mf_out_afifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_PPU_CLUSTER_MF_OUT_AFIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cluster_mf_out_afifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_cluster_mf_out_afifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_PPU_CLUSTER_MF_OUT_AFIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_key_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_key_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_PPU_COP_KEY_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_key_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_key_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_PPU_COP_KEY_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_key_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_key_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_PPU_COP_KEY_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_key_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_key_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_PPU_COP_KEY_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_random_mod_para_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_random_mod_para_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_PPU_COP_RANDOM_MOD_PARA_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_random_mod_para_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_random_mod_para_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_PPU_COP_RANDOM_MOD_PARA_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_random_mod_para_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_random_mod_para_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_PPU_COP_RANDOM_MOD_PARA_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_random_mod_para_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_random_mod_para_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_PPU_COP_RANDOM_MOD_PARA_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_random_mod_result_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_random_mod_result_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_PPU_COP_RANDOM_MOD_RESULT_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_random_mod_result_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_random_mod_result_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_PPU_COP_RANDOM_MOD_RESULT_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_random_mod_result_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_random_mod_result_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_PPU_COP_RANDOM_MOD_RESULT_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_random_mod_result_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_random_mod_result_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_PPU_COP_RANDOM_MOD_RESULT_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_checksum_result_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_checksum_result_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_PPU_COP_CHECKSUM_RESULT_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_checksum_result_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_checksum_result_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_PPU_COP_CHECKSUM_RESULT_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_checksum_result_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_checksum_result_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_PPU_COP_CHECKSUM_RESULT_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_checksum_result_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_checksum_result_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_PPU_COP_CHECKSUM_RESULT_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_crc_first_para_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_crc_first_para_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_PPU_COP_CRC_FIRST_PARA_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_crc_first_para_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_crc_first_para_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_PPU_COP_CRC_FIRST_PARA_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_crc_first_para_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_crc_first_para_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_PPU_COP_CRC_FIRST_PARA_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_crc_first_para_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_crc_first_para_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_PPU_COP_CRC_FIRST_PARA_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_crc_bypass_delay_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_crc_bypass_delay_prog_full_assert_cfg;
}DPP_PPU_PPU_PPU_COP_CRC_BYPASS_DELAY_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_crc_bypass_delay_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_crc_bypass_delay_prog_full_negate_cfg;
}DPP_PPU_PPU_PPU_COP_CRC_BYPASS_DELAY_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_crc_bypass_delay_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_crc_bypass_delay_prog_empty_assert_cfg;
}DPP_PPU_PPU_PPU_COP_CRC_BYPASS_DELAY_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_crc_bypass_delay_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_crc_bypass_delay_prog_empty_negate_cfg;
}DPP_PPU_PPU_PPU_COP_CRC_BYPASS_DELAY_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_crc_second_para_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_crc_second_para_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_PPU_COP_CRC_SECOND_PARA_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_crc_second_para_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_crc_second_para_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_PPU_COP_CRC_SECOND_PARA_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_crc_second_para_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_crc_second_para_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_PPU_COP_CRC_SECOND_PARA_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_crc_second_para_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_crc_second_para_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_PPU_COP_CRC_SECOND_PARA_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_crc_result_fwft_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_crc_result_fwft_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_PPU_COP_CRC_RESULT_FWFT_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_crc_result_fwft_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_crc_result_fwft_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_PPU_COP_CRC_RESULT_FWFT_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_crc_result_fwft_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_crc_result_fwft_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_PPU_COP_CRC_RESULT_FWFT_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_crc_result_fwft_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_crc_result_fwft_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_PPU_COP_CRC_RESULT_FWFT_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_multiply_para_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_multiply_para_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_PPU_COP_MULTIPLY_PARA_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_multiply_para_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_multiply_para_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_PPU_COP_MULTIPLY_PARA_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_multiply_para_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_multiply_para_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_PPU_COP_MULTIPLY_PARA_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_multiply_para_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_multiply_para_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_PPU_COP_MULTIPLY_PARA_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_multiply_para_result_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_multiply_para_result_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_PPU_COP_MULTIPLY_PARA_RESULT_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_multiply_para_result_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_multiply_para_result_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_PPU_COP_MULTIPLY_PARA_RESULT_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_multiply_para_result_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_multiply_para_result_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_PPU_COP_MULTIPLY_PARA_RESULT_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_cop_multiply_para_result_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_multiply_para_result_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_PPU_COP_MULTIPLY_PARA_RESULT_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_free_global_num_fwft_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 free_global_num_fwft_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_FREE_GLOBAL_NUM_FWFT_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_free_global_num_fwft_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 free_global_num_fwft_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_FREE_GLOBAL_NUM_FWFT_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_free_global_num_fwft_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 free_global_num_fwft_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_FREE_GLOBAL_NUM_FWFT_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_free_global_num_fwft_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 free_global_num_fwft_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_FREE_GLOBAL_NUM_FWFT_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_pktrx_mc_ptr_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_pktrx_mc_ptr_fifo_prog_full_assert_cfg;
}DPP_PPU_PPU_PPU_PKTRX_MC_PTR_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_pktrx_mc_ptr_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_pktrx_mc_ptr_fifo_prog_full_negate_cfg;
}DPP_PPU_PPU_PPU_PKTRX_MC_PTR_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_ppu_pktrx_mc_ptr_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_pktrx_mc_ptr_fifo_prog_empty_assert_cfg;
}DPP_PPU_PPU_PPU_PKTRX_MC_PTR_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_ppu_ppu_pktrx_mc_ptr_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_pktrx_mc_ptr_fifo_prog_empty_negate_cfg;
}DPP_PPU_PPU_PPU_PKTRX_MC_PTR_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_ppu_pkt_data0_t
{
    ZXIC_UINT32 pkt_data0;
}DPP_PPU_PPU_PKT_DATA0_T;

typedef struct dpp_ppu_ppu_pkt_data1_t
{
    ZXIC_UINT32 pkt_data1;
}DPP_PPU_PPU_PKT_DATA1_T;

typedef struct dpp_ppu_ppu_pkt_data2_t
{
    ZXIC_UINT32 pkt_data2;
}DPP_PPU_PPU_PKT_DATA2_T;

typedef struct dpp_ppu_ppu_pkt_data3_t
{
    ZXIC_UINT32 pkt_data3;
}DPP_PPU_PPU_PKT_DATA3_T;

typedef struct dpp_ppu_ppu_pkt_data4_t
{
    ZXIC_UINT32 pkt_data4;
}DPP_PPU_PPU_PKT_DATA4_T;

typedef struct dpp_ppu_ppu_pkt_data5_t
{
    ZXIC_UINT32 pkt_data5;
}DPP_PPU_PPU_PKT_DATA5_T;

typedef struct dpp_ppu_ppu_pkt_data6_t
{
    ZXIC_UINT32 pkt_data6;
}DPP_PPU_PPU_PKT_DATA6_T;

typedef struct dpp_ppu_ppu_pkt_data7_t
{
    ZXIC_UINT32 pkt_data7;
}DPP_PPU_PPU_PKT_DATA7_T;

typedef struct dpp_ppu_ppu_pkt_data8_t
{
    ZXIC_UINT32 pkt_data8;
}DPP_PPU_PPU_PKT_DATA8_T;

typedef struct dpp_ppu_ppu_pkt_data9_t
{
    ZXIC_UINT32 pkt_data9;
}DPP_PPU_PPU_PKT_DATA9_T;

typedef struct dpp_ppu_ppu_pkt_data10_t
{
    ZXIC_UINT32 pkt_data10;
}DPP_PPU_PPU_PKT_DATA10_T;

typedef struct dpp_ppu_ppu_pkt_data11_t
{
    ZXIC_UINT32 pkt_data11;
}DPP_PPU_PPU_PKT_DATA11_T;

typedef struct dpp_ppu_ppu_pkt_data12_t
{
    ZXIC_UINT32 pkt_data12;
}DPP_PPU_PPU_PKT_DATA12_T;

typedef struct dpp_ppu_ppu_pkt_data13_t
{
    ZXIC_UINT32 pkt_data13;
}DPP_PPU_PPU_PKT_DATA13_T;

typedef struct dpp_ppu_ppu_pkt_data14_t
{
    ZXIC_UINT32 pkt_data14;
}DPP_PPU_PPU_PKT_DATA14_T;

typedef struct dpp_ppu_ppu_pkt_data15_t
{
    ZXIC_UINT32 pkt_data15;
}DPP_PPU_PPU_PKT_DATA15_T;

typedef struct dpp_ppu_ppu_pkt_data16_t
{
    ZXIC_UINT32 pkt_data16;
}DPP_PPU_PPU_PKT_DATA16_T;

typedef struct dpp_ppu_ppu_pkt_data17_t
{
    ZXIC_UINT32 pkt_data17;
}DPP_PPU_PPU_PKT_DATA17_T;

typedef struct dpp_ppu_ppu_pkt_data18_t
{
    ZXIC_UINT32 pkt_data18;
}DPP_PPU_PPU_PKT_DATA18_T;

typedef struct dpp_ppu_ppu_pkt_data19_t
{
    ZXIC_UINT32 pkt_data19;
}DPP_PPU_PPU_PKT_DATA19_T;

typedef struct dpp_ppu_ppu_pkt_data20_t
{
    ZXIC_UINT32 pkt_data20;
}DPP_PPU_PPU_PKT_DATA20_T;

typedef struct dpp_ppu_ppu_pkt_data21_t
{
    ZXIC_UINT32 pkt_data21;
}DPP_PPU_PPU_PKT_DATA21_T;

typedef struct dpp_ppu_ppu_pkt_data22_t
{
    ZXIC_UINT32 pkt_data22;
}DPP_PPU_PPU_PKT_DATA22_T;

typedef struct dpp_ppu_ppu_pkt_data23_t
{
    ZXIC_UINT32 pkt_data23;
}DPP_PPU_PPU_PKT_DATA23_T;

typedef struct dpp_ppu_ppu_pkt_data24_t
{
    ZXIC_UINT32 pkt_data24;
}DPP_PPU_PPU_PKT_DATA24_T;

typedef struct dpp_ppu_ppu_pkt_data25_t
{
    ZXIC_UINT32 pkt_data25;
}DPP_PPU_PPU_PKT_DATA25_T;

typedef struct dpp_ppu_ppu_pkt_data26_t
{
    ZXIC_UINT32 pkt_data26;
}DPP_PPU_PPU_PKT_DATA26_T;

typedef struct dpp_ppu_ppu_pkt_data27_t
{
    ZXIC_UINT32 pkt_data27;
}DPP_PPU_PPU_PKT_DATA27_T;

typedef struct dpp_ppu_ppu_pkt_data28_t
{
    ZXIC_UINT32 pkt_data28;
}DPP_PPU_PPU_PKT_DATA28_T;

typedef struct dpp_ppu_ppu_pkt_data29_t
{
    ZXIC_UINT32 pkt_data29;
}DPP_PPU_PPU_PKT_DATA29_T;

typedef struct dpp_ppu_ppu_pkt_data30_t
{
    ZXIC_UINT32 pkt_data30;
}DPP_PPU_PPU_PKT_DATA30_T;

typedef struct dpp_ppu_ppu_pkt_data31_t
{
    ZXIC_UINT32 pkt_data31;
}DPP_PPU_PPU_PKT_DATA31_T;

typedef struct dpp_ppu_ppu_pkt_data32_t
{
    ZXIC_UINT32 pkt_data32;
}DPP_PPU_PPU_PKT_DATA32_T;

typedef struct dpp_ppu_ppu_pkt_data33_t
{
    ZXIC_UINT32 pkt_data33;
}DPP_PPU_PPU_PKT_DATA33_T;

typedef struct dpp_ppu_ppu_pkt_data34_t
{
    ZXIC_UINT32 pkt_data34;
}DPP_PPU_PPU_PKT_DATA34_T;

typedef struct dpp_ppu_ppu_pkt_data35_t
{
    ZXIC_UINT32 pkt_data35;
}DPP_PPU_PPU_PKT_DATA35_T;

typedef struct dpp_ppu_ppu_pkt_data36_t
{
    ZXIC_UINT32 pkt_data36;
}DPP_PPU_PPU_PKT_DATA36_T;

typedef struct dpp_ppu_ppu_pkt_data37_t
{
    ZXIC_UINT32 pkt_data37;
}DPP_PPU_PPU_PKT_DATA37_T;

typedef struct dpp_ppu_ppu_pkt_data38_t
{
    ZXIC_UINT32 pkt_data38;
}DPP_PPU_PPU_PKT_DATA38_T;

typedef struct dpp_ppu_ppu_pkt_data39_t
{
    ZXIC_UINT32 pkt_data39;
}DPP_PPU_PPU_PKT_DATA39_T;

typedef struct dpp_ppu_ppu_pkt_data40_t
{
    ZXIC_UINT32 pkt_data40;
}DPP_PPU_PPU_PKT_DATA40_T;

typedef struct dpp_ppu_ppu_pkt_data41_t
{
    ZXIC_UINT32 pkt_data41;
}DPP_PPU_PPU_PKT_DATA41_T;

typedef struct dpp_ppu_ppu_pkt_data42_t
{
    ZXIC_UINT32 pkt_data42;
}DPP_PPU_PPU_PKT_DATA42_T;

typedef struct dpp_ppu_ppu_pkt_data43_t
{
    ZXIC_UINT32 pkt_data43;
}DPP_PPU_PPU_PKT_DATA43_T;

typedef struct dpp_ppu_ppu_pkt_data44_t
{
    ZXIC_UINT32 pkt_data44;
}DPP_PPU_PPU_PKT_DATA44_T;

typedef struct dpp_ppu_ppu_pkt_data45_t
{
    ZXIC_UINT32 pkt_data45;
}DPP_PPU_PPU_PKT_DATA45_T;

typedef struct dpp_ppu_ppu_pkt_data46_t
{
    ZXIC_UINT32 pkt_data46;
}DPP_PPU_PPU_PKT_DATA46_T;

typedef struct dpp_ppu_ppu_pkt_data47_t
{
    ZXIC_UINT32 pkt_data47;
}DPP_PPU_PPU_PKT_DATA47_T;

typedef struct dpp_ppu_ppu_pkt_data48_t
{
    ZXIC_UINT32 pkt_data48;
}DPP_PPU_PPU_PKT_DATA48_T;

typedef struct dpp_ppu_ppu_pkt_data49_t
{
    ZXIC_UINT32 pkt_data49;
}DPP_PPU_PPU_PKT_DATA49_T;

typedef struct dpp_ppu_ppu_pkt_data50_t
{
    ZXIC_UINT32 pkt_data50;
}DPP_PPU_PPU_PKT_DATA50_T;

typedef struct dpp_ppu_ppu_pkt_data51_t
{
    ZXIC_UINT32 pkt_data51;
}DPP_PPU_PPU_PKT_DATA51_T;

typedef struct dpp_ppu_ppu_pkt_data52_t
{
    ZXIC_UINT32 pkt_data52;
}DPP_PPU_PPU_PKT_DATA52_T;

typedef struct dpp_ppu_ppu_pkt_data53_t
{
    ZXIC_UINT32 pkt_data53;
}DPP_PPU_PPU_PKT_DATA53_T;

typedef struct dpp_ppu_ppu_pkt_data54_t
{
    ZXIC_UINT32 pkt_data54;
}DPP_PPU_PPU_PKT_DATA54_T;

typedef struct dpp_ppu_ppu_pkt_data55_t
{
    ZXIC_UINT32 pkt_data55;
}DPP_PPU_PPU_PKT_DATA55_T;

typedef struct dpp_ppu_ppu_pkt_data56_t
{
    ZXIC_UINT32 pkt_data56;
}DPP_PPU_PPU_PKT_DATA56_T;

typedef struct dpp_ppu_ppu_pkt_data57_t
{
    ZXIC_UINT32 pkt_data57;
}DPP_PPU_PPU_PKT_DATA57_T;

typedef struct dpp_ppu_ppu_pkt_data58_t
{
    ZXIC_UINT32 pkt_data58;
}DPP_PPU_PPU_PKT_DATA58_T;

typedef struct dpp_ppu_ppu_pkt_data59_t
{
    ZXIC_UINT32 pkt_data59;
}DPP_PPU_PPU_PKT_DATA59_T;

typedef struct dpp_ppu_ppu_pkt_data60_t
{
    ZXIC_UINT32 pkt_data60;
}DPP_PPU_PPU_PKT_DATA60_T;

typedef struct dpp_ppu_ppu_pkt_data61_t
{
    ZXIC_UINT32 pkt_data61;
}DPP_PPU_PPU_PKT_DATA61_T;

typedef struct dpp_ppu_ppu_pkt_data62_t
{
    ZXIC_UINT32 pkt_data62;
}DPP_PPU_PPU_PKT_DATA62_T;

typedef struct dpp_ppu_ppu_pkt_data63_t
{
    ZXIC_UINT32 pkt_data63;
}DPP_PPU_PPU_PKT_DATA63_T;

typedef struct dpp_ppu_ppu_pkt_data64_t
{
    ZXIC_UINT32 pkt_data64;
}DPP_PPU_PPU_PKT_DATA64_T;

typedef struct dpp_ppu_ppu_pkt_data65_t
{
    ZXIC_UINT32 pkt_data65;
}DPP_PPU_PPU_PKT_DATA65_T;

typedef struct dpp_ppu_ppu_pkt_data66_t
{
    ZXIC_UINT32 pkt_data66;
}DPP_PPU_PPU_PKT_DATA66_T;

typedef struct dpp_ppu_ppu_pkt_data67_t
{
    ZXIC_UINT32 pkt_data67;
}DPP_PPU_PPU_PKT_DATA67_T;

typedef struct dpp_ppu_ppu_pkt_data68_t
{
    ZXIC_UINT32 pkt_data68;
}DPP_PPU_PPU_PKT_DATA68_T;

typedef struct dpp_ppu_ppu_pkt_data69_t
{
    ZXIC_UINT32 pkt_data69;
}DPP_PPU_PPU_PKT_DATA69_T;

typedef struct dpp_ppu_ppu_pkt_data70_t
{
    ZXIC_UINT32 pkt_data70;
}DPP_PPU_PPU_PKT_DATA70_T;

typedef struct dpp_ppu_ppu_pkt_data71_t
{
    ZXIC_UINT32 pkt_data71;
}DPP_PPU_PPU_PKT_DATA71_T;

typedef struct dpp_ppu_ppu_pkt_data72_t
{
    ZXIC_UINT32 pkt_data72;
}DPP_PPU_PPU_PKT_DATA72_T;

typedef struct dpp_ppu_ppu_pkt_data73_t
{
    ZXIC_UINT32 pkt_data73;
}DPP_PPU_PPU_PKT_DATA73_T;

typedef struct dpp_ppu_ppu_pkt_data74_t
{
    ZXIC_UINT32 pkt_data74;
}DPP_PPU_PPU_PKT_DATA74_T;

typedef struct dpp_ppu_ppu_pkt_data75_t
{
    ZXIC_UINT32 pkt_data75;
}DPP_PPU_PPU_PKT_DATA75_T;

typedef struct dpp_ppu_ppu_pkt_data76_t
{
    ZXIC_UINT32 pkt_data76;
}DPP_PPU_PPU_PKT_DATA76_T;

typedef struct dpp_ppu_ppu_pkt_data77_t
{
    ZXIC_UINT32 pkt_data77;
}DPP_PPU_PPU_PKT_DATA77_T;

typedef struct dpp_ppu_ppu_pkt_data78_t
{
    ZXIC_UINT32 pkt_data78;
}DPP_PPU_PPU_PKT_DATA78_T;

typedef struct dpp_ppu_ppu_pkt_data79_t
{
    ZXIC_UINT32 pkt_data79;
}DPP_PPU_PPU_PKT_DATA79_T;

typedef struct dpp_ppu_ppu_pkt_data80_t
{
    ZXIC_UINT32 pkt_data80;
}DPP_PPU_PPU_PKT_DATA80_T;

typedef struct dpp_ppu_ppu_pkt_data81_t
{
    ZXIC_UINT32 pkt_data81;
}DPP_PPU_PPU_PKT_DATA81_T;

typedef struct dpp_ppu_ppu_pkt_data82_t
{
    ZXIC_UINT32 pkt_data82;
}DPP_PPU_PPU_PKT_DATA82_T;

typedef struct dpp_ppu_ppu_pkt_data83_t
{
    ZXIC_UINT32 pkt_data83;
}DPP_PPU_PPU_PKT_DATA83_T;

typedef struct dpp_ppu_ppu_pkt_data84_t
{
    ZXIC_UINT32 pkt_data84;
}DPP_PPU_PPU_PKT_DATA84_T;

typedef struct dpp_ppu_ppu_pkt_data85_t
{
    ZXIC_UINT32 pkt_data85;
}DPP_PPU_PPU_PKT_DATA85_T;

typedef struct dpp_ppu_ppu_pkt_data86_t
{
    ZXIC_UINT32 pkt_data86;
}DPP_PPU_PPU_PKT_DATA86_T;

typedef struct dpp_ppu_ppu_pkt_data87_t
{
    ZXIC_UINT32 pkt_data87;
}DPP_PPU_PPU_PKT_DATA87_T;

typedef struct dpp_ppu_ppu_pkt_data88_t
{
    ZXIC_UINT32 pkt_data88;
}DPP_PPU_PPU_PKT_DATA88_T;

typedef struct dpp_ppu_ppu_pkt_data89_t
{
    ZXIC_UINT32 pkt_data89;
}DPP_PPU_PPU_PKT_DATA89_T;

typedef struct dpp_ppu_ppu_pkt_data90_t
{
    ZXIC_UINT32 pkt_data90;
}DPP_PPU_PPU_PKT_DATA90_T;

typedef struct dpp_ppu_ppu_pkt_data91_t
{
    ZXIC_UINT32 pkt_data91;
}DPP_PPU_PPU_PKT_DATA91_T;

typedef struct dpp_ppu_ppu_pkt_data92_t
{
    ZXIC_UINT32 pkt_data92;
}DPP_PPU_PPU_PKT_DATA92_T;

typedef struct dpp_ppu_ppu_pkt_data93_t
{
    ZXIC_UINT32 pkt_data93;
}DPP_PPU_PPU_PKT_DATA93_T;

typedef struct dpp_ppu_ppu_pkt_data94_t
{
    ZXIC_UINT32 pkt_data94;
}DPP_PPU_PPU_PKT_DATA94_T;

typedef struct dpp_ppu_ppu_pkt_data95_t
{
    ZXIC_UINT32 pkt_data95;
}DPP_PPU_PPU_PKT_DATA95_T;

typedef struct dpp_ppu_ppu_pkt_data96_t
{
    ZXIC_UINT32 pkt_data96;
}DPP_PPU_PPU_PKT_DATA96_T;

typedef struct dpp_ppu_ppu_pkt_data97_t
{
    ZXIC_UINT32 pkt_data97;
}DPP_PPU_PPU_PKT_DATA97_T;

typedef struct dpp_ppu_ppu_pkt_data98_t
{
    ZXIC_UINT32 pkt_data98;
}DPP_PPU_PPU_PKT_DATA98_T;

typedef struct dpp_ppu_ppu_pkt_data99_t
{
    ZXIC_UINT32 pkt_data99;
}DPP_PPU_PPU_PKT_DATA99_T;

typedef struct dpp_ppu_ppu_pkt_data100_t
{
    ZXIC_UINT32 pkt_data100;
}DPP_PPU_PPU_PKT_DATA100_T;

typedef struct dpp_ppu_ppu_pkt_data101_t
{
    ZXIC_UINT32 pkt_data101;
}DPP_PPU_PPU_PKT_DATA101_T;

typedef struct dpp_ppu_ppu_pkt_data102_t
{
    ZXIC_UINT32 pkt_data102;
}DPP_PPU_PPU_PKT_DATA102_T;

typedef struct dpp_ppu_ppu_pkt_data103_t
{
    ZXIC_UINT32 pkt_data103;
}DPP_PPU_PPU_PKT_DATA103_T;

typedef struct dpp_ppu_ppu_pkt_data104_t
{
    ZXIC_UINT32 pkt_data104;
}DPP_PPU_PPU_PKT_DATA104_T;

typedef struct dpp_ppu_ppu_pkt_data105_t
{
    ZXIC_UINT32 pkt_data105;
}DPP_PPU_PPU_PKT_DATA105_T;

typedef struct dpp_ppu_ppu_pkt_data106_t
{
    ZXIC_UINT32 pkt_data106;
}DPP_PPU_PPU_PKT_DATA106_T;

typedef struct dpp_ppu_ppu_pkt_data107_t
{
    ZXIC_UINT32 pkt_data107;
}DPP_PPU_PPU_PKT_DATA107_T;

typedef struct dpp_ppu_ppu_pkt_data108_t
{
    ZXIC_UINT32 pkt_data108;
}DPP_PPU_PPU_PKT_DATA108_T;

typedef struct dpp_ppu_ppu_pkt_data109_t
{
    ZXIC_UINT32 pkt_data109;
}DPP_PPU_PPU_PKT_DATA109_T;

typedef struct dpp_ppu_ppu_pkt_data110_t
{
    ZXIC_UINT32 pkt_data110;
}DPP_PPU_PPU_PKT_DATA110_T;

typedef struct dpp_ppu_ppu_pkt_data111_t
{
    ZXIC_UINT32 pkt_data111;
}DPP_PPU_PPU_PKT_DATA111_T;

typedef struct dpp_ppu_ppu_pkt_data112_t
{
    ZXIC_UINT32 pkt_data112;
}DPP_PPU_PPU_PKT_DATA112_T;

typedef struct dpp_ppu_ppu_pkt_data113_t
{
    ZXIC_UINT32 pkt_data113;
}DPP_PPU_PPU_PKT_DATA113_T;

typedef struct dpp_ppu_ppu_pkt_data114_t
{
    ZXIC_UINT32 pkt_data114;
}DPP_PPU_PPU_PKT_DATA114_T;

typedef struct dpp_ppu_ppu_pkt_data115_t
{
    ZXIC_UINT32 pkt_data115;
}DPP_PPU_PPU_PKT_DATA115_T;

typedef struct dpp_ppu_ppu_pkt_data116_t
{
    ZXIC_UINT32 pkt_data116;
}DPP_PPU_PPU_PKT_DATA116_T;

typedef struct dpp_ppu_ppu_pkt_data117_t
{
    ZXIC_UINT32 pkt_data117;
}DPP_PPU_PPU_PKT_DATA117_T;

typedef struct dpp_ppu_ppu_pkt_data118_t
{
    ZXIC_UINT32 pkt_data118;
}DPP_PPU_PPU_PKT_DATA118_T;

typedef struct dpp_ppu_ppu_pkt_data119_t
{
    ZXIC_UINT32 pkt_data119;
}DPP_PPU_PPU_PKT_DATA119_T;

typedef struct dpp_ppu_ppu_pkt_data120_t
{
    ZXIC_UINT32 pkt_data120;
}DPP_PPU_PPU_PKT_DATA120_T;

typedef struct dpp_ppu_ppu_pkt_data121_t
{
    ZXIC_UINT32 pkt_data121;
}DPP_PPU_PPU_PKT_DATA121_T;

typedef struct dpp_ppu_ppu_pkt_data122_t
{
    ZXIC_UINT32 pkt_data122;
}DPP_PPU_PPU_PKT_DATA122_T;

typedef struct dpp_ppu_ppu_pkt_data123_t
{
    ZXIC_UINT32 pkt_data123;
}DPP_PPU_PPU_PKT_DATA123_T;

typedef struct dpp_ppu_ppu_pkt_data124_t
{
    ZXIC_UINT32 pkt_data124;
}DPP_PPU_PPU_PKT_DATA124_T;

typedef struct dpp_ppu_ppu_pkt_data125_t
{
    ZXIC_UINT32 pkt_data125;
}DPP_PPU_PPU_PKT_DATA125_T;

typedef struct dpp_ppu_ppu_pkt_data126_t
{
    ZXIC_UINT32 pkt_data126;
}DPP_PPU_PPU_PKT_DATA126_T;

typedef struct dpp_ppu_ppu_pkt_data127_t
{
    ZXIC_UINT32 pkt_data127;
}DPP_PPU_PPU_PKT_DATA127_T;

typedef struct dpp_ppu_ppu_spr0_t
{
    ZXIC_UINT32 spr0;
}DPP_PPU_PPU_SPR0_T;

typedef struct dpp_ppu_ppu_spr1_t
{
    ZXIC_UINT32 spr1;
}DPP_PPU_PPU_SPR1_T;

typedef struct dpp_ppu_ppu_spr2_t
{
    ZXIC_UINT32 spr2;
}DPP_PPU_PPU_SPR2_T;

typedef struct dpp_ppu_ppu_spr3_t
{
    ZXIC_UINT32 spr3;
}DPP_PPU_PPU_SPR3_T;

typedef struct dpp_ppu_ppu_spr4_t
{
    ZXIC_UINT32 spr4;
}DPP_PPU_PPU_SPR4_T;

typedef struct dpp_ppu_ppu_spr5_t
{
    ZXIC_UINT32 spr5;
}DPP_PPU_PPU_SPR5_T;

typedef struct dpp_ppu_ppu_spr6_t
{
    ZXIC_UINT32 spr6;
}DPP_PPU_PPU_SPR6_T;

typedef struct dpp_ppu_ppu_spr7_t
{
    ZXIC_UINT32 spr7;
}DPP_PPU_PPU_SPR7_T;

typedef struct dpp_ppu_ppu_spr8_t
{
    ZXIC_UINT32 spr8;
}DPP_PPU_PPU_SPR8_T;

typedef struct dpp_ppu_ppu_spr9_t
{
    ZXIC_UINT32 spr9;
}DPP_PPU_PPU_SPR9_T;

typedef struct dpp_ppu_ppu_spr10_t
{
    ZXIC_UINT32 spr10;
}DPP_PPU_PPU_SPR10_T;

typedef struct dpp_ppu_ppu_spr11_t
{
    ZXIC_UINT32 spr11;
}DPP_PPU_PPU_SPR11_T;

typedef struct dpp_ppu_ppu_spr12_t
{
    ZXIC_UINT32 spr12;
}DPP_PPU_PPU_SPR12_T;

typedef struct dpp_ppu_ppu_spr13_t
{
    ZXIC_UINT32 spr13;
}DPP_PPU_PPU_SPR13_T;

typedef struct dpp_ppu_ppu_spr14_t
{
    ZXIC_UINT32 spr14;
}DPP_PPU_PPU_SPR14_T;

typedef struct dpp_ppu_ppu_spr15_t
{
    ZXIC_UINT32 spr15;
}DPP_PPU_PPU_SPR15_T;

typedef struct dpp_ppu_ppu_spr16_t
{
    ZXIC_UINT32 spr16;
}DPP_PPU_PPU_SPR16_T;

typedef struct dpp_ppu_ppu_spr17_t
{
    ZXIC_UINT32 spr17;
}DPP_PPU_PPU_SPR17_T;

typedef struct dpp_ppu_ppu_spr18_t
{
    ZXIC_UINT32 spr18;
}DPP_PPU_PPU_SPR18_T;

typedef struct dpp_ppu_ppu_spr19_t
{
    ZXIC_UINT32 spr19;
}DPP_PPU_PPU_SPR19_T;

typedef struct dpp_ppu_ppu_spr20_t
{
    ZXIC_UINT32 spr20;
}DPP_PPU_PPU_SPR20_T;

typedef struct dpp_ppu_ppu_spr21_t
{
    ZXIC_UINT32 spr21;
}DPP_PPU_PPU_SPR21_T;

typedef struct dpp_ppu_ppu_spr22_t
{
    ZXIC_UINT32 spr22;
}DPP_PPU_PPU_SPR22_T;

typedef struct dpp_ppu_ppu_spr23_t
{
    ZXIC_UINT32 spr23;
}DPP_PPU_PPU_SPR23_T;

typedef struct dpp_ppu_ppu_spr24_t
{
    ZXIC_UINT32 spr24;
}DPP_PPU_PPU_SPR24_T;

typedef struct dpp_ppu_ppu_spr25_t
{
    ZXIC_UINT32 spr25;
}DPP_PPU_PPU_SPR25_T;

typedef struct dpp_ppu_ppu_spr26_t
{
    ZXIC_UINT32 spr26;
}DPP_PPU_PPU_SPR26_T;

typedef struct dpp_ppu_ppu_spr27_t
{
    ZXIC_UINT32 spr27;
}DPP_PPU_PPU_SPR27_T;

typedef struct dpp_ppu_ppu_spr28_t
{
    ZXIC_UINT32 spr28;
}DPP_PPU_PPU_SPR28_T;

typedef struct dpp_ppu_ppu_spr29_t
{
    ZXIC_UINT32 spr29;
}DPP_PPU_PPU_SPR29_T;

typedef struct dpp_ppu_ppu_spr30_t
{
    ZXIC_UINT32 spr30;
}DPP_PPU_PPU_SPR30_T;

typedef struct dpp_ppu_ppu_spr31_t
{
    ZXIC_UINT32 spr31;
}DPP_PPU_PPU_SPR31_T;

typedef struct dpp_ppu_ppu_rsp0_t
{
    ZXIC_UINT32 rsp0;
}DPP_PPU_PPU_RSP0_T;

typedef struct dpp_ppu_ppu_rsp1_t
{
    ZXIC_UINT32 rsp1;
}DPP_PPU_PPU_RSP1_T;

typedef struct dpp_ppu_ppu_rsp2_t
{
    ZXIC_UINT32 rsp2;
}DPP_PPU_PPU_RSP2_T;

typedef struct dpp_ppu_ppu_rsp3_t
{
    ZXIC_UINT32 rsp3;
}DPP_PPU_PPU_RSP3_T;

typedef struct dpp_ppu_ppu_rsp4_t
{
    ZXIC_UINT32 rsp4;
}DPP_PPU_PPU_RSP4_T;

typedef struct dpp_ppu_ppu_rsp5_t
{
    ZXIC_UINT32 rsp5;
}DPP_PPU_PPU_RSP5_T;

typedef struct dpp_ppu_ppu_rsp6_t
{
    ZXIC_UINT32 rsp6;
}DPP_PPU_PPU_RSP6_T;

typedef struct dpp_ppu_ppu_rsp7_t
{
    ZXIC_UINT32 rsp7;
}DPP_PPU_PPU_RSP7_T;

typedef struct dpp_ppu_ppu_rsp8_t
{
    ZXIC_UINT32 rsp8;
}DPP_PPU_PPU_RSP8_T;

typedef struct dpp_ppu_ppu_rsp9_t
{
    ZXIC_UINT32 rsp9;
}DPP_PPU_PPU_RSP9_T;

typedef struct dpp_ppu_ppu_rsp10_t
{
    ZXIC_UINT32 rsp10;
}DPP_PPU_PPU_RSP10_T;

typedef struct dpp_ppu_ppu_rsp11_t
{
    ZXIC_UINT32 rsp11;
}DPP_PPU_PPU_RSP11_T;

typedef struct dpp_ppu_ppu_rsp12_t
{
    ZXIC_UINT32 rsp12;
}DPP_PPU_PPU_RSP12_T;

typedef struct dpp_ppu_ppu_rsp13_t
{
    ZXIC_UINT32 rsp13;
}DPP_PPU_PPU_RSP13_T;

typedef struct dpp_ppu_ppu_rsp14_t
{
    ZXIC_UINT32 rsp14;
}DPP_PPU_PPU_RSP14_T;

typedef struct dpp_ppu_ppu_rsp15_t
{
    ZXIC_UINT32 rsp15;
}DPP_PPU_PPU_RSP15_T;

typedef struct dpp_ppu_ppu_rsp16_t
{
    ZXIC_UINT32 rsp16;
}DPP_PPU_PPU_RSP16_T;

typedef struct dpp_ppu_ppu_rsp17_t
{
    ZXIC_UINT32 rsp17;
}DPP_PPU_PPU_RSP17_T;

typedef struct dpp_ppu_ppu_rsp18_t
{
    ZXIC_UINT32 rsp18;
}DPP_PPU_PPU_RSP18_T;

typedef struct dpp_ppu_ppu_rsp19_t
{
    ZXIC_UINT32 rsp19;
}DPP_PPU_PPU_RSP19_T;

typedef struct dpp_ppu_ppu_rsp20_t
{
    ZXIC_UINT32 rsp20;
}DPP_PPU_PPU_RSP20_T;

typedef struct dpp_ppu_ppu_rsp21_t
{
    ZXIC_UINT32 rsp21;
}DPP_PPU_PPU_RSP21_T;

typedef struct dpp_ppu_ppu_rsp22_t
{
    ZXIC_UINT32 rsp22;
}DPP_PPU_PPU_RSP22_T;

typedef struct dpp_ppu_ppu_rsp23_t
{
    ZXIC_UINT32 rsp23;
}DPP_PPU_PPU_RSP23_T;

typedef struct dpp_ppu_ppu_rsp24_t
{
    ZXIC_UINT32 rsp24;
}DPP_PPU_PPU_RSP24_T;

typedef struct dpp_ppu_ppu_rsp25_t
{
    ZXIC_UINT32 rsp25;
}DPP_PPU_PPU_RSP25_T;

typedef struct dpp_ppu_ppu_rsp26_t
{
    ZXIC_UINT32 rsp26;
}DPP_PPU_PPU_RSP26_T;

typedef struct dpp_ppu_ppu_rsp27_t
{
    ZXIC_UINT32 rsp27;
}DPP_PPU_PPU_RSP27_T;

typedef struct dpp_ppu_ppu_rsp28_t
{
    ZXIC_UINT32 rsp28;
}DPP_PPU_PPU_RSP28_T;

typedef struct dpp_ppu_ppu_rsp29_t
{
    ZXIC_UINT32 rsp29;
}DPP_PPU_PPU_RSP29_T;

typedef struct dpp_ppu_ppu_rsp30_t
{
    ZXIC_UINT32 rsp30;
}DPP_PPU_PPU_RSP30_T;

typedef struct dpp_ppu_ppu_rsp31_t
{
    ZXIC_UINT32 rsp31;
}DPP_PPU_PPU_RSP31_T;

typedef struct dpp_ppu_ppu_key0_t
{
    ZXIC_UINT32 key0;
}DPP_PPU_PPU_KEY0_T;

typedef struct dpp_ppu_ppu_key1_t
{
    ZXIC_UINT32 key1;
}DPP_PPU_PPU_KEY1_T;

typedef struct dpp_ppu_ppu_key2_t
{
    ZXIC_UINT32 key2;
}DPP_PPU_PPU_KEY2_T;

typedef struct dpp_ppu_ppu_key3_t
{
    ZXIC_UINT32 key3;
}DPP_PPU_PPU_KEY3_T;

typedef struct dpp_ppu_ppu_key4_t
{
    ZXIC_UINT32 key4;
}DPP_PPU_PPU_KEY4_T;

typedef struct dpp_ppu_ppu_key5_t
{
    ZXIC_UINT32 key5;
}DPP_PPU_PPU_KEY5_T;

typedef struct dpp_ppu_ppu_key6_t
{
    ZXIC_UINT32 key6;
}DPP_PPU_PPU_KEY6_T;

typedef struct dpp_ppu_ppu_key7_t
{
    ZXIC_UINT32 key7;
}DPP_PPU_PPU_KEY7_T;

typedef struct dpp_ppu_ppu_key8_t
{
    ZXIC_UINT32 key8;
}DPP_PPU_PPU_KEY8_T;

typedef struct dpp_ppu_ppu_key9_t
{
    ZXIC_UINT32 key9;
}DPP_PPU_PPU_KEY9_T;

typedef struct dpp_ppu_ppu_key10_t
{
    ZXIC_UINT32 key10;
}DPP_PPU_PPU_KEY10_T;

typedef struct dpp_ppu_ppu_key11_t
{
    ZXIC_UINT32 key11;
}DPP_PPU_PPU_KEY11_T;

typedef struct dpp_ppu_ppu_key12_t
{
    ZXIC_UINT32 key12;
}DPP_PPU_PPU_KEY12_T;

typedef struct dpp_ppu_ppu_key13_t
{
    ZXIC_UINT32 key13;
}DPP_PPU_PPU_KEY13_T;

typedef struct dpp_ppu_ppu_key14_t
{
    ZXIC_UINT32 key14;
}DPP_PPU_PPU_KEY14_T;

typedef struct dpp_ppu_ppu_key15_t
{
    ZXIC_UINT32 key15;
}DPP_PPU_PPU_KEY15_T;

typedef struct dpp_ppu_ppu_key16_t
{
    ZXIC_UINT32 key16;
}DPP_PPU_PPU_KEY16_T;

typedef struct dpp_ppu_ppu_key17_t
{
    ZXIC_UINT32 key17;
}DPP_PPU_PPU_KEY17_T;

typedef struct dpp_ppu_ppu_key18_t
{
    ZXIC_UINT32 key18;
}DPP_PPU_PPU_KEY18_T;

typedef struct dpp_ppu_ppu_key19_t
{
    ZXIC_UINT32 key19;
}DPP_PPU_PPU_KEY19_T;

typedef struct dpp_ppu_ppu_flag_t
{
    ZXIC_UINT32 me_num;
    ZXIC_UINT32 thread_num;
    ZXIC_UINT32 flag;
}DPP_PPU_PPU_FLAG_T;

typedef struct dpp_ppu_cluster_int_1200m_flag_t
{
    ZXIC_UINT32 me7_interrupt_flag;
    ZXIC_UINT32 me6_interrupt_flag;
    ZXIC_UINT32 me5_interrupt_flag;
    ZXIC_UINT32 me4_interrupt_flag;
    ZXIC_UINT32 me3_interrupt_flag;
    ZXIC_UINT32 me2_interrupt_flag;
    ZXIC_UINT32 me1_interrupt_flag;
    ZXIC_UINT32 me0_interrupt_flag;
}DPP_PPU_CLUSTER_INT_1200M_FLAG_T;

typedef struct dpp_ppu_cluster_bp_instr_l_t
{
    ZXIC_UINT32 bp_instr_l;
}DPP_PPU_CLUSTER_BP_INSTR_L_T;

typedef struct dpp_ppu_cluster_bp_instr_h_t
{
    ZXIC_UINT32 bp_instr_h;
}DPP_PPU_CLUSTER_BP_INSTR_H_T;

typedef struct dpp_ppu_cluster_bp_addr_t
{
    ZXIC_UINT32 bp_addr;
}DPP_PPU_CLUSTER_BP_ADDR_T;

typedef struct dpp_ppu_cluster_drr_t
{
    ZXIC_UINT32 drr;
}DPP_PPU_CLUSTER_DRR_T;

typedef struct dpp_ppu_cluster_dsr_t
{
    ZXIC_UINT32 dsr;
}DPP_PPU_CLUSTER_DSR_T;

typedef struct dpp_ppu_cluster_dbg_rtl_date_t
{
    ZXIC_UINT32 dbg_rtl_date;
}DPP_PPU_CLUSTER_DBG_RTL_DATE_T;

typedef struct dpp_ppu_cluster_me_continue_t
{
    ZXIC_UINT32 me_continue;
}DPP_PPU_CLUSTER_ME_CONTINUE_T;

typedef struct dpp_ppu_cluster_me_step_t
{
    ZXIC_UINT32 me_step;
}DPP_PPU_CLUSTER_ME_STEP_T;

typedef struct dpp_ppu_cluster_me_refresh_t
{
    ZXIC_UINT32 me_refresh;
}DPP_PPU_CLUSTER_ME_REFRESH_T;

typedef struct dpp_ppu_cluster_drr_clr_t
{
    ZXIC_UINT32 drr_clr;
}DPP_PPU_CLUSTER_DRR_CLR_T;

typedef struct dpp_ppu_cluster_me_busy_thresold_t
{
    ZXIC_UINT32 me_busy_thresold;
}DPP_PPU_CLUSTER_ME_BUSY_THRESOLD_T;

typedef struct dpp_ppu_cluster_int_1200m_sta_t
{
    ZXIC_UINT32 me7_interrupt_sta;
    ZXIC_UINT32 me6_interrupt_sta;
    ZXIC_UINT32 me5_interrupt_sta;
    ZXIC_UINT32 me4_interrupt_sta;
    ZXIC_UINT32 me3_interrupt_sta;
    ZXIC_UINT32 me2_interrupt_sta;
    ZXIC_UINT32 me1_interrupt_sta;
    ZXIC_UINT32 me0_interrupt_sta;
}DPP_PPU_CLUSTER_INT_1200M_STA_T;

typedef struct dpp_ppu_cluster_int_1200m_me_fifo_mask_l_t
{
    ZXIC_UINT32 me_free_pkt_q_overflow_mask;
    ZXIC_UINT32 me_free_pkt_q_underflow_mask;
    ZXIC_UINT32 me_free_thread_q_overflow_mask;
    ZXIC_UINT32 me_free_thread_q_underflow_mask;
    ZXIC_UINT32 me_pkt_in_overflow_mask;
    ZXIC_UINT32 me_pkt_in_underflow_mask;
    ZXIC_UINT32 me_rdy_q_overflow_mask;
    ZXIC_UINT32 me_rdy_q_underflow_mask;
    ZXIC_UINT32 me_pkt_out_q_overflow_mask;
    ZXIC_UINT32 me_pkt_out_q_underflow_mask;
    ZXIC_UINT32 me_continue_q_overflow_mask;
    ZXIC_UINT32 me_continue_q_underflow_mask;
    ZXIC_UINT32 me_esrh_q_overflow_mask;
    ZXIC_UINT32 me_esrh_q_underflow_mask;
    ZXIC_UINT32 me_isrh_q_overflow_mask;
    ZXIC_UINT32 me_isrh_q_underflow_mask;
    ZXIC_UINT32 me_cache_miss_q_overflow_mask;
    ZXIC_UINT32 me_cache_miss_q_underflow_mask;
    ZXIC_UINT32 me_base_q_u0_overflow_mask;
    ZXIC_UINT32 me_base_q_u0_underflow_mask;
    ZXIC_UINT32 me_base_q_u1_overflow_mask;
    ZXIC_UINT32 me_base_q_u1_underflow_mask;
    ZXIC_UINT32 me_base_q_u2_overflow_mask;
    ZXIC_UINT32 me_base_q_u2_underflow_mask;
    ZXIC_UINT32 me_base_q_u3_overflow_mask;
    ZXIC_UINT32 me_base_q_u3_underflow_mask;
    ZXIC_UINT32 me_reg_pc_q_overflow_mask;
    ZXIC_UINT32 me_reg_pc_q_underflow_mask;
    ZXIC_UINT32 me_branch_q_overflow_mask;
    ZXIC_UINT32 me_branch_q_underflow_mask;
    ZXIC_UINT32 me_pkt_base_q_overflow_mask;
    ZXIC_UINT32 me_pkt_base_q_underflow_mask;
}DPP_PPU_CLUSTER_INT_1200M_ME_FIFO_MASK_L_T;

typedef struct dpp_ppu_cluster_int_1200m_me_fifo_mask_h_t
{
    ZXIC_UINT32 me_except_refetch_pc_overflow_mask;
    ZXIC_UINT32 me_except_refetch_pc_underflow_mask;
}DPP_PPU_CLUSTER_INT_1200M_ME_FIFO_MASK_H_T;

typedef struct dpp_ppu_cluster_me_fifo_interrupt_flag_l_t
{
    ZXIC_UINT32 me_free_pkt_q_overflow_flag;
    ZXIC_UINT32 me_free_pkt_q_underflow_flag;
    ZXIC_UINT32 me_free_thread_q_overflow_flag;
    ZXIC_UINT32 me_free_thread_q_underflow_flag;
    ZXIC_UINT32 me_pkt_in_overflow_flag;
    ZXIC_UINT32 me_pkt_in_underflow_flag;
    ZXIC_UINT32 me_rdy_q_overflow_flag;
    ZXIC_UINT32 me_rdy_q_underflow_flag;
    ZXIC_UINT32 me_pkt_out_q_overflow_flag;
    ZXIC_UINT32 me_pkt_out_q_underflow_flag;
    ZXIC_UINT32 me_continue_q_overflow_flag;
    ZXIC_UINT32 me_continue_q_underflow_flag;
    ZXIC_UINT32 me_esrh_q_overflow_flag;
    ZXIC_UINT32 me_esrh_q_underflow_flag;
    ZXIC_UINT32 me_isrh_q_overflow_flag;
    ZXIC_UINT32 me_isrh_q_underflow_flag;
    ZXIC_UINT32 me_cache_miss_q_overflow_flag;
    ZXIC_UINT32 me_cache_miss_q_underflow_flag;
    ZXIC_UINT32 me_base_q_u0_overflow_flag;
    ZXIC_UINT32 me_base_q_u0_underflow_flag;
    ZXIC_UINT32 me_base_q_u1_overflow_flag;
    ZXIC_UINT32 me_base_q_u1_underflow_flag;
    ZXIC_UINT32 me_base_q_u2_overflow_flag;
    ZXIC_UINT32 me_base_q_u2_underflow_flag;
    ZXIC_UINT32 me_base_q_u3_overflow_flag;
    ZXIC_UINT32 me_base_q_u3_underflow_flag;
    ZXIC_UINT32 me_reg_pc_q_overflow_flag;
    ZXIC_UINT32 me_reg_pc_q_underflow_flag;
    ZXIC_UINT32 me_branch_q_overflow_flag;
    ZXIC_UINT32 me_branch_q_underflow_flag;
    ZXIC_UINT32 me_pkt_base_q_overflow_flag;
    ZXIC_UINT32 me_pkt_base_q_underflow_flag;
}DPP_PPU_CLUSTER_ME_FIFO_INTERRUPT_FLAG_L_T;

typedef struct dpp_ppu_cluster_me_fifo_interrupt_flag_h_t
{
    ZXIC_UINT32 me_except_refetch_pc_overflow_flag;
    ZXIC_UINT32 me_except_refetch_pc_underflow_flag;
}DPP_PPU_CLUSTER_ME_FIFO_INTERRUPT_FLAG_H_T;

typedef struct dpp_ppu_cluster_me_fifo_interrupt_sta_l_t
{
    ZXIC_UINT32 me_free_pkt_q_overflow_sta;
    ZXIC_UINT32 me_free_pkt_q_underflow_sta;
    ZXIC_UINT32 me_free_thread_q_overflow_sta;
    ZXIC_UINT32 me_free_thread_q_underflow_sta;
    ZXIC_UINT32 me_pkt_in_overflow_sta;
    ZXIC_UINT32 me_pkt_in_underflow_sta;
    ZXIC_UINT32 me_rdy_q_overflow_sta;
    ZXIC_UINT32 me_rdy_q_underflow_sta;
    ZXIC_UINT32 me_pkt_out_q_overflow_sta;
    ZXIC_UINT32 me_pkt_out_q_underflow_sta;
    ZXIC_UINT32 me_continue_q_overflow_sta;
    ZXIC_UINT32 me_continue_q_underflow_sta;
    ZXIC_UINT32 me_esrh_q_overflow_sta;
    ZXIC_UINT32 me_esrh_q_underflow_sta;
    ZXIC_UINT32 me_isrh_q_overflow_sta;
    ZXIC_UINT32 me_isrh_q_underflow_sta;
    ZXIC_UINT32 me_cache_miss_q_overflow_sta;
    ZXIC_UINT32 me_cache_miss_q_underflow_sta;
    ZXIC_UINT32 me_base_q_u0_overflow_sta;
    ZXIC_UINT32 me_base_q_u0_underflow_sta;
    ZXIC_UINT32 me_base_q_u1_overflow_sta;
    ZXIC_UINT32 me_base_q_u1_underflow_sta;
    ZXIC_UINT32 me_base_q_u2_overflow_sta;
    ZXIC_UINT32 me_base_q_u2_underflow_sta;
    ZXIC_UINT32 me_base_q_u3_overflow_sta;
    ZXIC_UINT32 me_base_q_u3_underflow_sta;
    ZXIC_UINT32 me_reg_pc_q_overflow_sta;
    ZXIC_UINT32 me_reg_pc_q_underflow_sta;
    ZXIC_UINT32 me_branch_q_overflow_sta;
    ZXIC_UINT32 me_branch_q_underflow_sta;
    ZXIC_UINT32 me_pkt_base_q_overflow_sta;
    ZXIC_UINT32 me_pkt_base_q_underflow_sta;
}DPP_PPU_CLUSTER_ME_FIFO_INTERRUPT_STA_L_T;

typedef struct dpp_ppu_cluster_me_fifo_interrupt_sta_h_t
{
    ZXIC_UINT32 me_except_refetch_pc_overflow_sta;
    ZXIC_UINT32 me_except_refetch_pc_underflow_sta;
}DPP_PPU_CLUSTER_ME_FIFO_INTERRUPT_STA_H_T;

typedef struct dpp_ppu_cluster_int_1200m_cluster_mex_fifo_mask_l_t
{
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u3_overflow_mask;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u3_underflow_mask;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u4_overflow_mask;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u4_underflow_mask;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u5_overflow_mask;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u5_underflow_mask;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u6_overflow_mask;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u6_underflow_mask;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u7_overflow_mask;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u7_underflow_mask;
    ZXIC_UINT32 ppu_ise_rsp_afifo_64x143_wrapper_u0_underflow_mask;
    ZXIC_UINT32 ise_rsp_ram_free_ptr_u0_overflow_mask;
    ZXIC_UINT32 ise_rsp_ram_free_ptr_u0_underflow_mask;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u0_overflow_mask;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u0_underflow_mask;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u1_overflow_mask;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u1_underflow_mask;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u2_overflow_mask;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u2_underflow_mask;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u3_overflow_mask;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u3_underflow_mask;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u4_overflow_mask;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u4_underflow_mask;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u5_overflow_mask;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u5_underflow_mask;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u6_overflow_mask;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u6_underflow_mask;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u7_overflow_mask;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u7_underflow_mask;
    ZXIC_UINT32 ppu_sta_rsp_afifo_64x79_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_sta_rsp_fwft_fifo_128x79_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_sta_rsp_fwft_fifo_128x79_wrapper_underflow_mask;
}DPP_PPU_CLUSTER_INT_1200M_CLUSTER_MEX_FIFO_MASK_L_T;

typedef struct dpp_ppu_cluster_int_1200m_cluster_mex_fifo_mask_h_t
{
    ZXIC_UINT32 ppu_se_key_afifo_32x54_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_se_key_afifo_32x665_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_sta_key_afifo_32x110_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_cluster_mf_in_afifo_32x2048_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_pbu_mcode_pf_rsp_fifo_32x13_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_pbu_mcode_pf_rsp_fifo_32x13_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_coprocess_rsp_fifo_32x77_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_coprocess_rsp_fifo_32x77_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_coprocess_rsp_fwft_fifo_128x78_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_coprocess_rsp_fwft_fifo_128x78_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_ese_rsp_afifo_64x271_wrapper_u0_underflow_mask;
    ZXIC_UINT32 ese_rsp_ram_free_ptr_u0_overflow_mask;
    ZXIC_UINT32 ese_rsp_ram_free_ptr_u0_underflow_mask;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u0_overflow_mask;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u0_underflow_mask;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u1_overflow_mask;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u1_underflow_mask;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u2_overflow_mask;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u2_underflow_mask;
}DPP_PPU_CLUSTER_INT_1200M_CLUSTER_MEX_FIFO_MASK_H_T;

typedef struct dpp_ppu_cluster_int_1200m_cluster_mex_fifo_flag_l_t
{
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u3_overflow_flag;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u3_underflow_flag;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u4_overflow_flag;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u4_underflow_flag;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u5_overflow_flag;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u5_underflow_flag;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u6_overflow_flag;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u6_underflow_flag;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u7_overflow_flag;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u7_underflow_flag;
    ZXIC_UINT32 ppu_ise_rsp_afifo_64x143_wrapper_u0_underflow_flag;
    ZXIC_UINT32 ise_rsp_ram_free_ptr_u0_overflow_flag;
    ZXIC_UINT32 ise_rsp_ram_free_ptr_u0_underflow_flag;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u0_overflow_flag;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u0_underflow_flag;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u1_overflow_flag;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u1_underflow_flag;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u2_overflow_flag;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u2_underflow_flag;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u3_overflow_flag;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u3_underflow_flag;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u4_overflow_flag;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u4_underflow_flag;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u5_overflow_flag;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u5_underflow_flag;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u6_overflow_flag;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u6_underflow_flag;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u7_overflow_flag;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u7_underflow_flag;
    ZXIC_UINT32 ppu_sta_rsp_afifo_64x79_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_sta_rsp_fwft_fifo_128x79_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_sta_rsp_fwft_fifo_128x79_wrapper_underflow_flag;
}DPP_PPU_CLUSTER_INT_1200M_CLUSTER_MEX_FIFO_FLAG_L_T;

typedef struct dpp_ppu_cluster_int_1200m_cluster_mex_fifo_flag_h_t
{
    ZXIC_UINT32 ppu_se_key_afifo_32x54_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_se_key_afifo_32x665_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_sta_key_afifo_32x110_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_cluster_mf_in_afifo_32x2048_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_pbu_mcode_pf_rsp_fifo_32x13_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_pbu_mcode_pf_rsp_fifo_32x13_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_coprocess_rsp_fifo_32x77_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_coprocess_rsp_fifo_32x77_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_coprocess_rsp_fwft_fifo_128x78_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_coprocess_rsp_fwft_fifo_128x78_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_ese_rsp_afifo_64x271_wrapper_u0_underflow_flag;
    ZXIC_UINT32 ese_rsp_ram_free_ptr_u0_overflow_flag;
    ZXIC_UINT32 ese_rsp_ram_free_ptr_u0_underflow_flag;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u0_overflow_flag;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u0_underflow_flag;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u1_overflow_flag;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u1_underflow_flag;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u2_overflow_flag;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u2_underflow_flag;
}DPP_PPU_CLUSTER_INT_1200M_CLUSTER_MEX_FIFO_FLAG_H_T;

typedef struct dpp_ppu_cluster_int_1200m_cluster_mex_fifo_stat_l_t
{
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u3_overflow_stat;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u3_underflow_stat;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u4_overflow_stat;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u4_underflow_stat;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u5_overflow_stat;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u5_underflow_stat;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u6_overflow_stat;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u6_underflow_stat;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u7_overflow_stat;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u7_underflow_stat;
    ZXIC_UINT32 ppu_ise_rsp_afifo_64x143_wrapper_u0_underflow_stat;
    ZXIC_UINT32 ise_rsp_ram_free_ptr_u0_overflow_stat;
    ZXIC_UINT32 ise_rsp_ram_free_ptr_u0_underflow_stat;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u0_overflow_stat;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u0_underflow_stat;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u1_overflow_stat;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u1_underflow_stat;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u2_overflow_stat;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u2_underflow_stat;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u3_overflow_stat;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u3_underflow_stat;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u4_overflow_stat;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u4_underflow_stat;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u5_overflow_stat;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u5_underflow_stat;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u6_overflow_stat;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u6_underflow_stat;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u7_overflow_stat;
    ZXIC_UINT32 ppu_ise_rsp_ptr_fwft_fifo_128x7_wrapper_u7_underflow_stat;
    ZXIC_UINT32 ppu_sta_rsp_afifo_64x79_wrapper_underflow_stat;
    ZXIC_UINT32 ppu_sta_rsp_fwft_fifo_128x79_wrapper_overflow_stat;
    ZXIC_UINT32 ppu_sta_rsp_fwft_fifo_128x79_wrapper_underflow_stat;
}DPP_PPU_CLUSTER_INT_1200M_CLUSTER_MEX_FIFO_STAT_L_T;

typedef struct dpp_ppu_cluster_int_1200m_cluster_mex_fifo_stat_h_t
{
    ZXIC_UINT32 ppu_se_key_afifo_32x54_wrapper_overflow_stat;
    ZXIC_UINT32 ppu_se_key_afifo_32x665_wrapper_overflow_stat;
    ZXIC_UINT32 ppu_sta_key_afifo_32x110_wrapper_overflow_stat;
    ZXIC_UINT32 ppu_cluster_mf_in_afifo_32x2048_wrapper_underflow_stat;
    ZXIC_UINT32 ppu_pbu_mcode_pf_rsp_fifo_32x13_wrapper_overflow_stat;
    ZXIC_UINT32 ppu_pbu_mcode_pf_rsp_fifo_32x13_wrapper_underflow_stat;
    ZXIC_UINT32 ppu_coprocess_rsp_fifo_32x77_wrapper_overflow_stat;
    ZXIC_UINT32 ppu_coprocess_rsp_fifo_32x77_wrapper_underflow_stat;
    ZXIC_UINT32 ppu_coprocess_rsp_fwft_fifo_128x78_wrapper_overflow_stat;
    ZXIC_UINT32 ppu_coprocess_rsp_fwft_fifo_128x78_wrapper_underflow_stat;
    ZXIC_UINT32 ppu_ese_rsp_afifo_64x271_wrapper_u0_underflow_stat;
    ZXIC_UINT32 ese_rsp_ram_free_ptr_u0_overflow_stat;
    ZXIC_UINT32 ese_rsp_ram_free_ptr_u0_underflow_stat;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u0_overflow_stat;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u0_underflow_stat;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u1_overflow_stat;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u1_underflow_stat;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u2_overflow_stat;
    ZXIC_UINT32 ppu_ese_rsp_ptr_fwft_fifo_128x7_wrapper_u2_underflow_stat;
}DPP_PPU_CLUSTER_INT_1200M_CLUSTER_MEX_FIFO_STAT_H_T;

typedef struct dpp_ppu_cluster_ppu_statics_wb_exception_cfg_t
{
    ZXIC_UINT32 csr_statics_wb_exception_code5;
    ZXIC_UINT32 csr_statics_wb_exception_code4;
    ZXIC_UINT32 csr_statics_wb_exception_code3;
    ZXIC_UINT32 csr_statics_wb_exception_code2;
    ZXIC_UINT32 csr_statics_wb_exception_code1;
    ZXIC_UINT32 csr_statics_wb_exception_code0;
}DPP_PPU_CLUSTER_PPU_STATICS_WB_EXCEPTION_CFG_T;

typedef struct dpp_ppu_cluster_thread_switch_en_t
{
    ZXIC_UINT32 thread_switch_en;
}DPP_PPU_CLUSTER_THREAD_SWITCH_EN_T;

typedef struct dpp_ppu_cluster_is_me_not_idle_t
{
    ZXIC_UINT32 me7_is_not_idle;
    ZXIC_UINT32 me6_is_not_idle;
    ZXIC_UINT32 me5_is_not_idle;
    ZXIC_UINT32 me4_is_not_idle;
    ZXIC_UINT32 me3_is_not_idle;
    ZXIC_UINT32 me2_is_not_idle;
    ZXIC_UINT32 me1_is_not_idle;
    ZXIC_UINT32 me0_is_not_idle;
}DPP_PPU_CLUSTER_IS_ME_NOT_IDLE_T;

typedef struct dpp_ppu_cluster_ppu_cluster_mf_in_afifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_cluster_mf_in_afifo_prog_empty_assert_cfg;
}DPP_PPU_CLUSTER_PPU_CLUSTER_MF_IN_AFIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_ppu_cluster_mf_in_afifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_cluster_mf_in_afifo_prog_empty_negate_cfg;
}DPP_PPU_CLUSTER_PPU_CLUSTER_MF_IN_AFIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_ese_rsp_afifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ese_rsp_afifo_prog_empty_assert_cfg;
}DPP_PPU_CLUSTER_ESE_RSP_AFIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_ese_rsp_afifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ese_rsp_afifo_prog_empty_negate_cfg;
}DPP_PPU_CLUSTER_ESE_RSP_AFIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_ise_rsp_afifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ise_rsp_afifo_prog_empty_assert_cfg;
}DPP_PPU_CLUSTER_ISE_RSP_AFIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_ise_rsp_afifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ise_rsp_afifo_prog_empty_negate_cfg;
}DPP_PPU_CLUSTER_ISE_RSP_AFIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_ppu_rsp_ptr_fwft_fifo0_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_rsp_ptr_fwft_fifo0_prog_full_assert_cfg;
}DPP_PPU_CLUSTER_PPU_RSP_PTR_FWFT_FIFO0_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_ppu_rsp_ptr_fwft_fifo0_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_rsp_ptr_fwft_fifo0_prog_full_negate_cfg;
}DPP_PPU_CLUSTER_PPU_RSP_PTR_FWFT_FIFO0_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_ppu_rsp_ptr_fwft_fifo0_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_rsp_ptr_fwft_fifo0_prog_empty_assert_cfg;
}DPP_PPU_CLUSTER_PPU_RSP_PTR_FWFT_FIFO0_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_ppu_rsp_ptr_fwft_fifo0_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_rsp_ptr_fwft_fifo0_prog_empty_negate_cfg;
}DPP_PPU_CLUSTER_PPU_RSP_PTR_FWFT_FIFO0_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_ppu_rsp_ptr_fwft_fifo1_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_rsp_ptr_fwft_fifo1_prog_full_assert_cfg;
}DPP_PPU_CLUSTER_PPU_RSP_PTR_FWFT_FIFO1_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_ppu_rsp_ptr_fwft_fifo1_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_rsp_ptr_fwft_fifo1_prog_full_negate_cfg;
}DPP_PPU_CLUSTER_PPU_RSP_PTR_FWFT_FIFO1_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_ppu_rsp_ptr_fwft_fifo1_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_rsp_ptr_fwft_fifo1_prog_empty_assert_cfg;
}DPP_PPU_CLUSTER_PPU_RSP_PTR_FWFT_FIFO1_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_ppu_rsp_ptr_fwft_fifo1_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_rsp_ptr_fwft_fifo1_prog_empty_negate_cfg;
}DPP_PPU_CLUSTER_PPU_RSP_PTR_FWFT_FIFO1_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_sta_rsp_afifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 sta_rsp_afifo_prog_empty_assert_cfg;
}DPP_PPU_CLUSTER_STA_RSP_AFIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_sta_rsp_afifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 sta_rsp_afifo_prog_empty_negate_cfg;
}DPP_PPU_CLUSTER_STA_RSP_AFIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_ppu_sta_rsp_fwft_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_sta_rsp_fwft_fifo_prog_full_assert_cfg;
}DPP_PPU_CLUSTER_PPU_STA_RSP_FWFT_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_ppu_sta_rsp_fwft_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_sta_rsp_fwft_fifo_prog_full_negate_cfg;
}DPP_PPU_CLUSTER_PPU_STA_RSP_FWFT_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_ppu_sta_rsp_fwft_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_sta_rsp_fwft_fifo_prog_empty_assert_cfg;
}DPP_PPU_CLUSTER_PPU_STA_RSP_FWFT_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_ppu_sta_rsp_fwft_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_sta_rsp_fwft_fifo_prog_empty_negate_cfg;
}DPP_PPU_CLUSTER_PPU_STA_RSP_FWFT_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_cop_rsp_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 cop_rsp_fifo_prog_full_assert_cfg;
}DPP_PPU_CLUSTER_COP_RSP_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_cop_rsp_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 cop_rsp_fifo_prog_full_negate_cfg;
}DPP_PPU_CLUSTER_COP_RSP_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_cop_rsp_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 cop_rsp_fifo_prog_empty_assert_cfg;
}DPP_PPU_CLUSTER_COP_RSP_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_cop_rsp_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 cop_rsp_fifo_prog_empty_negate_cfg;
}DPP_PPU_CLUSTER_COP_RSP_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_mcode_pf_rsp_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 mcode_pf_rsp_fifo_prog_full_assert_cfg;
}DPP_PPU_CLUSTER_MCODE_PF_RSP_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_mcode_pf_rsp_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 mcode_pf_rsp_fifo_prog_full_negate_cfg;
}DPP_PPU_CLUSTER_MCODE_PF_RSP_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_mcode_pf_rsp_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 mcode_pf_rsp_fifo_prog_empty_assert_cfg;
}DPP_PPU_CLUSTER_MCODE_PF_RSP_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_mcode_pf_rsp_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 mcode_pf_rsp_fifo_prog_empty_negate_cfg;
}DPP_PPU_CLUSTER_MCODE_PF_RSP_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_ppu_cop_rsp_fwft_fifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_rsp_fwft_fifo_prog_full_assert_cfg;
}DPP_PPU_CLUSTER_PPU_COP_RSP_FWFT_FIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_ppu_cop_rsp_fwft_fifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_rsp_fwft_fifo_prog_full_negate_cfg;
}DPP_PPU_CLUSTER_PPU_COP_RSP_FWFT_FIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_ppu_cop_rsp_fwft_fifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_cop_rsp_fwft_fifo_prog_empty_assert_cfg;
}DPP_PPU_CLUSTER_PPU_COP_RSP_FWFT_FIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_ppu_cop_rsp_fwft_fifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_cop_rsp_fwft_fifo_prog_empty_negate_cfg;
}DPP_PPU_CLUSTER_PPU_COP_RSP_FWFT_FIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_ppu_ise_key_afifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_ise_key_afifo_prog_full_assert_cfg;
}DPP_PPU_CLUSTER_PPU_ISE_KEY_AFIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_ppu_ise_key_afifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_ise_key_afifo_prog_full_negate_cfg;
}DPP_PPU_CLUSTER_PPU_ISE_KEY_AFIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_ppu_ese_key_afifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_ese_key_afifo_prog_full_assert_cfg;
}DPP_PPU_CLUSTER_PPU_ESE_KEY_AFIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_ppu_ese_key_afifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_ese_key_afifo_prog_full_negate_cfg;
}DPP_PPU_CLUSTER_PPU_ESE_KEY_AFIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_ppu_sta_key_afifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_sta_key_afifo_prog_full_assert_cfg;
}DPP_PPU_CLUSTER_PPU_STA_KEY_AFIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_ppu_sta_key_afifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_sta_key_afifo_prog_full_negate_cfg;
}DPP_PPU_CLUSTER_PPU_STA_KEY_AFIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_int_600m_cluster_mex_fifo_mask_t
{
    ZXIC_UINT32 ppu_se_key_afifo_32x54_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_se_key_afifo_32x665_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_sta_key_afifo_32x110_wrapper_underflow_mask;
    ZXIC_UINT32 ppu_cluster_mf_in_afifo_32x2048_wrapper_overflow_mask;
    ZXIC_UINT32 ppu_ese_rsp_afifo_64x271_wrapper_u0_overflow_mask;
    ZXIC_UINT32 ppu_ise_rsp_afifo_64x143_wrapper_u0_overflow_mask;
    ZXIC_UINT32 ppu_sta_rsp_afifo_64x79_wrapper_overflow_mask;
}DPP_PPU_CLUSTER_INT_600M_CLUSTER_MEX_FIFO_MASK_T;

typedef struct dpp_ppu_cluster_cluster_mex_fifo_600m_interrupt_flag_t
{
    ZXIC_UINT32 ppu_se_key_afifo_32x54_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_se_key_afifo_32x665_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_sta_key_afifo_32x110_wrapper_underflow_flag;
    ZXIC_UINT32 ppu_cluster_mf_in_afifo_32x2048_wrapper_overflow_flag;
    ZXIC_UINT32 ppu_ese_rsp_afifo_64x271_wrapper_u0_overflow_flag;
    ZXIC_UINT32 ppu_ise_rsp_afifo_64x143_wrapper_u0_overflow_flag;
    ZXIC_UINT32 ppu_sta_rsp_afifo_64x79_wrapper_overflow_flag;
}DPP_PPU_CLUSTER_CLUSTER_MEX_FIFO_600M_INTERRUPT_FLAG_T;

typedef struct dpp_ppu_cluster_cluster_mex_fifo_600m_interrupt_sta_t
{
    ZXIC_UINT32 ppu_se_key_afifo_32x54_wrapper_underflow_sta;
    ZXIC_UINT32 ppu_se_key_afifo_32x665_wrapper_underflow_sta;
    ZXIC_UINT32 ppu_sta_key_afifo_32x110_wrapper_underflow_sta;
    ZXIC_UINT32 ppu_cluster_mf_in_afifo_32x2048_wrapper_overflow_sta;
    ZXIC_UINT32 ppu_ese_rsp_afifo_64x271_wrapper_u0_overflow_sta;
    ZXIC_UINT32 ppu_ise_rsp_afifo_64x143_wrapper_u0_overflow_sta;
    ZXIC_UINT32 ppu_sta_rsp_afifo_64x79_wrapper_overflow_sta;
}DPP_PPU_CLUSTER_CLUSTER_MEX_FIFO_600M_INTERRUPT_STA_T;

typedef struct dpp_ppu_cluster_mex_cnt_cfg_t
{
    ZXIC_UINT32 csr_count_overflow_mode;
    ZXIC_UINT32 csr_count_rd_mode;
}DPP_PPU_CLUSTER_MEX_CNT_CFG_T;

typedef struct dpp_ppu_cluster_int_600m_cluster_mex_ram_ecc_error_interrupt_mask_t
{
    ZXIC_UINT32 ppu_sta_key_ram_1r1w_32x110_ecc_double_err_mask;
    ZXIC_UINT32 ppu_se_key_afifo_32x665_ecc_double_err_mask;
    ZXIC_UINT32 ppu_se_key_afifo_32x54_ecc_double_err_mask;
    ZXIC_UINT32 ppu_sta_key_ram_1r1w_32x110_ecc_single_err_flag;
    ZXIC_UINT32 ppu_se_key_afifo_32x665_ecc_single_err_mask;
    ZXIC_UINT32 ppu_se_key_afifo_32x54_ecc_single_err_mask;
}DPP_PPU_CLUSTER_INT_600M_CLUSTER_MEX_RAM_ECC_ERROR_INTERRUPT_MASK_T;

typedef struct dpp_ppu_cluster_cluster_mex_ram_600m_ecc_error_interrupt_flag_t
{
    ZXIC_UINT32 ppu_sta_key_ram_1r1w_32x110_ecc_double_err_flag;
    ZXIC_UINT32 ppu_se_key_afifo_32x665_ecc_double_err_flag;
    ZXIC_UINT32 ppu_se_key_afifo_32x54_ecc_double_err_flag;
    ZXIC_UINT32 ppu_sta_key_ram_1r1w_32x110_ecc_single_err_flag;
    ZXIC_UINT32 ppu_se_key_afifo_32x665_ecc_single_err_flag;
    ZXIC_UINT32 ppu_se_key_afifo_32x54_ecc_single_err_flag;
}DPP_PPU_CLUSTER_CLUSTER_MEX_RAM_600M_ECC_ERROR_INTERRUPT_FLAG_T;

typedef struct dpp_ppu_cluster_cluster_mex_ram_600m_ecc_error_interrupt_sta_t
{
    ZXIC_UINT32 ppu_sta_key_ram_1r1w_32x110_ecc_double_err_stat;
    ZXIC_UINT32 ppu_se_key_afifo_32x665_ecc_double_err_stat;
    ZXIC_UINT32 ppu_se_key_afifo_32x54_ecc_double_err_stat;
    ZXIC_UINT32 ppu_sta_key_ram_1r1w_32x110_ecc_single_err_stat;
    ZXIC_UINT32 ppu_se_key_afifo_32x665_ecc_single_err_stat;
    ZXIC_UINT32 ppu_se_key_afifo_32x54_ecc_single_err_stat;
}DPP_PPU_CLUSTER_CLUSTER_MEX_RAM_600M_ECC_ERROR_INTERRUPT_STA_T;

typedef struct dpp_ppu_cluster_ppu_cluster_mf_in_afifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ppu_cluster_mf_in_afifo_prog_full_assert_cfg;
}DPP_PPU_CLUSTER_PPU_CLUSTER_MF_IN_AFIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_ppu_cluster_mf_in_afifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ppu_cluster_mf_in_afifo_prog_full_negate_cfg;
}DPP_PPU_CLUSTER_PPU_CLUSTER_MF_IN_AFIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_ese_rsp_afifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ese_rsp_afifo_prog_full_assert_cfg;
}DPP_PPU_CLUSTER_ESE_RSP_AFIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_ese_rsp_afifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ese_rsp_afifo_prog_full_negate_cfg;
}DPP_PPU_CLUSTER_ESE_RSP_AFIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_ise_rsp_afifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 ise_rsp_afifo_prog_full_assert_cfg;
}DPP_PPU_CLUSTER_ISE_RSP_AFIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_ise_rsp_afifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 ise_rsp_afifo_prog_full_negate_cfg;
}DPP_PPU_CLUSTER_ISE_RSP_AFIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_sta_rsp_afifo_prog_full_assert_cfg_t
{
    ZXIC_UINT32 sta_rsp_afifo_prog_full_assert_cfg;
}DPP_PPU_CLUSTER_STA_RSP_AFIFO_PROG_FULL_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_sta_rsp_afifo_prog_full_negate_cfg_t
{
    ZXIC_UINT32 sta_rsp_afifo_prog_full_negate_cfg;
}DPP_PPU_CLUSTER_STA_RSP_AFIFO_PROG_FULL_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_ppu_ise_key_afifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_ise_key_afifo_prog_empty_assert_cfg;
}DPP_PPU_CLUSTER_PPU_ISE_KEY_AFIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_ppu_ise_key_afifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_ise_key_afifo_prog_empty_negate_cfg;
}DPP_PPU_CLUSTER_PPU_ISE_KEY_AFIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_ppu_ese_key_afifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_ese_key_afifo_prog_empty_assert_cfg;
}DPP_PPU_CLUSTER_PPU_ESE_KEY_AFIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_ppu_ese_key_afifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_ese_key_afifo_prog_empty_negate_cfg;
}DPP_PPU_CLUSTER_PPU_ESE_KEY_AFIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_ppu_sta_key_afifo_prog_empty_assert_cfg_t
{
    ZXIC_UINT32 ppu_sta_key_afifo_prog_empty_assert_cfg;
}DPP_PPU_CLUSTER_PPU_STA_KEY_AFIFO_PROG_EMPTY_ASSERT_CFG_T;

typedef struct dpp_ppu_cluster_ppu_sta_key_afifo_prog_empty_negate_cfg_t
{
    ZXIC_UINT32 ppu_sta_key_afifo_prog_empty_negate_cfg;
}DPP_PPU_CLUSTER_PPU_STA_KEY_AFIFO_PROG_EMPTY_NEGATE_CFG_T;

typedef struct dpp_ppu_cluster_ppu_cluster_mf_vld_cnt_h_t
{
    ZXIC_UINT32 ppu_cluster_mf_vld_cnt_h;
}DPP_PPU_CLUSTER_PPU_CLUSTER_MF_VLD_CNT_H_T;

typedef struct dpp_ppu_cluster_ppu_cluster_mf_vld_cnt_l_t
{
    ZXIC_UINT32 ppu_cluster_mf_vld_cnt_l;
}DPP_PPU_CLUSTER_PPU_CLUSTER_MF_VLD_CNT_L_T;

typedef struct dpp_ppu_cluster_cluster_ise_key_out_vld_cnt_t
{
    ZXIC_UINT32 cluster_ise_key_out_vld_cnt;
}DPP_PPU_CLUSTER_CLUSTER_ISE_KEY_OUT_VLD_CNT_T;

typedef struct dpp_ppu_cluster_ise_cluster_rsp_in_vld_cnt_t
{
    ZXIC_UINT32 ise_cluster_rsp_in_vld_cnt;
}DPP_PPU_CLUSTER_ISE_CLUSTER_RSP_IN_VLD_CNT_T;

typedef struct dpp_ppu_cluster_cluster_ese_key_out_vld_cnt_t
{
    ZXIC_UINT32 cluster_ese_key_out_vld_cnt;
}DPP_PPU_CLUSTER_CLUSTER_ESE_KEY_OUT_VLD_CNT_T;

typedef struct dpp_ppu_cluster_ese_cluster_rsp_in_vld_cnt_t
{
    ZXIC_UINT32 ese_cluster_rsp_in_vld_cnt;
}DPP_PPU_CLUSTER_ESE_CLUSTER_RSP_IN_VLD_CNT_T;

typedef struct dpp_ppu_cluster_cluster_stat_cmd_vld_cnt_t
{
    ZXIC_UINT32 cluster_stat_cmd_vld_cnt;
}DPP_PPU_CLUSTER_CLUSTER_STAT_CMD_VLD_CNT_T;

typedef struct dpp_ppu_cluster_stat_cluster_rsp_vld_cnt_t
{
    ZXIC_UINT32 stat_cluster_rsp_vld_cnt;
}DPP_PPU_CLUSTER_STAT_CLUSTER_RSP_VLD_CNT_T;

typedef struct dpp_ppu_cluster_mex_debug_key_vld_cnt_t
{
    ZXIC_UINT32 mex_debug_key_vld_cnt;
}DPP_PPU_CLUSTER_MEX_DEBUG_KEY_VLD_CNT_T;

typedef struct dpp_ppu_cluster_ise_cluster_key_fc_cnt_t
{
    ZXIC_UINT32 ise_cluster_key_fc_cnt;
}DPP_PPU_CLUSTER_ISE_CLUSTER_KEY_FC_CNT_T;

typedef struct dpp_ppu_cluster_ese_cluster_key_fc_cnt_t
{
    ZXIC_UINT32 ese_cluster_key_fc_cnt;
}DPP_PPU_CLUSTER_ESE_CLUSTER_KEY_FC_CNT_T;

typedef struct dpp_ppu_cluster_cluster_ise_rsp_fc_cnt_t
{
    ZXIC_UINT32 cluster_ise_rsp_fc_cnt;
}DPP_PPU_CLUSTER_CLUSTER_ISE_RSP_FC_CNT_T;

typedef struct dpp_ppu_cluster_cluster_ese_rsp_fc_cnt_t
{
    ZXIC_UINT32 cluster_ese_rsp_fc_cnt;
}DPP_PPU_CLUSTER_CLUSTER_ESE_RSP_FC_CNT_T;

typedef struct dpp_ppu_cluster_stat_cluster_cmd_fc_cnt_t
{
    ZXIC_UINT32 stat_cluster_cmd_fc_cnt;
}DPP_PPU_CLUSTER_STAT_CLUSTER_CMD_FC_CNT_T;

typedef struct dpp_ppu_cluster_cluster_stat_rsp_fc_cnt_t
{
    ZXIC_UINT32 cluster_stat_rsp_fc_cnt;
}DPP_PPU_CLUSTER_CLUSTER_STAT_RSP_FC_CNT_T;

typedef struct dpp_ppu_cluster_cluster_ppu_mf_vld_cnt_l_t
{
    ZXIC_UINT32 cluster_ppu_mf_vld_cnt_l;
}DPP_PPU_CLUSTER_CLUSTER_PPU_MF_VLD_CNT_L_T;

typedef struct dpp_ppu_cluster_cluster_ppu_mf_vld_cnt_h_t
{
    ZXIC_UINT32 cluster_ppu_mf_vld_cnt_h;
}DPP_PPU_CLUSTER_CLUSTER_PPU_MF_VLD_CNT_H_T;

typedef struct dpp_ppu_cluster_cluster_cop_key_vld_cnt_l_t
{
    ZXIC_UINT32 cluster_cop_key_vld_cnt_l;
}DPP_PPU_CLUSTER_CLUSTER_COP_KEY_VLD_CNT_L_T;

typedef struct dpp_ppu_cluster_cluster_cop_key_vld_cnt_h_t
{
    ZXIC_UINT32 cluster_cop_key_vld_cnt_h;
}DPP_PPU_CLUSTER_CLUSTER_COP_KEY_VLD_CNT_H_T;

typedef struct dpp_ppu_cluster_cop_cluster_rsp_vld_cnt_l_t
{
    ZXIC_UINT32 cop_cluster_rsp_vld_cnt_l;
}DPP_PPU_CLUSTER_COP_CLUSTER_RSP_VLD_CNT_L_T;

typedef struct dpp_ppu_cluster_cop_cluster_rsp_vld_cnt_h_t
{
    ZXIC_UINT32 cop_cluster_rsp_vld_cnt_h;
}DPP_PPU_CLUSTER_COP_CLUSTER_RSP_VLD_CNT_H_T;

typedef struct dpp_ppu_cluster_mex_me_pkt_in_sop_cnt_l_t
{
    ZXIC_UINT32 mex_me_pkt_in_sop_cnt_l;
}DPP_PPU_CLUSTER_MEX_ME_PKT_IN_SOP_CNT_L_T;

typedef struct dpp_ppu_cluster_mex_me_pkt_in_sop_cnt_h_t
{
    ZXIC_UINT32 mex_me_pkt_in_sop_cnt_h;
}DPP_PPU_CLUSTER_MEX_ME_PKT_IN_SOP_CNT_H_T;

typedef struct dpp_ppu_cluster_mex_me_pkt_in_eop_cnt_l_t
{
    ZXIC_UINT32 mex_me_pkt_in_eop_cnt_l;
}DPP_PPU_CLUSTER_MEX_ME_PKT_IN_EOP_CNT_L_T;

typedef struct dpp_ppu_cluster_mex_me_pkt_in_eop_cnt_h_t
{
    ZXIC_UINT32 mex_me_pkt_in_eop_cnt_h;
}DPP_PPU_CLUSTER_MEX_ME_PKT_IN_EOP_CNT_H_T;

typedef struct dpp_ppu_cluster_mex_me_pkt_in_vld_cnt_l_t
{
    ZXIC_UINT32 mex_me_pkt_in_vld_cnt_l;
}DPP_PPU_CLUSTER_MEX_ME_PKT_IN_VLD_CNT_L_T;

typedef struct dpp_ppu_cluster_mex_me_pkt_in_vld_cnt_h_t
{
    ZXIC_UINT32 mex_me_pkt_in_vld_cnt_h;
}DPP_PPU_CLUSTER_MEX_ME_PKT_IN_VLD_CNT_H_T;

typedef struct dpp_ppu_cluster_me_mex_pkt_out_sop_cnt_l_t
{
    ZXIC_UINT32 me_mex_pkt_out_sop_cnt_l;
}DPP_PPU_CLUSTER_ME_MEX_PKT_OUT_SOP_CNT_L_T;

typedef struct dpp_ppu_cluster_me_mex_pkt_out_sop_cnt_h_t
{
    ZXIC_UINT32 me_mex_pkt_out_sop_cnt_h;
}DPP_PPU_CLUSTER_ME_MEX_PKT_OUT_SOP_CNT_H_T;

typedef struct dpp_ppu_cluster_me_mex_pkt_out_eop_cnt_l_t
{
    ZXIC_UINT32 me_mex_pkt_out_eop_cnt_l;
}DPP_PPU_CLUSTER_ME_MEX_PKT_OUT_EOP_CNT_L_T;

typedef struct dpp_ppu_cluster_me_mex_pkt_out_eop_cnt_h_t
{
    ZXIC_UINT32 me_mex_pkt_out_eop_cnt_h;
}DPP_PPU_CLUSTER_ME_MEX_PKT_OUT_EOP_CNT_H_T;

typedef struct dpp_ppu_cluster_me_mex_pkt_out_vld_cnt_l_t
{
    ZXIC_UINT32 me_mex_pkt_out_vld_cnt_l;
}DPP_PPU_CLUSTER_ME_MEX_PKT_OUT_VLD_CNT_L_T;

typedef struct dpp_ppu_cluster_me_mex_pkt_out_vld_cnt_h_t
{
    ZXIC_UINT32 me_mex_pkt_out_vld_cnt_h;
}DPP_PPU_CLUSTER_ME_MEX_PKT_OUT_VLD_CNT_H_T;

typedef struct dpp_ppu_cluster_me_mex_i_key_out_sop_cnt_l_t
{
    ZXIC_UINT32 me_mex_i_key_out_sop_cnt_l;
}DPP_PPU_CLUSTER_ME_MEX_I_KEY_OUT_SOP_CNT_L_T;

typedef struct dpp_ppu_cluster_me_mex_i_key_out_sop_cnt_h_t
{
    ZXIC_UINT32 me_mex_i_key_out_sop_cnt_h;
}DPP_PPU_CLUSTER_ME_MEX_I_KEY_OUT_SOP_CNT_H_T;

typedef struct dpp_ppu_cluster_me_mex_i_key_out_eop_cnt_l_t
{
    ZXIC_UINT32 me_mex_i_key_out_eop_cnt_l;
}DPP_PPU_CLUSTER_ME_MEX_I_KEY_OUT_EOP_CNT_L_T;

typedef struct dpp_ppu_cluster_me_mex_i_key_out_eop_cnt_h_t
{
    ZXIC_UINT32 me_mex_i_key_out_eop_cnt_h;
}DPP_PPU_CLUSTER_ME_MEX_I_KEY_OUT_EOP_CNT_H_T;

typedef struct dpp_ppu_cluster_me_mex_i_key_out_vld_cnt_l_t
{
    ZXIC_UINT32 me_mex_i_key_out_vld_cnt_l;
}DPP_PPU_CLUSTER_ME_MEX_I_KEY_OUT_VLD_CNT_L_T;

typedef struct dpp_ppu_cluster_me_mex_i_key_out_vld_cnt_h_t
{
    ZXIC_UINT32 me_mex_i_key_out_vld_cnt_h;
}DPP_PPU_CLUSTER_ME_MEX_I_KEY_OUT_VLD_CNT_H_T;

typedef struct dpp_ppu_cluster_me_mex_e_key_out_sop_cnt_l_t
{
    ZXIC_UINT32 me_mex_e_key_out_sop_cnt_l;
}DPP_PPU_CLUSTER_ME_MEX_E_KEY_OUT_SOP_CNT_L_T;

typedef struct dpp_ppu_cluster_me_mex_e_key_out_sop_cnt_h_t
{
    ZXIC_UINT32 me_mex_e_key_out_sop_cnt_h;
}DPP_PPU_CLUSTER_ME_MEX_E_KEY_OUT_SOP_CNT_H_T;

typedef struct dpp_ppu_cluster_me_mex_e_key_out_eop_cnt_l_t
{
    ZXIC_UINT32 me_mex_e_key_out_eop_cnt_l;
}DPP_PPU_CLUSTER_ME_MEX_E_KEY_OUT_EOP_CNT_L_T;

typedef struct dpp_ppu_cluster_me_mex_e_key_out_eop_cnt_h_t
{
    ZXIC_UINT32 me_mex_e_key_out_eop_cnt_h;
}DPP_PPU_CLUSTER_ME_MEX_E_KEY_OUT_EOP_CNT_H_T;

typedef struct dpp_ppu_cluster_me_mex_e_key_out_vld_cnt_l_t
{
    ZXIC_UINT32 me_mex_e_key_out_vld_cnt_l;
}DPP_PPU_CLUSTER_ME_MEX_E_KEY_OUT_VLD_CNT_L_T;

typedef struct dpp_ppu_cluster_me_mex_e_key_out_vld_cnt_h_t
{
    ZXIC_UINT32 me_mex_e_key_out_vld_cnt_h;
}DPP_PPU_CLUSTER_ME_MEX_E_KEY_OUT_VLD_CNT_H_T;

typedef struct dpp_ppu_cluster_me_mex_demux_ise_key_vld_cnt_l_t
{
    ZXIC_UINT32 me_mex_demux_ise_key_vld_cnt_l;
}DPP_PPU_CLUSTER_ME_MEX_DEMUX_ISE_KEY_VLD_CNT_L_T;

typedef struct dpp_ppu_cluster_me_mex_demux_ise_key_vld_cnt_h_t
{
    ZXIC_UINT32 me_mex_demux_ise_key_vld_cnt_h;
}DPP_PPU_CLUSTER_ME_MEX_DEMUX_ISE_KEY_VLD_CNT_H_T;

typedef struct dpp_ppu_cluster_me_mex_demux_ese_key_vld_cnt_l_t
{
    ZXIC_UINT32 me_mex_demux_ese_key_vld_cnt_l;
}DPP_PPU_CLUSTER_ME_MEX_DEMUX_ESE_KEY_VLD_CNT_L_T;

typedef struct dpp_ppu_cluster_me_mex_demux_ese_key_vld_cnt_h_t
{
    ZXIC_UINT32 me_mex_demux_ese_key_vld_cnt_h;
}DPP_PPU_CLUSTER_ME_MEX_DEMUX_ESE_KEY_VLD_CNT_H_T;

typedef struct dpp_ppu_cluster_me_mex_demux_sta_key_vld_cnt_l_t
{
    ZXIC_UINT32 me_mex_demux_sta_key_vld_cnt_l;
}DPP_PPU_CLUSTER_ME_MEX_DEMUX_STA_KEY_VLD_CNT_L_T;

typedef struct dpp_ppu_cluster_me_mex_demux_sta_key_vld_cnt_h_t
{
    ZXIC_UINT32 me_mex_demux_sta_key_vld_cnt_h;
}DPP_PPU_CLUSTER_ME_MEX_DEMUX_STA_KEY_VLD_CNT_H_T;

typedef struct dpp_ppu_cluster_me_mex_demux_cop_key_vld_cnt_l_t
{
    ZXIC_UINT32 me_mex_demux_cop_key_vld_cnt_l;
}DPP_PPU_CLUSTER_ME_MEX_DEMUX_COP_KEY_VLD_CNT_L_T;

typedef struct dpp_ppu_cluster_me_mex_demux_cop_key_vld_cnt_h_t
{
    ZXIC_UINT32 me_mex_demux_cop_key_vld_cnt_h;
}DPP_PPU_CLUSTER_ME_MEX_DEMUX_COP_KEY_VLD_CNT_H_T;

typedef struct dpp_ppu_cluster_mex_me_demux_ise_rsp_vld_cnt_l_t
{
    ZXIC_UINT32 mex_me_demux_ise_rsp_vld_cnt_l;
}DPP_PPU_CLUSTER_MEX_ME_DEMUX_ISE_RSP_VLD_CNT_L_T;

typedef struct dpp_ppu_cluster_mex_me_demux_ise_rsp_vld_cnt_h_t
{
    ZXIC_UINT32 mex_me_demux_ise_rsp_vld_cnt_h;
}DPP_PPU_CLUSTER_MEX_ME_DEMUX_ISE_RSP_VLD_CNT_H_T;

typedef struct dpp_ppu_cluster_mex_me_demux_ese_rsp_vld_cnt_l_t
{
    ZXIC_UINT32 mex_me_demux_ese_rsp_vld_cnt_l;
}DPP_PPU_CLUSTER_MEX_ME_DEMUX_ESE_RSP_VLD_CNT_L_T;

typedef struct dpp_ppu_cluster_mex_me_demux_ese_rsp_vld_cnt_h_t
{
    ZXIC_UINT32 mex_me_demux_ese_rsp_vld_cnt_h;
}DPP_PPU_CLUSTER_MEX_ME_DEMUX_ESE_RSP_VLD_CNT_H_T;

typedef struct dpp_ppu_cluster_mex_me_demux_sta_rsp_vld_cnt_l_t
{
    ZXIC_UINT32 mex_me_demux_sta_rsp_vld_cnt_l;
}DPP_PPU_CLUSTER_MEX_ME_DEMUX_STA_RSP_VLD_CNT_L_T;

typedef struct dpp_ppu_cluster_mex_me_demux_sta_rsp_vld_cnt_h_t
{
    ZXIC_UINT32 mex_me_demux_sta_rsp_vld_cnt_h;
}DPP_PPU_CLUSTER_MEX_ME_DEMUX_STA_RSP_VLD_CNT_H_T;

typedef struct dpp_ppu_cluster_mex_me_demux_cop_rsp_vld_cnt_l_t
{
    ZXIC_UINT32 mex_me_demux_cop_rsp_vld_cnt_l;
}DPP_PPU_CLUSTER_MEX_ME_DEMUX_COP_RSP_VLD_CNT_L_T;

typedef struct dpp_ppu_cluster_mex_me_demux_cop_rsp_vld_cnt_h_t
{
    ZXIC_UINT32 mex_me_demux_cop_rsp_vld_cnt_h;
}DPP_PPU_CLUSTER_MEX_ME_DEMUX_COP_RSP_VLD_CNT_H_T;

typedef struct dpp_ppu_cluster_me_exception_code0_cnt_l_t
{
    ZXIC_UINT32 me_exception_code0_cnt_l;
}DPP_PPU_CLUSTER_ME_EXCEPTION_CODE0_CNT_L_T;

typedef struct dpp_ppu_cluster_me_exception_code0_cnt_h_t
{
    ZXIC_UINT32 me_exception_code0_cnt_h;
}DPP_PPU_CLUSTER_ME_EXCEPTION_CODE0_CNT_H_T;

typedef struct dpp_ppu_cluster_me_exception_code1_cnt_l_t
{
    ZXIC_UINT32 me_exception_code1_cnt_l;
}DPP_PPU_CLUSTER_ME_EXCEPTION_CODE1_CNT_L_T;

typedef struct dpp_ppu_cluster_me_exception_code1_cnt_h_t
{
    ZXIC_UINT32 me_exception_code1_cnt_h;
}DPP_PPU_CLUSTER_ME_EXCEPTION_CODE1_CNT_H_T;

typedef struct dpp_ppu_cluster_me_exception_code2_cnt_l_t
{
    ZXIC_UINT32 me_exception_code2_cnt_l;
}DPP_PPU_CLUSTER_ME_EXCEPTION_CODE2_CNT_L_T;

typedef struct dpp_ppu_cluster_me_exception_code2_cnt_h_t
{
    ZXIC_UINT32 me_exception_code2_cnt_h;
}DPP_PPU_CLUSTER_ME_EXCEPTION_CODE2_CNT_H_T;

typedef struct dpp_ppu_cluster_me_exception_code3_cnt_l_t
{
    ZXIC_UINT32 me_exception_code3_cnt_l;
}DPP_PPU_CLUSTER_ME_EXCEPTION_CODE3_CNT_L_T;

typedef struct dpp_ppu_cluster_me_exception_code3_cnt_h_t
{
    ZXIC_UINT32 me_exception_code3_cnt_h;
}DPP_PPU_CLUSTER_ME_EXCEPTION_CODE3_CNT_H_T;

typedef struct dpp_ppu_cluster_me_exception_code4_cnt_l_t
{
    ZXIC_UINT32 me_exception_code4_cnt_l;
}DPP_PPU_CLUSTER_ME_EXCEPTION_CODE4_CNT_L_T;

typedef struct dpp_ppu_cluster_me_exception_code4_cnt_h_t
{
    ZXIC_UINT32 me_exception_code4_cnt_h;
}DPP_PPU_CLUSTER_ME_EXCEPTION_CODE4_CNT_H_T;

typedef struct dpp_ppu_cluster_me_exception_code5_cnt_l_t
{
    ZXIC_UINT32 me_exception_code5_cnt_l;
}DPP_PPU_CLUSTER_ME_EXCEPTION_CODE5_CNT_L_T;

typedef struct dpp_ppu_cluster_me_exception_code5_cnt_h_t
{
    ZXIC_UINT32 me_exception_code5_cnt_h;
}DPP_PPU_CLUSTER_ME_EXCEPTION_CODE5_CNT_H_T;


#ifdef __cplusplus
}
#endif
#endif

