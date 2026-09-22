/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef ZXDH_USER_IOCTL_VERBS_H
#define ZXDH_USER_IOCTL_VERBS_H

#include <linux/types.h>

struct zxdh_query_qpc_resp {
	__u8 retry_flag;
	__u8 rnr_retry_flag;
	__u8 read_retry_flag;
	__u8 cur_retry_count;
	__u8 retry_cqe_sq_opcode;
	__u8 err_flag;
	__u8 ack_err_flag;
	__u8 package_err_flag;
	__u8 recv_err_flag;
	__u8 retry_count;
	__u32 tx_last_ack_psn;
};

struct zxdh_modify_qpc_req {
	__u8 retry_flag;
	__u8 rnr_retry_flag;
	__u8 read_retry_flag;
	__u8 cur_retry_count;
	__u8 retry_cqe_sq_opcode;
	__u8 err_flag;
	__u8 ack_err_flag;
	__u8 package_err_flag;
};

struct zxdh_cap_start_resp {
	__u64 cap_pa_node0;
	__u64 cap_pa_node1;
};

struct zxdh_mp_cap_resp {
	__u8 mcode_type;
	__u8 cap_gqp_num;
	__u16 cap_gqpid[4];
	__u64 cap_pa;
};

#define MAX_ACTIVE_GQP_NUM 16
struct zxdh_active_vhca_gqps {
	__u16 vhca_id;
	__u16 gqp_id[MAX_ACTIVE_GQP_NUM];
	__u8 gqp_num;
};

struct zxdh_cc_basic_info {
	__u32 active_gqp_cnt;
	__u16 active_vhca_sq_cnt;
	__u16 active_vhca_read_cnt;
	__u16 active_vhca_ack_cnt;
	__u16 active_qp_sq_cur_cnt;
	__u16 active_qp_rq_cur_cnt;
	__u16 task_prefetch_recv_com_cnt;
	__u64 tx_pkt_cnt;
	__u64 rx_pkt_cnt;
	__u16 flight_pkt_cnt;
	__u16 retry_timeout_cnt;
	__u16 retry_read_cnt;
	__u16 retry_rnr_cnt;
	__u16 retry_nak_cnt;
	__u16 drop_read_msg_cnt;
	__u32 tx_pkt_cnp_cnt;
	__u32 rx_pkt_cnp_cnt;
	__u32 tx_pkt_rtt_t1_cnt;
	__u32 rx_pkt_rtt_t2_cnt;
	__u32 tx_pkt_rtt_t4_cnt;
	__u32 rx_pkt_rtt_t5_cnt;
	__u16 limit_tx_sq_cnt;
	__u16 limit_tx_read_ack_cnt;
	__u32 backpres_tx_pfc_flg_pyh0_3;
	__u32 backpres_tx_pfc_flg_pyh4_7;
	__u16 backpres_tx_pfc_cnt;
	__u16 rx_pkt_ecn_cnt;
	__u8 backpres_rx_pfc_cnt;
	__u8 backpres_rx;
};

struct zxdh_get_object_data_req {
	__u32 queue_id;
	__u8 object_id;
	__u32 entry_idx;
	__u8 object_num;
};

struct zxdh_get_object_data_resp {
	__u64 object_mmap_offset;
	__u32 length;
	__u32 object_size;
	__u64 srqp_aligned_offset;
	__u16 vhca_id;
	__u8 route_id;
	__u8 reserved;
	__u32 queue_id;
};

struct zxdh_db_show_res_map_req {
	__u8 type;
	__u32 qp_id;
	__u64 reg_va;
	__u64 value_va;
	__u64 idx_va;
	__u32 count;
};

struct zxdh_db_show_res_map_resp {
	__u32 count;
	__u64 qp_8k_index;
};

enum zxdh_show_res_map_type {
    ZXDH_SHOW_RES_MAP_PF_TO_QPN,
    ZXDH_SHOW_RES_MAP_PF_TO_VHCA,
    ZXDH_SHOW_RES_MAP_VHCA_TO_PF,
    ZXDH_SHOW_RES_MAP_8K_TO_GQP,
    ZXDH_SHOW_RES_MAP_GQP_TO_VHCA_CREATED,
    ZXDH_SHOW_RES_MAP_GQP_TO_VHCA_ACTIVE,
    ZXDH_SHOW_RES_MAP_QP_TO_8K,
    ZXDH_SHOW_RES_MAP_UNKNOWN,
};

enum zxdh_context_type {
	ZXDH_RX_READ_QPC = 1,
	ZXDH_TX_READ_QPC,
	ZXDH_READ_CQC,
	ZXDH_READ_CEQC,
	ZXDH_READ_AEQC,
	ZXDH_RX_READ_SRQC,
	ZXDH_READ_MRTE,
};

struct zxdh_context_req {  
    enum zxdh_context_type type;
    __u32 resource_id;
};

#define MAX_CONTEXT_SIZE 22
struct zxdh_context_resp {  
    __u64 context_info[MAX_CONTEXT_SIZE];
    __u8 context_size;
};

struct config_rdma_switch_req {
	enum switch_name_e type;
	__u32 value;
};

struct query_rdma_switch_req {
	enum switch_name_e type;
};

struct query_rdma_switch_resp {
	uint32_t value;
	uint32_t next_reboot_value;
};

enum zxdh_health_check_reg_type {
	ZXDH_NORMAL_REG,
	ZXDH_WRITE_FIRST_REG,
	ZXDH_SMMU_REG,
};


struct zxdh_health_check_req {
	__u64 reg_va;
	__u64 value_va;
	__u64 reg_value_va_ex;
	__u16 count;
	__u8 reg_type:2;
};

enum zxdh_cfg_dev_parameter_type {
	TX_STOP_ON_AEQ = 1,
	RX_STOP_ON_AEQ,
	TXRX_STOP_IOVA_CAP,
	CLEAR_ALL_CC_BASIC_CNT,
	CLEAR_ALL_GQPS_MP_CAP,
};

struct zxdh_health_check_resp {
	__u16 count;
	__u16 count_ex;
};

struct zxdh_reg_value {
	__u64 reg_addr;
	__u32 value;
};

struct zxdh_cfg_dev_parameter_req {
    __u8 type;
    __u8 reserved1;
    __u16 reserved2;
};

enum hw_module {
	HW_MODULE_TX,
	HW_MODULE_RX,
	HW_MODULE_CQP,
};

enum zxdh_ram_read_error_code_const {
	ZXDH_CUSTOM_READ_RAM_ERROR_BASE = 100,
	ZXDH_COPY_USER_PARAM_ERROR,
	ZXDH_READ_RAM_ERROR,
	ZXDH_COPY_DATA_TO_USER_ERROR,
	/* Must be last entry*/
	ZXDH_CUSTOM_ERROR_UNKOWN,
};

struct zxdh_read_ram_req {
	__u32 ram_id;
	__u32 ram_addr;
	__u32 ram_width;
	__u32 read_count;
	__u64 value_va;
	__u16 hw_module;
	__u16 reserved1;
	__u32 reserved2;
};

#endif
