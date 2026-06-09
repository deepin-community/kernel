// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2008 - 2022 Xel Technology. */

#include "xlnid.h"
#include "xlnid_common.h"
#include "xlnid_type.h"

#ifdef XLNID_PROCFS
#ifndef XLNID_SYSFS

#include <linux/module.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/netdevice.h>

static struct proc_dir_entry *xlnid_top_dir;

static struct net_device_stats *procfs_get_stats(struct net_device *netdev)
{
#ifndef HAVE_NETDEV_STATS_IN_NETDEV
	struct xlnid_adapter *adapter;
#endif

	if (netdev == NULL) {
		return NULL;
	}

#ifdef HAVE_NETDEV_STATS_IN_NETDEV
	/* only return the current stats */
	return &netdev->stats;
#else
	adapter = netdev_priv(netdev);

	/* only return the current stats */
	return &adapter->net_stats;
#endif /* HAVE_NETDEV_STATS_IN_NETDEV */
}

bool xlnid_thermal_present(struct xlnid_adapter *adapter)
{
	s32 status;

	if (adapter == NULL) {
		return false;
	}

	status = xlnid_init_thermal_sensor_thresh_generic(&(adapter->hw));

	if (status != XLNID_SUCCESS) {
		return false;
	}

	return true;
}

static int xlnid_fwbanner(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	return snprintf(page, count, "%s\n", adapter->eeprom_id);
}

static int xlnid_porttype(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	return snprintf(page, count, "%d\n", test_bit(__XLNID_DOWN, &adapter->state));
}

static int xlnid_portspeed(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;
	int speed = 0;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	switch (adapter->link_speed) {
	case XLNID_LINK_SPEED_100_FULL:
		speed = 1;
		break;

	case XLNID_LINK_SPEED_1GB_FULL:
		speed = 10;
		break;

	case XLNID_LINK_SPEED_10GB_FULL:
		speed = 100;
		break;
	}

	return snprintf(page, count, "%d\n", speed);
}

static int xlnid_wqlflag(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	return snprintf(page, count, "%d\n", adapter->wol);
}

static int xlnid_xflowctl(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;
	struct xlnid_hw *hw;

	if (!adapter) {
		return snprintf(page, count, "error: no adapter\n");
	}

	hw = &adapter->hw;

	return snprintf(page, count, "%d\n", hw->fc.current_mode);
}

static int xlnid_rxdrops(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	net_stats = procfs_get_stats(adapter->netdev);

	if (net_stats == NULL) {
		return snprintf(page, count, "error: no net stats\n");
	}

	return snprintf(page, count, "%lu\n", net_stats->rx_dropped);
}

static int xlnid_rxerrors(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	net_stats = procfs_get_stats(adapter->netdev);

	if (net_stats == NULL) {
		return snprintf(page, count, "error: no net stats\n");
	}

	return snprintf(page, count, "%lu\n", net_stats->rx_errors);
}

static int xlnid_rxupacks(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_hw *hw;
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;

	if (!adapter) {
		return snprintf(page, count, "error: no adapter\n");
	}

	hw = &adapter->hw;

	return snprintf(page, count, "%d\n", XLNID_READ_REG(hw, XLNID_TPR));
}

static int xlnid_rxmpacks(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_hw *hw;
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;

	if (!adapter) {
		return snprintf(page, count, "error: no adapter\n");
	}

	hw = &adapter->hw;

	return snprintf(page, count, "%d\n", XLNID_READ_REG(hw, XLNID_MPRC));
}

static int xlnid_rxbpacks(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_hw *hw;
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;

	if (!adapter) {
		return snprintf(page, count, "error: no adapter\n");
	}

	hw = &adapter->hw;

	return snprintf(page, count, "%d\n", XLNID_READ_REG(hw, XLNID_BPRC));
}

static int xlnid_txupacks(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_hw *hw;
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;

	if (!adapter) {
		return snprintf(page, count, "error: no adapter\n");
	}

	hw = &adapter->hw;

	return snprintf(page, count, "%d\n", XLNID_READ_REG(hw, XLNID_TPT));
}

static int xlnid_txmpacks(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_hw *hw;
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;

	if (!adapter) {
		return snprintf(page, count, "error: no adapter\n");
	}

	hw = &adapter->hw;

	return snprintf(page, count, "%d\n", XLNID_READ_REG(hw, XLNID_MPTC));
}

