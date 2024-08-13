/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HW_MBX_H
#define SSS_HW_MBX_H

#include <linux/types.h>

/* between Driver to MPU */
enum sss_mgmt_cmd {
	/* flr */
	SSS_COMM_MGMT_CMD_FUNC_RESET = 0,
	SSS_COMM_MGMT_CMD_FEATURE_NEGO,
	SSS_COMM_MGMT_CMD_FLUSH_DOORBELL,
	SSS_COMM_MGMT_CMD_START_FLUSH,
	SSS_COMM_MGMT_CMD_SET_FUNC_FLR,
	SSS_COMM_MGMT_CMD_GET_GLOBAL_ATTR,
	SSS_COMM_MGMT_CMD_SET_PPF_FLR_TYPE,
	SSS_COMM_MGMT_CMD_SET_FUNC_SVC_USED_STATE,

	/* msi-x */
	SSS_COMM_MGMT_CMD_CFG_MSIX_NUM = 10,

	/* init cfg */
	SSS_COMM_MGMT_CMD_SET_CTRLQ_CTXT = 20,
	SSS_COMM_MGMT_CMD_SET_VAT,
	SSS_COMM_MGMT_CMD_CFG_PAGESIZE,
	SSS_COMM_MGMT_CMD_CFG_MSIX_CTRL_REG,
	SSS_COMM_MGMT_CMD_SET_CEQ_CTRL_REG,
	SSS_COMM_MGMT_CMD_SET_DMA_ATTR,

	/* infra */
	SSS_COMM_MGMT_CMD_GET_MQM_FIX_INFO = 40,
	SSS_COMM_MGMT_CMD_SET_MQM_CFG_INFO,
	SSS_COMM_MGMT_CMD_SET_MQM_SRCH_GPA,
	SSS_COMM_MGMT_CMD_SET_PPF_TMR,
	SSS_COMM_MGMT_CMD_SET_PPF_HT_GPA,
	SSS_COMM_MGMT_CMD_SET_FUNC_TMR_BITMAT,
	SSS_COMM_MGMT_CMD_SET_MBX_CRDT,
	SSS_COMM_MGMT_CMD_CFG_TEMPLATE,
	SSS_COMM_MGMT_CMD_SET_MQM_LIMIT,

	/* get chip info */
	SSS_COMM_MGMT_CMD_GET_FW_VERSION = 60,
	SSS_COMM_MGMT_CMD_GET_BOARD_INFO,
	SSS_COMM_MGMT_CMD_SYNC_TIME,
	SSS_COMM_MGMT_CMD_GET_HW_PF_INFOS,
	SSS_COMM_MGMT_CMD_SEND_BDF_INFO,
	SSS_COMM_MGMT_CMD_GET_VIRTIO_BDF_INFO,
	SSS_COMM_MGMT_CMD_GET_SML_TABLE_INFO,

	/* update firmware */
	SSS_COMM_MGMT_CMD_UPDATE_FW = 80,
	SSS_COMM_MGMT_CMD_ACTIVE_FW,
	SSS_COMM_MGMT_CMD_HOT_ACTIVE_FW,
	SSS_COMM_MGMT_CMD_HOT_ACTIVE_DONE_NOTICE,
	SSS_COMM_MGMT_CMD_SWITCH_CFG,
	SSS_COMM_MGMT_CMD_CHECK_FLASH,
	SSS_COMM_MGMT_CMD_CHECK_FLASH_RW,
	SSS_COMM_MGMT_CMD_RESOURCE_CFG,
	SSS_COMM_MGMT_CMD_UPDATE_BIOS, /* merge to SSS_COMM_MGMT_CMD_UPDATE_FW */
	SSS_COMM_MGMT_CMD_MPU_GIT_CODE,

	/* chip reset */
	SSS_COMM_MGMT_CMD_FAULT_REPORT = 100,
	SSS_COMM_MGMT_CMD_WATCHDOG_INFO,
	SSS_COMM_MGMT_CMD_MGMT_RESET,
	SSS_COMM_MGMT_CMD_FFM_SET,

