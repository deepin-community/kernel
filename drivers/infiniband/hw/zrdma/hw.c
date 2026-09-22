// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */
#include "main.h"
#include "user.h"
#include "icrdma_hw.h"
#include "hmc.h"
#include "slib.h"

/* types of hmc objects */
enum zxdh_hmc_rsrc_type iw_hmc_obj_types[ZXDH_HMC_IW_TXWINDOW + 1] = {
	ZXDH_HMC_IW_QP, ZXDH_HMC_IW_CQ,	 ZXDH_HMC_IW_SRQ,      ZXDH_HMC_IW_AH,
	ZXDH_HMC_IW_MR, ZXDH_HMC_IW_IRD, ZXDH_HMC_IW_TXWINDOW,
};

struct zxdh_aeq_cqp_errorcode_tbl error_code_info[] = {
	{ZXDH_AE_REQ_AXI_RSP_ERR, "tx/rdmatx_parse_top/wqe_ddr_axi_err"},
	{ZXDH_AE_REQ_WQE_FLUSH, "tx/rdmatx_ack_sys_top/flush_aeq"},
	{ZXDH_AE_REQ_WR_ORD_ERR, "tx/rdmatx_ack_sys_top/ord_size_err"},
	{ZXDH_AE_REQ_WR_INV_OPCODE, "tx/rdmatx_parse_top/opcode_err_flag"},
	{ZXDH_AE_REQ_WR_CQP_QP_STATE, "tx/rdmatx_parse_top/cqp_state_axi_err"},
	{ZXDH_AE_REQ_WR_LEN_ERR, "tx/rdmatx_parse_top/fragment_num_err"},
	{ZXDH_AE_REQ_WR_INLINE_LEN_ERR, "tx/rdmatx_parse_top/inline_data_length_err"},
	{ZXDH_AE_REQ_WR_AH_VALID_ERR, "tx/rdmatx_parse_top/ah_valid_err"},
	{ZXDH_AE_REQ_WR_UD_PD_IDX_ERR, "tx/rdmatx_parse_top/ud_pd_index_err"},
	{IRMDA_AE_REQ_WR_QP_STATE_ERR, "tx/rdmatx_parse_top/qp_state_err"},
	{ZXDH_AE_REQ_WR_SERVER_TYPE_MISMATCH_OPCODE, "tx/rdmatx_parse_top/service_type_err"},
	{ZXDH_AE_REQ_WR_UD_PAYLOAD_OUT_OF_PMTU, "tx/rdmatx_parse_top/wqe_axiw_resp_err"},
	{ZXDH_AE_REQ_WR_PRE_READ_MOD_WQE_LEN_ZERO, "tx/rdmatx_parse_top/wqe_len_err"},
	{ZXDH_AE_REQ_WR_ADDL_SGE_NOT_READ_BACK, "tx/rdmatx_parse_top/wqe_deficient_clr_err"},
	{ZXDH_AE_REQ_WR_IMM_OPCODE_MISMATCH_FLAG, "tx/rdmatx_parse_top/immdt_err"},
	{ZXDH_AE_REQ_HAD_SEND_MSG_OUT_OF_RANGE, "tx/rdmatx_parse_top/wqe_offset_err"},
	{ZXDH_AE_REQ_ARP_LENGTH_ERR, "tx/rdmatx_parse_top/arp_length_err"},
	{ZXDH_AE_REQ_ARP_ADDR_ERR, "tx/rdmatx_parse_top/arp_addr_err"},
	{ZXDH_AE_REQ_RC_PLD_LENGTH_ERR, "tx/rdmatx_parse_top/rc_pld_length_err"},
	{ZXDH_AE_REQ_NVME_IDX_ERR, "tx/rdmatx_ack_sys_top/nvme_index_err"},
	{ZXDH_AE_REQ_NVME_NOF_QID_ERR, "tx/rdmatx_ack_sys_top/nvme_nof_qid_err"},
	{ZXDH_AE_REQ_NVME_PD_IDX_ERR, "tx/rdmatx_ack_sys_top/nvme_nof_pd_index_err"},
	{ZXDH_AE_REQ_NVME_LEN_ERR, "tx/rdmatx_ack_sys_top/nvme_length_err"},
	{ZXDH_AE_REQ_NVME_KEY_ERR, "tx/rdmatx_ack_sys_top/nvme_key_err"},
	{ZXDH_AE_REQ_NVME_ACC_ERR, "tx/rdmatx_ack_sys_top/nvme_access_err"},
	{ZXDH_AE_REQ_NVME_TX_ROUTE_IDX_ERR, "tx/rdmatx_doorbell_mgr/index check error"},
	{ZXDH_AE_REQ_NVME_TX_ROUTE_NOF_QID_ERR, "tx/rdmatx_doorbell_mgr/qid check error"},
	{ZXDH_AE_REQ_NVME_TX_ROUTE_PD_IDX_ERR, "tx/rdmatx_doorbell_mgr/pd_index check error"},
	{ZXDH_AE_REQ_NVME_TX_ROUTE_LEN_ERR, "tx/rdmatx_doorbell_mgr/length check error"},
	{ZXDH_AE_REQ_NVME_TX_ROUTE_KEY_ERR, "tx/rdmatx_doorbell_mgr/key check error"},
	{ZXDH_AE_REQ_NVME_TX_ROUTE_ACC_ERR, "tx/rdmatx_doorbell_mgr/access check error"},
	{ZXDH_AE_REQ_MRTE_STATE_FREE, "tx/rdmatx_ack_sys_top/mrte_state_err0"},
	{ZXDH_AE_REQ_MRTE_STATE_INVALID, "tx/rdmatx_ack_sys_top/mrte_state_err1"},
	{ZXDH_AE_REQ_MRTE_MW_QP_ID_ERR, "tx/rdmatx_ack_sys_top/mrte_qpn_err"},
	{ZXDH_AE_REQ_MRTE_PD_IDX_ERR, "tx/rdmatx_ack_sys_top/mrte_pd_index_err"},
	{ZXDH_AE_REQ_MRTE_KEY_ERR, "tx/rdmatx_ack_sys_top/mrte_R_key_err"},
	{ZXDH_AE_REQ_MRTE_STAG_IDX_RANGE_ERR, "tx/rdmatx_ack_sys_top/mrte_key_err1"},
	{ZXDH_AE_REQ_MRTE_VIRT_ADDR_AND_LEN_ERR, "tx/rdmatx_ack_sys_top/vaddr_length_err"},
	{ZXDH_AE_REQ_MRTE_ACC_ERR, "tx/rdmatx_ack_sys_top/access_permis_err"},
	{ZXDH_AE_REQ_MRTE_STAG_IDX_RANGE_RSV_ERR, "tx/rdmatx_ack_sys_top/mrte_key_err2"},
	{ZXDH_AE_REQ_LOC_LEN_READ_REP_ERR, "tx/rdmatx_ack_sys_top/loc_len_err"},
	{ZXDH_AE_REQ_REM_INV_OPCODE, "tx/rdmatx_ack_sys_top/nak_invalid_req"},
	{ZXDH_AE_REQ_REM_INV_RKEY, "tx/rdmatx_ack_sys_top/nak_remote_access_err"},
	{ZXDH_AE_REQ_REM_OPERATIONAL_ERR, "tx/rdmatx_ack_sys_top/nak_remote_operational_err"},
	{ZXDH_AE_REQ_RETURN_NAK, "tx/rdmatx_ack_sys_top/nak_reserved"},
	{ZXDH_AE_REQ_LOG_SQ_SIZE_MISMATCH_WQE_POINTER, "tx/rdmatx_ack_sys_top/sq_size_nomatch"},
	{ZXDH_AE_REQ_OFED_INVALID_SQ_OPCODE, "tx/rdmatx_ack_sys_top/illegal_ofed_sq_opocde"},
	{ZXDH_AE_REQ_NVME_INVALID_SQ_OPCODE, "tx/rdmatx_ack_sys_top/illegal_nvme_sq_opocde"},
	{ZXDH_AE_REQ_RETRY_EXC_PSN_OUT_RANGE, "tx/rdmatx_ack_sys_top/nak_retry_limit"},
	{ZXDH_AE_REQ_RETRY_EXC_ACK_PSN_OUT_RANGE, "tx/rdmatx_ack_sys_top/read_retry_limit"},
	{ZXDH_AE_REQ_RETRY_EXC_LOC_ACK_OUT_RANGE, "tx/rdmatx_ack_sys_top/timeout_retry_limit"},
	{ZXDH_AE_REQ_RETRY_EXC_RNR_NAK_OUT_RANGE, "tx/rdmatx_ack_sys_top/rnr_retry_limit"},
	{ZXDH_AE_REQ_RETRY_EXC_TX_WINDOW_GET_ENTRY_ERR, "tx/rdmatx_window_top/tx_window_no_entry"},
	{ZXDH_AE_REQ_RETRY_EXC_TX_WINDOW_MSN_FALLBACK, "tx/rdmatx_window_top/tx_window_back_msn"},
	{ZXDH_AE_REQ_RETRY_EXC_TX_WINDOW_MSN_LITTLE, "tx/rdmatx_window_top/tx_window_small_msn"},
	{ZXDH_AE_PSN_ABNORMAL, "tx/rdmatx_ack_sys_top/psn_abnormal"},
	{ZXDH_AE_REQ_PSN_LESS_THAN_START_PSN, "tx/rdmatx_ack_sys_top/illegal_wqe_len"},
	{ZXDH_AE_REQ_WQE_MRTE_STATE_FREE, "tx/rdmatx_parse_top/mrte_state_err0"},
	{ZXDH_AE_REQ_WQE_MRTE_STATE_INV, "tx/rdmatx_parse_top/mrte_state_err1"},
	{ZXDH_AE_REQ_WQE_MRTE_MW_QP_ID_ERR, "tx/rdmatx_parse_top/mrte_qpn_err"},
	{ZXDH_AE_REQ_WQE_MRTE_PD_IDX_ERR, "tx/rdmatx_parse_top/mrte_pd_index_err"},
	{ZXDH_AE_REQ_WQE_MRTE_KEY_ERR, "tx/rdmatx_parse_top/mrte_R_key_err"},
	{ZXDH_AE_REQ_WQE_MRTE_STAG_IDX_ERR, "tx/rdmatx_parse_top/mrte_key_err1"},
	{ZXDH_AE_REQ_WQE_MRTE_VIRT_ADDR_AND_LEN_CHK_ERR, "tx/rdmatx_parse_top/vaddr_length_err"},
	{ZXDH_AE_REQ_WQE_MRTE_ACC_ERR, "tx/rdmatx_parse_top/access_permis_err"},
	{ZXDH_AE_REQ_WQE_MRTE_RSV_LKEY_EN_ERR, "tx/rdmatx_parse_top/mrte_key_err2"},
	{ZXDH_AE_REQ_WR_WQE_ZERO_LEN_SGE, "tx/rdmatx_parse_top/fragment_length_err"},
	{ZXDH_AE_RSP_WQE_FLUSH, "rx/cqp_flush"},
	{ZXDH_AE_RSP_PRIFIELD_CHK_INV_OPCODE, "rx/rdmarx_prifield_check/pri_opcode_err"},
	{ZXDH_AE_RSP_PRIFIELD_CHK_OUT_OF_ORDER, "rx/rdmarx_prifield_check/pri_opcode_seq_err"},
	{ZXDH_AE_RSP_PRIFIELD_CHK_LEN_ERR, "rx/rdmarx_prifield_check/pri_length_err"},
	{ZXDH_AE_RSP_SRQ_CHK_SRQ_STA_ERR, "rx/rdmarx_read_srqc_top/srq_state_err"},
	{ZXDH_AE_RSP_WQE_CHK_FORMAT_ERR, "rx/rdmarx_read_wqe_top/wqe_sign_err"},
	{ZXDH_AE_RSP_WQE_CHK_LEN_ERR, "rx/rdmarx_read_wqe_top/wqe_len_err"},
	{ZXDH_AE_RSP_PKT_TYPE_NOF_IOQ_ERR, "rx/nof/nof_ioq_err"},
	{ZXDH_AE_RSP_PKT_TYPE_NOF_PD_IDX_ERR, "rx/nof/nof_pdnum_err"},
	{ZXDH_AE_RSP_PKT_TYPE_NOF_LEN_ERR, "rx/nof/nof_len_err"},
	{ZXDH_AE_RSP_PKT_TYPE_NOF_RKEY_ERR, "rx/nof/nof_rkey_err"},
	{ZXDH_AE_RSP_PKT_TYPE_NOF_ACC_ERR, "rx/nof/nof_acc_err"},
	{ZXDH_AE_RSP_PKT_TYPE_CQ_OVERFLOW, "rx/cq/cq_overflow_aeq_cqn"},
	{ZXDH_AE_RSP_PKT_TYPE_IRD_OVERFLOW_ERR, "rx/acknak/ird_ovf"},
	{ZXDH_AE_RSP_PKT_TYPE_CQ_OVERFLOW_QP, "rx/cq/cq_overflow_aeq_qpn"},
	{ZXDH_AE_RSP_PKT_TYPE_CQ_STATE, "rx/cq/cq_state_err_aeq_qpn"},
	{ZXDH_AE_RSP_PKT_TYPE_CQ_TWO_PBLE_RSP, "rx/cq/cq_axi_err_aeq_cqn"},
	{ZXDH_AE_RSP_PKT_TYPE_CQ_TWO_PBLE_RSP_QP, "rx/cq/cq_axi_err_aeq_qpn"},
	{ZXDH_AE_RSP_SRQ_WATER_SIG, "rx/rdmarx_read_srqc_top/srq_limit_water_wrn"},
	{ZXDH_AE_RSP_PKT_TYPE_MR_DISTRIBUTE_ERR, "rx/mrcheck/mr_mw_state_free_err"},
	{ZXDH_AE_RSP_PKT_TYPE_MR_INV_ERR, "rx/mrcheck/mr_mw_state_invalid_err"},
	{ZXDH_AE_RSP_PKT_TYPE_MR_QP_CHK_ERR, "rx/mrcheck/type2b_mw_qpn_check_err"},
	{ZXDH_AE_RSP_PKT_TYPE_MR_PD_CHK_ERR, "rx/mrcheck/mr_mw_pd_check_err"},
	{ZXDH_AE_RSP_PKT_TYPE_MR_KEY_CHK_ERR, "rx/mrcheck/mr_mw_key_check_err"},
	{ZXDH_AE_RSP_PKT_TYPE_MR_STAG_IDX_ERR, "rx/mrcheck/mr_mw_stag_index_check_err"},
	{ZXDH_AE_RSP_PKT_TYPE_MR_BOUNDARY_ERR, "rx/mrcheck/mr_mw_boundary_check_err"},
	{ZXDH_AE_RSP_PKT_TYPE_MR_ACC_ERR, "rx/mrcheck/mr_mw_access_check_err"},
	{ZXDH_AE_RSP_PKT_TYPE_MR_STAG0_ERR, "rx/mrcheck/mr_mw_0stag_index_check_err"},
	{ZXDH_AE_RSP_PKT_TYPE_MW_STATE_ERR, "rx/r_invalid/mw_state_invalid_err"},
	{ZXDH_AE_RSP_PKT_TYPE_MW_PD_ERR, "rx/r_invalid/mw_pd_check_err"},
	{ZXDH_AE_RSP_PKT_TYPE_MW_KEY_ERR, "rx/r_invalid/mw_rkey_check_err"},
	{ZXDH_AE_RSP_PKT_TYPE_MW_TYPE2B_QPN_ERR, "rx/r_invalid/type2bmw_qpn_check_err"},
	{ZXDH_AE_RSP_PKT_TYPE_MW_KEY_IDX_ERR, "rx/r_invalid/mw_stag_index_check_err"},
	{ZXDH_AE_RSP_PKT_TYPE_MW_SHARE_MR, "rx/r_invalid/mw_share_mr_check_err"},
	{ZXDH_AE_RSP_PKT_TYPE_MW_TYPE_ERR, "rx/r_invalid/mw_type1_check_err"},
	{ZXDH_AE_RSP_PKT_TYPE_REM_INV_PD_ERR, "rx/r_invalid/mr_pd_check_err"},
	{ZXDH_AE_RSP_PKT_TYPE_REM_INV_KEY_ERR, "rx/r_invalid/mr_rkey_check_err"},
	{ZXDH_AE_RSP_CHK_ERR_SHARE_MR, "rx/r_invalid/mr_share_mr_check_err"},
	{ZXDH_AE_RSP_MW_NUM_ERR, "rx/r_invalid/mr_bond_mw_num_check_err"},
	{ZXDH_AE_RSP_INV_EN_ERR, "rx/r_invalid/mr_canbe_r_invalid_check_err"},
	{ZXDH_AE_RSP_QP_AXI_RSP_ERR, "rx/rq/axi_resp_err"},
	{ZXDH_AE_RSP_SRQ_AXI_RSP_SIG, "rx/rdmarx_read_srqc_top/srq_axi_resp_err"},
	{ZXDH_CQP_UNSUPPORTED_OPCODE, "cqp/unsupported_opcode"},
	{ZXDH_CQP_WQE_NOT_VALID, "cqp/wqe_not_valid"},
	{ZXDH_CQP_QUERY_VHCA_FAIL, "cqp/query_vhca_fail"},
	{ZXDH_CQP_CONFIG_QPC_ADDR_ERR, "cqp/config_qpc_addr_error"},
	{ZXDH_CQP_INVALID_MRTE_INDEX, "cqp/invalid_mrte_index"},
	{ZXDH_CQP_INVALID_ACCESS_RIGHTS, "cqp/invalid_access_rights"},
	{ZXDH_CQP_INVALID_FLAG_ON_REG_MR, "cqp/invalid_flag_on_reg_mr"},
	{ZXDH_CQP_INVALID_PBL_HOST_PAGE, "cqp/invalid_pbl/host_page"},
	{ZXDH_CQP_INVALID_AH_INDEX, "cqp/invalid_ah_index"},
	{ZXDH_CQP_INVALID_MGC_INDEX, "cqp/invalid_mgc_index"},
	{ZXDH_CQP_QUERY_WR_ADDR_ERR, "cqp/query_wr_addr_error"},
	{ZXDH_CQP_DMA_WR_SRC_ADDR_ERR, "cqp/dma_wr_src_addr_error"},
	{ZXDH_CQP_DMA_WR32_INVALID_DATA_NUMBER, "cqp/dma_wr32_invalid_data_number"},
	{ZXDH_CQP_DMA_WR32_DST_ADDR_ERR, "cqp/dma_wr32_dst_addr_error"},
	{ZXDH_CQP_DMA_WR64_INVALID_DATA_NUMBER, "cqp/dma_wr64_invalid_data_number"},
	{ZXDH_CQP_DMA_WR64_DST_ADDR_ERR, "cqp/dma_wr64_dst_addr_error"},
	{ZXDH_CQP_DMA_RD_SRC_ADDR_ERR, "cqp/dma_rd_src_addr_error"},
	{ZXDH_CQP_DMA_RD_CQE_INVALID_DATA_NUMBER, "cqp/dma_rd_cqe_invalid_data_number"},
	{ZXDH_CQP_DMA_RD32_CQE_SRC_ADDR_ERR, "cqp/dma_rd32_cqe_src_addr_error"},
	{ZXDH_CQP_DMA_RD64_CQE_SRC_ADDR_ERR, "cqp/dma_rd64_cqe_src_addr_error"},
	{ZXDH_CQP_INVALID_CQ_INDEX, "cqp/invalid_cq_index"},
	{ZXDH_CQP_CONFIG_QPC_LENGTH_DISMATCH, "cqp/config_qpc_length_dismatch"},
	{ZXDH_CQP_CONFIG_QPC_OPCODE_DISMATCH, "cqp/config_qpc_opcode_dismatch"},
	{ZXDH_CQP_CONFIG_QPC_TX_TIME_OUT, "cqp/config_qpc_tx_time_out"},
	{ZXDH_CQP_CONFIG_QPC_RX_TIME_OUT, "cqp/config_qpc_rx_time_out"},
	{ZXDH_CQP_CONFIG_QPC_AXIM_RD_ERR, "cqp/config_qpc_axim_rd_error"},
	{ZXDH_CQP_CONFIG_CONTEXT_TIME_OUT, "cqp/config_context_time_out"},
	{ZXDH_CQP_QUERY_QPC_TX_TIME_OUT, "cqp/query_qpc_tx_time_out"},
	{ZXDH_CQP_QUERY_QPC_RX_TIME_OUT, "cqp/query_qpc_rx_time_out"},
	{ZXDH_CQP_FLUSH_WQE_TX_TIME_OUT, "cqp/flush_wqe_tx_time_out"},
	{ZXDH_CQP_FLUSH_WQE_RX_TIME_OUT, "cqp/flush_wqe_rx_time_out"},
	{ZXDH_CQP_QUERY_CONTEXT_TIME_OUT, "cqp/query_context_time_out"},
	{ZXDH_CQP_DMA32_C1_WAIT_INTERRUPT_TIME_OUT, "cqp/dma32_c1_wait_interrupt_time_out"},
	{ZXDH_CQP_C10_AXIM_WE_ERR, "cqp/c10_axim_we_error"},
	{ZXDH_CQP_DMA_C1_OPCODE_DISMATCH, "cqp/dma_c1_opcode_dismatch"},
	{ZXDH_CQP_DMA_C1_READ_DATA_ERR, "cqp/dma_c1_read_data_error"},
	{ZXDH_CQP_DMA_C1_LENGTH_DISMATCH, "cqp/dma_c1_length_dismatch"},
	{ZXDH_CQP_DMA_C1_AXIM_WE_ERR, "cqp/dma_c1_axim_we_error"},
	{ZXDH_CQP_DMA32_C0_WAIT_INTERRUPT_TIME_OUT, "cqp/dma32_c0_wait_interrupt_time_out"},
	{ZXDH_CQP_C00_AXIM_WE_ERR, "cqp/c00_axim_we_error"},
	{ZXDH_CQP_DMA_C0_OPCODE_DISMATCH, "cqp/dma_c0_opcode_dismatch"},
	{ZXDH_CQP_DMA_C0_READ_DATA_ERR, "cqp/dma_c0_read_data_error"},
	{ZXDH_CQP_DMA_C0_LENGTH_DISMATCH, "cqp/dma_c0_length_dismatch"},
	{ZXDH_CQP_DMA_C0_AXIM_WE_ERR, "cqp/dam_c0_axim_we_error"},
	{ZXDH_CQP_READ_MRTE_ERR, "cqp/read_mrte_error"},
	{ZXDH_CQP_DEALLOCATE_MR_STATE_ERR, "cqp/deallocate_mr_state_error"},
	{ZXDH_CQP_DEALLOCATE_MR_MEMORY_REGION_DISMATCH, "cqp/deallocate_mr_memory_region_dismatch"},
	{ZXDH_CQP_DEALLOCATE_MR_PD_DIMATCH, "cqp/deallocate_mr_pd_dismatch"},
	{ZXDH_CQP_DEALLOCATE_MR_MW_NUM_NONZERO, "cqp/deallocate_mr_mw_num_nonzero"},
	{ZXDH_CQP_DEALLOCATE_MW_STATE_ERR, "cqp/deallocate_mw_state_error"},
	{ZXDH_CQP_DEALLOCATE_MW_MEMORY_REGION_DISMATCH, "cqp/deallocate_mw_memory_region_dismatch"},
	{ZXDH_CQP_DEALLOCATE_MW_PD_DIMATCH, "cqp/deallocate_mw_pd_dismatch"},
	{ZXDH_CQP_DEALLOCATE_MW_MR_STATE_ERR, "cqp/deallocate_mw_mr_state_error"},
	{ZXDH_CQP_DEALLOCATE_MW_MR_MEMORY_REGION_DISMATCH, "cqp/deallocate_mw_mr_memory_region_dismatch"},
	{ZXDH_CQP_DEALLOCATE_MW_MR_MW_NUM_ZERO, "cqp/deallocate_mw_mr_mw_num_zero"},
	{ZXDH_CQP_ALLOCATE_MW_STATE_ERR, "cqp/allocate_mw_state_error"},
	{ZXDH_CQP_ALLOCATE_MR_STATE_ERR, "cqp/allocate_mr_state_error"},
	{ZXDH_CQP_REGISTER_MR_STATE_ERR, "cqp/register_mr_state_error"},
	{ZXDH_CQP_FIRST_RD_MRTE_ERR, "cqp/first_rd_mrte_error"},
	{ZXDH_CQP_FIRST_WE_MRTE_ERR, "cqp/first_we_mrte_error"},
	{ZXDH_CQP_SECOND_RD_MRTE_ERR, "cqp/second_rd_mrte_error"},
	{ZXDH_CQP_SECOND_WE_MRTE_ERR, "cqp/second_we_mrte_error"},
	{ZXDH_CQP_FIRST_REQUEST_TIME_OUT, "cqp/first_request_time_out"},
	{ZXDH_CQP_SECOND_REQUEST_TIME_OUT, "cqp/second_request_time_out"},
	{ZXDH_CQP_DEALLOCATE_MW_HAVE_AXI_AND_STATE_ERR, "cqp/deallocate_mw_have_axi_and_state_error"},
	{ZXDH_CQP_DEALLOCATE_MR_AXI_ERR_AND_MEMORY_REGION_DISMATCH, "cqp/deallocate_mr_axi_error_and_memory_region_dismatch"},
	{ZXDH_CQP_DEALLOCATE_MR_AXI_ERR_AND_MW_NUM_ZERO, "cqp/deallocate_mr_axi_error_and_mw_num_zero"},
	{ZXDH_CQP_ALLOCATE_MR_HAVE_AXI_AND_STATE_ERR, "cqp/allocate_mr_have_axi_error_and_state_error"},
	{ZXDH_CQP_REGISTER_MR_HAVE_AXI_AND_STATE_ERR, "cqp/register_mr_have_axi_error_and_state_error"},
	{ZXDH_CQP_CQPC_NOT_VALID, "cqp/cqpc_not_valid"},
	{ZXDH_CQP_AQ_SIZE_ERR, "cqp/sq_size_error"},
	{ZXDH_CQP_HEAD_ERR, "cqp/head_error"},
	{ZXDH_CQP_READ_WQE_ERR, "cqp/read_wqe_error"},
	{ZXDH_CQP_INVALID_CQP_CQ_INDEX, "cqp/invalid_cqp_cq_index"},
	{ZXDH_CQP_PARSE_WQE_AXIM_ERR, "cqp/parse_wqe_axim_error"},
	{ZXDH_CQP_TAIL_ERR, "cqp/tail_error"},
};

