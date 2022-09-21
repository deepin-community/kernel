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
 *
 */


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

#include "ngbe.h"
#include "ngbe_type.h"
#include "ngbe_sriov.h"

#ifdef CONFIG_PCI_IOV
static int __ngbe_enable_sriov(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	int num_vf_macvlans, i;
	struct vf_macvlans *mv_list;

	adapter->flags |= NGBE_FLAG_SRIOV_ENABLED;
	e_dev_info("SR-IOV enabled with %d VFs\n", adapter->num_vfs);

	/* Enable VMDq flag so device will be set in VM mode */
	adapter->flags |= NGBE_FLAG_VMDQ_ENABLED;
	if (!adapter->ring_feature[RING_F_VMDQ].limit)
		adapter->ring_feature[RING_F_VMDQ].limit = 1;
	adapter->ring_feature[RING_F_VMDQ].offset = adapter->num_vfs;

	num_vf_macvlans = hw->mac.num_rar_entries -
		(NGBE_MAX_PF_MACVLANS + 1 + adapter->num_vfs);

	adapter->mv_list = mv_list = kcalloc(num_vf_macvlans,
										sizeof(struct vf_macvlans),
										GFP_KERNEL);
	if (mv_list) {
		/* Initialize list of VF macvlans */
		INIT_LIST_HEAD(&adapter->vf_mvs.l);
		for (i = 0; i < num_vf_macvlans; i++) {
			mv_list->vf = -1;
			mv_list->free = true;
			list_add(&mv_list->l, &adapter->vf_mvs.l);
			mv_list++;
		}
	}

	/* Initialize default switching mode VEB */
	wr32m(hw, NGBE_PSR_CTL,
		NGBE_PSR_CTL_SW_EN, NGBE_PSR_CTL_SW_EN);

	/* If call to enable VFs succeeded then allocate memory
	 * for per VF control structures.
	 */
	adapter->vfinfo = kcalloc(adapter->num_vfs,
			sizeof(struct vf_data_storage), GFP_KERNEL);
	if (!adapter->vfinfo) {
		adapter->num_vfs = 0;
		e_dev_info("failed to allocate memory for VF Data Storage\n");
		return -ENOMEM;
	}

	/* enable L2 switch and replication */
	adapter->flags |= NGBE_FLAG_SRIOV_L2SWITCH_ENABLE |
			  NGBE_FLAG_SRIOV_REPLICATION_ENABLE;
	// NGBE_FLAG_SRIOV_REPLICATION_ENABLE not used

#ifdef NGBE_DISABLE_VF_MQ
	/* We do not support RSS w/ SR-IOV */
	adapter->ring_feature[RING_F_RSS].limit = 1;
#endif

	/* enable spoof checking for all VFs */
	for (i = 0; i < adapter->num_vfs; i++) {
		/* enable spoof checking for all VFs */
		adapter->vfinfo[i].spoofchk_enabled = true;

#ifdef HAVE_NDO_SET_VF_RSS_QUERY_EN
		/* We support VF RSS querying only for 82599 and x540
		 * devices at the moment. These devices share RSS
		 * indirection table and RSS hash key with PF therefore
		 * we want to disable the querying by default.
		 */
		adapter->vfinfo[i].rss_query_enabled = 0;

#endif

		/* Untrust all VFs */
		adapter->vfinfo[i].trusted = false;

		/* set the default xcast mode */
		adapter->vfinfo[i].xcast_mode = NGBEVF_XCAST_MODE_NONE;
	}

	wr32m(hw, NGBE_CFG_PORT_CTL,
		NGBE_CFG_PORT_CTL_NUM_VT_MASK, NGBE_CFG_PORT_CTL_NUM_VT_8);

	return 0;
}

#define NGBE_BA4_ADDR(vfinfo, reg) \
	((u8 __iomem *)((u8 *)(vfinfo)->b4_addr + (reg)))

/**
 * ngbe_get_vfs - Find and take references to all vf devices
 * @adapter: Pointer to adapter struct
 */
static void ngbe_get_vfs(struct ngbe_adapter *adapter)
{
	struct pci_dev *pdev = adapter->pdev;
	u16 vendor = pdev->vendor;
	struct pci_dev *vfdev;
	int vf = 0;
	u16 vf_id;
	int pos;

	pos = pci_find_ext_capability(pdev, PCI_EXT_CAP_ID_SRIOV);
	if (!pos)
		return;
	pci_read_config_word(pdev, pos + PCI_SRIOV_VF_DID, &vf_id);

	vfdev = pci_get_device(vendor, vf_id, NULL);
	for (; vfdev; vfdev = pci_get_device(vendor, vf_id, vfdev)) {
		struct vf_data_storage *vfinfo;
		if (!vfdev->is_virtfn)
			continue;
		if (vfdev->physfn != pdev)
			continue;
		if (vf >= adapter->num_vfs)
			continue;

		/*pci_dev_get(vfdev);*/
		vfinfo = &adapter->vfinfo[vf];
		vfinfo->vfdev = vfdev;
		vfinfo->b4_addr = ioremap(pci_resource_start(vfdev, 4), 64);

		++vf;
	}
}

/**
 * ngbe_pet_vfs - Release references to all vf devices
 * @adapter: Pointer to adapter struct
 */
static void ngbe_put_vfs(struct ngbe_adapter *adapter)
{
	unsigned int num_vfs = adapter->num_vfs, vf;

	/* put the reference to all of the vf devices */
	for (vf = 0; vf < num_vfs; ++vf) {
		struct vf_data_storage *vfinfo;
		struct pci_dev *vfdev = adapter->vfinfo[vf].vfdev;

		if (!vfdev)
			continue;

		vfinfo = &adapter->vfinfo[vf];
		iounmap(vfinfo->b4_addr);
		vfinfo->b4_addr = NULL;
		vfinfo->vfdev = NULL;
		/*pci_dev_put(vfdev);*/
	}
}

/* Note this function is called when the user wants to enable SR-IOV
 * VFs using the now deprecated module parameter
 */
void ngbe_enable_sriov(struct ngbe_adapter *adapter)
{
	int pre_existing_vfs = 0;

	pre_existing_vfs = pci_num_vf(adapter->pdev);
	if (!pre_existing_vfs && !adapter->num_vfs)
		return;

	/* If there are pre-existing VFs then we have to force
	 * use of that many - over ride any module parameter value.
	 * This may result from the user unloading the PF driver
	 * while VFs were assigned to guest VMs or because the VFs
	 * have been created via the new PCI SR-IOV sysfs interface.
	 */
	if (pre_existing_vfs) {
		adapter->num_vfs = pre_existing_vfs;
		dev_warn(&adapter->pdev->dev,
			 "Virtual Functions already enabled for this device -"
			 "Please reload all VF drivers to avoid spoofed packet "
			 "errors\n");
	} else {
		int err;
		/*
		 * The sapphire supports up to 64 VFs per physical function
		 * but this implementation limits allocation to 63 so that
		 * basic networking resources are still available to the
		 * physical function.  If the user requests greater thn
		 * 63 VFs then it is an error - reset to default of zero.
		 */
		adapter->num_vfs = min_t(unsigned int, adapter->num_vfs,
					 NGBE_MAX_VFS_DRV_LIMIT);

		err = pci_enable_sriov(adapter->pdev, adapter->num_vfs);
		if (err) {
			e_err(probe, "Failed to enable PCI sriov: %d\n", err);
			adapter->num_vfs = 0;
			return;
		}
	}

	if (!__ngbe_enable_sriov(adapter)) {
		ngbe_get_vfs(adapter);
		return;
	}

	/* If we have gotten to this point then there is no memory available
	 * to manage the VF devices - print message and bail.
	 */
	e_err(probe, "Unable to allocate memory for VF Data Storage - "
	      "SRIOV disabled\n");
	ngbe_disable_sriov(adapter);
}
#endif /* CONFIG_PCI_IOV */

