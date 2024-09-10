// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

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
#include <linux/if_vlan.h>

#include "rnpgbe.h"
#include "rnpgbe_type.h"
#include "rnpgbe_sriov.h"

#ifdef CONFIG_PCI_IOV
static int __rnpgbe_enable_sriov(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	int num_vf_macvlans, i, num_vebvlans;
	struct vf_macvlans *mv_list;
	struct vf_vebvlans *vv_list = NULL;

	dbg("%s:%d flags:0x%x\n", __func__, __LINE__, adapter->flags);
	/* sriov and dcb cannot open together */
	/* reset numtc */
	adapter->flags &= (~RNP_FLAG_DCB_ENABLED);
	netdev_reset_tc(adapter->netdev);

	e_info(probe, "SR-IOV enabled with %d VFs\n", adapter->num_vfs);

	dbg("%s:%d flags:0x%x\n", __func__, __LINE__, adapter->flags);

	/* Enable VMDq flag so device will be set in VM mode */
	adapter->flags |= RNP_FLAG_VMDQ_ENABLED;
	if (!adapter->ring_feature[RING_F_VMDQ].limit)
		adapter->ring_feature[RING_F_VMDQ].limit = 1;
	if (hw->feature_flags & RNP_NET_FEATURE_VF_FIXED)
		adapter->ring_feature[RING_F_VMDQ].offset = 0;
	else
		adapter->ring_feature[RING_F_VMDQ].offset = hw->max_vfs - 1;

	num_vf_macvlans = hw->num_rar_entries -
			  (hw->max_pf_macvlans + 1 + adapter->num_vfs);
	num_vebvlans = hw->num_vebvlan_entries;

	adapter->mv_list = mv_list = kcalloc(
		num_vf_macvlans, sizeof(struct vf_macvlans), GFP_KERNEL);
	if (num_vebvlans)
		hw->vv_list = vv_list = kcalloc(
			num_vebvlans, sizeof(struct vf_vebvlans), GFP_KERNEL);

	if (mv_list) {
		/* Initialize list of VF macvlans */
		INIT_LIST_HEAD(&adapter->vf_mvs.l);
		for (i = 0; i < num_vf_macvlans; i++) {
			mv_list->vf = -1;
			mv_list->free = true;
			mv_list->rar_entry = hw->mac.num_rar_entries -
					     (i + adapter->num_vfs + 1);
			list_add(&mv_list->l, &adapter->vf_mvs.l);
			mv_list++;
		}
	}

	if (vv_list) {
		/* Initialize list of VF macvlans */
		INIT_LIST_HEAD(&hw->vf_vas.l);
		for (i = 0; i < num_vebvlans; i++) {
			vv_list->vid = -1;
			vv_list->vid = 0;
			vv_list->free = true;
			vv_list->veb_entry = i;
			list_add(&vv_list->l, &hw->vf_vas.l);
			vv_list++;
		}
	}

	adapter->flags2 |= RNP_FLAG2_BRIDGE_MODE_VEB;
	dbg("%s:%d flags:0x%x\n", __func__, __LINE__, adapter->flags);

	hw->ops.set_sriov_status(hw, true);

	adapter->vfinfo = kcalloc(adapter->num_vfs,
				  sizeof(struct vf_data_storage), GFP_KERNEL);
	if (adapter->vfinfo) {
		/* limit trafffic classes based on VFs enabled */
		/* TODO analyze VF need support pfc or traffic classes */
		/* We do not support RSS w/ SR-IOV */
		adapter->ring_feature[RING_F_RSS].limit = hw->sriov_ring_limit;

		/* Disable RSC when in SR-IOV mode */
		adapter->flags2 &=
			~(RNP_FLAG2_RSC_CAPABLE | RNP_FLAG2_RSC_ENABLED);

		adapter->flags |= RNP_FLAG_SRIOV_ENABLED;
		/* force close eee if open sriov */
		adapter->eee_enabled = 0;
		return 0;
	}

	/* open flags at last to avoid null call adapter->vfinfo */
	dbg("%s:%d flags:0x%x\n", __func__, __LINE__, adapter->flags);
	return -ENOMEM;
}

void rnpgbe_enable_sriov_true(struct rnpgbe_adapter *adapter)
{
	int err = 0;

	if (!(adapter->flags & RNP_FLAG_SRIOV_ENABLED))
		return;

	adapter->flags |= RNP_FLAG_SRIOV_INIT_DONE;

	err = pci_enable_sriov(adapter->pdev, adapter->num_vfs);
	if (err) {
		printk(KERN_DEBUG "Failed to enable PCI sriov: %d num %d\n",
		       err, adapter->num_vfs);
		printk(KERN_DEBUG "We cannot handle this error\n");
	}

	adapter->flags |= RNP_FLAG_VF_INIT_DONE;
}

/* Note this function is called when the user wants to enable SR-IOV
 * VFs using the now deprecated module parameter
 * never used
 */
void rnpgbe_enable_sriov(struct rnpgbe_adapter *adapter)
{
	int pre_existing_vfs = 0;
	struct rnpgbe_hw *hw = &adapter->hw;

	pre_existing_vfs = pci_num_vf(adapter->pdev);
	if (!pre_existing_vfs && !adapter->num_vfs)
		return;

	dbg("%s:%d flags:0x%x\n", __func__, __LINE__, adapter->flags);
	if (!pre_existing_vfs)
		dev_warn(&adapter->pdev->dev,
			 "Enabling SR-IOV VFs using the module parameter is deprecated"
			 "- please use the pci sysfs interface.\n");

	dbg("%s:%d flags:0x%x\n", __func__, __LINE__, adapter->flags);
	/* If there are pre-existing VFs then we have to force
	 * use of that many - over ride any module parameter value.
	 * This may result from the user unloading the PF driver
	 * while VFs were assigned to guest VMs or because the VFs
	 * have been created via the new PCI SR-IOV sysfs interface.
	 */
	if (pre_existing_vfs) {
		adapter->num_vfs = pre_existing_vfs;
		dev_warn(&adapter->pdev->dev,
			 "Virtual Functions already enabled for this device - Please"
			 "reload all VF drivers to avoid spoofed packet errors\n");
	} else {
		int i;
		/*
		 * The n10 supports up to 64 VFs per physical function
		 * but this implementation limits allocation to 126 so that
		 * basic networking resources are still available to the
		 * physical function.  If the user requests greater than
		 * 64 VFs then it is an error - reset to default of zero.
		 */
		adapter->num_vfs =
			min_t(unsigned int, adapter->num_vfs, hw->max_vfs - 1);

		if (__rnpgbe_enable_sriov(adapter)) {
			e_err(probe, "Failed to alloc memory for sriov\n");
			adapter->num_vfs = 0;
		}

		for (i = 0; i < adapter->num_vfs; i++)
			rnpgbe_vf_configuration(adapter->pdev,
						(i | 0x10000000));

		dbg("%s:%d flags:0x%x\n", __func__, __LINE__, adapter->flags);
	}
}

