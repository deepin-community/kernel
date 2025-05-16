/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2024 Huawei Technologies Co., Ltd */

#ifndef MPU_INBAND_CMD_DEFS_H
#define MPU_INBAND_CMD_DEFS_H

#include "mpu_cmd_base_defs.h"
#include "mpu_outband_ncsi_cmd_defs.h"

#define HARDWARE_ID_1XX3V100_TAG 31 /* 1xx3v100 tag */
#define DUMP_16B_PER_LINE	16
#define DUMP_8_VAR_PER_LINE	8
#define DUMP_4_VAR_PER_LINE	4
#define FW_UPDATE_MGMT_TIMEOUT	3000000U

#define FUNC_RESET_FLAG_MAX_VALUE ((1U << (RES_TYPE_MAX + 1)) - 1)
struct comm_cmd_func_reset {
	struct mgmt_msg_head head;
	u16 func_id; /**< function id */
	u16 rsvd1[3];
	u64 reset_flag; /**< reset function type flag @see enum func_reset_flag_e */
};

enum {
	COMM_F_API_CHAIN = 1U << 0,
	COMM_F_CLP = 1U << 1,
	COMM_F_CHANNEL_DETECT = 1U << 2,
	COMM_F_MBOX_SEGMENT = 1U << 3,
	COMM_F_CMDQ_NUM = 1U << 4,
	COMM_F_VIRTIO_VQ_SIZE = 1U << 5,
	COMM_F_ONLY_ENHANCE_CMDQ = 1U << 8,
	COMM_F_USE_REAL_RX_BUF_SIZE = 1U << 9,
	COMM_F_CMD_BUF_SIZE = 1U << 10,
};

#define COMM_MAX_FEATURE_QWORD 4
enum COMM_FEATURE_NEGO_OPCODE {
	COMM_FEATURE_NEGO_OPCODE_GET = 0,
	COMM_FEATURE_NEGO_OPCODE_SET = 1
};

struct comm_cmd_feature_nego {
	struct mgmt_msg_head head;
	u16 func_id;	/**< function id */
	u8 opcode;	/**< operate type 0: get, 1: set */
	u8 rsvd;
	u64 s_feature[COMM_MAX_FEATURE_QWORD]; /**< feature info */
};

struct comm_cmd_func_flr_set {
	struct mgmt_msg_head head;

	u16 func_id;	/**< function id */
	u8 type;	/**< 1: flr enable */
	u8 isall;	/**< flr type  0: specify PF and associated VF flr, 1: all functions flr */
	u32 rsvd;
};

struct comm_cmd_clear_doorbell {
	struct mgmt_msg_head head;

	u16 func_id; /**< function id */
	u16 rsvd1[3];
};

struct comm_cmd_clear_resource {
	struct mgmt_msg_head head;

	u16 func_id; /**< function id */
	u16 rsvd1[3];
};

struct comm_global_attr {
	u8 max_host_num;	/**< maximum number of host */
	u8 max_pf_num;		/**< maximum number of pf */
	u16 vf_id_start;	/**< VF function id start */

	u8 mgmt_host_node_id;	/**< node id */
	u8 cmdq_num;		/**< cmdq num */
	u16 cmd_buf_size;   /**< cmd buff size */

	u32 rsvd2[8];
};

struct comm_cmd_get_glb_attr {
	struct mgmt_msg_head head;
	struct comm_global_attr attr; /**< global attr @see struct comm_global_attr */
};

struct comm_cmd_ppf_flr_type_set {
	struct mgmt_msg_head head;

	u16 func_id;
	u8 func_service_type;
	u8 rsvd1;
	u32 ppf_flr_type; /**< function flr type 1:statefull 0:stateless */
};

struct comm_cmd_func_svc_used_state {
	struct mgmt_msg_head head;
	u16 func_id;
	u16 svc_type;
	u8 used_state;
	u8 rsvd[35];
};

struct comm_cmd_cfg_msix_num {
	struct comm_info_head head;

	u16 func_id;
	u8 op_code; /**< operate type 1: alloc 0: free */
	u8 rsvd0;

	u16 msix_num;
	u16 rsvd1;
};

struct cmdq_ctxt_info {
	u64 curr_wqe_page_pfn;
	u64 wq_block_pfn;
};

struct comm_cmd_cmdq_ctxt {
	struct mgmt_msg_head head;

	u16 func_id;
	u8 cmdq_id;
	u8 rsvd1[5];

	struct cmdq_ctxt_info ctxt;
};

struct enhance_cmdq_ctxt_info {
	u64	eq_cfg;
	u64	dfx_pi_ci;

	u64	pft_thd;
	u64	pft_ci;

	u64	rsv;
	u64	ci_cla_addr;
};

struct comm_cmd_enhance_cmdq_ctxt {
	struct mgmt_msg_head head;

	u16 func_id;
	u8 cmdq_id;
	u8 rsvd1[5];

	struct enhance_cmdq_ctxt_info ctxt;
};

struct comm_cmd_root_ctxt {
	struct mgmt_msg_head head;

