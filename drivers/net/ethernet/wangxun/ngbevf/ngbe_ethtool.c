/*******************************************************************************

  WangXun(R) GbE PCI Express Virtual Function Linux Network Driver
  Copyright(c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:
  Software Team <linux.nic@trustnetic.com>
  WangXun Technology, HuaXing Times Square A507, Hangzhou, China.

*******************************************************************************/

#include <linux/types.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/ethtool.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include "ngbe.h"
#include "ngbe_diag.h"

#ifndef ETH_GSTRING_LEN
#define ETH_GSTRING_LEN 32
#endif

#define NGBE_ALL_RAR_ENTRIES 16

#ifdef ETHTOOL_OPS_COMPAT
#include "kcompat_ethtool.c"
#endif

/**
 * ngbe transmit and receive states
 **/
#ifdef ETHTOOL_GSTATS
struct ngbe_stat {
	char name[ETH_GSTRING_LEN];
	int size;
	int offset;
};

#define NGBE_STAT(_name, _stru, _stat) { \
	.name = _name, \
	.size = sizeof_field(_stru, _stat), \
	.offset = offsetof(_stru, _stat), \
}

#define NGBE_NET_STAT(_stat) \
		NGBE_STAT("net."#_stat, ngbe_net_stats_t, _stat)
static const struct ngbe_stat ngbe_net_stats[] = {
	NGBE_NET_STAT(rx_packets),
	NGBE_NET_STAT(tx_packets),
	NGBE_NET_STAT(rx_bytes),
	NGBE_NET_STAT(tx_bytes),
	NGBE_NET_STAT(multicast),
};
#define NGBE_NET_STATS_LEN   ARRAY_SIZE(ngbe_net_stats)

#define NGBE_SW_STAT(_name, _stat) \
		NGBE_STAT("sw."_name, struct ngbevf_adapter, _stat)
static struct ngbe_stat ngbe_sw_stats[] = {
	NGBE_SW_STAT("tx_busy",              sw_stats.tx_busy),
	NGBE_SW_STAT("tx_restart_queue",     sw_stats.tx_restart_queue),
	NGBE_SW_STAT("tx_timeout_count",     sw_stats.tx_timeout_count),

	NGBE_SW_STAT("rx_csum_bad",          sw_stats.rx_csum_bad),
	NGBE_SW_STAT("rx_no_dma_resources",  sw_stats.rx_no_dma_resources),
	NGBE_SW_STAT("rx_alloc_page_failed", sw_stats.rx_alloc_page_failed),
	NGBE_SW_STAT("rx_alloc_buff_failed", sw_stats.rx_alloc_buff_failed),
};
#define NGBE_SW_STATS_LEN	ARRAY_SIZE(ngbe_sw_stats)

#define NGBE_HW_STAT(_name, _stat) \
		NGBE_STAT("hw."_name, struct ngbevf_adapter, _stat)
static struct ngbe_stat ngbevf_hw_stats[] = {
	NGBE_HW_STAT("gprc",       stats.gprc),
	NGBE_HW_STAT("gptc",       stats.gptc),
	NGBE_HW_STAT("gorc",       stats.gorc),
	NGBE_HW_STAT("gotc",       stats.gotc),
	NGBE_HW_STAT("mprc",       stats.mprc),
	NGBE_HW_STAT("bprc",       stats.bprc),
	NGBE_HW_STAT("mptc",       stats.mptc),
	NGBE_HW_STAT("bptc",       stats.bptc),
	NGBE_HW_STAT("last_gprc",  last_stats.gprc),
	NGBE_HW_STAT("last_gptc",  last_stats.gptc),
	NGBE_HW_STAT("last_gorc",  last_stats.gorc),
	NGBE_HW_STAT("last_gotc",  last_stats.gotc),
	NGBE_HW_STAT("last_mprc",  last_stats.mprc),
	NGBE_HW_STAT("last_bprc",  last_stats.bprc),
	NGBE_HW_STAT("last_mptc",  last_stats.mptc),
	NGBE_HW_STAT("last_bptc",  last_stats.bptc),

	NGBE_HW_STAT("base_gprc",  base_stats.gprc),
	NGBE_HW_STAT("base_gptc",  base_stats.gptc),
	NGBE_HW_STAT("base_gorc",  base_stats.gorc),
	NGBE_HW_STAT("base_gotc",  base_stats.gotc),
	NGBE_HW_STAT("base_mprc",  base_stats.mprc),
	NGBE_HW_STAT("base_bprc",  base_stats.bprc),
	NGBE_HW_STAT("base_mptc",  base_stats.mptc),
	NGBE_HW_STAT("base_bptc",  base_stats.bptc),

	NGBE_HW_STAT("reset_gprc", reset_stats.gprc),
	NGBE_HW_STAT("reset_gptc", reset_stats.gptc),
	NGBE_HW_STAT("reset_gorc", reset_stats.gorc),
	NGBE_HW_STAT("reset_gotc", reset_stats.gotc),
	NGBE_HW_STAT("reset_mprc", reset_stats.mprc),
	NGBE_HW_STAT("reset_bprc", reset_stats.bprc),
	NGBE_HW_STAT("reset_mptc", reset_stats.mptc),
	NGBE_HW_STAT("reset_bptc", reset_stats.bptc),

};
#define NGBE_HW_STATS_LEN	ARRAY_SIZE(ngbevf_hw_stats)

#define NGBE_QUEUE_STATS_LEN \
	   ((((struct ngbevf_adapter *)netdev_priv(netdev))->num_tx_queues \
	      + ((struct ngbevf_adapter *)netdev_priv(netdev))->num_rx_queues) \
	      * (sizeof(struct ngbe_stat) / sizeof(u64)))

#define NGBE_STATS_LEN (NGBE_NET_STATS_LEN \
			 + NGBE_SW_STATS_LEN \
			 + NGBE_HW_STATS_LEN \
			 + NGBE_QUEUE_STATS_LEN)
