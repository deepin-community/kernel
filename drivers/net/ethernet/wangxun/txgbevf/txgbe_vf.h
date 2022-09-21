/*******************************************************************************

  WangXun(R) 10GbE PCI Express Virtual Function Linux Network Driver
  Copyright(c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:
  Software Team <linux.nic@trustnetic.com>
  WangXun Technology, HuaXing Times Square A507, Hangzhou, China.

*******************************************************************************/

#ifndef __TXGBE_VF_H__
#define __TXGBE_VF_H__

#define TXGBE_VF_IRQ_CLEAR_MASK 7
#define TXGBE_VF_MAX_TX_QUEUES  8
#define TXGBE_VF_MAX_RX_QUEUES  8

/* DCB define */
#define TXGBE_VF_MAX_TRAFFIC_CLASS      8

struct txgbe_hw;

#include "txgbe_type.h"
#include "txgbe_regs.h"
#include "txgbe_mbx.h"

struct txgbe_mac_operations {
	s32 (*init_hw)(struct txgbe_hw *);
	s32 (*reset_hw)(struct txgbe_hw *);
	s32 (*start_hw)(struct txgbe_hw *);
	s32 (*clear_hw_cntrs)(struct txgbe_hw *);
	enum txgbe_media_type (*get_media_type)(struct txgbe_hw *);
	u32 (*get_supported_physical_layer)(struct txgbe_hw *);
	s32 (*get_mac_addr)(struct txgbe_hw *, u8 *);
	s32 (*stop_adapter)(struct txgbe_hw *);
	s32 (*get_bus_info)(struct txgbe_hw *);

	/* Link */
	s32 (*setup_link)(struct txgbe_hw *, txgbe_link_speed, bool);
	s32 (*check_link)(struct txgbe_hw *, txgbe_link_speed *, bool *, bool);
	s32 (*get_link_capabilities)(struct txgbe_hw *, txgbe_link_speed *,
				     bool *);

	/* RAR, Multicast, VLAN */
	s32 (*set_rar)(struct txgbe_hw *, u32, u8 *, u32, u32);
	s32 (*set_uc_addr)(struct txgbe_hw *, u32, u8 *);
	s32 (*init_rx_addrs)(struct txgbe_hw *);
	s32 (*update_mc_addr_list)(struct txgbe_hw *, u8 *, u32,
				   txgbe_mc_addr_itr, bool);
	s32 (*update_xcast_mode)(struct txgbe_hw *, int);
	s32 (*enable_mc)(struct txgbe_hw *);
	s32 (*disable_mc)(struct txgbe_hw *);
	s32 (*clear_vfta)(struct txgbe_hw *);
	s32 (*set_vfta)(struct txgbe_hw *, u32, u32, bool, bool);
};

struct txgbe_mac_info {
	struct txgbe_mac_operations ops;
	u8 addr[6];
	u8 perm_addr[6];

	enum txgbe_mac_type type;

	s32  mc_filter_type;

	bool get_link_status;
	u32  max_tx_queues;
	u32  max_rx_queues;
	u32  max_msix_vectors;
};

struct txgbe_mbx_operations {
	void (*init_params)(struct txgbe_hw *hw);
	s32  (*read)(struct txgbe_hw *, u32 *, u16,  u16);
	s32  (*write)(struct txgbe_hw *, u32 *, u16, u16);
	s32  (*read_posted)(struct txgbe_hw *, u32 *, u16,  u16);
	s32  (*write_posted)(struct txgbe_hw *, u32 *, u16, u16);
	s32  (*check_for_msg)(struct txgbe_hw *, u16);
	s32  (*check_for_ack)(struct txgbe_hw *, u16);
	s32  (*check_for_rst)(struct txgbe_hw *, u16);
};

struct txgbe_mbx_stats {
	u32 msgs_tx;
	u32 msgs_rx;

	u32 acks;
	u32 reqs;
	u32 rsts;
};