int ngbe_disable_sriov(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;

#ifdef CONFIG_PCI_IOV
	/*
	 * If our VFs are assigned we cannot shut down SR-IOV
	 * without causing issues, so just leave the hardware
	 * available but disabled
	 */
	if (pci_vfs_assigned(adapter->pdev)) {
		e_dev_warn("Unloading driver while VFs are assigned -"
			   "VFs will not be deallocated\n");
		return -EPERM;
	}
	/* disable iov and allow time for transactions to clear */
	pci_disable_sriov(adapter->pdev);
#endif

	/* set num VFs to 0 to prevent access to vfinfo */
	adapter->num_vfs = 0;

	/* put the reference to all of the vf devices */
#ifdef CONFIG_PCI_IOV
	ngbe_put_vfs(adapter);
#endif
	/* free VF control structures */
	kfree(adapter->vfinfo);
	adapter->vfinfo = NULL;

	/* free macvlan list */
	kfree(adapter->mv_list);
	adapter->mv_list = NULL;

	/* if SR-IOV is already disabled then there is nothing to do */
	if (!(adapter->flags & NGBE_FLAG_SRIOV_ENABLED))
		return 0;

#if 0
#ifdef CONFIG_PCI_IOV
	/*
	 * If our VFs are assigned we cannot shut down SR-IOV
	 * without causing issues, so just leave the hardware
	 * available but disabled
	 */
	if (pci_vfs_assigned(adapter->pdev)) {
		e_dev_warn("Unloading driver while VFs are assigned -"
			   "VFs will not be deallocated\n");
		return -EPERM;
	}
	/* disable iov and allow time for transactions to clear */
	pci_disable_sriov(adapter->pdev);
#endif
#endif

	/* set default pool back to 0 */
	wr32m(hw, NGBE_PSR_VM_CTL,
		NGBE_PSR_VM_CTL_POOL_MASK, 0);
	NGBE_WRITE_FLUSH(hw);

	adapter->ring_feature[RING_F_VMDQ].offset = 0;

	/* take a breather then clean up driver data */
	msleep(100);

	adapter->flags &= ~NGBE_FLAG_SRIOV_ENABLED;

	/* Disable VMDq flag so device will be set in VM mode */
	if (adapter->ring_feature[RING_F_VMDQ].limit == 1) {
		adapter->flags &= ~NGBE_FLAG_VMDQ_ENABLED;
	}

	return 0;
}

static int ngbe_set_vf_multicasts(struct ngbe_adapter *adapter,
				   u32 *msgbuf, u32 vf)
{
	u16 entries = (msgbuf[0] & NGBE_VT_MSGINFO_MASK)
		       >> NGBE_VT_MSGINFO_SHIFT;
	u16 *hash_list = (u16 *)&msgbuf[1];
	struct vf_data_storage *vfinfo = &adapter->vfinfo[vf];
	struct ngbe_hw *hw = &adapter->hw;
	int i;
	u32 vector_bit;
	u32 vector_reg;
	u32 mta_reg;
	u32 vmolr = rd32(hw, NGBE_PSR_VM_L2CTL(vf));

	/* only so many hash values supported */
	entries = min(entries, (u16)NGBE_MAX_VF_MC_ENTRIES);

	/* salt away the number of multi cast addresses assigned
	 * to this VF for later use to restore when the PF multi cast
	 * list changes
	 */
	vfinfo->num_vf_mc_hashes = entries;

	/* VFs are limited to using the MTA hash table for their multicast
	 * addresses */
	for (i = 0; i < entries; i++)
		vfinfo->vf_mc_hashes[i] = hash_list[i];

	for (i = 0; i < vfinfo->num_vf_mc_hashes; i++) {
		vector_reg = (vfinfo->vf_mc_hashes[i] >> 5) & 0x7F;
		vector_bit = vfinfo->vf_mc_hashes[i] & 0x1F;
		/* errata 5: maintain a copy of the register table conf */
		mta_reg = hw->mac.mta_shadow[vector_reg];
		mta_reg |= (1 << vector_bit);
		hw->mac.mta_shadow[vector_reg] = mta_reg;
		wr32(hw, NGBE_PSR_MC_TBL(vector_reg), mta_reg);
	}
	vmolr |= NGBE_PSR_VM_L2CTL_ROMPE;
	wr32(hw, NGBE_PSR_VM_L2CTL(vf), vmolr);

	return 0;
}

void ngbe_restore_vf_multicasts(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	struct vf_data_storage *vfinfo;
	u32 i, j;
	u32 vector_bit;
	u32 vector_reg;

	for (i = 0; i < adapter->num_vfs; i++) {
		u32 vmolr = rd32(hw, NGBE_PSR_VM_L2CTL(i));
		vfinfo = &adapter->vfinfo[i];
		for (j = 0; j < vfinfo->num_vf_mc_hashes; j++) {
			hw->addr_ctrl.mta_in_use++;
			vector_reg = (vfinfo->vf_mc_hashes[j] >> 5) & 0x7F;
			vector_bit = vfinfo->vf_mc_hashes[j] & 0x1F;
			wr32m(hw, NGBE_PSR_MC_TBL(vector_reg),
				1 << vector_bit, 1 << vector_bit);
			/* errata 5: maintain a copy of the reg table conf */
			hw->mac.mta_shadow[vector_reg] |= (1 << vector_bit);
		}
		if (vfinfo->num_vf_mc_hashes)
			vmolr |= NGBE_PSR_VM_L2CTL_ROMPE;
		else
			vmolr &= ~NGBE_PSR_VM_L2CTL_ROMPE;
		wr32(hw, NGBE_PSR_VM_L2CTL(i), vmolr);
	}

	/* Restore any VF macvlans */
	ngbe_full_sync_mac_table(adapter);
}

int ngbe_set_vf_vlan(struct ngbe_adapter *adapter, int add, int vid, u16 vf)
{
	struct ngbe_hw *hw = &adapter->hw;

	/* VLAN 0 is a special case, don't allow it to be removed */
	if (!vid && !add)
		return 0;

	return TCALL(hw, mac.ops.set_vfta, vid, vf, (bool)add);
}

