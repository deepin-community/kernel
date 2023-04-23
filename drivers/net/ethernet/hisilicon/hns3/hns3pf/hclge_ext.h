/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright (c) 2016-2017 Hisilicon Limited. */

#ifndef __HCLGE_EXT_H
#define __HCLGE_EXT_H
#include <linux/types.h>

#define HCLGE_PHY_ID_FOR_RTL8211 0x001cc910
#define HCLGE_PHY_ID_FOR_MVL1512 0x01410dd0
#define HCLGE_PHY_ID_FOR_YT8521  0x00000110
#define HCLGE_PHY_ID_MASK	0xFFFFFFF0U

enum hclge_phy_page_region {
	HCLGE_PHY_REGION_UTP_MII,
	HCLGE_PHY_REGION_UTP_MMD,
	HCLGE_PHY_REGION_UTP_LDS,
	HCLGE_PHY_REGION_UTP_EXT,
	HCLGE_PHY_REGION_SDS_MII,
	HCLGE_PHY_REGION_SDS_EXT,
	HCLGE_PHY_REGION_COM_REG,
	HCLGE_PHY_REGION_MAX
};

enum hclge_phy_op_code {
	PHY_OP_READ,
	PHY_OP_WRITE
};

#define HCLGE_8211_PHY_INDIRECT_PAGE	0xa43
#define HCLGE_8211_PHY_INDIRECT_REG	0x1b
#define HCLGE_8211_PHY_INDIRECT_DATA	0x1c
#define HCLGE_8211_PHY_INDIRECT_RANGE1_S	0xDC0
#define HCLGE_8211_PHY_INDIRECT_RANGE1_E	0xDCF
#define HCLGE_8211_PHY_INDIRECT_RANGE2_S	0xDE0
#define HCLGE_8211_PHY_INDIRECT_RANGE2_E	0xDF0

#define HCLGE_8521_PHY_SMI_SDS_ADDR 0xA000
#define HCLGE_8521_PHY_LDS_MII_ADDR 0x100

#define HCLGE_NOTIFY_PARA_CFG_PKT_EN		BIT(0)
#define HCLGE_NOTIFY_PARA_CFG_START_EN		BIT(1)
#define HCLGE_NOTIFY_PARA_CFG_PKT_NUM_M		GENMASK(5, 2)
#define HCLGE_NOTIFY_PARA_CFG_PKT_NUM_S		2

#define HCLGE_TORUS_MAC_ID_MASK 0x3
#define HCLGE_TOURS_TCX_MAP_TCY_INIT 0x1c6144
#define HCLGE_TOURS_TCX_MAP_TCY_NODE0_INIT 0x1c6141

#define HCLGE_VLAN_FE_NIC_INGRESS 0
#define HCLGE_VLAN_FE_ROCEE_INGRESS 2

#define HCLGE_TORUS_LPBK_DROP_EN 20
#define HCLGE_TC2VLANPRI_MAPPING_EN 19
#define HCLGE_LLDP_LAN_PAIR_EN 18
#define HCLGE_MC_BC_LAN_PAIR_EN 17
#define HCLGE_UC_LAN_PAIR_EN 16

#define HCLGE_TORUS_TC1_DROP_EN BIT(26)

#define HCLGE_TOURS_TCX_MAP_TCY_MASK 0x1c71c7

struct hclge_id_info_cmd {
	__le32 chip_id;
	__le32 mac_id;
	__le32 io_die_id;
	u8 rsv[12];
};

struct hclge_num_info_cmd {
	__le32 chip_num;
	__le32 io_die_num;
	u8 rsv[16];
};

struct hclge_port_num_info_cmd {
	__le32 port_num;
	u8 rsv[20];
};

struct hclge_pfc_storm_para_cmd {
	__le32 dir;
	__le32 enable;
	__le32 period_ms;
	__le32 times;
	__le32 recovery_period_ms;
	__le32 rsv;
};

struct hclge_notify_pkt_param_cmd {
	__le32 cfg;
	__le32 ipg;
	__le32 data[16];
	u8 vld_cfg;
	u8 vld_ipg;
	u8 vld_data;
	u8 rsv[21];
};

struct hclge_torus_cfg_cmd {
	u8 rsv[4];
	__le32 lan_port_pair;
	__le32 lan_fwd_tc_cfg;
	__le32 pause_time_out;
	__le32 pause_time_out_en;
	__le32 torus_en;
};

struct hclge_sfp_present_cmd {
	__le32 sfp_present;
	__le32 rsv[5];
};

struct hclge_sfp_enable_cmd {
	__le32 sfp_enable;
	__le32 rsv[5];
};

struct hclge_lamp_signal_cmd {
	__le32 type;
	__le32 status;
	u8 error;
	u8 locate;
	u8 activity;
	u8 rsv[13];
};

struct hclge_reset_fail_type_map {
	enum hnae3_reset_type reset_type;
	enum hnae3_event_type_custom custom_type;
};

typedef int (*hclge_priv_ops_fn)(struct hclge_dev *hdev, void *data,
				 size_t length);

/**
 * nic_event_fn_t - nic event handler prototype
 * @netdev:	net device
 * @hnae3_event_type_custom:	nic device event type
 */
typedef void (*nic_event_fn_t) (struct net_device *netdev,
				enum hnae3_event_type_custom);

/**
 * nic_register_event - register for nic event handling
 * @event_call:	nic event handler
 * return 0 - success , negative - fail
 */
int nic_register_event(nic_event_fn_t event_call);

/**
 * nic_unregister_event - unregister for nic event handling
 * return 0 - success , negative - fail
 */
int nic_unregister_event(void);

int hclge_ext_call_event(struct hclge_dev *hdev,
			 enum hnae3_event_type_custom event_t);
void hclge_ext_reset_end(struct hclge_dev *hdev, bool done);

int hclge_ext_ops_handle(struct hnae3_handle *handle, int opcode,
			 void *data, size_t length);
#endif
