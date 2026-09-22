/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef ZXDH_USER_H
#define ZXDH_USER_H

#define zxdh_handle void *
#define zxdh_adapter_handle zxdh_handle
#define zxdh_qp_handle zxdh_handle
#define zxdh_cq_handle zxdh_handle
#define zxdh_pd_id zxdh_handle
#define zxdh_stag_handle zxdh_handle
#define zxdh_stag_index u32
#define zxdh_stag u32
#define zxdh_stag_key u8
#define zxdh_tagged_offset u64
#define zxdh_access_privileges u32
#define zxdh_physical_fragment u64
#define zxdh_address_list u64 *
#define zxdh_sgl struct zxdh_sge *

#define ZXDH_MAX_MR_SIZE 0x200000000000ULL

#define ZXDH_ACCESS_FLAGS_LOCALREAD 0x01
#define ZXDH_ACCESS_FLAGS_LOCALWRITE 0x02
#define ZXDH_ACCESS_FLAGS_REMOTEREAD_ONLY 0x04
#define ZXDH_ACCESS_FLAGS_REMOTEREAD 0x05
#define ZXDH_ACCESS_FLAGS_REMOTEWRITE_ONLY 0x08
#define ZXDH_ACCESS_FLAGS_REMOTEWRITE 0x0a
#define ZXDH_ACCESS_FLAGS_BIND_WINDOW 0x10
#define ZXDH_ACCESS_FLAGS_ZERO_BASED 0x20
#define ZXDH_ACCESS_FLAGS_ALL 0x3f

#define ZXDH_OP_TYPE_NOP 0x00
#define ZXDH_OP_TYPE_SEND 0x01
#define ZXDH_OP_TYPE_SEND_WITH_IMM 0x02
#define ZXDH_OP_TYPE_SEND_INV 0x03
#define ZXDH_OP_TYPE_WRITE 0x04
#define ZXDH_OP_TYPE_WRITE_WITH_IMM 0x05
#define ZXDH_OP_TYPE_READ 0x06
#define ZXDH_OP_TYPE_BIND_MW 0x07
#define ZXDH_OP_TYPE_FAST_REG_MR 0x08
#define ZXDH_OP_TYPE_LOCAL_INV 0x09
#define ZXDH_OP_TYPE_UD_SEND 0x0a
#define ZXDH_OP_TYPE_UD_SEND_WITH_IMM 0x0b

#define ZXDH_OP_TYPE_REC 0x3e
#define ZXDH_OP_TYPE_REC_IMM 0x3f
#define ERROR_CODE_VALUE 65
#define ZXDH_FLUSH_MAJOR_ERR 1
#define ZXDH_RETRY_ACK_MAJOR_ERR 0x8
#define ZXDH_RETRY_ACK_MINOR_ERR 0xf3
#define ZXDH_TX_WINDOW_QUERY_ITEM_MINOR_ERR 0xf5

#define ZXDH_MAX_MSIX_INTERRUPT_SIZE 24

#define ZXDH_MIN_ROCE_QP_ID 1
#define ZXDH_MIN_ROCE_SRQ_ID 1

#define ZXDH_SQE_SIZE 4
#define ZXDH_RQE_SIZE 2
#define IRDMARX_RD_TIME_LIMIT_VALUE 0x20

#define ZXDH_4K_TO_64K_MUL 16

enum zxdh_hw_stats_state {
	ZXDH_HW_STATS_INVALID = 0,
	ZXDH_HW_STATS_VALID,
};

enum zxdh_cfg_ram_state {
	ZXDH_CFG_RAM_FREE = 0,
	ZXDH_CFG_RAM_BUSY,
};

enum zxdh_stat_rd_clr_mode {
	ZXDH_STAT_RD_MODE_UNCLR = 0, //Not reading clearly
	ZXDH_STAT_RD_MODE_CLR, // Read Clearly
};

