// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#include "ne6x.h"
#include "ne6x_portmap.h"
#include "ne6x_reg.h"
#include "ne6x_dev.h"
#include <linux/firmware.h>
#include "version.h"

static const char ne6x_gstrings_test[][ETH_GSTRING_LEN] = {
	"Link test ", "Loopback test ", "Register test ", "Interrupt test"
};

#define NE6X_TEST_LEN (sizeof(ne6x_gstrings_test) / ETH_GSTRING_LEN)

static int ne6x_q_stats_len(struct net_device *netdev)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	int stats_size, total_slen = 0;

	/* Tx stats */
	stats_size = sizeof(struct ne6x_q_stats) + sizeof(struct ne6x_txq_stats);
	total_slen += adpt->num_queue * (stats_size / sizeof(u64));

	/* Rx stats */
	stats_size = sizeof(struct ne6x_q_stats) + sizeof(struct ne6x_rxq_stats);
	total_slen += adpt->num_queue * (stats_size / sizeof(u64));

	/* CQ stats */
	stats_size = sizeof(struct ne6x_cq_stats);
	total_slen += adpt->num_queue * (stats_size / sizeof(u64));

	return total_slen;
}

struct ne6x_stats {
	char stat_string[ETH_GSTRING_LEN];
	int  sizeof_stat;
	int  stat_offset;
};

/* Helper macro for defining some statistics directly copied from the netdev
 * stats structure.
 */
#define NE6X_NETDEV_STAT(_net_stat) NE6X_STAT(struct rtnl_link_stats64, #_net_stat, _net_stat)

static const struct ne6x_stats ne6x_gstrings_adpt_stats[] = {
	NE6X_NETDEV_STAT(rx_packets),
	NE6X_NETDEV_STAT(tx_packets),
	NE6X_NETDEV_STAT(rx_bytes),
	NE6X_NETDEV_STAT(tx_bytes),
	NE6X_NETDEV_STAT(rx_errors),
	NE6X_NETDEV_STAT(tx_errors),
	NE6X_NETDEV_STAT(rx_dropped),
	NE6X_NETDEV_STAT(tx_dropped),
	NE6X_NETDEV_STAT(collisions),
	NE6X_NETDEV_STAT(rx_length_errors),
	NE6X_NETDEV_STAT(rx_crc_errors),
};

#define NE6X_DEVICE_ETH_STAT(_dev_eth_stat) NE6X_STAT(struct  ne6x_eth_stats, \
						    #_dev_eth_stat, _dev_eth_stat)

static const struct ne6x_stats ne6x_gstrings_adpt_dev_eth_stats[] = {
	NE6X_DEVICE_ETH_STAT(rx_unicast),
	NE6X_DEVICE_ETH_STAT(rx_multicast),
	NE6X_DEVICE_ETH_STAT(rx_broadcast),
	NE6X_DEVICE_ETH_STAT(rx_discards),
	NE6X_DEVICE_ETH_STAT(rx_miss),
	NE6X_DEVICE_ETH_STAT(tx_unicast),
	NE6X_DEVICE_ETH_STAT(tx_multicast),
	NE6X_DEVICE_ETH_STAT(tx_broadcast),
	NE6X_DEVICE_ETH_STAT(rx_malform),
	NE6X_DEVICE_ETH_STAT(tx_malform),
};

#define NE6X_PF_STAT(_name, _stat) NE6X_STAT(struct ne6x_pf, _name, _stat)

static const struct ne6x_stats ne6x_gstrings_pf_stats[] = {
	NE6X_PF_STAT("tx_timeout", tx_timeout_count),
};

/* per-queue ring statistics */
#define NE6X_QUEUE_STAT(_name, _stat) NE6X_STAT(struct ne6x_ring, _name, _stat)

static const struct ne6x_stats ne6x_gstrings_tx_queue_stats[] = {
	NE6X_QUEUE_STAT("tx_queue_%u_packets",       stats.packets),
	NE6X_QUEUE_STAT("tx_queue_%u_bytes",         stats.bytes),
	NE6X_QUEUE_STAT("tx_queue_%u_rst",           tx_stats.restart_q),
	NE6X_QUEUE_STAT("tx_queue_%u_busy",          tx_stats.tx_busy),
	NE6X_QUEUE_STAT("tx_queue_%u_line",          tx_stats.tx_linearize),
	NE6X_QUEUE_STAT("tx_queue_%u_csum_err",      tx_stats.csum_err),
	NE6X_QUEUE_STAT("tx_queue_%u_csum",          tx_stats.csum_good),
	NE6X_QUEUE_STAT("tx_queue_%u_pcie_read_err", tx_stats.tx_pcie_read_err),
	NE6X_QUEUE_STAT("tx_queue_%u_ecc_err",       tx_stats.tx_ecc_err),
	NE6X_QUEUE_STAT("tx_queue_%u_drop_addr",     tx_stats.tx_drop_addr),
};

static const struct ne6x_stats ne6x_gstrings_rx_queue_stats[] = {
	NE6X_QUEUE_STAT("rx_queue_%u_packets",       stats.packets),
	NE6X_QUEUE_STAT("rx_queue_%u_bytes",         stats.bytes),
	NE6X_QUEUE_STAT("rx_queue_%u_no_eop",        rx_stats.non_eop_descs),
	NE6X_QUEUE_STAT("rx_queue_%u_alloc_pg_err",  rx_stats.alloc_page_failed),
	NE6X_QUEUE_STAT("rx_queue_%u_alloc_buf_err", rx_stats.alloc_buf_failed),
	NE6X_QUEUE_STAT("rx_queue_%u_pg_reuse",      rx_stats.page_reuse_count),
	NE6X_QUEUE_STAT("rx_queue_%u_csum_err",      rx_stats.csum_err),
	NE6X_QUEUE_STAT("rx_queue_%u_csum",          rx_stats.csum_good),
	NE6X_QUEUE_STAT("rx_queue_%u_mem_err",       rx_stats.rx_mem_error),
	NE6X_QUEUE_STAT("rx_queue_%u_rx_err",        rx_stats.rx_err),
};

static const struct ne6x_stats ne6x_gstrings_cq_queue_stats[] = {
	NE6X_QUEUE_STAT("cx_queue_%u_nums",    cq_stats.cq_num),
	NE6X_QUEUE_STAT("cx_queue_%u_tx_nums", cq_stats.tx_num),
	NE6X_QUEUE_STAT("cx_queue_%u_rx_nums", cq_stats.rx_num),
};

/* port mac statistics */
#define NE6X_PORT_MAC_STAT(_name, _stat) NE6X_STAT(struct ne6x_adapter, _name, _stat)

