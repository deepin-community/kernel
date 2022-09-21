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
 *
 * based on ixgbe_procfs.h, Copyright(c) 1999 - 2017 Intel Corporation.
 * Contact Information:
 * Linux NICS <linux.nics@intel.com>
 * e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 */


#include "txgbe.h"
#include "txgbe_hw.h"
#include "txgbe_type.h"

#ifdef TXGBE_PROCFS
#ifndef TXGBE_SYSFS

#include <linux/module.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/netdevice.h>

static struct proc_dir_entry *txgbe_top_dir;

static struct net_device_stats *procfs_get_stats(struct net_device *netdev)
{
#ifndef HAVE_NETDEV_STATS_IN_NETDEV
	struct txgbe_adapter *adapter;
#endif
	if (netdev == NULL)
		return NULL;

#ifdef HAVE_NETDEV_STATS_IN_NETDEV
	/* only return the current stats */
	return &netdev->stats;
#else
	adapter = netdev_priv(netdev);

	/* only return the current stats */
	return &adapter->net_stats;
#endif /* HAVE_NETDEV_STATS_IN_NETDEV */
}

static int txgbe_fwbanner(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	return snprintf(page, count, "%s\n", adapter->eeprom_id);
}

static int txgbe_porttype(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	return snprintf(page, count, "%d\n",
			test_bit(__TXGBE_DOWN, &adapter->state));
}

static int txgbe_portspeed(char *page, char __always_unused **start,
			   off_t __always_unused off, int count,
			   int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	int speed = 0;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	switch (adapter->link_speed) {
	case TXGBE_LINK_SPEED_100_FULL:
		speed = 1;
		break;
	case TXGBE_LINK_SPEED_1GB_FULL:
		speed = 10;
		break;
	case TXGBE_LINK_SPEED_10GB_FULL:
		speed = 100;
		break;
	default:
		break;
	}
	return snprintf(page, count, "%d\n", speed);
}

static int txgbe_wqlflag(char *page, char __always_unused **start,
			 off_t __always_unused off, int count,
			 int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	return snprintf(page, count, "%d\n", adapter->wol);
}

static int txgbe_xflowctl(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	struct txgbe_hw *hw;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");

	return snprintf(page, count, "%d\n", hw->fc.current_mode);
}

static int txgbe_rxdrops(char *page, char __always_unused **start,
			 off_t __always_unused off, int count,
			 int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	net_stats  = procfs_get_stats(adapter->netdev);
	if (net_stats == NULL)
		return snprintf(page, count, "error: no net stats\n");

	return snprintf(page, count, "%lu\n",
			net_stats->rx_dropped);
}

static int txgbe_rxerrors(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	net_stats  = procfs_get_stats(adapter->netdev);
	if (net_stats == NULL)
		return snprintf(page, count, "error: no net stats\n");

	return snprintf(page, count, "%lu\n", net_stats->rx_errors);
}

static int txgbe_rxupacks(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct txgbe_hw *hw;
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");

	return snprintf(page, count, "%d\n", rd32(hw, TXGBE_TPR));
}

static int txgbe_rxmpacks(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct txgbe_hw *hw;
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	int i, mprc = 0;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");
	for (i = 0; i < 128; i++)
		mprc += rd32(hw, TXGBE_PX_MPRC(i));
	return snprintf(page, count, "%d\n", mprc);
}

static int txgbe_rxbpacks(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct txgbe_hw *hw;
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");

	return snprintf(page, count, "%d\n",
			rd32(hw, TXGBE_RX_BC_FRAMES_GOOD_LOW));
}

static int txgbe_txupacks(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct txgbe_hw *hw;
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");

	return snprintf(page, count, "%d\n",
			rd32(hw, TXGBE_TX_FRAME_CNT_GOOD_BAD_LOW));
}

static int txgbe_txmpacks(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct txgbe_hw *hw;
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");

	return snprintf(page, count, "%d\n",
			rd32(hw, TXGBE_TX_MC_FRAMES_GOOD_LOW));
}

static int txgbe_txbpacks(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct txgbe_hw *hw;
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");

	return snprintf(page, count, "%d\n",
			rd32(hw, TXGBE_TX_BC_FRAMES_GOOD_LOW));
}

