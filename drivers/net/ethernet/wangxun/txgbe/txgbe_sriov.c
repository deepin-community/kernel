// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2019 - 2022 Beijing WangXun Technology Co., Ltd. */

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

#include "txgbe.h"
#include "txgbe_type.h"
#include "txgbe_sriov.h"

static void txgbe_set_vf_rx_tx(struct txgbe_adapter *adapter, int vf);
static int txgbe_set_queue_rate_limit_vf(struct txgbe_adapter *adapter,
					 u32 *msgbuf, u32 vf);

#ifdef CONFIG_PCI_IOV
static int __txgbe_enable_sriov(struct txgbe_adapter *adapter,
				unsigned int num_vfs)
{
	struct txgbe_hw *hw = &adapter->hw;
	int num_vf_macvlans, i;
	struct vf_macvlans *mv_list;
	u32 value = 0;

	adapter->flags |= TXGBE_FLAG_SRIOV_ENABLED;
	e_dev_info("SR-IOV enabled with %d VFs\n", num_vfs);

	if (num_vfs != 1) {
		if (adapter->ring_feature[RING_F_RSS].indices == 4)
			value = TXGBE_CFG_PORT_CTL_NUM_VT_32;
		else /* adapter->ring_feature[RING_F_RSS].indices <= 2 */
			value = TXGBE_CFG_PORT_CTL_NUM_VT_64;
	}
	wr32m(hw, TXGBE_CFG_PORT_CTL,
	      TXGBE_CFG_PORT_CTL_NUM_VT_MASK,
	      value);

	/* Enable VMDq flag so device will be set in VM mode */
	adapter->flags |= TXGBE_FLAG_VMDQ_ENABLED;
	if (!adapter->ring_feature[RING_F_VMDQ].limit)
		adapter->ring_feature[RING_F_VMDQ].limit = 1;
	adapter->ring_feature[RING_F_VMDQ].offset = num_vfs;

	num_vf_macvlans = hw->mac.num_rar_entries -
		(TXGBE_MAX_PF_MACVLANS + 1 + num_vfs);

	mv_list = kcalloc(num_vf_macvlans,
			  sizeof(struct vf_macvlans),
					     GFP_KERNEL);
	adapter->mv_list = mv_list;
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
	wr32m(hw, TXGBE_PSR_CTL,
	      TXGBE_PSR_CTL_SW_EN, TXGBE_PSR_CTL_SW_EN);

	/* If call to enable VFs succeeded then allocate memory
	 * for per VF control structures.
	 */
	adapter->vfinfo = kcalloc(num_vfs,
				  sizeof(struct vf_data_storage), GFP_KERNEL);
	if (!adapter->vfinfo)
		return -ENOMEM;

	adapter->num_vfs = num_vfs;

	/* enable L2 switch and replication */
	adapter->flags |= TXGBE_FLAG_SRIOV_L2SWITCH_ENABLE |
			  TXGBE_FLAG_SRIOV_REPLICATION_ENABLE;

	/* limit traffic classes based on VFs enabled */
	if (adapter->num_vfs < 16) {
		adapter->dcb_cfg.num_tcs.pg_tcs =
			TXGBE_DCB_MAX_TRAFFIC_CLASS;
		adapter->dcb_cfg.num_tcs.pfc_tcs =
			TXGBE_DCB_MAX_TRAFFIC_CLASS;
	} else if (adapter->num_vfs < 32) {
		adapter->dcb_cfg.num_tcs.pg_tcs = 4;
		adapter->dcb_cfg.num_tcs.pfc_tcs = 4;
	} else {
		adapter->dcb_cfg.num_tcs.pg_tcs = 1;
		adapter->dcb_cfg.num_tcs.pfc_tcs = 1;
	}
	adapter->dcb_cfg.vt_mode = true;

	/* Disable RSC when in SR-IOV mode */
	adapter->flags2 &= ~(TXGBE_FLAG2_RSC_CAPABLE |
				TXGBE_FLAG2_RSC_ENABLED);

	/* enable spoof checking for all VFs */
	for (i = 0; i < adapter->num_vfs; i++) {
		/* enable spoof checking for all VFs */
		adapter->vfinfo[i].spoofchk_enabled = true;
		adapter->vfinfo[i].link_enable = true;
		adapter->vfinfo[i].link_state = TXGBE_VF_LINK_STATE_AUTO;

		/* We support VF RSS querying only for 82599 and x540
		 * devices at the moment. These devices share RSS
		 * indirection table and RSS hash key with PF therefore
		 * we want to disable the querying by default.
		 */
		adapter->vfinfo[i].rss_query_enabled = 0;

		/* Untrust all VFs */
		adapter->vfinfo[i].trusted = false;

		/* set the default xcast mode */
		adapter->vfinfo[i].xcast_mode = TXGBEVF_XCAST_MODE_NONE;
	}

	return 0;
}

#define TXGBE_BA4_ADDR(vfinfo, reg) \
	((u8 __iomem *)((u8 *)(vfinfo)->b4_addr + (reg)))
static int txgbe_vf_backup(struct txgbe_adapter *adapter, u16 vf)
{
	struct vf_data_storage *vfinfo = &adapter->vfinfo[vf];

	if (!vfinfo->b4_addr)
		return -1;
	return 0;
}

static int txgbe_vf_restore(struct txgbe_adapter *adapter, u16 vf)
{
	struct vf_data_storage *vfinfo = &adapter->vfinfo[vf];

	if (!vfinfo->b4_addr)
		return -1;

	return 0;
}

/**
 * txgbe_get_vfs - Find and take references to all vf devices
 * @adapter: Pointer to adapter struct
 */
static void txgbe_get_vfs(struct txgbe_adapter *adapter)
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
#ifdef CONFIG_PCI_IOV
		txgbe_vf_backup(adapter, vf);
#endif
		++vf;
	}
}

/**
 * txgbe_pet_vfs - Release references to all vf devices
 * @adapter: Pointer to adapter struct
 */
