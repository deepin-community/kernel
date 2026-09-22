/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef ZXDH_TYPE_H
#define ZXDH_TYPE_H
#include "status.h"
#include "osdep.h"
#include "zrdma.h"
#include "user.h"
#include "hmc.h"
#include "uda.h"
#include "vf.h"
#include "ws.h"
#include "virtchnl.h"
#include "private_verbs_cmd.h"

enum zxdh_page_size {
	ZXDH_PAGE_SIZE_4K = 0,
	ZXDH_PAGE_SIZE_64K = 4,
	ZXDH_PAGE_SIZE_2M = 9,
	ZXDH_PAGE_SIZE_1G = 18,
};

enum zxdh_hdrct_flags {
	DDP_LEN_FLAG = 0x80,
	DDP_HDR_FLAG = 0x40,
	RDMA_HDR_FLAG = 0x20,
};

enum zxdh_term_layers {
	LAYER_RDMA = 0,
	LAYER_DDP = 1,
	LAYER_MPA = 2,
};

enum zxdh_pble_type {
	PBLE_QUEUE = 0,
	PBLE_MR = 1,
};

enum zxdh_term_error_types {
	RDMAP_REMOTE_PROT = 1,
	RDMAP_REMOTE_OP = 2,
	DDP_CATASTROPHIC = 0,
	DDP_TAGGED_BUF = 1,
	DDP_UNTAGGED_BUF = 2,
	DDP_LLP = 3,
};

enum zxdh_term_rdma_errors {
	RDMAP_INV_STAG = 0x00,
	RDMAP_INV_BOUNDS = 0x01,
	RDMAP_ACCESS = 0x02,
	RDMAP_UNASSOC_STAG = 0x03,
	RDMAP_TO_WRAP = 0x04,
	RDMAP_INV_RDMAP_VER = 0x05,
	RDMAP_UNEXPECTED_OP = 0x06,
	RDMAP_CATASTROPHIC_LOCAL = 0x07,
	RDMAP_CATASTROPHIC_GLOBAL = 0x08,
	RDMAP_CANT_INV_STAG = 0x09,
	RDMAP_UNSPECIFIED = 0xff,
};

enum zxdh_term_ddp_errors {
	DDP_CATASTROPHIC_LOCAL = 0x00,
	DDP_TAGGED_INV_STAG = 0x00,
	DDP_TAGGED_BOUNDS = 0x01,
	DDP_TAGGED_UNASSOC_STAG = 0x02,
	DDP_TAGGED_TO_WRAP = 0x03,
	DDP_TAGGED_INV_DDP_VER = 0x04,
	DDP_UNTAGGED_INV_QN = 0x01,
	DDP_UNTAGGED_INV_MSN_NO_BUF = 0x02,
	DDP_UNTAGGED_INV_MSN_RANGE = 0x03,
	DDP_UNTAGGED_INV_MO = 0x04,
	DDP_UNTAGGED_INV_TOO_LONG = 0x05,
	DDP_UNTAGGED_INV_DDP_VER = 0x06,
};

enum zxdh_term_mpa_errors {
	MPA_CLOSED = 0x01,
	MPA_CRC = 0x02,
	MPA_MARKER = 0x03,
	MPA_REQ_RSP = 0x04,
};

/* AEQ work mode enumeration */
enum zxdh_aeq_work_mode {
	ZXDH_AEQ_MODE_INTERRUPT = 0,  /* Interrupt mode (default) */
	ZXDH_AEQ_MODE_POLLING = 1,    /* Polling mode */
};

enum zxdh_qp_event_type {
	ZXDH_QP_EVENT_CATASTROPHIC,
	ZXDH_QP_EVENT_ACCESS_ERR,
	ZXDH_QP_EVENT_REQ_ERR,
};

enum zxdh_hw_stats_index {
	/* 32-bit */
	HW_STAT_DUPLICATE_REQUEST = 0,
	HW_STAT_NP_CNP_SENT,
	HW_STAT_NP_ECN_MARKED_ROCE_PACKETS,
	HW_STAT_OUT_OF_SEQUENCE,
	HW_STAT_PACKET_SEQ_ERR,
	HW_STAT_REQ_CQE_ERROR,
	HW_STAT_REQ_REMOTE_ACCESS_ERRORS,
	HW_STAT_REQ_REMOTE_INVALID_REQUEST,
	HW_STAT_REQ_REMOTE_OPERATION_ERRORS,
	HW_STAT_REQ_LOCAL_LENGTH_ERROR,
	HW_STAT_RESP_CQE_ERROR,
	HW_STAT_RESP_REMOTE_ACCESS_ERRORS,
	HW_STAT_RESP_REMOTE_INVALID_REQUEST,
	HW_STAT_RESP_REMOTE_OPERATION_ERRORS,
	HW_STAT_RESP_RNR_NAK,
	HW_STAT_RNR_NAK_RETRY_ERR,
	HW_STAT_RP_CNP_HANDLED,
	HW_STAT_RX_READ_REQUESTS,
	HW_STAT_RX_WRITE_REQUESTS,
	HW_STAT_RX_ICRC_ENCAPSULATED,
	HW_STAT_ROCE_SLOW_RESTART_CNPS,
	HW_STAT_RDMA_TX_PKTS,
	HW_STAT_RDMA_TX_BYTES,
	HW_STAT_RDMA_RX_PKTS,
	HW_STAT_RDMA_RX_BYTES,
	ZXDH_HW_STAT_INDEX_MAX,
};

enum zxdh_ib_hw_stats_index {
	IB_STAT_SYMBOL_ERROR = 0,
	IB_STAT_LINK_ERROR_RECOVERY,
	IB_STAT_LINK_DOWNED,
	IB_STAT_PORT_RCV_ERRORS,
	IB_STAT_PORT_RCV_REMPHYS_ERRORS,
	IB_STAT_PORT_RCV_SWITCH_RELAY_ERRORS,
	IB_STAT_PORT_XMIT_DISCARDS,
	IB_STAT_PORT_XMIT_CONTRAINT_ERRORS,
	IB_STAT_PORT_XMIT_WAIT,
	IB_STAT_PORT_RCV_CONSTRAINT_ERRORS,
	IB_STAT_LINK_OVERRUN_ERRORS,
	IB_STAT_VL15_DROPPED,
	IB_STAT_PORT_XMIT_DATA,
	IB_STAT_PORT_RCV_DATA,
	IB_STAT_PORT_XMIT_PACKETS,
	IB_STAT_PORT_RCV_PACKETS,
	IB_STAT_PORT_UNICAST_XMIT_PACKETS,
	IB_STAT_PORT_UNICAST_RCV_PACKETS,
	IB_STAT_PORT_MULTICAST_XMIT_PACKETS,
	IB_STAT_PORT_MULTICAST_RCV_PACKETS,
	IB_STAT_LOCAL_LINK_INTEGRITY_ERRORS,
	IB_STAT_INDEX_MAX,
};

enum zxdh_module_type {
	ZXDH_IB_STAT = 0,
	ZXDH_RDMA_STAT,
};

#define ZXDH_MIN_FEATURES 2

enum zxdh_feature_type {
	ZXDH_FEATURE_FW_INFO = 0,
	ZXDH_HW_VERSION_INFO = 1,
	ZXDH_QSETS_MAX = 26,
	ZXDH_MAX_FEATURES, /* Must be last entry */
};

enum zxdh_sched_prio_type {
	ZXDH_PRIO_WEIGHTED_RR = 1,
	ZXDH_PRIO_STRICT = 2,
	ZXDH_PRIO_WEIGHTED_STRICT = 3,
};

enum zxdh_vm_vf_type {
	ZXDH_VF_TYPE = 0,
	ZXDH_VM_TYPE,
	ZXDH_PF_TYPE,
};

enum zxdh_cqp_hmc_profile {
	ZXDH_HMC_PROFILE_DEFAULT = 1,
	ZXDH_HMC_PROFILE_FAVOR_VF = 2,
	ZXDH_HMC_PROFILE_EQUAL = 3,
};

