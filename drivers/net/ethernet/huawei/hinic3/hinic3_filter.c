// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include "ossl_knl.h"
#include "hinic3_hw.h"
#include "hinic3_crm.h"
#include "hinic3_nic_dev.h"
#include "hinic3_srv_nic.h"

static unsigned char set_filter_state = 1;
module_param(set_filter_state, byte, 0444);
MODULE_PARM_DESC(set_filter_state, "Set mac filter config state: 0 - disable, 1 - enable (default=1)");

static int hinic3_uc_sync(struct net_device *netdev, u8 *addr)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);

	return hinic3_set_mac(nic_dev->hwdev, addr, 0,
			      hinic3_global_func_id(nic_dev->hwdev),
			      HINIC3_CHANNEL_NIC);
}

static int hinic3_uc_unsync(struct net_device *netdev, u8 *addr)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);

	/* The addr is in use */
	if (ether_addr_equal(addr, netdev->dev_addr))
		return 0;

	return hinic3_del_mac(nic_dev->hwdev, addr, 0,
			      hinic3_global_func_id(nic_dev->hwdev),
			      HINIC3_CHANNEL_NIC);
}

void hinic3_clean_mac_list_filter(struct hinic3_nic_dev *nic_dev)
{
	struct net_device *netdev = nic_dev->netdev;
	struct hinic3_mac_filter *ftmp = NULL;
	struct hinic3_mac_filter *f = NULL;

	list_for_each_entry_safe(f, ftmp, &nic_dev->uc_filter_list, list) {
		if (f->state == HINIC3_MAC_HW_SYNCED)
			hinic3_uc_unsync(netdev, f->addr);
		list_del(&f->list);
		kfree(f);
	}

	list_for_each_entry_safe(f, ftmp, &nic_dev->mc_filter_list, list) {
		if (f->state == HINIC3_MAC_HW_SYNCED)
			hinic3_uc_unsync(netdev, f->addr);
		list_del(&f->list);
		kfree(f);
	}
}

static struct hinic3_mac_filter *hinic3_find_mac(const struct list_head *filter_list,
						 u8 *addr)
{
	struct hinic3_mac_filter *f = NULL;

	list_for_each_entry(f, filter_list, list) {
		if (ether_addr_equal(addr, f->addr))
			return f;
	}
	return NULL;
}

static struct hinic3_mac_filter *hinic3_add_filter(struct hinic3_nic_dev *nic_dev,
						   struct list_head *mac_filter_list,
						   u8 *addr)
{
	struct hinic3_mac_filter *f = NULL;

	f = kzalloc(sizeof(*f), GFP_ATOMIC);
	if (!f)
		goto out;

	ether_addr_copy(f->addr, addr);

	INIT_LIST_HEAD(&f->list);
	list_add_tail(&f->list, mac_filter_list);

	f->state = HINIC3_MAC_WAIT_HW_SYNC;
	set_bit(HINIC3_MAC_FILTER_CHANGED, &nic_dev->flags);

out:
	return f;
}

static void hinic3_del_filter(struct hinic3_nic_dev *nic_dev,
			      struct hinic3_mac_filter *f)
{
	set_bit(HINIC3_MAC_FILTER_CHANGED, &nic_dev->flags);

	if (f->state == HINIC3_MAC_WAIT_HW_SYNC) {
		/* have not added to hw, delete it directly */
		list_del(&f->list);
		kfree(f);
		return;
	}

	f->state = HINIC3_MAC_WAIT_HW_UNSYNC;
}

static struct hinic3_mac_filter *hinic3_mac_filter_entry_clone(const struct hinic3_mac_filter *src)
{
	struct hinic3_mac_filter *f = NULL;

	f = kzalloc(sizeof(*f), GFP_ATOMIC);
	if (!f)
		return NULL;

	*f = *src;
	INIT_LIST_HEAD(&f->list);

	return f;
}

static void hinic3_undo_del_filter_entries(struct list_head *filter_list,
					   const struct list_head *from)
{
	struct hinic3_mac_filter *ftmp = NULL;
	struct hinic3_mac_filter *f = NULL;

	list_for_each_entry_safe(f, ftmp, from, list) {
		if (hinic3_find_mac(filter_list, f->addr))
			continue;

		if (f->state == HINIC3_MAC_HW_SYNCED)
			f->state = HINIC3_MAC_WAIT_HW_UNSYNC;

		list_move_tail(&f->list, filter_list);
	}
}