static void txgbe_put_vfs(struct txgbe_adapter *adapter)
{
	unsigned int num_vfs = adapter->num_vfs, vf;

	/* put the reference to all of the vf devices */
	for (vf = 0; vf < num_vfs; ++vf) {
		struct vf_data_storage *vfinfo;
		struct pci_dev *vfdev = adapter->vfinfo[vf].vfdev;

		if (!vfdev)
			continue;

#ifdef CONFIG_PCI_IOV
		txgbe_vf_restore(adapter, vf);
#endif

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
void txgbe_enable_sriov(struct txgbe_adapter *adapter)
{
	int pre_existing_vfs = 0;
	unsigned int num_vfs;

	pre_existing_vfs = pci_num_vf(adapter->pdev);
	if (!pre_existing_vfs && !adapter->max_vfs)
		return;

	/* If there are pre-existing VFs then we have to force
	 * use of that many - over ride any module parameter value.
	 * This may result from the user unloading the PF driver
	 * while VFs were assigned to guest VMs or because the VFs
	 * have been created via the new PCI SR-IOV sysfs interface.
	 */
	if (pre_existing_vfs) {
		num_vfs = pre_existing_vfs;
		dev_warn(&adapter->pdev->dev,
			 "Virtual Functions already enabled for this device.\n");
	} else {
		int err;
		/* The sapphire/amber-lite supports up to 64 VFs per physical
		 * function but this implementation limits allocation to 63 so
		 * that basic networking resources are still available to the
		 * physical function.  If the user requests greater thn
		 * 63 VFs then it is an error - reset to default of zero.
		 */
		num_vfs = min_t(unsigned int, adapter->max_vfs,
				TXGBE_MAX_VFS_DRV_LIMIT);

		err = pci_enable_sriov(adapter->pdev, num_vfs);
		if (err) {
			e_err(probe, "Failed to enable PCI sriov: %d\n", err);
			adapter->num_vfs = 0;
			return;
		}
	}

	if (!__txgbe_enable_sriov(adapter, num_vfs)) {
		txgbe_get_vfs(adapter);
		return;
	}

	/* If we have gotten to this point then there is no memory available
	 * to manage the VF devices - print message and bail.
	 */
	e_err(probe, "Unable to allocate memory for VF Data Storage SRIOV disabled\n");
	txgbe_disable_sriov(adapter);
}
#endif /* CONFIG_PCI_IOV */

int txgbe_disable_sriov(struct txgbe_adapter *adapter)
{
	struct txgbe_hw *hw = &adapter->hw;

#ifdef CONFIG_PCI_IOV
	/* If our VFs are assigned we cannot shut down SR-IOV
	 * without causing issues, so just leave the hardware
	 * available but disabled
	 */
	if (pci_vfs_assigned(adapter->pdev)) {
		e_dev_warn("Unloading driver while VFs are assigned VFs will not be deallocated\n");
		return -EPERM;
	}
	/* disable iov and allow time for transactions to clear */
	pci_disable_sriov(adapter->pdev);
#endif

	/* set num VFs to 0 to prevent access to vfinfo */
	adapter->num_vfs = 0;

	/* put the reference to all of the vf devices */
#ifdef CONFIG_PCI_IOV
	txgbe_put_vfs(adapter);
#endif

	/* free VF control structures */
	kfree(adapter->vfinfo);
	adapter->vfinfo = NULL;

	/* free macvlan list */
	kfree(adapter->mv_list);
	adapter->mv_list = NULL;

	/* if SR-IOV is already disabled then there is nothing to do */
	if (!(adapter->flags & TXGBE_FLAG_SRIOV_ENABLED))
		return 0;

	/* set default pool back to 0 */
	wr32m(hw, TXGBE_PSR_VM_CTL,
	      TXGBE_PSR_VM_CTL_POOL_MASK, 0);
	TXGBE_WRITE_FLUSH(hw);

	adapter->ring_feature[RING_F_VMDQ].offset = 0;

	/* take a breather then clean up driver data */
	msleep(100);

	adapter->flags &= ~(TXGBE_FLAG_SRIOV_ENABLED |
						TXGBE_FLAG_SRIOV_L2SWITCH_ENABLE |
						TXGBE_FLAG_SRIOV_REPLICATION_ENABLE);

	/* Disable VMDq flag so device will be set in VM mode */
	if (adapter->ring_feature[RING_F_VMDQ].limit == 1)
		adapter->flags &= ~TXGBE_FLAG_VMDQ_ENABLED;

	return 0;
}

static int txgbe_set_vf_multicasts(struct txgbe_adapter *adapter,
				   u32 *msgbuf, u32 vf)
{
	u16 entries = (msgbuf[0] & TXGBE_VT_MSGINFO_MASK)
		       >> TXGBE_VT_MSGINFO_SHIFT;
	u16 *hash_list = (u16 *)&msgbuf[1];
	struct vf_data_storage *vfinfo = &adapter->vfinfo[vf];
	struct txgbe_hw *hw = &adapter->hw;
	int i;
	u32 vector_bit;
	u32 vector_reg;
	u32 mta_reg;
	u32 vmolr = rd32(hw, TXGBE_PSR_VM_L2CTL(vf));

	/* only so many hash values supported */
	entries = min_t(u16, entries, TXGBE_MAX_VF_MC_ENTRIES);

	/* salt away the number of multi cast addresses assigned
	 * to this VF for later use to restore when the PF multi cast
	 * list changes
	 */
	vfinfo->num_vf_mc_hashes = entries;

	/* VFs are limited to using the MTA hash table for their multicast
	 * addresses
	 */
	for (i = 0; i < entries; i++)
		vfinfo->vf_mc_hashes[i] = hash_list[i];

	for (i = 0; i < vfinfo->num_vf_mc_hashes; i++) {
		vector_reg = (vfinfo->vf_mc_hashes[i] >> 5) & 0x7F;
		vector_bit = vfinfo->vf_mc_hashes[i] & 0x1F;
		/* errata 5: maintain a copy of the register table conf */
		mta_reg = hw->mac.mta_shadow[vector_reg];
		mta_reg |= (1 << vector_bit);
		hw->mac.mta_shadow[vector_reg] = mta_reg;
		wr32(hw, TXGBE_PSR_MC_TBL(vector_reg), mta_reg);
	}
	vmolr |= TXGBE_PSR_VM_L2CTL_ROMPE;
	wr32(hw, TXGBE_PSR_VM_L2CTL(vf), vmolr);

	return 0;
}

void txgbe_restore_vf_multicasts(struct txgbe_adapter *adapter)
{
	struct txgbe_hw *hw = &adapter->hw;
	struct vf_data_storage *vfinfo;
	u32 i, j;
	u32 vector_bit;
	u32 vector_reg;

	for (i = 0; i < adapter->num_vfs; i++) {
		u32 vmolr = rd32(hw, TXGBE_PSR_VM_L2CTL(i));

		vfinfo = &adapter->vfinfo[i];
		for (j = 0; j < vfinfo->num_vf_mc_hashes; j++) {
			hw->addr_ctrl.mta_in_use++;
			vector_reg = (vfinfo->vf_mc_hashes[j] >> 5) & 0x7F;
			vector_bit = vfinfo->vf_mc_hashes[j] & 0x1F;
			wr32m(hw, TXGBE_PSR_MC_TBL(vector_reg),
			      1 << vector_bit, 1 << vector_bit);
			/* errata 5: maintain a copy of the reg table conf */
			hw->mac.mta_shadow[vector_reg] |= (1 << vector_bit);
		}
		if (vfinfo->num_vf_mc_hashes)
			vmolr |= TXGBE_PSR_VM_L2CTL_ROMPE;
		else
			vmolr &= ~TXGBE_PSR_VM_L2CTL_ROMPE;
		wr32(hw, TXGBE_PSR_VM_L2CTL(i), vmolr);
	}

	/* Restore any VF macvlans */
	txgbe_full_sync_mac_table(adapter);
}

int txgbe_set_vf_vlan(struct txgbe_adapter *adapter, int add, int vid, u16 vf)
{
	struct txgbe_hw *hw = &adapter->hw;

	/* VLAN 0 is a special case, don't allow it to be removed */
	if (!vid && !add)
		return 0;

	return hw->mac.ops.set_vfta(hw, vid, vf, (bool)add);
}

static int txgbe_set_vf_lpe(struct txgbe_adapter *adapter, u32 max_frame,
			    u32 vf)
{
	struct txgbe_hw *hw = &adapter->hw;
	u32 max_frs, reg_val;

	/* For sapphire/amber-lite we have to keep all PFs and VFs operating
	 * with the same max_frame value in order to avoid sending an oversize
	 * frame to a VF.  In order to guarantee this is handled correctly
	 * for all cases we have several special exceptions to take into
	 * account before we can enable the VF for receive
	 */
	u32 reg_offset, vf_shift, vfre;
	s32 err = 0;

	/* determine VF receive enable location */
	vf_shift = vf % 32;
	reg_offset = vf / 32;

	/* enable or disable receive depending on error */
	vfre = rd32(hw, TXGBE_RDM_VF_RE(reg_offset));
	if (err)
		vfre &= ~(1 << vf_shift);
	else
		vfre |= 1 << vf_shift;
	wr32(hw, TXGBE_RDM_VF_RE(reg_offset), vfre);

	/* pull current max frame size from hardware */
	max_frs = rd32(hw, TXGBE_PSR_MAX_SZ);
	if (max_frs < max_frame)
		wr32(hw, TXGBE_PSR_MAX_SZ, max_frame);

	/* pull current max frame size from hardware */
	max_frs = DIV_ROUND_UP(max_frame, 1024);
	reg_val = rd32(hw, TXGBE_MAC_WDG_TIMEOUT) &
		TXGBE_MAC_WDG_TIMEOUT_WTO_MASK;
	if (max_frs > (reg_val + TXGBE_MAC_WDG_TIMEOUT_WTO_DELTA))
		wr32(hw, TXGBE_MAC_WDG_TIMEOUT,
		     max_frs - TXGBE_MAC_WDG_TIMEOUT_WTO_DELTA);

	e_info(hw, "VF requests change max MTU to %d\n", max_frame);

	return 0;
}

void txgbe_set_vmolr(struct txgbe_hw *hw, u16 vf, bool aupe)
{
	u32 vmolr = rd32(hw, TXGBE_PSR_VM_L2CTL(vf));

	vmolr |=  TXGBE_PSR_VM_L2CTL_BAM;
	if (aupe)
		vmolr |= TXGBE_PSR_VM_L2CTL_AUPE;
	else
		vmolr &= ~TXGBE_PSR_VM_L2CTL_AUPE;
	wr32(hw, TXGBE_PSR_VM_L2CTL(vf), vmolr);
}

static void txgbe_set_vmvir(struct txgbe_adapter *adapter,
			    u16 vid, u16 qos, u16 vf, __be16 vlan_proto)
{
	struct txgbe_hw *hw = &adapter->hw;
	u32 vmvir = vid | (qos << VLAN_PRIO_SHIFT) |
		TXGBE_TDM_VLAN_INS_VLANA_DEFAULT;

	if (vlan_proto == htons(ETH_P_8021AD))
		vmvir |= 1 << TXGBE_TDM_VLAN_INS_TPID_SEL_SHIFT;
	wr32(hw, TXGBE_TDM_VLAN_INS(vf), vmvir);
}

static void txgbe_clear_vmvir(struct txgbe_adapter *adapter, u32 vf)
{
	struct txgbe_hw *hw = &adapter->hw;

	wr32(hw, TXGBE_TDM_VLAN_INS(vf), 0);
}

static inline void txgbe_vf_reset_event(struct txgbe_adapter *adapter, u16 vf)
{
	struct txgbe_hw *hw = &adapter->hw;
	struct vf_data_storage *vfinfo = &adapter->vfinfo[vf];
	u8 num_tcs = netdev_get_num_tc(adapter->netdev);

	/* add PF assigned VLAN or VLAN 0 */
	txgbe_set_vf_vlan(adapter, true, vfinfo->pf_vlan, vf);

	/* reset offloads to defaults */
	txgbe_set_vmolr(hw, vf, !vfinfo->pf_vlan);

	/* set outgoing tags for VFs */
	if (!vfinfo->pf_vlan && !vfinfo->pf_qos && !num_tcs) {
		txgbe_clear_vmvir(adapter, vf);
	} else {
		if (vfinfo->pf_qos || !num_tcs)
			txgbe_set_vmvir(adapter, vfinfo->pf_vlan,
					vfinfo->pf_qos, vf, vfinfo->vlan_proto);
		else
			txgbe_set_vmvir(adapter, vfinfo->pf_vlan,
					adapter->default_up, vf, vfinfo->vlan_proto);

		if (vfinfo->spoofchk_enabled)
			hw->mac.ops.set_vlan_anti_spoofing(hw, true, vf);
	}

	/* reset multicast table array for vf */
	adapter->vfinfo[vf].num_vf_mc_hashes = 0;

	/* Flush and reset the mta with the new values */
	txgbe_set_rx_mode(adapter->netdev);

	txgbe_del_mac_filter(adapter, adapter->vfinfo[vf].vf_mac_addresses, vf);

	/* reset VF api back to unknown */
	adapter->vfinfo[vf].vf_api = txgbe_mbox_api_10;
}

int txgbe_set_vf_mac(struct txgbe_adapter *adapter,
		     u16 vf, unsigned char *mac_addr)
{
	s32 retval = 0;

	txgbe_del_mac_filter(adapter, adapter->vfinfo[vf].vf_mac_addresses, vf);
	retval = txgbe_add_mac_filter(adapter, mac_addr, vf);
	if (retval >= 0)
		memcpy(adapter->vfinfo[vf].vf_mac_addresses, mac_addr,
		       ETH_ALEN);
	else
		memset(adapter->vfinfo[vf].vf_mac_addresses, 0, ETH_ALEN);

	return retval;
}

static int txgbe_negotiate_vf_api(struct txgbe_adapter *adapter,
				  u32 *msgbuf, u32 vf)
{
	int api = msgbuf[1];

	switch (api) {
	case txgbe_mbox_api_10:
	case txgbe_mbox_api_11:
	case txgbe_mbox_api_12:
	case txgbe_mbox_api_13:
	case txgbe_mbox_api_21:
	case txgbe_mbox_api_22:
		adapter->vfinfo[vf].vf_api = api;
		return 0;
	default:
		break;
	}

	e_info(drv, "VF %d requested invalid api version %u\n", vf, api);

	return -1;
}

static int txgbe_get_vf_queues(struct txgbe_adapter *adapter,
			       u32 *msgbuf, u32 vf)
{
	struct net_device *dev = adapter->netdev;
	struct txgbe_ring_feature *vmdq = &adapter->ring_feature[RING_F_VMDQ];
	unsigned int default_tc = 0;
	u8 num_tcs = netdev_get_num_tc(dev);

	/* verify the PF is supporting the correct APIs */
	switch (adapter->vfinfo[vf].vf_api) {
	case txgbe_mbox_api_22:
	case txgbe_mbox_api_21:
	case txgbe_mbox_api_20:
	case txgbe_mbox_api_13:
	case txgbe_mbox_api_12:
	case txgbe_mbox_api_11:
		break;
	default:
		return -1;
	}

	/* only allow 1 Tx queue for bandwidth limiting */
	msgbuf[TXGBE_VF_TX_QUEUES] = __ALIGN_MASK(1, ~vmdq->mask);
	msgbuf[TXGBE_VF_RX_QUEUES] = __ALIGN_MASK(1, ~vmdq->mask);

	/* if TCs > 1 determine which TC belongs to default user priority */
	if (num_tcs > 1)
		default_tc = netdev_get_prio_tc_map(dev, adapter->default_up);

	/* notify VF of need for VLAN tag stripping, and correct queue */
	if (num_tcs)
		msgbuf[TXGBE_VF_TRANS_VLAN] = num_tcs;
	else if (adapter->vfinfo[vf].pf_vlan || adapter->vfinfo[vf].pf_qos)
		msgbuf[TXGBE_VF_TRANS_VLAN] = 1;
	else
		msgbuf[TXGBE_VF_TRANS_VLAN] = 0;

	/* notify VF of default queue */
	msgbuf[TXGBE_VF_DEF_QUEUE] = default_tc;

	return 0;
}

static int txgbe_set_vf_macvlan(struct txgbe_adapter *adapter,
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
				txgbe_del_mac_filter(adapter,
						     entry->vf_macvlan, vf);
			}
		}
	}

	/* If index was zero then we were asked to clear the uc list
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

	/* If we traversed the entire list and didn't find a free entry
	 * then we're out of space on the RAR table.  Also entry may
	 * be NULL because the original memory allocation for the list
	 * failed, which is not fatal but does mean we can't support
	 * VF requests for MACVLAN because we couldn't allocate
	 * memory for the list management required.
	 */
	if (!entry || !entry->free)
		return -ENOSPC;

	retval = txgbe_add_mac_filter(adapter, mac_addr, vf);
	if (retval >= 0) {
		entry->free = false;
		entry->is_macvlan = true;
		entry->vf = vf;
		memcpy(entry->vf_macvlan, mac_addr, ETH_ALEN);
	}

	return retval;
}