static int ngbe_set_vf_lpe(struct ngbe_adapter *adapter, u32 max_frame,
			    u32 vf)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 max_frs, reg_val;

	/*
	 * For sapphire we have to keep all PFs and VFs operating with
	 * the same max_frame value in order to avoid sending an oversize
	 * frame to a VF.  In order to guarantee this is handled correctly
	 * for all cases we have several special exceptions to take into
	 * account before we can enable the VF for receive
	 */
	struct net_device *dev = adapter->netdev;
	int pf_max_frame = dev->mtu + ETH_HLEN;
	u32 vf_shift, vfre;
	s32 err = 0;



	switch (adapter->vfinfo[vf].vf_api) {
	case ngbe_mbox_api_11:
	case ngbe_mbox_api_12:
	case ngbe_mbox_api_13:
		/*
		 * Version 1.1 supports jumbo frames on VFs if PF has
		 * jumbo frames enabled which means legacy VFs are
		 * disabled
		 */
		if (pf_max_frame > ETH_FRAME_LEN)
			break;
		/* fall through */
	default:
		/*
		 * If the PF or VF are running w/ jumbo frames enabled
		 * we need to shut down the VF Rx path as we cannot
		 * support jumbo frames on legacy VFs
		 */
		if ((pf_max_frame > ETH_FRAME_LEN) ||
		    (max_frame > (ETH_FRAME_LEN + ETH_FCS_LEN)))
			err = -EINVAL;
		break;
	}

	/* determine VF receive enable location */
	vf_shift = vf;

	/* enable or disable receive depending on error */
	vfre = rd32(hw, NGBE_RDM_POOL_RE);
	if (err)
		vfre &= ~(1 << vf_shift);
	else
		vfre |= 1 << vf_shift;
	wr32(hw, NGBE_RDM_POOL_RE, vfre);

	if (err) {
		e_err(drv, "VF max_frame %d out of range\n", max_frame);
		return err;
	}

	/* pull current max frame size from hardware */
	max_frs = DIV_ROUND_UP(max_frame, 1024);
	reg_val = rd32(hw, NGBE_MAC_WDG_TIMEOUT) &
		NGBE_MAC_WDG_TIMEOUT_WTO_MASK;
	if (max_frs > (reg_val + NGBE_MAC_WDG_TIMEOUT_WTO_DELTA)) {
		wr32(hw, NGBE_MAC_WDG_TIMEOUT,
			max_frs - NGBE_MAC_WDG_TIMEOUT_WTO_DELTA);
	}

	e_info(hw, "VF requests change max MTU to %d\n", max_frame);

	return 0;
}

void ngbe_set_vmolr(struct ngbe_hw *hw, u16 vf, bool aupe)
{
	u32 vmolr = rd32(hw, NGBE_PSR_VM_L2CTL(vf));
	vmolr |=  NGBE_PSR_VM_L2CTL_BAM;
	if (aupe)
		vmolr |= NGBE_PSR_VM_L2CTL_AUPE;
	else
		vmolr &= ~NGBE_PSR_VM_L2CTL_AUPE;
	wr32(hw, NGBE_PSR_VM_L2CTL(vf), vmolr);
}

static void ngbe_set_vmvir(struct ngbe_adapter *adapter,
			    u16 vid, u16 qos, u16 vf)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 vmvir = vid | (qos << VLAN_PRIO_SHIFT) |
		NGBE_TDM_VLAN_INS_VLANA_DEFAULT;

	wr32(hw, NGBE_TDM_VLAN_INS(vf), vmvir);
}

static void ngbe_clear_vmvir(struct ngbe_adapter *adapter, u32 vf)
{
	struct ngbe_hw *hw = &adapter->hw;

	wr32(hw, NGBE_TDM_VLAN_INS(vf), 0);
}

static inline void ngbe_vf_reset_event(struct ngbe_adapter *adapter, u16 vf)
{
	struct ngbe_hw *hw = &adapter->hw;
	struct vf_data_storage *vfinfo = &adapter->vfinfo[vf];
	u8 num_tcs = netdev_get_num_tc(adapter->netdev);

	/* add PF assigned VLAN or VLAN 0 */
	ngbe_set_vf_vlan(adapter, true, vfinfo->pf_vlan, vf);

	/* reset offloads to defaults */
	ngbe_set_vmolr(hw, vf, !vfinfo->pf_vlan);

	/* set outgoing tags for VFs */
	if (!vfinfo->pf_vlan && !vfinfo->pf_qos && !num_tcs) {
		ngbe_clear_vmvir(adapter, vf);
	} else {
		if (vfinfo->pf_qos || !num_tcs)
			ngbe_set_vmvir(adapter, vfinfo->pf_vlan,
					vfinfo->pf_qos, vf);
		else
			ngbe_set_vmvir(adapter, vfinfo->pf_vlan,
					adapter->default_up, vf);

		if (vfinfo->spoofchk_enabled)
			TCALL(hw, mac.ops.set_vlan_anti_spoofing, true, vf);
	}

	/* reset multicast table array for vf */
	adapter->vfinfo[vf].num_vf_mc_hashes = 0;

	/* Flush and reset the mta with the new values */
	ngbe_set_rx_mode(adapter->netdev);

	ngbe_del_mac_filter(adapter, adapter->vfinfo[vf].vf_mac_addresses, vf);

	/* reset VF api back to unknown */
	adapter->vfinfo[vf].vf_api = ngbe_mbox_api_10;
}

int ngbe_set_vf_mac(struct ngbe_adapter *adapter,
		     u16 vf, unsigned char *mac_addr)
{
	s32 retval = 0;
	ngbe_del_mac_filter(adapter, adapter->vfinfo[vf].vf_mac_addresses, vf);
	retval = ngbe_add_mac_filter(adapter, mac_addr, vf);
	if (retval >= 0)
		memcpy(adapter->vfinfo[vf].vf_mac_addresses, mac_addr, ETH_ALEN);
	else
		memset(adapter->vfinfo[vf].vf_mac_addresses, 0, ETH_ALEN);

	return retval;
}

static int ngbe_negotiate_vf_api(struct ngbe_adapter *adapter,
				  u32 *msgbuf, u32 vf)
{
	int api = msgbuf[1];

	switch (api) {
	case ngbe_mbox_api_10:
	case ngbe_mbox_api_11:
	case ngbe_mbox_api_12:
	case ngbe_mbox_api_13:
		adapter->vfinfo[vf].vf_api = api;
		return 0;
	default:
		break;
	}

	e_info(drv, "VF %d requested invalid api version %u\n", vf, api);

	return -1;
}

static int ngbe_get_vf_queues(struct ngbe_adapter *adapter,
			       u32 *msgbuf, u32 vf)
{
	struct net_device *dev = adapter->netdev;
	unsigned int default_tc = 0;
	u8 num_tcs = netdev_get_num_tc(dev);

	/* verify the PF is supporting the correct APIs */
	switch (adapter->vfinfo[vf].vf_api) {
	case ngbe_mbox_api_20:
	case ngbe_mbox_api_11:
		break;
	default:
		return -1;
	}

	/* only allow 1 Tx queue for bandwidth limiting */
	msgbuf[NGBE_VF_TX_QUEUES] = 1;
	msgbuf[NGBE_VF_RX_QUEUES] = 1;

	/* notify VF of need for VLAN tag stripping, and correct queue */
	if (num_tcs)
		msgbuf[NGBE_VF_TRANS_VLAN] = num_tcs;
	else if (adapter->vfinfo[vf].pf_vlan || adapter->vfinfo[vf].pf_qos)
		msgbuf[NGBE_VF_TRANS_VLAN] = 1;
	else
		msgbuf[NGBE_VF_TRANS_VLAN] = 0;

	/* notify VF of default queue */
	msgbuf[NGBE_VF_DEF_QUEUE] = default_tc;

	return 0;
}

