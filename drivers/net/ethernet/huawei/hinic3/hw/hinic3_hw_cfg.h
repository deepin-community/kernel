/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_HW_CFG_H
#define HINIC3_HW_CFG_H

#include <linux/types.h>
#include "cfg_mgmt_mpu_cmd_defs.h"
#include "hinic3_hwdev.h"

#define CFG_MAX_CMD_TIMEOUT     30000 /* ms */

enum {
	CFG_FREE = 0,
	CFG_BUSY = 1
};

/* start position for CEQs allocation, Max number of CEQs is 32 */
enum {
	CFG_RDMA_CEQ_BASE       = 0
};

/* RDMA resource */
#define K_UNIT              BIT(10)
#define M_UNIT              BIT(20)
#define G_UNIT              BIT(30)

#define VIRTIO_BASE_VQ_SIZE 2048U
#define VIRTIO_DEFAULT_VQ_SIZE 8192U

/* L2NIC */
#define HINIC3_CFG_MAX_QP	256

/* RDMA */
#define RDMA_RSVD_QPS       2
#define ROCE_MAX_WQES       (8 * K_UNIT - 1)
#define IWARP_MAX_WQES      (8 * K_UNIT)

#define RDMA_MAX_SQ_SGE     16

#define ROCE_MAX_RQ_SGE     16

/* value changed should change ROCE_MAX_WQE_BB_PER_WR synchronously */
#define RDMA_MAX_SQ_DESC_SZ (256)

/* (256B(cache_line_len) - 16B(ctrl_seg_len) - 48B(max_task_seg_len)) */
#define ROCE_MAX_SQ_INLINE_DATA_SZ   192

#define ROCE_MAX_RQ_DESC_SZ     256

#define ROCE_QPC_ENTRY_SZ       512

#define WQEBB_SZ                64

#define ROCE_RDMARC_ENTRY_SZ    32
#define ROCE_MAX_QP_INIT_RDMA   128
#define ROCE_MAX_QP_DEST_RDMA   128

#define ROCE_MAX_SRQ_WQES       (16 * K_UNIT - 1)
#define ROCE_RSVD_SRQS          0
#define ROCE_MAX_SRQ_SGE        15
#define ROCE_SRQC_ENTERY_SZ     64

#define RDMA_MAX_CQES       (8 * M_UNIT - 1)
#define RDMA_RSVD_CQS       0

#define RDMA_CQC_ENTRY_SZ   128

#define RDMA_CQE_SZ         64
#define RDMA_RSVD_MRWS      128
#define RDMA_MPT_ENTRY_SZ   64
#define RDMA_NUM_MTTS       (1 * G_UNIT)
#define LOG_MTT_SEG         9
#define MTT_ENTRY_SZ        8
#define LOG_RDMARC_SEG      3

#define LOCAL_ACK_DELAY     15
#define RDMA_NUM_PORTS      1
#define ROCE_MAX_MSG_SZ     (2 * G_UNIT)

#define DB_PAGE_SZ          (4 * K_UNIT)
#define DWQE_SZ             256

#define NUM_PD              (128 * K_UNIT)
#define RSVD_PD             0

#define MAX_XRCDS           (64 * K_UNIT)
#define RSVD_XRCDS          0

#define MAX_GID_PER_PORT    128
#define GID_ENTRY_SZ        32
#define RSVD_LKEY           ((RDMA_RSVD_MRWS - 1) << 8)
#define NUM_COMP_VECTORS    32
#define PAGE_SZ_CAP         ((1UL << 12) | (1UL << 16) | (1UL << 21))
#define ROCE_MODE           1

#define MAX_FRPL_LEN        511
#define MAX_PKEYS           1

/* ToE */
#define TOE_PCTX_SZ         1024
#define TOE_CQC_SZ          64

/* IoE */
#define IOE_PCTX_SZ         512

/* FC */
#define FC_PCTX_SZ          256
#define FC_CCTX_SZ          256
#define FC_SQE_SZ           128
#define FC_SCQC_SZ          64
#define FC_SCQE_SZ          64
#define FC_SRQC_SZ          64
#define FC_SRQE_SZ          32

/* OVS */
#define OVS_PCTX_SZ         512

/* PPA */
#define PPA_PCTX_SZ         512

/* IPsec */
#define IPSEC_SACTX_SZ      512