struct zxdh_errorcode_map_tbl errorcode_map[ZXDH_AEQ_ERRORCODE_MAP_LEN] = {0};

/**
 * zxdh_iwarp_ce_handler - handle iwarp completions
 * @iwcq: iwarp cq receiving event
 */
static void zxdh_iwarp_ce_handler(struct zxdh_sc_cq *iwcq)
{
	struct zxdh_cq *cq = iwcq->back_cq;

	if (cq != NULL) {
		if (!cq->user_mode)
			cq->armed = false;
		if (cq->ibcq.comp_handler && (iwcq->cq_uk.valid_cq == true))
			cq->ibcq.comp_handler(&cq->ibcq, cq->ibcq.cq_context);
	}
}

static void zxdh_ceq_ena_intr(struct zxdh_sc_dev *dev, u32 ceq_id);
int zxdh_vf_init_hmc(struct zxdh_pci_f *rf);
int zxdh_regis_hash_tbl(struct zxdh_aeq_cqp_errorcode_tbl error_code[], int len);
int zxdh_get_hash_val(u32 error_code);
int zxdh_create_hashkey_normal(u32 hash_val, struct zxdh_aeq_cqp_errorcode_tbl *phash_tbl);
int zxdh_solve_hash_collision(u32 hash_val, struct zxdh_aeq_cqp_errorcode_tbl *phash_tbl);
char *zxdh_get_aeq_info(u32 error_code);
void zxdh_free_errorcode_map(struct zxdh_errorcode_map_tbl errorcode_map[], u32 size);
static void zxdh_free_aeq_cqp_errorcode_tbl(void);

/**
 * zxdh_process_ceq - handle ceq for completions
 * @rf: RDMA PCI function
 * @ceq: ceq having cq for completion
 */
static void zxdh_process_ceq(struct zxdh_pci_f *rf, struct zxdh_ceq *ceq)
{
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_sc_ceq *sc_ceq;
	struct zxdh_sc_cq *cq;
	unsigned long flags;

	sc_ceq = &ceq->sc_ceq;
	do {
		spin_lock_irqsave(&ceq->ce_lock, flags);
		cq = zxdh_sc_process_ceq(dev, sc_ceq);
		if (!cq) {
			spin_unlock_irqrestore(&ceq->ce_lock, flags);
			break;
		}
		if (cq->cq_type == ZXDH_CQ_TYPE_IO)
			zxdh_iwarp_ce_handler(cq);
		spin_unlock_irqrestore(&ceq->ce_lock, flags);

		if (cq->cq_type == ZXDH_CQ_TYPE_CQP) {
			rf->sc_dev.ceq_interrupt = true;
			queue_work(rf->cqp_cmpl_wq, &rf->cqp_cmpl_work);
		}
	} while (1);
}

static void zxdh_set_flush_fields_requester(struct zxdh_sc_qp *qp,
					    struct zxdh_aeqe_info *info)
{
	switch (info->ae_id) {
	case ZXDH_AE_REQ_NVME_IDX_ERR:
	case ZXDH_AE_REQ_NVME_PD_IDX_ERR:
	case ZXDH_AE_REQ_NVME_KEY_ERR:
	case ZXDH_AE_REQ_NVME_ACC_ERR:
	case ZXDH_AE_REQ_NVME_TX_ROUTE_IDX_ERR:
	case ZXDH_AE_REQ_NVME_TX_ROUTE_PD_IDX_ERR:
	case ZXDH_AE_REQ_NVME_TX_ROUTE_KEY_ERR:
	case ZXDH_AE_REQ_NVME_TX_ROUTE_ACC_ERR:
	case ZXDH_AE_REQ_MW_INV_LKEY_ERR:
	case ZXDH_AE_REQ_MW_INV_TYPE_ERR:
	case ZXDH_AE_REQ_MW_INV_STATE_INV:
	case ZXDH_AE_REQ_MW_INV_PD_IDX_ERR:
	case ZXDH_AE_REQ_MW_INV_SHARE_MEM_ERR:
	case ZXDH_AE_REQ_MW_INV_PARENT_STATE_INV:
	case ZXDH_AE_REQ_MW_INV_MW_NUM_ZERO:
	case ZXDH_AE_REQ_MW_INV_MW_STAG_31_8_ZERO:
	case ZXDH_AE_REQ_MW_INV_QP_NUM_ERR:
	case ZXDH_AE_REQ_MR_INV_INV_LKEY_ERR:
	case ZXDH_AE_REQ_MR_INV_MW_NUM_ZERO:
	case ZXDH_AE_REQ_MR_INV_STATE_ERR:
	case ZXDH_AE_REQ_MR_INV_EN_ERR:
	case ZXDH_AE_REQ_MR_INV_SHARE_MEM_ERR:
	case ZXDH_AE_REQ_MR_INV_PD_IDX_ERR:
	case ZXDH_AE_REQ_MR_INV_MW_STAG_31_8_ZERO:
	case ZXDH_AE_REQ_MWBIND_WRITE_ACC_ERR:
	case ZXDH_AE_REQ_MWBIND_VA_BIND_ERR:
	case ZXDH_AE_REQ_MWBIND_PD_IDX_ERR:
	case ZXDH_AE_REQ_MWBIND_MRTE_STATE_TYPE_ERR:
	case ZXDH_AE_REQ_MWBIND_VA_LEN_ERR:
	case ZXDH_AE_REQ_MWBIND_TYPE_VA_ERR:
	case ZXDH_AE_REQ_MWBIND_TYPE_IDX_ERR:
	case ZXDH_AE_REQ_MWBIND_MRTE_MR_ERR:
	case ZXDH_AE_REQ_MWBIND_TYPE2_LEN_ERR:
	case ZXDH_AE_REQ_MWBIND_MRTE_STATE_ERR:
	case ZXDH_AE_REQ_MWBIND_QPC_EN_ERR:
	case ZXDH_AE_REQ_MWBIND_PARENT_MR_ERR:
	case ZXDH_AE_REQ_MWBIND_ACC_BIT4_ERR:
	case ZXDH_AE_REQ_MWBIND_MW_STAG_ERR:
	case ZXDH_AE_REQ_MWBIND_IDX_OUT_RANGE:
	case ZXDH_AE_REQ_MR_FASTREG_ACC_ERR:
	case ZXDH_AE_REQ_MR_FASTREG_PD_IDX_ERR:
	case ZXDH_AE_REQ_MR_FASTREG_MRTE_STATE_ERR:
	case ZXDH_AE_REQ_MR_FASTREG_MR_IS_NOT_1:
	case ZXDH_AE_REQ_MR_FASTREG_QPC_EN_ERR:
	case ZXDH_AE_REQ_MR_FASTREG_STAG_LEN_ERR:
	case ZXDH_AE_REQ_MR_FASTREG_SHARE_MR_ERR:
	case ZXDH_AE_REQ_MR_FASTREG_MW_STAG_ERR:
	case ZXDH_AE_REQ_MR_FASTREG_IDX_OUT_RANGE:
	case ZXDH_AE_REQ_MR_FASTREG_MR_EN_ERR:
	case ZXDH_AE_REQ_MW_BIND_PD_IDX_ERR:
	case ZXDH_AE_REQ_MRTE_STATE_FREE:
	case ZXDH_AE_REQ_MRTE_STATE_INVALID:
	case ZXDH_AE_REQ_MRTE_MW_QP_ID_ERR:
	case ZXDH_AE_REQ_MRTE_PD_IDX_ERR:
	case ZXDH_AE_REQ_MRTE_KEY_ERR:
	case ZXDH_AE_REQ_MRTE_STAG_IDX_RANGE_ERR:
	case ZXDH_AE_REQ_MRTE_VIRT_ADDR_AND_LEN_ERR:
	case ZXDH_AE_REQ_MRTE_ACC_ERR:
	case ZXDH_AE_REQ_MRTE_STAG_IDX_RANGE_RSV_ERR:
	case ZXDH_AE_REQ_REM_INV_RKEY:
	case ZXDH_AE_REQ_WQE_MRTE_STATE_FREE:
	case ZXDH_AE_REQ_WQE_MRTE_STATE_INV:
	case ZXDH_AE_REQ_WQE_MRTE_MW_QP_ID_ERR:
	case ZXDH_AE_REQ_WQE_MRTE_PD_IDX_ERR:
	case ZXDH_AE_REQ_WQE_MRTE_KEY_ERR:
	case ZXDH_AE_REQ_WQE_MRTE_STAG_IDX_ERR:
	case ZXDH_AE_REQ_WQE_MRTE_VIRT_ADDR_AND_LEN_CHK_ERR:
	case ZXDH_AE_REQ_WQE_MRTE_ACC_ERR:
	case ZXDH_AE_REQ_WQE_MRTE_RSV_LKEY_EN_ERR:
		qp->event_type = ZXDH_QP_EVENT_ACCESS_ERR;
		break;
	case ZXDH_AE_REQ_REM_INV_OPCODE:
	case ZXDH_AE_REQ_OFED_INVALID_SQ_OPCODE:
	case ZXDH_AE_REQ_NVME_INVALID_SQ_OPCODE:
		qp->event_type = ZXDH_QP_EVENT_REQ_ERR;
		break;
	default:
		qp->event_type = ZXDH_QP_EVENT_CATASTROPHIC;
		break;
	}
}

static void zxdh_set_flush_fields_responder(struct zxdh_sc_qp *qp,
					    struct zxdh_aeqe_info *info)
{
	switch (info->ae_id) {
	case ZXDH_AE_RSP_PRIFIELD_CHK_INV_OPCODE:
		qp->event_type = ZXDH_QP_EVENT_REQ_ERR;
		break;
	case ZXDH_AE_RSP_PKT_TYPE_NOF_PD_IDX_ERR:
	case ZXDH_AE_RSP_PKT_TYPE_NOF_RKEY_ERR:
	case ZXDH_AE_RSP_PKT_TYPE_NOF_ACC_ERR:
	case ZXDH_AE_RSP_PKT_TYPE_MR_DISTRIBUTE_ERR:
	case ZXDH_AE_RSP_PKT_TYPE_MR_INV_ERR:
	case ZXDH_AE_RSP_PKT_TYPE_MR_QP_CHK_ERR:
	case ZXDH_AE_RSP_PKT_TYPE_MR_PD_CHK_ERR:
	case ZXDH_AE_RSP_PKT_TYPE_MR_KEY_CHK_ERR:
	case ZXDH_AE_RSP_PKT_TYPE_MR_STAG_IDX_ERR:
	case ZXDH_AE_RSP_PKT_TYPE_MR_BOUNDARY_ERR:
	case ZXDH_AE_RSP_PKT_TYPE_MR_ACC_ERR:
	case ZXDH_AE_RSP_PKT_TYPE_MR_STAG0_ERR:
	case ZXDH_AE_RSP_PKT_TYPE_MW_STATE_ERR:
	case ZXDH_AE_RSP_PKT_TYPE_MW_PD_ERR:
	case ZXDH_AE_RSP_PKT_TYPE_MW_KEY_ERR:
	case ZXDH_AE_RSP_PKT_TYPE_MW_TYPE2B_QPN_ERR:
	case ZXDH_AE_RSP_PKT_TYPE_MW_KEY_IDX_ERR:
	case ZXDH_AE_RSP_PKT_TYPE_MW_SHARE_MR:
	case ZXDH_AE_RSP_PKT_TYPE_MW_TYPE_ERR:
	case ZXDH_AE_RSP_PKT_TYPE_REM_INV_PD_ERR:
	case ZXDH_AE_RSP_PKT_TYPE_REM_INV_KEY_ERR:
	case ZXDH_AE_RSP_PKT_TYPE_REM_INV_ACC_ERR:
	case ZXDH_AE_RSP_CHK_ERR_SHARE_MR:
	case ZXDH_AE_RSP_MW_NUM_ERR:
	case ZXDH_AE_RSP_INV_EN_ERR:
		qp->event_type = ZXDH_QP_EVENT_ACCESS_ERR;
		break;
	default:
		qp->event_type = ZXDH_QP_EVENT_CATASTROPHIC;
		break;
	}
}

int zxdh_set_smmu_invalid(struct zxdh_pci_f *rf)
{
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	u32 cnt = 0, val = 0, status = 0;

	writel(0, (u32 __iomem *)(dev->hw->hw_addr +
				  C_RDMA_CQP_CQ_DISTRIBUTE_DONE(dev->ep_id)));

	zxdh_sc_send_mailbox_cmd(dev, ZTE_ZXDH_OP_SET_SMMU_INVALID, 0x12, 0x13,
				 0x15, dev->vf_id);

	do {
		val = readl(dev->hw->hw_addr +
					C_RDMA_CQP_CQ_DISTRIBUTE_DONE(dev->ep_id));			
		if (cnt++ > ZXDH_MAILBOX_CYC_NUM * dev->hw_attrs.max_done_count) {
			status = -ETIMEDOUT;
			pr_info("[zxdh_rdma] vhca_id:%d waiting completed SET_SMMU_INVALID mailbox too long time,timeout!\n",dev->vhca_id);
			break;
		}
		if (dev->hw_attrs.skip_hw == true) {
			status = -ETIMEDOUT;
			break;
		}
		usleep_range(ZXDH_MAILBOX_SLEEP_TIME, ZXDH_MAILBOX_SLEEP_TIME + 10);
	} while (!val);

	return status;
}

int zxdh_vf_init_hmc(struct zxdh_pci_f *rf)
{
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	u32 cnt = 0, val = 0, status = 0;

	writel(0, (u32 __iomem *)(dev->hw->hw_addr +
				  C_RDMA_CQP_CQ_DISTRIBUTE_DONE(dev->ep_id)));

	zxdh_sc_send_mailbox_cmd(dev, ZTE_ZXDH_VCHNL_OP_GET_HMC_FCN, 0x12, 0x13,
				 0x15, dev->vf_id);

	do {
		val = readl(dev->hw->hw_addr +
					C_RDMA_CQP_CQ_DISTRIBUTE_DONE(dev->ep_id));			
		if (cnt++ > ZXDH_MAILBOX_CYC_NUM * dev->hw_attrs.max_done_count) {
			status = -ETIMEDOUT;
			pr_info("[zxdh_rdma] vhca_id:%d waiting completed GET_HMC mailbox too long time,timeout!\n",dev->vhca_id);
			break;
		}
		if (dev->hw_attrs.skip_hw == true) {
			status = -ETIMEDOUT;
			break;
		}
		usleep_range(ZXDH_MAILBOX_SLEEP_TIME, ZXDH_MAILBOX_SLEEP_TIME + 10);
	} while (!val);

	return status;
}

static int zxdh_vf_del_hmc(struct zxdh_pci_f *rf)
{
	u32 cnt = 0, val = 0;
	int ret;
	u32 cqp_status = 0xFFFF;
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	u64 cqp_status_phy_addr = C_RDMA_CQP_STATUS_PHY_ADDR + dev->vhca_id_pf * 0x1000;

	if (rf->ftype == 0)
		return 0;
	if (dev->hw_attrs.skip_hw == true)
		return 0;

	ret = zxdh_rdma_reg_read(rf, cqp_status_phy_addr, &cqp_status);
	if (ret || cqp_status != 1) {
		pr_err("[zxdh_rdma] %s[%d]: failed!\n", __func__, __LINE__);
		return -EPIPE;
	}

	if (!dev->hmc_use_dpu_ddr) {
		zxdh_set_smmu_invalid(rf);
	}

	writel(0, (u32 __iomem *)(dev->hw->hw_addr +
				C_RDMA_CQP_CQ_DISTRIBUTE_DONE(dev->ep_id)));

	zxdh_sc_send_mailbox_cmd(
		dev, ZTE_ZXDH_OP_DEL_HMC_OBJ_RANGE,
		0, 0, 0, rf->vf_id);
	do {
		val = readl(dev->hw->hw_addr + C_RDMA_CQP_CQ_DISTRIBUTE_DONE(dev->ep_id));
		if (cnt++ > ZXDH_MAILBOX_CYC_NUM * dev->hw_attrs.max_done_count) {
			pr_info("[zxdh_rdma] vhca_id:%d waiting completed DEL_HMC mailbox too long time,timeout!\n",dev->vhca_id);
			return -ETIMEDOUT;
		}
		if (dev->hw_attrs.skip_hw == true) {
			return -EPIPE;
		}
		usleep_range(ZXDH_MAILBOX_SLEEP_TIME, ZXDH_MAILBOX_SLEEP_TIME + 10);
	} while (!val);

	return 0;
}

static int zxdh_vf_init_np_tbl(struct zxdh_pci_f *rf)
{
	struct iidc_core_dev_info *cdev_info = rf->cdev;
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	u32 cnt = 0, val = 0;

	if (!rf->sc_dev.np_mode_low_lat) {
		writel(0, (u32 __iomem *)(dev->hw->hw_addr +
					  C_RDMA_CQP_CQ_DISTRIBUTE_DONE(dev->ep_id)));

		zxdh_sc_send_mailbox_cmd(dev, ZTE_ZXDH_OP_REQ_NP_CONFIG,
					 cdev_info->vport_id, 0, 0, dev->vf_id);
		do {
			val = readl(dev->hw->hw_addr +
						C_RDMA_CQP_CQ_DISTRIBUTE_DONE(dev->ep_id));
			if (cnt++ > ZXDH_MAILBOX_CYC_NUM * dev->hw_attrs.max_done_count) {
				pr_info("[zxdh_rdma] vhca_id:%d waiting completed NP_CONFIG mailbox too long time,timeout!\n",dev->vhca_id);
				return -ETIMEDOUT;
			}
			if (dev->hw_attrs.skip_hw == true) {
				return -ETIMEDOUT;
			}
			usleep_range(ZXDH_MAILBOX_SLEEP_TIME, ZXDH_MAILBOX_SLEEP_TIME + 10);
		} while (!val);
	}
	return 0;
}

static int zxdh_vf_deinit_np_tbl(struct zxdh_pci_f *rf)
{
	u32 cnt = 0, val = 0;
	int ret;
	struct iidc_core_dev_info *cdev_info = rf->cdev;
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	u32 cqp_status = 0xFFFF;
	u64 cqp_status_phy_addr = C_RDMA_CQP_STATUS_PHY_ADDR + dev->vhca_id_pf * 0x1000;

	if (rf->ftype == 0)
		return 0;
	if (dev->hw_attrs.skip_hw == true)
		return 0;

	ret = zxdh_rdma_reg_read(rf, cqp_status_phy_addr, &cqp_status);
	if (ret || cqp_status != 1) {
		pr_err("[zxdh_rdma][%s][%d]:read reg failed, ret:%d!\n", __func__, __LINE__, ret);
		return -EPIPE;
	}

	if (!dev->np_mode_low_lat) {
		writel(0, (u32 __iomem *)(dev->hw->hw_addr +
					C_RDMA_CQP_CQ_DISTRIBUTE_DONE(dev->ep_id)));

		zxdh_sc_send_mailbox_cmd(dev, ZTE_ZXDH_OP_DEL_NP_CONFIG,
					cdev_info->vport_id, 0, 0, dev->vf_id);
		do {
			val = readl(dev->hw->hw_addr + C_RDMA_CQP_CQ_DISTRIBUTE_DONE(dev->ep_id));
			if (cnt++ > ZXDH_MAILBOX_CYC_NUM * dev->hw_attrs.max_done_count) {
				pr_info("[zxdh_rdma] vhca_id:%d waiting completed NP_CONFIG mailbox too long time,timeout!\n",dev->vhca_id);
				return -ETIMEDOUT;
			}
			if (dev->hw_attrs.skip_hw == true) {
				return -EPIPE;
			}
			usleep_range(ZXDH_MAILBOX_SLEEP_TIME, ZXDH_MAILBOX_SLEEP_TIME + 10);
		} while (!val);
	}

	return 0;
}

void zxdh_stop_cap_worker(struct work_struct *work)
{
	uint32_t reg_val = 0;
	struct aeq_stop_cap_work *aeq_stop_cap_work =
		container_of(work, struct aeq_stop_cap_work, work);
	struct zxdh_pci_f *rf = aeq_stop_cap_work->rf;

	kfree(aeq_stop_cap_work);
	if (rf->sc_dev.tx_stop_on_aeq == 1){
		if (zxdh_rdma_reg_read(rf, RDMATX_DATA_START_CAP, &reg_val))
			pr_err("[zxdh_rdma] zxdh_rdma_reg_read RDMATX_DATA_START_CAP failed\n");
		if(reg_val != 0 && zxdh_rdma_reg_write(rf, RDMATX_DATA_START_CAP, 0))
			pr_err("[zxdh_rdma] zxdh_rdma_reg_write RDMATX_DATA_START_CAP failed\n");
		rf->sc_dev.tx_stop_on_aeq = 0;
	}
	
	if (rf->sc_dev.rx_stop_on_aeq == 1){
		if (zxdh_rdma_reg_read(rf, RDMARX_DATA_START_CAP, &reg_val))
			pr_err("[zxdh_rdma] zxdh_rdma_reg_read RDMARX_DATA_START_CAP failed\n");
		if (reg_val != 0 && zxdh_rdma_reg_write(rf, RDMARX_DATA_START_CAP, 0))
			pr_err("[zxdh_rdma] zxdh_rdma_reg_write RDMARX_DATA_START_CAP failed\n");
	    rf->sc_dev.rx_stop_on_aeq = 0;
	}
}

void zxdh_aeq_process_stop_cap(struct zxdh_pci_f *rf)
{
	struct aeq_stop_cap_work *stop_cap_work;

	stop_cap_work = kzalloc(sizeof(*stop_cap_work), GFP_ATOMIC);
	if (!stop_cap_work) {
		pr_err("[zxdh_rdma] kzalloc stop_cap_work failed!\n");
		return;
	}

	stop_cap_work->rf = rf;
	INIT_WORK(&stop_cap_work->work, zxdh_stop_cap_worker);
	queue_work(rf->iwdev->cleanup_wq, &stop_cap_work->work);
}

void zrdma_cleanup_rdma_tools_cfg(struct zxdh_pci_f *rf)
{
	if (rf->sc_dev.hw_attrs.skip_hw == false) {
		if (zxdh_rdma_reg_write(rf, RDMATX_DATA_START_CAP, 0))
			pr_err("[zxdh_rdma] zrdma_cleanup_rdma_tools_cfg write RDMATX_DATA_START_CAP failed\n");
		if (zxdh_rdma_reg_write(rf, RDMARX_DATA_START_CAP, 0))
			pr_err("[zxdh_rdma] zrdma_cleanup_rdma_tools_cfg write RDMARX_DATA_START_CAP failed\n");
	}
}

