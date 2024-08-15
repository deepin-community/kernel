// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#include "ne6xvf.h"
#include "ne6xvf_ethtool_stats.h"
#include "ne6xvf_txrx.h"

static const char ne6xvf_gstrings_test[][ETH_GSTRING_LEN] = {
	"Register test	(offline)",
	"Eeprom test	(offline)",
	"Interrupt test (offline)",
	"Link test	 (on/offline)"
};

#define NE6XVF_TEST_LEN (sizeof(ne6xvf_gstrings_test) / ETH_GSTRING_LEN)

static int ne6xvf_q_stats_len(struct net_device *netdev)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);
	int stats_size, total_slen = 0;

	/* Tx stats */
	stats_size = sizeof(struct ne6x_q_stats) + sizeof(struct ne6x_txq_stats);
	total_slen += adapter->num_active_queues * (stats_size / sizeof(u64));

	/* Rx stats */
	stats_size = sizeof(struct ne6x_q_stats) + sizeof(struct ne6x_rxq_stats);
	total_slen += adapter->num_active_queues * (stats_size / sizeof(u64));

	/* CQ stats */
	stats_size = sizeof(struct ne6x_cq_stats);
	total_slen += adapter->num_active_queues * (stats_size / sizeof(u64));

	return total_slen;
}

struct ne6xvf_stats {
	char stat_string[ETH_GSTRING_LEN];
	int sizeof_stat;
	int stat_offset;
};

/* Helper macro for defining some statistics directly copied from the netdev
 * stats structure.
 */
#define NE6XVF_NETDEV_STAT(_net_stat) NE6XVF_STAT(struct rtnl_link_stats64, #_net_stat, _net_stat)

/* per-queue ring statistics */
#define NE6XVF_QUEUE_STAT(_name, _stat) NE6XVF_STAT(struct ne6x_ring, _name, _stat)

static const struct ne6xvf_stats ne6xvf_gstrings_tx_queue_stats[] = {
	NE6XVF_QUEUE_STAT("tx_queue_%u_packets",       stats.packets),
	NE6XVF_QUEUE_STAT("tx_queue_%u_bytes",         stats.bytes),
	NE6XVF_QUEUE_STAT("tx_queue_%u_rst",           tx_stats.restart_q),
	NE6XVF_QUEUE_STAT("tx_queue_%u_busy",          tx_stats.tx_busy),
	NE6XVF_QUEUE_STAT("tx_queue_%u_line",          tx_stats.tx_linearize),
	NE6XVF_QUEUE_STAT("tx_queue_%u_csum_err",      tx_stats.csum_err),
	NE6XVF_QUEUE_STAT("tx_queue_%u_csum",          tx_stats.csum_good),
	NE6XVF_QUEUE_STAT("tx_queue_%u_pcie_read_err", tx_stats.tx_pcie_read_err),
	NE6XVF_QUEUE_STAT("tx_queue_%u_ecc_err",       tx_stats.tx_ecc_err),
	NE6XVF_QUEUE_STAT("tx_queue_%u_drop_addr",     tx_stats.tx_drop_addr),
};

static const struct ne6xvf_stats ne6xvf_gstrings_rx_queue_stats[] = {
	NE6XVF_QUEUE_STAT("rx_queue_%u_packets",       stats.packets),
	NE6XVF_QUEUE_STAT("rx_queue_%u_bytes",         stats.bytes),
	NE6XVF_QUEUE_STAT("rx_queue_%u_no_eop",        rx_stats.non_eop_descs),
	NE6XVF_QUEUE_STAT("rx_queue_%u_alloc_pg_err",  rx_stats.alloc_page_failed),
	NE6XVF_QUEUE_STAT("rx_queue_%u_alloc_buf_err", rx_stats.alloc_buf_failed),
	NE6XVF_QUEUE_STAT("rx_queue_%u_pg_reuse",      rx_stats.page_reuse_count),
	NE6XVF_QUEUE_STAT("rx_queue_%u_csum_err",      rx_stats.csum_err),
	NE6XVF_QUEUE_STAT("rx_queue_%u_csum",          rx_stats.csum_good),
	NE6XVF_QUEUE_STAT("rx_queue_%u_mem_err",       rx_stats.rx_mem_error),
	NE6XVF_QUEUE_STAT("rx_queue_%u_rx_err",        rx_stats.rx_err),
};