static int ngbe_get_vf_link_status(struct ngbe_adapter *adapter,
				  u32 *msgbuf, u32 vf)
{
	/* verify the PF is supporting the correct APIs */
	switch (adapter->vfinfo[vf].vf_api) {
	case ngbe_mbox_api_11:
	case ngbe_mbox_api_12:
	case ngbe_mbox_api_13:
		break;
	default:
		return -1;
	}

	if (adapter->link_up)
		msgbuf[1] = NGBE_VF_STATUS_LINKUP;
	else
		msgbuf[1] = 0;

	return 0;
}

static int ngbe_set_vf_macvlan(struct ngbe_adapter *adapter,
				u16 vf, int index, unsigned char *mac_addr)
{
	struct list_head *pos;
	struct vf_macvlans *entry;
	s32 retval = 0;

	if (index <= 1) {
		list_for_each(pos, &adapter->vf_mvs.l) {
			entry = list_entry(pos, struct vf_macvlans, l);
			if (entry->vf == vf) {
				entry->vf = -1;
				entry->free = true;
				entry->is_macvlan = false;
				ngbe_del_mac_filter(adapter,
									entry->vf_macvlan, vf);
			}
		}
	}

	/*
	 * If index was zero then we were asked to clear the uc list
	 * for the VF.  We're done.
	 */
	if (!index)
		return 0;

	entry = NULL;

	list_for_each(pos, &adapter->vf_mvs.l) {
		entry = list_entry(pos, struct vf_macvlans, l);
		if (entry->free)
			break;
	}

	/*
	 * If we traversed the entire list and didn't find a free entry
	 * then we're out of space on the RAR table.  Also entry may
	 * be NULL because the original memory allocation for the list
	 * failed, which is not fatal but does mean we can't support
	 * VF requests for MACVLAN because we couldn't allocate
	 * memory for the list manangbeent required.
	 */
	if (!entry || !entry->free)
		return -ENOSPC;

	retval = ngbe_add_mac_filter(adapter, mac_addr, vf);
	if (retval >= 0) {
		entry->free = false;
		entry->is_macvlan = true;
		entry->vf = vf;
		memcpy(entry->vf_macvlan, mac_addr, ETH_ALEN);
	}

	return retval;
}

#ifdef CONFIG_PCI_IOV
int ngbe_vf_configuration(struct pci_dev *pdev, unsigned int event_mask)
{
	unsigned char vf_mac_addr[6];
	struct ngbe_adapter *adapter = pci_get_drvdata(pdev);
	unsigned int vfn = (event_mask & 0x7);
	bool enable = ((event_mask & 0x10000000U) != 0);

	if (enable) {
		memset(vf_mac_addr, 0, ETH_ALEN);
		memcpy(adapter->vfinfo[vfn].vf_mac_addresses, vf_mac_addr, 6);
	}

	return 0;
}
#endif /* CONFIG_PCI_IOV */

static inline void ngbe_write_qde(struct ngbe_adapter *adapter, u32 vf,
								u32 qde)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 q_per_pool = 1;
	u32 reg = 0;
	u32 i = vf * q_per_pool;

	reg = rd32(hw, NGBE_RDM_PF_QDE);
	reg |= qde << i;

	wr32(hw, NGBE_RDM_PF_QDE, reg);

}

static inline void ngbe_write_hide_vlan(struct ngbe_adapter *adapter, u32 vf,
				   u32 hide_vlan)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 q_per_pool = 1;
	u32 reg = 0;
	u32 i = vf * q_per_pool;
	reg = rd32(hw, NGBE_RDM_PF_HIDE);

	if (hide_vlan == 1)
		reg |= hide_vlan << i;
	else
		reg &= hide_vlan << i;

	wr32(hw, NGBE_RDM_PF_HIDE, reg);
}

static int ngbe_vf_reset_msg(struct ngbe_adapter *adapter, u16 vf)
{
	struct ngbe_hw *hw = &adapter->hw;
	unsigned char *vf_mac = adapter->vfinfo[vf].vf_mac_addresses;
	u32 reg, vf_shift;
	u32 msgbuf[4] = {0, 0, 0, 0};
	u8 *addr = (u8 *)(&msgbuf[1]);
	struct net_device *dev = adapter->netdev;
	int pf_max_frame;

	e_info(probe, "VF Reset msg received from vf %d\n", vf);

	/* reset the filters for the device */
	ngbe_vf_reset_event(adapter, vf);

	/* set vf mac address */
	if (!is_zero_ether_addr(vf_mac))
		ngbe_set_vf_mac(adapter, vf, vf_mac);

	vf_shift = vf;

	/* enable transmit for vf */
	wr32m(hw, NGBE_TDM_POOL_TE,
		1 << vf, 1 << vf);

	/* force drop enable for all VF Rx queues */
	ngbe_write_qde(adapter, vf, 1);

	/* enable receive for vf */
	reg = rd32(hw, NGBE_RDM_POOL_RE);
	reg |= 1 << vf_shift;

	pf_max_frame = dev->mtu + ETH_HLEN;

	if (pf_max_frame > ETH_FRAME_LEN)
		reg &= ~(1 << vf_shift);
	wr32(hw, NGBE_RDM_POOL_RE, reg);

	/* enable VF mailbox for further messages */
	adapter->vfinfo[vf].clear_to_send = true;

	/* reply to reset with ack and vf mac address */
	msgbuf[0] = NGBE_VF_RESET;
	if (!is_zero_ether_addr(vf_mac)) {
		msgbuf[0] |= NGBE_VT_MSGTYPE_ACK;
		memcpy(addr, vf_mac, ETH_ALEN);
	} else {
		msgbuf[0] |= NGBE_VT_MSGTYPE_NACK;
		dev_warn(pci_dev_to_dev(adapter->pdev),
			 "VF %d has no MAC address assigned, you may have to "
			 "assign one manually\n", vf);
	}

	/*
	 * Piggyback the multicast filter type so VF can compute the
	 * correct vectors
	 */
	msgbuf[3] = hw->mac.mc_filter_type;
	ngbe_write_mbx(hw, msgbuf, NGBE_VF_PERMADDR_MSG_LEN, vf);

	return 0;
}