static void zxdh_report_aeq_info(u32 vhca_id, struct zxdh_aeq_report_info *report_info)
{
    bool need_print = false;
    u32 retry_cnt = 0;
    u32 cq_flow_cnt = 0;

    if (report_info == NULL)
        return;

    if (report_info->retry_timeout_cnt > ZXDH_AEQ_RETRY_TIMEOUT_NUM) {
        report_info->retry_timeout_cnt -= ZXDH_AEQ_RETRY_TIMEOUT_NUM;
        retry_cnt = report_info->retry_timeout_cnt;
        need_print = true;
    }

    if (report_info->cq_flow_qp_cnt > ZXDH_AEQ_CQ_FLOW_QP_NUM) {
        report_info->cq_flow_qp_cnt -= ZXDH_AEQ_CQ_FLOW_QP_NUM;
        cq_flow_cnt = report_info->cq_flow_qp_cnt;
        need_print = true;
    }

    if (!need_print)
        return;

    if (retry_cnt && cq_flow_cnt)
        pr_info("[zxdh_rdma]%s vhca_id:%d ae_id:2291 cnt:%d ae_id:120 cnt:%d\n",
                __func__, vhca_id, retry_cnt, cq_flow_cnt);
    else if (retry_cnt)
        pr_info("[zxdh_rdma]%s vhca_id:%d ae_id:2291 cnt:%d\n",
                __func__, vhca_id, retry_cnt);
    else
        pr_info("[zxdh_rdma]%s vhca_id:%d ae_id:120 cnt:%d\n",
                __func__, vhca_id, cq_flow_cnt);
}

/**
 * zxdh_process_aeq - handle aeq events
 * @rf: RDMA PCI function
 * @dev: sc device struct
 * @sc_aeq: pointer to aeq structure
 */
static void zxdh_process_aeq(struct zxdh_pci_f *rf, struct zxdh_sc_dev *dev, struct zxdh_sc_aeq *sc_aeq)
{
	struct zxdh_sc_dev *dbg_dev = &rf->sc_dev;
	struct zxdh_aeqe_info aeinfo;
	struct zxdh_aeqe_info *info = &aeinfo;
	int ret;
	struct zxdh_qp *iwqp = NULL;
	struct zxdh_cq *iwcq = NULL;
	struct zxdh_srq *iwsrq = NULL;
	struct zxdh_sc_qp *qp = NULL;
	unsigned long flags;
	struct ib_event ibevent;
    char *error_info;
	u32 aeqcnt = 0;
	struct zxdh_aeq_report_info report_info = {0};

	if (!sc_aeq->size)
		return;

	do {
		memset(info, 0, sizeof(*info));
		ret = zxdh_sc_get_next_aeqe(sc_aeq, info, &report_info);
		if (ret)
			break;
        
		aeqcnt++;

		atomic_inc(&rf->aeq.aeq_event_count);

		error_info = zxdh_get_aeq_info(info->ae_id);
		zxdh_dbg(
			dbg_dev,
			"AEQ: ae_id = 0x%x bool qp=%d qp_id = %d tcp_state=%d iwarp_state=%d ae_src=%d, error_info=%s\n",
			info->ae_id, info->qp, info->qp_cq_id, info->tcp_state,
			info->iwarp_state, info->ae_src, error_info);
		kfree(error_info);

		if ((rf->sc_dev.tx_stop_on_aeq != 0 || 
			rf->sc_dev.rx_stop_on_aeq != 0) &&
			info->ae_id != ZXDH_AE_RSP_SRQ_WATER_SIG){
			zxdh_aeq_process_stop_cap(rf);
		}
		
		if (info->qp) {
			spin_lock_irqsave(&rf->qptable_lock, flags);
			if (info->qp_cq_id == 1) {
				info->qp_cq_id = dev->base_qpn + 1;
			}
			if (info->qp_cq_id < dev->base_qpn) {
				spin_unlock_irqrestore(&rf->qptable_lock,
						       flags);
				pr_err("[zxdh_rdma] qp information is valid,qpn < base_qpn, qpn:%d\n",
				       info->qp_cq_id);
				continue;
			} else if (info->qp_cq_id >= (dev->base_qpn + dev->max_qp)) {
				spin_unlock_irqrestore(&rf->qptable_lock,
						       flags);
				pr_err("[zxdh_rdma] qp information is valid,qpn >= (base_qpn + max_qp), qpn:%d\n",
				       info->qp_cq_id);
				continue;
			} 
			iwqp = rf->qp_table[info->qp_cq_id - dev->base_qpn];
			if (!iwqp) {
				spin_unlock_irqrestore(&rf->qptable_lock,
						       flags);
				zxdh_dbg(dbg_dev,
					 "AEQ: qp_id %d is already freed\n",
					 info->qp_cq_id);
				continue;
			}
			zxdh_qp_add_ref(&iwqp->ibqp);
			spin_unlock_irqrestore(&rf->qptable_lock, flags);
			qp = &iwqp->sc_qp;
			spin_lock_irqsave(&iwqp->lock, flags);
			iwqp->hw_iwarp_state = info->iwarp_state;
			iwqp->last_aeq = info->ae_id;
			spin_unlock_irqrestore(&iwqp->lock, flags);
		} else {
			if (info->ae_id == ZXDH_AE_REQ_WQE_FLUSH)
				continue;
			else if (info->ae_id == ZXDH_AE_RSP_WQE_FLUSH)
				continue;
			else if (info->ae_id == ZXDH_AE_REQ_WR_CQP_QP_STATE) {
				pr_info("[zxdh_rdma] [%s] cqp qp state err!\n",__func__);
				continue;
			}
		}

		switch (info->ae_id) {
		case ZXDH_AE_RSP_SRQ_WATER_SIG:
			spin_lock_irqsave(&rf->srqtable_lock, flags);
			if (info->qp_cq_id < dev->base_srqn) {
				spin_unlock_irqrestore(&rf->srqtable_lock,
						       flags);
				pr_err("[zxdh_rdma] aeq srq water limit event,srqn < base_srqn, srqn:%d\n",
				       info->qp_cq_id);
				continue;
			} else if (info->qp_cq_id >= (dev->base_srqn + dev->max_srq)) {
				spin_unlock_irqrestore(&rf->srqtable_lock,
						       flags);
				pr_err("[zxdh_rdma] aeq srq water limit event,srqn >= (base_srqn + max_srq), srqn:%d\n",
				       info->qp_cq_id);
				continue;
			}
			iwsrq = rf->srq_table[info->qp_cq_id - dev->base_srqn];
			if (!iwsrq) {
				spin_unlock_irqrestore(&rf->srqtable_lock,
						       flags);
				zxdh_dbg(dbg_dev,
					 "AEQ: srq_id %d is already freed\n",
					 info->qp_cq_id);
				continue;
			}
			zxdh_srq_add_ref(&iwsrq->ibsrq);
			spin_unlock_irqrestore(&rf->srqtable_lock, flags);
			if (iwsrq->ibsrq.event_handler) {
				ibevent.device = iwsrq->ibsrq.device;
				ibevent.event = IB_EVENT_SRQ_LIMIT_REACHED;
				ibevent.element.srq = &iwsrq->ibsrq;
				iwsrq->ibsrq.event_handler(
					&ibevent, iwsrq->ibsrq.srq_context);
			}
			zxdh_srq_rem_ref(&iwsrq->ibsrq);
			break;
		case ZXDH_AE_RSP_PKT_TYPE_CQ_OVERFLOW:
		case ZXDH_AE_RSP_PKT_TYPE_CQ_TWO_PBLE_RSP:
			error_info = zxdh_get_aeq_info(info->ae_id);
			pr_info("Processing CQ[0x%x], ae_id: 0x%04X, info=%s\n",
                                info->qp_cq_id, info->ae_id, error_info);
			kfree(error_info);
			spin_lock_irqsave(&rf->cqtable_lock, flags);
			if (info->qp_cq_id < dev->base_cqn) {
				spin_unlock_irqrestore(&rf->cqtable_lock,
						       flags);
				pr_err("[zxdh_rdma] aeq cq err, cqn < base_cqn cqn:%d\n",
				       info->qp_cq_id);
				continue;
			} else if (info->qp_cq_id >= (dev->base_cqn + dev->max_cq)) {
				spin_unlock_irqrestore(&rf->cqtable_lock,
						       flags);
				pr_err("[zxdh_rdma] aeq cq err, cqn >= (base_cqn + max_cq) cqn:%d\n",
				       info->qp_cq_id);
				continue;
			}
			iwcq = rf->cq_table[info->qp_cq_id - dev->base_cqn];
			if (!iwcq) {
				spin_unlock_irqrestore(&rf->cqtable_lock,
						       flags);
				zxdh_dbg(dbg_dev,
					 "AEQ: cq_id %d is already freed\n",
					 info->qp_cq_id);
				continue;
			}
			zxdh_cq_add_ref(&iwcq->ibcq);
			spin_unlock_irqrestore(&rf->cqtable_lock, flags);
			if (iwcq->ibcq.event_handler) {
				ibevent.device = iwcq->ibcq.device;
				ibevent.event = IB_EVENT_CQ_ERR;
				ibevent.element.cq = &iwcq->ibcq;
				iwcq->ibcq.event_handler(&ibevent,
							 iwcq->ibcq.cq_context);
			}
			zxdh_cq_rem_ref(&iwcq->ibcq);
			break;
		case ZXDH_AE_RSP_SRQ_AXI_RSP_SIG:

			spin_lock_irqsave(&rf->qptable_lock, flags);
			if (info->qp_cq_id < dev->base_qpn) {
				spin_unlock_irqrestore(&rf->qptable_lock,
						       flags);
				pr_err("[zxdh_rdma] aeq srq axi err, qpn < base_qpn qpn:%d\n",
				       info->qp_cq_id);
				continue;
			} else if (info->qp_cq_id >= (dev->base_qpn + dev->max_qp)) {
				spin_unlock_irqrestore(&rf->qptable_lock,
						       flags);
				pr_err("[zxdh_rdma] aeq srq axi err, qpn >= (base_qpn + max_qp) qpn:%d\n",
				       info->qp_cq_id);
				continue;
			}
			iwqp = rf->qp_table[info->qp_cq_id - dev->base_qpn];
			if (!iwqp) {
				spin_unlock_irqrestore(&rf->qptable_lock,
						       flags);
				zxdh_dbg(dbg_dev,
					 "AEQ: qp_id %d is already freed\n",
					 info->qp_cq_id);
				continue;
			}
			spin_unlock_irqrestore(&rf->qptable_lock, flags);

			if (iwqp->is_srq == false) {
				pr_err("[zxdh_rdma] aeq srq axi err, qp is not bound to srq\n");
				continue;
			}
			iwsrq = iwqp->iwsrq;

			spin_lock_irqsave(&rf->srqtable_lock, flags);
			if (!iwsrq) {
				spin_unlock_irqrestore(&rf->srqtable_lock,
						       flags);
				zxdh_dbg(dbg_dev,
					 "AEQ: srq_id %d is already freed\n",
					 info->qp_cq_id);
				continue;
			}
			zxdh_srq_add_ref(&iwsrq->ibsrq);
			spin_unlock_irqrestore(&rf->srqtable_lock, flags);
			if (iwsrq->ibsrq.event_handler) {
				ibevent.device = iwsrq->ibsrq.device;
				ibevent.event = IB_EVENT_SRQ_ERR;
				ibevent.element.srq = &iwsrq->ibsrq;
				iwsrq->ibsrq.event_handler(
					&ibevent, iwsrq->ibsrq.srq_context);
			}
			zxdh_srq_rem_ref(&iwsrq->ibsrq);
			break;
		case ZXDH_AE_RSP_WQE_FLUSH:
			if (iwqp && iwqp->is_srq == true) {
				if (iwqp->ibqp.event_handler) {
					ibevent.device = iwqp->ibqp.device;
					ibevent.event = IB_EVENT_QP_LAST_WQE_REACHED;
					ibevent.element.qp = &iwqp->ibqp;
					iwqp->ibqp.event_handler(&ibevent, iwqp->ibqp.qp_context);
				}
			}
			break;
		case ZXDH_AE_REQ_RETRY_EXC_LOC_ACK_OUT_RANGE:
			// 0x8f3��������
			if (iwqp)
				zxdh_aeq_process_retry_err(iwqp);

			break;
		case ZXDH_AE_REQ_RETRY_EXC_TX_WINDOW_GET_ENTRY_ERR:
			// 0x8f5��������
			if (iwqp)
				zxdh_aeq_process_entry_err(iwqp);

			break;
		default:
			if (qp == NULL)
				break;

			if (info->ae_src == ZXDH_AE_REQUESTER) { //requestor
				zxdh_set_flush_fields_requester(qp, info);
			} else if (info->ae_src ==
				   ZXDH_AE_RESPONDER) { //responder
				zxdh_set_flush_fields_responder(qp, info);
			} else {
				pr_err("[zxdh_rdma] bad ae_src, ae_src:%d\n", info->ae_src);
				break;
			}
			if (iwqp)
				zxdh_aeq_qp_disconn(iwqp);

			break;
		}

		if (info->qp)
			zxdh_qp_rem_ref(&iwqp->ibqp);
	} while (1);

	if (aeqcnt) {
 		zxdh_report_aeq_info(dev->vhca_id, &report_info);
		zxdh_sc_repost_aeq_tail(dev, sc_aeq->aeq_ring.tail);
	}
}

/**
 * zxdh_ceq_ena_intr - set up device interrupts
 * @dev: hardware control device structure
 * @ceq_id: ceq of the interrupt to be enabled
 */
static void zxdh_ceq_ena_intr(struct zxdh_sc_dev *dev, u32 ceq_id)
{
	dev->irq_ops->zxdh_ceq_en_irq(dev, ceq_id);
}

/**
 * zxdh_aeq_ena_intr - set up device interrupts
 * @dev: hardware control device structure
 * @enable: aeq of the interrupt to be enabled
 */
void zxdh_aeq_ena_intr(struct zxdh_sc_dev *dev, bool enable)
{
	dev->irq_ops->zxdh_aeq_en_irq(dev, enable);
}
EXPORT_SYMBOL(zxdh_aeq_ena_intr);

/**
 * zxdh_aeq_poll_thread - AEQ polling thread function
 * @data: RDMA PCI function
 *
 * Polling thread for AEQ events in ext_mem mode where interrupts don't work
 * Optimized version with:
 * - Dynamic polling interval adjustment based on event rate
 * - Batch processing to limit CPU usage per poll
 * - Empty poll detection to reduce unnecessary wakeups
 */
static int zxdh_aeq_poll_thread(void *data)
{
	struct zxdh_pci_f *rf = data;
	struct zxdh_aeq *aeq = &rf->aeq;
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_sc_aeq *sc_aeq = &aeq->sc_aeq;
	u32 loop_count = 0;
	u32 empty_poll_count = 0;
	u32 current_interval = aeq->poll_interval_ms;
	u32 events_processed = 0;

	dev_info(&rf->pcidev->dev, "AEQ polling thread started (interval=%dms)\n",
		 current_interval);

	while (!kthread_should_stop()) {
		if (atomic_read(&aeq->poll_state)) {
			u32 prev_event_count = atomic_read(&aeq->aeq_event_count);

			/* Process AEQ events with batch limit */
			zxdh_process_aeq(rf, dev, sc_aeq);

			/* Calculate events processed in this iteration */
			events_processed = atomic_read(&aeq->aeq_event_count) - prev_event_count;

			/* Dynamic polling interval adjustment */
			if (events_processed == 0) {
				empty_poll_count++;

				/* After consecutive empty polls, increase interval */
				if (empty_poll_count >= ZXDH_AEQ_EMPTY_THRESHOLD) {
					current_interval = min((u32)(current_interval * 2),
							     (u32)ZXDH_AEQ_POLL_INTERVAL_MAX);
					empty_poll_count = 0;

					if (current_interval > aeq->poll_interval_ms) {
						dev_dbg(&rf->pcidev->dev,
							"AEQ: No events, increasing poll interval to %dms\n",
							current_interval);
					}
				}
			} else {
				/* Events detected, reset to default interval */
				empty_poll_count = 0;
				if (current_interval > aeq->poll_interval_ms) {
					current_interval = aeq->poll_interval_ms;
					dev_dbg(&rf->pcidev->dev,
						"AEQ: Events detected, resetting poll interval to %dms\n",
						current_interval);
				}
			}

			/* Log polling activity every 1000 loops */
			if (++loop_count % 1000 == 0) {
				u32 total_events = atomic_read(&aeq->aeq_event_count);
				dev_dbg(&rf->pcidev->dev,
					"AEQ polling: loops=%u, events=%u, interval=%dms\n",
					loop_count, total_events, current_interval);
			}
		}

		/* Wait for the dynamically adjusted polling interval */
		msleep_interruptible(current_interval);
	}

	dev_info(&rf->pcidev->dev, "AEQ polling thread stopping\n");
	return 0;
}

/**
 * zxdh_aeq_polling_start - Start AEQ polling mode
 * @rf: RDMA PCI function
 *
 * Initialize and start the polling thread for AEQ event processing
 */
int zxdh_aeq_polling_start(struct zxdh_pci_f *rf)
{
	struct zxdh_aeq *aeq = &rf->aeq;

	/* Set polling parameters */
	aeq->poll_interval_ms = rf->aeq_poll_interval_ms > 0 ?
	                       rf->aeq_poll_interval_ms : ZXDH_AEQ_POLL_INTERVAL_DEFAULT;
	atomic_set(&aeq->poll_state, 1);
	init_waitqueue_head(&aeq->poll_wait);
	atomic_set(&aeq->aeq_event_count, 0);

	dev_info(&rf->pcidev->dev, "Starting AEQ polling mode with interval %d ms\n",
	         aeq->poll_interval_ms);

	/* Create polling thread */
	aeq->poll_thread = kthread_create(zxdh_aeq_poll_thread, rf,
	                                "zxdh_aeq_poll_%s", dev_name(&rf->pcidev->dev));
	if (IS_ERR(aeq->poll_thread)) {
		dev_err(&rf->pcidev->dev, "Failed to create AEQ poll thread\n");
		return PTR_ERR(aeq->poll_thread);
	}

	/* Start the thread */
	wake_up_process(aeq->poll_thread);

	dev_info(&rf->pcidev->dev, "AEQ polling started successfully with thread %d ms interval\n",
	         aeq->poll_interval_ms);

	return 0;
}

/**
 * zxdh_aeq_polling_stop - Stop AEQ polling mode
 * @rf: RDMA PCI function
 *
 * Stop the polling thread and clean up resources
 */
void zxdh_aeq_polling_stop(struct zxdh_pci_f *rf)
{
	struct zxdh_aeq *aeq = &rf->aeq;

	/* Stop polling state */
	atomic_set(&aeq->poll_state, 0);

	/* Stop polling thread */
	if (aeq->poll_thread && !IS_ERR(aeq->poll_thread)) {
		kthread_stop(aeq->poll_thread);
		aeq->poll_thread = NULL;
	}

	dev_info(&rf->pcidev->dev, "AEQ polling stopped\n");
}

/**
 * zxdh_aeq_get_status - Get current AEQ status information
 * @rf: RDMA PCI function
 * @buf: Buffer to store status information
 * @size: Size of the buffer
 *
 * Returns formatted status string for debugging
 */
ssize_t zxdh_aeq_get_status(struct zxdh_pci_f *rf, char *buf, size_t size)
{
	struct zxdh_aeq *aeq = &rf->aeq;
	const char *mode_str;
	int len;

	switch (aeq->work_mode) {
	case ZXDH_AEQ_MODE_POLLING:
		mode_str = "polling";
		break;
	case ZXDH_AEQ_MODE_INTERRUPT:
	default:
		mode_str = "interrupt";
		break;
	}

	len = zte_snprintf_s(buf, size,
		"AEQ Status:\n"
		"  Mode: %s\n"
		"  Poll Interval: %u ms\n"
		"  Poll State: %d\n"
		"  Event Count: %u\n"
		"  Thread: %s\n",
		mode_str,
		aeq->poll_interval_ms,
		atomic_read(&aeq->poll_state),
		atomic_read(&aeq->aeq_event_count),
		aeq->poll_thread ? "running" : "stopped");

	return len > 0 ? len : 0;
}
EXPORT_SYMBOL(zxdh_aeq_get_status);

/**
 * zxdh_init_aeq_config - Initialize AEQ configuration
 * @rf: RDMA PCI function
 *
 * Initialize AEQ working mode and polling parameters
 */
int zxdh_init_aeq_config(struct zxdh_pci_f *rf)
{
	/* Apply module parameters or defaults */
	rf->aeq_poll_interval_ms = ZXDH_AEQ_POLL_INTERVAL_DEFAULT;

	/* Ensure polling interval is within valid range */
	if (rf->aeq_poll_interval_ms < ZXDH_AEQ_POLL_INTERVAL_MIN)
		rf->aeq_poll_interval_ms = ZXDH_AEQ_POLL_INTERVAL_MIN;
	if (rf->aeq_poll_interval_ms > ZXDH_AEQ_POLL_INTERVAL_MAX)
		rf->aeq_poll_interval_ms = ZXDH_AEQ_POLL_INTERVAL_MAX;

	/* When zxdh_use_ext_mem returns true, use aeq polling;
	 * otherwise use aeq interrupt.
	 */
	if (rf->use_ext_mem_flag) {
		rf->aeq.work_mode = ZXDH_AEQ_MODE_POLLING;
	} else {
		rf->aeq.work_mode = ZXDH_AEQ_MODE_INTERRUPT;
	}

	rf->aeq.irq_sta = false;

	dev_info(&rf->pcidev->dev, "AEQ config initialized: polling_interval=%d ms, use_ext_mem=%d\n",
	         rf->aeq_poll_interval_ms, rf->use_ext_mem_flag);

	return 0;
}

/**
 * zxdh_dpc - tasklet for aeq and ceq 0
 * @t: tasklet_struct ptr
 */
static void zxdh_dpc(struct tasklet_struct *t)
{
	struct zxdh_pci_f *rf = from_tasklet(rf, t, dpc_tasklet);
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_aeq *aeq = &rf->aeq;
	struct zxdh_sc_aeq *sc_aeq = &aeq->sc_aeq;

	struct zxdh_sc_dev *ext_dev = &rf->ext_sc_dev;
	struct zxdh_aeq *ext_aeq = &rf->ext_aeq;
	struct zxdh_sc_aeq *ext_sc_aeq = &ext_aeq->sc_aeq;

	zxdh_process_aeq(rf, dev, sc_aeq);
	zxdh_aeq_ena_intr(&rf->sc_dev, true);

	if(rf->use_blue_flame_flag && (!rf->ftype))
	{
		zxdh_process_aeq(rf, ext_dev, ext_sc_aeq);
		zxdh_aeq_ena_intr(&rf->ext_sc_dev, true);
	}
}

/**
 * zxdh_ceq_dpc - dpc handler for CEQ
 * @t: tasklet_struct ptr
 */
static void zxdh_ceq_dpc(struct tasklet_struct *t)
{
	struct zxdh_ceq *iwceq = from_tasklet(iwceq, t, dpc_tasklet);
	struct zxdh_pci_f *rf = iwceq->rf;

	zxdh_process_ceq(rf, iwceq);
	zxdh_ceq_ena_intr(&rf->sc_dev, iwceq->sc_ceq.ceq_id);
}

/**
 * zxdh_save_msix_info - copy msix vector information to iwarp device
 * @rf: RDMA PCI function
 *
 * Allocate iwdev msix table and copy the msix info to the table
 * Return 0 if successful, otherwise return error
 */