static int txgbe_txerrors(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	net_stats  = procfs_get_stats(adapter->netdev);
	if (net_stats == NULL)
		return snprintf(page, count, "error: no net stats\n");

	return snprintf(page, count, "%lu\n",
			net_stats->tx_errors);
}

static int txgbe_txdrops(char *page, char __always_unused **start,
			 off_t __always_unused off, int count,
			 int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	net_stats  = procfs_get_stats(adapter->netdev);
	if (net_stats == NULL)
		return snprintf(page, count, "error: no net stats\n");

	return snprintf(page, count, "%lu\n",
			net_stats->tx_dropped);
}

static int txgbe_rxframes(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	net_stats  = procfs_get_stats(adapter->netdev);
	if (net_stats == NULL)
		return snprintf(page, count, "error: no net stats\n");

	return snprintf(page, count, "%lu\n",
			net_stats->rx_packets);
}

static int txgbe_rxbytes(char *page, char __always_unused **start,
			 off_t __always_unused off, int count,
			 int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	net_stats  = procfs_get_stats(adapter->netdev);
	if (net_stats == NULL)
		return snprintf(page, count, "error: no net stats\n");

	return snprintf(page, count, "%lu\n",
			net_stats->rx_bytes);
}

static int txgbe_txframes(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	net_stats  = procfs_get_stats(adapter->netdev);
	if (net_stats == NULL)
		return snprintf(page, count, "error: no net stats\n");

	return snprintf(page, count, "%lu\n",
			net_stats->tx_packets);
}

static int txgbe_txbytes(char *page, char __always_unused **start,
			 off_t __always_unused off, int count,
			 int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	net_stats  = procfs_get_stats(adapter->netdev);
	if (net_stats == NULL)
		return snprintf(page, count, "error: no net stats\n");

	return snprintf(page, count, "%lu\n",
			net_stats->tx_bytes);
}