	u16 func_id;
	u8 set_cmdq_depth;
	u8 cmdq_depth;
	u16 rx_buf_sz;
	u8 lro_en;
	u8 cmdq_mode;
	u16 sq_depth;
	u16 rq_depth;
	u64 rsvd2;
};

struct comm_cmd_wq_page_size {
	struct mgmt_msg_head head;

	u16 func_id;	/**< function id */
	u8 opcode;	/**< operate type 0:get , 1:set */
	/* real_size=4KB*2^page_size, range(0~20) must be checked by driver */
	u8 page_size;

	u32 rsvd1;
};

struct comm_cmd_msix_config {
	struct mgmt_msg_head head;

	u16 func_id;	/**< function id */
	u8 opcode;	/**< operate type 0:get , 1:set */
	u8 rsvd1;
	u16 msix_index;
	u8 pending_cnt;
	u8 coalesce_timer_cnt;
	u8 resend_timer_cnt;
	u8 lli_timer_cnt;
	u8 lli_credit_cnt;
	u8 rsvd2[5];
};

struct comm_cmd_ceq_ctrl_reg {
	struct mgmt_msg_head head;

	u16 func_id; /**< function id */
	u16 q_id;
	u32 ctrl0;
	u32 ctrl1;
	u32 rsvd1;
};

struct comm_cmd_dma_attr_config {
	struct mgmt_msg_head head;

	u16 func_id; /**< function id */
	u8 entry_idx;
	u8 st;
	u8 at;
	u8 ph;
	u8 no_snooping;
	u8 tph_en;
	u32 resv1;
};

struct comm_cmd_ppf_tbl_htrp_config {
	struct mgmt_msg_head head;

	u32 hotreplace_flag;
};

struct comm_cmd_get_eqm_num {
	struct mgmt_msg_head head;

	u8 host_id; /**< host id */
	u8 rsvd1[3];
	u32 chunk_num;
	u32 search_gpa_num;
};

struct comm_cmd_eqm_cfg {
	struct mgmt_msg_head head;

	u8 host_id;	/**< host id */
	u8 valid;	/**< 0:clear config , 1:set config */
	u16 rsvd1;
	u32 page_size;	/**< page size */
	u32 rsvd2;
};

struct comm_cmd_eqm_search_gpa {
	struct mgmt_msg_head head;

	u8 host_id;	/**< host id Deprecated field, not used */
	u8 rsvd1[3];
	u32 start_idx;	/**< start index */
	u32 num;
	u32 rsvd2;
	u64 gpa_hi52[0];	/**< [gpa data */
};

struct comm_cmd_ppf_tmr_op {
	struct mgmt_msg_head head;

	u8 ppf_id;	/**< ppf function id */
	u8 opcode;	/**< operation type  1: start timer, 0: stop timer */
	u8 rsvd1[6];
};

struct comm_cmd_ht_gpa {
	struct mgmt_msg_head head;

	u8 host_id;	/**< host id */
	u8 rsvd0[3];
	u32 rsvd1[7];
	u64 page_pa0;
	u64 page_pa1;
};

struct comm_cmd_func_tmr_bitmap_op {
	struct mgmt_msg_head head;

	u16 func_id;	/**< function id */
	u8 opcode;	/**< operation type  1: start timer, 0: stop timer */
	u8 rsvd1[5];
};

#define DD_CFG_TEMPLATE_MAX_IDX 12
#define DD_CFG_TEMPLATE_MAX_TXT_LEN 64
#define CFG_TEMPLATE_OP_QUERY 0
#define CFG_TEMPLATE_OP_SET 1
#define CFG_TEMPLATE_SET_MODE_BY_IDX 0
#define CFG_TEMPLATE_SET_MODE_BY_NAME 1

struct comm_cmd_cfg_template {
	struct mgmt_msg_head head;
	u8 opt_type;	/**< operation type 0: query  1: set */
	u8 set_mode;	/**< set mode 0:index mode 1:name mode. */
	u8 tp_err;
	u8 rsvd0;

	u8 cur_index;	/**< current cfg tempalte index. */
	u8 cur_max_index;	/** max support cfg tempalte index. */
	u8 rsvd1[2];
	u8 cur_name[DD_CFG_TEMPLATE_MAX_TXT_LEN]; /**< current cfg tempalte name. */
	u8 cur_cfg_temp_info[DD_CFG_TEMPLATE_MAX_IDX][DD_CFG_TEMPLATE_MAX_TXT_LEN];

	u8 next_index;		/**< next reset cfg tempalte index. */
	u8 next_max_index;	/**< max support cfg tempalte index. */
	u8 rsvd2[2];
	u8 next_name[DD_CFG_TEMPLATE_MAX_TXT_LEN]; /**< next reset cfg tempalte name. */
	u8 next_cfg_temp_info[DD_CFG_TEMPLATE_MAX_IDX][DD_CFG_TEMPLATE_MAX_TXT_LEN];
};

