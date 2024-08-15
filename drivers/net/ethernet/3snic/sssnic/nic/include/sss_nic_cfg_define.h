/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_CFG_DEFINE_H
#define SSS_NIC_CFG_DEFINE_H

#include "sss_hw_mbx_msg.h"
#include "sss_nic_cfg_mag_define.h"
#include "sss_nic_cfg_vf_define.h"
#include "sss_nic_cfg_rss_define.h"
#include "sss_nic_dcb_define.h"
#include "sss_nic_tcam_define.h"

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

#define SSSNIC_MBX_OPCODE_SET	1
#define SSSNIC_MBX_OPCODE_GET	0

#define SSSNIC_MBX_OPCODE_ADD	1
#define SSSNIC_MBX_OPCODE_DEL	0

#ifndef BIT
#define BIT(n) (1UL << (n))
#endif

#define SSSNIC_MIN_MTU_SIZE				256

#define SSSNIC_MAX_JUMBO_FRAME_SIZE		9600

#define SSSNIC_PF_SET_VF_ALREADY		0x4

#define SSSNIC_LOWEST_LATENCY			1

#define SSSNIC_MAX_FEATURE_QWORD		4

#define SSSNIC_MBX_OPCODE_GET_DCB_STATE		0
#define SSSNIC_MBX_OPCODE_SET_DCB_STATE		1
#define SSSNIC_DCB_STATE_DISABLE		0
#define SSSNIC_DCB_STATE_ENABLE			1

#define SSSNIC_STD_SFP_INFO_MAX_SIZE	640

#define SSSNIC_BIOS_SIGNATURE			0x1923E518
#define SSSNIC_BIOS_FUN_VALID			1
#define SSSNIC_BIOS_FUN_INVALID			0

enum sss_nic_func_tbl_cfg_type {
	SSSNIC_FUNC_CFG_TYPE_INIT,
	SSSNIC_FUNC_CFG_TYPE_RX_BUF_SIZE,
	SSSNIC_FUNC_CFG_TYPE_MTU,
};

enum sss_nic_feature_cap {
	SSSNIC_F_CSUM = BIT(0),
	SSSNIC_F_SCTP_CRC = BIT(1),
	SSSNIC_F_TSO = BIT(2),
	SSSNIC_F_LRO = BIT(3),
	SSSNIC_F_UFO = BIT(4),
	SSSNIC_F_RSS = BIT(5),
	SSSNIC_F_RX_VLAN_FILTER = BIT(6),
	SSSNIC_F_RX_VLAN_STRIP = BIT(7),
	SSSNIC_F_TX_VLAN_INSERT = BIT(8),
	SSSNIC_F_VXLAN_OFFLOAD = BIT(9),
	SSSNIC_F_IPSEC_OFFLOAD = BIT(10),
	SSSNIC_F_FDIR = BIT(11),
	SSSNIC_F_PROMISC = BIT(12),
	SSSNIC_F_ALLMULTI = BIT(13),
	SSSNIC_F_XSFP_REPORT = BIT(14),
	SSSNIC_F_VF_MAC = BIT(15),
	SSSNIC_F_RATE_LIMIT = BIT(16),
	SSSNIC_F_RXQ_RECOVERY = BIT(17),
};

/* BIOS CONF */
enum {
	SSSNIC_NVM_PF_SPEED_LIMIT = BIT(6),
};

/* Commands between NIC to MPU */
enum sss_nic_mbx_opcode {
	SSSNIC_MBX_OPCODE_VF_REGISTER = 0, /* only for PFD and VFD */

	/* FUNC CFG */
	SSSNIC_MBX_OPCODE_SET_FUNC_TBL = 5,
	SSSNIC_MBX_OPCODE_SET_VPORT_ENABLE,
	SSSNIC_MBX_OPCODE_SET_RX_MODE,
	SSSNIC_MBX_OPCODE_SQ_CI_ATTR_SET,
	SSSNIC_MBX_OPCODE_GET_VPORT_STAT,
	SSSNIC_MBX_OPCODE_CLEAN_VPORT_STAT,
	SSSNIC_MBX_OPCODE_CLEAR_QP_RESOURCE,
	SSSNIC_MBX_OPCODE_CFG_FLEX_QUEUE,
	/* LRO CFG */
	SSSNIC_MBX_OPCODE_CFG_RX_LRO,
	SSSNIC_MBX_OPCODE_CFG_LRO_TIMER,
	SSSNIC_MBX_OPCODE_FEATURE_NEGO,
	SSSNIC_MBX_OPCODE_CFG_LOCAL_LRO_STATE,