static const struct ne6x_stats ne6x_gstrings_port_mac_stats[] = {
	NE6X_PORT_MAC_STAT("port.rx_eth_byte",        stats.mac_rx_eth_byte),
	NE6X_PORT_MAC_STAT("port.rx_eth",             stats.mac_rx_eth),
	NE6X_PORT_MAC_STAT("port.rx_eth_undersize",   stats.mac_rx_eth_undersize),
	NE6X_PORT_MAC_STAT("port.rx_eth_crc_err",     stats.mac_rx_eth_crc),
	NE6X_PORT_MAC_STAT("port.rx_eth_64b",         stats.mac_rx_eth_64b),
	NE6X_PORT_MAC_STAT("port.rx_eth_65_127b",     stats.mac_rx_eth_65_127b),
	NE6X_PORT_MAC_STAT("port.rx_eth_128_255b",    stats.mac_rx_eth_128_255b),
	NE6X_PORT_MAC_STAT("port.rx_eth_256_511b",    stats.mac_rx_eth_256_511b),
	NE6X_PORT_MAC_STAT("port.rx_eth_512_1023b",   stats.mac_rx_eth_512_1023b),
	NE6X_PORT_MAC_STAT("port.rx_eth_1024_15360b", stats.mac_rx_eth_1024_15360b),
	NE6X_PORT_MAC_STAT("port.tx_eth_byte",        stats.mac_tx_eth_byte),
	NE6X_PORT_MAC_STAT("port.tx_eth",             stats.mac_tx_eth),
	NE6X_PORT_MAC_STAT("port.tx_eth_undersize",   stats.mac_tx_eth_undersize),
	NE6X_PORT_MAC_STAT("port.tx_eth_64b",         stats.mac_tx_eth_64b),
	NE6X_PORT_MAC_STAT("port.tx_eth_65_127b",     stats.mac_tx_eth_65_127b),
	NE6X_PORT_MAC_STAT("port.tx_eth_128_255b",    stats.mac_tx_eth_128_255b),
	NE6X_PORT_MAC_STAT("port.tx_eth_256_511b",    stats.mac_tx_eth_256_511b),
	NE6X_PORT_MAC_STAT("port.tx_eth_512_1023b",   stats.mac_tx_eth_512_1023b),
	NE6X_PORT_MAC_STAT("port.tx_eth_1024_15360b", stats.mac_tx_eth_1024_15360b),
};

#define NE6X_ADPT_STATS_LEN              ARRAY_SIZE(ne6x_gstrings_adpt_stats)
#define NE6X_ADPT_DEV_ETH_STATS_LEN      ARRAY_SIZE(ne6x_gstrings_adpt_dev_eth_stats)

#define NE6X_PF_STATS_LEN                ARRAY_SIZE(ne6x_gstrings_pf_stats)
#define NE6X_PORT_MAC_STATS_LEN          ARRAY_SIZE(ne6x_gstrings_port_mac_stats)

#define NE6X_ALL_STATS_LEN(n) \
	(NE6X_ADPT_STATS_LEN + NE6X_ADPT_DEV_ETH_STATS_LEN + \
	 NE6X_PF_STATS_LEN + NE6X_PORT_MAC_STATS_LEN + ne6x_q_stats_len(n))

struct ne6x_priv_flag {
	char name[ETH_GSTRING_LEN];
	u32 bitno;			/* bit position in pf->flags */
};

#define NE6X_PRIV_FLAG(_name, _bitno) { \
	.name = _name, \
	.bitno = _bitno, \
}

static const struct ne6x_priv_flag ne6x_gstrings_priv_flags[] = {
	NE6X_PRIV_FLAG("disable-fw-lldp", NE6X_ADPT_F_DISABLE_FW_LLDP),
	NE6X_PRIV_FLAG("link-down-on-close", NE6X_ADPT_F_LINKDOWN_ON_CLOSE),
	NE6X_PRIV_FLAG("write-protect", NE6X_ADPT_F_NORFLASH_WRITE_PROTECT),
	NE6X_PRIV_FLAG("ddos-switch", NE6X_ADPT_F_DDOS_SWITCH),
	NE6X_PRIV_FLAG("white-list", NE6X_ADPT_F_ACL),
	NE6X_PRIV_FLAG("trust-vlan", NE6X_ADPT_F_TRUST_VLAN),
};

#define NE6X_PRIV_FLAG_ARRAY_SIZE	ARRAY_SIZE(ne6x_gstrings_priv_flags)

static void ne6x_get_settings_link_up_fec(struct net_device *netdev,
					  u32 link_speed,
					  struct ethtool_link_ksettings *ks)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	enum ne6x_fec_state fec = NE6X_FEC_NONE;

	switch (link_speed) {
	case NE6X_LINK_SPEED_25GB:
	case NE6X_LINK_SPEED_100GB:
		ethtool_link_ksettings_add_link_mode(ks, supported, FEC_NONE);
		ethtool_link_ksettings_add_link_mode(ks, supported, FEC_RS);
		ethtool_link_ksettings_add_link_mode(ks, supported, FEC_BASER);

		ne6x_dev_get_fec(adpt, &fec);
		if (fec == NE6X_FEC_RS)
			ethtool_link_ksettings_add_link_mode(ks, advertising, FEC_RS);
		else if (fec == NE6X_FEC_BASER)
			ethtool_link_ksettings_add_link_mode(ks, advertising, FEC_BASER);
		else
			ethtool_link_ksettings_add_link_mode(ks, advertising, FEC_NONE);

		break;
	default:
		ethtool_link_ksettings_add_link_mode(ks, supported, FEC_NONE);
		ethtool_link_ksettings_add_link_mode(ks, advertising, FEC_NONE);
		break;
	}
}

static void ne6x_get_settings_link_up(struct ethtool_link_ksettings *ks, struct net_device *netdev)
{
	struct ne6x_link_status *link_info;
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);

	ethtool_link_ksettings_zero_link_mode(ks, supported);
	ethtool_link_ksettings_zero_link_mode(ks, advertising);

	link_info = &adpt->port_info->phy.link_info;
	switch (link_info->link_speed) {
	case NE6X_LINK_SPEED_100GB:
		ks->base.speed = SPEED_100000;
		ethtool_link_ksettings_add_link_mode(ks, advertising, 100000baseCR4_Full);
		break;
	case NE6X_LINK_SPEED_40GB:
		ks->base.speed = SPEED_40000;
		ethtool_link_ksettings_add_link_mode(ks, advertising, 40000baseCR4_Full);
		break;
	case NE6X_LINK_SPEED_25GB:
		ks->base.speed = SPEED_25000;
		ethtool_link_ksettings_add_link_mode(ks, advertising, 25000baseCR_Full);
		break;
	case NE6X_LINK_SPEED_10GB:
		ks->base.speed = SPEED_10000;
		ethtool_link_ksettings_add_link_mode(ks, advertising, 10000baseT_Full);
		break;
	case NE6X_LINK_SPEED_200GB:
		ks->base.speed = SPEED_200000;
		break;
	default:
		netdev_info(netdev, "WARNING: Unrecognized link_speed (0x%x).\n",
			    link_info->link_speed);
		break;
	}

	ks->base.duplex = DUPLEX_FULL;

	if (link_info->an_info & NE6X_AQ_AN_COMPLETED)
		ethtool_link_ksettings_add_link_mode(ks, lp_advertising, Autoneg);

	ethtool_link_ksettings_add_link_mode(ks, advertising, Pause);

	ne6x_get_settings_link_up_fec(netdev, link_info->link_speed, ks);
}

static void ne6x_phy_type_to_ethtool(struct ne6x_adapter *adpt,
				     struct ethtool_link_ksettings *ks)
{
	ethtool_link_ksettings_zero_link_mode(ks, supported);
	ethtool_link_ksettings_zero_link_mode(ks, advertising);
}

static void ne6x_get_settings_link_down(struct ethtool_link_ksettings *ks,
					struct net_device *netdev)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);

	ne6x_phy_type_to_ethtool(adpt, ks);
	/* With no link, speed and duplex are unknown */
	ks->base.speed = SPEED_UNKNOWN;
	ks->base.duplex = DUPLEX_UNKNOWN;
}