static void hinic3_undo_add_filter_entries(struct list_head *filter_list,
					   const struct list_head *from)
{
	struct hinic3_mac_filter *ftmp = NULL;
	struct hinic3_mac_filter *tmp = NULL;
	struct hinic3_mac_filter *f = NULL;

	list_for_each_entry_safe(f, ftmp, from, list) {
		tmp = hinic3_find_mac(filter_list, f->addr);
		if (tmp && tmp->state == HINIC3_MAC_HW_SYNCED)
			tmp->state = HINIC3_MAC_WAIT_HW_SYNC;
	}
}

static void hinic3_cleanup_filter_list(const struct list_head *head)
{
	struct hinic3_mac_filter *ftmp = NULL;
	struct hinic3_mac_filter *f = NULL;

	list_for_each_entry_safe(f, ftmp, head, list) {
		list_del(&f->list);
		kfree(f);
	}
}

static int hinic3_mac_filter_sync_hw(struct hinic3_nic_dev *nic_dev,
				     struct list_head *del_list,
				     struct list_head *add_list)
{
	struct net_device *netdev = nic_dev->netdev;
	struct hinic3_mac_filter *ftmp = NULL;
	struct hinic3_mac_filter *f = NULL;
	int err = 0, add_count = 0;

	if (!list_empty(del_list)) {
		list_for_each_entry_safe(f, ftmp, del_list, list) {
			err = hinic3_uc_unsync(netdev, f->addr);
			if (err) { /* ignore errors when delete mac */
				nic_err(&nic_dev->pdev->dev, "Failed to delete mac\n");
			}

			list_del(&f->list);
			kfree(f);
		}
	}

	if (!list_empty(add_list)) {
		list_for_each_entry_safe(f, ftmp, add_list, list) {
			err = hinic3_uc_sync(netdev, f->addr);
			if (err) {
				nic_err(&nic_dev->pdev->dev, "Failed to add mac\n");
				return err;
			}

			add_count++;
			list_del(&f->list);
			kfree(f);
		}
	}

	return add_count;
}

static int hinic3_mac_filter_sync(struct hinic3_nic_dev *nic_dev,
				  struct list_head *mac_filter_list, bool uc)
{
	struct net_device *netdev = nic_dev->netdev;
	struct list_head tmp_del_list, tmp_add_list;
	struct hinic3_mac_filter *fclone = NULL;
	struct hinic3_mac_filter *ftmp = NULL;
	struct hinic3_mac_filter *f = NULL;
	int err = 0, add_count = 0;

	INIT_LIST_HEAD(&tmp_del_list);
	INIT_LIST_HEAD(&tmp_add_list);

	list_for_each_entry_safe(f, ftmp, mac_filter_list, list) {
		if (f->state != HINIC3_MAC_WAIT_HW_UNSYNC)
			continue;

		f->state = HINIC3_MAC_HW_UNSYNCED;
		list_move_tail(&f->list, &tmp_del_list);
	}

	list_for_each_entry_safe(f, ftmp, mac_filter_list, list) {
		if (f->state != HINIC3_MAC_WAIT_HW_SYNC)
			continue;

		fclone = hinic3_mac_filter_entry_clone(f);
		if (!fclone) {
			err = -ENOMEM;
			break;
		}

		f->state = HINIC3_MAC_HW_SYNCED;
		list_add_tail(&fclone->list, &tmp_add_list);
	}

	if (err) {
		hinic3_undo_del_filter_entries(mac_filter_list, &tmp_del_list);
		hinic3_undo_add_filter_entries(mac_filter_list, &tmp_add_list);
		nicif_err(nic_dev, drv, netdev, "Failed to clone mac_filter_entry\n");

		hinic3_cleanup_filter_list(&tmp_del_list);
		hinic3_cleanup_filter_list(&tmp_add_list);
		return -ENOMEM;
	}

	add_count = hinic3_mac_filter_sync_hw(nic_dev, &tmp_del_list,
					      &tmp_add_list);
	if (list_empty(&tmp_add_list))
		return add_count;

	/* there are errors when add mac to hw, delete all mac in hw */
	hinic3_undo_add_filter_entries(mac_filter_list, &tmp_add_list);
	/* VF don't support to enter promisc mode,
	 * so we can't delete any other uc mac
	 */
	if (!HINIC3_FUNC_IS_VF(nic_dev->hwdev) || !uc) {
		list_for_each_entry_safe(f, ftmp, mac_filter_list, list) {
			if (f->state != HINIC3_MAC_HW_SYNCED)
				continue;

			fclone = hinic3_mac_filter_entry_clone(f);
			if (!fclone)
				break;

			f->state = HINIC3_MAC_WAIT_HW_SYNC;
			list_add_tail(&fclone->list, &tmp_del_list);
		}
	}