#define MQM_SUPPORT_COS_NUM 8
#define MQM_INVALID_WEIGHT 256
#define MQM_LIMIT_SET_FLAG_READ 0
#define MQM_LIMIT_SET_FLAG_WRITE 1
struct comm_cmd_set_mqm_limit {
	struct mgmt_msg_head head;

	u16 set_flag;	/**< operation type 0: read  1: write */
	u16 func_id;	/**< function id */
	/* Indicates the weight of cos_id. The value ranges from 0 to 255.
	 * The value 0 indicates SP scheduling.
	 */
	u16 cos_weight[MQM_SUPPORT_COS_NUM]; /**< cos weight range[0,255] */
	u32 host_min_rate; /**< current host minimum rate */
	u32 func_min_rate; /**< current function minimum rate,unit:Mbps */
	u32 func_max_rate; /**< current function maximum rate,unit:Mbps */
	u8 rsvd[64]; /* Reserved */
};

#define HINIC3_FW_VERSION_LEN 16
#define HINIC3_FW_COMPILE_TIME_LEN 20

enum hinic3_fw_ver_type {
	HINIC3_FW_VER_TYPE_BOOT,
	HINIC3_FW_VER_TYPE_MPU,
	HINIC3_FW_VER_TYPE_NPU,
	HINIC3_FW_VER_TYPE_SMU_L0,
	HINIC3_FW_VER_TYPE_SMU_L1,
	HINIC3_FW_VER_TYPE_CFG,
};

struct comm_cmd_get_fw_version {
	struct mgmt_msg_head head;

	u16 fw_type; /**< firmware type  @see enum hinic3_fw_ver_type */
	u16 fw_dfx_vld : 1; /**< 0: release, 1: debug */
	u16 rsvd1 : 15;
	u8 ver[HINIC3_FW_VERSION_LEN]; /**< firmware version */
	u8 time[HINIC3_FW_COMPILE_TIME_LEN]; /**< firmware compile time */
};

struct hinic3_board_info {
	u8 board_type;		/**< board type */
	u8 port_num;		/**< current port number */
	u8 port_speed;		/**< port speed */
	u8 pcie_width;		/**< pcie width */
	u8 host_num;		/**< host number */
	u8 pf_num;		/**< pf number */
	u16 vf_total_num;	/**< vf total number */
	u8 tile_num;		/**< tile number */
	u8 qcm_num;		/**< qcm number */
	u8 core_num;		/**< core number */
	u8 work_mode;		/**< work mode */
	u8 service_mode;	/**< service mode */
	u8 pcie_mode;		/**< pcie mode */
	u8 boot_sel;		/**< boot sel */
	u8 board_id;		/**< board id */
	u32 rsvd;
	u32 service_en_bitmap;	/**< service en bitmap */
	u8 scenes_id;		/**< scenes id */
	u8 cfg_template_id;	/**< cfg template index */
	u8 hardware_id;		/**< hardware id */
	u8 spu_en;		/**< spu enable flag */
	u16 pf_vendor_id;	/**< pf vendor id */
	u8 tile_bitmap;		/**< used tile bitmap */
	u8 sm_bitmap;		/**< used sm bitmap */
};

struct comm_cmd_board_info {
	struct mgmt_msg_head head;

	struct hinic3_board_info info; /**< board info  @see struct hinic3_board_info */
	u32 rsvd[22];
};

struct comm_cmd_sync_time {
	struct mgmt_msg_head head;

	u64 mstime;	/**< time,unit:ms */
	u64 rsvd1;
};

struct hw_pf_info {
	u16 glb_func_idx;	/**< function id */
	u16 glb_pf_vf_offset;
	u8 p2p_idx;
	u8 itf_idx;		/**< host id */
	u16 max_vfs;		/**< max vf number */
	u16 max_queue_num;	/**< max queue number */
	u16 vf_max_queue_num;
	u16 port_id;
	u16 rsvd0;
	u32 pf_service_en_bitmap;
	u32 vf_service_en_bitmap;
	u16 rsvd1[2];

	u8 device_type;
	u8 bus_num;		/**< bdf info */
	u16 vf_stride;		/**< vf stride */
	u16 vf_offset;		/**< vf offset */
	u8 rsvd[2];
};

#define CMD_MAX_MAX_PF_NUM 32
struct hinic3_hw_pf_infos {
	u8 num_pfs;		/**< pf number */
	u8 rsvd1[3];

	struct hw_pf_info infos[CMD_MAX_MAX_PF_NUM]; /**< pf info  @see struct hw_pf_info */
};

struct comm_cmd_hw_pf_infos {
	struct mgmt_msg_head head;

	struct hinic3_hw_pf_infos infos; /**< all pf info  @see struct hinic3_hw_pf_infos */
};

struct comm_cmd_bdf_info {
	struct mgmt_msg_head head;

	u16 function_idx; /**< function id */
	u8 rsvd1[2];
	u8 bus;		/**< bus info */
	u8 device;	/**< device info */
	u8 function;	/**< function info */
	u8 rsvd2[5];
};

struct mpu_pcie_pf_info {
	u32 device_id;  /**< device id */
	u32 vendor_id;  /**< vendor id */
};