static int ne6x_get_link_ksettings(struct net_device *netdev,
				   struct ethtool_link_ksettings *ks)
{
	struct ne6x_link_status *hw_link_info;
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);

	ethtool_link_ksettings_zero_link_mode(ks, supported);
	ethtool_link_ksettings_zero_link_mode(ks, advertising);
	ethtool_link_ksettings_zero_link_mode(ks, lp_advertising);
	hw_link_info = &adpt->port_info->phy.link_info;

	/* set speed and duplex */
	if (hw_link_info->link_info & NE6X_AQ_LINK_UP)
		ne6x_get_settings_link_up(ks, netdev);
	else
		ne6x_get_settings_link_down(ks, netdev);

	if (!ne6x_dev_check_speed(adpt, SPEED_10000))
		ethtool_link_ksettings_add_link_mode(ks, supported, 10000baseT_Full);

	if (!ne6x_dev_check_speed(adpt, SPEED_25000))
		ethtool_link_ksettings_add_link_mode(ks, supported, 25000baseCR_Full);

	if (!ne6x_dev_check_speed(adpt, SPEED_100000))
		ethtool_link_ksettings_add_link_mode(ks, supported, 100000baseCR4_Full);

	if (!ne6x_dev_check_speed(adpt, SPEED_40000))
		ethtool_link_ksettings_add_link_mode(ks, supported, 40000baseCR4_Full);

	ethtool_link_ksettings_add_link_mode(ks, supported, FIBRE);
	ethtool_link_ksettings_add_link_mode(ks, advertising, FIBRE);
	ks->base.port = PORT_FIBRE;

	/* Set flow control settings */
	ethtool_link_ksettings_add_link_mode(ks, supported, Pause);

	return 0;
}

static int ne6x_set_link_ksettings(struct net_device *netdev,
				   const struct ethtool_link_ksettings *ks)
{
	bool if_running = netif_running(netdev);
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	struct ne6x_pf *pf = adpt->back;
	u32 master = (adpt->idx == 0);
	char *speed = "Unknown ";
	u32 link_speed;
	u32 sfp_speed;
	int ret;

	if (ne6x_dev_check_speed(adpt, ks->base.speed)) {
		dev_info(&pf->pdev->dev, "speed not support\n");
		return -EOPNOTSUPP;
	}

	if (!master && pf->dev_type == NE6000AI_2S_X16H_25G_N5) {
		dev_info(&pf->pdev->dev, "only master port can change speed\n");
		return -EOPNOTSUPP;
	}

	switch (ks->base.speed) {
	case SPEED_100000:
		link_speed = NE6X_LINK_SPEED_100GB;
		break;
	case SPEED_40000:
		link_speed = NE6X_LINK_SPEED_40GB;
		break;
	case SPEED_25000:
		link_speed = NE6X_LINK_SPEED_25GB;
		break;
	case SPEED_10000:
		link_speed = NE6X_LINK_SPEED_10GB;
		break;
	default:
		return -EOPNOTSUPP;
	}

	ret = ne6x_dev_get_sfp_speed(adpt, &sfp_speed);
	if (!ret) {
		switch (sfp_speed) {
		case NE6X_LINK_SPEED_40GB:
			speed = "40 G";
			break;
		case NE6X_LINK_SPEED_100GB:
			speed = "100 G";
			break;
		case NE6X_LINK_SPEED_10GB:
			speed = "10 G";
			break;
		case NE6X_LINK_SPEED_25GB:
			speed = "25 G";
			break;
		case NE6X_LINK_SPEED_200GB:
			speed = "200 G";
			break;
		default:
			break;
		}

		if (sfp_speed != link_speed)
			netdev_info(adpt->netdev, "speed not match, sfp support%sbps Full Duplex\n",
				    speed);
	}

	if (if_running)
		ne6x_close(adpt->netdev);

	ret = ne6x_dev_set_speed(adpt, link_speed);
	if (if_running)
		ne6x_open(adpt->netdev);

	return ret;
}

static void __ne6x_add_stat_strings(u8 **p, const struct ne6x_stats stats[],
				    const unsigned int size,
				    ...)
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

#define ne6x_add_stat_strings(p, stats, ...) \
	__ne6x_add_stat_strings(p, stats, ARRAY_SIZE(stats), ##__VA_ARGS__)

static void ne6x_get_stat_strings(struct net_device *netdev, u8 *data)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	unsigned int i;

	ne6x_add_stat_strings(&data, ne6x_gstrings_adpt_stats);
	ne6x_add_stat_strings(&data, ne6x_gstrings_adpt_dev_eth_stats);
	ne6x_add_stat_strings(&data, ne6x_gstrings_pf_stats);

	for (i = 0; i < adpt->num_queue; i++) {
		ne6x_add_stat_strings(&data, ne6x_gstrings_tx_queue_stats, i);
		ne6x_add_stat_strings(&data, ne6x_gstrings_rx_queue_stats, i);
		ne6x_add_stat_strings(&data, ne6x_gstrings_cq_queue_stats, i);
	}

	ne6x_add_stat_strings(&data, ne6x_gstrings_port_mac_stats);
}

static void ne6x_get_priv_flag_strings(struct net_device *netdev, u8 *data)
{
	unsigned int i;
	u8 *p = data;

	for (i = 0; i < NE6X_PRIV_FLAG_ARRAY_SIZE; i++) {
		snprintf(p, ETH_GSTRING_LEN, "%s", ne6x_gstrings_priv_flags[i].name);
		p += ETH_GSTRING_LEN;
	}
}

static void ne6x_get_strings(struct net_device *netdev, u32 stringset, u8 *data)
{
	switch (stringset) {
	case ETH_SS_STATS:
		ne6x_get_stat_strings(netdev, data);
		break;
	case ETH_SS_TEST:
		memcpy(data, ne6x_gstrings_test, NE6X_TEST_LEN * ETH_GSTRING_LEN);
		break;
	case ETH_SS_PRIV_FLAGS:
		ne6x_get_priv_flag_strings(netdev, data);
		break;
	default:
		break;
	}
}

static int ne6x_get_sset_count(struct net_device *netdev, int sset)
{
	switch (sset) {
	case ETH_SS_STATS:
		return NE6X_ALL_STATS_LEN(netdev);
	case ETH_SS_TEST:
		return NE6X_TEST_LEN;
	case ETH_SS_PRIV_FLAGS:
		return NE6X_PRIV_FLAG_ARRAY_SIZE;
	default:
		return -EOPNOTSUPP;
	}
}

static void ne6x_get_mac_stats(struct ne6x_adapter *adpt)
{
	ne6x_dev_get_mac_stats(adpt);
}