static int ngbe_set_vf_mac_addr(struct ngbe_adapter *adapter,
				 u32 *msgbuf, u16 vf)
{
	u8 *new_mac = ((u8 *)(&msgbuf[1]));

	if (!is_valid_ether_addr(new_mac)) {
		e_warn(drv, "VF %d attempted to set invalid mac\n", vf);
		return -1;
	}

	if (adapter->vfinfo[vf].pf_set_mac &&
	    memcmp(adapter->vfinfo[vf].vf_mac_addresses, new_mac,
		   ETH_ALEN)) {
		u8 *pm = adapter->vfinfo[vf].vf_mac_addresses;
		e_warn(drv,
				"VF %d attempted to set a new MAC address but it already "
				"has an administratively set MAC address "
				"%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X\n",
				vf, pm[0], pm[1], pm[2], pm[3], pm[4], pm[5]);
		e_warn(drv, "Check the VF driver and if it is not using the "
				"correct MAC address you may need to reload the VF "
				"driver\n");
		return -1;
	}
	return ngbe_set_vf_mac(adapter, vf, new_mac) < 0;
}

#ifdef CONFIG_PCI_IOV
static int ngbe_find_vlvf_entry(struct ngbe_hw *hw, u32 vlan)
{
	u32 vlvf;
	s32 regindex;

	/* short cut the special case */
	if (vlan == 0)
		return 0;

	/* Search for the vlan id in the VLVF entries */
	for (regindex = 1; regindex < NGBE_PSR_VLAN_SWC_ENTRIES; regindex++) {
		wr32(hw, NGBE_PSR_VLAN_SWC_IDX, regindex);
		vlvf = rd32(hw, NGBE_PSR_VLAN_SWC);
		if ((vlvf & VLAN_VID_MASK) == vlan)
			break;
	}

	/* Return a negative value if not found */
	if (regindex >= NGBE_PSR_VLAN_SWC_ENTRIES)
		regindex = -1;

	return regindex;
}
#endif /* CONFIG_PCI_IOV */

static int ngbe_set_vf_vlan_msg(struct ngbe_adapter *adapter,
				 u32 *msgbuf, u16 vf)
{
	struct ngbe_hw *hw = &adapter->hw;
	int add = (msgbuf[0] & NGBE_VT_MSGINFO_MASK) >> NGBE_VT_MSGINFO_SHIFT;
	int vid = (msgbuf[1] & NGBE_PSR_VLAN_SWC_VLANID_MASK);
	int err;
	u8 tcs = netdev_get_num_tc(adapter->netdev);

	if (adapter->vfinfo[vf].pf_vlan || tcs) {
		e_warn(drv,
		       "VF %d attempted to override administratively set VLAN "
		       "configuration\n"
		       "Reload the VF driver to resume operations\n",
		       vf);
		return -1;
	}

	if (add)
		adapter->vfinfo[vf].vlan_count++;
	else if (adapter->vfinfo[vf].vlan_count)
		adapter->vfinfo[vf].vlan_count--;

	/* in case of promiscuous mode any VLAN filter set for a VF must
	 * also have the PF pool added to it.
	 */
	if (add && adapter->netdev->flags & IFF_PROMISC)
		err = ngbe_set_vf_vlan(adapter, add, vid, VMDQ_P(0));

	err = ngbe_set_vf_vlan(adapter, add, vid, vf);
	if (!err && adapter->vfinfo[vf].spoofchk_enabled)
		TCALL(hw, mac.ops.set_vlan_anti_spoofing, true, vf);

#ifdef CONFIG_PCI_IOV
	/* Go through all the checks to see if the VLAN filter should
	 * be wiped completely.
	 */
	if (!add && adapter->netdev->flags & IFF_PROMISC) {
		u32 bits = 0, vlvf;
		s32 reg_ndx;

		reg_ndx = ngbe_find_vlvf_entry(hw, vid);
		if (reg_ndx < 0)
			goto out;
		wr32(hw, NGBE_PSR_VLAN_SWC_IDX, reg_ndx);
		vlvf = rd32(hw, NGBE_PSR_VLAN_SWC);
		/* See if any other pools are set for this VLAN filter
		 * entry other than the PF.
		 */
		if (VMDQ_P(0) < 32) {
			bits = rd32(hw, NGBE_PSR_VLAN_SWC_VM_L);
			bits &= ~(1 << VMDQ_P(0));
		} else {
			bits &= ~(1 << (VMDQ_P(0) - 32));
			bits |= rd32(hw, NGBE_PSR_VLAN_SWC_VM_L);
		}

		/* If the filter was removed then ensure PF pool bit
		 * is cleared if the PF only added itself to the pool
		 * because the PF is in promiscuous mode.
		 */
		if ((vlvf & VLAN_VID_MASK) == vid &&
#ifndef HAVE_VLAN_RX_REGISTER
			!test_bit(vid, adapter->active_vlans) &&
#endif
			!bits)
			ngbe_set_vf_vlan(adapter, add, vid, VMDQ_P(0));
	}

out:
#endif
	return err;
}

static int ngbe_set_vf_macvlan_msg(struct ngbe_adapter *adapter,
				    u32 *msgbuf, u16 vf)
{
	u8 *new_mac = ((u8 *)(&msgbuf[1]));
	int index = (msgbuf[0] & NGBE_VT_MSGINFO_MASK) >>
			NGBE_VT_MSGINFO_SHIFT;
	int err;

	if (adapter->vfinfo[vf].pf_set_mac && index > 0) {
		e_warn(drv,
			"VF %d requested MACVLAN filter but is administratively denied\n",
			vf);
		return -1;
	}

	/* An non-zero index indicates the VF is setting a filter */
	if (index) {
		if (!is_valid_ether_addr(new_mac)) {
			e_warn(drv, "VF %d attempted to set invalid mac\n", vf);
			return -1;
		}
#if defined(IFLA_VF_MAX) && defined(HAVE_VF_SPOOFCHK_CONFIGURE)
		/*
		 * If the VF is allowed to set MAC filters then turn off
		 * anti-spoofing to avoid false positives.
		 */
		if (adapter->vfinfo[vf].spoofchk_enabled)
			ngbe_ndo_set_vf_spoofchk(adapter->netdev, vf, false);
#endif /* defined(IFLA_VF_MAX) && defined(HAVE_VF_SPOOFCHK_CONFIGURE) */
	}

	err = ngbe_set_vf_macvlan(adapter, vf, index, new_mac);
	if (err == -ENOSPC)
		e_warn(drv,
		       "VF %d has requested a MACVLAN filter but there is no "
		       "space for it\n",
		       vf);

	return err < 0;
}