enum zxdh_device_caps_const {
	ZXDH_WQE_SIZE = 4,
	ZXDH_CQP_WQE_SIZE = 8,
	ZXDH_CQE_SIZE = 8,
	ZXDH_EXTENDED_CQE_SIZE = 8,
	ZXDH_AEQE_SIZE = 4,
	ZXDH_CEQE_SIZE = 2,
	ZXDH_CQP_CTX_SIZE = 8,
	ZXDH_SHADOW_AREA_SIZE = 1,
	ZXDH_GATHER_STATS_BUF_SIZE = 1024,
	ZXDH_MIN_IW_QP_ID = 0,
	ZXDH_QUERY_FPM_BUF_SIZE = 176,
	ZXDH_COMMIT_FPM_BUF_SIZE = 176,
	ZXDH_MAX_IW_QP_ID = 262143,
	ZXDH_MIN_CEQID = 0,
	ZXDH_MAX_CEQID = 4095,
	ZXDH_CEQ_MAX_COUNT = ZXDH_MAX_CEQID + 1,
	ZXDH_MIN_CQID = 0,
	ZXDH_MAX_CQID = 524287,
	ZXDH_MIN_AEQ_ENTRIES = 1,
	ZXDH_MAX_AEQ_ENTRIES = 131072, // 64k QP + 32k CQ + 32k SRQ
	ZXDH_MIN_CEQ_ENTRIES = 1,
	ZXDH_MAX_CEQ_ENTRIES = 32768, // 32k CQ
	ZXDH_MIN_CQ_SIZE = 1,
	ZXDH_MAX_CQ_SIZE = 2097152, // 2M
	ZXDH_DB_ID_ZERO = 0,
	ZXDH_MAX_OUTBOUND_MSG_SIZE = 2147483647,
	ZXDH_MAX_INBOUND_MSG_SIZE = 2147483647,
	ZXDH_MAX_PUSH_PAGE_COUNT = 1024,
	ZXDH_MAX_PE_ENA_VF_COUNT = 128,
	ZXDH_MAX_VF_FPM_ID = 47,
	ZXDH_MAX_SQ_PAYLOAD_SIZE = 2147483648,
	ZXDH_MAX_INLINE_DATA_SIZE = 217,
	ZXDH_MAX_WQ_ENTRIES = 32768,
	ZXDH_Q2_BUF_SIZE = 256,
	ZXDH_QP_CTX_SIZE = 512,
	ZXDH_CQ_CTX_SIZE = 64,
	ZXDH_CEQ_CTX_SIZE = 32,
	ZXDH_AEQ_CTX_SIZE = 32,
	ZXDH_SRQ_CTX_SIZE = 64,
	ZXDH_MAX_PDS = 1048576, // 1M
};

enum zxdh_host_epid {
	ZXDH_HOST_EP0_ID = 5,
	ZXDH_HOST_EP1_ID = 6,
	ZXDH_HOST_EP2_ID = 7,
	ZXDH_HOST_EP3_ID = 8,
	ZXDH_HOST_EP4_ID = 9,
};

enum zxdh_addressing_type {
	ZXDH_ADDR_TYPE_ZERO_BASED = 0,
	ZXDH_ADDR_TYPE_VA_BASED = 1,
};

enum zxdh_queue_status {
	ZXDH_QUEUE_STATE_INVALID = 0,
	ZXDH_QUEUE_STATE_OK,
};

enum zxdh_ceqe_size {
	ZXDH_CEQE_SIZE_16_BYTE = 0,
	ZXDH_CEQE_SIZE_32_BYTE,
	ZXDH_CEQE_SIZE_64_BYTE,
	ZXDH_CEQE_SIZE_128_BYTE,
};

enum zxdh_irq_type {
	ZXDH_IRQ_TYPE_MSIX = 0,
	ZXDH_IRQ_TYPE_PIN,
};

enum zxdh_ceq_aggregation_cnt {
	IRMDA_CEQ_AGGREGATION_CNT_0,
	IRMDA_CEQ_AGGREGATION_CNT_1 = 1,
	ZXDH_CEQ_AGGREGATION_CNT_2 = 2,
};

