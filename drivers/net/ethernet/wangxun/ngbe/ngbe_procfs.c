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


#include "ngbe.h"
#include "ngbe_hw.h"
#include "ngbe_type.h"

#ifdef NGBE_PROCFS
#ifndef NGBE_SYSFS

#include <linux/module.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/netdevice.h>

static struct proc_dir_entry *ngbe_top_dir;

static struct net_device_stats *procfs_get_stats(struct net_device *netdev)
{
#ifndef HAVE_NETDEV_STATS_IN_NETDEV
	struct ngbe_adapter *adapter;
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

static int ngbe_fwbanner(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	return snprintf(page, count, "%s\n", adapter->eeprom_id);
}

static int ngbe_porttype(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	return snprintf(page, count, "%d\n",
			test_bit(__NGBE_DOWN, &adapter->state));
}

static int ngbe_portspeed(char *page, char __always_unused **start,
			   off_t __always_unused off, int count,
			   int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	int speed = 0;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	switch (adapter->link_speed) {
	case NGBE_LINK_SPEED_100_FULL:
		speed = 1;
		break;
	case NGBE_LINK_SPEED_1GB_FULL:
		speed = 10;
		break;
	case NGBE_LINK_SPEED_10GB_FULL:
		speed = 100;
		break;
	default:
		break;
	}
	return snprintf(page, count, "%d\n", speed);
}

static int ngbe_wqlflag(char *page, char __always_unused **start,
			 off_t __always_unused off, int count,
			 int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	return snprintf(page, count, "%d\n", adapter->wol);
}

static int ngbe_xflowctl(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	struct ngbe_hw *hw;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");

	return snprintf(page, count, "%d\n", hw->fc.current_mode);
}

static int ngbe_rxdrops(char *page, char __always_unused **start,
			 off_t __always_unused off, int count,
			 int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	net_stats  = procfs_get_stats(adapter->netdev);
	if (net_stats == NULL)
		return snprintf(page, count, "error: no net stats\n");

	return snprintf(page, count, "%lu\n",
			net_stats->rx_dropped);
}

static int ngbe_rxerrors(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	net_stats  = procfs_get_stats(adapter->netdev);
	if (net_stats == NULL)
		return snprintf(page, count, "error: no net stats\n");

	return snprintf(page, count, "%lu\n", net_stats->rx_errors);
}

static int ngbe_rxupacks(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct ngbe_hw *hw;
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");

	return snprintf(page, count, "%d\n", rd32(hw, NGBE_TPR));
}

static int ngbe_rxmpacks(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct ngbe_hw *hw;
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	int i, mprc = 0;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");
	for (i = 0; i < 8; i++)
		mprc += rd32(hw, NGBE_PX_MPRC(i));
	return snprintf(page, count, "%d\n", mprc);
}

static int ngbe_rxbpacks(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct ngbe_hw *hw;
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");

	return snprintf(page, count, "%d\n",
			rd32(hw, NGBE_RX_BC_FRAMES_GOOD_LOW));
}

static int ngbe_txupacks(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct ngbe_hw *hw;
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");

	return snprintf(page, count, "%d\n",
			rd32(hw, NGBE_TX_FRAME_CNT_GOOD_BAD_LOW));
}

static int ngbe_txmpacks(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct ngbe_hw *hw;
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");

	return snprintf(page, count, "%d\n",
			rd32(hw, NGBE_TX_MC_FRAMES_GOOD_LOW));
}

static int ngbe_txbpacks(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct ngbe_hw *hw;
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");

	return snprintf(page, count, "%d\n",
			rd32(hw, NGBE_TX_BC_FRAMES_GOOD_LOW));
}

static int ngbe_txerrors(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	net_stats  = procfs_get_stats(adapter->netdev);
	if (net_stats == NULL)
		return snprintf(page, count, "error: no net stats\n");

	return snprintf(page, count, "%lu\n",
			net_stats->tx_errors);
}

static int ngbe_txdrops(char *page, char __always_unused **start,
			 off_t __always_unused off, int count,
			 int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	net_stats  = procfs_get_stats(adapter->netdev);
	if (net_stats == NULL)
		return snprintf(page, count, "error: no net stats\n");

	return snprintf(page, count, "%lu\n",
			net_stats->tx_dropped);
}

static int ngbe_rxframes(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	net_stats  = procfs_get_stats(adapter->netdev);
	if (net_stats == NULL)
		return snprintf(page, count, "error: no net stats\n");

	return snprintf(page, count, "%lu\n",
			net_stats->rx_packets);
}

static int ngbe_rxbytes(char *page, char __always_unused **start,
			 off_t __always_unused off, int count,
			 int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	net_stats  = procfs_get_stats(adapter->netdev);
	if (net_stats == NULL)
		return snprintf(page, count, "error: no net stats\n");

	return snprintf(page, count, "%lu\n",
			net_stats->rx_bytes);
}

static int ngbe_txframes(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	net_stats  = procfs_get_stats(adapter->netdev);
	if (net_stats == NULL)
		return snprintf(page, count, "error: no net stats\n");

	return snprintf(page, count, "%lu\n",
			net_stats->tx_packets);
}

static int ngbe_txbytes(char *page, char __always_unused **start,
			 off_t __always_unused off, int count,
			 int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	net_stats  = procfs_get_stats(adapter->netdev);
	if (net_stats == NULL)
		return snprintf(page, count, "error: no net stats\n");

	return snprintf(page, count, "%lu\n",
			net_stats->tx_bytes);
}

static int ngbe_linkstat(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct ngbe_hw *hw;
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	int bitmask = 0;
	u32 link_speed;
	bool link_up = false;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");

	if (!test_bit(__NGBE_DOWN, &adapter->state))
		bitmask |= 1;

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

static int ngbe_funcid(char *page, char __always_unused **start,
			off_t __always_unused off, int count,
			int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	struct ngbe_hw *hw;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	hw = &adapter->hw;
	if (hw == NULL)
		return snprintf(page, count, "error: no hw data\n");

	return snprintf(page, count, "0x%X\n", hw->bus.func);
}

static int ngbe_funcvers(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void __always_unused *data)
{
	return snprintf(page, count, "%s\n", ngbe_driver_version);
}

static int ngbe_macburn(char *page, char __always_unused **start,
			 off_t __always_unused off, int count,
			 int __always_unused *eof, void *data)
{
	struct ngbe_hw *hw;
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
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

static int ngbe_macadmn(char *page, char __always_unused **start,
			 off_t __always_unused off, int count,
			 int __always_unused *eof, void *data)
{
	struct ngbe_hw *hw;
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
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

static int ngbe_maclla1(char *page, char __always_unused **start,
			 off_t __always_unused off, int count,
			 int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	struct ngbe_hw *hw;
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
	default:
		return snprintf(page, count, "unexpected port %d\n", hw->bus.func);
	}
}

static int ngbe_mtusize(char *page, char __always_unused **start,
			 off_t __always_unused off, int count,
			 int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	struct net_device *netdev;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	netdev = adapter->netdev;
	if (netdev == NULL)
		return snprintf(page, count, "error: no net device\n");

	return snprintf(page, count, "%d\n", netdev->mtu);
}

static int ngbe_featflag(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	int bitmask = 0;
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
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

static int ngbe_lsominct(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void __always_unused *data)
{
	return snprintf(page, count, "%d\n", 1);
}

static int ngbe_prommode(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	struct net_device *netdev;

	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");
	netdev = adapter->netdev;
	if (netdev == NULL)
		return snprintf(page, count, "error: no net device\n");

	return snprintf(page, count, "%d\n",
			netdev->flags & IFF_PROMISC);
}

static int ngbe_txdscqsz(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	return snprintf(page, count, "%d\n", adapter->tx_ring[0]->count);
}

static int ngbe_rxdscqsz(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	return snprintf(page, count, "%d\n", adapter->rx_ring[0]->count);
}

static int ngbe_rxqavg(char *page, char __always_unused **start,
			off_t __always_unused off, int count,
			int __always_unused *eof, void *data)
{
	int index;
	int diff = 0;
	u16 ntc;
	u16 ntu;
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
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

static int ngbe_txqavg(char *page, char __always_unused **start,
			off_t __always_unused off, int count,
			int __always_unused *eof, void *data)
{
	int index;
	int diff = 0;
	u16 ntc;
	u16 ntu;
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
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

static int ngbe_iovotype(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void __always_unused *data)
{
	return snprintf(page, count, "2\n");
}

static int ngbe_funcnbr(char *page, char __always_unused **start,
			 off_t __always_unused off, int count,
			 int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	return snprintf(page, count, "%d\n", adapter->num_vfs);
}

static int ngbe_pciebnbr(char *page, char __always_unused **start,
			  off_t __always_unused off, int count,
			  int __always_unused *eof, void *data)
{
	struct ngbe_adapter *adapter = (struct ngbe_adapter *)data;
	if (adapter == NULL)
		return snprintf(page, count, "error: no adapter\n");

	return snprintf(page, count, "%d\n", adapter->pdev->bus->number);
}

static int ngbe_therm_dealarmthresh(char *page, char __always_unused **start,
				   off_t __always_unused off, int count,
				   int __always_unused *eof, void *data)
{
	struct ngbe_therm_proc_data *therm_data =
		(struct ngbe_therm_proc_data *)data;

	if (therm_data == NULL)
		return snprintf(page, count, "error: no therm_data\n");

	return snprintf(page, count, "%d\n",
			therm_data->sensor_data->dalarm_thresh);
}


static int ngbe_therm_alarmthresh(char *page, char __always_unused **start,
				     off_t __always_unused off, int count,
				     int __always_unused *eof, void *data)
{
	struct ngbe_therm_proc_data *therm_data =
		(struct ngbe_therm_proc_data *)data;

	if (therm_data == NULL)
		return snprintf(page, count, "error: no therm_data\n");

	return snprintf(page, count, "%d\n",
			therm_data->sensor_data->alarm_thresh);
}

static int ngbe_therm_temp(char *page, char __always_unused **start,
			    off_t __always_unused off, int count,
			    int __always_unused *eof, void *data)
{
	s32 status;
	struct ngbe_therm_proc_data *therm_data =
		(struct ngbe_therm_proc_data *)data;

	if (therm_data == NULL)
		return snprintf(page, count, "error: no therm_data\n");

	status = ngbe_get_thermal_sensor_data(therm_data->hw);
	if (status != 0)
		snprintf(page, count, "error: status %d returned\n", status);

	return snprintf(page, count, "%d\n", therm_data->sensor_data->temp);
}


struct ngbe_proc_type {
	char name[32];
	int (*read)(char*, char**, off_t, int, int*, void*);
};

struct ngbe_proc_type ngbe_proc_entries[] = {
	{"fwbanner", &ngbe_fwbanner},
	{"porttype", &ngbe_porttype},
	{"portspeed", &ngbe_portspeed},
	{"wqlflag", &ngbe_wqlflag},
	{"xflowctl", &ngbe_xflowctl},
	{"rxdrops", &ngbe_rxdrops},
	{"rxerrors", &ngbe_rxerrors},
	{"rxupacks", &ngbe_rxupacks},
	{"rxmpacks", &ngbe_rxmpacks},
	{"rxbpacks", &ngbe_rxbpacks},
	{"txdrops", &ngbe_txdrops},
	{"txerrors", &ngbe_txerrors},
	{"txupacks", &ngbe_txupacks},
	{"txmpacks", &ngbe_txmpacks},
	{"txbpacks", &ngbe_txbpacks},
	{"rxframes", &ngbe_rxframes},
	{"rxbytes", &ngbe_rxbytes},
	{"txframes", &ngbe_txframes},
	{"txbytes", &ngbe_txbytes},
	{"linkstat", &ngbe_linkstat},
	{"funcid", &ngbe_funcid},
	{"funcvers", &ngbe_funcvers},
	{"macburn", &ngbe_macburn},
	{"macadmn", &ngbe_macadmn},
	{"maclla1", &ngbe_maclla1},
	{"mtusize", &ngbe_mtusize},
	{"featflag", &ngbe_featflag},
	{"lsominct", &ngbe_lsominct},
	{"prommode", &ngbe_prommode},
	{"txdscqsz", &ngbe_txdscqsz},
	{"rxdscqsz", &ngbe_rxdscqsz},
	{"txqavg", &ngbe_txqavg},
	{"rxqavg", &ngbe_rxqavg},
	{"iovotype", &ngbe_iovotype},
	{"funcnbr", &ngbe_funcnbr},
	{"pciebnbr", &ngbe_pciebnbr},
	{"", NULL}
};

struct ngbe_proc_type ngbe_internal_entries[] = {
	{"temp", &ngbe_therm_temp},
	{"alarmthresh", &ngbe_therm_alarmthresh},
	{"dealarmthresh", &ngbe_therm_dealarmthresh},
	{"", NULL}
};

void ngbe_del_proc_entries(struct ngbe_adapter *adapter)
{
	int index;
	int i;
	char buf[16];   /* much larger than the sensor number will ever be */

	if (ngbe_top_dir == NULL)
		return;

	for (i = 0; i < NGBE_MAX_SENSORS; i++) {
		if (adapter->therm_dir[i] == NULL)
			continue;

		for (index = 0; ; index++) {
			if (ngbe_internal_entries[index].read == NULL)
				break;

			 remove_proc_entry(ngbe_internal_entries[index].name,
					   adapter->therm_dir[i]);
		}
		snprintf(buf, sizeof(buf), "sensor_%d", i);
		remove_proc_entry(buf, adapter->info_dir);
	}

	if (adapter->info_dir != NULL) {
		for (index = 0; ; index++) {
			if (ngbe_proc_entries[index].read == NULL)
				break;
			remove_proc_entry(ngbe_proc_entries[index].name,
					  adapter->info_dir);
		}
		remove_proc_entry("info", adapter->eth_dir);
	}

	if (adapter->eth_dir != NULL)
		remove_proc_entry(pci_name(adapter->pdev), ngbe_top_dir);
}

/* called from ngbe_main.c */
void ngbe_procfs_exit(struct ngbe_adapter *adapter)
{
	ngbe_del_proc_entries(adapter);
}

int ngbe_procfs_topdir_init(void)
{
	ngbe_top_dir = proc_mkdir("driver/ngbe", NULL);
	if (ngbe_top_dir == NULL)
		return -ENOMEM;

	return 0;
}

void ngbe_procfs_topdir_exit(void)
{
	remove_proc_entry("driver/ngbe", NULL);
}

/* called from ngbe_main.c */
int ngbe_procfs_init(struct ngbe_adapter *adapter)
{
	int rc = 0;
	int index;
	int i;
	char buf[16];   /* much larger than the sensor number will ever be */

	adapter->eth_dir = NULL;
	adapter->info_dir = NULL;
	adapter->therm_dir = NULL;

	if (ngbe_top_dir == NULL) {
		rc = -ENOMEM;
		goto fail;
	}

	adapter->eth_dir = proc_mkdir(pci_name(adapter->pdev), ngbe_top_dir);
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
		if (ngbe_proc_entries[index].read == NULL)
			break;
		if (!(create_proc_read_entry(ngbe_proc_entries[index].name,
					   0444,
					   adapter->info_dir,
					   ngbe_proc_entries[index].read,
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
		if (ngbe_internal_entries[index].read == NULL)
			break;
		/*
		 * therm_data struct contains pointer the read func
		 * will be needing
		 */
		adapter->therm_data.hw = &adapter->hw;
		adapter->therm_data.sensor_data =
			&adapter->hw.mac.thermal_sensor_data.sensor;

		if (!(create_proc_read_entry(
				   ngbe_internal_entries[index].name,
				   0444,
				   adapter->therm_dir,
				   ngbe_internal_entries[index].read,
				   &adapter->therm_data))) {
			rc = -ENOMEM;
			goto fail;
		}
	}

	goto exit;

fail:
	ngbe_del_proc_entries(adapter);
exit:
	return rc;
}

#endif /* !NGBE_SYSFS */
#endif /* NGBE_PROCFS */