struct dev_sf_svc_attr {
	bool ft_en;     /* business enable flag (not include RDMA) */
	bool ft_pf_en;  /* In FPGA Test VF resource is in PF or not,
			 * 0 - VF, 1 - PF, VF doesn't need this bit.
			 */
	bool rdma_en;
	bool rdma_pf_en;/* In FPGA Test VF RDMA resource is in PF or not,
			 * 0 - VF, 1 - PF, VF doesn't need this bit.
			 */
};

enum intr_type {
	INTR_TYPE_MSIX,
	INTR_TYPE_MSI,
	INTR_TYPE_INT,
	INTR_TYPE_NONE,
	/* PXE,OVS need single thread processing,
	 * synchronization messages must use poll wait mechanism interface
	 */
};

/* device capability */
struct service_cap {
	struct dev_sf_svc_attr sf_svc_attr;
	u16 svc_type;      /* user input service type */
	u16 chip_svc_type; /* HW supported service type, reference to servic_bit_define */

	u8 host_id;
	u8 ep_id;
	u8 er_id;       /* PF/VF's ER */
	u8 port_id;     /* PF/VF's physical port */

	/* Host global resources */
	u16 host_total_function;
	u8 pf_num;
	u8 pf_id_start;
	u16 vf_num; /* max numbers of vf in current host */
	u16 vf_id_start;
	u8 host_oq_id_mask_val;
	u8 host_valid_bitmap;
	u8 master_host_id;
	u8 srv_multi_host_mode;
	u16 virtio_vq_size;

	u8 hot_plug_disable;
	u8 bond_create_mode;
	u8 os_hot_replace;
	u8 rsvd1;

	u8 timer_pf_num;
	u8 timer_pf_id_start;
	u16 timer_vf_num;
	u16 timer_vf_id_start;

	u8 flexq_en;
	u8 cos_valid_bitmap;
	u8 port_cos_valid_bitmap;
	u16 max_vf;      /* max VF number that PF supported */

	u16 fake_vf_start_id;
	u16 fake_vf_num;
	u32 fake_vf_max_pctx;
	u16 fake_vf_bfilter_start_addr;
	u16 fake_vf_bfilter_len;

	u16 fake_vf_num_cfg;

	/* DO NOT get interrupt_type from firmware */
	enum intr_type interrupt_type;

	bool sf_en;     /* stateful business status */
	u8 timer_en;    /* 0:disable, 1:enable */
	u8 bloomfilter_en; /* 0:disable, 1:enable */

	u8 lb_mode;
	u8 smf_pg;

	/* For test */
	u32 test_mode;
	u32 test_qpc_num;
	u32 test_qpc_resvd_num;
	u32 test_page_size_reorder;
	bool test_xid_alloc_mode;
	bool test_gpa_check_enable;
	u8 test_qpc_alloc_mode;
	u8 test_scqc_alloc_mode;

	u32 test_max_conn_num;
	u32 test_max_cache_conn_num;
	u32 test_scqc_num;
	u32 test_mpt_num;
	u32 test_scq_resvd_num;
	u32 test_mpt_recvd_num;
	u32 test_hash_num;
	u32 test_reorder_num;

	u32 max_connect_num; /* PF/VF maximum connection number(1M) */
	/* The maximum connections which can be stick to cache memory, max 1K */
	u16 max_stick2cache_num;
	/* Starting address in cache memory for bloom filter, 64Bytes aligned */
	u16 bfilter_start_addr;
	/* Length for bloom filter, aligned on 64Bytes. The size is length*64B.
	 * Bloom filter memory size + 1 must be power of 2.
	 * The maximum memory size of bloom filter is 4M
	 */
	u16 bfilter_len;
	/* The size of hash bucket tables, align on 64 entries.
	 * Be used to AND (&) the hash value. Bucket Size +1 must be power of 2.
	 * The maximum number of hash bucket is 4M
	 */
	u16 hash_bucket_num;

	u8 map_host_id;
	u8 fake_vf_en;
	u8 fake_vf_start_bit;
	u8 fake_vf_end_bit;
	u8 fake_vf_page_bit;

	struct nic_service_cap      nic_cap;            /* NIC capability */
	struct rdma_service_cap     rdma_cap;           /* RDMA capability */
	struct fc_service_cap       fc_cap;             /* FC capability */
	struct toe_service_cap      toe_cap;            /* ToE capability */
	struct ovs_service_cap      ovs_cap;            /* OVS capability */
	struct ipsec_service_cap    ipsec_cap;          /* IPsec capability */
	struct ppa_service_cap      ppa_cap;            /* PPA capability */
	struct vbs_service_cap      vbs_cap;            /* VBS capability */
};