enum zxdh_vf_active_state {
	IRMDA_VF_STATE_INVALID = 0,
	ZXDH_VF_STATE_VALID,
};

enum zxdh_flush_opcode {
	FLUSH_INVALID = 0,
	FLUSH_GENERAL_ERR,
	FLUSH_PROT_ERR,
	FLUSH_REM_ACCESS_ERR,
	FLUSH_LOC_QP_OP_ERR,
	FLUSH_REM_OP_ERR,
	FLUSH_LOC_LEN_ERR,
	FLUSH_FATAL_ERR,
	FLUSH_RETRY_EXC_ERR,
	FLUSH_MW_BIND_ERR,
	FLUSH_REM_INV_REQ_ERR,
	FLUSH_MR_FASTREG_ERR,
};

enum zxdh_cmpl_status {
	ZXDH_COMPL_STATUS_SUCCESS = 0,
	ZXDH_COMPL_STATUS_FLUSHED,
	ZXDH_COMPL_STATUS_INVALID_WQE,
	ZXDH_COMPL_STATUS_QP_CATASTROPHIC,
	ZXDH_COMPL_STATUS_REMOTE_TERMINATION,
	ZXDH_COMPL_STATUS_INVALID_STAG,
	ZXDH_COMPL_STATUS_BASE_BOUND_VIOLATION,
	ZXDH_COMPL_STATUS_ACCESS_VIOLATION,
	ZXDH_COMPL_STATUS_INVALID_PD_ID,
	ZXDH_COMPL_STATUS_WRAP_ERROR,
	ZXDH_COMPL_STATUS_STAG_INVALID_PDID,
	ZXDH_COMPL_STATUS_RDMA_READ_ZERO_ORD,
	ZXDH_COMPL_STATUS_QP_NOT_PRIVLEDGED,
	ZXDH_COMPL_STATUS_STAG_NOT_INVALID,
	ZXDH_COMPL_STATUS_INVALID_PHYS_BUF_SIZE,
	ZXDH_COMPL_STATUS_INVALID_PHYS_BUF_ENTRY,
	ZXDH_COMPL_STATUS_INVALID_FBO,
	ZXDH_COMPL_STATUS_INVALID_LEN,
	ZXDH_COMPL_STATUS_INVALID_ACCESS,
	ZXDH_COMPL_STATUS_PHYS_BUF_LIST_TOO_LONG,
	ZXDH_COMPL_STATUS_INVALID_VIRT_ADDRESS,
	ZXDH_COMPL_STATUS_INVALID_REGION,
	ZXDH_COMPL_STATUS_INVALID_WINDOW,
	ZXDH_COMPL_STATUS_INVALID_TOTAL_LEN,
	ZXDH_COMPL_STATUS_RETRY_ACK_ERR,
	ZXDH_COMPL_STATUS_TX_WINDOW_QUERY_ITEM_ERR,
	ZXDH_COMPL_STATUS_UNKNOWN,
};

enum zxdh_cmpl_notify {
	ZXDH_CQ_COMPL_EVENT = 0,
	ZXDH_CQ_COMPL_SOLICITED = 1,
};

enum zxdh_qp_caps {
	ZXDH_WRITE_WITH_IMM = 1,
	ZXDH_SEND_WITH_IMM = 2,
	ZXDH_ROCE = 4,
	ZXDH_PUSH_MODE = 8,
};

struct zxdh_qp_uk;
struct zxdh_cq_uk;
struct zxdh_qp_uk_init_info;
struct zxdh_cq_uk_init_info;

struct zxdh_sge {
	zxdh_tagged_offset tag_off;
	u32 len;
	zxdh_stag stag;
};

struct zxdh_ring {
	u32 head;
	u32 tail;
	u32 size;
};

struct zxdh_cqe {
	__le64 buf[ZXDH_CQE_SIZE];
};

struct zxdh_extended_cqe {
	__le64 buf[ZXDH_EXTENDED_CQE_SIZE];
};