static const struct ne6xvf_stats ne6xvf_gstrings_cq_queue_stats[] = {
	NE6XVF_QUEUE_STAT("cx_queue_%u_nums",          cq_stats.cq_num),
	NE6XVF_QUEUE_STAT("cx_queue_%u_tx_nums",       cq_stats.tx_num),
	NE6XVF_QUEUE_STAT("cx_queue_%u_rx_nums",       cq_stats.rx_num),
};

/* port mac statistics */
#define NE6XVF_PORT_MAC_STAT(_name, _stat) NE6XVF_STAT(struct ne6xvf_vsi, _name, _stat)

#define NE6XVF_ALL_STATS_LEN(n) (ne6xvf_q_stats_len(n))

#define ne6xvf_ethtool_advertise_link_mode(aq_link_speed, ethtool_link_mode) \
	ethtool_link_ksettings_add_link_mode(ks, advertising, ethtool_link_mode)

static void ne6xvf_get_settings_link_up(struct ethtool_link_ksettings *ks,
					struct net_device *netdev)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);

	switch (adapter->link_speed) {
	case NE6X_LINK_SPEED_100GB:
		ks->base.speed = SPEED_100000;
		break;
	case NE6X_LINK_SPEED_40GB:
		ks->base.speed = SPEED_40000;
		break;
	case NE6X_LINK_SPEED_25GB:
		ks->base.speed = SPEED_25000;
		break;
	case NE6X_LINK_SPEED_10GB:
		ks->base.speed = SPEED_10000;
		break;
	case NE6X_LINK_SPEED_200GB:
		ks->base.speed = SPEED_200000;
		break;
	default:
		netdev_info(netdev, "WARNING: Unrecognized link_speed (0x%x).\n",
			    adapter->link_speed);
		break;
	}
	ks->base.duplex = DUPLEX_FULL;
}

/**
 * ne6xvf_get_settings_link_down - Get the Link settings when link is down
 * @ks: ethtool ksettings to fill in
 * @netdev: network interface device structure
 *
 * Reports link settings that can be determined when link is down
 */
static void ne6xvf_get_settings_link_down(struct ethtool_link_ksettings *ks,
					  struct net_device *netdev)
{
	ks->base.speed = SPEED_UNKNOWN;
	ks->base.duplex = DUPLEX_UNKNOWN;
}

/**
 * ne6xvf_get_link_ksettings - Get Link Speed and Duplex settings
 * @netdev: network interface device structure
 * @ks: ethtool ksettings
 *
 * Reports speed/duplex settings based on media_type
 */
static int ne6xvf_get_link_ksettings(struct net_device *netdev, struct ethtool_link_ksettings *ks)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);

	ethtool_link_ksettings_zero_link_mode(ks, supported);
	ethtool_link_ksettings_zero_link_mode(ks, advertising);
	ethtool_link_ksettings_zero_link_mode(ks, lp_advertising);

	ks->base.port = PORT_NONE;
	if (adapter->link_up) {
		/* Set flow control settings */
		ne6xvf_get_settings_link_up(ks, netdev);
	} else {
		ne6xvf_get_settings_link_down(ks, netdev);
	}

	return 0;
}

/**
 * ne6xvf_set_link_ksettings - Set Speed and Duplex
 * @netdev: network interface device structure
 * @ks: ethtool ksettings
 *
 * Set speed/duplex per media_types advertised/forced
 */
static int ne6xvf_set_link_ksettings(struct net_device *netdev,
				     const struct ethtool_link_ksettings *ks)
{
	return -EOPNOTSUPP;
}

static void __ne6xvf_add_stat_strings(u8 **p, const struct ne6xvf_stats stats[],
				      const unsigned int size, ...)
{
	unsigned int i;

	for (i = 0; i < size; i++) {
		va_list args;

		va_start(args, size);
		vsnprintf(*p, ETH_GSTRING_LEN, stats[i].stat_string, args);
		*p += ETH_GSTRING_LEN;
		va_end(args);
	}
}

