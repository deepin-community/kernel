/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_NIC_CFG_H
#define HINIC3_NIC_CFG_H

#include <linux/types.h>
#include <linux/netdevice.h>

#include "mag_mpu_cmd.h"
#include "mag_mpu_cmd_defs.h"
#include "nic_cfg_comm.h"
#include "nic_mpu_cmd_defs.h"

#define OS_VF_ID_TO_HW(os_vf_id) ((os_vf_id) + 1)
#define HW_VF_ID_TO_OS(hw_vf_id) ((hw_vf_id) - 1)

#define HINIC3_VLAN_PRIORITY_SHIFT 13

#define HINIC3_RSS_INDIR_4B_UNIT 3
#define HINIC3_RSS_INDIR_NUM 2

#define HINIC3_RSS_KEY_RSV_NUM 2
#define HINIC3_MAX_NUM_RQ 256

#define HINIC3_MIN_MTU_SIZE 256
#define HINIC3_MAX_JUMBO_FRAME_SIZE 9600

#define HINIC3_PF_SET_VF_ALREADY 0x4
#define HINIC3_MGMT_STATUS_EXIST 0x6
#define CHECK_IPSU_15BIT 0x8000

#define HINIC3_MGMT_STATUS_TABLE_EMPTY 0xB /* Table empty */
#define HINIC3_MGMT_STATUS_TABLE_FULL 0xC  /* Table full */

#define HINIC3_LOWEST_LATENCY 3
#define HINIC3_MULTI_VM_LATENCY 32
#define HINIC3_MULTI_VM_PENDING_LIMIT 4

#define HINIC3_RX_RATE_LOW 200000
#define HINIC3_RX_COAL_TIME_LOW 25
#define HINIC3_RX_PENDING_LIMIT_LOW 2

#define HINIC3_RX_RATE_HIGH 700000
#define HINIC3_RX_COAL_TIME_HIGH 225
#define HINIC3_RX_PENDING_LIMIT_HIGH 8

#define HINIC3_RX_RATE_THRESH 50000
#define HINIC3_TX_RATE_THRESH 50000
#define HINIC3_RX_RATE_LOW_VM 100000
#define HINIC3_RX_PENDING_LIMIT_HIGH_VM 87

#define HINIC3_DCB_PCP 0
#define HINIC3_DCB_DSCP 1

#define MAX_LIMIT_BW 100

#define HINIC3_INVALID_BOND_ID 0xffffffff

enum hinic3_valid_link_settings {
	HILINK_LINK_SET_SPEED = 0x1,
	HILINK_LINK_SET_AUTONEG = 0x2,
	HILINK_LINK_SET_FEC = 0x4,
};

enum hinic3_link_follow_status {
	HINIC3_LINK_FOLLOW_DEFAULT,
	HINIC3_LINK_FOLLOW_PORT,
	HINIC3_LINK_FOLLOW_SEPARATE,
	HINIC3_LINK_FOLLOW_STATUS_MAX,
};

enum hinic3_nic_pf_direct {
	HINIC3_NIC_RX = 0,
	HINIC3_NIC_TX,
};

struct hinic3_link_ksettings {
	u32 valid_bitmap;
	u8 speed;   /* enum nic_speed_level */
	u8 autoneg; /* 0 - off; 1 - on */
	u8 fec;	    /* 0 - RSFEC; 1 - BASEFEC; 2 - NOFEC */
};

u64 hinic3_get_feature_cap(void *hwdev);

