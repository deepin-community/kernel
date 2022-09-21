/*
 * WangXun Gigabit PCI Express Linux driver
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


#ifndef _NGBE_SRIOV_H_
#define _NGBE_SRIOV_H_

/* ngbe driver limit the max number of VFs could be enabled to
 * 7 (NGBE_MAX_VF_FUNCTIONS - 1)
 */
#define NGBE_MAX_VFS_DRV_LIMIT  (NGBE_MAX_VF_FUNCTIONS - 1)

void ngbe_restore_vf_multicasts(struct ngbe_adapter *adapter);
int ngbe_set_vf_vlan(struct ngbe_adapter *adapter, int add, int vid, u16 vf);
void ngbe_set_vmolr(struct ngbe_hw *hw, u16 vf, bool aupe);
void ngbe_msg_task(struct ngbe_adapter *adapter);
int ngbe_set_vf_mac(struct ngbe_adapter *adapter,
		     u16 vf, unsigned char *mac_addr);
void ngbe_disable_tx_rx(struct ngbe_adapter *adapter);
void ngbe_ping_all_vfs(struct ngbe_adapter *adapter);
#ifdef IFLA_VF_MAX
int ngbe_ndo_set_vf_mac(struct net_device *netdev, int queue, u8 *mac);
#ifdef IFLA_VF_VLAN_INFO_MAX
int ngbe_ndo_set_vf_vlan(struct net_device *netdev, int queue, u16 vlan,
			  u8 qos, __be16 vlan_proto);
#else
int ngbe_ndo_set_vf_vlan(struct net_device *netdev, int queue, u16 vlan,
			  u8 qos);
#endif
#ifdef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
int ngbe_ndo_set_vf_bw(struct net_device *netdev, int vf, int min_tx_rate,
			int max_tx_rate);
#else
int ngbe_ndo_set_vf_bw(struct net_device *netdev, int vf, int tx_rate);
#endif /* HAVE_NDO_SET_VF_MIN_MAX_TX_RATE */
#ifdef HAVE_VF_SPOOFCHK_CONFIGURE
int ngbe_ndo_set_vf_spoofchk(struct net_device *netdev, int vf, bool setting);
#endif
#ifdef HAVE_NDO_SET_VF_TRUST
int ngbe_ndo_set_vf_trust(struct net_device *netdev, int vf, bool setting);
#endif
int ngbe_ndo_get_vf_config(struct net_device *netdev,
			    int vf, struct ifla_vf_info *ivi);
#endif /* IFLA_VF_MAX */
int ngbe_disable_sriov(struct ngbe_adapter *adapter);
#ifdef CONFIG_PCI_IOV
int ngbe_vf_configuration(struct pci_dev *pdev, unsigned int event_mask);
void ngbe_enable_sriov(struct ngbe_adapter *adapter);
#endif
int ngbe_pci_sriov_configure(struct pci_dev *dev, int num_vfs);

#define NGBE_VF_STATUS_LINKUP         0x1

/*
 * These are defined in ngbe_type.h on behalf of the VF driver
 * but we need them here unwrapped for the PF driver.
 */
//#define NGBE_DEV_ID_SP_VF                      0x1000
#endif /* _NGBE_SRIOV_H_ */

