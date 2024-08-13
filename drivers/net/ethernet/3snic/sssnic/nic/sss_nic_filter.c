// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

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

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_nic_dev_define.h"
#include "sss_nic_cfg.h"

enum sss_nic_rx_mode_state {
	SSSNIC_PROMISC_ON,
	SSSNIC_ALLMULTI_ON,
	SSSNIC_PROMISC_FORCE_ON,
	SSSNIC_ALLMULTI_FORCE_ON,
};

enum sss_nic_mac_filter_state {
	SSSNIC_MAC_FILTER_WAIT_SYNC,
	SSSNIC_MAC_FILTER_SYNCED,
	SSSNIC_MAC_FILTER_WAIT_UNSYNC,
	SSSNIC_MAC_FILTER_UNSYNCED,
};

struct sss_nic_mac_filter {
	struct list_head list;
	u8 address[ETH_ALEN];
	unsigned long status;
};

#define SSSNIC_DEFAULT_RX_MODE	(SSSNIC_RX_MODE_UC | SSSNIC_RX_MODE_MC | SSSNIC_RX_MODE_BC)

static bool mc_mac_filter = true;
module_param(mc_mac_filter, bool, 0444);
MODULE_PARM_DESC(mc_mac_filter, "Set multicast mac filter: 0 - disable, 1 - enable (default=1)");

static int sss_nic_sync_uc(struct net_device *netdev, u8 *address)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	return sss_nic_set_mac(nic_dev, address, 0,
			       sss_get_global_func_id(nic_dev->hwdev), SSS_CHANNEL_NIC);
}

static int sss_nic_unsync_uc(struct net_device *netdev, u8 *address)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	/* The addr is in use */
	if (ether_addr_equal(address, netdev->dev_addr))
		return 0;

	return sss_nic_del_mac(nic_dev, address, 0,
			       sss_get_global_func_id(nic_dev->hwdev), SSS_CHANNEL_NIC);
}

void sss_nic_clean_mac_list_filter(struct sss_nic_dev *nic_dev)
{
	struct sss_nic_mac_filter *filter = NULL;
	struct sss_nic_mac_filter *tmp_filter = NULL;
	struct net_device *netdev = nic_dev->netdev;

	list_for_each_entry_safe(filter, tmp_filter, &nic_dev->uc_filter_list, list) {
		if (filter->status == SSSNIC_MAC_FILTER_SYNCED)
			sss_nic_unsync_uc(netdev, filter->address);
		list_del(&filter->list);
		kfree(filter);
	}

	list_for_each_entry_safe(filter, tmp_filter, &nic_dev->mc_filter_list, list) {
		if (filter->status == SSSNIC_MAC_FILTER_SYNCED)
			sss_nic_unsync_uc(netdev, filter->address);
		list_del(&filter->list);
		kfree(filter);
	}
}

static struct sss_nic_mac_filter *sss_nic_find_mac(const struct list_head *filter_list,
						   u8 *address)
{
	struct sss_nic_mac_filter *filter = NULL;

	list_for_each_entry(filter, filter_list, list) {
		if (ether_addr_equal(address, filter->address))
			return filter;
	}
	return NULL;
}

static struct sss_nic_mac_filter *sss_nic_add_filter(struct sss_nic_dev *nic_dev,
						     struct list_head *mac_filter_list,
		u8 *address)
{
	struct sss_nic_mac_filter *filter;

	filter = kzalloc(sizeof(*filter), GFP_ATOMIC);
	if (!filter)
		goto out;

	ether_addr_copy(filter->address, address);

	INIT_LIST_HEAD(&filter->list);
	list_add_tail(&filter->list, mac_filter_list);

	filter->status = SSSNIC_MAC_FILTER_WAIT_SYNC;
	set_bit(SSSNIC_MAC_FILTER_CHANGED, &nic_dev->flags);

out:
	return filter;
}