	SSSNIC_MBX_OPCODE_CACHE_OUT_QP_RES,
	/* MAC & VLAN CFG */
	SSSNIC_MBX_OPCODE_GET_MAC = 20,
	SSSNIC_MBX_OPCODE_SET_MAC,
	SSSNIC_MBX_OPCODE_DEL_MAC,
	SSSNIC_MBX_OPCODE_UPDATE_MAC,
	SSSNIC_MBX_OPCODE_GET_ALL_DEFAULT_MAC,

	SSSNIC_MBX_OPCODE_CFG_FUNC_VLAN,
	SSSNIC_MBX_OPCODE_SET_VLAN_FILTER_EN,
	SSSNIC_MBX_OPCODE_SET_RX_VLAN_OFFLOAD,
	SSSNIC_MBX_OPCODE_SMAC_CHECK_STATE,

	/* SR-IOV */
	SSSNIC_MBX_OPCODE_CFG_VF_VLAN = 40,
	SSSNIC_MBX_OPCODE_SET_SPOOPCHK_STATE,
	/* RATE LIMIT */
	SSSNIC_MBX_OPCODE_SET_MAX_MIN_RATE,

	/* RSS CFG */
	SSSNIC_MBX_OPCODE_RSS_CFG = 60,
	SSSNIC_MBX_OPCODE_RSS_TEMP_MGR,
	SSSNIC_MBX_OPCODE_GET_RSS_CTX_TBL,
	SSSNIC_MBX_OPCODE_CFG_RSS_HASH_KEY,
	SSSNIC_MBX_OPCODE_CFG_RSS_HASH_ENGINE,
	SSSNIC_MBX_OPCODE_SET_RSS_CTX_TBL_INTO_FUNC,

	/* IP checksum error packets, enable rss quadruple hash */
	SSSNIC_MBX_OPCODE_IPCS_ERR_RSS_ENABLE_OP = 66,

	/* PPA/FDIR */
	SSSNIC_MBX_OPCODE_ADD_TC_FLOW = 80,
	SSSNIC_MBX_OPCODE_DEL_TC_FLOW,
	SSSNIC_MBX_OPCODE_GET_TC_FLOW,
	SSSNIC_MBX_OPCODE_FLUSH_TCAM,
	SSSNIC_MBX_OPCODE_CFG_TCAM_BLOCK,
	SSSNIC_MBX_OPCODE_ENABLE_TCAM,
	SSSNIC_MBX_OPCODE_GET_TCAM_BLOCK,
	SSSNIC_MBX_OPCODE_CFG_PPA_TABLE_ID,
	SSSNIC_MBX_OPCODE_SET_PPA_EN = 88,
	SSSNIC_MBX_OPCODE_CFG_PPA_MODE,
	SSSNIC_MBX_OPCODE_CFG_PPA_FLUSH,
	SSSNIC_MBX_OPCODE_SET_FDIR_STATUS,
	SSSNIC_MBX_OPCODE_GET_PPA_COUNTER,

	/* PORT CFG */
	SSSNIC_MBX_OPCODE_SET_PORT_ENABLE = 100,
	SSSNIC_MBX_OPCODE_CFG_PAUSE_INFO,

	SSSNIC_MBX_OPCODE_SET_PORT_CAR,
	SSSNIC_MBX_OPCODE_SET_ER_DROP_PKT,

	SSSNIC_MBX_OPCODE_GET_VF_COS,
	SSSNIC_MBX_OPCODE_SETUP_COS_MAPPING,
	SSSNIC_MBX_OPCODE_SET_ETS,
	SSSNIC_MBX_OPCODE_SET_PFC,
	SSSNIC_MBX_OPCODE_QOS_ETS,
	SSSNIC_MBX_OPCODE_QOS_PFC,
	SSSNIC_MBX_OPCODE_QOS_DCB_STATE,
	SSSNIC_MBX_OPCODE_QOS_PORT_CFG,
	SSSNIC_MBX_OPCODE_QOS_MAP_CFG,
	SSSNIC_MBX_OPCODE_FORCE_PKT_DROP,
	SSSNIC_MBX_OPCODE_TX_PAUSE_EXCP_NOTICE = 118,
	SSSNIC_MBX_OPCODE_INQUIRT_PAUSE_CFG = 119,