static int ngbe_update_vf_xcast_mode(struct ngbe_adapter *adapter,
				      u32 *msgbuf, u32 vf)
{
	struct ngbe_hw *hw = &adapter->hw;
	int xcast_mode = msgbuf[1];
	u32 vmolr, fctrl, disable, enable;

	/* verify the PF is supporting the correct APIs */
	switch (adapter->vfinfo[vf].vf_api) {
	case ngbe_mbox_api_12:
		/* promisc introduced in 1.3 version */
		if (xcast_mode == NGBEVF_XCAST_MODE_PROMISC)
			return -EOPNOTSUPP;
		/* Fall threw */
	case ngbe_mbox_api_13:
		break;
	default:
		return -EOPNOTSUPP;
	}
#if 0 /* trust all vf */
	if (xcast_mode > NGBEVF_XCAST_MODE_MULTI &&
	    !adapter->vfinfo[vf].trusted) {
		xcast_mode = NGBEVF_XCAST_MODE_MULTI;
	}
#endif /* trust all vf */
	if (adapter->vfinfo[vf].xcast_mode == xcast_mode)
		goto out;

	switch (xcast_mode) {
	case NGBEVF_XCAST_MODE_NONE:
		disable = NGBE_PSR_VM_L2CTL_BAM |
				NGBE_PSR_VM_L2CTL_ROMPE |
				NGBE_PSR_VM_L2CTL_MPE |
				NGBE_PSR_VM_L2CTL_UPE |
				NGBE_PSR_VM_L2CTL_VPE;
		enable = 0;
		break;
	case NGBEVF_XCAST_MODE_MULTI:
		disable = NGBE_PSR_VM_L2CTL_MPE |
				NGBE_PSR_VM_L2CTL_UPE |
				NGBE_PSR_VM_L2CTL_VPE;
		enable = NGBE_PSR_VM_L2CTL_BAM |
				NGBE_PSR_VM_L2CTL_ROMPE;
		break;
	case NGBEVF_XCAST_MODE_ALLMULTI:
		disable = NGBE_PSR_VM_L2CTL_UPE |
				NGBE_PSR_VM_L2CTL_VPE;
		enable = NGBE_PSR_VM_L2CTL_BAM |
				NGBE_PSR_VM_L2CTL_ROMPE |
				NGBE_PSR_VM_L2CTL_MPE;
		break;
	case NGBEVF_XCAST_MODE_PROMISC:
		fctrl = rd32(hw, NGBE_PSR_CTL);
		if (!(fctrl & NGBE_PSR_CTL_UPE)) {
			/* VF promisc requires PF in promisc */
			e_warn(drv,
					"Enabling VF promisc requires PF in promisc\n");
			return -EPERM;
		}
		disable = 0;
		enable = NGBE_PSR_VM_L2CTL_BAM |
				NGBE_PSR_VM_L2CTL_ROMPE |
				NGBE_PSR_VM_L2CTL_MPE |
				NGBE_PSR_VM_L2CTL_UPE |
				NGBE_PSR_VM_L2CTL_VPE;
		break;
	default:
		return -EOPNOTSUPP;
	}

	vmolr = rd32(hw, NGBE_PSR_VM_L2CTL(vf));
	vmolr &= ~disable;
	vmolr |= enable;
	wr32(hw, NGBE_PSR_VM_L2CTL(vf), vmolr);

	adapter->vfinfo[vf].xcast_mode = xcast_mode;

out:
	msgbuf[1] = xcast_mode;

	return 0;
}

static int ngbe_rcv_msg_from_vf(struct ngbe_adapter *adapter, u16 vf)
{
	u16 mbx_size = NGBE_VXMAILBOX_SIZE;
	u32 msgbuf[NGBE_VXMAILBOX_SIZE];
	struct ngbe_hw *hw = &adapter->hw;
	s32 retval;

	retval = ngbe_read_mbx(hw, msgbuf, mbx_size, vf);

	if (retval) {
		pr_err("Error receiving message from VF\n");
		return retval;
	}

	/* this is a message we already processed, do nothing */
	if (msgbuf[0] & (NGBE_VT_MSGTYPE_ACK | NGBE_VT_MSGTYPE_NACK))
		return retval;

	/* flush the ack before we write any messages back */
	NGBE_WRITE_FLUSH(hw);

	if (msgbuf[0] == NGBE_VF_RESET)
		return ngbe_vf_reset_msg(adapter, vf);

	/*
	 * until the vf completes a virtual function reset it should not be
	 * allowed to start any configuration.
	 */

	if (!adapter->vfinfo[vf].clear_to_send) {
		msgbuf[0] |= NGBE_VT_MSGTYPE_NACK;
		ngbe_write_mbx(hw, msgbuf, 1, vf);
		return retval;
	}

	switch ((msgbuf[0] & 0xFFFF)) {
	case NGBE_VF_SET_MAC_ADDR:
		retval = ngbe_set_vf_mac_addr(adapter, msgbuf, vf);
		break;
	case NGBE_VF_SET_MULTICAST:
		retval = ngbe_set_vf_multicasts(adapter, msgbuf, vf);
		break;
	case NGBE_VF_SET_VLAN:
		retval = ngbe_set_vf_vlan_msg(adapter, msgbuf, vf);
		break;
	case NGBE_VF_SET_LPE:
		if (msgbuf[1] > NGBE_MAX_JUMBO_FRAME_SIZE) {
			e_err(drv, "VF max_frame %d exceed MAX_JUMBO_FRAME_SIZE\n", msgbuf[1]);
			return -EINVAL;
		}
		retval = ngbe_set_vf_lpe(adapter, msgbuf[1], vf);
		break;
	case NGBE_VF_SET_MACVLAN:
		retval = ngbe_set_vf_macvlan_msg(adapter, msgbuf, vf);
		break;
	case NGBE_VF_API_NEGOTIATE:
		retval = ngbe_negotiate_vf_api(adapter, msgbuf, vf);
		break;
	case NGBE_VF_GET_QUEUES:
		retval = ngbe_get_vf_queues(adapter, msgbuf, vf);
		break;
	case NGBE_VF_UPDATE_XCAST_MODE:
		retval = ngbe_update_vf_xcast_mode(adapter, msgbuf, vf);
		break;
	case NGBE_VF_GET_LINK_STATUS:
		retval = ngbe_get_vf_link_status(adapter, msgbuf, vf);
		break;
	case NGBE_VF_BACKUP:
		break;
	default:
		e_err(drv, "Unhandled Msg %8.8x\n", msgbuf[0]);
		retval = NGBE_ERR_MBX;
		break;
	}

	/* notify the VF of the results of what it sent us */
	if (retval)
		msgbuf[0] |= NGBE_VT_MSGTYPE_NACK;
	else
		msgbuf[0] |= NGBE_VT_MSGTYPE_ACK;

	msgbuf[0] |= NGBE_VT_MSGTYPE_CTS;

	ngbe_write_mbx(hw, msgbuf, mbx_size, vf);

	return retval;
}

static void ngbe_rcv_ack_from_vf(struct ngbe_adapter *adapter, u16 vf)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 msg = NGBE_VT_MSGTYPE_NACK;

	/* if device isn't clear to send it shouldn't be reading either */
	if (!adapter->vfinfo[vf].clear_to_send)
		ngbe_write_mbx(hw, &msg, 1, vf);
}

void ngbe_msg_task(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	u16 vf;

	for (vf = 0; vf < adapter->num_vfs; vf++) {
		/* process any reset requests */
		if (!ngbe_check_for_rst(hw, vf))
			ngbe_vf_reset_event(adapter, vf);

		/* process any messages pending */
		if (!ngbe_check_for_msg(hw, vf))
			ngbe_rcv_msg_from_vf(adapter, vf);

		/* process any acks */
		if (!ngbe_check_for_ack(hw, vf))
			ngbe_rcv_ack_from_vf(adapter, vf);
	}
}