#define ne6xvf_add_stat_strings(p, stats, ...) \
	__ne6xvf_add_stat_strings(p, stats, ARRAY_SIZE(stats), ##__VA_ARGS__)

static void ne6xvf_get_stat_strings(struct net_device *netdev, u8 *data)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);
	unsigned int i;

	for (i = 0; i < adapter->num_active_queues; i++) {
		ne6xvf_add_stat_strings(&data, ne6xvf_gstrings_tx_queue_stats, i);
		ne6xvf_add_stat_strings(&data, ne6xvf_gstrings_rx_queue_stats, i);
		ne6xvf_add_stat_strings(&data, ne6xvf_gstrings_cq_queue_stats, i);
	}
}

static void ne6xvf_get_strings(struct net_device *netdev, u32 stringset, u8 *data)
{
	switch (stringset) {
	case ETH_SS_STATS:
		ne6xvf_get_stat_strings(netdev, data);
		break;
	case ETH_SS_TEST:
		memcpy(data, ne6xvf_gstrings_test, NE6XVF_TEST_LEN * ETH_GSTRING_LEN);
	default:
		break;
	}
}

static int ne6xvf_get_sset_count(struct net_device *netdev, int sset)
{
	switch (sset) {
	case ETH_SS_STATS:
		/* The number (and order) of strings reported *must* remain
		 * constant for a given netdevice. This function must not
		 * report a different number based on run time parameters
		 * (such as the number of queues in use, or the setting of
		 * a private ethtool flag). This is due to the nature of the
		 * ethtool stats API.
		 *
		 * Userspace programs such as ethtool must make 3 separate
		 * ioctl requests, one for size, one for the strings, and
		 * finally one for the stats. Since these cross into
		 * userspace, changes to the number or size could result in
		 * undefined memory access or incorrect string<->value
		 * correlations for statistics.
		 *
		 * Even if it appears to be safe, changes to the size or
		 * order of strings will suffer from race conditions and are
		 * not safe.
		 */
		return NE6XVF_ALL_STATS_LEN(netdev);
	case ETH_SS_TEST:
		return NE6XVF_TEST_LEN;
	default:
		return -EOPNOTSUPP;
	}
}

static void ne6xvf_get_ethtool_stats(struct net_device *netdev,
				     struct ethtool_stats __always_unused *stats,
				     u64 *data)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);
	struct ne6x_ring *tx_ring;
	struct ne6x_ring *rx_ring;
	struct ne6x_ring *cq_ring;
	unsigned int j;
	int i = 0;

	ne6xvf_update_pf_stats(adapter);

	/* populate per queue stats */
	rcu_read_lock();
	for (j = 0; j < adapter->num_active_queues; j++) {
		tx_ring = &adapter->tx_rings[j];
		if (tx_ring) {
			data[i++] = tx_ring->stats.packets;
			data[i++] = tx_ring->stats.bytes;
			data[i++] = tx_ring->tx_stats.restart_q;
			data[i++] = tx_ring->tx_stats.tx_busy;
			data[i++] = tx_ring->tx_stats.tx_linearize;
			data[i++] = tx_ring->tx_stats.csum_err;
			data[i++] = tx_ring->tx_stats.csum_good;
			data[i++] = tx_ring->tx_stats.tx_pcie_read_err;
			data[i++] = tx_ring->tx_stats.tx_ecc_err;
			data[i++] = tx_ring->tx_stats.tx_drop_addr;
		} else {
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
		}

		rx_ring = &adapter->rx_rings[j];
		if (rx_ring) {
			data[i++] = rx_ring->stats.packets;
			data[i++] = rx_ring->stats.bytes;
			data[i++] = rx_ring->rx_stats.non_eop_descs;
			data[i++] = rx_ring->rx_stats.alloc_page_failed;
			data[i++] = rx_ring->rx_stats.alloc_buf_failed;
			data[i++] = rx_ring->rx_stats.page_reuse_count;
			data[i++] = rx_ring->rx_stats.csum_err;
			data[i++] = rx_ring->rx_stats.csum_good;
			data[i++] = rx_ring->rx_stats.rx_mem_error;
			data[i++] = rx_ring->rx_stats.rx_err;
		} else {
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
		}

		cq_ring = &adapter->cq_rings[j];
		if (cq_ring) {
			data[i++] = cq_ring->cq_stats.cq_num;
			data[i++] = cq_ring->cq_stats.tx_num;
			data[i++] = cq_ring->cq_stats.rx_num;
		} else {
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
		}
	}
	rcu_read_unlock();
}