static bool rnpgbe_vfs_are_assigned(struct rnpgbe_adapter *adapter)
{
	struct pci_dev *pdev = adapter->pdev;
	struct pci_dev *vfdev;
	unsigned int dev_id = RNP_DEV_ID_N10_PF0_VF_N;
	unsigned int vendor_id = PCI_VENDOR_ID_MUCSE;

	switch (adapter->pdev->device) {
	case RNP_DEV_ID_N10_PF0:
	case RNP_DEV_ID_N10_PF1:
		vendor_id = 0x1dab;
		if (rnpgbe_is_pf1(pdev))
			dev_id = RNP_DEV_ID_N10_PF1_VF;
		else
			dev_id = RNP_DEV_ID_N10_PF0_VF;
		break;
	case PCI_DEVICE_ID_N10_PF0:
	case PCI_DEVICE_ID_N10_PF1:
		vendor_id = PCI_VENDOR_ID_MUCSE;
		if (rnpgbe_is_pf1(pdev))
			dev_id = RNP_DEV_ID_N10_PF1_VF_N;
		else
			dev_id = RNP_DEV_ID_N10_PF0_VF_N;
	}

	/* loop through all the VFs to see if we own any that are assigned */
	vfdev = pci_get_device(vendor_id, dev_id, NULL);
	while (vfdev) {
		/* if we don't own it we don't care */
		if (vfdev->is_virtfn && vfdev->physfn == pdev) {
			/* if it is assigned we cannot release it */
			if (vfdev->dev_flags & PCI_DEV_FLAGS_ASSIGNED)
				return true;
		}

		vfdev = pci_get_device(vendor_id, dev_id, vfdev);
	}

	return false;
}
#endif /* #ifdef CONFIG_PCI_IOV */

int rnpgbe_disable_sriov(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	int rss;
	int time = 0;

	if (!(adapter->flags & RNP_FLAG_SRIOV_ENABLED))
		return 0;

	adapter->num_vfs = 0;
	adapter->flags &= ~RNP_FLAG_SRIOV_ENABLED;
	adapter->flags &= ~RNP_FLAG_SRIOV_INIT_DONE;
	adapter->flags &= ~RNP_FLAG_VF_INIT_DONE;
	adapter->vlan_count = 0;
	msleep(100);
	hw->ops.set_mac_rx(hw, false);
	hw->ops.set_sriov_status(hw, false);

	/* set num VFs to 0 to prevent access to vfinfo */
	while (test_and_set_bit(__RNP_USE_VFINFI, &adapter->state)) {
		msleep(100);
		time++;

		if (time > 100) {
			printk(KERN_DEBUG "wait flags timeout\n");
			break;
		}
	}
	if (time < 100)
		clear_bit(__RNP_USE_VFINFI, &adapter->state);

	/* free VF control structures */
	kfree(adapter->vfinfo);
	adapter->vfinfo = NULL;

	/* free macvlan list */
	if (hw->vv_list) {
		kfree(hw->vv_list);
		hw->vv_list = NULL;
	}

	if (adapter->mv_list) {
		kfree(adapter->mv_list);
		adapter->mv_list = NULL;
	}

	dbg("%s:%d flags:0x%x\n", __func__, __LINE__, adapter->flags);
#ifdef CONFIG_PCI_IOV
	/*
	 * If our VFs are assigned we cannot shut down SR-IOV
	 * without causing issues, so just leave the hardware
	 * available but disabled
	 */
	if (rnpgbe_vfs_are_assigned(adapter)) {
		e_dev_warn("Unloading driver while VFs are assigned"
			   "- VFs will not be "
			   "deallocated\n");
		return -EPERM;
	}
	/* disable iov and allow time for transactions to clear */
	pci_disable_sriov(adapter->pdev);
#endif
	dbg("%s:%d flags:0x%x\n", __func__, __LINE__, adapter->flags);

	/* set default pool back to 0 */

	/* Disable VMDq flag so device will be set in VM mode */
	if (adapter->ring_feature[RING_F_VMDQ].limit == 1)
		adapter->flags &= ~RNP_FLAG_VMDQ_ENABLED;
	adapter->ring_feature[RING_F_VMDQ].offset = 0;

	rss = min_t(int, adapter->max_ring_pair_counts, num_online_cpus());

	rss = min_t(int, rss,
		    hw->mac.max_msix_vectors - adapter->num_other_vectors);

	adapter->ring_feature[RING_F_RSS].limit = rss;

	/* take a breather then clean up driver data */
	msleep(100);

	dbg("%s:%d flags:0x%x\n", __func__, __LINE__, adapter->flags);
	return 0;
}

bool check_ari_mode(struct pci_dev *dev)
{
	struct pci_bus *bus = dev->bus;

	return bus->self && bus->self->ari_enabled;
}

static int rnpgbe_pci_sriov_enable(struct pci_dev *dev, int num_vfs)
{
#ifdef CONFIG_PCI_IOV
	struct rnpgbe_adapter *adapter = pci_get_drvdata(dev);
	struct rnpgbe_hw *hw = &adapter->hw;
	int err = 0;
	int i;
	int pre_existing_vfs = pci_num_vf(dev);

	if (pre_existing_vfs && pre_existing_vfs != num_vfs)
		err = rnpgbe_disable_sriov(adapter);
	else if (pre_existing_vfs && pre_existing_vfs == num_vfs)
		goto out;

	if (hw->feature_flags & RNP_VEB_VLAN_MASK_EN) {
		if (adapter->vlan_count > hw->max_vfs - 1) {
			dev_err(&adapter->pdev->dev,
				"vlans is too much, delete less than %d vlans\n",
				hw->max_vfs - 1);

			err = -EOPNOTSUPP;
			goto err_out;
		}

	} else if (adapter->vlan_count > 1) {
		dev_err(&adapter->pdev->dev,
			"only 1 vlan in sriov mode, delete other vlans\n");
		dev_err(&adapter->pdev->dev, "please delete all vlans first\n");

		err = -EOPNOTSUPP;
		goto err_out;
	}

	adapter->vlan_count = 0;
	if (err)
		goto err_out;

	/* While the SR-IOV capability structure reports total VFs to be
	 * 64 we limit the actual number that can be allocated to 63 so
	 * that some transmit/receive resources can be reserved to the
	 * PF.  The PCI bus driver already checks for other values out of
	 * range.
	 */

	if (check_ari_mode(dev)) {
		if (num_vfs > (hw->max_vfs - 1)) {
			err = -EPERM;
			goto err_out;
		}
	} else {
		if (num_vfs > hw->max_vfs_noari) {
			err = -EPERM;
			goto err_out;
		}
	}

	/* maybe too early */
	adapter->num_vfs = num_vfs;

	err = __rnpgbe_enable_sriov(adapter);
	if (err)
		goto err_out;

	for (i = 0; i < adapter->num_vfs; i++)
		rnpgbe_vf_configuration(dev, (i | 0x10000000));
	dbg("flags:0x%x\n", adapter->flags);
	if (hw->ops.clr_rar_all)
		hw->ops.clr_rar_all(hw);

	rnpgbe_sriov_reinit(adapter);

	adapter->flags |= RNP_FLAG_SRIOV_INIT_DONE;
	err = pci_enable_sriov(dev, num_vfs);
	if (err) {
		e_dev_warn("Failed to enable PCI sriov: %d num %d\n", err,
			   num_vfs);
		rnpgbe_disable_sriov(adapter);
		rnpgbe_sriov_reinit(adapter);
		goto err_out;
	}
	adapter->flags |= RNP_FLAG_VF_INIT_DONE;

out:
	return num_vfs;

err_out:
	return err;
#endif
	return 0;
}

static int rnpgbe_pci_sriov_disable(struct pci_dev *dev)
{
	struct rnpgbe_adapter *adapter = pci_get_drvdata(dev);
	int err;
	u32 current_flags = adapter->flags;

	err = rnpgbe_disable_sriov(adapter);

	/* Only reinit if no error and state changed */
	if (!err && current_flags != adapter->flags) {
		/* rnpgbe_disable_sriov() doesn't clear VMDQ flag */
		adapter->flags &= ~RNP_FLAG_VMDQ_ENABLED;
#ifdef CONFIG_PCI_IOV
		rnpgbe_sriov_reinit(adapter);
#endif
	}

	return err;
}