static void sss_nic_del_filter(struct sss_nic_dev *nic_dev,
			       struct sss_nic_mac_filter *filter)
{
	set_bit(SSSNIC_MAC_FILTER_CHANGED, &nic_dev->flags);

	if (filter->status == SSSNIC_MAC_FILTER_WAIT_SYNC) {
		/* have not added to hw, delete it directly */
		list_del(&filter->list);
		kfree(filter);
		return;
	}

	filter->status = SSSNIC_MAC_FILTER_WAIT_UNSYNC;
}

static struct sss_nic_mac_filter *sss_nic_copy_mac_filter_entry(const struct sss_nic_mac_filter *ft)
{
	struct sss_nic_mac_filter *filter;

	filter = kzalloc(sizeof(*filter), GFP_ATOMIC);
	if (!filter)
		return NULL;

	*filter = *ft;
	INIT_LIST_HEAD(&filter->list);

	return filter;
}

static void sss_nic_undo_del_filter_entry(struct list_head *filter_list,
					  const struct list_head *from)
{
	struct sss_nic_mac_filter *filter = NULL;
	struct sss_nic_mac_filter *tmp_filter = NULL;

	list_for_each_entry_safe(filter, tmp_filter, from, list) {
		if (sss_nic_find_mac(filter_list, filter->address))
			continue;

		if (filter->status == SSSNIC_MAC_FILTER_SYNCED)
			filter->status = SSSNIC_MAC_FILTER_WAIT_UNSYNC;

		list_move_tail(&filter->list, filter_list);
	}
}

static void sss_nic_undo_add_filter_entry(struct list_head *filter_list,
					  const struct list_head *from)
{
	struct sss_nic_mac_filter *filter = NULL;
	struct sss_nic_mac_filter *tmp_filter = NULL;
	struct sss_nic_mac_filter *ftmp_filter = NULL;

	list_for_each_entry_safe(filter, ftmp_filter, from, list) {
		tmp_filter = sss_nic_find_mac(filter_list, filter->address);
		if (tmp_filter && tmp_filter->status == SSSNIC_MAC_FILTER_SYNCED)
			tmp_filter->status = SSSNIC_MAC_FILTER_WAIT_SYNC;
	}
}

static void sss_nic_cleanup_filter_list(const struct list_head *head)
{
	struct sss_nic_mac_filter *filter = NULL;
	struct sss_nic_mac_filter *ftmp_filter = NULL;

	list_for_each_entry_safe(filter, ftmp_filter, head, list) {
		list_del(&filter->list);
		kfree(filter);
	}
}

static int sss_nic_sync_mac_filter_to_hw(struct sss_nic_dev *nic_dev,
					 struct list_head *del_list,
		struct list_head *add_list)
{
	struct sss_nic_mac_filter *filter = NULL;
	struct sss_nic_mac_filter *ftmp_filter = NULL;
	struct net_device *netdev = nic_dev->netdev;
	int ret = 0;
	int add_num = 0;

	if (!list_empty(del_list)) {
		list_for_each_entry_safe(filter, ftmp_filter, del_list, list) {
			ret = sss_nic_unsync_uc(netdev, filter->address);
			if (ret != 0) { /* ignore errors when delete mac */
				nic_err(nic_dev->dev_hdl, "Fail to delete mac\n");
			}

			list_del(&filter->list);
			kfree(filter);
		}
	}

	if (!list_empty(add_list)) {
		list_for_each_entry_safe(filter, ftmp_filter, add_list, list) {
			ret = sss_nic_sync_uc(netdev, filter->address);
			if (ret != 0) {
				nic_err(nic_dev->dev_hdl, "Fail to add mac\n");
				return ret;
			}

			add_num++;
			list_del(&filter->list);
			kfree(filter);
		}
	}

	return add_num;
}