static void ne6xvf_get_drvinfo(struct net_device *netdev, struct ethtool_drvinfo *drvinfo)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);

	strscpy(drvinfo->driver, ne6xvf_driver_name, sizeof(drvinfo->driver));
	strscpy(drvinfo->version, ne6xvf_driver_version, sizeof(drvinfo->version));
	strlcpy(drvinfo->fw_version, "N/A", 4);
	strlcpy(drvinfo->bus_info, pci_name(adapter->pdev), sizeof(drvinfo->bus_info));
}

static void ne6xvf_get_regs(struct net_device *netdev, struct ethtool_regs *regs, void *p) {}

static void ne6xvf_self_test(struct net_device *dev, struct ethtool_test *eth_test, u64 *data)
{
	memset(data, 0, sizeof(*data) * NE6XVF_TEST_LEN);
}

static int ne6xvf_get_regs_len(struct net_device *netdev)
{
	return 0;
}

static void ne6xvf_get_ringparam(struct net_device *netdev, struct ethtool_ringparam *ring,
				 struct kernel_ethtool_ringparam __always_unused *ker,
				 struct netlink_ext_ack __always_unused *extack)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);

	ring->rx_max_pending = NE6X_MAX_NUM_DESCRIPTORS;
	ring->tx_max_pending = NE6X_MAX_NUM_DESCRIPTORS;
	ring->rx_mini_max_pending = NE6X_MIN_NUM_DESCRIPTORS;
	ring->rx_jumbo_max_pending = 0;
	ring->rx_pending = adapter->rx_desc_count;
	ring->tx_pending = adapter->tx_desc_count;
	ring->rx_mini_pending = NE6X_MIN_NUM_DESCRIPTORS;
	ring->rx_jumbo_pending = 0;
}

static int ne6xvf_set_ringparam(struct net_device *netdev, struct ethtool_ringparam *ring,
				struct kernel_ethtool_ringparam __always_unused *ker,
				struct netlink_ext_ack __always_unused *extack)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);
	u32 new_rx_count, new_tx_count, new_cq_count;
	int err;

	if (ring->tx_pending > NE6X_MAX_NUM_DESCRIPTORS ||
	    ring->tx_pending < NE6X_MIN_NUM_DESCRIPTORS ||
	    ring->rx_pending > NE6X_MAX_NUM_DESCRIPTORS ||
	    ring->rx_pending < NE6X_MIN_NUM_DESCRIPTORS) {
		netdev_info(netdev, "Descriptors requested (Tx: %d / Rx: %d) out of range [%d-%d]\n",
			    ring->tx_pending, ring->rx_pending, NE6X_MIN_NUM_DESCRIPTORS,
			    NE6X_MAX_NUM_DESCRIPTORS);
		return -EINVAL;
	}

	new_tx_count = ALIGN(ring->tx_pending, NE6X_REQ_DESCRIPTOR_MULTIPLE);
	new_rx_count = ALIGN(ring->rx_pending, NE6X_REQ_DESCRIPTOR_MULTIPLE);
	new_cq_count = new_rx_count + new_rx_count;

	if (new_tx_count == adapter->tx_desc_count && new_rx_count == adapter->rx_desc_count)
		return 0;

	if (!netif_running(adapter->netdev)) {
		adapter->tx_desc_count = new_tx_count;
		adapter->rx_desc_count = new_rx_count;
		adapter->cq_desc_count = new_cq_count;
		netdev_info(netdev, "Link is down, queue count change happens when link is brought up\n");
		return 0;
	}

	err = ne6xvf_close(adapter->netdev);
	if (err) {
		netdev_err(netdev, "fail to close vf\n");
		return err;
	}
	netdev_info(netdev, "Descriptors change  from (Tx: %d / Rx: %d) to [%d-%d]\n",
		    adapter->tx_rings[0].count, adapter->rx_rings[0].count, new_tx_count,
		    new_rx_count);
	adapter->tx_desc_count = new_tx_count;
	adapter->rx_desc_count = new_rx_count;
	adapter->cq_desc_count = new_cq_count;

	err = ne6xvf_open(adapter->netdev);
	if (err) {
		netdev_err(netdev, "fail to open vf\n");
		return err;
	}

	return 0;
}