struct zxdh_post_send {
	zxdh_sgl sg_list;
	u32 num_sges;
	u32 qkey;
	u32 dest_qp;
	u32 ah_id;
};

struct zxdh_post_inline_send {
	void *data;
	u32 len;
	u32 qkey;
	u32 dest_qp;
	u32 ah_id;
};

struct zxdh_post_rq_info {
	u64 wr_id;
	zxdh_sgl sg_list;
	u32 num_sges;
};

struct zxdh_rdma_write {
	zxdh_sgl lo_sg_list;
	u32 num_lo_sges;
	struct zxdh_sge rem_addr;
};

struct zxdh_inline_rdma_write {
	void *data;
	u32 len;
	struct zxdh_sge rem_addr;
};

struct zxdh_rdma_read {
	zxdh_sgl lo_sg_list;
	u32 num_lo_sges;
	struct zxdh_sge rem_addr;
};

struct zxdh_bind_window {
	zxdh_stag mr_stag;
	u64 bind_len;
	void *va;
	enum zxdh_addressing_type addressing_type;
	u8 ena_reads : 1;
	u8 ena_writes : 1;
	zxdh_stag mw_stag;
	u8 mem_window_type_1 : 1;
};

struct zxdh_inv_local_stag {
	zxdh_stag target_stag;
};

struct zxdh_post_sq_info {
	u64 wr_id;
	u8 op_type;
	u8 l4len;
	u8 signaled : 1;
	u8 solicited : 1;
	u8 read_fence : 1;
	u8 local_fence : 1;
	u8 inline_data : 1;
	u8 imm_data_valid : 1;
	u8 push_wqe : 1;
	u8 report_rtt : 1;
	u8 udp_hdr : 1;
	u8 defer_flag : 1;
	u32 imm_data;
	u32 stag_to_inv;
	union {
		struct zxdh_post_send send;
		struct zxdh_rdma_write rdma_write;
		struct zxdh_rdma_read rdma_read;
		struct zxdh_bind_window bind_window;
		struct zxdh_inv_local_stag inv_local_stag;
		struct zxdh_inline_rdma_write inline_rdma_write;
		struct zxdh_post_inline_send inline_send;
	} op;
};

struct zxdh_cq_poll_info {
	u64 wr_id;
	zxdh_qp_handle qp_handle;
	u32 bytes_xfered;
	u32 tcp_seq_num_rtt;
	u32 qp_id;
	u32 ud_src_qpn;
	u32 imm_data;
	zxdh_stag inv_stag; /* or L_R_Key */
	enum zxdh_cmpl_status comp_status;
	u16 major_err;
	u16 minor_err;
	u16 ud_vlan;
	u8 ud_smac[6];
	u8 op_type;
	u8 stag_invalid_set : 1; /* or L_R_Key set */
	u8 push_dropped : 1;
	u8 error : 1;
	u8 solicited_event : 1;
	u8 ipv4 : 1;
	u8 ud_vlan_valid : 1;
	u8 ud_smac_valid : 1;
	u8 imm_valid : 1;
};

int zxdh_uk_inline_rdma_write(struct zxdh_qp_uk *qp,
			      struct zxdh_post_sq_info *info, bool post_sq);
int zxdh_uk_rc_inline_send(struct zxdh_qp_uk *qp,
			   struct zxdh_post_sq_info *info, bool post_sq);
int zxdh_uk_ud_inline_send(struct zxdh_qp_uk *qp,
			   struct zxdh_post_sq_info *info, bool post_sq);
int zxdh_uk_mw_bind(struct zxdh_qp_uk *qp, struct zxdh_post_sq_info *info,
		    bool post_sq);
int zxdh_uk_post_nop(struct zxdh_qp_uk *qp, u64 wr_id, bool signaled,
		     bool post_sq);