static void ne6x_get_ethtool_stats(struct net_device *netdev,
				   struct ethtool_stats __always_unused *stats,
				   u64 *data)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	struct ne6x_pf *pf = adpt->back;
	struct ne6x_ring *tx_ring;
	struct ne6x_ring *rx_ring;
	struct ne6x_ring *cq_ring;
	unsigned int j;
	int i = 0;
	char *p;

	ne6x_update_pf_stats(adpt);

	for (j = 0; j < NE6X_ADPT_STATS_LEN; j++) {
		p = (char *)ne6x_get_adpt_stats_struct(adpt) +
				ne6x_gstrings_adpt_stats[j].stat_offset;
		data[i++] = (ne6x_gstrings_adpt_stats[j].sizeof_stat == sizeof(u64)) ?
			    *(u64 *)p : *(u32 *)p;
	}

	for (j = 0; j < NE6X_ADPT_DEV_ETH_STATS_LEN; j++) {
		p = (char *)(&adpt->eth_stats) +
			ne6x_gstrings_adpt_dev_eth_stats[j].stat_offset;
		data[i++] = (ne6x_gstrings_adpt_dev_eth_stats[j].sizeof_stat ==
				sizeof(u64)) ? *(u64 *)p : *(u32 *)p;
	}

	for (j = 0; j < NE6X_PF_STATS_LEN; j++) {
		p = (char *)pf + ne6x_gstrings_pf_stats[j].stat_offset;
		data[i++] = (ne6x_gstrings_pf_stats[j].sizeof_stat == sizeof(u64)) ?
			    *(u64 *)p : *(u32 *)p;
	}

	/* populate per queue stats */
	rcu_read_lock();
	for (j = 0; j < adpt->num_queue; j++) {
		tx_ring = READ_ONCE(adpt->tx_rings[j]);
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

		rx_ring = READ_ONCE(adpt->rx_rings[j]);
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

		cq_ring = READ_ONCE(adpt->cq_rings[j]);
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

	ne6x_get_mac_stats(adpt);

	for (j = 0; j < NE6X_PORT_MAC_STATS_LEN; j++) {
		p = (char *)adpt + ne6x_gstrings_port_mac_stats[j].stat_offset;
		data[i++] = (ne6x_gstrings_port_mac_stats[j].sizeof_stat == sizeof(u64)) ?
			    *(u64 *)p : *(u32 *)p;
	}
}

extern char ne6x_driver_name[];

static void ne6x_get_drvinfo(struct net_device *netdev, struct ethtool_drvinfo *drvinfo)
{
	u32 soc_ver = 0, np_ver = 0, erom_ver = 0;
	struct ne6x_pf *pf = ne6x_netdev_to_pf(netdev);
	char nvm_version_str[32];
	char driver_name[32];
	char temp_str[16] = {0};

	snprintf(driver_name, 32, "%s", ne6x_driver_name);
	strscpy(drvinfo->driver, driver_name, sizeof(drvinfo->driver));
	strscpy(drvinfo->version, VERSION, sizeof(drvinfo->version));
	memset(nvm_version_str, 0, sizeof(nvm_version_str));
	soc_ver = pf->verinfo.firmware_soc_ver;
	np_ver = pf->verinfo.firmware_np_ver & 0xFFFF;
	erom_ver = pf->verinfo.firmware_pxe_ver & 0xFFFF;
	snprintf(nvm_version_str, 20, "%d.%d.%d.%d ", (soc_ver & 0xff000000) >> 24,
		 ((erom_ver & 0xFFFF) / 100), ((soc_ver & 0xFFFF) / 100),
		 ((np_ver & 0xFFFF) / 100));
	if (erom_ver % 100) {
		snprintf(temp_str, 4, "P%d", (erom_ver % 100));
		strncat(nvm_version_str, temp_str, 4);
	}
	if ((soc_ver & 0xffff) % 100) {
		snprintf(temp_str, 4, "A%d", ((soc_ver & 0xffff) % 100));
		strncat(nvm_version_str, temp_str, 4);
	}
	if (np_ver % 100) {
		snprintf(temp_str, 4, "N%d", (np_ver % 100));
		strncat(nvm_version_str, temp_str, 4);
	}
	strlcpy(drvinfo->fw_version, nvm_version_str, sizeof(drvinfo->fw_version));
	strlcpy(drvinfo->bus_info, pci_name(pf->pdev), sizeof(drvinfo->bus_info));
}

static void ne6x_get_regs(struct net_device *netdev, struct ethtool_regs *regs, void *p)
{
	struct ne6x_pf *pf = ne6x_netdev_to_pf(netdev);
	struct ne6x_hw *hw = &pf->hw;
	unsigned int i, j, ri;
	u32 *reg_buf = p;
	u32 reg;

	regs->version = 1;

	/* loop through the diags reg table for what to print */
	ri = 0;
	for (i = 0; ne6x_reg_list[i].offset != 0; i++) {
		for (j = 0; j < ne6x_reg_list[i].elements; j++) {
			reg = ne6x_reg_list[i].offset + (j * ne6x_reg_list[i].stride);
			reg_buf[ri++] = rd64(hw, reg);
		}
	}
}

static void ne6x_self_test(struct net_device *dev, struct ethtool_test *eth_test, u64 *data)
{
	memset(data, 0, sizeof(*data) * NE6X_TEST_LEN);
}

static int ne6x_get_regs_len(struct net_device *netdev)
{
	int reg_count = 0;
	int i;

	for (i = 0; ne6x_reg_list[i].offset != 0; i++)
		reg_count += ne6x_reg_list[i].elements;

	return reg_count * sizeof(u32);
}

static void ne6x_get_ringparam(struct net_device *netdev,
			       struct ethtool_ringparam *ring,
			       struct kernel_ethtool_ringparam __always_unused *ker,
			       struct netlink_ext_ack __always_unused *extack)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);

	ring->rx_max_pending = NE6X_MAX_NUM_DESCRIPTORS;
	ring->tx_max_pending = NE6X_MAX_NUM_DESCRIPTORS;
	ring->rx_mini_max_pending = NE6X_MIN_NUM_DESCRIPTORS;
	ring->rx_jumbo_max_pending = 0;
	ring->rx_pending = adpt->num_rx_desc;
	ring->tx_pending = adpt->num_tx_desc;
	ring->rx_mini_pending = NE6X_MIN_NUM_DESCRIPTORS;
	ring->rx_jumbo_pending = 0;
}

static int ne6x_set_ringparam(struct net_device *netdev,
			      struct ethtool_ringparam *ring,
			      struct kernel_ethtool_ringparam __always_unused *ker,
			      struct netlink_ext_ack __always_unused *extack)
{
	u32 new_rx_count, new_tx_count, new_cq_count, new_tg_count;
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	struct ne6x_pf *pf = adpt->back;
	int timeout = 50;
	int err = 0;
	int i;

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
	new_cq_count = new_tx_count + new_rx_count;
	new_tg_count = new_tx_count;

	if (new_tx_count == adpt->num_tx_desc && new_rx_count == adpt->num_rx_desc)
		return 0;

	while (test_and_set_bit(NE6X_CONFIG_BUSY, pf->state)) {
		timeout--;
		if (!timeout)
			return -EBUSY;

		usleep_range(1000, 2000);
	}

	if (!netif_running(adpt->netdev)) {
		adpt->num_tx_desc = new_tx_count;
		adpt->num_rx_desc = new_rx_count;
		adpt->num_cq_desc = new_cq_count;
		adpt->num_tg_desc = new_tg_count;
		netdev_info(netdev, "Link is down, queue count change happens when link is brought up\n");
		goto done;
	}

	err = ne6x_close(adpt->netdev);
	if (err) {
		netdev_err(netdev, "fail to close adpt = %d\n", adpt->idx);
		goto done;
	}