static int zxdh_save_msix_info(struct zxdh_pci_f *rf)
{
	struct zxdh_qvlist_info *iw_qvlist;
	struct zxdh_qv_info *iw_qvinfo;
#ifdef MSIX_DEBUG
	struct msix_entry *pmsix;
#else
	u16 entry;
#endif
	u32 size;
	u32 online_cpus_num;

	if (!rf->msix_count)
		return -EINVAL;

	size = sizeof(struct zxdh_msix_vector) * rf->msix_count;
	size += sizeof(struct zxdh_qvlist_info);
	size += sizeof(struct zxdh_qv_info) * rf->msix_count - 1;
	rf->iw_msixtbl = kzalloc(size, GFP_KERNEL);
	if (!rf->iw_msixtbl)
		return -ENOMEM;

	rf->iw_qvlist =
		(struct zxdh_qvlist_info *)(&rf->iw_msixtbl[rf->msix_count]);
	iw_qvlist = rf->iw_qvlist;
	iw_qvinfo = iw_qvlist->qv_info;
	iw_qvlist->num_vectors = rf->msix_count;
	online_cpus_num = num_online_cpus();
#ifdef MSIX_DEBUG
	pmsix = rf->msix_entries;
#else
	entry = rf->msix_entries->entry;
#endif

#ifdef MSIX_SUPPORT
	for (i = 0, ceq_idx = 0; i < rf->msix_count; i++, iw_qvinfo++) {
#ifdef MSIX_DEBUG
		rf->iw_msixtbl[i].idx = pmsix->entry;
		rf->iw_msixtbl[i].irq = pmsix->vector;
#else
		rf->iw_msixtbl[i].idx = entry + i;
		vector = pci_irq_vector(rf->pcidev, (entry + i));
		rf->iw_msixtbl[i].irq = vector;
#endif
		if (rf->msix_count <= (online_cpus_num + 1))
			rf->iw_msixtbl[i].cpu_affinity = ceq_idx;
		else
			rf->iw_msixtbl[i].cpu_affinity =
				(ceq_idx % online_cpus_num);
		if (!i) {
			iw_qvinfo->aeq_idx = 0;
			iw_qvinfo->ceq_idx = ZXDH_Q_INVALID_IDX;
		} else {
			iw_qvinfo->aeq_idx = ZXDH_Q_INVALID_IDX;
			iw_qvinfo->ceq_idx = ceq_idx++;
		}
		iw_qvinfo->itr_idx = ZXDH_IDX_NOITR;
		iw_qvinfo->v_idx = rf->iw_msixtbl[i].idx;
#ifdef MSIX_DEBUG
		pmsix++;
#endif
	}
#endif
	return 0;
}

/**
 * zxdh_aeq_handler - interrupt handler for aeq
 * @irq: Interrupt request number
 * @data: RDMA PCI function
 */
static irqreturn_t zxdh_aeq_handler(int irq, void *data)
{
	struct zxdh_pci_f *rf = data;

	tasklet_schedule(&rf->dpc_tasklet);

	return IRQ_HANDLED;
}

/**
 * zxdh_ceq_handler - interrupt handler for ceq
 * @irq: interrupt request number
 * @data: ceq pointer
 */
static irqreturn_t zxdh_ceq_handler(int irq, void *data)
{
	struct zxdh_ceq *iwceq = data;

	if (iwceq->irq != irq)
		dev_err(idev_to_dev(&iwceq->rf->sc_dev),
			"expected irq = %d received irq = %d\n", iwceq->irq,
			irq);
	tasklet_schedule(&iwceq->dpc_tasklet);

	return IRQ_HANDLED;
}

#ifdef MSIX_SUPPORT
/**
 * zxdh_destroy_irq - destroy device interrupts
 * @rf: RDMA PCI function
 * @msix_vec: msix vector to disable irq
 * @dev_id: parameter to pass to free_irq (used during irq setup)
 * @irq_type: interrupt type
 *
 * The function is called when destroying aeq/ceq
 */
static void zxdh_destroy_irq(struct zxdh_pci_f *rf, struct zxdh_msix_vector *msix_vec, void *dev_id, u8 irq_type)
{
    struct zxdh_msix_info *msix_info = NULL;
    struct zxdh_aeq *aeq;
    struct zxdh_ceq *ceq;

    if (irq_type == IRQ_TYPE_AEQ) {
        aeq = &rf->aeq;
        msix_info = &aeq->msix_info;
    } else {
        if (dev_id) {
            ceq = (struct zxdh_ceq *)dev_id; 
            msix_info = &ceq->msix_info;
        }  
    }

    if (rf->net_irq_cap == false) {
        irq_set_affinity_hint(msix_vec->irq, NULL);
        free_irq(msix_vec->irq, dev_id);
    } else {
        rf->gen_ops.zxdh_common_func(msix_info, NULL, ZXDH_FUNC_IRQ_FREE);	
    }

    if (msix_info && cpumask_available(msix_info->mask))
        free_cpumask_var(msix_info->mask);
}
#endif

/**
 * zxdh_destroy_cqp  - destroy control qp
 * @rf: RDMA PCI function
 * @free_hwcqp: 1 if hw cqp should be freed
 *
 * Issue destroy cqp request and
 * free the resources associated with the cqp
 */
static void zxdh_destroy_cqp(struct zxdh_pci_f *rf, bool free_hwcqp)
{
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_cqp *cqp = &rf->cqp;
	int status = 0;

	if (rf->cqp_cmpl_wq)
		destroy_workqueue(rf->cqp_cmpl_wq);
	status = zxdh_sc_cqp_destroy(dev->cqp, free_hwcqp);
	if (status)
		zxdh_dbg(dev, "ERR: Destroy CQP failed %d\n", status);

	zxdh_cleanup_pending_cqp_op(rf);

	if (!rf->use_ext_mem_flag)
	{
		dma_free_coherent(dev->hw->device, cqp->sq.size, cqp->sq.va,
			  cqp->sq.pa);
	}
	cqp->sq.va = NULL;
	kfree(cqp->scratch_array);
	cqp->scratch_array = NULL;
	kfree(cqp->cqp_requests);
	cqp->cqp_requests = NULL;
}

static void zxdh_destroy_ext_cqp(struct zxdh_pci_f *rf, bool free_hwcqp)
{
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_sc_dev *ext_dev = &rf->ext_sc_dev;
	struct zxdh_cqp *ext_cqp = &rf->ext_cqp;
	int status = 0;

	status = zxdh_ext_sc_cqp_destroy(ext_dev->cqp, free_hwcqp);
	if (status)
		zxdh_dbg(dev, "ERR: Destroy EXT CQP failed %d\n", status);

	dma_free_coherent(ext_dev->hw->device, ext_cqp->sq.size, ext_cqp->sq.va,
			ext_cqp->sq.pa);
	ext_cqp->sq.va = NULL;
	kfree(ext_cqp->scratch_array);
	ext_cqp->scratch_array = NULL;
	kfree(ext_cqp->cqp_requests);
	ext_cqp->cqp_requests = NULL;
}

static void zxdh_destroy_virt_aeq(struct zxdh_pci_f *rf)
{
	struct zxdh_aeq *aeq = &rf->aeq;
	u32 pg_cnt = DIV_ROUND_UP(aeq->mem.size, PAGE_SIZE);
	dma_addr_t *pg_arr = (dma_addr_t *)aeq->palloc.level1.addr;

	zxdh_unmap_vm_page_list(&rf->hw, pg_arr, pg_cnt);
	zxdh_free_pble(rf->pble_rsrc, &aeq->palloc);
	vfree(aeq->mem.va);
}

static void zxdh_destroy_virt_ext_aeq(struct zxdh_pci_f *rf)
{
	struct zxdh_aeq *ext_aeq = &rf->ext_aeq;
	u32 pg_cnt = DIV_ROUND_UP(ext_aeq->mem.size, PAGE_SIZE);
	dma_addr_t *pg_arr = (dma_addr_t *)ext_aeq->palloc.level1.addr;

	zxdh_unmap_vm_page_list(&rf->hw, pg_arr, pg_cnt);
	zxdh_free_pble(rf->pble_rsrc, &ext_aeq->palloc);
	vfree(ext_aeq->mem.va);
}

static int zxdh_destroy_aeq_reg_cmd(struct zxdh_sc_dev *dev, struct zxdh_sc_aeq *aeq)
{
	struct zxdh_sc_cqp *cqp;
	__le64 *wqe;
	u64 hdr;
	u32 tail = 0, val = 0;
	int ret_code = 0;
	u64 scratch = 0;

	cqp = dev->cqp;
	wqe = zxdh_sc_cqp_get_next_send_wqe(cqp, scratch);
	if (!wqe)
		return -ENOSPC;

	hdr = FIELD_PREP(ZXDH_AEQC_INTR_IDX, aeq->msix_idx) |
	      FIELD_PREP(ZXDH_AEQC_AEQ_HEAD, 0) |
	      FIELD_PREP(ZXDH_AEQC_LEAF_PBL_SIZE, aeq->pbl_chunk_size) |
	      FIELD_PREP(ZXDH_AEQC_VIRTUALLY_MAPPED, aeq->virtual_map) |
	      FIELD_PREP(ZXDH_AEQC_AEQ_SIZE, aeq->elem_cnt) |
	      FIELD_PREP(ZXDH_AEQC_AEQ_STATE, 1);
	dma_wmb();
	set_64bit_val(wqe, 8, hdr);

	set_64bit_val(wqe, 16,
		      aeq->virtual_map ? aeq->first_pm_pbl_idx :
					       aeq->aeq_elem_pa);

	hdr = FIELD_PREP(ZXDH_CQPSQ_OPCODE, ZXDH_CQP_OP_DESTROY_AEQ) |
	      FIELD_PREP(ZXDH_CQPSQ_WQEVALID, cqp->polarity);
	dma_wmb(); /* make sure WQE is written before valid bit is set */

	set_64bit_val(wqe, 0, hdr);

	print_hex_dump_debug("WQE: AEQ_DESTROY WQE", DUMP_PREFIX_OFFSET, 16, 8,
			     wqe, ZXDH_CQP_WQE_SIZE * 8, false);

	val = readl(dev->hw->hw_addr + C_RDMA_CQP_TAIL(dev->ep_id));
	tail = (u32)FIELD_GET(ZXDH_CQPTAIL_WQTAIL, val);

	zxdh_sc_cqp_post_sq(cqp);

	ret_code = zxdh_cqp_poll_registers(cqp, tail,
					   dev->hw_attrs.max_done_count);

	if (ret_code)
		return ret_code;

	return 0;	
}

static int zxdh_destroy_ext_aeq_reg_cmd(struct zxdh_sc_dev *ext_dev, struct zxdh_sc_aeq *ext_aeq)
{
	struct zxdh_sc_cqp *ext_cqp;
	__le64 *wqe;
	u64 hdr;
	u32 tail = 0, val = 0;
	int ret_code = 0;
	u64 scratch = 0;

	ext_cqp = ext_dev->cqp;
	wqe = zxdh_ext_sc_cqp_get_next_send_wqe(ext_cqp, scratch);
	if (!wqe)
		return -ENOSPC;

	hdr = FIELD_PREP(ZXDH_AEQC_INTR_IDX, ext_aeq->msix_idx) |
	      FIELD_PREP(ZXDH_AEQC_AEQ_HEAD, 0) |
	      FIELD_PREP(ZXDH_AEQC_LEAF_PBL_SIZE, ext_aeq->pbl_chunk_size) |
	      FIELD_PREP(ZXDH_AEQC_VIRTUALLY_MAPPED, ext_aeq->virtual_map) |
	      FIELD_PREP(ZXDH_AEQC_AEQ_SIZE, ext_aeq->elem_cnt) |
	      FIELD_PREP(ZXDH_AEQC_AEQ_STATE, 1);
	dma_wmb();
	set_64bit_val(wqe, 8, hdr);

	set_64bit_val(wqe, 16,
		      ext_aeq->virtual_map ? ext_aeq->first_pm_pbl_idx :
					       ext_aeq->aeq_elem_pa);

	hdr = FIELD_PREP(ZXDH_CQPSQ_OPCODE, ZXDH_CQP_OP_DESTROY_AEQ) |
	      FIELD_PREP(ZXDH_CQPSQ_WQEVALID, ext_cqp->polarity);
	dma_wmb(); /* make sure WQE is written before valid bit is set */

	set_64bit_val(wqe, 0, hdr);

	print_hex_dump_debug("WQE: EXT AEQ_DESTROY WQE", DUMP_PREFIX_OFFSET, 16, 8,
			     wqe, ZXDH_CQP_WQE_SIZE * 8, false);

	val = readl(ext_dev->hw->ext_hw_addr + C_RDMA_EXT_CQP_TAIL(ext_dev->ep_id));
	tail = (u32)FIELD_GET(ZXDH_CQPTAIL_WQTAIL, val);

	zxdh_sc_cqp_post_sq(ext_cqp);

	ret_code = zxdh_ext_cqp_poll_registers(ext_cqp, tail,
					   ext_dev->hw_attrs.max_done_count);

	if (ret_code)
		return ret_code;

	return 0;	
}

/**
 * zxdh_destroy_aeq_reg - destroy aeq
 * @rf: RDMA PCI function
 *
 * Issue a destroy aeq request and
 * free the resources associated with the aeq
 * The function is called during driver unload
 */
static void zxdh_destroy_aeq_reg(struct zxdh_pci_f *rf)
{
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_aeq *aeq = &rf->aeq;
	int status = -EBUSY;
#ifdef MSIX_SUPPORT
    if (aeq->irq_sta == true) {
        aeq->irq_sta = false;
        zxdh_destroy_irq(rf, rf->iw_msixtbl, rf, IRQ_TYPE_AEQ);
    }
#endif
	aeq->sc_aeq.size = 0;
	status = zxdh_destroy_aeq_reg_cmd(dev, &aeq->sc_aeq);
	if (status)
		zxdh_dbg(dev, "ERR: Destroy AEQ failed %d\n", status);

	if (aeq->virtual_map)
		zxdh_destroy_virt_aeq(rf);
	else {
		if (!rf->use_ext_mem_flag)
		{
			dma_free_coherent(dev->hw->device, aeq->mem.size, aeq->mem.va,
					aeq->mem.pa);
		}
		aeq->mem.va = NULL;
	}
}

/**
 * zxdh_destroy_ext_aeq_reg - destroy ext aeq
 * @rf: RDMA PCI function
 */
static void zxdh_destroy_ext_aeq_reg(struct zxdh_pci_f *rf)
{
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_sc_dev *ext_dev = &rf->ext_sc_dev;
	struct zxdh_aeq *ext_aeq = &rf->ext_aeq;
	int status = -EBUSY;

	ext_aeq->sc_aeq.size = 0;
	status = zxdh_destroy_ext_aeq_reg_cmd(ext_dev, &ext_aeq->sc_aeq);
	if (status)
		zxdh_dbg(dev, "ERR: Destroy EXT AEQ failed %d\n", status);

	if (ext_aeq->virtual_map){
		zxdh_destroy_virt_ext_aeq(rf);
	} else {
		dma_free_coherent(ext_dev->hw->device, ext_aeq->mem.size, ext_aeq->mem.va,
				ext_aeq->mem.pa);
		ext_aeq->mem.va = NULL;
	}
}

/**
 * zxdh_destroy_aeq - destroy aeq
 * @rf: RDMA PCI function
 *
 * Issue a destroy aeq request and
 * free the resources associated with the aeq
 * The function is called during driver unload
 */
static void zxdh_destroy_aeq(struct zxdh_pci_f *rf)
{
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_aeq *aeq = &rf->aeq;
	int status = -EBUSY;
#ifdef MSIX_SUPPORT
	if (aeq->irq_sta == true) {
		aeq->irq_sta = false;
		zxdh_destroy_irq(rf, rf->iw_msixtbl, rf, IRQ_TYPE_AEQ);
	}
#endif
	aeq->sc_aeq.size = 0;
	status = zxdh_cqp_aeq_cmd(dev, &aeq->sc_aeq, ZXDH_OP_AEQ_DESTROY);
	if (status)
		zxdh_dbg(dev, "ERR: Destroy AEQ failed %d\n", status);

	if (aeq->virtual_map)
		zxdh_destroy_virt_aeq(rf);
	else {
		if (!rf->use_ext_mem_flag) {
			dma_free_coherent(dev->hw->device, aeq->mem.size, aeq->mem.va,
					aeq->mem.pa);
		}
		aeq->mem.va = NULL;
	}
}

/**
 * zxdh_destroy_ceq - destroy ceq
 * @rf: RDMA PCI function
 * @iwceq: ceq to be destroyed
 *
 * Issue a destroy ceq request and
 * free the resources associated with the ceq
 */
static void zxdh_destroy_ceq(struct zxdh_pci_f *rf, struct zxdh_ceq *iwceq)
{
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	int status;
	unsigned long flags;

	if (rf->reset)
		goto exit;

	status = zxdh_sc_ceq_destroy(&iwceq->sc_ceq, 0, 1);
	if (status) {
		zxdh_dbg(dev, "ERR: CEQ destroy command failed %d\n", status);
		goto exit;
	}

	status = zxdh_sc_cceq_destroy_done(&iwceq->sc_ceq);
	if (status)
		zxdh_dbg(dev, "ERR: CEQ destroy completion failed %d\n",
			 status);
exit:
	spin_lock_irqsave(&iwceq->ce_lock, flags);
	iwceq->sc_ceq.valid_ceq = false;
	spin_unlock_irqrestore(&iwceq->ce_lock, flags);
	if (!rf->use_ext_mem_flag)
	{
		dma_free_coherent(dev->hw->device, iwceq->mem.size, iwceq->mem.va,
			  iwceq->mem.pa);
	}
	iwceq->mem.va = NULL;
}

/**
 * zxdh_del_ceq_0 - destroy ceq 0
 * @rf: RDMA PCI function
 *
 * Disable the ceq 0 interrupt and destroy the ceq 0
 */
static void zxdh_del_ceq_0(struct zxdh_pci_f *rf)
{
	struct zxdh_ceq *iwceq = rf->ceqlist;
	struct zxdh_msix_vector *msix_vec;

	msix_vec = &rf->iw_msixtbl[1];

#ifdef MSIX_SUPPORT
	if (iwceq->irq_sta == true) {
		iwceq->irq_sta = false;
		zxdh_destroy_irq(rf, msix_vec, iwceq, IRQ_TYPE_CEQ);
	}
#endif
	zxdh_destroy_ceq(rf, iwceq);
	rf->sc_dev.ceq_valid = false;
	rf->ceqs_count = 0;
}

/**
 * zxdh_del_ceqs - destroy all ceq's except CEQ 0
 * @rf: RDMA PCI function
 *
 * Go through all of the device ceq's, except 0, and for each
 * ceq disable the ceq interrupt and destroy the ceq
 */
static void zxdh_del_ceqs(struct zxdh_pci_f *rf)
{
	struct zxdh_ceq *iwceq = &rf->ceqlist[1];

	struct zxdh_msix_vector *msix_vec;
	u32 i = 0;
	unsigned long flags;

	msix_vec = &rf->iw_msixtbl[2];
	for (i = 1; i < rf->ceqs_count; i++, msix_vec++, iwceq++) {
#ifdef MSIX_SUPPORT
		if (iwceq->irq_sta == true) {
			iwceq->irq_sta = false;
			zxdh_destroy_irq(rf, msix_vec, iwceq, IRQ_TYPE_CEQ);
		}
#endif
		zxdh_cqp_ceq_cmd(&rf->sc_dev, &iwceq->sc_ceq,
				 ZXDH_OP_CEQ_DESTROY);
		spin_lock_irqsave(&iwceq->ce_lock, flags);
		iwceq->sc_ceq.valid_ceq = false;
		spin_unlock_irqrestore(&iwceq->ce_lock, flags);
		if (!rf->use_ext_mem_flag) {
			dma_free_coherent(rf->sc_dev.hw->device, iwceq->mem.size,
				  iwceq->mem.va, iwceq->mem.pa);
		}
		iwceq->mem.va = NULL;
	}

	rf->ceqs_count = 1;
}

/**
 * zxdh_destroy_ccq - destroy control cq
 * @rf: RDMA PCI function
 *
 * Issue destroy ccq request and
 * free the resources associated with the ccq
 */
static void zxdh_destroy_ccq(struct zxdh_pci_f *rf)
{
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_ccq *ccq = &rf->ccq;
	int status = 0;

	if (!rf->reset)
		status = zxdh_sc_ccq_destroy(dev->ccq, 0, true);
	if (status)
		zxdh_dbg(dev, "ERR: CCQ destroy failed %d\n", status);
	if (!rf->use_ext_mem_flag) {
		dma_free_coherent(dev->hw->device, ccq->mem_cq.size, ccq->mem_cq.va,
			  ccq->mem_cq.pa);
		dma_free_coherent(dev->hw->device, ccq->shadow_area.size,
			  ccq->shadow_area.va, ccq->shadow_area.pa);
	}
	ccq->mem_cq.va = NULL;
	ccq->shadow_area.va = NULL;
	zxdh_free_rsrc(rf, rf->allocated_cqs,
		       ccq->sc_cq.cq_uk.cq_id - dev->base_cqn);
}

void zxdh_del_data_cap_objects(struct zxdh_sc_dev *dev)
{
	unsigned int i;
	struct zxdh_hmc_sd_entry *sd_entry;
	struct zxdh_dma_mem *mem = NULL;

	if (!dev->data_cap_sd.entry)
		return;
	for (i = 0; i < dev->data_cap_sd.sd_cnt; i++) {
		if (!dev->data_cap_sd.entry[i].valid)
			continue;

		sd_entry = &dev->data_cap_sd.entry[i];
		mem = &sd_entry->u.bp.addr;
		if (!mem || !mem->va)
			pr_err("[zxdh_rdma] HMC: error cqp sd mem\n");
		else {
			dma_free_coherent(dev->hw->device, mem->size, mem->va,
					  mem->pa);
			mem->va = NULL;
		}
	}
	kfree(dev->data_cap_sd.entry);
	dev->data_cap_sd.entry = NULL;
}

void zxdh_del_hmc_objects(struct zxdh_sc_dev *dev,
			  struct zxdh_hmc_info *hmc_info)
{
	unsigned int i, sd_idx;
	u32 del_sd_cnt = 0;
	struct zxdh_hmc_sd_entry *sd_entry;
	struct zxdh_dma_mem *mem = NULL;
	struct zxdh_dma_mem *mem_harware = NULL;

	for (i = 0; i < hmc_info->hmc_entry_total; i++) {
		if (!hmc_info->sd_table.sd_entry[i].valid)
			continue;
		zxdh_prep_remove_sd_bp(hmc_info, i);
		hmc_info->sd_indexes[del_sd_cnt] = (u16)i;
		del_sd_cnt++;
	}

	for (i = 0; i < del_sd_cnt; i++) {
		sd_idx = hmc_info->sd_indexes[i];
		sd_entry = &hmc_info->sd_table.sd_entry[sd_idx];
		mem = &sd_entry->u.bp.addr;
		if (!mem || !mem->va)
			pr_err("[zxdh_rdma] HMC: error cqp sd mem\n");
		else {
			dma_free_coherent(dev->hw->device, mem->size, mem->va,
					  mem->pa);
			mem->va = NULL;
		}

		mem_harware = &sd_entry->u.bp.addr_hardware;
		if (mem_harware && mem_harware->va) {
			dma_free_coherent(dev->hw->device, mem_harware->size,
					  mem_harware->va, mem_harware->pa);
			mem_harware->va = NULL;
		}
	}
}

/**
 * zxdh_create_hmc_objs - create all hmc objects for the device
 * @rf: RDMA PCI function
 * @privileged: permission to create HMC objects
 *
 * Create the device hmc objects and allocate hmc pages
 * Return 0 if successful, otherwise clean up and return error
 */
static int zxdh_create_hmc_objs(struct zxdh_pci_f *rf, bool privileged)
{
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_hmc_create_obj_info info = {};
	int i, status = 0;

	info.hmc_info = dev->hmc_info;
	info.privileged = privileged;
	info.add_sd_cnt = 0;

	for (i = 0; i < IW_HMC_OBJ_TYPE_NUM; i++) {
		if (dev->hmc_info->hmc_obj[iw_hmc_obj_types[i]].cnt) {
			info.rsrc_type = iw_hmc_obj_types[i];
			info.count = dev->hmc_info->hmc_obj[info.rsrc_type].cnt;
			status = zxdh_sc_create_hmc_obj(dev, &info);
			if (status) {
				zxdh_del_hmc_objects(&rf->sc_dev,
						     rf->sc_dev.hmc_info);
                ibdev_err(&rf->iwdev->ibdev, "ERR: create obj type %d status = %d\n", iw_hmc_obj_types[i], status);
				break;
			}
		}
	}

	return status;
}