static int sss_nic_sync_mac_filter(struct sss_nic_dev *nic_dev,
				   struct list_head *mac_filter_list, bool uc)
{
	struct net_device *netdev = nic_dev->netdev;
	struct list_head del_tmp_list;
	struct list_head add_tmp_list;
	struct sss_nic_mac_filter *filter = NULL;
	struct sss_nic_mac_filter *ftmp_filter = NULL;
	struct sss_nic_mac_filter *fclone_filter = NULL;
	int ret = 0;
	int add_num = 0;

	INIT_LIST_HEAD(&del_tmp_list);
	INIT_LIST_HEAD(&add_tmp_list);

	list_for_each_entry_safe(filter, ftmp_filter, mac_filter_list, list) {
		if (filter->status != SSSNIC_MAC_FILTER_WAIT_UNSYNC)
			continue;

		filter->status = SSSNIC_MAC_FILTER_UNSYNCED;
		list_move_tail(&filter->list, &del_tmp_list);
	}

	list_for_each_entry_safe(filter, ftmp_filter, mac_filter_list, list) {
		if (filter->status != SSSNIC_MAC_FILTER_WAIT_SYNC)
			continue;

		fclone_filter = sss_nic_copy_mac_filter_entry(filter);
		if (!fclone_filter) {
			ret = -ENOMEM;
			break;
		}

		filter->status = SSSNIC_MAC_FILTER_SYNCED;
		list_add_tail(&fclone_filter->list, &add_tmp_list);
	}

	if (ret != 0) {
		sss_nic_undo_del_filter_entry(mac_filter_list, &del_tmp_list);
		sss_nic_undo_add_filter_entry(mac_filter_list, &add_tmp_list);
		nicif_err(nic_dev, drv, netdev, "Fail to clone mac_filter_entry\n");

		sss_nic_cleanup_filter_list(&del_tmp_list);
		sss_nic_cleanup_filter_list(&add_tmp_list);
		return -ENOMEM;
	}

	add_num = sss_nic_sync_mac_filter_to_hw(nic_dev, &del_tmp_list, &add_tmp_list);
	if (list_empty(&add_tmp_list))
		return add_num;

	/* there are errors when add mac to hw, delete all mac in hw */
	sss_nic_undo_add_filter_entry(mac_filter_list, &add_tmp_list);
	/* VF don't support to enter promisc mode,
	 * so we can't delete any other uc mac
	 */
	if (!SSSNIC_FUNC_IS_VF(nic_dev->hwdev) || !uc) {
		list_for_each_entry_safe(filter, ftmp_filter, mac_filter_list, list) {
			if (filter->status != SSSNIC_MAC_FILTER_SYNCED)
				continue;

			fclone_filter = sss_nic_copy_mac_filter_entry(filter);
			if (!fclone_filter)
				break;

			filter->status = SSSNIC_MAC_FILTER_WAIT_SYNC;
			list_add_tail(&fclone_filter->list, &del_tmp_list);
		}
	}

	sss_nic_cleanup_filter_list(&add_tmp_list);
	sss_nic_sync_mac_filter_to_hw(nic_dev, &del_tmp_list, &add_tmp_list);

	/* need to enter promisc/allmulti mode */
	return -ENOMEM;
}