#define HINIC3_SUPPORT_FEATURE(hwdev, feature) \
	(hinic3_get_feature_cap(hwdev) & NIC_F_##feature)
#define HINIC3_SUPPORT_CSUM(hwdev) HINIC3_SUPPORT_FEATURE(hwdev, CSUM)
#define HINIC3_SUPPORT_SCTP_CRC(hwdev) HINIC3_SUPPORT_FEATURE(hwdev, SCTP_CRC)
#define HINIC3_SUPPORT_TSO(hwdev) HINIC3_SUPPORT_FEATURE(hwdev, TSO)
#define HINIC3_SUPPORT_UFO(hwdev) HINIC3_SUPPORT_FEATURE(hwdev, UFO)
#define HINIC3_SUPPORT_LRO(hwdev) HINIC3_SUPPORT_FEATURE(hwdev, LRO)
#define HINIC3_SUPPORT_RSS(hwdev) HINIC3_SUPPORT_FEATURE(hwdev, RSS)
#define HINIC3_SUPPORT_RXVLAN_FILTER(hwdev) \
	HINIC3_SUPPORT_FEATURE(hwdev, RX_VLAN_FILTER)
#define HINIC3_SUPPORT_VLAN_OFFLOAD(hwdev) \
	(HINIC3_SUPPORT_FEATURE(hwdev, RX_VLAN_STRIP) && \
	 HINIC3_SUPPORT_FEATURE(hwdev, TX_VLAN_INSERT))
#define HINIC3_SUPPORT_VXLAN_OFFLOAD(hwdev) \
	HINIC3_SUPPORT_FEATURE(hwdev, VXLAN_OFFLOAD)
#define HINIC3_SUPPORT_IPSEC_OFFLOAD(hwdev) \
	HINIC3_SUPPORT_FEATURE(hwdev, IPSEC_OFFLOAD)
#define HINIC3_SUPPORT_FDIR(hwdev) HINIC3_SUPPORT_FEATURE(hwdev, FDIR)
#define HINIC3_SUPPORT_PROMISC(hwdev) HINIC3_SUPPORT_FEATURE(hwdev, PROMISC)
#define HINIC3_SUPPORT_ALLMULTI(hwdev) HINIC3_SUPPORT_FEATURE(hwdev, ALLMULTI)
#define HINIC3_SUPPORT_VF_MAC(hwdev) HINIC3_SUPPORT_FEATURE(hwdev, VF_MAC)
#define HINIC3_SUPPORT_RATE_LIMIT(hwdev) HINIC3_SUPPORT_FEATURE(hwdev, RATE_LIMIT)
#define HINIC3_SUPPORT_TX_COMPACT_WQE_OL(hwdev) HINIC3_SUPPORT_FEATURE(hwdev, TX_COMPACT_WQE_OL)
#define HINIC3_SUPPORT_RX_COMPACT_CQE(hwdev) HINIC3_SUPPORT_FEATURE(hwdev, RX_COMPACT_CQE)

#define HINIC3_SUPPORT_RXQ_RECOVERY(hwdev) HINIC3_SUPPORT_FEATURE(hwdev, RXQ_RECOVERY)

struct nic_rss_type {
	u8 tcp_ipv6_ext;
	u8 ipv6_ext;
	u8 tcp_ipv6;
	u8 ipv6;
	u8 tcp_ipv4;
	u8 ipv4;
	u8 udp_ipv6;
	u8 udp_ipv4;
};

enum hinic3_rss_hash_type {
	HINIC3_RSS_HASH_ENGINE_TYPE_XOR = 0,
	HINIC3_RSS_HASH_ENGINE_TYPE_TOEP,
	HINIC3_RSS_HASH_ENGINE_TYPE_MAX,
};

/* rss */
struct nic_rss_indirect_tbl {
	u32 rsvd[4]; /* Make sure that 16B beyond entry[] */
	u16 entry[NIC_RSS_INDIR_SIZE];
};

struct nic_rss_context_tbl {
	u32 rsvd[4];
	u32 ctx;
};

#define NIC_CONFIG_ALL_QUEUE_VLAN_CTX 0xFFFF
struct nic_vlan_ctx {
	u32 func_id;
	u32 qid; /* if qid = 0xFFFF, config current function all queue */
	u32 vlan_tag;
	u32 vlan_mode;
	u32 vlan_sel;
};

enum hinic3_link_status {
	HINIC3_LINK_DOWN = 0,
	HINIC3_LINK_UP
};

struct nic_port_info {
	u8 port_type;
	u8 autoneg_cap;
	u8 autoneg_state;
	u8 duplex;
	u8 speed;
	u8 fec;
	u8 lanes;
	u8 rsvd;
	u32 supported_mode;
	u32 advertised_mode;
	u32 supported_fec_mode;
	u32 bond_speed;
};

struct nic_pause_config {
	u8 auto_neg;
	u8 rx_pause;
	u8 tx_pause;
};

struct rxq_check_info {
	u16	hw_pi;
	u16	hw_ci;
};

struct hinic3_rxq_hw {
	u32	func_id;
	u32	num_queues;

	u32	rsvd[14];
};

#define MODULE_TYPE_SFP 0x3
#define MODULE_TYPE_QSFP28 0x11
#define MODULE_TYPE_QSFP 0x0C
#define MODULE_TYPE_QSFP_PLUS 0x0D
#define MODULE_TYPE_DSFP 0x1B
#define MODULE_TYPE_QSFP_CMIS 0x1E

#define TCAM_IP_TYPE_MASK     0x1
#define TCAM_TUNNEL_TYPE_MASK 0xF
#define TCAM_FUNC_ID_MASK     0x7FFF

int hinic3_delete_bond(void *hwdev);
int hinic3_open_close_bond(void *hwdev, u32 bond_en);
int hinic3_create_bond(void *hwdev, u32 *bond_id);

int hinic3_add_tcam_rule(void *hwdev, struct nic_tcam_cfg_rule *tcam_rule);
int hinic3_del_tcam_rule(void *hwdev, u32 index);

int hinic3_alloc_tcam_block(void *hwdev, u16 *index);
int hinic3_free_tcam_block(void *hwdev, u16 *index);

int hinic3_set_fdir_tcam_rule_filter(void *hwdev, bool enable);

int hinic3_flush_tcam_rule(void *hwdev);

/* *
 * @brief hinic3_update_mac - update mac address to hardware
 * @param hwdev: device pointer to hwdev
 * @param old_mac: old mac to delete
 * @param new_mac: new mac to update
 * @param vlan_id: vlan id
 * @param func_id: function index
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_update_mac(void *hwdev, const u8 *old_mac, u8 *new_mac, u16 vlan_id,
		      u16 func_id);

/* *
 * @brief hinic3_get_default_mac - get default mac address
 * @param hwdev: device pointer to hwdev
 * @param mac_addr: mac address from hardware
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_get_default_mac(void *hwdev, u8 *mac_addr);

/* *
 * @brief hinic3_set_port_mtu - set function mtu
 * @param hwdev: device pointer to hwdev
 * @param new_mtu: mtu
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_set_port_mtu(void *hwdev, u16 new_mtu);

/* *
 * @brief hinic3_get_link_state - get link state
 * @param hwdev: device pointer to hwdev
 * @param link_state: link state, 0-link down, 1-link up
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_get_link_state(void *hwdev, u8 *link_state);

/* *
 * @brief hinic3_get_vport_stats - get function stats
 * @param hwdev: device pointer to hwdev
 * @param func_id: function index
 * @param stats: function stats
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_get_vport_stats(void *hwdev, u16 func_id, struct hinic3_vport_stats *stats);

/* *
 * @brief hinic3_notify_all_vfs_link_changed - notify to all vfs link changed
 * @param hwdev: device pointer to hwdev
 * @param link_status: link state, 0-link down, 1-link up
 */
void hinic3_notify_all_vfs_link_changed(void *hwdev, u8 link_status);

/* *
 * @brief hinic3_force_drop_tx_pkt - force drop tx packet
 * @param hwdev: device pointer to hwdev
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_force_drop_tx_pkt(void *hwdev);

/* *
 * @brief hinic3_set_rx_mode - set function rx mode
 * @param hwdev: device pointer to hwdev
 * @param enable: rx mode state
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_set_rx_mode(void *hwdev, u32 enable);

/* *
 * @brief hinic3_set_rx_vlan_offload - set function vlan offload valid state
 * @param hwdev: device pointer to hwdev
 * @param en: 0-disable, 1-enable
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_set_rx_vlan_offload(void *hwdev, u8 en);

/* *
 * @brief hinic3_set_rx_lro_state - set rx LRO configuration
 * @param hwdev: device pointer to hwdev
 * @param lro_en: 0-disable, 1-enable
 * @param lro_timer: LRO aggregation timeout
 * @param lro_max_pkt_len: LRO coalesce packet size(unit is 1K)
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_set_rx_lro_state(void *hwdev, u8 lro_en, u32 lro_timer,
			    u32 lro_max_pkt_len);

/* *
 * @brief hinic3_set_vf_spoofchk - set vf spoofchk
 * @param hwdev: device pointer to hwdev
 * @param vf_id:  vf id
 * @param spoofchk: spoofchk
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_set_vf_spoofchk(void *hwdev, u16 vf_id, bool spoofchk);

/* *
 * @brief hinic3_vf_info_spoofchk - get vf spoofchk info
 * @param hwdev: device pointer to hwdev
 * @param vf_id: vf id
 * @retval spoofchk state
 */
bool hinic3_vf_info_spoofchk(void *hwdev, int vf_id);

/* *
 * @brief hinic3_add_vf_vlan - add vf vlan id
 * @param hwdev: device pointer to hwdev
 * @param vf_id: vf id
 * @param vlan: vlan id
 * @param qos: qos
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_add_vf_vlan(void *hwdev, int vf_id, u16 vlan, u8 qos);

/* *
 * @brief hinic3_kill_vf_vlan - kill vf vlan
 * @param hwdev: device pointer to hwdev
 * @param vf_id: vf id
 * @param vlan: vlan id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_kill_vf_vlan(void *hwdev, int vf_id);

/* *
 * @brief hinic3_set_vf_mac - set vf mac
 * @param hwdev: device pointer to hwdev
 * @param vf_id: vf id
 * @param mac_addr: vf mac address
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_set_vf_mac(void *hwdev, int vf_id, const unsigned char *mac_addr);

/* *
 * @brief hinic3_vf_info_vlanprio - get vf vlan priority
 * @param hwdev: device pointer to hwdev
 * @param vf_id: vf id
 * @retval zero: vlan priority
 */
u16 hinic3_vf_info_vlanprio(void *hwdev, int vf_id);

/* *
 * @brief hinic3_set_vf_tx_rate - set vf tx rate
 * @param hwdev: device pointer to hwdev
 * @param vf_id: vf id
 * @param max_rate: max rate
 * @param min_rate: min rate
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_set_vf_tx_rate(void *hwdev, u16 vf_id, u32 max_rate, u32 min_rate);

/* *
 * @brief hinic3_set_vf_tx_rate - set vf tx rate
 * @param hwdev: device pointer to hwdev
 * @param vf_id: vf id
 * @param ivi: vf info
 * @retval zero: success
 * @retval non-zero: failure
 */
void hinic3_get_vf_config(void *hwdev, u16 vf_id, struct ifla_vf_info *ivi);

/* *
 * @brief hinic3_set_vf_link_state - set vf link state
 * @param hwdev: device pointer to hwdev
 * @param vf_id: vf id
 * @param link: link state
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_set_vf_link_state(void *hwdev, u16 vf_id, int link);

/* *
 * @brief hinic3_get_port_info - set port info
 * @param hwdev: device pointer to hwdev
 * @param port_info: port info
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_get_port_info(void *hwdev, struct nic_port_info *port_info,
			 u16 channel);

/* *
 * @brief hinic3_set_rss_type - set rss type
 * @param hwdev: device pointer to hwdev
 * @param rss_type: rss type
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_set_rss_type(void *hwdev, struct nic_rss_type rss_type);

/* *
 * @brief hinic3_get_rss_type - get rss type
 * @param hwdev: device pointer to hwdev
 * @param rss_type: rss type
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_get_rss_type(void *hwdev, struct nic_rss_type *rss_type);

/* *
 * @brief hinic3_rss_get_hash_engine - get rss hash engine
 * @param hwdev: device pointer to hwdev
 * @param type: hash engine
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_rss_get_hash_engine(void *hwdev, u8 *type);

/* *
 * @brief hinic3_rss_set_hash_engine - set rss hash engine
 * @param hwdev: device pointer to hwdev
 * @param type: hash engine
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_rss_set_hash_engine(void *hwdev, u8 type);

/* *
 * @brief hinic3_rss_cfg - set rss configuration
 * @param hwdev: device pointer to hwdev
 * @param rss_en: enable rss flag
 * @param type: number of TC
 * @param cos_num: cos num
 * @param num_qps: number of queue
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_rss_cfg(void *hwdev, u8 rss_en, u8 cos_num, u8 *prio_tc,
		   u16 num_qps);

/* *
 * @brief hinic3_rss_set_template_tbl - set template table
 * @param hwdev: device pointer to hwdev
 * @param key: rss key
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_rss_set_hash_key(void *hwdev, const u8 *key);

/* *
 * @brief hinic3_rss_get_template_tbl - get template table
 * @param hwdev: device pointer to hwdev
 * @param key: rss key
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_rss_get_hash_key(void *hwdev, u8 *key);

/* *
 * @brief hinic3_refresh_nic_cfg - refresh port cfg
 * @param hwdev: device pointer to hwdev
 * @param port_info: port information
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_refresh_nic_cfg(void *hwdev, struct nic_port_info *port_info);

/* *
 * @brief hinic3_add_vlan - add vlan
 * @param hwdev: device pointer to hwdev
 * @param vlan_id: vlan id
 * @param func_id: function id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_add_vlan(void *hwdev, u16 vlan_id, u16 func_id);

/* *
 * @brief hinic3_del_vlan - delete vlan
 * @param hwdev: device pointer to hwdev
 * @param vlan_id: vlan id
 * @param func_id: function id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_del_vlan(void *hwdev, u16 vlan_id, u16 func_id);

/* *
 * @brief hinic3_rss_set_indir_tbl - set rss indirect table
 * @param hwdev: device pointer to hwdev
 * @param indir_table: rss indirect table
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_rss_set_indir_tbl(void *hwdev, const u32 *indir_table);

/* *
 * @brief hinic3_rss_get_indir_tbl - get rss indirect table
 * @param hwdev: device pointer to hwdev
 * @param indir_table: rss indirect table
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_rss_get_indir_tbl(void *hwdev, u32 *indir_table);

/* *
 * @brief hinic3_get_phy_port_stats - get port stats
 * @param hwdev: device pointer to hwdev
 * @param stats: port stats
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_get_phy_port_stats(void *hwdev, struct mag_cmd_port_stats *stats);

/* *
 * @brief hinic3_get_phy_rsfec_stats - get rsfec stats
 * @param hwdev: device pointer to hwdev
 * @param stats: rsfec(Reed-Solomon Forward Error Correction) stats
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_get_phy_rsfec_stats(void *hwdev, struct mag_cmd_rsfec_stats *stats);

int hinic3_set_port_funcs_state(void *hwdev, bool enable);

int hinic3_reset_port_link_cfg(void *hwdev);

int hinic3_force_port_relink(void *hwdev);

int hinic3_set_dcb_state(void *hwdev, struct hinic3_dcb_state *dcb_state);

int hinic3_dcb_set_pfc(void *hwdev, u8 pfc_en, u8 pfc_bitmap);

int hinic3_dcb_get_pfc(void *hwdev, u8 *pfc_en_bitmap);

int hinic3_dcb_set_ets(void *hwdev, u8 *cos_tc, u8 *cos_bw, u8 *cos_prio,
		       u8 *tc_bw, u8 *tc_prio);

int hinic3_dcb_set_cos_up_map(void *hwdev, u8 cos_valid_bitmap, u8 *cos_up,
			      u8 max_cos_num);

int hinic3_dcb_set_rq_iq_mapping(void *hwdev, u32 num_rqs, u8 *map,
				 u32 max_map_num);

int hinic3_sync_dcb_state(void *hwdev, u8 op_code, u8 state);

int hinic3_get_pause_info(void *hwdev, struct nic_pause_config *nic_pause);

int hinic3_set_pause_info(void *hwdev, struct nic_pause_config nic_pause);

int hinic3_set_link_settings(void *hwdev,
			     struct hinic3_link_ksettings *settings);

int hinic3_set_vlan_fliter(void *hwdev, u32 vlan_filter_ctrl);

void hinic3_clear_vfs_info(void *hwdev);

int hinic3_notify_vf_outband_cfg(void *hwdev, u16 func_id, u16 vlan_id);

int hinic3_update_mac_vlan(void *hwdev, u16 old_vlan, u16 new_vlan, int vf_id);

int hinic3_set_led_status(void *hwdev, enum mag_led_type type,
			  enum mag_led_mode mode);

int hinic3_set_func_capture_en(void *hwdev, u16 func_id, bool cap_en);

int hinic3_set_loopback_mode(void *hwdev, u8 mode, u8 enable);
int hinic3_get_loopback_mode(void *hwdev, u8 *mode, u8 *enable);

#ifdef HAVE_NDO_SET_VF_TRUST
bool hinic3_get_vf_trust(void *hwdev, int vf_id);
int hinic3_set_vf_trust(void *hwdev, u16 vf_id, bool trust);
#endif

int hinic3_set_autoneg(void *hwdev, bool enable);

int hinic3_get_sfp_type(void *hwdev, u8 *sfp_type, u8 *sfp_type_ext);
int hinic3_get_sfp_eeprom(void *hwdev, u8 *data, u32 len);
int hinic3_get_tlv_xsfp_eeprom(void *hwdev, u8 *data, u32 len);

bool hinic3_if_sfp_absent(void *hwdev);
int hinic3_get_sfp_info(void *hwdev, struct mag_cmd_get_xsfp_info *sfp_info);
int hinic3_get_sfp_tlv_info(void *hwdev,
			    struct drv_tag_mag_cmd_get_xsfp_tlv_rsp *sfp_tlv_info,
			    const struct tag_mag_cmd_get_xsfp_tlv_req *sfp_tlv_info_req);

/* *
 * @brief hinic3_set_nic_feature_to_hw - sync nic feature to hardware
 * @param hwdev: device pointer to hwdev
 */
int hinic3_set_nic_feature_to_hw(void *hwdev);

/* *
 * @brief hinic3_update_nic_feature - update nic feature
 * @param hwdev: device pointer to hwdev
 * @param s_feature: nic features
 * @param size: @s_feature's array size
 */
void hinic3_update_nic_feature(void *hwdev, u64 s_feature);

/* *
 * @brief hinic3_set_link_status_follow - set link follow status
 * @param hwdev: device pointer to hwdev
 * @param status: link follow status
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_set_link_status_follow(void *hwdev, enum hinic3_link_follow_status status);

/* *
 * @brief hinic3_update_pf_bw - update pf bandwidth
 * @param hwdev: device pointer to hwdev
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_update_pf_bw(void *hwdev);

/* *
 * @brief hinic3_set_pf_bw_limit - set pf bandwidth limit
 * @param hwdev: device pointer to hwdev
 * @param bw_limit: pf bandwidth limit
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_set_pf_bw_limit(void *hwdev, u32 bw_limit);

/* *
 * @brief hinic3_set_pf_rate - set pf rate
 * @param hwdev: device pointer to hwdev
 * @param speed_level: speed level
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_set_pf_rate(void *hwdev, u8 speed_level);

int hinic3_get_rxq_hw_info(void *hwdev, struct rxq_check_info *rxq_info, u16 num_qps, u16 wqe_type);

#if defined(HAVE_NDO_UDP_TUNNEL_ADD) || defined(HAVE_UDP_TUNNEL_NIC_INFO)
/* *
 * @brief hinic3_vlxan_port_config - add/del vxlan dst port
 * @param hwdev: device pointer to hwdev
 * @param func_id: function id
 * @param port: vxlan dst port
 * @param action: add or del, del will set to default value (0x12B5)
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_vlxan_port_config(void *hwdev, u16 func_id, u16 port, u8 action);
#endif /* HAVE_NDO_UDP_TUNNEL_ADD || HAVE_UDP_TUNNEL_NIC_INFO */

int hinic3_get_outband_vlan_cfg(void *hwdev, u16 *outband_default_vid);

#endif