static int zxdh_create_hmcobjs_dpuddr(struct zxdh_pci_f *rf)
{
	u32 sd_lmt, hmc_entry_total = 0, j = 0, k = 0, mem_size = 0, cnt = 0;
	u64 fpm_limit = 0;
	struct zxdh_hmc_info *hmc_info;
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_sc_dev *ext_dev = &rf->ext_sc_dev;
	struct zxdh_virt_mem virt_mem = {};
	struct zxdh_hmc_obj_info *obj_info;

	hmc_info = dev->hmc_info;

	zxdh_hmc_dpu_capability(dev);
	for (k = 0; k < ZXDH_HMC_IW_MAX; k++) {
		zxdh_sc_write_hmc_register(dev, hmc_info->hmc_obj, k,
					   dev->vhca_id);
		if (rf->use_blue_flame_flag && (!rf->ftype)) {
			zxdh_ext_sc_write_hmc_register(dev, hmc_info->hmc_obj, k,
						ext_dev->vhca_id);
		}
	}

	obj_info = hmc_info->hmc_obj;
	for (k = ZXDH_HMC_IW_PBLE; k < ZXDH_HMC_IW_MAX; k++) {
		cnt = obj_info[k].cnt;

		fpm_limit = obj_info[k].size * cnt;

		if (fpm_limit == 0)
			continue;

		if (k == ZXDH_HMC_IW_PBLE)
			hmc_info->hmc_first_entry_pble = hmc_entry_total;

		if (k == ZXDH_HMC_IW_PBLE_MR)
			hmc_info->hmc_first_entry_pble_mr = hmc_entry_total;

		sd_lmt = (u32)((fpm_limit - 1) / ZXDH_HMC_DIRECT_BP_SIZE);
		sd_lmt += 1;

		if (sd_lmt == 1) {
			hmc_entry_total++;
		} else {
			for (j = 0; j < sd_lmt - 1; j++)
				hmc_entry_total++;

			if (fpm_limit % ZXDH_HMC_DIRECT_BP_SIZE)
				hmc_entry_total++;
		}
	}

	mem_size = sizeof(struct zxdh_hmc_sd_entry) * hmc_entry_total;
	virt_mem.size = mem_size;
	virt_mem.va = kzalloc(virt_mem.size, GFP_KERNEL);
	if (!virt_mem.va) {
		zxdh_dbg(
			dev,
			"HMC: failed to allocate memory for sd_entry buffer\n");
		return -ENOMEM;
	}
	hmc_info->sd_table.sd_entry = virt_mem.va;
	hmc_info->hmc_entry_total = hmc_entry_total;

	return 0;
}

/**
 * zxdh_create_cqp - create control qp
 * @rf: RDMA PCI function
 *
 * Return 0, if the cqp and all the resources associated with it
 * are successfully created, otherwise return error
 */
static int zxdh_create_cqp(struct zxdh_pci_f *rf)
{
	u32 sqsize = ZXDH_CQP_SW_SQSIZE_2048;
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_cqp_init_info cqp_init_info = {};
	struct zxdh_cqp *cqp =
		&rf->cqp; // this struct will be transferred to CQE.
	u16 maj_err, min_err;
	int i, status;

	cqp->cqp_requests =
		kcalloc(sqsize, sizeof(*cqp->cqp_requests), GFP_KERNEL);
	if (!cqp->cqp_requests)
		return -ENOMEM;

	cqp->scratch_array =
		kcalloc(sqsize, sizeof(*cqp->scratch_array), GFP_KERNEL);
	if (!cqp->scratch_array) {
		status = -ENOMEM;
		goto err_scratch;
	}

	dev->cqp = &cqp->sc_cqp;
	dev->cqp->dev = dev;
	cqp->sq.size = ALIGN(sizeof(struct zxdh_cqp_sq_wqe) * sqsize,
			     ZXDH_CQP_ALIGNMENT);
	if (rf->use_ext_mem_flag)
	{
		cqp->sq.va = (void *)zxdh_get_ext_mem(rf, cqp->sq.size, &cqp->sq.pa);
	}
	else
	{
		cqp->sq.va = dma_alloc_coherent(dev->hw->device, cqp->sq.size,
						&cqp->sq.pa, GFP_KERNEL);
	}
	if (!cqp->sq.va) {
		status = -ENOMEM;
		goto err_sq;
	}

	// populate the cqp init info
	cqp_init_info.dev = dev;
	cqp_init_info.sq_size = sqsize;
	cqp_init_info.sq = cqp->sq.va;
	cqp_init_info.sq_pa = cqp->sq.pa;
	if (dev->privileged) {
		cqp_init_info.hmc_profile = rf->rsrc_profile;
		cqp_init_info.ena_vf_count = rf->max_rdma_vfs;
	}
	cqp_init_info.scratch_array = cqp->scratch_array;
	cqp_init_info.protocol_used = rf->protocol_used;
	memcpy(&cqp_init_info.dcqcn_params, &rf->dcqcn_params,
	       sizeof(cqp_init_info.dcqcn_params));

	cqp_init_info.hw_maj_ver = ZXDH_CQPHC_HW_MAJVER_GEN_2;
	status = zxdh_sc_cqp_init(dev->cqp, &cqp_init_info);
	if (status) {
		pr_err("[zxdh_rdma] ERR: cqp init status %d\n", status);
		goto err_ctx;
	}

	spin_lock_init(&cqp->req_lock);
	spin_lock_init(&cqp->compl_lock);

	status = zxdh_sc_cqp_create(dev->cqp, &maj_err, &min_err);
	if (status) {
		zxdh_dbg(
			dev,
			"ERR: cqp create failed - status %d maj_err %d min_err %d\n",
			status, maj_err, min_err);
		goto err_create;
	}

	INIT_LIST_HEAD(&cqp->cqp_avail_reqs);
	INIT_LIST_HEAD(&cqp->cqp_pending_reqs);

	/* init the waitqueue of the cqp_requests and add them to the list */
	for (i = 0; i < sqsize; i++) {
		init_waitqueue_head(&cqp->cqp_requests[i].waitq);
		list_add_tail(&cqp->cqp_requests[i].list, &cqp->cqp_avail_reqs);
	}
	init_waitqueue_head(&cqp->remove_wq);
	return 0;

err_create:
err_ctx:
	if (rf->use_ext_mem_flag) {
		zxdh_free_ext_mem(rf, cqp->sq.va, cqp->sq.size);
	} else {
		dma_free_coherent(dev->hw->device, cqp->sq.size, cqp->sq.va,
			  cqp->sq.pa);
	}
	cqp->sq.va = NULL;
err_sq:
	kfree(cqp->scratch_array);
	cqp->scratch_array = NULL;
err_scratch:
	kfree(cqp->cqp_requests);
	cqp->cqp_requests = NULL;

	return status;
}

/**
 * zxdh_create_ext_cqp - create extern control qp
 * @rf: RDMA PCI function
 *
 * Return 0, if the cqp and all the resources associated with it
 * are successfully created, otherwise return error
 */
static int zxdh_create_ext_cqp(struct zxdh_pci_f *rf)
{
	u32 sqsize = ZXDH_CQP_SW_SQSIZE_2048;
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_sc_dev *ext_dev = &rf->ext_sc_dev;
	struct zxdh_cqp_init_info cqp_init_info = {};
	struct zxdh_cqp *ext_cqp =
		&rf->ext_cqp; // this struct will be transferred to CQE.
	u16 maj_err, min_err;
	int status;

	ext_cqp->cqp_requests =
		kcalloc(sqsize, sizeof(*ext_cqp->cqp_requests), GFP_KERNEL);
	if (!ext_cqp->cqp_requests)
		return -ENOMEM;

	ext_cqp->scratch_array =
		kcalloc(sqsize, sizeof(*ext_cqp->scratch_array), GFP_KERNEL);
	if (!ext_cqp->scratch_array) {
		status = -ENOMEM;
		goto err_scratch;
	}

	ext_dev->cqp = &ext_cqp->sc_cqp;
	ext_dev->cqp->dev = ext_dev;
	ext_cqp->sq.size = ALIGN(sizeof(struct zxdh_cqp_sq_wqe) * sqsize,
			     ZXDH_CQP_ALIGNMENT);
	ext_cqp->sq.va = dma_alloc_coherent(ext_dev->hw->device, ext_cqp->sq.size,
					&ext_cqp->sq.pa, GFP_KERNEL);
	if (!ext_cqp->sq.va) {
		status = -ENOMEM;
		goto err_sq;
	}

	// populate the ext_cqp init info
	cqp_init_info.dev = ext_dev;
	cqp_init_info.sq_size = sqsize;
	cqp_init_info.sq = ext_cqp->sq.va;
	cqp_init_info.sq_pa = ext_cqp->sq.pa;
	if (ext_dev->privileged) {
		cqp_init_info.hmc_profile = rf->rsrc_profile;
		cqp_init_info.ena_vf_count = rf->max_rdma_vfs;
	}
	cqp_init_info.scratch_array = ext_cqp->scratch_array;
	cqp_init_info.protocol_used = rf->protocol_used;
	memcpy(&cqp_init_info.dcqcn_params, &rf->dcqcn_params,
	       sizeof(cqp_init_info.dcqcn_params));

	cqp_init_info.hw_maj_ver = ZXDH_CQPHC_HW_MAJVER_GEN_2;
	status = zxdh_ext_sc_cqp_init(ext_dev->cqp, &cqp_init_info);
	if (status) {
		pr_err("[zxdh_rdma] ERR: ext_cqp init status %d\n", status);
		goto err_ctx;
	}

	status = zxdh_ext_sc_cqp_create(ext_dev->cqp, &maj_err, &min_err);
	if (status) {
		zxdh_dbg(
			dev,
			"ERR: ext_cqp create failed - status %d maj_err %d min_err %d\n",
			status, maj_err, min_err);
		goto err_create;
	}

	return 0;

err_create:
err_ctx:
	dma_free_coherent(ext_dev->hw->device, ext_cqp->sq.size, ext_cqp->sq.va,
			ext_cqp->sq.pa);
	ext_cqp->sq.va = NULL;
err_sq:
	kfree(ext_cqp->scratch_array);
	ext_cqp->scratch_array = NULL;
err_scratch:
	kfree(ext_cqp->cqp_requests);
	ext_cqp->cqp_requests = NULL;

	return status;
}

/**
 * zxdh_create_ccq - create control cq
 * @rf: RDMA PCI function
 *
 * Return 0, if the ccq and the resources associated with it
 * are successfully created, otherwise return error
 */
static int zxdh_create_ccq(struct zxdh_pci_f *rf)
{
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_ccq_init_info info = {};
	struct zxdh_ccq *ccq = &rf->ccq;
	u32 cq_num = 0;
	int status;

	status = zxdh_alloc_rsrc(
		rf, rf->allocated_cqs, rf->max_cq, &cq_num,
		&rf->next_cq); /* cq_num is the allocated cq_id. */
	if (status)
		return status;
	cq_num += dev->base_cqn;
	info.cq_num = cq_num;
	dev->ccq = &ccq->sc_cq;
	dev->ccq->dev = dev;
	info.dev = dev;
	ccq->shadow_area.size = sizeof(struct zxdh_cq_shadow_area);
	ccq->mem_cq.size = ALIGN(sizeof(struct zxdh_cqe) * IW_CCQ_SIZE,
				 ZXDH_CQ0_ALIGNMENT);

	if (rf->use_ext_mem_flag)
	{
		ccq->mem_cq.va = (void *)zxdh_get_ext_mem(rf, ccq->mem_cq.size, &ccq->mem_cq.pa);
		if (!ccq->mem_cq.va)
		{
			pr_err("[zxdh_rdma] Failed to get ext_mem for ccq\n");
			zxdh_free_rsrc(rf, rf->allocated_cqs, cq_num - dev->base_cqn);
			return -ENOMEM;
		}

		ccq->shadow_area.va = (void *)zxdh_get_ext_mem(rf, ccq->shadow_area.size, &ccq->shadow_area.pa);
		if (!ccq->shadow_area.va)
		{
			pr_err("[zxdh_rdma] Failed to get ext_mem for ccq_shadow\n");
			// 释放已分配的 mem_cq
			zxdh_free_ext_mem(rf, ccq->mem_cq.va, ccq->mem_cq.size);
			ccq->mem_cq.va = NULL;
			ccq->mem_cq.pa = 0;
			zxdh_free_rsrc(rf, rf->allocated_cqs, cq_num - dev->base_cqn);
			return -ENOMEM;
		}

	}
	else
	{
		ccq->mem_cq.va = dma_alloc_coherent(dev->hw->device, ccq->mem_cq.size,
							&ccq->mem_cq.pa, GFP_KERNEL);
		
		if (!ccq->mem_cq.va)
		{
			zxdh_free_rsrc(rf, rf->allocated_cqs, cq_num - dev->base_cqn);
			return -ENOMEM;
		}

		ccq->shadow_area.va =
			dma_alloc_coherent(dev->hw->device, ccq->shadow_area.size,
					&ccq->shadow_area.pa, GFP_KERNEL);

		if (!ccq->shadow_area.va) {
			dma_free_coherent(dev->hw->device, ccq->mem_cq.size,
					ccq->mem_cq.va, ccq->mem_cq.pa);
			ccq->mem_cq.va = NULL;
			zxdh_free_rsrc(rf, rf->allocated_cqs, cq_num - dev->base_cqn);
			return -ENOMEM;
		}
	}

	ccq->sc_cq.back_cq = ccq;
	/* populate the ccq init info */
	info.cq_base = ccq->mem_cq.va;
	info.cq_pa = ccq->mem_cq.pa;
	info.num_elem = IW_CCQ_SIZE;
	info.shadow_area = ccq->shadow_area.va;
	info.shadow_area_pa = ccq->shadow_area.pa;
	info.ceqe_mask = false;
	info.ceq_id_valid = true;
	info.ceq_id = dev->base_ceqn;
	info.ceq_index = 0;
	info.shadow_read_threshold = 16;
	info.cqe_size = ZXDH_CQE_SIZE_64;
	info.cq_max = 0;
	info.cq_period = 0;
	info.scqe_break_moderation_en = false;
	info.cq_st = 0;
	info.is_in_list_cnt = 0;

	status = zxdh_sc_ccq_init(dev->ccq, &info);
	if (status)
		goto exit;

	status = zxdh_sc_ccq_create(dev->ccq, 0, true);
exit:
	if (status) {
		if (!rf->use_ext_mem_flag)
		{
			dma_free_coherent(dev->hw->device, ccq->mem_cq.size,
				  ccq->mem_cq.va, ccq->mem_cq.pa);
			dma_free_coherent(dev->hw->device, ccq->shadow_area.size,
					ccq->shadow_area.va, ccq->shadow_area.pa);
		}
		ccq->mem_cq.va = NULL;
		ccq->shadow_area.va = NULL;
		zxdh_free_rsrc(rf, rf->allocated_cqs, cq_num - dev->base_cqn);
	}

	return status;
}

/**
 * zxdh_cfg_ceq_vector - set up the msix interrupt vector for
 * ceq
 * @rf: RDMA PCI function
 * @iwceq: ceq associated with the vector
 * @ceq_id: the id number of the iwceq
 * @msix_vec: interrupt vector information
 *
 * Allocate interrupt resources and enable irq handling
 * Return 0 if successful, otherwise return error
 */
static int zxdh_cfg_ceq_vector(struct zxdh_pci_f *rf, struct zxdh_ceq *iwceq,
			       u32 ceq_id, struct zxdh_msix_vector *msix_vec)
{
#ifndef MSIX_SUPPORT
	return 0;
#endif
	int status;
	struct zxdh_msix_info *msix_info;

	msix_info = &iwceq->msix_info;
	if (!alloc_cpumask_var(&msix_info->mask, GFP_KERNEL)) {
        pr_err("[zxdh_rdma] [%s] alloc cpumask var failed!\n",__func__);
        return -ENOMEM;
	}
	tasklet_setup(&iwceq->dpc_tasklet, zxdh_ceq_dpc);
	cpumask_clear(&msix_vec->mask);
	cpumask_set_cpu(msix_vec->cpu_affinity, &msix_vec->mask);
	status = snprintf(msix_info->name, ZXDH_MAX_IRQ_NAME, "dinghai10e_ceq%d@pci:%s", msix_vec->idx, pci_name(rf->pcidev));
	if (status < 0) {
        pr_err("[zxdh_rdma] ERR: ceq irq name config fail\n");
        free_cpumask_var(msix_info->mask);
        return status;
	}
	if (rf->net_irq_cap == false) {
        status = request_irq(msix_vec->irq, zxdh_ceq_handler, 0, msix_info->name, iwceq);
        irq_set_affinity_hint(msix_vec->irq, &msix_vec->mask);
	} else {
        msix_info->handler = zxdh_ceq_handler;
        msix_info->dh_dev = rf->dh_dev;
        msix_info->irq = msix_vec->irq;
        msix_info->flags = 0;
        msix_info->data = iwceq;
        cpumask_copy(msix_info->mask, &msix_vec->mask);
        status = rf->gen_ops.zxdh_common_func(msix_info, NULL, ZXDH_FUNC_IRQ_REQUEST);
	}
	if (status) {
        pr_err("[zxdh_rdma] ERR: ceq irq config fail\n");
        free_cpumask_var(msix_info->mask);
        return status;
	}
	iwceq->irq = msix_vec->irq;
	iwceq->msix_idx = msix_vec->idx;
	iwceq->irq_sta = true;
	msix_vec->ceq_id = ceq_id;
	return 0;
}

/**
 * zxdh_cfg_aeq_vector - set up the msix vector for aeq
 * @rf: RDMA PCI function
 *
 * Allocate interrupt resources and enable irq handling
 * Return 0 if successful, otherwise return error
 */
static int zxdh_cfg_aeq_vector(struct zxdh_pci_f *rf)
{
#ifndef MSIX_SUPPORT
	return 0;
#endif
	struct zxdh_msix_vector *msix_vec = rf->iw_msixtbl;
	int ret = 0;
	struct zxdh_msix_info *msix_info;
	u32 num_cpu = 0;
	u32 cpu_loop = 0;

	msix_info = &rf->aeq.msix_info;
	if (!alloc_cpumask_var(&msix_info->mask, GFP_KERNEL)) {
        pr_err("[zxdh_rdma] [%s] alloc cpumask var failed!\n",__func__);
        return -ENOMEM;
	}
	tasklet_setup(&rf->dpc_tasklet, zxdh_dpc);
	ret = snprintf(msix_info->name, ZXDH_MAX_IRQ_NAME, "dinghai10e_aeq%d@pci:%s", msix_vec->idx, pci_name(rf->pcidev));
	if (ret < 0) {
        pr_err("[zxdh_rdma] ERR: aeq irq name config fail\n");
        free_cpumask_var(msix_info->mask);
        return ret;
	}
	if (rf->net_irq_cap == false) {
	    ret = request_irq(msix_vec->irq, zxdh_aeq_handler, 0, msix_info->name, rf);
	} else {
        msix_info->handler = zxdh_aeq_handler;
        msix_info->dh_dev = rf->dh_dev;
        msix_info->irq = msix_vec->irq;
        msix_info->flags = 0;
        msix_info->data = rf;
        num_cpu = num_online_cpus();
        cpumask_clear(&msix_vec->mask);
        for (cpu_loop= 0; cpu_loop < num_cpu; cpu_loop++) {
            cpumask_set_cpu(cpu_loop, &msix_vec->mask);
        }
        cpumask_copy(msix_info->mask, &msix_vec->mask);
        ret = rf->gen_ops.zxdh_common_func(msix_info, NULL, ZXDH_FUNC_IRQ_REQUEST);	
	}
	if (ret) {
        pr_err("[zxdh_rdma] ERR: aeq irq config fail\n");
        free_cpumask_var(msix_info->mask);
        return -EINVAL;
	}
	rf->sc_dev.irq_ops->zxdh_cfg_aeq(&rf->sc_dev, msix_vec->idx);
	rf->aeq.irq = msix_vec->irq;
	rf->aeq.msix_idx = msix_vec->idx;
	rf->aeq.irq_sta = true;
	return 0;
}

/**
 * zxdh_cfg_ext_aeq_vector - set up the msix vector for ext aeq
 * @rf: RDMA PCI function
 *
 * Allocate interrupt resources and enable irq handling
 * Return 0 if successful, otherwise return error
 */
static int zxdh_cfg_ext_aeq_vector(struct zxdh_pci_f *rf)
{
#ifndef MSIX_SUPPORT
	return 0;
#endif
	struct zxdh_msix_vector *msix_vec = rf->iw_msixtbl;

	// 配置 ext vhca 的 msix 中断信息
	rf->ext_sc_dev.irq_ops->zxdh_cfg_aeq(&rf->ext_sc_dev, msix_vec->idx);
	rf->ext_aeq.irq = msix_vec->irq;
	rf->ext_aeq.msix_idx = msix_vec->idx;
	return 0;
}

/**
 * zxdh_create_ceq - create completion event queue
 * @rf: RDMA PCI function
 * @iwceq: pointer to the ceq resources to be created
 * @ceq_id: the id number of the iwceq
 *
 * Return 0, if the ceq and the resources associated with it
 * are successfully created, otherwise return error
 */
static int zxdh_create_ceq(struct zxdh_pci_f *rf, struct zxdh_ceq *iwceq,
			   u32 ceq_id)
{
	int status;
	struct zxdh_ceq_init_info info = {};
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	u64 scratch;
	u32 ceq_size;
	u32 log2_ceq_size;

	info.ceq_id = ceq_id;
	info.ceq_index = ceq_id - dev->base_ceqn;
	iwceq->rf = rf;
	ceq_size = min(rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_CQ].cnt,
		       dev->hw_attrs.max_hw_ceq_size);
	ceq_size = roundup_pow_of_two(ceq_size);
	log2_ceq_size = order_base_2(ceq_size);

	iwceq->mem.size =
		ALIGN(sizeof(struct zxdh_ceqe) * ceq_size, ZXDH_CEQ_ALIGNMENT);

	if (rf->use_ext_mem_flag)
	{
		iwceq->mem.va = (void *)zxdh_get_ext_mem(rf, iwceq->mem.size, &iwceq->mem.pa);
	}
	else
	{
		iwceq->mem.va = dma_alloc_coherent(dev->hw->device, iwceq->mem.size,
						&iwceq->mem.pa, GFP_KERNEL);
	}

	if (!iwceq->mem.va)
		return -ENOMEM;

	info.ceqe_base = iwceq->mem.va;
	info.ceqe_pa = iwceq->mem.pa;
	info.elem_cnt = ceq_size;
	info.log2_elem_size = log2_ceq_size;
	info.msix_idx = iwceq->msix_idx;
	iwceq->sc_ceq.ceq_id = ceq_id;
	iwceq->sc_ceq.valid_ceq = true;
	info.dev = dev;
	scratch = (uintptr_t)&rf->cqp.sc_cqp;
	status = zxdh_sc_ceq_init(&iwceq->sc_ceq, &info);

	if (!status) {
		if (dev->ceq_valid)
			status = zxdh_cqp_ceq_cmd(&rf->sc_dev, &iwceq->sc_ceq,
						  ZXDH_OP_CEQ_CREATE);
		else
			status = zxdh_sc_cceq_create(&iwceq->sc_ceq, scratch);
	}

	if (status) {
		if (rf->use_ext_mem_flag) {
			zxdh_free_ext_mem(rf, iwceq->mem.va, iwceq->mem.size);
		} else {
			dma_free_coherent(dev->hw->device, iwceq->mem.size,
				iwceq->mem.va, iwceq->mem.pa);
		}
		iwceq->mem.va = NULL;
	}

	return status;
}

/**
 * zxdh_setup_ceq_0 - create CEQ 0 and it's interrupt resource
 * @rf: RDMA PCI function
 *
 * Allocate a list for all device completion event queues
 * Create the ceq 0 and configure it's msix interrupt vector
 * Return 0, if successfully set up, otherwise return error
 */
static int zxdh_setup_ceq_0(struct zxdh_pci_f *rf)
{
	struct zxdh_ceq *iwceq;
	struct zxdh_msix_vector *msix_vec;
	int status = 0;
	u32 num_ceqs;

	num_ceqs = min(rf->msix_count, rf->sc_dev.max_ceqs);
	rf->ceqlist = kcalloc(num_ceqs, sizeof(*rf->ceqlist), GFP_KERNEL);
	if (!rf->ceqlist) {
		status = -ENOMEM;
		goto exit;
	}

	iwceq = &rf->ceqlist[0];
	//0 is aeq, 1~xx is ceq
	msix_vec = &rf->iw_msixtbl[1];
	iwceq->irq = msix_vec->irq;
	iwceq->msix_idx = msix_vec->idx;
	status = zxdh_create_ceq(rf, iwceq, rf->sc_dev.base_ceqn);
	if (status) {
		pr_err("[zxdh_rdma] ERR: create ceq status = %d\n", status);
		goto exit;
	}

	spin_lock_init(&iwceq->ce_lock);
	status = zxdh_cfg_ceq_vector(rf, iwceq, rf->sc_dev.base_ceqn, msix_vec);
	if (status) {
		zxdh_destroy_ceq(rf, iwceq);
		goto exit;
	}

	zxdh_ceq_ena_intr(&rf->sc_dev, iwceq->sc_ceq.ceq_id);
	rf->ceqs_count++;

exit:
	if (status && !rf->ceqs_count) {
		kfree(rf->ceqlist);
		rf->ceqlist = NULL;
		return status;
	}
	rf->sc_dev.ceq_valid = true;

	return 0;
}