#ifdef CONFIG_PCI_IOV
int txgbe_vf_configuration(struct pci_dev *pdev, unsigned int event_mask)
{
	unsigned char vf_mac_addr[6];
	struct txgbe_adapter *adapter = pci_get_drvdata(pdev);
	unsigned int vfn = (event_mask & 0x3f);
	bool enable = ((event_mask & 0x10000000U) != 0);

	if (enable) {
		memset(vf_mac_addr, 0, ETH_ALEN);
		memcpy(adapter->vfinfo[vfn].vf_mac_addresses, vf_mac_addr, 6);
	}

	return 0;
}
#endif /* CONFIG_PCI_IOV */

static inline void txgbe_write_qde(struct txgbe_adapter *adapter, u32 vf,
				   u32 qde)
{
	struct txgbe_hw *hw = &adapter->hw;
	struct txgbe_ring_feature *vmdq = &adapter->ring_feature[RING_F_VMDQ];
	u32 q_per_pool = __ALIGN_MASK(1, ~vmdq->mask);
	u32 reg = 0;
	u32 i = vf * q_per_pool;
	u32 n = i / 32;

	reg = rd32(hw, TXGBE_RDM_PF_QDE(n));
	for (i = (vf * q_per_pool - n * 32);
	     i < ((vf + 1) * q_per_pool - n * 32);
	     i++) {
		if (qde == 1)
			reg |= qde << i;
		else
			reg &= qde << i;
	}

	wr32(hw, TXGBE_RDM_PF_QDE(n), reg);
}

static inline void txgbe_write_hide_vlan(struct txgbe_adapter *adapter, u32 vf,
					 u32 hide_vlan)
{
	struct txgbe_hw *hw = &adapter->hw;
	struct txgbe_ring_feature *vmdq = &adapter->ring_feature[RING_F_VMDQ];
	u32 q_per_pool = __ALIGN_MASK(1, ~vmdq->mask);
	u32 reg = 0;
	u32 i = vf * q_per_pool;
	u32 n = i / 32;

	reg = rd32(hw, TXGBE_RDM_PF_HIDE(n));
	for (i = (vf * q_per_pool - n * 32);
	     i < ((vf + 1) * q_per_pool - n * 32);
	     i++) {
		if (hide_vlan == 1)
			reg |= hide_vlan << i;
		else
			reg &= hide_vlan << i;
	}

	wr32(hw, TXGBE_RDM_PF_HIDE(n), reg);
}

static int txgbe_vf_reset_msg(struct txgbe_adapter *adapter, u16 vf)
{
	struct txgbe_hw *hw = &adapter->hw;
	unsigned char *vf_mac = adapter->vfinfo[vf].vf_mac_addresses;
	u32 reg = 0;
	u32 reg_offset, vf_shift;
	u32 msgbuf[4] = {0, 0, 0, 0};
	u8 *addr = (u8 *)(&msgbuf[1]);
	struct net_device *dev = adapter->netdev;
	int pf_max_frame;

	e_info(probe, "VF Reset msg received from vf %d\n", vf);

#ifdef CONFIG_PCI_IOV
	txgbe_vf_restore(adapter, vf);
#endif

	/* reset the filters for the device */
	txgbe_vf_reset_event(adapter, vf);

	/* set vf mac address */
	if (!is_zero_ether_addr(vf_mac))
		txgbe_set_vf_mac(adapter, vf, vf_mac);

	vf_shift = vf % 32;
	reg_offset = vf / 32;

	/* force drop enable for all VF Rx queues */
	txgbe_write_qde(adapter, vf, 1);

	/* set transmit and receive for vf */
	txgbe_set_vf_rx_tx(adapter, vf);

	pf_max_frame = dev->mtu + ETH_HLEN;

#if IS_ENABLED(CONFIG_FCOE)
	if (dev->features & NETIF_F_FCOE_MTU)
		pf_max_frame = max_t(int, pf_max_frame,
				     TXGBE_FCOE_JUMBO_FRAME_SIZE);
#endif /* CONFIG_FCOE */

	if (pf_max_frame > ETH_FRAME_LEN)
		reg = (1 << vf_shift);
	wr32(hw, TXGBE_RDM_VFRE_CLR(reg_offset), reg);

	/* enable VF mailbox for further messages */
	adapter->vfinfo[vf].clear_to_send = true;

	/* reply to reset with ack and vf mac address */
	msgbuf[0] = TXGBE_VF_RESET;
	if (!is_zero_ether_addr(vf_mac)) {
		msgbuf[0] |= TXGBE_VT_MSGTYPE_ACK;
		memcpy(addr, vf_mac, ETH_ALEN);
	} else {
		msgbuf[0] |= TXGBE_VT_MSGTYPE_NACK;
		dev_warn(pci_dev_to_dev(adapter->pdev),
			 "VF %d has no MAC address assigned, you may have to assign one manually\n",
			vf);
	}

	/* Piggyback the multicast filter type so VF can compute the
	 * correct vectors
	 */
	msgbuf[3] = hw->mac.mc_filter_type;
	txgbe_write_mbx(hw, msgbuf, TXGBE_VF_PERMADDR_MSG_LEN, vf);

	return 0;
}

static int txgbe_set_vf_mac_addr(struct txgbe_adapter *adapter,
				 u32 *msgbuf, u16 vf)
{
	u8 *new_mac = ((u8 *)(&msgbuf[1]));

	if (!is_valid_ether_addr(new_mac)) {
		e_warn(drv, "VF %d attempted to set invalid mac\n", vf);
		return -1;
	}

	if (adapter->vfinfo[vf].pf_set_mac && !adapter->vfinfo[vf].trusted &&
	    memcmp(adapter->vfinfo[vf].vf_mac_addresses, new_mac,
		   ETH_ALEN)) {
		u8 *pm = adapter->vfinfo[vf].vf_mac_addresses;

		e_warn(drv, "VF %d attempted to set a new MAC address but it already ", vf);
		e_warn(drv, "has an administratively set MAC address ");
		e_warn(drv, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X\n",
		       pm[0], pm[1], pm[2], pm[3], pm[4], pm[5]);
		return -1;
	}
	return txgbe_set_vf_mac(adapter, vf, new_mac) < 0;
}