static int rnpgbe_set_vf_multicasts(struct rnpgbe_adapter *adapter,
				    u32 *msgbuf,
				    u32 vf)
{
	int entries = (msgbuf[0] & RNP_VT_MSGINFO_MASK) >> RNP_VT_MSGINFO_SHIFT;
	u16 *hash_list = (u16 *)&msgbuf[1];
	struct vf_data_storage *vfinfo = &adapter->vfinfo[vf];
	struct rnpgbe_hw *hw = &adapter->hw;
	int i;

	/* only so many hash values supported */
	entries = min(entries, RNP_MAX_VF_MC_ENTRIES);

	/*
	 * salt away the number of multi cast addresses assigned
	 * to this VF for later use to restore when the PF multi cast
	 * list changes
	 */
	vfinfo->num_vf_mc_hashes = entries;

	/*
	 * VFs are limited to using the MTA hash table for their multicast
	 * addresses
	 */
	for (i = 0; i < entries; i++)
		vfinfo->vf_mc_hashes[i] = hash_list[i];

	for (i = 0; i < vfinfo->num_vf_mc_hashes; i++)
		hw->ops.set_sriov_vf_mc(hw, vfinfo->vf_mc_hashes[i]);

	return 0;
}

void rnpgbe_restore_vf_macs(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	int vf;
	u8 *mac_addr;
	int rar_entry;

	for (vf = 0; vf < adapter->num_vfs; vf++) {
		mac_addr = adapter->vfinfo[vf].vf_mac_addresses;
		rar_entry = hw->mac.num_rar_entries - (vf + 1);
		/* setup to the hw */
		if (hw->feature_flags & RNP_NET_FEATURE_VF_FIXED)
			hw->ops.set_rar_with_vf(hw, mac_addr, rar_entry, vf + 1,
						true);
		else
			hw->ops.set_rar_with_vf(hw, mac_addr, rar_entry, vf,
						true);
	}
}

void rnpgbe_restore_vf_macvlans(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	struct list_head *pos;
	struct vf_macvlans *entry;

	hw_dbg(hw, "%s Staring..\n", __func__);

	list_for_each(pos, &adapter->vf_mvs.l) {
		entry = list_entry(pos, struct vf_macvlans, l);
		if (!entry->free) {
			hw_dbg(hw, "  vf:%d MACVLAN: RAR[%d] <= %pM\n",
			       entry->vf, entry->rar_entry, entry->vf_macvlan);

			if (hw->feature_flags & RNP_NET_FEATURE_VF_FIXED) {
				hw->ops.set_rar_with_vf(hw, entry->vf_macvlan,
							entry->rar_entry,
							entry->vf + 1, true);
			} else {
				hw->ops.set_rar_with_vf(hw, entry->vf_macvlan,
							entry->rar_entry,
							entry->vf, true);
			}
		}
	}
	hw_dbg(hw, "%s Done\n", __func__);
}

void rnpgbe_restore_vf_multicasts(struct rnpgbe_adapter *adapter)
{
	/* Restore any VF macvlans */
	rnpgbe_restore_vf_macvlans(adapter);
}

static int rnpgbe_set_vf_vlan(struct rnpgbe_adapter *adapter, int add, int vid,
			      u32 vf)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	int true_handle = 1;
	int i;
	/* VLAN 0 is a special case, don't allow it to be removed */
	if (!vid && !add)
		return 0;

	/* should check other vf */
	if ((adapter->flags & RNP_FLAG_SRIOV_ENABLED)) {
		/* if other vf use this vlan, don't true remove */
		if (!add) {
			/* check equal pf_vlan? */
			if (vid == adapter->vf_vlan)
				true_handle = 0;
			if (!test_and_set_bit(__RNP_USE_VFINFI,
					      &adapter->state)) {
				for (i = 0; i < adapter->num_vfs; i++) {
					/* check if other vf_vlan still valid */
					if ((i != vf) &&
					    (vid == adapter->vfinfo[i].vf_vlan))
						true_handle = 0;
					/* check if other pf_vlan still valid */
					if ((i != vf) &&
					    (vid == adapter->vfinfo[i].pf_vlan))
						true_handle = 0;
				}
				clear_bit(__RNP_USE_VFINFI, &adapter->state);
			}
		}
	}
	if (true_handle)
		hw->ops.set_vf_vlan_filter(hw, vid, vf, (bool)add, false);

	if (adapter->priv_flags & RNP_PRIV_FLAG_SRIOV_VLAN_MODE) {
		if (hw->ops.set_vf_vlan_mode) {
			if (hw->feature_flags & RNP_NET_FEATURE_VF_FIXED)
				hw->ops.set_vf_vlan_mode(hw, vid, vf + 1,
							 (bool)add);
			else
				hw->ops.set_vf_vlan_mode(hw, vid, vf,
							 (bool)add);
		}
	}

	return 0;
}

static inline void rnpgbe_vf_reset_event(struct rnpgbe_adapter *adapter, u32 vf)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	int rar_entry = hw->mac.num_rar_entries - (vf + 1);
	int i;

	/* reset multicast table array for vf */
	adapter->vfinfo[vf].num_vf_mc_hashes = 0;

	/* Flush and reset the mta with the new values */
	rnpgbe_set_rx_mode(adapter->netdev);

	/* clear this rar_entry */
	hw->ops.clr_rar(hw, rar_entry);

	/* reset VF api back to unknown */
	adapter->vfinfo[vf].vf_api = 0;
	/* clear vf multicast */
	for (i = 0; i < RNP_MAX_VF_MC_ENTRIES; i++)
		adapter->vfinfo[vf].vf_mc_hashes[i] = 0;
	/* clear vf vlan setup */
	adapter->vfinfo[vf].vf_vlan = 0;
	adapter->vfinfo[vf].vlan_count = 0;
}

static int rnpgbe_set_vf_mac(struct rnpgbe_adapter *adapter, int vf,
			     unsigned char *mac_addr)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	/* this rar_entry may be cofict with mac vlan with pf */
	int rar_entry = hw->mac.num_rar_entries - (vf + 1);

	memcpy(adapter->vfinfo[vf].vf_mac_addresses, mac_addr, 6);

	/* setup to the hw */
	if (hw->feature_flags & RNP_NET_FEATURE_VF_FIXED)
		hw->ops.set_rar_with_vf(hw, mac_addr, rar_entry, vf + 1, true);
	else
		hw->ops.set_rar_with_vf(hw, mac_addr, rar_entry, vf, true);

	return 0;
}