#endif /* ETHTOOL_GSTATS */

/**
 * ngbe self tests
 **/
#ifdef ETHTOOL_TEST
static const char ngbe_gstrings_test[][ETH_GSTRING_LEN] = {
	"Register test  (offline)",
	"Link test   (on/offline)"
};
#define NGBE_TEST_LEN (sizeof(ngbe_gstrings_test) / ETH_GSTRING_LEN)
#endif /* ETHTOOL_TEST */

#ifdef HAVE_ETHTOOL_CONVERT_U32_AND_LINK_MODE
static int ngbe_get_link_ksettings(struct net_device *netdev,
				      struct ethtool_link_ksettings *cmd)
#else
static int ngbe_get_settings(struct net_device *netdev,
				struct ethtool_cmd *ecmd)
#endif
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	struct ngbevf_hw *hw = &adapter->hw;
	u32 link_speed = 0;
	bool link_up = false;

#ifdef HAVE_ETHTOOL_CONVERT_U32_AND_LINK_MODE
	ethtool_link_ksettings_zero_link_mode(cmd, supported);
	cmd->base.autoneg = AUTONEG_DISABLE;
	cmd->base.port = -1;
#else
	ecmd->supported = SUPPORTED_1000baseT_Full | SUPPORTED_100baseT_Full | SUPPORTED_10baseT_Full;
	ecmd->advertising = ADVERTISED_1000baseT_Full | ADVERTISED_100baseT_Full | ADVERTISED_10baseT_Full;
	
	ecmd->transceiver = XCVR_DUMMY1;
	ecmd->port = -1;
#endif

	if (!in_interrupt()) {
		hw->mac.get_link_status = 1;
		TCALL(hw, mac.ops.check_link, &link_speed, &link_up, false);
	} else {
		/*
		 * this case is a special workaround for RHEL5 bonding
		 * that calls this routine from interrupt context
		 */
		link_speed = adapter->link_speed;
		link_up = adapter->link_up;
	}

	if (link_up) {
		__u32 speed = SPEED_1000;
		switch(link_speed) {
		case NGBE_LINK_SPEED_1GB_FULL:
			speed = SPEED_1000;
			break;
		case NGBE_LINK_SPEED_100_FULL:
			speed = SPEED_100;
			break;
		case NGBE_LINK_SPEED_10_FULL:
			speed = SPEED_10;
			break;
		}

#ifdef HAVE_ETHTOOL_CONVERT_U32_AND_LINK_MODE
		cmd->base.speed = speed;
		cmd->base.duplex = DUPLEX_FULL;
	} else {
		cmd->base.speed = SPEED_UNKNOWN;
		cmd->base.duplex = DUPLEX_UNKNOWN;
	}
#else
		ethtool_cmd_speed_set(ecmd, speed);
		ecmd->duplex = DUPLEX_FULL;
	} else {
		ethtool_cmd_speed_set(ecmd, SPEED_UNKNOWN);
		ecmd->duplex = DUPLEX_UNKNOWN;
	}

	ecmd->supported |= SUPPORTED_TP;
	ecmd->advertising |= ADVERTISED_TP;
	ecmd->port = PORT_TP;
	ecmd->autoneg = AUTONEG_DISABLE;
#endif
	return 0;
}

#ifdef HAVE_ETHTOOL_CONVERT_U32_AND_LINK_MODE
static int ngbe_set_link_ksettings(struct net_device __always_unused *netdev,
		       const struct ethtool_link_ksettings __always_unused *cmd)
#else
static int ngbe_set_settings(struct net_device __always_unused *netdev,
				struct ethtool_cmd __always_unused *ecmd)
#endif
{
	return -EINVAL;
}

#ifndef HAVE_NDO_SET_FEATURES
static u32 ngbe_get_rx_csum(struct net_device *netdev)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	return (adapter->flagsd & NGBE_F_CAP_RX_CSUM);
}

static int ngbe_set_rx_csum(struct net_device *netdev, u32 data)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	if (data)
		adapter->flagsd |= NGBE_F_CAP_RX_CSUM;
	else
		adapter->flagsd &= ~NGBE_F_CAP_RX_CSUM;

	if (netif_running(netdev)) {
		if (!adapter->dev_closed) {
			ngbevf_reinit_locked(adapter);
		}
	} else {
		ngbe_reset(adapter);
	}

	return 0;
}

static u32 ngbe_get_tx_csum(struct net_device *netdev)
{
	return (netdev->features & NETIF_F_IP_CSUM) != 0;
}

static int ngbe_set_tx_csum(struct net_device *netdev, u32 data)
{
	if (data)
#ifdef NETIF_F_IPV6_CSUM
		netdev->features |= (NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM);
	else
		netdev->features &= ~(NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM);
#else
		netdev->features |= NETIF_F_IP_CSUM;
	else
		netdev->features &= ~NETIF_F_IP_CSUM;
#endif

	return 0;
}