#ifdef CONFIG_PCI_IOV
static int txgbe_find_vlvf_entry(struct txgbe_hw *hw, u32 vlan)
{
	u32 vlvf;
	s32 regindex;

	/* short cut the special case */
	if (vlan == 0)
		return 0;

	/* Search for the vlan id in the VLVF entries */
	for (regindex = 1; regindex < TXGBE_PSR_VLAN_SWC_ENTRIES; regindex++) {
		wr32(hw, TXGBE_PSR_VLAN_SWC_IDX, regindex);
		vlvf = rd32(hw, TXGBE_PSR_VLAN_SWC);
		if ((vlvf & VLAN_VID_MASK) == vlan)
			break;
	}

	/* Return a negative value if not found */
	if (regindex >= TXGBE_PSR_VLAN_SWC_ENTRIES)
		regindex = -1;

	return regindex;
}
#endif /* CONFIG_PCI_IOV */

static int txgbe_set_vf_vlan_msg(struct txgbe_adapter *adapter,
				 u32 *msgbuf, u16 vf)
{
	struct txgbe_hw *hw = &adapter->hw;
	int add = (msgbuf[0] & TXGBE_VT_MSGINFO_MASK) >> TXGBE_VT_MSGINFO_SHIFT;
	int vid = (msgbuf[1] & TXGBE_PSR_VLAN_SWC_VLANID_MASK);
	int vlan_offload = (msgbuf[0] & TXGBE_VT_MSGINFO_MASK) >>
						TXGBE_VT_MSGINFO_VLAN_OFFLOAD_SHIFT;
	int err = 0;
	u8 tcs = netdev_get_num_tc(adapter->netdev);

	if (adapter->vfinfo[vf].pf_vlan || tcs) {
		if (!vlan_offload) {
			goto out;
		} else {
			e_warn(drv, "VF %d attempted to override set VLAN configuration\n",
			       vf);
			return -1;
		}
	}

	if (add)
		adapter->vfinfo[vf].vlan_count++;
	else if (adapter->vfinfo[vf].vlan_count)
		adapter->vfinfo[vf].vlan_count--;

	/* in case of promiscuous mode any VLAN filter set for a VF must
	 * also have the PF pool added to it.
	 */
	if (add && adapter->netdev->flags & IFF_PROMISC)
		err = txgbe_set_vf_vlan(adapter, add, vid, VMDQ_P(0));

	err = txgbe_set_vf_vlan(adapter, add, vid, vf);
	if (!err && adapter->vfinfo[vf].spoofchk_enabled)
		hw->mac.ops.set_vlan_anti_spoofing(hw, true, vf);

#ifdef CONFIG_PCI_IOV
	/* Go through all the checks to see if the VLAN filter should
	 * be wiped completely.
	 */
	if (!add && adapter->netdev->flags & IFF_PROMISC) {
		u32 bits, vlvf;
		s32 reg_ndx;

		reg_ndx = txgbe_find_vlvf_entry(hw, vid);
		if (reg_ndx < 0)
			goto out;
		wr32(hw, TXGBE_PSR_VLAN_SWC_IDX, reg_ndx);
		vlvf = rd32(hw, TXGBE_PSR_VLAN_SWC);
		/* See if any other pools are set for this VLAN filter
		 * entry other than the PF.
		 */
		if (VMDQ_P(0) < 32) {
			bits = rd32(hw, TXGBE_PSR_VLAN_SWC_VM_L);
			bits &= ~(1 << VMDQ_P(0));
			bits |= rd32(hw,
					       TXGBE_PSR_VLAN_SWC_VM_H);
		} else {
			bits = rd32(hw,
				    TXGBE_PSR_VLAN_SWC_VM_H);
			bits &= ~(1 << (VMDQ_P(0) - 32));
			bits |= rd32(hw, TXGBE_PSR_VLAN_SWC_VM_L);
		}

		/* If the filter was removed then ensure PF pool bit
		 * is cleared if the PF only added itself to the pool
		 * because the PF is in promiscuous mode.
		 */
		if ((vlvf & VLAN_VID_MASK) == vid &&
		    !test_bit(vid, adapter->active_vlans) &&
		    !bits)
			txgbe_set_vf_vlan(adapter, add, vid, VMDQ_P(0));
	}

out:
#endif
	return err;
}

static int txgbe_set_vf_macvlan_msg(struct txgbe_adapter *adapter,
				    u32 *msgbuf, u16 vf)
{
	u8 *new_mac = ((u8 *)(&msgbuf[1]));
	int index = (msgbuf[0] & TXGBE_VT_MSGINFO_MASK) >>
		    TXGBE_VT_MSGINFO_SHIFT;
	int err;

	if (adapter->vfinfo[vf].pf_set_mac && !adapter->vfinfo[vf].trusted &&
	    index > 0) {
		e_warn(drv,
		       "VF %d requested MACVLAN filter but is administratively denied\n",
			vf);
		return 0;
	}

	/* An non-zero index indicates the VF is setting a filter */
	if (index) {
		if (!is_valid_ether_addr(new_mac)) {
			e_warn(drv, "VF %d attempted to set invalid mac\n", vf);
			return -1;
		}
		/* If the VF is allowed to set MAC filters then turn off
		 * anti-spoofing to avoid false positives.
		 */
		if (adapter->vfinfo[vf].spoofchk_enabled)
			txgbe_ndo_set_vf_spoofchk(adapter->netdev, vf, false);
	}

	err = txgbe_set_vf_macvlan(adapter, vf, index, new_mac);
	if (err == -ENOSPC)
		e_warn(drv, "VF %d has requested a MACVLAN filter but there is no space for it\n",
		       vf);

	return err < 0;
}

static int txgbe_update_vf_xcast_mode(struct txgbe_adapter *adapter,
				      u32 *msgbuf, u32 vf)
{
	struct txgbe_hw *hw = &adapter->hw;
	int xcast_mode = msgbuf[1];
	u32 vmolr, disable, enable;

	/* verify the PF is supporting the correct APIs */
	switch (adapter->vfinfo[vf].vf_api) {
	case txgbe_mbox_api_12:
		/* promisc introduced in 1.3 version */
		if (xcast_mode == TXGBEVF_XCAST_MODE_PROMISC)
			return -EOPNOTSUPP;
	case txgbe_mbox_api_13:
	case txgbe_mbox_api_20:
	case txgbe_mbox_api_21:
	case txgbe_mbox_api_22:
		break;
	default:
		return -EOPNOTSUPP;
	}

	if (xcast_mode > TXGBEVF_XCAST_MODE_MULTI &&
	    !adapter->vfinfo[vf].trusted) {
		xcast_mode = TXGBEVF_XCAST_MODE_MULTI;
	}

	if (adapter->vfinfo[vf].xcast_mode == xcast_mode)
		goto out;

	switch (xcast_mode) {
	case TXGBEVF_XCAST_MODE_NONE:
		disable = TXGBE_PSR_VM_L2CTL_BAM | TXGBE_PSR_VM_L2CTL_ROMPE |
			  TXGBE_PSR_VM_L2CTL_MPE | TXGBE_PSR_VM_L2CTL_UPE | TXGBE_PSR_VM_L2CTL_VPE;
		enable = 0;
		break;
	case TXGBEVF_XCAST_MODE_MULTI:
		disable = TXGBE_PSR_VM_L2CTL_MPE | TXGBE_PSR_VM_L2CTL_UPE | TXGBE_PSR_VM_L2CTL_VPE;
		enable = TXGBE_PSR_VM_L2CTL_BAM | TXGBE_PSR_VM_L2CTL_ROMPE;
		break;
	case TXGBEVF_XCAST_MODE_ALLMULTI:
		disable = TXGBE_PSR_VM_L2CTL_UPE;
		enable = TXGBE_PSR_VM_L2CTL_BAM | TXGBE_PSR_VM_L2CTL_ROMPE |
			 TXGBE_PSR_VM_L2CTL_MPE | TXGBE_PSR_VM_L2CTL_VPE;
		break;
	case TXGBEVF_XCAST_MODE_PROMISC:
		disable = 0;
		enable = TXGBE_PSR_VM_L2CTL_BAM | TXGBE_PSR_VM_L2CTL_ROMPE |
			 TXGBE_PSR_VM_L2CTL_MPE | TXGBE_PSR_VM_L2CTL_UPE | TXGBE_PSR_VM_L2CTL_VPE;
		break;
	default:
		return -EOPNOTSUPP;
	}

	vmolr = rd32(hw, TXGBE_PSR_VM_L2CTL(vf));
	vmolr &= ~disable;
	vmolr |= enable;
	wr32(hw, TXGBE_PSR_VM_L2CTL(vf), vmolr);

	adapter->vfinfo[vf].xcast_mode = xcast_mode;

out:
	msgbuf[1] = xcast_mode;

	return 0;
}

static int txgbe_get_vf_link_state(struct txgbe_adapter *adapter,
				   u32 *msgbuf, u32 vf)
{
	u32 *link_state = &msgbuf[1];

	/* verify the PF is supporting the correct API */
	switch (adapter->vfinfo[vf].vf_api) {
	case txgbe_mbox_api_12:
	case txgbe_mbox_api_13:
	case txgbe_mbox_api_21:
	case txgbe_mbox_api_22:
		break;
	default:
		return -EOPNOTSUPP;
	}
	*link_state = adapter->vfinfo[vf].link_state;

	return 0;
}