int zxdh_uk_post_receive(struct zxdh_qp_uk *qp, struct zxdh_post_rq_info *info);
void zxdh_uk_qp_post_wr(struct zxdh_qp_uk *qp);
void zxdh_uk_qp_set_shadow_area(struct zxdh_qp_uk *qp);
int zxdh_uk_rdma_read(struct zxdh_qp_uk *qp, struct zxdh_post_sq_info *info,
		      bool post_sq);
int zxdh_uk_rdma_write(struct zxdh_qp_uk *qp, struct zxdh_post_sq_info *info,
		       bool post_sq);
int zxdh_uk_rc_send(struct zxdh_qp_uk *qp, struct zxdh_post_sq_info *info,
		    bool post_sq);
int zxdh_uk_ud_send(struct zxdh_qp_uk *qp, struct zxdh_post_sq_info *info,
		    bool post_sq);
int zxdh_uk_stag_local_invalidate(struct zxdh_qp_uk *qp,
				  struct zxdh_post_sq_info *info, bool post_sq);

struct zxdh_wqe_uk_ops {
	void (*iw_copy_inline_data)(u8 *dest, u8 *src, u32 len, u8 polarity,
				    bool imm_data_flag);
	u16 (*iw_inline_data_size_to_quanta)(u32 data_size, bool imm_data_flag);
	void (*iw_set_fragment)(__le64 *wqe, u32 offset, struct zxdh_sge *sge,
				u8 valid);
	void (*iw_set_mw_bind_wqe)(__le64 *wqe,
				   struct zxdh_bind_window *op_info);
};

enum zxdh_status_code zxdh_uk_cq_poll_cmpl(struct zxdh_cq_uk *cq, struct zxdh_cq_poll_info *info);
void zxdh_uk_cq_request_notification(struct zxdh_cq_uk *cq,
				     enum zxdh_cmpl_notify cq_notify);
void zxdh_uk_cq_resize(struct zxdh_cq_uk *cq, void *cq_base, int size);
void zxdh_uk_cq_set_resized_cnt(struct zxdh_cq_uk *qp, u16 cnt);
void zxdh_uk_cq_init(struct zxdh_cq_uk *cq, struct zxdh_cq_uk_init_info *info);
int zxdh_uk_qp_init(struct zxdh_qp_uk *qp, struct zxdh_qp_uk_init_info *info);
struct zxdh_sq_uk_wr_trk_info {
	u64 wrid;
	u32 wr_len;
	u16 quanta;
	u8 reserved[2];
};

struct zxdh_qp_sq_quanta {
	__le64 elem[ZXDH_SQE_SIZE];
};

struct zxdh_qp_rq_quanta {
	__le64 elem[ZXDH_RQE_SIZE];
};

struct zxdh_qp_uk {
	struct zxdh_qp_sq_quanta *sq_base;
	struct zxdh_qp_rq_quanta *rq_base;
	struct zxdh_uk_attrs *uk_attrs;
	u32 __iomem *wqe_alloc_db;
	struct zxdh_sq_uk_wr_trk_info *sq_wrtrk_array;
	u64 *rq_wrid_array;
	__le64 *shadow_area;
	__le32 *push_db;
	__le64 *push_wqe;
	struct zxdh_ring sq_ring;
	struct zxdh_ring rq_ring;
	struct zxdh_ring initial_ring;
	u32 qp_id;
	u32 qp_caps;
	u32 sq_size;
	u32 rq_size;
	u32 max_sq_frag_cnt;
	u32 max_rq_frag_cnt;
	u32 max_inline_data;
	struct zxdh_wqe_uk_ops wqe_ops;
	u16 conn_wqes;
	u8 qp_type;
	u8 swqe_polarity;
	u8 swqe_polarity_deferred;
	u8 rwqe_polarity;
	u8 rq_wqe_size;
	u8 rq_wqe_size_multiplier;
	u8 deferred_flag : 1;
	u8 push_mode : 1; /* whether the last post wqe was pushed */
	u8 push_dropped : 1;
	u8 first_sq_wq : 1;
	u8 sq_flush_complete : 1; /* Indicates flush was seen and SQ was empty after the flush */
	u8 rq_flush_complete : 1; /* Indicates flush was seen and RQ was empty after the flush */
	u8 destroy_pending : 1; /* Indicates the QP is being destroyed */
	void *back_qp;
	spinlock_t *lock;
	u8 dbg_rq_flushed;
	u16 ord_cnt;
	u16 qp_8k_index;
	u16 rwqe_signature;
	u8 sq_flush_seen;
	u8 rq_flush_seen;
	u8 rd_fence_rate;
	u8 user_pri;
	u8 pmtu;
	u8 is_srq;
	u32 cqe_last_ack_psn;
	u32 qp_last_ack_psn;
	u8 cqe_retry_cnt;
	u8 qp_reset_cnt;
};