	hinic3_cleanup_filter_list(&tmp_add_list);
	hinic3_mac_filter_sync_hw(nic_dev, &tmp_del_list, &tmp_add_list);

	/* need to enter promisc/allmulti mode */
	return -ENOMEM;
}

static void hinic3_mac_filter_sync_all(struct hinic3_nic_dev *nic_dev)
{
	struct net_device *netdev = nic_dev->netdev;
	int add_count;

	if (test_bit(HINIC3_MAC_FILTER_CHANGED, &nic_dev->flags)) {
		clear_bit(HINIC3_MAC_FILTER_CHANGED, &nic_dev->flags);
		add_count = hinic3_mac_filter_sync(nic_dev,
						   &nic_dev->uc_filter_list,
						   true);
		if (add_count < 0 && HINIC3_SUPPORT_PROMISC(nic_dev->hwdev)) {
			set_bit(HINIC3_PROMISC_FORCE_ON,
				&nic_dev->rx_mod_state);
			nicif_info(nic_dev, drv, netdev, "Promisc mode forced on\n");
		} else if (add_count) {
			clear_bit(HINIC3_PROMISC_FORCE_ON,
				  &nic_dev->rx_mod_state);
		}

		add_count = hinic3_mac_filter_sync(nic_dev,
						   &nic_dev->mc_filter_list,
						   false);
		if (add_count < 0 && HINIC3_SUPPORT_ALLMULTI(nic_dev->hwdev)) {
			set_bit(HINIC3_ALLMULTI_FORCE_ON,
				&nic_dev->rx_mod_state);
			nicif_info(nic_dev, drv, netdev, "All multicast mode forced on\n");
		} else if (add_count) {
			clear_bit(HINIC3_ALLMULTI_FORCE_ON,
				  &nic_dev->rx_mod_state);
		}
	}
}

#define HINIC3_DEFAULT_RX_MODE	(NIC_RX_MODE_UC | NIC_RX_MODE_MC | \
				NIC_RX_MODE_BC)

static void hinic3_update_mac_filter(struct hinic3_nic_dev *nic_dev,
				     const struct netdev_hw_addr_list *src_list,
				     struct list_head *filter_list)
{
	struct hinic3_mac_filter *filter = NULL;
	struct hinic3_mac_filter *ftmp = NULL;
	struct hinic3_mac_filter *f = NULL;
	struct netdev_hw_addr *ha = NULL;

	/* add addr if not already in the filter list */
	netif_addr_lock_bh(nic_dev->netdev);
	netdev_hw_addr_list_for_each(ha, src_list) {
		filter = hinic3_find_mac(filter_list, ha->addr);
		if (!filter)
			hinic3_add_filter(nic_dev, filter_list, ha->addr);
		else if (filter->state == HINIC3_MAC_WAIT_HW_UNSYNC)
			filter->state = HINIC3_MAC_HW_SYNCED;
	}
	netif_addr_unlock_bh(nic_dev->netdev);

	/* delete addr if not in netdev list */
	list_for_each_entry_safe(f, ftmp, filter_list, list) {
		bool found = false;

		netif_addr_lock_bh(nic_dev->netdev);
		netdev_hw_addr_list_for_each(ha, src_list)
			if (ether_addr_equal(ha->addr, f->addr)) {
				found = true;
				break;
			}
		netif_addr_unlock_bh(nic_dev->netdev);

		if (found)
			continue;

		hinic3_del_filter(nic_dev, f);
	}
}

#ifndef NETDEV_HW_ADDR_T_MULTICAST
static void hinic3_update_mc_filter(struct hinic3_nic_dev *nic_dev,
				    struct list_head *filter_list)
{
	struct hinic3_mac_filter *filter = NULL;
	struct hinic3_mac_filter *ftmp = NULL;
	struct hinic3_mac_filter *f = NULL;
	struct dev_mc_list *ha = NULL;