static void sss_nic_sync_all_mac_filter(struct sss_nic_dev *nic_dev)
{
	struct net_device *netdev = nic_dev->netdev;
	int add_num;

	if (test_bit(SSSNIC_MAC_FILTER_CHANGED, &nic_dev->flags)) {
		clear_bit(SSSNIC_MAC_FILTER_CHANGED, &nic_dev->flags);
		add_num = sss_nic_sync_mac_filter(nic_dev, &nic_dev->uc_filter_list, true);
		if (add_num < 0 && SSSNIC_SUPPORT_PROMISC(nic_dev->nic_io)) {
			set_bit(SSSNIC_PROMISC_FORCE_ON, &nic_dev->rx_mode);
			nicif_info(nic_dev, drv, netdev, " Force promisc mode on\n");
		} else if (add_num != 0) {
			clear_bit(SSSNIC_PROMISC_FORCE_ON, &nic_dev->rx_mode);
		}

		add_num = sss_nic_sync_mac_filter(nic_dev, &nic_dev->mc_filter_list, false);
		if (add_num < 0 && SSSNIC_SUPPORT_ALLMULTI(nic_dev->nic_io)) {
			set_bit(SSSNIC_ALLMULTI_FORCE_ON, &nic_dev->rx_mode);
			nicif_info(nic_dev, drv, netdev, "Force allmulti mode on\n");
		} else if (add_num != 0) {
			clear_bit(SSSNIC_ALLMULTI_FORCE_ON, &nic_dev->rx_mode);
		}
	}
}

static void sss_nic_update_mac_filter(struct sss_nic_dev *nic_dev,
				      const struct netdev_hw_addr_list *src_list,
				      struct list_head *filter_list)
{
	struct sss_nic_mac_filter *filter = NULL;
	struct sss_nic_mac_filter *ftmp_filter = NULL;
	struct sss_nic_mac_filter *f_filter = NULL;
	struct netdev_hw_addr *hw_addr = NULL;

	/* add addr if not already in the filter list */
	netif_addr_lock_bh(nic_dev->netdev);
	netdev_hw_addr_list_for_each(hw_addr, src_list) {
		filter = sss_nic_find_mac(filter_list, hw_addr->addr);
		if (!filter)
			sss_nic_add_filter(nic_dev, filter_list, hw_addr->addr);
		else if (filter->status == SSSNIC_MAC_FILTER_WAIT_UNSYNC)
			filter->status = SSSNIC_MAC_FILTER_SYNCED;
	}
	netif_addr_unlock_bh(nic_dev->netdev);

	/* delete addr if not in netdev list */
	list_for_each_entry_safe(f_filter, ftmp_filter, filter_list, list) {
		bool find = false;

		netif_addr_lock_bh(nic_dev->netdev);
		netdev_hw_addr_list_for_each(hw_addr, src_list)
			if (ether_addr_equal(hw_addr->addr, f_filter->address)) {
				find = true;
				break;
			}
		netif_addr_unlock_bh(nic_dev->netdev);

		if (find)
			continue;

		sss_nic_del_filter(nic_dev, f_filter);
	}
}

#ifndef NETDEV_HW_ADDR_T_MULTICAST
static void sss_nic_update_mc_filter(struct sss_nic_dev *nic_dev,
				     struct list_head *filter_list)
{
	struct sss_nic_mac_filter *filter = NULL;
	struct sss_nic_mac_filter *ftmp_filter = NULL;
	struct sss_nic_mac_filter *f_filter = NULL;
	struct dev_mc_list *hw_addr = NULL;

	/* add addr if not already in the filter list */
	netif_addr_lock_bh(nic_dev->netdev);
	netdev_for_each_mc_addr(hw_addr, nic_dev->netdev) {
		filter = sss_nic_find_mac(filter_list, hw_addr->da_addr);
		if (!filter)
			sss_nic_add_filter(nic_dev, filter_list, hw_addr->da_addr);
		else if (filter->status == SSSNIC_MAC_FILTER_WAIT_UNSYNC)
			filter->status = SSSNIC_MAC_FILTER_SYNCED;
	}
	netif_addr_unlock_bh(nic_dev->netdev);
	/* delete addr if not in netdev list */
	list_for_each_entry_safe(f_filter, ftmp_filter, filter_list, list) {
		bool find = false;

		netif_addr_lock_bh(nic_dev->netdev);
		netdev_for_each_mc_addr(hw_addr, nic_dev->netdev)
			if (ether_addr_equal(hw_addr->da_addr, f_filter->address)) {
				find = true;
				break;
			}
		netif_addr_unlock_bh(nic_dev->netdev);

		if (find)
			continue;

		sss_nic_del_filter(nic_dev, f_filter);
	}
}
#endif