/**
 * zxdh_setup_ceqs - manage the device ceq's and their interrupt resources
 * @rf: RDMA PCI function
 *
 * Allocate a list for all device completion event queues
 * Create the ceq's and configure their msix interrupt vectors
 * Return 0, if ceqs are successfully set up, otherwise return error
 */
static int zxdh_setup_ceqs(struct zxdh_pci_f *rf)
{
	u32 i;
	u32 ceq_id;
	u32 ceq_id_offset;
	struct zxdh_ceq *iwceq;
	struct zxdh_msix_vector *msix_vec;
	int status;
	u32 num_ceqs;

	num_ceqs = min(rf->msix_count, rf->sc_dev.max_ceqs);
	i = 2;
	for (ceq_id_offset = 1; ceq_id_offset < num_ceqs;
	     i++, ceq_id_offset++) {
		iwceq = &rf->ceqlist[ceq_id_offset];
		ceq_id = rf->sc_dev.base_ceqn + ceq_id_offset;
		msix_vec = &rf->iw_msixtbl[i];
		iwceq->irq = msix_vec->irq;
		iwceq->msix_idx = msix_vec->idx;
		status = zxdh_create_ceq(rf, iwceq, ceq_id);
		if (status) {
			pr_err("[zxdh_rdma] ERR: create ceq status = %d\n", status);
			goto del_ceqs;
		}
		spin_lock_init(&iwceq->ce_lock);
		status = zxdh_cfg_ceq_vector(rf, iwceq, ceq_id, msix_vec);
		if (status) {
			zxdh_destroy_ceq(rf, iwceq);
			goto del_ceqs;
		}

		zxdh_ceq_ena_intr(&rf->sc_dev, iwceq->sc_ceq.ceq_id);
		rf->ceqs_count++;
	}

	return 0;

del_ceqs:
	zxdh_del_ceqs(rf);

	return status;
}

#if 0
static int zxdh_create_virt_aeq(struct zxdh_pci_f *rf, u32 size)
{
	struct zxdh_aeq *aeq = &rf->aeq;
	dma_addr_t *pg_arr;
	u32 pg_cnt;
	int status;

	if (rf->rdma_ver < ZXDH_GEN_2)
		return -EOPNOTSUPP;

	aeq->mem.size = sizeof(struct zxdh_sc_aeqe) * size;
	aeq->mem.va = vzalloc(aeq->mem.size);

	if (!aeq->mem.va)
		return -ENOMEM;

	pg_cnt = DIV_ROUND_UP(aeq->mem.size, PAGE_SIZE);
	status = zxdh_get_pble(rf->pble_rsrc, &aeq->palloc, pg_cnt, true);
	if (status) {
		vfree(aeq->mem.va);
		return status;
	}

	pg_arr = (dma_addr_t *)aeq->palloc.level1.addr;
	status = zxdh_map_vm_page_list(&rf->hw, aeq->mem.va, pg_arr, pg_cnt);
	if (status) {
		zxdh_free_pble(rf->pble_rsrc, &aeq->palloc);
		vfree(aeq->mem.va);
		return status;
	}

	return 0;
}
#endif

/**
 * zxdh_create_aeq - create async event queue
 * @rf: RDMA PCI function
 *
 * Return 0, if the aeq and the resources associated with it
 * are successfully created, otherwise return error
 */
static int zxdh_create_aeq(struct zxdh_pci_f *rf)
{
	struct zxdh_aeq_init_info info = {};
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_aeq *aeq = &rf->aeq;
	struct zxdh_hmc_info *hmc_info = rf->sc_dev.hmc_info;
	u32 aeq_size;
	u8 multiplier = ZXDH_PER_QP_AEQE_NUM;
	int status;

	aeq_size = multiplier * hmc_info->hmc_obj[ZXDH_HMC_IW_QP].cnt +
		   hmc_info->hmc_obj[ZXDH_HMC_IW_CQ].cnt +
		   hmc_info->hmc_obj[ZXDH_HMC_IW_SRQ].cnt;
	aeq_size = min(aeq_size, dev->hw_attrs.max_hw_aeq_size);

	aeq->mem.size = ALIGN(sizeof(struct zxdh_sc_aeqe) * aeq_size,
			      ZXDH_AEQ_ALIGNMENT);
	
	if (rf->use_ext_mem_flag)
	{
		aeq->mem.va = (void *)zxdh_get_ext_mem(rf, aeq->mem.size, &aeq->mem.pa);
		if (!aeq->mem.va)
		{
			pr_err("[zxdh_rdma] Failed to get ext_mem for aeq\n");
			return -ENOMEM;
		}
	}
	else
	{
		aeq->mem.va = dma_alloc_coherent(dev->hw->device, aeq->mem.size,
						&aeq->mem.pa,
						GFP_KERNEL | __GFP_NOWARN);
	}

	if (aeq->mem.va)
		goto skip_virt_aeq;

	pr_err("[zxdh_rdma] aeq_size out of range, failed to apply for physical memory!\n");
	return -ENOMEM;

#if 0
	/* physically mapped aeq failed. setup virtual aeq */
	status = zxdh_create_virt_aeq(rf, aeq_size);
	if (status)
		return status;

	info.virtual_map = true;
	aeq->virtual_map = info.virtual_map;
	info.pbl_chunk_size = 1;
	info.first_pm_pbl_idx = aeq->palloc.level1.idx;
#endif

skip_virt_aeq:
	info.aeqe_base = aeq->mem.va;
	info.aeq_elem_pa = aeq->mem.pa;
	info.elem_cnt = aeq_size;
	info.dev = dev;
	info.msix_idx = rf->iw_msixtbl->idx;
	status = zxdh_sc_aeq_init(&aeq->sc_aeq, &info);
	if (status)
		goto err;

	status = zxdh_cqp_aeq_create(&aeq->sc_aeq);
	if (status)
		goto err;

	return 0;

err:
	if (aeq->virtual_map)
		zxdh_destroy_virt_aeq(rf);
	else {
		if (rf->use_ext_mem_flag) {
			zxdh_free_ext_mem(rf, aeq->mem.va, aeq->mem.size);
		} else {
			dma_free_coherent(dev->hw->device, aeq->mem.size, aeq->mem.va,
				  aeq->mem.pa);
		}
		aeq->mem.va = NULL;
	}
	return status;
}

/**
 * zxdh_create_ext_aeq - create ext async event queue
 * @rf: RDMA PCI function
 *
 * Return 0, if the aeq and the resources associated with it
 * are successfully created, otherwise return error
 */
static int zxdh_create_ext_aeq(struct zxdh_pci_f *rf)
{
	struct zxdh_aeq_init_info info = {};
	struct zxdh_sc_dev *ext_dev = &rf->ext_sc_dev;
	struct zxdh_aeq *ext_aeq = &rf->ext_aeq;
	struct zxdh_hmc_info *hmc_info = rf->sc_dev.hmc_info;  // ext vhca 和原始 vhca 共用的 hmc信息
	u32 aeq_size;
	u8 multiplier = (rf->protocol_used == ZXDH_IWARP_PROTOCOL_ONLY) ? 2 : 1;
	int status;

	aeq_size = multiplier * hmc_info->hmc_obj[ZXDH_HMC_IW_QP].cnt +
		   hmc_info->hmc_obj[ZXDH_HMC_IW_CQ].cnt +
		   hmc_info->hmc_obj[ZXDH_HMC_IW_SRQ].cnt;
	aeq_size = min(aeq_size, ext_dev->hw_attrs.max_hw_aeq_size);

	ext_aeq->mem.size = ALIGN(sizeof(struct zxdh_sc_aeqe) * aeq_size,
			      ZXDH_AEQ_ALIGNMENT);

	ext_aeq->mem.va = dma_alloc_coherent(ext_dev->hw->device, ext_aeq->mem.size,
					&ext_aeq->mem.pa,
					GFP_KERNEL | __GFP_NOWARN);

	if (ext_aeq->mem.va)
		goto skip_virt_aeq;

	pr_err("[zxdh_rdma] ext aeq_size out of range, failed to apply for physical memory!\n");
	return -ENOMEM;

#if 0
	/* physically mapped aeq failed. setup virtual aeq */
	status = zxdh_create_virt_aeq(rf, aeq_size);    // todo: zxdh_create_virt_ext_aeq
	if (status)
		return status;

	info.virtual_map = true;
	ext_aeq->virtual_map = info.virtual_map;
	info.pbl_chunk_size = 1;
	info.first_pm_pbl_idx = ext_aeq->palloc.level1.idx;
#endif

skip_virt_aeq:
	info.aeqe_base = ext_aeq->mem.va;
	info.aeq_elem_pa = ext_aeq->mem.pa;
	info.elem_cnt = aeq_size;
	info.dev = ext_dev;
	info.msix_idx = rf->iw_msixtbl->idx;
	status = zxdh_sc_aeq_init(&ext_aeq->sc_aeq, &info);
	if (status)
		goto err;

	status = zxdh_cqp_ext_aeq_create(&ext_aeq->sc_aeq);
	if (status)
		goto err;

	return 0;

err:
	if (ext_aeq->virtual_map){
		pr_info("[zxdh_rdma] [%s] ext_aeq->virtual_map \n", __func__);
		zxdh_destroy_virt_ext_aeq(rf);
	} else {
		dma_free_coherent(ext_dev->hw->device, ext_aeq->mem.size, ext_aeq->mem.va,
				ext_aeq->mem.pa);
		ext_aeq->mem.va = NULL;
	}
	return status;
}

/**
 * zxdh_setup_aeq - set up the device aeq
 * @rf: RDMA PCI function
 *
 * Create the aeq and configure its msix interrupt vector or polling mode
 * Return 0 if successful, otherwise return error
 */
static int zxdh_setup_aeq(struct zxdh_pci_f *rf)
{
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	int status;

	status = zxdh_create_aeq(rf);
	if (status)
		return status;

	/* Check if polling mode should be used */
	if (rf->aeq.work_mode == ZXDH_AEQ_MODE_POLLING) {
		/* Polling mode: start polling thread instead of configuring interrupts */
		status = zxdh_aeq_polling_start(rf);
		if (status) {
			zxdh_init_destroy_aeq(rf);
			return status;
		}

		/* Even in polling mode, configure basic interrupt registers */
		zxdh_cfg_aeq(&rf->sc_dev, rf->iw_msixtbl->idx);
		/* But don't enable interrupts */
		zxdh_aeq_ena_intr(dev, false);

	} else {
		/* Interrupt mode: configure interrupt vector */
		status = zxdh_cfg_aeq_vector(rf);
		if (status) {
			zxdh_init_destroy_aeq(rf);
			return status;
		}
		zxdh_aeq_ena_intr(dev, true);
	}
	return 0;
}

/**
 * zxdh_setup_ext_aeq - set up the device ext aeq
 * @rf: RDMA PCI function
 *
 * Create the aeq and configure its msix interrupt vector
 * Return 0 if successful, otherwise return error
 */
static int zxdh_setup_ext_aeq(struct zxdh_pci_f *rf)
{
	struct zxdh_sc_dev *ext_dev = &rf->ext_sc_dev;
	int status;

	status = zxdh_create_ext_aeq(rf);
	if (status)
		return status;
	status = zxdh_cfg_ext_aeq_vector(rf);
	if (status) {
		zxdh_init_destroy_ext_aeq(rf);
		return status;
	}
	zxdh_aeq_ena_intr(ext_dev, true);
	return 0;
}

/**
 * zxdh_hmc_setup - create hmc objects for the device
 * @rf: RDMA PCI function
 *
 * Set up the device private memory space for the number and size of
 * the hmc objects and create the objects
 * Return 0 if successful, otherwise return error
 */
static int zxdh_hmc_setup(struct zxdh_pci_f *rf)
{
	int status;
	struct zxdh_sc_dev *dev = &rf->sc_dev;

	status = zxdh_cfg_fpm_val(dev);
	if (status)
		return status;

	if (!rf->use_ext_mem_flag)
		status = zxdh_create_hmc_objs(rf, true);

	return status;
}

static int zxdh_data_cap_setup(struct zxdh_pci_f *rf)
{
	int status;
	struct zxdh_sc_dev *dev = &rf->sc_dev;

	status = zxdh_sc_create_date_cap_obj(dev);
	if (status) {
		zxdh_del_data_cap_objects(&rf->sc_dev);
		ibdev_err(&rf->iwdev->ibdev, "ERR: create data cap status = %d\n", status);
	}

	return status;
}

/**
 * zxdh_del_init_mem - deallocate memory resources
 * @rf: RDMA PCI function
 */
static void zxdh_del_init_mem(struct zxdh_pci_f *rf)
{
	struct zxdh_sc_dev *dev = &rf->sc_dev;

	kfree(dev->hmc_info->sd_table.sd_entry);
	dev->hmc_info->sd_table.sd_entry = NULL;

	vfree(rf->qp_cnt_8k_idxs);
	rf->qp_cnt_8k_idxs = NULL;

	vfree(rf->mem_rsrc);
	rf->mem_rsrc = NULL;

	if (rf->ceqlist) {
		kfree(rf->ceqlist);
		rf->ceqlist = NULL;
	}
	if (rf->iw_msixtbl) {
		kfree(rf->iw_msixtbl);
		rf->iw_msixtbl = NULL;
	}
	kfree(rf->hmc_info_mem);
	rf->hmc_info_mem = NULL;
}

/**
 * zxdh_initialize_dev - initialize device
 * @rf: RDMA PCI function
 *
 * Allocate memory for the hmc objects and initialize iwdev
 * Return 0 if successful, otherwise clean up the resources
 * and return error
 */
static int zxdh_initialize_dev(struct zxdh_pci_f *rf)
{
	struct zxdh_device_init_info info = {};
	int ret = 0;

	info.bar0 = rf->hw.hw_addr;
	info.privileged = !rf->ftype;
	info.max_vfs = rf->max_rdma_vfs;
	info.hw = &rf->hw;
	rf->vlan_parse_en = 1;
	ret = zxdh_sc_dev_init(rf->rdma_ver, &rf->sc_dev, &info);

	return ret;
}

/**
 * zxdh_rt_deinit_hw - clean up the zrdma device resources
 * @iwdev: zrdma device
 *
 * remove the mac ip entry and ipv4/ipv6 addresses, destroy the
 * device queues and free the pble and the hmc objects
 */
void zxdh_rt_deinit_hw(struct zxdh_device *iwdev)
{
	switch (iwdev->init_state) {
	case AEQ_CREATED:
	case PBLE_CHUNK_MEM:
	case CEQS_CREATED:
	default:
		dev_warn(idev_to_dev(&iwdev->rf->sc_dev),
			 "rt bad init_state = %d\n", iwdev->init_state);
		break;
	}

	if (iwdev->cleanup_wq)
		destroy_workqueue(iwdev->cleanup_wq);
}

static int zxdh_setup_init_state(struct zxdh_pci_f *rf)
{
	int status;

	status = zxdh_save_msix_info(rf);
	if (status)
		return status;
	rf->hw.device = &rf->pcidev->dev;

	mutex_init(&rf->sc_dev.vchnl_mutex);
	status = zxdh_initialize_dev(rf);
	if (status)
		goto clean_msixtbl;

	return 0;

clean_msixtbl:
	kfree(rf->iw_msixtbl);
	rf->iw_msixtbl = NULL;
	return status;
}

/**
 * zxdh_get_used_rsrc - determine resources used internally
 * @iwdev: zrdma device
 *
 * Called at the end of open to get all internal allocations
 */
static void zxdh_get_used_rsrc(struct zxdh_device *iwdev)
{
	iwdev->rf->used_pds = find_next_zero_bit(iwdev->rf->allocated_pds,
						 iwdev->rf->max_pd, 0);
	iwdev->rf->used_qps = find_next_zero_bit(iwdev->rf->allocated_qps,
						 iwdev->rf->max_qp, 0);
	iwdev->rf->used_cqs = find_next_zero_bit(iwdev->rf->allocated_cqs,
						 iwdev->rf->max_cq, 0);
	iwdev->rf->used_mrs = find_next_zero_bit(iwdev->rf->allocated_mrs,
						 iwdev->rf->max_mr, 0);
	iwdev->rf->used_srqs = find_next_zero_bit(iwdev->rf->allocated_srqs,
						  iwdev->rf->max_srq, 0);
}

static void zxdh_shutdown_vhca(struct zxdh_pci_f *rf)
{
	u32 invalid_sid = 63;
	u32 qpc_axi_info;
	writel(invalid_sid, (u32 __iomem *)(rf->sc_dev.hw->hw_addr + C_RDMAIO_TABLE2(rf->sc_dev.ep_id)));
	qpc_axi_info = readl((u32 __iomem *)(rf->sc_dev.hw->hw_addr + C_HMC_QPC_RX(rf->sc_dev.ep_id)));
	qpc_axi_info |= (3 << 2);
	writel(qpc_axi_info,
		       (u32 __iomem *)(rf->sc_dev.hw->hw_addr + C_HMC_QPC_RX(rf->sc_dev.ep_id)));
}

static void zxdh_shutdown_ext_vhca(struct zxdh_pci_f *rf)
{
	u32 invalid_sid = 63;
	u32 qpc_axi_info;
	writel(invalid_sid, (u32 __iomem *)(rf->ext_sc_dev.hw->ext_hw_addr + C_RDMAIO_EXT_TABLE2(rf->ext_sc_dev.ep_id)));
	qpc_axi_info = readl((u32 __iomem *)(rf->ext_sc_dev.hw->ext_hw_addr + C_EXT_HMC_QPC_RX(rf->ext_sc_dev.ep_id)));
	qpc_axi_info |= (3 << 2);
	writel(qpc_axi_info,
		       (u32 __iomem *)(rf->ext_sc_dev.hw->ext_hw_addr + C_EXT_HMC_QPC_RX(rf->ext_sc_dev.ep_id)));
}

void zxdh_ctrl_deinit_hw(struct zxdh_pci_f *rf)
{
	uint16_t vf_id;
	struct zxdh_vfdev *vf_dev = NULL;
	enum init_completion_state state = rf->init_state;

	rf->init_state = INVALID_STATE;
	/* Stop AEQ polling before destroy to avoid use-after-free */
	if (rf->aeq.work_mode == ZXDH_AEQ_MODE_POLLING) {
		zxdh_aeq_polling_stop(rf);
	}
	if (state > AEQ_CREATED)
	    zxdh_destroy_aeq(rf);
	else if(state == AEQ_CREATED)
		zxdh_destroy_aeq_reg(rf);
	if (rf->rsrc_created) {
		zxdh_destroy_pble_prm(rf->pble_rsrc);
		zxdh_destroy_pble_prm(rf->pble_mr_rsrc);
		zxdh_del_ceqs(rf);
		rf->rsrc_created = false;
	}

	switch (state) {
	case VF_NP_TBL_INITIALIZED:
		if (rf->ftype)
			zxdh_vf_deinit_np_tbl(rf);
		fallthrough;
	case VF_HMC_CREATED:
		if (rf->ftype)
			zxdh_vf_del_hmc(rf);
		fallthrough;
	case CEQ0_CREATED:
		zxdh_del_ceq_0(rf);
		fallthrough;
	case CCQ_CREATED:
		zxdh_destroy_ccq(rf);
		fallthrough;
	case HW_RSRC_INITIALIZED:
	case HMC_OBJS_CREATED:
	case DATA_CAP_CREATED:
		if (enable_iova_cap) {
			zxdh_del_data_cap_objects(&rf->sc_dev);
		}
		fallthrough;
	case CQP_QP_CREATED:
		zxdh_destroy_cqp_qp(rf);
		fallthrough;
	case SMMU_PAGETABLE_INITIALIZED:
		if (!rf->ftype)
			zxdh_smmu_pagetable_exit(&rf->sc_dev);
		fallthrough;
	case CQP_CREATED:
		zxdh_destroy_cqp(rf, !rf->reset);
		fallthrough;
	case INITIAL_STATE:
		break;
	case INVALID_STATE:
	default:
		pr_warn("[zxdh_rdma] ctrl bad init_state = %d\n", rf->init_state);
		break;
	}

    if(rf->use_blue_flame_flag && (!rf->ftype)){
        pr_info("[zxdh_rdma] [%s] rf->state:%d , ext vhca deinit hw \n", __func__, state);
        if(state >= AEQ_CREATED)
        {
            zxdh_destroy_ext_aeq_reg(rf);
            zxdh_destroy_ext_cqp_qp(rf);
            zxdh_destroy_ext_cqp(rf, !rf->reset);
        }
        else if(state == CQP_QP_CREATED)
        {
            zxdh_destroy_ext_cqp_qp(rf);
            zxdh_destroy_ext_cqp(rf, !rf->reset);
        }
        else if(state >= CQP_CREATED)
        {
            zxdh_destroy_ext_cqp(rf, !rf->reset);
        }
    }

	if (rf->ftype == 0) {
		for (vf_id = 0; vf_id < rf->max_rdma_vfs; vf_id++) {
			vf_dev = zxdh_find_vf_dev(&rf->sc_dev, vf_id);
			if (vf_dev) {
				zxdh_del_hmc_objects(
					&rf->sc_dev,
					&vf_dev->hmc_info);
				zxdh_remove_vf_dev(&rf->sc_dev, vf_dev);
			}
		}
	}
	if(rf->sc_dev.hw_attrs.skip_hw == false)
	{
		msleep(10);
	}
	zxdh_shutdown_vhca(rf);
	if (rf->use_blue_flame_flag && (!rf->ftype)){
		zxdh_shutdown_ext_vhca(rf);
	}
    zxdh_del_hmc_objects(&rf->sc_dev, rf->sc_dev.hmc_info);
    zxdh_del_init_mem(rf);
}

/**
 * zxdh_rt_init_hw - Initializes runtime portion of HW
 * @iwdev: zrdma device
 *
 * Create device queues ILQ, IEQ, CEQs and PBLEs. Setup zrdma
 * device resource objects.
 */
int zxdh_rt_init_hw(struct zxdh_device *iwdev)
{
	struct zxdh_pci_f *rf = iwdev->rf;
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	int status;

	zxdh_sc_dev_qplist_init(dev);
	do {
		if (!rf->rsrc_created) {
			status = zxdh_setup_ceqs(rf);
			if (status)
				break;

			iwdev->init_state = CEQS_CREATED;

			rf->pble_rsrc->fpm_base_addr =
				rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_PBLE]
					.base;
			rf->sc_dev.hmc_info->pble_hmc_index =
				rf->sc_dev.hmc_info->hmc_first_entry_pble;
			status = zxdh_hmc_init_pble(&rf->sc_dev, rf->pble_rsrc,
						    PBLE_QUEUE);
			if (status) {
				zxdh_del_ceqs(rf);
				break;
			}
			rf->pble_mr_rsrc->fpm_base_addr =
				rf->sc_dev.hmc_info
					->hmc_obj[ZXDH_HMC_IW_PBLE_MR]
					.base;
			rf->sc_dev.hmc_info->pble_mr_hmc_index =
				rf->sc_dev.hmc_info->hmc_first_entry_pble_mr;
			status = zxdh_hmc_init_pble(&rf->sc_dev,
						    rf->pble_mr_rsrc, PBLE_MR);
			if (status) {
				zxdh_destroy_pble_prm(rf->pble_rsrc);
				zxdh_del_ceqs(rf);
				break;
			}

			iwdev->init_state = PBLE_CHUNK_MEM;
			rf->rsrc_created = true;
		}

		iwdev->device_cap_flags =
			IB_DEVICE_MEM_WINDOW | IB_DEVICE_MEM_MGT_EXTENSIONS |
			IB_DEVICE_BAD_QKEY_CNTR | IB_DEVICE_SYS_IMAGE_GUID |
			IB_DEVICE_RC_RNR_NAK_GEN | IB_DEVICE_N_NOTIFY_CQ;
#ifndef IB_DEV_CAPS_VER_2
		iwdev->device_cap_flags |= IB_DEVICE_LOCAL_DMA_LKEY;
#endif

		iwdev->cleanup_wq = alloc_ordered_workqueue(
			"zrdma-cleanup-wq", WQ_HIGHPRI | WQ_UNBOUND);
		if (!iwdev->cleanup_wq)
			return -ENOMEM;

		zxdh_get_used_rsrc(iwdev);
		init_waitqueue_head(&iwdev->suspend_wq);

		return 0;
	} while (0);

	dev_err(idev_to_dev(dev),
		"HW runtime init FAIL status = %d last cmpl = %d\n", status,
		iwdev->init_state);
	zxdh_rt_deinit_hw(iwdev);

	return status;
}

