// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2008 - 2022 Xel Technology. */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/ipv6.h>

#include "xlnid.h"
#include "xlnid_type.h"
#include "xlnid_sriov.h"

int xlnid_disable_sriov(struct xlnid_adapter *adapter)
{
	return 0;
}

static int xlnid_pci_sriov_enable(struct pci_dev __maybe_unused *dev,
									int __maybe_unused num_vfs)
{
	return 0;
}

static int xlnid_pci_sriov_disable(struct pci_dev *dev)
{
	struct xlnid_adapter *adapter = pci_get_drvdata(dev);
	int err;

	if (!adapter->num_vfs && !pci_num_vf(dev)) {
		return -EINVAL;
	}

	err = xlnid_disable_sriov(adapter);

	return err;
}

int xlnid_pci_sriov_configure(struct pci_dev *dev, int num_vfs)
{
	if (num_vfs == 0) {
		return xlnid_pci_sriov_disable(dev);
	}

	else {
		return xlnid_pci_sriov_enable(dev, num_vfs);
	}
}

int xlnid_set_vf_vlan(struct xlnid_adapter *adapter, int add, int vid, u32 vf)
{
	struct xlnid_hw *hw = &adapter->hw;
	int err;

#ifndef HAVE_VLAN_RX_REGISTER

	/*  If VLAN overlaps with one the PF is currently monitoring make
		sure that we are able to allocate a VLVF entry.  This may be
		redundant but it guarantees PF will maintain visibility to
		the VLAN.
	*/
	if (add && test_bit(vid, adapter->active_vlans)) {
		err = hw->mac.ops.set_vfta(hw, vid, VMDQ_P(0), true, false);

		if (err) {
			return err;
		}
	}

#endif

	err = hw->mac.ops.set_vfta(hw, vid, vf, !!add, false);

	return err;
}

void xlnid_set_vmolr(struct xlnid_hw *hw, u32 vf, bool aupe)
{
	return;
}

static void xlnid_set_vmvir(struct xlnid_adapter *adapter, u16 vid, u16 qos,
							u32 vf)
{
	return;
}

static void xlnid_clear_vmvir(struct xlnid_adapter *adapter, u32 vf)
{
	return;
}

int xlnid_set_vf_mac(struct xlnid_adapter *adapter, int vf,
						unsigned char *mac_addr)
{
	s32 retval = 0;

	xlnid_del_mac_filter(adapter, adapter->vfinfo[vf].vf_mac_addresses, vf);
	retval = xlnid_add_mac_filter(adapter, mac_addr, vf);

	if (retval >= 0) {
		memcpy(adapter->vfinfo[vf].vf_mac_addresses, mac_addr, ETH_ALEN);
	}

	else {
		memset(adapter->vfinfo[vf].vf_mac_addresses, 0, ETH_ALEN);
	}

	return retval;
}

#if 0
int xlnid_vf_configuration(struct pci_dev *pdev, unsigned int event_mask)
{
	unsigned char vf_mac_addr[6];
	struct xlnid_adapter *adapter = pci_get_drvdata(pdev);
	unsigned int vfn = (event_mask & 0x3f);
	bool enable = ((event_mask & 0x10000000U) != 0);

	if (enable) {
		memset(vf_mac_addr, 0, ETH_ALEN);
		memcpy(adapter->vfinfo[vfn].vf_mac_addresses, vf_mac_addr, 6);
	}

	return 0;
}
#endif /* CONFIG_PCI_IOV */

#ifdef HAVE_NDO_SET_VF_TRUST
static inline void xlnid_ping_vf(struct xlnid_adapter *adapter, int vf)
{
	return;
}
#endif

#ifdef HAVE_NDO_SET_VF_TRUST
int xlnid_ndo_set_vf_trust(struct net_device *netdev, int vf, bool setting)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	if (vf >= adapter->num_vfs) {
		return -EINVAL;
	}

	/* nothing to do */
	if (adapter->vfinfo[vf].trusted == setting) {
		return 0;
	}

	adapter->vfinfo[vf].trusted = setting;

	/* reset VF to reconfigure features */
	adapter->vfinfo[vf].clear_to_send = false;
	xlnid_ping_vf(adapter, vf);

	e_info(drv, "VF %u is %strusted\n", vf, setting ? "" : "not ");

	return 0;
}