	netdev_info(netdev, "Descriptors change  from (Tx: %d / Rx: %d) to [%d-%d]\n",
		    adpt->tx_rings[0]->count, adpt->rx_rings[0]->count, new_tx_count, new_rx_count);

	/* simple case - set for the next time the netdev is started */
	for (i = 0; i < adpt->num_queue; i++) {
		adpt->tx_rings[i]->count = new_tx_count;
		adpt->rx_rings[i]->count = new_rx_count;
		adpt->cq_rings[i]->count = new_cq_count;
		adpt->tg_rings[i]->count = new_tg_count;
	}

	adpt->num_tx_desc = new_tx_count;
	adpt->num_rx_desc = new_rx_count;
	adpt->num_cq_desc = new_cq_count;
	adpt->num_tg_desc = new_tg_count;

	err = ne6x_open(adpt->netdev);
	if (err) {
		netdev_err(netdev, "fail to open adpt = %d\n", adpt->idx);
		goto done;
	}

done:
	clear_bit(NE6X_CONFIG_BUSY, pf->state);

	return err;
}

static void ne6x_get_pauseparam(struct net_device *netdev, struct ethtool_pauseparam *pause)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	struct ne6x_flowctrl flowctrl;
	int ret;

	ret = ne6x_dev_get_flowctrl(adpt, &flowctrl);
	if (ret)
		return;

	pause->autoneg = 0;
	pause->rx_pause = flowctrl.rx_pause;
	pause->tx_pause = flowctrl.tx_pause;
}

static int ne6x_set_pauseparam(struct net_device *netdev,
			       struct ethtool_pauseparam *pause)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	struct ne6x_flowctrl flowctrl;
	int ret;

	if (pause->autoneg)
		return -EOPNOTSUPP;

	flowctrl.autoneg = pause->autoneg;
	flowctrl.rx_pause = pause->rx_pause;
	flowctrl.tx_pause = pause->tx_pause;

	ret = ne6x_dev_set_flowctrl(adpt, &flowctrl);
	if (ret)
		return ret;

	return 0;
}

static int ne6x_get_coalesce(struct net_device *netdev,
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

static int ne6x_get_eeprom_len(struct net_device *netdev) { return 256; }

static int ne6x_get_eeprom(struct net_device *netdev,
			   struct ethtool_eeprom *eeprom, u8 *bytes)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	struct ne6x_pf *pf = adpt->back;
	struct ne6x_hw *hw = &pf->hw;
	u8 *eeprom_buff;
	int err = 0;
	int ret_val;
	u32 magic;

	if (eeprom->len == 0)
		return -EINVAL;

	magic = hw->vendor_id | (hw->device_id << 16);
	if (eeprom->magic && eeprom->magic != magic) {
		/* make sure it is the right magic for NVMUpdate */
		if ((eeprom->magic >> 16) != hw->device_id)
			err = -EINVAL;
		else if (test_bit(NE6X_RESET_INTR_RECEIVED, pf->state))
			err = -EBUSY;

		return err;
	}

	/* normal ethtool get_eeprom support */
	eeprom->magic = hw->vendor_id | (hw->device_id << 16);

	eeprom_buff = kzalloc(eeprom->len, GFP_KERNEL);
	if (!eeprom_buff)
		return -ENOMEM;

	ret_val = ne6x_dev_read_eeprom(adpt, 0x0, (u8 *)eeprom_buff, eeprom->len);
	memcpy(bytes, eeprom_buff, eeprom->len);
	kfree(eeprom_buff);

	return ret_val;
}

#define L3_RSS_FLAGS				(RXH_IP_DST | RXH_IP_SRC)
#define L4_RSS_FLAGS				(RXH_L4_B_0_1 | RXH_L4_B_2_3)

static u64 ne6x_get_rss_hash_opts(struct ne6x_adapter *adpt, u64 flow_type)
{
	u64 data = 0;

	switch (flow_type) {
	case TCP_V4_FLOW:
		if (adpt->rss_info.hash_type & NE6X_RSS_HASH_TYPE_IPV4)
			data |= RXH_IP_DST | RXH_IP_SRC;
		if (adpt->rss_info.hash_type & NE6X_RSS_HASH_TYPE_IPV4_TCP)
			data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		break;
	case UDP_V4_FLOW:
		if (adpt->rss_info.hash_type & NE6X_RSS_HASH_TYPE_IPV4)
			data |= RXH_IP_DST | RXH_IP_SRC;
		if (adpt->rss_info.hash_type & NE6X_RSS_HASH_TYPE_IPV4_UDP)
			data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		break;
	case TCP_V6_FLOW:
		if (adpt->rss_info.hash_type & NE6X_RSS_HASH_TYPE_IPV6)
			data |= RXH_IP_DST | RXH_IP_SRC;
		if (adpt->rss_info.hash_type & NE6X_RSS_HASH_TYPE_IPV6_TCP)
			data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		break;
	case UDP_V6_FLOW:
		if (adpt->rss_info.hash_type & NE6X_RSS_HASH_TYPE_IPV6)
			data |= RXH_IP_DST | RXH_IP_SRC;
		if (adpt->rss_info.hash_type & NE6X_RSS_HASH_TYPE_IPV6_UDP)
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

static int ne6x_set_rss_hash_opts(struct ne6x_adapter *adpt, struct ethtool_rxnfc *cmd)
{
	u16 rss_flags = adpt->rss_info.hash_type;
	int status;

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

	if (rss_flags == adpt->rss_info.hash_type)
		return 0;

	adpt->rss_info.hash_type = rss_flags;

	status = ne6x_dev_set_rss(adpt, &adpt->rss_info);

	return (status != 0) ? (-EIO) : 0;
}

static int ne6x_get_rxnfc(struct net_device *netdev, struct ethtool_rxnfc *info, u32 *rules)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);

	switch (info->cmd) {
	case ETHTOOL_GRXFH:
		info->data = ne6x_get_rss_hash_opts(adpt, info->flow_type);
		break;
	case ETHTOOL_GRXRINGS:
		info->data = adpt->num_queue;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ne6x_set_rxnfc(struct net_device *netdev, struct ethtool_rxnfc *info)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	int status = 0;

	switch (info->cmd) {
	case ETHTOOL_GRXRINGS:
		info->data = adpt->num_queue;
		break;
	case ETHTOOL_SRXFH:
		status = ne6x_set_rss_hash_opts(adpt, info);
		break;
	default:
		return -EINVAL;
	}

	return status;
}

static u32 ne6x_get_rxfh_key_size(struct net_device *netdev)
{
	return NE6X_RSS_MAX_KEY_SIZE;
}

static u32 ne6x_get_rss_table_size(struct net_device *netdev)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	struct ne6x_rss_info *rss_info = &adpt->rss_info;

	return rss_info->ind_table_size;
}

static int ne6x_get_rxfh(struct net_device *netdev, u32 *p, u8 *key, u8 *hfunc)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	struct ne6x_rss_info *rss_info = &adpt->rss_info;
	unsigned int n = rss_info->ind_table_size;

	if (hfunc)
		*hfunc = ETH_RSS_HASH_TOP;

	if (p) {
		while (n--)
			p[n] = rss_info->ind_table[n];
	}

	if (key)
		memcpy(key, rss_info->hash_key, ne6x_get_rxfh_key_size(netdev));

	return 0;
}