struct mpu_pcie_device_info {
	u8 status;
	u8 version;
	u8 rep_aeq_num;
	u8 rsvd[5];

	struct mpu_pcie_pf_info pf_info[32];
	u32 bus_id; /**< bus id */
	u32 pf_num; /**< pf index */
};

#define TABLE_INDEX_MAX 129
struct sml_table_id_info {
	u8 node_id;
	u8 instance_id;
};

struct comm_cmd_get_sml_tbl_data {
	struct comm_info_head head; /* 8B */
	u8 tbl_data[512]; /**< sml table data */
};

struct comm_cmd_sdi_info {
	struct mgmt_msg_head head;
	u32 cfg_sdi_mode; /**< host mode, 0:normal 1:virtual machine 2:bare metal */
};

struct comm_cmd_root_ctx_load_req {
	struct comm_info_head head;
	u32 func_id;	/**< function id */
	u32 smf_id;	/**< smf id */
	u32 queue_id;	/**< queue id */
};

#define HINIC_OVS_BOND_DEFAULT_ID 1
struct hinic3_hw_bond_infos {
	u8 bond_id;
	u8 valid;
	u8 rsvd1[2];
};

struct comm_cmd_hw_bond_infos {
	struct mgmt_msg_head head;
	struct hinic3_hw_bond_infos infos; /**< bond info  @see struct hinic3_hw_bond_infos */
};

/* 工具数据长度为1536（1.5K），工具最大发2k，包含头部 */
struct cmd_update_fw {
	struct comm_info_head head; // 8B
	u16 fw_flag;	/**< subfirmware flag, bit 0: last slice flag, bit 1 first slice flag */
	u16 slice_len;	/**< current slice length */
	u32 fw_crc;	/**< subfirmware crc */
	u32 fw_type;	/**< subfirmware type */
	u32 bin_total_len;	/**< total firmware length, only fisrt slice is effective */
	u32 bin_section_len;	/**< subfirmware length */
	u32 fw_verion;		/**< subfirmware version */
	u32 fw_offset;		/**< current slice offset of current subfirmware */
	u32 data[0];		/**< data */
};

struct cmd_switch_cfg {
	struct comm_info_head msg_head;
	u8 index; /**< index, range[0,7] */
	u8 data[7];
};

struct cmd_active_firmware {
	struct comm_info_head msg_head;
	u8 index; /* 0 ~ 7 */
	u8 data[7];
};

#define HOT_ACTIVE_MPU 1
#define HOT_ACTIVE_NPU 2
#define HOT_ACTIVE_MNPU 3
struct cmd_hot_active_fw {
	struct comm_info_head head;
	u32 type;	/**< hot actice firmware type 1: mpu; 2: ucode; 3: mpu & npu */
	u32 data[3];
};

#define FLASH_CHECK_OK 1
#define FLASH_CHECK_ERR 2
#define FLASH_CHECK_DISMATCH 3

struct comm_info_check_flash {
	struct comm_info_head head;

	u8 status; /**< flash check status */
	u8 rsv[3];
};

struct cmd_get_mpu_git_code {
	struct comm_info_head head; /* 8B */
	u32 rsvd;		/* reserve */
	char mpu_git_code[64];	/**< mpu git tag and compile time */
};

#define DATA_LEN_1K 1024
struct comm_info_sw_watchdog {
	struct comm_info_head head;

	u32 curr_time_h;	/**< infinite loop occurrence time,cycle */
	u32 curr_time_l;	/**< infinite loop occurrence time,cycle */
	u32 task_id;		/**< task id .task that occur in an infinite loop */
	u32 rsv;

	u64 pc;

	u64 elr;
	u64 spsr;
	u64 far;
	u64 esr;
	u64 xzr;
	u64 x30;
	u64 x29;
	u64 x28;
	u64 x27;
	u64 x26;
	u64 x25;
	u64 x24;
	u64 x23;
	u64 x22;
	u64 x21;
	u64 x20;
	u64 x19;
	u64 x18;
	u64 x17;
	u64 x16;
	u64 x15;
	u64 x14;
	u64 x13;
	u64 x12;
	u64 x11;
	u64 x10;
	u64 x09;
	u64 x08;
	u64 x07;
	u64 x06;
	u64 x05;
	u64 x04;
	u64 x03;
	u64 x02;
	u64 x01;
	u64 x00;

	u64 stack_top;		/**< stack top */
	u64 stack_bottom;	/**< stack bottom */
	u64 sp;			/**< sp pointer */
	u32 curr_used;		/**< the size currently used by the stack */
	u32 peak_used;		/**< historical peak of stack usage */
	u32 is_overflow;	/**< stack overflow flag */

	u32 stack_actlen;	/**< actual stack length(<=1024) */
	u8 stack_data[DATA_LEN_1K]; /* If the value exceeds 1024, it will be truncated. */
};

struct nic_log_info_request {
	u8 status;
	u8 version;
	u8 rsvd0[6];

