/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef CFG_MGMT_MPU_CMD_DEFS_H
#define CFG_MGMT_MPU_CMD_DEFS_H

#include "mpu_cmd_base_defs.h"

enum servic_bit_define {
	SERVICE_BIT_NIC		= 0,
	SERVICE_BIT_ROCE	= 1,
	SERVICE_BIT_VBS		= 2,
	SERVICE_BIT_TOE		= 3,
	SERVICE_BIT_IPSEC	= 4,
	SERVICE_BIT_FC		= 5,
	SERVICE_BIT_VIRTIO	= 6,
	SERVICE_BIT_OVS		= 7,
	SERVICE_BIT_NVME	= 8,
	SERVICE_BIT_ROCEAA	= 9,
	SERVICE_BIT_CURRENET	= 10,
	SERVICE_BIT_PPA		= 11,
	SERVICE_BIT_MIGRATE	= 12,
	SERVICE_BIT_VROCE	= 13,
	SERVICE_BIT_BIFUR	= 14,
	SERVICE_BIT_MAX
};

#define CFG_SERVICE_MASK_NIC		(0x1 << SERVICE_BIT_NIC)
#define CFG_SERVICE_MASK_ROCE		(0x1 << SERVICE_BIT_ROCE)
#define CFG_SERVICE_MASK_VBS		(0x1 << SERVICE_BIT_VBS)
#define CFG_SERVICE_MASK_TOE		(0x1 << SERVICE_BIT_TOE)
#define CFG_SERVICE_MASK_IPSEC		(0x1 << SERVICE_BIT_IPSEC)
#define CFG_SERVICE_MASK_FC		(0x1 << SERVICE_BIT_FC)
#define CFG_SERVICE_MASK_VIRTIO		(0x1 << SERVICE_BIT_VIRTIO)
#define CFG_SERVICE_MASK_OVS		(0x1 << SERVICE_BIT_OVS)
#define CFG_SERVICE_MASK_NVME		(0x1 << SERVICE_BIT_NVME)
#define CFG_SERVICE_MASK_ROCEAA		(0x1 << SERVICE_BIT_ROCEAA)
#define CFG_SERVICE_MASK_CURRENET	(0x1 << SERVICE_BIT_CURRENET)
#define CFG_SERVICE_MASK_PPA		(0x1 << SERVICE_BIT_PPA)
#define CFG_SERVICE_MASK_MIGRATE	(0x1 << SERVICE_BIT_MIGRATE)
#define CFG_SERVICE_MASK_VROCE		(0x1 << SERVICE_BIT_VROCE)
#define CFG_SERVICE_MASK_BIFUR		(0x1 << SERVICE_BIT_BIFUR)

/* Definition of the scenario ID in the cfg_data, which is used for SML memory allocation. */
enum scenes_id_define {
	SCENES_ID_FPGA_ETH		= 0,
	SCENES_ID_COMPUTE_STANDARD	= 1,
	SCENES_ID_STORAGE_ROCEAA_2x100	= 2,
	SCENES_ID_STORAGE_ROCEAA_4x25	= 3,
	SCENES_ID_CLOUD			= 4,
	SCENES_ID_FC			= 5,
	SCENES_ID_STORAGE_ROCE		= 6,
	SCENES_ID_COMPUTE_ROCE		= 7,
	SCENES_ID_STORAGE_TOE		= 8,
	SCENES_ID_COMPUTE_DPU           = 100,
	SCENES_ID_COMPUTE_SMART_NIC     = 101,
	SCENES_ID_MAX
};

/* struct cfg_cmd_dev_cap.sf_svc_attr */
enum {
	SF_SVC_FT_BIT	= (1 << 0),
	SF_SVC_RDMA_BIT	= (1 << 1),
};

struct cfg_cmd_host_timer {
	struct mgmt_msg_head head;

	u8 host_id;
	u8 rsvd1;

	u8 timer_pf_num;
	u8 timer_pf_id_start;
	u16 timer_vf_num;
	u16 timer_vf_id_start;
	u32 rsvd2[8];
};

struct cfg_cmd_dev_cap {
	struct mgmt_msg_head head;