static int txgbe_linkstat(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct txgbe_hw *hw;
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	int bitmask = 0;
	u32 link_speed;
	bool link_up = false;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");

	if (!test_bit(__TXGBE_DOWN, &adapter->state))
		bitmask |= 1;

	if (TCALL(hw, mac.ops.check_link, &link_speed, &link_up, false)
		/* always assume link is up, if no check link function */
		link_up = true;
	if (link_up)
		bitmask |= 2;

	if (adapter->old_lsc != adapter->lsc_int) {
		bitmask |= 4;
		adapter->old_lsc = adapter->lsc_int;
	}

	return snprintf(page, count, "0x%X\n", bitmask);
}

static int txgbe_funcid(char *page, char __always_unused **start,
			off_t __always_unused off, int count,
			int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	struct txgbe_hw *hw;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");

	return snprintf(page, count, "0x%X\n", hw->bus.func);
}

static int txgbe_funcvers(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void __always_unused *data)
{
	return snprintf(page, count, "%s\n", txgbe_driver_version);
}

static int txgbe_macburn(char *page, char __always_unused **start,
			 off_t __always_unused off, int count,
			 int __always_unused *eof, void *data)
{
	struct txgbe_hw *hw;
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");

	return snprintf(page, count, "0x%02X%02X%02X%02X%02X%02X\n",
		       (unsigned int)hw->mac.perm_addr[0],
		       (unsigned int)hw->mac.perm_addr[1],
		       (unsigned int)hw->mac.perm_addr[2],
		       (unsigned int)hw->mac.perm_addr[3],
		       (unsigned int)hw->mac.perm_addr[4],
		       (unsigned int)hw->mac.perm_addr[5]);
}

static int txgbe_macadmn(char *page, char __always_unused **start,
			 off_t __always_unused off, int count,
			 int __always_unused *eof, void *data)
{
	struct txgbe_hw *hw;
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");

	return snprintf(page, count, "0x%02X%02X%02X%02X%02X%02X\n",
		       (unsigned int)hw->mac.addr[0],
		       (unsigned int)hw->mac.addr[1],
		       (unsigned int)hw->mac.addr[2],
		       (unsigned int)hw->mac.addr[3],
		       (unsigned int)hw->mac.addr[4],
		       (unsigned int)hw->mac.addr[5]);
}

static int txgbe_maclla1(char *page, char __always_unused **start,
			 off_t __always_unused off, int count,
			 int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	struct txgbe_hw *hw;
	int rc;
	u16 eeprom_buff[6];
	u16 first_word = 0x37;
	const u16 word_count = ARRAY_SIZE(eeprom_buff);

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");

	rc = TCALL(hw, eeprom.ops.read_buffer, first_word, 1, &first_word);
	if (rc != 0)
		return snprintf(page, count,
				"error: reading pointer to the EEPROM\n");

	if (first_word != 0x0000 && first_word != 0xFFFF) {
		rc = TCALL(hw, eeprom.ops.read_buffer, first_word, word_count,
					eeprom_buff);
		if (rc != 0)
			return snprintf(page, count, "error: reading buffer\n");
	} else {
		memset(eeprom_buff, 0, sizeof(eeprom_buff));
	}

	switch (hw->bus.func) {
	case 0:
		return snprintf(page, count, "0x%04X%04X%04X\n",
				eeprom_buff[0],
				eeprom_buff[1],
				eeprom_buff[2]);
	case 1:
		return snprintf(page, count, "0x%04X%04X%04X\n",
				eeprom_buff[3],
				eeprom_buff[4],
				eeprom_buff[5]);
	}
	return snprintf(page, count, "unexpected port %d\n", hw->bus.func);
}

static int txgbe_mtusize(char *page, char __always_unused **start,
			 off_t __always_unused off, int count,
			 int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	struct net_device *netdev;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	netdev = adapter->netdev;
	if (netdev == NULL)
		return snprintf(page, count, "error: no net device\n");

	return snprintf(page, count, "%d\n", netdev->mtu);
}

static int txgbe_featflag(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	int bitmask = 0;
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	struct net_device *netdev;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	netdev = adapter->netdev;
	if (netdev == NULL)
		return snprintf(page, count, "error: no net device\n");
	if (adapter->netdev->features & NETIF_F_RXCSUM)
		bitmask |= 1;
	return snprintf(page, count, "%d\n", bitmask);
}

static int txgbe_lsominct(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void __always_unused *data)
{
	return snprintf(page, count, "%d\n", 1);
}

static int txgbe_prommode(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	struct net_device *netdev;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	netdev = adapter->netdev;
	if (netdev == NULL)
		return snprintf(page, count, "error: no net device\n");

	return snprintf(page, count, "%d\n",
			netdev->flags & IFF_PROMISC);
}

static int txgbe_txdscqsz(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	return snprintf(page, count, "%d\n", adapter->tx_ring[0]->count);
}

static int txgbe_rxdscqsz(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	return snprintf(page, count, "%d\n", adapter->rx_ring[0]->count);
}

static int txgbe_rxqavg(char *page, char __always_unused **start,
			off_t __always_unused off, int count,
			int __always_unused *eof, void *data)
{
	int index;
	int diff = 0;
	u16 ntc;
	u16 ntu;
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	for (index = 0; index < adapter->num_rx_queues; index++) {
		ntc = adapter->rx_ring[index]->next_to_clean;
		ntu = adapter->rx_ring[index]->next_to_use;

		if (ntc >= ntu)
			diff += (ntc - ntu);
		else
			diff += (adapter->rx_ring[index]->count - ntu + ntc);
	}
	if (adapter->num_rx_queues <= 0)
		return snprintf(page, count,
				"can't calculate, number of queues %d\n",
				adapter->num_rx_queues);
	return snprintf(page, count, "%d\n", diff/adapter->num_rx_queues);
}

static int txgbe_txqavg(char *page, char __always_unused **start,
			off_t __always_unused off, int count,
			int __always_unused *eof, void *data)
{
	int index;
	int diff = 0;
	u16 ntc;
	u16 ntu;
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	for (index = 0; index < adapter->num_tx_queues; index++) {
		ntc = adapter->tx_ring[index]->next_to_clean;
		ntu = adapter->tx_ring[index]->next_to_use;

		if (ntc >= ntu)
			diff += (ntc - ntu);
		else
			diff += (adapter->tx_ring[index]->count - ntu + ntc);
	}
	if (adapter->num_tx_queues <= 0)
		return snprintf(page, count,
				"can't calculate, number of queues %d\n",
				adapter->num_tx_queues);
	return snprintf(page, count, "%d\n",
			diff/adapter->num_tx_queues);
}

static int txgbe_iovotype(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void __always_unused *data)
{
	return snprintf(page, count, "2\n");
}

static int txgbe_funcnbr(char *page, char __always_unused **start,
			 off_t __always_unused off, int count,
			 int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	return snprintf(page, count, "%d\n", adapter->num_vfs);
}

static int txgbe_pciebnbr(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	return snprintf(page, count, "%d\n", adapter->pdev->bus->number);
}

static int txgbe_therm_dealarmthresh(char *page, char __always_unused **start,
				   off_t __always_unused off, int count,
				   int __always_unused *eof, void *data)
{
	struct txgbe_therm_proc_data *therm_data =
		(struct txgbe_therm_proc_data *)data;

	if (therm_data == NULL)
		return snprintf(page, count, "error: no therm_data\n");

	return snprintf(page, count, "%d\n",
			therm_data->sensor_data->dalarm_thresh);
}


static int txgbe_therm_alarmthresh(char *page, char __always_unused **start,
				     off_t __always_unused off, int count,
				     int __always_unused *eof, void *data)
{
	struct txgbe_therm_proc_data *therm_data =
		(struct txgbe_therm_proc_data *)data;

	if (therm_data == NULL)
		return snprintf(page, count, "error: no therm_data\n");

	return snprintf(page, count, "%d\n",
			therm_data->sensor_data->alarm_thresh);
}

static int txgbe_therm_temp(char *page, char __always_unused **start,
			    off_t __always_unused off, int count,
			    int __always_unused *eof, void *data)
{
	s32 status;
	struct txgbe_therm_proc_data *therm_data =
		(struct txgbe_therm_proc_data *)data;

	if (therm_data == NULL)
		return snprintf(page, count, "error: no therm_data\n");

	status = txgbe_get_thermal_sensor_data(therm_data->hw);
	if (status != 0)
		snprintf(page, count, "error: status %d returned\n", status);

	return snprintf(page, count, "%d\n", therm_data->sensor_data->temp);
}


struct txgbe_proc_type {
	char name[32];
	int (*read)(char*, char**, off_t, int, int*, void*);
};

struct txgbe_proc_type txgbe_proc_entries[] = {
	{"fwbanner", &txgbe_fwbanner},
	{"porttype", &txgbe_porttype},
	{"portspeed", &txgbe_portspeed},
	{"wqlflag", &txgbe_wqlflag},
	{"xflowctl", &txgbe_xflowctl},
	{"rxdrops", &txgbe_rxdrops},
	{"rxerrors", &txgbe_rxerrors},
	{"rxupacks", &txgbe_rxupacks},
	{"rxmpacks", &txgbe_rxmpacks},
	{"rxbpacks", &txgbe_rxbpacks},
	{"txdrops", &txgbe_txdrops},
	{"txerrors", &txgbe_txerrors},
	{"txupacks", &txgbe_txupacks},
	{"txmpacks", &txgbe_txmpacks},
	{"txbpacks", &txgbe_txbpacks},
	{"rxframes", &txgbe_rxframes},
	{"rxbytes", &txgbe_rxbytes},
	{"txframes", &txgbe_txframes},
	{"txbytes", &txgbe_txbytes},
	{"linkstat", &txgbe_linkstat},
	{"funcid", &txgbe_funcid},
	{"funcvers", &txgbe_funcvers},
	{"macburn", &txgbe_macburn},
	{"macadmn", &txgbe_macadmn},
	{"maclla1", &txgbe_maclla1},
	{"mtusize", &txgbe_mtusize},
	{"featflag", &txgbe_featflag},
	{"lsominct", &txgbe_lsominct},
	{"prommode", &txgbe_prommode},
	{"txdscqsz", &txgbe_txdscqsz},
	{"rxdscqsz", &txgbe_rxdscqsz},
	{"txqavg", &txgbe_txqavg},
	{"rxqavg", &txgbe_rxqavg},
	{"iovotype", &txgbe_iovotype},
	{"funcnbr", &txgbe_funcnbr},
	{"pciebnbr", &txgbe_pciebnbr},
	{"", NULL}
};

struct txgbe_proc_type txgbe_internal_entries[] = {
	{"temp", &txgbe_therm_temp},
	{"alarmthresh", &txgbe_therm_alarmthresh},
	{"dealarmthresh", &txgbe_therm_dealarmthresh},
	{"", NULL}
};

void txgbe_del_proc_entries(struct txgbe_adapter *adapter)
{
	int index;
	int i;
	char buf[16];   /* much larger than the sensor number will ever be */

	if (txgbe_top_dir == NULL)
		return;

	for (i = 0; i < TXGBE_MAX_SENSORS; i++) {
		if (adapter->therm_dir[i] == NULL)
			continue;

		for (index = 0; ; index++) {
			if (txgbe_internal_entries[index].read == NULL)
				break;

			 remove_proc_entry(txgbe_internal_entries[index].name,
					   adapter->therm_dir[i]);
		}
		snprintf(buf, sizeof(buf), "sensor_%d", i);
		remove_proc_entry(buf, adapter->info_dir);
	}

	if (adapter->info_dir != NULL) {
		for (index = 0; ; index++) {
			if (txgbe_proc_entries[index].read == NULL)
				break;
			remove_proc_entry(txgbe_proc_entries[index].name,
					  adapter->info_dir);
		}
		remove_proc_entry("info", adapter->eth_dir);
	}

	if (adapter->eth_dir != NULL)
		remove_proc_entry(pci_name(adapter->pdev), txgbe_top_dir);
}

/* called from txgbe_main.c */
void txgbe_procfs_exit(struct txgbe_adapter *adapter)
{
	txgbe_del_proc_entries(adapter);
}

int txgbe_procfs_topdir_init(void)
{
	txgbe_top_dir = proc_mkdir("driver/txgbe", NULL);
	if (txgbe_top_dir == NULL)
		return -ENOMEM;

	return 0;
}

void txgbe_procfs_topdir_exit(void)
{
	remove_proc_entry("driver/txgbe", NULL);
}

/* called from txgbe_main.c */
int txgbe_procfs_init(struct txgbe_adapter *adapter)
{
	int rc = 0;
	int index;
	int i;
	char buf[16];   /* much larger than the sensor number will ever be */

	adapter->eth_dir = NULL;
	adapter->info_dir = NULL;
	adapter->therm_dir = NULL;

	if (txgbe_top_dir == NULL) {
		rc = -ENOMEM;
		goto fail;
	}

	adapter->eth_dir = proc_mkdir(pci_name(adapter->pdev), txgbe_top_dir);
	if (adapter->eth_dir == NULL) {
		rc = -ENOMEM;
		goto fail;
	}

	adapter->info_dir = proc_mkdir("info", adapter->eth_dir);
	if (adapter->info_dir == NULL) {
		rc = -ENOMEM;
		goto fail;
	}
	for (index = 0; ; index++) {
		if (txgbe_proc_entries[index].read == NULL)
			break;
		if (!(create_proc_read_entry(txgbe_proc_entries[index].name,
					   0444,
					   adapter->info_dir,
					   txgbe_proc_entries[index].read,
					   adapter))) {

			rc = -ENOMEM;
			goto fail;
		}
	}
	if (!TCALL(&(adapter->hw), ops.init_thermal_sensor_thresh))
		goto exit;


	snprintf(buf, sizeof(buf), "sensor");
	adapter->therm_dir = proc_mkdir(buf, adapter->info_dir);
	if (adapter->therm_dir == NULL) {
		rc = -ENOMEM;
		goto fail;
	}
	for (index = 0; ; index++) {
		if (txgbe_internal_entries[index].read == NULL)
			break;
		/*
		 * therm_data struct contains pointer the read func
		 * will be needing
		 */
		adapter->therm_data.hw = &adapter->hw;
		adapter->therm_data.sensor_data =
			&adapter->hw.mac.thermal_sensor_data.sensor;

		if (!(create_proc_read_entry(
				   txgbe_internal_entries[index].name,
				   0444,
				   adapter->therm_dir,
				   txgbe_internal_entries[index].read,
				   &adapter->therm_data))) {
			rc = -ENOMEM;
			goto fail;
		}
	}

	goto exit;

fail:
	txgbe_del_proc_entries(adapter);
exit:
	return rc;
}

#endif /* !TXGBE_SYSFS */
#endif /* TXGBE_PROCFS */