	u32 offset;
	u8 log_or_index;	/* 0:log 1:index */
	u8 type;	/* log type 0:up 1:ucode 2:smu 3:mpu lastword 4.npu lastword */
	u8 area;	/* area 0:ram 1:flash (this bit is valid only when log_or_index is 0) */
	u8 rsvd1;	/* reserved */
};

#define MPU_TEMP_OP_GET 0
#define MPU_TEMP_THRESHOLD_OP_CFG 1
struct comm_temp_in_info {
	struct comm_info_head head;
	u8 opt_type;	/**< operation type 0:read operation 1:cfg operation */
	u8 rsv[3];
	s32 max_temp;	/**< maximum threshold of temperature */
	s32 min_temp;	/**< minimum threshold of temperature */
};

struct comm_temp_out_info {
	struct comm_info_head head;
	s32 temp_data;		/**< current temperature */
	s32 max_temp_threshold;	/**< maximum threshold of temperature */
	s32 min_temp_threshold;	/**< minimum threshold of temperature */
	s32 max_temp;		/**< maximum  temperature */
	s32 min_temp;		/**< minimum temperature */
};

/* 关闭芯片自复位 */
struct comm_cmd_enable_auto_rst_chip {
	struct comm_info_head head;
	u8 op_code;	/**< operation type 0:get operation 1:set operation */
	u8 enable;	/* auto reset status 0: disable auto reset chip 1: enable */
	u8 rsvd[2];
};

struct comm_chip_id_info {
	struct comm_info_head head;
	u8 chip_id; /**< chip id */
	u8 rsvd[3];
};

struct mpu_log_status_info {
	struct comm_info_head head;
	u8 type;	/**< operation type 0:read operation 1:write operation */
	u8 log_status;	/**< log status 0:idle 1:busy */
	u8 rsvd[2];
};

struct comm_cmd_msix_info {
	struct comm_info_head head;
	u8 rsvd1;
	u8 flag;	/**< table flag 0:second table, 1:actual table */
	u8 rsvd[2];
};

struct comm_cmd_channel_detect {
	struct mgmt_msg_head head;

	u16 func_id;	/**< function id */
	u16 rsvd1[3];
	u32 rsvd2[2];
};

#define MAX_LOG_BUF_SIZE 1024
#define FLASH_NPU_COUNTER_HEAD_MAGIC (0x5a)
#define FLASH_NPU_COUNTER_NIC_TYPE 0
#define FLASH_NPU_COUNTER_FC_TYPE 1

struct flash_npu_counter_head_s {
	u8 magic;
	u8 tbl_type;
	u8 count_type;	/**< 0：nic；1：fc */
	u8 count_num;	/**< current count number */
	u16 base_offset;	/**< address offset */
	u16 base_count;
};

struct flash_counter_info {
	struct comm_info_head head;

	u32 length; /**< flash counter buff len */
	u32 offset; /**< flash counter buff offset */
	u8 data[MAX_LOG_BUF_SIZE]; /**< flash counter data */
};

enum mpu_sm_cmd_type {
	COMM_SM_CTR_RD16 = 1,
	COMM_SM_CTR_RD32,
	COMM_SM_CTR_RD64_PAIR,
	COMM_SM_CTR_RD64,
	COMM_SM_CTR_RD32_CLEAR,
	COMM_SM_CTR_RD64_PAIR_CLEAR,
	COMM_SM_CTR_RD64_CLEAR,
	COMM_SM_CTR_RD16_CLEAR,
};

struct comm_read_ucode_sm_req {
	struct mgmt_msg_head msg_head;

	u32 node;	/**< node id @see enum INTERNAL_RING_NODE_ID_E */
	u32 count_id;	/**< count id */
	u32 instanse;	/**< instance id */
	u32 type;	/**< read type @see enum mpu_sm_cmd_type */
};

struct comm_read_ucode_sm_resp {
	struct mgmt_msg_head msg_head;

	u64 val1;
	u64 val2;
};

#define PER_REQ_MAX_DATA_LEN 0x600

struct comm_read_ucode_sm_per_req {
	struct mgmt_msg_head msg_head;

	u32 tbl_type;
	u32 count_id;
};

struct comm_read_ucode_sm_per_resp {
	struct mgmt_msg_head msg_head;

	u8 data[PER_REQ_MAX_DATA_LEN];
};

struct ucode_sm_counter_get_info {
	u32 width_type;
	u32 tbl_type;
	unsigned int base_count;
	unsigned int count_num;
};

enum log_type {
	MPU_LOG_CLEAR = 0,
	SMU_LOG_CLEAR = 1,
	NPU_LOG_CLEAR = 2,
	SPU_LOG_CLEAR = 3,
	ALL_LOG_CLEAR = 4,
};

struct comm_cmd_clear_log {
	struct comm_info_head head;
	u32 type;	/**< log type @see enum log_type */
};

#define ABLESWITCH 1
#define IMABLESWITCH 2
enum switch_workmode_op {
	SWITCH_WORKMODE_SWITCH = 0,
	SWITCH_WORKMODE_OTHER = 1
};