	/* add addr if not already in the filter list */
	netif_addr_lock_bh(nic_dev->netdev);
	netdev_for_each_mc_addr(ha, nic_dev->netdev) {
		filter = hinic3_find_mac(filter_list, ha->da_addr);
		if (!filter)
			hinic3_add_filter(nic_dev, filter_list, ha->da_addr);
		else if (filter->state == HINIC3_MAC_WAIT_HW_UNSYNC)
			filter->state = HINIC3_MAC_HW_SYNCED;
	}
	netif_addr_unlock_bh(nic_dev->netdev);
	/* delete addr if not in netdev list */
	list_for_each_entry_safe(f, ftmp, filter_list, list) {
		bool found = false;

		netif_addr_lock_bh(nic_dev->netdev);
		netdev_for_each_mc_addr(ha, nic_dev->netdev)
			if (ether_addr_equal(ha->da_addr, f->addr)) {
				found = true;
				break;
			}
		netif_addr_unlock_bh(nic_dev->netdev);

		if (found)
			continue;

		hinic3_del_filter(nic_dev, f);
	}
}
#endif

static void update_mac_filter(struct hinic3_nic_dev *nic_dev)
{
	struct net_device *netdev = nic_dev->netdev;

	if (test_and_clear_bit(HINIC3_UPDATE_MAC_FILTER, &nic_dev->flags)) {
		hinic3_update_mac_filter(nic_dev, &netdev->uc,
					 &nic_dev->uc_filter_list);
		/* FPGA mc only 12 entry, default disable mc */
		if (set_filter_state) {
#ifdef NETDEV_HW_ADDR_T_MULTICAST
			hinic3_update_mac_filter(nic_dev, &netdev->mc,
						 &nic_dev->mc_filter_list);
#else
			hinic3_update_mc_filter(nic_dev,
						&nic_dev->mc_filter_list);
#endif
		}
	}
}

static void sync_rx_mode_to_hw(struct hinic3_nic_dev *nic_dev, int promisc_en,
			       int allmulti_en)
{
	struct net_device *netdev = nic_dev->netdev;
	u32 rx_mod = HINIC3_DEFAULT_RX_MODE;
	int err;

	rx_mod |= (promisc_en ? NIC_RX_MODE_PROMISC : 0);
	rx_mod |= (allmulti_en ? NIC_RX_MODE_MC_ALL : 0);

	if (promisc_en != test_bit(HINIC3_HW_PROMISC_ON,
				   &nic_dev->rx_mod_state))
		nicif_info(nic_dev, drv, netdev,
			   "%s promisc mode\n",
			   promisc_en ? "Enter" : "Left");
	if (allmulti_en !=
	    test_bit(HINIC3_HW_ALLMULTI_ON, &nic_dev->rx_mod_state))
		nicif_info(nic_dev, drv, netdev,
			   "%s all_multi mode\n",
			   allmulti_en ? "Enter" : "Left");

	err = hinic3_set_rx_mode(nic_dev->hwdev, rx_mod);
	if (err) {
		nicif_err(nic_dev, drv, netdev, "Failed to set rx_mode\n");
		return;
	}

	promisc_en ? set_bit(HINIC3_HW_PROMISC_ON, &nic_dev->rx_mod_state) :
		clear_bit(HINIC3_HW_PROMISC_ON, &nic_dev->rx_mod_state);

	allmulti_en ? set_bit(HINIC3_HW_ALLMULTI_ON, &nic_dev->rx_mod_state) :
		clear_bit(HINIC3_HW_ALLMULTI_ON, &nic_dev->rx_mod_state);
}

void hinic3_set_rx_mode_work(struct work_struct *work)
{
	struct hinic3_nic_dev *nic_dev =
			container_of(work, struct hinic3_nic_dev, rx_mode_work);
	struct net_device *netdev = nic_dev->netdev;
	int promisc_en = 0, allmulti_en = 0;

	update_mac_filter(nic_dev);

	hinic3_mac_filter_sync_all(nic_dev);

	if (HINIC3_SUPPORT_PROMISC(nic_dev->hwdev))
		promisc_en = !!(netdev->flags & IFF_PROMISC) ||
			test_bit(HINIC3_PROMISC_FORCE_ON,
				 &nic_dev->rx_mod_state);

	if (HINIC3_SUPPORT_ALLMULTI(nic_dev->hwdev))
		allmulti_en = !!(netdev->flags & IFF_ALLMULTI) ||
			test_bit(HINIC3_ALLMULTI_FORCE_ON,
				 &nic_dev->rx_mod_state);

	if (promisc_en !=
	    test_bit(HINIC3_HW_PROMISC_ON, &nic_dev->rx_mod_state) ||
	    allmulti_en !=
	    test_bit(HINIC3_HW_ALLMULTI_ON, &nic_dev->rx_mod_state))
		sync_rx_mode_to_hw(nic_dev, promisc_en, allmulti_en);
}

