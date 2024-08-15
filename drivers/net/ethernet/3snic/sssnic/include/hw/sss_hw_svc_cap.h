/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HW_SVC_CAP_H
#define SSS_HW_SVC_CAP_H

#include <linux/types.h>

enum sss_service_type {
	SSS_SERVICE_TYPE_NIC = 0,
	SSS_SERVICE_TYPE_OVS,
	SSS_SERVICE_TYPE_ROCE,
	SSS_SERVICE_TYPE_TOE,
	SSS_SERVICE_TYPE_IOE,
	SSS_SERVICE_TYPE_FC,
	SSS_SERVICE_TYPE_VBS,
	SSS_SERVICE_TYPE_IPSEC,
	SSS_SERVICE_TYPE_VIRTIO,
	SSS_SERVICE_TYPE_MIGRATE,
	SSS_SERVICE_TYPE_PPA,
	SSS_SERVICE_TYPE_CUSTOM,
	SSS_SERVICE_TYPE_VROCE,
	SSS_SERVICE_TYPE_MAX,

	SSS_SERVICE_TYPE_INTF = (1 << 15),
	SSS_SERVICE_TYPE_QMM = (1 << 16),
};

/* RDMA service capability */
enum {
	SSS_RDMA_BMME_FLAG_LOCAL_INV = (1 << 0),
	SSS_RDMA_BMME_FLAG_REMOTE_INV = (1 << 1),
	SSS_RDMA_BMME_FLAG_FAST_REG_WR = (1 << 2),
	SSS_RDMA_BMME_FLAG_RESERVED_LKEY = (1 << 3),
	SSS_RDMA_BMME_FLAG_TYPE_2_WIN = (1 << 4),
	SSS_RDMA_BMME_FLAG_WIN_TYPE_2B = (1 << 5),

	SSS_RDMA_DEV_CAP_FLAG_XRC = (1 << 6),
	SSS_RDMA_DEV_CAP_FLAG_MEM_WINDOW = (1 << 7),
	SSS_RDMA_DEV_CAP_FLAG_ATOMIC = (1 << 8),
	SSS_RDMA_DEV_CAP_FLAG_APM = (1 << 9),
};

struct sss_ppa_service_cap {
	u16 qpc_pseudo_vf_start;
	u16 qpc_pseudo_vf_num;
	u32 qpc_pseudo_vf_ctx_num;
	u32 pctx_size; /* 512B */
	u32 bloomfilter_len;
	u8 bloomfilter_en;
	u8 rsvd0;
	u16 rsvd1;
};

struct sss_vbs_service_cap {
	u16 vbs_max_volq;
	u16 rsvd1;
};

/* PF/VF ToE service resource */
struct sss_dev_toe_svc_cap {
	u32 max_pctx; /* Parent Context: max specifications 1M */
	u32 max_cctxt;
	u32 max_cq;
	u16 max_srq;
	u32 srq_id_start;
	u32 max_mpt;
};

/* ToE services */
struct sss_toe_service_cap {
	struct sss_dev_toe_svc_cap dev_toe_cap;

	u8 alloc_flag;
	u8 rsvd[3];
	u32 pctx_size; /* 1KB */
	u32 scqc_size; /* 64B */
};

/* PF FC service resource */
struct sss_dev_fc_svc_cap {
	/* PF Parent QPC */
	u32 max_parent_qpc_num; /* max number is 2048 */

	/* PF Child QPC */
	u32 max_child_qpc_num; /* max number is 2048 */
	u32 child_qpc_id_start;

	/* PF SCQ */
	u32 scq_num; /* 16 */

	/* PF supports SRQ */
	u32 srq_num; /* Number of SRQ is 2 */

	u8 vp_id_start;
	u8 vp_id_end;
};

/* FC services */
struct sss_fc_service_cap {
	struct sss_dev_fc_svc_cap dev_fc_cap;

	/* Parent QPC */
	u32 parent_qpc_size; /* 256B */

	/* Child QPC */
	u32 child_qpc_size; /* 256B */

	/* SQ */
	u32 sqe_size; /* 128B(in linked list mode) */

	/* SCQ */
	u32 scqc_size; /* Size of the Context 32B */
	u32 scqe_size; /* 64B */

	/* SRQ */
	u32 srqc_size; /* Size of SRQ Context (64B) */
	u32 srqe_size; /* 32B */
};

struct sss_dev_roce_svc_own_cap {
	u32 max_qp;
	u32 max_cq;
	u32 max_srq;
	u32 max_mpt;
	u32 max_drc_qp;

	u32 cmtt_cl_start;
	u32 cmtt_cl_end;
	u32 cmtt_cl_size;

	u32 dmtt_cl_start;
	u32 dmtt_cl_end;
	u32 dmtt_cl_size;

	u32 wqe_cl_start;
	u32 wqe_cl_end;
	u32 wqe_cl_size;