enum switch_workmode_obj {
	SWITCH_WORKMODE_FC = 0,
	SWITCH_WORKMODE_TOE = 1,
	SWITCH_WORKMODE_ROCE_AND_NOF = 2,
	SWITCH_WORKMODE_NOF_AA = 3,
	SWITCH_WORKMODE_ETH_CNTR = 4,
	SWITCH_WORKMODE_NOF_CNTR = 5,
};

struct comm_cmd_check_if_switch_workmode {
	struct mgmt_msg_head head;
	u8 switch_able;
	u8 rsvd1;
	u16 rsvd2[3];
	u32 rsvd3[3];
};

struct comm_cmd_switch_workmode {
	struct mgmt_msg_head head;
	u8 operation;
	u8 work_mode;
	u16 rsvd1[3];
	u32 rsvd2[3];
};

#define MIG_NOR_VM_ONE_MAX_SGE_MEM (64 * 8)
#define MIG_NOR_VM_ONE_MAX_MEM (MIG_NOR_VM_ONE_MAX_SGE_MEM + 16)
#define MIG_VM_MAX_SML_ENTRY_NUM 24

struct comm_cmd_migrate_dfx_s {
	struct mgmt_msg_head head;
	u32 hpa_entry_id; /**< hpa entry id */
	u8 vm_hpa[MIG_NOR_VM_ONE_MAX_MEM]; /**< vm hpa info */
};

#define BDF_BUS_BIT 8
struct pf_bdf_info {
	u8 itf_idx;		 /**< host id */
	u16 bdf;			/**< bdf info */
	u8 pf_bdf_info_vld; /**< pf bdf info valid */
};

struct vf_bdf_info {
	u16 glb_pf_vf_offset;	/**< global_func_id offset of 1st vf in pf */
	u16 max_vfs;		/**< vf number */
	u16 vf_stride;		/**< VF_RID_SETTING.vf_stride */
	u16 vf_offset;		/**< VF_RID_SETTING.vf_offset */
	u8 bus_num;		/**< tl_cfg_bus_num */
	u8 rsv[3];
};

struct cmd_get_bdf_info_s {
	struct mgmt_msg_head head;
	struct pf_bdf_info	pf_bdf_info[CMD_MAX_MAX_PF_NUM];
	struct vf_bdf_info	vf_bdf_info[CMD_MAX_MAX_PF_NUM];
	u32 vf_num; /**< vf num */
};

#define CPI_TCAM_DBG_CMD_SET_TASK_ENABLE_VALID 0x1
#define CPI_TCAM_DBG_CMD_SET_TIME_INTERVAL_VALID 0x2
#define CPI_TCAM_DBG_CMD_TYPE_SET 0
#define CPI_TCAM_DBG_CMD_TYPE_GET 1

struct comm_cmd_cpi_tcam_dbg {
	struct comm_info_head head;
	u8 type;
	u8 set_bitmap;
	u8 enable;
	u8 rsvd;
	u32 time_interval;
};

#define UDIE_ID_DATA_LEN 8
#define TDIE_ID_DATA_LEN 18
struct comm_cmd_get_die_id {
	struct comm_info_head head;

	u32 die_id_data[UDIE_ID_DATA_LEN]; /**< die id data */
};

struct comm_cmd_get_totem_die_id {
	struct comm_info_head head;

	u32 die_id_data[TDIE_ID_DATA_LEN]; /**< die id data */
};

#define MAX_EFUSE_INFO_BUF_SIZE 1024

enum comm_efuse_opt_type {
	EFUSE_OPT_UNICORN_EFUSE_BURN = 1,	/**< burn unicorn efuse bin */
	EFUSE_OPT_UPDATE_SWSB = 2,		/**< hw rotpk switch to guest rotpk */
	EFUSE_OPT_TOTEM_EFUSE_BURN = 3		/**< burn totem efuse bin */
};

struct comm_efuse_cfg_info {
	struct comm_info_head head;
	u8 opt_type;	/**< operation type @see enum comm_efuse_opt_type */
	u8 rsvd[3];
	u32 total_len;	/**< entire package leng value */
	u32 data_csum;	/**< data csum */
	u8 data[MAX_EFUSE_INFO_BUF_SIZE]; /**< efuse cfg data, size 768byte */
};

/* serloop模块接口 */
struct comm_cmd_hi30_serloop {
	struct comm_info_head head;

	u32 macro;
	u32 lane;
	u32 prbs_pattern;
	u32 result;
};

struct cmd_sector_info {
	struct comm_info_head head;
	u32 offset;	/**< flash addr */
	u32 len;	/**< flash length */
};

struct cmd_query_fw {
	struct comm_info_head head;
	u32 offset;	/**< offset addr */
	u32 len;	/**< length */
};

struct comm_cmd_get_cfg_info_t {
	struct comm_info_head head;
	u8 rsvd[8];
	u8 info[64];	/**< flash config info */
};

