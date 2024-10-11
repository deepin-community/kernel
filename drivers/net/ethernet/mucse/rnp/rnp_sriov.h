/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#ifndef _RNP_SRIOV_H_
#define _RNP_SRIOV_H_

void rnp_restore_vf_multicasts(struct rnp_adapter *adapter);
void rnp_restore_vf_macvlans(struct rnp_adapter *adapter);

void rnp_restore_vf_macs(struct rnp_adapter *adapter);
void rnp_msg_task(struct rnp_adapter *adapter);
int rnp_vf_configuration(struct pci_dev *pdev, unsigned int event_mask);
void rnp_disable_tx_rx(struct rnp_adapter *adapter);
void rnp_ping_all_vfs(struct rnp_adapter *adapter);
int rnp_ndo_set_vf_bw(struct net_device *netdev, int vf,
		      int __always_unused min_tx_rate, int max_tx_rate);
int rnp_ndo_set_vf_mac(struct net_device *netdev, int queue, u8 *mac);
int rnp_msg_post_status(struct rnp_adapter *adapter, enum PF_STATUS status);

int rnp_setup_ring_maxrate(struct rnp_adapter *adapter, int ring, u64 max_rate);
int rnp_get_vf_ringnum(struct rnp_hw *hw, int vf, int num);
int rnp_ndo_set_vf_bw(struct net_device *netdev, int vf,
		      int __always_unused min_tx_rate, int max_tx_rate);
int rnp_ndo_set_vf_spoofchk(struct net_device *netdev, int vf, bool setting);
int rnp_ndo_get_vf_config(struct net_device *netdev, int vf,
			  struct ifla_vf_info *ivi);
void rnp_check_vf_rate_limit(struct rnp_adapter *adapter);
int rnp_disable_sriov(struct rnp_adapter *adapter);
#ifdef CONFIG_PCI_IOV
void rnp_enable_sriov_true(struct rnp_adapter *adapter);
void rnp_enable_sriov(struct rnp_adapter *adapter);
#endif
int rnp_pci_sriov_configure(struct pci_dev *dev, int num_vfs);
int rnp_ndo_set_vf_vlan(struct net_device *netdev, int vf, u16 vlan, u8 qos,
			__be16 vlan_proto);
int rnp_ndo_set_vf_link_state(struct net_device *netdev, int vf, int state);
#ifdef CONFIG_PCI_IOV
int rnp_ndo_set_vf_spoofchk(struct net_device *netdev, int vf, bool setting);
#endif
int rnp_ndo_set_vf_trust(struct net_device *netdev, int vf, bool setting);
#endif /* _RNP_SRIOV_H_ */
