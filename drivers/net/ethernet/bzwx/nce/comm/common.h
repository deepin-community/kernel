/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _NE6X_COMMON_H
#define _NE6X_COMMON_H

#define NE6X_MAX_U64                  0xFFFFFFFFFFFFFFFFULL

#define NE6X_MODULE_TYPE_TOTAL_BYTE   3

#define NE6X_AQ_LINK_UP               0x1ULL
#define NE6X_AQ_AN_COMPLETED          BIT(0)

#define PCI_VENDOR_ID_BZWX 0xD20C

struct ne6x_eth_stats {
	u64 rx_bytes;		 /* gorc */
	u64 rx_unicast;		 /* uprc */
	u64 rx_multicast;	 /* mprc */
	u64 rx_broadcast;	 /* bprc */
	u64 rx_discards;	 /* rdpc */
	u64 rx_miss;
	u64 rx_unknown_protocol; /* rupp */
	u64 tx_bytes;		 /* gotc */
	u64 tx_unicast;		 /* uptc */
	u64 tx_multicast;	 /* mptc */
	u64 tx_broadcast;	 /* bptc */
	u64 tx_discards;	 /* tdpc */
	u64 tx_errors;		 /* tepc */
	u64 rx_malform;
	u64 tx_malform;
};

enum ne6x_phy_type {
	NE6X_PHY_TYPE_UNKNOWN = 0,
	NE6X_PHY_TYPE_10GBASE = 1,
	NE6X_PHY_TYPE_25GBASE,
	NE6X_PHY_TYPE_40GBASE,
	NE6X_PHY_TYPE_100GBASE,
	NE6X_PHY_TYPE_200GBASE,
};

#define NE6X_LINK_SPEED_10GB_SHIFT  0x1
#define NE6X_LINK_SPEED_40GB_SHIFT  0x2
#define NE6X_LINK_SPEED_25GB_SHIFT  0x3
#define NE6X_LINK_SPEED_100GB_SHIFT 0x4
#define NE6X_LINK_SPEED_200GB_SHIFT 0x5

enum ne6x_sdk_link_speed {
	NE6X_LINK_SPEED_UNKNOWN = 0,
	NE6X_LINK_SPEED_10GB    = BIT(NE6X_LINK_SPEED_10GB_SHIFT),
	NE6X_LINK_SPEED_40GB    = BIT(NE6X_LINK_SPEED_40GB_SHIFT),
	NE6X_LINK_SPEED_25GB    = BIT(NE6X_LINK_SPEED_25GB_SHIFT),
	NE6X_LINK_SPEED_100GB   = BIT(NE6X_LINK_SPEED_100GB_SHIFT),
	NE6X_LINK_SPEED_200GB   = BIT(NE6X_LINK_SPEED_200GB_SHIFT),
};

struct ne6x_link_status {
	u64 phy_type_low;
	u64 phy_type_high;

	u16 max_frame_size;
	u16 req_speeds;
	u8  topo_media_conflict;
	u8  link_cfg_err;
	u8  lse_ena; /* Link Status Event notification */
	u8  link_info;
	u8  an_info;
	u8  ext_info;
	u8  fec_info;
	u8  pacing;
	u32 link_speed;
	u8  module_type[NE6X_MODULE_TYPE_TOTAL_BYTE];
};

struct ne6x_mac_info {
	u8 perm_addr[ETH_ALEN];
};

struct ne6x_link_info {
	u32 link;
	u32 speed;
};

enum ne6x_media_type {
	NE6X_MEDIA_UNKNOWN = 0,
	NE6X_MEDIA_FIBER,
	NE6X_MEDIA_BASET,
	NE6X_MEDIA_BACKPLANE,
	NE6X_MEDIA_DA,
	NE6X_MEDIA_AUI,
};

struct ne6x_phy_info {
	struct ne6x_link_status link_info;
	struct ne6x_link_status link_info_old;
	u64                     phy_type_low;
	u64                     phy_type_high;
	enum ne6x_media_type    media_type;
	u8                      get_link_info;
	u16                     curr_user_speed_req;
};

struct ne6x_port_info {
	struct ne6x_hw       *hw; /* back pointer to HW instance */

	u8                    lport;
	u8                    hw_port_id; /* hardware port id */
	u8                    hw_trunk_id;
	u32                   hw_queue_base_old;
	u32                   hw_queue_base;
	u32                   hw_max_queue;

	u32                   queue; /* current used queue */
	struct ne6x_link_info link_status;
	struct ne6x_mac_info  mac;
	struct ne6x_phy_info  phy;
};

struct ne6x_bus_info {
	u16 domain_num;
	u16 device;
	u8  func;
	u8  bus_num;
};