static int ne6x_set_rxfh(struct net_device *netdev, const u32 *p, const u8 *key, const u8 hfunc)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	struct ne6x_rss_info *rss_info = &adpt->rss_info;
	unsigned int i;
	int status;

	/* We do not allow change in unsupported parameters */
	if (hfunc != ETH_RSS_HASH_NO_CHANGE && hfunc != ETH_RSS_HASH_TOP)
		return -EOPNOTSUPP;

	/* Fill out the redirection table */
	if (p) {
		/* Allow at least 2 queues w/ SR-IOV. */
		for (i = 0; i < rss_info->ind_table_size; i++)
			rss_info->ind_table[i] = p[i];
	}

	/* Fill out the rss hash key */
	if (key)
		memcpy(&rss_info->hash_key[0], key, ne6x_get_rxfh_key_size(netdev));

	status = ne6x_dev_set_rss(adpt, rss_info);

	return (status == 0) ? 0 : (-EIO);
}

static void ne6x_get_channels(struct net_device *netdev, struct ethtool_channels *channels)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);

	channels->max_rx = 0;
	channels->max_tx = 0;
	channels->max_other = 0;
	channels->max_combined = adpt->port_info->hw_max_queue;
	channels->rx_count = 0;
	channels->tx_count = 0;
	channels->other_count = 0;
	channels->combined_count = adpt->num_queue;
}

static int ne6x_set_channels(struct net_device *netdev,  struct ethtool_channels *channels)
{
	int qp_remaining, q_vectors, i;
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	struct ne6x_pf *pf = adpt->back;
	int timeout = 50;
	int err = 0;

	if (!channels->combined_count || channels->rx_count || channels->tx_count ||
	    channels->combined_count > pf->hw.expect_vp)
		return -EINVAL;

	if (channels->combined_count == adpt->num_queue) {
		/* nothing to do */
		netdev_info(netdev, "channel not change, nothing to do!\n");
		return 0;
	}

	while (test_and_set_bit(NE6X_CONFIG_BUSY, pf->state)) {
		timeout--;
		if (!timeout) {
			netdev_info(netdev, "ne6x config busy, timeout!!!\n");
			return -EBUSY;
		}
		usleep_range(1000, 2000);
	}

	/* set for the next time the netdev is started */
	if (!netif_running(adpt->netdev)) {
		adpt->port_info->queue = channels->combined_count;
		adpt->num_q_vectors = adpt->port_info->queue;
		adpt->num_queue = adpt->num_q_vectors;
		qp_remaining = adpt->num_queue;
		q_vectors = adpt->num_q_vectors;

		for (i = 0; i < adpt->num_q_vectors; i++) {
			adpt->q_vectors[i]->num_ringpairs =
				DIV_ROUND_UP(qp_remaining, q_vectors - i);
			adpt->q_vectors[i]->reg_idx =
				adpt->q_vectors[i]->v_idx + adpt->base_vector;
			qp_remaining--;
		}

		for (i = 0; i < adpt->rss_info.ind_table_size; i++)
			adpt->rss_info.ind_table[i] =
				ethtool_rxfh_indir_default(i, adpt->num_queue);

		ne6x_dev_set_rss(adpt, &adpt->rss_info);
		netdev_info(netdev, "Link is down, queue count change happens when link is brought up\n");
		goto done;
	}

	err = ne6x_close(adpt->netdev);
	if (err) {
		netdev_err(netdev, "fail to close adpt = %d\n", adpt->idx);
		goto done;
	}

	adpt->port_info->queue = channels->combined_count;
	adpt->num_q_vectors = adpt->port_info->queue;
	adpt->num_queue = adpt->num_q_vectors;
	qp_remaining = adpt->num_queue;
	q_vectors = adpt->num_q_vectors;

	for (i = 0; i < adpt->num_q_vectors; i++) {
		adpt->q_vectors[i]->num_ringpairs = DIV_ROUND_UP(qp_remaining, q_vectors - i);
		adpt->q_vectors[i]->reg_idx =  adpt->q_vectors[i]->v_idx + adpt->base_vector;
		qp_remaining--;
	}

	for (i = 0; i < adpt->rss_info.ind_table_size; i++)
		adpt->rss_info.ind_table[i] = ethtool_rxfh_indir_default(i, adpt->num_queue);

	ne6x_dev_set_rss(adpt, &adpt->rss_info);
	err = ne6x_open(adpt->netdev);
	if (err) {
		netdev_err(netdev, "fail to open adpt = %d\n", adpt->idx);
		goto done;
	}

done:
	clear_bit(NE6X_CONFIG_BUSY, pf->state);

	return err;
}

static int ne6x_set_phys_id(struct net_device *netdev, enum ethtool_phys_id_state state)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);

	switch (state) {
	case ETHTOOL_ID_ACTIVE:
		ne6x_dev_set_led(adpt, true);
		return 1;
	case ETHTOOL_ID_ON:
		return 0;
	case ETHTOOL_ID_OFF:
		return 0;
	case ETHTOOL_ID_INACTIVE:
		ne6x_dev_set_led(adpt, false);
	}

	return 0;
}

static int ne6x_nway_reset(struct net_device *netdev) { return 0; }

static u64 ne6x_link_test(struct net_device *netdev, u64 *data)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	bool link_up = false;
	int verify;

	verify = 0;
	link_up = adpt->port_info->phy.link_info.link_info & NE6X_AQ_LINK_UP;
	usleep_range(10, 20);

	link_up &= verify;
	if (link_up)
		*data = 1;
	else
		*data = 0;

	return *data;
}

static void ne6x_diag_test(struct net_device *netdev, struct ethtool_test *eth_test, u64 *data)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);

	/* Online tests */
	if (ne6x_link_test(netdev, &data[NE6X_ETH_TEST_LINK]))
		eth_test->flags |= ETH_TEST_FL_FAILED;

	data[NE6X_ETH_TEST_LOOPBACK] = 0;
	if (ne6x_dev_test_loopback(adpt)) {
		data[NE6X_ETH_TEST_LOOPBACK] = 1;
		eth_test->flags |= ETH_TEST_FL_FAILED;
	}

	data[NE6X_ETH_TEST_REG] = 0;
	if (ne6x_dev_test_reg(adpt)) {
		data[NE6X_ETH_TEST_REG] = 1;
		eth_test->flags |= ETH_TEST_FL_FAILED;
	}

	data[NE6X_ETH_TEST_INT] = 0;
	if (ne6x_dev_test_intr(adpt)) {
		data[NE6X_ETH_TEST_INT] = 1;
		eth_test->flags |= ETH_TEST_FL_FAILED;
	}
}

static int ne6x_get_fec_param(struct net_device *netdev, struct ethtool_fecparam *fecparam)
{
	struct ne6x_link_status *hw_link_info;
	enum ne6x_fec_state fec = NE6X_FEC_NONE;
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	int err = 0;

	hw_link_info = &adpt->port_info->phy.link_info;
	if (hw_link_info->link_info & NE6X_AQ_LINK_UP) {
		switch (hw_link_info->link_speed) {
		case NE6X_LINK_SPEED_25GB:
		case NE6X_LINK_SPEED_100GB:
			err = ne6x_dev_get_fec(adpt, &fec);
			if (fec == NE6X_FEC_RS) {
				fecparam->fec |= ETHTOOL_FEC_RS;
				fecparam->active_fec = ETHTOOL_FEC_RS;
			} else if (fec == NE6X_FEC_BASER) {
				fecparam->fec |= ETHTOOL_FEC_BASER;
				fecparam->active_fec = ETHTOOL_FEC_BASER;
			} else {
				fecparam->fec |= ETHTOOL_FEC_OFF;
				fecparam->active_fec = ETHTOOL_FEC_OFF;
			}
			break;
		default:
			fecparam->fec |= ETHTOOL_FEC_OFF;
			fecparam->active_fec = ETHTOOL_FEC_OFF;
			break;
		}
	} else {
		fecparam->fec |= ETHTOOL_FEC_OFF;
		fecparam->active_fec = ETHTOOL_FEC_OFF;
	}

	return err;
}