static int txgbe_get_fw_version(struct txgbe_adapter *adapter,
				u32 *msgbuf, u32 vf)
{
	unsigned long *fw_version = (unsigned long *)&msgbuf[1];
	int ret;

	/* verify the PF is supporting the correct API */
	switch (adapter->vfinfo[vf].vf_api) {
	case txgbe_mbox_api_12:
	case txgbe_mbox_api_13:
	case txgbe_mbox_api_21:
	case txgbe_mbox_api_22:
		break;
	default:
		return -EOPNOTSUPP;
	}

	ret = kstrtoul(adapter->eeprom_id, 16, fw_version);
	if (ret < 0)
		return ret;

	if (*fw_version == 0)
		return -EOPNOTSUPP;

	return 0;
}

static int txgbe_add_5tuple_filter_vf(struct txgbe_adapter *adapter,
				      u32 *msgbuf, u32 vf)
{
	struct txgbe_5tuple_filter_info *filter = &adapter->ft_filter_info;
	struct txgbe_hw *hw = &adapter->hw;
	u16 index, sw_idx, i, j;

	/* look for an unused 5tuple filter index,
	 * and insert the filter to list.
	 */
	for (sw_idx = 0; sw_idx < TXGBE_MAX_RDB_5T_CTL0_FILTERS; sw_idx++) {
		i = sw_idx / (sizeof(uint32_t) * 8);
		j = sw_idx % (sizeof(uint32_t) * 8);
		if (!(filter->fivetuple_mask[i] & (1 << j))) {
			filter->fivetuple_mask[i] |= 1 << j;
			break;
		}
	}
	if (sw_idx >= TXGBE_MAX_RDB_5T_CTL0_FILTERS) {
		e_err(drv, "5tuple filters are full.\n");
		return -EINVAL;
	}

	/* convert filter index on each vf to the global index */
	index = msgbuf[TXGBEVF_5T_CMD] & 0xFFFF;
	adapter->vfinfo[vf].ft_filter_idx[index] = sw_idx;

	/* pool index */
	msgbuf[TXGBEVF_5T_CTRL0] |= vf << TXGBE_RDB_5T_CTL0_POOL_SHIFT;
	/* compute absolute queue index */
	msgbuf[TXGBEVF_5T_CTRL1] += (vf * adapter->num_rx_queues_per_pool) <<
				    TXGBE_RDB_5T_CTL1_RING_SHIFT;

	wr32(hw, TXGBE_RDB_5T_CTL0(sw_idx), msgbuf[TXGBEVF_5T_CTRL0]);
	wr32(hw, TXGBE_RDB_5T_CTL1(sw_idx), msgbuf[TXGBEVF_5T_CTRL1]);
	wr32(hw, TXGBE_RDB_5T_SDP(sw_idx), msgbuf[TXGBEVF_5T_PORT]);
	wr32(hw, TXGBE_RDB_5T_DA(sw_idx), msgbuf[TXGBEVF_5T_DA]);
	wr32(hw, TXGBE_RDB_5T_SA(sw_idx), msgbuf[TXGBEVF_5T_SA]);

	return 0;
}

static void txgbe_del_5tuple_filter_vf(struct txgbe_adapter *adapter,
				       u32 cmd, u32 vf)
{
	struct txgbe_5tuple_filter_info *filter = &adapter->ft_filter_info;
	struct txgbe_hw *hw = &adapter->hw;
	u16 index, sw_idx;

	/* convert the global index to filter index on each vf */
	index = cmd & 0xFFFF;
	sw_idx = adapter->vfinfo[vf].ft_filter_idx[index];

	filter->fivetuple_mask[sw_idx / (sizeof(uint32_t) * 8)] &=
		~(1 << (sw_idx % (sizeof(uint32_t) * 8)));

	wr32(hw, TXGBE_RDB_5T_CTL0(sw_idx), 0);
	wr32(hw, TXGBE_RDB_5T_CTL1(sw_idx), 0);
	wr32(hw, TXGBE_RDB_5T_SDP(sw_idx), 0);
	wr32(hw, TXGBE_RDB_5T_DA(sw_idx), 0);
	wr32(hw, TXGBE_RDB_5T_SA(sw_idx), 0);
}

static int txgbe_set_5tuple_filter_vf(struct txgbe_adapter *adapter,
				      u32 *msgbuf, u32 vf)
{
	u32 cmd = msgbuf[TXGBEVF_5T_CMD];
	bool add;

	/* verify the PF is supporting the correct API */
	if (adapter->vfinfo[vf].vf_api < txgbe_mbox_api_21)
		return -EOPNOTSUPP;

	add = !!(cmd & BIT(TXGBEVF_5T_ADD_SHIFT));
	if (add)
		return txgbe_add_5tuple_filter_vf(adapter, msgbuf, vf);

	txgbe_del_5tuple_filter_vf(adapter, cmd, vf);

	return 0;
}

static int txgbe_rcv_msg_from_vf(struct txgbe_adapter *adapter, u16 vf)
{
	u16 mbx_size = TXGBE_VXMAILBOX_SIZE;
	u32 msgbuf[TXGBE_VXMAILBOX_SIZE];
	struct txgbe_hw *hw = &adapter->hw;
	s32 retval;

	retval = txgbe_read_mbx(hw, msgbuf, mbx_size, vf);

	if (retval) {
		pr_err("Error receiving message from VF\n");
		return retval;
	}

	/* this is a message we already processed, do nothing */
	if (msgbuf[0] & (TXGBE_VT_MSGTYPE_ACK | TXGBE_VT_MSGTYPE_NACK))
		return retval;

	/* flush the ack before we write any messages back */
	TXGBE_WRITE_FLUSH(hw);

	if (msgbuf[0] == TXGBE_VF_RESET)
		return txgbe_vf_reset_msg(adapter, vf);

	/* until the vf completes a virtual function reset it should not be
	 * allowed to start any configuration.
	 */

	if (!adapter->vfinfo[vf].clear_to_send) {
		msgbuf[0] |= TXGBE_VT_MSGTYPE_NACK;
		txgbe_write_mbx(hw, msgbuf, 1, vf);
		return retval;
	}

	switch ((msgbuf[0] & 0xFFFF)) {
	case TXGBE_VF_SET_MAC_ADDR:
		retval = txgbe_set_vf_mac_addr(adapter, msgbuf, vf);
		break;
	case TXGBE_VF_SET_MULTICAST:
		retval = txgbe_set_vf_multicasts(adapter, msgbuf, vf);
		break;
	case TXGBE_VF_SET_VLAN:
		retval = txgbe_set_vf_vlan_msg(adapter, msgbuf, vf);
		break;
	case TXGBE_VF_SET_LPE:
		if (msgbuf[1] > TXGBE_MAX_JUMBO_FRAME_SIZE) {
			e_err(drv, "VF max_frame %d out of range\n", msgbuf[1]);
			return -EINVAL;
		}
		retval = txgbe_set_vf_lpe(adapter, msgbuf[1], vf);
		break;
	case TXGBE_VF_SET_MACVLAN:
		retval = txgbe_set_vf_macvlan_msg(adapter, msgbuf, vf);
		break;
	case TXGBE_VF_API_NEGOTIATE:
		retval = txgbe_negotiate_vf_api(adapter, msgbuf, vf);
		break;
	case TXGBE_VF_GET_QUEUES:
		retval = txgbe_get_vf_queues(adapter, msgbuf, vf);
		break;
	case TXGBE_VF_UPDATE_XCAST_MODE:
		retval = txgbe_update_vf_xcast_mode(adapter, msgbuf, vf);
		break;
	case TXGBE_VF_GET_LINK_STATE:
		retval = txgbe_get_vf_link_state(adapter, msgbuf, vf);
		break;
	case TXGBE_VF_GET_FW_VERSION:
		retval = txgbe_get_fw_version(adapter, msgbuf, vf);
		break;
	case TXGBE_VF_SET_5TUPLE:
		retval = txgbe_set_5tuple_filter_vf(adapter, msgbuf, vf);
		break;
	case TXGBE_VF_QUEUE_RATE_LIMIT:
		retval = txgbe_set_queue_rate_limit_vf(adapter, msgbuf, vf);
		break;
	case TXGBE_VF_BACKUP:
#ifdef CONFIG_PCI_IOV
		retval = txgbe_vf_backup(adapter, vf);
#endif
		break;
	default:
		e_err(drv, "Unhandled Msg %8.8x\n", msgbuf[0]);
		retval = TXGBE_ERR_MBX;
		break;
	}

	/* notify the VF of the results of what it sent us */
	if (retval)
		msgbuf[0] |= TXGBE_VT_MSGTYPE_NACK;
	else
		msgbuf[0] |= TXGBE_VT_MSGTYPE_ACK;

	msgbuf[0] |= TXGBE_VT_MSGTYPE_CTS;

	txgbe_write_mbx(hw, msgbuf, mbx_size, vf);

	return retval;
}

static void txgbe_rcv_ack_from_vf(struct txgbe_adapter *adapter, u16 vf)
{
	struct txgbe_hw *hw = &adapter->hw;
	u32 msg = TXGBE_VT_MSGTYPE_NACK;

	/* if device isn't clear to send it shouldn't be reading either */
	if (!adapter->vfinfo[vf].clear_to_send)
		txgbe_write_mbx(hw, &msg, 1, vf);
}

void txgbe_msg_task(struct txgbe_adapter *adapter)
{
	struct txgbe_hw *hw = &adapter->hw;
	u16 vf;

	for (vf = 0; vf < adapter->num_vfs; vf++) {
		/* process any reset requests */
		if (!txgbe_check_for_rst(hw, vf))
			txgbe_vf_reset_event(adapter, vf);

		/* process any messages pending */
		if (!txgbe_check_for_msg(hw, vf))
			txgbe_rcv_msg_from_vf(adapter, vf);

		/* process any acks */
		if (!txgbe_check_for_ack(hw, vf))
			txgbe_rcv_ack_from_vf(adapter, vf);
	}
}