static void zxdh_config_tx_regs(struct zxdh_sc_dev *dev)
{
	u32 temp;
	struct zxdh_pci_f *rf;

	rf = container_of(dev, struct zxdh_pci_f, sc_dev);

	if(rf->use_ext_mem_flag)
	{
		temp = FIELD_PREP(ZXDH_TX_CACHE_ID, 0) |
	       FIELD_PREP(ZXDH_TX_INDICATE_ID, ZXDH_INDICATE_HOST_NOSMMU) |
	       FIELD_PREP(ZXDH_TX_AXI_ID, (ZXDH_AXID_HOST_EP0 + ZXDH_EXT_MEM_EP_ID)) |
	       FIELD_PREP(ZXDH_TX_WAY_PARTITION, 0);
		
		writel(temp,
			(u32 __iomem *)(dev->hw->hw_addr + RDMATX_ACK_SQWQE_PARA_CFG(dev->ep_id)));
		writel(temp,
			(u32 __iomem *)(dev->hw->hw_addr + RDMATX_ACK_DDR_PARA_CFG(dev->ep_id)));
		writel(temp,
			(u32 __iomem *)(dev->hw->hw_addr + RDMATX_DB_SQWQE_ID_CFG(dev->ep_id)));
		writel(temp, (u32 __iomem *)(dev->hw->hw_addr + RDMATX_SQWQE_PARA_CFG(dev->ep_id)));
		writel(temp,
			(u32 __iomem *)(dev->hw->hw_addr + RDMATX_PAYLOAD_PARA_CFG(dev->ep_id)));
	
		temp = FIELD_PREP(ZXDH_TX_CACHE_ID, dev->cache_id) |
			FIELD_PREP(ZXDH_TX_INDICATE_ID,
				ZXDH_INDICATE_HOST_NOSMMU) |
			FIELD_PREP(ZXDH_TX_AXI_ID,
				(ZXDH_AXID_HOST_EP0 + ZXDH_EXT_MEM_EP_ID)) |
			FIELD_PREP(ZXDH_TX_WAY_PARTITION, 0);
			
		writel(temp, (u32 __iomem *)(dev->hw->hw_addr + C_HMC_MRTE_TX2(dev->ep_id)));
		writel(temp, (u32 __iomem *)(dev->hw->hw_addr + C_HMC_PBLEMR_TX2(dev->ep_id)));

		writel((ZXDH_AXID_HOST_EP0 + ZXDH_EXT_MEM_EP_ID),
			(u32 __iomem *)(dev->hw->hw_addr + RDMATX_HOSTID_CFG(dev->ep_id)));
	}
	else
	{
		temp = FIELD_PREP(ZXDH_TX_CACHE_ID, 0) |
			FIELD_PREP(ZXDH_TX_INDICATE_ID, ZXDH_INDICATE_HOST_NOSMMU) |
			FIELD_PREP(ZXDH_TX_AXI_ID, (ZXDH_AXID_HOST_EP0 + dev->ep_id)) |
			FIELD_PREP(ZXDH_TX_WAY_PARTITION, 0);

		writel(temp,
			(u32 __iomem *)(dev->hw->hw_addr + RDMATX_ACK_SQWQE_PARA_CFG(dev->ep_id)));
		writel(temp,
			(u32 __iomem *)(dev->hw->hw_addr + RDMATX_ACK_DDR_PARA_CFG(dev->ep_id)));
		writel(temp,
			(u32 __iomem *)(dev->hw->hw_addr + RDMATX_DB_SQWQE_ID_CFG(dev->ep_id)));
		writel(temp, (u32 __iomem *)(dev->hw->hw_addr + RDMATX_SQWQE_PARA_CFG(dev->ep_id)));
		writel(temp,
			(u32 __iomem *)(dev->hw->hw_addr + RDMATX_PAYLOAD_PARA_CFG(dev->ep_id)));

		if (dev->hmc_use_dpu_ddr) {
			temp = FIELD_PREP(ZXDH_TX_CACHE_ID, dev->cache_id) |
				FIELD_PREP(ZXDH_TX_INDICATE_ID, ZXDH_INDICATE_DPU_DDR) |
				FIELD_PREP(ZXDH_TX_AXI_ID,
					(ZXDH_AXID_HOST_EP0 + dev->ep_id)) |
				FIELD_PREP(ZXDH_TX_WAY_PARTITION, 0);
		} else {
			temp = FIELD_PREP(ZXDH_TX_CACHE_ID, dev->cache_id) |
				FIELD_PREP(ZXDH_TX_INDICATE_ID,
					ZXDH_INDICATE_HOST_SMMU) |
				FIELD_PREP(ZXDH_TX_AXI_ID,
					(ZXDH_AXID_HOST_EP0 + dev->ep_id)) |
				FIELD_PREP(ZXDH_TX_WAY_PARTITION, 0);
		}
		writel(temp, (u32 __iomem *)(dev->hw->hw_addr + C_HMC_MRTE_TX2(dev->ep_id)));
		writel(temp, (u32 __iomem *)(dev->hw->hw_addr + C_HMC_PBLEMR_TX2(dev->ep_id)));

		writel((ZXDH_AXID_HOST_EP0 + dev->ep_id),
			(u32 __iomem *)(dev->hw->hw_addr + RDMATX_HOSTID_CFG(dev->ep_id)));
	}
}

static void zxdh_config_ext_tx_regs(struct zxdh_sc_dev *ext_dev)
{
	u32 temp;

	temp = FIELD_PREP(ZXDH_TX_CACHE_ID, 0) |
	       FIELD_PREP(ZXDH_TX_INDICATE_ID, ZXDH_INDICATE_HOST_NOSMMU) |
	       FIELD_PREP(ZXDH_TX_AXI_ID, (ZXDH_AXID_HOST_EP0 + ext_dev->ep_id)) |
	       FIELD_PREP(ZXDH_TX_WAY_PARTITION, 0);

	writel(temp,
	       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + RDMATX_EXT_ACK_DDR_PARA_CFG(ext_dev->ep_id)));
	writel(temp,
	       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + RDMATX_EXT_PAYLOAD_PARA_CFG(ext_dev->ep_id)));

	//ext
	temp = FIELD_PREP(ZXDH_TX_CACHE_ID, 0) |
	       FIELD_PREP(ZXDH_TX_INDICATE_ID, ZXDH_INDICATE_L2D) |
	       FIELD_PREP(ZXDH_TX_AXI_ID, ZXDH_AXID_L2D) |
	       FIELD_PREP(ZXDH_TX_WAY_PARTITION, 0);

	writel(temp,
	       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + RDMATX_EXT_ACK_SQWQE_PARA_CFG(ext_dev->ep_id)));
	writel(temp,
	       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + RDMATX_EXT_DB_SQWQE_ID_CFG(ext_dev->ep_id)));
	writel(temp, (u32 __iomem *)(ext_dev->hw->ext_hw_addr + RDMATX_EXT_SQWQE_PARA_CFG(ext_dev->ep_id)));

	if (ext_dev->hmc_use_dpu_ddr) {
		temp = FIELD_PREP(ZXDH_TX_CACHE_ID, ext_dev->cache_id) |
		       FIELD_PREP(ZXDH_TX_INDICATE_ID, ZXDH_INDICATE_DPU_DDR) |
		       FIELD_PREP(ZXDH_TX_AXI_ID,
				  (ZXDH_AXID_HOST_EP0 + ext_dev->ep_id)) |
		       FIELD_PREP(ZXDH_TX_WAY_PARTITION, 0);
	} else {
		temp = FIELD_PREP(ZXDH_TX_CACHE_ID, ext_dev->cache_id) |
			FIELD_PREP(ZXDH_TX_INDICATE_ID,
				ZXDH_INDICATE_HOST_SMMU) |
			FIELD_PREP(ZXDH_TX_AXI_ID,
				(ZXDH_AXID_HOST_EP0 + ext_dev->ep_id)) |
			FIELD_PREP(ZXDH_TX_WAY_PARTITION, 0);
	}
	writel(temp, (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_EXT_HMC_MRTE_TX2(ext_dev->ep_id)));
	writel(temp, (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_EXT_HMC_PBLEMR_TX2(ext_dev->ep_id)));

	writel((ZXDH_AXID_HOST_EP0 + ext_dev->ep_id),
	       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + RDMATX_EXT_HOSTID_CFG(ext_dev->ep_id)));

}

static void zxdh_config_rx_regs(struct zxdh_sc_dev *dev)
{
	struct zxdh_pci_f *rf = container_of(dev, struct zxdh_pci_f, sc_dev);

	u32 temp;

	if(rf->use_ext_mem_flag)
	{
		temp = FIELD_PREP(ZXDH_RX_CACHE_ID, 0) |
	       FIELD_PREP(ZXDH_RX_INDICATE_ID, ZXDH_INDICATE_HOST_NOSMMU) |
	       FIELD_PREP(ZXDH_RX_AXI_ID, (ZXDH_AXID_HOST_EP0 + ZXDH_EXT_MEM_EP_ID)) |
	       FIELD_PREP(ZXDH_RX_WAY_PARTITION, 0);

		writel(temp,
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_PLD_WR_AXIID_RAM(dev->ep_id)));
		writel(temp, (u32 __iomem *)(dev->hw->hw_addr + RDMARX_RQ_AXI_RAM(dev->ep_id)));
		writel(temp, (u32 __iomem *)(dev->hw->hw_addr + RDMARX_SRQ_AXI_RAM(dev->ep_id)));
		writel(temp,
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_ACK_RQDB_AXI_RAM(dev->ep_id)));
		writel(temp,
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_CQ_CQE_AXI_INFO_RAM(dev->ep_id)));
		writel(temp,
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_CQ_DBSA_AXI_INFO_RAM(dev->ep_id)));
		writel(dev->hmc_fn_id, (u32 __iomem *)(dev->hw->hw_addr +
							RDMARX_MUL_CACHE_CFG_SIDN_RAM(dev->ep_id)));
		writel((ZXDH_AXID_HOST_EP0 + ZXDH_EXT_MEM_EP_ID),
			(u32 __iomem *)(dev->hw->hw_addr +
					RDMARX_MUL_COPY_QPN_INDICATE(dev->ep_id)));
		writel(RDMARX_MAX_MSG_SIZE,
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_VHCA_MAX_SIZE_RAM(dev->ep_id)));
	}
	else
	{
		temp = FIELD_PREP(ZXDH_RX_CACHE_ID, 0) |
	       FIELD_PREP(ZXDH_RX_INDICATE_ID, ZXDH_INDICATE_HOST_NOSMMU) |
	       FIELD_PREP(ZXDH_RX_AXI_ID, (ZXDH_AXID_HOST_EP0 + dev->ep_id)) |
	       FIELD_PREP(ZXDH_RX_WAY_PARTITION, 0);

		writel(temp,
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_PLD_WR_AXIID_RAM(dev->ep_id)));
		writel(temp, (u32 __iomem *)(dev->hw->hw_addr + RDMARX_RQ_AXI_RAM(dev->ep_id)));
		writel(temp, (u32 __iomem *)(dev->hw->hw_addr + RDMARX_SRQ_AXI_RAM(dev->ep_id)));
		writel(temp,
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_ACK_RQDB_AXI_RAM(dev->ep_id)));
		writel(temp,
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_CQ_CQE_AXI_INFO_RAM(dev->ep_id)));
		writel(temp,
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_CQ_DBSA_AXI_INFO_RAM(dev->ep_id)));
		writel(dev->hmc_fn_id, (u32 __iomem *)(dev->hw->hw_addr +
							RDMARX_MUL_CACHE_CFG_SIDN_RAM(dev->ep_id)));
		writel((ZXDH_AXID_HOST_EP0 + dev->ep_id),
			(u32 __iomem *)(dev->hw->hw_addr +
					RDMARX_MUL_COPY_QPN_INDICATE(dev->ep_id)));
		writel(RDMARX_MAX_MSG_SIZE,
			(u32 __iomem *)(dev->hw->hw_addr + RDMARX_VHCA_MAX_SIZE_RAM(dev->ep_id)));

		if (rf->ftype == 0) {
			// writel(ZXDH_HMC_HOST_MGCPAYLOAD_MAX_QUANTITY, (u32 __iomem *)(dev->hw->hw_addr + RDMARX_MUL_CACHE_CFG_INDEX_SUM_RAM));
			// writel(1, (u32 __iomem *)(dev->hw->hw_addr + RDMARX_MUL_CACHE_CFG_VLD_RAM));
			// writel(dev->vhca_id, (u32 __iomem *)(dev->hw->hw_addr + RDMARX_MUL_CACHE_CFG_VHCA_RAM));
		}
	}
}

static void zxdh_config_ext_rx_regs(struct zxdh_sc_dev *ext_dev)
{
	u32 temp;

	temp = FIELD_PREP(ZXDH_RX_CACHE_ID, 0) |
	       FIELD_PREP(ZXDH_RX_INDICATE_ID, ZXDH_INDICATE_HOST_NOSMMU) |
	       FIELD_PREP(ZXDH_RX_AXI_ID, (ZXDH_AXID_HOST_EP0 + ext_dev->ep_id)) |
	       FIELD_PREP(ZXDH_RX_WAY_PARTITION, 0);

	writel(temp,
	       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + RDMARX_EXT_PLD_WR_AXIID_RAM(ext_dev->ep_id)));
	writel(temp, (u32 __iomem *)(ext_dev->hw->ext_hw_addr + RDMARX_SRQ_AXI_RAM(ext_dev->ep_id)));
	writel(temp,
	       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + EXT_RDMARX_ACK_RQDB_AXI_RAM(ext_dev->ep_id)));
	writel(temp,
	       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + RDMARX_EXT_CQ_CQE_AXI_INFO_RAM(ext_dev->ep_id)));
	writel(temp,
	       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + RDMARX_EXT_CQ_DBSA_AXI_INFO_RAM(ext_dev->ep_id)));
	writel(ext_dev->hmc_fn_id, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						       RDMARX_EXT_MUL_CACHE_CFG_SIDN_RAM(ext_dev->ep_id)));
	writel((ZXDH_AXID_HOST_EP0 + ext_dev->ep_id),
	       (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
			       RDMARX_EXT_MUL_COPY_QPN_INDICATE(ext_dev->ep_id)));
	writel(RDMARX_MAX_MSG_SIZE,
	       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + RDMARX_EXT_VHCA_MAX_SIZE_RAM(ext_dev->ep_id)));

	temp = FIELD_PREP(ZXDH_RX_CACHE_ID, 0) |
	       FIELD_PREP(ZXDH_RX_INDICATE_ID, ZXDH_INDICATE_L2D) |
	       FIELD_PREP(ZXDH_RX_AXI_ID, ZXDH_AXID_L2D) |
	       FIELD_PREP(ZXDH_RX_WAY_PARTITION, 0);

	writel(temp, (u32 __iomem *)(ext_dev->hw->ext_hw_addr + RDMARX_EXT_RQ_AXI_RAM(ext_dev->ep_id)));
}

static void zxdh_config_io_regs(struct zxdh_sc_dev *dev)
{
	u32 temp0, temp1, temp2;
	struct zxdh_pci_f *rf = container_of(dev, struct zxdh_pci_f, sc_dev);

	temp0 = FIELD_PREP(ZXDH_IOTABLE2_SID, dev->hmc_fn_id);
	writel(temp0, (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE2(dev->ep_id)));

	if(rf->use_ext_mem_flag)
		temp1 = FIELD_PREP(ZXDH_IOTABLE4_EPID,
				(ZXDH_HOST_EP0_ID + ZXDH_EXT_MEM_EP_ID)) |
			FIELD_PREP(ZXDH_IOTABLE4_VFID, dev->vf_id) |
			FIELD_PREP(ZXDH_IOTABLE4_PFID, rf->pf_id);
	else
		temp1 = FIELD_PREP(ZXDH_IOTABLE4_EPID,
				(ZXDH_HOST_EP0_ID + dev->ep_id)) |
			FIELD_PREP(ZXDH_IOTABLE4_VFID, dev->vf_id) |
			FIELD_PREP(ZXDH_IOTABLE4_PFID, rf->pf_id);
	writel(temp1, (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE4(dev->ep_id)));

	temp0 = 0x10000;
	writel(temp0, (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE3(dev->ep_id)));
	for (temp0 = 0; temp0 < 32; temp0++) {
		if (temp0 < ZXDH_RW_PAYLOAD || temp0 == ZXDH_QPC_OBJ_ID) {
			writel(0, (u32 __iomem *)(dev->hw->hw_addr +
						  C_RDMAIO_TABLE5_0(dev->ep_id) +
						  (temp0 * 4)));
		} else {
			temp2 = (rf->ftype == 0) ? 0 : ZXDH_TABLE5_VF_EN;
			writel(temp2, (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE5_0(dev->ep_id) + (temp0 * 4)));
		}
	}

	if (rf->ftype == 0) {
		writel(0,
		       (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE6_0(dev->ep_id)));
		writel(0,
		       (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE6_1(dev->ep_id)));
		writel(0,
		       (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE6_2(dev->ep_id)));
		writel(0,
		       (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE6_3(dev->ep_id)));
		writel(0,
		       (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE6_4(dev->ep_id)));
		writel(0,
		       (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE6_5(dev->ep_id)));
		writel(0,
		       (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE6_6(dev->ep_id)));
		writel(0,
		       (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE6_7(dev->ep_id)));
		writel(0,
		       (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE6_8(dev->ep_id)));
		writel(0,
		       (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE6_9(dev->ep_id)));
		writel(0,
		       (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE6_10(dev->ep_id)));
		writel(0,
		       (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE6_11(dev->ep_id)));
		writel(0,
		       (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE6_12(dev->ep_id)));
		writel(0,
		       (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE6_13(dev->ep_id)));
		writel(0,
		       (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE6_14(dev->ep_id)));
		writel(0,
		       (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE6_15(dev->ep_id)));

		if(rf->use_ext_mem_flag)
			temp2 = FIELD_PREP(ZXDH_IOTABLE7_PFID, rf->pf_id) |
				FIELD_PREP(ZXDH_IOTABLE7_EPID,
					(ZXDH_HOST_EP0_ID + ZXDH_EXT_MEM_EP_ID));
		else
			temp2 = FIELD_PREP(ZXDH_IOTABLE7_PFID, rf->pf_id) |
				FIELD_PREP(ZXDH_IOTABLE7_EPID,
					(ZXDH_HOST_EP0_ID + rf->ep_id));
		writel(temp2,
		       (u32 __iomem *)(dev->hw->hw_addr + C_RDMAIO_TABLE7(dev->ep_id)));
	} 
}

static void zxdh_config_ext_io_regs(struct zxdh_sc_dev *ext_dev)
{
	u32 temp0, temp1;
	struct zxdh_pci_f *rf = container_of(ext_dev, struct zxdh_pci_f, ext_sc_dev);
	struct zxdh_sc_dev *dev = &rf->sc_dev;

	temp0 = FIELD_PREP(ZXDH_IOTABLE2_SID, ext_dev->hmc_fn_id);
	writel(temp0, (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_RDMAIO_EXT_TABLE2(ext_dev->ep_id)));

	temp1 = FIELD_PREP(ZXDH_IOTABLE4_EPID,
			   (ZXDH_HOST_EP0_ID + dev->ep_id)) |
		FIELD_PREP(ZXDH_IOTABLE4_VFID, dev->vf_id) |
		FIELD_PREP(ZXDH_IOTABLE4_PFID, rf->pf_id);
	writel(temp1, (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_RDMAIO_EXT_TABLE4(ext_dev->ep_id)));

	temp0 = 0x10000;
	writel(temp0, (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_RDMAIO_EXT_TABLE3(ext_dev->ep_id)));
	for (temp0 = 0; temp0 < 32; temp0++) {
		if (temp0 < ZXDH_RW_PAYLOAD || temp0 == ZXDH_QPC_OBJ_ID) {
			writel(0, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						  C_RDMAIO_EXT_TABLE5_0(ext_dev->ep_id) +
						  (temp0 * 4)));
		} else {
			writel((rf->ftype), (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
							    C_RDMAIO_EXT_TABLE5_0(ext_dev->ep_id) +
							    (temp0 * 4)));
		}
	}
}

static void zxdh_config_hw_regs(struct zxdh_sc_dev *dev)
{
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	struct zxdh_sc_dev *ext_dev = &rf->ext_sc_dev;

	zxdh_config_tx_regs(dev);
	zxdh_config_rx_regs(dev);
	zxdh_config_io_regs(dev);

	// ext
	if(rf->use_blue_flame_flag && (!rf->ftype))
	{
		zxdh_config_ext_tx_regs(ext_dev);
		zxdh_config_ext_rx_regs(ext_dev);
		zxdh_config_ext_io_regs(ext_dev);
	}
}

static void zxdh_open_schedule(struct zxdh_sc_dev *dev)
{
    writel(ZXDH_HW_SCHEDULE_ON, (u32 __iomem *)(dev->hw->hw_addr + RDMATX_QUEUE_VHCA_FLAG(dev->ep_id)));
}

/**
 * zxdh_ctrl_init_hw - Initializes control portion of HW
 * @rf: RDMA PCI function
 *
 * Create admin queues, HMC obejcts and RF resource objects
 */