static int rnpgbe_set_vf_macvlan(struct rnpgbe_adapter *adapter, int vf,
				 int index, unsigned char *mac_addr)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	struct list_head *pos;
	struct vf_macvlans *entry;
	/* index = 0 , only earase */
	/* index = 1 , earase and then set */
	if (index <= 1) {
		list_for_each (pos, &adapter->vf_mvs.l) {
			entry = list_entry(pos, struct vf_macvlans, l);
			if (entry->vf == vf) {
				entry->vf = -1;
				entry->free = true;
				entry->is_macvlan = false;
				hw->ops.clr_rar(hw, entry->rar_entry);
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

	list_for_each (pos, &adapter->vf_mvs.l) {
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
	 * memory for the list management required.
	 */
	if (!entry || !entry->free)
		return -ENOSPC;

	entry->free = false;
	entry->is_macvlan = true;
	entry->vf = vf;
	memcpy(entry->vf_macvlan, mac_addr, ETH_ALEN);

	if (hw->feature_flags & RNP_NET_FEATURE_VF_FIXED) {
		hw->ops.set_rar_with_vf(hw, entry->vf_macvlan, entry->rar_entry,
					entry->vf + 1, true);
	} else {
		hw->ops.set_rar_with_vf(hw, entry->vf_macvlan, entry->rar_entry,
					entry->vf, true);
	}
	return 0;
}

int rnpgbe_vf_configuration(struct pci_dev *pdev, unsigned int event_mask)
{
	unsigned char vf_mac_addr[6];
	struct rnpgbe_adapter *adapter = pci_get_drvdata(pdev);
	unsigned int vfn = (event_mask & 0x3f);

	bool enable = ((event_mask & 0x10000000U) != 0);

	if (enable) {
		eth_zero_addr(vf_mac_addr);
		memcpy(vf_mac_addr, adapter->hw.mac.perm_addr, 6);
		vf_mac_addr[5] = vf_mac_addr[5] + (0x80 | vfn);
		vf_mac_addr[4] = vf_mac_addr[4] + (pdev->devfn);

		memcpy(adapter->vfinfo[vfn].vf_mac_addresses, vf_mac_addr, 6);
	}

	return 0;
}

static int rnpgbe_vf_reset_msg(struct rnpgbe_adapter *adapter, u32 vf)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	unsigned char *vf_mac = adapter->vfinfo[vf].vf_mac_addresses;
	u32 msgbuf[RNP_VF_PERMADDR_MSG_LEN];
	u8 *addr = (u8 *)(&msgbuf[1]);

	/* reset the filters for the device */
	rnpgbe_vf_reset_event(adapter, vf);

	/* set vf mac address */
	if (!is_zero_ether_addr(vf_mac))
		rnpgbe_set_vf_mac(adapter, vf, vf_mac);

	/* enable VF mailbox for further messages */
	adapter->vfinfo[vf].clear_to_send = true;

	/* Enable counting of spoofed packets in the SSVPC register */

	/* reply to reset with ack and vf mac address */
	msgbuf[0] = RNP_VF_RESET;
	if (!is_zero_ether_addr(vf_mac)) {
		msgbuf[0] |= RNP_VT_MSGTYPE_ACK;
		memcpy(addr, vf_mac, ETH_ALEN);
	} else {
		msgbuf[0] |= RNP_VT_MSGTYPE_NACK;
		dev_warn(&adapter->pdev->dev,
			 "VF %d has no MAC address assigned, you have to assign"
			 "one manually\n",
			 vf);
	}

	/*
	 * Piggyback the multicast filter type so VF can compute the
	 * correct vectors
	 */
	msgbuf[RNP_VF_MC_TYPE_WORD] = 0;
	/* setup link status , pause mode, ft padding mode */

	/* link status */
	/* pause mode */
	msgbuf[RNP_VF_MC_TYPE_WORD] |= (0xff & hw->fc.current_mode) << 16;
	if (adapter->priv_flags & RNP_PRIV_FLAG_FT_PADDING)
		msgbuf[RNP_VF_MC_TYPE_WORD] |= (0x01 << 8);
	else
		msgbuf[RNP_VF_MC_TYPE_WORD] |= (0x00 << 8);
	/* mc_type */
	msgbuf[RNP_VF_MC_TYPE_WORD] |= rd32(hw, RNP_ETH_DMAC_MCSTCTRL) & 0x03;
	msgbuf[RNP_VF_DMA_VERSION_WORD] = rd32(hw, RNP_DMA_VERSION);
	msgbuf[RNP_VF_VLAN_WORD] = adapter->vfinfo[vf].pf_vlan;
	msgbuf[RNP_VF_PHY_TYPE_WORD] = (hw->mac_type << 16) | hw->phy_type;
	msgbuf[RNP_VF_FW_VERSION_WORD] = (hw->fw_version);
	if (adapter->vfinfo[vf].link_state == rnpgbe_link_state_auto) {
		msgbuf[RNP_VF_LINK_STATUS_WORD] =
			(adapter->link_up ? RNP_PF_LINK_UP : 0) |
			adapter->link_speed;
	} else if (adapter->vfinfo[vf].link_state == rnpgbe_link_state_on) {
		msgbuf[RNP_VF_LINK_STATUS_WORD] =
			RNP_PF_LINK_UP | adapter->link_speed;

	} else {
		msgbuf[RNP_VF_LINK_STATUS_WORD] = 0;
	}

	msgbuf[RNP_VF_AXI_MHZ] = hw->usecstocount;
	msgbuf[RNP_VF_FEATURE] = 0;
	if (adapter->netdev->features & NETIF_F_HW_VLAN_CTAG_FILTER)
		msgbuf[RNP_VF_FEATURE] |= PF_FEATRURE_VLAN_FILTER;
	if (hw->ncsi_en)
		msgbuf[RNP_VF_FEATURE] |= PF_NCSI_EN;

	/* now vf maybe has no irq handler if it is the first reset*/
	rnpgbe_write_mbx(hw, msgbuf, RNP_VF_PERMADDR_MSG_LEN, vf);

	return 0;
}

static int rnpgbe_get_vf_mac_addr(struct rnpgbe_adapter *adapter, u32 *msgbuf,
				  u32 vf)
{
	u8 *mac = ((u8 *)(&msgbuf[1]));

	memcpy(mac, adapter->vfinfo[vf].vf_mac_addresses, 6);

	return 0;
}

/* vf call setup a new mac */
static int rnpgbe_set_vf_mac_addr(struct rnpgbe_adapter *adapter, u32 *msgbuf,
				  u32 vf)
{
	u8 *new_mac = ((u8 *)(&msgbuf[1]));

	if (!is_valid_ether_addr(new_mac)) {
		e_warn(drv, "VF %d attempted to set invalid mac\n", vf);
		return -1;
	}

	if (adapter->vfinfo[vf].pf_set_mac &&
	    memcmp(adapter->vfinfo[vf].vf_mac_addresses, new_mac, ETH_ALEN)) {
		e_warn(drv,
		       "VF %d attempted to override administratively set MAC address\n"
		       "Reload the VF driver to resume operations\n",
		       vf);
		return -1;
	}
	rnpgbe_set_vf_mac(adapter, vf, new_mac);

	return 0;
}

static int rnpgbe_set_vf_vlan_msg(struct rnpgbe_adapter *adapter, u32 *msgbuf,
				  u32 vf)
{
	int add = ((msgbuf[0] & RNP_VT_MSGINFO_MASK) >> RNP_VT_MSGINFO_SHIFT);
	int vid = (msgbuf[1] & RNP_VLVF_VLANID_MASK);
	int err;

	if (adapter->vfinfo[vf].pf_vlan) {
		e_warn(drv,
		       "VF %d attempted to override administratively set VLAN "
		       "configuration\n"
		       "Reload the VF driver to resume operations\n",
		       vf);
		return -1;
	}

	if ((add) && (adapter->vfinfo[vf].vlan_count)) {
		e_warn(drv, "VF %d attempted to set more than 1 vlan", vf);
		e_warn(drv, " vlan now %d, try to set %d\n",
		       adapter->vfinfo[vf].vf_vlan, vid);
		return -1;
	}

	/* vlan 0 has no work todo */
	if (!vid)
		return 0;
	if (add) {
		adapter->vfinfo[vf].vlan_count++;
		/* store vf vlan setup */
		adapter->vfinfo[vf].vf_vlan = vid;
	} else if (adapter->vfinfo[vf].vlan_count) {
		adapter->vfinfo[vf].vf_vlan = 0;
		adapter->vfinfo[vf].vlan_count--;
	}

	err = rnpgbe_set_vf_vlan(adapter, add, vid, vf);

	return err;
}

static int rnpgbe_set_vf_vlan_strip_msg(struct rnpgbe_adapter *adapter,
					u32 *msgbuf, u32 vf)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	int vlan_strip_on = !!(msgbuf[1] >> 31);
	int queue_cnt = msgbuf[1] & 0xffff;
	int err = 0, i;

	vf_dbg("strip_on:%d queeu_cnt:%d, %d %d\n", vlan_strip_on, queue_cnt,
	       msgbuf[2], msgbuf[3]);

	for (i = 0; i < queue_cnt; i++) {
		if (vlan_strip_on)
			hw->ops.set_vlan_strip(hw, msgbuf[2 + i], true);
		else
			hw->ops.set_vlan_strip(hw, msgbuf[2 + i], false);
	}

	return err;
}

static int rnpgbe_set_vf_macvlan_msg(struct rnpgbe_adapter *adapter,
				     u32 *msgbuf, u32 vf)
{
	u8 *new_mac = ((u8 *)(&msgbuf[1]));
	int index = (msgbuf[0] & RNP_VT_MSGINFO_MASK) >> RNP_VT_MSGINFO_SHIFT;
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
	}

	err = rnpgbe_set_vf_macvlan(adapter, vf, index, new_mac);
	if (err == -ENOSPC)
		e_warn(drv, "VF %d has requested a MACVLAN filter but "
			    "there is no space for it\n", vf);

	return err < 0;
}