void txgbe_disable_tx_rx(struct txgbe_adapter *adapter)
{
	struct txgbe_hw *hw = &adapter->hw;

	/* disable transmit and receive for all vfs */
	wr32(hw, TXGBE_TDM_VF_TE(0), 0);
	wr32(hw, TXGBE_TDM_VF_TE(1), 0);

	wr32(hw, TXGBE_RDM_VF_RE(0), 0);
	wr32(hw, TXGBE_RDM_VF_RE(1), 0);
}

static inline void txgbe_ping_vf(struct txgbe_adapter *adapter, int vf)
{
	struct txgbe_hw *hw = &adapter->hw;
	u32 ping;

	ping = TXGBE_PF_CONTROL_MSG;
	if (adapter->vfinfo[vf].clear_to_send)
		ping |= TXGBE_VT_MSGTYPE_CTS;
	txgbe_write_mbx(hw, &ping, 1, vf);
}

void txgbe_ping_all_vfs(struct txgbe_adapter *adapter)
{
	struct txgbe_hw *hw = &adapter->hw;
	u32 ping;
	u16 i;

	for (i = 0 ; i < adapter->num_vfs; i++) {
		ping = TXGBE_PF_CONTROL_MSG;
		if (adapter->vfinfo[i].clear_to_send)
			ping |= TXGBE_VT_MSGTYPE_CTS;
		txgbe_write_mbx(hw, &ping, 1, i);
	}
}

void txgbe_ping_vf_with_link_status(struct txgbe_adapter *adapter, bool link_up, u16 vf)
{
	struct txgbe_hw *hw = &adapter->hw;
	u32 msgbuf[2] = {0, 0};

	if (vf > adapter->num_vfs)
		return;

	msgbuf[0] = TXGBE_PF_NOFITY_VF_LINK_STATUS | TXGBE_PF_CONTROL_MSG;
	msgbuf[1] = (adapter->speed << 1) | link_up;
	//if (adapter->notify_down)
	//	msgbuf[1] |= TXGBE_PF_NOFITY_VF_NET_NOT_RUNNING;
	if (adapter->vfinfo[vf].clear_to_send)
		msgbuf[0] |= TXGBE_VT_MSGTYPE_CTS;
	txgbe_write_mbx(hw, msgbuf, 2, vf);
}

void txgbe_ping_all_vfs_with_link_status(struct txgbe_adapter *adapter, bool link_up)
{
	struct txgbe_hw *hw = &adapter->hw;
	u32 msgbuf[2] = {0, 0};
	u16 i;

	if (!adapter->num_vfs)
		return;

	msgbuf[0] = TXGBE_PF_NOFITY_VF_LINK_STATUS | TXGBE_PF_CONTROL_MSG;
	if (link_up)
		msgbuf[1] = (adapter->speed << 1) | link_up;
	//if (adapter->notify_down)
	//	msgbuf[1] |= TXGBE_PF_NOFITY_VF_NET_NOT_RUNNING;
	for (i = 0 ; i < adapter->num_vfs; i++) {
		if (adapter->vfinfo[i].clear_to_send)
			msgbuf[0] |= TXGBE_VT_MSGTYPE_CTS;
		txgbe_write_mbx(hw, msgbuf, 2, i);
	}
}

/**
 * txgbe_set_all_vfs - update vfs queues
 * @adapter: Pointer to adapter struct
 *
 * Update setting transmit and receive queues for all vfs
 **/
void txgbe_set_all_vfs(struct txgbe_adapter *adapter)
{
	int i;

	for (i = 0 ; i < adapter->num_vfs; i++) {
		txgbe_set_vf_link_state(adapter, i,
					adapter->vfinfo[i].link_state);
	}
}

int txgbe_ndo_set_vf_trust(struct net_device *netdev, int vf, bool setting)
{
	struct txgbe_adapter *adapter = netdev_priv(netdev);

	if (vf >= adapter->num_vfs)
		return -EINVAL;

	/* nothing to do */
	if (adapter->vfinfo[vf].trusted == setting)
		return 0;

	adapter->vfinfo[vf].trusted = setting;

	/* reset VF to reconfigure features */
	adapter->vfinfo[vf].clear_to_send = false;
	txgbe_ping_vf(adapter, vf);

	e_info(drv, "VF %u is %strusted\n", vf, setting ? "" : "not ");

	return 0;
}

static int txgbe_pci_sriov_enable(struct pci_dev __maybe_unused *dev,
				  int __maybe_unused num_vfs)
{
	int err = 0;
#ifdef CONFIG_PCI_IOV
	struct txgbe_adapter *adapter = pci_get_drvdata(dev);
	int i;
	int pre_existing_vfs = pci_num_vf(dev);

	if (!(adapter->flags & TXGBE_FLAG_SRIOV_CAPABLE)) {
		e_dev_warn("SRIOV not supported on this device\n");
		return -EOPNOTSUPP;
	}

	if (pre_existing_vfs && pre_existing_vfs != num_vfs)
		err = txgbe_disable_sriov(adapter);
	else if (pre_existing_vfs && pre_existing_vfs == num_vfs)
		goto out;

	if (err)
		goto err_out;

	/* While the SR-IOV capability structure reports total VFs to be
	 * 64 we limit the actual number that can be allocated to 63 so
	 * that some transmit/receive resources can be reserved to the
	 * PF.  The PCI bus driver already checks for other values out of
	 * range.
	 */
	if ((num_vfs + adapter->num_vmdqs) > TXGBE_MAX_VF_FUNCTIONS) {
		err = -EPERM;
		goto err_out;
	}

	err = __txgbe_enable_sriov(adapter, num_vfs);
	if (err)
		goto err_out;

	for (i = 0; i < adapter->num_vfs; i++)
		txgbe_vf_configuration(dev, (i | 0x10000000));

	/* reset before enabling SRIOV to avoid mailbox issues */
	txgbe_sriov_reinit(adapter);

	err = pci_enable_sriov(dev, num_vfs);
	if (err) {
		e_dev_warn("Failed to enable PCI sriov: %d\n", err);
		goto err_out;
	}
	txgbe_get_vfs(adapter);

out:
	return num_vfs;

err_out:
#endif

	return err;
}

static int txgbe_pci_sriov_disable(struct pci_dev *dev)
{
	struct txgbe_adapter *adapter = pci_get_drvdata(dev);
	int err;
#ifdef CONFIG_PCI_IOV
	u32 current_flags = adapter->flags;
#endif

	err = txgbe_disable_sriov(adapter);

	/* Only reinit if no error and state changed */
#ifdef CONFIG_PCI_IOV
	if (!err && current_flags != adapter->flags)
		txgbe_sriov_reinit(adapter);
#endif

	return err;
}

int txgbe_pci_sriov_configure(struct pci_dev *dev, int num_vfs)
{
	if (num_vfs == 0)
		return txgbe_pci_sriov_disable(dev);
	else
		return txgbe_pci_sriov_enable(dev, num_vfs);
}

int txgbe_ndo_set_vf_mac(struct net_device *netdev, int vf, u8 *mac)
{
	s32 retval = 0;
	struct txgbe_adapter *adapter = netdev_priv(netdev);

	if (vf < 0 || vf >= adapter->num_vfs)
		return -EINVAL;

	if (is_valid_ether_addr(mac)) {
		dev_info(pci_dev_to_dev(adapter->pdev),
			 "setting MAC %pM on VF %d\n", mac, vf);
		dev_info(pci_dev_to_dev(adapter->pdev),
			 "Reload the VF driver to make this change effective.\n");
		retval = txgbe_set_vf_mac(adapter, vf, mac);
		if (retval >= 0) {
			adapter->vfinfo[vf].pf_set_mac = true;
			if (test_bit(__TXGBE_DOWN, &adapter->state)) {
				dev_warn(pci_dev_to_dev(adapter->pdev),
					 "The VF MAC address has been set, but the PF device is not up.\n");
				dev_warn(pci_dev_to_dev(adapter->pdev),
					 "Bring the PF device up before attempting to use the VF device.\n");
			}
		} else {
			dev_warn(pci_dev_to_dev(adapter->pdev),
				 "The VF MAC address was NOT set due to invalid or duplicate MAC address.\n");
		}
	} else if (is_zero_ether_addr(mac)) {
		unsigned char *vf_mac_addr =
						adapter->vfinfo[vf].vf_mac_addresses;

		/* nothing to do */
		if (is_zero_ether_addr(vf_mac_addr))
			return 0;

		dev_info(pci_dev_to_dev(adapter->pdev), "removing MAC on VF %d\n",
			 vf);

		retval = txgbe_del_mac_filter(adapter, vf_mac_addr, vf);
		if (retval >= 0) {
			adapter->vfinfo[vf].pf_set_mac = false;
			memcpy(vf_mac_addr, mac, ETH_ALEN);
		} else {
			dev_warn(pci_dev_to_dev(adapter->pdev), "Could NOT remove the VF MAC address.\n");
		}
	} else {
		retval = -EINVAL;
	}
	return retval;
}

static int txgbe_enable_port_vlan(struct txgbe_adapter *adapter,
				  int vf, u16 vlan, u8 qos, __be16 vlan_proto)
{
	struct txgbe_hw *hw = &adapter->hw;
	int err;

	err = txgbe_set_vf_vlan(adapter, true, vlan, vf);
	if (err)
		goto out;
	txgbe_set_vmvir(adapter, vlan, qos, vf, vlan_proto);
	txgbe_set_vmolr(hw, vf, false);
	if (adapter->vfinfo[vf].spoofchk_enabled)
		hw->mac.ops.set_vlan_anti_spoofing(hw, true, vf);
	adapter->vfinfo[vf].vlan_count++;
	/* enable hide vlan */
	txgbe_write_qde(adapter, vf, 1);
	txgbe_write_hide_vlan(adapter, vf, 1);
	adapter->vfinfo[vf].pf_vlan = vlan;
	adapter->vfinfo[vf].pf_qos = qos;
	adapter->vfinfo[vf].vlan_proto = vlan_proto;
	dev_info(pci_dev_to_dev(adapter->pdev),
		 "Setting VLAN %d, QOS 0x%x on VF %d\n", vlan, qos, vf);
	if (test_bit(__TXGBE_DOWN, &adapter->state)) {
		dev_warn(pci_dev_to_dev(adapter->pdev),
			 "The VF VLAN has been set, but the PF device is not up.\n");
		dev_warn(pci_dev_to_dev(adapter->pdev),
			 "Bring the PF device up before attempting to use the VF device.\n");
	}

out:
	return err;
}

