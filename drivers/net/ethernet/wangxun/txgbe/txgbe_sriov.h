/*
 * WangXun 10 Gigabit PCI Express Linux driver
 * Copyright (c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 */


#ifndef _TXGBE_SRIOV_H_
#define _TXGBE_SRIOV_H_

/* txgbe driver limit the max number of VFs could be enabled to
 * 63 (TXGBE_MAX_VF_FUNCTIONS - 1)
 */
#define TXGBE_MAX_VFS_DRV_LIMIT  (TXGBE_MAX_VF_FUNCTIONS - 1)

void txgbe_restore_vf_multicasts(struct txgbe_adapter *adapter);
int txgbe_set_vf_vlan(struct txgbe_adapter *adapter, int add, int vid, u16 vf);
void txgbe_set_vmolr(struct txgbe_hw *hw, u16 vf, bool aupe);
void txgbe_msg_task(struct txgbe_adapter *adapter);
int txgbe_set_vf_mac(struct txgbe_adapter *adapter,
		     u16 vf, unsigned char *mac_addr);
void txgbe_disable_tx_rx(struct txgbe_adapter *adapter);
void txgbe_ping_all_vfs(struct txgbe_adapter *adapter);
#ifdef IFLA_VF_MAX
int txgbe_ndo_set_vf_mac(struct net_device *netdev, int queue, u8 *mac);
#ifdef IFLA_VF_VLAN_INFO_MAX
int txgbe_ndo_set_vf_vlan(struct net_device *netdev, int queue, u16 vlan,
			  u8 qos, __be16 vlan_proto);
#else
int txgbe_ndo_set_vf_vlan(struct net_device *netdev, int queue, u16 vlan,
			  u8 qos);
#endif
#ifdef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
int txgbe_ndo_set_vf_bw(struct net_device *netdev, int vf, int min_tx_rate,
			int max_tx_rate);
#else
int txgbe_ndo_set_vf_bw(struct net_device *netdev, int vf, int tx_rate);
#endif /* HAVE_NDO_SET_VF_MIN_MAX_TX_RATE */
#ifdef HAVE_VF_SPOOFCHK_CONFIGURE
int txgbe_ndo_set_vf_spoofchk(struct net_device *netdev, int vf, bool setting);
#endif
#ifdef HAVE_NDO_SET_VF_TRUST
int txgbe_ndo_set_vf_trust(struct net_device *netdev, int vf, bool setting);
#endif
int txgbe_ndo_get_vf_config(struct net_device *netdev,
			    int vf, struct ifla_vf_info *ivi);
#endif /* IFLA_VF_MAX */
int txgbe_disable_sriov(struct txgbe_adapter *adapter);
#ifdef CONFIG_PCI_IOV
int txgbe_vf_configuration(struct pci_dev *pdev, unsigned int event_mask);
void txgbe_enable_sriov(struct txgbe_adapter *adapter);
#endif
int txgbe_pci_sriov_configure(struct pci_dev *dev, int num_vfs);

/*
 * These are defined in txgbe_type.h on behalf of the VF driver
 * but we need them here unwrapped for the PF driver.
 */
#define TXGBE_DEV_ID_SP_VF                      0x1000
#endif /* _TXGBE_SRIOV_H_ */