static int rnpgbe_negotiate_vf_api(struct rnpgbe_adapter *adapter, u32 *msgbuf,
				   u32 vf)
{
	adapter->vfinfo[vf].vf_api = 0;

	return 0;
}

static int rnpgbe_get_vf_reg(struct rnpgbe_adapter *adapter, u32 *msgbuf,
			     u32 vf)
{
	u32 reg = msgbuf[1];

	msgbuf[1] = rd32(&adapter->hw, reg);

	return 0;
}

static int rnpgbe_set_vf_mtu(struct rnpgbe_adapter *adapter, u32 *msgbuf,
			     u32 vf)
{
	struct net_device *netdev = adapter->netdev;

	if (msgbuf[1] > netdev->mtu) {
		e_dev_warn("vf %d try to change %d mtu to %d (too large)\n",
			   vf, netdev->mtu, msgbuf[1]);
		return -1;
	} else {
		return 0;
	}
}

static int rnpgbe_get_vf_mtu(struct rnpgbe_adapter *adapter, u32 *msgbuf,
			     u32 vf)
{
	struct net_device *netdev = adapter->netdev;

	msgbuf[1] = netdev->mtu;

	return 0;
}

static int rnpgbe_get_vf_fw(struct rnpgbe_adapter *adapter, u32 *msgbuf, u32 vf)
{
	struct rnpgbe_hw *hw = &adapter->hw;

	msgbuf[1] = hw->fw_version;

	return 0;
}

static int rnpgbe_get_vf_link(struct rnpgbe_adapter *adapter, u32 *msgbuf,
			      u32 vf)
{
	if (adapter->vfinfo[vf].link_state == rnpgbe_link_state_auto) {
		msgbuf[1] = (adapter->link_up ? RNP_PF_LINK_UP : 0) |
			    adapter->link_speed;
	} else if (adapter->vfinfo[vf].link_state == rnpgbe_link_state_on) {
		msgbuf[1] = RNP_PF_LINK_UP | adapter->link_speed;

	} else {
		msgbuf[1] = 0;
	}

	return 0;
}

static int rnpgbe_get_vf_dma_frag(struct rnpgbe_adapter *adapter, u32 *msgbuf,
				  u32 vf)
{
	/* we fixed 1536 bytes */
	msgbuf[1] = 1536;
	return 0;
}

static int rnpgbe_vf_get_stats_clr(struct rnpgbe_adapter *adapter, u32 *msgbuf,
				   u32 vf)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	struct rnpgbe_dma_info *dma = &hw->dma;

	if (dma_rd32(dma, RNP500_STATISTIC_CRL(vf)))
		msgbuf[1] = 1;
	else
		msgbuf[1] = 0;

	return 0;
}

static int rnpgbe_vf_set_stats_clr(struct rnpgbe_adapter *adapter, u32 *msgbuf,
				   u32 vf)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	struct rnpgbe_dma_info *dma = &hw->dma;

	if (msgbuf[1])
		dma_wr32(dma, RNP500_STATISTIC_CRL(vf), 1);
	else
		dma_wr32(dma, RNP500_STATISTIC_CRL(vf), 0);

	return 0;
}

static int rnpgbe_get_vf_queues(struct rnpgbe_adapter *adapter, u32 *msgbuf,
				u32 vf)
{
	struct rnpgbe_hw *hw = &adapter->hw;

	msgbuf[RNP_VF_TX_QUEUES] = hw->sriov_ring_limit;
	msgbuf[RNP_VF_RX_QUEUES] = hw->sriov_ring_limit;
	msgbuf[RNP_VF_TRANS_VLAN] = adapter->vfinfo[vf].pf_vlan;
	msgbuf[RNP_VF_DEF_QUEUE] = 0;
	if (hw->hw_type == rnpgbe_hw_n400) {
		/* n400, we use */
		/* vf0 use ring4 */
		/* vf1 use ring8 */
		msgbuf[RNP_VF_QUEUE_START] = vf * 4 + 4;

	} else {
		if (hw->feature_flags & RNP_NET_FEATURE_VF_FIXED)
			msgbuf[RNP_VF_QUEUE_START] = vf * hw->sriov_ring_limit +
						     hw->sriov_ring_limit;
		else
			msgbuf[RNP_VF_QUEUE_START] = vf * hw->sriov_ring_limit;
	}
	msgbuf[RNP_VF_QUEUE_DEPTH] = (adapter->tx_ring_item_count << 16) |
				     adapter->rx_ring_item_count;

	return 0;
}