static int txgbe_disable_port_vlan(struct txgbe_adapter *adapter, int vf)
{
	struct txgbe_hw *hw = &adapter->hw;
	int err;

	err = txgbe_set_vf_vlan(adapter, false,
				adapter->vfinfo[vf].pf_vlan, vf);
	txgbe_clear_vmvir(adapter, vf);
	txgbe_set_vmolr(hw, vf, true);
	hw->mac.ops.set_vlan_anti_spoofing(hw, false, vf);
	if (adapter->vfinfo[vf].vlan_count)
		adapter->vfinfo[vf].vlan_count--;
	/* disable hide vlan */
	txgbe_write_hide_vlan(adapter, vf, 0);
	adapter->vfinfo[vf].pf_vlan = 0;
	adapter->vfinfo[vf].pf_qos = 0;
	adapter->vfinfo[vf].vlan_proto = 0;

	return err;
}

int txgbe_ndo_set_vf_vlan(struct net_device *netdev, int vf, u16 vlan,
			  u8 qos, __be16 vlan_proto)
{
	int err = 0;
	struct txgbe_adapter *adapter = netdev_priv(netdev);

	/* VLAN IDs accepted range 0-4094 */
	if (vf >= adapter->num_vfs || (vlan > VLAN_VID_MASK - 1) || qos > 7)
		return -EINVAL;

	if (vlan_proto != htons(ETH_P_8021Q) && vlan_proto != htons(ETH_P_8021AD))
		return -EPROTONOSUPPORT;

	if (vlan || qos) {
		/* Check if there is already a port VLAN set, if so
		 * we have to delete the old one first before we
		 * can set the new one.  The usage model had
		 * previously assumed the user would delete the
		 * old port VLAN before setting a new one but this
		 * is not necessarily the case.
		 */
		if (adapter->vfinfo[vf].pf_vlan)
			err = txgbe_disable_port_vlan(adapter, vf);
		if (err)
			goto out;
		err = txgbe_enable_port_vlan(adapter, vf, vlan, qos, vlan_proto);
	} else {
		err = txgbe_disable_port_vlan(adapter, vf);
	}
out:
	return err;
}

int txgbe_link_mbps(struct txgbe_adapter *adapter)
{
	switch (adapter->link_speed) {
	case TXGBE_LINK_SPEED_40GB_FULL:
		return 40000;
	case TXGBE_LINK_SPEED_25GB_FULL:
		return 25000;
	case TXGBE_LINK_SPEED_10GB_FULL:
		return 10000;
	case TXGBE_LINK_SPEED_1GB_FULL:
		return 1000;
	default:
		return 0;
	}
}

u16 txgbe_frac_to_bi(u16 frac, u16 denom, int max_bits)
{
	u16 value = 0;

	while (frac > 0 && max_bits > 0) {
		max_bits -= 1;
		frac *= 2;
		if (frac >= denom) {
			value |= BIT(max_bits);
			frac -= denom;
		}
	}

	return value;
}

static void txgbe_set_vf_rate_limit(struct txgbe_adapter *adapter, int vf)
{
	struct txgbe_ring_feature *vmdq = &adapter->ring_feature[RING_F_VMDQ];
	struct txgbe_hw *hw = &adapter->hw;
	u32 bcnrc_val;
	int factor_int;
	int factor_fra;
	int link_speed;
	u16 queue, queues_per_pool;
	u16 max_tx_rate = adapter->vfinfo[vf].max_tx_rate;

	/* determine how many queues per pool based on VMDq mask */
	queues_per_pool = __ALIGN_MASK(1, ~vmdq->mask);

	/* Set global transmit compensation time to the MMW_SIZE in RTTBCNRM
	 * register. Typically MMW_SIZE=0x014 if 9728-byte jumbo is supported
	 * and 0x004 otherwise.
	 */
	wr32(hw, TXGBE_TDM_MMW, 0x14);

	if (hw->mac.type == txgbe_mac_aml || hw->mac.type == txgbe_mac_aml40) {
		if (max_tx_rate) {
			u16 frac;

			link_speed = adapter->vf_rate_link_speed / 1000 * 1024;

			/* Calculate the rate factor values to set */
			factor_int = link_speed / max_tx_rate;
			frac = (link_speed % max_tx_rate) * 10000 / max_tx_rate;
			factor_fra = txgbe_frac_to_bi(frac, 10000, 14);

			wr32(hw, TXGBE_TDM_RL_VM_IDX, vf);
			wr32m(hw, TXGBE_TDM_RL_VM_CFG,
			      TXGBE_TDM_FACTOR_INT_MASK,
				factor_int << TXGBE_TDM_FACTOR_INT_SHIFT);
			wr32m(hw, TXGBE_TDM_RL_VM_CFG,
			      TXGBE_TDM_FACTOR_FRA_MASK,
				factor_fra << TXGBE_TDM_FACTOR_FRA_SHIFT);
			wr32m(hw, TXGBE_TDM_RL_VM_CFG,
			      TXGBE_TDM_RL_EN, TXGBE_TDM_RL_EN);
		} else {
			wr32(hw, TXGBE_TDM_RL_VM_IDX, vf);
			wr32m(hw, TXGBE_TDM_RL_VM_CFG,
			      TXGBE_TDM_RL_EN, 0);
		}
	} else {
		max_tx_rate /= queues_per_pool;
		bcnrc_val = TXGBE_TDM_RP_RATE_MAX(max_tx_rate);

		/* write value for all Tx queues belonging to VF */
		for (queue = 0; queue < queues_per_pool; queue++) {
			unsigned int reg_idx = (vf * queues_per_pool) + queue;

			wr32(hw, TXGBE_TDM_RP_IDX, reg_idx);
			wr32(hw, TXGBE_TDM_RP_RATE, bcnrc_val);
			if (max_tx_rate)
				wr32m(hw, TXGBE_TDM_RP_CTL,
				      TXGBE_TDM_RP_CTL_RLEN, TXGBE_TDM_RP_CTL_RLEN);
			else
				wr32m(hw, TXGBE_TDM_RP_CTL,
				      TXGBE_TDM_RP_CTL_RLEN, 0);
		}
	}
}

void txgbe_check_vf_rate_limit(struct txgbe_adapter *adapter)
{
	int i;

	/* VF Tx rate limit was not set */
	if (!adapter->vf_rate_link_speed)
		return;

	if (txgbe_link_mbps(adapter) != adapter->vf_rate_link_speed) {
		adapter->vf_rate_link_speed = 0;
		dev_info(pci_dev_to_dev(adapter->pdev),
			 "Link speed has been changed. VF Transmit rate is disabled\n");
	}

	for (i = 0; i < adapter->num_vfs; i++) {
		if (!adapter->vf_rate_link_speed)
			adapter->vfinfo[i].max_tx_rate = 0;

		txgbe_set_vf_rate_limit(adapter, i);
	}
}

static int
txgbe_set_queue_rate_limit_vf(struct txgbe_adapter *adapter,
			      u32 *msgbuf, u32 vf)
{
	struct txgbe_ring_feature *vmdq = &adapter->ring_feature[RING_F_VMDQ];
	struct txgbe_hw *hw = &adapter->hw;
	u16 queue, queues_per_pool, max_tx_rate;
	int factor_int, factor_fra, link_speed;
	u32 reg_idx;

	if (hw->mac.type != txgbe_mac_aml)
		return -EOPNOTSUPP;

	/* verify the PF is supporting the correct API */
	if (adapter->vfinfo[vf].vf_api < txgbe_mbox_api_22)
		return -EOPNOTSUPP;

	/* determine how many queues per pool based on VMDq mask */
	queues_per_pool = __ALIGN_MASK(1, ~vmdq->mask);

	queue = msgbuf[TXGBEVF_Q_RATE_INDEX];
	max_tx_rate = msgbuf[TXGBEVF_Q_RATE_LIMIT];

	/* convert queue index on each vf to the global index */
	reg_idx = (vf * queues_per_pool) + queue;

	/* Set global transmit compensation time to the MMW_SIZE in RTTBCNRM
	 * register. Typically MMW_SIZE=0x014 if 9728-byte jumbo is supported
	 * and 0x004 otherwise.
	 */
	wr32(hw, TXGBE_TDM_MMW, 0x14);

	if (max_tx_rate) {
		u16 frac;

		link_speed = txgbe_link_mbps(adapter) / 1000 * 1024;

		/* Calculate the rate factor values to set */
		factor_int = link_speed / max_tx_rate;
		frac = (link_speed % max_tx_rate) * 10000 / max_tx_rate;
		factor_fra = txgbe_frac_to_bi(frac, 10000, 14);

		wr32(hw, TXGBE_TDM_RL_QUEUE_IDX, reg_idx);
		wr32m(hw, TXGBE_TDM_RL_QUEUE_CFG,
		      TXGBE_TDM_FACTOR_INT_MASK, factor_int << TXGBE_TDM_FACTOR_INT_SHIFT);
		wr32m(hw, TXGBE_TDM_RL_QUEUE_CFG,
		      TXGBE_TDM_FACTOR_FRA_MASK, factor_fra << TXGBE_TDM_FACTOR_FRA_SHIFT);
		wr32m(hw, TXGBE_TDM_RL_QUEUE_CFG,
		      TXGBE_TDM_RL_EN, TXGBE_TDM_RL_EN);
	} else {
		wr32(hw, TXGBE_TDM_RL_QUEUE_IDX, reg_idx);
		wr32m(hw, TXGBE_TDM_RL_QUEUE_CFG,
		      TXGBE_TDM_RL_EN, 0);
	}

	adapter->vfinfo[vf].queue_max_tx_rate[queue] = max_tx_rate;
	e_info(drv, "set vf %d queue %d max_tx_rate to %d Mbps",
	       vf, queue, max_tx_rate);

	return 0;
}