#endif
#ifdef IFLA_VF_MAX
int xlnid_ndo_set_vf_mac(struct net_device *netdev, int vf, u8 *mac)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	s32 retval = 0;

	if (vf >= adapter->num_vfs) {
		return -EINVAL;
	}

	if (is_valid_ether_addr(mac)) {
		dev_info(pci_dev_to_dev(adapter->pdev), "setting MAC %pM on VF %d\n", mac, vf);
		dev_info(pci_dev_to_dev(adapter->pdev),
					"Reload the VF driver to make this change effective.\n");

		retval = xlnid_set_vf_mac(adapter, vf, mac);

		if (retval >= 0) {
			/* pf_set_mac is used in ESX5.1 and base driver but not in ESX5.5 */
			adapter->vfinfo[vf].pf_set_mac = true;

			if (test_bit(__XLNID_DOWN, &adapter->state)) {
				dev_warn(pci_dev_to_dev(adapter->pdev),
							"The VF MAC address has been set, but the PF device is not up.\n");
				dev_warn(pci_dev_to_dev(adapter->pdev),
							"Bring the PF device up before attempting to use the VF device.\n");
			}
		}

		else {
			dev_warn(pci_dev_to_dev(adapter->pdev),
						"The VF MAC address was NOT set due to invalid or duplicate MAC address.\n");
		}
	}

	else if (is_zero_ether_addr(mac)) {
		unsigned char *vf_mac_addr = adapter->vfinfo[vf].vf_mac_addresses;

		/* nothing to do */
		if (is_zero_ether_addr(vf_mac_addr)) {
			return 0;
		}

		dev_info(pci_dev_to_dev(adapter->pdev), "removing MAC on VF %d\n", vf);

		retval = xlnid_del_mac_filter(adapter, vf_mac_addr, vf);

		if (retval >= 0) {
			adapter->vfinfo[vf].pf_set_mac = false;
			memcpy(vf_mac_addr, mac, ETH_ALEN);
		}

		else {
			dev_warn(pci_dev_to_dev(adapter->pdev),
						"Could NOT remove the VF MAC address.\n");
		}
	}

	else {
		retval = -EINVAL;
	}

	return retval;
}

static int xlnid_enable_port_vlan(struct xlnid_adapter *adapter, int vf,
									u16 vlan, u8 qos)
{
	struct xlnid_hw *hw = &adapter->hw;
	int err;

	err = xlnid_set_vf_vlan(adapter, true, vlan, vf);

	if (err) {
		goto out;
	}

	/* Revoke tagless access via VLAN 0 */
	xlnid_set_vf_vlan(adapter, false, 0, vf);

	xlnid_set_vmvir(adapter, vlan, qos, vf);
	xlnid_set_vmolr(hw, vf, false);

	adapter->vfinfo[vf].pf_vlan = vlan;
	adapter->vfinfo[vf].pf_qos = qos;
	dev_info(pci_dev_to_dev(adapter->pdev), "Setting VLAN %d, QOS 0x%x on VF %d\n",
				vlan, qos, vf);

	if (test_bit(__XLNID_DOWN, &adapter->state)) {
		dev_warn(pci_dev_to_dev(adapter->pdev),
					"The VF VLAN has been set, but the PF device is not up.\n");
		dev_warn(pci_dev_to_dev(adapter->pdev),
					"Bring the PF device up before attempting to use the VF device.\n");
	}

out:
	return err;
}

static int xlnid_disable_port_vlan(struct xlnid_adapter *adapter, int vf)
{
	struct xlnid_hw *hw = &adapter->hw;
	int err;

	err = xlnid_set_vf_vlan(adapter, false, adapter->vfinfo[vf].pf_vlan, vf);
	/* Restore tagless access via VLAN 0 */
	xlnid_set_vf_vlan(adapter, true, 0, vf);
	xlnid_clear_vmvir(adapter, vf);
	xlnid_set_vmolr(hw, vf, true);

	adapter->vfinfo[vf].pf_vlan = 0;
	adapter->vfinfo[vf].pf_qos = 0;

	return err;
}

#ifdef IFLA_VF_VLAN_INFO_MAX
int xlnid_ndo_set_vf_vlan(struct net_device *netdev, int vf, u16 vlan, u8 qos,
							__be16 vlan_proto)
#else
int xlnid_ndo_set_vf_vlan(struct net_device *netdev, int vf, u16 vlan, u8 qos)
#endif
{
	int err = 0;
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	/* VLAN IDs accepted range 0-4094 */
	if ((vf >= adapter->num_vfs) || (vlan > VLAN_VID_MASK - 1) || (qos > 7)) {
		return -EINVAL;
	}

#ifdef IFLA_VF_VLAN_INFO_MAX

	if (vlan_proto != htons(ETH_P_8021Q)) {
		return -EPROTONOSUPPORT;
	}

#endif

	if (vlan || qos) {
		/*
			Check if there is already a port VLAN set, if so
			we have to delete the old one first before we
			can set the new one.  The usage model had
			previously assumed the user would delete the
			old port VLAN before setting a new one but this
			is not necessarily the case.
		*/
		if (adapter->vfinfo[vf].pf_vlan) {
			err = xlnid_disable_port_vlan(adapter, vf);
		}

		if (err) {
			goto out;
		}

		err = xlnid_enable_port_vlan(adapter, vf, vlan, qos);

	}

	else {
		err = xlnid_disable_port_vlan(adapter, vf);
	}

out:
	return err;
}