enum zxdh_quad_entry_type {
	ZXDH_QHASH_TYPE_TCP_ESTABLISHED = 1,
	ZXDH_QHASH_TYPE_TCP_SYN,
	ZXDH_QHASH_TYPE_UDP_UNICAST,
	ZXDH_QHASH_TYPE_UDP_MCAST,
	ZXDH_QHASH_TYPE_ROCE_MCAST,
	ZXDH_QHASH_TYPE_ROCEV2_HW,
};

enum zxdh_quad_hash_manage_type {
	ZXDH_QHASH_MANAGE_TYPE_DELETE = 0,
	ZXDH_QHASH_MANAGE_TYPE_ADD,
	ZXDH_QHASH_MANAGE_TYPE_MODIFY,
};

enum zxdh_syn_rst_handling {
	ZXDH_SYN_RST_HANDLING_HW_TCP_SECURE = 0,
	ZXDH_SYN_RST_HANDLING_HW_TCP,
	ZXDH_SYN_RST_HANDLING_FW_TCP_SECURE,
	ZXDH_SYN_RST_HANDLING_FW_TCP,
};

enum zxdh_queue_type {
	ZXDH_QUEUE_TYPE_SQ_RQ = 0,
	ZXDH_QUEUE_TYPE_CQP,
};

enum zxdh_cqe_source_type {
	ZXDH_CQE_SOURCE_OTHERQP = 0,
	ZXDH_CQE_SOURCE_CQP,
};

struct zxdh_sc_dev;
struct zxdh_vsi_pestat;
struct zxdh_src_copy_dest;

struct zxdh_dcqcn_cc_params {
	u8 cc_cfg_valid;
	u8 min_dec_factor;
	u8 min_rate;
	u8 dcqcn_f;
	u16 rai_factor;
	u16 hai_factor;
	u16 dcqcn_t;
	u32 dcqcn_b;
	u32 rreduce_mperiod;
};

struct zxdh_cqp_init_info {
	u64 cqp_compl_ctx;
	u64 host_ctx_pa;
	u64 sq_pa;
	struct zxdh_sc_dev *dev;
	struct zxdh_cqp_quanta *sq;
	struct zxdh_dcqcn_cc_params dcqcn_params;
	__le64 *host_ctx;
	u64 *scratch_array;
	u32 sq_size;
	u16 hw_maj_ver;
	u16 hw_min_ver;
	u8 struct_ver;
	u8 hmc_profile;
	u8 ena_vf_count;
	u8 ceqs_per_vf;
	u8 en_datacenter_tcp : 1;
	u8 disable_packed : 1;
	u8 rocev2_rto_policy : 1;
	u8 en_rem_endpoint_trk : 1;
	enum zxdh_protocol_used protocol_used;
};

struct zxdh_terminate_hdr {
	u8 layer_etype;
	u8 error_code;
	u8 hdrct;
	u8 rsvd;
};

struct zxdh_cqp_sq_wqe {
	__le64 buf[ZXDH_CQP_WQE_SIZE];
};

struct zxdh_sc_aeqe {
	__le64 buf[ZXDH_AEQE_SIZE];
};

struct zxdh_ceqe {
	__le64 buf[ZXDH_CEQE_SIZE];
};

struct zxdh_cqp_ctx {
	__le64 buf[ZXDH_CQP_CTX_SIZE];
};

struct zxdh_cq_shadow_area {
	__le64 buf[ZXDH_SHADOW_AREA_SIZE];
};

struct zxdh_dev_hw_stats_offsets {
	u32 stats_offset[ZXDH_HW_STAT_INDEX_MAX];
};

struct zxdh_dev_hw_stats {
	u64 stats_val[ZXDH_GATHER_STATS_BUF_SIZE / sizeof(u64)];
};

struct zxdh_gather_stats {
	u64 val[ZXDH_GATHER_STATS_BUF_SIZE / sizeof(u64)];
};

struct zxdh_hw_stat_map {
	u16 byteoff;
	u8 bitoff;
	u64 bitmask;
};

struct zxdh_stats_gather_info {
	u8 use_hmc_fcn_index : 1;
	u8 use_stats_inst : 1;
	u16 hmc_fcn_index;
	u8 stats_inst_index;
	struct zxdh_dma_mem stats_buff_mem;
	void *gather_stats_va;
	void *last_gather_stats_va;
};

struct zxdh_vsi_pestat {
	struct zxdh_hw *hw;
	struct zxdh_dev_hw_stats hw_stats;
	struct zxdh_stats_gather_info gather_info;
	struct timer_list stats_timer;
	struct zxdh_sc_vsi *vsi;
	spinlock_t lock; /* rdma stats lock */
};

struct zxdh_hw {
	u8 __iomem *hw_addr;
	u8 __iomem *ext_hw_addr;
	u8 __iomem *priv_hw_addr;
	u8 __iomem *pci_hw_addr;
	u8 __iomem *hbm_info_page_addr;
	struct device *device;
	struct zxdh_hmc_info hmc;
};

struct zxdh_pfpdu {
	struct list_head rxlist;
	u32 rcv_nxt;
	u32 fps;
	u32 max_fpdu_data;
	u32 nextseqnum;
	u32 rcv_start_seq;
	u8 mode : 1;
	u8 mpa_crc_err : 1;
	u8 marker_len;
	u64 total_ieq_bufs;
	u64 fpdu_processed;
	u64 bad_seq_num;
	u64 crc_err;
	u64 no_tx_bufs;
	u64 tx_err;
	u64 out_of_order;
	u64 pmode_count;
	struct zxdh_sc_ah *ah;
	struct zxdh_puda_buf *ah_buf;
	spinlock_t lock; /* fpdu processing lock */
	struct zxdh_puda_buf *lastrcv_buf;
};

struct zxdh_sc_pd {
	struct zxdh_sc_dev *dev;
	u32 pd_id;
	int abi_ver;
};

struct zxdh_cqp_quanta {
	__le64 elem[ZXDH_CQP_WQE_SIZE];
};

struct zxdh_sc_cqp {
	u32 size;
	u64 sq_pa;
	u64 host_ctx_pa;
	void *back_cqp;
	struct zxdh_sc_dev *dev;
	int (*process_cqp_sds)(struct zxdh_sc_dev *dev,
			       struct zxdh_update_sds_info *info);
	int (*process_config_pte_table)(struct zxdh_sc_dev *dev,
					struct zxdh_src_copy_dest src_dest);
	struct zxdh_ring sq_ring;
	struct zxdh_cqp_quanta *sq_base;
	struct zxdh_dcqcn_cc_params dcqcn_params;
	__le64 *host_ctx;
	u64 *scratch_array;
	u32 cqp_id;
	u32 sq_size;
	u32 hw_sq_size;
	u16 hw_maj_ver;
	u16 hw_min_ver;
	u8 struct_ver;
	u8 polarity;
	u8 hmc_profile;
	u8 ena_vf_count;
	u8 timeout_count;
	u8 ceqs_per_vf;
	u8 en_datacenter_tcp : 1;
	u8 disable_packed : 1;
	u8 rocev2_rto_policy : 1;
	u8 en_rem_endpoint_trk : 1;
	u8 state_cfg : 1; // C_RDMA_CQP_CONTEXT_0 [31]
	enum zxdh_protocol_used protocol_used;
};

struct zxdh_sc_aeq {
	u32 size;
	u64 aeq_elem_pa;
	struct zxdh_sc_dev *dev;
	struct zxdh_sc_aeqe *aeqe_base;
	void *pbl_list;
	u32 elem_cnt;
	struct zxdh_ring aeq_ring;
	u8 pbl_chunk_size;
	u32 first_pm_pbl_idx;
	u32 msix_idx;
	u8 polarity;
	u8 get_polarity_flag;
	u8 virtual_map : 1;
};

struct zxdh_sc_ceq {
	u32 size;
	u64 ceq_elem_pa;
	struct zxdh_sc_dev *dev;
	struct zxdh_ceqe *ceqe_base;
	void *pbl_list;
	bool valid_ceq;
	u32 ceq_id;
	u32 ceq_index;
	u32 elem_cnt;
	u32 log2_elem_size;
	struct zxdh_ring ceq_ring;
	u8 pbl_chunk_size;
	u8 tph_val;
	u32 first_pm_pbl_idx;
	u32 msix_idx;
	u8 polarity;
	struct zxdh_sc_vsi *vsi;
	struct zxdh_sc_cq **reg_cq;
	u32 reg_cq_size;
	spinlock_t req_cq_lock; /* protect access to reg_cq array */
	u8 virtual_map : 1;
	u8 tph_en : 1;
	u8 itr_no_expire : 1;
};