	/* chip info/log */
	SSS_COMM_MGMT_CMD_GET_LOG = 120,
	SSS_COMM_MGMT_CMD_TEMP_OP,
	SSS_COMM_MGMT_CMD_EN_AUTO_RST_CHIP,
	SSS_COMM_MGMT_CMD_CFG_REG,
	SSS_COMM_MGMT_CMD_GET_CHIP_ID,
	SSS_COMM_MGMT_CMD_SYSINFO_DFX,
	SSS_COMM_MGMT_CMD_PCIE_DFX_NTC,
	SSS_COMM_MGMT_CMD_DICT_LOG_STATUS, /* LOG STATUS 127 */
	SSS_COMM_MGMT_CMD_MSIX_INFO,
	SSS_COMM_MGMT_CMD_CHANNEL_DETECT,

	/* DFT mode */
	SSS_COMM_MGMT_CMD_GET_DIE_ID = 200,
	SSS_COMM_MGMT_CMD_GET_EFUSE_TEST,
	SSS_COMM_MGMT_CMD_EFUSE_INFO_CFG,
	SSS_COMM_MGMT_CMD_GPIO_CTL,
	SSS_COMM_MGMT_CMD_HI30_SERLOOP_START, /* DFT or ssslink */
	SSS_COMM_MGMT_CMD_HI30_SERLOOP_STOP, /* DFT or ssslink */
	SSS_COMM_MGMT_CMD_HI30_MBIST_SET_FLAG, /* DFT or ssslink */
	SSS_COMM_MGMT_CMD_HI30_MBIST_GET_RESULT, /* DFT or ssslink */
	SSS_COMM_MGMT_CMD_ECC_TEST,
	SSS_COMM_MGMT_CMD_FUNC_BIST_TEST,
	SSS_COMM_MGMT_CMD_VPD_SET,
	SSS_COMM_MGMT_CMD_VPD_GET,

	SSS_COMM_MGMT_CMD_ERASE_FLASH,
	SSS_COMM_MGMT_CMD_QUERY_FW_INFO,
	SSS_COMM_MGMT_CMD_GET_CFG_INFO,
	SSS_COMM_MGMT_CMD_GET_UART_LOG,
	SSS_COMM_MGMT_CMD_SET_UART_CMD,
	SSS_COMM_MGMT_CMD_SPI_TEST,

	/* ALL reg read/write merge to SSS_COMM_MGMT_CMD_CFG_REG */
	SSS_COMM_MGMT_CMD_UP_REG_GET,
	SSS_COMM_MGMT_CMD_UP_REG_SET,
	SSS_COMM_MGMT_CMD_REG_READ,
	SSS_COMM_MGMT_CMD_REG_WRITE,
	SSS_COMM_MGMT_CMD_MAG_REG_WRITE,
	SSS_COMM_MGMT_CMD_ANLT_REG_WRITE,

	SSS_COMM_MGMT_CMD_HEART_EVENT,
	SSS_COMM_MGMT_CMD_NCSI_OEM_GET_DRV_INFO,
	SSS_COMM_MGMT_CMD_LASTWORD_GET, /* merge to SSS_COMM_MGMT_CMD_GET_LOG */
	SSS_COMM_MGMT_CMD_READ_BIN_DATA,
	SSS_COMM_MGMT_CMD_WWPN_GET,
	SSS_COMM_MGMT_CMD_WWPN_SET,

	SSS_COMM_MGMT_CMD_SEND_API_ACK_BY_UP,

	SSS_COMM_MGMT_CMD_SET_MAC,

	/* MPU patch cmd */
	SSS_COMM_MGMT_CMD_LOAD_PATCH,
	SSS_COMM_MGMT_CMD_REMOVE_PATCH,
	SSS_COMM_MGMT_CMD_PATCH_ACTIVE,
	SSS_COMM_MGMT_CMD_PATCH_DEACTIVE,
	SSS_COMM_MGMT_CMD_PATCH_SRAM_OPTIMIZE,
	/* container host process */
	SSS_COMM_MGMT_CMD_CONTAINER_HOST_PROC,
	/* nsci counter */
	SSS_COMM_MGMT_CMD_NCSI_COUNTER_PROC,
};