static int xlnid_link_mbps(struct xlnid_adapter *adapter)
{
	switch (adapter->link_speed) {
	case XLNID_LINK_SPEED_100_FULL:
		return 100;

	case XLNID_LINK_SPEED_1GB_FULL:
		return 1000;

	case XLNID_LINK_SPEED_10GB_FULL:
		return 10000;

	default:
		return 0;
	}
}

#ifdef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
int xlnid_ndo_set_vf_bw(struct net_device *netdev, int vf,
						int __always_unused min_tx_rate, int max_tx_rate)
#else
int xlnid_ndo_set_vf_bw(struct net_device *netdev, int vf, int max_tx_rate)
#endif                          /* HAVE_NDO_SET_VF_MIN_MAX_TX_RATE */
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	int link_speed;

	/* verify VF is active */
	if (vf >= adapter->num_vfs) {
		return -EINVAL;
	}

	/* verify link is up */
	if (!adapter->link_up) {
		return -EINVAL;
	}

	/* verify we are linked at 10Gbps */
	link_speed = xlnid_link_mbps(adapter);

	if (link_speed != 10000) {
		return -EINVAL;
	}

	/* rate limit cannot be less than 10Mbs or greater than link speed */
	if (max_tx_rate && ((max_tx_rate <= 10) || (max_tx_rate > link_speed))) {
		return -EINVAL;
	}

	/* store values */
	adapter->vf_rate_link_speed = link_speed;
	adapter->vfinfo[vf].tx_rate = max_tx_rate;

	/* update hardware configuration */
	//xlnid_set_vf_rate_limit(adapter, vf);

	return 0;
}

#endif /* IFLA_VF_MAX */
#if 0
int xlnid_ndo_set_vf_spoofchk(struct net_device *netdev, int vf, bool setting)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;

	if (vf >= adapter->num_vfs) {
		return -EINVAL;
	}

	adapter->vfinfo[vf].spoofchk_enabled = setting;

	/* configure MAC spoofing */
	hw->mac.ops.set_mac_anti_spoofing(hw, setting, vf);

	/* configure VLAN spoofing */
	hw->mac.ops.set_vlan_anti_spoofing(hw, setting, vf);

	/*  Ensure LLDP and FC is set for Ethertype Antispoofing if we will be
		calling set_ethertype_anti_spoofing for each VF in loop below
	*/
	if (hw->mac.ops.set_ethertype_anti_spoofing) {
		XLNID_WRITE_REG(hw, XLNID_ETQF(XLNID_ETQF_FILTER_LLDP),
						(XLNID_ETQF_FILTER_EN | XLNID_ETQF_TX_ANTISPOOF | XLNID_ETH_P_LLDP));

		XLNID_WRITE_REG(hw, XLNID_ETQF(XLNID_ETQF_FILTER_FC),
						(XLNID_ETQF_FILTER_EN | XLNID_ETQF_TX_ANTISPOOF | ETH_P_PAUSE));

		hw->mac.ops.set_ethertype_anti_spoofing(hw, setting, vf);
	}

	return 0;
}

#endif /* CONFIG_PCI_IOV */
#ifdef IFLA_VF_MAX
#ifdef HAVE_NDO_SET_VF_RSS_QUERY_EN
int xlnid_ndo_set_vf_rss_query_en(struct net_device *netdev, int vf,
									bool setting)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	if (vf >= adapter->num_vfs) {
		return -EINVAL;
	}

	adapter->vfinfo[vf].rss_query_enabled = setting;

	return 0;
}

#endif
int xlnid_ndo_get_vf_config(struct net_device *netdev, int vf,
							struct ifla_vf_info *ivi)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	if (vf >= adapter->num_vfs) {
		return -EINVAL;
	}

	ivi->vf = vf;
	memcpy(&ivi->mac, adapter->vfinfo[vf].vf_mac_addresses, ETH_ALEN);

#ifdef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
	ivi->max_tx_rate = adapter->vfinfo[vf].tx_rate;
	ivi->min_tx_rate = 0;
#else
	ivi->tx_rate = adapter->vfinfo[vf].tx_rate;
#endif /* HAVE_NDO_SET_VF_MIN_MAX_TX_RATE */

	ivi->vlan = adapter->vfinfo[vf].pf_vlan;
	ivi->qos = adapter->vfinfo[vf].pf_qos;
#ifdef HAVE_VF_SPOOFCHK_CONFIGURE
	ivi->spoofchk = adapter->vfinfo[vf].spoofchk_enabled;
#endif
#ifdef HAVE_NDO_SET_VF_RSS_QUERY_EN
	ivi->rss_query_en = adapter->vfinfo[vf].rss_query_enabled;
#endif
#ifdef HAVE_NDO_SET_VF_TRUST
	ivi->trusted = adapter->vfinfo[vf].trusted;
#endif
	return 0;
}
#endif /* IFLA_VF_MAX */