/**
 * ne6xvf_get_pauseparam -  Get Flow Control status
 * @netdev: netdevice structure
 * @pause: buffer to return pause parameters
 *
 * Return tx/rx-pause status
 **/
static void ne6xvf_get_pauseparam(struct net_device *netdev, struct ethtool_pauseparam *pause)
{
	pause->autoneg = 0;
	pause->rx_pause = 0;
	pause->tx_pause = 0;
}

/**
 * ne6xvf_get_coalesce - get a netdev's coalesce settings
 * @netdev: the netdev to check
 * @ec: ethtool coalesce data structure
 *
 **/
static int ne6xvf_get_coalesce(struct net_device *netdev,
			       struct ethtool_coalesce *ec,
			       struct kernel_ethtool_coalesce *kernel_coal,
			       struct netlink_ext_ack *extack)
{
	ec->tx_max_coalesced_frames_irq = 256;
	ec->rx_max_coalesced_frames_irq = 256;
	ec->use_adaptive_rx_coalesce = 0;
	ec->use_adaptive_tx_coalesce = 0;
	ec->rx_coalesce_usecs = 0;
	ec->tx_coalesce_usecs = 0;
	ec->rx_coalesce_usecs_high = 0;
	ec->tx_coalesce_usecs_high = 0;

	return 0;
}

static int ne6xvf_get_eeprom_len(struct net_device *netdev)
{
	return 0x64;
}

static int ne6xvf_set_phys_id(struct net_device *netdev, enum ethtool_phys_id_state state)
{
	int blink_freq = 2;

	switch (state) {
	case ETHTOOL_ID_ACTIVE:
		return blink_freq;
	case ETHTOOL_ID_ON:
		break;
	case ETHTOOL_ID_OFF:
		break;
	case ETHTOOL_ID_INACTIVE:
		break;
	default:
		break;
	}

	return 0;
}

static int ne6xvf_nway_reset(struct net_device *netdev)
{
	return 0;
}

static void ne6xvf_diag_test(struct net_device *netdev, struct ethtool_test *eth_test, u64 *data)
{
	data[NE6XVF_ETH_TEST_LINK] = 0;

	/* Offline only tests, not run in online; pass by default */
	data[NE6XVF_ETH_TEST_REG] = 0;
	data[NE6XVF_ETH_TEST_EEPROM] = 0;
	data[NE6XVF_ETH_TEST_INTR] = 0;
}