	u16 func_id;
	u16 rsvd1;

	/* Public resources */
	u8 host_id;
	u8 ep_id;
	u8 er_id;
	u8 port_id;

	u16 host_total_func;
	u8 host_pf_num;
	u8 pf_id_start;
	u16 host_vf_num;
	u16 vf_id_start;
	u8 host_oq_id_mask_val;
	u8 timer_en;
	u8 host_valid_bitmap;
	u8 rsvd_host;

	u16 svc_cap_en;
	u16 max_vf;
	u8 flexq_en;
	u8 valid_cos_bitmap;
	/* Reserved for func_valid_cos_bitmap */
	u8 port_cos_valid_bitmap;
	u8 rsvd_func1;
	u32 rsvd_func2;

	u8 sf_svc_attr;
	u8 func_sf_en;
	u8 lb_mode;
	u8 smf_pg;

	u32 max_conn_num;
	u16 max_stick2cache_num;
	u16 max_bfilter_start_addr;
	u16 bfilter_len;
	u16 hash_bucket_num;

	/* shared resource */
	u8 host_sf_en;
	u8 master_host_id;
	u8 srv_multi_host_mode;
	u8 virtio_vq_size;

	u8 hot_plug_disable;
	u8 bond_create_mode;
	u8 lro_enable;
	u8 os_hot_replace;

	u32 rsvd_func3[4];

	/* l2nic */
	u16 nic_max_sq_id;
	u16 nic_max_rq_id;
	u16 nic_default_num_queues;
	u16 outband_vlan_cfg_en;
	u32 rsvd2_nic[2];

	/* RoCE */
	u32 roce_max_qp;
	u32 roce_max_cq;
	u32 roce_max_srq;
	u32 roce_max_mpt;
	u32 roce_max_drc_qp;

	u32 roce_cmtt_cl_start;
	u32 roce_cmtt_cl_end;
	u32 roce_cmtt_cl_size;

	u32 roce_dmtt_cl_start;
	u32 roce_dmtt_cl_end;
	u32 roce_dmtt_cl_size;

	u32 roce_wqe_cl_start;
	u32 roce_wqe_cl_end;
	u32 roce_wqe_cl_size;
	u8 roce_srq_container_mode;
	u8 rsvd_roce1[3];
	u32 rsvd_roce2[5];

	/* IPsec */
	u32 ipsec_max_sactx;
	u16 ipsec_max_cq;
	u16 rsvd_ipsec1;
	u32 rsvd_ipsec[2];

	/* OVS */
	u32 ovs_max_qpc;
	u32 rsvd_ovs1[3];

	/* ToE */
	u32 toe_max_pctx;
	u32 toe_max_cq;
	u16 toe_max_srq;
	u16 toe_srq_id_start;
	u16 toe_max_mpt;
	u16 toe_rsvd_1;
	u32 toe_max_cctxt;
	u32 rsvd_toe[1];

	/* FC */
	u32 fc_max_pctx;
	u32 fc_max_scq;
	u32 fc_max_srq;

	u32 fc_max_cctx;
	u32 fc_cctx_id_start;

	u8 fc_vp_id_start;
	u8 fc_vp_id_end;
	u8 rsvd_fc1[2];
	u32 rsvd_fc2[5];

	/* VBS */
	u16 vbs_max_volq;
	u8  vbs_main_pf_enable;
	u8  vbs_vsock_pf_enable;
	u8  vbs_fushion_queue_pf_enable;
	u8  rsvd0_vbs;
	u16 rsvd1_vbs;
	u32 rsvd2_vbs[2];

	u16 fake_vf_start_id;
	u16 fake_vf_num;
	u32 fake_vf_max_pctx;
	u16 fake_vf_bfilter_start_addr;
	u16 fake_vf_bfilter_len;

	u32 map_host_id : 3;
	u32 fake_vf_en : 1;
	u32 fake_vf_start_bit : 4;
	u32 fake_vf_end_bit : 4;
	u32 fake_vf_page_bit : 4;
	u32 rsvd2 : 16;

	u32 rsvd_glb[7];
};

#endif
