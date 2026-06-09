/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2008 - 2022 Xel Technology. */


#ifndef _XLNID_SRIOV_H_
#define _XLNID_SRIOV_H_

/*  xlnid driver limit the max number of VFs could be enabled to
		63 (XLNID_MAX_VF_FUNCTIONS - 1)
*/
#define XLNID_MAX_VFS_DRV_LIMIT  (XLNID_MAX_VF_FUNCTIONS - 1)
#define XLNID_MAX_VFS_1TC    XLNID_MAX_VFS_DRV_LIMIT
#define XLNID_MAX_VFS_4TC    31
#define XLNID_MAX_VFS_8TC    15

void xlnid_restore_vf_multicasts(struct xlnid_adapter *adapter);
int xlnid_set_vf_vlan(struct xlnid_adapter *adapter, int add, int vid, u32 vf);
void xlnid_set_vmolr(struct xlnid_hw *hw, u32 vf, bool aupe);
int xlnid_set_vf_mac(struct xlnid_adapter *adapter,
						int vf, unsigned char *mac_addr);
#ifdef IFLA_VF_MAX
int xlnid_ndo_set_vf_mac(struct net_device *netdev, int queue, u8 *mac);
#ifdef IFLA_VF_VLAN_INFO_MAX
int xlnid_ndo_set_vf_vlan(struct net_device *netdev, int queue, u16 vlan,
							u8 qos, __be16 vlan_proto);
#else
int xlnid_ndo_set_vf_vlan(struct net_device *netdev, int queue, u16 vlan,
							u8 qos);
#endif
#ifdef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
int xlnid_ndo_set_vf_bw(struct net_device *netdev, int vf, int min_tx_rate,
						int max_tx_rate);
#else
int xlnid_ndo_set_vf_bw(struct net_device *netdev, int vf, int tx_rate);
#endif /* HAVE_NDO_SET_VF_MIN_MAX_TX_RATE */
#ifdef HAVE_NDO_SET_VF_RSS_QUERY_EN
int xlnid_ndo_set_vf_rss_query_en(struct net_device *netdev, int vf,
									bool setting);
#endif
#ifdef HAVE_NDO_SET_VF_TRUST
int xlnid_ndo_set_vf_trust(struct net_device *netdev, int vf, bool setting);
#endif
int xlnid_ndo_get_vf_config(struct net_device *netdev,
							int vf, struct ifla_vf_info *ivi);
#endif /* IFLA_VF_MAX */
int xlnid_disable_sriov(struct xlnid_adapter *adapter);
#if 0
int xlnid_vf_configuration(struct pci_dev *pdev, unsigned int event_mask);
void xlnid_enable_sriov(struct xlnid_adapter *adapter);
int xlnid_ndo_set_vf_spoofchk(struct net_device *netdev, int vf, bool setting);
#endif
int xlnid_pci_sriov_configure(struct pci_dev *dev, int num_vfs);
void xlnid_dump_registers(struct xlnid_adapter *adapter);

#endif /* _XLNID_SRIOV_H_ */