#ifdef NETIF_F_TSO
static int ngbe_set_tso(struct net_device *netdev, u32 data)
{
#ifndef HAVE_NETDEV_VLAN_FEATURES
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
#endif
	if (data) {
		netdev->features |= NETIF_F_TSO;
#ifdef NETIF_F_TSO6
		netdev->features |= NETIF_F_TSO6;
#endif
	} else {
		netif_tx_stop_all_queues(netdev);
		netdev->features &= ~NETIF_F_TSO;
#ifdef NETIF_F_TSO6
		netdev->features &= ~NETIF_F_TSO6;
#endif
#ifndef HAVE_NETDEV_VLAN_FEATURES
#ifdef NETIF_F_HW_VLAN_TX
		/* disable TSO on all VLANs if they're present */
		if (adapter->vlgrp) {
			int i;
			struct net_device *v_netdev;
			for (i = 0; i < VLAN_GROUP_ARRAY_LEN; i++) {
				v_netdev =
				       vlan_group_get_device(adapter->vlgrp, i);
				if (v_netdev) {
					v_netdev->features &= ~NETIF_F_TSO;
#ifdef NETIF_F_TSO6
					v_netdev->features &= ~NETIF_F_TSO6;
#endif
					vlan_group_set_device(adapter->vlgrp, i,
							      v_netdev);
				}
			}
		}
#endif /* NETIF_F_HW_VLAN_TX */
#endif /* HAVE_NETDEV_VLAN_FEATURES */
		netif_tx_start_all_queues(netdev);
	}
	return 0;
}
#endif /* NETIF_F_TSO */
#endif /* HAVE_NDO_SET_FEATURES */

static u32 ngbe_get_msglevel(struct net_device *netdev)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	return adapter->msg_enable;
}

static void ngbe_set_msglevel(struct net_device *netdev, u32 data)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	adapter->msg_enable = data;
}

#define NGBE_REGS_LEN  45
static int ngbe_get_regs_len(struct net_device __always_unused *netdev)
{
	return NGBE_REGS_LEN * sizeof(u32);
}

#define NGBE_GET_STAT(_A_, _R_) _A_->stats._R_

static void ngbe_get_regs(struct net_device *netdev, struct ethtool_regs *regs,
			   void *p)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	struct ngbevf_hw *hw = &adapter->hw;
	u32 *regs_buff = p;

	memset(p, 0, NGBE_REGS_LEN * sizeof(u32));

	regs->version = (1 << 24) | hw->revision_id << 16 | hw->device_id;

	/* NGBE_VFCTRL is a Write Only register, so just return 0 */
	regs_buff[0] = 0x0;

	/* General Registers */
	regs_buff[1] = rd32(hw, NGBE_VXSTATUS);
	regs_buff[3] = rd32(hw, NGBE_VXRXMEMWRAP);

	/* Interrupt */
	regs_buff[5] = rd32(hw, NGBE_VXICR);
	regs_buff[6] = rd32(hw, NGBE_VXICS);
	regs_buff[7] = rd32(hw, NGBE_VXIMS);
	regs_buff[8] = rd32(hw, NGBE_VXIMC);
	regs_buff[11] = rd32(hw, NGBE_VXITR(0));
	regs_buff[12] = rd32(hw, NGBE_VXIVAR(0));
	regs_buff[13] = rd32(hw, NGBE_VXIVAR_MISC);

	/* Receive DMA */
	regs_buff[14] = rd32(hw, NGBE_VXRDBAL);
	regs_buff[16] = rd32(hw, NGBE_VXRDBAH);
	regs_buff[20] = rd32(hw, NGBE_VXRDH);
	regs_buff[22] = rd32(hw, NGBE_VXRDT);
	regs_buff[24] = rd32(hw, NGBE_VXRXDCTL);

	/* Receive */
	regs_buff[28] = rd32(hw, NGBE_VXMRQC);

	/* Transmit */
	regs_buff[29] = rd32(hw, NGBE_VXTDBAL);
	regs_buff[31] = rd32(hw, NGBE_VXTDBAH);
	regs_buff[35] = rd32(hw, NGBE_VXTDH);
	regs_buff[37] = rd32(hw, NGBE_VXTDT);
	regs_buff[39] = rd32(hw, NGBE_VXTXDCTL);

}

static int ngbe_get_eeprom(struct net_device __always_unused *netdev,
			    struct ethtool_eeprom __always_unused *eeprom,
			    u8 __always_unused *bytes)
{
	return -EOPNOTSUPP;
}

static int ngbe_set_eeprom(struct net_device __always_unused *netdev,
			    struct ethtool_eeprom __always_unused *eeprom,
			    u8 __always_unused *bytes)
{
	return -EOPNOTSUPP;
}

static void ngbe_get_drvinfo(struct net_device *netdev,
			      struct ethtool_drvinfo *drvinfo)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);

	strlcpy(drvinfo->driver, ngbe_driver_name,
		sizeof(drvinfo->driver));
	strlcpy(drvinfo->version, ngbe_driver_version,
		sizeof(drvinfo->version));
	strlcpy(drvinfo->fw_version, ngbe_firmware_version,
		sizeof(drvinfo->fw_version));
	strlcpy(drvinfo->bus_info, pci_name(adapter->pdev),
		sizeof(drvinfo->bus_info));
}

static void ngbe_get_ringparam(struct net_device *netdev,
				  struct ethtool_ringparam *ring)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);

	ring->rx_max_pending = NGBE_MAX_RXD;
	ring->tx_max_pending = NGBE_MAX_TXD;
	ring->rx_mini_max_pending = 0;
	ring->rx_jumbo_max_pending = 0;
	ring->rx_pending = adapter->rx_ring_count;
	ring->tx_pending = adapter->tx_ring_count;
	ring->rx_mini_pending = 0;
	ring->rx_jumbo_pending = 0;
}