struct nic_cmd_get_uart_log_info {
	struct comm_info_head head;
	struct {
		u32 ret : 8;
		u32 version : 8;
		u32 log_elem_real_num : 16;
	} log_head;
	char uart_log[MAX_LOG_BUF_SIZE];
};

#define MAX_LOG_CMD_BUF_SIZE 128

struct nic_cmd_set_uart_log_cmd {
	struct comm_info_head head;
	struct {
		u32 ret : 8;
		u32 version : 8;
		u32 cmd_elem_real_num : 16;
	} log_head;
	char uart_cmd[MAX_LOG_CMD_BUF_SIZE];
};

struct dbgtool_up_reg_opt_info {
	struct comm_info_head head;

	u8 len;
	u8 is_car;
	u8 car_clear_flag;
	u32 csr_addr;	/**< register addr  */
	u32 csr_value;	/**< register value */
};

struct comm_info_reg_read_write {
	struct comm_info_head head;

	u32 reg_addr;	/**< register address */
	u32 val_length;	/**< register value length */

	u32 data[2];	/**< register value */
};

#ifndef DFX_MAG_MAX_REG_NUM
#define DFX_MAG_MAX_REG_NUM (32)
#endif
struct comm_info_dfx_mag_reg {
	struct comm_info_head head;
	u32 write;	/**< read or write flag: 0:read; 1:write */
	u32 reg_addr;	/**< register address */
	u32 reg_cnt;	/**< register num , up to 32 */
	u32 clear;	/**< clear flag: 0:do not clear after read  1:clear after read */
	u32 data[DFX_MAG_MAX_REG_NUM]; /**< register data */
};

struct comm_info_dfx_anlt_reg {
	struct comm_info_head head;
	u32 write;	/**< read or write flag: 0:read; 1:write */
	u32 reg_addr;	/**< register address */
	u32 reg_cnt;	/**< register num , up to 32 */
	u32 clear;	/**< clear flag: 0:do not clear after read  1:clear after read */
	u32 data[DFX_MAG_MAX_REG_NUM]; /**< register data */
};

#define MAX_DATA_NUM	(240)
struct csr_msg {
	struct {
		u32 node_id	: 5;	// [4:0]
		u32 data_width	: 10;	// [14:5]
		u32 rsvd	: 17;	// [31:15]
	} bits;
	u32 addr;
};

struct comm_cmd_heart_event {
	struct mgmt_msg_head head;

	u8 init_sta;	/* 0: mpu init ok, 1: mpu init error. */
	u8 rsvd1[3];
	u32 heart;	/* add one by one */
	u32 heart_handshake;	/* should be alwasys: 0x5A5A5A5A */
};

#define XREGS_NUM 31
struct tag_cpu_tick {
	u32 cnt_hi;
	u32 cnt_lo;
};

struct tag_ax_exc_reg_info {
	u64 ttbr0;
	u64 ttbr1;
	u64 tcr;
	u64 mair;
	u64 sctlr;
	u64 vbar;
	u64 current_el;
	u64 sp;
	/* The memory layout of the following fields is the same as that of TskContext. */
	u64 elr;		/* 返回地址 */
	u64 spsr;
	u64 far_r;
	u64 esr;
	u64 xzr;
	u64 xregs[XREGS_NUM];	/* 0~30: x30~x0 */
};

struct tag_exc_info {
	char os_ver[48];	/**< os version */
	char app_ver[64];	/**< application version*/
	u32 exc_cause;		/**< exception reason */
	u32 thread_type;	/**< Thread type before exception */
	u32 thread_id;		/**< Thread PID before exception */
	u16 byte_order;		/**< byte order */
	u16 cpu_type;		/**< CPU type */
	u32 cpu_id;		/**< CPU ID */
	struct tag_cpu_tick cpu_tick;	/**< CPU Tick */
	u32 nest_cnt;		/**< exception nesting count */
	u32 fatal_errno;	/**< fatal error code, valid when a fatal error occurs */
	u64 uw_sp;		/**< exception front stack pointer */
	u64 stack_bottom;	/**< bottom of stack before exception */
	/* Context information of the core register when an exception occurs.
	 * 82\57 must be located in byte 152, If any change is made,
	 * the OS_EXC_REGINFO_OFFSET macro in sre_platform.eh needs to be updated.
	 */
	struct tag_ax_exc_reg_info reg_info;	/**< register info @see EXC_REGS_S */
};

/* 上报给驱动的up lastword模块接口 */
#define MPU_LASTWORD_SIZE 1024
struct tag_comm_info_up_lastword {
	struct comm_info_head head;

	struct tag_exc_info stack_info;
	u32 stack_actlen;	/**< actual stack length (<=1024) */
	u8 stack_data[MPU_LASTWORD_SIZE];
};

struct comm_cmd_mbox_csr_rd_req {
	struct mgmt_msg_head head;
	struct csr_msg csr_info[MAX_DATA_NUM];
	u32 data_num;
};

struct comm_cmd_mbox_csr_wt_req {
	struct mgmt_msg_head head;
	struct csr_msg csr_info;
	u64 value;
};