int zxdh_ctrl_init_hw(struct zxdh_pci_f *rf)
{
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	u32 k = 0;
	int status = 0;
	
	do {
		status = zxdh_setup_init_state(rf);
		if (status)
			break;
		rf->init_state = INITIAL_STATE;
		zxdh_open_schedule(dev);
		zxdh_config_hw_regs(dev);

		status = zxdh_create_cqp(rf);
		if (status)
			break;
		if (rf->use_blue_flame_flag && (!rf->ftype)) {
			status = zxdh_create_ext_cqp(rf);
			if (status)
				break;
			
			// 在使用 cqp 之前清 cache0
			pr_info("[zxdh_rdma] [%s] flush_cache0_all_space \n", __func__);
			flush_cache0_all_space(&rf->ext_sc_dev);
		}

		rf->init_state = CQP_CREATED;
		zxdh_init_destroy_aeq(rf);
		if (rf->use_blue_flame_flag && (!rf->ftype)) {
			zxdh_init_destroy_ext_aeq(rf);
		}
		if (!rf->ftype) {
			status = zxdh_smmu_pagetable_init(dev);
			if (status)
				break;
			rf->init_state = SMMU_PAGETABLE_INITIALIZED;
			if (enable_iova_cap) {
				if (!rf->use_ext_mem_flag)
				{
					if (rf->sc_dev.ep_id != ZXDH_ZF_EPID || dev->hmc_use_dpu_ddr) {
						status = zxdh_data_cap_setup(rf);
						if (status)
							break;
						rf->init_state = DATA_CAP_CREATED;
					}
				}
			}
			if (dev->hmc_use_dpu_ddr) {
				status = zxdh_clear_dpuddr(
					dev, true); //TODO:VF clear dpu ddr
				if (status) {
					pr_err("[zxdh_rdma] clear dpuddr failed! status=%d\n", status);
					break;
				}
				status = zxdh_create_hmcobjs_dpuddr(rf);
			} else
				status = zxdh_hmc_setup(rf);

			if (dev->clear_dpu_mem.va) {
				dma_free_coherent(dev->hw->device,
						  dev->clear_dpu_mem.size,
						  dev->clear_dpu_mem.va,
						  dev->clear_dpu_mem.pa);
				dev->clear_dpu_mem.va = NULL;
			}

			for (k = 0; k < rf->max_rdma_vfs; k++)
				zxdh_pf_get_vf_hmc_res(dev, k);

		} else if (rf->ftype == 1) {
			zxdh_hmc_dpu_capability(dev);
			for (k = 0; k < ZXDH_HMC_IW_MAX; k++) {
				zxdh_sc_write_hmc_register(
					dev, dev->hmc_info->hmc_obj, k,
					dev->vhca_id);
			}
			zxdh_create_vf_pblehmc_entry(dev);
		} else {
			pr_info("[zxdh_rdma] ftype is error!!\n");
			status = EINVAL;
		}

		if (status)
			break;
		rf->init_state = HMC_OBJS_CREATED;

		status = zxdh_initialize_hw_rsrc(rf);
		if (status)
			break;
		rf->init_state = HW_RSRC_INITIALIZED;
		status = zxdh_create_cqp_qp(rf);
		if (status)
			break;
		if (rf->use_blue_flame_flag && (!rf->ftype)) {
			status = zxdh_create_ext_cqp_qp(rf);
			if (status)
				break;
		}
		rf->init_state = CQP_QP_CREATED;

		/* Initialize AEQ configuration (polling/interrupt mode) */
		status = zxdh_init_aeq_config(rf);
		if (status)
			break;

		status = zxdh_setup_aeq(rf);
		if (status)
			break;
		if (rf->use_blue_flame_flag && (!rf->ftype)) {
			status = zxdh_setup_ext_aeq(rf);
			if (status)
				break;
		}
		rf->init_state = AEQ_CREATED;

		status = zxdh_create_ccq(rf);
		if (status)
			break;
		rf->init_state = CCQ_CREATED;

		status = zxdh_setup_ceq_0(rf);
		if (status)
			break;

		rf->sc_dev.ceq_0_ok = true;
		rf->sc_dev.ceq_interrupt = false;
		rf->init_state = CEQ0_CREATED;
		/* Handles processing of CQP completions */
		rf->cqp_cmpl_wq = alloc_ordered_workqueue(
			"cqp_cmpl_wq", WQ_HIGHPRI | WQ_UNBOUND);
		if (!rf->cqp_cmpl_wq) {
			status = -ENOMEM;
			break;
		}
		INIT_WORK(&rf->cqp_cmpl_work, cqp_compl_worker);
#ifdef MSIX_SUPPORT
		zxdh_sc_ccq_arm(dev->ccq);
#endif

		if (rf->ftype == 1 && !dev->hmc_use_dpu_ddr) {
			zxdh_set_smmu_invalid(rf);
			status = zxdh_vf_init_hmc(rf);
			if (status)
				break;
			rf->init_state = VF_HMC_CREATED;
		}

		if (rf->ftype) {
			status = zxdh_vf_init_np_tbl(rf);
			if (status)
				break;
			rf->init_state = VF_NP_TBL_INITIALIZED;
		}

		return 0;
	} while (0);

	pr_err("[zxdh_rdma] ZRDMA vhca_id:%d hardware initialization FAILED, init_state=%d status=%d\n",
	       rf->sc_dev.vhca_id, rf->init_state, status);
	zxdh_ctrl_deinit_hw(rf);
	return status;
}

/**
 * zxdh_set_hw_rsrc - set hw memory resources.
 * @rf: RDMA PCI function
 */
static void zxdh_set_hw_rsrc(struct zxdh_pci_f *rf)
{
#ifdef Z_CONFIG_RDMA_ARP
	rf->allocated_srqs =
		(void *)(rf->mem_rsrc +
			 (sizeof(struct zxdh_arp_entry) * rf->arp_table_size));
#else
	rf->allocated_srqs = (void *)(rf->mem_rsrc);
#endif
	rf->allocated_qps = &rf->allocated_srqs[BITS_TO_LONGS(rf->max_srq)];
	rf->allocated_cqs = &rf->allocated_qps[BITS_TO_LONGS(rf->max_qp)];
	rf->allocated_mrs = &rf->allocated_cqs[BITS_TO_LONGS(rf->max_cq)];
	rf->allocated_pds = &rf->allocated_mrs[BITS_TO_LONGS(rf->max_mr)];
	rf->allocated_ahs = &rf->allocated_pds[BITS_TO_LONGS(rf->max_pd)];
	rf->allocated_mcgs = &rf->allocated_ahs[BITS_TO_LONGS(rf->max_ah)];
	rf->allocated_pris = &rf->allocated_mcgs[BITS_TO_LONGS(rf->max_mcg)];
	rf->allocated_8k_idx = &rf->allocated_pris[BITS_TO_LONGS(rf->max_pri)];
#ifdef Z_CONFIG_RDMA_ARP
	rf->allocated_arps = &rf->allocated_8k_idx[BITS_TO_LONGS(rf->max_8k_idx)];
	rf->qp_table = (struct zxdh_qp **)(&rf->allocated_arps[BITS_TO_LONGS(
		rf->arp_table_size)]);

#else
	rf->qp_table =
		(struct zxdh_qp *
			 *)(&rf->allocated_8k_idx[BITS_TO_LONGS(rf->max_8k_idx)]);
#endif
	rf->cq_table = (struct zxdh_cq **)(&rf->qp_table[rf->max_qp]);
	rf->srq_table = (struct zxdh_srq **)(&rf->cq_table[rf->max_cq]);
	rf->allocated_bf_qps = (unsigned long *)(&rf->srq_table[rf->max_srq]);

	spin_lock_init(&rf->rsrc_lock);
#ifdef Z_CONFIG_RDMA_ARP
	spin_lock_init(&rf->arp_lock);
#endif
	spin_lock_init(&rf->qptable_lock);
	spin_lock_init(&rf->cqtable_lock);
	spin_lock_init(&rf->srqtable_lock);
}

/**
 * zxdh_calc_mem_rsrc_size - calculate memory resources size.
 * @rf: RDMA PCI function
 */
static u32 zxdh_calc_mem_rsrc_size(struct zxdh_pci_f *rf)
{
	u32 rsrc_size;

#ifdef Z_CONFIG_RDMA_ARP
	rsrc_size = sizeof(struct zxdh_arp_entry) * rf->arp_table_size;
	rsrc_size += sizeof(unsigned long) * BITS_TO_LONGS(rf->max_srq);
#else
	rsrc_size = sizeof(unsigned long) * BITS_TO_LONGS(rf->max_srq);
#endif
	rsrc_size += sizeof(unsigned long) * BITS_TO_LONGS(rf->max_qp);
	rsrc_size += sizeof(unsigned long) * BITS_TO_LONGS(rf->max_mr);
	rsrc_size += sizeof(unsigned long) * BITS_TO_LONGS(rf->max_cq);
	rsrc_size += sizeof(unsigned long) * BITS_TO_LONGS(rf->max_pd);
#ifdef Z_CONFIG_RDMA_ARP
	rsrc_size += sizeof(unsigned long) * BITS_TO_LONGS(rf->arp_table_size);
#endif
	rsrc_size += sizeof(unsigned long) * BITS_TO_LONGS(rf->max_ah);
	rsrc_size += sizeof(unsigned long) * BITS_TO_LONGS(rf->max_mcg);
	rsrc_size += sizeof(unsigned long) * BITS_TO_LONGS(rf->max_pri);
	rsrc_size += sizeof(unsigned long) * BITS_TO_LONGS(rf->max_8k_idx);
	rsrc_size += sizeof(unsigned long) * BITS_TO_LONGS(rf->max_bf_qp);
	rsrc_size += sizeof(struct zxdh_qp **) * rf->max_qp;
	rsrc_size += sizeof(struct zxdh_cq **) * rf->max_cq;
	rsrc_size += sizeof(struct zxdh_srq **) * rf->max_srq;

	return rsrc_size;
}

/**
 * zxdh_initialize_hw_rsrc - initialize hw resource tracking array
 * @rf: RDMA PCI function
 */
int zxdh_initialize_hw_rsrc(struct zxdh_pci_f *rf)
{
	u32 rsrc_size;
	u32 mrdrvbits;
	int ret;

	rf->max_cqe = rf->sc_dev.hw_attrs.uk_attrs.max_hw_cq_size;
	rf->max_qp = rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_QP].cnt;
	rf->max_mr = rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_MR].cnt;
	rf->max_cq = rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_CQ].cnt;
	rf->max_srq = rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_SRQ].cnt;
	rf->max_pd = rf->sc_dev.hw_attrs.max_hw_pds;
	rf->max_ah = rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_AH].cnt;
	rf->max_mcg = rf->max_qp;
	rf->max_pri = ZXDH_MAX_USER_PRIORITY;
	rf->max_8k_idx = rf->sc_dev.vhca_8k_index_cnt;

	rf->qp_cnt_8k_idxs = vzalloc(rf->max_8k_idx * sizeof(u32));
	if (!rf->qp_cnt_8k_idxs) {
		ret = -ENOMEM;
		goto mem_8k_qp_cnt_vmalloc_fail;
	}

	rsrc_size = zxdh_calc_mem_rsrc_size(rf);
	rf->mem_rsrc = vzalloc(rsrc_size);
	if (!rf->mem_rsrc) {
		ret = -ENOMEM;
		goto mem_rsrc_vmalloc_fail;
	}
#ifdef Z_CONFIG_RDMA_ARP
	rf->arp_table = (struct zxdh_arp_entry *)rf->mem_rsrc;
#endif

	zxdh_set_hw_rsrc(rf);

	set_bit(0, rf->allocated_mrs);
	set_bit(1, rf->allocated_mrs);
	set_bit(0, rf->allocated_pds);
	set_bit(0, rf->allocated_qps);
#ifdef Z_CONFIG_RDMA_ARP
	set_bit(0, rf->allocated_arps);
#endif
	set_bit(0, rf->allocated_ahs);
	set_bit(0, rf->allocated_mcgs);
	// set_bit(0, rf->allocated_srqs);

	/* stag index mask has a minimum of 14 bits */
	mrdrvbits = 24 - max(get_count_order(rf->max_mr), 14);
	rf->mr_stagmask = ~(((1 << mrdrvbits) - 1) << (32 - mrdrvbits));

	return 0;

mem_rsrc_vmalloc_fail:
	vfree(rf->qp_cnt_8k_idxs);
mem_8k_qp_cnt_vmalloc_fail:
	return ret;
}

/**
 * zxdh_cqp_ce_handler - handle cqp completions
 * @rf: RDMA PCI function
 * @cq: cq for cqp completions
 */
void zxdh_cqp_ce_handler(struct zxdh_pci_f *rf, struct zxdh_sc_cq *cq)
{
	struct zxdh_cqp_request *cqp_request;
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	u32 cqe_count = 0;
	struct zxdh_ccq_cqe_info info;
	unsigned long flags;
	int ret = 0;
	char *error_info;
	bool error_flag = false;

	do {
		memset(&info, 0, sizeof(info));
		spin_lock_irqsave(&rf->cqp.compl_lock, flags);
		ret = zxdh_sc_ccq_get_cqe_info(cq, &info, &error_flag);
		spin_unlock_irqrestore(&rf->cqp.compl_lock, flags);
		if (ret) {
			if (dev->hw_attrs.skip_hw == true)
				return ;
			break;
		}

		if(error_flag) {
			zxdh_rdma_skip_hw_cfg(rf);
			return ;
		}

		cqp_request =
			(struct zxdh_cqp_request *)(unsigned long)info.scratch;
		if (info.error &&
		    zxdh_cqp_crit_err(dev, cqp_request->info.cqp_cmd,
				      info.maj_err_code, info.min_err_code))
		{
            error_info = zxdh_get_aeq_info((u32)info.min_err_code + 65536);
			pr_err("[zxdh_rdma] cqp opcode = 0x%x maj_err_code = 0x%x min_err_code = 0x%x error_info=%s\n",
			        info.op_code, info.maj_err_code,
			        info.min_err_code, error_info);
			kfree(error_info);
		}
		if (cqp_request && (info.mailbox_cqe != 1)) {
			cqp_request->compl_info.maj_err_code =
				info.maj_err_code;
			cqp_request->compl_info.min_err_code =
				info.min_err_code;
			cqp_request->compl_info.op_ret_val = info.op_ret_val;
			cqp_request->compl_info.error = info.error;

			if (info.op_code == ZXDH_CQP_OP_WQE_DMA_READ_USECQE) {
				cqp_request->compl_info.addrbuf[0] =
					info.addrbuf[0];
				cqp_request->compl_info.addrbuf[1] =
					info.addrbuf[1];
				cqp_request->compl_info.addrbuf[2] =
					info.addrbuf[2];
				cqp_request->compl_info.addrbuf[3] =
					info.addrbuf[3];
				cqp_request->compl_info.addrbuf[4] =
					info.addrbuf[4];
			}

			if (cqp_request->waiting) {
				cqp_request->request_done = true;
				wake_up(&cqp_request->waitq);
				zxdh_put_cqp_request(&rf->cqp, cqp_request);
			} else {
				if (cqp_request->callback_fcn)
					cqp_request->callback_fcn(cqp_request);
				zxdh_put_cqp_request(&rf->cqp, cqp_request);
			}
		} else if (info.mailbox_cqe == 1) {
			if (rf->ftype == 0) {
				ret = zxdh_recv_mb(dev, &info);
				if (ret != 0)
					pr_info("[zxdh_rdma] pf recv mb failed\n");
			} else {
				ret = zxdh_recv_vf_mb(dev, &info);
				if (ret != 0)
					pr_info("[zxdh_rdma] vf recv mb failed\n");
			}	
		}

		cqe_count++;
	} while (1);

	if (cqe_count) {
		zxdh_sc_ccq_arm(dev->ccq);
		dev->ceq_interrupt = false;
		zxdh_process_bh(dev);
	}
	if (dev->ceq_interrupt == true) {
		zxdh_sc_ccq_arm(dev->ccq);
		dev->ceq_interrupt = false;
	}
}

/**
 * cqp_compl_worker - Handle cqp completions
 * @work: Pointer to work structure
 */
void cqp_compl_worker(struct work_struct *work)
{
	struct zxdh_pci_f *rf =
		container_of(work, struct zxdh_pci_f, cqp_cmpl_work);
	struct zxdh_sc_cq *cq = &rf->ccq.sc_cq;

	zxdh_cqp_ce_handler(rf, cq);
}

/**
 * zxdh_hw_flush_wqes - flush qp's wqe
 * @rf: RDMA PCI function
 * @qp: hardware control qp
 * @info: info for flush
 * @wait: flag wait for completion
 */
int zxdh_hw_flush_wqes(struct zxdh_pci_f *rf, struct zxdh_sc_qp *qp,
		       struct zxdh_qp_flush_info *info, bool wait)
{
	int status;
	struct zxdh_qp_flush_info *hw_info;
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	hw_info = &cqp_request->info.in.u.qp_flush_wqes.info;
	memcpy(hw_info, info, sizeof(*hw_info));
	cqp_info->cqp_cmd = ZXDH_OP_QP_FLUSH_WQES;
	cqp_info->post_sq = 1;
	cqp_info->in.u.qp_flush_wqes.qp = qp;
	cqp_info->in.u.qp_flush_wqes.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(rf, cqp_request);
	if (status) {
		qp->qp_uk.sq_flush_complete = true;
		qp->qp_uk.rq_flush_complete = true;
		zxdh_put_cqp_request(&rf->cqp, cqp_request);
		return status;
	}

	if (!wait || cqp_request->compl_info.maj_err_code)
		goto put_cqp;

	if (info->rq) {
		if (cqp_request->compl_info.min_err_code ==
			    ZXDH_CQP_COMPL_SQ_WQE_FLUSHED ||
		    cqp_request->compl_info.min_err_code == 0) {
			/* RQ WQE flush was requested but did not happen */
			qp->qp_uk.rq_flush_complete = true;
		}
	}
	if (info->sq) {
		if (cqp_request->compl_info.min_err_code ==
			    ZXDH_CQP_COMPL_RQ_WQE_FLUSHED ||
		    cqp_request->compl_info.min_err_code == 0) {
			/* SQ WQE flush was requested but did not happen */
			qp->qp_uk.sq_flush_complete = true;
		}
	}

put_cqp:
	zxdh_put_cqp_request(&rf->cqp, cqp_request);

	return status;
}

void zxdh_flush_wqes(struct zxdh_qp *iwqp, u32 flush_mask)
{
	struct zxdh_qp_flush_info info = {};
	struct zxdh_pci_f *rf = iwqp->iwdev->rf;
	u8 flush_code = iwqp->sc_qp.flush_code;

	if (!(flush_mask & ZXDH_FLUSH_SQ) && !(flush_mask & ZXDH_FLUSH_RQ))
		return;

	if (iwqp->sc_qp.is_nvmeof_ioq)
		return;


	/* Set flush info fields*/
	info.sq = flush_mask & ZXDH_FLUSH_SQ;
	info.rq = flush_mask & ZXDH_FLUSH_RQ;

	/* Generate userflush errors in CQE */
	info.sq_major_code = ZXDH_FLUSH_MAJOR_ERR;
	info.sq_minor_code = FLUSH_GENERAL_ERR;
	info.rq_major_code = ZXDH_FLUSH_MAJOR_ERR;
	info.rq_minor_code = FLUSH_GENERAL_ERR;
	info.userflushcode = true;

	if (flush_mask & ZXDH_REFLUSH) {
		if (info.sq)
			iwqp->sc_qp.flush_sq = false;
		if (info.rq)
			iwqp->sc_qp.flush_rq = false;
	} else {
		if (flush_code) {
			if (info.sq && iwqp->sc_qp.sq_flush_code)
				info.sq_minor_code = flush_code;
			if (info.rq && iwqp->sc_qp.rq_flush_code)
				info.rq_minor_code = flush_code;
		}
	}

	/* Issue flush */
	(void)zxdh_hw_flush_wqes(rf, &iwqp->sc_qp, &info,
				 flush_mask & ZXDH_FLUSH_WAIT);
	iwqp->flush_issued = true;
}

void zxdh_create_hasg_tbl(void)
{
	if(0 == zxdh_regis_hash_tbl(error_code_info, 185))
	{
		pr_info("[zxdh_rdma] aeq hash table register succeed!");
	}
}

int zxdh_regis_hash_tbl(struct zxdh_aeq_cqp_errorcode_tbl error_code[], int len)
{
    u32 hash_val = 0;
	int i;
    for (i = 0; i < len; i++)
    {
        hash_val = zxdh_get_hash_val(error_code[i].error_code);

        if (hash_val >= ZXDH_AEQ_ERRORCODE_MAP_LEN )
        {
            return 1;
        }
        if (errorcode_map[hash_val].used_flag == 0)
        {
            //没有冲突
            if (1 == zxdh_create_hashkey_normal(hash_val, &error_code[i]))
            {
                return 1;
            }
        }
        else
        {
            //冲突
            if (1 == zxdh_solve_hash_collision(hash_val, &error_code[i]))
            {
                return 1;
            }
        }
    }
    return 0;
}

int zxdh_get_hash_val(u32 error_code)
{
    u32 temp = error_code;
    u32 temp2 = 0;
    u32 hash_val = 0;
    while (temp != 0)
    {
        temp2 += temp % 13;
        temp = (int)(temp / 11);
    }
    
    hash_val = temp2 % ZXDH_AEQ_ERRORCODE_MAP_LEN;
    return hash_val;

}

int zxdh_create_hashkey_normal(u32 hash_val, struct zxdh_aeq_cqp_errorcode_tbl *phash_tbl)
{
    u32 len = 0;
    if (hash_val >= ZXDH_AEQ_ERRORCODE_MAP_LEN)
    {
        return 1;
    }
    errorcode_map[hash_val].error_code = phash_tbl->error_code;
    errorcode_map[hash_val].used_flag  = 1;
    len = strnlen(phash_tbl->error_info, ZXDH_AEQ_ERRORINFO_MAX_LEN) + 1;
    errorcode_map[hash_val].error_info = (char *)kmalloc(len, GFP_KERNEL);
    if (errorcode_map[hash_val].error_info == NULL)
    {
		errorcode_map[hash_val].used_flag  = 0;
        return 1;
    }
    memset((void*)errorcode_map[hash_val].error_info, 0, len);
    strscpy((char*)errorcode_map[hash_val].error_info, phash_tbl->error_info, len);

    return 0;
}

int zxdh_solve_hash_collision(u32 hash_val, struct zxdh_aeq_cqp_errorcode_tbl *phash_tbl)
{
    u32 len = 0;
	struct zxdh_errorcode_map_tbl *ptemp;
	struct zxdh_errorcode_map_tbl *pnew;

    if (hash_val >= ZXDH_AEQ_ERRORCODE_MAP_LEN)
    {
        return 1;
    }

    errorcode_map[hash_val].used_list_flag = 1;
    
    ptemp = &errorcode_map[hash_val];
    while (ptemp->pnext != NULL)
    {
        ptemp = ptemp->pnext;
    }
    
    pnew = (struct zxdh_errorcode_map_tbl *)kmalloc(sizeof(struct zxdh_errorcode_map_tbl), GFP_KERNEL);
    if (NULL == pnew)
    {
        return 1;
    }

    pnew->error_code = phash_tbl->error_code;
    pnew->pnext = NULL;
    pnew->used_flag = 1;
    pnew->used_list_flag = 1;

    len = strnlen(phash_tbl->error_info, ZXDH_AEQ_ERRORINFO_MAX_LEN) + 1;
    pnew->error_info = (char *)kmalloc(len, GFP_KERNEL);
    if  (pnew->error_info == NULL)
    {
        kfree(pnew);
        return 1;
    }
    memset((void*)pnew->error_info, 0, len);
    strscpy((char*)pnew->error_info, phash_tbl->error_info, len);
    ptemp->pnext = pnew;

    return 0;
}

char *zxdh_get_aeq_info(u32 error_code)
{
	u32 hash_val = zxdh_get_hash_val(error_code);
    char *error_info;

    error_info = (char *)kmalloc(ZXDH_AEQ_ERRORINFO_MAX_LEN, GFP_ATOMIC);
    if (NULL == error_info)
    {
		pr_info("[zxdh_rdma] error_info is null\n");
        return 0;
    }
    memset((void*)error_info, 0, ZXDH_AEQ_ERRORINFO_MAX_LEN);
    if (1 != errorcode_map[hash_val].used_flag)
    {
                strscpy((char*)error_info, "This errorcode is invalid.", ZXDH_AEQ_ERRORINFO_MAX_LEN);
    }
    if (0 == errorcode_map[hash_val].used_list_flag)
    {
                strscpy((char*)error_info, errorcode_map[hash_val].error_info, ZXDH_AEQ_ERRORINFO_MAX_LEN);
    }
    else
    {
        struct zxdh_errorcode_map_tbl *pHashMapDataRet = &errorcode_map[hash_val];
        while (NULL != pHashMapDataRet && pHashMapDataRet->error_code  != error_code)
        {
            pHashMapDataRet = pHashMapDataRet->pnext;
        }
		if(pHashMapDataRet)
		{
            strscpy((char*)error_info, pHashMapDataRet->error_info, ZXDH_AEQ_ERRORINFO_MAX_LEN);
		}
		else
		{
			strscpy(error_info, "Error code not found in hash chain.", ZXDH_AEQ_ERRORINFO_MAX_LEN);
		}
        
    }
    return error_info;
}

void zxdh_free_errorcode_map(struct zxdh_errorcode_map_tbl errorcode_map[], u32 size)
{
	int i;

	for (i = 0; i < size; i++)
	{
		struct zxdh_errorcode_map_tbl *pcurrent; 
		pcurrent = &errorcode_map[i];

		while(NULL != pcurrent)
		{
			if(pcurrent->error_info)
			{
				kfree((void *)pcurrent->error_info);
			}
			pcurrent = pcurrent->pnext;
		}
	}
}

static void zxdh_free_aeq_cqp_errorcode_tbl(void)
{
    u32 hash_val = 0;
    struct zxdh_errorcode_map_tbl *ptemp = NULL;
    struct zxdh_errorcode_map_tbl *ptemp2 = NULL;

    for (hash_val = 0; hash_val < ZXDH_AEQ_ERRORCODE_MAP_LEN; hash_val++)
    {
        ptemp = &errorcode_map[hash_val];
        ptemp = ptemp->pnext;
        while (ptemp != NULL)
        {
            ptemp2 = ptemp;
            ptemp = ptemp->pnext;
            kfree(ptemp2);
        }
    }
    ptemp = NULL;
    ptemp2 = NULL;
}

void zxdh_remove_hashtable(void)
{
	zxdh_free_errorcode_map(errorcode_map, ZXDH_AEQ_ERRORCODE_MAP_LEN);
	zxdh_free_aeq_cqp_errorcode_tbl();
}