	/* MISC */
	SSSNIC_MBX_OPCODE_BIOS_CFG = 120,
	SSSNIC_MBX_OPCODE_SET_FIRMWARE_CUSTOM_PACKETS_MSG,

	/* BOND */
	SSSNIC_MBX_OPCODE_BOND_DEV_CREATE = 134,
	SSSNIC_MBX_OPCODE_BOND_DEV_DELETE,
	SSSNIC_MBX_OPCODE_BOND_DEV_OPEN_CLOSE,
	SSSNIC_MBX_OPCODE_BOND_INFO_GET,
	SSSNIC_MBX_OPCODE_BOND_ACTIVE_INFO_GET,
	SSSNIC_MBX_OPCODE_BOND_ACTIVE_NOTICE,

	/* DFX */
	SSSNIC_MBX_OPCODE_GET_SM_TABLE = 140,
	SSSNIC_MBX_OPCODE_RD_LINE_TBL,

	SSSNIC_MBX_OPCODE_SET_UCAPTURE_OPT = 160,
	SSSNIC_MBX_OPCODE_SET_VHD_CFG,

	/* move to SSSLINK */
	SSSNIC_MBX_OPCODE_GET_PORT_STAT = 200,
	SSSNIC_MBX_OPCODE_CLEAN_PORT_STAT,
	SSSNIC_MBX_OPCODE_CFG_LOOPBACK_MODE,
	SSSNIC_MBX_OPCODE_GET_SFP_QSFP_INFO,
	SSSNIC_MBX_OPCODE_SET_SFP_STATUS,
	SSSNIC_MBX_OPCODE_GET_LIGHT_MODULE_ABS,
	SSSNIC_MBX_OPCODE_GET_LINK_INFO,
	SSSNIC_MBX_OPCODE_CFG_AN_TYPE,
	SSSNIC_MBX_OPCODE_GET_PORT_INFO,
	SSSNIC_MBX_OPCODE_SET_LINK_SETTINGS,
	SSSNIC_MBX_OPCODE_ACTIVATE_BIOS_LINK_CFG,
	SSSNIC_MBX_OPCODE_RESTORE_LINK_CFG,
	SSSNIC_MBX_OPCODE_SET_LINK_FOLLOW,
	SSSNIC_MBX_OPCODE_GET_LINK_STATE,
	SSSNIC_MBX_OPCODE_LINK_STATUS_REPORT,
	SSSNIC_MBX_OPCODE_CABLE_PLUG_EVENT,
	SSSNIC_MBX_OPCODE_LINK_ERR_EVENT,
	SSSNIC_MBX_OPCODE_SET_LED_STATUS,

	SSSNIC_MBX_OPCODE_MAX = 256,
};

/* NIC CTRLQ MODE */
enum sss_nic_ctrlq_opcode {
	SSSNIC_CTRLQ_OPCODE_MODIFY_QUEUE_CTX = 0,
	SSSNIC_CTRLQ_OPCODE_CLEAN_QUEUE_CONTEXT,
	SSSNIC_CTRLQ_OPCODE_ARM_SQ,
	SSSNIC_CTRLQ_OPCODE_ARM_RQ,
	SSSNIC_CTRLQ_OPCODE_SET_RSS_INDIR_TABLE,
	SSSNIC_CTRLQ_OPCODE_SET_RSS_CONTEXT_TABLE,
	SSSNIC_CTRLQ_OPCODE_GET_RSS_INDIR_TABLE,
	SSSNIC_CTRLQ_OPCODE_GET_RSS_CONTEXT_TABLE,
	SSSNIC_CTRLQ_OPCODE_SET_IQ_ENABLE,
	SSSNIC_CTRLQ_OPCODE_SET_RQ_FLUSH = 10,
	SSSNIC_CTRLQ_OPCODE_MODIFY_VLAN_CTX,
	SSSNIC_CTRLQ_OPCODE_PPA_HASH_TABLE,
	SSSNIC_CTRLQ_OPCODE_RXQ_INFO_GET = 13,
};

struct sss_nic_rq_pc_info {
	u16	hw_pi;
	u16	hw_ci;
};

struct sss_nic_rq_hw_info {
	u32	func_id;
	u32	num_queues;
	u32	rsvd[14];
};