static int xlnid_txbpacks(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_hw *hw;
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;

	if (!adapter) {
		return snprintf(page, count, "error: no adapter\n");
	}

	hw = &adapter->hw;

	return snprintf(page, count, "%d\n", XLNID_READ_REG(hw, XLNID_BPTC));
}

static int xlnid_txerrors(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	net_stats = procfs_get_stats(adapter->netdev);

	if (net_stats == NULL) {
		return snprintf(page, count, "error: no net stats\n");
	}

	return snprintf(page, count, "%lu\n", net_stats->tx_errors);
}

static int xlnid_txdrops(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	net_stats = procfs_get_stats(adapter->netdev);

	if (net_stats == NULL) {
		return snprintf(page, count, "error: no net stats\n");
	}

	return snprintf(page, count, "%lu\n", net_stats->tx_dropped);
}

static int xlnid_rxframes(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	net_stats = procfs_get_stats(adapter->netdev);

	if (net_stats == NULL) {
		return snprintf(page, count, "error: no net stats\n");
	}

	return snprintf(page, count, "%lu\n", net_stats->rx_packets);
}

static int xlnid_rxbytes(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	net_stats = procfs_get_stats(adapter->netdev);

	if (net_stats == NULL) {
		return snprintf(page, count, "error: no net stats\n");
	}

	return snprintf(page, count, "%lu\n", net_stats->rx_bytes);
}

static int xlnid_txframes(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	net_stats = procfs_get_stats(adapter->netdev);

	if (net_stats == NULL) {
		return snprintf(page, count, "error: no net stats\n");
	}

	return snprintf(page, count, "%lu\n", net_stats->tx_packets);
}

static int xlnid_txbytes(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;
	struct net_device_stats *net_stats;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	net_stats = procfs_get_stats(adapter->netdev);

	if (net_stats == NULL) {
		return snprintf(page, count, "error: no net stats\n");
	}

	return snprintf(page, count, "%lu\n", net_stats->tx_bytes);
}

static int xlnid_linkstat(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_hw *hw;
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;
	int bitmask = 0;
	u32 link_speed;
	bool link_up = false;

	if (!adapter) {
		return snprintf(page, count, "error: no adapter\n");
	}

	hw = &adapter->hw;

	if (!test_bit(__XLNID_DOWN, &adapter->state)) {
		bitmask |= 1;
	}

	if (hw->mac.ops.check_link) {
		hw->mac.ops.check_link(hw, &link_speed, &link_up, false);
	}

	else
		/* always assume link is up, if no check link function */
	{
		link_up = true;
	}

	if (link_up) {
		bitmask |= 2;
	}

	if (adapter->old_lsc != adapter->lsc_int) {
		bitmask |= 4;
		adapter->old_lsc = adapter->lsc_int;
	}

	return snprintf(page, count, "0x%X\n", bitmask);
}

static int xlnid_funcid(char *page, char __always_unused **start,
						off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;
	struct xlnid_hw *hw;

	if (!adapter) {
		return snprintf(page, count, "error: no adapter\n");
	}

	hw = &adapter->hw;

	return snprintf(page, count, "0x%X\n", hw->bus.func);
}

static int xlnid_funcvers(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof,
							void __always_unused *data)
{
	return snprintf(page, count, "%s\n", xlnid_driver_version);
}

static int xlnid_macburn(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_hw *hw;
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;

	if (!adapter) {
		return snprintf(page, count, "error: no adapter\n");
	}

	hw = &adapter->hw;

	return snprintf(page, count, "0x%02X%02X%02X%02X%02X%02X\n",
					(unsigned int)hw->mac.perm_addr[0],
					(unsigned int)hw->mac.perm_addr[1], (unsigned int)hw->mac.perm_addr[2],
					(unsigned int)hw->mac.perm_addr[3], (unsigned int)hw->mac.perm_addr[4],
					(unsigned int)hw->mac.perm_addr[5]);
}

static int xlnid_macadmn(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_hw *hw;
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;

	if (!adapter) {
		return snprintf(page, count, "error: no adapter\n");
	}

	hw = &adapter->hw;

	return snprintf(page, count, "0x%02X%02X%02X%02X%02X%02X\n",
					(unsigned int)hw->mac.addr[0], (unsigned int)hw->mac.addr[1],
					(unsigned int)hw->mac.addr[2], (unsigned int)hw->mac.addr[3],
					(unsigned int)hw->mac.addr[4], (unsigned int)hw->mac.addr[5]);
}