void ngbe_disable_tx_rx(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;

	/* disable transmit and receive for all vfs */
	wr32(hw, NGBE_TDM_POOL_TE, 0);
	wr32(hw, NGBE_RDM_POOL_RE, 0);
}

#ifdef HAVE_NDO_SET_VF_TRUST
static inline void ngbe_ping_vf(struct ngbe_adapter *adapter, int vf)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 ping;

	ping = NGBE_PF_CONTROL_MSG;
	if (adapter->vfinfo[vf].clear_to_send)
		ping |= NGBE_VT_MSGTYPE_CTS;
	ngbe_write_mbx(hw, &ping, 1, vf);
}
#endif

void ngbe_ping_all_vfs(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 ping;
	u16 i;

	for (i = 0 ; i < adapter->num_vfs; i++) {
		ping = NGBE_PF_CONTROL_MSG;
		if (adapter->vfinfo[i].clear_to_send)
			ping |= NGBE_VT_MSGTYPE_CTS;
		ngbe_write_mbx(hw, &ping, 1, i);
	}
}

#ifdef HAVE_NDO_SET_VF_TRUST
int ngbe_ndo_set_vf_trust(struct net_device *netdev, int vf, bool setting)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);

	if (vf >= adapter->num_vfs)
		return -EINVAL;

	/* nothing to do */
	if (adapter->vfinfo[vf].trusted == setting)
		return 0;

	adapter->vfinfo[vf].trusted = setting;

	/* reset VF to reconfigure features */
	adapter->vfinfo[vf].clear_to_send = false;
	ngbe_ping_vf(adapter, vf);

	e_info(drv, "VF %u is %strusted\n", vf, setting ? "" : "not ");

	return 0;
}
#endif

#ifdef CONFIG_PCI_IOV
static int ngbe_pci_sriov_enable(struct pci_dev *dev, int num_vfs)
{
	struct ngbe_adapter *adapter = pci_get_drvdata(dev);
	int err = 0;
	int i;
	int pre_existing_vfs = pci_num_vf(dev);

	if (!(adapter->flags & NGBE_FLAG_SRIOV_CAPABLE)) {
		e_dev_warn("SRIOV not supported on this device\n");
		return -EOPNOTSUPP;
	}

	if (pre_existing_vfs && pre_existing_vfs != num_vfs)
		err = ngbe_disable_sriov(adapter);
	else if (pre_existing_vfs && pre_existing_vfs == num_vfs)
		goto out;

	if (err)
		goto err_out;

	/* While the SR-IOV capability structure reports total VFs to be
	 * 8 we limit the actual number that can be allocated to 7 so
	 * that some transmit/receive resources can be reserved to the
	 * PF.  The PCI bus driver already checks for other values out of
	 * range.
	 */
	if ((num_vfs + adapter->num_vmdqs) > NGBE_MAX_VF_FUNCTIONS) {
		err = -EPERM;
		goto err_out;
	}

	adapter->num_vfs = num_vfs;

	err = __ngbe_enable_sriov(adapter);
	if (err)
		goto err_out;

	for (i = 0; i < adapter->num_vfs; i++)
		ngbe_vf_configuration(dev, (i | 0x10000000));

	err = pci_enable_sriov(dev, num_vfs);
	if (err) {
		e_dev_warn("Failed to enable PCI sriov: %d\n", err);
		goto err_out;
	}
	ngbe_get_vfs(adapter);
	msleep(100);
	ngbe_sriov_reinit(adapter);
out:
	return num_vfs;
err_out:
	return err;
}

static int ngbe_pci_sriov_disable(struct pci_dev *dev)
{
	struct ngbe_adapter *adapter = pci_get_drvdata(dev);
	int err;
	u32 current_flags = adapter->flags;

	err = ngbe_disable_sriov(adapter);

	/* Only reinit if no error and state changed */
	if (!err && current_flags != adapter->flags)
		ngbe_sriov_reinit(adapter);

	return err;
}
#endif

int ngbe_pci_sriov_configure(struct pci_dev __maybe_unused *dev,
							 int __maybe_unused num_vfs)
{
#ifdef CONFIG_PCI_IOV
	if (num_vfs == 0)
		return ngbe_pci_sriov_disable(dev);
	else
		return ngbe_pci_sriov_enable(dev, num_vfs);
#endif
	return 0;
}

#ifdef IFLA_VF_MAX
int ngbe_ndo_set_vf_mac(struct net_device *netdev, int vf, u8 *mac)
{
	s32 retval = 0;
	struct ngbe_adapter *adapter = netdev_priv(netdev);

	if (!is_valid_ether_addr(mac) || (vf >= adapter->num_vfs))
		return -EINVAL;

	dev_info(pci_dev_to_dev(adapter->pdev),
		"setting MAC %pM on VF %d\n", mac, vf);
	dev_info(pci_dev_to_dev(adapter->pdev),
		"Reload the VF driver to make this change effective.\n");
	retval = ngbe_set_vf_mac(adapter, vf, mac);
	if (retval >= 0) {
		adapter->vfinfo[vf].pf_set_mac = true;
		if (test_bit(__NGBE_DOWN, &adapter->state)) {
			dev_warn(pci_dev_to_dev(adapter->pdev),
				"The VF MAC address has been set, but the PF "
				"device is not up.\n");
			dev_warn(pci_dev_to_dev(adapter->pdev),
				"Bring the PF device up before attempting to "
				"use the VF device.\n");
		}
	} else {
		dev_warn(pci_dev_to_dev(adapter->pdev),
			"The VF MAC address was NOT set due to invalid or "
			"duplicate MAC address.\n");
	}

	return retval;
}

static int ngbe_enable_port_vlan(struct ngbe_adapter *adapter,
				   int vf, u16 vlan, u8 qos)
{
	struct ngbe_hw *hw = &adapter->hw;
	int err;

	err = ngbe_set_vf_vlan(adapter, true, vlan, vf);
	if (err)
		goto out;
	ngbe_set_vmvir(adapter, vlan, qos, vf);
	ngbe_set_vmolr(hw, vf, false);
	if (adapter->vfinfo[vf].spoofchk_enabled)
		TCALL(hw, mac.ops.set_vlan_anti_spoofing, true, vf);
	adapter->vfinfo[vf].vlan_count++;
	/* enable hide vlan */
	ngbe_write_qde(adapter, vf, 1);
	ngbe_write_hide_vlan(adapter, vf, 1);
	adapter->vfinfo[vf].pf_vlan = vlan;
	adapter->vfinfo[vf].pf_qos = qos;
	dev_info(pci_dev_to_dev(adapter->pdev),
		 "Setting VLAN %d, QOS 0x%x on VF %d\n", vlan, qos, vf);
	if (test_bit(__NGBE_DOWN, &adapter->state)) {
		dev_warn(pci_dev_to_dev(adapter->pdev),
			"The VF VLAN has been set, but the PF device is not "
			"up.\n");
		dev_warn(pci_dev_to_dev(adapter->pdev),
			"Bring the PF device up before attempting to use the VF "
			"device.\n");
	}

out:
	return err;
}