struct zxdh_sc_cq {
	struct zxdh_cq_uk cq_uk;
	u64 cq_pa;
	u64 shadow_area_pa;
	struct zxdh_sc_dev *dev;
	struct zxdh_sc_dev *ext_dev;
	struct zxdh_sc_vsi *vsi;
	void *pbl_list;
	void *back_cq;
	u32 ceq_id;
	u32 ceq_index;
	u32 shadow_read_threshold;
	u8 pbl_chunk_size;
	u8 cq_type;
	u8 tph_val;
	u32 first_pm_pbl_idx;
	u8 ceqe_mask : 1;
	u8 virtual_map : 1;
	u8 ceq_id_valid : 1;
	u8 tph_en;
	u8 cq_st;
	u16 is_in_list_cnt;
	u16 cq_max;
	u16 cq_period;
	u8 scqe_break_moderation_en : 1;
	u8 cq_overflow_locked_flag : 1;
};

struct zxdh_sc_qp {
	struct zxdh_qp_uk qp_uk;
	u64 sq_pa;
	u64 rq_pa;
	u64 hw_host_ctx_pa;
	u64 shadow_area_pa;
	struct zxdh_sc_dev *dev;
	struct zxdh_sc_dev *ext_dev;
	struct zxdh_sc_vsi *vsi;
	struct zxdh_sc_pd *pd;
	struct zxdh_sc_srq *srq;
	__le64 *hw_host_ctx;
	void *llp_stream_handle;
	struct zxdh_pfpdu pfpdu;
	u32 ieq_qp;
	u32 bf_idx;
	u8 *q2_buf;
	u64 qp_compl_ctx;
	u32 qp_ctx_num;
	u16 qs_handle;
	u16 push_offset;
	u8 flush_wqes_count;
	u8 sq_tph_val;
	u8 rq_tph_val;
	u8 qp_state;
	u8 hw_sq_size;
	u8 hw_rq_size;
	u8 src_mac_addr_idx;

	u8 on_qoslist    : 1;
	u8 ieq_pass_thru : 1;
	u8 sq_tph_en     : 1;
	u8 rq_tph_en     : 1;
	u8 rcv_tph_en    : 1;
	u8 xmit_tph_en   : 1;
	u8 virtual_map   : 1;
	u8 flush_sq      : 1;

	u8 flush_rq      : 1;
	u8 sq_flush_code : 1;
	u8 rq_flush_code : 1;
	u8 is_nvmeof_ioq : 1;
	u8 is_nvmeof_tgt : 1;
	u8 nvme_flush_qp : 1;
	u8 is_credit_en  : 1;
	u8 resv          : 1;

	u32 nvmeof_qid;
	enum zxdh_flush_opcode flush_code;
	enum zxdh_qp_event_type event_type;
	u8 term_flags;
	u8 user_pri;
	u8 pri_index;
	struct list_head list;
	u8 is_srq;
	u32 tx_last_ack_psn;
	u32 aeq_entry_err_last_psn;
	u32 aeq_retry_err_last_psn;
	u8 entry_err_cnt;
	u8 retry_err_cnt;
	u8 use_blue_flame_flag;
	u8 modify_to_ets;
};

struct zxdh_stats_inst_info {
	bool use_hmc_fcn_index;
	u8 hmc_fn_id;
	u8 stats_idx;
};

struct zxdh_up_info {
	u8 map[8];
	u8 cnp_up_override;
	u8 hmc_fcn_idx;
	u8 use_vlan : 1;
	u8 use_cnp_up_override : 1;
};

#define ZXDH_MAX_WS_NODES 0x3FF
#define ZXDH_WS_NODE_INVALID 0xFFFF

struct zxdh_ws_node_info {
	u16 id;
	u16 vsi;
	u16 parent_id;
	u16 qs_handle;
	u8 type_leaf : 1;
	u8 enable : 1;
	u8 prio_type;
	u8 tc;
	u8 weight;
};

#define ZXDH_VCHNL_MAX_VF_MSG_SIZE 512
#define ZXDH_LEAF_DEFAULT_REL_BW 64
#define ZXDH_PARENT_DEFAULT_REL_BW 1

struct zxdh_qos {
	struct list_head qplist;
	struct mutex qos_mutex; /* protect QoS attributes per QoS level */
	u64 lan_qos_handle;
	u32 l2_sched_node_id;
	u32 qp_cnt;
	u16 qs_handle;
	u8 pri_allocated;
	u8 pri_index;
	u8 traffic_class;
	u8 rel_bw;
	u8 prio_type;
	bool valid;
};

struct zxdh_config_check {
	u8 config_ok : 1;
	u8 lfc_set : 1;
	u8 pfc_set : 1;
	u8 traffic_class;
	u16 qs_handle;
};

struct zxdh_vfdev {
	struct zxdh_sc_dev *pf_dev;
	struct zxdh_sc_vsi *vf_vsi;
	u8 *hmc_info_mem;
	u8 vf_msg_buf[ZXDH_VCHNL_MAX_VF_MSG_SIZE];
	struct zxdh_hmc_info hmc_info;
	u32 max_ceqs;
	u32 pbleq_unallocated_pble;
	u64 pbleq_fpm_base_addr;
	u64 pbleq_next_fpm_addr;
	u32 pblemr_unallocated_pble;
	u64 pblemr_fpm_base_addr;
	u64 pblemr_next_fpm_addr;

	refcount_t refcnt;
	u16 pmf_index;
	u16 vf_id;
	u16 vhca_id;
	u16 iw_vf_idx;
	u8 stats_initialized : 1;
	u8 pf_hmc_initialized : 1;
	u8 reset_en : 1;
	u8 port_vlan_en : 1;
};

#define ZXDH_INVALID_STATS_IDX 0xff
struct zxdh_sc_vsi {
	u16 vsi_idx;
	struct zxdh_sc_dev *dev;
	struct zxdh_vfdev *vf_dev;
	void *back_vsi;
	u32 ilq_count;
	struct zxdh_virt_mem ilq_mem;
	struct zxdh_puda_rsrc *ilq;
	u32 ieq_count;
	struct zxdh_virt_mem ieq_mem;
	struct zxdh_puda_rsrc *ieq;
	u32 exception_lan_q;
	u16 mtu;
	u16 vf_id;
	enum zxdh_vm_vf_type vm_vf_type;
	u8 stats_inst_alloc : 1;
	u8 tc_change_pending : 1;
	struct zxdh_vsi_pestat *pestat;
	atomic_t qp_suspend_reqs;
	int (*register_qset)(struct zxdh_sc_vsi *vsi,
			     struct zxdh_ws_node *tc_node);
	void (*unregister_qset)(struct zxdh_sc_vsi *vsi,
				struct zxdh_ws_node *tc_node);
	struct zxdh_config_check cfg_check[ZXDH_MAX_USER_PRIORITY];
	bool tc_print_warning[IEEE_8021QAZ_MAX_TCS];
	u8 qos_rel_bw;
	u8 qos_prio_type;
	u8 stats_idx;
	u8 dscp_map[ZXDH_DSCP_NUM_VAL];
	struct zxdh_qos qos[ZXDH_MAX_USER_PRIORITY];
	u64 hw_stats_regs[ZXDH_HW_STAT_INDEX_MAX];
	u8 dscp_mode : 1;
};
struct zxdh_srq_axi_ram {
	u32 __iomem *db;
	u32 __iomem *srql;
};

struct zxdh_ceq_axi {
	u32 __iomem *ceqe_axi_info;
	u32 __iomem *rpble_axi_info;
	u32 __iomem *lpble_axi_info;
	u32 __iomem *int_info;
};

