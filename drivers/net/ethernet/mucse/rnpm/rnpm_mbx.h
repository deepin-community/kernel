/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2022 - 2025 Mucse Corporation. */

#ifndef _RNPM_MBX_H_
#define _RNPM_MBX_H_

#include "rnpm_type.h"

#define RNPM_VFMAILBOX_SIZE 14 /* 16 32 bit words - 64 bytes */
#define RNPM_ERR_MBX -100

#define RNPM_VT_MSGTYPE_ACK 0x80000000
/* Messages below or'd with
 * this are the ACK
 */
#define RNPM_VT_MSGTYPE_NACK 0x40000000
/* Messages below or'd with
 * this are the NACK
 */
#define RNPM_VT_MSGTYPE_CTS 0x20000000
/* Indicates that VF is still
 * clear to send requests
 */
#define RNPM_VT_MSGINFO_SHIFT 16
/* bits 23:16 are used for exra info for certain messages */
#define RNPM_VT_MSGINFO_MASK (0xFF << RNPM_VT_MSGINFO_SHIFT)
/* VLAN pool filtering masks */
#define RNPM_VLVF_VIEN 0x80000000 /* filter is valid */
#define RNPM_VLVF_ENTRIES 64
#define RNPM_VLVF_VLANID_MASK 0x00000FFF

/* mailbox API, legacy requests */
#define RNPM_VF_RESET 0x01 /* VF requests reset */
#define RNPM_VF_SET_MAC_ADDR 0x02 /* VF requests PF to set MAC addr */
#define RNPM_VF_SET_MULTICAST 0x03 /* VF requests PF to set MC addr */
#define RNPM_VF_SET_VLAN 0x04 /* VF requests PF to set VLAN */

/* mailbox API, version 1.0 VF requests */
#define RNPM_VF_SET_LPE 0x05 /* VF requests PF to set VMOLR.LPE */
#define RNPM_VF_SET_MACVLAN 0x06 /* VF requests PF for unicast filter */
#define RNPM_VF_API_NEGOTIATE 0x08 /* negotiate API version */

/* mailbox API, version 1.1 VF requests */
#define RNPM_VF_GET_QUEUES 0x09 /* get queue configuration */
#define RNPM_VF_SET_VLAN_STRIP 0x0a /* VF requests PF to set VLAN STRIP */
#define RNPM_VF_REG_RD 0x0b /* vf read reg */
#define RNPM_VF_GET_STATUS 0x0c /* vf get pf ethtool setup */

#define RNPM_PF_SET_FCS 0x0c /* PF set fcs status */
#define RNPM_PF_SET_PAUSE 0x0d /* PF set pause status */
#define RNPM_PF_SET_FT_PADDING 0x0e /* PF set ft padding status */

#define RNPM_PF_REMOVE 0x0f
/* GET_QUEUES return data indices within the mailbox */
#define RNPM_VF_TX_QUEUES 1 /* number of Tx queues supported */
#define RNPM_VF_RX_QUEUES 2 /* number of Rx queues supported */
#define RNPM_VF_TRANS_VLAN 3 /* Indication of port vlan */
#define RNPM_VF_DEF_QUEUE 4 /* Default queue offset */

#define RNPM_VF_GET_LINK 0x10 /* get link status */

/* length of permanent address message returned from PF */
#define RNPM_VF_PERMADDR_MSG_LEN 5
/* word in permanent address message with the current multicast type */
#define RNPM_VF_MC_TYPE_WORD 3
#define RNPM_VF_DMA_VERSION_WORD 4

#define RNPM_PF_CONTROL_PRING_MSG 0x0100 /* PF control message */

#define RNPM_VF_MBX_INIT_TIMEOUT 2000 /* number of retries on mailbox */
#define RNPM_VF_MBX_INIT_DELAY 500 /* microseconds between retries */

enum MBX_ID {
	MBX_VF0 = 0,
	MBX_VF1,
	MBX_VF63,
	MBX_CM3CPU,
	MBX_FW = MBX_CM3CPU,
	MBX_VFCNT
};

enum PF_STATUS {
	PF_FCS_STATUS,
	PF_PAUSE_STATUS,
	PF_FT_PADDING_STATUS,
	PF_VLAN_FILTER_STATUS,
};

/* dummy port stats */
struct mbx_port_stat {
	bool duplex;
	int speed;
	bool abs;
	bool link;
};

s32 rnpm_read_mbx(struct rnpm_hw *hw, u32 *msg, u16 size, enum MBX_ID);
s32 rnpm_write_mbx(struct rnpm_hw *hw, u32 *msg, u16 size, enum MBX_ID);
s32 rnpm_check_for_msg(struct rnpm_hw *hw, enum MBX_ID);
s32 rnpm_check_for_ack(struct rnpm_hw *hw, enum MBX_ID);
s32 rnpm_check_for_rst(struct rnpm_hw *hw, enum MBX_ID);

