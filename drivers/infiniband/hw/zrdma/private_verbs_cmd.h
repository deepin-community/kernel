/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef ZXDH_PRIVATE_VERBS_CMD_H
#define ZXDH_PRIVATE_VERBS_CMD_H
#include <rdma/uverbs_ioctl.h>

#define EXTRACT_BITS(value, low, high) \
	(((value) >> (low)) & ((1U << ((high) - (low) + 1)) - 1))
#define C_RDMA_TX_SUB_RW_RSV0 0x62065f84f0u
#define C_RDMA_TX_SUB_RO_RSV5 0x62065f84b4
#define C_SQ_CPU_MAINTAIN_RESERVE1 0x6206623284u
#define C_RQ_CPU_MAINTAIN_RESERVE1 0x62066232a0u
#define C_ACK_CPU_MAINTAIN_RESERVE1 0x62066232bcu
#define C_DB_AXI_INTERFACW_STATE_REG2 0x620660b214u
#define C_WQE_PREFETCH_TOP_FIFO_WE_RD_CNT0 0x62065F0FBCu
#define C_RDMATX_ARBITRATION_DIN_0 0x62065f061cu
#define HOST3_ERR_INFO_FIFO_OVERFLOW_CNT 0x62065F08b0u
#define C_PKT_TIME_OUT_CNT 0x62065F0678u
#define C_ICRC_PROC_SOP_CNT_HW 0x6205400084u
#define C_ICRC_PROC_EOP_CNT_HW 0x6205400088u
#define RDMATX_ACK_RSV_RO_REG_0_HW 0x62065e80a0u
#define RDMATX_ACK_RD_MSG_LOSS_FLAG_CNT 0x62065e83dcu
#define C_NHD_CHECK_ICRC_REMOVAL_EOP_CNT_HW 0x62054004f8u
#define PKT_RTT_T1_GEN_SOP_CNT 0x62065f8494u
#define C_NHD_CHECK_RTT_PROC_SOP_CNT 0x62054004f0u
#define C_RAM_TEST_RSV_1 0x6205478800u
#define C_NHD_CHECK_RTT_PROC_EOP_CNT 0x62054004f4u
#define C_SQ_CPU_FIFO_OVERFLOW_CNT 0x6206623290u
#define C_RQ_CPU_FIFO_OVERFLOW_CNT 0x62066232acu
#define RDMATX_ACK_RSV_RO_REG_1 0x62065e80a4u
#define C_ACK_CPU_FIFO_OVERFLOW_CNT 0x62066232c8u
#define C_SQ_CPU_MAINTAIN_RESERVE2 0x6206623288u
#define C_SQ_CPU_MAINTAIN_RESERVE3 0x620662328cu
#define C_NP_RDY_TEST 0x62065F0ec8u
#define C_ICRC_CHECK_SOP_CNT_HW 0x620540008cu
#define C_MUL_CACHE_ARBITER_D2B_SOP_CNT 0x62054008A8u

#define ZXDH_RESET_RETRY_CQE_SQ_OPCODE_ERR 0x1f

#define C_RQ_INDICATE_ID_REG_CHECK 0x6205800500
#define C_RQDB_INDICATE_ID_REG_CHECK 0x6205800600
#define C_SRQ_INDICATE_ID_REG_CHECK 0x6205800508
#define C_SRQP_INDICATE_ID_REG_CHECK 0x6205800494
#define C_SRQDB_INDICATE_ID_REG_CHECK 0x620580048c
#define C_SQ_INDICATE_ID_REG_CHECK 0x6206800c04

#define C_DB_SHOW_PF_START_QPN_MAP 0x6206600228
#define C_DB_SHOW_PF_END_VHCA_MAP 0x620660022c
#define C_DB_SHOW_PF_VHCA_MAP 0x6206600400
#define C_DB_SHOW_VHCA_PHYSICAL_MAP 0x6206800430
#define C_DB_SHOW_8K_2K_MAP 0x6206602000
#define C_DB_SHOW_GQP_VHCA_MAP 0x62065fc000
#define C_CHECK_GQP_ACTIVE_WRITE 0x62065f81d4
#define C_CHECK_GQP_ACTIVE_READ 0x62065f84c0

#define C_ACTIVE_VHCA_SQ_CNT_CLEAN 0x620662327c
#define C_ACTIVE_VHCA_READ_CNT_CLEAN 0x6206623298
#define C_ACTIVE_VHCA_ACK_CNT_CLEAN 0x62066232b4
#define C_TASK_PREFETCH_RECV_COM_CNT_CLEAN 0x62065F0FB4
#define C_TX_PKT_CNT_CLEAN 0x62065F0680
#define C_RX_PKT_CNT_CLEAN 0x6205400008
#define C_RETRY_TIMEOUTE_CNT_CLEAN 0x62065e809c
#define C_TX_PKT_CNP_CNT_CLEAN 0x620546008c
#define C_TX_PKT_RTT_T1_CNT_CLEAN 0x62065f81dc
#define C_TX_PKT_RTT_T4_CNT_CLEAN 0x6205478880