struct zxdh_aeq_vhca_pfvf {
	u32 __iomem *aeq_msix_data;
	u32 __iomem *aeq_msix_config;
	u32 __iomem *aeq_root_axi_data;
	u32 __iomem *aeq_leaf_axi_data;
	u32 __iomem *aeq_wr_axi_data;
	u32 __iomem *aeq_aee_flag;
};

struct zxdh_hw_stats {
	u64 rdma_stats_entry[ZXDH_HW_STAT_INDEX_MAX];
};

struct zxdh_rdma_stats_get {
	u64 rdma_stats_entry[ZXDH_HW_STAT_INDEX_MAX];
	u8 rdma_stats_entry_sta[ZXDH_HW_STAT_INDEX_MAX];
};
struct zxdh_data_cap_sd {
	u64 data_cap_base;
	u64 data_len;
	u16 sd_cnt;
	struct zxdh_hmc_sd_entry *entry;
};

struct zxdh_pri_8k_index_info {
	u16 pri_8k_index_start;
	u16 pri_8k_index_cnt;
};

struct zxdh_sc_dev {
	struct list_head cqp_cmd_head; /* head of the CQP command list */
	spinlock_t cqp_lock; /* protect CQP list access */
	bool stats_idx_array[ZXDH_MAX_STATS_COUNT_GEN1];
	struct zxdh_dma_mem vf_fpm_query_buf[ZXDH_MAX_PE_ENA_VF_COUNT];
	struct zxdh_dma_mem clear_dpu_mem;
	struct zxdh_dma_mem nof_clear_dpu_mem;

	u64 pte_l2d_startpa; // PTE  L2D PA
	u32 pte_l2d_len; // PTE  L2D LEN
	struct zxdh_hw *hw;
	u8 __iomem *db_addr;
	u32 __iomem *wqe_alloc_db;
	u32 __iomem *cq_arm_db;
	u32 __iomem *aeq_alloc_db;
	u32 __iomem *cqp_db;
	u32 __iomem *cq_ack_db;
	u32 __iomem *ceq_itr_mask_db;
	u32 __iomem *aeq_itr_mask_db;
	u32 __iomem *hw_regs[ZXDH_MAX_REGS];
	u32 __iomem *ceq_itr_enable;
	// u32 __iomem *ceq_ep_addr[ZXDH_MAX_EP_NUM];
	// struct zxdh_ep_addr ceq_ep_addr[ZXDH_MAX_EP_NUM];
	struct zxdh_ceq_axi ceq_axi;
	u32 __iomem *aeq_itr_enable;
	u32 __iomem *aeq_tail_pointer;
	// struct zxdh_ep_addr aeq_ep_addr[ZXDH_MAX_EP_NUM];
	struct zxdh_aeq_vhca_pfvf aeq_vhca_pfvf;
	// struct zxdh_cm_aeq_axi aeq_axi;
	u32 ceq_itr; /* Interrupt throttle, usecs between interrupts: 0 disabled. 2 - 8160 */
	struct zxdh_srq_axi_ram srq_axi_ram;
	u64 hw_masks[ZXDH_MAX_MASKS];
	u8 hw_shifts[ZXDH_MAX_SHIFTS];
	struct zxdh_hw_stats stats_entry;
	bool ram_stats_first_call; /* first call to get_ram functions should return 0 silently */
	u64 hw_stats_regs[ZXDH_HW_STAT_INDEX_MAX];
	u64 hw_stats_vf_regs[ZXDH_HW_STAT_INDEX_MAX];
	u64 feature_info[ZXDH_MAX_FEATURES];
	u64 cqp_cmd_stats[ZXDH_MAX_CQP_OPS];
	struct zxdh_hw_attrs hw_attrs;
	struct zxdh_hmc_info *hmc_info;
	struct zxdh_vfdev *vf_dev[ZXDH_MAX_PE_ENA_VF_COUNT];
	u8 vf_recv_buf[ZXDH_VCHNL_MAX_VF_MSG_SIZE];
	u16 vf_recv_len;

	spinlock_t vf_dev_lock; /* sync vf_dev usage with async events like reset */
	struct workqueue_struct *vchnl_wq;
	struct zxdh_sc_cqp *cqp;
	struct zxdh_sc_aeq *aeq;
	struct zxdh_sc_ceq *ceq[ZXDH_CEQ_MAX_COUNT];
	struct zxdh_sc_cq *ccq;
	const struct zxdh_irq_ops *irq_ops;
	u32 max_ceqs;
	u32 base_qpn;
	u32 base_cqn;
	u32 base_srqn;
	u32 base_ceqn;
	u32 max_qp;
	u32 max_cq;
	u32 max_srq;
	struct zxdh_ws_node *ws_tree_root;
	struct mutex ws_mutex; /* ws tree mutex */
	struct zxdh_qos qos[ZXDH_MAX_USER_PRIORITY];
	struct zxdh_pri_8k_index_info pri_8k_index_info[ZXDH_MAX_USER_PRIORITY];
	u16 num_vfs;
	u16 active_vfs_num;
	u8 hmc_fn_id;
	u16 vf_id;
	u16 vhca_id;
	u16 vhca_id_pf;
	u16 cache_id;
	u8 ep_id;
	u8 hmc_epid;
	u8 soc_tx_rx_cqp_ind;
	u8 soc_tx_rx_cqp_axid;
	u8 soc_rdma_io_ind;
	u16 ird_size;
	u32 total_vhca;
    u16 vhca_gqp_start;
    u16 vhca_gqp_cnt;
    u16 vhca_8k_index_start;
    u16 vhca_8k_index_cnt;
    u16 vhca_ud_gqp;
    u16 vhca_ud_8k_index;
	u64 nof_ioq_ddr_addr;
	u8 chip_version;
	u64 l2d_smmu_addr;
	u32 l2d_smmu_l2_offset;
	u32 s_udV8NumL2Pta;
	u8 vchnl_up : 1;
	u8 ceq_valid : 1;
	u8 privileged : 1;
	u8 double_vlan_en : 1;
	u8 hmc_use_dpu_ddr : 1;
	u8 np_mode_low_lat : 1;
	u8 vf_mb_init : 1;
	struct mutex vchnl_mutex;
	u8 pci_rev;
	int (*ws_add)(struct zxdh_sc_vsi *vsi, u8 user_pri);
	void (*ws_remove)(struct zxdh_sc_vsi *vsi, u8 user_pri);
	void (*ws_reset)(struct zxdh_sc_vsi *vsi);
	struct zxdh_hmc_obj_manage hmc_pf_manager_info;
	struct smmu_pte_address *pte_address;
	struct zxdh_vf_hmc_obj_info vf_hmcobjinfo[256];
	struct zxdh_data_cap_sd data_cap_sd;
	u8 ceq_0_ok;
	u8 ceq_interrupt;
	u8 tx_stop_on_aeq : 1;
	u8 rx_stop_on_aeq : 1;
	u8 flag3 : 1;
	u8 flag4 : 1;
	u8 flag5 : 1;
	u8 driver_load;
	u8 flr_query;
	u8 fw_np_mac;
	u8 vf_flr_cap;
	bool is_blue_flame;
};

struct zxdh_modify_cq_info {
	u64 cq_pa;
	struct zxdh_cqe *cq_base;
	u32 cq_size;
	u32 shadow_read_threshold;
	u8 pbl_chunk_size;
	u32 first_pm_pbl_idx;
	u8 virtual_map : 1;
	u8 cq_resize : 1;
};

struct zxdh_create_qp_info {
	u8 ord_valid : 1;
	u8 tcp_ctx_valid : 1;
	u8 cq_num_valid : 1;
	u8 arp_cache_idx_valid : 1;
	u8 mac_valid : 1;
	bool force_lpb;
	u8 next_iwarp_state;
};