struct comm_cmd_mbox_csr_rd_ret {
	struct mgmt_msg_head head;
	u64 value[MAX_DATA_NUM];
};

struct comm_cmd_mbox_csr_wt_ret {
	struct mgmt_msg_head head;
};

enum comm_virtio_dev_type {
	COMM_VIRTIO_NET_TYPE = 0,
	COMM_VIRTIO_BLK_TYPE = 1,
	COMM_VIRTIO_SCSI_TYPE = 4,
	COMM_VIRTIO_FS_TYPE = 5,
};

struct comm_virtio_dev_cmd {
	u16 device_type;	/**< device type @see enum comm_virtio_dev_type */
	u16 device_id;
	u32 devid_switch;
	u32 sub_vendor_id;
	u32 sub_class_code;
	u32 flash_en;
};

struct comm_virtio_dev_ctl {
	u32 device_type_mark;
	u32 devid_switch_mark;
	u32 sub_vendor_id_mark;
	u32 sub_class_code_mark;
	u32 flash_en_mark;
};

struct comm_cmd_set_virtio_dev {
	struct comm_info_head head;
	struct comm_virtio_dev_cmd virtio_dev_cmd;	/**<  @see struct comm_virtio_dev_cmd_s */
	struct comm_virtio_dev_ctl virtio_dev_ctl;	/**<  @see struct comm_virtio_dev_ctl_s */
};

/* Interfaces of the MAC Module */
#ifndef MAC_ADDRESS_BYTE_NUM
#define MAC_ADDRESS_BYTE_NUM (6)
#endif
struct comm_info_mac {
	struct comm_info_head head;

	u16 is_valid;
	u16 rsvd0;
	u8 data[MAC_ADDRESS_BYTE_NUM];
	u16 rsvd1;
};

struct cmd_patch_active {
	struct comm_info_head head;
	u32 fw_type;				/**< firmware type */
	u32 data[3];				/**< reserved */
};

struct cmd_patch_deactive {
	struct comm_info_head head;
	u32 fw_type;				/**< firmware type */
	u32 data[3];				/**< reserved */
};

struct cmd_patch_remove {
	struct comm_info_head head;
	u32 fw_type;				/**< firmware type */
	u32 data[3];				/**< reserved */
};

struct cmd_patch_sram_optimize {
	struct comm_info_head head;
	u32 data[4];				/**< reserved */
};

struct comm_cmd_con_sel_sta {
	struct comm_info_head head;
	u8 valid;
	u8 host_id; /**< host id, range[0,3] */
	u8 rsvd[2];
};

/* ncsi counter */
struct nsci_counter_in_info_s {
	struct comm_info_head head;
	u8 opt_type; /**< operate type 0:read counter 1:counter clear */
	u8 rsvd[3];
};

struct channel_status_check_info_s {
	struct comm_info_head head;
	u32 rsvd1;
	u32 rsvd2;
};

struct comm_cmd_compatible_info {
	struct mgmt_msg_head head;
	u8 chip_ver;
	u8 host_env;
	u8 rsv[13];

	u8 cmd_count;
	union {
		struct {
			u8 module;
			u8 mod_type;
			u16 cmd;
		} cmd_desc;
		u32 cmd_desc_val;
	} cmds_desc[24];
	u8 cmd_ver[24];
};

struct tag_ncsi_chan_info {
	u8 aen_en;	/**< aen enable */
	u8 index;	/**< index of channel */
	u8 port;	/**< net port number */
	u8 state;	/**< ncsi state */
	u8 ncsi_port_en;	/**< ncsi port enable flag (1:enable 0:disable) */
	u8 rsv[3];
	struct tag_ncsi_chan_capa capabilities;	/**< ncsi channel capabilities*/
	struct tg_g_ncsi_parameters parameters;	/**< ncsi state */
};

struct comm_cmd_ncsi_settings {
	u8 ncsi_ver;		/**< ncsi version */
	u8 ncsi_pkg_id;
	u8 arb_en;		/**< arbitration en */
	u8 duplex_set;		/**< duplex mode */
	u8 chan_num;		/**< Number of virtual channels */
	u8 iid;			/**< identify new instances of a command */
	u8 lldp_over_ncsi_enable;
	u8 lldp_over_mctp_enable;
	u32 magicwd;
	u8 lldp_tx_enable;
	u8 rsvd[3];
	u32 crc;
	struct tag_ncsi_chan_info ncsi_chan_info;
};

struct comm_cmd_ncsi_cfg {
	struct comm_info_head head;
	u8 ncsi_cable_state;	/**< ncsi cable status 0:cable out of place，1:cable in place */
	u8 setting_type;	/**< nsci info type:0:ram cofig, 1: flash config */
	u8 port;		/**< net port number */
	u8 erase_flag;		/**< flash erase flag, 1: erase flash info */
	struct comm_cmd_ncsi_settings setting_info;
};

#define MQM_ATT_PAGE_NUM	128

/* Maximum segment data length of the upgrade command */
#define MAX_FW_FRAGMENT_LEN (1536)

#endif