static int rnpgbe_rcv_msg_from_vf(struct rnpgbe_adapter *adapter, u32 vf)
{
	u32 mbx_size = RNP_VFMAILBOX_SIZE;
	u32 msgbuf[RNP_VFMAILBOX_SIZE];
	struct rnpgbe_hw *hw = &adapter->hw;
	s32 retval;

	vf_dbg("msg from vf:%d\n", vf);

	retval = rnpgbe_read_mbx(hw, msgbuf, mbx_size, vf);
	if (retval) {
		pr_err("Error receiving message from VF\n");
		return retval;
	}
	vf_dbg("msg[0]=0x%08x\n", msgbuf[0]);

	/* this is a message we already processed, do nothing */
	if (msgbuf[0] & (RNP_VT_MSGTYPE_ACK | RNP_VT_MSGTYPE_NACK))
		return retval;

	/* flush the ack before we write any messages back */

	/* clear vf_num */
	msgbuf[0] &= (~RNP_VF_MASK);

	/* this is a vf reset irq */
	if ((msgbuf[0] & RNP_MAIL_CMD_MASK) == RNP_VF_RESET) {
		vf_dbg("vf %d up\n", vf);
		return rnpgbe_vf_reset_msg(adapter, vf);
	}

	/*
	 * until the vf completes a virtual function reset it should not be
	 * allowed to start any configuration.
	 */
	if (!adapter->vfinfo[vf].clear_to_send) {
		vf_dbg("wait vf clear to send\n");
		msgbuf[0] |= RNP_VT_MSGTYPE_NACK;
		rnpgbe_write_mbx(hw, msgbuf, 1, vf);
		return retval;
	}

	switch ((msgbuf[0] & RNP_MAIL_CMD_MASK)) {
	case RNP_VF_SET_MAC_ADDR:
		retval = rnpgbe_set_vf_mac_addr(adapter, msgbuf, vf);
		break;
	case RNP_VF_SET_MULTICAST:
		retval = rnpgbe_set_vf_multicasts(adapter, msgbuf, vf);
		break;
	case RNP_VF_SET_VLAN:
		retval = rnpgbe_set_vf_vlan_msg(adapter, msgbuf, vf);
		break;
	case RNP_VF_SET_VLAN_STRIP:
		retval = rnpgbe_set_vf_vlan_strip_msg(adapter, msgbuf, vf);
		break;
	case RNP_VF_GET_MACADDR:
		retval = rnpgbe_get_vf_mac_addr(adapter, msgbuf, vf);
		break;
	case RNP_VF_SET_MACVLAN:
		retval = rnpgbe_set_vf_macvlan_msg(adapter, msgbuf, vf);
		break;
	case RNP_VF_API_NEGOTIATE:
		retval = rnpgbe_negotiate_vf_api(adapter, msgbuf, vf);
		break;
	case RNP_VF_GET_QUEUES:
		retval = rnpgbe_get_vf_queues(adapter, msgbuf, vf);
		break;
	case RNP_VF_REG_RD:
		retval = rnpgbe_get_vf_reg(adapter, msgbuf, vf);
		break;
	case RNP_VF_GET_MTU:
		retval = rnpgbe_get_vf_mtu(adapter, msgbuf, vf);
		break;
	case RNP_VF_SET_MTU:
		retval = rnpgbe_set_vf_mtu(adapter, msgbuf, vf);
		break;
	case RNP_VF_GET_FW:
		retval = rnpgbe_get_vf_fw(adapter, msgbuf, vf);
		break;
	case RNP_VF_GET_LINK:
		retval = rnpgbe_get_vf_link(adapter, msgbuf, vf);
		break;
	case RNP_PF_REMOVE:
		vf_dbg("vf %d removed\n", vf);
		adapter->vfinfo[vf].clear_to_send = false;
		adapter->vfinfo[vf].vf_vlan = 0;
		/* todo clean vf info */
		retval = 1;
		break;
	case RNP_VF_RESET_PF:
		adapter->flags2 |= RNP_FLAG2_RESET_PF;
		retval = 1;
		break;
	case RNP_VF_GET_DMA_FRAG:
		retval = rnpgbe_get_vf_dma_frag(adapter, msgbuf, vf);

		break;
	case RNP_VF_SET_STATS_CLR:
		retval = rnpgbe_vf_set_stats_clr(adapter, msgbuf, vf);
		break;
	case RNP_VF_GET_STATS_CLR:
		retval = rnpgbe_vf_get_stats_clr(adapter, msgbuf, vf);
		break;
	default:
		e_err(drv, "Unhandled Msg %8.8x\n", msgbuf[0]);
		retval = RNP_ERR_MBX;
		break;
	}

	/* notify the VF of the results of what it sent us */
	if (retval)
		msgbuf[0] |= RNP_VT_MSGTYPE_NACK;
	else
		msgbuf[0] |= RNP_VT_MSGTYPE_ACK;

	/* write vf_num */
	msgbuf[0] |= (vf << 21);

	msgbuf[0] |= RNP_VT_MSGTYPE_CTS;

	if ((msgbuf[0] & RNP_MAIL_CMD_MASK) != RNP_PF_REMOVE)
		rnpgbe_write_mbx(hw, msgbuf, mbx_size, vf);

	return retval;
}

static void rnpgbe_rcv_ack_from_vf(struct rnpgbe_adapter *adapter, u32 vf)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	u32 msg = RNP_VT_MSGTYPE_NACK;

	/* if device isn't clear to send it shouldn't be reading either */
	if (!adapter->vfinfo[vf].clear_to_send)
		rnpgbe_write_mbx(hw, &msg, 1, vf);
}

void rnpgbe_msg_task(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	u32 vf;

	rnpgbe_fw_msg_handler(adapter);

	if (!(adapter->flags & RNP_FLAG_SRIOV_INIT_DONE))
		return;
	for (vf = 0; vf < adapter->num_vfs; vf++) {
		if (test_and_set_bit(__VF_MBX_USED,
				     &adapter->vfinfo[vf].status)) {
			adapter->miss_time++;
			e_info(drv, "we missed some irqs %d\n", vf);
			continue;
		}

		/* process any messages pending */
		if (!rnpgbe_check_for_msg(hw, vf))
			rnpgbe_rcv_msg_from_vf(adapter, vf);

		/* process any acks */
		if (!rnpgbe_check_for_ack(hw, vf))
			rnpgbe_rcv_ack_from_vf(adapter, vf);
		clear_bit(__VF_MBX_USED, &adapter->vfinfo[vf].status);
	}
}

int rnpgbe_msg_post_status_signle_link(struct rnpgbe_adapter *adapter, int vf,
				       int link_state)
{
	u32 msgbuf[RNP_VFMAILBOX_SIZE];
	struct rnpgbe_hw *hw = &adapter->hw;
	struct rnpgbe_mbx_info *mbx = &hw->mbx;

	msgbuf[0] = RNP_PF_SET_LINK | (vf << RNP_VNUM_OFFSET);
	switch (link_state) {
	case rnpgbe_link_state_on:
		msgbuf[1] = RNP_PF_LINK_UP | adapter->link_speed;
		break;
	case rnpgbe_link_state_off:
		msgbuf[1] = 0;
		break;
	case rnpgbe_link_state_auto:
		if (adapter->link_up)
			msgbuf[1] = RNP_PF_LINK_UP | adapter->link_speed;
		else
			msgbuf[1] = 0;
		break;
	}
	return mbx->ops.write(hw, msgbuf, 2, vf);
}

int rnpgbe_msg_post_status_signle(struct rnpgbe_adapter *adapter,
				  enum PF_STATUS status, int vf)
{
	u32 msgbuf[RNP_VFMAILBOX_SIZE];
	struct rnpgbe_hw *hw = &adapter->hw;
	struct rnpgbe_mbx_info *mbx = &hw->mbx;