static int ne6x_set_fec_param(struct net_device *netdev, struct ethtool_fecparam *fecparam)
{
	enum ne6x_fec_state fec = NE6X_FEC_NONE;
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	struct ne6x_pf *pf = adpt->back;
	int err = 0;

	switch (fecparam->fec) {
	case ETHTOOL_FEC_AUTO:
		dev_warn(&pf->pdev->dev, "Unsupported FEC mode: AUTO");
		err = -EINVAL;
		goto done;
	case ETHTOOL_FEC_RS:
		fec = NE6X_FEC_RS;
		break;
	case ETHTOOL_FEC_BASER:
		fec = NE6X_FEC_BASER;
		break;
	case ETHTOOL_FEC_OFF:
	case ETHTOOL_FEC_NONE:
		fec = NE6X_FEC_NONE;
		break;
	default:
		dev_warn(&pf->pdev->dev, "Unsupported FEC mode: %d", fecparam->fec);
		err = -EINVAL;
		goto done;
	}

	err = ne6x_dev_set_fec(adpt, fec);
	if (err)
		return err;

done:
	return err;
}

static const char * const flash_region_strings[] = {
	"810 loader",
	"810 app",
	"807 app",
	"NP Image",
	"PXE Image",
};

static int ethtool_flash_firmware(struct net_device *netdev, u32 type, const u8 *data,
				  u32 size)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	struct ne6x_pf *pf = adpt->back;
	int ret;

	ret = ne6x_dev_upgrade_firmware(adpt, type, (u8 *)data, size, 1);
	if (ret)
		dev_err(&pf->pdev->dev, "Failed to flash firmware\n");

	return ret;
}

static int ethtool_flash_region(struct net_device *netdev, const u8 *data, u32 size, u32 region)
{
	struct ne6x_pf *pf = ne6x_netdev_to_pf(netdev);
	int ret;

	netdev_info(netdev, "%s = 0x%x\n", __func__, region);

	switch (region) {
	case NE6X_ETHTOOL_FLASH_810_APP:
	case NE6X_ETHTOOL_FLASH_NP:
	case NE6X_ETHTOOL_FLASH_PXE:
	case NE6X_ETHTOOL_FLASH_810_LOADER:
	case NE6X_ETHTOOL_FRU:
	case NE6X_ETHTOOL_FLASH_807_APP:
		ret = ethtool_flash_firmware(netdev, region, data, size);
		break;
	default:
		ret = -EOPNOTSUPP;
		break;
	}

	if (ret)
		dev_info(&pf->pdev->dev, "loading %s fail, reload driver\n",
			 flash_region_strings[region]);

	return ret;
}

static int ne6x_ethtool_get_flash_region(struct net_device *netdev, const u8 *data, u32 *size)
{
	int region = -1;
	int ret;

	ret = ne6x_dev_validate_fw(data, *size, &region);
	if (ret) {
		netdev_err(netdev, "firmware error ret = %d\n", ret);
		return -1;
	}

	return region;
}

static int ne6x_set_flash(struct net_device *netdev, struct ethtool_flash *ef)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	struct ne6x_pf *pf = adpt->back;
	const struct firmware *fw;
	unsigned int master;
	size_t fw_size;
	u8 *fw_data;
	int region;
	int ret;

	master = (adpt->idx == 0);
	if (!master) {
		dev_info(&pf->pdev->dev, "only master port can upgrade\n");
		return -1;
	}

	ret = request_firmware(&fw, ef->data, &pf->pdev->dev);
	if (ret < 0)
		return ret;

	fw_data = (u8 *)fw->data;
	fw_size = fw->size;
	if (fw_size > 0) {
		region = ne6x_ethtool_get_flash_region(netdev, fw_data, (u32 *)&fw_size);
		if (region < 0) {
			ret = region;
			goto out_free_fw;
		}

		ret = ethtool_flash_region(netdev, fw_data, fw_size, region);
		if (ret)
			goto out_free_fw;
	}

out_free_fw:
	release_firmware(fw);
	return ret;
}

#define NE6X_FIRMWARE_RESET_CHIP \
	((ETH_RESET_MGMT | ETH_RESET_IRQ |	\
	  ETH_RESET_DMA | ETH_RESET_FILTER |	\
	  ETH_RESET_OFFLOAD | ETH_RESET_MAC |	\
	  ETH_RESET_PHY | ETH_RESET_RAM) << ETH_RESET_SHARED_SHIFT)

static int ne6x_reset(struct net_device *netdev, u32 *flags)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	bool reload = false;
	u32 req = *flags;

	if (!req)
		return -EINVAL;

	if (adpt->idx != 0x0) {
		netdev_err(netdev, "Reset is not supported from a eth0_nfp1\n");
		return -EOPNOTSUPP;
	}

	if ((req & NE6X_FIRMWARE_RESET_CHIP) == NE6X_FIRMWARE_RESET_CHIP) {
		/* This feature is not supported in older firmware versions */
		if (!ne6x_dev_reset_firmware(adpt)) {
			netdev_info(netdev, "Firmware reset request successful.\n");
			reload = true;
			*flags &= ~NE6X_FIRMWARE_RESET_CHIP;
		}
	}

	if (reload)
		netdev_info(netdev, "Reload driver to complete reset\n");

	return 0;
}

static int ne6x_get_module_info(struct net_device *netdev, struct ethtool_modinfo *modinfo)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	struct ne6x_sfp_mod_type_len sfp_mod;
	int err;

	err = ne6x_dev_get_sfp_type_len(adpt, &sfp_mod);
	if (err)
		return err;

	modinfo->type = sfp_mod.type;
	modinfo->eeprom_len = sfp_mod.len;
	netdev_info(netdev, "type %d erprom_len %d.\n", sfp_mod.type, sfp_mod.len);

	return 0;
}

#define STD_SFP_INFO_MAX_SIZE	640

static int ne6x_get_module_eeprom(struct net_device *netdev, struct ethtool_eeprom *ee, u8 *data)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	u8 sfp_data[STD_SFP_INFO_MAX_SIZE];
	int err;

	if (!ee->len || ((ee->len + ee->offset) > STD_SFP_INFO_MAX_SIZE))
		return -EINVAL;

	memset(data, 0, ee->len);
	err = ne6x_dev_get_sfp_eeprom(adpt, sfp_data, ee->offset, ee->len, 0);
	if (err)
		return err;

	memcpy(data, sfp_data + ee->offset, ee->len);

	return 0;
}

