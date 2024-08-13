/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_TOOL_HW_H
#define SSS_TOOL_HW_H

#define SSS_TOOL_CMD_TYPE (0x18)

#define SSS_TOOL_PF_DEV_MAX  32
/* Indicates the maximum number of interrupts that can be recorded.
 * Subsequent interrupts are not recorded in FFM.
 */
#define SSS_TOOL_FFM_RECORD_MAX 64

#define SSS_TOOL_PF_INFO_MAX (16)
#define SSS_TOOL_BUSINFO_LEN (32)

#define SSS_TOOL_CHIP_FAULT_SIZE (110 * 1024)
#define SSS_TOOL_DRV_BUF_SIZE_MAX 4096

/* dbgtool command type */
/* You can add commands as required. The dbgtool command can be
 * used to invoke all interfaces of the kernel-mode x86 driver.
 */
enum sss_tool_dbg_cmd {
	SSS_TOOL_DBG_CMD_API_RD = 0,
	SSS_TOOL_DBG_CMD_API_WR,
	SSS_TOOL_DBG_CMD_FFM_RD,
	SSS_TOOL_DBG_CMD_FFM_CLR,
	SSS_TOOL_DBG_CMD_PF_DEV_INFO_GET,
	SSS_TOOL_DBG_CMD_MSG_2_UP,
	SSS_TOOL_DBG_CMD_FREE_MEM,
	SSS_TOOL_DBG_CMD_NUM
};

enum module_name {
	SSS_TOOL_MSG_TO_NPU = 1,
	SSS_TOOL_MSG_TO_MPU,
	SSS_TOOL_MSG_TO_SM,
	SSS_TOOL_MSG_TO_HW_DRIVER,
#define SSS_TOOL_MSG_TO_SRV_DRV_BASE (SSS_TOOL_MSG_TO_HW_DRIVER + 1)
	SSS_TOOL_MSG_TO_NIC_DRIVER = SSS_TOOL_MSG_TO_SRV_DRV_BASE,
	SSS_TOOL_MSG_TO_OVS_DRIVER,
	SSS_TOOL_MSG_TO_ROCE_DRIVER,
	SSS_TOOL_MSG_TO_TOE_DRIVER,
	SSS_TOOL_MSG_TO_IOE_DRIVER,
	SSS_TOOL_MSG_TO_FC_DRIVER,
	SSS_TOOL_MSG_TO_VBS_DRIVER,
	SSS_TOOL_MSG_TO_IPSEC_DRIVER,
	SSS_TOOL_MSG_TO_VIRTIO_DRIVER,
	SSS_TOOL_MSG_TO_MIGRATE_DRIVER,
	SSS_TOOL_MSG_TO_PPA_DRIVER,
	SSS_TOOL_MSG_TO_CUSTOM_DRIVER = SSS_TOOL_MSG_TO_SRV_DRV_BASE + 11,
	SSS_TOOL_MSG_TO_DRIVER_MAX = SSS_TOOL_MSG_TO_SRV_DRV_BASE + 15, /* reserved */
};

enum sss_tool_adm_msg_type {
	SSS_TOOL_ADM_MSG_READ,
	SSS_TOOL_ADM_MSG_WRITE
};

enum sss_tool_sm_cmd_type {
	SSS_TOOL_SM_CMD_RD16 = 1,
	SSS_TOOL_SM_CMD_RD32,
	SSS_TOOL_SM_CMD_RD64_PAIR,
	SSS_TOOL_SM_CMD_RD64,
	SSS_TOOL_SM_CMD_RD32_CLEAR,
	SSS_TOOL_SM_CMD_RD64_PAIR_CLEAR,
	SSS_TOOL_SM_CMD_RD64_CLEAR
};

enum sss_tool_channel_type {
	SSS_TOOL_CHANNEL_MBOX = 1,
	SSS_TOOL_CHANNEL_ADM_MSG_BYPASS,
	SSS_TOOL_CHANNEL_ADM_MSG_TO_MPU,
	SSS_TOOL_CHANNEL_CLP,
};

struct sss_tool_api_cmd_rd {
	u32 pf_id;
	u8 dest;
	u8 *cmd;
	u16 size;
	void *ack;
	u16 ack_size;
};

struct sss_tool_api_cmd_wr {
	u32 pf_id;
	u8 dest;
	u8 *cmd;
	u16 size;
};

struct sss_tool_pf_dev_info {
	u64 bar0_size;
	u8 bus;
	u8 slot;
	u8 func;
	u64 phy_addr;
};

struct sss_tool_ffm_intr_info {
	u8 node_id;
	/* error level of the interrupt source */
	u8 err_level;
	/* Classification by interrupt source properties */
	u16 err_type;
	u32 err_csr_addr;
	u32 err_csr_value;
};

struct sss_tool_ffm_intr_tm_info {
	struct sss_tool_ffm_intr_info intr_info;
	u8 times;
	u8 sec;
	u8 min;
	u8 hour;
	u8 mday;
	u8 mon;
	u16 year;
};

struct sss_tool_ffm_record_info {
	u32 ffm_num;
	u32 last_err_csr_addr;
	u32 last_err_csr_value;
	struct sss_tool_ffm_intr_tm_info ffm[SSS_TOOL_FFM_RECORD_MAX];
};

struct sss_tool_knl_dbg_info {
	struct semaphore dbgtool_sem;
	struct sss_tool_ffm_record_info *ffm;
};

struct sss_tool_msg_to_up {
	u8 pf_id;
	u8 mod;
	u8 cmd;
	void *buf_in;
	u16 in_size;
	void *buf_out;
	u16 *out_size;
};

struct sss_tool_dbg_param {
	union {
		struct sss_tool_api_cmd_rd api_rd;
		struct sss_tool_api_cmd_wr api_wr;
		struct sss_tool_pf_dev_info *dev_info;
		struct sss_tool_ffm_record_info *ffm_rd;
		struct sss_tool_msg_to_up msg2up;
	} param;
	char chip_name[16];
};

struct sss_tool_pf {
	char name[IFNAMSIZ];
	char bus_info[SSS_TOOL_BUSINFO_LEN];
	u32 pf_type;
};

struct sss_tool_card_info {
	struct sss_tool_pf pf[SSS_TOOL_PF_INFO_MAX];
	u32 pf_num;
};

struct sss_tool_pf_info {
	u32 valid;
	u32 pf_id;
};

struct sss_tool_cmd_chip_fault_stats {
	u32 offset;
	u8 chip_fault_stats[SSS_TOOL_DRV_BUF_SIZE_MAX];
};

struct sss_tool_npu_msg {
	u32 mod : 8;
	u32 cmd : 8;
	u32 ack_type : 3;
	u32 direct_resp : 1;
	u32 len : 12;
};

struct sss_tool_mpu_msg {
	u32 channel : 8;
	u32 mod : 8;
	u32 cmd : 16;
};

struct sss_tool_msg {
	char device_name[IFNAMSIZ];
	u32 module;
	union {
		u32 msg_formate; /* for driver */
		struct sss_tool_npu_msg npu_cmd;
		struct sss_tool_mpu_msg mpu_cmd;
	};
	u32 timeout; /* for mpu/npu cmd */
	u32 func_id;
	u32 buf_in_size;
	u32 buf_out_size;
	void *in_buf;
	void *out_buf;
	int bus_num;
	u8 port_id;
	u8 rsvd1[3];
	u32 rsvd2[4];
};

#endif /* SSS_TOOL_HW_H */