#define L3_RSS_FLAGS (RXH_IP_DST | RXH_IP_SRC)
#define L4_RSS_FLAGS (RXH_L4_B_0_1 | RXH_L4_B_2_3)
static int ne6xvf_get_rss_hash_opts(struct ne6xvf_adapter *adapter, u64 flow_type)
{
	u64 data = 0;

	switch (flow_type) {
	case TCP_V4_FLOW:
		if (adapter->rss_info.hash_type & NE6X_RSS_HASH_TYPE_IPV4)
			data |= RXH_IP_DST | RXH_IP_SRC;
		if (adapter->rss_info.hash_type & NE6X_RSS_HASH_TYPE_IPV4_TCP)
			data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		break;
	case UDP_V4_FLOW:
		if (adapter->rss_info.hash_type & NE6X_RSS_HASH_TYPE_IPV4)
			data |= RXH_IP_DST | RXH_IP_SRC;
		if (adapter->rss_info.hash_type & NE6X_RSS_HASH_TYPE_IPV4_UDP)
			data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		break;
	case TCP_V6_FLOW:
		if (adapter->rss_info.hash_type & NE6X_RSS_HASH_TYPE_IPV6)
			data |= RXH_IP_DST | RXH_IP_SRC;
		if (adapter->rss_info.hash_type & NE6X_RSS_HASH_TYPE_IPV6_TCP)
			data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		break;
	case UDP_V6_FLOW:
		if (adapter->rss_info.hash_type & NE6X_RSS_HASH_TYPE_IPV6)
			data |= RXH_IP_DST | RXH_IP_SRC;
		if (adapter->rss_info.hash_type & NE6X_RSS_HASH_TYPE_IPV6_UDP)
			data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		break;
	case SCTP_V4_FLOW:
	case AH_ESP_V4_FLOW:
	case AH_V4_FLOW:
	case ESP_V4_FLOW:
	case IPV4_FLOW:
	case SCTP_V6_FLOW:
	case AH_ESP_V6_FLOW:
	case AH_V6_FLOW:
	case ESP_V6_FLOW:
	case IPV6_FLOW:
		/* Default is src/dest for IP, no matter the L4 hashing */
		data |= RXH_IP_SRC | RXH_IP_DST;
		break;
	}

	return data;
}

static int ne6xvf_set_rss_hash_opts(struct ne6xvf_adapter *adapter, struct ethtool_rxnfc *cmd)
{
	u16 rss_flags = adapter->rss_info.hash_type;

	if (cmd->data != L3_RSS_FLAGS && cmd->data != (L3_RSS_FLAGS | L4_RSS_FLAGS))
		return -EINVAL;

	switch (cmd->flow_type) {
	case TCP_V4_FLOW:
		if (cmd->data == L3_RSS_FLAGS)
			rss_flags &= ~NE6X_RSS_HASH_TYPE_IPV4_TCP;
		else if (cmd->data == (L3_RSS_FLAGS | L4_RSS_FLAGS))
			rss_flags |= NE6X_RSS_HASH_TYPE_IPV4 | NE6X_RSS_HASH_TYPE_IPV4_TCP;
		break;
	case TCP_V6_FLOW:
		if (cmd->data == L3_RSS_FLAGS)
			rss_flags &= ~NE6X_RSS_HASH_TYPE_IPV6_TCP;
		else if (cmd->data == (L3_RSS_FLAGS | L4_RSS_FLAGS))
			rss_flags |= NE6X_RSS_HASH_TYPE_IPV6 | NE6X_RSS_HASH_TYPE_IPV6_TCP;
		break;
	case UDP_V4_FLOW:
		if (cmd->data == L3_RSS_FLAGS)
			rss_flags &= ~NE6X_RSS_HASH_TYPE_IPV4_UDP;
		else if (cmd->data == (L3_RSS_FLAGS | L4_RSS_FLAGS))
			rss_flags |= NE6X_RSS_HASH_TYPE_IPV4 | NE6X_RSS_HASH_TYPE_IPV4_UDP;
		break;
	case UDP_V6_FLOW:
		if (cmd->data == L3_RSS_FLAGS)
			rss_flags &= ~NE6X_RSS_HASH_TYPE_IPV6_UDP;
		else if (cmd->data == (L3_RSS_FLAGS | L4_RSS_FLAGS))
			rss_flags |= NE6X_RSS_HASH_TYPE_IPV6 | NE6X_RSS_HASH_TYPE_IPV6_UDP;
		break;
	default:
		return -EINVAL;
	}

	if (rss_flags == adapter->rss_info.hash_type)
		return 0;

	adapter->rss_info.hash_type = rss_flags;
	adapter->aq_required |= NE6XVF_FLAG_AQ_CONFIGURE_RSS;

	return 0;
}

/**
 * ne6xvf_set_rxnfc - command to set Rx flow rules.
 * @netdev: network interface device structure
 * @cmd: ethtool rxnfc command
 *
 * Returns 0 for success and negative values for errors
 */
static int ne6xvf_set_rxnfc(struct net_device *netdev, struct ethtool_rxnfc *info)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);
	int ret = -EOPNOTSUPP;

	switch (info->cmd) {
	case ETHTOOL_SRXFH:
		ret = ne6xvf_set_rss_hash_opts(adapter, info);
		break;
	default:
		break;
	}

	return ret;
}