enum sss_channel_type {
	SSS_CHANNEL_DEFAULT,
	SSS_CHANNEL_COMM,
	SSS_CHANNEL_NIC,
	SSS_CHANNEL_ROCE,
	SSS_CHANNEL_TOE,
	SSS_CHANNEL_FC,
	SSS_CHANNEL_OVS,
	SSS_CHANNEL_DSW,
	SSS_CHANNEL_MIG,
	SSS_CHANNEL_CRYPT,
	SSS_CHANNEL_MAX = 32,
};

enum sss_mbx_errcode {
	SSS_MBX_ERRCODE_NO_ERRORS		= 0,
	/* VF send the mbx data to the wrong destination functions */
	SSS_MBX_ERRCODE_VF_TO_WRONG_FUNC	= 0x100,
	/* PPF send the mbx data to the wrong destination functions */
	SSS_MBX_ERRCODE_PPF_TO_WRONG_FUNC	= 0x200,
	/* PF send the mbx data to the wrong destination functions */
	SSS_MBX_ERRCODE_PF_TO_WRONG_FUNC	= 0x300,
	/* The mbx data size is set to all zero */
	SSS_MBX_ERRCODE_ZERO_DATA_SIZE	= 0x400,
	/* The sender function attribute has not been learned by hardware */
	SSS_MBX_ERRCODE_UNKNOWN_SRC_FUNC	= 0x500,
	/* The receiver function attr has not been learned by hardware */
	SSS_MBX_ERRCODE_UNKNOWN_DES_FUNC	= 0x600,
};

/* CTRLQ MODULE_TYPE */
enum sss_mod_type {
	SSS_MOD_TYPE_COMM = 0,  /* HW communication module */
	SSS_MOD_TYPE_L2NIC = 1, /* L2NIC module */
	SSS_MOD_TYPE_ROCE = 2,
	SSS_MOD_TYPE_PLOG = 3,
	SSS_MOD_TYPE_TOE = 4,
	SSS_MOD_TYPE_FLR = 5,
	SSS_MOD_TYPE_RSVD1 = 6,
	SSS_MOD_TYPE_CFGM = 7, /* Configuration module */
	SSS_MOD_TYPE_QMM = 8,
	SSS_MOD_TYPE_RSVD2 = 9,
	COMM_MOD_FC = 10,
	SSS_MOD_TYPE_OVS = 11,
	SSS_MOD_TYPE_DSW = 12,
	SSS_MOD_TYPE_MIGRATE = 13,
	SSS_MOD_TYPE_SSSLINK = 14,
	SSS_MOD_TYPE_CRYPT = 15, /* secure crypto module */
	SSS_MOD_TYPE_VIO = 16,
	SSS_MOD_TYPE_IMU = 17,
	SSS_MOD_TYPE_DFT = 18, /* DFT */
	SSS_MOD_TYPE_HW_MAX = 19, /* hardware max module id */
	/* Software module id, for PF/VF and multi-host */
	SSS_MOD_TYPE_SW_FUNC = 20,
	SSS_MOD_TYPE_MAX,
};

/* func reset flag */
enum sss_func_reset_flag {
	SSS_RESET_TYPE_FLUSH_BIT = 0,
	SSS_RESET_TYPE_MQM,
	SSS_RESET_TYPE_SMF,
	SSS_RESET_TYPE_PF_BW_CFG,

	SSS_RESET_TYPE_COMM = 10,
	SSS_RESET_TYPE_COMM_MGMT_CH,
	SSS_RESET_TYPE_COMM_CMD_CH,
	SSS_RESET_TYPE_NIC,
	SSS_RESET_TYPE_OVS,
	SSS_RESET_TYPE_VBS,
	SSS_RESET_TYPE_ROCE,
	SSS_RESET_TYPE_FC,
	SSS_RESET_TYPE_TOE,
	SSS_RESET_TYPE_IPSEC,
	SSS_RESET_TYPE_MAX,
};

#define SSS_NIC_RESET		BIT(SSS_RESET_TYPE_NIC)
#define SSS_OVS_RESET		BIT(SSS_RESET_TYPE_OVS)
#define SSS_VBS_RESET		BIT(SSS_RESET_TYPE_VBS)
#define SSS_ROCE_RESET		BIT(SSS_RESET_TYPE_ROCE)
#define SSS_FC_RESET		BIT(SSS_RESET_TYPE_FC)
#define SSS_TOE_RESET		BIT(SSS_RESET_TYPE_TOE)
#define SSS_IPSEC_RESET		BIT(SSS_RESET_TYPE_IPSEC)