struct zxdh_modify_qp_info {
	u64 rx_win0;
	u64 rx_win1;
	u64 qpc_tx_mask_low;
	u64 qpc_tx_mask_high;
	u64 qpc_rx_mask_low;
	u64 qpc_rx_mask_high;
	u16 new_mss;
	u8 next_iwarp_state;
	u8 curr_iwarp_state;
	u8 termlen;
	u16 udp_sport;
	u8 ord_valid : 1;
	u8 tcp_ctx_valid : 1;
	u8 udp_ctx_valid : 1;
	u8 cq_num_valid : 1;
	u8 arp_cache_idx_valid : 1;
	u8 reset_tcp_conn : 1;
	u8 remove_hash_idx : 1;
	u8 dont_send_term : 1;
	u8 dont_send_fin : 1;
	u8 cached_var_valid : 1;
	u8 mss_change : 1;
	u8 force_lpb : 1;
	u8 mac_valid : 1;
};

struct zxdh_modify_srq_info {
	int limit;
};

struct zxdh_create_srq_info {
	u8 state;
};

struct zxdh_destroy_srq_info {
	u8 state;
};

struct zxdh_ccq_cqe_info {
	struct zxdh_sc_cqp *cqp;
	u64 scratch;
	u64 op_ret_val;
	u16 maj_err_code;
	u16 min_err_code;
	u8 op_code;
	u8 mailbox_cqe;
	__le64 addrbuf[5];
	bool error;
};

struct zxdh_qos_tc_info {
	u64 tc_ctx;
	u8 rel_bw;
	u8 prio_type;
	u8 egress_virt_up;
	u8 ingress_virt_up;
};

struct zxdh_l2params {
	struct zxdh_qos_tc_info tc_info[ZXDH_MAX_USER_PRIORITY];
	u32 num_apps;
	u16 qs_handle_list[ZXDH_MAX_USER_PRIORITY];
	u16 mtu;
	u8 up2tc[ZXDH_MAX_USER_PRIORITY];
	u8 dscp_map[ZXDH_DSCP_NUM_VAL];
	u8 num_tc;
	u8 vsi_rel_bw;
	u8 vsi_prio_type;
	u8 mtu_changed : 1;
	u8 tc_changed : 1;
	u8 dscp_mode : 1;
};

struct zxdh_vsi_init_info {
	struct zxdh_sc_dev *dev;
	void *back_vsi;
	struct zxdh_l2params *params;
	u16 exception_lan_q;
	u16 pf_data_vsi_num;
	enum zxdh_vm_vf_type vm_vf_type;
	int (*register_qset)(struct zxdh_sc_vsi *vsi,
			     struct zxdh_ws_node *tc_node);
	void (*unregister_qset)(struct zxdh_sc_vsi *vsi,
				struct zxdh_ws_node *tc_node);
};

struct zxdh_vsi_stats_info {
	struct zxdh_vsi_pestat *pestat;
	u8 fcn_id;
	bool alloc_stats_inst;
};

struct zxdh_device_init_info {
	struct zxdh_hw *hw;
	void __iomem *bar0;
	struct workqueue_struct *vchnl_wq;
	u16 max_vfs;
	u8 hmc_fn_id;
	bool privileged;
};

struct zxdh_ceq_init_info {
	u64 ceqe_pa;
	struct zxdh_sc_dev *dev;
	u64 *ceqe_base;
	void *pbl_list;
	u32 elem_cnt;
	u32 log2_elem_size;
	u32 ceq_id;
	u32 ceq_index;
	u8 virtual_map : 1;
	u8 tph_en : 1;
	u8 itr_no_expire : 1;
	u8 pbl_chunk_size;
	u8 tph_val;
	u32 first_pm_pbl_idx;
	struct zxdh_sc_vsi *vsi;
	struct zxdh_sc_cq **reg_cq;
	u32 reg_cq_idx;
	u32 msix_idx;
};

struct zxdh_aeq_init_info {
	u64 aeq_elem_pa;
	struct zxdh_sc_dev *dev;
	u32 *aeqe_base;
	void *pbl_list;
	u32 elem_cnt;
	bool virtual_map;
	u8 pbl_chunk_size;
	u32 first_pm_pbl_idx;
	u32 msix_idx;
};

struct zxdh_ccq_init_info {
	u64 cq_pa;
	u64 shadow_area_pa;
	struct zxdh_sc_dev *dev;
	struct zxdh_cqe *cq_base;
	__le64 *shadow_area;
	void *pbl_list;
	u32 num_elem;
	u32 ceq_id;
	u32 ceq_index;
	u32 cq_num;
	u32 shadow_read_threshold;
	u8 ceqe_mask : 1;
	u8 ceq_id_valid : 1;
	u8 cqe_size;
	u8 virtual_map : 1;
	u8 tph_en : 1;
	u8 tph_val;
	u8 pbl_chunk_size;
	u16 cq_max;
	u16 cq_period;
	u8 scqe_break_moderation_en : 1;
	u8 cq_st;
	u16 is_in_list_cnt;
	u32 first_pm_pbl_idx;
	struct zxdh_sc_vsi *vsi;
};

struct zxdh_udp_offload_info {
	u8 ipv4 : 1;
	u8 insert_vlan_tag : 1;
	u8 ttl;
	u8 tos;
	u16 src_port;
	u16 dst_port;
	u32 dest_ip_addr[4];
	u16 pmtu;
	u16 vlan_tag;
	u8 dest_mac[ETH_ALEN];
	u32 flow_label;
	u8 udp_state;
	u32 psn_nxt;
	u32 lsn;
	u32 epsn;
	u32 psn_max;
	u32 psn_una;
	u32 local_ipaddr[4];
	u32 cwnd;
	u8 rexmit_thresh;
	u8 rnr_nak_thresh;
	u8 timeout;
	u8 min_rnr_timer;
};

struct zxdh_roce_offload_info {
	u16 p_key;
	u16 err_rq_idx;
	u32 qkey;
	u32 dest_qp;
	u32 local_qp;
	u8 roce_tver;
	u8 ack_credits;
	u8 err_rq_idx_valid;
	u32 pd_id;
	u16 ord_size;
	u16 ird_size;
	u8 is_qp1 : 1;
	u8 udprivcq_en : 1;
	u8 dcqcn_en : 1;
	u8 ecn_en : 1;
	u8 rcv_no_icrc : 1;
	u8 wr_rdresp_en : 1;
	u8 bind_en : 1;
	u8 fast_reg_en : 1;
	u8 priv_mode_en : 1;
	u8 rd_en : 1;
	u8 timely_en : 1;
	u8 dctcp_en : 1;
	u8 fw_cc_enable : 1;
	u8 use_stats_inst : 1;
	u16 t_high;
	u16 t_low;
	u8 last_byte_sent;
	u8 mac_addr[ETH_ALEN];
	u8 rtomin;
};

struct zxdh_iwarp_offload_info {
	u16 rcv_mark_offset;
	u16 snd_mark_offset;
	u8 ddp_ver;
	u8 rdmap_ver;
	u8 iwarp_mode;
	u16 err_rq_idx;
	u32 pd_id;
	u16 ord_size;
	u16 ird_size;
	u8 ib_rd_en : 1;
	u8 align_hdrs : 1;
	u8 rcv_no_mpa_crc : 1;
	u8 err_rq_idx_valid : 1;
	u8 snd_mark_en : 1;
	u8 rcv_mark_en : 1;
	u8 wr_rdresp_en : 1;
	u8 bind_en : 1;
	u8 fast_reg_en : 1;
	u8 priv_mode_en : 1;
	u8 rd_en : 1;
	u8 timely_en : 1;
	u8 use_stats_inst : 1;
	u8 ecn_en : 1;
	u8 dctcp_en : 1;
	u16 t_high;
	u16 t_low;
	u8 last_byte_sent;
	u8 mac_addr[ETH_ALEN];
	u8 rtomin;
};