int txgbe_ndo_set_vf_bw(struct net_device *netdev,
			int vf,
			int min_tx_rate,
			int max_tx_rate)
{
	struct txgbe_adapter *adapter = netdev_priv(netdev);
	int link_speed;

	/* verify VF is active */
	if (vf >= adapter->num_vfs)
		return -EINVAL;

	/* verify link is up */
	if (!adapter->link_up)
		return -EINVAL;

	/* verify we are linked at 1 or 10 Gbps */
	if (adapter->link_speed < TXGBE_LINK_SPEED_1GB_FULL)
		return -EINVAL;

	link_speed = txgbe_link_mbps(adapter);
	/* rate limit cannot be less than 10Mbs or greater than link speed */
	if (max_tx_rate && (max_tx_rate <= 10 || max_tx_rate > link_speed))
		return -EINVAL;

	/* store values */
	adapter->vfinfo[vf].min_tx_rate = min_tx_rate;
	adapter->vf_rate_link_speed = link_speed;
	adapter->vfinfo[vf].max_tx_rate = max_tx_rate;

	/* update hardware configuration */
	txgbe_set_vf_rate_limit(adapter, vf);

	return 0;
}

int txgbe_ndo_set_vf_spoofchk(struct net_device *netdev, int vf, bool setting)
{
	struct txgbe_adapter *adapter = netdev_priv(netdev);
	struct txgbe_hw *hw = &adapter->hw;
	u32 regval;

	if (vf >= adapter->num_vfs)
		return -EINVAL;

	adapter->vfinfo[vf].spoofchk_enabled = setting;

	if (vf < 32) {
		regval = (setting << vf);
		wr32m(hw, TXGBE_TDM_MAC_AS_L,
		      regval | (1 << vf), regval);

		if (adapter->vfinfo[vf].vlan_count) {
			wr32m(hw, TXGBE_TDM_VLAN_AS_L,
			      regval | (1 << vf), regval);
		}
	} else {
		regval = (setting << (vf - 32));
		wr32m(hw, TXGBE_TDM_MAC_AS_H,
		      regval | (1 << (vf - 32)), regval);

		if (adapter->vfinfo[vf].vlan_count) {
			wr32m(hw, TXGBE_TDM_VLAN_AS_H,
			      regval | (1 << (vf - 32)), regval);
		}
	}
	return 0;
}

/**
 * txgbe_set_vf_rx_tx - Set VF rx tx
 * @adapter: Pointer to adapter struct
 * @vf: VF identifier
 *
 * Set or reset correct transmit and receive for vf
 **/
static void txgbe_set_vf_rx_tx(struct txgbe_adapter *adapter, int vf)
{
	struct txgbe_hw *hw = &adapter->hw;
	u32 reg_cur_tx, reg_cur_rx, reg_req_tx, reg_req_rx;
	u32 reg_offset, vf_shift;

	vf_shift = vf % 32;
	reg_offset = vf / 32;

	reg_cur_tx = rd32(hw, TXGBE_TDM_VF_TE(reg_offset));
	reg_cur_rx = rd32(hw, TXGBE_RDM_VF_RE(reg_offset));

	if (adapter->vfinfo[vf].link_enable) {
		reg_req_tx = reg_cur_tx | 1 << vf_shift;
		reg_req_rx = reg_cur_rx | 1 << vf_shift;
		/* Enable particular VF */
		if (reg_cur_tx != reg_req_tx)
			wr32(hw, TXGBE_TDM_VF_TE(reg_offset), reg_req_tx);
		if (reg_cur_rx != reg_req_rx)
			wr32(hw, TXGBE_RDM_VF_RE(reg_offset), reg_req_rx);
	} else {
		reg_req_tx = 1 << vf_shift;
		reg_req_rx = 1 << vf_shift;
		/* Disable particular VF */
		if (reg_cur_tx & reg_req_tx)
			wr32(hw, TXGBE_TDM_VFTE_CLR(reg_offset), reg_req_tx);
		if (reg_cur_rx & reg_req_rx)
			wr32(hw, TXGBE_RDM_VFRE_CLR(reg_offset), reg_req_rx);
	}
	if (adapter->vfinfo[vf].link_state == IFLA_VF_LINK_STATE_ENABLE &&
	    !(rd32(hw, TXGBE_MAC_TX_CFG) & TXGBE_MAC_TX_CFG_TE)) {
		wr32m(hw, TXGBE_MAC_TX_CFG, TXGBE_MAC_TX_CFG_TE,
		      TXGBE_MAC_TX_CFG_TE);
		TXGBE_WRITE_FLUSH(hw);
		wr32m(hw, TXGBE_MAC_TX_CFG, TXGBE_MAC_TX_CFG_TE,
		      TXGBE_MAC_TX_CFG_TE);
	}
}

/**
 * txgbe_set_vf_link_state - Set link state
 * @adapter: Pointer to adapter struct
 * @vf: VF identifier
 * @state: required link state
 *
 * Set a link force state on/off a single vf
 **/
void txgbe_set_vf_link_state(struct txgbe_adapter *adapter, int vf, int state)
{
	bool link_up = adapter->link_up;

	adapter->vfinfo[vf].link_state = state;

	switch (state) {
	case TXGBE_VF_LINK_STATE_AUTO:
		if (test_bit(__TXGBE_DOWN, &adapter->state)) {
			adapter->vfinfo[vf].link_enable = false;
		} else {
			link_up = adapter->link_up;
			adapter->vfinfo[vf].link_enable = true;
		}
		break;
	case TXGBE_VF_LINK_STATE_ENABLE:
		adapter->vfinfo[vf].link_enable = true;
		link_up = true;
		break;
	case TXGBE_VF_LINK_STATE_DISABLE:
		adapter->vfinfo[vf].link_enable = false;
		link_up = false;
		break;
	}

	/* restart the VF */
	adapter->vfinfo[vf].clear_to_send = false;
	txgbe_ping_vf(adapter, vf);

	txgbe_ping_vf_with_link_status(adapter, link_up, vf);

	txgbe_set_vf_rx_tx(adapter, vf);
}

/**
 * txgbe_ndo_set_vf_link_state - Set link state
 * @netdev: network interface device structure
 * @vf: VF identifier
 * @state: required link state
 *
 * Set the link state of a specified VF, regardless of physical link state
 **/
int txgbe_ndo_set_vf_link_state(struct net_device *netdev, int vf, int state)
{
	struct txgbe_adapter *adapter = netdev_priv(netdev);

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
			 "NDO set VF %d link state enable\n", vf);
		txgbe_set_vf_link_state(adapter, vf, TXGBE_VF_LINK_STATE_ENABLE);
		break;
	case IFLA_VF_LINK_STATE_DISABLE:
		dev_info(pci_dev_to_dev(adapter->pdev),
			 "NDO set VF %d link state disable\n", vf);
		txgbe_set_vf_link_state(adapter, vf, TXGBE_VF_LINK_STATE_DISABLE);
		break;
	case IFLA_VF_LINK_STATE_AUTO:
		dev_info(pci_dev_to_dev(adapter->pdev),
			 "NDO set VF %d link state auto\n", vf);
		txgbe_set_vf_link_state(adapter, vf, TXGBE_VF_LINK_STATE_AUTO);
		break;
	default:
		dev_err(pci_dev_to_dev(adapter->pdev),
			"NDO set VF %d - invalid link state %d\n", vf, state);
		ret = -EINVAL;
	}
out:
	return ret;
}

int txgbe_trans_vf_link_state(int state)
{
	switch (state) {
	case TXGBE_VF_LINK_STATE_ENABLE:
		return IFLA_VF_LINK_STATE_ENABLE;
	case TXGBE_VF_LINK_STATE_DISABLE:
		return IFLA_VF_LINK_STATE_DISABLE;
	case TXGBE_VF_LINK_STATE_AUTO:
		return IFLA_VF_LINK_STATE_AUTO;
	}
	return IFLA_VF_LINK_STATE_AUTO;
}

int txgbe_ndo_get_vf_config(struct net_device *netdev,
			    int vf, struct ifla_vf_info *ivi)
{
	struct txgbe_adapter *adapter = netdev_priv(netdev);

	if (vf >= adapter->num_vfs)
		return -EINVAL;
	ivi->vf = vf;
	memcpy(&ivi->mac, adapter->vfinfo[vf].vf_mac_addresses, ETH_ALEN);

	ivi->max_tx_rate = adapter->vfinfo[vf].max_tx_rate;
	ivi->min_tx_rate = adapter->vfinfo[vf].min_tx_rate;

	ivi->vlan = adapter->vfinfo[vf].pf_vlan;
	ivi->qos = adapter->vfinfo[vf].pf_qos;
	ivi->vlan_proto = adapter->vfinfo[vf].vlan_proto;
	ivi->spoofchk = adapter->vfinfo[vf].spoofchk_enabled;
	ivi->trusted = adapter->vfinfo[vf].trusted;
	ivi->linkstate = txgbe_trans_vf_link_state(adapter->vfinfo[vf].link_state);

	return 0;
}