static int ngbe_set_ringparam(struct net_device *netdev,
				 struct ethtool_ringparam *ring)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	struct ngbe_ring *tx_ring = NULL, *rx_ring = NULL;
	int i, err = 0;
	u32 new_rx_count, new_tx_count;

	if ((ring->rx_mini_pending) || (ring->rx_jumbo_pending))
		return -EINVAL;

	new_rx_count = max(ring->rx_pending, (u32)NGBE_MIN_RXD);
	new_rx_count = min(new_rx_count, (u32)NGBE_MAX_RXD);
	new_rx_count = ALIGN(new_rx_count, NGBE_REQ_RX_DESCRIPTOR_MULTIPLE);

	new_tx_count = max(ring->tx_pending, (u32)NGBE_MIN_TXD);
	new_tx_count = min(new_tx_count, (u32)NGBE_MAX_TXD);
	new_tx_count = ALIGN(new_tx_count, NGBE_REQ_TX_DESCRIPTOR_MULTIPLE);

	if ((new_tx_count == adapter->tx_ring_count) &&
	    (new_rx_count == adapter->rx_ring_count)) {
		/* nothing to do */
		return 0;
	}

	while (test_and_set_bit(__NGBE_RESETTING, &adapter->state))
		msleep(1);

	/*
	 * If the adapter isn't up and running then just set the
	 * new parameters and scurry for the exits.
	 */
	if (!netif_running(adapter->netdev)) {
		for (i = 0; i < adapter->num_tx_queues; i++)
			adapter->tx_ring[i]->count = new_tx_count;
		for (i = 0; i < adapter->num_rx_queues; i++)
			adapter->rx_ring[i]->count = new_rx_count;
		adapter->tx_ring_count = new_tx_count;
		adapter->rx_ring_count = new_rx_count;
		goto clear_reset;
	}

	if (new_tx_count != adapter->tx_ring_count) {
		tx_ring = vmalloc(adapter->num_tx_queues * sizeof(*tx_ring));
		if (!tx_ring) {
			err = -ENOMEM;
			goto clear_reset;
		}

		for (i = 0; i < adapter->num_tx_queues; i++) {
			/* clone ring and setup updated count */
			tx_ring[i] = *adapter->tx_ring[i];
			tx_ring[i].count = new_tx_count;
			err = ngbe_setup_tx_resources(&tx_ring[i]);
			if (err) {
				while (i) {
					i--;
					ngbe_free_tx_resources(&tx_ring[i]);
				}

				vfree(tx_ring);
				tx_ring = NULL;

				goto clear_reset;
			}
		}
	}

	if (new_rx_count != adapter->rx_ring_count) {
		rx_ring = vmalloc(adapter->num_rx_queues * sizeof(*rx_ring));
		if (!rx_ring) {
			err = -ENOMEM;
			goto clear_reset;
		}

		for (i = 0; i < adapter->num_rx_queues; i++) {
			/* clone ring and setup updated count */
			rx_ring[i] = *adapter->rx_ring[i];
			rx_ring[i].count = new_rx_count;
			err = ngbe_setup_rx_resources(adapter, &rx_ring[i]);
			if (err) {
				while (i) {
					i--;
					ngbe_free_rx_resources(&rx_ring[i]);
				}

				vfree(rx_ring);
				rx_ring = NULL;

				goto clear_reset;
			}
		}
	}

	/* bring interface down to prepare for update */
	/*ngbe_down +  free_irq*/
	ngbe_down(adapter);
	ngbe_free_irq(adapter);

	/* Tx */
	if (tx_ring) {
		for (i = 0; i < adapter->num_tx_queues; i++) {
			ngbe_free_tx_resources(adapter->tx_ring[i]);
			*adapter->tx_ring[i] = tx_ring[i];
		}
		adapter->tx_ring_count = new_tx_count;

		vfree(tx_ring);
		tx_ring = NULL;
	}

	/* Rx */
	if (rx_ring) {
		for (i = 0; i < adapter->num_rx_queues; i++) {
			ngbe_free_rx_resources(adapter->rx_ring[i]);
			*adapter->rx_ring[i] = rx_ring[i];
		}
		adapter->rx_ring_count = new_rx_count;

		vfree(rx_ring);
		rx_ring = NULL;
	}

	/* restore interface using new values */
	/*ngbe_up +  request_irq*/
	ngbe_configure(adapter);
	ngbe_request_irq(adapter);
	ngbe_up_complete(adapter);

clear_reset:
	/* free Tx resources if Rx error is encountered */
	if (tx_ring) {
		for (i = 0; i < adapter->num_tx_queues; i++)
			ngbe_free_tx_resources(&tx_ring[i]);
		vfree(tx_ring);
	}

	clear_bit(__NGBE_RESETTING, &adapter->state);
	return err;
}

static void ngbe_get_ethtool_stats(struct net_device *netdev,
				      struct ethtool_stats __always_unused *stats,
				      u64 *data)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	ngbe_net_stats_t *net_stats = &adapter->net_stats;
	struct ngbe_ring *ring;
	int i = 0, j;
	char *p;
#ifdef HAVE_NDO_GET_STATS64
	unsigned int start;
#endif

#ifdef HAVE_NDO_GET_STATS64
		struct rtnl_link_stats64 temp;
	//	const struct rtnl_link_stats64 *net_stats;
	//	unsigned int start;
#else
		struct net_device_stats *net_stats;
#ifdef HAVE_NET_DEVICE_OPS
		const struct net_device_ops *ops = netdev->netdev_ops;
#endif /* HAVE_NET_DEVICE_OPS */
#endif /* HAVE_NDO_GET_STATS64 */


	ngbe_update_stats(adapter);
#ifdef HAVE_NDO_GET_STATS64
		net_stats = dev_get_stats(netdev, &temp);
#else
#ifdef HAVE_NET_DEVICE_OPS
		net_stats = ops->ndo_get_stats(netdev);
#else
		net_stats = netdev->get_stats(netdev);