static int xlnid_maclla1(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;
	struct xlnid_hw *hw;
	int rc;
	u16 eeprom_buff[6];
	u16 first_word = 0x37;
	const u16 word_count = ARRAY_SIZE(eeprom_buff);

	if (!adapter) {
		return snprintf(page, count, "error: no adapter\n");
	}

	hw = &adapter->hw;

	rc = hw->eeprom.ops.read_buffer(hw, first_word, 1, &first_word);

	if (rc != 0) {
		return snprintf(page, count, "error: reading pointer to the EEPROM\n");
	}

	if (first_word != 0x0000 && first_word != 0xFFFF) {
		rc = hw->eeprom.ops.read_buffer(hw, first_word, word_count, eeprom_buff);

		if (rc != 0) {
			return snprintf(page, count, "error: reading buffer\n");
		}
	}

	else {
		memset(eeprom_buff, 0, sizeof(eeprom_buff));
	}

	switch (hw->bus.func) {
	case 0:
		return snprintf(page, count, "0x%04X%04X%04X\n", eeprom_buff[0], eeprom_buff[1],
		eeprom_buff[2]);

	case 1:
		return snprintf(page, count, "0x%04X%04X%04X\n", eeprom_buff[3], eeprom_buff[4],
		eeprom_buff[5]);
	}

	return snprintf(page, count, "unexpected port %d\n", hw->bus.func);
}

static int xlnid_mtusize(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;
	struct net_device *netdev;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	netdev = adapter->netdev;

	if (netdev == NULL) {
		return snprintf(page, count, "error: no net device\n");
	}

	return snprintf(page, count, "%d\n", netdev->mtu);
}

static int xlnid_featflag(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	int bitmask = 0;
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;
	struct net_device *netdev;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	netdev = adapter->netdev;

	if (netdev == NULL) {
		return snprintf(page, count, "error: no net device\n");
	}

	if (adapter->netdev->features & NETIF_F_RXCSUM) {
		bitmask |= 1;
	}

	return snprintf(page, count, "%d\n", bitmask);
}

static int xlnid_lsominct(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof,
							void __always_unused *data)
{
	return snprintf(page, count, "%d\n", 1);
}

static int xlnid_prommode(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;
	struct net_device *netdev;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	netdev = adapter->netdev;

	if (netdev == NULL) {
		return snprintf(page, count, "error: no net device\n");
	}

	return snprintf(page, count, "%d\n", netdev->flags & IFF_PROMISC);
}

static int xlnid_txdscqsz(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	return snprintf(page, count, "%d\n", adapter->tx_ring[0]->count);
}

static int xlnid_rxdscqsz(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	return snprintf(page, count, "%d\n", adapter->rx_ring[0]->count);
}

static int xlnid_rxqavg(char *page, char __always_unused **start,
						off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	int index;
	int diff = 0;
	u16 ntc;
	u16 ntu;
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	for (index = 0; index < adapter->num_rx_queues; index++) {
		ntc = adapter->rx_ring[index]->next_to_clean;
		ntu = adapter->rx_ring[index]->next_to_use;

		if (ntc >= ntu) {
			diff += (ntc - ntu);
		}

		else {
			diff += (adapter->rx_ring[index]->count - ntu + ntc);
		}
	}

	if (adapter->num_rx_queues <= 0) {
		return snprintf(page, count, "can't calculate, number of queues %d\n",
						adapter->num_rx_queues);
	}

	return snprintf(page, count, "%d\n", diff / adapter->num_rx_queues);
}

static int xlnid_txqavg(char *page, char __always_unused **start,
						off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	int index;
	int diff = 0;
	u16 ntc;
	u16 ntu;
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	for (index = 0; index < adapter->num_tx_queues; index++) {
		ntc = adapter->tx_ring[index]->next_to_clean;
		ntu = adapter->tx_ring[index]->next_to_use;

		if (ntc >= ntu) {
			diff += (ntc - ntu);
		}

		else {
			diff += (adapter->tx_ring[index]->count - ntu + ntc);
		}
	}

	if (adapter->num_tx_queues <= 0) {
		return snprintf(page, count, "can't calculate, number of queues %d\n",
						adapter->num_tx_queues);
	}

	return snprintf(page, count, "%d\n", diff / adapter->num_tx_queues);
}

static int xlnid_iovotype(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof,
							void __always_unused *data)
{
	return snprintf(page, count, "2\n");
}

static int xlnid_funcnbr(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	return snprintf(page, count, "%d\n", adapter->num_vfs);
}