	switch (status) {
	case PF_FCS_STATUS:
		msgbuf[0] = RNP_PF_SET_FCS | (vf << RNP_VNUM_OFFSET);
		if (adapter->netdev->features & NETIF_F_RXFCS)
			msgbuf[1] = 1;
		else
			msgbuf[1] = 0;
		break;
	case PF_PAUSE_STATUS:
		msgbuf[0] = RNP_PF_SET_PAUSE | (vf << RNP_VNUM_OFFSET);
		msgbuf[1] = hw->fc.requested_mode;
		break;
	case PF_FT_PADDING_STATUS:
		msgbuf[0] = RNP_PF_SET_FT_PADDING | (vf << RNP_VNUM_OFFSET);
		if (adapter->priv_flags & RNP_PRIV_FLAG_FT_PADDING)
			msgbuf[1] = 1;
		else
			msgbuf[1] = 0;

		break;
	case PF_VLAN_FILTER_STATUS:
		msgbuf[0] = RNP_PF_SET_VLAN_FILTER | (vf << RNP_VNUM_OFFSET);
		if (adapter->netdev->features & NETIF_F_HW_VLAN_CTAG_FILTER)
			msgbuf[1] = 1;
		else
			msgbuf[1] = 0;
		break;
	case PF_SET_VLAN_STATUS:
		msgbuf[0] = RNP_PF_SET_VLAN | (vf << RNP_VNUM_OFFSET);

		msgbuf[1] = adapter->vfinfo[vf].pf_vlan;
		break;
	case PF_SET_LINK_STATUS:
		if (adapter->vfinfo[vf].link_state != rnpgbe_link_state_auto)
			return 0;
		/* only update link state if in auto mode */
		msgbuf[0] = RNP_PF_SET_LINK | (vf << RNP_VNUM_OFFSET);
		if (adapter->link_up)
			msgbuf[1] = RNP_PF_LINK_UP | adapter->link_speed;
		else
			msgbuf[1] = 0;
		break;
	case PF_SET_MTU:
		msgbuf[0] = RNP_PF_SET_MTU | (vf << RNP_VNUM_OFFSET);
		msgbuf[1] = adapter->netdev->mtu;
		break;
	case PF_SET_RESET:
		msgbuf[0] = RNP_PF_SET_RESET | (vf << RNP_VNUM_OFFSET);
		msgbuf[1] = 0;

		break;
	}
	return mbx->ops.write_posted(hw, msgbuf, 2, vf);
}

/* try to send mailbox to all active vf */
int rnpgbe_msg_post_status(struct rnpgbe_adapter *adapter,
			   enum PF_STATUS status)
{
	u32 vf;
	int err = 0;

	if (!(adapter->flags & RNP_FLAG_SRIOV_ENABLED))
		return 0;
	/* broadcast */
	for (vf = 0; vf < adapter->num_vfs; vf++) {
		if (!(adapter->vfinfo[vf].clear_to_send))
			continue;

		if (!test_bit(__RNP_IN_IRQ, &adapter->state)) {
			if (test_and_set_bit(__VF_MBX_USED,
					     &adapter->vfinfo[vf].status)) {
				adapter->miss_time++;
				printk(KERN_DEBUG "send miss \n");
				return -1;
			}
			err |= rnpgbe_msg_post_status_signle(adapter, status,
							     vf);
			clear_bit(__VF_MBX_USED, &adapter->vfinfo[vf].status);
		}
	}
	return err;
}

void rnpgbe_ping_all_vfs(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	u32 ping;
	int i;

	for (i = 0; i < adapter->num_vfs; i++) {
		ping = RNP_PF_CONTROL_PRING_MSG;
		/* only send to active vf */
		ping |= RNP_VT_MSGTYPE_CTS;
		rnpgbe_write_mbx(hw, &ping, 1, i);
	}
}

int rnpgbe_get_vf_ringnum(struct rnpgbe_hw *hw, int vf, int num)
{
	return vf;
}

int rnpgbe_setup_ring_maxrate(struct rnpgbe_adapter *adapter, int ring,
			      u64 max_rate)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	struct rnpgbe_dma_info *dma = &hw->dma;
	int samples_1sec = adapter->hw.usecstocount * 100000;

	dma_ring_wr32(dma, RING_OFFSET(ring) + RNP_DMA_REG_TX_FLOW_CTRL_TM,
		      samples_1sec);
	dma_ring_wr32(dma, RING_OFFSET(ring) + RNP_DMA_REG_TX_FLOW_CTRL_TH,
		      max_rate);
	return 0;
}

static int rnpgbe_disable_port_vlan(struct rnpgbe_adapter *adapter, int vf)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	int err;

	err = rnpgbe_set_vf_vlan(adapter, false, adapter->vfinfo[vf].pf_vlan,
				 vf);

	if (adapter->priv_flags & RNP_PRIV_FLAG_SRIOV_VLAN_MODE) {
		if (hw->ops.set_vf_vlan_mode) {
			if (hw->feature_flags & RNP_NET_FEATURE_VF_FIXED)
				hw->ops.set_vf_vlan_mode(
					hw, adapter->vfinfo[vf].pf_vlan, vf + 1,
					false);
			else
				hw->ops.set_vf_vlan_mode(
					hw, adapter->vfinfo[vf].pf_vlan, vf,
					false);
		}
	}
	adapter->vfinfo[vf].pf_vlan = 0;
	adapter->vfinfo[vf].pf_qos = 0;
	/* clear veb */
	hw->ops.set_vf_vlan_filter(hw, 0, vf, false, true);

	return err;
}

static int rnpgbe_enable_port_vlan(struct rnpgbe_adapter *adapter, int vf,
				   u16 vlan, u8 qos)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	int err;

	err = rnpgbe_set_vf_vlan(adapter, true, vlan, vf);
	if (err)
		goto out;

	adapter->vfinfo[vf].pf_vlan = vlan;
	adapter->vfinfo[vf].pf_qos = qos;
	dev_info(pci_dev_to_dev(adapter->pdev),
		 "Setting VLAN %d, QOS 0x%x on VF %d\n", vlan, qos, vf);
	if (test_bit(__RNP_DOWN, &adapter->state)) {
		dev_warn(
			pci_dev_to_dev(adapter->pdev),
			"The VF VLAN has been set, but the PF device is not up.\n");
		dev_warn(
			pci_dev_to_dev(adapter->pdev),
			"Bring the PF device up before attempting to use the VF device.\n");
	}
	/* setup veb only */
	hw->ops.set_vf_vlan_filter(hw, vlan, vf, true, true);

	/* if in sriov vlan mode should setup pfvlvf table */
	if (adapter->priv_flags & RNP_PRIV_FLAG_SRIOV_VLAN_MODE) {
		if (hw->ops.set_vf_vlan_mode) {
			if (hw->feature_flags & RNP_NET_FEATURE_VF_FIXED)
				hw->ops.set_vf_vlan_mode(hw, vlan, vf + 1,
							 true);
			else
				hw->ops.set_vf_vlan_mode(hw, vlan, vf, true);
		}
	}
out:
	return err;
}

int rnpgbe_ndo_set_vf_vlan(struct net_device *netdev, int vf, u16 vlan, u8 qos,
			   __be16 vlan_proto)
{
	int err = 0;
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);

	/* VLAN IDs accepted range 0-4094 */
	if (vf < 0 || vf >= adapter->num_vfs || vlan > VLAN_VID_MASK - 1 ||
	    qos > 7)
		return -EINVAL;

	if (vlan_proto != htons(ETH_P_8021Q))
		return -EPROTONOSUPPORT;
	if (vlan || qos) {
		/*
		 * Check if there is already a port VLAN set, if so
		 * we have to delete the old one first before we
		 * can set the new one.  The usage model had
		 * previously assumed the user would delete the
		 * old port VLAN before setting a new one but this
		 * is not necessarily the case.
		 */
		if (adapter->vfinfo[vf].vf_vlan) {
			dev_err(&adapter->pdev->dev,
				"vf set vlan before, delete it before add new\n");
			err = -EINVAL;
			goto out;
		}
		if (adapter->vfinfo[vf].pf_vlan)
			err = rnpgbe_disable_port_vlan(adapter, vf);
		if (err)
			goto out;
		err = rnpgbe_enable_port_vlan(adapter, vf, vlan, qos);

	} else {
		/* if vf set vlan,  return error */
		if ((!adapter->vfinfo[vf].pf_vlan) &&
		    (adapter->vfinfo[vf].vf_vlan)) {
			dev_err(&adapter->pdev->dev,
				"pf cann't delete vf set vlan\n");

			err = -EINVAL;
			goto out;
		} else if (adapter->vfinfo[vf].pf_vlan)
			err = rnpgbe_disable_port_vlan(adapter, vf);
	}
	/* send mbx to vf */
	if (adapter->vfinfo[vf].clear_to_send)
		rnpgbe_msg_post_status_signle(adapter, PF_SET_VLAN_STATUS, vf);