#endif /* HAVE_NET_DEVICE_OPS */
#endif /* HAVE_NDO_GET_STATS64 */
	for (j = 0; j < NGBE_NET_STATS_LEN; j++) {
		p = (char *)net_stats + ngbe_net_stats[j].offset;
		data[i++] = (ngbe_net_stats[j].size == sizeof(u64))
			? *(u64 *)p : *(u32 *)p;
	}

	for (j = 0; j < NGBE_SW_STATS_LEN; j++) {
		p = (char *)adapter + ngbe_sw_stats[j].offset;
		data[i++] = (ngbe_sw_stats[j].size == sizeof(u64))
			? *(u64 *)p : *(u32 *)p;
	}

	for (j = 0; j < NGBE_HW_STATS_LEN; j++) {
		p = (char *)adapter + ngbevf_hw_stats[j].offset;
		data[i++] = (ngbevf_hw_stats[j].size == sizeof(u64))
			? *(u64 *)p : *(u32 *)p;
	}

	/* populate Tx queue data */
	for (j = 0; j < adapter->num_tx_queues; j++) {
		ring = adapter->tx_ring[j];
		if (!ring) {
			data[i++] = 0;
			data[i++] = 0;
#ifdef BP_EXTENDED_STATS
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
#endif
			continue;
		}

#ifdef HAVE_NDO_GET_STATS64
		do {
			start = u64_stats_fetch_begin_irq(&ring->syncp);
#endif
			data[i]   = ring->stats.packets;
			data[i+1] = ring->stats.bytes;
#ifdef HAVE_NDO_GET_STATS64
		} while (u64_stats_fetch_retry_irq(&ring->syncp, start));
#endif
		i += 2;
#ifdef BP_EXTENDED_STATS
		data[i] = ring->stats.yields;
		data[i+1] = ring->stats.misses;
		data[i+2] = ring->stats.cleaned;
		i += 3;
#endif
	}

	/* populate Rx queue data */
	for (j = 0; j < adapter->num_rx_queues; j++) {
		ring = adapter->rx_ring[j];
		if (!ring) {
			data[i++] = 0;
			data[i++] = 0;
#ifdef BP_EXTENDED_STATS
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
#endif
			continue;
		}

#ifdef HAVE_NDO_GET_STATS64
		do {
			start = u64_stats_fetch_begin_irq(&ring->syncp);
#endif
			data[i]   = ring->stats.packets;
			data[i+1] = ring->stats.bytes;
#ifdef HAVE_NDO_GET_STATS64
		} while (u64_stats_fetch_retry_irq(&ring->syncp, start));
#endif

		i += 2;
#ifdef BP_EXTENDED_STATS
		data[i] = ring->stats.yields;
		data[i+1] = ring->stats.misses;
		data[i+2] = ring->stats.cleaned;
		i += 3;
#endif
	}
}

static void ngbe_get_strings(struct net_device *netdev, u32 stringset,
				u8 *data)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	char *p = (char *)data;
	int i;

	switch (stringset) {
	case ETH_SS_TEST:
		memcpy(data, *ngbe_gstrings_test,
		       NGBE_TEST_LEN * ETH_GSTRING_LEN);
		break;
	case ETH_SS_STATS:
		for (i = 0; i < NGBE_NET_STATS_LEN; i++) {
			snprintf(p, ETH_GSTRING_LEN, "%-24s",
				 ngbe_net_stats[i].name);
			p += ETH_GSTRING_LEN;
		}

		for (i = 0; i < NGBE_SW_STATS_LEN; i++) {
			snprintf(p, ETH_GSTRING_LEN, "%-24s",
				 ngbe_sw_stats[i].name);
			p += ETH_GSTRING_LEN;
		}

		for (i = 0; i < NGBE_HW_STATS_LEN; i++) {
			snprintf(p, ETH_GSTRING_LEN, "%-24s",
				 ngbevf_hw_stats[i].name);
			p += ETH_GSTRING_LEN;
		}

		for (i = 0; i < adapter->num_tx_queues; i++) {
			sprintf(p, "txq_%0u_%-18s", i, "packets");
			p += ETH_GSTRING_LEN;
			sprintf(p, "txq_%0u_%-18s", i, "bytes");
			p += ETH_GSTRING_LEN;
#ifdef BP_EXTENDED_STATS
			sprintf(p, "txq_%0u_%-18s", i, "bp_napi_yield");
			p += ETH_GSTRING_LEN;
			sprintf(p, "txq_%0u_%-18s", i, "bp_misses");
			p += ETH_GSTRING_LEN;
			sprintf(p, "txq_%0u_%-18s", i, "bp_cleaned");
			p += ETH_GSTRING_LEN;
#endif
		}
		for (i = 0; i < adapter->num_rx_queues; i++) {
			sprintf(p, "rxq_%0u_%-18s", i, "packets");
			p += ETH_GSTRING_LEN;
			sprintf(p, "rxq_%0u_%-18s", i, "bytes");
			p += ETH_GSTRING_LEN;
#ifdef BP_EXTENDED_STATS
			sprintf(p, "rxq_%0u_%-18s", i, "bp_poll_yield");
			p += ETH_GSTRING_LEN;
			sprintf(p, "rxq_%0u_%-18s", i, "bp_misses");
			p += ETH_GSTRING_LEN;
			sprintf(p, "rxq_%0u_%-18s", i, "bp_cleaned");
			p += ETH_GSTRING_LEN;
#endif /* BP_EXTENDED_STATS */
		}
		break;
	}
}

static int ngbe_link_test(struct ngbevf_adapter *adapter, u64 *data)
{
	struct ngbevf_hw *hw = &adapter->hw;
	bool link_up;
	u32 link_speed = 0;
	*data = 0;

	TCALL(hw, mac.ops.check_link, &link_speed, &link_up, true);
	if (link_up)
		return *data;
	else
		*data = 1;
	return *data;
}