static int xlnid_pciebnbr(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)data;

	if (adapter == NULL) {
		return snprintf(page, count, "error: no adapter\n");
	}

	return snprintf(page, count, "%d\n", adapter->pdev->bus->number);
}

static int xlnid_therm_location(char *page, char __always_unused **start,
								off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_therm_proc_data *therm_data = (struct xlnid_therm_proc_data *)data;

	if (therm_data == NULL) {
		return snprintf(page, count, "error: no therm_data\n");
	}

	return snprintf(page, count, "%d\n", therm_data->sensor_data->location);
}

static int xlnid_therm_maxopthresh(char *page, char __always_unused **start,
									off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_therm_proc_data *therm_data = (struct xlnid_therm_proc_data *)data;

	if (therm_data == NULL) {
		return snprintf(page, count, "error: no therm_data\n");
	}

	return snprintf(page, count, "%d\n", therm_data->sensor_data->max_op_thresh);
}

static int xlnid_therm_cautionthresh(char *page, char __always_unused **start,
										off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	struct xlnid_therm_proc_data *therm_data = (struct xlnid_therm_proc_data *)data;

	if (therm_data == NULL) {
		return snprintf(page, count, "error: no therm_data\n");
	}

	return snprintf(page, count, "%d\n", therm_data->sensor_data->caution_thresh);
}

static int xlnid_therm_temp(char *page, char __always_unused **start,
							off_t __always_unused off, int count, int __always_unused *eof, void *data)
{
	s32 status;
	struct xlnid_therm_proc_data *therm_data = (struct xlnid_therm_proc_data *)data;

	if (therm_data == NULL) {
		return snprintf(page, count, "error: no therm_data\n");
	}

	status = xlnid_get_thermal_sensor_data_generic(therm_data->hw);

	if (status != XLNID_SUCCESS) {
		snprintf(page, count, "error: status %d returned\n", status);
	}

	return snprintf(page, count, "%d\n", therm_data->sensor_data->temp);
}

struct xlnid_proc_type {
	char name[32];
	int (*read)(char *, char **, off_t, int, int *, void *);
};

struct xlnid_proc_type xlnid_proc_entries[] = {
	{ "fwbanner", &xlnid_fwbanner },
	{ "porttype", &xlnid_porttype },
	{ "portspeed", &xlnid_portspeed },
	{ "wqlflag", &xlnid_wqlflag },
	{ "xflowctl", &xlnid_xflowctl },
	{ "rxdrops", &xlnid_rxdrops },
	{ "rxerrors", &xlnid_rxerrors },
	{ "rxupacks", &xlnid_rxupacks },
	{ "rxmpacks", &xlnid_rxmpacks },
	{ "rxbpacks", &xlnid_rxbpacks },
	{ "txdrops", &xlnid_txdrops },
	{ "txerrors", &xlnid_txerrors },
	{ "txupacks", &xlnid_txupacks },
	{ "txmpacks", &xlnid_txmpacks },
	{ "txbpacks", &xlnid_txbpacks },
	{ "rxframes", &xlnid_rxframes },
	{ "rxbytes", &xlnid_rxbytes },
	{ "txframes", &xlnid_txframes },
	{ "txbytes", &xlnid_txbytes },
	{ "linkstat", &xlnid_linkstat },
	{ "funcid", &xlnid_funcid },
	{ "funcvers", &xlnid_funcvers },
	{ "macburn", &xlnid_macburn },
	{ "macadmn", &xlnid_macadmn },
	{ "maclla1", &xlnid_maclla1 },
	{ "mtusize", &xlnid_mtusize },
	{ "featflag", &xlnid_featflag },
	{ "lsominct", &xlnid_lsominct },
	{ "prommode", &xlnid_prommode },
	{ "txdscqsz", &xlnid_txdscqsz },
	{ "rxdscqsz", &xlnid_rxdscqsz },
	{ "txqavg", &xlnid_txqavg },
	{ "rxqavg", &xlnid_rxqavg },
	{ "iovotype", &xlnid_iovotype },
	{ "funcnbr", &xlnid_funcnbr },
	{ "pciebnbr", &xlnid_pciebnbr },
	{ "", NULL }
};

struct xlnid_proc_type xlnid_internal_entries[] = {
	{ "location", &xlnid_therm_location },
	{ "temp", &xlnid_therm_temp },
	{ "cautionthresh", &xlnid_therm_cautionthresh },
	{ "maxopthresh", &xlnid_therm_maxopthresh },
	{ "", NULL }
};