s32 rnpm_init_mbx_params_pf(struct rnpm_hw *hw);

extern struct rnpm_mbx_operations mbx_ops_generic;
struct rnpm_info;
struct rnpm_pf_adapter;

int rnpm_fw_get_macaddr(struct rnpm_hw *hw, int pfvfnum, u8 *mac_addr,
			int lane);
int rnpm_fw_get_pcs_reg(struct rnpm_hw *hw, int nr_lane, int reg,
			int *req_reg);
int rnpm_mbx_fw_reset_phy(struct rnpm_hw *hw);
int rnpm_mbx_get_capability(struct rnpm_hw *hw, struct rnpm_info *info);
int rnpm_fw_msg_handler(struct rnpm_pf_adapter *pf_adapter);
int rnpm_fw_update(struct rnpm_hw *hw, int partition, const u8 *fw_bin,
		   int bytes);
int rnpm_mbx_pf_link_event_enable(struct rnpm_hw *hw, int enable);
int rnpm_mbx_pf_link_event_enable_nolock(struct rnpm_hw *hw, int enable);
int rnpm_mbx_lane_link_changed_event_enable(struct rnpm_hw *hw,
					    int enable);

int rnpm_mbx_led_set(struct rnpm_hw *hw, int value);
#define MBX_IFDOWN (0)
#define MBX_IFUP (1)
#define MBX_PROBE (2)
#define MBX_REMOVE (3)
int rnpm_mbx_ifup_down(struct rnpm_hw *hw, int up);

void rnpm_link_stat_mark_reset(struct rnpm_hw *hw);

int rnpm_mbx_sfp_module_eeprom_info(struct rnpm_hw *hw, int sfp_addr,
				    int reg, int data_len, u8 *buf);
int rnpm_mbx_sfp_write(struct rnpm_hw *hw, int sfp_addr, int reg, short v);
int rnpm_mbx_get_dump(struct rnpm_hw *hw, int flags, u8 *data_out,
		      int bytes);
int rnpm_mbx_set_dump(struct rnpm_hw *hw, int flag);
int rnpm_mbx_phy_link_set(struct rnpm_hw *hw, int speeds);
int rnpm_mbx_get_temp(struct rnpm_hw *hw, int *voltage);
int rnpm_maintain_req(struct rnpm_hw *hw, int cmd, int arg0,
		      int req_data_bytes, int reply_bytes,
		      dma_addr_t dma_phy_addr);
int rnpm_mbx_get_lane_stat(struct rnpm_hw *hw);
int rnpm_set_lane_fun(struct rnpm_hw *hw, int fun, int value0, int value1,
		      int value2, int value3);
void rnpm_link_stat_mark(struct rnpm_hw *hw, int nr_lane, int up);
void rnpm_mbx_probe_stat_set(struct rnpm_pf_adapter *pf_adapter,
			     int probe);
int rnpm_mbx_get_phy_statistics(struct rnpm_hw *hw, u8 *data);
int rnpm_mbx_get_link(struct rnpm_hw *hw);
int rnpm_hw_set_clause73_autoneg_enable(struct rnpm_hw *hw, int enable);
int rnpm_hw_set_fw_10g_1g_auto_detch(struct rnpm_hw *hw, int enable);
int rnpm_mbx_reg_writev(struct rnpm_hw *hw, int fw_reg, int value[4],
			int bytes);
int rnpm_mbx_reg_write(struct rnpm_hw *hw, int fw_reg, int value);
int rnpm_mbx_fw_reg_read(struct rnpm_hw *hw, int fw_reg);
int rnpm_mbx_wol_set(struct rnpm_hw *hw, u32 mode);
int rnpm_mbx_force_speed(struct rnpm_hw *hw, int speed);
int rnpm_mbx_lldp_status_get(struct rnpm_hw *hw);
int rnpm_mbx_lldp_port_enable(struct rnpm_hw *hw, bool enable);

#define cm3_reg_write32(hw, cm3_rpu_reg, v) \
	rnpm_mbx_reg_write((hw), (cm3_rpu_reg), (v))

#define cm3_reg_read32(hw, cm3_rpu_reg) \
	rnpm_mbx_fw_reg_read((hw), (cm3_rpu_reg))

int rnpm_mbx_ddr_csl_enable(struct rnpm_hw *hw, int enable,
			    dma_addr_t dma_phy, int bytes);

int rnpm_mpe_set_queue_info(struct rnpm_pf_adapter *adapter, int lane,
			    int mpe_id_bitmask, int queue_start,
			    int queue_end);
struct rnpm_adapter;
int rnpm_ipsec_rss_update_args(struct rnpm_adapter *adapter);
int rnpm_get_port_stats2(struct rnpm_hw *hw, struct mbx_port_stat *stat);
#endif /* _RNPM_MBX_H_ */