struct txgbe_mbx_info {
	struct txgbe_mbx_operations ops;
	struct txgbe_mbx_stats stats;
	u32 timeout;
	u32 udelay;
	u32 v2p_mailbox; /* buffered r2c bits */
	u16 size;
};

struct txgbe_flash_operations {
	s32 (*init_params)(struct txgbe_hw *);
	s32 (*read_buffer)(struct txgbe_hw *, u32, u32, u32 *);
	s32 (*write_buffer)(struct txgbe_hw *, u32, u32, u32 *);
};

struct txgbe_flash_info {
	struct txgbe_flash_operations ops;
	u32 semaphore_delay;
	u32 dword_size;
	u16 address_bits;
};

struct txgbe_hw {
	void *back;
	u16 *msg_enable;
	struct pci_dev *pdev;

	u8 __iomem *hw_addr;
	u8 __iomem *b4_addr;

	struct txgbe_mac_info mac;
	struct txgbe_mbx_info mbx;
	struct txgbe_flash_info flash;

	u16 device_id;
	u16 subsystem_vendor_id;
	u16 subsystem_device_id;
	u16 vendor_id;

	u8  revision_id;
	bool adapter_stopped;

	int api_version;
	
	u32 b4_buf[16];
};

struct txgbe_sw_stats {
	u64 tx_busy;
	u64 tx_restart_queue;
	u64 tx_timeout_count;
	u64 rx_csum_bad;
	u64 rx_no_dma_resources;
	u64 rx_alloc_page_failed;
	u64 rx_alloc_buff_failed;
};

struct txgbe_hw_stats {
	u64 gprc;
	u64 gptc;
	u64 gorc;
	u64 gotc;
	u64 mprc;
};

#define TCALL(hw, func, args...) (((hw)->func != NULL) \
		? (hw)->func((hw), ##args) : -TXGBE_ERR_NOSUPP)

s32 txgbe_init_ops_vf(struct txgbe_hw *hw);
s32 txgbe_init_hw_vf(struct txgbe_hw *hw);
s32 txgbe_start_hw_vf(struct txgbe_hw *hw);
s32 txgbe_reset_hw_vf(struct txgbe_hw *hw);
s32 txgbe_stop_adapter_vf(struct txgbe_hw *hw);
u32 txgbe_get_num_of_tx_queues_vf(struct txgbe_hw *hw);
u32 txgbe_get_num_of_rx_queues_vf(struct txgbe_hw *hw);
s32 txgbe_get_mac_addr_vf(struct txgbe_hw *hw, u8 *mac_addr);
s32 txgbe_setup_mac_link_vf(struct txgbe_hw *hw, txgbe_link_speed speed,
			    bool autoneg_wait_to_complete);
s32 txgbe_check_mac_link_vf(struct txgbe_hw *hw, txgbe_link_speed *speed,
			    bool *link_up, bool autoneg_wait_to_complete);
s32 txgbe_set_rar_vf(struct txgbe_hw *hw, u32 index, u8 *addr, u32 vmdq,
		     u32 enable_addr);
s32 txgbe_set_uc_addr_vf(struct txgbe_hw *hw, u32 index, u8 *addr);
s32 txgbe_update_mc_addr_list_vf(struct txgbe_hw *hw, u8 *mc_addr_list,
				 u32 mc_addr_count, txgbe_mc_addr_itr,
				 bool clear);
s32 txgbe_update_xcast_mode(struct txgbe_hw *hw, int xcast_mode);
s32 txgbe_set_vfta_vf(struct txgbe_hw *hw, u32 vlan, u32 vind,
		      bool vlan_on, bool vlvf_bypass);
s32 txgbe_rlpml_set_vf(struct txgbe_hw *hw, u16 max_size);
int txgbe_negotiate_api_version(struct txgbe_hw *hw, int api);
int txgbe_get_queues(struct txgbe_hw *hw, unsigned int *num_tcs,
		       unsigned int *default_tc);
#endif /* __TXGBE_VF_H__ */