#define C_ACTIVE_VHCA_SQ_CNT_CLEAN_MASK BIT(4)
#define C_ACTIVE_VHCA_READ_CNT_CLEAN_MASK BIT(4)
#define C_ACTIVE_VHCA_ACK_CNT_CLEAN_MASK BIT(4)
#define C_TASK_PREFETCH_RECV_COM_CNT_CLEAN_MASK BIT(2)
#define C_TX_PKT_CNT_CLEAN_MASK BIT(0)
#define C_RX_PKT_CNT_CLEAN_MASK BIT(0)
#define C_RETRY_TIMEOUTE_CNT_CLEAN_MASK BIT(0)
#define C_TX_PKT_CNP_CNT_CLEAN_MASK BIT(3)
#define C_TX_PKT_RTT_T1_CNT_CLEAN_MASK BIT(0)
#define C_TX_PKT_RTT_T4_CNT_CLEAN_MASK BIT(2)



enum switch_status {
	SWITCH_CLOSE = 0,
	SWITCH_OPEN = 1,
	SWITCH_ERROR,
};

enum zxdh_qp_modify_qpc_mask {
	ZXDH_RETRY_CQE_SQ_OPCODE = 1 << 0,
	ZXDH_ERR_FLAG_SET = 1 << 1,
	ZXDH_PACKAGE_ERR_FLAG = 1 << 2,
	ZXDH_TX_LAST_ACK_PSN = 1 << 3,
	ZXDH_TX_LAST_ACK_WQE_OFFSET_SET = 1 << 4,
	ZXDH_TX_READ_RETRY_FLAG_SET = 1 << 5,
	ZXDH_TX_RDWQE_PYLD_LENGTH = 1 << 6,
	ZXDH_TX_RECV_READ_FLAG_SET = 1 << 7,
	ZXDH_TX_RD_MSG_LOSS_ERR_FLAG_SET = 1 << 8,
};

enum zxdh_qp_reset_qp_code {
	ZXDH_RESET_RETRY_TX_ITEM_FLAG = 1,
};

enum zxdh_read_context_size_const {
	ZXDH_RX_READ_QPC_SIZE = 168,
	ZXDH_RX_QPC_SHIFT = 256,
	ZXDH_TX_READ_QPC_SIZE = 176,
	ZXDH_READ_CQC_SIZE = 64,
	ZXDH_READ_CEQC_SIZE = 24,
	ZXDH_READ_AEQC_SIZE = 16,
	ZXDH_RX_READ_SRQC_SIZE = 64,
	ZXDH_READ_MRTE_SIZE = 64,
};

enum zxdh_obj_size_const {
	ZXDH_PBLE_MR_QUADRUPLE_SIZE = 32, // quadruple pble mr size 
	ZXDH_PBLE_QUEUE_QUADRUPLE_SIZE = 32,  // quadruple pble queue size 
	ZXDH_PBLE_RQ_SIZE = 8,
	ZXDH_AH_SIZE = 64,
	ZXDH_IRD_SIZE = 64,
	ZXDH_TX_WINDOW_SIZE = 64,
	ZXDH_CQ_SHADOW_AREA_SIZE = 8,
	ZXDH_RQ_SHADOW_AREA_SIZE = 8,
	ZXDH_SRQ_SHADOW_AREA_SIZE = 8,
	ZXDH_SQ_UNIT_SIZE = 32,
	ZXDH_CQ_SIZE = 64,
	ZXDH_CEQ_SIZE = 16,
	ZXDH_AEQ_SIZE = 16,
	ZXDH_SRQP_INDEX_SIZE = 2,
};

struct zxdh_srqc_item {
	u32 list_leaf_pbl_size;
	long log_srq_size;
	u64 srq_address;
	u64 srq_list_address;
	u32 leaf_pbl_size;
	u32 log_srq_stride;
	u32 log_srq_stride_wqe_real_size;
	u64 dbr_address;
	u32 hw_wqe_cnt;
};

struct zxdh_qpc_item {
	u32 rq_leaf_pbl_size;
	u64 rq_address;
	u32 log_rq_wqe_size;
	u32 log_rq_wqe_real_size;
	long log_rq_size;
	u64 db_address;
	u32 sq_leaf_pbl_size;
	u64 sq_address;
	long log_sq_size;
};