static void sss_nic_update_all_mac_filter(struct sss_nic_dev *nic_dev)
{
	struct net_device *netdev = nic_dev->netdev;

	if (test_and_clear_bit(SSSNIC_UPDATE_MAC_FILTER, &nic_dev->flags)) {
		sss_nic_update_mac_filter(nic_dev, &netdev->uc,
					  &nic_dev->uc_filter_list);
		if (mc_mac_filter) {
#ifdef NETDEV_HW_ADDR_T_MULTICAST
			sss_nic_update_mac_filter(nic_dev, &netdev->mc, &nic_dev->mc_filter_list);
#else
			sss_nic_update_mc_filter(nic_dev, &nic_dev->mc_filter_list);
#endif
		}
	}
}

static void sss_nic_sync_rx_mode_to_hw(struct sss_nic_dev *nic_dev, int allmulti_enter,
				       int promisc_enter)
{
	int ret;
	u32 rx_mode = SSSNIC_DEFAULT_RX_MODE;
	struct net_device *netdev = nic_dev->netdev;

	rx_mode |= (allmulti_enter ? SSSNIC_RX_MODE_MC_ALL : 0);
	rx_mode |= (promisc_enter ? SSSNIC_RX_MODE_PROMISC : 0);

	if (allmulti_enter !=
	    test_bit(SSSNIC_ALLMULTI_ON, &nic_dev->rx_mode))
		nicif_info(nic_dev, drv, netdev,
			   "%s allmulti mode\n",
			   allmulti_enter ? "Enable" : "Disable");

	if (promisc_enter != test_bit(SSSNIC_PROMISC_ON,
				      &nic_dev->rx_mode))
		nicif_info(nic_dev, drv, netdev,
			   "%s promisc mode\n",
			   promisc_enter ? "Enable" : "Disable");

	ret = sss_nic_set_rx_mode(nic_dev, rx_mode);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to set rx mode\n");
		return;
	}

	if (allmulti_enter != 0)
		set_bit(SSSNIC_ALLMULTI_ON, &nic_dev->rx_mode);
	else
		clear_bit(SSSNIC_ALLMULTI_ON, &nic_dev->rx_mode);

	if (promisc_enter != 0)
		set_bit(SSSNIC_PROMISC_ON, &nic_dev->rx_mode);
	else
		clear_bit(SSSNIC_PROMISC_ON, &nic_dev->rx_mode);
}

void sss_nic_set_rx_mode_work(struct work_struct *work)
{
	struct sss_nic_dev *nic_dev =
		container_of(work, struct sss_nic_dev, rx_mode_work);
	struct net_device *netdev = nic_dev->netdev;
	int allmulti_enter = 0;
	int promisc_enter = 0;

	sss_nic_update_all_mac_filter(nic_dev);

	sss_nic_sync_all_mac_filter(nic_dev);

	if (SSSNIC_SUPPORT_ALLMULTI(nic_dev->nic_io))
		allmulti_enter = !!(netdev->flags & IFF_ALLMULTI) ||
				 test_bit(SSSNIC_ALLMULTI_FORCE_ON,
					  &nic_dev->rx_mode);

	if (SSSNIC_SUPPORT_PROMISC(nic_dev->nic_io))
		promisc_enter = !!(netdev->flags & IFF_PROMISC) ||
				test_bit(SSSNIC_PROMISC_FORCE_ON,
					 &nic_dev->rx_mode);

	if (allmulti_enter !=
	    test_bit(SSSNIC_ALLMULTI_ON, &nic_dev->rx_mode) ||
	    promisc_enter !=
	    test_bit(SSSNIC_PROMISC_ON, &nic_dev->rx_mode))
		sss_nic_sync_rx_mode_to_hw(nic_dev, allmulti_enter, promisc_enter);
}