struct zxdh_tcp_offload_info {
	u8 ipv4 : 1;
	u8 no_nagle : 1;
	u8 insert_vlan_tag : 1;
	u8 time_stamp : 1;
	u8 drop_ooo_seg : 1;
	u8 avoid_stretch_ack : 1;
	u8 wscale : 1;
	u8 ignore_tcp_opt : 1;
	u8 ignore_tcp_uns_opt : 1;
	u8 cwnd_inc_limit;
	u8 dup_ack_thresh;
	u8 ttl;
	u8 src_mac_addr_idx;
	u8 tos;
	u16 src_port;
	u16 dst_port;
	u32 dest_ip_addr[4];
	//u32 dest_ip_addr0;
	//u32 dest_ip_addr1;
	//u32 dest_ip_addr2;
	//u32 dest_ip_addr3;
	u32 snd_mss;
	u16 syn_rst_handling;
	u16 vlan_tag;
	u16 arp_idx;
	u32 flow_label;
	u8 tcp_state;
	u8 snd_wscale;
	u8 rcv_wscale;
	u32 time_stamp_recent;
	u32 time_stamp_age;
	u32 snd_nxt;
	u32 snd_wnd;
	u32 rcv_nxt;
	u32 rcv_wnd;
	u32 snd_max;
	u32 snd_una;
	u32 srtt;
	u32 rtt_var;
	u32 ss_thresh;
	u32 cwnd;
	u32 snd_wl1;
	u32 snd_wl2;
	u32 max_snd_window;
	u8 rexmit_thresh;
	u32 local_ipaddr[4];
};

struct zxdh_qp_host_ctx_info {
	u64 qp_compl_ctx;
	union {
		struct zxdh_tcp_offload_info *tcp_info;
		struct zxdh_udp_offload_info *udp_info;
	};
	union {
		struct zxdh_iwarp_offload_info *iwarp_info;
		struct zxdh_roce_offload_info *roce_info;
	};
	u32 send_cq_num;
	u32 rcv_cq_num;
	u32 rem_endpoint_idx;
	u8 stats_idx;
	u8 srq_valid : 1;
	u8 tcp_info_valid : 1;
	u8 iwarp_info_valid : 1;
	u8 stats_idx_valid : 1;
	u8 user_pri;
	u8 next_qp_state;
	u8 use_srq : 1;
};

struct zxdh_aeqe_info {
	u64 compl_ctx;
	u32 qp_cq_id;
	u16 ae_id;
	u16 wqe_idx;
	u8 tcp_state;
	u8 iwarp_state;
	u8 qp : 1;
	u8 cq : 1;
	u8 sq : 1;
	u8 rq : 1;
	u8 srq : 1;
	u8 in_rdrsp_wr : 1;
	u8 out_rdrsp : 1;
	u8 aeqe_overflow : 1;
	u8 q2_data_written;
	u8 ae_src;
	u32 vhca_id;
};

struct zxdh_aeq_report_info {
	u32 retry_timeout_cnt;
	u32 cq_flow_qp_cnt;
};

struct zxdh_allocate_stag_info {
	u64 total_len;
	u64 first_pm_pbl_idx;
	u32 chunk_size;
	u32 stag_key;
	u32 stag_idx;
	u32 page_size;
	u32 pd_id;
	u16 access_rights;
	u8 remote_access : 1;
	u8 use_hmc_fcn_index : 1;
	u8 use_pf_rid : 1;
	u16 hmc_fcn_index;
};

struct zxdh_mw_alloc_info {
	u32 mw_stag_index;
	u32 page_size;
	u32 pd_id;
	u8 remote_access : 1;
	u8 mw_wide : 1;
	u8 mw1_bind_dont_vldt_key : 1;
};

struct zxdh_reg_ns_stag_info {
	u64 reg_addr_pa;
	u64 va;
	u64 total_len;
	u32 page_size;
	u32 chunk_size;
	u32 first_pm_pbl_index;
	enum zxdh_addressing_type addr_type;
	zxdh_stag_index stag_idx;
	u16 access_rights;
	u32 pd_id;
	zxdh_stag_key stag_key;
	u8 use_hmc_fcn_index : 1;
	u16 hmc_fcn_index;
	u8 use_pf_rid : 1;
};

struct zxdh_fast_reg_stag_info {
	u64 wr_id;
	u64 reg_addr_pa;
	u64 fbo;
	void *va;
	u64 total_len;
	u32 page_size;
	u32 chunk_size;
	u32 first_pm_pbl_index;
	enum zxdh_addressing_type addr_type;
	zxdh_stag_index stag_idx;
	u16 access_rights;
	u32 pd_id;
	zxdh_stag_key stag_key;
	u8 local_fence : 1;
	u8 read_fence : 1;
	u8 signaled : 1;
	u8 push_wqe : 1;
	u8 use_hmc_fcn_index : 1;
	u16 hmc_fcn_index;
	u8 use_pf_rid : 1;
	u8 defer_flag : 1;
};

struct zxdh_dealloc_stag_info {
	u32 stag_idx;
	u32 pd_id;
	u8 mr : 1;
	u8 dealloc_pbl : 1;
};

struct zxdh_register_shared_stag {
	u64 va;
	enum zxdh_addressing_type addr_type;
	zxdh_stag_index new_stag_idx;
	zxdh_stag_index parent_stag_idx;
	u32 access_rights;
	u32 pd_id;
	u32 page_size;
	zxdh_stag_key new_stag_key;
};

struct zxdh_qp_init_info {
	struct zxdh_qp_uk_init_info qp_uk_init_info;
	struct zxdh_sc_pd *pd;
	struct zxdh_sc_vsi *vsi;
	struct zxdh_sc_dev *dev;
	struct zxdh_sc_dev *ext_dev;
	__le64 *host_ctx;
	u8 *q2;
	u64 sq_pa;
	u64 rq_pa;
	u64 host_ctx_pa;
	u64 q2_pa;
	u64 shadow_area_pa;
	u8 sq_tph_val;
	u8 rq_tph_val;
	u8 sq_tph_en : 1;
	u8 rq_tph_en : 1;
	u8 rcv_tph_en : 1;
	u8 xmit_tph_en : 1;
	u8 virtual_map : 1;
};

struct zxdh_cq_init_info {
	struct zxdh_sc_dev *dev;
	u64 cq_base_pa;
	u64 shadow_area_pa;
	u32 ceq_id;
	u32 ceq_index;
	u32 shadow_read_threshold;
	u8 pbl_chunk_size;
	u32 first_pm_pbl_idx;
	u8 virtual_map : 1;
	u8 ceqe_mask : 1;
	u8 ceq_id_valid : 1;
	u8 tph_en : 1;
	u8 tph_val;
	u8 type;
	struct zxdh_cq_uk_init_info cq_uk_init_info;
	struct zxdh_sc_vsi *vsi;
};

struct zxdh_upload_context_info {
	u64 buf_pa;
	u32 qp_id;
	u8 qp_type;
	u8 freeze_qp : 1;
	u8 raw_format : 1;
};

struct zxdh_local_mac_entry_info {
	u8 mac_addr[6];
	u16 entry_idx;
};

struct zxdh_add_arp_cache_entry_info {
	u8 mac_addr[ETH_ALEN];
	u32 reach_max;
	u16 arp_index;
	bool permanent;
};

struct zxdh_apbvt_info {
	u16 port;
	bool add;
};

struct zxdh_qhash_table_info {
	struct zxdh_sc_vsi *vsi;
	enum zxdh_quad_hash_manage_type manage;
	enum zxdh_quad_entry_type entry_type;
	u8 vlan_valid : 1;
	u8 ipv4_valid : 1;
	u8 mac_addr[ETH_ALEN];
	u16 vlan_id;
	u8 user_pri;
	u32 qp_num;
	u32 dest_ip[4];
	u32 src_ip[4];
	u16 dest_port;
	u16 src_port;
};

struct zxdh_cqp_manage_push_page_info {
	u32 push_idx;
	u16 qs_handle;
	u8 free_page;
	u8 push_page_type;
};

struct zxdh_qp_flush_info {
	u16 sq_minor_code;
	u16 sq_major_code;
	u16 rq_minor_code;
	u16 rq_major_code;
	u16 ae_code;
	u8 ae_src;
	bool sq : 1;
	bool rq : 1;
	u8 userflushcode : 1;
	u8 generate_ae : 1;
};

struct zxdh_gen_ae_info {
	u16 ae_code;
	u8 ae_src;
};

struct zxdh_cqp_timeout {
	u64 compl_cqp_cmds;
	u32 count;
};

struct zxdh_src_copy_dest {
	u64 src;
	u32 len;
	u64 dest;
};

struct zxdh_dam_read_bycqe {
	u8 num;
	u8 bitwidth; // 0:64   1:32
	u8 valuetype;
	__le64 addrbuf[5];
};