struct sss_nic_mbx_feature_nego {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 opcode; /* 1: set, 0: get */
	u8 rsvd;
	u64 feature[SSSNIC_MAX_FEATURE_QWORD];
};

struct sss_nic_mbx_mac_addr {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u16 vlan_id;
	u16 rsvd1;
	u8 mac[ETH_ALEN];
};

struct sss_nic_mbx_mac_update {
	struct sss_nic_mbx_mac_addr old_mac;
	u16 rsvd2;
	u8 new_mac[ETH_ALEN];
};

struct sss_nic_mbx_vport_state {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u16 rsvd1;
	u8 state; /* 0--disable, 1--enable */
	u8 rsvd2[3];
};

struct sss_nic_mbx_clear_qp_resource {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u16 rsvd1;
};

struct sss_nic_mbx_invalid_qp_cache {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u16 rsvd1;
};

struct sss_nic_mbx_port_stats_info {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u16 rsvd1;
};

struct sss_nic_port_stats {
	u64 tx_unicast_pkts;
	u64 tx_unicast_bytes;
	u64 tx_multicast_pkts;
	u64 tx_multicast_bytes;
	u64 tx_broadcast_pkts;
	u64 tx_broadcast_bytes;

	u64 rx_unicast_pkts;
	u64 rx_unicast_bytes;
	u64 rx_multicast_pkts;
	u64 rx_multicast_bytes;
	u64 rx_broadcast_pkts;
	u64 rx_broadcast_bytes;

	u64 tx_discard;
	u64 rx_discard;
	u64 tx_err;
	u64 rx_err;
};

struct sss_nic_mbx_port_stats {
	struct sss_mgmt_msg_head head;

	u32 stats_size;
	u32 rsvd1;
	struct sss_nic_port_stats stats;
	u64 rsvd2[6];
};

struct sss_nic_func_table_cfg {
	u16 rx_wqe_buf_size;
	u16 mtu;
	u32 rsvd[9];
};

struct sss_nic_mbx_set_func_table {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u16 rsvd;

	u32 cfg_bitmap;
	struct sss_nic_func_table_cfg tbl_cfg;
};

struct sss_nic_mbx_intr_attr {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 dma_attr_off;
	u8 pending_limit;
	u8 coalescing_time;
	u8 intr_en;
	u16 intr_id;
	u32 l2nic_sqn;
	u32 rsvd;
	u64 ci_addr;
};

struct sss_nic_mbx_offload_vlan {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 vlan_offload;
	u8 rsvd1[5];
};

struct sss_nic_mbx_lro_cfg {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 opcode;
	u8 rsvd1;
	u8 lro_ipv4_en;
	u8 lro_ipv6_en;
	u8 lro_max_pkt_len; /* unit is 1K */
	u8 resv2[13];
};

struct sss_nic_mbx_lro_timer {
	struct sss_mgmt_msg_head head;

	u8 opcode; /* 1: set timer value, 0: get timer value */
	u8 rsvd1;
	u16 rsvd2;
	u32 timer;
};

struct sss_nic_mbx_vf_vlan_cfg {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 opcode;
	u8 rsvd1;
	u16 vlan_id;
	u8 qos;
	u8 rsvd2[5];
};

struct sss_nic_mbx_set_spoofchk {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 state;
	u8 rsvd1;
};

struct sss_nic_mbx_tx_rate_cfg {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u16 rsvd1;
	u32 min_rate;
	u32 max_rate;
	u8 rsvd2[8];
};

struct sss_nic_mbx_attach_vf {
	struct sss_mgmt_msg_head head;

	u8 op_register; /* 0 - unregister, 1 - register */
	u8 rsvd1[3];
	u32 extra_feature;
	u8 rsvd2[32];
};

struct sss_nic_mbx_vlan_cfg {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 opcode;
	u8 rsvd1;
	u16 vlan_id;
	u16 rsvd2;
};

/* set vlan filter */
struct sss_nic_mbx_vlan_filter_cfg {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 resvd[2];
	u32 vlan_filter_ctrl; /* bit0:vlan filter en; bit1:broadcast_filter_en */
};

struct sss_nic_mbx_force_drop_pkt {
	struct sss_mgmt_msg_head head;

	u8 port;
	u8 rsvd1[3];
};

struct sss_nic_mbx_set_rx_mode {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u16 rsvd1;
	u32 rx_mode;
};

/* rss */
struct sss_nic_mbx_rss_ctx {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u16 rsvd1;
	u32 context;
};