static int ngbe_disable_port_vlan(struct ngbe_adapter *adapter, int vf)
{
	struct ngbe_hw *hw = &adapter->hw;
	int err;

	err = ngbe_set_vf_vlan(adapter, false,
				adapter->vfinfo[vf].pf_vlan, vf);
	ngbe_clear_vmvir(adapter, vf);
	ngbe_set_vmolr(hw, vf, true);
	TCALL(hw, mac.ops.set_vlan_anti_spoofing, false, vf);
	if (adapter->vfinfo[vf].vlan_count)
		adapter->vfinfo[vf].vlan_count--;
	/* disable hide vlan */
	ngbe_write_hide_vlan(adapter, vf, 0);
	adapter->vfinfo[vf].pf_vlan = 0;
	adapter->vfinfo[vf].pf_qos = 0;

	return err;
}
#ifdef IFLA_VF_MAX
#ifdef IFLA_VF_VLAN_INFO_MAX
int ngbe_ndo_set_vf_vlan(struct net_device *netdev, int vf, u16 vlan,
			  u8 qos, __be16 vlan_proto)
#else
int ngbe_ndo_set_vf_vlan(struct net_device *netdev, int vf, u16 vlan, u8 qos)
#endif
{
	int err = 0;
	struct ngbe_adapter *adapter = netdev_priv(netdev);

	/* VLAN IDs accepted range 0-4094 */
	if ((vf >= adapter->num_vfs) || (vlan > VLAN_VID_MASK-1) || (qos > 7))
		return -EINVAL;
#ifdef IFLA_VF_VLAN_INFO_MAX
	if (vlan_proto != htons(ETH_P_8021Q))
		return -EPROTONOSUPPORT;
#endif
	if (vlan || qos) {
		/*
		 * Check if there is already a port VLAN set, if so
		 * we have to delete the old one first before we
		 * can set the new one.  The usage model had
		 * previously assumed the user would delete the
		 * old port VLAN before setting a new one but this
		 * is not necessarily the case.
		 */
		if (adapter->vfinfo[vf].pf_vlan)
			err = ngbe_disable_port_vlan(adapter, vf);
		if (err)
			goto out;
		err = ngbe_enable_port_vlan(adapter, vf, vlan, qos);

	} else {
		err = ngbe_disable_port_vlan(adapter, vf);
	}
out:
	return err;
}
#endif /* IFLA_VF_MAX */
#if 0
static void ngbe_set_vf_rate_limit(struct ngbe_adapter *adapter, int vf)
{
	struct ngbe_ring_feature *vmdq = &adapter->ring_feature[RING_F_VMDQ];
	struct ngbe_hw *hw = &adapter->hw;
	u32 bcnrc_val;
	u16 queue, queues_per_pool;
	u16 max_tx_rate = adapter->vfinfo[vf].max_tx_rate;
#ifdef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
	u16 min_tx_rate = adapter->vfinfo[vf].min_tx_rate;
#endif

	/* determine how many queues per pool based on VMDq mask */
	queues_per_pool = __ALIGN_MASK(1, ~vmdq->mask);

	max_tx_rate /= queues_per_pool;
	bcnrc_val = NGBE_TDM_RP_RATE_MAX(max_tx_rate);
#ifdef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
	min_tx_rate /= queues_per_pool;
	bcnrc_val |= NGBE_TDM_RP_RATE_MIN(min_tx_rate);
#endif

	/*
	 * Set global transmit compensation time to the MMW_SIZE in RTTBCNRM
	 * register. Typically MMW_SIZE=0x014 if 9728-byte jumbo is supported
	 * and 0x004 otherwise.
	 */
	wr32(hw, NGBE_TDM_MMW, 0x14);

	/* write value for all Tx queues belonging to VF */
	for (queue = 0; queue < queues_per_pool; queue++) {
		unsigned int reg_idx = (vf * queues_per_pool) + queue;

		wr32(hw, NGBE_TDM_RP_IDX, reg_idx);
		wr32(hw, NGBE_TDM_RP_RATE, bcnrc_val);
		if (max_tx_rate)
			wr32m(hw, NGBE_TDM_RP_CTL,
				NGBE_TDM_RP_CTL_RLEN, NGBE_TDM_RP_CTL_RLEN);
		else
			wr32m(hw, NGBE_TDM_RP_CTL,
				NGBE_TDM_RP_CTL_RLEN, 0);
	}
}
#endif
#ifdef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
int ngbe_ndo_set_vf_bw(struct net_device *netdev,
			int vf,
			int min_tx_rate,
			int max_tx_rate)
#else
int ngbe_ndo_set_vf_bw(struct net_device *netdev, int vf, int max_tx_rate)
#endif /* HAVE_NDO_SET_VF_MIN_MAX_TX_RATE */
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);

	/* verify VF is active */
	if (vf >= adapter->num_vfs)
		return -EINVAL;

	/* verify link is up */
	if (!adapter->link_up)
		return -EINVAL;

	/* verify we are linked at 1 or 10 Gbps */
	if (adapter->link_speed < NGBE_LINK_SPEED_1GB_FULL)
		return -EINVAL;

	/* store values */
#ifdef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
	adapter->vfinfo[vf].min_tx_rate = min_tx_rate;
#endif
	adapter->vfinfo[vf].max_tx_rate = max_tx_rate;

	return 0;
}

#ifdef HAVE_VF_SPOOFCHK_CONFIGURE
int ngbe_ndo_set_vf_spoofchk(struct net_device *netdev, int vf, bool setting)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;
	u32 regval;

	if (vf >= adapter->num_vfs)
		return -EINVAL;

	adapter->vfinfo[vf].spoofchk_enabled = setting;

	if (vf < 32) {
		regval = (setting << vf);
		wr32m(hw, NGBE_TDM_MAC_AS_L,
			regval | (1 << vf), regval);

		if (adapter->vfinfo[vf].vlan_count) {
			wr32m(hw, NGBE_TDM_VLAN_AS_L,
				regval | (1 << vf), regval);
		}
	}

	return 0;
}
#endif /* HAVE_VF_SPOOFCHK_CONFIGURE */
int ngbe_ndo_get_vf_config(struct net_device *netdev,
			    int vf, struct ifla_vf_info *ivi)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	if (vf >= adapter->num_vfs)
		return -EINVAL;
	ivi->vf = vf;
	memcpy(&ivi->mac, adapter->vfinfo[vf].vf_mac_addresses, ETH_ALEN);

#ifdef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
	ivi->max_tx_rate = adapter->vfinfo[vf].max_tx_rate;
	ivi->min_tx_rate = adapter->vfinfo[vf].min_tx_rate;
#else
	ivi->tx_rate = adapter->vfinfo[vf].max_tx_rate;
#endif

	ivi->vlan = adapter->vfinfo[vf].pf_vlan;
	ivi->qos = adapter->vfinfo[vf].pf_qos;
#ifdef HAVE_VF_SPOOFCHK_CONFIGURE
	ivi->spoofchk = adapter->vfinfo[vf].spoofchk_enabled;
#endif
#ifdef HAVE_NDO_SET_VF_TRUST
	ivi->trusted = adapter->vfinfo[vf].trusted;
#endif

	return 0;
}
#endif /* IFLA_VF_MAX */