struct zxdh_cq_uk {
	struct zxdh_cqe *cq_base;
	u32 __iomem *cqe_alloc_db;
	u32 __iomem *cq_ack_db;
	__le64 *shadow_area;
	u32 cq_id;
	u32 cq_size;
	u32 cq_log_size;
	u32 cqe_rd_cnt;
	bool valid_cq;
	struct zxdh_ring cq_ring;
	u8 polarity;
	u8 armed : 1;
	u8 cqe_size;
};

struct zxdh_qp_uk_init_info {
	struct zxdh_qp_sq_quanta *sq;
	struct zxdh_qp_rq_quanta *rq;
	struct zxdh_uk_attrs *uk_attrs;
	u32 __iomem *wqe_alloc_db;
	__le64 *shadow_area;
	struct zxdh_sq_uk_wr_trk_info *sq_wrtrk_array;
	u64 *rq_wrid_array;
	u32 qp_id;
	u32 qp_caps;
	u32 sq_size;
	u32 rq_size;
	u32 max_sq_frag_cnt;
	u32 max_rq_frag_cnt;
	u32 max_inline_data;
	u8 first_sq_wq;
	u8 type;
	u8 rd_fence_rate;
	int abi_ver;
	bool legacy_mode;
};

struct zxdh_cq_uk_init_info {
	u32 __iomem *cqe_alloc_db;
	u32 __iomem *cq_ack_db;
	struct zxdh_cqe *cq_base;
	__le64 *shadow_area;
	u32 cq_size;
	u32 cq_log_size;
	u32 cq_id;
	u8 cqe_size;
};

__le64 *zxdh_qp_get_next_send_wqe(struct zxdh_qp_uk *qp, u32 *wqe_idx,
				  u16 quanta, u32 total_size,
				  struct zxdh_post_sq_info *info);
__le64 *zxdh_qp_get_next_recv_wqe(struct zxdh_qp_uk *qp, u32 *wqe_idx);
void zxdh_uk_clean_cq(void *q, struct zxdh_cq_uk *cq);
int zxdh_nop(struct zxdh_qp_uk *qp, u64 wr_id, bool signaled, bool post_sq);
int zxdh_fragcnt_to_quanta_sq(u32 frag_cnt, u16 *quanta);
int zxdh_fragcnt_to_wqesize_rq(u32 frag_cnt, u16 *wqe_size);
void zxdh_get_sq_wqe_shift(struct zxdh_uk_attrs *uk_attrs, u32 sge,
			   u32 inline_data, u8 *shift);
void zxdh_get_rq_wqe_shift(struct zxdh_uk_attrs *uk_attrs, u32 sge, u8 *shift);
int zxdh_get_sqdepth(u32 max_hw_wq_quanta, u32 sq_size, u8 shift, u32 *wqdepth);
int zxdh_get_rqdepth(u32 max_hw_rq_quanta, u32 rq_size, u8 shift, u32 *wqdepth);
#ifdef Z_CONFIG_PUSH_MODE
void zxdh_qp_push_wqe(struct zxdh_qp_uk *qp, __le64 *wqe, u16 quanta,
		      u32 wqe_idx, bool post_sq);
#endif
void zxdh_clr_wqes(struct zxdh_qp_uk *qp, u32 qp_wqe_idx);
#endif /* ZXDH_USER_H */