static int ngbe_reg_test(struct ngbevf_adapter *adapter, u64 *data)
{
	struct ngbevf_hw *hw = &adapter->hw;

	*data = ngbe_diag_reg_test(hw);

	return *data;
}

#ifdef HAVE_ETHTOOL_GET_SSET_COUNT
static int ngbe_get_sset_count(struct net_device *netdev, int stringset)
{
	switch(stringset) {
	case ETH_SS_TEST:
		return NGBE_TEST_LEN;
	case ETH_SS_STATS:
		return NGBE_STATS_LEN;
	default:
		return -EINVAL;
	}
}
#else
static int ngbe_diag_test_count(struct net_device __always_unused *netdev)
{
	return NGBE_TEST_LEN;
}

static int ngbe_get_stats_count(struct net_device *netdev)
{
	return NGBE_STATS_LEN;
}
#endif

static void ngbe_diag_test(struct net_device *netdev,
			      struct ethtool_test *eth_test, u64 *data)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	bool if_running = netif_running(netdev);

	if (NGBE_REMOVED(adapter->hw.hw_addr)) {
		DPRINTK(DRV, ERR, "Adapter removed - test blocked\n");
		eth_test->flags |= ETH_TEST_FL_FAILED;
		data[0] = 1;
		data[1] = 1;
		return;
	}
	set_bit(__NGBE_TESTING, &adapter->state);
	if (eth_test->flags == ETH_TEST_FL_OFFLINE) {
		/* Offline tests */

		DPRINTK(HW, INFO, "offline testing starting\n");

		/* Link test performed before hardware reset so autoneg doesn't
		 * interfere with test result */
		if (ngbe_link_test(adapter, &data[1]))
			eth_test->flags |= ETH_TEST_FL_FAILED;

		if (if_running)
			/* indicate we're in test mode */
			ngbe_close(netdev);
		else
			ngbe_reset(adapter);

		DPRINTK(HW, INFO, "register testing starting\n");
		if (ngbe_reg_test(adapter, &data[0]))
			eth_test->flags |= ETH_TEST_FL_FAILED;

		ngbe_reset(adapter);

		clear_bit(__NGBE_TESTING, &adapter->state);
		if (if_running)
			ngbe_open(netdev);
	} else {
		DPRINTK(HW, INFO, "online testing starting\n");
		/* Online tests */
		if (ngbe_link_test(adapter, &data[1]))
			eth_test->flags |= ETH_TEST_FL_FAILED;

		/* Online tests aren't run; pass by default */
		data[0] = 0;

		clear_bit(__NGBE_TESTING, &adapter->state);
	}
	msleep_interruptible(4 * 1000);
}

static int ngbe_nway_reset(struct net_device *netdev)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);

	if (netif_running(netdev))
		ngbevf_reinit_locked(adapter);

	return 0;
}

static int ngbe_get_coalesce(struct net_device *netdev,
				struct ethtool_coalesce *ec)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);

	/* only valid if in constant ITR mode */
	ec->rx_coalesce_usecs = adapter->rx_itr_setting > 1
				  ? adapter->rx_itr_setting << 1
				  : adapter->rx_itr_setting;

	/* if in mixed tx/rx queues per vector mode, report only rx settings */
	if (adapter->q_vector[0]->tx.count && adapter->q_vector[0]->rx.count)
		return 0;

	/* only valid if in constant ITR mode */
	ec->tx_coalesce_usecs = adapter->tx_itr_setting > 1
				  ? adapter->tx_itr_setting << 1
				  : adapter->tx_itr_setting;

	return 0;
}

static int ngbe_set_coalesce(struct net_device *netdev,
				struct ethtool_coalesce *ec)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	struct ngbe_q_vector *q_vector;
	int i;
	u16 tx_itr_param, rx_itr_param;

	/* don't accept tx specific changes if we've got mixed RxTx vectors */
	if (adapter->q_vector[0]->tx.count && adapter->q_vector[0]->rx.count
	    && ec->tx_coalesce_usecs)
		return -EINVAL;

	if ((ec->rx_coalesce_usecs > NGBE_VXITR_INTERVAL(~0) >> 1) ||
	    (ec->tx_coalesce_usecs > NGBE_VXITR_INTERVAL(~0) >> 1))
		return -EINVAL;

	adapter->rx_itr_setting = ec->rx_coalesce_usecs > 1
				    ? ec->rx_coalesce_usecs >> 1
				    : ec->rx_coalesce_usecs;

	if (adapter->rx_itr_setting == 1)
		rx_itr_param = NGBE_20K_ITR;
	else
		rx_itr_param = adapter->rx_itr_setting;

	adapter->tx_itr_setting = ec->tx_coalesce_usecs > 1
				    ? ec->tx_coalesce_usecs >> 1
				    : ec->tx_coalesce_usecs;

	if (adapter->tx_itr_setting == 1)
		tx_itr_param = NGBE_12K_ITR;
	else
		tx_itr_param = adapter->tx_itr_setting;

	for (i = 0; i < adapter->num_q_vectors; i++) {
		q_vector = adapter->q_vector[i];
		if (q_vector->tx.count && !q_vector->rx.count)
			/* tx only */
			q_vector->itr = tx_itr_param;
		else
			/* rx only or mixed */
			q_vector->itr = rx_itr_param;
		ngbe_write_eitr(q_vector);
	}

	return 0;
}

#ifdef ETHTOOL_GRXRINGS
#define UDP_RSS_FLAGS (NGBE_F_ENA_RSS_IPV4UDP | \
		       NGBE_F_ENA_RSS_IPV6UDP)