struct zxdh_qp_addr_context {
	u32 qp_type;
	u32 addr_mode;
	u32 wqe_size;
	u32 wqe_index;
	u64 qp_base_addr;
};

struct zxdh_reset_qp_retry_tx_item {
	u16 tx_win_raddr;
	u32 tx_last_ack_psn;
	u32 last_ack_wqe_offset;
	u16 hw_sq_tail_una;
	u32 rnr_retry_time_l;
	u8 rnr_retry_time_h;
	u8 rnr_retry_threshold;
	u8 read_retry_flag;
	u8 rnr_retry_flag;
	u8 retry_flag;
	u8 cur_retry_count;
	u32 rdwqe_pyld_length;
	u8 recv_read_flag;
	u8 recv_err_flag;
	u8 recv_rd_msg_loss_err_flag;
	u8 recv_rd_msg_loss_err_cnt;
	u8 rd_msg_loss_err_flag;
	u8 pktchk_rd_msg_loss_err_cnt;
	u8 ack_err_flag;
	u8 err_flag;
	u8 package_err_flag;
	u8 retry_cqe_sq_opcode;
};

struct zxdh_qp_tx_win_item {
	u32 start_psn;
	u16 wqe_pointer;
};

struct zxdh_modify_qpc_item {
	u32 tx_last_ack_psn;
	u32 last_ack_wqe_offset;
	u16 hw_sq_tail_una;
	u32 rnr_retry_time_l;
	u8 rnr_retry_time_h;
	u8 rnr_retry_threshold;
	u8 read_retry_flag;
	u8 rnr_retry_flag;
	u8 retry_flag;
	u8 cur_retry_count;
	u8 rdwqe_pyld_length_l;
	u32 rdwqe_pyld_length_h;
	u8 recv_read_flag;
	u8 recv_err_flag;
	u8 recv_rd_msg_loss_err_flag;
	u8 recv_rd_msg_loss_err_cnt;
	u8 rd_msg_loss_err_flag;
	u8 pktchk_rd_msg_loss_err_cnt;
	u8 ack_err_flag;
	u8 err_flag;
	u8 package_err_flag;
	u8 retry_cqe_sq_opcode;
};

struct hw_object_wqe_context {
	u8 op_code;
	u8 wqe_valid;
	u16 src_vhca_Index;
	u8 src_object_id;
	u8 src_waypartition;
	u8 src_path_select;
	u8 src_interface_select;
	u16 dest_vhca_index;
	u8 dest_object_id;
	u8 dest_waypartition;
	u8 dest_path_select;
	u8 dest_interface_select;
	u32 data_length;
	u32 object_size;
	u64 src_address;
	u64 dest_address;
	u64 srqp_aligned_offset;
	int zxdh_hmc_rsrc_type;
	struct zxdh_sc_dev *dev;
	struct zxdh_get_object_data_req *req;
};

enum zxdh_error_code_const {
	ZXDH_NOT_SUPPORT_OBJECT_ID = 100,
	ZXDH_DMA_MEMORY_OVER_2M = 101,
	ZXDH_DMA_READ_NOT_32_ALIGN = 102,
	ZXDH_CACHE_ID_CHECK_ERROR = 103,
	ZXDH_ENTRY_IDX_ERROR = 104,
	ZXDH_PBLE_ADDRESSING_ONLY_SUPPORTS_OBJECT_NUMBER_1 = 105,
	ZXDH_NOT_SUPPORT_TWO_LEVEL_PBLE_CODE = 106,
	ZXDH_NOT_SUPPORT_VIRTUAL_ADDRESS = 107,
	ZXDH_DATA_ENTRY_IDX_OVER_LIMIT = 108,
	ZXDH_QUEUE_ID_ERROR = 109,
	ZXDH_NOT_SUPPORT_CURRENT_DEVICE = 110,
	ZXDH_IOVA_CAP_NOT_SUPPORT = 111,
};

struct zxdh_cqc_item {
	u8 leaf_pbl_size;
	u64 doorbell_shadow_addr;
	u8 log_cqe_num;
	u32 hw_cq_head;
	u64 cq_address;
	u64 root_pble;
};

struct zxdh_aeqc_item {
	u32 aeq_size;
	u8 virtually_mapped;
	u8 leaf_pbl_size;
	u32 aeq_head;
	u64 aeq_address;
};

struct zxdh_ceqc_item {
	u8 leaf_pbl_size;
	u32 ceqe_head;
	u8 log_ceq_num;
	u64 ceq_address;
};