	u32 qpc_entry_size;
	u32 max_wqe;
	u32 max_rq_sg;
	u32 max_sq_inline_data_size;
	u32 max_rq_desc_size;

	u32 rdmarc_entry_size;
	u32 max_qp_init_rdma;
	u32 max_qp_dest_rdma;

	u32 max_srq_wqe;
	u32 reserved_srq;
	u32 max_srq_sge;
	u32 srqc_entry_size;

	u32 max_msg_size; /* Message size 2GB */
};

/* RDMA service capability */
struct sss_dev_rdma_svc_cap {
	struct sss_dev_roce_svc_own_cap roce_own_cap;
};

struct sss_nic_service_cap {
	u16 max_sq;
	u16 max_rq;
	u16 def_queue_num;
};

/* RDMA services */
struct sss_rdma_service_cap {
	struct sss_dev_rdma_svc_cap dev_rdma_cap;

	/* 1. the number of MTT PA must be integer power of 2
	 * 2. represented by logarithm. Each MTT table can
	 * contain 1, 2, 4, 8, and 16 PA)
	 */
	u8 log_mtt;

	/* Number of MTT table (4M), is actually MTT seg number */
	u32 mtt_num;

	u32 log_mtt_seg;
	u32 mtt_entry_size; /* MTT table size 8B, including 1 PA(64bits) */
	u32 mpt_entry_size; /* MPT table size (64B) */

	u32 dmtt_cl_start;
	u32 dmtt_cl_end;
	u32 dmtt_cl_size;

	/* 1. the number of RDMArc PA must be integer power of 2
	 * 2. represented by logarithm. Each MTT table can
	 * contain 1, 2, 4, 8, and 16 PA)
	 */
	u8 log_rdmarc;

	u32 reserved_qp; /* Number of reserved QP */
	u32 max_sq_sg; /* Maximum SGE number of SQ (8) */

	/* WQE maximum size of SQ(1024B), inline maximum
	 * size if 960B(944B aligned to the 960B),
	 * 960B=>wqebb alignment=>1024B
	 */
	u32 max_sq_desc_size;

	/* Currently, the supports 64B and 128B,
	 * defined as 64Bytes
	 */
	u32 wqebb_size;

	u32 max_cqe; /* Size of the depth of the CQ (64K-1) */
	u32 reserved_cq; /* Number of reserved CQ */
	u32 cqc_entry_size; /* Size of the CQC (64B/128B) */
	u32 cqe_size; /* Size of CQE (32B) */

	u32 reserved_mrw; /* Number of reserved MR/MR Window */

	/* max MAP of FMR,
	 * (1 << (32-ilog2(num_mpt)))-1;
	 */
	u32 max_fmr_map;

	u32 log_rdmarc_seg; /* table number of each RDMArc seg(3) */

	/* Timeout time. Formula:Tr=4.096us*2(local_ca_ack_delay), [Tr,4Tr] */
	u32 local_ca_ack_delay;
	u32 port_num; /* Physical port number */

	u32 db_page_size; /* Size of the DB (4KB) */
	u32 direct_wqe_size; /* Size of the DWQE (256B) */

	u32 pd_num; /* Maximum number of PD (128K) */
	u32 reserved_pd; /* Number of reserved PD */
	u32 max_xrcd; /* Maximum number of xrcd (64K) */
	u32 reserved_xrcd; /* Number of reserved xrcd */

	u32 max_gid_per_port; /* gid number (16) of each port */

	/* RoCE v2 GID table is 32B,
	 * compatible RoCE v1 expansion
	 */
	u32 gid_entry_size;

	u32 reserved_lkey; /* local_dma_lkey */
	u32 comp_vector_num; /* Number of complete vector (32) */
	u32 page_size_cap; /* Supports 4K,8K,64K,256K,1M and 4M page_size */

	u32 flag; /* RDMA some identity */
	u32 max_frpl_len; /* Maximum number of pages frmr registration */
	u32 max_pkey; /* Number of supported pkey group */
};

/* PF OVS service resource */
struct sss_dev_ovs_svc_cap {
	u32 max_pctx; /* Parent Context: max specifications 1M */
	u32 pseudo_vf_max_pctx;
	u16 pseudo_vf_num;
	u16 pseudo_vf_start_id;
	u8 dynamic_qp_en;
};

/* OVS services */
struct sss_ovs_service_cap {
	struct sss_dev_ovs_svc_cap dev_ovs_cap;

	u32 pctx_size; /* 512B */
};

/* PF IPsec service resource */
struct sss_dev_ipsec_svc_cap {
	u32 max_sactx; /* max IPsec SA context num */
	u16 max_cq; /* max IPsec SCQC num */
	u16 rsvd0;
};

/* IPsec services */
struct sss_ipsec_service_cap {
	struct sss_dev_ipsec_svc_cap dev_ipsec_cap;
	u32 sactx_size; /* 512B */
};

#endif