typedef int (*sss_vf_mbx_handler_t)(void *pri_handle, u16 cmd, void *buf_in,
				    u16 in_size, void *buf_out, u16 *out_size);

typedef int (*sss_pf_mbx_handler_t)(void *pri_handle, u16 vf_id, u16 cmd,
				    void *buf_in, u16 in_size, void *buf_out,
				    u16 *out_size);

typedef int (*sss_ppf_mbx_handler_t)(void *pri_handle, u16 pf_id, u16 vf_id,
				     u16 cmd, void *buf_in, u16 in_size,
				     void *buf_out, u16 *out_size);

typedef int (*sss_pf_from_ppf_mbx_handler_t)(void *pri_handle,
		u16 cmd, void *buf_in, u16 in_size, void *buf_out, u16 *out_size);

/**
 * @brief sss_register_pf_mbx_handler - pf register mbx msg callback
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param pri_handle: private data will be used by the callback
 * @param callback: callback function
 * @retval zero: success
 * @retval non-zero: failure
 **/
int sss_register_pf_mbx_handler(void *hwdev, u8 mod, void *pri_handle, sss_pf_mbx_handler_t cb);

/**
 * @brief sss_register_vf_mbx_handler - vf register mbx msg callback
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param pri_handle: private data will be used by the callback
 * @param callback: callback function
 * @retval zero: success
 * @retval non-zero: failure
 **/
int sss_register_vf_mbx_handler(void *hwdev, u8 mod, void *pri_handle, sss_vf_mbx_handler_t cb);

/**
 * @brief sss_unregister_pf_mbx_handler - pf register mbx msg callback
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 **/
void sss_unregister_pf_mbx_handler(void *hwdev, u8 mod);

/**
 * @brief sss_unregister_vf_mbx_handler - pf register mbx msg callback
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 **/
void sss_unregister_vf_mbx_handler(void *hwdev, u8 mod);

/**
 * @brief sss_sync_send_mbx_msg - msg to management cpu
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param in_size: in buffer size
 * @param buf_out: message buffer out
 * @param out_size: out buffer size
 * @param timeout: timeout
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int sss_sync_mbx_send_msg(void *hwdev, u8 mod, u16 cmd, void *buf_in,
			  u16 in_size, void *buf_out, u16 *out_size, u32 timeout, u16 channel);

#define sss_sync_send_msg_ch(hwdev, cmd, buf_in, in_size, buf_out, out_size, channel) \
			sss_sync_mbx_send_msg(hwdev, SSS_MOD_TYPE_COMM, cmd, \
			buf_in, in_size, buf_out, out_size, 0, channel)

#define sss_sync_send_msg(hwdev, cmd, buf_in, in_size, buf_out, out_size) \
			sss_sync_mbx_send_msg(hwdev, SSS_MOD_TYPE_COMM, cmd, \
			buf_in, in_size, buf_out, out_size, 0, SSS_CHANNEL_COMM)

#define SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, cmd_ptr) \
	((ret) != 0 || (out_len) == 0 || (cmd_ptr)->head.state != SSS_MGMT_CMD_SUCCESS)

/**
 * @brief sss_mbx_send_to_pf - vf mbx message to pf
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param in_size: in buffer size
 * @param buf_out: message buffer out
 * @param out_size: out buffer size
 * @param timeout: timeout
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int sss_mbx_send_to_pf(void *hwdev, u8 mod, u16 cmd, void *buf_in,
		       u16 in_size, void *buf_out, u16 *out_size, u32 timeout, u16 channel);

/**
 * @brief sss_mbx_send_to_vf - mbx message to vf
 * @param hwdev: device pointer to hwdev
 * @param vf_id: vf index
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param in_size: in buffer size
 * @param buf_out: message buffer out
 * @param out_size: out buffer size
 * @param timeout: timeout
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int sss_mbx_send_to_vf(void *hwdev, u16 vf_id, u8 mod, u16 cmd, void *buf_in,
		       u16 in_size, void *buf_out, u16 *out_size, u32 timeout, u16 channel);

#endif