#define CAP_NODE_NUM 2
#define NODE1 1
#define NODE0 0
#define EN_32bit_GROUP_NUM 16
#define BIT_O_31 0
#define BIT_32_63 1
#define BIT_64_95 2
#define BIT_96_127 3
#define BIT_128_159 4
#define BIT_160_191 5
#define BIT_192_223 6
#define BIT_224_255 7
#define BIT_256_287 8
#define BIT_288_319 9
#define BIT_320_351 10
#define BIT_352_383 11
#define BIT_384_415 12
#define BIT_416_447 13
#define BIT_448_479 14
#define BIT_480_511 15
#define CAP_TX 1
#define CAP_RX 2
#define CAP_CFG_ERROR 0x1
#define CAP_ALLOC_ADDR_ERROR 0x2
#define CAP_WRITE_NODE0_REGS_ERROR 0x4
#define CAP_WRITE_NODE1_REGS_ERROR 0x8
#define CAP_ALREADY_START 0x10
#define MAX_CAP_QPS 4
#define VHCA_RC_UD_GQP_MAX_CNT 1023  // 1个vhca最大的 gqp 数量 = 1022+1

struct zxdh_cap_cfg {
	uint8_t cap_position;
	uint64_t size;
	uint32_t channel_select[CAP_NODE_NUM];
	uint32_t channel_open[CAP_NODE_NUM];
	uint32_t node_choose[CAP_NODE_NUM];
	uint32_t node_select[CAP_NODE_NUM];
	uint32_t compare_bit_en[EN_32bit_GROUP_NUM][CAP_NODE_NUM];
	uint32_t compare_data[EN_32bit_GROUP_NUM][CAP_NODE_NUM];
	uint32_t rdma_time_wrl2d[CAP_NODE_NUM];
	uint32_t extra[CAP_NODE_NUM][EN_32bit_GROUP_NUM];
	uint32_t cap_data_start_cap;
};

struct zxdh_cap_gqp {
	uint16_t gqpid[MAX_CAP_QPS];
	uint8_t gqp_num;
};

struct zxdh_rdma_tool_context {
	bool mp_cap_on;
	struct zxdh_cap_gqp mp_cap_gqps;
	bool data_cap_on;
	uint8_t data_cap_position;
	uint8_t data_cap_mem_type;
};

struct zxdh_mp_cap_gqp {
	uint16_t gqpid[VHCA_RC_UD_GQP_MAX_CNT];
	uint8_t gqp_num;
};


struct zxdh_mp_cap_gqps {
	uint16_t gqpid[MAX_CAP_QPS];
	uint8_t gqp_num;
};

struct zxdh_mp_cap_cfg {
	bool cap_use_l2d;
	uint32_t qpn[MAX_CAP_QPS];
	uint8_t qpn_num;
};

struct zxdh_mp_cap_gqp_cfg {
	bool cap_use_l2d;
	uint16_t gqpid[MAX_CAP_QPS];
	uint8_t gqp_num;
	bool is_vm_capture;
};

/* ZXDH Devices ID */
#define ZXDH_DEV_ID_ADAPTIVE_EVB_PF 0x8040 /* ZXDH EVB PF DEVICE ID*/
#define ZXDH_DEV_ID_ADAPTIVE_EVB_VF 0x8041 /* ZXDH EVB VF DEVICE ID*/
#define ZXDH_DEV_ID_ADAPTIVE_E312_PF 0x8049 /* ZXDH E312 PF DEVICE ID*/
#define ZXDH_DEV_ID_ADAPTIVE_E312_VF 0x8060 /* ZXDH E312 VF DEVICE ID*/
#define ZXDH_DEV_ID_ADAPTIVE_X512_PF 0x806B /* ZXDH X512 PF DEVICE ID*/
#define ZXDH_DEV_ID_ADAPTIVE_X512_VF 0x806C /* ZXDH X512 VF DEVICE ID*/

#define ZXDH_SMMU_OFFSET 0x40000u
#define ZXDH_MP_BASERTT_OFFSET 0x8000u
#define ZXDH_SMMU_CMDQ_OFFSET 0x3000u
#define ZXDH_L2D_MPCAP_BUFF_SIZE 0x14000u
#define ZXDH_CAP_DATA_HOST_MEM_SIZE (2 * 1024 * 1024)
#define ZXDH_CAP_DATA_HMC_MEM_SIZE (1024 * 1024 * 1024)
#define ZXDH_LOG_BUF_SIZE 4096

#ifndef ZXDH_UAPI_DEF
const struct uverbs_object_tree_def *zxdh_ib_get_devx_tree(void);
#endif
void zxdh_destroy_rdma_tool_context(struct ib_ucontext *context);
void zxdh_init_rdma_tool_context(struct ib_ucontext *context);
#endif