struct zxdh_dma_write64_date {
	u8 num;
	__le64 addrbuf[3];
	__le64 databuf[3];
};

struct zxdh_dma_write32_date {
	u8 num;
	u8 inter_sour_sel;
	u8 need_inter;
	__le64 addrbuf[4];
	__le64 databuf[4];
};

struct zxdh_path_index {
	u16 vhca_id;
	u8 obj_id;
	u8 waypartion;
	u8 path_select;
	u8 inter_select;
};

struct zxdh_mailboxhead_data {
	u64 msg0;
	u64 msg1;
	u64 msg2;
	u64 msg3;
	u64 msg4;
};

struct zxdh_irq_ops {
	void (*zxdh_cfg_aeq)(struct zxdh_sc_dev *dev, u32 irq_idx);
	void (*zxdh_ceq_en_irq)(struct zxdh_sc_dev *dev, u32 idx);
	void (*zxdh_aeq_en_irq)(struct zxdh_sc_dev *dev, bool enable);
};

u32 zxdh_num_to_log(u32 size_num);

void zxdh_sc_ccq_arm(struct zxdh_sc_cq *ccq);
int zxdh_sc_ccq_create(struct zxdh_sc_cq *ccq, u64 scratch, bool post_sq);
int zxdh_sc_ccq_destroy(struct zxdh_sc_cq *ccq, u64 scratch, bool post_sq);
int zxdh_sc_ccq_get_cqe_info(struct zxdh_sc_cq *ccq,
			     struct zxdh_ccq_cqe_info *info, bool *error_flag);
int zxdh_sc_ccq_init(struct zxdh_sc_cq *ccq, struct zxdh_ccq_init_info *info);

int zxdh_sc_cceq_create(struct zxdh_sc_ceq *ceq, u64 scratch);
int zxdh_sc_cceq_destroy_done(struct zxdh_sc_ceq *ceq);

int zxdh_sc_ceq_destroy(struct zxdh_sc_ceq *ceq, u64 scratch, bool post_sq);
int zxdh_sc_ceq_init(struct zxdh_sc_ceq *ceq, struct zxdh_ceq_init_info *info);
void zxdh_sc_cleanup_ceqes(struct zxdh_sc_cq *cq, struct zxdh_sc_ceq *ceq);
void *zxdh_sc_process_ceq(struct zxdh_sc_dev *dev, struct zxdh_sc_ceq *ceq);

int zxdh_sc_aeq_init(struct zxdh_sc_aeq *aeq, struct zxdh_aeq_init_info *info);
int zxdh_sc_get_next_aeqe(struct zxdh_sc_aeq *aeq, struct zxdh_aeqe_info *info, struct zxdh_aeq_report_info *report_info);
int zxdh_sc_repost_aeq_tail(struct zxdh_sc_dev *dev, u32 idx);

void zxdh_sc_pd_init(struct zxdh_sc_dev *dev, struct zxdh_sc_pd *pd, u32 pd_id,
		     int abi_ver);
void zxdh_cfg_aeq(struct zxdh_sc_dev *dev, u32 irq_idx);
void zxdh_cfg_ext_aeq(struct zxdh_sc_dev *ext_dev, u32 irq_idx);
#if IS_ENABLED(CONFIG_CONFIGFS_FS)
void zxdh_set_irq_rate_limit(struct zxdh_sc_dev *dev, u32 idx, u32 interval);
#endif
void zxdh_check_cqp_progress(struct zxdh_cqp_timeout *cqp_timeout,
			     struct zxdh_sc_dev *dev);
int zxdh_cqp_poll_registers(struct zxdh_sc_cqp *cqp, u32 tail, u32 count);
int zxdh_ext_cqp_poll_registers(struct zxdh_sc_cqp *cqp, u32 tail, u32 count);
int zxdh_sc_cqp_create(struct zxdh_sc_cqp *cqp, u16 *maj_err, u16 *min_err);
int zxdh_ext_sc_cqp_create(struct zxdh_sc_cqp *cqp, u16 *maj_err, u16 *min_err);
int zxdh_sc_cqp_destroy(struct zxdh_sc_cqp *cqp, bool free_hwcqp);
int zxdh_ext_sc_cqp_destroy(struct zxdh_sc_cqp *ext_cqp, bool free_hwcqp);
int zxdh_sc_cqp_init(struct zxdh_sc_cqp *cqp, struct zxdh_cqp_init_info *info);
int zxdh_ext_sc_cqp_init(struct zxdh_sc_cqp *cqp, struct zxdh_cqp_init_info *info);
void zxdh_sc_cqp_post_sq(struct zxdh_sc_cqp *cqp);
int zxdh_sc_poll_for_cqp_op_done(struct zxdh_sc_cqp *cqp, u8 opcode,
				 struct zxdh_ccq_cqe_info *cmpl_info);
int zxdh_sc_qp_create(struct zxdh_sc_qp *qp, u64 scratch, bool post_sq);
int zxdh_sc_qp_destroy(struct zxdh_sc_qp *qp, u64 scratch, bool ignore_mw_bnd,
		       bool post_sq);
int zxdh_sc_qp_flush_wqes(struct zxdh_sc_qp *qp,
			  struct zxdh_qp_flush_info *info, u64 scratch,
			  bool post_sq);
int zxdh_sc_qp_init(struct zxdh_sc_qp *qp, struct zxdh_qp_init_info *info);
int zxdh_sc_qp_modify(struct zxdh_sc_qp *qp, struct zxdh_modify_qp_info *info,
		      u64 scratch, bool post_sq);
void zxdh_sc_qp_setctx_roce(struct zxdh_sc_qp *qp, __le64 *qp_ctx,
			    struct zxdh_qp_host_ctx_info *info);
void zxdh_sc_qp_resetctx_roce(struct zxdh_sc_qp *qp, __le64 *qp_ctx);
u16 get_rc_gqp_from_8k_index(u16 rc_8k_index, u16 rc_gqp_start, u16 rc_gqp_cnt);
int zxdh_sc_cq_destroy(struct zxdh_sc_cq *cq, u64 scratch, bool post_sq);
int zxdh_sc_cq_init(struct zxdh_sc_cq *cq, struct zxdh_cq_init_info *info);
void zxdh_sc_cq_resize(struct zxdh_sc_cq *cq, struct zxdh_modify_cq_info *info);
int zxdh_sc_aeq_destroy(struct zxdh_sc_aeq *aeq, u64 scratch, bool post_sq);

void sc_vsi_update_stats(struct zxdh_sc_vsi *vsi);
void zxdh_sc_qp_modify_ctx_udp_sport(struct zxdh_sc_qp *qp, __le64 *qp_ctx,
				     struct zxdh_qp_host_ctx_info *info);
void zxdh_sc_qp_modify_private_cmd_qpc(struct zxdh_sc_qp *qp, __le64 *qp_ctx,
				       struct zxdh_modify_qpc_item *info);
struct zxdh_qp;
int zxdh_reset_retry_tx_item(struct zxdh_qp *iwqp);
int process_hw_modify_qpc_cmd(struct zxdh_qp *iwqp,
				     		  struct zxdh_modify_qpc_item *modify_item,
				     		  u64 modify_mask);
			   
struct cqp_info {
	union {
		struct {
			struct zxdh_sc_qp *qp;
			struct zxdh_create_qp_info info;
			u64 scratch;
		} qp_create;

		struct {
			struct zxdh_sc_qp *qp;
			struct zxdh_modify_qp_info info;
			u64 scratch;
		} qp_modify;

		struct {
			struct zxdh_sc_qp *qp;
			u64 scratch;
			bool remove_hash_idx;
			bool ignore_mw_bnd;
		} qp_destroy;

		struct {
			struct zxdh_sc_srq *srq;
			struct zxdh_create_srq_info info;
			u64 scratch;
		} srq_create;

		struct {
			struct zxdh_sc_srq *srq;
			struct zxdh_modify_srq_info info;
			u64 scratch;
		} srq_modify;

		struct {
			struct zxdh_sc_srq *srq;
			u64 scratch;
			struct zxdh_destroy_srq_info info;
			// bool remove_hash_idx;
		} srq_destroy;

