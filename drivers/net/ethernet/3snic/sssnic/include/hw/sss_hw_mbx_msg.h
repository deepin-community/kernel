/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HW_MBX_MSG_H
#define SSS_HW_MBX_MSG_H

#include <linux/types.h>

#define SSS_MGMT_MSG_SET_CMD		1
#define SSS_MGMT_MSG_GET_CMD		0

#define SSS_MGMT_CMD_SUCCESS	0

struct sss_mgmt_msg_head {
	u8 state;
	u8 version;
	u8 rsvd0[6];
};

struct sss_cmd_func_reset {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u16 rsvd[3];
	u64 reset_flag;
};

enum {
	SSS_COMM_F_ADM = 1U << 0,
	SSS_COMM_F_CLP = 1U << 1,
	SSS_COMM_F_CHANNEL_DETECT = 1U << 2,
	SSS_COMM_F_MBX_SEGMENT = 1U << 3,
	SSS_COMM_F_CTRLQ_NUM = 1U << 4,
	SSS_COMM_F_VIRTIO_VQ_SIZE = 1U << 5,
};

#define SSS_MAX_FEATURE_QWORD 4
struct sss_cmd_feature_nego {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 opcode; /* 1: set, 0: get */
	u8 rsvd;
	u64 feature[SSS_MAX_FEATURE_QWORD];
};

struct sss_cmd_clear_doorbell {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u16 rsvd[3];
};

struct sss_cmd_clear_resource {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u16 rsvd[3];
};

struct sss_comm_global_attr {
	u8 max_host_num;
	u8 max_pf_num;
	u16 vf_id_start;

	u8 mgmt_host_node_id; /* for adm msg to mgmt cpu */
	u8 ctrlq_num;
	u8 rsvd1[2];
	u32 rsvd2[8];
};

struct sss_cmd_channel_detect {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u16 rsvd1[3];
	u32 rsvd2[2];
};

enum sss_svc_type {
	SSS_SVC_TYPE_COM = 0,
	SSS_SVC_TYPE_NIC,
	SSS_SVC_TYPE_OVS,
	SSS_SVC_TYPE_ROCE,
	SSS_SVC_TYPE_TOE,
	SSS_SVC_TYPE_IOE,
	SSS_SVC_TYPE_FC,
	SSS_SVC_TYPE_VBS,
	SSS_SVC_TYPE_IPSEC,
	SSS_SVC_TYPE_VIRTIO,
	SSS_SVC_TYPE_MIGRATE,
	SSS_SVC_TYPE_PPA,
	SSS_SVC_TYPE_MAX,
};

struct sss_cmd_func_svc_used_state {
	struct sss_mgmt_msg_head head;
	u16 func_id;
	u16 svc_type;
	u8 used_state;
	u8 rsvd[35];
};

struct sss_cmd_get_glb_attr {
	struct sss_mgmt_msg_head head;

	struct sss_comm_global_attr attr;
};

enum sss_fw_ver_type {
	SSS_FW_VER_TYPE_BOOT,
	SSS_FW_VER_TYPE_MPU,
	SSS_FW_VER_TYPE_NPU,
	SSS_FW_VER_TYPE_SMU_L0,
	SSS_FW_VER_TYPE_SMU_L1,
	SSS_FW_VER_TYPE_CFG,
};

#define SSS_FW_VERSION_LEN 16
#define SSS_FW_COMPILE_TIME_LEN 20
struct sss_cmd_get_fw_version {
	struct sss_mgmt_msg_head head;

	u16 fw_type;
	u16 rsvd;
	u8 ver[SSS_FW_VERSION_LEN];
	u8 time[SSS_FW_COMPILE_TIME_LEN];
};

/* hardware define: ctrlq context */
struct sss_ctrlq_ctxt_info {
	u64 curr_wqe_page_pfn;
	u64 wq_block_pfn;
};

struct sss_cmd_ctrlq_ctxt {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 ctrlq_id;
	u8 rsvd[5];

	struct sss_ctrlq_ctxt_info ctxt;
};

struct sss_cmd_root_ctxt {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 set_ctrlq_depth;
	u8 ctrlq_depth;
	u16 rx_buf_sz;
	u8 lro_en;
	u8 rsvd1;
	u16 sq_depth;
	u16 rq_depth;
	u64 rsvd2;
};

struct sss_cmd_wq_page_size {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 opcode;
	u8 page_size;

	u32 rsvd;
};

struct sss_cmd_msix_config {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 opcode;
	u8 rsvd1;
	u16 msix_index;
	u8 pending_cnt;
	u8 coalesce_timer_cnt;
	u8 resend_timer_cnt;
	u8 lli_timer_cnt;
	u8 lli_credit_cnt;
	u8 rsvd2[5];
};

struct sss_cmd_dma_attr_config {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 entry_id;
	u8 st;
	u8 at;
	u8 ph;
	u8 no_snooping;
	u8 tph_en;
	u32 resv;
};

struct sss_cmd_ceq_ctrl_reg {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u16 qid;
	u32 ctrl0;
	u32 ctrl1;
	u32 rsvd1;
};

struct sss_board_info {
	u8 board_type;
	u8 port_num;
	u8 port_speed;
	u8 pcie_width;
	u8 host_num;
	u8 pf_num;
	u16 vf_total_num;
	u8 tile_num;
	u8 qcm_num;
	u8 core_num;
	u8 work_mode;
	u8 service_mode;
	u8 pcie_mode;
	u8 boot_sel;
	u8 board_id;
	u32 cfg_addr;
	u32 service_en_bitmap;
	u8 scenes_id;
	u8 cfg_tmpl_id;
	u8 hw_id;
	u8 rsvd;
	u16 pf_vendor_id;
	u8 tile_bitmap;
	u8 sm_bitmap;
};

struct sss_cmd_board_info {
	struct sss_mgmt_msg_head head;

	struct sss_board_info info;
	u32 rsvd[22];
};

struct sss_cmd_sync_time {
	struct sss_mgmt_msg_head head;

	u64 mstime;
	u64 rsvd;
};

struct sss_cmd_bdf_info {
	struct sss_mgmt_msg_head head;

	u16 function_id;
	u8 rsvd1[2];
	u8 bus;
	u8 device;
	u8 function;
	u8 rsvd2[5];
};

#endif