struct ne6x_mbx_snap_buffer_data {
	u8 state : 4;
	u8 len   : 4;
	u8 type;
	u8 data[6];
};

/* Structure to track messages sent by VFs on mailbox:
 * 1. vf_cntr : a counter array of VFs to track the number of
 * asynchronous messages sent by each VF
 * 2. vfcntr_len : number of entries in VF counter array
 */
struct ne6x_mbx_vf_counter {
	u32 *vf_cntr;
	u32  vfcntr_len;
};

/* Enum defining the different states of the mailbox snapshot in the
 * PF-VF mailbox overflow detection algorithm. The
 * snapshot can be in
 * states:
 * 1. NE6X_MAL_VF_DETECT_STATE_NEW_SNAPSHOT - generate a new static snapshot
 * within
 * the mailbox buffer.
 * 2. NE6X_MAL_VF_DETECT_STATE_TRAVERSE - iterate through the mailbox snaphot
 * 3.
 * NE6X_MAL_VF_DETECT_STATE_DETECT - track the messages sent per VF via the
 * mailbox and mark any VFs sending more
 * messages than the threshold limit set.
 * 4. NE6X_MAL_VF_DETECT_STATE_INVALID - Invalid mailbox state set to
 * 0xFFFFFFFF.
 */
enum ne6x_mbx_snapshot_state {
	NE6X_MAL_VF_DETECT_STATE_NEW_SNAPSHOT = 0,
	NE6X_MAL_VF_DETECT_STATE_TRAVERSE,
	NE6X_MAL_VF_DETECT_STATE_DETECT,
	NE6X_MAL_VF_DETECT_STATE_INVALID = 0xF,
};

struct ne6x_mbx_snapshot {
	enum ne6x_mbx_snapshot_state state;
	struct ne6x_mbx_vf_counter mbx_vf;
};

enum virtchnl_vf_config_codes {
	VIRTCHNL_VF_CONFIG_TRUST = 0,
	VIRTCHNL_VF_CONFIG_FORCE_LINK = 1,
};

struct virtchnl_vf_config {
	u8 type;
	u8 data[5];
};

enum ne6x_adapter_state {
	NE6X_ADPT_DOWN,
	NE6X_ADPT_NEEDS_RESTART,
	NE6X_ADPT_NETDEV_ALLOCD,
	NE6X_ADPT_NETDEV_REGISTERED,
	NE6X_ADPT_UMAC_FLTR_CHANGED,
	NE6X_ADPT_MMAC_FLTR_CHANGED,
	NE6X_ADPT_VLAN_FLTR_CHANGED,
	NE6X_ADPT_PROMISC_CHANGED,
	NE6X_ADPT_RELEASING,
	NE6X_ADPT_RECOVER,
	NE6X_ADPT_DOWN_REQUESTED,
	NE6X_ADPT_OPEN,
	NE6X_ADPT_NBITS /* must be last */
};

struct ne6x_adapt_comm {
	u16 port_info;
	DECLARE_BITMAP(state, NE6X_ADPT_NBITS);
};

struct ne6x_vlan {
	u16 tpid;
	u16 vid;
	u8 prio;
};

struct ne6x_vf_vlan {
	u16 vid;
	u16 tpid;
};

struct ne6x_macvlan {
	struct list_head list;
	struct net_device *vdev;
	u8 mac[ETH_ALEN];
};

/* values for UPT1_RSSConf.hashFunc */
enum {
	NE6X_RSS_HASH_TYPE_NONE      = 0x0,
	NE6X_RSS_HASH_TYPE_IPV4      = 0x01,
	NE6X_RSS_HASH_TYPE_IPV4_TCP  = 0x02,
	NE6X_RSS_HASH_TYPE_IPV6      = 0x04,
	NE6X_RSS_HASH_TYPE_IPV6_TCP  = 0x08,
	NE6X_RSS_HASH_TYPE_IPV4_UDP  = 0x10,
	NE6X_RSS_HASH_TYPE_IPV6_UDP  = 0x20,
};

enum {
	NE6X_RSS_HASH_FUNC_NONE      = 0x0,
	NE6X_RSS_HASH_FUNC_TOEPLITZ  = 0x01,
};

#define NE6X_RSS_MAX_KEY_SIZE        40
#define NE6X_RSS_MAX_IND_TABLE_SIZE  128

struct ne6x_rss_info {
	u16 hash_type;
	u16 hash_func;
	u16 hash_key_size;
	u16 ind_table_size;
	u8  hash_key[NE6X_RSS_MAX_KEY_SIZE];
	u8  ind_table[NE6X_RSS_MAX_IND_TABLE_SIZE];
};

#define NE6X_VF_VLAN(vid, tpid) ((struct ne6x_vf_vlan){vid, tpid})

#endif