/**
 * iavf_get_rxnfc - command to get RX flow classification rules
 * @netdev: network interface device structure
 * @cmd: ethtool rxnfc command
 * @rule_locs: pointer to store rule locations
 *
 * Returns Success if the command is supported.
 **/
static int ne6xvf_get_rxnfc(struct net_device *netdev, struct ethtool_rxnfc *cmd, u32 *rule_locs)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);
	int ret = -EOPNOTSUPP;

	switch (cmd->cmd) {
	case ETHTOOL_GRXRINGS:
		cmd->data = adapter->num_active_queues;
		ret = 0;
		break;
	case ETHTOOL_GRXFH:
		cmd->data = ne6xvf_get_rss_hash_opts(adapter, cmd->flow_type);
		break;
	default:
		break;
	}

	return 0;
}

/**
 * ne6xvf_get_rxfh_key_size - get the RSS hash key size
 * @netdev: network interface device structure
 *
 * Returns the table size.
 **/
static u32 ne6xvf_get_rxfh_key_size(struct net_device *netdev)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);

	return adapter->rss_info.hash_key_size;
}

/**
 * iavf_get_rxfh_indir_size - get the rx flow hash indirection table size
 * @netdev: network interface device structure
 *
 * Returns the table size.
 **/
static u32 ne6xvf_get_rxfh_indir_size(struct net_device *netdev)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);

	return adapter->rss_info.ind_table_size;
}

/**
 * ne6xvf_get_rxfh - get the rx flow hash indirection table
 * @netdev: network interface device structure
 * @indir: indirection table
 * @key: hash key
 * @hfunc: hash function in use
 *
 * Reads the indirection table directly from the hardware. Always returns 0.
 **/
static int ne6xvf_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key, u8 *hfunc)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);
	u16 i;

	if (hfunc)
		*hfunc = ETH_RSS_HASH_TOP;

	if (key)
		memcpy(key, adapter->rss_info.hash_key, adapter->rss_info.hash_key_size);

	if (indir) {
		/* Each 32 bits pointed by 'indir' is stored with a lut entry */
		for (i = 0; i < adapter->rss_info.ind_table_size; i++)
			indir[i] = (u32)adapter->rss_info.ind_table[i];
	}

	return 0;
}

/**
 * ne6xvf_set_rxfh - set the Rx flow hash indirection table
 * @netdev: network interface device structure
 * @indir: indirection table
 * @key: hash key
 * @hfunc: hash function
 *
 * Returns -EINVAL if the table specifies an invalid queue ID, otherwise
 * returns 0 after programming the table.
 */
static int ne6xvf_set_rxfh(struct net_device *netdev, const u32 *indir,
			   const u8 *key, const u8 hfunc)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);
	int i;

	if (hfunc != ETH_RSS_HASH_NO_CHANGE && hfunc != ETH_RSS_HASH_TOP)
		return -EOPNOTSUPP;

	if (!key && !indir)
		return 0;

	if (key)
		memcpy(&adapter->rss_info.hash_key[0], key, adapter->rss_info.hash_key_size);

	if (indir) {
		/* Each 32 bits pointed by 'indir' is stored with a lut entry */
		for (i = 0; i < adapter->rss_info.ind_table_size; i++)
			adapter->rss_info.ind_table[i] = (u8)(indir[i]);
	}

	adapter->aq_required |= NE6XVF_FLAG_AQ_CONFIGURE_RSS;

	return 0;
}

/**
 * iavf_get_channels: get the number of channels supported by the device
 * @netdev: network interface device structure
 * @ch: channel information structure
 *
 * For the purposes of our device, we only use combined channels, i.e. a tx/rx
 * queue pair. Report one extra channel to match our "other" MSI-X vector.
 **/
static void ne6xvf_get_channels(struct net_device *netdev, struct ethtool_channels *channels)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);

	channels->max_rx = 0;
	channels->max_tx = 0;
	channels->max_other = 0;
	channels->max_combined = adapter->max_queues;
	channels->rx_count = 0;
	channels->tx_count = 0;
	channels->other_count = 0;
	channels->combined_count = adapter->num_active_queues;
}

