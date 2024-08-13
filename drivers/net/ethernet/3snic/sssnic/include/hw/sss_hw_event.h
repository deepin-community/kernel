/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HW_EVENT_H
#define SSS_HW_EVENT_H

#include <linux/types.h>

#include "sss_hw_svc_cap.h"

enum sss_fault_source_type {
	/* same as SSS_FAULT_TYPE_CHIP */
	SSS_FAULT_SRC_HW_MGMT_CHIP = 0,
	/* same as SSS_FAULT_TYPE_NPU */
	SSS_FAULT_SRC_HW_MGMT_NPU,
	/* same as SSS_FAULT_TYPE_MEM_RD_TIMEOUT */
	SSS_FAULT_SRC_HW_MGMT_MEM_RD_TIMEOUT,
	/* same as SSS_FAULT_TYPE_MEM_WR_TIMEOUT */
	SSS_FAULT_SRC_HW_MGMT_MEM_WR_TIMEOUT,
	/* same as SSS_FAULT_TYPE_REG_RD_TIMEOUT */
	SSS_FAULT_SRC_HW_MGMT_REG_RD_TIMEOUT,
	/* same as SSS_FAULT_TYPE_REG_WR_TIMEOUT */
	SSS_FAULT_SRC_HW_MGMT_REG_WR_TIMEOUT,
	SSS_FAULT_SRC_SW_MGMT_NPU,
	SSS_FAULT_SRC_MGMT_WATCHDOG,
	SSS_FAULT_SRC_MGMT_RESET = 8,
	SSS_FAULT_SRC_HW_PHY_FAULT,
	SSS_FAULT_SRC_TX_PAUSE_EXCP,
	SSS_FAULT_SRC_PCIE_LINK_DOWN = 20,
	SSS_FAULT_SRC_HOST_HEARTBEAT_LOST = 21,
	SSS_FAULT_SRC_TX_TIMEOUT,
	SSS_FAULT_SRC_TYPE_MAX,
};

enum sss_comm_event_type {
	SSS_EVENT_PCIE_LINK_DOWN,
	SSS_EVENT_HEART_LOST,
	SSS_EVENT_FAULT,
	SSS_EVENT_SRIOV_STATE_CHANGE,
	SSS_EVENT_CARD_REMOVE,
	SSS_EVENT_MGMT_WATCHDOG,
	SSS_EVENT_MAX
};

enum sss_event_service_type {
	SSS_EVENT_SRV_COMM,
	SSS_SERVICE_EVENT_BASE,
	SSS_EVENT_SRV_NIC = SSS_SERVICE_EVENT_BASE + SSS_SERVICE_TYPE_NIC,
	SSS_EVENT_SRV_MIGRATE = SSS_SERVICE_EVENT_BASE + SSS_SERVICE_TYPE_MIGRATE,
};

enum sss_fault_err_level {
	SSS_FAULT_LEVEL_FATAL,
	SSS_FAULT_LEVEL_SERIOUS_RESET,
	SSS_FAULT_LEVEL_HOST,
	SSS_FAULT_LEVEL_SERIOUS_FLR,
	SSS_FAULT_LEVEL_GENERAL,
	SSS_FAULT_LEVEL_SUGGESTION,
	SSS_FAULT_LEVEL_MAX,
};

enum sss_fault_type {
	SSS_FAULT_TYPE_CHIP,
	SSS_FAULT_TYPE_NPU,
	SSS_FAULT_TYPE_MEM_RD_TIMEOUT,
	SSS_FAULT_TYPE_MEM_WR_TIMEOUT,
	SSS_FAULT_TYPE_REG_RD_TIMEOUT,
	SSS_FAULT_TYPE_REG_WR_TIMEOUT,
	SSS_FAULT_TYPE_PHY_FAULT,
	SSS_FAULT_TYPE_TSENSOR_FAULT,
	SSS_FAULT_TYPE_MAX,
};

#define SSS_SRV_EVENT_TYPE(svc, type)	((((u32)(svc)) << 16) | (type))

#define SSS_MGMT_CMD_UNSUPPORTED			0xFF

union sss_fault_hw_mgmt {
	u32 val[4];
	/* valid only type == SSS_FAULT_TYPE_CHIP */
	struct {
		u8 node_id;
		/* enum sss_fault_err_level */
		u8 err_level;
		u16 err_type;
		u32 err_csr_addr;
		u32 err_csr_value;
		/* func_id valid only if err_level == SSS_FAULT_LEVEL_SERIOUS_FLR */
		u8 rsvd1;
		u8 host_id;
		u16 func_id;
	} chip;

	/* valid only if type == SSS_FAULT_TYPE_NPU */
	struct {
		u8 cause_id;
		u8 core_id;
		u8 c_id;
		u8 rsvd3;
		u32 epc;
		u32 rsvd4;
		u32 rsvd5;
	} ucode;

	/* valid only if type == SSS_FAULT_TYPE_MEM_RD_TIMEOUT ||
	 * SSS_FAULT_TYPE_MEM_WR_TIMEOUT
	 */
	struct {
		u32 err_csr_ctrl;
		u32 err_csr_data;
		u32 ctrl_tab;
		u32 mem_id;
	} mem_timeout;

	/* valid only if type == SSS_FAULT_TYPE_REG_RD_TIMEOUT ||
	 * SSS_FAULT_TYPE_REG_WR_TIMEOUT
	 */
	struct {
		u32 err_csr;
		u32 rsvd6;
		u32 rsvd7;
		u32 rsvd8;
	} reg_timeout;

	struct {
		/* 0: read; 1: write */
		u8 op_type;
		u8 port_id;
		u8 dev_ad;
		u8 rsvd9;
		u32 csr_addr;
		u32 op_data;
		u32 rsvd10;
	} phy_fault;
};

/* defined by chip */
struct sss_fault_event {
	u8 type; /* enum sss_fault_type */
	u8 fault_level; /* sdk write fault level for uld event */
	u8 rsvd[2];
	union sss_fault_hw_mgmt info;
};

struct sss_cmd_fault_event {
	u8 status;
	u8 ver;
	u8 rsvd[6];
	struct sss_fault_event fault_event;
};

struct sss_event_info {
	u16 service;	/* enum sss_event_service_type */
	u16 type; /* enum sss_comm_event_type */
	u8 event_data[104];
};

typedef void (*sss_event_handler_t)(void *handle, struct sss_event_info *event);

#endif