//emerald not support per vm rss
#if 0
static int ngbe_set_rss_hash_opt(struct ngbe_adapter *adapter,
				    struct ethtool_rxnfc *nfc)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 flags = adapter->flagsd;

	/*
	 * RSS does not support anything other than hashing
	 * to queues on src and dst IPs and ports
	 */
	if (nfc->data & ~(RXH_IP_SRC | RXH_IP_DST |
			  RXH_L4_B_0_1 | RXH_L4_B_2_3))
		return -EINVAL;

	switch (nfc->flow_type) {
	case TCP_V4_FLOW:
	case TCP_V6_FLOW:
		if (!(nfc->data & RXH_IP_SRC) ||
		    !(nfc->data & RXH_IP_DST) ||
		    !(nfc->data & RXH_L4_B_0_1) ||
		    !(nfc->data & RXH_L4_B_2_3))
			return -EINVAL;
		break;
	case UDP_V4_FLOW:
		if (!(nfc->data & RXH_IP_SRC) ||
		    !(nfc->data & RXH_IP_DST))
			return -EINVAL;
		switch (nfc->data & (RXH_L4_B_0_1 | RXH_L4_B_2_3)) {
		case 0:
			flags &= ~NGBE_F_ENA_RSS_IPV4UDP;
			break;
		case (RXH_L4_B_0_1 | RXH_L4_B_2_3):
			flags |= NGBE_F_ENA_RSS_IPV4UDP;
			break;
		default:
			return -EINVAL;
		}
		break;
	case UDP_V6_FLOW:
		if (!(nfc->data & RXH_IP_SRC) ||
		    !(nfc->data & RXH_IP_DST))
			return -EINVAL;
		switch (nfc->data & (RXH_L4_B_0_1 | RXH_L4_B_2_3)) {
		case 0:
			flags &= ~NGBE_F_ENA_RSS_IPV6UDP;
			break;
		case (RXH_L4_B_0_1 | RXH_L4_B_2_3):
			flags |= NGBE_F_ENA_RSS_IPV6UDP;
			break;
		default:
			return -EINVAL;
		}
		break;
	case AH_ESP_V4_FLOW:
	case AH_V4_FLOW:
	case ESP_V4_FLOW:
	case SCTP_V4_FLOW:
	case AH_ESP_V6_FLOW:
	case AH_V6_FLOW:
	case ESP_V6_FLOW:
	case SCTP_V6_FLOW:
		if (!(nfc->data & RXH_IP_SRC) ||
		    !(nfc->data & RXH_IP_DST) ||
		    (nfc->data & RXH_L4_B_0_1) ||
		    (nfc->data & RXH_L4_B_2_3))
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}

	/* if we changed something we need to update flags */
	if (flags != adapter->flagsd) {
		u32 vfmrqc = 0;

		if ((flags & UDP_RSS_FLAGS) &&
		    !(adapter->flagsd & UDP_RSS_FLAGS))
			DPRINTK(DRV, WARNING, "enabling UDP RSS: fragmented packets may arrive out of order to the stack above\n");

		adapter->flagsd = flags;

		/* Perform hash on these packet types */
		vfmrqc |= NGBE_VXMRQC_RSS_ALG_IPV4 |
			  NGBE_VXMRQC_RSS_ALG_IPV4_TCP |
			  NGBE_VXMRQC_RSS_ALG_IPV6 |
			  NGBE_VXMRQC_RSS_ALG_IPV6_TCP;

		if (flags & NGBE_F_ENA_RSS_IPV4UDP)
			vfmrqc |= NGBE_VXMRQC_RSS_ALG_IPV4_UDP;

		if (flags & NGBE_F_ENA_RSS_IPV6UDP)
			vfmrqc |= NGBE_VXMRQC_RSS_ALG_IPV6_UDP;

		wr32m(hw, NGBE_VXMRQC, NGBE_VXMRQC_RSS(~0),
			NGBE_VXMRQC_RSS(vfmrqc));
	}

	return 0;
}

static int ngbe_get_rss_hash_opts(struct ngbe_adapter *adapter,
				   struct ethtool_rxnfc *cmd)
{
	cmd->data = 0;

	/* Report default options for RSS on ngbevf */
	switch (cmd->flow_type) {
	case TCP_V4_FLOW:
		cmd->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		/* fall through */
	case UDP_V4_FLOW:
		if (adapter->flagsd & NGBE_F_ENA_RSS_IPV4UDP)
			cmd->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		/* fall through */
	case SCTP_V4_FLOW:
	case AH_ESP_V4_FLOW:
	case AH_V4_FLOW:
	case ESP_V4_FLOW:
	case IPV4_FLOW:
		cmd->data |= RXH_IP_SRC | RXH_IP_DST;
		break;
	case TCP_V6_FLOW:
		cmd->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		/* fall through */
	case UDP_V6_FLOW:
		if (adapter->flagsd & NGBE_F_ENA_RSS_IPV6UDP)
			cmd->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		/* fall through */
	case SCTP_V6_FLOW:
	case AH_ESP_V6_FLOW:
	case AH_V6_FLOW:
	case ESP_V6_FLOW:
	case IPV6_FLOW:
		cmd->data |= RXH_IP_SRC | RXH_IP_DST;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
#endif
static int ngbe_set_rxnfc(struct net_device *dev, struct ethtool_rxnfc *cmd)
{
	//struct ngbe_adapter *adapter = netdev_priv(dev);
	int ret = -EOPNOTSUPP;
//emerald not support per vm rss
#if 0
	if (cmd->cmd == ETHTOOL_SRXFH)
		ret = ngbe_set_rss_hash_opt(adapter, cmd);
#endif
	return ret;
}

static int ngbe_get_rxnfc(struct net_device *dev, struct ethtool_rxnfc *info,
#ifdef HAVE_ETHTOOL_GET_RXNFC_VOID_RULE_LOCS
			     __always_unused void *rule_locs)
#else
			     __always_unused u32 *rule_locs)
#endif
{
	struct ngbevf_adapter *adapter = netdev_priv(dev);
	//int ret;

	switch (info->cmd) {
	case ETHTOOL_GRXRINGS:
		info->data = adapter->num_rx_queues;
		break;
//emerald not support per vm rss
#if 0
	case ETHTOOL_GRXFH:
		ret = ngbe_get_rss_hash_opts(adapter, info);
		if (ret)
			return ret;
		break;
#endif
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}
#endif /* ETHTOOL_GRXRINGS */

#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)
static u32 ngbe_get_rxfh_indir_size(struct net_device *netdev)
{
	return NGBE_VFRETA_SIZE;
}

static u32 ngbe_get_rxfh_key_size(struct net_device *netdev)
{
	return NGBE_RSS_HASH_KEY_SIZE;
}

#ifdef HAVE_RXFH_HASHFUNC
static int ngbe_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key,
			    u8 *hfunc)
#else
static int ngbe_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key)
#endif
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	int err = 0;