out:
	return err;
}

#if IS_ENABLED(CONFIG_PCI_IOV)
int rnpgbe_ndo_set_vf_spoofchk(struct net_device *netdev, int vf, bool setting)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);

	if (vf < 0 || vf >= adapter->num_vfs)
		return -EINVAL;

	/* maybe we not support this in hw */
	adapter->vfinfo[vf].spoofchk_enabled = setting;

	return 0;
}

#endif /* CONFIG_PCI_IOV */

int rnpgbe_ndo_set_vf_trust(struct net_device *netdev, int vf, bool setting)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);

	if (vf < 0 || vf >= adapter->num_vfs)
		return -EINVAL;

	/* nothing to do */
	if (adapter->vfinfo[vf].trusted == setting)
		return 0;

	adapter->vfinfo[vf].trusted = setting;
	e_info(drv, "VF %u is %strusted\n", vf, setting ? "" : "not ");

	return 0;
}

int rnpgbe_ndo_set_vf_link_state(struct net_device *netdev, int vf, int state)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	int ret = 0;

	if (vf < 0 || vf >= adapter->num_vfs) {
		dev_err(pci_dev_to_dev(adapter->pdev),
			"NDO set VF link - invalid VF identifier %d\n", vf);
		ret = -EINVAL;
		goto out;
	}

	switch (state) {
	case IFLA_VF_LINK_STATE_ENABLE:
		dev_info(pci_dev_to_dev(adapter->pdev),
			 "NDO set VF %d link state %d\n", vf, state);
		adapter->vfinfo[vf].link_state = rnpgbe_link_state_on;
		rnpgbe_msg_post_status_signle_link(adapter, vf,
						   rnpgbe_link_state_on);
		break;
	case IFLA_VF_LINK_STATE_DISABLE:
		dev_info(pci_dev_to_dev(adapter->pdev),
			 "NDO set VF %d link state disable\n", vf);
		adapter->vfinfo[vf].link_state = rnpgbe_link_state_off;
		rnpgbe_msg_post_status_signle_link(adapter, vf,
						   rnpgbe_link_state_off);
		break;
	case IFLA_VF_LINK_STATE_AUTO:
		dev_info(pci_dev_to_dev(adapter->pdev),
			 "NDO set VF %d link state auto\n", vf);
		adapter->vfinfo[vf].link_state = rnpgbe_link_state_auto;
		rnpgbe_msg_post_status_signle_link(adapter, vf,
						   rnpgbe_link_state_auto);
		break;
	default:
		dev_err(pci_dev_to_dev(adapter->pdev),
			"NDO set VF %d - invalid link state %d\n", vf, state);
		ret = -EINVAL;
	}
out:
	return ret;
}

int rnpgbe_ndo_set_vf_bw(struct net_device *netdev, int vf,
			 int __always_unused min_tx_rate, int max_tx_rate)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	struct rnpgbe_hw *hw = &adapter->hw;
	/* limit vf ring rate */
	int ring_max_rate;
	int vf_ring;
	int link_speed = 0;
	u64 real_rate = 0;

	if (vf >= hw->max_vfs - 1)
		return -EINVAL;

	switch (adapter->link_speed) {
	case RNP_LINK_SPEED_40GB_FULL:
		link_speed = 40000;
		break;
	case RNP_LINK_SPEED_25GB_FULL:
		link_speed = 25000;
		break;
	case RNP_LINK_SPEED_10GB_FULL:
		link_speed = 10000;
		break;
	case RNP_LINK_SPEED_1GB_FULL:
		link_speed = 1000;
		break;
	case RNP_LINK_SPEED_100_FULL:
		link_speed = 100;
		break;
	}
	/* rate limit cannot be greater than link speed */
	if (max_tx_rate && (max_tx_rate > link_speed))
		return -EINVAL;

	adapter->vfinfo[vf].tx_rate = max_tx_rate;

	ring_max_rate = max_tx_rate / hw->sriov_ring_limit;

	if (max_tx_rate <= 10)
		real_rate = (ring_max_rate * 1000 * 85) >> 3;
	else if (max_tx_rate <= 50)
		real_rate = (ring_max_rate * 1000 * 90) >> 3;
	else if (max_tx_rate <= 100)
		real_rate = (ring_max_rate * 1000 * 94) >> 3;
	else
		real_rate = (ring_max_rate * 1000 * 99) >> 3;

	vf_ring = rnpgbe_get_vf_ringnum(hw, vf, 0);
	rnpgbe_setup_ring_maxrate(adapter, vf_ring, real_rate);

	return 0;
}

int rnpgbe_ndo_set_vf_mac(struct net_device *netdev, int vf, u8 *mac)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);

	if (!is_valid_ether_addr(mac) || (vf >= adapter->num_vfs))
		return -EINVAL;
	adapter->vfinfo[vf].pf_set_mac = true;
	dev_info(&adapter->pdev->dev, "setting MAC %pM on VF %d\n", mac, vf);
	dev_info(&adapter->pdev->dev, "Reload the VF driver to make this"
				      " change effective.");
	if (test_bit(__RNP_DOWN, &adapter->state)) {
		dev_warn(&adapter->pdev->dev,
			 "The VF MAC address has been set,"
			 " but the PF device is not up.\n");
		dev_warn(&adapter->pdev->dev,
			 "Bring the PF device up before"
			 " attempting to use the VF device.\n");
	}
	rnpgbe_set_vf_mac(adapter, vf, mac);
	/* send reset to vf only vf is up */
	if (adapter->vfinfo[vf].clear_to_send)
		rnpgbe_msg_post_status_signle(adapter, PF_SET_RESET, vf);

	return 0;
}

int rnpgbe_ndo_get_vf_config(struct net_device *netdev, int vf,
			     struct ifla_vf_info *ivi)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);

	if (vf >= adapter->num_vfs)
		return -EINVAL;
	ivi->vf = vf;
	memcpy(&ivi->mac, adapter->vfinfo[vf].vf_mac_addresses, ETH_ALEN);
	ivi->max_tx_rate = adapter->vfinfo[vf].tx_rate;
	ivi->min_tx_rate = 0;

	if (adapter->vfinfo[vf].pf_vlan)
		ivi->vlan = adapter->vfinfo[vf].pf_vlan;
	else
		ivi->vlan = adapter->vfinfo[vf].vf_vlan;

	ivi->qos = adapter->vfinfo[vf].pf_qos;
	ivi->spoofchk = adapter->vfinfo[vf].spoofchk_enabled;
	ivi->linkstate = adapter->vfinfo[vf].link_state;
	ivi->trusted = adapter->vfinfo[vf].trusted;

	return 0;
}

int rnpgbe_pci_sriov_configure(struct pci_dev *dev, int num_vfs)
{
	vf_dbg("\n\n !!!! %s:%d num_vfs:%d\n", __func__, __LINE__, num_vfs);
	if (num_vfs == 0)
		return rnpgbe_pci_sriov_disable(dev);
	else
		return rnpgbe_pci_sriov_enable(dev, num_vfs);
}