/**
 * ne6xvf_set_channels: set the new channel count
 * @netdev: network interface device structure
 * @ch: channel information structure
 *
 * Negotiate a new number of channels with the PF then do a reset.  During
 * reset we'll realloc queues and fix the RSS table.  Returns 0 on success,
 * negative on failure.
 **/
static int ne6xvf_set_channels(struct net_device *netdev, struct ethtool_channels *channels)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);
	int err = 0;

	if (!channels->combined_count || channels->rx_count || channels->tx_count ||
	    channels->combined_count > adapter->vf_res->num_queue_pairs)
		return -EINVAL;

	if (channels->rx_count == adapter->num_active_queues) {
		/* nothing to do */
		netdev_info(netdev, "channel not change, nothing to do!\n");
		return 0;
	}

	/* set for the next time the netdev is started */
	if (!netif_running(adapter->netdev)) {
		adapter->num_active_queues = channels->combined_count;

		netif_set_real_num_rx_queues(adapter->netdev, adapter->num_active_queues);
		netif_set_real_num_tx_queues(adapter->netdev, adapter->num_active_queues);

		ne6xvf_fill_rss_lut(adapter);
		adapter->aq_required |= NE6XVF_FLAG_AQ_CHANGED_RSS;

		netdev_info(netdev, "Link is down, queue count change happens when link is brought up\n");

		return 0;
	}

	err = ne6xvf_close(adapter->netdev);
	if (err) {
		netdev_err(netdev, "fail to close vf\n");
		return err;
	}

	adapter->num_active_queues = channels->combined_count;

	netif_set_real_num_rx_queues(adapter->netdev, adapter->num_active_queues);
	netif_set_real_num_tx_queues(adapter->netdev, adapter->num_active_queues);

	ne6xvf_fill_rss_lut(adapter);
	adapter->aq_required |= NE6XVF_FLAG_AQ_CHANGED_RSS;

	err = ne6xvf_open(adapter->netdev);
	if (err) {
		netdev_err(netdev, "fail to open vf\n");
		return err;
	}

	return 0;
}

static const struct ethtool_ops ne6xvf_ethtool_ops = {
	.get_link_ksettings  = ne6xvf_get_link_ksettings,
	.set_link_ksettings  = ne6xvf_set_link_ksettings,
	.get_strings         = ne6xvf_get_strings,
	.get_sset_count      = ne6xvf_get_sset_count,
	.get_ethtool_stats   = ne6xvf_get_ethtool_stats,
	.get_drvinfo         = ne6xvf_get_drvinfo,
	.get_link            = ethtool_op_get_link,
	.get_regs            = ne6xvf_get_regs,
	.get_regs_len        = ne6xvf_get_regs_len,
	.self_test           = ne6xvf_self_test,
	.get_ringparam       = ne6xvf_get_ringparam,
	.set_ringparam       = ne6xvf_set_ringparam,
	.get_pauseparam      = ne6xvf_get_pauseparam,
	.get_coalesce        = ne6xvf_get_coalesce,
	.get_eeprom_len      = ne6xvf_get_eeprom_len,
	.get_rxnfc           = ne6xvf_get_rxnfc,
	.set_rxnfc           = ne6xvf_set_rxnfc,
	.get_rxfh_key_size   = ne6xvf_get_rxfh_key_size,
	.get_rxfh_indir_size = ne6xvf_get_rxfh_indir_size,
	.get_rxfh            = ne6xvf_get_rxfh,
	.set_rxfh            = ne6xvf_set_rxfh,
	.get_channels        = ne6xvf_get_channels,
	.set_channels        = ne6xvf_set_channels,
	.set_phys_id         = ne6xvf_set_phys_id,
	.nway_reset          = ne6xvf_nway_reset,
	.self_test           = ne6xvf_diag_test,
};

void ne6xvf_set_ethtool_ops(struct net_device *dev)
{
	dev->ethtool_ops = &ne6xvf_ethtool_ops;
}