struct svc_cap_info {
	u32 func_idx;
	struct service_cap cap;
};

struct cfg_eq {
	enum hinic3_service_type type;
	int eqn;
	int free; /* 1 - alocated, 0- freed */
};

struct cfg_eq_info {
	struct cfg_eq *eq;

	u8 num_ceq;

	u8 num_ceq_remain;

	/* mutex used for allocate EQs */
	struct mutex eq_mutex;
};

struct irq_alloc_info_st {
	enum hinic3_service_type type;
	int free;                /* 1 - alocated, 0- freed */
	struct irq_info info;
};

struct cfg_irq_info {
	struct irq_alloc_info_st *alloc_info;
	u16 num_total;
	u16 num_irq_remain;
	u16 num_irq_hw;          /* device max irq number */

	/* mutex used for allocate EQs */
	struct mutex irq_mutex;
};

#define VECTOR_THRESHOLD	2

struct cfg_mgmt_info {
	struct hinic3_hwdev *hwdev;
	struct service_cap  svc_cap;
	struct cfg_eq_info  eq_info;        /* EQ */
	struct cfg_irq_info irq_param_info; /* IRQ */
	u32 func_seq_num;                   /* temporary */
};

#define CFG_SERVICE_FT_EN	(CFG_SERVICE_MASK_VBS | CFG_SERVICE_MASK_TOE | \
				 CFG_SERVICE_MASK_IPSEC | CFG_SERVICE_MASK_FC | \
				 CFG_SERVICE_MASK_VIRTIO | CFG_SERVICE_MASK_OVS)
#define CFG_SERVICE_RDMA_EN	CFG_SERVICE_MASK_ROCE

#define IS_NIC_TYPE(dev) \
	(((u32)(dev)->cfg_mgmt->svc_cap.chip_svc_type) & CFG_SERVICE_MASK_NIC)
#define IS_ROCE_TYPE(dev) \
	(((u32)(dev)->cfg_mgmt->svc_cap.chip_svc_type) & CFG_SERVICE_MASK_ROCE)
#define IS_VBS_TYPE(dev) \
	(((u32)(dev)->cfg_mgmt->svc_cap.chip_svc_type) & CFG_SERVICE_MASK_VBS)
#define IS_TOE_TYPE(dev) \
	(((u32)(dev)->cfg_mgmt->svc_cap.chip_svc_type) & CFG_SERVICE_MASK_TOE)
#define IS_IPSEC_TYPE(dev) \
	(((u32)(dev)->cfg_mgmt->svc_cap.chip_svc_type) & CFG_SERVICE_MASK_IPSEC)
#define IS_FC_TYPE(dev) \
	(((u32)(dev)->cfg_mgmt->svc_cap.chip_svc_type) & CFG_SERVICE_MASK_FC)
#define IS_OVS_TYPE(dev) \
	(((u32)(dev)->cfg_mgmt->svc_cap.chip_svc_type) & CFG_SERVICE_MASK_OVS)
#define IS_FT_TYPE(dev) \
	(((u32)(dev)->cfg_mgmt->svc_cap.chip_svc_type) & CFG_SERVICE_FT_EN)
#define IS_RDMA_TYPE(dev) \
	(((u32)(dev)->cfg_mgmt->svc_cap.chip_svc_type) & CFG_SERVICE_RDMA_EN)
#define IS_RDMA_ENABLE(dev) \
	((dev)->cfg_mgmt->svc_cap.sf_svc_attr.rdma_en)
#define IS_PPA_TYPE(dev) \
		(((u32)(dev)->cfg_mgmt->svc_cap.chip_svc_type) & CFG_SERVICE_MASK_PPA)
#define IS_MIGR_TYPE(dev) \
		(((u32)(dev)->cfg_mgmt->svc_cap.chip_svc_type) & CFG_SERVICE_MASK_MIGRATE)
#define IS_BIFUR_TYPE(dev) \
		(((u32)(dev)->cfg_mgmt->svc_cap.chip_svc_type) & CFG_SERVICE_MASK_BIFUR)

int init_cfg_mgmt(struct hinic3_hwdev *dev);

void free_cfg_mgmt(struct hinic3_hwdev *dev);

int init_capability(struct hinic3_hwdev *dev);

void free_capability(struct hinic3_hwdev *dev);

int hinic3_init_vf_dev_cap(void *hwdev);

u8 hinic3_get_bond_create_mode(void *dev);

#endif