static u32 ne6x_get_priv_flags(struct net_device *netdev)
{
	const struct ne6x_priv_flag *priv_flag;
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	u32 is_write_proterct = false;
	u32 i, ret_flags = 0;
	u32 value = 0;

	ne6x_dev_get_norflash_write_protect(adpt->back, &is_write_proterct);
	if (is_write_proterct)
		set_bit(NE6X_ADPT_F_NORFLASH_WRITE_PROTECT, adpt->flags);
	else
		clear_bit(NE6X_ADPT_F_NORFLASH_WRITE_PROTECT, adpt->flags);

	if (ne6x_dev_get_trust_vlan(adpt->back))
		set_bit(NE6X_ADPT_F_TRUST_VLAN, adpt->flags);
	else
		clear_bit(NE6X_ADPT_F_TRUST_VLAN, adpt->flags);
	value = ne6x_dev_get_features(adpt);
	if (value & NE6X_F_RX_FW_LLDP)
		clear_bit(NE6X_ADPT_F_DISABLE_FW_LLDP, adpt->flags);
	else
		set_bit(NE6X_ADPT_F_DISABLE_FW_LLDP, adpt->flags);

	for (i = 0; i < NE6X_PRIV_FLAG_ARRAY_SIZE; i++) {
		priv_flag = &ne6x_gstrings_priv_flags[i];
		if (test_bit(priv_flag->bitno, adpt->flags))
			ret_flags |= BIT(i);
	}

	return ret_flags;
}

static int ne6x_set_priv_flags(struct net_device *netdev, u32 flags)
{
	DECLARE_BITMAP(change_flags, NE6X_ADPT_F_NBITS);
	DECLARE_BITMAP(orig_flags, NE6X_ADPT_F_NBITS);
	const struct ne6x_priv_flag *priv_flag;
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(netdev);
	int ret = 0;
	u32 i;

	if (flags > BIT(NE6X_PRIV_FLAG_ARRAY_SIZE))
		return -EINVAL;

	bitmap_copy(orig_flags, adpt->flags, NE6X_ADPT_F_NBITS);

	for (i = 0; i < NE6X_PRIV_FLAG_ARRAY_SIZE; i++) {
		priv_flag = &ne6x_gstrings_priv_flags[i];

		if (flags & BIT(i))
			set_bit(priv_flag->bitno, adpt->flags);
		else
			clear_bit(priv_flag->bitno, adpt->flags);
	}

	bitmap_xor(change_flags, adpt->flags, orig_flags, NE6X_ADPT_F_NBITS);

	if (test_bit(NE6X_ADPT_F_DISABLE_FW_LLDP, change_flags)) {
		if (test_bit(NE6X_ADPT_F_DISABLE_FW_LLDP, adpt->flags))
			ne6x_dev_set_fw_lldp(adpt, false);
		else
			ne6x_dev_set_fw_lldp(adpt, true);
	}

	if (test_bit(NE6X_ADPT_F_NORFLASH_WRITE_PROTECT, change_flags)) {
		if (test_bit(NE6X_ADPT_F_NORFLASH_WRITE_PROTECT, adpt->flags))
			ne6x_dev_set_norflash_write_protect(adpt->back, true);
		else
			ne6x_dev_set_norflash_write_protect(adpt->back, false);
	}

	if (test_bit(NE6X_ADPT_F_DDOS_SWITCH, change_flags)) {
		if (test_bit(NE6X_ADPT_F_DDOS_SWITCH, adpt->flags))
			ne6x_dev_set_ddos(adpt->back, true);
		else
			ne6x_dev_set_ddos(adpt->back, false);
	}

	if (test_bit(NE6X_ADPT_F_ACL, change_flags)) {
		if (adpt->idx != 0) {
			netdev_err(netdev, "only adpt 0 support acl flag\n");
			return -EINVAL;
		}
		if (test_bit(NE6X_ADPT_F_ACL, adpt->flags)) {
			if (ne6x_dev_set_white_list(adpt->back, true))
				return -EPERM;
		} else {
			ne6x_dev_set_white_list(adpt->back, false);
		}
	}
	if (test_bit(NE6X_ADPT_F_TRUST_VLAN, change_flags)) {
		if (test_bit(NE6X_ADPT_F_TRUST_VLAN, adpt->flags))
			ne6x_dev_set_trust_vlan(adpt->back, true);
		else
			ne6x_dev_set_trust_vlan(adpt->back, false);
	}
	return ret;
}

static int ne6x_get_dump_flag(struct net_device *dev, struct ethtool_dump *dump)
{
	struct ne6x_pf *pf = ne6x_netdev_to_pf(dev);

	dump->version = 1;
	dump->flag = 0;

	/* Calculate the requested preset idx length */
	if (ne6x_dev_get_dump_data_len(pf, &dump->len)) {
		dump->len = 0;
		return -EAGAIN;
	}

	return 0;
}

static int ne6x_get_dump_data(struct net_device *dev, struct ethtool_dump *dump, void *buffer)
{
	struct ne6x_pf *pf = ne6x_netdev_to_pf(dev);
	u32 *p = buffer;

	if (ne6x_dev_get_dump_data(pf, p, dump->len))
		return -EAGAIN;

	return 0;
}

static const struct ethtool_ops ne6x_ethtool_ops = {
	.get_link_ksettings  = ne6x_get_link_ksettings,
	.set_link_ksettings  = ne6x_set_link_ksettings,
	.get_strings         = ne6x_get_strings,
	.get_sset_count      = ne6x_get_sset_count,
	.get_ethtool_stats   = ne6x_get_ethtool_stats,
	.get_drvinfo         = ne6x_get_drvinfo,
	.get_link            = ethtool_op_get_link,
	.get_regs            = ne6x_get_regs,
	.get_regs_len        = ne6x_get_regs_len,
	.get_dump_flag       = ne6x_get_dump_flag,
	.get_dump_data       = ne6x_get_dump_data,
	.self_test           = ne6x_self_test,
	.get_ringparam       = ne6x_get_ringparam,
	.set_ringparam       = ne6x_set_ringparam,
	.get_pauseparam      = ne6x_get_pauseparam,
	.set_pauseparam      = ne6x_set_pauseparam,
	.get_coalesce        = ne6x_get_coalesce,
	.get_eeprom_len      = ne6x_get_eeprom_len,
	.get_eeprom          = ne6x_get_eeprom,
	.get_rxnfc           = ne6x_get_rxnfc,
	.set_rxnfc           = ne6x_set_rxnfc,
	.get_rxfh_key_size   = ne6x_get_rxfh_key_size,
	.get_rxfh_indir_size = ne6x_get_rss_table_size,
	.get_rxfh            = ne6x_get_rxfh,
	.set_rxfh            = ne6x_set_rxfh,
	.get_channels        = ne6x_get_channels,
	.set_channels        = ne6x_set_channels,
	.flash_device        = ne6x_set_flash,
	.reset               = ne6x_reset,
	.get_module_info     = ne6x_get_module_info,
	.get_module_eeprom   = ne6x_get_module_eeprom,
	.get_priv_flags      = ne6x_get_priv_flags,
	.set_priv_flags      = ne6x_set_priv_flags,
	.set_phys_id         = ne6x_set_phys_id,
	.nway_reset          = ne6x_nway_reset,
	.self_test           = ne6x_diag_test,
	.get_fecparam        = ne6x_get_fec_param,
	.set_fecparam        = ne6x_set_fec_param,
};

void ne6x_set_ethtool_ops(struct net_device *dev)
{
	dev->ethtool_ops = &ne6x_ethtool_ops;
}