		struct {
			struct zxdh_sc_cq *cq;
			u64 scratch;
		} cq_create;

		struct {
			struct zxdh_sc_cq *cq;
			struct zxdh_modify_cq_info info;
			u64 scratch;
		} cq_modify;

		struct {
			struct zxdh_sc_cq *cq;
			u64 scratch;
		} cq_destroy;

		struct {
			struct zxdh_sc_dev *dev;
			struct zxdh_allocate_stag_info info;
			u64 scratch;
		} alloc_stag;

		struct {
			struct zxdh_sc_dev *dev;
			struct zxdh_mw_alloc_info info;
			u64 scratch;
		} mw_alloc;

		struct {
			struct zxdh_sc_dev *dev;
			struct zxdh_reg_ns_stag_info info;
			u64 scratch;
		} mr_reg_non_shared;

		struct {
			struct zxdh_sc_dev *dev;
			struct zxdh_dealloc_stag_info info;
			u64 scratch;
		} dealloc_stag;

		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_add_arp_cache_entry_info info;
			u64 scratch;
		} add_arp_cache_entry;

		struct {
			struct zxdh_sc_cqp *cqp;
			u64 scratch;
			u16 arp_index;
		} del_arp_cache_entry;

		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_local_mac_entry_info info;
			u64 scratch;
		} add_local_mac_entry;

		struct {
			struct zxdh_sc_cqp *cqp;
			u64 scratch;
			u8 entry_idx;
			u8 ignore_ref_count;
		} del_local_mac_entry;

		struct {
			struct zxdh_sc_cqp *cqp;
			u64 scratch;
		} alloc_local_mac_entry;

		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_manage_vf_pble_info info;
			u64 scratch;
		} manage_vf_pble_bp;

		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_cqp_manage_push_page_info info;
			u64 scratch;
		} manage_push_page;

		struct {
			struct zxdh_sc_dev *dev;
			struct zxdh_upload_context_info info;
			u64 scratch;
		} qp_upload_context;

		struct {
			struct zxdh_sc_dev *dev;
			struct zxdh_hmc_fcn_info info;
			u64 scratch;
		} manage_hmc_pm;

		struct {
			struct zxdh_sc_ceq *ceq;
			u64 scratch;
		} ceq_create;

		struct {
			struct zxdh_sc_ceq *ceq;
			u64 scratch;
		} ceq_destroy;

		struct {
			struct zxdh_sc_aeq *aeq;
			u64 scratch;
		} aeq_create;

		struct {
			struct zxdh_sc_aeq *aeq;
			u64 scratch;
		} aeq_destroy;

		struct {
			struct zxdh_sc_qp *qp;
			struct zxdh_qp_flush_info info;
			u64 scratch;
		} qp_flush_wqes;

		struct {
			struct zxdh_sc_qp *qp;
			struct zxdh_gen_ae_info info;
			u64 scratch;
		} gen_ae;

		struct {
			struct zxdh_sc_cqp *cqp;
			void *fpm_val_va;
			u64 fpm_val_pa;
			u8 hmc_fn_id;
			u64 scratch;
		} query_fpm_val;

		struct {
			struct zxdh_sc_cqp *cqp;
			void *fpm_val_va;
			u64 fpm_val_pa;
			u8 hmc_fn_id;
			u64 scratch;
		} commit_fpm_val;

		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_apbvt_info info;
			u64 scratch;
		} manage_apbvt_entry;

		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_qhash_table_info info;
			u64 scratch;
		} manage_qhash_table_entry;

		struct {
			struct zxdh_sc_dev *dev;
			struct zxdh_update_sds_info info;
			u64 scratch;
		} update_pe_sds;

		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_sc_qp *qp;
			u64 scratch;
		} suspend_resume;

		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_ah_info info;
			u64 scratch;
		} ah_create;

		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_ah_info info;
			u64 scratch;
		} ah_destroy;

		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_mcast_grp_info *info;
			u64 scratch;
		} mc_create;

		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_mcast_grp_info *info;
			u64 scratch;
		} mc_destroy;

		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_mcast_grp_info *info;
			u64 scratch;
		} mc_modify;

		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_stats_inst_info info;
			u64 scratch;
		} stats_manage;

		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_stats_gather_info info;
			u64 scratch;
		} stats_gather;

		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_ws_node_info info;
			u64 scratch;
		} ws_node;

		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_up_info info;
			u64 scratch;
		} up_map;

		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_dma_mem query_buff_mem;
			u64 scratch;
		} query_rdma;

		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_src_copy_dest src_dest;
			struct zxdh_path_index src_path_index;
			struct zxdh_path_index dest_path_index;
			bool host;
			u64 scratch;
		} dma_writeread;
		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_mailboxhead_data mbhead_data;
			u64 scratch;
			u32 dst_vf_id;
		} hmc_mb;
		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_path_index dest_path_index;
			struct zxdh_dma_write32_date dma_data;
			u64 scratch;
		} dma_write32data;
		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_path_index dest_path_index;
			struct zxdh_dma_write64_date dma_data;
			u64 scratch;
		} dma_write64data;
		struct {
			struct zxdh_sc_cqp *cqp;
			struct zxdh_dam_read_bycqe dma_rcqe;
			struct zxdh_path_index src_path_index;
			u64 scratch;
		} dma_read_cqe;
		struct {
			struct zxdh_sc_dev *dev;
			u32 qpn;
			u64 qpc_buf_pa;
			u64 scratch;
		} query_qpc;
		struct {
			struct zxdh_sc_dev *dev;
			u32 cqn;
			u64 cqc_buf_pa;
			u64 scratch;
		} query_cqc;
		struct {
			struct zxdh_sc_dev *dev;
			u32 ceqn;
			u64 ceqc_buf_pa;
			u64 scratch;
		} query_ceqc;
		struct {
			struct zxdh_sc_dev *dev;
			u16 aeqn;
			u64 aeqc_buf_pa;
			u64 scratch;
		} query_aeqc;
		struct {
			struct zxdh_sc_dev *dev;
			u32 srqn;
			u64 srqc_buf_pa;
			u64 scratch;
		} query_srqc;

		struct {
			struct zxdh_sc_cqp *cqp;
			u64 scratch;
			u32 mkeyindex;
		} query_mkey;
		
	} u;
};

struct cqp_cmds_info {
	struct list_head cqp_cmd_entry;
	u8 cqp_cmd;
	u8 post_sq;
	struct cqp_info in;
};

struct zxdh_virtchnl_work {
	struct work_struct work;
	u8 vf_msg_buf[ZXDH_VCHNL_MAX_VF_MSG_SIZE];
	struct zxdh_sc_dev *dev;
	u16 vf_id;
	u16 len;
};

__le64 *zxdh_sc_cqp_get_next_send_wqe_idx(struct zxdh_sc_cqp *cqp, u64 scratch,
					  u32 *wqe_idx);

/**
 * zxdh_sc_cqp_get_next_send_wqe - get next wqe on cqp sq
 * @cqp: struct for cqp hw
 * @scratch: private data for CQP WQE
 */
static inline __le64 *zxdh_sc_cqp_get_next_send_wqe(struct zxdh_sc_cqp *cqp,
						    u64 scratch)
{
	u32 wqe_idx;

	return zxdh_sc_cqp_get_next_send_wqe_idx(cqp, scratch, &wqe_idx);
}

__le64 *zxdh_ext_sc_cqp_get_next_send_wqe_idx(struct zxdh_sc_cqp *cqp, u64 scratch,
					  u32 *wqe_idx);

/**
 * zxdh_ext_sc_cqp_get_next_send_wqe - get next wqe on cqp sq
 * @cqp: struct for cqp hw
 * @scratch: private data for CQP WQE
 */
static inline __le64 *zxdh_ext_sc_cqp_get_next_send_wqe(struct zxdh_sc_cqp *cqp,
						    u64 scratch)
{
	u32 wqe_idx;

	return zxdh_ext_sc_cqp_get_next_send_wqe_idx(cqp, scratch, &wqe_idx);
}

#endif /* ZXDH_TYPE_H */