struct sss_nic_mbx_rss_engine_cfg {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 opcode;
	u8 hash_engine;
	u8 rsvd1[4];
};

struct sss_nic_mbx_rss_key_cfg {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 opcode;
	u8 rsvd1;
	u8 key[SSSNIC_RSS_KEY_SIZE];
};

struct sss_nic_mbx_rss_cfg {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 rss_en;
	u8 rq_priority_number;
	u8 prio_tc[SSSNIC_DCB_COS_MAX];
	u16 qp_num;
	u16 rsvd1;
};

struct sss_nic_mbx_vf_dcb_cfg {
	struct sss_mgmt_msg_head head;

	struct sss_nic_dcb_info dcb_info;
};

struct sss_nic_mbx_dcb_state {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 op_code;      /* 0 - get dcb state, 1 - set dcb state */
	u8 state;        /* 0 - disable, 1 - enable dcb */
	u8 port_state;   /* 0 - disable, 1 - enable dcb */
	u8 rsvd[7];
};

struct sss_nic_mbx_pause_cfg {
	struct sss_mgmt_msg_head head;

	u8 port_id;
	u8 opcode;
	u16 rsvd1;
	u8 auto_neg;
	u8 rx_pause;
	u8 tx_pause;
	u8 rsvd2[5];
};

/* pfc/pause tx abnormal */
struct sss_nic_msg_tx_pause_info {
	struct sss_mgmt_msg_head head;

	u32 tx_pause_except; /* 1: 异常，0: 正常 */
	u32 except_level;
	u32 rsvd;
};

struct sss_nic_mbx_set_tcam_state {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 tcam_enable;
	u8 rsvd1;
	u32 rsvd2;
};

/* alloc tcam block output struct */
struct sss_nic_mbx_tcam_block_cfg {
	struct sss_mgmt_msg_head head;

	u16 func_id;  /* func_id */
	u8 alloc_en;
	u8 tcam_type; /* 0: 16 size tcam block, 1: 0 size tcam block */
	u16 tcam_block_index;
	u16 mpu_alloc_block_size;
};

struct sss_nic_mbx_flush_tcam_rule {
	struct sss_mgmt_msg_head head;

	u16 func_id; /* func_id */
	u16 rsvd;
};

struct sss_nic_mbx_add_tcam_rule {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 type;
	u8 rsvd;
	struct sss_nic_tcam_rule_cfg rule;
};

struct sss_nic_mbx_del_tcam_rule {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u8 type;
	u8 rsvd;
	u32 index_start;
	u32 index_num;
};

/* note:must 4 byte align */
struct sss_nic_bios_cfg {
	u32 signature; /* check flash data valid */
	u8 pxe_en;     /* PXE enable: 0 - disable 1 - enable */
	u8 extend_mode;
	u8 rsvd0[2];
	u8 pxe_vlan_en;     /* PXE VLAN enable: 0 - disable 1 - enable */
	u8 pxe_vlan_pri;    /* PXE VLAN priority: 0-7 */
	u16 pxe_vlan_id;    /* PXE VLAN ID 1-4094 */
	u32 service_mode;   /* refer to CHIPIF_SERVICE_MODE_x macro */
	u32 pf_bw;          /* PF rate，percent 0-100 */
	u8 speed;           /* enum of port speed */
	u8 auto_neg;        /* 0 - invalid 1 - open 2 - close */
	u8 lanes;           /* lane num */
	u8 fec;             /* FEC mode, refer to enum mag_cmd_port_fec */
	u8 auto_adapt;      /* 0 - invalid 1 - open 2 - close */
	u8 func_valid;      /* 0 - func_id is invalid，other - func_id is valid */
	u8 func_id;
	u8 sriov_en;        /* SRIOV-EN: 0 - invalid， 1 - open， 2 - close */
};

struct sss_nic_mbx_bios_cfg {
	struct sss_mgmt_msg_head head;
	u32 op_code; /* Operation Code: Bit0[0: read 1:write, BIT1-6: cfg_mask */
	struct sss_nic_bios_cfg bios_cfg;
};

/* lacp status update */
struct sss_nic_msg_bond_active_info {
	struct sss_mgmt_msg_head head;
	u32 bond_id;
	u32 bon_mmi_status; /* bond link state */
	u32 active_bitmap;  /* slave port state */

	u8 rsvd[16];
};

#endif