#ifdef HAVE_RXFH_HASHFUNC
	if (hfunc)
		*hfunc = ETH_RSS_HASH_TOP;
#endif

	if (key)
		memcpy(key, adapter->rss_key, sizeof(adapter->rss_key));

	if (indir) {
		int i;

		for (i = 0; i < NGBE_VFRETA_SIZE; i++)
			indir[i] = adapter->rss_indir_tbl[i];
	}

	return err;
}
#endif /* ETHTOOL_GRSSH && ETHTOOL_SRSSH */

static struct ethtool_ops ngbe_ethtool_ops = {
#ifdef HAVE_ETHTOOL_CONVERT_U32_AND_LINK_MODE
	.get_link_ksettings	= ngbe_get_link_ksettings,
	.set_link_ksettings	= ngbe_set_link_ksettings,
#else
	.get_settings           = ngbe_get_settings,
	.set_settings           = ngbe_set_settings,
#endif
	.get_drvinfo            = ngbe_get_drvinfo,
	.get_regs_len           = ngbe_get_regs_len,
	.get_regs               = ngbe_get_regs,
	.nway_reset             = ngbe_nway_reset,
	.get_link               = ethtool_op_get_link,
	.get_eeprom             = ngbe_get_eeprom,
	.set_eeprom             = ngbe_set_eeprom,
#ifndef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
#ifdef HAVE_ETHTOOL_GET_TS_INFO
	.get_ts_info            = ethtool_op_get_ts_info,
#endif /* HAVE_ETHTOOL_GET_TS_INFO */
#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)
	.get_rxfh_indir_size	= ngbe_get_rxfh_indir_size,
	.get_rxfh_key_size	= ngbe_get_rxfh_key_size,
	.get_rxfh		= ngbe_get_rxfh,
#endif /* ETHTOOL_GRSSH && ETHTOOL_SRSSH */
#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */
	.get_ringparam          = ngbe_get_ringparam,
	.set_ringparam          = ngbe_set_ringparam,
	.get_msglevel           = ngbe_get_msglevel,
	.set_msglevel           = ngbe_set_msglevel,
#ifndef HAVE_NDO_SET_FEATURES
	.get_rx_csum            = ngbe_get_rx_csum,
	.set_rx_csum            = ngbe_set_rx_csum,
	.get_tx_csum            = ngbe_get_tx_csum,
	.set_tx_csum            = ngbe_set_tx_csum,
	.get_sg                 = ethtool_op_get_sg,
	.set_sg                 = ethtool_op_set_sg,
#ifdef NETIF_F_TSO
	.get_tso                = ethtool_op_get_tso,
	.set_tso                = ngbe_set_tso,
#endif
#endif
	.self_test              = ngbe_diag_test,
#ifdef HAVE_ETHTOOL_GET_SSET_COUNT
	.get_sset_count         = ngbe_get_sset_count,
#else
	.self_test_count        = ngbe_diag_test_count,
	.get_stats_count        = ngbe_get_stats_count,
#endif
	.get_strings            = ngbe_get_strings,
	.get_ethtool_stats      = ngbe_get_ethtool_stats,
#ifdef HAVE_ETHTOOL_GET_PERM_ADDR
	.get_perm_addr          = ethtool_op_get_perm_addr,
#endif
	.get_coalesce           = ngbe_get_coalesce,
	.set_coalesce           = ngbe_set_coalesce,
#ifdef ETHTOOL_GRXRINGS
	.get_rxnfc              = ngbe_get_rxnfc,
	.set_rxnfc              = ngbe_set_rxnfc,
#endif
};

#ifdef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
static const struct ethtool_ops_ext ngbe_ethtool_ops_ext = {
	.size           = sizeof(struct ethtool_ops_ext),
	.get_ts_info    = ethtool_op_get_ts_info,
#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)
	.get_rxfh_indir_size	= ngbe_get_rxfh_indir_size,
	.get_rxfh_key_size	= ngbe_get_rxfh_key_size,
	.get_rxfh		= ngbe_get_rxfh,
#endif /* ETHTOOL_GRSSH && ETHTOOL_SRSSH */
};
#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */

void ngbe_set_ethtool_ops(struct net_device *netdev)
{
#ifndef ETHTOOL_OPS_COMPAT
	netdev->ethtool_ops = &ngbe_ethtool_ops;
#else
	SET_ETHTOOL_OPS(netdev, &ngbe_ethtool_ops);
#endif

#ifdef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
	set_ethtool_ops_ext(netdev, &ngbe_ethtool_ops_ext);
#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */
}