void xlnid_del_proc_entries(struct xlnid_adapter *adapter)
{
	int index;
	int i;
	char buf[16];               /* much larger than the sensor number will ever be */

	if (xlnid_top_dir == NULL) {
		return;
	}

	for (i = 0; i < XLNID_MAX_SENSORS; i++) {
		if (adapter->therm_dir[i] == NULL) {
			continue;
		}

		for (index = 0;; index++) {
			if (xlnid_internal_entries[index].read == NULL) {
				break;
			}

			remove_proc_entry(xlnid_internal_entries[index].name, adapter->therm_dir[i]);
		}

		snprintf(buf, sizeof(buf), "sensor_%d", i);
		remove_proc_entry(buf, adapter->info_dir);
	}

	if (adapter->info_dir != NULL) {
		for (index = 0;; index++) {
			if (xlnid_proc_entries[index].read == NULL) {
				break;
			}

			remove_proc_entry(xlnid_proc_entries[index].name, adapter->info_dir);
		}

		remove_proc_entry("info", adapter->eth_dir);
	}

	if (adapter->eth_dir != NULL) {
		remove_proc_entry(pci_name(adapter->pdev), xlnid_top_dir);
	}
}

/* called from xlnid_main.c */
void xlnid_procfs_exit(struct xlnid_adapter *adapter)
{
	xlnid_del_proc_entries(adapter);
}

int xlnid_procfs_topdir_init(void)
{
	xlnid_top_dir = proc_mkdir("driver/xlnid", NULL);

	if (xlnid_top_dir == NULL) {
		return -ENOMEM;
	}

	return 0;
}

void xlnid_procfs_topdir_exit(void)
{
	remove_proc_entry("driver/xlnid", NULL);
}

/* called from xlnid_main.c */
int xlnid_procfs_init(struct xlnid_adapter *adapter)
{
	int rc = 0;
	int index;
	int i;
	char buf[16];               /* much larger than the sensor number will ever be */

	adapter->eth_dir = NULL;
	adapter->info_dir = NULL;

	for (i = 0; i < XLNID_MAX_SENSORS; i++) {
		adapter->therm_dir[i] = NULL;
	}

	if (xlnid_top_dir == NULL) {
		rc = -ENOMEM;
		goto fail;
	}

	adapter->eth_dir = proc_mkdir(pci_name(adapter->pdev), xlnid_top_dir);

	if (adapter->eth_dir == NULL) {
		rc = -ENOMEM;
		goto fail;
	}

	adapter->info_dir = proc_mkdir("info", adapter->eth_dir);

	if (adapter->info_dir == NULL) {
		rc = -ENOMEM;
		goto fail;
	}

	for (index = 0;; index++) {
		if (xlnid_proc_entries[index].read == NULL) {
			break;
		}

		if (!(create_proc_read_entry(xlnid_proc_entries[index].name, 0444,
										adapter->info_dir, xlnid_proc_entries[index].read, adapter))) {

			rc = -ENOMEM;
			goto fail;
		}
	}

	if (xlnid_thermal_present(adapter) == false) {
		goto exit;
	}

	for (i = 0; i < XLNID_MAX_SENSORS; i++) {

		if (adapter->hw.mac.thermal_sensor_data.sensor[i].location == 0) {
			continue;
		}

		snprintf(buf, sizeof(buf), "sensor_%d", i);
		adapter->therm_dir[i] = proc_mkdir(buf, adapter->info_dir);

		if (adapter->therm_dir[i] == NULL) {
			rc = -ENOMEM;
			goto fail;
		}

		for (index = 0;; index++) {
			if (xlnid_internal_entries[index].read == NULL) {
				break;
			}

			/*
				therm_data struct contains pointer the read func
				will be needing
			*/
			adapter->therm_data[i].hw = &adapter->hw;
			adapter->therm_data[i].sensor_data =
				&adapter->hw.mac.thermal_sensor_data.sensor[i];

			if (!(create_proc_read_entry(xlnid_internal_entries[index].name, 0444,
											adapter->therm_dir[i], xlnid_internal_entries[index].read,
											&adapter->therm_data[i]))) {
				rc = -ENOMEM;
				goto fail;
			}
		}
	}

	goto exit;

fail:
	xlnid_del_proc_entries(adapter);
exit:
	return rc;
}

#endif /* !XLNID_SYSFS */
#endif /* XLNID_PROCFS */
