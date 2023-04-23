/* SPDX-License-Identifier: GPL-2.0+ */
// Copyright (c) 2023 Hisilicon Limited.

#ifndef __HNAE3_EXT_H
#define __HNAE3_EXT_H

enum hnae3_event_type_custom {
	HNAE3_VF_RESET_CUSTOM,
	HNAE3_VF_FUNC_RESET_CUSTOM,
	HNAE3_VF_PF_FUNC_RESET_CUSTOM,
	HNAE3_VF_FULL_RESET_CUSTOM,
	HNAE3_FLR_RESET_CUSTOM,
	HNAE3_FUNC_RESET_CUSTOM,
	HNAE3_GLOBAL_RESET_CUSTOM,
	HNAE3_IMP_RESET_CUSTOM,
	HNAE3_UNKNOWN_RESET_CUSTOM,
	HNAE3_NONE_RESET_CUSTOM,
	HNAE3_PORT_FAULT,
	HNAE3_RESET_DONE_CUSTOM,
	HNAE3_FUNC_RESET_FAIL_CUSTOM,
	HNAE3_GLOBAL_RESET_FAIL_CUSTOM,
	HNAE3_IMP_RESET_FAIL_CUSTOM,
	HNAE3_PPU_POISON_CUSTOM,
	HNAE3_IMP_RD_POISON_CUSTOM,
	HNAE3_ROCEE_AXI_RESP_CUSTOM,
	HNAE3_INVALID_EVENT_CUSTOM,
};

enum hnae3_ext_opcode {
	HNAE3_EXT_OPC_RESET,
	HNAE3_EXT_OPC_EVENT_CALLBACK,
	HNAE3_EXT_OPC_GET_PFC_STORM_PARA,
	HNAE3_EXT_OPC_SET_PFC_STORM_PARA,
	HNAE3_EXT_OPC_SET_NOTIFY_PARAM,
	HNAE3_EXT_OPC_SET_NOTIFY_START,
	HNAE3_EXT_OPC_SET_TORUS_PARAM,
	HNAE3_EXT_OPC_GET_TORUS_PARAM,
	HNAE3_EXT_OPC_CLEAN_STATS64,
	HNAE3_EXT_OPC_GET_PORT_EXT_ID_INFO,
	HNAE3_EXT_OPC_GET_PORT_EXT_NUM_INFO,
	HNAE3_EXT_OPC_GET_PORT_NUM,
	HNAE3_EXT_OPC_GET_PRESENT,
	HNAE3_EXT_OPC_SET_SFP_STATE,
	HNAE3_EXT_OPC_DISABLE_LANE,
	HNAE3_EXT_OPC_GET_LANE_STATUS,
	HNAE3_EXT_OPC_DISABLE_CLOCK,
	HNAE3_EXT_OPC_SET_PFC_TIME,
	HNAE3_EXT_OPC_GET_HILINK_REF_LOS,
	HNAE3_EXT_OPC_GET_PORT_FAULT_STATUS,
	HNAE3_EXT_OPC_GET_PORT_TYPE,
	HNAE3_EXT_OPC_SET_MAC_STATE,
	HNAE3_EXT_OPC_SET_LED,
	HNAE3_EXT_OPC_GET_LED_SIGNAL,
	HNAE3_EXT_OPC_GET_PHY_REG,
	HNAE3_EXT_OPC_SET_PHY_REG,
};

struct hnae3_led_state_para {
	u32 type;
	u32 status;
};

struct hnae3_phy_para {
	u32 page_select_addr;
	u32 reg_addr;
	u16 page;
	u16 data;
};

struct hnae3_lamp_signal {
	u8 error;
	u8 locate;
	u8 activity;
};

struct hnae3_pfc_storm_para {
	u32 dir;
	u32 enable;
	u32 period_ms;
	u32 times;
	u32 recovery_period_ms;
};

enum hnae3_port_fault_type {
	HNAE3_FAULT_TYPE_CDR_FLASH,
	HNAE3_FAULT_TYPE_9545_ERR,
	HNAE3_FAULT_TYPE_CDR_CORE,
	HNAE3_FAULT_TYPE_HILINK_REF_LOS,
	HNAE3_FAULT_TYPE_INVALID
};

struct hnae3_port_fault {
	u32 fault_type;
	u32 fault_status;
};

struct hnae3_notify_pkt_param {
	u32 ipg;     /* inter-packet gap of sending, the unit is one cycle of clock */
	u16 num;     /* packet number of sending */
	u8 enable;   /* send enable, 0=Disable, 1=Enable */
	u8 init;     /* initialization flag, product does not need to set value */
	u8 data[64]; /* note packet data */
};

struct hnae3_torus_param {
	u32 enable;       /* 1d torus mode enable */
	u32 mac_id;       /* export mac id of port */
	u8 is_node0;      /* if current node is node0 */
};

struct hane3_port_ext_id_info {
	u32 chip_id;
	u32 mac_id;
	u32 io_die_id;
};

struct hane3_port_ext_num_info {
	u32 chip_num;
	u32 io_die_num;
};
#endif
